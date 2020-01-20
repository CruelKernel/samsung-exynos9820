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

#ifndef CAMERAPP_STR_PARAM_V100_H
#define CAMERAPP_STR_PARAM_V100_H

#include "camerapp-str-hw.h"

#define GROUP_NUM_REGION_0	(3)
#define GROUP_NUM_REGION_1	(1)
#define GROUP_NUM_REGION_2	(1)
#define GROUP_NUM_REGION_3	(1)
#define GROUP_NUM_REGION_4	(3)
#define GROUP_NUM_REGION_5	(3)
#define GROUP_NUM_REGION_6	(2)
#define GROUP_NUM_REGION_7	(3)

//For STRA
#define FACE_THRESHOLD		(FLOAT_TO_INT(35.0f, BIT_07))
#define NON_FACE_THRESHOLD	(FLOAT_TO_INT(15.4f, BIT_07))
#define SKIN_RATIO		(FLOAT_TO_INT(0.65f, BIT_14))

//For STRB
#define COVARIANCE_SCALER	(FLOAT_TO_INT(5.0f, BIT_07))
#define	MU_YS_VAL		(130.0f)
#define MU_YE_VAL		(70.0f)
#define MU_YS			(FLOAT_TO_INT(MU_YS_VAL, BIT_04))
#define MU_YE			(FLOAT_TO_INT(MU_YE_VAL, BIT_04))
#define INVERSE_MU_YSE		(FLOAT_TO_INT(1.0f / (MU_YS_VAL - MU_YE_VAL), BIT_16))
#define Y_INTERPOLATION		(2)
#define MUL_K_VAL		(0.4f)
#define MUL_K			(FLOAT_TO_INT(MUL_K_VAL, BIT_07))

//For STRC
#define CLIP_RATIO_VAL		(0.3f)
#define CLIP_RATIO		(FLOAT_TO_INT(CLIP_RATIO_VAL, BIT_10))
#define INVERSE_CLIP_RATIO	(FLOAT_TO_INT(1.0f / CLIP_RATIO_VAL, BIT_05))
#define MUL_MASK		(FLOAT_TO_INT(4.1f, BIT_10))
#define HW_RING_SIZE		(12)

//For STRD
#define ENABLE_DENOISE		(true)
#define BLUR_KERNEL_SIZE	(5)
#define MUL_DIVISION		(FLOAT_TO_UINT32(1.0f / (BLUR_KERNEL_SIZE * BLUR_KERNEL_SIZE - 1), BIT_20))
#define NON_EDGE_IN_HIGH	(1 << BIT_10) - 1;
#define EDGE_IN_HIGH		(128)
#define BLUR_ALPHA		(608)
#define INVERSE_MUL_K		(FLOAT_TO_UINT32(1.0f / MUL_K_VAL, BIT_07))

#define STR_MEM_STRIDE		(16)

//For S/W Calculation
#define STR_REGION_SIZE_ALIGN	(16)
#define STR_ROI_ASPECT_RATIO	(FLOAT_TO_INT(4.6f, BIT_10)) // height / width
#define STR_WIDTH_DELTA_RATIO	(FLOAT_TO_INT(0.13f, BIT_10))
#define STR_HEIGHT_DELTA_RATIO	((STR_WIDTH_DELTA_RATIO * STR_ROI_ASPECT_RATIO) >> BIT_10)
#define STR_RING_SIZE		(12)
#define STR_INVERSE_RATIO_X	(0.17f)
#define STR_INVERSE_RATIO_Y	(0.30f)
#define STR_MAX_TILE_WIDTH	(488)

static float ref_model_mean[REF_PARAM_CNT][2] = {
	{ 107.9361f, 156.5573f },
	{ 107.7817f, 152.3858f },
	{ 101.0027f, 162.6109f }
};

