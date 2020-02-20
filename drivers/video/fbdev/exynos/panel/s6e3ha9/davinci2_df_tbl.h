/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/d2_dynamic_freq.h
 *
 * Copyright (c) 2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __D2_DYNAMIC_FREQ__
#define __D2_DYNAMIC_FREQ__

#include "../dynamic_freq.h"

struct dynamic_freq_range d2_freq_range_850[] = {
	DEFINE_FREQ_RANGE(0, 0, 0),
};

struct dynamic_freq_range d2_freq_range_900[] = {
	DEFINE_FREQ_RANGE(0, 0, 2),
};

struct dynamic_freq_range d2_freq_range_1800[] = {
	DEFINE_FREQ_RANGE(0, 0, 0),
};

struct dynamic_freq_range d2_freq_range_1900[] = {
	DEFINE_FREQ_RANGE(0, 0, 0),
};

struct dynamic_freq_range d2_freq_range_wb01[] = {
	DEFINE_FREQ_RANGE(10562, 10612, 3),
	DEFINE_FREQ_RANGE(10613, 10691, 1),
	DEFINE_FREQ_RANGE(10692, 10806, 3),
	DEFINE_FREQ_RANGE(10807, 10838, 1),
};

struct dynamic_freq_range d2_freq_range_wb02[] = {
	DEFINE_FREQ_RANGE(9662, 9749, 1),
	DEFINE_FREQ_RANGE(9750, 9837, 3),
	DEFINE_FREQ_RANGE(9838, 9938, 1),
};

struct dynamic_freq_range d2_freq_range_wb03[] = {
	DEFINE_FREQ_RANGE(1162, 1243, 0),
	DEFINE_FREQ_RANGE(1244, 1310, 1),
	DEFINE_FREQ_RANGE(1311, 1381, 3),
	DEFINE_FREQ_RANGE(1382, 1430, 0),
	DEFINE_FREQ_RANGE(1431, 1498, 1),
	DEFINE_FREQ_RANGE(1499, 1513, 2),
};

struct dynamic_freq_range d2_freq_range_wb04[] = {
	DEFINE_FREQ_RANGE(1537, 1588, 0),
	DEFINE_FREQ_RANGE(1589, 1666, 1),
	DEFINE_FREQ_RANGE(1667, 1738, 0),
};

struct dynamic_freq_range d2_freq_range_wb05[] = {
	DEFINE_FREQ_RANGE(4357, 4415, 3),
	DEFINE_FREQ_RANGE(4416, 4458, 2),
};

struct dynamic_freq_range d2_freq_range_wb07[] = {
	DEFINE_FREQ_RANGE(2237, 2263, 1),
	DEFINE_FREQ_RANGE(2264, 2355, 0),
	DEFINE_FREQ_RANGE(2356, 2451, 1),
	DEFINE_FREQ_RANGE(2452, 2547, 2),
	DEFINE_FREQ_RANGE(2548, 2563, 1),
};

struct dynamic_freq_range d2_freq_range_wb08[] = {
	DEFINE_FREQ_RANGE(2937, 3001, 2),
	DEFINE_FREQ_RANGE(3002, 3088, 3),
};


struct dynamic_freq_range d2_freq_range_td1[] = {
	DEFINE_FREQ_RANGE(0, 0, 2),
};

struct dynamic_freq_range d2_freq_range_td2[] = {
	DEFINE_FREQ_RANGE(0, 0, 1),
};

struct dynamic_freq_range d2_freq_range_td3[] = {
	DEFINE_FREQ_RANGE(0, 0, 3),
};

struct dynamic_freq_range d2_freq_range_td4[] = {
	DEFINE_FREQ_RANGE(0, 0, 3),
};

struct dynamic_freq_range d2_freq_range_td5[] = {
	DEFINE_FREQ_RANGE(0, 0, 0),
};

struct dynamic_freq_range d2_freq_range_td6[] = {
	DEFINE_FREQ_RANGE(0, 0, 2),
};

