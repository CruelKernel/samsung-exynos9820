/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_FLASH_H
#define FIMC_IS_FLASH_H


static int control_flash_gpio(u32 gpio, int val) {
	int ret = 0;

	__gpio_set_value(gpio, val);

	return ret;
}

#endif
