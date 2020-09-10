/* drivers/gpu/arm/.../platform/gpu_custom_interface.c
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
 * @file gpu_custom_interface.c
 * DVFS
 */

#include <mali_kbase.h>

#include <linux/fb.h>

#if defined(CONFIG_MALI_DVFS) && defined(CONFIG_EXYNOS_THERMAL) && defined(CONFIG_GPU_THERMAL)
#include "exynos_tmu.h"
#endif

#include "mali_kbase_platform.h"
#include "gpu_dvfs_handler.h"
#include "gpu_dvfs_governor.h"
#include "gpu_control.h"
#ifdef CONFIG_CPU_THERMAL_IPA
#include "gpu_ipa.h"
#endif /* CONFIG_CPU_THERMAL_IPA */
#include "gpu_custom_interface.h"

#ifdef CONFIG_MALI_RT_PM
#include <soc/samsung/exynos-pd.h>
#endif

extern struct kbase_device *pkbdev;

int gpu_pmqos_dvfs_min_lock(int level)
{
#ifdef CONFIG_MALI_DVFS
	int clock;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: platform context is not initialized\n", __func__);
		return -ENODEV;
	}

	clock = gpu_dvfs_get_clock(level);
	if (clock < 0)
		gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, PMQOS_LOCK, 0);
	else
		gpu_dvfs_clock_lock(GPU_DVFS_MIN_LOCK, PMQOS_LOCK, clock);
#endif /* CONFIG_MALI_DVFS */
	return 0;
}

static ssize_t show_clock(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int clock = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

#ifdef CONFIG_MALI_RT_PM
	if (platform->exynos_pm_domain) {
		mutex_lock(&platform->exynos_pm_domain->access_lock);
		if(!platform->dvs_is_enabled && gpu_is_power_on())
			clock = gpu_get_cur_clock(platform);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
	}
#else
	if (gpu_control_is_power_on(pkbdev) == 1) {
		mutex_lock(&platform->gpu_clock_lock);
		if (!platform->dvs_is_enabled)
			clock = gpu_get_cur_clock(platform);
		mutex_unlock(&platform->gpu_clock_lock);
	}
#endif

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_clock(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int clk = 0;
	int ret, i, policy_count;
	static bool cur_state;
	const struct kbase_pm_policy *const *policy_list;
	static const struct kbase_pm_policy *prev_policy;
	static bool prev_tmu_status = true;
#ifdef CONFIG_MALI_DVFS
	static bool prev_dvfs_status = true;
#endif /* CONFIG_MALI_DVFS */
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &clk);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (!cur_state) {
		prev_tmu_status = platform->tmu_status;
#ifdef CONFIG_MALI_DVFS
		prev_dvfs_status = platform->dvfs_status;
#endif /* CONFIG_MALI_DVFS */
		prev_policy = kbase_pm_get_policy(pkbdev);
	}

	if (clk == 0) {
		kbase_pm_set_policy(pkbdev, prev_policy);
		platform->tmu_status = prev_tmu_status;
#ifdef CONFIG_MALI_DVFS
		if (!platform->dvfs_status)
			gpu_dvfs_on_off(true);
#endif /* CONFIG_MALI_DVFS */
		cur_state = false;
	} else {
		policy_count = kbase_pm_list_policies(pkbdev, &policy_list);
		for (i = 0; i < policy_count; i++) {
			if (sysfs_streq(policy_list[i]->name, "always_on")) {
				kbase_pm_set_policy(pkbdev, policy_list[i]);
				break;
			}
		}
		platform->tmu_status = false;
#ifdef CONFIG_MALI_DVFS
		if (platform->dvfs_status)
			gpu_dvfs_on_off(false);
#endif /* CONFIG_MALI_DVFS */
		gpu_set_target_clk_vol(clk, false);
		cur_state = true;
	}

	return count;
}

static ssize_t show_vol(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", gpu_get_cur_voltage(platform));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_power_state(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", gpu_control_is_power_on(pkbdev));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static int gpu_get_asv_table(struct exynos_context *platform, char *buf, size_t buf_size)
{
	int i, cnt = 0;

	if (!platform)
		return -ENODEV;

	if (buf == NULL)
		return 0;

	cnt += snprintf(buf+cnt, buf_size-cnt, "GPU, vol, min, max, down_stay, mif, cpu0, cpu1\n");

	for (i = gpu_dvfs_get_level(platform->gpu_max_clock); i <= gpu_dvfs_get_level(platform->gpu_min_clock); i++) {
		cnt += snprintf(buf+cnt, buf_size-cnt, "%d, %7d, %2d, %3d, %d, %7d, %7d, %7d\n",
		platform->table[i].clock, platform->table[i].voltage, platform->table[i].min_threshold,
		platform->table[i].max_threshold, platform->table[i].down_staycount, platform->table[i].mem_freq,
		platform->table[i].cpu_little_min_freq, platform->table[i].cpu_middle_min_freq);
	}

	return cnt;
}

