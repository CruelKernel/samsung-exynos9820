/* iva_ctrl.c
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 * Authors:
 *	Ilkwan Kim <ilkwan.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/pm_runtime.h>
#include <linux/clk-provider.h>
#include <linux/of_device.h>
#include <linux/sched/task.h>
#include <linux/pm_opp.h>
#ifdef CONFIG_EXT_BOOTMEM
#include <linux/ext_bootmem.h>
#endif
#ifdef CONFIG_EXYNOS_IOVMM
#include <linux/exynos_iovmm.h>
#endif
#ifdef CONFIG_EXYNOS_IMA
#include <exynos_ima.h>
#endif
#include "iva_ctrl_ioctl.h"
#include "regs/iva_base_addr.h"
#include "iva_ipc_queue.h"
#include "iva_ctrl.h"
#include "iva_mbox.h"
#include "iva_mcu.h"
#include "iva_pmu.h"
#include "iva_sh_mem.h"
#include "iva_mem.h"
#include "iva_sysfs.h"
#include "iva_ipc_header.h"
#include "iva_vdma.h"

#define ENABLE_MMAP_US
#undef ENABLE_FPGA_REG_ACCESS
#undef ENABLE_LOCAL_PLAT_DEVICE
#undef ENABLE_MEASURE_IVA_PERF
#undef ENABLE_IVA_PANIC_HANDLER
#undef ENABLE_MCU_EXIT_AT_SHUTDOWN
#undef ENABLE_POWER_OFF_AT_SHUTDOWN
#undef ENABLE_SYSMMU_FAULT_REPORT	/* depening on sysmmu driver impl. */

#ifdef MODULE
#define IVA_CTRL_NAME			"iva_ctrl"
#define IVA_CTRL_MAJOR			(63)
#endif
#define IVA_MBOX_IRQ_RSC_NAME		"iva_mbox_irq"

#ifdef CONFIG_ARCH_EXYNOS
#if defined(CONFIG_SOC_EXYNOS8895)
#define IVA_PM_QOS_IVA			(PM_QOS_CAM_THROUGHPUT)
#define IVA_PM_QOS_IVA_REQ		(670000)	/* L2, L0:(690000)*/
#elif defined(CONFIG_SOC_EXYNOS9810)
#define IVA_PM_QOS_IVA			(PM_QOS_IVA_THROUGHPUT)
#define IVA_PM_QOS_IVA_REQ		(534000)
#define IVA_PM_IVA_MIN			(34000)
#elif defined(CONFIG_SOC_EXYNOS9820)
#define IVA_CTL_QOS_DEV
#define IVA_PM_QOS_IVA			(PM_QOS_IVA_THROUGHPUT)
#define IVA_PM_QOS_IVA_REQ		(534000)
#define IVA_PM_QOS_DEV			(PM_QOS_DEVICE_THROUGHPUT)
#define IVA_PM_QOS_DEV_REQ		(400000)
#define IVA_PM_QOS_MIF			(PM_QOS_BUS_THROUGHPUT)
#define IVA_PM_QOS_MIF_REQ		(1794000)
#define IVA_PM_IVA_MIN			(34000)
#else
#undef IVA_PM_QOS_IVA
#undef IVA_PM_QOS_IVA_REQ
#define IVA_PM_IVA_MIN			(10000)
#endif
#endif

static struct iva_dev_data *g_iva_data;

