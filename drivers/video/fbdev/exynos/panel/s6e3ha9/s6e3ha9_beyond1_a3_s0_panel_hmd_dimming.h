/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/s6e3ha9_beyond1_a3_s0_panel_hmd_dimming.h
 *
 * Header file for S6E3HA9 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA9_BEYOND1_A3_S0_PANEL_HMD_DIMMING_H__
#define __S6E3HA9_BEYOND1_A3_S0_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha9_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA9
 * PANEL : BEYOND1_A3_S0
 */
static s32 beyond1_a3_s0_hmd_rtbl10nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 7, 6, 5, 2, -1, -1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl11nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 7, 6, 5, 2, -1, -2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl12nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 7, 5, 5, 2, 0, -1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl13nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 7, 5, 4, 1, -1, -1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl14nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 7, 5, 5, 1, 0, -1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl15nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 6, 5, 4, 2, 1, 0, 0 };
static s32 beyond1_a3_s0_hmd_rtbl16nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 6, 5, 4, 1, 1, 0, 0 };
static s32 beyond1_a3_s0_hmd_rtbl17nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 7, 5, 4, 2, 0, 1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl19nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 4, 3, 1, -1, 0, 0 };
static s32 beyond1_a3_s0_hmd_rtbl20nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 6, 5, 4, 1, 0, 1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl21nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 6, 5, 4, 1, 0, 1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl22nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 5, 4, 3, 0, -1, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl23nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 5, 4, 1, 0, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl25nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 6, 5, 3, 2, 1, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl27nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 4, 3, 1, 1, 1, 0 };
static s32 beyond1_a3_s0_hmd_rtbl29nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 3, 3, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl31nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 6, 5, 4, 3, 5, 4, 0 };
static s32 beyond1_a3_s0_hmd_rtbl33nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 5, 6, 4, 2, 3, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl35nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 9, 5, 5, 4, 2, 4, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl37nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 6, 6, 3, 2, 5, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl39nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 5, 4, 3, 6, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl41nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 5, 4, 3, 4, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl44nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 3, 5, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl47nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 3, 6, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl50nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 9, 5, 5, 4, 4, 6, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl53nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 7, 5, 5, 4, 4, 7, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl56nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 7, 5, 4, 3, 4, 7, 4, 0 };
static s32 beyond1_a3_s0_hmd_rtbl60nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 5, 4, 4, 7, 4, 0 };
static s32 beyond1_a3_s0_hmd_rtbl64nit[S6E3HA9_NR_TP] = { 0, 0, 0, 8, 8, 5, 5, 4, 4, 8, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl68nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 7, 5, 4, 4, 5, 8, 4, 0 };
static s32 beyond1_a3_s0_hmd_rtbl72nit[S6E3HA9_NR_TP] = { 0, 0, 0, 9, 7, 5, 5, 4, 5, 9, 5, 0 };
static s32 beyond1_a3_s0_hmd_rtbl77nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 5, 3, 3, 2, 3, 6, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl82nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 5, 3, 3, 2, 3, 6, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl87nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 5, 2, 3, 2, 4, 5, 2, 0 };
static s32 beyond1_a3_s0_hmd_rtbl93nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 5, 3, 3, 2, 4, 6, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl99nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 4, 2, 3, 2, 4, 5, 3, 0 };
static s32 beyond1_a3_s0_hmd_rtbl105nit[S6E3HA9_NR_TP] = { 0, 0, 0, 5, 5, 3, 3, 3, 5, 7, 7, 0 };

