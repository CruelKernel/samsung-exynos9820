/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/s6e3ha9_beyondx_a3_s0_panel_hmd_dimming.h
 *
 * Header file for S6E3HA9 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA9_BEYONDX_A3_S0_PANEL_HMD_DIMMING_H__
#define __S6E3HA9_BEYONDX_A3_S0_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha9_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA9
 * PANEL : BEYONDX_A3_S0
 */
static s32 beyondx_a3_s0_hmd_rtbl10nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 11, 8, 7, 6, 2, -1, -1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl11nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 11, 8, 7, 5, 1, -1, -2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl12nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 6, 5, 2, -1, -1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl13nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 8, 7, 5, 1, -1, -2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl14nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 8, 7, 6, 3, 1, 0, 0 };
static s32 beyondx_a3_s0_hmd_rtbl15nit[S6E3HA9_NR_TP] = { 0, 0, 0, 11, 11, 7, 6, 5, 2, -1, -1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl16nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 7, 5, 2, -1, -1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl17nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 11, 7, 6, 5, 2, -1, 0, 0 };
static s32 beyondx_a3_s0_hmd_rtbl19nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 7, 5, 4, 2, -1, 0, 0 };
static s32 beyondx_a3_s0_hmd_rtbl20nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 6, 5, 1, 1, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl21nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 7, 5, 1, 0, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl22nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 6, 5, 1, 0, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl23nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 6, 5, 1, 1, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl25nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 7, 6, 4, 2, 1, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl27nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 6, 4, 1, 2, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl29nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 6, 5, 3, 2, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl31nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 10, 6, 6, 4, 2, 2, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl33nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 6, 6, 4, 2, 2, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl35nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 10, 6, 6, 4, 2, 4, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl37nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 7, 7, 5, 3, 5, 3, 0 };
static s32 beyondx_a3_s0_hmd_rtbl39nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 5, 3, 3, 4, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl41nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 6, 5, 3, 4, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl44nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 5, 6, 5, 4, 5, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl47nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 5, 5, 4, 3, 4, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl50nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 5, 6, 5, 4, 6, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl53nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 3, 6, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl56nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 6, 5, 4, 4, 6, 2, 0 };
static s32 beyondx_a3_s0_hmd_rtbl60nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 5, 6, 5, 4, 6, 3, 0 };
static s32 beyondx_a3_s0_hmd_rtbl64nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 5, 5, 4, 6, 3, 0 };
static s32 beyondx_a3_s0_hmd_rtbl68nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 5, 5, 4, 8, 4, 0 };
static s32 beyondx_a3_s0_hmd_rtbl72nit[S6E3HA9_NR_TP] = { 0, 0, 0, 10, 9, 6, 6, 5, 5, 8, 5, 0 };
static s32 beyondx_a3_s0_hmd_rtbl77nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 5, 3, 3, 2, 2, 4, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl82nit[S6E3HA9_NR_TP] = { 0, 0, 0, 6, 5, 4, 3, 3, 3, 5, 3, 0 };
static s32 beyondx_a3_s0_hmd_rtbl87nit[S6E3HA9_NR_TP] = { 0, 0, 0, 6, 5, 3, 4, 3, 4, 5, 1, 0 };
static s32 beyondx_a3_s0_hmd_rtbl93nit[S6E3HA9_NR_TP] = { 0, 0, 0, 6, 6, 4, 3, 3, 5, 6, 3, 0 };
static s32 beyondx_a3_s0_hmd_rtbl99nit[S6E3HA9_NR_TP] = { 0, 0, 0, 7, 5, 3, 3, 3, 5, 6, 3, 0 };
static s32 beyondx_a3_s0_hmd_rtbl105nit[S6E3HA9_NR_TP] = { 0, 0, 0, 6, 5, 3, 3, 3, 5, 6, 4, 0 };


