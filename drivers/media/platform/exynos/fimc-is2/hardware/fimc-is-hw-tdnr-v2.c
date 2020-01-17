/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-mcscaler-v2.h"
#include "api/fimc-is-hw-api-mcscaler-v2.h"
#include "../interface/fimc-is-interface-ischain.h"
#include "fimc-is-param.h"
#include "fimc-is-err.h"

static const struct tdnr_configs init_tdnr_cfgs = {
	.general_cfg = {
		.use_average_current = true,
		.auto_coeff_3d = true,
		.blending_threshold = 6,
	},
	.yuv_tables = {
		.x_grid_y = {57, 89, 126},
		.y_std_offset = 27,
		.y_std_slope = {-22, -6, -33, 31},
		.x_grid_u = {57, 89, 126},
		.u_std_offset = 4,
		.u_std_slope = {36, 2, -24, 1},
		.x_grid_v = {57, 89, 126},
		.v_std_offset = 4,
		.v_std_slope = {36, 3, -13, -25},
	},
	.temporal_dep_cfg = {
		.temporal_weight_coeff_y1 = 6,
		.temporal_weight_coeff_y2 = 12,
		.temporal_weight_coeff_uv1 = 6,
		.temporal_weight_coeff_uv2 = 12,
		.auto_lut_gains_y = {3, 9, 15},
		.y_offset = 0,
		.auto_lut_gains_uv = {3, 9, 15},
		.uv_offset = 0,
	},
	.temporal_indep_cfg = {
		.prev_gridx = {4, 8, 12},
		.prev_gridx_lut = {2, 6, 10, 14},
	},
	.constant_lut_coeffs = {0, 0, 0},
	.refine_cfg = {
		.is_refine_on = true,
		.refine_mode = TDNR_REFINE_MAX,
		.refine_threshold = 12,
		.refine_coeff_update = 4,
	},
	.regional_dep_cfg = {
		.is_region_diff_on = false,
		.region_gain = 15,
		.other_channels_check = false,
		.other_channel_gain = 10,
	},
	.regional_indep_cfg = {
		.dont_use_region_sign = true,
		.diff_condition_are_all_components_similar = false,
		.line_condition = true,
		.is_motiondetect_luma_mode_mean = true,
		.region_offset = 0,
		.is_motiondetect_chroma_mode_mean = true,
		.other_channel_offset = 0,
		.coefficient_offset = 16,
	},
	.spatial_dep_cfg = {
		.weight_mode = TDNR_WEIGHT_MIN,
		.spatial_gain = 4,
		.spatial_separate_weights = false,
		.spatial_luma_gain = {34, 47, 60, 73},
		.spatial_uv_gain = {34, 47, 60, 73},
	},
	.spatial_indep_cfg = {
		.spatial_refine_threshold = 17,
		.spatial_luma_offset = {0, 0, 0, 0},
		.spatial_uv_offset = {0, 0, 0, 0},
	},
};

void fimc_is_hw_mcsc_tdnr_init(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *mcs_param, u32 instance)
{
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;

	BUG_ON(!hw_ip->priv_info);
	BUG_ON(!mcs_param);

	cap = GET_MCSC_HW_CAP(hw_ip);
	if (cap->tdnr != MCSC_CAP_SUPPORT)
		return;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
#ifdef ENABLE_DNR_IN_MCSC
	hw_mcsc->tdnr_first = MCSC_DNR_USE_FIRST;
	hw_mcsc->tdnr_output = MCSC_DNR_OUTPUT_INDEX;
	hw_mcsc->tdnr_internal_buf = MCSC_DNR_USE_INTERNAL_BUF;
	hw_mcsc->cur_tdnr_mode = TDNR_MODE_BYPASS;
	hw_mcsc->yic_en = TDNR_YIC_ENABLE;

	if (hw_mcsc->tdnr_internal_buf && mcs_param->control.buffer_address) {
		hw_mcsc->dvaddr_tdnr[0] = mcs_param->control.buffer_address;
		hw_mcsc->dvaddr_tdnr[1] = hw_mcsc->dvaddr_tdnr[0] +
			ALIGN(MCSC_DNR_WIDTH * MCSC_DNR_HEIGHT * 2, 16);
	}

	fimc_is_scaler_clear_tdnr_rdma_addr(hw_ip->regs, TDNR_IMAGE);
	fimc_is_scaler_clear_tdnr_wdma_addr(hw_ip->regs, TDNR_IMAGE);

	memcpy(&hw_mcsc->tdnr_cfgs, &init_tdnr_cfgs, sizeof(struct tdnr_configs));
#endif
}

void fimc_is_hw_mcsc_tdnr_deinit(struct fimc_is_hw_ip *hw_ip,
	struct mcs_param *mcs_param, u32 instance)
{
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap;

	BUG_ON(!hw_ip->priv_info);
	BUG_ON(!mcs_param);

	cap = GET_MCSC_HW_CAP(hw_ip);
	if (cap->tdnr != MCSC_CAP_SUPPORT)
		return;

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	fimc_is_scaler_set_tdnr_mode_select(hw_ip->regs, TDNR_MODE_BYPASS);

	fimc_is_scaler_clear_tdnr_rdma_addr(hw_ip->regs, TDNR_IMAGE);
	fimc_is_scaler_clear_tdnr_rdma_addr(hw_ip->regs, TDNR_WEIGHT);

	fimc_is_scaler_set_tdnr_wdma_enable(hw_ip->regs, TDNR_WEIGHT, false);
	fimc_is_scaler_clear_tdnr_wdma_addr(hw_ip->regs, TDNR_WEIGHT);

