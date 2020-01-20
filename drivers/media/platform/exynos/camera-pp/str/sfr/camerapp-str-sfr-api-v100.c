/*
 * Samsung Exynos STR device driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "camerapp-str-sfr-v100.h"
#include "camerapp-str-sfr-api-v100.h"

void _str_sfr_top_apb_sw_rst(void __iomem *base_addr, enum str_ip ip)
{
	u32 reg_val = 0;

	switch (ip) {
	case PU_STRA:
		reg_val = (1 << 0);
		break;
	case PU_STRB:
		reg_val = (1 << 1);
		break;
	case PU_STRC:
		reg_val = (1 << 2);
		break;
	case PU_STRD:
		reg_val = (1 << 3);
		break;
	case DXI_MUX:
		reg_val = (1 << 29);
		break;
	case SRAM_MUX:
		reg_val = (1 << 30);
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_PU_APB_SW_RST_0], reg_val);
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_PU_APB_SW_RST_0], 0);
}

void _str_sfr_mux_sel(void __iomem *base_addr, enum str_mux_port src, enum str_mux_port dst)
{
	enum str_mux_reg_name reg_id;
	enum str_mux_reg_field field_id;
	u32 reg_val;

	if (src < WDMA00 || dst > STRD_OUT0) {
		pr_err("[STR:HW][%s]invalid src(%d) dst(%d)\n", __func__, src, dst);
		return;
	}

	reg_id = STR_R_MUXSEL_WDMA00 + (src - WDMA00);
	field_id = STR_F_MUXSEL_WDMA00 + (src - WDMA00);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_mux_fields[field_id], dst);
	camerapp_sfr_set_reg(base_addr, &str_mux_regs[reg_id], reg_val);
}

void _str_sfr_dma_set_frame_region(void __iomem *base_addr, enum str_mux_port dma_ch,
		u32 pos_x, u32 pos_y, u32 width, u32 height)
{
	const struct camerapp_sfr_reg *pos_reg;
	const struct camerapp_sfr_reg *size_reg;
	const struct camerapp_sfr_field *pos_x_field;
	const struct camerapp_sfr_field *pos_y_field;
	const struct camerapp_sfr_field *width_field;
	const struct camerapp_sfr_field *height_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		pos_reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_FRAME_START];
		size_reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_START_X];
		pos_y_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_START_Y];
		width_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_SIZE_H];
		height_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_SIZE_V];
		break;
	case RDMA01:
		pos_reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_FRAME_START];
		size_reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_START_X];
		pos_y_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_START_Y];
		width_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_SIZE_H];
		height_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_SIZE_V];
		break;
	case RDMA02:
		pos_reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_FRAME_START];
		size_reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_START_X];
		pos_y_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_START_Y];
		width_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_SIZE_H];
		height_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_SIZE_V];
		break;
	case WDMA00:
		pos_reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_FRAME_START];
		size_reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH0_FRAME_START_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH0_FRAME_START_Y];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH0_FRAME_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH0_FRAME_SIZE_V];
		break;
	case WDMA01:
		pos_reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_FRAME_START];
		size_reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH1_FRAME_START_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH1_FRAME_START_Y];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH1_FRAME_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH1_FRAME_SIZE_V];
		break;
	case WDMA02:
		pos_reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_FRAME_START];
		size_reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH2_FRAME_START_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH2_FRAME_START_Y];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH2_FRAME_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH2_FRAME_SIZE_V];
		break;
	case WDMA03:
		pos_reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_FRAME_START];
		size_reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_FRAME_SIZE];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH3_FRAME_START_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH3_FRAME_START_Y];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH3_FRAME_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH3_FRAME_SIZE_V];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, pos_x_field, pos_x);
	reg_val = camerapp_sfr_set_field_value(reg_val, pos_y_field, pos_y);
	camerapp_sfr_set_reg(base_addr, pos_reg, reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, width_field, width);
	reg_val = camerapp_sfr_set_field_value(reg_val, height_field, height);
	camerapp_sfr_set_reg(base_addr, size_reg, reg_val);
}

void _str_sfr_dma_set_tile_size(void __iomem *base_addr, enum str_mux_port dma_ch, u32 width, u32 height)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *width_field;
	const struct camerapp_sfr_field *height_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_TILE_SIZE];
		width_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_SIZE_H];
		height_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_SIZE_V];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_TILE_SIZE];
		width_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_SIZE_H];
		height_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_SIZE_V];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_TILE_SIZE];
		width_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_SIZE_H];
		height_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_SIZE_V];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_TILE_SIZE];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_SIZE_V];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_TILE_SIZE];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_SIZE_V];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_TILE_SIZE];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_SIZE_V];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_TILE_SIZE];
		width_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_SIZE_H];
		height_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_SIZE_V];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, width_field, width);
	reg_val = camerapp_sfr_set_field_value(reg_val, height_field, height);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_tile_num(void __iomem *base_addr, enum str_mux_port dma_ch, u32 num_x, u32 num_y)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *num_x_field;
	const struct camerapp_sfr_field *num_y_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_TILE_NUM];
		num_x_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_NUM_H];
		num_y_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_NUM_V];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_TILE_NUM];
		num_x_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_NUM_H];
		num_y_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_NUM_V];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_TILE_NUM];
		num_x_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_NUM_H];
		num_y_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_NUM_V];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_TILE_NUM];
		num_x_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_NUM_H];
		num_y_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_NUM_V];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_TILE_NUM];
		num_x_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_NUM_H];
		num_y_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_NUM_V];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_TILE_NUM];
		num_x_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_NUM_H];
		num_y_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_NUM_V];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_TILE_NUM];
		num_x_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_NUM_H];
		num_y_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_NUM_V];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, num_x_field, num_x);
	reg_val = camerapp_sfr_set_field_value(reg_val, num_y_field, num_y);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_tot_ring(void __iomem *base_addr, enum str_mux_port dma_ch,
		u8 left, u8 top, u8 right, u8 bottom)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *left_field;
	const struct camerapp_sfr_field *top_field;
	const struct camerapp_sfr_field *right_field;
	const struct camerapp_sfr_field *bottom_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_TOT_RING];
		left_field = &str_rdma_fields[STR_F_RDMA0_CH0_TOT_RING_LEFT];
		top_field = &str_rdma_fields[STR_F_RDMA0_CH0_TOT_RING_TOP];
		right_field = &str_rdma_fields[STR_F_RDMA0_CH0_TOT_RING_RIGHT];
		bottom_field = &str_rdma_fields[STR_F_RDMA0_CH0_TOT_RING_BOTTOM];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_TOT_RING];
		left_field = &str_rdma_fields[STR_F_RDMA0_CH1_TOT_RING_LEFT];
		top_field = &str_rdma_fields[STR_F_RDMA0_CH1_TOT_RING_TOP];
		right_field = &str_rdma_fields[STR_F_RDMA0_CH1_TOT_RING_RIGHT];
		bottom_field = &str_rdma_fields[STR_F_RDMA0_CH1_TOT_RING_BOTTOM];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_TOT_RING];
		left_field = &str_rdma_fields[STR_F_RDMA0_CH2_TOT_RING_LEFT];
		top_field = &str_rdma_fields[STR_F_RDMA0_CH2_TOT_RING_TOP];
		right_field = &str_rdma_fields[STR_F_RDMA0_CH2_TOT_RING_RIGHT];
		bottom_field = &str_rdma_fields[STR_F_RDMA0_CH2_TOT_RING_BOTTOM];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_TOT_RING];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_BOTTOM];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_TOT_RING];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_BOTTOM];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_TOT_RING];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_BOTTOM];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_TOT_RING];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_BOTTOM];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, left_field, left);
	reg_val = camerapp_sfr_set_field_value(reg_val, top_field, top);
	reg_val = camerapp_sfr_set_field_value(reg_val, right_field, right);
	reg_val = camerapp_sfr_set_field_value(reg_val, bottom_field, bottom);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_tile_ctrl_mode(void __iomem *base_addr, enum str_mux_port dma_ch,
		bool left, bool top, bool right, bool bottom,
		u8 tile_h, u8 tile_v,
		bool asym, bool rand, bool manual)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *left_field;
	const struct camerapp_sfr_field *top_field;
	const struct camerapp_sfr_field *right_field;
	const struct camerapp_sfr_field *bottom_field;
	const struct camerapp_sfr_field *tile_h_field;
	const struct camerapp_sfr_field *tile_v_field;
	const struct camerapp_sfr_field *asym_field;
	const struct camerapp_sfr_field *rand_field;
	const struct camerapp_sfr_field *manual_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_MODE];
		left_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_RING_LEFT];
		top_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_RING_TOP];
		right_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_RING_RIGHT];
		bottom_field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_RING_BOTTOM];
		tile_h_field = &str_rdma_fields[STR_F_RDMA0_CH0_M_TILE_MODE_H];
		tile_v_field = &str_rdma_fields[STR_F_RDMA0_CH0_M_TILE_MODE_V];
		asym_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_ASYM_MODE];
		rand_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_RAND_MODE];
		manual_field = &str_rdma_fields[STR_F_RDMA0_CH0_TILE_MANUAL_MODE];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_MODE];
		left_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_RING_LEFT];
		top_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_RING_TOP];
		right_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_RING_RIGHT];
		bottom_field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_RING_BOTTOM];
		tile_h_field = &str_rdma_fields[STR_F_RDMA0_CH1_M_TILE_MODE_H];
		tile_v_field = &str_rdma_fields[STR_F_RDMA0_CH1_M_TILE_MODE_V];
		asym_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_ASYM_MODE];
		rand_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_RAND_MODE];
		manual_field = &str_rdma_fields[STR_F_RDMA0_CH1_TILE_MANUAL_MODE];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_MODE];
		left_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_RING_LEFT];
		top_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_RING_TOP];
		right_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_RING_RIGHT];
		bottom_field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_RING_BOTTOM];
		tile_h_field = &str_rdma_fields[STR_F_RDMA0_CH2_M_TILE_MODE_H];
		tile_v_field = &str_rdma_fields[STR_F_RDMA0_CH2_M_TILE_MODE_V];
		asym_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_ASYM_MODE];
		rand_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_RAND_MODE];
		manual_field = &str_rdma_fields[STR_F_RDMA0_CH2_TILE_MANUAL_MODE];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_MODE];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH0_TOT_RING_BOTTOM];
		tile_h_field = &str_wdma_fields[STR_F_WDMA0_CH0_M_TILE_MODE_H];
		tile_v_field = &str_wdma_fields[STR_F_WDMA0_CH0_M_TILE_MODE_V];
		asym_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_ASYM_MODE];
		rand_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_RAND_MODE];
		manual_field = &str_wdma_fields[STR_F_WDMA0_CH0_TILE_MANUAL_MODE];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_MODE];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH1_TOT_RING_BOTTOM];
		tile_h_field = &str_wdma_fields[STR_F_WDMA0_CH1_M_TILE_MODE_H];
		tile_v_field = &str_wdma_fields[STR_F_WDMA0_CH1_M_TILE_MODE_V];
		asym_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_ASYM_MODE];
		rand_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_RAND_MODE];
		manual_field = &str_wdma_fields[STR_F_WDMA0_CH1_TILE_MANUAL_MODE];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_MODE];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH2_TOT_RING_BOTTOM];
		tile_h_field = &str_wdma_fields[STR_F_WDMA0_CH2_M_TILE_MODE_H];
		tile_v_field = &str_wdma_fields[STR_F_WDMA0_CH2_M_TILE_MODE_V];
		asym_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_ASYM_MODE];
		rand_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_RAND_MODE];
		manual_field = &str_wdma_fields[STR_F_WDMA0_CH2_TILE_MANUAL_MODE];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_MODE];
		left_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_LEFT];
		top_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_TOP];
		right_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_RIGHT];
		bottom_field = &str_wdma_fields[STR_F_WDMA0_CH3_TOT_RING_BOTTOM];
		tile_h_field = &str_wdma_fields[STR_F_WDMA0_CH3_M_TILE_MODE_H];
		tile_v_field = &str_wdma_fields[STR_F_WDMA0_CH3_M_TILE_MODE_V];
		asym_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_ASYM_MODE];
		rand_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_RAND_MODE];
		manual_field = &str_wdma_fields[STR_F_WDMA0_CH3_TILE_MANUAL_MODE];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, left_field, left);
	reg_val = camerapp_sfr_set_field_value(reg_val, top_field, top);
	reg_val = camerapp_sfr_set_field_value(reg_val, right_field, right);
	reg_val = camerapp_sfr_set_field_value(reg_val, bottom_field, bottom);
	reg_val = camerapp_sfr_set_field_value(reg_val, tile_h_field, tile_h);
	reg_val = camerapp_sfr_set_field_value(reg_val, tile_v_field, tile_v);
	reg_val = camerapp_sfr_set_field_value(reg_val, asym_field, asym);
	reg_val = camerapp_sfr_set_field_value(reg_val, rand_field, rand);
	reg_val = camerapp_sfr_set_field_value(reg_val, manual_field, manual);

	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_tile_pos(void __iomem *base_addr, enum str_mux_port dma_ch, u32 pos_x, u32 pos_y)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *pos_x_field;
	const struct camerapp_sfr_field *pos_y_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_TILE_CTRL_TILE_POS];
		pos_x_field = &str_rdma_fields[STR_F_RDMA0_CH0_M_TILE_X];
		pos_y_field = &str_rdma_fields[STR_F_RDMA0_CH0_M_TILE_Y];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_TILE_CTRL_TILE_POS];
		pos_x_field = &str_rdma_fields[STR_F_RDMA0_CH1_M_TILE_X];
		pos_y_field = &str_rdma_fields[STR_F_RDMA0_CH1_M_TILE_Y];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_TILE_CTRL_TILE_POS];
		pos_x_field = &str_rdma_fields[STR_F_RDMA0_CH2_M_TILE_X];
		pos_y_field = &str_rdma_fields[STR_F_RDMA0_CH2_M_TILE_Y];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_TILE_CTRL_TILE_POS];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH0_M_TILE_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH0_M_TILE_Y];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_TILE_CTRL_TILE_POS];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH1_M_TILE_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH1_M_TILE_Y];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_TILE_CTRL_TILE_POS];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH2_M_TILE_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH2_M_TILE_Y];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_TILE_CTRL_TILE_POS];
		pos_x_field = &str_wdma_fields[STR_F_WDMA0_CH3_M_TILE_X];
		pos_y_field = &str_wdma_fields[STR_F_WDMA0_CH3_M_TILE_Y];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, pos_x_field, pos_x);
	reg_val = camerapp_sfr_set_field_value(reg_val, pos_y_field, pos_y);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_addr(void __iomem *base_addr, enum str_mux_port dma_ch, u32 addr)
{
	const struct camerapp_sfr_reg *reg;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_BASEADDR];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_BASEADDR];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_BASEADDR];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_BASEADDR];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_BASEADDR];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_BASEADDR];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_BASEADDR];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, reg, addr);
}

void _str_sfr_dma_set_stride(void __iomem *base_addr, enum str_mux_port dma_ch, u32 str, bool neg)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *str_field;
	const struct camerapp_sfr_field *neg_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_STRIDE];
		str_field = &str_rdma_fields[STR_F_RDMA0_CH0_STRIDE];
		neg_field = &str_rdma_fields[STR_F_RDMA0_CH0_STRIDE_NEG];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_STRIDE];
		str_field = &str_rdma_fields[STR_F_RDMA0_CH1_STRIDE];
		neg_field = &str_rdma_fields[STR_F_RDMA0_CH1_STRIDE_NEG];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_STRIDE];
		str_field = &str_rdma_fields[STR_F_RDMA0_CH2_STRIDE];
		neg_field = &str_rdma_fields[STR_F_RDMA0_CH2_STRIDE_NEG];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_STRIDE];
		str_field = &str_wdma_fields[STR_F_WDMA0_CH0_STRIDE];
		neg_field = &str_wdma_fields[STR_F_WDMA0_CH0_STRIDE_NEG];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_STRIDE];
		str_field = &str_wdma_fields[STR_F_WDMA0_CH1_STRIDE];
		neg_field = &str_wdma_fields[STR_F_WDMA0_CH1_STRIDE_NEG];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_STRIDE];
		str_field = &str_wdma_fields[STR_F_WDMA0_CH2_STRIDE];
		neg_field = &str_wdma_fields[STR_F_WDMA0_CH2_STRIDE_NEG];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_STRIDE];
		str_field = &str_wdma_fields[STR_F_WDMA0_CH3_STRIDE];
		neg_field = &str_wdma_fields[STR_F_WDMA0_CH3_STRIDE_NEG];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, str_field, str);
	reg_val = camerapp_sfr_set_field_value(reg_val, neg_field, neg);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_format(void __iomem *base_addr, enum str_mux_port dma_ch,
		u8 bpp, u8 ppc, bool repeat_x, bool repeat_y, u8 line_valid, u8 line_skip)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *bpp_field;
	const struct camerapp_sfr_field *ppc_field;
	const struct camerapp_sfr_field *rep_field;
	const struct camerapp_sfr_field *val_field;
	const struct camerapp_sfr_field *skip_field;
	u8 repeat = 0;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_FORMAT];
		bpp_field = &str_rdma_fields[STR_F_RDMA0_CH0_FMT_BPP];
		ppc_field = &str_rdma_fields[STR_F_RDMA0_CH0_FMT_PPC];
		rep_field = &str_rdma_fields[STR_F_RDMA0_CH0_FMT_REPEAT];
		val_field = &str_rdma_fields[STR_F_RDMA0_CH0_FMT_LINE_VALID];
		skip_field = &str_rdma_fields[STR_F_RDMA0_CH0_FMT_LINE_SKIP];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_FORMAT];
		bpp_field = &str_rdma_fields[STR_F_RDMA0_CH1_FMT_BPP];
		ppc_field = &str_rdma_fields[STR_F_RDMA0_CH1_FMT_PPC];
		rep_field = &str_rdma_fields[STR_F_RDMA0_CH1_FMT_REPEAT];
		val_field = &str_rdma_fields[STR_F_RDMA0_CH1_FMT_LINE_VALID];
		skip_field = &str_rdma_fields[STR_F_RDMA0_CH1_FMT_LINE_SKIP];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_FORMAT];
		bpp_field = &str_rdma_fields[STR_F_RDMA0_CH2_FMT_BPP];
		ppc_field = &str_rdma_fields[STR_F_RDMA0_CH2_FMT_PPC];
		rep_field = &str_rdma_fields[STR_F_RDMA0_CH2_FMT_REPEAT];
		val_field = &str_rdma_fields[STR_F_RDMA0_CH2_FMT_LINE_VALID];
		skip_field = &str_rdma_fields[STR_F_RDMA0_CH2_FMT_LINE_SKIP];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_FORMAT];
		bpp_field = &str_wdma_fields[STR_F_WDMA0_CH0_FMT_BPP];
		ppc_field = &str_wdma_fields[STR_F_WDMA0_CH0_FMT_PPC];
		rep_field = &str_wdma_fields[STR_F_WDMA0_CH0_FMT_REPEAT];
		val_field = &str_wdma_fields[STR_F_WDMA0_CH0_FMT_LINE_VALID];
		skip_field = &str_wdma_fields[STR_F_WDMA0_CH0_FMT_LINE_SKIP];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_FORMAT];
		bpp_field = &str_wdma_fields[STR_F_WDMA0_CH1_FMT_BPP];
		ppc_field = &str_wdma_fields[STR_F_WDMA0_CH1_FMT_PPC];
		rep_field = &str_wdma_fields[STR_F_WDMA0_CH1_FMT_REPEAT];
		val_field = &str_wdma_fields[STR_F_WDMA0_CH1_FMT_LINE_VALID];
		skip_field = &str_wdma_fields[STR_F_WDMA0_CH1_FMT_LINE_SKIP];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_FORMAT];
		bpp_field = &str_wdma_fields[STR_F_WDMA0_CH2_FMT_BPP];
		ppc_field = &str_wdma_fields[STR_F_WDMA0_CH2_FMT_PPC];
		rep_field = &str_wdma_fields[STR_F_WDMA0_CH2_FMT_REPEAT];
		val_field = &str_wdma_fields[STR_F_WDMA0_CH2_FMT_LINE_VALID];
		skip_field = &str_wdma_fields[STR_F_WDMA0_CH2_FMT_LINE_SKIP];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_FORMAT];
		bpp_field = &str_wdma_fields[STR_F_WDMA0_CH3_FMT_BPP];
		ppc_field = &str_wdma_fields[STR_F_WDMA0_CH3_FMT_PPC];
		rep_field = &str_wdma_fields[STR_F_WDMA0_CH3_FMT_REPEAT];
		val_field = &str_wdma_fields[STR_F_WDMA0_CH3_FMT_LINE_VALID];
		skip_field = &str_wdma_fields[STR_F_WDMA0_CH3_FMT_LINE_SKIP];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	repeat = repeat_y;
	repeat = (repeat << 1) | repeat_x;

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, bpp_field, bpp);
	reg_val = camerapp_sfr_set_field_value(reg_val, ppc_field, ppc);
	reg_val = camerapp_sfr_set_field_value(reg_val, rep_field, repeat);
	reg_val = camerapp_sfr_set_field_value(reg_val, val_field, line_valid);
	reg_val = camerapp_sfr_set_field_value(reg_val, skip_field, line_skip);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_dma_set_ctrl(void __iomem *base_addr, enum str_mux_port dma_ch,
		u32 fifo_start, u32 fifo_size, bool fake_run)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *start_field;
	const struct camerapp_sfr_field *size_field;
	const struct camerapp_sfr_field *fake_field;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_CONTROL];
		start_field = &str_rdma_fields[STR_F_RDMA0_CH0_CTRL_FIFO_ADDR];
		size_field = &str_rdma_fields[STR_F_RDMA0_CH0_CTRL_FIFO_SIZE];
		fake_field = &str_rdma_fields[STR_F_RDMA0_CH0_CTRL_FAKE_RUN];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_CONTROL];
		start_field = &str_rdma_fields[STR_F_RDMA0_CH1_CTRL_FIFO_ADDR];
		size_field = &str_rdma_fields[STR_F_RDMA0_CH1_CTRL_FIFO_SIZE];
		fake_field = &str_rdma_fields[STR_F_RDMA0_CH1_CTRL_FAKE_RUN];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_CONTROL];
		start_field = &str_rdma_fields[STR_F_RDMA0_CH2_CTRL_FIFO_ADDR];
		size_field = &str_rdma_fields[STR_F_RDMA0_CH2_CTRL_FIFO_SIZE];
		fake_field = &str_rdma_fields[STR_F_RDMA0_CH2_CTRL_FAKE_RUN];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_CONTROL];
		start_field = &str_wdma_fields[STR_F_WDMA0_CH0_CTRL_FIFO_ADDR];
		size_field = &str_wdma_fields[STR_F_WDMA0_CH0_CTRL_FIFO_SIZE];
		fake_field = &str_wdma_fields[STR_F_WDMA0_CH0_CTRL_FAKE_RUN];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_CONTROL];
		start_field = &str_wdma_fields[STR_F_WDMA0_CH1_CTRL_FIFO_ADDR];
		size_field = &str_wdma_fields[STR_F_WDMA0_CH1_CTRL_FIFO_SIZE];
		fake_field = &str_wdma_fields[STR_F_WDMA0_CH1_CTRL_FAKE_RUN];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_CONTROL];
		start_field = &str_wdma_fields[STR_F_WDMA0_CH2_CTRL_FIFO_ADDR];
		size_field = &str_wdma_fields[STR_F_WDMA0_CH2_CTRL_FIFO_SIZE];
		fake_field = &str_wdma_fields[STR_F_WDMA0_CH2_CTRL_FAKE_RUN];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_CONTROL];
		start_field = &str_wdma_fields[STR_F_WDMA0_CH3_CTRL_FIFO_ADDR];
		size_field = &str_wdma_fields[STR_F_WDMA0_CH3_CTRL_FIFO_SIZE];
		fake_field = &str_wdma_fields[STR_F_WDMA0_CH3_CTRL_FAKE_RUN];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, start_field, fifo_start);
	reg_val = camerapp_sfr_set_field_value(reg_val, size_field, fifo_size);
	reg_val = camerapp_sfr_set_field_value(reg_val, fake_field, fake_run);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

u32 _str_sfr_top_pu_tile_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_PU_TILE_0]);
}

u32 _str_sfr_top_pu_frame_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_PU_FRAME_0]);
}

u32 _str_sfr_top_pu_err_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_PU_ERROR_0]);
}

u32 _str_sfr_top_dma_tile_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_DMA_TILE_0]);
}

u32 _str_sfr_top_dma_frame_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_DMA_FRAME_0]);
}

u32 _str_sfr_top_dma_err_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_DMA_ERROR_0]);
}

u32 _str_sfr_top_dxi_tile_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_DXI_TILE_0]);
}

u32 _str_sfr_top_dxi_frame_done_intr_status(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_STATUS_DXI_FRAME_0]);
}

void _str_sfr_top_pu_tile_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_PU_TILE_0], reg_val);
}

void _str_sfr_top_pu_frame_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_PU_FRAME_0], reg_val);
}

void _str_sfr_top_pu_err_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_PU_ERROR_0], reg_val);
}

void _str_sfr_top_dma_tile_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_DMA_TILE_0], reg_val);
}

void _str_sfr_top_dma_frame_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_DMA_FRAME_0], reg_val);
}

void _str_sfr_top_dma_err_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_DMA_ERROR_0], reg_val);
}

void _str_sfr_top_dxi_tile_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_DXI_TILE_0], reg_val);
}

void _str_sfr_top_dxi_frame_done_intr_clear(void __iomem *base_addr, u32 reg_val)
{
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_CLEAR_DXI_FRAME_0], reg_val);
}

void _str_sfr_pu_set_frame_size(void __iomem *base_addr, enum str_ip ip, u32 width, u32 height)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *width_field;
	const struct camerapp_sfr_field *height_field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_TILE_CTRL_FRAME_SIZE];
		width_field = &str_stra_fields[STR_F_STRA_FRAME_SIZE_H];
		height_field = &str_stra_fields[STR_F_STRA_FRAME_SIZE_V];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_TILE_CTRL_FRAME_SIZE];
		width_field = &str_strb_fields[STR_F_STRB_FRAME_SIZE_H];
		height_field = &str_strb_fields[STR_F_STRB_FRAME_SIZE_V];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_TILE_CTRL_FRAME_SIZE];
		width_field = &str_strc_fields[STR_F_STRC_FRAME_SIZE_H];
		height_field = &str_strc_fields[STR_F_STRC_FRAME_SIZE_V];
		break;
	case PU_STRD:
		reg = &str_strd_regs[STR_R_STRD_TILE_CTRL_FRAME_SIZE];
		width_field = &str_strd_fields[STR_F_STRD_FRAME_SIZE_H];
		height_field = &str_strd_fields[STR_F_STRD_FRAME_SIZE_V];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, width_field, width);
	reg_val = camerapp_sfr_set_field_value(reg_val, height_field, height);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_pu_set_tile_size(void __iomem *base_addr, enum str_ip ip, u32 width, u32 height)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *width_field;
	const struct camerapp_sfr_field *height_field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_TILE_CTRL_TILE_SIZE];
		width_field = &str_stra_fields[STR_F_STRA_TILE_SIZE_H];
		height_field = &str_stra_fields[STR_F_STRA_TILE_SIZE_V];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_TILE_CTRL_TILE_SIZE];
		width_field = &str_strb_fields[STR_F_STRB_TILE_SIZE_H];
		height_field = &str_strb_fields[STR_F_STRB_TILE_SIZE_V];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_TILE_CTRL_TILE_SIZE];
		width_field = &str_strc_fields[STR_F_STRC_TILE_SIZE_H];
		height_field = &str_strc_fields[STR_F_STRC_TILE_SIZE_V];
		break;
	case PU_STRD:
		reg = &str_strd_regs[STR_R_STRD_TILE_CTRL_TILE_SIZE];
		width_field = &str_strd_fields[STR_F_STRD_TILE_SIZE_H];
		height_field = &str_strd_fields[STR_F_STRD_TILE_SIZE_V];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, width_field, width);
	reg_val = camerapp_sfr_set_field_value(reg_val, height_field, height);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_pu_set_tile_num(void __iomem *base_addr, enum str_ip ip, u8 num_x, u8 num_y)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *x_field;
	const struct camerapp_sfr_field *y_field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_TILE_CTRL_TILE_NUM];
		x_field = &str_stra_fields[STR_F_STRA_TILE_NUM_H];
		y_field = &str_stra_fields[STR_F_STRA_TILE_NUM_V];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_TILE_CTRL_TILE_NUM];
		x_field = &str_strb_fields[STR_F_STRB_TILE_NUM_H];
		y_field = &str_strb_fields[STR_F_STRB_TILE_NUM_V];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_TILE_CTRL_TILE_NUM];
		x_field = &str_strc_fields[STR_F_STRC_TILE_NUM_H];
		y_field = &str_strc_fields[STR_F_STRC_TILE_NUM_V];
		break;
	case PU_STRD:
		reg = &str_strd_regs[STR_R_STRD_TILE_CTRL_TILE_NUM];
		x_field = &str_strd_fields[STR_F_STRD_TILE_NUM_H];
		y_field = &str_strd_fields[STR_F_STRD_TILE_NUM_V];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, x_field, num_x);
	reg_val = camerapp_sfr_set_field_value(reg_val, y_field, num_y);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_pu_set_tot_ring(void __iomem *base_addr, enum str_ip ip, u8 left, u8 top, u8 right, u8 bottom)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *left_field;
	const struct camerapp_sfr_field *top_field;
	const struct camerapp_sfr_field *right_field;
	const struct camerapp_sfr_field *bottom_field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_TILE_CTRL_TOT_RING];
		left_field = &str_stra_fields[STR_F_STRA_TOT_RING_LEFT];
		top_field = &str_stra_fields[STR_F_STRA_TOT_RING_TOP];
		right_field = &str_stra_fields[STR_F_STRA_TOT_RING_RIGHT];
		bottom_field = &str_stra_fields[STR_F_STRA_TOT_RING_BOTTOM];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_TILE_CTRL_TOT_RING];
		left_field = &str_strb_fields[STR_F_STRB_TOT_RING_LEFT];
		top_field = &str_strb_fields[STR_F_STRB_TOT_RING_TOP];
		right_field = &str_strb_fields[STR_F_STRB_TOT_RING_RIGHT];
		bottom_field = &str_strb_fields[STR_F_STRB_TOT_RING_BOTTOM];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_TILE_CTRL_TOT_RING];
		left_field = &str_strc_fields[STR_F_STRC_TOT_RING_LEFT];
		top_field = &str_strc_fields[STR_F_STRC_TOT_RING_TOP];
		right_field = &str_strc_fields[STR_F_STRC_TOT_RING_RIGHT];
		bottom_field = &str_strc_fields[STR_F_STRC_TOT_RING_BOTTOM];
		break;
	case PU_STRD:
		reg = &str_strd_regs[STR_R_STRD_TILE_CTRL_TOT_RING];
		left_field = &str_strd_fields[STR_F_STRD_TOT_RING_LEFT];
		top_field = &str_strd_fields[STR_F_STRD_TOT_RING_TOP];
		right_field = &str_strd_fields[STR_F_STRD_TOT_RING_RIGHT];
		bottom_field = &str_strd_fields[STR_F_STRD_TOT_RING_BOTTOM];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, left_field, left);
	reg_val = camerapp_sfr_set_field_value(reg_val, top_field, top);
	reg_val = camerapp_sfr_set_field_value(reg_val, right_field, right);
	reg_val = camerapp_sfr_set_field_value(reg_val, bottom_field, bottom);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_pu_set_tile_ctrl_mode(void __iomem *base_addr, enum str_ip ip,
		u8 tile_h, u8 tile_v, bool asym, bool manual)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *tile_h_field;
	const struct camerapp_sfr_field *tile_v_field;
	const struct camerapp_sfr_field *asym_field;
	const struct camerapp_sfr_field *manual_field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_TILE_CTRL_MODE];
		tile_h_field = &str_stra_fields[STR_F_STRA_M_TILE_MODE_H];
		tile_v_field = &str_stra_fields[STR_F_STRA_M_TILE_MODE_V];
		asym_field = &str_stra_fields[STR_F_STRA_TILE_ASYM_MODE];
		manual_field = &str_stra_fields[STR_F_STRA_TILE_MANUAL_MODE];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_TILE_CTRL_MODE];
		tile_h_field = &str_strb_fields[STR_F_STRB_M_TILE_MODE_H];
		tile_v_field = &str_strb_fields[STR_F_STRB_M_TILE_MODE_V];
		asym_field = &str_strb_fields[STR_F_STRB_TILE_ASYM_MODE];
		manual_field = &str_strb_fields[STR_F_STRB_TILE_MANUAL_MODE];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_TILE_CTRL_MODE];
		tile_h_field = &str_strc_fields[STR_F_STRC_M_TILE_MODE_H];
		tile_v_field = &str_strc_fields[STR_F_STRC_M_TILE_MODE_V];
		asym_field = &str_strc_fields[STR_F_STRC_TILE_ASYM_MODE];
		manual_field = &str_strc_fields[STR_F_STRC_TILE_MANUAL_MODE];
		break;
	case PU_STRD:
		reg = &str_strd_regs[STR_R_STRD_TILE_CTRL_MODE];
		tile_h_field = &str_strd_fields[STR_F_STRD_M_TILE_MODE_H];
		tile_v_field = &str_strd_fields[STR_F_STRD_M_TILE_MODE_V];
		asym_field = &str_strd_fields[STR_F_STRD_TILE_ASYM_MODE];
		manual_field = &str_strd_fields[STR_F_STRD_TILE_MANUAL_MODE];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, tile_h_field, tile_h);
	reg_val = camerapp_sfr_set_field_value(reg_val, tile_v_field, tile_v);
	reg_val = camerapp_sfr_set_field_value(reg_val, asym_field, asym);
	reg_val = camerapp_sfr_set_field_value(reg_val, manual_field, manual);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_pu_set_tile_pos(void __iomem *base_addr, enum str_ip ip, u32 pos_x, u32 pos_y)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *x_field;
	const struct camerapp_sfr_field *y_field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_TILE_CTRL_TILE_POS];
		x_field = &str_stra_fields[STR_F_STRA_M_TILE_X];
		y_field = &str_stra_fields[STR_F_STRA_M_TILE_Y];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_TILE_CTRL_TILE_POS];
		x_field = &str_strb_fields[STR_F_STRB_M_TILE_X];
		y_field = &str_strb_fields[STR_F_STRB_M_TILE_Y];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_TILE_CTRL_TILE_POS];
		x_field = &str_strc_fields[STR_F_STRC_M_TILE_X];
		y_field = &str_strc_fields[STR_F_STRC_M_TILE_Y];
		break;
	case PU_STRD:
		reg = &str_strd_regs[STR_R_STRD_TILE_CTRL_TILE_POS];
		x_field = &str_strd_fields[STR_F_STRD_M_TILE_X];
		y_field = &str_strd_fields[STR_F_STRD_M_TILE_Y];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, x_field, pos_x);
	reg_val = camerapp_sfr_set_field_value(reg_val, y_field, pos_y);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_pu_set_face_num(void __iomem *base_addr, enum str_ip ip, u8 num)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_FACE_NUMBER];
		field = &str_stra_fields[STR_F_STRA_FACE_NUMBER];
		break;
	case PU_STRB:
		reg = &str_strb_regs[STR_R_STRB_FACE_NUMBER];
		field = &str_strb_fields[STR_F_STRB_FACE_NUMBER];
		break;
	case PU_STRC:
		reg = &str_strc_regs[STR_R_STRC_FACE_NUMBER];
		field = &str_strc_fields[STR_F_STRC_FACE_NUMBER];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, field, num);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_stra_set_ctrl(void __iomem *base_addr, bool rev_uv, u8 cam_type, u8 gyro)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_REVERSE_UV], rev_uv);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_CAM_TYPE], cam_type);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_GYRO], gyro);

	camerapp_sfr_set_reg(base_addr, &str_stra_regs[STR_R_STRA_CTRL], reg_val);
}

void _str_sfr_stra_set_ref_skin_num(void __iomem *base_addr, u8 num)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_NUM_REF_SKIN], num);
	camerapp_sfr_set_reg(base_addr, &str_stra_regs[STR_R_STRA_NUM_REF_SKIN], reg_val);
}

void _str_sfr_stra_set_ref_param_mean(void __iomem *base_addr, enum str_stra_ref_param_id id, u32 mean0, u32 mean1)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *mean0_field;
	const struct camerapp_sfr_field *mean1_field;
	u32 reg_val;

	switch (id) {
	case REF_PARAM_0:
		reg = &str_stra_regs[STR_R_STRA_RPAR0_MU];
		mean0_field = &str_stra_fields[STR_F_STRA_RPAR0_MU0];
		mean1_field = &str_stra_fields[STR_F_STRA_RPAR0_MU1];
		break;
	case REF_PARAM_1:
		reg = &str_stra_regs[STR_R_STRA_RPAR1_MU];
		mean0_field = &str_stra_fields[STR_F_STRA_RPAR1_MU0];
		mean1_field = &str_stra_fields[STR_F_STRA_RPAR1_MU1];
		break;
	case REF_PARAM_2:
		reg = &str_stra_regs[STR_R_STRA_RPAR2_MU];
		mean0_field = &str_stra_fields[STR_F_STRA_RPAR2_MU0];
		mean1_field = &str_stra_fields[STR_F_STRA_RPAR2_MU1];
		break;
	default:
		pr_err("[STR:HW][%s]invalid param id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, mean0_field, mean0);
	reg_val = camerapp_sfr_set_field_value(reg_val, mean1_field, mean1);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_stra_set_ref_param_inv_cov(void __iomem *base_addr, enum str_stra_ref_param_id id,
		u32 cov00, u32 cov01, u32 cov11)
{
	const struct camerapp_sfr_reg *cov0_reg, *cov1_reg;
	const struct camerapp_sfr_field *cov00_field;
	const struct camerapp_sfr_field *cov01_field;
	const struct camerapp_sfr_field *cov11_field;
	u32 reg_val;

	switch (id) {
	case REF_PARAM_0:
		cov0_reg = &str_stra_regs[STR_R_STRA_RPAR0_INV_COV0];
		cov1_reg = &str_stra_regs[STR_R_STRA_RPAR0_INV_COV1];
		cov00_field = &str_stra_fields[STR_F_STRA_RPAR0_INV_COV00];
		cov01_field = &str_stra_fields[STR_F_STRA_RPAR0_INV_COV01];
		cov11_field = &str_stra_fields[STR_F_STRA_RPAR0_INV_COV11];
		break;
	case REF_PARAM_1:
		cov0_reg = &str_stra_regs[STR_R_STRA_RPAR1_INV_COV0];
		cov1_reg = &str_stra_regs[STR_R_STRA_RPAR1_INV_COV1];
		cov00_field = &str_stra_fields[STR_F_STRA_RPAR1_INV_COV00];
		cov01_field = &str_stra_fields[STR_F_STRA_RPAR1_INV_COV01];
		cov11_field = &str_stra_fields[STR_F_STRA_RPAR1_INV_COV11];
		break;
	case REF_PARAM_2:
		cov0_reg = &str_stra_regs[STR_R_STRA_RPAR2_INV_COV0];
		cov1_reg = &str_stra_regs[STR_R_STRA_RPAR2_INV_COV1];
		cov00_field = &str_stra_fields[STR_F_STRA_RPAR2_INV_COV00];
		cov01_field = &str_stra_fields[STR_F_STRA_RPAR2_INV_COV01];
		cov11_field = &str_stra_fields[STR_F_STRA_RPAR2_INV_COV11];
		break;
	default:
		pr_err("[STR:HW][%s]invalid param id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov00_field, cov00);
	reg_val = camerapp_sfr_set_field_value(reg_val, cov01_field, cov01);
	camerapp_sfr_set_reg(base_addr, cov0_reg, reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov11_field, cov11);
	camerapp_sfr_set_reg(base_addr, cov1_reg, reg_val);
}

void _str_sfr_stra_set_ref_param_weight(void __iomem *base_addr, enum str_stra_ref_param_id id, u8 weight)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *weight_field;
	u32 reg_val;

	switch (id) {
	case REF_PARAM_0:
		reg = &str_stra_regs[STR_R_STRA_RPAR0_WEIGHT];
		weight_field = &str_stra_fields[STR_F_STRA_RPAR0_WEIGHT];
		break;
	case REF_PARAM_1:
		reg = &str_stra_regs[STR_R_STRA_RPAR1_WEIGHT];
		weight_field = &str_stra_fields[STR_F_STRA_RPAR1_WEIGHT];
		break;
	case REF_PARAM_2:
		reg = &str_stra_regs[STR_R_STRA_RPAR2_WEIGHT];
		weight_field = &str_stra_fields[STR_F_STRA_RPAR2_WEIGHT];
		break;
	default:
		pr_err("[STR:HW][%s]invalid param id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, weight_field, weight);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_stra_set_image_size(void __iomem *base_addr, u32 width, u32 height)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_IMAGE_SIZE_H], width);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_IMAGE_SIZE_V], height);
	camerapp_sfr_set_reg(base_addr, &str_stra_regs[STR_R_STRA_IMAGE_SIZE], reg_val);
}

void _str_sfr_stra_set_dist_th(void __iomem *base_addr, u32 face_th, u32 no_face_th)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_FACE_TH], face_th);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_NO_FACE_TH], no_face_th);
	camerapp_sfr_set_reg(base_addr, &str_stra_regs[STR_R_STRA_DIST_THRESHOLD], reg_val);
}

void _str_sfr_stra_set_skin_ratio(void __iomem *base_addr, u32 ratio)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_stra_fields[STR_F_STRA_SKIN_RATIO], ratio);
	camerapp_sfr_set_reg(base_addr, &str_stra_regs[STR_R_STRA_SKIN_RATIO], reg_val);
}

void _str_sfr_strb_set_ctrl(void __iomem *base_addr, bool rev_uv, u8 cam_type, u8 gyro, bool use_reg,
		bool is_preview, bool res_delta, bool is_second)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_REVERSE_UV], rev_uv);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_CAM_TYPE], cam_type);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_GYRO], gyro);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_PARAM_IN_REG], use_reg);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_PREVIEW], is_preview);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_RES_DELARR], res_delta);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_SECOND], is_second);

	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_CTRL], reg_val);
}

void _str_sfr_strb_set_cov_scale(void __iomem *base_addr, u32 cov_scale)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_COVSCALE], cov_scale);
	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_COVSCALE], reg_val);
}

void _str_sfr_strb_set_in_params(void __iomem *base_addr, u32 mu_ye, u32 mu_ys,
		u32 inv_mu_yse, u8 y_int, u8 mul_k)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_MU_YE], mu_ye);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_MU_YS], mu_ys);
	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_PARAM0], reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_INV_MU_YSE], inv_mu_yse);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_YINTERP_PARAM], y_int);
	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_PARAM1], reg_val);

	reg_val = 0;
	camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_MUL_K], mul_k);
	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_PARAM2], reg_val);
}

void _str_sfr_strb_set_grp_num(void __iomem *base_addr, u8 num)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_NUM_GROUP], num);
	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_NUM_GROUP], reg_val);
}

void _str_sfr_strb_set_grp_param_mean(void __iomem *base_addr, enum str_strb_grp_param_id id,
		u32 mu0, u32 mu1, u32 mu2)
{
	const struct camerapp_sfr_reg *muc_reg, *muy_reg;
	const struct camerapp_sfr_field *mu0_field;
	const struct camerapp_sfr_field *mu1_field;
	const struct camerapp_sfr_field *mu2_field;
	u32 reg_val;

	switch (id) {
	case GRP_PARAM_0:
		muc_reg = &str_strb_regs[STR_R_STRB_GPAR0_MUC];
		muy_reg = &str_strb_regs[STR_R_STRB_GPAR0_MUY];
		mu0_field = &str_strb_fields[STR_F_STRB_GPAR0_MU0];
		mu1_field = &str_strb_fields[STR_F_STRB_GPAR0_MU1];
		mu2_field = &str_strb_fields[STR_F_STRB_GPAR0_MU2];
		break;
	case GRP_PARAM_1:
		muc_reg = &str_strb_regs[STR_R_STRB_GPAR1_MUC];
		muy_reg = &str_strb_regs[STR_R_STRB_GPAR1_MUY];
		mu0_field = &str_strb_fields[STR_F_STRB_GPAR1_MU0];
		mu1_field = &str_strb_fields[STR_F_STRB_GPAR1_MU1];
		mu2_field = &str_strb_fields[STR_F_STRB_GPAR1_MU2];
		break;
	case GRP_PARAM_2:
		muc_reg = &str_strb_regs[STR_R_STRB_GPAR2_MUC];
		muy_reg = &str_strb_regs[STR_R_STRB_GPAR2_MUY];
		mu0_field = &str_strb_fields[STR_F_STRB_GPAR2_MU0];
		mu1_field = &str_strb_fields[STR_F_STRB_GPAR2_MU1];
		mu2_field = &str_strb_fields[STR_F_STRB_GPAR2_MU2];
		break;
	default:
		pr_err("[STR:HW][%s]invalid group id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, mu0_field, mu0);
	reg_val = camerapp_sfr_set_field_value(reg_val, mu1_field, mu1);
	camerapp_sfr_set_reg(base_addr, muc_reg, reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, mu2_field, mu2);
	camerapp_sfr_set_reg(base_addr, muy_reg, reg_val);
}

void _str_sfr_strb_set_grp_param_invcov0(void __iomem *base_addr, enum str_strb_grp_param_id id,
		u32 invcov0, u32 invcov1, u32 invcov2)
{
	const struct camerapp_sfr_reg *cov_a_reg, *cov_b_reg;
	const struct camerapp_sfr_field *cov0_field;
	const struct camerapp_sfr_field *cov1_field;
	const struct camerapp_sfr_field *cov2_field;
	u32 reg_val;

	switch (id) {
	case GRP_PARAM_0:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR0_INVCOV0A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR0_INVCOV0B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV00];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV01];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV02];
		break;
	case GRP_PARAM_1:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR1_INVCOV0A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR1_INVCOV0B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV00];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV01];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV02];
		break;
	case GRP_PARAM_2:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR2_INVCOV0A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR2_INVCOV0B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV00];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV01];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV02];
		break;
	default:
		pr_err("[STR:HW][%s]invalid group id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov0_field, invcov0);
	reg_val = camerapp_sfr_set_field_value(reg_val, cov1_field, invcov1);
	camerapp_sfr_set_reg(base_addr, cov_a_reg, reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov2_field, invcov2);
	camerapp_sfr_set_reg(base_addr, cov_b_reg, reg_val);

}

void _str_sfr_strb_set_grp_param_invcov1(void __iomem *base_addr, enum str_strb_grp_param_id id,
		u32 invcov0, u32 invcov1, u32 invcov2)
{
	const struct camerapp_sfr_reg *cov_a_reg, *cov_b_reg;
	const struct camerapp_sfr_field *cov0_field;
	const struct camerapp_sfr_field *cov1_field;
	const struct camerapp_sfr_field *cov2_field;
	u32 reg_val;

	switch (id) {
	case GRP_PARAM_0:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR0_INVCOV1A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR0_INVCOV1B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV10];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV11];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV12];
		break;
	case GRP_PARAM_1:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR1_INVCOV1A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR1_INVCOV1B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV10];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV11];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV12];
		break;
	case GRP_PARAM_2:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR2_INVCOV1A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR2_INVCOV1B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV10];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV11];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV12];
		break;
	default:
		pr_err("[STR:HW][%s]invalid group id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov0_field, invcov0);
	reg_val = camerapp_sfr_set_field_value(reg_val, cov1_field, invcov1);
	camerapp_sfr_set_reg(base_addr, cov_a_reg, reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov2_field, invcov2);
	camerapp_sfr_set_reg(base_addr, cov_b_reg, reg_val);

}

void _str_sfr_strb_set_grp_param_invcov2(void __iomem *base_addr, enum str_strb_grp_param_id id,
		u32 invcov0, u32 invcov1, u32 invcov2)
{
	const struct camerapp_sfr_reg *cov_a_reg, *cov_b_reg;
	const struct camerapp_sfr_field *cov0_field;
	const struct camerapp_sfr_field *cov1_field;
	const struct camerapp_sfr_field *cov2_field;
	u32 reg_val;

	switch (id) {
	case GRP_PARAM_0:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR0_INVCOV2A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR0_INVCOV2B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV20];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV21];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR0_INVCOV22];
		break;
	case GRP_PARAM_1:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR1_INVCOV2A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR1_INVCOV2B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV20];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV21];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR1_INVCOV22];
		break;
	case GRP_PARAM_2:
		cov_a_reg = &str_strb_regs[STR_R_STRB_GPAR2_INVCOV2A];
		cov_b_reg = &str_strb_regs[STR_R_STRB_GPAR2_INVCOV2B];
		cov0_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV20];
		cov1_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV21];
		cov2_field = &str_strb_fields[STR_F_STRB_GPAR2_INVCOV22];
		break;
	default:
		pr_err("[STR:HW][%s]invalid group id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov0_field, invcov0);
	reg_val = camerapp_sfr_set_field_value(reg_val, cov1_field, invcov1);
	camerapp_sfr_set_reg(base_addr, cov_a_reg, reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, cov2_field, invcov2);
	camerapp_sfr_set_reg(base_addr, cov_b_reg, reg_val);

}

void _str_sfr_strb_set_grp_param_pref_col(void __iomem *base_addr, enum str_strb_grp_param_id id,
		u8 col0, u8 col1, u8 col2, u8 col3)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *col0_field;
	const struct camerapp_sfr_field *col1_field;
	const struct camerapp_sfr_field *col2_field;
	const struct camerapp_sfr_field *col3_field;
	u32 reg_val;

	switch (id) {
	case GRP_PARAM_0:
		reg = &str_strb_regs[STR_R_STRB_GPAR0_PREF_COL];
		col0_field = &str_strb_fields[STR_F_STRB_GPAR0_PREF_COL0];
		col1_field = &str_strb_fields[STR_F_STRB_GPAR0_PREF_COL1];
		col2_field = &str_strb_fields[STR_F_STRB_GPAR0_PREF_COL2];
		col3_field = &str_strb_fields[STR_F_STRB_GPAR0_PREF_COL3];
		break;
	case GRP_PARAM_1:
		reg = &str_strb_regs[STR_R_STRB_GPAR1_PREF_COL];
		col0_field = &str_strb_fields[STR_F_STRB_GPAR1_PREF_COL0];
		col1_field = &str_strb_fields[STR_F_STRB_GPAR1_PREF_COL1];
		col2_field = &str_strb_fields[STR_F_STRB_GPAR1_PREF_COL2];
		col3_field = &str_strb_fields[STR_F_STRB_GPAR1_PREF_COL3];
		break;
	case GRP_PARAM_2:
		reg = &str_strb_regs[STR_R_STRB_GPAR2_PREF_COL];
		col0_field = &str_strb_fields[STR_F_STRB_GPAR2_PREF_COL0];
		col1_field = &str_strb_fields[STR_F_STRB_GPAR2_PREF_COL1];
		col2_field = &str_strb_fields[STR_F_STRB_GPAR2_PREF_COL2];
		col3_field = &str_strb_fields[STR_F_STRB_GPAR2_PREF_COL3];
		break;
	default:
		pr_err("[STR:HW][%s]invalid group id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, col0_field, col0);
	reg_val = camerapp_sfr_set_field_value(reg_val, col1_field, col1);
	reg_val = camerapp_sfr_set_field_value(reg_val, col2_field, col2);
	reg_val = camerapp_sfr_set_field_value(reg_val, col3_field, col3);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_strb_set_grp_param_del_y(void __iomem *base_addr, enum str_strb_grp_param_id id, u32 del_y)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *field;
	u32 reg_val;

	switch (id) {
	case GRP_PARAM_0:
		reg = &str_strb_regs[STR_R_STRB_GPAR0_DEL_Y];
		field = &str_strb_fields[STR_F_STRB_GPAR0_DEL_Y];
		break;
	case GRP_PARAM_1:
		reg = &str_strb_regs[STR_R_STRB_GPAR1_DEL_Y];
		field = &str_strb_fields[STR_F_STRB_GPAR1_DEL_Y];
		break;
	case GRP_PARAM_2:
		reg = &str_strb_regs[STR_R_STRB_GPAR2_DEL_Y];
		field = &str_strb_fields[STR_F_STRB_GPAR2_DEL_Y];
		break;
	default:
		pr_err("[STR:HW][%s]invalid group id(%d)\n", __func__, id);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, field, del_y);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_strc_set_ctrl(void __iomem *base_addr, bool rev_uv, u8 cam_type, u8 gyro, bool use_reg)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_REVERSE_UV], rev_uv);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_CAM_TYPE], cam_type);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_GYRO], gyro);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_PARAM_IN_REG], use_reg);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_CTRL], reg_val);
}

void _str_sfr_strc_set_clip_ratio(void __iomem *base_addr, u32 ratio, u32 inv_ratio)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_CLIP_RATIO], ratio);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_INVS_CR], inv_ratio);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_CLIP_RATIO], reg_val);
}

void _str_sfr_strc_set_mask_mul(void __iomem *base_addr, u32 mask_mul)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_MASK_MUL], mask_mul);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_MASK_MUL], reg_val);
}

void _str_sfr_strc_set_invs(void __iomem *base_addr, u32 inv_x, u32 inv_y)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_INVS_X], inv_x);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_INVS_X], reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_INVS_Y], inv_y);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_INVS_Y], reg_val);
}

void _str_sfr_strc_set_face(void __iomem *base_addr, u32 left, u32 top, u32 right, u32 bottom)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_FACE_LEFT], left);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_FACE_RIGHT], right);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_FACE_POS_H], reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_FACE_TOP], top);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_FACE_BOTTOM], bottom);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_FACE_POS_V], reg_val);

}

void _str_sfr_strc_set_roi(void __iomem *base_addr, u32 left, u32 top, u32 right, u32 bottom)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_ROI_LEFT], left);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_ROI_RIGHT], right);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_FACE_POS_H], reg_val);

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_ROI_TOP], top);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strc_fields[STR_F_STRC_ROI_BOTTOM], bottom);
	camerapp_sfr_set_reg(base_addr, &str_strc_regs[STR_R_STRC_FACE_POS_V], reg_val);

}

void _str_sfr_strd_set_ctrl(void __iomem *base_addr, bool enable_denoise)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val,
			&str_strd_fields[STR_F_STRD_DENOISE_ON], enable_denoise);
	camerapp_sfr_set_reg(base_addr, &str_strd_regs[STR_R_STRD_CTRL], reg_val);
}

void _str_sfr_strd_set_denoise_kern(void __iomem *base_addr, u8 kern_size, bool no_center_pix)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_KERNEL_SIZE], kern_size);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_NO_CENTER_PIX], no_center_pix);
	camerapp_sfr_set_reg(base_addr, &str_strd_regs[STR_R_STRD_DENOISE_KERNEL], reg_val);
}

void _str_sfr_strd_set_denoise_muldiv(void __iomem *base_addr, u32 muldiv)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_DENOISE_MUDIV], muldiv);
	camerapp_sfr_set_reg(base_addr, &str_strd_regs[STR_R_STRD_DENOISE_MUDIV], reg_val);
}

void _str_sfr_strd_set_denoise_param0(void __iomem *base_addr, u32 non_edge, u32 denoise)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_NON_EDGE_IN_HI], non_edge);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_DENOISE_HI], denoise);
	camerapp_sfr_set_reg(base_addr, &str_strd_regs[STR_R_STRD_DENOISE_PARAM0], reg_val);
}

void _str_sfr_strd_set_denoise_param1(void __iomem *base_addr, u32 blur, u32 inv_mul)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_BLUR_ALPHA], blur);
	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strd_fields[STR_F_STRD_DENOISE_INVMULK], inv_mul);
	camerapp_sfr_set_reg(base_addr, &str_strd_regs[STR_R_STRD_DENOISE_PARAM1], reg_val);
}

void _str_sfr_dma_start(void __iomem *base_addr, enum str_mux_port port)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *field;
	u32 reg_val;

	switch (port) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_FRAME_START];
		field = &str_rdma_fields[STR_F_RDMA0_CH0_FRAME_START];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_FRAME_START];
		field = &str_rdma_fields[STR_F_RDMA0_CH1_FRAME_START];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_FRAME_START];
		field = &str_rdma_fields[STR_F_RDMA0_CH2_FRAME_START];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_FRAME_START];
		field = &str_wdma_fields[STR_F_WDMA0_CH0_FRAME_START];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_FRAME_START];
		field = &str_wdma_fields[STR_F_WDMA0_CH1_FRAME_START];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_FRAME_START];
		field = &str_wdma_fields[STR_F_WDMA0_CH2_FRAME_START];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_FRAME_START];
		field = &str_wdma_fields[STR_F_WDMA0_CH3_FRAME_START];
		break;
	default:
		pr_err("[STR:HW][%s]invalid port(%d)\n", __func__, port);
		return;
	}

	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, field, 1);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void _str_sfr_top_dma_drcg_diable(void __iomem *base_addr, bool disable)
{
	u32 reg_val = 0;

	if (disable) {
		reg_val |= (1 << 0); //RDMA
		reg_val |= (1 << 16); //WDMA
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_DMA_DRCG_DIS_0], reg_val);
}

void _str_sfr_dma_sbi_stop_request(void __iomem *base_addr, bool stop)
{
	camerapp_sfr_set_field(base_addr, &str_dma_cfg_regs[STR_R_RDMA0_SBI_CONFIG],
			&str_dma_cfg_fields[STR_F_RDMA0_SBI_STOP_REQ], stop);
	camerapp_sfr_set_field(base_addr, &str_dma_cfg_regs[STR_R_WDMA0_SBI_CONFIG],
			&str_dma_cfg_fields[STR_F_WDMA0_SBI_STOP_REQ], stop);
}

bool _str_sfr_rdma_get_sbi_stop_ack(void __iomem *base_addr)
{
	return camerapp_sfr_get_field(base_addr, &str_dma_cfg_regs[STR_R_RDMA0_SBI_STATUS],
			&str_dma_cfg_fields[STR_F_RDMA0_SBI_STOP_ACK]);
}

bool _str_sfr_wdma_get_sbi_stop_ack(void __iomem *base_addr)
{
	return camerapp_sfr_get_field(base_addr, &str_dma_cfg_regs[STR_R_WDMA0_SBI_STATUS],
			&str_dma_cfg_fields[STR_F_WDMA0_SBI_STOP_ACK]);
}

void _str_sfr_sw_reset(void __iomem *base_addr, bool reset)
{
	u32 reg_val;

	if (reset)
		reg_val = 0xFFFF;
	else
		reg_val = 0;

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_PU_CORE_SW_RST_0], reg_val);
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_PU_APB_SW_RST_0], reg_val);
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_DMA_CORE_SW_RST_0], reg_val);
	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_DMA_APB_SW_RST_0], reg_val);

	camerapp_sfr_set_field(base_addr, &str_cmdq_regs[STR_R_STR_CMDQ_RESET],
			&str_cmdq_fields[STR_F_STR_CMDQ_RESET], reg_val);
}

u32 str_sfr_get_reg_ver(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_REG_VERSION]);
}

u32 str_sfr_get_str_ver(void __iomem *base_addr)
{
	return camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_SLSI_VERSION]);
}

void str_sfr_cmdq_set_mode(void __iomem *base_addr, bool enable)
{
	pr_info("[STR:HW]%s(%s)\n", __func__, (enable ? "ENABLE" : "DISABLE"));

	camerapp_sfr_set_field(base_addr, &str_cmdq_regs[STR_R_STR_CMDQ_SET_MODE],
			&str_cmdq_fields[STR_F_STR_CMDQ_SET_MODE], enable);
}

void str_sfr_dxi_mux_sel(void __iomem *base_addr, enum str_ip ip)
{
	_str_sfr_top_apb_sw_rst(base_addr, ip);

	switch (ip) {
	case PU_STRA:
		_str_sfr_mux_sel(base_addr, STRA_IN0, RDMA00);
		_str_sfr_mux_sel(base_addr, WDMA00, STRA_OUT0);
		break;
	case PU_STRB:
		_str_sfr_mux_sel(base_addr, STRB_IN0, RDMA00);
		_str_sfr_mux_sel(base_addr, STRB_IN1, RDMA01);
		_str_sfr_mux_sel(base_addr, STRB_IN2, RDMA02);
		break;
	case PU_STRC:
		_str_sfr_mux_sel(base_addr, STRC_IN0, RDMA00);
		_str_sfr_mux_sel(base_addr, STRC_IN1, RDMA01);
		_str_sfr_mux_sel(base_addr, STRC_IN2, RDMA02);
		_str_sfr_mux_sel(base_addr, WDMA00, STRC_OUT0);
		_str_sfr_mux_sel(base_addr, WDMA01, STRC_OUT1);
		_str_sfr_mux_sel(base_addr, WDMA02, STRC_OUT2);
		_str_sfr_mux_sel(base_addr, WDMA03, STRC_OUT3);
		break;
	case PU_STRD:
		_str_sfr_mux_sel(base_addr, STRD_IN0, RDMA00);
		_str_sfr_mux_sel(base_addr, STRD_IN1, RDMA01);
		_str_sfr_mux_sel(base_addr, WDMA00, STRD_OUT0);
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}
}

void str_sfr_stra_set_params(void __iomem *base_addr, struct stra_params *param)
{
	if (!param) {
		pr_err("[STR:HW][%s]param is NULL\n", __func__);
		return;
	}

	_str_sfr_stra_set_ctrl(base_addr, param->rev_uv, param->cam_type, param->gyro);
	_str_sfr_pu_set_face_num(base_addr, PU_STRA, param->face_num);
	_str_sfr_stra_set_ref_skin_num(base_addr, param->ref_skin_num);
	_str_sfr_stra_set_image_size(base_addr, param->width, param->height);
	_str_sfr_stra_set_dist_th(base_addr, param->face_th, param->no_face_th);
	_str_sfr_stra_set_skin_ratio(base_addr, param->skin_ratio);

	for (enum str_stra_ref_param_id id = REF_PARAM_0; id < REF_PARAM_CNT; id++) {
		_str_sfr_stra_set_ref_param_mean(base_addr, id,
				param->ref_param[id].mean[0], param->ref_param[id].mean[1]);
		_str_sfr_stra_set_ref_param_inv_cov(base_addr, id,
				param->ref_param[id].cov[0], param->ref_param[id].cov[1],
				param->ref_param[id].cov[2]);
		_str_sfr_stra_set_ref_param_weight(base_addr, id, param->ref_param[id].weight);
	}
}

void str_sfr_strb_set_params(void __iomem *base_addr, struct strb_params *param)
{
	if (!param) {
		pr_err("[STR:HW][%s]param is NULL\n", __func__);
		return;
	}

	_str_sfr_strb_set_ctrl(base_addr, param->rev_uv, param->cam_type, param->gyro,
			param->use_reg, param->is_preview, param->res_delta, param->is_second);
	_str_sfr_pu_set_face_num(base_addr, PU_STRB, param->face_num);
	_str_sfr_strb_set_cov_scale(base_addr, param->cov_scale);
	_str_sfr_strb_set_in_params(base_addr, param->mu_ye, param->mu_ys,
			param->inv_mu_yse, param->y_int, param->mul_k);
	_str_sfr_strb_set_grp_num(base_addr, param->grp_num);

	for (enum str_strb_grp_param_id id = GRP_PARAM_0; id < GRP_PARAM_CNT; id++) {
		_str_sfr_strb_set_grp_param_mean(base_addr, id,
				param->grp_param[id].mean[0], param->grp_param[id].mean[1],
				param->grp_param[id].mean[2]);
		_str_sfr_strb_set_grp_param_invcov0(base_addr, id,
				param->grp_param[id].invcov[0][0], param->grp_param[id].invcov[0][1],
				param->grp_param[id].invcov[0][2]);
		_str_sfr_strb_set_grp_param_invcov1(base_addr, id,
				param->grp_param[id].invcov[1][0], param->grp_param[id].invcov[1][1],
				param->grp_param[id].invcov[1][2]);
		_str_sfr_strb_set_grp_param_invcov2(base_addr, id,
				param->grp_param[id].invcov[2][0], param->grp_param[id].invcov[2][1],
				param->grp_param[id].invcov[2][2]);
		_str_sfr_strb_set_grp_param_pref_col(base_addr, id,
				param->grp_param[id].pref_col[0], param->grp_param[id].pref_col[1],
				param->grp_param[id].pref_col[2], param->grp_param[id].pref_col[3]);
		_str_sfr_strb_set_grp_param_del_y(base_addr, id, param->grp_param[id].del_y);
	}
}

void str_sfr_strc_set_params(void __iomem *base_addr, struct strc_params *param)
{
	if (!param) {
		pr_err("[STR:HW][%s]param is NULL\n", __func__);
		return;
	}

	_str_sfr_strc_set_ctrl(base_addr, param->rev_uv, param->cam_type, param->gyro, param->use_reg);
	_str_sfr_pu_set_face_num(base_addr, PU_STRC, param->face_num);
	_str_sfr_strc_set_clip_ratio(base_addr, param->clip_ratio, param->inv_clip_ratio);
	_str_sfr_strc_set_mask_mul(base_addr, param->mask_mul);
	_str_sfr_strc_set_invs(base_addr, param->inv_x, param->inv_y);
	_str_sfr_strc_set_face(base_addr, param->face.left, param->face.top, param->face.right, param->face.bottom);
	_str_sfr_strc_set_roi(base_addr, param->roi.left, param->roi.top, param->roi.right, param->roi.bottom);
}

void str_sfr_strd_set_params(void __iomem *base_addr, struct strd_params *param)
{
	if (!param) {
		pr_err("[STR:HW][%s]param is NULL\n", __func__);
		return;
	}

	_str_sfr_strd_set_ctrl(base_addr, param->enable_denoise);
	_str_sfr_strd_set_denoise_kern(base_addr, param->kern_size, param->no_center_pix);
	_str_sfr_strd_set_denoise_muldiv(base_addr, param->muldiv);
	_str_sfr_strd_set_denoise_param0(base_addr, param->non_edge, param->denoise);
	_str_sfr_strd_set_denoise_param1(base_addr, param->blur, param->inv_mul);
}

void str_sfr_dma_set_cfg(void __iomem *base_addr, enum str_mux_port dma_ch, struct str_dma_cfg *dma_cfg)
{
	_str_sfr_dma_set_frame_region(base_addr, dma_ch,
			dma_cfg->pos_x, dma_cfg->pos_y, dma_cfg->width, dma_cfg->height);
	_str_sfr_dma_set_tile_size(base_addr, dma_ch, dma_cfg->tile_w, dma_cfg->tile_h);
	_str_sfr_dma_set_tile_num(base_addr, dma_ch, dma_cfg->tile_x_num, dma_cfg->tile_y_num);
	_str_sfr_dma_set_tot_ring(base_addr, dma_ch,
			dma_cfg->tot_ring.left, dma_cfg->tot_ring.top,
			dma_cfg->tot_ring.right, dma_cfg->tot_ring.bottom);
	_str_sfr_dma_set_tile_ctrl_mode(base_addr, dma_ch,
			dma_cfg->frame_ring.left, dma_cfg->frame_ring.top,
			dma_cfg->frame_ring.right, dma_cfg->frame_ring.bottom,
			dma_cfg->m_tile_mode_x, dma_cfg->m_tile_mode_y,
			dma_cfg->tile_asym_mode, dma_cfg->tile_rand_mode, dma_cfg->tile_manual_mode);
	_str_sfr_dma_set_tile_pos(base_addr, dma_ch, dma_cfg->m_tile_pos_x, dma_cfg->m_tile_pos_y);
	_str_sfr_dma_set_addr(base_addr, dma_ch, dma_cfg->addr);
	_str_sfr_dma_set_stride(base_addr, dma_ch, dma_cfg->stride, false);
	_str_sfr_dma_set_format(base_addr, dma_ch, dma_cfg->bpp, dma_cfg->ppc,
			(dma_cfg->repeat & 0x1), (dma_cfg->repeat & 0x2),
			dma_cfg->line_valid, dma_cfg->line_skip);
	_str_sfr_dma_set_ctrl(base_addr, dma_ch, dma_cfg->fifo_addr, dma_cfg->fifo_size, false);
	str_sfr_dma_err_enable(base_addr, dma_ch, DMA_ERR_ENABLE);
}

void str_sfr_pu_set_cfg(void __iomem *base_addr, enum str_ip ip, struct str_pu_size_cfg *size_cfg)
{
	pr_info("HSL:[%s][IP%d]frame_size(%dx%d) tile_size(%dx%d) tile_num(%dx%d)\n",
			__func__, ip,
			size_cfg->width, size_cfg->height,
			size_cfg->tile_w, size_cfg->tile_h,
			size_cfg->tile_x_num, size_cfg->tile_y_num);
	_str_sfr_pu_set_frame_size(base_addr, ip, size_cfg->width, size_cfg->height);
	_str_sfr_pu_set_tile_size(base_addr, ip, size_cfg->tile_w, size_cfg->tile_h);
	_str_sfr_pu_set_tile_num(base_addr, ip, size_cfg->tile_x_num, size_cfg->tile_y_num);
	_str_sfr_pu_set_tot_ring(base_addr, ip,
			size_cfg->tot_ring.left, size_cfg->tot_ring.top,
			size_cfg->tot_ring.right, size_cfg->tot_ring.bottom);
	_str_sfr_pu_set_tile_ctrl_mode(base_addr, ip,
			size_cfg->m_tile_mode_x, size_cfg->m_tile_mode_y,
			size_cfg->tile_asym_mode, size_cfg->tile_manual_mode);
	_str_sfr_pu_set_tile_pos(base_addr, ip, size_cfg->m_tile_pos_x, size_cfg->m_tile_pos_y);
}

void str_sfr_pu_start(void __iomem *base_addr, enum str_ip ip)
{
	const struct camerapp_sfr_reg *reg;
	const struct camerapp_sfr_field *field;
	u32 reg_val;

	switch (ip) {
	case PU_STRA:
		reg = &str_stra_regs[STR_R_STRA_FRAME_START];
		field = &str_stra_fields[STR_F_STRA_FRAME_START];
		break;
	case PU_STRB:
		reg = &str_stra_regs[STR_R_STRB_FRAME_START];
		field = &str_stra_fields[STR_F_STRB_FRAME_START];
		break;
	case PU_STRC:
		reg = &str_stra_regs[STR_R_STRC_FRAME_START];
		field = &str_stra_fields[STR_F_STRC_FRAME_START];
		break;
	case PU_STRD:
		reg = &str_stra_regs[STR_R_STRD_FRAME_START];
		field = &str_stra_fields[STR_F_STRD_FRAME_START];
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	pr_info("HSL:[%s][IP%d]\n", __func__, ip);
	reg_val = 0;
	reg_val = camerapp_sfr_set_field_value(reg_val, field, 1);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void str_sfr_pu_dma_start(void __iomem *base_addr, enum str_ip ip)
{
	switch (ip) {
	case PU_STRA:
		_str_sfr_dma_start(base_addr, WDMA00);
		_str_sfr_dma_start(base_addr, RDMA00);
		break;
	case PU_STRB:
		_str_sfr_dma_start(base_addr, RDMA00);
		_str_sfr_dma_start(base_addr, RDMA01);
		_str_sfr_dma_start(base_addr, RDMA02);
		break;
	case PU_STRC:
		_str_sfr_dma_start(base_addr, WDMA00);
		_str_sfr_dma_start(base_addr, WDMA01);
		_str_sfr_dma_start(base_addr, WDMA02);
		_str_sfr_dma_start(base_addr, WDMA03);
		_str_sfr_dma_start(base_addr, RDMA00);
		_str_sfr_dma_start(base_addr, RDMA01);
		_str_sfr_dma_start(base_addr, RDMA02);
		break;
	case PU_STRD:
		_str_sfr_dma_start(base_addr, WDMA00);
		_str_sfr_dma_start(base_addr, RDMA00);
		_str_sfr_dma_start(base_addr, RDMA01);
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}
}

void str_sfr_cmdq_set_frm_marker(void __iomem *base_addr)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_cmdq_fields[STR_F_STR_CMDQ_FRM_MARKER], 1);
	camerapp_sfr_set_reg(base_addr, &str_cmdq_regs[STR_R_STR_CMDQ_FRM_MARKER], reg_val);

	reg_val = camerapp_sfr_get_reg(base_addr, &str_cmdq_regs[STR_R_STR_CMDQ_FRM_MARKER]); 
	pr_info("HSL:[%s]:mark(%d)\n", __func__, reg_val);
}

void str_sfr_strb_rerun(void __iomem *base_addr)
{
	u32 reg_val = 0;

	reg_val = camerapp_sfr_set_field_value(reg_val, &str_strb_fields[STR_F_STRB_SECOND], 1);
	camerapp_sfr_set_reg(base_addr, &str_strb_regs[STR_R_STRB_CTRL], reg_val);

	str_sfr_pu_start(base_addr, PU_STRB);
	_str_sfr_dma_start(base_addr, RDMA00);
	_str_sfr_dma_start(base_addr, RDMA01);
}

void str_sfr_cmdq_set_run_cnt(void __iomem *base_addr, u8 face_num)
{
	u32 reg_val = 0;
	u8 run_cnt = face_num * 5; /* 5 PU cycle per single face */

	pr_info("[STR:HW]frame start. face_num(%d)\n", face_num);

	reg_val = camerapp_sfr_set_field_value(reg_val,
			&str_cmdq_fields[STR_F_STR_CMDQ_FRAME_RUN_CNT], run_cnt);
	camerapp_sfr_set_reg(base_addr, &str_cmdq_regs[STR_R_STR_CMDQ_RUN_CNT], reg_val);
}

