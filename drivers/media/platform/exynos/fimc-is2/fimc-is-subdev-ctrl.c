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

 #include <linux/module.h>

#include "fimc-is-core.h"
#include "fimc-is-param.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-debug.h"

struct fimc_is_subdev * video2subdev(enum fimc_is_subdev_device_type device_type,
	void *device, u32 vid)
{
	struct fimc_is_subdev *subdev = NULL;
	struct fimc_is_device_sensor *sensor = NULL;
	struct fimc_is_device_ischain *ischain = NULL;

	if (device_type == FIMC_IS_SENSOR_SUBDEV) {
		sensor = (struct fimc_is_device_sensor *)device;
	} else {
		ischain = (struct fimc_is_device_ischain *)device;
		sensor = ischain->sensor;
	}

	switch (vid) {
	case FIMC_IS_VIDEO_SS0VC0_NUM:
	case FIMC_IS_VIDEO_SS1VC0_NUM:
	case FIMC_IS_VIDEO_SS2VC0_NUM:
	case FIMC_IS_VIDEO_SS3VC0_NUM:
	case FIMC_IS_VIDEO_SS4VC0_NUM:
	case FIMC_IS_VIDEO_SS5VC0_NUM:
		subdev = &sensor->ssvc0;
		break;
	case FIMC_IS_VIDEO_SS0VC1_NUM:
	case FIMC_IS_VIDEO_SS1VC1_NUM:
	case FIMC_IS_VIDEO_SS2VC1_NUM:
	case FIMC_IS_VIDEO_SS3VC1_NUM:
	case FIMC_IS_VIDEO_SS4VC1_NUM:
	case FIMC_IS_VIDEO_SS5VC1_NUM:
		subdev = &sensor->ssvc1;
		break;
	case FIMC_IS_VIDEO_SS0VC2_NUM:
	case FIMC_IS_VIDEO_SS1VC2_NUM:
	case FIMC_IS_VIDEO_SS2VC2_NUM:
	case FIMC_IS_VIDEO_SS3VC2_NUM:
	case FIMC_IS_VIDEO_SS4VC2_NUM:
	case FIMC_IS_VIDEO_SS5VC2_NUM:
		subdev = &sensor->ssvc2;
		break;
	case FIMC_IS_VIDEO_SS0VC3_NUM:
	case FIMC_IS_VIDEO_SS1VC3_NUM:
	case FIMC_IS_VIDEO_SS2VC3_NUM:
	case FIMC_IS_VIDEO_SS3VC3_NUM:
	case FIMC_IS_VIDEO_SS4VC3_NUM:
	case FIMC_IS_VIDEO_SS5VC3_NUM:
		subdev = &sensor->ssvc3;
		break;
	case FIMC_IS_VIDEO_30S_NUM:
	case FIMC_IS_VIDEO_31S_NUM:
		subdev = &ischain->group_3aa.leader;
		break;
	case FIMC_IS_VIDEO_30C_NUM:
	case FIMC_IS_VIDEO_31C_NUM:
		subdev = &ischain->txc;
		break;
	case FIMC_IS_VIDEO_30P_NUM:
	case FIMC_IS_VIDEO_31P_NUM:
		subdev = &ischain->txp;
		break;
	case FIMC_IS_VIDEO_30F_NUM:
	case FIMC_IS_VIDEO_31F_NUM:
		subdev = &ischain->txf;
		break;
	case FIMC_IS_VIDEO_30G_NUM:
	case FIMC_IS_VIDEO_31G_NUM:
		subdev = &ischain->txg;
		break;
	case FIMC_IS_VIDEO_I0S_NUM:
	case FIMC_IS_VIDEO_I1S_NUM:
		subdev = &ischain->group_isp.leader;
		break;
	case FIMC_IS_VIDEO_I0C_NUM:
	case FIMC_IS_VIDEO_I1C_NUM:
		subdev = &ischain->ixc;
		break;
	case FIMC_IS_VIDEO_I0P_NUM:
	case FIMC_IS_VIDEO_I1P_NUM:
		subdev = &ischain->ixp;
		break;
	case FIMC_IS_VIDEO_ME0C_NUM:
	case FIMC_IS_VIDEO_ME1C_NUM:
		subdev = &ischain->mexc;
		break;
	case FIMC_IS_VIDEO_D0S_NUM:
	case FIMC_IS_VIDEO_D1S_NUM:
		subdev = &ischain->group_dis.leader;
		break;
	case FIMC_IS_VIDEO_D0C_NUM:
	case FIMC_IS_VIDEO_D1C_NUM:
		subdev = &ischain->dxc;
		break;
	case FIMC_IS_VIDEO_DCP0S_NUM:
		subdev = &ischain->group_dcp.leader;
		break;
	case FIMC_IS_VIDEO_DCP1S_NUM:
		subdev = &ischain->dc1s;
		break;
	case FIMC_IS_VIDEO_DCP0C_NUM:
		subdev = &ischain->dc0c;
		break;
	case FIMC_IS_VIDEO_DCP1C_NUM:
		subdev = &ischain->dc1c;
		break;
	case FIMC_IS_VIDEO_DCP2C_NUM:
		subdev = &ischain->dc2c;
		break;
	case FIMC_IS_VIDEO_DCP3C_NUM:
		subdev = &ischain->dc3c;
		break;
	case FIMC_IS_VIDEO_DCP4C_NUM:
		subdev = &ischain->dc4c;
		break;
	case FIMC_IS_VIDEO_M0S_NUM:
	case FIMC_IS_VIDEO_M1S_NUM:
		subdev = &ischain->group_mcs.leader;
		break;
	case FIMC_IS_VIDEO_M0P_NUM:
		subdev = &ischain->m0p;
		break;
	case FIMC_IS_VIDEO_M1P_NUM:
		subdev = &ischain->m1p;
		break;
	case FIMC_IS_VIDEO_M2P_NUM:
		subdev = &ischain->m2p;
		break;
	case FIMC_IS_VIDEO_M3P_NUM:
		subdev = &ischain->m3p;
		break;
	case FIMC_IS_VIDEO_M4P_NUM:
		subdev = &ischain->m4p;
		break;
	case FIMC_IS_VIDEO_M5P_NUM:
		subdev = &ischain->m5p;
		break;
	case FIMC_IS_VIDEO_VRA_NUM:
		subdev = &ischain->group_vra.leader;
		break;
	default:
		err("[%d] vid %d is NOT found", ((device_type == FIMC_IS_SENSOR_SUBDEV) ?
				 (ischain ? ischain->instance : 0) : (sensor ? sensor->instance : 0)), vid);
		break;
	}

	return subdev;
}

