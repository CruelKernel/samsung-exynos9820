/*
 * Samsung Exynos SoC series FIMC-IS driver
 *
 * exynos fimc-is2 video functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/v4l2-mediabus.h>
#include <linux/bug.h>

#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"

const struct v4l2_file_operations fimc_is_ssxvc3_video_fops;
const struct v4l2_ioctl_ops fimc_is_ssxvc3_video_ioctl_ops;
const struct vb2_ops fimc_is_ssxvc3_qops;

int fimc_is_ssxvc3_video_probe(void *data)
{
	int ret = 0;
	struct fimc_is_device_sensor *device;
	struct fimc_is_video *video;
	char* video_name;
	u32 video_id;
	u32 instance;

	FIMC_BUG(!data);

	device = (struct fimc_is_device_sensor *)data;
	if (!device->pdev) {
		probe_err("pdev is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	instance = device->instance;
	video = &device->video_ssxvc3;
	video->resourcemgr = device->resourcemgr;

	if (instance >= FIMC_IS_SENSOR_COUNT) {
		err("instance %d was invalid", instance);
		ret = -EINVAL;
		goto p_err;
	}

	video_name = FIMC_IS_VIDEO_SSXVC3_NAME(instance);
	video_id = FIMC_IS_VIDEO_SS0VC3_NUM + (instance * 4);

	ret = fimc_is_video_probe(video,
			video_name,
			video_id,
			VFL_DIR_RX,
			&device->resourcemgr->mem,
			&device->v4l2_dev,
			&fimc_is_ssxvc3_video_fops,
			&fimc_is_ssxvc3_video_ioctl_ops);
	if (ret)
		dev_err(&device->pdev->dev, "%s is fail(%d)\n", __func__, ret);

p_err:
	return ret;
}

/*
 * =============================================================================
 * Video File Opertation
 * =============================================================================
 */

static int fimc_is_ssxvc3_video_open(struct file *file)
{
	int ret = 0;
	int ret_err = 0;
	u32 instance;
	u32 resource_type;
	struct fimc_is_video *video;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_device_sensor *device;
	struct fimc_is_resourcemgr *resourcemgr;
	char name[FIMC_IS_STR_LEN];

	vctx = NULL;
	device = NULL;
	video = video_drvdata(file);
	resourcemgr = video->resourcemgr;
	if (!resourcemgr) {
		err("resourcemgr is NULL");
		ret = -EINVAL;
		goto err_resource_null;
	}

	instance = ((video->id - FIMC_IS_VIDEO_SS0VC0_NUM) >> 2);
	switch(instance) {
	case 0:
		resource_type = RESOURCE_TYPE_SENSOR0;
		break;
	case 1:
		resource_type = RESOURCE_TYPE_SENSOR1;
		break;
	case 2:
		resource_type = RESOURCE_TYPE_SENSOR2;
		break;
	case 3:
		resource_type = RESOURCE_TYPE_SENSOR3;
		break;
	case 4:
		resource_type = RESOURCE_TYPE_SENSOR4;
		break;
	case 5:
		resource_type = RESOURCE_TYPE_SENSOR5;
		break;
	default:
		err("video id was invalid %d", video->id);
		ret = -EINVAL;
		goto err_video_id;
	}

	ret = fimc_is_resource_open(resourcemgr, resource_type, (void **)&device);
	if (ret) {
		err("fimc_is_resource_open is fail(%d)", ret);
		goto err_resource_open;
	}

	if (!device) {
		err("device is NULL");
		ret = -EINVAL;
		goto err_device_null;
	}

	minfo("[SS%dVC3:V] %s\n", device, device->instance, __func__);

	snprintf(name, sizeof(name), "SS%dVC3", device->instance);
	ret = open_vctx(file, video, &vctx, device->instance, FRAMEMGR_ID_SSXVC3, name);
	if (ret) {
		merr("open_vctx is fail(%d)", device, ret);
		goto err_vctx_open;
	}

	ret = fimc_is_video_open(vctx,
		device,
		VIDEO_SSXVC3_READY_BUFFERS,
		video,
		&fimc_is_ssxvc3_qops,
		&fimc_is_sensor_subdev_ops);
	if (ret) {
		merr("fimc_is_video_open is fail(%d)", device, ret);
		goto err_video_open;
	}

	ret = fimc_is_sensor_subdev_open(device, vctx);
	if (ret) {
		merr("fimc_is_sensor_subdev_open is fail(%d)", device, ret);
		goto err_ischain_open;
	}

	return 0;

err_ischain_open:
	ret_err = fimc_is_video_close(vctx);
	if (ret_err)
		merr("fimc_is_video_close is fail(%d)", device, ret_err);
err_video_open:
	ret_err = close_vctx(file, video, vctx);
	if (ret_err < 0)
		merr("close_vctx is fail(%d)", device, ret_err);
err_vctx_open:
err_device_null:
err_resource_open:
err_video_id:
err_resource_null:
	return ret;
}