void str_sfr_top_irq_enable(void __iomem *base_addr, u8 port_num, enum str_irq_src_id src)
{
	const struct camerapp_sfr_reg *reg;
	u32 reg_val = 0;
	u8 irq_reg_id = STR_R_STR_ENABLE_IRQ_0;

	irq_reg_id += port_num;
	if (irq_reg_id > STR_R_STR_ENABLE_IRQ_5) {
		pr_err("[STR:HW][%s]invalid IRQ port_num(%d)\n", __func__, port_num);
		return;
	}

	reg = &str_top_regs[irq_reg_id];
	reg_val = camerapp_sfr_get_reg(base_addr, reg);
	reg_val |= (1 << src);

	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

void str_sfr_top_roi_evt_enable(void __iomem *base_addr, enum str_ip ip)
{
	u32 reg_val = 0;

	switch (ip) {
	case PU_STRA:
		reg_val = (1 << 0); //STRA
		reg_val |= (1 << 20); //WDMA00
		break;
	case PU_STRB:
		reg_val = (1 << 1); //STRB
		break;
	case PU_STRC:
		reg_val = (1 << 20); //WDMA00
		reg_val |= (1 << 21); //WDMA01
		reg_val |= (1 << 22); //WDMA02
		reg_val |= (1 << 23); //WDMA03
		break;
	case PU_STRD:
		reg_val = (1 << 20); //WDMA00
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_CMDQ_ROI_EVT_EN], reg_val);
}

