/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_API_MCSCALER_V1_H
#define FIMC_IS_HW_API_MCSCALER_V1_H

#include "fimc-is-hw-api-common.h"

#define RATIO_X8_8	1048576
#define RATIO_X7_8	1198373
#define RATIO_X6_8	1398101
#define RATIO_X5_8	1677722
#define RATIO_X4_8	2097152
#define RATIO_X3_8	2796203
#define RATIO_X2_8	4194304

#define MCSC_SETFILE_VERSION	0x14027431

void fimc_is_scaler_start(void __iomem *base_addr);
void fimc_is_scaler_stop(void __iomem *base_addr);

u32 fimc_is_scaler_sw_reset(void __iomem *base_addr);

void fimc_is_scaler_set_stop_req_post_en_ctrl(void __iomem *base_addr, u32 value);

void fimc_is_scaler_clear_intr_all(void __iomem *base_addr);
void fimc_is_scaler_disable_intr(void __iomem *base_addr);
void fimc_is_scaler_mask_intr(void __iomem *base_addr, u32 intr_mask);

void fimc_is_scaler_get_input_status(void __iomem *base_addr, u32 *hl, u32 *vl);
void fimc_is_scaler_set_poly_scaler_enable(void __iomem *base_addr, u32 enable);
void fimc_is_scaler_set_poly_img_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_get_poly_img_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scaler_set_poly_src_size(void __iomem *base_addr, u32 pos_x, u32 pos_y,
	u32 width, u32 height);
void fimc_is_scaler_get_poly_src_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scaler_set_poly_dst_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_get_poly_dst_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scaler_set_post_scaler_enable(void __iomem *base_addr, u32 enable);
void fimc_is_scaler_set_post_img_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_get_post_img_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scaler_set_post_dst_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_get_post_dst_size(void __iomem *base_addr, u32 *width, u32 *height);

void fimc_is_scaler_set_poly_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio);
void fimc_is_scaler_set_post_scaling_ratio(void __iomem *base_addr, u32 hratio, u32 vratio);
void fimc_is_scaler_set_poly_scaler_coef(void __iomem *base_addr, u32 hratio, u32 vratio);

void fimc_is_scaler_set_dma_size(void __iomem *base_addr, u32 width, u32 height);
void fimc_is_scaler_get_dma_size(void __iomem *base_addr, u32 *width, u32 *height);
void fimc_is_scaler_set_stride_size(void __iomem *base_addr, u32 y_stride, u32 uv_stride);
void fimc_is_scaler_get_stride_size(void __iomem *base_addr, u32 *y_stride, u32 *uv_stride);

void fimc_is_scaler_set_flip_mode(void __iomem *base_addr, u32 flip);
void fimc_is_scaler_set_swap_mode(void __iomem *base_addr, u32 swap);

void fimc_is_scaler_set_direct_out_path(void __iomem *base_addr, u32 direct_sel, bool direct_out_en);
void fimc_is_scaler_set_dma_out_path(void __iomem *base_addr, u32 dma_out_format, bool dma_out_en);
void fimc_is_scaler_set_dma_enable(void __iomem *base_addr, bool enable);
void fimc_is_scaler_set_dma_out_frame_seq(void __iomem *base_addr, u32 frame_seq);
void fimc_is_scaler_set_dma_out_addr(void __iomem *base_addr, u32 y_addr, u32 cb_addr,
	u32 cr_addr, int buf_index);
void fimc_is_scaler_clear_dma_out_addr(void __iomem *base_addr);

void fimc_is_scaler_set_420_conversion(void __iomem *base_addr, u32 conv420_weight, bool conv420_en);
void fimc_is_scaler_set_dma_out_img_fmt(void __iomem *base_addr, u32 dma_foramt);
int fimc_is_scaler_get_dma_out_img_fmt(void __iomem *base_addr);
void fimc_is_scaler_set_dither(void __iomem *base_addr, bool dither_en);

void fimc_is_scaler_set_bchs_enable(void __iomem *base_addr, bool bchs_en);
void fimc_is_scaler_set_b_c(void __iomem *base_addr, u32 y_offset, u32 y_gain);
void fimc_is_scaler_set_h_s(void __iomem *base_addr, u32 c_gain00, u32 c_gain01,
	u32 c_gain10, u32 c_gain11);

void fimc_is_scaler_clear_intr_src(void __iomem *base_addr, u32 status);

u32 fimc_is_scaler_get_intr_mask(void __iomem *base_addr);
u32 fimc_is_scaler_get_intr_status(void __iomem *base_addr);

u32 fimc_is_scaler_get_otf_out_enable(void __iomem *base_addr);
u32 fimc_is_scaler_get_dma_enable(void __iomem *base_addr);
u32 fimc_is_scaler_get_version(void __iomem *base_addr);
#endif
