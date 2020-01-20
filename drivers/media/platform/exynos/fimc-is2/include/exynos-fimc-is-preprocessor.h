/*
 * /include/media/exynos_fimc_is_sensor.h
 *
 * Copyright (C) 2012 Samsung Electronics, Co. Ltd
 *
 * Exynos series exynos_fimc_is_sensor device support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MEDIA_EXYNOS_PREPROCESSOR_H
#define MEDIA_EXYNOS_PREPROCESSOR_H

#include <linux/platform_device.h>

struct exynos_platform_fimc_is_preproc {
	int (*iclk_cfg)(struct device *dev, u32 scenario, u32 channel);
	int (*iclk_on)(struct device *dev,u32 scenario, u32 channel);
	int (*iclk_off)(struct device *dev, u32 scenario, u32 channel);
	int (*mclk_on)(struct device *dev, u32 scenario, u32 channel);
	int (*mclk_off)(struct device *dev, u32 scenario, u32 channel);
	u32 scenario;
	u32 mclk_ch;
	u32 id;
};

static inline int exynos_fimc_is_preproc_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel) {return 0;}
static inline int exynos_fimc_is_preproc_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel) {return 0;}
static inline int exynos_fimc_is_preproc_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel) {return 0;}
static inline int exynos_fimc_is_preproc_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel) {return 0;}
static inline int exynos_fimc_is_preproc_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel) {return 0;}

#endif /* MEDIA_EXYNOS_PREPROCESSOR_H */
