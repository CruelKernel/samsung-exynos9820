/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS ISPP GDC driver
 * (FIMC-IS PostProcessing Generic Distortion Correction driver)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/printk.h>

#include "camerapp-str-param_v100.h"
#include "camerapp-str-cal.h"
#include "camerapp-str-hw.h"

void str_hw_init_ref_model(struct str_hw *hw)
{
	for (enum str_stra_ref_param_id id = REF_PARAM_0; id < REF_PARAM_CNT; id++) {
		hw->stra_param.ref_param[id].mean[0] = FLOAT_TO_INT(ref_model_mean[id][0], BIT_04);
		hw->stra_param.ref_param[id].mean[1] = FLOAT_TO_INT(ref_model_mean[id][1], BIT_04);

		hw->stra_param.ref_param[id].cov[0] = FLOAT_TO_INT(ref_model_inv_cov[id][0][0], BIT_14);
		hw->stra_param.ref_param[id].cov[1] = FLOAT_TO_INT(ref_model_inv_cov[id][0][1], BIT_14);
		hw->stra_param.ref_param[id].cov[2] = FLOAT_TO_INT(ref_model_inv_cov[id][1][1], BIT_14);

		hw->stra_param.ref_param[id].weight = FLOAT_TO_INT(ref_model_weight[id], BIT_07);
	}
}

void str_hw_init_group(struct str_hw *hw, enum str_grp_region_id region_id)
{
	for (enum str_strb_grp_param_id id = GRP_PARAM_0; id < grp_model_num[region_id]; id++) {
		hw->strb_param.grp_param[id].mean[0] =
			FLOAT_TO_INT(grp_mean_regions[region_id][id][0], BIT_04);
		hw->strb_param.grp_param[id].mean[1] =
			FLOAT_TO_INT(grp_mean_regions[region_id][id][1], BIT_04);
		hw->strb_param.grp_param[id].mean[2] =
			FLOAT_TO_INT(grp_mean_regions[region_id][id][2], BIT_04);

		hw->strb_param.grp_param[id].invcov[0][0] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][0][0], BIT_14);
		hw->strb_param.grp_param[id].invcov[0][1] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][0][1], BIT_14);
		hw->strb_param.grp_param[id].invcov[0][2] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][0][2], BIT_14);
		hw->strb_param.grp_param[id].invcov[1][0] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][1][0], BIT_14);
		hw->strb_param.grp_param[id].invcov[1][1] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][1][1], BIT_14);
		hw->strb_param.grp_param[id].invcov[1][2] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][1][2], BIT_14);
		hw->strb_param.grp_param[id].invcov[2][0] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][2][0], BIT_14);
		hw->strb_param.grp_param[id].invcov[2][1] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][2][1], BIT_14);
		hw->strb_param.grp_param[id].invcov[2][2] =
			FLOAT_TO_INT(grp_invcov_regions[region_id][id][2][2], BIT_14);

		hw->strb_param.grp_param[id].pref_col[0] = grp_pref_col_regions[region_id][id][0][0];
		hw->strb_param.grp_param[id].pref_col[1] = grp_pref_col_regions[region_id][id][0][1];
		hw->strb_param.grp_param[id].pref_col[2] = grp_pref_col_regions[region_id][id][1][0];
		hw->strb_param.grp_param[id].pref_col[3] = grp_pref_col_regions[region_id][id][1][0];

		hw->strb_param.grp_param[id].del_y = grp_del_y_regions[region_id][id];
	}

	hw->grp_region_id = region_id;
}

void str_hw_set_size(struct str_hw *hw, u32 width, u32 height)
{
	hw->image_width = width;
	hw->image_height = height;
}

void str_hw_set_direction(struct str_hw *hw, enum camera_id cam_type, enum direction_id gyro)
{
	enum direction_id mouth_pos;

	hw->cam_type = cam_type;
	hw->gyro = gyro;

	switch (cam_type) {
	case CAM_REAR:
		switch (gyro) {
		case HOR_DOWN:
			mouth_pos = HOR_DOWN;
			break;
		case VER_LEFT:
			mouth_pos = VER_LEFT;
			break;
		case HOR_UP:
			mouth_pos = HOR_UP;
			break;
		case VER_RIGHT:
			mouth_pos = VER_RIGHT;
			break;
		}
		break;
	case CAM_FRONT:
		switch (gyro) {
		case HOR_DOWN:
			mouth_pos = HOR_DOWN;
			break;
		case VER_LEFT:
			mouth_pos = VER_LEFT;
			break;
		case HOR_UP:
			mouth_pos = HOR_UP;
			break;
		case VER_RIGHT:
			mouth_pos = VER_RIGHT;
			break;
		}
		break;
	}

	hw->mouth_pos = mouth_pos;
}

void str_hw_set_preview_flag(struct str_hw *hw, bool is_preview)
{
	hw->is_preview = is_preview;
}

void str_hw_set_face(struct str_hw *hw, u32 left, u32 top, u32 right, u32 bottom, u8 face_id)
{
	hw->face_y.left = left;
	hw->face_y.top = top;
	hw->face_y.right = right;
	hw->face_y.bottom = bottom;
	hw->face_id = face_id;

	str_cal_adjust_face_region(&hw->face_y);
	str_cal_get_face_c_region(&hw->face_y, &hw->face_c);
	str_cal_get_roi(&hw->face_y, hw->is_preview, hw->image_width, hw->image_height, &hw->roi_y);
	str_cal_get_roi_c_region(&hw->roi_y, &hw->roi_c);
}

