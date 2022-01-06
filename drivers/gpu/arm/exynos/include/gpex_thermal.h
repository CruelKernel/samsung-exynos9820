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

#ifndef _MALI_EXYNOS_THERMAL_H_
#define _MALI_EXYNOS_THERMAL_H_

#include <stdbool.h>

/**
 * gpex_thermal_init() - initializes mali exynos thermal module
 *
 * Return: 0 on success
 */
int gpex_thermal_init(void);

/**
 * gpex_thermal_term() - terminates mali exynos thermal module
 */
void gpex_thermal_term(void);

/**
 * gpex_thermal_gpu_normal() - invoked when gpu temp is back to normal
 *
 * Called by gpu thermal notifier function registered by gpexbe_notifier
 *
 * Return: 0 on success
 */
int gpex_thermal_gpu_normal(void);

/**
 * gpex_thermal_gpu_throttle() - invoked when gpu temp is too high and need to be throttled
 *
 * Called by gpu thermal notifier function registered by gpexbe_notifier
 *
 * Return: 0 on success
 */
int gpex_thermal_gpu_throttle(int frequency);

/**
 * gpex_thermal_set_status() - enables or disables gpu thermal throttling
 */
void gpex_thermal_set_status(bool status);

#endif /* _MALI_EXYNOS_THERMAL_H_ */