static s32 beyond1_a3_s0_hmd_ctbl10nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, -8, -8, 2, -7, -4, 0, -5, -5, 3, -6, -6, 2, -7, -3, 1, -3, 0, 0, 1, 3, -1, 2, 0, 0, 1 };
static s32 beyond1_a3_s0_hmd_ctbl11nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, -8, -8, 2, -8, -4, 0, -5, -5, 3, -6, -4, 2, -5, -3, 1, -2, 0, 0, 1, 5, 0, 3, 0, 0, 1 };
static s32 beyond1_a3_s0_hmd_ctbl12nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, -8, -8, 2, -8, -4, 0, -5, -5, 3, -6, -4, 2, -5, -3, 1, -2, 0, 0, 1, 3, 0, 1, 0, 0, 1 };
static s32 beyond1_a3_s0_hmd_ctbl13nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, -8, -8, 2, -8, -4, 0, -5, -5, 3, -6, -4, 2, -6, -3, 1, -3, 3, 0, 2, 0, 0, -1, 2, 0, 4 };
static s32 beyond1_a3_s0_hmd_ctbl14nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, -8, -8, 0, -8, -4, 0, -5, -5, 3, -6, -4, 2, -6, -2, 1, 0, 2, 0, 2, 0, 0, -1, 2, 0, 4 };
static s32 beyond1_a3_s0_hmd_ctbl15nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, -8, -8, 2, -8, -4, 0, -5, -5, 3, -6, -4, 2, -6, -2, 0, 0, 2, 0, 1, 0, 0, -1, 2, 0, 3 };
static s32 beyond1_a3_s0_hmd_ctbl16nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, -8, -8, 1, -8, -4, -1, -5, -6, 3, -6, -4, 2, -6, -2, 0, 0, 2, 0, 2, -1, 0, -1, 2, 0, 3 };
static s32 beyond1_a3_s0_hmd_ctbl17nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, -6, -8, 0, -8, -4, -1, -5, -6, 3, -6, -4, 2, -6, -2, 0, 0, 3, 0, 2, -1, 0, -1, 3, -1, 4 };
static s32 beyond1_a3_s0_hmd_ctbl19nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, -6, -8, 0, -8, -4, 2, -5, -6, 3, -6, -4, 2, -6, -2, 0, 0, 2, 0, 2, 0, 0, -1, 4, -1, 5 };
static s32 beyond1_a3_s0_hmd_ctbl20nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, -6, -8, 0, -8, -4, 2, -5, -6, 3, -6, -4, 2, -6, -2, 0, 0, 2, 0, 2, 0, 0, -1, 3, -1, 5 };
static s32 beyond1_a3_s0_hmd_ctbl21nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, -6, -8, 0, -8, -4, 2, -5, -6, 3, -6, -4, 2, -6, -2, 0, 0, 2, 0, 1, -1, 0, -1, 5, -1, 6 };
static s32 beyond1_a3_s0_hmd_ctbl22nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, -5, -6, 0, -8, -4, 2, -5, -6, 2, -6, -5, 2, -6, -2, 1, 0, 3, 0, 2, -1, 0, -1, 5, -1, 6 };
static s32 beyond1_a3_s0_hmd_ctbl23nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, -5, -6, 0, -9, -5, 2, -6, -3, 2, -4, -5, 2, -5, 0, 0, 0, 0, 0, 0, -1, 0, -1, 6, -1, 7 };
static s32 beyond1_a3_s0_hmd_ctbl25nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -7, -6, 0, -8, -5, 2, -6, -4, 2, -5, -4, 2, -5, 0, -1, 1, -3, 0, -2, -1, 0, -1, 6, -1, 6 };
static s32 beyond1_a3_s0_hmd_ctbl27nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -6, -5, 1, -8, -4, 2, -4, -4, 3, -5, -4, 2, -4, 0, -1, 0, -2, 0, -1, -1, 0, -1, 7, -1, 7 };
static s32 beyond1_a3_s0_hmd_ctbl29nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 3, -4, -5, 1, -8, -5, 1, -5, -5, 2, -6, -3, 1, -4, 1, -1, 2, -3, 0, -3, -1, 0, -1, 7, -1, 8 };
static s32 beyond1_a3_s0_hmd_ctbl31nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 4, -6, -3, 0, -8, -5, 1, -6, -5, 2, -6, -3, 2, -4, 2, 0, 2, -4, 0, -3, -1, 0, -1, 7, -2, 8 };
static s32 beyond1_a3_s0_hmd_ctbl33nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 5, -7, -8, -1, -10, -5, 2, -6, -5, 2, -6, -4, 0, -4, 0, 0, 0, -2, 0, -1, -1, 0, -1, 8, -2, 8 };
static s32 beyond1_a3_s0_hmd_ctbl35nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, -6, -7, -1, -9, -5, 1, -6, -6, 2, -6, -2, 1, -2, 0, 0, 1, -3, 0, -2, -1, 0, -1, 8, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl37nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -5, -7, 1, -10, -5, 2, -6, -5, 1, -5, -3, 1, -3, 0, 0, 1, -2, 0, -2, 0, 0, -1, 9, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl39nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -6, -7, 0, -10, -6, 2, -6, -4, 2, -5, -2, 1, -2, 0, 0, 1, -2, 0, -2, -1, 0, -1, 9, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl41nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -5, -6, -1, -10, -5, 2, -6, -7, 2, -5, -1, 1, -2, 0, 0, 2, -3, 0, -3, 0, 0, -1, 10, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl44nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -3, -8, 0, -11, -4, 2, -5, -6, 2, -4, -1, 1, -1, 0, 0, 0, -2, 0, -2, 0, 0, 0, 10, -2, 10 };
static s32 beyond1_a3_s0_hmd_ctbl47nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -3, -8, 0, -10, -5, 2, -5, -5, 2, -5, -1, 1, -2, 0, 0, 1, -3, 0, -3, -1, 0, -1, 11, -2, 12 };
static s32 beyond1_a3_s0_hmd_ctbl50nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -5, -6, 0, -7, -5, 1, -5, -6, 2, -6, -2, 1, -3, 1, 0, 1, -2, 0, -2, 0, 0, -1, 10, -2, 10 };
static s32 beyond1_a3_s0_hmd_ctbl53nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, -3, -9, 1, -10, -3, 2, -4, -5, 2, -5, -2, 1, -4, 0, 0, 0, -1, 0, -1, 0, 0, 0, 10, -2, 11 };
static s32 beyond1_a3_s0_hmd_ctbl56nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, -2, -9, 1, -10, -4, 2, -4, -6, 2, -6, -1, 1, -2, 0, 0, 1, -1, 0, -1, 0, 0, -1, 10, -2, 11 };
static s32 beyond1_a3_s0_hmd_ctbl60nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, -5, -7, 1, -9, -5, 3, -5, -4, 2, -4, -1, 1, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 11, -2, 11 };
static s32 beyond1_a3_s0_hmd_ctbl64nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 4, -5, -8, 1, -10, -5, 2, -6, -5, 1, -4, -2, 0, -3, 0, -1, 0, -2, 0, -1, 0, 0, -1, 12, -2, 12 };
static s32 beyond1_a3_s0_hmd_ctbl68nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -3, -8, 3, -10, -5, 2, -6, -5, 1, -4, 0, 1, -1, 0, 0, 1, -2, 0, -1, 0, 0, -1, 12, -2, 12 };
static s32 beyond1_a3_s0_hmd_ctbl72nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -5, -7, 2, -10, -5, 2, -5, -5, 1, -4, -1, 0, -2, 0, 0, 0, -2, 0, -1, 0, 0, -1, 13, -3, 12 };
static s32 beyond1_a3_s0_hmd_ctbl77nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 5, -6, -5, 0, -7, -4, 1, -3, -4, 0, -4, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, -1, 8, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl82nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, -6, -6, -1, -8, -5, 0, -5, -3, 0, -3, 0, -2, -2, -1, 2, 0, -1, 0, 0, 0, 0, 0, 7, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl87nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 2, -6, -4, 1, -7, -5, 0, -4, -2, 1, -2, 0, -1, 0, -4, 0, -2, 0, 0, 0, 0, 0, 0, 8, -2, 8 };
static s32 beyond1_a3_s0_hmd_ctbl93nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 3, -5, -6, 0, -7, -5, 1, -5, -1, 1, -3, -1, -2, 0, -3, 0, -2, 0, 1, 0, 0, 0, 0, 8, -2, 8 };
static s32 beyond1_a3_s0_hmd_ctbl99nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 1, -7, -6, 0, -8, -5, 3, -4, -2, 0, -2, 0, -2, 0, -3, 0, -2, 0, 0, -1, 0, 0, -1, 8, -2, 9 };
static s32 beyond1_a3_s0_hmd_ctbl105nit[S6E3HA9_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, -3, -2, 1, -4, -5, 2, -4, -1, 1, -2, 0, -2, 0, -2, 1, -2, 0, 1, 0, 0, 0, 0, 9, -2, 9 };

