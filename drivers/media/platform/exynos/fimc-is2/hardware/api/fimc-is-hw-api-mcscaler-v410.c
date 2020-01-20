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
#include "sfr/fimc-is-sfr-mcsc-v410.h"
#include "fimc-is-hw.h"
#include "fimc-is-hw-control.h"
#include "fimc-is-param.h"


/* WDMA SRAM base value, UG recommanded value */
#define WDMA0_SRAM_BASE_VALUE	(0)
#define WDMA1_SRAM_BASE_VALUE	(256)
#define WDMA2_SRAM_BASE_VALUE	(512)	/* max 768 */

#define WDMADS_SRAM_BASE_VALUE	(0)	/* DS */
#define WDMATDNR_SRAM_BASE_VALUE	(200)	/* TDNR */

/* 10bit coefficient */
const struct mcsc_v_coef v_coef_4tap[7] = {
	/* x8/8 */
	{
		{  0, -15, -25, -31, -33, -33, -31, -27, -23},
		{512, 508, 495, 473, 443, 408, 367, 324, 279},
		{  0,  20,  45,  75, 110, 148, 190, 234, 279},
		{  0,  -1,  -3,  -5,  -8, -11, -14, -19, -23},
	},
	/* x7/8 */
	{
		{ 32,  17,   3,  -7, -14, -18, -20, -20, -19},
		{448, 446, 437, 421, 399, 373, 343, 310, 275},
		{ 32,  55,  79, 107, 138, 170, 204, 240, 275},
		{  0,  -6,  -7,  -9, -11, -13, -15, -18, -19},
	},
	/* x6/8 */
	{
		{ 61,  46,  31,  19,   9,   2,  -3,  -7,  -9},
		{390, 390, 383, 371, 356, 337, 315, 291, 265},
		{ 61,  83, 106, 130, 156, 183, 210, 238, 265},
		{  0,  -7,  -8,  -8,  -9, -10, -10, -10,  -9},
	},
	/* x5/8 */
	{
		{ 85,  71,  56,  43,  32,  23,  16,   9,   5},
		{341, 341, 336, 328, 317, 304, 288, 271, 251},
		{ 86, 105, 124, 145, 166, 187, 209, 231, 251},
		{  0,  -5,  -4,  -4,  -3,  -2,  -1,   1,   5},
	},
	/* x4/8 */
	{
		{104,  89,  76,  63,  52,  42,  33,  26,  20},
		{304, 302, 298, 293, 285, 275, 264, 251, 236},
		{104, 120, 136, 153, 170, 188, 205, 221, 236},
		{  0,   1,   2,   3,   5,   7,  10,  14,  20},
	},
	/* x3/8 */
	{
		{118, 103,  90,  78,  67,  57,  48,  40,  33},
		{276, 273, 270, 266, 260, 253, 244, 234, 223},
		{118, 129, 143, 157, 171, 185, 199, 211, 223},
		{  0,   7,   9,  11,  14,  17,  21,  27,  33},
	},
	/* x2/8 */
	{
		{127, 111, 100,  88,  78,  68,  59,  50,  43},
		{258, 252, 250, 247, 242, 237, 230, 222, 213},
		{127, 135, 147, 159, 171, 182, 193, 204, 213},
		{  0,  14,  15,  18,  21,  25,  30,  36,  43},
	}
};

const struct mcsc_h_coef h_coef_8tap[7] = {
	/* x8/8 */
	{
		{  0,  -2,  -4,  -5,  -6,  -6,  -6,  -6,  -5},
		{  0,   8,  14,  20,  23,  25,  26,  25,  23},
		{  0, -25, -46, -62, -73, -80, -83, -82, -78},
		{512, 509, 499, 482, 458, 429, 395, 357, 316},
		{  0,  30,  64, 101, 142, 185, 228, 273, 316},
		{  0,  -9, -19, -30, -41, -53, -63, -71, -78},
		{  0,   2,   5,   8,  12,  15,  19,  21,  23},
		{  0,  -1,  -1,  -2,  -3,  -3,  -4,  -5,  -5},
	},
	/* x7/8 */
	{
		{ 12,   9,   7,   5,   3,   2,   1,   0,  -1},
		{-32, -24, -16,  -9,  -3,   2,   7,  10,  13},
		{ 56,  29,   6, -14, -30, -43, -53, -60, -65},
		{444, 445, 438, 426, 410, 390, 365, 338, 309},
		{ 52,  82, 112, 144, 177, 211, 244, 277, 309},
		{-32, -39, -46, -52, -58, -63, -66, -66, -65},
		{ 12,  13,  14,  15,  16,  16,  16,  15,  13},
		{  0,  -3,	-3,  -3,  -3,  -3,  -2,  -2,  -1},
	},
	/* x6/8 */
	{
		{  8,   9,   8,   8,   8,   7,   7,   5,   5},
		{-44, -40, -36, -32, -27, -22, -18, -13,  -9},
		{100,  77,  57,  38,  20,   5,  -9, -20, -30},
		{384, 382, 377, 369, 358, 344, 329, 310, 290},
		{100, 123, 147, 171, 196, 221, 245, 268, 290},
		{-44, -47, -49, -49, -48, -47, -43, -37, -30},
		{  8,   8,   7,   5,   3,   1,  -2,  -5,  -9},
		{  0,   0,   1,   2,   2,   3,   3,   4,   5},
	},
	/* x5/8 */
	{
		{ -3,  -3,  -1,   0,   1,   2,   2,   3,   3},
		{-31, -32, -33, -32, -31, -30, -28, -25, -23},
		{130, 113,  97,  81,  66,  52,  38,  26,  15},
		{320, 319, 315, 311, 304, 296, 286, 274, 261},
		{130, 147, 165, 182, 199, 216, 232, 247, 261},
		{-31, -29, -26, -22, -17, -11,  -3,   5,  15},
		{ -3,  -6,  -8, -11, -13, -16, -18, -21, -23},
		{  0,   3,   3,   3,   3,   3,   3,   3,   3},
	},
	/* x4/8 */
	{
		{-11, -10,  -9,  -8,  -7,  -6,  -5,  -5,  -4},
		{  0,  -4,  -7, -10, -12, -14, -15, -16, -17},
		{140, 129, 117, 106,  95,  85,  74,  64,  55},
		{255, 254, 253, 250, 246, 241, 236, 229, 222},
		{140, 151, 163, 174, 185, 195, 204, 214, 222},
		{  0,   5,  10,  16,  22,  29,  37,  46,  55},
		{-12, -13, -14, -15, -16, -16, -17, -17, -17},
		{  0,   0,  -1,  -1,  -1,  -2,  -2,  -3,  -4},
	},
	/* x3/8 */
	{
		{ -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5},
		{ 31,  27,  23,  19,  16,  12,  10,   7,   5},
		{133, 126, 119, 112, 105,  98,  91,  84,  78},
		{195, 195, 194, 193, 191, 189, 185, 182, 178},
		{133, 139, 146, 152, 158, 163, 169, 174, 178},
		{ 31,  37,  41,  47,  53,  59,  65,  71,  78},
		{ -6,  -4,  -3,  -2,  -2,   0,   1,   3,   5},
		{  0,  -3,  -3,  -4,  -4,  -4,  -4,  -4,  -5},
	},
	/* x2/8 */
	{
		{ 10,   9,   7,   6,   5,   4,   4,   3,   2},
		{ 52,  48,  45,  41,  38,  35,  31,  29,  26},
		{118, 114, 110, 106, 102,  98,  94,  89,  85},
		{152, 152, 151, 150, 149, 148, 146, 145, 143},
		{118, 122, 125, 129, 132, 135, 138, 140, 143},
		{ 52,  56,  60,  64,  68,  72,  77,  81,  85},
		{ 10,  11,  13,  15,  17,  19,  21,  23,  26},
		{  0,   0,   1,   1,   1,   1,   1,   2,   2},
	}
};

void fimc_is_scaler_start(void __iomem *base_addr, u32 hw_id)
{
	/* Qactive must set to "1" before scaler enable */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_APB_CLK_GATE_CTRL], &mcsc_fields[MCSC_F_QACTIVE_ENABLE], 1);

	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_ENABLE], &mcsc_fields[MCSC_F_SCALER_ENABLE], 1);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_rdma_start(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_RDMA_START],
			&mcsc_fields[MCSC_F_SCALER_RDMA_START], 1);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC rdma api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_stop(void __iomem *base_addr, u32 hw_id)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_ENABLE], &mcsc_fields[MCSC_F_SCALER_ENABLE], 0);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

#if defined(ENABLE_HWACG_CONTROL)
	/* Qactive must set to "0" for ip clock gating */
	if (!fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_ENABLE])
		&& fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_SCALER_RUNNING_STATUS], &mcsc_fields[MCSC_F_SCALER_IDLE])
		fimc_is_hw_set_field(base_addr,
			&mcsc_regs[MCSC_R_APB_CLK_GATE_CTRL], &mcsc_fields[MCSC_F_QACTIVE_ENABLE], 0);
#endif
}

#define IS_RESET_DONE(addr, reg, field)	\
	(fimc_is_hw_get_field(addr, &mcsc_regs[reg], &mcsc_fields[field]) != 0)

