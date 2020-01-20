/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_RPB_H
#define FIMC_IS_CIS_RPB_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_RPB_MAX_WIDTH		(4656)
#define SENSOR_RPB_MAX_HEIGHT		(3520)

/* TODO: Check below values are valid */
#define SENSOR_RPB_FINE_INTEGRATION_TIME_MIN                0x200
#define SENSOR_RPB_FINE_INTEGRATION_TIME_MAX                0x200
#define SENSOR_RPB_COARSE_INTEGRATION_TIME_MIN              0x5
#define SENSOR_RPB_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x5

#define USE_GROUP_PARAM_HOLD	(0)

#endif