void str_sfr_top_pu_frame_done_intr_enable(void __iomem *base_addr, enum str_ip ip)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_PU_FRAME_0]);

	switch (ip) {
	case PU_STRA:
		reg_val |= (1 << 0);
		break;
	case PU_STRB:
		reg_val |= (1 << 1);
		break;
	case PU_STRC:
		reg_val |= (1 << 2);
		break;
	case PU_STRD:
		reg_val |= (1 << 3);
		break;
	case CMDQ:
		reg_val |= (1 << 4);
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_PU_FRAME_0], reg_val);
}

u32 str_sfr_top_get_frame_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_pu_frame_done_intr_status(base_addr);
	_str_sfr_top_pu_frame_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_dma_err_enable(void __iomem *base_addr, enum str_mux_port dma_ch, u32 reg_val)
{
	const struct camerapp_sfr_reg *reg;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_ERROR_ENABLE];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_ERROR_ENABLE];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_ERROR_ENABLE];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_ERROR_ENABLE];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_ERROR_ENABLE];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_ERROR_ENABLE];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_ERROR_ENABLE];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, reg, reg_val);
}

u32 str_sfr_dma_err_clear(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	const struct camerapp_sfr_reg *reg;
	u32 reg_val;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_ERROR_CLEAR];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_ERROR_CLEAR];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_ERROR_CLEAR];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_ERROR_CLEAR];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_ERROR_CLEAR];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_ERROR_CLEAR];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_ERROR_CLEAR];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return 0;
	}

	reg_val = camerapp_sfr_get_reg(base_addr, reg);
	camerapp_sfr_set_reg(base_addr, reg, reg_val);

	return reg_val;
}

