/*
 * Samsung Exynos5 SoC series FIMC-IS AF driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

struct fimc_is_af_gpio {
	char *sda;
	char *scl;
	char *pinname;
};

struct remove_af_noise {
	void	 *af_pdata;
	int16_t (*af_func)(void *, bool);
};

struct fimc_is_device_af {
	struct v4l2_device				v4l2_dev;
	struct platform_device				*pdev;
	unsigned long					state;
	struct exynos_platform_fimc_is_sensor		*pdata;
	struct i2c_client			*client;
	struct fimc_is_core		*core;
	struct remove_af_noise	af_sensor_interface;
	int af_noise_count;
};

int fimc_is_af_i2c_read(struct i2c_client *client, u16 addr, u16 *data);
int fimc_is_af_i2c_write(struct i2c_client *client ,u8 addr, u8 data);
int16_t fimc_is_af_enable(void *device, bool onoff);
int16_t fimc_is_af_move_lens(struct fimc_is_core *core);
#ifdef CAMERA_REAR2_AF
int16_t fimc_is_af_move_lens_rear2(struct fimc_is_core *core);
#endif
