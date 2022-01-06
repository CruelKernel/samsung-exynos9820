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

#include <linux/version.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include <gpexbe_devicetree.h>
#include <gpex_utils.h>
#include <gpex_qos.h>
#include <gpex_clock.h>
#include <gpex_tsg.h>
#include <gpexbe_qos.h>
#include <gpexbe_bts.h>
#include <gpex_clboost.h>
#include <gpex_gts.h>
#include <gpexbe_llc_coherency.h>

#include <gpexwa_peak_notify.h>

#define HCM_MODE_A (1 << 0)
#define HCM_MODE_B (1 << 1)
#define HCM_MODE_C (1 << 2)

struct _qos_info {
	bool is_pm_qos_init;
	int mo_min_clock;
	unsigned int is_set_bts; // Check the pair of bts scenario.
	bool gpu_bts_support;
	spinlock_t bts_spinlock;

	int cpu_cluster_count; // is this worth keeping after init?

	int qos_table_size;
	int clqos_table_size;

	/* HCM Stuff */
	int gpu_heavy_compute_cpu0_min_clock;
	int gpu_heavy_compute_vk_cpu0_min_clock;
};

struct _qos_table {
	int gpu_clock;
	int mem_freq;
	int cpu_little_min_freq;
	int cpu_middle_min_freq;
	int cpu_big_max_freq;
	int llc_ways;
};

struct _clqos_table {
	int gpu_clock;
	int mif_min;
	int little_min;
	int middle_min;
	int big_max;
};

static struct _qos_info qos_info;
static struct _qos_table *qos_table;
static struct _clqos_table *clqos_table;

static int gpex_qos_get_table_idx(int clock)
{
	int idx;

	for (idx = 0; idx < qos_info.qos_table_size; idx++) {
		if (qos_table[idx].gpu_clock == clock)
			return idx;
	}

	return -EINVAL;
}

int gpex_qos_set(gpex_qos_flag flags, int val)
{
	if (!qos_info.is_pm_qos_init) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: PM QOS ERROR : pm_qos not initialized\n", __func__);
		return -ENOENT;
	}

	gpexbe_qos_request_update((mali_pmqos_flags)flags, val);

	/* TODO: record PMQOS state somewhere */

	return 0;
}

int gpex_qos_unset(gpex_qos_flag flags)
{
	if (!qos_info.is_pm_qos_init) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: PM QOS ERROR : pm_qos not initialized\n", __func__);
		return -ENOENT;
	}
	gpexbe_qos_request_unset((mali_pmqos_flags)flags);

	/* TODO: record PMQOS state somewhere */

	return 0;
}

int gpex_qos_init()
{
	int i = 0;
	gpu_dt *dt = gpexbe_devicetree_get_gpu_dt();

	/* TODO: check dependent backends are initializaed */

	spin_lock_init(&qos_info.bts_spinlock);

	qos_info.gpu_bts_support = (bool)gpexbe_devicetree_get_int(gpu_bts_support);
	qos_info.mo_min_clock = gpexbe_devicetree_get_int(gpu_mo_min_clock);

	qos_info.qos_table_size = dt->gpu_dvfs_table_size.row;
	qos_table = kcalloc(qos_info.qos_table_size, sizeof(*qos_table), GFP_KERNEL);

	for (i = 0; i < qos_info.qos_table_size; i++) {
		qos_table[i].gpu_clock = dt->gpu_dvfs_table[i].clock;
		qos_table[i].mem_freq = dt->gpu_dvfs_table[i].mem_freq;
		qos_table[i].cpu_little_min_freq = dt->gpu_dvfs_table[i].cpu_little_min_freq;
		qos_table[i].cpu_middle_min_freq = dt->gpu_dvfs_table[i].cpu_middle_min_freq;
		qos_table[i].cpu_big_max_freq = dt->gpu_dvfs_table[i].cpu_big_max_freq;
		qos_table[i].llc_ways = dt->gpu_dvfs_table[i].llc_ways;
	}

#if 0
	if (gpex_qos_get_table_idx(qos_info.mo_min_clock) < 0) {
		/* TODO: print error msg */
		BUG();
		return -1;
	}
#endif

	qos_info.clqos_table_size = dt->gpu_cl_pmqos_table_size.row;
	clqos_table = kcalloc(qos_info.clqos_table_size, sizeof(*clqos_table), GFP_KERNEL);

	for (i = 0; i < qos_info.clqos_table_size; i++) {
		clqos_table[i].gpu_clock = dt->gpu_cl_pmqos_table[i].clock;
		clqos_table[i].mif_min = dt->gpu_cl_pmqos_table[i].mif_min;
		clqos_table[i].little_min = dt->gpu_cl_pmqos_table[i].little_min;
		clqos_table[i].middle_min = dt->gpu_cl_pmqos_table[i].middle_min;
		clqos_table[i].big_max = dt->gpu_cl_pmqos_table[i].big_max;
	}

	qos_info.gpu_heavy_compute_cpu0_min_clock =
		gpexbe_devicetree_get_int(gpu_heavy_compute_cpu0_min_clock);
	qos_info.gpu_heavy_compute_vk_cpu0_min_clock =
		gpexbe_devicetree_get_int(gpu_heavy_compute_vk_cpu0_min_clock);

	/* Request to set QOS of other IPs */
	gpexbe_qos_request_add(PMQOS_MIF | PMQOS_LITTLE | PMQOS_MIDDLE | PMQOS_BIG | PMQOS_MIN |
			       PMQOS_MAX);

	qos_info.is_pm_qos_init = true;

	return 0;
}

