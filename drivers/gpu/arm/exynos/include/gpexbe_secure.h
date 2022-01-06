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

#ifndef _GPEXBE_SECURE_H_
#define _GPEXBE_SECURE_H_

struct protected_mode_ops;
struct kbase_device;

struct protected_mode_ops *gpexbe_secure_get_protected_mode_ops(void);

int gpexbe_secure_legacy_jm_enter_protected_mode(struct kbase_device *kbdev);
int gpexbe_secure_legacy_jm_exit_protected_mode(struct kbase_device *kbdev);
int gpexbe_secure_legacy_pm_exit_protected_mode(struct kbase_device *kbdev);

#endif /* _GPEXBE_SECURE_H_ */
