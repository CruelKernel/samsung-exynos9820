/* drivers/gpu/arm/.../platform/gpu_control.c
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
 * @file gpu_control.c
 * DVFS
 */

#include <mali_kbase.h>

#include <linux/of_device.h>
#include <linux/pm_qos.h>
#include <linux/pm_domain.h>
#include <linux/clk.h>

#include "mali_kbase_platform.h"
#include "gpu_dvfs_handler.h"
#include "gpu_control.h"

#ifdef CONFIG_EXYNOS_PD
#include <soc/samsung/exynos-pd.h>
#endif
#ifdef CONFIG_EXYNOS_PMU
#include <soc/samsung/exynos-pmu.h>
#endif
#ifdef CONFIG_CAL_IF
#include <soc/samsung/cal-if.h>
#endif
#ifdef CONFIG_OF
#include <linux/of.h>
#endif

extern struct regulator *g3d_m_regulator;
unsigned int gpu_pmu_status_reg_offset;
unsigned int gpu_pmu_status_local_pwr_mask;
#define EXYNOS_PMU_G3D_STATUS	gpu_pmu_status_reg_offset
#define LOCAL_PWR_CFG			gpu_pmu_status_local_pwr_mask

#ifdef CONFIG_MALI_RT_PM
static struct exynos_pm_domain *gpu_get_pm_domain(char *g3d_genpd_name)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;
	struct exynos_pm_domain *pd_temp, *pd = NULL;

	for_each_compatible_node(np, NULL, "samsung,exynos-pd") {
		if (!of_device_is_available(np))
			continue;

		pdev = of_find_device_by_node(np);
		pd_temp = (struct exynos_pm_domain *)platform_get_drvdata(pdev);
		if (!strcmp(g3d_genpd_name, (const char *)(pd_temp->genpd.name))) {
			pd = pd_temp;
			break;
		}
	}

	if(pd == NULL)
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: g3d pm_domain is null\n", __func__);

	return pd;
}
#endif /* CONFIG_MALI_RT_PM */

int gpu_register_dump(void)
{
	return 0;
}

int gpu_is_power_on(void)
{
	unsigned int val = 0;

#ifdef CONFIG_EXYNOS_PMU
	exynos_pmu_read(EXYNOS_PMU_G3D_STATUS, &val);
#else
	val = 0xf;
#endif
	return ((val & LOCAL_PWR_CFG) == LOCAL_PWR_CFG) ? 1 : 0;
}

int gpu_control_is_power_on(struct kbase_device *kbdev)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	mutex_lock(&platform->gpu_clock_lock);
	ret = gpu_is_power_on();
	mutex_unlock(&platform->gpu_clock_lock);

	return ret;
}

int gpu_get_cur_clock(struct exynos_context *platform)
{
	if (!platform)
		return -ENODEV;
#ifdef CONFIG_CAL_IF
	return cal_dfs_get_rate(platform->g3d_cmu_cal_id);
#else
	return 0;
#endif
}

#ifdef CONFIG_MALI_DVFS
static int gpu_set_dvfs_using_calapi(struct exynos_context *platform, int clk)
{
	int ret = 0;

#ifdef CONFIG_MALI_RT_PM
	if (platform->exynos_pm_domain)
		mutex_lock(&platform->exynos_pm_domain->access_lock);

	if (!gpu_is_power_on()) {
		ret = -1;
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "%s: can't set clock in the power-off state!\n", __func__);
		goto err;
	}
#endif /* CONFIG_MALI_RT_PM */

	if (clk == platform->cur_clock) {
		ret = 0;
		GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "%s: skipped to set clock for %dMhz!\n",
				__func__, platform->cur_clock);

#ifdef CONFIG_MALI_RT_PM
		if (platform->exynos_pm_domain)
			mutex_unlock(&platform->exynos_pm_domain->access_lock);
#endif
		return ret;
	}

	cal_dfs_set_rate(platform->g3d_cmu_cal_id, clk);

	platform->cur_clock = cal_dfs_get_rate(platform->g3d_cmu_cal_id);

	GPU_LOG(DVFS_DEBUG, LSI_CLOCK_VALUE, clk, platform->cur_clock,
		"[id: %x] clock set: %d, clock get: %d\n",
		platform->g3d_cmu_cal_id, clk, platform->cur_clock);

