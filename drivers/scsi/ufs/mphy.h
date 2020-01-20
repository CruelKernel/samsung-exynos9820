/*
 * drivers/scsi/ufs/mphy.h
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _MPHY_H_
#define _MPHY_H_

#define TX_HIBERN8TIME_CAP		0x0f
#define TX_MIN_ACTIVATE_TIME		0x33

#define RX_HS_G1_SYNC_LENGTH_CAP	0x8b
#define RX_HS_G1_PREP_LENGTH_CAP	0x8c
#define RX_HS_G2_SYNC_LENGTH_CAP	0x94
#define RX_HS_G3_SYNC_LENGTH_CAP	0x95
#define RX_HS_G2_PREP_LENGTH_CAP	0x96
#define RX_HS_G3_PREP_LENGTH_CAP	0x97
 #define SYNC_RANGE_FINE	(0 << 6)
 #define SYNC_RANGE_COARSE	(1 << 6)
 #define SYNC_LEN(x)		((x) & 0x3f)
 #define PREP_LEN(x)		((x) & 0xf)
#define RX_ADV_GRANULARITY_CAP		0x98
 #define RX_ADV_FINE_GRAN_STEP(x)	((((x) & 0x3) << 1) | 0x1)
#define TX_ADV_GRANULARITY_CAP		0x10
 #define TX_ADV_FINE_GRAN_STEP(x)	((((x) & 0x3) << 1) | 0x1)
#define RX_MIN_ACTIVATETIME_CAP		0x8f
#define RX_HIBERN8TIME_CAP		0x92
#define RX_ADV_HIBERN8TIME_CAP		0x99
#define RX_ADV_MIN_ACTIVATETIME_CAP	0x9a

#endif /* _MPHY_H_ */
