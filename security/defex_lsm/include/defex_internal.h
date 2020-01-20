/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#ifndef __CONFIG_SECURITY_DEFEX_INTERNAL_H
#define __CONFIG_SECURITY_DEFEX_INTERNAL_H

#include <linux/crc32.h>
#include <linux/fs.h>
#include <linux/hashtable.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/defex.h>
#include "defex_config.h"

#define DEFEX_MAJOR_VERSION			2
#define DEFEX_MINOR_VERSION			0
#define DEFEX_REVISION				"rel"

/* DEFEX Features */
#define FEATURE_NONE				0
#define FEATURE_CHECK_CREDS			(1 << 0)
#define FEATURE_CHECK_CREDS_SOFT		(1 << 1)
#define FEATURE_JAILHOUSE			(1 << 2) /* reserved for future use */
#define FEATURE_JAILHOUSE_SOFT			(1 << 3) /* reserved for future use */
#define FEATURE_RESTRICT_SYSCALL		(1 << 4) /* reserved for future use */
#define FEATURE_RESTRICT_SYSCALL_SOFT		(1 << 5) /* reserved for future use */
#define FEATURE_IMMUTABLE			(1 << 6) /* reserved for future use */
#define FEATURE_SAFEPLACE			(1 << 7)
#define FEATURE_SAFEPLACE_SOFT			(1 << 8)
#define FEATURE_FIVE				(1 << 9) /* reserved for future use */
#define FEATURE_FIVE_SOFT			(1 << 10) /* reserved for future use */

#define FEATURE_CLEAR_ALL			(0xFF0000)

#define DEFEX_ALLOW				0
#define DEFEX_DENY				1

#define DEFEX_OK				0
#define DEFEX_NOK				1

#define DEFEX_STARTED				1

/* DEFEX FLAG ATTRs */
#define DEFEX_ATTR_PRIVESC_DIR			(1 << 0)
#define DEFEX_ATTR_PRIVESC_EXP			(1 << 1)
#define DEFEX_ATTR_JAILHOUSE_DIR		(1 << 2) /* reserved for future use */
#define DEFEX_ATTR_JAILHOUSE_EXP		(1 << 3) /* reserved for future use */
#define DEFEX_ATTR_RESTRICT_EXP			(1 << 4) /* reserved for future use */
#define DEFEX_ATTR_RESTRICT_LV1_EXP		(1 << 5) /* reserved for future use */
#define DEFEX_ATTR_SAFEPLACE_DIR		(1 << 6)
#define DEFEX_INSIDE_PRIVESC_DIR		(1 << 8)
#define DEFEX_OUTSIDE_PRIVESC_DIR		(1 << 9)
#define DEFEX_INSIDE_JAILHOUSE_DIR		(1 << 10) /* reserved for future use */
#define DEFEX_OUTSIDE_JAILHOUSE_DIR		(1 << 11) /* reserved for future use */
#define DEFEX_ATTR_IMMUTABLE			(1 << 12) /* reserved for future use */
#define DEFEX_ATTR_IMMUTABLE_WR			(1 << 13) /* reserved for future use */
#define DEFEX_ATTR_FIVE_EXP			(1 << 14) /* reserved for future use */

/* -------------------------------------------------------------------------- */
/* Integrity feature */
/* -------------------------------------------------------------------------- */

#define DEFEX_INTEGRITY_FAIL				(1 << 1)

/* -------------------------------------------------------------------------- */
/* Hash tables */
/* -------------------------------------------------------------------------- */
extern DECLARE_HASHTABLE(creds_hash, 15);
void creds_fast_hash_init(void);

/* -------------------------------------------------------------------------- */
/* PrivEsc feature */
/* -------------------------------------------------------------------------- */

#ifdef STRICT_UID_TYPE_CHECKS
#define CHECK_ROOT_CREDS(x) (uid_eq(x->cred->uid, GLOBAL_ROOT_UID) || \
		gid_eq(x->cred->gid, GLOBAL_ROOT_GID) || \
		uid_eq(x->cred->euid, GLOBAL_ROOT_UID) || \
		gid_eq(x->cred->egid, GLOBAL_ROOT_GID))

