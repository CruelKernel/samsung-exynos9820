/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The main source file of Samsung Exynos SMFC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include "smfc.h"

/* SMFC SPECIFIC CONTROLS */
#define V4L2_CID_JPEG_SEC_COMP_QUALITY	(V4L2_CID_JPEG_CLASS_BASE + 20)
#define V4L2_CID_JPEG_QTABLES2		(V4L2_CID_JPEG_CLASS_BASE + 22)
#define V4L2_CID_JPEG_HWFC_ENABLE	(V4L2_CID_JPEG_CLASS_BASE + 25)

#define SMFC_FMT_MAIN_SIZE(val) ((val) & 0xFFFF)
#define SMFC_FMT_SEC_SIZE(val) (((val) >> 16) & 0xFFFF)

const struct smfc_image_format smfc_image_formats[] = {
	{
		/* JPEG should be the first format */
		.description	= "Baseline JPEG(Sequential DCT)",
		.v4l2_pixfmt	= V4L2_PIX_FMT_JPEG,
		.regcfg		= 0, /* Chagneable accroding to chroma factor */
		.bpp_buf	= {0}, /* undeterministic */
		.bpp_pix	= {0}, /* undeterministic */
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1, /* dummy chroma subsampling factor */
		.chroma_vfactor	= 1, /* These will not affect to H/W config. */
	}, {
		/* YUYV is the default format */
		.description	= "YUV4:2:2 1-Plane 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YUYV,
		.regcfg		= SMFC_IMGFMT_YUYV,
		.bpp_buf	= {16},
		.bpp_pix	= {16},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 1,
	}, {
		.description	= "YUV4:2:2 1-Plane 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YVYU,
		.regcfg		= SMFC_IMGFMT_YVYU,
		.bpp_buf	= {16},
		.bpp_pix	= {16},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 1,
	}, {
		.description	= "YUV4:2:2 2-Plane 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_NV16,
		.regcfg		= SMFC_IMGFMT_NV16,
		.bpp_buf	= {16},
		.bpp_pix	= {8, 8, 0},
		.num_planes	= 2,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 1,
	}, {
		.description	= "YUV4:2:2 2-Plane 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_NV61,
		.regcfg		= SMFC_IMGFMT_NV61,
		.bpp_buf	= {16},
		.bpp_pix	= {8, 8, 0},
		.num_planes	= 2,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 1,
	}, {
		.description	= "YUV4:2:2 3-Plane 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YUV422P,
		.regcfg		= SMFC_IMGFMT_YUV422,
		.bpp_buf	= {16},
		.bpp_pix	= {8, 2, 2},
		.num_planes	= 3,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 1,
	}, {
		.description	= "YUV4:2:0 2-Plane(Cb/Cr) 12BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_NV12,
		.regcfg		= SMFC_IMGFMT_NV12,
		.bpp_buf	= {12},
		.bpp_pix	= {8, 4, 0},
		.num_planes	= 2,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YUV4:2:0 2-Plane(Cr/Cb) 12BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_NV21,
		.regcfg		= SMFC_IMGFMT_NV21,
		.bpp_buf	= {12},
		.bpp_pix	= {8, 4, 0},
		.num_planes	= 2,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YUV4:2:0 3-Plane 12BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YUV420,
		.regcfg		= SMFC_IMGFMT_YUV420P,
		.bpp_buf	= {12},
		.bpp_pix	= {8, 2, 2},
		.num_planes	= 3,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YVU4:2:0 3-Plane 12BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YVU420,
		.regcfg		= SMFC_IMGFMT_YVU420P,
		.bpp_buf	= {12},
		.bpp_pix	= {8, 2, 2},
		.num_planes	= 3,
		.num_buffers	= 1,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YUV4:4:4 3-Plane 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YUV444,
		.regcfg		= SMFC_IMGFMT_YUV444,
		.bpp_buf	= {24},
		.bpp_pix	= {8, 8, 8},
		.num_planes	= 3,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	}, {
		.description	= "RGB565 LE 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_RGB565,
		.regcfg		= SMFC_IMGFMT_RGB565,
		.bpp_buf	= {16},
		.bpp_pix	= {16},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	}, {
		.description	= "RGB565 BE 16BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_RGB565X,
		.regcfg		= SMFC_IMGFMT_BGR565,
		.bpp_buf	= {16},
		.bpp_pix	= {16},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	}, {
		.description	= "RGB888 LE 24BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_RGB24,
		.regcfg		= SMFC_IMGFMT_RGB24,
		.bpp_buf	= {24},
		.bpp_pix	= {24},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	}, {
		.description	= "RGB888 BE 24BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_BGR24,
		.regcfg		= SMFC_IMGFMT_BGR24,
		.bpp_buf	= {24},
		.bpp_pix	= {24},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	}, {
		.description	= "ABGR8888 LE 32BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_RGB32,
		.regcfg		= SMFC_IMGFMT_RGBA32,
		.bpp_buf	= {32},
		.bpp_pix	= {32},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	}, {
		.description	= "ARGB8888 BE 32BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_BGR32,
		.regcfg		= SMFC_IMGFMT_BGRA32,
		.bpp_buf	= {32},
		.bpp_pix	= {32},
		.num_planes	= 1,
		.num_buffers	= 1,
		.chroma_hfactor	= 1,
		.chroma_vfactor	= 1,
	},
	/* multi-planar formats must be at the last becuse of VIDOC_ENUM_FMT */
	{
		.description	= "YUV4:2:0 2-MPlane(Cb/Cr) 8+4BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_NV12M,
		.regcfg		= SMFC_IMGFMT_NV12,
		.bpp_buf	= {8, 4},
		.bpp_pix	= {8, 4},
		.num_planes	= 2,
		.num_buffers	= 2,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YUV4:2:0 2-MPlane(Cr/Cb) 8+4BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_NV21M,
		.regcfg		= SMFC_IMGFMT_NV21,
		.bpp_buf	= {8, 4},
		.bpp_pix	= {8, 4},
		.num_planes	= 2,
		.num_buffers	= 2,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YUV4:2:0 3-MPlane 8+2+2BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YUV420M,
		.regcfg		= SMFC_IMGFMT_YUV420P,
		.bpp_buf	= {8, 2, 2},
		.bpp_pix	= {8, 2, 2},
		.num_planes	= 3,
		.num_buffers	= 3,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}, {
		.description	= "YVU4:2:0 3-MPlane 8+2+2BPP",
		.v4l2_pixfmt	= V4L2_PIX_FMT_YVU420M,
		.regcfg		= SMFC_IMGFMT_YVU420P,
		.bpp_buf	= {8, 2, 2},
		.bpp_pix	= {8, 2, 2},
		.num_planes	= 3,
		.num_buffers	= 3,
		.chroma_hfactor	= 2,
		.chroma_vfactor	= 2,
	}
};

