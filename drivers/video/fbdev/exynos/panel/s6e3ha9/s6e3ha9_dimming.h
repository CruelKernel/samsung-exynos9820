/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/s6e3ha9_dimming.h
 *
 * Header file for S6E3HA9 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA9_DIMMING_H__
#define __S6E3HA9_DIMMING_H__
#include <linux/types.h>
#include <linux/kernel.h>
#include "../dimming.h"
#include "s6e3ha9.h"

#define S6E3HA9_NR_TP (12)

#define S6E3HA9_NR_LUMINANCE (74)
#define S6E3HA9_TARGET_LUMINANCE (420)
#define S6E3HA9_BEYOND_TARGET_LUMINANCE (420)
#define S6E3HA9_DAVINCI_TARGET_LUMINANCE (420)

#ifndef CONFIG_LCD_HBM_60_STEP
#define S6E3HA9_NR_HBM_LUMINANCE (16)
#else
#define S6E3HA9_NR_HBM_LUMINANCE (80)
#endif

#define S6E3HA9_BEYOND_NR_HBM_LUMINANCE (16)
#define S6E3HA9_DAVINCI_NR_HBM_LUMINANCE (16)

#define S6E3HA9_TARGET_HBM_LUMINANCE (800)
#define S6E3HA9_BEYOND_TARGET_HBM_LUMINANCE (800)
#define S6E3HA9_DAVINCI_TARGET_HBM_LUMINANCE (800)

#define S6E3HA9_HMD_NR_LUMINANCE (37)
#define S6E3HA9_HMD_TARGET_LUMINANCE (420)
#define S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE (420)
#define S6E3HA9_DAVINCI_HMD_TARGET_LUMINANCE (420)

#ifdef CONFIG_SUPPORT_AOD_BL
#define S6E3HA9_AOD_NR_LUMINANCE (4)
#define S6E3HA9_AOD_TARGET_LUMINANCE (420)
#endif

#define S6E3HA9_TOTAL_NR_LUMINANCE (S6E3HA9_NR_LUMINANCE + S6E3HA9_NR_HBM_LUMINANCE)
#define S6E3HA9_BEYOND_TOTAL_NR_LUMINANCE (S6E3HA9_NR_LUMINANCE + S6E3HA9_BEYOND_NR_HBM_LUMINANCE)
#define S6E3HA9_DAVINCI_TOTAL_NR_LUMINANCE (S6E3HA9_NR_LUMINANCE + S6E3HA9_DAVINCI_NR_HBM_LUMINANCE)
#define S6E3HA9_HMD_TOTAL_NR_LUMINANCE (S6E3HA9_HMD_NR_LUMINANCE)

#define S6E3HA9_TOTAL_PAC_STEPS		(PANEL_BACKLIGHT_PAC_STEPS + S6E3HA9_NR_HBM_LUMINANCE)
#define S6E3HA9_BEYOND_TOTAL_PAC_STEPS		(PANEL_BACKLIGHT_PAC_STEPS + S6E3HA9_BEYOND_NR_HBM_LUMINANCE)
#define S6E3HA9_DAVINCI_TOTAL_PAC_STEPS		(PANEL_BACKLIGHT_PAC_STEPS + S6E3HA9_DAVINCI_NR_HBM_LUMINANCE)

static struct tp s6e3ha9_beyond_tp[S6E3HA9_NR_TP] = {
	{ .level = 0, .volt_src = VREG_OUT, .name = "VT", .center = { 0x0, 0x0, 0x0 }, .numerator = 0, .denominator = 1200, .bits = 4 },
	{ .level = 0, .volt_src = V0_OUT, .name = "V0", .center = { 0x3, 0x3, 0x3 }, .numerator = 0, .denominator = 1200, .bits = 4 },
	{ .level = 1, .volt_src = V0_OUT, .name = "V1", .center = { 0x80, 0x80, 0x80 }, .numerator = 0, .denominator = 256, .bits = 8 },
	{ .level = 7, .volt_src = V0_OUT, .name = "V7", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 11, .volt_src = VT_OUT, .name = "V11", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 23, .volt_src = VT_OUT, .name = "V23", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 35, .volt_src = VT_OUT, .name = "V35", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 51, .volt_src = VT_OUT, .name = "V51", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 87, .volt_src = VT_OUT, .name = "V87", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 151, .volt_src = VT_OUT, .name = "V151", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 203, .volt_src = VT_OUT, .name = "V203", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 255, .volt_src = VREG_OUT, .name = "V255", .center = { 0x200, 0x200, 0x200 }, .numerator = 177, .denominator = 1200, .bits = 10 },
};

