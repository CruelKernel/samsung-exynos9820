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

#ifndef FIMC_IS_PAFSTAT_H
#define FIMC_IS_PAFSTAT_H

#include "fimc-is-device-sensor.h"
#include "fimc-is-interface-sensor.h"

#define MAX_NUM_OF_PAFSTAT 2

int pafstat_register(struct fimc_is_module_enum *module, int pafstat_ch);
int pafstat_unregister(struct fimc_is_module_enum *module);
int fimc_is_pafstat_reset_recovery(struct v4l2_subdev *subdev, u32 reset_mode, int pd_mode);

#endif
