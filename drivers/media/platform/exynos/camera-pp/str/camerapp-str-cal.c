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

#include "camerapp-str-param_v100.h"
#include "camerapp-str-hw.h"
#include "camerapp-str-cal.h"

void str_cal_adjust_face_region(struct str_region *face)
{
	u32 width, height;
	u32 mv_x, mv_y;

	if (!face) {
		pr_err("[STR:CAL][%s]face is NULL.\n", __func__);
		return;
	}

	width = face->right - face->left + 1;
	height = face->bottom - face->top + 1;

	/* To do center adjustment */
	mv_x = (width % STR_REGION_SIZE_ALIGN) >> 1;
	mv_y = (height % STR_REGION_SIZE_ALIGN) >> 1;

	face->left = ALIGN_DOWN((face->left + mv_x), 2);
	face->top = ALIGN_DOWN((face->top + mv_y), 2);
	face->right = face->left + ALIGN_DOWN(width, STR_REGION_SIZE_ALIGN);
	face->bottom = face->top + ALIGN_DOWN(height, STR_REGION_SIZE_ALIGN);

	pr_info("[STR:CAL][%s]region(%d,%d-%d,%d)\n", __func__,
			face->left, face->top, face->right, face->bottom);
}

void str_cal_get_face_c_region(struct str_region *face_y, struct str_region *face_c)
{
	u32 width, height;

	if (!face_y || !face_c) {
		pr_err("[STR:CAL][%s]face is NULL. face_y(%p), face_c(%p)\n",
				__func__, face_y, face_c);
		return;
	}

	width = (face_y->right - face_y->left + 1) >> 1;
	height = (face_y->bottom - face_y->top + 1) >> 1;

	face_c->left = face_y->left >> 1;
	face_c->top = face_y->top >> 1;
	face_c->right = face_c->left + width - 1;
	face_c->bottom = face_c->top + height - 1;

	pr_info("[STR:CAL][%s]region_c(%d,%d-%d,%d)\n", __func__,
			face_c->left, face_c->top, face_c->right, face_c->bottom);
}

void str_cal_get_roi(struct str_region *face, enum direction_id mouth_pos,
		u32 image_w, u32 image_h, struct str_region *roi)
{
	u32 width, height;
	int del_w, del_h;
	int w_ratio, h_ratio;
	u32 roi_left, roi_top, roi_right, roi_bottom;
	u32 mv_x, mv_y;

	if (!face || !roi) {
		pr_err("[STR:CAL][%s]face(%p) roi(%p) is NULL.\n", __func__, face, roi);
		return;
	}

	width = face->right - face->left + 1;
	height = face->bottom - face->top + 1;

	switch (mouth_pos) {
	case HOR_DOWN:
	case HOR_UP:
		w_ratio = STR_WIDTH_DELTA_RATIO;
		h_ratio = STR_HEIGHT_DELTA_RATIO;
		break;
	case VER_LEFT:
	case VER_RIGHT:
		/* 90 degree rotation */
		w_ratio = STR_HEIGHT_DELTA_RATIO;
		h_ratio = STR_WIDTH_DELTA_RATIO;
		break;
	default:
		pr_warn("[STR:CAL][%s]Unsupported mouth position(%d)\n", __func__, mouth_pos);
		w_ratio = STR_WIDTH_DELTA_RATIO;
		h_ratio = STR_HEIGHT_DELTA_RATIO;
		break;
	}

	del_w = (width * w_ratio + (1 << (BIT_10 - 1))) >> BIT_10;
	del_h = (height * h_ratio + (1 << (BIT_10 - 1))) >> BIT_10;

	/* Add delta to each boundary of face region */
	roi_left = face->left - del_w;
	roi_top = face->top - del_h;
	roi_right = face->left + width + del_w;
	roi_bottom = face->top + height + del_h;

	/* Adjust boundary margin of STR RING */
	roi_left = (roi_left < STR_RING_SIZE) ? STR_RING_SIZE : roi_left;
	roi_top = (roi_top < STR_RING_SIZE) ? STR_RING_SIZE : roi_top;
	roi_right = (roi_right > image_w - 1 - STR_RING_SIZE) ? (image_w - 1 - STR_RING_SIZE) : roi_right;
	roi_bottom = (roi_bottom> image_h - 1 - STR_RING_SIZE) ? (image_h - 1 - STR_RING_SIZE) : roi_bottom;

	/* Adjust STR region size align */
	width = roi_right - roi_left + 1;
	height = roi_bottom - roi_top + 1;

	/* To do centor adjustment */
	mv_x = (width % STR_REGION_SIZE_ALIGN) >> 1;
	mv_y = (height % STR_REGION_SIZE_ALIGN) >> 1;

	roi->left = ALIGN_DOWN((roi_left + mv_x), 2);
	roi->top = ALIGN_DOWN((roi_top + mv_y), 2);
	roi->right = roi_left + ALIGN_DOWN(width, STR_REGION_SIZE_ALIGN);
	roi->bottom = roi_top + ALIGN_DOWN(height, STR_REGION_SIZE_ALIGN);

	pr_info("[STR:CAL][%s]region(%d,%d-%d,%d)\n", __func__,
			roi->left, roi->top, roi->right, roi->bottom);
}

