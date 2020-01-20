/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS ISPP GDC driver
 * (FIMC-IS PostProcessing Generic Distortion Correction driver)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <video/videonode.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-sg.h>

#include "camerapp-str-core.h"
#include "camerapp-str-video.h"

static const struct str_fmt str_fmts[] = {
	{
		.name		= "YUV420 non-contiguous 2-planar, Y/CrCb",
		.pix_fmt	= V4L2_PIX_FMT_NV21M,
		.bit_depth	= {8, 4},
		.num_planes	= STR_NUM_PLANES(2),
		.num_comp	= 2,
	},
};

const struct str_fmt *_str_find_format(u32 pix_fmt)
{
	for (int i = 0; i < ARRAY_SIZE(str_fmts); i++) {
		if (str_fmts[i].pix_fmt == pix_fmt)
			return &str_fmts[i];
	}

	return NULL;
}

int _str_set_planesize(struct str_frame_cfg *frame_cfg)
{
	if (!frame_cfg) {
		pr_err("[STR][%s:%d]frame_cfg is NULL.\n", __func__, __LINE__);
		return -EINVAL;
	}

	for (int i = 0; i < frame_cfg->fmt->num_planes; i++) {
		switch (frame_cfg->fmt->pix_fmt) {
		case V4L2_PIX_FMT_NV21M:
		default:
			frame_cfg->bytesperline[i] = (frame_cfg->fmt->bit_depth[i] * frame_cfg->width) >> 3;
			frame_cfg->size[i] = frame_cfg->bytesperline[i] * frame_cfg->height;
			break;
		}
	}

	return 0;
}

dma_addr_t _str_get_dma_addr(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_dma_sg_plane_dma_addr(vb2_buf, plane);
}

void *_str_get_kvaddr(struct vb2_buffer *vb2_buf, u32 plane)
{
	return vb2_plane_vaddr(vb2_buf, plane);
}

int _str_get_buf_addr(struct str_ctx *ctx, struct str_vb2_buffer *str_buf)
{
	struct str_frame_cfg *frame_cfg = &(ctx->frame_cfg);
	u32 pix_size, meta_plane_index;

	if (str_buf->vb2_buf.num_planes < frame_cfg->fmt->num_planes) {
		pr_err("[STR][%s]Invalid num_planes(%d). expected(%d)\n", __func__,
				str_buf->vb2_buf.num_planes, frame_cfg->fmt->num_planes);
		return -EINVAL;
	}

	memset(str_buf->dva, 0x00, sizeof(str_buf->dva));
	memset(str_buf->kva, 0x00, sizeof(str_buf->kva));
	meta_plane_index = str_buf->vb2_buf.num_planes - 1; /* Always use the last plane. */

	/* Image planes */
	str_buf->dva[0] = _str_get_dma_addr(&str_buf->vb2_buf, 0);
	if (!str_buf->dva[0]) {
		pr_err("[STR][%s:%d]Failed to get_dma_addr.\n", __func__, __LINE__);
		return -EFAULT;
	}

	pix_size = frame_cfg->width * frame_cfg->height;

	switch (frame_cfg->fmt->num_comp) {
	case 1:
		str_buf->size[0] = frame_cfg->size[0];
		break;
	case 2:
		str_buf->size[0] = pix_size;

		if (frame_cfg->fmt->num_planes == STR_NUM_PLANES(1)) {
			str_buf->dva[1] = str_buf->dva[0] + pix_size;
			str_buf->size[1] = frame_cfg->size[0] - pix_size;
		} else if (frame_cfg->fmt->num_planes == STR_NUM_PLANES(2)) {
			str_buf->dva[1] = _str_get_dma_addr(&str_buf->vb2_buf, 1);
			if (!str_buf->dva[1]) {
				pr_err("[STR][%s:%d]Failed to get_dma_addr.\n", __func__, __LINE__);
				return -EFAULT;
			}

			str_buf->size[1] = frame_cfg->size[1];
		}
		break;
	case 3:
		break;
	default:
		pr_err("[STR][%s:%d]Unsupported num_comp(%d)\n",
				__func__, __LINE__, frame_cfg->fmt->num_comp);
		return -EINVAL;
	}

	for (int i = 0; i < frame_cfg->fmt->num_comp; i++) {
		pr_info("[STR][%s:%d][P%d]addr(%p) size(%x)\n", __func__, __LINE__,
				i, str_buf->dva[i], str_buf->size[i]);
	}

	/* Metadata plane */
	str_buf->kva[meta_plane_index] = (ulong)_str_get_kvaddr(&str_buf->vb2_buf, meta_plane_index);

	return 0;
}

