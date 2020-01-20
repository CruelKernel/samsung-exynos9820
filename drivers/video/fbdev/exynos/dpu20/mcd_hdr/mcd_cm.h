/* linux/drivers/video/fbdev/exynos/mcd_hdr/mcd_cm.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * header file for Samsung EXYNOS SoC HDR driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MCD_CM_H__
#define __MCD_CM_H__


#include "hdr_drv.h"

struct mcd_cm_params_info {
    unsigned int needDither;
    unsigned int isAlphaPremultiplied;

//Todo need to remove
    unsigned int src_ds;
    unsigned int tgt_ds;

	unsigned int src_gamma;
	unsigned int src_gamut;
	unsigned int dst_gamma;
	unsigned int dst_gamut;

	unsigned int src_min_luminance;
	unsigned int src_max_luminance;

	unsigned int dst_max_luminance;
	
    unsigned int *hdr10p_lut;
};

/*Todo need to move to graphics-base.h*/

typedef enum {
    HAL_DATASPACE_UNKNOWN                               = 0,        // 0x0
    HAL_DATASPACE_ARBITRARY                             = 1,        // 0x1
    HAL_DATASPACE_STANDARD_SHIFT                        = 16,
    HAL_DATASPACE_STANDARD_MASK                         = 4128768,  // (63 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_UNSPECIFIED                  = 0,        // (0 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT709                        = 65536,    // (1 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT601_625                    = 131072,   // (2 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED         = 196608,   // (3 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT601_525                    = 262144,   // (4 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED         = 327680,   // (5 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT2020                       = 393216,   // (6 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE    = 458752,   // (7 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_BT470M                       = 524288,   // (8 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_FILM                         = 589824,   // (9 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_DCI_P3                       = 655360,   // (10 << STANDARD_SHIFT)
    HAL_DATASPACE_STANDARD_ADOBE_RGB                    = 720896,   // (11 << STANDARD_SHIFT)
    HAL_DATASPACE_TRANSFER_SHIFT                        = 22,
    HAL_DATASPACE_TRANSFER_MASK                         = 130023424,// (31 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_UNSPECIFIED                  = 0,        // (0 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_LINEAR                       = 4194304,  // (1 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_SRGB                         = 8388608,  // (2 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_SMPTE_170M                   = 12582912, // (3 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_GAMMA2_2                     = 16777216, // (4 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_GAMMA2_6                     = 20971520, // (5 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_GAMMA2_8                     = 25165824, // (6 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_ST2084                       = 29360128, // (7 << TRANSFER_SHIFT)
    HAL_DATASPACE_TRANSFER_HLG                          = 33554432, // (8 << TRANSFER_SHIFT)
    HAL_DATASPACE_RANGE_SHIFT                           = 27,
    HAL_DATASPACE_RANGE_MASK                            = 939524096,// (7 << RANGE_SHIFT)
    HAL_DATASPACE_RANGE_UNSPECIFIED                     = 0,        // (0 << RANGE_SHIFT)
    HAL_DATASPACE_RANGE_FULL                            = 134217728,// (1 << RANGE_SHIFT)
    HAL_DATASPACE_RANGE_LIMITED                         = 268435456,// (2 << RANGE_SHIFT)
    HAL_DATASPACE_RANGE_EXTENDED                        = 402653184,// (3 << RANGE_SHIFT)
    HAL_DATASPACE_SRGB_LINEAR                           = 512,      // 0x200
    HAL_DATASPACE_V0_SRGB_LINEAR                        = 138477568,// (STANDARD_BT709     | TRANSFER_LINEAR      | RANGE_FULL    )
    HAL_DATASPACE_V0_SCRGB_LINEAR                       = 406913024,// (STANDARD_BT709     | TRANSFER_LINEAR      | RANGE_EXTENDED)
    HAL_DATASPACE_SRGB                                  = 513,      // 0x201
    HAL_DATASPACE_V0_SRGB                               = 142671872,// (STANDARD_BT709     | TRANSFER_SRGB        | RANGE_FULL    )
    HAL_DATASPACE_V0_SCRGB                              = 411107328,// (STANDARD_BT709     | TRANSFER_SRGB        | RANGE_EXTENDED)
    HAL_DATASPACE_JFIF                                  = 257,      // 0x101
    HAL_DATASPACE_V0_JFIF                               = 146931712,// (STANDARD_BT601_625 | TRANSFER_SMPTE_170M  | RANGE_FULL    )
    HAL_DATASPACE_BT601_625                             = 258,      // 0x102
    HAL_DATASPACE_V0_BT601_625                          = 281149440,// (STANDARD_BT601_625 | TRANSFER_SMPTE_170M  | RANGE_LIMITED )
    HAL_DATASPACE_BT601_525                             = 259,      // 0x103
    HAL_DATASPACE_V0_BT601_525                          = 281280512,// (STANDARD_BT601_525 | TRANSFER_SMPTE_170M  | RANGE_LIMITED )
    HAL_DATASPACE_BT709                                 = 260,      // 0x104
    HAL_DATASPACE_V0_BT709                              = 281083904,// (STANDARD_BT709     | TRANSFER_SMPTE_170M  | RANGE_LIMITED )
    HAL_DATASPACE_DCI_P3_LINEAR                         = 139067392,// (STANDARD_DCI_P3    | TRANSFER_LINEAR      | RANGE_FULL    )
    HAL_DATASPACE_DCI_P3                                = 155844608,// (STANDARD_DCI_P3    | TRANSFER_GAMMA2_6    | RANGE_FULL    )
    HAL_DATASPACE_DISPLAY_P3_LINEAR                     = 139067392,// (STANDARD_DCI_P3    | TRANSFER_LINEAR      | RANGE_FULL    )
    HAL_DATASPACE_DISPLAY_P3                            = 143261696,// (STANDARD_DCI_P3    | TRANSFER_SRGB        | RANGE_FULL    )
    HAL_DATASPACE_ADOBE_RGB                             = 151715840,// (STANDARD_ADOBE_RGB | TRANSFER_GAMMA2_2    | RANGE_FULL    )
    HAL_DATASPACE_BT2020_LINEAR                         = 138805248,// (STANDARD_BT2020    | TRANSFER_LINEAR      | RANGE_FULL    )
    HAL_DATASPACE_BT2020                                = 147193856,// (STANDARD_BT2020    | TRANSFER_SMPTE_170M  | RANGE_FULL    )
    HAL_DATASPACE_BT2020_PQ                             = 163971072,// (STANDARD_BT2020    | TRANSFER_ST2084      | RANGE_FULL    )
    HAL_DATASPACE_DEPTH                                 = 4096,     // 0x1000
    HAL_DATASPACE_SENSOR                                = 4097,     // 0x1001
} android_dataspace_t;

void mcd_cm_reg_set_params(struct mcd_hdr_device *hdr, struct mcd_cm_params_info *params);
int mcd_cm_reg_reset(struct mcd_hdr_device *hdr);
int mcd_cm_reg_dump(struct mcd_hdr_device *hdr);

#endif //__MCD_CM_H__