static int32_t __iva_ctrl_init(struct device *dev)
{
	int ret;
	struct iva_dev_data *iva = dev_get_drvdata(dev);
	struct clk *iva_clk = iva->iva_clk;

#ifdef ENABLE_FPGA_REG_ACCESS
	uint32_t	val;
	void __iomem	*peri_regs;

	/* TO DO : check again later */
	/* enable the Qrequest otherwise IVA will start going into low-power mode.
	 * this is required for all FPGA >= v29
	 * Also write 1 to bit 4 which is Qchannel_en (CMU model in FPGA).
	 * Avail in FPGA >= 42b
	 */
	peri_regs = devm_ioremap(dev, PERI_PHY_BASE_ADDR, PERI_SIZE);
	if (!peri_regs) {
		dev_err(dev, "%s() fail to ioremap. peri phys(0x%x), size(0x%x)\n",
			__func__, PERI_PHY_BASE_ADDR, PERI_SIZE);
		return -ENOMEM;
	}
	writel(0x12, peri_regs + 0x1008);

	/* version register. if upper 8-bits are non-zero then "assume" to be in
	 * 2-FPGA build environment where SCore FPGA version is contained in bits
	 * 31:24 and IVA FPGA in 23:16
	 */
	val = readl(peri_regs + 0x1000);
	if (val & 0xff000000)
		dev_info(dev, "%s() FPGA ver: IVA=%d SCore=%d\n",
			__func__, (val >> 16) & val, val >> 24);
	else
		dev_info(dev, "%s() FPGA ver: %d\n", __func__, val >> 16);

	devm_iounmap(dev, peri_regs);
#endif

	dev_dbg(dev, "%s() init iva state(0x%lx) with hwa en(%d)\n",
			__func__, iva->state, iva->en_hwa_req);

	if (test_and_set_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_warn(dev, "%s() already iva inited(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

	if (iva_clk) {
		ret = clk_prepare_enable(iva_clk);
		if (ret) {
			dev_err(dev, "fail to enable iva_clk(%d)\n", ret);
			return ret;
		}
	#if 0
		/* confirm */
		if (!__clk_is_enabled(iva_clk)) {
			dev_err(dev, "%s() req iva_clk on but off\n", __func__);
			return -EINVAL;
		}
	#endif
		dev_dbg(dev, "%s() success to enable iva_clk with rate(%ld)\n",
			__func__, clk_get_rate(iva_clk));
	}

#ifdef CONFIG_EXYNOS_IMA
	ima_host_begin();
#endif
	iva_pmu_init(iva, iva->en_hwa_req);

	/* iva mbox supports both of iva and score. */
	iva_mbox_init(iva);
	iva_ipcq_init_pending_mail(&iva->mcu_ipcq);

	return 0;

}

static int32_t __iva_ctrl_deinit(struct device *dev)
{
	struct iva_dev_data *iva = dev_get_drvdata(dev);
	struct clk *iva_clk = iva->iva_clk;

	if (!test_and_clear_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_warn(dev, "%s() already iva deinited(0x%lx)\n",
				__func__, iva->state);
		return 0;
	}

	dev_dbg(dev, "%s()\n", __func__);

	iva_mbox_deinit(iva);
	iva_pmu_deinit(iva);

#ifdef CONFIG_EXYNOS_IMA
	ima_host_end();
#endif
	if (iva_clk) {
		clk_disable_unprepare(iva_clk);
	#if 1	/* check */
		if (__clk_is_enabled(iva_clk)) {
			dev_err(dev, "%s() req iva_clk off but on\n", __func__);
			return -EINVAL;
		}
	#endif
	}

	return 0;
}

static int32_t iva_ctrl_init(struct iva_proc *proc, bool en_hwa)
{
	int			ret;
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;

	/* already init_ref > 0 */
	if (kref_get_unless_zero(&proc->init_ref)) {
		dev_info(dev, "%s() already proc init requested\n", __func__);
		goto init_skip;
	}
	kref_init(&proc->init_ref);

	/* This process requests iva initialization, for the first time. */
	if (kref_get_unless_zero(&iva->glob_init_ref)) {
		dev_info(dev, "%s() already iva initialized\n", __func__);
		goto init_skip;
	}

	/* initialize the iva globally, for the first time */
	iva->en_hwa_req = en_hwa;

#ifdef CONFIG_PM
	ret = pm_runtime_get_sync(dev);
#else
	ret = __iva_ctrl_init(dev);
#endif
	if (ret < 0) {
		dev_err(dev, "%s() fail to enable iva clk/pwr. ret(%d)\n",
				__func__, ret);
		refcount_set(&proc->init_ref.refcount, 0);
		return ret;
	}

	/* now initialize global krefs */
	kref_init(&iva->glob_init_ref);

	dev_dbg(dev, "%s() rt ret(%d) iva clk rate(%ld)\n", __func__,
			ret, clk_get_rate(iva->iva_clk));
init_skip:
	dev_dbg(dev, "%s() glob_init_ref(%d) <- init_ref(%d)\n",
			__func__,
			kref_read(&iva->glob_init_ref),
			kref_read(&proc->init_ref));
	return 0;
}

static inline int32_t iva_ctrl_init_usr(struct iva_proc *proc,
		uint32_t __user *en_hwa_usr)
{
	uint32_t en_hwa;

	get_user(en_hwa, en_hwa_usr);

	return iva_ctrl_init(proc, en_hwa);
}

static void iva_ctrl_release_global_init_ref(struct kref *kref)
{
	int ret;
	struct iva_dev_data *iva =
			container_of(kref, struct iva_dev_data, glob_init_ref);
	struct device *dev = iva->dev;

	if (!test_bit(IVA_ST_INIT_DONE, &iva->state)) {
		dev_info(dev, "%s() already iva deinited(0x%lx). ",
			__func__, iva->state);
		dev_info(dev, "plz check the pair of init/deinit\n");
		return;
	}

#ifdef CONFIG_PM
	ret = pm_runtime_put_sync(dev);
#else
	ret = __iva_ctrl_deinit(dev);
#endif
	if (ret < 0) {
		dev_err(dev, "%s() fail to deinit iva(%d)\n", __func__, ret);
		return;
	}

	dev_dbg(dev, "%s()\n", __func__);
}

static void iva_ctrl_release_proc_init_ref(struct kref *kref)
{
	struct iva_proc		*proc = container_of(kref, struct iva_proc, init_ref);
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
	int ret;

	/*
	 * if this is called,
	 * this process want to deinit iva finally.
	 * now this request is propagated globally.
	 */

	ret = kref_put(&iva->glob_init_ref, iva_ctrl_release_global_init_ref);
	if (!ret) {
		dev_info(dev, "%s() still global init requesters in a system. ",
			__func__);
		dev_info(dev, "glob_init_ref(%d)\n",
			kref_read(&iva->glob_init_ref));
	}
}

static int32_t iva_ctrl_deinit(struct iva_proc *proc, bool forced)
{
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
	unsigned int	put_cnt;

	put_cnt = kref_read(&proc->init_ref);
	if (!put_cnt)
		return 0;

	if (forced) {
		dev_warn(dev, "%s() forced init requester removal,", __func__);
		dev_warn(dev, " put_cnt(%d) == init_ref(%d)\n",
			put_cnt, kref_read(&proc->init_ref));

	} else
		put_cnt = 1;

	/* instead of kref_sub */
	if (refcount_sub_and_test(put_cnt, &proc->init_ref.refcount)) {
		iva_ctrl_release_proc_init_ref(&proc->init_ref);
	} else {
		dev_info(dev, "%s() still init requesters in the process. ",
			__func__);
		dev_warn(dev, "glob_init_ref(%d) <- init_ref(%d)\n",
			kref_read(&iva->glob_init_ref),
			kref_read(&proc->init_ref));
	}

	/* always return as successful */
	return 0;
}

static void __maybe_unused iva_ctrl_deinit_forced(struct iva_dev_data *iva)
{
	struct list_head	*work_node;
	struct iva_proc		*proc;

	list_for_each(work_node, &iva->proc_head) {
		proc = list_entry(work_node, struct iva_proc, proc_node);
		iva_ctrl_deinit(proc, true/*forced*/);
	}
}

static void iva_ctrl_mcu_boot_prepare_system(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;
	int dev_qos_req = 0;
#ifdef CONFIG_PM_SLEEP
	/* prevent the system to suspend */
	if (!wake_lock_active(&iva->iva_wake_lock)) {
		wake_lock(&iva->iva_wake_lock);
		dev_dbg(dev, "%s() wake_lock, now(%d)\n",
				__func__, wake_lock_active(&iva->iva_wake_lock));
	}
#endif

#ifdef IVA_PM_QOS_IVA
	if (!test_and_set_bit(IVA_ST_PM_QOS_IVA, &iva->state)) {
		dev_dbg(dev, "%s() request iva pm_qos(0x%lx), rate(%d)\n",
				__func__, iva->state, iva->iva_qos_rate);
		pm_qos_update_request(&iva->iva_qos_iva, iva->iva_qos_rate);
#ifdef IVA_CTL_QOS_DEV
		if (iva->iva_bus_qos_en) {
			dev_qos_req = IVA_PM_QOS_DEV_REQ;
		}
		pm_qos_update_request(&iva->iva_qos_dev, dev_qos_req);
#endif
	}
#endif
	dev_dbg(dev, "%s() iva_clk rate(%ld)\n", __func__,
				clk_get_rate(iva->iva_clk));
}

static void iva_ctrl_mcu_boot_unprepare_system(struct iva_dev_data *iva)
{
	struct device *dev = iva->dev;

#ifdef IVA_PM_QOS_IVA
	if (test_and_clear_bit(IVA_ST_PM_QOS_IVA, &iva->state)) {
		pm_qos_update_request(&iva->iva_qos_iva, 0);
#ifdef IVA_CTL_QOS_DEV
		pm_qos_update_request(&iva->iva_qos_dev, 0);
#endif
		clear_bit(IVA_ST_PM_QOS_IVA, &iva->state);
	}
#endif
#ifdef CONFIG_PM_SLEEP
	if (wake_lock_active(&iva->iva_wake_lock)) {
		wake_unlock(&iva->iva_wake_lock);
		dev_info(dev, "%s() wake_unlock, now(%d)\n",
			__func__, wake_lock_active(&iva->iva_wake_lock));
	}
#endif
	dev_dbg(dev, "%s() iva_clk rate(%ld)\n", __func__,
			clk_get_rate(iva->iva_clk));
}

static int32_t iva_ctrl_mcu_boot_file(struct iva_proc *proc,
			const char __user *mcu_file_u, int32_t wait_ready)
{
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
	int	name_len;
	char	mcu_file[IVA_CTRL_BOOT_FILE_LEN];
	int32_t ret;

	if (!iva_ctrl_is_iva_on(iva)) {
		dev_dbg(dev, "%s() mcu boot request w/o iva init(0x%lx)\n",
				__func__, iva->state);

		return -EIO;
	}

	name_len = strncpy_from_user(mcu_file, mcu_file_u, sizeof(mcu_file) - 1);
	mcu_file[sizeof(mcu_file) - 1] = 0x0;
	if (name_len <= 0) {
		dev_err(dev, "%s() unexpected name_len(%d)\n",
				__func__, name_len);
		return name_len;
	}

	/* already boot_ref > 0 */
	if (kref_get_unless_zero(&proc->boot_ref)) {
		dev_info(dev, "%s() already proc boot requested\n", __func__);
		goto boot_skip;
	}
	kref_init(&proc->boot_ref);

	/* This process requests iva boot, for the first time. */
	if (kref_get_unless_zero(&iva->glob_boot_ref)) {
		dev_info(dev, "%s() already iva booted.\n", __func__);
		goto boot_skip;
	}

	iva_ctrl_mcu_boot_prepare_system(iva);

	ret = iva_mcu_boot_file(iva, mcu_file, wait_ready);
	if (ret) {	/* error */
		iva_ctrl_mcu_boot_unprepare_system(iva);

		refcount_set(&proc->boot_ref.refcount, 0);
		return ret;
	}

	/* now initialize kref */
	kref_init(&iva->glob_boot_ref);
	dev_dbg(dev, "%s() success to boot mcu\n", __func__);

boot_skip:
	dev_dbg(dev, "%s() glob_init_ref(%d) <- boot_ref(%d)\n",
			__func__,
			kref_read(&iva->glob_init_ref),
			kref_read(&proc->init_ref));
	return 0;
}

static void iva_ctrl_release_global_boot_ref(struct kref *kref)
{
	struct iva_dev_data *iva =
			container_of(kref, struct iva_dev_data, glob_boot_ref);

	iva_mcu_exit(iva);
	iva_ctrl_mcu_boot_unprepare_system(iva);
}

static void iva_ctrl_release_proc_boot_ref(struct kref *kref)
{
	struct iva_proc		*proc = container_of(kref, struct iva_proc, boot_ref);
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
	int ret;

	/*
	 * if this is called, this process want to exit mcu finally.
	 *	now this request is propagated globally.
	 */

	ret = kref_put(&iva->glob_boot_ref, iva_ctrl_release_global_boot_ref);
	if (!ret) {
		dev_info(dev, "%s() still global boot requesters in a system.",
			__func__);
		dev_info(dev, "glob_boot_ref(%d)\n",
			kref_read(&iva->glob_boot_ref));
	}
}

static int32_t iva_ctrl_mcu_exit(struct iva_proc *proc, bool forced)
{
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
	unsigned int	put_cnt;

	put_cnt = kref_read(&proc->boot_ref);
	if (!put_cnt)
		return 0;

	if (forced) {
		dev_warn(dev, "%s() forced mcu exit requester removal, ",
			__func__);
		dev_warn(dev, "put_cnt(%d) == boot_ref(%d)\n",
			put_cnt, kref_read(&proc->boot_ref));

	} else
		put_cnt = 1;

	if (refcount_sub_and_test(put_cnt, &proc->boot_ref.refcount)) {
		iva_ctrl_release_proc_boot_ref(&proc->boot_ref);
	} else {
		dev_info(dev, "%s() still mcu boot requesters in the process.",
			__func__);
		dev_info(dev, "glob_boot_ref(%d) <- boot_ref(%d)\n",
			kref_read(&iva->glob_boot_ref),
			kref_read(&proc->boot_ref));
	}

	/* always return as successful */
	return 0;
}

static void __maybe_unused iva_ctrl_mcu_exit_forced(struct iva_dev_data *iva)
{
	struct list_head	*work_node;
	struct iva_proc		*proc;

	list_for_each(work_node, &iva->proc_head) {
		proc = list_entry(work_node, struct iva_proc, proc_node);
		iva_ctrl_mcu_exit(proc, true/*forced*/);
	}
}

bool iva_ctrl_is_iva_on(struct iva_dev_data *iva)
{
	return !!kref_read(&iva->glob_init_ref);
}

bool iva_ctrl_is_boot(struct iva_dev_data *iva)
{
	return !!kref_read(&iva->glob_boot_ref);
}

static int32_t iva_ctrl_get_iva_status_usr(struct iva_dev_data *iva,
		struct iva_status_param __user *st_usr)
{
	if (!st_usr) {
		dev_err(iva->dev, "no usr pointer provided.\n");
		return -EINVAL;
	}

	if (put_user((uint32_t)iva->state, (uint32_t *) &st_usr->status))
		return -EFAULT;

	if (put_user((uint32_t)iva->iva_qos_rate, (uint32_t *) &st_usr->clk_rate))
		return -EFAULT;

	return 0;
}

/* Rule should be aligned with user layer. */
#define BUS_CLK_FLAG_S (24)
#define IVA_UNPACK_CLK_REQ(val) (val & ((1 << BUS_CLK_FLAG_S) - 1))
#define IVA_UNPACK_BUS_CLK_FLAG(val) (val & (1 << BUS_CLK_FLAG_S))

static int32_t iva_ctrl_set_clk_usr(struct iva_dev_data *iva,
		uint32_t __user *usr_clk_rate)
{
	struct device	*dev = iva->dev;
	uint32_t	usr_req, clk_req;
	unsigned long	freq = 0;

	get_user(usr_req, usr_clk_rate);

	clk_req = IVA_UNPACK_CLK_REQ(usr_req);
	if (iva->iva_dvfs_dev) {
		struct device *dvfs_dev = &iva->iva_dvfs_dev->dev;

		while (!IS_ERR(dev_pm_opp_find_freq_ceil(dvfs_dev, &freq))) {
			if (freq == clk_req)
				break;
			freq++;
		}
	}
	dev_info(dev, "IVA: change freq of IVA %d -> %d KHz\n",
					iva->iva_qos_rate, clk_req);

	if (freq == clk_req)
		iva->iva_qos_rate = clk_req;
	else
		dev_warn(dev, "fail to set qos rate, req(%u KHz), cur(%d KHz)\n",
				clk_req, iva->iva_qos_rate);

	dev_dbg(dev, "%s() iva_clk rate(%ld)\n", __func__,
			clk_get_rate(iva->iva_clk));

	iva->iva_bus_qos_en = IVA_UNPACK_BUS_CLK_FLAG(usr_req);
	return 0;
}

const char *iva_ctrl_get_ioctl_str(unsigned int cmd)
{
	#define IOCTL_STR_ENTRY(cmd_nr, cmd_str)[cmd_nr] = cmd_str
	static const char * const iva_ioctl_str[] = {
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_INIT_IVA),		"INIT_IVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_DEINIT_IVA),		"DEINIT_IVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_BOOT_MCU),		"BOOT_MCU"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_EXIT_MCU),		"EXIT_MCU"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_GET_STATUS),		"GET_IVA_STATUS"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_IPC_QUEUE_SEND_CMD),	"IPCQ_SEND"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_IPC_QUEUE_RECEIVE_RSP),	"IPCQ_RECEIVE"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_GET_IOVA),		"ION_GET_IOVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_PUT_IOVA),		"ION_PUT_IOVA"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_ALLOC),			"ION_ALLOC"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_FREE),			"ION_FREE"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_SYNC_FOR_CPU),		"ION_SYNC_FOR_CPU"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_ION_SYNC_FOR_DEVICE),	"ION_SYNC_FOR_DEVICE"),
		IOCTL_STR_ENTRY(_IOC_NR(IVA_CTRL_SET_CLK),		"SET_IVA_CLK"),
	};

	unsigned int cmd_nr = _IOC_NR(cmd);

	if (cmd_nr >= (unsigned int) ARRAY_SIZE(iva_ioctl_str))
		return "UNKNOWN IOCTL";

	return iva_ioctl_str[cmd_nr];
}