	hw_mcsc->cur_tdnr_mode = TDNR_MODE_BYPASS;
}

static int fimc_is_hw_mcsc_check_tdnr_mode_pre(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head,
	struct fimc_is_frame *frame,
	struct tpu_param *tpu_param,
	struct mcs_param *mcs_param,
	enum tdnr_mode cur_mode)
{
	enum tdnr_mode tdnr_mode;
	struct fimc_is_hw_mcsc *hw_mcsc;
	struct fimc_is_hw_mcsc_cap *cap = GET_MCSC_HW_CAP(hw_ip);
	u32 lindex, hindex;
#ifdef MCSC_DNR_USE_TUNING
	enum exynos_sensor_position sensor_position;
	tdnr_setfile_contents *tdnr_tuneset;
#endif
	bool setfile_tdnr_enable = true;

	BUG_ON(!hw_ip->priv_info);
	BUG_ON(!tpu_param);
	BUG_ON(!cap);

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	/* bypass setting at below case
	 * 1. dnr_bypass parameter is true
	 * 2. internal shot
	 * 3. mcsc setfile "tdnr_enable" is "0"
	 */
#ifdef MCSC_DNR_USE_TUNING
	sensor_position = hw_ip->hardware->sensor_position[atomic_read(&hw_ip->instance)];
	tdnr_tuneset = &hw_mcsc->cur_setfile[sensor_position][frame->instance]->tdnr_contents;
	setfile_tdnr_enable = tdnr_tuneset->tdnr_enable;
#endif

	if ((tpu_param->config.tdnr_bypass == true)
		|| (frame->type == SHOT_TYPE_INTERNAL)
		|| setfile_tdnr_enable == false) {
		tdnr_mode = TDNR_MODE_BYPASS;
	} else {
		lindex = frame->shot->ctl.vendor_entry.lowIndexParam;
		hindex = frame->shot->ctl.vendor_entry.highIndexParam;

		/* 2dnr setting at below case
		 * 1. bypass true -> false setting
		 * 2. head group shot count is "0"(first shot)
		 * 3. tdnr wdma size changed
		 * 4. tdnr wdma dma out disabled
		 * 5. setfile tuneset changed(TODO)
		 */
		if ((cur_mode == TDNR_MODE_BYPASS)
			|| (!atomic_read(&head->scount))
			|| (lindex & LOWBIT_OF(PARAM_MCS_INPUT))
			|| (hindex & HIGHBIT_OF(PARAM_MCS_INPUT)))
			tdnr_mode = TDNR_MODE_2DNR; /* first frame */
		else
			/* set to 3DNR mode */
			tdnr_mode = TDNR_MODE_3DNR;
	}

	return tdnr_mode;
}

static void fimc_is_hw_mcsc_check_tdnr_mode_post(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head,
	enum tdnr_mode cur_tdnr_mode,
	enum tdnr_mode *changed_tdnr_mode)
{
	/* at FULL - OTF scenario, if register update is not finished
	 * until frame end interrupt occured,
	 * 3DNR RDMA transaction timing can be delayed.
	 * so, It should set to 2DNR mode for not operate RDMA
	 */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state)
		&& atomic_read(&hw_ip->status.Vvalid) == V_BLANK) {
		*changed_tdnr_mode = TDNR_MODE_2DNR;
		warn_hw("[ID:%d] TDNR mode changed(%d->%d) due to not finished update",
			hw_ip->id, cur_tdnr_mode, *changed_tdnr_mode);
	}
}

static int fimc_is_hw_mcsc_cfg_tdnr_rdma(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head,
	struct fimc_is_frame *frame,
	struct mcs_param *mcs_param,
	enum tdnr_mode tdnr_mode)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 prev_wdma_addr = 0;
	u32 prev_width = 0, prev_height = 0;
	u32 prev_y_stride = 0, prev_uv_stride = 0;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		warn_hw("wrong TDNR setting at internal shot");
		return -EINVAL;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	/* buffer addr setting
	 * at OTF, RDMA[n+1] = WDMA[n]
	 *
	 * image buffer
	 */
	if (tdnr_mode == TDNR_MODE_2DNR) {
		fimc_is_scaler_clear_tdnr_rdma_addr(hw_ip->regs, TDNR_IMAGE);
		fimc_is_scaler_clear_tdnr_rdma_addr(hw_ip->regs, TDNR_WEIGHT);
		return ret;
	}

	fimc_is_scaler_get_tdnr_wdma_addr(hw_ip->regs, TDNR_IMAGE,
			&prev_wdma_addr, NULL, NULL, USE_DMA_BUFFER_INDEX);
	fimc_is_scaler_set_tdnr_rdma_addr(hw_ip->regs, TDNR_IMAGE,
			prev_wdma_addr, 0, 0, USE_DMA_BUFFER_INDEX);

	if (hw_mcsc->cur_tdnr_mode == TDNR_MODE_3DNR && tdnr_mode == TDNR_MODE_3DNR)
		return ret;

	/* 2DNR -> 3DNR */
	fimc_is_scaler_get_tdnr_wdma_size(hw_ip->regs,
			TDNR_IMAGE, &prev_width, &prev_height);
	fimc_is_scaler_get_tdnr_wdma_stride(hw_ip->regs,
			TDNR_IMAGE, &prev_y_stride, &prev_uv_stride);

	if (prev_width == 0 || prev_height == 0
			|| prev_y_stride == 0) {
		warn_hw("[ID:%d] MCSC prev_image size(%dx%d), stride(y:%d)is invalid",
				hw_ip->id, prev_width, prev_height, prev_y_stride);
		return -EINVAL;
	}

	fimc_is_scaler_set_tdnr_rdma_size(hw_ip->regs,
			TDNR_IMAGE, prev_width, prev_height);
	fimc_is_scaler_set_tdnr_rdma_stride(hw_ip->regs,
			TDNR_IMAGE, prev_y_stride, 0);

	return ret;
}

