/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>

#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-device-preprocessor.h"
#include "fimc-is-device-companion.h"

int fimc_is_comp_get_mode(struct v4l2_subdev *subdev,
		struct fimc_is_device_sensor *device, u16 *mode_select)
{
	struct fimc_is_preprocessor *preprocessor;
	struct fimc_is_device_preproc *device_preproc;
	struct fimc_is_module_enum *module;
	const struct fimc_is_companion_cfg *cfg_table, *select = NULL;
	u32 cfgs, mode = 0;
	u32 position, width, height, framerate;
	u32 approximate_value = UINT_MAX;
	int deviation;

	if (unlikely(!subdev)) {
		err("subdev_preprocessor is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!device)) {
		err("device_sensor is NULL\n");
		return -EINVAL;
	}

	preprocessor = (struct fimc_is_preprocessor *)v4l2_get_subdevdata(subdev);
	if (unlikely(!preprocessor)) {
		merr("preprocessor is NULL\n", device);
		return -EINVAL;
	}

	device_preproc = preprocessor->device_preproc;
	if (unlikely(!device_preproc)) {
		merr("device_preproc is NULL\n", device);
		return -EINVAL;
	}

	module = device_preproc->module;
	if (unlikely(!module)) {
		merr("module_enum is NULL\n", device);
		return -EINVAL;
	}

	position = module->position;
	cfg_table = preprocessor->cfg;
	cfgs = preprocessor->cfgs;
	width = device->image.window.width;
	height = device->image.window.height;
	framerate = device->image.framerate;

	/* find sensor mode by w/h and fps range */
	for (mode = 0; mode < cfgs; mode++) {
		if ((cfg_table[mode].width == width) && (cfg_table[mode].height == height)) {
			deviation = cfg_table[mode].framerate - framerate;
			if (deviation == 0) {
				/* You don't need to find another sensor mode */
				select = &cfg_table[mode];
				break;
			} else if ((deviation > 0) && approximate_value > abs(deviation)) {
				/* try to find framerate smaller than previous */
				approximate_value = abs(deviation);
				select = &cfg_table[mode];
			}
		}
	}

	if (!select) {
		merr("companion mode(%dx%d@%dfps) is not found", device, width, height, framerate);
		return -EINVAL;
	}

	minfo("Companion mode(%dx%d@%d) = %d\n", device,
			select->width,
			select->height,
			select->framerate,
			select->mode);

	*mode_select = select->mode;

	return 0;
}
