/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS ISPP GDC driver
 * (FIMC-IS PostProcessing Generic Distortion Correction driver)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>
#include <linux/smc.h>
#include <media/v4l2-ioctl.h>

#include <video/videonode.h>

#include "camerapp-gdc.h"
#include "camerapp-hw-api-gdc.h"
#include "camerapp-video.h"

int gdc_log_level;
module_param_named(gdc_log_level, gdc_log_level, uint, S_IRUGO | S_IWUSR);

/*
 * If true, writes the latency of H/W operation to v4l2_buffer.reserved2
 * in the unit of nano seconds.  It must not be enabled with real use-case
 * because v4l2_buffer.reserved may be used for other purpose.
 * The latency is written to the destination buffer.
 */
int __gdc_measure_hw_latency;
module_param_named(gdc_measure_hw_latency, __gdc_measure_hw_latency, int, 0644);

struct vb2_gdc_buffer {
	struct v4l2_m2m_buffer mb;
	struct gdc_ctx *ctx;
	ktime_t ktime;
};

static const struct gdc_fmt gdc_formats[] = {
	{
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12M,
		.cfg_val	= GDC_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21M,
		.cfg_val	= GDC_CFG_FMT_YCRCB420_2P,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
		.cfg_val	= GDC_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 12 },
		.num_planes = 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,
		.cfg_val	= GDC_CFG_FMT_YCRCB420_2P,
		.bitperpixel	= { 12 },
		.num_planes = 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.cfg_val	= GDC_CFG_FMT_YVYU,
		.bitperpixel	= { 16 },
		.num_planes = 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.cfg_val	= GDC_CFG_FMT_YUYV,
		.bitperpixel	= { 16 },
		.num_planes = 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
		.cfg_val	= GDC_CFG_FMT_YCBCR422_2P,
		.bitperpixel	= { 16 },
		.num_planes = 1,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,
		.cfg_val	= GDC_CFG_FMT_YCRCB422_2P,
		.bitperpixel	= { 16 },
		.num_planes = 1,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 non-contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16M,
		.cfg_val	= GDC_CFG_FMT_YCBCR422_2P,
		.bitperpixel	= { 8, 8 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 non-contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61M,
		.cfg_val	= GDC_CFG_FMT_YCRCB422_2P,
		.bitperpixel	= { 8, 8 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "P010_16B",
		.pixelformat	= V4L2_PIX_FMT_NV12M_P010,
		.cfg_val	= GDC_CFG_FMT_P010_16B_2P,
		.bitperpixel	= { 16, 16 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "P010_16B",
		.pixelformat	= V4L2_PIX_FMT_NV21M_P010,
		.cfg_val	= GDC_CFG_FMT_P010_16B_2P,
		.bitperpixel	= { 16, 16 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "P210_16B",
		.pixelformat	= V4L2_PIX_FMT_NV16M_P210,
		.cfg_val	= GDC_CFG_FMT_P210_16B_2P,
		.bitperpixel	= { 16, 16 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "P210_16B",
		.pixelformat	= V4L2_PIX_FMT_NV61M_P210,
		.cfg_val	= GDC_CFG_FMT_P210_16B_2P,
		.bitperpixel	= { 16, 16 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV422 2P 10bit(8+2)",
		.pixelformat	= V4L2_PIX_FMT_NV16M_S10B,
		.cfg_val	= GDC_CFG_FMT_YCRCB422_8P2_2P,
		.bitperpixel	= { 8, 8 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV422 2P 10bit(8+2)",
		.pixelformat	= V4L2_PIX_FMT_NV61M_S10B,
		.cfg_val	= GDC_CFG_FMT_YCRCB422_8P2_2P,
		.bitperpixel	= { 8, 8 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV420 2P 10bit(8+2)",
		.pixelformat	= V4L2_PIX_FMT_NV12M_S10B,
		.cfg_val	= GDC_CFG_FMT_YCRCB420_8P2_2P,
		.bitperpixel	= { 8, 8 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV420 2P 10bit(8+2)",
		.pixelformat	= V4L2_PIX_FMT_NV21M_S10B,
		.cfg_val	= GDC_CFG_FMT_YCRCB420_8P2_2P,
		.bitperpixel	= { 8, 8 },
		.num_planes = 2,
		.num_comp	= 2,
		.h_shift	= 1,
	},
};

static const struct gdc_variant gdc_variant[] = {
	{
		.limit_input = {
			.min_w		= 32,
			.min_h		= 32,
			.max_w		= 8192,
			.max_h		= 6144,
		},
		.limit_output = {
			.min_w		= 32,
			.min_h		= 32,
			.max_w		= 8192,
			.max_h		= 6144,
		},
		.version		= 0,
	},
};

/* Find the matches format */
static const struct gdc_fmt *gdc_find_format(struct gdc_dev *gdc,
						u32 pixfmt, bool output_buf)
{
	const struct gdc_fmt *gdc_fmt;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(gdc_formats); ++i) {
		gdc_fmt = &gdc_formats[i];
		if (gdc_fmt->pixelformat == pixfmt) {
			return &gdc_formats[i];
		}
	}

	return NULL;
}

static int gdc_v4l2_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	strncpy(cap->driver, MODULE_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, MODULE_NAME, sizeof(cap->card) - 1);

	cap->capabilities = V4L2_CAP_STREAMING |
		V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE;
	cap->capabilities |= V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = 0x0100 ;

	return 0;
}

static int gdc_v4l2_enum_fmt_mplane(struct file *file, void *fh,
			     struct v4l2_fmtdesc *f)
{
	const struct gdc_fmt *gdc_fmt;

	if (f->index >= ARRAY_SIZE(gdc_formats))
		return -EINVAL;

	gdc_fmt = &gdc_formats[f->index];
	strncpy(f->description, gdc_fmt->name, sizeof(f->description) - 1);
	f->pixelformat = gdc_fmt->pixelformat;

	return 0;
}

static int gdc_v4l2_g_fmt_mplane(struct file *file, void *fh,
			  struct v4l2_format *f)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	const struct gdc_fmt *gdc_fmt;
	struct gdc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	gdc_fmt = frame->gdc_fmt;

	pixm->width		= frame->width;
	pixm->height		= frame->height;
	pixm->pixelformat	= frame->pixelformat;
	pixm->field		= V4L2_FIELD_NONE;
	pixm->num_planes	= frame->gdc_fmt->num_planes;
	pixm->colorspace	= 0;

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = (pixm->width *
				gdc_fmt->bitperpixel[i]) >> 3;
		if (gdc_fmt_is_ayv12(gdc_fmt->pixelformat)) {
			unsigned int y_size, c_span;
			y_size = pixm->width * pixm->height;
			c_span = ALIGN(pixm->width >> 1, 16);
			pixm->plane_fmt[i].sizeimage =
				y_size + (c_span * pixm->height >> 1) * 2;
		} else {
			pixm->plane_fmt[i].sizeimage =
				pixm->plane_fmt[i].bytesperline * pixm->height;
		}

		v4l2_dbg(1, gdc_log_level, &ctx->gdc_dev->m2m.v4l2_dev,
				"[%d] plane: bytesperline %d, sizeimage %d\n",
				i, pixm->plane_fmt[i].bytesperline,
				pixm->plane_fmt[i].sizeimage);
	}

	return 0;
}

static int gdc_v4l2_try_fmt_mplane(struct file *file, void *fh,
			    struct v4l2_format *f)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	const struct gdc_fmt *gdc_fmt;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	const struct gdc_size_limit *limit;
	struct gdc_frame *frame;
	int i, num_planes;
	int h_align = 0;
	int w_align = 0;

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
				"not supported v4l2 type\n");
		return -EINVAL;
	}

	gdc_fmt = gdc_find_format(ctx->gdc_dev, f->fmt.pix_mp.pixelformat, V4L2_TYPE_IS_OUTPUT(f->type));
	if (!gdc_fmt) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
				"not supported format type\n");
		return -EINVAL;
	}

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		limit = &ctx->gdc_dev->variant->limit_input;
	else
		limit = &ctx->gdc_dev->variant->limit_output;