void str_hw_stra_update_param(struct str_hw *hw) {
	struct stra_params *param = &hw->stra_param;

	/* PU Control Parameters */
	param->rev_uv = 0;
	param->cam_type = 0;
	param->gyro = 0;
	param->face_num = hw->face_id;
	param->ref_skin_num = REF_PARAM_CNT;
	param->width = hw->image_width;
	param->height = hw->image_height;
	param->face_th = FACE_THRESHOLD;
	param->no_face_th = NON_FACE_THRESHOLD;
	param->skin_ratio = SKIN_RATIO;
	param->size_cfg.width = hw->face_c.right - hw->face_c.left;
	param->size_cfg.height = hw->face_c.bottom - hw->face_c.top;
	param->size_cfg.tile_w = param->size_cfg.width;
	param->size_cfg.tile_h = param->size_cfg.height;
	param->size_cfg.tile_x_num = 1;
	param->size_cfg.tile_y_num = 1;
	param->size_cfg.tot_ring.left = 0;
	param->size_cfg.tot_ring.top = 0;
	param->size_cfg.tot_ring.right = 0;
	param->size_cfg.tot_ring.bottom = 0;
	param->size_cfg.tile_asym_mode = 0;
	param->size_cfg.tile_manual_mode = 0;
	param->size_cfg.m_tile_mode_x = 0;
	param->size_cfg.m_tile_mode_y = 0;
	param->size_cfg.m_tile_pos_x = 0;
	param->size_cfg.m_tile_pos_x = 0;

	/* RDMA0 Paramteers - chroma face region */
	param->face_c.pos_x = hw->face_c.left * 2; //(U+V)
	param->face_c.pos_y = hw->face_c.top;
	param->face_c.width = (hw->face_c.right - hw->face_c.left) * 2; //(U+V)
	param->face_c.height = hw->face_c.bottom - hw->face_c.top;
	param->face_c.tile_w = param->face_c.width;
	param->face_c.tile_h = param->face_c.height;
	param->face_c.tile_x_num = 1;
	param->face_c.tile_y_num = 1;
	param->face_c.tot_ring.left = 0;
	param->face_c.tot_ring.top = 0;
	param->face_c.tot_ring.right = 0;
	param->face_c.tot_ring.bottom = 0;
	param->face_c.frame_ring.left = 0;
	param->face_c.frame_ring.top = 0;
	param->face_c.frame_ring.right = 0;
	param->face_c.frame_ring.bottom = 0;
	param->face_c.tile_asym_mode = 0;
	param->face_c.tile_rand_mode = 0;
	param->face_c.tile_manual_mode = 0;
	param->face_c.m_tile_mode_x = 0;
	param->face_c.m_tile_mode_y = 0;
	param->face_c.m_tile_pos_x = 0;
	param->face_c.m_tile_pos_y = 0;
	param->face_c.addr = hw->image_c_addr.addr;
	param->face_c.stride = ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->face_c.line_skip = 0;
	param->face_c.line_valid = 0;
	param->face_c.repeat = 0;
	param->face_c.ppc = PPC_2;
	param->face_c.bpp = 8;
	param->face_c.fifo_addr = 0;
	param->face_c.fifo_size = 64;

	/* WDMA0 Paramteers - distance map */
	param->dist_map.pos_x = 0;
	param->dist_map.pos_y = 0;
	param->dist_map.width = hw->face_c.right - hw->face_c.left;
	param->dist_map.height = hw->face_c.bottom - hw->face_c.top;
	param->dist_map.tile_w = param->dist_map.width;
	param->dist_map.tile_h = param->dist_map.height;
	param->dist_map.tile_x_num = 1;
	param->dist_map.tile_y_num = 1;
	param->dist_map.tot_ring.left = 0;
	param->dist_map.tot_ring.top = 0;
	param->dist_map.tot_ring.right = 0;
	param->dist_map.tot_ring.bottom = 0;
	param->dist_map.frame_ring.left = 0;
	param->dist_map.frame_ring.top = 0;
	param->dist_map.frame_ring.right = 0;
	param->dist_map.frame_ring.bottom = 0;
	param->dist_map.tile_asym_mode = 0;
	param->dist_map.tile_rand_mode = 0;
	param->dist_map.tile_manual_mode = 0;
	param->dist_map.m_tile_mode_x = 0;
	param->dist_map.m_tile_mode_y = 0;
	param->dist_map.m_tile_pos_x = 0;
	param->dist_map.m_tile_pos_y = 0;
	param->dist_map.addr = hw->roi_y_addr.addr;
	param->dist_map.stride = ALIGN((param->dist_map.height * 2), STR_MEM_STRIDE);
	param->dist_map.line_skip = 0;
	param->dist_map.line_valid = 0;
	param->dist_map.repeat = 0;
	param->dist_map.ppc = PPC_1;
	param->dist_map.bpp = 16;
	param->dist_map.fifo_addr = 0;
	param->dist_map.fifo_size = 64;
}

