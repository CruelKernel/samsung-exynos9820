/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Host notify class driver
 *
 * Copyright (C) 2011-2020 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
*/

 /* usb notify layer v3.5 */

#ifndef __LINUX_HOST_NOTIFY_H__
#define __LINUX_HOST_NOTIFY_H__

enum host_uevent_state {
	NOTIFY_HOST_NONE,
	NOTIFY_HOST_ADD,
	NOTIFY_HOST_REMOVE,
	NOTIFY_HOST_OVERCURRENT,
	NOTIFY_HOST_LOWBATT,
	NOTIFY_HOST_BLOCK,
	NOTIFY_HOST_UNKNOWN,
	NOTIFY_HOST_SOURCE,
	NOTIFY_HOST_SINK,
};

enum host_uevent_type {
	NOTIFY_UNKNOWN_STATE,
	NOTIFY_HOST_STATE,
	NOTIFY_POWER_STATE,
};

enum otg_hostnotify_mode {
	NOTIFY_NONE_MODE,
	NOTIFY_HOST_MODE,
	NOTIFY_PERIPHERAL_MODE,
	NOTIFY_TEST_MODE,
};

enum booster_power {
	NOTIFY_POWER_OFF,
	NOTIFY_POWER_ON,
};

enum set_command {
	NOTIFY_SET_OFF,
	NOTIFY_SET_ON,
};

struct host_notify_dev {
	const char *name;
	struct device *dev;
	int index;
	int host_state;
	int host_change;
	int power_state;
	int power_change;
	int mode;
	int booster;
	int (*set_mode)(bool on);
	int (*set_booster)(bool on);
};

#ifdef CONFIG_USB_HOST_NOTIFY
extern int host_state_notify(struct host_notify_dev *ndev, int state);
extern int host_notify_dev_register(struct host_notify_dev *ndev);
extern void host_notify_dev_unregister(struct host_notify_dev *ndev);
#else
static inline int host_state_notify(struct host_notify_dev *ndev, int state)
	{return 0; }
static inline int host_notify_dev_register(struct host_notify_dev *ndev)
	{return 0; }
static inline void host_notify_dev_unregister(struct host_notify_dev *ndev) {}
#endif

#endif /* __LINUX_HOST_NOTIFY_H__ */
