/*
 * include/linux/muic/muic_notifier.h
 *
 * header file supporting MUIC notifier call chain information
 *
 * Copyright (C) 2010 Samsung Electronics
 * Seung-Jin Hahn <sjin.hahn@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __MUIC_NOTIFIER_H__
#define __MUIC_NOTIFIER_H__

#include <linux/ccic/ccic_notifier.h>

/* MUIC notifier call chain command */
typedef enum {
	MUIC_NOTIFY_CMD_DETACH	= 0,
	MUIC_NOTIFY_CMD_ATTACH,
	MUIC_NOTIFY_CMD_LOGICALLY_DETACH,
	MUIC_NOTIFY_CMD_LOGICALLY_ATTACH,
} muic_notifier_cmd_t;

/* MUIC notifier call sequence,
 * largest priority number device will be called first. */
typedef enum {
	MUIC_NOTIFY_DEV_DOCK = 0,
	MUIC_NOTIFY_DEV_MHL,
	MUIC_NOTIFY_DEV_USB,
	MUIC_NOTIFY_DEV_TSP,
	MUIC_NOTIFY_DEV_CHARGER,
	MUIC_NOTIFY_DEV_CPUIDLE,
	MUIC_NOTIFY_DEV_CPUFREQ,
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	MUIC_NOTIFY_DEV_MANAGER,
#endif
	MUIC_NOTIFY_DEV_CABLE_DATA,
} muic_notifier_device_t;

struct muic_notifier_struct {
	muic_attached_dev_t attached_dev;
	muic_notifier_cmd_t cmd;
	CC_NOTI_ATTACH_TYPEDEF cxt;
	CC_NOTI_ATTACH_TYPEDEF cxt2;
	struct blocking_notifier_head notifier_call_chain;
	struct mutex mutex;
};

#define MUIC_NOTIFIER_BLOCK(name)	\
	struct notifier_block (name)

/* muic notifier init/notify function
 * this function is for JUST MUIC device driver.
 * DON'T use function anywhrer else!!
 */
extern void muic_notifier_attach_attached_dev(muic_attached_dev_t new_dev);
extern void muic_notifier_detach_attached_dev(muic_attached_dev_t cur_dev);
extern void muic_notifier_logically_attach_attached_dev(muic_attached_dev_t new_dev);
extern void muic_notifier_logically_detach_attached_dev(muic_attached_dev_t cur_dev);

/* muic notifier register/unregister API
 * for used any where want to receive muic attached device attach/detach. */
extern int muic_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, muic_notifier_device_t listener);
extern int muic_notifier_unregister(struct notifier_block *nb);

/* Choose a proper noti. interface for a test */
extern void muic_notifier_set_new_noti(bool flag);

#endif /* __MUIC_NOTIFIER_H__ */