/* ley : need to check */
	w_align = gdc_fmt->h_shift;
	h_align = gdc_fmt->v_shift;

	/* Bound an image to have width and height in limit */
	v4l_bound_align_image(&pixm->width, limit->min_w, limit->max_w,
			w_align, &pixm->height, limit->min_h,
			limit->max_h, h_align, 0);
/**/
	num_planes = pixm->num_planes < GDC_MAX_PLANES ? pixm->num_planes : GDC_MAX_PLANES;
	for (i = 0; i < num_planes; ++i) {
		/* The pixm->plane_fmt[i].sizeimage for the plane which
		 * contains the src blend data has to be calculated as per the
		 * size of the actual width and actual height of the src blend
		 * buffer */

		pixm->plane_fmt[i].bytesperline = (pixm->width *
						gdc_fmt->bitperpixel[i]) >> 3;
		if (gdc_fmt_is_ayv12(gdc_fmt->pixelformat)) {
			unsigned int y_size, c_span;
			y_size = pixm->width * pixm->height;
			c_span = ALIGN(pixm->width >> 1, 16);
			pixm->plane_fmt[i].sizeimage =
				y_size + (c_span * pixm->height >> 1) * 2;
		} else {
			if ((gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_S10B)
				|| (gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_S10B)) {
				if ((i % 2) == 0)
					pixm->plane_fmt[i].sizeimage =
						NV12M_Y_SIZE(pixm->width, pixm->height)
						+ NV12M_Y_2B_SIZE(pixm->width, pixm->height);
				else if ((i % 2) == 1)
					pixm->plane_fmt[i].sizeimage =
						NV12M_CBCR_SIZE(pixm->width, pixm->height)
						+ NV12M_CBCR_2B_SIZE(pixm->width, pixm->height);
			} else if ((gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_S10B)
				|| (gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_S10B)) {
				if ((i % 2) == 0)
					pixm->plane_fmt[i].sizeimage =
						NV16M_Y_SIZE(pixm->width, pixm->height)
						+ NV16M_Y_2B_SIZE(pixm->width, pixm->height);
				else if ((i % 2) == 1)
					pixm->plane_fmt[i].sizeimage =
						NV16M_CBCR_SIZE(pixm->width, pixm->height)
						+ NV16M_CBCR_2B_SIZE(pixm->width, pixm->height);
			} else
				pixm->plane_fmt[i].sizeimage =
					pixm->plane_fmt[i].bytesperline * pixm->height;
		}

		v4l2_dbg(1, gdc_log_level, &ctx->gdc_dev->m2m.v4l2_dev,
				"[%d] plane: bytesperline %d, sizeimage %d\n",
				i, pixm->plane_fmt[i].bytesperline,
				pixm->plane_fmt[i].sizeimage);
	}

	return 0;
}

static int gdc_v4l2_s_fmt_mplane(struct file *file, void *fh,
				 struct v4l2_format *f)

{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	struct gdc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	const struct gdc_size_limit *limitout =
				&ctx->gdc_dev->variant->limit_input;
	const struct gdc_size_limit *limitcap =
				&ctx->gdc_dev->variant->limit_output;
	int i, ret = 0;
	gdc_dbg("gdc s_fmt_mplane\n");

	if (vb2_is_streaming(vq)) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev, "device is busy\n");
		return -EBUSY;
	}

	ret = gdc_v4l2_try_fmt_mplane(file, fh, f);
	if (ret < 0)
		return ret;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	set_bit(CTX_PARAMS, &ctx->flags);

	frame->gdc_fmt = gdc_find_format(ctx->gdc_dev, f->fmt.pix_mp.pixelformat, V4L2_TYPE_IS_OUTPUT(f->type));
	if (!frame->gdc_fmt) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
				"not supported format values\n");
		return -EINVAL;
	}

	for (i = 0; i < frame->gdc_fmt->num_planes; i++)
		frame->bytesused[i] = pixm->plane_fmt[i].sizeimage;

	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width > limitout->max_w) ||
			 (pixm->height > limitout->max_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			"%dx%d of source image is not supported: too large\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width > limitcap->max_w) ||
			 (pixm->height > limitcap->max_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			"%dx%d of target image is not supported: too large\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width < limitout->min_w) ||
			 (pixm->height < limitout->min_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			"%dx%d of source image is not supported: too small\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width < limitcap->min_w) ||
			 (pixm->height < limitcap->min_h))) {
		v4l2_err(&ctx->gdc_dev->m2m.v4l2_dev,
			"%dx%d of target image is not supported: too small\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	frame->width = pixm->width;
	frame->height = pixm->height;
	frame->pixelformat = pixm->pixelformat;
	frame->pixel_size = pixm->reserved[1];

	return 0;
}

static int gdc_v4l2_reqbufs(struct file *file, void *fh,
			    struct v4l2_requestbuffers *reqbufs)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	gdc_dbg("v4l2_reqbuf\n");

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int gdc_v4l2_querybuf(struct file *file, void *fh,
			     struct v4l2_buffer *buf)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	gdc_dbg("v4l2_querybuf\n");
	return v4l2_m2m_querybuf(file, ctx->m2m_ctx, buf);
}

static int gdc_v4l2_qbuf(struct file *file, void *fh,
			 struct v4l2_buffer *buf)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	gdc_dbg("v4l2_qbuf\n");

	return v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
}

static int gdc_v4l2_dqbuf(struct file *file, void *fh,
			  struct v4l2_buffer *buf)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	gdc_dbg("v4l2_d_qbuf\n");
	return v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
}

static int gdc_v4l2_streamon(struct file *file, void *fh,
			     enum v4l2_buf_type type)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	gdc_dbg("v4l2_stream_on\n");
	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static int gdc_v4l2_streamoff(struct file *file, void *fh,
			      enum v4l2_buf_type type)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(fh);
	gdc_dbg("v4l2_stream_off\n");
	return v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
}

