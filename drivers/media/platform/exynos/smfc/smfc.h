/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * The main header file of Samsung Exynos SMFC Driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MEDIA_EXYNOS_SMFC_H_
#define _MEDIA_EXYNOS_SMFC_H_

#include <linux/ktime.h>
#include <linux/pm_qos.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ctrls.h>

#include "smfc-regs.h"

#define MODULE_NAME	"exynos-jpeg"

struct device;
struct video_device;

struct smfc_image_format {
	const char	*description;
	__u32		v4l2_pixfmt;
	u32		regcfg;
	unsigned char	bpp_buf[3];
	unsigned char	bpp_pix[3];
	unsigned char	num_planes;
	unsigned char	num_buffers;
	unsigned char	chroma_hfactor;
	unsigned char	chroma_vfactor;
};

extern const struct smfc_image_format smfc_image_formats[];

#define SMFC_DEFAULT_OUTPUT_FORMAT	(&smfc_image_formats[1])
#define SMFC_DEFAULT_CAPTURE_FORMAT	(&smfc_image_formats[0])

static inline bool is_jpeg(const struct smfc_image_format *fmt)
{
	return fmt->bpp_buf[0] == 0;
}

/* SMFC SPECIFIC DEVICE CAPABILITIES */
/* set if H/W supports for decompression */
#define V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION		0x0100
/* set if H/W can compress dual images */
#define V4L2_CAP_EXYNOS_JPEG_B2B_COMPRESSION		0x0200
/* set if H/W supports for Hardware Flow Control */
#define V4L2_CAP_EXYNOS_JPEG_HWFC			0x0400
/* set if H/W supports for HWFC on internal buffers */
#define V4L2_CAP_EXYNOS_JPEG_HWFC_EMBEDDED		0x0800
/* set if H/W has a register to configure stream buffer size */
#define V4L2_CAP_EXYNOS_JPEG_MAX_STREAMSIZE		0x1000
/* set if H/W does not have 128-bit alignment constraint for stream base */
#define V4L2_CAP_EXYNOS_JPEG_NO_STREAMBASE_ALIGN	0x2000
/* set if H/W does not have 128-bit alignment constraint for image base */
#define V4L2_CAP_EXYNOS_JPEG_NO_IMAGEBASE_ALIGN		0x4000
/*
 * Set if the driver requires the address of SOS marker for the start address
 * of the JPEG stream. Unset if the driver requires the address of SOI marker
 * for the start address of the JPEG stream even though H/W requires the address
 * of SOS marker to decompress when the driver is able to find the address of
 * SOS marker from the given address of SOI marker.
 */
#define V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION_FROM_SOS	0x10000
/* set if H/W supports for cropping during decompression */
#define V4L2_CAP_EXYNOS_JPEG_DECOMPRESSION_CROP		0x20000
/* set if H/W supports for downscaling(1/2, 1/4 and 1/8) during decompression */
#define V4L2_CAP_EXYNOS_JPEG_DOWNSCALING		0x40000
/* set if vb2 supports offset of dmabuf */
#define V4L2_CAP_EXYNOS_JPEG_DMABUF_OFFSET	0x80000

struct smfc_device_data {
	__u32 device_caps;
	unsigned char burstlenth_bits;
};

/* Set when H/W starts, cleared in irq/timeout handler */
#define SMFC_DEV_RUNNING	(1 << 0)
/* Set when suspend handler is called, cleared before irq handler returns. */
#define SMFC_DEV_SUSPENDING	(1 << 1)
/* Set when timeout handler is called, cleared before the handler returns. */
#define SMFC_DEV_TIMEDOUT	(1 << 3)
/* Set if HWFC is enabled in device_run, cleared in irq/timeout handler */
#define SMFC_DEV_OTF_EMUMODE	(1 << 4)

struct smfc_dev {
	struct v4l2_device v4l2_dev;
	struct video_device *videodev;
	struct v4l2_m2m_dev *m2mdev;
	struct device *dev;
	void __iomem *reg;
	const struct smfc_device_data *devdata;
	spinlock_t flag_lock;
	struct mutex video_device_mutex;
	struct timer_list timer;
	int device_id;
	u32 hwver;
	u32 flags;

	struct clk *clk_gate;
	struct clk *clk_gate2; /* available if clk_gate is valid */
	struct pm_qos_request qosreq_int;
	s32 qosreq_int_level;

};

#define SMFC_CTX_COMPRESS	(1 << 0)
#define SMFC_CTX_B2B_COMPRESS	(1 << 1) /* valid if SMFC_CTX_COMPRESS is set */

static inline bool smfc_is_capable(const struct smfc_dev *smfc, u32 capability)
{
	return (smfc->devdata->device_caps & capability) == capability;
}