static u32 fimc_is_scaler_sw_reset_global(void __iomem *base_addr)
{
	u32 reset_count = 0;

	/* request scaler reset */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC_RESET_CTRL_GLOBAL],
		&mcsc_fields[MCSC_F_SW_RESET_GLOBAL], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (IS_RESET_DONE(base_addr, MCSC_R_SCALER_RESET_STATUS, MCSC_F_SW_RESET_GLOBAL_STATUS));

	return 0;
}

u32 fimc_is_scaler_sw_reset(void __iomem *base_addr, u32 hw_id, u32 global, u32 partial)
{
	int ret = 0;

	ret = fimc_is_scaler_sw_reset_global(base_addr);
	return ret;
}

void fimc_is_scaler_clear_intr_all(void __iomem *base_addr, u32 hw_id)
{
	u32 reg_value = 0;

	switch (hw_id) {
	case DEV_HW_MCSC0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FM_SUB_FRAME_START_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FM_SUB_FRAME_FINISH_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_OVER_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_FRAME_CRUSH_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_CORE_FINISH_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA_FINISH_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_START_INT], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_END_INT], 1);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT], reg_value);
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
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_FM_SUB_FRAME_START_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_FM_SUB_FRAME_FINISH_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_OVER_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_SHADOW_COPY_FINISH_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_INPUT_FRAME_CRUSH_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_SCALER_OVERFLOW_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_INPUT_VERTICAL_UNF_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_INPUT_VERTICAL_OVF_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_INPUT_HORIZONTAL_UNF_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_INPUT_HORIZONTAL_OVF_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_CORE_FINISH_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA_FINISH_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_START_INT_MASK], 1);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FRAME_END_INT_MASK], 1);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK], reg_value);
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
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK], intr_mask);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}
}

void fimc_is_scaler_set_stop_req_post_en_ctrl(void __iomem *base_addr, u32 hw_id, u32 value)
{
	/* not support */
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
	default:
		break;
	}
}

void fimc_is_scaler_get_input_status(void __iomem *base_addr, u32 hw_id, u32 *hl, u32 *vl)
{
	switch (hw_id) {
	case DEV_HW_MCSC0:
		*hl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS],
			&mcsc_fields[MCSC_F_CUR_HORIZONTAL_CNT]);
		*vl = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_STATUS],
			&mcsc_fields[MCSC_F_CUR_VERTICAL_CNT]);
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_TYPE],
			&mcsc_fields[MCSC_F_SCALER_INPUT_TYPE], rdma);
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
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_TYPE],
			&mcsc_fields[MCSC_F_SCALER_INPUT_TYPE]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_INPUT_IMG_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SCALER_INPUT_IMG_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_IMG_SIZE], reg_value);

		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IMAGE_WIDTH], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IMAGE_HEIGHT], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_DIMENSIONS], reg_value);
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
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_IMG_SIZE],
			&mcsc_fields[MCSC_F_SCALER_INPUT_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_INPUT_IMG_SIZE],
			&mcsc_fields[MCSC_F_SCALER_INPUT_IMG_VSIZE]);
		break;
	default:
		warn_hw("invalid hw_id(%d) for MCSC api\n", hw_id);
		break;
	}

}

u32 fimc_is_scaler_get_scaler_path(void __iomem *base_addr, u32 hw_id, u32 output_id)
{
	u32 input = 0, enable_poly = 0, enable_post = 0, enable_dma = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		enable_poly = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_SC0_CTRL], &mcsc_fields[MCSC_F_SC0_ENABLE]);
		enable_post = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_PC0_CTRL], &mcsc_fields[MCSC_F_PC0_ENABLE]);
		enable_dma = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT1:
		enable_poly = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_SC1_CTRL], &mcsc_fields[MCSC_F_SC1_ENABLE]);
		enable_post = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_PC1_CTRL], &mcsc_fields[MCSC_F_PC1_ENABLE]);
		enable_dma = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT2:
		enable_poly = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_SC2_CTRL], &mcsc_fields[MCSC_F_SC2_ENABLE]);
		enable_post = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_PC2_CTRL], &mcsc_fields[MCSC_F_PC2_ENABLE]);
		enable_dma = fimc_is_hw_get_field(base_addr,
			&mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL], &mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE]);
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
	/* Not support */
}

void fimc_is_scaler_set_poly_src_size(void __iomem *base_addr, u32 output_id,
	u32 pos_x, u32 pos_y, u32 width, u32 height)
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC1_SRC_HPOS], pos_x);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC1_SRC_VPOS], pos_y);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_POS], reg_value);

		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC1_SRC_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC1_SRC_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC2_SRC_HPOS], pos_x);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC2_SRC_VPOS], pos_y);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_POS], reg_value);

		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC2_SRC_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC2_SRC_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_poly_src_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE],
			&mcsc_fields[MCSC_F_SC0_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_SRC_SIZE],
			&mcsc_fields[MCSC_F_SC0_SRC_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE],
			&mcsc_fields[MCSC_F_SC1_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_SRC_SIZE],
			&mcsc_fields[MCSC_F_SC1_SRC_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE],
			&mcsc_fields[MCSC_F_SC2_SRC_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_SRC_SIZE],
			&mcsc_fields[MCSC_F_SC2_SRC_VSIZE]);
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC1_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC1_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC2_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_SC2_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_poly_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE],
			&mcsc_fields[MCSC_F_SC0_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC0_DST_SIZE],
			&mcsc_fields[MCSC_F_SC0_DST_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE],
			&mcsc_fields[MCSC_F_SC1_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC1_DST_SIZE],
			&mcsc_fields[MCSC_F_SC1_DST_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE],
			&mcsc_fields[MCSC_F_SC2_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SC2_DST_SIZE],
			&mcsc_fields[MCSC_F_SC2_DST_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_RATIO],
			&mcsc_fields[MCSC_F_SC0_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_RATIO],
			&mcsc_fields[MCSC_F_SC0_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_RATIO],
			&mcsc_fields[MCSC_F_SC1_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_RATIO],
			&mcsc_fields[MCSC_F_SC1_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_RATIO],
			&mcsc_fields[MCSC_F_SC2_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_RATIO],
			&mcsc_fields[MCSC_F_SC2_V_RATIO], vratio);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_h_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 h_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_H_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_SC0_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_H_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_SC1_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_H_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_SC2_H_INIT_PHASE_OFFSET], h_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_v_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 v_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_V_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_SC0_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_V_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_SC1_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_V_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_SC2_V_INIT_PHASE_OFFSET], v_offset);
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
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0A + (8 * index)], h_coef_8tap[h_sel].h_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0B + (8 * index)], h_coef_8tap[h_sel].h_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0AB + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0C + (8 * index)], h_coef_8tap[h_sel].h_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0D + (8 * index)], h_coef_8tap[h_sel].h_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0CD + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0E + (8 * index)], h_coef_8tap[h_sel].h_coef_e[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0F + (8 * index)], h_coef_8tap[h_sel].h_coef_f[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0EF + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0G + (8 * index)], h_coef_8tap[h_sel].h_coef_g[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_H_COEFF_0H + (8 * index)], h_coef_8tap[h_sel].h_coef_h[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_H_COEFF_0GH + (4 * index)], reg_value);
		}
		break;
	case MCSC_OUTPUT1:
		for (index = 0; index <= 8; index++) {
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0A + (8 * index)], h_coef_8tap[h_sel].h_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0B + (8 * index)], h_coef_8tap[h_sel].h_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0AB + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0C + (8 * index)], h_coef_8tap[h_sel].h_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0D + (8 * index)], h_coef_8tap[h_sel].h_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0CD + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0E + (8 * index)], h_coef_8tap[h_sel].h_coef_e[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0F + (8 * index)], h_coef_8tap[h_sel].h_coef_f[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0EF + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0G + (8 * index)], h_coef_8tap[h_sel].h_coef_g[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_H_COEFF_0H + (8 * index)], h_coef_8tap[h_sel].h_coef_h[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_H_COEFF_0GH + (4 * index)], reg_value);
		}
		break;
	case MCSC_OUTPUT2:
		for (index = 0; index <= 8; index++) {
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0A + (8 * index)], h_coef_8tap[h_sel].h_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0B + (8 * index)], h_coef_8tap[h_sel].h_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0AB + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0C + (8 * index)], h_coef_8tap[h_sel].h_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0D + (8 * index)], h_coef_8tap[h_sel].h_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0CD + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0E + (8 * index)], h_coef_8tap[h_sel].h_coef_e[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0F + (8 * index)], h_coef_8tap[h_sel].h_coef_f[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0EF + (4 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0G + (8 * index)], h_coef_8tap[h_sel].h_coef_g[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_H_COEFF_0H + (8 * index)], h_coef_8tap[h_sel].h_coef_h[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_H_COEFF_0GH + (4 * index)], reg_value);
		}
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
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_V_COEFF_0A + (4 * index)], v_coef_4tap[v_sel].v_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_V_COEFF_0B + (4 * index)], v_coef_4tap[v_sel].v_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0AB + (2 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_V_COEFF_0C + (4 * index)], v_coef_4tap[v_sel].v_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC0_V_COEFF_0D + (4 * index)], v_coef_4tap[v_sel].v_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC0_V_COEFF_0CD + (2 * index)], reg_value);
		}
		break;
	case MCSC_OUTPUT1:
		for (index = 0; index <= 8; index++) {
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_V_COEFF_0A + (4 * index)], v_coef_4tap[v_sel].v_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_V_COEFF_0B + (4 * index)], v_coef_4tap[v_sel].v_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_0AB + (2 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_V_COEFF_0C + (4 * index)], v_coef_4tap[v_sel].v_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC1_V_COEFF_0D + (4 * index)], v_coef_4tap[v_sel].v_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC1_V_COEFF_0CD + (2 * index)], reg_value);
		}
		break;
	case MCSC_OUTPUT2:
		for (index = 0; index <= 8; index++) {
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_V_COEFF_0A + (4 * index)], v_coef_4tap[v_sel].v_coef_a[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_V_COEFF_0B + (4 * index)], v_coef_4tap[v_sel].v_coef_b[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_0AB + (2 * index)], reg_value);
			reg_value = 0;
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_V_COEFF_0C + (4 * index)], v_coef_4tap[v_sel].v_coef_c[index]);
			reg_value = fimc_is_hw_set_field_value(reg_value,
				&mcsc_fields[MCSC_F_SC2_V_COEFF_0D + (4 * index)], v_coef_4tap[v_sel].v_coef_d[index]);
			fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SC2_V_COEFF_0CD + (2 * index)], reg_value);
		}
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_poly_scaler_coef(void __iomem *base_addr, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coef,
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

void fimc_is_scaler_set_poly_round_mode(void __iomem *base_addr, u32 output_id, u32 mode)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC0_ROUND_MODE],
			&mcsc_fields[MCSC_F_SC0_ROUND_MODE], mode);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC1_ROUND_MODE],
			&mcsc_fields[MCSC_F_SC1_ROUND_MODE], mode);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_SC2_ROUND_MODE],
			&mcsc_fields[MCSC_F_SC2_ROUND_MODE], mode);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaler_enable(void __iomem *base_addr, u32 output_id, u32 enable)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_CTRL], &mcsc_fields[MCSC_F_PC0_ENABLE], enable);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_CTRL], &mcsc_fields[MCSC_F_PC1_ENABLE], enable);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_CTRL], &mcsc_fields[MCSC_F_PC2_ENABLE], enable);
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_IMG_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_IMG_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_IMG_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_IMG_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_IMG_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_IMG_SIZE], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_IMG_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_IMG_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_IMG_SIZE], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_post_img_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_IMG_SIZE],
			&mcsc_fields[MCSC_F_PC0_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_IMG_SIZE],
			&mcsc_fields[MCSC_F_PC0_IMG_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_IMG_SIZE],
			&mcsc_fields[MCSC_F_PC1_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_IMG_SIZE],
			&mcsc_fields[MCSC_F_PC1_IMG_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_IMG_SIZE],
			&mcsc_fields[MCSC_F_PC2_IMG_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_IMG_SIZE],
			&mcsc_fields[MCSC_F_PC2_IMG_VSIZE]);
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_DST_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_DST_SIZE], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_DST_HSIZE], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_DST_VSIZE], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_DST_SIZE], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_post_dst_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_DST_SIZE],
			&mcsc_fields[MCSC_F_PC0_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_DST_SIZE],
			&mcsc_fields[MCSC_F_PC0_DST_VSIZE]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_DST_SIZE],
			&mcsc_fields[MCSC_F_PC1_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_DST_SIZE],
			&mcsc_fields[MCSC_F_PC1_DST_VSIZE]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_DST_SIZE],
			&mcsc_fields[MCSC_F_PC2_DST_HSIZE]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_DST_SIZE],
			&mcsc_fields[MCSC_F_PC2_DST_VSIZE]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaling_ratio(void __iomem *base_addr, u32 output_id, u32 hratio, u32 vratio)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_H_RATIO],
			&mcsc_fields[MCSC_F_PC0_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_V_RATIO],
			&mcsc_fields[MCSC_F_PC0_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_H_RATIO],
			&mcsc_fields[MCSC_F_PC1_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_V_RATIO],
			&mcsc_fields[MCSC_F_PC1_V_RATIO], vratio);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_H_RATIO],
			&mcsc_fields[MCSC_F_PC2_H_RATIO], hratio);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_V_RATIO],
			&mcsc_fields[MCSC_F_PC2_V_RATIO], vratio);
		break;
	default:
		break;
	}
}


