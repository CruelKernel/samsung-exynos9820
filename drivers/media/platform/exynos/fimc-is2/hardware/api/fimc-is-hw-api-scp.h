/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_API_SCP_H
#define FIMC_IS_HW_API_SCP_H

#include "fimc-is-hw-api-common.h"
#include "../../interface/fimc-is-interface-ischain.h"

#define RATIO_X8_8	65536
#define RATIO_X7_8	74898
#define RATIO_X6_8	87381
#define RATIO_X5_8	104857
#define RATIO_X4_8	131072
#define RATIO_X3_8	174762
#define RATIO_X2_8	262144

#define SCP_SETFILE_VERSION 0x14027431

struct scp_v_coef {
	int v_coef_a[9];
	int v_coef_b[9];
	int v_coef_c[9];
	int v_coef_d[9];
};

struct scp_h_coef {
	int h_coef_a[9];
	int h_coef_b[9];
	int h_coef_c[9];
	int h_coef_d[9];
	int h_coef_e[9];
	int h_coef_f[9];
	int h_coef_g[9];
	int h_coef_h[9];
};

typedef struct {
	/* Brightness/Contrast control param */
	u32 y_offset;
	u32 y_gain;

	/* Hue/Saturation control param */
	u32 c_gain00;
	u32 c_gain01;
	u32 c_gain10;
	u32 c_gain11;
} scaler_setfile_contents;

struct hw_api_scaler_setfile {
	u32 setfile_version;

	/* contents for Full/Narrow mode
	 * 0 : SCALER_OUTPUT_YUV_RANGE_FULL
	 * 1 : SCALER_OUTPUT_YUV_RANGE_NARROW
	 */
	scaler_setfile_contents contents[2];
};

void fimc_is_scp_start(void __iomem *base_addr);
void fimc_is_scp_stop(void __iomem *base_addr);

u32 fimc_is_scp_sw_reset(void __iomem *base_addr);
void fimc_is_scp_set_trigger_mode(void __iomem *base_addr, bool isExtTrigger);

void fimc_is_scp_clear_intr_all(void __iomem *base_addr);
void fimc_is_scp_disable_intr(void __iomem *base_addr);
void fimc_is_scp_mask_intr(void __iomem *base_addr, u32 intr_mask);

void fimc_is_scp_enable_frame_start_trigger(void __iomem *base_addr, bool enable);

void fimc_is_scp_set_pre_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scp_get_pre_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scp_set_src_size(void __iomem *base_addr, u32 pos_x, u32 pos_y, u32 width, u32 height);
void fimc_is_scp_get_src_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scp_set_dst_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scp_get_dst_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scp_set_img_size(void __iomem *base_addr, u32 width, u32 height);

void fimc_is_scp_set_pre_scaling_ratio(void __iomem *base_addr, u32 sh_factor, u32 hratio, u32 vratio);
void fimc_is_scp_set_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio);
void fimc_is_scp_set_horizontal_coef(void __iomem *base_addr, u32 hratio, u32 vratio);
void fimc_is_scp_set_vertical_coef(void __iomem *base_addr, u32 hratio, u32 vratio);

void fimc_is_scp_set_y_h_coef(void __iomem *base_addr, struct scp_h_coef *y_h_coef);
void fimc_is_scp_set_cb_h_coef(void __iomem *base_addr, struct scp_h_coef *cb_h_coef);
void fimc_is_scp_set_cr_h_coef(void __iomem *base_addr, struct scp_h_coef *cr_h_coef);

void fimc_is_scp_set_y_v_coef(void __iomem *base_addr, struct scp_v_coef *y_v_coef);
void fimc_is_scp_set_cb_v_coef(void __iomem *base_addr, struct scp_v_coef *cb_v_coef);
void fimc_is_scp_set_cr_v_coef(void __iomem *base_addr, struct scp_v_coef *cr_v_coef);

void fimc_is_scp_set_canv_size(void __iomem *base_addr, u32 y_h_size, u32 y_v_size, u32 c_h_size, u32 c_v_size);
void fimc_is_scp_set_dma_offset(void __iomem *base_addr,
	u32 y_h_offset, u32 y_v_offset,
	u32 cb_h_offset, u32 cb_v_offset,
	u32 cr_h_offset, u32 cr_v_offset);

void fimc_is_scp_set_flip_mode(void __iomem *base_addr, u32 flip);
void fimc_is_scp_set_swap_mode(void __iomem *base_addr, u32 swap);

void fimc_is_scp_set_direct_out_path(void __iomem *base_addr, u32 direct_sel, bool direct_out_en);
void fimc_is_scp_set_dma_out_path(void __iomem *base_addr, bool dma_out_en, u32 dma_sel);
void fimc_is_scp_set_dma_enable(void __iomem *base_addr, bool enable);
void fimc_is_scp_set_dma_out_frame_seq(void __iomem *base_addr, u32 frame_seq);
void fimc_is_scp_set_dma_out_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr, u32 cr_addr, int buf_index);
void fimc_is_scp_clear_dma_out_addr(void __iomem *base_addr);

void fimc_is_scp_set_420_conversion(void __iomem *base_addr, u32 conv420_weight, bool conv420_en);
void fimc_is_scp_set_dma_out_img_fmt(void __iomem *base_addr, u32 order2p_Out, u32 order422_out, u32 cint_out);
int fimc_is_scp_get_dma_out_img_fmt(void __iomem *base_addr);
void fimc_is_scp_set_dither(void __iomem *base_addr, bool dither_en);

void fimc_is_scp_set_b_c(void __iomem *base_addr, u32 y_offset, u32 y_gain);
void fimc_is_scp_set_h_s(void __iomem *base_addr, u32 c_gain00, u32 c_gain01, u32 c_gain10, u32 c_gain11);

void fimc_is_scp_clear_intr_src(void __iomem *base_addr, u32 status);

u32 fimc_is_scp_get_intr_mask(void __iomem *base_addr);
u32 fimc_is_scp_get_intr_status(void __iomem *base_addr);

u32 fimc_is_scp_get_otf_out_enable(void __iomem *base_addr);
u32 fimc_is_scp_get_dma_enable(void __iomem *base_addr);
u32 fimc_is_scp_get_version(void __iomem *base_addr);

#endif
