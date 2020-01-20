/*
 * Copyright (C) 2015-2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

  /* usb_power_notify.h */

#ifndef __LINUX_USB_POWER_NOTIFY_H__
#define __LINUX_USB_POWER_NOTIFY_H__

#define HUB_MAX_DEPTH	7

extern void register_usb_power_notify(void);
extern void unregister_usb_power_notify(void);
extern int xhci_port_power_set(u32 on, u32 prt);
#endif

