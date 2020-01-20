/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_2L2_H
#define FIMC_IS_CIS_2L2_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_2L2_MAX_WIDTH		(4032 + 0)
#define SENSOR_2L2_MAX_HEIGHT		(3024 + 0)

#define SENSOR_2L2_FINE_INTEGRATION_TIME_MIN                0x100
#define SENSOR_2L2_FINE_INTEGRATION_TIME_MAX                0x100
#define SENSOR_2L2_COARSE_INTEGRATION_TIME_MIN              0x04
#define SENSOR_2L2_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x08

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_2l2_mode_enum {
	SENSOR_2L2_8064X3024_30FPS = 0,
	SENSOR_2L2_8064X2268_30FPS,
	SENSOR_2L2_6048X3024_30FPS,
	SENSOR_2L2_4032X2268_60FPS,
	SENSOR_2L2_2016X1134_120FPS,
	SENSOR_2L2_2016X1134_240FPS,
	SENSOR_2L2_1008X756_120FPS,
	SENSOR_2L2_2016X1512_30FPS,
	SENSOR_2L2_1504X1504_30FPS,
	SENSOR_2L2_2016X1134_30FPS,
	SENSOR_2L2_8064X1960_30FPS,
};

#endif

