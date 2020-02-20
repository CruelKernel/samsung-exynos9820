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
//#include <linux/android_pmem.h>
#include <linux/io.h>
#include <linux/types.h>
//#include <asm/smc.h>

#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
#include <linux/rkp_cfp.h>
#endif
#define TZIC_DEV "tzic"
#define SMC_CMD_STORE_BINFO	 (-201)

#ifdef CONFIG_TZDEV
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/miscdevice.h>
#include "tzdev/tzirs.h"
#include "tzdev/tz_cdev.h"
#endif /* CONFIG_TZDEV */

static int gotoCpu0(void);
static int gotoAllCpu(void) __attribute__ ((unused));

uint32_t exynos_smc1(uint32_t cmd, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	register uint32_t reg0 __asm__("x0") = cmd;
	register uint32_t reg1 __asm__("x1") = arg1;
	register uint32_t reg2 __asm__("x2") = arg2;
	register uint32_t reg3 __asm__("x3") = arg3;

	__asm__ volatile (
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		PRE_SMC_INLINE
#endif
		"dsb	sy\n"
		"smc	0\n"
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		POST_SMC_INLINE
#endif
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	return reg0;
}

uint32_t exynos_smc_new(uint32_t cmd, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	register uint32_t reg0 __asm__("x0") = cmd;
	register uint32_t reg1 __asm__("x1") = arg1;
	register uint32_t reg2 __asm__("x2") = arg2;
	register uint32_t reg3 __asm__("x3") = arg3;

	__asm__ volatile (
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		PRE_SMC_INLINE
#endif
		"dsb	sy\n"
		"smc	0\n"
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		POST_SMC_INLINE
#endif
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	return reg1;
}

int exynos_smc_read_oemflag(uint32_t ctrl_word, uint32_t *val)
{
	uint32_t	cmd = 0;
	uint32_t	arg1 = 0;
	uint32_t	arg2 = 0;
	uint32_t	arg3 = 0;

	register uint32_t reg0 __asm__("x0") = cmd;
	register uint32_t reg1 __asm__("x1") = arg1;
	register uint32_t reg2 __asm__("x2") = arg2;
	register uint32_t reg3 __asm__("x3") = arg3;

	uint32_t idx = 0;

	for (idx = 0; reg2 != ctrl_word; idx++) {
		reg0 = -202;
		reg1 = 1;
		reg2 = idx;
		__asm__ volatile (
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
			PRE_SMC_INLINE
#endif
			"dsb	sy\n"
			"smc	0\n"
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
			POST_SMC_INLINE
#endif
			: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
		);
		if (reg1)
			return -1;
	}

	reg0 = -202;
	reg1 = 1;
	reg2 = idx;

	__asm__ volatile (
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		PRE_SMC_INLINE
#endif
		"dsb	sy\n"
		"smc	0\n"
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		POST_SMC_INLINE
#endif
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);
	if (reg1)
		return -1;

	*val = reg2;

	return 0;
}

int exynos_smc_read_oemflag_new(uint32_t getflag, uint32_t *val)
{
	uint32_t	cmd = 0;
	uint32_t	arg1 = 0;
	uint32_t	arg2 = 0;
	uint32_t	arg3 = 0;

	register uint32_t reg0 __asm__("x0") = cmd;
	register uint32_t reg1 __asm__("x1") = arg1;
	register uint32_t reg2 __asm__("x2") = arg2;
	register uint32_t reg3 __asm__("x3") = arg3;

	uint32_t idx = 0;

	reg0 = 0x83000003;
	reg1 = 1;
	reg2 = getflag;
	reg3 = idx;

	__asm__ volatile (
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		PRE_SMC_INLINE
#endif
		"dsb	sy\n"
		"smc	0\n"
#ifdef CONFIG_RKP_CFP_FIX_SMC_BUG
		POST_SMC_INLINE
#endif
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	if (reg1 == -1)
		return -1;

	*val = reg1;
	return 0;
}


static DEFINE_MUTEX(tzic_mutex);
static struct class *driver_class;
static dev_t tzic_device_no;
static struct cdev tzic_cdev;

#define LOG printk
#define TZIC_IOC_MAGIC 0x9E
#define TZIC_IOCTL_SET_FUSE_REQ _IO(TZIC_IOC_MAGIC, 1)
#define TZIC_IOCTL_GET_FUSE_REQ _IOR(TZIC_IOC_MAGIC, 0, unsigned int)

#define TZIC_IOCTL_SET_FUSE_REQ_NEW _IO(TZIC_IOC_MAGIC, 11)
#define TZIC_IOCTL_GET_FUSE_REQ_NEW _IO(TZIC_IOC_MAGIC, 10)

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
#endif /* CONFIG_TZDEV */

// Sec_OemFlagID_t
enum {
    OEMFLAG_MIN_FLAG = 2,
    OEMFLAG_TZ_DRM,
    OEMFLAG_FIDD,
    OEMFLAG_CC,
    OEMFLAG_ETC,
    OEMFLAG_CUSTOM_KERNEL,
    OEMFLAG_NUM_OF_FLAG,
};

struct t_flag {
	uint32_t  name;
	uint32_t  func_cmd;
	uint32_t  value;
};

static long tzic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct t_flag param = { 0, 0, 0 };
#ifdef CONFIG_TZDEV
	unsigned long p1, p2, p3;

	//struct irs_ctx __user *ioargp = (struct irs_ctx __user *) arg;
	//struct irs_ctx ctx = {0};

	if (_IOC_TYPE(cmd) != IOC_MAGIC  && _IOC_TYPE(cmd) != TZIC_IOC_MAGIC) {
		LOG(KERN_INFO "[oemflag]INVALID CMD = %d\n", cmd);
		return -ENOTTY;
	}
#endif /* CONFIG_TZDEV */

	ret = gotoCpu0();
	if (ret != 0) {
		LOG(KERN_INFO "[oemflag]changing core failed!\n");
		return -1;
	}
	switch (cmd) {
#ifdef CONFIG_TZDEV
	case IOCTL_IRS_CMD:
		LOG(KERN_INFO "[oemflag]tzirs cmd\n");
		/* get flag id */
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret != 0) {
			LOG(KERN_INFO "[oemflag]copy_from_user failed, ret = 0x%08x\n", ret);
			goto return_new_from;
		}

		p1 = param.name;
		p2 = param.value;
		p3 = param.func_cmd;

		LOG(KERN_INFO "[oemflag]before: id = 0x%lx, value = 0x%lx, cmd = 0x%lx\n", (unsigned long)p1, (unsigned long)p2, (unsigned long)p3);

		ret = tzirs_smc(&p1, &p2, &p3);

		LOG(KERN_INFO "[oemflag]after: id = 0x%lx, value = 0x%lx, cmd = 0x%lx\n", (unsigned long)p1, (unsigned long)p2, (unsigned long)p3);

		if (ret) {
			LOG(KERN_INFO "[oemflag]Unable to send IRS_CMD : id = 0x%lx, ret = %d\n", (unsigned long)p1, ret);
			return -EFAULT;
		}

		param.name = (uint32_t)p1;
		param.value = (uint32_t)p2;
		param.func_cmd = (uint32_t)p3;

		goto return_new_to;
	break;
#endif /* CONFIG_TZDEV */

	case TZIC_IOCTL_SET_FUSE_REQ:
		LOG(KERN_INFO "[oemflag]set_fuse\n");
		exynos_smc1(SMC_CMD_STORE_BINFO, 0x80010001, 0, 0);
		exynos_smc1(SMC_CMD_STORE_BINFO, 0x00000001, 0, 0);
		goto return_default;
	break;

	case TZIC_IOCTL_GET_FUSE_REQ:
		LOG(KERN_INFO "[oemflag]get_fuse\n");
		if (!access_ok(VERIFY_WRITE, (void *)arg, sizeof((void *)arg))) {
			LOG(KERN_INFO "Address is not in user space");
			return -1;
		}
		exynos_smc_read_oemflag(0x80010001, (uint32_t *) arg);
		goto return_default;
	break;

	case TZIC_IOCTL_SET_FUSE_REQ_NEW:
		LOG(KERN_INFO "[oemflag]set_fuse_new\n");
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret) {
			LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
			goto return_new_from;
		}
		if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
			LOG(KERN_INFO "[oemflag]set_fuse_name : %u\n", param.name);
			exynos_smc_new(0x83000004, 0, param.name, 0);
			goto return_new_to;
		} else {
			LOG(KERN_INFO "[oemflag]command error\n");
			param.value = -1;
			goto return_new_to;
		}
	break;

	case TZIC_IOCTL_GET_FUSE_REQ_NEW:
		LOG(KERN_INFO "[oemflag]get_fuse_new\n");
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (ret) {
			LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
			goto return_new_from;
		}
		if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
			LOG(KERN_INFO "[oemflag]get_fuse_name : %u\n", param.name);
			exynos_smc_read_oemflag_new(param.name, &param.value);
			LOG(KERN_INFO "[oemflag]get_oemflag_value : %u\n", param.value);
			goto return_new_to;
		} else {
			LOG(KERN_INFO "[oemflag]command error\n");
			param.value = -1;
			goto return_new_to;
		}
	break;

	default:
		LOG(KERN_INFO "[oemflag]default\n");
		ret = copy_from_user(&param, (void *)arg, sizeof(param));
		if (param.func_cmd == IRS_SET_FLAG_VALUE_CMD) {
			LOG(KERN_INFO "[oemflag]set_fuse_new\n");
			if (ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				goto return_new_from;
			}
			if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
				LOG(KERN_INFO "[oemflag]set_fuse_name : %u\n", param.name);
				exynos_smc_new(0x83000004, 0, param.name, 0);
				goto return_new_to;
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				param.value = -1;
				goto return_new_to;
			}
		} else if (param.func_cmd == IRS_GET_FLAG_VAL_CMD) {
			LOG(KERN_INFO "[oemflag]get_fuse_new\n");
			if (ret) {
				LOG(KERN_INFO "[oemflag]ERROR copy from user\n");
				goto return_new_from;
			}
			if ((param.name > OEMFLAG_MIN_FLAG) && (param.name < OEMFLAG_NUM_OF_FLAG)) {
				LOG(KERN_INFO "[oemflag]get_fuse_name : %u\n", param.name);
				exynos_smc_read_oemflag_new(param.name, &param.value);
				LOG(KERN_INFO "[oemflag]get_oemflag_value : %u\n", param.value);
				goto return_new_to;
			} else {
				LOG(KERN_INFO "[oemflag]command error\n");
				param.value = -1;
				goto return_new_to;
			}
		} else {
			LOG(KERN_INFO "[oemflag]command error\n");
			param.value = -1;
			goto return_new_to;
		}
	}

 return_default:
	gotoAllCpu();
	return 0;
 return_new_from:
	gotoAllCpu();
	return copy_from_user(&param, (void *)arg, sizeof(param));
 return_new_to:
	gotoAllCpu();
	return copy_to_user((void *)arg, &param, sizeof(param));
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

int tzic_get_tamper_flag(void)
{
	uint32_t arg;

	exynos_smc_read_oemflag(0x80010001, &arg);

	return arg;
}
EXPORT_SYMBOL(tzic_get_tamper_flag);

int tzic_set_tamper_flag(void)
{
	exynos_smc1(SMC_CMD_STORE_BINFO, 0x80010001, 0, 0);
	return exynos_smc1(SMC_CMD_STORE_BINFO, 0x00000001, 0, 0);
}
EXPORT_SYMBOL(tzic_set_tamper_flag);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Samsung TZIC Driver");
MODULE_VERSION("1.00");

module_init(tzic_init);
module_exit(tzic_exit);
