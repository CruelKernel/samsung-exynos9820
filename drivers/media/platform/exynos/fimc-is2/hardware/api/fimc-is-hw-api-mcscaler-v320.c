/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-api-mcscaler-v2.h"
#include "sfr/fimc-is-sfr-mcsc-v320.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-control.h"
#include "fimc-is-param.h"


/* WDMA SRAM base value, UG recommanded value */
#define WDMA0_SRAM_BASE_VALUE	(0)
#define WDMA1_SRAM_BASE_VALUE	(200)
#define WDMA2_SRAM_BASE_VALUE	(700)

const struct mcsc_v_coef v_coef_4tap[7] = {
	/* x8/8 */
	{
		{  0,  -4,  -6,  -8,  -8,  -8,  -8,  -7,  -6},
		{128, 127, 124, 118, 111, 102,  92,  81,  70},
		{  0,   5,  11,  19,  27,  37,  48,  59,  70},
		{  0,   0,  -1,  -1,  -2,  -3,  -4,  -5,  -6},
	},
	/* x7/8 */
	{
		{  8,   4,   1,  -2,  -3,  -5,  -5,  -5,  -5},
		{112, 111, 109, 105, 100,  93,  86,  77,  69},
		{  8,  14,  20,  27,  34,  43,  51,  60,  69},
		{  0,  -1,  -2,  -2,  -3,  -3,  -4,  -4,  -5},
	},
	/* x6/8 */
	{
		{ 16,  12,   8,   5,   2,   0,  -1,  -2,  -2},
		{ 96,  97,  96,  93,  89,  84,  79,  73,  66},
		{ 16,  21,  26,  32,  39,  46,  53,  59,  66},
		{  0,  -2,  -2,  -2,  -2,  -2,  -3,  -2,  -2},
	},
	/* x5/8 */
	{
		{ 22,  18,  14,  11,   8,   6,   4,   2,   1},
		{ 84,  85,  84,  82,  79,  76,  72,  68,  63},
		{ 22,  26,  31,  36,  42,  47,  52,  58,  63},
		{  0,  -1,  -1,  -1,  -1,  -1,   0,   0,   1},
	},
	/* x4/8 */
	{
		{ 26,  22,  19,  16,  13,  10,   8,   6,   5},
		{ 76,  76,  75,  73,  71,  69,  66,  63,  59},
		{ 26,  30,  34,  38,  43,  47,  51,  55,  59},
		{  0,   0,   0,   1,   1,   2,   3,   4,   5},
	},
	/* x3/8 */
	{
		{ 29,  26,  23,  20,  17,  15,  12,  10,   8},
		{ 70,  68,  67,  66,  65,  63,  61,  58,  56},
		{ 29,  32,  36,  39,  43,  46,  50,  53,  56},
		{  0,   2,   2,   3,   3,   4,   5,   7,   8},
	},
	/* x2/8 */
	{
		{ 32,  28,  25,  22,  19,  17,  15,  13,  11},
		{ 64,  63,  62,  62,  61,  59,  58,  55,  53},
		{ 32,  34,  37,  40,  43,  46,  48,  51,  53},
		{  0,   3,   4,   4,   5,   6,   7,   9,  11},
	}
};

