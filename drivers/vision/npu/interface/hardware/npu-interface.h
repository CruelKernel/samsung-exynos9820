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

#ifndef _NPU_INTERFACE_H_
#define _NPU_INTERFACE_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include "../../ncp_header.h"
#include "../../npu-log.h"
#include "../../npu-util-llq.h"
#include "mailbox_ipc.h"
#include "mailbox_msg.h"
#include "../../npu-if-protodrv-mbox2.h"
#include "../../npu-protodrv.h"
#include "../../npu-system.h"

#define MAX_MAILBOX	4
#define TRY_CNT	100
#define QSIZE	100
#define BUFSIZE		1024
#define FW_LOGSIZE	(1024 * 128)
#define TRUE		1
#define FALSE		0
#define LENGTHOFEVIDENCE 128

#define IOR8(port)            readb((const volatile void *)&port)
#define IOR16(port)           readw((const volatile void *)&port)
#define IOR32(port)           readl((const volatile void *)&port)
#define IOR64(port)           readq((const volatile void *)&port)


/* Interface object to mailbox_ipc */
struct npu_interface {
	/*mailbox header address */
	volatile struct mailbox_hdr *mbox_hdr;
	/*mailbox sfr address */
	volatile struct mailbox_sfr *sfr;
	/*mailbox undelay. it could be removed later. */
	void *addr;
	int (*msgid_get_type)(int);
	protodrv_notifier rslt_notifier;
	struct mutex lock;
};

enum numCtrl_e {
	ECTRL_LOW = 1,
	ECTRL_HIGH,
	ECTRL_ACK,
	ECTRL_REPORT,
};

int npu_interface_probe(struct device *dev, void *regs);
int npu_interface_open(struct npu_system *system);
int npu_interface_close(struct npu_system *system);

int nw_req_manager(int msgid, struct npu_nw *nw);
int fr_req_manager(int msgid, struct npu_frame *frame);
int nw_rslt_manager(int *ret_msgid, struct npu_nw *nw);
int fr_rslt_manager(int *ret_msgid, struct npu_frame *frame);
int nw_rslt_available(void);
int fr_rslt_available(void);
int register_msgid_get_type(int (*msgid_get_type_func)(int));
int register_rslt_notifier(protodrv_notifier);
void fw_rprt_manager(void);
int mbx_rslt_fault_listener(void);
int npu_check_unposted_mbox(int nCtrl);
#endif
