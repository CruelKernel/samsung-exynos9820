/*
 * This header provides constants for many UFS bindings.
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *	Kiwoong Kim <kwmad.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DT_BINDINGS_UFS_UFS_H
#define _DT_BINDINGS_UFS_UFS_H

#define true		1
#define false		0

/*
 * from ufs-exynos.h
 */
#define UFS_VER_0004	4
#define UFS_VER_0005	5
#define UFS_VER_0006	6

#define	PHY_CFG_NONE	0
#define	PHY_PCS_COMN	1
#define	PHY_PCS_RXTX	2
#define	PHY_PMA_COMN	3
#define	PHY_PMA_TRSV	4
#define	PHY_PLL_WAIT	5
#define	PHY_CDR_WAIT	6
#define	UNIPRO_STD_MIB	7
#define	UNIPRO_DBG_MIB	8
#define	UNIPRO_DBG_APB	9
#define	PHY_PCS_RX	10
#define	PHY_PCS_TX	11
#define	PHY_PCS_RX_PRD	12
#define	PHY_PCS_TX_PRD	13
#define	UNIPRO_DBG_PRD  14
#define	PHY_PMA_TRSV_LANE1_SQ_OFF 15
#define	COMMON_WAIT	16

#define BIT_0		(1U << 0)
#define BIT_4		(1U << 4)
#define BIT_5		(1U << 5)
#define BIT_8		(1U << 8)
#define BIT_9		(1U << 9)
#define BIT_16		(1U << 16)
#define BIT_17		(1U << 17)
#define BIT_22		(1U << 22)
#define BIT_23		(1U << 23)

#define PMD_PWM_G1_L1	(1U << 0)
#define PMD_PWM_G1_L2	(1U << 1)
#define PMD_PWM_G2_L1	(1U << 2)
#define PMD_PWM_G2_L2	(1U << 3)
#define PMD_PWM_G3_L1	(1U << 4)
#define PMD_PWM_G3_L2	(1U << 5)
#define PMD_PWM_G4_L1	(1U << 6)
#define PMD_PWM_G4_L2	(1U << 7)
#define PMD_PWM_G5_L1	(1U << 8)
#define PMD_PWM_G5_L2	(1U << 9)
#define PMD_HS_G1_L1	(1U << 10)
#define PMD_HS_G1_L2	(1U << 11)
#define PMD_HS_G2_L1	(1U << 12)
#define PMD_HS_G2_L2	(1U << 13)
#define PMD_HS_G3_L1	(1U << 14)
#define PMD_HS_G3_L2	(1U << 15)

#define PMD_ALL		(PMD_HS_G3_L2 - 1)
#define PMD_PWM		(PMD_PWM_G4_L2 - 1)
#define PMD_HS		(PMD_ALL ^ PMD_PWM)

/*
 * from unipro.h
 */
#define PA_ACTIVETXDATALANES	0x1560
#define PA_ACTIVERXDATALANES	0x1580
#define PA_TXTRAILINGCLOCKS	0x1564
#define PA_PHY_TYPE		0x1500
#define PA_AVAILTXDATALANES	0x1520
#define PA_AVAILRXDATALANES	0x1540
#define PA_MINRXTRAILINGCLOCKS	0x1543
#define PA_TXPWRSTATUS		0x1567
#define PA_RXPWRSTATUS		0x1582
#define PA_TXFORCECLOCK		0x1562
#define PA_TXPWRMODE		0x1563
#define PA_LEGACYDPHYESCDL	0x1570
#define PA_MAXTXSPEEDFAST	0x1521
#define PA_MAXTXSPEEDSLOW	0x1522
#define PA_MAXRXSPEEDFAST	0x1541
#define PA_MAXRXSPEEDSLOW	0x1542
#define PA_TXLINKSTARTUPHS	0x1544
#define PA_TXSPEEDFAST		0x1565
#define PA_TXSPEEDSLOW		0x1566
#define PA_REMOTEVERINFO	0x15A0
#define PA_TXGEAR		0x1568
#define PA_TXTERMINATION	0x1569
#define PA_HSSERIES		0x156A
#define PA_PWRMODE		0x1571
#define PA_RXGEAR		0x1583
#define PA_RXTERMINATION	0x1584
#define PA_MAXRXPWMGEAR		0x1586
#define PA_MAXRXHSGEAR		0x1587
#define PA_RXHSUNTERMCAP	0x15A5
#define PA_RXLSTERMCAP		0x15A6
#define PA_PACPREQTIMEOUT	0x1590
#define PA_PACPREQEOBTIMEOUT	0x1591
#define PA_HIBERN8TIME		0x15A7
#define PA_LOCALVERINFO		0x15A9
#define PA_GRANULARITY		0x15AA
#define PA_TACTIVATE		0x15A8
#define PA_PACPFRAMECOUNT	0x15C0
#define PA_PACPERRORCOUNT	0x15C1
#define PA_PHYTESTCONTROL	0x15C2
#define PA_PWRMODEUSERDATA0	0x15B0
#define PA_PWRMODEUSERDATA1	0x15B1
#define PA_PWRMODEUSERDATA2	0x15B2
#define PA_PWRMODEUSERDATA3	0x15B3
#define PA_PWRMODEUSERDATA4	0x15B4
#define PA_PWRMODEUSERDATA5	0x15B5
#define PA_PWRMODEUSERDATA6	0x15B6
#define PA_PWRMODEUSERDATA7	0x15B7
#define PA_PWRMODEUSERDATA8	0x15B8
#define PA_PWRMODEUSERDATA9	0x15B9
#define PA_PWRMODEUSERDATA10	0x15BA
#define PA_PWRMODEUSERDATA11	0x15BB
#define PA_CONNECTEDTXDATALANES	0x1561
#define PA_CONNECTEDRXDATALANES	0x1581
#define PA_LOGICALLANEMAP	0x15A1
#define PA_SLEEPNOCONFIGTIME	0x15A2
#define PA_STALLNOCONFIGTIME	0x15A3
#define PA_SAVECONFIGTIME	0x15A4

#endif
