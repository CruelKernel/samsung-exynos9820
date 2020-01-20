/*
 * linux/drivers/video/fbdev/exynos/dpu20/adap_freq_tbl.h
 *
 * Copyright (c) 2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ADAP_FREQ_TBL__
#define __ADAP_FREQ_TBL__

#include "adap_freq.h"

struct band_info band_gsm850_info[] = {
	DEFINE_BAND_INFO(128, 251, 0),
};

struct band_info band_egsm900_info[] = {
	DEFINE_BAND_INFO(0, 95,	2),
	DEFINE_BAND_INFO(96, 124, 3),
	DEFINE_BAND_INFO(975, 1023, 0),
};

struct band_info band_dcs1800_info[] = {
	DEFINE_BAND_INFO(512, 595, 2),
	DEFINE_BAND_INFO(596, 722, 0),
	DEFINE_BAND_INFO(723, 836, 2),
	DEFINE_BAND_INFO(837, 885, 0),
};

struct band_info band_pcs1900_info[] = {
	DEFINE_BAND_INFO(512, 637, 1),
	DEFINE_BAND_INFO(638, 693, 2),
	DEFINE_BAND_INFO(694, 810, 0),
};

struct band_info band_wb01_info[] = {
	DEFINE_BAND_INFO(10562, 10665, 0),
	DEFINE_BAND_INFO(10666, 10796, 2),
	DEFINE_BAND_INFO(10797, 10838, 0),
};

struct band_info band_wb02_info[] = {
	DEFINE_BAND_INFO(9662, 9776, 1),
	DEFINE_BAND_INFO(9777, 9832, 2),
	DEFINE_BAND_INFO(9833, 9938, 0),
};

struct band_info band_wb03_info[] = {
	DEFINE_BAND_INFO(1162, 1234, 2),
	DEFINE_BAND_INFO(1235, 1361, 0),
	DEFINE_BAND_INFO(1362, 1475, 2),
	DEFINE_BAND_INFO(1476, 1513, 0),
};

struct band_info band_wb04_info[] = {
	DEFINE_BAND_INFO(1537, 1640, 0),
	DEFINE_BAND_INFO(1641, 1738, 2),
};

struct band_info band_wb05_info[] = {
	DEFINE_BAND_INFO(4357, 4458, 0),
};

struct band_info band_wb07_info[] = {
	DEFINE_BAND_INFO(2237, 2331, 2),
	DEFINE_BAND_INFO(2332, 2409, 0),
	DEFINE_BAND_INFO(2410, 2496, 1),
	DEFINE_BAND_INFO(2497, 2563, 2),
};

struct band_info band_wb08_info[] = {
	DEFINE_BAND_INFO(2937, 3011, 0),
	DEFINE_BAND_INFO(3012, 3088, 3),
};

struct band_info band_bc0_info[] = {
	DEFINE_BAND_INFO(1, 1023, 0),
	DEFINE_BAND_INFO(1024, 1424, 3),
};

struct band_info band_bc1_info[] = {
	DEFINE_BAND_INFO(0, 508, 1),
	DEFINE_BAND_INFO(510, 730, 2),
	DEFINE_BAND_INFO(732, 1199, 0),
};

struct band_info band_bc10_info[] = {
	DEFINE_BAND_INFO(0, 719, 3),
	DEFINE_BAND_INFO(720, 919, 0),
};

struct band_info band_lb01_info[] = {
	DEFINE_BAND_INFO(0, 231, 0),
	DEFINE_BAND_INFO(232, 493, 2),
	DEFINE_BAND_INFO(494, 599, 0),
};

struct band_info band_lb02_info[] = {
	DEFINE_BAND_INFO(600, 854, 1),
	DEFINE_BAND_INFO(855, 965, 2),
	DEFINE_BAND_INFO(966, 1199, 0),
};

struct band_info band_lb03_info[] = {
	DEFINE_BAND_INFO(1200, 1369, 2),
	DEFINE_BAND_INFO(1370, 1623, 0),
	DEFINE_BAND_INFO(1624, 1851, 2),
	DEFINE_BAND_INFO(1852, 1949, 0),
};

struct band_info band_lb04_info[] = {
	DEFINE_BAND_INFO(1950, 2181, 0),
	DEFINE_BAND_INFO(2182, 2399, 2),
};


struct band_info band_lb05_info[] = {
	DEFINE_BAND_INFO(2400, 2649, 0),
};

struct band_info band_lb07_info[] = {
	DEFINE_BAND_INFO(2750, 2964, 2),
	DEFINE_BAND_INFO(2965, 3119, 0),
	DEFINE_BAND_INFO(3120, 3294, 1),
	DEFINE_BAND_INFO(3295, 3449, 2),
};

struct band_info band_lb08_info[] = {
	DEFINE_BAND_INFO(3450, 3624, 0),
	DEFINE_BAND_INFO(3625, 3799, 3),
};

struct band_info band_lb12_info[] = {
	DEFINE_BAND_INFO(5010, 5179, 0),
};

struct band_info band_lb13_info[] = {
	DEFINE_BAND_INFO(5180, 5279, 3),
};

struct band_info band_lb14_info[] = {
	DEFINE_BAND_INFO(5280, 5379, 3),
};

struct band_info band_lb17_info[] = {
	DEFINE_BAND_INFO(5730, 5849, 0),
};

struct band_info band_lb18_info[] = {
	DEFINE_BAND_INFO(5850, 5947, 3),
	DEFINE_BAND_INFO(5948, 5999, 0),
};

struct band_info band_lb19_info[] = {
	DEFINE_BAND_INFO(6000, 6149, 0),
};

struct band_info band_lb20_info[] = {
	DEFINE_BAND_INFO(6150, 6236, 0),
	DEFINE_BAND_INFO(6237, 6449, 3),
};

struct band_info band_lb21_info[] = {
	DEFINE_BAND_INFO(6450, 6599, 0),
};

struct band_info band_lb25_info[] = {
	DEFINE_BAND_INFO(8040, 8294, 1),
	DEFINE_BAND_INFO(8295, 8405, 2),
	DEFINE_BAND_INFO(8406, 8642, 0),
	DEFINE_BAND_INFO(8643, 8689, 1),
};

struct band_info band_lb26_info[] = {
	DEFINE_BAND_INFO(8690, 8798, 3),
	DEFINE_BAND_INFO(8799, 9039, 0),
};

struct band_info band_lb28_info[] = {
	DEFINE_BAND_INFO(9210, 9361, 3),
	DEFINE_BAND_INFO(9362, 9600, 0),
	DEFINE_BAND_INFO(9601, 9659, 2),
};

struct band_info band_lb29_info[] = {
	DEFINE_BAND_INFO(9660, 9738, 3),
	DEFINE_BAND_INFO(9739, 9769, 0),
};

struct band_info band_lb30_info[] = {
	DEFINE_BAND_INFO(9770, 9869, 0),
};

struct band_info band_lb32_info[] = {
	DEFINE_BAND_INFO(9920, 10160, 1),
	DEFINE_BAND_INFO(10161, 10244, 2),
	DEFINE_BAND_INFO(10245, 10359, 3),
};

struct band_info band_lb34_info[] = {
	DEFINE_BAND_INFO(36200, 36349, 0),
};

struct band_info band_lb38_info[] = {
	DEFINE_BAND_INFO(37750, 37982, 2),
	DEFINE_BAND_INFO(37983, 38143, 0),
	DEFINE_BAND_INFO(38144, 38249, 1),
};

struct band_info band_lb39_info[] = {
	DEFINE_BAND_INFO(38250, 38524, 1),
	DEFINE_BAND_INFO(38525, 38649, 2),
};

struct band_info band_lb40_info[] = {
	DEFINE_BAND_INFO(38650, 38889, 0),
	DEFINE_BAND_INFO(38890, 39172, 2),
	DEFINE_BAND_INFO(39173, 39367, 0),
	DEFINE_BAND_INFO(39368, 39649, 2),
};

struct band_info band_lb41_info[] = {
	DEFINE_BAND_INFO(39650, 39831, 0),
	DEFINE_BAND_INFO(39832, 40080, 3),
	DEFINE_BAND_INFO(40081, 40307, 0),
	DEFINE_BAND_INFO(40308, 40571, 3),
	DEFINE_BAND_INFO(40572, 40783, 0),
	DEFINE_BAND_INFO(40784, 41061, 3),
	DEFINE_BAND_INFO(41062, 41259, 0),
	DEFINE_BAND_INFO(41260, 41434, 1),
	DEFINE_BAND_INFO(41435, 41589, 2),
};

struct band_info band_lb42_info[] = {
	DEFINE_BAND_INFO(41590, 41804, 3),
	DEFINE_BAND_INFO(41805, 42005, 1),
	DEFINE_BAND_INFO(42006, 42294, 3),
	DEFINE_BAND_INFO(42295, 42484, 1),
	DEFINE_BAND_INFO(42485, 42732, 0),
	DEFINE_BAND_INFO(42733, 42963, 1),
	DEFINE_BAND_INFO(42964, 43208, 0),
	DEFINE_BAND_INFO(43209, 43443, 1),
	DEFINE_BAND_INFO(43444, 43589, 0),
};

struct band_info band_lb48_info[] = {
	DEFINE_BAND_INFO(55240, 55425, 3),
	DEFINE_BAND_INFO(55426, 55593, 1),
	DEFINE_BAND_INFO(55594, 55835, 0),
	DEFINE_BAND_INFO(55836, 56072, 1),
	DEFINE_BAND_INFO(56073, 56311, 0),
	DEFINE_BAND_INFO(56312, 56551, 1),
	DEFINE_BAND_INFO(56552, 56739, 0),
};

struct band_info band_lb66_info[] = {
	DEFINE_BAND_INFO(66436, 66667, 0),
	DEFINE_BAND_INFO(66668, 66929, 2),
	DEFINE_BAND_INFO(66930, 67143, 0),
	DEFINE_BAND_INFO(67144, 67335, 2),
};

struct band_info band_lb71_info[] = {
	DEFINE_BAND_INFO(68586, 68698, 3),
	DEFINE_BAND_INFO(68699, 68935, 0),
};

struct ril_band_info total_band_info[RIL_BAND_MAX] = {
	[RIL_BAND_GSM850] = DEFINE_RIL_INFO(band_gsm850_info),
	[RIL_BAND_EGSM900] = DEFINE_RIL_INFO(band_egsm900_info),
	[RIL_BAND_DCS1800] = DEFINE_RIL_INFO(band_dcs1800_info),
	[RIL_BAND_PCS1900] = DEFINE_RIL_INFO(band_pcs1900_info),
	[RIL_BAND_WB01] = DEFINE_RIL_INFO(band_wb01_info),
	[RIL_BAND_WB02] = DEFINE_RIL_INFO(band_wb02_info),
	[RIL_BAND_WB03] = DEFINE_RIL_INFO(band_wb03_info),
	[RIL_BAND_WB04] = DEFINE_RIL_INFO(band_wb04_info),
	[RIL_BAND_WB05] = DEFINE_RIL_INFO(band_wb05_info),
	[RIL_BAND_WB07] = DEFINE_RIL_INFO(band_wb07_info),
	[RIL_BAND_WB08] = DEFINE_RIL_INFO(band_wb08_info),
	[RIL_BAND_BC0] = DEFINE_RIL_INFO(band_bc0_info),
	[RIL_BAND_BC1] = DEFINE_RIL_INFO(band_bc1_info),
	[RIL_BAND_BC10] = DEFINE_RIL_INFO(band_bc10_info),
	[RIL_BAND_LB01] = DEFINE_RIL_INFO(band_lb01_info),
	[RIL_BAND_LB02] = DEFINE_RIL_INFO(band_lb02_info),
	[RIL_BAND_LB03] = DEFINE_RIL_INFO(band_lb03_info),
	[RIL_BAND_LB04] = DEFINE_RIL_INFO(band_lb04_info),
	[RIL_BAND_LB05] = DEFINE_RIL_INFO(band_lb05_info),
	[RIL_BAND_LB07] = DEFINE_RIL_INFO(band_lb07_info),
	[RIL_BAND_LB08] = DEFINE_RIL_INFO(band_lb08_info),
	[RIL_BAND_LB12] = DEFINE_RIL_INFO(band_lb12_info),
	[RIL_BAND_LB13] = DEFINE_RIL_INFO(band_lb13_info),
	[RIL_BAND_LB14] = DEFINE_RIL_INFO(band_lb14_info),
	[RIL_BAND_LB17] = DEFINE_RIL_INFO(band_lb17_info),
	[RIL_BAND_LB18] = DEFINE_RIL_INFO(band_lb18_info),
	[RIL_BAND_LB19] = DEFINE_RIL_INFO(band_lb19_info),
	[RIL_BAND_LB20] = DEFINE_RIL_INFO(band_lb20_info),
	[RIL_BAND_LB21] = DEFINE_RIL_INFO(band_lb21_info),
	[RIL_BAND_LB25] = DEFINE_RIL_INFO(band_lb25_info),
	[RIL_BAND_LB26] = DEFINE_RIL_INFO(band_lb26_info),
	[RIL_BAND_LB28] = DEFINE_RIL_INFO(band_lb28_info),
	[RIL_BAND_LB29] = DEFINE_RIL_INFO(band_lb29_info),
	[RIL_BAND_LB30] = DEFINE_RIL_INFO(band_lb30_info),
	[RIL_BAND_LB32] = DEFINE_RIL_INFO(band_lb32_info),
	[RIL_BAND_LB34] = DEFINE_RIL_INFO(band_lb34_info),
	[RIL_BAND_LB38] = DEFINE_RIL_INFO(band_lb38_info),
	[RIL_BAND_LB39] = DEFINE_RIL_INFO(band_lb39_info),
	[RIL_BAND_LB40] = DEFINE_RIL_INFO(band_lb40_info),
	[RIL_BAND_LB41] = DEFINE_RIL_INFO(band_lb41_info),
	[RIL_BAND_LB42] = DEFINE_RIL_INFO(band_lb42_info),
	[RIL_BAND_LB48] = DEFINE_RIL_INFO(band_lb48_info),
	[RIL_BAND_LB66] = DEFINE_RIL_INFO(band_lb66_info),
	[RIL_BAND_LB71] = DEFINE_RIL_INFO(band_lb71_info),
};
#endif //__ADAP_FREQ_TBL__
