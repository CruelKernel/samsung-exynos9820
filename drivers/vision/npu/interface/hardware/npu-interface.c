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
#include <linux/init.h>
#include <linux/device.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include "npu-interface.h"
#include "npu-util-llq.h"

#define LINE_TO_SGMT(sgmt_len, ptr)	((ptr) & ((sgmt_len) - 1))
static struct workqueue_struct *wq;
static struct work_struct work_report;
static void __rprt_manager(struct work_struct *w);


struct npu_interface interface = {
	.mbox_hdr = NULL,
	.sfr = NULL,
	.addr = NULL,
};

void dbg_dump_mbox(void)
{
	int rptr;
	u32 sgmt_len;
	char *base;

	volatile struct mailbox_ctrl *ctrl;

	ctrl = &(interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_REPORT]);
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;
	rptr = 0;
	npu_debug_memdump32_by_memcpy((u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr)),
		(u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr) + (8 * K_SIZE)));

	ctrl = &(interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_RESPONSE]);
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;
	rptr = 0;
	npu_debug_memdump32_by_memcpy((u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr)),
		(u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr) + (2 * K_SIZE)));

	ctrl = &(interface.mbox_hdr->h2fctrl[MAILBOX_H2FCTRL_HPRIORITY]);
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;
	rptr = 0;
	npu_debug_memdump32_by_memcpy((u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr)),
		(u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr) + (1 * K_SIZE)));

	ctrl = &(interface.mbox_hdr->h2fctrl[MAILBOX_H2FCTRL_LPRIORITY]);
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;
	rptr = 0;
	npu_debug_memdump32_by_memcpy((u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr)),
		(u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr) + (1 * K_SIZE)));
	//Dump mbox_hdr
	npu_debug_memdump32_by_memcpy((u32 *)interface.mbox_hdr,
		(u32 *)(interface.mbox_hdr + 1));
}

void dbg_print_error(void)
{
	u32 nSize = 0;
	u32 wptr, rptr, sgmt_len;
	char buf[BUFSIZE - 1] = "";
	char *base;

	volatile struct mailbox_ctrl *ctrl;

	if (interface.mbox_hdr == NULL)
		return;
	mutex_lock(&interface.lock);

	ctrl = &(interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_REPORT]);
	wptr = ctrl->wptr;
	rptr = ctrl->rptr;
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;

	if (wptr < rptr) { //When wptr circled a time,
		while (rptr != 0) {
			if ((sgmt_len - rptr) >= BUFSIZE) {
				nSize = BUFSIZE - 1;
				memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
				rptr += nSize;
			} else {
				nSize = sgmt_len - rptr;
				memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
				rptr = 0;
			}
			pr_err("%s\n", buf);
			buf[0] = '\0';
		}
	}
	while (wptr != rptr) {
		if ((wptr - rptr) > BUFSIZE - 1) {//need more memcpy from io.
			nSize = BUFSIZE - 1;
			memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
			rptr += nSize;
		} else {
			nSize = wptr - rptr;
			memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
			rptr = wptr;
		}
		pr_err("%s\n", buf);
		buf[0] = '\0';
	}
	mutex_unlock(&interface.lock);
}

void dbg_print_ncp_header(struct ncp_header *nhdr)
{
	npu_info("============= ncp_header at %pK =============\n", nhdr);
	npu_info("mabic_number1 \t: 0X%X\n", nhdr->magic_number1);
	npu_info("hdr_version \t\t: 0X%X\n", nhdr->hdr_version);
	npu_info("hdr_size \t\t: 0X%X\n", nhdr->hdr_size);
	npu_info("net_id \t\t: 0X%X\n", nhdr->net_id);
	npu_info("unique_id \t\t: 0X%X\n", nhdr->unique_id);
	npu_info("priority \t\t: 0X%X\n", nhdr->priority);
	npu_info("flags \t\t: 0X%X\n", nhdr->flags);
	npu_info("period \t\t: 0X%X\n", nhdr->period);
	npu_info("workload \t\t: 0X%X\n", nhdr->workload);

	npu_info("addr_vector_offset \t: 0X%X\n", nhdr->address_vector_offset);
	npu_info("addr_vector_cnt \t: 0X%X\n", nhdr->address_vector_cnt);
	npu_info("magic_number2 \t: 0X%X\n", nhdr->magic_number2);
}