static int gdc_v4l2_s_ctrl(struct file * file, void * priv,
			struct v4l2_control *ctrl)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);
	int ret = 0;
	gdc_dbg("v4l2_s_ctrl = %d (%d)\n", ctrl->id, ctrl->value);

	switch (ctrl->id) {
	case V4L2_CID_CAMERAPP_GDC_GRID_CROP_START:
		ctx->crop_param.crop_start_x = (ctrl->value & 0xFFFF0000) >> 16;
		ctx->crop_param.crop_start_y = (ctrl->value & 0x0000FFFF);
		break;
	case V4L2_CID_CAMERAPP_GDC_GRID_CROP_SIZE:
		ctx->crop_param.crop_width = (ctrl->value & 0xFFFF0000) >> 16;
		ctx->crop_param.crop_height = (ctrl->value & 0x0000FFFF);
		break;
	case V4L2_CID_CAMERAPP_GDC_GRID_SENSOR_SIZE:
		ctx->crop_param.sensor_width = (ctrl->value & 0xFFFF0000) >> 16;
		ctx->crop_param.sensor_height = (ctrl->value & 0x0000FFFF);
		break;
	case V4L2_CID_CAMERAPP_SENSOR_NUM:
		ctx->crop_param.sensor_num = ctrl->value;
		gdc_dbg("sensor number = %d\n", ctx->crop_param.sensor_num);
		break;
	default:
		ret = -EINVAL;
		gdc_dbg("Err: Invalid ioctl id(%d)\n", ctrl->id);
		break;
	}

	return ret;
}

static int gdc_v4l2_s_ext_ctrls(struct file * file, void * priv,
				 struct v4l2_ext_controls * ctrls)
{
	int ret = 0;
	int i;
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);
	struct gdc_dev *gdc = ctx->gdc_dev;
	struct v4l2_ext_control *ext_ctrl;
	struct v4l2_control ctrl;
	gdc_dbg("v4l2_s_ext_ctrl\n");

	BUG_ON(!ctx);

	for (i = 0; i < ctrls->count; i++) {
		ext_ctrl = (ctrls->controls + i);

		gdc_dbg("ctrl ID:%d\n", ext_ctrl->id);
		switch (ext_ctrl->id) {
		case V4L2_CID_CAMERAPP_GDC_GRID_CONTROL:
			{
				struct gdc_crop_param *crop_param =
					(struct gdc_crop_param *)&ctx->crop_param;
				ret = copy_from_user(crop_param, ext_ctrl->ptr, sizeof(struct gdc_crop_param));
				gdc_dbg("sensor_num: %d, sensor_size: %dx%d, crop_width:%d\n",
					crop_param->sensor_num, crop_param->sensor_width,
					crop_param->sensor_height, crop_param->crop_width);

				ctx->grid_param.is_valid = false;
				crop_param->is_crop_dzoom = false;

				if ((crop_param->crop_width != crop_param->sensor_width)
					|| (crop_param->crop_height != crop_param->sensor_height))
					crop_param->is_crop_dzoom = true;

				/* Check input crop boundary */
				if (crop_param->crop_start_x + crop_param->crop_width > ctx->s_frame.width
					|| crop_param->crop_start_y + crop_param->crop_height > ctx->s_frame.height) {
					dev_err(gdc->dev, "Invalid input crop size %d,%d %dx%d. Input size %dx%d\n",
							crop_param->crop_start_x, crop_param->crop_start_y,
							crop_param->crop_width, crop_param->crop_height,
							ctx->s_frame.width, ctx->s_frame.height);

					crop_param->crop_start_x = 0;
					crop_param->crop_start_y = 0;
					crop_param->crop_width = ctx->s_frame.width;
					crop_param->crop_height = ctx->s_frame.height;
				}
			}
			break;
		default:
			ctrl.id = ext_ctrl->id;
			ctrl.value = ext_ctrl->value;

			ret = gdc_v4l2_s_ctrl(file, ctx, &ctrl);
			if (ret) {
				gdc_dbg("gdc_v4l2_s_ctrl is fail(%d)\n", ret);
				goto p_err;
			}
			break;
		}
	}

p_err:
	return ret;
}

static const struct v4l2_ioctl_ops gdc_v4l2_ioctl_ops = {
	.vidioc_querycap		= gdc_v4l2_querycap,

	.vidioc_enum_fmt_vid_cap_mplane	= gdc_v4l2_enum_fmt_mplane,
	.vidioc_enum_fmt_vid_out_mplane	= gdc_v4l2_enum_fmt_mplane,

	.vidioc_g_fmt_vid_cap_mplane	= gdc_v4l2_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= gdc_v4l2_g_fmt_mplane,

	.vidioc_try_fmt_vid_cap_mplane	= gdc_v4l2_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= gdc_v4l2_try_fmt_mplane,

	.vidioc_s_fmt_vid_cap_mplane	= gdc_v4l2_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= gdc_v4l2_s_fmt_mplane,

	.vidioc_reqbufs			= gdc_v4l2_reqbufs,
	.vidioc_querybuf		= gdc_v4l2_querybuf,

	.vidioc_qbuf			= gdc_v4l2_qbuf,
	.vidioc_dqbuf			= gdc_v4l2_dqbuf,

	.vidioc_streamon		= gdc_v4l2_streamon,
	.vidioc_streamoff		= gdc_v4l2_streamoff,

	.vidioc_s_ctrl		= gdc_v4l2_s_ctrl,
	.vidioc_s_ext_ctrls		= gdc_v4l2_s_ext_ctrls,
};

static int gdc_ctx_stop_req(struct gdc_ctx *ctx)
{
	struct gdc_ctx *curr_ctx;
	struct gdc_dev *gdc = ctx->gdc_dev;
	int ret = 0;

	curr_ctx = v4l2_m2m_get_curr_priv(gdc->m2m.m2m_dev);
	if (!test_bit(CTX_RUN, &ctx->flags) || (curr_ctx != ctx))
		return 0;

	set_bit(CTX_ABORT, &ctx->flags);

	ret = wait_event_timeout(gdc->wait,
			!test_bit(CTX_RUN, &ctx->flags), GDC_TIMEOUT);

	/* TODO: How to handle case of timeout event */
	if (ret == 0) {
		dev_err(gdc->dev, "device failed to stop request\n");
		ret = -EBUSY;
	}

	return ret;
}

static int gdc_vb2_queue_setup(struct vb2_queue *vq,
		unsigned int *num_buffers, unsigned int *num_planes,
		unsigned int sizes[], struct device *alloc_devs[])
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	struct gdc_frame *frame;
	int i;

	gdc_dbg("gdc queue setup\n");
	frame = ctx_get_frame(ctx, vq->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	/* Get number of planes from format_list in driver */
	*num_planes = frame->gdc_fmt->num_planes;
	for (i = 0; i < frame->gdc_fmt->num_planes; i++) {
		sizes[i] = frame->bytesused[i];
		alloc_devs[i] = ctx->gdc_dev->dev;
	}

	return vb2_queue_init(vq);
}

static int gdc_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct gdc_frame *frame;
	int i;

	frame = ctx_get_frame(ctx, vb->vb2_queue->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		for (i = 0; i < frame->gdc_fmt->num_planes; i++)
			vb2_set_plane_payload(vb, i, frame->bytesused[i]);
	}

	return 0;
}

static void gdc_vb2_buf_finish(struct vb2_buffer *vb)
{
	/* No operation */
}

static void gdc_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	gdc_dbg("gdc buf_queue\n");

	if (ctx->m2m_ctx)
		v4l2_m2m_buf_queue(ctx->m2m_ctx, v4l2_buf);
}

static void gdc_vb2_buf_cleanup(struct vb2_buffer *vb)
{
	/* No operation */
}

