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

#ifndef _MALI_EXYNOS_IF_H_
#define _MALI_EXYNOS_IF_H_

#ifdef CONFIG_MALI_EXYNOS_DVFS
int gpu_dvfs_get_clock(int level);
int gpu_dvfs_get_voltage(int clock);
int gpu_dvfs_get_step(void);
int gpu_dvfs_get_cur_clock(void);
int gpu_dvfs_get_utilization(void);
int gpu_dvfs_get_max_freq(void);
int gpu_dvfs_get_sustainable_info_array(int index);
int gpu_dvfs_get_max_lock(void); // This is also declared in internal interface
bool gpu_dvfs_get_need_cpu_qos(void);
#else
static inline int gpu_dvfs_get_clock(int level)
{
	return 0;
}
static inline int gpu_dvfs_get_voltage(int clock)
{
	return 0;
}
static inline int gpu_dvfs_get_step(void)
{
	return 0;
}
static inline int gpu_dvfs_get_cur_clock(void)
{
	return 0;
}
static inline int gpu_dvfs_get_utilization(void)
{
	return 0;
}
static inline int gpu_dvfs_get_max_freq(void)
{
	return 0;
}
static inline int gpu_dvfs_get_sustainable_info_array(int index)
{
	return 0;
}
static inline int gpu_dvfs_get_max_lock(void)
{
	return 0;
}
static inline bool gpu_dvfs_get_need_cpu_qos(void)
{
	return false;
}
#endif

#endif /* _MALI_EXYNOS_DVFS_H_ */
