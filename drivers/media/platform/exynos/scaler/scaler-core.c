/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS Scaler driver
 *
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
#include <linux/sort.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>
#include <linux/smc.h>
#include <linux/ion_exynos.h>

#include <media/v4l2-ioctl.h>
#include <media/m2m1shot.h>
#include <media/m2m1shot-helper.h>
#include <media/videobuf2-dma-sg.h>

#include "scaler.h"
#include "scaler-regs.h"

/* Protection IDs of Scaler are 1 and 2. */
#define SC_SMC_PROTECTION_ID(instance)	(1 + (instance))

int sc_log_level;
module_param_named(sc_log_level, sc_log_level, uint, 0644);

int sc_set_blur;
module_param_named(sc_set_blur, sc_set_blur, uint, 0644);

int sc_show_stat;
module_param_named(sc_show_stat, sc_show_stat, uint, 0644);

#define BUF_EXT_SIZE	512
#define BUF_WIDTH_ALIGN	128

static inline unsigned int sc_ext_buf_size(int width)
{
	return IS_ALIGNED(width, BUF_WIDTH_ALIGN) ? 0 : BUF_EXT_SIZE;
}

/*
 * If true, writes the latency of H/W operation to v4l2_buffer.reserved2
 * in the unit of nano seconds.  It must not be enabled with real use-case
 * because v4l2_buffer.reserved may be used for other purpose.
 * The latency is written to the destination buffer.
 */
int __measure_hw_latency;
module_param_named(measure_hw_latency, __measure_hw_latency, int, 0644);

struct vb2_sc_buffer {
	struct v4l2_m2m_buffer mb;
	struct sc_ctx *ctx;
	ktime_t ktime;
};

static const struct sc_fmt sc_formats[] = {
	{
		.name		= "RGB565",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
		.cfg_val	= SCALER_CFG_FMT_RGB565,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "RGB1555",
		.pixelformat	= V4L2_PIX_FMT_RGB555X,
		.cfg_val	= SCALER_CFG_FMT_ARGB1555,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "ARGB4444",
		.pixelformat	= V4L2_PIX_FMT_RGB444,
		.cfg_val	= SCALER_CFG_FMT_ARGB4444,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {	/* swaps of ARGB32 in bytes in half word, half words in word */
		.name		= "RGBA8888",
		.pixelformat	= V4L2_PIX_FMT_RGB32,
		.cfg_val	= SCALER_CFG_FMT_RGBA8888 |
					SCALER_CFG_BYTE_HWORD_SWAP,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "BGRA8888",
		.pixelformat	= V4L2_PIX_FMT_BGR32,
		.cfg_val	= SCALER_CFG_FMT_ARGB8888,
		.bitperpixel	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.is_rgb		= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21,
		.cfg_val	= SCALER_CFG_FMT_YCRCB420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12M,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV21M,
		.cfg_val	= SCALER_CFG_FMT_YCRCB420_2P,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 2-planar, Y/CbCr, tiled",
		.pixelformat	= V4L2_PIX_FMT_NV12MT_16X16,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_TILE_EN,
		.bitperpixel	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 3-planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420,	/* I420 */
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YVU 4:2:0 contiguous 3-planar, Y/Cr/Cb",
		.pixelformat	= V4L2_PIX_FMT_YVU420,	/* YV12 */
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 non-contiguous 3-planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420M,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YVU 4:2:0 non-contiguous 3-planar, Y/Cr/Cb",
		.pixelformat	= V4L2_PIX_FMT_YVU420M,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, YCbYCr",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.cfg_val	= SCALER_CFG_FMT_YUYV,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, CbYCrY",
		.pixelformat	= V4L2_PIX_FMT_UYVY,
		.cfg_val	= SCALER_CFG_FMT_UYVY,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 packed, YCrYCb",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.cfg_val	= SCALER_CFG_FMT_YVYU,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV16,
		.cfg_val	= SCALER_CFG_FMT_YCBCR422_2P,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CrCb",
		.pixelformat	= V4L2_PIX_FMT_NV61,
		.cfg_val	= SCALER_CFG_FMT_YCRCB422_2P,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 3-planar, Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
		.cfg_val	= SCALER_CFG_FMT_YCBCR422_3P,
		.bitperpixel	= { 16 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous Y/CbCr",
		.pixelformat	= V4L2_PIX_FMT_NV12N,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous Y/CbCr 10-bit",
		.pixelformat	= V4L2_PIX_FMT_NV12N_10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 15 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 3-planar Y/Cb/Cr",
		.pixelformat	= V4L2_PIX_FMT_YUV420N,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_3P,
		.bitperpixel	= { 12 },
		.num_planes	= 1,
		.num_comp	= 3,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr 8+2 bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_S10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 10, 5 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous 2-planar, Y/CbCr 10-bit",
		.pixelformat	= V4L2_PIX_FMT_NV12M_P010,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_BYTE_SWAP |
					SCALER_CFG_10BIT_P010,
		.bitperpixel	= { 16, 8 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:0 contiguous, Y/CbCr 10-bit",
		.pixelformat	= V4L2_PIX_FMT_NV12_P010,
		.cfg_val	= SCALER_CFG_FMT_YCBCR420_2P |
					SCALER_CFG_BYTE_SWAP |
					SCALER_CFG_10BIT_P010,
		.bitperpixel	= { 24 },
		.num_planes	= 1,
		.num_comp	= 2,
		.h_shift	= 1,
		.v_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CbCr 8+2 bit",
		.pixelformat	= V4L2_PIX_FMT_NV16M_S10B,
		.cfg_val	= SCALER_CFG_FMT_YCBCR422_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 10, 10 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
	}, {
		.name		= "YUV 4:2:2 contiguous 2-planar, Y/CrCb 8+2 bit",
		.pixelformat	= V4L2_PIX_FMT_NV61M_S10B,
		.cfg_val	= SCALER_CFG_FMT_YCRCB422_2P |
					SCALER_CFG_10BIT_S10,
		.bitperpixel	= { 10, 10 },
		.num_planes	= 2,
		.num_comp	= 2,
		.h_shift	= 1,
	},
};

#define SCALE_RATIO_CONST(x, y) (u32)((1048576ULL * (x)) / (y))

#define SCALE_RATIO(x, y)						\
({									\
		u32 ratio;						\
		if (__builtin_constant_p(x) && __builtin_constant_p(y))	{\
			ratio = SCALE_RATIO_CONST(x, y);		\
		} else if ((x) < 2048) {				\
			ratio = (u32)((1048576UL * (x)) / (y));		\
		} else {						\
			unsigned long long dividend = 1048576ULL;	\
			dividend *= x;					\
			do_div(dividend, y);				\
			ratio = (u32)dividend;				\
		}							\
		ratio;							\
})

#define SCALE_RATIO_FRACT(x, y, z) (u32)(((x << 20) + SCALER_FRACT_VAL(y)) / z)

/* must specify in revers order of SCALER_VERSION(xyz) */
static const u32 sc_version_table[][2] = {
	{ 0x04000001, SCALER_VERSION(5, 1, 0) }, /* SC_POLY */
	{ 0x04000000, SCALER_VERSION(5, 1, 0) }, /* SC_POLY */
	{ 0x02000100, SCALER_VERSION(5, 0, 1) }, /* SC_POLY */
	{ 0x02000000, SCALER_VERSION(5, 0, 0) },
	{ 0x80060007, SCALER_VERSION(4, 2, 0) }, /* SC_BI */
	{ 0x0100000f, SCALER_VERSION(4, 0, 1) }, /* SC_POLY */
	{ 0xA0000013, SCALER_VERSION(4, 0, 1) },
	{ 0xA0000012, SCALER_VERSION(4, 0, 1) },
	{ 0x80050007, SCALER_VERSION(4, 0, 0) }, /* SC_POLY */
	{ 0xA000000B, SCALER_VERSION(3, 0, 2) },
	{ 0xA000000A, SCALER_VERSION(3, 0, 2) },
	{ 0x8000006D, SCALER_VERSION(3, 0, 1) },
	{ 0x80000068, SCALER_VERSION(3, 0, 0) },
	{ 0x8004000C, SCALER_VERSION(2, 2, 0) },
	{ 0x80000008, SCALER_VERSION(2, 1, 1) },
	{ 0x80000048, SCALER_VERSION(2, 1, 0) },
	{ 0x80010000, SCALER_VERSION(2, 0, 1) },
	{ 0x80000047, SCALER_VERSION(2, 0, 0) },
};

static const struct sc_variant sc_variant[] = {
	{
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 1, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
		.minsize_srcplane	= 4096 + 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
		.extra_buf		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(5, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 64),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
		.pixfmt_10bit		= 1,
		.extra_buf		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(4, 2, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 1,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(4, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_up_swmax		= SCALE_RATIO_CONST(1, 16),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 1,
		.initphase		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(4, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(3, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(16, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 1,
		.ratio_20bit		= 1,
		.initphase		= 1,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(2, 2, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 1,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.version		= SCALER_VERSION(2, 0, 1),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	}, {
		.limit_input = {
			.min_w		= 16,
			.min_h		= 16,
			.max_w		= 8192,
			.max_h		= 8192,
		},
		.limit_output = {
			.min_w		= 4,
			.min_h		= 4,
			.max_w		= 4096,
			.max_h		= 4096,
		},
		.version		= SCALER_VERSION(2, 0, 0),
		.sc_up_max		= SCALE_RATIO_CONST(1, 8),
		.sc_down_min		= SCALE_RATIO_CONST(4, 1),
		.sc_down_swmin		= SCALE_RATIO_CONST(16, 1),
		.blending		= 0,
		.prescale		= 0,
		.ratio_20bit		= 0,
		.initphase		= 0,
	},
};

#if defined(CONFIG_PM_DEVFREQ)
static void sc_pm_qos_update_devfreq(struct sc_qos_request *qos_req,
				int pm_qos_class, u32 freq)
{
	struct pm_qos_request *req;

	if (pm_qos_class == PM_QOS_BUS_THROUGHPUT)
		req = &qos_req->mif_req;
	else if (pm_qos_class == PM_QOS_DEVICE_THROUGHPUT)
		req = &qos_req->int_req;
	else
		return;

	if (!pm_qos_request_active(req))
		pm_qos_add_request(req, pm_qos_class, 0);

	pm_qos_update_request(req, freq);
}

static void sc_pm_qos_remove_devfreq(struct sc_qos_request *qos_req,
				int pm_qos_class)
{
	struct pm_qos_request *req;

	if (pm_qos_class == PM_QOS_BUS_THROUGHPUT)
		req = &qos_req->mif_req;
	else if (pm_qos_class == PM_QOS_DEVICE_THROUGHPUT)
		req = &qos_req->int_req;
	else
		return;

	if (pm_qos_request_active(req))
		pm_qos_remove_request(req);
}
#else
#define sc_pm_qos_update_devfreq(qos_req, pm_qos_class, freq) do { } while (0)
#define sc_pm_qos_remove_devfreq(qos_req, pm_qos_class) do { } while (0)
#endif

void sc_remove_devfreq(struct sc_qos_request *qos_req,
			struct sc_qos_table *qos_table)
{
	sc_pm_qos_remove_devfreq(qos_req, PM_QOS_BUS_THROUGHPUT);
	sc_pm_qos_remove_devfreq(qos_req, PM_QOS_DEVICE_THROUGHPUT);
}

void sc_request_devfreq(struct sc_qos_request *qos_req,
		struct sc_qos_table *qos_table, unsigned long lv)
{
	sc_pm_qos_update_devfreq(qos_req,
			PM_QOS_BUS_THROUGHPUT, qos_table[lv].freq_mif);
	sc_pm_qos_update_devfreq(qos_req,
			PM_QOS_DEVICE_THROUGHPUT, qos_table[lv].freq_int);
}

static bool sc_get_pm_qos_level_by_data_size(struct sc_ctx *ctx, int framerate)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *frame = &ctx->d_frame;
	struct sc_qos_table *qos_table = sc->qos_table;
	unsigned int dst_len = 0;
	int i;

	for (i = 0; i < frame->sc_fmt->num_planes; i++)
		dst_len += frame->bytesused[i];

	/*
	 * We don't need accurate size for level selection.
	 * It is divided by 256 to use data size within 32bit value.
	 * It should be done before calculating framerate.
	 */
	dst_len /= 256;
	dst_len *= framerate;

	for (i = 0; i < sc->qos_table_cnt; i++)
		if (qos_table[i].data_size < dst_len)
			break;

	/* Set the minimum level */
	if (i == sc->qos_table_cnt)
		i--;

	/* No request is required if level is same. */
	if (ctx->pm_qos_lv == i)
		return false;

	ctx->pm_qos_lv = i;

	return true;
}

/*
 * Required Clock (KHz) = (width * height) / (process time(ms) * ppc)
 */
static int sc_get_clock_khz(struct sc_ctx *ctx,
			struct sc_frame *frame, int process_time)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_ppc_table *ppc_table = sc->ppc_table;
	int ppc_idx = (ctx->flip_rot_cfg & SCALER_ROT_90) ? 1 : 0;
	int i, bpp = 0, ppc = 0;
	unsigned long clk;

	for (i = 0; i < frame->sc_fmt->num_planes; i++)
		bpp += frame->sc_fmt->bitperpixel[i];

	for (i = 0; i < sc->ppc_table_cnt; i++) {
		if (ppc_table[i].bpp == bpp) {
			ppc = ppc_table[i].ppc[ppc_idx];
			break;
		}
	}

	if (!ppc || !process_time)
		return 0;

	/*
	 * process_time is unit of microseconds,
	 * and ppc was already multiplied by 100 to eliminate the decimal point.
	 * So clk should be multiplied by 100000.
	 */
	clk = (unsigned long)(frame->crop.width * frame->crop.height) * 100000;
	clk /= (process_time * ppc);

	if (clk > INT_MAX)
		clk = INT_MAX;

	return (int)clk;
}

static bool sc_get_pm_qos_level_by_ppc(struct sc_ctx *ctx, int framerate)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_qos_table *qos_table = sc->qos_table;
	int time_us = 1000000 / framerate;
	int src_clk, dst_clk, target_clk;
	int i;

	src_clk = sc_get_clock_khz(ctx, &ctx->s_frame, time_us);
	dst_clk = sc_get_clock_khz(ctx, &ctx->d_frame, time_us);

	target_clk = max(src_clk, dst_clk);

	for (i = sc->qos_table_cnt - 1; i >= 0; i--) {
		if (qos_table[i].freq_int >= target_clk)
			break;
	}

	/* No request is required if level is same. */
	if (ctx->pm_qos_lv == i)
		return false;

	ctx->pm_qos_lv = i;

	return true;
}

static bool sc_get_pm_qos_level(struct sc_ctx *ctx, int framerate)
{
	struct sc_dev *sc = ctx->sc_dev;

	/* No need to calculate if no qos_table exists. */
	if (!sc->qos_table)
		return false;

	if (!sc->ppc_table)
		return sc_get_pm_qos_level_by_data_size(ctx, framerate);

	return sc_get_pm_qos_level_by_ppc(ctx, framerate);
}

/* Find the matches format */
static const struct sc_fmt *sc_find_format(struct sc_dev *sc,
						u32 pixfmt, bool output_buf)
{
	const struct sc_fmt *sc_fmt;
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(sc_formats); ++i) {
		sc_fmt = &sc_formats[i];
		if (sc_fmt->pixelformat == pixfmt) {
			if (!!(sc_fmt->cfg_val & SCALER_CFG_TILE_EN)) {
				/* tile mode is not supported from v3.0.0 */
				if (sc->version >= SCALER_VERSION(3, 0, 0))
					return NULL;
				if (!output_buf)
					return NULL;
			}
			/* bytes swap is not supported under v2.1.0 */
			if (!!(sc_fmt->cfg_val & SCALER_CFG_SWAP_MASK) &&
					(sc->version < SCALER_VERSION(2, 1, 0)))
				return NULL;
			return &sc_formats[i];
		}
	}

	return NULL;
}

static int sc_v4l2_querycap(struct file *file, void *fh,
			     struct v4l2_capability *cap)
{
	strncpy(cap->driver, MODULE_NAME, sizeof(cap->driver) - 1);
	strncpy(cap->card, MODULE_NAME, sizeof(cap->card) - 1);

	cap->capabilities = V4L2_CAP_STREAMING |
		V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE;
	cap->capabilities |= V4L2_CAP_DEVICE_CAPS;
	cap->device_caps = V4L2_CAP_FENCES;

	return 0;
}

static int sc_v4l2_enum_fmt_mplane(struct file *file, void *fh,
			     struct v4l2_fmtdesc *f)
{
	const struct sc_fmt *sc_fmt;

	if (f->index >= ARRAY_SIZE(sc_formats))
		return -EINVAL;

	sc_fmt = &sc_formats[f->index];
	strncpy(f->description, sc_fmt->name, sizeof(f->description) - 1);
	f->pixelformat = sc_fmt->pixelformat;

	return 0;
}

static int sc_v4l2_g_fmt_mplane(struct file *file, void *fh,
			  struct v4l2_format *f)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	const struct sc_fmt *sc_fmt;
	struct sc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	sc_fmt = frame->sc_fmt;

	pixm->width		= frame->width;
	pixm->height		= frame->height;
	pixm->pixelformat	= frame->pixelformat;
	pixm->field		= V4L2_FIELD_NONE;
	pixm->num_planes	= frame->sc_fmt->num_planes;
	pixm->colorspace	= 0;

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = (pixm->width *
				sc_fmt->bitperpixel[i]) >> 3;
		if (sc_fmt_is_ayv12(sc_fmt->pixelformat)) {
			unsigned int y_size, c_span;
			y_size = pixm->width * pixm->height;
			c_span = ALIGN(pixm->width >> 1, 16);
			pixm->plane_fmt[i].sizeimage =
				y_size + (c_span * pixm->height >> 1) * 2;
		} else {
			pixm->plane_fmt[i].sizeimage =
				pixm->plane_fmt[i].bytesperline * pixm->height;
		}

		v4l2_dbg(1, sc_log_level, &ctx->sc_dev->m2m.v4l2_dev,
				"[%d] plane: bytesperline %d, sizeimage %d\n",
				i, pixm->plane_fmt[i].bytesperline,
				pixm->plane_fmt[i].sizeimage);
	}

	return 0;
}

int sc_calc_s10b_planesize(u32 pixelformat, u32 width, u32 height,
				u32 *ysize, u32 *csize, bool only_8bit)
{
	int ret = 0;

	switch (pixelformat) {
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV12N_10B:
			*ysize = NV12M_Y_SIZE(width, height);
			*csize = NV12M_CBCR_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
			*ysize = NV16M_Y_SIZE(width, height);
			*csize = NV16M_CBCR_SIZE(width, height);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret || only_8bit)
		return ret;

	switch (pixelformat) {
	case V4L2_PIX_FMT_NV12M_S10B:
	case V4L2_PIX_FMT_NV12N_10B:
			*ysize += NV12M_Y_2B_SIZE(width, height);
			*csize += NV12M_CBCR_2B_SIZE(width, height);
		break;
	case V4L2_PIX_FMT_NV16M_S10B:
	case V4L2_PIX_FMT_NV61M_S10B:
			*ysize += NV16M_Y_2B_SIZE(width, height);
			*csize += NV16M_CBCR_2B_SIZE(width, height);
		break;
	}

	/* Do not consider extra size for 2bit CbCr */
	*csize -= 256;

	return ret;
}

static int sc_calc_fmt_s10b_size(const struct sc_fmt *sc_fmt,
		struct v4l2_pix_format_mplane *pixm, unsigned int ext_size)
{
	int i, ret;

	BUG_ON(sc_fmt->num_comp != 2);

	for (i = 0; i < pixm->num_planes; i++)
		pixm->plane_fmt[i].bytesperline = (pixm->width *
				sc_fmt->bitperpixel[i]) >> 3;

	ret = sc_calc_s10b_planesize(sc_fmt->pixelformat,
			pixm->width, pixm->height,
			&pixm->plane_fmt[0].sizeimage,
			&pixm->plane_fmt[1].sizeimage, false);
	if (ret) {
		pr_err("Scaler doesn't support %s format\n", sc_fmt->name);
		return ret;
	}

	for (i = 0; ext_size && i < pixm->num_planes; i++)
		pixm->plane_fmt[i].sizeimage += (i == 0) ? ext_size : ext_size/2;

	return ret;
}

static int sc_v4l2_try_fmt_mplane(struct file *file, void *fh,
			    struct v4l2_format *f)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	const struct sc_fmt *sc_fmt;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	const struct sc_size_limit *limit;
	int i;
	int h_align = 0;
	int w_align = 0;
	unsigned int ext_size = 0;

	if (!V4L2_TYPE_IS_MULTIPLANAR(f->type)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"not supported v4l2 type\n");
		return -EINVAL;
	}

	sc_fmt = sc_find_format(ctx->sc_dev, f->fmt.pix_mp.pixelformat, V4L2_TYPE_IS_OUTPUT(f->type));
	if (!sc_fmt) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"not supported format type\n");
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		limit = &ctx->sc_dev->variant->limit_input;
	else
		limit = &ctx->sc_dev->variant->limit_output;

	/*
	 * Y_SPAN - should even in interleaved YCbCr422
	 * C_SPAN - should even in YCbCr420 and YCbCr422
	 */
	w_align = sc_fmt->h_shift;
	h_align = sc_fmt->v_shift;

	/* Bound an image to have width and height in limit */
	v4l_bound_align_image(&pixm->width, limit->min_w, limit->max_w,
			w_align, &pixm->height, limit->min_h,
			limit->max_h, h_align, 0);

	pixm->num_planes = sc_fmt->num_planes;
	pixm->colorspace = 0;

	if (ctx->sc_dev->variant->extra_buf && V4L2_TYPE_IS_OUTPUT(f->type))
		ext_size = sc_ext_buf_size(pixm->width);

	if (sc_fmt_is_s10bit_yuv(sc_fmt->pixelformat))
		return sc_calc_fmt_s10b_size(sc_fmt, pixm, ext_size);

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = (pixm->width *
				sc_fmt->bitperpixel[i]) >> 3;
		if (sc_fmt_is_ayv12(sc_fmt->pixelformat)) {
			unsigned int y_size, c_span;
			y_size = pixm->width * pixm->height;
			c_span = ALIGN(pixm->width >> 1, 16);
			pixm->plane_fmt[i].sizeimage =
				y_size + (c_span * pixm->height >> 1) * 2;
		} else {
			pixm->plane_fmt[i].sizeimage =
				pixm->plane_fmt[i].bytesperline * pixm->height;
		}

		v4l2_dbg(1, sc_log_level, &ctx->sc_dev->m2m.v4l2_dev,
				"[%d] plane: bytesperline %d, sizeimage %d\n",
				i, pixm->plane_fmt[i].bytesperline,
				pixm->plane_fmt[i].sizeimage);
	}

	for (i = 0; ext_size && i < pixm->num_planes; i++)
		pixm->plane_fmt[i].sizeimage += (i == 0) ? ext_size : ext_size/2;

	return 0;
}

