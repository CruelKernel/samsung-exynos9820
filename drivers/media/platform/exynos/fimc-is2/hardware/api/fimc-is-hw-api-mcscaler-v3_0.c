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
#include "sfr/fimc-is-sfr-mcsc-v3_0.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-control.h"
#include "fimc-is-param.h"

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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_GCTRL_0], &mcsc_fields[MCSC_F_SCALER_ENABLE_0], 1);
		break;
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_GCTRL_1], &mcsc_fields[MCSC_F_SCALER_ENABLE_1], 1);
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
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_GCTRL_1], &mcsc_fields[MCSC_F_SCALER_ENABLE_1], 0);
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

static u32 fimc_is_scaler1_sw_reset(void __iomem *base_addr, u32 partial)
{
	u32 reset_count = 0;

	/* select chain1 partial reset */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_RESET_CTRL_1], &mcsc_fields[MCSC_F_PARTIAL_RESET_ENABLE_1], partial);

	/* request scaler reset */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_RESET_CTRL_1], &mcsc_fields[MCSC_F_SW_RESET_1], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_RESET_STATUS], &mcsc_fields[MCSC_F_SW_RESET_1_STATUS]) != 0);

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
	case DEV_HW_MCSC1:
		ret = fimc_is_scaler1_sw_reset(base_addr, partial);
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
	case DEV_HW_MCSC1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_OUT_STALL_BLOCKING_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_TRIGGER_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_HW_TRIGGER_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_CORE_FINISH_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA_FINISH_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_START_INT_1], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_END_INT_1], 1);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], reg_value);
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
	case DEV_HW_MCSC1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_OUT_STALL_BLOCKING_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_TRIGGER_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_HW_TRIGGER_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_CORE_FINISH_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA_FINISH_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_START_INT_1_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_END_INT_1_MASK], 1);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_1], reg_value);
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
	case DEV_HW_MCSC1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_1], intr_mask);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_shadow_ctrl(void __iomem *base_addr, u32 hw_id, enum mcsc_shadow_ctrl ctrl)
{
	/* not used */
}

void fimc_is_scaler_clear_shadow_ctrl(void __iomem *base_addr, u32 hw_id)
{
	/* not used */
}

void fimc_is_scaler_set_stop_req_post_en_ctrl(void __iomem *base_addr, u32 hw_id, u32 value)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_STOP_REQ_POST_EN_CTRL_0], &mcsc_fields[MCSC_F_STOP_REQ_POST_EN_CTRL_TYPE_0], value);
		break;
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_STOP_REQ_POST_EN_CTRL_1], &mcsc_fields[MCSC_F_STOP_REQ_POST_EN_CTRL_TYPE_1], value);
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
		*hl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_0],	&mcsc_fields[MCSC_F_CUR_HORIZONTAL_CNT_0]);
		*vl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_0],	&mcsc_fields[MCSC_F_CUR_VERTICAL_CNT_0]);
		break;
	case DEV_HW_MCSC1:
		*hl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_1], &mcsc_fields[MCSC_F_CUR_HORIZONTAL_CNT_1]);
		*vl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS_1], &mcsc_fields[MCSC_F_CUR_VERTICAL_CNT_1]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_input_source(void __iomem *base_addr, u32 hw_id, u32 rdma)
{
	/*  0: otf input, 1: rdma input */
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT_SRC0_CTRL], &mcsc_fields[MCSC_F_INPUT_SRC0_SEL], rdma);
		break;
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT_SRC1_CTRL], &mcsc_fields[MCSC_F_INPUT_SRC1_SEL], rdma);
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
	case DEV_HW_MCSC1:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_INPUT_SRC1_CTRL], &mcsc_fields[MCSC_F_INPUT_SRC1_SEL]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

	return ret;
}

void fimc_is_scaler_set_dither(void __iomem *base_addr, u32 hw_id, bool dither_en)
{
	dbg_hw(2, "invalid hw_id(%d) for MCSC api\n", hw_id);
}

void fimc_is_scaler_set_input_img_size(void __iomem *base_addr, u32 hw_id, u32 width, u32 height)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT0_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT0_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT0_IMG_VSIZE], height);
		break;
	case DEV_HW_MCSC1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT1_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT1_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_INPUT1_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT1_IMG_VSIZE], height);
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
	case DEV_HW_MCSC1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_INPUT1_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT1_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_INPUT1_IMG_SIZE], &mcsc_fields[MCSC_F_INPUT1_IMG_VSIZE]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

}