void str_hw_strb_update_param(struct str_hw *hw) {
	struct strb_params *param = &hw->strb_param;

	/* PU Control Parameters */
	param->rev_uv = 0;
	param->cam_type = hw->cam_type;
	param->gyro = hw->gyro;
	param->use_reg = false;
	param->is_preview = hw->is_preview;
	param->res_delta = (hw->face_id == 0);
	param->is_second = false;
	param->face_num = hw->face_id;
	param->cov_scale = COVARIANCE_SCALER;
	param->mu_ye = MU_YE;
	param->mu_ys = MU_YS;
	param->inv_mu_yse = INVERSE_MU_YSE;
	param->y_int = Y_INTERPOLATION;
	param->mul_k = MUL_K;
	param->grp_num = grp_model_num[hw->grp_region_id];
	param->size_cfg.width = hw->face_c.right - hw->face_c.left;
	param->size_cfg.height = hw->face_c.bottom - hw->face_c.top;
	param->size_cfg.tile_w = param->size_cfg.width;
	param->size_cfg.tile_h = param->size_cfg.height;
	param->size_cfg.tile_x_num = 1;
	param->size_cfg.tile_y_num = 1;
	param->size_cfg.tot_ring.left = 0;
	param->size_cfg.tot_ring.top = 0;
	param->size_cfg.tot_ring.right = 0;
	param->size_cfg.tot_ring.bottom = 0;
	param->size_cfg.tile_asym_mode = 0;
	param->size_cfg.tile_manual_mode = 0;
	param->size_cfg.m_tile_mode_x = 0;
	param->size_cfg.m_tile_mode_y = 0;
	param->size_cfg.m_tile_pos_x = 0;
	param->size_cfg.m_tile_pos_x = 0;

	/* RDMA0 Paramteers - distance map */
	param->dist_map.pos_x = 0;
	param->dist_map.pos_y = 0;
	param->dist_map.width = hw->face_c.right - hw->face_c.left;
	param->dist_map.height = hw->face_c.bottom - hw->face_c.top;
	param->dist_map.tile_w = param->dist_map.width;
	param->dist_map.tile_h = param->dist_map.height;
	param->dist_map.tile_x_num = 1;
	param->dist_map.tile_y_num = 1;
	param->dist_map.tot_ring.left = 0;
	param->dist_map.tot_ring.top = 0;
	param->dist_map.tot_ring.right = 0;
	param->dist_map.tot_ring.bottom = 0;
	param->dist_map.frame_ring.left = 0;
	param->dist_map.frame_ring.top = 0;
	param->dist_map.frame_ring.right = 0;
	param->dist_map.frame_ring.bottom = 0;
	param->dist_map.tile_asym_mode = 0;
	param->dist_map.tile_rand_mode = 0;
	param->dist_map.tile_manual_mode = 0;
	param->dist_map.m_tile_mode_x = 0;
	param->dist_map.m_tile_mode_y = 0;
	param->dist_map.m_tile_pos_x = 0;
	param->dist_map.m_tile_pos_y = 0;
	param->dist_map.addr = hw->roi_y_addr.addr;
	param->dist_map.stride = ALIGN((param->dist_map.height * 2), STR_MEM_STRIDE);
	param->dist_map.line_skip = 0;
	param->dist_map.line_valid = 0;
	param->dist_map.repeat = 0;
	param->dist_map.ppc = PPC_1;
	param->dist_map.bpp = 16;
	param->dist_map.fifo_addr = 0;
	param->dist_map.fifo_size = 64;

	/* RDMA1 Parameters - chroma face region */
	param->face_c.pos_x = hw->face_c.left * 2; //(U+V)
	param->face_c.pos_y = hw->face_c.top;
	param->face_c.width = (hw->face_c.right - hw->face_c.left) * 2; //(U+V)
	param->face_c.height = hw->face_c.bottom - hw->face_c.top;
	param->face_c.tile_w = param->face_c.width;
	param->face_c.tile_h = param->face_c.height;
	param->face_c.tile_x_num = 1;
	param->face_c.tile_y_num = 1;
	param->face_c.tot_ring.left = 0;
	param->face_c.tot_ring.top = 0;
	param->face_c.tot_ring.right = 0;
	param->face_c.tot_ring.bottom = 0;
	param->face_c.frame_ring.left = 0;
	param->face_c.frame_ring.top = 0;
	param->face_c.frame_ring.right = 0;
	param->face_c.frame_ring.bottom = 0;
	param->face_c.tile_asym_mode = 0;
	param->face_c.tile_rand_mode = 0;
	param->face_c.tile_manual_mode = 0;
	param->face_c.m_tile_mode_x = 0;
	param->face_c.m_tile_mode_y = 0;
	param->face_c.m_tile_pos_x = 0;
	param->face_c.m_tile_pos_y = 0;
	param->face_c.addr = hw->image_c_addr.addr;
	param->face_c.stride = ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->face_c.line_skip = 0;
	param->face_c.line_valid = 0;
	param->face_c.repeat = 0;
	param->face_c.ppc = PPC_2;
	param->face_c.bpp = 8;
	param->face_c.fifo_addr = 64;
	param->face_c.fifo_size = 64;

	/* RDMA2 Parameters - luma face region */
	param->face_y.pos_x = hw->face_y.left;
	param->face_y.pos_y = hw->face_y.top;
	param->face_y.width = (hw->face_y.right - hw->face_y.left);
	param->face_y.height = hw->face_y.bottom - hw->face_y.top;
	param->face_y.tile_w = param->face_y.width;
	param->face_y.tile_h = param->face_y.height;
	param->face_y.tile_x_num = 1;
	param->face_y.tile_y_num = 1;
	param->face_y.tot_ring.left = 0;
	param->face_y.tot_ring.top = 0;
	param->face_y.tot_ring.right = 0;
	param->face_y.tot_ring.bottom = 0;
	param->face_y.frame_ring.left = 0;
	param->face_y.frame_ring.top = 0;
	param->face_y.frame_ring.right = 0;
	param->face_y.frame_ring.bottom = 0;
	param->face_y.tile_asym_mode = 0;
	param->face_y.tile_rand_mode = 0;
	param->face_y.tile_manual_mode = 0;
	param->face_y.m_tile_mode_x = 0;
	param->face_y.m_tile_mode_y = 0;
	param->face_y.m_tile_pos_x = 0;
	param->face_y.m_tile_pos_y = 0;
	param->face_y.addr = hw->image_y_addr.addr;
	param->face_y.stride = ALIGN(hw->image_width, STR_MEM_STRIDE) * 2;
	param->face_y.line_skip = 0;
	param->face_y.line_valid = 0;
	param->face_y.repeat = 0;
	param->face_y.ppc = PPC_2;
	param->face_y.bpp = 8;
	param->face_y.fifo_addr = 128;
	param->face_y.fifo_size = 64;
}

