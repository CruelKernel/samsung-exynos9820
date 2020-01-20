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

	/* TODO */

p_err:
	return ret;
}

int fimc_is_ischain_runtime_suspend_post(struct device *dev)
{
	int ret = 0;
#ifndef CONFIG_PM
	/* TODO */
#endif

	return ret;
}

int fimc_is_ischain_runtime_resume_pre(struct device *dev)
{
	int ret = 0;

#ifndef CONFIG_PM
	/* TODO */
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
	/* TODO */

	return ret;
}
