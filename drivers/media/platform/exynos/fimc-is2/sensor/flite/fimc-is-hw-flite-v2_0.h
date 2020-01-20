/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_FLITE_H
#define FIMC_IS_DEVICE_FLITE_H

#include <linux/interrupt.h>
#include "fimc-is-type.h"

#define FLITE_ENABLE_FLAG	1
#define FLITE_ENABLE_MASK	0xFFFF
#define FLITE_ENABLE_SHIFT	0

#define FLITE_NOWAIT_FLAG	1
#define FLITE_NOWAIT_MASK	0xFFFF0000
#define FLITE_NOWAIT_SHIFT	16

#define FLITE_OVERFLOW_COUNT	10

#define FLITE_VVALID_TIME_BASE 32 /* ms */
/**************************/
/* Camera Source size */
#define FLITE_REG_CISRCSIZE				(0x00)
#define FLITE_REG_CISRCSIZE_SIZE_H(x)			((x) << 16)
#define FLITE_REG_CISRCSIZE_SIZE_V(x)			((x) << 0)
#define FLITE_REG_CISRCSIZE_ORDER422_IN_YCBYCR		(0 << 14)
#define FLITE_REG_CISRCSIZE_ORDER422_IN_YCRYCB		(1 << 14)
#define FLITE_REG_CISRCSIZE_ORDER422_IN_CBYCRY		(2 << 14)
#define FLITE_REG_CISRCSIZE_ORDER422_IN_CRYCBY		(3 << 14)

/* Global control */
#define FLITE_REG_CIGCTRL				0x04
#define FLITE_REG_CIGCTRL_YUV422_1P			(0x1E << 24)
#define FLITE_REG_CIGCTRL_RAW8				(0x2A << 24)
#define FLITE_REG_CIGCTRL_RAW10				(0x2B << 24)
#define FLITE_REG_CIGCTRL_RAW12				(0x2C << 24)
#define FLITE_REG_CIGCTRL_RAW14				(0x2D << 24)

/* User defined formats. x = 0...0xF. */
#define FLITE_REG_CIGCTRL_USER(x)			(0x30 + x - 1)
#define FLITE_REG_CIGCTRL_OLOCAL_DISABLE		(1 << 22)
#define FLITE_REG_CIGCTRL_SHADOWMASK_DISABLE		(1 << 21)
#define FLITE_REG_CIGCTRL_ODMA_DISABLE			(1 << 20)
#define FLITE_REG_CIGCTRL_SWRST_REQ			(1 << 19)
#define FLITE_REG_CIGCTRL_SWRST_RDY			(1 << 18)
#define FLITE_REG_CIGCTRL_SWRST				(1 << 17)
#define FLITE_REG_CIGCTRL_TEST_PATTERN_COLORBAR		(1 << 15)
#define FLITE_REG_CIGCTRL_INVPOLPCLK			(1 << 14)
#define FLITE_REG_CIGCTRL_INVPOLVSYNC			(1 << 13)
#define FLITE_REG_CIGCTRL_INVPOLHREF			(1 << 12)
#define FLITE_REG_CIGCTRL_IRQ_LASTEN0_ENABLE		(0 << 8)
#define FLITE_REG_CIGCTRL_IRQ_LASTEN0_DISABLE		(1 << 8)
#define FLITE_REG_CIGCTRL_IRQ_ENDEN0_ENABLE		(0 << 7)
#define FLITE_REG_CIGCTRL_IRQ_ENDEN0_DISABLE		(1 << 7)
#define FLITE_REG_CIGCTRL_IRQ_STARTEN0_ENABLE		(0 << 6)
#define FLITE_REG_CIGCTRL_IRQ_STARTEN0_DISABLE		(1 << 6)
#define FLITE_REG_CIGCTRL_IRQ_OVFEN0_ENABLE		(0 << 5)
#define FLITE_REG_CIGCTRL_IRQ_OVFEN0_DISABLE		(1 << 5)
#define FLITE_REG_CIGCTRL_SELCAM_MIPI			(1 << 3)

/* Image Capture Enable */
#define FLITE_REG_CIIMGCPT				(0x08)
#define FLITE_REG_CIIMGCPT_IMGCPTEN			(1 << 31)
#define FLITE_REG_CIIMGCPT_CPT_FREN			(1 << 25)
#define FLITE_REG_CIIMGCPT_CPT_FRPTR(x)			((x) << 19)
#define FLITE_REG_CIIMGCPT_CPT_MOD_FRCNT		(1 << 18)
#define FLITE_REG_CIIMGCPT_CPT_MOD_FREN			(0 << 18)
#define FLITE_REG_CIIMGCPT_CPT_FRCNT(x)			((x) << 10)

/* Capture Sequence */
#define FLITE_REG_CICPTSEQ				(0x0C)
#define FLITE_REG_CPT_FRSEQ(x)				((x) << 0)