static int sc_v4l2_s_fmt_mplane(struct file *file, void *fh,
				 struct v4l2_format *f)

{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct vb2_queue *vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	struct sc_frame *frame;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	const struct sc_size_limit *limitout =
				&ctx->sc_dev->variant->limit_input;
	const struct sc_size_limit *limitcap =
				&ctx->sc_dev->variant->limit_output;
	int i, ret = 0;

	if (vb2_is_streaming(vq)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev, "device is busy\n");
		return -EBUSY;
	}

	ret = sc_v4l2_try_fmt_mplane(file, fh, f);
	if (ret < 0)
		return ret;

	frame = ctx_get_frame(ctx, f->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	set_bit(CTX_PARAMS, &ctx->flags);

	frame->sc_fmt = sc_find_format(ctx->sc_dev, f->fmt.pix_mp.pixelformat, V4L2_TYPE_IS_OUTPUT(f->type));
	if (!frame->sc_fmt) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"not supported format values\n");
		return -EINVAL;
	}

	for (i = 0; i < frame->sc_fmt->num_planes; i++)
		frame->bytesused[i] = pixm->plane_fmt[i].sizeimage;

	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width > limitout->max_w) ||
			 (pixm->height > limitout->max_h))) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"%dx%d of source image is not supported: too large\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width > limitcap->max_w) ||
			 (pixm->height > limitcap->max_h))) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"%dx%d of target image is not supported: too large\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width < limitout->min_w) ||
			 (pixm->height < limitout->min_h))) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"%dx%d of source image is not supported: too small\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (!V4L2_TYPE_IS_OUTPUT(f->type) &&
		((pixm->width < limitcap->min_w) ||
			 (pixm->height < limitcap->min_h))) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"%dx%d of target image is not supported: too small\n",
			pixm->width, pixm->height);
		return -EINVAL;
	}

	if (pixm->flags == V4L2_PIX_FMT_FLAG_PREMUL_ALPHA &&
			ctx->sc_dev->version != SCALER_VERSION(4, 0, 0))
		frame->pre_multi = true;
	else
		frame->pre_multi = false;

	frame->width = pixm->width;
	frame->height = pixm->height;
	frame->pixelformat = pixm->pixelformat;

	frame->crop.width = pixm->width;
	frame->crop.height = pixm->height;

	return 0;
}

static int sc_v4l2_reqbufs(struct file *file, void *fh,
			    struct v4l2_requestbuffers *reqbufs)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int sc_v4l2_querybuf(struct file *file, void *fh,
			     struct v4l2_buffer *buf)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_querybuf(file, ctx->m2m_ctx, buf);
}

static int sc_v4l2_qbuf(struct file *file, void *fh,
			 struct v4l2_buffer *buf)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);

	return v4l2_m2m_qbuf(file, ctx->m2m_ctx, buf);
}

static int sc_v4l2_dqbuf(struct file *file, void *fh,
			  struct v4l2_buffer *buf)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_dqbuf(file, ctx->m2m_ctx, buf);
}

static int sc_v4l2_streamon(struct file *file, void *fh,
			     enum v4l2_buf_type type)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static int sc_v4l2_streamoff(struct file *file, void *fh,
			      enum v4l2_buf_type type)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	return v4l2_m2m_streamoff(file, ctx->m2m_ctx, type);
}

static int sc_v4l2_cropcap(struct file *file, void *fh,
			    struct v4l2_cropcap *cr)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct sc_frame *frame;

	frame = ctx_get_frame(ctx, cr->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	cr->bounds.left		= 0;
	cr->bounds.top		= 0;
	cr->bounds.width	= frame->width;
	cr->bounds.height	= frame->height;
	cr->defrect		= cr->bounds;

	return 0;
}

static int sc_v4l2_g_crop(struct file *file, void *fh, struct v4l2_crop *cr)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct sc_frame *frame;

	frame = ctx_get_frame(ctx, cr->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	cr->c.left = SC_CROP_MAKE_FR_VAL(frame->crop.left, ctx->init_phase.yh);
	cr->c.top = SC_CROP_MAKE_FR_VAL(frame->crop.top, ctx->init_phase.yv);
	cr->c.width = SC_CROP_MAKE_FR_VAL(frame->crop.width, ctx->init_phase.w);
	cr->c.height = SC_CROP_MAKE_FR_VAL(frame->crop.height, ctx->init_phase.h);

	return 0;
}

static int sc_get_fract_val(struct v4l2_rect *rect, struct sc_ctx *ctx)
{
	ctx->init_phase.yh = SC_CROP_GET_FR_VAL(rect->left);
	if (ctx->init_phase.yh)
		rect->left &= SC_CROP_INT_MASK;

	ctx->init_phase.yv = SC_CROP_GET_FR_VAL(rect->top);
	if (ctx->init_phase.yv)
		rect->top &= SC_CROP_INT_MASK;

	ctx->init_phase.w = SC_CROP_GET_FR_VAL(rect->width);
	if (ctx->init_phase.w) {
		rect->width &= SC_CROP_INT_MASK;
		rect->width += 1;
	}

	ctx->init_phase.h = SC_CROP_GET_FR_VAL(rect->height);
	if (ctx->init_phase.h) {
		rect->height &= SC_CROP_INT_MASK;
		rect->height += 1;
	}

	if (sc_fmt_is_yuv420(ctx->s_frame.sc_fmt->pixelformat)) {
		ctx->init_phase.ch = ctx->init_phase.yh / 2;
		ctx->init_phase.cv = ctx->init_phase.yv / 2;
	} else {
		ctx->init_phase.ch = ctx->init_phase.yh;
		ctx->init_phase.cv = ctx->init_phase.yv;
	}

	if ((ctx->init_phase.yh || ctx->init_phase.yv || ctx->init_phase.w
			|| ctx->init_phase.h) &&
		(!(sc_fmt_is_yuv420(ctx->s_frame.sc_fmt->pixelformat) ||
		sc_fmt_is_rgb888(ctx->s_frame.sc_fmt->pixelformat)))) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"%s format on real number is not supported",
			ctx->s_frame.sc_fmt->name);
		return -EINVAL;
	}

	v4l2_dbg(1, sc_log_level, &ctx->sc_dev->m2m.v4l2_dev,
				"src crop position (x,y,w,h) =	\
				(%d.%d, %d.%d, %d.%d, %d.%d) %d, %d\n",
				rect->left, ctx->init_phase.yh,
				rect->top, ctx->init_phase.yv,
				rect->width, ctx->init_phase.w,
				rect->height, ctx->init_phase.h,
				ctx->init_phase.ch, ctx->init_phase.cv);
	return 0;
}

static int sc_v4l2_s_crop(struct file *file, void *fh,
		const struct v4l2_crop *cr)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(fh);
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *frame;
	struct v4l2_rect rect = cr->c;
	const struct sc_size_limit *limit = NULL;
	int w_align = 0;
	int h_align = 0;
	int ret = 0;

	frame = ctx_get_frame(ctx, cr->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (!test_bit(CTX_PARAMS, &ctx->flags)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"color format is not set\n");
		return -EINVAL;
	}

	if (cr->c.left < 0 || cr->c.top < 0) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"crop position is negative\n");
		return -EINVAL;
	}

	if (V4L2_TYPE_IS_OUTPUT(cr->type)) {
		ret = sc_get_fract_val(&rect, ctx);
		if (ret < 0)
			return ret;
		limit = &sc->variant->limit_input;
		set_bit(CTX_SRC_FMT, &ctx->flags);
	} else {
		limit = &sc->variant->limit_output;
		set_bit(CTX_DST_FMT, &ctx->flags);
	}

	w_align = frame->sc_fmt->h_shift;
	h_align = frame->sc_fmt->v_shift;

	/* Bound an image to have crop width and height in limit */
	v4l_bound_align_image(&rect.width, limit->min_w, limit->max_w,
			w_align, &rect.height, limit->min_h,
			limit->max_h, h_align, 0);

	/* Bound an image to have crop position in limit */
	v4l_bound_align_image(&rect.left, 0, frame->width - rect.width,
			w_align, &rect.top, 0, frame->height - rect.height,
			h_align, 0);

	if (!V4L2_TYPE_IS_OUTPUT(cr->type) &&
			sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat))
		rect.width = ALIGN_DOWN(rect.width, 4);

	if ((rect.height > frame->height) || (rect.top > frame->height) ||
		(rect.width > frame->width) || (rect.left > frame->width)) {
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"Out of crop range: (%d,%d,%d,%d) from %dx%d\n",
			rect.left, rect.top, rect.width, rect.height,
			frame->width, frame->height);
		return -EINVAL;
	}

	frame->crop.top = rect.top;
	frame->crop.left = rect.left;
	frame->crop.height = rect.height;
	frame->crop.width = rect.width;

	return 0;
}

static const struct v4l2_ioctl_ops sc_v4l2_ioctl_ops = {
	.vidioc_querycap		= sc_v4l2_querycap,

	.vidioc_enum_fmt_vid_cap_mplane	= sc_v4l2_enum_fmt_mplane,
	.vidioc_enum_fmt_vid_out_mplane	= sc_v4l2_enum_fmt_mplane,

	.vidioc_g_fmt_vid_cap_mplane	= sc_v4l2_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= sc_v4l2_g_fmt_mplane,

	.vidioc_try_fmt_vid_cap_mplane	= sc_v4l2_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= sc_v4l2_try_fmt_mplane,

	.vidioc_s_fmt_vid_cap_mplane	= sc_v4l2_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= sc_v4l2_s_fmt_mplane,

	.vidioc_reqbufs			= sc_v4l2_reqbufs,
	.vidioc_querybuf		= sc_v4l2_querybuf,

	.vidioc_qbuf			= sc_v4l2_qbuf,
	.vidioc_dqbuf			= sc_v4l2_dqbuf,

	.vidioc_streamon		= sc_v4l2_streamon,
	.vidioc_streamoff		= sc_v4l2_streamoff,

	.vidioc_g_crop			= sc_v4l2_g_crop,
	.vidioc_s_crop			= sc_v4l2_s_crop,
	.vidioc_cropcap			= sc_v4l2_cropcap
};