u32 str_sfr_dma_get_err_status(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	const struct camerapp_sfr_reg *reg;

	switch (dma_ch) {
	case RDMA00:
		reg = &str_rdma_regs[STR_R_RDMA0_CH0_ERROR_STATUS];
		break;
	case RDMA01:
		reg = &str_rdma_regs[STR_R_RDMA0_CH1_ERROR_STATUS];
		break;
	case RDMA02:
		reg = &str_rdma_regs[STR_R_RDMA0_CH2_ERROR_STATUS];
		break;
	case WDMA00:
		reg = &str_wdma_regs[STR_R_WDMA0_CH0_ERROR_STATUS];
		break;
	case WDMA01:
		reg = &str_wdma_regs[STR_R_WDMA0_CH1_ERROR_STATUS];
		break;
	case WDMA02:
		reg = &str_wdma_regs[STR_R_WDMA0_CH2_ERROR_STATUS];
		break;
	case WDMA03:
		reg = &str_wdma_regs[STR_R_WDMA0_CH3_ERROR_STATUS];
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma ch(%d)\n", __func__, dma_ch);
		return 0;
	}

	return camerapp_sfr_get_reg(base_addr, reg);
}

u32 str_sfr_sw_all_reset(void __iomem *base_addr)
{
	u32 reset_count = 0;

	pr_info("[STR:HW][%s]\n", __func__);

	_str_sfr_top_dma_drcg_diable(base_addr, true);
	_str_sfr_dma_sbi_stop_request(base_addr, true);

	do {
		if (reset_count++ > 10000) {
			pr_err("[STR:HW][%s]RDMA stop ack timeout. reset_count(%d)\n", __func__, reset_count);
			return reset_count;
		}
	} while (!_str_sfr_rdma_get_sbi_stop_ack(base_addr));

	reset_count = 0;
	do {
		if (reset_count++ > 10000) {
			pr_err("[STR:HW][%s]WDMA stop ack timeout. reset_count(%d)\n", __func__, reset_count);
			return reset_count;
		}
	} while (!_str_sfr_wdma_get_sbi_stop_ack(base_addr));

	_str_sfr_sw_reset(base_addr, true);
	_str_sfr_sw_reset(base_addr, false);
	_str_sfr_dma_sbi_stop_request(base_addr, false);
	_str_sfr_top_dma_drcg_diable(base_addr, false);

	return 0;
}