static int fimc_is_hw_mcsc_cfg_tdnr_wdma(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head,
	struct fimc_is_frame *frame,
	struct mcs_param *mcs_param,
	enum tdnr_mode tdnr_mode)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc;
	u32 prev_wdma_addr = 0, wdma_addr = 0;
	u32 wdma_width = 0, wdma_height = 0, img_width = 0, img_height = 0;
	u32 img_y_stride = 0;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		warn_hw("wrong TDNR setting at internal shot");
		return -EINVAL;
	}

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	switch (tdnr_mode) {
	case TDNR_MODE_2DNR:
		wdma_addr = hw_mcsc->dvaddr_tdnr[0];
		break;
	case TDNR_MODE_3DNR:
		fimc_is_scaler_get_tdnr_wdma_addr(hw_ip->regs, TDNR_IMAGE,
				&prev_wdma_addr, NULL, NULL, USE_DMA_BUFFER_INDEX);
		wdma_addr = (prev_wdma_addr == hw_mcsc->dvaddr_tdnr[0]) ?
			hw_mcsc->dvaddr_tdnr[1] : hw_mcsc->dvaddr_tdnr[0];
		break;
	case TDNR_MODE_BYPASS:
	default:
		return -EINVAL;
	}

	fimc_is_scaler_set_tdnr_wdma_addr(hw_ip->regs, TDNR_IMAGE,
			wdma_addr, 0, 0, USE_DMA_BUFFER_INDEX);

	if (hw_mcsc->cur_tdnr_mode == TDNR_MODE_3DNR && tdnr_mode == TDNR_MODE_3DNR)
		return ret;

	switch (tdnr_mode) {
	case TDNR_MODE_2DNR:
	case TDNR_MODE_3DNR:
		img_width = mcs_param->input.width;
		img_height = mcs_param->input.height;
		break;
	case TDNR_MODE_BYPASS:
	default:
		return -EINVAL;
	}

	if (hw_mcsc->yic_en == TDNR_YIC_ENABLE) {
		/* Compression */
		wdma_width = ((((int)((img_width * 2 + 31) / 32)) + (int)(img_width / 2 )) * 2);
		wdma_height = (int)(img_height / 2);
	} else {
		/* No Compression */
		wdma_width = img_width;
		wdma_height = img_height;
	}
	fimc_is_scaler_set_tdnr_wdma_size(hw_ip->regs,
			TDNR_IMAGE, wdma_width, wdma_height);

	/* TDNR DMA stride = image size * 2 */
	img_y_stride = wdma_width * 2;
	fimc_is_scaler_set_tdnr_wdma_stride(hw_ip->regs,
			TDNR_IMAGE, img_y_stride, 0);

	return ret;
}

/* "TDNR translate function"
 * re_configure interpolated NI depended factor to SFR configurations
 */
static void translate_temporal_factor(struct temporal_ni_dep_config *temporal_cfg,
	struct ni_dep_factors interpolated_factor)
{
	ulong temp_val = 0;

	temporal_cfg->auto_lut_gains_y[ARR3_VAL1] =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_motion_detection_luma_low);
	temporal_cfg->auto_lut_gains_y[ARR3_VAL2] =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_motion_detection_luma_contrast);
	temporal_cfg->auto_lut_gains_y[ARR3_VAL3] =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_motion_detection_luma_high);

	temporal_cfg->auto_lut_gains_uv[ARR3_VAL1] =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_motion_detection_chroma_low);
	temporal_cfg->auto_lut_gains_uv[ARR3_VAL2] =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_motion_detection_chroma_contrast);
	temporal_cfg->auto_lut_gains_uv[ARR3_VAL3] =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_motion_detection_chroma_high);

	temporal_cfg->y_offset =
		interpolated_factor.temporal_motion_detection_luma_off ? 255 : 0;
	temporal_cfg->uv_offset =
		interpolated_factor.temporal_motion_detection_luma_off ? 255 : 0;

	/* value = 16 * (1 - source / 256)
	 *       = 2^4 - source / 2^4
	 *       = (2^8 - source) / 2^4
	 */
	temp_val = (1 << (8 + INTERPOLATE_SHIFT))
		- (ulong)interpolated_factor.temporal_weight_luma_power_base;
	temporal_cfg->temporal_weight_coeff_y1 = (u32)(temp_val >> (4 + INTERPOLATE_SHIFT));

	/* value = 16 * (1 - (source1 / 256) * (source2 / 256))
	 *       = 2^4 - ((source1 * source2) / 2^12)
	 *       = (2^16 - source1 * source2) / 2^12
	 */
	temp_val = RESTORE_SHIFT_VALUE(interpolated_factor.temporal_weight_luma_power_base) *
			RESTORE_SHIFT_VALUE(interpolated_factor.temporal_weight_luma_power_gamma);
	temporal_cfg->temporal_weight_coeff_y2 = ((1 << 16) - temp_val) >> 12;

	/* value = 16 * (1 - source / 256)
	 *       = 2^4 - source / 2^4
	 *       = (2^8 - source) / 2^4
	 */
	temp_val = (1 << (8 + INTERPOLATE_SHIFT))
		- (ulong)interpolated_factor.temporal_weight_chroma_power_base;
	temporal_cfg->temporal_weight_coeff_uv1 = (u32)(temp_val >> (4 + INTERPOLATE_SHIFT));

	/* value = 16 * (1 - (source1 / 256) * (source2 / 256))
	 *       = 2^4 - ((source1 * source2) / 2^12)
	 *       = (2^16 - source1 * source2) / 2^12
	 */
	temp_val = RESTORE_SHIFT_VALUE(interpolated_factor.temporal_weight_chroma_power_base) *
			RESTORE_SHIFT_VALUE(interpolated_factor.temporal_weight_chroma_power_gamma);
	temporal_cfg->temporal_weight_coeff_uv2 = ((1 << 16) - temp_val) >> 12;
}

