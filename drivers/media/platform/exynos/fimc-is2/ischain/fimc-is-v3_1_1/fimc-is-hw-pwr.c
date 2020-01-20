/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
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
#include "../../fimc-is-device-sensor.h"
#include "../../fimc-is-device-ischain.h"

int fimc_is_sensor_runtime_suspend_pre(struct device *dev)
{
	/*TODO*/

	return 0;
}

int fimc_is_sensor_runtime_resume_pre(struct device *dev)
{
	/*TODO*/

	return 0;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
	return 0;
}

int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	/*TODO*/

	return 0;
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
	writel((1 << 15), PMUREG_ISP_ARM_OPTION);

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
	u32 timeout;
	int ret = 0;

	timeout = 1000;
	while ((readl(EXYNOS_PMU_ISP_STATUS) & 0xF) && timeout) {
		timeout--;
		usleep_range(1000, 1000);
	}

	if (timeout == 0) {
		err("ISP power down fail(0x%08x, 0x%08x, 0x%08x, 0x%08x)",
				readl(EXYNOS_PMU_ISP_STATUS),
				readl(EXYNOS_PMU_A5IS_CONFIGURATION),
				readl(EXYNOS_PMU_A5IS_STATUS),
				readl(EXYNOS_PMU_A5IS_OPTION)
				);
		ret = -ETIME;
		goto p_err;
	}

p_err:
	return ret;
}
