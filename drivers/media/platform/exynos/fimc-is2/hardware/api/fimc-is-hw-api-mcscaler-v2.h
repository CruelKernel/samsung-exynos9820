/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_API_MCSCALER_V2_H
#define FIMC_IS_HW_API_MCSCALER_V2_H

#include "fimc-is-hw-api-common.h"
#include "../fimc-is-hw-mcscaler-v2.h"

#define RATIO_X8_8	1048576
#define RATIO_X7_8	1198373
#define RATIO_X6_8	1398101
#define RATIO_X5_8	1677722
#define RATIO_X4_8	2097152
#define RATIO_X3_8	2796203
#define RATIO_X2_8	4194304

enum mcsc_wdma_priority {
	MCSC_WDMA_OUTPUT0_Y = 0,
	MCSC_WDMA_OUTPUT0_U = 1,
	MCSC_WDMA_OUTPUT0_V = 2,
	MCSC_WDMA_OUTPUT1_Y = 3,
	MCSC_WDMA_OUTPUT1_U = 4,
	MCSC_WDMA_OUTPUT1_V = 5,
	MCSC_WDMA_OUTPUT2_Y = 6,
	MCSC_WDMA_OUTPUT2_U = 7,
	MCSC_WDMA_OUTPUT2_V = 8,
	MSCC_WDMA_PRI_A_MAX,

	MCSC_WDMA_OUTPUT3_Y = 0,
	MCSC_WDMA_OUTPUT3_U = 1,
	MCSC_WDMA_OUTPUT3_V = 2,
	MCSC_WDMA_OUTPUT4_Y = 3,
	MCSC_WDMA_OUTPUT4_U = 4,
	MCSC_WDMA_OUTPUT4_V = 5,
	MCSC_WDMA_OUTPUT5_Y = 6,	/* SSB */
	MSCC_WDMA_PRI_B_MAX
};

enum mcsc_filter_coeff {
	MCSC_COEFF_x8_8 = 0,	/* A (8/8 ~ ) */
	MCSC_COEFF_x7_8 = 1,	/* B (7/8 ~ ) */
	MCSC_COEFF_x6_8 = 2,	/* C (6/8 ~ ) */
	MCSC_COEFF_x5_8 = 3,	/* D (5/8 ~ ) */
	MCSC_COEFF_x4_8 = 4,	/* E (4/8 ~ ) */
	MCSC_COEFF_x3_8 = 5,	/* F (3/8 ~ ) */
	MCSC_COEFF_x2_8 = 6,	/* G (2/8 ~ ) */
	MCSC_COEFF_MAX
};

struct mcsc_v_coef {
	int v_coef_a[9];
	int v_coef_b[9];
	int v_coef_c[9];
	int v_coef_d[9];
};

struct mcsc_h_coef {
	int h_coef_a[9];
	int h_coef_b[9];
	int h_coef_c[9];
	int h_coef_d[9];
	int h_coef_e[9];
	int h_coef_f[9];
	int h_coef_g[9];
	int h_coef_h[9];
};

void fimc_is_scaler_start(void __iomem *base_addr, u32 hw_id);
void fimc_is_scaler_stop(void __iomem *base_addr, u32 hw_id);
void fimc_is_scaler_rdma_start(void __iomem *base_addr, u32 hw_id);

u32 fimc_is_scaler_sw_reset(void __iomem *base_addr, u32 hw_id, u32 global, u32 partial);

void fimc_is_scaler_clear_intr_all(void __iomem *base_addr, u32 hw_id);
void fimc_is_scaler_disable_intr(void __iomem *base_addr, u32 hw_id);
void fimc_is_scaler_mask_intr(void __iomem *base_addr, u32 hw_id, u32 intr_mask);

void fimc_is_scaler_set_shadow_ctrl(void __iomem *base_addr, u32 hw_id, enum mcsc_shadow_ctrl ctrl);
void fimc_is_scaler_clear_shadow_ctrl(void __iomem *base_addr, u32 hw_id);

void fimc_is_scaler_set_stop_req_post_en_ctrl(void __iomem *base_addr, u32 hw_id, u32 value);

void fimc_is_scaler_get_input_status(void __iomem *base_addr, u32 hw_id, u32 *hl, u32 *vl);
void fimc_is_scaler_set_input_source(void __iomem *base_addr, u32 hw_id, u32 rdma);
u32 fimc_is_scaler_get_input_source(void __iomem *base_addr, u32 hw_id);
void fimc_is_scaler_set_dither(void __iomem *base_addr, u32 hw_id, bool dither_en);
void fimc_is_scaler_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height);
void fimc_is_scaler_get_input_img_size(void __iomem *base_addr, u32 hw_id, u32 *width, u32 *height);

