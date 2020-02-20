/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include "include/defex_internal.h"

struct defex_immutable *global_immutable_obj;

#ifdef DEFEX_SYSFS_ENABLE
static struct kobj_type immutable_ktype;

ssize_t task_defex_immutable_attr_show(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	struct immutable_attribute *attribute;
	struct defex_immutable *immutable;

	immutable = to_immutable_obj(kobj);
	attribute = to_immutable_attr(attr);

	if (!attribute->show)
		return -EIO;

	return attribute->show(immutable, attribute, buf);
}

ssize_t immutable_status_show(struct defex_immutable *immutable_obj,
			      struct immutable_attribute *attr, char *buf)
{
	return snprintf(buf, MAX_LEN, "%u\n", immutable_obj->status);
}


ssize_t task_defex_immutable_attr_store(struct kobject *kobj,
		struct attribute *attr, const char *buf, size_t len)
{
	struct immutable_attribute *attribute;
	struct defex_immutable *immutable;

	immutable = to_immutable_obj(kobj);
	attribute = to_immutable_attr(attr);

	if (!attribute->store)
		return -EIO;

	return attribute->store(immutable, attribute, buf, len);
}

static void task_defex_immutable_release(struct kobject *kobj)
{
	struct defex_immutable *immutable_obj;

	immutable_obj = to_immutable_obj(kobj);
	kfree(immutable_obj);
}
#endif /* DEFEX_SYSFS_ENABLE */

ssize_t immutable_status_store(struct defex_immutable *immutable_obj,
		struct immutable_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int status;

	ret = kstrtouint(buf, 10, &status);
	if (ret != 0 || status > 2)
		return -EINVAL;

	immutable_obj->status = status;
	return count;
}

struct defex_immutable *task_defex_create_immutable_obj(struct kset *defex_kset)
{
	struct defex_immutable *immutable;

	immutable = kzalloc(sizeof(*immutable), GFP_KERNEL);
	if (!immutable)
		return NULL;

	immutable->kobj.kset = defex_kset;
#ifdef DEFEX_SYSFS_ENABLE
	if (kobject_init_and_add(&immutable->kobj, &immutable_ktype, NULL, "%s", "immutable")) {
		kobject_put(&immutable->kobj);
		return NULL;
	}

	kobject_uevent(&immutable->kobj, KOBJ_ADD);
#endif /* DEFEX_SYSFS_ENABLE */
	immutable->status = 0;
	return immutable;
}

void task_defex_destroy_immutable_obj(struct defex_immutable *immutable)
{
#ifdef DEFEX_SYSFS_ENABLE
	kobject_put(&immutable->kobj);
#endif /* DEFEX_SYSFS_ENABLE */
}

#ifdef DEFEX_SYSFS_ENABLE
static struct immutable_attribute immutable_status_attribute =
	__ATTR(status, 0600, immutable_status_show, immutable_status_store);


static struct attribute *immutable_default_attrs[] = {
	&immutable_status_attribute.attr,
	NULL,
};

static const struct sysfs_ops immutable_sysfs_ops = {
	.show = task_defex_immutable_attr_show,
	.store = task_defex_immutable_attr_store,
};

static struct kobj_type immutable_ktype = {
	.sysfs_ops = &immutable_sysfs_ops,
	.release = task_defex_immutable_release,
	.default_attrs = immutable_default_attrs,
};
#endif /* DEFEX_SYSFS_ENABLE */