/* Camera Window Offset */
#define FLITE_REG_CIWDOFST				(0x10)
#define FLITE_REG_CIWDOFST_WINOFSEN			(1 << 31)
#define FLITE_REG_CIWDOFST_CLROVIY			(1 << 30)
#define FLITE_REG_CIWDOFST_WINHOROFST(x)		((x) << 16)
#define FLITE_REG_CIWDOFST_HOROFF_MASK			(0x1fff << 16)
#define FLITE_REG_CIWDOFST_CLROVFICB			(1 << 15)
#define FLITE_REG_CIWDOFST_CLROVFICR			(1 << 14)
#define FLITE_REG_CIWDOFST_WINVEROFST(x)		((x) << 0)
#define FLITE_REG_CIWDOFST_VEROFF_MASK			(0x1fff << 0)

/* Cmaera Window Offset2 */
#define FLITE_REG_CIWDOFST2				(0x14)
#define FLITE_REG_CIWDOFST2_WINHOROFST2(x)		((x) << 16)
#define FLITE_REG_CIWDOFST2_WINVEROFST2(x)		((x) << 0)

/* Camera Output DMA Format */
#define FLITE_REG_CIODMAFMT				(0x18)
#define FLITE_REG_CIODMAFMT_1D_DMA			(1 << 15)
#define FLITE_REG_CIODMAFMT_2D_DMA			(0 << 15)
#define FLITE_REG_CIODMAFMT_PACK12			(1 << 14)
#define FLITE_REG_CIODMAFMT_NORMAL			(0 << 14)
#define FLITE_REG_CIODMAFMT_CRYCBY			(0 << 4)
#define FLITE_REG_CIODMAFMT_CBYCRY			(1 << 4)
#define FLITE_REG_CIODMAFMT_YCRYCB			(2 << 4)
#define FLITE_REG_CIODMAFMT_YCBYCR			(3 << 4)

/* Camera Output Canvas */
#define FLITE_REG_CIOCAN				(0x20)
#define FLITE_REG_CIOCAN_OCAN_V(x)			((x) << 16)
#define FLITE_REG_CIOCAN_OCAN_H(x)			((x) << 0)

/* Camera Output DMA Offset */
#define FLITE_REG_CIOOFF				(0x24)
#define FLITE_REG_CIOOFF_OOFF_V(x)			((x) << 16)
#define FLITE_REG_CIOOFF_OOFF_H(x)			((x) << 0)

/* Camera Output DMA Address */
#define FLITE_REG_CIOSA					(0x30)
#define FLITE_REG_CIOSA_OSA(x)				((x) << 0)

/* Camera Status */
#define FLITE_REG_CISTATUS				(0x40)
#define FLITE_REG_CISTATUS_MIPI_VVALID			(1 << 22)
#define FLITE_REG_CISTATUS_MIPI_HVALID			(1 << 21)
#define FLITE_REG_CISTATUS_MIPI_DVALID			(1 << 20)
#define FLITE_REG_CISTATUS_ITU_VSYNC			(1 << 14)
#define FLITE_REG_CISTATUS_ITU_HREFF			(1 << 13)
#define FLITE_REG_CISTATUS_OVFIY			(1 << 10)
#define FLITE_REG_CISTATUS_OVFICB			(1 << 9)
#define FLITE_REG_CISTATUS_OVFICR			(1 << 8)
#define FLITE_REG_CISTATUS_IRQ_SRC_OVERFLOW		(1 << 7)
#define FLITE_REG_CISTATUS_IRQ_SRC_LASTCAPEND		(1 << 6)
#define FLITE_REG_CISTATUS_IRQ_SRC_FRMSTART		(1 << 5)
#define FLITE_REG_CISTATUS_IRQ_SRC_FRMEND		(1 << 4)
#define FLITE_REG_CISTATUS_IRQ_CAM			(1 << 0)
#define FLITE_REG_CISTATUS_IRQ_MASK			(0xf << 4)

/* Camera Status2 */
#define FLITE_REG_CISTATUS2				(0x44)
#define FLITE_REG_CISTATUS2_LASTCAPEND			(1 << 1)
#define FLITE_REG_CISTATUS2_FRMEND			(1 << 0)

/* Camera Status3 */
#define FLITE_REG_CISTATUS3				0x48
#define FLITE_REG_CISTATUS3_PRESENT_MASK		(0x3F)

/* Qos Threshold */
#define FLITE_REG_CITHOLD				(0xF0)
#define FLITE_REG_CITHOLD_W_QOS_EN			(1 << 30)
#define FLITE_REG_CITHOLD_WTH_QOS(x)			((x) << 0)

/* Camera General Purpose */
#define FLITE_REG_CIGENERAL				(0xFC)
#define FLITE_REG_CIGENERAL_3AA0_CAM_A			(0)
#define FLITE_REG_CIGENERAL_3AA0_CAM_B			(1)
#define FLITE_REG_CIGENERAL_3AA0_CAM_C			(2)
#define FLITE_REG_CIGENERAL_3AA0_CAM_D			(3)
#define FLITE_REG_CIGENERAL_3AA1_CAM_A			(0 << 14)
#define FLITE_REG_CIGENERAL_3AA1_CAM_B			(1 << 14)
#define FLITE_REG_CIGENERAL_3AA1_CAM_C			(2 << 14)
#define FLITE_REG_CIGENERAL_3AA1_CAM_D			(3 << 14)