int fimc_is_subdev_probe(struct fimc_is_subdev *subdev,
	u32 instance,
	u32 id,
	char *name,
	const struct fimc_is_subdev_ops *sops)
{
	FIMC_BUG(!subdev);
	FIMC_BUG(!name);

	subdev->id = id;
	subdev->instance = instance;
	subdev->ops = sops;
	memset(subdev->name, 0x0, sizeof(subdev->name));
	strncpy(subdev->name, name, sizeof(char[3]));
	clear_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_START, &subdev->state);

	/* for internal use */
	clear_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &subdev->state);

	return 0;
}

int fimc_is_subdev_open(struct fimc_is_subdev *subdev,
	struct fimc_is_video_ctx *vctx,
	void *ctl_data)
{
	int ret = 0;
	struct fimc_is_video *video = GET_VIDEO(vctx);
	const struct param_control *init_ctl = (const struct param_control *)ctl_data;

	FIMC_BUG(!subdev);

	if (test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state)) {
		mserr("already open", subdev, subdev);
		ret = -EPERM;
		goto p_err;
	}

	subdev->vctx = vctx;
	subdev->vid = (video) ? video->id : 0;
	subdev->cid = CAPTURE_NODE_MAX;
	subdev->input.width = 0;
	subdev->input.height = 0;
	subdev->input.crop.x = 0;
	subdev->input.crop.y = 0;
	subdev->input.crop.w = 0;
	subdev->input.crop.h = 0;
	subdev->output.width = 0;
	subdev->output.height = 0;
	subdev->output.crop.x = 0;
	subdev->output.crop.y = 0;
	subdev->output.crop.w = 0;
	subdev->output.crop.h = 0;

	if (init_ctl) {
		set_bit(FIMC_IS_SUBDEV_START, &subdev->state);

		if (subdev->id == ENTRY_VRA) {
			/* vra only control by command for enabling or disabling */
			if (init_ctl->cmd == CONTROL_COMMAND_STOP)
				clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
			else
				set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
		} else {
			if (init_ctl->bypass == CONTROL_BYPASS_ENABLE)
				clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
			else
				set_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
		}
	} else {
		clear_bit(FIMC_IS_SUBDEV_START, &subdev->state);
		clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	}

	set_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state);

p_err:
	return ret;
}

/*
 * DMA abstraction:
 * A overlapped use case of DMA should be detected.
 */
static int fimc_is_sensor_check_subdev_open(struct fimc_is_device_sensor *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int i;
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *all_sensor;
	struct fimc_is_device_sensor *each_sensor;

	FIMC_BUG(!device);
	FIMC_BUG(!subdev);

	core = device->private_data;
	all_sensor = fimc_is_get_sensor_device(core);
	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		each_sensor = &all_sensor[i];
		if (each_sensor == device)
			continue;

		if (each_sensor->dma_abstract == false)
			continue;

		if (test_bit(FIMC_IS_SENSOR_OPEN, &each_sensor->state)) {
			if (test_bit(FIMC_IS_SUBDEV_OPEN, &each_sensor->ssvc0.state)) {
				if (each_sensor->ssvc0.dma_ch[0] == subdev->dma_ch[0]) {
					merr("vc0 dma(%d) is overlapped with another sensor(I:%d).\n",
						device, subdev->dma_ch[0], i);
					goto err_check_vc_open;
				}
			}

			if (test_bit(FIMC_IS_SUBDEV_OPEN, &each_sensor->ssvc1.state)) {
				if (each_sensor->ssvc1.dma_ch[0] == subdev->dma_ch[0]) {
					merr("vc1 dma(%d) is overlapped with another sensor(I:%d).\n",
						device, subdev->dma_ch[0], i);
					goto err_check_vc_open;
				}
			}

			if (test_bit(FIMC_IS_SUBDEV_OPEN, &each_sensor->ssvc2.state)) {
				if (each_sensor->ssvc2.dma_ch[0] == subdev->dma_ch[0]) {
					merr("vc2 dma(%d) is overlapped with another sensor(I:%d).\n",
						device, subdev->dma_ch[0], i);
					goto err_check_vc_open;
				}
			}

			if (test_bit(FIMC_IS_SUBDEV_OPEN, &each_sensor->ssvc3.state)) {
				if (each_sensor->ssvc3.dma_ch[0] == subdev->dma_ch[0]) {
					merr("vc3 dma(%d) is overlapped with another sensor(I:%d).\n",
						device, subdev->dma_ch[0], i);
					goto err_check_vc_open;
				}
			}
		}
	}

	ret = fimc_is_subdev_open(subdev, vctx, NULL);
	if (ret) {
		merr("fimc_is_subdev_open is fail(%d)", device, ret);
		goto err_subdev_open;
	}
	fimc_is_put_sensor_device(core);

	return 0;

err_subdev_open:
err_check_vc_open:
	fimc_is_put_sensor_device(core);
	return ret;
}
int fimc_is_sensor_subdev_open(struct fimc_is_device_sensor *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	subdev = video2subdev(FIMC_IS_SENSOR_SUBDEV, (void *)device, GET_VIDEO(vctx)->id);
	if (!subdev) {
		merr("video2subdev is fail", device);
		ret = -EINVAL;
		goto err_video2subdev;
	}

	ret = fimc_is_sensor_check_subdev_open(device, subdev, vctx);
	if (ret) {
		merr("fimc_is_sensor_check_subdev_open is fail", device);
		ret = -EINVAL;
		goto err_check_subdev_open;
	}

	vctx->subdev = subdev;

	return 0;

err_check_subdev_open:
err_video2subdev:
	return ret;
}

int fimc_is_ischain_subdev_open(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	int ret_err = 0;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));

	subdev = video2subdev(FIMC_IS_ISCHAIN_SUBDEV, (void *)device, GET_VIDEO(vctx)->id);
	if (!subdev) {
		merr("video2subdev is fail", device);
		ret = -EINVAL;
		goto err_video2subdev;
	}

	vctx->subdev = subdev;

	ret = fimc_is_subdev_open(subdev, vctx, NULL);
	if (ret) {
		merr("fimc_is_subdev_open is fail(%d)", device, ret);
		goto err_subdev_open;
	}

	ret = fimc_is_ischain_open_wrap(device, false);
	if (ret) {
		merr("fimc_is_ischain_open_wrap is fail(%d)", device, ret);
		fimc_is_subdev_close(subdev);
		goto err_ischain_open;
	}

	return 0;

