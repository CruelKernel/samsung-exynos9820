/*
 * linux/drivers/video/fbdev/exynos/dpu_9810/decon_reg.c
 *
 * Copyright 2013-2017 Samsung Electronics
 *	  SeungBeom Park <sb1.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../decon.h"

/******************* DECON CAL functions *************************/
static void dpu_reg_set_qactive_pll(u32 id, u32 en)
{
	sysreg_write_mask(id, DISP_DPU_TE_QACTIVE_PLL_EN, en ? ~0 : 0,
			TE_QACTIVE_PLL_EN);
}

#ifdef CONFIG_SUPPORT_HMD
bool is_hmd_running(struct decon_device *decon)
{
#ifdef CONFIG_EXYNOS_COMMON_PANEL
	if ((decon->panel_state != NULL) &&
		(decon->panel_state->hmd_on))
		return false;
	else
		return true;
#else
	return true;
#endif
}
#endif

static int decon_reg_reset(u32 id)
{
	int tries;

	decon_write_mask(id, GLOBAL_CONTROL, ~0, GLOBAL_CONTROL_SRESET);
	for (tries = 2000; tries; --tries) {
		if (~decon_read(id, GLOBAL_CONTROL) & GLOBAL_CONTROL_SRESET)
			break;
		udelay(10);
	}

	if (!tries) {
		decon_err("failed to reset Decon\n");
		return -EBUSY;
	}

	return 0;
}

/* select op mode */
static void decon_reg_set_operation_mode(u32 id, enum decon_psr_mode mode)
{
	u32 val, mask;

	mask = GLOBAL_CONTROL_OPERATION_MODE_F;
	if (mode == DECON_MIPI_COMMAND_MODE)
		val = GLOBAL_CONTROL_OPERATION_MODE_CMD_F;
	else
		val = GLOBAL_CONTROL_OPERATION_MODE_VIDEO_F;
	decon_write_mask(id, GLOBAL_CONTROL, val, mask);
}

static void decon_reg_direct_on_off(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = (GLOBAL_CONTROL_DECON_EN | GLOBAL_CONTROL_DECON_EN_F);
	decon_write_mask(id, GLOBAL_CONTROL, val, mask);
}

static void decon_reg_per_frame_off(u32 id)
{
	decon_write_mask(id, GLOBAL_CONTROL, 0, GLOBAL_CONTROL_DECON_EN_F);
}

static u32 decon_reg_get_idle_status(u32 id)
{
	u32 val;

	val = decon_read(id, GLOBAL_CONTROL);
	if (val & GLOBAL_CONTROL_IDLE_STATUS)
		return 1;

	return 0;
}

static u32 decon_reg_get_run_status(u32 id)
{
	u32 val;

	val = decon_read(id, GLOBAL_CONTROL);
	if (val & GLOBAL_CONTROL_RUN_STATUS)
		return 1;

	return 0;
}

static int decon_reg_wait_run_status_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;
	u32 status;

	do {
		status = decon_reg_get_run_status(id);
		cnt--;
		udelay(delay_time);
	} while (!status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon run status(%u)\n",
								id, status);
		return -EBUSY;
	}

	return 0;
}

/* Determine that DECON is perfectly shuttled off through
 * checking this function
 */
static int decon_reg_wait_run_is_off_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;
	u32 status;

	do {
		status = decon_reg_get_run_status(id);
		cnt--;
		udelay(delay_time);
	} while (status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon run is shut-off(%u)\n",
								id, status);
		return -EBUSY;
	}

	return 0;
}

/* In bring-up, all bits are disabled */
static void decon_reg_set_clkgate_mode(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	/* all unmask */
	mask = CLOCK_CONTROL_0_CG_MASK | CLOCK_CONTROL_0_QACTIVE_MASK;
	decon_write_mask(id, CLOCK_CONTROL_0, val, mask);
}

static void decon_reg_set_te_qactive_pll_mode(u32 id, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	/* all unmask */
	mask = CLOCK_CONTROL_0_TE_QACTIVE_PLL_ON;
	decon_write_mask(0, CLOCK_CONTROL_0, val, mask);
}

/*
 * API is considering real possible Display Scenario
 * such as following examples
 *  < Single display >
 *  < Dual/Triple display >
 *  < Dual display + DP >
 *
 * Current API does not configure various 8K case fully!
 * Therefore, modify/add configuration cases if necessary
 * "Resource Confliction" will happen if enabled simultaneously
 */
static void decon_reg_set_sram_share(u32 id, enum decon_fifo_mode fifo_mode)
{
	u32 val = 0;

	switch (fifo_mode) {
	case DECON_FIFO_04K:
		if (id == 0)
			val = SRAM0_SHARE_ENABLE_F;
		else if (id == 1)
			val = SRAM1_SHARE_ENABLE_F;
		else if (id == 2)
			val = SRAM2_SHARE_ENABLE_F;
		break;
	case DECON_FIFO_08K:
		if (id == 0)
			val = SRAM0_SHARE_ENABLE_F | SRAM1_SHARE_ENABLE_F;
		else if (id == 1)
			val = 0;
		else if (id == 2)
			val = SRAM2_SHARE_ENABLE_F | SRAM3_SHARE_ENABLE_F;
		break;
	case DECON_FIFO_12K:
			if (id == 2) {
				val = SRAM1_SHARE_ENABLE_F | SRAM2_SHARE_ENABLE_F
					| SRAM3_SHARE_ENABLE_F;
			} else {
				decon_err("decon%d can't support SRAM 12KB\n",
										id);
			}
			break;

	case DECON_FIFO_16K:
		val = ALL_SRAM_SHARE_ENABLE;
		break;
	case DECON_FIFO_00K:
	default:
		break;
	}

	decon_write(id, SRAM_SHARE_ENABLE, val);
}

static void decon_reg_set_scaled_image_size(u32 id,
		enum decon_dsi_mode dsi_mode, struct decon_lcd *lcd_info)
{
	u32 val, mask;

	val = SCALED_SIZE_HEIGHT_F(lcd_info->yres) |
			SCALED_SIZE_WIDTH_F(lcd_info->xres);
	mask = SCALED_SIZE_HEIGHT_MASK | SCALED_SIZE_WIDTH_MASK;
	decon_write_mask(id, SCALED_SIZE_CONTROL_0, val, mask);
}

static void decon_reg_set_outfifo_size_ctl0(u32 id, u32 width, u32 height)
{
	u32 val;
	u32 th, mask;

	/* OUTFIFO_0 */
	val = OUTFIFO_HEIGHT_F(height) | OUTFIFO_WIDTH_F(width);
	mask = OUTFIFO_HEIGHT_MASK | OUTFIFO_WIDTH_MASK;
	decon_write(id, OUTFIFO_SIZE_CONTROL_0, val);

	/* may be implemented later by considering 1/2H transfer */
	th = OUTFIFO_TH_1H_F; /* 1H transfer */
	mask = OUTFIFO_TH_MASK;
	decon_write_mask(id, OUTFIFO_TH_CONTROL_0, th, mask);
}

static void decon_reg_set_outfifo_size_ctl1(u32 id, u32 width, u32 height)
{
	u32 val, mask;

	val = OUTFIFO_1_WIDTH_F(width);
	mask = OUTFIFO_1_WIDTH_MASK;

	/* OUTFIFO_1 */
	decon_write_mask(id, OUTFIFO_SIZE_CONTROL_1, val, mask);
}

static void decon_reg_set_outfifo_size_ctl2(u32 id, u32 width, u32 height)
{
	u32 val, mask;

	val = OUTFIFO_COMPRESSED_SLICE_HEIGHT_F(height) |
			OUTFIFO_COMPRESSED_SLICE_WIDTH_F(width);
	mask = OUTFIFO_COMPRESSED_SLICE_HEIGHT_MASK |
				OUTFIFO_COMPRESSED_SLICE_WIDTH_MASK;

	/* OUTFIFO_2 */
	decon_write_mask(id, OUTFIFO_SIZE_CONTROL_2, val, mask);
}

static void decon_reg_set_rgb_order(u32 id, enum decon_rgb_order order)
{
	u32 val, mask;

	val = OUTFIFO_PIXEL_ORDER_SWAP_F(order);
	mask = OUTFIFO_PIXEL_ORDER_SWAP_MASK;
	decon_write_mask(id, OUTFIFO_DATA_ORDER_CONTROL, val, mask);
}

static void decon_reg_set_blender_bg_image_size(u32 id,
		enum decon_dsi_mode dsi_mode, struct decon_lcd *lcd_info)
{
	u32 width, val, mask;

	width = lcd_info->xres;

	if (dsi_mode == DSI_MODE_DUAL_DSI)
		width = width * 2;

	val = BLENDER_BG_HEIGHT_F(lcd_info->yres) | BLENDER_BG_WIDTH_F(width);
	mask = BLENDER_BG_HEIGHT_MASK | BLENDER_BG_WIDTH_MASK;
	decon_write_mask(id, BLENDER_BG_IMAGE_SIZE_0, val, mask);

}

static void decon_reg_set_data_path(u32 id, enum decon_data_path d_path,
		enum decon_scaler_path s_path)
{
	u32 val, mask;

	val = SCALER_PATH_F(s_path) | COMP_OUTIF_PATH_F(d_path);
	mask = SCALER_PATH_MASK | COMP_OUTIF_PATH_MASK;
	decon_write_mask(id, DATA_PATH_CONTROL_2, val, mask);
}

/*
 * Check major configuration of data_path_control
 *    DSCC[7]
 *    DSC_ENC1[5] DSC_ENC0[4]
 *    DP_IF[3]
 *    DSIM_IF1[1] DSIM_IF0[0]
 */
static u32 decon_reg_get_data_path_cfg(u32 id, enum decon_path_cfg con_id)
{
	u32 val;
	u32 d_path;
	u32 bRet = 0;

	val = decon_read(id, DATA_PATH_CONTROL_2);
	d_path = COMP_OUTIF_PATH_GET(val);

	switch (con_id) {
	case PATH_CON_ID_DSCC_EN:
		if (d_path & (0x1 << PATH_CON_ID_DSCC_EN))
			bRet = 1;
		break;
	case PATH_CON_ID_DUAL_DSC:
		if ((d_path & (0x3 << PATH_CON_ID_DUAL_DSC)) == 0x30)
			bRet = 1;
		break;
	case PATH_CON_ID_DP:
		if (d_path & (0x3 << PATH_CON_ID_DP))
			bRet = 1;
		break;
	case PATH_CON_ID_DSIM_IF0:
		if (d_path & (0x1 << PATH_CON_ID_DSIM_IF0))
			bRet = 1;
		break;
	case PATH_CON_ID_DSIM_IF1:
		if (d_path & (0x1 << PATH_CON_ID_DSIM_IF1))
			bRet = 1;
		break;
	default:
		break;
	}

	return bRet;
}

static void decon_reg_set_scaled_size(u32 id, u32 scaled_w, u32 scaled_h)
{
	u32 val, mask;

	val = SCALED_SIZE_HEIGHT_F(scaled_h) |
			SCALED_SIZE_WIDTH_F(scaled_w);
	mask = SCALED_SIZE_HEIGHT_MASK | SCALED_SIZE_WIDTH_MASK;
	decon_write_mask(id, SCALED_SIZE_CONTROL_0, val, mask);
}

