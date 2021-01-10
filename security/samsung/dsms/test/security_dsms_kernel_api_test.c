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
#include "dsms_message_list.h"
#include "dsms_test.h"
#include "security_dsms_test_utils.h"

/* -------------------------------------------------------------------------- */
/* Module test functions */
/* -------------------------------------------------------------------------- */

static void security_dsms_send_message_test(struct test *test)
{
	// should fail, not yet in policy. TODO success test
	EXPECT_EQ(test, -1, dsms_send_message("KUNIT", "kunit test", 0));
}

/*
 * dsms_process_dsms_message_test - deploy two tests on the
 * process_dsms_message function
 * @test - struct test pointer to the running test instance context.
 *
 * Test 1 - The error-free call to the function
 * Test 2 - Trigger the limit error case
 * Test 3 - Trigger the memory error case
 */
static void security_dsms_process_dsms_message_test(struct test *test)
{
	struct dsms_message *message;
	int test1;
	int test2;
	int test3;

	message = create_message("KNIT", "kunit test", 0);
	EXPECT_NE(test, NULL, message);

	// Test 1 - The error-free call to the function
	test1 = process_dsms_message(message);
	EXPECT_NE(test, -EBUSY, test1);

	// If the message was processed successfully we need to create another message.
	if (test1 == 0) {
		message = create_message("KNIT", "kunit test", 0);
		EXPECT_NE(test, NULL, message);
	}

	// Test 2 - Trigger the limit error case
	atomic_set(&list_counter, LIST_COUNT_LIMIT);
	test2 = process_dsms_message(message);
	EXPECT_EQ(test, -EBUSY, test2);
	atomic_set(&list_counter, 0);

	// If the message was processed successfully we need to create another message.
	if (test2 == 0) {
		message = create_message("KNIT", "kunit test", 0);
		EXPECT_NE(test, NULL, message);
	}

	// Test 3 - Trigger the memory error case
	security_dsms_test_request_kmalloc_fail_at(1);
	test3 = process_dsms_message(message);
	EXPECT_EQ(test, -ENOMEM, test3);
	security_dsms_test_cancel_kmalloc_fail_requests();

	// If the message was not processed successfully we need to free the memory
	// in the end of the test.
	if (test3 != 0) {
		kfree(message->feature_code);
		kfree(message->detail);
		kfree(message);
	}
}

/* -------------------------------------------------------------------------- */
/* Module definition */
/* -------------------------------------------------------------------------- */

static struct test_case security_dsms_kernel_api_test_cases[] = {
	TEST_CASE(security_dsms_send_message_test),
	TEST_CASE(security_dsms_process_dsms_message_test),
	{},
};

static struct test_module security_dsms_kernel_api_module = {
	.name = "security-dsms-kernel-api",
	.test_cases = security_dsms_kernel_api_test_cases,
};
module_test(security_dsms_kernel_api_module);