static const struct smfc_image_format *smfc_find_format(
				struct smfc_dev *smfc, __u32 v4l2_pixfmt)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(smfc_image_formats); i++)
		if (smfc_image_formats[i].v4l2_pixfmt == v4l2_pixfmt)
			return &smfc_image_formats[i];

	dev_warn(smfc->dev, "Pixel format '%08X' not found, YUYV is forced.\n",
		v4l2_pixfmt);

	return V4L2_TYPE_IS_OUTPUT(v4l2_pixfmt) ? SMFC_DEFAULT_OUTPUT_FORMAT
						: SMFC_DEFAULT_CAPTURE_FORMAT;
}

static const char *buf_type_name(__u32 type)
{
	return V4L2_TYPE_IS_OUTPUT(type) ? "capture" : "output";
}

static int smfc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct smfc_ctx *ctx = container_of(ctrl->handler,
					    struct smfc_ctx, v4l2_ctrlhdlr);
	switch (ctrl->id) {
	case V4L2_CID_JPEG_COMPRESSION_QUALITY:
		ctx->quality_factor = (unsigned char)ctrl->val;
		break;
	case V4L2_CID_JPEG_SEC_COMP_QUALITY:
		ctx->thumb_quality_factor = (unsigned char)ctrl->val;
		break;
	case V4L2_CID_JPEG_HWFC_ENABLE:
		if (!smfc_is_capable(ctx->smfc,
				V4L2_CAP_EXYNOS_JPEG_HWFC)) {
			dev_err(ctx->smfc->dev,
				"HWFC(OTF mode) not supported (ver.%08X)",
				ctx->smfc->hwver);
			return -EINVAL;
		}
		ctx->enable_hwfc = (unsigned char)ctrl->val;
		break;
	case V4L2_CID_JPEG_RESTART_INTERVAL:
		ctx->restart_interval = (unsigned char)ctrl->val;
		break;
	case V4L2_CID_JPEG_CHROMA_SUBSAMPLING:
		switch (ctrl->val) {
		case V4L2_JPEG_CHROMA_SUBSAMPLING_444:
			ctx->chroma_hfactor = 1;
			ctx->chroma_vfactor = 1;
			break;
		case V4L2_JPEG_CHROMA_SUBSAMPLING_420:
			ctx->chroma_hfactor = 2;
			ctx->chroma_vfactor = 2;
			break;
		case V4L2_JPEG_CHROMA_SUBSAMPLING_411:
			ctx->chroma_hfactor = 4;
			ctx->chroma_vfactor = 1;
			break;
		case V4L2_JPEG_CHROMA_SUBSAMPLING_GRAY:
			ctx->chroma_hfactor = 0;
			ctx->chroma_vfactor = 0;
			break;
		case V4L2_JPEG_CHROMA_SUBSAMPLING_410:
			dev_info(ctx->smfc->dev,
				"Compression to YUV410 is not supported\n");
			/* pass through to 422 */
		case V4L2_JPEG_CHROMA_SUBSAMPLING_422:
		default:
			ctx->chroma_hfactor = 2;
			ctx->chroma_vfactor = 1;
			break;
		}
		break;
	case V4L2_CID_JPEG_QTABLES2:
		/*
		 * reset the quality factor to indicate that the custom q-table
		 * is configured. It is sustained until the new quality factor
		 * is configured.
		 */
		ctx->quality_factor = 0;
		break;
	default:
		dev_err(ctx->smfc->dev, "Unsupported CID %#x\n", ctrl->id);
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops smfc_ctrl_ops = {
	.s_ctrl = smfc_s_ctrl,
};

int smfc_init_controls(struct smfc_dev *smfc,
			struct v4l2_ctrl_handler *hdlr)
{
	struct smfc_ctx *ctx =
			container_of(hdlr, struct smfc_ctx, v4l2_ctrlhdlr);
	const char *msg;
	struct v4l2_ctrl_config ctrlcfg;

	v4l2_ctrl_handler_init(hdlr, 1);

	if (!v4l2_ctrl_new_std(hdlr, &smfc_ctrl_ops,
				V4L2_CID_JPEG_COMPRESSION_QUALITY,
				1, 100, 1, 96)) {
		msg = "quality factor";
		goto err;
	}

	if (!v4l2_ctrl_new_std(hdlr, &smfc_ctrl_ops,
				V4L2_CID_JPEG_RESTART_INTERVAL,
				0, 64, 1, 0)) {
		msg = "restart interval";
		goto err;
	}

	if (!v4l2_ctrl_new_std_menu(hdlr, &smfc_ctrl_ops,
				V4L2_CID_JPEG_CHROMA_SUBSAMPLING,
				V4L2_JPEG_CHROMA_SUBSAMPLING_GRAY, 0,
				V4L2_JPEG_CHROMA_SUBSAMPLING_422)) {
		msg = "chroma subsampling";
		goto err;
	}

	memset(&ctrlcfg, 0, sizeof(ctrlcfg));
	ctrlcfg.ops = &smfc_ctrl_ops;
	ctrlcfg.id = V4L2_CID_JPEG_SEC_COMP_QUALITY;
	ctrlcfg.name = "Quality factor of secondray image";
	ctrlcfg.type = V4L2_CTRL_TYPE_INTEGER;
	ctrlcfg.min = 1;
	ctrlcfg.max = 100;
	ctrlcfg.step = 1;
	ctrlcfg.def = 50;
	if (!v4l2_ctrl_new_custom(hdlr, &ctrlcfg, NULL)) {
		msg = "secondary quality factor";
		goto err;
	}

	memset(&ctrlcfg, 0, sizeof(ctrlcfg));
	ctrlcfg.ops = &smfc_ctrl_ops;
	ctrlcfg.id = V4L2_CID_JPEG_HWFC_ENABLE;
	ctrlcfg.name = "HWFC configuration";
	ctrlcfg.type = V4L2_CTRL_TYPE_BOOLEAN;
	ctrlcfg.min = 0;
	ctrlcfg.max = 1;
	ctrlcfg.step = 1;
	ctrlcfg.def = 0;
	if (!v4l2_ctrl_new_custom(hdlr, &ctrlcfg, NULL)) {
		msg = "secondary quality factor";
		goto err;
	}

	memset(&ctrlcfg, 0, sizeof(ctrlcfg));
	ctrlcfg.ops = &smfc_ctrl_ops;
	ctrlcfg.id = V4L2_CID_JPEG_QTABLES2;
	ctrlcfg.name = "Quantization table configration";
	ctrlcfg.type = V4L2_CTRL_TYPE_U8;
	ctrlcfg.min = 1;
	ctrlcfg.max = 255;
	ctrlcfg.step = 1;
	ctrlcfg.def = 1;
	ctrlcfg.dims[0] = SMFC_MCU_SIZE; /* A qtable has 64 values */
	ctrlcfg.dims[1] = 2; /* 2 qtables are required for 0:luma, 1:chroma */
	ctx->ctrl_qtbl2 = v4l2_ctrl_new_custom(hdlr, &ctrlcfg, NULL);
	if (!ctx->ctrl_qtbl2) {
		msg = "Q-Table";
		goto err;
	}

	return 0;
err:
	v4l2_ctrl_handler_free(hdlr);

	dev_err(smfc->dev, "Failed to install %s control (%d)\n",
		msg, hdlr->error);
	return hdlr->error;
}

static int smfc_v4l2_querycap(struct file *filp, void *fh,
			     struct v4l2_capability *cap)
{
	struct smfc_dev *smfc = v4l2_fh_to_smfc_ctx(fh)->smfc;

	strncpy(cap->driver, MODULE_NAME, sizeof(cap->driver));
	strncpy(cap->bus_info, dev_name(smfc->dev), sizeof(cap->bus_info));
	scnprintf(cap->card, sizeof(cap->card), "Still MFC %02x.%02x.%04x",
			(smfc->hwver >> 24) & 0xFF, (smfc->hwver >> 16) & 0xFF,
			smfc->hwver & 0xFFFF);

	cap->driver[sizeof(cap->driver) - 1] = '\0';
	cap->card[sizeof(cap->card) - 1] = '\0';
	cap->bus_info[sizeof(cap->bus_info) - 1] = '\0';

	cap->capabilities = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_M2M_MPLANE
				| V4L2_CAP_VIDEO_M2M;
	cap->capabilities |= V4L2_CAP_DEVICE_CAPS;

	cap->device_caps = smfc->devdata->device_caps;
	cap->device_caps |= V4L2_CAP_EXYNOS_JPEG_DMABUF_OFFSET;

	return 0;
}

static int smfc_v4l2_enum_fmt_mplane(struct file *filp, void *fh,
				     struct v4l2_fmtdesc *f)
{
	const struct smfc_image_format *fmt;

	if (f->index >= ARRAY_SIZE(smfc_image_formats))
		return -EINVAL;

	fmt = &smfc_image_formats[f->index];
	strncpy(f->description, fmt->description, sizeof(f->description));
	f->description[sizeof(f->description) - 1] = '\0';
	f->pixelformat = fmt->v4l2_pixfmt;
	if (fmt->bpp_buf[0] == 0)
		f->flags = V4L2_FMT_FLAG_COMPRESSED;

	return 0;
}

static int smfc_v4l2_enum_fmt(struct file *filp, void *fh,
			      struct v4l2_fmtdesc *f)
{
	int ret = smfc_v4l2_enum_fmt_mplane(filp, fh, f);

	if (ret < 0)
		return ret;

	return smfc_image_formats[f->index].num_buffers > 1 ? -EINVAL : 0;
}

static int smfc_v4l2_g_fmt_mplane(struct file *filp, void *fh,
				  struct v4l2_format *f)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);

	f->fmt.pix_mp.width = ctx->width;
	f->fmt.pix_mp.height = ctx->height;

	if (!smfc_is_compressed_type(ctx, f->type)) {
		int i;
		/* uncompressed image */
		f->fmt.pix_mp.pixelformat = ctx->img_fmt->v4l2_pixfmt;
		f->fmt.pix_mp.num_planes = ctx->img_fmt->num_buffers;
		for (i = 0; i < ctx->img_fmt->num_buffers; i++) {
			f->fmt.pix_mp.plane_fmt[i].bytesperline =
				(f->fmt.pix_mp.width *
					 ctx->img_fmt->bpp_buf[i]) / 8;
			f->fmt.pix_mp.plane_fmt[i].sizeimage =
					f->fmt.pix_mp.plane_fmt[i].bytesperline;
			f->fmt.pix_mp.plane_fmt[i].sizeimage *=
							f->fmt.pix_mp.height;
		}
	} else {
		f->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_JPEG;
		f->fmt.pix_mp.num_planes = 1;
		f->fmt.pix_mp.plane_fmt[0].bytesperline = 0;
		f->fmt.pix_mp.plane_fmt[0].sizeimage = 0;
	}

	f->fmt.pix_mp.field = 0;
	f->fmt.pix_mp.colorspace = 0;

	return 0;
}

