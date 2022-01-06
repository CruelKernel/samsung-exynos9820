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

#include <linux/suspend.h>
#include <linux/pm_runtime.h>

#if IS_ENABLED(CONFIG_EXYNOS_PMU)
#include <soc/samsung/exynos-pmu.h>
#endif
#if IS_ENABLED(CONFIG_EXYNOS_PMU_IF)
#include <soc/samsung/exynos-pmu-if.h>
#endif

#include <gpex_ifpo.h>
#include <gpex_dvfs.h>
#include <gpex_clock.h>
#include <gpex_qos.h>
#include <gpex_utils.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_pm.h>
#include <gpex_pm.h>

#include <gpexbe_secure.h>
#include <gpexbe_smc.h>

#include <gpex_tsg.h>
#include <gpex_clboost.h>

#include <gpexwa_wakeup_clock.h>

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_ARM) && IS_ENABLED(CONFIG_SOC_S5E8825)
#include <soc/samsung/exynos-smc.h>
#endif

struct pm_info {
	int runtime_pm_delay_time;
	bool power_status;
	int state;
	bool skip_auto_suspend;
	struct device *dev;
};

static struct pm_info pm;

int gpex_pm_set_state(int state)
{
	if (GPEX_PM_STATE_START <= state && state <= GPEX_PM_STATE_END) {
		pm.state = state;
		GPU_LOG(MALI_EXYNOS_INFO, "gpex_pm: gpex_pm state is set as 0x%X", pm.state);
		return 0;
	}

	GPU_LOG(MALI_EXYNOS_WARNING,
		"gpex_pm: Attempted to set gpex_pm state with invalid value 0x%x", state);
	return -1;
}

int gpex_pm_get_state(int *state)
{
	if (GPEX_PM_STATE_START <= pm.state && pm.state <= GPEX_PM_STATE_END) {
		*state = pm.state;
		return 0;
	}

	GPU_LOG(MALI_EXYNOS_WARNING, "gpex_pm: gpex_pm has invalid state now 0x%x", pm.state);
	return -1;
}

int gpex_pm_get_status(bool clock_lock)
{
	int ret = 0;
	//unsigned int val = 0;

	if (clock_lock)
		gpex_clock_mutex_lock();

	ret = gpexbe_pm_get_status();

	if (clock_lock)
		gpex_clock_mutex_unlock();

	return ret;
}

/* Read the power_status value set by gpex_pm module */
int gpex_pm_get_power_status()
{
	return pm.power_status;
}

void gpex_pm_lock()
{
	gpexbe_pm_access_lock();
}

void gpex_pm_unlock()
{
	gpexbe_pm_access_unlock();
}

static ssize_t show_power_state(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_pm_get_status(true));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_power_state);

/******************************
 * RTPM callback functions
 * ***************************/
int gpex_pm_power_on(struct device *dev)
{
	int ret = 0;

	pm.skip_auto_suspend = false;

#if IS_ENABLED(CONFIG_MALI_EXYNOS_BLOCK_RPM_WHILE_SUSPEND_RESUME)
	if (pm.state == GPEX_PM_STATE_RESUME_BEGIN && gpex_pm_get_status(false) > 0) {
		pm.skip_auto_suspend = true;
	} else {
		ret = pm_runtime_get_sync(dev);
	}
#else
	ret = pm_runtime_get_sync(dev);
#endif

	if (ret >= 0) {
		gpex_ifpo_power_up();
		GPU_LOG_DETAILED(MALI_EXYNOS_INFO, LSI_GPU_RPM_RESUME_API, ret, 0u, "power on\n");
	} else {
		GPU_LOG(MALI_EXYNOS_ERROR, "runtime pm returned %d\n", ret);
	}

	gpex_dvfs_start();

	return ret;
}

void gpex_pm_power_autosuspend(struct device *dev)
{
	int ret = 0;

	gpex_ifpo_power_down();

	if (!pm.skip_auto_suspend) {
		pm_runtime_mark_last_busy(dev);
		ret = pm_runtime_put_autosuspend(dev);
	}

	GPU_LOG_DETAILED(MALI_EXYNOS_INFO, LSI_GPU_RPM_SUSPEND_API, ret, 0u,
			 "power autosuspend prepare\n");
}

void gpex_pm_suspend(struct device *dev)
{
	int ret = 0;

	gpexwa_wakeup_clock_suspend();
	gpex_qos_set_from_clock(0);
	ret = pm_runtime_suspend(dev);

	GPU_LOG_DETAILED(MALI_EXYNOS_INFO, LSI_SUSPEND_CALLBACK, ret, 0u, "power suspend\n");
}

static struct delayed_work gpu_poweroff_delay_set_work;

static void gpu_poweroff_delay_recovery_callback(struct work_struct *data)
{
	if (!pm.dev->power.use_autosuspend)
		return;

	device_lock(pm.dev);
	pm_runtime_set_autosuspend_delay(pm.dev, pm.runtime_pm_delay_time);
	device_unlock(pm.dev);

	gpex_clock_set_user_min_lock_input(0);
	gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, SYSFS_LOCK, 0);
	GPU_LOG(MALI_EXYNOS_DEBUG, "gpu poweroff delay recovery done & clock min unlock\n");

	gpex_clboost_set_state(CLBOOST_ENABLE);
}

