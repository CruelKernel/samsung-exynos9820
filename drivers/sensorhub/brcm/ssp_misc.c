/*
 *  Copyright (C) 2014, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"

#define	SHAKE_ON	1
#define	SHAKE_OFF	2

struct remove_af_noise {
	void *af_pdata;
	int16_t (*af_func)(void *, bool);
};

static struct remove_af_noise af_sensor;

int remove_af_noise_register(struct remove_af_noise *af_cam)
{
	if (af_cam->af_pdata)
		af_sensor.af_pdata = af_cam->af_pdata;

	if (af_cam->af_func)
		af_sensor.af_func = af_cam->af_func;

	if (!af_cam->af_pdata || !af_cam->af_func) {
		ssp_dbg("[SSP]: %s - no af struct\n", __func__);
		return ERROR;
	}

	return 0;
}
EXPORT_SYMBOL(remove_af_noise_register);

void remove_af_noise_unregister(struct remove_af_noise *af_cam)
{
	af_sensor.af_pdata = NULL;
	af_sensor.af_func = NULL;
}
EXPORT_SYMBOL(remove_af_noise_unregister);

void report_shake_cam_data(struct ssp_data *data, int sensor_type,
	struct sensor_value *shake_cam_data)
{
	data->buf[SHAKE_CAM].x = shake_cam_data->x;

	ssp_dbg("[SSP]: %s - shake = %d\n", __func__,
		(char)data->buf[SHAKE_CAM].x);

	if (likely(af_sensor.af_pdata && af_sensor.af_func)) {
		if ((char)data->buf[SHAKE_CAM].x == SHAKE_ON)
			af_sensor.af_func(af_sensor.af_pdata, true);
		else if ((char)data->buf[SHAKE_CAM].x == SHAKE_OFF)
			af_sensor.af_func(af_sensor.af_pdata, false);
	} else {
		ssp_dbg("[SSP]: %s - no af_func\n", __func__);
	}
}
