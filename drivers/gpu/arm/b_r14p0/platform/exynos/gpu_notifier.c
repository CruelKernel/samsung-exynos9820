/* drivers/gpu/arm/.../platform/gpu_notifier.c
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series platform-dependent codes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_notifier.c
 */

#include <mali_kbase.h>

#include <linux/suspend.h>
#include <linux/pm_runtime.h>

#include "mali_kbase_platform.h"
#include "gpu_dvfs_handler.h"
#include "gpu_notifier.h"
#include "gpu_control.h"

#ifdef CONFIG_EXYNOS_THERMAL
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 17, 0)
#include <mach/tmu.h>
#else
#include <soc/samsung/tmu.h>
#endif
#endif /* CONFIG_EXYNOS_THERMAL */

#ifdef CONFIG_EXYNOS_BUSMONITOR
#include <linux/exynos-busmon.h>
#endif

#include <linux/oom.h>

extern struct kbase_device *pkbdev;

#if defined (CONFIG_EXYNOS_THERMAL) && defined(CONFIG_GPU_THERMAL)
static void gpu_tmu_normal_work(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_DVFS
	struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;
	if (!platform)
		return;

	gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, TMU_LOCK, 0);
#endif /* CONFIG_MALI_DVFS */
}

static int gpu_tmu_notifier(struct notifier_block *notifier,
				unsigned long event, void *v)
{
	int frequency;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;
#ifdef CONFIG_DEBUG_SNAPSHOT_THERMAL
	char *cooling_device_name = "GPU";
#endif

	if (!platform)
		return -ENODEV;

	if (!platform->tmu_status)
		return NOTIFY_OK;

	platform->voltage_margin = 0;
	frequency = *(int *)v;

	if (event == GPU_COLD) {
		platform->voltage_margin = platform->gpu_default_vol_margin;
	} else if (event == GPU_NORMAL) {
		gpu_tmu_normal_work(pkbdev);
	} else if (event == GPU_THROTTLING || event == GPU_TRIPPING) {
#ifdef CONFIG_MALI_DVFS
		gpu_dvfs_clock_lock(GPU_DVFS_MAX_LOCK, TMU_LOCK, frequency);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_THERMAL
		dbg_snapshot_thermal(NULL, 0, cooling_device_name, frequency);
#endif
	}

	GPU_LOG(DVFS_DEBUG, LSI_TMU_VALUE, 0u, event, "tmu event %lu, frequency %d\n", event, frequency);

	gpu_set_target_clk_vol(platform->cur_clock, false);

	return NOTIFY_OK;
}

static struct notifier_block gpu_tmu_nb = {
	.notifier_call = gpu_tmu_notifier,
};
#endif /* CONFIG_EXYNOS_THERMAL */


static int gpu_power_on(struct kbase_device *kbdev)
{
	int ret;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return -ENODEV;

	GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "power on\n");

#ifdef CONFIG_MALI_RT_PM
	if (!platform->inter_frame_pm_status)
		gpu_control_disable_customization(kbdev);

	ret = pm_runtime_get_sync(kbdev->dev);

	if (platform->inter_frame_pm_status)
		gpu_control_disable_customization(kbdev);
#else
	ret = 0;
#endif

	if (ret > 0) {
#ifdef CONFIG_MALI_DVFS
		if (platform->early_clk_gating_status) {
			GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "already power on\n");
			gpu_control_enable_clock(kbdev);
		}
#endif
		return 0;
	} else if (ret == 0) {
		return 1;
	} else {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "runtime pm returned %d\n", ret);
		return 0;
	}
}

static void gpu_power_off(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return;

	GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "power off\n");
#ifdef CONFIG_MALI_RT_PM
	gpu_control_enable_customization(kbdev);

	pm_runtime_mark_last_busy(kbdev->dev);
	pm_runtime_put_autosuspend(kbdev->dev);

#ifdef CONFIG_MALI_DVFS
	if (platform->early_clk_gating_status)
		gpu_control_disable_clock(kbdev);
#endif
#endif
}

