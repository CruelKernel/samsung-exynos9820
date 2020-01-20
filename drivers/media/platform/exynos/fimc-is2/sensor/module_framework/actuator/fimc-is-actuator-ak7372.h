/*
 * Samsung Exynos SoC series Actuator driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_AK7372_H
#define FIMC_IS_DEVICE_AK7372_H

#define AK7372_PRODUCT_ID		0xC
#define AK7372_POS_SIZE_BIT		ACTUATOR_POS_SIZE_9BIT
#define AK7372_POS_MAX_SIZE		((1 << AK7372_POS_SIZE_BIT) - 1)
#define AK7372_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC
#define AK7372_REG_POS_HIGH		0x00
#define AK7372_REG_POS_LOW		0x01

#endif
