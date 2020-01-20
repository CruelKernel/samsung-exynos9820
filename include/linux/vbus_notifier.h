/*
 * include/linux/vbus_notifier.h
 *
 * header file supporting VBUS notifier call chain information
 *
 * Copyright (C) 2010 Samsung Electronics
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

#ifndef __VBUS_NOTIFIER_H__
#define __VBUS_NOTIFIER_H__

/* VBUS notifier call chain command */
typedef enum {
	VBUS_NOTIFY_CMD_NONE = 0,
	VBUS_NOTIFY_CMD_FALLING,
	VBUS_NOTIFY_CMD_RISING,
} vbus_notifier_cmd_t;

/* VBUS notifier call sequence,
 * largest priority number device will be called first. */
typedef enum {
	VBUS_NOTIFY_DEV_USB,
	VBUS_NOTIFY_DEV_CHARGER,
	VBUS_NOTIFY_DEV_MANAGER,	
} vbus_notifier_device_t;

typedef enum {
	STATUS_VBUS_UNKNOWN = 0,
	STATUS_VBUS_LOW,
	STATUS_VBUS_HIGH,
} vbus_status_t;

typedef enum {
	VBUS_NOTIFIER_NOT_READY = 0,
	VBUS_NOTIFIER_NOT_READY_DETECT,
	VBUS_NOTIFIER_READY,
} vbus_notifier_stat_t;

struct vbus_notifier_struct {
	struct mutex		vbus_mutex;
	vbus_notifier_stat_t	status;
	vbus_status_t		vbus_type;
	vbus_notifier_cmd_t	cmd;
	struct blocking_notifier_head notifier_call_chain;
};

#define VBUS_NOTIFIER_BLOCK(name)	\
	struct notifier_block (name)

/* vbus notifier init/notify function
 * this function is for JUST VBUS device driver.
 * DON'T use function anywhrer else!!
 */
extern void vbus_notifier_handle(vbus_status_t new_dev);

/* vbus notifier register/unregister API
 * for used any where want to receive vbus attached device attach/detach. */
extern int vbus_notifier_register(struct notifier_block *nb,
		notifier_fn_t notifier, vbus_notifier_device_t listener);
extern int vbus_notifier_unregister(struct notifier_block *nb);

#endif /* __VBUS_NOTIFIER_H__ */