struct smfc_decomp_htable {
	struct {
		union {
			u8 code[SMFC_NUM_HCODE];
			u32 code32[SMFC_NUM_HCODE / 4];
		};
		union {
			u8 value[SMFC_NUM_DC_HVAL];
			u32 value32[SMFC_NUM_DC_HVAL / 4];
		};
	} dc[2];
	struct {
		union {
			u8 code[SMFC_NUM_HCODE];
			u32 code32[SMFC_NUM_HCODE / 4];
		};
		union {
			u8 value[SMFC_NUM_AC_HVAL];
			u32 value32[SMFC_NUM_AC_HVAL / 4];
		};
	} ac[2];

	struct {
		unsigned char idx_dc;
		unsigned char idx_ac;
	} compsel[SMFC_MAX_NUM_COMP]; /* compsel[0] for component 1 */
};

#define INVALID_QTBLIDX 0xFF
struct smfc_decomp_qtable {
	/* quantizers are *NOT* stored in the zig-zag scan order */
	u8 table[SMFC_MAX_QTBL_COUNT][SMFC_MCU_SIZE];
	char compsel[SMFC_MAX_QTBL_COUNT];
};

struct smfc_crop {
	u32 width;
	u32 height;
	u32 po[SMFC_MAX_NUM_COMP];
	u32 so[SMFC_MAX_NUM_COMP];
};

struct smfc_ctx {
	struct v4l2_fh fh;
	struct v4l2_ctrl_handler v4l2_ctrlhdlr;
	struct smfc_dev *smfc;
	ktime_t ktime_beg;
	u32 flags;
	/* uncomressed image description */
	const struct smfc_image_format *img_fmt;
	__u32 width;
	__u32 height;

	/* cropping size settings */
	struct smfc_crop crop;

	/* Compression settings */
	unsigned char chroma_hfactor; /* horizontal chroma subsampling factor */
	unsigned char chroma_vfactor; /* vertical chroma subsampling factor */
	unsigned char restart_interval;
	unsigned char quality_factor;
	struct v4l2_ctrl *ctrl_qtbl2;
	/*
	 * thumbnail information:
	 * format of thumbnail should be the same as the main image
	 * It is not the H/W restriction. Just a choice for simpler S/W design.
	 */
	__u32 thumb_width;
	__u32 thumb_height;
	unsigned char thumb_quality_factor;
	unsigned char enable_hwfc;

	/* Decompression settings */
	struct smfc_decomp_qtable *quantizer_tables;
	struct smfc_decomp_htable *huffman_tables;
	unsigned char stream_hfactor;
	unsigned char stream_vfactor;
	unsigned char num_components;
	unsigned int offset_of_sos;
	__u16 stream_width;
	__u16 stream_height;
};

extern const struct v4l2_ioctl_ops smfc_v4l2_ioctl_ops;

static inline struct smfc_ctx *v4l2_fh_to_smfc_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct smfc_ctx, fh);
}

/* return the previous flag */
static inline u32 smfc_config_ctxflag(struct smfc_ctx *ctx, u32 flag, bool set)
{
	u32 prevflags = ctx->flags;
	ctx->flags = set ? ctx->flags | flag : ctx->flags & ~flag;
	return prevflags;
}

static inline bool smfc_is_compressed_type(struct smfc_ctx *ctx, __u32 type)
{
	return (!(ctx->flags & SMFC_CTX_COMPRESS)) == V4L2_TYPE_IS_OUTPUT(type);
}

int smfc_init_controls(struct smfc_dev *smfc, struct v4l2_ctrl_handler *hdlr);

int smfc_parse_jpeg_header(struct smfc_ctx *ctx, struct vb2_buffer *vb);

/* H/W Configuration */
void smfc_hwconfigure_tables(struct smfc_ctx *ctx,
			     unsigned int qfactor, const u8 qtbl[]);
void smfc_hwconfigure_tables_for_decompression(struct smfc_ctx *ctx);
void smfc_hwconfigure_image(struct smfc_ctx *ctx,
			    unsigned int hfactor, unsigned int vfactor);
void smfc_hwconfigure_start(struct smfc_ctx *ctx,
			    unsigned int rst_int, bool hwfc_en);
void smfc_hwconfigure_2nd_tables(struct smfc_ctx *ctx, unsigned int qfactor);
void smfc_hwconfigure_2nd_image(struct smfc_ctx *ctx, bool hwfc_enabled);
bool smfc_hwstatus_okay(struct smfc_dev *smfc, struct smfc_ctx *ctx);
void smfc_hwconfigure_reset(struct smfc_dev *smfc);
void smfc_dump_registers(struct smfc_dev *smfc);
static inline u32 smfc_get_streamsize(struct smfc_dev *smfc)
{
	return __raw_readl(smfc->reg + REG_MAIN_STREAM_SIZE);
}

static inline u32 smfc_get_2nd_streamsize(struct smfc_dev *smfc)
{
	return __raw_readl(smfc->reg + REG_SEC_STREAM_SIZE);
}

#endif /* _MEDIA_EXYNOS_SMFC_H_ */
