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
#ifdef DEFEX_DSMS_ENABLE
#include <linux/dsms.h>
#endif
#include <linux/file.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/namei.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/stddef.h>
#ifdef DEFEX_DSMS_ENABLE
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#endif

#ifdef DEFEX_DEPENDING_ON_OEMUNLOCK
bool boot_state_unlocked __ro_after_init;
static int __init verifiedboot_state_setup(char *str)
{
	static const char unlocked[] = "orange";

	boot_state_unlocked = !strncmp(str, unlocked, sizeof(unlocked));

	if(boot_state_unlocked)
		pr_crit("Device is unlocked and DEFEX will be disabled.");

	return 0;
}

__setup("androidboot.verifiedbootstate=", verifiedboot_state_setup);

int warranty_bit __ro_after_init;
static int __init get_warranty_bit(char *str)
{
	get_option(&str, &warranty_bit);

	return 0;
}

__setup("androidboot.warranty_bit=", get_warranty_bit);
#endif /* DEFEX_DEPENDING_ON_OEMUNLOCK */

__visible_for_testing struct task_struct *get_parent_task(const struct task_struct *p)
{
	struct task_struct *parent = NULL;

	read_lock(&tasklist_lock);
	parent = p->parent;
	if (parent)
		get_task_struct(parent);
	read_unlock(&tasklist_lock);
	return parent;
}

#ifdef DEFEX_DSMS_ENABLE

#	define PED_VIOLATION "DFX1"
#	define SAFEPLACE_VIOLATION "DFX2"
#	define INTEGRITY_VIOLATION "DFX3"
#	define IMMUTABLE_VIOLATION "DFX4"
#	define MESSAGE_BUFFER_SIZE 200
#	define STORED_CREDS_SIZE 100

static void defex_report_violation(const char *violation, uint64_t counter, struct defex_context *dc,
	uid_t stored_uid, uid_t stored_fsuid, uid_t stored_egid, int case_num)
{
	int usermode_result;
	char message[MESSAGE_BUFFER_SIZE + 1];

	struct task_struct *parent = NULL, *p = dc->task;
	const uid_t uid = uid_get_value(dc->cred.uid);
	const uid_t euid = uid_get_value(dc->cred.euid);
	const uid_t fsuid = uid_get_value(dc->cred.fsuid);
	const uid_t egid = uid_get_value(dc->cred.egid);
	const char *process_name = p->comm;
	const char *prt_process_name = NULL;
	const char *program_path = get_dc_process_name(dc);
	char *prt_program_path = NULL;
	char *file_path = NULL;
	char stored_creds[STORED_CREDS_SIZE + 1];

	parent = get_parent_task(p);
	if (!parent)
		return;

	prt_process_name = parent->comm;
	prt_program_path = defex_get_filename(parent);

	if (dc->target_file && !case_num) {
		file_path = get_dc_target_name(dc);
	} else {
		snprintf(stored_creds, sizeof(stored_creds), "[%ld, %ld, %ld]", (long)stored_uid, (long)stored_fsuid, (long)stored_egid);
		stored_creds[sizeof(stored_creds) - 1] = 0;
	}
#ifdef DEFEX_DEPENDING_ON_OEMUNLOCK
	snprintf(message, sizeof(message), "%d, %d, sc=%d, tsk=%s(%s), %s(%s), [%ld %ld %ld %ld], %s%s, %d", warranty_bit, boot_state_unlocked, dc->syscall_no, process_name, program_path, prt_process_name,
		prt_program_path, (long)uid, (long)euid, (long)fsuid, (long)egid,
		(file_path ? "file=" : "stored "), (file_path ? file_path : stored_creds), case_num);
#else
	snprintf(message, sizeof(message), "sc=%d, tsk=%s(%s), %s(%s), [%ld %ld %ld %ld], %s%s, %d",
		dc->syscall_no, process_name, program_path, prt_process_name,
		prt_program_path, (long)uid, (long)euid, (long)fsuid, (long)egid,
		(file_path ? "file=" : "stored "), (file_path ? file_path : stored_creds), case_num);
#endif
	message[sizeof(message) - 1] = 0;

	usermode_result = dsms_send_message(violation, message, counter);
#ifdef DEFEX_DEBUG_ENABLE
	printk(KERN_ERR "DEFEX Violation : feature=%s value=%ld, detail=[%s]", violation, (long)counter, message);
	printk(KERN_ERR "DEFEX Result : %d\n", usermode_result);
#endif /* DEFEX_DEBUG_ENABLE */

	safe_str_free(prt_program_path);
	put_task_struct(parent);
}
#endif /* DEFEX_DSMS_ENABLE */

