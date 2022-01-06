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

#include <linux/device.h>
#include <gpex_dvfs.h>
#include "../gpex_dvfs_internal.h"

int gpex_dvfs_init(struct device **dev)
{
	return 0;
}

void gpex_dvfs_term(void)
{
	return;
}

int gpex_dvfs_set_clock_callback(void)
{
	return 0;
}

int gpex_dvfs_enable(void)
{
	return 0;
}

int gpex_dvfs_disable(void)
{
	return 0;
}

void gpex_dvfs_start(void)
{
	return;
}

void gpex_dvfs_stop(void)
{
	return;
}

int gpex_dvfs_set_polling_speed(int polling_speed)
{
	return 0;
}

int gpex_dvfs_get_status(void)
{
	return 0;
}

int gpex_dvfs_get_governor_type(void)
{
	return 0;
}

int gpex_dvfs_get_utilization(void)
{
	return 0;
}

int gpex_dvfs_get_polling_speed(void)
{
	return 0;
}

void gpex_dvfs_spin_lock(unsigned long *flags)
{
	return;
}

void gpex_dvfs_spin_unlock(unsigned long *flags)
{
	return;
}

// for external
int gpu_dvfs_get_clock(int level)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_clock);

int gpu_dvfs_get_voltage(int clock)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_voltage);

int gpu_dvfs_get_step(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_step);

int gpu_dvfs_get_cur_clock(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_cur_clock);

int gpu_dvfs_get_utilization(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_utilization);

int gpu_dvfs_get_min_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_min_freq);

int gpu_dvfs_get_max_freq(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_freq);

int gpu_dvfs_get_sustainable_info_array(int index)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_sustainable_info_array);

int gpu_dvfs_get_max_lock(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_lock);

bool gpu_dvfs_get_need_cpu_qos(void)
{
	return true;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_need_cpu_qos);

int gpex_dvfs_external_init(struct dvfs_info *_dvfs)
{
	return 0;
}
EXPORT_SYMBOL_GPL(gpex_dvfs_external_init);
