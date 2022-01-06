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

struct exynos_color_aspects_legacy {
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

struct exynos_hdr_static_info_legacy {
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

struct exynos_video_dec_data_legacy {
	int ninterlaced_type;
};

struct exynos_video_enc_data_legacy {
	struct exynos_video_ysum_data sysum_data;
	unsigned long long p_roi_data;	/* for fixing byte alignemnt on 64x32 problem */
	int n_use_gdc_otf;
	int n_is_gdc_otf;
};

struct exynoshdrdynamicinfo_legacy {
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

struct exynos_video_meta_legacy {
	enum exynos_video_info_type etype;
	/* common */
	struct exynos_hdr_static_info_legacy shdr_static_info;
	struct exynos_color_aspects_legacy	scolor_aspects;
	union {
		struct exynos_video_dec_data_legacy dec;
		struct exynos_video_enc_data_legacy enc;
	} data;
	struct exynoshdrdynamicinfo_legacy shdrdynamicinfo;
};

/************************************/
/*** HDR meta updated to new form ***/
/************************************/

struct exynos_color_aspects {
	int mrange;
	int mprimaries;
	int mtransfer;
	int mmatrix_coeffs;
};

struct exynos_hdr_static_info {
	int mid;
	union {
		struct exynos_type1 stype1;
	};
};

struct exynos_video_dec_data {
	int ninterlaced_type;
};

typedef struct _GDCRect {
	int x;
	int y;
	int w;
	int h;
} GDCRect;

typedef struct _GDCCropInfo {
	GDCRect rect;
	int fullW;
	int fullH;
} GDCCropInfo;

#define GDC_GRID_MAX_HEIGHT 33
#define GDC_GRID_MAX_WIDTH 33

typedef struct {
	int32_t gridX[GDC_GRID_MAX_HEIGHT * GDC_GRID_MAX_WIDTH];
	int32_t gridY[GDC_GRID_MAX_HEIGHT * GDC_GRID_MAX_WIDTH];
	uint32_t width;
	uint32_t height;
} GDCGridTable;

typedef struct {
	GDCCropInfo cropInfo;
	GDCGridTable gridTable;

	int is_grid_mode;
	int is_bypass_mode;
	int index;
} GDCInfo;

struct exynos_video_enc_data {
	struct exynos_video_ysum_data sysum_data;
	unsigned long long pRoiData;  /* for fixing byte alignemnt on 64x32 problem */
	int nUseGdcOTF;
	int nIsGdcOTF;
	GDCInfo sGDCInfo;
};

typedef struct _ExynosHdrData_ST2094_40 {
	unsigned char  country_code;
	unsigned short provider_code;
	unsigned short provider_oriented_code;

	unsigned char  application_identifier;
	unsigned char  application_version;

	unsigned char num_windows;

	unsigned short window_upper_left_corner_x[2];
	unsigned short window_upper_left_corner_y[2];
	unsigned short window_lower_right_corner_x[2];
	unsigned short window_lower_right_corner_y[2];
	unsigned short center_of_ellipse_x[2];
	unsigned short center_of_ellipse_y[2];
	unsigned char  rotation_angle[2];
	unsigned short semimajor_axis_internal_ellipse[2];
	unsigned short semimajor_axis_external_ellipse[2];
	unsigned short semiminor_axis_external_ellipse[2];
	unsigned char  overlap_process_option[2];

	unsigned int  targeted_system_display_maximum_luminance;
	unsigned char targeted_system_display_actual_peak_luminance_flag;
	unsigned char num_rows_targeted_system_display_actual_peak_luminance;
	unsigned char num_cols_targeted_system_display_actual_peak_luminance;
	unsigned char targeted_system_display_actual_peak_luminance[25][25];

	unsigned int maxscl[3][3];
	unsigned int average_maxrgb[3];

	unsigned char num_maxrgb_percentiles[3];
	unsigned char maxrgb_percentages[3][15];
	unsigned int  maxrgb_percentiles[3][15];

	unsigned short fraction_bright_pixels[3];

	unsigned char mastering_display_actual_peak_luminance_flag;
	unsigned char num_rows_mastering_display_actual_peak_luminance;
	unsigned char num_cols_mastering_display_actual_peak_luminance;
	unsigned char mastering_display_actual_peak_luminance[25][25];

	struct {
		unsigned char   tone_mapping_flag[3];
		unsigned short  knee_point_x[3];
		unsigned short  knee_point_y[3];
		unsigned char   num_bezier_curve_anchors[3];
		unsigned short  bezier_curve_anchors[3][15];
	} tone_mapping;

	unsigned char color_saturation_mapping_flag[3];
	unsigned char color_saturation_weight[3];
} ExynosHdrData_ST2094_40;

typedef struct _ExynosHdrDynamicInfo {
	unsigned int valid;
	unsigned int reserved[42];

	ExynosHdrData_ST2094_40 data;

} ExynosHdrDynamicInfo;

struct exynos_video_meta {
	enum exynos_video_info_type etype;
	int nHDRType;
	/* common */
	struct exynos_color_aspects	scolor_aspects;
	struct exynos_hdr_static_info	shdr_static_info;

	ExynosHdrDynamicInfo shdrdynamicinfo;

	int nPixelFormat;

	union {
		struct exynos_video_dec_data dec;
		struct exynos_video_enc_data enc;
	} data;

	int reserved[20];   /* reserved filed for additional info */
};

#endif /* VENDOR_VIDEO_API_H_ */
