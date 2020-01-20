/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef fimc_is_hw_csi_H
#define fimc_is_hw_csi_H

#ifdef CONFIG_CSIS_V3_4
#define S5PCSIS_INTMSK_EVEN_BEFORE                      (1 << 31)
#define S5PCSIS_INTMSK_EVEN_AFTER                       (1 << 30)
#define S5PCSIS_INTMSK_ODD_BEFORE                       (1 << 29)
#define S5PCSIS_INTMSK_ODD_AFTER                        (1 << 28)
#define S5PCSIS_INTMSK_FRAME_START_CH3                  (1 << 27)
#define S5PCSIS_INTMSK_FRAME_START_CH2                  (1 << 26)
#define S5PCSIS_INTMSK_FRAME_START_CH1                  (1 << 25)
#define S5PCSIS_INTMSK_FRAME_START_CH0                  (1 << 24)
#define S5PCSIS_INTMSK_FRAME_END_CH3                    (1 << 23)
#define S5PCSIS_INTMSK_FRAME_END_CH2                    (1 << 22)
#define S5PCSIS_INTMSK_FRAME_END_CH1                    (1 << 21)
#define S5PCSIS_INTMSK_FRAME_END_CH0                    (1 << 20)
#define S5PCSIS_INTMSK_ERR_SOT_HS                       (1 << 16)
#define S5PCSIS_INTMSK_ERR_LOST_FS_CH3                  (1 << 15)
#define S5PCSIS_INTMSK_ERR_LOST_FS_CH2                  (1 << 14)
#define S5PCSIS_INTMSK_ERR_LOST_FS_CH1                  (1 << 13)
#define S5PCSIS_INTMSK_ERR_LOST_FS_CH0                  (1 << 12)
#define S5PCSIS_INTMSK_ERR_LOST_FE_CH3                  (1 << 11)
#define S5PCSIS_INTMSK_ERR_LOST_FE_CH2                  (1 << 10)
#define S5PCSIS_INTMSK_ERR_LOST_FE_CH1                  (1 << 9)
#define S5PCSIS_INTMSK_ERR_LOST_FE_CH0                  (1 << 8)
#define S5PCSIS_INTMSK_ERR_OVER_CH3                     (1 << 7)
#define S5PCSIS_INTMSK_ERR_OVER_CH2                     (1 << 6)
#define S5PCSIS_INTMSK_ERR_OVER_CH1                     (1 << 5)
#define S5PCSIS_INTMSK_ERR_OVER_CH0                     (1 << 4)
#define S5PCSIS_INTMSK_ERR_ECC                          (1 << 2)
#define S5PCSIS_INTMSK_ERR_CRC                          (1 << 1)
#define S5PCSIS_INTMSK_ERR_ID                           (1 << 0)

/* Interrupt source */
#define S5PCSIS_INTSRC                                  (0x14)
#define S5PCSIS_INTSRC_EVEN_BEFORE                      (1 << 31)
#define S5PCSIS_INTSRC_EVEN_AFTER                       (1 << 30)
#define S5PCSIS_INTSRC_EVEN                             (0x3 << 30)
#define S5PCSIS_INTSRC_ODD_BEFORE                       (1 << 29)
#define S5PCSIS_INTSRC_ODD_AFTER                        (1 << 28)
#define S5PCSIS_INTSRC_ODD                              (0x3 << 28)
#define S5PCSIS_INTSRC_FRAME_START                      (0xf << 24)
#define S5PCSIS_INTSRC_FRAME_END                        (0xf << 20)
#define S5PCSIS_INTSRC_ERR_SOT_HS                       (0xf << 16)
#define S5PCSIS_INTSRC_ERR_LOST_FS                      (0xf << 12)
#define S5PCSIS_INTSRC_ERR_LOST_FE                      (0xf << 8)
#define S5PCSIS_INTSRC_ERR_OVER                         (0xf << 4)
#define S5PCSIS_INTSRC_ERR_ECC                          (1 << 2)
#define S5PCSIS_INTSRC_ERR_CRC                          (1 << 1)
#define S5PCSIS_INTSRC_ERR_ID                           (1 << 0)
#define S5PCSIS_INTSRC_ERRORS                           (0xf1111117)
#endif

#endif
