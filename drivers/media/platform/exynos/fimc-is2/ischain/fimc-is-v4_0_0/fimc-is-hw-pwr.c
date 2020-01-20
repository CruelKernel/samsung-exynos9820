/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
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

#include <mach/map.h>
#include <mach/regs-clock.h>

#include "fimc-is-config.h"
#include "fimc-is-type.h"
#include "fimc-is-regs.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"

int fimc_is_sensor_runtime_suspend_pre(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;

	device = (struct fimc_is_device_sensor *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		goto p_err;
	}

	if (device->instance == 0) {
		if ((readl(EXYNOS_PMU_CAM0_STATUS) & 0xF) != 0xF) {
			err("CAM0 power is not on(0x%08x)\n", readl(EXYNOS_PMU_CAM0_STATUS));
			ret = -EINVAL;
			goto p_err;
		}
	} else {
		if ((readl(EXYNOS_PMU_CAM1_STATUS) & 0xF) != 0xF) {
			err("CAM1 power is not on(0x%08x)\n", readl(EXYNOS_PMU_CAM1_STATUS));
			ret = -EINVAL;
			goto p_err;
		}
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

#ifndef CONFIG_PM
	{
		u32 try_count;

		/* CAM0 */
		/* 1. All Clock On */
		setbits(EXYNOS7420_MUX_ENABLE_TOP05, 28, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP05, 24, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP05, 20, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP05, 16, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP05, 12, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP05, 8, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP06, 28, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP06, 24, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP06, 20, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP06, 16, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 28, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 24, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 20, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 16, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 12, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 8, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 28, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 24, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 20, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 16, 0x1, 0x1);

		/* 2. Related Async-bridge Clock On */
		/*
		 * if CAM1, ISP0, ISP1 is power on
		 * setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 23, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 12, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP0_LOCAL0, 12, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 4, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP1_LOCAL, 4, 0x1, 0x1);
		 */

		/* 3. Power On */
		setbits(EXYNOS_PMU_CLKRUN_CMU_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_CLKSTOP_CMU_CAM0_SYS_PWR_REG,0, 0x1, 0x0);
		setbits(EXYNOS_PMU_DISABLE_PLL_CMU_CAM0_SYS_PWR_REG,0, 0x1, 0x0);
		setbits(EXYNOS_PMU_RESET_LOGIC_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_MEMORY_CAM0_SYS_PWR_REG, 4, 0x3, 0x0);
		setbits(EXYNOS_PMU_MEMORY_CAM0_SYS_PWR_REG, 0, 0x3, 0x0);
		setbits(EXYNOS_PMU_RESET_CMU_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_CAM0_OPTION, 1, 0x1, 0x1);
		setbits(EXYNOS_PMU_CAM0_CONFIGURATION, 0, 0xF, 0xF);

		for (try_count = 0; try_count < 10; ++try_count) {
			if (getbits(EXYNOS_PMU_CAM0_STATUS, 0, 0xF) == 0xF) {
				minfo("CAM0 power on\n", device);
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10) {
			merr("CAM0 power on is fail", device);
			goto p_err;
		}

		/* CAM1 */
		/* 1. All Clock On */
		setbits(EXYNOS7420_MUX_ENABLE_TOP07, 28, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP07, 24, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP07, 20, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP07, 16, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP07, 12, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP07, 8, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 28, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 24, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 20, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 16, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 12, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 8, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_TOP0_CAM10, 20, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_TOP0_CAM10, 8, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_TOP0_CAM10, 4, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_SCLK_TOP0_CAM10, 20, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_SCLK_TOP0_CAM10, 8, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_SCLK_TOP0_CAM10, 4, 0x1, 0x1);

		/*2. Related Async-bridge Clock On */
		setbits(EXYNOS7420_ENABLE_ACLK_CAM01, 16, 0x1, 0x1);
		/*
		 * if ISP0, ISP1 is power on
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 17, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 15, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 12, 0x1, 0x1);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 8, 0x3, 0x3);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP01, 0, 0x3, 0x3);
		 * setbits(EXYNOS7420_ENABLE_ACLK_ISP0_LOCAL1, 0, 0x3, 0x3);
		 */

		/*3. Power On */
		setbits(EXYNOS_PMU_CLKRUN_CMU_CAM1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_CLKSTOP_CMU_CAM1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_DISABLE_PLL_CMU_CAM1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_RESET_LOGIC_CAM1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_MEMORY_CAM1_SYS_PWR_REG, 4, 0x3, 0x0);
		setbits(EXYNOS_PMU_MEMORY_CAM1_SYS_PWR_REG, 0, 0x3, 0x0);
		setbits(EXYNOS_PMU_RESET_CMU_CAM1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_CAM1_OPTION, 1, 0x1, 0x1);
		setbits(EXYNOS_PMU_CAM1_CONFIGURATION, 0, 0xF, 0xF);

		for (try_count = 0; try_count < 10; ++try_count) {
			if (getbits(EXYNOS_PMU_CAM1_STATUS, 0, 0xF) == 0xF) {
				minfo("CAM1 power on\n", device);
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10) {
			merr("CAM1 power on is fail", device);
			goto p_err;
		}
	}
#endif

p_err:
	return ret;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
#ifndef CONFIG_PM
	u32 try_count;

	/* CAM0 */
	/* 1. All Clock On */
	setbits(EXYNOS7420_MUX_ENABLE_TOP05, 28, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP05, 24, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP05, 20, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP05, 16, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP05, 12, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP05, 8, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP06, 28, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP06, 24, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP06, 20, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP06, 16, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 28, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 24, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 16, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP05, 8, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 28, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 24, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP06, 16, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM00, 0, 0x1F, 0x1F);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM01, 16, 0x7, 0x7);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM01, 10, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM00, 0, 0x1F, 0x1F);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM01, 16, 0x7, 0x7);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM0_LOCAL, 4, 0x7, 0x7);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM0_LOCAL, 0, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM0_LOCAL, 4, 0x7, 0x7);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM0_LOCAL, 0, 0x3, 0x3);

	/* 2. Related Async-bridge Clock On */
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 23, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP0_LOCAL0, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 4, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP1_LOCAL, 4, 0x1, 0x1);

	/* 3. Power Off */
	setbits(EXYNOS_PMU_CLKRUN_CMU_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_CLKSTOP_CMU_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_DISABLE_PLL_CMU_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_RESET_LOGIC_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_MEMORY_CAM0_SYS_PWR_REG, 4, 0x3, 0x0);
	setbits(EXYNOS_PMU_MEMORY_CAM0_SYS_PWR_REG, 0, 0x3, 0x0);
	setbits(EXYNOS_PMU_RESET_CMU_CAM0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_CAM0_OPTION, 1, 0x1, 0x1);
	setbits(EXYNOS_PMU_CAM0_CONFIGURATION, 0, 0xF, 0x0);

	for (try_count = 0; try_count < 10; ++try_count) {
		if (getbits(EXYNOS_PMU_CAM0_STATUS, 0, 0xF) == 0) {
			info("CAM0 power off\n");
			break;
		}
		mdelay(1);
	}

	if (try_count >= 10)
		err("CAM0 power off is fail(%X)", getbits(EXYNOS_PMU_CAM0_STATUS, 0, 0xF));

	/* ISP0 */
	/* 1. All Clock On */
	setbits(EXYNOS7420_MUX_ENABLE_TOP04, 28, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP04, 24, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP04, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 28, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 24, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 17, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 15, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 13, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 0, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP01, 0, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP0_LOCAL0, 0, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP0_LOCAL1, 0, 0x3, 0x3);

	/* 2. Related Async-bridge Clock On */
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 25, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 10, 0x1, 0x1);
	/*
	 * if CAM0 is power on
	 * setbits(EXYNOS7420_ENABLE_ACLK_CAM00, 0, 0x3, 0x3);
	 * setbits(EXYNOS7420_ENABLE_ACLK_CAM0_LOCAL, 0, 0x3, 0x3);
	 */
	setbits(EXYNOS7420_ENABLE_ACLK_CAM10, 19, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM1_LOCAL, 4, 0x1, 0x1);

	/* 3. Power Off */
	setbits(EXYNOS_PMU_CLKRUN_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_CLKSTOP_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_DISABLE_PLL_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_RESET_LOGIC_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_MEMORY_ISP0_SYS_PWR_REG, 4, 0x3, 0x0);
	setbits(EXYNOS_PMU_MEMORY_ISP0_SYS_PWR_REG, 0, 0x3, 0x0);
	setbits(EXYNOS_PMU_RESET_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_ISP0_OPTION, 1, 0x1, 0x1);
	setbits(EXYNOS_PMU_ISP0_CONFIGURATION, 0, 0xF, 0x0);

	for (try_count = 0; try_count < 10; ++try_count) {
		if (getbits(EXYNOS_PMU_ISP0_STATUS, 0, 0xF) == 0) {
			info("ISP0 power off\n");
			break;
		}
		mdelay(1);
	}

	if (try_count >= 10)
		err("ISP0 power off is fail(%X)", getbits(EXYNOS_PMU_ISP0_STATUS, 0, 0xF));

	/* ISP1 */
	/* 1. All Clock On */
	setbits(EXYNOS7420_MUX_ENABLE_TOP04, 16, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP04, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 16, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 8, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 0, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_PCLK_ISP1, 0, 0x1, 0x13);
	setbits(EXYNOS7420_ENABLE_ACLK_ISP1_LOCAL, 0, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_PCLK_ISP1_LOCAL, 0, 0x1, 0x1);

	/* 2. Related Async-bridge Clock On */
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 26, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 11, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 22, 0x1, 0x1);
	/*
	 * if CAM0 is power on
	 * setbits(EXYNOS7420_ENABLE_ACLK_CAM00, 0, 0x3, 0x3);
	 * setbits(EXYNOS7420_ENABLE_ACLK_CAM0_LOCAL, 0, 0x3, 0x3);
	 */

	/* 3. Power Off */
	setbits(EXYNOS_PMU_CLKRUN_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_CLKSTOP_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_DISABLE_PLL_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_RESET_LOGIC_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_MEMORY_ISP1_SYS_PWR_REG, 0, 0x3, 0x0);
	setbits(EXYNOS_PMU_RESET_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_ISP1_OPTION, 1, 0x1, 0x1);
	setbits(EXYNOS_PMU_ISP1_CONFIGURATION, 0, 0xF, 0x0);

	for (try_count = 0; try_count < 10; ++try_count) {
		if (getbits(EXYNOS_PMU_ISP1_STATUS, 0, 0xF) == 0) {
			info("ISP1 power off\n");
			break;
		}
		mdelay(1);
	}

	if (try_count >= 10)
		err("ISP1 power off is fail(%X)", getbits(EXYNOS_PMU_ISP1_STATUS, 0, 0xF));

	/* CAM1 */
	/* 1. All Clock On */
	setbits(EXYNOS7420_MUX_ENABLE_TOP07, 28, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP07, 24, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP07, 20, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP07, 16, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP07, 12, 0x1, 0x1);
	setbits(EXYNOS7420_MUX_ENABLE_TOP07, 8, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 28, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 24, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 16, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 12, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_TOP07, 8, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_TOP0_CAM10, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_TOP0_CAM10, 8, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_TOP0_CAM10, 4, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_SCLK_TOP0_CAM10, 20, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_SCLK_TOP0_CAM10, 8, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_SCLK_TOP0_CAM10, 4, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM10, 2, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM10, 0, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 28, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 25, 0x3, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 22, 0x3, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 10, 0x3, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM13, 2, 0x3, 0x1);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM10, 2, 0x3, 0x3);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM10, 0, 0x1, 0x1);
	setbits(EXYNOS7420_ENABLE_ACLK_CAM1_LOCAL, 0, 0x7, 0x7);
	setbits(EXYNOS7420_ENABLE_PCLK_CAM1_LOCAL, 0, 0x7, 0x7);

	/* 2. Related Async-bridge Clock On */
	/*
	 * if CAM0, ISP0, ISP1 is power on
	 * setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 17, 0x1, 0x1);
	 * setbits(EXYNOS7420_ENABLE_ACLK_ISP00, 15, 0x1, 0x1);
	 * setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 12, 0x1, 0x1);
	 * setbits(EXYNOS7420_ENABLE_ACLK_ISP1, 8, 0x3, 0x3);
	 * setbits(EXYNOS7420_ENABLE_ACLK_CAM01, 16, 0x1, 0x1);
	 * setbits(EXYNOS7420_ENABLE_ACLK_ISP01, 0, 0x3, 0x3);
	 * setbits(EXYNOS7420_ENABLE_ACLK_ISP0_LOCAL1, 0, 0x3, 0x3);
	 */

	/* 3. Power Off */
	{
		ulong pmu_cam1 = (ulong)ioremap_nocache(0x145E0000, SZ_4K);
		/* LPI_MASK_CAM1_ASYNCBRIDGE (This mask setting should be removed at EVT1.) */
		setbits((void *)(pmu_cam1 + 0x20), 5, 0x1, 0x1);
		/* LPI_MASK_CAM1_ASYNCBRIDGE (This mask setting should be removed at EVT1.) */
		setbits((void *)(pmu_cam1 + 0x20), 4, 0x1, 0x1);
		iounmap((void *)pmu_cam1);
	}

	setbits(EXYNOS_PMU_CLKRUN_CMU_CAM1_SYS_PWR_REG, 0, 0x1, 0x0);
	setbits(EXYNOS_PMU_CLKSTOP_CMU_CAM1_SYS_PWR_REG,0, 0x1, 0x0);
	setbits(EXYNOS_PMU_DISABLE_PLL_CMU_CAM1_SYS_PWR_REG,0, 0x1, 0x0);
	setbits(EXYNOS_PMU_RESET_LOGIC_CAM1_SYS_PWR_REG,0, 0x1, 0x0);
	setbits(EXYNOS_PMU_MEMORY_CAM1_SYS_PWR_REG,4, 0x3, 0x0);
	setbits(EXYNOS_PMU_MEMORY_CAM1_SYS_PWR_REG,0, 0x3, 0x0);
	setbits(EXYNOS_PMU_RESET_CMU_CAM1_SYS_PWR_REG,0, 0x1, 0x0);
	setbits(EXYNOS_PMU_CAM1_OPTION,0, 0x1FF, 0x102);
	setbits(EXYNOS_PMU_CAM1_CONFIGURATION,0, 0xF, 0x0);

	for (try_count = 0; try_count < 2000; ++try_count) {
		if (getbits(EXYNOS_PMU_CAM1_STATUS, 0, 0xF) == 0) {
			info("CAM1 power off\n");
			break;
		}
		mdelay(1);
	}

	if (try_count >= 2000)
		err("CAM1 power off is fail(%X)", getbits(EXYNOS_PMU_CAM1_STATUS, 0, 0xF));