static int fimc_is_ssxvc3_video_close(struct file *file)
{
	int ret = 0;
	int refcount;
	struct fimc_is_video_ctx *vctx = file->private_data;
	struct fimc_is_video *video;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!file);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_VIDEO(vctx));
	FIMC_BUG(!GET_DEVICE(vctx));

	video = GET_VIDEO(vctx);
	device = GET_DEVICE(vctx);

	ret = fimc_is_sensor_subdev_close(device, vctx);
	if (ret)
		merr("fimc_is_sensor_subdev_close is fail(%d)", device, ret);

	ret = fimc_is_video_close(vctx);
	if (ret)
		merr("fimc_is_video_close is fail(%d)", device, ret);

	refcount = close_vctx(file, video, vctx);
	if (refcount < 0)
		merr("close_vctx is fail(%d)", device, refcount);

	minfo("[SS%dVC3:V] %s(%d):%d\n", device, device->instance, __func__, refcount, ret);

	return ret;
}

static unsigned int fimc_is_ssxvc3_video_poll(struct file *file,
	struct poll_table_struct *wait)
{
	u32 ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	ret = fimc_is_video_poll(file, vctx, wait);
	if (ret)
		merr("fimc_is_video_poll is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_video_mmap(struct file *file,
	struct vm_area_struct *vma)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	ret = fimc_is_video_mmap(file, vctx, vma);
	if (ret)
		merr("fimc_is_video_mmap is fail(%d)", vctx, ret);

	return ret;
}

const struct v4l2_file_operations fimc_is_ssxvc3_video_fops = {
	.owner		= THIS_MODULE,
	.open		= fimc_is_ssxvc3_video_open,
	.release	= fimc_is_ssxvc3_video_close,
	.poll		= fimc_is_ssxvc3_video_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= fimc_is_ssxvc3_video_mmap,
};

/*
 * =============================================================================
 * Video Ioctl Opertation
 * =============================================================================
 */

static int fimc_is_ssxvc3_video_querycap(struct file *file, void *fh,
	struct v4l2_capability *cap)
{
	struct fimc_is_video *video = video_drvdata(file);

	FIMC_BUG(!cap);
	FIMC_BUG(!video);

	snprintf(cap->driver, sizeof(cap->driver), "%s", video->vd.name);
	snprintf(cap->card, sizeof(cap->card), "%s", video->vd.name);
	cap->capabilities |= V4L2_CAP_STREAMING
			| V4L2_CAP_VIDEO_CAPTURE
			| V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	cap->device_caps |= cap->capabilities;

	return 0;
}

