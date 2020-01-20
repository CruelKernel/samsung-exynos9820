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

#include "fimc-is-core.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-subdev-ctrl.h"
#include "fimc-is-config.h"
#include "fimc-is-param.h"
#include "fimc-is-video.h"
#include "fimc-is-type.h"
#include "fimc-is-dvfs.h"

static void fimc_is_ischain_dis_ctrl(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes,
	bool full_bypass)
{
	struct param_control *control;

	control = NULL;

#ifdef ENABLE_FULL_BYPASS
	full_bypass = true;
#endif
	/* Configue Control */
	control = fimc_is_itf_g_param(device, NULL, PARAM_TPU_CONTROL);

	if (full_bypass) {
		control->cmd = CONTROL_COMMAND_STOP;
		control->bypass = CONTROL_BYPASS_ENABLE;
		minfo("[F%d] TPU FULL BYPASS\n", device, frame ? frame->fcount : -1);
	} else {
		control->cmd = CONTROL_COMMAND_START;
		control->bypass = CONTROL_BYPASS_DISABLE;
#ifdef ENABLE_DNR_IN_TPU
		device->is_region->shared[350] = device->minfo->dvaddr_tpu;
		control->buffer_number = SIZE_DNR_INTERNAL_BUF * NUM_DNR_INTERNAL_BUF;
		control->buffer_address = device->minfo->dvaddr_tpu;
#else
		control->buffer_number = 0;
		control->buffer_address = 0;
#endif
	}
}

static int fimc_is_ischain_dis_cfg(struct fimc_is_subdev *leader,
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
	struct param_otf_input *otf_input;
	struct param_otf_output *otf_output;
	struct param_dma_input *dma_input;
	struct fimc_is_device_ischain *device;
	u32 width, height, dis_width, dis_height;
	int fps;
	bool full_bypass;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!leader);
	FIMC_BUG(!device);
	FIMC_BUG(!incrop);
	FIMC_BUG(!lindex);
	FIMC_BUG(!hindex);
	FIMC_BUG(!indexes);

	otf_input = NULL;
	otf_output = NULL;
	dma_input = NULL;
	width = incrop->w;
	height = incrop->h;
	dis_width = otcrop->w;
	dis_height = otcrop->h;
	group = &device->group_dis;
	otf_output = NULL;
	dma_input = NULL;
	queue = GET_SUBDEV_QUEUE(leader);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	fps = fimc_is_sensor_g_framerate(device->sensor);

	if (fps > 60)
		full_bypass = true;
	else
		full_bypass = false;

	/* control setting */
	fimc_is_ischain_dis_ctrl(device, frame, lindex, hindex, indexes, full_bypass);

	otf_input = fimc_is_itf_g_param(device, frame, PARAM_TPU_OTF_INPUT);
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		otf_input->cmd = OTF_INPUT_COMMAND_ENABLE;
	else
		otf_input->cmd = OTF_INPUT_COMMAND_DISABLE;
	otf_input->width = width;
	otf_input->height = height;
	otf_input->format = OTF_YUV_FORMAT;
	*lindex |= LOWBIT_OF(PARAM_TPU_OTF_INPUT);
	*hindex |= HIGHBIT_OF(PARAM_TPU_OTF_INPUT);
	(*indexes)++;

	dma_input = fimc_is_itf_g_param(device, frame, PARAM_TPU_DMA_INPUT);
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		dma_input->cmd = DMA_INPUT_COMMAND_DISABLE;
	else
		dma_input->cmd = DMA_INPUT_COMMAND_ENABLE;
	dma_input->width = width;
	dma_input->height = height;
	dma_input->format = DMA_INPUT_FORMAT_YUV422_CHUNKER;
	*lindex |= LOWBIT_OF(PARAM_TPU_DMA_INPUT);
	*hindex |= HIGHBIT_OF(PARAM_TPU_DMA_INPUT);
	(*indexes)++;

	otf_output = fimc_is_itf_g_param(device, frame, PARAM_TPU_OTF_OUTPUT);
	otf_output->width = dis_width;
	otf_output->height = dis_height;
	otf_output->format = OTF_YUV_FORMAT;
	*lindex |= LOWBIT_OF(PARAM_TPU_OTF_OUTPUT);
	*hindex |= HIGHBIT_OF(PARAM_TPU_OTF_OUTPUT);
	(*indexes)++;

	leader->input.crop = *incrop;

p_err:
	return ret;
}

static int fimc_is_ischain_dis_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct tpu_param *tpu_param;
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

	mdbgs_ischain(4, "DIS TAG\n", device);

	incrop = (struct fimc_is_crop *)node->input.cropRegion;
	otcrop = (struct fimc_is_crop *)node->output.cropRegion;

	group = &device->group_dis;
	leader = subdev->leader;
	lindex = hindex = indexes = 0;
	tpu_param = &device->is_region->parameter.tpu;

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
		inparm.x = 0;
		inparm.y = 0;
		inparm.w = tpu_param->otf_input.width;
		inparm.h = tpu_param->otf_input.height;
	} else {
		inparm.x = 0;
		inparm.y = 0;
		inparm.w = tpu_param->dma_input.width;
		inparm.h = tpu_param->dma_input.height;
	}

	if (IS_NULL_CROP(incrop))
		*incrop = inparm;

	if (!COMPARE_CROP(incrop, &inparm)||
		test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
		ret = fimc_is_ischain_dis_cfg(subdev,
			device,
			frame,
			incrop,
			otcrop,
			&lindex,
			&hindex,
			&indexes);
		if (ret) {
			merr("fimc_is_ischain_dis_cfg is fail(%d)", device, ret);
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

const struct fimc_is_subdev_ops fimc_is_subdev_dis_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_dis_cfg,
	.tag			= fimc_is_ischain_dis_tag,
};
