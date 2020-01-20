/*
 * Samsung Exynos SoC series FIMC-IS2 driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
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

static int fimc_is_ischain_vra_cfg(struct fimc_is_subdev *leader,
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
	struct param_otf_input *otf_input;
	struct param_dma_input *dma_input;
#ifdef ENABLE_FD_SW
	struct param_fd_config *fd_config;
#endif
	struct fimc_is_device_ischain *device;
	u32 width, height;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!leader);
	FIMC_BUG(!device);
	FIMC_BUG(!incrop);
	FIMC_BUG(!lindex);
	FIMC_BUG(!hindex);
	FIMC_BUG(!indexes);

	width = incrop->w;
	height = incrop->h;

	group = &device->group_vra;

	if (!test_bit(FIMC_IS_GROUP_INIT, &group->state)) {
		mswarn("group is NOT initialized", leader, leader);
		goto p_err;
	}

#ifdef ENABLE_FD_SW
	fd_config = fimc_is_itf_g_param(device, frame, PARAM_FD_CONFIG);

	fimc_is_lib_fd_size_check(device->fd_lib, fd_config, &leader->input,
			width, height);

	*lindex |= LOWBIT_OF(PARAM_FD_CONFIG);
	*hindex |= HIGHBIT_OF(PARAM_FD_CONFIG);
	(*indexes)++;
#endif

	otf_input = fimc_is_itf_g_param(device, frame, PARAM_FD_OTF_INPUT);
	otf_input->cmd = OTF_INPUT_COMMAND_DISABLE;
	otf_input->format = OTF_YUV_FORMAT;
	otf_input->bitwidth = OTF_INPUT_BIT_WIDTH_8BIT;
	otf_input->order = OTF_INPUT_ORDER_BAYER_GR_BG;
	otf_input->width = width;
	otf_input->height = height;
	*lindex |= LOWBIT_OF(PARAM_FD_OTF_INPUT);
	*hindex |= HIGHBIT_OF(PARAM_FD_OTF_INPUT);
	(*indexes)++;

	dma_input = fimc_is_itf_g_param(device, frame, PARAM_FD_DMA_INPUT);
	dma_input->cmd = DMA_INPUT_COMMAND_ENABLE;
	dma_input->format = DMA_INPUT_FORMAT_YUV420;
	dma_input->bitwidth = DMA_INPUT_BIT_WIDTH_8BIT;
	dma_input->order = DMA_INPUT_ORDER_CrCb;
	dma_input->plane = DMA_INPUT_PLANE_2;
	dma_input->width = width;
	dma_input->height = height;
	*lindex |= LOWBIT_OF(PARAM_FD_DMA_INPUT);
	*hindex |= HIGHBIT_OF(PARAM_FD_DMA_INPUT);
	(*indexes)++;

	leader->input.crop = *incrop;

p_err:
	return ret;
}

static int fimc_is_ischain_vra_tag(struct fimc_is_subdev *subdev,
	void *device_data,
	struct fimc_is_frame *frame,
	struct camera2_node *node)
{
	int ret = 0;
	struct fimc_is_group *group;
	struct fimc_is_crop inparm;
	struct fimc_is_crop *incrop, *otcrop;
	struct vra_param *vra_param;
#ifdef ENABLE_FD_SW
	struct fimc_is_lib_fd *lib_data;
	struct camera2_uctl *uctl;
#endif
	struct fimc_is_subdev *leader;
	struct fimc_is_device_ischain *device;
	u32 lindex, hindex, indexes;

	device = (struct fimc_is_device_ischain *)device_data;

	FIMC_BUG(!subdev);
	FIMC_BUG(!device);
	FIMC_BUG(!device->is_region);
	FIMC_BUG(!frame);
	FIMC_BUG(!frame->shot);

	incrop = (struct fimc_is_crop *)node->input.cropRegion;
	otcrop = (struct fimc_is_crop *)node->output.cropRegion;

	group = &device->group_vra;
	leader = subdev->leader;
	lindex = hindex = indexes = 0;
	vra_param = &device->is_region->parameter.vra;

#ifdef DBG_STREAMING
	mdbgd_ischain("VRA TAG\n", device);
#endif

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state)) {
		inparm.x = 0;
		inparm.y = 0;
		inparm.w = vra_param->otf_input.width;
		inparm.h = vra_param->otf_input.height;
	} else {
		inparm.x = 0;
		inparm.y = 0;
		inparm.w = vra_param->dma_input.width;
		inparm.h = vra_param->dma_input.height;
	}

	if (IS_NULL_CROP(incrop))
		*incrop = inparm;

	if (!COMPARE_CROP(incrop, &inparm)||
		test_bit(FIMC_IS_SUBDEV_FORCE_SET, &leader->state)) {
		ret = fimc_is_ischain_vra_cfg(subdev,
			device,
			frame,
			incrop,
			otcrop,
			&lindex,
			&hindex,
			&indexes);
		if (ret) {
			merr("fimc_is_ischain_vra_cfg is fail(%d)", device, ret);
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

#ifdef ENABLE_FD_SW
	lib_data = device->fd_lib->lib_data;
	uctl = &frame->shot->uctl;

	if (!test_bit(FIMC_IS_SUBDEV_RUN, &device->group_vra.leader.state))
		return ret;

	ret = fimc_is_lib_fd_map_init(device->fd_lib,
		device->minfo->kvaddr_lhfd, device->minfo->dvaddr_lhfd,
		device->minfo->kvaddr_fshared, vra_param);
	if (ret) {
		merr("[FD] fimc_is_lib_fd_map_init fail\n", device);
		return ret;
	}

	ret = fimc_is_lib_fd_create_detector(device->fd_lib, vra_param);
	if (ret) {
		merr("failed to fimc_is_lib_fd_create_detector (%d)=n", device, ret);
		return ret;
	}

	ret = fimc_is_lib_fd_convert_orientation(uctl->scalerUd.orientation,
		&lib_data->orientation);
	if (ret) {
		merr("Invalided orientation value of Host FD", device);
		return ret;
	}

	ret = fimc_is_lib_fd_select_buf(lib_data, &uctl->fdUd,
		device->minfo->kvaddr_fshared, device->minfo->dvaddr_fshared);
#endif

p_err:
	return ret;
}

const struct fimc_is_subdev_ops fimc_is_subdev_vra_ops = {
	.bypass			= NULL,
	.cfg			= fimc_is_ischain_vra_cfg,
	.tag			= fimc_is_ischain_vra_tag,
};
