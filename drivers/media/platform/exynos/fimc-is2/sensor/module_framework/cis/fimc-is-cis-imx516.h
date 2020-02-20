/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CIS_IMX516_H
#define FIMC_IS_CIS_IMX516_H

#include "fimc-is-cis.h"

#define EXT_CLK_Mhz (26)

/* FIXME */
#define SENSOR_IMX516_MAX_WIDTH    (1280)
#define SENSOR_IMX516_MAX_HEIGHT   (482)

enum sensor_imx516_mode_enum {
	SENSOR_IMX516_1280x3840_30FPS = 0,  /* VGA : 640 * 2, 480 * 8 */
	SENSOR_IMX516_640x1920_30FPS,       /* QVGA : 320 * 2, 240 * 8  */
	SENSOR_IMX516_MODE_MAX,
};

#ifdef USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION
/* merge into sensor driver */
enum {
	CAM_IMX516_SET_A_REAR_DEFAULT_TX_CLOCK = 0, /* Default - 100Mhz */
	CAM_IMX516_SET_A_REAR_TX_99p9_MHZ = 0, /* 100Mhz */
	CAM_IMX516_SET_A_REAR_TX_95p8_MHZ = 1, /* 96Mhz */
	CAM_IMX516_SET_A_REAR_TX_102p6_MHZ = 2, /* 101Mhz */
};

#endif

int sensor_imx516_cis_get_uid_index(struct fimc_is_cis *cis, u32 mode, u32 *uid_index);
#endif