static ssize_t show_asv_table(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += gpu_get_asv_table(platform, buf+ret, (size_t)PAGE_SIZE-ret);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static int gpu_get_dvfs_table(struct exynos_context *platform, char *buf, size_t buf_size)
{
	int i, cnt = 0;

	if (!platform)
		return -ENODEV;

	if (buf == NULL)
		return 0;

	for (i = gpu_dvfs_get_level(platform->gpu_max_clock); i <= gpu_dvfs_get_level(platform->gpu_min_clock); i++)
		cnt += snprintf(buf+cnt, buf_size-cnt, " %d", platform->table[i].clock);

	cnt += snprintf(buf+cnt, buf_size-cnt, "\n");

	return cnt;
}

static ssize_t show_dvfs_table(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += gpu_get_dvfs_table(platform, buf+ret, (size_t)PAGE_SIZE-ret);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_time_in_state(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	gpu_dvfs_update_time_in_state(gpu_control_is_power_on(pkbdev) * platform->cur_clock);

	for (i = gpu_dvfs_get_level(platform->gpu_min_clock); i >= gpu_dvfs_get_level(platform->gpu_max_clock); i--) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d %llu\n",
				platform->table[i].clock,
				platform->table[i].time);
	}

	if (ret >= PAGE_SIZE - 1) {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_time_in_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	gpu_dvfs_init_time_in_state();

	return count;
}

static ssize_t show_utilization(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", gpu_control_is_power_on(pkbdev) * platform->env_data.utilization);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_perf(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", gpu_control_is_power_on(pkbdev) * platform->env_data.perf);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

#ifdef CONFIG_MALI_DVFS
static ssize_t show_dvfs(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->dvfs_status);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_dvfs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if (sysfs_streq("0", buf))
		gpu_dvfs_on_off(false);
	else if (sysfs_streq("1", buf))
		gpu_dvfs_on_off(true);

	return count;
}

static ssize_t show_governor(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info;
	int i;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%s\n", governor_info[i].name);

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "[Current Governor] %s", governor_info[platform->governor_type].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_governor(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int next_governor_type;
	struct exynos_context *platform  = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &next_governor_type);

	if ((next_governor_type < 0) || (next_governor_type >= G3D_MAX_GOVERNOR_NUM)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_governor_change(next_governor_type);

	if (ret < 0) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u,
				"%s: fail to set the new governor (%d)\n", __func__, next_governor_type);
		return -ENOENT;
	}

	return count;
}

static ssize_t show_max_lock_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i;
	int max_lock_status[NUMBER_LOCK];
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	for (i = 0; i < NUMBER_LOCK; i++)
		max_lock_status[i] = platform->user_max_lock[i];
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	for (i = 0; i < NUMBER_LOCK; i++)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "[%d:%d]", i,  max_lock_status[i]);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_min_lock_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i;
	int min_lock_status[NUMBER_LOCK];
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	for (i = 0; i < NUMBER_LOCK; i++)
		min_lock_status[i] = platform->user_min_lock[i];
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	for (i = 0; i < NUMBER_LOCK; i++)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "[%d:%d]", i,  min_lock_status[i]);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_max_lock_dvfs(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;
	int user_locked_clock = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	locked_clock = platform->max_lock;
	user_locked_clock = platform->user_max_lock_input;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	if (locked_clock > 0)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d / %d", locked_clock, user_locked_clock);
	else
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "-1");

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_max_lock_dvfs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, clock = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	if (sysfs_streq("0", buf)) {
		platform->user_max_lock_input = 0;
		gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		platform->user_max_lock_input = clock;

		clock = gpu_dvfs_get_level_clock(clock);

		ret = gpu_dvfs_get_level(clock);
		if ((ret < gpu_dvfs_get_level(platform->gpu_max_clock)) || (ret > gpu_dvfs_get_level(platform->gpu_min_clock))) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid clock value (%d)\n", __func__, clock);
			return -ENOENT;
		}

		if (clock == platform->gpu_max_clock)
			gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, SYSFS_LOCK, 0);
		else
			gpu_dvfs_clock_lock(GPU_DVFS_MAX_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}

static ssize_t show_min_lock_dvfs(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;
	int user_locked_clock = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	locked_clock = platform->min_lock;
	user_locked_clock = platform->user_min_lock_input;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	if (locked_clock > 0)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d / %d", locked_clock, user_locked_clock);
	else
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "-1");

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_min_lock_dvfs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, clock = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	if (sysfs_streq("0", buf)) {
		platform->user_min_lock_input = 0;
		gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		platform->user_min_lock_input = clock;

		clock = gpu_dvfs_get_level_clock(clock);

		ret = gpu_dvfs_get_level(clock);
		if ((ret < gpu_dvfs_get_level(platform->gpu_max_clock)) || (ret > gpu_dvfs_get_level(platform->gpu_min_clock))) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid clock value (%d)\n", __func__, clock);
			return -ENOENT;
		}

		if (clock > platform->gpu_max_clock_limit)
			clock = platform->gpu_max_clock_limit;

		if (clock == platform->gpu_min_clock)
			gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, SYSFS_LOCK, 0);
		else
			gpu_dvfs_clock_lock(GPU_DVFS_MIN_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}

static ssize_t show_down_staycount(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	for (i = gpu_dvfs_get_level(platform->gpu_max_clock); i <= gpu_dvfs_get_level(platform->gpu_min_clock); i++)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "Clock %d - %d\n",
			platform->table[i].clock, platform->table[i].down_staycount);
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

