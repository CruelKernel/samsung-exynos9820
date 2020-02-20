/* linux/drivers/video/exynos/fbdev/dpu_9810/dpp_regs.c
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS9 SoC series Display Pre Processor driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#if defined(CONFIG_EXYNOS_HDR_TUNABLE_TONEMAPPING)
#include <video/exynos_hdr_tunables.h>
#endif

#include "../dpp.h"
#include "../dpp_coef.h"
#include "../hdr_lut.h"

#define DPP_SC_RATIO_MAX	((1 << 20) * 8 / 8)
#define DPP_SC_RATIO_7_8	((1 << 20) * 8 / 7)
#define DPP_SC_RATIO_6_8	((1 << 20) * 8 / 6)
#define DPP_SC_RATIO_5_8	((1 << 20) * 8 / 5)
#define DPP_SC_RATIO_4_8	((1 << 20) * 8 / 4)
#define DPP_SC_RATIO_3_8	((1 << 20) * 8 / 3)

/****************** IDMA CAL functions ******************/
static void idma_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, IDMA_IRQ, val, IDMA_ALL_IRQ_MASK);
}

static void idma_reg_set_irq_enable(u32 id)
{
	dma_write_mask(id, IDMA_IRQ, ~0, IDMA_IRQ_ENABLE);
}

static void idma_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, IDMA_ENABLE, val, IDMA_ALL_CLOCK_GATE_EN_MASK);
}

static void idma_reg_set_in_qos_lut(u32 id, u32 lut_id, u32 qos_t)
{
	u32 reg_id;

	if (lut_id == 0)
		reg_id = DPU_DMA_QOS_LUT07_00;
	else
		reg_id = DPU_DMA_QOS_LUT15_08;
	dma_com_write(id, reg_id, qos_t);
}

static void idma_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, IDMA_DYNAMIC_GATING_EN, val, IDMA_DG_EN_ALL);
}

static void idma_reg_set_out_frame_alpha(u32 id, u32 alpha)
{
	dma_write_mask(id, IDMA_OUT_CON, IDMA_OUT_FRAME_ALPHA(alpha),
			IDMA_OUT_FRAME_ALPHA_MASK);
}

static void idma_reg_clear_irq(u32 id, u32 irq)
{
	dma_write_mask(id, IDMA_IRQ, ~0, irq);
}

static void idma_reg_set_sw_reset(u32 id)
{
	dma_write_mask(id, IDMA_ENABLE, ~0, IDMA_SRESET);
}

static int idma_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dma_read(id, IDMA_ENABLE);
		if (!(cfg & (IDMA_SRESET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[idma] timeout sw-reset\n");

	return -1;
}

static void idma_reg_set_coordinates(u32 id, struct decon_frame *src)
{
	dma_write(id, IDMA_SRC_OFFSET,
			IDMA_SRC_OFFSET_Y(src->y) | IDMA_SRC_OFFSET_X(src->x));
	dma_write(id, IDMA_SRC_SIZE,
			IDMA_SRC_HEIGHT(src->f_h) | IDMA_SRC_WIDTH(src->f_w));
	dma_write(id, IDMA_IMG_SIZE,
			IDMA_IMG_HEIGHT(src->h) | IDMA_IMG_WIDTH(src->w));
}

static void idma_reg_set_rotation(u32 id, u32 rot)
{
	dma_write_mask(id, IDMA_IN_CON, IDMA_ROTATION(rot), IDMA_ROTATION_MASK);
}

static void idma_reg_set_block_mode(u32 id, bool en, int x, int y, u32 w, u32 h)
{
	if (!en) {
		dma_write_mask(id, IDMA_IN_CON, 0, IDMA_BLOCK_EN);
		return;
	}

	dma_write(id, IDMA_BLOCK_OFFSET,
			IDMA_BLK_OFFSET_Y(y) | IDMA_BLK_OFFSET_X(x));
	dma_write(id, IDMA_BLOCK_SIZE, IDMA_BLK_HEIGHT(h) | IDMA_BLK_WIDTH(w));
	dma_write_mask(id, IDMA_IN_CON, ~0, IDMA_BLOCK_EN);

	dpp_dbg("dpp%d: block x(%d) y(%d) w(%d) h(%d)\n", id, x, y, w, h);
}

static void idma_reg_set_format(u32 id, u32 fmt)
{
	dma_write_mask(id, IDMA_IN_CON, IDMA_IMG_FORMAT(fmt),
			IDMA_IMG_FORMAT_MASK);
}

#if defined(DMA_BIST)
static void idma_reg_set_test_pattern(u32 id, u32 pat_id, u32 *pat_dat)
{
	dma_write_mask(id, IDMA_IN_REQ_DEST, ~0, IDMA_IN_REG_DEST_SEL_MASK);

	if (pat_id == 0) {
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_0, pat_dat[0]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_1, pat_dat[1]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_2, pat_dat[2]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN0_3, pat_dat[3]);
	} else {
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_0, pat_dat[4]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_1, pat_dat[5]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_2, pat_dat[6]);
		dma_com_write(id, DPU_DMA_TEST_PATTERN1_3, pat_dat[7]);
	}
}
#endif

static void idma_reg_set_afbc(u32 id, u32 en, u32 rcv_num)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, IDMA_IN_CON, val, IDMA_AFBC_EN);
	dma_write_mask(id, IDMA_RECOVERY_CTRL, val, IDMA_RECOVERY_EN);
	dma_com_write_mask(id, DPU_DMA_RECOVERY_NUM_CTRL,
			DPU_DMA_RECOVERY_NUM(rcv_num),
			DPU_DMA_RECOVERY_NUM_MASK);
}

/****************** ODMA CAL functions ******************/
static void odma_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, ODMA_IRQ, val, ODMA_ALL_IRQ_MASK);
}

static void odma_reg_set_irq_enable(u32 id)
{
	dma_write_mask(id, ODMA_IRQ, ~0, ODMA_IRQ_ENABLE);
}
#if 0
static void odma_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, ODMA_ENABLE, val, ODMA_ALL_CLOCK_GATE_EN_MASK);
}
#endif
static void odma_reg_set_in_qos_lut(u32 id, u32 lut_id, u32 qos_t)
{
	u32 reg_id;

	if (lut_id == 0)
		reg_id = ODMA_OUT_QOS_LUT07_00;
	else
		reg_id = ODMA_OUT_QOS_LUT15_08;
	dma_write(id, reg_id, qos_t);
}

static void odma_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, ODMA_DYNAMIC_GATING_EN, val, ODMA_DG_EN_ALL);
}

