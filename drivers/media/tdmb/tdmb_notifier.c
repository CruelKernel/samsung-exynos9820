/*
 *
 * drivers/media/tdmb/tdmb_notifier.c
 *
 * tdmb notifier
 *
 * Copyright (C) (2017, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */



#include <linux/device.h>

#include <linux/notifier.h>
#include <linux/tdmb_notifier.h>
#include <linux/sec_sysfs.h>

#define SET_TDMB_NOTIFIER_BLOCK(nb, fn, dev) do {	\
		(nb)->notifier_call = (fn);		\
		(nb)->priority = (dev);			\
	} while (0)

#define DESTROY_TDMB_NOTIFIER_BLOCK(nb)			\
		SET_TDMB_NOTIFIER_BLOCK(nb, NULL, -1)

static BLOCKING_NOTIFIER_HEAD(tdmb_notifier_call_chain);
static struct tdmb_notifier_struct tdmb_notifier;

int tdmb_notifier_register(struct notifier_block *nb, notifier_fn_t notifier,
			enum tdmb_notifier_device_t listener)
{
	int ret = 0;

	pr_info("%s: listener=%d register\n", __func__, listener);

	SET_TDMB_NOTIFIER_BLOCK(nb, notifier, listener);
	ret = blocking_notifier_chain_register(&tdmb_notifier_call_chain, nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_register error(%d)\n",
				__func__, ret);

	/* tdmb's attached_device status notify */
	nb->notifier_call(nb, tdmb_notifier.event,
			&(tdmb_notifier));

	return ret;
}

int tdmb_notifier_unregister(struct notifier_block *nb)
{
	int ret = 0;

	pr_info("%s: listener=%d unregister\n", __func__, nb->priority);

	ret = blocking_notifier_chain_unregister(&tdmb_notifier_call_chain, nb);
	if (ret < 0)
		pr_err("%s: blocking_notifier_chain_unregister error(%d)\n",
				__func__, ret);
	DESTROY_TDMB_NOTIFIER_BLOCK(nb);

	return ret;
}

static int tdmb_notifier_notify(void)
{
	int ret = 0;

	ret = blocking_notifier_call_chain(&tdmb_notifier_call_chain,
				tdmb_notifier.event, &(tdmb_notifier));

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

static void tdmb_notifier_set_property(struct tdmb_notifier_struct *value)
{
	tdmb_notifier.event = value->event;
	switch (value->event) {
	case TDMB_NOTIFY_EVENT_TUNNER:
		tdmb_notifier.tdmb_status.pwr = value->tdmb_status.pwr;
		break;
	default:
		break;

	}
}

void tdmb_notifier_call(struct tdmb_notifier_struct *value)
{
	/*event broadcast */
	pr_info("%s: TDMB_NOTIFY_EVENT :%d\n", __func__, value->event);
	tdmb_notifier_set_property(value);
	tdmb_notifier_notify();
}