#define MIN_DOWN_STAYCOUNT	1
#define MAX_DOWN_STAYCOUNT	10
static ssize_t set_down_staycount(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long flags;
	char tmpbuf[32];
	char *sptr, *tok;
	int ret = -1;
	int clock = -1, level = -1, down_staycount = 0;
	unsigned int len = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	len = (unsigned int)min(count, sizeof(tmpbuf) - 1);
	memcpy(tmpbuf, buf, len);
	tmpbuf[len] = '\0';
	sptr = tmpbuf;

	tok = strsep(&sptr, " ,");
	if (tok == NULL) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid input\n", __func__);
		return -ENOENT;
	}

	ret = kstrtoint(tok, 0, &clock);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid input %d\n", __func__, clock);
		return -ENOENT;
	}

	tok = strsep(&sptr, " ,");
	if (tok == NULL) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid input\n", __func__);
		return -ENOENT;
	}

	ret = kstrtoint(tok, 0, &down_staycount);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid input %d\n", __func__, down_staycount);
		return -ENOENT;
	}

	level = gpu_dvfs_get_level(clock);
	if (level < 0) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid clock value (%d)\n", __func__, clock);
		return -ENOENT;
	}

	if ((down_staycount < MIN_DOWN_STAYCOUNT) || (down_staycount > MAX_DOWN_STAYCOUNT)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: down_staycount is out of range (%d, %d ~ %d)\n",
			__func__, down_staycount, MIN_DOWN_STAYCOUNT, MAX_DOWN_STAYCOUNT);
		return -ENOENT;
	}

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	platform->table[level].down_staycount = down_staycount;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	return count;
}

static ssize_t show_highspeed_clock(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_clock = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	highspeed_clock = platform->interactive.highspeed_clock;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", highspeed_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_highspeed_clock(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_clock = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &highspeed_clock);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_get_level(highspeed_clock);
	if ((ret < gpu_dvfs_get_level(platform->gpu_max_clock)) || (ret > gpu_dvfs_get_level(platform->gpu_min_clock))) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid clock value (%d)\n", __func__, highspeed_clock);
		return -ENOENT;
	}

	if (highspeed_clock > platform->gpu_max_clock_limit)
		highspeed_clock = platform->gpu_max_clock_limit;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	platform->interactive.highspeed_clock = highspeed_clock;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	return count;
}

static ssize_t show_highspeed_load(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_load = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	highspeed_load = platform->interactive.highspeed_load;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", highspeed_load);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_highspeed_load(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_load = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &highspeed_load);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((highspeed_load < 0) || (highspeed_load > 100)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid load value (%d)\n", __func__, highspeed_load);
		return -ENOENT;
	}

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	platform->interactive.highspeed_load = highspeed_load;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	return count;
}

static ssize_t show_highspeed_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_delay = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	highspeed_delay = platform->interactive.highspeed_delay;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", highspeed_delay);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_highspeed_delay(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_delay = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &highspeed_delay);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((highspeed_delay < 0) || (highspeed_delay > 5)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid load value (%d)\n", __func__, highspeed_delay);
		return -ENOENT;
	}

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	platform->interactive.highspeed_delay = highspeed_delay;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	return count;
}

static ssize_t show_wakeup_lock(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->wakeup_lock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_wakeup_lock(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	if (sysfs_streq("0", buf))
		platform->wakeup_lock = false;
	else if (sysfs_streq("1", buf))
		platform->wakeup_lock = true;
	else
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid val - only [0 or 1] is available\n", __func__);

	return count;
}

static ssize_t show_polling_speed(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->polling_speed);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_polling_speed(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, polling_speed;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &polling_speed);

	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((polling_speed < 100) || (polling_speed > 1000)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: out of range [100~1000] (%d)\n", __func__, polling_speed);
		return -ENOENT;
	}

	platform->polling_speed = polling_speed;

	return count;
}

static ssize_t show_tmu(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->tmu_status);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_tmu_control(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	if (sysfs_streq("0", buf)) {
		if (platform->voltage_margin != 0) {
			platform->voltage_margin = 0;
			gpu_set_target_clk_vol(platform->cur_clock, false);
		}
		gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, TMU_LOCK, 0);
		platform->tmu_status = false;
	} else if (sysfs_streq("1", buf))
		platform->tmu_status = true;
	else
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value - only [0 or 1] is available\n", __func__);

	return count;
}

#ifdef CONFIG_CPU_THERMAL_IPA
static ssize_t show_norm_utilization(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
#ifdef CONFIG_EXYNOS_THERMAL

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", gpu_ipa_dvfs_get_norm_utilisation(pkbdev));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}
#else
	GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: EXYNOS THERMAL build config is disabled\n", __func__);
#endif /* CONFIG_EXYNOS_THERMAL */

	return ret;
}

static ssize_t show_utilization_stats(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
#ifdef CONFIG_EXYNOS_THERMAL
	struct mali_debug_utilisation_stats stats;

	gpu_ipa_dvfs_get_utilisation_stats(&stats);

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "util=%d norm_util=%d norm_freq=%d time_busy=%u time_idle=%u time_tick=%d",
					stats.s.utilisation, stats.s.norm_utilisation,
					stats.s.freq_for_norm, stats.time_busy, stats.time_idle,
					stats.time_tick);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}
#else
	GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: EXYNOS THERMAL build config is disabled\n", __func__);
#endif /* CONFIG_EXYNOS_THERMAL */

	return ret;
}
#endif /* CONFIG_CPU_THERMAL_IPA */
#endif /* CONFIG_MALI_DVFS */

static ssize_t show_debug_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "[Current] %d (%d ~ %d)",
				gpu_get_debug_level(), DVFS_DEBUG_START+1, DVFS_DEBUG_END-1);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_debug_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int debug_level, ret;

	ret = kstrtoint(buf, 0, &debug_level);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((debug_level <= DVFS_DEBUG_START) || (debug_level >= DVFS_DEBUG_END)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid debug level (%d)\n", __func__, debug_level);
		return -ENOENT;
	}

	gpu_set_debug_level(debug_level);

	return count;
}

