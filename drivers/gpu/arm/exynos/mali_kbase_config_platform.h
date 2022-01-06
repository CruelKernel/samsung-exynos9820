/*
 *
 * (C) COPYRIGHT 2014-2015 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#ifndef _MALI_KBASE_CONFIG_PLATFORM_H_
#define _MALI_KBASE_CONFIG_PLATFORM_H_

#include "mali_exynos_kbase_entrypoint.h"

/**
 * Power management configuration
 *
 * Attached value: pointer to @ref kbase_pm_callback_conf
 * Default value: See @ref kbase_pm_callback_conf
 */
#if IS_ENABLED(CONFIG_MALI_EXYNOS_RTPM)
#define POWER_MANAGEMENT_CALLBACKS (&pm_callbacks)
extern struct kbase_pm_callback_conf pm_callbacks;
#else
#define POWER_MANAGEMENT_CALLBACKS (NULL)
#endif

/**
 * Platform specific configuration functions
 *
 * Attached value: pointer to @ref kbase_platform_funcs_conf
 * Default value: See @ref kbase_platform_funcs_conf
 */
/* MALI_SEC_INTEGRATION */
#define PLATFORM_FUNCS (&platform_funcs)

/**
 * Platform specific callback functions for entering protected mode
 */
#if IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_LEGACY) ||                                      \
	IS_ENABLED(CONFIG_MALI_EXYNOS_SECURE_RENDERING_ARM)
#define PLATFORM_PROTECTED_CALLBACKS mali_exynos_get_protected_ops()
#endif

extern struct kbase_platform_funcs_conf platform_funcs;

#endif /* _MALI_KBASE_CONFIG_PLATFORM_H_ */
