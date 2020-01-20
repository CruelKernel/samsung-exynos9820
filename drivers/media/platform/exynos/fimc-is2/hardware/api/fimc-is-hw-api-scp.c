/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-hw-api-scp.h"
#include "sfr/fimc-is-sfr-scaler-v401.h"

struct scp_v_coef scp_v_coef[7] = {
	/* x8/8 */
	{
		{0, -4, -6, -8, -8, -8, -8, -7, -6},
		{128, 127, 124, 118, 111, 102, 92, 81, 70},
		{0, 5, 11, 19, 27, 37, 48, 59, 70},
		{0, 0, -1, -1, -2, -3, -4, -5, -6},
	},
	/* x7/8 */
	{
		{8, 4, 1, -2, -3, -5, -5, -5, -5},
		{112, 111, 109, 105, 100, 93, 86, 77, 69},
		{8, 14, 20, 27, 34, 43, 51, 60, 69},
		{0, -1, -2, -2, -3, -3, -4, -4, -5},
	},
	/* x6/8 */
	{
		{16, 12, 8, 5, 2, 0, -1, -2, -2},
		{96, 97, 96, 93, 89, 84, 79, 73, 66},
		{16, 21, 26, 32, 39, 46, 53, 59, 66},
		{0, -2, -2, -2, -2, -2, -3, -2, -2},
	},
	/* x5/8 */
	{
		{22, 18, 14, 11, 8, 6, 4, 2, 1},
		{84, 85, 84, 82, 79, 76, 72, 68, 63},
		{22, 26, 31, 36, 42, 47, 52, 58, 63},
		{0, -1, -1, -1, -1, -1, 0, 0, 1},
	},
	/* x4/8 */
	{
		{26, 22, 19, 16, 13, 10, 8, 6, 5},
		{76, 76, 75, 73, 71, 69, 66, 63, 59},
		{26, 30, 34, 38, 43, 47, 51, 55, 59},
		{0, 0, 0, 1, 1, 2, 3, 4, 5},
	},
	/* x3/8 */
	{
		{29, 26, 23, 20, 17, 15, 12, 10, 8},
		{70, 68, 67, 66, 65, 63, 61, 58, 56},
		{29, 32, 36, 39, 43, 46, 50, 53, 56},
		{0, 2, 2, 3, 3, 4, 5, 7, 8},
	},
	/* x2/8 */
	{
		{32, 28, 25, 22, 19, 17, 15, 13, 11},
		{64, 63, 62, 62, 61, 59, 58, 55, 53},
		{32, 34, 37, 40, 43, 46, 48, 51, 53},
		{0, 3, 4, 4, 5, 6, 7, 9, 11},
	}
};

struct scp_h_coef scp_h_coef[7] = {
	/* x8/8 */
	{
		{0, -1, -1, -1, -1, -1, -2, -1, -1},
		{0, 2, 4, 5, 6, 6, 7, 6, 6},
		{0, -6, -12, -15, -18, -20, -21, -20, -20},
		{128, 127, 125, 120, 114, 107, 99, 89, 79},
		{0, 7, 16, 25, 35, 46, 57, 68, 79},
		{0, -2, -5, -8, -10, -13, -16, -18, -20},
		{0, 1, 1, 2, 3, 4, 5, 5, 6},
		{0, 0, 0, 0, -1, -1, -1, -1, -1},
	},
	/* x7/8 */
	{
		{3, 2, 2, 1, 1, 1, 0, 0, 0},
		{-8, -6, -4, -2, -1, 1, 2, 3, 3},
		{14, 7, 1, -3, -7, -11, -13, -15, -16},
		{111, 112, 110, 106, 103, 97, 91, 85, 77},
		{13, 21, 28, 36, 44, 53, 61, 69, 77},
		{-8, -10, -12, -13, -15, -16, -16, -17, -16},
		{3, 3, 4, 4, 4, 4, 4, 4, 3},
		{0, -1, -1, -1, -1, -1, -1, -1, 0},
	},
	/* x6/8 */
	{
		{2, 2, 2, 2, 2, 2, 2, 1, 1},
		{-11, -10, -9, -8, -7, -5, -4, -3, -2},
		{25, 19, 14, 10, 5, 1, -2, -5, -7},
		{96, 96, 94, 92, 90, 86, 82, 77, 72},
		{25, 31, 37, 43, 49, 55, 61, 67, 72},
		{-11, -12, -12, -12, -12, -12, -11, -9, -7},
		{2, 2, 2, 1, 1, 0, -1, -1, -2},
		{0, 0, 0, 0, 0, 1, 1, 1, 1},
	},
	/* x5/8 */
	{
		{-1, -1, 0, 0, 0, 0, 1, 1, 1},
		{-8, -8, -8, -8, -8, -7, -7, -6, -6},
		{33, 28, 24, 20, 16, 13, 10, 6, 4},
		{80, 80, 79, 78, 76, 74, 71, 68, 65},
		{33, 37, 41, 46, 50, 54, 58, 62, 65},
		{-8, -7, -7, -6, -4, -3, -1, 1, 4},
		{-1, -2, -2, -3, -3, -4, -5, -5, -6},
		{0, 1, 1, 1, 1, 1, 1, 1, 1},
	},
	/* x4/8 */
	{
		{-3, -3, -2, -2, -2, -2, -1, -1, -1},
		{0, -1, -2, -3, -3, -3, -4, -4, -4},
		{35, 32, 29, 27, 24, 21, 19, 16, 14},
		{64, 64, 63, 63, 61, 60, 59, 57, 55},
		{35, 38, 41, 43, 46, 49, 51, 53, 55},
		{0, 1, 2, 4, 6, 7, 9, 12, 14},
		{-3, -3, -3, -4, -4, -4, -4, -4, -4},
		{0, 0, 0, 0, 0, 0, -1, -1, -1},
	},
	/* x3/8 */
	{
		{-1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 7, 6, 5, 4, 3, 2, 2, 1},
		{33, 31, 30, 28, 26, 24, 23, 21, 19},
		{48, 49, 49, 48, 48, 47, 47, 45, 45},
		{33, 35, 36, 38, 39, 41, 42, 43, 45},
		{8, 9, 10, 12, 13, 15, 16, 18, 19},
		{-1, -1, -1, -1, 0, 0, 0, 1, 1},
		{0, -1, -1, -1, -1, -1, -1, -1, -1},
	},
	/* x2/8 */
	{
		{2, 2, 2, 2, 1, 1, 1, 1, 1},
		{13, 12, 11, 10, 10, 9, 8, 7, 6},
		{30, 29, 28, 26, 26, 24, 24, 22, 21},
		{38, 38, 38, 38, 37, 37, 37, 36, 36},
		{30, 30, 31, 32, 33, 34, 34, 35, 36},
		{13, 14, 15, 16, 17, 18, 19, 20, 21},
		{2, 3, 3, 4, 4, 5, 5, 6, 6},
		{0, 0, 0, 0, 0, 0, 0, 1, 1},
	}
};