static int str_vb2_queue_setup(struct vb2_queue *vb2_q,
		unsigned int *num_bufs,
		unsigned int *num_planes,
		unsigned int sizes[],
		struct device *alloc_devs[])
{
	struct str_ctx *ctx = vb2_q->drv_priv;
	struct str_core *core = video_to_core(ctx->video);

	/* num_bufs is already filled by videobuf2-core */
	*num_planes = ctx->frame_cfg.fmt->num_planes;
	for (int i = 0; i < ctx->frame_cfg.fmt->num_planes; i++) {
		sizes[i] = ctx->frame_cfg.size[i];
		alloc_devs[i] = core->dev;
	}

	return 0;
}

static void str_vb2_lock(struct vb2_queue *vb2_q)
{
	struct str_ctx *ctx = vb2_q->drv_priv;

	mutex_lock(&(ctx->video->lock));
}

static void str_vb2_unlock(struct vb2_queue *vb2_q)
{
	struct str_ctx *ctx = vb2_q->drv_priv;

	mutex_unlock(&(ctx->video->lock));
}

static int str_vb2_buf_init(struct vb2_buffer *vb)
{
	/* Do nothing */
	return 0;
}

static int str_vb2_buf_prepare(struct vb2_buffer *vb)
{
	/* Do nothing */
	return 0;
}

static void str_vb2_buf_finish(struct vb2_buffer *vb)
{
	/* Do nothing */
}

static void str_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	/* Do nothing */
}

static int str_vb2_start_streaming(struct vb2_queue *vb2_q, unsigned int count)
{
	struct str_ctx *ctx = vb2_q->drv_priv;
	struct str_core *core = video_to_core(ctx->video);

	return str_power_clk_enable(core);
}

static void str_vb2_stop_streaming(struct vb2_queue *vb2_q)
{
	struct str_ctx *ctx = vb2_q->drv_priv;
	struct str_core *core = video_to_core(ctx->video);

	/* Guarantee the H/W idle state. */

	str_power_clk_disable(core);
}

static void str_vb2_buf_queue(struct vb2_buffer *vb)
{
	int ret = 0;
	struct str_ctx *ctx = vb->vb2_queue->drv_priv;
	struct str_vb2_buffer *str_buf = vb2_to_str_buf(vb);

	pr_info("[STR][%s:%d]index(%d)\n", __func__, __LINE__, vb->index);

	ret = _str_get_buf_addr(ctx, str_buf);
	if (ret) {
		pr_err("[STR][%s:%d]Failed to get_buf_addr. ret(%d)\n", __func__, __LINE__, ret);
		return;
	}

	ctx->buf = str_buf;
	ctx->frame_cfg.meta = (struct str_metadata *)str_buf->kva[vb->num_planes - 1];

	/* Do H/W configuration */
	ret = str_shot(ctx);
	if (ret) {
		pr_err("[STR][%s]failed to str_shot. ret(%d)\n", __func__, ret);
		return;
	}
}

static const struct vb2_ops str_vb2_ops = {
	.queue_setup		= str_vb2_queue_setup,
	.wait_prepare		= str_vb2_unlock,
	.wait_finish		= str_vb2_lock,
	.buf_init		= str_vb2_buf_init,
	.buf_prepare		= str_vb2_buf_prepare,
	.buf_finish		= str_vb2_buf_finish,
	.buf_cleanup		= str_vb2_buf_cleanup,
	.start_streaming	= str_vb2_start_streaming,
	.stop_streaming		= str_vb2_stop_streaming,
	.is_unordered		= NULL,
	.buf_queue		= str_vb2_buf_queue,
};

static unsigned int str_video_poll(struct file *file, struct poll_table_struct *wait)
{
	struct str_ctx *ctx = file->private_data;

	return vb2_poll(&ctx->vb2_q, file, wait);
}

