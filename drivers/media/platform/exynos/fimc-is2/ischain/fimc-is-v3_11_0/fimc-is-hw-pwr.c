/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 video functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <soc/samsung/exynos-pmu.h>

#include "fimc-is-config.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"

#define PMU_ISP_REG_OFFSET 0x4040

int fimc_is_sensor_runtime_suspend_pre(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;

	device = (struct fimc_is_device_sensor *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_sensor_runtime_resume_pre(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;

	device = (struct fimc_is_device_sensor *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
	int ret = 0;
#ifndef CONFIG_PM
	{
		struct fimc_is_device_ischain *device;
		void __iomem *reg_mux_clkphy;
		void __iomem *reg_isp_idleness_ch0;
		void __iomem *reg_vra_idleness_ch0;
		void __iomem *reg_vra_idleness_ch1;
		u32 try_count;
		unsigned int state;

		device = (struct fimc_is_device_ishcain *)dev_get_drvdata(dev);
		if (!device) {
			err("device is NULL");
			goto p_err;
		}

		/* set PHY_CLK MUX sel to OSC */
		reg_mux_clkphy = (device->instance == 0) ?
			ioremap(0x144D0230, SZ_4K) : ioremap(0x144D0234, SZ_4K);

		state = readl(reg_mux_clkphy);
		state &= ~(1 << 12);
		writel(state, reg_mux_clkphy);

		/* set PHY_CLK MUX ignore bit */
		state = readl(reg_mux_clkphy);
		state |= (1 << 27);
		writel(state, reg_mux_clkphy);

		/* Check ISP idleness */
		reg_isp_idleness_ch0 = ioremap(0x14403020, SZ_4K);
		for (try_count = 0; try_count < 10; ++try_count) {
			state = readl(reg_isp_idleness_ch0);
			if ((state & 0x1)) {
				info("ISP IDLE(0x%08x)\n", state);
				break;
			}
			mdelay(1);

			if (try_count >= 10) {
				err("ISP status is not IDLE");
				ret = -ETIME;
			}
		}

		/* Check VRA_CH0 idleness */
		reg_vra_idleness_ch0 = ioremap(0x14443010, SZ_4K);
		for (try_count = 0; try_count < 10; ++try_count) {
			state = readl(reg_vra_idleness_ch0);
			if ((state & 0x1)) {
				info("VRA_CH0 IDLE(0x%08x)\n", state);
				break;
			}
			mdelay(1);

			if (try_count >= 10) {
				err("VRA_CH0 status is not IDLE");
				ret = -ETIME;
			}
		}

		/* Check VRA_CH1 idleness */
		reg_vra_idleness_ch1 = ioremap(0x1444B010, SZ_4K);
		for (try_count = 0; try_count < 10; ++try_count) {
			state = readl(reg_vra_idleness_ch1);
			if ((state & 0x1)) {
				info("VRA_CH1 IDLE(0x%08x)\n", state);
				break;
			}
			mdelay(1);

			if (try_count >= 10) {
				err("VRA_CH1 status is not IDLE");
				ret = -ETIME;
			}
		}

		/* Check Power On */
		exynos_pmu_read(PMU_ISP_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0)
			warn("ISP power is already off(0x%08x)\n", state);

		/* Set USE_SC_FEEDBACK */
		exynos_pmu_update(PMU_ISP_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);

		/* Set LOCAL_PWR_CFG */
		exynos_pmu_update(PMU_ISP_REG_OFFSET, 0xF << 0, 0x0 << 0);

		/* Check Power Off */
		for (try_count = 0; try_count < 10; ++try_count) {
			exynos_pmu_read(PMU_ISP_REG_OFFSET + 0x4, &state);
			if (!(state & 0xF)) {
				info("ISP power off(0x%08x)\n", state);
				break;
			}
			mdelay(1);

			if (try_count >= 10) {
				err("ISP power off is fail");
				ret = -ETIME;
			}
		}

		iounmap(reg_mux_clkphy);
		iounmap(reg_isp_idleness_ch0);
		iounmap(reg_vra_idleness_ch0);
		iounmap(reg_vra_idleness_ch1);
	}
p_err:
#endif

	return ret;
}

int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	int ret = 0;

#ifndef CONFIG_PM
	{
		u32 try_count;
		unsigned int state;

		/* Check Power Off */
		exynos_pmu_read(PMU_ISP_REG_OFFSET + 0x4, &state);
		if ((state & 0xF) == 0xF)
			warn("ISP power is already on(0x%08x)\n", state);

		/* Set USE_SC_FEEDBACK */
		exynos_pmu_update(PMU_ISP_REG_OFFSET + 0x8, 0x1 << 1, 0x1 << 1);

		/* Set LOCAL_PWR_CFG */
		exynos_pmu_update(PMU_ISP_REG_OFFSET, 0xF << 0, 0xF << 0);

		/* Check Power On */
		for (try_count = 0; try_count < 10; ++try_count) {
			exynos_pmu_read(PMU_ISP_REG_OFFSET + 0x4, &state);
			if ((state & 0xF) == 0xF) {
				info("ISP power on(0x%08x)\n", state);
				break;
			}
			mdelay(1);

			if (try_count >= 10) {
				err("ISP power on is fail");
				ret = -ETIME;
			}
		}
	}
#endif

	return ret;
}

int fimc_is_ischain_runtime_resume_post(struct device *dev)
{
	int ret = 0;

	return ret;
}

int fimc_is_runtime_suspend_post(struct device *dev)
{
	int ret = 0;
	u32 timeout;
	unsigned int state;

	timeout = 1000;
	do {
		exynos_pmu_read(PMU_ISP_REG_OFFSET + 0x4, &state);
		if (!(state & 0xF))
			break;

		timeout--;
		usleep_range(1000, 1000);
	} while (timeout);

	if (timeout == 0) {
		err("ISP power down fail(0x%08x)", state);
		ret = -ETIME;
	}

	return ret;
}
