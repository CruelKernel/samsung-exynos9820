/*
 * Samsung Exynos5 SoC series Actuator driver
 *
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_AK7371_H
#define FIMC_IS_DEVICE_AK7371_H

#define AK7371_PRODUCT_ID		0x9
#define AK7371_POS_SIZE_BIT		ACTUATOR_POS_SIZE_9BIT
#define AK7371_POS_MAX_SIZE		((1 << AK7371_POS_SIZE_BIT) - 1)
#define AK7371_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC
#define AK7371_REG_POS_HIGH		0x00
#define AK7371_REG_POS_LOW		0x01

#endif
