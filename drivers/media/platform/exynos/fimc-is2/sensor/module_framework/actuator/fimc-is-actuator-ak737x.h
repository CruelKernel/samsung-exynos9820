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

#ifndef FIMC_IS_DEVICE_AK737X_H
#define FIMC_IS_DEVICE_AK737X_H

#define ACTUATOR_NAME		"AK737X"

#define AK737X_PRODUCT_ID_AK7371		0x09
#define AK737X_PRODUCT_ID_AK7372		0x0C
#define AK737X_PRODUCT_ID_AK7374		0x0E

#define AK737X_POS_SIZE_BIT		ACTUATOR_POS_SIZE_9BIT
#define AK737X_POS_MAX_SIZE		((1 << AK737X_POS_SIZE_BIT) - 1)
#define AK737X_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC
#define AK737X_REG_POS_HIGH		0x00
#define AK737X_REG_POS_LOW		0x01

#define AK737X_MAX_PRODUCT_LIST		6

#endif
