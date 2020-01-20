/*
 * Copyright (C) 2012-2018, Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/atomic.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#if defined(CONFIG_ARCH_MSM) && defined(CONFIG_MSM_SCM)
#if defined(CONFIG_ARCH_MSM8939) || defined(CONFIG_ARCH_MSM8996)
#include <asm/cacheflush.h>
#include <soc/qcom/scm.h>
#include <soc/qcom/qseecomi.h>
#else
#error "Unsupported target! The only MSM8996 and MSM8939 are supported for Qualcomm chipset"
#endif
#endif

#include "tzdev.h"
#include "tzlog.h"
#include "tz_iwlog.h"
#include "tz_platform.h"
#include "tz_kthread_pool.h"

/* Define a tzdev device structure for use with dev_debug() etc */
static struct device_driver tzdev_drv = {
	.name = "tzdev"
};

static struct device tzd = {
	.driver = &tzdev_drv
};

static struct device *tzdev_dev = &tzd;

#define QSEE_CE_CLK_100MHZ	100000000

#define QSEE_CLK_ON             0x1
#define QSEE_CLK_OFF            0x0

/* svc_id and cmd_id to call QSEE 3-rd party smc handler */
#define TZ_SVC_EXECUTIVE_EXT		250
#define TZ_CMD_ID_EXEC_SMC_EXT		1

#define SCM_V2_EBUSY			-12

#define SCM_TZM_FNID(s, c) (((((s) & 0xFF) << 8) | ((c) & 0xFF)) | 0x33000000)

#define TZ_EXECUTIVE_EXT_ID_PARAM_ID \
		TZ_SYSCALL_CREATE_PARAM_ID_4( \
		TZ_SYSCALL_PARAM_TYPE_BUF_RW, TZ_SYSCALL_PARAM_TYPE_VAL,\
		TZ_SYSCALL_PARAM_TYPE_BUF_RW, TZ_SYSCALL_PARAM_TYPE_VAL)

static struct workqueue_struct *tzdev_msm_workq;

struct tzdev_msm_work {
	struct work_struct work;
	struct completion done;
	int ret;
	union {
		unsigned long val;
		void *ptr;
	} data;
};

struct tzdev_msm_msg {
	uint32_t p0;
	uint32_t p1;
	uint32_t p2;
	uint32_t p3;
	uint32_t p4;
	uint32_t p5;
	uint32_t p6;
	__s64 tv_sec;
	__s32 tv_nsec;
	uint32_t crypto_clk;
};

struct tzdev_msm_ret_msg {
	uint32_t p0;
	uint32_t p1;
	uint32_t p2;
	uint32_t p3;
	uint32_t timer_remains_ms;
};

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT)
static int tzdev_qc_pm_clock_enable(void);
static void tzdev_qc_pm_clock_disable(void);

static struct clk *tzdev_core_src = NULL;
static struct clk *tzdev_core_clk = NULL;
static struct clk *tzdev_iface_clk = NULL;
static struct clk *tzdev_bus_clk = NULL;
static DEFINE_MUTEX(tzdev_qc_clk_mutex);
static int tzdev_qc_clk_users = 0;
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT */

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG)
static unsigned long tzdev_qc_clk = QSEE_CLK_OFF;
#else /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG */
static unsigned long tzdev_qc_clk = QSEE_CLK_ON;
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG */

static DEFINE_MUTEX(tzdev_init_seq_mutex);

static ssize_t tzdev_run_init_sequence_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;

	(void) dev;
	(void) attr;
	(void) buf;

	mutex_lock(&tzdev_init_seq_mutex);
	ret = tzdev_run_init_sequence();
	mutex_unlock(&tzdev_init_seq_mutex);

	if (ret)
		return ret;

	return count;
}

static DEVICE_ATTR(init_seq, S_IWUSR | S_IWGRP, NULL, tzdev_run_init_sequence_store);

#define BF_SMC_SUCCESS		0
#define BF_SMC_INTERRUPTED	1
#define BF_SMC_KERNEL_PANIC	2

DEFINE_MUTEX(tzdev_smc_lock);

struct hrtimer tzdev_get_event_timer;

static enum hrtimer_restart tzdev_get_event_timer_handler(struct hrtimer *timer)
{
	tz_kthread_pool_cmd_send();

	return HRTIMER_NORESTART;
}

