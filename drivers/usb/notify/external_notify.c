// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2020 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

 /* usb notify layer v3.5 */

#define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/usb.h>
#include <linux/usb_notify.h>

struct external_notify_struct {
	struct blocking_notifier_head notifier_call_chain;
	int call_chain_init;
};

#define SET_EXTERNAL_NOTIFY_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_EXTERNAL_NOTIFY_BLOCK(nb)			\
		SET_EXTERNAL_NOTIFY_BLOCK(nb, NULL, -1)

static struct external_notify_struct external_notifier;

static const char *cmd_string(unsigned long cmd)
{
	switch (cmd) {
	case EXTERNAL_NOTIFY_3S_NODEVICE:
		return "3s_no_device";
	case EXTERNAL_NOTIFY_DEVICE_CONNECT:
		return "device_connect";
	case EXTERNAL_NOTIFY_HOSTBLOCK_PRE:
		return "host_block_pre";
	case EXTERNAL_NOTIFY_HOSTBLOCK_POST:
		return "host_block_post";
	case EXTERNAL_NOTIFY_MDMBLOCK_PRE:
		return "mdm_block_pre";
	case EXTERNAL_NOTIFY_MDMBLOCK_POST:
		return "mdm_block_post";
	case EXTERNAL_NOTIFY_POWERROLE:
		return "power_role_notify";
	case EXTERNAL_NOTIFY_DEVICEADD:
		return "host_mode_device_added";
	case EXTERNAL_NOTIFY_HOSTBLOCK_EARLY:
		return "host_block_pre_fast";
	default:
		return "undefined";
	}
}

static const char *listener_string(int  listener)
{
	switch (listener) {
	case EXTERNAL_NOTIFY_DEV_MUIC:
		return "muic";
	case EXTERNAL_NOTIFY_DEV_CHARGER:
		return "charger";
	case EXTERNAL_NOTIFY_DEV_PDIC:
		return "pdic";
	default:
		return "undefined";
	}
}

static int create_external_notify(void)
{
	if (!external_notifier.call_chain_init) {
		pr_info("%s\n", __func__);
		BLOCKING_INIT_NOTIFIER_HEAD
				(&(external_notifier.notifier_call_chain));
		external_notifier.call_chain_init = 1;
	}
	return 0;
}

int usb_external_notify_register(struct notifier_block *nb,
			notifier_fn_t notifier, int listener)
{
	int ret = 0;

	pr_info("%s: listener=(%s)%d register\n", __func__,
			listener_string(listener), listener);

	create_external_notify();

	SET_EXTERNAL_NOTIFY_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register
				(&(external_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	return ret;
}
EXPORT_SYMBOL(usb_external_notify_register);

int usb_external_notify_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=(%s)%d unregister\n", __func__,
			listener_string(nb->priority),
				nb->priority);

	ret = blocking_notifier_chain_unregister
				(&(external_notifier.notifier_call_chain), nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_EXTERNAL_NOTIFY_BLOCK(nb);

	return ret;
}
EXPORT_SYMBOL(usb_external_notify_unregister);

int send_external_notify(unsigned long cmd, int data)
{
	int ret = 0;

	pr_info("%s: cmd=%s(%lu), data=%d\n", __func__, cmd_string(cmd),
						cmd, data);

	create_external_notify();

	ret = blocking_notifier_call_chain
		(&(external_notifier.notifier_call_chain),
			cmd, (void *)&(data));

	switch (ret) {
	case NOTIFY_STOP_MASK:
	case NOTIFY_BAD:
		pr_err("%s: notify error occur(0x%x)\n", __func__, ret);
		break;
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s: notify done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s: notify status unknown(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}
EXPORT_SYMBOL(send_external_notify);

void external_notifier_init(void)
{
	pr_info("%s\n", __func__);
	create_external_notify();
}
EXPORT_SYMBOL(external_notifier_init);

