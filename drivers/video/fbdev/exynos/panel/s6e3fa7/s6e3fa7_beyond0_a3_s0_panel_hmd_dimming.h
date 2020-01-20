/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7_beyond0_a3_s0_panel_hmd_dimming.h
 *
 * Header file for S6E3FA7 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA7_BEYOND0_A3_S0_PANEL_HMD_DIMMING_H__
#define __S6E3FA7_BEYOND0_A3_S0_PANEL_HMD_DIMMING_H__
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3fa7_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3FA7
 * PANEL : BEYOND0_A3_S0
 */
static s32 beyond0_a3_s0_hmd_rtbl10nit[S6E3FA7_NR_TP] = { 0, 0, 13, 12, 10, 8, 6, 4, 2, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl11nit[S6E3FA7_NR_TP] = { 0, 0, 13, 12, 10, 8, 6, 4, 2, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl12nit[S6E3FA7_NR_TP] = { 0, 0, 13, 12, 9, 8, 6, 4, 2, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl13nit[S6E3FA7_NR_TP] = { 0, 0, 13, 12, 10, 8, 6, 4, 3, 0, 0 };
static s32 beyond0_a3_s0_hmd_rtbl14nit[S6E3FA7_NR_TP] = { 0, 0, 13, 12, 10, 7, 5, 3, 2, 0, 0 };
static s32 beyond0_a3_s0_hmd_rtbl15nit[S6E3FA7_NR_TP] = { 0, 0, 13, 11, 9, 7, 6, 3, 2, 0, 0 };
static s32 beyond0_a3_s0_hmd_rtbl16nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 9, 7, 6, 3, 2, 0, 0 };
static s32 beyond0_a3_s0_hmd_rtbl17nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 9, 7, 6, 3, 1, 0, 0 };
static s32 beyond0_a3_s0_hmd_rtbl19nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 9, 7, 6, 3, 1, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl20nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 9, 7, 6, 2, 2, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl21nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 8, 7, 5, 2, 1, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl22nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 8, 7, 5, 2, 1, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl23nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 8, 6, 5, 2, 1, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl25nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 8, 7, 5, 3, 2, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl27nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 8, 7, 6, 3, 3, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl29nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 8, 7, 5, 3, 3, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl31nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 7, 7, 5, 3, 3, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl33nit[S6E3FA7_NR_TP] = { 0, 0, 12, 11, 7, 7, 5, 3, 3, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl35nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 7, 5, 3, 3, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl37nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 6, 5, 4, 4, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl39nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 6, 4, 4, 4, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl41nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 6, 6, 5, 4, 5, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl44nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 5, 5, 4, 5, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl47nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 6, 5, 4, 5, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl50nit[S6E3FA7_NR_TP] = { 0, 0, 11, 9, 6, 5, 4, 4, 5, 1, 0 };
static s32 beyond0_a3_s0_hmd_rtbl53nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 6, 5, 5, 6, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl56nit[S6E3FA7_NR_TP] = { 0, 0, 11, 9, 6, 6, 5, 5, 7, 3, 0 };
static s32 beyond0_a3_s0_hmd_rtbl60nit[S6E3FA7_NR_TP] = { 0, 0, 12, 10, 7, 6, 5, 5, 6, 3, 0 };
static s32 beyond0_a3_s0_hmd_rtbl64nit[S6E3FA7_NR_TP] = { 0, 0, 11, 9, 6, 5, 5, 5, 6, 3, 0 };
static s32 beyond0_a3_s0_hmd_rtbl68nit[S6E3FA7_NR_TP] = { 0, 0, 12, 9, 6, 6, 5, 5, 8, 5, 0 };
static s32 beyond0_a3_s0_hmd_rtbl72nit[S6E3FA7_NR_TP] = { 0, 0, 12, 9, 6, 6, 5, 5, 7, 6, 0 };
static s32 beyond0_a3_s0_hmd_rtbl77nit[S6E3FA7_NR_TP] = { 0, 0, 7, 5, 4, 3, 3, 3, 5, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl82nit[S6E3FA7_NR_TP] = { 0, 0, 7, 6, 3, 3, 3, 3, 5, 2, 0 };
static s32 beyond0_a3_s0_hmd_rtbl87nit[S6E3FA7_NR_TP] = { 0, 0, 7, 6, 4, 3, 3, 4, 6, 3, 0 };
static s32 beyond0_a3_s0_hmd_rtbl93nit[S6E3FA7_NR_TP] = { 0, 0, 7, 5, 3, 3, 4, 4, 6, 4, 0 };
static s32 beyond0_a3_s0_hmd_rtbl99nit[S6E3FA7_NR_TP] = { 0, 0, 8, 6, 4, 3, 4, 4, 6, 4, 0 };
static s32 beyond0_a3_s0_hmd_rtbl105nit[S6E3FA7_NR_TP] = { 0, 0, 8, 6, 3, 3, 4, 5, 7, 5, 0 };

static s32 beyond0_a3_s0_hmd_ctbl10nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -11, 1, -4, -7, 2, -6, -10, 4, -9, -11, 1, -11, -11, 3, -11, -4, 2, -4, -3, 0, -2, -1, 0, -1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl11nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -3, -10, 0, -8, -12, 3, -9, -11, 1, -12, -10, 3, -11, -5, 2, -4, -1, 1, 0, -1, -1, -1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl12nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -3, -10, 0, -8, -12, 3, -10, -10, 1, -10, -9, 3, -10, -5, 1, -5, -2, 0, -2, -1, -1, -1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl13nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -2, -7, 3, -6, -11, 2, -10, -9, 2, -8, -8, 4, -9, -4, 0, -4, -1, -1, -2, -2, -2, -2, -1, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl14nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -2, -8, 4, -9, -12, 1, -11, -12, 1, -12, -9, 2, -10, -4, 2, -4, 0, 0, 0, -2, -1, -1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl15nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -4, -7, 3, -9, -8, 2, -9, -10, 4, -10, -10, 1, -10, -4, 1, -4, -1, 0, -1, -1, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl16nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, 1, -4, -5, 4, -10, -7, 2, -7, -10, 3, -11, -9, 1, -9, -4, 0, -4, -1, 0, 0, -1, -1, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_hmd_ctbl17nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -5, 2, -8, -9, 4, -10, -7, 3, -7, -8, 2, -8, -3, 0, -3, -1, 0, -2, -1, -1, -1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl19nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -3, 3, -8, -10, 3, -9, -9, 2, -9, -9, 1, -9, -3, 0, -4, -1, 0, -1, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl20nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -3, -10, 4, -8, -10, 4, -8, -8, 2, -10, -9, -1, -9, -2, 1, -3, -1, 0, -1, 0, 0, 0, -1, 0, 0 };
static s32 beyond0_a3_s0_hmd_ctbl21nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -10, 3, -8, -9, 3, -8, -9, 1, -10, -8, 0, -9, -2, 1, -2, 0, 0, 0, 0, 0, 0, -1, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl22nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -4, -10, 3, -10, -7, 3, -7, -7, 2, -7, -8, 0, -7, -2, 1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_hmd_ctbl23nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, 1, -4, -11, 3, -11, -6, 4, -6, -7, 3, -8, -7, 0, -7, -2, 1, -3, -2, -1, -2, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl25nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -3, -11, 2, -12, -8, 3, -8, -8, 2, -9, -9, 0, -8, -1, 1, -2, -2, -1, -1, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl27nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -5, 1, -4, -10, 3, -10, -8, 3, -10, -8, 2, -7, -8, 0, -8, -2, 1, -2, -2, -1, -1, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl29nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -4, -10, 3, -10, -7, 4, -10, -8, 2, -8, -9, 0, -8, -2, 0, -3, -1, -1, -1, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl31nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -4, -12, 3, -12, -8, 3, -8, -7, 2, -7, -8, 0, -7, 0, 1, -2, 0, 0, 0, 0, 0, 1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl33nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -8, 4, -10, -8, 3, -8, -6, 2, -7, -7, -1, -8, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl35nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 0, -4, -11, 4, -11, -9, 3, -8, -5, 1, -6, -8, -1, -8, -2, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl37nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 0, -4, -10, 4, -12, -7, 3, -9, -7, 1, -6, -7, 0, -6, -2, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl39nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 0, -4, -10, 4, -12, -7, 3, -9, -7, 1, -6, -7, 0, -6, -4, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl41nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 0, -4, -10, 4, -12, -8, 3, -8, -5, 2, -6, -6, 1, -5, -3, 0, -2, -1, 0, 0, 1, 0, 0, 1, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl44nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 0, -4, -11, 4, -12, -8, 3, -8, -5, 2, -6, -4, 2, -4, -1, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl47nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, 0, -6, -10, 4, -10, -8, 3, -8, -4, 2, -6, -6, 1, -5, -2, 0, -2, 0, 0, 0, 1, 0, 0, 0, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl50nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -2, 0, -4, -12, 4, -12, -8, 3, -7, -5, 2, -5, -5, 1, -4, -2, 0, -1, -1, 0, 0, 0, 0, 0, 1, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl53nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -5, 0, -4, -11, 4, -10, -8, 3, -8, -5, 0, -6, -4, 0, -4, -2, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl56nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -5, 0, -4, -14, 4, -14, -7, 3, -7, -4, 2, -5, -4, 1, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl60nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -5, 0, -4, -14, 4, -14, -7, 3, -7, -4, 2, -6, -3, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl64nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -5, -14, 4, -14, -5, 3, -6, -4, 2, -5, -4, 1, -3, 0, 0, -1, 1, 0, 0, 0, 0, 0, -1, 0, 0 };
static s32 beyond0_a3_s0_hmd_ctbl68nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -5, -14, 4, -14, -6, 4, -6, -5, 2, -5, -4, 1, -4, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
static s32 beyond0_a3_s0_hmd_ctbl72nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, 2, -6, -13, 6, -12, -6, 3, -6, -5, 1, -5, -3, 0, -4, -2, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl77nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 1, -4, -11, 5, -12, -6, 2, -5, -5, 1, -4, -2, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl82nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -11, 5, -11, -5, 2, -6, -5, 0, -4, -3, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl87nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -10, 5, -11, -5, 2, -6, -3, 1, -4, -2, 0, -2, -1, 0, -1, 0, 0, -1, 0, 0, 0, 1, 0, 3 };
static s32 beyond0_a3_s0_hmd_ctbl93nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -4, -12, 5, -11, -4, 2, -5, -3, 1, -3, -1, 1, -2, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl99nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, 2, -6, -11, 4, -11, -5, 2, -5, -3, 1, -4, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, 0, 2 };
static s32 beyond0_a3_s0_hmd_ctbl105nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, 2, -6, -11, 4, -11, -5, 2, -5, -3, 1, -4, -2, 0, -3, -2, 0, -1, -1, 0, 0, 0, 0, 0, 2, 0, 2 };


