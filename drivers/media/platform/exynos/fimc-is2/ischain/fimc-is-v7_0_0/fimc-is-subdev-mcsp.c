/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
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

#if defined(CONFIG_VIDEO_EXYNOS_SMFC) && defined(ENABLE_HWFC)
#include "../../../smfc/smfc-sync.h"
#endif

static int fimc_is_ischain_mxp_adjust_crop(struct fimc_is_device_ischain *device,
	u32 input_crop_w, u32 input_crop_h,
	u32 *output_crop_w, u32 *output_crop_h);

static int fimc_is_ischain_mxp_cfg(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct fimc_is_queue *queue;
	struct fimc_is_fmt *format;
	struct param_mcs_output *mcs_output;
	struct fimc_is_device_ischain *device;
	u32 width, height;
	u32 crange;
	int scenario_id = -1;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!device);
	FIMC_BUG(!incrop);

	queue = GET_SUBDEV_QUEUE(subdev);
	if (!queue) {
		merr("queue is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	format = queue->framecfg.format;
	if (!format) {
		mserr("format is NULL", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	width = queue->framecfg.width;
	height = queue->framecfg.height;
	fimc_is_ischain_mxp_adjust_crop(device, incrop->w, incrop->h, &width, &height);
#ifdef ENABLE_DVFS
	scenario_id = device->resourcemgr->dvfs_ctrl.static_ctrl->cur_scenario_id;
#endif

	if (queue->framecfg.quantization == V4L2_QUANTIZATION_FULL_RANGE) {
		crange = SCALER_OUTPUT_YUV_RANGE_FULL;
		msinfo("CRange:W\n", device, subdev);
	} else {
		crange = SCALER_OUTPUT_YUV_RANGE_NARROW;
		if (!FIMC_IS_SKIP_PERFRAME_LOG(scenario_id))
			msinfo("CRange:N\n", device, subdev);
	}

	mcs_output = fimc_is_itf_g_param(device, frame, subdev->param_dma_ot);

	mcs_output->otf_cmd = OTF_OUTPUT_COMMAND_ENABLE;
	mcs_output->otf_format = OTF_OUTPUT_FORMAT_YUV422;
	mcs_output->otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT;
	mcs_output->otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG;

	mcs_output->dma_bitwidth = format->hw_bitwidth;
	mcs_output->dma_format = format->hw_format;
	mcs_output->dma_order = format->hw_order;
	mcs_output->plane = format->hw_plane;

	mcs_output->crop_offset_x = incrop->x;
	mcs_output->crop_offset_y = incrop->y;
	mcs_output->crop_width = incrop->w;
	mcs_output->crop_height = incrop->h;

	mcs_output->width = width;
	mcs_output->height = height;
	/* HW spec: stride should be aligned by 16 byte. */
	mcs_output->dma_stride_y = ALIGN(max(width * format->bitsperpixel[0] / BITS_PER_BYTE,
					queue->framecfg.bytesperline[0]), 16);
	mcs_output->dma_stride_c = ALIGN(max(width * format->bitsperpixel[1] / BITS_PER_BYTE,
					queue->framecfg.bytesperline[1]), 16);

	mcs_output->yuv_range = crange;
	mcs_output->flip = (u32)queue->framecfg.flip >> 1; /* Caution: change from bitwise to enum */

#ifdef ENABLE_HWFC
	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		mcs_output->hwfc = 1; /* TODO: enum */
	else
		mcs_output->hwfc = 0; /* TODO: enum */
#endif

#ifdef SOC_VRA
	if (device->group_mcs.junction == subdev) {
		struct param_otf_input *otf_input;
		otf_input = fimc_is_itf_g_param(device, frame, PARAM_FD_OTF_INPUT);
		otf_input->width = width;
		otf_input->height = height;
		*lindex |= LOWBIT_OF(PARAM_FD_OTF_INPUT);
		*hindex |= HIGHBIT_OF(PARAM_FD_OTF_INPUT);
		(*indexes)++;
	}
#endif

	*lindex |= LOWBIT_OF(subdev->param_dma_ot);
	*hindex |= HIGHBIT_OF(subdev->param_dma_ot);
	(*indexes)++;


p_err:
	return ret;
}

#define MXP_RATIO_UP	(14)
static int fimc_is_ischain_mxp_adjust_crop(struct fimc_is_device_ischain *device,
	u32 input_crop_w, u32 input_crop_h,
	u32 *output_crop_w, u32 *output_crop_h)
{
	int changed = 0;

	if (*output_crop_w > input_crop_w * MXP_RATIO_UP) {
		mwarn("Cannot be scaled up beyond %d times(%d -> %d)",
			device, MXP_RATIO_UP, input_crop_h, *output_crop_h);
		*output_crop_h = input_crop_h * MXP_RATIO_UP;
		changed |= 0x01;
	}

	if (*output_crop_h > input_crop_h * MXP_RATIO_UP) {
		mwarn("Cannot be scaled up beyond %d times(%d -> %d)",
			device, MXP_RATIO_UP, input_crop_h, *output_crop_h);
		*output_crop_h = input_crop_h * MXP_RATIO_UP;
		changed |= 0x02;
	}

	if (*output_crop_w < (input_crop_w + 23) / 24) {
		mwarn("Cannot be scaled down beyond 1/16 times(%d -> %d)",
			device, input_crop_w, *output_crop_w);
		*output_crop_w = ALIGN((input_crop_w + 23) / 24, 2);
		changed |= 0x10;
	}

	if (*output_crop_h < (input_crop_h + 23) / 24) {
		mwarn("Cannot be scaled down beyond 1/16 times(%d -> %d)",
			device, input_crop_h, *output_crop_h);
		*output_crop_h = ALIGN((input_crop_h + 23) / 24, 2);
		changed |= 0x20;
	}

	return changed;
}

static int fimc_is_ischain_mxp_compare_size(struct fimc_is_device_ischain *device,
	struct mcs_param *mcs_param,
	struct fimc_is_crop *incrop)
{
	int changed = 0;

	if (mcs_param->input.otf_cmd == OTF_INPUT_COMMAND_ENABLE) {
		if (incrop->x + incrop->w > mcs_param->input.width) {
			mwarn("Out of crop width region(%d < %d)",
				device, mcs_param->input.width, incrop->x + incrop->w);
			incrop->x = 0;
			incrop->w = mcs_param->input.width;
			changed |= 0x01;
		}

		if (incrop->y + incrop->h > mcs_param->input.height) {
			mwarn("Out of crop height region(%d < %d)",
				device, mcs_param->input.height, incrop->y + incrop->h);
			incrop->y = 0;
			incrop->h = mcs_param->input.height;
			changed |= 0x02;
		}
	} else {
		if (incrop->x + incrop->w > mcs_param->input.dma_crop_width) {
			mwarn("Out of crop width region(%d < %d)",
				device, mcs_param->input.dma_crop_width, incrop->x + incrop->w);
			incrop->x = 0;
			incrop->w = mcs_param->input.dma_crop_width;
			changed |= 0x01;
		}

		if (incrop->y + incrop->h > mcs_param->input.dma_crop_height) {
			mwarn("Out of crop height region(%d < %d)",
				device, mcs_param->input.dma_crop_height, incrop->y + incrop->h);
			incrop->y = 0;
			incrop->h = mcs_param->input.dma_crop_height;
			changed |= 0x02;
		}
	}

	return changed;
}

static int fimc_is_ischain_mxp_start(struct fimc_is_device_ischain *device,
	struct camera2_node *node,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct fimc_is_queue *queue,
	struct mcs_param *mcs_param,
	struct param_mcs_output *mcs_output,
	struct fimc_is_crop *incrop,
	struct fimc_is_crop *otcrop,
	ulong index,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;
	struct fimc_is_fmt *format, *tmp_format;
	struct param_otf_input *otf_input;
	u32 crange;
	int scenario_id = -1;

	FIMC_BUG(!queue);
	FIMC_BUG(!queue->framecfg.format);

	format = queue->framecfg.format;

	otf_input = NULL;
#ifdef ENABLE_DVFS
	scenario_id = device->resourcemgr->dvfs_ctrl.static_ctrl->cur_scenario_id;
#endif

	/* if output DS, skip check a incrop & input mcs param
	 * because, DS input size set to preview port output size
	 */
	if ((index - PARAM_MCS_OUTPUT0) != MCSC_OUTPUT_DS)
		fimc_is_ischain_mxp_compare_size(device, mcs_param, incrop);

	fimc_is_ischain_mxp_adjust_crop(device, incrop->w, incrop->h, &otcrop->w, &otcrop->h);

	if (queue->framecfg.quantization == V4L2_QUANTIZATION_FULL_RANGE) {
		crange = SCALER_OUTPUT_YUV_RANGE_FULL;
		mdbg_pframe("CRange:W\n", device, subdev, frame);
	} else {
		crange = SCALER_OUTPUT_YUV_RANGE_NARROW;
		if (!FIMC_IS_SKIP_PERFRAME_LOG(scenario_id))
			mdbg_pframe("CRange:N\n", device, subdev, frame);
	}

	if (node->pixelformat && format->pixelformat != node->pixelformat) { /* per-frame control for RGB */
		tmp_format = fimc_is_find_format((u32)node->pixelformat, 0);
		if (tmp_format) {
			mdbg_pframe("pixelformat is changed(%c%c%c%c->%c%c%c%c)\n",
				device, subdev, frame,
				(char)((format->pixelformat >> 0) & 0xFF),
				(char)((format->pixelformat >> 8) & 0xFF),
				(char)((format->pixelformat >> 16) & 0xFF),
				(char)((format->pixelformat >> 24) & 0xFF),
				(char)((tmp_format->pixelformat >> 0) & 0xFF),
				(char)((tmp_format->pixelformat >> 8) & 0xFF),
				(char)((tmp_format->pixelformat >> 16) & 0xFF),
				(char)((tmp_format->pixelformat >> 24) & 0xFF));
			queue->framecfg.format = format = tmp_format;
		} else {
			mdbg_pframe("pixelformat is not found(%c%c%c%c)\n",
				device, subdev, frame,
				(char)((node->pixelformat >> 0) & 0xFF),
				(char)((node->pixelformat >> 8) & 0xFF),
				(char)((node->pixelformat >> 16) & 0xFF),
				(char)((node->pixelformat >> 24) & 0xFF));
		}
	}

	mcs_output->otf_format = OTF_OUTPUT_FORMAT_YUV422;
	mcs_output->otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT;
	mcs_output->otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG;

	mcs_output->dma_cmd = DMA_OUTPUT_COMMAND_ENABLE;
	mcs_output->dma_bitwidth = format->hw_bitwidth;
	mcs_output->dma_format = format->hw_format;
	mcs_output->dma_order = format->hw_order;
	mcs_output->plane = format->hw_plane;

	mcs_output->crop_offset_x = incrop->x; /* per frame */
	mcs_output->crop_offset_y = incrop->y; /* per frame */
	mcs_output->crop_width = incrop->w; /* per frame */
	mcs_output->crop_height = incrop->h; /* per frame */

	mcs_output->width = otcrop->w; /* per frame */
	mcs_output->height = otcrop->h; /* per frame */
	/* HW spec: stride should be aligned by 16 byte. */

	mcs_output->dma_stride_y = ALIGN(max(otcrop->w * format->bitsperpixel[0] / BITS_PER_BYTE,
					queue->framecfg.bytesperline[0]), 16);
	mcs_output->dma_stride_c = ALIGN(max(otcrop->w * format->bitsperpixel[1] / BITS_PER_BYTE,
					queue->framecfg.bytesperline[1]), 16);

	mcs_output->yuv_range = crange;
	mcs_output->flip = (u32)queue->framecfg.flip >> 1; /* Caution: change from bitwise to enum */

#ifdef ENABLE_HWFC
	if (test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
		mcs_output->hwfc = 1; /* TODO: enum */
	else
		mcs_output->hwfc = 0; /* TODO: enum */
#endif

	*lindex |= LOWBIT_OF(index);
	*hindex |= HIGHBIT_OF(index);
	(*indexes)++;

#ifdef SOC_VRA
	if (device->group_mcs.junction == subdev) {
		otf_input = fimc_is_itf_g_param(device, frame, PARAM_FD_OTF_INPUT);
		otf_input->width = otcrop->w;
		otf_input->height = otcrop->h;
		*lindex |= LOWBIT_OF(PARAM_FD_OTF_INPUT);
		*hindex |= HIGHBIT_OF(PARAM_FD_OTF_INPUT);
		(*indexes)++;
	}
#endif

	set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

	return ret;
}

static int fimc_is_ischain_mxp_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct param_mcs_output *mcs_output,
	ulong index,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
	int ret = 0;

	mdbgd_ischain("%s\n", device, __func__);

	mcs_output->dma_cmd = DMA_OUTPUT_COMMAND_DISABLE;
	*lindex |= LOWBIT_OF(index);
	*hindex |= HIGHBIT_OF(index);
	(*indexes)++;

	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);

	return ret;
}

static void fimc_is_ischain_mxp_otf_enable(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame,
	struct param_mcs_output *mcs_output,
	ulong index,
	u32 *lindex,
	u32 *hindex,
	u32 *indexes)
{
#if !defined(USE_VRA_OTF)
	struct param_mcs_input *input;

	input = fimc_is_itf_g_param(device, frame, PARAM_MCS_INPUT);

	mcs_output->otf_cmd = OTF_OUTPUT_COMMAND_ENABLE;

	mcs_output->otf_format = OTF_OUTPUT_FORMAT_YUV422;
	mcs_output->otf_bitwidth = OTF_OUTPUT_BIT_WIDTH_8BIT;
	mcs_output->otf_order = OTF_OUTPUT_ORDER_BAYER_GR_BG;

	mcs_output->crop_offset_x = 0;
	mcs_output->crop_offset_y = 0;

	if (input->otf_cmd == OTF_INPUT_COMMAND_ENABLE) {
		mcs_output->crop_width = input->width;
		mcs_output->crop_height = input->height;
	} else {
		mcs_output->crop_width = input->dma_crop_width;
		mcs_output->crop_height = input->dma_crop_height;
	}

	/* HACK */
	if (mcs_output->crop_width > 640 && mcs_output->crop_height > 480) {
		mcs_output->width = mcs_output->crop_width;
		mcs_output->height = mcs_output->crop_height;
	} else {
		mcs_output->width = 640;
		mcs_output->height = 480;
	}

	*lindex |= LOWBIT_OF(index);
	*hindex |= HIGHBIT_OF(index);
	(*indexes)++;

	mdbg_pframe("OTF only enable [%d, %d, %d, %d]-->[%d, %d]\n", device, subdev, frame,
		mcs_output->crop_offset_x, mcs_output->crop_offset_y,
		mcs_output->crop_width, mcs_output->crop_height,
		mcs_output->width, mcs_output->height);
#endif
}

static void fimc_is_ischain_mxp_otf_disable(struct fimc_is_device_ischain *device,
		struct fimc_is_subdev *subdev,
		struct fimc_is_frame *frame,
		struct param_mcs_output *mcs_output,
		ulong index,
		u32 *lindex,
		u32 *hindex,
		u32 *indexes)
{
#if !defined(USE_VRA_OTF)
	mcs_output->otf_cmd = OTF_OUTPUT_COMMAND_DISABLE;

	*lindex |= LOWBIT_OF(index);
	*hindex |= HIGHBIT_OF(index);
	(*indexes)++;

	mdbg_pframe("OTF only disable\n", device, subdev, frame);
#endif
}

static int fimc_is_ischain_mxp_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *ldr_frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_group *head;
	struct fimc_is_subdev *leader;
	struct fimc_is_queue *queue;
	struct mcs_param *mcs_param;
	struct param_mcs_output *mcs_output;
	struct fimc_is_crop *incrop, *otcrop, inparm, otparm;
	struct fimc_is_device_ischain *device;
	u32 index, lindex, hindex, indexes;
	u32 pixelformat = 0;
	u32 *target_addr;
	bool change_pixelformat = false;
	int scenario_id = -1;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!subdev);
	FIMC_BUG(!GET_SUBDEV_QUEUE(subdev));
	FIMC_BUG(!ldr_frame);
	FIMC_BUG(!ldr_frame->shot);
	FIMC_BUG(!node);

	mdbgs_ischain(4, "MXP TAG(request %d)\n", device, node->request);

	lindex = hindex = indexes = 0;
	leader = subdev->leader;
	mcs_param = &device->is_region->parameter.mcs;
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
	case FIMC_IS_VIDEO_M0P_NUM:
		index = PARAM_MCS_OUTPUT0;
		target_addr = ldr_frame->sc0TargetAddress;
		break;
	case FIMC_IS_VIDEO_M1P_NUM:
		index = PARAM_MCS_OUTPUT1;
		target_addr = ldr_frame->sc1TargetAddress;
		break;
	case FIMC_IS_VIDEO_M2P_NUM:
		index = PARAM_MCS_OUTPUT2;
		target_addr = ldr_frame->sc2TargetAddress;
		break;
	case FIMC_IS_VIDEO_M3P_NUM:
		index = PARAM_MCS_OUTPUT3;
		target_addr = ldr_frame->sc3TargetAddress;
		break;
	case FIMC_IS_VIDEO_M4P_NUM:
		index = PARAM_MCS_OUTPUT4;
		target_addr = ldr_frame->sc4TargetAddress;
		break;
	case FIMC_IS_VIDEO_M5P_NUM:
		index = PARAM_MCS_OUTPUT5;
		target_addr = ldr_frame->sc5TargetAddress;
		break;
	default:
		mserr("vid(%d) is not matched", device, subdev, node->vid);
		ret = -EINVAL;
		goto p_err;
	}

	mcs_output = fimc_is_itf_g_param(device, ldr_frame, index);

