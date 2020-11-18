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

#include <linux/delay.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched/clock.h>

#include "mailbox_ipc.h"

#define LINE_TO_SGMT(sgmt_len, ptr)	((ptr) & ((sgmt_len) - 1))

static u32 NPU_MAILBOX_SECTION_CONFIG[MAX_MAILBOX] = {
	1 * K_SIZE,         /* Size of 1st mailbox - 8K */
	1 * K_SIZE,         /* Size of 2nd mailbox - 8K */
	2 * K_SIZE,         /* Size of 3rd mailbox - 8K */
	8 * K_SIZE,         /* Size of 4th mailbox - 4K */
};
void dbg_print_msg(struct message *msg, struct command *cmd);
void dbg_print_ctrl(volatile struct mailbox_ctrl *mCtrl);
void dbg_print_hdr(volatile struct mailbox_hdr *mHdr);

static int wait_for_mem_value(volatile u32 *addr, const u32 expected, int ms_timeout);
static u32 get_logging_time_ms(void);


//TODO : interface should assign the ioremaped address with (NPU_IOMEM_SRAM_END, NPU_MAILBOX_SIZE)
int mailbox_init(volatile struct mailbox_hdr *header)
{
	//Memory Alloc, and Wait Sig.
	int ret = 0;
	int i;
	u32 cur_ofs;
	volatile struct mailbox_ctrl ctrl[MAX_MAILBOX];

	npu_info("mailbox initialize: start, header base at %pK\n", header);
	npu_info("mailbox initialize: wait for firmware boot signature.\n");
	ret = wait_for_mem_value(&(header->signature1), MAILBOX_SIGNATURE1, 3000);
	if (ret) {
		npu_err("init. error(%d) in firmware waiting\n", ret);
		goto err_exit;
	}
	npu_info("header signature \t: %X\n", header->signature1);

	cur_ofs = NPU_MAILBOX_HDR_SECTION_LEN;  /* Start position of data_ofs */
	for (i = 0; i < MAX_MAILBOX; i++) {
		if (unlikely(NPU_MAILBOX_SECTION_CONFIG[i] == 0)) {
		npu_err("invalid mailbox size on %d th mailbox\n", i);
		BUG_ON(1);
		}
		cur_ofs += NPU_MAILBOX_SECTION_CONFIG[i];
		ctrl[i].sgmt_ofs = cur_ofs;
		ctrl[i].sgmt_len = NPU_MAILBOX_SECTION_CONFIG[i];
		ctrl[i].wptr = ctrl[i].rptr = 0;
	}

	header->h2fctrl[0] = ctrl[0];
	header->h2fctrl[1] = ctrl[1];
	header->f2hctrl[0] = ctrl[2];
	header->f2hctrl[1] = ctrl[3];

	header->max_slot = 0;//TODO : TBD in firmware policy.
	header->debug_time = get_logging_time_ms();
	/* half low 16 bits is mailbox ipc version, half high 16 bits is command version */
	header->version = ((COMMAND_VERSION << 16) | MAILBOX_VERSION);
	npu_info("header version \t: %08X\n", header->version);
	header->log_level = 192;

	/* Write Signature */
	header->signature2 = MAILBOX_SIGNATURE2;
	//dbg_print_hdr(header);

	npu_info("init. success in NPU mailbox\n");
	return ret;
err_exit:
	/* Dump mailbox controller */
	if (header)
		npu_debug_memdump32((u32 *)header, (u32 *)(header + 1));

	return ret;
}

/*
 * Busy-wait for the contents of address is set to expected value,
 * up to ms_timeout ms
 * return value:
 *  0 : OK
 *  -ETIMEDOUT : Timeout
 */
static int wait_for_mem_value(volatile u32 *addr, const u32 expected, int ms_timeout)
{
	int i;
	u32 v = 0;

	for (i = 0; i < ms_timeout; i++) {
		v = readl(addr);
		if (v == expected)
			goto ok_exit;
		msleep(1);
	}
	/* Timed-out */
	npu_err("timeout after waiting %d(ms). Current value: [0x%08x]@%pK\n",
		ms_timeout, v, addr);
	return -ETIMEDOUT;
ok_exit:
	return 0;
}

/*
 * Get the current time for kmsg in miliseconds unit
 */
static u32 get_logging_time_ms(void)
{
	unsigned long long kmsg_time_ns = sched_clock();

	/* Convert to ms */
	return (u32)(kmsg_time_ns / 1000 / 1000);
}

static inline u32 __get_readable_size(u32 sgmt_len, u32 wptr, u32 rptr)
{
	return wptr - rptr;
}

static inline u32 __get_writable_size(u32 sgmt_len, u32 wptr, u32 rptr)
{
	return sgmt_len - __get_readable_size(sgmt_len, wptr, rptr);
}