static int sc_ctx_stop_req(struct sc_ctx *ctx)
{
	struct sc_ctx *curr_ctx;
	struct sc_dev *sc = ctx->sc_dev;
	int ret = 0;

	curr_ctx = v4l2_m2m_get_curr_priv(sc->m2m.m2m_dev);
	if (!test_bit(CTX_RUN, &ctx->flags) || (curr_ctx != ctx))
		return 0;

	set_bit(CTX_ABORT, &ctx->flags);

	ret = wait_event_timeout(sc->wait,
			!test_bit(CTX_RUN, &ctx->flags), SC_TIMEOUT);

	/* TODO: How to handle case of timeout event */
	if (ret == 0) {
		dev_err(sc->dev, "device failed to stop request\n");
		ret = -EBUSY;
	}

	return ret;
}

static void sc_calc_planesize(struct sc_frame *frame, unsigned int pixsize)
{
	int idx = frame->sc_fmt->num_planes;

	while (idx-- > 0)
		frame->addr.size[idx] =
			(pixsize * frame->sc_fmt->bitperpixel[idx]) / 8;
}

static void sc_calc_intbufsize(struct sc_dev *sc, struct sc_int_frame *int_frame)
{
	struct sc_frame *frame = &int_frame->frame;
	unsigned int pixsize, bytesize;
	unsigned int ext_size = 0, i;
	u32 min_size = sc->variant->minsize_srcplane;

	pixsize = frame->width * frame->height;
	bytesize = (pixsize * frame->sc_fmt->bitperpixel[0]) >> 3;

	if (sc->variant->extra_buf)
		ext_size = sc_ext_buf_size(frame->width);

	switch (frame->sc_fmt->num_comp) {
	case 1:
		frame->addr.size[SC_PLANE_Y] = bytesize;
		break;
	case 2:
		if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat)) {
			sc_calc_s10b_planesize(frame->sc_fmt->pixelformat,
					frame->width, frame->height,
					&frame->addr.size[SC_PLANE_Y],
					&frame->addr.size[SC_PLANE_CB],
					false);
		} else if (frame->sc_fmt->num_planes == 1) {
			if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12_P010)
				pixsize *= 2;
			frame->addr.size[SC_PLANE_Y] = pixsize;
			frame->addr.size[SC_PLANE_CB] = bytesize - pixsize;
		} else if (frame->sc_fmt->num_planes == 2) {
			sc_calc_planesize(frame, pixsize);
		}
		break;
	case 3:
		if (frame->sc_fmt->num_planes == 1) {
			if (sc_fmt_is_ayv12(frame->sc_fmt->pixelformat)) {
				unsigned int c_span;
				c_span = ALIGN(frame->width >> 1, 16);
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = c_span * (frame->height >> 1);
				frame->addr.size[SC_PLANE_CR] = frame->addr.size[SC_PLANE_CB];
			} else {
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = (bytesize - pixsize) / 2;
				frame->addr.size[SC_PLANE_CR] = frame->addr.size[SC_PLANE_CB];
			}
		} else if (frame->sc_fmt->num_planes == 3) {
			sc_calc_planesize(frame, pixsize);
		} else {
			dev_err(sc->dev, "Please check the num of comp\n");
		}

		break;
	default:
		break;
	}

	for (i = 0; ext_size && i < frame->sc_fmt->num_comp; i++)
		frame->addr.size[i] += (i == 0) ? ext_size : ext_size/2;

	for (i = 0; i < frame->sc_fmt->num_comp; i++) {
		if (frame->addr.size[i] < min_size)
			frame->addr.size[i] = min_size;
	}

	memcpy(&int_frame->src_addr, &frame->addr, sizeof(int_frame->src_addr));
	memcpy(&int_frame->dst_addr, &frame->addr, sizeof(int_frame->dst_addr));
}

static void free_intermediate_frame(struct sc_ctx *ctx)
{
	int i;

	if (ctx->i_frame == NULL)
		return;

	if (!ctx->i_frame->dma_buf[0])
		return;

	for(i = 0; i < 3; i++) {
		if (ctx->i_frame->src_addr.ioaddr[i])
			ion_iovmm_unmap(ctx->i_frame->attachment[i],
					ctx->i_frame->src_addr.ioaddr[i]);
		if (ctx->i_frame->dst_addr.ioaddr[i])
			ion_iovmm_unmap(ctx->i_frame->attachment[i],
					ctx->i_frame->dst_addr.ioaddr[i]);
		if (ctx->i_frame->dma_buf[i]) {
			dma_buf_unmap_attachment(ctx->i_frame->attachment[i],
					ctx->i_frame->sgt[i], DMA_BIDIRECTIONAL);
			dma_buf_detach(ctx->i_frame->dma_buf[i], ctx->i_frame->attachment[i]);
			dma_buf_put(ctx->i_frame->dma_buf[i]);
		}
	}

	memset(&ctx->i_frame->dma_buf, 0, sizeof(struct dma_buf *) * 3);
	memset(&ctx->i_frame->src_addr, 0, sizeof(ctx->i_frame->src_addr));
	memset(&ctx->i_frame->dst_addr, 0, sizeof(ctx->i_frame->dst_addr));
}

static void destroy_intermediate_frame(struct sc_ctx *ctx)
{
	if (ctx->i_frame) {
		free_intermediate_frame(ctx);
		kfree(ctx->i_frame);
		ctx->i_frame = NULL;
		clear_bit(CTX_INT_FRAME, &ctx->flags);
	}
}

static bool alloc_intermediate_buffer(struct device *dev,
				      struct sc_int_frame *iframe, int i,
				      size_t size, const char *heapname,
				      unsigned long flags)
{
	iframe->dma_buf[i] = ion_alloc_dmabuf(heapname, size, flags);
	if (IS_ERR(iframe->dma_buf[i])) {
		dev_err(dev,
			"Failed to allocate intermediate buffer.%d (err %ld)",
			i, PTR_ERR(iframe->dma_buf[i]));
		goto err_dmabuf;
	}

	iframe->attachment[i] = dma_buf_attach(iframe->dma_buf[i], dev);
	if (IS_ERR(iframe->attachment[i])) {
		dev_err(dev,
			"Failed to attach from dma_buf of buffer.%d (err %ld)",
			i, PTR_ERR(iframe->attachment[i]));
		goto err_attach;
	}

	iframe->sgt[i] = dma_buf_map_attachment(iframe->attachment[i],
						DMA_BIDIRECTIONAL);
	if (IS_ERR(iframe->sgt[i])) {
		dev_err(dev, "Failed to get sgt from buffer.%d (err %ld)",
			i, PTR_ERR(iframe->sgt[i]));
		goto err_map;
	}

	iframe->src_addr.ioaddr[i] = ion_iovmm_map(iframe->attachment[i], 0,
						   size, DMA_TO_DEVICE, 0);
	if (IS_ERR_VALUE(iframe->src_addr.ioaddr[i])) {
		dev_err(dev,
			"Failed to allocate iova of buffer.%d (err %pa)",
			i, &iframe->src_addr.ioaddr[i]);
		goto err_src_map;
	}

	iframe->dst_addr.ioaddr[i] = ion_iovmm_map(iframe->attachment[i], 0,
						   size, DMA_FROM_DEVICE, 0);
	if (IS_ERR_VALUE(iframe->dst_addr.ioaddr[i])) {
		dev_err(dev,
			"Failed to allocate iova of buffer.%d (err %pa)",
			i, &iframe->dst_addr.ioaddr[i]);
		goto err_dst_map;
	}

	return true;

err_dst_map:
	iframe->dst_addr.ioaddr[i] = 0;
	ion_iovmm_unmap(iframe->attachment[i], iframe->src_addr.ioaddr[i]);
err_src_map:
	iframe->src_addr.ioaddr[i] = 0;
	dma_buf_unmap_attachment(iframe->attachment[i], iframe->sgt[i],
				 DMA_BIDIRECTIONAL);
err_map:
	iframe->sgt[i] = NULL;
	dma_buf_detach(iframe->dma_buf[i], iframe->attachment[i]);
err_attach:
	iframe->attachment[i] = NULL;
	dma_buf_put(iframe->dma_buf[i]);
err_dmabuf:
	iframe->dma_buf[i] = NULL;
	return false;
}

static bool initialize_initermediate_frame(struct sc_ctx *ctx)
{
	struct sc_frame *frame;
	struct sc_dev *sc = ctx->sc_dev;
	const char *heapname;
	unsigned long flag;
	int i;

	frame = &ctx->i_frame->frame;

	frame->crop.top = 0;
	frame->crop.left = 0;
	frame->width = frame->crop.width;
	frame->height = frame->crop.height;

	/*
	 * Check if intermeidate frame is already initialized by a previous
	 * frame. If it is already initialized, intermediate buffer is no longer
	 * needed to be initialized because image setting is never changed
	 * while streaming continues.
	 */
	if (ctx->i_frame->dma_buf[0])
		return true;

	sc_calc_intbufsize(sc, ctx->i_frame);

	if(test_bit(CTX_INT_FRAME_CP, &sc->state)) {
		heapname = "vscaler_heap";
		flag = ION_FLAG_PROTECTED;
	} else {
		heapname = "ion_system_heap";
		flag = 0;
	}

	for (i = 0; i < SC_MAX_PLANES; i++) {
		if (!frame->addr.size[i])
			break;

		if (!alloc_intermediate_buffer(sc->dev, ctx->i_frame, i,
					       frame->addr.size[i],
					       heapname, flag))
			goto err_ion_alloc;

		frame->addr.ioaddr[i] = ctx->i_frame->dst_addr.ioaddr[i];
	}

	return true;

err_ion_alloc:
	free_intermediate_frame(ctx);
	return false;
}

static bool allocate_intermediate_frame(struct sc_ctx *ctx)
{
	if (ctx->i_frame == NULL) {
		ctx->i_frame = kzalloc(sizeof(*ctx->i_frame), GFP_KERNEL);
		if (ctx->i_frame == NULL) {
			dev_err(ctx->sc_dev->dev,
				"Failed to allocate intermediate frame\n");
			return false;
		}
	}

	return true;
}

/* Zoom-out range: (x1/4, x1/16] */
static int sc_prepare_2nd_scaling(struct sc_ctx *ctx,
				  __s32 src_width, __s32 src_height,
				  unsigned int *h_ratio, unsigned int *v_ratio)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct v4l2_rect crop = ctx->d_frame.crop;
	const struct sc_size_limit *limit;
	unsigned int halign = 0, walign = 0;
	const struct sc_fmt *target_fmt = ctx->d_frame.sc_fmt;

	if (!allocate_intermediate_frame(ctx))
		return -ENOMEM;

	limit = &sc->variant->limit_input;
	if (*v_ratio > SCALE_RATIO_CONST(4, 1))
		crop.height = ((src_height + 7) / 8) * 2;

	if (crop.height < limit->min_h)
		crop.height = limit->min_h;

	if (*h_ratio > SCALE_RATIO_CONST(4, 1))
		crop.width = ((src_width + 7) / 8) * 2;

	if (crop.width < limit->min_w)
		crop.width = limit->min_w;

	if (*v_ratio < SCALE_RATIO_CONST(1, 8)) {
		crop.height = src_height * 8;
		if (crop.height > limit->max_h)
			crop.height = limit->max_h;
	}

	if (*h_ratio < SCALE_RATIO_CONST(1, 8)) {
		crop.width = src_width * 8;
		if (crop.width > limit->max_w)
			crop.width = limit->max_w;
	}

	walign = target_fmt->h_shift;
	halign = target_fmt->v_shift;

	limit = &sc->variant->limit_output;
	v4l_bound_align_image(&crop.width, limit->min_w, limit->max_w,
			walign, &crop.height, limit->min_h,
			limit->max_h, halign, 0);

	*h_ratio = SCALE_RATIO(src_width, crop.width);
	*v_ratio = SCALE_RATIO(src_height, crop.height);

	if (sc_fmt_is_s10bit_yuv(ctx->d_frame.sc_fmt->pixelformat))
		crop.width = ALIGN(crop.width, 4);

	if ((ctx->i_frame->frame.sc_fmt != ctx->d_frame.sc_fmt) ||
		memcmp(&crop, &ctx->i_frame->frame.crop, sizeof(crop)) ||
		(ctx->cp_enabled != test_bit(CTX_INT_FRAME_CP, &sc->state))) {
		if(ctx->cp_enabled)
			set_bit(CTX_INT_FRAME_CP, &sc->state);
		else
			clear_bit(CTX_INT_FRAME_CP, &sc->state);

		memcpy(&ctx->i_frame->frame, &ctx->d_frame,
				sizeof(ctx->d_frame));
		memcpy(&ctx->i_frame->frame.crop, &crop, sizeof(crop));
		free_intermediate_frame(ctx);
		if (!initialize_initermediate_frame(ctx)) {
			free_intermediate_frame(ctx);
			return -ENOMEM;
		}
	}

	return 0;
}

static struct sc_dnoise_filter sc_filter_tab[5] = {
	{SC_FT_240,   426,  240},
	{SC_FT_480,   854,  480},
	{SC_FT_720,  1280,  720},
	{SC_FT_960,  1920,  960},
	{SC_FT_1080, 1920, 1080},
};

static int sc_find_filter_size(struct sc_ctx *ctx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sc_filter_tab); i++) {
		if (sc_filter_tab[i].strength == ctx->dnoise_ft.strength) {
			if (ctx->s_frame.width >= ctx->s_frame.height) {
				ctx->dnoise_ft.w = sc_filter_tab[i].w;
				ctx->dnoise_ft.h = sc_filter_tab[i].h;
			} else {
				ctx->dnoise_ft.w = sc_filter_tab[i].h;
				ctx->dnoise_ft.h = sc_filter_tab[i].w;
			}
			break;
		}
	}

	if (i == ARRAY_SIZE(sc_filter_tab)) {
		dev_err(ctx->sc_dev->dev,
			"%s: can't find filter size\n", __func__);
		return -EINVAL;
	}

	if (ctx->s_frame.crop.width < ctx->dnoise_ft.w ||
			ctx->s_frame.crop.height < ctx->dnoise_ft.h) {
		dev_err(ctx->sc_dev->dev,
			"%s: filter is over source size.(%dx%d -> %dx%d)\n",
			__func__, ctx->s_frame.crop.width,
			ctx->s_frame.crop.height, ctx->dnoise_ft.w,
			ctx->dnoise_ft.h);
		return -EINVAL;
	}
	return 0;
}

static int sc_prepare_denoise_filter(struct sc_ctx *ctx)
{
	unsigned int sc_down_min = ctx->sc_dev->variant->sc_down_min;

	if (ctx->dnoise_ft.strength <= SC_FT_BLUR)
		return 0;

	if (sc_find_filter_size(ctx))
		return -EINVAL;

	if (!allocate_intermediate_frame(ctx))
		return -ENOMEM;

	memcpy(&ctx->i_frame->frame, &ctx->d_frame, sizeof(ctx->d_frame));
	ctx->i_frame->frame.crop.width = ctx->dnoise_ft.w;
	ctx->i_frame->frame.crop.height = ctx->dnoise_ft.h;

	free_intermediate_frame(ctx);
	if (!initialize_initermediate_frame(ctx)) {
		free_intermediate_frame(ctx);
		dev_err(ctx->sc_dev->dev,
			"%s: failed to initialize int_frame\n", __func__);
		return -ENOMEM;
	}

	ctx->h_ratio = SCALE_RATIO(ctx->s_frame.crop.width, ctx->dnoise_ft.w);
	ctx->v_ratio = SCALE_RATIO(ctx->s_frame.crop.height, ctx->dnoise_ft.h);

	if ((ctx->h_ratio > sc_down_min) ||
			(ctx->h_ratio < ctx->sc_dev->variant->sc_up_max)) {
		dev_err(ctx->sc_dev->dev,
			"filter can't support width scaling(%d -> %d)\n",
			ctx->s_frame.crop.width, ctx->dnoise_ft.w);
		goto err_ft;
	}

	if ((ctx->v_ratio > sc_down_min) ||
			(ctx->v_ratio < ctx->sc_dev->variant->sc_up_max)) {
		dev_err(ctx->sc_dev->dev,
			"filter can't support height scaling(%d -> %d)\n",
			ctx->s_frame.crop.height, ctx->dnoise_ft.h);
		goto err_ft;
	}

	if (ctx->sc_dev->variant->prescale) {
		BUG_ON(sc_down_min != SCALE_RATIO_CONST(16, 1));

		if (ctx->h_ratio > SCALE_RATIO_CONST(8, 1))
			ctx->pre_h_ratio = 2;
		else if (ctx->h_ratio > SCALE_RATIO_CONST(4, 1))
			ctx->pre_h_ratio = 1;
		else
			ctx->pre_h_ratio = 0;

		if (ctx->v_ratio > SCALE_RATIO_CONST(8, 1))
			ctx->pre_v_ratio = 2;
		else if (ctx->v_ratio > SCALE_RATIO_CONST(4, 1))
			ctx->pre_v_ratio = 1;
		else
			ctx->pre_v_ratio = 0;

		if (ctx->pre_h_ratio || ctx->pre_v_ratio) {
			if (!IS_ALIGNED(ctx->s_frame.crop.width,
					1 << (ctx->pre_h_ratio +
					ctx->s_frame.sc_fmt->h_shift))) {
				dev_err(ctx->sc_dev->dev,
			"filter can't support not-aligned source(%d -> %d)\n",
			ctx->s_frame.crop.width, ctx->dnoise_ft.w);
				goto err_ft;
			} else if (!IS_ALIGNED(ctx->s_frame.crop.height,
					1 << (ctx->pre_v_ratio +
					ctx->s_frame.sc_fmt->v_shift))) {
				dev_err(ctx->sc_dev->dev,
			"filter can't support not-aligned source(%d -> %d)\n",
			ctx->s_frame.crop.height, ctx->dnoise_ft.h);
				goto err_ft;
			} else {
				ctx->h_ratio >>= ctx->pre_h_ratio;
				ctx->v_ratio >>= ctx->pre_v_ratio;
			}
		}
	}

	return 0;

err_ft:
	free_intermediate_frame(ctx);
	return -EINVAL;
}