u32 fimc_is_scaler_get_scaler_path(void __iomem *base_addr, u32 hw_id, u32 output_id);
void fimc_is_scaler_set_poly_scaler_enable(void __iomem *base_addr, u32 hw_id, u32 output_id, u32 enable);
void fimc_is_scaler_set_poly_scaler_bypass(void __iomem *base_addr, u32 output_id, u32 bypass);
void fimc_is_scaler_set_poly_src_size(void __iomem *base_addr, u32 output_id, u32 pos_x, u32 pos_y, u32 width, u32 height);
void fimc_is_scaler_get_poly_src_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height);
void fimc_is_scaler_set_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height);
void fimc_is_scaler_get_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height);
void fimc_is_scaler_set_poly_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio);
void fimc_is_scaler_set_h_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 h_offset);
void fimc_is_scaler_set_v_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 v_offset);
void fimc_is_scaler_set_poly_scaler_coef(void __iomem *base_addr, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coef,
	enum exynos_sensor_position sensor_position);
void fimc_is_scaler_set_poly_round_mode(void __iomem *base_addr, u32 output_id, u32 mode);

void fimc_is_scaler_set_post_scaler_enable(void __iomem *base_addr, u32 output_id, u32 enable);
void fimc_is_scaler_set_post_img_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height);
void fimc_is_scaler_get_post_img_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height);
void fimc_is_scaler_set_post_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height);
void fimc_is_scaler_get_post_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height);
void fimc_is_scaler_set_post_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio);
void fimc_is_scaler_set_post_scaler_coef(void __iomem *base_addr, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coef);
void fimc_is_scaler_set_post_round_mode(void __iomem *base_addr, u32 output_id, u32 mode);

void fimc_is_scaler_set_420_conversion(void __iomem *base_addr, u32 output_id, u32 conv420_weight, u32 conv420_en);
void fimc_is_scaler_set_bchs_enable(void __iomem *base_addr, u32 output_id, bool bchs_en);
void fimc_is_scaler_set_b_c(void __iomem *base_addr, u32 output_id, u32 y_offset, u32 y_gain);
void fimc_is_scaler_set_h_s(void __iomem *base_addr, u32 output_id, u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11);
void fimc_is_scaler_set_bchs_clamp(void __iomem *base_addr, u32 output_id, u32 y_max, u32 y_min, u32 c_max, u32 c_min);

void fimc_is_scaler_set_wdma_pri(void __iomem *base_addr, u32 output_id, u32 plane);
void fimc_is_scaler_set_wdma_axi_pri(void __iomem *base_addr);
void fimc_is_scaler_set_wdma_sram_base(void __iomem *base_addr, u32 output_id);
void fimc_is_scaler_set_dma_out_enable(void __iomem *base_addr, u32 output_id, bool dma_out_en);
void fimc_is_scaler_set_otf_out_enable(void __iomem *base_addr, u32 output_id, bool otf_out_en);
u32 fimc_is_scaler_get_dma_out_enable(void __iomem *base_addr, u32 output_id);
u32 fimc_is_scaler_get_otf_out_enable(void __iomem *base_addr, u32 output_id);
void fimc_is_scaler_set_otf_out_path(void __iomem *base_addr, u32 output_id);

void fimc_is_scaler_set_rdma_format(void __iomem *base_addr, u32 hw_id, u32 dma_in_format);
void fimc_is_scaler_set_wdma_format(void __iomem *base_addr, u32 hw_id, u32 output_id, u32 dma_out_format);
void fimc_is_scaler_get_wdma_format(void __iomem *base_addr, u32 output_id, u32 *dma_out_format);
void fimc_is_scaler_set_swap_mode(void __iomem *base_addr, u32 output_id, u32 swap);
void fimc_is_scaler_set_flip_mode(void __iomem *base_addr, u32 output_id, u32 flip);
void fimc_is_scaler_set_rdma_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_get_rdma_size(void __iomem *base_addr, u32 *width, u32 *height);
void fimc_is_scaler_set_wdma_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height);
void fimc_is_scaler_get_wdma_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height);
void fimc_is_scaler_set_rdma_stride(void __iomem *base_addr, u32 y_stride, u32 uv_stride);
void fimc_is_scaler_get_rdma_stride(void __iomem *base_addr, u32 *y_stride, u32 *uv_stride);
void fimc_is_scaler_set_wdma_stride(void __iomem *base_addr, u32 output_id, u32 y_stride, u32 uv_stride);
void fimc_is_scaler_get_wdma_stride(void __iomem *base_addr, u32 output_id, u32 *y_stride, u32 *uv_stride);
void fimc_is_scaler_set_rdma_frame_seq(void __iomem *base_addr, u32 frame_seq);
void fimc_is_scaler_set_wdma_frame_seq(void __iomem *base_addr, u32 output_id, u32 frame_seq);
void fimc_is_scaler_set_rdma_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index);
void fimc_is_scaler_set_wdma_addr(void __iomem *base_addr, u32 output_id, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index);
void fimc_is_scaler_get_wdma_addr(void __iomem *base_addr, u32 output_id, u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index);
void fimc_is_scaler_clear_rdma_addr(void __iomem *base_addr);
void fimc_is_scaler_clear_wdma_addr(void __iomem *base_addr, u32 output_id);

