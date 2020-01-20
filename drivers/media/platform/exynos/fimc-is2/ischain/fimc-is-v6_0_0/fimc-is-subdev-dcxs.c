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

#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-hw-dvfs.h"

static int fimc_is_ischain_dcxs_cfg(struct fimc_is_subdev *subdev,
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

static int fimc_is_ischain_dcxs_start(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct fimc_is_queue *queue,
	struct fimc_is_crop *incrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct param_dma_input *dma_input;
	struct fimc_is_fmt *format;
	u32 width, height;
	/* VOTF of slave in is alwasy disabled because only VOTF of master in is used */
	u32 votf_enable = OTF_INPUT_COMMAND_DISABLE;

	FIMC_BUG(!device);
	FIMC_BUG(!incrop);
	FIMC_BUG(!lindex);
	FIMC_BUG(!hindex);
	FIMC_BUG(!indexes);

	width = incrop->w;
	height = incrop->h;

	group = &device->group_dcp;

	format = queue->framecfg.format;
	if (!format) {
		merr("format is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	/* DCP: Always use DMA input only */
	dma_input = fimc_is_itf_g_param(device, frame, PARAM_DCP_INPUT_SLAVE);
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

	*lindex |= LOWBIT_OF(PARAM_DCP_INPUT_SLAVE);
	*hindex |= HIGHBIT_OF(PARAM_DCP_INPUT_SLAVE);
	(*indexes)++;

	subdev->input.crop = *incrop;

	set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

p_err:
	return ret;
}

static int fimc_is_ischain_dcxs_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct param_dma_output *dma_input;

	mdbgd_ischain("%s\n", device, __func__);

	dma_input = fimc_is_itf_g_param(device, frame, PARAM_DCP_INPUT_SLAVE);
	dma_input->cmd = DMA_INPUT_COMMAND_DISABLE;
	*lindex |= LOWBIT_OF(PARAM_DCP_INPUT_SLAVE);
	*hindex |= HIGHBIT_OF(PARAM_DCP_INPUT_SLAVE);
	(*indexes)++;

	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

	return ret;
}

static int fimc_is_ischain_dcxs_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *ldr_frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_subdev *leader;
	struct fimc_is_queue *queue;
	struct dcp_param *dcp_param;
	struct fimc_is_crop *incrop, inparm;
	struct fimc_is_device_ischain *device;
	u32 lindex, hindex, indexes;
	u32 pixelformat = 0;
	u32 *target_addr;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!subdev);
	FIMC_BUG(!GET_SUBDEV_QUEUE(subdev));
	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!ldr_frame->shot);
	FIMC_BUG(!node);

	mdbgs_ischain(4, "DCXC TAG(request %d)\n", device, node->request);

	lindex = hindex = indexes = 0;
	leader = subdev->leader;
	dcp_param = &device->is_region->parameter.dcp;
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

	target_addr = ldr_frame->shot->uctl.scalerUd.sourceAddress; /* TODO */

	if (node->request) {
		incrop = (struct fimc_is_crop *)node->input.cropRegion;

		inparm.x = 0;
		inparm.y = 0;
		inparm.w = dcp_param->dma_input_s.width;
		inparm.h = dcp_param->dma_input_s.height;

		if (IS_NULL_CROP(incrop))
			*incrop = inparm;

		if (!COMPARE_CROP(incrop, &inparm) ||
			!test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state) ||
			test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
			ret = fimc_is_ischain_dcxs_start(device,
				subdev,
				ldr_frame,
				queue,
				incrop,
				&lindex,
				&hindex,
				&indexes);
			if (ret) {
				merr("fimc_is_ischain_dcxs_start is fail(%d)", device, ret);
				goto p_err;
			}

			mdbg_pframe("in_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
				incrop->x, incrop->y, incrop->w, incrop->h);
		}

		ret = fimc_is_ischain_buf_tag(device,
			subdev,
			ldr_frame,
			pixelformat,
			incrop->w,
			incrop->h,
			target_addr);
		if (ret) {
			mswarn("%d frame is drop", device, subdev, ldr_frame->fcount);
			node->request = 0;
		}
	} else {
		if (test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state)) {
			ret = fimc_is_ischain_dcxs_stop(device,
				subdev,
				ldr_frame,
				&lindex,
				&hindex,
				&indexes);
			if (ret) {
				merr("fimc_is_ischain_dcxs_stop is fail(%d)", device, ret);
				goto p_err;
			}

			mdbg_pframe(" off\n", device, subdev, ldr_frame);
		}

		target_addr[0] = 0;
		target_addr[1] = 0;
		target_addr[2] = 0;
		node->request = 0;
	}

	ret = fimc_is_itf_s_param(device, ldr_frame, lindex, hindex, indexes);
	if (ret) {
		mrerr("fimc_is_itf_s_param is fail(%d)", device, ldr_frame, ret);
		goto p_err;
	}

p_err:
	return ret;
}

const struct fimc_is_subdev_ops fimc_is_subdev_dcxs_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_dcxs_cfg,
	.tag			= fimc_is_ischain_dcxs_tag,
};
