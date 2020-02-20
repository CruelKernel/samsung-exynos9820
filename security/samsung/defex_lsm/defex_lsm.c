/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include "include/defex_caches.h"
#include "include/defex_catch_list.h"
#include "include/defex_internal.h"

MODULE_DESCRIPTION("Defex Linux Security Module");

asmlinkage int defex_syscall_enter(long int syscallno, struct pt_regs *regs);
asmlinkage int (* const defex_syscall_catch_enter)(long int syscallno, struct pt_regs *regs) = defex_syscall_enter;

asmlinkage int defex_syscall_enter(long int syscallno, struct pt_regs *regs)
{
	long err;
	const struct local_syscall_struct *item;

	if (!current)
		return 0;

#if !defined(__arm__) && defined(CONFIG_COMPAT)
	if (regs->pstate & PSR_MODE32_BIT)
		item = get_local_syscall_compat(syscallno);
	else
#endif /* __arm__ && CONFIG_COMPAT */
		item = get_local_syscall(syscallno);

	if (!item)
		return 0;

	err = item->err_code;
	if (err) {
		if (task_defex_enforce(current, NULL, item->local_syscall))
			return err;
	}
	return 0;
}

//INIT/////////////////////////////////////////////////////////////////////////
static __init int defex_lsm_init(void)
{
	int ret;

#ifdef DEFEX_CACHES_ENABLE
	defex_file_cache_init();
#endif /* DEFEX_CACHES_ENABLE */
	creds_fast_hash_init();

#ifdef DEFEX_PED_ENABLE
	hash_init(creds_hash);
#endif /* DEFEX_PED_ENABLE */

	ret = defex_init_sysfs();
	if (ret) {
		pr_crit("DEFEX_LSM defex_init_sysfs() failed!");
		return ret;
	}

	printk(KERN_INFO "DEFEX_LSM started");
	return 0;
}

module_init(defex_lsm_init);
