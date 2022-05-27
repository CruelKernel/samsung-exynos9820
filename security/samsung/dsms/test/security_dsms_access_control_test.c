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
#include "dsms_access_control.h"
#include "dsms_test.h"

/* -------------------------------------------------------------------------- */
/* Module test functions */
/* -------------------------------------------------------------------------- */

static void find_policy_entry_test(struct test *test)
{
	EXPECT_EQ(test, NULL, find_policy_entry("test"));
}

static void compare_policy_entries_test(struct test *test)
{
	struct dsms_policy_entry entry;

	entry.file_path = "/path/test";
	entry.function_name = "myfunction";
	EXPECT_GT(test, compare_policy_entries("myfunction1", &entry), 0);
	EXPECT_EQ(test, compare_policy_entries("myfunction", &entry), 0);
	EXPECT_LT(test, compare_policy_entries("myfunct", &entry), 0);
	entry.function_name = "myfunction1";
	EXPECT_EQ(test, compare_policy_entries("myfunction1", &entry), 0);
	EXPECT_LT(test, compare_policy_entries("Myfunction", &entry), 0);
}

static void should_ignore_allowlist_suffix_test(struct test *test)
{
	EXPECT_EQ(test, 1, should_ignore_allowlist_suffix());
}

static void dsms_policy_size_test(struct test *test)
{
	EXPECT_EQ(test, 3, dsms_policy_size());
	EXPECT_LT(test, 0, dsms_policy_size());
	EXPECT_GT(test, 4, dsms_policy_size());
}

static void dsms_verify_access_test(struct test *test)
{
	EXPECT_EQ(test, DSMS_DENY, dsms_verify_access(NULL));
}

/*
 * dsms_verify_access_address_not_in_kallsyms_test - caller address not in
 * kallsyms test case
 * @test - struct test pointer to the running test instance context.
 *
 * Test the case where the address passed to dsms_verify_access is not null and
 * is not in the kallsyms. It is expected to return a DSMS_DENY.
 */
static void dsms_verify_access_address_not_in_kallsyms_test(struct test *test)
{
	EXPECT_EQ(test, DSMS_DENY, dsms_verify_access((const void *)0x1));
}

/* -------------------------------------------------------------------------- */
/* Module definition */
/* -------------------------------------------------------------------------- */

static struct test_case dsms_access_control_test_cases[] = {
	TEST_CASE(compare_policy_entries_test),
	TEST_CASE(should_ignore_allowlist_suffix_test),
	TEST_CASE(dsms_policy_size_test),
	TEST_CASE(dsms_verify_access_test),
	TEST_CASE(find_policy_entry_test),
	TEST_CASE(dsms_verify_access_address_not_in_kallsyms_test),
	{},
};

static struct test_module dsms_access_control_test_module = {
	.name = "security-dsms-access-control-test",
	.test_cases = dsms_access_control_test_cases,
};
module_test(dsms_access_control_test_module);
