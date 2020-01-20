/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <asm/barrier.h>
#include <linux/compiler.h>
#include <linux/const.h>
#ifdef CONFIG_SECURITY_DSMS
#include <linux/dsms.h>
#endif
#include <linux/file.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/namei.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#ifdef CONFIG_SECURITY_DSMS
#include <linux/string.h>
#endif
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include "include/defex_caches.h"
#include "include/defex_catch_list.h"
#include "include/defex_config.h"
#include "include/defex_internal.h"
#include "include/defex_rules.h"

#ifdef CONFIG_SECURITY_DSMS

#	define PED_VIOLATION "DFX1"
#	define SAFEPLACE_VIOLATION "DFX2"
#	define INTEGRITY_VIOLATION "DFX3"
#	define MESSAGE_BUFFER_SIZE 200
#	define STORED_CREDS_SIZE 100

static void defex_report_violation(const char *violation, uint64_t counter,
	int syscall, struct task_struct *p, struct file *f, uid_t stored_uid, uid_t stored_fsuid, uid_t stored_egid)
{
	int usermode_result;
	char message[MESSAGE_BUFFER_SIZE + 1];

	const uid_t uid = uid_get_value(p->cred->uid);
	const uid_t euid = uid_get_value(p->cred->euid);
	const uid_t fsuid = uid_get_value(p->cred->fsuid);
	const uid_t egid = uid_get_value(p->cred->egid);
	const char *process_name = p->comm;
	const char *program_path = defex_get_filename(p);
	const pid_t pid = p->pid, tgid = p->tgid;
	
	char *file_path = NULL, *buff = NULL;
	const struct path *dpath = NULL;

	char stored_creds[STORED_CREDS_SIZE + 1];

	if (f != NULL) {
		buff = kzalloc(PATH_MAX, GFP_KERNEL);
		if (buff != NULL) {
			dpath = &(f->f_path);
			file_path = d_path(dpath, buff, PATH_MAX);
		}
	}
	else {
		snprintf(stored_creds, sizeof(stored_creds), "[euid=%ld fsuid=%ld egid=%ld]", (long)stored_uid, (long)stored_fsuid, (long)stored_egid);
		stored_creds[sizeof(stored_creds) - 1] = 0;
	}

	snprintf(message, sizeof(message), "sc=%d tsk=%s (%s) pid=%u tgid=%u uid=%ld euid=%ld fsuid=%ld egid=%ld%s%s",
		syscall, process_name, (program_path ? program_path : ""), pid, tgid, (long)uid, (long)euid, (long)fsuid, (long)egid,
		(file_path ? " file=" : " stored "), (file_path ? file_path : stored_creds));
	message[sizeof(message) - 1] = 0;

#ifdef DEFEX_DEBUG_ENABLE
	printk(KERN_ERR "DEFEX Violation : feature=%s value=%ld, detail=[%s]",
		violation, (long)counter, message);
#endif /* DEFEX_DEBUG_ENABLE */
	usermode_result = dsms_send_message(violation, message, counter);
#ifdef DEFEX_DEBUG_ENABLE
	printk(KERN_ERR "DEFEX Result : %d\n", usermode_result);
#endif /* DEFEX_DEBUG_ENABLE */

	kfree(program_path);
	kfree(buff);
}
#endif /* CONFIG_SECURITY_DSMS */

#ifdef DEFEX_SAFEPLACE_ENABLE
static long kill_process(struct task_struct *p)
{
	read_lock(&tasklist_lock);
	force_sig(SIGKILL, p);
	read_unlock(&tasklist_lock);
	return 0;
}
#endif /* DEFEX_SAFEPLACE_ENABLE */

#ifdef DEFEX_PED_ENABLE
static long kill_process_group(struct task_struct *p, int tgid, int pid)
{
	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (p->tgid == tgid)
			send_sig(SIGKILL, p, 0);
	}
	send_sig(SIGKILL, current, 0);
	read_unlock(&tasklist_lock);
	return 0;
}
#endif /* DEFEX_PED_ENABLE */

struct file *defex_get_source_file(struct task_struct *p)
{
	struct file *file_addr = NULL;

#ifdef DEFEX_CACHES_ENABLE
	file_addr = defex_file_cache_find(p->pid);