#ifdef CONFIG_MALI_RT_PM
err:
	if (platform->exynos_pm_domain)
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
#endif /* CONFIG_MALI_RT_PM */
	return ret;
}

int gpu_control_set_dvfs(struct kbase_device *kbdev, int clock)
{
	int ret = 0;
	bool is_up = false;
	static int prev_clock = -1;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	if (platform->dvs_is_enabled || (platform->inter_frame_pm_status && !platform->inter_frame_pm_is_poweron)) {
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u,
			"%s: can't set clock in the dvs mode (requested clock %d)\n", __func__, clock);
		return 0;
	}
#ifdef CONFIG_MALI_DVFS
	if (gpu_dvfs_get_level(clock) < 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: mismatch clock error (%d)\n", __func__, clock);
		return -1;
	}
#endif

	is_up = prev_clock < clock;

#ifdef CONFIG_MALI_PM_QOS
	if (is_up)
		gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_SET);
#endif /* CONFIG_MALI_PM_QOS */

	if (platform->g3d_cmu_cal_id)
		gpu_set_dvfs_using_calapi(platform, clock);

#ifdef CONFIG_MALI_PM_QOS
	if (!is_up)	/* is_down */
		gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_SET);
#endif /* CONFIG_MALI_PM_QOS */

	gpu_dvfs_update_time_in_state(prev_clock);
	prev_clock = clock;

	return ret;
}

int gpu_control_set_clock(struct kbase_device *kbdev, int clock)
{
	int ret = 0;
	bool is_up = false;
	static int prev_clock = -1;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	if (platform->dvs_is_enabled || (platform->inter_frame_pm_status && !platform->inter_frame_pm_is_poweron)) {
		GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u,
			"%s: can't set clock in the dvs mode (requested clock %d)\n", __func__, clock);
		return 0;
	}
#ifdef CONFIG_MALI_DVFS
	if (gpu_dvfs_get_level(clock) < 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: mismatch clock error (%d)\n", __func__, clock);
		return -1;
	}
#endif

	is_up = prev_clock < clock;

#ifdef CONFIG_MALI_PM_QOS
	if (is_up)
		gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_SET);
#endif /* CONFIG_MALI_PM_QOS */

#ifdef CONFIG_MALI_PM_QOS
	if (is_up && ret)
		gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_SET);
	else if (!is_up && !ret)
		gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_SET);
#endif /* CONFIG_MALI_PM_QOS */

	gpu_dvfs_update_time_in_state(prev_clock);
	prev_clock = clock;

	return ret;
}

int gpu_control_enable_clock(struct kbase_device *kbdev)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	gpu_dvfs_update_time_in_state(0);

	return ret;
}

int gpu_control_disable_clock(struct kbase_device *kbdev)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	gpu_dvfs_update_time_in_state(platform->cur_clock);
#ifdef CONFIG_MALI_PM_QOS
	gpu_pm_qos_command(platform, GPU_CONTROL_PM_QOS_RESET);
#endif /* CONFIG_MALI_PM_QOS */

	return ret;
}

#ifdef CONFIG_MALI_ASV_CALIBRATION_SUPPORT
int gpu_control_power_policy_set(struct kbase_device *kbdev, const char *buf)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	const struct kbase_pm_policy *const *policy_list;
	static const struct kbase_pm_policy *prev_policy;
	int policy_count;
	int i;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	prev_policy = kbase_pm_get_policy(kbdev);

	policy_count = kbase_pm_list_policies(&policy_list);

	GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: gpu dev_drv name = %s\n", __func__, kbdev->dev->driver->name);
	GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: gpu prev power policy = %s\n", __func__, prev_policy->name);
	GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: gpu power policy count= %d\n", __func__, policy_count);

	for (i = 0; i < policy_count; i++) {
		if (sysfs_streq(policy_list[i]->name, buf)) {
			kbase_pm_set_policy(kbdev, policy_list[i]);
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: gpu cur power policy = %s\n", __func__, policy_list[i]->name);
			break;
		}
	}

	return ret;
}
#endif


#endif

#ifdef CONFIG_REGULATOR
int gpu_enable_dvs(struct exynos_context *platform)
{
#ifdef CONFIG_MALI_RT_PM
	if (!platform->dvs_status)
		return 0;

	if (!gpu_is_power_on()) {
		GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "%s: can't set dvs in the power-off state!\n", __func__);
		return -1;
	}