static void odma_reg_set_out_frame_alpha(u32 id, u32 alpha)
{
	dma_write_mask(id, ODMA_OUT_CON1, ODMA_OUT_FRAME_ALPHA(alpha),
			ODMA_OUT_FRAME_ALPHA_MASK);
}

static void odma_reg_clear_irq(u32 id, u32 irq)
{
	dma_write_mask(id, ODMA_IRQ, ~0, irq);
}

static void odma_reg_set_sw_reset(u32 id)
{
	dma_write_mask(id, ODMA_ENABLE, ~0, ODMA_SRSET);
}

static int odma_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dma_read(id, ODMA_ENABLE);
		if (!(cfg & (ODMA_SRSET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[odma] timeout sw-reset\n");

	return -1;
}

static void odma_reg_set_coordinates(u32 id, struct decon_frame *dst)
{
	dma_write(id, ODMA_DST_OFFSET,
			ODMA_DST_OFFSET_Y(dst->y) | ODMA_DST_OFFSET_X(dst->x));
	dma_write(id, ODMA_DST_SIZE,
			ODMA_DST_HEIGHT(dst->f_h) | ODMA_DST_WIDTH(dst->f_w));
	dma_write(id, ODMA_OUT_IMG_SIZE,
			ODMA_OUT_IMG_HEIGHT(dst->h) | ODMA_OUT_IMG_WIDTH(dst->w));
}

static void odma_reg_set_format(u32 id, u32 fmt)
{
	dma_write_mask(id, ODMA_OUT_CON0, ODMA_IMG_FORMAT(fmt),
			ODMA_IMG_FORMAT_MASK);
}

/****************** DPP CAL functions ******************/
static void dpp_reg_set_irq_mask_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dpp_write_mask(id, DPP_IRQ, val, DPP_ALL_IRQ_MASK);
}

static void dpp_reg_set_irq_enable(u32 id)
{
	dpp_write_mask(id, DPP_IRQ, ~0, DPP_IRQ_ENABLE);
}

static void dpp_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dpp_write_mask(id, DPP_ENABLE, val, DPP_ALL_CLOCK_GATE_EN_MASK);
}

static void dpp_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dpp_write_mask(id, DPP_DYNAMIC_GATING_EN, val, DPP_DG_EN_ALL);
}

static void dpp_reg_set_linecnt(u32 id, u32 en)
{
	if (en)
		dpp_write_mask(id, DPP_LINECNT_CON,
				DPP_LC_MODE(0) | DPP_LC_ENABLE(1),
				DPP_LC_MODE_MASK | DPP_LC_ENABLE_MASK);
	else
		dpp_write_mask(id, DPP_LINECNT_CON, DPP_LC_ENABLE(0),
				DPP_LC_ENABLE_MASK);
}

static void dpp_reg_clear_irq(u32 id, u32 irq)
{
	dpp_write_mask(id, DPP_IRQ, ~0, irq);
}

static void dpp_reg_set_sw_reset(u32 id)
{
	dpp_write_mask(id, DPP_ENABLE, ~0, DPP_SRSET);
}