static int str_video_open(struct file *file)
{
	int ret = 0;
	struct str_video *video = video_drvdata(file);
	struct str_ctx *ctx;

	atomic_inc(&video->open_cnt);

	pr_info("[STR][%s:%d]Open. open_cnt(%d)\n", __func__, __LINE__, atomic_read(&video->open_cnt));

	ctx = kzalloc(sizeof(struct str_ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->video = video;
	file->private_data = ctx;

	ctx->vb2_q.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	ctx->vb2_q.io_modes	= VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	ctx->vb2_q.ops = &str_vb2_ops;
	ctx->vb2_q.mem_ops = &vb2_dma_sg_memops;
	ctx->vb2_q.drv_priv = ctx;
	ctx->vb2_q.buf_struct_size = sizeof(struct str_vb2_buffer);
	ctx->vb2_q.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(&ctx->vb2_q);
	if (ret) {
		pr_err("[STR][%s:%d]Failed to vb2_queue_init. ret(%d)\n", __func__, __LINE__, ret);
		goto err_exit;
	}

	return 0;

err_exit:
	file->private_data = NULL;
	atomic_dec(&video->open_cnt);
	kfree(ctx);

	return ret;
}

static int str_video_release(struct file *file)
{
	struct str_video *video = video_drvdata(file);
	struct str_ctx *ctx = file->private_data;

	atomic_dec(&video->open_cnt);

	pr_info("[STR][%s:%d]Release. open_cnt(%d)\n", __func__, __LINE__, atomic_read(&video->open_cnt));

	vb2_queue_release(&ctx->vb2_q);
	file->private_data = NULL;

	if (!IS_ERR_OR_NULL(ctx->buf_y)) {
		pr_info("[STR][%s:%d]Free buf_y(%zu)\n", __func__, __LINE__, ctx->buf_y->size);
		str_ion_free(ctx->buf_y);
		ctx->buf_y = NULL;
	}
	if (!IS_ERR_OR_NULL(ctx->buf_c)) {
		pr_info("[STR][%s:%d]Free buf_c(%zu)\n", __func__, __LINE__, ctx->buf_c->size);
		str_ion_free(ctx->buf_c);
		ctx->buf_c = NULL;
	}

	kfree(ctx);

	return 0;
}

static const struct v4l2_file_operations str_v4l2_fops = {
	.owner			= THIS_MODULE,
	.read			= NULL,
	.write			= NULL,
	.poll			= str_video_poll,
	.unlocked_ioctl		= video_ioctl2,
	.get_unmapped_area	= NULL,
	.mmap			= NULL,
	.open			= str_video_open,
	.release		= str_video_release,
};

static int str_video_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	strncpy(cap->driver, MODULE_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, MODULE_NAME, sizeof(cap->driver) - 1);

	cap->capabilities = V4L2_CAP_STREAMING
		| V4L2_CAP_VIDEO_OUTPUT_MPLANE
		| V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = cap->capabilities;

	return 0;
}

static int str_video_enum_fmt_mplane(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	const struct str_fmt *str_fmt;

	if (f->index >= ARRAY_SIZE(str_fmts))
		return -EINVAL;

	str_fmt = &str_fmts[f->index];
	strncpy(f->description, str_fmt->name, sizeof(f->description) - 1);
	f->pixelformat = str_fmt->pix_fmt;

	return 0;
}

static int str_video_g_fmt_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct str_ctx *ctx = file->private_data;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		pr_err("[STR][%s:%d]Invalid v4l2_buf_type(%d)\n", __func__, __LINE__, f->type);
		return -EINVAL;
	}

	pix_mp->width = ctx->frame_cfg.width;
	pix_mp->height = ctx->frame_cfg.height;
	pix_mp->pixelformat = ctx->frame_cfg.fmt->pix_fmt;
	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->colorspace = V4L2_COLORSPACE_DEFAULT;
	pix_mp->num_planes = ctx->frame_cfg.fmt->num_planes;

	for (int i = 0; i < pix_mp->num_planes; i++) {
		pix_mp->plane_fmt[i].bytesperline = ctx->frame_cfg.bytesperline[i];
		pix_mp->plane_fmt[i].sizeimage = ctx->frame_cfg.size[i];
		pr_info("[STR][%s:%d][P%d]bytesperline(%d), sizeimage(%d)\n", __func__, __LINE__,
				i, pix_mp->plane_fmt[i].bytesperline, pix_mp->plane_fmt[i].sizeimage);
	}

	return 0;
}

