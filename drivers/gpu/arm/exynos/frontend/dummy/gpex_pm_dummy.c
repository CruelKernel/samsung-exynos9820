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

#include <gpex_pm.h>
#include <gpex_dvfs.h>

int gpex_pm_init(void)
{
	return 0;
}

void gpex_pm_term(void)
{
	return;
}

int gpex_pm_get_status(bool clock_lock)
{
	return 1;
}

int gpex_pm_get_power_status(void)
{
	return 1;
}

void gpex_pm_lock(void)
{
	return;
}

void gpex_pm_unlock(void)
{
	return;
}

int gpex_pm_set_state(int state)
{
	return 0;
}

int gpex_pm_get_state(int *state)
{
	return 0;
}

/***************************
 * RTPM helper functions
 **************************/
int gpex_pm_power_on(struct device *dev)
{
	gpex_dvfs_start();

	return 0;
}

void gpex_pm_power_autosuspend(struct device *dev)
{
	return;
}

void gpex_pm_suspend(struct device *dev)
{
	return;
}

int gpex_pm_runtime_init(struct device *dev)
{
	return 0;
}

void gpex_pm_runtime_term(struct device *dev)
{
	return;
}

void gpex_pm_runtime_off_prepare(struct device *dev)
{
	gpex_dvfs_stop();

	return;
}

int gpex_pm_runtime_on_prepare(struct device *dev)
{
	return 0;
}
