/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
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

/**
 Utility function to adjust output crop size based on the
 H/W limitation of SCP scaling.
 output_crop_w and output_crop_h are call-by reference parameter,
 which contain intended cropping size. Adjusted size will be stored on
 those parameters when this function returns.
 */
static int fimc_is_ischain_scp_adjust_crop(struct fimc_is_device_ischain *device,
	struct scp_param *scp_param,
	u32 *output_crop_w, u32 *output_crop_h)
{
	int changed = 0;

	if (*output_crop_w > scp_param->otf_input.width * 8) {
		mwarn("Cannot be scaled up beyond 8 times(%d -> %d)",
			device, scp_param->otf_input.width, *output_crop_w);
		*output_crop_w = scp_param->otf_input.width * 4;
		changed |= 0x01;
	}

	if (*output_crop_h > scp_param->otf_input.height * 8) {
		mwarn("Cannot be scaled up beyond 8 times(%d -> %d)",
			device, scp_param->otf_input.height, *output_crop_h);
		*output_crop_h = scp_param->otf_input.height * 4;
		changed |= 0x02;
	}

	if (*output_crop_w < (scp_param->otf_input.width + 15) / 16) {
		mwarn("Cannot be scaled down beyond 1/16 times(%d -> %d)",
			device, scp_param->otf_input.width, *output_crop_w);
		*output_crop_w = (scp_param->otf_input.width + 15) / 16;
		changed |= 0x10;
	}

	if (*output_crop_h < (scp_param->otf_input.height + 15) / 16) {
		mwarn("Cannot be scaled down beyond 1/16 times(%d -> %d)",
			device, scp_param->otf_input.height, *output_crop_h);
		*output_crop_h = (scp_param->otf_input.height + 15) / 16;
		changed |= 0x20;
	}

	return changed;
}

static int fimc_is_ischain_scp_cfg(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct param_scaler_input_crop *input_crop;
	struct param_scaler_output_crop *output_crop;
	struct param_otf_input *otf_input;
	struct param_otf_output *otf_output;
	struct param_dma_output *dma_output;
	struct fimc_is_device_ischain *device;
	u32 scp_width, scp_height;

	device = (struct fimc_is_device_ischain *)device_data;

	scp_width = subdev->output.crop.w;
	scp_height = subdev->output.crop.h;

	otf_input = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_OTF_INPUT);
	otf_input->width = incrop->w;
	otf_input->height = incrop->h;
	otf_input->format = OTF_YUV_FORMAT;
	otf_input->bitwidth = OTF_INPUT_BIT_WIDTH_12BIT;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_OTF_INPUT);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_OTF_INPUT);
	(*indexes)++;

	input_crop = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_INPUT_CROP);
	input_crop->cmd = SCALER_CROP_COMMAND_ENABLE;
	input_crop->pos_x = 0;
	input_crop->pos_y = 0;
	input_crop->crop_width = incrop->w;
	input_crop->crop_height = incrop->h;
	input_crop->in_width = incrop->w;
	input_crop->in_height = incrop->h;
	input_crop->out_width = scp_width;
	input_crop->out_height = scp_height;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_INPUT_CROP);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_INPUT_CROP);
	(*indexes)++;

	output_crop = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_OUTPUT_CROP);
	output_crop->cmd = SCALER_CROP_COMMAND_DISABLE;
	output_crop->pos_x = 0;
	output_crop->pos_y = 0;
	output_crop->crop_width = scp_width;
	output_crop->crop_height = scp_height;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_OUTPUT_CROP);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_OUTPUT_CROP);
	(*indexes)++;

	dma_output = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_DMA_OUTPUT);
	dma_output->width = scp_width;
	dma_output->height = scp_height;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_DMA_OUTPUT);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_DMA_OUTPUT);
	(*indexes)++;

	otf_output = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_OTF_OUTPUT);
	otf_output->width = scp_width;
	otf_output->height = scp_height;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_OTF_OUTPUT);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_OTF_OUTPUT);
	(*indexes)++;

	return ret;
}