static int sc_find_scaling_ratio(struct sc_ctx *ctx)
{
	__s32 src_width, src_height;
	unsigned int h_ratio, v_ratio;
	struct sc_dev *sc = ctx->sc_dev;
	unsigned int sc_down_min = sc->variant->sc_down_min;
	unsigned int sc_up_max = sc->variant->sc_up_max;

	if ((ctx->s_frame.crop.width == 0) ||
			(ctx->d_frame.crop.width == 0))
		return 0; /* s_fmt is not complete */

	src_width = ctx->s_frame.crop.width;
	src_height = ctx->s_frame.crop.height;
	if (!!(ctx->flip_rot_cfg & SCALER_ROT_90))
		swap(src_width, src_height);

	h_ratio = SCALE_RATIO(src_width, ctx->d_frame.crop.width);
	v_ratio = SCALE_RATIO(src_height, ctx->d_frame.crop.height);

	/*
	 * If the source crop width or height is fractional value
	 * calculate scaling ratio including it and calculate with original
	 * crop.width and crop.height value because they were rounded up.
	 */
	if (ctx->init_phase.w)
		h_ratio = SCALE_RATIO_FRACT((src_width - 1), ctx->init_phase.w,
				ctx->d_frame.crop.width);
	if (ctx->init_phase.h)
		v_ratio = SCALE_RATIO_FRACT((src_height - 1), ctx->init_phase.h,
				ctx->d_frame.crop.height);
	sc_dbg("Scaling ratio h_ratio %d, v_ratio %d\n", h_ratio, v_ratio);

	if ((h_ratio > sc->variant->sc_down_swmin) ||
			(h_ratio < sc->variant->sc_up_swmax)) {
		dev_err(sc->dev, "Width scaling is out of range(%d -> %d)\n",
			src_width, ctx->d_frame.crop.width);
		return -EINVAL;
	}

	if ((v_ratio > sc->variant->sc_down_swmin) ||
			(v_ratio < sc->variant->sc_up_swmax)) {
		dev_err(sc->dev, "Height scaling is out of range(%d -> %d)\n",
			src_height, ctx->d_frame.crop.height);
		return -EINVAL;
	}

	if (sc->variant->prescale) {
		BUG_ON(sc_down_min != SCALE_RATIO_CONST(16, 1));

		if (h_ratio > SCALE_RATIO_CONST(8, 1)) {
			ctx->pre_h_ratio = 2;
		} else if (h_ratio > SCALE_RATIO_CONST(4, 1)) {
			ctx->pre_h_ratio = 1;
		} else {
			ctx->pre_h_ratio = 0;
		}

		if (v_ratio > SCALE_RATIO_CONST(8, 1)) {
			ctx->pre_v_ratio = 2;
		} else if (v_ratio > SCALE_RATIO_CONST(4, 1)) {
			ctx->pre_v_ratio = 1;
		} else {
			ctx->pre_v_ratio = 0;
		}

		/*
		 * If the source image resolution violates the constraints of
		 * pre-scaler, then performs poly-phase scaling twice
		 */
		if (ctx->pre_h_ratio || ctx->pre_v_ratio) {
			if (!IS_ALIGNED(src_width, 1 << (ctx->pre_h_ratio +
					ctx->s_frame.sc_fmt->h_shift)) ||
				!IS_ALIGNED(src_height, 1 << (ctx->pre_v_ratio +
					ctx->s_frame.sc_fmt->v_shift))) {
				sc_down_min = SCALE_RATIO_CONST(4, 1);
				ctx->pre_h_ratio = 0;
				ctx->pre_v_ratio = 0;
			} else {
				h_ratio >>= ctx->pre_h_ratio;
				v_ratio >>= ctx->pre_v_ratio;
			}
		}

		if (sc_down_min == SCALE_RATIO_CONST(4, 1)) {
			dev_info(sc->dev,
			"%s: Prepared 2nd polyphase scaler (%dx%d->%dx%d)\n",
			__func__,
			ctx->s_frame.crop.width, ctx->s_frame.crop.height,
			ctx->d_frame.crop.width, ctx->d_frame.crop.height);
		}
	}

	if ((h_ratio > sc_down_min) || (v_ratio > sc_down_min)
			|| (h_ratio < sc_up_max) || (v_ratio < sc_up_max)) {
		int ret;

		ret = sc_prepare_2nd_scaling(ctx, src_width, src_height,
						&h_ratio, &v_ratio);
		if (ret)
			return ret;
	} else {
		destroy_intermediate_frame(ctx);
	}

	ctx->h_ratio = h_ratio;
	ctx->v_ratio = v_ratio;

	return 0;
}

static int sc_vb2_queue_setup(struct vb2_queue *vq,
		unsigned int *num_buffers, unsigned int *num_planes,
		unsigned int sizes[], struct device *alloc_devs[])
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	struct sc_frame *frame;
	int ret;
	int i;

	frame = ctx_get_frame(ctx, vq->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	/* Get number of planes from format_list in driver */
	*num_planes = frame->sc_fmt->num_planes;
	for (i = 0; i < frame->sc_fmt->num_planes; i++) {
		sizes[i] = frame->bytesused[i];
		alloc_devs[i] = ctx->sc_dev->dev;
	}

	ret = sc_find_scaling_ratio(ctx);
	if (ret)
		return ret;

	ret = sc_prepare_denoise_filter(ctx);
	if (ret)
		return ret;

	return 0;
}

/*
 * This function should be used for source buffer only.
 * In case of destination buffer, frame->bytesused[] is not valid.
 */
static int sc_check_src_plane_size(struct sc_ctx *ctx, struct vb2_buffer *vb,
					 struct sc_frame *frame, u32 min_size)
{
	struct sg_table *sgt;
	int i, ret = 0;

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type) || !min_size)
		return ret;

	for (i = 0; i < frame->sc_fmt->num_planes; i++) {
		if (frame->bytesused[i] < min_size) {
			sgt = (struct sg_table *)vb2_plane_cookie(vb, i);
			if (!sgt) {
				v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
					"invalid sgt in plane %d\n", i);
				return -EINVAL;
			}

			if (sg_nents_for_len(sgt->sgl, (u64)min_size) <= 0) {
				ret = -EINVAL;
				break;
			}
		}
	}

	if (ret)
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
			"plane%d size %d is smaller than %d\n",
			i, frame->bytesused[i], min_size);

	return ret;
}

static int sc_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct sc_frame *frame;
	u32 min_size = ctx->sc_dev->variant->minsize_srcplane;
	int i;

	frame = ctx_get_frame(ctx, vb->vb2_queue->type);
	if (IS_ERR(frame))
		return PTR_ERR(frame);

	if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		/* Plane size checking is needed for source buffer */
		if (sc_check_src_plane_size(ctx, vb, frame, min_size))
			return -EINVAL;
	} else {
		for (i = 0; i < frame->sc_fmt->num_planes; i++)
			vb2_set_plane_payload(vb, i, frame->bytesused[i]);
	}

	return 0;
}

static void sc_vb2_buf_finish(struct vb2_buffer *vb)
{
}

static void sc_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, v4l2_buf);
	v4l2_m2m_try_schedule(ctx->m2m_ctx);
}

static void sc_vb2_buf_cleanup(struct vb2_buffer *vb)
{
}

static void sc_vb2_lock(struct vb2_queue *vq)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	mutex_lock(&ctx->sc_dev->lock);
}

static void sc_vb2_unlock(struct vb2_queue *vq)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	mutex_unlock(&ctx->sc_dev->lock);
}

static int sc_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	set_bit(CTX_STREAMING, &ctx->flags);

	return 0;
}

static void sc_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct sc_ctx *ctx = vb2_get_drv_priv(vq);
	int ret;

	ret = sc_ctx_stop_req(ctx);
	if (ret < 0)
		dev_err(ctx->sc_dev->dev, "wait timeout\n");

	clear_bit(CTX_STREAMING, &ctx->flags);
}

static struct vb2_ops sc_vb2_ops = {
	.queue_setup		= sc_vb2_queue_setup,
	.buf_prepare		= sc_vb2_buf_prepare,
	.buf_finish		= sc_vb2_buf_finish,
	.buf_queue		= sc_vb2_buf_queue,
	.buf_cleanup		= sc_vb2_buf_cleanup,
	.wait_finish		= sc_vb2_lock,
	.wait_prepare		= sc_vb2_unlock,
	.start_streaming	= sc_vb2_start_streaming,
	.stop_streaming		= sc_vb2_stop_streaming,
};

static int queue_init(void *priv, struct vb2_queue *src_vq,
		      struct vb2_queue *dst_vq)
{
	struct sc_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->ops = &sc_vb2_ops;
	src_vq->mem_ops = &vb2_dma_sg_memops;
	src_vq->drv_priv = ctx;
	src_vq->buf_struct_size = sizeof(struct vb2_sc_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->ops = &sc_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_sg_memops;
	dst_vq->drv_priv = ctx;
	dst_vq->buf_struct_size = sizeof(struct vb2_sc_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;

	return vb2_queue_init(dst_vq);
}

static bool sc_configure_rotation_degree(struct sc_ctx *ctx, int degree)
{
	ctx->flip_rot_cfg &= ~SCALER_ROT_MASK;

	/*
	 * we expect that the direction of rotation is clockwise
	 * but the Scaler does in counter clockwise.
	 * Since the GScaler doest that in clockwise,
	 * the following makes the direction of rotation by the Scaler
	 * clockwise.
	 */
	if (degree == 270) {
		ctx->flip_rot_cfg |= SCALER_ROT_90;
	} else if (degree == 180) {
		ctx->flip_rot_cfg |= SCALER_ROT_180;
	} else if (degree == 90) {
		ctx->flip_rot_cfg |= SCALER_ROT_270;
	} else if (degree != 0) {
		dev_err(ctx->sc_dev->dev,
			"%s: Rotation of %d is not supported\n",
			__func__, degree);
		return false;
	}

	return true;
}

static void sc_set_framerate(struct sc_ctx *ctx, int framerate)
{
	if (!ctx->sc_dev->qos_table)
		return;

	mutex_lock(&ctx->pm_qos_lock);
	if (framerate == 0) {
		cancel_delayed_work(&ctx->qos_work);
		sc_remove_devfreq(&ctx->pm_qos, ctx->sc_dev->qos_table);
		ctx->framerate = 0;
	} else {
		if (framerate != ctx->framerate) {
			ctx->framerate = framerate;
			if (!sc_get_pm_qos_level(ctx, ctx->framerate)) {
				mutex_unlock(&ctx->pm_qos_lock);
				return;
			}
			sc_request_devfreq(&ctx->pm_qos,
					ctx->sc_dev->qos_table, ctx->pm_qos_lv);
		}
		mod_delayed_work(system_wq,
				&ctx->qos_work, msecs_to_jiffies(50));
	}
	mutex_unlock(&ctx->pm_qos_lock);
}

static int sc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sc_ctx *ctx;
	int ret = 0;

	sc_dbg("ctrl ID:%d, value:%d\n", ctrl->id, ctrl->val);
	ctx = container_of(ctrl->handler, struct sc_ctx, ctrl_handler);

	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
		if (ctrl->val)
			ctx->flip_rot_cfg |= SCALER_FLIP_X_EN;
		else
			ctx->flip_rot_cfg &= ~SCALER_FLIP_X_EN;
		break;
	case V4L2_CID_HFLIP:
		if (ctrl->val)
			ctx->flip_rot_cfg |= SCALER_FLIP_Y_EN;
		else
			ctx->flip_rot_cfg &= ~SCALER_FLIP_Y_EN;
		break;
	case V4L2_CID_ROTATE:
		if (!sc_configure_rotation_degree(ctx, ctrl->val))
			return -EINVAL;
		break;
	case V4L2_CID_GLOBAL_ALPHA:
		ctx->g_alpha = ctrl->val;
		break;
	case V4L2_CID_2D_BLEND_OP:
		if (!ctx->sc_dev->variant->blending && (ctrl->val > 0)) {
			dev_err(ctx->sc_dev->dev,
				"%s: blending is not supported from v2.2.0\n",
				__func__);
			return -EINVAL;
		}
		ctx->bl_op = ctrl->val;
		break;
	case V4L2_CID_2D_FMT_PREMULTI:
		ctx->pre_multi = ctrl->val;
		break;
	case V4L2_CID_2D_DITH:
		ctx->dith = ctrl->val;
		break;
	case V4L2_CID_CSC_EQ:
		ctx->csc.csc_eq = ctrl->val;
		break;
	case V4L2_CID_CSC_RANGE:
		ctx->csc.csc_range = ctrl->val;
		break;
	case V4L2_CID_CONTENT_PROTECTION:
		ctx->cp_enabled = !!ctrl->val;
		break;
	case SC_CID_DNOISE_FT:
		ctx->dnoise_ft.strength = ctrl->val;
		break;
	case SC_CID_FRAMERATE:
		sc_set_framerate(ctx, ctrl->val);
		break;
	}

	return ret;
}

static const struct v4l2_ctrl_ops sc_ctrl_ops = {
	.s_ctrl = sc_s_ctrl,
};

static const struct v4l2_ctrl_config sc_custom_ctrl[] = {
	{
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_GLOBAL_ALPHA,
		.name = "Set constant src alpha",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = 0,
		.max = 255,
		.def = 255,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_2D_BLEND_OP,
		.name = "set blend op",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = 0,
		.max = BL_OP_ADD,
		.def = 0,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_2D_DITH,
		.name = "set dithering",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = false,
		.max = true,
		.def = false,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_2D_FMT_PREMULTI,
		.name = "set pre-multiplied format",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = false,
		.max = true,
		.def = false,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_CSC_EQ,
		.name = "Set CSC equation",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = V4L2_COLORSPACE_DEFAULT,
		.max = V4L2_COLORSPACE_BT2020,
		.def = V4L2_COLORSPACE_DEFAULT,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_CSC_RANGE,
		.name = "Set CSC range",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = SC_CSC_NARROW,
		.max = SC_CSC_WIDE,
		.def = SC_CSC_NARROW,
	}, {
		.ops = &sc_ctrl_ops,
		.id = V4L2_CID_CONTENT_PROTECTION,
		.name = "Enable contents protection",
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.flags = V4L2_CTRL_FLAG_SLIDER,
		.step = 1,
		.min = 0,
		.max = 1,
		.def = 0,
	}, {
		.ops = &sc_ctrl_ops,
		.id = SC_CID_DNOISE_FT,
		.name = "Enable denoising filter",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.step = 1,
		.min = 0,
		.max = SC_FT_MAX,
		.def = 0,
	}, {
		.ops = &sc_ctrl_ops,
		.id = SC_CID_FRAMERATE,
		.name = "Frame rate setting",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.flags = V4L2_CTRL_FLAG_EXECUTE_ON_WRITE,
		.step = 1,
		.min = 0,
		.max = SC_FRAMERATE_MAX,
		.def = 0,
	}
};