static struct dimming_lut beyond1_a3_s0_hmd_dimming_lut[] = {
	DIM_LUT_V0_INIT(10, 46, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl10nit, beyond1_a3_s0_hmd_ctbl10nit),
	DIM_LUT_V0_INIT(11, 50, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl11nit, beyond1_a3_s0_hmd_ctbl11nit),
	DIM_LUT_V0_INIT(12, 55, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl12nit, beyond1_a3_s0_hmd_ctbl12nit),
	DIM_LUT_V0_INIT(13, 59, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl13nit, beyond1_a3_s0_hmd_ctbl13nit),
	DIM_LUT_V0_INIT(14, 64, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl14nit, beyond1_a3_s0_hmd_ctbl14nit),
	DIM_LUT_V0_INIT(15, 67, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl15nit, beyond1_a3_s0_hmd_ctbl15nit),
	DIM_LUT_V0_INIT(16, 71, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl16nit, beyond1_a3_s0_hmd_ctbl16nit),
	DIM_LUT_V0_INIT(17, 77, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl17nit, beyond1_a3_s0_hmd_ctbl17nit),
	DIM_LUT_V0_INIT(19, 85, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl19nit, beyond1_a3_s0_hmd_ctbl19nit),
	DIM_LUT_V0_INIT(20, 89, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl20nit, beyond1_a3_s0_hmd_ctbl20nit),
	DIM_LUT_V0_INIT(21, 93, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl21nit, beyond1_a3_s0_hmd_ctbl21nit),
	DIM_LUT_V0_INIT(22, 98, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl22nit, beyond1_a3_s0_hmd_ctbl22nit),
	DIM_LUT_V0_INIT(23, 100, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl23nit, beyond1_a3_s0_hmd_ctbl23nit),
	DIM_LUT_V0_INIT(25, 104, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl25nit, beyond1_a3_s0_hmd_ctbl25nit),
	DIM_LUT_V0_INIT(27, 112, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl27nit, beyond1_a3_s0_hmd_ctbl27nit),
	DIM_LUT_V0_INIT(29, 119, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl29nit, beyond1_a3_s0_hmd_ctbl29nit),
	DIM_LUT_V0_INIT(31, 126, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl31nit, beyond1_a3_s0_hmd_ctbl31nit),
	DIM_LUT_V0_INIT(33, 134, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl33nit, beyond1_a3_s0_hmd_ctbl33nit),
	DIM_LUT_V0_INIT(35, 141, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl35nit, beyond1_a3_s0_hmd_ctbl35nit),
	DIM_LUT_V0_INIT(37, 148, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl37nit, beyond1_a3_s0_hmd_ctbl37nit),
	DIM_LUT_V0_INIT(39, 155, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl39nit, beyond1_a3_s0_hmd_ctbl39nit),
	DIM_LUT_V0_INIT(41, 164, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl41nit, beyond1_a3_s0_hmd_ctbl41nit),
	DIM_LUT_V0_INIT(44, 175, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl44nit, beyond1_a3_s0_hmd_ctbl44nit),
	DIM_LUT_V0_INIT(47, 185, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl47nit, beyond1_a3_s0_hmd_ctbl47nit),
	DIM_LUT_V0_INIT(50, 195, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl50nit, beyond1_a3_s0_hmd_ctbl50nit),
	DIM_LUT_V0_INIT(53, 204, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl53nit, beyond1_a3_s0_hmd_ctbl53nit),
	DIM_LUT_V0_INIT(56, 214, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl56nit, beyond1_a3_s0_hmd_ctbl56nit),
	DIM_LUT_V0_INIT(60, 228, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl60nit, beyond1_a3_s0_hmd_ctbl60nit),
	DIM_LUT_V0_INIT(64, 242, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl64nit, beyond1_a3_s0_hmd_ctbl64nit),
	DIM_LUT_V0_INIT(68, 255, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl68nit, beyond1_a3_s0_hmd_ctbl68nit),
	DIM_LUT_V0_INIT(72, 266, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl72nit, beyond1_a3_s0_hmd_ctbl72nit),
	DIM_LUT_V0_INIT(77, 208, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl77nit, beyond1_a3_s0_hmd_ctbl77nit),
	DIM_LUT_V0_INIT(82, 219, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl82nit, beyond1_a3_s0_hmd_ctbl82nit),
	DIM_LUT_V0_INIT(87, 231, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl87nit, beyond1_a3_s0_hmd_ctbl87nit),
	DIM_LUT_V0_INIT(93, 247, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl93nit, beyond1_a3_s0_hmd_ctbl93nit),
	DIM_LUT_V0_INIT(99, 259, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl99nit, beyond1_a3_s0_hmd_ctbl99nit),
	DIM_LUT_V0_INIT(105, 274, GAMMA_2_15, beyond1_a3_s0_hmd_rtbl105nit, beyond1_a3_s0_hmd_ctbl105nit),
};

