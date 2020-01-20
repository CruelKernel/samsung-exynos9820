/*
 *  Copyright (C) 2016, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef __SSP_DUMP_H__
#define __SSP_DUMP_H__

#include "ssp.h"

//#define SENSOR_DUMP_FILE_STORE

/* dumpregister size*/
#define DUMPREGISTER_SIZE_ACCELEROMTER 101
#define DUMPREGISTER_SIZE_GEOMAGNETIC_FIELD	52
#if defined(CONFIG_SENSORS_SSP_LPS22H)
#define DUMPREGISTER_SIZE_PRESSURE 44
#else
#define DUMPREGISTER_SIZE_PRESSURE 58
#endif
#if defined(CONFIG_SENSORS_SSP_TMD3700) || defined(CONFIG_SENSORS_SSP_TMD3725)
#define DUMPREGISTER_SIZE_PROXIMITY	103
#else
#define DUMPREGISTER_SIZE_PROXIMITY	127
#endif


#ifdef SENSOR_DUMP_FILE_STORE
#define SENSOR_DUMP_PATH		"/data/log/sensor_dump_"
#define SENSOR_DUMP_FILE_LENGTH		100
#endif

#define DUMPREGISTER_MAX_SIZE		DUMPREGISTER_SIZE_PROXIMITY
#define NUM_LINE_ITEM			16
#define LENGTH_1BYTE_HEXA_WITH_BLANK	3	/* example : 'ff '*/
#define LENGTH_EXTRA_STRING				2	/* '\n\0' */
#define LENGTH_SENSOR_NAME_MAX			30
#define LENGTH_SENSOR_TYPE_MAX			3	/*	SENSOR_MAX digits = 3 */

#define sensor_dump_length(x)  (x*LENGTH_1BYTE_HEXA_WITH_BLANK+(x/NUM_LINE_ITEM)*(LENGTH_EXTRA_STRING-1) + 1)

#define SENSOR_DUMP_SENSOR_LIST		{ACCELEROMETER_SENSOR, \
					GYROSCOPE_SENSOR, \
					GEOMAGNETIC_UNCALIB_SENSOR, \
					PRESSURE_SENSOR,\
					PROXIMITY_SENSOR, \
					LIGHT_SENSOR}
#endif