static void translate_regional_factor(struct regional_ni_dep_config *regional_cfg,
	struct ni_dep_factors interpolated_factor)
{
	regional_cfg->is_region_diff_on = interpolated_factor.temporal_weight_hot_region;
	regional_cfg->region_gain =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_weight_hot_region_power);
	regional_cfg->other_channels_check =
		interpolated_factor.temporal_weight_chroma_threshold;
	regional_cfg->other_channel_gain =
		RESTORE_SHIFT_VALUE(interpolated_factor.temporal_weight_chroma_power);
}

static void translate_spatial_factor(struct spatial_ni_dep_config *spatial_cfg,
	struct ni_dep_factors interpolated_factor)
{
	ulong temp_val = 0;

	/* value = 16 * (1 - source / 256)
	 *       = 2^4 - source / 2^4
	 *       = (2^8 - source) / 2^4
	 */
	temp_val = (1 << (8 + INTERPOLATE_SHIFT))
		- (ulong)interpolated_factor.spatial_power;
	spatial_cfg->spatial_gain = (u32)(temp_val >> (4 + INTERPOLATE_SHIFT));
	spatial_cfg->weight_mode = interpolated_factor.spatial_weight_mode;
	spatial_cfg->spatial_separate_weights =
		interpolated_factor.spatial_separate_weighting;

	spatial_cfg->spatial_luma_gain[ARR4_VAL1] =
		RESTORE_SHIFT_VALUE(interpolated_factor.spatial_pd_luma_slope +
			interpolated_factor.spatial_pd_luma_offset);
	spatial_cfg->spatial_uv_gain[ARR4_VAL1] =
		RESTORE_SHIFT_VALUE(interpolated_factor.spatial_pd_chroma_slope +
			interpolated_factor.spatial_pd_chroma_offset);

	spatial_cfg->spatial_luma_gain[ARR4_VAL2] =
		RESTORE_SHIFT_VALUE(2 * interpolated_factor.spatial_pd_luma_slope +
			interpolated_factor.spatial_pd_luma_offset);
	spatial_cfg->spatial_uv_gain[ARR4_VAL2] =
		RESTORE_SHIFT_VALUE(2 * interpolated_factor.spatial_pd_chroma_slope +
			interpolated_factor.spatial_pd_chroma_offset);

	spatial_cfg->spatial_luma_gain[ARR4_VAL3] =
		RESTORE_SHIFT_VALUE(3 * interpolated_factor.spatial_pd_luma_slope +
			interpolated_factor.spatial_pd_luma_offset);
	spatial_cfg->spatial_uv_gain[ARR4_VAL3] =
		RESTORE_SHIFT_VALUE(3 * interpolated_factor.spatial_pd_chroma_slope +
			interpolated_factor.spatial_pd_chroma_offset);

	spatial_cfg->spatial_luma_gain[ARR4_VAL4] =
		RESTORE_SHIFT_VALUE(4 * interpolated_factor.spatial_pd_luma_slope +
			interpolated_factor.spatial_pd_luma_offset);
	spatial_cfg->spatial_uv_gain[ARR4_VAL4] =
		RESTORE_SHIFT_VALUE(4 * interpolated_factor.spatial_pd_chroma_slope +
			interpolated_factor.spatial_pd_chroma_offset);
}

static void translate_yuv_table_factor(struct yuv_table_config *yuv_table_cfg,
	struct ni_dep_factors interpolated_factor)
{
	int arr_idx;

	for (arr_idx = ARR3_VAL1; arr_idx < ARR3_MAX; arr_idx++) {
		yuv_table_cfg->x_grid_y[arr_idx] =
			RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.x_grid_y[arr_idx]);
		yuv_table_cfg->x_grid_u[arr_idx] =
			RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.x_grid_u[arr_idx]);
		yuv_table_cfg->x_grid_v[arr_idx] =
			RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.x_grid_v[arr_idx]);
	}

	for (arr_idx = ARR4_VAL1; arr_idx < ARR4_MAX; arr_idx++) {
		yuv_table_cfg->y_std_slope[arr_idx] =
			RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.y_std_slope[arr_idx]);
		yuv_table_cfg->u_std_slope[arr_idx] =
			RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.u_std_slope[arr_idx]);
		yuv_table_cfg->v_std_slope[arr_idx] =
			RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.v_std_slope[arr_idx]);
	}

	yuv_table_cfg->y_std_offset =
		RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.y_std_offset);

	yuv_table_cfg->u_std_offset =
		RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.u_std_offset);

	yuv_table_cfg->v_std_offset =
		RESTORE_SHIFT_VALUE(interpolated_factor.yuv_tables.v_std_offset);
}

