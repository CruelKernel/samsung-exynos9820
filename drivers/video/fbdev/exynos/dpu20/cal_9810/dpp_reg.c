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
		reg_id = DPU_DMA_IN_QOS_LUT07_00;
	else
		reg_id = DPU_DMA_IN_QOS_LUT15_08;
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
	dma_write_mask(id, IDMA_ENABLE, ~0, IDMA_SRSET);
}

static int idma_reg_wait_sw_reset_status(u32 id)
{
	u32 cfg = 0;
	unsigned long cnt = 100000;

	do {
		cfg = dma_read(id, IDMA_ENABLE);
		if (!(cfg & (IDMA_SRSET)))
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

static void odma_reg_set_clock_gate_en_all(u32 id, u32 en)
{
	u32 val = en ? ~0 : 0;

	dma_write_mask(id, ODMA_ENABLE, val, ODMA_ALL_CLOCK_GATE_EN_MASK);
}

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
#if defined(SUPPORT_USER_COEF)
	u32 val, mask;
	u32 csc_id = DPP_CSC_ID_BT_2020 + CSC_RANGE_LIMITED;
	u32 c00, c01, c02;
	u32 c10, c11, c12;
	u32 c20, c21, c22;

	if (csc_std == CSC_BT_2020)
		csc_id = DPP_CSC_ID_BT_2020 + csc_rng;
	else if (csc_std == CSC_DCI_P3)
		csc_id = DPP_CSC_ID_DCI_P3 + csc_rng;
	else
		dpp_err("Undefined CSC Type!!!\n");

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
	u32 mode = (csc_eq <= CSC_DCI_P3) ? CSC_COEF_HARDWIRED : CSC_COEF_CUSTOMIZED;
	u32 val, mask;

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
#if defined(CONFIG_EXYNOS_HDR_TUNABLE_TONEMAPPING)
	u32 i = 0;
	u32 *lut_x = NULL;
	u32 *lut_y = NULL;

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
#endif
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
		struct dpp_img_format *vi)
{
	u32 sz_align = 1;

	if (vi->yuv)
		sz_align = 2;

	vc->src_mul_w = SRC_SIZE_MULTIPLE * sz_align;
	vc->src_mul_h = SRC_SIZE_MULTIPLE * sz_align;
	vc->src_w_min = SRC_WIDTH_MIN * sz_align;
	vc->src_w_max = SRC_WIDTH_MAX;
	vc->src_h_min = SRC_HEIGHT_MIN;
	vc->src_h_max = SRC_HEIGHT_MAX;
	vc->img_mul_w = IMG_SIZE_MULTIPLE * sz_align;
	vc->img_mul_h = IMG_SIZE_MULTIPLE * sz_align;
	vc->img_w_min = IMG_WIDTH_MIN * sz_align;
	vc->img_w_max = IMG_WIDTH_MAX;
	vc->img_h_min = IMG_HEIGHT_MIN * sz_align;
	if (vi->rot > DPP_ROT_180)
		vc->img_h_max = IMG_ROT_HEIGHT_MAX;
	else
		vc->img_h_max = IMG_HEIGHT_MAX;
	vc->src_mul_x = SRC_OFFSET_MULTIPLE * sz_align;
	vc->src_mul_y = SRC_OFFSET_MULTIPLE * sz_align;

	vc->sca_w_min = SCALED_WIDTH_MIN;
	vc->sca_w_max = SCALED_WIDTH_MAX;
	vc->sca_h_min = SCALED_HEIGHT_MIN;
	vc->sca_h_max = SCALED_HEIGHT_MAX;
	vc->sca_mul_w = SCALED_SIZE_MULTIPLE;
	vc->sca_mul_h = SCALED_SIZE_MULTIPLE;

	vc->blk_w_min = BLK_WIDTH_MIN;
	vc->blk_w_max = BLK_WIDTH_MAX;
	vc->blk_h_min = BLK_HEIGHT_MIN;
	vc->blk_h_max = BLK_HEIGHT_MAX;
	vc->blk_mul_w = BLK_SIZE_MULTIPLE;
	vc->blk_mul_h = BLK_SIZE_MULTIPLE;

	if (vi->wb) {
		vc->src_mul_w = DST_SIZE_MULTIPLE * sz_align;
		vc->src_mul_h = DST_SIZE_MULTIPLE * sz_align;
		vc->src_w_min = DST_SIZE_WIDTH_MIN;
		vc->src_w_max = DST_SIZE_WIDTH_MAX;
		vc->src_h_min = DST_SIZE_HEIGHT_MIN;
		vc->src_h_max = DST_SIZE_HEIGHT_MAX;
		vc->img_mul_w = DST_IMG_MULTIPLE * sz_align;
		vc->img_mul_h = DST_IMG_MULTIPLE * sz_align;
		vc->img_w_min = DST_IMG_WIDTH_MIN;
		vc->img_w_max = DST_IMG_WIDTH_MAX;
		vc->img_h_min = DST_IMG_HEIGHT_MIN;
		vc->img_h_max = DST_IMG_HEIGHT_MAX;
		vc->src_mul_x = DST_OFFSET_MULTIPLE * sz_align;
		vc->src_mul_y = DST_OFFSET_MULTIPLE * sz_align;
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
		odma_reg_set_clock_gate_en_all(id, 0);
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
	if (test_bit(DPP_ATTR_CSC, &attr) && test_bit(DPP_ATTR_DPP, &attr))
		dpp_reg_set_csc_params(id, p->eq_mode);
	else if (test_bit(DPP_ATTR_CSC, &attr) && test_bit(DPP_ATTR_ODMA, &attr))
		wb_mux_reg_set_csc_params(id, p->eq_mode);

	if (test_bit(DPP_ATTR_SCALE, &attr))
		dpp_reg_set_scale_ratio(id, p);

	/* configure coordinates and size of IDMA, DPP, ODMA and WB MUX */
	dma_dpp_reg_set_coordinates(id, p, attr);

	if (test_bit(DPP_ATTR_ROT, &attr))
		idma_reg_set_rotation(id, p->rot);

	/* configure base address of IDMA and ODMA */
	dma_reg_set_base_addr(id, p, attr);

	if (test_bit(DPP_ATTR_BLOCK, &attr))
		idma_reg_set_block_mode(id, p->is_block, p->block.x, p->block.y,
				p->block.w, p->block.h);

	/* configure image format of IDMA, DPP, ODMA and WB MUX */
	dma_dpp_reg_set_format(id, p, attr);

	if (test_bit(DPP_ATTR_HDR, &attr))
		dpp_reg_set_hdr_params(id, p);

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

void dma_reg_dump_com_debug_regs(int id)
{
	u32 sel[12] = {0x0000, 0x0100, 0x0200, 0x0204, 0x0205, 0x0300, 0x4000,
		0x4001, 0x4005, 0x8000, 0x8001, 0x8005};

	dpp_info("%s: checked = %d\n", __func__, checked);
	if (checked)
		return;

	dpp_info("-< DMA COMMON DEBUG SFR >-\n");
	dpp_reg_dump_ch_data(id, REG_AREA_DMA_COM, sel, 12);

	checked = true;
}

void dma_reg_dump_debug_regs(int id)
{
	u32 sel_g[11] = {
		0x0000, 0x0001, 0x0002, 0x0004, 0x000A, 0x000B, 0x0400, 0x0401,
		0x0402, 0x0405, 0x0406
	};
	u32 sel_v[39] = {
		0x1000, 0x1001, 0x1002, 0x1004, 0x100A, 0x100B, 0x1400, 0x1401,
		0x1402, 0x1405, 0x1406, 0x2000, 0x2001, 0x2002, 0x2004, 0x200A,
		0x200B, 0x2400, 0x2401, 0x2402, 0x2405, 0x2406, 0x3000, 0x3001,
		0x3002, 0x3004, 0x300A, 0x300B, 0x3400, 0x3401, 0x3402, 0x3405,
		0x3406, 0x4002, 0x4003, 0x4004, 0x4005, 0x4006, 0x4007
	};
	u32 sel_f[12] = {
		0x5100, 0x5101, 0x5104, 0x5105, 0x5200, 0x5202, 0x5204, 0x5205,
		0x5300, 0x5302, 0x5303, 0x5306
	};
	u32 sel_r[22] = {
		0x6100, 0x6101, 0x6102, 0x6103, 0x6104, 0x6105, 0x6200, 0x6201,
		0x6202, 0x6203, 0x6204, 0x6205, 0x6300, 0x6301, 0x6302, 0x6306,
		0x6307, 0x6400, 0x6401, 0x6402, 0x6406, 0x6407
	};
	u32 sel_com[4] = {
		0x7000, 0x7001, 0x7002, 0x7003
	};

	dpp_info("-< DPU_DMA%d DEBUG SFR >-\n", id);
	switch (DPU_CH2DMA(id)) {
	case IDMA_G0:
	case IDMA_G1:
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	case IDMA_VG0:
	case IDMA_VG1:
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_v, 39);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	case IDMA_VGF0:
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_v, 39);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_f, 12);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	case IDMA_VGF1:
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_v, 39);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_f, 12);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_r, 22);
		dpp_reg_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	default:
		dpp_err("DPP%d is wrong ID\n", id);
		return;
	}
}