const struct mcsc_h_coef h_coef_8tap[7] = {
	/* x8/8 */
	{
		{  0,  -1,  -1,  -1,  -1,  -1,  -2,  -1,  -1},
		{  0,   2,   4,   5,   6,   6,   7,   6,   6},
		{  0,  -6, -12, -15, -18, -20, -21, -20, -20},
		{128, 127, 125, 120, 114, 107,  99,  89,  79},
		{  0,   7,  16,  25,  35,  46,  57,  68,  79},
		{  0,  -2,  -5,  -8, -10, -13, -16, -18, -20},
		{  0,   1,   1,   2,   3,   4,   5,   5,   6},
		{  0,   0,   0,   0,  -1,  -1,  -1,  -1,  -1},
	},
	/* x7/8 */
	{
		{  3,   2,   2,   1,   1,   1,   0,   0,   0},
		{ -8,  -6,  -4,  -2,  -1,   1,   2,   3,   3},
		{ 14,   7,   1,  -3,  -7, -11, -13, -15, -16},
		{111, 112, 110, 106, 103,  97,  91,  85,  77},
		{ 13,  21,  28,  36,  44,  53,  61,  69,  77},
		{ -8, -10, -12, -13, -15, -16, -16, -17, -16},
		{  3,   3,   4,   4,   4,   4,   4,   4,   3},
		{  0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0},
	},
	/* x6/8 */
	{
		{  2,   2,   2,   2,   2,   2,   2,   1,   1},
		{-11, -10,  -9,  -8,  -7,  -5,  -4,  -3,  -2},
		{ 25,  19,  14,  10,   5,   1,  -2,  -5,  -7},
		{ 96,  96,  94,  92,  90,  86,  82,  77,  72},
		{ 25,  31,  37,  43,  49,  55,  61,  67,  72},
		{-11, -12, -12, -12, -12, -12, -11,  -9,  -7},
		{  2,   2,   2,   1,   1,   0,  -1,  -1,  -2},
		{  0,   0,   0,   0,   0,   1,   1,   1,   1},
	},
	/* x5/8 */
	{
		{ -1,  -1,   0,   0,   0,   0,   1,   1,   1},
		{ -8,  -8,  -8,  -8,  -8,  -7,  -7,  -6,  -6},
		{ 33,  28,  24,  20,  16,  13,  10,   6,   4},
		{ 80,  80,  79,  78,  76,  74,  71,  68,  65},
		{ 33,  37,  41,  46,  50,  54,  58,  62,  65},
		{ -8,  -7,  -7,  -6,  -4,  -3,  -1,   1,   4},
		{ -1,  -2,  -2,  -3,  -3,  -4,  -5,  -5,  -6},
		{  0,   1,   1,   1,   1,   1,   1,   1,   1},
	},
	/* x4/8 */
	{
		{ -3,  -3,  -2,  -2,  -2,  -2,  -1,  -1,  -1},
		{  0,  -1,  -2,  -3,  -3,  -3,  -4,  -4,  -4},
		{ 35,  32,  29,  27,  24,  21,  19,  16,  14},
		{ 64,  64,  63,  63,  61,  60,  59,  57,  55},
		{ 35,  38,  41,  43,  46,  49,  51,  53,  55},
		{  0,   1,   2,   4,   6,   7,   9,  12,  14},
		{ -3,  -3,  -3,  -4,  -4,  -4,  -4,  -4,  -4},
		{  0,   0,   0,   0,   0,   0,  -1,  -1,  -1},
	},
	/* x3/8 */
	{
		{ -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
		{  8,   7,   6,   5,   4,   3,   2,   2,   1},
		{ 33,  31,  30,  28,  26,  24,  23,  21,  19},
		{ 48,  49,  49,  48,  48,  47,  47,  45,  45},
		{ 33,  35,  36,  38,  39,  41,  42,  43,  45},
		{  8,   9,  10,  12,  13,  15,  16,  18,  19},
		{ -1,  -1,  -1,  -1,   0,   0,   0,   1,   1},
		{  0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
	},
	/* x2/8 */
	{
		{  2,   2,   2,   2,   1,   1,   1,   1,   1},
		{ 13,  12,  11,  10,  10,   9,   8,   7,   6},
		{ 30,  29,  28,  26,  26,  24,  24,  22,  21},
		{ 38,  38,  38,  38,  37,  37,  37,  36,  36},
		{ 30,  30,  31,  32,  33,  34,  34,  35,  36},
		{ 13,  14,  15,  16,  17,  18,  19,  20,  21},
		{  2,   3,   3,   4,   4,   5,   5,   6,   6},
		{  0,   0,   0,   0,   0,   0,   0,   1,   1},
	}
};

void fimc_is_scaler_start(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		/* Qactive must set to "1" before scaler enable */
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_APB_CLK_GATE_CTRL], &mcsc_fields[MCSC_F_QACTIVE_ENABLE], 1);

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

u32 fimc_is_scaler_sw_reset(void __iomem *base_addr, u32 hw_id, u32 global, u32 partial)
{
	int ret = 0;

	/* MC-SC v3.20 must always use Global reset
	 * As TDNR Block only re-set global reset settings,
	 * (for flush remained RDMA1(or RDMA2) data)
	 */
	ret = fimc_is_scaler_sw_reset_global(base_addr);

	return ret;
}

void fimc_is_scaler_clear_intr_all(void __iomem *base_addr, u32 hw_id)
{
	u32 reg_value = 0;
	switch (hw_id) {
	case DEV_HW_MCSC0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_OVER_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_OUT_STALL_BLOCKING_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_0], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_0], 1);
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_OVER_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_OUT_STALL_BLOCKING_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_0_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_0_MASK], 1);
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

static void fimc_is_scaler0_set_shadow_ctrl(void __iomem *base_addr, enum mcsc_shadow_ctrl ctrl)
{
	u32 reg_value = 0;

	switch (ctrl) {
	case SHADOW_WRITE_START:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_WR_START], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_WR_FINISH], 0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SHADOW_REG_CTRL], reg_value);
		break;
	case SHADOW_WRITE_FINISH:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_WR_START], 0);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_WR_FINISH], 1);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SHADOW_REG_CTRL], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_shadow_ctrl(void __iomem *base_addr, u32 hw_id, enum mcsc_shadow_ctrl ctrl)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_scaler0_set_shadow_ctrl(base_addr, ctrl);
		break;
	case DEV_HW_MCSC1:
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_shadow_ctrl(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SHADOW_REG_CTRL], 0x0);
		break;
	case DEV_HW_MCSC1:
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_input_status(void __iomem *base_addr, u32 hw_id, u32 *hl, u32 *vl)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		*hl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_0],	&mcsc_fields[MCSC_F_CUR_HORIZONTAL_CNT_0]);
		*vl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_0],	&mcsc_fields[MCSC_F_CUR_VERTICAL_CNT_0]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_input_source(void __iomem *base_addr, u32 hw_id, u32 rdma)
{
	/* not support */
}

