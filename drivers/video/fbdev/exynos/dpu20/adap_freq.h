/*
 * linux/drivers/video/fbdev/exynos/dpu20/adap_freq.h
 *
 * Copyright (c) 2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ADAP_FREQ__
#define __ADAP_FREQ__

#define TBL_ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define DEFINE_BAND_INFO(_min, _max, _idx)	\
{							\
	.min = _min,			\
	.max = _max,			\
	.freq_idx = _idx,		\
}

#define DEFINE_RIL_INFO(_array)	\
{								\
	.array = _array,			\
	.size = TBL_ARR_SIZE(_array),	\
}

struct band_info {
	int	min;
	int max;
	int freq_idx;
};

struct ril_band_info {
	struct band_info *array;
	unsigned int size;
};

/* refer to platform/hardware/ril/secril/modem/ipc/ipc41/ipcdefv41.h */
struct __packed ril_noti_info {
	u8 rat;
	u32 band;
	u32 channel;
};

enum {
	RIL_BAND_GSM850	= 1,
	RIL_BAND_EGSM900 = 2,
	RIL_BAND_DCS1800 = 3,
	RIL_BAND_PCS1900 = 4,
	RIL_BAND_WB01 = 11,
	RIL_BAND_WB02 = 12,
	RIL_BAND_WB03 = 13,
	RIL_BAND_WB04 = 14,
	RIL_BAND_WB05 = 15,
	RIL_BAND_WB07 = 17,
	RIL_BAND_WB08 = 18,
	RIL_BAND_BC0  = 61,
	RIL_BAND_BC1  = 62,
	RIL_BAND_BC10 = 71,
	RIL_BAND_LB01 = 91,
	RIL_BAND_LB02 = 92,
	RIL_BAND_LB03 = 93,
	RIL_BAND_LB04 = 94,
	RIL_BAND_LB05 = 95,
	RIL_BAND_LB07 = 97,
	RIL_BAND_LB08 = 98,
	RIL_BAND_LB12 = 102,
	RIL_BAND_LB13 = 103,
	RIL_BAND_LB14 = 104,
	RIL_BAND_LB17 = 107,
	RIL_BAND_LB18 = 108,
	RIL_BAND_LB19 = 109,
	RIL_BAND_LB20 = 110,
	RIL_BAND_LB21 = 111,
	RIL_BAND_LB25 = 115,
	RIL_BAND_LB26 = 116,
	RIL_BAND_LB28 = 118,
	RIL_BAND_LB29 = 119,
	RIL_BAND_LB30 = 120,
	RIL_BAND_LB32 = 122,
	RIL_BAND_LB34 = 124,
	RIL_BAND_LB38 = 128,
	RIL_BAND_LB39 = 129,
	RIL_BAND_LB40 = 130,
	RIL_BAND_LB41 = 131,
	RIL_BAND_LB42 = 132,
	RIL_BAND_LB48 = 138,
	RIL_BAND_LB66 = 156,
	RIL_BAND_LB71 = 161,
	RIL_BAND_MAX = 162,
};
#endif //__RIL_FREQ__