/*
 * width : width of updated LCD region
 * height : height of updated LCD region
 * is_dsc : 1: DSC is enabled 0: DSC is disabled
 */
static void decon_reg_set_data_path_size(u32 id, u32 width, u32 height, bool is_dsc,
		u32 dsc_cnt, u32 slice_w, u32 slice_h, u32 ds_en[2])
{
	u32 outfifo_w;

	if (is_dsc)
		outfifo_w = slice_w << ds_en[0];
	else
		outfifo_w = width;

	/* OUTFIFO size is compressed size if DSC is enabled */
	decon_reg_set_outfifo_size_ctl0(id, outfifo_w, height);
	if (dsc_cnt == 2)
		decon_reg_set_outfifo_size_ctl1(id, outfifo_w, 0);
	if (is_dsc)
		decon_reg_set_outfifo_size_ctl2(id, slice_w, slice_h);

	/*
	 * SCALED size is updated LCD size if partial update is operating,
	 * this indicates partial size.
	 */
	decon_reg_set_scaled_size(id, width, height);
}

/*
 * 'DATA_PATH_CONTROL_2' SFR must be set before calling this function!!
 * [width]
 * - no compression  : x-resolution
 * - dsc compression : width_per_enc
 */
static void decon_reg_config_data_path_size(u32 id,
	u32 width, u32 height, u32 overlap_w,
	struct decon_dsc *p, struct decon_param *param)
{
	u32 dual_dsc = 0;
	u32 dual_dsi = 0;
	u32 dsim_if0 = 1;
	u32 dsim_if1 = 0;
	u32 width_f;
	u32 sw;

	dual_dsc = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DUAL_DSC);
	dsim_if0 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF0);
	dsim_if1 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF1);
	if (dsim_if0 && dsim_if1)
		dual_dsi = 1;

	/* OUTFIFO */
	if (param->lcd_info->dsc_enabled) {
		width_f = p->width_per_enc;
		sw = param->lcd_info->dsc_enc_sw;
		/* DSC 1EA */
		if (param->lcd_info->dsc_cnt == 1) {
			decon_reg_set_outfifo_size_ctl0(id, width_f, height);
			decon_reg_set_outfifo_size_ctl2(id,
					sw, p->slice_height);
		} else if (param->lcd_info->dsc_cnt == 2) {	/* DSC 2EA */
			decon_reg_set_outfifo_size_ctl0(id, width_f, height);
			decon_reg_set_outfifo_size_ctl1(id, width_f, 0);
			decon_reg_set_outfifo_size_ctl2(id,
					sw, p->slice_height);
		}
	} else {
		decon_reg_set_outfifo_size_ctl0(id, width, height);
	}
}

/*
 * [ CAUTION ]
 * 'DATA_PATH_CONTROL_2' SFR must be set before calling this function!!
 *
 * [ Implementation Info about CONNECTION ]
 * 1) DECON 0 - DSIMIF0
 * 2) DECON 0 - DSIMIF1
 * 3) DECON 1 - DSIMIF0
 * --> modify code if you want to change connection between dsim_if and dsim
 */

/*
 * later check.
 * Dual DSI connection
 * DP connection
 *
 */

static void decon_reg_set_interface(u32 id, struct decon_mode_info *psr)
{
	/* connection sfrs are changed in Lhotse */
	u32 val = DSIM_CONNECTION_DSIM0_F(0);
	u32 mask = DSIM_CONNECTION_DSIM0_MASK;
	u32 dsim_if0 = 1;
	u32 dsim_if1 = 0;
	u32 dual_dsi = 0;

	if (psr->out_type == DECON_OUT_DSI) {
		dsim_if0 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF0);
		dsim_if1 = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSIM_IF1);
		if (dsim_if0 && dsim_if1)
			dual_dsi = 1;

		if (dual_dsi) {
			/* decon0 - (dsim_if0-dsim0) - (dsim_if1-dsim1) */
			val = DSIM_CONNECTION_DSIM0_F(0)
				| DSIM_CONNECTION_DSIM1_F(1);
			mask =  DSIM_CONNECTION_DSIM0_MASK
				| DSIM_CONNECTION_DSIM1_MASK;
		} else { /* single dsi : DSIM0 only */
			if (dsim_if0) {
				if (id == 0) {
					/* DECON0 - DSIMIF0 - DSIM0 */
					val = DSIM_CONNECTION_DSIM0_F(0);
					mask =  DSIM_CONNECTION_DSIM0_MASK;
				} else if (id == 1) {
					/* DECON1 - DSIMIF0 - DSIM0 */
					val = DSIM_CONNECTION_DSIM0_F(2);
					mask =  DSIM_CONNECTION_DSIM0_MASK;
				}
			}
			if (dsim_if1) {
				if (id == 0) {
					/* DECON0 - DSIMIF1 - DSIM0 */
					val = DSIM_CONNECTION_DSIM0_F(1);
					mask =  DSIM_CONNECTION_DSIM0_MASK;
				}
			}
		}

		decon_write_mask(0, DSIM_CONNECTION_CONTROL, val, mask);
	} else if (psr->out_type == DECON_OUT_DP) {
		/* for MST support */
		if (id == 2) {
			/* decon2 - DP0 : default for single DP */
			val = DP_CONNECTION_SEL_DP0(2);
			mask =  DP_CONNECTION_SEL_DP0_MASK;
		} else if (id == 1) {
			/* decon1 - DP1 */
			val = DP_CONNECTION_SEL_DP1(1);
			mask =  DP_CONNECTION_SEL_DP0_MASK;
		}

		decon_write_mask(0, DP_CONNECTION_CONTROL, val, mask);
	}
}

static void decon_reg_set_bpc(u32 id, struct decon_lcd *lcd_info)
{
	u32 val = 0, mask;

	if (lcd_info->bpc == 10)
		val = GLOBAL_CONTROL_TEN_BPC_MODE_F;

	mask = GLOBAL_CONTROL_TEN_BPC_MODE_MASK;

	decon_write_mask(id, GLOBAL_CONTROL, val, mask);
}

static void decon_reg_config_win_channel(u32 id, u32 win_idx, int ch)
{
	u32 val, mask;

	val = WIN_CHMAP_F(win_idx, ch);
	mask = WIN_CHMAP_MASK(win_idx);
	decon_write_mask(id, DATA_PATH_CONTROL_1, val, mask);
}

static void decon_reg_configure_trigger(u32 id, enum decon_trig_mode mode)
{
	u32 val, mask;

	mask = HW_TRIG_EN;
	val = (mode == DECON_SW_TRIG) ? 0 : ~0;
	decon_write_mask(id, HW_SW_TRIG_CONTROL, val, mask);
}

static void dsc_reg_swreset(u32 dsc_id)
{
	dsc_write_mask(dsc_id, DSC_CONTROL0, 1, DSC_SW_RESET);
}

static void dsc_reg_set_dcg_all(u32 dsc_id, u32 en)
{
	u32 val = 0;

	val = en ? DSC_DCG_EN_ALL_MASK : 0;
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_DCG_EN_ALL_MASK);
}

static void dsc_reg_set_swap(u32 dsc_id, u32 bit_s, u32 byte_s, u32 word_s)
{
	u32 val;

	val = DSC_SWAP(bit_s, byte_s, word_s);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_SWAP_MASK);
}

static void dsc_reg_set_flatness_det_th(u32 dsc_id, u32 th)
{
	u32 val;

	val = DSC_FLATNESS_DET_TH_F(th);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_FLATNESS_DET_TH_MASK);
}

static void dsc_reg_set_slice_mode_change(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_SLICE_MODE_CH_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_SLICE_MODE_CH_MASK);
}

static void dsc_reg_set_auto_clock_gate(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_CG_EN_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_CG_EN_MASK);
}

static void dsc_reg_set_dual_slice(u32 dsc_id, u32 en)
{
	u32 val;

	val = DSC_DUAL_SLICE_EN_F(en);
	dsc_write_mask(dsc_id, DSC_CONTROL0, val, DSC_DUAL_SLICE_EN_MASK);
}

static void dsc_reg_set_remainder(u32 dsc_id, u32 remain)
{
	u32 val;

	val = DSC_REMAINDER_F(remain);
	dsc_write_mask(dsc_id, DSC_CONTROL3, val, DSC_REMAINDER_MASK);
}

static void dsc_reg_set_grpcntline(u32 dsc_id, u32 line)
{
	u32 val;

	val = DSC_GRPCNTLINE_F(line);
	dsc_write_mask(dsc_id, DSC_CONTROL3, val, DSC_GRPCNTLINE_MASK);
}

/*
 * dsc PPS Configuration
 */

/*
 * APIs which user setting or calculation is required are implemented
 * - PPS04 ~ PPS35 except reserved
 * - PPS58 ~ PPS59
 */
static void dsc_reg_set_pps_04_comp_cfg(u32 dsc_id, u32 comp_cfg)
{
	u32 val, mask;

	val = PPS04_COMP_CFG(comp_cfg);
	mask = PPS04_COMP_CFG_MASK;
	dsc_write_mask(dsc_id, DSC_PPS04_07, val, mask);
}

static void dsc_reg_set_pps_05_bit_per_pixel(u32 dsc_id, u32 bpp)
{
	u32 val, mask;

	val = PPS05_BPP(bpp);
	mask = PPS05_BPP_MASK;
	dsc_write_mask(dsc_id, DSC_PPS04_07, val, mask);
}

static void dsc_reg_set_pps_06_07_picture_height(u32 dsc_id, u32 height)
{
	u32 val, mask;

	val = PPS06_07_PIC_HEIGHT(height);
	mask = PPS06_07_PIC_HEIGHT_MASK;
	dsc_write_mask(dsc_id, DSC_PPS04_07, val, mask);
}

static void dsc_reg_set_pps_08_09_picture_width(u32 dsc_id, u32 width)
{
	u32 val, mask;

	val = PPS08_09_PIC_WIDHT(width);
	mask = PPS08_09_PIC_WIDHT_MASK;
	dsc_write_mask(dsc_id, DSC_PPS08_11, val, mask);
}

static void dsc_reg_set_pps_10_11_slice_height(u32 dsc_id, u32 slice_height)
{
	u32 val, mask;

	val = PPS10_11_SLICE_HEIGHT(slice_height);
	mask = PPS10_11_SLICE_HEIGHT_MASK;
	dsc_write_mask(dsc_id, DSC_PPS08_11, val, mask);
}

static void dsc_reg_set_pps_12_13_slice_width(u32 dsc_id, u32 slice_width)
{
	u32 val, mask;

	val = PPS12_13_SLICE_WIDTH(slice_width);
	mask = PPS12_13_SLICE_WIDTH_MASK;
	dsc_write_mask(dsc_id, DSC_PPS12_15, val, mask);
}

/* chunk_size = slice_width */
static void dsc_reg_set_pps_14_15_chunk_size(u32 dsc_id, u32 chunk_size)
{
	u32 val, mask;

	val = PPS14_15_CHUNK_SIZE(chunk_size);
	mask = PPS14_15_CHUNK_SIZE_MASK;
	dsc_write_mask(dsc_id, DSC_PPS12_15, val, mask);
}

static void dsc_reg_set_pps_16_17_init_xmit_delay(u32 dsc_id, u32 xmit_delay)
{
	u32 val, mask;

	val = PPS16_17_INIT_XMIT_DELAY(xmit_delay);
	mask = PPS16_17_INIT_XMIT_DELAY_MASK;
	dsc_write_mask(dsc_id, DSC_PPS16_19, val, mask);
}