static struct dimming_lut beyond0_a3_s0_hmd_dimming_lut[] = {
	DIM_LUT_V0_INIT(10, 48, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl10nit, beyond0_a3_s0_hmd_ctbl10nit),
	DIM_LUT_V0_INIT(11, 52, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl11nit, beyond0_a3_s0_hmd_ctbl11nit),
	DIM_LUT_V0_INIT(12, 57, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl12nit, beyond0_a3_s0_hmd_ctbl12nit),
	DIM_LUT_V0_INIT(13, 62, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl13nit, beyond0_a3_s0_hmd_ctbl13nit),
	DIM_LUT_V0_INIT(14, 66, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl14nit, beyond0_a3_s0_hmd_ctbl14nit),
	DIM_LUT_V0_INIT(15, 71, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl15nit, beyond0_a3_s0_hmd_ctbl15nit),
	DIM_LUT_V0_INIT(16, 76, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl16nit, beyond0_a3_s0_hmd_ctbl16nit),
	DIM_LUT_V0_INIT(17, 80, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl17nit, beyond0_a3_s0_hmd_ctbl17nit),
	DIM_LUT_V0_INIT(19, 90, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl19nit, beyond0_a3_s0_hmd_ctbl19nit),
	DIM_LUT_V0_INIT(20, 94, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl20nit, beyond0_a3_s0_hmd_ctbl20nit),
	DIM_LUT_V0_INIT(21, 98, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl21nit, beyond0_a3_s0_hmd_ctbl21nit),
	DIM_LUT_V0_INIT(22, 102, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl22nit, beyond0_a3_s0_hmd_ctbl22nit),
	DIM_LUT_V0_INIT(23, 106, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl23nit, beyond0_a3_s0_hmd_ctbl23nit),
	DIM_LUT_V0_INIT(25, 115, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl25nit, beyond0_a3_s0_hmd_ctbl25nit),
	DIM_LUT_V0_INIT(27, 123, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl27nit, beyond0_a3_s0_hmd_ctbl27nit),
	DIM_LUT_V0_INIT(29, 130, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl29nit, beyond0_a3_s0_hmd_ctbl29nit),
	DIM_LUT_V0_INIT(31, 140, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl31nit, beyond0_a3_s0_hmd_ctbl31nit),
	DIM_LUT_V0_INIT(33, 149, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl33nit, beyond0_a3_s0_hmd_ctbl33nit),
	DIM_LUT_V0_INIT(35, 157, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl35nit, beyond0_a3_s0_hmd_ctbl35nit),
	DIM_LUT_V0_INIT(37, 166, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl37nit, beyond0_a3_s0_hmd_ctbl37nit),
	DIM_LUT_V0_INIT(39, 175, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl39nit, beyond0_a3_s0_hmd_ctbl39nit),
	DIM_LUT_V0_INIT(41, 181, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl41nit, beyond0_a3_s0_hmd_ctbl41nit),
	DIM_LUT_V0_INIT(44, 192, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl44nit, beyond0_a3_s0_hmd_ctbl44nit),
	DIM_LUT_V0_INIT(47, 204, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl47nit, beyond0_a3_s0_hmd_ctbl47nit),
	DIM_LUT_V0_INIT(50, 216, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl50nit, beyond0_a3_s0_hmd_ctbl50nit),
	DIM_LUT_V0_INIT(53, 228, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl53nit, beyond0_a3_s0_hmd_ctbl53nit),
	DIM_LUT_V0_INIT(56, 240, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl56nit, beyond0_a3_s0_hmd_ctbl56nit),
	DIM_LUT_V0_INIT(60, 257, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl60nit, beyond0_a3_s0_hmd_ctbl60nit),
	DIM_LUT_V0_INIT(64, 269, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl64nit, beyond0_a3_s0_hmd_ctbl64nit),
	DIM_LUT_V0_INIT(68, 285, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl68nit, beyond0_a3_s0_hmd_ctbl68nit),
	DIM_LUT_V0_INIT(72, 297, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl72nit, beyond0_a3_s0_hmd_ctbl72nit),
	DIM_LUT_V0_INIT(77, 230, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl77nit, beyond0_a3_s0_hmd_ctbl77nit),
	DIM_LUT_V0_INIT(82, 244, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl82nit, beyond0_a3_s0_hmd_ctbl82nit),
	DIM_LUT_V0_INIT(87, 256, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl87nit, beyond0_a3_s0_hmd_ctbl87nit),
	DIM_LUT_V0_INIT(93, 271, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl93nit, beyond0_a3_s0_hmd_ctbl93nit),
	DIM_LUT_V0_INIT(99, 287, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl99nit, beyond0_a3_s0_hmd_ctbl99nit),
	DIM_LUT_V0_INIT(105, 299, GAMMA_2_15, beyond0_a3_s0_hmd_rtbl105nit, beyond0_a3_s0_hmd_ctbl105nit),
};