static int fimc_is_ischain_scp_update(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct fimc_is_queue *queue,
	struct scp_param *scp_param,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes,
	bool start)
{
	int ret = 0;
	struct param_dma_output *dma_output;
#ifdef SOC_VRA
	struct param_otf_input *otf_input;
#endif
	struct param_otf_output *otf_output;
	struct param_scaler_input_crop *input_crop;
	struct param_scaler_output_crop *output_crop;
	struct param_scaler_imageeffect *imageeffect;
	struct fimc_is_subdev *leader;
	struct fimc_is_group *group;
	u32 crange;

	FIMC_BUG(!queue);
	FIMC_BUG(!queue->framecfg.format);

	fimc_is_ischain_scp_adjust_crop(device, scp_param, &otcrop->w, &otcrop->h);

	if (queue->framecfg.quantization == V4L2_QUANTIZATION_FULL_RANGE) {
		crange = SCALER_OUTPUT_YUV_RANGE_FULL;
		mdbg_pframe("CRange:W(%d)\n", device, subdev, frame, start);
	} else {
		crange = SCALER_OUTPUT_YUV_RANGE_NARROW;
		mdbg_pframe("CRange:N(%d)\n", device, subdev, frame, start);
	}

	input_crop = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_INPUT_CROP);
	input_crop->cmd = SCALER_CROP_COMMAND_ENABLE;
	input_crop->pos_x = incrop->x;
	input_crop->pos_y = incrop->y;
	input_crop->crop_width = incrop->w;
	input_crop->crop_height = incrop->h;
	input_crop->in_width = scp_param->otf_input.width;
	input_crop->in_height = scp_param->otf_input.height;
	input_crop->out_width = otcrop->w;
	input_crop->out_height = otcrop->h;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_INPUT_CROP);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_INPUT_CROP);
	(*indexes)++;

	/*
	 * scaler can't apply stride to each plane, only y plane.
	 * basically cb, cr plane should be half of y plane,
	 * and it's automatically set
	 *
	 * 3 plane : all plane should be 8 or 16 stride
	 * 2 plane : y plane should be 32, 16 stride, others should be half stride of y
	 * 1 plane : all plane should be 8 stride
	 */
	/*
	 * limitation of output_crop.pos_x and pos_y
	 * YUV422 3P, YUV420 3P : pos_x and pos_y should be x2
	 * YUV422 1P : pos_x should be x2
	 */
	output_crop = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_OUTPUT_CROP);
	output_crop->cmd = SCALER_CROP_COMMAND_ENABLE;
	output_crop->pos_x = otcrop->x;
	output_crop->pos_y = otcrop->y;
	output_crop->crop_width = otcrop->w;
	output_crop->crop_height = otcrop->h;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_OUTPUT_CROP);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_OUTPUT_CROP);
	(*indexes)++;

	leader = subdev->leader;
	group = container_of(leader, struct fimc_is_group, leader);
	otf_output = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_OTF_OUTPUT);
	if (!group->gnext)
		otf_output->cmd = OTF_OUTPUT_COMMAND_ENABLE;
	else
		otf_output->cmd = OTF_OUTPUT_COMMAND_DISABLE;
	otf_output->width = otcrop->w;
	otf_output->height = otcrop->h;
	*lindex |= LOWBIT_OF(PARAM_SCALERP_OTF_OUTPUT);
	*hindex |= HIGHBIT_OF(PARAM_SCALERP_OTF_OUTPUT);
	(*indexes)++;

#ifdef SOC_VRA
	if (otf_output->cmd == OTF_OUTPUT_COMMAND_ENABLE) {
		otf_input = fimc_is_itf_g_param(device, frame, PARAM_FD_OTF_INPUT);
		otf_input->width = otcrop->w;
		otf_input->height = otcrop->h;
		*lindex |= LOWBIT_OF(PARAM_FD_OTF_INPUT);
		*hindex |= HIGHBIT_OF(PARAM_FD_OTF_INPUT);
		(*indexes)++;
	}
#endif
	dma_output = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_DMA_OUTPUT);
	if (start) {
		dma_output->cmd = DMA_OUTPUT_COMMAND_ENABLE;
		dma_output->format = queue->framecfg.format->hw_format;
		dma_output->plane = queue->framecfg.format->hw_plane;
		dma_output->order = queue->framecfg.format->hw_order;
		dma_output->width = otcrop->w;
		dma_output->height = otcrop->h;
		*lindex |= LOWBIT_OF(PARAM_SCALERP_DMA_OUTPUT);
		*hindex |= HIGHBIT_OF(PARAM_SCALERP_DMA_OUTPUT);
		(*indexes)++;

		imageeffect = fimc_is_itf_g_param(device, frame, PARAM_SCALERP_IMAGE_EFFECT);
		imageeffect->yuv_range = crange;
		*lindex |= LOWBIT_OF(PARAM_SCALERP_IMAGE_EFFECT);
		*hindex |= HIGHBIT_OF(PARAM_SCALERP_IMAGE_EFFECT);
		(*indexes)++;

		set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	} else {
		dma_output->cmd = DMA_OUTPUT_COMMAND_DISABLE;
		*lindex |= LOWBIT_OF(PARAM_SCALERP_DMA_OUTPUT);
		*hindex |= HIGHBIT_OF(PARAM_SCALERP_DMA_OUTPUT);
		(*indexes)++;

		clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	}

	return ret;
}