static int sc_add_ctrls(struct sc_ctx *ctx)
{
	unsigned long i;

	v4l2_ctrl_handler_init(&ctx->ctrl_handler, SC_MAX_CTRL_NUM);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &sc_ctrl_ops,
			V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &sc_ctrl_ops,
			V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(&ctx->ctrl_handler, &sc_ctrl_ops,
			V4L2_CID_ROTATE, 0, 270, 90, 0);

	for (i = 0; i < ARRAY_SIZE(sc_custom_ctrl); i++)
		v4l2_ctrl_new_custom(&ctx->ctrl_handler,
				&sc_custom_ctrl[i], NULL);
	if (ctx->ctrl_handler.error) {
		int err = ctx->ctrl_handler.error;
		v4l2_err(&ctx->sc_dev->m2m.v4l2_dev,
				"v4l2_ctrl_handler_init failed %d\n", err);
		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		return err;
	}

	v4l2_ctrl_handler_setup(&ctx->ctrl_handler);

	return 0;
}

static int sc_power_clk_enable(struct sc_dev *sc)
{
	int ret;

	if (in_interrupt())
		ret = pm_runtime_get(sc->dev);
	else
		ret = pm_runtime_get_sync(sc->dev);

	if (ret < 0) {
		dev_err(sc->dev,
			"%s=%d: Failed to enable local power\n", __func__, ret);
		return ret;
	}

	if (!IS_ERR(sc->pclk)) {
		ret = clk_enable(sc->pclk);
		if (ret) {
			dev_err(sc->dev, "%s: Failed to enable PCLK (err %d)\n",
				__func__, ret);
			goto err_pclk;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_enable(sc->aclk);
		if (ret) {
			dev_err(sc->dev, "%s: Failed to enable ACLK (err %d)\n",
				__func__, ret);
			goto err_aclk;
		}
	}

	return 0;
err_aclk:
	if (!IS_ERR(sc->pclk))
		clk_disable(sc->pclk);
err_pclk:
	pm_runtime_put(sc->dev);
	return ret;
}

static void sc_clk_power_disable(struct sc_dev *sc)
{
	sc_clear_aux_power_cfg(sc);

	if (!IS_ERR(sc->aclk))
		clk_disable(sc->aclk);

	if (!IS_ERR(sc->pclk))
		clk_disable(sc->pclk);

	pm_runtime_put(sc->dev);
}

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
static int sc_ctrl_protection(struct sc_dev *sc, struct sc_ctx *ctx, bool en)
{
	int ret;

	ret = exynos_smc(SMC_PROTECTION_SET, 0,
			SC_SMC_PROTECTION_ID(sc->dev_id),
			(en ? SMC_PROTECTION_ENABLE : SMC_PROTECTION_DISABLE));
	if (ret != 0) {
		dev_err(sc->dev,
			"fail to protection enable (%d)\n", ret);
	}
	return ret;
}
#endif

static void sc_timeout_qos_work(struct work_struct *work)
{
	struct sc_ctx *ctx = container_of(work, struct sc_ctx,
						qos_work.work);

	mutex_lock(&ctx->pm_qos_lock);

	sc_remove_devfreq(&ctx->pm_qos, ctx->sc_dev->qos_table);
	ctx->framerate = 0;

	mutex_unlock(&ctx->pm_qos_lock);
}

static int sc_open(struct file *file)
{
	struct sc_dev *sc = video_drvdata(file);
	struct sc_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(sc->dev, "no memory for open context\n");
		return -ENOMEM;
	}

	atomic_inc(&sc->m2m.in_use);

	ctx->context_type = SC_CTX_V4L2_TYPE;
	INIT_LIST_HEAD(&ctx->node);
	ctx->sc_dev = sc;

	v4l2_fh_init(&ctx->fh, sc->m2m.vfd);

	INIT_DELAYED_WORK(&ctx->qos_work, sc_timeout_qos_work);
	ctx->pm_qos_lv = -1;
	mutex_init(&ctx->pm_qos_lock);

	ret = sc_add_ctrls(ctx);
	if (ret)
		goto err_fh;

	ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);

	/* Default color format */
	ctx->s_frame.sc_fmt = &sc_formats[0];
	ctx->d_frame.sc_fmt = &sc_formats[0];

	if (!IS_ERR(sc->pclk)) {
		ret = clk_prepare(sc->pclk);
		if (ret) {
			dev_err(sc->dev, "%s: failed to prepare PCLK(err %d)\n",
					__func__, ret);
			goto err_pclk_prepare;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_prepare(sc->aclk);
		if (ret) {
			dev_err(sc->dev, "%s: failed to prepare ACLK(err %d)\n",
					__func__, ret);
			goto err_aclk_prepare;
		}
	}

	/* Setup the device context for mem2mem mode. */
	ctx->m2m_ctx = v4l2_m2m_ctx_init(sc->m2m.m2m_dev, ctx, queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		ret = -EINVAL;
		goto err_ctx;
	}

	return 0;

err_ctx:
	if (!IS_ERR(sc->aclk))
		clk_unprepare(sc->aclk);
err_aclk_prepare:
	if (!IS_ERR(sc->pclk))
		clk_unprepare(sc->pclk);
err_pclk_prepare:
	v4l2_fh_del(&ctx->fh);
err_fh:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_exit(&ctx->fh);
	atomic_dec(&sc->m2m.in_use);
	kfree(ctx);

	return ret;
}

static int sc_release(struct file *file)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(file->private_data);
	struct sc_dev *sc = ctx->sc_dev;

	sc_dbg("refcnt= %d", atomic_read(&sc->m2m.in_use));

	atomic_dec(&sc->m2m.in_use);

	v4l2_m2m_ctx_release(ctx->m2m_ctx);

	destroy_intermediate_frame(ctx);

	if (ctx->framerate)
		flush_delayed_work(&ctx->qos_work);

	if (!IS_ERR(sc->aclk))
		clk_unprepare(sc->aclk);
	if (!IS_ERR(sc->pclk))
		clk_unprepare(sc->pclk);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	return 0;
}

static unsigned int sc_poll(struct file *file,
			     struct poll_table_struct *wait)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(file->private_data);

	return v4l2_m2m_poll(file, ctx->m2m_ctx, wait);
}

static int sc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct sc_ctx *ctx = fh_to_sc_ctx(file->private_data);

	return v4l2_m2m_mmap(file, ctx->m2m_ctx, vma);
}

static const struct v4l2_file_operations sc_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= sc_open,
	.release	= sc_release,
	.poll		= sc_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= sc_mmap,
};

static void sc_job_finish(struct sc_dev *sc, struct sc_ctx *ctx)
{
	unsigned long flags;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;

	spin_lock_irqsave(&sc->slock, flags);

	if (ctx->context_type == SC_CTX_V4L2_TYPE) {
		ctx = v4l2_m2m_get_curr_priv(sc->m2m.m2m_dev);
		if (!ctx || !ctx->m2m_ctx) {
			dev_err(sc->dev, "current ctx is NULL\n");
			spin_unlock_irqrestore(&sc->slock, flags);
			return;

		}
		clear_bit(CTX_RUN, &ctx->flags);

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
		if (test_bit(DEV_CP, &sc->state)) {
			sc_ctrl_protection(sc, ctx, false);
			clear_bit(DEV_CP, &sc->state);
		}
#endif
		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		BUG_ON(!src_vb || !dst_vb);

		v4l2_m2m_buf_done(src_vb, VB2_BUF_STATE_ERROR);
		v4l2_m2m_buf_done(dst_vb, VB2_BUF_STATE_ERROR);

		v4l2_m2m_job_finish(sc->m2m.m2m_dev, ctx->m2m_ctx);
	} else {
		struct m2m1shot_task *task =
			m2m1shot_get_current_task(sc->m21dev);

		BUG_ON(ctx->context_type != SC_CTX_M2M1SHOT_TYPE);

		m2m1shot_task_finish(sc->m21dev, task, true);
	}

	spin_unlock_irqrestore(&sc->slock, flags);
}

static void sc_watchdog(unsigned long arg)
{
	struct sc_dev *sc = (struct sc_dev *)arg;
	struct sc_ctx *ctx;
	unsigned long flags;

	sc_dbg("timeout watchdog\n");
	if (atomic_read(&sc->wdt.cnt) >= SC_WDT_CNT) {
		sc_hwset_soft_reset(sc);

		atomic_set(&sc->wdt.cnt, 0);
		clear_bit(DEV_RUN, &sc->state);

		spin_lock_irqsave(&sc->ctxlist_lock, flags);
		ctx = sc->current_ctx;
		sc->current_ctx = NULL;
		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

		BUG_ON(!ctx);
		sc_job_finish(sc, ctx);
		sc_clk_power_disable(sc);
		return;
	}

	if (test_bit(DEV_RUN, &sc->state)) {
		sc_hwregs_dump(sc);
		if (sc->current_ctx)
			sc_ctx_dump(sc->current_ctx);
		exynos_sysmmu_show_status(sc->dev);
		atomic_inc(&sc->wdt.cnt);
		dev_err(sc->dev, "scaler is still running\n");
		mod_timer(&sc->wdt.timer, jiffies + SC_TIMEOUT);
	} else {
		sc_dbg("scaler finished job\n");
	}

}

static void sc_set_csc_coef(struct sc_ctx *ctx)
{
	struct sc_frame *s_frame, *d_frame;
	struct sc_dev *sc;
	enum sc_csc_idx idx;

	sc = ctx->sc_dev;
	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	if (s_frame->sc_fmt->is_rgb == d_frame->sc_fmt->is_rgb)
		idx = NO_CSC;
	else if (s_frame->sc_fmt->is_rgb)
		idx = CSC_R2Y;
	else
		idx = CSC_Y2R;

	sc_hwset_csc_coef(sc, idx, &ctx->csc);
}

static bool sc_process_2nd_stage(struct sc_dev *sc, struct sc_ctx *ctx)
{
	struct sc_frame *s_frame, *d_frame;
	const struct sc_size_limit *limit;
	unsigned int halign = 0, walign = 0;
	unsigned int pre_h_ratio = 0;
	unsigned int pre_v_ratio = 0;
	unsigned int h_ratio = SCALE_RATIO(1, 1);
	unsigned int v_ratio = SCALE_RATIO(1, 1);

	if (!test_bit(CTX_INT_FRAME, &ctx->flags))
		return false;

	s_frame = &ctx->i_frame->frame;
	d_frame = &ctx->d_frame;

	s_frame->addr.ioaddr[SC_PLANE_Y] = ctx->i_frame->src_addr.ioaddr[SC_PLANE_Y];
	s_frame->addr.ioaddr[SC_PLANE_CB] = ctx->i_frame->src_addr.ioaddr[SC_PLANE_CB];
	s_frame->addr.ioaddr[SC_PLANE_CR] = ctx->i_frame->src_addr.ioaddr[SC_PLANE_CR];

	walign = d_frame->sc_fmt->h_shift;
	halign = d_frame->sc_fmt->v_shift;

	limit = &sc->variant->limit_input;
	v4l_bound_align_image(&s_frame->crop.width, limit->min_w, limit->max_w,
			walign, &s_frame->crop.height, limit->min_h,
			limit->max_h, halign, 0);

	sc_hwset_src_image_format(sc, s_frame->sc_fmt);
	sc_hwset_dst_image_format(sc, d_frame->sc_fmt);
	sc_hwset_src_imgsize(sc, s_frame);
	sc_hwset_dst_imgsize(sc, d_frame);

	if ((ctx->flip_rot_cfg & SCALER_ROT_90) &&
		(ctx->dnoise_ft.strength > SC_FT_BLUR)) {
		h_ratio = SCALE_RATIO(s_frame->crop.height, d_frame->crop.width);
		v_ratio = SCALE_RATIO(s_frame->crop.width, d_frame->crop.height);
	} else {
		h_ratio = SCALE_RATIO(s_frame->crop.width, d_frame->crop.width);
		v_ratio = SCALE_RATIO(s_frame->crop.height, d_frame->crop.height);
	}

	pre_h_ratio = 0;
	pre_v_ratio = 0;

	if (!sc->variant->ratio_20bit) {
		/* No prescaler, 1/4 precision */
		BUG_ON(h_ratio > SCALE_RATIO(4, 1));
		BUG_ON(v_ratio > SCALE_RATIO(4, 1));

		h_ratio >>= 4;
		v_ratio >>= 4;
	}

	/* no rotation */

	sc_hwset_hratio(sc, h_ratio, pre_h_ratio);
	sc_hwset_vratio(sc, v_ratio, pre_v_ratio);

	sc_hwset_polyphase_hcoef(sc, h_ratio, h_ratio, 0);
	sc_hwset_polyphase_vcoef(sc, v_ratio, v_ratio, 0);

	sc_hwset_src_pos(sc, s_frame->crop.left, s_frame->crop.top,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);
	sc_hwset_src_wh(sc, s_frame->crop.width, s_frame->crop.height,
			pre_h_ratio, pre_v_ratio,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);

	sc_hwset_dst_pos(sc, d_frame->crop.left, d_frame->crop.top);
	sc_hwset_dst_wh(sc, d_frame->crop.width, d_frame->crop.height);

	sc_hwset_src_addr(sc, s_frame);
	sc_hwset_dst_addr(sc, d_frame);

	if ((ctx->flip_rot_cfg & SCALER_ROT_MASK) &&
		(ctx->dnoise_ft.strength > SC_FT_BLUR))
		sc_hwset_flip_rotation(sc, ctx->flip_rot_cfg);
	else
		sc_hwset_flip_rotation(sc, 0);

	sc_hwset_start(sc);

	clear_bit(CTX_INT_FRAME, &ctx->flags);

	return true;
}

static void sc_set_dithering(struct sc_ctx *ctx)
{
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *s_frame = &ctx->s_frame;
	struct sc_frame *d_frame = &ctx->d_frame;
	unsigned int val = 0;

	if (ctx->dith)
		val = sc_dith_val(1, 1, 1);

	if (sc->variant->pixfmt_10bit) {
		if (sc_fmt_is_s10bit_yuv(s_frame->sc_fmt->pixelformat))
			val |= SCALER_DITH_SRC_INV;
		if (sc_fmt_is_s10bit_yuv(d_frame->sc_fmt->pixelformat))
			val |= SCALER_DITH_DST_EN;
	}

	sc_dbg("dither value is 0x%x\n", val);
	sc_hwset_dith(sc, val);
}

static void sc_set_initial_phase(struct sc_ctx *ctx)
{
	struct sc_dev *sc = ctx->sc_dev;

	/* TODO: need to check scaling, csc, rot according to H/W Goude  */
	sc_hwset_src_init_phase(sc, &ctx->init_phase);
}