static int smfc_v4l2_g_fmt(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);

	f->fmt.pix.width = ctx->width;
	f->fmt.pix.height = ctx->height;

	if (!smfc_is_compressed_type(ctx, f->type)) {
		if (ctx->img_fmt->num_buffers > 1) {
			dev_err(ctx->smfc->dev,
				"Current format is a multi-planar format\n");
			return -EINVAL;
		}

		/* uncompressed image */
		f->fmt.pix.pixelformat = ctx->img_fmt->v4l2_pixfmt;
		f->fmt.pix.bytesperline =
			(f->fmt.pix.width * ctx->img_fmt->bpp_buf[0]) / 8;
		f->fmt.pix.sizeimage = f->fmt.pix.bytesperline;
		f->fmt.pix.sizeimage *= f->fmt.pix.height;
	} else {
		f->fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
		f->fmt.pix.bytesperline = 0;
		f->fmt.pix.sizeimage = 0;
	}

	f->fmt.pix.field = 0;
	f->fmt.pix.colorspace = 0;

	return 0;
}

static bool __smfc_check_image_size(struct device *dev, __u32 type,
				  const struct smfc_image_format *smfc_fmt,
				  __u32 width, __u32 height)
{
	__u32 min_width = SMFC_MIN_WIDTH << smfc_fmt->chroma_hfactor;
	__u32 min_height = SMFC_MIN_WIDTH << smfc_fmt->chroma_vfactor;

	if ((width < min_width) || (height < min_height)) {
		dev_warn(dev, "Too small image size(%ux%u) for '%s'\n",
				width, height, buf_type_name(type));
		return false;
	}

	if ((width > SMFC_MAX_WIDTH) || (height > SMFC_MAX_HEIGHT)) {
		dev_warn(dev, "Too large image size(%ux%u) for '%s'\n",
				width, height, buf_type_name(type));
		return false;
	}

	if (((width % smfc_fmt->chroma_hfactor) != 0) ||
		((height % smfc_fmt->chroma_vfactor) != 0)) {
		dev_err(dev, "Invalid size %ux%u for format '%s'\n",
			width, height, smfc_fmt->description);
		return false;
	}

	return true;
}
static bool smfc_check_image_size(struct device *dev, __u32 type,
				  const struct smfc_image_format *smfc_fmt,
				  __u32 width, __u32 height)
{
	__u32 twidth = SMFC_FMT_SEC_SIZE(width);
	__u32 theight = SMFC_FMT_SEC_SIZE(height);

	width = SMFC_FMT_MAIN_SIZE(width);
	height = SMFC_FMT_MAIN_SIZE(height);

	if (!__smfc_check_image_size(dev, type, smfc_fmt, width, height))
		return false;

	if (!twidth || !theight) /* thumbnail image size is not specified */
		return true;

	if (!__smfc_check_image_size(dev, type, smfc_fmt, twidth, theight))
		return false;

	return true;
}

