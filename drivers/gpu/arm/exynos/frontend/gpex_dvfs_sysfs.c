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

#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_clock.h>
#include <gpex_utils.h>

#include "gpex_dvfs_internal.h"
#include "gpu_dvfs_governor.h"

static struct dvfs_info *dvfs;

static int gpu_dvfs_governor_change(int governor_type)
{
	mutex_lock(&dvfs->handler_lock);
	gpu_dvfs_governor_setting(governor_type);
	mutex_unlock(&dvfs->handler_lock);

	return 0;
}

static int gpu_get_dvfs_table(char *buf, size_t buf_size)
{
	int i, cnt = 0;

	if (buf == NULL)
		return 0;

	for (i = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	     i <= gpex_clock_get_table_idx(gpex_clock_get_min_clock()); i++)
		cnt += snprintf(buf + cnt, buf_size - cnt, " %d", dvfs->table[i].clock);

	cnt += snprintf(buf + cnt, buf_size - cnt, "\n");

	return cnt;
}

static ssize_t show_dvfs_table(char *buf)
{
	ssize_t ret = 0;

	ret += gpu_get_dvfs_table(buf + ret, (size_t)PAGE_SIZE - ret);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_dvfs_table);

static ssize_t show_dvfs(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", dvfs->status);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_dvfs);

static ssize_t set_dvfs(const char *buf, size_t count)
{
	if (sysfs_streq("0", buf))
		gpex_dvfs_disable();
	else if (sysfs_streq("1", buf))
		gpex_dvfs_enable();

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_dvfs);

static ssize_t show_governor(char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info;
	int i;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s\n", governor_info[i].name);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "[Current Governor] %s",
			governor_info[dvfs->governor_type].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_governor);

static ssize_t set_governor(const char *buf, size_t count)
{
	int ret;
	int next_governor_type;

	ret = kstrtoint(buf, 0, &next_governor_type);

	if ((next_governor_type < 0) || (next_governor_type >= G3D_MAX_GOVERNOR_NUM)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_governor_change(next_governor_type);

	if (ret < 0) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: fail to set the new governor (%d)\n", __func__,
			next_governor_type);
		return -ENOENT;
	}

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_governor);

static ssize_t show_down_staycount(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int i = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	for (i = gpex_clock_get_table_idx(gpex_clock_get_max_clock());
	     i <= gpex_clock_get_table_idx(gpex_clock_get_min_clock()); i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "Clock %d - %d\n", dvfs->table[i].clock,
				dvfs->table[i].down_staycount);
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_down_staycount);

#define MIN_DOWN_STAYCOUNT 1
#define MAX_DOWN_STAYCOUNT 10
static ssize_t set_down_staycount(const char *buf, size_t count)
{
	unsigned long flags;
	char tmpbuf[32];
	char *sptr, *tok;
	int ret = -1;
	int clock = -1, level = -1, down_staycount = 0;
	unsigned int len = 0;

	len = (unsigned int)min(count, sizeof(tmpbuf) - 1);
	memcpy(tmpbuf, buf, len);
	tmpbuf[len] = '\0';
	sptr = tmpbuf;

	tok = strsep(&sptr, " ,");
	if (tok == NULL) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input\n", __func__);
		return -ENOENT;
	}

	ret = kstrtoint(tok, 0, &clock);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input %d\n", __func__, clock);
		return -ENOENT;
	}

	tok = strsep(&sptr, " ,");
	if (tok == NULL) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input\n", __func__);
		return -ENOENT;
	}

	ret = kstrtoint(tok, 0, &down_staycount);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid input %d\n", __func__, down_staycount);
		return -ENOENT;
	}

	level = gpex_clock_get_table_idx(clock);
	if (level < 0) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__, clock);
		return -ENOENT;
	}

	if ((down_staycount < MIN_DOWN_STAYCOUNT) || (down_staycount > MAX_DOWN_STAYCOUNT)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: down_staycount is out of range (%d, %d ~ %d)\n",
			__func__, down_staycount, MIN_DOWN_STAYCOUNT, MAX_DOWN_STAYCOUNT);
		return -ENOENT;
	}

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->table[level].down_staycount = down_staycount;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_down_staycount);

static ssize_t show_highspeed_clock(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_clock = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	highspeed_clock = dvfs->interactive.highspeed_clock;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", highspeed_clock);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_highspeed_clock);

static ssize_t set_highspeed_clock(const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_clock = -1;

	ret = kstrtoint(buf, 0, &highspeed_clock);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpex_clock_get_table_idx(highspeed_clock);
	if ((ret < gpex_clock_get_table_idx(gpex_clock_get_max_clock())) ||
	    (ret > gpex_clock_get_table_idx(gpex_clock_get_min_clock()))) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid clock value (%d)\n", __func__,
			highspeed_clock);
		return -ENOENT;
	}

	if (highspeed_clock > gpex_clock_get_max_clock_limit())
		highspeed_clock = gpex_clock_get_max_clock_limit();

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->interactive.highspeed_clock = highspeed_clock;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_highspeed_clock);