struct dynamic_freq_range d2_freq_range_bc0[] = {
	DEFINE_FREQ_RANGE(0, 0, 3),
};

struct dynamic_freq_range d2_freq_range_bc1[] = {
	DEFINE_FREQ_RANGE(0, 0, 1),
};

struct dynamic_freq_range d2_freq_range_bc10[] = {
	DEFINE_FREQ_RANGE(0, 540, 2),
};


struct dynamic_freq_range d2_freq_range_lb01[] = {
	DEFINE_FREQ_RANGE(0, 126, 3),
	DEFINE_FREQ_RANGE(127, 283, 1),
	DEFINE_FREQ_RANGE(284, 513, 3),
	DEFINE_FREQ_RANGE(514, 599, 1),
};

struct dynamic_freq_range d2_freq_range_lb02[] = {
	DEFINE_FREQ_RANGE(600, 800, 1),
	DEFINE_FREQ_RANGE(801, 976, 3),
	DEFINE_FREQ_RANGE(977, 1177, 1),
	DEFINE_FREQ_RANGE(1178, 1199, 2),
};

struct dynamic_freq_range d2_freq_range_lb03[] = {
	DEFINE_FREQ_RANGE(1200, 1388, 0),
	DEFINE_FREQ_RANGE(1389, 1521, 1),
	DEFINE_FREQ_RANGE(1522, 1664, 3),
	DEFINE_FREQ_RANGE(1665, 1762, 0),
	DEFINE_FREQ_RANGE(1763, 1897, 1),
	DEFINE_FREQ_RANGE(1898, 1949, 2),
};

struct dynamic_freq_range d2_freq_range_lb04[] = {
	DEFINE_FREQ_RANGE(1950, 2076, 3),
	DEFINE_FREQ_RANGE(2077, 2233, 1),
	DEFINE_FREQ_RANGE(2234, 2399, 0),
};


struct dynamic_freq_range d2_freq_range_lb05[] = {
	DEFINE_FREQ_RANGE(2400, 2542, 3),
	DEFINE_FREQ_RANGE(2543, 2649, 1),
};

struct dynamic_freq_range d2_freq_range_lb07[] = {
	DEFINE_FREQ_RANGE(2750, 2827, 1),
	DEFINE_FREQ_RANGE(2828, 3016, 2),
	DEFINE_FREQ_RANGE(3017, 3203, 1),
	DEFINE_FREQ_RANGE(3204, 3395, 2),
	DEFINE_FREQ_RANGE(3396, 3449, 1),
};

struct dynamic_freq_range d2_freq_range_lb08[] = {
	DEFINE_FREQ_RANGE(3450, 3604, 2),
	DEFINE_FREQ_RANGE(3605, 3799, 3),
};

struct dynamic_freq_range d2_freq_range_lb12[] = {
	DEFINE_FREQ_RANGE(5010, 5179, 2),
	DEFINE_FREQ_RANGE(5180, 5279, 3),
};

struct dynamic_freq_range d2_freq_range_lb13[] = {
	DEFINE_FREQ_RANGE(5180, 5279, 3),	
};

struct dynamic_freq_range d2_freq_range_lb14[] = {
	DEFINE_FREQ_RANGE(5280, 5379, 0),	
};

struct dynamic_freq_range d2_freq_range_lb17[] = {
	DEFINE_FREQ_RANGE(5730, 5849, 2),	
};

struct dynamic_freq_range d2_freq_range_lb18[] = {
	DEFINE_FREQ_RANGE(5850, 5999, 3),	
};

struct dynamic_freq_range d2_freq_range_lb19[] = {
	DEFINE_FREQ_RANGE(6000, 6149, 1),	
};

struct dynamic_freq_range d2_freq_range_lb20[] = {
	DEFINE_FREQ_RANGE(6150, 6298, 3),
	DEFINE_FREQ_RANGE(6299, 6449, 2),
};

