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

static int fimc_is_ischain_dcxc_cfg(struct fimc_is_subdev *subdev,
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

static int fimc_is_ischain_dcxc_start(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct fimc_is_queue *queue,
	struct param_dma_output *dma_output,
	struct fimc_is_crop *otcrop,
	ulong index,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct fimc_is_fmt *format;
	struct fimc_is_group *group;
	u32 votf_enable;
	u32 votf_dst_ip;
	u32 votf_dst_axi_id_m = 0;
	u32 votf_dst_axi_id_s = 0;
	u32 votf_dst_axi_id_d = 0;

	group = &device->group_dcp;
	if (group->child) { /* OTF output */
		votf_enable = OTF_INPUT_COMMAND_ENABLE;
		if (test_bit(FIMC_IS_SUBDEV_START, &device->dc2c.state)) {
			/* CIP2: OOF */
			votf_dst_ip = VOTF_IP_CIP2;
			votf_dst_axi_id_d = VOTF_AXI_ID_CIP2_DISPARITY_IN;
		} else {
			/* CIP1: fusion */
			votf_dst_ip = VOTF_IP_CIP1;
			votf_dst_axi_id_m = VOTF_AXI_ID_CIP1_MASTER_IN;
			votf_dst_axi_id_s = VOTF_AXI_ID_CIP1_SALVE_IN;
		}
	} else { /* M2M output */
		votf_enable = OTF_INPUT_COMMAND_DISABLE;
		votf_dst_ip = VOTF_IP_DCP;
	}

	format = queue->framecfg.format;
	if (!format) {
		merr("format is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	dma_output->cmd = DMA_OUTPUT_COMMAND_ENABLE;
	dma_output->format = format->hw_format;
	dma_output->bitwidth = format->hw_bitwidth;
	dma_output->order = format->hw_order;
	dma_output->plane = format->hw_plane;

	dma_output->width = otcrop->w;
	dma_output->height = otcrop->h;
	dma_output->dma_crop_offset_x = otcrop->x;
	dma_output->dma_crop_offset_y = otcrop->y;
	dma_output->dma_crop_width = otcrop->w;
	dma_output->dma_crop_height = otcrop->h;

	dma_output->v_otf_enable = votf_enable;
	dma_output->v_otf_dst_ip = votf_dst_ip;
	dma_output->v_otf_token_line = VOTF_TOKEN_LINE;

	if (index == PARAM_DCP_OUTPUT_MASTER) {
		/* Master */
		dma_output->v_otf_dst_axi_id = votf_dst_axi_id_m;
	} else if (index == PARAM_DCP_OUTPUT_SLAVE) {
		/* Slave */
		dma_output->v_otf_dst_axi_id = votf_dst_axi_id_s;
	} else if (index == PARAM_DCP_OUTPUT_DISPARITY) {
		/* Disparity */
		dma_output->v_otf_dst_axi_id = votf_dst_axi_id_d;
	} else {
		/* Sub image */
	}

	*lindex |= LOWBIT_OF(index);
	*hindex |= HIGHBIT_OF(index);
	(*indexes)++;

	subdev->output.crop = *otcrop;

	set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

p_err:
	return ret;
}

static int fimc_is_ischain_dcxc_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct param_dma_output *dma_output,
	ulong index,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;

	mdbgd_ischain("%s\n", device, __func__);

	dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;
	*lindex |= LOWBIT_OF(index);
	*hindex |= HIGHBIT_OF(index);
	(*indexes)++;

	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

	return ret;
}

static int fimc_is_ischain_dcxc_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *ldr_frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_subdev *leader;
	struct fimc_is_queue *queue;
	struct param_dma_output *dma_output;
	struct fimc_is_crop *otcrop, otparm;
	struct fimc_is_device_ischain *device;
	u32 index, lindex, hindex, indexes;
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

	switch (node->vid) {
	case FIMC_IS_VIDEO_DCP0C_NUM:
		index = PARAM_DCP_OUTPUT_MASTER;
		target_addr = ldr_frame->shot->uctl.scalerUd.sccTargetAddress; /* TODO: name */
		break;
	case FIMC_IS_VIDEO_DCP1C_NUM:
		index = PARAM_DCP_OUTPUT_SLAVE;
		target_addr = ldr_frame->shot->uctl.scalerUd.scpTargetAddress; /* TODO: name */
		break;
	case FIMC_IS_VIDEO_DCP2C_NUM:
		index = PARAM_DCP_OUTPUT_DISPARITY;
		target_addr = ldr_frame->shot->uctl.scalerUd.dxcTargetAddress;
		break;
	case FIMC_IS_VIDEO_DCP3C_NUM:
		index = PARAM_DCP_OUTPUT_MASTER_DS;
		/* FIXME: sccTargetAddress is overwrited */
		target_addr = ldr_frame->shot->uctl.scalerUd.sccTargetAddress;
		break;
	case FIMC_IS_VIDEO_DCP4C_NUM:
		index = PARAM_DCP_OUTPUT_SLAVE_DS;
		/* FIXME: scpTargetAddress is overwrited */
		target_addr = ldr_frame->shot->uctl.scalerUd.scpTargetAddress;
		break;
	default:
		mserr("vid(%d) is not matched", device, subdev, node->vid);
		ret = -EINVAL;
		goto p_err;
	}

	dma_output = fimc_is_itf_g_param(device, ldr_frame, index);

	if (node->request) {
		otcrop = (struct fimc_is_crop *)node->output.cropRegion;

		otparm.x = 0;
		otparm.y = 0;
		otparm.w = dma_output->width;
		otparm.h = dma_output->height;

		if (IS_NULL_CROP(otcrop))
			*otcrop = otparm;

		if (!COMPARE_CROP(otcrop, &otparm) ||
			!test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state) ||
			test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
			ret = fimc_is_ischain_dcxc_start(device,
				subdev,
				ldr_frame,
				queue,
				dma_output,
				otcrop,
				index,
				&lindex,
				&hindex,
				&indexes);
			if (ret) {
				merr("fimc_is_ischain_dcxc_start is fail(%d)", device, ret);
				goto p_err;
			}

			mdbg_pframe("ot_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
				otcrop->x, otcrop->y, otcrop->w, otcrop->h);
		}

		ret = fimc_is_ischain_buf_tag(device,
			subdev,
			ldr_frame,
			pixelformat,
			otcrop->w,
			otcrop->h,
			target_addr);
		if (ret) {
			mswarn("%d frame is drop", device, subdev, ldr_frame->fcount);
			node->request = 0;
		}
	} else {
		if (test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state)) {
			ret = fimc_is_ischain_dcxc_stop(device,
				subdev,
				ldr_frame,
				dma_output,
				index,
				&lindex,
				&hindex,
				&indexes);
			if (ret) {
				merr("fimc_is_ischain_dcxc_stop is fail(%d)", device, ret);
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

const struct fimc_is_subdev_ops fimc_is_subdev_dcxc_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_dcxc_cfg,
	.tag			= fimc_is_ischain_dcxc_tag,
};
