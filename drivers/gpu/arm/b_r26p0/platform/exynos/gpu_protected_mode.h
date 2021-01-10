/*
 *
 * (C) COPYRIGHT 2020 Samsung Electronics Inc. All rights reserved.
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
 */

#ifndef _GPU_PROTECTED_MODE_H_
#define _GPU_PROTECTED_MODE_H_

#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_LEGACY)
#include <mali_kbase.h>

int exynos_secure_mode_enable(struct protected_mode_device *pdev);

int exynos_secure_mode_disable(struct protected_mode_device *pdev);

int kbase_jm_enter_protected_mode_exynos(struct kbase_device *kbdev,
		struct kbase_jd_atom **katom, int idx, int js);

int kbase_jm_exit_protected_mode_exynos(struct kbase_device *kbdev,
		struct kbase_jd_atom **katom, int idx, int js);

int kbase_protected_mode_disable_exynos(struct kbase_device *kbdev);
#endif /* CONFIG_MALI_EXYNOS_SECURE_RENDERING_LEGACY */

#endif /* _GPU_PROTECTED_MODE_H_ */
