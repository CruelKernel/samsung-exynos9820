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

#ifndef FIMC_IS_CIS_4H5YC_H
#define FIMC_IS_CIS_4H5YC_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_4H5YC_MAX_WIDTH		(3264 + 16)
#define SENSOR_4H5YC_MAX_HEIGHT		(2448 + 10)

/* TODO: Check below values are valid */
#define SENSOR_4H5YC_FINE_INTEGRATION_TIME_MIN                0x1C5
#define SENSOR_4H5YC_FINE_INTEGRATION_TIME_MAX                0x1C5
#define SENSOR_4H5YC_COARSE_INTEGRATION_TIME_MIN              0x1
#define SENSOR_4H5YC_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x4

#define USE_GROUP_PARAM_HOLD	(0)

#endif

