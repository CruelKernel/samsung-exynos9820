/*
 * PROCA LSM module
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
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

#include "proca_identity.h"
#include "proca_certificate.h"
#include "proca_task_descr.h"
#include "proca_table.h"
#include "proca_log.h"

#include "five_hooks.h"

#include <linux/module.h>
#include <linux/lsm_hooks.h>
#include <linux/file.h>
#include <linux/task_integrity.h>
#include <linux/xattr.h>
#include <linux/proca.h>

static void proca_task_free_hook(struct task_struct *task);

static void proca_file_free_security_hook(struct file *file);

static struct security_hook_list proca_ops[] = {
	LSM_HOOK_INIT(task_free, proca_task_free_hook),
	LSM_HOOK_INIT(file_free_security, proca_file_free_security_hook),
};

static void proca_hook_task_forked(struct task_struct *parent,
				enum task_integrity_value parent_tint_value,
				struct task_struct *child,
				enum task_integrity_value child_tint_value);

static void proca_hook_file_processed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *xattr,
				size_t xattr_size, int result);

static void proca_hook_file_signed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *xattr,
				size_t xattr_size, int result);

static void proca_hook_file_skipped(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file);

static struct five_hook_list five_ops[] = {
	FIVE_HOOK_INIT(task_forked, proca_hook_task_forked),
	FIVE_HOOK_INIT(file_processed, proca_hook_file_processed),
	FIVE_HOOK_INIT(file_signed, proca_hook_file_signed),
	FIVE_HOOK_INIT(file_skipped, proca_hook_file_skipped),
};

static struct proca_table g_proca_table;

static int read_xattr(struct dentry *dentry, const char *name,
			char **xattr_value)
{
	ssize_t ret;
	void *buffer = NULL;

	*xattr_value = NULL;
	ret = __vfs_getxattr(dentry, dentry->d_inode, name, NULL, 0);
	if (ret <= 0)
		return 0;

	buffer = kmalloc(ret + 1, GFP_NOFS);
	if (!buffer)
		return 0;

	ret = __vfs_getxattr(dentry, dentry->d_inode, name,
				buffer, ret + 1);

	if (ret <= 0) {
		ret = 0;
		kfree(buffer);
	} else {
		*xattr_value = buffer;
	}

	return ret;
}

static struct proca_task_descr *prepare_proca_task_descr(
				struct task_struct *task, struct file *file,
				void *xattr, size_t xattr_size,
				char **out_five_xattr_value)
{
	struct proca_certificate parsed_cert;
	struct proca_identity ident;
	char *pa_xattr_value = NULL;
	size_t pa_xattr_size;
	char *five_sign_xattr_value = NULL;
	size_t five_sign_xattr_size;
	struct proca_task_descr *task_descr = NULL;

	pa_xattr_size = read_xattr(file->f_path.dentry,
				XATTR_NAME_PA, &pa_xattr_value);
	if (!pa_xattr_value)
		return task_descr;

	PROCA_DEBUG_LOG("%s xattr was found for task %d\n", XATTR_NAME_PA,
			task->pid);

	if (parse_proca_certificate(pa_xattr_value, pa_xattr_size,
				    &parsed_cert))
		goto xattr_cleanup;

	if (xattr) {
		five_sign_xattr_value = kmemdup(xattr, xattr_size, GFP_KERNEL);
		five_sign_xattr_size = xattr_size;
	} else {
		five_sign_xattr_size = read_xattr(file->f_path.dentry,
					  XATTR_NAME_FIVE,
					  &five_sign_xattr_value);
	}

	if (!five_sign_xattr_value) {
		pr_info("Failed to read five xattr from file with pa xattr\n");
		goto proca_cert_cleanup;
	}

	if (!compare_with_five_signature(&parsed_cert, five_sign_xattr_value,
					 five_sign_xattr_size)) {
		pr_info("Comparison with five signature for %s failed.\n",
			parsed_cert.app_name);
		goto five_xattr_cleanup;
	}

	if (init_proca_identity(&ident, file,
			pa_xattr_value, pa_xattr_size,
			&parsed_cert))
		goto five_xattr_cleanup;

	task_descr = create_proca_task_descr(task, &ident);
	if (!task_descr)
		goto proca_identity_cleanup;

	*out_five_xattr_value = five_sign_xattr_value;

	return task_descr;

proca_identity_cleanup:;
	deinit_proca_identity(&ident);

five_xattr_cleanup:;
	kfree(five_sign_xattr_value);

proca_cert_cleanup:;
	deinit_proca_certificate(&parsed_cert);

xattr_cleanup:;
	kfree(pa_xattr_value);

	return NULL;
}

static bool is_bprm(struct task_struct *task, struct file *old_file,
				struct file *new_file)
{
	return locks_inode(task->mm->exe_file) == locks_inode(new_file) &&
			 locks_inode(old_file) != locks_inode(new_file);
}

static void proca_hook_file_processed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *xattr,
				size_t xattr_size, int result)
{
	char *five_xattr_value = NULL;
	bool need_set_five = false;
	struct proca_task_descr *target_task_descr = NULL;

	target_task_descr = proca_table_get_by_pid(&g_proca_table, task->pid);

	if (!task->mm || !task->mm->exe_file)
		return;

	if (target_task_descr &&
		is_bprm(task, target_task_descr->proca_identity.file, file)) {
		PROCA_DEBUG_LOG(
			"Task descr for task %d already exists before exec\n",
			task->pid);

		target_task_descr = proca_table_remove_by_pid(
					&g_proca_table,
					task->pid);
		destroy_proca_task_descr(target_task_descr);
		target_task_descr = NULL;
	}

	if (!target_task_descr) {
		target_task_descr = prepare_proca_task_descr(task, file,
							xattr, xattr_size,
							&five_xattr_value);
		if (target_task_descr)
			proca_table_add_task_descr(&g_proca_table,
						target_task_descr);
	}

	need_set_five |= task_integrity_value_allow_sign(tint_value);

	if ((five_xattr_value || need_set_five) && !file->f_signature) {
		if (!five_xattr_value)
			read_xattr(file->f_path.dentry, XATTR_NAME_FIVE,
				   &five_xattr_value);
		file->f_signature = five_xattr_value;
	} else if (five_xattr_value && file->f_signature) {
		kfree(five_xattr_value);
	}
}

static void proca_hook_file_signed(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file, void *xattr,
				size_t xattr_size, int result)
{
	char *xattr_value = NULL;

	if (!file || result != 0 || !xattr)
		return;

	kfree(file->f_signature);

	xattr_value = kmemdup(xattr, xattr_size, GFP_KERNEL);
	file->f_signature = xattr_value;
}

static void proca_hook_file_skipped(struct task_struct *task,
				enum task_integrity_value tint_value,
				struct file *file)
{
	char *xattr_value = NULL;
	struct dentry *dentry;

	if (!task || !file)
		return;

	if (file->f_signature)
		return;

	dentry = file->f_path.dentry;

	if (task_integrity_value_allow_sign(tint_value) &&
		read_xattr(dentry, XATTR_NAME_FIVE, &xattr_value) != 0) {
		// PROCA get FIVE signature for runtime provisioning
		// from kernel, so
		// we should set f_signature for each signed file
		file->f_signature = xattr_value;
	} else if (__vfs_getxattr(dentry, dentry->d_inode, XATTR_NAME_PA,
				NULL, 0) > 0) {
		// Workaround for Android applications.
		// If file has user.pa - check it.
		five_file_verify(task, file);
	}
}

static void proca_hook_task_forked(struct task_struct *parent,
			enum task_integrity_value parent_tint_value,
			struct task_struct *child,
			enum task_integrity_value child_tint_value)
{
	struct proca_task_descr *target_task_descr = NULL;
	struct proca_identity ident;

	if (!parent || !child)
		return;

	target_task_descr = proca_table_get_by_pid(&g_proca_table, parent->pid);
	if (!target_task_descr)
		return;

	PROCA_DEBUG_LOG("Going to clone proca identity from task %d to %d\n",
			parent->pid, child->pid);

	if (proca_identity_copy(&ident, &target_task_descr->proca_identity))
		return;

	target_task_descr = create_proca_task_descr(child, &ident);
	if (!target_task_descr) {
		deinit_proca_identity(&ident);
		return;
	}

	proca_table_add_task_descr(&g_proca_table, target_task_descr);
}

static void proca_task_free_hook(struct task_struct *task)
{
	struct proca_task_descr *target_task_descr = NULL;

	target_task_descr = proca_table_remove_by_pid(&g_proca_table,
						      task->pid);

	destroy_proca_task_descr(target_task_descr);
}

static void proca_file_free_security_hook(struct file *file)
{
	kfree(file->f_signature);
	file->f_signature = NULL;
}

int proca_get_task_cert(const struct task_struct *task,
			const char **cert, size_t *cert_size)
{
	struct proca_task_descr *task_descr = NULL;

	BUG_ON(!task || !cert || !cert_size);

	task_descr = proca_table_get_by_pid(&g_proca_table, task->pid);
	if (!task_descr)
		return -ESRCH;

	*cert = task_descr->proca_identity.certificate;
	*cert_size = task_descr->proca_identity.certificate_size;
	return 0;
}

static __init int proca_module_init(void)
{
	int ret;

	ret = init_certificate_validation_hash();
	if (ret)
		return ret;

	proca_table_init(&g_proca_table);

	security_add_hooks(proca_ops, ARRAY_SIZE(proca_ops), "proca_lsm");
	five_add_hooks(five_ops, ARRAY_SIZE(five_ops));

	pr_info("PROCA LSM was initialized\n");

	return 0;
}
late_initcall(proca_module_init);

MODULE_DESCRIPTION("PROCA LSM module");
MODULE_LICENSE("GPL");