static float ref_model_inv_cov[REF_PARAM_CNT][2][2] = {
	{
		{ 0.0948f, 0.0893f },
		{ 0.0893f, 0.0979f }
	},
	{
		{ 0.0592f, 0.0423f },
		{ 0.0423f, 0.0677f }
	},
	{
		{ 0.0215f, 0.0171f },
		{ 0.0171f, 0.0602f }
	}
};

static float ref_model_weight[REF_PARAM_CNT] = {
	0.4645f,
	0.3330f,
	0.2025f
};

/*
 * Group Params: White
 */
static float grp_mean_h_white_all[3] = {
	111.6317f, 152.4871f, 169.8002f
};

static float grp_invcov_h_white_all[3][3] = {
	{ 0.0944f, 0.0767f, 0.0018f },
	{ 0.0767f, 0.0998f, 0.0035f },
	{ 0.0018f, 0.0035f, 0.0026f }
};

static u8 grp_pref_col_h_white_all[2][2] = {
	{ 106, 162 },
	{ 107, 161 }
};

#define GRP_DEL_Y_H_WHITE_ALL	40

/*
 * Group Params: USA (Hispanic)
 */
static float grp_mean_f_hispanic[3] = {
	109.0262f, 155.0612f, 153.8600f
};

static float grp_invcov_f_hispanic[3][3] = {
	{ 0.1444f, 0.1264f, 0.0046f },
	{ 0.1264f, 0.1527f, 0.0072f },
	{ 0.0046f, 0.0072f, 0.0024f }
};

static u8 grp_pref_col_f_hispanic[2][2] = {
	{ 108, 159 },
	{ 109, 156 }
};

#define GRP_DEL_Y_F_HISPANIC	16

/*
 * Group Params: USA (Block)
 */
static float grp_mean_g_black[3] = {
	114.3942f, 148.8480f, 137.2178f
};

static float grp_invcov_g_black[3][3] = {
	{ 0.1006f, 0.0924f, 0.0020f },
	{ 0.0924f, 0.1007f, 0.0009f },
	{ 0.0020f, 0.0009f, 0.0014f }
};

static u8 grp_pref_col_g_black[2][2] = {
	{ 114, 149 },
	{ 113, 150 }
};

#define GRP_DEL_Y_G_BLACK	24

/*
 * Group Params: China Korea
 */
static float grp_mean_b_china_korea[3] = {
	109.5704f, 152.8544f, 168.6223f
};

static float grp_invcov_b_china_korea[3][3] = {
	{ 0.0476f, 0.0350f, 0.0004f },
	{ 0.0350f, 0.0749f, 0.0065f },
	{ 0.0004f, 0.0065f, 0.0031f }
};

static u8 grp_pref_col_b_china_korea[2][2] = {
	{ 114, 149 },
	{ 114, 150 }
};

#define GRP_DEL_Y_B_CHINA_KOREA	128

/*
 * India
 */
static float grp_mean_c_india[3] = {
	106.6095f, 154.7484f, 168.3919f
};

static float grp_invcov_c_india[3][3] = {
	{ 0.0426f, 0.0459f, -0.0021f },
	{ 0.0459f, 0.0878f, -0.0010f },
	{ -0.0021f, -0.0010f, 0.0029f }
};

static u8 grp_pref_col_c_india[2][2] = {
	{ 112, 153 },
	{ 113, 152 }
};

#define GRP_DEL_Y_C_INDIA	152

/*
 * Indonesia
 */
static float grp_d_indonesia[3] = {
	111.3573f, 153.3356f, 143.7479f
};

static float grp_invcov_d_indonesia[3][3] = {
	{ 0.0979f, 0.0562f, 0.0003f },
	{ 0.0562f, 0.0849f, 0.0041f },
	{ 0.0003f, 0.0041f, 0.0022f }
};

static u8 grp_pref_col_d_indonesia[2][2] = {
	{ 113, 149 },
	{ 111, 152 }
};

#define GRP_DEL_Y_D_INDONESIA	168

/*
 * Region Index
 */
