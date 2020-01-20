/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Header file for Exynos REPEATER driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _REPEATER_H_
#define _REPEATER_H_

#define MAX_SHARED_BUFFER_NUM		3

struct repeater_info {
	int pixel_format;
	int width;
	int height;
	int buffer_count;
	int fps;
	int buf_fd[MAX_SHARED_BUFFER_NUM];
};

#define REPEATER_IOCTL_MAGIC		'R'

#define REPEATER_IOCTL_MAP_BUF		\
	_IOWR(REPEATER_IOCTL_MAGIC, 0x10, struct repeater_info)
#define REPEATER_IOCTL_UNMAP_BUF	\
	_IO(REPEATER_IOCTL_MAGIC, 0x11)

#define REPEATER_IOCTL_START		\
	_IO(REPEATER_IOCTL_MAGIC, 0x20)
#define REPEATER_IOCTL_STOP		\
	_IO(REPEATER_IOCTL_MAGIC, 0x21)
#define REPEATER_IOCTL_PAUSE	\
	_IO(REPEATER_IOCTL_MAGIC, 0x22)
#define REPEATER_IOCTL_RESUME	\
	_IO(REPEATER_IOCTL_MAGIC, 0x23)

#define REPEATER_IOCTL_DUMP		\
	_IOR(REPEATER_IOCTL_MAGIC, 0x31, int)

#endif /* _REPEATER_H_ */
