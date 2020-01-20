/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for processing SCore driver interrupts
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_INTERFACE_H__
#define __SCORE_INTERFACE_H__

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>

#include "score_frame.h"

struct score_system;

/**
 * struct score_interface - Object processing interrupt delivered to APCPU
 * @dev: device structure registered in platform
 * @system: object about score_system structure
 * @irq0: interrupt number
 * @sfr: kernel virtual address matched with sfr base address of SCore device
 * @framemgr: object about score_frame_manager structure
 */
struct score_interface {
	struct device			*dev;
	struct score_system		*system;
	int				irq0;
	void __iomem			*sfr;
	struct score_frame_manager	framemgr;

#if defined(CONFIG_EXYNOS_SCORE_FPGA)
	struct kthread_worker		isr_worker;
	struct task_struct		*isr_task;
	struct kthread_work		isr_work;
#endif
};

int score_interface_target_halt(struct score_interface *itf, int core_id);
int score_interface_open(struct score_interface *itf);
void score_interface_close(struct score_interface *itf);

int score_interface_probe(struct score_system *system);
void score_interface_remove(struct score_interface *interface);

#endif
