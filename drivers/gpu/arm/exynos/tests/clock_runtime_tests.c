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

#include <gpex_clock.h>

int test_gpex_clock_init(void)
{
	return 0;
}

/**
 * test_gpex_clock_term - test functionality of gpex_clock_term function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_term(void)
{
	return -1;
}

/**
 * test_gpex_clock_prepare_runtime_off - test functionality of gpex_clock_prepare_runtime_off function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_prepare_runtime_off(void)
{
	return -1;
}

/**
 * test_gpex_clock_set - test functionality of gpex_clock_set function
 *
 * @clk:		Input clock value provided
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_set(int clk)
{
	int curr_clk;

	gpex_clock_set(clk);
	curr_clk = gpex_clock_get_cur_clock();

	if (clk == curr_clk) {
		return 0;
	}

	return -1;
}

/**
 * test_gpex_clock_lock_clock - test functionality of gpex_clock_lock_clock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_lock_clock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_clock_slow - test functionality of gpex_clock_get_clock_slow function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_clock_slow(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_table_idx - test functionality of gpex_clock_get_table_idx function
 *
 * @clk:		Input clock value provided
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_table_idx(int clk)
{
	return -1;
}

/**
 * test_gpex_clock_get_table_idx_cur - test functionality of gpex_clock_get_table_idx_cur function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_table_idx_cur(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_boot_clock - test functionality of gpex_clock_get_boot_clock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_boot_clock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_max_clock  - test functionality of gpex_clock_get_max_clock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_max_clock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_max_clock_limit  - test functionality of gpex_clock_get_max_clock_limit function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_max_clock_limit(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_min_clock  - test functionality of gpex_clock_get_min_clock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_min_clock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_cur_clock  - test functionality of gpex_clock_get_cur_clock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_cur_clock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_min_lock  - test functionality of gpex_clock_get_min_lock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_min_lock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_max_lock  - test functionality of gpex_clock_get_max_lock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_max_lock(void)
{
	return -1;
}

/**
 * test_gpex_clock_mutex_lock  - test functionality of gpex_clock_mutex_lock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_mutex_lock(void)
{
	return -1;
}

/**
 * test_gpex_clock_mutex_unlock  - test functionality of test_gpex_clock_mutex_unlock function
 *
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_mutex_unlock(void)
{
	return -1;
}

/**
 * test_gpex_clock_get_voltage - test functionality of gpex_clock_get_voltage function
 *
 * @clk:		Input clock value provided
 * Return:  0 if successful, -1 if unsuccessful
 */
int test_gpex_clock_get_voltage(int clk)
{
	int curr_voltage;
	int voltage;

	voltage = gpex_clock_get_voltage(clk);
	curr_voltage = gpex_clock_get_voltage(clk);

	if (voltage == curr_voltage) {
		return 0;
	}

	return -1;
}
