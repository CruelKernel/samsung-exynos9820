/*
 * PROCA fnctl declarations
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 * Hryhorii Tur, <hryhorii.tur@partner.samsung.com>
 * Ivan Vorobiov, <i.vorobiov@samsung.com>
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

#ifndef _LINUX_PROCA_H
#define _LINUX_PROCA_H

#include <linux/file.h>
#include <linux/hashtable.h>
#include <linux/types.h>
#include <linux/sched.h>

struct proca_certificate {
	char *app_name;
	size_t app_name_size;

	char *five_signature_hash;
	size_t five_signature_hash_size;
};

struct proca_identity {
	void *certificate;
	size_t certificate_size;
	struct proca_certificate parsed_cert;
	struct file *file;
};

struct proca_task_descr {
	struct task_struct *task;
	struct proca_identity proca_identity;
	struct hlist_node pid_map_node;
	struct hlist_node app_name_map_node;
};

#define PROCA_TASKS_TABLE_SHIFT 10

struct proca_table {
	unsigned int hash_tables_shift;

	DECLARE_HASHTABLE(pid_map, PROCA_TASKS_TABLE_SHIFT);
	spinlock_t pid_map_lock;

	DECLARE_HASHTABLE(app_name_map, PROCA_TASKS_TABLE_SHIFT);
	spinlock_t app_name_map_lock;
};

#if defined(CONFIG_FIVE_PA_FEATURE) || defined(CONFIG_PROCA)
int proca_fcntl_setxattr(struct file *file, void __user *lv_xattr);
#else
static inline int proca_fcntl_setxattr(struct file *file, void __user *lv_xattr)
{
	return 0;
}
#endif

#if defined(CONFIG_PROCA)
int proca_get_task_cert(const struct task_struct *task,
			const char **cert, size_t *cert_size);
#else
static inline int proca_get_task_cert(const struct task_struct *task,
				      const char **cert, size_t *cert_size)
{
	return -ESRCH;
}
#endif

#if defined(CONFIG_SEC_DEBUG_GAF_V5)
const void *sec_gaf_get_addr(void);
#else
static inline const void *sec_gaf_get_addr(void)
{
	return NULL;
}
#endif

void proca_compat_task_free_hook(struct task_struct *task);
void proca_compat_file_free_security_hook(struct file *file);

#endif /* _LINUX_PROCA_H */