u32 fimc_is_scaler_get_input_source(void __iomem *base_addr, u32 hw_id)
{
	int ret = 0;

	/* not support */

	return ret;
}

void fimc_is_scaler_set_dither(void __iomem *base_addr, u32 hw_id, bool dither_en)
{
	/* not support */
}

void fimc_is_scaler_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height)
{
	u32 reg_value = 0;

	switch (hw_id) {
	case DEV_HW_MCSC0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT0_IMG_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT0_IMG_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], reg_value);
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
	u32 input, enable_poly, enable_post, enable_dma;

	switch (output_id) {
		case MCSC_OUTPUT0:
			enable_poly = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_ENABLE]);
			enable_post = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC0_POST_SC_ENABLE]);
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE]);
			input = 0; /* only MCSC0 support this version */
			break;
		case MCSC_OUTPUT1:
			enable_poly = 0; /* output1 not exist scaler */
			enable_post = 0;
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE]);
			input = 0; /* only MCSC0 support this version */
			break;
		default:
			break;
	}
	dbg_hw(2, "[ID:%d]%s:[SRC:%d]->[OUT:%d], en(poly:%d, post:%d), dma(%d)\n",
		hw_id, __func__, input, output_id, enable_poly, enable_post, enable_dma);

	return (DEV_HW_MCSC0 + input);
}

void fimc_is_scaler_set_poly_scaler_enable(void __iomem *base_addr, u32 hw_id, u32 output_id, u32 enable)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_ENABLE], enable);
	case MCSC_OUTPUT1:
		/* not support */
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
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_src_size(void __iomem *base_addr, u32 output_id, u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_SRC_HPOS], pos_x);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_SRC_VPOS], pos_y);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_POS], reg_value);

		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_SRC_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_SRC_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		/* not support */
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
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		/* not support */
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
		/* not support */
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
		/* not support */
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
		/* not support */
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
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_h_coef(void __iomem *base_addr, u32 output_id, u32 h_sel)
{
	int index;
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		for (index = 0; index <= 8; index++) {
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0A + (8 * index)], h_coef_8tap[h_sel].h_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0B + (8 * index)], h_coef_8tap[h_sel].h_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0AB + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0C + (8 * index)], h_coef_8tap[h_sel].h_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0D + (8 * index)], h_coef_8tap[h_sel].h_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0CD + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0E + (8 * index)], h_coef_8tap[h_sel].h_coef_e[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0F + (8 * index)], h_coef_8tap[h_sel].h_coef_f[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0EF + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0G + (8 * index)], h_coef_8tap[h_sel].h_coef_g[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_H_COEFF_0H + (8 * index)], h_coef_8tap[h_sel].h_coef_h[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0GH + (4 * index)], reg_value);
		}
		break;
	case MCSC_OUTPUT1:
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_v_coef(void __iomem *base_addr, u32 output_id, u32 v_sel)
{
	int index;
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		for (index = 0; index <= 8; index++) {
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_V_COEFF_0A + (4 * index)], v_coef_4tap[v_sel].v_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_V_COEFF_0B + (4 * index)], v_coef_4tap[v_sel].v_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0AB + (2 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_V_COEFF_0C + (4 * index)], v_coef_4tap[v_sel].v_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC0_V_COEFF_0D + (4 * index)], v_coef_4tap[v_sel].v_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0CD + (2 * index)], reg_value);
		}
		break;
	case MCSC_OUTPUT1:
		/* not support */
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
	u32 h_coef = 0;
	u32 v_coef = 0;
	u32 h_phase_offset = 0; /* this value equals 0 - scale-down operation */
	u32 v_phase_offset = 0;

	/* adjust H coef */
	if (hratio <= RATIO_X8_8) { /* scale up case */
		h_coef = MCSC_COEFF_x8_8;
		if (hratio != RATIO_X8_8)
			h_phase_offset = hratio >> 1;
	} else if (hratio > RATIO_X8_8 && hratio <= RATIO_X7_8) {
		h_coef = MCSC_COEFF_x7_8;
	} else if (hratio > RATIO_X7_8 && hratio <= RATIO_X6_8) {
		h_coef = MCSC_COEFF_x6_8;
	} else if (hratio > RATIO_X6_8 && hratio <= RATIO_X5_8) {
		h_coef = MCSC_COEFF_x5_8;
	} else if (hratio > RATIO_X5_8 && hratio <= RATIO_X4_8) {
		h_coef = MCSC_COEFF_x4_8;
	} else if (hratio > RATIO_X4_8 && hratio <= RATIO_X3_8) {
		h_coef = MCSC_COEFF_x3_8;
	} else if (hratio > RATIO_X3_8 && hratio <= RATIO_X2_8) {
		h_coef = MCSC_COEFF_x2_8;
	} else {
		h_coef = MCSC_COEFF_x2_8;
	}

	/* adjust V coef */
	if (vratio <= RATIO_X8_8) {
		v_coef = MCSC_COEFF_x8_8;
		if (vratio != RATIO_X8_8)
			v_phase_offset = vratio >> 1;
	} else if (vratio > RATIO_X8_8 && vratio <= RATIO_X7_8) {
		v_coef = MCSC_COEFF_x7_8;
	} else if (vratio > RATIO_X7_8 && vratio <= RATIO_X6_8) {
		v_coef = MCSC_COEFF_x6_8;
	} else if (vratio > RATIO_X6_8 && vratio <= RATIO_X5_8) {
		v_coef = MCSC_COEFF_x5_8;
	} else if (vratio > RATIO_X5_8 && vratio <= RATIO_X4_8) {
		v_coef = MCSC_COEFF_x4_8;
	} else if (vratio > RATIO_X4_8 && vratio <= RATIO_X3_8) {
		v_coef = MCSC_COEFF_x3_8;
	} else if (vratio > RATIO_X3_8 && vratio <= RATIO_X2_8) {
		v_coef = MCSC_COEFF_x2_8;
	} else {
		v_coef = MCSC_COEFF_x2_8;
	}

	fimc_is_scaler_set_h_init_phase_offset(base_addr, output_id, h_phase_offset);
	fimc_is_scaler_set_v_init_phase_offset(base_addr, output_id, v_phase_offset);
	fimc_is_scaler_set_poly_scaler_h_coef(base_addr, output_id, h_coef);
	fimc_is_scaler_set_poly_scaler_v_coef(base_addr, output_id, v_coef);
}

