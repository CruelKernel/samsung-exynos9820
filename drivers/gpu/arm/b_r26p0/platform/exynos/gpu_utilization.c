/* drivers/gpu/arm/.../platform/gpu_utilization.c
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
 * @file gpu_utilization.c
 * DVFS
 */

#include <mali_kbase.h>

#include "mali_kbase_platform.h"
#include "gpu_control.h"
#include "gpu_dvfs_handler.h"
#include "gpu_ipa.h"

extern struct kbase_device *pkbdev;

/* MALI_SEC_INTEGRATION */
#ifdef CONFIG_MALI_DVFS
extern int gpu_pm_get_dvfs_utilisation(struct kbase_device *kbdev, int *, int *);
static void gpu_dvfs_update_utilization(struct kbase_device *kbdev)
{
	unsigned long flags;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

#if defined(CONFIG_MALI_DVFS) && defined(CONFIG_CPU_THERMAL_IPA)
	if (platform->time_tick < platform->gpu_dvfs_time_interval) {
		platform->time_tick++;
		platform->time_busy += kbdev->pm.backend.metrics.values.time_busy;
		platform->time_idle += kbdev->pm.backend.metrics.values.time_idle;
	} else {
		platform->time_busy = kbdev->pm.backend.metrics.values.time_busy;
		platform->time_idle = kbdev->pm.backend.metrics.values.time_idle;
		platform->time_tick = 0;
	}
#endif /* CONFIG_MALI_DVFS && CONFIG_CPU_THERMAL_IPA */

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);

	platform->env_data.utilization = gpu_pm_get_dvfs_utilisation(kbdev, 0, 0);

	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

#if defined(CONFIG_MALI_DVFS) && defined(CONFIG_CPU_THERMAL_IPA)
	gpu_ipa_dvfs_calc_norm_utilisation(kbdev);
#endif /* CONFIG_MALI_DVFS && CONFIG_CPU_THERMAL_IPA */
}
#endif /* CONFIG_MALI_DVFS */

int gpu_dvfs_start_env_data_gathering(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	return 0;
}

int gpu_dvfs_stop_env_data_gathering(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	return 0;
}

#ifdef CONFIG_MALI_DVFS
int gpu_dvfs_reset_env_data(struct kbase_device *kbdev)
{
	unsigned long flags;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);
	/* reset gpu utilization value */
	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	kbdev->pm.backend.metrics.values.time_idle = kbdev->pm.backend.metrics.values.time_idle + kbdev->pm.backend.metrics.values.time_busy;
	kbdev->pm.backend.metrics.values.time_busy = 0;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	return 0;
}

int gpu_dvfs_calculate_env_data(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	static int polling_period;

	DVFS_ASSERT(platform);

	gpu_dvfs_update_utilization(kbdev);

	polling_period -= platform->polling_speed;
	if (polling_period > 0)
		return 0;

	if (platform->dvs_is_enabled == true)
		return 0;

	return 0;
}
#endif

int gpu_dvfs_calculate_env_data_ppmu(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	return 0;
}

int gpu_dvfs_utilization_init(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "utilization module initialized\n");

	return 0;
}

int gpu_dvfs_utilization_deinit(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	DVFS_ASSERT(platform);

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "utilization module de-initialized\n");

	return 0;
}
