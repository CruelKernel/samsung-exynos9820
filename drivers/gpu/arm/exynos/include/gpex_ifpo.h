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

#ifndef _MALI_EXYNOS_IFPO_H_
#define _MALI_EXYNOS_IFPO_H_

/**
 * enum ifpo_mode - describes IFPO enable or disable state
 *
 * @IFPO_DISABLED
 * @IFPO_ENABLED
 */
typedef enum ifpo_mode {
	IFPO_DISABLED = 0,
	IFPO_ENABLED,
} ifpo_mode;

/**
 * enum ifpo_mode - describes GPU power up or down state as set by IFPO
 *
 * @IFPO_POWER_DOWN
 * @IFPO_POWER_UP
 */
typedef enum ifpo_state {
	IFPO_POWER_DOWN = 0,
	IFPO_POWER_UP,
} ifpo_status;

/**
 * gpex_ifpo_init() - Initialize IFPO module
 *
 * Initializes to enabled and powered-on state
 *
 * Return: 0 on success
 */
int gpex_ifpo_init(void);

/**
 * gpex_ifpo_init() - Terminate IFPO module
 */
void gpex_ifpo_term(void);

/**
 * gpex_ifpo_power_up() - power on the gpu from ifpo power down state
 *
 * Context: takes gpex_clock_lock (if clock module enabled)
 *
 * Return: 0 on success
 */
int gpex_ifpo_power_up(void);

/**
 * gpex_ifpo_power_down() - power off the gpu from ifpo power up state
 *
 * Context: takes gpex_clock_lock (if clock module enabled)
 *
 * Return: 0 on success
 */
int gpex_ifpo_power_down(void);

/**
 * gpex_ifpo_get_status() - get current GPU power state as controlled by IFPO
 *
 * Return: IFPO power state of enum type ifpo_status
 */
ifpo_status gpex_ifpo_get_status(void);
ifpo_mode gpex_ifpo_get_mode(void);

#endif /* _MALI_EXYNOS_IFPO_H_ */
