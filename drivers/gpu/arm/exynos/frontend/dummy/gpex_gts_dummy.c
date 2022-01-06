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

#include <linux/module.h>
#include <gpex_gts.h>

int gpex_gts_get_ioctl_gts_info(struct mali_exynos_ioctl_gts_info *info)
{
	return 0;
}
void gpex_gts_update_jobslot_util(bool gpu_active, u32 ns_time)
{
	return;
}

void gpex_gts_set_jobslot_status(bool is_active)
{
	return;
}

void gpex_gts_clear(void)
{
	return;
}

void gpex_gts_set_hcm_mode(int hcm_mode_val)
{
	return;
}

int gpex_gts_get_hcm_mode(void)
{
	return 0;
}

int gpex_gts_init(struct device **dev)
{
	return 0;
}

void gpex_gts_term(void)
{
	return;
}

bool gpex_gts_update_gpu_data(void)
{
	return true;
}

void gpu_register_out_data(void (*fn)(u64 *cnt))
{
	return;
}
EXPORT_SYMBOL_GPL(gpu_register_out_data);
