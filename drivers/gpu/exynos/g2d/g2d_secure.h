/*
 * linux/drivers/gpu/exynos/g2d/g2d_secure.h
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * Samsung Graphics 2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __EXYNOS_G2D_SECURE_H__
#define __EXYNOS_G2D_SECURE_H__

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
#include <linux/smc.h>
#else
static inline int exynos_smc(unsigned long cmd, unsigned long arg1,
			     unsigned long arg2, unsigned long arg3)
{
	return 0; /* DRMDRV_OK */
}
#endif

/*
 * SMC_DRM_G2D_CMD_DATA should be defined in <linux/smc.h> before use of it.
 * However, it may not be defined for the project that does not need it including
 * Exynos9820.
 */
#ifndef SMC_DRM_G2D_CMD_DATA
#define SMC_DRM_G2D_CMD_DATA            0x8200202d
#endif

/*
 * SMC_PROTECTION_SET is defined in <linux/smc.h>. This is just a hedge of the
 * situation when <linux/smc.h> forgets to define SMC_PROTECTION_SET.
 */
#ifndef SMC_PROTECTION_SET
#define SMC_PROTECTION_SET		0x82002010
#endif

#ifndef G2D_ALWAYS_S
#define G2D_ALWAYS_S 37
#endif

#endif /* __EXYNOS_G2D_SECURE_H__ */