void str_hw_strc_update_param(struct str_hw *hw) {
	struct strc_params *param = &hw->strc_param;

	/* PU Control Parameters */
	param->rev_uv = 0;
	param->cam_type = hw->cam_type;
	param->gyro = hw->gyro;
	param->use_reg = false;
	param->face_num = hw->face_id;
	param->clip_ratio = CLIP_RATIO;
	param->inv_clip_ratio = INVERSE_CLIP_RATIO;
	param->mask_mul = MUL_MASK;
	str_cal_get_inversed_face_size(&hw->face_y, hw->mouth_pos, &param->inv_x, &param->inv_y);
	param->face.left = hw->face_y.left;
	param->face.top = hw->face_y.top;
	param->face.right = hw->face_y.right;
	param->face.bottom = hw->face_y.bottom;
	param->roi.left = hw->roi_y.left - HW_RING_SIZE;
	param->roi.top = hw->roi_y.top - HW_RING_SIZE;
	param->roi.right = hw->roi_y.right + HW_RING_SIZE;
	param->roi.bottom = hw->roi_y.bottom + HW_RING_SIZE;
	param->size_cfg.width = hw->roi_c.right - hw->roi_c.left + 12;
	param->size_cfg.height = hw->roi_c.bottom - hw->roi_c.top + 12;
	param->size_cfg.tile_w = param->size_cfg.width;
	param->size_cfg.tile_h = param->size_cfg.height;
	param->size_cfg.tile_x_num = 1;
	param->size_cfg.tile_y_num = 1;
	param->size_cfg.tot_ring.left = 0;
	param->size_cfg.tot_ring.top = 0;
	param->size_cfg.tot_ring.right = 0;
	param->size_cfg.tot_ring.bottom = 0;
	param->size_cfg.tile_asym_mode = 0;
	param->size_cfg.tile_manual_mode = 0;
	param->size_cfg.m_tile_mode_x = 0;
	param->size_cfg.m_tile_mode_y = 0;
	param->size_cfg.m_tile_pos_x = 0;
	param->size_cfg.m_tile_pos_x = 0;

	/* RDMA0 Paramteers - chroma ROI */
	param->roi_c_src.pos_x = (hw->roi_c.left - 6) * 2; //(U+V)
	param->roi_c_src.pos_y = (hw->roi_c.top - 6);
	param->roi_c_src.width = (hw->roi_c.right - hw->roi_c.left + 12) * 2; //(U+V)
	param->roi_c_src.height = (hw->roi_c.bottom - hw->roi_c.top + 12);
	param->roi_c_src.tile_w = param->roi_c_src.width;
	param->roi_c_src.tile_h = param->roi_c_src.height;
	param->roi_c_src.tile_x_num = 1;
	param->roi_c_src.tile_y_num = 1;
	param->roi_c_src.tot_ring.left = 0;
	param->roi_c_src.tot_ring.top = 0;
	param->roi_c_src.tot_ring.right = 0;
	param->roi_c_src.tot_ring.bottom = 0;
	param->roi_c_src.frame_ring.left = 0;
	param->roi_c_src.frame_ring.top = 0;
	param->roi_c_src.frame_ring.right = 0;
	param->roi_c_src.frame_ring.bottom = 0;
	param->roi_c_src.tile_asym_mode = 0;
	param->roi_c_src.tile_rand_mode = 0;
	param->roi_c_src.tile_manual_mode = 0;
	param->roi_c_src.m_tile_mode_x = 0;
	param->roi_c_src.m_tile_mode_y = 0;
	param->roi_c_src.m_tile_pos_x = 0;
	param->roi_c_src.m_tile_pos_y = 0;
	param->roi_c_src.addr = hw->image_c_addr.addr;
	param->roi_c_src.stride = ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->roi_c_src.line_skip = 0;
	param->roi_c_src.line_valid = 0;
	param->roi_c_src.repeat = 0;
	param->roi_c_src.ppc = PPC_2;
	param->roi_c_src.bpp = 8;
	param->roi_c_src.fifo_addr = 0;
	param->roi_c_src.fifo_size = 64;

	/* RDMA1 Paramteers - luma ROI even */
	param->roi_y_src_even.pos_x = hw->roi_y.left - 12;
	param->roi_y_src_even.pos_y = (hw->roi_y.top - 12) / 2; //half line
	param->roi_y_src_even.width = (hw->roi_y.right - hw->roi_y.left) + 24;
	param->roi_y_src_even.height = (hw->roi_y.bottom - hw->roi_y.top + 24) / 2; //half line
	param->roi_y_src_even.tile_w = param->roi_y_src_even.width;
	param->roi_y_src_even.tile_h = param->roi_y_src_even.height;
	param->roi_y_src_even.tile_x_num = 1;
	param->roi_y_src_even.tile_y_num = 1;
	param->roi_y_src_even.tot_ring.left = 0;
	param->roi_y_src_even.tot_ring.top = 0;
	param->roi_y_src_even.tot_ring.right = 0;
	param->roi_y_src_even.tot_ring.bottom = 0;
	param->roi_y_src_even.frame_ring.left = 0;
	param->roi_y_src_even.frame_ring.top = 0;
	param->roi_y_src_even.frame_ring.right = 0;
	param->roi_y_src_even.frame_ring.bottom = 0;
	param->roi_y_src_even.tile_asym_mode = 0;
	param->roi_y_src_even.tile_rand_mode = 0;
	param->roi_y_src_even.tile_manual_mode = 0;
	param->roi_y_src_even.m_tile_mode_x = 0;
	param->roi_y_src_even.m_tile_mode_y = 0;
	param->roi_y_src_even.m_tile_pos_x = 0;
	param->roi_y_src_even.m_tile_pos_y = 0;
	param->roi_y_src_even.addr = hw->image_y_addr.addr;
	param->roi_y_src_even.stride = ALIGN(hw->image_width, STR_MEM_STRIDE) * 2;
	param->roi_y_src_even.line_skip = 0;
	param->roi_y_src_even.line_valid = 0;
	param->roi_y_src_even.repeat = 0;
	param->roi_y_src_even.ppc = PPC_2;
	param->roi_y_src_even.bpp = 8;
	param->roi_y_src_even.fifo_addr = 64;
	param->roi_y_src_even.fifo_size = 64;

	/* RDMA2 Paramteers - luma ROI odd */
	param->roi_y_src_odd.pos_x = hw->roi_y.left - 12;
	param->roi_y_src_odd.pos_y = (hw->roi_y.top - 12) / 2; //half line
	param->roi_y_src_odd.width = (hw->roi_y.right - hw->roi_y.left) + 24;
	param->roi_y_src_odd.height = (hw->roi_y.bottom - hw->roi_y.top + 24) / 2; //half line
	param->roi_y_src_odd.tile_w = param->roi_y_src_odd.width;
	param->roi_y_src_odd.tile_h = param->roi_y_src_odd.height;
	param->roi_y_src_odd.tile_x_num = 1;
	param->roi_y_src_odd.tile_y_num = 1;
	param->roi_y_src_odd.tot_ring.left = 0;
	param->roi_y_src_odd.tot_ring.top = 0;
	param->roi_y_src_odd.tot_ring.right = 0;
	param->roi_y_src_odd.tot_ring.bottom = 0;
	param->roi_y_src_odd.frame_ring.left = 0;
	param->roi_y_src_odd.frame_ring.top = 0;
	param->roi_y_src_odd.frame_ring.right = 0;
	param->roi_y_src_odd.frame_ring.bottom = 0;
	param->roi_y_src_odd.tile_asym_mode = 0;
	param->roi_y_src_odd.tile_rand_mode = 0;
	param->roi_y_src_odd.tile_manual_mode = 0;
	param->roi_y_src_odd.m_tile_mode_x = 0;
	param->roi_y_src_odd.m_tile_mode_y = 0;
	param->roi_y_src_odd.m_tile_pos_x = 0;
	param->roi_y_src_odd.m_tile_pos_y = 0;
	param->roi_y_src_odd.addr = hw->image_y_addr.addr + ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->roi_y_src_odd.stride = ALIGN(hw->image_width, STR_MEM_STRIDE) * 2;
	param->roi_y_src_odd.line_skip = 0;
	param->roi_y_src_odd.line_valid = 0;
	param->roi_y_src_odd.repeat = 0;
	param->roi_y_src_odd.ppc = PPC_2;
	param->roi_y_src_odd.bpp = 8;
	param->roi_y_src_odd.fifo_addr = 128;
	param->roi_y_src_odd.fifo_size = 64;

	/* WDMA0 Parameters - Chroma ROI Mask */
	param->roi_mask_c.pos_x = 0;
	param->roi_mask_c.pos_y = 0;
	param->roi_mask_c.width = hw->roi_c.right - hw->roi_c.left + 12;
	param->roi_mask_c.height = hw->roi_c.bottom - hw->roi_c.top + 12;
	param->roi_mask_c.tile_w = param->roi_mask_c.width;
	param->roi_mask_c.tile_h = param->roi_mask_c.height;
	param->roi_mask_c.tile_x_num = 1;
	param->roi_mask_c.tile_y_num = 1;
	param->roi_mask_c.tot_ring.left = 0;
	param->roi_mask_c.tot_ring.top = 0;
	param->roi_mask_c.tot_ring.right = 0;
	param->roi_mask_c.tot_ring.bottom = 0;
	param->roi_mask_c.frame_ring.left = 0;
	param->roi_mask_c.frame_ring.top = 0;
	param->roi_mask_c.frame_ring.right = 0;
	param->roi_mask_c.frame_ring.bottom = 0;
	param->roi_mask_c.tile_asym_mode = 0;
	param->roi_mask_c.tile_rand_mode = 0;
	param->roi_mask_c.tile_manual_mode = 0;
	param->roi_mask_c.m_tile_mode_x = 0;
	param->roi_mask_c.m_tile_mode_y = 0;
	param->roi_mask_c.m_tile_pos_x = 0;
	param->roi_mask_c.m_tile_pos_y = 0;
	param->roi_mask_c.addr = hw->roi_c_addr.addr;
	param->roi_mask_c.stride = ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->roi_mask_c.line_skip = 0;
	param->roi_mask_c.line_valid = 0;
	param->roi_mask_c.repeat = 0;
	param->roi_mask_c.ppc = PPC_1;
	param->roi_mask_c.bpp = 8;
	param->roi_mask_c.fifo_addr = 0;
	param->roi_mask_c.fifo_size = 64;

	/* WDMA1 Parameters - Chroma ROI */
	param->roi_c_dst.pos_x = hw->roi_c.left - 12;
	param->roi_c_dst.pos_y = hw->roi_c.top - 12;
	param->roi_c_dst.width = (hw->roi_c.right - hw->roi_c.left + 12) * 2; //(U+V)
	param->roi_c_dst.height = hw->roi_c.bottom - hw->roi_c.top + 12;
	param->roi_c_dst.tile_w = param->roi_c_src.width;
	param->roi_c_dst.tile_h = param->roi_c_src.height;
	param->roi_c_dst.tile_x_num = 1;
	param->roi_c_dst.tile_y_num = 1;
	param->roi_c_dst.tot_ring.left = 0;
	param->roi_c_dst.tot_ring.top = 0;
	param->roi_c_dst.tot_ring.right = 0;
	param->roi_c_dst.tot_ring.bottom = 0;
	param->roi_c_dst.frame_ring.left = 0;
	param->roi_c_dst.frame_ring.top = 0;
	param->roi_c_dst.frame_ring.right = 0;
	param->roi_c_dst.frame_ring.bottom = 0;
	param->roi_c_dst.tile_asym_mode = 0;
	param->roi_c_dst.tile_rand_mode = 0;
	param->roi_c_dst.tile_manual_mode = 0;
	param->roi_c_dst.m_tile_mode_x = 0;
	param->roi_c_dst.m_tile_mode_y = 0;
	param->roi_c_dst.m_tile_pos_x = 0;
	param->roi_c_dst.m_tile_pos_y = 0;
	param->roi_c_dst.addr = hw->image_c_addr.addr;
	param->roi_c_dst.stride = ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->roi_c_dst.line_skip = 0;
	param->roi_c_dst.line_valid = 0;
	param->roi_c_dst.repeat = 0;
	param->roi_c_dst.ppc = PPC_2;
	param->roi_c_dst.bpp = 8;
	param->roi_c_dst.fifo_addr = 64;
	param->roi_c_dst.fifo_size = 64;

	/* WDMA2 Parameters - temporal luma ROI even */
	param->roi_y_dst_even.pos_x = 0;
	param->roi_y_dst_even.pos_y = 0;
	param->roi_y_dst_even.width = hw->roi_y.right - hw->roi_y.left + 24;
	param->roi_y_dst_even.height = (hw->roi_y.bottom - hw->roi_y.top + 24) / 2; //half
	param->roi_y_dst_even.tile_w = param->roi_y_dst_even.width;
	param->roi_y_dst_even.tile_h = param->roi_y_dst_even.height;
	param->roi_y_dst_even.tile_x_num = 1;
	param->roi_y_dst_even.tile_y_num = 1;
	param->roi_y_dst_even.tot_ring.left = 0;
	param->roi_y_dst_even.tot_ring.top = 0;
	param->roi_y_dst_even.tot_ring.right = 0;
	param->roi_y_dst_even.tot_ring.bottom = 0;
	param->roi_y_dst_even.frame_ring.left = 0;
	param->roi_y_dst_even.frame_ring.top = 0;
	param->roi_y_dst_even.frame_ring.right = 0;
	param->roi_y_dst_even.frame_ring.bottom = 0;
	param->roi_y_dst_even.tile_asym_mode = 0;
	param->roi_y_dst_even.tile_rand_mode = 0;
	param->roi_y_dst_even.tile_manual_mode = 0;
	param->roi_y_dst_even.m_tile_mode_x = 0;
	param->roi_y_dst_even.m_tile_mode_y = 0;
	param->roi_y_dst_even.m_tile_pos_x = 0;
	param->roi_y_dst_even.m_tile_pos_y = 0;
	param->roi_y_dst_even.addr = hw->roi_y_addr.addr;
	param->roi_y_dst_even.stride = ALIGN(param->roi_y_dst_even.width, STR_MEM_STRIDE) * 2;
	param->roi_y_dst_even.line_skip = 0;
	param->roi_y_dst_even.line_valid = 0;
	param->roi_y_dst_even.repeat = 0;
	param->roi_y_dst_even.ppc = PPC_2;
	param->roi_y_dst_even.bpp = 8;
	param->roi_y_dst_even.fifo_addr = 128;
	param->roi_y_dst_even.fifo_size = 64;

	/* WDMA3 Parameters - temporal luma ROI odd */
	param->roi_y_dst_odd.pos_x = 0;
	param->roi_y_dst_odd.pos_y = 0;
	param->roi_y_dst_odd.width = hw->roi_y.right - hw->roi_y.left + 24;
	param->roi_y_dst_odd.height = (hw->roi_y.bottom - hw->roi_y.top + 24) / 2; //half
	param->roi_y_dst_odd.tile_w = param->roi_y_dst_odd.width;
	param->roi_y_dst_odd.tile_h = param->roi_y_dst_odd.height;
	param->roi_y_dst_odd.tile_x_num = 1;
	param->roi_y_dst_odd.tile_y_num = 1;
	param->roi_y_dst_odd.tot_ring.left = 0;
	param->roi_y_dst_odd.tot_ring.top = 0;
	param->roi_y_dst_odd.tot_ring.right = 0;
	param->roi_y_dst_odd.tot_ring.bottom = 0;
	param->roi_y_dst_odd.frame_ring.left = 0;
	param->roi_y_dst_odd.frame_ring.top = 0;
	param->roi_y_dst_odd.frame_ring.right = 0;
	param->roi_y_dst_odd.frame_ring.bottom = 0;
	param->roi_y_dst_odd.tile_asym_mode = 0;
	param->roi_y_dst_odd.tile_rand_mode = 0;
	param->roi_y_dst_odd.tile_manual_mode = 0;
	param->roi_y_dst_odd.m_tile_mode_x = 0;
	param->roi_y_dst_odd.m_tile_mode_y = 0;
	param->roi_y_dst_odd.m_tile_pos_x = 0;
	param->roi_y_dst_odd.m_tile_pos_y = 0;
	param->roi_y_dst_odd.addr = hw->roi_y_addr.addr + param->roi_y_dst_even.stride;
	param->roi_y_dst_odd.stride = ALIGN(param->roi_y_dst_odd.width, STR_MEM_STRIDE) * 2;
	param->roi_y_dst_odd.line_skip = 0;
	param->roi_y_dst_odd.line_valid = 0;
	param->roi_y_dst_odd.repeat = 0;
	param->roi_y_dst_odd.ppc = PPC_2;
	param->roi_y_dst_odd.bpp = 8;
	param->roi_y_dst_odd.fifo_addr = 192;
	param->roi_y_dst_odd.fifo_size = 64;
}

