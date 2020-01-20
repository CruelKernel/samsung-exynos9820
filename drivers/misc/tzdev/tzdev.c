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
#include <linux/buffer_head.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mmzone.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/pid.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include <asm/segment.h>
#include <asm/uaccess.h>

#include "sysdep.h"
#include "tz_boost.h"
#include "tz_cdev.h"
#include "tz_cma.h"
#include "tz_deploy_tzar.h"
#include "tz_fsdev.h"
#include "tz_iw_boot_log.h"
#include "tz_iwio.h"
#include "tz_iwlog.h"
#include "tz_iwservice.h"
#include "tz_iwsock.h"
#include "tz_kthread_pool.h"
#include "tz_mem.h"
#include "tz_panic_dump.h"
#include "tz_platform.h"
#include "tz_uiwsock.h"
#include "tzdev.h"
#include "tzlog.h"
#include "tzprofiler.h"

MODULE_AUTHOR("Jaemin Ryu <jm77.ryu@samsung.com>");
MODULE_AUTHOR("Vasily Leonenko <v.leonenko@samsung.com>");
MODULE_AUTHOR("Alex Matveev <alex.matveev@samsung.com>");
MODULE_DESCRIPTION("TZDEV driver");
MODULE_LICENSE("GPL");

unsigned int tzdev_verbosity;
unsigned int tzdev_teec_verbosity;
unsigned int tzdev_kthread_verbosity;
unsigned int tzdev_uiwsock_verbosity;

module_param(tzdev_verbosity, uint, 0644);
MODULE_PARM_DESC(tzdev_verbosity, "0: normal, 1: verbose, 2: debug");

module_param(tzdev_teec_verbosity, uint, 0644);
MODULE_PARM_DESC(tzdev_teec_verbosity, "0: error, 1: info, 2: debug");

module_param(tzdev_kthread_verbosity, uint, 0644);
MODULE_PARM_DESC(tzdev_kthread_verbosity, "0: error, 1: info, 2: debug");

module_param(tzdev_uiwsock_verbosity, uint, 0644);
MODULE_PARM_DESC(tzdev_uiwsock_verbosity, "0: error, 1: info, 2: debug");

enum tzdev_nwd_state {
	TZDEV_NWD_DOWN,
	TZDEV_NWD_UP,
};

enum tzdev_swd_state {
	TZDEV_SWD_DOWN,
	TZDEV_SWD_UP,
	TZDEV_SWD_DEAD
};

struct tzdev_shmem {
	struct list_head link;
	unsigned int id;
};

static atomic_t tzdev_nwd_state = ATOMIC_INIT(TZDEV_NWD_DOWN);
static atomic_t tzdev_swd_state = ATOMIC_INIT(TZDEV_SWD_DOWN);

static struct tzio_sysconf tz_sysconf;

static int tzdev_alloc_aux_channels(void)
{
	int ret;
	unsigned int i;

	for (i = 0; i < NR_SW_CPU_IDS; ++i) {
		ret = tz_iwio_alloc_aux_channel(i);
		if (ret)
			return ret;
	}

	return 0;
}

static int tzdev_sysconf(struct tzio_sysconf *sysconf)
{
	int ret = 0;
	struct tz_iwio_aux_channel *ch;

	ch = tz_iwio_get_aux_channel();

	sysconf->nwd_sysconf.flags = tzdev_platform_get_sysconf_flags();

#ifdef CONFIG_TZDEV_HOTPLUG
	sysconf->nwd_sysconf.flags |= SYSCONF_NWD_CPU_HOTPLUG;
#endif /* CONFIG_TZDEV_HOTPLUG */

#ifdef CONFIG_TZDEV_DEPLOY_TZAR
	sysconf->nwd_sysconf.flags |= SYSCONF_NWD_TZDEV_DEPLOY_TZAR;
#endif /* CONFIG_TZDEV_DEPLOY_TZAR */

	memcpy(ch->buffer, &sysconf->nwd_sysconf, sizeof(struct tzio_nwd_sysconf));

	ret = tzdev_smc_sysconf();
	if (ret) {
		tzdev_print(0, "tzdev_smc_sysconf() failed with %d\n", ret);
		goto out;
	}

	memcpy(&sysconf->swd_sysconf, ch->buffer, sizeof(struct tzio_swd_sysconf));
out:
	tz_iwio_put_aux_channel();

	return ret;
}

static irqreturn_t tzdev_event_handler(int irq, void *ptr)
{
	tz_kthread_pool_cmd_send();

	return IRQ_HANDLED;
}