static void gdc_vb2_lock(struct vb2_queue *vq)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	mutex_lock(&ctx->gdc_dev->lock);
}

static void gdc_vb2_unlock(struct vb2_queue *vq)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	mutex_unlock(&ctx->gdc_dev->lock);
}

static int gdc_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	set_bit(CTX_STREAMING, &ctx->flags);

	return 0;
}

static void gdc_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct gdc_ctx *ctx = vb2_get_drv_priv(vq);
	int ret;

	ret = gdc_ctx_stop_req(ctx);
	if (ret < 0)
		dev_err(ctx->gdc_dev->dev, "wait timeout\n");

	clear_bit(CTX_STREAMING, &ctx->flags);
}

static struct vb2_ops gdc_vb2_ops = {
	.queue_setup		= gdc_vb2_queue_setup,
	.buf_prepare		= gdc_vb2_buf_prepare,
	.buf_finish		= gdc_vb2_buf_finish,
	.buf_queue		= gdc_vb2_buf_queue,
	.buf_cleanup		= gdc_vb2_buf_cleanup,
	.wait_finish		= gdc_vb2_lock,
	.wait_prepare		= gdc_vb2_unlock,
	.start_streaming	= gdc_vb2_start_streaming,
	.stop_streaming		= gdc_vb2_stop_streaming,
};

static int queue_init(void *priv, struct vb2_queue *src_vq,
		      struct vb2_queue *dst_vq)
{
	struct gdc_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->ops = &gdc_vb2_ops;
	src_vq->mem_ops = &vb2_dma_sg_memops;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct vb2_gdc_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->ops = &gdc_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_sg_memops;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct vb2_gdc_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	return vb2_queue_init(dst_vq);
}

static int gdc_power_clk_enable(struct gdc_dev *gdc)
{
	int ret;

	if (in_interrupt())
		ret = pm_runtime_get(gdc->dev);
	else
		ret = pm_runtime_get_sync(gdc->dev);

	if (ret < 0) {
		dev_err(gdc->dev,
			"%s=%d: Failed to enable local power\n", __func__, ret);
		return ret;
	}

	if (!IS_ERR(gdc->pclk)) {
		ret = clk_enable(gdc->pclk);
		if (ret) {
			dev_err(gdc->dev, "%s: Failed to enable PCLK (err %d)\n",
				__func__, ret);
			goto err_pclk;
		}
	}

	if (!IS_ERR(gdc->aclk)) {
		ret = clk_enable(gdc->aclk);
		if (ret) {
			dev_err(gdc->dev, "%s: Failed to enable ACLK (err %d)\n",
				__func__, ret);
			goto err_aclk;
		}
	}

	return 0;
err_aclk:
	if (!IS_ERR(gdc->pclk))
		clk_disable(gdc->pclk);
err_pclk:
	pm_runtime_put(gdc->dev);
	return ret;
}

static void gdc_clk_power_disable(struct gdc_dev *gdc)
{
	if (!IS_ERR(gdc->aclk))
		clk_disable(gdc->aclk);

	if (!IS_ERR(gdc->pclk))
		clk_disable(gdc->pclk);

	pm_runtime_put(gdc->dev);
}

static int gdc_open(struct file *file)
{
	struct gdc_dev *gdc = video_drvdata(file);
	struct gdc_ctx *ctx;
	int ret = 0;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(gdc->dev, "no memory for open context\n");
		return -ENOMEM;
	}

	atomic_inc(&gdc->m2m.in_use);

	INIT_LIST_HEAD(&ctx->node);
	ctx->gdc_dev = gdc;

	v4l2_fh_init(&ctx->fh, gdc->m2m.vfd);
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	/* Default color format */
	ctx->s_frame.gdc_fmt = &gdc_formats[0];
	ctx->d_frame.gdc_fmt = &gdc_formats[0];

	if (!IS_ERR(gdc->pclk)) {
		ret = clk_prepare(gdc->pclk);
		if (ret) {
			dev_err(gdc->dev, "%s: failed to prepare PCLK(err %d)\n",
					__func__, ret);
			goto err_pclk_prepare;
		}
	}

	if (!IS_ERR(gdc->aclk)) {
		ret = clk_prepare(gdc->aclk);
		if (ret) {
			dev_err(gdc->dev, "%s: failed to prepare ACLK(err %d)\n",
					__func__, ret);
			goto err_aclk_prepare;
		}
	}

	/* Setup the device context for mem2mem mode. */
	ctx->m2m_ctx = v4l2_m2m_ctx_init(gdc->m2m.m2m_dev, ctx, queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = -EINVAL;
		goto err_ctx;
	}

	gdc_dbg("gdc open = %d\n", ret);
	return 0;

err_ctx:
	if (!IS_ERR(gdc->aclk))
		clk_unprepare(gdc->aclk);
err_aclk_prepare:
	if (!IS_ERR(gdc->pclk))
		clk_unprepare(gdc->pclk);
err_pclk_prepare:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	atomic_dec(&gdc->m2m.in_use);
	kfree(ctx);

	return ret;
}

static int gdc_release(struct file *file)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);
	struct gdc_dev *gdc = ctx->gdc_dev;

	gdc_dbg("refcnt= %d", atomic_read(&gdc->m2m.in_use));

	atomic_dec(&gdc->m2m.in_use);

	gdc_dbg("gdc close\n");

	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	if (!IS_ERR(gdc->aclk))
		clk_unprepare(gdc->aclk);
	if (!IS_ERR(gdc->pclk))
		clk_unprepare(gdc->pclk);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	return 0;
}

static unsigned int gdc_poll(struct file *file,
			     struct poll_table_struct *wait)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);

	return v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
}

static int gdc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct gdc_ctx *ctx = fh_to_gdc_ctx(file->private_data);

	return v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
}

static const struct v4l2_file_operations gdc_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= gdc_open,
	.release	= gdc_release,
	.poll		= gdc_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= gdc_mmap,
};

static void gdc_job_finish(struct gdc_dev *gdc, struct gdc_ctx *ctx)
{
	unsigned long flags;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;

	spin_lock_irqsave(&gdc->slock, flags);

	ctx = v4l2_m2m_get_curr_priv(gdc->m2m.m2m_dev);
	if (!ctx || !ctx->m2m_ctx) {
		dev_err(gdc->dev, "current ctx is NULL\n");
		spin_unlock_irqrestore(&gdc->slock, flags);
		return;

	}
	clear_bit(CTX_RUN, &ctx->flags);

	src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

	BUG_ON(!src_vb || !dst_vb);

	v4l2_m2m_buf_done(src_vb, VB2_BUF_STATE_ERROR);
	v4l2_m2m_buf_done(dst_vb, VB2_BUF_STATE_ERROR);

	v4l2_m2m_job_finish(gdc->m2m.m2m_dev, ctx->m2m_ctx);

	spin_unlock_irqrestore(&gdc->slock, flags);
}