static int tzdev_scm_call(struct tzdev_smc_data *data)
{
	struct tzdev_msm_msg msm_msg = {
		data->args[0], data->args[1], data->args[2], data->args[3], data->args[4],
		data->args[5], data->args[6], 0, 0, tzdev_qc_clk
	};
	struct tzdev_msm_ret_msg ret_msg = {0, 0, 0, 0, 0};
	struct scm_desc desc = {0};
	struct timespec ts;
	void *scm_buf;
	int ret;

	BUG_ON(raw_smp_processor_id() != 0);

	scm_buf = kzalloc(PAGE_ALIGN(sizeof(msm_msg)), GFP_KERNEL);
	if (!scm_buf)
		return -ENOMEM;

	getnstimeofday(&ts);
	msm_msg.tv_sec = ts.tv_sec;
	msm_msg.tv_nsec = ts.tv_nsec;
	memcpy(scm_buf, &msm_msg, sizeof(msm_msg));
	dmac_flush_range(scm_buf, (unsigned char *)scm_buf + sizeof(msm_msg));

	desc.arginfo = TZ_EXECUTIVE_EXT_ID_PARAM_ID;
	desc.args[0] = virt_to_phys(scm_buf);
	desc.args[1] = sizeof(msm_msg);
	desc.args[2] = virt_to_phys(scm_buf);
	desc.args[3] = sizeof(ret_msg);

	mutex_lock(&tzdev_smc_lock);
	hrtimer_cancel(&tzdev_get_event_timer);

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT) && !defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG)
	ret = tzdev_qc_pm_clock_enable();
	if (ret)
		goto out;
#endif

	do {
		ret = scm_call2(SCM_TZM_FNID(TZ_SVC_EXECUTIVE_EXT,
				TZ_CMD_ID_EXEC_SMC_EXT), &desc);
	} while (!ret && desc.ret[0] == BF_SMC_INTERRUPTED);

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT) && !defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG)
	tzdev_qc_pm_clock_disable();
#endif

	if (ret) {
		tzdev_print(0, "scm_call() failed: %d\n", ret);
		if (ret == SCM_V2_EBUSY)
			ret = -EBUSY;
		goto out;
	}

	if (desc.ret[0] == BF_SMC_KERNEL_PANIC) {
		static unsigned int panic_msg_printed;

		tz_iwlog_read_buffers();

		if (!panic_msg_printed) {
			tzdev_print(0, "Secure kernel panicked\n");
			panic_msg_printed = 1;
		}

#if defined(CONFIG_TZDEV_SWD_PANIC_IS_CRITICAL)
		panic("Secure kernel panicked\n");
#else
		ret = -EIO;
		goto out;
#endif
	}

	dmac_flush_range(scm_buf, (unsigned char *)scm_buf + sizeof(ret_msg));
	memcpy(&ret_msg, scm_buf, sizeof(ret_msg));

	if (ret_msg.timer_remains_ms) {
		unsigned long secs;
		unsigned long nsecs;
		secs = ret_msg.timer_remains_ms / MSEC_PER_SEC;
		nsecs = (ret_msg.timer_remains_ms % MSEC_PER_SEC) * NSEC_PER_MSEC;

		hrtimer_start(&tzdev_get_event_timer,
				ktime_set(secs, nsecs), HRTIMER_MODE_REL);
	}

out:
	mutex_unlock(&tzdev_smc_lock);

	kfree(scm_buf);

	data->args[0] = ret_msg.p0;
	data->args[1] = ret_msg.p1;
	data->args[2] = ret_msg.p2;
	data->args[3] = ret_msg.p3;

	return ret;
}

static void tzdev_smc_call_routed(struct work_struct *work)
{
	struct tzdev_msm_work *msm_work = container_of(work, struct tzdev_msm_work, work);

	msm_work->ret = tzdev_scm_call(msm_work->data.ptr);

	complete(&msm_work->done);
}

