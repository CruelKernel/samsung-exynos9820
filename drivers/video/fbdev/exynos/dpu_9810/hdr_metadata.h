/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos HDR metadata
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef VENDOR_VIDEO_API_H_
#define VENDOR_VIDEO_API_H_

#include <linux/types.h>

enum exynos_video_info_type {
    VIDEO_INFO_TYPE_INVALID         = 0,
    VIDEO_INFO_TYPE_HDR_STATIC      = 0x1 << 0,
    VIDEO_INFO_TYPE_COLOR_ASPECTS   = 0x1 << 1,
    VIDEO_INFO_TYPE_INTERLACED      = 0x1 << 2,
    VIDEO_INFO_TYPE_YSUM_DATA       = 0x1 << 3,
};

struct exynos_video_ysum_data {
    unsigned int high;
    unsigned int low;
};

struct exynos_color_aspects {
    int mrange;
    int mprimaries;
    int mtransfer;
    int mmatrix_coeffs;
};

struct exynos_primaries {
    unsigned int x;
    unsigned int y;
};

struct exynos_type1 {
    struct exynos_primaries	mr;
    struct exynos_primaries	mg;
    struct exynos_primaries	mb;
    struct exynos_primaries	mw;
    unsigned int mmax_display_luminance;
    unsigned int mmin_display_luminance;
    unsigned int mmax_content_light_level;
    unsigned int mmax_frame_average_light_level;
};

struct exynos_hdr_static_info {
    int mid;
    union {
        struct exynos_type1	stype1;
    };
};

struct exynos_video_dec_data {
    struct exynos_hdr_static_info	shdr_static_info;
    struct exynos_color_aspects		scolor_aspects;
    int ninterlaced_type;
};

struct exynos_video_enc_data {
    struct exynos_hdr_static_info	shdr_static_info;
    struct exynos_color_aspects		scolor_aspects;
    struct exynos_video_ysum_data	sysum_data;
};

struct exynos_video_meta {
    enum exynos_video_info_type	etype;

    union {
        struct exynos_video_dec_data	dec;
        struct exynos_video_enc_data	enc;
    } data;
};

#endif /* VENDOR_VIDEO_API_H_ */
