/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <linux/cred.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include "include/defex_debug.h"
#include "include/defex_internal.h"

static int last_cmd;

static int set_user(struct cred *new_cred)
{
	struct user_struct *new_user;

	new_user = alloc_uid(new_cred->uid);
	if (!new_user)
		return -EAGAIN;

	free_uid(new_cred->user);
	new_cred->user = new_user;
	return 0;
}

/*
 * target_id = (0 - set all uids, 1 - set fsuid, 2 - set all gids)
 */
static int set_cred(int target_id, int new_val)
{
	struct user_namespace *ns = current_user_ns();
	const struct cred *old_cred;
	struct cred *new_cred;
	kuid_t kuid;
	kgid_t kgid;
	uid_t new_uid;
	gid_t new_gid;

	new_cred = prepare_creds();
	if (!new_cred)
		return -ENOMEM;
	old_cred = current_cred();

	switch (target_id) {
	case 0:
		new_uid = new_val;
		kuid = make_kuid(ns, new_uid);
		if (!uid_valid(kuid))
			goto do_abort;
		new_cred->uid = new_cred->euid = new_cred->suid = new_cred->fsuid = kuid;
		if (!uid_eq(new_cred->uid, old_cred->uid)) {
			if (set_user(new_cred) < 0)
				goto do_abort;
		}
		break;

	case 1:
		new_uid = new_val;
		kuid = make_kuid(ns, new_uid);
		if (!uid_valid(kuid))
			goto do_abort;
		new_cred->fsuid = kuid;
		break;

	case 2:
		new_gid = new_val;
		kgid = make_kgid(ns, new_gid);
		if (!gid_valid(kgid))
			goto do_abort;
		new_cred->gid = new_cred->egid = new_cred->sgid = new_cred->fsgid = kgid;
		break;
	}
	return commit_creds(new_cred);

do_abort:
	abort_creds(new_cred);
	return -EPERM;
}

static ssize_t debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct task_struct *p = current;
	int i, l, new_val = -1;
	int ret = 0;

	static const char *prefix[] = {
		"uid=",
		"fsuid=",
		"gid="
	};

	if (!buf || !p)
		return -EINVAL;

	for (i = 0; i < sizeof(prefix) / sizeof(prefix[0]); i++) {
		l = strlen(prefix[i]);
		if (!strncmp(buf, prefix[i], l))
			break;
	}
	if (i == (sizeof(prefix) / sizeof(prefix[0])))
		return -EINVAL;

	switch (i) {
	case DBG_SETUID ... DBG_SETGID:
		ret = kstrtoint(buf + l, 10, &new_val);
		if (ret != 0)
			return -EINVAL;
		ret = set_cred(i, new_val);
		break;
	default:
		break;
	}

	last_cmd = i;
	return (!ret)?count:ret;
}

static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct task_struct *p = current;
	int res = 0;

	if (!p)
		return 0;

	switch (last_cmd) {
	case DBG_SETUID ... DBG_SETGID:
		res = snprintf(buf, MAX_LEN + 1, "pid=%d\nuid=%d\ngid=%d\neuid=%d\negid=%d\n",
			p->pid,
			uid_get_value(p->cred->uid),
			uid_get_value(p->cred->gid),
			uid_get_value(p->cred->euid),
			uid_get_value(p->cred->egid));
		break;
	}

	return res;
}

static struct kobj_attribute debug_attribute = __ATTR(debug, 0660, debug_show, debug_store);

int defex_create_debug(struct kset *defex_kset)
{
	int retval;

	retval = sysfs_create_file(&defex_kset->kobj, &debug_attribute.attr);
	if (retval)
		return DEFEX_NOK;

	kobject_uevent(&defex_kset->kobj, KOBJ_ADD);
	return DEFEX_OK;
}