u32 fimc_is_scaler_get_scaler_path(void __iomem *base_addr, u32 hw_id, u32 output_id)
{
	u32 input = 0, enable_poly, enable_post, enable_dma;

	switch (output_id) {
		case MCSC_OUTPUT0:
			enable_poly = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_ENABLE]);
			enable_post = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC0_POST_SC_ENABLE]);
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE]);
			input = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC0_INPUT_SEL]);
			break;
		case MCSC_OUTPUT1:
			enable_poly = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_CTRL], &mcsc_fields[MCSC_F_SC1_ENABLE]);
			enable_post = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC1_POST_SC_ENABLE]);
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE]);
			input = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC1_INPUT_SEL]);
			break;
		case MCSC_OUTPUT2:
			enable_poly = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_CTRL], &mcsc_fields[MCSC_F_SC2_ENABLE]);
			enable_post = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC2_POST_SC_ENABLE]);
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE]);
			input = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC2_INPUT_SEL]);
			break;
		case MCSC_OUTPUT3:
			enable_poly = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC3_CTRL], &mcsc_fields[MCSC_F_SC3_ENABLE]);
			enable_post = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC3_POST_SC_ENABLE]);
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC3_DMA_OUT_ENABLE]);
			input = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC3_INPUT_SEL]);
			break;
		case MCSC_OUTPUT4:
			enable_poly = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC4_CTRL], &mcsc_fields[MCSC_F_SC4_ENABLE]);
			enable_post = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC4_POST_SC_ENABLE]);
			enable_dma = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC4_DMA_OUT_ENABLE]);
			input = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC4_INPUT_SEL]);
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
	u32 input_source = 0;

	switch (hw_id) {
	case DEV_HW_MCSC0:
		input_source = 0;
		break;
	case DEV_HW_MCSC1:
		input_source = 1;
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		hw_id = 0; /* TODO: select proper input path */
		break;
	}

	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_ENABLE], enable);
		if (enable)
			fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC0_INPUT_SEL], input_source);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_CTRL], &mcsc_fields[MCSC_F_SC1_ENABLE], enable);
		if (enable)
			fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC1_INPUT_SEL], input_source);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_CTRL], &mcsc_fields[MCSC_F_SC2_ENABLE], enable);
		if (enable)
			fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC2_INPUT_SEL], input_source);
		break;
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_CTRL], &mcsc_fields[MCSC_F_SC3_ENABLE], enable);
		if (enable)
			fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC3_INPUT_SEL], input_source);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_CTRL], &mcsc_fields[MCSC_F_SC4_ENABLE], enable);
		if (enable)
			fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_PATH_CTRL], &mcsc_fields[MCSC_F_SC4_INPUT_SEL], input_source);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_CTRL], &mcsc_fields[MCSC_F_SC3_BYPASS], bypass);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_CTRL], &mcsc_fields[MCSC_F_SC4_BYPASS], bypass);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_SRC_POS], &mcsc_fields[MCSC_F_SC3_SRC_HPOS], pos_x);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_SRC_POS], &mcsc_fields[MCSC_F_SC3_SRC_VPOS], pos_y);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_SRC_SIZE], &mcsc_fields[MCSC_F_SC3_SRC_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_SRC_SIZE], &mcsc_fields[MCSC_F_SC3_SRC_VSIZE], height);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_SRC_POS], &mcsc_fields[MCSC_F_SC4_SRC_HPOS], pos_x);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_SRC_POS], &mcsc_fields[MCSC_F_SC4_SRC_VPOS], pos_y);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_SRC_SIZE], &mcsc_fields[MCSC_F_SC4_SRC_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_SRC_SIZE], &mcsc_fields[MCSC_F_SC4_SRC_VSIZE], height);
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
	case MCSC_OUTPUT3:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC3_SRC_SIZE], &mcsc_fields[MCSC_F_SC3_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC3_SRC_SIZE], &mcsc_fields[MCSC_F_SC3_SRC_VSIZE]);
		break;
	case MCSC_OUTPUT4:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC4_SRC_SIZE], &mcsc_fields[MCSC_F_SC4_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC4_SRC_SIZE], &mcsc_fields[MCSC_F_SC4_SRC_VSIZE]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_DST_SIZE], &mcsc_fields[MCSC_F_SC3_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_DST_SIZE], &mcsc_fields[MCSC_F_SC3_DST_VSIZE], height);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_DST_SIZE], &mcsc_fields[MCSC_F_SC4_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_DST_SIZE], &mcsc_fields[MCSC_F_SC4_DST_VSIZE], height);
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
	case MCSC_OUTPUT3:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC3_DST_SIZE], &mcsc_fields[MCSC_F_SC3_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC3_DST_SIZE], &mcsc_fields[MCSC_F_SC3_DST_VSIZE]);
		break;
	case MCSC_OUTPUT4:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC4_DST_SIZE], &mcsc_fields[MCSC_F_SC4_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC4_DST_SIZE], &mcsc_fields[MCSC_F_SC4_DST_VSIZE]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_RATIO], &mcsc_fields[MCSC_F_SC3_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_RATIO], &mcsc_fields[MCSC_F_SC3_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_RATIO], &mcsc_fields[MCSC_F_SC4_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_RATIO], &mcsc_fields[MCSC_F_SC4_V_RATIO], vratio);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC3_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC4_H_INIT_PHASE_OFFSET], h_offset);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC3_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_INIT_PHASE_OFFSET], &mcsc_fields[MCSC_F_SC4_V_INIT_PHASE_OFFSET], v_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_h_coef(void __iomem *base_addr, u32 output_id, u32 h_sel)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_0A], h_coef_8tap[h_sel].h_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_0B], h_coef_8tap[h_sel].h_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_0C], h_coef_8tap[h_sel].h_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_0D], h_coef_8tap[h_sel].h_coef_d[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_0E], h_coef_8tap[h_sel].h_coef_e[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_0F], h_coef_8tap[h_sel].h_coef_f[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_0G], h_coef_8tap[h_sel].h_coef_g[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_0H], h_coef_8tap[h_sel].h_coef_h[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_1A], h_coef_8tap[h_sel].h_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_1B], h_coef_8tap[h_sel].h_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_1C], h_coef_8tap[h_sel].h_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_1D], h_coef_8tap[h_sel].h_coef_d[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_1E], h_coef_8tap[h_sel].h_coef_e[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_1F], h_coef_8tap[h_sel].h_coef_f[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_1G], h_coef_8tap[h_sel].h_coef_g[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_1H], h_coef_8tap[h_sel].h_coef_h[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_2A], h_coef_8tap[h_sel].h_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_2B], h_coef_8tap[h_sel].h_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_2C], h_coef_8tap[h_sel].h_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_2D], h_coef_8tap[h_sel].h_coef_d[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_2E], h_coef_8tap[h_sel].h_coef_e[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_2F], h_coef_8tap[h_sel].h_coef_f[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_2G], h_coef_8tap[h_sel].h_coef_g[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_2H], h_coef_8tap[h_sel].h_coef_h[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_3A], h_coef_8tap[h_sel].h_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_3B], h_coef_8tap[h_sel].h_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_3C], h_coef_8tap[h_sel].h_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_3D], h_coef_8tap[h_sel].h_coef_d[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_3E], h_coef_8tap[h_sel].h_coef_e[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_3F], h_coef_8tap[h_sel].h_coef_f[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_3G], h_coef_8tap[h_sel].h_coef_g[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_3H], h_coef_8tap[h_sel].h_coef_h[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_4A], h_coef_8tap[h_sel].h_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_4B], h_coef_8tap[h_sel].h_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_4C], h_coef_8tap[h_sel].h_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_4D], h_coef_8tap[h_sel].h_coef_d[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_4E], h_coef_8tap[h_sel].h_coef_e[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_4F], h_coef_8tap[h_sel].h_coef_f[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_4G], h_coef_8tap[h_sel].h_coef_g[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_4H], h_coef_8tap[h_sel].h_coef_h[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_5A], h_coef_8tap[h_sel].h_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_5B], h_coef_8tap[h_sel].h_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_5C], h_coef_8tap[h_sel].h_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_5D], h_coef_8tap[h_sel].h_coef_d[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_5E], h_coef_8tap[h_sel].h_coef_e[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_5F], h_coef_8tap[h_sel].h_coef_f[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_5G], h_coef_8tap[h_sel].h_coef_g[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_5H], h_coef_8tap[h_sel].h_coef_h[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_6A], h_coef_8tap[h_sel].h_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_6B], h_coef_8tap[h_sel].h_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_6C], h_coef_8tap[h_sel].h_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_6D], h_coef_8tap[h_sel].h_coef_d[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_6E], h_coef_8tap[h_sel].h_coef_e[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_6F], h_coef_8tap[h_sel].h_coef_f[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_6G], h_coef_8tap[h_sel].h_coef_g[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_6H], h_coef_8tap[h_sel].h_coef_h[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_7A], h_coef_8tap[h_sel].h_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_7B], h_coef_8tap[h_sel].h_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_7C], h_coef_8tap[h_sel].h_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_7D], h_coef_8tap[h_sel].h_coef_d[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_7E], h_coef_8tap[h_sel].h_coef_e[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_7F], h_coef_8tap[h_sel].h_coef_f[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_7G], h_coef_8tap[h_sel].h_coef_g[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_7H], h_coef_8tap[h_sel].h_coef_h[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_8A], h_coef_8tap[h_sel].h_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC0_H_COEFF_8B], h_coef_8tap[h_sel].h_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_8C], h_coef_8tap[h_sel].h_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC0_H_COEFF_8D], h_coef_8tap[h_sel].h_coef_d[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_8E], h_coef_8tap[h_sel].h_coef_e[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC0_H_COEFF_8F], h_coef_8tap[h_sel].h_coef_f[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_8G], h_coef_8tap[h_sel].h_coef_g[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC0_H_COEFF_8H], h_coef_8tap[h_sel].h_coef_h[8]);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_0A], h_coef_8tap[h_sel].h_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_0B], h_coef_8tap[h_sel].h_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_0C], h_coef_8tap[h_sel].h_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_0D], h_coef_8tap[h_sel].h_coef_d[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_0E], h_coef_8tap[h_sel].h_coef_e[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_0F], h_coef_8tap[h_sel].h_coef_f[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_0G], h_coef_8tap[h_sel].h_coef_g[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_0H], h_coef_8tap[h_sel].h_coef_h[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_1A], h_coef_8tap[h_sel].h_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_1B], h_coef_8tap[h_sel].h_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_1C], h_coef_8tap[h_sel].h_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_1D], h_coef_8tap[h_sel].h_coef_d[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_1E], h_coef_8tap[h_sel].h_coef_e[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_1F], h_coef_8tap[h_sel].h_coef_f[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_1G], h_coef_8tap[h_sel].h_coef_g[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_1H], h_coef_8tap[h_sel].h_coef_h[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_2A], h_coef_8tap[h_sel].h_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_2B], h_coef_8tap[h_sel].h_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_2C], h_coef_8tap[h_sel].h_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_2D], h_coef_8tap[h_sel].h_coef_d[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_2E], h_coef_8tap[h_sel].h_coef_e[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_2F], h_coef_8tap[h_sel].h_coef_f[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_2G], h_coef_8tap[h_sel].h_coef_g[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_2H], h_coef_8tap[h_sel].h_coef_h[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_3A], h_coef_8tap[h_sel].h_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_3B], h_coef_8tap[h_sel].h_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_3C], h_coef_8tap[h_sel].h_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_3D], h_coef_8tap[h_sel].h_coef_d[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_3E], h_coef_8tap[h_sel].h_coef_e[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_3F], h_coef_8tap[h_sel].h_coef_f[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_3G], h_coef_8tap[h_sel].h_coef_g[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_3H], h_coef_8tap[h_sel].h_coef_h[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_4A], h_coef_8tap[h_sel].h_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_4B], h_coef_8tap[h_sel].h_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_4C], h_coef_8tap[h_sel].h_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_4D], h_coef_8tap[h_sel].h_coef_d[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_4E], h_coef_8tap[h_sel].h_coef_e[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_4F], h_coef_8tap[h_sel].h_coef_f[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_4G], h_coef_8tap[h_sel].h_coef_g[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_4H], h_coef_8tap[h_sel].h_coef_h[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_5A], h_coef_8tap[h_sel].h_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_5B], h_coef_8tap[h_sel].h_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_5C], h_coef_8tap[h_sel].h_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_5D], h_coef_8tap[h_sel].h_coef_d[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_5E], h_coef_8tap[h_sel].h_coef_e[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_5F], h_coef_8tap[h_sel].h_coef_f[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_5G], h_coef_8tap[h_sel].h_coef_g[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_5H], h_coef_8tap[h_sel].h_coef_h[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_6A], h_coef_8tap[h_sel].h_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_6B], h_coef_8tap[h_sel].h_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_6C], h_coef_8tap[h_sel].h_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_6D], h_coef_8tap[h_sel].h_coef_d[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_6E], h_coef_8tap[h_sel].h_coef_e[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_6F], h_coef_8tap[h_sel].h_coef_f[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_6G], h_coef_8tap[h_sel].h_coef_g[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_6H], h_coef_8tap[h_sel].h_coef_h[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_7A], h_coef_8tap[h_sel].h_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_7B], h_coef_8tap[h_sel].h_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_7C], h_coef_8tap[h_sel].h_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_7D], h_coef_8tap[h_sel].h_coef_d[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_7E], h_coef_8tap[h_sel].h_coef_e[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_7F], h_coef_8tap[h_sel].h_coef_f[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_7G], h_coef_8tap[h_sel].h_coef_g[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_7H], h_coef_8tap[h_sel].h_coef_h[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_8A], h_coef_8tap[h_sel].h_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC1_H_COEFF_8B], h_coef_8tap[h_sel].h_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_8C], h_coef_8tap[h_sel].h_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC1_H_COEFF_8D], h_coef_8tap[h_sel].h_coef_d[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_8E], h_coef_8tap[h_sel].h_coef_e[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC1_H_COEFF_8F], h_coef_8tap[h_sel].h_coef_f[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_8G], h_coef_8tap[h_sel].h_coef_g[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC1_H_COEFF_8H], h_coef_8tap[h_sel].h_coef_h[8]);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_0A], h_coef_8tap[h_sel].h_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_0B], h_coef_8tap[h_sel].h_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_0C], h_coef_8tap[h_sel].h_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_0D], h_coef_8tap[h_sel].h_coef_d[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_0E], h_coef_8tap[h_sel].h_coef_e[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_0F], h_coef_8tap[h_sel].h_coef_f[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_0G], h_coef_8tap[h_sel].h_coef_g[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_0H], h_coef_8tap[h_sel].h_coef_h[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_1A], h_coef_8tap[h_sel].h_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_1B], h_coef_8tap[h_sel].h_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_1C], h_coef_8tap[h_sel].h_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_1D], h_coef_8tap[h_sel].h_coef_d[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_1E], h_coef_8tap[h_sel].h_coef_e[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_1F], h_coef_8tap[h_sel].h_coef_f[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_1G], h_coef_8tap[h_sel].h_coef_g[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_1H], h_coef_8tap[h_sel].h_coef_h[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_2A], h_coef_8tap[h_sel].h_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_2B], h_coef_8tap[h_sel].h_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_2C], h_coef_8tap[h_sel].h_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_2D], h_coef_8tap[h_sel].h_coef_d[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_2E], h_coef_8tap[h_sel].h_coef_e[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_2F], h_coef_8tap[h_sel].h_coef_f[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_2G], h_coef_8tap[h_sel].h_coef_g[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_2H], h_coef_8tap[h_sel].h_coef_h[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_3A], h_coef_8tap[h_sel].h_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_3B], h_coef_8tap[h_sel].h_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_3C], h_coef_8tap[h_sel].h_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_3D], h_coef_8tap[h_sel].h_coef_d[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_3E], h_coef_8tap[h_sel].h_coef_e[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_3F], h_coef_8tap[h_sel].h_coef_f[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_3G], h_coef_8tap[h_sel].h_coef_g[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_3H], h_coef_8tap[h_sel].h_coef_h[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_4A], h_coef_8tap[h_sel].h_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_4B], h_coef_8tap[h_sel].h_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_4C], h_coef_8tap[h_sel].h_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_4D], h_coef_8tap[h_sel].h_coef_d[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_4E], h_coef_8tap[h_sel].h_coef_e[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_4F], h_coef_8tap[h_sel].h_coef_f[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_4G], h_coef_8tap[h_sel].h_coef_g[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_4H], h_coef_8tap[h_sel].h_coef_h[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_5A], h_coef_8tap[h_sel].h_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_5B], h_coef_8tap[h_sel].h_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_5C], h_coef_8tap[h_sel].h_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_5D], h_coef_8tap[h_sel].h_coef_d[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_5E], h_coef_8tap[h_sel].h_coef_e[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_5F], h_coef_8tap[h_sel].h_coef_f[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_5G], h_coef_8tap[h_sel].h_coef_g[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_5H], h_coef_8tap[h_sel].h_coef_h[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_6A], h_coef_8tap[h_sel].h_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_6B], h_coef_8tap[h_sel].h_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_6C], h_coef_8tap[h_sel].h_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_6D], h_coef_8tap[h_sel].h_coef_d[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_6E], h_coef_8tap[h_sel].h_coef_e[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_6F], h_coef_8tap[h_sel].h_coef_f[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_6G], h_coef_8tap[h_sel].h_coef_g[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_6H], h_coef_8tap[h_sel].h_coef_h[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_7A], h_coef_8tap[h_sel].h_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_7B], h_coef_8tap[h_sel].h_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_7C], h_coef_8tap[h_sel].h_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_7D], h_coef_8tap[h_sel].h_coef_d[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_7E], h_coef_8tap[h_sel].h_coef_e[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_7F], h_coef_8tap[h_sel].h_coef_f[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_7G], h_coef_8tap[h_sel].h_coef_g[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_7H], h_coef_8tap[h_sel].h_coef_h[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_8A], h_coef_8tap[h_sel].h_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC2_H_COEFF_8B], h_coef_8tap[h_sel].h_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_8C], h_coef_8tap[h_sel].h_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC2_H_COEFF_8D], h_coef_8tap[h_sel].h_coef_d[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_8E], h_coef_8tap[h_sel].h_coef_e[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC2_H_COEFF_8F], h_coef_8tap[h_sel].h_coef_f[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_8G], h_coef_8tap[h_sel].h_coef_g[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC2_H_COEFF_8H], h_coef_8tap[h_sel].h_coef_h[8]);
		break;
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_0A], h_coef_8tap[h_sel].h_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_0B], h_coef_8tap[h_sel].h_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_0C], h_coef_8tap[h_sel].h_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_0D], h_coef_8tap[h_sel].h_coef_d[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_0E], h_coef_8tap[h_sel].h_coef_e[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_0F], h_coef_8tap[h_sel].h_coef_f[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_0G], h_coef_8tap[h_sel].h_coef_g[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_0H], h_coef_8tap[h_sel].h_coef_h[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_1A], h_coef_8tap[h_sel].h_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_1B], h_coef_8tap[h_sel].h_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_1C], h_coef_8tap[h_sel].h_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_1D], h_coef_8tap[h_sel].h_coef_d[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_1E], h_coef_8tap[h_sel].h_coef_e[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_1F], h_coef_8tap[h_sel].h_coef_f[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_1G], h_coef_8tap[h_sel].h_coef_g[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_1H], h_coef_8tap[h_sel].h_coef_h[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_2A], h_coef_8tap[h_sel].h_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_2B], h_coef_8tap[h_sel].h_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_2C], h_coef_8tap[h_sel].h_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_2D], h_coef_8tap[h_sel].h_coef_d[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_2E], h_coef_8tap[h_sel].h_coef_e[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_2F], h_coef_8tap[h_sel].h_coef_f[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_2G], h_coef_8tap[h_sel].h_coef_g[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_2H], h_coef_8tap[h_sel].h_coef_h[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_3A], h_coef_8tap[h_sel].h_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_3B], h_coef_8tap[h_sel].h_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_3C], h_coef_8tap[h_sel].h_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_3D], h_coef_8tap[h_sel].h_coef_d[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_3E], h_coef_8tap[h_sel].h_coef_e[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_3F], h_coef_8tap[h_sel].h_coef_f[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_3G], h_coef_8tap[h_sel].h_coef_g[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_3H], h_coef_8tap[h_sel].h_coef_h[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_4A], h_coef_8tap[h_sel].h_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_4B], h_coef_8tap[h_sel].h_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_4C], h_coef_8tap[h_sel].h_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_4D], h_coef_8tap[h_sel].h_coef_d[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_4E], h_coef_8tap[h_sel].h_coef_e[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_4F], h_coef_8tap[h_sel].h_coef_f[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_4G], h_coef_8tap[h_sel].h_coef_g[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_4H], h_coef_8tap[h_sel].h_coef_h[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_5A], h_coef_8tap[h_sel].h_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_5B], h_coef_8tap[h_sel].h_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_5C], h_coef_8tap[h_sel].h_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_5D], h_coef_8tap[h_sel].h_coef_d[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_5E], h_coef_8tap[h_sel].h_coef_e[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_5F], h_coef_8tap[h_sel].h_coef_f[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_5G], h_coef_8tap[h_sel].h_coef_g[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_5H], h_coef_8tap[h_sel].h_coef_h[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_6A], h_coef_8tap[h_sel].h_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_6B], h_coef_8tap[h_sel].h_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_6C], h_coef_8tap[h_sel].h_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_6D], h_coef_8tap[h_sel].h_coef_d[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_6E], h_coef_8tap[h_sel].h_coef_e[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_6F], h_coef_8tap[h_sel].h_coef_f[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_6G], h_coef_8tap[h_sel].h_coef_g[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_6H], h_coef_8tap[h_sel].h_coef_h[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_7A], h_coef_8tap[h_sel].h_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_7B], h_coef_8tap[h_sel].h_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_7C], h_coef_8tap[h_sel].h_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_7D], h_coef_8tap[h_sel].h_coef_d[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_7E], h_coef_8tap[h_sel].h_coef_e[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_7F], h_coef_8tap[h_sel].h_coef_f[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_7G], h_coef_8tap[h_sel].h_coef_g[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_7H], h_coef_8tap[h_sel].h_coef_h[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_8A], h_coef_8tap[h_sel].h_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC3_H_COEFF_8B], h_coef_8tap[h_sel].h_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_8C], h_coef_8tap[h_sel].h_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC3_H_COEFF_8D], h_coef_8tap[h_sel].h_coef_d[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_8E], h_coef_8tap[h_sel].h_coef_e[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC3_H_COEFF_8F], h_coef_8tap[h_sel].h_coef_f[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_8G], h_coef_8tap[h_sel].h_coef_g[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC3_H_COEFF_8H], h_coef_8tap[h_sel].h_coef_h[8]);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_0A], h_coef_8tap[h_sel].h_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_0B], h_coef_8tap[h_sel].h_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_0C], h_coef_8tap[h_sel].h_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_0D], h_coef_8tap[h_sel].h_coef_d[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_0E], h_coef_8tap[h_sel].h_coef_e[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_0F], h_coef_8tap[h_sel].h_coef_f[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_0G], h_coef_8tap[h_sel].h_coef_g[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_0GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_0H], h_coef_8tap[h_sel].h_coef_h[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_1A], h_coef_8tap[h_sel].h_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_1B], h_coef_8tap[h_sel].h_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_1C], h_coef_8tap[h_sel].h_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_1D], h_coef_8tap[h_sel].h_coef_d[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_1E], h_coef_8tap[h_sel].h_coef_e[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_1F], h_coef_8tap[h_sel].h_coef_f[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_1G], h_coef_8tap[h_sel].h_coef_g[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_1GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_1H], h_coef_8tap[h_sel].h_coef_h[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_2A], h_coef_8tap[h_sel].h_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_2B], h_coef_8tap[h_sel].h_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_2C], h_coef_8tap[h_sel].h_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_2D], h_coef_8tap[h_sel].h_coef_d[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_2E], h_coef_8tap[h_sel].h_coef_e[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_2F], h_coef_8tap[h_sel].h_coef_f[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_2G], h_coef_8tap[h_sel].h_coef_g[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_2GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_2H], h_coef_8tap[h_sel].h_coef_h[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_3A], h_coef_8tap[h_sel].h_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_3B], h_coef_8tap[h_sel].h_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_3C], h_coef_8tap[h_sel].h_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_3D], h_coef_8tap[h_sel].h_coef_d[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_3E], h_coef_8tap[h_sel].h_coef_e[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_3F], h_coef_8tap[h_sel].h_coef_f[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_3G], h_coef_8tap[h_sel].h_coef_g[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_3GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_3H], h_coef_8tap[h_sel].h_coef_h[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_4A], h_coef_8tap[h_sel].h_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_4B], h_coef_8tap[h_sel].h_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_4C], h_coef_8tap[h_sel].h_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_4D], h_coef_8tap[h_sel].h_coef_d[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_4E], h_coef_8tap[h_sel].h_coef_e[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_4F], h_coef_8tap[h_sel].h_coef_f[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_4G], h_coef_8tap[h_sel].h_coef_g[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_4GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_4H], h_coef_8tap[h_sel].h_coef_h[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_5A], h_coef_8tap[h_sel].h_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_5B], h_coef_8tap[h_sel].h_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_5C], h_coef_8tap[h_sel].h_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_5D], h_coef_8tap[h_sel].h_coef_d[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_5E], h_coef_8tap[h_sel].h_coef_e[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_5F], h_coef_8tap[h_sel].h_coef_f[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_5G], h_coef_8tap[h_sel].h_coef_g[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_5GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_5H], h_coef_8tap[h_sel].h_coef_h[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_6A], h_coef_8tap[h_sel].h_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_6B], h_coef_8tap[h_sel].h_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_6C], h_coef_8tap[h_sel].h_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_6D], h_coef_8tap[h_sel].h_coef_d[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_6E], h_coef_8tap[h_sel].h_coef_e[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_6F], h_coef_8tap[h_sel].h_coef_f[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_6G], h_coef_8tap[h_sel].h_coef_g[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_6GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_6H], h_coef_8tap[h_sel].h_coef_h[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_7A], h_coef_8tap[h_sel].h_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_7B], h_coef_8tap[h_sel].h_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_7C], h_coef_8tap[h_sel].h_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_7D], h_coef_8tap[h_sel].h_coef_d[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_7E], h_coef_8tap[h_sel].h_coef_e[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_7F], h_coef_8tap[h_sel].h_coef_f[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_7G], h_coef_8tap[h_sel].h_coef_g[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_7GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_7H], h_coef_8tap[h_sel].h_coef_h[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_8A], h_coef_8tap[h_sel].h_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8AB], &mcsc_fields[MCSC_F_SC4_H_COEFF_8B], h_coef_8tap[h_sel].h_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_8C], h_coef_8tap[h_sel].h_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8CD], &mcsc_fields[MCSC_F_SC4_H_COEFF_8D], h_coef_8tap[h_sel].h_coef_d[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_8E], h_coef_8tap[h_sel].h_coef_e[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8EF], &mcsc_fields[MCSC_F_SC4_H_COEFF_8F], h_coef_8tap[h_sel].h_coef_f[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_8G], h_coef_8tap[h_sel].h_coef_g[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_H_COEFF_8GH], &mcsc_fields[MCSC_F_SC4_H_COEFF_8H], h_coef_8tap[h_sel].h_coef_h[8]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_v_coef(void __iomem *base_addr, u32 output_id, u32 v_sel)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_0A], v_coef_4tap[v_sel].v_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_0B], v_coef_4tap[v_sel].v_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_0C], v_coef_4tap[v_sel].v_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_0D], v_coef_4tap[v_sel].v_coef_d[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_1A], v_coef_4tap[v_sel].v_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_1B], v_coef_4tap[v_sel].v_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_1C], v_coef_4tap[v_sel].v_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_1D], v_coef_4tap[v_sel].v_coef_d[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_2A], v_coef_4tap[v_sel].v_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_2B], v_coef_4tap[v_sel].v_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_2C], v_coef_4tap[v_sel].v_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_2D], v_coef_4tap[v_sel].v_coef_d[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_3A], v_coef_4tap[v_sel].v_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_3B], v_coef_4tap[v_sel].v_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_3C], v_coef_4tap[v_sel].v_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_3D], v_coef_4tap[v_sel].v_coef_d[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_4A], v_coef_4tap[v_sel].v_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_4B], v_coef_4tap[v_sel].v_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_4C], v_coef_4tap[v_sel].v_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_4D], v_coef_4tap[v_sel].v_coef_d[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_5A], v_coef_4tap[v_sel].v_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_5B], v_coef_4tap[v_sel].v_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_5C], v_coef_4tap[v_sel].v_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_5D], v_coef_4tap[v_sel].v_coef_d[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_6A], v_coef_4tap[v_sel].v_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_6B], v_coef_4tap[v_sel].v_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_6C], v_coef_4tap[v_sel].v_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_6D], v_coef_4tap[v_sel].v_coef_d[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_7A], v_coef_4tap[v_sel].v_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_7B], v_coef_4tap[v_sel].v_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_7C], v_coef_4tap[v_sel].v_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_7D], v_coef_4tap[v_sel].v_coef_d[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_8A], v_coef_4tap[v_sel].v_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC0_V_COEFF_8B], v_coef_4tap[v_sel].v_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_8C], v_coef_4tap[v_sel].v_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC0_V_COEFF_8D], v_coef_4tap[v_sel].v_coef_d[8]);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_0A], v_coef_4tap[v_sel].v_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_0B], v_coef_4tap[v_sel].v_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_0C], v_coef_4tap[v_sel].v_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_0D], v_coef_4tap[v_sel].v_coef_d[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_1A], v_coef_4tap[v_sel].v_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_1B], v_coef_4tap[v_sel].v_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_1C], v_coef_4tap[v_sel].v_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_1D], v_coef_4tap[v_sel].v_coef_d[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_2A], v_coef_4tap[v_sel].v_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_2B], v_coef_4tap[v_sel].v_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_2C], v_coef_4tap[v_sel].v_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_2D], v_coef_4tap[v_sel].v_coef_d[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_3A], v_coef_4tap[v_sel].v_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_3B], v_coef_4tap[v_sel].v_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_3C], v_coef_4tap[v_sel].v_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_3D], v_coef_4tap[v_sel].v_coef_d[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_4A], v_coef_4tap[v_sel].v_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_4B], v_coef_4tap[v_sel].v_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_4C], v_coef_4tap[v_sel].v_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_4D], v_coef_4tap[v_sel].v_coef_d[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_5A], v_coef_4tap[v_sel].v_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_5B], v_coef_4tap[v_sel].v_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_5C], v_coef_4tap[v_sel].v_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_5D], v_coef_4tap[v_sel].v_coef_d[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_6A], v_coef_4tap[v_sel].v_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_6B], v_coef_4tap[v_sel].v_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_6C], v_coef_4tap[v_sel].v_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_6D], v_coef_4tap[v_sel].v_coef_d[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_7A], v_coef_4tap[v_sel].v_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_7B], v_coef_4tap[v_sel].v_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_7C], v_coef_4tap[v_sel].v_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_7D], v_coef_4tap[v_sel].v_coef_d[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_8A], v_coef_4tap[v_sel].v_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC1_V_COEFF_8B], v_coef_4tap[v_sel].v_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_8C], v_coef_4tap[v_sel].v_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC1_V_COEFF_8D], v_coef_4tap[v_sel].v_coef_d[8]);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_0A], v_coef_4tap[v_sel].v_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_0B], v_coef_4tap[v_sel].v_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_0C], v_coef_4tap[v_sel].v_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_0D], v_coef_4tap[v_sel].v_coef_d[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_1A], v_coef_4tap[v_sel].v_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_1B], v_coef_4tap[v_sel].v_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_1C], v_coef_4tap[v_sel].v_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_1D], v_coef_4tap[v_sel].v_coef_d[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_2A], v_coef_4tap[v_sel].v_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_2B], v_coef_4tap[v_sel].v_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_2C], v_coef_4tap[v_sel].v_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_2D], v_coef_4tap[v_sel].v_coef_d[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_3A], v_coef_4tap[v_sel].v_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_3B], v_coef_4tap[v_sel].v_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_3C], v_coef_4tap[v_sel].v_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_3D], v_coef_4tap[v_sel].v_coef_d[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_4A], v_coef_4tap[v_sel].v_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_4B], v_coef_4tap[v_sel].v_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_4C], v_coef_4tap[v_sel].v_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_4D], v_coef_4tap[v_sel].v_coef_d[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_5A], v_coef_4tap[v_sel].v_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_5B], v_coef_4tap[v_sel].v_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_5C], v_coef_4tap[v_sel].v_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_5D], v_coef_4tap[v_sel].v_coef_d[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_6A], v_coef_4tap[v_sel].v_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_6B], v_coef_4tap[v_sel].v_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_6C], v_coef_4tap[v_sel].v_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_6D], v_coef_4tap[v_sel].v_coef_d[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_7A], v_coef_4tap[v_sel].v_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_7B], v_coef_4tap[v_sel].v_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_7C], v_coef_4tap[v_sel].v_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_7D], v_coef_4tap[v_sel].v_coef_d[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_8A], v_coef_4tap[v_sel].v_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC2_V_COEFF_8B], v_coef_4tap[v_sel].v_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_8C], v_coef_4tap[v_sel].v_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC2_V_COEFF_8D], v_coef_4tap[v_sel].v_coef_d[8]);
		break;
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_0A], v_coef_4tap[v_sel].v_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_0B], v_coef_4tap[v_sel].v_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_0C], v_coef_4tap[v_sel].v_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_0D], v_coef_4tap[v_sel].v_coef_d[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_1A], v_coef_4tap[v_sel].v_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_1B], v_coef_4tap[v_sel].v_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_1C], v_coef_4tap[v_sel].v_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_1D], v_coef_4tap[v_sel].v_coef_d[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_2A], v_coef_4tap[v_sel].v_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_2B], v_coef_4tap[v_sel].v_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_2C], v_coef_4tap[v_sel].v_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_2D], v_coef_4tap[v_sel].v_coef_d[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_3A], v_coef_4tap[v_sel].v_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_3B], v_coef_4tap[v_sel].v_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_3C], v_coef_4tap[v_sel].v_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_3D], v_coef_4tap[v_sel].v_coef_d[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_4A], v_coef_4tap[v_sel].v_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_4B], v_coef_4tap[v_sel].v_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_4C], v_coef_4tap[v_sel].v_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_4D], v_coef_4tap[v_sel].v_coef_d[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_5A], v_coef_4tap[v_sel].v_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_5B], v_coef_4tap[v_sel].v_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_5C], v_coef_4tap[v_sel].v_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_5D], v_coef_4tap[v_sel].v_coef_d[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_6A], v_coef_4tap[v_sel].v_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_6B], v_coef_4tap[v_sel].v_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_6C], v_coef_4tap[v_sel].v_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_6D], v_coef_4tap[v_sel].v_coef_d[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_7A], v_coef_4tap[v_sel].v_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_7B], v_coef_4tap[v_sel].v_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_7C], v_coef_4tap[v_sel].v_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_7D], v_coef_4tap[v_sel].v_coef_d[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_8A], v_coef_4tap[v_sel].v_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC3_V_COEFF_8B], v_coef_4tap[v_sel].v_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_8C], v_coef_4tap[v_sel].v_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC3_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC3_V_COEFF_8D], v_coef_4tap[v_sel].v_coef_d[8]);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_0A], v_coef_4tap[v_sel].v_coef_a[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_0AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_0B], v_coef_4tap[v_sel].v_coef_b[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_0C], v_coef_4tap[v_sel].v_coef_c[0]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_0CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_0D], v_coef_4tap[v_sel].v_coef_d[0]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_1A], v_coef_4tap[v_sel].v_coef_a[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_1AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_1B], v_coef_4tap[v_sel].v_coef_b[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_1C], v_coef_4tap[v_sel].v_coef_c[1]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_1CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_1D], v_coef_4tap[v_sel].v_coef_d[1]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_2A], v_coef_4tap[v_sel].v_coef_a[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_2AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_2B], v_coef_4tap[v_sel].v_coef_b[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_2C], v_coef_4tap[v_sel].v_coef_c[2]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_2CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_2D], v_coef_4tap[v_sel].v_coef_d[2]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_3A], v_coef_4tap[v_sel].v_coef_a[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_3AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_3B], v_coef_4tap[v_sel].v_coef_b[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_3C], v_coef_4tap[v_sel].v_coef_c[3]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_3CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_3D], v_coef_4tap[v_sel].v_coef_d[3]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_4A], v_coef_4tap[v_sel].v_coef_a[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_4AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_4B], v_coef_4tap[v_sel].v_coef_b[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_4C], v_coef_4tap[v_sel].v_coef_c[4]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_4CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_4D], v_coef_4tap[v_sel].v_coef_d[4]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_5A], v_coef_4tap[v_sel].v_coef_a[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_5AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_5B], v_coef_4tap[v_sel].v_coef_b[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_5C], v_coef_4tap[v_sel].v_coef_c[5]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_5CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_5D], v_coef_4tap[v_sel].v_coef_d[5]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_6A], v_coef_4tap[v_sel].v_coef_a[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_6AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_6B], v_coef_4tap[v_sel].v_coef_b[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_6C], v_coef_4tap[v_sel].v_coef_c[6]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_6CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_6D], v_coef_4tap[v_sel].v_coef_d[6]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_7A], v_coef_4tap[v_sel].v_coef_a[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_7AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_7B], v_coef_4tap[v_sel].v_coef_b[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_7C], v_coef_4tap[v_sel].v_coef_c[7]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_7CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_7D], v_coef_4tap[v_sel].v_coef_d[7]);

		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_8A], v_coef_4tap[v_sel].v_coef_a[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_8AB], &mcsc_fields[MCSC_F_SC4_V_COEFF_8B], v_coef_4tap[v_sel].v_coef_b[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_8C], v_coef_4tap[v_sel].v_coef_c[8]);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC4_V_COEFF_8CD], &mcsc_fields[MCSC_F_SC4_V_COEFF_8D], v_coef_4tap[v_sel].v_coef_d[8]);
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
		h_coef = MCSC_COEFF_x7_8;
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
		v_coef = MCSC_COEFF_x7_8;
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC1_POST_SC_ENABLE], enable);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC2_POST_SC_ENABLE], enable);
		break;
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC3_POST_SC_ENABLE], enable);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_CTRL], &mcsc_fields[MCSC_F_PC4_POST_SC_ENABLE], enable);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_IMG_VSIZE], height);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_IMG_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_IMG_VSIZE], height);
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
	case MCSC_OUTPUT3:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_IMG_VSIZE]);
		break;
	case MCSC_OUTPUT4:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_IMG_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_IMG_VSIZE]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_DST_VSIZE], height);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_DST_HSIZE], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_DST_VSIZE], height);
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
	case MCSC_OUTPUT3:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC3_POST_SC_DST_VSIZE]);
		break;
	case MCSC_OUTPUT4:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_DST_SIZE], &mcsc_fields[MCSC_F_PC4_POST_SC_DST_VSIZE]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_H_RATIO], &mcsc_fields[MCSC_F_PC3_POST_SC_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_POST_SC_V_RATIO], &mcsc_fields[MCSC_F_PC3_POST_SC_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_H_RATIO], &mcsc_fields[MCSC_F_PC4_POST_SC_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_POST_SC_V_RATIO], &mcsc_fields[MCSC_F_PC4_POST_SC_V_RATIO], vratio);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_CONV420_WEIGHT], &mcsc_fields[MCSC_F_PC3_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_CONV420_CTRL], &mcsc_fields[MCSC_F_PC3_CONV420_ENABLE], conv420_en);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_CONV420_WEIGHT], &mcsc_fields[MCSC_F_PC4_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_CONV420_CTRL], &mcsc_fields[MCSC_F_PC4_CONV420_ENABLE], conv420_en);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_CTRL], &mcsc_fields[MCSC_F_PC3_BCHS_ENABLE], bchs_en);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_CTRL], &mcsc_fields[MCSC_F_PC4_BCHS_ENABLE], bchs_en);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_BC], &mcsc_fields[MCSC_F_PC3_BCHS_YOFFSET], y_offset);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_BC], &mcsc_fields[MCSC_F_PC3_BCHS_YGAIN], y_gain);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_BC], &mcsc_fields[MCSC_F_PC4_BCHS_YOFFSET], y_offset);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_BC], &mcsc_fields[MCSC_F_PC4_BCHS_YGAIN], y_gain);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_HS1], &mcsc_fields[MCSC_F_PC3_BCHS_C_GAIN_00], c_gain00);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_HS1], &mcsc_fields[MCSC_F_PC3_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_HS2], &mcsc_fields[MCSC_F_PC3_BCHS_C_GAIN_10], c_gain10);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_BCHS_HS2], &mcsc_fields[MCSC_F_PC3_BCHS_C_GAIN_11], c_gain11);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_HS1], &mcsc_fields[MCSC_F_PC4_BCHS_C_GAIN_00], c_gain00);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_HS1], &mcsc_fields[MCSC_F_PC4_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_HS2], &mcsc_fields[MCSC_F_PC4_BCHS_C_GAIN_10], c_gain10);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_BCHS_HS2], &mcsc_fields[MCSC_F_PC4_BCHS_C_GAIN_11], c_gain11);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_bchs_clamp(void __iomem *base_addr, u32 output_id, u32 y_max, u32 y_min, u32 c_max, u32 c_min)
{
	u32 reg_value = 0, reg_idx = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_Y_CLAMP_MAX], y_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_Y_CLAMP_MIN], y_min);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_C_CLAMP_MAX], c_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_BCHS_C_CLAMP_MIN], c_min);
		reg_idx = MCSC_R_PC0_BCHS_CLAMP;
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_Y_CLAMP_MAX], y_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_Y_CLAMP_MIN], y_min);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_C_CLAMP_MAX], c_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_C_CLAMP_MIN], c_min);
		reg_idx = MCSC_R_PC1_BCHS_CLAMP;
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_Y_CLAMP_MAX], y_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_Y_CLAMP_MIN], y_min);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_C_CLAMP_MAX], c_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_C_CLAMP_MIN], c_min);
		reg_idx = MCSC_R_PC2_BCHS_CLAMP;
		break;
	case MCSC_OUTPUT3:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC3_BCHS_Y_CLAMP_MAX], y_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC3_BCHS_Y_CLAMP_MIN], y_min);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC3_BCHS_C_CLAMP_MAX], c_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC3_BCHS_C_CLAMP_MIN], c_min);
		reg_idx = MCSC_R_PC3_BCHS_CLAMP;
		break;
	case MCSC_OUTPUT4:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC4_BCHS_Y_CLAMP_MAX], y_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC4_BCHS_Y_CLAMP_MIN], y_min);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC4_BCHS_C_CLAMP_MAX], c_max);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC4_BCHS_C_CLAMP_MIN], c_min);
		reg_idx = MCSC_R_PC4_BCHS_CLAMP;
		break;
	default:
		return;
	}

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[reg_idx], reg_value);
	dbg_hw(2, "[OUT:%d]set_bchs_clamp: sfr_offset(0x%x), read_value(0x%x)\n", output_id,
			mcsc_regs[reg_idx].sfr_offset,
			fimc_is_hw_get_reg(base_addr, &mcsc_regs[reg_idx]));
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC3_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC4_DMA_OUT_ENABLE], (u32)dma_out_en);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC3_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC3_OTF_OUT_ENABLE], (u32)otf_out_en);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC4_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC4_OTF_OUT_ENABLE], (u32)otf_out_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_pri(void __iomem *base_addr, u32 output_id, u32 plane)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_0], &mcsc_fields[MCSC_F_WDMA_PRI_A_0], 0);	/* MCSC_WDMA_OUTPUT0_Y */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_0], &mcsc_fields[MCSC_F_WDMA_PRI_A_1], 3);	/* MCSC_WDMA_OUTPUT1_Y */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_0], &mcsc_fields[MCSC_F_WDMA_PRI_A_2], 6);	/* MCSC_WDMA_OUTPUT2_Y */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_0], &mcsc_fields[MCSC_F_WDMA_PRI_A_3], 1);	/* MCSC_WDMA_OUTPUT0_U */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_0], &mcsc_fields[MCSC_F_WDMA_PRI_A_4], 4);	/* MCSC_WDMA_OUTPUT1_U */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_0], &mcsc_fields[MCSC_F_WDMA_PRI_A_5], 7);	/* MCSC_WDMA_OUTPUT2_U */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_1], &mcsc_fields[MCSC_F_WDMA_PRI_A_6], 2);	/* MCSC_WDMA_OUTPUT0_V */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_1], &mcsc_fields[MCSC_F_WDMA_PRI_A_7], 5);	/* MCSC_WDMA_OUTPUT1_V */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_1], &mcsc_fields[MCSC_F_WDMA_PRI_A_8], 8);	/* MCSC_WDMA_OUTPUT2_V */

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_0], 6);	/* MCSC_WDMA_OUTPUT5_Y: FIXED */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_1], 0);	/* MCSC_WDMA_OUTPUT3_Y */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_2], 3);	/* MCSC_WDMA_OUTPUT4_Y */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_3], 1);	/* MCSC_WDMA_OUTPUT3_U */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_4], 4);	/* MCSC_WDMA_OUTPUT4_U */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_5], 2);	/* MCSC_WDMA_OUTPUT3_V */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_PRI_2], &mcsc_fields[MCSC_F_WDMA_PRI_B_6], 5);	/* MCSC_WDMA_OUTPUT4_V */
}

