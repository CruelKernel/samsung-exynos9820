/*
 * sec_pm_debug.c
 *
 *  Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *  Minsung Kim <ms925.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/sec_class.h>

unsigned long long sleep_time_sec;
unsigned int sleep_count;

#ifdef CONFIG_REGULATOR_S2MPS19
extern void s2mps19_get_pwr_onoffsrc(u8 *onsrc, u8 *offsrc);
#endif

static void inline get_pwr_onoffsrc(u8 *onsrc, u8 *offsrc)
{
	*onsrc = 0;
	*offsrc = 0;

#ifdef CONFIG_REGULATOR_S2MPS19
	s2mps19_get_pwr_onoffsrc(onsrc, offsrc);
#endif
}

static int sec_pm_debug_ocp_info_show(struct seq_file *m, void *v)
{
	u8 onsrc = 0, offsrc = 0;

	get_pwr_onoffsrc(&onsrc, &offsrc);
	seq_printf(m, "No OCP(ON:0x%02X OFF:0x%02X)\n", onsrc, offsrc);

	return 0;
}

static int ocp_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_pm_debug_ocp_info_show, inode->i_private);
}

const static struct file_operations ocp_info_fops = {
	.open		= ocp_info_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int sec_pm_debug_on_offsrc_show(struct seq_file *m, void *v)
{
	u8 onsrc = 0, offsrc = 0;

	get_pwr_onoffsrc(&onsrc, &offsrc);
	seq_printf(m, "PWRONSRC:0x%02X OFFSRC:0x%02X\n", onsrc, offsrc);

	return 0;
}

static int on_offsrc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_pm_debug_on_offsrc_show, inode->i_private);
}

const static struct file_operations on_offsrc_fops = {
	.open		= on_offsrc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static ssize_t sleep_time_sec_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", sleep_time_sec);
}

static ssize_t sleep_count_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", sleep_count);
}

static ssize_t pwr_on_off_src_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 onsrc = 0, offsrc = 0;

	get_pwr_onoffsrc(&onsrc, &offsrc);

	return sprintf(buf, "ONSRC:0x%02X OFFSRC:0x%02X\n", onsrc, offsrc);
}

static DEVICE_ATTR_RO(sleep_time_sec);
static DEVICE_ATTR_RO(sleep_count);
static DEVICE_ATTR_RO(pwr_on_off_src);

static struct attribute *sec_pm_debug_attrs[] = {
	&dev_attr_sleep_time_sec.attr,
	&dev_attr_sleep_count.attr,
	&dev_attr_pwr_on_off_src.attr,
	NULL
};
ATTRIBUTE_GROUPS(sec_pm_debug);

static int __init sec_pm_debug_init(void)
{
	struct device *sec_pm_dev;
	int ret;

	debugfs_create_file("ocp_info", 0444, NULL, NULL, &ocp_info_fops);
	debugfs_create_file("pwr_on_offsrc", 0444, NULL, NULL, &on_offsrc_fops);

	sec_pm_dev = sec_device_create(NULL, "pm");

	if (IS_ERR(sec_pm_dev)) {
		pr_err("%s: fail to create sec_pm_dev\n", __func__);
		return PTR_ERR(sec_pm_dev);
	}

	ret = sysfs_create_groups(&sec_pm_dev->kobj, sec_pm_debug_groups);
	if (ret) {
		pr_err("%s: failed to create sysfs groups(%d)\n", __func__, ret);
		goto err_create_sysfs;
	}

	return 0;

err_create_sysfs:
	sec_device_destroy(sec_pm_dev->devt);

	return ret;
}
late_initcall(sec_pm_debug_init);