err_ischain_open:
	ret_err = fimc_is_subdev_close(subdev);
	if (ret_err)
		merr("fimc_is_subdev_close is fail(%d)", device, ret_err);
err_subdev_open:
err_video2subdev:
	return ret;
}

int fimc_is_subdev_close(struct fimc_is_subdev *subdev)
{
	int ret = 0;

	if (!test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state)) {
		mserr("subdev is already close", subdev, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	subdev->leader = NULL;
	subdev->vctx = NULL;
	subdev->vid = 0;

	clear_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_START, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_FORCE_SET, &subdev->state);

	/* for internal use */
	clear_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &subdev->state);

p_err:
	return 0;
}

int fimc_is_sensor_subdev_close(struct fimc_is_device_sensor *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);

	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	vctx->subdev = NULL;

	if (test_bit(FIMC_IS_SENSOR_FRONT_START, &device->state)) {
		merr("sudden close, call sensor_front_stop()\n", device);

		ret = fimc_is_sensor_front_stop(device);
		if (ret)
			merr("fimc_is_sensor_front_stop is fail(%d)", device, ret);
	}

	ret = fimc_is_subdev_close(subdev);
	if (ret)
		merr("fimc_is_subdev_close is fail(%d)", device, ret);

p_err:
	return ret;
}

int fimc_is_ischain_subdev_close(struct fimc_is_device_ischain *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!vctx);

	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	vctx->subdev = NULL;

	ret = fimc_is_subdev_close(subdev);
	if (ret) {
		merr("fimc_is_subdev_close is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_ischain_close_wrap(device);
	if (ret) {
		merr("fimc_is_ischain_close_wrap is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_subdev_start(struct fimc_is_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_subdev *leader;

	FIMC_BUG(!subdev);
	FIMC_BUG(!subdev->leader);

	leader = subdev->leader;

	if (test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		mserr("already start", subdev, subdev);
		goto p_err;
	}

	if (test_bit(FIMC_IS_SUBDEV_START, &leader->state)) {
		mserr("leader%d is ALREADY started", subdev, subdev, leader->id);
		goto p_err;
	}

	set_bit(FIMC_IS_SUBDEV_START, &subdev->state);

p_err:
	return ret;
}

static int fimc_is_sensor_subdev_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_sensor *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(FIMC_IS_SENSOR_S_INPUT, &device->state)) {
		mserr("device is not yet init", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	if (test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		mserr("already start", subdev, subdev);
		goto p_err;
	}

	set_bit(FIMC_IS_SUBDEV_START, &subdev->state);

p_err:
	return ret;
}

static int fimc_is_ischain_subdev_start(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(FIMC_IS_ISCHAIN_INIT, &device->state)) {
		mserr("device is not yet init", device, subdev);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_subdev_start(subdev);
	if (ret) {
		mserr("fimc_is_subdev_start is fail(%d)", device, subdev, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_subdev_stop(struct fimc_is_subdev *subdev)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_subdev *leader;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	FIMC_BUG(!subdev);
	FIMC_BUG(!subdev->leader);

	leader = subdev->leader;
	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	FIMC_BUG(!framemgr);

	if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		merr("already stop", subdev);
		goto p_err;
	}

	if (test_bit(FIMC_IS_SUBDEV_START, &leader->state)) {
		merr("leader%d is NOT stopped", subdev, leader->id);
		goto p_err;
	}

	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_16, flags);

	if (framemgr->queued_count[FS_PROCESS] > 0) {
		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_16, flags);
		merr("being processed, can't stop", subdev);
		ret = -EINVAL;
		goto p_err;
	}

	frame = peek_frame(framemgr, FS_REQUEST);
	while (frame) {
		CALL_VOPS(subdev->vctx, done, frame->index, VB2_BUF_STATE_ERROR);
		trans_frame(framemgr, frame, FS_COMPLETE);
		frame = peek_frame(framemgr, FS_REQUEST);
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_16, flags);

	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_START, &subdev->state);

p_err:
	return ret;
}

static int fimc_is_sensor_subdev_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_device_sensor *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (!test_bit(FIMC_IS_SUBDEV_START, &subdev->state)) {
		merr("already stop", subdev);
		goto p_err;
	}

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_16, flags);

	frame = peek_frame(framemgr, FS_PROCESS);
	while (frame) {
		CALL_VOPS(subdev->vctx, done, frame->index, VB2_BUF_STATE_ERROR);
		trans_frame(framemgr, frame, FS_COMPLETE);
		frame = peek_frame(framemgr, FS_PROCESS);
	}

	frame = peek_frame(framemgr, FS_REQUEST);
	while (frame) {
		CALL_VOPS(subdev->vctx, done, frame->index, VB2_BUF_STATE_ERROR);
		trans_frame(framemgr, frame, FS_COMPLETE);
		frame = peek_frame(framemgr, FS_REQUEST);
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_16, flags);

	clear_bit(FIMC_IS_SUBDEV_RUN, &subdev->state);
	clear_bit(FIMC_IS_SUBDEV_START, &subdev->state);

p_err:
	return ret;
}