void dbg_print_interface(void)
{
	npu_info("=============  interface at %pK =============\n", &interface);
	npu_info("mbox_hdr \t\t: 0x%pK\n", interface.mbox_hdr);
	npu_info("mbox_hdr.max_slot \t: %d\n", interface.mbox_hdr->max_slot);
	npu_info("mbox_hdr.version \t: %d\n", interface.mbox_hdr->version);
	npu_info("mbox_hdr.signature2 \t: 0X%x\n", interface.mbox_hdr->signature2);
	npu_info("mbox_hdr.signature1 \t: 0X%x\n", interface.mbox_hdr->signature1);
	npu_info("sfr \t\t\t: 0x%pK\n", interface.sfr);
	npu_info("addr \t\t\t: 0x%pK\n", interface.addr);
}


static irqreturn_t mailbox_isr2(int irq, void *data)
{
	u32 val;

	val = interface.sfr->grp[3].ms;
	if (val)
		interface.sfr->grp[3].c = val;
	if (interface.rslt_notifier != NULL)
		interface.rslt_notifier(NULL);
	return IRQ_HANDLED;
}

static irqreturn_t mailbox_isr3(int irq, void *data)
{
	u32 val;

	val = interface.sfr->grp[4].ms;
	if (val)
		interface.sfr->grp[4].c = val;
	if (wq)
		queue_work(wq, &work_report);
	return IRQ_HANDLED;
}

static void __send_interrupt(u32 cmdType)
{
	u32 val;

	switch (cmdType) {
	case COMMAND_LOAD:
	case COMMAND_UNLOAD:
	case COMMAND_PROFILE_CTL:
	case COMMAND_PURGE:
	case COMMAND_POWERDOWN:
	case COMMAND_FW_TEST:
		interface.sfr->grp[0].g = 0x10000;
		val = interface.sfr->grp[0].g;
		//npu_info("g0.g = %x\n", val);
		break;
	case COMMAND_PROCESS:
		interface.sfr->grp[2].g = 0xFFFFFFFF;
		val = interface.sfr->grp[2].g;
		//npu_info("g2.g = %x\n", val);
		break;
	case COMMAND_DONE:
		interface.sfr->grp[3].g = 0xFFFFFFFF;
		val = interface.sfr->grp[3].g;
		//npu_info("g3.g = %x\n", val);
		break;
	default:
		break;
	}
}

static int npu_set_cmd(struct message *msg, struct command *cmd, u32 cmdType)
{
	int ret = 0;

	ret = mbx_ipc_put((void *)interface.addr, &interface.mbox_hdr->h2fctrl[cmdType], msg, cmd);
	if (ret)
		goto I_ERR;
	__send_interrupt(msg->command);
	return 0;
I_ERR:
	switch (ret) {
	case -ERESOURCE:
		npu_warn("No space left on mailbox : ret = %d\n", ret);
		break;
	default:
		npu_err("mbx_ipc_put err with %d\n", ret);
		break;
	}
	return ret;
}

static ssize_t get_ncp_hdr_size(const struct npu_nw *nw)
{
	struct ncp_header *ncp_header;

	BUG_ON(!nw);

	if (nw->ncp_addr.vaddr == NULL) {
		npu_err("not specified in ncp_addr.kvaddr");
		return -EINVAL;
	}

	ncp_header = (struct ncp_header *)nw->ncp_addr.vaddr;
	//dbg_print_ncp_header(ncp_header);
	if (ncp_header->magic_number1 != NCP_MAGIC1) {
		npu_info("invalid MAGIC of NCP header (0x%08x) at (%pK)", ncp_header->magic_number1, ncp_header);
		return -EINVAL;
	}
	return ncp_header->hdr_size;
}

