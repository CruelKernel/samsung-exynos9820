/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_star_a3_s1_panel_hmd_dimming.h
 *
 * Header file for S6E3HA8 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_STAR_A3_S1_PANEL_HMD_DIMMING_H__
#define __S6E3HA8_STAR_A3_S1_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha8_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA8
 * PANEL : STAR_A3_S1
 */
static s32 star_a3_s1_hmd_rtbl10nit[S6E3HA8_NR_TP] = { 0, 0, 0, 11, 9, 8, 7, 6, 4, 2, 2, 0 };
static s32 star_a3_s1_hmd_rtbl11nit[S6E3HA8_NR_TP] = { 0, 0, 0, 11, 10, 8, 7, 6, 3, 2, 1, 0 };
static s32 star_a3_s1_hmd_rtbl12nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 7, 7, 6, 4, 3, 1, 0 };
static s32 star_a3_s1_hmd_rtbl13nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 7, 6, 5, 2, 2, 0, 0 };
static s32 star_a3_s1_hmd_rtbl14nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 8, 6, 5, 3, 2, -1, 0 };
static s32 star_a3_s1_hmd_rtbl15nit[S6E3HA8_NR_TP] = { 0, 0, 0, 11, 9, 7, 6, 5, 3, 2, 0, 0 };
static s32 star_a3_s1_hmd_rtbl16nit[S6E3HA8_NR_TP] = { 0, 0, 0, 11, 9, 7, 6, 5, 3, 2, 0, 0 };
static s32 star_a3_s1_hmd_rtbl17nit[S6E3HA8_NR_TP] = { 0, 0, 0, 11, 10, 7, 5, 5, 3, 1, 0, 0 };
static s32 star_a3_s1_hmd_rtbl19nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 7, 5, 4, 3, 2, 1, 0 };
static s32 star_a3_s1_hmd_rtbl20nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 7, 6, 4, 1, 1, 0, 0 };
static s32 star_a3_s1_hmd_rtbl21nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 7, 5, 4, 2, 1, 1, 0 };
static s32 star_a3_s1_hmd_rtbl22nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 10, 6, 5, 4, 2, 2, 2, 0 };
static s32 star_a3_s1_hmd_rtbl23nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 5, 4, 1, 1, 2, 0 };
static s32 star_a3_s1_hmd_rtbl25nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 10, 7, 6, 4, 2, 1, 1, 0 };
static s32 star_a3_s1_hmd_rtbl27nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 6, 5, 3, 1, 1, 2, 0 };
static s32 star_a3_s1_hmd_rtbl29nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 6, 6, 4, 2, 2, 3, 0 };
static s32 star_a3_s1_hmd_rtbl31nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 9, 5, 5, 3, 3, 2, 3, 0 };
static s32 star_a3_s1_hmd_rtbl33nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 9, 6, 5, 3, 3, 3, 3, 0 };
static s32 star_a3_s1_hmd_rtbl35nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 9, 6, 6, 3, 2, 3, 4, 0 };
static s32 star_a3_s1_hmd_rtbl37nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 5, 5, 4, 3, 3, 3, 0 };
static s32 star_a3_s1_hmd_rtbl39nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 6, 6, 4, 4, 4, 3, 0 };
static s32 star_a3_s1_hmd_rtbl41nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 6, 5, 4, 3, 5, 3, 0 };
static s32 star_a3_s1_hmd_rtbl44nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 5, 4, 4, 4, 5, 3, 0 };
static s32 star_a3_s1_hmd_rtbl47nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 9, 6, 5, 4, 4, 7, 4, 0 };
static s32 star_a3_s1_hmd_rtbl50nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 4, 5, 4, 0 };
static s32 star_a3_s1_hmd_rtbl53nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 8, 5, 4, 3, 4, 6, 4, 0 };
static s32 star_a3_s1_hmd_rtbl56nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 4, 6, 5, 0 };
static s32 star_a3_s1_hmd_rtbl60nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 8, 5, 5, 4, 5, 7, 5, 0 };
static s32 star_a3_s1_hmd_rtbl64nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 8, 5, 4, 4, 6, 8, 5, 0 };
static s32 star_a3_s1_hmd_rtbl68nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 8, 6, 5, 4, 5, 7, 6, 0 };
static s32 star_a3_s1_hmd_rtbl72nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 8, 5, 5, 5, 5, 8, 8, 0 };
static s32 star_a3_s1_hmd_rtbl77nit[S6E3HA8_NR_TP] = { 0, 0, 0, 5, 4, 3, 2, 1, 3, 5, 3, 0 };
static s32 star_a3_s1_hmd_rtbl82nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 5, 2, 3, 2, 3, 5, 4, 0 };
static s32 star_a3_s1_hmd_rtbl87nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 5, 3, 3, 2, 2, 5, 5, 0 };
static s32 star_a3_s1_hmd_rtbl93nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 4, 2, 2, 1, 3, 5, 4, 0 };
static s32 star_a3_s1_hmd_rtbl99nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 4, 3, 3, 2, 4, 6, 5, 0 };
static s32 star_a3_s1_hmd_rtbl105nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 4, 2, 2, 2, 3, 7, 8, 0 };