static u8 beyond1_a3_s0_hmd_dimming_gamma_table[S6E3HA9_HMD_NR_LUMINANCE][S6E3HA9_GAMMA_CMD_CNT - 1];
static u8 beyond1_a3_s0_hmd_dimming_aor_table[S6E3HA9_HMD_NR_LUMINANCE][2] = {
	{ 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 },
	{ 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 },
	{ 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 }, { 0x09, 0x93 },
	{ 0x09, 0x93 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 }, { 0x08, 0x61 },
};

static struct maptbl beyond1_a3_s0_hmd_dimming_param_maptbl[MAX_DIMMING_MAPTBL] = {
#ifdef CONFIG_SUPPORT_HMD
	[DIMMING_GAMMA_MAPTBL] = DEFINE_2D_MAPTBL(beyond1_a3_s0_hmd_dimming_gamma_table, init_hmd_gamma_table, getidx_dimming_maptbl, copy_common_maptbl),
	[DIMMING_AOR_MAPTBL] = DEFINE_2D_MAPTBL(beyond1_a3_s0_hmd_dimming_aor_table, init_hmd_aor_table, getidx_dimming_maptbl, copy_common_maptbl),
#endif
};

static unsigned int beyond1_a3_s0_hmd_brt_tbl[S6E3HA9_HMD_NR_LUMINANCE] = {
	BRT(26), BRT(29), BRT(31), BRT(33), BRT(36), BRT(38), BRT(41), BRT(46), BRT(48), BRT(50),
	BRT(53), BRT(55), BRT(60), BRT(65), BRT(70), BRT(75), BRT(80), BRT(84), BRT(89), BRT(94),
	BRT(99), BRT(106), BRT(114), BRT(121), BRT(128), BRT(135), BRT(145), BRT(155), BRT(165), BRT(174),
	BRT(186), BRT(199), BRT(211), BRT(225), BRT(240), BRT(254), BRT(255),
};