void fimc_is_scaler_set_post_scaler_enable(void __iomem *base_addr, u32 output_id, u32 enable)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC0_POST_SC_ENABLE], enable);
		break;
	case MCSC_OUTPUT1:
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_img_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_POST_SC_IMG_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_POST_SC_IMG_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_IMG_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		/* not support */
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
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_dst_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_POST_SC_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_POST_SC_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_DST_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		/* not support */
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
		/* not support */
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
		/* not support */
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
		/* not support */
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

		/* default BCHS clamp value */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_CLAMP], 0xFF00FF00);
		break;
	case MCSC_OUTPUT1:
		/* not support */
		break;
	default:
		break;
	}
}

/* brightness/contrast control */
void fimc_is_scaler_set_b_c(void __iomem *base_addr, u32 output_id, u32 y_offset, u32 y_gain)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_YOFFSET], y_offset);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_YGAIN], y_gain);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_BC], reg_value);
		break;
	case MCSC_OUTPUT1:
		/* not support */
		break;
	default:
		break;
	}
}

/* hue/saturation control */
void fimc_is_scaler_set_h_s(void __iomem *base_addr, u32 output_id, u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_00], c_gain00);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_HS1], reg_value);
		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_10], c_gain10);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_C_GAIN_11], c_gain11);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_HS2], reg_value);
		break;
	case MCSC_OUTPUT1:
		/* not support */
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
		/* not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_pri(void __iomem *base_addr, u32 output_id, u32 plane)
{
	/* not support at this version */
}

void fimc_is_scaler_set_wdma_axi_pri(void __iomem *base_addr)
{
	/* not support at this version */
}

void fimc_is_scaler_set_wdma_sram_base(void __iomem *base_addr, u32 output_id)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0], &mcsc_fields[MCSC_F_WDMA0_SRAM_BASE], WDMA0_SRAM_BASE_VALUE);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0], &mcsc_fields[MCSC_F_WDMA1_SRAM_BASE], WDMA1_SRAM_BASE_VALUE);
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
		/* not support */
		break;
	default:
		break;
	}

	return ret;
}

void fimc_is_scaler_set_otf_out_path(void __iomem *base_addr, u32 output_id)
{
	/* not support */
}

void fimc_is_scaler_set_rdma_format(void __iomem *base_addr, u32 hw_id, u32 dma_in_format)
{
	/* not support */
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
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_format(void __iomem *base_addr, u32 output_id, u32 *dma_out_format)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*dma_out_format = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA0_DATA_FORMAT]);
		break;
	case MCSC_OUTPUT1:
		*dma_out_format = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA1_DATA_FORMAT]);
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
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_size(void __iomem *base_addr, u32 width, u32 height)
{
	/* not support */
}

void fimc_is_scaler_get_rdma_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	/* not support */
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
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_stride(void __iomem *base_addr, u32 y_stride, u32 uv_stride)
{
	/* not support */
}

void fimc_is_scaler_get_rdma_stride(void __iomem *base_addr, u32 *y_stride, u32 *uv_stride)
{
	/* not support */
}

void fimc_is_scaler_set_wdma_stride(void __iomem *base_addr, u32 output_id, u32 y_stride, u32 uv_stride)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_Y_STRIDE], y_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_C_STRIDE], uv_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_Y_STRIDE], y_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_C_STRIDE], uv_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE], reg_value);
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
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_frame_seq(void __iomem *base_addr, u32 frame_seq)
{
	/* not support */
}

void fimc_is_scaler_set_wdma_frame_seq(void __iomem *base_addr, u32 output_id, u32 frame_seq)
{
	/* not support */
}