static u8 grp_model_num[GRP_REGION_CNT] = {
	GROUP_NUM_REGION_0,
	GROUP_NUM_REGION_1,
	GROUP_NUM_REGION_2,
	GROUP_NUM_REGION_3,
	GROUP_NUM_REGION_4,
	GROUP_NUM_REGION_5,
	GROUP_NUM_REGION_6,
	GROUP_NUM_REGION_7
};

/*
 * Region 0: Europe
 */
static float* grp_mean_region0[GROUP_NUM_REGION_0] = {
	grp_mean_h_white_all,
	grp_mean_f_hispanic,
	grp_mean_g_black
};

static float (*grp_invcov_region0[GROUP_NUM_REGION_0])[3] = {
	grp_invcov_h_white_all,
	grp_invcov_f_hispanic,
	grp_invcov_g_black
};

static u8 (*grp_pref_col_region0[GROUP_NUM_REGION_0])[2] = {
	grp_pref_col_h_white_all,
	grp_pref_col_f_hispanic,
	grp_pref_col_g_black,
};

static int grp_del_y_region0[GROUP_NUM_REGION_0] = {
	GRP_DEL_Y_H_WHITE_ALL,
	GRP_DEL_Y_F_HISPANIC,
	GRP_DEL_Y_G_BLACK,
};

/*
 * Region 1: Far East Asia
 */
static float* grp_mean_region1[GROUP_NUM_REGION_1] = {
	grp_mean_b_china_korea
};

static float (*grp_invcov_region1[GROUP_NUM_REGION_1])[3] = {
	grp_invcov_b_china_korea
};

static u8 (*grp_pref_col_region1[GROUP_NUM_REGION_1])[2] = {
	grp_pref_col_b_china_korea
};

static int grp_del_y_region1[GROUP_NUM_REGION_1] = {
	GRP_DEL_Y_B_CHINA_KOREA
};

/*
 * Region 2: India / Middle East Asia
 */
static float* grp_mean_region2[GROUP_NUM_REGION_2] = {
	grp_mean_c_india,
};

static float (*grp_invcov_region2[GROUP_NUM_REGION_2])[3] = {
	grp_invcov_c_india
};

static u8 (*grp_pref_col_region2[GROUP_NUM_REGION_2])[2] = {
	grp_pref_col_c_india
};

static int grp_del_y_region2[GROUP_NUM_REGION_2] = {
	GRP_DEL_Y_C_INDIA
};

/*
 * Region 3: South Eas Asia
 */
static float* grp_mean_region3[GROUP_NUM_REGION_3] = {
	grp_d_indonesia
};

static float (*grp_invcov_region3[GROUP_NUM_REGION_3])[3] = {
	grp_invcov_d_indonesia
};

static u8 (*grp_pref_col_region3[GROUP_NUM_REGION_3])[2] = {
	grp_pref_col_d_indonesia
};

static int grp_del_y_region3[GROUP_NUM_REGION_3] = {
	GRP_DEL_Y_D_INDONESIA
};

/*
 * Region 4: North America
 */
static float* grp_mean_region4[GROUP_NUM_REGION_4] = {
	grp_mean_h_white_all,
	grp_mean_f_hispanic,
	grp_mean_g_black
};

static float (*grp_invcov_region4[GROUP_NUM_REGION_4])[3] = {
	grp_invcov_h_white_all,
	grp_invcov_f_hispanic,
	grp_invcov_g_black
};

static u8 (*grp_pref_col_region4[GROUP_NUM_REGION_4])[2] = {
	grp_pref_col_h_white_all,
	grp_pref_col_f_hispanic,
	grp_pref_col_g_black
};

static int grp_del_y_region4[GROUP_NUM_REGION_4] = {
	GRP_DEL_Y_H_WHITE_ALL,
	GRP_DEL_Y_F_HISPANIC,
	GRP_DEL_Y_G_BLACK
};

