/*
 * Samsung Exynos9 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_COMMON_ENUM_H
#define FIMC_IS_COMMON_ENUM_H

enum camera_pixel_size {
	CAMERA_PIXEL_SIZE_8BIT = 0,
	CAMERA_PIXEL_SIZE_10BIT,
	CAMERA_PIXEL_SIZE_PACKED_10BIT,
	CAMERA_PIXEL_SIZE_8_2BIT,
	CAMERA_PIXEL_SIZE_12BIT_COMP
};

#endif
