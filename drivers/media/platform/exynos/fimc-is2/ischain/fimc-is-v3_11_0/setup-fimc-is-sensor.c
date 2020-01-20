/* linux/arch/arm/mach-exynos/setup-fimc-sensor.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * FIMC-IS gpio and clock configuration
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
#include <exynos-fimc-is-module.h>

static int exynos7870_fimc_is_csi_gate(struct device *dev, u32 instance, bool mask)
{
	int ret = 0;

	pr_debug("%s(instance : %d / mask : %d)\n", __func__, instance, mask);

	switch (instance) {
	case 0:
		if (mask)
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
		else
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4_user");
		break;
	case 1:
		if (mask)
			fimc_is_disable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
		else
			fimc_is_enable(dev, "umux_isp_clkphy_isp_s_rxbyteclkhs0_s4s_user");
		break;
	default:
		pr_err("(%s) instance is invalid(%d)\n", __func__, instance);
		ret = -EINVAL;
		break;
	}

	return ret;
}

int exynos7870_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	return  0;
}

int exynos7870_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	fimc_is_enable(dev, "gate_isp_cam");
	fimc_is_enable(dev, "pxmxdx_isp_cam");

	switch (channel) {
	case 0:
		/* CSI */
		exynos7870_fimc_is_csi_gate(dev, 0, false);
		break;
	case 1:
		/* CSI */
		exynos7870_fimc_is_csi_gate(dev, 1, false);
		break;
	default:
		pr_err("channel is invalid(%d)\n", channel);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int exynos7870_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	int ret = 0;

	fimc_is_disable(dev, "gate_isp_cam");
	fimc_is_disable(dev, "pxmxdx_isp_cam");
	/* CSI */
	exynos7870_fimc_is_csi_gate(dev, channel, true);

	return ret;
}

int exynos7870_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d_sclk", channel);

	fimc_is_enable(dev, sclk_name);
	fimc_is_set_rate(dev, sclk_name, 26 * 1000000);

	return 0;
}

int exynos7870_fimc_is_sensor_mclk_off(struct device *dev,
		u32 scenario,
		u32 channel)
{
	char sclk_name[30];

	pr_debug("%s(scenario : %d / ch : %d)\n", __func__, scenario, channel);

	snprintf(sclk_name, sizeof(sclk_name), "isp_sensor%d_sclk", channel);

	fimc_is_disable(dev, sclk_name);

	return 0;
}

int exynos_fimc_is_sensor_iclk_cfg(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_sensor_iclk_cfg(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_sensor_iclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_sensor_iclk_on(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_sensor_iclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_sensor_iclk_off(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_sensor_mclk_on(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_sensor_mclk_on(dev, scenario, channel);
	return 0;
}

int exynos_fimc_is_sensor_mclk_off(struct device *dev,
	u32 scenario,
	u32 channel)
{
	exynos7870_fimc_is_sensor_mclk_off(dev, scenario, channel);
	return 0;
}
