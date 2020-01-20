/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-api-mcscaler-v2.h"
#include "sfr/fimc-is-sfr-mcsc-v122_123.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-control.h"
#include "fimc-is-param.h"

void fimc_is_scaler_start(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_GCTRL_0], &mcsc_fields[MCSC_F_SCALER_ENABLE_0], 1);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_stop(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_GCTRL_0], &mcsc_fields[MCSC_F_SCALER_ENABLE_0], 0);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

static u32 fimc_is_scaler_sw_reset_global(void __iomem *base_addr)
{
	u32 reset_count = 0;

	/* request scaler reset */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_RESET_CTRL_GLOBAL], &mcsc_fields[MCSC_F_SW_RESET_GLOBAL], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_RESET_STATUS], &mcsc_fields[MCSC_F_SW_RESET_GLOBAL_STATUS]) != 0);

	return 0;
}

static u32 fimc_is_scaler0_sw_reset(void __iomem *base_addr, u32 partial)
{
	u32 reset_count = 0;

	/* select chain0 partial reset */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_RESET_CTRL_0], &mcsc_fields[MCSC_F_PARTIAL_RESET_ENABLE_0], partial);

	/* request scaler reset */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_RESET_CTRL_0], &mcsc_fields[MCSC_F_SW_RESET_0], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_RESET_STATUS], &mcsc_fields[MCSC_F_SW_RESET_0_STATUS]) != 0);

	return 0;
}

u32 fimc_is_scaler_sw_reset(void __iomem *base_addr, u32 hw_id, u32 global, u32 partial)
{
	int ret = 0;

	if (global) {
		ret = fimc_is_scaler_sw_reset_global(base_addr);
		return ret;
	}

	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		ret = fimc_is_scaler0_sw_reset(base_addr, partial);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

	return ret;
}

void fimc_is_scaler_clear_intr_all(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], 0x2FFB);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_disable_intr(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_0], 0x2FFB);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_mask_intr(void __iomem *base_addr, u32 hw_id, u32 intr_mask)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_0], intr_mask);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_stop_req_post_en_ctrl(void __iomem *base_addr, u32 hw_id, u32 value)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_STOP_REQ_POST_EN_CTRL_0], &mcsc_fields[MCSC_F_STOP_REQ_POST_EN_CTRL_TYPE_0], value);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

/* get the total line count of OTF input */
void fimc_is_scaler_get_input_status(void __iomem *base_addr, u32 hw_id, u32 *hl, u32 *vl)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		*hl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_0], &mcsc_fields[MCSC_F_CUR_HORIZONTAL_CNT_0]);
		*vl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_0], &mcsc_fields[MCSC_F_CUR_VERTICAL_CNT_0]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_input_source(void __iomem *base_addr, u32 hw_id, u32 rdma)
{
	return;
}

u32 fimc_is_scaler_get_input_source(void __iomem *base_addr, u32 hw_id)
{
	/* there's no way to get input type by reading input source reg */
	return 0;
}

void fimc_is_scaler_set_dither(void __iomem *base_addr, u32 hw_id, bool dither_en)
{
	return;
}

void fimc_is_scaler_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_IMG_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_IMG_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_IMG_VSIZE], height);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_get_input_img_size(void __iomem *base_addr, u32 hw_id, u32 *width, u32 *height)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_IMG_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_IMG_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_IMG_VSIZE]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

u32 fimc_is_scaler_get_scaler_path(void __iomem *base_addr, u32 hw_id, u32 output_id)
{
	return hw_id;
}

