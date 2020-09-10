// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/notifier.h>
#include <linux/export.h>

static BLOCKING_NOTIFIER_HEAD(panel_notifier_list);

/**
 *	panel_notifier_register - register a client notifier
 *	@nb: notifier block to callback on events
 */
int panel_notifier_register(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&panel_notifier_list, nb);
}
EXPORT_SYMBOL(panel_notifier_register);

/**
 *	panel_notifier_unregister - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int panel_notifier_unregister(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&panel_notifier_list, nb);
}
EXPORT_SYMBOL(panel_notifier_unregister);

/**
 * panel_notifier_call_chain - notify clients
 *
 */
int panel_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&panel_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(panel_notifier_call_chain);