static bool smfc_check_capable_of_decompression(const struct smfc_dev *smfc,
		const struct smfc_image_format *smfc_fmt, __u32 type)
{
	if (smfc_is_capable(smfc, V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION))
		return true;

	if (is_jpeg(smfc_fmt) != V4L2_TYPE_IS_OUTPUT(type)) /* compression? */
		return true;

	dev_err(smfc->dev, "Decompression is not supported (ver.%08X)",
		smfc->hwver);

	return false;
}

static int smfc_calc_crop(struct smfc_ctx *ctx, const struct v4l2_crop *cr)
{
	u32 left = cr->c.left;
	u32 top = cr->c.top;
	u32 frame_width = ctx->width;
	u32 crop_width = cr->c.width;
	unsigned char bpp_pix = ctx->img_fmt->bpp_pix[0]/8;
	unsigned char chroma_hfactor = ctx->img_fmt->chroma_hfactor;
	unsigned char chroma_vfactor = ctx->img_fmt->chroma_vfactor;
	unsigned int i;

	ctx->crop.po[0] = (frame_width * top + left) * bpp_pix;
	ctx->crop.so[0] = (frame_width - crop_width) * bpp_pix;

	for(i = 1; i < ctx->img_fmt->num_planes; i++) {
		ctx->crop.po[i] = (frame_width * top / chroma_vfactor + left )
			* bpp_pix * 2 / chroma_hfactor / i;
		ctx->crop.so[i] = (frame_width - crop_width) * bpp_pix
			* 2 / chroma_hfactor / i;

		if(i == 2) {
			ctx->crop.po[i-1] = ctx->crop.po[i];
			ctx->crop.so[i-1] = ctx->crop.so[i];
		}
	}

	return 0;
}