#if CONFIG_TZDEV_IWI_PANIC != 0
static void dump_kernel_panic_bh(struct work_struct *work)
{
	atomic_set(&tzdev_swd_state, TZDEV_SWD_DEAD);
	if (atomic_read(&tzdev_nwd_state) == TZDEV_NWD_UP) {
		tz_iw_boot_log_read();
		tz_iwlog_read_buffers();
		tz_iwsock_kernel_panic_handler();
		tz_kthread_pool_fini();
		tzdev_mem_release_panic_handler();
		panic("tzdev: IWI_PANIC raised\n");
	}
}

static DECLARE_WORK(dump_kernel_panic, dump_kernel_panic_bh);

static irqreturn_t tzdev_panic_handler(int irq, void *ptr)
{
	schedule_work(&dump_kernel_panic);
	return IRQ_HANDLED;
}
#endif

static void tzdev_resolve_iwis_id(unsigned int *iwi_event, unsigned int *iwi_panic)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "samsung,blowfish");
	if (!node)
		node = of_find_compatible_node(NULL, NULL, "samsung,teegris");

	if (!node) {
		*iwi_event = CONFIG_TZDEV_IWI_EVENT;
#if CONFIG_TZDEV_IWI_PANIC != 0
		*iwi_panic = CONFIG_TZDEV_IWI_PANIC;
#endif
		tzdev_print(0, "of_find_compatible_node() failed\n");
		return;
	}

	*iwi_event = irq_of_parse_and_map(node, 0);
	if (!*iwi_event) {
		*iwi_event = CONFIG_TZDEV_IWI_EVENT;
		tzdev_print(0, "Use IWI event IRQ number from config %d\n",
			CONFIG_TZDEV_IWI_EVENT);
	}

#if CONFIG_TZDEV_IWI_PANIC != 0
	*iwi_panic = irq_of_parse_and_map(node, 1);
	if (!*iwi_panic) {
		*iwi_panic = CONFIG_TZDEV_IWI_PANIC;
		tzdev_print(0, "Use IWI panic IRQ number from config %d\n",
			CONFIG_TZDEV_IWI_PANIC);
	}
#endif
	of_node_put(node);
}

static void tzdev_register_iwis(void)
{
	int ret;
	unsigned int iwi_event;
	unsigned int iwi_panic;

	tzdev_resolve_iwis_id(&iwi_event, &iwi_panic);

	ret = request_irq(iwi_event, tzdev_event_handler, 0, "tzdev_iwi_event", NULL);
	if (ret)
		tzdev_print(0, "TZDEV_IWI_EVENT registration failed: %d\n", ret);

#if CONFIG_TZDEV_IWI_PANIC != 0
	ret = request_irq(iwi_panic, tzdev_panic_handler, 0, "tzdev_iwi_panic", NULL);
	if (ret)
		tzdev_print(0, "TZDEV_IWI_PANIC registration failed: %d\n", ret);
#endif
}

int __tzdev_smc_cmd(struct tzdev_smc_data *data)
{
	int ret;

	tzprofiler_enter_sw();
	tz_iwlog_schedule_delayed_work();

	tzdev_print(2, "tzdev_smc_cmd: enter: args={0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x}\n",
			data->args[0], data->args[1], data->args[2], data->args[3],
			data->args[4], data->args[5], data->args[6]);

	ret = tzdev_platform_smc_call(data);

	tzdev_print(2, "tzdev_smc_cmd: exit: args={0x%x 0x%x 0x%x 0x%x} ret=%d\n",
			data->args[0], data->args[1], data->args[2], data->args[3], ret);

	tz_iwlog_cancel_delayed_work();
	tz_iwlog_read_buffers();

	tzprofiler_wait_for_bufs();
	tzprofiler_exit_sw();

	tz_iwsock_check_notifications();
	tz_iwservice_handle_swd_request();

	return ret;
}

/*  TZDEV interface functions */
unsigned int tzdev_is_up(void)
{
	return atomic_read(&tzdev_swd_state) == TZDEV_SWD_UP;
}

