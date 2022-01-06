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

#include <gpexbe_pm.h>

struct exynos_pm_domain {
	int dummy;
};

static struct exynos_pm_domain dummy_pm_domain;

int gpexbe_pm_init(void)
{
	return 0;
}

void gpexbe_pm_term(void)
{
	return;
}

void gpexbe_pm_access_lock(void)
{
	return;
}

void gpexbe_pm_access_unlock(void)
{
	return;
}

int gpexbe_pm_pd_control_up(void)
{
	return 0;
}

int gpexbe_pm_pd_control_down(void)
{
	return 0;
}

int gpexbe_pm_get_status(void)
{
	return 0;
}

struct exynos_pm_domain *gpexbe_pm_get_exynos_pm_domain(void)
{
	return &dummy_pm_domain;
}
