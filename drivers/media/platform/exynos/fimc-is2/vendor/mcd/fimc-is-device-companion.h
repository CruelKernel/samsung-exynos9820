/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FIMC_IS_DEVICE_COMPANION_H
#define FIMC_IS_DEVICE_COMPANION_H

#define FIMC_IS_COMPANION_CFG(w, h, f, m) {	\
	.width		= w,			\
	.height		= h,			\
	.framerate	= f,			\
	.mode		= m,			\
}

struct fimc_is_companion_cfg {
	u32 width;
	u32 height;
	u32 framerate;
	int mode;
};

int fimc_is_comp_get_mode(struct v4l2_subdev *subdev,
		struct fimc_is_device_sensor *device, u16 *mode_select);
#endif
