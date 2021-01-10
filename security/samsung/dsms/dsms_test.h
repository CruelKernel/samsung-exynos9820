/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_TEST_H
#define _DSMS_TEST_H

/* -------------------------------------------------------------------------- */
/* macros to allow testing initialization code */
/* -------------------------------------------------------------------------- */

#if defined(DSMS_KUNIT_ENABLED)

#define declare_kunit_init_module(init_function_name)		\
	extern int init_function_name##_kunit_helper(void)

#define kunit_init_module(init_function_name)			\
	declare_kunit_init_module(init_function_name);		\
	static __init int init_function_name(void)		\
	{							\
		return init_function_name##_kunit_helper();	\
	}							\
	int init_function_name##_kunit_helper(void)

#else

#define declare_kunit_init_module(init_function_name)

#define kunit_init_module(init_function_name)			\
	static __init int init_function_name(void)

#endif /* defined(DSMS_KUNIT_ENABLED) */

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Functions exported for each module (using __visible_for_testing) */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#if !defined(DSMS_KUNIT_ENABLED)

#define __visible_for_testing static

#else

#include <kunit/mock.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/umh.h>

/* -------------------------------------------------------------------------- */
/* kmalloc mock */
/* -------------------------------------------------------------------------- */

extern void *security_dsms_test_kmalloc_mock(size_t size, gfp_t flags);
#define kmalloc security_dsms_test_kmalloc_mock

/* -------------------------------------------------------------------------- */
/* dsms_access_control */
/* -------------------------------------------------------------------------- */

struct dsms_policy_entry;

extern int compare_policy_entries(const void *function_name, const void *entry);
extern struct dsms_policy_entry *find_policy_entry(const char *function_name);

/* -------------------------------------------------------------------------- */
/* dsms_init */
/* -------------------------------------------------------------------------- */

declare_kunit_init_module(dsms_init);

/* -------------------------------------------------------------------------- */
/* dsms_kernel_api */
/* -------------------------------------------------------------------------- */

extern atomic_t list_counter;
extern struct dsms_message *create_message(const char *feature_code,
		const char *detail,
		int64_t value);

/* -------------------------------------------------------------------------- */
/* dsms_rate_limit */
/* -------------------------------------------------------------------------- */

extern int dsms_message_count;
extern u64 dsms_round_start_ms;

extern int dsms_get_max_messages_per_round(void);
extern u64 round_end_ms(u64 round_start_ms);
extern int is_new_round(u64 now_ms, u64 last_round_start_ms);
extern u64 dsms_get_time_ms(void);

#endif /* !defined(DSMS_KUNIT_ENABLED) */
#endif /* _DSMS_TEST_H */