void fimc_is_scaler_set_poly_scaler_enable(void __iomem *base_addr, u32 hw_id, u32 output_id, u32 enable)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_CTRL], &mcsc_fields[MCSC_F_POLY_SC0_P0_ENABLE], enable);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_bypass(void __iomem *base_addr, u32 output_id, u32 bypass)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_CTRL], &mcsc_fields[MCSC_F_POLY_SC0_P0_BYPASS], bypass);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_src_size(void __iomem *base_addr, u32 output_id, u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_SRC_POS], &mcsc_fields[MCSC_F_POLY_SC0_P0_SRC_HPOS], pos_x);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_SRC_POS], &mcsc_fields[MCSC_F_POLY_SC0_P0_SRC_VPOS], pos_y);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_SRC_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_SRC_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_SRC_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_SRC_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_poly_src_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_SRC_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_SRC_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_SRC_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_DST_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_DST_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_DST_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_DST_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_DST_SIZE], &mcsc_fields[MCSC_F_POLY_SC0_P0_DST_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_H_RATIO], &mcsc_fields[MCSC_F_POLY_SC0_P0_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_V_RATIO], &mcsc_fields[MCSC_F_POLY_SC0_P0_V_RATIO], vratio);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_h_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 h_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_H_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_POLY_SC0_P0_H_INIT_PHASE_OFFSET], h_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_v_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 v_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_V_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_POLY_SC0_P0_V_INIT_PHASE_OFFSET], v_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_coef(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	u32 h_coef;
	u32 v_coef;

	/* adjust H coef */
	if (hratio <= RATIO_X8_8)
		h_coef = 0;
	else if (hratio > RATIO_X8_8 && hratio <= RATIO_X7_8)
		h_coef = 1;
	else if (hratio > RATIO_X7_8 && hratio <= RATIO_X6_8)
		h_coef = 2;
	else if (hratio > RATIO_X6_8 && hratio <= RATIO_X5_8)
		h_coef = 3;
	else if (hratio > RATIO_X5_8 && hratio <= RATIO_X4_8)
		h_coef = 4;
	else if (hratio > RATIO_X4_8 && hratio <= RATIO_X3_8)
		h_coef = 5;
	else if (hratio > RATIO_X3_8 && hratio <= RATIO_X2_8)
		h_coef = 6;
	else
		return;

	/* adjust V coef */
	if (vratio <= RATIO_X8_8)
		v_coef = 0;
	else if (vratio > RATIO_X8_8 && vratio <= RATIO_X7_8)
		v_coef = 1;
	else if (vratio > RATIO_X7_8 && vratio <= RATIO_X6_8)
		v_coef = 2;
	else if (vratio > RATIO_X6_8 && vratio <= RATIO_X5_8)
		v_coef = 3;
	else if (vratio > RATIO_X5_8 && vratio <= RATIO_X4_8)
		v_coef = 4;
	else if (vratio > RATIO_X4_8 && vratio <= RATIO_X3_8)
		v_coef = 5;
	else if (vratio > RATIO_X3_8 && vratio <= RATIO_X2_8)
		v_coef = 6;
	else
		return;

	/* set coefficient for horizontal/vertical scaling filter */
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_COEFF_CTRL], &mcsc_fields[MCSC_F_POLY_SC0_P0_H_COEFF_SEL], h_coef);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POLY_SC0_P0_COEFF_CTRL], &mcsc_fields[MCSC_F_POLY_SC0_P0_V_COEFF_SEL], v_coef);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaler_enable(void __iomem *base_addr, u32 output_id, u32 enable)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_CTRL], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_ENABLE], enable);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_img_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_IMG_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_post_img_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_IMG_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_DST_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_post_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_DST_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_H_RATIO], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_POST_SC_V_RATIO], &mcsc_fields[MCSC_F_POST_CHAIN0_POST_SC_V_RATIO], vratio);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_420_conversion(void __iomem *base_addr, u32 output_id, u32 conv420_weight, u32 conv420_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_CONV420_CTRL], &mcsc_fields[MCSC_F_POST_CHAIN0_CONV420_ENABLE], conv420_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_bchs_enable(void __iomem *base_addr, u32 output_id, bool bchs_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_CTRL], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_ENABLE], bchs_en);
		break;
	default:
		break;
	}
}

/* brightness/contrast control */
void fimc_is_scaler_set_b_c(void __iomem *base_addr, u32 output_id, u32 y_offset, u32 y_gain)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_BC], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_YOFFSET], y_offset);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_BC], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_YGAIN], y_gain);
		break;
	default:
		break;
	}
}