static void dsc_reg_set_pps_18_19_init_dec_delay(u32 dsc_id, u32 dec_delay)
{
	u32 val, mask;

	val = PPS18_19_INIT_DEC_DELAY(dec_delay);
	mask = PPS18_19_INIT_DEC_DELAY_MASK;
	dsc_write_mask(dsc_id, DSC_PPS16_19, val, mask);
}

static void dsc_reg_set_pps_21_initial_scale_value(u32 dsc_id, u32 scale_value)
{
	u32 val, mask;

	val = PPS21_INIT_SCALE_VALUE(scale_value);
	mask = PPS21_INIT_SCALE_VALUE_MASK;
	dsc_write_mask(dsc_id, DSC_PPS20_23, val, mask);
}

static void dsc_reg_set_pps_22_23_scale_increment_interval(u32 dsc_id, u32 sc_inc)
{
	u32 val, mask;

	val = PPS22_23_SCALE_INC_INTERVAL(sc_inc);
	mask = PPS22_23_SCALE_INC_INTERVAL_MASK;
	dsc_write_mask(dsc_id, DSC_PPS20_23, val, mask);
}

static void dsc_reg_set_pps_24_25_scale_decrement_interval(u32 dsc_id, u32 sc_dec)
{
	u32 val, mask;

	val = PPS24_25_SCALE_DEC_INTERVAL(sc_dec);
	mask = PPS24_25_SCALE_DEC_INTERVAL_MASK;
	dsc_write_mask(dsc_id, DSC_PPS24_27, val, mask);
}

static void dsc_reg_set_pps_27_first_line_bpg_offset(u32 dsc_id, u32 fl_bpg_off)
{
	u32 val, mask;

	val = PPS27_FL_BPG_OFFSET(fl_bpg_off);
	mask = PPS27_FL_BPG_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS24_27, val, mask);
}

static void dsc_reg_set_pps_28_29_nfl_bpg_offset(u32 dsc_id, u32 nfl_bpg_off)
{
	u32 val, mask;

	val = PPS28_29_NFL_BPG_OFFSET(nfl_bpg_off);
	mask = PPS28_29_NFL_BPG_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS28_31, val, mask);
}

static void dsc_reg_set_pps_30_31_slice_bpg_offset(u32 dsc_id, u32 slice_bpg_off)
{
	u32 val, mask;

	val = PPS30_31_SLICE_BPG_OFFSET(slice_bpg_off);
	mask = PPS30_31_SLICE_BPG_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS28_31, val, mask);
}

static void dsc_reg_set_pps_32_33_initial_offset(u32 dsc_id, u32 init_off)
{
	u32 val, mask;

	val = PPS32_33_INIT_OFFSET(init_off);
	mask = PPS32_33_INIT_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS32_35, val, mask);
}

static void dsc_reg_set_pps_34_35_final_offset(u32 dsc_id, u32 fin_off)
{
	u32 val, mask;

	val = PPS34_35_FINAL_OFFSET(fin_off);
	mask = PPS34_35_FINAL_OFFSET_MASK;
	dsc_write_mask(dsc_id, DSC_PPS32_35, val, mask);
}

static void dsc_reg_set_pps_58_59_rc_range_param0(u32 dsc_id, u32 rc_range_param)
{
	u32 val, mask;

	val = PPS58_59_RC_RANGE_PARAM(rc_range_param);
	mask = PPS58_59_RC_RANGE_PARAM_MASK;
	dsc_write_mask(dsc_id, DSC_PPS56_59, val, mask);
}

/* full size default value */
static u32 dsc_get_dual_slice_mode(struct decon_lcd *lcd_info)
{
	u32 dual_slice_en = 0;

	if (lcd_info->dsc_cnt == 1) {
		if (lcd_info->dsc_slice_num == 2)
			dual_slice_en = 1;
	} else if (lcd_info->dsc_cnt == 2) {
		if (lcd_info->dsc_slice_num == 4)
			dual_slice_en = 1;
	} else {
		dual_slice_en = 0;
	}

	return dual_slice_en;
}

/* full size default value */
static u32 dsc_get_slice_mode_change(struct decon_lcd *lcd_info)
{
	u32 slice_mode_ch = 0;

	if (lcd_info->dsc_cnt == 2) {
		if (lcd_info->dsc_slice_num == 2)
			slice_mode_ch = 1;
	}

	return slice_mode_ch;
}

static void dsc_get_partial_update_info(u32 slice_cnt, u32 dsc_cnt, bool in_slice[4],
		u32 ds_en[2], u32 sm_ch[2])
{
	switch (slice_cnt) {
	case 4:
		if ((in_slice[0] + in_slice[1]) % 2) {
			ds_en[DECON_DSC_ENC0] = 0;
			sm_ch[DECON_DSC_ENC0] = 1;
		} else {
			ds_en[DECON_DSC_ENC0] = 1;
			sm_ch[DECON_DSC_ENC0] = 0;
		}

		if ((in_slice[2] + in_slice[3]) % 2) {
			ds_en[DECON_DSC_ENC1] = 0;
			sm_ch[DECON_DSC_ENC1] = 1;
		} else {
			ds_en[DECON_DSC_ENC1] = 1;
			sm_ch[DECON_DSC_ENC1] = 0;
		}

		break;
	case 2:
		if (dsc_cnt == 2) {
			ds_en[DECON_DSC_ENC0] = 0;
			sm_ch[DECON_DSC_ENC0] = 1;

			ds_en[DECON_DSC_ENC1] = 0;
			sm_ch[DECON_DSC_ENC1] = 1;
		} else {
			if (in_slice[0]) {
				ds_en[DECON_DSC_ENC0] = 0;
				sm_ch[DECON_DSC_ENC0] = 1;
			} else if (in_slice[1]) {
				ds_en[DECON_DSC_ENC0] = 0;
				sm_ch[DECON_DSC_ENC0] = 1;
			} else {
				ds_en[DECON_DSC_ENC0] = 1;
				sm_ch[DECON_DSC_ENC0] = 0;
			}

			ds_en[DECON_DSC_ENC1] = ds_en[DECON_DSC_ENC0];
			sm_ch[DECON_DSC_ENC1] = sm_ch[DECON_DSC_ENC0];
		}
		break;
	case 1:
		ds_en[DECON_DSC_ENC0] = 0;
		sm_ch[DECON_DSC_ENC0] = 0;

		ds_en[DECON_DSC_ENC1] = 0;
		sm_ch[DECON_DSC_ENC1] = 0;
		break;
	default:
		decon_err("Not specified case for Partial Update in DSC!\n");
		break;
	}
}

static void dsc_reg_config_control(u32 dsc_id, u32 ds_en, u32 sm_ch)
{
	dsc_reg_set_dcg_all(dsc_id, 0);	/* No clock gating */
	dsc_reg_set_swap(dsc_id, 0x0, 0x1, 0x0);
	/* flatness detection is fixed 2@8bpc / 8@10bpc / 32@12bpc */
	dsc_reg_set_flatness_det_th(dsc_id, 0x2);
	dsc_reg_set_auto_clock_gate(dsc_id, 0);	/* No auto clock gating */
	dsc_reg_set_dual_slice(dsc_id, ds_en);
	dsc_reg_set_slice_mode_change(dsc_id, sm_ch);
}

static void dsc_reg_config_control_width(u32 dsc_id, u32 slice_width)
{

	u32 dsc_remainder;
	u32 dsc_grpcntline;

	if (slice_width % 3)
		dsc_remainder = slice_width % 3;
	else
		dsc_remainder = 3;

	dsc_reg_set_remainder(dsc_id, dsc_remainder);
	dsc_grpcntline = (slice_width + 2) / 3;
	dsc_reg_set_grpcntline(dsc_id, dsc_grpcntline);
}

/*
 * overlap_w
 * - default : 0
 * - range : [0, 32] & (multiples of 2)
 *    if non-zero value is applied, this means slice_w increasing.
 *    therefore, DECON & DSIM setting must also be aligned.
 *    --> must check if DDI module is supporting this feature !!!
 */
static void dsc_calc_pps_info(struct decon_lcd *lcd_info, u32 dscc_en,
	struct decon_dsc *dsc_enc)
{
	u32 width, height;
	u32 slice_width, slice_height;
	u32 pic_width, pic_height;
	u32 width_eff;
	u32 dual_slice_en = 0;
	u32 bpp, chunk_size;
	u32 slice_bits;
	u32 groups_per_line, groups_total;

	/* initial values, also used for other pps calcualtion */
	u32 rc_model_size = 0x2000;
	u32 num_extra_mux_bits = 246;
	u32 initial_xmit_delay = 0x200;
	u32 initial_dec_delay = 0x4c0;
	/* when 'slice_w >= 70' */
	u32 initial_scale_value = 0x20;
	u32 first_line_bpg_offset = 0x0c;
	u32 initial_offset = 0x1800;
	u32 rc_range_parameters = 0x0102;

	u32 final_offset, final_scale;
	u32 flag, nfl_bpg_offset, slice_bpg_offset;
	u32 scale_increment_interval, scale_decrement_interval;
	u32 slice_width_byte_unit, comp_slice_width_byte_unit;
	u32 comp_slice_width_pixel_unit;
	u32 overlap_w = 0;
	u32 dsc_enc0_w = 0, dsc_enc0_h;
	u32 dsc_enc1_w = 0, dsc_enc1_h;
	u32 i, j;

	width = lcd_info->xres;
	height = lcd_info->yres;

	overlap_w = dsc_enc->overlap_w;

	if (dscc_en)
		/* OVERLAP can be used in the dual-slice case (if one ENC) */
		width_eff = (width >> 1) + overlap_w;
	else
		width_eff = width + overlap_w;

	pic_width = width_eff;
	dual_slice_en = dsc_get_dual_slice_mode(lcd_info);
	if (dual_slice_en)
		slice_width = width_eff >> 1;
	else
		slice_width = width_eff;

	pic_height = height;
	slice_height = lcd_info->dsc_slice_h;

	bpp = 8;
	chunk_size = slice_width;
	slice_bits = 8 * chunk_size * slice_height;

	while ((slice_bits - num_extra_mux_bits) % 48)
		num_extra_mux_bits--;

	groups_per_line = (slice_width + 2) / 3;
	groups_total = groups_per_line * slice_height;

	final_offset = rc_model_size - ((initial_xmit_delay * (8<<4) + 8)>>4)
		+ num_extra_mux_bits;
	final_scale = 8 * rc_model_size / (rc_model_size - final_offset);

	flag = (first_line_bpg_offset * 2048) % (slice_height - 1);
	nfl_bpg_offset = (first_line_bpg_offset * 2048) / (slice_height - 1);
	if (flag)
		nfl_bpg_offset = nfl_bpg_offset + 1;

	flag = 2048 * (rc_model_size - initial_offset + num_extra_mux_bits)
		% groups_total;
	slice_bpg_offset = 2048
		* (rc_model_size - initial_offset + num_extra_mux_bits)
		/ groups_total;
	if (flag)
		slice_bpg_offset = slice_bpg_offset + 1;

