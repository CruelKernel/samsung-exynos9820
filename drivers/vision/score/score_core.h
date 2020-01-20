/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for managing score_context and score file operations
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_CORE_H__
#define __SCORE_CORE_H__

#include <linux/device.h>
#include <linux/miscdevice.h>

#include "score_system.h"
#include "score_ioctl.h"

/* Information about SCore device file */
#define SCORE_DEV_NAME_LEN		16
#define SCORE_DEV_NAME			"score"

/*
 * Max number of context that can be allocated at SCore driver
 * (TODO this is temporal value)
 */
#define SCORE_MAX_CONTEXT		64

struct score_device;

/**
 * struct score_dev - Device data to register SCore device
 * @minor: minor device number of SCore driver
 * @name: name of SCore device file
 * @miscdev: object about miscdevice structure
 * @cdev: object about cdev structure
 * @parent: parent device
 *
 * TODO reorganize device data structure (dev, miscdev, cdev, parent)
 */
struct score_dev {
	int				minor;
	char				name[SCORE_DEV_NAME_LEN];

	struct miscdevice		miscdev;
	struct cdev			*cdev;
	struct device			*parent;
};

/**
 * struct score_core - Controller of file operations for SCore device
 * @sdev: device data to register SCore device
 * @device: object about score_device
 * @system: object about score_system
 * @ctx_list: list of total contexts
 * @ctx_count: count of total contexts
 * @ctx_slock: spinlock to manage list and count of context
 * @wait_time: max time that context can wait result response (unit : ms)
 * @ctx_map: map to allocate context id
 * @device_lock: mutex to prevent device is being opened or closed
		 at the same time
 * @ioctl_ops: ioctl operations
 * @dpmu_print: select dpmu to be printed always for debugging
 */
struct score_core {
	struct score_dev		sdev;
	struct score_device		*device;
	struct score_system		*system;
	struct list_head		ctx_list;
	unsigned int			ctx_count;
	spinlock_t			ctx_slock;
	unsigned int			wait_time;
	DECLARE_BITMAP(ctx_map, SCORE_MAX_CONTEXT);
	struct mutex			device_lock;
	const struct score_ioctl_ops	*ioctl_ops;
	bool				dpmu_print;
};

int score_core_probe(struct score_device *device);
void score_core_remove(struct score_core *core);

#endif