#ifdef CONFIG_MALI_EXYNOS_TRACE
static ssize_t show_trace_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int level;

	for (level = TRACE_NONE + 1; level < TRACE_END - 1; level++)
		if (gpu_check_trace_level(level))
			ret += snprintf(buf+ret, PAGE_SIZE-ret, "<%d> ", level);
	ret += snprintf(buf+ret, PAGE_SIZE-ret, "\nList: %d ~ %d\n(None: %d, All: %d)",
										TRACE_NONE + 1, TRACE_ALL - 1, TRACE_NONE, TRACE_ALL);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_trace_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int trace_level, ret;

	ret = kstrtoint(buf, 0, &trace_level);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((trace_level <= TRACE_START) || (trace_level >= TRACE_END)) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid trace level (%d)\n", __func__, trace_level);
		return -ENOENT;
	}

	gpu_set_trace_level(trace_level);

	return count;
}

extern void kbasep_trace_format_msg(struct kbase_trace *trace_msg, char *buffer, int len);
static ssize_t show_trace_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	u32 start, end;

	spin_lock_irqsave(&pkbdev->trace_lock, flags);
	start = pkbdev->trace_first_out;
	end = pkbdev->trace_next_in;

	while (start != end) {
		char buffer[KBASE_TRACE_SIZE];
		struct kbase_trace *trace_msg = &pkbdev->trace_rbuf[start];

		kbasep_trace_format_msg(trace_msg, buffer, KBASE_TRACE_SIZE);
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%s\n", buffer);

        if (ret >= PAGE_SIZE - 1)
            break;

		start = (start + 1) & KBASE_TRACE_MASK;
	}

	spin_unlock_irqrestore(&pkbdev->trace_lock, flags);
	KBASE_TRACE_CLEAR(pkbdev);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t init_trace_dump(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	KBASE_TRACE_CLEAR(pkbdev);

	return count;
}
#endif /* CONFIG_MALI_EXYNOS_TRACE */

#ifdef DEBUG_FBDEV
static ssize_t show_fbdev(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i;

	for (i = 0; i < num_registered_fb; i++)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "fb[%d] xres=%d, yres=%d, addr=0x%lx\n", i, registered_fb[i]->var.xres, registered_fb[i]->var.yres, registered_fb[i]->fix.smem_start);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}
#endif

static int gpu_get_status(struct exynos_context *platform, char *buf, size_t buf_size)
{
	int cnt = 0;
	int i;
	int mmu_fault_cnt = 0;

	if (!platform)
		return -ENODEV;

	if (buf == NULL)
		return 0;

	for (i = GPU_MMU_TRANSLATION_FAULT; i <= GPU_MMU_MEMORY_ATTRIBUTES_FAULT; i++)
		mmu_fault_cnt += platform->gpu_exception_count[i];

	cnt += snprintf(buf+cnt, buf_size-cnt, "reset count : %d\n", platform->gpu_exception_count[GPU_RESET]);
	cnt += snprintf(buf+cnt, buf_size-cnt, "data invalid count : %d\n", platform->gpu_exception_count[GPU_DATA_INVALIDATE_FAULT]);
	cnt += snprintf(buf+cnt, buf_size-cnt, "mmu fault count : %d\n", mmu_fault_cnt);

	for (i = 0; i < BMAX_RETRY_CNT; i++)
		cnt += snprintf(buf+cnt, buf_size-cnt, "warmup retry count %d : %d\n", i+1, platform->balance_retry_count[i]);

	return cnt;
}

static ssize_t show_gpu_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += gpu_get_status(platform, buf+ret, (size_t)PAGE_SIZE-ret);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

#ifdef CONFIG_MALI_SEC_VK_BOOST
static ssize_t show_vk_boost_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->ctx_vk_need_qos);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}
#endif

#ifdef CONFIG_MALI_SUSTAINABLE_OPT
static ssize_t show_sustainable_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->sustainable.status);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}
#endif

#ifdef CONFIG_MALI_SEC_CL_BOOST
static ssize_t set_cl_boost_disable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int cl_boost_disable = 0;
	int ret;

	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret = kstrtoint(buf, 0, &cl_boost_disable);
	if (ret) {
		GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (cl_boost_disable == 0)
		platform->cl_boost_disable = false;
	else
		platform->cl_boost_disable = true;

	return count;
}

static ssize_t show_cl_boost_disable(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->cl_boost_disable);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}
#endif
/** The sysfs file @c clock, fbdev.
 *
 * This is used for obtaining information about the mali t series operating clock & framebuffer address,
 */