#if defined(CONFIG_REGULATOR_S2MPS16)
	/* Do not need to enable dvs during suspending */
	if (!pkbdev->pm.suspending) {
		if (cal_dfs_ext_ctrl(dvfs_g3d, cal_dfs_dvs, 1) != 0) {
			GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to enable dvs\n", __func__);
			return -1;
		}
	}
#endif /* CONFIG_REGULATOR_S2MPS16 */

	GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "dvs is enabled (vol: %d)\n", gpu_get_cur_voltage(platform));
#endif
	return 0;
}

int gpu_disable_dvs(struct exynos_context *platform)
{
	if (!platform->dvs_status)
		return 0;

#ifdef CONFIG_MALI_RT_PM
#if defined(CONFIG_REGULATOR_S2MPS16)
	if (cal_dfs_ext_ctrl(dvfs_g3d, cal_dfs_dvs, 0) != 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to disable dvs\n", __func__);
		return -1;
	}
#endif /* CONFIG_REGULATOR_S2MPS16 */

	GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u, "dvs is disabled (vol: %d)\n", gpu_get_cur_voltage(platform));
#endif
	return 0;
}

int gpu_inter_frame_power_on(struct exynos_context *platform)
{
#ifdef CONFIG_MALI_RT_PM
	int status;

	if (!platform->inter_frame_pm_status)
		return 0;

	mutex_lock(&platform->exynos_pm_domain->access_lock);

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (status) {
		GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u,
				"%s: status checking : Already gpu inter frame power on\n",__func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return 0;
	}

	if (cal_pd_control(platform->exynos_pm_domain->cal_pdid, 1) != 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to gpu inter frame power on\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (!status) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: status error : gpu inter frame power on\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	mutex_unlock(&platform->exynos_pm_domain->access_lock);
	GPU_LOG(DVFS_DEBUG, LSI_IFPM_POWER_ON, 0u, 0u, "gpu inter frame power on\n");
#endif
	return 0;
}

int gpu_inter_frame_power_off(struct exynos_context *platform)
{
#ifdef CONFIG_MALI_RT_PM
	int status;

	if (!platform->inter_frame_pm_status)
		return 0;

	mutex_lock(&platform->exynos_pm_domain->access_lock);

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (!status) {
		GPU_LOG(DVFS_DEBUG, DUMMY, 0u, 0u,
				"%s: status checking: Already gpu inter frame power off\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return 0;
	}

	if (cal_pd_control(platform->exynos_pm_domain->cal_pdid, 0) != 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: failed to gpu inter frame power off\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	status = cal_pd_status(platform->exynos_pm_domain->cal_pdid);
	if (status) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: status error :  gpu inter frame power off\n", __func__);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
		return -1;
	}

	mutex_unlock(&platform->exynos_pm_domain->access_lock);
	GPU_LOG(DVFS_DEBUG, LSI_IFPM_POWER_OFF, 0u, 0u, "gpu inter frame power off\n");
#endif
	return 0;
}


int gpu_control_enable_customization(struct kbase_device *kbdev)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;
	if (!platform)
		return -ENODEV;

#ifdef CONFIG_REGULATOR
#if defined(CONFIG_SCHED_EMS) || defined(CONFIG_SCHED_EHMP)
	mutex_lock(&platform->gpu_sched_hmp_lock);

	if (platform->inter_frame_pm_feature == false)
		platform->inter_frame_pm_status = false;
	else if (platform->ctx_need_qos == true)
		platform->inter_frame_pm_status = false;
#ifdef CONFIG_MALI_SEC_CL_BOOST
	else if (kbdev->pm.backend.metrics.is_full_compute_util)
		platform->inter_frame_pm_status = false;
#endif
	else
		platform->inter_frame_pm_status = true;

	mutex_unlock(&platform->gpu_sched_hmp_lock);
#endif
	if (!platform->dvs_status && !platform->inter_frame_pm_status)
		return 0;

	mutex_lock(&platform->gpu_clock_lock);

	if (platform->dvs_status) {
		ret = gpu_enable_dvs(platform);
		platform->dvs_is_enabled = true;
	} else if (platform->inter_frame_pm_status) {
		/* inter frame power off */
		if (platform->gpu_set_pmu_duration_reg &&
				platform->gpu_set_pmu_duration_val)
			exynos_pmu_write(platform->gpu_set_pmu_duration_reg, platform->gpu_set_pmu_duration_val);
		gpu_inter_frame_power_off(platform);
		platform->inter_frame_pm_is_poweron = false;
	}
	mutex_unlock(&platform->gpu_clock_lock);
#endif /* CONFIG_REGULATOR */

	return ret;
}

int gpu_control_disable_customization(struct kbase_device *kbdev)
{
	int ret = 0;
	struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;
	if (!platform)
		return -ENODEV;

#ifdef CONFIG_REGULATOR
	if (!platform->dvs_status && !platform->inter_frame_pm_status)
		return 0;

	mutex_lock(&platform->gpu_clock_lock);
	if (platform->dvs_status) {
		ret = gpu_disable_dvs(platform);
		platform->dvs_is_enabled = false;
	} else if (platform->inter_frame_pm_status) {
		/* inter frame power on */
		gpu_inter_frame_power_on(platform);
		platform->inter_frame_pm_is_poweron = true;
	}

	mutex_unlock(&platform->gpu_clock_lock);
#endif /* CONFIG_REGULATOR */

	return ret;
}

#ifdef CONFIG_MALI_ASV_CALIBRATION_SUPPORT
struct workqueue_struct *gpu_asv_cali_wq;
struct delayed_work gpu_asv_cali_stop_work;

static void gpu_asv_calibration_stop_callback(struct work_struct *data)
{
	struct exynos_context *platform = (struct exynos_context *) pkbdev->platform_context;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return;
	}

	gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, ASV_CALI_LOCK, 0);
	gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, ASV_CALI_LOCK, 0);
	gpu_control_power_policy_set(pkbdev, "demand");
	platform->gpu_auto_cali_status = false;
}