/* hue/saturation control */
void fimc_is_scaler_set_h_s(void __iomem *base_addr, u32 output_id, u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_HS1], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_C_GAIN_00], c_gain00);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_HS1], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_HS2], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_C_GAIN_10], c_gain10);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_BCHS_HS2], &mcsc_fields[MCSC_F_POST_CHAIN0_BCHS_C_GAIN_11], c_gain11);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_dma_out_enable(void __iomem *base_addr, u32 output_id, bool dma_out_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_POST_CHAIN0_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_otf_out_enable(void __iomem *base_addr, u32 output_id, bool otf_out_en)
{
	return;
}

u32 fimc_is_scaler_get_dma_out_enable(void __iomem *base_addr, u32 output_id)
{
	int ret = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_POST_CHAIN0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_POST_CHAIN0_DMA_OUT_ENABLE]);
		break;
	default:
		break;
	}

	return ret;
}

u32 fimc_is_scaler_get_otf_out_enable(void __iomem *base_addr, u32 output_id)
{
	return 0;
}

void fimc_is_scaler_set_otf_out_path(void __iomem *base_addr, u32 output_id)
{
	return;
}

void fimc_is_scaler_set_rdma_format(void __iomem *base_addr, u32 hw_id, u32 dma_in_format)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_DATA_FORMAT], &mcsc_fields[MCSC_F_RDMA_DATA_FORMAT], dma_in_format);
}

void fimc_is_scaler_set_wdma_format(void __iomem *base_addr, u32 hw_id, u32 output_id, u32 dma_out_format)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA0_DATA_FORMAT], dma_out_format);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_swap_mode(void __iomem *base_addr, u32 output_id, u32 swap)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_SWAP_TABLE], &mcsc_fields[MCSC_F_WDMA0_SWAP_TABLE], swap);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_flip_mode(void __iomem *base_addr, u32 output_id, u32 flip)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_FLIP_CONTROL], &mcsc_fields[MCSC_F_WDMA0_FLIP_CONTROL], flip);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_size(void __iomem *base_addr, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_WIDTH], &mcsc_fields[MCSC_F_RDMA_WIDTH], width);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_HEIGHT], &mcsc_fields[MCSC_F_RDMA_HEIGHT], height);
}

void fimc_is_scaler_get_rdma_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA_WIDTH], &mcsc_fields[MCSC_F_RDMA_WIDTH]);
	*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA_HEIGHT], &mcsc_fields[MCSC_F_RDMA_HEIGHT]);
}

void fimc_is_scaler_set_wdma_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_WIDTH], &mcsc_fields[MCSC_F_WDMA0_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_HEIGHT], &mcsc_fields[MCSC_F_WDMA0_HEIGHT], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_WIDTH], &mcsc_fields[MCSC_F_WDMA0_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_HEIGHT], &mcsc_fields[MCSC_F_WDMA0_HEIGHT]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_stride(void __iomem *base_addr, u32 y_stride, u32 uv_stride)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_STRIDE], &mcsc_fields[MCSC_F_RDMA_Y_STRIDE], y_stride);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_STRIDE], &mcsc_fields[MCSC_F_RDMA_C_STRIDE], uv_stride);
}

void fimc_is_scaler_get_rdma_stride(void __iomem *base_addr, u32 *y_stride, u32 *uv_stride)
{
	*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA_STRIDE], &mcsc_fields[MCSC_F_RDMA_Y_STRIDE]);
	*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA_STRIDE], &mcsc_fields[MCSC_F_RDMA_C_STRIDE]);
}

void fimc_is_scaler_set_wdma_stride(void __iomem *base_addr, u32 output_id, u32 y_stride, u32 uv_stride)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE], &mcsc_fields[MCSC_F_WDMA0_Y_STRIDE], y_stride);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE], &mcsc_fields[MCSC_F_WDMA0_C_STRIDE], uv_stride);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_stride(void __iomem *base_addr, u32 output_id, u32 *y_stride, u32 *uv_stride)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE], &mcsc_fields[MCSC_F_WDMA0_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE], &mcsc_fields[MCSC_F_WDMA0_C_STRIDE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_frame_seq(void __iomem *base_addr, u32 frame_seq)
{
	u32 seq = 0;

	if (!(frame_seq <= 0xffffffff))
		return;

	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN0], frame_seq >> 0);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN1], frame_seq >> 1);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN2], frame_seq >> 2);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN3], frame_seq >> 3);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN4], frame_seq >> 4);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN5], frame_seq >> 5);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN6], frame_seq >> 6);
	seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN7], frame_seq >> 7);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], seq);
}

