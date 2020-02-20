/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * SFR access functions for Samsung EXYNOS SoC MIPI-DSI Master driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../dsim.h"
#include "regs-decon.h"

/* dsim version */
#define DSIM_VER_EVT0			0x02020000
#define DSIM_VER_EVT1			0x02030000

/* These definitions are need to guide from AP team */
#define DSIM_STOP_STATE_CNT		0xA
#define DSIM_BTA_TIMEOUT		0xff
#define DSIM_LP_RX_TIMEOUT		0xffff
#define DSIM_MULTI_PACKET_CNT		0xffff
#define DSIM_PLL_STABLE_TIME		0x682A
#define DSIM_FIFOCTRL_THRESHOLD		0x1 /* 1 ~ 32 */

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
#define PLL_SLEEP_CNT_MULT		450
#define PLL_SLEEP_CNT_MARGIN_RATIO	0	/* 10% ~ 20% */
#define PLL_SLEEP_CNT_MARGIN	(PLL_SLEEP_CNT_MULT * PLL_SLEEP_CNT_MARGIN_RATIO / 100)
#endif

/* If below values depend on panel. These values wil be move to panel file.
 * And these values are valid in case of video mode only.
 */
#define DSIM_CMD_ALLOW_VALUE		4
#define DSIM_STABLE_VFP_VALUE		2
#define TE_PROTECT_ON_TIME		158 /* 15.8ms*/
#define TE_TIMEOUT_TIME			180 /* 18ms */

#define PLL_LOCK_CNT_MULT		500
#define PLL_LOCK_CNT_MARGIN_RATIO	0	/* 10% ~ 20% */
#define PLL_LOCK_CNT_MARGIN	(PLL_LOCK_CNT_MULT * PLL_LOCK_CNT_MARGIN_RATIO / 100)

u32 DSIM_PHY_BIAS_CON_VAL[] = {
	0x00000010,
	0x00000110,
	0x00003223,
	0x00000000,
	0x00000000,
};

u32 DSIM_PHY_PLL_CON_VAL[] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000500,
	0x0000008E,
	0x00001450,
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	0x00000A28,
#endif
};

u32 DSIM_PHY_MC_GNR_CON_VAL[] = {
	0x00000000,
	0x00001450,
};
u32 DSIM_PHY_MC_ANA_CON_VAL[] = {
	/* EDGE_CON[14:12] DPHY=3'b111, CPHY=3'b001 */
	0x00007033,
	0x00000000,
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	0x00000002,
#endif
};

/* same value in all master data lane */
u32 DSIM_PHY_MD_GNR_CON_VAL[] = {
	0x00000000,
	0x00001450,
};
u32 DSIM_PHY_MD_ANA_CON_VAL[] = {
	0x00007033,
	0x00000000,
	0x00000000,
	0x00000000,
};


/* DPHY timing table */
/* below table have to be changed to meet MK DPHY spec*/
const u32 dphy_timing[][10] = {
	/* bps, clk_prepare, clk_zero, clk_post, clk_trail,
	 * hs_prepare, hs_zero, hs_trail, lpx, hs_exit
	 */
	{2500, 12, 43, 11, 11, 11, 20, 10, 9 , 17,},
	{2490, 12, 43, 11, 11, 11, 20, 10, 9 , 17,},
	{2480, 12, 42, 11, 11, 11, 20, 10, 9 , 17,},
	{2470, 11, 43, 11, 11, 11, 20, 10, 9 , 16,},
	{2460, 11, 43, 11, 11, 11, 20, 10, 9 , 16,},
	{2450, 11, 43, 11, 11, 11, 20, 10, 9 , 16,},
	{2440, 11, 42, 11, 11, 11, 19, 10, 9 , 16,},
	{2430, 11, 42, 11, 11, 11, 19, 10, 9 , 16,},
	{2420, 11, 42, 11, 10, 11, 19, 10, 9 , 16,},
	{2410, 11, 42, 11, 10, 11, 19, 10, 9 , 16,},
	{2400, 11, 41, 10, 10, 11, 19, 10, 8 , 16,},
	{2390, 11, 41, 10, 10, 11, 19, 10, 8 , 16,},
	{2380, 11, 41, 10, 10, 11, 19, 9 , 8 , 16,},
	{2370, 11, 41, 10, 10, 11, 18, 9 , 8 , 16,},
	{2360, 11, 41, 10, 10, 11, 18, 9 , 8 , 16,},
	{2350, 11, 40, 10, 10, 11, 18, 9 , 8 , 16,},
	{2340, 11, 40, 10, 10, 11, 18, 9 , 8 , 16,},
	{2330, 11, 40, 10, 10, 10, 19, 9 , 8 , 16,},
	{2320, 11, 40, 10, 10, 10, 19, 9 , 8 , 15,},
	{2310, 11, 39, 10, 10, 10, 19, 9 , 8 , 15,},
	{2300, 11, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2290, 11, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2280, 11, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2270, 10, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2260, 10, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2250, 10, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2240, 10, 39, 10, 10, 10, 18, 9 , 8 , 15,},
	{2230, 10, 38, 10, 10, 10, 18, 9 , 8 , 15,},
	{2220, 10, 38, 10, 10, 10, 17, 9 , 8 , 15,},
	{2210, 10, 38, 10, 10, 10, 17, 9 , 8 , 15,},
	{2200, 10, 38, 9 , 10, 10, 17, 9 , 8 , 15,},
	{2190, 10, 38, 9 , 9 , 10, 17, 9 , 8 , 15,},
	{2180, 10, 37, 9 , 9 , 10, 17, 9 , 8 , 14,},
	{2170, 10, 37, 9 , 9 , 10, 17, 9 , 8 , 14,},
	{2160, 10, 37, 9 , 9 , 10, 17, 9 , 8 , 14,},
	{2150, 10, 37, 9 , 9 , 10, 16, 8 , 8 , 14,},
	{2140, 10, 36, 9 , 9 , 10, 16, 8 , 8 , 14,},
	{2130, 10, 36, 9 , 9 , 10, 16, 8 , 7 , 14,},
	{2120, 10, 36, 9 , 9 , 9 , 17, 8 , 7 , 14,},
	{2110, 10, 36, 9 , 9 , 9 , 17, 8 , 7 , 14,},
	{2100, 10, 35, 9 , 9 , 9 , 17, 8 , 7 , 14,},
	{2090, 10, 35, 9 , 9 , 9 , 17, 8 , 7 , 14,},
	{2080, 9 , 36, 9 , 9 , 9 , 16, 8 , 7 , 14,},
	{2070, 9 , 36, 9 , 9 , 9 , 16, 8 , 7 , 14,},
	{2060, 9 , 35, 9 , 9 , 9 , 16, 8 , 7 , 14,},
	{2050, 9 , 35, 9 , 9 , 9 , 16, 8 , 7 , 14,},
	{2040, 9 , 35, 9 , 9 , 9 , 16, 8 , 7 , 14,},
	{2030, 9 , 35, 9 , 9 , 9 , 16, 8 , 7 , 13,},
	{2020, 9 , 35, 9 , 9 , 9 , 16, 8 , 7 , 13,},
	{2010, 9 , 34, 9 , 9 , 9 , 15, 8 , 7 , 13,},
	{2000, 9 , 34, 8 , 9 , 9 , 15, 8 , 7 , 13,},
	{1990, 9 , 34, 8 , 9 , 9 , 15, 8 , 7 , 13,},
	{1980, 9 , 34, 8 , 9 , 9 , 15, 8 , 7 , 13,},
	{1970, 9 , 33, 8 , 9 , 9 , 15, 8 , 7 , 13,},
	{1960, 9 , 33, 8 , 9 , 9 , 15, 8 , 7 , 13,},
	{1950, 9 , 33, 8 , 8 , 9 , 15, 8 , 7 , 13,},
	{1940, 9 , 33, 8 , 8 , 9 , 15, 8 , 7 , 13,},
	{1930, 9 , 32, 8 , 8 , 9 , 14, 8 , 7 , 13,},
	{1920, 9 , 32, 8 , 8 , 9 , 14, 8 , 7 , 13,},
	{1910, 9 , 32, 8 , 8 , 8 , 15, 7 , 7 , 13,},
	{1900, 9 , 32, 8 , 8 , 8 , 15, 7 , 7 , 13,},
	{1890, 9 , 31, 8 , 8 , 8 , 15, 7 , 7 , 12,},
	{1880, 8 , 32, 8 , 8 , 8 , 15, 7 , 7 , 12,},
	{1870, 8 , 32, 8 , 8 , 8 , 15, 7 , 7 , 12,},
	{1860, 8 , 32, 8 , 8 , 8 , 14, 7 , 6 , 12,},
	{1850, 8 , 32, 8 , 8 , 8 , 14, 7 , 6 , 12,},
	{1840, 8 , 31, 8 , 8 , 8 , 14, 7 , 6 , 12,},
	{1830, 8 , 31, 8 , 8 , 8 , 14, 7 , 6 , 12,},
	{1820, 8 , 31, 8 , 8 , 8 , 14, 7 , 6 , 12,},
	{1810, 8 , 31, 8 , 8 , 8 , 14, 7 , 6 , 12,},
	{1800, 8 , 30, 7 , 8 , 8 , 14, 7 , 6 , 12,},
	{1790, 8 , 30, 7 , 8 , 8 , 13, 7 , 6 , 12,},
	{1780, 8 , 30, 7 , 8 , 8 , 13, 7 , 6 , 12,},
	{1770, 8 , 30, 7 , 8 , 8 , 13, 7 , 6 , 12,},
	{1760, 8 , 29, 7 , 8 , 8 , 13, 7 , 6 , 12,},
	{1750, 8 , 29, 7 , 8 , 8 , 13, 7 , 6 , 12,},
	{1740, 8 , 29, 7 , 8 , 8 , 13, 7 , 6 , 11,},
	{1730, 8 , 29, 7 , 8 , 8 , 13, 7 , 6 , 11,},
	{1720, 8 , 29, 7 , 7 , 8 , 13, 7 , 6 , 11,},
	{1710, 8 , 28, 7 , 7 , 8 , 12, 7 , 6 , 11,},
	{1700, 8 , 28, 7 , 7 , 7 , 13, 7 , 6 , 11,},
	{1690, 8 , 28, 7 , 7 , 7 , 13, 7 , 6 , 11,},
	{1680, 7 , 29, 7 , 7 , 7 , 13, 6 , 6 , 11,},
	{1670, 7 , 28, 7 , 7 , 7 , 13, 6 , 6 , 11,},
	{1660, 7 , 28, 7 , 7 , 7 , 13, 6 , 6 , 11,},
	{1650, 7 , 28, 7 , 7 , 7 , 13, 6 , 6 , 11,},
	{1640, 7 , 28, 7 , 7 , 7 , 12, 6 , 6 , 11,},
	{1630, 7 , 27, 7 , 7 , 7 , 12, 6 , 6 , 11,},
	{1620, 7 , 27, 7 , 7 , 7 , 12, 6 , 6 , 11,},
	{1610, 7 , 27, 7 , 7 , 7 , 12, 6 , 6 , 11,},
	{1600, 7 , 27, 6 , 7 , 7 , 12, 6 , 5 , 10,},
	{1590, 7 , 26, 6 , 7 , 7 , 12, 6 , 5 , 10,},
	{1580, 7 , 26, 6 , 7 , 7 , 12, 6 , 5 , 10,},
	{1570, 7 , 26, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1560, 7 , 26, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1550, 7 , 26, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1540, 7 , 25, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1530, 7 , 25, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1520, 7 , 25, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1510, 7 , 25, 6 , 7 , 7 , 11, 6 , 5 , 10,},
	{1500, 7 , 24, 6 , 7 , 7 , 10, 6 , 5 , 10,},
	{1490, 59, 25, 6 , 77, 59, 10, 70, 44, 9 ,},
	{1480, 59, 24, 6 , 76, 58, 10, 70, 44, 9 ,},
	{1470, 58, 24, 6 , 76, 58, 10, 69, 44, 9 ,},
	{1460, 58, 24, 6 , 76, 58, 10, 69, 43, 9 ,},
	{1450, 58, 24, 6 , 75, 57, 10, 68, 43, 9 ,},
	{1440, 57, 24, 6 , 75, 57, 10, 68, 43, 9 ,},
	{1430, 57, 23, 6 , 75, 57, 10, 68, 43, 8 ,},
	{1420, 56, 23, 6 , 74, 57, 9 , 67, 43, 8 ,},
	{1410, 56, 23, 6 , 74, 57, 9 , 67, 43, 8 ,},
	{1400, 56, 23, 5 , 74, 55, 9 , 67, 41, 8 ,},
	{1390, 55, 23, 5 , 73, 55, 9 , 66, 41, 8 ,},
	{1380, 55, 23, 5 , 73, 54, 9 , 66, 41, 8 ,},
	{1370, 54, 22, 5 , 72, 54, 9 , 66, 41, 8 ,},
	{1360, 54, 22, 5 , 72, 54, 9 , 65, 40, 8 ,},
	{1350, 54, 22, 5 , 72, 53, 9 , 65, 40, 8 ,},
	{1340, 53, 22, 5 , 71, 53, 9 , 65, 40, 8 ,},
	{1330, 53, 22, 5 , 71, 53, 9 , 64, 39, 8 ,},
	{1320, 52, 22, 5 , 71, 53, 8 , 64, 40, 8 ,},
	{1310, 52, 21, 5 , 70, 53, 8 , 64, 40, 8 ,},
	{1300, 51, 21, 5 , 70, 51, 8 , 63, 38, 8 ,},
	{1290, 51, 21, 5 , 70, 51, 8 , 63, 38, 7 ,},
	{1280, 51, 21, 5 , 69, 51, 8 , 63, 38, 7 ,},
	{1270, 50, 21, 5 , 69, 50, 8 , 62, 38, 7 ,},
	{1260, 50, 20, 5 , 69, 50, 8 , 62, 37, 7 ,},
	{1250, 49, 20, 5 , 68, 49, 8 , 62, 37, 7 ,},
	{1240, 49, 20, 5 , 68, 49, 8 , 61, 37, 7 ,},
	{1230, 49, 20, 5 , 68, 49, 8 , 61, 36, 7 ,},
	{1220, 48, 20, 5 , 67, 48, 8 , 61, 36, 7 ,},
	{1210, 48, 19, 5 , 67, 48, 7 , 60, 36, 7 ,},
	{1200, 49, 19, 4 , 67, 49, 7 , 60, 36, 7 ,},
	{1190, 48, 19, 4 , 66, 48, 7 , 60, 36, 7 ,},
	{1180, 48, 19, 4 , 66, 48, 7 , 59, 36, 7 ,},
	{1170, 46, 19, 4 , 66, 46, 7 , 59, 35, 7 ,},
	{1160, 46, 18, 4 , 65, 46, 7 , 59, 34, 7 ,},
	{1150, 45, 18, 4 , 65, 46, 7 , 58, 34, 7 ,},
	{1140, 45, 18, 4 , 65, 45, 7 , 58, 34, 6 ,},
	{1130, 45, 18, 4 , 64, 45, 7 , 58, 33, 6 ,},
	{1120, 44, 18, 4 , 64, 44, 7 , 57, 33, 6 ,},
	{1110, 44, 18, 4 , 64, 44, 7 , 57, 33, 6 ,},
	{1100, 43, 17, 4 , 63, 44, 6 , 57, 32, 6 ,},
	{1090, 43, 17, 4 , 63, 44, 6 , 56, 33, 6 ,},
	{1080, 43, 17, 4 , 63, 44, 6 , 56, 33, 6 ,},
	{1070, 42, 17, 4 , 62, 44, 6 , 56, 33, 6 ,},
	{1060, 42, 17, 4 , 62, 42, 6 , 55, 31, 6 ,},
	{1050, 41, 17, 4 , 62, 42, 6 , 55, 31, 6 ,},
	{1040, 41, 16, 4 , 61, 41, 6 , 54, 31, 6 ,},
	{1030, 41, 16, 4 , 61, 41, 6 , 54, 30, 6 ,},
	{1020, 40, 16, 4 , 61, 41, 6 , 54, 30, 6 ,},
	{1010, 40, 16, 4 , 60, 40, 6 , 53, 30, 6 ,},
	{1000, 39, 16, 3 , 60, 40, 6 , 53, 29, 5 ,},
	{990 , 39, 15, 3 , 60, 39, 6 , 53, 29, 5 ,},
	{980 , 39, 15, 3 , 59, 39, 5 , 52, 29, 5 ,},
	{970 , 38, 15, 3 , 59, 39, 5 , 52, 29, 5 ,},
	{960 , 38, 15, 3 , 59, 39, 5 , 52, 29, 5 ,},
	{950 , 37, 15, 3 , 58, 39, 5 , 51, 29, 5 ,},
	{940 , 37, 14, 3 , 58, 39, 5 , 51, 29, 5 ,},
	{930 , 37, 14, 3 , 57, 37, 5 , 51, 27, 5 ,},
	{920 , 36, 14, 3 , 57, 37, 5 , 50, 27, 5 ,},
	{910 , 36, 14, 3 , 57, 36, 5 , 50, 27, 5 ,},
	{900 , 35, 14, 3 , 56, 36, 5 , 50, 26, 5 ,},
	{890 , 35, 14, 3 , 56, 36, 5 , 49, 26, 5 ,},
	{880 , 35, 13, 3 , 56, 35, 5 , 49, 26, 5 ,},
	{870 , 34, 13, 3 , 55, 35, 4 , 49, 26, 5 ,},
	{860 , 34, 13, 3 , 55, 35, 4 , 48, 25, 5 ,},
	{850 , 33, 13, 3 , 55, 35, 4 , 48, 26, 4 ,},
	{840 , 33, 13, 3 , 54, 35, 4 , 48, 26, 4 ,},
	{830 , 33, 12, 3 , 54, 33, 4 , 47, 24, 4 ,},
	{820 , 32, 12, 3 , 54, 33, 4 , 47, 24, 4 ,},
	{810 , 32, 12, 3 , 53, 33, 4 , 47, 24, 4 ,},
	{800 , 31, 12, 2 , 53, 32, 4 , 46, 23, 4 ,},
	{790 , 31, 12, 2 , 53, 32, 4 , 46, 23, 4 ,},
	{780 , 30, 12, 2 , 52, 31, 4 , 46, 23, 4 ,},
	{770 , 30, 11, 2 , 52, 31, 4 , 45, 23, 4 ,},
	{760 , 30, 11, 2 , 52, 31, 3 , 45, 22, 4 ,},
	{750 , 29, 11, 2 , 51, 30, 3 , 45, 22, 4 ,},
	{740 , 29, 11, 2 , 51, 30, 3 , 44, 22, 4 ,},
	{730 , 28, 11, 2 , 51, 31, 3 , 44, 22, 4 ,},
	{720 , 28, 10, 2 , 50, 30, 3 , 44, 22, 4 ,},
	{710 , 28, 10, 2 , 50, 30, 3 , 43, 22, 4 ,},
	{700 , 27, 10, 2 , 50, 28, 3 , 43, 20, 3 ,},
	{690 , 27, 10, 2 , 49, 28, 3 , 43, 20, 3 ,},
	{680 , 26, 10, 2 , 49, 28, 3 , 42, 20, 3 ,},
	{670 , 26, 10, 2 , 49, 27, 3 , 42, 20, 3 ,},
	{660 , 26, 9 , 2 , 48, 27, 3 , 42, 19, 3 ,},
	{650 , 25, 9 , 2 , 48, 26, 3 , 41, 19, 3 ,},
	{640 , 25, 9 , 2 , 48, 26, 2 , 41, 19, 3 ,},
	{630 , 24, 9 , 2 , 47, 26, 2 , 40, 18, 3 ,},
	{620 , 24, 9 , 2 , 47, 26, 2 , 40, 19, 3 ,},
	{610 , 24, 8 , 2 , 47, 26, 2 , 40, 19, 3 ,},
	{600 , 23, 8 , 1 , 46, 26, 2 , 39, 18, 3 ,},
	{590 , 23, 8 , 1 , 46, 24, 2 , 39, 17, 3 ,},
	{580 , 22, 8 , 1 , 46, 24, 2 , 39, 17, 3 ,},
	{570 , 22, 8 , 1 , 45, 23, 2 , 38, 17, 3 ,},
	{560 , 22, 7 , 1 , 45, 23, 2 , 38, 16, 2 ,},
	{550 , 21, 7 , 1 , 45, 23, 2 , 38, 16, 2 ,},
	{540 , 21, 7 , 1 , 44, 22, 2 , 37, 16, 2 ,},
	{530 , 20, 7 , 1 , 44, 22, 1 , 37, 15, 2 ,},
	{520 , 20, 7 , 1 , 43, 21, 1 , 37, 15, 2 ,},
	{510 , 20, 6 , 1 , 43, 21, 1 , 36, 15, 2 ,},
	{500 , 19, 6 , 1 , 43, 22, 1 , 36, 15, 2 ,},
};