DEVICE_ATTR(clock, S_IRUGO|S_IWUSR, show_clock, set_clock);
DEVICE_ATTR(vol, S_IRUGO, show_vol, NULL);
DEVICE_ATTR(power_state, S_IRUGO, show_power_state, NULL);
DEVICE_ATTR(asv_table, S_IRUGO, show_asv_table, NULL);
DEVICE_ATTR(dvfs_table, S_IRUGO, show_dvfs_table, NULL);
DEVICE_ATTR(time_in_state, S_IRUGO|S_IWUSR, show_time_in_state, set_time_in_state);
DEVICE_ATTR(utilization, S_IRUGO, show_utilization, NULL);
DEVICE_ATTR(perf, S_IRUGO, show_perf, NULL);
#ifdef CONFIG_MALI_DVFS
DEVICE_ATTR(dvfs, S_IRUGO|S_IWUSR, show_dvfs, set_dvfs);
DEVICE_ATTR(dvfs_governor, S_IRUGO|S_IWUSR, show_governor, set_governor);
DEVICE_ATTR(dvfs_max_lock_status, S_IRUGO, show_max_lock_status, NULL);
DEVICE_ATTR(dvfs_min_lock_status, S_IRUGO, show_min_lock_status, NULL);
DEVICE_ATTR(dvfs_max_lock, S_IRUGO|S_IWUSR, show_max_lock_dvfs, set_max_lock_dvfs);
DEVICE_ATTR(dvfs_min_lock, S_IRUGO|S_IWUSR, show_min_lock_dvfs, set_min_lock_dvfs);
DEVICE_ATTR(down_staycount, S_IRUGO|S_IWUSR, show_down_staycount, set_down_staycount);
DEVICE_ATTR(highspeed_clock, S_IRUGO|S_IWUSR, show_highspeed_clock, set_highspeed_clock);
DEVICE_ATTR(highspeed_load, S_IRUGO|S_IWUSR, show_highspeed_load, set_highspeed_load);
DEVICE_ATTR(highspeed_delay, S_IRUGO|S_IWUSR, show_highspeed_delay, set_highspeed_delay);
DEVICE_ATTR(wakeup_lock, S_IRUGO|S_IWUSR, show_wakeup_lock, set_wakeup_lock);
DEVICE_ATTR(polling_speed, S_IRUGO|S_IWUSR, show_polling_speed, set_polling_speed);
DEVICE_ATTR(tmu, S_IRUGO|S_IWUSR, show_tmu, set_tmu_control);
#ifdef CONFIG_CPU_THERMAL_IPA
DEVICE_ATTR(norm_utilization, S_IRUGO, show_norm_utilization, NULL);
DEVICE_ATTR(utilization_stats, S_IRUGO, show_utilization_stats, NULL);
#endif /* CONFIG_CPU_THERMAL_IPA */
#endif /* CONFIG_MALI_DVFS */
DEVICE_ATTR(debug_level, S_IRUGO|S_IWUSR, show_debug_level, set_debug_level);
#ifdef CONFIG_MALI_EXYNOS_TRACE
DEVICE_ATTR(trace_level, S_IRUGO|S_IWUSR, show_trace_level, set_trace_level);
DEVICE_ATTR(trace_dump, S_IRUGO|S_IWUSR, show_trace_dump, init_trace_dump);
#endif /* CONFIG_MALI_EXYNOS_TRACE */
#ifdef DEBUG_FBDEV
DEVICE_ATTR(fbdev, S_IRUGO, show_fbdev, NULL);
#endif
DEVICE_ATTR(gpu_status, S_IRUGO, show_gpu_status, NULL);
#ifdef CONFIG_MALI_SEC_VK_BOOST
DEVICE_ATTR(vk_boost_status, S_IRUGO, show_vk_boost_status, NULL);
#endif
#ifdef CONFIG_MALI_SUSTAINABLE_OPT
DEVICE_ATTR(sustainable_status, S_IRUGO, show_sustainable_status, NULL);
#endif
#ifdef CONFIG_MALI_SEC_CL_BOOST
DEVICE_ATTR(cl_boost_disable, S_IRUGO|S_IWUSR, show_cl_boost_disable, set_cl_boost_disable);
#endif

