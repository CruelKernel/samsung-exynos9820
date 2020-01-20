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

#ifndef CAMERAPP_STR_CAL_H
#define CAMERAPP_STR_CAL_H

#include "camerapp-str-sfr-api-v100.h"
#include "camerapp-str-metadata.h"

void str_cal_adjust_face_region(struct str_region *face);
void str_cal_get_face_c_region(struct str_region *face_y, struct str_region *face_c);
void str_cal_get_roi(struct str_region *face, enum direction_id mouth_pos,
		u32 image_w, u32 image_h, struct str_region *roi);
void str_cal_get_roi_c_region(struct str_region *roi_y, struct str_region *roi_c);
void str_cal_get_inversed_face_size(struct str_region *face, enum direction_id mouth_pos, u32 *inv_x, u32 *inv_y);
void str_cal_get_tile_size(u32 width, u8 *tile_num, u32 *tile_width);

#endif //CAMERAPP_STR_CAL_H
