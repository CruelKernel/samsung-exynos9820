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

#ifndef _GPU_DVFS_GOVERNOR_H_
#define _GPU_DVFS_GOVERNOR_H_

typedef enum {
	G3D_DVFS_GOVERNOR_DEFAULT = 0,
	G3D_DVFS_GOVERNOR_INTERACTIVE,
	G3D_DVFS_GOVERNOR_JOINT,
	G3D_DVFS_GOVERNOR_STATIC,
	G3D_DVFS_GOVERNOR_BOOSTER,
	G3D_DVFS_GOVERNOR_DYNAMIC,
	G3D_MAX_GOVERNOR_NUM,
} gpu_governor_type;

void gpu_dvfs_update_start_clk(int governor_type, int clk);
void *gpu_dvfs_get_governor_info(void);
int gpu_dvfs_decide_next_freq(int utilization);
int gpu_dvfs_governor_setting(int governor_type);
int gpu_dvfs_governor_setting_locked(int governor_type);
int gpu_weight_prediction_utilisation(int utilization);
#endif /* _GPU_DVFS_GOVERNOR_H_ */