static int dpp_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dpp_read(id, DPP_ENABLE);
		if (!(cfg & (DPP_SRSET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[dpp] timeout sw reset\n");

	return -EBUSY;
}

static void dpp_reg_set_csc_coef(u32 id, u32 csc_std, u32 csc_rng)
{
#if defined(CONFIG_EXYNOS_MCD_HDR)
	u32 val, mask;
	u32 csc_id = CSC_BT_601_625;
	u32 c00, c01, c02;
	u32 c10, c11, c12;
	u32 c20, c21, c22;

	if ((csc_std > CSC_DCI_P3) && (csc_std <= CSC_ADOBE_RGB))
		csc_id = (csc_std - CSC_BT_601_625) * 2 + csc_rng;
	else
		dpp_err("Undefined CSC Type!!!\n");

	if (csc_rng > CSC_RANGE_FULL) {
		dpp_err("DPP:ERR:%s:Undefined csc range : %d\n", __func__, csc_rng);
		csc_rng = CSC_RANGE_FULL;
	}

	if (csc_std == CSC_BT_2020_CONSTANT_LUMINANCE)
		dpp_info("DPP:INFO:%s:csc std : BT_2020_CONSTANT_LUMINANCE\n", __func__);

	c00 = csc_3x3_t[csc_id][0][0];
	c01 = csc_3x3_t[csc_id][0][1];
	c02 = csc_3x3_t[csc_id][0][2];

	c10 = csc_3x3_t[csc_id][1][0];
	c11 = csc_3x3_t[csc_id][1][1];
	c12 = csc_3x3_t[csc_id][1][2];

	c20 = csc_3x3_t[csc_id][2][0];
	c21 = csc_3x3_t[csc_id][2][1];
	c22 = csc_3x3_t[csc_id][2][2];

	mask = (DPP_CSC_COEF_H_MASK | DPP_CSC_COEF_L_MASK);
	val = (DPP_CSC_COEF_H(c01) | DPP_CSC_COEF_L(c00));
	dpp_write_mask(id, DPP_CSC_COEF0, val, mask);

	val = (DPP_CSC_COEF_H(c10) | DPP_CSC_COEF_L(c02));
	dpp_write_mask(id, DPP_CSC_COEF1, val, mask);

	val = (DPP_CSC_COEF_H(c12) | DPP_CSC_COEF_L(c11));
	dpp_write_mask(id, DPP_CSC_COEF2, val, mask);

	val = (DPP_CSC_COEF_H(c21) | DPP_CSC_COEF_L(c20));
	dpp_write_mask(id, DPP_CSC_COEF3, val, mask);

	mask = DPP_CSC_COEF_L_MASK;
	val = DPP_CSC_COEF_L(c22);
	dpp_write_mask(id, DPP_CSC_COEF4, val, mask);

	dpp_dbg("---[CSC Type = %s_%s]---\n",
		csc_std == 3 ? "DCI_P3" : "BT_2020",
		csc_rng == 0 ? "LTD" : "FULL");
	dpp_dbg("0x%3x  0x%3x  0x%3x\n", c00, c01, c02);
	dpp_dbg("0x%3x  0x%3x  0x%3x\n", c10, c11, c12);
	dpp_dbg("0x%3x  0x%3x  0x%3x\n", c20, c21, c22);
#endif
}

static void dpp_reg_set_csc_params(u32 id, u32 csc_eq)
{

	u32 type = (csc_eq >> CSC_STANDARD_SHIFT) & 0x3F;
	u32 range = (csc_eq >> CSC_RANGE_SHIFT) & 0x7;
	u32 mode = (type <= CSC_DCI_P3) ? CSC_COEF_HARDWIRED : CSC_COEF_CUSTOMIZED;
	u32 val, mask;

	if (type == CSC_STANDARD_UNSPECIFIED) {
		dpp_dbg("unspecified CSC type(%d)! -> BT_601\n", type);
		type = CSC_BT_601;
		mode = CSC_COEF_HARDWIRED;
	}

	if (range == CSC_RANGE_UNSPECIFIED) {
		dpp_dbg("unspecified CSC range! -> LIMIT\n");
		range = CSC_RANGE_LIMITED;
	}

	val = (DPP_CSC_TYPE(type) | DPP_CSC_RANGE(range) | DPP_CSC_MODE(mode));
	mask = (DPP_CSC_TYPE_MASK | DPP_CSC_RANGE_MASK | DPP_CSC_MODE_MASK);
	dpp_write_mask(id, DPP_IN_CON, val, mask);

	if (mode == CSC_COEF_CUSTOMIZED)
		dpp_reg_set_csc_coef(id, type, range);

}

static void dpp_reg_set_h_coef(u32 id, u32 h_ratio)
{
	int i, j, k, sc_ratio;

	if (h_ratio <= DPP_SC_RATIO_MAX)
		sc_ratio = 0;
	else if (h_ratio <= DPP_SC_RATIO_7_8)
		sc_ratio = 1;
	else if (h_ratio <= DPP_SC_RATIO_6_8)
		sc_ratio = 2;
	else if (h_ratio <= DPP_SC_RATIO_5_8)
		sc_ratio = 3;
	else if (h_ratio <= DPP_SC_RATIO_4_8)
		sc_ratio = 4;
	else if (h_ratio <= DPP_SC_RATIO_3_8)
		sc_ratio = 5;
	else
		sc_ratio = 6;

	for (i = 0; i < 9; i++)
		for (j = 0; j < 8; j++)
			for (k = 0; k < 2; k++)
				dpp_write(id, DPP_H_COEF(i, j, k),
						h_coef_8t[sc_ratio][i][j]);
}

static void dpp_reg_set_v_coef(u32 id, u32 v_ratio)
{
	int i, j, k, sc_ratio;

	if (v_ratio <= DPP_SC_RATIO_MAX)
		sc_ratio = 0;
	else if (v_ratio <= DPP_SC_RATIO_7_8)
		sc_ratio = 1;
	else if (v_ratio <= DPP_SC_RATIO_6_8)
		sc_ratio = 2;
	else if (v_ratio <= DPP_SC_RATIO_5_8)
		sc_ratio = 3;
	else if (v_ratio <= DPP_SC_RATIO_4_8)
		sc_ratio = 4;
	else if (v_ratio <= DPP_SC_RATIO_3_8)
		sc_ratio = 5;
	else
		sc_ratio = 6;

	for (i = 0; i < 9; i++)
		for (j = 0; j < 4; j++)
			for (k = 0; k < 2; k++)
				dpp_write(id, DPP_V_COEF(i, j, k),
						v_coef_4t[sc_ratio][i][j]);
}

static void dpp_reg_set_scale_ratio(u32 id, struct dpp_params_info *p)
{
	dpp_write_mask(id, DPP_MAIN_H_RATIO, DPP_H_RATIO(p->h_ratio),
			DPP_H_RATIO_MASK);
	dpp_write_mask(id, DPP_MAIN_V_RATIO, DPP_V_RATIO(p->v_ratio),
			DPP_V_RATIO_MASK);

	dpp_reg_set_h_coef(id, p->h_ratio);
	dpp_reg_set_v_coef(id, p->v_ratio);

	dpp_dbg("h_ratio : %#x, v_ratio : %#x\n", p->h_ratio, p->v_ratio);
}


#ifdef CONFIG_EXYNOS_MCD_HDR
void dpp_reg_sel_hdr(u32 id, enum hdr_path path)
{
#if 0
	u32 path = HDR_PATH_LSI;

	if (p->wcg_mode)
		path = HDR_PATH_MCD;
#endif
	dpp_write_mask(id, DPP_ENABLE, DPP_SET_HDR_SEL(path), DPP_HDR_SEL_MASK);
}

u32 dpp_read_hdr_path(u32 id)
{
	return(dpp_read(id, DPP_ENABLE) & DPP_HDR_SEL_MASK);
}
#endif
static void dpp_reg_set_img_size(u32 id, u32 w, u32 h)
{
	dpp_write(id, DPP_IMG_SIZE, DPP_IMG_HEIGHT(h) | DPP_IMG_WIDTH(w));
}

static void dpp_reg_set_scaled_img_size(u32 id, u32 w, u32 h)
{
	dpp_write(id, DPP_SCALED_IMG_SIZE,
			DPP_SCALED_IMG_HEIGHT(h) | DPP_SCALED_IMG_WIDTH(w));
}

static void dpp_reg_set_alpha_type(u32 id, u32 type)
{
	/* [type] 0=per-frame, 1=per-pixel */
	dpp_write_mask(id, DPP_IN_CON, DPP_ALPHA_SEL(type), DPP_ALPHA_SEL_MASK);
}

static void dpp_reg_set_format(u32 id, u32 fmt)
{
	dpp_write_mask(id, DPP_IN_CON, DPP_IMG_FORMAT(fmt), DPP_IMG_FORMAT_MASK);
}

#ifndef CONFIG_EXYNOS_MCD_HDR 

static void dpp_reg_set_eotf_lut(u32 id, struct dpp_params_info *p)
{
	u32 i = 0;
	u32 *lut_x = NULL;
	u32 *lut_y = NULL;

	if (p->hdr == DPP_HDR_ST2084) {
		if (p->max_luminance > 1000) {
			lut_x = eotf_x_axis_st2084_4000;
			lut_y = eotf_y_axis_st2084_4000;
		} else {
			lut_x = eotf_x_axis_st2084_1000;
			lut_y = eotf_y_axis_st2084_1000;
		}
	} else if (p->hdr == DPP_HDR_HLG) {
		lut_x = eotf_x_axis_hlg;
		lut_y = eotf_y_axis_hlg;
	} else {
		dpp_err("Undefined HDR standard Type!!!\n");
		return;
	}

	for (i = 0; i < MAX_EOTF; i++) {
		dpp_write_mask(id,
			DPP_HDR_EOTF_X_AXIS_ADDR(i),
			DPP_HDR_EOTF_X_AXIS_VAL(i, lut_x[i]),
			DPP_HDR_EOTF_MASK(i));
		dpp_write_mask(id,
			DPP_HDR_EOTF_Y_AXIS_ADDR(i),
			DPP_HDR_EOTF_Y_AXIS_VAL(i, lut_y[i]),
			DPP_HDR_EOTF_MASK(i));
	}
}

static void dpp_reg_set_gm_lut(u32 id, struct dpp_params_info *p)
{
	u32 i = 0;
	u32 *lut_gm = NULL;

	if (p->eq_mode == CSC_BT_2020) {
		lut_gm = gm_coef_2020_p3;
	} else if (p->eq_mode == CSC_DCI_P3) {
		return;
	} else {
		dpp_err("Undefined HDR CSC Type!!!\n");
		return;
	}

	for (i = 0; i < MAX_GM; i++) {
		dpp_write_mask(id,
			DPP_HDR_GM_COEF_ADDR(i),
			lut_gm[i],
			DPP_HDR_GM_COEF_MASK);
	}
}

static void dpp_reg_set_tm_lut(u32 id, struct dpp_params_info *p)
{
	u32 i = 0;
	u32 *lut_x = NULL;
	u32 *lut_y = NULL;

#if defined(CONFIG_EXYNOS_HDR_TUNABLE_TONEMAPPING)
	if (!exynos_hdr_get_tm_lut_xy(tm_x_tune, tm_y_tune)) {
		if ((p->max_luminance > 1000) && (p->max_luminance < 10000)) {
			lut_x = tm_x_axis_gamma_2P2_4000;
			lut_y = tm_y_axis_gamma_2P2_4000;
		} else {
			lut_x = tm_x_axis_gamma_2P2_1000;
			lut_y = tm_y_axis_gamma_2P2_1000;
		}
	} else {
		lut_x = tm_x_tune;
		lut_y = tm_y_tune;
	}
#else
	if ((p->max_luminance > 1000) && (p->max_luminance < 10000)) {
		lut_x = tm_x_axis_gamma_2P2_4000;
		lut_y = tm_y_axis_gamma_2P2_4000;
	} else {
		lut_x = tm_x_axis_gamma_2P2_1000;
		lut_y = tm_y_axis_gamma_2P2_1000;
	}
#endif

	for (i = 0; i < MAX_TM; i++) {
		dpp_write_mask(id,
			DPP_HDR_TM_X_AXIS_ADDR(i),
			DPP_HDR_TM_X_AXIS_VAL(i, lut_x[i]),
			DPP_HDR_TM_MASK(i));
		dpp_write_mask(id,
			DPP_HDR_TM_Y_AXIS_ADDR(i),
			DPP_HDR_TM_Y_AXIS_VAL(i, lut_y[i]),
			DPP_HDR_TM_MASK(i));
	}
}

static void dpp_reg_set_hdr_params(u32 id, struct dpp_params_info *p)
{
	u32 val, val2, mask;

	val = (p->hdr == DPP_HDR_ST2084 || p->hdr == DPP_HDR_HLG) ? ~0 : 0;
	mask = DPP_HDR_ON_MASK | DPP_EOTF_ON_MASK | DPP_TM_ON_MASK;
	dpp_write_mask(id, DPP_VGRF_HDR_CON, val, mask);

	val2 = (p->eq_mode != CSC_DCI_P3) ? ~0 : 0;
	dpp_write_mask(id, DPP_VGRF_HDR_CON, val2,  DPP_GM_ON_MASK);

	if (val) {
		dpp_reg_set_eotf_lut(id, p);
		dpp_reg_set_gm_lut(id, p);
		dpp_reg_set_tm_lut(id, p);
	}
}
#endif

/****************** WB MUX CAL functions ******************/
static void wb_mux_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dpp_write_mask(id, DPU_WB_ENABLE, val, WB_ALL_CLOCK_GATE_EN_MASK);
}

static void wb_mux_reg_set_dynamic_gating_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dpp_write_mask(id, DPU_WB_DYNAMIC_GATING_EN, val, WB_DG_EN_ALL);
}

