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
#include "sfr/fimc-is-sfr-mcsc-v210.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-control.h"
#include "fimc-is-param.h"

void fimc_is_scaler_start(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
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
	u32 reg_value = 0;
	switch (hw_id) {
	case DEV_HW_MCSC0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_OUT_STALL_BLOCKING_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_TRIGGER_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_HW_TRIGGER_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_CORE_FINISH_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA_FINISH_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_START_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_END_INT_0], 1);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], reg_value);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_disable_intr(void __iomem *base_addr, u32 hw_id)
{
	u32 reg_value = 0;
	switch (hw_id) {
	case DEV_HW_MCSC0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_OUT_STALL_BLOCKING_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_TRIGGER_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_HW_TRIGGER_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_CORE_FINISH_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA_FINISH_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_START_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_END_INT_0_MASK], 1);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_0], reg_value);
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_STOP_REQ_POST_EN_CTRL_0], &mcsc_fields[MCSC_F_STOP_REQ_POST_EN_CTRL_TYPE_0], value);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_get_input_status(void __iomem *base_addr, u32 hw_id, u32 *hl, u32 *vl)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
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
	/* 0: otf input, 1: rdma input */
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT_SRC0_CTRL], &mcsc_fields[MCSC_F_INPUT_SRC0_SEL], rdma);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

u32 fimc_is_scaler_get_input_source(void __iomem *base_addr, u32 hw_id)
{
	int ret = 0;
	/* 0: otf input, 1: rdma input */
	switch (hw_id) {
	case DEV_HW_MCSC0:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_INPUT_SRC0_CTRL], &mcsc_fields[MCSC_F_INPUT_SRC0_SEL]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

	return ret;
}

