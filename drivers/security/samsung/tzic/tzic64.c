/*
 * Samsung TZIC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define KMSG_COMPONENT "TZIC"

#include <linux/printk.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/types.h>
#include "tzic.h"

#ifdef CONFIG_TZDEV
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>
#if defined(CONFIG_TEEGRIS_VERSION) && (CONFIG_TEEGRIS_VERSION >= 4)
#include "extensions/irs.h"
#else
#include "tzirs.h"
#endif
#endif /* CONFIG_TZDEV */

#define TZIC_DEV "tzic"

#define TZIC_IOC_MAGIC          0x9E
#define TZIC_IOCTL_GET_FUSE_REQ _IO(TZIC_IOC_MAGIC, 0)
#define TZIC_IOCTL_SET_FUSE_REQ _IO(TZIC_IOC_MAGIC, 1)
#define TZIC_IOCTL_SET_FUSE_REQ_DEFAULT _IO(TZIC_IOC_MAGIC, 2)
#define TZIC_IOCTL_GET_FUSE_REQ_NEW _IO(TZIC_IOC_MAGIC, 10)
#define TZIC_IOCTL_SET_FUSE_REQ_NEW _IO(TZIC_IOC_MAGIC, 11)

#ifndef LOG
#define LOG printk
#endif

#ifdef CONFIG_TZIC_USE_QSEECOM
#define HLOS_IMG_TAMPER_FUSE    0
#endif /* CONFIG_TZIC_USE_QSEECOM */

#ifndef CONFIG_TZDEV
// TZ_IRS_CMD
enum {
	IRS_SET_FLAG_CMD        =           1,
	IRS_SET_FLAG_VALUE_CMD,
	IRS_INC_FLAG_CMD,
	IRS_GET_FLAG_VAL_CMD,
	IRS_ADD_FLAG_CMD,
	IRS_DEL_FLAG_CMD
};
#endif //CONFIG_TZDEV

#if defined(CONFIG_TZIC_USE_TZDEV) || defined(CONFIG_TZIC_USE_TRUSTONIC)
static int gotoCpu0(void);
static int gotoAllCpu(void) __attribute__ ((unused));
#endif

struct t_flag {
	uint32_t  name;
	uint32_t  func_cmd;
	uint32_t  value;
};

static DEFINE_MUTEX(tzic_mutex);
static struct class *driver_class;
static dev_t tzic_device_no;
static struct cdev tzic_cdev;

/*
General set flag function, it call function from tzic64_[secureOs].c
*/
int tzic_flags_set(enum oemflag_id index)
{
	int ret = 0;
	uint32_t name = index;

	if (name > OEMFLAG_MIN_FLAG && name < OEMFLAG_NUM_OF_FLAG) {
		ret = tzic_oem_flags_get(name);
		if (ret) {
			LOG(KERN_INFO "[oemflag]flag is already set. %u\n", name);
			return 0;
		}

		LOG(KERN_INFO "[oemflag]set_fuse_name : %u\n", name);

		ret = tzic_oem_flags_set(name);
		if (ret) {
			LOG(KERN_INFO "set_tamper_fuse error: ret=%d\n", ret);
			return 1;
		}
		ret = tzic_oem_flags_get(name);
		if (!ret) {
			LOG(KERN_INFO "get_tamper_fuse error: ret=%d\n", ret);
			return 1;
		}
	} else {
		LOG(KERN_INFO "[oemflag]param name is wrong\n");
		ret = -EINVAL;
	}
	return 0;
}

/*
General get flag function, it call function from tzic64_[secureOs].c
*/
int tzic_flags_get(enum oemflag_id index)
{
	int ret = 0;
	uint32_t name = index;

	LOG(KERN_INFO "[oemflag]get_fuse_name : %u\n", name);

	if (name > OEMFLAG_MIN_FLAG && name < OEMFLAG_NUM_OF_FLAG) {
		ret = tzic_oem_flags_get(name);
	} else {
		LOG(KERN_INFO "[oemflag]param name is wrong\n");
		ret = -EINVAL;
	}
	return ret;
}