static inline s32 __maybe_unused iva_timeval_diff_us(struct timeval *a,
		struct timeval *b)
{
	return (a->tv_sec - b->tv_sec) * USEC_PER_SEC +
			(a->tv_usec - b->tv_usec);
}

static long iva_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user		*p = (void __user *)arg;
	struct device		*dev;
	int			ret;
	struct iva_dev_data	*iva;
	struct iva_proc		*proc;

#ifdef ENABLE_MEASURE_IVA_PERF
	struct timeval before, after;

	do_gettimeofday(&before);
#endif
	proc	= (struct iva_proc *) filp->private_data;
	iva	= proc->iva_data;
	dev	= iva->dev;

	dev_dbg(dev, "%s() cmd(%s) arg(0x%p)\n", __func__,
			iva_ctrl_get_ioctl_str(cmd), p);

	if (_IOC_TYPE(cmd) != IVA_CTRL_MAGIC || !p) {
		dev_warn(dev, "%s() check the your ioctl cmd(0x%x) arg(0x%p)\n",
				__func__, cmd, p);
		return -EINVAL;
	}

	switch (cmd) {
	case IVA_CTRL_INIT_IVA:
		ret = iva_ctrl_init_usr(proc, (uint32_t __user *) p);
		break;
	case IVA_CTRL_DEINIT_IVA:
		ret = iva_ctrl_deinit(proc, false);
		break;
	case IVA_CTRL_BOOT_MCU:
		ret = iva_ctrl_mcu_boot_file(proc,
				(const char __user *) p, false);
		break;
	case IVA_CTRL_EXIT_MCU:
		ret = iva_ctrl_mcu_exit(proc, false);
		break;
	case IVA_CTRL_GET_STATUS:
		ret = iva_ctrl_get_iva_status_usr(iva, (struct iva_status_param __user *) p);
		break;
	case IVA_IPC_QUEUE_SEND_CMD:
		ret = iva_ipcq_send_cmd_usr(proc, (struct ipc_cmd_param __user *) p);
		break;
	case IVA_IPC_QUEUE_RECEIVE_RSP:
		ret = iva_ipcq_wait_res_usr(proc, (struct ipc_res_param __user *) p);
		break;
	case IVA_CTRL_SET_CLK:
		ret = iva_ctrl_set_clk_usr(iva, (uint32_t __user *) p);
		break;
	case IVA_ION_GET_IOVA:
	case IVA_ION_PUT_IOVA:
	case IVA_ION_ALLOC:
	case IVA_ION_FREE: /* This may not be used */
	case IVA_ION_SYNC_FOR_CPU:
	case IVA_ION_SYNC_FOR_DEVICE:
		ret = iva_mem_ioctl(proc, cmd, p);
		break;
	default:
		dev_warn(dev, "%s() unhandled cmd 0x%x",
				__func__, cmd);
		ret = -ENOTTY;
		break;
	}

