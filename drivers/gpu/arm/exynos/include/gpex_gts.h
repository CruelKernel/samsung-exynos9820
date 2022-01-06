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

#ifndef _GPEX_GTS_H_
#define _GPEX_GTS_H_

#include <linux/device.h>
#include <mali_exynos_ioctl.h>

#define HCM_MODE_A (1 << 0)
#define HCM_MODE_B (1 << 1)
#define HCM_MODE_C (1 << 2)

int gpex_gts_get_ioctl_gts_info(struct mali_exynos_ioctl_gts_info *info);
void gpex_gts_update_jobslot_util(bool gpu_active, u32 ns_time);
void gpex_gts_set_jobslot_status(bool is_active);
void gpex_gts_clear(void);
void gpex_gts_set_hcm_mode(int hcm_mode_val);
int gpex_gts_get_hcm_mode(void);
bool gpex_gts_update_gpu_data(void);
int gpex_gts_init(struct device **dev);
void gpex_gts_term(void);

#endif /* _GPEX_GTS_H_ */