void fimc_is_scaler_set_wdma_axi_pri(void __iomem *base_addr)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_AXI_PRI], &mcsc_fields[MCSC_F_WDMA_AXI_PRITYP], 0);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_AXI_PRI], &mcsc_fields[MCSC_F_WDMA_AXI_FIXEDPRITYP], 0);
}

void fimc_is_scaler_set_wdma_sram_base(void __iomem *base_addr, u32 output_id)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0], &mcsc_fields[MCSC_F_WDMA0_SRAM_BASE], 0);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0], &mcsc_fields[MCSC_F_WDMA1_SRAM_BASE], 128);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0], &mcsc_fields[MCSC_F_WDMA2_SRAM_BASE], 256);
		break;
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_1], &mcsc_fields[MCSC_F_WDMA3_SRAM_BASE], 0);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_1], &mcsc_fields[MCSC_F_WDMA4_SRAM_BASE], 128);
		break;
	case MCSC_OUTPUT_SSB:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_1], &mcsc_fields[MCSC_F_WDMA5_SRAM_BASE], 256);
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
	case MCSC_OUTPUT3:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC3_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT4:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC4_DMA_OUT_ENABLE]);
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
	case MCSC_OUTPUT3:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC3_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC3_OTF_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT4:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC4_OTF_OUT_CTRL], &mcsc_fields[MCSC_F_PC4_OTF_OUT_ENABLE]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA3_DATA_FORMAT], dma_out_format);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_DATA_FORMAT], &mcsc_fields[MCSC_F_WDMA4_DATA_FORMAT], dma_out_format);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_format(void __iomem *base_addr, u32 output_id, u32 *dma_out_format)
{
	/* not used */
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_SWAP_TABLE], &mcsc_fields[MCSC_F_WDMA3_SWAP_TABLE], swap);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_SWAP_TABLE], &mcsc_fields[MCSC_F_WDMA4_SWAP_TABLE], swap);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_FLIP_CONTROL], &mcsc_fields[MCSC_F_WDMA3_FLIP_CONTROL], flip);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_FLIP_CONTROL], &mcsc_fields[MCSC_F_WDMA4_FLIP_CONTROL], flip);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_WIDTH], &mcsc_fields[MCSC_F_WDMA3_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_HEIGHT], &mcsc_fields[MCSC_F_WDMA3_HEIGHT], height);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_WIDTH], &mcsc_fields[MCSC_F_WDMA4_WIDTH], width);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_HEIGHT], &mcsc_fields[MCSC_F_WDMA4_HEIGHT], height);
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
	case MCSC_OUTPUT3:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_WIDTH], &mcsc_fields[MCSC_F_WDMA3_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_HEIGHT], &mcsc_fields[MCSC_F_WDMA3_HEIGHT]);
		break;
	case MCSC_OUTPUT4:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_WIDTH], &mcsc_fields[MCSC_F_WDMA4_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_HEIGHT], &mcsc_fields[MCSC_F_WDMA4_HEIGHT]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_STRIDE], &mcsc_fields[MCSC_F_WDMA3_Y_STRIDE], y_stride);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_STRIDE], &mcsc_fields[MCSC_F_WDMA3_C_STRIDE], uv_stride);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_STRIDE], &mcsc_fields[MCSC_F_WDMA4_Y_STRIDE], y_stride);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_STRIDE], &mcsc_fields[MCSC_F_WDMA4_C_STRIDE], uv_stride);
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
	case MCSC_OUTPUT3:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_STRIDE], &mcsc_fields[MCSC_F_WDMA3_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_STRIDE], &mcsc_fields[MCSC_F_WDMA3_C_STRIDE]);
		break;
	case MCSC_OUTPUT4:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_STRIDE], &mcsc_fields[MCSC_F_WDMA4_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_STRIDE], &mcsc_fields[MCSC_F_WDMA4_C_STRIDE]);
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
	case MCSC_OUTPUT3:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN1], (frame_seq & 0x01) >> 0);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN2], (frame_seq & 0x02) >> 1);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN3], (frame_seq & 0x04) >> 2);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN4], (frame_seq & 0x08) >> 3);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN5], (frame_seq & 0x10) >> 4);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN6], (frame_seq & 0x20) >> 5);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN7], (frame_seq & 0x40) >> 6);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA3_BASE_ADDR_EN8], (frame_seq & 0x80) >> 7);
		break;
	case MCSC_OUTPUT4:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN1], (frame_seq & 0x01) >> 0);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN2], (frame_seq & 0x02) >> 1);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN3], (frame_seq & 0x04) >> 2);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN4], (frame_seq & 0x08) >> 3);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN5], (frame_seq & 0x10) >> 4);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN6], (frame_seq & 0x20) >> 5);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN7], (frame_seq & 0x40) >> 6);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR_EN], &mcsc_fields[MCSC_F_WDMA4_BASE_ADDR_EN8], (frame_seq & 0x80) >> 7);
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