static int fimc_is_ischain_scp_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *ldr_frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_subdev *leader;
	struct fimc_is_queue *queue;
	struct camera2_scaler_uctl *scalerUd;
	struct scp_param *scp_param;
	struct fimc_is_crop *incrop, *otcrop, inparm, otparm;
	struct fimc_is_device_ischain *device;
	u32 lindex, hindex, indexes;
	u32 pixelformat = 0;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!device);
	FIMC_BUG(!subdev);
	FIMC_BUG(!GET_SUBDEV_QUEUE(subdev));
	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!ldr_frame->shot);

#ifdef DBG_STREAMING
	mdbgd_ischain("SCP TAG(request %d)\n", device, node->request);
#endif

	lindex = hindex = indexes = 0;
	leader = subdev->leader;
	scp_param = &device->is_region->parameter.scalerp;
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

	incrop = (struct fimc_is_crop *)node->input.cropRegion;
	otcrop = (struct fimc_is_crop *)node->output.cropRegion;

	inparm.x = scp_param->input_crop.pos_x;
	inparm.y = scp_param->input_crop.pos_y;
	inparm.w = scp_param->input_crop.crop_width;
	inparm.h = scp_param->input_crop.crop_height;

	otparm.x = scp_param->output_crop.pos_x;
	otparm.y = scp_param->output_crop.pos_y;
	otparm.w = scp_param->output_crop.crop_width;
	otparm.h = scp_param->output_crop.crop_height;

	if (IS_NULL_CROP(incrop))
		*incrop = inparm;

	if (IS_NULL_CROP(otcrop))
		*otcrop = otparm;

	if (node->request) {
		if (!COMPARE_CROP(incrop, &inparm) ||
			!COMPARE_CROP(otcrop, &otparm) ||
			!test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state) ||
			test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
			ret = fimc_is_ischain_scp_update(device,
				subdev,
				ldr_frame,
				queue,
				scp_param,
				incrop,
				otcrop,
				&lindex,
				&hindex,
				&indexes,
				true);
			if (ret) {
				merr("fimc_is_ischain_scp_update is fail(%d)", device, ret);
				goto p_err;
			}

			mdbg_pframe("in_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
				incrop->x, incrop->y, incrop->w, incrop->h);
			mdbg_pframe("ot_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
				otcrop->x, otcrop->y, otcrop->w, otcrop->h);
		}

		ret = fimc_is_ischain_buf_tag(device,
			subdev,
			ldr_frame,
			pixelformat,
			otcrop->w,
			otcrop->h,
			scalerUd->scpTargetAddress);
		if (ret) {
			mswarn("%d frame is drop", device, subdev, ldr_frame->fcount);
			node->request = 0;
		}
	} else {
		if (test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state)) {
			ret = fimc_is_ischain_scp_update(device,
				subdev,
				ldr_frame,
				queue,
				scp_param,
				incrop,
				otcrop,
				&lindex,
				&hindex,
				&indexes,
				false);
			if (ret) {
				merr("fimc_is_ischain_scp_update is fail(%d)", device, ret);
				goto p_err;
			}

			mdbg_pframe("in_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
				incrop->x, incrop->y, incrop->w, incrop->h);
			mdbg_pframe("ot_crop[%d, %d, %d, %d] off\n", device, subdev, ldr_frame,
				otcrop->x, otcrop->y, otcrop->w, otcrop->h);
		}

		scalerUd->scpTargetAddress[0] = 0;
		scalerUd->scpTargetAddress[1] = 0;
		scalerUd->scpTargetAddress[2] = 0;
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

const struct fimc_is_subdev_ops fimc_is_subdev_scp_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_scp_cfg,
	.tag			= fimc_is_ischain_scp_tag,
};