void fimc_is_scaler_set_wdma_frame_seq(void __iomem *base_addr, u32 output_id, u32 frame_seq)
{
	u32 seq = 0;

	if (!(frame_seq <= 0xffffffff))
		return;

	switch (output_id) {
	case MCSC_OUTPUT0:
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN0], frame_seq >> 0);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN1], frame_seq >> 1);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN2], frame_seq >> 2);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN3], frame_seq >> 3);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN4], frame_seq >> 4);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN5], frame_seq >> 5);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN6], frame_seq >> 6);
		seq = fimc_is_hw_set_field_value(seq, &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN7], frame_seq >> 7);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], seq);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR00], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR10], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR20], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR21], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR22], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR23], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR24], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR25], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR26], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR27], cr_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma0_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR00], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR10], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR20], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR21], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR22], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR23], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR24], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR25], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR26], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR27], cr_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_addr(void __iomem *base_addr, u32 output_id, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_scaler_set_wdma0_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_rdma_addr(void __iomem *base_addr)
{
	/* DMA Y address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR00], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR01], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR02], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR03], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR04], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR05], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR06], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR07], 0x0);

	/* DMA CB address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR10], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR11], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR12], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR13], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR14], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR15], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR16], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR17], 0x0);

	/* DMA CR address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR20], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR21], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR22], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR23], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR24], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR25], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR26], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR27], 0x0);
}

void fimc_is_scaler_clear_wdma_addr(void __iomem *base_addr, u32 output_id)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR00], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR01], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR02], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR03], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR04], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR05], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR06], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR07], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR10], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR11], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR12], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR13], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR14], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR15], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR16], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR17], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR20], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR21], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR22], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR23], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR24], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR25], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR26], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR27], 0x0);
		break;
	default:
		break;
	}
}

/* for hwfc */
void fimc_is_scaler_set_hwfc_auto_clear(void __iomem *base_addr, u32 output_id, bool auto_clear)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_ENABLE_AUTO_CLEAR],
			&mcsc_fields[MCSC_F_HWFC_ENABLE_AUTO_CLEAR], auto_clear);
}

void fimc_is_scaler_set_hwfc_idx_reset(void __iomem *base_addr, u32 output_id, bool reset)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_INDEX_RESET],
			&mcsc_fields[MCSC_F_INDEX_RESET], reset);
}

void fimc_is_scaler_set_hwfc_mode(void __iomem *base_addr, u32 hwfc_output_ids)
{
	u32 val = MCSC_HWFC_MODE_OFF;

	if (hwfc_output_ids & (1 << MCSC_OUTPUT0))
		val = MCSC_HWFC_MODE_REGION_A_PORT;

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_MODE],
			&mcsc_fields[MCSC_F_HWFC_MODE], val);
}

