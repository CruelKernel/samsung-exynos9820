/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __TZ_PLATFORM_H__
#define __TZ_PLATFORM_H__

#include <linux/file.h>

#include "tzdev.h"

int tzdev_platform_register(void);
void tzdev_platform_unregister(void);
int tzdev_platform_init(void);
int tzdev_platform_fini(void);
int tzdev_platform_open(struct inode *inode, struct file *filp);
int tzdev_platform_close(struct inode *inode, struct file *filp);
long tzdev_fd_platform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int tzdev_fd_platform_close(struct inode *inode, struct file *filp);
int tzdev_platform_smc_call(struct tzdev_smc_data *data);
uint32_t tzdev_platform_get_sysconf_flags(void);

#endif /* __TZ_PLATFORM_H__ */
