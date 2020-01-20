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

#ifndef NPU_INTERFACE_H_
#define NPU_INTERFACE_H

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include "ncp_header.h"
#include "npu-log.h"
#include "npu-util-llq.h"
#include "interface/loopback/npu-mailbox.h"
#include "npu-util-liststatemgr.h"

#define LOW_ADDR    0x100
#define NOR_ADDR    0x111
#define HIGH_ADDR   0x112

#define TRY_CNT     100
#define QSIZE       100

#define enter_request_barrier(frame) mutex_lock(frame->lock);
#define exit_request_barrier(frame) mutex_unlock(frame->lock);
#define init_process_barrier(itf) spin_lock_init(&itf->process_barrier);
#define enter_process_barrier(itf) spin_lock_irq(&itf->process_barrier);
#define exit_process_barrier(itf) spin_unlock_irq(&itf->process_barrier);


/**
  * @brief	NCP Interface structure.
  */
struct npu_interface {
	struct ncp_header	*ncp_header;

	void	__iomem		*regs;
	resource_size_t		regs_size;
	spinlock_t			process_barrier;
	/**
	  * Normally, struct npu_mailbox used to be contained in private_data when sending interrupt,
	  * and when receiving private_data, it contains npu_mailbox.
	  */
	void				*private_data;
};

int npu_interface_probe(struct npu_interface *interface,
						struct device *dev,
						void __iomem *code,
						resource_size_t code_size,
						void __iomem *regs,
						resource_size_t regs_size,
						u32 irq0, u32 irq1);
int npu_interface_open(struct npu_interface *interface);
int npu_interface_close(struct npu_interface *interface);
int npu_set_cmd(struct npu_interface *interface);

/**
Handler for Upper Layer(N/W, Frame Manager) should be added.

*/

static void __send_interrupt(struct npu_interface *interface);
static irqreturn_t interface_isr(int irq, void *data);
static irqreturn_t interface_isr0(int irq, void *data);
static irqreturn_t interface_isr1(int irq, void *data);

#endif
