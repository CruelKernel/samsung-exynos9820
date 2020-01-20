/*
 * Samsung Exynos STR device driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef CAMERAPP_STR_SFR_API_V100_H
#define CAMERAPP_STR_SFR_API_V100_H

#include <linux/types.h>

enum str_ip {
	PU_STRA,
	PU_STRB,
	PU_STRC,
	PU_STRD,
	CMDQ,
	DXI_MUX,
	SRAM_MUX,
};

enum str_mux_port {
	RDMA00 = 1,
	RDMA01,
	RDMA02,
	STRA_OUT0,
	STRC_OUT0,
	STRC_OUT1,
	STRC_OUT2,
	STRC_OUT3,
	STRD_OUT0, //9
	WDMA00,
	WDMA01,
	WDMA02,
	WDMA03,
	STRA_IN0,
	STRB_IN0,
	STRB_IN1,
	STRB_IN2,
	STRC_IN0,
	STRC_IN1,
	STRC_IN2,
	STRD_IN0,
	STRD_IN1,
	TDMA
};

enum str_irq_src_id {
	PU_TILE = 0,
	PU_FRAME,
	PU_ERROR,
	DMA_TILE,
	DMA_FRAME,
	DMA_ERROR,
	DXI_TILE,
	DXI_FRAME	//7
};

enum str_stra_ref_param_id {
	REF_PARAM_0,
	REF_PARAM_1,
	REF_PARAM_2,
	REF_PARAM_CNT
};

struct str_region {
	u32	left;
	u32	top;
	u32	right;
	u32	bottom;
};

struct str_pu_size_cfg {
	u32			width;
	u32			height;
	u32			tile_w;
	u32			tile_h;
	u8			tile_x_num;
	u8			tile_y_num;
	struct str_region	tot_ring;
	bool			tile_asym_mode;
	bool			tile_manual_mode;
	u8			m_tile_mode_x;
	u8			m_tile_mode_y;
	u32			m_tile_pos_x;
	u32			m_tile_pos_y;
};

struct str_dma_cfg {
	u32			pos_x;
	u32			pos_y;
	u32			width;
	u32			height;
	u32			tile_w;
	u32			tile_h;
	u8			tile_x_num;
	u8			tile_y_num;
	struct str_region	tot_ring;
	struct str_region	frame_ring; //boolean values
	bool			tile_asym_mode;
	bool			tile_rand_mode;
	bool			tile_manual_mode;
	u8			m_tile_mode_x;
	u8			m_tile_mode_y;
	u32			m_tile_pos_x;
	u32			m_tile_pos_y;
	dma_addr_t		addr;
	u32			stride;
	u8			line_skip;
	u8			line_valid;
	u8			repeat;
	u8			ppc;
	u8			bpp;
	u32			fifo_addr;
	u32			fifo_size;
};

struct stra_ref_params {
	u32 	mean[2];
	u32 	cov[3];
	u8 	weight;
};

struct stra_params {
	bool			rev_uv;
	u8			cam_type; /* 0: rear, 1: front */
	u8			gyro;
	u8			face_num;
	u8			ref_skin_num;
	struct stra_ref_params	ref_param[REF_PARAM_CNT];
	u32			width;
	u32			height;
	u32			face_th;
	u32			no_face_th;
	u32			skin_ratio;
	struct str_pu_size_cfg	size_cfg;
	struct str_dma_cfg	face_c;
	struct str_dma_cfg	dist_map;
};

enum str_strb_grp_param_id {
	GRP_PARAM_0,
	GRP_PARAM_1,
	GRP_PARAM_2,
	GRP_PARAM_CNT
};

struct strb_grp_params {
	u32	mean[3];
	u32	invcov[3][3];
	u8	pref_col[4];
	u32	del_y;
};

struct strb_params {
	bool			rev_uv;
	u8			cam_type; /* 0: rear, 1: front */
	u8			gyro;
	bool			use_reg;
	bool			is_preview;
	bool			res_delta;
	bool			is_second;
	u8			face_num;
	u32			cov_scale;
	u32			mu_ye;
	u32			mu_ys;
	u32			inv_mu_yse;
	u8			y_int;
	u8			mul_k;
	u8			grp_num;
	struct strb_grp_params	grp_param[GRP_PARAM_CNT];
	struct str_pu_size_cfg	size_cfg;
	struct str_dma_cfg	dist_map;
	struct str_dma_cfg	face_c;
	struct str_dma_cfg	face_y;
};