static void gdc_watchdog(unsigned long arg)
{
	struct gdc_dev *gdc = (struct gdc_dev *)arg;
	struct gdc_ctx *ctx;
	unsigned long flags;

	gdc_dbg("timeout watchdog\n");
	if (atomic_read(&gdc->wdt.cnt) >= GDC_WDT_CNT) {
		camerapp_hw_gdc_sw_reset(gdc->regs_base);

		atomic_set(&gdc->wdt.cnt, 0);
		clear_bit(DEV_RUN, &gdc->state);

		spin_lock_irqsave(&gdc->ctxlist_lock, flags);
		ctx = gdc->current_ctx;
		gdc->current_ctx = NULL;
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);

		BUG_ON(!ctx);
		gdc_job_finish(gdc, ctx);
		gdc_clk_power_disable(gdc);
		return;
	}

	if (test_bit(DEV_RUN, &gdc->state)) {
		camerapp_gdc_sfr_dump(gdc->regs_base);
		exynos_sysmmu_show_status(gdc->dev);
		atomic_inc(&gdc->wdt.cnt);
		dev_err(gdc->dev, "gdc is still running\n");
		mod_timer(&gdc->wdt.timer, jiffies + GDC_TIMEOUT);
	} else {
		gdc_dbg("gdc finished job\n");
	}

}


#define GDC_SRC_PBCONFIG	(SYSMMU_PBUFCFG_TLB_UPDATE |		\
			SYSMMU_PBUFCFG_ASCENDING | SYSMMU_PBUFCFG_READ)
#define GDC_DST_PBCONFIG	(SYSMMU_PBUFCFG_TLB_UPDATE |		\
			SYSMMU_PBUFCFG_ASCENDING | SYSMMU_PBUFCFG_WRITE)

static void gdc_set_prefetch_buffers(struct device *dev, struct gdc_ctx *ctx)
{
	struct gdc_frame *s_frame = &ctx->s_frame;
	struct gdc_frame *d_frame = &ctx->d_frame;
	struct sysmmu_prefbuf pb_reg[6];
	unsigned int i = 0;

	pb_reg[i].base = s_frame->addr.y;
	pb_reg[i].size = s_frame->addr.ysize;
	pb_reg[i++].config = GDC_SRC_PBCONFIG;
	if (s_frame->gdc_fmt->num_comp >= 2) {
		pb_reg[i].base = s_frame->addr.cb;
		pb_reg[i].size = s_frame->addr.cbsize;
		pb_reg[i++].config = GDC_SRC_PBCONFIG;
	}
	if (s_frame->gdc_fmt->num_comp >= 3) {
		pb_reg[i].base = s_frame->addr.cr;
		pb_reg[i].size = s_frame->addr.crsize;
		pb_reg[i++].config = GDC_SRC_PBCONFIG;
	}

	pb_reg[i].base = d_frame->addr.y;
	pb_reg[i].size = d_frame->addr.ysize;
	pb_reg[i++].config = GDC_DST_PBCONFIG;
	if (d_frame->gdc_fmt->num_comp >= 2) {
		pb_reg[i].base = d_frame->addr.cb;
		pb_reg[i].size = d_frame->addr.cbsize;
		pb_reg[i++].config = GDC_DST_PBCONFIG;
	}
	if (d_frame->gdc_fmt->num_comp >= 3) {
		pb_reg[i].base = d_frame->addr.cr;
		pb_reg[i].size = d_frame->addr.crsize;
		pb_reg[i++].config = GDC_DST_PBCONFIG;
	}
}
static int gdc_run_next_job(struct gdc_dev *gdc)
{
	unsigned long flags;
	struct gdc_ctx *ctx;
	struct gdc_frame *d_frame, *s_frame;

	int ret;

	spin_lock_irqsave(&gdc->ctxlist_lock, flags);

	if (gdc->current_ctx || list_empty(&gdc->context_list)) {
		/* a job is currently being processed or no job is to run */
		spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);
		return 0;
	}

	ctx = list_first_entry(&gdc->context_list, struct gdc_ctx, node);

	list_del_init(&ctx->node);

	gdc->current_ctx = ctx;

	spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);

	/*
	 * gdc_run_next_job() must not reenter while gdc->state is DEV_RUN.
	 * DEV_RUN is cleared when an operation is finished.
	 */
	gdc_dbg("gdc hw setting\n");

	BUG_ON(test_bit(DEV_RUN, &gdc->state));

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	ret = gdc_power_clk_enable(gdc);
	if (ret) {
		pm_runtime_put(gdc->dev);
		return ret;
	}
	gdc_dbg("gdc clk enable\n");

	camerapp_hw_gdc_sw_reset(gdc->regs_base);

	gdc_dbg("gdc sw reset\n");

	camerapp_gdc_grid_setting(gdc);

	camerapp_hw_gdc_update_param(gdc->regs_base, gdc);
	gdc_dbg("gdc tpu param update done\n");

	set_bit(DEV_RUN, &gdc->state);
	set_bit(CTX_RUN, &ctx->flags);

/* ley : need to check : smmu_prefetch buffer setting */
	gdc_set_prefetch_buffers(gdc->dev, ctx);
	mod_timer(&gdc->wdt.timer, jiffies + GDC_TIMEOUT);

	if (__gdc_measure_hw_latency) {
		struct vb2_v4l2_buffer *vb =
			v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
		struct v4l2_m2m_buffer *mb =
			container_of(vb, typeof(*mb), vb);
		struct vb2_gdc_buffer *svb =
			container_of(mb, typeof(*svb), mb);

		svb->ktime = ktime_get();
	}

	camerapp_hw_gdc_start(gdc->regs_base);

	return 0;
}

static int gdc_add_context_and_run(struct gdc_dev *gdc, struct gdc_ctx *ctx)
{
	unsigned long flags;

	spin_lock_irqsave(&gdc->ctxlist_lock, flags);
	list_add_tail(&ctx->node, &gdc->context_list);
	spin_unlock_irqrestore(&gdc->ctxlist_lock, flags);

	return gdc_run_next_job(gdc);
}

static irqreturn_t gdc_irq_handler(int irq, void *priv)
{
	struct gdc_dev *gdc = priv;
	struct gdc_ctx *ctx;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;
	u32 irq_status;

	gdc_dbg("irq handler\n");
	spin_lock(&gdc->slock);

	clear_bit(DEV_RUN, &gdc->state);

	/*
	 * ok to access gdc->current_ctx withot ctxlist_lock held
	 * because it is not modified until gdc_run_next_job() is called.
	 */
	ctx = gdc->current_ctx;

	BUG_ON(!ctx);

	irq_status = camerapp_hw_gdc_get_intr_status_and_clear(gdc->regs_base);

	if (GDC_INT_OK(irq_status)) {
		gdc_dbg("gdc frame end\n");

		gdc_dbg("intr = %x\n", irq_status);

		del_timer(&gdc->wdt.timer);

		gdc_clk_power_disable(gdc);

		clear_bit(CTX_RUN, &ctx->flags);

		BUG_ON(ctx != v4l2_m2m_get_curr_priv(gdc->m2m.m2m_dev));

		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		BUG_ON(!src_vb || !dst_vb);

		if (__gdc_measure_hw_latency) {
			struct v4l2_m2m_buffer *mb =
				container_of(dst_vb, typeof(*mb), vb);
			struct vb2_gdc_buffer *svb =
				container_of(mb, typeof(*svb), mb);

			dst_vb->vb2_buf.timestamp =
				(__u32)ktime_us_delta(ktime_get(), svb->ktime);
		}

		v4l2_m2m_buf_done(src_vb,
				GDC_INT_OK(irq_status) ?
				VB2_BUF_STATE_DONE : VB2_BUF_STATE_ERROR);
		v4l2_m2m_buf_done(dst_vb,
				GDC_INT_OK(irq_status) ?
				VB2_BUF_STATE_DONE : VB2_BUF_STATE_ERROR);

		if (test_bit(DEV_SUSPEND, &gdc->state)) {
			gdc_dbg("wake up blocked process by suspend\n");
			wake_up(&gdc->wait);
		} else {
			v4l2_m2m_job_finish(gdc->m2m.m2m_dev, ctx->m2m_ctx);
		}

		/* Wake up from CTX_ABORT state */
		if (test_and_clear_bit(CTX_ABORT, &ctx->flags))
			wake_up(&gdc->wait);

		spin_lock(&gdc->ctxlist_lock);
		gdc->current_ctx = NULL;
		spin_unlock(&gdc->ctxlist_lock);
	}

	spin_unlock(&gdc->slock);

	return IRQ_HANDLED;
}