static void wb_mux_reg_set_out_frame_alpha(u32 id, u32 alpha)
{
	dpp_write_mask(id, DPU_WB_OUT_CON1, WB_OUT_FRAME_ALPHA(alpha),
			WB_OUT_FRAME_ALPHA_MASK);
}

static void wb_mux_reg_set_sw_reset(u32 id)
{
	dpp_write_mask(id, DPU_WB_ENABLE, ~0, WB_SRSET);
}

static int wb_mux_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dpp_read(id, DPU_WB_ENABLE);
		if (!(cfg & (WB_SRSET)))
			return 0;
		udelay(10);
	} while (--cnt);

	dpp_err("[wb] timeout sw-reset\n");

	return -1;
}

static void wb_mux_reg_set_csc_params(u32 id, u32 csc_eq)
{
	u32 type = (csc_eq >> CSC_STANDARD_SHIFT) & 0x3F;
	u32 range = (csc_eq >> CSC_RANGE_SHIFT) & 0x7;

	u32 rgb_type = (type << 1) | (range << 0);
	/* only support {601, 709, N, W} */
	if (rgb_type > 3) {
		dpp_warn("[WB] Unsupported RGB type(%d) !\n", rgb_type);
		dpp_warn("[WB] -> forcing BT_601_LIMITTED\n");
		rgb_type = ((CSC_BT_601 << 1) | CSC_RANGE_LIMITED);
	}

	dpp_write_mask(id, DPU_WB_OUT_CON0, WB_RGB_TYPE(rgb_type),
			WB_RGB_TYPE_MASK);
}

static void wb_mux_reg_set_dst_size(u32 id, u32 w, u32 h)
{
	dpp_write(id, DPU_WB_DST_SIZE, WB_DST_HEIGHT(h) | WB_DST_WIDTH(w));
}

static void wb_mux_reg_set_csc_r2y(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dpp_write_mask(id, DPU_WB_OUT_CON0, val, WB_CSC_R2Y_MASK);
}

static void wb_mux_reg_set_uv_offset(u32 id, u32 off_x, u32 off_y)
{
	dpp_write_mask(id, DPU_WB_OUT_CON1,
			WB_UV_OFFSET_Y(off_y) | WB_UV_OFFSET_X(off_x),
			WB_UV_OFFSET_Y_MASK | WB_UV_OFFSET_X_MASK);
}

