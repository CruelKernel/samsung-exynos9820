/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_4E5_H
#define FIMC_IS_DEVICE_4E5_H

#define SENSOR_S5K4EC_INSTANCE	0
#define SENSOR_S5K4EC_NAME	SENSOR_NAME_S5K4EC
#define SENSOR_S5K4EC_DRIVING

enum {
	AUTO_FOCUS_FAILED,
	AUTO_FOCUS_DONE,
	AUTO_FOCUS_CANCELLED,
};

enum af_operation_status {
	AF_NONE = 0,
	AF_START,
	AF_CANCEL,
};

enum s5k4ecgx_runmode {
	S5K4ECGX_RUNMODE_NOTREADY,
	S5K4ECGX_RUNMODE_IDLE,
	S5K4ECGX_RUNMODE_RUNNING,
	S5K4ECGX_RUNMODE_CAPTURE,
};

struct fimc_is_module_4ec {
	struct fimc_is_image	image;

	u16			vis_duration;
	u16			frame_length_line;
	u32			line_length_pck;
	u32			system_clock;

	u32			mode;
	u32			contrast;
	u32			effect;
	u32			ev;
	u32			flash_mode;
	u32			focus_mode;
	u32			iso;
	u32			metering;
	u32			saturation;
	u32			scene_mode;
	u32			sharpness;
	u32			white_balance;
	u32			fps;
	u32			aeawb_lockunlock;
	u32			zoom_ratio;

	enum s5k4ecgx_runmode		runmode;
	enum af_operation_status	af_status;
};

int sensor_4ec_probe(struct i2c_client *client,
	const struct i2c_device_id *id);

#endif