void fimc_is_scaler_set_hwfc_config(void __iomem *base_addr,
		u32 output_id, u32 format, u32 plane, u32 dma_idx, u32 width, u32 height)
{
	u32 val = 0;
	u32 img_resol = width * height;
	u32 total_img_byte0 = 0;
	u32 total_img_byte1 = 0;
	u32 total_img_byte2 = 0;
	u32 total_width_byte0 = 0;
	u32 total_width_byte1 = 0;
	u32 total_width_byte2 = 0;
	enum fimc_is_mcsc_hwfc_format hwfc_format = 0;

	val = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_A]);

	switch (format) {
	case DMA_OUTPUT_FORMAT_YUV422:
		switch (plane) {
		case 3:
			total_img_byte0 = img_resol;
			total_img_byte1 = img_resol >> 1;
			total_img_byte2 = img_resol >> 1;
			total_width_byte0 = width;
			total_width_byte1 = width >> 1;
			total_width_byte2 = width >> 1;
			break;
		case 2:
			total_img_byte0 = img_resol;
			total_img_byte1 = img_resol;
			total_width_byte0 = width;
			total_width_byte1 = width;
			break;
		case 1:
			total_img_byte0 = img_resol << 1;
			total_width_byte0 = width << 1;
			break;
		default:
			err_hw("invalid hwfc plane (%d, %d, %dx%d)\n",
					format, plane, width, height);
			return;
		}

		hwfc_format = MCSC_HWFC_FMT_YUV444_YUV422;
		break;
	case DMA_OUTPUT_FORMAT_YUV420:
		switch (plane) {
		case 3:
			total_img_byte0 = img_resol;
			total_img_byte1 = img_resol >> 2;
			total_img_byte2 = img_resol >> 2;
			total_width_byte0 = width;
			total_width_byte1 = width >> 2;
			total_width_byte2 = width >> 2;
			break;
		case 2:
			total_img_byte0 = img_resol;
			total_img_byte1 = img_resol >> 1;
			total_width_byte0 = width;
			total_width_byte1 = width >> 1;
			break;
		default:
			err_hw("invalid hwfc plane (%d, %d, %dx%d)\n",
					format, plane, width, height);
			return;
		}

		hwfc_format = MCSC_HWFC_FMT_YUV420;
		break;
	default:
		err_hw("invalid hwfc format (%d, %d, %dx%d)\n",
				format, plane, width, height);
		return;
	}

	/* format */
	val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_FORMAT_A], hwfc_format);

	/* plane */
	val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_PLANE_A], plane);

	/* dma idx */
	val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_ID0_A], dma_idx);
	val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_ID1_A], dma_idx+1);
	val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_ID2_A], dma_idx+2);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_A], val);

	/* total pixel/byte */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE0_A], total_img_byte0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE1_A], total_img_byte1);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE2_A], total_img_byte2);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE0_A], total_width_byte0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE1_A], total_width_byte1);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE2_A], total_width_byte2);
}

u32 fimc_is_scaler_get_hwfc_idx_bin(void __iomem *base_addr, u32 output_id)
{
	return fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_REGION_IDX_BIN],
			&mcsc_fields[MCSC_F_REGION_IDX_BIN_A]);
}

u32 fimc_is_scaler_get_hwfc_cur_idx(void __iomem *base_addr, u32 output_id)
{
	return fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_CURR_REGION],
			&mcsc_fields[MCSC_F_CURR_REGION_A]);
}

static void fimc_is_scaler0_clear_intr_src(void __iomem *base_addr, u32 status)
{
	u32 val = 0;

	val = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0]);

	if (status & (1 << INTR_MC_SCALER_OVERFLOW))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_INPUT_PROTOCOL_ERR))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_INPUT_PROTOCOL_ERR_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_SHADOW_TRIGGER))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_SHADOW_TRIGGER_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_SHADOW_HW_TRIGGER))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_SHADOW_HW_TRIGGER_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_CORE_FINISH))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_CORE_FINISH_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_WDMA_FINISH_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_FRAME_START))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_FRAME_START_INT_0], 1);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_FRAME_END_INT_0], 1);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], val);
}

void fimc_is_scaler_clear_intr_src(void __iomem *base_addr, u32 hw_id, u32 status)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		fimc_is_scaler0_clear_intr_src(base_addr, status);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

u32 fimc_is_scaler_get_intr_mask(void __iomem *base_addr, u32 hw_id)
{
	int ret = 0;

	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		ret = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_0]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

	return ret;
}

u32 fimc_is_scaler_get_intr_status(void __iomem *base_addr, u32 hw_id)
{
	int ret = 0;

	switch (hw_id) {
	case DEV_HW_MCSC0:
	case DEV_HW_MCSC1:
		ret = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

	return ret;
}

u32 fimc_is_scaler_get_version(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_VERSION]);
}

void fimc_is_scaler_dump(void __iomem *base_addr)
{
	u32 i = 0;
	u32 reg_value = 0;

	info_hw("MCSC ver 1.22 / 1.23");

	for(i = 0; i < MCSC_REG_CNT; i++) {
		reg_value = readl(base_addr + mcsc_regs[i].sfr_offset);
		sfrinfo("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			mcsc_regs[i].reg_name, mcsc_regs[i].sfr_offset, reg_value);
	}
}
