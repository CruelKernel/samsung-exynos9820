/*
 * Copyright (c) 2020-2022 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/compat.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/task.h>
#endif

#include "include/dtm.h"
#include "include/dtm_engine.h"
#include "include/dtm_log.h"
#include "include/dtm_utils.h"

/* From fs/exec.c: SM8150_Q, SM8250_Q, SM8250_R, exynos9820, exynos9830 */
struct user_arg_ptr {
#ifdef CONFIG_COMPAT
	bool is_compat;
#endif
	union {
		const char __user *const __user *native;
#ifdef CONFIG_COMPAT
		const compat_uptr_t __user *compat;
#endif
	} ptr;
};

/* From fs/exec.c: SM8150_Q, SM8250_Q, SM8250_R, exynos9820, exynos9830 */
static const char __user *get_user_arg_ptr(struct user_arg_ptr argv, int nr)
{
	const char __user *native;

#ifdef CONFIG_COMPAT
	if (unlikely(argv.is_compat)) {
		compat_uptr_t compat;

		if (get_user(compat, argv.ptr.compat + nr))
			return ERR_PTR(-EFAULT);

		return compat_ptr(compat);
	}
#endif

	if (get_user(native, argv.ptr.native + nr))
		return ERR_PTR(-EFAULT);

	return native;
}

static void dtm_kfree_args(struct dtm_context *context)
{
	const char **argv;
	int arg, to_free;

	if (unlikely(!is_dtm_context_valid(context)))
		return;
	argv = context->callee_argv;
	arg = min_t(int, context->callee_argc, DTM_MAX_ARGC);
	to_free = context->callee_copied_argc;
	context->callee_copied_argc = 0;
	while (--arg >= 0 && to_free > 0) {
		if (!argv[arg])
			continue;
		kfree(argv[arg]);
		to_free--;
	}
}

/*
 * Gets call argument value, copying from user if needed.
 */
const char *dtm_get_callee_arg(struct dtm_context *context, int arg_index)
{
	struct user_arg_ptr argv;
	const char __user *user_str;
	char *copy;
	int max_argc, arg_len, copy_len;

	if (unlikely(!is_dtm_context_valid(context)))
		return NULL;
	max_argc = min_t(int, context->callee_argc, DTM_MAX_ARGC);
	if (unlikely((arg_index < 0) || (arg_index >= max_argc)))
		return NULL;
	if (context->callee_argv[arg_index])
		return context->callee_argv[arg_index];

	argv = *(struct user_arg_ptr *)context->callee_argv_ref;
	user_str = get_user_arg_ptr(argv, arg_index);
	if (IS_ERR(user_str))
		return NULL;

	arg_len = strnlen_user(user_str, MAX_ARG_STRLEN);
	if (unlikely(!arg_len))
		return NULL;

	copy_len = min_t(int, arg_len, DTM_MAX_ARG_STRLEN);
	copy = kzalloc(copy_len, GFP_KERNEL);
	if (unlikely(!copy))
		return NULL;

	if (unlikely(copy_from_user(copy, user_str, copy_len)))
		goto out_free_copy;
	copy[copy_len - 1] = '\0';

	context->callee_argv[arg_index] = copy;
	context->callee_copied_argc++;
	context->callee_copied_args_len += copy_len;
	context->callee_total_args_len += arg_len;
	return copy;

out_free_copy:
	kfree(copy);
	return NULL;
}

/*
 * Initializes dtm context data structure.
 */
__visible_for_testing bool dtm_context_get(struct dtm_context *context,
					   struct defex_context *defex_context,
					   int callee_argc,
					   void *callee_argv_ref)
{
	memset(context, 0, sizeof(*context));
	context->defex_context = defex_context;
	context->callee_argc = callee_argc;
	context->callee_argv_ref = callee_argv_ref;
	return true;
}

/*
 * Releases resources associated to dtm context.
 */
__visible_for_testing void dtm_context_put(struct dtm_context *context)
{
	dtm_kfree_args(context);
}

/*
 * Gets program name for current call.
 */
const char *dtm_get_program_name(struct dtm_context *context)
{
	if (unlikely(!is_dtm_context_valid(context)))
		return NULL;
	if (context->program_name)
		return context->program_name;
	context->program_name = dtm_get_callee_arg(context, 0);
	if (context->program_name == NULL)
		context->program_name = DTM_UNKNOWN;
	return context->program_name;
}

/**
 * Gets stdin mode bit for current call.
 */
int dtm_get_stdin_mode_bit(struct dtm_context *context)
{
	if (unlikely(!context))
		return DTM_FD_MODE_ERROR;
	if (!context->stdin_mode_bit)
		context->stdin_mode_bit = dtm_get_fd_mode_bit(0);
	return context->stdin_mode_bit;
}

/**
 * Gets stdin mode for current call.
 */
const char *dtm_get_stdin_mode(struct dtm_context *context)
{
	if (unlikely(!context))
		return NULL;
	if (!context->stdin_mode)
		context->stdin_mode = dtm_get_fd_mode_bit_name(
					dtm_get_stdin_mode_bit(context));
	return context->stdin_mode;
}

int defex_trusted_map_lookup(struct defex_context *defex_context,
			     int callee_argc, void *callee_argv_ref)
{
	int ret = DTM_DENY;
	struct dtm_context context;

	if (unlikely(!defex_context || !(defex_context->task)))
		goto out;
	if (unlikely(!dtm_context_get(&context, defex_context, callee_argc, callee_argv_ref)))
		goto out;
	ret = dtm_enforce(&context);
	dtm_context_put(&context);
out:
	return ret;
}
