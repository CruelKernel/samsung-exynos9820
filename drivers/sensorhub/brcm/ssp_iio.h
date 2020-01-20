/*
 *  Copyright (C) 2015, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __SSP_IIO_H__
#define __SSP_IIO_H__

#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/events.h>
#include <linux/iio/kfifo_buf.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/types.h>
#include "ssp.h"


#define IIO_CHANNEL		-1
#define IIO_SCAN_INDEX		3
#define IIO_SIGN		's'
#define IIO_SHIFT		0

#define META_EVENT		0
#define META_TIMESTAMP		0

#define PROX_AVG_READ_NUM	80

struct sensor_value;

void report_sensor_data(struct ssp_data *, int, struct sensor_value *);
int initialize_indio_dev(struct ssp_data *data);
void remove_indio_dev(struct ssp_data *data);
short thermistor_rawToTemperature(struct ssp_data *data, int type, s16 raw);

#endif