#ifdef DEFEX_SAFEPLACE_ENABLE
__visible_for_testing long kill_process(struct task_struct *p)
{
	read_lock(&tasklist_lock);
	send_sig(SIGKILL, p, 0);
	read_unlock(&tasklist_lock);
	return 0;
}
#endif /* DEFEX_SAFEPLACE_ENABLE */

#ifdef DEFEX_PED_ENABLE
__visible_for_testing long kill_process_group(struct task_struct *p, int tgid, int pid)
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

__visible_for_testing int task_defex_is_secured(struct defex_context *dc)
{
	struct file *exe_file = get_dc_process_file(dc);
	char *proc_name = get_dc_process_name(dc);
	int is_secured = 1;

	if (!get_dc_process_dpath(dc))
		return is_secured;

	is_secured = !rules_lookup2(proc_name, feature_ped_exception, exe_file);
	return is_secured;
}

__visible_for_testing int at_same_group(unsigned int uid1, unsigned int uid2)
{
	static const unsigned int lod_base = 0x61A8;

	/* allow the weaken privilege */
	if (uid1 >= 10000 && uid2 < 10000) return 1;
	/* allow traverse in the same class */
	if ((uid1 / 1000) == (uid2 / 1000)) return 1;
	/* allow traverse to isolated ranges */
	if (uid1 >= 90000) return 1;
	/* allow LoD process */
	return ((uid1 >> 16) == lod_base) && ((uid2 >> 16) == lod_base);
}

__visible_for_testing int at_same_group_gid(unsigned int gid1, unsigned int gid2)
{
	static const unsigned int lod_base = 0x61A8, inet = 3003;

	/* allow the weaken privilege */
	if (gid1 >= 10000 && gid2 < 10000) return 1;
	/* allow traverse in the same class */
	if ((gid1 / 1000) == (gid2 / 1000)) return 1;
	/* allow traverse to isolated ranges */
	if (gid1 >= 90000) return 1;
	/* allow LoD process */
	return (((gid1 >> 16) == lod_base) || (gid1 == inet)) && ((gid2 >> 16) == lod_base);
}

#ifdef DEFEX_LP_ENABLE
/* Lower Permission feature decision function */
__visible_for_testing int lower_adb_permission(struct defex_context *dc, unsigned short cred_flags)
{
	char *parent_file;
	struct task_struct *parent = NULL, *p = dc->task;
#ifndef DEFEX_PERMISSIVE_LP
	struct cred *shellcred;
	static const char adbd_str[] = "/system/bin/adbd";
#endif /* DEFEX_PERMISSIVE_LP */
	int ret = 0;

	parent = get_parent_task(p);
	if (!parent || p->pid == 1 || parent->pid == 1)
		goto out;

	parent_file = defex_get_filename(parent);

#ifndef DEFEX_PERMISSIVE_LP
	if (!strncmp(parent_file, adbd_str, sizeof(adbd_str))) {
		shellcred = prepare_creds();
		pr_crit("[DEFEX] adb with root");
		if (!shellcred) {
			pr_crit("[DEFEX] prepare_creds fail");
			ret = 0;
			goto out;
		}

		uid_set_value(shellcred->uid, 2000);
		uid_set_value(shellcred->suid, 2000);
		uid_set_value(shellcred->euid, 2000);
		uid_set_value(shellcred->fsuid, 2000);
		uid_set_value(shellcred->gid, 2000);
		uid_set_value(shellcred->sgid, 2000);
		uid_set_value(shellcred->egid, 2000);
		uid_set_value(shellcred->fsgid, 2000);
		commit_creds(shellcred);
		memcpy(&dc->cred, shellcred, sizeof(struct cred));
		set_task_creds(p, 2000, 2000, 2000, cred_flags);

		ret = 1;
	}
#endif /* DEFEX_PERMISSIVE_LP */

	safe_str_free(parent_file);
out:
	if (parent)
		put_task_struct(parent);
	return ret;
}
#endif /* DEFEX_LP_ENABLE */