void fimc_is_scaler_set_rdma_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_2], cr_addr);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_set_wdma0_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2], cr_addr);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_set_wdma1_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2], cr_addr);
		break;
	default:
		/* only index 0 support */
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
	default:
		break;
	}
}

static void fimc_is_scaler_get_wdma0_addr(void __iomem *base_addr, u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2]);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_get_wdma1_addr(void __iomem *base_addr, u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2]);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

void fimc_is_scaler_get_wdma_addr(void __iomem *base_addr, u32 output_id, u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_scaler_get_wdma0_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	case MCSC_OUTPUT1:
		fimc_is_scaler_get_wdma1_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_rdma_addr(void __iomem *base_addr)
{
	/* DMA Y address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_0], 0x0);

	/* DMA CB address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_1], 0x0);

	/* DMA CR address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_2], 0x0);
}

void fimc_is_scaler_clear_wdma_addr(void __iomem *base_addr, u32 output_id)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2], 0x0);
		break;
	case MCSC_OUTPUT1:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2], 0x0);
		break;
	default:
		break;
	}
}

/* for tdnr */
void fimc_is_scaler_set_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_addr, u32 cb_addr, u32 cr_addr)
{
	switch (type) {
	case TDNR_IMAGE:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_2], cr_addr);
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA2_BASE_ADDR_0], y_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type)
{
	switch (type) {
	case TDNR_IMAGE:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_0], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_BASE_ADDR_2], 0x0);
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA2_BASE_ADDR_0], 0x0);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_addr, u32 cb_addr, u32 cr_addr)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0], y_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_addr, u32 *cb_addr, u32 *cr_addr)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0], 0x0);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_WIDTH], &mcsc_fields[MCSC_F_WDMA2_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_HEIGHT], &mcsc_fields[MCSC_F_WDMA2_HEIGHT], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 *width, u32 *height)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_WIDTH], &mcsc_fields[MCSC_F_WDMA2_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_HEIGHT], &mcsc_fields[MCSC_F_WDMA2_HEIGHT]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_rdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height)
{
	switch (type) {
	case TDNR_IMAGE:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA1_WIDTH], &mcsc_fields[MCSC_F_RDMA1_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA1_HEIGHT], &mcsc_fields[MCSC_F_RDMA1_HEIGHT], height);
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA2_WIDTH], &mcsc_fields[MCSC_F_RDMA2_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA2_HEIGHT], &mcsc_fields[MCSC_F_RDMA2_HEIGHT], height);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_rdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_in_format)
{
	switch (type) {
	case TDNR_IMAGE:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA1_DATA_FORMAT], &mcsc_fields[MCSC_F_RDMA1_DATA_FORMAT], dma_in_format);
		break;
	case TDNR_WEIGHT:
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_wdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_out_format)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_stride, u32 uv_stride)
{
	u32 reg_value = 0;

	switch (type) {
	case TDNR_IMAGE:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMA1_Y_STRIDE], y_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMA1_C_STRIDE], uv_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMA1_STRIDE], reg_value);
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMA2_STRIDE], &mcsc_fields[MCSC_F_RDMA2_Y_STRIDE], y_stride);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_stride, u32 *uv_stride)
{
	switch (type) {
	case TDNR_IMAGE:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA1_STRIDE], &mcsc_fields[MCSC_F_RDMA1_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA1_STRIDE], &mcsc_fields[MCSC_F_RDMA1_C_STRIDE]);
		break;
	case TDNR_WEIGHT:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMA2_STRIDE], &mcsc_fields[MCSC_F_RDMA2_Y_STRIDE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_stride, u32 uv_stride)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], &mcsc_fields[MCSC_F_WDMA2_Y_STRIDE], y_stride);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_stride, u32 *uv_stride)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], &mcsc_fields[MCSC_F_WDMA2_Y_STRIDE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_wdma_sram_base(void __iomem *base_addr, enum tdnr_buf_type type)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0], &mcsc_fields[MCSC_F_WDMA2_SRAM_BASE], WDMA2_SRAM_BASE_VALUE);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_wdma_enable(void __iomem *base_addr, enum tdnr_buf_type type, bool dma_out_en)
{
	switch (type) {
	case TDNR_IMAGE:
		break;
	case TDNR_WEIGHT:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_tdnr_image_size(void __iomem *base_addr, u32 width, u32 height)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IMAGE_WIDTH], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IMAGE_HEIGHT], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_DIMENSIONS], reg_value);
}

void fimc_is_scaler_set_tdnr_mode_select(void __iomem *base_addr, enum tdnr_mode mode)
{
	u32 reg_value = 0;
	u32 tdnr_first = 0, tdnr_on = 0, tdnr_enable = 0;

	switch (mode) {
	case TDNR_MODE_BYPASS:
		tdnr_first = 1;
		tdnr_on = 1;
		tdnr_enable = 0;
		break;
	case TDNR_MODE_2DNR:
		tdnr_first = 1;
		tdnr_on = 0;
		tdnr_enable = 0;
		break;
	case TDNR_MODE_3DNR:
		tdnr_first = 0;
		tdnr_on = 1;
		tdnr_enable = 1;
	default:
		break;
	}

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_FIRST_FRAME], tdnr_first);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_ON], tdnr_on);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_MODE_SELECTION], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_ENABLE], &mcsc_fields[MCSC_F_TDNR_ENABLE], tdnr_enable);
}