const u32 b_dphyctl[14] = {
	0x0af, 0x0c8, 0x0e1, 0x0fa,			/* esc 7 ~ 10 */
	0x113, 0x12c, 0x145, 0x15e, 0x177,	/* esc 11 ~ 15 */
	0x190, 0x1a9, 0x1c2, 0x1db, 0x1f4	/* esc 16 ~ 20 */
};

/***************************** DPHY CAL functions *******************************/
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
static void dsim_reg_set_dphy_dither_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, DSIM_PHY_DITHER_EN);
}
#endif

#ifdef DPHY_LOOP
void dsim_reg_set_dphy_loop_back_test(u32 id)
{
	dsim_phy_write_mask(id, 0x0370, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0470, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0570, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0670, 1, (0x3 << 0));
	dsim_phy_write_mask(id, 0x0770, 1, (0x3 << 0));
}

static void dsim_reg_set_dphy_loop_test(u32 id)
{
	dsim_phy_write_mask(id, 0x0374, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0474, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0574, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0674, ~0, (1 << 3));
	dsim_phy_write_mask(id, 0x0774, ~0, (1 << 3));

	dsim_phy_write_mask(id, 0x0374, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0474, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0574, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0674, 0x6, (0x7 << 0));
	dsim_phy_write_mask(id, 0x0774, 0x6, (0x7 << 0));

	dsim_phy_write_mask(id, 0x037c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x047c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x057c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x067c, 0x2, (0xffff << 0));
	dsim_phy_write_mask(id, 0x077c, 0x2, (0xffff << 0));
}
#endif

static void dsim_reg_set_dphy_wclk_buf_sft(u32 id, u32 cnt)
{
	u32 val = DSIM_PHY_WCLK_BUF_SFT_CNT(cnt);

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON6, val, DSIM_PHY_WCLK_BUF_SFT_CNT_MASK);
}

/* DPHY setting */
static void dsim_reg_set_pll_freq(u32 id, u32 p, u32 m, u32 s, u32 k)
{
	u32 val, mask;

	/* K value */
	val = DSIM_PHY_PMS_K(k);
	mask = DSIM_PHY_PMS_K_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON1, val, mask);

	/* P value */
	val = DSIM_PHY_PMS_P(p);
	mask = DSIM_PHY_PMS_P_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON0, val, mask);

	/* M value */
	val = DSIM_PHY_PMS_M(m);
	mask = DSIM_PHY_PMS_M_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val, mask);

	/* S value */
	val = DSIM_PHY_PMS_S(s);
	mask = DSIM_PHY_PMS_S_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON0, val, mask);
}

static void dsim_reg_set_dphy_timing_values(u32 id,
			struct dphy_timing_value *t, u32 hsmode)
{
	u32 val, mask;
	u32 hs_en, skewcal_en;
	u32 i;

	/* HS mode setting */
	if (hsmode) {
		/* under 1500Mbps : don't need SKEWCAL enable */
		hs_en = DSIM_PHY_HSTX_CLK_SEL;
		skewcal_en = 0;
	} else {
		/* above 1500Mbps : need SKEWCAL enable */
		hs_en = 0;
		skewcal_en = 1;
	}