void fimc_is_scp_start(void __iomem *base_addr)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_SCALER_ENABLE], 1);
}

void fimc_is_scp_stop(void __iomem *base_addr)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_SCALER_ENABLE], 0);
}

u32 fimc_is_scp_sw_reset(void __iomem *base_addr)
{
	u32 reset_count = 0;

	/* request scaler reset */
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_SOFTRST_REQ], 1);

	/* wait reset complete */
	do {
		reset_count++;
		if (reset_count > 10000)
			return reset_count;
	} while (fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_SOFTRST_REQ]) != 0);

	return 0;
}

void fimc_is_scp_set_trigger_mode(void __iomem *base_addr, bool isExtTrigger)
{
	/* 0 - frame start/end trigger, 1 - externel trigger(default) */
	if (isExtTrigger)
		fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_SHADOW_TRIGGER_MODE], 1);
	else
		fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_SHADOW_TRIGGER_MODE], 0);
}

void fimc_is_scp_clear_intr_all(void __iomem *base_addr)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<31);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<30);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<24);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<20);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<16);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<12);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<8);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<4);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], 1<<0);
}

void fimc_is_scp_disable_intr(void __iomem *base_addr)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_STALLBLOCKING_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_OUTSTALLBLOCKING_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_CORE_START_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_CORE_END_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_SHADOWTRIGGER_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_SHADOWTRIGGERBYHW_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_SHADOWCONDITION_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_FRAME_START_INTR_MASK], 1);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_INTR_MASK], &scp_fields[SCP_F_FRAME_END_INTR_MASK], 1);
}

void fimc_is_scp_mask_intr(void __iomem *base_addr, u32 intr_mask)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_MASK], intr_mask);
}

void fimc_is_scp_enable_frame_start_trigger(void __iomem *base_addr, bool enable)
{
	/* 0 - frame end trigger, 1 - frame start trigger(default) */
	if (enable)
		fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_FRAME_START_TRIG_EN], 1);
	else
		fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_GCTRL], &scp_fields[SCP_F_FRAME_START_TRIG_EN], 0);
}

void fimc_is_scp_set_pre_size(void __iomem *base_addr, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_PRE_SIZE], &scp_fields[SCP_F_PRE_HSIZE], width);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_PRE_SIZE], &scp_fields[SCP_F_PRE_VSIZE], height);
}

void fimc_is_scp_get_pre_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_PRE_SIZE], &scp_fields[SCP_F_PRE_HSIZE]);
	*height = fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_PRE_SIZE], &scp_fields[SCP_F_PRE_VSIZE]);
}

void fimc_is_scp_set_img_size(void __iomem *base_addr, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_IMG_SIZE], &scp_fields[SCP_F_IMG_HSIZE], width);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_IMG_SIZE], &scp_fields[SCP_F_IMG_VSIZE], height);
}