/* Cred. violation feature decision function */
#define AID_MEDIA_RW	1023
#define AID_MEDIA_OBB	1059
#define AID_SYSTEM	1000

__visible_for_testing int task_defex_check_creds(struct defex_context *dc)
{
	char *path = NULL;
	int check_deeper, case_num;
	unsigned int cur_uid, cur_euid, cur_fsuid, cur_egid;
	unsigned int ref_uid, ref_fsuid, ref_egid;
	struct task_struct *parent, *p = dc->task;
	unsigned short cred_flags;
	const struct cred *parent_cred;
	static const unsigned int dead_uid = 0xDEADBEAF;

	if (!is_task_creds_ready() || !p->cred)
		goto out;

	get_task_creds(p, &ref_uid, &ref_fsuid, &ref_egid, &cred_flags);

	cur_uid = uid_get_value(dc->cred.uid);
	cur_euid = uid_get_value(dc->cred.euid);
	cur_fsuid = uid_get_value(dc->cred.fsuid);
	cur_egid = uid_get_value(dc->cred.egid);

	if (!ref_uid) {
		if (p->tgid != p->pid && p->tgid != 1) {
			path = get_dc_process_name(dc);
			pr_crit("defex[6]: cred wasn't stored [task=%s, filename=%s, uid=%d, tgid=%u, pid=%u, ppid=%u]\n",
				p->comm, path, cur_uid, p->tgid, p->pid, p->real_parent->pid);
			pr_crit("defex[6]: stored [euid=%d fsuid=%d egid=%d] current [uid=%d euid=%d fsuid=%d egid=%d]\n",
				ref_uid, ref_fsuid, ref_egid, cur_uid, cur_euid, cur_fsuid, cur_egid);
			goto exit;
		}

		parent = get_parent_task(p);
		if (parent) {
			parent_cred = get_task_cred(parent);
			if (CHECK_ROOT_CREDS(parent_cred))
				cred_flags |= CRED_FLAGS_PROOT;
			put_cred(parent_cred);
			put_task_struct(parent);
		}

		if (CHECK_ROOT_CREDS(&dc->cred)) {
#ifdef DEFEX_LP_ENABLE
			if (!lower_adb_permission(dc, cred_flags))
#endif /* DEFEX_LP_ENABLE */
			{
				set_task_creds(p, 1, 1, 1, cred_flags);
			}
		}
		else
			set_task_creds(p, cur_euid, cur_fsuid, cur_egid, cred_flags);
	} else if (ref_uid == 1) {
		if (!CHECK_ROOT_CREDS(&dc->cred))
			set_task_creds(p, cur_euid, cur_fsuid, cur_egid, cred_flags);
	} else if (ref_uid == dead_uid) {
		path = get_dc_process_name(dc);
		pr_crit("defex[5]: process wasn't killed [task=%s, filename=%s, uid=%d, tgid=%u, pid=%u, ppid=%u]\n",
			p->comm, path, cur_uid, p->tgid, p->pid, p->real_parent->pid);
		pr_crit("defex[5]: stored [euid=%d fsuid=%d egid=%d] current [uid=%d euid=%d fsuid=%d egid=%d]\n",
			ref_uid, ref_fsuid, ref_egid, cur_uid, cur_euid, cur_fsuid, cur_egid);
		goto exit;
	} else {
		check_deeper = 0;
		/* temporary allow fsuid changes to "media_rw" */
		if ( (cur_uid != ref_uid) ||
				(cur_euid != ref_uid) ||
	 			(cur_egid != ref_egid) ||
	  			!((cur_fsuid == ref_fsuid) ||
	  			 (cur_fsuid == ref_uid) ||
	  			 (cur_fsuid%100000 == AID_SYSTEM) ||
	  			 (cur_fsuid%100000 == AID_MEDIA_RW) ||
	  			 (cur_fsuid%100000 == AID_MEDIA_OBB)) ) {
			check_deeper = 1;
			if (CHECK_ROOT_CREDS(&dc->cred))
				set_task_creds(p, 1, 1, 1, cred_flags);
			else
				set_task_creds(p, cur_euid, cur_fsuid, cur_egid, cred_flags);
		}
		if (check_deeper &&
				(!at_same_group(cur_uid, ref_uid) ||
				!at_same_group(cur_euid, ref_uid) ||
				!at_same_group_gid(cur_egid, ref_egid) ||
				!at_same_group(cur_fsuid, ref_fsuid)) &&
				task_defex_is_secured(dc)) {
			case_num = ((p->tgid == p->pid) ? 1 : 2);
			goto trigger_violation;
		}
	}

	if (CHECK_ROOT_CREDS(&dc->cred) && !(cred_flags & CRED_FLAGS_PROOT) && task_defex_is_secured(dc)) {
		if (p->tgid != p->pid) {
			case_num = 3;
			goto trigger_violation;
		}
		case_num = 4;
		goto trigger_violation;
	}

out:
	return DEFEX_ALLOW;

trigger_violation:
	set_task_creds(p, dead_uid, dead_uid, dead_uid, cred_flags);
	path = get_dc_process_name(dc);
	pr_crit("defex[%d]: credential violation [task=%s, filename=%s, uid=%d, tgid=%u, pid=%u, ppid=%u]\n",
		case_num, p->comm, path, cur_uid, p->tgid, p->pid, p->real_parent->pid);
	pr_crit("defex[%d]: stored [euid=%d fsuid=%d egid=%d] current [uid=%d euid=%d fsuid=%d egid=%d]\n",
		case_num, ref_uid, ref_fsuid, ref_egid, cur_uid, cur_euid, cur_fsuid, cur_egid);

#ifdef DEFEX_DSMS_ENABLE
	defex_report_violation(PED_VIOLATION, 0, dc, ref_uid, ref_fsuid, ref_egid, case_num);
#endif /* DEFEX_DSMS_ENABLE */

exit:
	return -DEFEX_DENY;
}
#endif /* DEFEX_PED_ENABLE */