static unsigned int beyond1_a3_s0_hmd_lum_tbl[S6E3HA9_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

static struct brightness_table s6e3ha9_beyond1_a3_s0_panel_hmd_brightness_table = {
	.brt = beyond1_a3_s0_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(beyond1_a3_s0_hmd_brt_tbl),
	.lum = beyond1_a3_s0_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(beyond1_a3_s0_hmd_lum_tbl),
	.sz_ui_lum = S6E3HA9_HMD_NR_LUMINANCE,
	.sz_hbm_lum = 0,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = beyond1_a3_s0_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(beyond1_a3_s0_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3ha9_beyond1_a3_s0_preliminary_panel_hmd_dimming_info = {
	.name = "s6e3ha9_beyond1_a3_s0_preliminary_hmd",
	.dim_init_info = {
		.name = "s6e3ha9_beyond1_a3_s0_preliminary_hmd",
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
		.dim_lut = beyond1_a3_s0_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA9_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha9_beyond1_a3_s0_panel_hmd_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyond1_a3_s0_hmd_dimming_param_maptbl,
	.dim_flash_on = false,	/* read dim flash when probe or not */
};

static struct panel_dimming_info s6e3ha9_beyond1_a3_s0_panel_hmd_dimming_info = {
	.name = "s6e3ha9_beyond1_a3_s0_hmd",
	.dim_init_info = {
		.name = "s6e3ha9_beyond1_a3_s0_hmd",
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
		.dim_lut = beyond1_a3_s0_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA9_BEYOND_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA9_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha9_beyond1_a3_s0_panel_hmd_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyond1_a3_s0_hmd_dimming_param_maptbl,
	.dim_flash_on = true,	/* read dim flash when probe or not */
};
#endif /* __S6E3HA9_BEYOND1_A3_S0_PANEL_HMD_DIMMING_H__ */
