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

#include <linux/notifier.h>
#include <soc/samsung/tmu.h>
#include "gpexbe_notifier_internal.h"

/***********************************************
 * 9810 don't have gpu clock setting notifiers
 **********************************************/

int gpexbe_notifier_internal_add(gpex_notifier_type type, struct notifier_block *nb)
{
	if (type == GPU_NOTIFIER_THERMAL)
		return exynos_gpu_add_notifier(nb);

	return 0;
}

int gpexbe_notifier_internal_remove(gpex_notifier_type type, struct notifier_block *nb)
{
	return 0;
}