#ifdef DEFEX_SAFEPLACE_ENABLE
/* Safeplace feature decision function */
__visible_for_testing int task_defex_safeplace(struct defex_context *dc)
{
	int ret = DEFEX_ALLOW, is_violation = 0;
	char *proc_file, *new_file;
	struct task_struct *p = dc->task;

	if (!CHECK_ROOT_CREDS(&dc->cred))
		goto out;

	if (!get_dc_target_dpath(dc))
		goto out;

	new_file = get_dc_target_name(dc);
	is_violation = rules_lookup2(new_file, feature_safeplace_path, dc->target_file);
#ifdef DEFEX_INTEGRITY_ENABLE
	if (is_violation != DEFEX_INTEGRITY_FAIL)
#endif /* DEFEX_INTEGRITY_ENABLE */
		is_violation = !is_violation;

	if (is_violation) {
		ret = -DEFEX_DENY;
		proc_file = get_dc_process_name(dc);

#ifdef DEFEX_INTEGRITY_ENABLE
		if (is_violation == DEFEX_INTEGRITY_FAIL) {
			pr_crit("defex: integrity violation [task=%s (%s), child=%s, uid=%d]\n",
				p->comm, proc_file, new_file, uid_get_value(dc->cred.uid));
#ifdef DEFEX_DSMS_ENABLE
			defex_report_violation(INTEGRITY_VIOLATION, 0, dc, 0, 0, 0, 0);
#endif /* DEFEX_DSMS_ENABLE */

			/*  Temporary make permissive mode for tereble
			 *  system image is changed as google's and defex might not work
			 */
			ret = DEFEX_ALLOW;
		}
		else
#endif /* DEFEX_INTEGRITY_ENABLE */
		{
			pr_crit("defex: safeplace violation [task=%s (%s), child=%s, uid=%d]\n",
				p->comm, proc_file, new_file, uid_get_value(dc->cred.uid));
#ifdef DEFEX_DSMS_ENABLE
			defex_report_violation(SAFEPLACE_VIOLATION, 0, dc, 0, 0, 0, 0);
#endif /* DEFEX_DSMS_ENABLE */
		}
	}
out:
	return ret;
}
#endif /* DEFEX_SAFEPLACE_ENABLE */