static u8 beyond0_a3_s0_hmd_dimming_gamma_table[S6E3FA7_HMD_NR_LUMINANCE][S6E3FA7_GAMMA_CMD_CNT - 1];
static u8 beyond0_a3_s0_hmd_dimming_aor_table[S6E3FA7_HMD_NR_LUMINANCE][2] = {
	{ 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD },
	{ 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD },
	{ 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD }, { 0x01, 0xCD },
	{ 0x01, 0xCD }, { 0x02, 0xB4 }, { 0x02, 0xB4 }, { 0x02, 0xB4 }, { 0x02, 0xB4 }, { 0x02, 0xB4 }, { 0x02, 0xB4 },
};

static struct maptbl beyond0_a3_s0_hmd_dimming_param_maptbl[MAX_DIMMING_MAPTBL] = {
#ifdef CONFIG_SUPPORT_HMD
	[DIMMING_GAMMA_MAPTBL] = DEFINE_2D_MAPTBL(beyond0_a3_s0_hmd_dimming_gamma_table, init_hmd_gamma_table, getidx_dimming_maptbl, copy_common_maptbl),
	[DIMMING_AOR_MAPTBL] = DEFINE_2D_MAPTBL(beyond0_a3_s0_hmd_dimming_aor_table, init_hmd_aor_table, getidx_dimming_maptbl, copy_common_maptbl),
#endif
};