	/* clock lane setting */
	val = DSIM_PHY_ULPS_EXIT(t->b_dphyctl);
	mask = DSIM_PHY_ULPS_EXIT_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON4, val, mask);

	val = hs_en;
	mask = DSIM_PHY_HSTX_CLK_SEL;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON0, val, mask);

	/* skew cal implementation : disable */
	val = skewcal_en;
	mask = DSIM_PHY_SKEWCAL_EN;
	dsim_phy_write_mask(id, DSIM_PHY_MC_DESKEW_CON0, val, mask);
	/* add 'run|init_run|wait_run time' if skewcal is enabled */

	val = DSIM_PHY_TLPX(t->lpx);
	mask = DSIM_PHY_TLPX_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON0, val, mask);

	val = DSIM_PHY_TCLK_PREPARE(t->clk_prepare);
	mask = DSIM_PHY_TCLK_PREPARE_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON1, val, mask);

	val = DSIM_PHY_TCLK_ZERO(t->clk_zero);
	mask = DSIM_PHY_TCLK_ZERO_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON1, val, mask);

	val = DSIM_PHY_THS_EXIT(t->hs_exit);
	mask = DSIM_PHY_THS_EXIT_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON2, val, mask);

	val = DSIM_PHY_TCLK_TRAIL(t->clk_trail);
	mask = DSIM_PHY_TCLK_TRAIL_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON2, val, mask);

	val = DSIM_PHY_TCLK_POST(t->clk_post);
	mask = DSIM_PHY_TCLK_POST_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_TIME_CON3, val, mask);

	/* add other clock lane setting if necessary */

	/* data lane setting : D0 ~ D3 */
	for (i = 0; i < 4; i++) {
		val = DSIM_PHY_ULPS_EXIT(t->b_dphyctl);
		mask = DSIM_PHY_ULPS_EXIT_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON4(i), val, mask);

		val = hs_en;
		mask = DSIM_PHY_HSTX_CLK_SEL;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON0(i), val, mask);

		/* skew cal implementation later */
		val = DSIM_PHY_THS_PREPARE(t->hs_prepare);
		mask = DSIM_PHY_THS_PREPARE_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON1(i), val, mask);

		val = DSIM_PHY_TLPX(t->lpx) | DSIM_PHY_TLP_EXIT_SKEW(0)
			| DSIM_PHY_TLP_ENTRY_SKEW(0);
		mask = DSIM_PHY_TLPX_MASK | DSIM_PHY_TLP_EXIT_SKEW_MASK
			| DSIM_PHY_TLP_ENTRY_SKEW_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON0(i), val, mask);

		val = DSIM_PHY_THS_EXIT(t->hs_exit);
		mask = DSIM_PHY_THS_EXIT_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON2(i), val, mask);

		val = DSIM_PHY_THS_ZERO(t->hs_zero);
		mask = DSIM_PHY_THS_ZERO_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON1(i), val, mask);

		val = DSIM_PHY_THS_TRAIL(t->hs_trail);
		mask = DSIM_PHY_THS_TRAIL_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON2(i), val, mask);

		val = DSIM_PHY_TTA_GET(3) | DSIM_PHY_TTA_GO(0);
		mask = DSIM_PHY_TTA_GET_MASK | DSIM_PHY_TTA_GO_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_MD_TIME_CON3(i), val, mask);

		/* add other clock lane setting if necessary */
	}
}

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
static void dsim_reg_set_dphy_param_dither(u32 id, struct stdphy_pms *dphy_pms)
{
	u32 val, mask;

	/* MFR */
	val = DSIM_PHY_DITHER_MFR(dphy_pms->mfr);
	mask = DSIM_PHY_DITHER_MFR_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON3, val, mask);

	/* MRR */
	val = DSIM_PHY_DITHER_MRR(dphy_pms->mrr);
	mask = DSIM_PHY_DITHER_MRR_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON3, val, mask);

	/* SEL_PF */
	val = DSIM_PHY_DITHER_SEL_PF(dphy_pms->sel_pf);
	mask = DSIM_PHY_DITHER_SEL_PF_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON5, val, mask);

	/* ICP */
	val = DSIM_PHY_DITHER_ICP(dphy_pms->icp);
	mask = DSIM_PHY_DITHER_ICP_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON5, val, mask);

	/* AFC_ENB */
	val = (dphy_pms->afc_enb) ? ~0 : 0;
	mask = DSIM_PHY_DITHER_AFC_ENB;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, mask);

	/* EXTAFC */
	val = DSIM_PHY_DITHER_EXTAFC(dphy_pms->extafc);
	mask = DSIM_PHY_DITHER_EXTAFC_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, mask);

	/* FEED_EN */
	val = (dphy_pms->feed_en) ? ~0 : 0;
	mask = DSIM_PHY_DITHER_FEED_EN;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val, mask);

	/* FSEL */
	val = (dphy_pms->fsel) ? ~0 : 0;
	mask = DSIM_PHY_DITHER_FSEL;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, mask);

	/* FOUT_MASK */
	val = (dphy_pms->fout_mask) ? ~0 : 0;
	mask = DSIM_PHY_DITHER_FOUT_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val, mask);

	/* RSEL */
	val = DSIM_PHY_DITHER_RSEL(dphy_pms->rsel);
	mask = DSIM_PHY_DITHER_RSEL_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON4, val, mask);
}
#endif

/* BIAS Block Control Register */
static void dsim_reg_set_bias_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < 5; i++)
		dsim_phy_extra_write(id, DSIM_PHY_BIAS_CON(i), blk_ctl[i]);
}

/* PLL Control Register */
static void dsim_reg_set_pll_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < (int)(sizeof(DSIM_PHY_PLL_CON_VAL)/
			sizeof(*DSIM_PHY_PLL_CON_VAL)); i++)
		dsim_phy_write(id, DSIM_PHY_PLL_CON(i), blk_ctl[i]);
}

/* Master Clock Lane General Control Register */
static void dsim_reg_set_mc_gnr_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < 2; i++)
		dsim_phy_write(id, DSIM_PHY_MC_GNR_CON(i), blk_ctl[i]);
}

/* Master Clock Lane Analog Block Control Register */
static void dsim_reg_set_mc_ana_con(u32 id, u32 *blk_ctl)
{
	u32 i;
#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	u32 reg_cnt = 2;
#else
	u32 reg_cnt = 3;
#endif

	for (i = 0; i < reg_cnt; i++)
		dsim_phy_write(id, DSIM_PHY_MC_ANA_CON(i), blk_ctl[i]);
}

/* Master Data Lane General Control Register */
static void dsim_reg_set_md_gnr_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		dsim_phy_write(id, DSIM_PHY_MD_GNR_CON0(i), blk_ctl[0]);
		dsim_phy_write(id, DSIM_PHY_MD_GNR_CON1(i), blk_ctl[1]);
	}
}

/* Master Data Lane Analog Block Control Register */
static void dsim_reg_set_md_ana_con(u32 id, u32 *blk_ctl)
{
	u32 i;

	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON0(i), blk_ctl[0]);
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON1(i), blk_ctl[1]);
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON2(i), blk_ctl[2]);
		dsim_phy_write(id, DSIM_PHY_MD_ANA_CON3(i), blk_ctl[3]);
	}
}

#ifdef DPDN_INV_SWAP
void dsim_reg_set_inv_dpdn(u32 id, u32 inv_clk, u32 inv_data[4])
{
	u32 i;
	u32 val, mask;

	val = inv_clk ? (DSIM_PHY_CLK_INV) : 0;
	mask = DSIM_PHY_CLK_INV;
	dsim_phy_write_mask(id, DSIM_PHY_MC_DATA_CON0, val, mask);

	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		val = inv_data[i] ? (DSIM_PHY_DATA_INV) : 0;
		mask = DSIM_PHY_DATA_INV;
		dsim_phy_write_mask(id,  DSIM_PHY_MD_DATA_CON0(i), val, mask);
	}
}

static void dsim_reg_set_dpdn_swap(u32 id, u32 clk_swap)
{
	u32 val, mask;

	val = DSIM_PHY_DPDN_SWAP(clk_swap);
	mask = DSIM_PHY_DPDN_SWAP_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_MC_ANA_CON1, val, mask);
}
#endif

/******************* DSIM CAL functions *************************/
static void dsim_reg_sw_reset(u32 id)
{
	u32 cnt = 1000;
	u32 state;

	dsim_write_mask(id, DSIM_SWRST, ~0, DSIM_SWRST_RESET);

	do {
		state = dsim_read(id, DSIM_SWRST) & DSIM_SWRST_RESET;
		cnt--;
		udelay(10);
	} while (state && cnt);

	if (!cnt)
		dsim_err("%s is timeout.\n", __func__);
}

#if 0
/* this function may be used for later use */
static void dsim_reg_dphy_resetn(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SWRST, val, DSIM_DPHY_RST); /* reset high */
}
#endif

static void dsim_reg_set_num_of_lane(u32 id, u32 lane)
{
	u32 val = DSIM_CONFIG_NUM_OF_DATA_LANE(lane);

	dsim_write_mask(id, DSIM_CONFIG, val,
				DSIM_CONFIG_NUM_OF_DATA_LANE_MASK);
}

static void dsim_reg_enable_lane(u32 id, u32 lane, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_LANES_EN(lane));
}

/*
 * lane_id : 0 = MC, 1 = MD0, 2 = MD1, 3 = MD2, 4 = MD3
 */
static int dsim_reg_wait_phy_ready(u32 id, u32 lane_id, u32 en)
{
	u32 ready, reg_id, val;
	u32 cnt = 1000;

	if (lane_id == 0)
		reg_id = DSIM_PHY_MC_GNR_CON0;
	else
		reg_id = DSIM_PHY_MD_GNR_CON0(lane_id-1);

	do {
		val = dsim_phy_read(id, reg_id);
		ready = DSIM_PHY_PHY_READY_GET(val);
		/* enable case */
		if ((en == 1) && (ready == 1))
			break;
		/* disable case */
		if ((en == 0) && (ready == 0))
			break;

		cnt--;
		udelay(10);
	} while (cnt);

	if (!cnt) {
		dsim_err("PHY lane(%d) is not ready[timeout]\n", lane_id);
		return -EBUSY;
	}

	return 0;
}

static int dsim_reg_enable_lane_phy(u32 id, u32 lane, u32 en)
{
	u32 i, lane_cnt = 0;
	u32 reg_id;
	u32 ret = 0;
	u32 val = en ? ~0 : 0;

	/* check enabled data lane count */
	for (i = 0; i < MAX_DSIM_DATALANE_CNT; i++) {
		if ((lane >> i) & 0x1)
			lane_cnt++;
	}

	/*
	 * [step1] enable phy_enable
	 */

	/* (1.1) clock lane on|off */
	reg_id = DSIM_PHY_MC_GNR_CON0;
	dsim_phy_write_mask(id, reg_id, val, DSIM_PHY_PHY_ENABLE);

	/* (1.2) data lane on|off */
	for (i = 0; i < lane_cnt; i++) {
		reg_id = DSIM_PHY_MD_GNR_CON0(i);
		dsim_phy_write_mask(id, reg_id, val, DSIM_PHY_PHY_ENABLE);
	}

	/*
	 * [step2] wait for phy_ready
	 */

	/* (2.1) check ready of clock lane */
	if (dsim_reg_wait_phy_ready(id, 0, en))
		ret++;

	/* (2.2) check ready of data lanes (index : from '1') */
	for (i = 1; i <= lane_cnt; i++) {
		if (dsim_reg_wait_phy_ready(id, i, en))
			ret++;
	}

	if (ret) {
		dsim_err("Error to enable PHY lane(err=%d)\n", ret);
		return -EBUSY;
	} else
		return 0;
}

static void dsim_reg_pll_stable_time(u32 id, u32 lock_cnt)
{
	u32 val, mask;

	val = DSIM_PHY_PLL_LOCK_CNT(lock_cnt);
	mask = DSIM_PHY_PLL_LOCK_CNT_MASK;
	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON7, val, mask);
}

static void dsim_reg_set_pll(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON0, val, DSIM_PHY_PLL_EN_MASK);
}

static u32 dsim_reg_is_pll_stable(u32 id)
{
	u32 val, pll_lock;

	val = dsim_phy_read(id, DSIM_PHY_PLL_STAT0);
	pll_lock = DSIM_PHY_PLL_LOCK_GET(val);
	if (pll_lock)
		return 1;

	return 0;
}