void fimc_is_scaler_set_dither(void __iomem *base_addr, u32 hw_id, bool dither_en)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DITHER_0_CTRL], &mcsc_fields[MCSC_F_DITHER_0_ON], dither_en);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT0_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT0_IMG_VSIZE], height);
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
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT0_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT0_IMG_VSIZE]);
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_ENABLE], enable);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_CTRL], &mcsc_fields[MCSC_F_SC1_ENABLE], enable);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_CTRL], &mcsc_fields[MCSC_F_SC2_ENABLE], enable);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_bypass(void __iomem *base_addr, u32 output_id, u32 bypass)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_BYPASS], bypass);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_CTRL], &mcsc_fields[MCSC_F_SC1_BYPASS], bypass);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_CTRL], &mcsc_fields[MCSC_F_SC2_BYPASS], bypass);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_src_size(void __iomem *base_addr, u32 output_id, u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_POS], &mcsc_fields[MCSC_F_SC0_SRC_HPOS], pos_x);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_POS], &mcsc_fields[MCSC_F_SC0_SRC_VPOS], pos_y);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE], &mcsc_fields[MCSC_F_SC0_SRC_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE], &mcsc_fields[MCSC_F_SC0_SRC_VSIZE], height);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_POS], &mcsc_fields[MCSC_F_SC1_SRC_HPOS], pos_x);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_POS], &mcsc_fields[MCSC_F_SC1_SRC_VPOS], pos_y);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE], &mcsc_fields[MCSC_F_SC1_SRC_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE], &mcsc_fields[MCSC_F_SC1_SRC_VSIZE], height);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_POS], &mcsc_fields[MCSC_F_SC2_SRC_HPOS], pos_x);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_POS], &mcsc_fields[MCSC_F_SC2_SRC_VPOS], pos_y);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE], &mcsc_fields[MCSC_F_SC2_SRC_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE], &mcsc_fields[MCSC_F_SC2_SRC_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_poly_src_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE], &mcsc_fields[MCSC_F_SC0_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE], &mcsc_fields[MCSC_F_SC0_SRC_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE], &mcsc_fields[MCSC_F_SC1_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE], &mcsc_fields[MCSC_F_SC1_SRC_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE], &mcsc_fields[MCSC_F_SC2_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE], &mcsc_fields[MCSC_F_SC2_SRC_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE], &mcsc_fields[MCSC_F_SC0_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE], &mcsc_fields[MCSC_F_SC0_DST_VSIZE], height);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE], &mcsc_fields[MCSC_F_SC1_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE], &mcsc_fields[MCSC_F_SC1_DST_VSIZE], height);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE], &mcsc_fields[MCSC_F_SC2_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE], &mcsc_fields[MCSC_F_SC2_DST_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE], &mcsc_fields[MCSC_F_SC0_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE], &mcsc_fields[MCSC_F_SC0_DST_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE], &mcsc_fields[MCSC_F_SC1_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE], &mcsc_fields[MCSC_F_SC1_DST_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE], &mcsc_fields[MCSC_F_SC2_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE], &mcsc_fields[MCSC_F_SC2_DST_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_RATIO], &mcsc_fields[MCSC_F_SC0_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_RATIO], &mcsc_fields[MCSC_F_SC0_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_RATIO], &mcsc_fields[MCSC_F_SC1_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_RATIO], &mcsc_fields[MCSC_F_SC1_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_RATIO], &mcsc_fields[MCSC_F_SC2_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_RATIO], &mcsc_fields[MCSC_F_SC2_V_RATIO], vratio);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_h_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 h_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC0_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC1_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC2_H_INIT_PHASE_OFFSET], h_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_v_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 v_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC0_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC1_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC2_V_INIT_PHASE_OFFSET], v_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_coef(void __iomem *base_addr,
	u32 output_id,
	u32 hratio,
	u32 vratio,
	enum exynos_sensor_position sensor_position)
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_COEFF_CTRL], &mcsc_fields[MCSC_F_SC0_H_COEFF_SEL], h_coef);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_COEFF_CTRL], &mcsc_fields[MCSC_F_SC0_V_COEFF_SEL], v_coef);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_COEFF_CTRL], &mcsc_fields[MCSC_F_SC1_H_COEFF_SEL], h_coef);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_COEFF_CTRL], &mcsc_fields[MCSC_F_SC1_V_COEFF_SEL], v_coef);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_COEFF_CTRL], &mcsc_fields[MCSC_F_SC2_H_COEFF_SEL], h_coef);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_COEFF_CTRL], &mcsc_fields[MCSC_F_SC2_V_COEFF_SEL], v_coef);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaler_enable(void __iomem *base_addr, u32 output_id, u32 enable)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC0_POST_SC_ENABLE], enable);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC1_POST_SC_ENABLE], enable);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC2_POST_SC_ENABLE], enable);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_img_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_IMG_VSIZE], height);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_IMG_VSIZE], height);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_IMG_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_post_img_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_IMG_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_IMG_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_IMG_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_DST_VSIZE], height);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_DST_VSIZE], height);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_DST_VSIZE], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_post_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC0_POST_SC_DST_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC1_POST_SC_DST_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC2_POST_SC_DST_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_H_RATIO], &mcsc_fields[MCSC_F_PC0_POST_SC_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_V_RATIO], &mcsc_fields[MCSC_F_PC0_POST_SC_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_H_RATIO], &mcsc_fields[MCSC_F_PC1_POST_SC_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_V_RATIO], &mcsc_fields[MCSC_F_PC1_POST_SC_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_H_RATIO], &mcsc_fields[MCSC_F_PC2_POST_SC_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_V_RATIO], &mcsc_fields[MCSC_F_PC2_POST_SC_V_RATIO], vratio);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_420_conversion(void __iomem *base_addr, u32 output_id, u32 conv420_weight, u32 conv420_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_CONV420_WEIGHT], &mcsc_fields[MCSC_F_PC0_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_CONV420_CTRL], &mcsc_fields[MCSC_F_PC0_CONV420_ENABLE], conv420_en);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_CONV420_WEIGHT], &mcsc_fields[MCSC_F_PC1_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_CONV420_CTRL], &mcsc_fields[MCSC_F_PC1_CONV420_ENABLE], conv420_en);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_CONV420_WEIGHT], &mcsc_fields[MCSC_F_PC2_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_CONV420_CTRL], &mcsc_fields[MCSC_F_PC2_CONV420_ENABLE], conv420_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_bchs_enable(void __iomem *base_addr, u32 output_id, bool bchs_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_CTRL], &mcsc_fields[MCSC_F_PC0_BCHS_ENABLE], bchs_en);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_CTRL], &mcsc_fields[MCSC_F_PC1_BCHS_ENABLE], bchs_en);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_CTRL], &mcsc_fields[MCSC_F_PC2_BCHS_ENABLE], bchs_en);
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_BC], &mcsc_fields[MCSC_F_PC0_BCHS_YOFFSET], y_offset);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_BC], &mcsc_fields[MCSC_F_PC0_BCHS_YGAIN], y_gain);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_BC], &mcsc_fields[MCSC_F_PC1_BCHS_YOFFSET], y_offset);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_BC], &mcsc_fields[MCSC_F_PC1_BCHS_YGAIN], y_gain);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_BC], &mcsc_fields[MCSC_F_PC2_BCHS_YOFFSET], y_offset);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_BC], &mcsc_fields[MCSC_F_PC2_BCHS_YGAIN], y_gain);
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_HS1], &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_00], c_gain00);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_HS1], &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_HS2], &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_10], c_gain10);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_HS2], &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_11], c_gain11);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_HS1], &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_00], c_gain00);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_HS1], &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_HS2], &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_10], c_gain10);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_HS2], &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_11], c_gain11);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_HS1], &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_00], c_gain00);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_HS1], &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_HS2], &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_10], c_gain10);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_HS2], &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_11], c_gain11);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_dma_out_enable(void __iomem *base_addr, u32 output_id, bool dma_out_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_otf_out_enable(void __iomem *base_addr, u32 output_id, bool otf_out_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_OTF_OUT_ENABLE], (u32)otf_out_en);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_OTF_OUT_ENABLE], (u32)otf_out_en);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_OTF_OUT_ENABLE], (u32)otf_out_en);
		break;
	default:
		break;
	}
}

