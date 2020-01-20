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
#define PMU_CMU_ISP_REG_OFFSET 0x1400
#define PMU_RST_ISP_REG_OFFSET 0x1500

int fimc_is_sensor_runtime_suspend_pre(struct device *dev)
{
	return 0;
}

int fimc_is_sensor_runtime_resume_pre(struct device *dev)
{
	return 0;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
	return 0;
}

int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	return 0;
}

int fimc_is_ischain_runtime_resume_post(struct device *dev)
{
	return 0;
}

int fimc_is_runtime_suspend_post(struct device *dev)
{
	int ret = 0;
#ifdef CONFIG_PM
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
#endif
	return ret;
}