#ifdef DEFEX_IMMUTABLE_ENABLE

/* Immutable feature decision function */
__visible_for_testing int task_defex_src_exception(struct defex_context *dc)
{
	struct file *exe_file = get_dc_process_file(dc);
	char *proc_name = get_dc_process_name(dc);
	int allow = 1;

	if (!get_dc_process_dpath(dc))
		return allow;

	exe_file = get_dc_process_file(dc);
	allow = rules_lookup2(proc_name, feature_immutable_src_exception, exe_file);
	return allow;
}

/* Immutable feature decision function */
__visible_for_testing int task_defex_immutable(struct defex_context *dc, int attribute)
{
	int ret = DEFEX_ALLOW, is_violation = 0;
	char *proc_file, *new_file;
	struct task_struct *p = dc->task;

	if (!get_dc_target_dpath(dc))
		goto out;

	new_file = get_dc_target_name(dc);
	is_violation = rules_lookup2(new_file, attribute, dc->target_file);

	if (is_violation) {
		/* Check the Source exception and self-access */
		if (attribute == feature_immutable_path_open &&
				(task_defex_src_exception(dc) ||
				defex_files_identical(get_dc_process_file(dc), dc->target_file)))
			goto out;

		ret = -DEFEX_DENY;
		proc_file = get_dc_process_name(dc);
		pr_crit("defex: immutable %s violation [task=%s (%s), access to:%s]\n",
			(attribute==feature_immutable_path_open)?"open":"write", p->comm, proc_file, new_file);
#ifdef DEFEX_DSMS_ENABLE
 		defex_report_violation(IMMUTABLE_VIOLATION, 0, dc, 0, 0, 0, 0);
#endif /* DEFEX_DSMS_ENABLE */
	}
out:
	return ret;
}
#endif /* DEFEX_IMMUTABLE_ENABLE */

