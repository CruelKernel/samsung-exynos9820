/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos9110 DECON CAL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_DECON_CAL_H__
#define __SAMSUNG_DECON_CAL_H__

#include "../panels/decon_lcd.h"

#define CEIL(x)			((x-(u32)(x) > 0 ? (u32)(x+1) : (u32)(x)))

#define CHIP_VER		(9110)
#define MAX_DECON_CNT		1
#define MAX_DECON_WIN		3
#define MAX_DPP_SUBDEV		3

#define DSC0_OFFSET		0x4000
#define DSC1_OFFSET		0x5000
#define DPU_RESOURCE_CONFLICT_INT_PEND	0
#define DPU_RESOURCE_CONFLICT_INT_EN	0

enum decon_idma_type {
	IDMA_G0 = 0,
	IDMA_G1,
	IDMA_VG0,
	IDMA_VG1,
	IDMA_VGF0,
	IDMA_VGF1, /* VGRF in case of Exynos9810 */
	ODMA_WB,
	MAX_DECON_DMA_TYPE,
};

enum decon_fifo_mode {
	DECON_FIFO_00K = 0,
	DECON_FIFO_04K,
	DECON_FIFO_08K,
	DECON_FIFO_12K,
	DECON_FIFO_16K,
};

enum decon_dsi_mode {
	DSI_MODE_SINGLE = 0,
	DSI_MODE_DUAL_DSI,
	DSI_MODE_DUAL_DISPLAY,
	DSI_MODE_NONE
};

enum decon_data_path {
	/* No comp - OUTFIFO0 DSIM_IF0 */
	DPATH_NOCOMP_OUTFIFO0_DSIMIF0			= 0x001,
	/* No comp - FF0 - FORMATTER1 - DSIM_IF1 */
	DPATH_NOCOMP_OUTFIFO0_DSIMIF1			= 0x002,
	/* No comp - SPLITTER - FF0/1 - FORMATTER0/1 - DSIM_IF0/1 */
	DPATH_NOCOMP_SPLITTER_OUTFIFO01_DSIMIF01	= 0x003,

	/* DSC_ENC0 - OUTFIFO0 - DSIM_IF0 */
	DPATH_DSCENC0_OUTFIFO0_DSIMIF0		= 0x011,
	/* DSC_ENC0 - OUTFIFO0 - DSIM_IF1 */
	DPATH_DSCENC0_OUTFIFO0_DSIMIF1		= 0x012,

	/* DSCC,DSC_ENC0/1 - OUTFIFO01 DSIM_IF0 */
	DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF0	= 0x0B1,
	/* DSCC,DSC_ENC0/1 - OUTFIFO01 DSIM_IF1 */
	DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF1	= 0x0B2,
	/* DSCC,DSC_ENC0/1 - OUTFIFO01 DSIM_IF0/1*/
	DPATH_DSCC_DSCENC01_OUTFIFO01_DSIMIF01	= 0x0B3,

	/* WB_PRE */
	DPATH_WBPRE_ONLY			= 0x100,
};

enum decon_scaler_path {
	SCALERPATH_OFF	= 0x0,
	SCALERPATH_VGF	= 0x1,
	SCALERPATH_VGRF	= 0x2,
};

enum decon_path_cfg {
	PATH_CON_ID_DSIM_IF0 = 0,
	PATH_CON_ID_DSIM_IF1 = 1,
	PATH_CON_ID_DP = 3,
	PATH_CON_ID_DUAL_DSC = 4,
	PATH_CON_ID_DSCC_EN = 7,
};

enum decon_trig_mode {
	DECON_HW_TRIG = 0,
	DECON_SW_TRIG
};

enum decon_out_type {
	DECON_OUT_DSI = 0,
	DECON_OUT_EDP,
	DECON_OUT_DP,
	DECON_OUT_WB
};

enum decon_rgb_order {
	DECON_RGB = 0x0,
	DECON_GBR = 0x1,
	DECON_BRG = 0x2,
	DECON_BGR = 0x4,
	DECON_RBG = 0x5,
	DECON_GRB = 0x6,
};