#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
#ifdef CONFIG_MALI_DVFS
#define BUF_SIZE 1000
static ssize_t show_kernel_sysfs_gpu_info(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;
	ssize_t ret = 0;

	if (!platform)
		return -ENODEV;

	if (buf == NULL)
		return 0;

	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"SSTOP\":\"%d\",", platform->gpu_exception_count[GPU_SOFT_STOP]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"HSTOP\":\"%d\",", platform->gpu_exception_count[GPU_HARD_STOP]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"RESET\":\"%d\",", platform->gpu_exception_count[GPU_RESET]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"DIFLT\":\"%d\",", platform->gpu_exception_count[GPU_DATA_INVALIDATE_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"TRFLT\":\"%d\",", platform->gpu_exception_count[GPU_MMU_TRANSLATION_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"PMFLT\":\"%d\",", platform->gpu_exception_count[GPU_MMU_PERMISSION_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"BFLT\":\"%d\",", platform->gpu_exception_count[GPU_MMU_TRANSTAB_BUS_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"ACCFG\":\"%d\",", platform->gpu_exception_count[GPU_MMU_ACCESS_FLAG_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"ASFLT\":\"%d\",", platform->gpu_exception_count[GPU_MMU_ADDRESS_SIZE_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"ATFLT\":\"%d\",", platform->gpu_exception_count[GPU_MMU_MEMORY_ATTRIBUTES_FAULT]);
	ret += snprintf(buf+ret, BUF_SIZE-ret, "\"UNKN\":\"%d\"", platform->gpu_exception_count[GPU_UNKNOWN]);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_kernel_sysfs_max_lock_dvfs(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	locked_clock = platform->max_lock;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	if (locked_clock > 0)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", locked_clock);
	else
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->gpu_max_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_kernel_sysfs_max_lock_dvfs(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret, clock = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	if (sysfs_streq("0", buf)) {
		platform->user_max_lock_input = 0;
		gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		platform->user_max_lock_input = clock;

		clock = gpu_dvfs_get_level_clock(clock);

		ret = gpu_dvfs_get_level(clock);
		if ((ret < gpu_dvfs_get_level(platform->gpu_max_clock)) || (ret > gpu_dvfs_get_level(platform->gpu_min_clock))) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid clock value (%d)\n", __func__, clock);
			return -ENOENT;
		}

		if (clock == platform->gpu_max_clock)
			gpu_dvfs_clock_lock(GPU_DVFS_MAX_UNLOCK, SYSFS_LOCK, 0);
		else
			gpu_dvfs_clock_lock(GPU_DVFS_MAX_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}

static ssize_t show_kernel_sysfs_available_governor(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info;
	int i;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%s ", governor_info[i].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_kernel_sysfs_min_lock_dvfs(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int locked_clock = -1;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	spin_lock_irqsave(&platform->gpu_dvfs_spinlock, flags);
	locked_clock = platform->min_lock;
	spin_unlock_irqrestore(&platform->gpu_dvfs_spinlock, flags);

	if (locked_clock > 0)
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", locked_clock);
	else
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", platform->gpu_min_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_kernel_sysfs_min_lock_dvfs(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret, clock = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	if (sysfs_streq("0", buf)) {
		platform->user_min_lock_input = 0;
		gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, SYSFS_LOCK, 0);
	} else {
		ret = kstrtoint(buf, 0, &clock);
		if (ret) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
			return -ENOENT;
		}

		platform->user_min_lock_input = clock;

		clock = gpu_dvfs_get_level_clock(clock);

		ret = gpu_dvfs_get_level(clock);
		if ((ret < gpu_dvfs_get_level(platform->gpu_max_clock)) || (ret > gpu_dvfs_get_level(platform->gpu_min_clock))) {
			GPU_LOG(DVFS_WARNING, DUMMY, 0u, 0u, "%s: invalid clock value (%d)\n", __func__, clock);
			return -ENOENT;
		}

		if (clock > platform->gpu_max_clock_limit)
			clock = platform->gpu_max_clock_limit;

		if (clock == platform->gpu_min_clock)
			gpu_dvfs_clock_lock(GPU_DVFS_MIN_UNLOCK, SYSFS_LOCK, 0);
		else
			gpu_dvfs_clock_lock(GPU_DVFS_MIN_LOCK, SYSFS_LOCK, clock);
	}

	return count;
}
#endif /* #ifdef CONFIG_MALI_DVFS */

static ssize_t show_kernel_sysfs_utilization(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%3d%%", platform->env_data.utilization);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_kernel_sysfs_clock(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int clock = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

#ifdef CONFIG_MALI_RT_PM
	if (platform->exynos_pm_domain) {
		mutex_lock(&platform->exynos_pm_domain->access_lock);
		if (!platform->dvs_is_enabled && gpu_is_power_on())
			clock = gpu_get_cur_clock(platform);
		mutex_unlock(&platform->exynos_pm_domain->access_lock);
	}
#else
	if (gpu_control_is_power_on(pkbdev) == 1) {
		mutex_lock(&platform->gpu_clock_lock);
		if (!platform->dvs_is_enabled)
			clock = gpu_get_cur_clock(platform);
		mutex_unlock(&platform->gpu_clock_lock);
	}
#endif

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d", clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t show_kernel_sysfs_freq_table(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int i = 0;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	for (i = gpu_dvfs_get_level(platform->gpu_min_clock); i >= gpu_dvfs_get_level(platform->gpu_max_clock); i--) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d ", platform->table[i].clock);
	}

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

#ifdef CONFIG_MALI_DVFS
static ssize_t show_kernel_sysfs_governor(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info = NULL;
	struct exynos_context *platform = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%s", governor_info[platform->governor_type].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static ssize_t set_kernel_sysfs_governor(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int i = 0;
	int next_governor_type = -1;
	size_t governor_name_size = 0;
	gpu_dvfs_governor_info *governor_info = NULL;
	struct exynos_context *platform  = (struct exynos_context *)pkbdev->platform_context;

	if (!platform)
		return -ENODEV;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++) {
		governor_name_size = strlen(governor_info[i].name);
		if (!strncmp(buf, governor_info[i].name, governor_name_size)) {
			next_governor_type = i;
			break;
		}
	}

	if ((next_governor_type < 0) || (next_governor_type >= G3D_MAX_GOVERNOR_NUM)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_governor_change(next_governor_type);

	if (ret < 0) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u,
				"%s: fail to set the new governor (%d)\n", __func__, next_governor_type);
		return -ENOENT;
	}

	return count;
}
#endif /* #ifdef CONFIG_MALI_DVFS */

static ssize_t show_kernel_sysfs_gpu_model(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/* COPY from mali_kbase_core_linux.c : 2594 line, last updated: 20161017, r2p0-03rel0 */
	static const struct gpu_product_id_name {
		unsigned id;
		char *name;
	} gpu_product_id_names[] = {
		{ .id = GPU_ID_PI_T60X, .name = "Mali-T60x" },
		{ .id = GPU_ID_PI_T62X, .name = "Mali-T62x" },
		{ .id = GPU_ID_PI_T72X, .name = "Mali-T72x" },
		{ .id = GPU_ID_PI_T76X, .name = "Mali-T76x" },
		{ .id = GPU_ID_PI_T82X, .name = "Mali-T82x" },
		{ .id = GPU_ID_PI_T83X, .name = "Mali-T83x" },
		{ .id = GPU_ID_PI_T86X, .name = "Mali-T86x" },
		{ .id = GPU_ID_PI_TFRX, .name = "Mali-T88x" },
		{ .id = GPU_ID2_PRODUCT_TMIX >> GPU_ID_VERSION_PRODUCT_ID_SHIFT,
		  .name = "Mali-G71" },
		{ .id = GPU_ID2_PRODUCT_THEX >> GPU_ID_VERSION_PRODUCT_ID_SHIFT,
		  .name = "Mali-THEx" },
	};
	const char *product_name = "(Unknown Mali GPU)";
	struct kbase_device *kbdev;
	u32 gpu_id;
	unsigned product_id, product_id_mask;
	unsigned i;
	bool is_new_format;

	kbdev = pkbdev;
	if (!kbdev)
		return -ENODEV;

	gpu_id = kbdev->gpu_props.props.raw_props.gpu_id;
	product_id = gpu_id >> GPU_ID_VERSION_PRODUCT_ID_SHIFT;
	is_new_format = GPU_ID_IS_NEW_FORMAT(product_id);
	product_id_mask =
		(is_new_format ?
			GPU_ID2_PRODUCT_MODEL :
			GPU_ID_VERSION_PRODUCT_ID) >>
		GPU_ID_VERSION_PRODUCT_ID_SHIFT;

	for (i = 0; i < ARRAY_SIZE(gpu_product_id_names); ++i) {
		const struct gpu_product_id_name *p = &gpu_product_id_names[i];

		if ((GPU_ID_IS_NEW_FORMAT(p->id) == is_new_format) &&
		    (p->id & product_id_mask) ==
		    (product_id & product_id_mask)) {
			product_name = p->name;
			break;
		}
	}

	return scnprintf(buf, PAGE_SIZE, "%s\n", product_name);
}

#if defined(CONFIG_MALI_DVFS) && defined(CONFIG_EXYNOS_THERMAL) && defined(CONFIG_GPU_THERMAL)

extern struct exynos_tmu_data *gpu_thermal_data;

static ssize_t show_kernel_sysfs_gpu_temp(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int gpu_temp = 0;
	int gpu_temp_int = 0;
	int gpu_temp_point = 0;


	if (!gpu_thermal_data) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "[Kernel group SYSFS] thermal driver does not ready\n");
		return -ENODEV;
	}

	mutex_lock(&gpu_thermal_data->lock);

	if (gpu_thermal_data->num_of_sensors)
		gpu_temp = gpu_thermal_data->tmu_read(gpu_thermal_data) * MCELSIUS;

	mutex_unlock(&gpu_thermal_data->lock);

	gpu_temp_int = gpu_temp / 1000;
	gpu_temp_point = gpu_temp % gpu_temp_int;
	ret += snprintf(buf+ret, PAGE_SIZE-ret, "%d.%d", gpu_temp_int, gpu_temp_point);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf+ret, PAGE_SIZE-ret, "\n");
	} else {
		buf[PAGE_SIZE-2] = '\n';
		buf[PAGE_SIZE-1] = '\0';
		ret = PAGE_SIZE-1;
	}

	return ret;
}