void str_sfr_top_pu_err_done_intr_enable(void __iomem *base_addr, enum str_ip ip)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_PU_ERROR_0]);

	switch (ip) {
	case PU_STRA:
		reg_val |= (1 << 0);
		break;
	case PU_STRB:
		reg_val |= (1 << 1);
		break;
	case PU_STRC:
		reg_val |= (1 << 2);
		break;
	case PU_STRD:
		reg_val |= (1 << 3);
		break;
	case SRAM_MUX:
		reg_val |= (1 << 30);
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_PU_ERROR_0], reg_val);
}

u32 str_sfr_top_get_pu_err_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_pu_err_done_intr_status(base_addr);
	_str_sfr_top_pu_err_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_top_dma_frame_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DMA_FRAME_0]);

	switch (dma_ch) {
	case RDMA00:
		reg_val |= (1 << 0);
		break;
	case RDMA01:
		reg_val |= (1 << 1);
		break;
	case RDMA02:
		reg_val |= (1 << 2);
		break;
	case WDMA00:
		reg_val |= (1 << 16);
		break;
	case WDMA01:
		reg_val |= (1 << 17);
		break;
	case WDMA02:
		reg_val |= (1 << 18);
		break;
	case WDMA03:
		reg_val |= (1 << 19);
		break;
	case TDMA:
		reg_val |= (1 << 29);
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma_ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DMA_FRAME_0], reg_val);
}

