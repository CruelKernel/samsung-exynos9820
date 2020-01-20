/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for controlling SCore driver connected with APCPU driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_SYSFS_H__
#define __SCORE_SYSFS_H__

#include <linux/device.h>
#include "score_device.h"

int score_sysfs_probe(struct score_device *device);
void score_sysfs_remove(struct score_device *device);

#endif
