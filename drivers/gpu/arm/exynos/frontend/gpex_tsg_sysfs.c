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

#include <gpex_tsg.h>
#include <gpex_utils.h>

#include "gpex_tsg_internal.h"

static struct _tsg_info *tsg_info;

static ssize_t show_weight_table_idx_0(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_weight_table_idx(0));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_weight_table_idx_0);

static ssize_t show_weight_table_idx_1(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_weight_table_idx(1));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_weight_table_idx_1);

static ssize_t show_queued_threshold_0(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_queued_threshold(0));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_queued_threshold_0);

static ssize_t show_queued_threshold_1(char *buf)
{
	ssize_t ret = 0;
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", gpex_tsg_get_queued_threshold(1));

	if (ret < PAGE_SIZE - 1) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		ret = PAGE_SIZE - 1;
	}

	return ret;
}
CREATE_SYSFS_DEVICE_READ_FUNCTION(show_queued_threshold_1);

static ssize_t set_weight_table_idx_0(const char *buf, size_t count)
{
	int ret, table_idx_0;
	ret = kstrtoint(buf, 0, &table_idx_0);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_weight_table_idx(0, table_idx_0);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_weight_table_idx_0);

static ssize_t set_weight_table_idx_1(const char *buf, size_t count)
{
	int ret, table_idx_1;
	ret = kstrtoint(buf, 0, &table_idx_1);

	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_weight_table_idx(1, table_idx_1);

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_weight_table_idx_1);

static ssize_t set_queued_threshold_0(const char *buf, size_t count)
{
	unsigned int threshold = 0;
	int ret;

	ret = kstrtoint(buf, 0, &threshold);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}
	gpex_tsg_set_queued_threshold(0, threshold);

	GPU_LOG(MALI_EXYNOS_ERROR, "%s: queued_threshold_0 = %d\n", __func__,
		gpex_tsg_get_queued_threshold(0));

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_queued_threshold_0);

static ssize_t set_queued_threshold_1(const char *buf, size_t count)
{
	unsigned int threshold = 0;
	int ret;

	ret = kstrtoint(buf, 0, &threshold);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	gpex_tsg_set_queued_threshold(1, threshold);

	GPU_LOG(MALI_EXYNOS_ERROR, "%s: queued_threshold_1 = %d\n", __func__,
		gpex_tsg_get_queued_threshold(1));

	return count;
}
CREATE_SYSFS_DEVICE_WRITE_FUNCTION(set_queued_threshold_1);

int gpex_tsg_sysfs_init(struct _tsg_info *_tsg_info)
{
	tsg_info = _tsg_info;

	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(weight_table_idx_0, show_weight_table_idx_0,
					 set_weight_table_idx_0);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(weight_table_idx_1, show_weight_table_idx_1,
					 set_weight_table_idx_1);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(queued_threshold_0, show_queued_threshold_0,
					 set_queued_threshold_0);
	GPEX_UTILS_SYSFS_DEVICE_FILE_ADD(queued_threshold_1, show_queued_threshold_1,
					 set_queued_threshold_1);

	return 0;
}

int gpex_tsg_sysfs_term(void)
{
	tsg_info = (void *)0;

	return 0;
}