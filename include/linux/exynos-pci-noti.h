/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PCI_NOTI_H
#define __PCI_NOTI_H

enum exynos_pcie_event {
	EXYNOS_PCIE_EVENT_INVALID = 0,
	EXYNOS_PCIE_EVENT_LINKDOWN = 0x1,
	EXYNOS_PCIE_EVENT_LINKUP = 0x2,
	EXYNOS_PCIE_EVENT_WAKEUP = 0x4,
	EXYNOS_PCIE_EVENT_WAKE_RECOVERY = 0x8,
	EXYNOS_PCIE_EVENT_NO_ACCESS = 0x10,
};

enum exynos_pcie_trigger {
	EXYNOS_PCIE_TRIGGER_CALLBACK,
	EXYNOS_PCIE_TRIGGER_COMPLETION,
};

struct exynos_pcie_notify {
	enum exynos_pcie_event event;
	void *user;
	void *data;
	u32 options;
};

struct exynos_pcie_register_event {
	u32 events;
	void *user;
	enum exynos_pcie_trigger mode;
	void (*callback)(struct exynos_pcie_notify *notify);
	struct exynos_pcie_notify notify;
	struct completion *completion;
	u32 options;
};
#endif