/*
 * Region 5: South America
 */
static float* grp_mean_region5[GROUP_NUM_REGION_5] = {
	grp_mean_h_white_all,
	grp_mean_f_hispanic,
	grp_mean_g_black
};

static float (*grp_invcov_region5[GROUP_NUM_REGION_5])[3] = {
	grp_invcov_h_white_all,
	grp_invcov_f_hispanic,
	grp_invcov_g_black
};

static u8 (*grp_pref_col_region5[GROUP_NUM_REGION_5])[2] = {
	grp_pref_col_h_white_all,
	grp_pref_col_f_hispanic,
	grp_pref_col_g_black
};

static int grp_del_y_region5[GROUP_NUM_REGION_5] = {
	GRP_DEL_Y_H_WHITE_ALL,
	GRP_DEL_Y_F_HISPANIC,
	GRP_DEL_Y_G_BLACK
};

/*
 * Region 6: Africa
 */
static float* grp_mean_region6[GROUP_NUM_REGION_6] = {
	grp_mean_f_hispanic,
	grp_mean_g_black
};

static float (*grp_invcov_region6[GROUP_NUM_REGION_6])[3] = {
	grp_invcov_f_hispanic,
	grp_invcov_g_black
};

static u8 (*grp_pref_col_region6[GROUP_NUM_REGION_6])[2] = {
	grp_pref_col_f_hispanic,
	grp_pref_col_g_black
};

static int grp_del_y_region6[GROUP_NUM_REGION_6] = {
	GRP_DEL_Y_F_HISPANIC,
	GRP_DEL_Y_G_BLACK
};

/*
 * Region 7: Oceania
 */
static float* grp_mean_region7[GROUP_NUM_REGION_7] = {
	grp_mean_h_white_all,
	grp_mean_b_china_korea,
	grp_mean_f_hispanic
};

static float (*grp_invcov_region7[GROUP_NUM_REGION_7])[3] = {
	grp_invcov_h_white_all,
	grp_invcov_b_china_korea,
	grp_invcov_f_hispanic
};

static u8 (*grp_pref_col_region7[GROUP_NUM_REGION_7])[2] = {
	grp_pref_col_h_white_all,
	grp_pref_col_b_china_korea,
	grp_pref_col_f_hispanic
};

static int grp_del_y_region7[GROUP_NUM_REGION_7] = {
	GRP_DEL_Y_H_WHITE_ALL,
	GRP_DEL_Y_B_CHINA_KOREA,
	GRP_DEL_Y_F_HISPANIC
};

/*
 * Total Region
 */
static float* *grp_mean_regions[GRP_REGION_CNT] = {
	grp_mean_region0,
	grp_mean_region1,
	grp_mean_region2,
	grp_mean_region3,
	grp_mean_region4,
	grp_mean_region5,
	grp_mean_region6,
	grp_mean_region7,
};

static float (**grp_invcov_regions[GRP_REGION_CNT])[3] = {
	grp_invcov_region0,
	grp_invcov_region1,
	grp_invcov_region2,
	grp_invcov_region3,
	grp_invcov_region4,
	grp_invcov_region5,
	grp_invcov_region6,
	grp_invcov_region7,
};

static u8 (**grp_pref_col_regions[GRP_REGION_CNT])[2] = {
	grp_pref_col_region0,
	grp_pref_col_region1,
	grp_pref_col_region2,
	grp_pref_col_region3,
	grp_pref_col_region4,
	grp_pref_col_region5,
	grp_pref_col_region6,
	grp_pref_col_region7,
};

static int *grp_del_y_regions[GRP_REGION_CNT] = {
	grp_del_y_region0,
	grp_del_y_region1,
	grp_del_y_region2,
	grp_del_y_region3,
	grp_del_y_region4,
	grp_del_y_region5,
	grp_del_y_region6,
	grp_del_y_region7,
};

#endif //CAMERAPP_STR_PARAM_V100_H