static int dsim_reg_enable_pll(u32 id, u32 en)
{

	u32 cnt = 1000;

	if (en) {
		dsim_reg_clear_int(id, DSIM_INTSRC_PLL_STABLE);

		dsim_reg_set_pll(id, 1);
		while (1) {
			cnt--;
			if (dsim_reg_is_pll_stable(id))
				return 0;
			if (cnt == 0)
				return -EBUSY;
			udelay(10);
		}
	} else {
		dsim_reg_set_pll(id, 0);
		while (1) {
			cnt--;
			if (!dsim_reg_is_pll_stable(id))
				return 0;
			if (cnt == 0)
				return -EBUSY;
			udelay(10);
		}
	}

	return 0;
}

static void dsim_reg_set_esc_clk_en(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_ESCCLK_EN);
}

static void dsim_reg_set_esc_clk_prescaler(u32 id, u32 en, u32 p)
{
	u32 val = en ? DSIM_CLK_CTRL_ESCCLK_EN : 0;
	u32 mask = DSIM_CLK_CTRL_ESCCLK_EN | DSIM_CLK_CTRL_ESC_PRESCALER_MASK;

	val |= DSIM_CLK_CTRL_ESC_PRESCALER(p);
	dsim_write_mask(id, DSIM_CLK_CTRL, val, mask);
}

static void dsim_reg_set_esc_clk_on_lane(u32 id, u32 en, u32 lane)
{
	u32 val;

	lane = (lane >> 1) | (1 << 4);

	val = en ? DSIM_CLK_CTRL_LANE_ESCCLK_EN(lane) : 0;
	dsim_write_mask(id, DSIM_CLK_CTRL, val,
				DSIM_CLK_CTRL_LANE_ESCCLK_EN_MASK);
}

static void dsim_reg_set_stop_state_cnt(u32 id)
{
	u32 val = DSIM_ESCMODE_STOP_STATE_CNT(DSIM_STOP_STATE_CNT);

	dsim_write_mask(id, DSIM_ESCMODE, val,
				DSIM_ESCMODE_STOP_STATE_CNT_MASK);
}

static void dsim_reg_set_bta_timeout(u32 id)
{
	u32 val = DSIM_TIMEOUT_BTA_TOUT(DSIM_BTA_TIMEOUT);

	dsim_write_mask(id, DSIM_TIMEOUT, val, DSIM_TIMEOUT_BTA_TOUT_MASK);
}

static void dsim_reg_set_lpdr_timeout(u32 id)
{
	u32 val = DSIM_TIMEOUT_LPDR_TOUT(DSIM_LP_RX_TIMEOUT);

	dsim_write_mask(id, DSIM_TIMEOUT, val, DSIM_TIMEOUT_LPDR_TOUT_MASK);
}

static void dsim_reg_disable_hsa(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HSA_DISABLE);
}

static void dsim_reg_disable_hbp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HBP_DISABLE);
}

static void dsim_reg_disable_hfp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HFP_DISABLE);
}

static void dsim_reg_disable_hse(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_HSE_DISABLE);
}

static void dsim_reg_set_burst_mode(u32 id, u32 burst)
{
	u32 val = burst ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_BURST_MODE);
}

static void dsim_reg_set_sync_inform(u32 id, u32 inform)
{
	u32 val = inform ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_SYNC_INFORM);
}

/*
 * For EVT0 0 means enable, 1 means disable.
 * For EVT1 0 means disable, 1 means enable.
 */
static void dsim_reg_set_pll_clk_gate_enable(u32 id, u32 en)
{
	u32 ver, mask, reg_id;
	u32 val = en ? ~0 : 0;

	ver = dsim_read(id, DSIM_VERSION);
	if (ver == DSIM_VER_EVT0) {
		reg_id = 0x0200;
		mask = 0x1;
	} else {
		reg_id = DSIM_CONFIG;
		mask = DSIM_CONFIG_PLL_CLOCK_GATING;
	}
	dsim_write_mask(id, reg_id, val, mask);
}

/* This function is available from EVT1 */
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0) && defined(CONFIG_EXYNOS_PLL_SLEEP)
static void dsim_reg_set_pll_sleep_enable(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PLL_SLEEP);
}
#endif

/* 0=D-PHY, 1=C-PHY */
void dsim_reg_set_phy_selection(u32 id, u32 sel)
{
	u32 val = sel ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PHY_SELECTION);
}

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
static void dsim_reg_set_vfp(u32 id, u32 vfp)
{
	u32 val = DSIM_VPORCH_VFP_TOTAL(vfp);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_VFP_TOTAL_MASK);
}

static void dsim_reg_set_cmdallow(u32 id, u32 cmdallow)
{
	u32 val = DSIM_VPORCH_VFP_CMD_ALLOW(cmdallow);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_VFP_CMD_ALLOW_MASK);
}

static void dsim_reg_set_stable_vfp(u32 id, u32 stablevfp)
{
	u32 val = DSIM_VPORCH_STABLE_VFP(stablevfp);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_STABLE_VFP_MASK);
}

static void dsim_reg_set_vbp(u32 id, u32 vbp)
{
	u32 val = DSIM_VPORCH_VBP(vbp);

	dsim_write_mask(id, DSIM_VPORCH, val, DSIM_VPORCH_VBP_MASK);
}
#else
static void dsim_reg_set_vporch(u32 id, u32 vbp, u32 vfp)
{
	u32 val = DSIM_VPORCH_VFP_TOTAL(vfp) | DSIM_VPORCH_VBP(vbp);
	dsim_write(id, DSIM_VPORCH, val);
}

static void dsim_reg_set_vfp_detail(u32 id, u32 cmd_allow, u32 stable_vfp)
{
	u32 val = DSIM_VPORCH_VFP_CMD_ALLOW(cmd_allow)
		| DSIM_VPORCH_STABLE_VFP(stable_vfp);
	dsim_write(id, DSIM_VFP_DETAIL, val);
}
#endif

static void dsim_reg_set_hfp(u32 id, u32 hfp)
{
	u32 val = DSIM_HPORCH_HFP(hfp);

	dsim_write_mask(id, DSIM_HPORCH, val, DSIM_HPORCH_HFP_MASK);
}

static void dsim_reg_set_hbp(u32 id, u32 hbp)
{
	u32 val = DSIM_HPORCH_HBP(hbp);

	dsim_write_mask(id, DSIM_HPORCH, val, DSIM_HPORCH_HBP_MASK);
}

static void dsim_reg_set_vsa(u32 id, u32 vsa)
{
	u32 val = DSIM_SYNC_VSA(vsa);

	dsim_write_mask(id, DSIM_SYNC, val, DSIM_SYNC_VSA_MASK);
}

static void dsim_reg_set_hsa(u32 id, u32 hsa)
{
	u32 val = DSIM_SYNC_HSA(hsa);

	dsim_write_mask(id, DSIM_SYNC, val, DSIM_SYNC_HSA_MASK);
}

static void dsim_reg_set_vresol(u32 id, u32 vresol)
{
	u32 val = DSIM_RESOL_VRESOL(vresol);

	dsim_write_mask(id, DSIM_RESOL, val, DSIM_RESOL_VRESOL_MASK);
}

static void dsim_reg_set_hresol(u32 id, u32 hresol, struct decon_lcd *lcd)
{
	u32 width, val;

	if (lcd->dsc_enabled)
		width = lcd->dsc_enc_sw * lcd->dsc_slice_num;
	else
		width = hresol;

	val = DSIM_RESOL_HRESOL(width);

	dsim_write_mask(id, DSIM_RESOL, val, DSIM_RESOL_HRESOL_MASK);
}

static void dsim_reg_set_porch(u32 id, struct decon_lcd *lcd)
{
	if (lcd->mode == DECON_VIDEO_MODE) {
#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
		dsim_reg_set_vbp(id, lcd->vbp);
		dsim_reg_set_vfp(id, lcd->vfp);
		dsim_reg_set_stable_vfp(id, DSIM_STABLE_VFP_VALUE);
		dsim_reg_set_cmdallow(id, DSIM_CMD_ALLOW_VALUE);
#else
		/*
		 * In EVT1, v-porch bit-field was extended to 16-bit.
		 * write_mask can't be used to write v-porch field in EVT1
		 * because correct v-porch value can not be read in registers.
		 * So, we changed the 32-bit value to write at once.
		 */
		dsim_reg_set_vporch(id, lcd->vbp, lcd->vfp);
		dsim_reg_set_vfp_detail(id,
				DSIM_CMD_ALLOW_VALUE,
				DSIM_STABLE_VFP_VALUE);
#endif
		dsim_reg_set_hbp(id, lcd->hbp);
		dsim_reg_set_hfp(id, lcd->hfp);
		dsim_reg_set_vsa(id, lcd->vsa);
		dsim_reg_set_hsa(id, lcd->hsa);
	}
}

static void dsim_reg_set_pixel_format(u32 id, u32 pixformat)
{
	u32 val = DSIM_CONFIG_PIXEL_FORMAT(pixformat);

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PIXEL_FORMAT_MASK);
}

static void dsim_reg_set_vc_id(u32 id, u32 vcid)
{
	u32 val = DSIM_CONFIG_VC_ID(vcid);

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_VC_ID_MASK);
}

static void dsim_reg_set_video_mode(u32 id, u32 mode)
{
	u32 val = mode ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_VIDEO_MODE);
}

static int dsim_reg_wait_idle_status(u32 id, u32 is_vm)
{
	u32 cnt = 1000;
	u32 reg_id, val, status;

	if (is_vm)
		reg_id = DSIM_LINK_STATUS0;
	else
		reg_id = DSIM_LINK_STATUS1;

	do {
		val = dsim_read(id, reg_id);
		status = is_vm ? DSIM_LINK_STATUS0_VIDEO_MODE_STATUS_GET(val) :
				DSIM_LINK_STATUS1_CMD_MODE_STATUS_GET(val);
		if (status == DSIM_STATUS_IDLE)
			break;
		cnt--;
		udelay(10);
	} while (cnt);

	if (!cnt) {
		dsim_err("dsim%d wait timeout idle status(%u)\n", id, status);
		return -EBUSY;
	}

	return 0;
}

/* 0 = command, 1 = video mode */
static u32 dsim_reg_get_display_mode(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_CONFIG);
	return DSIM_CONFIG_DISPLAY_MODE_GET(val);
}

static void dsim_reg_enable_dsc(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_CPRS_EN);
}

static void dsim_reg_set_num_of_slice(u32 id, u32 num_of_slice)
{
	u32 val = DSIM_CPRS_CTRL_NUM_OF_SLICE(num_of_slice);

	dsim_write_mask(id, DSIM_CPRS_CTRL, val,
				DSIM_CPRS_CTRL_NUM_OF_SLICE_MASK);
}

static void dsim_reg_get_num_of_slice(u32 id, u32 *num_of_slice)
{
	u32 val = dsim_read(id, DSIM_CPRS_CTRL);

	*num_of_slice = DSIM_CPRS_CTRL_NUM_OF_SLICE_GET(val);
}

static void dsim_reg_set_multi_slice(u32 id, struct decon_lcd *lcd_info)
{
	u32 multi_slice = 1;
	u32 val;

	/* if multi-slice(2~4 slices) DSC compression is used in video mode
	 * MULTI_SLICE_PACKET configuration must be matched
	 * to DDI's configuration
	 */
	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE)
		multi_slice = 1;
	else if (lcd_info->mode == DECON_VIDEO_MODE)
		multi_slice = lcd_info->dsc_slice_num > 1 ? 1 : 0;

	/* if MULTI_SLICE_PACKET is enabled,
	 * only one packet header is transferred
	 * for multi slice
	 */
	val = multi_slice ? ~0 : 0;
	dsim_write_mask(id, DSIM_CPRS_CTRL, val,
				DSIM_CPRS_CTRL_MULI_SLICE_PACKET);
}