void str_cal_get_roi_c_region(struct str_region *roi_y, struct str_region *roi_c)
{
	u32 width, height;

	if (!roi_y || !roi_c) {
		pr_err("[STR:CAL][%s]ROI is NULL. roi_y(%p), roi_c(%p)\n",
				__func__, roi_y, roi_c);
		return;
	}

	width = (roi_y->right - roi_y->left + 1) >> 1;
	height = (roi_y->bottom - roi_y->top + 1) >> 1;

	roi_c->left = roi_y->left >> 1;
	roi_c->top = roi_y->top >> 1;
	roi_c->right = roi_c->left + width - 1;
	roi_c->bottom = roi_c->top + height - 1;

	pr_info("[STR:CAL][%s]region_c(%d,%d-%d,%d)\n", __func__,
			roi_c->left, roi_c->top, roi_c->right, roi_c->bottom);

}

void str_cal_get_inversed_face_size(struct str_region *face, enum direction_id mouth_pos, u32 *inv_x, u32 *inv_y)
{
	u32 width, height;
	float ratio_x, ratio_y;

	if (!face || !inv_x || !inv_y) {
		pr_err("[STR:CAL][%s]face(%p), inv_x(%p), inv_y(%p) is NULL.\n",
				__func__, face, inv_x, inv_y);
		return;
	}

	width = face->right - face->left + 1;
	height = face->bottom - face->top + 1;

	switch (mouth_pos) {
	case HOR_DOWN:
	case HOR_UP:
		ratio_x = STR_INVERSE_RATIO_Y;
		ratio_y = STR_INVERSE_RATIO_X;
		break;
	case VER_LEFT:
	case VER_RIGHT:
		ratio_x = STR_INVERSE_RATIO_X;
		ratio_y = STR_INVERSE_RATIO_Y;
		break;
	default:
		pr_warn("[STR:CAL][%s]Unsupported mouth position(%d)\n", __func__, mouth_pos);
		ratio_x = STR_INVERSE_RATIO_Y;
		ratio_y = STR_INVERSE_RATIO_X;
		break;
	}

	*inv_x = FLOAT_TO_INT((1.0f / (2 * width * ratio_x)), BIT_20);
	*inv_y = FLOAT_TO_INT((1.0f / (2 * height * ratio_y)), BIT_20);

	pr_info("[STR:CAL][%s]inverse(%d,%d)\n", __func__, *inv_x, *inv_y);
}

void str_cal_get_tile_size(u32 width, u8 *tile_num, u32 *tile_width)
{
	if (!tile_num || !tile_width) {
		pr_err("[STR:CAL][%s]tile_num(%p), tile_width(%p) is NULL.\n",
				__func__, tile_num, tile_width);
		return;
	}

	*tile_num = (width + STR_MAX_TILE_WIDTH - 1) / STR_MAX_TILE_WIDTH;
	*tile_width = width / *tile_num;

	pr_info("[STR:CAL][%s]tile_num(%d), tile_width(%d)\n", __func__, *tile_num, *tile_width);
}
