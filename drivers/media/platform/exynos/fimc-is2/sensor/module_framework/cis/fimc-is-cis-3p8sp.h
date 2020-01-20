/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_3P8SP_H
#define FIMC_IS_CIS_3P8SP_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_3P8SP_MAX_WIDTH		(4592 + 16)
#define SENSOR_3P8SP_MAX_HEIGHT		(3478 + 10)

/* TODO: Check below values are valid */
#define SENSOR_3P8SP_FINE_INTEGRATION_TIME_MIN                0x618
#define SENSOR_3P8SP_FINE_INTEGRATION_TIME_MAX                0x618
#define SENSOR_3P8SP_COARSE_INTEGRATION_TIME_MIN              0x06
#define SENSOR_3P8SP_COARSE_INTEGRATION_TIME_MAX_MARGIN       0xE33

#define USE_GROUP_PARAM_HOLD	(0)

#endif