static void dsim_reg_set_size_of_slice(u32 id, struct decon_lcd *lcd_info)
{
	u32 slice_w = lcd_info->dsc_dec_sw;
	u32 val_01 = 0, mask_01 = 0;
	u32 val_23 = 0, mask_23 = 0;

	if (lcd_info->dsc_slice_num == 4) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		mask_01 = DSIM_SLICE01_SIZE_OF_SLICE1_MASK |
			DSIM_SLICE01_SIZE_OF_SLICE0_MASK;
		val_23 = DSIM_SLICE23_SIZE_OF_SLICE3(slice_w) |
			DSIM_SLICE23_SIZE_OF_SLICE2(slice_w);
		mask_23 = DSIM_SLICE23_SIZE_OF_SLICE3_MASK |
			DSIM_SLICE23_SIZE_OF_SLICE2_MASK;

		dsim_write_mask(id, DSIM_SLICE01, val_01, mask_01);
		dsim_write_mask(id, DSIM_SLICE23, val_23, mask_23);
	} else if (lcd_info->dsc_slice_num == 2) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE1(slice_w) |
			DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		mask_01 = DSIM_SLICE01_SIZE_OF_SLICE1_MASK |
			DSIM_SLICE01_SIZE_OF_SLICE0_MASK;

		dsim_write_mask(id, DSIM_SLICE01, val_01, mask_01);
	} else if (lcd_info->dsc_slice_num == 1) {
		val_01 = DSIM_SLICE01_SIZE_OF_SLICE0(slice_w);
		mask_01 = DSIM_SLICE01_SIZE_OF_SLICE0_MASK;

		dsim_write_mask(id, DSIM_SLICE01, val_01, mask_01);
	} else {
		dsim_err("not supported slice mode. dsc(%d), slice(%d)\n",
				lcd_info->dsc_cnt, lcd_info->dsc_slice_num);
	}
}

static void dsim_reg_print_size_of_slice(u32 id)
{
	u32 val;
	u32 slice0_w, slice1_w, slice2_w, slice3_w;

	val = dsim_read(id, DSIM_SLICE01);
	slice0_w = DSIM_SLICE01_SIZE_OF_SLICE0_GET(val);
	slice1_w = DSIM_SLICE01_SIZE_OF_SLICE1_GET(val);

	val = dsim_read(id, DSIM_SLICE23);
	slice2_w = DSIM_SLICE23_SIZE_OF_SLICE2_GET(val);
	slice3_w = DSIM_SLICE23_SIZE_OF_SLICE3_GET(val);

	dsim_dbg("dsim%d: slice0 w(%d), slice1 w(%d), slice2 w(%d), slice3(%d)\n",
			id, slice0_w, slice1_w, slice2_w, slice3_w);
}

static void dsim_reg_set_multi_packet_count(u32 id, u32 multipacketcnt)
{
	u32 val = DSIM_CMD_CONFIG_MULTI_PKT_CNT(multipacketcnt);

	dsim_write_mask(id, DSIM_CMD_CONFIG, val,
				DSIM_CMD_CONFIG_MULTI_PKT_CNT_MASK);
}

static void dsim_reg_set_time_stable_vfp(u32 id, u32 stablevfp)
{
	u32 val = DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP(stablevfp);

	dsim_write_mask(id, DSIM_CMD_TE_CTRL0, val,
				DSIM_CMD_TE_CTRL0_TIME_STABLE_VFP_MASK);
}

static void dsim_reg_set_time_te_protect_on(u32 id, u32 teprotecton)
{
	u32 val = DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON(teprotecton);

	dsim_write_mask(id, DSIM_CMD_TE_CTRL1, val,
			DSIM_CMD_TE_CTRL1_TIME_TE_PROTECT_ON_MASK);
}

static void dsim_reg_set_time_te_timeout(u32 id, u32 tetout)
{
	u32 val = DSIM_CMD_TE_CTRL1_TIME_TE_TOUT(tetout);

	dsim_write_mask(id, DSIM_CMD_TE_CTRL1, val,
				DSIM_CMD_TE_CTRL1_TIME_TE_TOUT_MASK);
}

static void dsim_reg_set_cmd_ctrl(u32 id, struct decon_lcd *lcd_info,
						struct dsim_clks *clks)
{
	unsigned int time_stable_vfp;
	unsigned int time_te_protect_on;
	unsigned int time_te_tout;

	time_stable_vfp = lcd_info->xres * DSIM_STABLE_VFP_VALUE * 3 / 100;
	time_te_protect_on = (clks->hs_clk * TE_PROTECT_ON_TIME) / 16;
	time_te_tout = (clks->hs_clk * TE_TIMEOUT_TIME) / 16;
	dsim_reg_set_time_stable_vfp(id, time_stable_vfp);
	dsim_reg_set_time_te_protect_on(id, time_te_protect_on);
	dsim_reg_set_time_te_timeout(id, time_te_tout);
}

static void dsim_reg_enable_noncont_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val,
				DSIM_CLK_CTRL_NONCONT_CLOCK_LANE);
}

static void dsim_reg_enable_clocklane(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val,
				DSIM_CLK_CTRL_CLKLANE_ONOFF);
}

static void dsim_reg_enable_packetgo(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CMD_CONFIG, val,
				DSIM_CMD_CONFIG_PKT_GO_EN);
}

static void dsim_reg_enable_multi_cmd_packet(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CMD_CONFIG, val,
				DSIM_CMD_CONFIG_MULTI_CMD_PKT_EN);
}

static void dsim_reg_enable_shadow(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SFR_CTRL, val,
				DSIM_SFR_CTRL_SHADOW_EN);
}

static void dsim_reg_set_link_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_CLOCK_SEL);
}

static int dsim_reg_get_link_clock(u32 id)
{
	int val = 0;

	val = dsim_read_mask(id, DSIM_CLK_CTRL, DSIM_CLK_CTRL_CLOCK_SEL);

	return val;
}

static void dsim_reg_enable_hs_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_TX_REQUEST_HSCLK);
}

static void dsim_reg_enable_word_clock(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CLK_CTRL, val, DSIM_CLK_CTRL_WORDCLK_EN);
}

u32 dsim_reg_is_hs_clk_ready(u32 id)
{
	if (dsim_read(id, DSIM_DPHY_STATUS) & DSIM_DPHY_STATUS_TX_READY_HS_CLK)
		return 1;

	return 0;
}

u32 dsim_reg_is_clk_stop(u32 id)
{
	if (dsim_read(id, DSIM_DPHY_STATUS) & DSIM_DPHY_STATUS_STOP_STATE_CLK)
		return 1;

	return 0;
}

void dsim_reg_enable_per_frame_read(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_PER_FRAME_READ_EN);
}

int dsim_reg_wait_hs_clk_ready(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_hs_clk_ready(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not HS state.\n");
		return -EBUSY;
	}

	return 0;
}

int dsim_reg_wait_hs_clk_disable(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_clk_stop(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master clock isn't disabled.\n");
		return -EBUSY;
	}

	return 0;
}

void dsim_reg_force_dphy_stop_state(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_FORCE_STOP_STATE);
}

void dsim_reg_enter_ulps(u32 id, u32 enter)
{
	u32 val = enter ? ~0 : 0;
	u32 mask = DSIM_ESCMODE_TX_ULPS_CLK | DSIM_ESCMODE_TX_ULPS_DATA;

	dsim_write_mask(id, DSIM_ESCMODE, val, mask);
}

void dsim_reg_exit_ulps(u32 id, u32 exit)
{
	u32 val = exit ? ~0 : 0;
	u32 mask = DSIM_ESCMODE_TX_ULPS_CLK_EXIT |
			DSIM_ESCMODE_TX_ULPS_DATA_EXIT;

	dsim_write_mask(id, DSIM_ESCMODE, val, mask);
}

void dsim_reg_set_num_of_transfer(u32 id, u32 num_of_transfer)
{
	u32 val = DSIM_NUM_OF_TRANSFER_PER_FRAME(num_of_transfer);

	dsim_write_mask(id, DSIM_NUM_OF_TRANSFER, val,
				DSIM_NUM_OF_TRANSFER_PER_FRAME_MASK);

	dsim_dbg("%s, write value : 0x%x, read value : 0x%x\n", __func__,
			val, dsim_read(id, DSIM_NUM_OF_TRANSFER));
}

static u32 dsim_reg_is_ulps_state(u32 id, u32 lanes)
{
	u32 val = dsim_read(id, DSIM_DPHY_STATUS);
	u32 data_lane = lanes >> DSIM_LANE_CLOCK;

	if ((DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(val) == data_lane)
			&& (val & DSIM_DPHY_STATUS_ULPS_CLK))
		return 1;

	return 0;
}

static void dsim_reg_set_deskew_hw_interval(u32 id, u32 interval)
{
	u32 val = DSIM_DESKEW_CTRL_HW_INTERVAL(interval);

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val,
			DSIM_DESKEW_CTRL_HW_INTERVAL_MASK);
}

static void dsim_reg_set_deskew_hw_position(u32 id, u32 position)
{
	u32 val = position ? ~0 : 0;

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val,
				DSIM_DESKEW_CTRL_HW_POSITION);
}

static void dsim_reg_enable_deskew_hw_enable(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_DESKEW_CTRL, val, DSIM_DESKEW_CTRL_HW_EN);
}

static void dsim_reg_set_cm_underrun_lp_ref(u32 id, u32 lp_ref)
{
	u32 val = DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF(lp_ref);

	dsim_write_mask(id, DSIM_UNDERRUN_CTRL, val,
			DSIM_UNDERRUN_CTRL_CM_UNDERRUN_LP_REF_MASK);
}

static void dsim_reg_set_threshold(u32 id, u32 threshold)
{
	u32 val = DSIM_THRESHOLD_LEVEL(threshold);

	dsim_write_mask(id, DSIM_THRESHOLD, val, DSIM_THRESHOLD_LEVEL_MASK);

}

static void dsim_reg_enable_eotp(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_CONFIG, val, DSIM_CONFIG_EOTP_EN);
}

static void dsim_reg_set_vt_compensate(u32 id, u32 compensate)
{
	u32 val = DSIM_VIDEO_TIMER_COMPENSATE(compensate);

	dsim_write_mask(id, DSIM_VIDEO_TIMER, val,
			DSIM_VIDEO_TIMER_COMPENSATE_MASK);
}

static void dsim_reg_set_vstatus_int(u32 id, u32 vstatus)
{
	u32 val = DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL(vstatus);

	dsim_write_mask(id, DSIM_VIDEO_TIMER, val,
			DSIM_VIDEO_TIMER_VSTATUS_INTR_SEL_MASK);
}

static void dsim_reg_set_bist_te_interval(u32 id, u32 interval)
{
	u32 val = DSIM_BIST_CTRL0_BIST_TE_INTERVAL(interval);

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_TE_INTERVAL_MASK);
}

static void dsim_reg_set_bist_mode(u32 id, u32 bist_mode)
{
	u32 val = DSIM_BIST_CTRL0_BIST_PTRN_MODE(bist_mode);

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_PTRN_MODE_MASK);
}

static void dsim_reg_enable_bist_pattern_move(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_BIST_CTRL0, val,
			DSIM_BIST_CTRL0_BIST_PTRN_MOVE_EN);
}

static void dsim_reg_enable_bist(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_BIST_CTRL0, val, DSIM_BIST_CTRL0_BIST_EN);
}

static void dsim_set_hw_deskew(u32 id, u32 en)
{
	u32 hw_interval = 1;

	if (en) {
		dsim_reg_set_deskew_hw_interval(id, hw_interval);
		/* 0 : VBP first line, 1 : VFP last line*/
		dsim_reg_set_deskew_hw_position(id, 0);
		dsim_reg_enable_deskew_hw_enable(id, en);
	} else {
		dsim_reg_enable_deskew_hw_enable(id, en);
	}
}

static int dsim_reg_wait_enter_ulps_state(u32 id, u32 lanes)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_ulps_state(id, lanes);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not ULPS state.\n");
		return -EBUSY;
	}

	dsim_dbg("DSI Master is ULPS state.\n");

	return 0;
}

static u32 dsim_reg_is_not_ulps_state(u32 id)
{
	u32 val = dsim_read(id, DSIM_DPHY_STATUS);

	if (!(DSIM_DPHY_STATUS_ULPS_DATA_LANE_GET(val))
			&& !(val & DSIM_DPHY_STATUS_ULPS_CLK))
		return 1;

	return 0;
}

