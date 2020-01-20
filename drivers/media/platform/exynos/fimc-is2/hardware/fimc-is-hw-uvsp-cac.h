/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_UVSP_CAC_H
#define FIMC_IS_HW_UVSP_CAC_H

#define CAC_MAX_NI_DEPENDED_CFG     (9)
struct cac_map_thr_cfg {
	u32	map_spot_thr_l;
	u32	map_spot_thr_h;
	u32	map_spot_thr;
	u32	map_spot_nr_strength;
};

struct cac_crt_thr_cfg {
	u32	crt_color_thr_l_dot;
	u32	crt_color_thr_l_line;
	u32	crt_color_thr_h;
};

struct cac_cfg_by_ni {
	struct cac_map_thr_cfg	map_thr_cfg;
	struct cac_crt_thr_cfg	crt_thr_cfg;
};

struct cac_setfile_contents {
	u32	ni_max;
	u32	ni_vals[CAC_MAX_NI_DEPENDED_CFG];
	struct cac_cfg_by_ni	cfgs[CAC_MAX_NI_DEPENDED_CFG];
};

#define UVSP_MAX_NI_DEPENDED_CONFIGS     (9)
struct uvsp_ctrl {
	u32	binning_x;
	u32	binning_y;

	u32	radial_center_x;
	u32	radial_center_y;

	u32	biquad_a;
	u32	biquad_b;
};

struct uvsp_radial_cfg {
	u32	biquad_shift;

	bool	random_en;
	u32	random_power;
	bool	refine_en;
	u32	refine_luma_min;
	u32	refine_denom;

	bool	alpha_gain_add_en;
	bool	alpha_green_en;
	u32	alpha_r;
	u32	alpha_g;
	u32	alpha_b;
};

struct uvsp_pedestal_cfg {
	u32	r;
	u32	g;
	u32	b;
};

struct uvsp_offset_cfg {
	u32	r;
	u32	g;
	u32	b;
};

struct uvsp_desat_cfg {
	bool	ctrl_en;
	bool	ctrl_singleSide;
	u32	ctrl_luma_offset;
	u32	ctrl_gain_offset;

	u32	y_shift;
	u32	y_luma_max;
	u32	u_low;
	u32	u_high;
	u32	v_low;
	u32	v_high;
};

struct uvsp_r2y_coef_cfg {
	u32	r2y_coef_00;
	u32	r2y_coef_01;
	u32	r2y_coef_02;
	u32	r2y_coef_10;
	u32	r2y_coef_11;
	u32	r2y_coef_12;
	u32	r2y_coef_20;
	u32	r2y_coef_21;
	u32	r2y_coef_22;
	u32	r2y_coef_shift;
};

struct uvsp_cfg_by_ni {
	struct uvsp_radial_cfg		radial_cfg;
	struct uvsp_pedestal_cfg	pedestal_cfg;
	struct uvsp_offset_cfg		offset_cfg;
	struct uvsp_desat_cfg		desat_cfg;
	struct uvsp_r2y_coef_cfg	r2y_coef_cfg;
};

struct uvsp_setfile_contents {
	u32	ni_max;
	u32	ni_vals[UVSP_MAX_NI_DEPENDED_CONFIGS];
	struct uvsp_cfg_by_ni	cfgs[UVSP_MAX_NI_DEPENDED_CONFIGS];
};
#endif