static int fimc_is_ssxvc3_video_enum_fmt_mplane(struct file *file, void *priv,
	struct v4l2_fmtdesc *f)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_get_format_mplane(struct file *file, void *fh,
	struct v4l2_format *format)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_set_format_mplane(struct file *file, void *fh,
	struct v4l2_format *format)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	FIMC_BUG(!vctx);
	FIMC_BUG(!format);

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	ret = fimc_is_video_set_format_mplane(file, vctx, format);
	if (ret) {
		merr("fimc_is_video_set_format_mplane is fail(%d)", vctx, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ssxvc3_video_try_format_mplane(struct file *file, void *fh,
	struct v4l2_format *format)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_cropcap(struct file *file, void *fh,
	struct v4l2_cropcap *cropcap)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_get_crop(struct file *file, void *fh,
	struct v4l2_crop *crop)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_set_crop(struct file *file, void *fh,
	const struct v4l2_crop *crop)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_reqbufs(struct file *file, void *priv,
	struct v4l2_requestbuffers *buf)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;
	struct fimc_is_device_sensor *device;
	struct fimc_is_video *video;

	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_DEVICE(vctx));
	FIMC_BUG(!GET_VIDEO(vctx));

	mdbgv_ssxvc3("%s(buffers : %d)\n", vctx, __func__, buf->count);

	device = GET_DEVICE(vctx);
	video = GET_VIDEO(vctx);
	if (test_bit(FIMC_IS_SENSOR_BACK_START, &device->state)) {
		err("sensor%d still running, vid%d req is not applied", device->instance, video->id);
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_video_reqbufs(file, vctx, buf);
	if (ret) {
		merr("fimc_is_video_reqbufs is fail(%d)", device, ret);
		goto p_err;
	}

 p_err:
	return ret;
}

static int fimc_is_ssxvc3_video_querybuf(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	int ret;
	struct fimc_is_video_ctx *vctx = file->private_data;

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	ret = fimc_is_video_querybuf(file, vctx, buf);
	if (ret)
		merr("fimc_is_video_querybuf is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_video_qbuf(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	mvdbgs(3, "%s(index : %d)\n", vctx, &vctx->queue, __func__, buf->index);

	ret = CALL_VOPS(vctx, qbuf, buf);
	if (ret)
		merr("qbuf is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_video_dqbuf(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;
	bool blocking = file->f_flags & O_NONBLOCK;

	mvdbgs(3, "%s\n", vctx, &vctx->queue, __func__);

	ret = CALL_VOPS(vctx, dqbuf, buf, blocking);
	if (ret)
		merr("dqbuf is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_video_prepare(struct file *file, void *priv,
	struct v4l2_buffer *buf)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;
	struct fimc_is_device_sensor *device;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;

	FIMC_BUG(!buf);
	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_FRAMEMGR(vctx));
	FIMC_BUG(!GET_DEVICE(vctx));
	FIMC_BUG(!GET_VIDEO(vctx));

	device = GET_DEVICE(vctx);
	framemgr = GET_FRAMEMGR(vctx);
	frame = &framemgr->frames[buf->index];

	ret = fimc_is_video_prepare(file, vctx, buf);
	if (ret) {
		merr("fimc_is_video_prepare is fail(%d)", vctx, ret);
		goto p_err;
	}

p_err:
	minfo("[SS%dVC3:V] %s(%d):%d\n", device, device->instance, __func__, buf->index, ret);
	return ret;
}

static int fimc_is_ssxvc3_video_streamon(struct file *file, void *priv,
	enum v4l2_buf_type type)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	ret = fimc_is_video_streamon(file, vctx, type);
	if (ret)
		merr("fimc_is_video_streamon is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_video_streamoff(struct file *file, void *priv,
	enum v4l2_buf_type type)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	ret = fimc_is_video_streamoff(file, vctx, type);
	if (ret)
		merr("fimc_is_video_streamoff is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_video_enum_input(struct file *file, void *priv,
	struct v4l2_input *input)
{
	/* Todo: add enum input control code */
	return 0;
}

static int fimc_is_ssxvc3_video_g_input(struct file *file, void *priv,
	unsigned int *input)
{
	dbg("%s\n", __func__);
	return 0;
}

static int fimc_is_ssxvc3_video_s_input(struct file *file, void *priv,
	unsigned int input)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	FIMC_BUG(!vctx);

	ret = fimc_is_video_s_input(file, vctx);
	if (ret) {
		merr("fimc_is_video_s_input is fail(%d)", vctx, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_ssxvc3_video_g_ctrl(struct file *file, void *priv,
	struct v4l2_control *ctrl)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;
	struct fimc_is_framemgr *framemgr;

	FIMC_BUG(!vctx);
	FIMC_BUG(!ctrl);

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	framemgr = GET_FRAMEMGR(vctx);

	switch (ctrl->id) {
	case V4L2_CID_IS_G_COMPLETES:
		ctrl->value = framemgr->queued_count[FS_COMPLETE];
		break;
	default:
		err("unsupported ioctl(%d)\n", ctrl->id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int fimc_is_ssxvc3_video_g_ext_ctrl(struct file *file, void *priv,
	struct v4l2_ext_controls *ctrls)
{
	return 0;
}

static int fimc_is_ssxvc3_video_s_ctrl(struct file *file, void *priv,
	struct v4l2_control *ctrl)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = file->private_data;

	FIMC_BUG(!vctx);
	FIMC_BUG(!ctrl);

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	switch (ctrl->id) {
	default:
		ret = fimc_is_video_s_ctrl(file, vctx, ctrl);
		if (ret) {
			err("fimc_is_video_s_ctrl is fail(%d)", ret);
			goto p_err;
		}
		break;
	}

p_err:
	return ret;
}

const struct v4l2_ioctl_ops fimc_is_ssxvc3_video_ioctl_ops = {
	.vidioc_querycap		= fimc_is_ssxvc3_video_querycap,
	.vidioc_enum_fmt_vid_cap_mplane	= fimc_is_ssxvc3_video_enum_fmt_mplane,
	.vidioc_g_fmt_vid_cap_mplane	= fimc_is_ssxvc3_video_get_format_mplane,
	.vidioc_s_fmt_vid_cap_mplane	= fimc_is_ssxvc3_video_set_format_mplane,
	.vidioc_try_fmt_vid_cap_mplane	= fimc_is_ssxvc3_video_try_format_mplane,
	.vidioc_cropcap			= fimc_is_ssxvc3_video_cropcap,
	.vidioc_g_crop			= fimc_is_ssxvc3_video_get_crop,
	.vidioc_s_crop			= fimc_is_ssxvc3_video_set_crop,
	.vidioc_reqbufs			= fimc_is_ssxvc3_video_reqbufs,
	.vidioc_querybuf		= fimc_is_ssxvc3_video_querybuf,
	.vidioc_qbuf			= fimc_is_ssxvc3_video_qbuf,
	.vidioc_dqbuf			= fimc_is_ssxvc3_video_dqbuf,
	.vidioc_prepare_buf		= fimc_is_ssxvc3_video_prepare,
	.vidioc_streamon		= fimc_is_ssxvc3_video_streamon,
	.vidioc_streamoff		= fimc_is_ssxvc3_video_streamoff,
	.vidioc_enum_input		= fimc_is_ssxvc3_video_enum_input,
	.vidioc_g_input			= fimc_is_ssxvc3_video_g_input,
	.vidioc_s_input			= fimc_is_ssxvc3_video_s_input,
	.vidioc_g_ctrl			= fimc_is_ssxvc3_video_g_ctrl,
	.vidioc_s_ctrl			= fimc_is_ssxvc3_video_s_ctrl,
	.vidioc_g_ext_ctrls		= fimc_is_ssxvc3_video_g_ext_ctrl,
};

static int fimc_is_ssxvc3_queue_setup(struct vb2_queue *vbq,
	unsigned int *num_buffers,
	unsigned int *num_planes,
	unsigned int sizes[],
	struct device *alloc_devs[])
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = vbq->drv_priv;
	struct fimc_is_video *video;
	struct fimc_is_queue *queue;

	FIMC_BUG(!vctx);
	FIMC_BUG(!vctx->video);

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	video = GET_VIDEO(vctx);
	queue = GET_QUEUE(vctx);

	ret = fimc_is_queue_setup(queue,
		video->alloc_ctx,
		num_planes,
		sizes,
		alloc_devs);
	if (ret)
		merr("fimc_is_queue_setup is fail(%d)", vctx, ret);

	return ret;
}

static int fimc_is_ssxvc3_start_streaming(struct vb2_queue *vbq,
	unsigned int count)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = vbq->drv_priv;
	struct fimc_is_queue *queue;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!vctx);
	FIMC_BUG(!GET_DEVICE(vctx));

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	device = GET_DEVICE(vctx);
	queue = GET_QUEUE(vctx);

	ret = fimc_is_queue_start_streaming(queue, device);
	if (ret) {
		merr("fimc_is_queue_start_streaming is fail(%d)", device, ret);
		goto p_err;
	}

p_err:
	return ret;
}

static void fimc_is_ssxvc3_stop_streaming(struct vb2_queue *vbq)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = vbq->drv_priv;
	struct fimc_is_queue *queue;
	struct fimc_is_device_sensor *device;

	FIMC_BUG_VOID(!vctx);
	FIMC_BUG_VOID(!GET_DEVICE(vctx));

	mdbgv_ssxvc3("%s\n", vctx, __func__);

	device = GET_DEVICE(vctx);
	queue = GET_QUEUE(vctx);

	ret = fimc_is_queue_stop_streaming(queue, device);
	if (ret) {
		merr("fimc_is_queue_stop_streaming is fail(%d)", device, ret);
		return;
	}
}

static void fimc_is_ssxvc3_buffer_queue(struct vb2_buffer *vb)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = vb->vb2_queue->drv_priv;
	struct fimc_is_queue *queue;
	struct fimc_is_device_sensor *device;
	struct fimc_is_subdev *subdev;

	FIMC_BUG_VOID(!vctx);
	FIMC_BUG_VOID(!GET_DEVICE(vctx));

	mvdbgs(3, "%s(%d)\n", vctx, &vctx->queue, __func__, vb->index);

	device = GET_DEVICE(vctx);
	queue = GET_QUEUE(vctx);
	subdev = &device->ssvc3;

	ret = fimc_is_queue_buffer_queue(queue, vb);
	if (ret) {
		merr("fimc_is_queue_buffer_queue is fail(%d)", device, ret);
		return;
	}

	ret = fimc_is_subdev_buffer_queue(subdev, vb);
	if (ret) {
		merr("fimc_is_subdev_buffer_queue is fail(%d)", device, ret);
		return;
	}

#if !defined(CONFIG_USE_SENSOR_GROUP)
	ret = fimc_is_sensor_subdev_buffer_queue(device, ENTRY_SSVC3, vb->index);
	if (ret) {
		merr("fimc_is_sensor_subdev_buffer_queue is fail(%d)", device, ret);
		return;
	}
#endif
}