static int str_video_s_fmt_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	int ret = 0;
	struct str_ctx *ctx = file->private_data;
	struct str_core *core = video_to_core(ctx->video);
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	bool size_changed = false;
	size_t size;

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		pr_err("[STR][%s:%d]Invalid v4l2_buf_type(%d)\n", __func__, __LINE__, f->type);
		ret = -EINVAL;
		goto func_exit;
	} else if (vb2_is_streaming(&ctx->vb2_q)) {
		pr_err("[STR][%s:%d]Device is running.\n", __func__, __LINE__);
		ret =  -EBUSY;
		goto func_exit;
	}

	ctx->frame_cfg.fmt = _str_find_format(pix_mp->pixelformat);
	if (!ctx->frame_cfg.fmt) {
		pr_err("[STR][%s:%d]Not supported format(%x)\n", __func__, __LINE__,
				f->fmt.pix_mp.pixelformat);
		ret =  -EINVAL;
		goto func_exit;
	}

	if (ctx->frame_cfg.width != pix_mp->width || ctx->frame_cfg.height != pix_mp->height)
		size_changed = true;

	ctx->frame_cfg.width = pix_mp->width;
	ctx->frame_cfg.height = pix_mp->height;

	ret = _str_set_planesize(&ctx->frame_cfg);
	if (ret) {
		pr_err("[STR][%s:%d]Failed to set planesize. ret(%d)\n", __func__, __LINE__, ret);
		goto func_exit;
	}

	pr_info("[STR][%s:%d]size(%dx%d), pixelformat(%x), num_planes(%d)\n", __func__, __LINE__,
			ctx->frame_cfg.width, ctx->frame_cfg.height,
			ctx->frame_cfg.fmt->pix_fmt, ctx->frame_cfg.fmt->num_planes);

	/* Alloc internal ION memory */
	if (size_changed && !IS_ERR_OR_NULL(ctx->buf_y)) {
		pr_info("[STR][%s:%d]Free buf_y(%zu)\n", __func__, __LINE__, ctx->buf_y->size);
		str_ion_free(ctx->buf_y);
		ctx->buf_y = NULL;
	}
	if (size_changed && !IS_ERR_OR_NULL(ctx->buf_c)) {
		pr_info("[STR][%s:%d]Free buf_c(%zu)\n", __func__, __LINE__, ctx->buf_c->size);
		str_ion_free(ctx->buf_c);
		ctx->buf_c = NULL;
	}

	size = ctx->frame_cfg.width * ctx->frame_cfg.height;

	ctx->buf_y = str_ion_alloc(&core->ion_ctx, size);
	if (IS_ERR_OR_NULL(ctx->buf_y)) {
		ret = -ENOMEM;
		goto func_exit;
	}
	pr_info("[STR][%s]Allocated buf_y(%p/%zu) dva(%p)\n", __func__, ctx->buf_y, size, ctx->buf_y->iova);

	size /= 2;

	ctx->buf_c = str_ion_alloc(&core->ion_ctx, size);
	if (IS_ERR_OR_NULL(ctx->buf_y)) {
		ret = -ENOMEM;
		goto err_buf_y;
	}
	pr_info("[STR][%s]Allocated buf_c(%p/%zu) dva(%p)\n", __func__, ctx->buf_c, size, ctx->buf_c->iova);

	return ret;

err_buf_y:
	str_ion_free(ctx->buf_y);
	ctx->buf_y = NULL;

func_exit:
	return ret;
}

static int str_video_try_fmt_mplane(struct file *file, void *fh, struct v4l2_format *f)
{
	struct str_ctx *ctx = file->private_data;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		pr_err("[STR][%s:%d]Invalid v4l2_buf_type(%d)\n", __func__, __LINE__, f->type);
		return -EINVAL;
	}

	ctx->frame_cfg.fmt = _str_find_format(pix_mp->pixelformat);
	if (!ctx->frame_cfg.fmt) {
		pr_err("[STR][%s:%d]Not supported format(%x)\n", __func__, __LINE__,
				f->fmt.pix_mp.pixelformat);
		return -EINVAL;
	}

	return 0;
}

static int str_video_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *reqbufs)
{
	struct str_ctx *ctx = file->private_data;

	if (!V4L2_TYPE_IS_MULTIPLANAR(reqbufs->type)) {
		pr_err("[STR][%s:%d]Invalid v4l2_buf_type(%d)\n", __func__, __LINE__, reqbufs->type);
		return -EINVAL;
	}

	pr_info("[STR][%s:%d]count(%d)\n", __func__, __LINE__, reqbufs->count);

	return vb2_reqbufs(&ctx->vb2_q, reqbufs);
}

static int str_video_querybuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct str_ctx *ctx = file->private_data;

	pr_info("[STR][%s:%d]\n", __func__, __LINE__);

	return vb2_querybuf(&ctx->vb2_q, buf);
}

static int str_video_qbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct str_ctx *ctx = file->private_data;

	pr_info("[STR][%s:%d]index(%d) length(%d)\n", __func__, __LINE__, buf->index, buf->length);

	return vb2_qbuf(&ctx->vb2_q, buf);
}

