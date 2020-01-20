/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <linux/crc32.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include "include/defex_internal.h"

struct defex_safeplace *global_safeplace_obj;

#ifdef DEFEX_SYSFS_ENABLE
static struct kobj_type safeplace_ktype;

ssize_t task_defex_safeplace_attr_show(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	struct safeplace_attribute *attribute;
	struct defex_safeplace *safeplace;

	safeplace = to_safeplace_obj(kobj);
	attribute = to_safeplace_attr(attr);

	if (!attribute->show)
		return -EIO;

	return attribute->show(safeplace, attribute, buf);
}

ssize_t safeplace_status_show(struct defex_safeplace *safeplace_obj,
			      struct safeplace_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_LEN, "%u\n", safeplace_obj->status);
}


ssize_t task_defex_safeplace_attr_store(struct kobject *kobj,
		struct attribute *attr, const char *buf, size_t len)
{
	struct safeplace_attribute *attribute;
	struct defex_safeplace *safeplace;

	safeplace = to_safeplace_obj(kobj);
	attribute = to_safeplace_attr(attr);

	if (!attribute->store)
		return -EIO;

	return attribute->store(safeplace, attribute, buf, len);
}

static void task_defex_safeplace_release(struct kobject *kobj)
{
	struct defex_safeplace *safeplace_obj;

	safeplace_obj = to_safeplace_obj(kobj);
	kfree(safeplace_obj);
}
#endif /* DEFEX_SYSFS_ENABLE */

ssize_t safeplace_status_store(struct defex_safeplace *safeplace_obj,
		struct safeplace_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int status;

	ret = kstrtouint(buf, 10, &status);
	if (ret != 0 || status > 2)
		return -EINVAL;

	safeplace_obj->status = status;
	return count;
}

struct defex_safeplace *task_defex_create_safeplace_obj(struct kset *defex_kset)
{
	struct defex_safeplace *safeplace;

	safeplace = kzalloc(sizeof(*safeplace), GFP_KERNEL);
	if (!safeplace)
		return NULL;

	safeplace->kobj.kset = defex_kset;
#ifdef DEFEX_SYSFS_ENABLE
	if (kobject_init_and_add(&safeplace->kobj, &safeplace_ktype, NULL, "%s", "safeplace")) {
		kobject_put(&safeplace->kobj);
		return NULL;
	}

	kobject_uevent(&safeplace->kobj, KOBJ_ADD);
#endif /* DEFEX_SYSFS_ENABLE */
	safeplace->status = 0;
	return safeplace;
}

void task_defex_destroy_safeplace_obj(struct defex_safeplace *safeplace)
{
#ifdef DEFEX_SYSFS_ENABLE
	kobject_put(&safeplace->kobj);
#endif /* DEFEX_SYSFS_ENABLE */
}

#ifdef DEFEX_SYSFS_ENABLE
static struct safeplace_attribute safeplace_status_attribute =
	__ATTR(status, 0600, safeplace_status_show, safeplace_status_store);


static struct attribute *safeplace_default_attrs[] = {
	&safeplace_status_attribute.attr,
	NULL,
};

static const struct sysfs_ops safeplace_sysfs_ops = {
	.show = task_defex_safeplace_attr_show,
	.store = task_defex_safeplace_attr_store,
};

static struct kobj_type safeplace_ktype = {
	.sysfs_ops = &safeplace_sysfs_ops,
	.release = task_defex_safeplace_release,
	.default_attrs = safeplace_default_attrs,
};
#endif /* DEFEX_SYSFS_ENABLE */