void fimc_is_scaler_set_post_h_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 h_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_H_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_PC0_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_H_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_PC1_H_INIT_PHASE_OFFSET], h_offset);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_H_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_PC2_H_INIT_PHASE_OFFSET], h_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_v_init_phase_offset(void __iomem *base_addr, u32 output_id, u32 v_offset)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_V_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_PC0_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_V_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_PC1_V_INIT_PHASE_OFFSET], v_offset);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_V_INIT_PHASE_OFFSET],
			&mcsc_fields[MCSC_F_PC2_V_INIT_PHASE_OFFSET], v_offset);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaler_h_v_coef(void __iomem *base_addr, u32 output_id, u32 h_sel, u32 v_sel)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_H_COEFF_SEL], h_sel);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC0_V_COEFF_SEL], v_sel);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_COEFF_CTRL], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_H_COEFF_SEL], h_sel);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_V_COEFF_SEL], v_sel);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_COEFF_CTRL], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_H_COEFF_SEL], h_sel);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_V_COEFF_SEL], v_sel);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_COEFF_CTRL], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_post_scaler_coef(void __iomem *base_addr, u32 output_id,
	u32 hratio, u32 vratio, struct scaler_coef_cfg *sc_coef)
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

	fimc_is_scaler_set_post_h_init_phase_offset(base_addr, output_id, h_phase_offset);
	fimc_is_scaler_set_post_v_init_phase_offset(base_addr, output_id, v_phase_offset);
	fimc_is_scaler_set_post_scaler_h_v_coef(base_addr, output_id, h_coef, v_coef);
}

void fimc_is_scaler_set_post_round_mode(void __iomem *base_addr, u32 output_id, u32 mode)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_ROUND_MODE],
			&mcsc_fields[MCSC_F_PC0_ROUND_MODE], mode);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_ROUND_MODE],
			&mcsc_fields[MCSC_F_PC1_ROUND_MODE], mode);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_ROUND_MODE],
			&mcsc_fields[MCSC_F_PC2_ROUND_MODE], mode);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_420_conversion(void __iomem *base_addr, u32 output_id, u32 conv420_weight, u32 conv420_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_CONV420_WEIGHT],
			&mcsc_fields[MCSC_F_PC0_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_CONV420_CTRL],
			&mcsc_fields[MCSC_F_PC0_CONV420_ENABLE], conv420_en);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_CONV420_WEIGHT],
			&mcsc_fields[MCSC_F_PC1_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_CONV420_CTRL],
			&mcsc_fields[MCSC_F_PC1_CONV420_ENABLE], conv420_en);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_CONV420_WEIGHT],
			&mcsc_fields[MCSC_F_PC2_CONV420_WEIGHT], conv420_weight);
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_CONV420_CTRL],
			&mcsc_fields[MCSC_F_PC2_CONV420_ENABLE], conv420_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_bchs_enable(void __iomem *base_addr, u32 output_id, bool bchs_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_CTRL],
			&mcsc_fields[MCSC_F_PC0_BCHS_ENABLE], bchs_en);

		/* default BCHS clamp value */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_CLAMP_Y], 0x03FF0000);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC0_BCHS_CLAMP_C], 0x03FF0000);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_CTRL],
			&mcsc_fields[MCSC_F_PC1_BCHS_ENABLE], bchs_en);

		/* default BCHS clamp value */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_CLAMP_Y], 0x03FF0000);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_CLAMP_C], 0x03FF0000);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_CTRL],
			&mcsc_fields[MCSC_F_PC2_BCHS_ENABLE], bchs_en);

		/* default BCHS clamp value */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_CLAMP_Y], 0x03FF0000);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_CLAMP_C], 0x03FF0000);
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_YOFFSET], y_offset);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_YGAIN], y_gain);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_BC], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_YOFFSET], y_offset);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_YGAIN], y_gain);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_BC], reg_value);
		break;
	default:
		break;
	}
}

