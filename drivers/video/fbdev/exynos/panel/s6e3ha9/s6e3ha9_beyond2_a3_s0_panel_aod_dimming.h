/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/s6e3ha9_beyond2_a3_s0_panel_aod_dimming.h
 *
 * Header file for S6E3HA9 Dimming Driver
 *
 * Copyright (c) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA9_BEYOND2_A3_S0_PANEL_AOD_DIMMING_H__
#define __S6E3HA9_BEYOND2_A3_S0_PANEL_AOD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha9_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA9
 * PANEL : BEYOND2_A3_S0
 */
static unsigned int beyond2_a3_s0_aod_brt_tbl[S6E3HA9_AOD_NR_LUMINANCE] = {
	BRT_LT(40), BRT_LT(71), BRT_LT(94), BRT(255),
};

static unsigned int beyond2_a3_s0_aod_lum_tbl[S6E3HA9_AOD_NR_LUMINANCE] = {
	2, 10, 30, 60,
};

static struct brightness_table s6e3ha9_beyond2_a3_s0_panel_aod_brightness_table = {
	.brt = beyond2_a3_s0_aod_brt_tbl,
	.sz_brt = ARRAY_SIZE(beyond2_a3_s0_aod_brt_tbl),
	.lum = beyond2_a3_s0_aod_lum_tbl,
	.sz_lum = ARRAY_SIZE(beyond2_a3_s0_aod_lum_tbl),
	.brt_to_step = beyond2_a3_s0_aod_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(beyond2_a3_s0_aod_brt_tbl),
};

static struct panel_dimming_info s6e3ha9_beyond2_a3_s0_panel_aod_dimming_info = {
	.name = "s6e3ha9_beyond2_a3_s0_aod",
	.target_luminance = S6E3HA9_AOD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA9_AOD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha9_beyond2_a3_s0_panel_aod_brightness_table,
};
#endif /* __S6E3HA9_BEYOND2_A3_S0_PANEL_AOD_DIMMING_H__ */
