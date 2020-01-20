/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_LC898217_H
#define FIMC_IS_DEVICE_LC898217_H

#define LC898217_POS_SIZE_BIT		ACTUATOR_POS_SIZE_10BIT
#define LC898217_POS_MAX_SIZE		((1 << LC898217_POS_SIZE_BIT) - 1)
#define LC898217_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC
#define LC898217_REG_POS_HIGH		0x00
#define LC898217_REG_POS_LOW		0x01

#endif
