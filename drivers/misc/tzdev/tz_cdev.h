/*
 * Copyright (C) 2016 Samsung Electronics, Inc.
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

#ifndef __TZ_CDEV_H__
#define __TZ_CDEV_H__

#include <linux/cdev.h>
#include <linux/device.h>

struct tz_cdev {
	const char *name;
	struct module *owner;
	const struct file_operations *fops;
	struct device *device;
	struct cdev cdev;
	struct class *class;
	dev_t dev;
};

int tz_cdev_register(struct tz_cdev *cdev);
void tz_cdev_unregister(struct tz_cdev *cdev);

#endif /* __TZ_CDEV_H__ */
