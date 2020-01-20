/*
* Samsung Exynos5 SoC series Camera PostProcessing driver
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CAMERAPP_VIDEO_H
#define CAMERAPP_VIDEO_H

#include <linux/videodev2.h>

/* video node */
#define CAMERAPP_VIDEONODE_GDC	55
#define CAMERAPP_VIDEONODE_STR  56

#define EXYNOS_VIDEONODE_CAMERAPP(x)		(EXYNOS_VIDEONODE_FIMC_IS + x)


/* related Data format */
enum camerapp_pixel_size {
	CAMERAPP_PIXEL_SIZE_8BIT = 0,
	CAMERAPP_PIXEL_SIZE_10BIT,
	CAMERAPP_PIXEL_SIZE_PACKED_10BIT,
	CAMERAPP_PIXEL_SIZE_8_2BIT,
};

#endif