static int sc_run_next_job(struct sc_dev *sc)
{
	unsigned long flags;
	struct sc_ctx *ctx;
	struct sc_frame *d_frame, *s_frame;
	unsigned int pre_h_ratio = 0;
	unsigned int pre_v_ratio = 0;
	unsigned int h_ratio = SCALE_RATIO(1, 1);
	unsigned int v_ratio = SCALE_RATIO(1, 1);
	unsigned int ch_ratio = SCALE_RATIO(1, 1);
	unsigned int cv_ratio = SCALE_RATIO(1, 1);
	unsigned int h_shift, v_shift;
	int ret;

	spin_lock_irqsave(&sc->ctxlist_lock, flags);

	if (sc->current_ctx || list_empty(&sc->context_list)) {
		/* a job is currently being processed or no job is to run */
		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);
		return 0;
	}

	/*
	 * sc_run_next_job() must not reenter while sc->state is DEV_RUN.
	 * DEV_RUN is cleared when an operation is finished.
	 */
	BUG_ON(test_bit(DEV_RUN, &sc->state));

	set_bit(DEV_RUN, &sc->state);

	if (test_bit(DEV_SUSPEND, &sc->state)) {
		clear_bit(DEV_RUN, &sc->state);
		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);
		return 0;
	}

	ctx = list_first_entry(&sc->context_list, struct sc_ctx, node);

	set_bit(CTX_RUN, &ctx->flags);
	list_del_init(&ctx->node);

	sc->current_ctx = ctx;

	spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

	ret = sc_power_clk_enable(sc);
	if (ret) {
		/*
		 * Failed to enable the power and the clock. Let's push the task
		 * again for the later retry.
		 */
		spin_lock_irqsave(&sc->ctxlist_lock, flags);

		list_add(&ctx->node, &sc->context_list);
		sc->current_ctx = NULL;

		clear_bit(CTX_RUN, &ctx->flags);

		spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

		clear_bit(DEV_RUN, &sc->state);

		/*
		 * V4L2 mem2mem assumes that the tasks in device_run() are
		 * always succeed in processing in H/W while m2m1shot accepts
		 * failure in device_run(). m2m1shot2 returns failure to the
		 * users if devce_run() fails. To prevent returning failure to
		 * users and losing a task to run, we should assume that
		 * processing a task always succeeds.
		 */
		return 0;
	}

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	sc_hwset_clk_request(sc, true);
	sc_hwset_init(sc);

	if (ctx->i_frame) {
		set_bit(CTX_INT_FRAME, &ctx->flags);
		d_frame = &ctx->i_frame->frame;
	}

	sc_set_csc_coef(ctx);

	sc_hwset_src_image_format(sc, s_frame->sc_fmt);
	sc_hwset_dst_image_format(sc, d_frame->sc_fmt);

	sc_hwset_pre_multi_format(sc, s_frame->pre_multi, d_frame->pre_multi);

	sc_hwset_src_imgsize(sc, s_frame);
	sc_hwset_dst_imgsize(sc, d_frame);

	h_ratio = ctx->h_ratio;
	v_ratio = ctx->v_ratio;
	pre_h_ratio = ctx->pre_h_ratio;
	pre_v_ratio = ctx->pre_v_ratio;

	if (!sc->variant->ratio_20bit) {

		/* No prescaler, 1/4 precision */
		BUG_ON(h_ratio > SCALE_RATIO(4, 1));
		BUG_ON(v_ratio > SCALE_RATIO(4, 1));

		h_ratio >>= 4;
		v_ratio >>= 4;
	}

	h_shift = s_frame->sc_fmt->h_shift;
	v_shift = s_frame->sc_fmt->v_shift;

	if (!!(ctx->flip_rot_cfg & SCALER_ROT_90)) {
		swap(pre_h_ratio, pre_v_ratio);
		swap(h_shift, v_shift);
	}

	if (h_shift < d_frame->sc_fmt->h_shift)
		ch_ratio = h_ratio * 2; /* chroma scaling down */
	else if (h_shift > d_frame->sc_fmt->h_shift)
		ch_ratio = h_ratio / 2; /* chroma scaling up */
	else
		ch_ratio = h_ratio;

	if (v_shift < d_frame->sc_fmt->v_shift)
		cv_ratio = v_ratio * 2; /* chroma scaling down */
	else if (v_shift > d_frame->sc_fmt->v_shift)
		cv_ratio = v_ratio / 2; /* chroma scaling up */
	else
		cv_ratio = v_ratio;

	sc_hwset_hratio(sc, h_ratio, pre_h_ratio);
	sc_hwset_vratio(sc, v_ratio, pre_v_ratio);

	sc_hwset_polyphase_hcoef(sc, h_ratio, ch_ratio,
			ctx->dnoise_ft.strength);
	sc_hwset_polyphase_vcoef(sc, v_ratio, cv_ratio,
			ctx->dnoise_ft.strength);

	sc_hwset_src_pos(sc, s_frame->crop.left, s_frame->crop.top,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);
	sc_hwset_src_wh(sc, s_frame->crop.width, s_frame->crop.height,
			pre_h_ratio, pre_v_ratio,
			s_frame->sc_fmt->h_shift, s_frame->sc_fmt->v_shift);

	sc_hwset_dst_pos(sc, d_frame->crop.left, d_frame->crop.top);
	sc_hwset_dst_wh(sc, d_frame->crop.width, d_frame->crop.height);

	if (sc->variant->initphase)
		sc_set_initial_phase(ctx);

	sc_hwset_src_addr(sc, s_frame);
	sc_hwset_dst_addr(sc, d_frame);

	sc_set_dithering(ctx);

	if (ctx->bl_op)
		sc_hwset_blend(sc, ctx->bl_op, ctx->pre_multi, ctx->g_alpha);

	if (ctx->dnoise_ft.strength > SC_FT_BLUR)
		sc_hwset_flip_rotation(sc, 0);
	else
		sc_hwset_flip_rotation(sc, ctx->flip_rot_cfg);

	sc_hwset_int_en(sc);

	if (sc_show_stat & 0x1)
		sc_hwregs_dump(sc);
	if (sc_show_stat & 0x2)
		sc_ctx_dump(ctx);

	mod_timer(&sc->wdt.timer, jiffies + SC_TIMEOUT);

	if (__measure_hw_latency) {
		if (ctx->context_type == SC_CTX_V4L2_TYPE) {
			struct vb2_v4l2_buffer *vb =
					v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
			struct v4l2_m2m_buffer *mb =
					container_of(vb, typeof(*mb), vb);
			struct vb2_sc_buffer *svb =
					container_of(mb, typeof(*svb), mb);

			svb->ktime = ktime_get();
		} else {
			ctx->ktime_m2m1shot = ktime_get();
		}
	}
#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (ctx->cp_enabled) {
		ret = sc_ctrl_protection(sc, ctx, true);
		if (!ret)
			set_bit(DEV_CP, &sc->state);
	}
#endif
	sc_hwset_start(sc);

	return 0;
}

static int sc_add_context_and_run(struct sc_dev *sc, struct sc_ctx *ctx)
{
	unsigned long flags;

	if (test_bit(CTX_ABORT, &ctx->flags)) {
		dev_err(sc->dev, "aborted scaler device run\n");
		return -EAGAIN;
	}

	spin_lock_irqsave(&sc->ctxlist_lock, flags);
	list_add_tail(&ctx->node, &sc->context_list);
	spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

	return sc_run_next_job(sc);
}

static irqreturn_t sc_irq_handler(int irq, void *priv)
{
	struct sc_dev *sc = priv;
	struct sc_ctx *ctx;
	struct vb2_v4l2_buffer *src_vb, *dst_vb;
	u32 irq_status;

	spin_lock(&sc->slock);

	/*
	 * ok to access sc->current_ctx withot ctxlist_lock held
	 * because it is not modified until sc_run_next_job() is called.
	 */
	ctx = sc->current_ctx;

	BUG_ON(!ctx);

	irq_status = sc_hwget_and_clear_irq_status(sc);

	if (SCALER_INT_OK(irq_status) && sc_process_2nd_stage(sc, ctx))
		goto isr_unlock;

	del_timer(&sc->wdt.timer);

#ifdef CONFIG_EXYNOS_CONTENT_PATH_PROTECTION
	if (test_bit(DEV_CP, &sc->state)) {
		sc_ctrl_protection(sc, ctx, false);
		clear_bit(DEV_CP, &sc->state);
	}
#endif
	if (!SCALER_INT_OK(irq_status))
		sc_hwset_soft_reset(sc);

	sc_hwset_clk_request(sc, false);

	clear_bit(DEV_RUN, &sc->state);
	clear_bit(CTX_RUN, &ctx->flags);

	if (ctx->context_type == SC_CTX_V4L2_TYPE) {
		BUG_ON(ctx != v4l2_m2m_get_curr_priv(sc->m2m.m2m_dev));

		src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
		dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);

		BUG_ON(!src_vb || !dst_vb);

		if (__measure_hw_latency) {
			struct v4l2_m2m_buffer *mb =
					container_of(dst_vb, typeof(*mb), vb);
			struct vb2_sc_buffer *svb =
					container_of(mb, typeof(*svb), mb);

			dst_vb->vb2_buf.timestamp =
				(__u32)ktime_us_delta(ktime_get(), svb->ktime);
		}

		v4l2_m2m_buf_done(src_vb,
			SCALER_INT_OK(irq_status) ?
				VB2_BUF_STATE_DONE : VB2_BUF_STATE_ERROR);
		v4l2_m2m_buf_done(dst_vb,
			SCALER_INT_OK(irq_status) ?
				VB2_BUF_STATE_DONE : VB2_BUF_STATE_ERROR);

		v4l2_m2m_job_finish(sc->m2m.m2m_dev, ctx->m2m_ctx);

		/* Wake up from CTX_ABORT state */
		clear_bit(CTX_ABORT, &ctx->flags);
	} else {
		struct m2m1shot_task *task =
					m2m1shot_get_current_task(sc->m21dev);

		BUG_ON(ctx->context_type != SC_CTX_M2M1SHOT_TYPE);

		if (__measure_hw_latency)
			task->task.reserved[1] =
				(unsigned long)ktime_us_delta(
					ktime_get(), ctx->ktime_m2m1shot);

		m2m1shot_task_finish(sc->m21dev, task,
					SCALER_INT_OK(irq_status));
	}

	spin_lock(&sc->ctxlist_lock);
	sc->current_ctx = NULL;
	spin_unlock(&sc->ctxlist_lock);

	wake_up(&sc->wait);

	sc_run_next_job(sc);

	sc_clk_power_disable(sc);
isr_unlock:
	spin_unlock(&sc->slock);

	return IRQ_HANDLED;
}

static int sc_get_bufaddr(struct sc_dev *sc, struct vb2_buffer *vb2buf,
		struct sc_frame *frame)
{
	unsigned int pixsize, bytesize;

	pixsize = frame->width * frame->height;
	bytesize = (pixsize * frame->sc_fmt->bitperpixel[0]) >> 3;

	frame->addr.ioaddr[SC_PLANE_Y] = vb2_dma_sg_plane_dma_addr(vb2buf, 0);
	frame->addr.ioaddr[SC_PLANE_CB] = 0;
	frame->addr.ioaddr[SC_PLANE_CR] = 0;
	frame->addr.size[SC_PLANE_CB] = 0;
	frame->addr.size[SC_PLANE_CR] = 0;

	switch (frame->sc_fmt->num_comp) {
	case 1: /* rgb, yuyv */
		frame->addr.size[SC_PLANE_Y] = bytesize;
		break;
	case 2:
		if (frame->sc_fmt->num_planes == 1) {
			if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12N) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.ioaddr[SC_PLANE_CB] =
					NV12N_CBCR_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
				frame->addr.size[SC_PLANE_Y] = NV12N_Y_SIZE(w, h);
				frame->addr.size[SC_PLANE_CB] = NV12N_CBCR_SIZE(w, h);
			} else if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12N_10B) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.ioaddr[SC_PLANE_CB] =
					NV12N_10B_CBCR_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
				frame->addr.size[SC_PLANE_Y] = NV12N_Y_SIZE(w, h);
				frame->addr.size[SC_PLANE_CB] = NV12N_CBCR_SIZE(w, h);
			} else {
				if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12_P010)
					pixsize *= 2;
				frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_Y] + pixsize;
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = bytesize - pixsize;
			}
		} else if (frame->sc_fmt->num_planes == 2) {
			frame->addr.ioaddr[SC_PLANE_CB] = vb2_dma_sg_plane_dma_addr(vb2buf, 1);
			if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat))
				sc_calc_s10b_planesize(frame->sc_fmt->pixelformat,
						frame->width, frame->height,
						&frame->addr.size[SC_PLANE_Y],
						&frame->addr.size[SC_PLANE_CB], false);
			else
				sc_calc_planesize(frame, pixsize);
		}
		break;
	case 3:
		if (frame->sc_fmt->num_planes == 1) {
			if (sc_fmt_is_ayv12(frame->sc_fmt->pixelformat)) {
				unsigned int c_span;
				c_span = ALIGN(frame->width >> 1, 16);
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = c_span * (frame->height >> 1);
				frame->addr.size[SC_PLANE_CR] = frame->addr.size[SC_PLANE_CB];
				frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_Y] + pixsize;
				frame->addr.ioaddr[SC_PLANE_CR] = frame->addr.ioaddr[SC_PLANE_CB] + frame->addr.size[SC_PLANE_CB];
			} else if (frame->sc_fmt->pixelformat ==
					V4L2_PIX_FMT_YUV420N) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.size[SC_PLANE_Y] = YUV420N_Y_SIZE(w, h);
				frame->addr.size[SC_PLANE_CB] = YUV420N_CB_SIZE(w, h);
				frame->addr.size[SC_PLANE_CR] = YUV420N_CR_SIZE(w, h);
				frame->addr.ioaddr[SC_PLANE_CB] =
					YUV420N_CB_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
				frame->addr.ioaddr[SC_PLANE_CR] =
					YUV420N_CR_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
			} else {
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = (bytesize - pixsize) / 2;
				frame->addr.size[SC_PLANE_CR] = frame->addr.size[SC_PLANE_CB];
				frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_Y] + pixsize;
				frame->addr.ioaddr[SC_PLANE_CR] = frame->addr.ioaddr[SC_PLANE_CB] + frame->addr.size[SC_PLANE_CB];
			}
		} else if (frame->sc_fmt->num_planes == 3) {
			frame->addr.ioaddr[SC_PLANE_CB] = vb2_dma_sg_plane_dma_addr(vb2buf, 1);
			frame->addr.ioaddr[SC_PLANE_CR] = vb2_dma_sg_plane_dma_addr(vb2buf, 2);
			sc_calc_planesize(frame, pixsize);
		} else {
			dev_err(sc->dev, "Please check the num of comp\n");
		}
		break;
	default:
		break;
	}

	if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YVU420 ||
			frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YVU420M) {
		u32 t_cb = frame->addr.ioaddr[SC_PLANE_CB];
		frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_CR];
		frame->addr.ioaddr[SC_PLANE_CR] = t_cb;
	}

	sc_dbg("y addr %pa y size %#x\n", &frame->addr.ioaddr[SC_PLANE_Y], frame->addr.size[SC_PLANE_Y]);
	sc_dbg("cb addr %pa cb size %#x\n", &frame->addr.ioaddr[SC_PLANE_CB], frame->addr.size[SC_PLANE_CB]);
	sc_dbg("cr addr %pa cr size %#x\n", &frame->addr.ioaddr[SC_PLANE_CR], frame->addr.size[SC_PLANE_CR]);

	return 0;
}

static void sc_m2m_device_run(void *priv)
{
	struct sc_ctx *ctx = priv;
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *s_frame, *d_frame;

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	sc_get_bufaddr(sc, v4l2_m2m_next_src_buf(ctx->m2m_ctx), s_frame);
	sc_get_bufaddr(sc, v4l2_m2m_next_dst_buf(ctx->m2m_ctx), d_frame);

	sc_add_context_and_run(sc, ctx);
}

static void sc_m2m_job_abort(void *priv)
{
	struct sc_ctx *ctx = priv;
	int ret;

	ret = sc_ctx_stop_req(ctx);
	if (ret < 0)
		dev_err(ctx->sc_dev->dev, "wait timeout\n");
}

static struct v4l2_m2m_ops sc_m2m_ops = {
	.device_run	= sc_m2m_device_run,
	.job_abort	= sc_m2m_job_abort,
};

static void sc_unregister_m2m_device(struct sc_dev *sc)
{
	v4l2_m2m_release(sc->m2m.m2m_dev);
	video_device_release(sc->m2m.vfd);
	v4l2_device_unregister(&sc->m2m.v4l2_dev);
}

static int sc_register_m2m_device(struct sc_dev *sc, int dev_id)
{
	struct v4l2_device *v4l2_dev;
	struct device *dev;
	struct video_device *vfd;
	int ret = 0;

	dev = sc->dev;
	v4l2_dev = &sc->m2m.v4l2_dev;

	scnprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s.m2m",
			MODULE_NAME);

	ret = v4l2_device_register(dev, v4l2_dev);
	if (ret) {
		dev_err(sc->dev, "failed to register v4l2 device\n");
		return ret;
	}

	vfd = video_device_alloc();
	if (!vfd) {
		dev_err(sc->dev, "failed to allocate video device\n");
		goto err_v4l2_dev;
	}

	vfd->fops	= &sc_v4l2_fops;
	vfd->ioctl_ops	= &sc_v4l2_ioctl_ops;
	vfd->release	= video_device_release;
	vfd->lock	= &sc->lock;
	vfd->vfl_dir	= VFL_DIR_M2M;
	vfd->v4l2_dev	= v4l2_dev;
	scnprintf(vfd->name, sizeof(vfd->name), "%s:m2m", MODULE_NAME);

	video_set_drvdata(vfd, sc);

	sc->m2m.vfd = vfd;
	sc->m2m.m2m_dev = v4l2_m2m_init(&sc_m2m_ops);
	if (IS_ERR(sc->m2m.m2m_dev)) {
		dev_err(sc->dev, "failed to initialize v4l2-m2m device\n");
		ret = PTR_ERR(sc->m2m.m2m_dev);
		goto err_dev_alloc;
	}

	ret = video_register_device(vfd, VFL_TYPE_GRABBER, 50 + dev_id);
	if (ret) {
		dev_err(sc->dev, "failed to register video device (video%d)\n",
			50 + dev_id);
		goto err_m2m_dev;
	}

	return 0;

err_m2m_dev:
	v4l2_m2m_release(sc->m2m.m2m_dev);
err_dev_alloc:
	video_device_release(sc->m2m.vfd);
err_v4l2_dev:
	v4l2_device_unregister(v4l2_dev);

	return ret;
}

