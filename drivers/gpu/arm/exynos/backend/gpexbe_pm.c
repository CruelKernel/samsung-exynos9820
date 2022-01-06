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

#include <errno.h>

/* TODO: separate cal-if and pmu-if dependent stuff */
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-pd.h>
#include <linux/of.h>

#if IS_ENABLED(CONFIG_EXYNOS_PMU)
#include <soc/samsung/exynos-pmu.h>
#endif
#if IS_ENABLED(CONFIG_EXYNOS_PMU_IF)
#include <soc/samsung/exynos-pmu-if.h>
#endif

#include <gpex_utils.h>
#include <gpexbe_devicetree.h>

#include <gpexbe_pm.h>

static struct exynos_pm_domain *exynos_pm_domain;
static unsigned int gpu_pmu_status_reg_offset;
static unsigned int gpu_pmu_status_local_pwr_mask;

static struct exynos_pm_domain *gpu_get_pm_domain(const char *g3d_genpd_name)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;
	struct exynos_pm_domain *pd_temp, *pd = NULL;

	for_each_compatible_node (np, NULL, "samsung,exynos-pd") {
		if (!of_device_is_available(np))
			continue;

		pdev = of_find_device_by_node(np);
		pd_temp = (struct exynos_pm_domain *)platform_get_drvdata(pdev);
		if (!strcmp(g3d_genpd_name, (const char *)(pd_temp->genpd.name))) {
			pd = pd_temp;
			break;
		}
	}

	if (pd == NULL)
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: g3d pm_domain is null\n", __func__);

	return pd;
}

int gpexbe_pm_get_status()
{
	int ret = 0;
	unsigned int val = 0xf;

	exynos_pmu_read(gpu_pmu_status_reg_offset, &val);

	ret = ((val & gpu_pmu_status_local_pwr_mask) == gpu_pmu_status_local_pwr_mask) ? 1 : 0;

	return ret;
}
struct exynos_pm_domain *gpexbe_pm_get_exynos_pm_domain()
{
	return exynos_pm_domain;
}

void gpexbe_pm_access_lock()
{
	//DEBUG_ASSERT(exynos_pm_domain)
	mutex_lock(&exynos_pm_domain->access_lock);
}

void gpexbe_pm_access_unlock()
{
	//DEBUG_ASSERT(exynos_pm_domain)
	mutex_unlock(&exynos_pm_domain->access_lock);
}

static int gpexbe_pm_pd_control(int target_status)
{
	int status;

	gpexbe_pm_access_lock();

	status = cal_pd_status(exynos_pm_domain->cal_pdid);
	if (status == target_status) {
		gpexbe_pm_access_unlock();
		GPU_LOG(MALI_EXYNOS_DEBUG,
			"%s: status checking : Already gpu inter frame power %d\n", __func__,
			target_status);
		return 0;
	}

	if (cal_pd_control(exynos_pm_domain->cal_pdid, target_status) != 0) {
		gpexbe_pm_access_unlock();
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: failed to gpu inter frame power %d\n", __func__,
			target_status);
		return -1;
	}

	status = cal_pd_status(exynos_pm_domain->cal_pdid);
	if (status != target_status) {
		gpexbe_pm_access_unlock();
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: status error : gpu inter frame power %d\n",
			__func__, target_status);
		return -1;
	}

	gpexbe_pm_access_unlock();
	GPU_LOG_DETAILED(MALI_EXYNOS_DEBUG, LSI_IFPM_POWER_ON, 0u, 0u,
			 "gpu inter frame power on\n");

	return 0;
}

int gpexbe_pm_pd_control_up()
{
	return gpexbe_pm_pd_control(1);
}

int gpexbe_pm_pd_control_down()
{
	return gpexbe_pm_pd_control(0);
}

int gpexbe_pm_init()
{
	const char *g3d_genpd_name;

	g3d_genpd_name = gpexbe_devicetree_get_str(g3d_genpd_name);
	exynos_pm_domain = gpu_get_pm_domain(g3d_genpd_name);

	gpu_pmu_status_reg_offset = gpexbe_devicetree_get_int(gpu_pmu_status_reg_offset);
	gpu_pmu_status_local_pwr_mask = gpexbe_devicetree_get_int(gpu_pmu_status_local_pwr_mask);

	if (!exynos_pm_domain)
		return -EINVAL;

	return 0;
}

void gpexbe_pm_term()
{
	exynos_pm_domain = NULL;
}