static s32 star_a3_s1_hmd_ctbl10nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -74, -2, -2, -24, 0, 0, -10, 2, -4, -6, 2, -4, -4, 1, -3, -2, 0, -1, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl11nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -70, -4, -4, -27, 0, 0, -10, 1, -4, -5, 2, -4, -4, 1, -4, -2, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_s1_hmd_ctbl12nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -68, -2, -3, -23, 0, 0, -10, 1, -4, -4, 2, -4, -4, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_s1_hmd_ctbl13nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -65, -2, -3, -23, 0, -1, -8, 1, -3, -6, 2, -4, -3, 1, -3, -2, 0, -1, 1, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl14nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -65, -1, -1, -17, 0, -1, -8, 1, -4, -5, 2, -4, -3, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl15nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -65, 0, -2, -18, 0, -1, -9, 1, -4, -4, 1, -4, -4, 1, -3, -1, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_s1_hmd_ctbl16nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, -65, 0, -5, -16, 0, -1, -9, 1, -4, -3, 1, -4, -3, 1, -3, -1, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl17nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -60, 0, -4, -14, 0, -1, -10, 1, -4, -3, 1, -3, -3, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl19nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -55, 0, -4, -11, 1, -2, -8, 1, -4, -4, 1, -3, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 star_a3_s1_hmd_ctbl20nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -50, 0, -4, -11, 1, -2, -7, 1, -4, -3, 1, -3, -3, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl21nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -45, 0, -4, -12, 1, -3, -8, 1, -4, -3, 1, -3, -3, 1, -3, 0, 0, 0, 1, 0, 1, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl22nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -40, 0, -4, -13, 1, -3, -8, 1, -4, -3, 1, -3, -3, 1, -2, 0, 0, -1, 1, 0, 0, 1, 0, 3 };
static s32 star_a3_s1_hmd_ctbl23nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -35, 0, -3, -12, 1, -3, -6, 1, -3, -3, 1, -3, -3, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl25nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -30, 0, -3, -11, 1, -3, -5, 1, -3, -4, 1, -3, -2, 1, -2, 0, 0, 0, 0, 0, 1, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl27nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -25, 0, -3, -10, 1, -4, -4, 1, -2, -4, 1, -3, -1, 1, -2, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl29nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, 0, -2, -10, 1, -4, -4, 1, -3, -4, 1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_hmd_ctbl31nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, 0, -2, -10, 1, -4, -4, 1, -3, -4, 1, -3, 0, 1, -2, 0, 0, 0, 0, 0, 1, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl33nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -21, 1, -2, -9, 1, -3, -4, 1, -4, -3, 1, -2, -1, 1, -2, -1, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl35nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -19, 0, -2, -10, 1, -4, -3, 1, -3, -4, 1, -3, -2, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl37nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, -17, 0, -1, -10, 1, -4, -3, 1, -3, -4, 1, -3, -2, 0, -1, -1, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl39nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, -19, 1, -2, -9, 1, -3, -3, 1, -4, -3, 1, -2, -2, 0, -2, 0, 0, 0, 1, 0, 1, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl41nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, -21, 1, -2, -8, 1, -3, -3, 1, -3, -3, 1, -2, -3, 0, -2, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl44nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, -20, 1, -2, -7, 1, -3, -4, 1, -3, -2, 1, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl47nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, -20, 1, -4, -6, 1, -3, -3, 1, -2, -2, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 star_a3_s1_hmd_ctbl50nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -26, 1, -4, -9, 1, -4, -4, 1, -3, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl53nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, -23, 1, -4, -8, 1, -4, -2, 1, -2, -3, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl56nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, -2, -23, 1, -3, -7, 1, -4, -4, 1, -3, -2, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl60nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -24, 1, -4, -7, 1, -3, -3, 1, -3, -3, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 star_a3_s1_hmd_ctbl64nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -9, 0, -2, -18, 2, -4, -5, 1, -2, -4, 1, -3, -2, 1, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 star_a3_s1_hmd_ctbl68nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, -1, -19, 2, -4, -5, 1, -3, -3, 1, -3, -3, 0, -2, -1, 0, -1, -1, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_hmd_ctbl72nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, -1, -19, 1, -4, -6, 1, -3, -3, 1, -2, -1, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl77nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -40, 1, -2, -31, 0, -2, -3, 1, -3, -1, 0, -2, -2, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl82nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -31, 0, -2, -23, 1, -2, -3, 1, -3, -3, 0, -2, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_hmd_ctbl87nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -36, 0, -2, -26, 1, -3, -2, 1, -2, -2, 0, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 star_a3_s1_hmd_ctbl93nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -33, 0, -2, -23, 1, -3, -2, 1, -2, -2, 0, -2, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2, 0, 2 };
static s32 star_a3_s1_hmd_ctbl99nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -34, 0, -2, -24, 1, -3, -4, 0, -2, -2, 0, -2, -2, 0, -2, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_hmd_ctbl105nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -32, 0, -2, -21, 1, -3, -3, 1, -2, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 };