#ifdef ENABLE_MEASURE_IVA_PERF
	do_gettimeofday(&after);
	dev_info(dev, "%s(%d usec)\n", iva_ctrl_get_ioctl_str(cmd),
			iva_timeval_diff_us(&after, &before));
#endif
	return ret;
}


static int iva_ctrl_mmap(struct file *filp, struct vm_area_struct *vma)
{
#ifndef MODULE
	struct iva_proc		*proc = (struct iva_proc *) filp->private_data;
	struct iva_dev_data	*iva = proc->iva_data;
	struct device		*dev = iva->dev;
#else
	struct iva_dev_data	*iva = filp->private_data;
	struct device		*dev = iva->dev;
#endif

#ifdef ENABLE_MMAP_US
	int		ret;
	unsigned long	req_size = vma->vm_end - vma->vm_start;
	unsigned long	phys_iva = iva->iva_res->start;
	unsigned long	req_phys_iva = vma->vm_pgoff << PAGE_SHIFT;

	if ((req_phys_iva < phys_iva) || ((req_phys_iva + req_size) >
			(phys_iva + resource_size(iva->iva_res)))) {
		dev_warn(dev, "%s() not allowed to map the region(0x%lx-0x%lx)",
				__func__,
				req_phys_iva, req_phys_iva + req_size);
		return -EPERM;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			  req_size, vma->vm_page_prot);
	if (ret) {
		dev_err(dev, "%s() fail to remap, ret(%d), start(0x%lx), ",
			 __func__, ret, vma->vm_start);
		dev_err(dev, "end(0x%lx), pgoff(0x%lx)\n",
			vma->vm_end, vma->vm_pgoff);
		return ret;
	}

	dev_dbg(dev, "%s() success!!! start(0x%lx), end(0x%lx),	pgoff(0x%lx)\n",
		__func__, vma->vm_start, vma->vm_end, vma->vm_pgoff);
	return 0;
#else
	dev_err(dev, "%s() not allowed to map with start(0x%lx), ",
		__func__, vma->vm_start);
	dev_err(dev, "end(0x%lx), pgoff(0x%lx)\n", vma->vm_end, vma->vm_pgoff);
	return -EPERM;	/* error: not allowed */
#endif
}