static int sc_m2m1shot_init_context(struct m2m1shot_context *m21ctx)
{
	struct sc_dev *sc = dev_get_drvdata(m21ctx->m21dev->dev);
	struct sc_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	atomic_inc(&sc->m2m.in_use);

	if (!IS_ERR(sc->pclk)) {
		ret = clk_prepare(sc->pclk);
		if (ret) {
			dev_err(sc->dev, "%s: failed to prepare PCLK(err %d)\n",
					__func__, ret);
			goto err_pclk;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_prepare(sc->aclk);
		if (ret) {
			dev_err(sc->dev, "%s: failed to prepare ACLK(err %d)\n",
					__func__, ret);
			goto err_aclk;
		}
	}

	ctx->context_type = SC_CTX_M2M1SHOT_TYPE;
	INIT_LIST_HEAD(&ctx->node);
	ctx->sc_dev = sc;

	ctx->s_frame.sc_fmt = &sc_formats[0];
	ctx->d_frame.sc_fmt = &sc_formats[0];

	m21ctx->priv = ctx;
	ctx->m21_ctx = m21ctx;

	INIT_DELAYED_WORK(&ctx->qos_work, sc_timeout_qos_work);
	ctx->pm_qos_lv = -1;

	return 0;
err_aclk:
	if (!IS_ERR(sc->pclk))
		clk_unprepare(sc->pclk);
err_pclk:
	kfree(ctx);
	return ret;
}

static int sc_m2m1shot_free_context(struct m2m1shot_context *m21ctx)
{
	struct sc_ctx *ctx = m21ctx->priv;

	atomic_dec(&ctx->sc_dev->m2m.in_use);
	if (!IS_ERR(ctx->sc_dev->aclk))
		clk_unprepare(ctx->sc_dev->aclk);
	if (!IS_ERR(ctx->sc_dev->pclk))
		clk_unprepare(ctx->sc_dev->pclk);
	BUG_ON(!list_empty(&ctx->node));
	destroy_intermediate_frame(ctx);
	if (ctx->framerate)
		flush_delayed_work(&ctx->qos_work);

	kfree(ctx);
	return 0;
}

static int sc_m2m1shot_prepare_format(struct m2m1shot_context *m21ctx,
			struct m2m1shot_pix_format *fmt,
			enum dma_data_direction dir,
			size_t bytes_used[])
{
	struct sc_ctx *ctx = m21ctx->priv;
	struct sc_frame *frame = (dir == DMA_TO_DEVICE) ?
					&ctx->s_frame : &ctx->d_frame;
	s32 size_min = (dir == DMA_TO_DEVICE) ? 16 : 4;
	int i;
	unsigned int ext_size = 0;

	frame->sc_fmt = sc_find_format(ctx->sc_dev, fmt->fmt,
						(dir == DMA_TO_DEVICE));
	if (!frame->sc_fmt) {
		dev_err(ctx->sc_dev->dev,
			"%s: Pixel format %#x is not supported for %s\n",
			__func__, fmt->fmt,
			(dir == DMA_TO_DEVICE) ? "output" : "capture");
		return -EINVAL;
	}

	if (!fmt->crop.width)
		fmt->crop.width = fmt->width;
	if (!fmt->crop.height)
		fmt->crop.height = fmt->height;

	if (!fmt->width || !fmt->height ||
				!fmt->crop.width || !fmt->crop.height) {
		dev_err(ctx->sc_dev->dev,
			"%s: neither width nor height can be zero\n",
			__func__);
		return -EINVAL;
	}

	if ((fmt->width > 8192) || (fmt->height > 8192)) {
		dev_err(ctx->sc_dev->dev,
			"%s: requested image size %dx%d exceed 8192x8192\n",
			__func__, fmt->width, fmt->height);
		return -EINVAL;
	}

	if ((fmt->crop.width < size_min) || (fmt->crop.height < size_min)) {
		dev_err(ctx->sc_dev->dev,
			"%s: image size %dx%d must not less than %dx%d\n",
			__func__, fmt->width, fmt->height, size_min, size_min);
		return -EINVAL;
	}

	if ((fmt->crop.left < 0) || (fmt->crop.top < 0)) {
		dev_err(ctx->sc_dev->dev,
			"%s: negative crop offset(%d, %d) is not supported\n",
			__func__, fmt->crop.left, fmt->crop.top);
		return -EINVAL;
	}

	if ((fmt->width < (fmt->crop.width + fmt->crop.left)) ||
		(fmt->height < (fmt->crop.height + fmt->crop.top))) {
		dev_err(ctx->sc_dev->dev,
			"%s: crop region(%d,%d,%d,%d) is larger than image\n",
			__func__, fmt->crop.left, fmt->crop.top,
			fmt->crop.width, fmt->crop.height);
		return -EINVAL;
	}

	if (frame->sc_fmt->h_shift) {
		unsigned int halign = 1 << frame->sc_fmt->h_shift;

		if (!IS_ALIGNED(fmt->width, halign) ||
				!IS_ALIGNED(fmt->crop.width, halign)) {
			dev_err(ctx->sc_dev->dev,
				"span(%d) or width(%d) is not aligned as %d\n",
				fmt->width, fmt->crop.width, halign);
			return -EINVAL;
		}
	}

	if (frame->sc_fmt->v_shift) {
		unsigned int valign = (1 << frame->sc_fmt->v_shift);

		if (!IS_ALIGNED(fmt->height, valign)) {
			dev_err(ctx->sc_dev->dev,
				"height(%d) is not aligned as %d\n",
				fmt->height, valign);
			return -EINVAL;
		}
	}

	if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat) &&
			!IS_ALIGNED(fmt->crop.width, 4)) {
		dev_err(ctx->sc_dev->dev,
			"%s: crop.width(%d) is not aligned as 4\n",
			__func__, fmt->crop.width);
		return -EINVAL;
	}

	for (i = 0; i < frame->sc_fmt->num_planes; i++) {
		if (sc_fmt_is_ayv12(fmt->fmt)) {
			unsigned int y_size, c_span;
			y_size = fmt->width * fmt->height;
			c_span = ALIGN(fmt->width / 2, 16);
			bytes_used[i] = y_size + (c_span * fmt->height / 2) * 2;
		} else {
			bytes_used[i] = fmt->width * fmt->height;
			bytes_used[i] *= frame->sc_fmt->bitperpixel[i];
			bytes_used[i] /= 8;
		}
		frame->bytesused[i] = bytes_used[i];
	}

	if (ctx->sc_dev->variant->extra_buf && dir == DMA_TO_DEVICE) {
		ext_size = sc_ext_buf_size(fmt->width);

		for (i = 0; i < frame->sc_fmt->num_planes; i++) {
			bytes_used[i] += (i == 0) ? ext_size : ext_size/2;
			frame->bytesused[i] = bytes_used[i];
		}
	}

	frame->width = fmt->width;
	frame->height = fmt->height;
	frame->crop = fmt->crop;

	return frame->sc_fmt->num_planes;
}

static int sc_m2m1shot_prepare_operation(struct m2m1shot_context *m21ctx,
						struct m2m1shot_task *task)
{
	struct sc_ctx *ctx = m21ctx->priv;
	struct m2m1shot *shot = &task->task;
	int ret;

	if (!sc_configure_rotation_degree(ctx, shot->op.rotate))
		return -EINVAL;

	ctx->flip_rot_cfg &= ~(SCALER_FLIP_X_EN | SCALER_FLIP_Y_EN);

	if (shot->op.op & M2M1SHOT_OP_FLIP_VIRT)
		ctx->flip_rot_cfg |= SCALER_FLIP_X_EN;

	if (shot->op.op & M2M1SHOT_OP_FLIP_HORI)
		ctx->flip_rot_cfg |= SCALER_FLIP_Y_EN;

	ctx->csc.csc_eq = !(shot->op.op & M2M1SHOT_OP_CSC_709) ?
			V4L2_COLORSPACE_SMPTE170M : V4L2_COLORSPACE_REC709;
	ctx->csc.csc_range = !(shot->op.op & M2M1SHOT_OP_CSC_WIDE) ?
						SC_CSC_NARROW : SC_CSC_WIDE;
	ctx->pre_multi = !!(shot->op.op & M2M1SHOT_OP_PREMULTIPLIED_ALPHA);

	ctx->dnoise_ft.strength = (shot->op.op & SC_M2M1SHOT_OP_FILTER_MASK) >>
					SC_M2M1SHOT_OP_FILTER_SHIFT;

	ret = sc_find_scaling_ratio(m21ctx->priv);
	if (ret)
		return ret;

	return sc_prepare_denoise_filter(m21ctx->priv);
}

/*
 * This function should be used for source buffer only.
 * In case of destination buffer, plane->bytes_used is not valid.
 */
static int sc_m2m1shot_check_src_plane_size(
			struct m2m1shot_buffer_plane_dma *plane,
			enum dma_data_direction dir, u32 min_size)
{
	if ((dir != DMA_TO_DEVICE) || !min_size)
		return 0;

	if (plane->dmabuf) {
		if (plane->bytes_used < min_size &&
				plane->dmabuf->size < min_size)
			return -EINVAL;
	} else {
		if (plane->bytes_used < min_size)
			return -EINVAL;
	}

	return 0;
}

static int sc_m2m1shot_prepare_buffer(struct m2m1shot_context *m21ctx,
			struct m2m1shot_buffer_dma *buf_dma,
			int plane,
			enum dma_data_direction dir)
{
	struct sc_ctx *ctx = m21ctx->priv;
	u32 min_size = ctx->sc_dev->variant->minsize_srcplane;
	int ret;

	ret = m2m1shot_map_dma_buf(m21ctx->m21dev->dev,
				&buf_dma->plane[plane], dir);
	if (ret)
		return ret;

	/* Plane size checking is needed for source buffer */
	if (dir == DMA_TO_DEVICE) {
		ret = sc_m2m1shot_check_src_plane_size(
				&buf_dma->plane[plane], dir, min_size);
		if (ret) {
			dev_err(ctx->sc_dev->dev,
				"plane%d size %d is smaller than %d\n",
				plane, buf_dma->plane[plane].bytes_used,
				min_size);
			m2m1shot_unmap_dma_buf(m21ctx->m21dev->dev,
					&buf_dma->plane[plane], dir);
			return ret;
		}
	}

	ret = m2m1shot_dma_addr_map(m21ctx->m21dev->dev, buf_dma, plane, dir);
	if (ret) {
		m2m1shot_unmap_dma_buf(m21ctx->m21dev->dev,
					&buf_dma->plane[plane], dir);
		return ret;
	}

	m2m1shot_sync_for_device(m21ctx->m21dev->dev,
				&buf_dma->plane[plane], dir);

	return 0;
}

static void sc_m2m1shot_finish_buffer(struct m2m1shot_context *m21ctx,
			struct m2m1shot_buffer_dma *buf_dma,
			int plane,
			enum dma_data_direction dir)
{
	m2m1shot_sync_for_cpu(m21ctx->m21dev->dev,
			      &buf_dma->plane[plane], dir);
	m2m1shot_dma_addr_unmap(m21ctx->m21dev->dev, buf_dma, plane);
	m2m1shot_unmap_dma_buf(m21ctx->m21dev->dev,
				&buf_dma->plane[plane], dir);
}

static void sc_m2m1shot_get_bufaddr(struct sc_dev *sc,
			struct m2m1shot_buffer_dma *buf, struct sc_frame *frame)
{
	unsigned int pixsize, bytesize;

	pixsize = frame->width * frame->height;
	bytesize = (pixsize * frame->sc_fmt->bitperpixel[0]) >> 3;

	frame->addr.ioaddr[SC_PLANE_Y] = buf->plane[0].dma_addr;

	frame->addr.ioaddr[SC_PLANE_CB] = 0;
	frame->addr.ioaddr[SC_PLANE_CR] = 0;
	frame->addr.size[SC_PLANE_CB] = 0;
	frame->addr.size[SC_PLANE_CR] = 0;

	switch (frame->sc_fmt->num_comp) {
	case 1: /* rgb, yuyv */
		frame->addr.size[SC_PLANE_Y] = bytesize;
		break;
	case 2:
		if (frame->sc_fmt->num_planes == 1) {
			if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12N) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.ioaddr[SC_PLANE_CB] =
					NV12N_CBCR_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
				frame->addr.size[SC_PLANE_Y] = NV12N_Y_SIZE(w, h);
				frame->addr.size[SC_PLANE_CB] = NV12N_CBCR_SIZE(w, h);
			} else if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12N_10B) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.ioaddr[SC_PLANE_CB] =
					NV12N_10B_CBCR_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
				frame->addr.size[SC_PLANE_Y] = NV12N_Y_SIZE(w, h);
				frame->addr.size[SC_PLANE_CB] = NV12N_CBCR_SIZE(w, h);
			} else {
				if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_NV12_P010)
					pixsize *= 2;
				frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_Y] + pixsize;
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = bytesize - pixsize;
			}
		} else if (frame->sc_fmt->num_planes == 2) {
			frame->addr.ioaddr[SC_PLANE_CB] = buf->plane[1].dma_addr;

			if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat))
				sc_calc_s10b_planesize(frame->sc_fmt->pixelformat,
						frame->width, frame->height,
						&frame->addr.size[SC_PLANE_Y],
						&frame->addr.size[SC_PLANE_CB], false);
			else
				sc_calc_planesize(frame, pixsize);
		}
		break;
	case 3:
		if (frame->sc_fmt->num_planes == 1) {
			if (sc_fmt_is_ayv12(frame->sc_fmt->pixelformat)) {
				unsigned int c_span;
				c_span = ALIGN(frame->width >> 1, 16);
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] =
					c_span * (frame->height >> 1);
				frame->addr.size[SC_PLANE_CR] = frame->addr.size[SC_PLANE_CB];
				frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_Y] + pixsize;
				frame->addr.ioaddr[SC_PLANE_CR] =
					frame->addr.ioaddr[SC_PLANE_CB] + frame->addr.size[SC_PLANE_CB];
			} else if (frame->sc_fmt->pixelformat ==
					V4L2_PIX_FMT_YUV420N) {
				unsigned int w = frame->width;
				unsigned int h = frame->height;
				frame->addr.size[SC_PLANE_Y] = YUV420N_Y_SIZE(w, h);
				frame->addr.size[SC_PLANE_CB] = YUV420N_CB_SIZE(w, h);
				frame->addr.size[SC_PLANE_CR] = YUV420N_CR_SIZE(w, h);
				frame->addr.ioaddr[SC_PLANE_CB] =
					YUV420N_CB_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
				frame->addr.ioaddr[SC_PLANE_CR] =
					YUV420N_CR_BASE(frame->addr.ioaddr[SC_PLANE_Y], w, h);
			} else {
				frame->addr.size[SC_PLANE_Y] = pixsize;
				frame->addr.size[SC_PLANE_CB] = (bytesize - pixsize) / 2;
				frame->addr.size[SC_PLANE_CR] = frame->addr.size[SC_PLANE_CB];
				frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_Y] + pixsize;
				frame->addr.ioaddr[SC_PLANE_CR] =
					frame->addr.ioaddr[SC_PLANE_CB] + frame->addr.size[SC_PLANE_CB];
			}
		} else if (frame->sc_fmt->num_planes == 3) {
			frame->addr.ioaddr[SC_PLANE_CB] = buf->plane[1].dma_addr;
			frame->addr.ioaddr[SC_PLANE_CR] = buf->plane[2].dma_addr;

			sc_calc_planesize(frame, pixsize);
		} else {
			dev_err(sc->dev, "Please check the num of comp\n");
		}
		break;
	default:
		break;
	}

	if (frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YVU420 ||
			frame->sc_fmt->pixelformat == V4L2_PIX_FMT_YVU420M) {
		u32 t_cb = frame->addr.ioaddr[SC_PLANE_CB];
		frame->addr.ioaddr[SC_PLANE_CB] = frame->addr.ioaddr[SC_PLANE_CR];
		frame->addr.ioaddr[SC_PLANE_CR] = t_cb;
	}
}

static int sc_m2m1shot_device_run(struct m2m1shot_context *m21ctx,
				struct m2m1shot_task *task)
{
	struct sc_ctx *ctx = m21ctx->priv;
	struct sc_dev *sc = ctx->sc_dev;
	struct sc_frame *s_frame, *d_frame;

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	if (task->task.reserved[0])
		sc_set_framerate(ctx, (int)task->task.reserved[0]);

	sc_m2m1shot_get_bufaddr(sc, &task->dma_buf_out, s_frame);
	sc_m2m1shot_get_bufaddr(sc, &task->dma_buf_cap, d_frame);

	return sc_add_context_and_run(sc, ctx);
}

