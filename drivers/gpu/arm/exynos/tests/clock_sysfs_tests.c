/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

/* Implements */
#include "clock_sysfs_tests.h"

/* Uses */
#include <gpex_clock.h>
#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <mali_kbase_hwaccess_pm.h>

#define CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(ret)                                                  \
	{                                                                                          \
		pr_err("mali clock sysfs utest failed: %s:%d", __func__, __LINE__);                \
		return ret;                                                                        \
	}

static char test_buf[PAGE_SIZE];

static void test_helper_strtoint(int *out)
{
	kstrtoint(test_buf, 10, out);
}

static void test_helper_reset_test_buf(void)
{
	memset(test_buf, 0, sizeof(test_buf));
}

int test_get_valid_gpu_clock(void)
{
	int min_clock;
	int max_clock;
	int clock;

	min_clock = gpex_clock_get_min_clock();
	max_clock = gpex_clock_get_max_clock();

	for (clock = min_clock; clock <= max_clock; clock++) {
		int obtained_clock = gpex_get_valid_gpu_clock(clock, false);

		if (obtained_clock > clock)
			CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

		if (obtained_clock < 0)
			CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);
	}

	return 0;
}

EXTERN_SYSFS_DEVICE_READ_FUNCTION(show_clock);
EXTERN_SYSFS_KOBJECT_READ_FUNCTION(show_clock);
int test_show_clock(void)
{
	int min_clock;
	int max_clock;
	int clock_read;

	min_clock = gpex_clock_get_min_clock();
	max_clock = gpex_clock_get_max_clock();

	gpex_clock_set(min_clock);

	test_helper_reset_test_buf();
	show_clock_sysfs_device(NULL, NULL, test_buf);
	test_helper_strtoint(&clock_read);
	if (clock_read != min_clock)
		CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

	test_helper_reset_test_buf();
	show_clock_sysfs_kobject(NULL, NULL, test_buf);
	test_helper_strtoint(&clock_read);
	if (clock_read != min_clock)
		CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

	gpex_clock_set(max_clock);

	test_helper_reset_test_buf();
	show_clock_sysfs_device(NULL, NULL, test_buf);
	test_helper_strtoint(&clock_read);
	if (clock_read != max_clock)
		CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

	test_helper_reset_test_buf();
	show_clock_sysfs_kobject(NULL, NULL, test_buf);
	test_helper_strtoint(&clock_read);
	if (clock_read != max_clock)
		CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

	gpex_clock_set(min_clock);

	return 0;
}

EXTERN_SYSFS_DEVICE_WRITE_FUNCTION(set_clock);
EXTERN_SYSFS_KOBJECT_WRITE_FUNCTION(set_clock);
int test_set_clock(void)
{
	int min_clock;
	int max_clock;
	char clock_str[32];

	min_clock = gpex_clock_get_min_clock();
	max_clock = gpex_clock_get_max_clock();

	gpex_clock_set(min_clock);

	snprintf(clock_str, sizeof(clock_str), "%d", max_clock);
	set_clock_sysfs_device(NULL, NULL, clock_str, 0);
	if (max_clock != gpex_clock_get_clock_slow())
		CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

	snprintf(clock_str, sizeof(clock_str), "%d", min_clock);
	set_clock_sysfs_device(NULL, NULL, clock_str, 0);
	if (min_clock != gpex_clock_get_clock_slow())
		CLOCK_SYSFS_TEST_FAIL_LOG_AND_RETURN(-1);

	gpex_clock_set(min_clock);

	/* TODO: add test for set_clock to 0, which enables DVFS again */
	//set_clock_sysfs_device(NULL, NULL, "0", 1);

	return 0;
}

int clock_sysfs_tests_run_all()
{
	int idx, result = 0;
	int policy_count;
	struct kbase_device *kbdev;
	const struct kbase_pm_policy *const *policy_list;
	static const struct kbase_pm_policy *prev_policy;

	/* Disable DVFS  and switch to Always On PM Policy for duration of test */
	gpex_dvfs_disable();

	kbdev = gpex_utils_get_kbase_device();
	prev_policy = kbase_pm_get_policy(kbdev);
	policy_count = kbase_pm_list_policies(kbdev, &policy_list);

	for (idx = 0; idx < policy_count; idx++) {
		if (sysfs_streq(policy_list[idx]->name, "always_on")) {
			kbase_pm_set_policy(kbdev, policy_list[idx]);
			break;
		}
	}

	result |= test_get_valid_gpu_clock();
	result |= test_show_clock();
	result |= test_set_clock();

	kbase_pm_set_policy(kbdev, prev_policy);

	gpex_dvfs_enable();

	if (result)
		return -1;

	return 0;
}