void fimc_is_scp_set_src_size(void __iomem *base_addr, u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_SRC_POS], &scp_fields[SCP_F_SRC_HPOS], pos_x);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_SRC_POS], &scp_fields[SCP_F_SRC_VPOS], pos_y);

	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_SRC_SIZE], &scp_fields[SCP_F_SRC_HSIZE], width);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_SRC_SIZE], &scp_fields[SCP_F_SRC_VSIZE], height);
}
void fimc_is_scp_get_src_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_SRC_SIZE], &scp_fields[SCP_F_SRC_HSIZE]);
	*height = fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_SRC_SIZE], &scp_fields[SCP_F_SRC_VSIZE]);
}
void fimc_is_scp_set_dst_size(void __iomem *base_addr, u32 width, u32 height)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DST_SIZE], &scp_fields[SCP_F_DST_HSIZE], width);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DST_SIZE], &scp_fields[SCP_F_DST_VSIZE], height);
}

void fimc_is_scp_get_dst_size(void __iomem *base_addr, u32 *width, u32 *height)
{
	*width = fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_DST_SIZE], &scp_fields[SCP_F_DST_HSIZE]);
	*height = fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_DST_SIZE], &scp_fields[SCP_F_DST_VSIZE]);
}

void fimc_is_scp_set_pre_scaling_ratio(void __iomem *base_addr, u32 sh_factor, u32 hratio, u32 vratio)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_PRE_RATIO], &scp_fields[SCP_F_PRE_SHFACTOR], sh_factor);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_PRE_RATIO], &scp_fields[SCP_F_PRE_HRATIO], hratio);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_PRE_RATIO], &scp_fields[SCP_F_PRE_VRATIO], vratio);
}

void fimc_is_scp_set_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_H_RATIO], &scp_fields[SCP_F_H_RATIO], hratio);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_V_RATIO], &scp_fields[SCP_F_V_RATIO], vratio);
}