static unsigned int beyond0_a3_s0_hmd_brt_tbl[S6E3FA7_HMD_NR_LUMINANCE] = {
	BRT(26), BRT(29), BRT(31), BRT(33), BRT(36), BRT(38), BRT(41), BRT(46), BRT(48), BRT(50),
	BRT(53), BRT(55), BRT(60), BRT(65), BRT(70), BRT(75), BRT(80), BRT(84), BRT(89), BRT(94),
	BRT(99), BRT(106), BRT(114), BRT(121), BRT(128), BRT(135), BRT(145), BRT(155), BRT(165), BRT(174),
	BRT(186), BRT(199), BRT(211), BRT(225), BRT(240), BRT(254), BRT(255),
};

static unsigned int beyond0_a3_s0_hmd_lum_tbl[S6E3FA7_HMD_NR_LUMINANCE] = {
	10, 11, 12, 13, 14, 15, 16, 17, 19, 20,
	21, 22, 23, 25, 27, 29, 31, 33, 35, 37,
	39, 41, 44, 47, 50, 53, 56, 60, 64, 68,
	72, 77, 82, 87, 93, 99, 105,
};

static struct brightness_table s6e3fa7_beyond0_a3_s0_panel_hmd_brightness_table = {
	.brt = beyond0_a3_s0_hmd_brt_tbl,
	.sz_brt = ARRAY_SIZE(beyond0_a3_s0_hmd_brt_tbl),
	.lum = beyond0_a3_s0_hmd_lum_tbl,
	.sz_lum = ARRAY_SIZE(beyond0_a3_s0_hmd_lum_tbl),
	.sz_ui_lum = S6E3FA7_HMD_NR_LUMINANCE,
	.sz_hbm_lum = 0,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = beyond0_a3_s0_hmd_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(beyond0_a3_s0_hmd_brt_tbl),
};