/* TDNR interpolation function
 * NI depended value is get by reference NI value's linear interpolation
 */
static void interpolate_temporal_factor(struct ni_dep_factors *interpolated_factor,
	u32 noise_index,
	struct ni_dep_factors bottom_ni_factor,
	struct ni_dep_factors top_ni_factor)
{
	int bottom_ni = MULTIPLIED_NI((int)bottom_ni_factor.noise_index);
	int top_ni = MULTIPLIED_NI((int)top_ni_factor.noise_index);
	int diff_ni_top_to_bottom = top_ni - bottom_ni;
	int diff_ni_top_to_actual = top_ni - noise_index;
	int diff_ni_actual_to_bottom = noise_index - bottom_ni;

	interpolated_factor->temporal_motion_detection_luma_low =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_motion_detection_luma_low,
			top_ni_factor.temporal_motion_detection_luma_low,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_motion_detection_luma_contrast =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_motion_detection_luma_contrast,
			top_ni_factor.temporal_motion_detection_luma_contrast,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_motion_detection_luma_high =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_motion_detection_luma_high,
			top_ni_factor.temporal_motion_detection_luma_high,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_motion_detection_chroma_low =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_motion_detection_chroma_low,
			top_ni_factor.temporal_motion_detection_chroma_low,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_motion_detection_chroma_contrast =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_motion_detection_chroma_contrast,
			top_ni_factor.temporal_motion_detection_chroma_contrast,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_motion_detection_chroma_high =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_motion_detection_chroma_high,
			top_ni_factor.temporal_motion_detection_chroma_high,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_weight_luma_power_base =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_weight_luma_power_base,
			top_ni_factor.temporal_weight_luma_power_base,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_weight_luma_power_gamma =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_weight_luma_power_gamma,
			top_ni_factor.temporal_weight_luma_power_gamma,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_weight_chroma_power_base =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_weight_chroma_power_base,
			top_ni_factor.temporal_weight_chroma_power_base,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_weight_chroma_power_gamma =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_weight_chroma_power_gamma,
			top_ni_factor.temporal_weight_chroma_power_gamma,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	/* select value by the actual NI and ref NI range
	 * 1. Bottom NI -- Actual NI -------Top NI -> use Bottom NI factor
	 * 2. Bottom NI ------- Actual NI -- Top NI -> use Top NI factor
	 */

	if (diff_ni_top_to_actual > diff_ni_actual_to_bottom) {
		interpolated_factor->temporal_motion_detection_luma_off =
			top_ni_factor.temporal_motion_detection_luma_off;
		interpolated_factor->temporal_motion_detection_chroma_off =
			top_ni_factor.temporal_motion_detection_chroma_off;
	} else {
		interpolated_factor->temporal_motion_detection_luma_off =
			bottom_ni_factor.temporal_motion_detection_luma_off;
		interpolated_factor->temporal_motion_detection_chroma_off =
			bottom_ni_factor.temporal_motion_detection_chroma_off;
	}
}

static void interpolate_regional_factor(struct ni_dep_factors *interpolated_factor,
	u32 noise_index,
	struct ni_dep_factors bottom_ni_factor,
	struct ni_dep_factors top_ni_factor)
{
	int bottom_ni = MULTIPLIED_NI((int)bottom_ni_factor.noise_index);
	int top_ni = MULTIPLIED_NI((int)top_ni_factor.noise_index);
	int diff_ni_top_to_bottom = top_ni - bottom_ni;
	int diff_ni_top_to_actual = top_ni - noise_index;
	int diff_ni_actual_to_bottom = noise_index - bottom_ni;

	interpolated_factor->temporal_weight_hot_region_power =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_weight_hot_region_power,
			top_ni_factor.temporal_weight_hot_region_power,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->temporal_weight_chroma_power =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.temporal_weight_chroma_power,
			top_ni_factor.temporal_weight_chroma_power,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	/* select value by the actual NI and ref NI range
	 * 1. Bottom NI -- Actual NI -------Top NI -> use Bottom NI factor
	 * 2. Bottom NI ------- Actual NI -- Top NI -> use Top NI factor
	 */

	if (diff_ni_top_to_actual > diff_ni_actual_to_bottom) {
		interpolated_factor->temporal_weight_hot_region =
			top_ni_factor.temporal_weight_hot_region;
		interpolated_factor->temporal_weight_chroma_threshold =
			top_ni_factor.temporal_weight_chroma_threshold;
	} else {
		interpolated_factor->temporal_weight_hot_region =
			bottom_ni_factor.temporal_weight_hot_region;
		interpolated_factor->temporal_weight_chroma_threshold =
			bottom_ni_factor.temporal_weight_chroma_threshold;
	}
}

static void interpolate_spatial_factor(struct ni_dep_factors *interpolated_factor,
	u32 noise_index,
	struct ni_dep_factors bottom_ni_factor,
	struct ni_dep_factors top_ni_factor)
{
	int bottom_ni = MULTIPLIED_NI((int)bottom_ni_factor.noise_index);
	int top_ni = MULTIPLIED_NI((int)top_ni_factor.noise_index);
	int diff_ni_top_to_bottom = top_ni - bottom_ni;
	int diff_ni_top_to_actual = top_ni - noise_index;
	int diff_ni_actual_to_bottom = noise_index - bottom_ni;