void fimc_is_scp_set_horizontal_coef(void __iomem *base_addr, u32 hratio, u32 vratio)
{
	struct scp_h_coef y_h_coef, cb_h_coef, cr_h_coef;

	/* adjust H coef */
	if (hratio <= RATIO_X8_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[0];
	else if (hratio > RATIO_X8_8 && hratio <= RATIO_X7_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[1];
	else if (hratio > RATIO_X7_8 && hratio <= RATIO_X6_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[2];
	else if (hratio > RATIO_X6_8 && hratio <= RATIO_X5_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[3];
	else if (hratio > RATIO_X5_8 && hratio <= RATIO_X4_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[4];
	else if (hratio > RATIO_X4_8 && hratio <= RATIO_X3_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[5];
	else if (hratio > RATIO_X3_8 && hratio <= RATIO_X2_8)
		y_h_coef = cb_h_coef = cr_h_coef = scp_h_coef[6];
	else
		return;

	/* set coefficient for Y/Cb/Cr horizontal scaling filter */
	fimc_is_scp_set_y_h_coef(base_addr, &y_h_coef);
	fimc_is_scp_set_cb_h_coef(base_addr, &cb_h_coef);
	fimc_is_scp_set_cr_h_coef(base_addr, &cr_h_coef);
}

void fimc_is_scp_set_vertical_coef(void __iomem *base_addr, u32 hratio, u32 vratio)
{
	struct scp_v_coef y_v_coef, cb_v_coef, cr_v_coef;

	/* adjust V coef */
	if (vratio <= RATIO_X8_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[0];
	if (vratio > RATIO_X8_8 && vratio <= RATIO_X7_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[1];
	if (vratio > RATIO_X7_8 && vratio <= RATIO_X6_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[2];
	if (vratio > RATIO_X6_8 && vratio <= RATIO_X5_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[3];
	if (vratio > RATIO_X5_8 && vratio <= RATIO_X4_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[4];
	if (vratio > RATIO_X4_8 && vratio <= RATIO_X3_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[5];
	if (vratio > RATIO_X3_8 && vratio <= RATIO_X2_8)
		y_v_coef = cb_v_coef = cr_v_coef = scp_v_coef[6];
	else
		return;

	/* set coefficient for Y/Cb/Cr vertical scaling filter */
	fimc_is_scp_set_y_v_coef(base_addr, &y_v_coef);
	fimc_is_scp_set_cb_v_coef(base_addr, &cb_v_coef);
	fimc_is_scp_set_cr_v_coef(base_addr, &cr_v_coef);
}

void fimc_is_scp_set_y_h_coef(void __iomem *base_addr, struct scp_h_coef *y_h_coef)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0A], y_h_coef->h_coef_a[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1A], y_h_coef->h_coef_a[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2A], y_h_coef->h_coef_a[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3A], y_h_coef->h_coef_a[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4A], y_h_coef->h_coef_a[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5A], y_h_coef->h_coef_a[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6A], y_h_coef->h_coef_a[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7A], y_h_coef->h_coef_a[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8A], y_h_coef->h_coef_a[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0B], y_h_coef->h_coef_b[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1B], y_h_coef->h_coef_b[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2B], y_h_coef->h_coef_b[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3B], y_h_coef->h_coef_b[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4B], y_h_coef->h_coef_b[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5B], y_h_coef->h_coef_b[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6B], y_h_coef->h_coef_b[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7B], y_h_coef->h_coef_b[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8B], y_h_coef->h_coef_b[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0C], y_h_coef->h_coef_c[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1C], y_h_coef->h_coef_c[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2C], y_h_coef->h_coef_c[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3C], y_h_coef->h_coef_c[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4C], y_h_coef->h_coef_c[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5C], y_h_coef->h_coef_c[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6C], y_h_coef->h_coef_c[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7C], y_h_coef->h_coef_c[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8C], y_h_coef->h_coef_c[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0D], y_h_coef->h_coef_d[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1D], y_h_coef->h_coef_d[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2D], y_h_coef->h_coef_d[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3D], y_h_coef->h_coef_d[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4D], y_h_coef->h_coef_d[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5D], y_h_coef->h_coef_d[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6D], y_h_coef->h_coef_d[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7D], y_h_coef->h_coef_d[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8D], y_h_coef->h_coef_d[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0E], y_h_coef->h_coef_e[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1E], y_h_coef->h_coef_e[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2E], y_h_coef->h_coef_e[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3E], y_h_coef->h_coef_e[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4E], y_h_coef->h_coef_e[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5E], y_h_coef->h_coef_e[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6E], y_h_coef->h_coef_e[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7E], y_h_coef->h_coef_e[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8E], y_h_coef->h_coef_e[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0F], y_h_coef->h_coef_f[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1F], y_h_coef->h_coef_f[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2F], y_h_coef->h_coef_f[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3F], y_h_coef->h_coef_f[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4F], y_h_coef->h_coef_f[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5F], y_h_coef->h_coef_f[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6F], y_h_coef->h_coef_f[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7F], y_h_coef->h_coef_f[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8F], y_h_coef->h_coef_f[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0G], y_h_coef->h_coef_g[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1G], y_h_coef->h_coef_g[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2G], y_h_coef->h_coef_g[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3G], y_h_coef->h_coef_g[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4G], y_h_coef->h_coef_g[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5G], y_h_coef->h_coef_g[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6G], y_h_coef->h_coef_g[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7G], y_h_coef->h_coef_g[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8G], y_h_coef->h_coef_g[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_0H], y_h_coef->h_coef_h[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_1H], y_h_coef->h_coef_h[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_2H], y_h_coef->h_coef_h[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_3H], y_h_coef->h_coef_h[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_4H], y_h_coef->h_coef_h[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_5H], y_h_coef->h_coef_h[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_6H], y_h_coef->h_coef_h[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_7H], y_h_coef->h_coef_h[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_HCOEF_8H], y_h_coef->h_coef_h[8]);
}

void fimc_is_scp_set_cb_h_coef(void __iomem *base_addr, struct scp_h_coef *cb_h_coef)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0A], cb_h_coef->h_coef_a[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1A], cb_h_coef->h_coef_a[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2A], cb_h_coef->h_coef_a[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3A], cb_h_coef->h_coef_a[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4A], cb_h_coef->h_coef_a[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5A], cb_h_coef->h_coef_a[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6A], cb_h_coef->h_coef_a[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7A], cb_h_coef->h_coef_a[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8A], cb_h_coef->h_coef_a[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0B], cb_h_coef->h_coef_b[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1B], cb_h_coef->h_coef_b[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2B], cb_h_coef->h_coef_b[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3B], cb_h_coef->h_coef_b[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4B], cb_h_coef->h_coef_b[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5B], cb_h_coef->h_coef_b[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6B], cb_h_coef->h_coef_b[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7B], cb_h_coef->h_coef_b[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8B], cb_h_coef->h_coef_b[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0C], cb_h_coef->h_coef_c[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1C], cb_h_coef->h_coef_c[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2C], cb_h_coef->h_coef_c[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3C], cb_h_coef->h_coef_c[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4C], cb_h_coef->h_coef_c[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5C], cb_h_coef->h_coef_c[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6C], cb_h_coef->h_coef_c[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7C], cb_h_coef->h_coef_c[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8C], cb_h_coef->h_coef_c[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0D], cb_h_coef->h_coef_d[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1D], cb_h_coef->h_coef_d[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2D], cb_h_coef->h_coef_d[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3D], cb_h_coef->h_coef_d[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4D], cb_h_coef->h_coef_d[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5D], cb_h_coef->h_coef_d[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6D], cb_h_coef->h_coef_d[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7D], cb_h_coef->h_coef_d[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8D], cb_h_coef->h_coef_d[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0E], cb_h_coef->h_coef_e[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1E], cb_h_coef->h_coef_e[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2E], cb_h_coef->h_coef_e[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3E], cb_h_coef->h_coef_e[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4E], cb_h_coef->h_coef_e[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5E], cb_h_coef->h_coef_e[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6E], cb_h_coef->h_coef_e[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7E], cb_h_coef->h_coef_e[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8E], cb_h_coef->h_coef_e[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0F], cb_h_coef->h_coef_f[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1F], cb_h_coef->h_coef_f[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2F], cb_h_coef->h_coef_f[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3F], cb_h_coef->h_coef_f[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4F], cb_h_coef->h_coef_f[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5F], cb_h_coef->h_coef_f[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6F], cb_h_coef->h_coef_f[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7F], cb_h_coef->h_coef_f[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8F], cb_h_coef->h_coef_f[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0G], cb_h_coef->h_coef_g[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1G], cb_h_coef->h_coef_g[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2G], cb_h_coef->h_coef_g[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3G], cb_h_coef->h_coef_g[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4G], cb_h_coef->h_coef_g[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5G], cb_h_coef->h_coef_g[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6G], cb_h_coef->h_coef_g[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7G], cb_h_coef->h_coef_g[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8G], cb_h_coef->h_coef_g[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_0H], cb_h_coef->h_coef_h[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_1H], cb_h_coef->h_coef_h[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_2H], cb_h_coef->h_coef_h[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_3H], cb_h_coef->h_coef_h[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_4H], cb_h_coef->h_coef_h[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_5H], cb_h_coef->h_coef_h[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_6H], cb_h_coef->h_coef_h[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_7H], cb_h_coef->h_coef_h[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_HCOEF_8H], cb_h_coef->h_coef_h[8]);
}

void fimc_is_scp_set_cr_h_coef(void __iomem *base_addr, struct scp_h_coef *cr_h_coef)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0A], cr_h_coef->h_coef_a[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1A], cr_h_coef->h_coef_a[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2A], cr_h_coef->h_coef_a[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3A], cr_h_coef->h_coef_a[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4A], cr_h_coef->h_coef_a[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5A], cr_h_coef->h_coef_a[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6A], cr_h_coef->h_coef_a[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7A], cr_h_coef->h_coef_a[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8A], cr_h_coef->h_coef_a[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0B], cr_h_coef->h_coef_b[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1B], cr_h_coef->h_coef_b[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2B], cr_h_coef->h_coef_b[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3B], cr_h_coef->h_coef_b[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4B], cr_h_coef->h_coef_b[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5B], cr_h_coef->h_coef_b[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6B], cr_h_coef->h_coef_b[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7B], cr_h_coef->h_coef_b[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8B], cr_h_coef->h_coef_b[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0C], cr_h_coef->h_coef_c[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1C], cr_h_coef->h_coef_c[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2C], cr_h_coef->h_coef_c[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3C], cr_h_coef->h_coef_c[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4C], cr_h_coef->h_coef_c[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5C], cr_h_coef->h_coef_c[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6C], cr_h_coef->h_coef_c[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7C], cr_h_coef->h_coef_c[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8C], cr_h_coef->h_coef_c[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0D], cr_h_coef->h_coef_d[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1D], cr_h_coef->h_coef_d[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2D], cr_h_coef->h_coef_d[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3D], cr_h_coef->h_coef_d[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4D], cr_h_coef->h_coef_d[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5D], cr_h_coef->h_coef_d[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6D], cr_h_coef->h_coef_d[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7D], cr_h_coef->h_coef_d[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8D], cr_h_coef->h_coef_d[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0E], cr_h_coef->h_coef_e[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1E], cr_h_coef->h_coef_e[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2E], cr_h_coef->h_coef_e[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3E], cr_h_coef->h_coef_e[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4E], cr_h_coef->h_coef_e[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5E], cr_h_coef->h_coef_e[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6E], cr_h_coef->h_coef_e[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7E], cr_h_coef->h_coef_e[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8E], cr_h_coef->h_coef_e[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0F], cr_h_coef->h_coef_f[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1F], cr_h_coef->h_coef_f[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2F], cr_h_coef->h_coef_f[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3F], cr_h_coef->h_coef_f[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4F], cr_h_coef->h_coef_f[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5F], cr_h_coef->h_coef_f[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6F], cr_h_coef->h_coef_f[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7F], cr_h_coef->h_coef_f[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8F], cr_h_coef->h_coef_f[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0G], cr_h_coef->h_coef_g[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1G], cr_h_coef->h_coef_g[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2G], cr_h_coef->h_coef_g[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3G], cr_h_coef->h_coef_g[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4G], cr_h_coef->h_coef_g[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5G], cr_h_coef->h_coef_g[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6G], cr_h_coef->h_coef_g[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7G], cr_h_coef->h_coef_g[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8G], cr_h_coef->h_coef_g[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_0H], cr_h_coef->h_coef_h[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_1H], cr_h_coef->h_coef_h[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_2H], cr_h_coef->h_coef_h[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_3H], cr_h_coef->h_coef_h[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_4H], cr_h_coef->h_coef_h[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_5H], cr_h_coef->h_coef_h[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_6H], cr_h_coef->h_coef_h[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_7H], cr_h_coef->h_coef_h[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_HCOEF_8H], cr_h_coef->h_coef_h[8]);
}

void fimc_is_scp_set_y_v_coef(void __iomem *base_addr, struct scp_v_coef *y_v_coef)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_0A], y_v_coef->v_coef_a[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_1A], y_v_coef->v_coef_a[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_2A], y_v_coef->v_coef_a[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_3A], y_v_coef->v_coef_a[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_4A], y_v_coef->v_coef_a[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_5A], y_v_coef->v_coef_a[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_6A], y_v_coef->v_coef_a[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_7A], y_v_coef->v_coef_a[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_8A], y_v_coef->v_coef_a[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_0B], y_v_coef->v_coef_b[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_1B], y_v_coef->v_coef_b[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_2B], y_v_coef->v_coef_b[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_3B], y_v_coef->v_coef_b[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_4B], y_v_coef->v_coef_b[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_5B], y_v_coef->v_coef_b[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_6B], y_v_coef->v_coef_b[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_7B], y_v_coef->v_coef_b[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_8B], y_v_coef->v_coef_b[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_0C], y_v_coef->v_coef_c[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_1C], y_v_coef->v_coef_c[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_2C], y_v_coef->v_coef_c[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_3C], y_v_coef->v_coef_c[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_4C], y_v_coef->v_coef_c[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_5C], y_v_coef->v_coef_c[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_6C], y_v_coef->v_coef_c[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_7C], y_v_coef->v_coef_c[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_8C], y_v_coef->v_coef_c[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_0D], y_v_coef->v_coef_d[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_1D], y_v_coef->v_coef_d[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_2D], y_v_coef->v_coef_d[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_3D], y_v_coef->v_coef_d[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_4D], y_v_coef->v_coef_d[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_5D], y_v_coef->v_coef_d[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_6D], y_v_coef->v_coef_d[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_7D], y_v_coef->v_coef_d[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_Y_VCOEF_8D], y_v_coef->v_coef_d[8]);
}

