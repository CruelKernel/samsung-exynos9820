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

#ifndef _MALI_EXYNOS_CLOCK_INTERNAL_H_
#define _MALI_EXYNOS_CLOCK_INTERNAL_H_

#include <gpex_utils.h>
#include <gpex_clock.h>

typedef struct _gpu_clock_info {
	unsigned int clock;
	unsigned int voltage;
	u64 time;
	u64 time_busy;
} gpu_clock_info;

struct _clock_info {
	struct kbase_device *kbdev;
	int gpu_max_clock;
	int gpu_min_clock;
	int gpu_max_clock_limit;
	int boot_clock; // was known as gpu_dvfs_start_clock
	int cur_clock;
	gpu_clock_info *table;
	int table_size;

	struct mutex clock_lock;

	int max_lock;
	int min_lock;
	int user_max_lock[NUMBER_LOCK];
	int user_min_lock[NUMBER_LOCK];
	int user_max_lock_input;
	int user_min_lock_input;
};

int gpex_clock_sysfs_init(struct _clock_info *_clk_info);

int gpex_clock_update_time_in_state(int clock);
int gpex_clock_init_time_in_state(void);

#endif /* _MALI_EXYNOS_CLOCK_INTERNAL_H_ */