static int iva_ctrl_open(struct inode *inode, struct file *filp)
{
	struct device *dev;
	struct iva_dev_data	*iva;
	struct iva_proc		*proc;
#ifndef MODULE
	struct miscdevice *miscdev;

	miscdev = filp->private_data;
	dev = miscdev->parent;
	iva = dev_get_drvdata(dev);
#else
	/* smarter */
	iva = filp->private_data = g_iva_data;
	dev = iva->dev;
#endif
	mutex_lock(&iva->proc_mutex);
	if (!list_empty(&iva->proc_head)) {
		struct list_head	*work_node;
		struct iva_proc		*proc;

		list_for_each(work_node, &iva->proc_head) {
			proc = list_entry(work_node, struct iva_proc, proc_node);
			dev_err(dev, "%s() iva_proc is already open(%s, %d, %d)\n",
				__func__, proc->tsk->comm, proc->pid, proc->tid);
		}
		mutex_unlock(&iva->proc_mutex);
		return -EMFILE;
	}
	proc = devm_kmalloc(dev, sizeof(*proc), GFP_KERNEL);
	if (!proc) {
		dev_err(dev, "%s() fail to alloc mem for struct iva_proc\n",
			__func__);
		mutex_unlock(&iva->proc_mutex);
		return -ENOMEM;
	}
	/* assume only one open() per process */
	get_task_struct(current);
	proc->tsk	= current;
	proc->pid	= current->group_leader->pid;
	proc->tid	= current->pid;
	proc->iva_data	= iva;

	iva_mem_init_proc_mem(proc);

	refcount_set(&proc->init_ref.refcount, 0);
	refcount_set(&proc->boot_ref.refcount, 0);

	list_add(&proc->proc_node, &iva->proc_head);

	/* update file->private to hold struct iva_proc */
	filp->private_data = (void *) proc;
	mutex_unlock(&iva->proc_mutex);

	dev_dbg(dev, "%s() succeed to open from (%s, %d, %d)\n", __func__,
			proc->tsk->comm, proc->pid, proc->tid);
	return 0;
}