static void fimc_is_scaler_set_wdma3_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR21], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR22], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR23], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR24], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR25], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR26], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR27], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR08], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR28], cr_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma4_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR01], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR21], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR02], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR22], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR03], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR23], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR04], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR24], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR05], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR25], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR06], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR26], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR07], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR27], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR08], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR28], cr_addr);
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
	case MCSC_OUTPUT3:
		fimc_is_scaler_set_wdma3_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	case MCSC_OUTPUT4:
		fimc_is_scaler_set_wdma4_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_addr(void __iomem *base_addr, u32 output_id, u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* not used */
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
	case MCSC_OUTPUT3:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR01], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR02], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR03], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR04], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR05], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR06], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR07], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR08], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR11], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR12], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR13], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR14], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR15], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR16], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR17], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR18], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR21], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR22], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR23], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR24], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR25], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR26], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR27], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA3_BASE_ADDR28], 0x0);
		break;
	case MCSC_OUTPUT4:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR01], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR02], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR03], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR04], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR05], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR06], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR07], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR08], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR11], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR12], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR13], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR14], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR15], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR16], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR17], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR18], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR21], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR22], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR23], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR24], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR25], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR26], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR27], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA4_BASE_ADDR28], 0x0);
		break;
	default:
		break;
	}
}

