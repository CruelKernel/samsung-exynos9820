/* drivers/gpu/arm/.../platform/gpu_pmqos.c
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series DVFS driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_pmqos.c
 * DVFS
 */

#include <mali_kbase.h>

#include <linux/pm_qos.h>

#include "mali_kbase_platform.h"
#include "gpu_dvfs_handler.h"

struct pm_qos_request exynos5_g3d_mif_min_qos;
struct pm_qos_request exynos5_g3d_mif_max_qos;
struct pm_qos_request exynos5_g3d_cpu_cluster0_min_qos;
struct pm_qos_request exynos5_g3d_cpu_cluster1_max_qos;
struct pm_qos_request exynos5_g3d_cpu_cluster1_min_qos;
struct pm_qos_request exynos5_g3d_cpu_cluster2_max_qos;
struct pm_qos_request exynos5_g3d_cpu_cluster2_min_qos;

#ifdef CONFIG_MALI_SUSTAINABLE_OPT
struct pm_qos_request exynos5_g3d_cpu_cluster0_max_qos;
#endif

#ifdef CONFIG_MALI_PM_QOS
int gpu_pm_qos_command(struct exynos_context *platform, gpu_pmqos_state state)
{
	int idx;

	DVFS_ASSERT(platform);

#ifdef CONFIG_MALI_ASV_CALIBRATION_SUPPORT
	if (platform->gpu_auto_cali_status)
		return 0;
#endif

	if (!platform->devfreq_status)
		return 0;

	switch (state) {
	case GPU_CONTROL_PM_QOS_INIT:
		pm_qos_add_request(&exynos5_g3d_mif_min_qos, PM_QOS_BUS_THROUGHPUT, 0);
		if (platform->pmqos_mif_max_clock)
			pm_qos_add_request(&exynos5_g3d_mif_max_qos, PM_QOS_BUS_THROUGHPUT_MAX, PM_QOS_BUS_THROUGHPUT_MAX_DEFAULT_VALUE);
		pm_qos_add_request(&exynos5_g3d_cpu_cluster0_min_qos, PM_QOS_CLUSTER0_FREQ_MIN, 0);
		if (platform->gpu_pmqos_cpu_cluster_num == 3) {
			pm_qos_add_request(&exynos5_g3d_cpu_cluster1_min_qos, PM_QOS_CLUSTER1_FREQ_MIN, 0);
			pm_qos_add_request(&exynos5_g3d_cpu_cluster2_max_qos, PM_QOS_CLUSTER2_FREQ_MAX, PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE);
#ifdef CONFIG_MALI_SUSTAINABLE_OPT
			pm_qos_add_request(&exynos5_g3d_cpu_cluster0_max_qos, PM_QOS_CLUSTER0_FREQ_MAX, PM_QOS_CLUSTER0_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_add_request(&exynos5_g3d_cpu_cluster1_max_qos, PM_QOS_CLUSTER1_FREQ_MAX, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
#endif
			if (platform->boost_egl_min_lock)
				pm_qos_add_request(&exynos5_g3d_cpu_cluster2_min_qos, PM_QOS_CLUSTER2_FREQ_MIN, 0);
		} else if (platform->gpu_pmqos_cpu_cluster_num == 2) {
			pm_qos_add_request(&exynos5_g3d_cpu_cluster1_max_qos, PM_QOS_CLUSTER1_FREQ_MAX, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
			if (platform->boost_egl_min_lock)
				pm_qos_add_request(&exynos5_g3d_cpu_cluster1_min_qos, PM_QOS_CLUSTER1_FREQ_MIN, 0);
		}
		for (idx = 0; idx < platform->table_size; idx++)
			platform->save_cpu_max_freq[idx] = platform->table[idx].cpu_big_max_freq;
		platform->is_pm_qos_init = true;
		break;
	case GPU_CONTROL_PM_QOS_DEINIT:
		pm_qos_remove_request(&exynos5_g3d_mif_min_qos);
		if (platform->pmqos_mif_max_clock)
			pm_qos_remove_request(&exynos5_g3d_mif_max_qos);
		pm_qos_remove_request(&exynos5_g3d_cpu_cluster0_min_qos);
		if (platform->gpu_pmqos_cpu_cluster_num == 3) {
			pm_qos_remove_request(&exynos5_g3d_cpu_cluster1_min_qos);
			pm_qos_remove_request(&exynos5_g3d_cpu_cluster2_max_qos);
			if (platform->boost_egl_min_lock)
				pm_qos_remove_request(&exynos5_g3d_cpu_cluster2_min_qos);
		} else if (platform->gpu_pmqos_cpu_cluster_num == 2) {
			pm_qos_remove_request(&exynos5_g3d_cpu_cluster1_max_qos);
			if (platform->boost_egl_min_lock)
				pm_qos_remove_request(&exynos5_g3d_cpu_cluster1_min_qos);
		}
#ifdef CONFIG_MALI_SUSTAINABLE_OPT
		pm_qos_remove_request(&exynos5_g3d_cpu_cluster0_max_qos);
		pm_qos_remove_request(&exynos5_g3d_cpu_cluster1_max_qos);
#endif
		platform->is_pm_qos_init = false;
		break;
	case GPU_CONTROL_PM_QOS_SET:
		if (!platform->is_pm_qos_init) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: PM QOS ERROR : pm_qos deinit -> set\n", __func__);
			return -ENOENT;
		}
		KBASE_DEBUG_ASSERT(platform->step >= 0);
		pm_qos_update_request(&exynos5_g3d_mif_min_qos, platform->table[platform->step].mem_freq);
		if (platform->pmqos_mif_max_clock &&
				(platform->table[platform->step].clock >= platform->pmqos_mif_max_clock_base))
			pm_qos_update_request(&exynos5_g3d_mif_max_qos, platform->pmqos_mif_max_clock);
#ifdef CONFIG_MALI_VK_BOOST /* VK JOB Boost */
		mutex_lock(&platform->gpu_vk_boost_lock);
		if (platform->ctx_vk_need_qos && platform->max_lock == platform->gpu_vk_boost_max_clk_lock) {
			pm_qos_update_request(&exynos5_g3d_mif_min_qos, platform->gpu_vk_boost_mif_min_clk_lock);
		}
		mutex_unlock(&platform->gpu_vk_boost_lock);
#endif
		pm_qos_update_request(&exynos5_g3d_cpu_cluster0_min_qos, platform->table[platform->step].cpu_little_min_freq);
		if (platform->gpu_pmqos_cpu_cluster_num == 3) {
			pm_qos_update_request(&exynos5_g3d_cpu_cluster1_min_qos, platform->table[platform->step].cpu_middle_min_freq);
			if (!platform->boost_is_enabled)
				pm_qos_update_request(&exynos5_g3d_cpu_cluster2_max_qos, platform->table[platform->step].cpu_big_max_freq);
		} else if (platform->gpu_pmqos_cpu_cluster_num == 2) {
			if (!platform->boost_is_enabled)
				pm_qos_update_request(&exynos5_g3d_cpu_cluster1_max_qos, platform->table[platform->step].cpu_big_max_freq);
		}
#ifdef CONFIG_MALI_SUSTAINABLE_OPT
		if (((platform->cur_clock == platform->sustainable.info_array[0])
			|| (platform->max_lock == platform->sustainable.info_array[0]))
			&& platform->env_data.utilization > platform->sustainable.info_array[1]) {
			platform->sustainable.status = true;
			pm_qos_update_request(&exynos5_g3d_cpu_cluster0_max_qos, platform->sustainable.info_array[2]);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster1_max_qos, platform->sustainable.info_array[3]);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster2_max_qos, platform->sustainable.info_array[4]);
		} else {
			platform->sustainable.status = false;
			pm_qos_update_request(&exynos5_g3d_cpu_cluster0_max_qos, PM_QOS_CLUSTER0_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster1_max_qos, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster2_max_qos, platform->table[platform->step].cpu_big_max_freq);
		}
#endif
		break;
	case GPU_CONTROL_PM_QOS_RESET:
		if (!platform->is_pm_qos_init) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: PM QOS ERROR : pm_qos deinit -> reset\n", __func__);
			return -ENOENT;
		}
		pm_qos_update_request(&exynos5_g3d_mif_min_qos, 0);
		if (platform->pmqos_mif_max_clock)
			pm_qos_update_request(&exynos5_g3d_mif_max_qos, PM_QOS_BUS_THROUGHPUT_MAX_DEFAULT_VALUE);
		pm_qos_update_request(&exynos5_g3d_cpu_cluster0_min_qos, 0);
		if (platform->gpu_pmqos_cpu_cluster_num == 3) {
			pm_qos_update_request(&exynos5_g3d_cpu_cluster1_min_qos, 0);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster2_max_qos, PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE);
#ifdef CONFIG_MALI_SUSTAINABLE_OPT
			pm_qos_update_request(&exynos5_g3d_cpu_cluster0_max_qos, PM_QOS_CLUSTER0_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster1_max_qos, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
			pm_qos_update_request(&exynos5_g3d_cpu_cluster2_max_qos, PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE);
#endif
		} else if (platform->gpu_pmqos_cpu_cluster_num == 2) {
			pm_qos_update_request(&exynos5_g3d_cpu_cluster1_max_qos, PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE);
		}
		break;
	case GPU_CONTROL_PM_QOS_EGL_SET:
		if (!platform->is_pm_qos_init) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: PM QOS ERROR : pm_qos deinit -> egl_set\n", __func__);
			return -ENOENT;
		}
		if (platform->gpu_pmqos_cpu_cluster_num == 3) {
			pm_qos_update_request_timeout(&exynos5_g3d_cpu_cluster2_min_qos, platform->boost_egl_min_lock, 30000);
		} else if (platform->gpu_pmqos_cpu_cluster_num == 2) {
			pm_qos_update_request_timeout(&exynos5_g3d_cpu_cluster1_min_qos, platform->boost_egl_min_lock, 30000);
		}
		for (idx = 0; idx < platform->table_size; idx++) {
			if (platform->gpu_pmqos_cpu_cluster_num == 3) {
				platform->table[idx].cpu_big_max_freq = PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE;
			} else if (platform->gpu_pmqos_cpu_cluster_num == 2) {
				platform->table[idx].cpu_big_max_freq = PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE;
			}
		}
		break;
	case GPU_CONTROL_PM_QOS_EGL_RESET:
		if (!platform->is_pm_qos_init) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: PM QOS ERROR : pm_qos deinit -> egl_reset\n", __func__);
			return -ENOENT;
		}
		for (idx = 0; idx < platform->table_size; idx++)
			platform->table[idx].cpu_big_max_freq = platform->save_cpu_max_freq[idx];
		break;
	default:
		break;
	}

	return 0;
}
#endif