static struct panel_dimming_info s6e3fa7_beyond0_a3_s0_preliminary_panel_hmd_dimming_info = {
	.name = "s6e3fa7_beyond0_a3_s0_preliminary_hmd",
	.dim_init_info = {
		.name = "s6e3fa7_beyond0_a3_s0_preliminary_hmd",
		.nr_tp = S6E3FA7_NR_TP,
		.tp = s6e3fa7_beyond0_tp,
		.nr_luminance = S6E3FA7_HMD_NR_LUMINANCE,
		.vregout = 109051904LL, /* 6.5 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3FA7_BEYOND_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = beyond0_a3_s0_hmd_dimming_lut,
	},
	.target_luminance = S6E3FA7_BEYOND_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3FA7_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3fa7_beyond0_a3_s0_panel_hmd_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyond0_a3_s0_hmd_dimming_param_maptbl,
	.dim_flash_on = false,	/* read dim flash when probe or not */
};

static struct panel_dimming_info s6e3fa7_beyond0_a3_s0_panel_hmd_dimming_info = {
	.name = "s6e3fa7_beyond0_a3_s0_hmd",
	.dim_init_info = {
		.name = "s6e3fa7_beyond0_a3_s0_hmd",
		.nr_tp = S6E3FA7_NR_TP,
		.tp = s6e3fa7_beyond0_tp,
		.nr_luminance = S6E3FA7_HMD_NR_LUMINANCE,
		.vregout = 109051904LL, /* 6.5 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3FA7_BEYOND_HMD_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = beyond0_a3_s0_hmd_dimming_lut,
	},
	.target_luminance = S6E3FA7_BEYOND_HMD_TARGET_LUMINANCE,
	.nr_luminance = S6E3FA7_HMD_NR_LUMINANCE,
	.hbm_target_luminance = -1,
	.nr_hbm_luminance = 0,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3fa7_beyond0_a3_s0_panel_hmd_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyond0_a3_s0_hmd_dimming_param_maptbl,
	.dim_flash_on = true,	/* read dim flash when probe or not */
};
#endif /* __S6E3FA7_BEYOND0_A3_S0_PANEL_HMD_DIMMING_H__ */
