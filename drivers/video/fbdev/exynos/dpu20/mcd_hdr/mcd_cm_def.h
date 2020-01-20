/* linux/drivers/video/fbdev/exynos/mcd_hdr/mcd_cm_def.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * header file for Samsung EXYNOS SoC HDR driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MCD_CM_DEF_H__
#define __MCD_CM_DEF_H__

enum pq_index {
    INDEX_PQ1000 = 0,
    INDEX_PQ2000 = 1,
    INDEX_PQ3000 = 2,
    INDEX_PQ4000 = 3,
    INDEX_PQ_MAX,
};

enum target_nit_index {
    INDEX_T0200 = 0,
    INDEX_T0250 = 1,
    INDEX_T0300 = 2,
    INDEX_T0350 = 3,
    INDEX_T0400 = 4,
    INDEX_T0450 = 5,
    INDEX_T0500 = 6,
    INDEX_T0550 = 7,
    INDEX_T1000 = 8,
    INDEX_THLG,
    INDEX_TMAX
};

enum gamma_type_index {
    INDEX_TYPE_SDR = 0,
    INDEX_TYPE_PQ  = 1,
    INDEX_TYPE_HLG = 2,
    INDEX_TYPE_MAX,
};

enum gamut_index {
    INDEX_GAMUT_UNSPECIFIED = 0,    // HAL_DATASPACE_STANDARD_UNSPECIFIED
    INDEX_GAMUT_BT709          ,    // HAL_DATASPACE_STANDARD_BT709
    INDEX_GAMUT_BT601_625      ,    // HAL_DATASPACE_STANDARD_BT601_625 | HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED
    INDEX_GAMUT_BT601_525      ,    // HAL_DATASPACE_STANDARD_BT601_525 | HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED
    INDEX_GAMUT_BT2020         ,    // HAL_DATASPACE_STANDARD_BT2020    | HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE
    INDEX_GAMUT_BT470M         ,    // HAL_DATASPACE_STANDARD_BT470M
    INDEX_GAMUT_FILM           ,    // HAL_DATASPACE_STANDARD_FILM
    INDEX_GAMUT_DCI_P3         ,    // HAL_DATASPACE_STANDARD_DCI_P3
    INDEX_GAMUT_ADOBE_RGB      ,    // HAL_DATASPACE_STANDARD_ADOBE_RGB
    INDEX_GAMUT_NUM            ,
};


enum gamma_index {
    INDEX_GAMMA_UNSPECIFIED = 0,    // HAL_DATASPACE_TRANSFER_UNSPECIFIED
    INDEX_GAMMA_LINEAR         ,    // HAL_DATASPACE_TRANSFER_LINEAR
    INDEX_GAMMA_SRGB           ,    // HAL_DATASPACE_TRANSFER_SRGB
    INDEX_GAMMA_SMPTE_170M     ,    // HAL_DATASPACE_TRANSFER_SMPTE_170M
    INDEX_GAMMA_GAMMA2_2       ,    // HAL_DATASPACE_TRANSFER_GAMMA2_2
    INDEX_GAMMA_GAMMA2_6       ,    // HAL_DATASPACE_TRANSFER_GAMMA2_6
    INDEX_GAMMA_GAMMA2_8       ,    // HAL_DATASPACE_TRANSFER_GAMMA2_8
    INDEX_GAMMA_ST2084         ,    // HAL_DATASPACE_TRANSFER_ST2084
    INDEX_GAMMA_HLG            ,    // HAL_DATASPACE_TRANSFER_HLG
    INDEX_GAMMA_NUM            ,
};

struct cm_tables {
    unsigned int *eotf;   // SZ_CM_EOTF 96
    unsigned int *gm;     // SZ_CM_GM   5
    unsigned int *oetf;   // SZ_CM_OETF 30
    unsigned int  tms;    // SZ_CM_TMS  1
    unsigned int *sc;     // SZ_CM_SC   2
    unsigned int *tm;     // SZ_CM_TM   25
};

#endif //__MCD_CM_DEF_H__