void str_hw_strd_update_param(struct str_hw *hw)
{
	struct strd_params *param = &hw->strd_param;
	u8 tile_num;
	u32 tile_width;

	str_cal_get_tile_size((hw->roi_y.right - hw->roi_y.left + 1), &tile_num, &tile_width);

	/* PU Control Parameters */
	param->enable_denoise = ENABLE_DENOISE;
	param->kern_size = BLUR_KERNEL_SIZE;
	param->no_center_pix = false;
	param->muldiv = MUL_DIVISION;
	param->non_edge = NON_EDGE_IN_HIGH;
	param->denoise = EDGE_IN_HIGH;
	param->blur = BLUR_ALPHA;
	param->inv_mul = INVERSE_MUL_K;
	param->size_cfg.width = hw->roi_y.right - hw->roi_y.left;
	param->size_cfg.height = hw->roi_y.bottom - hw->roi_y.top;
	param->size_cfg.tile_w = tile_width;
	param->size_cfg.tile_h = param->size_cfg.height;
	param->size_cfg.tile_x_num = tile_num;
	param->size_cfg.tile_y_num = 1;
	param->size_cfg.tot_ring.left = 12;
	param->size_cfg.tot_ring.top = 12;
	param->size_cfg.tot_ring.right = 12;
	param->size_cfg.tot_ring.bottom = 12;
	param->size_cfg.tile_asym_mode = 0;
	param->size_cfg.tile_manual_mode = 0;
	param->size_cfg.m_tile_mode_x = 0;
	param->size_cfg.m_tile_mode_y = 0;
	param->size_cfg.m_tile_pos_x = 0;
	param->size_cfg.m_tile_pos_x = 0;

	/* RDMA0 Parameters - chroma ROI mask */
	param->roi_mask_c.pos_x = 6;
	param->roi_mask_c.pos_y = 6;
	param->roi_mask_c.width = hw->roi_c.right - hw->roi_c.left;
	param->roi_mask_c.height = hw->roi_c.bottom - hw->roi_c.top;
	param->roi_mask_c.tile_w = tile_width / 2;
	param->roi_mask_c.tile_h = param->roi_mask_c.height;
	param->roi_mask_c.tile_x_num = tile_num;
	param->roi_mask_c.tile_y_num = 1;
	param->roi_mask_c.tot_ring.left = 6;
	param->roi_mask_c.tot_ring.top = 6;
	param->roi_mask_c.tot_ring.right = 6;
	param->roi_mask_c.tot_ring.bottom = 6;
	param->roi_mask_c.frame_ring.left = 0;
	param->roi_mask_c.frame_ring.top = 0;
	param->roi_mask_c.frame_ring.right = 0;
	param->roi_mask_c.frame_ring.bottom = 0;
	param->roi_mask_c.tile_asym_mode = 0;
	param->roi_mask_c.tile_rand_mode = 0;
	param->roi_mask_c.tile_manual_mode = 0;
	param->roi_mask_c.m_tile_mode_x = 0;
	param->roi_mask_c.m_tile_mode_y = 0;
	param->roi_mask_c.m_tile_pos_x = 0;
	param->roi_mask_c.m_tile_pos_y = 0;
	param->roi_mask_c.addr = hw->roi_c_addr.addr;
	param->roi_mask_c.stride = hw->strc_param.roi_mask_c.stride;
	param->roi_mask_c.line_skip = 0;
	param->roi_mask_c.line_valid = 0;
	param->roi_mask_c.repeat = 0;
	param->roi_mask_c.ppc = PPC_2;
	param->roi_mask_c.bpp = 8;
	param->roi_mask_c.fifo_addr = 0;
	param->roi_mask_c.fifo_size = 64;

	/* RDMA1 Parameters - temporal luma ROI */
	param->roi_y_src.pos_x = 12;
	param->roi_y_src.pos_y = 12;
	param->roi_y_src.width = hw->roi_y.right - hw->roi_y.left;
	param->roi_y_src.height = hw->roi_y.bottom - hw->roi_y.top;
	param->roi_y_src.tile_w = tile_width;
	param->roi_y_src.tile_h = param->roi_y_src.height;
	param->roi_y_src.tile_x_num = tile_num;
	param->roi_y_src.tile_y_num = 1;
	param->roi_y_src.tot_ring.left = 12;
	param->roi_y_src.tot_ring.top = 12;
	param->roi_y_src.tot_ring.right = 12;
	param->roi_y_src.tot_ring.bottom = 12;
	param->roi_y_src.frame_ring.left = 1;
	param->roi_y_src.frame_ring.top = 1;
	param->roi_y_src.frame_ring.right = 1;
	param->roi_y_src.frame_ring.bottom = 1;
	param->roi_y_src.tile_asym_mode = 0;
	param->roi_y_src.tile_rand_mode = 0;
	param->roi_y_src.tile_manual_mode = 0;
	param->roi_y_src.m_tile_mode_x = 0;
	param->roi_y_src.m_tile_mode_y = 0;
	param->roi_y_src.m_tile_pos_x = 0;
	param->roi_y_src.m_tile_pos_y = 0;
	param->roi_y_src.addr = hw->roi_y_addr.addr;
	param->roi_y_src.stride = ALIGN((param->roi_y_src.width + 24), STR_MEM_STRIDE);
	param->roi_y_src.line_skip = 0;
	param->roi_y_src.line_valid = 0;
	param->roi_y_src.repeat = 0;
	param->roi_y_src.ppc = PPC_4;
	param->roi_y_src.bpp = 8;
	param->roi_y_src.fifo_addr = 64;
	param->roi_y_src.fifo_size = 64;

	/* WDMA0 Parameters - luma ROI */
	param->roi_y_dst.pos_x = hw->roi_y.left;
	param->roi_y_dst.pos_y = hw->roi_y.top;
	param->roi_y_dst.width = hw->roi_y.right - hw->roi_y.left;
	param->roi_y_dst.height = hw->roi_y.bottom - hw->roi_y.top;
	param->roi_y_dst.tile_w = param->roi_y_src.tile_w;
	param->roi_y_dst.tile_h = param->roi_y_src.height;
	param->roi_y_dst.tile_x_num = tile_num;
	param->roi_y_dst.tile_y_num = 1;
	param->roi_y_dst.tot_ring.left = 0;
	param->roi_y_dst.tot_ring.top = 0;
	param->roi_y_dst.tot_ring.right = 0;
	param->roi_y_dst.tot_ring.bottom = 0;
	param->roi_y_dst.frame_ring.left = 0;
	param->roi_y_dst.frame_ring.top = 0;
	param->roi_y_dst.frame_ring.right = 0;
	param->roi_y_dst.frame_ring.bottom = 0;
	param->roi_y_dst.tile_asym_mode = 0;
	param->roi_y_dst.tile_rand_mode = 0;
	param->roi_y_dst.tile_manual_mode = 0;
	param->roi_y_dst.m_tile_mode_x = 0;
	param->roi_y_dst.m_tile_mode_y = 0;
	param->roi_y_dst.m_tile_pos_x = 0;
	param->roi_y_dst.m_tile_pos_y = 0;
	param->roi_y_dst.addr = hw->image_y_addr.addr;
	param->roi_y_dst.stride = ALIGN(hw->image_width, STR_MEM_STRIDE);
	param->roi_y_dst.line_skip = 0;
	param->roi_y_dst.line_valid = 0;
	param->roi_y_dst.repeat = 0;
	param->roi_y_dst.ppc = PPC_4;
	param->roi_y_dst.bpp = 8;
	param->roi_y_dst.fifo_addr = 0;
	param->roi_y_dst.fifo_size = 64;
}

