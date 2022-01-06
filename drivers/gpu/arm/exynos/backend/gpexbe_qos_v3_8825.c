/*
 *
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
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 *//* SPDX-License-Identifier: GPL-2.0 */

#include <soc/samsung/exynos_pm_qos.h>
#include <soc/samsung/freq-qos-tracer.h>

#include <gpexbe_qos.h>
#include "gpexbe_qos_internal.h"

static struct exynos_pm_qos_request mif_min_qos;
static struct freq_qos_request cpu_cluster0_min_qos;
static struct cpufreq_policy *cpu_cluster0_policy;

/* Functions to set CPU & MIF pmqos from Mali */

void gpexbe_qos_request_add(mali_pmqos_flags type)
{
	if (pmqos_flag_check(type, PMQOS_MIF | PMQOS_MIN))
		exynos_pm_qos_add_request(&mif_min_qos, PM_QOS_BUS_THROUGHPUT, 0);

	if (pmqos_flag_check(type, PMQOS_LITTLE | PMQOS_MIN)) {
		cpu_cluster0_policy = cpufreq_cpu_get(0);
		if (cpu_cluster0_policy) {
			freq_qos_tracer_add_request(&cpu_cluster0_policy->constraints,
						    &cpu_cluster0_min_qos, FREQ_QOS_MIN, 0);
			cpufreq_cpu_put(cpu_cluster0_policy);
		} else {
			pr_err("mali: cpu_cluster0_policy is NULL");
		}
	}
}

void gpexbe_qos_request_remove(mali_pmqos_flags type)
{
	if (pmqos_flag_check(type, PMQOS_MIF | PMQOS_MIN))
		exynos_pm_qos_remove_request(&mif_min_qos);

	if (pmqos_flag_check(type, PMQOS_LITTLE | PMQOS_MIN))
		freq_qos_tracer_remove_request(&cpu_cluster0_min_qos);
}

void gpexbe_qos_request_update(mali_pmqos_flags type, s32 clock)
{
	if (pmqos_flag_check(type, PMQOS_MIF | PMQOS_MIN))
		exynos_pm_qos_update_request(&mif_min_qos, clock);

	if (pmqos_flag_check(type, PMQOS_LITTLE | PMQOS_MIN))
		freq_qos_update_request(&cpu_cluster0_min_qos, clock);
}

void gpexbe_qos_request_unset(mali_pmqos_flags type)
{
	if (pmqos_flag_check(type, PMQOS_MIF | PMQOS_MIN))
		exynos_pm_qos_update_request(&mif_min_qos, 0);

	if (pmqos_flag_check(type, PMQOS_LITTLE | PMQOS_MIN))
		freq_qos_update_request(&cpu_cluster0_min_qos, 0);
}
