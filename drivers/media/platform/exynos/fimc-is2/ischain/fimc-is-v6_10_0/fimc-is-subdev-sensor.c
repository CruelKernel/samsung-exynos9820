/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-device-ischain.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-subdev-ctrl.h"
#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-video.h"
#include "fimc-is-type.h"

static int fimc_is_sensor_subdev_cfg(struct fimc_is_subdev *leader,
	void *device_data,
	struct fimc_is_frame *frame,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret =0;

	return ret;
}

static int fimc_is_sensor_subdev_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;

	device = (struct fimc_is_device_sensor *)device_data;
	if (!frame) {
		merr("[SEN] process is empty", device);
		goto p_err;
	}

p_err:
	return ret;
}


const struct fimc_is_subdev_ops fimc_is_subdev_sensor_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_sensor_subdev_cfg,
	.tag			= fimc_is_sensor_subdev_tag,
};