static s32 beyondx_a3_s0_hmd_ctbl10nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl11nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl12nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl13nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl14nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl15nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl16nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl17nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl19nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl20nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl21nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl22nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl23nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl25nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl27nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl29nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl31nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl33nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl35nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl37nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl39nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl41nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl44nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl47nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl50nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl53nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl56nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl60nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl64nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl68nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl72nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl77nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl82nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl87nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl93nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl99nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyondx_a3_s0_hmd_ctbl105nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct dimming_lut beyondx_a3_s0_hmd_dimming_lut[] = {
	DIM_LUT_V0_INIT(10, 46, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl10nit, beyondx_a3_s0_hmd_ctbl10nit),
	DIM_LUT_V0_INIT(11, 50, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl11nit, beyondx_a3_s0_hmd_ctbl11nit),
	DIM_LUT_V0_INIT(12, 55, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl12nit, beyondx_a3_s0_hmd_ctbl12nit),
	DIM_LUT_V0_INIT(13, 59, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl13nit, beyondx_a3_s0_hmd_ctbl13nit),
	DIM_LUT_V0_INIT(14, 62, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl14nit, beyondx_a3_s0_hmd_ctbl14nit),
	DIM_LUT_V0_INIT(15, 68, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl15nit, beyondx_a3_s0_hmd_ctbl15nit),
	DIM_LUT_V0_INIT(16, 72, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl16nit, beyondx_a3_s0_hmd_ctbl16nit),
	DIM_LUT_V0_INIT(17, 77, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl17nit, beyondx_a3_s0_hmd_ctbl17nit),
	DIM_LUT_V0_INIT(19, 85, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl19nit, beyondx_a3_s0_hmd_ctbl19nit),
	DIM_LUT_V0_INIT(20, 89, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl20nit, beyondx_a3_s0_hmd_ctbl20nit),
	DIM_LUT_V0_INIT(21, 93, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl21nit, beyondx_a3_s0_hmd_ctbl21nit),
	DIM_LUT_V0_INIT(22, 97, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl22nit, beyondx_a3_s0_hmd_ctbl22nit),
	DIM_LUT_V0_INIT(23, 102, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl23nit, beyondx_a3_s0_hmd_ctbl23nit),
	DIM_LUT_V0_INIT(25, 109, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl25nit, beyondx_a3_s0_hmd_ctbl25nit),
	DIM_LUT_V0_INIT(27, 117, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl27nit, beyondx_a3_s0_hmd_ctbl27nit),
	DIM_LUT_V0_INIT(29, 125, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl29nit, beyondx_a3_s0_hmd_ctbl29nit),
	DIM_LUT_V0_INIT(31, 133, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl31nit, beyondx_a3_s0_hmd_ctbl31nit),
	DIM_LUT_V0_INIT(33, 141, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl33nit, beyondx_a3_s0_hmd_ctbl33nit),
	DIM_LUT_V0_INIT(35, 149, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl35nit, beyondx_a3_s0_hmd_ctbl35nit),
	DIM_LUT_V0_INIT(37, 156, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl37nit, beyondx_a3_s0_hmd_ctbl37nit),
	DIM_LUT_V0_INIT(39, 165, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl39nit, beyondx_a3_s0_hmd_ctbl39nit),
	DIM_LUT_V0_INIT(41, 172, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl41nit, beyondx_a3_s0_hmd_ctbl41nit),
	DIM_LUT_V0_INIT(44, 183, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl44nit, beyondx_a3_s0_hmd_ctbl44nit),
	DIM_LUT_V0_INIT(47, 194, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl47nit, beyondx_a3_s0_hmd_ctbl47nit),
	DIM_LUT_V0_INIT(50, 205, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl50nit, beyondx_a3_s0_hmd_ctbl50nit),
	DIM_LUT_V0_INIT(53, 214, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl53nit, beyondx_a3_s0_hmd_ctbl53nit),
	DIM_LUT_V0_INIT(56, 225, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl56nit, beyondx_a3_s0_hmd_ctbl56nit),
	DIM_LUT_V0_INIT(60, 240, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl60nit, beyondx_a3_s0_hmd_ctbl60nit),
	DIM_LUT_V0_INIT(64, 255, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl64nit, beyondx_a3_s0_hmd_ctbl64nit),
	DIM_LUT_V0_INIT(68, 267, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl68nit, beyondx_a3_s0_hmd_ctbl68nit),
	DIM_LUT_V0_INIT(72, 279, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl72nit, beyondx_a3_s0_hmd_ctbl72nit),
	DIM_LUT_V0_INIT(77, 215, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl77nit, beyondx_a3_s0_hmd_ctbl77nit),
	DIM_LUT_V0_INIT(82, 227, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl82nit, beyondx_a3_s0_hmd_ctbl82nit),
	DIM_LUT_V0_INIT(87, 242, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl87nit, beyondx_a3_s0_hmd_ctbl87nit),
	DIM_LUT_V0_INIT(93, 256, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl93nit, beyondx_a3_s0_hmd_ctbl93nit),
	DIM_LUT_V0_INIT(99, 269, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl99nit, beyondx_a3_s0_hmd_ctbl99nit),
	DIM_LUT_V0_INIT(105, 284, GAMMA_2_15, beyondx_a3_s0_hmd_rtbl105nit, beyondx_a3_s0_hmd_ctbl105nit),
};

