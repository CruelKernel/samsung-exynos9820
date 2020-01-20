/*
 * linux/soc/samsung/exynos-pm_dvs.h
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_EXYNOS_PM_DVS_H
#define __LINUX_EXYNOS_PM_DVS_H

#define EXYNOS_PM_DVS_MODULE_NAME "exynos-pm-dvs"

struct exynos_dvs_info {
	struct list_head	node;
	struct regulator	*regulator;
	const char		*id;
	u32			suspend_volt;
	u32			init_volt;
	u32			volt_range;
};

#endif /* __LINUX_EXYNOS_PM_DVS_H */