static int iva_ctrl_release(struct inode *inode, struct file *filp)
{
	struct device *dev;
	struct iva_dev_data	*iva;
	struct iva_proc		*proc;

	proc	= (struct iva_proc *) filp->private_data;
	iva	= proc->iva_data;
	dev	= iva->dev;

	mutex_lock(&iva->proc_mutex);
	list_del(&proc->proc_node);
	mutex_unlock(&iva->proc_mutex);

	dev_dbg(dev, "%s() try to close from (%s, %d, %d)\n", __func__,
			proc->tsk->comm, proc->pid, proc->tid);

	/* to do : per process */
	dev_dbg(dev, "%s() state(0x%lx)\n", __func__, iva->state);

	/* forced to release all iva mcu exit requests from this process */
	iva_ctrl_mcu_exit(proc, true);

	/* forced to release all iva init requests from this process */
	iva_ctrl_deinit(proc, true);

	iva_mem_deinit_proc_mem(proc);
	put_task_struct(proc->tsk);
	devm_kfree(dev, proc);
	return 0;
}

static const struct file_operations iva_ctl_fops = {
	.owner		= THIS_MODULE,
	.open           = iva_ctrl_open,
	.release	= iva_ctrl_release,
	.unlocked_ioctl	= iva_ctrl_ioctl,
	.compat_ioctl	= iva_ctrl_ioctl,
	.mmap		= iva_ctrl_mmap
};

#ifdef CONFIG_EXYNOS_IOVMM
static int iva_iommu_fault_handler(struct iommu_domain *domain,
		struct device *dev, unsigned long fault_addr,
		int fault_flags, void *token)
{
	struct iva_dev_data	*iva = token;

	dev_err(dev, "%s() is called with fault addr(0x%lx), err_cnt(%d)\n",
			__func__, fault_addr, atomic_read(&iva->mcu_err_cnt));
#ifdef ENABLE_SYSMMU_FAULT_REPORT
	iva_mcu_handle_error_k(iva, mcu_err_from_irq, 35 /* SYSMMU_FAULT */);
#else
	iva_mcu_show_status(iva);
	iva_mem_show_global_mapped_list_nolock(iva);
#endif
	return -EAGAIN;
}
#endif

#ifdef ENABLE_IVA_PANIC_HANDLER
static int iva_ctrl_catch_panic_event(struct notifier_block *nb,
		unsigned long event, void *data)
{
	struct iva_dev_data *iva = container_of(nb,
			struct iva_dev_data, k_panic_nb);

	dev_err(iva->dev, "%s() iva state(0x%lx)\n", __func__, iva->state);

	return NOTIFY_DONE;
}

static int iva_ctrl_register_panic_handler(struct iva_dev_data *iva)
{
	struct notifier_block *iva_nb = &iva->k_panic_nb;

	iva_nb->notifier_call	= iva_ctrl_catch_panic_event;
	iva_nb->next		= NULL;
	iva_nb->priority	= 0;

	return atomic_notifier_chain_register(&panic_notifier_list, iva_nb);
}

static int iva_ctrl_unregister_panic_handler(struct iva_dev_data *iva)
{
	struct notifier_block *iva_nb = &iva->k_panic_nb;

	atomic_notifier_chain_unregister(&panic_notifier_list, iva_nb);

	iva_nb->notifier_call	= NULL;
	iva_nb->next		= NULL;
	iva_nb->priority	= 0;

	return 0;
}
#endif

static void iva_ctl_parse_of_dt(struct device *dev, struct iva_dev_data	*iva)
{
	struct device_node *of_node = dev->of_node;
	struct device_node *child_node;

	/* overwrite parameters from of */
	if (!of_node) {
		dev_info(dev, "of node not found.\n");
		return;
	}

	child_node = of_find_node_by_name(of_node, "mcu-info");
	if (child_node) {
		of_property_read_u32(child_node, "mem_size",
				&iva->mcu_mem_size);
		of_property_read_u32(child_node, "shmem_size",
				&iva->mcu_shmem_size);
		of_property_read_u32(child_node, "print_delay",
				&iva->mcu_print_delay);
#if defined(CONFIG_SOC_EXYNOS9820)
		of_property_read_u32(child_node, "split_sram",
				&iva->mcu_split_sram);
#endif
		dev_dbg(dev,
			"%s() of mem_size(0x%x) shmem_size(0x%x) print_delay(%d)\n",
			__func__,
			iva->mcu_mem_size, iva->mcu_shmem_size,
			iva->mcu_print_delay);
	}

	child_node = of_parse_phandle(of_node, "dvfs-dev", 0);
	iva->iva_dvfs_dev = of_find_device_by_node(child_node);
	if (!iva->iva_dvfs_dev)
		dev_warn(dev, "fail to get iva dvfs dev\n");

	if (of_property_read_s32(of_node, "qos_rate", &iva->iva_qos_rate))
		iva->iva_qos_rate = IVA_PM_QOS_IVA_REQ;	/* default */
}

static int iva_ctl_probe(struct platform_device *pdev)
{
	int	ret = 0;
	struct device		*dev = &pdev->dev;
	struct iva_dev_data	*iva;
	struct resource		*res_iva;
#ifdef USE_NAMED_IRQ_RSC
	struct resource		*res_mbox_irq;
#endif
	int	irq_nr = IVA_MBOX_IRQ_NOT_DEFINED;

	/* iva */
	res_iva = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res_iva) {
		dev_err(dev, "failed to find memory resource for iva sfr\n");
		return -ENXIO;
	}
	dev_dbg(dev, "resource 0 %p (%x..%x)\n", res_iva,
			(uint32_t) res_iva->start, (uint32_t) res_iva->end);

	/* mbox irq */
#ifdef USE_NAMED_IRQ_RSC
	res_mbox_irq = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
			IVA_MBOX_IRQ_RSC_NAME);
	if (res_mbox_irq)
		irq_nr = res_mbox_irq->start;
	else
		dev_warn(dev, "not defined irq resource for iva mbox\n");