	interpolated_factor->spatial_power =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.spatial_power,
			top_ni_factor.spatial_power,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->spatial_pd_luma_slope =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.spatial_pd_luma_slope,
			top_ni_factor.spatial_pd_luma_slope,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->spatial_pd_luma_offset =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.spatial_pd_luma_offset,
			top_ni_factor.spatial_pd_luma_offset,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->spatial_pd_chroma_slope =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.spatial_pd_chroma_slope,
			top_ni_factor.spatial_pd_chroma_slope,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->spatial_pd_chroma_offset =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.spatial_pd_chroma_offset,
			top_ni_factor.spatial_pd_chroma_offset,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	/* select value by the actual NI and ref NI range
	 * 1. Bottom NI -- Actual NI -------Top NI -> use Bottom NI factor
	 * 2. Bottom NI ------- Actual NI -- Top NI -> use Top NI factor
	 */

	if (diff_ni_top_to_actual > diff_ni_actual_to_bottom) {
		interpolated_factor->spatial_weight_mode =
			top_ni_factor.spatial_weight_mode;
		interpolated_factor->spatial_separate_weighting =
			top_ni_factor.spatial_separate_weighting;
	} else {
		interpolated_factor->spatial_weight_mode =
			bottom_ni_factor.spatial_weight_mode;
		interpolated_factor->spatial_separate_weighting =
			bottom_ni_factor.spatial_separate_weighting;
	}
}

static void interpolate_yuv_table_factor(struct ni_dep_factors *interpolated_factor,
	u32 noise_index,
	struct ni_dep_factors bottom_ni_factor,
	struct ni_dep_factors top_ni_factor)
{
	int arr_idx;
	int bottom_ni = MULTIPLIED_NI((int)bottom_ni_factor.noise_index);
	int top_ni = MULTIPLIED_NI((int)top_ni_factor.noise_index);
	int diff_ni_top_to_bottom = top_ni - bottom_ni;
	int diff_ni_actual_to_bottom = noise_index - bottom_ni;

	for (arr_idx = ARR3_VAL1; arr_idx < ARR3_MAX; arr_idx++) {
		interpolated_factor->yuv_tables.x_grid_y[arr_idx] =
			GET_LINEAR_INTERPOLATE_VALUE(
				bottom_ni_factor.yuv_tables.x_grid_y[arr_idx],
				top_ni_factor.yuv_tables.x_grid_y[arr_idx],
				diff_ni_top_to_bottom,
				diff_ni_actual_to_bottom);

		interpolated_factor->yuv_tables.x_grid_u[arr_idx] =
			GET_LINEAR_INTERPOLATE_VALUE(
				bottom_ni_factor.yuv_tables.x_grid_u[arr_idx],
				top_ni_factor.yuv_tables.x_grid_u[arr_idx],
				diff_ni_top_to_bottom,
				diff_ni_actual_to_bottom);

		interpolated_factor->yuv_tables.x_grid_v[arr_idx] =
			GET_LINEAR_INTERPOLATE_VALUE(
				bottom_ni_factor.yuv_tables.x_grid_v[arr_idx],
				top_ni_factor.yuv_tables.x_grid_v[arr_idx],
				diff_ni_top_to_bottom,
				diff_ni_actual_to_bottom);
	}

	for (arr_idx = ARR4_VAL1; arr_idx < ARR4_MAX; arr_idx++) {
		interpolated_factor->yuv_tables.y_std_slope[arr_idx] =
			GET_LINEAR_INTERPOLATE_VALUE(
				bottom_ni_factor.yuv_tables.y_std_slope[arr_idx],
				top_ni_factor.yuv_tables.y_std_slope[arr_idx],
				diff_ni_top_to_bottom,
				diff_ni_actual_to_bottom);

		interpolated_factor->yuv_tables.u_std_slope[arr_idx] =
			GET_LINEAR_INTERPOLATE_VALUE(
				bottom_ni_factor.yuv_tables.u_std_slope[arr_idx],
				top_ni_factor.yuv_tables.u_std_slope[arr_idx],
				diff_ni_top_to_bottom,
				diff_ni_actual_to_bottom);

		interpolated_factor->yuv_tables.v_std_slope[arr_idx] =
			GET_LINEAR_INTERPOLATE_VALUE(
				bottom_ni_factor.yuv_tables.v_std_slope[arr_idx],
				top_ni_factor.yuv_tables.v_std_slope[arr_idx],
				diff_ni_top_to_bottom,
				diff_ni_actual_to_bottom);
	}

	interpolated_factor->yuv_tables.y_std_offset =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.yuv_tables.y_std_offset,
			top_ni_factor.yuv_tables.y_std_offset,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->yuv_tables.u_std_offset =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.yuv_tables.u_std_offset,
			top_ni_factor.yuv_tables.u_std_offset,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);

	interpolated_factor->yuv_tables.v_std_offset =
		GET_LINEAR_INTERPOLATE_VALUE(
			bottom_ni_factor.yuv_tables.v_std_offset,
			top_ni_factor.yuv_tables.v_std_offset,
			diff_ni_top_to_bottom,
			diff_ni_actual_to_bottom);
}