	if (file_addr == NULL) {
		file_addr = get_mm_exe_file(p->mm);
		defex_file_cache_add(p->pid, file_addr);
	} else {
		down_read(&p->mm->mmap_sem);
		if (file_addr != p->mm->exe_file) {
			file_addr = p->mm->exe_file;
			if (!file_addr) {
				up_read(&p->mm->mmap_sem);
				return NULL;
			}
			defex_file_cache_update(file_addr);
			get_file(file_addr);
		}
		up_read(&p->mm->mmap_sem);
	}
#else
	file_addr = get_mm_exe_file(p->mm);
#endif /* DEFEX_CACHES_ENABLE */
	return file_addr;
}

char *defex_get_filename(struct task_struct *p)
{
	struct file *exe_file = NULL;
	const struct path *dpath = NULL;
	char *path = NULL, *buff = NULL;
	char *filename = NULL;

	exe_file = defex_get_source_file(p);
	if (!exe_file)
		goto out_filename;

	dpath = &exe_file->f_path;

	buff = kzalloc(PATH_MAX, GFP_ATOMIC);
	if (buff)
		path = d_path(dpath, buff, PATH_MAX);

#ifndef DEFEX_CACHES_ENABLE
	fput(exe_file);
#endif /* DEFEX_CACHES_ENABLE */

out_filename:
	if (IS_ERR(path) || !path)
		filename = kstrdup("<unknown filename>", GFP_ATOMIC);
	else
		filename = kstrdup(path, GFP_ATOMIC);

	kfree(buff);
	return filename;
}

#ifdef DEFEX_PED_ENABLE
static int task_defex_is_secured(struct task_struct *p)
{
	struct file *exe_file = NULL;
	const struct path *dpath = NULL;
	int is_secured = 1;

	exe_file = defex_get_source_file(p);
	if (!exe_file)
		goto skip_secured;

	dpath = &exe_file->f_path;
	if (!dpath->dentry || !dpath->dentry->d_inode)
		goto out_secured;

	is_secured = !rules_lookup(dpath, feature_ped_exception, exe_file);

out_secured:
#ifndef DEFEX_CACHES_ENABLE
	fput(exe_file);
#endif /* DEFEX_CACHES_ENABLE */
skip_secured:
	return is_secured;
}
#endif /* DEFEX_PED_ENABLE */

#ifdef DEFEX_PED_ENABLE
static int at_same_group(unsigned int uid1, unsigned int uid2)
{
	static const unsigned int lod_base = 0x61A8;

	/* allow the weaken privilege */
	if (uid1 >= 10000 && uid2 < 10000) return 1;
	/* allow traverse in the same class */
	if ((uid1 / 1000) == (uid2 / 1000)) return 1;
	/* allow LoD process */
	return ((uid1 >> 16) == lod_base) && ((uid2 >> 16) == lod_base);
}

static int at_same_group_gid(unsigned int gid1, unsigned int gid2)
{
	static const unsigned int lod_base = 0x61A8, inet = 3003;

	/* allow the weaken privilege */
	if (gid1 >= 10000 && gid2 < 10000) return 1;
	/* allow traverse in the same class */
	if ((gid1 / 1000) == (gid2 / 1000)) return 1;
	/* allow LoD process */
	return (((gid1 >> 16) == lod_base) || (gid1 == inet)) && ((gid2 >> 16) == lod_base);
}