static bool smfc_v4l2_init_fmt_mplane(const struct smfc_ctx *ctx,
			const struct smfc_image_format *smfc_fmt,
			__u32 type, struct v4l2_pix_format_mplane *pix_mp)
{
	unsigned int i;

	if (!smfc_check_capable_of_decompression(ctx->smfc, smfc_fmt, type))
		return false;

	if (!smfc_check_image_size(ctx->smfc->dev, type,
				smfc_fmt, pix_mp->width, pix_mp->height))
		return false;
	/* informs the user that the format might be changed to the default */
	pix_mp->pixelformat = smfc_fmt->v4l2_pixfmt;
	/* JPEG format has zero in smfc_fmt->bpp_buf[0] */
	for (i = 0; i < smfc_fmt->num_buffers; i++) {
		pix_mp->plane_fmt[i].bytesperline =
				(SMFC_FMT_MAIN_SIZE(pix_mp->width) *
				 smfc_fmt->bpp_buf[i]) / 8;
		pix_mp->plane_fmt[i].sizeimage =
				pix_mp->plane_fmt[i].bytesperline *
					SMFC_FMT_MAIN_SIZE(pix_mp->height);
	}

	pix_mp->field = 0;
	pix_mp->num_planes = smfc_fmt->num_buffers;

	if (SMFC_FMT_SEC_SIZE(pix_mp->width) &&
				SMFC_FMT_SEC_SIZE(pix_mp->height)) {
		/*
		 * Format information for the secondary image is stored after
		 * the last plane of the main image. It is okay to use more
		 * planes because the maximum valid planes are eight
		 */
		unsigned int j;

		for (j = 0; j < smfc_fmt->num_buffers; j++) {
			pix_mp->plane_fmt[i + j].bytesperline =
				(SMFC_FMT_SEC_SIZE(pix_mp->width) *
					smfc_fmt->bpp_buf[j]) / 8;
			pix_mp->plane_fmt[i + j].sizeimage =
				pix_mp->plane_fmt[i + j].bytesperline *
					SMFC_FMT_SEC_SIZE(pix_mp->height);
		}
	}

	return true;
}