u32 fimc_is_scaler_get_dma_out_enable(void __iomem *base_addr, u32 output_id)
{
	int ret = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT1:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT2:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE]);
		break;
	default:
		break;
	}

	return ret;
}

u32 fimc_is_scaler_get_otf_out_enable(void __iomem *base_addr, u32 output_id)
{
	int ret = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_OTF_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT1:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_OTF_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT2:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_OTF_OUT_ENABLE]);
		break;
	default:
		break;
	}

	return ret;
}

void fimc_is_scaler_set_otf_out_path(void __iomem *base_addr, u32 output_id)
{
	/* 0 ~ 4 : otf out path */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_OTF_OUTPUT_PATH_CTRL], &mcsc_fields[MCSC_F_OTF_OUT_SEL], output_id);
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
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA1_DATA_FORMAT], dma_out_format);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA2_DATA_FORMAT], dma_out_format);
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
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_SWAP_TABLE], &mcsc_fields[MCSC_F_WDMA1_SWAP_TABLE], swap);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_SWAP_TABLE], &mcsc_fields[MCSC_F_WDMA2_SWAP_TABLE], swap);
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
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_FLIP_CONTROL], &mcsc_fields[MCSC_F_WDMA1_FLIP_CONTROL], flip);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_FLIP_CONTROL], &mcsc_fields[MCSC_F_WDMA2_FLIP_CONTROL], flip);
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
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_WIDTH], &mcsc_fields[MCSC_F_WDMA1_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_HEIGHT], &mcsc_fields[MCSC_F_WDMA1_HEIGHT], height);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_WIDTH], &mcsc_fields[MCSC_F_WDMA2_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_HEIGHT], &mcsc_fields[MCSC_F_WDMA2_HEIGHT], height);
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
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_WIDTH], &mcsc_fields[MCSC_F_WDMA1_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_HEIGHT], &mcsc_fields[MCSC_F_WDMA1_HEIGHT]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_WIDTH], &mcsc_fields[MCSC_F_WDMA2_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_HEIGHT], &mcsc_fields[MCSC_F_WDMA2_HEIGHT]);
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
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE], &mcsc_fields[MCSC_F_WDMA1_Y_STRIDE], y_stride);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE], &mcsc_fields[MCSC_F_WDMA1_C_STRIDE], uv_stride);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], &mcsc_fields[MCSC_F_WDMA2_Y_STRIDE], y_stride);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], &mcsc_fields[MCSC_F_WDMA2_C_STRIDE], uv_stride);
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
	case MCSC_OUTPUT1:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE], &mcsc_fields[MCSC_F_WDMA1_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE], &mcsc_fields[MCSC_F_WDMA1_C_STRIDE]);
		break;
	case MCSC_OUTPUT2:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], &mcsc_fields[MCSC_F_WDMA2_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], &mcsc_fields[MCSC_F_WDMA2_C_STRIDE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_frame_seq(void __iomem *base_addr, u32 frame_seq)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN1], (frame_seq & 0x01) >> 0);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN2], (frame_seq & 0x02) >> 1);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN3], (frame_seq & 0x04) >> 2);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN4], (frame_seq & 0x08) >> 3);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN5], (frame_seq & 0x10) >> 4);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN6], (frame_seq & 0x20) >> 5);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN7], (frame_seq & 0x40) >> 6);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR_EN], &mcsc_fields[MCSC_F_RDMA_BASE_ADDR_EN8], (frame_seq & 0x80) >> 7);
}