/* Cred. violation feature decision function */
#ifndef CONFIG_SECURITY_DSMS
static int task_defex_check_creds(struct task_struct *p)
#else
static int task_defex_check_creds(struct task_struct *p, int syscall)
#endif /* CONFIG_SECURITY_DSMS */
{
	char *path = NULL;
	int check_deeper, case_num;
	unsigned int cur_uid, cur_euid, cur_fsuid, cur_egid;
	unsigned int uid, fsuid, egid;
	unsigned int g_uid, g_fsuid, g_egid;
	static const unsigned int dead_uid = 0xDEADBEAF;

	if (!is_task_creds_ready() || !p->cred)
		goto out;

	get_task_creds(p->pid, &uid, &fsuid, &egid);
	if (p->tgid != p->pid) {
		get_task_creds(p->tgid, &g_uid, &g_fsuid, &g_egid);
	} else {
		g_uid = uid;
		g_fsuid = fsuid;
		g_egid = egid;
	}

	cur_uid = uid_get_value(p->cred->uid);
	cur_euid = uid_get_value(p->cred->euid);
	cur_fsuid = uid_get_value(p->cred->fsuid);
	cur_egid = uid_get_value(p->cred->egid);

	if (!uid) {
		if (CHECK_ROOT_CREDS(p))
			set_task_creds(p->pid, 1, 1, 1);
		else
			set_task_creds(p->pid, cur_euid, cur_fsuid, cur_egid);
	} else if (uid == 1) {
		if (!CHECK_ROOT_CREDS(p))
			set_task_creds(p->pid, cur_euid, cur_fsuid, cur_egid);
	} else if (uid == dead_uid || g_uid == dead_uid) {
		path = defex_get_filename(p);
		pr_crit("defex[5]: process wasn't killed [task=%s, filename=%s, uid=%d]\n", p->comm, path, cur_uid);
		pr_crit("defex[5]: uid=%d euid=%d fsuid=%d egid=%d\n",
			cur_uid, cur_euid, cur_fsuid, cur_egid);
		goto exit;
	} else {
		check_deeper = 0;
		if ((cur_uid != uid) || (cur_euid != uid) || !((cur_fsuid == fsuid) || (cur_fsuid == uid)) || (cur_egid != egid)) {
			check_deeper = 1;
			set_task_creds(p->pid, cur_euid, cur_fsuid, cur_egid);
		}
		if (check_deeper && (!at_same_group(cur_uid, uid) ||
				!at_same_group(cur_euid, uid) ||
				!at_same_group_gid(cur_egid, egid) ||
				!at_same_group(cur_fsuid, fsuid)) &&
				task_defex_is_secured(p)) {
			set_task_creds(p->pid, dead_uid, dead_uid, dead_uid);
			if (p->tgid != p->pid)
				set_task_creds(p->tgid, dead_uid, dead_uid, dead_uid);
			case_num = 1;
			goto show_violation;
		}

		if (p->tgid != p->pid) {
			if ((g_uid > 1) && (!at_same_group(cur_uid, g_uid) ||
					!at_same_group(cur_euid, g_uid) ||
					!at_same_group_gid(cur_egid, g_egid)) &&
					task_defex_is_secured(p)) {
				set_task_creds(p->tgid, dead_uid, dead_uid, dead_uid);
				if (p->tgid != p->pid)
					set_task_creds(p->pid, dead_uid, dead_uid, dead_uid);
				case_num = 2;
				goto show_violation;
			}
		}
	}

	if ((p->tgid != p->pid) && CHECK_ROOT_CREDS(p) && !CHECK_ROOT_CREDS(p->real_parent)) {
		if ((g_uid > 1) && task_defex_is_secured(p)) {
			set_task_creds(p->tgid, dead_uid, dead_uid, dead_uid);
			if (p->tgid != p->pid)
				set_task_creds(p->pid, dead_uid, dead_uid, dead_uid);
			case_num = 3;
			goto show_violation;
		}
	}

	if (CHECK_ROOT_CREDS(p) && !CHECK_ROOT_CREDS(p->real_parent) &&
			task_defex_is_secured(p)) {
		set_task_creds(p->pid, dead_uid, dead_uid, dead_uid);
		if (p->tgid != p->pid)
			set_task_creds(p->tgid, dead_uid, dead_uid, dead_uid);
		case_num = 4;
		goto show_violation;
	}

out:
	return DEFEX_ALLOW;

show_violation:
	path = defex_get_filename(p);
	pr_crit("defex[%d]: credential violation [task=%s, filename=%s, uid=%d]\n",
		case_num, p->comm, (path ? path : ""), cur_uid);
	pr_crit("defex[%d]: stored [euid=%d fsuid=%d egid=%d] uid=%d euid=%d fsuid=%d egid=%d\n",
		case_num, uid, fsuid, egid, cur_uid, cur_euid, cur_fsuid, cur_egid);

#ifdef CONFIG_SECURITY_DSMS
	defex_report_violation(PED_VIOLATION, 0, syscall, p, NULL, uid, fsuid, egid);
#endif /* CONFIG_SECURITY_DSMS */

exit:
	kfree(path);
	return -DEFEX_DENY;
}
#endif /* DEFEX_PED_ENABLE */