#else
	irq_nr = platform_get_irq(pdev, 0);
#endif
	dev_dbg(dev, "irq nr(%d)\n", irq_nr);

	iva = dev_get_drvdata(dev);
	if (!iva) {
		iva = devm_kzalloc(dev, sizeof(struct iva_dev_data), GFP_KERNEL);
		if (!iva) {
			dev_err(dev, "%s() fail to alloc mem for plat data\n",
					__func__);
			return -ENOMEM;
		}
		dev_dbg(dev, "%s() success to alloc mem for iva ctrl(%p)\n",
					__func__, iva);
		iva->flags = IVA_CTL_ALLOC_LOCALLY_F;
		dev_set_drvdata(dev, iva);
	}

	/* fill data */
#ifndef MODULE
	iva->iva_clk = devm_clk_get(dev, "clk_iva");
	if (IS_ERR(iva->iva_clk)) {
		dev_err(dev, "%s() fail to get clk_iva\n", __func__);
		ret = PTR_ERR(iva->iva_clk);
		goto err_iva_clk;
	}
#endif
	iva->dev		= dev;
	iva->misc.minor		= MISC_DYNAMIC_MINOR;
	iva->misc.name		= "iva_ctl";
	iva->misc.fops		= &iva_ctl_fops;
	iva->misc.parent	= dev;

	mutex_init(&iva->proc_mutex);
	INIT_LIST_HEAD(&iva->proc_head);

	iva_ctl_parse_of_dt(dev, iva);

	iva->iva_bus_qos_en = false;

	if (!iva->mcu_mem_size || !iva->mcu_shmem_size) {
		dev_warn(dev, "%s() forced set to mcu dft value\n", __func__);
#if defined(CONFIG_SOC_EXYNOS9820)
		if (iva->mcu_split_sram) {
#endif
			iva->mcu_mem_size	= VMCU_MEM_SIZE;
			iva->mcu_shmem_size	= SHMEM_SIZE;
#if defined(CONFIG_SOC_EXYNOS9820)
		} else {
			iva->mcu_mem_size	= EXTEND_VMCU_MEM_SIZE;
			iva->mcu_shmem_size	= EXTEND_SHMEM_SIZE;
		}
#endif
	}

	/* reserve the resources */
	iva->mbox_irq_nr = irq_nr;
	iva->iva_res = devm_request_mem_region(dev, res_iva->start,
			resource_size(res_iva), res_iva->name ? : dev_name(dev));
	if (!iva->iva_res) {
		dev_err(dev, "%s() fail to request resources. (%p)\n",
				__func__, iva->iva_res);
		ret = -EINVAL;
		goto err_req_iva_res;
	}

	iva->iva_va = devm_ioremap(dev, iva->iva_res->start,
			resource_size(iva->iva_res));
	if (!iva->iva_va) {
		dev_err(dev, "%s() fail to ioremap resources.(0x%llx--0x%llx)\n",
				__func__,
				(u64) iva->iva_res->start,
				(u64) iva->iva_res->end);
		ret = -EINVAL;
		goto err_io_remap;

	}

	ret = iva_mem_create_ion_client(iva);
	if (ret) {
		dev_err(dev, "%s() fail to create ion client ret(%d)\n",
			__func__,  ret);
		goto err_ion_client;
	}

	ret = iva_pmu_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva pmu. ret(%d)\n",
			__func__,  ret);
		goto err_pmu_prb;
	}

	ret = iva_mcu_probe(iva);
	if (ret) {
		dev_err(dev, "%s() fail to probe iva mcu. ret(%d)\n",
			__func__,  ret);
		goto err_mcu_prb;
	}

#ifdef MODULE
	ret = register_chrdev(IVA_CTRL_MAJOR, IVA_CTRL_NAME, &iva_ctl_fops);
	if (ret < 0) {
		dev_err(dev, "%s() fail to register. ret(%d)\n",
			__func__,  ret);
		goto err_misc_reg;
	}
#else
	ret = misc_register(&iva->misc);
	if (ret) {
		dev_err(dev, "%s() failed to register misc device\n",
				__func__);
		goto err_misc_reg;
	}
#endif
	/* enable runtime pm */
	pm_runtime_enable(dev);

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_set_fault_handler(dev, iva_iommu_fault_handler, iva);

	/* automatically controlled by rpm */
	ret = iovmm_activate(dev);
	if (ret) {
		dev_err(dev, "%s() fail to activate iovmm ret(%d)\n",
				__func__, ret);
		goto err_iovmm;
	}
#endif

	/* set sysfs */
	ret = iva_sysfs_init(dev);
	if (ret) {
		dev_err(dev, "%s() sysfs registeration is failed(%d)\n",
				__func__, ret);
	}

#ifdef ENABLE_IVA_PANIC_HANDLER
	/* register panic handler for debugging */
	iva_ctrl_register_panic_handler(iva);
#endif

#ifdef IVA_PM_QOS_IVA
	pm_qos_add_request(&iva->iva_qos_iva, IVA_PM_QOS_IVA, 0);
#ifdef IVA_CTL_QOS_DEV
	pm_qos_add_request(&iva->iva_qos_dev, IVA_PM_QOS_DEV, 0);
#endif
#else
	iva->iva_qos_rate = IVA_PM_IVA_MIN;
#endif

#ifdef CONFIG_PM_SLEEP
	/* initialize the iva wake lock */
	wake_lock_init(&iva->iva_wake_lock, WAKE_LOCK_SUSPEND, "iva_run_wlock");
#endif
	/* just for easy debug */
	g_iva_data = iva;

	dev_dbg(dev, "%s() probed successfully\n", __func__);
	return 0;

#ifdef CONFIG_EXYNOS_IOVMM
err_iovmm:
	pm_runtime_disable(dev);
#endif
err_misc_reg:
	iva_mcu_remove(iva);
err_mcu_prb:
	iva_pmu_remove(iva);
