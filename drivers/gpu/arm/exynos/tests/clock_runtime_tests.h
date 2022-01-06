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
int test_gpex_clock_init(void);
int test_gpex_clock_term(void);
int test_gpex_clock_prepare_runtime_off(void);
int test_gpex_clock_set(int clk);
int test_gpex_clock_lock_clock(void);
int test_gpex_clock_get_clock_slow(void);
int test_gpex_clock_get_table_idx(int clock);
int test_gpex_clock_get_table_idx_cur(void);
int test_gpex_clock_get_boot_clock(void);
int test_gpex_clock_get_max_clock(void);
int test_gpex_clock_get_max_clock_limit(void);
int test_gpex_clock_get_min_clock(void);
int test_gpex_clock_get_cur_clock(void);
int test_gpex_clock_get_min_lock(void);
int test_gpex_clock_get_max_lock(void);
int test_gpex_clock_mutex_lock(void);
int test_gpex_clock_mutex_unlock(void);
int test_gpex_clock_get_voltage(int clk);
