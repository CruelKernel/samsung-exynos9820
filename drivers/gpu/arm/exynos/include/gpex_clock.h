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

#ifndef _MALI_EXYNOS_CLOCK_H_
#define _MALI_EXYNOS_CLOCK_H_

#include <mali_kbase.h>
#include <linux/device.h>
typedef enum {
	GPU_CLOCK_MAX_LOCK = 0,
	GPU_CLOCK_MIN_LOCK,
	GPU_CLOCK_MAX_UNLOCK,
	GPU_CLOCK_MIN_UNLOCK,
} gpex_clock_lock_cmd_t;

typedef enum {
	TMU_LOCK = 0,
	SYSFS_LOCK,
	PMQOS_LOCK,
	CLBOOST_LOCK,
	NUMBER_LOCK
} gpex_clock_lock_type_t;

/*************************************
 * INIT/TERM
 ************************************/
int gpex_clock_init(struct device **dev);
void gpex_clock_term(void);

/*************************************
 * CALLBACKS
 ************************************/
/**
 * gpex_clock_prepare_runtime_off()
 *
 * Prepare for runtime off by updating time in state and applying
 * SOC specific work-arounds needed for runtime off.
 */
int gpex_clock_prepare_runtime_off(void);

/*************************************
 * SETTERS
 ************************************/
int gpex_clock_set(int clk);
int gpex_clock_lock_clock(gpex_clock_lock_cmd_t lock_command, gpex_clock_lock_type_t lock_type,
			  int clock);
void gpex_clock_set_user_min_lock_input(int clock);

/*************************************
 * GETTERS
 ************************************/
int gpex_clock_get_clock_slow(void);
int gpex_clock_get_table_idx(int clock);
int gpex_clock_get_table_idx_cur(void);
int gpex_clock_get_boot_clock(void);
int gpex_clock_get_max_clock(void);
int gpex_clock_get_max_clock_limit(void);
int gpex_clock_get_min_clock(void);
int gpex_clock_get_cur_clock(void);
int gpex_clock_get_min_lock(void);
int gpex_clock_get_max_lock(void);

int gpex_clock_get_voltage(int clk);
u64 gpex_clock_get_time(int level);
u64 gpex_clock_get_time_busy(int level);
int gpex_clock_get_clock(int level);
int gpex_get_valid_gpu_clock(int clock, bool is_round_up);
/**************************************
 * MUTEX
 *************************************/
void gpex_clock_mutex_lock(void);
void gpex_clock_mutex_unlock(void);

#endif /* _MALI_EXYNOS_CLOCK_H_ */