void gpex_qos_term()
{
	gpexbe_qos_request_remove(PMQOS_MIF | PMQOS_LITTLE | PMQOS_MIDDLE | PMQOS_BIG | PMQOS_MIN |
				  PMQOS_MAX);
	kfree(qos_table);
	kfree(clqos_table);
	qos_info.is_pm_qos_init = false;
}

int gpex_qos_set_bts_mo(int clock)
{
	int ret = 0;
	unsigned long flags;

	if (!qos_info.gpu_bts_support) {
		if (qos_info.is_set_bts) {
			/* TODO: print error */
			return -1;
		}
		return 0;
	}

	spin_lock_irqsave(&qos_info.bts_spinlock, flags);

	if (clock >= qos_info.mo_min_clock && !qos_info.is_set_bts) {
		ret = gpexbe_bts_set_bts_mo(1);
		if (ret) {
			/* TODO: print error */
		} else
			qos_info.is_set_bts = 1;
	} else if ((clock == 0 || clock < qos_info.mo_min_clock) && qos_info.is_set_bts) {
		ret = gpexbe_bts_set_bts_mo(0);
		if (ret) {
			/* TODO: print error */
		} else
			qos_info.is_set_bts = 0;
	}

	spin_unlock_irqrestore(&qos_info.bts_spinlock, flags);

	return ret;
}

int gpex_qos_set_from_clock(int gpu_clock)
{
	int idx = 0;

	if (gpu_clock == 0) {
		gpex_qos_unset(QOS_MIF | QOS_LITTLE | QOS_MIDDLE | QOS_MIN);
		gpex_qos_set_bts_mo(gpu_clock);
		gpexbe_llc_coherency_set_ways(0);

		return 0;
	}

	idx = gpex_qos_get_table_idx(gpu_clock);

	if (idx < 0) {
		/* TODO: print error msg */
		return -EINVAL;
	}

	if (gpex_clboost_check_activation_condition()) {
		gpex_qos_set(QOS_MIF | QOS_MIN, clqos_table[idx].mif_min);
		gpex_qos_set(QOS_LITTLE | QOS_MIN, clqos_table[idx].little_min);
		gpex_qos_set(QOS_MIDDLE | QOS_MIN, clqos_table[idx].middle_min);
		gpex_qos_set(QOS_BIG | QOS_MAX, INT_MAX);
		/* TODO: revamp the qos interface so default max min can be set without knowing the clock */
		//gpex_qos_set(QOS_BIG | QOS_MAX, BIG_MAX);
	} else {
		if (gpex_tsg_get_pmqos() == true) {
			gpex_qos_set(QOS_MIF | QOS_MIN, 0);
			gpex_qos_set(QOS_LITTLE | QOS_MIN, 0);
			gpex_qos_set(QOS_MIDDLE | QOS_MIN, 0);
			gpex_qos_set(QOS_BIG | QOS_MAX, INT_MAX);
		} else {
			gpex_qos_set(QOS_MIF | QOS_MIN, qos_table[idx].mem_freq);

			if (!(gpex_gts_get_hcm_mode() & (HCM_MODE_A | HCM_MODE_C))) {
				gpex_qos_set(QOS_LITTLE | QOS_MIN,
					     qos_table[idx].cpu_little_min_freq);
			} else if (gpex_gts_get_hcm_mode() & HCM_MODE_A) {
				gpex_qos_set(QOS_LITTLE | QOS_MIN,
					     qos_info.gpu_heavy_compute_cpu0_min_clock);
				//pr_info("HCM: mode A QOS");
			} else if (gpex_gts_get_hcm_mode() & HCM_MODE_C) {
				gpex_qos_set(QOS_LITTLE | QOS_MIN,
					     qos_info.gpu_heavy_compute_vk_cpu0_min_clock);
				//pr_info("HCM: mode B QOS");
			}

			gpex_qos_set(QOS_MIDDLE | QOS_MIN, qos_table[idx].cpu_middle_min_freq);
			gpex_qos_set(QOS_BIG | QOS_MAX, qos_table[idx].cpu_big_max_freq);
		}

		gpexwa_peak_notify_update();
	}

	gpex_qos_set_bts_mo(gpu_clock);
	gpexbe_llc_coherency_set_ways(qos_table[idx].llc_ways);

	return 0;
}
