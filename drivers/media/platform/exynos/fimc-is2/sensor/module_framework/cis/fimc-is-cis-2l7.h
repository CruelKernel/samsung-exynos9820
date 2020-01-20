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

#ifndef FIMC_IS_CIS_2L7_H
#define FIMC_IS_CIS_2L7_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_2L7_MAX_WIDTH		(4016 + 16)
#define SENSOR_2L7_MAX_HEIGHT		(3012 + 12)

#define SENSOR_2L7_FINE_INTEGRATION_TIME_MIN                0x100
#define SENSOR_2L7_FINE_INTEGRATION_TIME_MAX                0x100
#define SENSOR_2L7_COARSE_INTEGRATION_TIME_MIN              0x05
#define SENSOR_2L7_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x08

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_2l7_mode_enum {
	SENSOR_2L7_4032X3024_30FPS_MODE3 = 0,
	SENSOR_2L7_4032X2268_30FPS_MODE3,
	SENSOR_2L7_2016X1512_30FPS_MODE3,
	SENSOR_2L7_2016X1136_30FPS_MODE3,
};

#endif

