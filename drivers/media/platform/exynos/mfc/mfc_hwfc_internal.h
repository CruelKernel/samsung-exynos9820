/*
 * drivers/media/platform/exynos/mfc/mfc_hwfc_internal.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_HWFC_INTERNAL_H
#define __MFC_HWFC_INTERNAL_H __FILE__

#include "mfc_common.h"

/*
 * RGB encoding information to avoid confusion.
 *
 * V4L2_PIX_FMT_ARGB32 takes ARGB data like below.
 * MSB                              LSB
 * 3       2       1
 * 2       4       6       8       0
 * |B......BG......GR......RA......A|
 */
struct mfc_fmt enc_hwfc_formats[] = {
	{
		.name = "4:2:0 2 Planes Y/CbCr single",
		.fourcc = V4L2_PIX_FMT_NV12N,
		.codec_mode = MFC_FORMATS_NO_CODEC,
		.type = MFC_FMT_FRAME,
		.num_planes = 2,
		.mem_planes = 1,
	},
	{
		.name = "H264 Encoded Stream",
		.fourcc = V4L2_PIX_FMT_H264,
		.codec_mode = MFC_REG_CODEC_H264_ENC,
		.type = MFC_FMT_STREAM,
		.num_planes = 1,
		.mem_planes = 1,
	},
	{
		.name = "HEVC Encoded Stream",
		.fourcc = V4L2_PIX_FMT_HEVC,
		.codec_mode = MFC_REG_CODEC_HEVC_ENC,
		.type = MFC_FMT_STREAM,
		.num_planes = 1,
		.mem_planes = 1,
	},
};

#define NUM_FORMATS ARRAY_SIZE(enc_hwfc_formats)

#endif /* __MFC_HWFC_INTERNAL_H */