static struct dimming_lut star_a3_s1_hmd_dimming_lut[] = {
	DIM_LUT_V0_INIT(10, 42, GAMMA_2_15, star_a3_s1_hmd_rtbl10nit, star_a3_s1_hmd_ctbl10nit),
	DIM_LUT_V0_INIT(11, 47, GAMMA_2_15, star_a3_s1_hmd_rtbl11nit, star_a3_s1_hmd_ctbl11nit),
	DIM_LUT_V0_INIT(12, 52, GAMMA_2_15, star_a3_s1_hmd_rtbl12nit, star_a3_s1_hmd_ctbl12nit),
	DIM_LUT_V0_INIT(13, 57, GAMMA_2_15, star_a3_s1_hmd_rtbl13nit, star_a3_s1_hmd_ctbl13nit),
	DIM_LUT_V0_INIT(14, 61, GAMMA_2_15, star_a3_s1_hmd_rtbl14nit, star_a3_s1_hmd_ctbl14nit),
	DIM_LUT_V0_INIT(15, 66, GAMMA_2_15, star_a3_s1_hmd_rtbl15nit, star_a3_s1_hmd_ctbl15nit),
	DIM_LUT_V0_INIT(16, 69, GAMMA_2_15, star_a3_s1_hmd_rtbl16nit, star_a3_s1_hmd_ctbl16nit),
	DIM_LUT_V0_INIT(17, 73, GAMMA_2_15, star_a3_s1_hmd_rtbl17nit, star_a3_s1_hmd_ctbl17nit),
	DIM_LUT_V0_INIT(19, 82, GAMMA_2_15, star_a3_s1_hmd_rtbl19nit, star_a3_s1_hmd_ctbl19nit),
	DIM_LUT_V0_INIT(20, 88, GAMMA_2_15, star_a3_s1_hmd_rtbl20nit, star_a3_s1_hmd_ctbl20nit),
	DIM_LUT_V0_INIT(21, 91, GAMMA_2_15, star_a3_s1_hmd_rtbl21nit, star_a3_s1_hmd_ctbl21nit),
	DIM_LUT_V0_INIT(22, 95, GAMMA_2_15, star_a3_s1_hmd_rtbl22nit, star_a3_s1_hmd_ctbl22nit),
	DIM_LUT_V0_INIT(23, 101, GAMMA_2_15, star_a3_s1_hmd_rtbl23nit, star_a3_s1_hmd_ctbl23nit),
	DIM_LUT_V0_INIT(25, 107, GAMMA_2_15, star_a3_s1_hmd_rtbl25nit, star_a3_s1_hmd_ctbl25nit),
	DIM_LUT_V0_INIT(27, 115, GAMMA_2_15, star_a3_s1_hmd_rtbl27nit, star_a3_s1_hmd_ctbl27nit),
	DIM_LUT_V0_INIT(29, 122, GAMMA_2_15, star_a3_s1_hmd_rtbl29nit, star_a3_s1_hmd_ctbl29nit),
	DIM_LUT_V0_INIT(31, 130, GAMMA_2_15, star_a3_s1_hmd_rtbl31nit, star_a3_s1_hmd_ctbl31nit),
	DIM_LUT_V0_INIT(33, 138, GAMMA_2_15, star_a3_s1_hmd_rtbl33nit, star_a3_s1_hmd_ctbl33nit),
	DIM_LUT_V0_INIT(35, 147, GAMMA_2_15, star_a3_s1_hmd_rtbl35nit, star_a3_s1_hmd_ctbl35nit),
	DIM_LUT_V0_INIT(37, 156, GAMMA_2_15, star_a3_s1_hmd_rtbl37nit, star_a3_s1_hmd_ctbl37nit),
	DIM_LUT_V0_INIT(39, 163, GAMMA_2_15, star_a3_s1_hmd_rtbl39nit, star_a3_s1_hmd_ctbl39nit),
	DIM_LUT_V0_INIT(41, 171, GAMMA_2_15, star_a3_s1_hmd_rtbl41nit, star_a3_s1_hmd_ctbl41nit),
	DIM_LUT_V0_INIT(44, 182, GAMMA_2_15, star_a3_s1_hmd_rtbl44nit, star_a3_s1_hmd_ctbl44nit),
	DIM_LUT_V0_INIT(47, 192, GAMMA_2_15, star_a3_s1_hmd_rtbl47nit, star_a3_s1_hmd_ctbl47nit),
	DIM_LUT_V0_INIT(50, 204, GAMMA_2_15, star_a3_s1_hmd_rtbl50nit, star_a3_s1_hmd_ctbl50nit),
	DIM_LUT_V0_INIT(53, 214, GAMMA_2_15, star_a3_s1_hmd_rtbl53nit, star_a3_s1_hmd_ctbl53nit),
	DIM_LUT_V0_INIT(56, 225, GAMMA_2_15, star_a3_s1_hmd_rtbl56nit, star_a3_s1_hmd_ctbl56nit),
	DIM_LUT_V0_INIT(60, 239, GAMMA_2_15, star_a3_s1_hmd_rtbl60nit, star_a3_s1_hmd_ctbl60nit),
	DIM_LUT_V0_INIT(64, 253, GAMMA_2_15, star_a3_s1_hmd_rtbl64nit, star_a3_s1_hmd_ctbl64nit),
	DIM_LUT_V0_INIT(68, 265, GAMMA_2_15, star_a3_s1_hmd_rtbl68nit, star_a3_s1_hmd_ctbl68nit),
	DIM_LUT_V0_INIT(72, 279, GAMMA_2_15, star_a3_s1_hmd_rtbl72nit, star_a3_s1_hmd_ctbl72nit),
	DIM_LUT_V0_INIT(77, 214, GAMMA_2_15, star_a3_s1_hmd_rtbl77nit, star_a3_s1_hmd_ctbl77nit),
	DIM_LUT_V0_INIT(82, 226, GAMMA_2_15, star_a3_s1_hmd_rtbl82nit, star_a3_s1_hmd_ctbl82nit),
	DIM_LUT_V0_INIT(87, 239, GAMMA_2_15, star_a3_s1_hmd_rtbl87nit, star_a3_s1_hmd_ctbl87nit),
	DIM_LUT_V0_INIT(93, 252, GAMMA_2_15, star_a3_s1_hmd_rtbl93nit, star_a3_s1_hmd_ctbl93nit),
	DIM_LUT_V0_INIT(99, 264, GAMMA_2_15, star_a3_s1_hmd_rtbl99nit, star_a3_s1_hmd_ctbl99nit),
	DIM_LUT_V0_INIT(105, 279, GAMMA_2_15, star_a3_s1_hmd_rtbl105nit, star_a3_s1_hmd_ctbl105nit),
};

