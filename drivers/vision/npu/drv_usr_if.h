/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef _DRV_USR_IF_H_
#define _DRV_USR_IF_H_

/* Current supporting user API version */
#define USER_API_VERSION	3

struct drv_usr_share {
	unsigned int id;
	int ncp_fd;
	unsigned int ncp_size;
	unsigned long ncp_mmap;
};

#endif /* _DRV_USR_IF_H_ */
