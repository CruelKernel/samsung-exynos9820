/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Debug-SnapShot: Debug Framework for Ramdump based debugging method
 * The original code is Exynos-Snapshot for Exynos SoC
 *
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 * Author: Changki Kim <changki.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef EXYNOS_FLEXPMU_DEBUG_H
#define EXYNOS_FLEXPMU_DEBUG_H

#ifdef CONFIG_EXYNOS_FLEXPMU_DBG
extern void exynos_flexpmu_dbg_log_stop(void);
#else
#define exynos_flexpmu_dbg_log_stop()		do { } while(0)
#endif

#endif