u32 str_sfr_top_get_dma_frame_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_dma_frame_done_intr_status(base_addr);
	_str_sfr_top_dma_frame_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_top_dma_err_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DMA_ERROR_0]);

	switch (dma_ch) {
	case RDMA00:
		reg_val |= (1 << 0);
		break;
	case RDMA01:
		reg_val |= (1 << 1);
		break;
	case RDMA02:
		reg_val |= (1 << 2);
		break;
	case WDMA00:
		reg_val |= (1 << 16);
		break;
	case WDMA01:
		reg_val |= (1 << 17);
		break;
	case WDMA02:
		reg_val |= (1 << 18);
		break;
	case WDMA03:
		reg_val |= (1 << 19);
		break;
	case TDMA:
		reg_val |= (1 << 29);
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma_ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DMA_ERROR_0], reg_val);
}

u32 str_sfr_top_get_dma_err_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_dma_err_done_intr_status(base_addr);
	_str_sfr_top_dma_err_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_top_pu_tile_done_intr_enable(void __iomem *base_addr, enum str_ip ip)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_PU_TILE_0]);

	switch (ip) {
	case PU_STRA:
		reg_val |= (1 << 0);
		break;
	case PU_STRB:
		reg_val |= (1 << 1);
		break;
	case PU_STRC:
		reg_val |= (1 << 2);
		break;
	case PU_STRD:
		reg_val |= (1 << 3);
		break;
	default:
		pr_err("[STR:HW][%s]invalid ip(%d)\n", __func__, ip);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_PU_TILE_0], reg_val);
}