#endif

	return 0;
}

int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	int ret = 0;
	u32 timeout;

#ifndef CONFIG_PM
	{
		u32 try_count;

		/* ISP0 */
		/* 1. All Clock On */
		setbits(EXYNOS7420_MUX_ENABLE_TOP04, 28, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP04, 24, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP04, 20, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 28, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 24, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 20, 0x1, 0x1);

		/* 2. Related Async-bridge Clock On */
		setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 25, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 10, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM00, 0, 0x3, 0x3);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM0_LOCAL, 0, 0x3, 0x3);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM10, 19, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM1_LOCAL, 4, 0x1, 0x1);

		/* 3. Power On */
		setbits(EXYNOS_PMU_CLKRUN_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_CLKSTOP_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_DISABLE_PLL_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_RESET_LOGIC_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_MEMORY_ISP0_SYS_PWR_REG, 4, 0x3, 0x0);
		setbits(EXYNOS_PMU_MEMORY_ISP0_SYS_PWR_REG, 0, 0x3, 0x0);
		setbits(EXYNOS_PMU_RESET_CMU_ISP0_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_ISP0_OPTION, 1, 0x1, 0x1);
		setbits(EXYNOS_PMU_ISP0_CONFIGURATION, 0, 0xF, 0xF);

		for (try_count = 0; try_count < 10; ++try_count) {
			if (getbits(EXYNOS_PMU_ISP0_STATUS, 0, 0xF) == 0xF) {
				info("ISP0 power on\n");
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10)
			err("ISP0 power on is fail");

		/* ISP1 */
		/* 1. All Clock On */
		setbits(EXYNOS7420_MUX_ENABLE_TOP04, 16, 0x1, 0x1);
		setbits(EXYNOS7420_MUX_ENABLE_TOP04, 12, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 16, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_TOP04, 12, 0x1, 0x1);

		/* 2. Related Async-bridge Clock On */
		setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 26, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 22, 0x1, 0x1);
		setbits(EXYNOS7420_ENABLE_ACLK_CAM11, 11, 0x1, 0x1);

		/* 3. Power On */
		setbits(EXYNOS_PMU_CLKRUN_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_CLKSTOP_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_DISABLE_PLL_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_RESET_LOGIC_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_MEMORY_ISP1_SYS_PWR_REG, 0, 0x3, 0x0);
		setbits(EXYNOS_PMU_RESET_CMU_ISP1_SYS_PWR_REG, 0, 0x1, 0x0);
		setbits(EXYNOS_PMU_ISP1_OPTION, 1, 0x1, 0x1);
		setbits(EXYNOS_PMU_ISP1_CONFIGURATION, 0, 0xF, 0xF);

		for (try_count = 0; try_count < 10; ++try_count) {
			if (getbits(EXYNOS_PMU_ISP1_STATUS, 0, 0xF) == 0xF) {
				info("ISP1 power on\n");
				break;
			}
			mdelay(1);
		}

		if (try_count >= 10)
			err("ISP1 power on is fail");
	}