void fimc_is_scaler_set_tdnr_first(void __iomem *base_addr, u32 tdnr_first)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALE_PATH_CTRL], &mcsc_fields[MCSC_F_TDNR_FIRST], tdnr_first);
}

void fimc_is_scaler_set_tdnr_tuneset_general(void __iomem *base_addr, struct general_config config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_AVG_CUR],
								(u32)config.use_average_current);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_AUTO_COEF_3D],
								(u32)config.auto_coeff_3d);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_TEMPORAL_WEIGHTS_CALCULATION_CONFIG], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_ST_BLENDING_TH], &mcsc_fields[MCSC_F_TDNR_ST_BLENDING_TH],
												config.blending_threshold);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_TEMP_BLEND_THRESH_CRITERION],
									(u32)config.temporal_blend_thresh_criterion);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_TEMP_BLEND_THRESH_W_T_MAX],
									(u32)config.temporal_blend_thresh_weight_max);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_TEMP_BLEND_THRESH_W_T_MIN],
									(u32)config.temporal_blend_thresh_weight_min);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_TEMPORAL_BLEND_THRESHOLDS], reg_value);
}

void fimc_is_scaler_set_tdnr_tuneset_yuvtable(void __iomem *base_addr, struct yuv_table_config config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_GRID_0], config.x_grid_y[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_GRID_1], config.x_grid_y[ARR3_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_X_GRID_Y], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_X_GRID_Y_1], &mcsc_fields[MCSC_F_TDNR_Y_X_GRID_2],
												config.x_grid_y[ARR3_VAL3]);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_Y_STD_OFFSET], &mcsc_fields[MCSC_F_TDNR_Y_STD_OFFSET],
												config.y_std_offset);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_SLOPE_0],
									config.y_std_slope[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_SLOPE_1],
									config.y_std_slope[ARR4_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_Y_STD_SLOPE], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_SLOPE_2],
									config.y_std_slope[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_SLOPE_3],
									config.y_std_slope[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_Y_STD_SLOPE_1], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_U_X_GRID_0],
									config.x_grid_u[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_U_X_GRID_1],
									config.x_grid_u[ARR3_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_X_GRID_U], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_X_GRID_U_1], &mcsc_fields[MCSC_F_TDNR_U_X_GRID_2],
									config.x_grid_u[ARR3_VAL3]);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_U_STD_OFFSET], &mcsc_fields[MCSC_F_TDNR_U_STD_OFFSET],
									config.u_std_offset);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_U_X_SLOPE_0],
									config.u_std_slope[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_U_X_SLOPE_1],
									config.u_std_slope[ARR4_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_U_STD_SLOPE], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_U_X_SLOPE_2],
									config.u_std_slope[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_U_X_SLOPE_3],
									config.u_std_slope[ARR4_VAL3]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_U_STD_SLOPE_1], reg_value);


	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_V_X_GRID_0],
									config.x_grid_v[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_V_X_GRID_1],
									config.x_grid_v[ARR3_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_X_GRID_V], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_X_GRID_V_1], &mcsc_fields[MCSC_F_TDNR_V_X_GRID_2],
									config.x_grid_v[ARR3_VAL3]);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_V_STD_OFFSET], &mcsc_fields[MCSC_F_TDNR_V_STD_OFFSET],
									config.v_std_offset);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_V_X_SLOPE_0],
									config.v_std_slope[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_V_X_SLOPE_1],
									config.v_std_slope[ARR4_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_V_STD_SLOPE], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_V_X_SLOPE_2],
									config.v_std_slope[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_V_X_SLOPE_3],
									config.v_std_slope[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_V_STD_SLOPE_1], reg_value);
}

void fimc_is_scaler_set_tdnr_tuneset_temporal(void __iomem *base_addr,
	struct temporal_ni_dep_config dep_config,
	struct temporal_ni_indep_config indep_config)
{
	u32 reg_value = 0;

	/* noise index depended config */
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_WGT1],
									dep_config.temporal_weight_coeff_y1);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_WGT2],
									dep_config.temporal_weight_coeff_y2);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_WGT1],
									dep_config.temporal_weight_coeff_uv1);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_WGT2],
									dep_config.temporal_weight_coeff_uv2);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_TEMPORAL_WEIGHTS_COEFFS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_GAIN1],
									dep_config.auto_lut_gains_y[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_GAIN2],
									dep_config.auto_lut_gains_y[ARR3_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_GAIN3],
									dep_config.auto_lut_gains_y[ARR3_VAL3]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_AUTO_LUT_Y_GAINS], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_Y_OFFSET], &mcsc_fields[MCSC_F_TDNR_Y_OFFSET],
									dep_config.y_offset);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_GAIN1],
									dep_config.auto_lut_gains_uv[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_GAIN2],
									dep_config.auto_lut_gains_uv[ARR3_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_GAIN3],
									dep_config.auto_lut_gains_uv[ARR3_VAL3]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_AUTO_LUT_UV_GAINS], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_UV_OFFSET], &mcsc_fields[MCSC_F_TDNR_UV_OFFSET]
									,dep_config.uv_offset);

	/* noise index independed config */
	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID1],
									indep_config.prev_gridx[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID2],
									indep_config.prev_gridx[ARR3_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID3],
									indep_config.prev_gridx[ARR3_VAL3]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_WEIGHT_ENCODER], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID1_LUT],
									indep_config.prev_gridx_lut[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID2_LUT],
									indep_config.prev_gridx_lut[ARR4_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID3_LUT],
									indep_config.prev_gridx_lut[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_XGRID4_LUT],
									indep_config.prev_gridx_lut[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_WEIGHT_DECODER], reg_value);

}