static int dsim_reg_wait_exit_ulps_state(u32 id)
{
	u32 state;
	u32 cnt = 1000;

	do {
		state = dsim_reg_is_not_ulps_state(id);
		cnt--;
		udelay(10);
	} while (!state && cnt);

	if (!cnt) {
		dsim_err("DSI Master is not stop state.\n");
		return -EBUSY;
	}

	dsim_dbg("DSI Master is stop state.\n");

	return 0;
}

static int dsim_reg_get_dphy_timing(u32 hs_clk, u32 esc_clk,
		struct dphy_timing_value *t)
{
	int i;

	i = ARRAY_SIZE(dphy_timing) - 1;

	for (; i >= 0; i--) {
		if (dphy_timing[i][0] < hs_clk) {
			continue;
		} else {
			t->bps = hs_clk;
			t->clk_prepare = dphy_timing[i][1];
			t->clk_zero = dphy_timing[i][2];
			t->clk_post = dphy_timing[i][3];
			t->clk_trail = dphy_timing[i][4];
			t->hs_prepare = dphy_timing[i][5];
			t->hs_zero = dphy_timing[i][6];
			t->hs_trail = dphy_timing[i][7];
			t->lpx = dphy_timing[i][8];
			t->hs_exit = dphy_timing[i][9];
			break;
		}
	}

	if (i < 0) {
		dsim_err("%u Mhz hs clock can't find proper dphy timing values\n",
				hs_clk);
		return -EINVAL;
	}

	dsim_dbg("%s: bps(%u) clk_prepare(%u) clk_zero(%u) clk_post(%u)\n",
			__func__, t->bps, t->clk_prepare, t->clk_zero,
			t->clk_post);
	dsim_dbg("clk_trail(%u) hs_prepare(%u) hs_zero(%u) hs_trail(%u)\n",
			t->clk_trail, t->hs_prepare, t->hs_zero, t->hs_trail);
	dsim_dbg("lpx(%u) hs_exit(%u)\n", t->lpx, t->hs_exit);

	if ((esc_clk > 20) || (esc_clk < 7)) {
		dsim_err("%u Mhz cann't be used as escape clock\n", esc_clk);
		return -EINVAL;
	}

	t->b_dphyctl = b_dphyctl[esc_clk - 7];
	dsim_dbg("b_dphyctl(%u)\n", t->b_dphyctl);

	return 0;
}

static void dsim_reg_set_config(u32 id, struct decon_lcd *lcd_info,
		struct dsim_clks *clks)
{
	u32 threshold;
	u32 num_of_slice;
	u32 num_of_transfer;
	int idx;

	/* shadow read disable */
	dsim_reg_enable_shadow_read(id, 1);
	/* dsim_reg_enable_shadow(id, 1); */

	if (lcd_info->mode == DECON_VIDEO_MODE)
		dsim_reg_enable_clocklane(id, 0);
	else
		dsim_reg_enable_noncont_clock(id, 1);

	dsim_set_hw_deskew(id, 0); /* second param is to control enable bit */

	dsim_reg_set_bta_timeout(id);
	dsim_reg_set_lpdr_timeout(id);

	dsim_reg_set_cmd_transfer_mode(id, 0);
	dsim_reg_set_stop_state_cnt(id);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		/* DSU_MODE_1 is used in stead of 1 in MCD */
		idx = lcd_info->mres_mode - DSU_MODE_1;
		dsim_reg_set_cm_underrun_lp_ref(id,
				lcd_info->cmd_underrun_lp_ref[idx]);
	}

	if (lcd_info->dsc_enabled)
		threshold = lcd_info->dsc_enc_sw * lcd_info->dsc_slice_num;
	else
		threshold = lcd_info->xres;

	dsim_reg_set_threshold(id, threshold);

	dsim_reg_set_vresol(id, lcd_info->yres);
	dsim_reg_set_hresol(id, lcd_info->xres, lcd_info);
	dsim_reg_set_porch(id, lcd_info);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		if (lcd_info->dsc_enabled)
			/* use 1-line transfer only */
			num_of_transfer = lcd_info->yres;
		else
			num_of_transfer = lcd_info->xres * lcd_info->yres
							/ threshold;

		dsim_reg_set_num_of_transfer(id, num_of_transfer);
	} else {
		num_of_transfer = lcd_info->xres * lcd_info->yres / threshold;
		dsim_reg_set_num_of_transfer(id, num_of_transfer);
	}

	dsim_reg_set_num_of_lane(id, lcd_info->data_lane - 1);
	dsim_reg_enable_eotp(id, 1);
	dsim_reg_enable_per_frame_read(id, 0);
	dsim_reg_set_pixel_format(id, DSIM_PIXEL_FORMAT_RGB24);
	dsim_reg_set_vc_id(id, 0);

	/* CPSR & VIDEO MODE can be set when shadow enable on */
	/* shadow enable */
	dsim_reg_enable_shadow(id, 1);
	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_video_mode(id, DSIM_CONFIG_VIDEO_MODE);
		dsim_dbg("%s: video mode set\n", __func__);
	} else {
		dsim_reg_set_video_mode(id, 0);
		dsim_dbg("%s: command mode set\n", __func__);
	}

	dsim_reg_enable_dsc(id, lcd_info->dsc_enabled);

	/* shadow disable */
	dsim_reg_enable_shadow(id, 0);

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_disable_hsa(id, 0);
		dsim_reg_disable_hbp(id, 0);
		dsim_reg_disable_hfp(id, 1);
		dsim_reg_disable_hse(id, 0);
		dsim_reg_set_burst_mode(id, 1);
		dsim_reg_set_sync_inform(id, 0);
		dsim_reg_enable_clocklane(id, 0);
	}

	if (lcd_info->dsc_enabled) {
		dsim_dbg("%s: dsc configuration is set\n", __func__);
		dsim_reg_set_num_of_slice(id, lcd_info->dsc_slice_num);
		dsim_reg_set_multi_slice(id, lcd_info); /* multi slice */
		dsim_reg_set_size_of_slice(id, lcd_info);

		dsim_reg_get_num_of_slice(id, &num_of_slice);
		dsim_dbg("dsim%d: number of DSC slice(%d)\n", id, num_of_slice);
		dsim_reg_print_size_of_slice(id);
	}

	if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_multi_packet_count(id, 0xff);
		dsim_reg_enable_multi_cmd_packet(id, 0);
	}
	dsim_reg_enable_packetgo(id, 0);

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		dsim_reg_set_cmd_ctrl(id, lcd_info, clks);
	} else if (lcd_info->mode == DECON_VIDEO_MODE) {
		dsim_reg_set_vt_compensate(id, lcd_info->vt_compensation);
		dsim_reg_set_vstatus_int(id, DSIM_VSYNC);
	}

	/* dsim_reg_enable_shadow_read(id, 1); */
	/* dsim_reg_enable_shadow(id, 1); */

}

/*
 * configure and set DPHY PLL, byte clock, escape clock and hs clock
 *	- PMS value have to be optained by using PMS Gen.
 *      tool (MSC_PLL_WIZARD2_00.exe)
 *	- PLL out is source clock of HS clock
 *	- byte clock = HS clock / 16
 *	- calculate divider of escape clock using requested escape clock
 *	  from driver
 *	- DPHY PLL, byte clock, escape clock are enabled.
 *	- HS clock will be enabled another function.
 *
 * Parameters
 *	- hs_clk : in/out parameter.
 *		in :  requested hs clock. out : calculated hs clock
 *	- esc_clk : in/out paramter.
 *		in : requested escape clock. out : calculated escape clock
 *	- word_clk : out parameter. byte clock = hs clock / 16
 */
static int dsim_reg_set_clocks(u32 id, struct dsim_clks *clks,
		struct stdphy_pms *dphy_pms, u32 en)
{
	unsigned int esc_div;
	struct dsim_pll_param pll;
	struct dphy_timing_value t;
	u32 pll_lock_cnt;
	int ret = 0;
	u32 hsmode = 0;
#ifdef DPDN_INV_SWAP
	u32 inv_data[4] = {0, };
#endif

	if (en) {
		/*
		 * Do not need to set clocks related with PLL,
		 * if DPHY_PLL is already stabled because of LCD_ON_UBOOT.
		 */
		if (dsim_reg_is_pll_stable(id)) {
			dsim_info("dsim%d DPHY PLL is already stable\n", id);
			return -EBUSY;
		}

		/*
		 * set p, m, s to DPHY PLL
		 * PMS value has to be optained by PMS calculation tool
		 * released to customer
		 */
		pll.p = dphy_pms->p;
		pll.m = dphy_pms->m;
		pll.s = dphy_pms->s;
		pll.k = dphy_pms->k;

		/* get word clock */
		/* clks ->hs_clk is from DT */
		clks->word_clk = clks->hs_clk / 16;
		dsim_dbg("word clock is %u MHz\n", clks->word_clk);

		/* requeseted escape clock */
		dsim_dbg("requested escape clock %u MHz\n", clks->esc_clk);

		/* escape clock divider */
		esc_div = clks->word_clk / clks->esc_clk;

		/* adjust escape clock */
		if ((clks->word_clk / esc_div) > clks->esc_clk)
			esc_div += 1;

		/* adjusted escape clock */
		clks->esc_clk = clks->word_clk / esc_div;
		dsim_dbg("escape clock divider is 0x%x\n", esc_div);
		dsim_dbg("escape clock is %u MHz\n", clks->esc_clk);

		/* set BIAS ctrl : default value */
		dsim_reg_set_bias_con(id, DSIM_PHY_BIAS_CON_VAL);

		/* set PLL ctrl : default value */
		dsim_reg_set_pll_con(id, DSIM_PHY_PLL_CON_VAL);

		if (clks->hs_clk < 1500)
			hsmode = 1;

		dsim_reg_set_esc_clk_prescaler(id, 1, esc_div);
		/* get DPHY timing values using hs clock and escape clock */
		dsim_reg_get_dphy_timing(clks->hs_clk, clks->esc_clk, &t);
		dsim_reg_set_dphy_timing_values(id, &t, hsmode);
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
		/* check dither sequence */
		dsim_reg_set_dphy_param_dither(id, dphy_pms);
		dsim_reg_set_dphy_dither_en(id, 1);
#endif

		/* set clock lane General Control Register control */
		dsim_reg_set_mc_gnr_con(id, DSIM_PHY_MC_GNR_CON_VAL);

		/* set clock lane Analog Block Control Register control */
		dsim_reg_set_mc_ana_con(id, DSIM_PHY_MC_ANA_CON_VAL);

#ifdef DPDN_INV_SWAP
		dsim_reg_set_dpdn_swap(id, 1);
#endif

		/* set data lane General Control Register control */
		dsim_reg_set_md_gnr_con(id, DSIM_PHY_MD_GNR_CON_VAL);

#ifdef DPDN_INV_SWAP
		inv_data[0] = 0;
		inv_data[1] = 1;
		inv_data[2] = 1;
		inv_data[3] = 0;
		dsim_reg_set_inv_dpdn(id, 0, inv_data);
#endif

		/* set data lane Analog Block Control Register control */
		dsim_reg_set_md_ana_con(id, DSIM_PHY_MD_ANA_CON_VAL);

		/* set PMSK on PHY */
		dsim_reg_set_pll_freq(id, pll.p, pll.m, pll.s, pll.k);

		/*set wclk buf sft cnt */
		dsim_reg_set_dphy_wclk_buf_sft(id, 3);

		/* set PLL's lock time (lock_cnt)
		 * PLL lock cnt setting guide
		 * PLL_LOCK_CNT_MULT = 500
		 * PLL_LOCK_CNT_MARGIN = 10 (10%)
		 * PLL lock time = PLL_LOCK_CNT_MULT * Tp
		 * Tp = 1 / (OSC clk / pll_p)
		 * PLL lock cnt = PLL lock time * OSC clk
		 */
		pll_lock_cnt = (PLL_LOCK_CNT_MULT + PLL_LOCK_CNT_MARGIN) * pll.p;
		dsim_reg_pll_stable_time(id, pll_lock_cnt);

#ifdef DPHY_LOOP
		dsim_reg_set_dphy_loop_test(id);
#endif
		/* enable PLL */
		ret = dsim_reg_enable_pll(id, 1);
	} else {
		/* check disable PHY timing */
		/* TBD */
		dsim_reg_set_esc_clk_prescaler(id, 0, 0xff);
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
		/* check dither sequence */
		dsim_reg_set_dphy_dither_en(id, 0);
#endif
		dsim_reg_enable_pll(id, 0);
	}

