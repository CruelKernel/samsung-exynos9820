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

#include <linux/notifier.h>
#include <linux/thermal.h>

#include <gpex_utils.h>
#include <gpex_thermal.h>
#include <gpex_clock.h>
#include <gpexbe_debug.h>

struct _thermal_info {
	bool tmu_enabled;
};

static struct _thermal_info thermal;

void gpex_thermal_set_status(bool status)
{
	thermal.tmu_enabled = status;
}

int gpex_thermal_gpu_normal()
{
	int ret = 0;
	ret = gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, TMU_LOCK, 0);

	if (ret) {
		/* TODO: couldn't handle thermal throttling. print error log */
		;
	}

	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_TMU_VALUE, 0u, event,
			 "tmu event normal, remove gpu max lock\n");

	return ret;
}

int gpex_thermal_gpu_throttle(int freq)
{
	int ret = 0;

	if (!thermal.tmu_enabled) {
		ret = gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, TMU_LOCK, 0);
		/* TODO: print warning that gpu must be throttled, but gpu thermal is disabled */
		return ret;
	}

	ret = gpex_clock_lock_clock(GPU_CLOCK_MAX_LOCK, TMU_LOCK, freq);

	if (ret) {
		/* TODO: couldn't handle thermal throttling. print error log */
		;
	}

	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_TMU_VALUE, 0u, event,
			 "tmu event throttling, frequency %d\n", freq);

	gpexbe_debug_dbg_snapshot_thermal(freq);

	return ret;
}

/***********************************************************************
 * SYSFS FUNCTIONS
 ***********************************************************************/
static ssize_t show_tmu(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", thermal.tmu_enabled);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_tmu);

static ssize_t set_tmu_control(const char *buf, size_t count)
{
	if (sysfs_streq("0", buf)) {
		gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, TMU_LOCK, 0);
		thermal.tmu_enabled = false;
	} else if (sysfs_streq("1", buf))
		thermal.tmu_enabled = true;
	else
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value - only [0 or 1] is available\n",
			__func__);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_tmu_control);

static ssize_t show_kernel_sysfs_gpu_temp(char *buf)
{
	ssize_t ret = 0;
	int err = 0;
	int gpu_temp = 0;
	int gpu_temp_int = 0;
	int gpu_temp_point = 0;
	static struct thermal_zone_device *tz;

	/* TODO: move thermal_zone related funcs to backend */
	if (!tz)
		tz = thermal_zone_get_zone_by_name("G3D");

	err = thermal_zone_get_temp(tz, &gpu_temp);
	if (err) {
		GPU_LOG(MALI_EXYNOS_ERROR, "Error reading temp of gpu thermal zone: %d\n", err);
		return -ENODEV;
	}

	gpu_temp_int = gpu_temp / 1000;
	gpu_temp_point = gpu_temp % gpu_temp_int;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d.%d", gpu_temp_int, gpu_temp_point);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_gpu_temp);

static void gpex_thermal_create_sysfs_file()
{
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(tmu, show_tmu, set_tmu_control);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_tmu, show_kernel_sysfs_gpu_temp);
}

/***********************************************************************
 * INIT, TERM FUNCTIONS
 ***********************************************************************/
int gpex_thermal_init()
{
	gpex_thermal_create_sysfs_file();

	return 0;
}

void gpex_thermal_term()
{
	thermal.tmu_enabled = false;

	/* TODO: unregister tmu notifier */
}