/* hue/saturation control */
void fimc_is_scaler_set_h_s(void __iomem *base_addr, u32 output_id,
	u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11)
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
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_00], c_gain00);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_HS1], reg_value);
		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_10], c_gain10);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC1_BCHS_C_GAIN_11], c_gain11);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC1_BCHS_HS2], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_00], c_gain00);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_01], c_gain01);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_HS1], reg_value);
		reg_value = 0;
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_10], c_gain10);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_PC2_BCHS_C_GAIN_11], c_gain11);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_PC2_BCHS_HS2], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_bchs_clamp(void __iomem *base_addr, u32 output_id,
	u32 y_max, u32 y_min, u32 c_max, u32 c_min)
{
	u32 reg_value_y = 0, reg_idx_y = 0;
	u32 reg_value_c = 0, reg_idx_c = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value_y = fimc_is_hw_set_field_value(reg_value_y, &mcsc_fields[MCSC_F_PC0_BCHS_Y_CLAMP_MAX], y_max);
		reg_value_y = fimc_is_hw_set_field_value(reg_value_y, &mcsc_fields[MCSC_F_PC0_BCHS_Y_CLAMP_MIN], y_min);
		reg_value_c = fimc_is_hw_set_field_value(reg_value_c, &mcsc_fields[MCSC_F_PC0_BCHS_C_CLAMP_MAX], c_max);
		reg_value_c = fimc_is_hw_set_field_value(reg_value_c, &mcsc_fields[MCSC_F_PC0_BCHS_C_CLAMP_MIN], c_min);
		reg_idx_y = MCSC_R_PC0_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_PC0_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT1:
		reg_value_y = fimc_is_hw_set_field_value(reg_value_y, &mcsc_fields[MCSC_F_PC1_BCHS_Y_CLAMP_MAX], y_max);
		reg_value_y = fimc_is_hw_set_field_value(reg_value_y, &mcsc_fields[MCSC_F_PC1_BCHS_Y_CLAMP_MIN], y_min);
		reg_value_c = fimc_is_hw_set_field_value(reg_value_c, &mcsc_fields[MCSC_F_PC1_BCHS_C_CLAMP_MAX], c_max);
		reg_value_c = fimc_is_hw_set_field_value(reg_value_c, &mcsc_fields[MCSC_F_PC1_BCHS_C_CLAMP_MIN], c_min);
		reg_idx_y = MCSC_R_PC1_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_PC1_BCHS_CLAMP_C;
		break;
	case MCSC_OUTPUT2:
		reg_value_y = fimc_is_hw_set_field_value(reg_value_y, &mcsc_fields[MCSC_F_PC2_BCHS_Y_CLAMP_MAX], y_max);
		reg_value_y = fimc_is_hw_set_field_value(reg_value_y, &mcsc_fields[MCSC_F_PC2_BCHS_Y_CLAMP_MIN], y_min);
		reg_value_c = fimc_is_hw_set_field_value(reg_value_c, &mcsc_fields[MCSC_F_PC2_BCHS_C_CLAMP_MAX], c_max);
		reg_value_c = fimc_is_hw_set_field_value(reg_value_c, &mcsc_fields[MCSC_F_PC2_BCHS_C_CLAMP_MIN], c_min);
		reg_idx_y = MCSC_R_PC2_BCHS_CLAMP_Y;
		reg_idx_c = MCSC_R_PC2_BCHS_CLAMP_C;
		break;
	default:
		return;
	}

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[reg_idx_y], reg_value_y);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[reg_idx_c], reg_value_c);
	dbg_hw(2, "[OUT:%d]set_bchs_clamp_y_c: reg_off(0x%x)= val(0x%x), reg_off(0x%x)= val(0x%x)\n", output_id,
			mcsc_regs[reg_idx_y].sfr_offset, fimc_is_hw_get_reg(base_addr, &mcsc_regs[reg_idx_y]),
			mcsc_regs[reg_idx_c].sfr_offset, fimc_is_hw_get_reg(base_addr, &mcsc_regs[reg_idx_c]));
}

void fimc_is_scaler_set_dma_out_enable(void __iomem *base_addr, u32 output_id, bool dma_out_en)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	case MCSC_OUTPUT_DS:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_DS_DMA_OUT_ENABLE], (u32)dma_out_en);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_otf_out_enable(void __iomem *base_addr, u32 output_id, bool otf_out_en)
{
	/* not support at this version */
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
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0],
			&mcsc_fields[MCSC_F_WDMA0_SRAM_BASE], WDMA0_SRAM_BASE_VALUE);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0],
			&mcsc_fields[MCSC_F_WDMA1_SRAM_BASE], WDMA1_SRAM_BASE_VALUE);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_0],
			&mcsc_fields[MCSC_F_WDMA2_SRAM_BASE], WDMA2_SRAM_BASE_VALUE);
		break;
	case MCSC_OUTPUT_DS:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_DS],
			&mcsc_fields[MCSC_F_WDMADS_SRAM_BASE], WDMADS_SRAM_BASE_VALUE);
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
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC0_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_PC0_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT1:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC1_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_PC1_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT2:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_PC2_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_PC2_DMA_OUT_ENABLE]);
		break;
	case MCSC_OUTPUT_DS:
		ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_DS_DMA_OUT_CTRL],
			&mcsc_fields[MCSC_F_DS_DMA_OUT_ENABLE]);
		break;
	default:
		break;
	}

	return ret;
}

u32 fimc_is_scaler_get_otf_out_enable(void __iomem *base_addr, u32 output_id)
{
	/* not support at this version */
	return 0;
}

void fimc_is_scaler_set_otf_out_path(void __iomem *base_addr, u32 output_id)
{
	/* 0 ~ 2 : otf out path */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_OTF_OUT_PATH_CTRL],
		&mcsc_fields[MCSC_F_OTF_OUT_SEL], output_id);
}

void fimc_is_scaler_set_rdma_format(void __iomem *base_addr, u32 hw_id, u32 dma_in_format)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_DATA_FORMAT],
		&mcsc_fields[MCSC_F_RDMAOTF_DATA_FORMAT], dma_in_format);
}

void fimc_is_scaler_set_rdma_10bit_type(void __iomem *base_addr, u32 dma_in_10bit_type)
{
	u32 dither_en = 0;

	if (dma_in_10bit_type == 1)
		dither_en = 1;

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_10BIT_TYPE],
		&mcsc_fields[MCSC_F_RDMAOTF_10BIT_TYPE], dma_in_10bit_type);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_DITHER],
		&mcsc_fields[MCSC_F_RDMAOTF_DITHER_EN], dither_en);
}

void fimc_is_scaler_set_wdma_rgb_ctrl(void __iomem *base_addr, u32 output_id, u32 mcs_img_format, bool rgb_en)
{
	u32 reg_value = 0;
	u32 rgb_format = mcs_img_format - MCSC_RGB_ARGB8888;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_RGB_EN], rgb_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_RGB_FORMAT], rgb_format);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_RGB_CTRL], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_RGB_EN], rgb_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_RGB_FORMAT], rgb_format);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_RGB_CTRL], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_RGB_EN], rgb_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_RGB_FORMAT], rgb_format);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_RGB_CTRL], reg_value);
		break;
	case MCSC_OUTPUT_DS:
		/* Not support */
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_rgb_coefficient(void __iomem *base_addr, u32 alpha_value)
{
	u32 reg_value = 0;

	/* this value is valid only YUV422 only.  601 wide coefficient is used in JPEG, Pictures, preview. */
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_ALPHA], alpha_value);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_DST_Y_OFFSET], 0);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_SRC_Y_OFFSET], 0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA_RGB_OFFSET], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C00], 256);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C10], 0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA_RGB_COEF_0], reg_value);
	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C20], 369);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C01], 256);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA_RGB_COEF_1], reg_value);
	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C11], 32727);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C21], 32625);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA_RGB_COEF_2], reg_value);
	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C02], 256);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RGB_COEF_C12], 471);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA_RGB_COEF_3], reg_value);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_RGB_COEF_4],
		&mcsc_fields[MCSC_F_RGB_COEF_C22], 0);

}

void fimc_is_scaler_set_wdma_format(void __iomem *base_addr, u32 hw_id, u32 output_id, u32 dma_out_format)
{
	if (dma_out_format == MCSC_RGB_ARGB8888 || dma_out_format == MCSC_RGB_RGBA8888
		|| dma_out_format == MCSC_RGB_RGBA8888 || dma_out_format == MCSC_RGB_ABGR8888) {
		fimc_is_scaler_set_wdma_rgb_coefficient(base_addr, 0);	/* alpha is zero */
		fimc_is_scaler_set_wdma_rgb_ctrl(base_addr, output_id, dma_out_format, true);
		dma_out_format = MCSC_YUV422_1P_YUYV;
	}

	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMA0_DATA_FORMAT], dma_out_format);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMA1_DATA_FORMAT], dma_out_format);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMA2_DATA_FORMAT], dma_out_format);
		break;
	case MCSC_OUTPUT_DS:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMADS_DATA_FORMAT], dma_out_format);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_10bit_type(void __iomem *base_addr, u32 output_id,
	u32 format, u32 bitwidth, enum exynos_sensor_position sensor_position)
{
	u32 dither_en = 0;
	u32 round_en = 0;
	u32 img_10bit_type;
	u32 reg_value = 0;

	if (bitwidth == DMA_OUTPUT_BIT_WIDTH_10BIT)
		img_10bit_type = 1;
	else if (bitwidth == DMA_OUTPUT_BIT_WIDTH_16BIT)
		img_10bit_type = 2;
	else
		img_10bit_type = 0;

	if ((format == DMA_OUTPUT_FORMAT_RGB) || (output_id == MCSC_OUTPUT_DS)) {
		/* DS format is supported only NV61, NV16 */
		dither_en = 0;
		round_en = 1;
	} else if (bitwidth == DMA_OUTPUT_BIT_WIDTH_16BIT) {
		dither_en = 0;
		round_en = 0;
	} else {
		if (sensor_position == SENSOR_POSITION_FRONT) {
			dither_en = 0;
			round_en = 1;
		} else {
			dither_en = 1;
			round_en = 0;
		}
	}

	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_10BIT_TYPE],
			&mcsc_fields[MCSC_F_WDMA0_10BIT_TYPE], img_10bit_type);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_DITHER_EN], dither_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_ROUND_EN], round_en);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_DITHER], reg_value);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_10BIT_TYPE],
			&mcsc_fields[MCSC_F_WDMA1_10BIT_TYPE], img_10bit_type);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_DITHER_EN], dither_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_ROUND_EN], round_en);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_DITHER], reg_value);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_10BIT_TYPE],
			&mcsc_fields[MCSC_F_WDMA2_10BIT_TYPE], img_10bit_type);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_DITHER_EN], dither_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_ROUND_EN], round_en);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_DITHER], reg_value);
		break;
	case MCSC_OUTPUT_DS:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMADS_DITHER_EN], dither_en);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMADS_ROUND_EN], round_en);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_DITHER], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_format(void __iomem *base_addr, u32 output_id, u32 *dma_out_format)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*dma_out_format = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMA0_DATA_FORMAT]);
		break;
	case MCSC_OUTPUT1:
		*dma_out_format = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMA1_DATA_FORMAT]);
		break;
	case MCSC_OUTPUT2:
		*dma_out_format = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMA2_DATA_FORMAT]);
		break;
	case MCSC_OUTPUT_DS:
		*dma_out_format = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_DATA_FORMAT],
			&mcsc_fields[MCSC_F_WDMADS_DATA_FORMAT]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_swap_mode(void __iomem *base_addr, u32 output_id, u32 swap)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_SWAP_TABLE],
			&mcsc_fields[MCSC_F_WDMA0_SWAP_TABLE], swap);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_SWAP_TABLE],
			&mcsc_fields[MCSC_F_WDMA1_SWAP_TABLE], swap);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_SWAP_TABLE],
			&mcsc_fields[MCSC_F_WDMA2_SWAP_TABLE], swap);
		break;
	case MCSC_OUTPUT_DS:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_SWAP_TABLE],
			&mcsc_fields[MCSC_F_WDMADS_SWAP_TABLE], swap);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_flip_mode(void __iomem *base_addr, u32 output_id, u32 flip)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_FLIP_CONTROL],
			&mcsc_fields[MCSC_F_WDMA0_FLIP_CONTROL], flip);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_FLIP_CONTROL],
			&mcsc_fields[MCSC_F_WDMA1_FLIP_CONTROL], flip);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_FLIP_CONTROL],
			&mcsc_fields[MCSC_F_WDMA2_FLIP_CONTROL], flip);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_size(void __iomem *base_addr, u32 width, u32 height)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMAOTF_WIDTH], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMAOTF_HEIGHT], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_IMG_SIZE], reg_value);
}