	return ret;
}

static int dsim_reg_set_lanes(u32 id, u32 lanes, u32 en)
{
	/* LINK lanes */
	dsim_reg_enable_lane(id, lanes, en);

	return 0;
}

static int dsim_reg_set_lanes_dphy(u32 id, u32 lanes, u32 en)
{
	/* PHY lanes */
	if (dsim_reg_enable_lane_phy(id, (lanes >> 1), en))
		return -EBUSY;

	return 0;
}

static u32 dsim_reg_is_noncont_clk_enabled(u32 id)
{
	int ret;

	ret = dsim_read_mask(id, DSIM_CLK_CTRL,
					DSIM_CLK_CTRL_NONCONT_CLOCK_LANE);
	return ret;
}

static int dsim_reg_set_hs_clock(u32 id, u32 en)
{
	int reg = 0;
	int is_noncont = dsim_reg_is_noncont_clk_enabled(id);

	if (en) {
		dsim_reg_enable_hs_clock(id, 1);
		if (!is_noncont)
			reg = dsim_reg_wait_hs_clk_ready(id);
	} else {
		dsim_reg_enable_hs_clock(id, 0);
		reg = dsim_reg_wait_hs_clk_disable(id);
	}
	return reg;
}

static void dsim_reg_set_int(u32 id, u32 en)
{
	u32 val = en ? 0 : ~0;
	u32 mask;

	/*
	 * TODO: underrun irq will be unmasked in the future.
	 * underrun irq(dsim_reg_set_config) is ignored in zebu emulator.
	 * it's not meaningful
	 */
	mask = DSIM_INTMSK_SW_RST_RELEASE | DSIM_INTMSK_SFR_PL_FIFO_EMPTY |
		DSIM_INTMSK_SFR_PH_FIFO_EMPTY |
		DSIM_INTMSK_FRAME_DONE | DSIM_INTMSK_INVALID_SFR_VALUE |
		DSIM_INTMSK_UNDER_RUN | DSIM_INTMSK_RX_DATA_DONE |
		DSIM_INTMSK_ERR_RX_ECC | DSIM_INTMSK_VT_STATUS;

	dsim_write_mask(id, DSIM_INTMSK, val, mask);
}

/*
 * enter or exit ulps mode
 *
 * Parameter
 *	1 : enter ULPS mode
 *	0 : exit ULPS mode
 */
static int dsim_reg_set_ulps(u32 id, u32 en, u32 lanes)
{
	int ret = 0;

	if (en) {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;

	} else {
		/* Exit ULPS clock and data lane */
		dsim_reg_exit_ulps(id, 1);

		ret = dsim_reg_wait_exit_ulps_state(id);
		if (ret)
			return ret;

		/* wait at least 1ms : Twakeup time for MARK1 state  */
		udelay(1000);

		/* Clear ULPS exit request */
		dsim_reg_exit_ulps(id, 0);

		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	}

	return ret;
}

/*
 * enter or exit ulps mode for LSI DDI
 *
 * Parameter
 *	1 : enter ULPS mode
 *	0 : exit ULPS mode
 * assume that disp block power is off after ulps mode enter
 */
static int dsim_reg_set_smddi_ulps(u32 id, u32 en, u32 lanes)
{
	int ret = 0;

	if (en) {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;
		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);
	} else {
		/* Enable ULPS clock and data lane */
		dsim_reg_enter_ulps(id, 1);

		/* Check ULPS request for data lane */
		ret = dsim_reg_wait_enter_ulps_state(id, lanes);
		if (ret)
			return ret;

		/* Exit ULPS clock and data lane */
		dsim_reg_exit_ulps(id, 1);

		ret = dsim_reg_wait_exit_ulps_state(id);
		if (ret)
			return ret;

		/* wait at least 1ms : Twakeup time for MARK1 state */
		udelay(100);

		/* Clear ULPS enter request */
		dsim_reg_enter_ulps(id, 0);

		/* Clear ULPS exit request */
		dsim_reg_exit_ulps(id, 0);
	}

	return ret;
}

static int dsim_reg_set_ulps_by_ddi(u32 id, u32 ddi_type, u32 lanes, u32 en)
{
	int ret;

	switch (ddi_type) {
	case TYPE_OF_SM_DDI:
		ret = dsim_reg_set_smddi_ulps(id, en, lanes);
		break;
	case TYPE_OF_MAGNA_DDI:
		dsim_err("This ddi(%d) doesn't support ULPS\n", ddi_type);
		ret = -EINVAL;
		break;
	case TYPE_OF_NORMAL_DDI:
	default:
		ret = dsim_reg_set_ulps(id, en, lanes);
		break;
	}

	if (ret < 0)
		dsim_err("%s: failed to %s ULPS", __func__,
				en ? "enter" : "exit");

	return ret;
}

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
static void dsim_reg_set_dphy_shadow_update_req(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_OPTION_SUITE, val,
			DSIM_OPTION_SUITE_UPDT_EN_MASK);
}

static void dsim_reg_set_dphy_use_shadow(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val,
			DSIM_PHY_PLL_CON2_USE_SDW_MASK);
}

static void dsim_reg_set_dphy_pll_stable_cnt(u32 id, u32 cnt)
{
	u32 val = DSIM_PHY_PLL_CON8_PLL_STB_CNT(cnt);

	dsim_phy_write_mask(id, DSIM_PHY_PLL_CON8, val,
			DSIM_PHY_PLL_CON8_PLL_STB_CNT_MASK);
}
#endif

/******************** EXPORTED DSIM CAL APIs ********************/

void dpu_sysreg_select_dphy_rst_control(void __iomem *sysreg, u32 dsim_id, u32 sel)
{
	u32 phy_num = dsim_id ? 0 : 1;
	u32 old = readl(sysreg + DISP_DPU_MIPI_PHY_CON);
	u32 val = sel ? ~0 : 0;
	u32 mask = SEL_RESET_DPHY_MASK(phy_num);

	val = (val & mask) | (old & ~mask);
	writel(val, sysreg + DISP_DPU_MIPI_PHY_CON);
	dsim_dbg("%s: phy_con_sel val=0x%x", __func__, val);
}

void dpu_sysreg_dphy_reset(void __iomem *sysreg, u32 dsim_id, u32 rst)
{
	u32 old = readl(sysreg + DISP_DPU_MIPI_PHY_CON);
	u32 val = rst ? ~0 : 0;
	u32 mask = dsim_id ? M_RESETN_M4S4_TOP_MASK : M_RESETN_M4S4_MODULE_MASK;

	val = (val & mask) | (old & ~mask);
	writel(val, sysreg + DISP_DPU_MIPI_PHY_CON);
	dsim_dbg("%s: phy_con_rst val=0x%x", __func__, val);
}

static u32 dsim_reg_translate_lanecnt_to_lanes(int lanecnt)
{
	u32 lanes, i;

	lanes = DSIM_LANE_CLOCK;
	for (i = 1; i <= lanecnt; ++i)
		lanes |= 1 << i;

	return lanes;
}

int dsim_reg_init(u32 id, struct decon_lcd *lcd_info, struct dsim_clks *clks,
		bool panel_ctrl)
{
	int ret = 0;
	u32 lanes;
#if !defined(CONFIG_EXYNOS_LCD_ON_UBOOT)
	struct dsim_device *dsim = get_dsim_drvdata(id);
#endif
	if (dsim->state == DSIM_STATE_INIT) {
		if (dsim_reg_get_link_clock(dsim->id)) {
			dsim_info("dsim%d is already enabled in bootloader\n", dsim->id);
			return 0;
		}
	}

	lanes = dsim_reg_translate_lanecnt_to_lanes(lcd_info->data_lane);

	/* choose OSC_CLK */
	dsim_reg_set_link_clock(id, 0);

	dsim_reg_sw_reset(id);

	dsim_reg_set_lanes(id, lanes, 1);

	dsim_reg_set_esc_clk_on_lane(id, 1, lanes);

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	dsim_reg_set_pll_clk_gate_enable(id, 1); /* PHY pll clock gate disable */
#endif

	dsim_reg_enable_word_clock(id, 1);

	/* Enable DPHY reset : DPHY reset start */
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 0);

#if defined(CONFIG_EXYNOS_LCD_ON_UBOOT)
	/* TODO: This code will be implemented as uboot style */
#else
	/* Panel power on */
	if (panel_ctrl)
		ret = dsim_set_panel_power(dsim, 1);
#endif

	dsim_reg_set_clocks(id, clks, &lcd_info->dphy_pms, 1);

	dsim_reg_set_lanes_dphy(id, lanes, 1);
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 1); /* Release DPHY reset */

	dsim_reg_set_link_clock(id, 1);	/* Selection to word clock */

	dsim_reg_set_config(id, lcd_info, clks);

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	dsim_reg_set_pll_clk_gate_enable(id, 0); /* PHY pll clock gate enable */
	dsim_reg_set_pll_clk_gate_enable(id, 1); /* PHY pll clock gate disable */
#else
	dsim_reg_set_pll_clk_gate_enable(id, DPHY_PLL_CLK_GATE_EN); /* PHY pll clock gate disable */
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
	dsim_reg_set_pll_sleep_enable(id, true);	/* PHY pll sleep enable */
#endif
#endif

#if !defined(CONFIG_EXYNOS_COMMON_PANEL)
#if defined(CONFIG_EXYNOS_LCD_ON_UBOOT)
	/* TODO: This code will be implemented as uboot style */
#else
	if (!ret && panel_ctrl)
		ret = dsim_reset_panel(dsim);
#endif
#endif

	return ret;
}

/* Set clocks and lanes and HS ready */
void dsim_reg_start(u32 id)
{
	dsim_reg_set_hs_clock(id, 1);
	dsim_reg_set_int(id, 1);
}

/* Unset clocks and lanes and stop_state */
int dsim_reg_stop(u32 id, u32 lanes)
{
	int err = 0;
	u32 is_vm;

	struct dsim_device *dsim = get_dsim_drvdata(id);

	dsim_reg_clear_int(id, 0xffffffff);
	/* disable interrupts */
	dsim_reg_set_int(id, 0);

	/* first disable HS clock */
	if (dsim_reg_set_hs_clock(id, 0) < 0)
		dsim_err("The CLK lane doesn't be switched to LP mode\n");

	/* 0. wait the IDLE status */
	is_vm = dsim_reg_get_display_mode(id);
	err = dsim_reg_wait_idle_status(id, is_vm);
	if (err < 0)
		dsim_err("DSIM status is not IDLE!\n");

	/* 1. clock selection : OSC */
	dsim_reg_set_link_clock(id, 0);

	/* 2. master resetn */
	dpu_sysreg_dphy_reset(dsim->res.ss_regs, id, 0);
	/* 3. disable lane */
	dsim_reg_set_lanes_dphy(id, lanes, 0);
	/* 4. turn off WORDCLK and ESCCLK */
	dsim_reg_set_esc_clk_on_lane(id, 0, lanes);
	dsim_reg_set_esc_clk_en(id, 0);
	/* 5. disable PLL */
	dsim_reg_set_clocks(id, NULL, NULL, 0);

	if (err == 0)
		dsim_reg_sw_reset(id);

	return err;
}

/* Exit ULPS mode and set clocks and lanes */
int dsim_reg_exit_ulps_and_start(u32 id, u32 ddi_type, u32 lanes)
{
	int ret = 0;

	#if 0
	/*
	 * Guarantee 1.2v signal level for data lane(positive) when exit ULPS.
	 * DSIM Should be set standby. If not, lane goes to 600mv sometimes.
	 */
	dsim_reg_set_hs_clock(id, 1);
	dsim_reg_set_hs_clock(id, 0);
	#endif

	/* try to exit ULPS mode. The sequence is depends on DDI type */
	ret = dsim_reg_set_ulps_by_ddi(id, ddi_type, lanes, 0);
	dsim_reg_start(id);
	return ret;
}