void fimc_is_scp_set_cb_v_coef(void __iomem *base_addr, struct scp_v_coef *cb_v_coef)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_0A], cb_v_coef->v_coef_a[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_1A], cb_v_coef->v_coef_a[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_2A], cb_v_coef->v_coef_a[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_3A], cb_v_coef->v_coef_a[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_4A], cb_v_coef->v_coef_a[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_5A], cb_v_coef->v_coef_a[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_6A], cb_v_coef->v_coef_a[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_7A], cb_v_coef->v_coef_a[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_8A], cb_v_coef->v_coef_a[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_0B], cb_v_coef->v_coef_b[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_1B], cb_v_coef->v_coef_b[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_2B], cb_v_coef->v_coef_b[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_3B], cb_v_coef->v_coef_b[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_4B], cb_v_coef->v_coef_b[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_5B], cb_v_coef->v_coef_b[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_6B], cb_v_coef->v_coef_b[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_7B], cb_v_coef->v_coef_b[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_8B], cb_v_coef->v_coef_b[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_0C], cb_v_coef->v_coef_c[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_1C], cb_v_coef->v_coef_c[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_2C], cb_v_coef->v_coef_c[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_3C], cb_v_coef->v_coef_c[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_4C], cb_v_coef->v_coef_c[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_5C], cb_v_coef->v_coef_c[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_6C], cb_v_coef->v_coef_c[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_7C], cb_v_coef->v_coef_c[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_8C], cb_v_coef->v_coef_c[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_0D], cb_v_coef->v_coef_d[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_1D], cb_v_coef->v_coef_d[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_2D], cb_v_coef->v_coef_d[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_3D], cb_v_coef->v_coef_d[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_4D], cb_v_coef->v_coef_d[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_5D], cb_v_coef->v_coef_d[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_6D], cb_v_coef->v_coef_d[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_7D], cb_v_coef->v_coef_d[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CB_VCOEF_8D], cb_v_coef->v_coef_d[8]);
}