static u8 beyondx_a3_s0_hmd_dimming_gamma_table[S6E3HA9_HMD_NR_LUMINANCE][S6E3HA9_GAMMA_CMD_CNT - 1];
static u8 beyondx_a3_s0_hmd_dimming_aor_table[S6E3HA9_HMD_NR_LUMINANCE][2] = {
	{ 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 },
	{ 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 },
	{ 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 },
	{ 0x09, 0x93 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 },
};

static struct maptbl beyondx_a3_s0_hmd_dimming_param_maptbl[MAX_DIMMING_MAPTBL] = {
#ifdef CONFIG_SUPPORT_HMD
	[DIMMING_GAMMA_MAPTBL] = DEFINE_2D_MAPTBL(beyondx_a3_s0_hmd_dimming_gamma_table, init_hmd_gamma_table, getidx_dimming_maptbl, copy_common_maptbl),
	[DIMMING_AOR_MAPTBL] = DEFINE_2D_MAPTBL(beyondx_a3_s0_hmd_dimming_aor_table, init_hmd_aor_table, getidx_dimming_maptbl, copy_common_maptbl),
#endif
};

static unsigned int beyondx_a3_s0_hmd_brt_tbl[S6E3HA9_HMD_NR_LUMINANCE] = {
	BRT(26), BRT(29), BRT(31), BRT(33), BRT(36), BRT(38), BRT(41), BRT(46), BRT(48), BRT(50),
	BRT(53), BRT(55), BRT(60), BRT(65), BRT(70), BRT(75), BRT(80), BRT(84), BRT(89), BRT(94),
	BRT(99), BRT(106), BRT(114), BRT(121), BRT(128), BRT(135), BRT(145), BRT(155), BRT(165), BRT(174),
	BRT(186), BRT(199), BRT(211), BRT(225), BRT(240), BRT(254), BRT(255),
};

static unsigned int beyondx_a3_s0_hmd_lum_tbl[S6E3HA9_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

static struct brightness_table s6e3ha9_beyondx_a3_s0_panel_hmd_brightness_table = {
	.brt = beyondx_a3_s0_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(beyondx_a3_s0_hmd_brt_tbl),
	.lum = beyondx_a3_s0_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(beyondx_a3_s0_hmd_lum_tbl),
	.sz_ui_lum = S6E3HA9_HMD_NR_LUMINANCE,
	.sz_hbm_lum = 0,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = beyondx_a3_s0_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(beyondx_a3_s0_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3ha9_beyondx_a3_s0_preliminary_panel_hmd_dimming_info = {
	.name = "s6e3ha9_beyondx_a3_s0_preliminary_hmd",
	.dim_init_info = {
		.name = "s6e3ha9_beyondx_a3_s0_preliminary_hmd",
		.nr_tp = S6E3HA9_NR_TP,
		.tp = s6e3ha9_beyond_tp,
		.nr_luminance = S6E3HA9_HMD_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.vref = 16777216LL,	/* 1.0 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 24, 48, 72, 96, 120, 144, 168, 192, 216,
			276, 296, 316, 336, 356, 372,
		},
		.v0_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			120, 132, 144, 156, 168, 180,
		},
		.target_luminance = S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = beyondx_a3_s0_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA9_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha9_beyondx_a3_s0_panel_hmd_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyondx_a3_s0_hmd_dimming_param_maptbl,
	.dim_flash_on = false,	/* read dim flash when probe or not */
};

static struct panel_dimming_info s6e3ha9_beyondx_a3_s0_panel_hmd_dimming_info = {
	.name = "s6e3ha9_beyondx_a3_s0_hmd",
	.dim_init_info = {
		.name = "s6e3ha9_beyondx_a3_s0_hmd",
		.nr_tp = S6E3HA9_NR_TP,
		.tp = s6e3ha9_beyond_tp,
		.nr_luminance = S6E3HA9_HMD_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.vref = 16777216LL,	/* 1.0 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 24, 48, 72, 96, 120, 144, 168, 192, 216,
			276, 296, 316, 336, 356, 372,
		},
		.v0_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			120, 132, 144, 156, 168, 180,
		},
		.target_luminance = S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = beyondx_a3_s0_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA9_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha9_beyondx_a3_s0_panel_hmd_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyondx_a3_s0_hmd_dimming_param_maptbl,
	.dim_flash_on = true,	/* read dim flash when probe or not */
};
#endif /* __S6E3HA9_BEYONDX_A3_S0_PANEL_HMD_DIMMING_H__ */