/* Main decision function */
int task_defex_enforce(struct task_struct *p, struct file *f, int syscall)
{
	int ret = DEFEX_ALLOW;
	int feature_flag;
	const struct local_syscall_struct *item;
	struct defex_context dc;

#ifdef DEFEX_DEPENDING_ON_OEMUNLOCK
	if(boot_state_unlocked)
		return ret;
#endif /* DEFEX_DEPENDING_ON_OEMUNLOCK */

	if (!p || p->pid == 1 || !p->mm)
		return ret;

	if (syscall < 0) {
		item = get_local_syscall(-syscall);
		if (!item)
			return ret;
		syscall = item->local_syscall;
	}

	feature_flag = defex_get_features();
	get_task_struct(p);
	init_defex_context(&dc, syscall, p, f);

#ifdef DEFEX_PED_ENABLE
	/* Credential escalation feature */
	if (feature_flag & FEATURE_CHECK_CREDS)	{
		ret = task_defex_check_creds(&dc);
		if (ret) {
			if (!(feature_flag & FEATURE_CHECK_CREDS_SOFT)) {
				kill_process_group(p, p->tgid, p->pid);
				goto do_deny;
			}
		}
	}
#endif /* DEFEX_PED_ENABLE */

#ifdef DEFEX_SAFEPLACE_ENABLE
	/* Safeplace feature */
	if (feature_flag & FEATURE_SAFEPLACE) {
		if (syscall == __DEFEX_execve) {
			ret = task_defex_safeplace(&dc);
			if (ret == -DEFEX_DENY) {
				if (!(feature_flag & FEATURE_SAFEPLACE_SOFT)) {
					kill_process(p);
					goto do_deny;
				}
			}
		}
	}
#endif /* DEFEX_SAFEPLACE_ENABLE */

#ifdef DEFEX_IMMUTABLE_ENABLE
	/* Immutable feature */
	if (feature_flag & FEATURE_IMMUTABLE) {
		if (syscall == __DEFEX_openat || syscall == __DEFEX_write) {
			ret = task_defex_immutable(&dc,
				(syscall == __DEFEX_openat)?feature_immutable_path_open:feature_immutable_path_write);
			if (ret == -DEFEX_DENY) {
				if (!(feature_flag & FEATURE_IMMUTABLE_SOFT)) {
					goto do_deny;
				}
			}
		}
	}
#endif /* DEFEX_IMMUTABLE_ENABLE */
	release_defex_context(&dc);
	put_task_struct(p);
	return DEFEX_ALLOW;

do_deny:
	release_defex_context(&dc);
	put_task_struct(p);
	return -DEFEX_DENY;
}

int task_defex_zero_creds(struct task_struct *tsk)
{
	int is_fork = -1;
	get_task_struct(tsk);
	if (tsk->flags & (PF_KTHREAD | PF_WQ_WORKER)) {
		put_task_struct(tsk);
		return 0;
	}
	if (is_task_creds_ready()) {
		is_fork = ((tsk->flags & PF_FORKNOEXEC) && (!READ_ONCE(tsk->on_rq)));
#ifdef TASK_NEW
		if (!is_fork && (tsk->state & TASK_NEW))
			is_fork = 1;
#endif /* TASK_NEW */
		set_task_creds_tcnt(tsk, is_fork?1:-1);
	}

#ifdef DEFEX_CACHES_ENABLE
	defex_file_cache_delete(tsk->pid);
#endif /* DEFEX_CACHES_ENABLE */
	put_task_struct(tsk);
	return 0;
}

static unsigned int rules_load_cnt = 0;
int task_defex_user_exec(const char *new_file)
{
#ifdef DEFEX_UMH_RESTRICTION_ENABLE
	int res = DEFEX_DENY, is_violation = DEFEX_DENY;
	struct file *fp = NULL;

#ifdef DEFEX_DEPENDING_ON_OEMUNLOCK
	if(boot_state_unlocked)
		return DEFEX_ALLOW;
#endif /* DEFEX_DEPENDING_ON_OEMUNLOCK */

	if (!check_rules_ready()) {
		if (rules_load_cnt++%100 == 0)
			printk("[DEFEX] Rules not ready\n");
		goto umh_out;
	}

	if (current == NULL || current->fs == NULL) {
		goto umh_out;
	}

	fp = local_fopen(new_file, O_RDONLY, 0);
	if (IS_ERR(fp) || (fp == NULL)) {
		goto umh_out;
	} else {
		filp_close(fp, NULL);
	}

	is_violation = !rules_lookup2(new_file, feature_umhbin_path, NULL);
	if (is_violation) {
		printk("[DEFEX] UMH Exec Denied: %s\n", new_file);
		goto umh_out;
	}

	res = DEFEX_ALLOW;
umh_out:
	return res;
#else
	return DEFEX_ALLOW;
#endif /* DEFEX_UMH_RESTRICTION_ENABLE */
}
