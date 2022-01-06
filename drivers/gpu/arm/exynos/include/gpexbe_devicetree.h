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

#ifndef _GPEXBE_DEVICETREE_H_
#define _GPEXBE_DEVICETREE_H_

#include <linux/device.h>

#define gpexbe_devicetree_get_int(var)                                                             \
	gpexbe_devicetree_get_int_internal(offsetof(gpu_dt, var), #var)

#define gpexbe_devicetree_get_str(var)                                                             \
	gpexbe_devicetree_get_str_internal(offsetof(gpu_dt, var), #var)

typedef struct _dt_clock_item {
	int clock;
	int min_threshold;
	int max_threshold;
	int down_staycount;
	int mem_freq;
	int cpu_little_min_freq;
	int cpu_middle_min_freq;
	int cpu_big_max_freq;
	int llc_ways;
} dt_clock_item;

typedef struct _dt_clqos_item {
	int clock;
	int mif_min;
	int little_min;
	int middle_min;
	int big_max;
} dt_clqos_item;

typedef struct _dt_info {
	struct device *dev;
	int initialized;

	/* SOME EXCEPTIONS! clock table row and col num are stored as an array which is hard to access
	 * So I've made an exception and stored them as two u32 values instead of one array[2] */
	//u32 clk_table_row_cnt;
	//u32 clk_table_col_cnt;

	struct gpu_dvfs_table_size {
		u32 row;
		u32 col;
	} gpu_dvfs_table_size;

	dt_clock_item *gpu_dvfs_table;

	struct gpu_cl_pmqos_table_size {
		u32 row;
		u32 col;
	} gpu_cl_pmqos_table_size;

	dt_clqos_item *gpu_cl_pmqos_table;

	/* values from interactive_info field */
	struct interactive_info {
		u32 highspeed_clock;
		u32 highspeed_load;
		u32 highspeed_delay;
	} interactive_info;

	/* pm backend */
	const char *g3d_genpd_name;
	u32 gpu_pmu_status_reg_offset;
	u32 gpu_pmu_status_local_pwr_mask;

	/* clock backend */
	u32 g3d_cmu_cal_id;

	/* debug backend */
	u32 gpu_ess_id_type;

	/* PM */
	u32 gpu_runtime_pm_delay_time;

	/* IFPO */
	u32 gpu_inter_frame_pm;

	/* CLOCK */
	u32 gpu_max_clock;
	u32 gpu_min_clock;
	u32 gpu_pmqos_cpu_cluster_num;

	/* LEGACY DVFS */
	const char *governor;
	u32 gpu_dvfs_bl_config_clock;
	u32 gpu_dvfs_polling_time;

	/* QOS */
	u32 gpu_bts_support;
	u32 gpu_mo_min_clock;

	/* DEBUG */
	u32 gpu_debug_level;

	/* HCM */
	int gpu_heavy_compute_cpu0_min_clock;
	int gpu_heavy_compute_vk_cpu0_min_clock;

	/* TSG */
	uint32_t gpu_weight_table_idx_0;
	uint32_t gpu_weight_table_idx_1;
} gpu_dt;

int gpexbe_devicetree_get_int_internal(size_t offset, const char *str);
char *gpexbe_devicetree_get_str_internal(size_t offset, const char *str);
dt_clock_item *gpexbe_devicetree_get_clock_table(void);
dt_clqos_item *gpexbe_devicetree_get_clqos_table(void);
gpu_dt *gpexbe_devicetree_get_gpu_dt(void);
int gpexbe_devicetree_init(struct device *dev);
void gpexbe_devicetree_term(void);

#endif /* _GPEXBE_DEVICETREE_H_ */
