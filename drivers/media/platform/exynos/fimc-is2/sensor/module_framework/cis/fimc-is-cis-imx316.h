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

#ifndef FIMC_IS_CIS_IMX316_H
#define FIMC_IS_CIS_IMX316_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

/* FIXME */
#define SENSOR_IMX316_MAX_WIDTH    (480)
#define SENSOR_IMX316_MAX_HEIGHT   (182)

enum sensor_imx316_mode_enum {
	SENSOR_IMX316_480X180_120FPS,
};

#endif

