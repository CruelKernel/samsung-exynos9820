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
#include "npu-interface.h"

LSM_DECLARE(mailboxMgr, struct npu_interface, QSIZE, "TestLSM");
static int handle_ACK_from_MBOX(void);
static int handle_RSLT_from_MBOX(void);
static int handle_RPT_from_MBOX(void);

static int handle_ACK_to_MGR(void);
static int handle_REQ_from_MGR(void);

int IF_do_task(struct auto_sleep_thread_param *data)
{
	int ret = 0;
	/**
	  * 1. Check Ack, Report, Result in Buffer List from Lower Layer.
	  */
	ret += handle_ACK_from_MBOX();
	ret += handle_RSLT_from_MBOX();
	ret += handle_RPT_from_MBOX();

	/**
	  * 2. Check Request from Upper Layer(N/W, Frame manager)
	  */
	ret += handle_REQ_from_MGR();
	ret += handle_ACK_to_MGR();
	return ret;
}
int IF_check_work(struct auto_sleep_thread_param *data)
{
	/**
	  * 1. Check whether operation(manager, queue, mbox) is available or not
	  */
	return 0;
}
int IF_compare(const struct npu_interface *lhs, const struct npu_interface *rhs)
{
	return 0;
}
#define LSM_SETUP(INSTANCE) \
do { INSTANCE.lsm_init(IF_do_task, IF_check_work, IF_compare); } while (0)

#define LSM_DESTORY(INSTANCE) \
do { INSTANCE.lsm_destroy(); } while (0)

/**
  * Test Module for LSM
  */
int test_LSM(void)
{
	int ret = 0;
	//LSM_SETUP(mailboxMgr);
	struct ncp_header *nH;
	struct npu_interface *inpIF, *outIF;

	inpIF = kmalloc(sizeof(struct npu_interface), GFP_KERNEL);
	nH = kmalloc(sizeof(struct ncp_header), GFP_KERNEL);
	nH->magic_number1 = 12;
	nH->hdr_version = 12;
	inpIF->ncp_header = nH;

	ret = mailboxMgr.lsm_put_entry(REQUESTED, inpIF);
	if (ret) {
		npu_err("LSM: index(%d)\n", ret);
		return ret;
	}

	outIF = mailboxMgr.lsm_get_entry(FREE);
	if (ret) {
		npu_err("LSM: magic number(%d)\t hdr_version(%d)\n", outIF->ncp_header->magic_number1, outIF->ncp_header->hdr_version);
		return ret;
	}

	return ret;
}

/**
  * @brief	NPU Interface init module.
  *		Initialize Interface module and call mailbox init module.
  * @return	Success or Fail
 */
int npu_interface_probe(struct npu_interface *interface,
						struct device *dev,
						void __iomem *code,
						resource_size_t code_size,
						void __iomem *regs,
						resource_size_t regs_size,
						u32 irq0, u32 irq1)
{
	int ret = 0;

	BUG_ON(!interface);
	BUG_ON(!dev);
	BUG_ON(!code);
	BUG_ON(!regs);
	/**
	  * 1. Create LSM with proper size
	  */
	LSM_SETUP(mailboxMgr);
	//ret = test_LSM();
	/**
	  * 2. mem allocation for private_data in interface
	  */
	interface->private_data = kmalloc(sizeof(struct mailbox_ctrl), GFP_KERNEL);
	if (!interface->private_data) {
		probe_err("kmalloc is fail\n");
		ret = -ENOMEM;
		return -1;
	}

	/**
	  * 3. irq registration. It should be replaced real address.
	  */
	ret = devm_request_irq(dev, irq0, interface_isr0, 0, dev_name(dev), interface);
	if (ret) {
		probe_err("fail(%d) in devm_request_irq(0)\n", ret);
		return -1;
	}
	ret = devm_request_irq(dev, irq1, interface_isr1, 0, dev_name(dev), interface);
	if (ret) {
		probe_err("fail(%d) in devm_request_irq(1)\n", ret);
		return -1;
	}
	/**
	  * 4. Init Mailbox Mgr
	  *	- Nothing to Do
	  */
	init_process_barrier(interface);
	return ret;
}

static void __send_interrupt(struct npu_interface *interface)
{
	//int ret = 0;
	u32 type, offset, val, try_cnt;

	type = 0;
	BUG_ON(interface);

	/**
	  * Type should be updated.
	  */
	switch (type) {
	case NPU_H2F_LOW:
		offset = LOW_ADDR;
		break;
	case NPU_H2F_NORMAL:
		offset = NOR_ADDR;
		break;
	case NPU_H2F_HIGH:
		offset = HIGH_ADDR;
		break;
	}
	try_cnt = TRY_CNT;
	val = readl(interface->regs + offset);
	while (--try_cnt && val) {
		writel(0x0, interface->regs + offset);
		val = readl(interface->regs + offset);
	}
	writel(0x100, interface->regs + offset);
	udelay(1);
	writel(0x0, interface->regs + offset);

}


/**
  * - Check whether mailbox is ready or not.
  * - If ready, write the payload to mailbox
  * - send the interrupt to notify.
  * - wait the reply from F/W(mailbox confirm)
  */
int npu_set_cmd(struct npu_interface *interface)
{
	int ret = 0;
	struct mailbox_ctrl *mctrl;

	BUG_ON(!interface);
	mctrl = interface->private_data;
	/**
	  * size, type, cid, etc...
	  */

	enter_process_barrier(interface);
	ret = npu_mailbox_ready(mctrl);

	if (ret) {//failure case
		exit_process_barrier(interface);
	}

	ret = npu_mailbox_write(mctrl);
	if (ret) {//failure case
		exit_process_barrier(interface);
	}

	__send_interrupt(interface);
	exit_process_barrier(interface);
	return ret;
}

/**
  * @brief	ISR Handler function.
			From the interrupt irq, and data(it should be structure npu_interface)
  *

  */
static irqreturn_t	interface_isr(int irq, void *data)
{
	//int ret =0;
	struct npu_interface *interface;
	struct mailbox_ctrl *mctrl;

	interface = (struct npu_interface *)data;
	mctrl = interface->private_data;

	return IRQ_HANDLED;
}

static irqreturn_t interface_isr0(int irq, void *data)
{
	struct npu_interface *interface = (struct npu_interface *)data;

	writel(0, interface->regs + 0xC);//[BAE] offset should be updated.
	interface_isr(irq, data);

	return IRQ_HANDLED;
}

static irqreturn_t interface_isr1(int irq, void *data)
{
	struct npu_interface *interface = (struct npu_interface *)data;
	u32 val;

	val = readl(interface->regs + 0x1B4);//[BAE] offset should be updated.
	if (val & 0x2000) {//
		writel(0x2000, interface->regs + 0x1B4);//
		interface_isr(irq, data);
	}
	return IRQ_HANDLED;
}


static int handle_ACK_from_MBOX(void)
{
	return 0;
}
static int handle_RSLT_from_MBOX(void)
{
	return 0;
}
static int handle_RPT_from_MBOX(void)
{
	return 0;
}
static int handle_ACK_to_MGR(void)
{
	return 0;
}
static int handle_REQ_from_MGR(void)
{
	return 0;
}

