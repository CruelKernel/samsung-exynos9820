/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS PPMU header
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __DEVFREQ_EXYNOS_PPMU_H
#define __DEVFREQ_EXYNOS_PPMU_H __FILE__

#include <linux/ktime.h>
#include <linux/devfreq.h>
#include <soc/samsung/exynos-devfreq.h>

struct ppc_data {
	u64 ccnt;
	u64 pmcnt;
};

#if defined(CONFIG_EXYNOS_ALT_DVFS)
int exynos_devfreq_um_init(struct exynos_devfreq_data *data);
void exynos_devfreq_um_exit(struct exynos_devfreq_data *data);
void register_get_dev_status(struct exynos_devfreq_data *data);
#else
#define exynos_devfreq_um_init(a) do {} while(0)
#define exynos_devfreq_um_exit(a) do {} while(0)
#define register_get_dev_status(a) do {} while(0)
#endif

#endif /* __DEVFREQ_EXYNOS_PPMU_H */