#define GLOBAL_SYS_UID KUIDT_INIT(1000)
#define GLOBAL_SYS_GID KGIDT_INIT(1000)

#define CHECK_SYS_CREDS(x) (uid_eq(x->cred->uid, GLOBAL_SYS_UID) || \
		gid_eq(x->cred->gid, GLOBAL_SYS_GID) || \
		uid_eq(x->cred->euid, GLOBAL_SYS_UID) || \
		gid_eq(x->cred->egid, GLOBAL_SYS_GID))

#define uid_get_value(x)	(x.val)
#define uid_set_value(x, v)	x.val = v

#else
#define CHECK_ROOT_CREDS(x) ((x->cred->uid == 0) || (x->cred->gid == 0) || \
		(x->cred->euid == 0) || (x->cred->egid == 0))
#define uid_get_value(x)	(x)
#define uid_set_value(x, v)	(x = v)
#endif /* STRICT_UID_TYPE_CHECKS */

struct defex_privesc {
	struct kobject kobj;
	unsigned int status;
};
#define to_privesc_obj(obj) container_of(obj, struct defex_privesc, kobj)

struct privesc_attribute {
	struct attribute attr;
	ssize_t (*show)(struct defex_privesc *privesc, struct privesc_attribute *attr, char *buf);
	ssize_t (*store)(struct defex_privesc *foo, struct privesc_attribute *attr, const char *buf, size_t count);
};
#define to_privesc_attr(obj) container_of(obj, struct privesc_attribute, attr)

struct defex_privesc *task_defex_create_privesc_obj(struct kset *defex_kset);
void task_defex_destroy_privesc_obj(struct defex_privesc *privesc);
extern struct defex_privesc *global_privesc_obj;
ssize_t task_defex_privesc_store_status(struct defex_privesc *privesc_obj,
		struct privesc_attribute *attr, const char *buf, size_t count);

void get_task_creds(int pid, unsigned int *uid_ptr, unsigned int *fsuid_ptr, unsigned int *egid_ptr);
int set_task_creds(int pid, unsigned int uid, unsigned int fsuid, unsigned int egid);
void delete_task_creds(int pid);
int is_task_creds_ready(void);

/* -------------------------------------------------------------------------- */
/* SafePlace feature */
/* -------------------------------------------------------------------------- */

struct defex_safeplace {
	struct kobject kobj;
	unsigned int status;
};
#define to_safeplace_obj(obj) container_of(obj, struct defex_safeplace, kobj)

struct safeplace_attribute {
	struct attribute attr;
	ssize_t (*show)(struct defex_safeplace *safeplace, struct safeplace_attribute *attr, char *buf);
	ssize_t (*store)(struct defex_safeplace *foo, struct safeplace_attribute *attr, const char *buf, size_t count);
};
#define to_safeplace_attr(obj) container_of(obj, struct safeplace_attribute, attr)

struct defex_safeplace *task_defex_create_safeplace_obj(struct kset *defex_kset);
extern void task_defex_destroy_safeplace_obj(struct defex_safeplace *safeplace);
extern struct defex_safeplace *global_safeplace_obj;
ssize_t safeplace_status_store(struct defex_safeplace *safeplace_obj,
		struct safeplace_attribute *attr, const char *buf, size_t count);

/* -------------------------------------------------------------------------- */
/* Defex lookup API */
/* -------------------------------------------------------------------------- */

char *defex_get_filename(struct task_struct *p);
int rules_lookup(const struct path *dpath, int attribute, struct file *f);

/* -------------------------------------------------------------------------- */
/* Defex init API */
/* -------------------------------------------------------------------------- */

int defex_init_sysfs(void);

#endif /* CONFIG_SECURITY_DEFEX_INTERNAL_H */