static int smfc_v4l2_try_fmt_mplane(struct file *filp, void *fh,
				  struct v4l2_format *f)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);
	const struct smfc_image_format *smfc_fmt =
			smfc_find_format(ctx->smfc, f->fmt.pix_mp.pixelformat);

	return smfc_v4l2_init_fmt_mplane(
			ctx, smfc_fmt, f->type, &f->fmt.pix_mp) ? 0 : -EINVAL;
}

static int smfc_v4l2_prepare_s_fmt(struct smfc_ctx *ctx,
				   const struct smfc_image_format *smfc_fmt,
				   __u32 type)
{
	struct vb2_queue *thisvq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, type);
	struct vb2_queue *othervq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx,
			V4L2_TYPE_IS_OUTPUT(type) ?
				V4L2_BUF_TYPE_VIDEO_CAPTURE :
				V4L2_BUF_TYPE_VIDEO_OUTPUT);
	u32 flags;

	if (thisvq->num_buffers > 0) {
		dev_err(ctx->smfc->dev,
			"S_FMT after REQBUFS is not allowed\n");
		return -EBUSY;
	}

	flags = smfc_config_ctxflag(ctx, SMFC_CTX_COMPRESS,
			is_jpeg(smfc_fmt) != V4L2_TYPE_IS_OUTPUT(type));

	if (othervq->num_buffers > 0) { /* REQBUFSed on other vq */
		if ((flags & SMFC_CTX_COMPRESS) !=
					(ctx->flags & SMFC_CTX_COMPRESS)) {
			dev_err(ctx->smfc->dev,
				"Changing mode is prohibited after reqbufs\n");
			ctx->flags = flags;
			return -EBUSY;
		}
	}

	/* reset the buf type of vq with the given buf type */
	thisvq->type = type;

	return 0;
}

static __u32 v4l2_to_multiplane_type(__u32 type)
{
	/* V4L2_BUF_TYPE_VIDEO_CAPTURE+8 = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE */
	return V4L2_TYPE_IS_MULTIPLANAR(type) ? type : type + 8;
}