u32 str_sfr_top_get_pu_tile_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_pu_tile_done_intr_status(base_addr);
	_str_sfr_top_pu_tile_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_top_dma_tile_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DMA_TILE_0]);

	switch (dma_ch) {
	case RDMA00:
		reg_val |= (1 << 0);
		break;
	case RDMA01:
		reg_val |= (1 << 1);
		break;
	case RDMA02:
		reg_val |= (1 << 2);
		break;
	case WDMA00:
		reg_val |= (1 << 16);
		break;
	case WDMA01:
		reg_val |= (1 << 17);
		break;
	case WDMA02:
		reg_val |= (1 << 18);
		break;
	case WDMA03:
		reg_val |= (1 << 19);
		break;
	case TDMA:
		reg_val |= (1 << 29);
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma_ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DMA_TILE_0], reg_val);
}

u32 str_sfr_top_get_dma_tile_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_dma_tile_done_intr_status(base_addr);
	_str_sfr_top_dma_tile_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_top_dxi_tile_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DXI_TILE_0]);

	switch (dma_ch) {
	case RDMA00:
		reg_val |= (1 << 0);
		break;
	case RDMA01:
		reg_val |= (1 << 1);
		break;
	case RDMA02:
		reg_val |= (1 << 2);
		break;
	case WDMA00:
		reg_val |= (1 << 16);
		break;
	case WDMA01:
		reg_val |= (1 << 17);
		break;
	case WDMA02:
		reg_val |= (1 << 18);
		break;
	case WDMA03:
		reg_val |= (1 << 19);
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma_ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DXI_TILE_0], reg_val);
}

