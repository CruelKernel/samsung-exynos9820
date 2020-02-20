/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/reboot.h>

#include <linux/sec_ext.h>
#include <linux/sec_class.h>

#define SEC_EDTBO_FILENAME		"/spu/edtbo/edtbo.img"
#define SEC_PARAM_EXTRA_MAX		10240
#define SEC_PARAM_STR_MAX		2048

static unsigned long edtbo_ver = 0;
static unsigned long edtbo_offset = 0;
static unsigned long param_offset = 0;

static int __init init_sysup_edtbo(char *str)
{
	char *endstr;
	edtbo_ver = simple_strtoul(str, &endstr, 10);

	if (*endstr != '@' || kstrtoul(endstr + 1, 0, &edtbo_offset)) {
		pr_err("%s: Input string is invalid.\n", __func__);
		return -EINVAL;
	}

	pr_info("%s: Edtbo Version = %lu\n", __func__, edtbo_ver);

	return 0;
}

early_param("androidboot.sysup.edtbo", init_sysup_edtbo);

static int __init init_sysup_param(char *str)
{
	if (kstrtoul(str, 0, &param_offset)) {
		pr_err("%s: Input string is invalid.\n", __func__);
		return -EINVAL;
	}

	return 0;
}

early_param("androidboot.sysup.param", init_sysup_param);

static ssize_t sec_edtbo_update_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	struct fiemap *pfiemap;
	struct fiemap_extent_info fieinfo = { 0, };
	struct file *fp;
	mm_segment_t old_fs;
	struct inode *inode;
	u64 len;
	int update, error;

	if (!edtbo_offset)
		return -EFAULT;

	error = sscanf(buf, "%d", &update);
	if (error < 0 || update != 1)
		return -EINVAL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(SEC_EDTBO_FILENAME, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		pr_err("%s: file open error\n", __func__);
		error = -ENOENT;
		goto fp_err;
	}

	inode = file_inode(fp);
	if (!inode->i_op->fiemap) {
		error = -EOPNOTSUPP;
		goto open_err;
	}

	pfiemap = kmalloc(SEC_PARAM_EXTRA_MAX, GFP_KERNEL | __GFP_ZERO);
	if (!pfiemap) {
		error = -ENOMEM;
		goto open_err;
	}

	pfiemap->fm_length = ULONG_MAX;
	pfiemap->fm_extent_count = (SEC_PARAM_EXTRA_MAX - offsetof(struct fiemap, fm_extents)) / sizeof(struct fiemap_extent);

	error = fiemap_check_ranges(inode->i_sb, pfiemap->fm_start, pfiemap->fm_length, &len);
	if (error)
		goto alloc_err;

	fieinfo.fi_flags = pfiemap->fm_flags;
	fieinfo.fi_extents_max = pfiemap->fm_extent_count;
	fieinfo.fi_extents_start = pfiemap->fm_extents;

	error = inode->i_op->fiemap(inode, &fieinfo, pfiemap->fm_start, len);
	if (error)
		goto alloc_err;

	pfiemap->fm_flags = fieinfo.fi_flags;
	pfiemap->fm_mapped_extents = fieinfo.fi_extents_mapped;

	if (!pfiemap->fm_mapped_extents) {
		error = -EFAULT;
		goto alloc_err;
	}

	sec_set_param_extra(edtbo_offset, pfiemap, SEC_PARAM_EXTRA_MAX);

	error = size;

alloc_err:
	if (pfiemap)
		kfree(pfiemap);
open_err:
	filp_close(fp, NULL);
fp_err:
	set_fs(old_fs);

	return error;
}

static ssize_t sec_edtbo_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	pr_info("%s: Edtbo Version = %lu\n", __func__, edtbo_ver);

	return sprintf(buf, "%lu\n", edtbo_ver);
}

static struct kobj_attribute sec_sysup_edtbo_update_attr =
	__ATTR(edtbo_update, 0220, NULL, sec_edtbo_update_store);

static struct kobj_attribute sec_sysup_edtbo_version_attr =
	__ATTR(edtbo_version, 0440, sec_edtbo_version_show, NULL);

static struct attribute *sec_sysup_attributes[] = {
	&sec_sysup_edtbo_update_attr.attr,
	&sec_sysup_edtbo_version_attr.attr,
	NULL,
};

static struct attribute_group sec_sysup_attr_group = {
	.attrs = sec_sysup_attributes,
};

/* reboot notifier call */
static int sec_sysup_reboot_notifier(struct notifier_block *this,
				unsigned long event, void *_cmd)
{
	if (event == SYS_RESTART && _cmd) {
		size_t cmdlen = strlen(_cmd);
		if (cmdlen >= 5 && cmdlen < SEC_PARAM_STR_MAX && !strncmp(_cmd, "param", 5))
			sec_set_param_str(param_offset, _cmd, cmdlen);
	}

	return NOTIFY_OK;
}

static struct notifier_block sec_sysup_reboot_nb = {
	.notifier_call = sec_sysup_reboot_notifier,
	.priority = INT_MAX,
};

static int __init sec_sysup_init(void)
{
	int ret = 0;
	struct device *dev;

	pr_info("%s: start\n", __func__);

	dev = sec_device_create(NULL, "sec_sysup");
	if(!dev)
		pr_err("%s : sec device create failed!\n", __func__);

	ret = sysfs_create_group(&dev->kobj, &sec_sysup_attr_group);
	if (ret)
		pr_err("%s : could not create sysfs node\n", __func__);

	register_reboot_notifier(&sec_sysup_reboot_nb);

	return 0;
}

static void __exit sec_sysup_exit(void)
{
	pr_info("%s: exit\n", __func__);
}

module_init(sec_sysup_init);
module_exit(sec_sysup_exit);