static struct kobj_attribute gpu_temp_attribute =
	__ATTR(gpu_tmu, S_IRUGO, show_kernel_sysfs_gpu_temp, NULL);
#endif

#ifdef CONFIG_MALI_DVFS
static struct kobj_attribute gpu_info_attribute =
	__ATTR(gpu_info, S_IRUGO, show_kernel_sysfs_gpu_info, NULL);

static struct kobj_attribute gpu_max_lock_attribute =
	__ATTR(gpu_max_clock, S_IRUGO|S_IWUSR, show_kernel_sysfs_max_lock_dvfs, set_kernel_sysfs_max_lock_dvfs);

static struct kobj_attribute gpu_min_lock_attribute =
	__ATTR(gpu_min_clock, S_IRUGO|S_IWUSR, show_kernel_sysfs_min_lock_dvfs, set_kernel_sysfs_min_lock_dvfs);
#endif /* #ifdef CONFIG_MALI_DVFS */

static struct kobj_attribute gpu_busy_attribute =
	__ATTR(gpu_busy, S_IRUGO, show_kernel_sysfs_utilization, NULL);

static struct kobj_attribute gpu_clock_attribute =
	__ATTR(gpu_clock, S_IRUGO, show_kernel_sysfs_clock, NULL);

static struct kobj_attribute gpu_freq_table_attribute =
	__ATTR(gpu_freq_table, S_IRUGO, show_kernel_sysfs_freq_table, NULL);

#ifdef CONFIG_MALI_DVFS
static struct kobj_attribute gpu_governor_attribute =
	__ATTR(gpu_governor, S_IRUGO|S_IWUSR, show_kernel_sysfs_governor, set_kernel_sysfs_governor);

static struct kobj_attribute gpu_available_governor_attribute =
	__ATTR(gpu_available_governor, S_IRUGO, show_kernel_sysfs_available_governor, NULL);
#endif /* #ifdef CONFIG_MALI_DVFS */

static struct kobj_attribute gpu_model_attribute =
	__ATTR(gpu_model, S_IRUGO, show_kernel_sysfs_gpu_model, NULL);


static struct attribute *attrs[] = {
#ifdef CONFIG_MALI_DVFS
#if defined(CONFIG_EXYNOS_THERMAL) && defined(CONFIG_GPU_THERMAL)
	&gpu_temp_attribute.attr,
#endif
	&gpu_info_attribute.attr,
	&gpu_max_lock_attribute.attr,
	&gpu_min_lock_attribute.attr,
#endif /* #ifdef CONFIG_MALI_DVFS */
	&gpu_busy_attribute.attr,
	&gpu_clock_attribute.attr,
	&gpu_freq_table_attribute.attr,
#ifdef CONFIG_MALI_DVFS
	&gpu_governor_attribute.attr,
	&gpu_available_governor_attribute.attr,
#endif /* #ifdef CONFIG_MALI_DVFS */
	&gpu_model_attribute.attr,
	NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};
static struct kobject *external_kobj;
#endif

int gpu_create_sysfs_file(struct device *dev)
{
#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
	int retval = 0;
#endif

	if (device_create_file(dev, &dev_attr_clock)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [clock]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_vol)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [vol]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_power_state)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [power_state]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_asv_table)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [asv_table]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_dvfs_table)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs_table]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_time_in_state)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [time_in_state]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_utilization)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [utilization]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_perf)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [perf]\n");
		goto out;
	}
