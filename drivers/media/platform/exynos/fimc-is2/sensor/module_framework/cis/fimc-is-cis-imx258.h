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

#ifndef FIMC_IS_CIS_IMX258_H
#define FIMC_IS_CIS_IMX258_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (24.37)

#define SENSOR_IMX258_MAX_WIDTH		(4128 + 16)
#define SENSOR_IMX258_MAX_HEIGHT		(3096 + 10)

/* TODO: Check below values are valid */
#define SENSOR_IMX258_FINE_INTEGRATION_TIME_MIN                0x54C
#define SENSOR_IMX258_FINE_INTEGRATION_TIME_MAX                0x54C
#define SENSOR_IMX258_COARSE_INTEGRATION_TIME_MIN              0x1
#define SENSOR_IMX258_COARSE_INTEGRATION_TIME_MAX_MARGIN       0xA

#define USE_GROUP_PARAM_HOLD	(0)

#endif