void fimc_is_scaler_set_rdma_2bit_addr(void __iomem *base_addr, u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index);
void fimc_is_scaler_set_wdma_2bit_addr(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index);
void fimc_is_scaler_set_rdma_2bit_stride(void __iomem *base_addr, u32 y_2bit_stride, u32 uv_2bit_stride);
void fimc_is_scaler_set_wdma_2bit_stride(void __iomem *base_addr, u32 output_id, u32 y_2bit_stride, u32 uv_2bit_stride);
void fimc_is_scaler_set_rdma_10bit_type(void __iomem *base_addr, u32 dma_in_10bit_type);
void fimc_is_scaler_set_wdma_10bit_type(void __iomem *base_addr, u32 output_id, u32 format, u32 bitwidth, enum exynos_sensor_position sensor_position);

/* for hwfc */
void fimc_is_scaler_set_hwfc_auto_clear(void __iomem *base_addr, u32 output_id, bool auto_clear);
void fimc_is_scaler_set_hwfc_idx_reset(void __iomem *base_addr, u32 output_id, bool reset);
void fimc_is_scaler_set_hwfc_mode(void __iomem *base_addr, u32 hwfc_output_ids);
void fimc_is_scaler_set_hwfc_config(void __iomem *base_addr,
		u32 output_id, u32 format, u32 plane, u32 dma_idx, u32 width, u32 height);
u32 fimc_is_scaler_get_hwfc_idx_bin(void __iomem *base_addr, u32 output_id);
u32 fimc_is_scaler_get_hwfc_cur_idx(void __iomem *base_addr, u32 output_id);

/* for tdnr */
void fimc_is_scaler_set_tdnr_rdma_addr(void __iomem *base_addr,
		enum tdnr_buf_type type, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index);
void fimc_is_scaler_clear_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type);
void fimc_is_scaler_set_tdnr_wdma_addr(void __iomem *base_addr,
		enum tdnr_buf_type type, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index);
void fimc_is_scaler_get_tdnr_wdma_addr(void __iomem *base_addr,
		enum tdnr_buf_type type, u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index);
void fimc_is_scaler_clear_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type);
void fimc_is_scaler_set_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height);
void fimc_is_scaler_get_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 *width, u32 *height);
void fimc_is_scaler_set_tdnr_rdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height);
void fimc_is_scaler_set_tdnr_rdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_in_format);
void fimc_is_scaler_set_tdnr_wdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_out_format);
void fimc_is_scaler_set_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_stride, u32 uv_stride);
void fimc_is_scaler_get_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_stride, u32 *uv_stride);
void fimc_is_scaler_set_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_stride, u32 uv_stride);
void fimc_is_scaler_get_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_stride, u32 *uv_stride);
void fimc_is_scaler_set_tdnr_wdma_sram_base(void __iomem *base_addr, enum tdnr_buf_type type);
void fimc_is_scaler_set_tdnr_wdma_enable(void __iomem *base_addr, enum tdnr_buf_type type, bool dma_out_en);
void fimc_is_scaler_get_tdnr_image_size(void __iomem *base_addr, u32 *width, u32 *height);
void fimc_is_scaler_set_tdnr_image_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_set_tdnr_yic_ctrl(void __iomem *base_addr, enum yic_mode yic_enable);
void fimc_is_scaler_set_tdnr_mode_select(void __iomem *base_addr, enum tdnr_mode mode);
void fimc_is_scaler_set_tdnr_rdma_start(void __iomem *base_addr, enum tdnr_mode mode);
void fimc_is_scaler_set_tdnr_first(void __iomem *base_addr, u32 tdnr_first);

void fimc_is_scaler_set_tdnr_tuneset_general(void __iomem *base_addr, struct general_config config);
void fimc_is_scaler_set_tdnr_tuneset_yuvtable(void __iomem *base_addr, struct yuv_table_config config);
void fimc_is_scaler_set_tdnr_tuneset_temporal(void __iomem *base_addr,
	struct temporal_ni_dep_config dep_config,
	struct temporal_ni_indep_config indep_config);
void fimc_is_scaler_set_tdnr_tuneset_constant_lut_coeffs(void __iomem *base_addr, u32 *config);
void fimc_is_scaler_set_tdnr_tuneset_refine_control(void __iomem *base_addr,
	struct refine_control_config config);
