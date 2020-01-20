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

#ifndef FIMC_IS_CIS_2P7SQ_H
#define FIMC_IS_CIS_2P7SQ_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

#define SENSOR_2P7SQ_MAX_WIDTH		(4624 + 16)		/* 4640 */
#define SENSOR_2P7SQ_MAX_HEIGHT		(3476 + 12)		/* 3488 */

/* TODO: Check below values are valid */
#define SENSOR_2P7SQ_FINE_INTEGRATION_TIME_MIN                0x200
#define SENSOR_2P7SQ_FINE_INTEGRATION_TIME_MAX                0x200
#define SENSOR_2P7SQ_COARSE_INTEGRATION_TIME_MIN              0x4
#define SENSOR_2P7SQ_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x6

#define USE_GROUP_PARAM_HOLD	(1)

#endif

