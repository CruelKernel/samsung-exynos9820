/* linux/arch/arm/mach-exynos/setup-fimc-sensor.c
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * FIMC-IS-PREPROCESSOR gpio and clock configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#include <exynos-fimc-is.h>
#include <exynos-fimc-is-sensor.h>

int exynos7870_fimc_is_preproc_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return 0;
}

int exynos7870_fimc_is_preproc_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return 0;
}

int exynos7870_fimc_is_preproc_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return 0;
}

int exynos7870_fimc_is_preproc_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return 0;
}

int exynos7870_fimc_is_preproc_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_preproc_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_preproc_iclk_cfg(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_preproc_iclk_on(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_preproc_iclk_off(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_preproc_mclk_on(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_preproc_mclk_off(dev, scenario, channel);
	return 0;
}
