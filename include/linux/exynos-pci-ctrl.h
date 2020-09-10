/*
 * Copyright (C) 2015 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * EXYNOS MODEM CONTROL driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/

#ifndef __EXYNOS_PCIE_CTRL_H
#define __EXYNOS_PCIE_CTRL_H

/* PCIe L1SS Control ID */
#define PCIE_L1SS_CTRL_ARGOS            (0x1 << 0)
#define PCIE_L1SS_CTRL_BOOT             (0x1 << 1)
#define PCIE_L1SS_CTRL_CAMERA           (0x1 << 2)
#define PCIE_L1SS_CTRL_MODEM_IF         (0x1 << 3)
#define PCIE_L1SS_CTRL_WIFI		(0x1 << 4)
#define PCIE_L1SS_CTRL_MST              (0x1 << 5)
#define PCIE_L1SS_CTRL_TEST             (0x1 << 31)

#if defined(CONFIG_PCI_EXYNOS)
extern int exynos_pcie_l1ss_ctrl(int enable, int id);
#endif

#endif
