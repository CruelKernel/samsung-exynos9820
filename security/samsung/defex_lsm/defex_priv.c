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

struct defex_privesc *global_privesc_obj;

#ifdef DEFEX_SYSFS_ENABLE
static struct kobj_type privesc_ktype;

ssize_t task_defex_privesc_attr_show(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	struct privesc_attribute *attribute;
	struct defex_privesc *privesc;

	attribute = to_privesc_attr(attr);
	privesc = to_privesc_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(privesc, attribute, buf);
}

ssize_t task_defex_privesc_show_status(struct defex_privesc *privesc_obj,
		struct privesc_attribute *attr, char *buf)
{
	return snprintf(buf, 3, "%u\n", privesc_obj->status);
}

ssize_t task_defex_privesc_attr_store(struct kobject *kobj,
		struct attribute *attr, const char *buf, size_t len)
{
	struct privesc_attribute *attribute;
	struct defex_privesc *privesc;

	attribute = to_privesc_attr(attr);
	privesc = to_privesc_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(privesc, attribute, buf, len);
}

void task_defex_privesc_release(struct kobject *kobj)
{
	struct defex_privesc *privesc_obj;

	privesc_obj = to_privesc_obj(kobj);
	kfree(privesc_obj);
}
#endif /* DEFEX_SYSFS_ENABLE*/

ssize_t task_defex_privesc_store_status(struct defex_privesc *privesc_obj,
		struct privesc_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned int status;

	ret = kstrtouint(buf, 10, &status);
	if (ret != 0 || status > 2)
		return -EINVAL;
	privesc_obj->status = status;

	return count;
}

struct defex_privesc *task_defex_create_privesc_obj(struct kset *defex_kset)
{
	struct defex_privesc *privesc;

	privesc = kzalloc(sizeof(*privesc), GFP_KERNEL);
	if (!privesc)
		return NULL;

	privesc->kobj.kset = defex_kset;
#ifdef DEFEX_SYSFS_ENABLE
	if (kobject_init_and_add(&privesc->kobj, &privesc_ktype, NULL, "%s", "privesc")) {
		kobject_put(&privesc->kobj);
		return NULL;
	}

	kobject_uevent(&privesc->kobj, KOBJ_ADD);
#endif /* DEFEX_SYSFS_ENABLE */

	/* Initial state of PED feature (0 - disabled, 1 - enabled) */
	privesc->status = 0;
	return privesc;
}

void task_defex_destroy_privesc_obj(struct defex_privesc *privesc)
{
#ifdef DEFEX_SYSFS_ENABLE
	kobject_put(&privesc->kobj);
#endif /* DEFEX_SYSFS_ENABLE */
}

#ifdef DEFEX_SYSFS_ENABLE
static struct privesc_attribute privesc_status_attribute =
	__ATTR(status, 0660, task_defex_privesc_show_status, task_defex_privesc_store_status);

static struct attribute *privesc_default_attrs[] = {
	&privesc_status_attribute.attr,
	NULL,
};

const struct sysfs_ops privesc_sysfs_ops = {
	.show = task_defex_privesc_attr_show,
	.store = task_defex_privesc_attr_store,
};

static struct kobj_type privesc_ktype = {
	.sysfs_ops = &privesc_sysfs_ops,
	.release = task_defex_privesc_release,
	.default_attrs = privesc_default_attrs,
};
#endif /* DEFEX_SYSFS_ENABLE */