err_pmu_prb:
	iva_mem_destroy_ion_client(iva);
err_ion_client:
	devm_iounmap(dev, iva->iva_va);
err_io_remap:
	if (iva->iva_res) {
		devm_release_mem_region(dev, iva->iva_res->start,
				resource_size(iva->iva_res));
	}

err_req_iva_res:
	if (iva->iva_clk)
		devm_clk_put(dev, iva->iva_clk);

#ifndef MODULE
err_iva_clk:
#endif
	if (iva->flags & IVA_CTL_ALLOC_LOCALLY_F)
		devm_kfree(dev, iva);
	dev_set_drvdata(dev, NULL);
	return ret;
}


static int iva_ctl_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device		*dev = &pdev->dev;
	struct iva_dev_data	*iva = platform_get_drvdata(pdev);

	dev_info(dev, "%s()\n", __func__);

	g_iva_data = NULL;

#ifdef CONFIG_PM_SLEEP
	wake_lock_destroy(&iva->iva_wake_lock);
#endif
#ifdef ENABLE_IVA_PANIC_HANDLER
	iva_ctrl_unregister_panic_handler(iva);
#endif
	iva_sysfs_deinit(dev);

#ifdef CONFIG_EXYNOS_IOVMM
	iovmm_deactivate(dev);
#endif
	pm_runtime_disable(dev);

#ifdef MODULE
	unregister_chrdev(IVA_CTRL_MAJOR, IVA_CTRL_NAME);
#else
	misc_deregister(&iva->misc);
#endif
	iva_mcu_remove(iva);
	iva_pmu_remove(iva);

	iva_mem_destroy_ion_client(iva);

	devm_iounmap(dev, iva->iva_va);

	devm_release_mem_region(dev, iva->iva_res->start,
				resource_size(iva->iva_res));

	if (iva->iva_clk) {
		devm_clk_put(dev, iva->iva_clk);
		iva->iva_clk = NULL;
	}

	if (iva->flags & IVA_CTL_ALLOC_LOCALLY_F) {
		devm_kfree(dev, iva);
		platform_set_drvdata(pdev, NULL);
	}

	return ret;
}

static void iva_ctl_shutdown(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct iva_dev_data	*iva = platform_get_drvdata(pdev);
	unsigned long		state_org = iva->state;

#ifdef ENABLE_MCU_EXIT_AT_SHUTDOWN
	iva_ctrl_mcu_exit_forced(iva);
#endif
#ifdef ENABLE_POWER_OFF_AT_SHUTDOWN
	iva_ctrl_deinit_forced(iva);
#endif
	pm_runtime_disable(dev);

	dev_info(dev, "%s() iva state(0x%lx -> 0x%lx)\n", __func__,
			state_org, iva->state);
}

#ifdef CONFIG_PM_SLEEP
static int iva_ctl_suspend(struct device *dev)
{
	/* to do */
	dev_info(dev, "%s()\n", __func__);
	return 0;
}

static int iva_ctl_resume(struct device *dev)
{
	/* to do */
	dev_info(dev, "%s()\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_PM
static int iva_ctl_runtime_suspend(struct device *dev)
{
	return __iva_ctrl_deinit(dev);
}

static int iva_ctl_runtime_resume(struct device *dev)
{
	return __iva_ctrl_init(dev);
}
#endif

static const struct dev_pm_ops iva_ctl_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(iva_ctl_suspend, iva_ctl_resume)
	SET_RUNTIME_PM_OPS(iva_ctl_runtime_suspend, iva_ctl_runtime_resume, NULL)
};


#ifdef CONFIG_OF
static const struct of_device_id iva_ctl_match[] = {
	{ .compatible = "samsung,iva", .data = (void *)0 },
	{},
};
MODULE_DEVICE_TABLE(of, iva_ctl_match);
#endif

static struct platform_driver iva_ctl_driver = {
	.probe		= iva_ctl_probe,
	.remove		= iva_ctl_remove,
	.shutdown	= iva_ctl_shutdown,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "iva_ctl",
		.pm	= &iva_ctl_pm_ops,
		.of_match_table = of_match_ptr(iva_ctl_match),
	},
};

#ifdef ENABLE_LOCAL_PLAT_DEVICE
static struct resource iva_res[] = {
	{
		/* mcu mbox irq */
		.name	= IVA_MBOX_IRQ_RSC_NAME,
		.start	= IVA_MBOX_IRQ_NOT_DEFINED,
		.end	= IVA_MBOX_IRQ_NOT_DEFINED,
		.flags	= IORESOURCE_IRQ,
	}, {
		.name	= "iva_sfr",
		.start	= IVA_CFG_PHY_BASE_ADDR
		.end	= IVA_CFG_PHY_BASE_ADDR + IVA_CFG_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};


static struct iva_dev_data iva_pdata = {
	.mcu_mem_size		= VMCU_MEM_SIZE,
	.mcu_shmem_size		= SHMEM_SIZE,
	.mcu_print_delay	= 50,
};

static struct platform_device *iva_ctl_pdev;
#endif

static int __init iva_ctrl_module_init(void)
{
#ifdef ENABLE_LOCAL_PLAT_DEVICE
	iva_ctl_pdev = platform_device_register_resndata(NULL,
			"iva_ctl", -1, iva_res, (unsigned int) ARRAY_SIZE(iva_res),
			&iva_pdata, sizeof(iva_pdata));
#endif
	return platform_driver_register(&iva_ctl_driver);
}

static void __exit iva_ctrl_module_exit(void)
{
	platform_driver_unregister(&iva_ctl_driver);
#ifdef ENABLE_LOCAL_PLAT_DEVICE
	platform_device_unregister(iva_ctl_pdev);
#endif
}

#ifdef MODULE
module_init(iva_ctrl_module_init);
#else	/* dummy ion */
device_initcall_sync(iva_ctrl_module_init);
#endif
module_exit(iva_ctrl_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ilkwan.kim@samsung.com");