static int str_video_dqbuf(struct file *file, void *fh, struct v4l2_buffer *buf)
{
	struct str_ctx *ctx = file->private_data;
	bool nonblocking = false;

	pr_info("[STR][%s:%d]index(%d) length(%d)\n", __func__, __LINE__, buf->index, buf->length);

	return vb2_dqbuf(&ctx->vb2_q, buf, nonblocking);
}

static int str_video_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct str_ctx *ctx = file->private_data;

	pr_info("[STR][%s:%d]\n", __func__, __LINE__);

	if (ctx->vb2_q.type != type) {
		pr_err("[STR][%s:%d]Invalid buf_type(%d)\n", __func__, __LINE__, type);
		return -EINVAL;
	} else if (vb2_is_streaming(&ctx->vb2_q)) {
		pr_warn("[STR][%s:%d]Device is running\n", __func__, __LINE__);
		return 0;
	}

	return vb2_streamon(&ctx->vb2_q, type);
}

static int str_video_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct str_ctx *ctx = file->private_data;

	pr_info("[STR][%s:%d]\n", __func__, __LINE__);

	if (ctx->vb2_q.type != type) {
		pr_err("[STR][%s:%d]Invalid buf_type(%d)\n", __func__, __LINE__, type);
		return -EINVAL;
	} else if (!vb2_is_streaming(&ctx->vb2_q)) {
		pr_warn("[STR][%s:%d]Device is NOT running\n", __func__, __LINE__);
		return 0;
	}

	return vb2_streamoff(&ctx->vb2_q, type);
}

static int str_video_g_ctrl(struct file *file, void *priv, struct v4l2_control *ctrl)
{
	pr_info("[STR][%s:%d]id(%d)\n", __func__, __LINE__, ctrl->id);

	switch (ctrl->id) {
	default:
		pr_err("[STR][%s:%d]Invalid ctrl->id(%d)\n", __func__, __LINE__, ctrl->id);
		return -EINVAL;
	}

	return 0;
}

static int str_video_s_ctrl(struct file *file, void *priv, struct v4l2_control *ctrl)
{
	pr_info("[STR][%s:%d]id(%d)\n", __func__, __LINE__, ctrl->id);

	switch (ctrl->id) {
	default:
		pr_err("[STR][%s:%d]Invalid ctrl->id(%d)\n", __func__, __LINE__, ctrl->id);
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ioctl_ops str_v4l2_ioctl_ops = {
	.vidioc_querycap		= str_video_querycap,
	.vidioc_enum_fmt_vid_out_mplane	= str_video_enum_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= str_video_g_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= str_video_s_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= str_video_try_fmt_mplane,
	.vidioc_reqbufs			= str_video_reqbufs,
	.vidioc_querybuf		= str_video_querybuf,
	.vidioc_qbuf			= str_video_qbuf,
	.vidioc_dqbuf			= str_video_dqbuf,
	.vidioc_streamon		= str_video_streamon,
	.vidioc_streamoff		= str_video_streamoff,
	.vidioc_g_ctrl			= str_video_g_ctrl,
	.vidioc_s_ctrl			= str_video_s_ctrl,
};

int str_video_probe(void *data)
{
	int ret = 0;
	struct str_core *core = (struct str_core *)data;
	struct str_video *video = &core->video_str;
	struct device *dev;
	struct v4l2_device *v4l2_dev;
	struct video_device *video_dev;

	mutex_init(&video->lock);
	dev = core->dev;
	v4l2_dev = &video->v4l2_dev;
	video_dev = &video->video_dev;

	scnprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s", STR_VIDEO_NAME);

	ret = v4l2_device_register(dev, v4l2_dev);
	if (ret) {
		dev_err(dev, "Failed to register v4l2 device. ret(%d)\n", ret);
		return ret;
	}

	video->video_id = EXYNOS_VIDEONODE_CAMERAPP(CAMERAPP_VIDEONODE_STR);
	scnprintf(video_dev->name, sizeof(video_dev->name), "%s.out", MODULE_NAME);
	video_dev->vfl_dir = VFL_DIR_TX;
	video_dev->lock = &video->lock;

	video_set_drvdata(video_dev, video);
	video_dev->v4l2_dev = v4l2_dev;

	video_dev->fops = &str_v4l2_fops;
	video_dev->ioctl_ops = &str_v4l2_ioctl_ops;
	video_dev->release = video_device_release;

	ret = video_register_device(video_dev, VFL_TYPE_GRABBER, video->video_id);
	if (ret) {
		dev_err(dev, "Failed to register video device. ret(%d)\n", ret);
		goto err_v4l2_dev;
	}

	return 0;

err_v4l2_dev:
	v4l2_device_unregister(v4l2_dev);

	return ret;
}
