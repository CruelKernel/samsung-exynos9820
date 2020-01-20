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

#ifndef _MAILBOX_IPC_
#define _MAILBOX_IPC_

#include <linux/types.h>
#include <linux/io.h>
#include "mailbox.h"
#include "npu-common.h"
#include "npu-config.h"
#include "../../npu-log.h"
#include "mailbox_msg.h"
#include "../../npu-session.h"

#define MAX_MAILBOX	4
#define EPARAM		41
#define ERESOURCE	43
#define EALIGN		44

enum npu_mbox_index_e {
	NPU_MBOX_REQUEST_LOW = 0,
	NPU_MBOX_REQUEST_HIGH,
	NPU_MBOX_RESULT,//and RESPONSE
	NPU_MBOX_LOG,
	NPU_MBOX_RESERVED,
	NPU_MBOX_MAX_ID
};

void *mbx_ipc_translate(char *underlay, volatile struct mailbox_ctrl *ctrl, u32 ptr);
void mbx_ipc_print(char *underlay, volatile struct mailbox_ctrl *ctrl);
void mbx_ipc_print_dbg(char *underlay, volatile struct mailbox_ctrl *ctrl);
int mbx_ipc_put(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg, struct command *cmd);
int mbx_ipc_peek_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg);
int mbx_ipc_get_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg);
int mbx_ipc_get_cmd(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg, struct command *cmd);
int mbx_ipc_clr_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg);
int mailbox_init(volatile struct mailbox_hdr *header);
void dbg_print_hdr(volatile struct mailbox_hdr *mHdr);
void dbg_print_msg(struct message *msg, struct command *cmd);
void dbg_print_ctrl(volatile struct mailbox_ctrl *mCtrl);
int get_message_id(void);
void pop_message_id(int msg_id);

#endif