int npu_interface_probe(struct device *dev, void *regs)
{
	int ret = 0;

	BUG_ON(!dev);
	if (!regs) {
		probe_err("fail in %s\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	interface.sfr = (volatile struct mailbox_sfr *)regs;
	mutex_init(&interface.lock);
	wq = alloc_workqueue("my work", WQ_FREEZABLE|WQ_HIGHPRI, 1);
	INIT_WORK(&work_report, __rprt_manager);
	probe_info("complete in %s\n", __func__);
	return ret;
}

int npu_interface_open(struct npu_system *system)
{
	int ret = 0;
	struct npu_device *device;
	struct device *dev = &system->pdev->dev;

	BUG_ON(!system);
	device = container_of(system, struct npu_device, system);
	interface.addr = (void *)((system->tcu_sram.vaddr) + NPU_MAILBOX_BASE);
	interface.mbox_hdr = system->mbox_hdr;

	ret = devm_request_irq(dev, system->irq0, mailbox_isr2, 0, "exynos-npu", NULL);
	if (ret) {
		probe_err("fail(%d) in devm_request_irq(2)\n", ret);
		goto err_exit;
	}
	ret = devm_request_irq(dev, system->irq1, mailbox_isr3, 0, "exynos-npu", NULL);
	if (ret) {
		probe_err("fail(%d) in devm_request_irq(3)\n", ret);
		goto err_probe_irq2;
	}

	wq = create_singlethread_workqueue("rprt_manager");
	if (!wq) {
		npu_err("err in alloc_worqueue.\n");
		goto err_exit;
	}
	ret = mailbox_init(interface.mbox_hdr);
	if (ret) {
		npu_err("error(%d) in npu_mailbox_init\n", ret);
		goto err_workqueue;
	}

	return ret;

err_workqueue:
	destroy_workqueue(wq);
err_probe_irq2:
	devm_free_irq(dev, system->irq0, NULL);
err_exit:
	interface.addr = NULL;
	interface.mbox_hdr = NULL;
	set_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);
	npu_err("EMERGENCY_RECOVERY is triggered.\n");
	return ret;
}
int npu_interface_close(struct npu_system *system)
{
	int wptr, rptr;
	struct device *dev = &system->pdev->dev;

	if (!system) {
		npu_err("fail in %s\n", __func__);
		return -EINVAL;
	}

	devm_free_irq(dev, system->irq0, NULL);
	devm_free_irq(dev, system->irq1, NULL);

	queue_work(wq, &work_report);
	if ((wq) && (interface.mbox_hdr)) {
		if (work_pending(&work_report)) {
			cancel_work_sync(&work_report);
			wptr = interface.mbox_hdr->f2hctrl[1].wptr;
			rptr = interface.mbox_hdr->f2hctrl[1].rptr;
			npu_dbg("work was canceled due to interface close, rptr/wptr : %d/%d\n", wptr, rptr);
		}
		flush_workqueue(wq);
		destroy_workqueue(wq);
		wq = NULL;
	}

	interface.addr = NULL;
	interface.mbox_hdr = NULL;
	return 0;
}

int register_rslt_notifier(protodrv_notifier func)
{
	interface.rslt_notifier = func;
	return 0;
}

int register_msgid_get_type(int (*msgid_get_type_func)(int))
{
	interface.msgid_get_type = msgid_get_type_func;
	return 0;
}

int nw_req_manager(int msgid, struct npu_nw *nw)
{
	int ret = 0;
	struct command cmd = {};
	struct message msg = {};
	ssize_t hdr_size;

	switch (nw->cmd) {
	case NPU_NW_CMD_BASE:
		npu_info("abnormal command type\n");
		break;
	case NPU_NW_CMD_LOAD:
		cmd.c.load.oid = nw->uid;
		cmd.c.load.tid = NPU_MAILBOX_DEFAULT_TID;
		hdr_size = get_ncp_hdr_size(nw);
		if (hdr_size <= 0) {
			npu_info("fail in get_ncp_hdr_size: (%zd)", hdr_size);
			ret = FALSE;
			goto nw_req_err;
		}

		cmd.length = (u32)hdr_size;
		cmd.payload = nw->ncp_addr.daddr;
		msg.command = COMMAND_LOAD;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_UNLOAD:
		cmd.c.unload.oid = nw->uid;//NPU_MAILBOX_DEFAULT_OID;
		cmd.payload = 0;
		cmd.length = 0;
		msg.command = COMMAND_UNLOAD;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_PROFILE_START:
		cmd.c.profile_ctl.ctl = PROFILE_CTL_CODE_START;
		cmd.payload = nw->ncp_addr.daddr;
		cmd.length = nw->ncp_addr.size;
		msg.command = COMMAND_PROFILE_CTL;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_PROFILE_STOP:
		cmd.c.profile_ctl.ctl = PROFILE_CTL_CODE_STOP;
		cmd.payload = 0;
		cmd.length = 0;
		msg.command = COMMAND_PROFILE_CTL;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_STREAMOFF:
		cmd.c.purge.oid = nw->uid;
		cmd.payload = 0;
		msg.command = COMMAND_PURGE;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_POWER_DOWN:
		cmd.c.powerdown.dummy = nw->uid;
		cmd.payload = 0;
		msg.command = COMMAND_POWERDOWN;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_FW_TC_EXECUTE:
		cmd.c.fw_test.param = nw->param0;
		cmd.payload = 0;
		cmd.length = 0;
		msg.command = COMMAND_FW_TEST;
		msg.length = sizeof(struct command);
		break;
	case NPU_NW_CMD_END:
		break;
	default:
		npu_err("invalid CMD ID: (%db)", nw->cmd);
		ret = FALSE;
		goto nw_req_err;
	}
	msg.mid = msgid;

	ret = npu_set_cmd(&msg, &cmd, NPU_MBOX_REQUEST_LOW);
	if (ret)
		goto nw_req_err;

	mbx_ipc_print_dbg((void *)interface.addr, &interface.mbox_hdr->h2fctrl[0]);
	return TRUE;
nw_req_err:
	return ret;
}

int fr_req_manager(int msgid, struct npu_frame *frame)
{
	int ret = FALSE;
	struct command cmd = {};
	struct message msg = {};

	switch (frame->cmd) {
	case NPU_FRAME_CMD_BASE:
		break;
	case NPU_FRAME_CMD_Q:
		cmd.c.process.oid = frame->uid;
		cmd.c.process.fid = frame->frame_id;
		cmd.length = frame->mbox_process_dat.address_vector_cnt;
		cmd.payload = frame->mbox_process_dat.address_vector_start_daddr;
		msg.command = COMMAND_PROCESS;
		msg.length = sizeof(struct command);
		break;
	case NPU_FRAME_CMD_END:
		break;
	}
	msg.mid = msgid;
	ret = npu_set_cmd(&msg, &cmd, NPU_MBOX_REQUEST_HIGH);
	if (ret)
		goto fr_req_err;

	mbx_ipc_print_dbg((void *)interface.addr, &interface.mbox_hdr->h2fctrl[1]);
	return TRUE;
fr_req_err:
	return ret;
}

int nw_rslt_manager(int *ret_msgid, struct npu_nw *nw)
{
	int ret;
	struct message msg;
	struct command cmd;

	ret = mbx_ipc_peek_msg((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0], &msg);
	if (ret <= 0)
		return FALSE;

	ret = interface.msgid_get_type(msg.mid);
	if (ret != PROTO_DRV_REQ_TYPE_NW)
		return FALSE;//report error content:

	ret = mbx_ipc_get_msg((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0], &msg);
	if (ret <= 0)
		return FALSE;

	ret = mbx_ipc_get_cmd((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0], &msg, &cmd);
	if (ret) {
		npu_err("get command error\n");
		return FALSE;
	}

	if (msg.command == COMMAND_DONE) {
		npu_info("COMMAND_DONE for mid: (%d)\n", msg.mid);
		nw->result_code = NPU_ERR_NO_ERROR;
	} else if (msg.command == COMMAND_NDONE) {
		npu_err("COMMAND_NDONE for mid: (%d) error(%u/0x%08x)\n"
			, msg.mid, cmd.c.ndone.error, cmd.c.ndone.error);
		nw->result_code = cmd.c.ndone.error;
	} else {
		npu_err("invalid msg.command: (%d)\n", msg.command);
		return FALSE;
	}
	fw_rprt_manager();
	*ret_msgid = msg.mid;
	return TRUE;
}

int fr_rslt_manager(int *ret_msgid, struct npu_frame *frame)
{

	int ret = FALSE;
	struct message msg;
	struct command cmd;

	ret = mbx_ipc_peek_msg((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0], &msg);
	if (ret <= 0)
		return FALSE;

	ret = interface.msgid_get_type(msg.mid);
	if (ret != PROTO_DRV_REQ_TYPE_FRAME) {
		//npu_info("get type error: (%d)\n", ret);
		return FALSE;
	}

	ret = mbx_ipc_get_msg((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0], &msg);
	if (ret <= 0)// 0 : no msg, less than zero : Err
		return FALSE;

	ret = mbx_ipc_get_cmd((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0], &msg, &cmd);
	if (ret)
		return FALSE;

	if (msg.command == COMMAND_DONE) {
		npu_info("COMMAND_DONE for mid: (%d)\n", msg.mid);
		frame->result_code = NPU_ERR_NO_ERROR;
	} else if (msg.command == COMMAND_NDONE) {
		npu_err("COMMAND_NDONE for mid: (%d) error(%u/0x%08x)\n"
			, msg.mid, cmd.c.ndone.error, cmd.c.ndone.error);
		frame->result_code = cmd.c.ndone.error;
	} else {
		npu_err("invalid msg.command: (%d)\n", msg.command);
		return FALSE;
	}
	*ret_msgid = msg.mid;
	fw_rprt_manager();
	return TRUE;
}

//Print log which was written with last 128 byte.
int npu_check_unposted_mbox(int nCtrl)
{
	int pos;
	int ret = TRUE;
	char *base;
	u32 nSize, wptr, rptr, sgmt_len;
	u32 *buf, *strOut;
	volatile struct mailbox_ctrl *ctrl;

	if (interface.mbox_hdr == NULL)
		return -1;

	strOut = kzalloc(LENGTHOFEVIDENCE, GFP_ATOMIC);
	if (!strOut) {
		ret = -ENOMEM;
		goto err_exit;
	}
	buf = kzalloc(LENGTHOFEVIDENCE, GFP_ATOMIC);
	if (!buf) {
		if (strOut)
			kfree(strOut);
		ret = -ENOMEM;
		goto err_exit;
	}

	mutex_lock(&interface.lock);

	switch (nCtrl) {
	case ECTRL_LOW:
		ctrl = &(interface.mbox_hdr->h2fctrl[MAILBOX_H2FCTRL_LPRIORITY]);
		pr_err("[V] H2F_MBOX[LOW] - rptr/wptr : %d/%d\n", ctrl->rptr, ctrl->wptr);
		break;
	case ECTRL_HIGH:
		ctrl = &(interface.mbox_hdr->h2fctrl[MAILBOX_H2FCTRL_HPRIORITY]);
		pr_err("[V] H2F_MBOX[HIGH] - rptr/wptr : %d/%d\n", ctrl->rptr, ctrl->wptr);
		break;
	case ECTRL_ACK:
		ctrl = &(interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_RESPONSE]);
		pr_err("[V] F2H_MBOX[RESPONSE] - rptr/wptr : %d/%d\n", ctrl->rptr, ctrl->wptr);
		break;
	case ECTRL_REPORT:
		ctrl = &(interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_REPORT]);
		pr_err("[V] F2H_MBOX[REPORT] - rptr/wptr : %d/%d\n", ctrl->rptr, ctrl->wptr);
		break;
	default:
		BUG_ON(1);
	}

	wptr = ctrl->wptr;
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;

	pos = 0;
	if (wptr < LENGTHOFEVIDENCE) {
		rptr = sgmt_len - (LENGTHOFEVIDENCE - wptr);
		nSize = LENGTHOFEVIDENCE - wptr;
		npu_debug_memdump32_by_memcpy((u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr)), (u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr) + nSize));
		rptr = 0;
		pos = 0;
		nSize = wptr;
	} else {
		rptr = wptr - LENGTHOFEVIDENCE;
		nSize = LENGTHOFEVIDENCE;
	}
	if (wptr > 0)
		npu_debug_memdump32_by_memcpy((u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr)), (u32 *)(base + LINE_TO_SGMT(sgmt_len, rptr) + nSize));
	mutex_unlock(&interface.lock);
	ret = TRUE;
	if (buf)
		kfree(buf);
	if (strOut)
		kfree(strOut);