void fimc_is_scaler_set_tdnr_tuneset_constant_lut_coeffs(void __iomem *base_addr, u32 *config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_GRID_X1],
									config[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_GRID_X2],
									config[ARR3_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_CONSTANT_LUT_COEFFS], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_CONSTANT_LUT_COEFFS_1], &mcsc_fields[MCSC_F_TDNR_GRID_X3],
									config[ARR3_VAL3]);
}

void fimc_is_scaler_set_tdnr_tuneset_refine_control(void __iomem *base_addr,
	struct refine_control_config config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_WGT_REFINE_ON],
									(u32)config.is_refine_on);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_WGT_REFINE_MODE],
									config.refine_mode);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_REFINE_THR],
									config.refine_threshold);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_COEF_UPDATE],
									config.refine_coeff_update);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_TEMPORAL_WEIGHT_REFINE], reg_value);
}

void fimc_is_scaler_set_tdnr_tuneset_regional_feature(void __iomem *base_addr,
	struct regional_ni_dep_config dep_config,
	struct regional_ni_indep_config indep_config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_REGION_DIFF_ON],
									(u32)dep_config.is_region_diff_on);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IGNORE_REGION_SIGN],
									(u32)indep_config.dont_use_region_sign);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_DIFF_COND],
								(u32)indep_config.diff_condition_are_all_components_similar);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_DIFF_LINE_COND],
									(u32)indep_config.line_condition);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_MOTION_DETECT_MODE],
								(u32)indep_config.is_motiondetect_luma_mode_mean);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_Y_MOTION_FLAG_CONFIG], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_REGION_OFFSET],
								indep_config.region_offset);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_REGION_GAIN],
								dep_config.region_gain);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_Y_MOTION_FLAG], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_OTHER_CH_CHECK],
								(u32)dep_config.other_channels_check);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_MOTION_DETECT_MODE],
								(u32)indep_config.is_motiondetect_chroma_mode_mean);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_OTHER_CH_GAIN],
								dep_config.other_channel_gain);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_OTHER_CH_OFFSET],
								indep_config.other_channel_offset);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_UV_MOTION_FLAG], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_COEF_OFFSET], &mcsc_fields[MCSC_F_TDNR_COEF_OFFSET],
								indep_config.coefficient_offset);
}

void fimc_is_scaler_set_tdnr_tuneset_spatial(void __iomem *base_addr,
	struct spatial_ni_dep_config dep_config,
	struct spatial_ni_indep_config indep_config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_UV_2D_WEIGHT_MODE],
								dep_config.weight_mode);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_SEPERATE_WEIGHTS],
								(u32)dep_config.spatial_separate_weights);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_GAIN],
								dep_config.spatial_gain);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_REFINE_TH],
								indep_config.spatial_refine_threshold);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_DENOISING_CONFIG], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_GAIN1],
								dep_config.spatial_luma_gain[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_GAIN2],
								dep_config.spatial_luma_gain[ARR4_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_GAIN3],
								dep_config.spatial_luma_gain[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_GAIN4],
								dep_config.spatial_luma_gain[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_LUMA_GAINS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_OFFSET1],
								indep_config.spatial_luma_offset[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_OFFSET2],
								indep_config.spatial_luma_offset[ARR4_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_LUMA_OFFSETS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_OFFSET3],
								indep_config.spatial_luma_offset[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_LUMA_OFFSET4],
								indep_config.spatial_luma_offset[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_LUMA_OFFSETS_1], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_GAIN1],
								dep_config.spatial_uv_gain[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_GAIN2],
								dep_config.spatial_uv_gain[ARR4_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_GAIN3],
								dep_config.spatial_uv_gain[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_GAIN4],
								dep_config.spatial_uv_gain[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_UV_GAINS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_OFFSET1],
								indep_config.spatial_uv_offset[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_OFFSET2],
								indep_config.spatial_uv_offset[ARR4_VAL2]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_UV_OFFSETS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_OFFSET3],
								indep_config.spatial_uv_offset[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_2DNR_UV_OFFSET4],
								indep_config.spatial_uv_offset[ARR4_VAL4]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_SPATIAL_UV_OFFSETS_1], reg_value);
}

/* for hwfc */
void fimc_is_scaler_set_hwfc_auto_clear(void __iomem *base_addr, u32 output_id, bool auto_clear)
{
	/* not support */
}