	scale_increment_interval = (2048 * final_offset) / ((final_scale - 9)
		* (nfl_bpg_offset + slice_bpg_offset));
	scale_decrement_interval = groups_per_line / (initial_scale_value - 8);

	/* 3bytes per pixel */
	slice_width_byte_unit = slice_width * 3;
	/* integer value, /3 for 1/3 compression */
	comp_slice_width_byte_unit = slice_width_byte_unit / 3;
	/* integer value, /3 for pixel unit */
	comp_slice_width_pixel_unit = comp_slice_width_byte_unit / 3;

	i = comp_slice_width_byte_unit % 3;
	j = comp_slice_width_pixel_unit % 2;

	if (i == 0 && j == 0) {
		dsc_enc0_w = comp_slice_width_pixel_unit;
		dsc_enc0_h = pic_height;
		if (dscc_en) {
			dsc_enc1_w = comp_slice_width_pixel_unit;
			dsc_enc1_h = pic_height;
		}
	} else if (i == 0 && j != 0) {
		dsc_enc0_w = comp_slice_width_pixel_unit + 1;
		dsc_enc0_h = pic_height;
		if (dscc_en) {
			dsc_enc1_w = comp_slice_width_pixel_unit + 1;
			dsc_enc1_h = pic_height;
		}
	} else if (i != 0) {
		while (1) {
			comp_slice_width_pixel_unit++;
			j = comp_slice_width_pixel_unit % 2;
			if (j == 0)
				break;
		}
		dsc_enc0_w = comp_slice_width_pixel_unit;
		dsc_enc0_h = pic_height;
		if (dscc_en) {
			dsc_enc1_w = comp_slice_width_pixel_unit;
			dsc_enc1_h = pic_height;
		}
	}

	if (dual_slice_en) {
		dsc_enc0_w = dsc_enc0_w * 2;
		if (dscc_en)
			dsc_enc1_w = dsc_enc1_w * 2;
	}

	/* Save information to structure variable */
	dsc_enc->comp_cfg = 0x30;
	dsc_enc->bit_per_pixel = bpp << 4;
	dsc_enc->pic_height = pic_height;
	dsc_enc->pic_width = pic_width;
	dsc_enc->slice_height = slice_height;
	dsc_enc->slice_width = slice_width;
	dsc_enc->chunk_size = chunk_size;
	dsc_enc->initial_xmit_delay = initial_xmit_delay;
	dsc_enc->initial_dec_delay = initial_dec_delay;
	dsc_enc->initial_scale_value = initial_scale_value;
	dsc_enc->scale_increment_interval = scale_increment_interval;
	dsc_enc->scale_decrement_interval = scale_decrement_interval;
	dsc_enc->first_line_bpg_offset = first_line_bpg_offset;
	dsc_enc->nfl_bpg_offset = nfl_bpg_offset;
	dsc_enc->slice_bpg_offset = slice_bpg_offset;
	dsc_enc->initial_offset = initial_offset;
	dsc_enc->final_offset = final_offset;
	dsc_enc->rc_range_parameters = rc_range_parameters;

	dsc_enc->width_per_enc = dsc_enc0_w;
}

static void dsc_reg_set_pps(u32 dsc_id, struct decon_dsc *dsc_enc)
{
	dsc_reg_set_pps_04_comp_cfg(dsc_id, dsc_enc->comp_cfg);
	dsc_reg_set_pps_05_bit_per_pixel(dsc_id, dsc_enc->bit_per_pixel);
	dsc_reg_set_pps_06_07_picture_height(dsc_id, dsc_enc->pic_height);

	dsc_reg_set_pps_08_09_picture_width(dsc_id, dsc_enc->pic_width);
	dsc_reg_set_pps_10_11_slice_height(dsc_id, dsc_enc->slice_height);
	dsc_reg_set_pps_12_13_slice_width(dsc_id, dsc_enc->slice_width);
	dsc_reg_set_pps_14_15_chunk_size(dsc_id, dsc_enc->chunk_size);

	dsc_reg_set_pps_16_17_init_xmit_delay(dsc_id,
		dsc_enc->initial_xmit_delay);
#ifndef VESA_SCR_V4
	dsc_reg_set_pps_18_19_init_dec_delay(dsc_id, 0x01B4);
#else
	dsc_reg_set_pps_18_19_init_dec_delay(dsc_id,
		dsc_enc->initial_dec_delay);
#endif
	dsc_reg_set_pps_21_initial_scale_value(dsc_id,
		dsc_enc->initial_scale_value);

	dsc_reg_set_pps_22_23_scale_increment_interval(dsc_id,
		dsc_enc->scale_increment_interval);
	dsc_reg_set_pps_24_25_scale_decrement_interval(dsc_id,
		dsc_enc->scale_decrement_interval);

	dsc_reg_set_pps_27_first_line_bpg_offset(dsc_id,
		dsc_enc->first_line_bpg_offset);
	dsc_reg_set_pps_28_29_nfl_bpg_offset(dsc_id, dsc_enc->nfl_bpg_offset);

	dsc_reg_set_pps_30_31_slice_bpg_offset(dsc_id,
		dsc_enc->slice_bpg_offset);
	dsc_reg_set_pps_32_33_initial_offset(dsc_id, dsc_enc->initial_offset);
	dsc_reg_set_pps_34_35_final_offset(dsc_id, dsc_enc->final_offset);

	/* min_qp0 = 0 , max_qp0 = 4 , bpg_off0 = 2 */
	dsc_reg_set_pps_58_59_rc_range_param0(dsc_id,
		dsc_enc->rc_range_parameters);
#ifndef VESA_SCR_V4
	/* PPS79 ~ PPS87 : 3HF4 is different with VESA SCR v4 */
	dsc_write(dsc_id, 0x006C, 0x1AB62AF6);
	dsc_write(dsc_id, 0x0070, 0x2B342B74);
	dsc_write(dsc_id, 0x0074, 0x3B746BF4);
#endif
}

/*
 * Following PPS SFRs will be set from DDI PPS Table (DSC Decoder)
 * : not 'fix' type
 *   - PPS04 ~ PPS35
 *   - PPS58 ~ PPS59
 *   <PPS Table e.g.> SEQ_PPS_SLICE4[] @ s6e3hf4_param.h
 */
static void dsc_get_decoder_pps_info(struct decon_dsc *dsc_dec,
		const unsigned char pps_t[90])
{
	dsc_dec->comp_cfg = (u32) pps_t[4];
	dsc_dec->bit_per_pixel = (u32) pps_t[5];
	dsc_dec->pic_height = (u32) (pps_t[6] << 8 | pps_t[7]);
	dsc_dec->pic_width = (u32) (pps_t[8] << 8 | pps_t[9]);
	dsc_dec->slice_height = (u32) (pps_t[10] << 8 | pps_t[11]);
	dsc_dec->slice_width = (u32) (pps_t[12] << 8 | pps_t[13]);
	dsc_dec->chunk_size = (u32) (pps_t[14] << 8 | pps_t[15]);
	dsc_dec->initial_xmit_delay = (u32) (pps_t[16] << 8 | pps_t[17]);
	dsc_dec->initial_dec_delay = (u32) (pps_t[18] << 8 | pps_t[19]);
	dsc_dec->initial_scale_value = (u32) pps_t[21];
	dsc_dec->scale_increment_interval = (u32) (pps_t[22] << 8 | pps_t[23]);
	dsc_dec->scale_decrement_interval = (u32) (pps_t[24] << 8 | pps_t[25]);
	dsc_dec->first_line_bpg_offset = (u32) pps_t[27];
	dsc_dec->nfl_bpg_offset = (u32) (pps_t[28] << 8 | pps_t[29]);
	dsc_dec->slice_bpg_offset = (u32) (pps_t[30] << 8 | pps_t[31]);
	dsc_dec->initial_offset = (u32) (pps_t[32] << 8 | pps_t[33]);
	dsc_dec->final_offset = (u32) (pps_t[34] << 8 | pps_t[35]);
	dsc_dec->rc_range_parameters = (u32) (pps_t[58] << 8 | pps_t[59]);
}

static u32 dsc_cmp_pps_enc_dec(struct decon_dsc *p_enc, struct decon_dsc *p_dec)
{
	u32 diff_cnt = 0;

	if (p_enc->comp_cfg != p_dec->comp_cfg) {
		diff_cnt++;
		decon_dbg("[dsc_pps] comp_cfg (enc:dec = %d:%d)\n",
			p_enc->comp_cfg, p_dec->comp_cfg);
	}
	if (p_enc->bit_per_pixel != p_dec->bit_per_pixel) {
		diff_cnt++;
		decon_dbg("[dsc_pps] bit_per_pixel (enc:dec = %d:%d)\n",
			p_enc->bit_per_pixel, p_dec->bit_per_pixel);
	}
	if (p_enc->pic_height != p_dec->pic_height) {
		diff_cnt++;
		decon_dbg("[dsc_pps] pic_height (enc:dec = %d:%d)\n",
			p_enc->pic_height, p_dec->pic_height);
	}
	if (p_enc->pic_width != p_dec->pic_width) {
		diff_cnt++;
		decon_dbg("[dsc_pps] pic_width (enc:dec = %d:%d)\n",
			p_enc->pic_width, p_dec->pic_width);
	}
	if (p_enc->slice_height != p_dec->slice_height) {
		diff_cnt++;
		decon_dbg("[dsc_pps] slice_height (enc:dec = %d:%d)\n",
			p_enc->slice_height, p_dec->slice_height);
	}
	if (p_enc->slice_width != p_dec->slice_width) {
		diff_cnt++;
		decon_dbg("[dsc_pps] slice_width (enc:dec = %d:%d)\n",
			p_enc->slice_width, p_dec->slice_width);
	}
	if (p_enc->chunk_size != p_dec->chunk_size) {
		diff_cnt++;
		decon_dbg("[dsc_pps] chunk_size (enc:dec = %d:%d)\n",
			p_enc->chunk_size, p_dec->chunk_size);
	}
	if (p_enc->initial_xmit_delay != p_dec->initial_xmit_delay) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_xmit_delay (enc:dec = %d:%d)\n",
			p_enc->initial_xmit_delay, p_dec->initial_xmit_delay);
	}
	if (p_enc->initial_dec_delay != p_dec->initial_dec_delay) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_dec_delay (enc:dec = %d:%d)\n",
			p_enc->initial_dec_delay, p_dec->initial_dec_delay);
	}
	if (p_enc->initial_scale_value != p_dec->initial_scale_value) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_scale_value (enc:dec = %d:%d)\n",
			p_enc->initial_scale_value,
			p_dec->initial_scale_value);
	}
	if (p_enc->scale_increment_interval !=
			p_dec->scale_increment_interval) {
		diff_cnt++;
		decon_dbg("[dsc_pps] scale_inc_interval (enc:dec = %d:%d)\n",
					p_enc->scale_increment_interval,
					p_dec->scale_increment_interval);
	}
	if (p_enc->scale_decrement_interval !=
			p_dec->scale_decrement_interval) {
		diff_cnt++;
		decon_dbg("[dsc_pps] scale_dec_interval (enc:dec = %d:%d)\n",
					p_enc->scale_decrement_interval,
					p_dec->scale_decrement_interval);
	}
	if (p_enc->first_line_bpg_offset != p_dec->first_line_bpg_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] first_line_bpg_offset (enc:dec = %d:%d)\n",
					p_enc->first_line_bpg_offset,
					p_dec->first_line_bpg_offset);
	}
	if (p_enc->nfl_bpg_offset != p_dec->nfl_bpg_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] nfl_bpg_offset (enc:dec = %d:%d)\n",
			p_enc->nfl_bpg_offset, p_dec->nfl_bpg_offset);
	}
	if (p_enc->slice_bpg_offset != p_dec->slice_bpg_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] slice_bpg_offset (enc:dec = %d:%d)\n",
			p_enc->slice_bpg_offset, p_dec->slice_bpg_offset);
	}
	if (p_enc->initial_offset != p_dec->initial_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] initial_offset (enc:dec = %d:%d)\n",
			p_enc->initial_offset, p_dec->initial_offset);
	}
	if (p_enc->final_offset != p_dec->final_offset) {
		diff_cnt++;
		decon_dbg("[dsc_pps] final_offset (enc:dec = %d:%d)\n",
			p_enc->final_offset, p_dec->final_offset);
	}
	if (p_enc->rc_range_parameters != p_dec->rc_range_parameters) {
		diff_cnt++;
		decon_dbg("[dsc_pps] rc_range_parameters (enc:dec = %d:%d)\n",
						p_enc->rc_range_parameters,
						p_dec->rc_range_parameters);
	}

	decon_dbg("[dsc_pps] total different count : %d\n", diff_cnt);

	return diff_cnt;
}