/* Unset clocks and lanes and enter ULPS mode */
int dsim_reg_stop_and_enter_ulps(u32 id, u32 ddi_type, u32 lanes)
{
	int ret = 0;
	u32 is_vm;
	struct dsim_device *dsim = get_dsim_drvdata(id);
	struct dsim_regs regs;

	dsim_reg_clear_int(id, 0xffffffff);
	/* disable interrupts */
	dsim_reg_set_int(id, 0);

	/* 1. turn off clk lane & wait for stopstate_clk */
	ret = dsim_reg_set_hs_clock(id, 0);
	if (ret < 0)
		dsim_err("The CLK lane doesn't be switched to LP mode\n");

	/* 2. enter to ULPS & wait for ulps state of clk and data */
	dsim_reg_set_ulps_by_ddi(id, ddi_type, lanes, 1);

	/* 3. sequence for BLK_DPU off */
	/* 3.1 wait idle */
	is_vm = dsim_reg_get_display_mode(id);
	ret = dsim_reg_wait_idle_status(id, is_vm);
	if (ret < 0)
		dsim_err("%s : DSIM_status is not IDLE!\n", __func__);
	/* 3.2 OSC clock */
	dsim_reg_set_link_clock(id, 0);
	/* 3.3 off DPHY */
	dsim_reg_set_lanes_dphy(id, lanes, 0);
	dsim_reg_set_clocks(id, NULL, NULL, 0);

	if (ret < 0) {
		dsim_to_regs_param(dsim, &regs);
		__dsim_dump(id, &regs);
	}

	/* 3.4 sw reset */
	dsim_reg_sw_reset(id);

	return ret;
}

int dsim_reg_get_int_and_clear(u32 id)
{
	u32 val;

	val = dsim_read(id, DSIM_INTSRC);
	dsim_reg_clear_int(id, val);

	return val;
}

void dsim_reg_clear_int(u32 id, u32 int_src)
{
	dsim_write(id, DSIM_INTSRC, int_src);
}

void dsim_reg_wr_tx_header(u32 id, u32 d_id, unsigned long d0, u32 d1, u32 bta)
{
	u32 val = DSIM_PKTHDR_BTA_TYPE(bta) | DSIM_PKTHDR_ID(d_id) |
		DSIM_PKTHDR_DATA0(d0) | DSIM_PKTHDR_DATA1(d1);

	dsim_write_mask(id, DSIM_PKTHDR, val, DSIM_PKTHDR_DATA);
}

void dsim_reg_wr_tx_payload(u32 id, u32 payload)
{
	dsim_write(id, DSIM_PAYLOAD, payload);
}

u32 dsim_reg_header_fifo_is_empty(u32 id)
{
	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_PH_SFR);
}

u32 dsim_reg_is_writable_fifo_state(u32 id)
{
	u32 val = dsim_read(id, DSIM_FIFOCTRL);

	if (DSIM_FIFOCTRL_NUMBER_OF_PH_SFR_GET(val) < DSIM_FIFOCTRL_THRESHOLD)
		return 1;
	else
		return 0;
}

u32 dsim_reg_get_rx_fifo(u32 id)
{
	return dsim_read(id, DSIM_RXFIFO);
}

u32 dsim_reg_rx_fifo_is_empty(u32 id)
{
	return dsim_read_mask(id, DSIM_FIFOCTRL, DSIM_FIFOCTRL_EMPTY_RX);
}

// Detect AP corner case for mipi clk
#ifdef CONFIG_SEC_FACTORY
int dsim_reg_rx_err_cnt = 0;
#endif
int dsim_reg_rx_err_handler(u32 id, u32 rx_fifo)
{
	int ret = 0;
	u32 err_bit = rx_fifo >> 8; /* Error_Range [23:8] */

	if ((err_bit & MIPI_DSI_ERR_BIT_MASK) == 0) {
		dsim_dbg("dsim%d, Non error reporting format (rx_fifo=0x%x)\n",
				id, rx_fifo);
		return ret;
	}

	/* Parse error report bit*/
	if (err_bit & MIPI_DSI_ERR_SOT)
		dsim_err("SoT error!\n");
	if (err_bit & MIPI_DSI_ERR_SOT_SYNC)
		dsim_err("SoT sync error!\n");
	if (err_bit & MIPI_DSI_ERR_EOT_SYNC)
		dsim_err("EoT error!\n");
	if (err_bit & MIPI_DSI_ERR_ESCAPE_MODE_ENTRY_CMD)
		dsim_err("Escape mode entry command error!\n");
	if (err_bit & MIPI_DSI_ERR_LOW_POWER_TRANSMIT_SYNC)
		dsim_err("Low-power transmit sync error!\n");
	if (err_bit & MIPI_DSI_ERR_HS_RECEIVE_TIMEOUT)
		dsim_err("HS receive timeout error!\n");
	if (err_bit & MIPI_DSI_ERR_FALSE_CONTROL)
		dsim_err("False control error!\n");
	if (err_bit & MIPI_DSI_ERR_ECC_SINGLE_BIT)
		dsim_err("ECC error, single-bit(detected and corrected)!\n");
	if (err_bit & MIPI_DSI_ERR_ECC_MULTI_BIT)
		dsim_err("ECC error, multi-bit(detected, not corrected)!\n");
	if (err_bit & MIPI_DSI_ERR_CHECKSUM)
		dsim_err("Checksum error(long packet only)!\n");
	if (err_bit & MIPI_DSI_ERR_DATA_TYPE_NOT_RECOGNIZED)
		dsim_err("DSI data type not recognized!\n");
	if (err_bit & MIPI_DSI_ERR_VCHANNEL_ID_INVALID)
		dsim_err("DSI VC ID invalid!\n");
	if (err_bit & MIPI_DSI_ERR_INVALID_TRANSMIT_LENGTH)
		dsim_err("Invalid transmission length!\n");

	dsim_err("dsim%d, (rx_fifo=0x%x) Check DPHY values about HS clk.\n",
			id, rx_fifo);
	
#ifdef CONFIG_SEC_FACTORY
	dsim_reg_rx_err_cnt += 1;
	if (dsim_reg_rx_err_cnt >= 5) {
		panic("mipi dsi err detected. cause:0x%04x\n", err_bit);
	}
#endif
	return -EINVAL;
}

void dsim_reg_enable_shadow_read(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dsim_write_mask(id, DSIM_SFR_CTRL, val,
				DSIM_SFR_CTRL_SHADOW_REG_READ_EN);
}

void dsim_reg_function_reset(u32 id)
{
	u32 cnt = 1000;
	u32 state;

	dsim_write_mask(id, DSIM_SWRST, ~0,  DSIM_SWRST_FUNCRST);

	do {
		state = dsim_read(id, DSIM_SWRST) & DSIM_SWRST_FUNCRST;
		cnt--;
		udelay(10);
	} while (state && cnt);

	if (!cnt)
		dsim_err("%s is timeout.\n", __func__);

}

/* Set porch and resolution to support Partial update */
void dsim_reg_set_partial_update(u32 id, struct decon_lcd *lcd_info)
{
	dsim_reg_set_vresol(id, lcd_info->yres);
	dsim_reg_set_hresol(id, lcd_info->xres, lcd_info);
	dsim_reg_set_porch(id, lcd_info);
	dsim_reg_set_num_of_transfer(id, lcd_info->yres);
}

void dsim_reg_set_mres(u32 id, struct decon_lcd *lcd_info)
{
	u32 threshold;
	u32 num_of_slice;
	u32 num_of_transfer;
	int idx;

	if (lcd_info->mode != DECON_MIPI_COMMAND_MODE) {
		dsim_info("%s: mode[%d] doesn't support multi resolution\n",
				__func__, lcd_info->mode);
		return;
	}

	idx = lcd_info->mres_mode - DSU_MODE_1;
	dsim_reg_set_cm_underrun_lp_ref(id, lcd_info->cmd_underrun_lp_ref[idx]);

	if (lcd_info->dsc_enabled) {
		threshold = lcd_info->dsc_enc_sw * lcd_info->dsc_slice_num;
		/* use 1-line transfer only */
		num_of_transfer = lcd_info->yres;
	} else {
		threshold = lcd_info->xres;
		num_of_transfer = lcd_info->xres * lcd_info->yres / threshold;
	}

	dsim_reg_set_threshold(id, threshold);
	dsim_reg_set_vresol(id, lcd_info->yres);
	dsim_reg_set_hresol(id, lcd_info->xres, lcd_info);
	dsim_reg_set_porch(id, lcd_info);
	dsim_reg_set_num_of_transfer(id, num_of_transfer);

	dsim_reg_enable_dsc(id, lcd_info->dsc_enabled);
	if (lcd_info->dsc_enabled) {
		dsim_dbg("%s: dsc configuration is set\n", __func__);
		dsim_reg_set_num_of_slice(id, lcd_info->dsc_slice_num);
		dsim_reg_set_multi_slice(id, lcd_info); /* multi slice */
		dsim_reg_set_size_of_slice(id, lcd_info);

		dsim_reg_get_num_of_slice(id, &num_of_slice);
		dsim_dbg("dsim%d: number of DSC slice(%d)\n", id, num_of_slice);
		dsim_reg_print_size_of_slice(id);
	}
}

void dsim_reg_set_bist(u32 id, u32 en)
{
	if (en) {
		dsim_reg_set_bist_te_interval(id, 4505);
		dsim_reg_set_bist_mode(id, DSIM_GRAY_GRADATION);
		dsim_reg_enable_bist_pattern_move(id, true);
		dsim_reg_enable_bist(id, en);
	}
}

void dsim_reg_set_cmd_transfer_mode(u32 id, u32 lp)
{
	u32 val = lp ? ~0 : 0;

	dsim_write_mask(id, DSIM_ESCMODE, val, DSIM_ESCMODE_CMD_LPDT);
}

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
/* Only for command mode */
void dsim_reg_set_dphy_freq_hopping(u32 id, u32 p, u32 m, u32 k, u32 en)
{
	u32 val, mask;
	u32 pll_stable_cnt = (PLL_SLEEP_CNT_MULT + PLL_SLEEP_CNT_MARGIN) * p;

	if (en) {
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
		dsim_reg_set_pll_sleep_enable(id, false);
#endif
		dsim_reg_set_dphy_use_shadow(id, 1);
		dsim_reg_set_dphy_pll_stable_cnt(id, pll_stable_cnt);

		/* M value */
		val = DSIM_PHY_PMS_M(m);
		mask = DSIM_PHY_PMS_M_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_PLL_CON2, val, mask);

		/* K value */
		val = DSIM_PHY_PMS_K(k);
		mask = DSIM_PHY_PMS_K_MASK;
		dsim_phy_write_mask(id, DSIM_PHY_PLL_CON1, val, mask);

		dsim_reg_set_dphy_shadow_update_req(id, 1);
	} else {
		dsim_reg_set_dphy_use_shadow(id, 0);
		dsim_reg_set_dphy_shadow_update_req(id, 0);
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
		dsim_reg_set_pll_sleep_enable(id, true);
#endif
	}
}
#endif

void __dsim_dump(u32 id, struct dsim_regs *regs)
{
	/* change to updated register read mode (meaning: SHADOW in DECON) */
	dsim_info("=== DSIM %d LINK SFR DUMP ===\n", id);
	dsim_reg_enable_shadow_read(id, 0);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->regs, 0xFC, false);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->regs + 0x200, 0x4, false);

	dsim_info("=== DSIM %d DPHY SFR DUMP ===\n", id);
	/* DPHY dump */
	/* PLL */
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x0100, 0x24, false);
	/* MC */
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x0300, 0x48, false);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x03E0, 0x10, false);
	/* MD0 */
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x0400, 0x48, false);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x04C0, 0x20, false);
	/* MD1 */
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x0500, 0x48, false);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x05C0, 0x20, false);
	/* MD2 */
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x0600, 0x48, false);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x06C0, 0x20, false);
	/* MD3 */
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x0700, 0x48, false);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs->phy_regs + 0x07C0, 0x20, false);

	/* restore to avoid size mismatch (possible config error at DECON) */
	dsim_reg_enable_shadow_read(id, 1);
}
