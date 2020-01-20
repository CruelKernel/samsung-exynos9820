/******************************************************************************
 * Copyright (C) 2013 Broadcom Corporation
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *****************************************************************************
 */

#ifndef _BCM_GPS_HOSTWAKE_H_
#define _BCM_GPS_HOSTWAKE_H_

struct bcm_gps_hostwake_platform_data {
	unsigned int gpio_hostwake; /* HOST_WAKE : to indicate that ASIC has an geofence event. */
};

/*
 *example : arch/board_specific_file.c
 *
 *#define GPS_HOSTWAKE_GPIO (139)
 *
 *static struct bcm_gps_hostwake_platform_data gps_hostwake_data = {
 *    .gpio_hostwake = GPS_HOSTWAKE_GPIO,
 *};
 *
 *static struct platform_device bcm_gps_hostwake = {
 *        .name	= "bcm-gps-hostwake",
 *        .id	= -1,
 *        .dev	= {
 *                .platform_data	= &gps_hostwake_data,
 *        },
 *};
 *
 *
 *{
 *...
 *    platform_device_register(&bcm_gps_hostwake);
 *...
 *}
 */




#endif // _BCM_GPS_HOSTWAKE_H_