static unsigned int star_a3_s1_hmd_brt_tbl[S6E3HA8_HMD_NR_LUMINANCE] = {
	BRT(0), BRT(27), BRT(30), BRT(32), BRT(34), BRT(37), BRT(39), BRT(42), BRT(47), BRT(49),
	BRT(51), BRT(54), BRT(56), BRT(61), BRT(66), BRT(71), BRT(76), BRT(81), BRT(85), BRT(90),
	BRT(95), BRT(100), BRT(107), BRT(115), BRT(122), BRT(129), BRT(136), BRT(146), BRT(156), BRT(166),
	BRT(175), BRT(187), BRT(200), BRT(212), BRT(226), BRT(241), BRT(255),
};

static unsigned int star_a3_s1_hmd_lum_tbl[S6E3HA8_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

static struct brightness_table s6e3ha8_star_a3_s1_panel_hmd_brightness_table = {
	.brt = star_a3_s1_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(star_a3_s1_hmd_brt_tbl),
	.lum = star_a3_s1_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(star_a3_s1_hmd_lum_tbl),
	.sz_ui_lum = S6E3HA8_HMD_NR_LUMINANCE,
	.sz_hbm_lum = 0,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = star_a3_s1_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(star_a3_s1_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3ha8_star_a3_s1_panel_hmd_dimming_info = {
	.name = "s6e3ha8_star_a3_s1_hmd",
	.dim_init_info = {
		.name = "s6e3ha8_star_a3_s1_hmd",
		.nr_tp = S6E3HA8_NR_TP,
		.tp = s6e3ha8_tp,
		.nr_luminance = S6E3HA8_HMD_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HA8_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = star_a3_s1_hmd_dimming_lut,
	},
	.target_luminance = S6E3HA8_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA8_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha8_star_a3_s1_panel_hmd_brightness_table,
};
#endif /* __S6E3HA8_STAR_A3_S1_PANEL_HMD_DIMMING_H__ */