static int smfc_v4l2_s_fmt_mplane(struct file *filp, void *fh,
				    struct v4l2_format *f)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);
	const struct smfc_image_format *smfc_fmt =
			smfc_find_format(ctx->smfc, f->fmt.pix_mp.pixelformat);
	int ret = smfc_v4l2_prepare_s_fmt(ctx, smfc_fmt, f->type);

	if (ret)
		return ret;

	if (!smfc_v4l2_init_fmt_mplane(ctx, smfc_fmt, f->type, &f->fmt.pix_mp))
		return -EINVAL;

	if(!is_jpeg(smfc_fmt)) {
		ctx->width = SMFC_FMT_MAIN_SIZE(f->fmt.pix_mp.width);
		ctx->height = SMFC_FMT_MAIN_SIZE(f->fmt.pix_mp.height);
		ctx->thumb_width = SMFC_FMT_SEC_SIZE(f->fmt.pix_mp.width);
		ctx->thumb_height = SMFC_FMT_SEC_SIZE(f->fmt.pix_mp.height);

		memset(&ctx->crop, 0, sizeof(ctx->crop));

		ctx->crop.width = ctx->width;
		ctx->crop.height = ctx->height;
	}

	if (ctx->thumb_width && ctx->thumb_height) {
		if (!smfc_is_capable(ctx->smfc,
				V4L2_CAP_EXYNOS_JPEG_B2B_COMPRESSION)) {
			dev_err(ctx->smfc->dev,
				"back-to-back mode not supported (ver.%08X)",
				ctx->smfc->hwver);
			return -EINVAL;
		}
		ctx->flags |= SMFC_CTX_B2B_COMPRESS;
	} else {
		ctx->flags &= ~SMFC_CTX_B2B_COMPRESS;
	}

	if (f->fmt.pix_mp.pixelformat != V4L2_PIX_FMT_JPEG)
		ctx->img_fmt = smfc_fmt;

	if (ctx->flags & SMFC_CTX_B2B_COMPRESS) {
		struct vb2_queue *othervq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx,
			V4L2_TYPE_IS_OUTPUT(f->type) ?
				V4L2_BUF_TYPE_VIDEO_CAPTURE :
				V4L2_BUF_TYPE_VIDEO_OUTPUT);
		/*
		 * type change of the current queue is completed in
		 * smfc_v4l2_prepare_s_fmt()
		 */
		othervq->type = v4l2_to_multiplane_type(othervq->type);
	}

	return 0;
}

static bool smfc_v4l2_init_fmt(const struct smfc_ctx *ctx,
				const struct smfc_image_format *smfc_fmt,
				__u32 type, struct v4l2_pix_format *pix)
{
	if (V4L2_TYPE_IS_MULTIPLANAR(pix->pixelformat)) {
		dev_err(ctx->smfc->dev,
			"Multi-planar format is not permitted.\n");
		return false;
	}

	if (!smfc_check_capable_of_decompression(ctx->smfc, smfc_fmt, type))
		return false;

	if (!smfc_check_image_size(ctx->smfc->dev, type,
				smfc_fmt, pix->width, pix->height))
		return false;
	/* informs the user that the format might be changed to the default */
	pix->pixelformat = smfc_fmt->v4l2_pixfmt;
	/* JPEG format has zero in smfc_fmt->bpp_buf[0] */
	pix->bytesperline = (pix->width *  smfc_fmt->bpp_buf[0]) / 8;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->field = 0;

	return true;
}

static int smfc_v4l2_try_fmt(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);
	const struct smfc_image_format *smfc_fmt =
			smfc_find_format(ctx->smfc, f->fmt.pix.pixelformat);

	return smfc_v4l2_init_fmt(ctx, smfc_fmt, f->type, &f->fmt.pix)
								? 0 : -EINVAL;
}

static int smfc_v4l2_s_fmt(struct file *filp, void *fh, struct v4l2_format *f)
{
	struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);
	const struct smfc_image_format *smfc_fmt =
			smfc_find_format(ctx->smfc, f->fmt.pix.pixelformat);
	int ret = smfc_v4l2_prepare_s_fmt(ctx, smfc_fmt, f->type);

	if (ret)
		return ret;

	if (!smfc_v4l2_init_fmt(ctx, smfc_fmt, f->type, &f->fmt.pix))
		return -EINVAL;

	if(!is_jpeg(smfc_fmt)) {
		ctx->width = SMFC_FMT_MAIN_SIZE(f->fmt.pix.width);
		ctx->height = SMFC_FMT_MAIN_SIZE(f->fmt.pix.height);
		/* back-to-back compression is not supported with single plane */
		ctx->thumb_width = 0;
		ctx->thumb_height = 0;

		memset(&ctx->crop, 0, sizeof(ctx->crop));

		ctx->crop.width = ctx->width;
		ctx->crop.height = ctx->height;
	}

	if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_JPEG)
		ctx->img_fmt = smfc_fmt;

	return 0;
}

