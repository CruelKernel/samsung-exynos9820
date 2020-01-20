/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_2P2_H
#define FIMC_IS_CIS_2P2_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_2L1_MAX_WIDTH		(4032 + 0)
#define SENSOR_2L1_MAX_HEIGHT		(3024 + 0)

#define SENSOR_2L1_FINE_INTEGRATION_TIME_MIN                0x100
#define SENSOR_2L1_FINE_INTEGRATION_TIME_MAX                0x100
#define SENSOR_2L1_COARSE_INTEGRATION_TIME_MIN              0x05
#define SENSOR_2L1_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x08

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_2l1_mode_enum {
	SENSOR_2L1_8064X3024_30FPS = 0,
	SENSOR_2L1_8064X2268_30FPS,
	SENSOR_2L1_6048X3024_30FPS,
	SENSOR_2L1_4032X2268_60FPS,
	SENSOR_2L1_2016X1134_120FPS,
	SENSOR_2L1_2016X1134_240FPS,
	SENSOR_2L1_1008X756_120FPS,
	SENSOR_2L1_2016X1512_30FPS,
	SENSOR_2L1_1504X1504_30FPS,
	SENSOR_2L1_2016X1134_30FPS,
};

#endif