int tzdev_platform_smc_call(struct tzdev_smc_data *data)
{
	struct tzdev_msm_work msm_work;

	/* If current smc request is schedulable one then it's guaranteed
	 * that request done via kernel worker thread pinned to cpu 0
	 * and such request could be invoked directly.
	 * Otherwise it must be routed to cpu 0.
	 */
	if (likely(data->args[0] == TZDEV_SMC_SCHEDULE))
		return tzdev_scm_call(data);

	INIT_WORK_ONSTACK(&msm_work.work, tzdev_smc_call_routed);
	init_completion(&msm_work.done);
	msm_work.data.ptr = data;

	BUG_ON(!queue_work_on(0, tzdev_msm_workq, &msm_work.work));

	wait_for_completion(&msm_work.done);

	return msm_work.ret;
}

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT)
static int tzdev_qc_pm_clock_initialize(void)
{
	int ret = 0;
	int freq_val = 0;
#if defined(CONFIG_ARCH_MSM8939)
	const char *prop_name = "qcom,freq-val";
#else /* CONFIG_ARCH_MSM8939 */
	const char *prop_name = "qcom,ce-opp-freq";
#endif /* CONFIG_ARCH_MSM8939 */

	tzdev_core_src = clk_get(tzdev_dev, "core_clk_src");
	if (IS_ERR(tzdev_core_src)) {
		tzdev_print(0, "no tzdev_core_src, ret = %d\n", ret);
		ret = PTR_ERR(tzdev_core_src);
		goto error;
	}
	if (of_property_read_u32(tzdev_dev->of_node, prop_name, &freq_val)) {
		freq_val = QSEE_CE_CLK_100MHZ;
		tzdev_print(0, "Unable to get frequency value from \"%s\" property. " \
			       "Set default: %d\n", prop_name, freq_val);
	}
	ret = clk_set_rate(tzdev_core_src, freq_val);
	if (ret) {
		tzdev_print(0, "clk_set_rate failed, ret = %d\n", ret);
		ret = -EIO;
		goto put_core_src_clk;
	}

	tzdev_core_clk = clk_get(tzdev_dev, "core_clk");
	if (IS_ERR(tzdev_core_clk)) {
		tzdev_print(0, "no tzdev_core_clk\n");
		ret = PTR_ERR(tzdev_core_clk);
		goto clear_core_clk;
	}
	tzdev_iface_clk = clk_get(tzdev_dev, "iface_clk");
	if (IS_ERR(tzdev_iface_clk)) {
		tzdev_print(0, "no tzdev_iface_clk\n");
		ret = PTR_ERR(tzdev_iface_clk);
		goto put_core_clk;
	}
	tzdev_bus_clk = clk_get(tzdev_dev, "bus_clk");
	if (IS_ERR(tzdev_bus_clk)) {
		tzdev_print(0, "no tzdev_bus_clk\n");
		ret = PTR_ERR(tzdev_bus_clk);
		goto put_iface_clk;
	}

	tzdev_print(0, "Got QC HW crypto clks\n");
	return ret;

put_iface_clk:
	clk_put(tzdev_iface_clk);
	tzdev_bus_clk = NULL;

put_core_clk:
	clk_put(tzdev_core_clk);
	tzdev_iface_clk = NULL;

clear_core_clk:
	tzdev_core_clk = NULL;

put_core_src_clk:
	clk_put(tzdev_core_src);

error:
	tzdev_core_src = NULL;

	return ret;
}

static void tzdev_qc_pm_clock_finalize(void)
{
	clk_put(tzdev_bus_clk);
	clk_put(tzdev_iface_clk);
	clk_put(tzdev_core_clk);
	clk_put(tzdev_core_src);
}
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT */

static int tzdev_qc_probe(struct platform_device *pdev)
{
	tzdev_dev->of_node = pdev->dev.of_node;

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT)
	tzdev_qc_pm_clock_initialize();
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT */

	hrtimer_init(&tzdev_get_event_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	tzdev_get_event_timer.function = tzdev_get_event_timer_handler;

	return 0;
}

static int tzdev_qc_remove(struct platform_device *pdev)
{
#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT)
	tzdev_qc_pm_clock_finalize();
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT */

	hrtimer_cancel(&tzdev_get_event_timer);

	return 0;
}

static int tzdev_qc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int tzdev_qc_resume(struct platform_device *pdev)
{
	return 0;
}

struct of_device_id tzdev_qc_match[] = {
	{
		.compatible = "qcom,tzd",
	},
	{}
};

struct platform_driver tzdev_qc_plat_driver = {
	.probe = tzdev_qc_probe,
	.remove = tzdev_qc_remove,
	.suspend = tzdev_qc_suspend,
	.resume = tzdev_qc_resume,

	.driver = {
		.name = "tzdev",
		.owner = THIS_MODULE,
		.of_match_table = tzdev_qc_match,
	},
};

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT)
static int tzdev_qc_pm_clock_enable(void)
{
	int ret;

	mutex_lock(&tzdev_qc_clk_mutex);

	ret = clk_prepare_enable(tzdev_core_clk);
	if (ret) {
		tzdev_print(0, "failed to enable core clk, ret=%d\n", ret);
		goto out;
	}

	ret = clk_prepare_enable(tzdev_iface_clk);
	if (ret) {
		tzdev_print(0, "failed to enable iface clk, ret=%d\n", ret);
		goto unprepare_core_clk;
	}

	ret = clk_prepare_enable(tzdev_bus_clk);
	if (ret) {
		tzdev_print(0, "failed to enable bus clk, ret=%d\n\n", ret);
		goto unprepare_iface_clk;
	}

	tzdev_qc_clk_users++;

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG)
	if (tzdev_qc_clk_users == 1)
		tzdev_qc_clk = QSEE_CLK_ON;
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG */

	goto out;

unprepare_iface_clk:
	clk_disable_unprepare(tzdev_iface_clk);
unprepare_core_clk:
	clk_disable_unprepare(tzdev_core_clk);
out:
	mutex_unlock(&tzdev_qc_clk_mutex);

	return ret;
}

