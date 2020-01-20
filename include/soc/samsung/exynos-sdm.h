/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Security Dump Manager support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef EXYNOS_SDM_H
#define EXYNOS_SDM_H

#ifdef CONFIG_EXYNOS_SDM
int exynos_sdm_dump_secure_region(void);
int exynos_sdm_flush_secdram(void);
#else
#define exynos_sdm_dump_secure_region()		do { } while(0)
#define exynos_sdm_flush_secdram()		do { } while(0)
#endif

#endif /* EXYNOS_SDM_H */
