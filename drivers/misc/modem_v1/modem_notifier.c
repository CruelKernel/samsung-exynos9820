/*
 * Copyright (C) 2015 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdarg.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/modem_notifier.h>

#include "modem_prj.h"
#include "modem_utils.h"

static struct raw_notifier_head modem_event_notifier;

int register_modem_event_notifier(struct notifier_block *nb)
{
	if (!nb)
		return -ENOENT;

	return raw_notifier_chain_register(&modem_event_notifier, nb);
}

void modem_notify_event(enum modem_event evt)
{
	mif_err("event notify (%d) ++\n", evt);
	raw_notifier_call_chain(&modem_event_notifier, evt, NULL);
	mif_err("event notify (%d) --\n", evt);
}