static inline u32 __copy_message_from_line(char *base, u32 sgmt_len, u32 rptr, struct message *msg)
{
	msg->magic	= *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr + 0x00));
	msg->mid	= *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr + 0x04));
	msg->command	= *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr + 0x08));
	msg->length	= *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr + 0x0C));
	/*
	 * actually, self is not requried to copy
	 * msg->self	= *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr + 0x10));
	 */
	msg->data	= *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr + 0x14));

	return rptr + sizeof(struct message);
}

static inline u32 __copy_message_to_line(char *base, u32 sgmt_len, u32 wptr, const struct message *msg)
{
	*(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, wptr + 0x00)) = msg->magic;
	*(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, wptr + 0x04)) = msg->mid;
	*(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, wptr + 0x08)) = msg->command;
	*(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, wptr + 0x0C)) = msg->length;
	/*
	 * actually, self is not requried to copy
	 * *(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, wptr + 0x10)) = msg->self;
	 */
	*(volatile u32 *)(base + LINE_TO_SGMT(sgmt_len, wptr + 0x14)) = msg->data;

	return wptr + sizeof(struct message);
}

static inline u32 __copy_command_from_line(char *base, u32 sgmt_len, u32 rptr, void *cmd, u32 cmd_size)
{
	/* need to reimplement accroding to user environment */
	memcpy(cmd, base + LINE_TO_SGMT(sgmt_len, rptr), cmd_size);
	return rptr + cmd_size;
}

static inline u32 __copy_command_to_line(char *base, u32 sgmt_len, u32 wptr, const void *cmd, u32 cmd_size)
{
	/* need to reimplement accroding to user environment */
	//npu_info("base : %pK\t Move : %08X\t dst : %pK\n",
	//	base, LINE_TO_SGMT(sgmt_len, wptr), (base + LINE_TO_SGMT(sgmt_len, wptr)));
	//npu_info("sgmt_len : %d\t wptr : %d\t size of Cmd : %d\n", sgmt_len, wptr, cmd_size);
	memcpy_toio(base + LINE_TO_SGMT(sgmt_len, wptr), cmd, cmd_size);

	return wptr + cmd_size;
}

/* be careful to use this function, ptr should be virtual address not physical */
void *mbx_ipc_translate(char *underlay, volatile struct mailbox_ctrl *ctrl, u32 ptr)
{
	char *base;

	base = underlay - ctrl->sgmt_ofs;
	return base + LINE_TO_SGMT(ctrl->sgmt_len, ptr);
}

static void __mbx_ipc_print(char *underlay, volatile struct mailbox_ctrl *ctrl, bool is_dbg)
{
	u32 wptr, rptr, sgmt_len;
	struct message msg;
	struct command cmd;
	char *base;

	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;

	if (is_dbg) {
		npu_dbg("   VADDR    PADDR    MAGIC      MID      CMD      LEN     DATA  PAYLOAD      LEN\n");
		npu_dbg("--------------------------------------------------------------------------------\n");
	} else {
		npu_info("   VADDR    PADDR    MAGIC      MID      CMD      LEN     DATA  PAYLOAD      LEN\n");
		npu_info("--------------------------------------------------------------------------------\n");
	}

	while (rptr < wptr) {
		__copy_message_from_line(base, sgmt_len, rptr, &msg);
		/* copy_command should copy only command part else payload */
		__copy_command_from_line(base, sgmt_len, msg.data, &cmd, sizeof(struct command));
		if (is_dbg)
			npu_dbg("0x%08X 0x%08X 0x%08X %8d %8d 0x%08X 0x%08X 0x%08X 0x%08X\n",
				rptr, LINE_TO_SGMT(sgmt_len, rptr),
				msg.magic, msg.mid, msg.command, msg.length, msg.data,
				cmd.payload, cmd.length);
		else
			npu_info("0x%08X 0x%08X 0x%08X %8d %8d 0x%08X 0x%08X 0x%08X 0x%08X\n",
				rptr, LINE_TO_SGMT(sgmt_len, rptr),
				msg.magic, msg.mid, msg.command, msg.length, msg.data,
				cmd.payload, cmd.length);

		rptr = msg.data + msg.length;
	}
}

void mbx_ipc_print(char *underlay, volatile struct mailbox_ctrl *ctrl)
{
	__mbx_ipc_print(underlay, ctrl, false);
}
void mbx_ipc_print_dbg(char *underlay, volatile struct mailbox_ctrl *ctrl)
{
	__mbx_ipc_print(underlay, ctrl, true);
}