void str_hw_set_cmdq_mode(struct str_hw *hw, bool enable)
{
	pr_info("[STR:HW][%s]enable(%d)\n", __func__, enable);

	if (enable)
		str_sfr_sw_all_reset(hw->regs);

	str_sfr_cmdq_set_mode(hw->regs, enable);
}

void str_hw_stra_dxi_mux_cfg(struct str_hw *hw)
{
	str_sfr_dxi_mux_sel(hw->regs, PU_STRA);
}

void str_hw_strb_dxi_mux_cfg(struct str_hw *hw)
{
	str_sfr_dxi_mux_sel(hw->regs, PU_STRB);
}

void str_hw_strc_dxi_mux_cfg(struct str_hw *hw)
{
	str_sfr_dxi_mux_sel(hw->regs, PU_STRC);
}

void str_hw_strd_dxi_mux_cfg(struct str_hw *hw)
{
	str_sfr_dxi_mux_sel(hw->regs, PU_STRD);
}

void str_hw_stra_dma_cfg(struct str_hw *hw)
{
	/* RDMA0 - chroma face region */
	str_sfr_dma_set_cfg(hw->regs, RDMA00, &hw->stra_param.face_c);

	/* WDMA0 - distance map */
	str_sfr_dma_set_cfg(hw->regs, WDMA00, &hw->stra_param.dist_map);

	str_sfr_top_roi_evt_enable(hw->regs, PU_STRA);
}