static void dsc_reg_set_partial_update(u32 dsc_id, u32 dual_slice_en,
	u32 slice_mode_ch, u32 pic_h)
{
	/*
	 * Following SFRs must be considered
	 * - dual_slice_en
	 * - slice_mode_change
	 * - picture_height
	 * - picture_width (don't care @KC) : decided by DSI (-> dual: /2)
	 */
	dsc_reg_set_dual_slice(dsc_id, dual_slice_en);
	dsc_reg_set_slice_mode_change(dsc_id, slice_mode_ch);
	dsc_reg_set_pps_06_07_picture_height(dsc_id, pic_h);
}

/*
 * This table is only used to check DSC setting value when debugging
 * Copy or Replace table's data from current using LCD information
 * ( e.g. : SEQ_PPS_SLICE4 @ s6e3hf4_param.h )
 */
static const unsigned char DDI_PPS_INFO[] = {
	0x11, 0x00, 0x00, 0x89, 0x30,
	0x80, 0x0A, 0x00, 0x05, 0xA0,
	0x00, 0x40, 0x01, 0x68, 0x01,
	0x68, 0x02, 0x00, 0x01, 0xB4,

	0x00, 0x20, 0x04, 0xF2, 0x00,
	0x05, 0x00, 0x0C, 0x01, 0x87,
	0x02, 0x63, 0x18, 0x00, 0x10,
	0xF0, 0x03, 0x0C, 0x20, 0x00,

	0x06, 0x0B, 0x0B, 0x33, 0x0E,
	0x1C, 0x2A, 0x38, 0x46, 0x54,
	0x62, 0x69, 0x70, 0x77, 0x79,
	0x7B, 0x7D, 0x7E, 0x01, 0x02,

	0x01, 0x00, 0x09, 0x40, 0x09,
	0xBE, 0x19, 0xFC, 0x19, 0xFA,
	0x19, 0xF8, 0x1A, 0x38, 0x1A,
	0x78, 0x1A, 0xB6, 0x2A, 0xF6,

	0x2B, 0x34, 0x2B, 0x74, 0x3B,
	0x74, 0x6B, 0xF4, 0x00, 0x00
};

static void dsc_reg_set_encoder(u32 id, struct decon_param *p,
	struct decon_dsc *dsc_enc, u32 chk_en)
{
	u32 dsc_id;
	u32 dscc_en = 1;
	u32 ds_en = 0;
	u32 sm_ch = 0;
	struct decon_lcd *lcd_info = p->lcd_info;
	/* DDI PPS table : for compare with ENC PPS value */
	struct decon_dsc dsc_dec;
	/* set corresponding table like 'SEQ_PPS_SLICE4' */
	const unsigned char *pps_t = DDI_PPS_INFO;

	ds_en = dsc_get_dual_slice_mode(lcd_info);
	decon_dbg("dual slice(%d)\n", ds_en);

	sm_ch = dsc_get_slice_mode_change(lcd_info);
	decon_dbg("slice mode change(%d)\n", sm_ch);

	dscc_en = decon_reg_get_data_path_cfg(id, PATH_CON_ID_DSCC_EN);
	dsc_calc_pps_info(lcd_info, dscc_en, dsc_enc);

	if (id == 1) {
		dsc_reg_config_control(DECON_DSC_ENC1, ds_en, sm_ch);
		dsc_reg_config_control_width(DECON_DSC_ENC1,
					dsc_enc->slice_width);
		dsc_reg_set_pps(DECON_DSC_ENC1, dsc_enc);
	} else if (id == 2) {	/* only for DP */
		dsc_reg_config_control(DECON_DSC_ENC2, ds_en, sm_ch);
		dsc_reg_config_control_width(DECON_DSC_ENC2,
					dsc_enc->slice_width);
		dsc_reg_set_pps(DECON_DSC_ENC2, dsc_enc);
	} else {
		for (dsc_id = 0; dsc_id < lcd_info->dsc_cnt; dsc_id++) {
			dsc_reg_config_control(dsc_id, ds_en, sm_ch);
			dsc_reg_config_control_width(dsc_id,
						dsc_enc->slice_width);
			dsc_reg_set_pps(dsc_id, dsc_enc);
		}
	}

	if (chk_en) {
		dsc_get_decoder_pps_info(&dsc_dec, pps_t);
		if (dsc_cmp_pps_enc_dec(dsc_enc, &dsc_dec))
			decon_dbg("[WARNING] Check PPS value!!\n");
	}

}

static int dsc_reg_init(u32 id, struct decon_param *p, u32 overlap_w, u32 swrst)
{
	u32 dsc_id;
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_dsc dsc_enc;

	/* Basically, all SW-resets in DPU are not necessary */
	if (swrst) {
		for (dsc_id = 0; dsc_id < lcd_info->dsc_cnt; dsc_id++)
			dsc_reg_swreset(dsc_id);
	}

	dsc_enc.overlap_w = overlap_w;
	dsc_reg_set_encoder(id, p, &dsc_enc, 0);
	decon_reg_config_data_path_size(id,
		dsc_enc.width_per_enc, lcd_info->yres, overlap_w, &dsc_enc, p);

	return 0;
}

static void decon_reg_clear_int_all(u32 id)
{
	u32 mask;

	mask = (DPU_FRAME_DONE_INT_EN
			| DPU_FRAME_START_INT_EN);
	decon_write_mask(id, INTERRUPT_PENDING, ~0, mask);

	mask = (DPU_RESOURCE_CONFLICT_INT_EN
		| DPU_TIME_OUT_INT_EN);
	decon_write_mask(id, EXTRA_INTERRUPT_PENDING, ~0, mask);
}

static void decon_reg_configure_lcd(u32 id, struct decon_param *p)
{
	u32 overlap_w = 0;
	enum decon_data_path d_path = DPATH_DSCENC0_OUTFIFO0_DSIMIF0;
	enum decon_scaler_path s_path = SCALERPATH_OFF;

	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;
	enum decon_dsi_mode dsi_mode = psr->dsi_mode;
	enum decon_rgb_order rgb_order = DECON_RGB;

	if ((psr->out_type == DECON_OUT_DSI)
		&& !(lcd_info->dsc_enabled))
		rgb_order = DECON_BGR;
	else
		rgb_order = DECON_RGB;
	decon_reg_set_rgb_order(id, rgb_order);

	if (lcd_info->dsc_enabled) {
		if (lcd_info->dsc_cnt == 1)
			d_path = (id == 0) ?
				DPATH_DSCENC0_OUTFIFO0_DSIMIF0 :
				DECON2_DSCENC2_OUTFIFO0_DPIF;
		else if (lcd_info->dsc_cnt == 2 && !id)
			d_path = DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF0;
		else
			decon_err("[decon%d] dsc_cnt=%d : not supported\n",
				id, lcd_info->dsc_cnt);

		decon_reg_set_data_path(id, d_path, s_path);
		/* call decon_reg_config_data_path_size () inside */
		dsc_reg_init(id, p, overlap_w, 0);
	} else {
		if (dsi_mode == DSI_MODE_DUAL_DSI)
			d_path = DPATH_NOCOMP_SPLITTER_OUTFIFO01_DSIMIF01;
		else
			d_path = (id == 0) ?
				DPATH_NOCOMP_OUTFIFO0_DSIMIF0 :
				DECON2_NOCOMP_OUTFIFO0_DPIF;

		decon_reg_set_data_path(id, d_path, s_path);

		decon_reg_config_data_path_size(id,
			lcd_info->xres, lcd_info->yres, overlap_w, NULL, p);

		if (id == 2)
			decon_reg_set_bpc(id, lcd_info);
	}

	decon_reg_per_frame_off(id);
}

static void decon_reg_init_probe(u32 id, u32 dsi_idx, struct decon_param *p)
{
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;
	enum decon_data_path d_path = DPATH_DSCENC0_OUTFIFO0_DSIMIF0;
	enum decon_scaler_path s_path = SCALERPATH_OFF;
	enum decon_rgb_order rgb_order = DECON_RGB;
	enum decon_dsi_mode dsi_mode = psr->dsi_mode;
	u32 overlap_w = 0; /* default=0 : range=[0, 32] & (multiples of 2) */

	dpu_reg_set_qactive_pll(id, true);

	decon_reg_set_clkgate_mode(id, 0);

	decon_reg_set_sram_share(id, DECON_FIFO_04K);

	decon_reg_set_operation_mode(id, psr->psr_mode);

	decon_reg_set_blender_bg_image_size(id, psr->dsi_mode, lcd_info);

	decon_reg_set_scaled_image_size(id, psr->dsi_mode, lcd_info);

	/*
	 * same as decon_reg_configure_lcd(...) function
	 * except using decon_reg_update_req_global(id)
	 * instead of decon_reg_direct_on_off(id, 0)
	 */
	if (lcd_info->dsc_enabled)
		rgb_order = DECON_RGB;
	else
		rgb_order = DECON_BGR;
	decon_reg_set_rgb_order(id, rgb_order);

	if (lcd_info->dsc_enabled) {
		if (lcd_info->dsc_cnt == 1)
			d_path = (id == 0) ?
				DPATH_DSCENC0_OUTFIFO0_DSIMIF0 :
				DECON2_DSCENC2_OUTFIFO0_DPIF;
		else if (lcd_info->dsc_cnt == 2 && !id)
			d_path = DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF0;
		else
			decon_err("[decon%d] dsc_cnt=%d : not supported\n",
				id, lcd_info->dsc_cnt);

		decon_reg_set_data_path(id, d_path, s_path);
		/* call decon_reg_config_data_path_size () inside */
		dsc_reg_init(id, p, overlap_w, 0);
	} else {
		if (dsi_mode == DSI_MODE_DUAL_DSI)
			d_path = DPATH_NOCOMP_SPLITTER_OUTFIFO01_DSIMIF01;
		else
			d_path = (id == 0) ?
				DPATH_NOCOMP_OUTFIFO0_DSIMIF0 :
				DECON2_NOCOMP_OUTFIFO0_DPIF;

		decon_reg_set_data_path(id, d_path, s_path);

		decon_reg_config_data_path_size(id,
			lcd_info->xres, lcd_info->yres, overlap_w, NULL, p);
	}
}