int mbx_ipc_put(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg, struct command *cmd)
{
	int ret = 0;
	u32 writable_size, readable_size;
	u32 wptr, rptr, sgmt_len;
	u32 cmd_str_wptr, cmd_end_wptr;
	u32 ofs_to_next_sgmt;
	char *base;

	if ((msg == NULL) || (msg->length == 0) || (cmd == NULL)) {
		ret = -EPARAM;
		goto p_err;
	}

	if (!IS_ALIGNED(msg->length, 4)) {
		ret = -EALIGN;
		goto p_err;
	}
	if ((!underlay) || (!ctrl)) {
		ret = -EINVAL;
		goto p_err;
	}

	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;
	//npu_info("rptr : %d, \t wptr : %d\n", rptr, wptr);

	writable_size = __get_writable_size(sgmt_len, wptr, rptr);
	if (writable_size < sizeof(struct message)) {
		ret = -ERESOURCE;
		goto p_err;
	}

	cmd_str_wptr = wptr + sizeof(struct message);
	cmd_end_wptr = cmd_str_wptr + msg->length;

	ofs_to_next_sgmt = sgmt_len - LINE_TO_SGMT(sgmt_len, cmd_str_wptr);
	if (ofs_to_next_sgmt < msg->length) {
		cmd_str_wptr += ofs_to_next_sgmt;
		cmd_end_wptr += ofs_to_next_sgmt;
	}
	//npu_info("cmd_str_wptr : %d\t cmd_end_wptr : %d.\n", cmd_str_wptr, cmd_end_wptr);
	readable_size = __get_readable_size(sgmt_len, cmd_end_wptr, rptr);
	if (readable_size > sgmt_len) {
		ret = -ERESOURCE;
		goto p_err;
	}

	msg->magic = MESSAGE_MAGIC;
	msg->data = cmd_str_wptr;
	__copy_message_to_line(base, sgmt_len, wptr, msg);
	//npu_info("sgmt_len : %d\t wptr : %d\t cmd_str_wptr : %d\n", sgmt_len, wptr, cmd_str_wptr);
	/* if payload is a continous thing behind of command, payload should be updated */
	if (msg->length > sizeof(struct command)) {
		cmd->payload = (u32)(cmd_str_wptr + sizeof(struct command));
		npu_info("update cmd->payload: %d\n", cmd->payload);
	}
	__copy_command_to_line(base, sgmt_len, cmd_str_wptr, cmd, msg->length);
	ctrl->wptr = cmd_end_wptr;

p_err:
	return ret;
}

/*
 * return value
 * plus value : readed size
 * minus value : error code
 * zero : nothing to read
*/
int mbx_ipc_peek_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg)
{
	int ret = 0;
	u32 readable_size, updated_rptr;
	u32 wptr, rptr, sgmt_len;
	char *base;

	if (msg == NULL || underlay == NULL) {
		ret = -EINVAL;
		goto p_err;
	}
	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;
	readable_size = __get_readable_size(sgmt_len, wptr, rptr);
	if (readable_size == 0) {
		/* 0 read size means there is no any message but it's ok */
		ret = 0;
		goto p_err;
	} else if (readable_size < sizeof(struct message)) {
		ret = -ERESOURCE;
		goto p_err;
	}
	updated_rptr = __copy_message_from_line(base, sgmt_len, rptr, msg);
	if (msg->magic != MESSAGE_MAGIC) {
		ret = -EINVAL;
		goto p_err;
	}
	msg->self = rptr;
	//ctrl->rptr = updated_rptr;
	ret = sizeof(struct message);

p_err:
	return ret;
}

/*
 * return value
 * plus value : readed size
 * minus value : error code
 * zero : nothing to read
*/
int mbx_ipc_get_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg)
{
	int ret = 0;
	u32 readable_size, updated_rptr;
	u32 wptr, rptr, sgmt_len;
	char *base;

	if (msg == NULL) {
		ret = -EINVAL;
		goto p_err;
	}

	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;

	readable_size = __get_readable_size(sgmt_len, wptr, rptr);
	if (readable_size == 0) {
		/* 0 read size means there is no any message but it's ok */
		ret = 0;
		goto p_err;
	} else if (readable_size < sizeof(struct message)) {
		ret = -ERESOURCE;
		goto p_err;
	}
	updated_rptr = __copy_message_from_line(base, sgmt_len, rptr, msg);
	if (msg->magic != MESSAGE_MAGIC) {
		ret = -EINVAL;
		goto p_err;
	}

	msg->self = rptr;
	ctrl->rptr = updated_rptr;
	ret = sizeof(struct message);

p_err:
	return ret;
}


int mbx_ipc_get_cmd(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg, struct command *cmd)
{
	int ret = 0;
	u32 readable_size, updated_rptr;
	u32 wptr, rptr, sgmt_len;
	char *base;

	if ((msg == NULL) || (cmd == NULL)) {
		ret = -EINVAL;
		goto p_err;
	}

	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;

	readable_size = __get_readable_size(sgmt_len, wptr, rptr);
	if (readable_size < msg->length) {
		ret = -EINVAL;
		goto p_err;
	}

	updated_rptr = __copy_command_from_line(base, sgmt_len, msg->data, cmd, msg->length);

	ctrl->rptr = updated_rptr;

p_err:
	return ret;
}


