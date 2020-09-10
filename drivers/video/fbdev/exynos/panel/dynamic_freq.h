/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DYNAMIC_FREQ__
#define __DYNAMIC_FREQ__

#define TBL_ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define DEFINE_FREQ_RANGE(_min, _max, _idx)	\
{							\
	.min = _min,			\
	.max = _max,			\
	.freq_idx = _idx,		\
}

#define DEFINE_FREQ_SET(_array)	\
{								\
	.array = _array,			\
	.size = TBL_ARR_SIZE(_array),	\
}

struct dynamic_freq_range {
	int	min;
	int max;
	int freq_idx;
};

struct df_freq_tbl_info {
	struct dynamic_freq_range *array;
	unsigned int size;
};

struct __packed ril_noti_info {
	u8 rat;
	u32 band;
	u32 channel;
};

enum {
	FREQ_RANGE_850	= 1,
	FREQ_RANGE_900 = 2,
	FREQ_RANGE_1800 = 3,
	FREQ_RANGE_1900 = 4,
	FREQ_RANGE_WB01 = 11,
	FREQ_RANGE_WB02 = 12,
	FREQ_RANGE_WB03 = 13,
	FREQ_RANGE_WB04 = 14,
	FREQ_RANGE_WB05 = 15,
	FREQ_RANGE_WB07 = 17,
	FREQ_RANGE_WB08 = 18,
	FREQ_RANGE_TD1 = 51,
	FREQ_RANGE_TD2 = 52,
	FREQ_RANGE_TD3 = 53,
	FREQ_RANGE_TD4 = 54,
	FREQ_RANGE_TD5 = 55,
	FREQ_RANGE_TD6 = 56,
	FREQ_RANGE_BC0  = 61,
	FREQ_RANGE_BC1  = 62,
	FREQ_RANGE_BC10 = 71,
	FREQ_RANGE_LB01 = 91,
	FREQ_RANGE_LB02 = 92,
	FREQ_RANGE_LB03 = 93,
	FREQ_RANGE_LB04 = 94,
	FREQ_RANGE_LB05 = 95,
	FREQ_RANGE_LB07 = 97,
	FREQ_RANGE_LB08 = 98,
	FREQ_RANGE_LB12 = 102,
	FREQ_RANGE_LB13 = 103,
	FREQ_RANGE_LB14 = 104,
	FREQ_RANGE_LB17 = 107,
	FREQ_RANGE_LB18 = 108,
	FREQ_RANGE_LB19 = 109,
	FREQ_RANGE_LB20 = 110,
	FREQ_RANGE_LB21 = 111,
	FREQ_RANGE_LB25 = 115,
	FREQ_RANGE_LB26 = 116,
	FREQ_RANGE_LB28 = 118,
	FREQ_RANGE_LB29 = 119,
	FREQ_RANGE_LB30 = 120,
	FREQ_RANGE_LB32 = 122,
	FREQ_RANGE_LB34 = 124,
	FREQ_RANGE_LB38 = 128,
	FREQ_RANGE_LB39 = 129,
	FREQ_RANGE_LB40 = 130,
	FREQ_RANGE_LB41 = 131,
	FREQ_RANGE_LB42 = 132,
	FREQ_RANGE_LB48 = 138,
	FREQ_RANGE_LB66 = 156,
	FREQ_RANGE_LB71 = 161,
	FREQ_RANGE_MAX = 162,
};

int dynamic_freq_probe(struct panel_device *panel, struct df_freq_tbl_info *freq_set);
int check_df_update(struct panel_device *panel);
int set_dynamic_freq_ffc(struct panel_device *panel);
int finish_dynamic_freq(struct panel_device *panel);
int dynamic_freq_update(struct panel_device *panel, int idx);

#endif //__DYNAMIC_FREQ__