struct strc_params {
	bool			rev_uv;
	u8			cam_type; /* 0: rear, 1: front */
	u8			gyro;
	bool			use_reg;
	u8			face_num;
	u32			clip_ratio;
	u32			inv_clip_ratio;
	u32			mask_mul;
	u32			inv_x;
	u32			inv_y;
	struct str_region 	face;
	struct str_region	roi;
	struct str_pu_size_cfg	size_cfg;
	struct str_dma_cfg	roi_c_src;
	struct str_dma_cfg	roi_y_src_even;
	struct str_dma_cfg	roi_y_src_odd;
	struct str_dma_cfg	roi_mask_c;
	struct str_dma_cfg	roi_c_dst;
	struct str_dma_cfg	roi_y_dst_even;
	struct str_dma_cfg	roi_y_dst_odd;
};

struct strd_params {
	bool			enable_denoise;
	u8			kern_size;
	bool			no_center_pix;
	u32			muldiv;
	u32			non_edge;
	u32			denoise;
	u32			blur;
	u32			inv_mul;
	struct str_pu_size_cfg	size_cfg;
	struct str_dma_cfg	roi_mask_c;
	struct str_dma_cfg	roi_y_src;
	struct str_dma_cfg	roi_y_dst;
};

u32 str_sfr_get_reg_ver(void __iomem *base_addr);
u32 str_sfr_get_str_ver(void __iomem *base_addr);

void str_sfr_cmdq_set_mode(void __iomem *base_addr, bool enable);
void str_sfr_dxi_mux_sel(void __iomem *base_addr, enum str_ip ip);
void str_sfr_stra_set_params(void __iomem *base_addr, struct stra_params *param);
void str_sfr_strb_set_params(void __iomem *base_addr, struct strb_params *param);
void str_sfr_strc_set_params(void __iomem *base_addr, struct strc_params *param);
void str_sfr_strd_set_params(void __iomem *base_addr, struct strd_params *param);
void str_sfr_dma_set_cfg(void __iomem *base_addr, enum str_mux_port dma_ch, struct str_dma_cfg *dma_cfg);
void str_sfr_pu_set_cfg(void __iomem *base_addr, enum str_ip ip, struct str_pu_size_cfg *size_cfg);
void str_sfr_pu_start(void __iomem *base_addr, enum str_ip ip);
void str_sfr_pu_dma_start(void __iomem *base_addr, enum str_ip ip);
void str_sfr_cmdq_set_frm_marker(void __iomem *base_addr);
void str_sfr_strb_rerun(void __iomem *base_addr);
void str_sfr_cmdq_set_run_cnt(void __iomem *base_addr, u8 face_num);

void str_sfr_top_irq_enable(void __iomem *base_addr, u8 port_num, enum str_irq_src_id src);
void str_sfr_top_roi_evt_enable(void __iomem *base_addr, enum str_ip ip);
void str_sfr_top_pu_frame_done_intr_enable(void __iomem *base_addr, enum str_ip ip);
u32 str_sfr_top_get_frame_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_dma_err_enable(void __iomem *base_addr, enum str_mux_port dma_ch, u32 reg_val);
u32 str_sfr_dma_err_clear(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_dma_get_err_status(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_sw_all_reset(void __iomem *base_addr);

void str_sfr_top_pu_err_done_intr_enable(void __iomem *base_addr, enum str_ip ip);
u32 str_sfr_top_get_pu_err_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_top_dma_frame_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_top_get_dma_frame_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_top_dma_err_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_top_get_dma_err_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_top_pu_tile_done_intr_enable(void __iomem *base_addr, enum str_ip ip);
u32 str_sfr_top_get_pu_tile_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_top_dma_tile_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_top_get_dma_tile_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_top_dxi_tile_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_top_get_dxi_tile_done_intr_status_clear(void __iomem *base_addr);
void str_sfr_top_dxi_frame_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch);
u32 str_sfr_top_get_dxi_frame_done_intr_status_clear(void __iomem *base_addr);

/* DMA ERR Interrupt Enable Bit Configuration
 * [ 0] DMA OTF error
 * [ 1] FIFO Overflow
 * [ 2] FIFO Underflow
 * [ 3] SBI Error
 * [ 7] Base address & stride align error
 * [ 8] Format error
 * [ 9] YC420 Line Error
 * [12] DMA setting value change error
 * [13] DMA start error
 */
#define DMA_ERR_ENABLE		(0xFFFE)

#endif //CAMERAPP_STR_SFR_API_V100_H