static void decon_reg_set_blender_bg_size(u32 id, enum decon_dsi_mode dsi_mode,
		u32 bg_w, u32 bg_h)
{
	u32 width, val, mask;

	width = bg_w;

	if (dsi_mode == DSI_MODE_DUAL_DSI)
		width = width * 2;

	val = BLENDER_BG_HEIGHT_F(bg_h) | BLENDER_BG_WIDTH_F(width);
	mask = BLENDER_BG_HEIGHT_MASK | BLENDER_BG_WIDTH_MASK;
	decon_write_mask(id, BLENDER_BG_IMAGE_SIZE_0, val, mask);
}

static int decon_reg_stop_perframe(u32 id, u32 dsi_idx,
		struct decon_mode_info *psr, u32 fps)
{
	int ret = 0;
	int timeout_value = 0;

	decon_dbg("%s +\n", __func__);

	if ((psr->psr_mode == DECON_MIPI_COMMAND_MODE) &&
			(psr->trig_mode == DECON_HW_TRIG)) {
		decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
	}

	/* perframe stop */
	decon_reg_per_frame_off(id);

	decon_reg_update_req_global(id);

	/* timeout : 1 / fps + 20% margin */
	timeout_value = 1000 / fps * 12 / 10 + 5;
	ret = decon_reg_wait_run_is_off_timeout(id, timeout_value * MSEC);

#if defined(CONFIG_EXYNOS_DISPLAYPORT)
	if (psr->out_type == DECON_OUT_DP)
		displayport_reg_lh_p_ch_power(0);
#endif

	decon_dbg("%s -\n", __func__);
	return ret;
}

static int decon_reg_stop_inst(u32 id, u32 dsi_idx, struct decon_mode_info *psr,
		u32 fps)
{
	int ret = 0;
	int timeout_value = 0;

	decon_dbg("%s +\n", __func__);

	if ((psr->psr_mode == DECON_MIPI_COMMAND_MODE) &&
			(psr->trig_mode == DECON_HW_TRIG)) {
		decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
	}

	/* instant stop */
	decon_reg_direct_on_off(id, 0);

	decon_reg_update_req_global(id);

#if defined(CONFIG_EXYNOS_DISPLAYPORT)
	if (psr->out_type == DECON_OUT_DP)
		displayport_reg_lh_p_ch_power(0);
#endif

	/* timeout : 1 / fps + 20% margin */
	timeout_value = 1000 / fps * 12 / 10 + 5;
	ret = decon_reg_wait_run_is_off_timeout(id, timeout_value * MSEC);

	decon_dbg("%s -\n", __func__);
	return ret;
}

void decon_reg_set_win_enable(u32 id, u32 win_idx, u32 en)
{
	u32 val, mask;

	val = en ? ~0 : 0;
	mask = WIN_EN_F(win_idx);
	decon_write_mask(id, DATA_PATH_CONTROL_0, val, mask);
	decon_dbg("%s: 0x%x\n", __func__, decon_read(id, DATA_PATH_CONTROL_0));
}

/*
 * argb_color : 32-bit
 * A[31:24] - R[23:16] - G[15:8] - B[7:0]
 */
static void decon_reg_set_win_mapcolor(u32 id, u32 win_idx, u32 argb_color)
{
	u32 val, mask;
	u32 mc_alpha = 0, mc_red = 0;
	u32 mc_green = 0, mc_blue = 0;

	mc_alpha = (argb_color >> 24) & 0xFF;
	mc_red = (argb_color >> 16) & 0xFF;
	mc_green = (argb_color >> 8) & 0xFF;
	mc_blue = (argb_color >> 0) & 0xFF;

	val = WIN_MAPCOLOR_A_F(mc_alpha) | WIN_MAPCOLOR_R_F(mc_red);
	mask = WIN_MAPCOLOR_A_MASK | WIN_MAPCOLOR_R_MASK;
	decon_write_mask(id, WIN_COLORMAP_0(win_idx), val, mask);

	val = WIN_MAPCOLOR_G_F(mc_green) | WIN_MAPCOLOR_B_F(mc_blue);
	mask = WIN_MAPCOLOR_G_MASK | WIN_MAPCOLOR_B_MASK;
	decon_write_mask(id, WIN_COLORMAP_1(win_idx), val, mask);
}

static void decon_reg_set_win_plane_alpha(u32 id, u32 win_idx, u32 a0, u32 a1)
{
	u32 val, mask;

	val = WIN_ALPHA1_F(a1) | WIN_ALPHA0_F(a0);
	mask = WIN_ALPHA1_MASK | WIN_ALPHA0_MASK;
	decon_write_mask(id, WIN_CONTROL_0(win_idx), val, mask);
}

static void decon_reg_set_winmap(u32 id, u32 win_idx, u32 color, u32 en)
{
	u32 val, mask;

	/* Enable */
	val = en ? ~0 : 0;
	mask = WIN_MAPCOLOR_EN_F(win_idx);
	decon_write_mask(id, DATA_PATH_CONTROL_0, val, mask);
	decon_dbg("%s: 0x%x\n", __func__, decon_read(id, DATA_PATH_CONTROL_0));

	/* Color Set */
	decon_reg_set_win_mapcolor(0, win_idx, color);
}

/* ALPHA_MULT selection used in (a',b',c',d') coefficient */
static void decon_reg_set_win_alpha_mult(u32 id, u32 win_idx, u32 a_sel)
{
	u32 val, mask;

	val = WIN_ALPHA_MULT_SRC_SEL_F(a_sel);
	mask = WIN_ALPHA_MULT_SRC_SEL_MASK;
	decon_write_mask(id, WIN_CONTROL_0(win_idx), val, mask);
}

static void decon_reg_set_win_sub_coeff(u32 id, u32 win_idx,
		u32 fgd, u32 bgd, u32 fga, u32 bga)
{
	u32 val, mask;

	/*
	 * [ Blending Equation ]
	 * Color : Cr = (a x Cf) + (b x Cb)  <Cf=FG pxl_C, Cb=BG pxl_C>
	 * Alpha : Ar = (c x Af) + (d x Ab)  <Af=FG pxl_A, Ab=BG pxl_A>
	 *
	 * [ User-defined ]
	 * a' = WINx_FG_ALPHA_D_SEL : Af' that is multiplied by FG Pixel Color
	 * b' = WINx_BG_ALPHA_D_SEL : Ab' that is multiplied by BG Pixel Color
	 * c' = WINx_FG_ALPHA_A_SEL : Af' that is multiplied by FG Pixel Alpha
	 * d' = WINx_BG_ALPHA_A_SEL : Ab' that is multiplied by BG Pixel Alpha
	 */

	val = (WIN_FG_ALPHA_D_SEL_F(fgd)
		| WIN_BG_ALPHA_D_SEL_F(bgd)
		| WIN_FG_ALPHA_A_SEL_F(fga)
		| WIN_BG_ALPHA_A_SEL_F(bga));
	mask = (WIN_FG_ALPHA_D_SEL_MASK
		| WIN_BG_ALPHA_D_SEL_MASK
		| WIN_FG_ALPHA_A_SEL_MASK
		| WIN_BG_ALPHA_A_SEL_MASK);
	decon_write_mask(id, WIN_CONTROL_1(win_idx), val, mask);
}

static void decon_reg_set_win_func(u32 id, u32 win_idx, enum decon_win_func pd_func)
{
	u32 val, mask;

	val = WIN_FUNC_F(pd_func);
	mask = WIN_FUNC_MASK;
	decon_write_mask(id, WIN_CONTROL_0(win_idx), val, mask);
}

static void decon_reg_set_win_bnd_function(u32 id, u32 win_idx,
		struct decon_window_regs *regs)
{
	int plane_a = regs->plane_alpha;
	enum decon_blending blend = regs->blend;
	enum decon_win_func pd_func = PD_FUNC_USER_DEFINED;
	u8 alpha0 = 0xff;
	u8 alpha1 = 0xff;
	bool is_plane_a = false;
	u32 af_d = BND_COEF_ONE, ab_d = BND_COEF_ZERO,
		af_a = BND_COEF_ONE, ab_a = BND_COEF_ZERO;

	if (blend == DECON_BLENDING_NONE)
		pd_func = PD_FUNC_COPY;

	if ((plane_a >= 0) && (plane_a <= 0xff)) {
		alpha0 = plane_a;
		alpha1 = 0;
		is_plane_a = true;
	}

	if ((blend == DECON_BLENDING_COVERAGE) && !is_plane_a) {
		af_d = BND_COEF_AF;
		ab_d = BND_COEF_1_M_AF;
		af_a = BND_COEF_AF;
		ab_a = BND_COEF_1_M_AF;
	} else if ((blend == DECON_BLENDING_COVERAGE) && is_plane_a) {
		af_d = BND_COEF_ALPHA_MULT;
		ab_d = BND_COEF_1_M_ALPHA_MULT;
		af_a = BND_COEF_ALPHA_MULT;
		ab_a = BND_COEF_1_M_ALPHA_MULT;
	} else if ((blend == DECON_BLENDING_PREMULT) && !is_plane_a) {
		af_d = BND_COEF_ONE;
		ab_d = BND_COEF_1_M_AF;
		af_a = BND_COEF_ONE;
		ab_a = BND_COEF_1_M_AF;
	} else if ((blend == DECON_BLENDING_PREMULT) && is_plane_a) {
		af_d = BND_COEF_PLNAE_ALPHA0;
		ab_d = BND_COEF_1_M_ALPHA_MULT;
		af_a = BND_COEF_PLNAE_ALPHA0;
		ab_a = BND_COEF_1_M_ALPHA_MULT;
	} else if (blend == DECON_BLENDING_NONE) {
		decon_dbg("%s:%d none blending mode\n", __func__, __LINE__);
	} else {
		decon_warn("%s:%d undefined blending mode\n",
				__func__, __LINE__);
	}

	decon_reg_set_win_plane_alpha(id, win_idx, alpha0, alpha1);
	decon_reg_set_win_alpha_mult(id, win_idx, ALPHA_MULT_SRC_SEL_AF);
	decon_reg_set_win_func(id, win_idx, pd_func);
	if (pd_func == PD_FUNC_USER_DEFINED)
		decon_reg_set_win_sub_coeff(id,
				win_idx, af_d, ab_d, af_a, ab_a);
}

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0) && defined(CONFIG_EXYNOS_PLL_SLEEP)
void decon_reg_set_pll_sleep(u32 id, u32 en)
{
	u32 val, mask;

	if (id >= 2) {
		decon_info("%s:%d decon(%d) not allowed\n",
				__func__, __LINE__, id);
		return;
	}
	val = en ? ~0 : 0;
	mask = (id == 0) ? PLL_SLEEP_EN_OUTIF0_F : PLL_SLEEP_EN_OUTIF1_F;
	decon_write_mask(id, PLL_SLEEP_CONTROL, val, mask);
}

void decon_reg_set_pll_wakeup(u32 id, u32 en)
{
	u32 val, mask;

	if (id >= 2) {
		decon_info("%s:%d decon(%d) not allowed\n",
				__func__, __LINE__, id);
		return;
	}
	val = en ? ~0 : 0;
	mask = (id == 0) ? PLL_SLEEP_MASK_OUTIF0 : PLL_SLEEP_MASK_OUTIF1;
	decon_write_mask(id, PLL_SLEEP_CONTROL, val, mask);
}
#endif