static void fimc_is_ssxvc3_buffer_finish(struct vb2_buffer *vb)
{
	int ret = 0;
	struct fimc_is_video_ctx *vctx = vb->vb2_queue->drv_priv;
	struct fimc_is_device_sensor *device;
	struct fimc_is_subdev *subdev;

	FIMC_BUG_VOID(!vctx);
	FIMC_BUG_VOID(!GET_DEVICE(vctx));

	mvdbgs(3, "%s(%d)\n", vctx, &vctx->queue, __func__, vb->index);

	device = GET_DEVICE(vctx);
	subdev = &device->ssvc3;

	fimc_is_queue_buffer_finish(vb);

	ret = fimc_is_subdev_buffer_finish(subdev, vb);
	if (ret) {
		merr("fimc_is_subdev_buffer_finish is fail(%d)", device, ret);
		return;
	}
}

const struct vb2_ops fimc_is_ssxvc3_qops = {
	.queue_setup		= fimc_is_ssxvc3_queue_setup,
	.buf_init		= fimc_is_queue_buffer_init,
	.buf_cleanup		= fimc_is_queue_buffer_cleanup,
	.buf_prepare		= fimc_is_queue_buffer_prepare,
	.buf_queue		= fimc_is_ssxvc3_buffer_queue,
	.buf_finish		= fimc_is_ssxvc3_buffer_finish,
	.wait_prepare		= fimc_is_queue_wait_prepare,
	.wait_finish		= fimc_is_queue_wait_finish,
	.start_streaming	= fimc_is_ssxvc3_start_streaming,
	.stop_streaming		= fimc_is_ssxvc3_stop_streaming,
};