void fimc_is_scaler_set_hwfc_idx_reset(void __iomem *base_addr, u32 output_id, bool reset)
{
	/* not support */
}

void fimc_is_scaler_set_hwfc_mode(void __iomem *base_addr, u32 hwfc_output_ids)
{
	/* not support */
}

void fimc_is_scaler_set_hwfc_config(void __iomem *base_addr,
	u32 output_id, u32 format, u32 plane, u32 dma_idx, u32 width, u32 height)
{
	/* not support */
}

u32 fimc_is_scaler_get_hwfc_idx_bin(void __iomem *base_addr, u32 output_id)
{
	/* not support */
	return 0;
}

u32 fimc_is_scaler_get_hwfc_cur_idx(void __iomem *base_addr, u32 output_id)
{
	/* not support */
	return 0;
}

static void fimc_is_scaler0_clear_intr_src(void __iomem *base_addr, u32 status)
{
	if (status & (1 << INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF);

	if (status & (1 << INTR_MC_SCALER_SHADOW_COPY_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_SHADOW_COPY_FINISH);

	if (status & (1 << INTR_MC_SCALER_OUTSTALL))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_OUTSTALL);

	if (status & (1 << INTR_MC_SCALER_OVERFLOW))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_OVERFLOW);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF);

	if (status & (1 << INTR_MC_SCALER_CORE_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_CORE_FINISH);

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_WDMA_FINISH);

	if (status & (1 << INTR_MC_SCALER_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_FRAME_START);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_FRAME_END);
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

u32 fimc_is_scaler_handle_extended_intr(u32 status)
{
	int ret = 0;

	if (status & (1 << INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF)) {
		err_hw("[MCSC]Shadow Register Copy Overflow!! (0x%x)", status);
		FIMC_BUG(1);

		/* TODO: Shadow copy overflow recovery logic */
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

	info_hw("MCSC ver 3.0");

	for(i = 0; i < MCSC_REG_CNT; i++) {
		reg_value = readl(base_addr + mcsc_regs[i].sfr_offset);
		sfrinfo("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			mcsc_regs[i].reg_name, mcsc_regs[i].sfr_offset, reg_value);
	}
}

void fimc_is_scaler_rdma_start(void __iomem *base_addr, u32 hw_id)
{
	/* Not Support */
}

void fimc_is_scaler_set_rdma_2bit_addr(void __iomem *base_addr,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	/* Not Support */
}

void fimc_is_scaler_set_rdma_2bit_stride(void __iomem *base_addr, u32 y_2bit_stride, u32 uv_2bit_stride)
{
	/* Not Support */
}

void fimc_is_scaler_set_rdma_10bit_type(void __iomem *base_addr, u32 dma_in_10bit_type)
{
	/* Not Support */
}

void fimc_is_scaler_set_wdma_10bit_type(void __iomem *base_addr, u32 output_id, u32 img_10bit_type)
{
	/* Not Support */
}

void fimc_is_scaler_set_wdma_2bit_addr(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	/* Not Support */
}

void fimc_is_scaler_set_wdma_2bit_stride(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_stride, u32 uv_2bit_stride)
{
	/* Not Support */
}

void fimc_is_scaler_get_ysum_result(void __iomem *base_addr, u32 *luma_sum_msb, u32 *luma_sum_lsb)
{
	/* Not Support */
}

void fimc_is_scaler_set_poly_round_mode(void __iomem *base_addr, u32 output_id, u32 mode)
{
	/* Not Support */
}

void fimc_is_scaler_set_post_scaler_coef(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	/* Not Support */
}

void fimc_is_scaler_set_post_round_mode(void __iomem *base_addr, u32 output_id, u32 mode)
{
	/* Not Support */
}

void fimc_is_scaler_set_ds_src_size(void __iomem *base_addr, u32 width, u32 height)
{
	/* Not Support */
}

void fimc_is_scaler_set_ds_dst_size(void __iomem *base_addr, u32 width, u32 height)
{
	/* Not Support */
}

void fimc_is_scaler_set_ds_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio)
{
	/* Not Support */
}

void fimc_is_scaler_set_ds_init_phase_offset(void __iomem *base_addr, u32 h_offset, u32 v_offset)
{
	/* Not Support */
}

void fimc_is_scaler_set_ds_gamma_table_enable(void __iomem *base_addr, u32 ds_gamma_enable)
{
	/* Not Support */
}

void fimc_is_scaler_set_ds_enable(void __iomem *base_addr, u32 ds_enable)
{
	/* Not Support */
}

void fimc_is_scaler_set_ysum_input_sourece_enable(void __iomem *base_addr, u32 output_id, bool ysum_enable)
{
	/* Not Support */
}

void fimc_is_scaler_set_ysum_enable(void __iomem *base_addr, bool ysum_enable)
{
	/* Not Support */
}

void fimc_is_scaler_set_ysum_image_size(void __iomem *base_addr, u32 width, u32 height, u32 start_x, u32 start_y)
{
	/* Not Support */
}