void fimc_is_scp_set_cr_v_coef(void __iomem *base_addr, struct scp_v_coef *cr_v_coef)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_0A], cr_v_coef->v_coef_a[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_1A], cr_v_coef->v_coef_a[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_2A], cr_v_coef->v_coef_a[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_3A], cr_v_coef->v_coef_a[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_4A], cr_v_coef->v_coef_a[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_5A], cr_v_coef->v_coef_a[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_6A], cr_v_coef->v_coef_a[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_7A], cr_v_coef->v_coef_a[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_8A], cr_v_coef->v_coef_a[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_0B], cr_v_coef->v_coef_b[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_1B], cr_v_coef->v_coef_b[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_2B], cr_v_coef->v_coef_b[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_3B], cr_v_coef->v_coef_b[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_4B], cr_v_coef->v_coef_b[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_5B], cr_v_coef->v_coef_b[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_6B], cr_v_coef->v_coef_b[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_7B], cr_v_coef->v_coef_b[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_8B], cr_v_coef->v_coef_b[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_0C], cr_v_coef->v_coef_c[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_1C], cr_v_coef->v_coef_c[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_2C], cr_v_coef->v_coef_c[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_3C], cr_v_coef->v_coef_c[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_4C], cr_v_coef->v_coef_c[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_5C], cr_v_coef->v_coef_c[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_6C], cr_v_coef->v_coef_c[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_7C], cr_v_coef->v_coef_c[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_8C], cr_v_coef->v_coef_c[8]);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_0D], cr_v_coef->v_coef_d[0]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_1D], cr_v_coef->v_coef_d[1]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_2D], cr_v_coef->v_coef_d[2]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_3D], cr_v_coef->v_coef_d[3]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_4D], cr_v_coef->v_coef_d[4]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_5D], cr_v_coef->v_coef_d[5]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_6D], cr_v_coef->v_coef_d[6]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_7D], cr_v_coef->v_coef_d[7]);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_CR_VCOEF_8D], cr_v_coef->v_coef_d[8]);
}

