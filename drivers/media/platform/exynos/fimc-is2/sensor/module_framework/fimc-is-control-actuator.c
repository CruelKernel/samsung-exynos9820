/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>

#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>

#include "fimc-is-control-sensor.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"

int fimc_is_actuator_ctl_set_position(struct fimc_is_device_sensor *device,
					u32 position)
{
	int ret = 0;
	struct v4l2_control v4l2_ctrl;

	FIMC_BUG(!device);

	v4l2_ctrl.id = V4L2_CID_ACTUATOR_SET_POSITION;
	v4l2_ctrl.value = position;

	ret = fimc_is_sensor_s_ctrl(device, &v4l2_ctrl);
	if (ret < 0) {
		err("Actuator set position fail\n");
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_actuator_ctl_convert_position(u32 *pos,
				u32 src_max_pos, u32 src_direction,
				u32 tgt_max_pos, u32 tgt_direction)
{
	int ret = 0;
	u32 convert_pos = *pos;

	if (convert_pos >= (1 << src_max_pos)) {
		err("Actuator convert position size error\n");
		ret = -EINVAL;
		goto p_err;
	}

	if ((src_direction > ACTUATOR_RANGE_MAC_TO_INF) ||
			(tgt_direction > ACTUATOR_RANGE_MAC_TO_INF)) {
		err("Actuator convert direction error\n");
		ret = -EINVAL;
		goto p_err;
	}

	/* Convert bitage */
	if (src_max_pos < tgt_max_pos)
		convert_pos <<= (tgt_max_pos - src_max_pos);
	else if (src_max_pos > tgt_max_pos)
		convert_pos >>= (src_max_pos - tgt_max_pos);

	/* Convert Direction */
	if (src_direction != tgt_direction)
		convert_pos = ((1 << tgt_max_pos) - 1) - convert_pos;

	*pos = convert_pos;

p_err:

	return ret;
}

int fimc_is_actuator_ctl_search_position(u32 position,
		u32 *position_table,
		enum fimc_is_actuator_direction direction,
		u32 *searched_pos)
{
	int middle = 0, left = 0;
	int right = ACTUATOR_MAX_FOCUS_POSITIONS - 1;

	FIMC_BUG(!position_table);
	FIMC_BUG(!searched_pos);

	*searched_pos = 0;

	while (right >= left) {
		middle = (right + left) >> 1;

		if ((middle < 0) || (middle >= ACTUATOR_MAX_FOCUS_POSITIONS)) {
			err("%s: Invalid search argument\n", __func__);
			return -EINVAL;
		}

		if (position == position_table[middle]) {
			*searched_pos = (u32)middle;
			return 0;
		}

		if (direction == ACTUATOR_RANGE_INF_TO_MAC) {
			if (position < position_table[middle])
				right = middle - 1;
			else
				left = middle + 1;
		} else {
			if (position > position_table[middle])
				right = middle - 1;
			else
				left = middle + 1;
		}
	}

	warn("No item in array! HW pos: %d(Closest pos: real %d, virtual %d)\n",
			position, position_table[middle], middle);
	*searched_pos = (u32)middle;

	return 0;
}

enum hrtimer_restart fimc_is_actuator_m2m_af_set(struct hrtimer *timer)
{
	struct fimc_is_actuator_data *actuator_data = container_of(timer, struct fimc_is_actuator_data, afwindow_timer);
	struct fimc_is_actuator *actuator;
	struct fimc_is_device_sensor *device;
	struct fimc_is_device_sensor_peri *sensor_peri;

	actuator = container_of(actuator_data, struct fimc_is_actuator, actuator_data);
	FIMC_BUG(!actuator);

	sensor_peri = actuator->sensor_peri;
	FIMC_BUG(!sensor_peri);

	device = v4l2_get_subdev_hostdata(sensor_peri->subdev_actuator);
	FIMC_BUG(!device);

	/* Setting M2M AF position */
	fimc_is_sensor_peri_call_m2m_actuator(device);

	actuator_data->timer_check = HRTIMER_POSSIBLE;

	return HRTIMER_NORESTART;
}

int fimc_is_actuator_notify_m2m_actuator(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	u32 left_x, left_y, right_x, right_y;

	u32 af_window_ratio = 0;
	u32 virtual_image_size = 0;
	ulong timer_setting = 0;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("%s, module in is NULL", __func__);
		return -EINVAL;
	}

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;
	FIMC_BUG(!sensor_peri);

	left_x = sensor_peri->actuator->left_x;
	left_y = sensor_peri->actuator->left_y;
	right_x = sensor_peri->actuator->right_x;
	right_y = sensor_peri->actuator->right_y;

	/*
	 *  valid_time : usec
	 *  timer_setting : usec
	 *  right_x,y : virtual coordinates
	 *  VIRTUAL_COORDINATE : Virtual coordinate to image
	 */
	af_window_ratio = (right_y * VIRTUAL_COORDINATE_WIDTH) + right_x;
	virtual_image_size = (VIRTUAL_COORDINATE_WIDTH * VIRTUAL_COORDINATE_HEIGHT) / 1000;
	timer_setting = (sensor_peri->actuator->valid_time * (af_window_ratio / virtual_image_size)) / 1000;

	/*
	 * If not finish previous hrtimer work that's
	 * have problem hrtimer or sensor working
	 */
	if (sensor_peri->actuator->actuator_data.timer_check != HRTIMER_POSSIBLE)
		err("have problem of hrtimer, check set the time");

	sensor_peri->actuator->actuator_data.timer_check = HRTIMER_IMPOSSIBLE;

	/* timer msec setting */
	hrtimer_start(&sensor_peri->actuator->actuator_data.afwindow_timer,
			ktime_set(0, (timer_setting / 1000) * NSEC_PER_MSEC), HRTIMER_MODE_REL);

	return ret;
}
