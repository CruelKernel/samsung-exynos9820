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
	VIDEO_INFO_TYPE_INVALID			= 0,
	VIDEO_INFO_TYPE_HDR_STATIC		= 0x1 << 0,
	VIDEO_INFO_TYPE_COLOR_ASPECTS		= 0x1 << 1,
	VIDEO_INFO_TYPE_INTERLACED		= 0x1 << 2,
	VIDEO_INFO_TYPE_YSUM_DATA		= 0x1 << 3,
	VIDEO_INFO_TYPE_HDR_DYNAMIC		= 0x1 << 4,
	VIDEO_INFO_TYPE_CHECK_PIXEL_FORMAT	= 0x1 << 5,
	VIDEO_INFO_TYPE_GDC_OTF			= 0x1 << 6,
	VIDEO_INFO_TYPE_ROI_INFO		= 0x1 << 7,
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
		struct exynos_type1 stype1;
	};
};

#define MAX_ROIINFO_SIZE 32400
struct exynos_video_roi_data {
	int n_upper_qp_offset;
	int n_lower_qp_offset;
	int b_use_roi_info;
	int n_roi_mb_info_size;
	char p_roi_mb_info[MAX_ROIINFO_SIZE];
};

struct exynos_video_dec_data {
	int ninterlaced_type;
};

struct exynos_video_enc_data {
	struct exynos_video_ysum_data sysum_data;
	unsigned long long p_roi_data;	/* for fixing byte alignemnt on 64x32 problem */
	int n_use_gdc_otf;
	int n_is_gdc_otf;
};

struct exynoshdrdynamicinfo {
    unsigned int valid;

    struct {
        unsigned char  country_code;
        unsigned short provider_code;
        unsigned short provider_oriented_code;

        unsigned char  application_identifier;
        unsigned short application_version;

        unsigned int  display_maximum_luminance;

        unsigned int maxscl[3];

        unsigned char num_maxrgb_percentiles;
        unsigned char maxrgb_percentages[15];
        unsigned int  maxrgb_percentiles[15];

        struct tone_mapping {
            unsigned short  tone_mapping_flag;
            unsigned short  knee_point_x;
            unsigned short  knee_point_y;
            unsigned short  num_bezier_curve_anchors;
            unsigned short  bezier_curve_anchors[15];
        } tone;
    } data;

    unsigned int reserved[42];
};

struct exynos_video_meta {
	enum exynos_video_info_type etype;
	/* common */
	struct exynos_hdr_static_info shdr_static_info;
	struct exynos_color_aspects	scolor_aspects;
	union {
		struct exynos_video_dec_data dec;
		struct exynos_video_enc_data enc;
	} data;
	struct exynoshdrdynamicinfo shdrdynamicinfo;
};

#endif /* VENDOR_VIDEO_API_H_ */
