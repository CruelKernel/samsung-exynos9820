/*
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 *		http://www.samsung.com/
 *
 * Common defines for samsung usb controllers
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __LINUX_USB_SAMSUNG_USB_H
#define __LINUX_USB_SAMSUNG_USB_H

enum samsung_cpu_type {
	TYPE_S3C64XX,
	TYPE_EXYNOS4210,
	TYPE_EXYNOS5250,
	TYPE_EXYNOS5430,
	TYPE_EXYNOS7420,
	TYPE_EXYNOS7580,
	TYPE_EXYNOS8890,
	TYPE_EXYNOS8895,
};

enum samsung_usb_ip_type {
	TYPE_USB3DRD = 0,
	TYPE_USB3HOST,
	TYPE_USB2DRD,
	TYPE_USB2HOST,
};

enum samsung_phy_set_option {
	SET_DPPULLUP_ENABLE,
	SET_DPPULLUP_DISABLE,
	SET_DPDM_PULLDOWN,
};
#endif	/* __LINUX_USB_SAMSUNG_USB_H */
