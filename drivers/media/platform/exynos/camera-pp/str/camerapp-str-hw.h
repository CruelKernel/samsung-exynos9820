/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos CAMERA-PP GDC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CAMERAPP_STR_HW_H
#define CAMERAPP_STR_HW_H

#include <linux/kernel.h>

#include "camerapp-str-sfr-api-v100.h"
#include "camerapp-str-metadata.h"

enum str_grp_region_id {
	REGION_0,
	REGION_1,
	REGION_2,
	REGION_3,
	REGION_4,
	REGION_5,
	REGION_6,
	REGION_7,
	GRP_REGION_CNT
};

struct str_addr {
	dma_addr_t 	addr;
	u32 		size;
};

struct str_hw_dma_err_stat {
	u32	rdma00;
	u32	rdma01;
	u32	rdma02;
	u32	wdma00;
	u32	wdma01;
	u32	wdma02;
	u32	wdma03;
};

struct str_hw_irq_stat {
	u32				pu_tile_done;
	u32				pu_frame_done;
	u32				pu_err_done;
	u32				dma_tile_done;
	u32				dma_frame_done;
	u32				dma_err_done;
	struct str_hw_dma_err_stat	dma_err_status;
	u32				dxi_tile_done;
	u32				dxi_frame_done;
};

struct str_hw {
	void __iomem		*regs;
	struct str_hw_irq_stat	status;
	u32			reg_ver;
	u32			str_ver;
	struct str_addr		image_y_addr;
	struct str_addr		image_c_addr;
	struct str_addr		roi_y_addr;
	struct str_addr		roi_c_addr;
	u32			image_width;
	u32			image_height;
	enum camera_id		cam_type;
	enum direction_id	gyro;
	enum direction_id	mouth_pos;
	bool			is_preview;
	u8			face_id;
	enum str_grp_region_id	grp_region_id;
	struct str_region	face_y;
	struct str_region	face_c;
	struct str_region	roi_y;
	struct str_region	roi_c;
	struct stra_params 	stra_param;
	struct strb_params 	strb_param;
	struct strc_params 	strc_param;
	struct strd_params 	strd_param;
};

enum str_fraction_bit {
	BIT_01 = 1,
	BIT_O2,
	BIT_03,
	BIT_04,
	BIT_05,
	BIT_06,
	BIT_07,
	BIT_08,
	BIT_09,
	BIT_10,
	BIT_11,
	BIT_12,
	BIT_13,
	BIT_14,
	BIT_15,
	BIT_16,
	BIT_17,
	BIT_18,
	BIT_19,
	BIT_20
};

enum str_dma_ppc {
	PPC_1,
	PPC_2,
	PPC_4
};

#define FLOAT_TO_INT(a, b)	((int)((a) * (1 << (b))))
#define FLOAT_TO_UINT32(a, b)	((uint32_t)((a) * (1 << (b))))

void str_hw_init_ref_model(struct str_hw *hw);
void str_hw_init_group(struct str_hw *hw, enum str_grp_region_id region_id);
void str_hw_set_size(struct str_hw *hw, u32 width, u32 height);
void str_hw_set_direction(struct str_hw *hw, enum camera_id cam_type, enum direction_id gyro);
void str_hw_set_preview_flag(struct str_hw *hw, bool is_preview);
void str_hw_set_face(struct str_hw *hw, u32 left, u32 top, u32 right, u32 bottom, u8 face_id);

void str_hw_stra_update_param(struct str_hw *hw);
void str_hw_strb_update_param(struct str_hw *hw);
void str_hw_strc_update_param(struct str_hw *hw);
void str_hw_strd_update_param(struct str_hw *hw);

void str_hw_set_cmdq_mode(struct str_hw *hw, bool enable);

void str_hw_stra_dxi_mux_cfg(struct str_hw *hw);
void str_hw_strb_dxi_mux_cfg(struct str_hw *hw);
void str_hw_strc_dxi_mux_cfg(struct str_hw *hw);
void str_hw_strd_dxi_mux_cfg(struct str_hw *hw);

void str_hw_stra_dma_cfg(struct str_hw *hw);
void str_hw_strb_dma_cfg(struct str_hw *hw);
void str_hw_strc_dma_cfg(struct str_hw *hw);
void str_hw_strd_dma_cfg(struct str_hw *hw);

void str_hw_frame_done_irq_enable(struct str_hw *hw);

void str_hw_stra_ctrl_cfg(struct str_hw *hw);
void str_hw_strb_ctrl_cfg(struct str_hw *hw);
void str_hw_strc_ctrl_cfg(struct str_hw *hw);
void str_hw_strd_ctrl_cfg(struct str_hw *hw);

void str_hw_strb_set_second_mode(struct str_hw *hw);
void str_hw_str_start(struct str_hw *hw, u8 face_num);
void str_hw_update_irq_status(struct str_hw *hw);
void str_hw_update_version(struct str_hw *hw);
#endif //CAMERAPP_STR_HW_H