#ifdef ENABLE_DVFS
	scenario_id = device->resourcemgr->dvfs_ctrl.static_ctrl->cur_scenario_id;
#endif
	if (node->request) {
		incrop = (struct fimc_is_crop *)node->input.cropRegion;
		otcrop = (struct fimc_is_crop *)node->output.cropRegion;
		if (node->pixelformat) {
			change_pixelformat = !COMPARE_FORMAT(pixelformat, node->pixelformat);
			pixelformat = node->pixelformat;
		}

		inparm.x = mcs_output->crop_offset_x;
		inparm.y = mcs_output->crop_offset_y;
		inparm.w = mcs_output->crop_width;
		inparm.h = mcs_output->crop_height;

		otparm.x = 0;
		otparm.y = 0;
		otparm.w = mcs_output->width;
		otparm.h = mcs_output->height;

		if (IS_NULL_CROP(incrop))
			*incrop = inparm;

		if (IS_NULL_CROP(otcrop))
			*otcrop = otparm;

		if (!COMPARE_CROP(incrop, &inparm) ||
			!COMPARE_CROP(otcrop, &otparm) ||
			change_pixelformat ||
			!test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state) ||
			test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
			ret = fimc_is_ischain_mxp_start(device,
				node,
				subdev,
				ldr_frame,
				queue,
				mcs_param,
				mcs_output,
				incrop,
				otcrop,
				index,
				&lindex,
				&hindex,
				&indexes);
			if (ret) {
				merr("fimc_is_ischain_mxp_start is fail(%d)", device, ret);
				goto p_err;
			}

			if (!FIMC_IS_SKIP_PERFRAME_LOG(scenario_id)) {
				mdbg_pframe("in_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
					incrop->x, incrop->y, incrop->w, incrop->h);
				mdbg_pframe("ot_crop[%d, %d, %d, %d]\n", device, subdev, ldr_frame,
					otcrop->x, otcrop->y, otcrop->w, otcrop->h);
			}
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
		} else {
			/*
			 * For supporting multi input to single output.
			 * But this function is not supported in full OTF chain.
			 */
			if (device->group_mcs.head)
				head = device->group_mcs.head;
			else
				head = &device->group_mcs;

			if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state) && (head->asyn_shots == 1)) {
				ret = down_interruptible(&subdev->vctx->video->smp_multi_input);
				if (ret)
					mswarn(" smp_multi_input down fail(%d)", device, subdev, ret);
				else
					subdev->vctx->video->try_smp = true;

			}
		}
	} else {
		if (test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state)) {
			ret = fimc_is_ischain_mxp_stop(device,
				subdev,
				ldr_frame,
				mcs_output,
				index,
				&lindex,
				&hindex,
				&indexes);
			if (ret) {
				merr("fimc_is_ischain_mxp_stop is fail(%d)", device, ret);
				goto p_err;
			}

			if (!FIMC_IS_SKIP_PERFRAME_LOG(scenario_id))
				mdbg_pframe(" off\n", device, subdev, ldr_frame);
		}

		if ((node->vid - FIMC_IS_VIDEO_M0P_NUM)
			== (ldr_frame->shot->uctl.scalerUd.mcsc_sub_blk_port[INTERFACE_TYPE_DS]))
			fimc_is_ischain_mxp_otf_enable(device,
					subdev,
					ldr_frame,
					mcs_output,
					index,
					&lindex,
					&hindex,
					&indexes);
		else if (mcs_output->otf_cmd == OTF_OUTPUT_COMMAND_ENABLE)
			fimc_is_ischain_mxp_otf_disable(device,
					subdev,
					ldr_frame,
					mcs_output,
					index,
					&lindex,
					&hindex,
					&indexes);

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

#if defined(CONFIG_VIDEO_EXYNOS_SMFC) && defined(ENABLE_HWFC)
	switch (node->vid) {
	case FIMC_IS_VIDEO_M0P_NUM:
		break;
	case FIMC_IS_VIDEO_M1P_NUM:
		break;
	case FIMC_IS_VIDEO_M2P_NUM:
		break;
	case FIMC_IS_VIDEO_M3P_NUM:
		ret = exynos_smfc_wait_done(mcs_output->hwfc);
		break;
	case FIMC_IS_VIDEO_M4P_NUM:
		if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state))
			ret = exynos_smfc_wait_done(mcs_output->hwfc);
		break;
	case FIMC_IS_VIDEO_M5P_NUM:
		break;
	default:
		mserr("vid(%d) is not matched", device, subdev, node->vid);
		ret = -EINVAL;
		goto p_err;
	}

	if (ret) {
		mrerr("exynos_smfc_wait_done is fail(%d)", device, ldr_frame, ret);
		goto p_err;
	}
#endif


p_err:
	return ret;
}

const struct fimc_is_subdev_ops fimc_is_subdev_mcsp_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_mxp_cfg,
	.tag			= fimc_is_ischain_mxp_tag,
};