int mbx_ipc_ref_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *prev, struct message *next)
{
	int ret = 0;
	u32 readable_size, msg_str_rptr;
	u32 wptr, rptr, sgmt_len;
	char *base;

	if (next == NULL) {
		ret = -EINVAL;
		goto p_err;
	}

	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;

	readable_size = __get_readable_size(sgmt_len, wptr, rptr);
	if (readable_size < sizeof(struct message)) {
		ret = -ERESOURCE;
		goto p_err;
	}

	msg_str_rptr = ((prev == NULL) ? rptr : prev->data + prev->length);
	__copy_message_from_line(base, sgmt_len, msg_str_rptr, next);
	if (next->magic != MESSAGE_MAGIC) {
		ret = -EINVAL;
		goto p_err;
	}

	next->self = msg_str_rptr;

p_err:
	return ret;
}

int mbx_ipc_clr_msg(char *underlay, volatile struct mailbox_ctrl *ctrl, struct message *msg)
{
	int ret = 0;
	u32 wptr, rptr, sgmt_len;
	struct message temp;
	char *base;

	if (msg == NULL) {
		ret = -EINVAL;
		goto p_err;
	}

	base = underlay - ctrl->sgmt_ofs;
	sgmt_len = ctrl->sgmt_len;
	rptr = ctrl->rptr;
	wptr = ctrl->wptr;

	msg->magic = MESSAGE_MARK;
	__copy_message_to_line(base, sgmt_len, msg->self, msg);

	while (rptr < wptr) {
		__copy_message_from_line(base, sgmt_len, rptr, &temp);
		if (temp.magic != MESSAGE_MARK)
			break;

		rptr = temp.data + temp.length;
	}

	ctrl->rptr = rptr;

p_err:
	return ret;
}

void dbg_print_msg(struct message *msg, struct command *cmd)
{
	npu_info("debug message\n");
	npu_info("msg->magic: (0x%08X)\n", msg->magic);
	npu_info("msg->mid: (%d)\n", msg->mid);
	npu_info("msg->command: (%d)\n", msg->command);
	npu_info("msg->length: (%d)\n", msg->length);

	switch (msg->command) {
	case (COMMAND_LOAD):
		npu_info("\t command_load\n");
		npu_info("\t cmd_load->oid: (%d)\n", cmd->c.load.oid);
		npu_info("\t cmd_load->tid: (%d)\n", cmd->c.load.tid);
		break;
	case (COMMAND_PROCESS):
		npu_info("\t command_process\n");
		npu_info("\t cmd_process->oid \t: (%d)\n", cmd->c.process.oid);
		npu_info("\t cmd_process->fid \t: (%d)\n", cmd->c.process.fid);
		break;
	case (COMMAND_DONE):
		npu_info("\t command_done\n");
		npu_info("\t cmd_done->fid: (0x%x)\n", cmd->c.done.fid);
		break;
	case (COMMAND_NDONE):
		npu_info("\t command_ndone\n");
		npu_info("\t cmd_ndone->fid: (0x%x)\n", cmd->c.ndone.fid);
		npu_info("\t cmd_ndone->error: (0x%x)\n", cmd->c.ndone.error);
		break;
	default:
		break;
	}
	npu_info("\t cmd->payload \t: (%u)\n", cmd->payload);
	npu_info("\t cmd->length \t: (%d)\n", cmd->length);
}

void dbg_print_ctrl(volatile struct mailbox_ctrl *mCtrl)
{
	npu_info("sgmt_ofs \t: (%d)\n", mCtrl->sgmt_ofs);
	npu_info("sgmt_len \t: (%d)\n", mCtrl->sgmt_len);
	npu_info("wptr \t: (%d)\n", mCtrl->wptr);
	npu_info("rptr \t: (%d)\n", mCtrl->rptr);
}

void dbg_print_hdr(volatile struct mailbox_hdr *mHdr)
{
	npu_info("debug mailbox_hdr\n");
	npu_info("max_slot \t\t: (%d)\n", mHdr->max_slot);
	npu_info("debug_time \t\t: (%d)\n", mHdr->debug_time);
	npu_info("debug_code \t\t: (%d)\n", mHdr->debug_code);
	npu_info("totsize \t\t: (%d)\n", mHdr->totsize);
	npu_info("version \t\t: (0x%08X)\n", mHdr->version);
	npu_info("signature2 \t\t: (0x%x)\n", mHdr->signature2);
	npu_info("signature1 \t\t: (0x%x)\n", mHdr->signature1);
}