static long tzic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int i;
	struct t_flag param = { 0, 0, 0 };

#ifdef CONFIG_TZDEV
	if (_IOC_TYPE(cmd) != IOC_MAGIC  && _IOC_TYPE(cmd) != TZIC_IOC_MAGIC) {
		LOG(KERN_INFO "[oemflag]INVALID CMD = %d\n", cmd);
		return -ENOTTY;
	}
#endif /* CONFIG_TZDEV */

#if defined(CONFIG_TZIC_USE_TZDEV) || defined(CONFIG_TZIC_USE_TRUSTONIC)
	ret = gotoCpu0();
	if (ret != 0) {
		LOG(KERN_INFO "[oemflag]changing core failed!\n");
		return -1;
	}
#endif

	switch (cmd) {

#ifdef CONFIG_TZDEV
	case IOCTL_IRS_CMD:
		/* get flag id */
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret != 0) {
			LOG(KERN_INFO "[oemflag]copy_from_user failed, ret = 0x%08x\n", ret);
			break;
		}

		if(param.func_cmd==IRS_SET_FLAG_VALUE_CMD) {
			ret = tzic_flags_set(param.name);
		} else {
			ret = tzic_flags_get(param.name);
		}

	break;
#endif /* CONFIG_TZDEV */

#ifdef CONFIG_TZIC_USE_QSEECOM
	case TZIC_IOCTL_GET_FUSE_REQ:
		ret = tzic_flags_get(HLOS_IMG_TAMPER_FUSE);
		LOG(KERN_INFO "[oemflag]tamper_fuse value = %x\n", ret);
		break;

	case TZIC_IOCTL_SET_FUSE_REQ:
		ret = tzic_flags_get(HLOS_IMG_TAMPER_FUSE);
		LOG(KERN_INFO "[oemflag]tamper_fuse before = %x\n", ret);
		mutex_lock(&tzic_mutex);
		ret = tzic_flags_set(HLOS_IMG_TAMPER_FUSE);
		mutex_unlock(&tzic_mutex);
		if (ret)
			LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
		LOG(KERN_INFO "[oemflag]tamper_fuse after = %x\n", tzic_flags_get(HLOS_IMG_TAMPER_FUSE));
		break;
#endif //CONFIG_TZIC_USE_QSEECOM

	case TZIC_IOCTL_SET_FUSE_REQ_DEFAULT: //SET ALL OEM FLAG EXCEPT 0
		LOG(KERN_INFO "[oemflag]set_fuse_default\n");
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret) {
			LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
			return ret;
		}
		for (i = OEMFLAG_MIN_FLAG+1; i < OEMFLAG_NUM_OF_FLAG; i++) {
			param.name = i;
			ret = tzic_flags_get(param.name);
			LOG(KERN_INFO "[oemflag]tamper_fuse before = %x\n", ret);
			mutex_lock(&tzic_mutex);
			ret = tzic_flags_set(param.name);
			mutex_unlock(&tzic_mutex);
			if (ret)
				LOG(KERN_INFO "[oemflag]failed tzic_set_fuse_cmd: %d\n", ret);
			ret = tzic_flags_get(param.name);
			LOG(KERN_INFO "[oemflag]tamper_fuse after = %x\n", ret);
		}
	break;

	case TZIC_IOCTL_SET_FUSE_REQ_NEW:
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret) {
			LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
			break;
		}
		if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
			ret = tzic_flags_set(param.name);
		} else {
			LOG(KERN_INFO "[oemflag]command error\n");
			ret = -1;
		}
	break;

	case TZIC_IOCTL_GET_FUSE_REQ_NEW:
		LOG(KERN_INFO "[oemflag]get_fuse_new\n");
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret) {
			LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
			break;
		}
		if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
			ret = tzic_flags_get(param.name);
			LOG(KERN_INFO "[oemflag]get_oemflag_value : %u\n", ret);
		} else {
			LOG(KERN_INFO "[oemflag]command error\n");
			ret = -1;
		}
	break;

	default:
		LOG(KERN_INFO "[oemflag]default\n");
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (param.func_cmd == IRS_SET_FLAG_VALUE_CMD) {
			if (ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				break;
			}
			if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
				ret = tzic_flags_set(param.name);
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				ret = -1;
			}
		} else if (param.func_cmd == IRS_GET_FLAG_VAL_CMD) {
			LOG(KERN_INFO "[oemflag]get_fuse_new\n");
			if (ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				break;
			}
			if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
				ret = tzic_flags_get(param.name);
				LOG(KERN_INFO "[oemflag]get_oemflag_value : %u\n", ret);
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				ret = -1;
			}
		} else {
			LOG(KERN_INFO "[oemflag]command error\n");
			ret = -1;
		}
	}

