/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * exynos fimc-is2 video functions
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
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

/* sensor */
int fimc_is_sensor_runtime_resume_pre(struct device *dev)
{
#ifndef CONFIG_PM

#endif
	return 0;
}

int fimc_is_sensor_runtime_suspend_pre(struct device *dev)
{
	return 0;
}

/* ischain */
int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	return 0;
}

int fimc_is_ischain_runtime_resume_post(struct device *dev)
{
	return 0;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
#ifndef CONFIG_PM

#endif
	return 0;
}

int fimc_is_runtime_suspend_post(struct device *dev)
{
	return 0;
}