void fimc_is_scaler_get_rdma_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_IMG_SIZE],
		&mcsc_fields[MCSC_F_RDMAOTF_WIDTH]);
	*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_IMG_SIZE],
		&mcsc_fields[MCSC_F_RDMAOTF_HEIGHT]);
}

void fimc_is_scaler_set_wdma_size(void __iomem *base_addr, u32 output_id, u32 width, u32 height)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_WIDTH], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA0_HEIGHT], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_IMG_SIZE], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_WIDTH], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA1_HEIGHT], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_IMG_SIZE], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_WIDTH], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_HEIGHT], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_IMG_SIZE], reg_value);
		break;
	case MCSC_OUTPUT_DS:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMADS_WIDTH], width);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMADS_HEIGHT], height);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_IMG_SIZE], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_size(void __iomem *base_addr, u32 output_id, u32 *width, u32 *height)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMA0_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMA0_HEIGHT]);
		break;
	case MCSC_OUTPUT1:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMA1_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMA1_HEIGHT]);
		break;
	case MCSC_OUTPUT2:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMA2_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMA2_HEIGHT]);
		break;
	case MCSC_OUTPUT_DS:
		*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMADS_WIDTH]);
		*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMADS_HEIGHT]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_stride(void __iomem *base_addr, u32 y_stride, u32 uv_stride)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMAOTF_Y_STRIDE], y_stride);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMAOTF_C_STRIDE], uv_stride);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_STRIDE], reg_value);
}

void fimc_is_scaler_set_rdma_2bit_stride(void __iomem *base_addr, u32 y_2bit_stride, u32 uv_2bit_stride)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMAOTF_2BIT_Y_STRIDE], y_2bit_stride);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMAOTF_2BIT_C_STRIDE], uv_2bit_stride);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_2BIT_STRIDE], reg_value);
}

void fimc_is_scaler_get_rdma_stride(void __iomem *base_addr, u32 *y_stride, u32 *uv_stride)
{
	*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_STRIDE],
		&mcsc_fields[MCSC_F_RDMAOTF_Y_STRIDE]);
	*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_STRIDE],
		&mcsc_fields[MCSC_F_RDMAOTF_C_STRIDE]);
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
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_Y_STRIDE], y_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMA2_C_STRIDE], uv_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE], reg_value);
		break;
	case MCSC_OUTPUT_DS:
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMADS_Y_STRIDE], y_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_WDMADS_C_STRIDE], uv_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_STRIDE], reg_value);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_2bit_stride(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_stride, u32 uv_2bit_stride)
{
	u32 reg_value = 0;

	switch (output_id) {
	case MCSC_OUTPUT0:
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_WDMA0_2BIT_Y_STRIDE], y_2bit_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_WDMA0_2BIT_C_STRIDE], uv_2bit_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_2BIT_STRIDE], reg_value);
		break;
	case MCSC_OUTPUT1:
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_WDMA1_2BIT_Y_STRIDE], y_2bit_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_WDMA1_2BIT_C_STRIDE], uv_2bit_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_2BIT_STRIDE], reg_value);
		break;
	case MCSC_OUTPUT2:
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_WDMA2_2BIT_Y_STRIDE], y_2bit_stride);
		reg_value = fimc_is_hw_set_field_value(reg_value,
			&mcsc_fields[MCSC_F_WDMA2_2BIT_C_STRIDE], uv_2bit_stride);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_2BIT_STRIDE], reg_value);
		break;
	case MCSC_OUTPUT_DS:
		/* Not supported */
		break;
	default:
		break;
	}
}


void fimc_is_scaler_get_wdma_stride(void __iomem *base_addr, u32 output_id, u32 *y_stride, u32 *uv_stride)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE],
			&mcsc_fields[MCSC_F_WDMA0_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA0_STRIDE],
			&mcsc_fields[MCSC_F_WDMA0_C_STRIDE]);
		break;
	case MCSC_OUTPUT1:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE],
			&mcsc_fields[MCSC_F_WDMA1_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA1_STRIDE],
			&mcsc_fields[MCSC_F_WDMA1_C_STRIDE]);
		break;
	case MCSC_OUTPUT2:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE],
			&mcsc_fields[MCSC_F_WDMA2_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMA2_STRIDE],
			&mcsc_fields[MCSC_F_WDMA2_C_STRIDE]);
		break;
	case MCSC_OUTPUT_DS:
		*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_STRIDE],
			&mcsc_fields[MCSC_F_WDMADS_Y_STRIDE]);
		*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMADS_STRIDE],
			&mcsc_fields[MCSC_F_WDMADS_C_STRIDE]);
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

void fimc_is_scaler_set_rdma_addr(void __iomem *base_addr,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX1], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX1], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX2], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX2], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX2], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX3], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX3], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX3], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX4], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX4], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX4], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX5], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX5], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX5], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX6], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX6], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX6], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX7], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX7], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX7], cr_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_rdma_2bit_addr(void __iomem *base_addr,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1], cbcr_2bit_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX1], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX1], cbcr_2bit_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX2], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX2], cbcr_2bit_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX3], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX3], cbcr_2bit_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX4], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX4], cbcr_2bit_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX5], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX5], cbcr_2bit_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX6], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX6], cbcr_2bit_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX7], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX7], cbcr_2bit_addr);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_set_wdma0_addr(void __iomem *base_addr,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX1], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX1], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX2], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX2], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX2], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX3], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX3], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX3], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX4], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX4], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX4], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX5], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX5], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX5], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX6], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX6], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX6], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX7], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX7], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX7], cr_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma0_2bit_addr(void __iomem *base_addr,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1], cbcr_2bit_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX1], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX1], cbcr_2bit_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX2], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX2], cbcr_2bit_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX3], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX3], cbcr_2bit_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX4], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX4], cbcr_2bit_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX5], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX5], cbcr_2bit_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX6], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX6], cbcr_2bit_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX7], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX7], cbcr_2bit_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma1_addr(void __iomem *base_addr,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX1], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX1], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX2], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX2], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX2], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX3], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX3], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX3], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX4], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX4], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX4], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX5], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX5], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX5], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX6], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX6], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX6], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX7], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX7], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX7], cr_addr);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_set_wdma1_2bit_addr(void __iomem *base_addr,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1], cbcr_2bit_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX1], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX1], cbcr_2bit_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX2], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX2], cbcr_2bit_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX3], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX3], cbcr_2bit_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX4], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX4], cbcr_2bit_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX5], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX5], cbcr_2bit_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX6], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX6], cbcr_2bit_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX7], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX7], cbcr_2bit_addr);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_set_wdma2_addr(void __iomem *base_addr,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX1], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX1], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX2], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX2], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX2], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX3], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX3], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX3], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX4], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX4], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX4], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX5], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX5], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX5], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX6], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX6], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX6], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX7], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX7], cb_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX7], cr_addr);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_set_wdma2_2bit_addr(void __iomem *base_addr,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1], cbcr_2bit_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX1], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX1], cbcr_2bit_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX2], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX2], cbcr_2bit_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX3], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX3], cbcr_2bit_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX4], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX4], cbcr_2bit_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX5], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX5], cbcr_2bit_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX6], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX6], cbcr_2bit_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX7], y_2bit_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX7], cbcr_2bit_addr);
		break;
	default:
		/* only index 0 support */
		break;
	}
}

