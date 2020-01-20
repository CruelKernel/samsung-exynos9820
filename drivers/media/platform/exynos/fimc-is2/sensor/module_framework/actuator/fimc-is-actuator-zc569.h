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

#ifndef FIMC_IS_DEVICE_ZC569_H
#define FIMC_IS_DEVICE_ZC569_H

#define ACTUATOR_NAME		"ZC569"

#define ZC569_PRODUCT_ID		0x0E

#define ZC569_POS_SIZE_BIT		ACTUATOR_POS_SIZE_9BIT
#define ZC569_POS_MAX_SIZE		((1 << ZC569_POS_SIZE_BIT) - 1)
#define ZC569_POS_DIRECTION		ACTUATOR_RANGE_INF_TO_MAC
#define ZC569_REG_POS_HIGH		0x00
#define ZC569_REG_POS_LOW		0x01

#define ZC569_MAX_PRODUCT_LIST		2

#endif