static void reconfigure_ni_depended_tuneset(struct tdnr_setfile_contents *tdnr_tuneset,
	struct tdnr_configs *tdnr_cfgs,
	u32 noise_index,
	u32 bottom_ni_index,
	u32 top_ni_index)
{
	struct ni_dep_factors interpolated_factor;

	/* help variables for interpolation */
	struct ni_dep_factors bottom_ni_factor =
		tdnr_tuneset->ni_dep_factors[bottom_ni_index];
	struct ni_dep_factors top_ni_factor =
		tdnr_tuneset->ni_dep_factors[top_ni_index];

	interpolate_temporal_factor(&interpolated_factor,
			noise_index,
			bottom_ni_factor,
			top_ni_factor);

	interpolate_regional_factor(&interpolated_factor,
			noise_index,
			bottom_ni_factor,
			top_ni_factor);

	interpolate_spatial_factor(&interpolated_factor,
			noise_index,
			bottom_ni_factor,
			top_ni_factor);

	interpolate_yuv_table_factor(&interpolated_factor,
			noise_index,
			bottom_ni_factor,
			top_ni_factor);

	/* re_configure interpolated NI depended factor to SFR configurations */
	translate_temporal_factor(&tdnr_cfgs->temporal_dep_cfg, interpolated_factor);
	translate_regional_factor(&tdnr_cfgs->regional_dep_cfg, interpolated_factor);
	translate_spatial_factor(&tdnr_cfgs->spatial_dep_cfg, interpolated_factor);
	translate_yuv_table_factor(&tdnr_cfgs->yuv_tables, interpolated_factor);
}

static int fimc_is_hw_mcsc_cfg_tdnr_tuning_param(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_group *head,
	struct fimc_is_frame *frame,
	enum tdnr_mode *tdnr_mode,
	bool start_flag)
{
	int ret = 0;
	int ni_idx, arr_idx;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct tdnr_setfile_contents *tdnr_tuneset;
	struct tdnr_configs tdnr_cfgs;
	u32 max_ref_ni = 0, min_ref_ni = 0;
	u32 bottom_ni_index = 0, top_ni_index = 0;
	u32 ref_bottom_ni = 0, ref_top_ni = 0;
	u32 noise_index = 0;
	bool use_tdnr_tuning = false;
#ifdef MCSC_DNR_USE_TUNING
	enum exynos_sensor_position sensor_position;
	u32 instance;
#endif

	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	if (frame->type == SHOT_TYPE_INTERNAL) {
		warn_hw("wrong TDNR setting at internal shot");
		return -EINVAL;
	}

#ifdef MCSC_DNR_USE_TUNING
	instance = atomic_read(&hw_ip->instance);
	sensor_position = hw_ip->hardware->sensor_position[instance];
	tdnr_tuneset = &hw_mcsc->cur_setfile[sensor_position][frame->instance]->tdnr_contents;
	use_tdnr_tuning = true;
#endif

	if (!use_tdnr_tuning)
		goto config;

	/* copy NI independed settings */
	tdnr_cfgs.general_cfg = tdnr_tuneset->general_cfg;
	tdnr_cfgs.temporal_indep_cfg = tdnr_tuneset->temporal_indep_cfg;
	for (arr_idx = ARR3_VAL1; arr_idx < ARR3_MAX; arr_idx++)
		tdnr_cfgs.constant_lut_coeffs[arr_idx] = tdnr_tuneset->constant_lut_coeffs[arr_idx];
	tdnr_cfgs.refine_cfg = tdnr_tuneset->refine_cfg;
	tdnr_cfgs.regional_indep_cfg = tdnr_tuneset->regional_indep_cfg;
	tdnr_cfgs.spatial_indep_cfg = tdnr_tuneset->spatial_indep_cfg;

#ifdef FIXED_TDNR_NOISE_INDEX
	noise_index = FIXED_TDNR_NOISE_INDEX_VALUE;
#else
	noise_index = frame->noise_idx; /* get applying NI from frame */
#endif

	if (!start_flag && hw_mcsc->cur_ni[SUBBLK_TDNR] == noise_index)
		goto exit;

	/* find ref NI arry index for re-configure NI depended settings */
	max_ref_ni =
		MULTIPLIED_NI(tdnr_tuneset->ni_dep_factors[tdnr_tuneset->num_of_noiseindexes - 1].noise_index);
	min_ref_ni =
		MULTIPLIED_NI(tdnr_tuneset->ni_dep_factors[0].noise_index);

	if (noise_index >= max_ref_ni) {
		dbg_hw(2, "current NI (%d) > Max ref NI(%d), set bypass mode", noise_index, max_ref_ni);
		*tdnr_mode = TDNR_MODE_BYPASS;
		goto exit;
	} else if (noise_index <= min_ref_ni) {
		dbg_hw(2, "current NI (%d) < Min ref NI(%d), use first NI value", noise_index, min_ref_ni);
		bottom_ni_index = 0;
		top_ni_index = 0;
	} else {
		for (ni_idx = 0; ni_idx < tdnr_tuneset->num_of_noiseindexes - 1; ni_idx++) {
			ref_bottom_ni = MULTIPLIED_NI(tdnr_tuneset->ni_dep_factors[ni_idx].noise_index);
			ref_top_ni = MULTIPLIED_NI(tdnr_tuneset->ni_dep_factors[ni_idx + 1].noise_index);

			if (noise_index > ref_bottom_ni && noise_index < ref_top_ni) {
				bottom_ni_index = ni_idx;
				top_ni_index = ni_idx + 1;
				break;
			} else if (noise_index == ref_bottom_ni) {
				bottom_ni_index = ni_idx;
				top_ni_index = ni_idx;
				break;
			} else if (noise_index == ref_top_ni) {
				bottom_ni_index = ni_idx + 1;
				top_ni_index = ni_idx + 1;
				break;
			}
		}
	}

	/* interpolate & reconfigure by reference NI */
	reconfigure_ni_depended_tuneset(tdnr_tuneset, &tdnr_cfgs,
				noise_index, bottom_ni_index, top_ni_index);


config:
	/* update tuning SFR (NI independed cfg) */
	if (start_flag) {
		fimc_is_scaler_set_tdnr_tuneset_general(hw_ip->regs,
			tdnr_cfgs.general_cfg);
		fimc_is_scaler_set_tdnr_tuneset_constant_lut_coeffs(hw_ip->regs,
			tdnr_cfgs.constant_lut_coeffs);
		fimc_is_scaler_set_tdnr_tuneset_refine_control(hw_ip->regs,
			tdnr_cfgs.refine_cfg);
	}

	/* update tuning SFR (include NI depended cfg) */
	fimc_is_scaler_set_tdnr_tuneset_yuvtable(hw_ip->regs, tdnr_cfgs.yuv_tables);
	fimc_is_scaler_set_tdnr_tuneset_temporal(hw_ip->regs,
		tdnr_cfgs.temporal_dep_cfg, tdnr_cfgs.temporal_indep_cfg);
	fimc_is_scaler_set_tdnr_tuneset_regional_feature(hw_ip->regs,
		tdnr_cfgs.regional_dep_cfg, tdnr_cfgs.regional_indep_cfg);
	fimc_is_scaler_set_tdnr_tuneset_spatial(hw_ip->regs,
		tdnr_cfgs.spatial_dep_cfg, tdnr_cfgs.spatial_indep_cfg);

exit:
	hw_mcsc->cur_ni[SUBBLK_TDNR] = noise_index;

	return ret;
}

