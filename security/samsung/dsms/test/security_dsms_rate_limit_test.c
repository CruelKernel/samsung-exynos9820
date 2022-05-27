/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <kunit/mock.h>
#include <kunit/test.h>
#include <linux/dsms.h>
#include <linux/errno.h>
#include "dsms_rate_limit.h"
#include "dsms_test.h"

static u64 start_ms;

/* -------------------------------------------------------------------------- */
/* Module test functions */
/* -------------------------------------------------------------------------- */

static void round_end_ms_test(struct test *test)
{
	EXPECT_EQ(test, start_ms + ((u64)(1000L)), round_end_ms(start_ms));
	EXPECT_NE(test, start_ms + ((u64)(1001L)), round_end_ms(start_ms));
}

static void is_new_round_test(struct test *test)
{
	u64 now_ms = dsms_get_time_ms();

	EXPECT_EQ(test, 0, is_new_round(now_ms, start_ms));
}

static void dsms_check_message_rate_limit_deny_test(struct test *test)
{
	int failed = 0, i;

	for (i = dsms_get_max_messages_per_round(); i >= 0; --i)
		if (dsms_check_message_rate_limit() == DSMS_DENY)
			failed = 1;
	EXPECT_TRUE(test, failed);
}

static void dsms_check_message_rate_limit_success_test(struct test *test)
{
	EXPECT_EQ(test, DSMS_SUCCESS, dsms_check_message_rate_limit());
}

/* Test boundary cases (simulate clock wrapped, too many messages) */
static void dsms_check_message_rate_limit_boundary_test(struct test *test)
{
	int old_count;

	dsms_round_start_ms -= 10;
	EXPECT_EQ(test, DSMS_SUCCESS, dsms_check_message_rate_limit());
	old_count = dsms_message_count;
	dsms_round_start_ms = 0;
	dsms_message_count = dsms_get_max_messages_per_round() + 1;
	EXPECT_EQ(test, DSMS_SUCCESS, dsms_check_message_rate_limit());
	EXPECT_EQ(test, dsms_message_count, 0);
	dsms_message_count = old_count;
}

/**
 * dsms_check_message_rate_limit_reset_test
 *
 * This test sets the "dsms_round_start_ms" variable to the maximum value
 * of an unsigned 64 bit type (2^64 - 1). Such modification triggers the
 * "[rate limit] RESET" case on "dsms_check_message_rate_limit" function.
 *
 * @param test - struct test pointer to the running test instance context.
 */
static void dsms_check_message_rate_limit_reset_test(struct test *test)
{
	dsms_round_start_ms = -1;
	EXPECT_EQ(test, DSMS_SUCCESS, dsms_check_message_rate_limit());
}

/* -------------------------------------------------------------------------- */
/* Module initialization and exit functions */
/* -------------------------------------------------------------------------- */

static int dsms_rate_test_init(struct test *test)
{
	dsms_rate_limit_init();
	start_ms = dsms_get_time_ms();
	return 0;
}

/* -------------------------------------------------------------------------- */
/* Module definition */
/* -------------------------------------------------------------------------- */

static struct test_case dsms_rate_test_cases[] = {
	TEST_CASE(round_end_ms_test),
	TEST_CASE(is_new_round_test),
	TEST_CASE(dsms_check_message_rate_limit_deny_test),
	TEST_CASE(dsms_check_message_rate_limit_success_test),
	TEST_CASE(dsms_check_message_rate_limit_boundary_test),
	TEST_CASE(dsms_check_message_rate_limit_reset_test),
	{},
};

static struct test_module dsms_rate_test_module = {
	.name = "security-dsms-rate-limit-test",
	.init = dsms_rate_test_init,
	.test_cases = dsms_rate_test_cases,
};
module_test(dsms_rate_test_module);