struct dynamic_freq_range d2_freq_range_lb21[] = {
	DEFINE_FREQ_RANGE(6450, 6582, 2),
	DEFINE_FREQ_RANGE(6583, 6599, 0),
};

struct dynamic_freq_range d2_freq_range_lb25[] = {
	DEFINE_FREQ_RANGE(8040, 8240, 1),
	DEFINE_FREQ_RANGE(8241, 8416, 3),
	DEFINE_FREQ_RANGE(8417, 8617, 1),
	DEFINE_FREQ_RANGE(8618, 8689, 2),
};

struct dynamic_freq_range d2_freq_range_lb26[] = {
	DEFINE_FREQ_RANGE(8690, 8932, 3),
	DEFINE_FREQ_RANGE(8933, 9039, 1),
};

struct dynamic_freq_range d2_freq_range_lb28[] = {
	DEFINE_FREQ_RANGE(9210, 9403, 0),
	DEFINE_FREQ_RANGE(9404, 9517, 2),
	DEFINE_FREQ_RANGE(9518, 9659, 3),
};

struct dynamic_freq_range d2_freq_range_lb29[] = {
	DEFINE_FREQ_RANGE(9660, 9769, 3),
};

struct dynamic_freq_range d2_freq_range_lb30[] = {
	DEFINE_FREQ_RANGE(9770, 9869, 1),
};

struct dynamic_freq_range d2_freq_range_lb32[] = {
	DEFINE_FREQ_RANGE(9920, 10112, 2),
	DEFINE_FREQ_RANGE(10113, 10274, 0),
	DEFINE_FREQ_RANGE(10275, 10359, 1),
};

struct dynamic_freq_range d2_freq_range_lb34[] = {
	DEFINE_FREQ_RANGE(36200, 36349, 1),
};

struct dynamic_freq_range d2_freq_range_lb38[] = {
	DEFINE_FREQ_RANGE(37750, 37950, 1),
	DEFINE_FREQ_RANGE(37951, 38137, 0),
	DEFINE_FREQ_RANGE(38138, 38249, 1),
};

struct dynamic_freq_range d2_freq_range_lb39[] = {
	DEFINE_FREQ_RANGE(38250, 38436, 0),
	DEFINE_FREQ_RANGE(38437, 38574, 1),
	DEFINE_FREQ_RANGE(38575, 38649, 2),
};

struct dynamic_freq_range d2_freq_range_lb40[] = {
	DEFINE_FREQ_RANGE(38650, 38812, 3),
	DEFINE_FREQ_RANGE(38813, 38915, 1),
	DEFINE_FREQ_RANGE(38916, 39121, 0),
	DEFINE_FREQ_RANGE(39122, 39291, 1),
	DEFINE_FREQ_RANGE(39292, 39494, 0),
	DEFINE_FREQ_RANGE(39495, 39649, 1),
};

struct dynamic_freq_range d2_freq_range_lb41[] = {
	DEFINE_FREQ_RANGE(39838, 40029, 0),
	DEFINE_FREQ_RANGE(40030, 40214, 1),
	DEFINE_FREQ_RANGE(40215, 40403, 0),
	DEFINE_FREQ_RANGE(40404, 40590, 1),
	DEFINE_FREQ_RANGE(40591, 40777, 0),
	DEFINE_FREQ_RANGE(40778, 40967, 1),
	DEFINE_FREQ_RANGE(40968, 41156, 2),
	DEFINE_FREQ_RANGE(41157, 41343, 1),
	DEFINE_FREQ_RANGE(41344, 41535, 2),
	DEFINE_FREQ_RANGE(41536, 41589, 1),
};