static int gdc_get_bufaddr(struct gdc_dev *gdc, struct gdc_ctx *ctx,
		struct vb2_buffer *vb2buf, struct gdc_frame *frame)
{
	unsigned int pixsize, bytesize;
	unsigned int w = frame->width;
	unsigned int h = frame->height;

	pixsize = frame->width * frame->height;
	bytesize = (pixsize * frame->gdc_fmt->bitperpixel[0]) >> 3;

	frame->addr.y = gdc_get_dma_address(vb2buf, 0);
	if (!frame->addr.y)
		return -EINVAL;

	frame->addr.cb = 0;
	frame->addr.cr = 0;
	frame->addr.y_2bit = 0;
	frame->addr.cbcr_2bit = 0;
	frame->addr.cbsize = 0;
	frame->addr.crsize = 0;
	frame->addr.ysize_2bit = 0;
	frame->addr.cbcrsize_2bit = 0;

	switch (frame->gdc_fmt->num_comp) {
	case 1: /* rgb, yuyv */
		frame->addr.ysize = bytesize;
		break;
	case 2:
		if (frame->gdc_fmt->num_planes == 1) {
			/* V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV21,  V4L2_PIX_FMT_NV61, V4L2_PIX_FMT_NV16 */
			frame->addr.cb = frame->addr.y + pixsize;
			frame->addr.ysize = pixsize;
			frame->addr.cbsize = bytesize - pixsize;
		} else if (frame->gdc_fmt->num_planes == 2) {
			/* V4L2_PIX_FMT_NV21M, V4L2_PIX_FMT_NV12M */
			/* V4L2_PIX_FMT_NV61M, V4L2_PIX_FMT_NV16M */
			/* V4L2_PIX_FMT_NV12M_P010, V4L2_PIX_FMT_NV16M_P210 */
			/* V4L2_PIX_FMT_NV21M_P010, V4L2_PIX_FMT_NV61M_P210 */
			frame->addr.cb = gdc_get_dma_address(vb2buf, 1);
			if (!frame->addr.cb)
				return -EINVAL;

			/* 8+2 format */
			if ((frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV16M_S10B)
					|| (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV61M_S10B)) {
				frame->addr.ysize_2bit = NV16M_Y_SIZE(w, h);
				frame->addr.y_2bit = frame->addr.y + NV16M_Y_SIZE(w, h);
				frame->addr.cbcrsize_2bit = NV16M_CBCR_SIZE(w, h);
				frame->addr.cbcr_2bit = frame->addr.cb + NV16M_CBCR_SIZE(w,h);
			} else if ((frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV12M_S10B)
					|| (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_NV21M_S10B)) {
				frame->addr.ysize_2bit = NV12M_Y_SIZE(w, h);
				frame->addr.y_2bit = frame->addr.y + NV12M_Y_SIZE(w, h);
				frame->addr.cbcrsize_2bit = NV12M_CBCR_SIZE(w, h);
				frame->addr.cbcr_2bit = frame->addr.cb + NV12M_CBCR_SIZE(w,h);
			}
		} else {
			dev_err(gdc->dev, "Please check frame->gdc_fmt->pixelformat\n");
		}
		break;
	case 3:
		if (frame->gdc_fmt->num_planes == 1) {
			if (gdc_fmt_is_ayv12(frame->gdc_fmt->pixelformat)) {
				unsigned int c_span;
				c_span = ALIGN(frame->width >> 1, 16);
				frame->addr.ysize = pixsize;
				frame->addr.cbsize = c_span * (frame->height >> 1);
				frame->addr.crsize = frame->addr.cbsize;
				frame->addr.cb = frame->addr.y + pixsize;
				frame->addr.cr = frame->addr.cb + frame->addr.cbsize;
			} else if (frame->gdc_fmt->pixelformat ==
					V4L2_PIX_FMT_YUV420N) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.ysize = YUV420N_Y_SIZE(w, h);
				frame->addr.cbsize = YUV420N_CB_SIZE(w, h);
				frame->addr.crsize = YUV420N_CR_SIZE(w, h);
				frame->addr.cb =
					YUV420N_CB_BASE(frame->addr.y, w, h);
				frame->addr.cr =
					YUV420N_CR_BASE(frame->addr.y, w, h);
			} else {
				frame->addr.ysize = pixsize;
				frame->addr.cbsize = (bytesize - pixsize) / 2;
				frame->addr.crsize = frame->addr.cbsize;
				frame->addr.cb = frame->addr.y + pixsize;
				frame->addr.cr = frame->addr.cb + frame->addr.cbsize;
			}
		} else if (frame->gdc_fmt->num_planes == 3) {
			frame->addr.cb = gdc_get_dma_address(vb2buf, 1);
			if (!frame->addr.cb)
				return -EINVAL;

			frame->addr.cr = gdc_get_dma_address(vb2buf, 2);
			if (!frame->addr.cr)
				return -EINVAL;
			frame->addr.ysize =
				pixsize * frame->gdc_fmt->bitperpixel[0] >> 3;
			frame->addr.cbsize =
				pixsize * frame->gdc_fmt->bitperpixel[1] >> 3;
			frame->addr.crsize =
				pixsize * frame->gdc_fmt->bitperpixel[2] >> 3;
		}
		break;
	default:
		break;
	}

	if (frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_YVU420 || frame->gdc_fmt->pixelformat == V4L2_PIX_FMT_YVU420M) {
		u32 t_cb = frame->addr.cb;
		frame->addr.cb = frame->addr.cr;
		frame->addr.cr = t_cb;
	}

#ifdef ENABLE_PRINT_GDC_BUFADDR
	dev_info(gdc->dev, "y addr %pa y size %#x\n", &frame->addr.y, frame->addr.ysize);
	dev_info(gdc->dev, "cb addr %pa cb size %#x\n", &frame->addr.cb, frame->addr.cbsize);
	dev_info(gdc->dev, "cr addr %pa cr size %#x\n", &frame->addr.cr, frame->addr.crsize);
#endif

	return 0;
}

