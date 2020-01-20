/*
 *  Copyright (C) 2017, Samsung Electronics Co. Ltd. All Rights Reserved.
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

#ifndef FINGERPRINTSYSFS_H_
#define FINGERPRINTSYSFS_H_

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>

void set_fingerprint_attr(struct device *dev,
	struct device_attribute *attributes[]);
int fingerprint_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
void fingerprint_unregister(struct device *dev,
	struct device_attribute *attributes[]);
void destroy_fingerprint_class(void);

#endif
