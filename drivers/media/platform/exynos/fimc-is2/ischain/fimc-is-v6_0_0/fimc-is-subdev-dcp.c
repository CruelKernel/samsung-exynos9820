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

static int fimc_is_ischain_dcp_cfg(struct fimc_is_subdev *leader,
	void *device_data,
	struct fimc_is_frame *frame,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_queue *queue;
	struct param_dma_input *dma_input;
	struct param_control *control;
	struct fimc_is_device_ischain *device;
	struct fimc_is_fmt *format;
	u32 width, height;
	u32 votf_enable;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!leader);
	FIMC_BUG(!device);
	FIMC_BUG(!incrop);
	FIMC_BUG(!lindex);
	FIMC_BUG(!hindex);
	FIMC_BUG(!indexes);

	width = incrop->w;
	height = incrop->h;

	group = &device->group_dcp;
	if (test_bit(FIMC_IS_GROUP_VIRTUAL_OTF_INPUT, &group->state))
		votf_enable = OTF_INPUT_COMMAND_ENABLE;
	else
		votf_enable = OTF_INPUT_COMMAND_DISABLE;

	queue = GET_SUBDEV_QUEUE(leader);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	format = queue->framecfg.format;
	if (!format) {
		merr("format is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	/* Configure Control */
	if (!frame) {
		control = fimc_is_itf_g_param(device, NULL, PARAM_DCP_CONTROL);
		control->cmd = CONTROL_COMMAND_START;
		control->bypass = CONTROL_BYPASS_DISABLE;
		*lindex |= LOWBIT_OF(PARAM_DCP_CONTROL);
		*hindex |= HIGHBIT_OF(PARAM_DCP_CONTROL);
		(*indexes)++;
	}

	/* DCP: Always use DMA input only */
	dma_input = fimc_is_itf_g_param(device, frame, PARAM_DCP_INPUT_MASTER);
	dma_input->cmd = DMA_INPUT_COMMAND_ENABLE;
	dma_input->format = format->hw_format;
	dma_input->bitwidth = format->hw_bitwidth;
	dma_input->order = change_to_input_order(format->hw_order);
	dma_input->plane = format->hw_plane;
	dma_input->width = width;
	dma_input->height = height;
	dma_input->dma_crop_offset = (incrop->x << 16) | (incrop->y << 0);
	dma_input->dma_crop_width = width;
	dma_input->dma_crop_height = height;
	dma_input->v_otf_enable = votf_enable;
	dma_input->v_otf_token_line = VOTF_TOKEN_LINE;

	*lindex |= LOWBIT_OF(PARAM_DCP_INPUT_MASTER);
	*hindex |= HIGHBIT_OF(PARAM_DCP_INPUT_MASTER);
	(*indexes)++;

	leader->input.crop = *incrop;

p_err:
	return ret;
}

static int fimc_is_ischain_dcp_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct dcp_param *dcp_param;
	struct fimc_is_crop inparm;
	struct fimc_is_crop *incrop, *otcrop;
	struct fimc_is_subdev *leader;
	struct fimc_is_device_ischain *device;
	u32 lindex, hindex, indexes;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!subdev);
	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!frame);

	mdbgs_ischain(4, "DCP TAG\n", device);

	incrop = (struct fimc_is_crop *)node->input.cropRegion;
	otcrop = (struct fimc_is_crop *)node->output.cropRegion;

	group = &device->group_dcp;
	leader = subdev->leader;
	lindex = hindex = indexes = 0;
	dcp_param = &device->is_region->parameter.dcp;

	inparm.x = 0;
	inparm.y = 0;
	inparm.w = dcp_param->dma_input_m.width;
	inparm.h = dcp_param->dma_input_m.height;

	if (IS_NULL_CROP(incrop))
		*incrop = inparm;

	if (!COMPARE_CROP(incrop, &inparm) ||
		test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
		ret = fimc_is_ischain_dcp_cfg(subdev,
			device,
			frame,
			incrop,
			otcrop,
			&lindex,
			&hindex,
			&indexes);
		if (ret) {
			merr("fimc_is_ischain_dcp_cfg is fail(%d)", device, ret);
			goto p_err;
		}

		msrinfo("in_crop[%d, %d, %d, %d]\n", device, subdev, frame,
				incrop->x, incrop->y, incrop->w, incrop->h);
	}

	ret = fimc_is_itf_s_param(device, frame, lindex, hindex, indexes);
	if (ret) {
		mrerr("fimc_is_itf_s_param is fail(%d)", device, frame, ret);
		goto p_err;
	}

p_err:
	return ret;
}

const struct fimc_is_subdev_ops fimc_is_subdev_dcp_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_dcp_cfg,
	.tag			= fimc_is_ischain_dcp_tag,
};