#ifdef CONFIG_MALI_DVFS
	if (device_create_file(dev, &dev_attr_dvfs)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_dvfs_governor)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs_governor]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_dvfs_max_lock_status)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs_max_lock_status]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_dvfs_min_lock_status)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs_min_lock_status]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_dvfs_max_lock)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs_max_lock]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_dvfs_min_lock)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [dvfs_min_lock]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_down_staycount)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [down_staycount]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_highspeed_clock)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [highspeed_clock]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_highspeed_load)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [highspeed_load]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_highspeed_delay)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [highspeed_delay]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_wakeup_lock)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [wakeup_lock]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_polling_speed)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [polling_speed]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_tmu)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [tmu]\n");
		goto out;
	}
#ifdef CONFIG_CPU_THERMAL_IPA
	if (device_create_file(dev, &dev_attr_norm_utilization)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [norm_utilization]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_utilization_stats)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [utilization_stats]\n");
		goto out;
	}
#endif /* CONFIG_CPU_THERMAL_IPA */
#endif /* CONFIG_MALI_DVFS */
	if (device_create_file(dev, &dev_attr_debug_level)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [debug_level]\n");
		goto out;
	}
#ifdef CONFIG_MALI_EXYNOS_TRACE
	if (device_create_file(dev, &dev_attr_trace_level)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [trace_level]\n");
		goto out;
	}

	if (device_create_file(dev, &dev_attr_trace_dump)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [trace_dump]\n");
		goto out;
	}
#endif /* CONFIG_MALI_EXYNOS_TRACE */
#ifdef DEBUG_FBDEV
	if (device_create_file(dev, &dev_attr_fbdev)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [fbdev]\n");
		goto out;
	}
#endif

	if (device_create_file(dev, &dev_attr_gpu_status)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [gpu_status]\n");
		goto out;
	}

#ifdef CONFIG_MALI_SEC_VK_BOOST
	if (device_create_file(dev, &dev_attr_vk_boost_status)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [vk_boost_status]\n");
		goto out;
	}
#endif

#ifdef CONFIG_MALI_SUSTAINABLE_OPT
	if (device_create_file(dev, &dev_attr_sustainable_status)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [sustainable_status]\n");
		goto out;
	}
#endif

#ifdef CONFIG_MALI_SEC_CL_BOOST
	if (device_create_file(dev, &dev_attr_cl_boost_disable)) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create sysfs file [cl_boost_disable]\n");
		goto out;
	}
#endif

#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
	external_kobj = kobject_create_and_add("gpu", kernel_kobj);
	if (!external_kobj) {
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't create Kobj for group [KERNEL - GPU]\n");
		goto out;
	}

	retval = sysfs_create_group(external_kobj, &attr_group);
	if (retval) {
		kobject_put(external_kobj);
		GPU_LOG(DVFS_ERROR, DUMMY, 0u, 0u, "couldn't add sysfs group [KERNEL - GPU]\n");
		goto out;
	}
#endif

	return 0;
out:
	return -ENOENT;
}

void gpu_remove_sysfs_file(struct device *dev)
{
	device_remove_file(dev, &dev_attr_clock);
	device_remove_file(dev, &dev_attr_vol);
	device_remove_file(dev, &dev_attr_power_state);
	device_remove_file(dev, &dev_attr_asv_table);
	device_remove_file(dev, &dev_attr_dvfs_table);
	device_remove_file(dev, &dev_attr_time_in_state);
	device_remove_file(dev, &dev_attr_utilization);
	device_remove_file(dev, &dev_attr_perf);
#ifdef CONFIG_MALI_DVFS
	device_remove_file(dev, &dev_attr_dvfs);
	device_remove_file(dev, &dev_attr_dvfs_governor);
	device_remove_file(dev, &dev_attr_dvfs_max_lock_status);
	device_remove_file(dev, &dev_attr_dvfs_min_lock_status);
	device_remove_file(dev, &dev_attr_dvfs_max_lock);
	device_remove_file(dev, &dev_attr_dvfs_min_lock);
	device_remove_file(dev, &dev_attr_down_staycount);
	device_remove_file(dev, &dev_attr_highspeed_clock);
	device_remove_file(dev, &dev_attr_highspeed_load);
	device_remove_file(dev, &dev_attr_highspeed_delay);
	device_remove_file(dev, &dev_attr_wakeup_lock);
	device_remove_file(dev, &dev_attr_polling_speed);
	device_remove_file(dev, &dev_attr_tmu);
#ifdef CONFIG_CPU_THERMAL_IPA
	device_remove_file(dev, &dev_attr_norm_utilization);
	device_remove_file(dev, &dev_attr_utilization_stats);
#endif /* CONFIG_CPU_THERMAL_IPA */
#endif /* CONFIG_MALI_DVFS */
	device_remove_file(dev, &dev_attr_debug_level);
#ifdef CONFIG_MALI_EXYNOS_TRACE
	device_remove_file(dev, &dev_attr_trace_level);
	device_remove_file(dev, &dev_attr_trace_dump);
#endif /* CONFIG_MALI_EXYNOS_TRACE */
#ifdef DEBUG_FBDEV
	device_remove_file(dev, &dev_attr_fbdev);
#endif
	device_remove_file(dev, &dev_attr_gpu_status);
#ifdef CONFIG_MALI_SEC_VK_BOOST
	device_remove_file(dev, &dev_attr_vk_boost_status);
#endif
#ifdef CONFIG_MALI_SUSTAINABLE_OPT
	device_remove_file(dev, &dev_attr_sustainable_status);
#endif
#ifdef CONFIG_MALI_SEC_CL_BOOST
	device_remove_file(dev, &dev_attr_cl_boost_disable);
#endif
#ifdef CONFIG_MALI_DEBUG_KERNEL_SYSFS
	kobject_put(external_kobj);
#endif
}
