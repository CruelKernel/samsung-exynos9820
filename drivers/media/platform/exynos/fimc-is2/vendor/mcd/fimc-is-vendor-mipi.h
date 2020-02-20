/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is mipi channel definition
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_VENDER_MIPI_H
#define FIMC_IS_VENDER_MIPI_H

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
#define CAM_RAT_BAND(rat, band) ((rat<<16)|(band & 0xffff))
#define CAM_GET_RAT(rat_band) ((rat_band & 0xffff0000)>>16)
#define CAM_GET_BAND(rat_band) (0xffff & rat_band)

#define CAM_MIPI_NOT_INITIALIZED	-1

/* refer to platform/hardware/ril/secril/modem/ipc/ipc41/ipcdefv41.h */
struct __packed cam_cp_noti_info {
	u8 rat;
	u32 band;
	u32 channel;
};

/* for searching mipi channel */
struct cam_mipi_channel {
	u32 rat_band;
	u32 channel_min;
	u32 channel_max;
	u16 setting_index;
};

struct cam_mipi_setting {
	const char *str_mipi_clk;
	const u32 mipi_rate; /* it's not internal mipi clock */
	const u32 *setting;
	const u32 setting_size;
};

struct cam_mipi_sensor_mode {
	const u32 mode;
	const struct cam_mipi_channel *mipi_channel;
	const u32 mipi_channel_size;
	const struct cam_mipi_setting *sensor_setting;
	const u32 sensor_setting_size;
};

struct cam_tof_setting {
	const u32 tx_freq;
	const u32 *setting;
	const u32 setting_size;
};

struct cam_tof_sensor_mode {
	const u32 mode;
	const struct cam_mipi_channel *mipi_channel;
	const u32 mipi_channel_size;
	const struct cam_tof_setting *sensor_setting;
	const u32 sensor_setting_size;
};

/* RAT */
enum {
	CAM_RAT_1_GSM = 1,
	CAM_RAT_2_WCDMA = 2,
	CAM_RAT_3_LTE = 3,
	CAM_RAT_4_TDSCDMA = 4,
	CAM_RAT_5_CDMA = 5,
	CAM_RAT_6_WIFI = 6,
	CAM_RAT_7_NR = 7,
};

/* BAND */
enum {
	CAM_BAND_001_GSM_GSM850 = 1,
	CAM_BAND_002_GSM_EGSM900 = 2,
	CAM_BAND_003_GSM_DCS1800 = 3,
	CAM_BAND_004_GSM_PCS1900 = 4,

	CAM_BAND_011_WCDMA_WB01 = 11,
	CAM_BAND_012_WCDMA_WB02 = 12,
	CAM_BAND_013_WCDMA_WB03 = 13,
	CAM_BAND_014_WCDMA_WB04 = 14,
	CAM_BAND_015_WCDMA_WB05 = 15,
	CAM_BAND_016_WCDMA_WB06 = 16,
	CAM_BAND_017_WCDMA_WB07 = 17,
	CAM_BAND_018_WCDMA_WB08 = 18,
	CAM_BAND_019_WCDMA_WB09 = 19,
	CAM_BAND_020_WCDMA_WB10 = 20,
	CAM_BAND_021_WCDMA_WB11 = 21,
	CAM_BAND_022_WCDMA_WB12 = 22,
	CAM_BAND_023_WCDMA_WB13 = 23,
	CAM_BAND_024_WCDMA_WB14 = 24,
	CAM_BAND_025_WCDMA_WB15 = 25,
	CAM_BAND_026_WCDMA_WB16 = 26,
	CAM_BAND_027_WCDMA_WB17 = 27,
	CAM_BAND_028_WCDMA_WB18 = 28,
	CAM_BAND_029_WCDMA_WB19 = 29,
	CAM_BAND_030_WCDMA_WB20 = 30,
	CAM_BAND_031_WCDMA_WB21 = 31,
	CAM_BAND_032_WCDMA_WB22 = 32,
	CAM_BAND_033_WCDMA_WB23 = 33,
	CAM_BAND_034_WCDMA_WB24 = 34,
	CAM_BAND_035_WCDMA_WB25 = 35,
	CAM_BAND_036_WCDMA_WB26 = 36,
	CAM_BAND_037_WCDMA_WB27 = 37,
	CAM_BAND_038_WCDMA_WB28 = 38,
	CAM_BAND_039_WCDMA_WB29 = 39,
	CAM_BAND_040_WCDMA_WB30 = 40,
	CAM_BAND_041_WCDMA_WB31 = 41,
	CAM_BAND_042_WCDMA_WB32 = 42,

	CAM_BAND_051_TDSCDMA_TD1 = 51,
	CAM_BAND_052_TDSCDMA_TD2 = 52,
	CAM_BAND_053_TDSCDMA_TD3 = 53,
	CAM_BAND_054_TDSCDMA_TD4 = 54,
	CAM_BAND_055_TDSCDMA_TD5 = 55,
	CAM_BAND_056_TDSCDMA_TD6 = 56,