void fimc_is_scaler_set_tdnr_tuneset_regional_feature(void __iomem *base_addr,
	struct regional_ni_dep_config dep_config,
	struct regional_ni_indep_config indep_config);
void fimc_is_scaler_set_tdnr_tuneset_spatial(void __iomem *base_addr,
	struct spatial_ni_dep_config dep_config,
	struct spatial_ni_indep_config indep_config);

/* djag */
void fimc_is_scaler_set_djag_enable(void __iomem *base_addr, u32 djag_enable);
void fimc_is_scaler_set_djag_input_source(void __iomem *base_addr, u32 djag_input_sel);
void fimc_is_scaler_set_djag_src_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_set_djag_dst_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_set_djag_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio);
void fimc_is_scaler_set_djag_init_phase_offset(void __iomem *base_addr, u32 h_offset, u32 v_offset);
void fimc_is_scaler_set_djag_round_mode(void __iomem *base_addr, u32 round_enable);
void fimc_is_scaler_set_djag_tunning_param(void __iomem *base_addr, const struct djag_setfile_contents *djag_tune);
void fimc_is_scaler_set_djag_dither_wb(void __iomem *base_addr, struct djag_wb_thres_cfg *djag_wb, u32 wht, u32 blk);

/* cac */
void fimc_is_scaler_set_cac_enable(void __iomem *base_addr, u32 en);
void fimc_is_scaler_set_cac_input_source(void __iomem *base_addr, u32 in);
void fimc_is_scaler_set_cac_map_crt_thr(void __iomem *base_addr, struct cac_cfg_by_ni *cfg);

/* uvsp */
void fimc_is_scaler_set_uvsp_enable(void __iomem *base_addr, u32 hw_id, u32 en);
void fimc_is_scaler_set_uvsp_radial_ctrl(void __iomem *base_addr, u32 hw_id, struct uvsp_ctrl *cfg);
void fimc_is_scaler_set_uvsp_radial_cfg(void __iomem *base_addr, u32 hw_id, struct uvsp_radial_cfg *cfg);
void fimc_is_scaler_set_uvsp_pedestal_cfg(void __iomem *base_addr, u32 hw_id, struct uvsp_pedestal_cfg *cfg);
void fimc_is_scaler_set_uvsp_offset_cfg(void __iomem *base_addr, u32 hw_id, struct uvsp_offset_cfg *cfg);
void fimc_is_scaler_set_uvsp_desat_cfg(void __iomem *base_addr, u32 hw_id, struct uvsp_desat_cfg *cfg);
void fimc_is_scaler_set_uvsp_r2y_coef_cfg(void __iomem *base_addr, u32 hw_id, struct uvsp_r2y_coef_cfg *cfg);

/* ysum */
void fimc_is_scaler_set_ysum_input_sourece_enable(void __iomem *base_addr, u32 output_id, bool ysum_enable);
void fimc_is_scaler_set_ysum_enable(void __iomem *base_addr, bool ysum_enable);
void fimc_is_scaler_set_ysum_image_size(void __iomem *base_addr, u32 width, u32 height, u32 start_x, u32 start_y);
void fimc_is_scaler_get_ysum_result(void __iomem *base_addr, u32 *luma_sum_msb, u32 *luma_sum_lsb);

/* DS */
void fimc_is_scaler_set_ds_enable(void __iomem *base_addr, u32 ds_enable);
void fimc_is_scaler_set_ds_img_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_set_ds_src_size(void __iomem *base_addr, u32 width, u32 height, u32 x_pos, u32 y_pos);
void fimc_is_scaler_set_ds_dst_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_set_ds_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio);
void fimc_is_scaler_set_ds_init_phase_offset(void __iomem *base_addr, u32 h_offset, u32 v_offset);
void fimc_is_scaler_set_ds_gamma_table_enable(void __iomem *base_addr, u32 ds_gamma_enable);

/* FRO */
void fimc_is_scaler_set_lfro_mode_enable(void __iomem *base_addr, u32 hw_id, u32 lfro_enable, u32 lfro_total_fnum);
u32 fimc_is_scaler_get_lfro_mode_status(void __iomem *base_addr, u32 hw_id);

void fimc_is_scaler_clear_intr_src(void __iomem *base_addr, u32 hw_id, u32 status);
u32 fimc_is_scaler_get_intr_mask(void __iomem *base_addr, u32 hw_id);
u32 fimc_is_scaler_get_intr_status(void __iomem *base_addr, u32 hw_id);

u32 fimc_is_scaler_handle_extended_intr(u32 status);

u32 fimc_is_scaler_get_version(void __iomem *base_addr);
void fimc_is_scaler_dump(void __iomem *base_addr);
#endif