void dpp_reg_dump_debug_regs(int id)
{
	u32 sel_g[3] = {0x0000, 0x0100, 0x0101};
	u32 sel_vg[19] = {0x0000, 0x0100, 0x0101, 0x0200, 0x0201, 0x0202,
		0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x0208, 0x0300, 0x0301,
		0x0302, 0x0303, 0x0304, 0x0400, 0x0401};
	u32 sel_vgf[37] = {0x0000, 0x0100, 0x0101, 0x0200, 0x0201, 0x0210,
		0x0211, 0x0220, 0x0221, 0x0230, 0x0231, 0x0240, 0x0241, 0x0250,
		0x0251, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306,
		0x0307, 0x0308, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0500,
		0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0600, 0x0601};
	u32 cnt;
	u32 *sel = NULL;
	switch (DPU_CH2DMA(id)) {
	case IDMA_G0:
	case IDMA_G1:
		sel = sel_g;
		cnt = 3;
		break;
	case IDMA_VG0:
	case IDMA_VG1:
		sel = sel_vg;
		cnt = 19;
		break;
	case IDMA_VGF0:
	case IDMA_VGF1:
		sel = sel_vgf;
		cnt = 37;
		break;
	default:
		dpp_err("DPP%d is wrong ID\n", id);
		return;
	}

	dpp_write(id, 0x0C00, 0x1);
	dpp_info("-< DPP%d DEBUG SFR >-\n", id);
	dpp_reg_dump_ch_data(id, REG_AREA_DPP, sel, cnt);
}
