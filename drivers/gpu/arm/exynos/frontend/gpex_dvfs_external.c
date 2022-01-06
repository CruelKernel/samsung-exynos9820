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

#include <gpex_dvfs.h>
#include <gpex_clock.h>
#include <gpex_pm.h>

#include <mali_exynos_if.h>

#include "gpex_dvfs_internal.h"
static struct dvfs_info *dvfs;

int gpu_dvfs_get_clock(int level)
{
	if ((level < 0) || (level >= dvfs->table_size))
		return -1;

	return dvfs->table[level].clock;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_clock);

int gpu_dvfs_get_voltage(int clock)
{
	return gpex_clock_get_voltage(clock);
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_voltage);

int gpu_dvfs_get_step(void)
{
	return dvfs->table_size;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_step);

int gpu_dvfs_get_cur_clock(void)
{
	int clock = 0;

	gpex_pm_lock();
	if (gpex_pm_get_status(false))
		clock = gpex_clock_get_clock_slow();
	gpex_pm_unlock();

	return clock;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_cur_clock);

int gpu_dvfs_get_utilization(void)
{
	int util = 0;

	if (gpex_pm_get_status(true) == 1)
		util = dvfs->env_data.utilization;

	return util;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_utilization);

int gpu_dvfs_get_min_freq(void)
{
	return gpex_clock_get_min_clock();
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_min_freq);

int gpu_dvfs_get_max_freq(void)
{
	return gpex_clock_get_max_clock();
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_freq);

/* TODO: make a stub version for when dvfs is disabled */
/* Needed by 9830 as SUSTAINABLE_OPT feature */
int gpu_dvfs_get_sustainable_info_array(int index)
{
	CSTD_UNUSED(index);
	return 0;
}

/* TODO: make a stub version for when dvfs is disabled */
/* Needed by 9830 by exynos_perf_gmc.c */
int gpu_dvfs_get_max_lock(void)
{
	return gpex_clock_get_max_lock();
}

/* Needed by 9830 by exynos_perf_gmc.c (NEGATIVE_BOOST) */
bool gpu_dvfs_get_need_cpu_qos(void)
{
	return false;
}

int gpex_dvfs_external_init(struct dvfs_info *_dvfs)
{
	dvfs = _dvfs;

	return 0;
}
