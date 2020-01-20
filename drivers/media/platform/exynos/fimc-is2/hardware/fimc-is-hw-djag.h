/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_DJAG_H
#define FIMC_IS_HW_DJAG_H

/* for DeJag Block */
#define MCSC_DJAG_PRESCALE_INDEX_1		0	/* x1.0 */
#define MCSC_DJAG_PRESCALE_INDEX_2		1	/* x1.1~x1.4 */
#define MCSC_DJAG_PRESCALE_INDEX_3		2	/* x1.5~x2.0 */
#define MCSC_DJAG_PRESCALE_INDEX_4		3	/* x2.1~ */

#define MAX_SCALINGRATIOINDEX_DEPENDED_CONFIGS	(4)
#define MAX_DITHER_VALUE_CONFIGS				(9)

struct djag_xfilter_dejagging_coeff_config {
	u32 xfilter_dejagging_weight0;
	u32 xfilter_dejagging_weight1;
	u32 xfilter_hf_boost_weight;
	u32 center_hf_boost_weight;
	u32 diagonal_hf_boost_weight;
	u32 center_weighted_mean_weight;
};

struct djag_thres_1x5_matching_config {
	u32 thres_1x5_matching_sad;
	u32 thres_1x5_abshf;
};

struct djag_thres_shooting_detect_config {
	u32 thres_shooting_llcrr;
	u32 thres_shooting_lcr;
	u32 thres_shooting_neighbor;
	u32 thres_shooting_uucdd;
	u32 thres_shooting_ucd;
	u32 min_max_weight;
};

struct djag_lfsr_seed_config {
	u32 lfsr_seed_0;
	u32 lfsr_seed_1;
	u32 lfsr_seed_2;
};

struct djag_dither_config {
	u32 dither_value[MAX_DITHER_VALUE_CONFIGS];
	u32 sat_ctrl;
	u32 dither_sat_thres;
	u32 dither_thres;
};

struct djag_cp_config {
	u32 cp_hf_thres;
	u32 cp_arbi_max_cov_offset;
	u32 cp_arbi_max_cov_shift;
	u32 cp_arbi_denom;
	u32 cp_arbi_mode;
};

struct djag_setfile_contents {
	struct djag_xfilter_dejagging_coeff_config xfilter_dejagging_coeff_cfg;
	struct djag_thres_1x5_matching_config thres_1x5_matching_cfg;
	struct djag_thres_shooting_detect_config thres_shooting_detect_cfg;
	struct djag_lfsr_seed_config lfsr_seed_cfg;
	struct djag_dither_config dither_cfg;
	struct djag_cp_config cp_cfg;
};

struct djag_wb_thres_cfg {
	u32 dither_wb_thres;
};
#endif
