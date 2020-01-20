/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_CONTROL_SENSOR_H
#define FIMC_IS_CONTROL_SENSOR_H

#include <linux/workqueue.h>

#define NUM_OF_FRAME_30FPS	(1)
#define NUM_OF_FRAME_60FPS	(2)
#define NUM_OF_FRAME_120FPS	(4)
#define NUM_OF_FRAME_240FPS	(12)
#define NUM_OF_FRAME_480FPS	(16)

#define CAM2P0_UCTL_LIST_SIZE   (NUM_OF_FRAME_480FPS + 1)	/* This value must be larger than NUM_OF_FRAME */
#define EXPECT_DM_NUM		(CAM2P0_UCTL_LIST_SIZE)

struct gain_setting {
	u32 sensitivity;
	u32 long_again;
	u32 short_again;
	u32 long_dgain;
	u32 short_dgain;
};

/* Helper function */
u64 fimc_is_sensor_convert_us_to_ns(u32 usec);
u32 fimc_is_sensor_convert_ns_to_us(u64 nsec);

struct fimc_is_device_sensor;
void fimc_is_sensor_ctl_frame_evt(struct fimc_is_device_sensor *device);
void fimc_is_sensor_ois_update(struct fimc_is_device_sensor *device);
#ifdef USE_OIS_SLEEP_MODE
void fimc_is_sensor_ois_start(struct fimc_is_device_sensor *device);
void fimc_is_sensor_ois_stop(struct fimc_is_device_sensor *device);
#endif
int fimc_is_sensor_ctl_adjust_sync(struct fimc_is_device_sensor *device, u32 adjust_sync);
int fimc_is_sensor_ctl_low_noise_mode(struct fimc_is_device_sensor *device, u32 mode);

/* Actuator funtion */
int fimc_is_actuator_ctl_set_position(struct fimc_is_device_sensor *device, u32 position);
int fimc_is_actuator_ctl_convert_position(u32 *pos,
				u32 src_max_pos, u32 src_direction,
				u32 tgt_max_pos, u32 tgt_direction);
int fimc_is_actuator_ctl_search_position(u32 position,
				u32 *position_table,
				u32 direction,
				u32 *searched_pos);
#endif