static int smfc_v4l2_cropcap(struct file *file, void *fh,
                            struct v4l2_cropcap *cr)
{
        cr->bounds.left         = 0;
        cr->bounds.top          = 0;
        cr->bounds.width        = SMFC_MAX_WIDTH;
        cr->bounds.height       = SMFC_MAX_HEIGHT;
        cr->defrect             = cr->bounds;

        return 0;
}

static int smfc_v4l2_s_crop(struct file *file, void *fh,
                const struct v4l2_crop *cr)
{
        struct smfc_ctx *ctx = v4l2_fh_to_smfc_ctx(fh);
        unsigned int i;
        int ret = 0;

        if ((cr->c.left < 0) || (cr->c.top < 0) ||
				(cr->c.width < 16) || (cr->c.height < 16)) {
		v4l2_err(&ctx->smfc->v4l2_dev,
				"Invalid crop region (%d,%d):%dx%d\n",
				cr->c.left, cr->c.top, cr->c.width, cr->c.height);

                return -EINVAL;
        }

        if (((cr->c.left + cr->c.width) > ctx->width) ||
				((cr->c.top + cr->c.height) > ctx->height)) {
		v4l2_err(&ctx->smfc->v4l2_dev,
			"Crop (%d,%d):%dx%d overflows the image %dx%d\n",
			cr->c.left, cr->c.top, cr->c.width, cr->c.height,
			ctx->width, ctx->height);
                return -EINVAL;
        }

	if(!V4L2_TYPE_IS_OUTPUT(cr->type)) {
		v4l2_err(&ctx->smfc->v4l2_dev,
			"Cropping on the capture buffer is not supported\n");
		return -EINVAL;
	}

        ctx->crop.width = cr->c.width;
        ctx->crop.height = cr->c.height;

        for (i = 0 ; i < SMFC_MAX_NUM_COMP; i++) {
                ctx->crop.po[i] = 0;
                ctx->crop.so[i] = 0;
        }

        if (!!smfc_calc_crop(ctx, cr))
                return -EINVAL;

        return ret;
}

const struct v4l2_ioctl_ops smfc_v4l2_ioctl_ops = {
	.vidioc_querycap		= smfc_v4l2_querycap,
	.vidioc_enum_fmt_vid_cap	= smfc_v4l2_enum_fmt,
	.vidioc_enum_fmt_vid_out	= smfc_v4l2_enum_fmt,
	.vidioc_enum_fmt_vid_cap_mplane	= smfc_v4l2_enum_fmt_mplane,
	.vidioc_enum_fmt_vid_out_mplane	= smfc_v4l2_enum_fmt_mplane,
	.vidioc_g_fmt_vid_cap		= smfc_v4l2_g_fmt,
	.vidioc_g_fmt_vid_out		= smfc_v4l2_g_fmt,
	.vidioc_g_fmt_vid_cap_mplane	= smfc_v4l2_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= smfc_v4l2_g_fmt_mplane,
	.vidioc_try_fmt_vid_cap		= smfc_v4l2_try_fmt,
	.vidioc_try_fmt_vid_out		= smfc_v4l2_try_fmt,
	.vidioc_try_fmt_vid_cap_mplane	= smfc_v4l2_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= smfc_v4l2_try_fmt_mplane,
	.vidioc_s_fmt_vid_cap		= smfc_v4l2_s_fmt,
	.vidioc_s_fmt_vid_out		= smfc_v4l2_s_fmt,
	.vidioc_s_fmt_vid_cap_mplane	= smfc_v4l2_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= smfc_v4l2_s_fmt_mplane,
	.vidioc_cropcap			= smfc_v4l2_cropcap,
	.vidioc_s_crop			= smfc_v4l2_s_crop,

	.vidioc_reqbufs			= v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf		= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf			= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf			= v4l2_m2m_ioctl_dqbuf,

	.vidioc_streamon		= v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff		= v4l2_m2m_ioctl_streamoff,

	.vidioc_log_status		= v4l2_ctrl_log_status,
};