static struct tp s6e3ha9_davinci_tp[S6E3HA9_NR_TP] = {
	{ .level = 0, .volt_src = VREG_OUT, .name = "VT", .center = { 0x0, 0x0, 0x0 }, .numerator = 0, .denominator = 1200, .bits = 4 },
	{ .level = 0, .volt_src = V0_OUT, .name = "V0", .center = { 0x3, 0x3, 0x3 }, .numerator = 0, .denominator = 1200, .bits = 4 },
	{ .level = 1, .volt_src = V0_OUT, .name = "V1", .center = { 0x80, 0x80, 0x80 }, .numerator = 0, .denominator = 256, .bits = 8 },
	{ .level = 7, .volt_src = V0_OUT, .name = "V7", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 11, .volt_src = VT_OUT, .name = "V11", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 23, .volt_src = VT_OUT, .name = "V23", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 35, .volt_src = VT_OUT, .name = "V35", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 51, .volt_src = VT_OUT, .name = "V51", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 87, .volt_src = VT_OUT, .name = "V87", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 151, .volt_src = VT_OUT, .name = "V151", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 203, .volt_src = VT_OUT, .name = "V203", .center = { 0x80, 0x80, 0x80 }, .numerator = 64, .denominator = 320, .bits = 8 },
	{ .level = 255, .volt_src = VREG_OUT, .name = "V255", .center = { 0x200, 0x200, 0x200 }, .numerator = 177, .denominator = 1200, .bits = 10 },
};

#ifdef CONFIG_SUPPORT_DIM_FLASH
static struct dim_flash_info s6e3ha9_dim_flash_info[MAX_DIM_FLASH] = {
	[DIM_FLASH_GAMMA] = {
		.name = "dim_flash_gamma",
		.offset = S6E3HA9_DIM_FLASH_GAMMA_OFS,
		.nrow = S6E3HA9_DIM_FLASH_GAMMA_ROW,
		.ncol = S6E3HA9_DIM_FLASH_GAMMA_COL
	},
	[DIM_FLASH_AOR] = {
		.name = "dim_flash_aor",
		.offset = S6E3HA9_DIM_FLASH_AOR_OFS,
		.nrow = S6E3HA9_DIM_FLASH_AOR_ROW,
		.ncol = S6E3HA9_DIM_FLASH_AOR_COL
	},
	[DIM_FLASH_VINT] = {
		.name = "dim_flash_vint",
		.offset = S6E3HA9_DIM_FLASH_VINT_OFS,
		.nrow = S6E3HA9_DIM_FLASH_VINT_ROW,
		.ncol = S6E3HA9_DIM_FLASH_VINT_COL
	},
	[DIM_FLASH_ELVSS] = {
		.name = "dim_flash_elvss",
		.offset = S6E3HA9_DIM_FLASH_ELVSS_OFS,
		.nrow = S6E3HA9_DIM_FLASH_ELVSS_ROW,
		.ncol = S6E3HA9_DIM_FLASH_ELVSS_COL
	},
	[DIM_FLASH_IRC] = {
		.name = "dim_flash_irc",
		.offset = S6E3HA9_DIM_FLASH_IRC_OFS,
		.nrow = S6E3HA9_DIM_FLASH_IRC_ROW,
		.ncol = S6E3HA9_DIM_FLASH_IRC_COL
	},
	[DIM_FLASH_HMD_GAMMA] = {
		.name = "dim_flash_hmd_gamma",
		.offset = S6E3HA9_DIM_FLASH_HMD_GAMMA_OFS,
		.nrow = S6E3HA9_DIM_FLASH_HMD_GAMMA_ROW,
		.ncol = S6E3HA9_DIM_FLASH_HMD_GAMMA_COL
	},
	[DIM_FLASH_HMD_AOR] = {
		.name = "dim_flash_hmd_aor",
		.offset = S6E3HA9_DIM_FLASH_HMD_AOR_OFS,
		.nrow = S6E3HA9_DIM_FLASH_HMD_AOR_ROW,
		.ncol = S6E3HA9_DIM_FLASH_HMD_AOR_COL
	},
};
#endif	/* CONFIG_SUPPORT_DIM_FLASH */
#endif /* __S6E3HA9_DIMMING_H__ */