err_exit:
	return ret;
}

static void __rprt_manager(struct work_struct *w)
{
	char *base;
	u32 nSize;
	u32 wptr, rptr, sgmt_len;
	char buf[BUFSIZE - 1] = "\0";
	volatile struct mailbox_ctrl *ctrl;

	if (interface.mbox_hdr == NULL)
		return;
	mutex_lock(&interface.lock);

	ctrl = &(interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_REPORT]);
	wptr = ctrl->wptr;
	rptr = ctrl->rptr;
	sgmt_len = ctrl->sgmt_len;
	base = (void *)interface.addr - ctrl->sgmt_ofs;

	if (wptr < rptr) { //When wptr circled a time,
		while (rptr != 0) {
			if ((sgmt_len - rptr) >= BUFSIZE) {
				nSize = BUFSIZE - 1;
				memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
				rptr += nSize;
			} else {
				nSize = sgmt_len - rptr;
				memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
				rptr = 0;
			}
			npu_fw_report_store(buf, nSize);
			buf[0] = '\0';
		}
	}

	while (wptr != rptr) {
		if ((wptr - rptr) > BUFSIZE - 1) {//need more memcpy from io.
			nSize = BUFSIZE - 1;
			memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
			rptr += nSize;
		} else {
			nSize = wptr - rptr;
			memcpy_fromio(buf, base + LINE_TO_SGMT(sgmt_len, rptr), nSize);
			rptr = wptr;
		}
		npu_fw_report_store(buf, nSize);
		buf[0] = '\0';
	}
	interface.mbox_hdr->f2hctrl[MAILBOX_F2HCTRL_REPORT].rptr = wptr;
	mutex_unlock(&interface.lock);
	return;

}