#ifdef DEFEX_SAFEPLACE_ENABLE
/* Safeplace feature decision function */
static int task_defex_safeplace(struct task_struct *p, struct file *f)
{
	static const char def[] = "";
	int ret = DEFEX_ALLOW, is_violation = 0;
	char *proc_file, *new_file = (char *)def, *buff;
	const struct path *dpath = NULL;

	if (!CHECK_ROOT_CREDS(p))
		goto out;

	if (IS_ERR(f))
		goto out;

	dpath = &f->f_path;
	if (!dpath->dentry || !dpath->dentry->d_inode)
		goto out;

	is_violation = rules_lookup(dpath, feature_safeplace_path, f);
#ifdef DEFEX_INTEGRITY_ENABLE
	if (is_violation != DEFEX_INTEGRITY_FAIL)
#endif /* DEFEX_INTEGRITY_ENABLE */
		is_violation = !is_violation;

	if (is_violation) {
		ret = -DEFEX_DENY;
		proc_file = defex_get_filename(p);
		buff = kzalloc(PATH_MAX, GFP_ATOMIC);
		if (buff)
			new_file = d_path(dpath, buff, PATH_MAX);

#ifdef DEFEX_INTEGRITY_ENABLE
		if (is_violation == DEFEX_INTEGRITY_FAIL) {
			pr_crit("defex: integrity violation [task=%s (%s), child=%s, uid=%d]\n",
				p->comm, (proc_file ? proc_file : ""), new_file, uid_get_value(p->cred->uid));
#ifdef CONFIG_SECURITY_DSMS
			defex_report_violation(INTEGRITY_VIOLATION, 0, __DEFEX_execve, p, f, 0, 0, 0);
#endif /* CONFIG_SECURITY_DSMS */

			/*  Temporary make permissive mode for tereble
			 *  system image is changed as google's and defex might not work
			 */
			ret = DEFEX_ALLOW;
		}
		else
#endif /* DEFEX_INTEGRITY_ENABLE */
		{
			pr_crit("defex: safeplace violation [task=%s (%s), child=%s, uid=%d]\n",
				p->comm, (proc_file ? proc_file : ""), new_file, uid_get_value(p->cred->uid));
#ifdef CONFIG_SECURITY_DSMS
			defex_report_violation(SAFEPLACE_VIOLATION, 0, __DEFEX_execve, p, f, 0, 0, 0);
#endif /* CONFIG_SECURITY_DSMS */
		}

		kfree(proc_file);
		kfree(buff);
	}
out:
	return ret;
}
#endif /* DEFEX_SAFEPLACE_ENABLE */

/* Main decision function */
int task_defex_enforce(struct task_struct *p, struct file *f, int syscall)
{
	int ret = DEFEX_ALLOW;
	int feature_flag;
	const struct local_syscall_struct *item;

	if (!p || p->pid == 1 || !p->mm)
		return ret;

	if (syscall < 0) {
		item = get_local_syscall(-syscall);
		if (!item)
			return ret;
		syscall = item->local_syscall;
	}

	feature_flag = defex_get_features();

#ifdef DEFEX_PED_ENABLE
	/* Credential escalation feature */
	if (feature_flag & FEATURE_CHECK_CREDS)	{
#ifndef CONFIG_SECURITY_DSMS
		ret = task_defex_check_creds(p);
#else
		ret = task_defex_check_creds(p, syscall);
#endif /* CONFIG_SECURITY_DSMS */
		if (ret) {
			if (!(feature_flag & FEATURE_CHECK_CREDS_SOFT)) {
				kill_process_group(p, p->tgid, p->pid);
				return -DEFEX_DENY;
			}
		}
	}
#endif /* DEFEX_PED_ENABLE */

#ifdef DEFEX_SAFEPLACE_ENABLE
	/* Safeplace feature */
	if (feature_flag & FEATURE_SAFEPLACE) {
		if (syscall == __DEFEX_execve) {
			ret = task_defex_safeplace(p, f);
			if (ret == -DEFEX_DENY) {
				if (!(feature_flag & FEATURE_SAFEPLACE_SOFT)) {
					kill_process(p);
					return -DEFEX_DENY;
				}
			}
		}
	}
#endif /* DEFEX_SAFEPLACE_ENABLE */

	return DEFEX_ALLOW;
}

int task_defex_zero_creds(struct task_struct *tsk)
{
	if (is_task_creds_ready())
		delete_task_creds(tsk->pid);

#ifdef DEFEX_CACHES_ENABLE
	defex_file_cache_delete(tsk->pid);
#endif /* DEFEX_CACHES_ENABLE */

	return 0;
}