/* for tdnr */
void fimc_is_scaler_set_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_addr, u32 cb_addr, u32 cr_addr)
{
	/* not used */
}

void fimc_is_scaler_clear_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_addr, u32 cb_addr, u32 cr_addr)
{
	/* not used */
}

void fimc_is_scaler_get_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_addr, u32 *cb_addr, u32 *cr_addr)
{
	/* not used */
}

void fimc_is_scaler_clear_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height)
{
	/* not used */
}

void fimc_is_scaler_get_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 *width, u32 *height)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_rdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_rdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_in_format)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_wdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_out_format)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_stride, u32 uv_stride)
{
	/* not used */
}

void fimc_is_scaler_get_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_stride, u32 *uv_stride)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 y_stride, u32 uv_stride)
{
	/* not used */
}

void fimc_is_scaler_get_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type, u32 *y_stride, u32 *uv_stride)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_wdma_sram_base(void __iomem *base_addr, enum tdnr_buf_type type)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_wdma_enable(void __iomem *base_addr, enum tdnr_buf_type type, bool dma_out_en)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_image_size(void __iomem *base_addr, u32 width, u32 height)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_mode_select(void __iomem *base_addr, enum tdnr_mode mode)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_first(void __iomem *base_addr, u32 tdnr_first)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_general(void __iomem *base_addr, struct general_config config)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_yuvtable(void __iomem *base_addr, struct yuv_table_config config)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_temporal(void __iomem *base_addr,
	struct temporal_ni_dep_config dep_config,
	struct temporal_ni_indep_config indep_config)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_constant_lut_coeffs(void __iomem *base_addr, u32 *config)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_refine_control(void __iomem *base_addr,
	struct refine_control_config config)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_regional_feature(void __iomem *base_addr,
	struct regional_ni_dep_config dep_config,
	struct regional_ni_indep_config indep_config)
{
	/* not used */
}