	CAM_BAND_061_CDMA_BC0 = 61,
	CAM_BAND_062_CDMA_BC1 = 62,
	CAM_BAND_063_CDMA_BC2 = 63,
	CAM_BAND_064_CDMA_BC3 = 64,
	CAM_BAND_065_CDMA_BC4 = 65,
	CAM_BAND_066_CDMA_BC5 = 66,
	CAM_BAND_067_CDMA_BC6 = 67,
	CAM_BAND_068_CDMA_BC7 = 68,
	CAM_BAND_069_CDMA_BC8 = 69,
	CAM_BAND_070_CDMA_BC9 = 70,
	CAM_BAND_071_CDMA_BC10 = 71,
	CAM_BAND_072_CDMA_BC11 = 72,
	CAM_BAND_073_CDMA_BC12 = 73,
	CAM_BAND_074_CDMA_BC13 = 74,
	CAM_BAND_075_CDMA_BC14 = 75,
	CAM_BAND_076_CDMA_BC15 = 76,
	CAM_BAND_077_CDMA_BC16 = 77,
	CAM_BAND_078_CDMA_BC17 = 78,
	CAM_BAND_079_CDMA_BC18 = 79,
	CAM_BAND_080_CDMA_BC19 = 80,
	CAM_BAND_081_CDMA_BC20 = 81,
	CAM_BAND_082_CDMA_BC21 = 82,

	CAM_BAND_091_LTE_LB01 = 91,
	CAM_BAND_092_LTE_LB02 = 92,
	CAM_BAND_093_LTE_LB03 = 93,
	CAM_BAND_094_LTE_LB04 = 94,
	CAM_BAND_095_LTE_LB05 = 95,
	CAM_BAND_096_LTE_LB06 = 96,
	CAM_BAND_097_LTE_LB07 = 97,
	CAM_BAND_098_LTE_LB08 = 98,
	CAM_BAND_099_LTE_LB09 = 99,
	CAM_BAND_100_LTE_LB10 = 100,
	CAM_BAND_101_LTE_LB11 = 101,
	CAM_BAND_102_LTE_LB12 = 102,
	CAM_BAND_103_LTE_LB13 = 103,
	CAM_BAND_104_LTE_LB14 = 104,
	CAM_BAND_105_LTE_LB15 = 105,
	CAM_BAND_106_LTE_LB16 = 106,
	CAM_BAND_107_LTE_LB17 = 107,
	CAM_BAND_108_LTE_LB18 = 108,
	CAM_BAND_109_LTE_LB19 = 109,
	CAM_BAND_110_LTE_LB20 = 110,
	CAM_BAND_111_LTE_LB21 = 111,
	CAM_BAND_112_LTE_LB22 = 112,
	CAM_BAND_113_LTE_LB23 = 113,
	CAM_BAND_114_LTE_LB24 = 114,
	CAM_BAND_115_LTE_LB25 = 115,
	CAM_BAND_116_LTE_LB26 = 116,
	CAM_BAND_117_LTE_LB27 = 117,
	CAM_BAND_118_LTE_LB28 = 118,
	CAM_BAND_119_LTE_LB29 = 119,
	CAM_BAND_120_LTE_LB30 = 120,
	CAM_BAND_121_LTE_LB31 = 121,
	CAM_BAND_122_LTE_LB32 = 122,
	CAM_BAND_123_LTE_LB33 = 123,
	CAM_BAND_124_LTE_LB34 = 124,
	CAM_BAND_125_LTE_LB35 = 125,
	CAM_BAND_126_LTE_LB36 = 126,
	CAM_BAND_127_LTE_LB37 = 127,
	CAM_BAND_128_LTE_LB38 = 128,
	CAM_BAND_129_LTE_LB39 = 129,
	CAM_BAND_130_LTE_LB40 = 130,
	CAM_BAND_131_LTE_LB41 = 131,
	CAM_BAND_132_LTE_LB42 = 132,
	CAM_BAND_133_LTE_LB43 = 133,
	CAM_BAND_134_LTE_LB44 = 134,
	CAM_BAND_135_LTE_LB45 = 135,
	CAM_BAND_136_LTE_LB46 = 136,
	CAM_BAND_137_LTE_LB47 = 137,
	CAM_BAND_138_LTE_LB48 = 138,
	CAM_BAND_139_LTE_LB49 = 139,
	CAM_BAND_140_LTE_LB50 = 140,
	CAM_BAND_141_LTE_LB51 = 141,
	CAM_BAND_142_LTE_LB52 = 142,
	CAM_BAND_143_LTE_LB53 = 143,
	CAM_BAND_144_LTE_LB54 = 144,
	CAM_BAND_145_LTE_LB55 = 145,
	CAM_BAND_146_LTE_LB56 = 146,
	CAM_BAND_147_LTE_LB57 = 147,
	CAM_BAND_148_LTE_LB58 = 148,
	CAM_BAND_149_LTE_LB59 = 149,
	CAM_BAND_150_LTE_LB60 = 150,
	CAM_BAND_151_LTE_LB61 = 151,
	CAM_BAND_152_LTE_LB62 = 152,
	CAM_BAND_153_LTE_LB63 = 153,
	CAM_BAND_154_LTE_LB64 = 154,
	CAM_BAND_155_LTE_LB65 = 155,
	CAM_BAND_156_LTE_LB66 = 156,
	CAM_BAND_157_LTE_LB67 = 157,
	CAM_BAND_158_LTE_LB68 = 158,
	CAM_BAND_159_LTE_LB69 = 159,
	CAM_BAND_160_LTE_LB70 = 160,
	CAM_BAND_161_LTE_LB71 = 161,
	
	CAM_BAND_255_NR_NRxx = 255,
};

int fimc_is_vendor_select_mipi_by_rf_channel(const struct cam_mipi_channel *channel_list, const int size);
int fimc_is_vendor_verify_mipi_channel(const struct cam_mipi_channel *channel_list, const int size);
#endif

#endif /* FIMC_IS_VENDER_MIPI_H */
