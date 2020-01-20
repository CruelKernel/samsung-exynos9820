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

/* Global control */
#define FLITE_REG_CIGCTRL				0x04
#define FLITE_REG_CIGCTRL_RAW8				(0x2A << 24)
#define FLITE_REG_CIGCTRL_RAW10				(0x2B << 24)
#define FLITE_REG_CIGCTRL_RAW12				(0x2C << 24)

/* User defined formats. x = 0...0xF. */
#define FLITE_REG_CIGCTRL_USER(x)			(0x30 + x - 1)
#define FLITE_REG_CIGCTRL_OLOCAL_DISABLE		(1 << 22)
#define FLITE_REG_CIGCTRL_SHADOWMASK_DISABLE		(1 << 21)
#define FLITE_REG_CIGCTRL_SWRST				(1 << 17)
#define FLITE_REG_CIGCTRL_TEST_PATTERN_COLORBAR		(1 << 15)
#define FLITE_REG_CIGCTRL_IRQ_LASTEN0_ENABLE		(0 << 8)
#define FLITE_REG_CIGCTRL_IRQ_LASTEN0_DISABLE		(1 << 8)
#define FLITE_REG_CIGCTRL_IRQ_ENDEN0_ENABLE		(0 << 7)
#define FLITE_REG_CIGCTRL_IRQ_ENDEN0_DISABLE		(1 << 7)
#define FLITE_REG_CIGCTRL_IRQ_STARTEN0_ENABLE		(0 << 6)
#define FLITE_REG_CIGCTRL_IRQ_STARTEN0_DISABLE		(1 << 6)
#define FLITE_REG_CIGCTRL_IRQ_LINEEN0_ENABLE		(0 << 4)
#define FLITE_REG_CIGCTRL_IRQ_LINEEN0_DISABLE		(1 << 4)

/* Image Capture Enable */
#define FLITE_REG_CIIMGCPT				(0x08)
#define FLITE_REG_CIIMGCPT_IMGCPTEN			(1 << 31)

/* Capture Sequence */
#define FLITE_REG_CICPTSEQ				(0x0C)
#define FLITE_REG_CPT_FRSEQ(x)				((x) << 0)

/* Camera Window Offset */
#define FLITE_REG_CIWDOFST				(0x10)
#define FLITE_REG_CIWDOFST_WINOFSEN			(1 << 31)
#define FLITE_REG_CIWDOFST_WINHOROFST(x)		((x) << 16)
#define FLITE_REG_CIWDOFST_HOROFF_MASK			(0x1fff << 16)
#define FLITE_REG_CIWDOFST_WINVEROFST(x)		((x) << 0)
#define FLITE_REG_CIWDOFST_VEROFF_MASK			(0x1fff << 0)

/* Cmaera Window Offset2 */
#define FLITE_REG_CIWDOFST2				(0x14)
#define FLITE_REG_CIWDOFST2_WINHOROFST2(x)		((x) << 16)
#define FLITE_REG_CIWDOFST2_WINVEROFST2(x)		((x) << 0)

/* Camera Status */
#define FLITE_REG_CISTATUS				(0x40)
#define FLITE_REG_CISTATUS_MIPI_VVALID			(1 << 22)
#define FLITE_REG_CISTATUS_MIPI_HVALID			(1 << 21)
#define FLITE_REG_CISTATUS_MIPI_DVALID			(1 << 20)
#define FLITE_REG_CISTATUS_IRQ_SRC_LINE			(1 << 8)
#define FLITE_REG_CISTATUS_IRQ_SRC_LASTCAPEND		(1 << 6)
#define FLITE_REG_CISTATUS_IRQ_SRC_FRMSTART		(1 << 5)
#define FLITE_REG_CISTATUS_IRQ_SRC_FRMEND		(1 << 4)
#define FLITE_REG_CISTATUS_IRQ_CAM			(1 << 0)
#define FLITE_REG_CISTATUS_IRQ_MASK			(0x1f << 4)

/* Camera Status2 */
#define FLITE_REG_CISTATUS2				(0x44)
#define FLITE_REG_CISTATUS2_LASTCAPEND			(1 << 1)
#define FLITE_REG_CISTATUS2_FRMEND			(1 << 0)

/* Camera Status4 : vsync count */
#define FLITE_REG_CISTATUS4				(0x4C)

/* Camera Status5 : frame count */
#define FLITE_REG_CISTATUS5				(0x50)

/* Camera Status7 : input stream size check(debug) */
#define FLITE_REG_CISTATUS7				(0x58)
#define FLITE_REG_CISTATUS7_IN_ERR_DETECT		(1 << 31)

/* Line Interrupt */
#define FLITE_REG_LINEINT				(0xEC)
#define FLITE_REG_LINEINT_RATIO				((x) << 0)

#define FLITE_REG_CIVERINFO				(0x104)

/* BNS */
#define FLITE_REG_BINNINGON				(0x120)
#define FLITE_REG_BINNINGON_CLKGATE_ON(x)		(~(x) << 1)
#define FLITE_REG_BINNINGON_EN(x)			((x) << 0)

#define FLITE_REG_BINNINGCTRL				(0x124)
#define FLITE_REG_BINNINGCTRL_FACTOR_Y(x)		((x) << 22)
#define FLITE_REG_BINNINGCTRL_FACTOR_X(x)		((x) << 17)
#define FLITE_REG_BINNINGCTRL_PRECISION_BITS(x)		((x) << 10)
#define FLITE_REG_BINNINGCTRL_BITTAGE(x)		((x) << 5)
#define FLITE_REG_BINNINGCTRL_UNITY_SIZE(x)		((x) << 0)

#define FLITE_REG_BINNINGTOTAL				(0x12C)
#define FLITE_REG_BINNINGTOTAL_HEIGHT(x)		((x) << 16)
#define FLITE_REG_BINNINGTOTAL_WIDTH(x)			((x) << 0)

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
	{  52, 889,  59, 813, 187, 438, 562, 313, 687,   0,   0,  0,  0,  0,  0,  0}, /* factor = 0 */
	{ 114, 720, 166,  59, 889,  52, 269, 591, 140, 563, 437,  0,  0,  0,  0,  0}, /* factor = 1 */
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 2 */
	{  89, 797, 114, 376, 624,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 3 */
	{  48, 714, 238,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 4 */
	{ 130, 475, 285, 110, 114, 797,  89,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 5 */
	{ 125, 375, 375, 125,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 6 */
	{ 102, 265, 442, 121,  70,  84, 757, 108,  51,   0,   0,  0,  0,  0,  0,  0}, /* factor = 7 */
	{  80, 187, 560, 112,  61,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 8 */
	{  47, 101, 704,  78,  41,  29, 121, 445, 267, 103,  64,  0,  0,  0,  0,  0}, /* factor = 9 */
	{  37,  75, 750,  75,  38,  25,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 10 */
	{  41,  77, 696,  87,  46,  30,  23,  97, 253, 421, 115, 67, 47,  0,  0,  0}, /* factor = 11 */
	{  57, 103, 514, 171,  73,  47,  35,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 12 */
	{  61, 105, 386, 232,  89,  55,  40,  32,  46,  99, 690, 77, 41, 28, 19,  0}, /* factor = 13 */
	{  62, 100, 301, 301, 100,  60,  43,  33,   0,   0,   0,  0,  0,  0,  0,  0}, /* factor = 14 */
	{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,  0}, /* nothing */
};


#endif