static void gpu_power_suspend(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	int ret = 0;

	if (!platform)
		return;

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "power suspend\n");
	if (platform->dvs_status)
		gpu_control_enable_customization(kbdev);

	ret = pm_runtime_suspend(kbdev->dev);

#ifdef CONFIG_MALI_DVFS
	if (platform->early_clk_gating_status)
		gpu_control_disable_clock(kbdev);
#endif
}

#ifdef CONFIG_MALI_RT_PM

static int gpu_pm_notifier(struct notifier_block *nb, unsigned long event, void *cmd)
{
	int err = NOTIFY_OK;

	switch (event) {
	case PM_SUSPEND_PREPARE:
		GPU_LOG(DVFS_DEBUG, LSI_SUSPEND, 0u, 0u, "%s: suspend prepare event\n", __func__);
		break;
	case PM_POST_SUSPEND:
		GPU_LOG(DVFS_DEBUG, LSI_RESUME, 0u, 0u, "%s: resume event\n", __func__);
		break;
	default:
		break;
	}
	return err;
}

static struct notifier_block gpu_pm_nb = {
	.notifier_call = gpu_pm_notifier
};

static int gpu_device_runtime_init(struct kbase_device *kbdev)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	if (!platform) {
		dev_warn(kbdev->dev, "kbase_device_runtime_init failed %p\n", platform);
		ret = -ENOSYS;
		return ret;
	}

	dev_dbg(kbdev->dev, "kbase_device_runtime_init\n");

	pm_runtime_set_autosuspend_delay(kbdev->dev, platform->runtime_pm_delay_time);
	pm_runtime_use_autosuspend(kbdev->dev);

	pm_runtime_set_active(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	if (!pm_runtime_enabled(kbdev->dev)) {
		dev_warn(kbdev->dev, "pm_runtime not enabled");
		ret = -ENOSYS;
	}

	return ret;
}

static void gpu_device_runtime_disable(struct kbase_device *kbdev)
{
	pm_runtime_disable(kbdev->dev);
}

#if MALI_SEC_PROBE_TEST != 1
static int pm_callback_dvfs_on(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_DVFS
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	gpu_dvfs_timer_control(true);

	if (platform->dvfs_pending)
		platform->dvfs_pending = 0;
#endif

	return 0;
}
#endif

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return -ENODEV;

	GPU_LOG(DVFS_DEBUG, LSI_GPU_ON, 0u, 0u, "runtime on callback\n");

#ifdef CONFIG_MALI_DVFS
	gpu_control_enable_clock(kbdev);
#endif
	gpu_dvfs_start_env_data_gathering(kbdev);
	platform->power_status = true;
#ifdef CONFIG_MALI_DVFS
#ifdef CONFIG_MALI_SEC_CL_BOOST
	if (platform->dvfs_status && platform->wakeup_lock && !kbdev->pm.backend.metrics.is_full_compute_util)
#else
	if (platform->dvfs_status && platform->wakeup_lock)
#endif /* CONFIG_MALI_SEC_CL_BOOST */
		gpu_set_target_clk_vol(platform->gpu_dvfs_start_clock, false);
	else
		gpu_set_target_clk_vol(platform->cur_clock, false);
#endif /* CONFIG_MALI_DVFS */

	return 0;
}
extern void preload_balance_setup(struct kbase_device *kbdev);
static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return;

	GPU_LOG(DVFS_DEBUG, LSI_GPU_OFF, 0u, 0u, "runtime off callback\n");

	platform->power_status = false;

	gpu_control_disable_customization(kbdev);

	gpu_dvfs_stop_env_data_gathering(kbdev);
#ifdef CONFIG_MALI_DVFS
	gpu_dvfs_timer_control(false);
	if (platform->dvfs_pending)
		platform->dvfs_pending = 0;
	if (!platform->early_clk_gating_status)
		gpu_control_disable_clock(kbdev);
#endif /* CONFIG_MALI_DVFS */

#if defined(CONFIG_SOC_EXYNOS7420) || defined(CONFIG_SOC_EXYNOS7890)
	preload_balance_setup(kbdev);
