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

#ifndef _MALI_EXYNOS_PM_H_
#define _MALI_EXYNOS_PM_H_

#include <linux/device.h>

enum {
	GPEX_PM_STATE_START = 0,
	GPEX_PM_STATE_RESUME_BEGIN,
	GPEX_PM_STATE_RESUME_END,
	GPEX_PM_STATE_END
};

int gpex_pm_init(void);
void gpex_pm_term(void);

/**
 * gpex_pm_get_status() - Get GPU power status by reading from PMU
 *
 * @clock_lock: set to TRUE if you want this function to lock the clock mutex.
 *              FALSE if the clock mutex is already locked.
 *
 * Return: 1 if power is ON, 0 if power is OFF
 */
int gpex_pm_get_status(bool clock_lock);

/**
 * gpex_pm_get_power_status() - Get GPU power status by reading an internally tracked flag
 *
 * The status flag read by this method is kept track by gpex_pm module itself.
 *
 * Return: 1 if power is ON, 0 if power is OFF
 */
int gpex_pm_get_power_status(void);

/**
 * gpex_pm_lock() - must call this function before changing clock
 */
void gpex_pm_lock(void);
/**
 * gpex_pm_unlock() - must call this function after changing clock
 */
void gpex_pm_unlock(void);

/**
 * gpex_pm_set_state() - Set current GPU Exynos PM state
 *
 * @state: The state want to be set
 *
 * Return: 0 if successful, -1 if unsuccessful
 */
int gpex_pm_set_state(int state);

/**
 * gpex_pm_get_state() - Get current GPU Exynos PM state
 *
 * @state: The current state from gpex_pm
 *
 * Return: 0 if successful, -1 if unsuccessful
 */
int gpex_pm_get_state(int *state);

/***************************
 * RTPM helper functions
 **************************/
int gpex_pm_power_on(struct device *dev);
void gpex_pm_power_autosuspend(struct device *dev);
void gpex_pm_suspend(struct device *dev);
int gpex_pm_runtime_init(struct device *dev);
void gpex_pm_runtime_term(struct device *dev);
void gpex_pm_runtime_off_prepare(struct device *dev);
int gpex_pm_runtime_on_prepare(struct device *dev);

#endif /* _MALI_EXYNOS_PM_H_ */
