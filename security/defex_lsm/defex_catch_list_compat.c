/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
*/

#include <asm/unistd32.h>
#include <linux/errno.h>
#include <linux/types.h>
#include "include/defex_catch_list.h"

#ifdef __NR_seccomp
#define __NR_compat_syscalls		(__NR_seccomp + 1)
#else
#define __NR_compat_syscalls		400
#endif /* __NR_seccomp */

#define DEFEX_CATCH_COUNT	__NR_compat_syscalls
const int defex_nr_syscalls_compat = DEFEX_CATCH_COUNT;

#include "defex_catch_list.inc"

const struct local_syscall_struct *get_local_syscall_compat(int syscall_no)
{
	if (syscall_no >= __NR_compat_syscalls)
		return NULL;

	if (!syscall_catch_arr[syscall_no].local_syscall && !syscall_catch_arr[syscall_no].err_code && syscall_no) {
		return &syscall_catch_arr[0];
	}

	return &syscall_catch_arr[syscall_no];
}