void fimc_is_scaler_set_tdnr_tuneset_spatial(void __iomem *base_addr,
	struct spatial_ni_dep_config dep_config,
	struct spatial_ni_indep_config indep_config)
{
	/* not used */
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
	u32 read_val;

	if (hwfc_output_ids & (1 << MCSC_OUTPUT3))
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_FRAME_START_SELECT],
			&mcsc_fields[MCSC_F_FRAME_START_SELECT], 0x1);

	read_val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_MODE],
			&mcsc_fields[MCSC_F_HWFC_MODE]);

	if ((hwfc_output_ids & (1 << MCSC_OUTPUT3)) && (hwfc_output_ids & (1 << MCSC_OUTPUT4))) {
		val = MCSC_HWFC_MODE_REGION_A_B_PORT;
	} else if (hwfc_output_ids & (1 << MCSC_OUTPUT3)) {
		val = MCSC_HWFC_MODE_REGION_A_PORT;
	} else if (hwfc_output_ids & (1 << MCSC_OUTPUT4)) {
		err_hw("set_hwfc_mode: invalid output_ids(0x%x)\n", hwfc_output_ids);
		return;
	}

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_MODE],
			&mcsc_fields[MCSC_F_HWFC_MODE], val);

	dbg_hw(2, "set_hwfc_mode: regs(0x%x)(0x%x), hwfc_ids(0x%x)\n",
		read_val, val, hwfc_output_ids);
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
			err_hw("invalid hwfc plane (%d, %d, %dx%d)",
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
			err_hw("invalid hwfc plane (%d, %d, %dx%d)",
				format, plane, width, height);
			return;
		}

		hwfc_format = MCSC_HWFC_FMT_YUV420;
		break;
	default:
		err_hw("invalid hwfc format (%d, %d, %dx%d)",
			format, plane, width, height);
		return;
	}

	switch (output_id) {
	case MCSC_OUTPUT0:
		break;
	case MCSC_OUTPUT1:
		break;
	case MCSC_OUTPUT2:
		break;
	case MCSC_OUTPUT3:
		val = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_A]);
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
		break;
	case MCSC_OUTPUT4:
		val = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_B]);
		/* format */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_FORMAT_B], hwfc_format);

		/* plane */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_PLANE_B], plane);

		/* dma idx */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_ID0_B], dma_idx);
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_ID1_B], dma_idx+1);
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_ID2_B], dma_idx+2);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_B], val);

		/* total pixel/byte */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE0_B], total_img_byte0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE1_B], total_img_byte1);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE2_B], total_img_byte2);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE0_B], total_width_byte0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE1_B], total_width_byte1);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE2_B], total_width_byte2);
		break;
	default:
		break;
	}
}