int fimc_is_hw_mcsc_update_tdnr_register(struct fimc_is_hw_ip *hw_ip,
	struct fimc_is_frame *frame,
	struct is_param_region *param,
	bool start_flag)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct tpu_param *tpu_param;
	struct mcs_param *mcs_param;
	struct fimc_is_group *head;
	struct fimc_is_hw_mcsc_cap *cap;
	enum tdnr_mode tdnr_mode;

	BUG_ON(!hw_ip);
	BUG_ON(!hw_ip->priv_info);
	BUG_ON(!param);
	BUG_ON(!frame);

	tpu_param = &param->tpu;
	mcs_param = &param->mcs;
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;
	cap = GET_MCSC_HW_CAP(hw_ip);

	if (cap->tdnr != MCSC_CAP_SUPPORT)
		return ret;

	head = hw_ip->group[frame->instance]->head;

	tdnr_mode = fimc_is_hw_mcsc_check_tdnr_mode_pre(hw_ip, head,
			frame, tpu_param, mcs_param, hw_mcsc->cur_tdnr_mode);

	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &head->state))
		hw_mcsc->yic_en = TDNR_YIC_DISABLE;
	else
		hw_mcsc->yic_en = TDNR_YIC_ENABLE;
	fimc_is_scaler_set_tdnr_yic_ctrl(hw_ip->regs, hw_mcsc->yic_en);

	fimc_is_scaler_set_tdnr_image_size(hw_ip->regs,
			mcs_param->input.width, mcs_param->input.height);

	switch (tdnr_mode) {
	case TDNR_MODE_BYPASS:
		goto tdnr_mode_select;
	case TDNR_MODE_2DNR:
	case TDNR_MODE_3DNR:
		ret = fimc_is_hw_mcsc_cfg_tdnr_rdma(hw_ip, head, frame, mcs_param, tdnr_mode);
		if (ret) {
			err_hw("[ID:%d] tdnr rdma cfg failed", hw_ip->id);
			break;
		}

		ret = fimc_is_hw_mcsc_cfg_tdnr_wdma(hw_ip, head, frame, mcs_param, tdnr_mode);
		if (ret) {
			err_hw("[ID:%d] tdnr wdma cfg failed", hw_ip->id);
			break;
		}

		ret = fimc_is_hw_mcsc_cfg_tdnr_tuning_param(hw_ip, head, frame, &tdnr_mode, start_flag);
		if (ret) {
			err_hw("[ID:%d] tdnr tuning param setting failed", hw_ip->id);
			break;
		}
		break;
	default:
		break;
	}

	if (ret) {
		tdnr_mode = TDNR_MODE_BYPASS;
		err_hw("[ID:%d] set tdnr_mode(BYPASS)", hw_ip->id);
	}

	fimc_is_hw_mcsc_check_tdnr_mode_post(hw_ip, head,
			hw_mcsc->cur_tdnr_mode, &tdnr_mode);

tdnr_mode_select:
	fimc_is_scaler_set_tdnr_mode_select(hw_ip->regs, tdnr_mode);
	hw_mcsc->cur_tdnr_mode = tdnr_mode;

	return 0;
}

int fimc_is_hw_mcsc_recovery_tdnr_register(struct fimc_is_hw_ip *hw_ip,
		struct is_param_region *param, u32 instance)
{
	int ret = 0;
	struct fimc_is_hw_mcsc *hw_mcsc = NULL;
	struct mcs_param *mcs_param;
	enum tdnr_mode tdnr_mode;

	BUG_ON(!hw_ip);
	BUG_ON(!param);

	tdnr_mode = TDNR_MODE_BYPASS;
	mcs_param = &param->mcs;
	hw_mcsc = (struct fimc_is_hw_mcsc *)hw_ip->priv_info;

	fimc_is_hw_mcsc_tdnr_init(hw_ip, mcs_param, instance);

	fimc_is_scaler_set_tdnr_image_size(hw_ip->regs,
			mcs_param->output[hw_mcsc->tdnr_output].width,
			mcs_param->output[hw_mcsc->tdnr_output].height);

	fimc_is_scaler_set_tdnr_mode_select(hw_ip->regs, tdnr_mode);

	return ret;
}