void fimc_is_scp_set_canv_size(void __iomem *base_addr, u32 y_h_size, u32 y_v_size, u32 c_h_size, u32 c_v_size)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CANV_YHSIZE], &scp_fields[SCP_F_DMA_YHSIZE], y_h_size);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CANV_YVSIZE], &scp_fields[SCP_F_DMA_YVSIZE], y_v_size);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CANV_CHSIZE], &scp_fields[SCP_F_DMA_CHSIZE], c_h_size);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CANV_CVSIZE], &scp_fields[SCP_F_DMA_CVSIZE], c_v_size);
}

void fimc_is_scp_set_dma_offset(void __iomem *base_addr,
	u32 y_h_offset, u32 y_v_offset,
	u32 cb_h_offset, u32 cb_v_offset,
	u32 cr_h_offset, u32 cr_v_offset)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_YHOFFSET], &scp_fields[SCP_F_DMA_YHOFF], y_h_offset);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_YVOFFSET], &scp_fields[SCP_F_DMA_YVOFF], y_v_offset);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CBHOFFSET], &scp_fields[SCP_F_DMA_CBHOFF], cb_h_offset);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CBVOFFSET], &scp_fields[SCP_F_DMA_CBVOFF], cb_v_offset);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CRHOFFSET], &scp_fields[SCP_F_DMA_CRHOFF], cr_h_offset);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CRVOFFSET], &scp_fields[SCP_F_DMA_CRVOFF], cr_v_offset);
}

void fimc_is_scp_set_flip_mode(void __iomem *base_addr, u32 flip)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_FLIP_MODE_CTRL], flip);
}

void fimc_is_scp_set_dither(void __iomem *base_addr, bool dither_en)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_BC], &scp_fields[SCP_F_DITHER_ON], dither_en);
}

/* brightness/contrast control */
void fimc_is_scp_set_b_c(void __iomem *base_addr, u32 y_offset, u32 y_gain)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_BC], &scp_fields[SCP_F_BCHS_Y_OFFSET], y_offset);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_BC], &scp_fields[SCP_F_BCHS_Y_GAIN], y_gain);
}

/* hue/saturation control */
void fimc_is_scp_set_h_s(void __iomem *base_addr, u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_HS1], &scp_fields[SCP_F_BCHS_C_GAIN_00], c_gain00);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_HS1], &scp_fields[SCP_F_BCHS_C_GAIN_01], c_gain01);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_HS2], &scp_fields[SCP_F_BCHS_C_GAIN_10], c_gain10);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_HS2], &scp_fields[SCP_F_BCHS_C_GAIN_11], c_gain11);
}

void fimc_is_scp_set_direct_out_path(void __iomem *base_addr, u32 direct_sel, bool direct_out_en)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DIRECT_CON], &scp_fields[SCP_F_DIRECT_SEL], direct_sel);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DIRECT_CON], &scp_fields[SCP_F_DIRECTOUT_ENABLE], direct_out_en);
}

void fimc_is_scp_set_dma_out_path(void __iomem *base_addr, bool dma_out_en, u32 dma_sel)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_DMA_EN], dma_out_en);
	if (dma_out_en == 1)
		fimc_is_hw_set_field(base_addr,  &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_SC_DMA_SEL], dma_sel);
}

void fimc_is_scp_set_dma_enable(void __iomem *base_addr, bool enable)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_DMA_EN], enable);
}

void fimc_is_scp_set_dma_out_frame_seq(void __iomem *base_addr, u32 frame_seq)
{
	if (!(frame_seq <= 0xffffffff))
		return;

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CNTSEQ], frame_seq);
}

