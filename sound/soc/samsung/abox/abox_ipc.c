/* sound/soc/samsung/abox/abox_ipc.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Inter-Processor Communication driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched/clock.h>

#include <sound/samsung/abox.h>

#include "abox.h"
#include "abox_gic.h"
#include "abox_msg.h"
#include "abox_ipc.h"

#define ABOX_IPC_IRQ	15

static struct device *dev_abox;
static struct device *dev_gic;

static DEFINE_SPINLOCK(lock_tx);
static DEFINE_SPINLOCK(lock_rx);

static LIST_HEAD(ipc_actions);

static void abox_ipc_print_log(const char *fmt, ...)
{
	char *log;
	va_list ap;

	va_start(ap, fmt);

	log = kvasprintf(GFP_ATOMIC, fmt, ap);
	dev_info(dev_abox, "%s", log);
	kfree(log);

	va_end(ap);
}

static void abox_ipc_lock_tx(void)
{
	spin_lock(&lock_tx);
}

static void abox_ipc_unlock_tx(void)
{
	spin_unlock(&lock_tx);
}

static void abox_ipc_lock_rx(void)
{
	spin_lock(&lock_rx);
}

static void abox_ipc_unlock_rx(void)
{
	spin_unlock(&lock_rx);
}

static uint64_t abox_ipc_get_time(void)
{
	return sched_clock();
}

int abox_ipc_send(struct device *dev, const ABOX_IPC_MSG *ipc, size_t size,
		const void *bundle, size_t bundle_size)
{
	struct abox_msg_cmd cmd;
	struct abox_msg_send_data data[2];
	int ret = 0;
	int count = 1;

	dev_dbg(dev, "%s(%d, %d, %d, %zu, %zu)\n", __func__,
			ipc->ipcid, ipc->task_id, ipc->msg.system.msgtype,
			size, bundle_size);

	data[0].data = ipc;
	data[0].size = size;
	data[1].data = bundle;
	data[1].size = bundle_size;

	cmd.id = ipc->ipcid;
	switch (ipc->ipcid) {
	case IPC_SYSTEM:
	{
		const struct IPC_SYSTEM_MSG *ipc_msg = &ipc->msg.system;

		cmd.cmd = ipc_msg->msgtype;
		cmd.arg[0] = ipc_msg->param1;
		cmd.arg[1] = ipc_msg->param2;
		cmd.arg[2] = ipc_msg->param3;

		if (bundle) {
			data[0].size = offsetof(ABOX_IPC_MSG, msg) +
					offsetof(struct IPC_SYSTEM_MSG, bundle);
			count = 2;
		}
		break;
	}
	case IPC_PCMPLAYBACK:
	case IPC_PCMCAPTURE:
	{
		const struct IPC_PCMTASK_MSG *ipc_msg = &ipc->msg.pcmtask;

		cmd.cmd = ipc_msg->msgtype;
		cmd.arg[0] = ipc_msg->channel_id;
		cmd.arg[1] = 0;
		cmd.arg[2] = 0;
		break;
	}
	case IPC_OFFLOAD:
	{
		const struct IPC_OFFLOADTASK_MSG *ipc_msg = &ipc->msg.offload;

		cmd.cmd = ipc_msg->msgtype;
		cmd.arg[0] = ipc_msg->channel_id;
		cmd.arg[1] = 0;
		cmd.arg[2] = 0;
		break;
	}
	case IPC_ERAP:
	{
		const struct IPC_ERAP_MSG *ipc_msg = &ipc->msg.erap;

		cmd.cmd = ipc_msg->msgtype;
		cmd.arg[0] = 0;
		cmd.arg[1] = 0;
		cmd.arg[2] = 0;

		if (bundle) {
			data[0].size = offsetof(ABOX_IPC_MSG, msg) +
					offsetof(struct IPC_ERAP_MSG, param);
			count = 2;
		}
		break;
	}
	case IPC_ABOX_CONFIG:
	{
		const struct IPC_ABOX_CONFIG_MSG *ipc_msg = &ipc->msg.config;

		cmd.cmd = ipc_msg->msgtype;
		cmd.arg[0] = ipc_msg->param1;
		cmd.arg[1] = ipc_msg->param2;
		cmd.arg[2] = 0;
		break;
	}
	case IPC_ASB_TEST:
	{
		const struct IPC_ABOX_TEST_MSG *ipc_msg = &ipc->msg.asb;

		cmd.cmd = ipc_msg->msgtype;
		cmd.arg[0] = ipc_msg->param1;
		cmd.arg[1] = ipc_msg->param2;
		cmd.arg[2] = 0;
		break;
	}
	default:
		cmd.cmd = 0;
		cmd.arg[0] = 0;
		cmd.arg[1] = 0;
		cmd.arg[2] = 0;
		break;
	}

	ret = abox_msg_send(&cmd, data, count);
	abox_gic_generate_interrupt(dev_gic, ABOX_IPC_IRQ);

	return ret;
}

void abox_ipc_retry(void)
{
	abox_gic_generate_interrupt(dev_gic, ABOX_IPC_IRQ);
}

int abox_ipc_flush(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	return abox_msg_flush();
}

int abox_ipc_register_handler(struct device *dev, int ipc_id,
		abox_ipc_handler_t handler, void *data)
{
	struct abox_ipc_action *action;

	dev_dbg(dev, "%s(%d, %pf)\n", __func__, ipc_id, handler);

	list_for_each_entry(action, &ipc_actions, list) {
		if (action->handler != handler || action->ipc_id != ipc_id ||
				action->dev != dev)
			continue;

		action->data = data;
		dev_info(dev, "%s(%d, %pf) updating data\n",
				__func__, ipc_id, handler);
		return 0;
	}

	action = devm_kmalloc(dev_abox, sizeof(*action), GFP_KERNEL);
	action->dev = dev;
	action->ipc_id = ipc_id;
	action->handler = handler;
	action->data = data;
	list_add_tail(&action->list, &ipc_actions);

	return 0;
}

int abox_ipc_unregister_handler(struct device *dev, int ipc_id,
		abox_ipc_handler_t handler)
{
	struct abox_ipc_action *action;

	dev_dbg(dev, "%s(%d, %pf)\n", __func__, ipc_id, handler);

	list_for_each_entry(action, &ipc_actions, list) {
		if (action->handler != handler || action->ipc_id != ipc_id ||
				action->dev != dev)
			continue;

		list_del(&action->list);
		devm_kfree(dev_abox, action);
		return 0;
	}

	dev_err(dev, "%s(%d, %pf) handler not exist\n",
			__func__, ipc_id, handler);
	return -EINVAL;
}

static irqreturn_t abox_ipc_irq_handler(int irq, void *dev_id)
{
	ABOX_IPC_MSG ipc;
	struct device *dev = dev_id;
	irqreturn_t ret = IRQ_NONE;

	dev_dbg(dev, "%s\n", __func__);

	while (abox_msg_recv(NULL, &ipc, sizeof(ipc)) >= 0) {
		enum IPC_ID ipc_id = ipc.ipcid;
		struct abox_ipc_action *action;

		list_for_each_entry(action, &ipc_actions, list) {
			if (action->ipc_id != ipc_id)
				continue;

			ret |= action->handler(ipc_id, action->data, &ipc);
		}
	}

	return ret;
}

int abox_ipc_init(struct device *dev, void *tx, size_t tx_size,
		void *rx, size_t rx_size)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_msg_cfg cfg;
	int ret;

	dev_dbg(dev, "%s(%pK, %zu, %pK, %zu)\n", __func__,
			tx, tx_size, rx, rx_size);

	dev_abox = dev;
	dev_gic = data->dev_gic;

	cfg.tx_addr = tx;
	cfg.tx_size = tx_size;
	cfg.rx_addr = rx;
	cfg.rx_size = rx_size;
	cfg.tx_lock_f = abox_ipc_lock_tx;
	cfg.tx_unlock_f = abox_ipc_unlock_tx;
	cfg.rx_lock_f = abox_ipc_lock_rx;
	cfg.rx_unlock_f = abox_ipc_unlock_rx;
	cfg.get_time_f = abox_ipc_get_time;
	cfg.print_log_f = abox_ipc_print_log;
	cfg.ret_ok = 0;
	cfg.ret_err = -EIO;
	ret = abox_msg_init(&cfg);

	abox_gic_register_irq_handler(dev_gic, ABOX_IPC_IRQ,
			abox_ipc_irq_handler, dev_abox);

	return ret;
}