static void sc_m2m1shot_timeout_task(struct m2m1shot_context *m21ctx,
		struct m2m1shot_task *task)
{
	struct sc_ctx *ctx = m21ctx->priv;
	struct sc_dev *sc = ctx->sc_dev;
	unsigned long flags;

	sc_hwregs_dump(sc);
	exynos_sysmmu_show_status(sc->dev);

	sc_hwset_soft_reset(sc);

	sc_clk_power_disable(sc);

	spin_lock_irqsave(&sc->ctxlist_lock, flags);
	sc->current_ctx = NULL;
	spin_unlock_irqrestore(&sc->ctxlist_lock, flags);

	clear_bit(DEV_RUN, &sc->state);
	clear_bit(CTX_RUN, &ctx->flags);

	sc_run_next_job(sc);
}

static const struct m2m1shot_devops sc_m2m1shot_ops = {
	.init_context = sc_m2m1shot_init_context,
	.free_context = sc_m2m1shot_free_context,
	.prepare_format = sc_m2m1shot_prepare_format,
	.prepare_operation = sc_m2m1shot_prepare_operation,
	.prepare_buffer = sc_m2m1shot_prepare_buffer,
	.finish_buffer = sc_m2m1shot_finish_buffer,
	.device_run = sc_m2m1shot_device_run,
	.timeout_task = sc_m2m1shot_timeout_task,
};

static int __attribute__((unused)) sc_sysmmu_fault_handler(struct iommu_domain *domain,
	struct device *dev, unsigned long iova, int flags, void *token)
{
	struct sc_dev *sc = dev_get_drvdata(dev);

	if (test_bit(DEV_RUN, &sc->state)) {
		dev_info(dev, "System MMU fault called for IOVA %#lx\n", iova);
		sc_hwregs_dump(sc);
	}

	return 0;
}

static int sc_clk_get(struct sc_dev *sc)
{
	sc->aclk = devm_clk_get(sc->dev, "gate");
	if (IS_ERR(sc->aclk)) {
		if (PTR_ERR(sc->aclk) != -ENOENT) {
			dev_err(sc->dev, "Failed to get 'gate' clock: %ld",
				PTR_ERR(sc->aclk));
			return PTR_ERR(sc->aclk);
		}
		dev_info(sc->dev, "'gate' clock is not present\n");
	}

	sc->pclk = devm_clk_get(sc->dev, "gate2");
	if (IS_ERR(sc->pclk)) {
		if (PTR_ERR(sc->pclk) != -ENOENT) {
			dev_err(sc->dev, "Failed to get 'gate2' clock: %ld",
				PTR_ERR(sc->pclk));
			return PTR_ERR(sc->pclk);
		}
		dev_info(sc->dev, "'gate2' clock is not present\n");
	}

	sc->clk_chld = devm_clk_get(sc->dev, "mux_user");
	if (IS_ERR(sc->clk_chld)) {
		if (PTR_ERR(sc->clk_chld) != -ENOENT) {
			dev_err(sc->dev, "Failed to get 'mux_user' clock: %ld",
				PTR_ERR(sc->clk_chld));
			return PTR_ERR(sc->clk_chld);
		}
		dev_info(sc->dev, "'mux_user' clock is not present\n");
	}

	if (!IS_ERR(sc->clk_chld)) {
		sc->clk_parn = devm_clk_get(sc->dev, "mux_src");
		if (IS_ERR(sc->clk_parn)) {
			dev_err(sc->dev, "Failed to get 'mux_src' clock: %ld",
				PTR_ERR(sc->clk_parn));
			return PTR_ERR(sc->clk_parn);
		}
	} else {
		sc->clk_parn = ERR_PTR(-ENOENT);
	}

	return 0;
}

static void sc_clk_put(struct sc_dev *sc)
{
	if (!IS_ERR(sc->clk_parn))
		clk_put(sc->clk_parn);

	if (!IS_ERR(sc->clk_chld))
		clk_put(sc->clk_chld);

	if (!IS_ERR(sc->pclk))
		clk_put(sc->pclk);

	if (!IS_ERR(sc->aclk))
		clk_put(sc->aclk);
}

#ifdef CONFIG_PM_SLEEP
static int sc_suspend(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);
	int ret;

	set_bit(DEV_SUSPEND, &sc->state);

	ret = wait_event_timeout(sc->wait,
			!test_bit(DEV_RUN, &sc->state), SC_TIMEOUT);
	if (ret == 0)
		dev_err(sc->dev, "wait timeout\n");

	return 0;
}

static int sc_resume(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);

	clear_bit(DEV_SUSPEND, &sc->state);

	sc_run_next_job(sc);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int sc_runtime_resume(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);

	if (!IS_ERR(sc->clk_chld) && !IS_ERR(sc->clk_parn)) {
		int ret = clk_set_parent(sc->clk_chld, sc->clk_parn);
		if (ret) {
			dev_err(sc->dev, "%s: Failed to setup MUX: %d\n",
				__func__, ret);
			return ret;
		}
	}

	if (sc->qosreq_int_level > 0)
		pm_qos_update_request(&sc->qosreq_int, sc->qosreq_int_level);

	return 0;
}

static int sc_runtime_suspend(struct device *dev)
{
	struct sc_dev *sc = dev_get_drvdata(dev);
	if (sc->qosreq_int_level > 0)
		pm_qos_update_request(&sc->qosreq_int, 0);
	return 0;
}
#endif

static const struct dev_pm_ops sc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sc_suspend, sc_resume)
	SET_RUNTIME_PM_OPS(NULL, sc_runtime_resume, sc_runtime_suspend)
};

/* sort qos table in descending order of freq_int */
static int sc_compare_qos_table_entries(const void *p1, const void *p2)
{
	const struct sc_qos_table *t1 = p1, *t2 = p2;

	if (t1->freq_int < t2->freq_int)
		return 1;
	else
		return -1;

	return 0;
}

static int sc_populate_dt(struct sc_dev *sc)
{
	struct device *dev = sc->dev;
	struct sc_qos_table *qos_table;
	struct sc_ppc_table *ppc_table;
	int i, len;

	len = of_property_count_u32_elems(dev->of_node, "mscl_qos_table");
	if (len < 0) {
		dev_info(dev, "No qos table for scaler\n");
		return 0;
	}

	sc->qos_table_cnt = len / 3;

	qos_table = devm_kzalloc(dev, sizeof(struct sc_qos_table) * sc->qos_table_cnt, GFP_KERNEL);
	if (!qos_table)
		return -ENOMEM;

	of_property_read_u32_array(dev->of_node, "mscl_qos_table",
					(unsigned int *)qos_table, len);

	sort(qos_table, sc->qos_table_cnt, sizeof(*qos_table),
					sc_compare_qos_table_entries, NULL);

	for (i = 0; i < sc->qos_table_cnt; i++) {
		dev_info(dev, "MSCL QoS Table[%d] mif : %u int : %u [%u]\n", i,
			qos_table[i].freq_mif,
			qos_table[i].freq_int,
			qos_table[i].data_size);
	}

	sc->qos_table = qos_table;

	len = of_property_count_u32_elems(dev->of_node, "mscl_ppc_table");
	if (len < 0) {
		dev_info(dev, "No ppc table for scaler\n");
		return 0;
	}

	sc->ppc_table_cnt = len / 3;

	ppc_table = devm_kzalloc(dev,
			sizeof(*ppc_table) * sc->ppc_table_cnt, GFP_KERNEL);
	if (!ppc_table)
		return -ENOMEM;

	of_property_read_u32_array(dev->of_node, "mscl_ppc_table",
					(unsigned int *)ppc_table, len);

	for (i = 0; i < sc->ppc_table_cnt; i++) {
		dev_info(dev, "MSCL PPC Table[%d] bpp : %u ppc : %u/%u\n", i,
			ppc_table[i].bpp,
			ppc_table[i].ppc[0],
			ppc_table[i].ppc[1]);
	}

	sc->ppc_table = ppc_table;
	return 0;
}

#ifdef CONFIG_EXYNOS_ITMON
static bool sc_itmon_check(struct sc_dev *sc, char *str_itmon, char *str_attr)
{
	const char *name = NULL;

	if (!str_itmon)
		return false;

	of_property_read_string(sc->dev->of_node, str_attr, &name);
	if (!name)
		return false;

	if (strncmp(str_itmon, name, strlen(name)) == 0)
		return true;

	return false;
}

static int sc_itmon_notifier(struct notifier_block *nb,
			unsigned long action, void *nb_data)
{
	struct sc_dev *sc = container_of(nb, struct sc_dev, itmon_nb);
	struct itmon_notifier *itmon_info = nb_data;
	static int called_count;

	if (called_count != 0) {
		dev_info(sc->dev, "%s is called %d times, ignore it.\n",
				__func__, called_count);
		return NOTIFY_DONE;
	}

	if (sc_itmon_check(sc, itmon_info->master, "itmon,master")) {
		if (test_bit(DEV_RUN, &sc->state)) {
			if (sc->current_ctx)
				sc_ctx_dump(sc->current_ctx);
			sc_hwregs_dump(sc);
			exynos_sysmmu_show_status(sc->dev);
		} else {
			dev_info(sc->dev, "MSCL is not running!\n");
		}
		called_count++;
	} else if (sc_itmon_check(sc, itmon_info->dest, "itmon,dest")) {
		if (test_bit(DEV_RUN, &sc->state)) {
			if (sc->current_ctx)
				sc_ctx_dump(sc->current_ctx);
			if (itmon_info->onoff) {
				sc_hwregs_dump(sc);
				exynos_sysmmu_show_status(sc->dev);
			} else {
				dev_info(sc->dev, "MSCL power is disabled!\n");
			}
		} else {
			dev_info(sc->dev, "MSCL is not running!\n");
		}
		called_count++;
	}

	return NOTIFY_DONE;
}
#endif

static int sc_probe(struct platform_device *pdev)
{
	struct sc_dev *sc;
	struct resource *res;
	int ret = 0;
	size_t ivar;
	u32 hwver;

	sc = devm_kzalloc(&pdev->dev, sizeof(struct sc_dev), GFP_KERNEL);
	if (!sc) {
		dev_err(&pdev->dev, "no memory for scaler device\n");
		return -ENOMEM;
	}

	sc->dev = &pdev->dev;

	spin_lock_init(&sc->ctxlist_lock);
	INIT_LIST_HEAD(&sc->context_list);
	spin_lock_init(&sc->slock);
	mutex_init(&sc->lock);
	init_waitqueue_head(&sc->wait);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sc->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sc->regs))
		return PTR_ERR(sc->regs);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "failed to get IRQ resource\n");
		return -ENOENT;
	}

	ret = devm_request_irq(&pdev->dev, res->start, sc_irq_handler, 0,
			pdev->name, sc);
	if (ret) {
		dev_err(&pdev->dev, "failed to install irq\n");
		return ret;
	}

	dma_set_mask(&pdev->dev, DMA_BIT_MASK(36));

	atomic_set(&sc->wdt.cnt, 0);
	setup_timer(&sc->wdt.timer, sc_watchdog, (unsigned long)sc);

	ret = sc_clk_get(sc);
	if (ret)
		return ret;

	if (pdev->dev.of_node)
		sc->dev_id = of_alias_get_id(pdev->dev.of_node, "scaler");
	else
		sc->dev_id = pdev->id;

	sc->m21dev = m2m1shot_create_device(&pdev->dev, &sc_m2m1shot_ops,
						"scaler", sc->dev_id, -1);
	if (IS_ERR(sc->m21dev)) {
		dev_err(&pdev->dev, "%s: Failed to create m2m1shot_device\n",
			__func__);
		return PTR_ERR(sc->m21dev);
	}

	platform_set_drvdata(pdev, sc);

	pm_runtime_enable(&pdev->dev);

	ret = sc_populate_dt(sc);
	if (ret)
		return ret;

	ret = sc_register_m2m_device(sc, sc->dev_id);
	if (ret) {
		dev_err(&pdev->dev, "failed to register m2m device\n");
		goto err_m2m;
	}

#if defined(CONFIG_PM_DEVFREQ)
	if (!of_property_read_u32(pdev->dev.of_node, "mscl,int_qos_minlock",
				(u32 *)&sc->qosreq_int_level)) {
		if (sc->qosreq_int_level > 0) {
			pm_qos_add_request(&sc->qosreq_int,
						PM_QOS_DEVICE_THROUGHPUT, 0);
			dev_info(&pdev->dev, "INT Min.Lock Freq. = %u\n",
						sc->qosreq_int_level);
		}
	}
#endif
	if (of_property_read_u32(pdev->dev.of_node, "mscl,cfw",
				(u32 *)&sc->cfw))
		sc->cfw = 0;

	ret = iovmm_activate(sc->dev);
	if (ret) {
		dev_err(&pdev->dev, "failed to activate iommu\n");
		goto err_iommu;
	}

	ret = pm_runtime_get_sync(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: failed to local power on (err %d)\n",
			__func__, ret);
		goto err_ver_rpm_get;
	}

	if (!IS_ERR(sc->pclk)) {
		ret = clk_prepare_enable(sc->pclk);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: failed to enable PCLK (err %d)\n",
				__func__, ret);
			goto err_ver_pclk_get;
		}
	}

	if (!IS_ERR(sc->aclk)) {
		ret = clk_prepare_enable(sc->aclk);
		if (ret) {
			dev_err(&pdev->dev,
				"%s: failed to enable ACLK (err %d)\n",
				__func__, ret);
			goto err_ver_aclk_get;
		}
	}

	sc->version = SCALER_VERSION(2, 0, 0);

	hwver = __raw_readl(sc->regs + SCALER_VER);

	/* selects the lowest version number if no version is matched */
	for (ivar = 0; ivar < ARRAY_SIZE(sc_version_table); ivar++) {
		sc->version = sc_version_table[ivar][1];
		if (hwver == sc_version_table[ivar][0])
			break;
	}

	for (ivar = 0; ivar < ARRAY_SIZE(sc_variant); ivar++) {
		if (sc->version >= sc_variant[ivar].version) {
			sc->variant = &sc_variant[ivar];
			break;
		}
	}

	if (!IS_ERR(sc->aclk))
		clk_disable_unprepare(sc->aclk);
	if (!IS_ERR(sc->pclk))
		clk_disable_unprepare(sc->pclk);
	pm_runtime_put(&pdev->dev);

	iovmm_set_fault_handler(&pdev->dev, sc_sysmmu_fault_handler, sc);

#ifdef CONFIG_EXYNOS_ITMON
	sc->itmon_nb.notifier_call = sc_itmon_notifier;
	itmon_notifier_chain_register(&sc->itmon_nb);
#endif

	dev_info(&pdev->dev,
		"Driver probed successfully(version: %08x(%x))\n",
		hwver, sc->version);

	return 0;

err_ver_aclk_get:
	if (!IS_ERR(sc->pclk))
		clk_disable_unprepare(sc->pclk);
err_ver_pclk_get:
	pm_runtime_put(&pdev->dev);
err_ver_rpm_get:
	iovmm_deactivate(sc->dev);
err_iommu:
	if (sc->qosreq_int_level > 0)
		pm_qos_remove_request(&sc->qosreq_int);
	sc_unregister_m2m_device(sc);
err_m2m:
	m2m1shot_destroy_device(sc->m21dev);

	return ret;
}

static int sc_remove(struct platform_device *pdev)
{
	struct sc_dev *sc = platform_get_drvdata(pdev);

	iovmm_deactivate(sc->dev);

	sc_clk_put(sc);

	if (timer_pending(&sc->wdt.timer))
		del_timer(&sc->wdt.timer);

	m2m1shot_destroy_device(sc->m21dev);

	if (sc->qosreq_int_level > 0)
		pm_qos_remove_request(&sc->qosreq_int);

	return 0;
}

static void sc_shutdown(struct platform_device *pdev)
{
	struct sc_dev *sc = platform_get_drvdata(pdev);

	set_bit(DEV_SUSPEND, &sc->state);

	wait_event(sc->wait,
			!test_bit(DEV_RUN, &sc->state));

	iovmm_deactivate(sc->dev);
}

static const struct of_device_id exynos_sc_match[] = {
	{
		.compatible = "samsung,exynos5-scaler",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_sc_match);

static struct platform_driver sc_driver = {
	.probe		= sc_probe,
	.remove		= sc_remove,
	.shutdown	= sc_shutdown,
	.driver = {
		.name	= MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &sc_pm_ops,
		.of_match_table = of_match_ptr(exynos_sc_match),
	}
};

module_platform_driver(sc_driver);

MODULE_AUTHOR("Sunyoung, Kang <sy0816.kang@samsung.com>");
MODULE_DESCRIPTION("EXYNOS m2m scaler driver");
MODULE_LICENSE("GPL");