void fimc_is_scaler_set_wdma_frame_seq(void __iomem *base_addr, u32 output_id, u32 frame_seq)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN1], (frame_seq & 0x01) >> 0);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN2], (frame_seq & 0x02) >> 1);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN3], (frame_seq & 0x04) >> 2);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN4], (frame_seq & 0x08) >> 3);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN5], (frame_seq & 0x10) >> 4);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN6], (frame_seq & 0x20) >> 5);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN7], (frame_seq & 0x40) >> 6);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA0_BASE_ADDR_EN8], (frame_seq & 0x80) >> 7);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN1], (frame_seq & 0x01) >> 0);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN2], (frame_seq & 0x02) >> 1);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN3], (frame_seq & 0x04) >> 2);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN4], (frame_seq & 0x08) >> 3);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN5], (frame_seq & 0x10) >> 4);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN6], (frame_seq & 0x20) >> 5);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN7], (frame_seq & 0x40) >> 6);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA1_BASE_ADDR_EN8], (frame_seq & 0x80) >> 7);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN1], (frame_seq & 0x01) >> 0);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN2], (frame_seq & 0x02) >> 1);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN3], (frame_seq & 0x04) >> 2);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN4], (frame_seq & 0x08) >> 3);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN5], (frame_seq & 0x10) >> 4);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN6], (frame_seq & 0x20) >> 5);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN7], (frame_seq & 0x40) >> 6);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA2_BASE_ADDR_EN8], (frame_seq & 0x80) >> 7);
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
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR21], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR22], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR23], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR24], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR25], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR26], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR27], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR08], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR28], cr_addr);
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
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR21], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR22], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR23], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR24], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR25], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR26], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR27], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR08], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR28], cr_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma1_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR21], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR22], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR23], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR24], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR25], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR26], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR27], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR08], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR28], cr_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma2_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR21], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR22], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR23], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR24], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR25], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR26], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR27], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR08], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR28], cr_addr);
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
	case MCSC_OUTPUT1:
		fimc_is_scaler_set_wdma1_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	case MCSC_OUTPUT2:
		fimc_is_scaler_set_wdma2_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_rdma_addr(void __iomem *base_addr)
{
	/* DMA Y address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR01], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR02], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR03], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR04], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR05], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR06], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR07], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR08], 0x0);

	/* DMA CB address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR11], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR12], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR13], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR14], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR15], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR16], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR17], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR18], 0x0);

	/* DMA CR address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR21], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR22], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR23], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR24], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR25], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR26], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR27], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA_BASE_ADDR28], 0x0);
}

void fimc_is_scaler_clear_wdma_addr(void __iomem *base_addr, u32 output_id)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR01], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR02], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR03], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR04], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR05], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR06], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR07], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR08], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR11], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR12], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR13], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR14], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR15], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR16], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR17], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR18], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR21], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR22], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR23], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR24], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR25], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR26], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR27], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR28], 0x0);
		break;
	case MCSC_OUTPUT1:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR01], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR02], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR03], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR04], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR05], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR06], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR07], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR08], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR11], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR12], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR13], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR14], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR15], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR16], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR17], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR18], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR21], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR22], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR23], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR24], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR25], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR26], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR27], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR28], 0x0);
		break;
	case MCSC_OUTPUT2:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR01], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR02], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR03], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR04], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR05], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR06], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR07], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR08], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR11], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR12], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR13], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR14], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR15], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR16], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR17], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR18], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR21], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR22], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR23], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR24], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR25], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR26], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR27], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR28], 0x0);
		break;
	default:
		break;
	}
}

/* for hwfc */
void fimc_is_scaler_set_hwfc_auto_clear(void __iomem *base_addr, u32 output_id, bool auto_clear)
{
	return;
}

void fimc_is_scaler_set_hwfc_idx_reset(void __iomem *base_addr, u32 output_id, bool reset)
{
	return;
}

void fimc_is_scaler_set_hwfc_mode(void __iomem *base_addr, u32 hwfc_output_ids)
{
	return;
}

void fimc_is_scaler_set_hwfc_config(void __iomem *base_addr,
		u32 output_id, u32 format, u32 plane, u32 dma_idx, u32 width, u32 height)
{
	return;
}

u32 fimc_is_scaler_set_hwfc_idx_bin(void __iomem *base_addr, u32 output_id)
{
	return 0;
}

u32 fimc_is_scaler_set_hwfc_cur_idx(void __iomem *base_addr, u32 output_id)
{
	return 0;
}

static void fimc_is_scaler0_clear_intr_src(void __iomem *base_addr, u32 status)
{
	if (status & (1 << INTR_MC_SCALER_OUTSTALL))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 12);

	if (status & (1 << INTR_MC_SCALER_OVERFLOW))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 11);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 10);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 9);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 8);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 7);

	if (status & (1 << INTR_MC_SCALER_SHADOW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 5);

	if (status & (1 << INTR_MC_SCALER_SHADOW_HW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 4);

	if (status & (1 << INTR_MC_SCALER_CORE_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 3);

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 2);

	if (status & (1 << INTR_MC_SCALER_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 1);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << 0);
}

void fimc_is_scaler_clear_intr_src(void __iomem *base_addr, u32 hw_id, u32 status)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
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

	info_hw("MCSC ver 2.10");

	for(i = 0; i < MCSC_REG_CNT; i++) {
		reg_value = readl(base_addr + mcsc_regs[i].sfr_offset);
		sfrinfo("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			mcsc_regs[i].reg_name, mcsc_regs[i].sfr_offset, reg_value);
	}
}