enum decon_win_func {
	PD_FUNC_CLEAR			= 0x0,
	PD_FUNC_COPY			= 0x1,
	PD_FUNC_DESTINATION		= 0x2,
	PD_FUNC_SOURCE_OVER		= 0x3,
	PD_FUNC_DESTINATION_OVER	= 0x4,
	PD_FUNC_SOURCE_IN		= 0x5,
	PD_FUNC_DESTINATION_IN		= 0x6,
	PD_FUNC_SOURCE_OUT		= 0x7,
	PD_FUNC_DESTINATION_OUT		= 0x8,
	PD_FUNC_SOURCE_A_TOP		= 0x9,
	PD_FUNC_DESTINATION_A_TOP	= 0xa,
	PD_FUNC_XOR			= 0xb,
	PD_FUNC_PLUS			= 0xc,
	PD_FUNC_USER_DEFINED		= 0xd,
};

enum decon_blending {
	DECON_BLENDING_NONE = 0,
	DECON_BLENDING_PREMULT = 1,
	DECON_BLENDING_COVERAGE = 2,
	DECON_BLENDING_MAX = 3,
};

enum decon_set_trig {
	DECON_TRIG_DISABLE = 0,
	DECON_TRIG_ENABLE
};

enum decon_pixel_format {
	/* RGB 8bit display */
	/* 4byte */
	DECON_PIXEL_FORMAT_ARGB_8888 = 0,
	DECON_PIXEL_FORMAT_ABGR_8888,
	DECON_PIXEL_FORMAT_RGBA_8888,
	DECON_PIXEL_FORMAT_BGRA_8888,
	DECON_PIXEL_FORMAT_XRGB_8888,
	DECON_PIXEL_FORMAT_XBGR_8888,
	DECON_PIXEL_FORMAT_RGBX_8888,
	DECON_PIXEL_FORMAT_BGRX_8888,
	/* 2byte */
	DECON_PIXEL_FORMAT_RGBA_5551,
	DECON_PIXEL_FORMAT_BGRA_5551,
	DECON_PIXEL_FORMAT_ABGR_4444,
	DECON_PIXEL_FORMAT_RGBA_4444,
	DECON_PIXEL_FORMAT_BGRA_4444,
	DECON_PIXEL_FORMAT_RGB_565,
	DECON_PIXEL_FORMAT_BGR_565,

	/* RGB 10bit display */
	/* 4byte */
	DECON_PIXEL_FORMAT_ARGB_2101010,
	DECON_PIXEL_FORMAT_ABGR_2101010,
	DECON_PIXEL_FORMAT_RGBA_1010102,
	DECON_PIXEL_FORMAT_BGRA_1010102,

	/* YUV 8bit display */
	/* YUV422 2P */
	DECON_PIXEL_FORMAT_NV16,
	DECON_PIXEL_FORMAT_NV61,
	/* YUV422 3P */
	DECON_PIXEL_FORMAT_YVU422_3P,
	/* YUV420 2P */
	DECON_PIXEL_FORMAT_NV12,
	DECON_PIXEL_FORMAT_NV21,
	DECON_PIXEL_FORMAT_NV12M,
	DECON_PIXEL_FORMAT_NV21M,
	/* YUV420 3P */
	DECON_PIXEL_FORMAT_YUV420,
	DECON_PIXEL_FORMAT_YVU420,
	DECON_PIXEL_FORMAT_YUV420M,
	DECON_PIXEL_FORMAT_YVU420M,
	/* YUV - 2 planes but 1 buffer */
	DECON_PIXEL_FORMAT_NV12N,
	DECON_PIXEL_FORMAT_NV12N_10B,

	/* YUV 10bit display */
	/* YUV420 2P */
	DECON_PIXEL_FORMAT_NV12M_P010,
	DECON_PIXEL_FORMAT_NV21M_P010,

