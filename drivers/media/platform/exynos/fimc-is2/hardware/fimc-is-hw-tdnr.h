/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_TDNR_H
#define FIMC_IS_HW_TDNR_H

#define MAX_NOISEINDEX_DEPENDED_CONFIGS		(9)

/* DDK delivered NI to multiply 10 */
#define MULTIPLIED_NI(value)		(10 * (value))
#define INTERPOLATE_SHIFT			(12)
#define INTERPOLATE_NUMERATOR(Y1, Y2, diff_x_x1) \
	((((Y2) - (Y1)) * (diff_x_x1)) << INTERPOLATE_SHIFT)
#define GET_LINEAR_INTERPOLATE_VALUE(Y1, Y2, diff_x2_x1, diff_x_x1)		\
	(diff_x2_x1) ? (((INTERPOLATE_NUMERATOR((int)Y1, (int)Y2, diff_x_x1)) / (diff_x2_x1)) + \
					(((int)(Y1) << INTERPOLATE_SHIFT))) :	\
					(int)(Y1) << INTERPOLATE_SHIFT
#define RESTORE_SHIFT_VALUE(value) ((int)(value) >> INTERPOLATE_SHIFT)

enum tdnr_mode {
	TDNR_MODE_BYPASS = 0,
	TDNR_MODE_2DNR,
	TDNR_MODE_3DNR,
	TDNR_MODE_INVALID,
};

enum tdnr_buf_type {
	TDNR_IMAGE = 0,
	TDNR_WEIGHT,
};

enum tdnr_refine_mode {
	TDNR_REFINE_MAX = 0,
	TDNR_REFINE_MEAN,
	TDNR_REFINE_MIN,
};

enum tdnr_weight_mode {
	TDNR_WEIGHT_MAX = 0,
	TDNR_WEIGHT_MIN,
	TDNR_WEIGHT_LUMA,
};

enum arr_index3 {
	ARR3_VAL1 = 0,
	ARR3_VAL2,
	ARR3_VAL3,
	ARR3_MAX,
};

enum arr_index4 {
	ARR4_VAL1 = 0,
	ARR4_VAL2,
	ARR4_VAL3,
	ARR4_VAL4,
	ARR4_MAX,
};

struct general_config {
	bool	use_average_current;
	bool	auto_coeff_3d;
	u32	blending_threshold;
	u32     temporal_blend_thresh_weight_min;
	u32     temporal_blend_thresh_weight_max;
	u32     temporal_blend_thresh_criterion;
};

struct temporal_ni_dep_config {
	u32	temporal_weight_coeff_y1;
	u32	temporal_weight_coeff_y2;
	u32	temporal_weight_coeff_uv1;
	u32	temporal_weight_coeff_uv2;

	u32	auto_lut_gains_y[ARR3_MAX];
	u32	y_offset;
	u32	auto_lut_gains_uv[ARR3_MAX];
	u32	uv_offset;
};

struct temporal_ni_indep_config {
	u32	prev_gridx[ARR3_MAX];
	u32	prev_gridx_lut[ARR4_MAX];
};

struct refine_control_config {
	bool			is_refine_on;
	enum tdnr_refine_mode	refine_mode;
	u32			refine_threshold;
	u32			refine_coeff_update;
};

struct regional_ni_dep_config {
	bool	is_region_diff_on;
	u32	region_gain;
	bool	other_channels_check;
	u32	other_channel_gain;
};

struct regional_ni_indep_config {
	bool	dont_use_region_sign;
	bool	diff_condition_are_all_components_similar;
	bool	line_condition;
	bool	is_motiondetect_luma_mode_mean;
	u32	region_offset;
	bool	is_motiondetect_chroma_mode_mean;
	u32	other_channel_offset;
	u32	coefficient_offset;
};

struct spatial_ni_dep_config {
	enum tdnr_weight_mode	weight_mode;
	u32			spatial_gain;
	bool			spatial_separate_weights;
	u32			spatial_luma_gain[ARR4_MAX];
	u32			spatial_uv_gain[ARR4_MAX];
};

struct spatial_ni_indep_config {
	u32	spatial_refine_threshold;
	u32	spatial_luma_offset[ARR4_MAX];
	u32	spatial_uv_offset[ARR4_MAX];
};

struct yuv_table_config {
	u32	x_grid_y[ARR3_MAX];
	u32	y_std_offset;
	u32	y_std_slope[ARR4_MAX];

	u32	x_grid_u[ARR3_MAX];
	u32	u_std_offset;
	u32	u_std_slope[ARR4_MAX];

	u32	x_grid_v[ARR3_MAX];
	u32	v_std_offset;
	u32	v_std_slope[ARR4_MAX];
};

struct ni_dep_factors {
	u32			noise_index;
	u32			temporal_motion_detection_luma_low;
	u32			temporal_motion_detection_luma_contrast;
	u32			temporal_motion_detection_luma_high;
	bool			temporal_motion_detection_luma_off;
	u32			temporal_motion_detection_chroma_low;
	u32			temporal_motion_detection_chroma_contrast;
	u32			temporal_motion_detection_chroma_high;
	bool			temporal_motion_detection_chroma_off;
	u32			temporal_weight_luma_power_base;
	u32			temporal_weight_luma_power_gamma;
	u32			temporal_weight_chroma_power_base;
	u32			temporal_weight_chroma_power_gamma;
	bool			temporal_weight_hot_region;
	u32			temporal_weight_hot_region_power;
	bool			temporal_weight_chroma_threshold;
	u32			temporal_weight_chroma_power;
	u32			spatial_power;
	enum tdnr_weight_mode	spatial_weight_mode;
	bool			spatial_separate_weighting;
	u32			spatial_pd_luma_slope;
	u32			spatial_pd_luma_offset;
	u32			spatial_pd_chroma_slope;
	u32			spatial_pd_chroma_offset;

	struct yuv_table_config	yuv_tables;
};

struct tdnr_setfile_contents {
	bool	tdnr_enable;
	u32	num_of_noiseindexes;
	u32	compression_binary_error_thr;

	struct general_config			general_cfg;
	struct temporal_ni_indep_config		temporal_indep_cfg;
	u32					constant_lut_coeffs[ARR3_MAX];
	struct refine_control_config		refine_cfg;
	struct regional_ni_indep_config		regional_indep_cfg;
	struct spatial_ni_indep_config		spatial_indep_cfg;
	struct ni_dep_factors			ni_dep_factors[MAX_NOISEINDEX_DEPENDED_CONFIGS];
};

struct tdnr_configs {
	struct general_config			general_cfg;
	struct yuv_table_config			yuv_tables;
	struct temporal_ni_dep_config		temporal_dep_cfg;
	struct temporal_ni_indep_config		temporal_indep_cfg;
	u32					constant_lut_coeffs[ARR3_MAX];
	struct refine_control_config		refine_cfg;
	struct regional_ni_dep_config		regional_dep_cfg;
	struct regional_ni_indep_config		regional_indep_cfg;
	struct spatial_ni_dep_config		spatial_dep_cfg;
	struct spatial_ni_indep_config		spatial_indep_cfg;
};

#endif