/********** IDMA and ODMA combination CAL functions **********/
static void dma_reg_set_base_addr(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		dma_write(id, IDMA_IN_BASE_ADDR_Y, p->addr[0]);
		if (p->is_comp)
			dma_write(id, IDMA_IN_BASE_ADDR_C, p->addr[0]);
		else
			dma_write(id, IDMA_IN_BASE_ADDR_C, p->addr[1]);
		if (p->is_4p) {
			dma_write(id, IDMA_IN_BASE_ADDR_Y2, p->addr[2]);
			dma_write(id, IDMA_IN_BASE_ADDR_C2, p->addr[3]);
			dma_write_mask(id, IDMA_2BIT_STRIDE,
					IDMA_LUMA_2B_STRIDE(p->y_2b_strd),
					IDMA_LUMA_2B_STRIDE_MASK);
			dma_write_mask(id, IDMA_2BIT_STRIDE,
					IDMA_CHROMA_2B_STRIDE(p->c_2b_strd),
					IDMA_CHROMA_2B_STRIDE_MASK);
		}
	} else if (test_bit(DPP_ATTR_ODMA, &attr)) {
		dma_write(id, ODMA_IN_BASE_ADDR_Y, p->addr[0]);
		dma_write(id, ODMA_IN_BASE_ADDR_C, p->addr[1]);
	}
	dpp_dbg("dpp%d: base addr 1p(0x%p) 2p(0x%p) 3p(0x%p) 4p(0x%p)\n", id,
			(void *)p->addr[0], (void *)p->addr[1],
			(void *)p->addr[2], (void *)p->addr[3]);
}

/********** IDMA, ODMA, DPP and WB MUX combination CAL functions **********/
static void dma_dpp_reg_set_coordinates(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_set_coordinates(id, &p->src);

		if (test_bit(DPP_ATTR_DPP, &attr)) {
			if (p->rot > DPP_ROT_180)
				dpp_reg_set_img_size(id, p->src.h, p->src.w);
			else
				dpp_reg_set_img_size(id, p->src.w, p->src.h);
		}

		if (test_bit(DPP_ATTR_SCALE, &attr))
			dpp_reg_set_scaled_img_size(id, p->dst.w, p->dst.h);
	} else if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_set_coordinates(id, &p->src);
		wb_mux_reg_set_dst_size(id, p->src.w, p->src.h);
	}
}

static int dma_dpp_reg_set_format(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{
	u32 fmt;
	u32 alpha_type = 0; /* 0: per-frame, 1: per-pixel */
	u32 fmt_type = 0;
	u32 is_yuv = 0;

	switch (p->format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
		fmt = IDMA_IMG_FORMAT_ARGB8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_ABGR_8888:
		fmt = IDMA_IMG_FORMAT_ABGR8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_RGBA_8888:
		fmt = IDMA_IMG_FORMAT_RGBA8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_BGRA_8888:
		fmt = IDMA_IMG_FORMAT_BGRA8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_XRGB_8888:
		fmt = IDMA_IMG_FORMAT_XRGB8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		break;
	case DECON_PIXEL_FORMAT_XBGR_8888:
		fmt = IDMA_IMG_FORMAT_XBGR8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		break;
	case DECON_PIXEL_FORMAT_RGBX_8888:
		fmt = IDMA_IMG_FORMAT_RGBX8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		break;
	case DECON_PIXEL_FORMAT_BGRX_8888:
		fmt = IDMA_IMG_FORMAT_BGRX8888;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		break;
	case DECON_PIXEL_FORMAT_RGB_565:
		if (p->is_comp)
			fmt = IDMA_IMG_FORMAT_BGR565;
		else
			fmt = IDMA_IMG_FORMAT_RGB565;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		break;
	case DECON_PIXEL_FORMAT_BGR_565:
		if (p->is_comp)
			fmt = IDMA_IMG_FORMAT_RGB565;
		else
			fmt = IDMA_IMG_FORMAT_BGR565;
		fmt_type = DPP_IMG_FORMAT_ARGB8888;
		break;
	/* TODO: add ARGB1555 & ARGB4444 */
	case DECON_PIXEL_FORMAT_ARGB_2101010:
		fmt = IDMA_IMG_FORMAT_ARGB2101010;
		fmt_type = DPP_IMG_FORMAT_ARGB8101010;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_ABGR_2101010:
		fmt = IDMA_IMG_FORMAT_ABGR2101010;
		fmt_type = DPP_IMG_FORMAT_ARGB8101010;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_RGBA_1010102:
		fmt = IDMA_IMG_FORMAT_RGBA2101010;
		fmt_type = DPP_IMG_FORMAT_ARGB8101010;
		alpha_type = 1;
		break;
	case DECON_PIXEL_FORMAT_BGRA_1010102:
		fmt = IDMA_IMG_FORMAT_BGRA2101010;
		fmt_type = DPP_IMG_FORMAT_ARGB8101010;
		alpha_type = 1;
		break;

	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV12M:
		fmt = IDMA_IMG_FORMAT_YUV420_2P;
		fmt_type = DPP_IMG_FORMAT_YUV420_8P;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV21M:
	case DECON_PIXEL_FORMAT_NV12N:
		fmt = IDMA_IMG_FORMAT_YVU420_2P;
		fmt_type = DPP_IMG_FORMAT_YUV420_8P;
		is_yuv = 1;
		break;

	case DECON_PIXEL_FORMAT_NV12N_10B:
		fmt = IDMA_IMG_FORMAT_YVU420_8P2;
		fmt_type = DPP_IMG_FORMAT_YUV420_8P2;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV12M_P010:
	case DECON_PIXEL_FORMAT_NV12_P010:
		fmt = IDMA_IMG_FORMAT_YUV420_P010;
		fmt_type = DPP_IMG_FORMAT_YUV420_P010;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV21M_P010:
		fmt = IDMA_IMG_FORMAT_YVU420_P010;
		fmt_type = DPP_IMG_FORMAT_YUV420_P010;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV12M_S10B:
		fmt = IDMA_IMG_FORMAT_YVU420_8P2;
		fmt_type = DPP_IMG_FORMAT_YUV420_8P2;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV21M_S10B:
		fmt = IDMA_IMG_FORMAT_YUV420_8P2;
		fmt_type = DPP_IMG_FORMAT_YUV420_8P2;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV16:
		fmt = IDMA_IMG_FORMAT_YVU422_2P;
		fmt_type = DPP_IMG_FORMAT_YUV422_8P;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV61:
		fmt = IDMA_IMG_FORMAT_YUV422_2P;
		fmt_type = DPP_IMG_FORMAT_YUV422_8P;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV16M_P210:
		fmt = IDMA_IMG_FORMAT_YUV422_P210;
		fmt_type = DPP_IMG_FORMAT_YUV422_P210;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV61M_P210:
		fmt = IDMA_IMG_FORMAT_YVU422_P210;
		fmt_type = DPP_IMG_FORMAT_YUV422_P210;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV16M_S10B:
		fmt = IDMA_IMG_FORMAT_YUV422_8P2;
		fmt_type = DPP_IMG_FORMAT_YUV422_8P2;
		is_yuv = 1;
		break;
	case DECON_PIXEL_FORMAT_NV61M_S10B:
		fmt = IDMA_IMG_FORMAT_YVU422_8P2;
		fmt_type = DPP_IMG_FORMAT_YUV422_8P2;
		is_yuv = 1;
		break;
	default:
		dpp_err("Unsupported Format\n");
		return -EINVAL;
	}

	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_set_format(id, fmt);
		if (test_bit(DPP_ATTR_DPP, &attr)) {
			dpp_reg_set_alpha_type(id, alpha_type);
			dpp_reg_set_format(id, fmt_type);
		}
	} else if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_set_format(id, fmt);
		wb_mux_reg_set_csc_r2y(id, is_yuv);
		wb_mux_reg_set_uv_offset(id, 0, 0);
	}

	return 0;
}