int tzdev_run_init_sequence(void)
{
	int ret = 0;

	if (atomic_read(&tzdev_swd_state) == TZDEV_SWD_DOWN) {
		/* check kernel and driver version compatibility with TEEGRIS */
		ret = tzdev_smc_check_version();
		if (ret == -ENOSYS || ret == -EINVAL) {
			/* version is not compatibile. Not critical, continue ... */
			ret = 0;
		} else if (ret) {
			tzdev_print(0, "tzdev_smc_check_version() failed\n");
			goto out;
		}

		if (tzdev_cma_mem_register()) {
			tzdev_print(0, "tzdev_cma_mem_register() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tzdev_alloc_aux_channels()) {
			tzdev_print(0, "tzdev_alloc_AUX_Channels() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tz_iwservice_alloc()) {
			tzdev_print(0, "tzdev_alloc_service_channel() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tzdev_mem_init()) {
			tzdev_print(0, "tzdev_mem_init() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tz_iwlog_initialize()) {
			tzdev_print(0, "tz_iwlog_initialize() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tzprofiler_initialize()) {
			tzdev_print(0, "tzprofiler_initialize() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tz_panic_dump_alloc_buffer()) {
			tzdev_print(0, "tz_panic_dump_alloc_buffer() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tzdev_sysconf(&tz_sysconf)) {
			tzdev_print(0, "tzdev_sysconf() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tz_iwsock_init()) {
			tzdev_print(0, "tz_iwsock_init failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		tzdev_register_iwis();
		tz_boost_set_boost_mask(tz_sysconf.swd_sysconf.big_cpus_mask);

		if (tz_fsdev_initialize()) {
			tzdev_print(0, "tz_fsdev_initialize() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}

		if (atomic_cmpxchg(&tzdev_swd_state, TZDEV_SWD_DOWN, TZDEV_SWD_UP)) {
			ret = -ESHUTDOWN;
			goto out;
		}

		if (tzdev_deploy_tzar()) {
			tzdev_print(0, "tzdev_deploy_tzar() failed\n");
			ret = -ESHUTDOWN;
			goto out;
		}
	}
out:
	if (ret == -ESHUTDOWN) {
		atomic_set(&tzdev_swd_state, TZDEV_SWD_DEAD);
		tz_iw_boot_log_read();
		tz_iwlog_read_buffers();
	}

	return ret;
}

static int tzdev_get_sysconf(struct file *filp, unsigned long arg)
{
	struct tzio_sysconf __user *argp = (struct tzio_sysconf __user *)arg;

	if (copy_to_user(argp, &tz_sysconf, sizeof(struct tzio_sysconf)))
		return -EFAULT;

	return 0;
}

static int tzdev_register_shared_memory(struct file *filp, unsigned long arg)
{
	int ret;
	struct tzdev_shmem *shmem;
	struct tzdev_fd_data *data = filp->private_data;
	struct tzio_mem_register __user *argp = (struct tzio_mem_register __user *)arg;
	struct tzio_mem_register s;

	if (copy_from_user(&s, argp, sizeof(struct tzio_mem_register)))
		return -EFAULT;

	shmem = kzalloc(sizeof(struct tzdev_shmem), GFP_KERNEL);
	if (!shmem) {
		tzdev_print(0, "Failed to allocate shmem structure\n");
		return -ENOMEM;
	}

	ret = tzdev_mem_register_user(UINT_PTR(s.ptr), s.size, s.write);
	if (ret < 0) {
		kfree(shmem);
		return ret;
	}

	INIT_LIST_HEAD(&shmem->link);
	shmem->id = ret;

	spin_lock(&data->shmem_list_lock);
	list_add(&shmem->link, &data->shmem_list);
	spin_unlock(&data->shmem_list_lock);

	return shmem->id;
}

static int tzdev_release_shared_memory(struct file *filp, unsigned int id)
{
	struct tzdev_shmem *shmem;
	struct tzdev_fd_data *data = filp->private_data;
	unsigned int found = 0;

	spin_lock(&data->shmem_list_lock);
	list_for_each_entry(shmem, &data->shmem_list, link) {
		if (shmem->id == id) {
			list_del(&shmem->link);
			found = 1;
			break;
		}
	}
	spin_unlock(&data->shmem_list_lock);

	if (!found)
		return -EINVAL;

	kfree(shmem);

	return tzdev_mem_release_user(id);
}

static int tzdev_boost_control(struct file *filp, unsigned int state)
{
	struct tzdev_fd_data *data = filp->private_data;
	int ret = 0;

	mutex_lock(&data->mutex);

	switch (state) {
	case TZIO_BOOST_ON:
		if (!data->boost_state) {
			tz_boost_enable();
			data->boost_state = 1;
		} else {
			tzdev_print(0, "Trying to enable boost twice, filp=%pK\n", filp);
			ret = -EBUSY;
		}
		break;
	case TZIO_BOOST_OFF:
		if (data->boost_state) {
			tz_boost_disable();
			data->boost_state = 0;
		} else {
			tzdev_print(0, "Trying to disable boost twice, filp=%pK\n", filp);
			ret = -EBUSY;
		}
		break;
	default:
		tzdev_print(0, "Unknown boost request, state=%u filp=%pK\n", state, filp);
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&data->mutex);

	return ret;
}

static int tzdev_open(struct inode *inode, struct file *filp)
{
	struct tzdev_fd_data *data;

	if (!tzdev_is_up())
		return -EPERM;

	data = kzalloc(sizeof(struct tzdev_fd_data), GFP_KERNEL);
	if (!data) {
		tzdev_print(0, "Failed to allocate private data\n");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&data->shmem_list);
	spin_lock_init(&data->shmem_list_lock);
	mutex_init(&data->mutex);

	filp->private_data = data;

	return 0;
}

static int tzdev_release(struct inode *inode, struct file *filp)
{
	struct tzdev_shmem *shmem, *tmp;
	struct tzdev_fd_data *data = filp->private_data;

	list_for_each_entry_safe(shmem, tmp, &data->shmem_list, link) {
		list_del(&shmem->link);
		tzdev_mem_release_user(shmem->id);
		kfree(shmem);
	}

	if (data->boost_state)
		tzdev_boost_control(filp, TZIO_BOOST_OFF);

	tzdev_fd_platform_close(inode, filp);

	mutex_destroy(&data->mutex);

	kfree(data);

	return 0;
}

static long tzdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case TZIO_GET_SYSCONF:
		return tzdev_get_sysconf(filp, arg);
	case TZIO_MEM_REGISTER:
		return tzdev_register_shared_memory(filp, arg);
	case TZIO_MEM_RELEASE:
		return tzdev_release_shared_memory(filp, arg);
	case TZIO_BOOST_CONTROL:
		return tzdev_boost_control(filp, arg);
	default:
		return tzdev_fd_platform_ioctl(filp, cmd, arg);
	}
}

static const struct file_operations tzdev_fops = {
	.owner = THIS_MODULE,
	.open = tzdev_open,
	.release = tzdev_release,
	.unlocked_ioctl = tzdev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tzdev_ioctl,
#endif /* CONFIG_COMPAT */
};

static struct tz_cdev tzdev_cdev = {
	.name = "tzdev",
	.fops = &tzdev_fops,
	.owner = THIS_MODULE,
};

struct tz_cdev *tzdev_get_cdev(void)
{
	return &tzdev_cdev;
}

static int exit_tzdev(struct notifier_block *cb, unsigned long code, void *unused)
{
	(void)cb;
	(void)code;
	(void)unused;

	atomic_set(&tzdev_nwd_state, TZDEV_NWD_DOWN);

	tzdev_platform_unregister();
	tz_cdev_unregister(&tzdev_cdev);
	tzdev_cma_mem_release(tzdev_cdev.device);
	tz_kthread_pool_fini();
	tz_hotplug_exit();

	tz_uiwsock_fini();
	tz_iwsock_fini();

	tzdev_print(0, "tzdev exit done.\n");

	return NOTIFY_DONE;
}

static struct notifier_block tzdev_reboot_nb = {
	.notifier_call = exit_tzdev,
};

static int __init init_tzdev(void)
{
	int rc;

	rc = tz_cdev_register(&tzdev_cdev);
	if (rc) {
		tzdev_print(0, "tz_cdev_register() for device failed with error=%d\n", rc);
		return rc;
	}

	rc = tz_uiwsock_init();
	if (rc) {
		tzdev_print(0, "tz_uiwsock_init() failed with error=%d\n", rc);
		goto uiwsock_initialization_failed;
	}

	rc = tzdev_platform_register();
	if (rc) {
		tzdev_print(0, "tzdev_platform_register() failed with error=%d\n", rc);
		goto platform_driver_registration_failed;
	}

	rc = tz_hotplug_init();
	if (rc) {
		tzdev_print(0, "tzdev_init_hotplug() failed with error=%d\n", rc);
		goto hotplug_initialization_failed;
	}

	tzdev_cma_mem_init(tzdev_cdev.device);

	rc = tzdev_platform_init();
	if (rc) {
		tzdev_print(0, "tzdev_platform_init() failed with error=%d\n", rc);
		goto platform_initialization_failed;
	}

	rc = register_reboot_notifier(&tzdev_reboot_nb);
	if (rc) {
		tzdev_print(0, "reboot notifier registration failed() failed with error=%d\n", rc);
		goto reboot_notifier_registration_failed;
	}

	atomic_set(&tzdev_nwd_state, TZDEV_NWD_UP);

	return rc;

reboot_notifier_registration_failed:
	tzdev_platform_fini();
platform_initialization_failed:
	tzdev_cma_mem_release(tzdev_cdev.device);
	tz_hotplug_exit();
hotplug_initialization_failed:
	tzdev_platform_unregister();
platform_driver_registration_failed:
	tz_uiwsock_fini();
uiwsock_initialization_failed:
	tz_cdev_unregister(&tzdev_cdev);
	panic("tzdev: failed to initialize device, rc=%d\n", rc);

	return rc;
}

module_init(init_tzdev);