	/* YUV420(P8+2) 4P */
	DECON_PIXEL_FORMAT_NV12M_S10B,
	DECON_PIXEL_FORMAT_NV21M_S10B,

	/* YUV422 2P */
	DECON_PIXEL_FORMAT_NV16M_P210,
	DECON_PIXEL_FORMAT_NV61M_P210,

	/* YUV422(P8+2) 4P */
	DECON_PIXEL_FORMAT_NV16M_S10B,
	DECON_PIXEL_FORMAT_NV61M_S10B,

	DECON_PIXEL_FORMAT_MAX,
};

struct decon_mode_info {
	enum decon_psr_mode psr_mode;
	enum decon_trig_mode trig_mode;
	enum decon_out_type out_type;
	enum decon_dsi_mode dsi_mode;
};

struct decon_param {
	struct decon_mode_info psr;
	struct decon_lcd *lcd_info;
	u32 nr_windows;
	void __iomem *disp_ss_regs; /* TODO: remove ? */
};

struct decon_window_regs {
	u32 wincon;
	u32 start_pos;
	u32 end_pos;
	u32 colormap;
	u32 start_time;
	u32 pixel_count;
	u32 whole_w;
	u32 whole_h;
	u32 offset_x;
	u32 offset_y;
	u32 winmap_state;
	enum decon_idma_type type;
	int plane_alpha;
	enum decon_pixel_format format;
	enum decon_blending blend;
};

u32 DPU_DMA2CH(u32 dma);
u32 DPU_CH2DMA(u32 ch);
int decon_check_supported_formats(enum decon_pixel_format format);

/*************** DECON CAL APIs exposed to DECON driver ***************/
/* DECON control */
int decon_reg_init(u32 id, u32 dsi_idx, struct decon_param *p);
int decon_reg_start(u32 id, struct decon_mode_info *psr);
int decon_reg_stop(u32 id, u32 dsi_idx, struct decon_mode_info *psr, bool rst,
		u32 fps);

/* DECON window control */
void decon_reg_win_enable_and_update(u32 id, u32 win_idx, u32 en);
void decon_reg_set_window_control(u32 id, int win_idx,
		struct decon_window_regs *regs, u32 winmap_en);
void decon_reg_update_req_window_mask(u32 id, u32 win_idx);

/* DECON shadow update and trigger control */
void decon_reg_set_trigger(u32 id, struct decon_mode_info *psr,
		enum decon_set_trig en);
void decon_reg_update_req_and_unmask(u32 id, struct decon_mode_info *psr);
int decon_reg_wait_update_done_timeout(u32 id, unsigned long timeout);
int decon_reg_wait_update_done_and_mask(u32 id,
		struct decon_mode_info *psr, u32 timeout);

/* For window update and multi resolution feature */
int decon_reg_wait_idle_status_timeout(u32 id, unsigned long timeout);
void decon_reg_set_partial_update(u32 id, enum decon_dsi_mode dsi_mode,
		struct decon_lcd *lcd_info, bool in_slice[],
		u32 partial_w, u32 partial_h);
void decon_reg_set_mres(u32 id, struct decon_param *p);

/* For writeback configuration */
void decon_reg_release_resource(u32 id, struct decon_mode_info *psr);
void decon_reg_config_wb_size(u32 id, struct decon_lcd *lcd_info,
		struct decon_param *param);

/* DECON interrupt control */
void decon_reg_set_int(u32 id, struct decon_mode_info *psr, u32 en);
int decon_reg_get_interrupt_and_clear(u32 id, u32 *ext_irq);

void decon_reg_set_start_crc(u32 id, u32 en);
void decon_reg_set_select_crc_bits(u32 id, u32 bit_sel);
void decon_reg_get_crc_data(u32 id, u32 *w0_data, u32 *w1_data);

/* TODO: this will be removed later */
void decon_reg_update_req_global(u32 id);
/*********************************************************************/

#endif /* __SAMSUNG_DECON_CAL_H__ */