/******************** EXPORTED DPP CAL APIs ********************/
void dpp_constraints_params(struct dpp_size_constraints *vc,
		struct dpp_img_format *vi, struct dpp_restriction *res)
{
	u32 sz_align = 1;

	if (vi->yuv)
		sz_align = 2;

	vc->src_mul_w = res->src_f_w.align * sz_align;
	vc->src_mul_h = res->src_f_h.align * sz_align;
	vc->src_w_min = res->src_f_w.min * sz_align;
	vc->src_w_max = res->src_f_w.max;
	vc->src_h_min = res->src_f_h.min;
	vc->src_h_max = res->src_f_h.max;
	vc->img_mul_w = res->src_w.align * sz_align;
	vc->img_mul_h = res->src_h.align * sz_align;
	vc->img_w_min = res->src_w.min * sz_align;
	vc->img_w_max = res->src_w.max;
	vc->img_h_min = res->src_h.min * sz_align;
	if (vi->rot > DPP_ROT_180)
		vc->img_h_max = res->src_h_rot_max;
	else
		vc->img_h_max = res->src_h.max;
	vc->src_mul_x = res->src_x_align * sz_align;
	vc->src_mul_y = res->src_y_align * sz_align;

	vc->sca_w_min = res->dst_w.min;
	vc->sca_w_max = res->dst_w.max;
	vc->sca_h_min = res->dst_h.min;
	vc->sca_h_max = res->dst_h.max;
	vc->sca_mul_w = res->dst_w.align;
	vc->sca_mul_h = res->dst_h.align;

	vc->blk_w_min = res->blk_w.min;
	vc->blk_w_max = res->blk_w.max;
	vc->blk_h_min = res->blk_h.min;
	vc->blk_h_max = res->blk_h.max;
	vc->blk_mul_w = res->blk_w.align;
	vc->blk_mul_h = res->blk_h.align;

	if (vi->wb) {
		vc->src_mul_w = res->dst_f_w.align * sz_align;
		vc->src_mul_h = res->dst_f_h.align * sz_align;
		vc->src_w_min = res->dst_f_w.min;
		vc->src_w_max = res->dst_f_w.max;
		vc->src_h_min = res->dst_f_h.min;
		vc->src_h_max = res->dst_f_h.max;
		vc->img_mul_w = res->dst_w.align * sz_align;
		vc->img_mul_h = res->dst_h.align * sz_align;
		vc->img_w_min = res->dst_w.min;
		vc->img_w_max = res->dst_w.max;
		vc->img_h_min = res->dst_h.min;
		vc->img_h_max = res->dst_h.max;
		vc->src_mul_x = res->dst_x_align * sz_align;
		vc->src_mul_y = res->dst_y_align * sz_align;
	}
}

void dpp_reg_init(u32 id, const unsigned long attr)
{
	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_set_irq_mask_all(id, 0);
		idma_reg_set_irq_enable(id);
		idma_reg_set_clock_gate_en_all(id, 0);
		idma_reg_set_in_qos_lut(id, 0, 0x44444444);
		idma_reg_set_in_qos_lut(id, 1, 0x44444444);
		idma_reg_set_dynamic_gating_en_all(id, 0);
		idma_reg_set_out_frame_alpha(id, 0xFF);
	}

	if (test_bit(DPP_ATTR_DPP, &attr)) {
		dpp_reg_set_irq_mask_all(id, 0);
		dpp_reg_set_irq_enable(id);
		dpp_reg_set_clock_gate_en_all(id, 0);
		dpp_reg_set_dynamic_gating_en_all(id, 0);
		dpp_reg_set_linecnt(id, 1);
	}

	if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_set_irq_mask_all(id, 0);
		odma_reg_set_irq_enable(id);
		//odma_reg_set_clock_gate_en_all(id, 0);
		odma_reg_set_in_qos_lut(id, 0, 0x44444444);
		odma_reg_set_in_qos_lut(id, 1, 0x44444444);
		odma_reg_set_dynamic_gating_en_all(id, 0);
		odma_reg_set_out_frame_alpha(id, 0xFF);
		wb_mux_reg_set_clock_gate_en_all(id, 1);
		wb_mux_reg_set_dynamic_gating_en_all(id, 0); /* TODO: enable or disable ? */
		wb_mux_reg_set_out_frame_alpha(id, 0xFF);
	}

}

int dpp_reg_deinit(u32 id, bool reset, const unsigned long attr)
{
	if (test_bit(DPP_ATTR_IDMA, &attr)) {
		idma_reg_clear_irq(id, IDMA_ALL_IRQ_CLEAR);
		idma_reg_set_irq_mask_all(id, 1);
	}

	if (test_bit(DPP_ATTR_DPP, &attr)) {
		dpp_reg_clear_irq(id, DPP_ALL_IRQ_CLEAR);
		dpp_reg_set_irq_mask_all(id, 1);
	}

	if (test_bit(DPP_ATTR_ODMA, &attr)) {
		odma_reg_clear_irq(id, ODMA_ALL_IRQ_CLEAR);
		odma_reg_set_irq_mask_all(id, 1);
	}

	if (reset) {
		if (test_bit(DPP_ATTR_IDMA, &attr) &&
				!test_bit(DPP_ATTR_DPP, &attr)) { /* IDMA only */
			idma_reg_set_sw_reset(id);
			if (idma_reg_wait_sw_reset_status(id))
				return -1;
		} else if (test_bit(DPP_ATTR_IDMA, &attr) &&
				test_bit(DPP_ATTR_DPP, &attr)) { /* IDMA/DPP */
			idma_reg_set_sw_reset(id);
			dpp_reg_set_sw_reset(id);
			if (idma_reg_wait_sw_reset_status(id) ||
					dpp_reg_wait_sw_reset_status(id))
				return -1;
		} else if (test_bit(DPP_ATTR_ODMA, &attr)) { /* writeback */
			odma_reg_set_sw_reset(id);
			wb_mux_reg_set_sw_reset(id);
			if (odma_reg_wait_sw_reset_status(id) ||
					wb_mux_reg_wait_sw_reset_status(id))
				return -1;
		} else {
			dpp_err("%s: not support attribute case(0x%lx)\n",
					__func__, attr);
		}
	}

	return 0;
}