static int fimc_is_ischain_subdev_stop(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_subdev_stop(subdev);
	if (ret) {
		merr("fimc_is_subdev_stop is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_subdev_s_format(struct fimc_is_subdev *subdev,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	u32 pixelformat = 0, width, height;

	FIMC_BUG(!subdev);
	FIMC_BUG(!queue);
	FIMC_BUG(!queue->framecfg.format);

	pixelformat = queue->framecfg.format->pixelformat;

	width = queue->framecfg.width;
	height = queue->framecfg.height;

	switch (subdev->id) {
	case ENTRY_M0P:
	case ENTRY_M1P:
	case ENTRY_M2P:
	case ENTRY_M3P:
	case ENTRY_M4P:
		if (width % 8) {
			merr("width(%d) of format(%d) is not multiple of 8: need to check size",
				subdev, width, pixelformat);
		}
		break;
	default:
		break;
	}

	subdev->output.width = width;
	subdev->output.height = height;

	subdev->output.crop.x = 0;
	subdev->output.crop.y = 0;
	subdev->output.crop.w = subdev->output.width;
	subdev->output.crop.h = subdev->output.height;

	return ret;
}

static int fimc_is_sensor_subdev_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_sensor *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_subdev_s_format(subdev, queue);
	if (ret) {
		merr("fimc_is_subdev_s_format is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ischain_subdev_s_format(void *qdevice,
	struct fimc_is_queue *queue)
{
	int ret = 0;
	struct fimc_is_device_ischain *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_subdev_s_format(subdev, queue);
	if (ret) {
		merr("fimc_is_subdev_s_format is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_sensor_subdev_reqbuf(void *qdevice,
	struct fimc_is_queue *queue, u32 count)
{
	int i = 0;
	int ret = 0;
	struct fimc_is_device_sensor *device = qdevice;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_subdev *subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	FIMC_BUG(!device);
	FIMC_BUG(!queue);

	if (!count)
		goto p_err;

	vctx = container_of(queue, struct fimc_is_video_ctx, queue);
	subdev = vctx->subdev;
	if (!subdev) {
		merr("subdev is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	if (!framemgr) {
		merr("framemgr is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	for (i = 0; i < count; i++) {
		frame = &framemgr->frames[i];
		frame->subdev = subdev;
	}

p_err:
	return ret;
}

int fimc_is_subdev_buffer_queue(struct fimc_is_subdev *subdev,
	struct vb2_buffer *vb)
{
	int ret = 0;
	unsigned long flags;
	struct fimc_is_framemgr *framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	struct fimc_is_frame *frame;
	unsigned int index = vb->index;

	FIMC_BUG(!subdev);
	FIMC_BUG(!framemgr);
	FIMC_BUG(index >= framemgr->num_frames);

	frame = &framemgr->frames[index];

	/* 1. check frame validation */
	if (unlikely(!test_bit(FRAME_MEM_INIT, &frame->mem_state))) {
		mserr("frame %d is NOT init", subdev, subdev, index);
		ret = EINVAL;
		goto p_err;
	}

	/* 2. update frame manager */
	framemgr_e_barrier_irqs(framemgr, FMGR_IDX_17, flags);

	if (frame->state == FS_FREE) {
		trans_frame(framemgr, frame, FS_REQUEST);
	} else {
		mserr("frame %d is invalid state(%d)\n", subdev, subdev, index, frame->state);
		frame_manager_print_queues(framemgr);
		ret = -EINVAL;
	}

	framemgr_x_barrier_irqr(framemgr, FMGR_IDX_17, flags);

p_err:
	return ret;
}

int fimc_is_subdev_buffer_finish(struct fimc_is_subdev *subdev,
	struct vb2_buffer *vb)
{
	int ret = 0;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned int index = vb->index;

	if (!subdev) {
		warn("subdev is NULL(%d)", index);
		ret = -EINVAL;
		return ret;
	}

	if (unlikely(!test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state))) {
		warn("subdev was closed..(%d)", index);
		ret = -EINVAL;
		return ret;
	}

	framemgr = GET_SUBDEV_FRAMEMGR(subdev);
	if (unlikely(!framemgr)) {
		warn("subdev's framemgr is null..(%d)", index);
		ret = -EINVAL;
		return ret;
	}

	FIMC_BUG(index >= framemgr->num_frames);

	frame = &framemgr->frames[index];
	framemgr_e_barrier_irq(framemgr, FMGR_IDX_18);

	if (frame->state == FS_COMPLETE) {
		trans_frame(framemgr, frame, FS_FREE);
	} else {
		merr("frame is empty from complete", subdev);
		merr("frame(%d) is not com state(%d)", subdev, index, frame->state);
		frame_manager_print_queues(framemgr);
		ret = -EINVAL;
	}

	framemgr_x_barrier_irq(framemgr, FMGR_IDX_18);

	return ret;
}

const struct fimc_is_queue_ops fimc_is_sensor_subdev_ops = {
	.start_streaming	= fimc_is_sensor_subdev_start,
	.stop_streaming		= fimc_is_sensor_subdev_stop,
	.s_format		= fimc_is_sensor_subdev_s_format,
	.request_bufs		= fimc_is_sensor_subdev_reqbuf,
};

const struct fimc_is_queue_ops fimc_is_ischain_subdev_ops = {
	.start_streaming	= fimc_is_ischain_subdev_start,
	.stop_streaming		= fimc_is_ischain_subdev_stop,
	.s_format		= fimc_is_ischain_subdev_s_format
};

void fimc_is_subdev_dis_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
	/* this function is for dis start */
}

void fimc_is_subdev_dis_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
	/* this function is for dis stop */
}

void fimc_is_subdev_dis_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass)
{
	struct param_tpu_config *config_param;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!lindex);
	FIMC_BUG_VOID(!hindex);
	FIMC_BUG_VOID(!indexes);

	config_param = fimc_is_itf_g_param(device, frame, PARAM_TPU_CONFIG);
	config_param->dis_bypass = bypass ? CONTROL_BYPASS_ENABLE : CONTROL_BYPASS_DISABLE;
	*lindex |= LOWBIT_OF(PARAM_TPU_CONFIG);
	*hindex |= HIGHBIT_OF(PARAM_TPU_CONFIG);
	(*indexes)++;
}

void fimc_is_subdev_dnr_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
	/* this function is for dnr start */
}

void fimc_is_subdev_dnr_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
	/* this function is for dnr stop */
}

void fimc_is_subdev_dnr_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass)
{
	struct param_tpu_config *config_param;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!lindex);
	FIMC_BUG_VOID(!hindex);
	FIMC_BUG_VOID(!indexes);

	config_param = fimc_is_itf_g_param(device, frame, PARAM_TPU_CONFIG);
	config_param->tdnr_bypass = bypass ? CONTROL_BYPASS_ENABLE : CONTROL_BYPASS_DISABLE;
	*lindex |= LOWBIT_OF(PARAM_TPU_CONFIG);
	*hindex |= HIGHBIT_OF(PARAM_TPU_CONFIG);
	(*indexes)++;
}

