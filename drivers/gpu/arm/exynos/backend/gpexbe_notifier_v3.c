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
#include <soc/samsung/exynos_pm_qos.h>

#include "gpexbe_notifier_internal.h"

/*******************************************************************************
 * 2100 don't have thermal notifier. Instead calls thermal function directly...
 *******************************************************************************/

static int translate_notifier_type(gpex_notifier_type type)
{
	switch (type) {
	case GPU_NOTIFIER_MIN_LOCK:
		return PM_QOS_GPU_THROUGHPUT_MIN;
	case GPU_NOTIFIER_MAX_LOCK:
		return PM_QOS_GPU_THROUGHPUT_MAX;
	default:
		return -1;
	}

	return -1;
}

int gpexbe_notifier_internal_add(gpex_notifier_type type, struct notifier_block *nb)
{
	int external_pmqos_type;

	external_pmqos_type = translate_notifier_type(type);
	if (external_pmqos_type >= 0)
		return exynos_pm_qos_add_notifier(external_pmqos_type, nb);

	return 0;
}

int gpexbe_notifier_internal_remove(gpex_notifier_type type, struct notifier_block *nb)
{
	int external_pmqos_type;

	external_pmqos_type = translate_notifier_type(type);
	if (external_pmqos_type >= 0)
		return exynos_pm_qos_remove_notifier(external_pmqos_type, nb);

	return 0;
}