#if defined(DMA_BIST)
u32 pattern_data[] = {
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0x000000ff,
	0x000000ff,
	0x000000ff,
	0x000000ff,
};
#endif

void dpp_reg_configure_params(u32 id, struct dpp_params_info *p,
		const unsigned long attr)
{

#ifdef CONFIG_EXYNOS_MCD_HDR
		dpp_reg_sel_hdr(id, HDR_PATH_MCD);
#endif

	if (test_bit(DPP_ATTR_CSC, &attr) && test_bit(DPP_ATTR_DPP, &attr))
		dpp_reg_set_csc_params(id, p->eq_mode);
	else if (test_bit(DPP_ATTR_CSC, &attr) && test_bit(DPP_ATTR_ODMA, &attr))
		wb_mux_reg_set_csc_params(id, p->eq_mode);

	if (test_bit(DPP_ATTR_SCALE, &attr))
		dpp_reg_set_scale_ratio(id, p);

	/* configure coordinates and size of IDMA, DPP, ODMA and WB MUX */
	dma_dpp_reg_set_coordinates(id, p, attr);

	if (test_bit(DPP_ATTR_ROT, &attr) || test_bit(DPP_ATTR_FLIP, &attr))
		idma_reg_set_rotation(id, p->rot);

	/* configure base address of IDMA and ODMA */
	dma_reg_set_base_addr(id, p, attr);

	if (test_bit(DPP_ATTR_BLOCK, &attr))
		idma_reg_set_block_mode(id, p->is_block, p->block.x, p->block.y,
				p->block.w, p->block.h);

	/* configure image format of IDMA, DPP, ODMA and WB MUX */
	dma_dpp_reg_set_format(id, p, attr);
	
#ifndef CONFIG_EXYNOS_MCD_HDR
	if (test_bit(DPP_ATTR_HDR, &attr))
		dpp_reg_set_hdr_params(id, p);
#endif

	if (test_bit(DPP_ATTR_AFBC, &attr))
		idma_reg_set_afbc(id, p->is_comp, p->rcv_num);

#if defined(DMA_BIST)
	idma_reg_set_test_pattern(id, 0, pattern_data);
#endif
}

u32 dpp_reg_get_irq_and_clear(u32 id)
{
	u32 val, cfg_err;

	val = dpp_read(id, DPP_IRQ);
	dpp_reg_clear_irq(id, val);

	if (val & DPP_CONFIG_ERROR) {
		cfg_err = dpp_read(id, DPP_CFG_ERR_STATE);
		dpp_err("dpp%d config error occur(0x%x)\n", id, cfg_err);
	}

	return val;
}

u32 idma_reg_get_irq_and_clear(u32 id)
{
	u32 val, cfg_err;

	val = dma_read(id, IDMA_IRQ);
	idma_reg_clear_irq(id, val);

	if (val & IDMA_CONFIG_ERROR) {
		cfg_err = dma_read(id, IDMA_CFG_ERR_STATE);
		dpp_err("dpp%d idma config error occur(0x%x)\n", id, cfg_err);
	}

	return val;
}

u32 odma_reg_get_irq_and_clear(u32 id)
{
	u32 val, cfg_err;

	val = dma_read(id, ODMA_IRQ);
	odma_reg_clear_irq(id, val);

	if (val & ODMA_CONFIG_ERROR) {
		cfg_err = dma_read(id, ODMA_CFG_ERR_STATE);
		dpp_err("dpp%d odma config error occur(0x%x)\n", id, cfg_err);
	}

	return val;
}

static void dpp_reg_dump_ch_data(int id, enum dpp_reg_area reg_area,
		u32 sel[], u32 cnt)
{
	unsigned char linebuf[128] = {0, };
	int i, ret;
	int len = 0;
	u32 data;

	for (i = 0; i < cnt; i++) {
		if (!(i % 4) && i != 0) {
			linebuf[len] = '\0';
			len = 0;
			dpp_info("%s\n", linebuf);
		}

		if (reg_area == REG_AREA_DPP) {
			dpp_write(id, 0xC04, sel[i]);
			data = dpp_read(id, 0xC10);
		} else if (reg_area == REG_AREA_DMA) {
			dma_write(id, IDMA_DEBUG_CONTROL,
					IDMA_DEBUG_CONTROL_SEL(sel[i]) |
					IDMA_DEBUG_CONTROL_EN);
			data = dma_read(id, IDMA_DEBUG_DATA);
		} else { /* REG_AREA_DMA_COM */
			dma_com_write(0, DPU_DMA_DEBUG_CONTROL,
					DPU_DMA_DEBUG_CONTROL_SEL(sel[i]) |
					DPU_DMA_DEBUG_CONTROL_EN);
			data = dma_com_read(0, DPU_DMA_DEBUG_DATA);
		}

		ret = snprintf(linebuf + len, sizeof(linebuf) - len,
				"[0x%08x: %08x] ", sel[i], data);
		if (ret >= sizeof(linebuf) - len) {
			dpp_err("overflow: %d %ld %d\n",
					ret, sizeof(linebuf), len);
			return;
		}
		len += ret;
	}
	dpp_info("%s\n", linebuf);
}
static bool checked;

