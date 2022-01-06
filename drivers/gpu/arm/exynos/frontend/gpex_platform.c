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

#include <gpex_platform.h>
#include <gpex_utils.h>
#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_qos.h>
#include <gpex_thermal.h>
#include <gpex_clock.h>
#include <gpex_ifpo.h>
#include <gpex_tsg.h>
#include <gpex_clboost.h>
#include <gpexbe_devicetree.h>

#include <gpexbe_notifier.h>
#include <gpexbe_pm.h>
#include <gpexbe_clock.h>
#include <gpexbe_qos.h>
#include <gpexbe_bts.h>
#include <gpexbe_debug.h>
#include <gpexbe_utilization.h>
#include <gpexbe_llc_coherency.h>
#include <gpexbe_mem_usage.h>
#include <gpexbe_smc.h>
#include <gpex_gts.h>

#include <runtime_test_runner.h>

static struct exynos_context platform;

struct exynos_context *gpex_platform_get_context()
{
	return &platform;
}

struct exynos_context *gpex_platform_init(struct device **dev)
{
	memset(&platform, 0, sizeof(struct exynos_context));

	/* TODO: check return value */
	/* TODO: becareful with order */
	gpexbe_devicetree_init(*dev);
	gpex_utils_init(dev);

	gpexbe_utilization_init(dev);
	gpex_clboost_init();

	gpex_gts_init(dev);

	gpexbe_debug_init();

	gpex_thermal_init();
	gpexbe_notifier_init();

	gpexbe_llc_coherency_init(dev);

	gpexbe_pm_init();
	gpexbe_clock_init();
	gpex_pm_init();
	gpex_clock_init(dev);

	gpexbe_qos_init();
	gpexbe_bts_init();
	gpex_qos_init();

	gpex_ifpo_init();
	gpex_dvfs_init(dev);
	gpexbe_smc_init();
	gpex_tsg_init(dev);

	gpexbe_mem_usage_init();

	runtime_test_runner_init();

	gpex_utils_sysfs_kobject_files_create();
	gpex_utils_sysfs_device_files_create();

	return &platform;
}

void gpex_platform_term()
{
	runtime_test_runner_term();

	gpexbe_mem_usage_term();

	gpex_tsg_term();
	gpexbe_smc_term();
	gpex_ifpo_term();

	gpex_pm_term();
	gpexbe_pm_term();

	gpex_qos_term();
	gpexbe_qos_term();
	gpexbe_bts_term();

	/* DVFS stuff */
	gpex_dvfs_term();

	gpex_clock_term();
	gpexbe_clock_term();

	gpexbe_llc_coherency_term();

	gpexbe_notifier_term();
	gpex_thermal_term();

	gpexbe_debug_term();
	gpex_gts_term();

	gpex_clboost_term();
	gpexbe_utilization_term();
	gpex_utils_term();

	memset(&platform, 0, sizeof(struct exynos_context));
}
