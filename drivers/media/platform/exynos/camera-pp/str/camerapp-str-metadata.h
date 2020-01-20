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

#ifndef CAMERAPP_STR_METADATA_H
#define CAMERAPP_STR_METADATA_H

#define MAX_FACE_NUM	8

enum camera_id {
	CAM_REAR,
	CAM_FRONT,
};

enum direction_id {
	HOR_DOWN,
	VER_LEFT,
	HOR_UP,
	VER_RIGHT
};

struct str_face_region {
	u32	left;
	u32	top;
	u32	right;
	u32	bottom;
};

struct str_face_region_array {
	struct str_face_region	faces[MAX_FACE_NUM];
	u32			length;
};

struct str_metadata {
	struct str_face_region_array	face_array;
	enum direction_id		device_orientation;
	enum camera_id			lens_position;
	u32				region_index;
	bool				is_preview;
};

#endif //CAMERAPP_STR_METADATA_H