struct dynamic_freq_range d2_freq_range_lb42[] = {
	DEFINE_FREQ_RANGE(41590, 41772, 1),
	DEFINE_FREQ_RANGE(41773, 41983, 3),
	DEFINE_FREQ_RANGE(41984, 42149, 1),
	DEFINE_FREQ_RANGE(42150, 42371, 3),
	DEFINE_FREQ_RANGE(42372, 42525, 1),
	DEFINE_FREQ_RANGE(42526, 42758, 3),
	DEFINE_FREQ_RANGE(42759, 42902, 1),
	DEFINE_FREQ_RANGE(42903, 43021, 0),
	DEFINE_FREQ_RANGE(43022, 43156, 2),
	DEFINE_FREQ_RANGE(43157, 43278, 1),
	DEFINE_FREQ_RANGE(43279, 43395, 0),
	DEFINE_FREQ_RANGE(43396, 43535, 2),
	DEFINE_FREQ_RANGE(43536, 43589, 1),
};

struct dynamic_freq_range d2_freq_range_lb48[] = {
	DEFINE_FREQ_RANGE(41590, 41772, 1),
	DEFINE_FREQ_RANGE(41773, 41983, 3),
	DEFINE_FREQ_RANGE(41984, 42149, 1),
	DEFINE_FREQ_RANGE(42150, 42371, 3),
	DEFINE_FREQ_RANGE(42372, 42525, 1),
	DEFINE_FREQ_RANGE(42526, 42758, 3),
	DEFINE_FREQ_RANGE(42759, 42902, 1),
	DEFINE_FREQ_RANGE(42903, 43021, 0),
	DEFINE_FREQ_RANGE(43022, 43156, 2),
	DEFINE_FREQ_RANGE(43157, 43278, 1),
	DEFINE_FREQ_RANGE(43279, 43395, 0),
	DEFINE_FREQ_RANGE(43396, 43535, 2),
	DEFINE_FREQ_RANGE(43536, 43589, 1),
};

struct dynamic_freq_range d2_freq_range_lb66[] = {
	DEFINE_FREQ_RANGE(66436, 66562, 3),
	DEFINE_FREQ_RANGE(66563, 66719, 1),
	DEFINE_FREQ_RANGE(66720, 66949, 3),
	DEFINE_FREQ_RANGE(66950, 67095, 1),
	DEFINE_FREQ_RANGE(67096, 67312, 0),
	DEFINE_FREQ_RANGE(67313, 67335, 1),
};

struct dynamic_freq_range d2_freq_range_lb71[] = {
	DEFINE_FREQ_RANGE(68586, 68786, 2),
	DEFINE_FREQ_RANGE(68787, 68924, 3),
	DEFINE_FREQ_RANGE(68925, 68935, 0),
};