static void gdc_m2m_device_run(void *priv)
{
	struct gdc_ctx *ctx = priv;
	struct gdc_dev *gdc = ctx->gdc_dev;
	struct gdc_frame *s_frame, *d_frame;
	gdc_dbg("gdc m2m device run\n");

	if (test_bit(DEV_SUSPEND, &gdc->state)) {
		dev_err(gdc->dev, "GDC is in suspend state\n");
		return;
	}

	if (test_bit(CTX_ABORT, &ctx->flags)) {
		dev_err(gdc->dev, "aborted GDC device run\n");
		return;
	}

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	gdc_get_bufaddr(gdc, ctx, v4l2_m2m_next_src_buf(ctx->m2m_ctx), s_frame);
	gdc_get_bufaddr(gdc, ctx, v4l2_m2m_next_dst_buf(ctx->m2m_ctx), d_frame);

	gdc_dbg("gdc_src : format = %d, w = %d, h = %d\n", s_frame->gdc_fmt->cfg_val, s_frame->width, s_frame->height);
	gdc_dbg("gdc_dst : format = %d, w = %d, h = %d\n", d_frame->gdc_fmt->cfg_val, d_frame->width, d_frame->height);
	gdc_add_context_and_run(gdc, ctx);
}

static void gdc_m2m_job_abort(void *priv)
{
	struct gdc_ctx *ctx = priv;
	int ret;

	ret = gdc_ctx_stop_req(ctx);
	if (ret < 0)
		dev_err(ctx->gdc_dev->dev, "wait timeout\n");
}

static struct v4l2_m2m_ops gdc_m2m_ops = {
	.device_run	= gdc_m2m_device_run,
	.job_abort	= gdc_m2m_job_abort,
};

static void gdc_unregister_m2m_device(struct gdc_dev *gdc)
{
	v4l2_m2m_release(gdc->m2m.m2m_dev);
	video_device_release(gdc->m2m.vfd);
	v4l2_device_unregister(&gdc->m2m.v4l2_dev);
}

static int gdc_register_m2m_device(struct gdc_dev *gdc, int dev_id)
{
	struct v4l2_device *v4l2_dev;
	struct device *dev;
	struct video_device *vfd;
	int ret = 0;

	dev = gdc->dev;
	v4l2_dev = &gdc->m2m.v4l2_dev;

	scnprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s.m2m",
			MODULE_NAME);

	ret = v4l2_device_register(dev, v4l2_dev);
	if (ret) {
		dev_err(gdc->dev, "failed to register v4l2 device\n");
		return ret;
	}

	vfd = video_device_alloc();
	if (!vfd) {
		dev_err(gdc->dev, "failed to allocate video device\n");
		goto err_v4l2_dev;
	}

	vfd->fops	= &gdc_v4l2_fops;
	vfd->ioctl_ops	= &gdc_v4l2_ioctl_ops;
	vfd->release	= video_device_release;
	vfd->lock	= &gdc->lock;
	vfd->vfl_dir	= VFL_DIR_M2M;
	vfd->v4l2_dev	= v4l2_dev;
	scnprintf(vfd->name, sizeof(vfd->name), "%s:m2m", MODULE_NAME);

	video_set_drvdata(vfd, gdc);

	gdc->m2m.vfd = vfd;
	gdc->m2m.m2m_dev = v4l2_m2m_init(&gdc_m2m_ops);
	if (IS_ERR(gdc->m2m.m2m_dev)) {
		dev_err(gdc->dev, "failed to initialize v4l2-m2m device\n");
		ret = PTR_ERR(gdc->m2m.m2m_dev);
		goto err_dev_alloc;
	}

	ret = video_register_device(vfd, VFL_TYPE_GRABBER,
				EXYNOS_VIDEONODE_CAMERAPP(CAMERAPP_VIDEONODE_GDC));
	if (ret) {
		dev_err(gdc->dev, "failed to register video device\n");
		goto err_m2m_dev;
	}

	return 0;

err_m2m_dev:
	v4l2_m2m_release(gdc->m2m.m2m_dev);
err_dev_alloc:
	video_device_release(gdc->m2m.vfd);
err_v4l2_dev:
	v4l2_device_unregister(v4l2_dev);

	return ret;
}

static int __attribute__((unused)) gdc_sysmmu_fault_handler(struct iommu_domain *domain,
	struct device *dev, unsigned long iova, int flags, void *token)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);

	if (test_bit(DEV_RUN, &gdc->state)) {
		dev_info(dev, "System MMU fault called for IOVA %#lx\n", iova);
		camerapp_gdc_sfr_dump(gdc->regs_base);
	}

	return 0;
}

static int gdc_clk_get(struct gdc_dev *gdc)
{
	gdc->aclk = devm_clk_get(gdc->dev, "gate");
	if (IS_ERR(gdc->aclk)) {
		if (PTR_ERR(gdc->aclk) != -ENOENT) {
			dev_err(gdc->dev, "Failed to get 'gate' clock: %ld",
				PTR_ERR(gdc->aclk));
			return PTR_ERR(gdc->aclk);
		}
		dev_info(gdc->dev, "'gate' clock is not present\n");
	}

	gdc->pclk = devm_clk_get(gdc->dev, "gate2");
	if (IS_ERR(gdc->pclk)) {
		if (PTR_ERR(gdc->pclk) != -ENOENT) {
			dev_err(gdc->dev, "Failed to get 'gate2' clock: %ld",
				PTR_ERR(gdc->pclk));
			return PTR_ERR(gdc->pclk);
		}
		dev_info(gdc->dev, "'gate2' clock is not present\n");
	}

	gdc->clk_chld = devm_clk_get(gdc->dev, "mux_user");
	if (IS_ERR(gdc->clk_chld)) {
		if (PTR_ERR(gdc->clk_chld) != -ENOENT) {
			dev_err(gdc->dev, "Failed to get 'mux_user' clock: %ld",
				PTR_ERR(gdc->clk_chld));
			return PTR_ERR(gdc->clk_chld);
		}
		dev_info(gdc->dev, "'mux_user' clock is not present\n");
	}

	if (!IS_ERR(gdc->clk_chld)) {
		gdc->clk_parn = devm_clk_get(gdc->dev, "mux_src");
		if (IS_ERR(gdc->clk_parn)) {
			dev_err(gdc->dev, "Failed to get 'mux_src' clock: %ld",
				PTR_ERR(gdc->clk_parn));
			return PTR_ERR(gdc->clk_parn);
		}
	} else {
		gdc->clk_parn = ERR_PTR(-ENOENT);
	}

	return 0;
}

static void gdc_clk_put(struct gdc_dev *gdc)
{
	if (!IS_ERR(gdc->clk_parn))
		clk_put(gdc->clk_parn);

	if (!IS_ERR(gdc->clk_chld))
		clk_put(gdc->clk_chld);

	if (!IS_ERR(gdc->pclk))
		clk_put(gdc->pclk);

	if (!IS_ERR(gdc->aclk))
		clk_put(gdc->aclk);
}

#ifdef CONFIG_PM_SLEEP
static int gdc_suspend(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);
	int ret;

	set_bit(DEV_SUSPEND, &gdc->state);

	ret = wait_event_timeout(gdc->wait,
			!test_bit(DEV_RUN, &gdc->state), GDC_TIMEOUT);
	if (ret == 0)
		dev_err(gdc->dev, "wait timeout\n");

	return 0;
}