int gpu_asv_calibration_start(void)
{
	struct exynos_context *platform = (struct exynos_context *) pkbdev->platform_context;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is null\n", __func__);
		return -ENODEV;
	}

	platform->gpu_auto_cali_status = true;
	gpu_control_power_policy_set(pkbdev, "always_on");
	gpu_dvfs_clock_lock(GPU_DVFS_MAX_LOCK, ASV_CALI_LOCK, platform->gpu_asv_cali_lock_val);
	gpu_dvfs_clock_lock(GPU_DVFS_MIN_LOCK, ASV_CALI_LOCK, platform->gpu_asv_cali_lock_val);

	if (gpu_asv_cali_wq == NULL) {
		INIT_DELAYED_WORK(&gpu_asv_cali_stop_work, gpu_asv_calibration_stop_callback);
		gpu_asv_cali_wq = create_workqueue("g3d_asv_cali");

		queue_delayed_work_on(0, gpu_asv_cali_wq,
				&gpu_asv_cali_stop_work, msecs_to_jiffies(15000));	/* 15 second */
}

	return 0;
}
#endif

#endif /* CONFIG_REGULATOR */

int gpu_get_cur_voltage(struct exynos_context *platform)
{
	return 0;
}
int *get_mif_table(int *size)
{
	return NULL;
}

int gpu_control_module_init(struct kbase_device *kbdev)
{
#ifdef CONFIG_OF
	struct device_node *np;
#endif
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;

	if (!platform)
		return -ENODEV;

#ifdef CONFIG_MALI_RT_PM
	platform->exynos_pm_domain = gpu_get_pm_domain(platform->g3d_genpd_name);
#endif /* CONFIG_MALI_RT_PM */

#ifdef CONFIG_OF
	np = kbdev->dev->of_node;
	if (np != NULL) {
		gpu_update_config_data_int(np, "gpu_pmu_status_reg_offset", &gpu_pmu_status_reg_offset);
		gpu_update_config_data_int(np, "gpu_pmu_status_local_pwr_mask", &gpu_pmu_status_local_pwr_mask);
	}
#endif

	return 0;
}

void gpu_control_module_term(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;
	if (!platform)
		return;

#ifdef CONFIG_MALI_RT_PM
	platform->exynos_pm_domain = NULL;
#endif /* CONFIG_MALI_RT_PM */
}