static ssize_t show_highspeed_load(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_load = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	highspeed_load = dvfs->interactive.highspeed_load;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", highspeed_load);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_highspeed_load);

static ssize_t set_highspeed_load(const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_load = -1;

	ret = kstrtoint(buf, 0, &highspeed_load);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((highspeed_load < 0) || (highspeed_load > 100)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid load value (%d)\n", __func__,
			highspeed_load);
		return -ENOENT;
	}

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->interactive.highspeed_load = highspeed_load;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_highspeed_load);

static ssize_t show_highspeed_delay(char *buf)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_delay = -1;

	spin_lock_irqsave(&dvfs->spinlock, flags);
	highspeed_delay = dvfs->interactive.highspeed_delay;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", highspeed_delay);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_highspeed_delay);

static ssize_t set_highspeed_delay(const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long flags;
	int highspeed_delay = -1;

	ret = kstrtoint(buf, 0, &highspeed_delay);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if ((highspeed_delay < 0) || (highspeed_delay > 5)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid load value (%d)\n", __func__,
			highspeed_delay);
		return -ENOENT;
	}

	spin_lock_irqsave(&dvfs->spinlock, flags);
	dvfs->interactive.highspeed_delay = highspeed_delay;
	spin_unlock_irqrestore(&dvfs->spinlock, flags);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_highspeed_delay);

static ssize_t show_polling_speed(char *buf)
{
	ssize_t ret = 0;

	int polling_speed = gpex_dvfs_get_polling_speed();
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", polling_speed);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_polling_speed);

static ssize_t set_polling_speed(const char *buf, size_t count)
{
	int ret, polling_speed;

	ret = kstrtoint(buf, 0, &polling_speed);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_dvfs_set_polling_speed(polling_speed);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_polling_speed);

static ssize_t show_utilization(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d",
			gpex_pm_get_status(true) * dvfs->env_data.utilization);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_utilization);

static ssize_t show_kernel_sysfs_utilization(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%3d%%", dvfs->env_data.utilization);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_utilization)

#define BUF_SIZE 1000
static ssize_t show_kernel_sysfs_available_governor(char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info;
	int i;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s ", governor_info[i].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_available_governor)

static ssize_t show_kernel_sysfs_governor(char *buf)
{
	ssize_t ret = 0;
	gpu_dvfs_governor_info *governor_info = NULL;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%s", governor_info[dvfs->governor_type].name);

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_governor)

static ssize_t set_kernel_sysfs_governor(const char *buf, size_t count)
{
	int ret;
	int i = 0;
	int next_governor_type = -1;
	size_t governor_name_size = 0;
	gpu_dvfs_governor_info *governor_info = NULL;

	governor_info = (gpu_dvfs_governor_info *)gpu_dvfs_get_governor_info();

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++) {
		governor_name_size = strlen(governor_info[i].name);
		if (!strncmp(buf, governor_info[i].name, governor_name_size)) {
			next_governor_type = i;
			break;
		}
	}

	if ((next_governor_type < 0) || (next_governor_type >= G3D_MAX_GOVERNOR_NUM)) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	ret = gpu_dvfs_governor_change(next_governor_type);

	if (ret < 0) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: fail to set the new governor (%d)\n", __func__,
			next_governor_type);
		return -ENOENT;
	}

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_kernel_sysfs_governor)

int gpex_dvfs_sysfs_init(struct dvfs_info *_dvfs)
{
	dvfs = _dvfs;

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(dvfs, show_dvfs, set_dvfs);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(dvfs_governor, show_governor, set_governor);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(down_staycount, show_down_staycount, set_down_staycount);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(highspeed_clock, show_highspeed_clock,
					 set_highspeed_clock);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(highspeed_load, show_highspeed_load, set_highspeed_load);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(highspeed_delay, show_highspeed_delay,
					 set_highspeed_delay);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(polling_speed, show_polling_speed, set_polling_speed);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(dvfs_table, show_dvfs_table);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD_RO(utilization, show_utilization);

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_governor, show_kernel_sysfs_governor,
					  set_kernel_sysfs_governor);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_available_governor,
					     show_kernel_sysfs_available_governor);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_busy, show_kernel_sysfs_utilization);

	return 0;
}