void fimc_is_subdev_drc_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
#ifdef SOC_DRC
	struct param_control *ctl_param;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!lindex);
	FIMC_BUG_VOID(!hindex);
	FIMC_BUG_VOID(!indexes);

	ctl_param = fimc_is_itf_g_param(device, frame, PARAM_DRC_CONTROL);
	ctl_param->cmd = CONTROL_COMMAND_START;
	ctl_param->bypass = CONTROL_BYPASS_DISABLE;
	*lindex |= LOWBIT_OF(PARAM_DRC_CONTROL);
	*hindex |= HIGHBIT_OF(PARAM_DRC_CONTROL);
	(*indexes)++;
#endif
}

void fimc_is_subdev_drc_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
#ifdef SOC_DRC
	struct param_control *ctl_param;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!lindex);
	FIMC_BUG_VOID(!hindex);
	FIMC_BUG_VOID(!indexes);

	ctl_param = fimc_is_itf_g_param(device, frame, PARAM_DRC_CONTROL);
	ctl_param->cmd = CONTROL_COMMAND_STOP;
	ctl_param->bypass = CONTROL_BYPASS_ENABLE;
	*lindex |= LOWBIT_OF(PARAM_DRC_CONTROL);
	*hindex |= HIGHBIT_OF(PARAM_DRC_CONTROL);
	(*indexes)++;
#endif
}

void fimc_is_subdev_drc_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass)
{
#ifdef SOC_DRC
	struct param_control *ctl_param;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!lindex);
	FIMC_BUG_VOID(!hindex);
	FIMC_BUG_VOID(!indexes);

	ctl_param = fimc_is_itf_g_param(device, frame, PARAM_DRC_CONTROL);
	ctl_param->cmd = CONTROL_COMMAND_START;
	ctl_param->bypass = bypass ? CONTROL_BYPASS_ENABLE : CONTROL_BYPASS_DISABLE;
	*lindex |= LOWBIT_OF(PARAM_DRC_CONTROL);
	*hindex |= HIGHBIT_OF(PARAM_DRC_CONTROL);
	(*indexes)++;
#endif
}

void fimc_is_subdev_odc_start(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
	/* this function is for odc start */
}

void fimc_is_subdev_odc_stop(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes)
{
	/* this function is for odc stop */
}

void fimc_is_subdev_odc_bypass(struct fimc_is_device_ischain *device,
	struct fimc_is_frame *frame, u32 *lindex, u32 *hindex, u32 *indexes, bool bypass)
{
	struct param_tpu_config *config_param;

	FIMC_BUG_VOID(!device);
	FIMC_BUG_VOID(!lindex);
	FIMC_BUG_VOID(!hindex);
	FIMC_BUG_VOID(!indexes);

	config_param = fimc_is_itf_g_param(device, frame, PARAM_TPU_CONFIG);
	config_param->odc_bypass = bypass ? CONTROL_BYPASS_ENABLE : CONTROL_BYPASS_DISABLE;
	*lindex |= LOWBIT_OF(PARAM_TPU_CONFIG);
	*hindex |= HIGHBIT_OF(PARAM_TPU_CONFIG);
	(*indexes)++;
}

#ifdef ENABLE_FD_SW
int fimc_is_vra_trigger(struct fimc_is_device_ischain *device,
	struct fimc_is_subdev *subdev,
	struct fimc_is_frame *frame)
{
	int ret = 0;
	struct fimc_is_lib_fd *lib_data;
	struct camera2_fd_udm *fd_dm;
	struct camera2_stats_dm *fdstats;
	u32 detect_num = 0;

	FIMC_BUG(!device);
	FIMC_BUG(!subdev);
	FIMC_BUG(!frame);

	/* Parameter memset */
	fdstats = &frame->shot->dm.stats;
	memset(fdstats->faceRectangles, 0, sizeof(fdstats->faceRectangles));
	memset(fdstats->faceScores, 0, sizeof(fdstats->faceScores));
	memset(fdstats->faceIds, 0, sizeof(fdstats->faceIds));

	/* vra bypass check */
	if (!test_bit(FIMC_IS_SUBDEV_RUN, &subdev->state))
		goto vra_pass;

	if (unlikely(!device->fd_lib)) {
		merr("fimc_is_fd_lib is NULL", device);
		goto vra_pass;
	}

	if (unlikely(!device->fd_lib->lib_data)) {
		merr("fimc_is_lib_data is NULL", device);
		goto vra_pass;
	}

	lib_data = device->fd_lib->lib_data;
	if (!test_bit(FD_LIB_CONFIG, &device->fd_lib->state)) {
		merr("didn't become FD lib config", device);
		goto vra_pass;
	}

	/* ToDo: Support face detection mode by setfile */
	fdstats->faceDetectMode = FACEDETECT_MODE_FULL;

	if(!test_bit(FD_LIB_RUN, &device->fd_lib->state)) {

		/* Parameter copy for Host FD */
		fd_dm = &frame->shot->udm.fd;
		lib_data->fd_uctl.faceDetectMode = fdstats->faceDetectMode;
		lib_data->data_fd_lib->sat = fd_dm->vendorSpecific[0];

		if (!lib_data->num_detect) {
			memset(&lib_data->prev_fd_uctl, 0, sizeof(struct camera2_fd_uctl));
			lib_data->prev_num_detect = 0;
		} else {
			for (detect_num = 0; detect_num < lib_data->num_detect; detect_num++) {
				/* Parameter copy for HAL */
				fdstats->faceRectangles[detect_num][0] =
					lib_data->fd_uctl.faceRectangles[detect_num][0];
				fdstats->faceRectangles[detect_num][1] =
					lib_data->fd_uctl.faceRectangles[detect_num][1];
				fdstats->faceRectangles[detect_num][2] =
					lib_data->fd_uctl.faceRectangles[detect_num][2];
				fdstats->faceRectangles[detect_num][3] =
					lib_data->fd_uctl.faceRectangles[detect_num][3];
				fdstats->faceScores[detect_num] =
					lib_data->fd_uctl.faceScores[detect_num];
				fdstats->faceIds[detect_num] =
					lib_data->fd_uctl.faceIds[detect_num];
			}

			/* FD information backup */
			memcpy(&lib_data->prev_fd_uctl, &lib_data->fd_uctl,
				sizeof(struct camera2_fd_uctl));
			lib_data->prev_num_detect = lib_data->num_detect;

		}

		/* host fd thread run */
		fimc_is_lib_fd_trigger(device->fd_lib);

	} else {
		for (detect_num = 0; detect_num < lib_data->prev_num_detect; detect_num++) {
			/* Previous parameter copy for HAL */
			fdstats->faceRectangles[detect_num][0] =
				lib_data->prev_fd_uctl.faceRectangles[detect_num][0];
			fdstats->faceRectangles[detect_num][1] =
				lib_data->prev_fd_uctl.faceRectangles[detect_num][1];
			fdstats->faceRectangles[detect_num][2] =
				lib_data->prev_fd_uctl.faceRectangles[detect_num][2];
			fdstats->faceRectangles[detect_num][3] =
				lib_data->prev_fd_uctl.faceRectangles[detect_num][3];
			fdstats->faceScores[detect_num] =
				lib_data->prev_fd_uctl.faceScores[detect_num];
			fdstats->faceIds[detect_num] =
				lib_data->prev_fd_uctl.faceIds[detect_num];
		}
	}

vra_pass:
	return ret;
}
#endif