#endif

	timeout = 1000;
	while (((readl(EXYNOS_PMU_CAM1_STATUS) & 0xF) != 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("CAM1 power up fail(0x%08x)\n", readl(EXYNOS_PMU_CAM1_STATUS));
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	while (((readl(EXYNOS_PMU_ISP0_STATUS) & 0xF) != 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("ISP0 power up fail(0x%08x)\n", readl(EXYNOS_PMU_ISP0_STATUS));
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	while (((readl(EXYNOS_PMU_ISP1_STATUS) & 0xF) != 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("ISP1 power up fail(0x%08x)\n", readl(EXYNOS_PMU_ISP1_STATUS));
		ret = -ETIME;
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ischain_runtime_resume_post(struct device *dev)
{
	int ret = 0;
	u32 timeout;

	/* check power off */
	timeout = 1000;
	while (--timeout && (__raw_readl(PMUREG_ISP_ARM_STATUS) & 0x1)) {
		warn("A5 is not power off");
		writel(0x0, PMUREG_ISP_ARM_CONFIGURATION);
	}

	if (!timeout) {
		err("A5 power on failed");
		ret = -EINVAL;
		goto p_err;
	}

	/* A5 power on*/
	writel(0x1, PMUREG_ISP_ARM_CONFIGURATION);

	/* WFE & WFI use */
	writel((1 << 16 | 1 << 15), PMUREG_ISP_ARM_OPTION);

	/* check power on */
	timeout = 1000;
	while (--timeout && ((__raw_readl(PMUREG_ISP_ARM_STATUS) & 0x1) != 0x1))
		udelay(1);

	if (!timeout) {
		err("A5 power on failed");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_runtime_suspend_post(struct device *dev)
{
	int ret = 0;
	u32 timeout;

	timeout = 1000;
	while ((readl(EXYNOS_PMU_CAM0_STATUS) & 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("CAM0 power down fail(0x%08x)", readl(EXYNOS_PMU_CAM0_STATUS));
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	while ((readl(EXYNOS_PMU_CAM1_STATUS) & 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("CAM1 power down fail(0x%08x)", readl(EXYNOS_PMU_CAM1_STATUS));
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	while ((readl(EXYNOS_PMU_ISP0_STATUS) & 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("ISP0 power down fail(0x%08x)", readl(EXYNOS_PMU_ISP0_STATUS));
		ret = -ETIME;
		goto p_err;
	}

	timeout = 1000;
	while ((readl(EXYNOS_PMU_ISP1_STATUS) & 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("ISP1 power down fail(0x%08x)", readl(EXYNOS_PMU_ISP1_STATUS));
		ret = -ETIME;
		goto p_err;
	}

p_err:
	return ret;
}
