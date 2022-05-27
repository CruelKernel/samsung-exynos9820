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
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/umh.h>
#include "dsms_kernel_api.h"
#include "dsms_test.h"
#include "security_dsms_test_utils.h"

/* -------------------------------------------------------------------------- */
/* Module test functions */
/* -------------------------------------------------------------------------- */

static void dsms_alloc_user_test(struct test *test)
{
	static const char *const s[] = {NULL, "", "a", "Hello, world!\nBye bye world!"};
	static const struct { int64_t i; const char *s; } i_s[] = {
		{0, "0"}, {1, "1"}, {12, "12"}, {123456789L, "123456789"}, {-123, "-123"},
	};
	int i;
	const char *ss;

	for (i = 0; i < ARRAY_SIZE(s); ++i) {
		ss = dsms_alloc_user_string(s[i]);
		EXPECT_STREQ(test, s[i] == NULL ? "" : s[i], ss);
		dsms_free_user_string(ss);
	}
	for (i = 0; i < ARRAY_SIZE(i_s); ++i) {
		ss = dsms_alloc_user_value(i_s[i].i);
		EXPECT_STREQ(test, i_s[i].s, ss);
		dsms_free_user_string(ss);
	}
}

static void dsms_message_cleanup_test(struct test *test)
{
	static struct subprocess_info mock_subprocess_info;
	char **argv;
	int i;
	static const int argv_size = 5;

	atomic_inc(&message_counter);
	dsms_message_cleanup(NULL);

	argv = kmalloc_array(argv_size, sizeof(char *), GFP_KERNEL);
	for (i = 0; i < argv_size; i++)
		argv[i] = NULL;
	mock_subprocess_info.argv = argv;
	atomic_inc(&message_counter);
	dsms_message_cleanup(&mock_subprocess_info);
}

static void dsms_send_message_test(struct test *test)
{
	// should fail, not yet in policy. TODO success test
	EXPECT_EQ(test, -1, dsms_send_message("KUNIT", "kunit test", 0));
}

/*
 * dsms_send_allowed_message_test - deploy two tests on the
 * dsms_send_allowed_message function
 * @test - struct test pointer to the running test instance context.
 *
 * Test 1 - The error-free call to the function
 * Test 2 - Trigger the limit error case
 * Test 3 - Trigger the memory error cases
 */
static void dsms_send_allowed_message_test(struct test *test)
{
	// Test 1 - The error-free call to the function
	EXPECT_NE(test, -EBUSY, dsms_send_allowed_message("KUNIT", "kunit test", 0));

	// Test 2 - Trigger the limit error case
	atomic_set(&message_counter, MESSAGE_COUNT_LIMIT);
	EXPECT_EQ(test, -EBUSY, dsms_send_allowed_message("KUNIT", "kunit test", 0));
	atomic_set(&message_counter, 0);

	// Test 3 - Trigger the memory error cases
	dsms_test_request_kmalloc_fail_at(1);
	EXPECT_EQ(test, -ENOMEM, dsms_send_allowed_message("KUNIT", "kunit test", 0));
	dsms_test_request_kmalloc_fail_at(2);
	EXPECT_EQ(test, -ENOMEM, dsms_send_allowed_message("KUNIT", "kunit test", 0));
	dsms_test_cancel_kmalloc_fail_requests();
}

/* -------------------------------------------------------------------------- */
/* Module definition */
/* -------------------------------------------------------------------------- */

static struct test_case security_dsms_kernel_api_test_cases[] = {
	TEST_CASE(dsms_alloc_user_test),
	TEST_CASE(dsms_message_cleanup_test),
	TEST_CASE(dsms_send_message_test),
	TEST_CASE(dsms_send_allowed_message_test),
	{},
};

static struct test_module security_dsms_kernel_api_module = {
	.name = "security-dsms-kernel-api",
	.test_cases = security_dsms_kernel_api_test_cases,
};
module_test(security_dsms_kernel_api_module);