static int fimc_is_subdev_internal_alloc_buffer(struct fimc_is_subdev *subdev,
	struct fimc_is_mem *mem)
{
	int ret;
	int i;
	int buffer_size;
	struct fimc_is_frame *frame;

	FIMC_BUG(!subdev);

	if (subdev->buffer_num > SUBDEV_INTERNAL_BUF_MAX) {
		err("invalid internal buffer num size(%d)", subdev->buffer_num);
		return -EINVAL;
	}

	for (i = 0; i < subdev->buffer_num; i++) {
		buffer_size = subdev->output.width * subdev->output.height
					* subdev->bytes_per_pixel;

		if (buffer_size <= 0) {
			err("wrong internal subdev buffer size(%d)", buffer_size);
			return -EINVAL;
		}

		subdev->pb_subdev[i] = CALL_PTR_MEMOP(mem, alloc, mem->default_ctx, buffer_size, NULL, 0);
		if (IS_ERR_OR_NULL(subdev->pb_subdev[i])) {
			err("failed to allocate buffer for internal subdev");
			ret = -ENOMEM;
			goto err_allocate_pb_subdev;
		}
	}

	ret = frame_manager_open(&subdev->internal_framemgr, subdev->buffer_num);
	if (ret) {
		err("fimc_is_frame_open is fail(%d)", ret);
		ret = -EINVAL;
		goto err_open_framemgr;
	}

	for (i = 0; i < subdev->buffer_num; i++) {
		frame = &subdev->internal_framemgr.frames[i];
		frame->subdev = subdev;

		/* TODO : support multi-plane */
		frame->planes = 1;
		frame->dvaddr_buffer[0] = CALL_BUFOP(subdev->pb_subdev[i], dvaddr, subdev->pb_subdev[i]);
		frame->kvaddr_buffer[0] = CALL_BUFOP(subdev->pb_subdev[i], kvaddr, subdev->pb_subdev[i]);

		set_bit(FRAME_MEM_INIT, &frame->mem_state);
	}

	info("[%d] %s (subdev_id: %d, size: %d, buffernum: %d)",
		subdev->instance, __func__, subdev->id, buffer_size, subdev->buffer_num);

	return 0;

err_open_framemgr:
err_allocate_pb_subdev:
	while (i-- > 0)
		CALL_VOID_BUFOP(subdev->pb_subdev[i], free, subdev->pb_subdev[i]);

	return ret;
};

static int fimc_is_subdev_internal_free_buffer(struct fimc_is_subdev *subdev)
{
	int ret = 0;
	int i;

	FIMC_BUG(!subdev);

	frame_manager_close(&subdev->internal_framemgr);

	for (i = 0; i < subdev->buffer_num; i++)
		CALL_VOID_BUFOP(subdev->pb_subdev[i], free, subdev->pb_subdev[i]);

	info("[%d]%s(id: %d)", subdev->instance, __func__, subdev->id);

	return ret;
};

static int fimc_is_sensor_subdev_internal_alloc_buffer(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	int i;
	struct fimc_is_device_csi *csi;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_mem *mem = &device->resourcemgr->mem;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	for (i = CSI_VIRTUAL_CH_1; i < CSI_VIRTUAL_CH_MAX; i++) {
		dma_subdev = csi->dma_subdev[i];

		if (!dma_subdev)
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &dma_subdev->state))
			continue;

		if (dma_subdev->internal_framemgr.num_frames > 0) {
			mwarn("%s: [VC%d] already internal buffer alloced, re-alloc after free",
				device, __func__, i);

			ret = fimc_is_subdev_internal_free_buffer(dma_subdev);
			if (ret) {
				merr("subdev internal free buffer is fail", device);
				ret = -EINVAL;
				goto p_err;
			}
		}

		ret = fimc_is_subdev_internal_alloc_buffer(dma_subdev, mem);
		if (ret) {
			merr("subdev internal alloc buffer is fail", device);
			ret = -EINVAL;
			goto p_err;
		}
	}

p_err:
	return ret;
};

static int fimc_is_sensor_subdev_internal_free_buffer(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	int i;
	struct fimc_is_device_csi *csi;
	struct fimc_is_subdev *dma_subdev;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	if (test_bit(FIMC_IS_SENSOR_FRONT_START, &device->state)) {
		mwarn("sensor is not stopped, skip internal buffer free", device);
		goto p_err;
	}

	for (i = CSI_VIRTUAL_CH_1; i < CSI_VIRTUAL_CH_MAX; i++) {
		dma_subdev = csi->dma_subdev[i];

		if (!dma_subdev)
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &dma_subdev->state))
			continue;

		if (dma_subdev->internal_framemgr.num_frames == 0) {
			mwarn("[VC%d] already free internal buffer", device, i);
			continue;
		}

		ret = fimc_is_subdev_internal_free_buffer(dma_subdev);
		if (ret) {
			merr("subdev internal free buffer is fail", device);
			ret = -EINVAL;
			goto p_err;
		}
	}

p_err:
	return ret;
};

