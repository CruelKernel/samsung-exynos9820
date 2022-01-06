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

#ifndef _GPEXBE_NOTIFIER_INTERNAL_H_
#define _GPEXBE_NOTIFIER_INTERNAL_H_

#include <linux/notifier.h>

typedef enum {
	GPU_NOTIFIER_MIN_LOCK = 0,
	GPU_NOTIFIER_MAX_LOCK,
	GPU_NOTIFIER_THERMAL,
} gpex_notifier_type;

int gpexbe_notifier_internal_add(gpex_notifier_type type, struct notifier_block *nb);
int gpexbe_notifier_internal_remove(gpex_notifier_type type, struct notifier_block *nb);

#endif /* _GPEXBE_NOTIFIER_INTERNAL_H_ */