u32 str_sfr_top_get_dxi_tile_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_dxi_tile_done_intr_status(base_addr);
	_str_sfr_top_dxi_tile_done_intr_clear(base_addr, reg_val);

	return reg_val;
}

void str_sfr_top_dxi_frame_done_intr_enable(void __iomem *base_addr, enum str_mux_port dma_ch)
{
	u32 reg_val = camerapp_sfr_get_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DXI_FRAME_0]);

	switch (dma_ch) {
	case RDMA00:
		reg_val |= (1 << 0);
		break;
	case RDMA01:
		reg_val |= (1 << 1);
		break;
	case RDMA02:
		reg_val |= (1 << 2);
		break;
	case WDMA00:
		reg_val |= (1 << 16);
		break;
	case WDMA01:
		reg_val |= (1 << 17);
		break;
	case WDMA02:
		reg_val |= (1 << 18);
		break;
	case WDMA03:
		reg_val |= (1 << 19);
		break;
	default:
		pr_err("[STR:HW][%s]invalid dma_ch(%d)\n", __func__, dma_ch);
		return;
	}

	camerapp_sfr_set_reg(base_addr, &str_top_regs[STR_R_STR_INTR_ENABLE_DXI_FRAME_0], reg_val);
}

u32 str_sfr_top_get_dxi_frame_done_intr_status_clear(void __iomem *base_addr)
{
	u32 reg_val;

	reg_val = _str_sfr_top_dxi_frame_done_intr_status(base_addr);
	_str_sfr_top_dxi_frame_done_intr_clear(base_addr, reg_val);

	return reg_val;
}