void str_hw_strb_dma_cfg(struct str_hw *hw)
{
	/* RDMA0 - distance map */
	str_sfr_dma_set_cfg(hw->regs, RDMA00, &hw->strb_param.dist_map);

	/* RDMA1 - chroma face region */
	str_sfr_dma_set_cfg(hw->regs, RDMA01, &hw->strb_param.face_c);

	/* RDMA2 - luma face region */
	str_sfr_dma_set_cfg(hw->regs, RDMA02, &hw->strb_param.face_y);

	str_sfr_top_roi_evt_enable(hw->regs, PU_STRB);
}

void str_hw_strc_dma_cfg(struct str_hw *hw)
{
	/* RDMA0 - chroma ROI */
	str_sfr_dma_set_cfg(hw->regs, RDMA00, &hw->strc_param.roi_c_src);

	/* RDMA1 - luma ROI even */
	str_sfr_dma_set_cfg(hw->regs, RDMA01, &hw->strc_param.roi_y_src_even);

	/* RDMA2 - luma ROI odd */
	str_sfr_dma_set_cfg(hw->regs, RDMA02, &hw->strc_param.roi_y_src_odd);

	/* WDMA0 - Chroma ROI Mask */
	str_sfr_dma_set_cfg(hw->regs, WDMA00, &hw->strc_param.roi_mask_c);

	/* WDMA1 - Chroma ROI */
	str_sfr_dma_set_cfg(hw->regs, WDMA01, &hw->strc_param.roi_c_dst);

	/* WDMA2 - temporal luma ROI even */
	str_sfr_dma_set_cfg(hw->regs, WDMA02, &hw->strc_param.roi_y_dst_even);

	/* WDMA3 - temporal luma ROI odd */
	str_sfr_dma_set_cfg(hw->regs, WDMA03, &hw->strc_param.roi_y_dst_odd);

	str_sfr_top_roi_evt_enable(hw->regs, PU_STRC);
}

void str_hw_strd_dma_cfg(struct str_hw *hw)
{
	/* RDMA0 - chroma ROI mask */
	str_sfr_dma_set_cfg(hw->regs, RDMA00, &hw->strd_param.roi_mask_c);

	/* RDMA1 - temporal luma ROI */
	str_sfr_dma_set_cfg(hw->regs, RDMA01, &hw->strd_param.roi_y_src);

	/* WDMA0 - luma ROI */
	str_sfr_dma_set_cfg(hw->regs, WDMA00, &hw->strd_param.roi_y_dst);

	str_sfr_top_roi_evt_enable(hw->regs, PU_STRD);
}

void str_hw_frame_done_irq_enable(struct str_hw *hw)
{
	str_sfr_top_pu_tile_done_intr_enable(hw->regs, PU_STRA);
	str_sfr_top_pu_tile_done_intr_enable(hw->regs, PU_STRB);
	str_sfr_top_pu_tile_done_intr_enable(hw->regs, PU_STRC);
	str_sfr_top_pu_tile_done_intr_enable(hw->regs, PU_STRD);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, PU_TILE);

	str_sfr_top_pu_frame_done_intr_enable(hw->regs, PU_STRA);
	str_sfr_top_pu_frame_done_intr_enable(hw->regs, PU_STRB);
	str_sfr_top_pu_frame_done_intr_enable(hw->regs, PU_STRC);
	str_sfr_top_pu_frame_done_intr_enable(hw->regs, PU_STRD);
	str_sfr_top_pu_frame_done_intr_enable(hw->regs, CMDQ);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, PU_FRAME);

	str_sfr_top_pu_err_done_intr_enable(hw->regs, PU_STRA);
	str_sfr_top_pu_err_done_intr_enable(hw->regs, PU_STRB);
	str_sfr_top_pu_err_done_intr_enable(hw->regs, PU_STRC);
	str_sfr_top_pu_err_done_intr_enable(hw->regs, PU_STRD);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, PU_ERROR);

	str_sfr_top_dma_tile_done_intr_enable(hw->regs, RDMA00);
	str_sfr_top_dma_tile_done_intr_enable(hw->regs, RDMA01);
	str_sfr_top_dma_tile_done_intr_enable(hw->regs, RDMA02);
	str_sfr_top_dma_tile_done_intr_enable(hw->regs, WDMA00);
	str_sfr_top_dma_tile_done_intr_enable(hw->regs, WDMA01);
	str_sfr_top_dma_tile_done_intr_enable(hw->regs, WDMA02);
	str_sfr_top_dma_tile_done_intr_enable(hw->regs, WDMA03);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, DMA_TILE);

	str_sfr_top_dma_frame_done_intr_enable(hw->regs, RDMA00);
	str_sfr_top_dma_frame_done_intr_enable(hw->regs, RDMA01);
	str_sfr_top_dma_frame_done_intr_enable(hw->regs, RDMA02);
	str_sfr_top_dma_frame_done_intr_enable(hw->regs, WDMA00);
	str_sfr_top_dma_frame_done_intr_enable(hw->regs, WDMA01);
	str_sfr_top_dma_frame_done_intr_enable(hw->regs, WDMA02);
	str_sfr_top_dma_frame_done_intr_enable(hw->regs, WDMA03);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, DMA_FRAME);

	str_sfr_top_dma_err_done_intr_enable(hw->regs, RDMA00);
	str_sfr_top_dma_err_done_intr_enable(hw->regs, RDMA01);
	str_sfr_top_dma_err_done_intr_enable(hw->regs, RDMA02);
	str_sfr_top_dma_err_done_intr_enable(hw->regs, WDMA00);
	str_sfr_top_dma_err_done_intr_enable(hw->regs, WDMA01);
	str_sfr_top_dma_err_done_intr_enable(hw->regs, WDMA02);
	str_sfr_top_dma_err_done_intr_enable(hw->regs, WDMA03);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, DMA_ERROR);

	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, RDMA00);
	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, RDMA01);
	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, RDMA02);
	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, WDMA00);
	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, WDMA01);
	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, WDMA02);
	str_sfr_top_dxi_tile_done_intr_enable(hw->regs, WDMA03);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, DXI_TILE);

	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, RDMA00);
	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, RDMA01);
	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, RDMA02);
	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, WDMA00);
	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, WDMA01);
	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, WDMA02);
	str_sfr_top_dxi_frame_done_intr_enable(hw->regs, WDMA03);
	str_sfr_top_irq_enable(hw->regs, 0 /* IRQ 0 */, DXI_FRAME);
}

