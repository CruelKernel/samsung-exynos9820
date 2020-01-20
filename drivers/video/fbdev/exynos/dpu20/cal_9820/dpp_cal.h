/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos9820 DPP CAL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_DPP_CAL_H__
#define __SAMSUNG_DPP_CAL_H__

#include "../decon.h"

struct dpp_params_info {
	struct decon_frame src;
	struct decon_frame dst;
	struct decon_win_rect block;
	u32 rot;

	enum dpp_hdr_standard hdr;
	u32 min_luminance;
	u32 max_luminance;
	bool is_4p;
	u32 y_2b_strd;
	u32 c_2b_strd;

	bool is_comp;
	bool is_scale;
	bool is_block;
	enum decon_pixel_format format;
	dma_addr_t addr[MAX_PLANE_ADDR_CNT];
	enum dpp_csc_eq eq_mode;
	int h_ratio;
	int v_ratio;

	unsigned long rcv_num;
#ifdef CONFIG_EXYNOS_MCD_HDR
	u32 wcg_mode;
#endif
};

struct dpp_size_constraints {
	u32 src_mul_w;
	u32 src_mul_h;
	u32 src_w_min;
	u32 src_w_max;
	u32 src_h_min;
	u32 src_h_max;
	u32 img_mul_w;
	u32 img_mul_h;
	u32 img_w_min;
	u32 img_w_max;
	u32 img_h_min;
	u32 img_h_max;
	u32 blk_w_min;
	u32 blk_w_max;
	u32 blk_h_min;
	u32 blk_h_max;
	u32 blk_mul_w;
	u32 blk_mul_h;
	u32 src_mul_x;
	u32 src_mul_y;
	u32 sca_w_min;
	u32 sca_w_max;
	u32 sca_h_min;
	u32 sca_h_max;
	u32 sca_mul_w;
	u32 sca_mul_h;
	u32 dst_mul_w;
	u32 dst_mul_h;
	u32 dst_w_min;
	u32 dst_w_max;
	u32 dst_h_min;
	u32 dst_h_max;
	u32 dst_mul_x;
	u32 dst_mul_y;
};

struct dpp_img_format {
	u32 vgr;
	u32 normal;
	u32 rot;
	u32 scale;
	u32 format;
	u32 afbc_en;
	u32 yuv;
	u32 yuv422;
	u32 yuv420;
	u32 wb;
};

struct dpp_restriction;

/* DPP CAL APIs exposed to DPP driver */
void dpp_reg_init(u32 id, const unsigned long attr);
int dpp_reg_deinit(u32 id, bool reset, const unsigned long attr);
void dpp_reg_configure_params(u32 id, struct dpp_params_info *p,
		const unsigned long attr);
void dpp_constraints_params(struct dpp_size_constraints *vc,
		struct dpp_img_format *vi, struct dpp_restriction *res);

/* DPU_DMA, DPP DEBUG */
void __dpp_dump(u32 id, void __iomem *regs, void __iomem *dma_regs,
		unsigned long attr);

/* DPU_DMA and DPP interrupt handler */
u32 dpp_reg_get_irq_and_clear(u32 id);
u32 idma_reg_get_irq_and_clear(u32 id);
u32 odma_reg_get_irq_and_clear(u32 id);

#endif /* __SAMSUNG_DPP_CAL_H__ */
