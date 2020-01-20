/*
 * Samsung Exynos5 SoC series FIMC-IS EEPROM driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct fimc_is_device_eeprom {
	struct v4l2_device			v4l2_dev;
	struct platform_device			*pdev;
	unsigned long				state;
	struct exynos_platform_fimc_is_sensor	*pdata;
	struct i2c_client			*client;
	struct fimc_is_core			*core;
	int					driver_data;
};