u32 fimc_is_scaler_get_hwfc_idx_bin(void __iomem *base_addr, u32 output_id)
{
	u32 val = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		break;
	case MCSC_OUTPUT1:
		break;
	case MCSC_OUTPUT2:
		break;
	case MCSC_OUTPUT3:
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_REGION_IDX_BIN],
			&mcsc_fields[MCSC_F_REGION_IDX_BIN_A]);
		break;
	case MCSC_OUTPUT4:
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_REGION_IDX_BIN],
			&mcsc_fields[MCSC_F_REGION_IDX_BIN_B]);
		break;
	default:
		break;
	}

	return val;
}

u32 fimc_is_scaler_get_hwfc_cur_idx(void __iomem *base_addr, u32 output_id)
{
	u32 val = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		break;
	case MCSC_OUTPUT1:
		break;
	case MCSC_OUTPUT2:
		break;
	case MCSC_OUTPUT3:
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_CURR_REGION],
			&mcsc_fields[MCSC_F_CURR_REGION_A]);
		break;
	case MCSC_OUTPUT4:
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_CURR_REGION],
			&mcsc_fields[MCSC_F_CURR_REGION_B]);
		break;
	default:
		break;
	}

	return val;
}

static void fimc_is_scaler0_clear_intr_src(void __iomem *base_addr, u32 status)
{
	if (status & (1 << INTR_MC_SCALER_OUTSTALL))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_OUTSTALL);

	if (status & (1 << INTR_MC_SCALER_OVERFLOW))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_OUTSTALL);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF);

	if (status & (1 << INTR_MC_SCALER_SHADOW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_SHADOW_TRIGGER);

	if (status & (1 << INTR_MC_SCALER_SHADOW_HW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_SHADOW_HW_TRIGGER);

	if (status & (1 << INTR_MC_SCALER_CORE_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_CORE_FINISH);

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_WDMA_FINISH);

	if (status & (1 << INTR_MC_SCALER_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_FRAME_START);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_0], (u32)1 << INTR_MC_SCALER_FRAME_END);
}