static void dma_reg_dump_com_debug_regs(int id)
{
	u32 sel_ch0[30] = {
		0x0000, 0x0001, 0x0005, 0x0009, 0x000D, 0x000E, 0x0020, 0x0021,
		0x0025, 0x0029, 0x002D, 0x002E, 0x0100, 0x0101, 0x0105, 0x0109,
		0x010D, 0x010E, 0x0120, 0x0121, 0x0125, 0x0129, 0x012D, 0x012E,
		0x0200, 0x0300, 0x0400, 0x0401, 0x0402, 0x0403
	};

	u32 sel_ch1[29] = {
		0x4000, 0x4001, 0x4005, 0x4009, 0x400D, 0x400E, 0x4020, 0x4021,
		0x4025, 0x4029, 0x402D, 0x402E, 0x4100, 0x4101, 0x4105, 0x4109,
		0x410D, 0x410E, 0x4120, 0x4121, 0x4125, 0x4129, 0x412D, 0x412E,
		0x4200, 0x4300, 0x4400, 0x4401, 0x4402
	};

	u32 sel_ch2[5] = {
		0x8000, 0x8001, 0x8002, 0xC000, 0xC001
	};

	dpp_info("%s: checked = %d\n", __func__, checked);
	if (checked)
		return;

	dpp_info("-< DMA COMMON DEBUG SFR(CH0) >-\n");
	dpp_reg_dump_ch_data(id, REG_AREA_DMA_COM, sel_ch0, 30);

	dpp_info("-< DMA COMMON DEBUG SFR(CH1) >-\n");
	dpp_reg_dump_ch_data(id, REG_AREA_DMA_COM, sel_ch1, 29);

	dpp_info("-< DMA COMMON DEBUG SFR(CH2) >-\n");
	dpp_reg_dump_ch_data(id, REG_AREA_DMA_COM, sel_ch2, 5);

	checked = true;
}

static void dma_reg_dump_debug_regs(int id, unsigned long attr)
{
	u32 sel_plane0[11] = {
		0x0000, 0x0001, 0x0002, 0x0003, 0x0007, 0x0008, 0x0100, 0x0101,
		0x0102, 0x0103, 0x0200
	};

	u32 sel_plane1[11] = {
		0x1000, 0x1001, 0x1002, 0x1003, 0x1007, 0x1008, 0x1100, 0x1101,
		0x1102, 0x1103, 0x1200
	};

	u32 sel_plane2[11] = {
		0x2000, 0x2001, 0x2002, 0x2003, 0x2007, 0x2008, 0x2100, 0x2101,
		0x2102, 0x2103, 0x2200
	};

	u32 sel_plane3[11] = {
		0x3000, 0x3001, 0x3002, 0x3003, 0x3007, 0x3008, 0x3100, 0x3101,
		0x3102, 0x3103, 0x3200
	};

	u32 sel_yuv[6] = {
		0x4001, 0x4002, 0x4003, 0x4005, 0x4006, 0x4007
	};

	u32 sel_fbc[15] = {
		0x5100, 0x5101, 0x5104, 0x5105, 0x5200, 0x5201, 0x5202, 0x5203,
		0x5204, 0x5300, 0x5301, 0x5302, 0x5303, 0x5304, 0x5305
	};

	u32 sel_rot[16] = {
		0x6100, 0x6101, 0x6102, 0x6103, 0x6200, 0x6201, 0x6202, 0x6203,
		0x6300, 0x6301, 0x6305, 0x6306, 0x6400, 0x6401, 0x6405, 0x6406
	};

	u32 sel_pix[4] = {
		0x7000, 0x7001, 0x7002, 0x7003
	};

	dpp_info("-< DPU_DMA%d DEBUG SFR >-\n", id);
	dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_plane0, 11);

	if (test_bit(DPP_ATTR_CSC, &attr)) {
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_plane1, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_plane2, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_plane3, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_yuv, 6);
	}

	if (test_bit(DPP_ATTR_AFBC, &attr))
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_fbc, 15);

	if (test_bit(DPP_ATTR_ROT, &attr))
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_rot, 16);

	dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_pix, 4);
}

static void dpp_reg_dump_debug_regs(int id)
{
	u32 sel_gf[3] = {0x0000, 0x0100, 0x0101};
	u32 sel_vg_vgf[19] = {0x0000, 0x0100, 0x0101, 0x0200, 0x0201, 0x0202,
		0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x0208, 0x0300, 0x0301,
		0x0302, 0x0303, 0x0304, 0x0400, 0x0401};
	u32 sel_vgs_vgrfs[37] = {0x0000, 0x0100, 0x0101, 0x0200, 0x0201, 0x0210,
		0x0211, 0x0220, 0x0221, 0x0230, 0x0231, 0x0240, 0x0241, 0x0250,
		0x0251, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306,
		0x0307, 0x0308, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0500,
		0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0600, 0x0601};
	u32 cnt;
	u32 *sel = NULL;

	if (id == 0 || id == 2) { /* GF0, GF1 */
		sel =  sel_gf;
		cnt = 3;
	} else if (id == 3 || id == 4) { /* VGF, VG */
		sel = sel_vg_vgf;
		cnt = 19;
	} else if (id == 1 || id == 5) { /* VGRFS, VGS */
		sel = sel_vgs_vgrfs;
		cnt = 37;
	} else {
		dpp_err("DPP%d is wrong ID\n", id);
		return;
	}

	dpp_write(id, 0x0C00, 0x1);
	dpp_info("-< DPP%d DEBUG SFR >-\n", id);
	dpp_reg_dump_ch_data(id, REG_AREA_DPP, sel, cnt);
}

static void dma_dump_regs(u32 id, void __iomem *dma_regs)
{
	dpp_info("\n=== DPU_DMA%d SFR DUMP ===\n", id);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dma_regs, 0x6C, false);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dma_regs + 0x100, 0x8, false);

	dpp_info("=== DPU_DMA%d SHADOW SFR DUMP ===\n", id);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dma_regs + 0x800, 0x74, false);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dma_regs + 0x900, 0x8, false);
}

static void dpp_dump_regs(u32 id, void __iomem *regs, unsigned long attr)
{
	dpp_info("=== DPP%d SFR DUMP ===\n", id);

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs, 0x4C, false);
	if (test_bit(DPP_ATTR_AFBC, &attr)) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
				regs + 0x5B0, 0x10, false);
	}
	if (test_bit(DPP_ATTR_ROT, &attr)) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs + 0x600, 0x1E0, false);
	}
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs + 0xA54, 0x4, false);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs + 0xB00, 0x4C, false);
	if (test_bit(DPP_ATTR_AFBC, &attr)) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
				regs + 0xBB0, 0x10, false);
	}
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs + 0xD00, 0xC, false);
}

void __dpp_dump(u32 id, void __iomem *regs, void __iomem *dma_regs,
		unsigned long attr)
{
	dma_reg_dump_com_debug_regs(id);

	dma_dump_regs(id, dma_regs);
	dma_reg_dump_debug_regs(id, attr);

	dpp_dump_regs(id, regs, attr);
	dpp_reg_dump_debug_regs(id);
}
