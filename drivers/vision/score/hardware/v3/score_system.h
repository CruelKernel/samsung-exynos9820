/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for controlling SCore driver connected directly with SCore H/W
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_SYSTEM_H__
#define __SCORE_SYSTEM_H__

#include <linux/platform_device.h>
#include <linux/device.h>

#include "score_interface.h"
#include "score_mmu.h"
#include "score_firmware.h"
#include "score_scq.h"
#include "score_sm.h"
#include "score_sr.h"
#include "score_print.h"

struct score_device;

/* check again after delay time. the unit is microsecond */
#define CHECK_RETRY_DELAY (10)
/* check again 'retry count' times. */
#define CHECK_RETRY_COUNT   (1000/CHECK_RETRY_DELAY)

/**
 * struct score_system - Object controlling SCore H/W IP
 * @dev: device structure registered in platform
 * @interface: object about score_interface structure
 * @mem: object about score_mmu structure
 * @fwmgr: object about score_frame manager structure
 * @scq: object about score_scq structure
 * @sm: object about score_sm structure
 * @sr: object about score_sr structure
 * @print: object about score_print structure
 * @sfr: kernel virtual address matched with sfr base address of SCore device
 * @sfr_size: sfr area size of SCore deice
 * @cache_slock: spin lock to prevent simultaneous control
 */
struct score_system {
	struct device				*dev;
	struct score_interface			interface;
	struct score_mmu			mmu;
	struct score_firmware_manager		fwmgr;
	struct score_scq			scq;
	struct score_sm				sm;
	struct score_sr				sr;
	struct score_print			print;

	void __iomem				*sfr;
	resource_size_t				sfr_size;
	spinlock_t				cache_slock;
};

enum score_system_dcache_command {
	DCACHE_CMD_START = 0,
	DCACHE_INVALIDATE,
	DCACHE_CLEAN,
	DCACHE_FLUSH,
	DCACHE_CMD_END,
};

int score_system_dcache_control(struct score_system *system, int id, int cmd);
int score_system_dcache_control_all(struct score_system *system, int cmd);

int score_system_halt(struct score_system *system);
int score_system_sw_reset(struct score_system *system);
void score_system_wakeup(struct score_system *system);
int score_system_boot(struct score_system *system);

int score_system_start(struct score_system *system, unsigned int firmware_id);
void score_system_stop(struct score_system *system);

int score_system_open(struct score_system *system);
void score_system_close(struct score_system *system);

int score_system_probe(struct score_device *device);
void score_system_remove(struct score_system *system);

#endif