static void fimc_is_scaler_set_wdmads_addr(void __iomem *base_addr,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1], cb_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX1], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX1], cb_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX2], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX2], cb_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX3], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX3], cb_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX4], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX4], cb_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX5], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX5], cb_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX6], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX6], cb_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX7], y_addr);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX7], cb_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_addr(void __iomem *base_addr, u32 output_id,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
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
	case MCSC_OUTPUT_DS:
		fimc_is_scaler_set_wdmads_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_set_wdma_2bit_addr(void __iomem *base_addr, u32 output_id,
	u32 y_2bit_addr, u32 cbcr_2bit_addr, int buf_index)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_scaler_set_wdma0_2bit_addr(base_addr, y_2bit_addr, cbcr_2bit_addr, buf_index);
		break;
	case MCSC_OUTPUT1:
		fimc_is_scaler_set_wdma1_2bit_addr(base_addr, y_2bit_addr, cbcr_2bit_addr, buf_index);
		break;
	case MCSC_OUTPUT2:
		fimc_is_scaler_set_wdma2_2bit_addr(base_addr, y_2bit_addr, cbcr_2bit_addr, buf_index);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_get_wdmads_addr(void __iomem *base_addr,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1]);
		break;
	case 1:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX1]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX1]);
		break;
	case 2:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX2]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX2]);
		break;
	case 3:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX3]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX3]);
		break;
	case 4:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX4]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX4]);
		break;
	case 5:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX5]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX5]);
		break;
	case 6:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX6]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX6]);
		break;
	case 7:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX7]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX7]);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_get_wdma0_addr(void __iomem *base_addr,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2]);
		break;
	case 1:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX1]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX1]);
		break;
	case 2:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX2]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX2]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX2]);
		break;
	case 3:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX3]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX3]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX3]);
		break;
	case 4:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX4]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX4]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX4]);
		break;
	case 5:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX5]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX5]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX5]);
		break;
	case 6:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX6]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX6]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX6]);
		break;
	case 7:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX7]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX7]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX7]);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_get_wdma1_addr(void __iomem *base_addr,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2]);
		break;
	case 1:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX1]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX1]);
		break;
	case 2:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX2]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX2]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX2]);
		break;
	case 3:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX3]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX3]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX3]);
		break;
	case 4:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX4]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX4]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX4]);
		break;
	case 5:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX5]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX5]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX5]);
		break;
	case 6:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX6]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX6]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX6]);
		break;
	case 7:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX7]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX7]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX7]);
		break;
	default:
		break;
	}
}

static void fimc_is_scaler_get_wdma2_addr(void __iomem *base_addr,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2]);
		break;
	case 1:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX1]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX1]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX1]);
		break;
	case 2:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX2]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX2]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX2]);
		break;
	case 3:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX3]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX3]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX3]);
		break;
	case 4:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX4]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX4]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX4]);
		break;
	case 5:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX5]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX5]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX5]);
		break;
	case 6:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX6]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX6]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX6]);
		break;
	case 7:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX7]);
		*cb_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX7]);
		*cr_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX7]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_wdma_addr(void __iomem *base_addr, u32 output_id,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_scaler_get_wdma0_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	case MCSC_OUTPUT1:
		fimc_is_scaler_get_wdma1_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	case MCSC_OUTPUT2:
		fimc_is_scaler_get_wdma2_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	case MCSC_OUTPUT_DS:
		fimc_is_scaler_get_wdmads_addr(base_addr, y_addr, cb_addr, cr_addr, buf_index);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_rdma_idx_n_addr(void __iomem *base_addr)
{
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX1], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX1], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX1], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX1], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX1], 0x0);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX2], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX2], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX2], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX2], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX2], 0x0);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX3], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX3], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX3], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX3], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX3], 0x0);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX4], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX4], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX4], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX4], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX4], 0x0);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX5], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX5], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX5], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX5], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX5], 0x0);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX6], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX6], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX6], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX6], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX6], 0x0);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0_IDX7], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1_IDX7], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2_IDX7], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0_IDX7], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1_IDX7], 0x0);
}

void fimc_is_scaler_clear_rdma_addr(void __iomem *base_addr)
{
	/* DMA Y address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_0], 0x0);

	/* DMA CB address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_1], 0x0);

	/* DMA CR address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2], 0x0);

	/* DMA 2bit Y, CR address clear */
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_0], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMAOTF_BASE_ADDR_2BIT_1], 0x0);

	/* IDX_n address clear */
	fimc_is_scaler_clear_rdma_idx_n_addr(base_addr);
}

void fimc_is_scaler_clear_wdma_idx_n_addr(void __iomem *base_addr, u32 output_id)
{
	switch (output_id) {
	case MCSC_OUTPUT0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX1], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX2], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX3], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX4], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX5], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX6], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_1_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1_IDX7], 0x0);
		break;
	case MCSC_OUTPUT1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX1], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX2], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX3], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX4], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX5], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX6], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1_IDX7], 0x0);
		break;
	case MCSC_OUTPUT2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX1], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX2], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX3], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX4], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX5], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX6], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1_IDX7], 0x0);
		break;
	case MCSC_OUTPUT_DS:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX1], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX1], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX2], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX2], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX3], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX3], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX4], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX4], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX5], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX5], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX6], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX6], 0x0);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0_IDX7], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1_IDX7], 0x0);
		break;
	default:
		break;
	}
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

		/* DMA 2bit Y, CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_0], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA0_BASE_ADDR_2BIT_1], 0x0);
		break;
	case MCSC_OUTPUT1:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_0], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_1], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2], 0x0);

		/* DMA 2bit Y, CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_0], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA1_BASE_ADDR_2BIT_1], 0x0);
		break;
	case MCSC_OUTPUT2:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_0], 0x0);

		/* DMA CB address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_1], 0x0);

		/* DMA CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2], 0x0);

		/* DMA 2bit Y, CR address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_0], 0x0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMA2_BASE_ADDR_2BIT_1], 0x0);
		break;
	case MCSC_OUTPUT_DS:
		/* DMA Y address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_0], 0x0);

		/* DMA CBCr address clear */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMADS_BASE_ADDR_1], 0x0);
		break;
	default:
		break;
	}
	
	/* IDX_n address clear */
	fimc_is_scaler_clear_wdma_idx_n_addr(base_addr, output_id);
}

/* for tdnr */
void fimc_is_scaler_set_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0], y_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX1], y_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX2], y_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX3], y_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX4], y_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX5], y_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX6], y_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX7], y_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_tdnr_rdma_idx_n_addr(void __iomem *base_addr)
{
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX1], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX2], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX3], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX4], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX5], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX6], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0_IDX7], 0x0);
}

void fimc_is_scaler_clear_tdnr_rdma_addr(void __iomem *base_addr, enum tdnr_buf_type type)
{
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_BASE_ADDR_0], 0x0);
	fimc_is_scaler_clear_tdnr_rdma_idx_n_addr(base_addr);
}

void fimc_is_scaler_set_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0], y_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX1], y_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX2], y_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX3], y_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX4], y_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX5], y_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX6], y_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX7], y_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_get_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 *y_addr, u32 *cb_addr, u32 *cr_addr, int buf_index)
{
	switch (buf_index) {
	case 0:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0]);
		break;
	case 1:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX1]);
		break;
	case 2:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX2]);
		break;
	case 3:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX3]);
		break;
	case 4:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX4]);
		break;
	case 5:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX5]);
		break;
	case 6:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX6]);
		break;
	case 7:
		*y_addr = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX7]);
		break;
	default:
		break;
	}
}

void fimc_is_scaler_clear_tdnr_wdma_idx_n_addr(void __iomem *base_addr)
{
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX1], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX2], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX3], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX4], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX5], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX6], 0x0);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0_IDX7], 0x0);
}

void fimc_is_scaler_clear_tdnr_wdma_addr(void __iomem *base_addr, enum tdnr_buf_type type)
{
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_BASE_ADDR_0], 0x0);
	fimc_is_scaler_clear_tdnr_wdma_idx_n_addr(base_addr);
}

void fimc_is_scaler_set_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMATDNR_WIDTH], width);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMATDNR_HEIGHT], height);
}

void fimc_is_scaler_get_tdnr_wdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMATDNR_WIDTH]);
	*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_IMG_SIZE],
			&mcsc_fields[MCSC_F_WDMATDNR_HEIGHT]);
}

void fimc_is_scaler_set_tdnr_rdma_size(void __iomem *base_addr, enum tdnr_buf_type type, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_IMG_SIZE],
			&mcsc_fields[MCSC_F_RDMATDNR_WIDTH], width);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_IMG_SIZE],
			&mcsc_fields[MCSC_F_RDMATDNR_HEIGHT], height);
}

void fimc_is_scaler_set_tdnr_rdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_in_format)
{
	/* not supported */
}

void fimc_is_scaler_set_tdnr_wdma_format(void __iomem *base_addr, enum tdnr_buf_type type, u32 dma_out_format)
{
	/* not supported */
}