#endif
}
#endif /* CONFIG_MALI_RT_PM */

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = gpu_power_on,
	.power_off_callback = gpu_power_off,
	.power_suspend_callback = gpu_power_suspend,
#ifdef CONFIG_MALI_RT_PM
	.power_runtime_init_callback = gpu_device_runtime_init,
	.power_runtime_term_callback = gpu_device_runtime_disable,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_off_callback = pm_callback_runtime_off,
#if MALI_SEC_PROBE_TEST != 1
	.power_dvfs_on_callback = pm_callback_dvfs_on,
#endif
#else /* CONFIG_MALI_RT_PM */
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_off_callback = NULL,
#if MALI_SEC_PROBE_TEST != 1
	.power_dvfs_on_callback = NULL,
#endif
#endif /* CONFIG_MALI_RT_PM */
};

#ifdef CONFIG_EXYNOS_BUSMONITOR
static int gpu_noc_notifier(struct notifier_block *nb, unsigned long event, void *cmd)
{
	if (strstr((char *)cmd, "G3D")) {
		GPU_LOG(DVFS_ERROR, LSI_RESUME, 0u, 0u, "%s: gpu_noc_notifier\n", __func__);
		gpu_register_dump();
	}
	return 0;
}
#endif

#ifdef CONFIG_EXYNOS_BUSMONITOR
static struct notifier_block gpu_noc_nb = {
	.notifier_call = gpu_noc_notifier
};
#endif

/* temporarily comment out by chandolp */
#if 0
static int gpu_oomdebug_notifier(struct notifier_block *self,
						       unsigned long dummy, void *parm)
{
	struct list_head *entry;
	const struct list_head *kbdev_list;

	kbdev_list = kbase_dev_list_get();
	list_for_each(entry, kbdev_list) {
		struct kbase_device *kbdev = NULL;
		struct kbasep_kctx_list_element *element;

		kbdev = list_entry(entry, struct kbase_device, entry);
		/* output the total memory usage and cap for this device */
		pr_info("%-16s  %10u\n",
				kbdev->devname,
				atomic_read(&(kbdev->memdev.used_pages)));
		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry(element, &kbdev->kctx_list, link) {
			/* output the memory usage and cap for each kctx
			   54             * opened on this device */
			pr_info("  %s-0x%p %10u\n",
					"kctx",
					element->kctx,
					atomic_read(&(element->kctx->used_pages)));
		}
		mutex_unlock(&kbdev->kctx_list_lock);
	}
	kbase_dev_list_put(kbdev_list);
	return NOTIFY_OK;
}

static struct notifier_block gpu_oomdebug_nb = {
	.notifier_call = gpu_oomdebug_notifier,
};
#endif

int gpu_notifier_init(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;
	if (!platform)
		return -ENODEV;

	platform->voltage_margin = platform->gpu_default_vol_margin;
#if defined (CONFIG_EXYNOS_THERMAL) && defined(CONFIG_GPU_THERMAL)
	exynos_gpu_add_notifier(&gpu_tmu_nb);
#endif /* CONFIG_EXYNOS_THERMAL */

#ifdef CONFIG_MALI_RT_PM
	if (register_pm_notifier(&gpu_pm_nb))
		return -1;
#endif /* CONFIG_MALI_RT_PM */

#ifdef CONFIG_EXYNOS_BUSMONITOR
	busmon_notifier_chain_register(&gpu_noc_nb);
#endif

	platform->power_status = true;

	/* temporarily comment out by chandolp */
	/* Cannot find following API in 4.14 kernel */
	/* if (register_oomdebug_notifier(&gpu_oomdebug_nb) < 0)
		pr_err("%s: failed to register oom debug notifier\n", __func__); */

	return 0;
}

void gpu_notifier_term(void)
{
#ifdef CONFIG_MALI_RT_PM
	unregister_pm_notifier(&gpu_pm_nb);
#endif /* CONFIG_MALI_RT_PM */
	return;
}
