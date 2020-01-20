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

#ifndef FIMC_IS_CIS_4HA_H
#define FIMC_IS_CIS_4HA_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_4HA_MAX_WIDTH		(3104 + 0)
#define SENSOR_4HA_MAX_HEIGHT		(2326 + 0)

/* TODO: Check below values are valid */
#define SENSOR_4HA_FINE_INTEGRATION_TIME_MIN                0x1C5
#define SENSOR_4HA_FINE_INTEGRATION_TIME_MAX                0x1C5
#define SENSOR_4HA_COARSE_INTEGRATION_TIME_MIN              0x4
#define SENSOR_4HA_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x6

#define SENSOR_4HA_EXPOSURE_TIME_MAX						100000 /* 100ms */

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_4ha_mode_enum {
	SENSOR_4HA_3104X2326_24FPS = 0,
	SENSOR_4HA_2744X2056_30FPS,
	SENSOR_4HA_3104X1746_24FPS,
	SENSOR_4HA_3104X1746_30FPS,
	SENSOR_4HA_3104X1472_24FPS,
	SENSOR_4HA_2328X2328_24FPS,
	SENSOR_4HA_768X576_120FPS,
};
#endif