void fimc_is_scaler_set_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 y_stride, u32 uv_stride)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMATDNR_Y_STRIDE], y_stride);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_RDMATDNR_C_STRIDE], uv_stride);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_STRIDE], reg_value);
}

void fimc_is_scaler_get_tdnr_rdma_stride(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 *y_stride, u32 *uv_stride)
{
	*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_STRIDE],
			&mcsc_fields[MCSC_F_RDMATDNR_Y_STRIDE]);
	*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_RDMATDNR_STRIDE],
			&mcsc_fields[MCSC_F_RDMATDNR_C_STRIDE]);
}

void fimc_is_scaler_set_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 y_stride, u32 uv_stride)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_STRIDE],
			&mcsc_fields[MCSC_F_WDMATDNR_Y_STRIDE], y_stride);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_STRIDE],
			&mcsc_fields[MCSC_F_WDMATDNR_C_STRIDE], uv_stride);
}

void fimc_is_scaler_get_tdnr_wdma_stride(void __iomem *base_addr, enum tdnr_buf_type type,
	u32 *y_stride, u32 *uv_stride)
{
	*y_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_STRIDE],
			&mcsc_fields[MCSC_F_WDMATDNR_Y_STRIDE]);
	*uv_stride = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_WDMATDNR_STRIDE],
			&mcsc_fields[MCSC_F_WDMATDNR_C_STRIDE]);
}

void fimc_is_scaler_set_tdnr_wdma_sram_base(void __iomem *base_addr, enum tdnr_buf_type type)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_WDMA_SRAM_BASE_DS],
		&mcsc_fields[MCSC_F_WDMATDNR_SRAM_BASE], WDMATDNR_SRAM_BASE_VALUE);
}

void fimc_is_scaler_set_tdnr_wdma_enable(void __iomem *base_addr, enum tdnr_buf_type type, bool dma_out_en)
{
	/* not supported */
	/* wdma_enable is controlled in mode_select function */
}

void fimc_is_scaler_get_tdnr_image_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_TDNR_DIMENSIONS],
			&mcsc_fields[MCSC_F_TDNR_IMAGE_WIDTH]);
	*height = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_TDNR_DIMENSIONS],
			&mcsc_fields[MCSC_F_TDNR_IMAGE_HEIGHT]);
}

void fimc_is_scaler_set_tdnr_image_size(void __iomem *base_addr, u32 width, u32 height)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IMAGE_WIDTH], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_IMAGE_HEIGHT], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_DIMENSIONS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_YIC_ENC_X_SIZE], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_YIC_ENC_Y_SIZE], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_YIC_ENC_SIZE], reg_value);
	
	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_YIC_DEC_X_SIZE], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_YIC_DEC_Y_SIZE], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_YIC_DEC_SIZE], reg_value);
}

void fimc_is_scaler_set_tdnr_yic_ctrl(void __iomem *base_addr, enum yic_mode yic_mode)
{
	u32 reg_value = 0;

	/* enc_mode> 0: compression,   1: raw mode */
	/* dec_mode> 0: decompression, 1: raw mode */

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_YIC_ENC_MODE], (u32)yic_mode);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_YIC_DEC_MODE], (u32)yic_mode);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_YIC_CTRL], reg_value);
}

void fimc_is_scaler_set_tdnr_mode_select(void __iomem *base_addr, enum tdnr_mode mode)
{
	u32 reg_value = 0;
	u32 tdnr_enable = 0, tdnr_rdma_en = 0, tdnr_wdma_en = 0;
	u32 tdnr_mode = 3; /* 0: 2D (wdma off), 1: First frame (rdma off), 2: Next frame, 3: Bypass */

	switch (mode) {
	case TDNR_MODE_2DNR:
		tdnr_mode = 1; /* First frame */
		tdnr_enable = 0;
		tdnr_wdma_en = 1;
		tdnr_rdma_en = 0;
		fimc_is_scaler_clear_tdnr_rdma_addr(base_addr, TDNR_IMAGE);
		break;
	case TDNR_MODE_3DNR:
		tdnr_mode = 2;
		tdnr_enable = 1;
		tdnr_wdma_en = 1;
		tdnr_rdma_en = 1;
		break;
	case TDNR_MODE_BYPASS:
		tdnr_mode = 3;
		tdnr_enable = 0;
		tdnr_wdma_en = 0;
		tdnr_rdma_en = 0;
		fimc_is_scaler_clear_tdnr_rdma_addr(base_addr, TDNR_IMAGE);
		fimc_is_scaler_clear_tdnr_wdma_addr(base_addr, TDNR_IMAGE);
		break;
	default:
		break;
	}

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_MODE], tdnr_mode);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_WDMA_EN], tdnr_wdma_en);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_RDMA_EN], tdnr_rdma_en);

	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_MODE_SELECTION], reg_value);

#if 0 /* H/W guide: SCALER_ENABLE(0x0100) --> TDNR_RDMA_START(0x11A4) --> SCALER_RDMA_START(0x0128) */
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_RDMA_START],
		&mcsc_fields[MCSC_F_TDNR_RDMA_START], tdnr_enable);
#endif
}

void fimc_is_scaler_set_tdnr_rdma_start(void __iomem *base_addr, enum tdnr_mode mode)
{
	u32 tdnr_rdma_en = 0;

	switch (mode) {
	case TDNR_MODE_BYPASS:
	case TDNR_MODE_2DNR:
		tdnr_rdma_en = 0;
		break;
	case TDNR_MODE_3DNR:
		tdnr_rdma_en = 1;
		break;
	default:
		break;
	}
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_RDMA_START],
		&mcsc_fields[MCSC_F_TDNR_RDMA_START], tdnr_rdma_en);
}

void fimc_is_scaler_set_tdnr_first(void __iomem *base_addr, u32 tdnr_first)
{
	/* not support */
}

void fimc_is_scaler_set_tdnr_tuneset_general(void __iomem *base_addr, struct general_config config)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_AVG_CUR],
			(u32)config.use_average_current);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_AUTO_COEF_3D],
			(u32)config.auto_coeff_3d);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_TEMPORAL_WEIGHTS_CALCULATION_CONFIG], reg_value);

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_ST_BLENDING_TH],
			&mcsc_fields[MCSC_F_TDNR_ST_BLENDING_TH], config.blending_threshold);

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

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_GRID_0],
			config.x_grid_y[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_Y_X_GRID_1],
			config.x_grid_y[ARR3_VAL2]);
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

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_UV_OFFSET], &mcsc_fields[MCSC_F_TDNR_UV_OFFSET],
			dep_config.uv_offset);

	/* noise index independed config */
	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID1],
			indep_config.prev_gridx[ARR3_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID2],
			indep_config.prev_gridx[ARR3_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID3],
			indep_config.prev_gridx[ARR3_VAL3]);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_TDNR_WEIGHT_ENCODER], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID1_LUT],
			indep_config.prev_gridx_lut[ARR4_VAL1]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID2_LUT],
			indep_config.prev_gridx_lut[ARR4_VAL2]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID3_LUT],
			indep_config.prev_gridx_lut[ARR4_VAL3]);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_TDNR_PREV_X_GRID4_LUT],
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

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_TDNR_CONSTANT_LUT_COEFFS_1],
			&mcsc_fields[MCSC_F_TDNR_GRID_X3], config[ARR3_VAL3]);
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
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_ENABLE_AUTO_CLEAR],
			&mcsc_fields[MCSC_F_HWFC_ENABLE_AUTO_CLEAR], auto_clear);
}

void fimc_is_scaler_set_hwfc_idx_reset(void __iomem *base_addr, u32 output_id, bool reset)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_HWFC_INDEX_RESET],
			&mcsc_fields[MCSC_F_HWFC_INDEX_RESET], reset);
}

void fimc_is_scaler_set_hwfc_mode(void __iomem *base_addr, u32 hwfc_output_ids)
{
	u32 val = MCSC_HWFC_MODE_OFF;
	u32 read_val;

	read_val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_MODE],
			&mcsc_fields[MCSC_F_HWFC_MODE]);

	if ((hwfc_output_ids & (1 << MCSC_OUTPUT1)) && (hwfc_output_ids & (1 << MCSC_OUTPUT2))) {
		val = MCSC_HWFC_MODE_REGION_A_B_PORT;
	} else if (hwfc_output_ids & (1 << MCSC_OUTPUT1)) {
		val = MCSC_HWFC_MODE_REGION_A_PORT;
	} else if (hwfc_output_ids & (1 << MCSC_OUTPUT2)) {
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
		val = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_A]);

		/* format */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_FORMAT_A], hwfc_format);

		/* plane */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_PLANE_A], plane);

		/* dma idx */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_ID0_A], dma_idx);
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_ID1_A], dma_idx+1);
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_ID2_A], dma_idx+2);

		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_A], val);

		/* total pixel/byte */
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE0_A], total_img_byte0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE1_A], total_img_byte1);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_IMAGE_BYTE2_A], total_img_byte2);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE0_A], total_width_byte0);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE1_A], total_width_byte1);
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_TOTAL_WIDTH_BYTE2_A], total_width_byte2);
		break;
	case MCSC_OUTPUT2:
		val = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_HWFC_CONFIG_IMAGE_B]);
		/* format */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_FORMAT_B], hwfc_format);

		/* plane */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_PLANE_B], plane);

		/* dma idx */
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_ID0_B], dma_idx);
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_ID1_B], dma_idx+1);
		val = fimc_is_hw_set_field_value(val, &mcsc_fields[MCSC_F_HWFC_ID2_B], dma_idx+2);

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
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_REGION_IDX_BIN],
			&mcsc_fields[MCSC_F_HWFC_REGION_IDX_BIN_A]);
		break;
	case MCSC_OUTPUT2:
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_REGION_IDX_BIN],
			&mcsc_fields[MCSC_F_HWFC_REGION_IDX_BIN_B]);
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
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_CURR_REGION],
			&mcsc_fields[MCSC_F_HWFC_CURR_REGION_A]);
		break;
	case MCSC_OUTPUT2:
		val = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_HWFC_CURR_REGION],
			&mcsc_fields[MCSC_F_HWFC_CURR_REGION_B]);
		break;
	default:
		break;
	}

	return val;
}

