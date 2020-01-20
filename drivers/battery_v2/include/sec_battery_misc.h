/*
*  sec_battery_misc.h
*  Samsung Mobile Battery Misc Driver
*  Author: Yeongmi Ha <yeongmi86.ha@samsung.com>
*  Copyright (C) 2018 Samsung Electronics
*
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef __LINUX_SEC_BATTERY_MISC_H__
#define __LINUX_SEC_BATTERY_MISC_H__

// Samsung Wireless Authentication Message
enum swam_data_type {
	TYPE_SHORT = 0,
	TYPE_LONG,
};

enum swam_direction_type {
	DIR_OUT = 0,
	DIR_IN,
};

struct swam_data {
	unsigned short pid; /* Product ID */
	char type; /* swam_data_type */
	char dir; /* swam_direction_type */
	unsigned int size; /* data size */
	void __user *pData; /* data pointer */
};

struct sec_bat_misc_dev {
	struct swam_data u_data;
	atomic_t open_excl;
	atomic_t ioctl_excl;
	int (*swam_write)(void *data, int size);
	int (*swam_read)(void *data);
	int (*swam_ready)(void);
	void (*swam_close)(void);
};

#define sec_bat_misc_dev_t \
	struct sec_bat_misc_dev

#endif
