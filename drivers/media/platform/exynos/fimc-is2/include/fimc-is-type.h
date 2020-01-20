/*
* Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is vender functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_TYPE_H
#define FIMC_IS_TYPE_H

#include <linux/media-bus-format.h>
#include <media/v4l2-device.h>

enum fimc_is_device_type {
	FIMC_IS_DEVICE_SENSOR,
	FIMC_IS_DEVICE_ISCHAIN,
	FIMC_IS_DEVICE_MAX
};

struct fimc_is_window {
	u32 o_width;
	u32 o_height;
	u32 width;
	u32 height;
	u32 offs_h;
	u32 offs_v;
	u32 otf_width;
	u32 otf_height;
};

struct fimc_is_fmt {
	char				*name;
	u32				mbus_code;
	u32				pixelformat;
	u32				field;
	u32				num_planes;
	u32				hw_format;
	u32				hw_order;
	u32				hw_bitwidth;
	u32				hw_plane;
	u8				bitsperpixel[VIDEO_MAX_PLANES];
};

struct fimc_is_image {
	u32			framerate;
	struct fimc_is_window	window;
	struct fimc_is_fmt	format;
};

struct fimc_is_crop {
	u32			x;
	u32			y;
	u32			w;
	u32			h;
};

#define INIT_CROP(c) ((c)->x = 0, (c)->y = 0, (c)->w = 0, (c)->h = 0)
#define IS_NULL_CROP(c) !((c)->x + (c)->y + (c)->w + (c)->h)
#define COMPARE_CROP(c1, c2) (((c1)->x == (c2)->x) && ((c1)->y == (c2)->y) && ((c1->w) == (c2)->w) && ((c1)->h == (c2)->h))
#define TRANS_CROP(d, s) ((d)[0] = (s)[0], (d)[1] = (s)[1], (d)[2] = (s)[2], (d)[3] = (s)[3])
#define COMPARE_FORMAT(f1, f2) ((f1) == (f2))

#define TO_WORD_OFFSET(byte_offset) ((byte_offset) >> 2)

#define CONVRES(src, src_max, tar_max) \
	((src <= 0) ? (0) : ((src * tar_max + (src_max >> 1)) / src_max))

#endif
