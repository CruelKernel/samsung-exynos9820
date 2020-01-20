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

#ifndef FIMC_IS_CIS_SR259_H
#define FIMC_IS_CIS_SR259_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (24.37)

#define SENSOR_SR259_MAX_WIDTH		(1616 + 16)
#define SENSOR_SR259_MAX_HEIGHT		(1212 + 16)

/* TODO: Check below values are valid */
#define SENSOR_SR259_FINE_INTEGRATION_TIME_MIN                0x1C5
#define SENSOR_SR259_FINE_INTEGRATION_TIME_MAX                0x1C5
#define SENSOR_SR259_COARSE_INTEGRATION_TIME_MIN              0x1
#define SENSOR_SR259_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x4

#define SENSOR_SETFILE_PAGE_SELECT_ADDR		0x03

#define USE_GROUP_PARAM_HOLD	(1)

enum sr259_fixed_framerate {
	SR259_7_FIXED_FPS = 7,
	SR259_15_FIXED_FPS = 15,
	SR259_30_FIXED_FPS = 30,
};

#endif

