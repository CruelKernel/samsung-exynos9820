/*
 * This header provides constants for Exynos PCIe bindings.
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *	Kisang Lee <kisang80.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DT_BINDINGS_EXYNOS_PCI_H
#define _DT_BINDINGS_EXYNOS_PCI_H

#define true		1
#define false		0

#define EP_NO_DEVICE		0
#define EP_BCM_WIFI		1
#define EP_SAMSUNG_S359 	2
#define EP_QC_MODEM		3
#define EP_SAMSUNG_MODEM	4

/*
 * CAUTION - It SHOULD fit Target Link Speed Encoding
 * in Link Control2 Register(offset 0xA0)
 */
#define LINK_SPEED_GEN1		1
#define LINK_SPEED_GEN2		2
#define LINK_SPEED_GEN3		3

#endif