/******************** EXPORTED DECON CAL APIs ********************/
/* TODO: maybe this function will be moved to internal DECON CAL function */
void decon_reg_update_req_global(u32 id)
{
	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0,
			SHADOW_REG_UPDATE_REQ_GLOBAL);
}

int decon_reg_init(u32 id, u32 dsi_idx, struct decon_param *p)
{
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;
	enum decon_scaler_path s_path = SCALERPATH_OFF;

	/*
	 * DECON does not need to start, if DECON is already
	 * running(enabled in LCD_ON_UBOOT)
	 */
	if (decon_reg_get_run_status(id)) {
		decon_info("decon_reg_init already called by BOOTLOADER\n");
		decon_reg_init_probe(id, dsi_idx, p);
		if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
			decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
		return -EBUSY;
	}

	dpu_reg_set_qactive_pll(id, true);

	decon_reg_set_clkgate_mode(id, 0);

	if (psr->out_type == DECON_OUT_DP)
		decon_reg_set_te_qactive_pll_mode(id, 1);

	if (id == 0)
		decon_reg_set_sram_share(id, DECON_FIFO_04K);
	else if (id == 2)
		decon_reg_set_sram_share(id, DECON_FIFO_12K);

	decon_reg_set_operation_mode(id, psr->psr_mode);

	decon_reg_set_blender_bg_image_size(id, psr->dsi_mode, lcd_info);

	decon_reg_set_scaled_image_size(id, psr->dsi_mode, lcd_info);

	if (id == 2) {
		/* Set a TRIG mode */
		/* This code is for only DECON 2 s/w trigger mode */
		decon_reg_configure_trigger(id, psr->trig_mode);
		decon_reg_configure_lcd(id, p);
	} else {
		decon_reg_configure_lcd(id, p);
		if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
			decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);
	}

	/* FIXME: DECON_T dedicated to PRE_WB */
	if (p->psr.out_type == DECON_OUT_WB)
		decon_reg_set_data_path(id, DPATH_WBPRE_ONLY, s_path);

	/* asserted interrupt should be cleared before initializing decon hw */
	decon_reg_clear_int_all(id);

	/* Configure DECON dsim connection  : 'data_path' setting is required */
	decon_reg_set_interface(id, psr);

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0) && defined(CONFIG_EXYNOS_PLL_SLEEP)
	/* TODO : register for outfifo2 doesn't exist, needs a confirm */
	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE &&
			psr->dsi_mode != DSI_MODE_DUAL_DSI)
		decon_reg_set_pll_sleep(id, 1);
#endif

	return 0;
}

int decon_reg_start(u32 id, struct decon_mode_info *psr)
{
	int ret = 0;

	if (psr->out_type == DECON_OUT_DP)
		displayport_reg_lh_p_ch_power(1);

	decon_reg_direct_on_off(id, 1);
	decon_reg_update_req_global(id);

	/*
	 * DECON goes to run-status as soon as
	 * request shadow update without HW_TE
	 */
	ret = decon_reg_wait_run_status_timeout(id, 20 * 1000);

	/* wait until run-status, then trigger */
	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
		decon_reg_set_trigger(id, psr, DECON_TRIG_ENABLE);
	return ret;
}

/*
 * stop sequence should be carefully for stability
 * try sequecne
 *	1. perframe off
 *	2. instant off
 */
int decon_reg_stop(u32 id, u32 dsi_idx, struct decon_mode_info *psr, bool rst,
		u32 fps)
{
	int ret = 0;

#if !defined(CONFIG_SOC_EXYNOS9820_EVT0) && defined(CONFIG_EXYNOS_PLL_SLEEP)
	/* when pll is asleep, need to wake it up before stopping */
	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE &&
			psr->dsi_mode != DSI_MODE_DUAL_DSI)
		decon_reg_set_pll_wakeup(id, 1);
#endif

	if (psr->out_type == DECON_OUT_DP)
		decon_reg_set_te_qactive_pll_mode(id, 0);

	/* call perframe stop */
	ret = decon_reg_stop_perframe(id, dsi_idx, psr, fps);
	if (ret < 0) {
		decon_err("%s, failed to perframe_stop\n", __func__);
		/* if fails, call decon instant off */
		ret = decon_reg_stop_inst(id, dsi_idx, psr, fps);
		if (ret < 0)
			decon_err("%s, failed to instant_stop\n", __func__);
	}

	/* assert reset when stopped normally or requested */
	if (!ret && rst)
		decon_reg_reset(id);

	decon_reg_clear_int_all(id);

	return ret;
}

void decon_reg_win_enable_and_update(u32 id, u32 win_idx, u32 en)
{
	decon_reg_set_win_enable(id, win_idx, en);
	decon_reg_update_req_window(id, win_idx);
}

void decon_reg_all_win_shadow_update_req(u32 id)
{
	u32 mask;

	mask = SHADOW_REG_UPDATE_REQ_FOR_DECON;

	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0, mask);
}

void decon_reg_set_window_control(u32 id, int win_idx,
		struct decon_window_regs *regs, u32 winmap_en)
{
	u32 win_en = regs->wincon & WIN_EN_F(win_idx) ? 1 : 0;

	if (win_en) {
		decon_dbg("%s: win id = %d\n", __func__, win_idx);
		decon_reg_set_win_bnd_function(0, win_idx, regs);
		decon_write(0, WIN_START_POSITION(win_idx), regs->start_pos);
		decon_write(0, WIN_END_POSITION(win_idx), regs->end_pos);
		decon_write(0, WIN_START_TIME_CONTROL(win_idx),
							regs->start_time);
		decon_reg_set_winmap(id, win_idx, regs->colormap, winmap_en);
	}

	decon_reg_config_win_channel(id, win_idx, regs->ch);
	decon_reg_set_win_enable(id, win_idx, win_en);

	decon_dbg("%s: regs->ch(%d)\n", __func__, regs->ch);
}

void decon_reg_update_req_window_mask(u32 id, u32 win_idx)
{
	u32 mask;

	mask = SHADOW_REG_UPDATE_REQ_FOR_DECON;
	mask &= ~(SHADOW_REG_UPDATE_REQ_WIN(win_idx));
	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0, mask);
}

void decon_reg_update_req_window(u32 id, u32 win_idx)
{
	u32 mask;

	mask = SHADOW_REG_UPDATE_REQ_WIN(win_idx);
	decon_write_mask(id, SHADOW_REG_UPDATE_REQ, ~0, mask);
}

void decon_reg_set_trigger(u32 id, struct decon_mode_info *psr,
		enum decon_set_trig en)
{
	u32 val, mask;

	if (psr->psr_mode == DECON_VIDEO_MODE)
		return;

	if (psr->trig_mode == DECON_SW_TRIG) {
		val = (en == DECON_TRIG_ENABLE) ? SW_TRIG_EN : 0;
		mask = HW_TRIG_EN | SW_TRIG_EN;
	} else { /* DECON_HW_TRIG */
		val = (en == DECON_TRIG_ENABLE) ?
				HW_TRIG_EN : HW_TRIG_MASK_DECON;
		mask = HW_TRIG_EN | HW_TRIG_MASK_DECON;
	}

	decon_write_mask(id, HW_SW_TRIG_CONTROL, val, mask);
}

void decon_reg_update_req_and_unmask(u32 id, struct decon_mode_info *psr)
{
	decon_reg_update_req_global(id);

	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
		decon_reg_set_trigger(id, psr, DECON_TRIG_ENABLE);
}

int decon_reg_wait_update_done_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 100;
	unsigned long cnt = timeout / delay_time;

	while (decon_read(id, SHADOW_REG_UPDATE_REQ) && --cnt)
		udelay(delay_time);

	if (!cnt) {
		decon_err("decon%d timeout of updating decon registers\n", id);
		return -EBUSY;
	}

	return 0;
}

int decon_reg_wait_update_done_and_mask(u32 id,
		struct decon_mode_info *psr, u32 timeout)
{
	int result;

	result = decon_reg_wait_update_done_timeout(id, timeout);

	if (psr->psr_mode == DECON_MIPI_COMMAND_MODE)
		decon_reg_set_trigger(id, psr, DECON_TRIG_DISABLE);

	return result;
}

int decon_reg_wait_idle_status_timeout(u32 id, unsigned long timeout)
{
	unsigned long delay_time = 10;
	unsigned long cnt = timeout / delay_time;
	u32 status;

	do {
		status = decon_reg_get_idle_status(id);
		cnt--;
		udelay(delay_time);
	} while (!status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon idle status(%u)\n",
								id, status);
		return -EBUSY;
	}

	return 0;
}

int decon_reg_wait_idle_status_framecnt(u32 id, unsigned int frame_cnt)
{
	unsigned int cnt = frame_cnt;
	u32 status = 0;

	do {
		status = decon_reg_get_idle_status(id);
		cnt--;
		usleep_range(16600, 16600);
	} while(!status && cnt);

	if (!cnt) {
		decon_err("decon%d wait timeout decon idle status(%u)\n", id, status);
		return -EBUSY;
	}
	return 0;
}

void decon_reg_set_partial_update(u32 id, enum decon_dsi_mode dsi_mode,
		struct decon_lcd *lcd_info, bool in_slice[],
		u32 partial_w, u32 partial_h)
{
	u32 dual_slice_en[2] = {1, 1};
	u32 slice_mode_ch[2] = {0, 0};

	/* Here, lcd_info contains the size to be updated */
	decon_reg_set_blender_bg_size(id, dsi_mode, partial_w, partial_h);

	if (lcd_info->dsc_enabled) {
		/* get correct DSC configuration */
		dsc_get_partial_update_info(lcd_info->dsc_slice_num,
				lcd_info->dsc_cnt, in_slice,
				dual_slice_en, slice_mode_ch);
		/* To support dual-display : DECON1 have to set DSC1 */
		dsc_reg_set_partial_update(id, dual_slice_en[0],
				slice_mode_ch[0], partial_h);
		if (lcd_info->dsc_cnt == 2)
			dsc_reg_set_partial_update(1, dual_slice_en[1],
					slice_mode_ch[1], partial_h);
	}

	decon_reg_set_data_path_size(id, partial_w, partial_h,
		lcd_info->dsc_enabled, lcd_info->dsc_cnt,
		lcd_info->dsc_enc_sw, lcd_info->dsc_slice_h, dual_slice_en);

}

void decon_reg_set_mres(u32 id, struct decon_param *p)
{
	struct decon_lcd *lcd_info = p->lcd_info;
	struct decon_mode_info *psr = &p->psr;

	if (lcd_info->mode != DECON_MIPI_COMMAND_MODE) {
		dsim_info("%s: mode[%d] doesn't support multi resolution\n",
				__func__, lcd_info->mode);
		return;
	}

	decon_reg_set_blender_bg_image_size(id, psr->dsi_mode, lcd_info);
	decon_reg_set_scaled_image_size(id, psr->dsi_mode, lcd_info);

	decon_reg_configure_lcd(id, p);
}

void decon_reg_release_resource(u32 id, struct decon_mode_info *psr)
{
	decon_reg_per_frame_off(id);
	decon_reg_update_req_global(id);
	decon_reg_set_trigger(id, psr, DECON_TRIG_ENABLE);
}