/* for YSUM */
void fimc_is_scaler_set_ysum_input_sourece_enable(void __iomem *base_addr, u32 output_id, bool ysum_enable)
{
	/* not support */
}

void fimc_is_scaler_set_ysum_enable(void __iomem *base_addr, bool ysum_enable)
{
	/* not support */
}

void fimc_is_scaler_set_ysum_image_size(void __iomem *base_addr, u32 width, u32 height, u32 start_x, u32 start_y)
{
	/* not support */
}

void fimc_is_scaler_get_ysum_result(void __iomem *base_addr, u32 *luma_sum_msb, u32 *luma_sum_lsb)
{
	/* not support */
}

/* for DJAG */
void fimc_is_scaler_set_djag_enable(void __iomem *base_addr, u32 djag_enable)
{
	/* not support */
}

void fimc_is_scaler_set_djag_input_source(void __iomem *base_addr, u32 djag_input_sel)
{
	/* not support */
}

void fimc_is_scaler_set_djag_src_size(void __iomem *base_addr, u32 width, u32 height)
{
	/* not support */
}

void fimc_is_scaler_set_djag_dst_size(void __iomem *base_addr, u32 width, u32 height)
{
	/* not support */
}

void fimc_is_scaler_set_djag_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio)
{
	/* not support */
}

void fimc_is_scaler_set_djag_init_phase_offset(void __iomem *base_addr, u32 h_offset, u32 v_offset)
{
	/* not support */
}

void fimc_is_scaler_set_djag_round_mode(void __iomem *base_addr, u32 round_enable)
{
	/* not support */
}

void fimc_is_scaler_set_djag_tunning_param(void __iomem *base_addr, const struct djag_setfile_contents *djag_tune)
{
	/* not support */
}

void fimc_is_scaler_set_djag_wb_thres(void __iomem *base_addr, struct djag_wb_thres_cfg *djag_wb)
{
	/* not supported */
}

/* for CAC */
void fimc_is_scaler_set_cac_enable(void __iomem *base_addr, u32 en)
{
	/* not supported */
}

void fimc_is_scaler_set_cac_input_source(void __iomem *base_addr, u32 in)
{
	/* not supported */
}

void fimc_is_scaler_set_cac_map_crt_thr(void __iomem *base_addr, struct cac_cfg_by_ni *cfg)
{
	/* not supported */
}

/* DS */
void fimc_is_scaler_set_ds_enable(void __iomem *base_addr, u32 ds_enable)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_CTRL], &mcsc_fields[MCSC_F_DS_ENABLE], ds_enable);
}

void fimc_is_scaler_set_ds_img_size(void __iomem *base_addr, u32 width, u32 height)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_IMG_HSIZE], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_IMG_VSIZE], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_IMG_SIZE], reg_value);
}

void fimc_is_scaler_set_ds_src_size(void __iomem *base_addr, u32 width, u32 height, u32 x_pos, u32 y_pos)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_CROP_POS_H], x_pos);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_CROP_POS_V], y_pos);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_CROP_POS], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_SRC_HSIZE], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_SRC_VSIZE], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_SRC_SIZE], reg_value);
}

void fimc_is_scaler_set_ds_dst_size(void __iomem *base_addr, u32 width, u32 height)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_DST_HSIZE], width);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_DST_VSIZE], height);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_DST_SIZE], reg_value);
}

void fimc_is_scaler_set_ds_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_H_RATIO], &mcsc_fields[MCSC_F_DS_H_RATIO], hratio);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_V_RATIO], &mcsc_fields[MCSC_F_DS_V_RATIO], vratio);
}

void fimc_is_scaler_set_ds_init_phase_offset(void __iomem *base_addr, u32 h_offset, u32 v_offset)
{
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_H_INIT_PHASE_OFFSET],
		&mcsc_fields[MCSC_F_DS_H_INIT_PHASE_OFFSET], h_offset);
	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_V_INIT_PHASE_OFFSET],
		&mcsc_fields[MCSC_F_DS_V_INIT_PHASE_OFFSET], v_offset);
}

void fimc_is_scaler_set_ds_gamma_table_enable(void __iomem *base_addr, u32 ds_gamma_enable)
{
	u32 reg_value = 0;

	fimc_is_hw_set_field(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_EN],
		&mcsc_fields[MCSC_F_DS_GAMMA_EN], ds_gamma_enable);

/* Gamma table : LUT register with default values : it's a value for 10bit */
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_0], 0);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_1], 16);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_01], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_2], 32);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_3], 48);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_23], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_4], 60);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_5], 112);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_45], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_6], 152);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_7], 192);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_67], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_8], 260);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_9], 316);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_89], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_A], 420);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_B], 512);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_AB], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_C], 608);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_D], 708);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_CD], reg_value);

	reg_value = 0;
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_E], 832);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_DS_GAMMA_TBL_F], 1024);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_DS_GAMMA_TBL_EF], reg_value);
}

/* LFRO : Less Fast Read Out */
void fimc_is_scaler_set_lfro_mode_enable(void __iomem *base_addr, u32 hw_id, u32 lfro_enable, u32 lfro_total_fnum)
{
	u32 reg_value = 0;

	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FAST_MODE_NUM_MINUS1], lfro_total_fnum - 1);
	reg_value = fimc_is_hw_set_field_value(reg_value, &mcsc_fields[MCSC_F_FAST_MODE_EN], lfro_enable);
	fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_FAST_MODE_CTRL], reg_value);
}

u32 fimc_is_scaler_get_lfro_mode_status(void __iomem *base_addr, u32 hw_id)
{
	u32 ret = 0;
	u32 fcnt = 0;

	fcnt = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_FAST_MODE_STATUS], &mcsc_fields[MCSC_F_FAST_MODE_FRAME_CNT]);
	ret = fimc_is_hw_get_field(base_addr, &mcsc_regs[MCSC_R_SCALER_FAST_MODE_STATUS], &mcsc_fields[MCSC_F_FAST_MODE_ERROR_STATUS]);

	if (ret)
		warn_hw("[FRO:%d]frame status: (0x%x)\n", fcnt, ret);

	return ret;
}

static void fimc_is_scaler0_clear_intr_src(void __iomem *base_addr, u32 status)
{
	if (status & (1 << INTR_MC_SCALER_FM_SUB_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_FM_SUB_FRAME_START);

	if (status & (1 << INTR_MC_SCALER_FM_SUB_FRAME_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_FM_SUB_FRAME_FINISH);

	if (status & (1 << INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_SHADOW_COPY_FINISH_OVF);

	if (status & (1 << INTR_MC_SCALER_SHADOW_COPY_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_SHADOW_COPY_FINISH);

	if (status & (1 << INTR_MC_SCALER_INPUT_FRAME_CURSH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_INPUT_FRAME_CURSH);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_INPUT_VERTICAL_OVF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_UNF);

	if (status & (1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_INPUT_HORIZONTAL_OVF);

	if (status & (1 << INTR_MC_SCALER_CORE_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_CORE_FINISH);

	if (status & (1 << INTR_MC_SCALER_WDMA_FINISH))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_WDMA_FINISH);

	if (status & (1 << INTR_MC_SCALER_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_FRAME_START);

	if (status & (1 << INTR_MC_SCALER_FRAME_END))
		fimc_is_hw_set_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT],
			(u32)1 << INTR_MC_SCALER_FRAME_END);
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
		ret = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT_MASK]);
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
		ret = fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_INTERRUPT]);
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
		BUG_ON(1);

		/* TODO: Shadow copy overflow recovery logic */
	}

	return ret;
}

u32 fimc_is_scaler_get_version(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &mcsc_regs[MCSC_R_SCALER_VERSION]);
}

u32 fimc_is_scaler_get_idle_status(void __iomem *base_addr, u32 hw_id)
{
	return fimc_is_hw_get_field(base_addr,
		&mcsc_regs[MCSC_R_SCALER_RUNNING_STATUS], &mcsc_fields[MCSC_F_SCALER_IDLE]);
}

void fimc_is_scaler_dump(void __iomem *base_addr)
{
	u32 i = 0;
	u32 reg_value = 0;

	info_hw("MCSC ver 4.10");

	for (i = 0; i < MCSC_REG_CNT; i++) {
		reg_value = readl(base_addr + mcsc_regs[i].sfr_offset);
		sfrinfo("[DUMP] reg:[%s][0x%04X], value:[0x%08X]\n",
			mcsc_regs[i].reg_name, mcsc_regs[i].sfr_offset, reg_value);
	}
}