static int gdc_resume(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);

	clear_bit(DEV_SUSPEND, &gdc->state);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int gdc_runtime_resume(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);

	if (!IS_ERR(gdc->clk_chld) && !IS_ERR(gdc->clk_parn)) {
		int ret = clk_set_parent(gdc->clk_chld, gdc->clk_parn);
		if (ret) {
			dev_err(gdc->dev, "%s: Failed to setup MUX: %d\n",
				__func__, ret);
			return ret;
		}
	}

	if (gdc->qosreq_intcam_level > 0)
		pm_qos_update_request(&gdc->qosreq_intcam, gdc->qosreq_intcam_level);

	return 0;
}

static int gdc_runtime_suspend(struct device *dev)
{
	struct gdc_dev *gdc = dev_get_drvdata(dev);
	if (gdc->qosreq_intcam_level > 0)
		pm_qos_update_request(&gdc->qosreq_intcam, 0);
	return 0;
}
#endif

static const struct dev_pm_ops gdc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(gdc_suspend, gdc_resume)
	SET_RUNTIME_PM_OPS(NULL, gdc_runtime_resume, gdc_runtime_suspend)
};

static int gdc_probe(struct platform_device *pdev)
{
	struct gdc_dev *gdc;
	struct resource *rsc;
	int ret = 0;

	gdc = devm_kzalloc(&pdev->dev, sizeof(struct gdc_dev), GFP_KERNEL);
	if (!gdc) {
		dev_err(&pdev->dev, "no memory for GDC device\n");
		return -ENOMEM;
	}

	gdc->dev = &pdev->dev;

	spin_lock_init(&gdc->ctxlist_lock);
	INIT_LIST_HEAD(&gdc->context_list);
	spin_lock_init(&gdc->slock);
	mutex_init(&gdc->lock);
	init_waitqueue_head(&gdc->wait);

	rsc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gdc->regs_base = devm_ioremap_nocache(&pdev->dev, rsc->start, resource_size(rsc));
	if (IS_ERR(gdc->regs_base))
		return PTR_ERR(gdc->regs_base);

	rsc = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!rsc) {
		dev_err(&pdev->dev, "failed to get IRQ resource\n");
		return -ENOENT;
	}

	ret = devm_request_irq(&pdev->dev, rsc->start, gdc_irq_handler, 0,
			pdev->name, gdc);
	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		return ret;
	}

	atomic_set(&gdc->wdt.cnt, 0);
	setup_timer(&gdc->wdt.timer, gdc_watchdog, (unsigned long)gdc);

	ret = gdc_clk_get(gdc);
	if (ret)
		return ret;

	if (pdev->dev.of_node)
		gdc->dev_id = of_alias_get_id(pdev->dev.of_node, "camerapp-gdc");
	else
		gdc->dev_id = pdev->id;

	platform_set_drvdata(pdev, gdc);

	pm_runtime_enable(&pdev->dev);

	ret = gdc_register_m2m_device(gdc, gdc->dev_id);
	if (ret) {
		dev_err(&pdev->dev, "failed to register m2m device\n");
		goto err_wq;
	}

#if defined(CONFIG_PM_DEVFREQ)
	if (!of_property_read_u32(pdev->dev.of_node, "camerapp_gdc,intcam_qos_minlock",
				(u32 *)&gdc->qosreq_intcam_level)) {
		if (gdc->qosreq_intcam_level > 0) {
			pm_qos_add_request(&gdc->qosreq_intcam,
						PM_QOS_INTCAM_THROUGHPUT, 0);
			dev_info(&pdev->dev, "INTCAM Min.Lock Freq. = %u\n",
						gdc->qosreq_intcam_level);
		}
	}
#endif
/*
	if (of_property_read_u32(pdev->dev.of_node, "camerapp_gdc,cfw",
				(u32 *)&gdc->cfw))
		gdc->cfw = 0;

	if (of_find_property(pdev->dev.of_node, "prefetch-buffer,disable",
				NULL))
		gdc->pb_disable = true;
*/
	ret = iovmm_activate(gdc->dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to attach iommu\n");
		goto err_iommu;
	}

	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: failed to local power on (err %d)\n",
			__func__, ret);
		goto err_ver_rpm_get;
	}

	if (!IS_ERR(gdc->pclk)) {
		ret = clk_prepare_enable(gdc->pclk);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: failed to enable PCLK (err %d)\n",
				__func__, ret);
			goto err_ver_pclk_get;
		}
	}

	if (!IS_ERR(gdc->aclk)) {
		ret = clk_prepare_enable(gdc->aclk);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: failed to enable ACLK (err %d)\n",
				__func__, ret);
			goto err_ver_aclk_get;
		}
	}

	gdc->version = 0;	/* no version register in GDCv1.0 */

	gdc->variant = &gdc_variant[0];

	if (!IS_ERR(gdc->aclk))
		clk_disable_unprepare(gdc->aclk);
	if (!IS_ERR(gdc->pclk))
		clk_disable_unprepare(gdc->pclk);
	pm_runtime_put(&pdev->dev);

	iovmm_set_fault_handler(&pdev->dev, gdc_sysmmu_fault_handler, gdc);

	dev_info(&pdev->dev,
		"Driver probed successfully(version: %08x)\n",
		gdc->version);

	return 0;
err_ver_aclk_get:
	if (!IS_ERR(gdc->pclk))
		clk_disable_unprepare(gdc->pclk);
err_ver_pclk_get:
	pm_runtime_put(&pdev->dev);
err_ver_rpm_get:
	iovmm_deactivate(gdc->dev);
err_iommu:
	if (gdc->qosreq_intcam_level > 0)
		pm_qos_remove_request(&gdc->qosreq_intcam);
	gdc_unregister_m2m_device(gdc);
err_wq:
	return ret;
}

static int gdc_remove(struct platform_device *pdev)
{
	struct gdc_dev *gdc = platform_get_drvdata(pdev);

	iovmm_deactivate(gdc->dev);

	gdc_clk_put(gdc);

	if (timer_pending(&gdc->wdt.timer))
		del_timer(&gdc->wdt.timer);

	if (gdc->qosreq_intcam_level > 0)
		pm_qos_remove_request(&gdc->qosreq_intcam);

	return 0;
}

static void gdc_shutdown(struct platform_device *pdev)
{
	struct gdc_dev *gdc = platform_get_drvdata(pdev);
	unsigned long flags;

	spin_lock_irqsave(&gdc->slock, flags);
	set_bit(DEV_SUSPEND, &gdc->state);
	spin_unlock_irqrestore(&gdc->slock, flags);

	wait_event(gdc->wait,
			!test_bit(DEV_RUN, &gdc->state));

	iovmm_deactivate(gdc->dev);
}
static const struct of_device_id exynos_gdc_match[] = {
	{
		.compatible = "samsung,exynos5-camerapp-gdc",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_gdc_match);

static struct platform_driver gdc_driver = {
	.probe		= gdc_probe,
	.remove		= gdc_remove,
	.shutdown	= gdc_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &gdc_pm_ops,
		.of_match_table = of_match_ptr(exynos_gdc_match),
	}
};

module_platform_driver(gdc_driver);

MODULE_AUTHOR("SamsungLSI Camera");
MODULE_DESCRIPTION("EXYNOS CameraPP GDC driver");
MODULE_LICENSE("GPL");
