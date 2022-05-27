/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <kunit/test.h>
#include <kunit/mock.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "security_dsms_test_utils.h"

/* test utils "sees" actual kmalloc */
#undef kmalloc

/* -------------------------------------------------------------------------- */
/* General test functions: kmalloc mock function */
/* -------------------------------------------------------------------------- */

/* each bit indicates if kmalloc mock should return fail (NULL) */
static uint64_t dsms_test_kmalloc_fail_requests;

void *dsms_test_kmalloc_mock(size_t size, gfp_t flags)
{
	bool fail;

	fail = dsms_test_kmalloc_fail_requests & 1ul;
	dsms_test_kmalloc_fail_requests >>= 1;
	return fail ? NULL : kmalloc(size, flags);
}

/* Requests that kmalloc fails in the attempt given by argument (1 for next) */
void dsms_test_request_kmalloc_fail_at(int attempt_no)
{
	if (attempt_no > 0)
		dsms_test_kmalloc_fail_requests |= (1ul << (attempt_no-1));
}

/* Cancels all kmalloc fail requests */
void dsms_test_cancel_kmalloc_fail_requests(void)
{
	dsms_test_kmalloc_fail_requests = 0;
}

/* -------------------------------------------------------------------------- */
/* Module test functions */
/* -------------------------------------------------------------------------- */

static void dsms_test_kmalloc_mock_test(struct test *test)
{
	void *p;

	dsms_test_request_kmalloc_fail_at(1);
	dsms_test_request_kmalloc_fail_at(3);
	EXPECT_EQ(test, p = dsms_test_kmalloc_mock(1, GFP_KERNEL), NULL);
	kfree(p);
	EXPECT_NE(test, p = dsms_test_kmalloc_mock(1, GFP_KERNEL), NULL);
	kfree(p);
	EXPECT_EQ(test, p = dsms_test_kmalloc_mock(1, GFP_KERNEL), NULL);
	kfree(p);
	EXPECT_NE(test, p = dsms_test_kmalloc_mock(1, GFP_KERNEL), NULL);
	kfree(p);
}

/* -------------------------------------------------------------------------- */
/* Module initialization and exit functions */
/* -------------------------------------------------------------------------- */

static int security_dsms_test_utils_init(struct test *test)
{
	dsms_test_cancel_kmalloc_fail_requests();
	return 0;
}

static void security_dsms_test_utils_exit(struct test *test)
{
	dsms_test_cancel_kmalloc_fail_requests();
}

/* -------------------------------------------------------------------------- */
/* Module definition */
/* -------------------------------------------------------------------------- */

static struct test_case security_dsms_test_utils_test_cases[] = {
	TEST_CASE(dsms_test_kmalloc_mock_test),
	{},
};

static struct test_module security_dsms_test_utils_module = {
	.name = "security-dsms-test-utils-test",
	.init = security_dsms_test_utils_init,
	.exit = security_dsms_test_utils_exit,
	.test_cases = security_dsms_test_utils_test_cases,
};
module_test(security_dsms_test_utils_module);