static void tzdev_qc_pm_clock_disable(void)
{
	mutex_lock(&tzdev_qc_clk_mutex);

	tzdev_qc_clk_users--;
	BUG_ON(tzdev_qc_clk_users < 0);

	clk_disable_unprepare(tzdev_iface_clk);
	clk_disable_unprepare(tzdev_core_clk);
	clk_disable_unprepare(tzdev_bus_clk);

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG)
	if (!tzdev_qc_clk_users)
		tzdev_qc_clk = QSEE_CLK_OFF;
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG */

	mutex_unlock(&tzdev_qc_clk_mutex);
}
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_MANAGEMENT */

int tzdev_platform_register(void)
{
	return platform_driver_register(&tzdev_qc_plat_driver);
}

void tzdev_platform_unregister(void)
{
	platform_driver_unregister(&tzdev_qc_plat_driver);
}

int tzdev_platform_init(void)
{
	struct workqueue_struct *workq;
	struct workqueue_attrs *attrs;
	struct tz_cdev *tzdev_cdev;
	int ret;

	tzdev_cdev = tzdev_get_cdev();

	ret = device_create_file(tzdev_cdev->device, &dev_attr_init_seq);
	if (ret) {
		tzdev_print(0, "device_create_file() failed, error=%d\n", ret);
		return ret;
	}

	workq = create_singlethread_workqueue("tzdev_msm_workq");
	if (!workq) {
		tzdev_print(0, "create_singlethread_workqueue() failed\n");
		ret = -ENOMEM;
		goto out_free_sysfs;
	}

	attrs = alloc_workqueue_attrs(GFP_KERNEL);
	if (!attrs) {
		tzdev_print(0, "alloc_workqueue_attrs() failed\n");
		ret = -ENOMEM;
		goto out_free_workq;
	}

	attrs->nice = 0;
	attrs->no_numa = true;

	/* Bind workqueue to cpu 0 */
	cpumask_clear(attrs->cpumask);
	cpumask_set_cpu(0, attrs->cpumask);

	ret = apply_workqueue_attrs(workq, attrs);
	if (ret) {
		tzdev_print(0, "apply_workqueue_attrs() failed, ret=%d\n", ret);
		goto out_free_attrs;
	}

	free_workqueue_attrs(attrs);

	tzdev_msm_workq = workq;

	return 0;

out_free_attrs:
	free_workqueue_attrs(attrs);
out_free_workq:
	destroy_workqueue(workq);
out_free_sysfs:
	device_remove_file(tzdev_cdev->device, &dev_attr_init_seq);

	return ret;
}

int tzdev_platform_fini(void)
{
	if (tzdev_msm_workq) {
		flush_workqueue(tzdev_msm_workq);
		destroy_workqueue(tzdev_msm_workq);
		tzdev_msm_workq = NULL;
	}

	return 0;
}

int tzdev_platform_open(struct inode *inode, struct file *filp)
{
	(void)inode;
	(void)filp;

	return tzdev_run_init_sequence();
}

#if defined(CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG)
static int tzdev_set_crypto_clk(struct file *filp, unsigned int state)
{
	struct tzdev_fd_data *data = filp->private_data;
	int ret = 0;

	mutex_lock(&data->mutex);

	switch (state) {
	case TZIO_CRYPTO_CLOCK_ON:
		if (!data->crypto_clk_state) {
			ret = tzdev_qc_pm_clock_enable();
			if (!ret)
				data->crypto_clk_state = 1;
		} else {
			tzdev_print(0, "Trying to enable crypto clocks twice, filp=%pK\n", filp);
			ret = -EBUSY;
		}
		break;
	case TZIO_CRYPTO_CLOCK_OFF:
		if (data->crypto_clk_state) {
			tzdev_qc_pm_clock_disable();
			data->crypto_clk_state = 0;
		} else {
			tzdev_print(0, "Trying to disable crypto clocks twice, filp=%pK\n", filp);
			ret = -EBUSY;
		}
		break;
	default:
		tzdev_print(0, "Unknown crypto clocks request, state=%u filp=%pK\n", state, filp);
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&data->mutex);

	return ret;
}

long tzdev_fd_platform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case TZIO_CRYPTO_CLOCK_CONTROL:
		return tzdev_set_crypto_clk(filp, arg);
	default:
		return -ENOTTY;
	}
}

int tzdev_fd_platform_close(struct inode *inode, struct file *filp)
{
	struct tzdev_fd_data *data = filp->private_data;

	(void)inode;

	if (data->crypto_clk_state)
		tzdev_set_crypto_clk(filp, TZIO_CRYPTO_CLOCK_OFF);

	return 0;
}

uint32_t tzdev_platform_get_sysconf_flags(void)
{
	uint32_t flags = 0;

	flags |= SYSCONF_NWD_CRYPTO_CLOCK_MANAGEMENT;

	return flags;
}
#endif /* CONFIG_TZDEV_QC_CRYPTO_CLOCKS_USR_MNG */