void str_hw_stra_ctrl_cfg(struct str_hw *hw)
{
	str_sfr_pu_set_cfg(hw->regs, PU_STRA, &hw->stra_param.size_cfg);
	str_sfr_stra_set_params(hw->regs, &hw->stra_param);
	str_sfr_pu_start(hw->regs, PU_STRA);
	str_sfr_pu_dma_start(hw->regs, PU_STRA);

	/* Finish to set SFR of the current PU */
	str_sfr_cmdq_set_frm_marker(hw->regs);
}

void str_hw_strb_ctrl_cfg(struct str_hw *hw)
{
	str_sfr_pu_set_cfg(hw->regs, PU_STRB, &hw->strb_param.size_cfg);
	str_sfr_strb_set_params(hw->regs, &hw->strb_param);
	str_sfr_pu_start(hw->regs, PU_STRB);
	str_sfr_pu_dma_start(hw->regs, PU_STRB);

	/* Finish to set SFR of the current PU */
	str_sfr_cmdq_set_frm_marker(hw->regs);
}

void str_hw_strc_ctrl_cfg(struct str_hw *hw)
{
	str_sfr_pu_set_cfg(hw->regs, PU_STRC, &hw->strc_param.size_cfg);
	str_sfr_strc_set_params(hw->regs, &hw->strc_param);
	str_sfr_pu_start(hw->regs, PU_STRC);
	str_sfr_pu_dma_start(hw->regs, PU_STRC);

	/* Finish to set SFR of the current PU */
	str_sfr_cmdq_set_frm_marker(hw->regs);
}

void str_hw_strd_ctrl_cfg(struct str_hw *hw)
{
	str_sfr_pu_set_cfg(hw->regs, PU_STRD, &hw->strd_param.size_cfg);
	str_sfr_strd_set_params(hw->regs, &hw->strd_param);
	str_sfr_pu_start(hw->regs, PU_STRD);
	str_sfr_pu_dma_start(hw->regs, PU_STRD);

	/* Finish to set SFR of the current PU */
	str_sfr_cmdq_set_frm_marker(hw->regs);
}

void str_hw_strb_set_second_mode(struct str_hw *hw)
{
	str_sfr_strb_rerun(hw->regs);

	/* Finish to set SFR of the current PU */
	str_sfr_cmdq_set_frm_marker(hw->regs);
}

void str_hw_str_start(struct str_hw *hw, u8 face_num)
{
	str_sfr_cmdq_set_run_cnt(hw->regs, face_num);
}

void str_hw_update_irq_status(struct str_hw *hw)
{
	hw->status.pu_tile_done = str_sfr_top_get_pu_tile_done_intr_status_clear(hw->regs);
	hw->status.pu_frame_done = str_sfr_top_get_frame_done_intr_status_clear(hw->regs);
	hw->status.pu_err_done = str_sfr_top_get_pu_err_done_intr_status_clear(hw->regs);
	hw->status.dma_tile_done = str_sfr_top_get_dma_tile_done_intr_status_clear(hw->regs);
	hw->status.dma_frame_done = str_sfr_top_get_dma_frame_done_intr_status_clear(hw->regs);
	hw->status.dma_err_done = str_sfr_top_get_dma_err_done_intr_status_clear(hw->regs);
	if (hw->status.dma_err_done) {
		hw->status.dma_err_status.rdma00 = (hw->status.dma_err_done & (1 << 0)) ? str_sfr_dma_err_clear(hw->regs, RDMA00) : 0;
		hw->status.dma_err_status.rdma01 = (hw->status.dma_err_done & (1 << 1)) ? str_sfr_dma_err_clear(hw->regs, RDMA01) : 0;
		hw->status.dma_err_status.rdma02 = (hw->status.dma_err_done & (1 << 2)) ? str_sfr_dma_err_clear(hw->regs, RDMA02) : 0;
		hw->status.dma_err_status.wdma00 = (hw->status.dma_err_done & (1 << 16)) ? str_sfr_dma_err_clear(hw->regs, WDMA00) : 0;
		hw->status.dma_err_status.wdma01 = (hw->status.dma_err_done & (1 << 17)) ? str_sfr_dma_err_clear(hw->regs, WDMA01) : 0;
		hw->status.dma_err_status.wdma02 = (hw->status.dma_err_done & (1 << 18)) ? str_sfr_dma_err_clear(hw->regs, WDMA02) : 0;
		hw->status.dma_err_status.wdma03 = (hw->status.dma_err_done & (1 << 19)) ? str_sfr_dma_err_clear(hw->regs, WDMA03) : 0;
	}
	hw->status.dxi_tile_done = str_sfr_top_get_dxi_tile_done_intr_status_clear(hw->regs);
	hw->status.dxi_frame_done = str_sfr_top_get_dxi_frame_done_intr_status_clear(hw->regs);
}

void str_hw_update_version(struct str_hw *hw)
{
	hw->reg_ver = str_sfr_get_reg_ver(hw->regs);
	hw->str_ver = str_sfr_get_str_ver(hw->regs);
}