void fw_rprt_manager(void)
{
	if (wq)
		queue_work(wq, &work_report);
	else//not opened, or already closed.
		return;
}

int mbx_rslt_fault_listener(void)
{
	fw_rprt_manager();
	dbg_print_ctrl(&interface.mbox_hdr->h2fctrl[0]);
	mbx_ipc_print((void *)interface.addr, &interface.mbox_hdr->h2fctrl[0]);

	dbg_print_ctrl(&interface.mbox_hdr->h2fctrl[1]);
	mbx_ipc_print((void *)interface.addr, &interface.mbox_hdr->h2fctrl[1]);

	dbg_print_ctrl(&interface.mbox_hdr->f2hctrl[0]);
	mbx_ipc_print((void *)interface.addr, &interface.mbox_hdr->f2hctrl[0]);

	dbg_print_ctrl(&interface.mbox_hdr->f2hctrl[1]);

	fw_will_note(FW_LOGSIZE);
	dbg_print_error();
	return 0;
}

int nw_rslt_available(void)
{
	int ret;
	struct message msg;

	ret = mbx_ipc_peek_msg(
		(void *)interface.addr,
		&interface.mbox_hdr->f2hctrl[0], &msg);
	return ret;
}
int fr_rslt_available(void)
{
	int ret;
	struct message msg;

	ret = mbx_ipc_peek_msg(
		(void *)interface.addr,
		&interface.mbox_hdr->f2hctrl[0], &msg);
	return ret;
}

/* Unit test */
#ifdef CONFIG_VISION_UNITTEST
#define IDIOT_TESTCASE_IMPL "npu-interface.idiot"
#include "idiot-def.h"
#endif
