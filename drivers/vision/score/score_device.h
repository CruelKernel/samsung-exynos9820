/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for controlling SCore driver connected with APCPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_DEVICE_H__
#define __SCORE_DEVICE_H__

#include <linux/device.h>
#include <linux/platform_device.h>

#include "score_system.h"
#include "score_pm.h"
#include "score_clk.h"
#include "score_core.h"

/*
 * enum score_device_state - State of SCore driver by APCPU device
 * @SCORE_DEVICE_STATE_OPEN:
 *	open is state that booting of SCore device is completed
 * @SCORE_DEVICE_STATE_START:
 *	start is state that command is being delivered to SCore device
 * @SCORE_DEVICE_STATE_STOP:
 *	stop is state that all task is done of SCore device
 * @SCORE_DEVICE_STATE_CLOSE:
 *	close is state that SCore device stopped
 * @SCORE_DEVICE_STATE_SUSPEND:
 *	suspend is state that APCPU is stopping when SCore device doesn't stop
 */
enum score_device_state {
	SCORE_DEVICE_STATE_OPEN,
	SCORE_DEVICE_STATE_START,
	SCORE_DEVICE_STATE_STOP,
	SCORE_DEVICE_STATE_CLOSE,
	SCORE_DEVICE_STATE_SUSPEND
};

/*
 * struct score_device - Object including SCore driver connected with APCPU
 * @dev: device structure registered in platform
 * @pdev: platform device structure allocated in platform_driver_register
 * @open_count: the number of user that opens SCore device
 * @state: state of SCore driver by APCPU device
 * @cur_firmware_id:
 * @system: object including data of SCore driver about SCore H/W
 * @pm: object about power manager
 * @clk: object about clock manager
 * @core: object about file operations controller
 */
struct score_device {
	struct device			*dev;
	struct platform_device		*pdev;
	atomic_t			open_count;
	unsigned int			state;
	unsigned int			cur_firmware_id;

	struct score_system		system;
	struct score_pm			pm;
	struct score_clk		clk;
	struct score_core		core;
};

int score_device_check_start(struct score_device *device);
int score_device_open(struct score_device *device);
int score_device_start(struct score_device *device, unsigned int firmware_id,
		unsigned int boot_qos, unsigned int flag);
void score_device_get(struct score_device *device);
void score_device_put(struct score_device *device);

#endif