static int fimc_is_sensor_subdev_internal_start(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	int i, j;
	struct fimc_is_device_csi *csi;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned long flags;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	if (!test_bit(FIMC_IS_SENSOR_S_INPUT, &device->state)) {
		merr("device is not yet init", device);
		ret = -EINVAL;
		goto p_err;
	}

	for (i = CSI_VIRTUAL_CH_1; i < CSI_VIRTUAL_CH_MAX; i++) {
		dma_subdev = csi->dma_subdev[i];

		if (!dma_subdev)
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &dma_subdev->state))
			continue;

		if (test_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state)) {
			mwarn("[VC%d] subdev already start", device, i);
			continue;
		}

		/* qbuf a setting num of buffers before stream on */
		framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);
		if (unlikely(!framemgr)) {
			warn("VC%d subdev's framemgr is null", i);
			continue;
		}

		for (j = 0; j < framemgr->num_frames; j++) {
			frame = &framemgr->frames[j];

			/* 1. check frame validation */
			if (unlikely(!test_bit(FRAME_MEM_INIT, &frame->mem_state))) {
				mserr("frame %d is NOT init", dma_subdev, dma_subdev, j);
				ret = EINVAL;
				goto p_err;
			}

			/* 2. update frame manager */
			framemgr_e_barrier_irqs(framemgr, FMGR_IDX_17, flags);

			if (frame->state != FS_FREE) {
				mserr("frame %d is invalid state(%d)\n",
					dma_subdev, dma_subdev, j, frame->state);
				frame_manager_print_queues(framemgr);
				ret = -EINVAL;
			}

			framemgr_x_barrier_irqr(framemgr, FMGR_IDX_17, flags);
		}

		set_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state);

		minfo("[VC%d] %s(%s)\n", device, i, __func__, dma_subdev->data_type);
	}

p_err:

	return ret;
};

static int fimc_is_sensor_subdev_internal_stop(struct fimc_is_device_sensor *device)
{
	int ret = 0;
	int i;
	struct fimc_is_device_csi *csi;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned long flags;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(device->subdev_csi);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	for (i = CSI_VIRTUAL_CH_1; i < CSI_VIRTUAL_CH_MAX; i++) {
		dma_subdev = csi->dma_subdev[i];

		if (!dma_subdev)
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &dma_subdev->state))
			continue;

		if (!test_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state)) {
			mserr("already stopped", dma_subdev, dma_subdev);
			continue;
		}

		framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);
		if (unlikely(!framemgr)) {
			warn("VC%d subdev's framemgr is null", i);
			continue;
		}

		framemgr_e_barrier_irqs(framemgr, FMGR_IDX_16, flags);

		frame = peek_frame(framemgr, FS_PROCESS);
		while (frame) {
			trans_frame(framemgr, frame, FS_COMPLETE);
			frame = peek_frame(framemgr, FS_PROCESS);
		}

		frame = peek_frame(framemgr, FS_REQUEST);
		while (frame) {
			trans_frame(framemgr, frame, FS_COMPLETE);
			frame = peek_frame(framemgr, FS_REQUEST);
		}

		frame = peek_frame(framemgr, FS_COMPLETE);
		while (frame) {
			trans_frame(framemgr, frame, FS_FREE);
			frame = peek_frame(framemgr, FS_COMPLETE);
		}

		framemgr_x_barrier_irqr(framemgr, FMGR_IDX_16, flags);

		clear_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state);

		minfo("[VC%d] %s(%s)\n", device, i, __func__, dma_subdev->data_type);
	}

	return ret;
};

int fimc_is_subdev_internal_start(void *device, enum fimc_is_device_type type)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *ischain;

	FIMC_BUG(!device);

	switch (type) {
	case FIMC_IS_DEVICE_SENSOR:
		sensor = (struct fimc_is_device_sensor *)device;

		ret = fimc_is_sensor_subdev_internal_alloc_buffer(sensor);
		if (ret) {
			err("sensor internal alloc buffer fail(%d)", ret);
			break;
		}

		ret = fimc_is_sensor_subdev_internal_start(sensor);
		if (ret)
			err("sensor internal start fail(%d)", ret);

		break;
	case FIMC_IS_DEVICE_ISCHAIN:
		ischain = (struct fimc_is_device_ischain *)device;
		/* TODO */
		break;
	default:
		err("invalid device_type(%d)", type);
		ret = -EINVAL;
		break;
	}

	return ret;
};

int fimc_is_subdev_internal_stop(void *device, enum fimc_is_device_type type)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *ischain;

	FIMC_BUG(!device);

	switch (type) {
	case FIMC_IS_DEVICE_SENSOR:
		sensor = (struct fimc_is_device_sensor *)device;

		ret = fimc_is_sensor_subdev_internal_stop(sensor);
		if (ret) {
			err("sensor internal stop fail(%d)", ret);
			break;
		}

		ret = fimc_is_sensor_subdev_internal_free_buffer(sensor);
		if (ret)
			err("sensor internal buffer free fail(%d)", ret);

		break;
	case FIMC_IS_DEVICE_ISCHAIN:
		ischain = (struct fimc_is_device_ischain *)device;
		/* TODO */
		break;
	default:
		err("invalid device_type(%d)", type);
		ret = -EINVAL;
		break;
	}

	return ret;
};


static int fimc_is_sensor_subdev_internal_s_format(struct fimc_is_device_sensor *sensor)
{
	int ret = 0;
	int ch;
	struct fimc_is_device_csi *csi;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_subdev *subdev = NULL;
	struct fimc_is_vci_config *vci_cfg = NULL;

	FIMC_BUG(!sensor);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(sensor->subdev_module);
	if (!module) {
		err("module is NULL");
		return -EINVAL;
	}

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
		subdev = NULL;
		switch (ch) {
		case CSI_VIRTUAL_CH_1:
			if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &sensor->ssvc1.state))
				subdev = &sensor->ssvc1;
			break;
		case CSI_VIRTUAL_CH_2:
			if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &sensor->ssvc2.state))
				subdev = &sensor->ssvc2;
			break;
		case CSI_VIRTUAL_CH_3:
			if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &sensor->ssvc3.state))
				subdev = &sensor->ssvc3;
			break;
		}

		if (!subdev)
			continue;

		clear_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &subdev->state);

		if (!test_bit(FIMC_IS_SUBDEV_OPEN, &subdev->state))
			continue;

		vci_cfg = &sensor->cfg->output[ch];
		if (vci_cfg->type == VC_NOTHING)
			continue;

		/* VC type dependent value setting */
		switch (vci_cfg->type) {
		case VC_TAILPDAF:
			subdev->buffer_num = SUBDEV_INTERNAL_BUF_MAX;
			snprintf(subdev->data_type, sizeof(subdev->data_type), "VC_TAILPDAF");
			break;
		case VC_MIPISTAT:
			subdev->buffer_num = SUBDEV_INTERNAL_BUF_MAX;
			snprintf(subdev->data_type, sizeof(subdev->data_type), "VC_MIPISTAT");
			break;
		case VC_EMBEDDED:
			subdev->buffer_num = SUBDEV_INTERNAL_BUF_MAX;
			snprintf(subdev->data_type, sizeof(subdev->data_type), "VC_EMBEDDED");
			break;
		case VC_PRIVATE:
			subdev->buffer_num = SUBDEV_INTERNAL_BUF_MAX;
			snprintf(subdev->data_type, sizeof(subdev->data_type), "VC_PRIVATE");
			break;
		default:
			err("wrong internal vc ch(%d) type(%d)", ch, vci_cfg->type);
			return -EINVAL;
		}

		subdev->output.width = vci_cfg->width;
		subdev->output.height = vci_cfg->height;

		if (subdev->output.width == 0 || subdev->output.height == 0) {
			err("wrong internal vc size(%d x %d)",
				subdev->output.width, subdev->output.height);
			return -EINVAL;
		}

		subdev->output.crop.x = 0;
		subdev->output.crop.y = 0;
		subdev->output.crop.w = subdev->output.width;
		subdev->output.crop.h = subdev->output.height;

		if (vci_cfg->type == VC_TAILPDAF)
			subdev->bytes_per_pixel = 2;
		else
			subdev->bytes_per_pixel = 1;

		set_bit(FIMC_IS_SUBDEV_INTERNAL_S_FMT, &subdev->state);
	}

	return ret;
}

