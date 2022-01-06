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

#include <gpex_utils.h>
#include <gpex_clock.h>

#include <runtime_test_runner.h>
#include "clock_runtime_tests.h"
#include "pm_state_runtime_tests.h"
#include "clock_sysfs_tests.h"

enum test_modules { GPEX_CLOCK = 0, GPEX_CLOCK_SYSFS, GPEX_PM, GPEX_TEST_MODULE_CNT };

static char *test_module_name[GPEX_TEST_MODULE_CNT];
static uint64_t final_test_result = -1;

static int run_tests_gpex_clock()
{
	int result = 0;
	int min_level, max_level, avg_level, avg_clock;

	min_level = gpex_clock_get_table_idx(gpex_clock_get_min_clock());
	max_level = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	avg_level = (min_level + max_level) / 2;
	avg_clock = gpex_clock_get_clock(avg_level);

	result |= test_gpex_clock_init();
	result |= test_gpex_clock_term();
	result |= test_gpex_clock_prepare_runtime_off();
	result |= test_gpex_clock_set(avg_clock);
	result |= test_gpex_clock_lock_clock();
	result |= test_gpex_clock_get_clock_slow();
	result |= test_gpex_clock_get_table_idx(avg_clock);
	result |= test_gpex_clock_get_table_idx_cur();
	result |= test_gpex_clock_get_boot_clock();
	result |= test_gpex_clock_get_max_clock();
	result |= test_gpex_clock_get_max_clock_limit();
	result |= test_gpex_clock_get_min_clock();
	result |= test_gpex_clock_get_cur_clock();
	result |= test_gpex_clock_get_min_lock();
	result |= test_gpex_clock_get_max_lock();
	result |= test_gpex_clock_mutex_lock();
	result |= test_gpex_clock_mutex_unlock();
	result |= test_gpex_clock_get_voltage(avg_clock);

	/* TODO: These tests are not complete yet */
#if 0
	if (result)
		return -1;
#endif

	return 0;
}

static int run_tests_gpex_pm_state()
{
	int result = 0;

	result |= test_gpex_pm_state_init();
	result |= test_gpex_pm_state_set_bad_state();
	result |= test_gpex_pm_state_set_good_state();
	result |= test_gpex_pm_state_term();

	if (result)
		return -1;

	return 0;
}

static int run_tests_clock_sysfs_all()
{
	return clock_sysfs_tests_run_all();
}

static char *stringify_test_result(enum test_modules tmodule)
{
	uint64_t is_failed = final_test_result & (1u << tmodule);

	return is_failed ? "FAIL" : "PASS";
}

static ssize_t show_runtime_tests_result(char *buf)
{
	ssize_t ret = 0;
	int idx;

	for (idx = 0; idx < GPEX_TEST_MODULE_CNT; idx++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s: %s\n", test_module_name[idx],
				stringify_test_result(idx));
	}

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_runtime_tests_result)
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_runtime_tests_result)

static void mark_passed(enum test_modules tmodule)
{
	final_test_result &= ~(1u << tmodule);
}

static void mark_result(enum test_modules tmodule, int result)
{
	if (!result)
		mark_passed(tmodule);
}

static ssize_t run_runtime_tests(const char *buf, size_t count)
{
	final_test_result = -1;

	mark_result(GPEX_CLOCK, run_tests_gpex_clock());
	mark_result(GPEX_CLOCK_SYSFS, run_tests_clock_sysfs_all());
	mark_result(GPEX_PM, run_tests_gpex_pm_state());

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(run_runtime_tests)
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(run_runtime_tests)

int runtime_test_runner_init()
{
	test_module_name[GPEX_CLOCK] = "gpex clock";
	test_module_name[GPEX_CLOCK_SYSFS] = "gpex clock sysfs";
	test_module_name[GPEX_PM] = "gpex pm";

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(test_runner, show_runtime_tests_result, run_runtime_tests);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(test_runner, show_runtime_tests_result,
					  run_runtime_tests);

	return 0;
}

void runtime_test_runner_term()
{
	return;
}
