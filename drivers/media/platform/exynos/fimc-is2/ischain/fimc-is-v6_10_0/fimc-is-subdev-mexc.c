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
#include "fimc-is-subdev-ctrl.h"
#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-video.h"
#include "fimc-is-type.h"

static int fimc_is_ischain_mexc_cfg(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	return 0;
}

static int fimc_is_ischain_mexc_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *ldr_frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_queue *queue;
	struct camera2_scaler_uctl *scalerUd;
	struct fimc_is_device_ischain *device;
	u32 pixelformat = 0;
	u32 me_width, me_height;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!subdev);
	FIMC_BUG(!GET_SUBDEV_QUEUE(subdev));
	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!ldr_frame->shot);

	mdbgs_ischain(4, "MEXC TAG(request %d)\n", device, node->request);

	scalerUd = &ldr_frame->shot->uctl.scalerUd;
	queue = GET_SUBDEV_QUEUE(subdev);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (!queue->framecfg.format) {
		merr("format is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	pixelformat = queue->framecfg.format->pixelformat;

	if (node->request) {
		me_width = 320;
		me_height = 240;

		ret = fimc_is_ischain_buf_tag_64bit(device,
			subdev,
			ldr_frame,
			pixelformat,
			me_width,
			me_height,
			scalerUd->mexcTargetAddress);
		if (ret) {
			mswarn("%d frame is drop", device, subdev, ldr_frame->fcount);
			node->request = 0;
		}
	} else {
		scalerUd->mexcTargetAddress[0] = 0;
		scalerUd->mexcTargetAddress[1] = 0;
		scalerUd->mexcTargetAddress[2] = 0;
		node->request = 0;
	}

p_err:
	return ret;
}

const struct fimc_is_subdev_ops fimc_is_subdev_mexc_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_mexc_cfg,
	.tag			= fimc_is_ischain_mexc_tag,
};
