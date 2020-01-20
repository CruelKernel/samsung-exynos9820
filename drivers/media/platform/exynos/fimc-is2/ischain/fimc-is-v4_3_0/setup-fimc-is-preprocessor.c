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

int exynos8890_fimc_is_preproc_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return 0;
}

int exynos8890_fimc_is_preproc_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	fimc_is_enable(dev, "gate_i2c0_isp");
	fimc_is_enable(dev, "gate_i2c1_isp");
	fimc_is_enable(dev, "gate_i2c2_isp");
	fimc_is_enable(dev, "gate_i2c3_isp");
	fimc_is_enable(dev, "gate_wdt_isp");
	fimc_is_enable(dev, "gate_mcuctl_isp");
	fimc_is_enable(dev, "gate_uart_isp");
	fimc_is_enable(dev, "gate_pdma_isp");
	fimc_is_enable(dev, "gate_pwm_isp");
	fimc_is_enable(dev, "gate_spi0_isp");
	fimc_is_enable(dev, "gate_spi1_isp");
	fimc_is_enable(dev, "isp_spi0");
	fimc_is_enable(dev, "isp_spi1");
	fimc_is_enable(dev, "isp_uart");
	fimc_is_enable(dev, "gate_sclk_pwm_isp");
	fimc_is_enable(dev, "gate_sclk_uart_isp");
	fimc_is_enable(dev, "cam1_peri");

	fimc_is_set_rate(dev, "isp_spi0", 100 * 1000000);
	fimc_is_set_rate(dev, "isp_spi1", 100 * 1000000);
	fimc_is_set_rate(dev, "isp_uart", 132 * 1000000);

	return 0;
}

int exynos8890_fimc_is_preproc_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	fimc_is_disable(dev, "gate_i2c0_isp");
	fimc_is_disable(dev, "gate_i2c1_isp");
	fimc_is_disable(dev, "gate_i2c2_isp");
	fimc_is_disable(dev, "gate_i2c3_isp");
	fimc_is_disable(dev, "gate_wdt_isp");
	fimc_is_disable(dev, "gate_mcuctl_isp");
	fimc_is_disable(dev, "gate_uart_isp");
	fimc_is_disable(dev, "gate_pdma_isp");
	fimc_is_disable(dev, "gate_pwm_isp");
	fimc_is_disable(dev, "gate_spi0_isp");
	fimc_is_disable(dev, "gate_spi1_isp");
	fimc_is_disable(dev, "isp_spi0");
	fimc_is_disable(dev, "isp_spi1");
	fimc_is_disable(dev, "isp_uart");
	fimc_is_disable(dev, "gate_sclk_pwm_isp");
	fimc_is_disable(dev, "gate_sclk_uart_isp");
	fimc_is_disable(dev, "cam1_peri");

	return 0;
}

int exynos8890_fimc_is_preproc_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos8890_fimc_is_preproc_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}

/* Wrapper functions */
int exynos_fimc_is_preproc_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos8890_fimc_is_preproc_iclk_cfg(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos8890_fimc_is_preproc_iclk_on(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos8890_fimc_is_preproc_iclk_off(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos8890_fimc_is_preproc_mclk_on(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_preproc_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos8890_fimc_is_preproc_mclk_off(dev, scenario, channel);
	return 0;
}