static void fimc_is_scaler1_clear_intr_src(void __iomem *base_addr, u32 status)
{
	if (status & (1 << INTR_MC_SCALER_OUTSTALL))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_OUTSTALL);

	if (status & (1 << INTR_MC_SCALER_OVERFLOW))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_OUTSTALL);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF);

	if (status & (1 << INTR_MC_SCALER_SHADOW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_SHADOW_TRIGGER);

	if (status & (1 << INTR_MC_SCALER_SHADOW_HW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_SHADOW_HW_TRIGGER);

	if (status & (1 << INTR_MC_SCALER_CORE_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_CORE_FINISH);

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_WDMA_FINISH);

	if (status & (1 << INTR_MC_SCALER_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_FRAME_START);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1], (u32)1 << INTR_MC_SCALER_FRAME_END);
}

void fimc_is_scaler_clear_intr_src(void __iomem *base_addr, u32 hw_id, u32 status)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_scaler0_clear_intr_src(base_addr, status);
		break;
	case DEV_HW_MCSC1:
		fimc_is_scaler1_clear_intr_src(base_addr, status);
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
	case DEV_HW_MCSC1:
		ret = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK_1]);
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
	case DEV_HW_MCSC1:
		ret = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_1]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

	return ret;
}

u32 fimc_is_scaler_handle_extended_intr(u32 status)
{
	/* not used */
	return 0;
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

void fimc_is_scaler_set_wdma_2bit_stride(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_stride, u32 uv_2bit_stride)
{
	/* Not Support */
}

void fimc_is_scaler_set_wdma_2bit_addr(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
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