#define FLITE_REG_CIFCNTSEQ				0x100

/* BNS */
#define FLITE_REG_BINNINGON				(0x120)
#define FLITE_REG_BINNINGON_CLKGATE_ON(x)		(~(x) << 1)
#define FLITE_REG_BINNINGON_EN(x)			((x) << 0)

#define FLITE_REG_BINNINGCTRL				(0x124)
#define FLITE_REG_BINNINGCTRL_FACTOR_Y(x)		((x) << 22)
#define FLITE_REG_BINNINGCTRL_FACTOR_X(x)		((x) << 17)
#define FLITE_REG_BINNINGCTRL_SHIFT_UP_Y(x)		((x) << 15)
#define FLITE_REG_BINNINGCTRL_SHIFT_UP_X(x)		((x) << 13)
#define FLITE_REG_BINNINGCTRL_PRECISION_BITS(x)		((x) << 10)
#define FLITE_REG_BINNINGCTRL_BITTAGE(x)		((x) << 5)
#define FLITE_REG_BINNINGCTRL_UNITY_SIZE(x)		((x) << 0)

#define FLITE_REG_PEDESTAL				(0x128)
#define FLITE_REG_PEDESTAL_OUT(x)			((x) << 12)
#define FLITE_REG_PEDESTAL_IN(x)			((x) << 0)

#define FLITE_REG_BINNINGTOTAL				(0x12C)
#define FLITE_REG_BINNINGTOTAL_HEIGHT(x)		((x) << 16)
#define FLITE_REG_BINNINGTOTAL_WIDTH(x)			((x) << 0)

#define FLITE_REG_BINNINGINPUT				(0x130)
#define FLITE_REG_BINNINGINPUT_HEIGHT(x)		((x) << 16)
#define FLITE_REG_BINNINGINPUT_WIDTH(x)			((x) << 0)

#define FLITE_REG_BINNINGMARGIN				(0x134)
#define FLITE_REG_BINNINGMARGIN_TOP(x)			((x) << 16)
#define FLITE_REG_BINNINGMARGIN_LEFT(x)			((x) << 0)

#define FLITE_REG_BINNINGOUTPUT				(0x138)
#define FLITE_REG_BINNINGOUTPUT_HEIGHT(x)		((x) << 16)
#define FLITE_REG_BINNINGOUTPUT_WIDTH(x)		((x) << 0)

#define FLITE_REG_WEIGHTX01				(0x13C)
#define FLITE_REG_WEIGHTX01_1(x)			((x) << 16)
#define FLITE_REG_WEIGHTX01_0(x)			((x) << 0)

#define FLITE_REG_WEIGHTX23				(0x140)
#define FLITE_REG_WEIGHTX23_1(x)			((x) << 16)
#define FLITE_REG_WEIGHTX23_0(x)			((x) << 0)

#define FLITE_REG_WEIGHTY01				(0x15C)
#define FLITE_REG_WEIGHTY01_1(x)			((x) << 16)
#define FLITE_REG_WEIGHTY01_0(x)			((x) << 0)

#define FLITE_REG_WEIGHTY23				(0x160)
#define FLITE_REG_WEIGHTY23_1(x)			((x) << 16)
#define FLITE_REG_WEIGHTY23_0(x)			((x) << 0)

static const int bns_scales[17][16] = {
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 0 */
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 1 */
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 2 */
	{ 875, 125, 375, 625,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 3 */
	{ 750, 250,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 4 */
	{ 546, 328, 126, 114, 797,  89,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 5 */
	{ 429, 429, 142,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 6 */
	{ 295, 492, 134,  78,  84, 757, 108,  50,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 7 */
	{ 204, 606, 122,  66,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 8 */
	{ 106, 739,  82,  43,  30, 121, 445, 267, 103,  64,   0,   0,   0,   0,   0,   0},/* factor = 9 */
	{   1, 996,   1,   1,  66,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 10 */
	{  81, 725,  91,  48,  32,  23,  97, 253, 421, 115,  67,  47,   0,   0,   0,   0},/* factor = 11 */
	{ 109, 545, 182,  78,  50,  36,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 12 */
	{ 112, 411, 247,  95,  59,  43,  33,  46,  99, 690,  77,  41,  28,  21,   0,   0},/* factor = 13 */
	{ 107, 320, 320, 107,  64,  46,  36,   0,   0,   0,   0,   0,   0,   0,   0,   0},/* factor = 14 */
	{  91, 237, 396, 108,  62,  44,  34,  28,  40,  75, 675,  96,  45,  29,  22,  17},/* factor = 15 */
	{  72, 169, 507, 101,  56,  39,  30,  24,   0,   0,   0,   0,   0,   0,   0,   0} /* factor = 16 */
};



#endif