int fimc_is_subdev_internal_s_format(void *device, enum fimc_is_device_type type)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_ischain *ischain;

	FIMC_BUG(!device);

	switch (type) {
	case FIMC_IS_DEVICE_SENSOR:
		sensor = (struct fimc_is_device_sensor *)device;

		ret = fimc_is_sensor_subdev_internal_s_format(sensor);
		if (ret)
			err("sensor internal s_format fail(%d)", ret);

		break;
	case FIMC_IS_DEVICE_ISCHAIN:
		ischain = (struct fimc_is_device_ischain *)device;
		/* TODO */
		break;
	default:
		err("invalid device_type(%d)", type);
		ret = -EINVAL;
		break;
	}

	return ret;
};

int fimc_is_subdev_internal_open(void *device, enum fimc_is_device_type type)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor = NULL;
	struct fimc_is_device_ischain *ischain = NULL;
	struct fimc_is_subdev *subdev;
	char name[FIMC_IS_STR_LEN];

	FIMC_BUG(!device);

	switch (type) {
	case FIMC_IS_DEVICE_SENSOR:
		sensor = (struct fimc_is_device_sensor *)device;

		/* SSVC1 */
		subdev = &sensor->ssvc1;
		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state)) {
			snprintf(name, sizeof(name), "SS%dVC1", sensor->instance);
			frame_manager_probe(&subdev->internal_framemgr, FRAMEMGR_ID_SSXVC1, name);

			ret = fimc_is_sensor_check_subdev_open(sensor, subdev, NULL);
			if (ret) {
				err("fimc_is_sensor_check_subdev_open is fail(%d)", ret);
				goto p_err;
			}
		}

		/* SSVC2 */
		subdev = &sensor->ssvc2;
		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state)) {
			snprintf(name, sizeof(name), "SS%dVC2", sensor->instance);
			frame_manager_probe(&subdev->internal_framemgr, FRAMEMGR_ID_SSXVC2, name);

			ret = fimc_is_sensor_check_subdev_open(sensor, subdev, NULL);
			if (ret) {
				err("fimc_is_sensor_check_subdev_open is fail(%d)", ret);
				goto p_err;
			}
		}

		/* SSVC3 */
		subdev = &sensor->ssvc3;
		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state)) {
			snprintf(name, sizeof(name), "SS%dVC3", sensor->instance);
			frame_manager_probe(&subdev->internal_framemgr, FRAMEMGR_ID_SSXVC3, name);

			ret = fimc_is_sensor_check_subdev_open(sensor, subdev, NULL);
			if (ret) {
				err("fimc_is_sensor_check_subdev_open is fail(%d)", ret);
				goto p_err;
			}
		}
		break;
	case FIMC_IS_DEVICE_ISCHAIN:
		ischain = (struct fimc_is_device_ischain *)device;
		/* TODO */
		break;
	default:
		err("invalid device_type(%d)", type);
		ret = -EINVAL;
		break;
	}

p_err:

	return ret;
};

int fimc_is_subdev_internal_close(void *device, enum fimc_is_device_type type)
{
	int ret = 0;
	struct fimc_is_device_sensor *sensor = NULL;
	struct fimc_is_device_ischain *ischain = NULL;
	struct fimc_is_subdev *subdev;

	FIMC_BUG(!device);

	switch (type) {
	case FIMC_IS_DEVICE_SENSOR:
		sensor = (struct fimc_is_device_sensor *)device;

		/* for free buffer check */
		ret = fimc_is_sensor_subdev_internal_free_buffer(sensor);
		if (ret)
			merr("fimc_is_sensor_subdev_internal_free_buffer fail", sensor);

		/* SSVC1 */
		subdev = &sensor->ssvc1;
		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state)) {
			ret = fimc_is_subdev_close(subdev);
			if (ret) {
				merr("fimc_is_subdev_close is fail(%d)", sensor, ret);
				goto p_err;
			}
		}

		/* SSVC2 */
		subdev = &sensor->ssvc2;
		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state)) {
			ret = fimc_is_subdev_close(subdev);
			if (ret) {
				merr("fimc_is_subdev_close is fail(%d)", sensor, ret);
				goto p_err;
			}
		}

		/* SSVC3 */
		subdev = &sensor->ssvc3;
		if (test_bit(FIMC_IS_SUBDEV_INTERNAL_USE, &subdev->state)) {
			ret = fimc_is_subdev_close(subdev);
			if (ret) {
				merr("fimc_is_subdev_close is fail(%d)", sensor, ret);
				goto p_err;
			}
		}
		break;
	case FIMC_IS_DEVICE_ISCHAIN:
		ischain = (struct fimc_is_device_ischain *)device;
		/* TODO */
		break;
	default:
		err("invalid device_type(%d)", type);
		ret = -EINVAL;
		break;
	}

p_err:

	return ret;
};