#if defined(CONFIG_TZIC_USE_TZDEV) || defined(CONFIG_TZIC_USE_TRUSTONIC)
	gotoAllCpu();
#endif
	return ret;
}

static const struct file_operations tzic_fops = {
	.owner = THIS_MODULE,
	.compat_ioctl = tzic_ioctl,
	.unlocked_ioctl = tzic_ioctl,
};

static int __init tzic_init(void)
{
	int rc;
	struct device *class_dev;

	rc = alloc_chrdev_region(&tzic_device_no, 0, 1, TZIC_DEV);
	if (rc < 0) {
		LOG(KERN_INFO "alloc_chrdev_region failed %d\n", rc);
		return rc;
	}

	driver_class = class_create(THIS_MODULE, TZIC_DEV);
	if (IS_ERR(driver_class)) {
		rc = -ENOMEM;
		LOG(KERN_INFO "class_create failed %d\n", rc);
		goto unregister_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, tzic_device_no, NULL,
				  TZIC_DEV);
	if (!class_dev) {
		LOG(KERN_INFO "class_device_create failed %d\n", rc);
		rc = -ENOMEM;
		goto class_destroy;
	}

	cdev_init(&tzic_cdev, &tzic_fops);
	tzic_cdev.owner = THIS_MODULE;

	rc = cdev_add(&tzic_cdev, MKDEV(MAJOR(tzic_device_no), 0), 1);
	if (rc < 0) {
		LOG(KERN_INFO "cdev_add failed %d\n", rc);
		goto class_device_destroy;
	}

	return 0;

 class_device_destroy:
	device_destroy(driver_class, tzic_device_no);
 class_destroy:
	class_destroy(driver_class);
 unregister_chrdev_region:
	unregister_chrdev_region(tzic_device_no, 1);
	return rc;
}

static void __exit tzic_exit(void)
{
	device_destroy(driver_class, tzic_device_no);
	class_destroy(driver_class);
	unregister_chrdev_region(tzic_device_no, 1);
}

#if defined(CONFIG_TZIC_USE_TZDEV) || defined(CONFIG_TZIC_USE_TRUSTONIC)
static int gotoCpu0(void)
{
	int ret = 0;
	struct cpumask mask = CPU_MASK_CPU0;

	ret = set_cpus_allowed_ptr(current, &mask);

	if (ret != 0)
		LOG(KERN_INFO "set_cpus_allowed_ptr=%d.\n", ret);
	return ret;
}

static int gotoAllCpu(void)
{
	int ret = 0;
	struct cpumask mask = CPU_MASK_ALL;

	ret = set_cpus_allowed_ptr(current, &mask);

	if (ret != 0)
		LOG(KERN_INFO "set_cpus_allowed_ptr=%d.\n", ret);
	return ret;
}
#endif

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung TZIC Driver");
MODULE_VERSION("1.00");

module_init(tzic_init);
module_exit(tzic_exit);