struct df_freq_tbl_info d2_dynamic_freq_set[FREQ_RANGE_MAX] = {
	[FREQ_RANGE_850] = DEFINE_FREQ_SET(d2_freq_range_850),
	[FREQ_RANGE_900] = DEFINE_FREQ_SET(d2_freq_range_900),
	[FREQ_RANGE_1800] = DEFINE_FREQ_SET(d2_freq_range_1800),
	[FREQ_RANGE_1900] = DEFINE_FREQ_SET(d2_freq_range_1900),
	[FREQ_RANGE_WB01] = DEFINE_FREQ_SET(d2_freq_range_wb01),
	[FREQ_RANGE_WB02] = DEFINE_FREQ_SET(d2_freq_range_wb02),
	[FREQ_RANGE_WB03] = DEFINE_FREQ_SET(d2_freq_range_wb03),
	[FREQ_RANGE_WB04] = DEFINE_FREQ_SET(d2_freq_range_wb04),
	[FREQ_RANGE_WB05] = DEFINE_FREQ_SET(d2_freq_range_wb05),
	[FREQ_RANGE_WB07] = DEFINE_FREQ_SET(d2_freq_range_wb07),
	[FREQ_RANGE_WB08] = DEFINE_FREQ_SET(d2_freq_range_wb08),
	[FREQ_RANGE_TD1] = DEFINE_FREQ_SET(d2_freq_range_td1),
	[FREQ_RANGE_TD2] = DEFINE_FREQ_SET(d2_freq_range_td2),
	[FREQ_RANGE_TD3] = DEFINE_FREQ_SET(d2_freq_range_td3),
	[FREQ_RANGE_TD4] = DEFINE_FREQ_SET(d2_freq_range_td4),
	[FREQ_RANGE_TD5] = DEFINE_FREQ_SET(d2_freq_range_td5),
	[FREQ_RANGE_TD6] = DEFINE_FREQ_SET(d2_freq_range_td6),
	[FREQ_RANGE_BC0] = DEFINE_FREQ_SET(d2_freq_range_bc0),
	[FREQ_RANGE_BC1] = DEFINE_FREQ_SET(d2_freq_range_bc1),
	[FREQ_RANGE_BC10] = DEFINE_FREQ_SET(d2_freq_range_bc10),
	[FREQ_RANGE_LB01] = DEFINE_FREQ_SET(d2_freq_range_lb01),
	[FREQ_RANGE_LB02] = DEFINE_FREQ_SET(d2_freq_range_lb02),
	[FREQ_RANGE_LB03] = DEFINE_FREQ_SET(d2_freq_range_lb03),
	[FREQ_RANGE_LB04] = DEFINE_FREQ_SET(d2_freq_range_lb04),
	[FREQ_RANGE_LB05] = DEFINE_FREQ_SET(d2_freq_range_lb05),
	[FREQ_RANGE_LB07] = DEFINE_FREQ_SET(d2_freq_range_lb07),
	[FREQ_RANGE_LB08] = DEFINE_FREQ_SET(d2_freq_range_lb08),
	[FREQ_RANGE_LB12] = DEFINE_FREQ_SET(d2_freq_range_lb12),
	[FREQ_RANGE_LB13] = DEFINE_FREQ_SET(d2_freq_range_lb13),
	[FREQ_RANGE_LB14] = DEFINE_FREQ_SET(d2_freq_range_lb14),
	[FREQ_RANGE_LB17] = DEFINE_FREQ_SET(d2_freq_range_lb17),
	[FREQ_RANGE_LB18] = DEFINE_FREQ_SET(d2_freq_range_lb18),
	[FREQ_RANGE_LB19] = DEFINE_FREQ_SET(d2_freq_range_lb19),
	[FREQ_RANGE_LB20] = DEFINE_FREQ_SET(d2_freq_range_lb20),
	[FREQ_RANGE_LB21] = DEFINE_FREQ_SET(d2_freq_range_lb21),
	[FREQ_RANGE_LB25] = DEFINE_FREQ_SET(d2_freq_range_lb25),
	[FREQ_RANGE_LB26] = DEFINE_FREQ_SET(d2_freq_range_lb26),
	[FREQ_RANGE_LB28] = DEFINE_FREQ_SET(d2_freq_range_lb28),
	[FREQ_RANGE_LB29] = DEFINE_FREQ_SET(d2_freq_range_lb29),
	[FREQ_RANGE_LB30] = DEFINE_FREQ_SET(d2_freq_range_lb30),
	[FREQ_RANGE_LB32] = DEFINE_FREQ_SET(d2_freq_range_lb32),
	[FREQ_RANGE_LB34] = DEFINE_FREQ_SET(d2_freq_range_lb34),
	[FREQ_RANGE_LB38] = DEFINE_FREQ_SET(d2_freq_range_lb38),
	[FREQ_RANGE_LB39] = DEFINE_FREQ_SET(d2_freq_range_lb39),
	[FREQ_RANGE_LB40] = DEFINE_FREQ_SET(d2_freq_range_lb40),
	[FREQ_RANGE_LB41] = DEFINE_FREQ_SET(d2_freq_range_lb41),
	[FREQ_RANGE_LB42] = DEFINE_FREQ_SET(d2_freq_range_lb42),
	[FREQ_RANGE_LB48] = DEFINE_FREQ_SET(d2_freq_range_lb48),
	[FREQ_RANGE_LB66] = DEFINE_FREQ_SET(d2_freq_range_lb66),
	[FREQ_RANGE_LB71] = DEFINE_FREQ_SET(d2_freq_range_lb71),
};

#endif //__D2_DYNAMIC_FREQ__