static int gpu_poweroff_delay_recovery(unsigned int period)
{
#define POWEROFF_DELAY_MAX_PERIOD 1500
#define POWEROFF_DELAY_MIN_PERIOD 50

	/* boundary */
	if (period > POWEROFF_DELAY_MAX_PERIOD)
		period = POWEROFF_DELAY_MAX_PERIOD;
	else if (period < POWEROFF_DELAY_MIN_PERIOD)
		period = POWEROFF_DELAY_MIN_PERIOD;

	GPU_LOG(MALI_EXYNOS_DEBUG, "gpu poweroff delay set wq start(%u)\n", period);

	cancel_delayed_work_sync(&gpu_poweroff_delay_set_work);
	schedule_delayed_work(&gpu_poweroff_delay_set_work, msecs_to_jiffies(period));

	return 0;
}

static ssize_t set_sysfs_poweroff_delay(const char *buf, size_t count)
{
	long delay;

	if (!pm.dev->power.use_autosuspend)
		return -EIO;

	if (kstrtol(buf, 10, &delay) != 0 || delay != (int)delay)
		return -EINVAL;

	if (delay < pm.runtime_pm_delay_time)
		delay = pm.runtime_pm_delay_time;

	device_lock(pm.dev);
	pm_runtime_set_autosuspend_delay(pm.dev, delay);
	device_unlock(pm.dev);

	gpu_poweroff_delay_recovery((unsigned int)delay);

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_sysfs_poweroff_delay)

static ssize_t show_sysfs_poweroff_delay(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", pm.runtime_pm_delay_time);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_sysfs_poweroff_delay)

static void gpu_poweroff_delay_wq_init(void)
{
	INIT_DELAYED_WORK(&gpu_poweroff_delay_set_work, gpu_poweroff_delay_recovery_callback);
}

static void gpu_poweroff_delay_wq_deinit(void)
{
	cancel_delayed_work_sync(&gpu_poweroff_delay_set_work);
}

int gpex_pm_runtime_init(struct device *dev)
{
	int ret = 0;

	pm_runtime_set_autosuspend_delay(dev, pm.runtime_pm_delay_time);
	pm_runtime_use_autosuspend(dev);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	gpu_poweroff_delay_wq_init();

	if (!pm_runtime_enabled(dev)) {
		dev_warn(dev, "pm_runtime not enabled");
		ret = -ENOSYS;
	}

	return ret;
}

void gpex_pm_runtime_term(struct device *dev)
{
	pm_runtime_disable(dev);

	gpu_poweroff_delay_wq_deinit();
}

int gpex_pm_runtime_on_prepare(struct device *dev)
{
	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_ON, 0u, 0u, "runtime on callback\n");

	pm.power_status = true;

	gpexbe_smc_notify_power_on();

	gpexwa_wakeup_clock_restore();

	return 0;
}

/* TODO: 9830 need to store and restore clock before and after power off/on */
#if 0
int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	struct exynos_context *platform = (struct exynos_context *) kbdev->platform_context;
	if (!platform)
		return -ENODEV;

	GPU_LOG(MALI_EXYNOS_DEBUG, LSI_GPU_ON, 0u, 0u, "runtime on callback\n");

	platform->power_status = true;

	/* Set clock - restore previous g3d clock, after g3d runtime on */
	if (gpex_dvfs_get_status() && platform->wakeup_lock) {
		if (platform->restore_clock > G3D_DVFS_MIDDLE_CLOCK) {
			gpex_clock_set(platform->restore_clock);
			GPU_LOG(MALI_EXYNOS_DEBUG, LSI_GPU_ON, platform->restore_clock, gpex_clock_get_cur_clock(), "runtime on callback - restore clock = %d, cur clock = %d\n", platform->restore_clock, gpex_clock_get_cur_clock());
			platform->restore_clock = 0;
		}
	}
	return 0;
}
#endif

void gpex_pm_runtime_off_prepare(struct device *dev)
{
	CSTD_UNUSED(dev);
	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_GPU_OFF, 0u, 0u, "runtime off callback\n");

	gpexbe_smc_notify_power_off();

	/* power up from ifpo down state before going to full rtpm power off */
	gpex_ifpo_power_up();
	gpex_tsg_reset_count(0);
	gpex_dvfs_stop();

	gpex_clock_prepare_runtime_off();
	gpexwa_wakeup_clock_set();
	gpex_qos_set_from_clock(0);

	pm.power_status = false;
}

int gpex_pm_init(void)
{
	pm.power_status = true;

	pm.runtime_pm_delay_time = gpexbe_devicetree_get_int(gpu_runtime_pm_delay_time);

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(power_state, show_power_state);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_poweroff_delay, show_sysfs_poweroff_delay,
					  set_sysfs_poweroff_delay);

	pm.state = GPEX_PM_STATE_START;
	pm.skip_auto_suspend = false;

	pm.dev = gpex_utils_get_device();

	return 0;
}

void gpex_pm_term(void)
{
	memset(&pm, 0, sizeof(struct pm_info));

	return;
}