void fimc_is_scp_set_dma_out_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index)
{
	/* DMA Y, Cb and Cr address setting for matched index regs */
	switch (buf_index) {
	case 0:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR1], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR1], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR1], cr_addr);
		break;
	case 1:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR2], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR2], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR2], cr_addr);
		break;
	case 2:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR3], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR3], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR3], cr_addr);
		break;
	case 3:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR4], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR4], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR4], cr_addr);
		break;
	case 4:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR5], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR5], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR5], cr_addr);
		break;
	case 5:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR6], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR6], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR6], cr_addr);
		break;
	case 6:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR7], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR7], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR7], cr_addr);
		break;
	case 7:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR8], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR8], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR8], cr_addr);
		break;
	case 8:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR9], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR9], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR9], cr_addr);
		break;
	case 9:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR10], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR10], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR10], cr_addr);
		break;
	case 10:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR11], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR11], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR11], cr_addr);
		break;
	case 11:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR12], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR12], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR12], cr_addr);
		break;
	case 12:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR13], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR13], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR13], cr_addr);
		break;
	case 13:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR14], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR14], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR14], cr_addr);
		break;
	case 14:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR15], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR15], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR15], cr_addr);
		break;
	case 15:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR16], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR16], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR16], cr_addr);
		break;
	case 16:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR17], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR17], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR17], cr_addr);
		break;
	case 17:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR18], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR18], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR18], cr_addr);
		break;
	case 18:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR19], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR19], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR19], cr_addr);
		break;
	case 19:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR20], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR20], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR20], cr_addr);
		break;
	case 20:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR21], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR21], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR21], cr_addr);
		break;
	case 21:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR22], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR22], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR22], cr_addr);
		break;
	case 22:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR23], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR23], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR23], cr_addr);
		break;
	case 23:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR24], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR24], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR24], cr_addr);
		break;
	case 24:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR25], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR25], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR25], cr_addr);
		break;
	case 25:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR26], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR26], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR26], cr_addr);
		break;
	case 26:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR27], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR27], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR27], cr_addr);
		break;
	case 27:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR28], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR28], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR28], cr_addr);
		break;
	case 28:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR29], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR29], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR29], cr_addr);
		break;
	case 29:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR30], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR30], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR30], cr_addr);
		break;
	case 30:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR31], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR31], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR31], cr_addr);
		break;
	case 31:
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR32], y_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR32], cb_addr);
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR32], cr_addr);
		break;
	default:
		break;
	}
}

void fimc_is_scp_clear_dma_out_addr(void __iomem *base_addr)
{
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR1], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR1], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR1], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR2], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR2], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR2], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR3], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR3], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR3], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR4], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR4], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR4], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR5], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR5], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR5], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR6], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR6], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR6], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR7], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR7], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR7], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR8], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR8], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR8], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR9], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR9], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR9], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR10], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR10], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR10], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR11], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR11], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR11], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR12], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR12], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR12], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR13], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR13], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR13], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR14], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR14], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR14], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR15], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR15], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR15], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR16], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR16], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR16], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR17], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR17], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR17], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR18], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR18], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR18], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR19], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR19], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR19], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR20], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR20], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR20], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR21], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR21], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR21], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR22], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR22], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR22], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR23], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR23], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR23], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR24], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR24], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR24], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR25], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR25], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR25], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR26], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR26], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR26], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR27], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR27], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR27], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR28], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR28], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR28], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR29], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR29], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR29], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR30],0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR30], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR30], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR31], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR31], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR31], 0);

	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_YADDR32], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CBADDR32], 0);
	fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_DMA_CRADDR32], 0);
}

void fimc_is_scp_set_420_conversion(void __iomem *base_addr, u32 conv420_weight, bool conv420_en)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_CONV420], &scp_fields[SCP_F_CONV420_WEIGHT], conv420_weight);
	if (conv420_en == true)
		fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_CONV420], &scp_fields[SCP_F_CONV420_ENABLE], 1);
	else
		fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_CONV420], &scp_fields[SCP_F_CONV420_ENABLE], 0);
}

void fimc_is_scp_set_dma_out_img_fmt(void __iomem *base_addr, u32 order2p_Out, u32 order422_out, u32 cint_out)
{
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_ORDER2P_OUT], order2p_Out);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_ORDER422_OUT], order422_out);
	fimc_is_hw_set_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_C_INT_OUT], cint_out);
}

void fimc_is_scp_clear_intr_src(void __iomem *base_addr, u32 status)
{
	if (status & (1 << INTR_SCALER_STALL_BLOCKING))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 31);

	if (status & (1 << INTR_SCALER_OUTSTALL_BLOCKING))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 30);

	if (status & (1 << INTR_SCALER_CORE_START))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 24);

	if (status & (1 << INTR_SCALER_CORE_END))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 20);

	if (status & (1 << INTR_SCALER_SHADOW_TRIGGER))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 16);

	if (status & (1 << INTR_SCALER_SHADOW_TRIGGER_BYHW))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 12);

	if (status & (1 << INTR_SCALER_SHADOW_CONDITION))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 8);

	if (status & (1 << INTR_SCALER_FRAME_START))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 4);

	if (status & (1 << INTR_SCALER_FRAME_END))
		fimc_is_hw_set_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS], (u32)1 << 0);
}

u32 fimc_is_scp_get_otf_out_enable(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_DIRECT_CON], &scp_fields[SCP_F_DIRECTOUT_ENABLE]);
}

u32 fimc_is_scp_get_dma_enable(void __iomem *base_addr)
{
	return fimc_is_hw_get_field(base_addr, &scp_regs[ISP_SC_DMA_CON], &scp_fields[SCP_F_DMA_EN]);
}

u32 fimc_is_scp_get_intr_mask(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &scp_regs[ISP_SC_INTR_MASK]);
}

u32 fimc_is_scp_get_intr_status(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &scp_regs[ISP_SC_INTR_STATUS]);
}

u32 fimc_is_scp_get_version(void __iomem *base_addr)
{
	return fimc_is_hw_get_reg(base_addr, &scp_regs[ISP_SC_VERSION]);
}
