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

/* Implements */
#include <gpex_clboost.h>

/* Uses */
#include <gpexbe_utilization.h>
#include <gpex_utils.h>

struct _clboost_info {
	int activation_compute_ratio;
	clboost_state state;
};

static struct _clboost_info clboost_info;

int gpex_clboost_check_activation_condition(void)
{
	if (clboost_info.state == CLBOOST_DISABLE)
		return false;

	return gpexbe_utilization_get_pure_compute_time_rate() >=
	       clboost_info.activation_compute_ratio;
}

void gpex_clboost_set_state(clboost_state state)
{
	clboost_info.state = state;
}

static ssize_t sysfs_wrapup(ssize_t count, char *buf)
{
	if (count < PAGE_SIZE - 1) {
		count += snprintf(buf + count, PAGE_SIZE - count, "\n");
	} else {
		buf[PAGE_SIZE - 2] = '\n';
		buf[PAGE_SIZE - 1] = '\0';
		count = PAGE_SIZE - 1;
	}

	return count;
}

static ssize_t show_clboost_state(char *buf)
{
	ssize_t ret = 0;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "enabled(%d) active(%d) compute_ratio(%d)",
			clboost_info.state, gpex_clboost_check_activation_condition(),
			gpexbe_utilization_get_pure_compute_time_rate());

	return sysfs_wrapup(ret, buf);
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_clboost_state)

static ssize_t show_clboost_disable(char *buf)
{
	ssize_t ret = 0;
	bool clboost_disabled = false;

	if (clboost_info.state == CLBOOST_DISABLE)
		clboost_disabled = true;

	ret += snprintf(buf + ret, PAGE_SIZE - ret, "%d", clboost_disabled);

	return sysfs_wrapup(ret, buf);
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_clboost_disable)

static ssize_t set_clboost_disable(const char *buf, size_t count)
{
	unsigned int clboost_disable = 0;
	int ret;

	ret = kstrtoint(buf, 0, &clboost_disable);
	if (ret) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: invalid value\n", __func__);
		return -ENOENT;
	}

	if (clboost_disable == 0)
		clboost_info.state = CLBOOST_ENABLE;
	else
		clboost_info.state = CLBOOST_DISABLE;

	return count;
}
CREATE_SYSFS_KOBJECT_WRITE_FUNCTION(set_clboost_disable)

int gpex_clboost_init(void)
{
	clboost_info.state = CLBOOST_ENABLE;
	clboost_info.activation_compute_ratio = 100;

	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD(gpu_cl_boost_disable, show_clboost_disable,
					  set_clboost_disable);
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_cl_boost_state, show_clboost_state);

	return 0;
}

void gpex_clboost_term(void)
{
	clboost_info.state = CLBOOST_DISABLE;
	clboost_info.activation_compute_ratio = 101;
}