void decon_reg_config_wb_size(u32 id, struct decon_lcd *lcd_info,
		struct decon_param *param)
{
	decon_reg_set_blender_bg_image_size(id, DSI_MODE_SINGLE,
			lcd_info);
	decon_reg_config_data_path_size(id, lcd_info->xres,
			lcd_info->yres, 0, NULL, param);
}

void decon_reg_set_int(u32 id, struct decon_mode_info *psr, u32 en)
{
	u32 val, mask;

	decon_reg_clear_int_all(id);

	if (en) {
		val = (DPU_FRAME_DONE_INT_EN
			| DPU_FRAME_START_INT_EN
			| DPU_EXTRA_INT_EN
			| DPU_INT_EN);

		decon_write_mask(id, INTERRUPT_ENABLE,
				val, INTERRUPT_ENABLE_MASK);
		decon_dbg("decon %d, interrupt val = %x\n", id, val);

		val = (DPU_RESOURCE_CONFLICT_INT_EN
			| DPU_TIME_OUT_INT_EN);
		decon_write(id, EXTRA_INTERRUPT_ENABLE, val);
	} else {
		mask = (DPU_EXTRA_INT_EN | DPU_INT_EN);
		decon_write_mask(id, INTERRUPT_ENABLE, 0, mask);
	}
}

int decon_reg_get_interrupt_and_clear(u32 id, u32 *ext_irq)
{
	u32 val, val1;
	u32 reg_id;

	reg_id = INTERRUPT_PENDING;
	val = decon_read(id, reg_id);

	if (val & DPU_FRAME_START_INT_PEND)
		decon_write(id, reg_id, DPU_FRAME_START_INT_PEND);

	if (val & DPU_FRAME_DONE_INT_PEND)
		decon_write(id, reg_id, DPU_FRAME_DONE_INT_PEND);

	if (val & DPU_EXTRA_INT_PEND) {
		decon_write(id, reg_id, DPU_EXTRA_INT_PEND);

		reg_id = EXTRA_INTERRUPT_PENDING;
		val1 = decon_read(id, reg_id);
		*ext_irq = val1;

		if (val1 & DPU_RESOURCE_CONFLICT_INT_PEND) {
			decon_write(id, reg_id, DPU_RESOURCE_CONFLICT_INT_PEND);
			decon_warn("decon%d INFO0: SRAM_RSC & DSC = 0x%x\n",
				id,
				decon_read(id, RESOURCE_OCCUPANCY_INFO_0));
			decon_warn("decon%d INFO1: DMA_CH_RSC= 0x%x\n",
				id,
				decon_read(id, RESOURCE_OCCUPANCY_INFO_1));
			decon_warn("decon%d INFO2: WIN_RSC= 0x%x\n",
				id,
				decon_read(id, RESOURCE_OCCUPANCY_INFO_2));
		}

		if (val1 & DPU_TIME_OUT_INT_PEND)
			decon_write(id, reg_id, DPU_TIME_OUT_INT_PEND);
	}

	return val;
}

u32 decon_reg_get_cam_status(void __iomem *cam_status)
{
	if (cam_status)
		return readl(cam_status);
	else
		return 0xF;
}

void decon_reg_set_start_crc(u32 id, u32 en)
{
	decon_write_mask(id, CRC_CONTROL, en ? ~0 : 0, CRC_START);
}

/* bit_sel : 0=B, 1=G, 2=R */
void decon_reg_set_select_crc_bits(u32 id, u32 bit_sel)
{
	u32 val;

	val = CRC_COLOR_SEL(bit_sel);
	decon_write_mask(id, CRC_CONTROL, val, CRC_COLOR_SEL_MASK);
}

void decon_reg_get_crc_data(u32 id, u32 *w0_data, u32 *w1_data)
{
	u32 val;

	val = decon_read(id, CRC_DATA_0);
	*w0_data = CRC_DATA_DSIMIF0_GET(val);
	*w1_data = CRC_DATA_DSIMIF1_GET(val);
}

u32 DPU_DMA2CH(u32 dma)
{
	u32 ch_id;

	switch (dma) {
	case IDMA_GF0:
		ch_id = 0;
		break;
	case IDMA_GF1:
		ch_id = 2;
		break;
	case IDMA_VG:
		ch_id = 4;
		break;
	case IDMA_VGF:
		ch_id = 3;
		break;
	case IDMA_VGS:
		ch_id = 5;
		break;
	case IDMA_VGRFS:
		ch_id = 1;
		break;
	default:
		decon_dbg("channel(0x%x) is not valid\n", dma);
		return -1;
	}

	return ch_id;
}

u32 DPU_CH2DMA(u32 ch)
{
	u32 dma;

	switch (ch) {
	case 0:
		dma = IDMA_GF0;
		break;
	case 1:
		dma = IDMA_VGRFS;
		break;
	case 2:
		dma = IDMA_GF1;
		break;
	case 3:
		dma = IDMA_VGF;
		break;
	case 4:
		dma = IDMA_VG;
		break;
	case 5:
		dma = IDMA_VGS;
		break;
	default:
		decon_warn("channal(%d) is invalid\n", ch);
		return -1;
	}

	return dma;
}

int decon_check_supported_formats(enum decon_pixel_format format)
{
	switch (format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
	case DECON_PIXEL_FORMAT_ABGR_8888:
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
	case DECON_PIXEL_FORMAT_XRGB_8888:
	case DECON_PIXEL_FORMAT_XBGR_8888:
	case DECON_PIXEL_FORMAT_RGBX_8888:
	case DECON_PIXEL_FORMAT_BGRX_8888:
	case DECON_PIXEL_FORMAT_RGB_565:
	case DECON_PIXEL_FORMAT_BGR_565:
	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV12M:
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV21M:
	case DECON_PIXEL_FORMAT_NV12N:
	case DECON_PIXEL_FORMAT_NV12N_10B:

	case DECON_PIXEL_FORMAT_ARGB_2101010:
	case DECON_PIXEL_FORMAT_ABGR_2101010:
	case DECON_PIXEL_FORMAT_RGBA_1010102:
	case DECON_PIXEL_FORMAT_BGRA_1010102:

	case DECON_PIXEL_FORMAT_NV12M_P010:
	case DECON_PIXEL_FORMAT_NV21M_P010:
	case DECON_PIXEL_FORMAT_NV12M_S10B:
	case DECON_PIXEL_FORMAT_NV21M_S10B:
	case DECON_PIXEL_FORMAT_NV12_P010:

	case DECON_PIXEL_FORMAT_NV16:
	case DECON_PIXEL_FORMAT_NV61:
	case DECON_PIXEL_FORMAT_NV16M_P210:
	case DECON_PIXEL_FORMAT_NV61M_P210:
	case DECON_PIXEL_FORMAT_NV16M_S10B:
	case DECON_PIXEL_FORMAT_NV61M_S10B:
		return 0;
	default:
		break;
	}

	return -EINVAL;
}

/* base_regs means DECON0's SFR base address */
void __decon_dump(u32 id, void __iomem *regs, void __iomem *base_regs, bool dsc_en)
{
	decon_info("\n=== DECON%d SFR DUMP ===\n", id);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs, 0x620, false);

	decon_info("\n=== DECON%d SHADOW SFR DUMP ===\n", id);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			regs + SHADOW_OFFSET, 0x304, false);

	decon_info("\n=== DECON0 WINDOW SFR DUMP ===\n");
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			base_regs + 0x1000, 0x340, false);

	decon_info("\n=== DECON0 WINDOW SHADOW SFR DUMP ===\n");
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			base_regs + SHADOW_OFFSET + 0x1000, 0x220, false);

	if (dsc_en) {
		decon_info("\n=== DECON0 DSC0 SFR DUMP ===\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
				base_regs + 0x4000, 0x80, false);

		decon_info("\n=== DECON0 DSC1 SFR DUMP ===\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
				base_regs + 0x5000, 0x80, false);

		decon_info("\n=== DECON0 DSC0 SHADOW SFR DUMP ===\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
				base_regs + SHADOW_OFFSET + 0x4000, 0x80, false);

		decon_info("\n=== DECON0 DSC1 SHADOW SFR DUMP ===\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
				base_regs + SHADOW_OFFSET + 0x5000, 0x80, false);
	}
}

/* Makalu (9820) chip dependent HW limitation
 *	: returns 0 if no error
 *	: otherwise returns -EPERM for HW-wise not permitted
 */
int decon_check_global_limitation(struct decon_device *decon,
		struct decon_win_config *config)
{
	int ret = 0;
	int i, j;
	u32 bpp;
	/*
	 * AXI Port0 : CH0(GF0), CH1(VGRFS)
	 * AXI Port1 : CH2(GF1), CH3(VGF)
	 * AXI Port2 : CH4(VG), CH5(VGS)
	 */
	int axi_port[MAX_DECON_WIN] = {1, 0, 3, 2, 5, 4};

	for (i = 0; i < MAX_DECON_WIN; i++) {
		if (config[i].state != DECON_WIN_STATE_BUFFER)
			continue;

		if (config[i].idma_type < 0 ||
				config[i].idma_type >= decon->dt.dpp_cnt) {
			ret = -EINVAL;
			decon_err("invalid dpp channel(%d)\n",
					config[i].idma_type);
			goto err;
		}

		/* case 1 : In one axi domain, a channel has
		 *	compression & src.w over 2048
		 *	the one on the other should never have compression.
		 */
		if (config[i].compression && (config[i].src.w > 2048)) {
			for (j = 0; j < MAX_DECON_WIN; j++) {
				if (i == j || (config[i].idma_type >= ODMA_WB))
					continue;
				/* idma_type means DPP channel number */
				if ((config[j].state == DECON_WIN_STATE_BUFFER) &&
						(config[j].idma_type ==
						 axi_port[config[i].idma_type])) {
					if (config[j].compression) {
						ret = -EPERM;
						decon_err("When using both channel\
							as an AFBC width should\
							always be set\
							equal or under 2048\n");
						goto err;
					}
				}
			}
		/* case 2 : In an axi domain, a channel has rotation
		 *	one on the other should never have compression.
		 */
		} else if (config[i].dpp_parm.rot > DPP_ROT_180) {
			bpp = dpu_get_bpp(config[i].format);
			/* 10-bit YUV */
			if (bpp == 15 || bpp == 24) {
				decon_err("Limited 10-bit ROT!\n");
				ret = -EPERM;
				goto err;
			}
			/* 8-bit YUV */
			if ((config[i].src.w > ROT_MAX_W) &&
				(config[i].src.w * config[i].src.h > ROT_MAX_SZ)) {
				decon_err("Exceeded supporting ROT size!\n");
				ret = -EPERM;
				goto err;
			}

			for (j = 0; j < MAX_DECON_WIN; j++) {
				if (i == j || (config[i].idma_type >= ODMA_WB))
					continue;
				if ((config[j].state == DECON_WIN_STATE_BUFFER) &&
						(config[j].idma_type ==
						 axi_port[config[i].idma_type])) {
					if (config[j].compression) {
						ret = -EPERM;
						decon_err("AFBC & Roation/Flip not\
							allowed in the same AXI port\
							at the same time\n");
						goto err;
					}
				}
			}
		}
	}

err:
	return ret;
}
