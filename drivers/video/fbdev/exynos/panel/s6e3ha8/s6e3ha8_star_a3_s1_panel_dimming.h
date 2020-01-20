/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_star_a3_s1_panel_dimming.h
 *
 * Header file for S6E3HA8 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_STAR_A3_S1_PANEL_DIMMING_H___
#define __S6E3HA8_STAR_A3_S1_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha8_dimming.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA8
 * PANEL : STAR_A3_S1
 */
/* gray scale offset values */
static s32 star_a3_s1_rtbl2nit[S6E3HA8_NR_TP] = { 0, 0, 0, 27, 20, 20, 20, 16, 12, 6, 4, 0 };
static s32 star_a3_s1_rtbl3nit[S6E3HA8_NR_TP] = { 0, 0, 0, 21, 18, 19, 16, 13, 9, 5, 4, 0 };
static s32 star_a3_s1_rtbl4nit[S6E3HA8_NR_TP] = { 0, 0, 0, 18, 16, 16, 13, 11, 8, 4, 3, 0 };
static s32 star_a3_s1_rtbl5nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 14, 14, 11, 9, 6, 3, 3, 0 };
static s32 star_a3_s1_rtbl6nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 13, 12, 10, 9, 6, 3, 3, 0 };
static s32 star_a3_s1_rtbl7nit[S6E3HA8_NR_TP] = { 0, 0, 0, 13, 13, 11, 9, 7, 5, 2, 2, 0 };
static s32 star_a3_s1_rtbl8nit[S6E3HA8_NR_TP] = { 0, 0, 0, 13, 13, 11, 9, 7, 5, 2, 3, 0 };
static s32 star_a3_s1_rtbl9nit[S6E3HA8_NR_TP] = { 0, 0, 0, 13, 12, 10, 9, 7, 5, 3, 3, 0 };
static s32 star_a3_s1_rtbl10nit[S6E3HA8_NR_TP] = { 0, 0, 0, 13, 12, 10, 9, 7, 4, 2, 3, 0 };
static s32 star_a3_s1_rtbl11nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 12, 10, 9, 8, 5, 3, 3, 0 };
static s32 star_a3_s1_rtbl12nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 12, 10, 9, 8, 5, 3, 3, 0 };
static s32 star_a3_s1_rtbl13nit[S6E3HA8_NR_TP] = { 0, 0, 0, 15, 13, 11, 10, 8, 5, 3, 3, 0 };
static s32 star_a3_s1_rtbl14nit[S6E3HA8_NR_TP] = { 0, 0, 0, 15, 13, 11, 10, 9, 5, 3, 3, 0 };
static s32 star_a3_s1_rtbl15nit[S6E3HA8_NR_TP] = { 0, 0, 0, 15, 13, 12, 10, 9, 5, 3, 3, 0 };
static s32 star_a3_s1_rtbl16nit[S6E3HA8_NR_TP] = { 0, 0, 0, 15, 13, 11, 10, 8, 5, 2, 3, 0 };
static s32 star_a3_s1_rtbl17nit[S6E3HA8_NR_TP] = { 0, 0, 0, 15, 12, 11, 9, 8, 5, 2, 3, 0 };
static s32 star_a3_s1_rtbl18nit[S6E3HA8_NR_TP] = { 0, 0, 0, 15, 12, 10, 9, 7, 5, 2, 2, 0 };
static s32 star_a3_s1_rtbl19nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 11, 9, 9, 7, 4, 2, 3, 0 };
static s32 star_a3_s1_rtbl20nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 11, 9, 8, 6, 4, 2, 2, 0 };
static s32 star_a3_s1_rtbl21nit[S6E3HA8_NR_TP] = { 0, 0, 0, 13, 10, 9, 8, 6, 4, 2, 3, 0 };
static s32 star_a3_s1_rtbl23nit[S6E3HA8_NR_TP] = { 0, 0, 0, 12, 9, 8, 7, 6, 3, 1, 2, 0 };
static s32 star_a3_s1_rtbl24nit[S6E3HA8_NR_TP] = { 0, 0, 0, 12, 9, 8, 7, 6, 3, 1, 2, 0 };
static s32 star_a3_s1_rtbl26nit[S6E3HA8_NR_TP] = { 0, 0, 0, 12, 8, 7, 6, 5, 2, 1, 2, 0 };
static s32 star_a3_s1_rtbl27nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 8, 7, 6, 5, 3, 1, 2, 0 };
static s32 star_a3_s1_rtbl29nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 7, 6, 6, 4, 2, 1, 2, 0 };
static s32 star_a3_s1_rtbl31nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 6, 6, 5, 4, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl33nit[S6E3HA8_NR_TP] = { 0, 0, 0, 7, 6, 5, 5, 4, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl35nit[S6E3HA8_NR_TP] = { 0, 0, 0, 7, 5, 5, 4, 4, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl37nit[S6E3HA8_NR_TP] = { 0, 0, 0, 7, 5, 5, 4, 4, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl39nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 4, 4, 4, 3, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl42nit[S6E3HA8_NR_TP] = { 0, 0, 0, 6, 4, 4, 3, 3, 0, 0, 2, 0 };
static s32 star_a3_s1_rtbl45nit[S6E3HA8_NR_TP] = { 0, 0, 0, 5, 3, 3, 3, 3, 0, 0, 1, 0 };
static s32 star_a3_s1_rtbl48nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 3, 3, 2, 2, 0, 0, 1, 0 };
static s32 star_a3_s1_rtbl51nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 3, 3, 2, 2, 0, -1, 1, 0 };
static s32 star_a3_s1_rtbl54nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 2, 2, 2, 2, -1, 0, 1, 0 };
static s32 star_a3_s1_rtbl57nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, 2, 2, 2, 0, -1, 1, 0 };
static s32 star_a3_s1_rtbl61nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, 2, 2, 2, 0, -1, 1, 0 };
static s32 star_a3_s1_rtbl65nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, 2, 2, 2, 0, 0, 2, 0 };
static s32 star_a3_s1_rtbl69nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, 2, 2, 2, 0, 0, 1, 0 };
static s32 star_a3_s1_rtbl73nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 2, 2, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl78nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 2, 1, 1, 1, 2, 0 };
static s32 star_a3_s1_rtbl83nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 3, 1, 1, 0, 2, 0 };
static s32 star_a3_s1_rtbl88nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 2, 1, 1, 0, 1, 0 };
static s32 star_a3_s1_rtbl94nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 3, 3, 2, 1, 2, 1, 1, 0 };
static s32 star_a3_s1_rtbl100nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 3, 2, 3, 1, 1, 2, 2, 0 };
static s32 star_a3_s1_rtbl106nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 3, 2, 2, 2, 2, 3, 1, 0 };
static s32 star_a3_s1_rtbl113nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 2, 2, 2, 2, 2, 3, 1, 0 };
static s32 star_a3_s1_rtbl120nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 2, 2, 3, 3, 2, 3, 1, 0 };
static s32 star_a3_s1_rtbl128nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 2, 3, 2, 2, 2, 4, 0, 0 };
static s32 star_a3_s1_rtbl136nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 2, 2, 2, 2, 1, 4, 2, 0 };
static s32 star_a3_s1_rtbl145nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 2, 2, 2, 2, 2, 4, 2, 0 };
static s32 star_a3_s1_rtbl154nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 1, 2, 2, 2, 2, 4, 2, 0 };
static s32 star_a3_s1_rtbl164nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 1, 2, 2, 2, 2, 4, 2, 0 };
static s32 star_a3_s1_rtbl174nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 1, 1, 2, 2, 3, 5, 3, 0 };
static s32 star_a3_s1_rtbl185nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 1, 1, 2, 1, 2, 4, 1, 0 };
static s32 star_a3_s1_rtbl197nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 4, 1, 0 };
static s32 star_a3_s1_rtbl210nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, 1, 0, 1, 1, 0, 2, 1, 0 };
static s32 star_a3_s1_rtbl223nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0 };
static s32 star_a3_s1_rtbl237nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0 };
static s32 star_a3_s1_rtbl253nit[S6E3HA8_NR_TP] = { 0, 0, 0, -1, -1, 0, 0, 0, 0, 1, 0, 0 };
static s32 star_a3_s1_rtbl269nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, -1, -1, 0, 0, 2, 0, 0 };
static s32 star_a3_s1_rtbl286nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, 0, 0, 0, 0, 2, 1, 0 };
static s32 star_a3_s1_rtbl301nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -1, -1, 0, 0, 0, 1, 1, 0 };
static s32 star_a3_s1_rtbl317nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -1, -1, -1, 0, -1, 1, 0, 0 };
static s32 star_a3_s1_rtbl333nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -1, -1, 0, -1, -1, 1, -1, 0 };
static s32 star_a3_s1_rtbl340nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, -1, -1, -1, -1, 1, -1, 0 };
static s32 star_a3_s1_rtbl347nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, -1, -1, -2, -1, 0, 0, 0 };
static s32 star_a3_s1_rtbl354nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, -1, -1, -2, -1, 0, 0, 0 };
static s32 star_a3_s1_rtbl362nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -2, -2, 0, -2, 0 };
static s32 star_a3_s1_rtbl369nit[S6E3HA8_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -2, -2, 0, -2, 0 };
static s32 star_a3_s1_rtbl376nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 0, -1, -1, -2, -2, -2, -2, 0 };
static s32 star_a3_s1_rtbl384nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 0, -1, -1, -2, -2, -1, -2, 0 };
static s32 star_a3_s1_rtbl392nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -1, -1, -1, -2, -2, -1, -1, 0 };
static s32 star_a3_s1_rtbl400nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* rgb color offset values */
static s32 star_a3_s1_ctbl2nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -128, -1, 0, -128, 0, 10, -128, 0, 10, -95, -1, 6, -28, 1, 6, -11, 0, 4, -3, 0, 1, 0, 0, 1, -6, 0, 0 };
static s32 star_a3_s1_ctbl3nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -128, -1, -4, -128, 0, 9, -93, 1, 3, -43, -1, 3, -23, -1, 7, -5, 1, 4, -3, 0, 0, -1, 0, 0, -3, 0, 1 };
static s32 star_a3_s1_ctbl4nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -128, -1, -4, -128, 0, 9, -45, 0, 3, -29, 0, 4, -14, -1, 7, -6, 1, 2, -2, 0, 0, -1, 0, 1, -3, 0, 0 };
static s32 star_a3_s1_ctbl5nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -96, -1, -4, -124, 0, 9, -52, -2, 2, -22, 0, 5, -12, 1, 7, -5, 0, 1, -1, 0, 0, -1, 0, 1, -3, 0, 0 };
static s32 star_a3_s1_ctbl6nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -8, -1, -4, -113, 0, 9, -28, 1, 5, -16, 2, 6, -10, -1, 5, -5, 0, 0, -1, 0, 0, -1, 0, 1, -2, 0, 0 };
static s32 star_a3_s1_ctbl7nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -4, -76, 0, 9, -21, 0, 5, -11, 0, 5, -8, 1, 6, -4, -1, -1, -1, 0, 0, -1, 0, 0, -2, 0, 0 };
static s32 star_a3_s1_ctbl8nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -4, -31, 0, 9, -18, -2, 3, -10, 0, 5, -7, 1, 5, -4, -1, -1, -1, 0, 0, -1, 0, 0, -1, 0, 1 };
static s32 star_a3_s1_ctbl9nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -5, -14, 2, 9, -15, 0, 5, -9, -1, 4, -5, 1, 5, -4, 0, -2, -1, 0, 0, 0, 0, 1, -2, 0, 0 };
static s32 star_a3_s1_ctbl10nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -4, -11, 0, 9, -11, 0, 5, -9, -2, 3, -5, 1, 2, -4, 0, -2, 0, 0, 1, -1, 0, 0, -1, 0, 1 };
static s32 star_a3_s1_ctbl11nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -4, -10, 0, 9, -6, 0, 5, -4, 2, 5, -5, 0, 2, -4, 0, -2, 0, 0, 0, -1, 0, 0, -1, 0, 1 };
static s32 star_a3_s1_ctbl12nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -2, -4, -15, 0, 9, -3, 0, 5, -2, 2, 5, -5, 0, 1, -3, 0, -2, -1, 0, 0, -1, 0, 0, 0, 0, 1 };
static s32 star_a3_s1_ctbl13nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, 0, -6, -15, 0, 7, -1, 1, 5, -5, -1, 1, -4, 0, 1, -2, 1, -2, -1, 0, 0, 0, 0, 1, 0, 0, 1 };
static s32 star_a3_s1_ctbl14nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, 1, -8, -13, -1, 6, -2, 1, 4, -1, 1, 3, -6, -1, -1, -2, 1, -2, -1, 0, 0, 0, 0, 1, 0, 0, 1 };
static s32 star_a3_s1_ctbl15nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -2, -9, -13, 3, 9, -2, -1, 1, -1, 2, 3, -6, -1, -1, -2, 1, -2, 0, 0, 0, -1, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl16nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -9, -13, 0, 8, -1, 0, 3, -3, 0, 1, -4, 0, 0, -2, 0, -2, 0, 0, 0, -1, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl17nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -8, -15, 3, 9, -4, -2, 1, -1, 2, 3, -5, 0, -1, -2, -1, -2, 0, 0, 0, -1, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl18nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -6, -15, 0, 7, -4, -1, 2, -3, -1, 2, -3, 1, 0, -3, -1, -3, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl19nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, 2, -5, -15, -2, 6, 4, 3, 6, -5, -1, 1, -3, 0, -1, -3, 0, -3, 1, 0, 1, -1, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl20nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -3, -5, -15, -2, 3, 2, 1, 4, -5, -1, 0, -2, 1, 1, -3, 0, -3, 0, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl21nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -3, -5, -15, 3, 9, 0, 0, 3, -6, -2, -1, -2, 1, 1, -3, 0, -3, 1, 0, 1, -1, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl23nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -3, -5, -20, 1, 7, 2, 0, 4, -2, 2, 2, -5, -2, -2, -2, 0, -1, 1, 1, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl24nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -3, -5, -20, -1, 6, 1, -1, 4, -2, 2, 2, -5, -1, -2, -3, -1, -2, 1, 1, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl26nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -2, -4, -25, -3, 7, 4, 0, 5, -2, 1, 1, -3, 0, -1, -2, -1, -2, 1, 1, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl27nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -3, -25, 0, 7, 2, -1, 4, -2, 1, 1, -3, 0, 0, -3, -1, -2, 1, 1, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl29nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -3, -25, -2, 6, 8, 3, 6, -6, -2, -1, 0, 1, 1, -2, 0, -2, 1, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl31nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, 0, -4, -25, 2, 6, 7, 0, 5, -4, 0, 0, -3, -1, -2, 0, 1, 1, 0, 0, -1, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl33nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, 2, -3, -25, -3, 6, 10, 2, 7, -3, -1, 0, -3, -1, -2, -1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl35nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -15, -1, -4, -25, 0, 7, 7, -1, 4, 0, 3, 3, -4, -1, -2, -1, 0, -1, 1, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl37nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -1, -3, -25, -3, 6, 7, -1, 4, -1, 2, 2, -4, -1, -2, -1, 0, -1, 1, 0, 0, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl39nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -1, -4, -20, -1, 6, 15, 3, 8, -5, -1, -1, -2, 1, 1, -1, 0, -1, 1, 0, 0, -1, 0, -1, 2, 0, 3 };
static s32 star_a3_s1_ctbl42nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -1, -4, -19, -4, 8, 11, 0, 4, -2, 1, 1, -4, -1, -2, 1, 1, 0, 1, 0, 0, -1, 0, -1, 2, 0, 3 };
static s32 star_a3_s1_ctbl45nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -1, -4, -13, -2, 9, 16, 2, 7, -2, 1, 0, -4, -2, -2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_ctbl48nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -5, -3, -11, -2, 11, 10, -2, 3, 2, 1, 1, -2, 1, 0, 0, 0, -1, 1, 0, 1, -1, 0, -1, 2, 0, 3 };
static s32 star_a3_s1_ctbl51nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -5, -3, -11, -2, 11, 10, -3, 3, 1, 1, 1, -1, 1, 0, -2, -1, -3, 2, 1, 2, -1, 0, -1, 2, 0, 3 };
static s32 star_a3_s1_ctbl54nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -25, -5, -3, -7, -2, 10, 16, 2, 6, 1, 1, 1, -4, -2, -3, 1, 1, 0, 1, 0, 1, -1, 0, -1, 2, 0, 3 };
static s32 star_a3_s1_ctbl57nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -25, -8, -3, 0, 1, 12, 17, 1, 6, 0, 1, 0, -2, -1, -1, -1, 0, -2, 2, 0, 1, 0, 0, 0, 1, 0, 2 };
static s32 star_a3_s1_ctbl61nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, -5, 0, 0, 1, 10, 14, -1, 5, -3, 0, -1, -3, -1, -1, 0, 0, -1, 1, 0, 0, 1, 1, 1, 1, 0, 2 };
static s32 star_a3_s1_ctbl65nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -18, -5, 0, 0, 1, 12, 14, -1, 4, -3, 0, -1, -1, 0, 0, 1, 1, 0, 1, 0, 0, 0, -1, 0, 2, 1, 2 };
static s32 star_a3_s1_ctbl69nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -16, -5, 0, 0, 1, 12, 17, 3, 7, -3, -1, -2, -1, 1, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl73nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -14, 1, 3, 5, 3, 12, 10, -2, 2, 1, 2, 2, -2, 0, -1, 0, 0, -1, 0, 0, 0, 1, 1, 1, 1, 0, 2 };
static s32 star_a3_s1_ctbl78nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -12, -5, 3, 3, 7, 10, 15, 1, 6, -3, -1, -2, 1, 2, 2, -1, -1, -2, 2, 0, 2, 0, 0, 0, 1, 1, 2 };
static s32 star_a3_s1_ctbl83nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -10, -5, 3, 5, 5, 12, 9, 0, 3, -4, -2, -2, 0, 1, 1, 0, 0, -1, 1, 0, 0, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl88nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -10, -5, 1, 5, 7, 11, 10, 0, 3, 0, 0, 0, -1, 1, 0, 0, -1, -1, 1, 1, 1, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_ctbl94nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, 0, -4, -5, 6, 6, -2, 1, -2, -1, -1, 0, 1, 0, 0, -1, -1, 1, 0, 1, 1, 0, 1, 1, 0, 2 };
static s32 star_a3_s1_ctbl100nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -5, -6, 5, 9, 1, 4, -3, -1, -1, -1, 0, 0, 1, -1, 0, 0, 1, 0, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_ctbl106nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, -4, -6, 5, 9, 1, 4, -1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_ctbl113nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 7, 0, 2, -3, 7, 5, 0, 2, -2, -1, -1, 0, 1, 0, 1, 0, 0, -2, -1, -1, 0, 0, 0, 3, 1, 4 };
static s32 star_a3_s1_ctbl120nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 4, -2, 2, -3, 7, 7, 2, 3, 0, 0, 0, -3, -2, -2, 1, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 3 };
static s32 star_a3_s1_ctbl128nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 0, 5, 0, 9, 2, -2, 1, -1, 0, -1, 1, 1, 1, 0, 0, -1, -2, -1, -1, 1, 1, 0, 3, 0, 3 };
static s32 star_a3_s1_ctbl136nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 6, 0, 9, 4, 0, 2, -2, -1, -1, -1, 0, -1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl145nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, -1, 0, 1, -3, 7, 5, 1, 2, -1, 0, 0, -1, 0, 0, 1, 1, 0, 0, 0, 0, -1, 0, -1, 3, 1, 4 };
static s32 star_a3_s1_ctbl154nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, -2, -1, 9, 2, 11, 0, -2, 0, -1, -1, 0, 0, 0, 0, 1, 0, 0, -1, 0, -1, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl164nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, -2, -1, 7, 1, 10, 0, -2, 0, 0, 2, 1, -1, -1, -1, 1, 1, 0, 0, 0, 0, -1, 0, -1, 2, 1, 3 };
static s32 star_a3_s1_ctbl174nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, -3, -1, 6, 0, 9, 2, 1, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl185nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, -3, -2, 7, -2, 9, 2, 1, 2, -1, -1, 0, 0, 1, 0, 1, 0, 0, -2, -1, -1, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl197nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 10, 7, 5, -3, 8, -1, -3, -1, 1, 1, 1, -1, 0, -1, 1, 1, 0, -1, -1, 0, -1, 0, -1, 2, 1, 3 };
static s32 star_a3_s1_ctbl210nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 6, -2, -8, 3, 6, 2, 3, 0, 1, 0, -2, -2, -2, 2, 0, 1, -1, 0, -1, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl223nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -10, -5, 5, 5, 1, 3, 5, 1, 2, -1, -2, -1, 0, -1, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl237nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -5, -5, 5, 5, 2, 3, 2, -2, -1, 2, 1, 2, -1, -1, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 3 };
static s32 star_a3_s1_ctbl253nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -20, 3, 0, 4, 2, 3, 5, 1, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, -1, 2, 1, 3 };
static s32 star_a3_s1_ctbl269nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 12, 2, 0, 3, 6, 1, 2, 2, 2, 2, -1, -1, -1, 1, 0, 1, 0, 0, -1, 0, 0, 0, 2, 1, 2 };
static s32 star_a3_s1_ctbl286nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 3, 12, 0, 2, 3, 1, -2, 0, 2, 1, 1, -1, 0, -1, 1, 0, 1, 0, 0, -1, 0, 0, 1, 1, 1, 1 };
static s32 star_a3_s1_ctbl301nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 6, 12, 3, -4, 2, 4, 1, 2, 1, 0, 1, -1, -1, -1, 0, 0, 0, 1, 1, 0, -1, -1, 0, 2, 1, 2 };
static s32 star_a3_s1_ctbl317nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 5, 12, 2, -5, 1, 4, 1, 2, 2, 1, 2, -2, -1, -1, 1, 0, 1, -1, 0, -1, 1, 0, 1, 1, 1, 1 };
static s32 star_a3_s1_ctbl333nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 4, 12, 2, -5, 1, 5, 3, 4, -1, -2, -1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0 };
static s32 star_a3_s1_ctbl340nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 19, 4, 12, 6, -4, 3, 0, 0, 1, 1, 0, 1, -2, -1, -2, 1, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0 };
static s32 star_a3_s1_ctbl347nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 3, 12, 5, -3, 3, -1, 0, 0, -1, -2, -1, 0, 1, 0, 1, 0, 1, 0, 0, -1, 0, 0, 1, 0, 0, 0 };
static s32 star_a3_s1_ctbl354nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 3, 12, 6, -3, 3, -1, -1, 0, -1, -2, -1, 1, 1, 1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0 };
static s32 star_a3_s1_ctbl362nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 3, 12, -1, -5, -2, 3, 0, 1, 2, 1, 2, 0, 0, 0, 1, 1, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0 };
static s32 star_a3_s1_ctbl369nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 13, -1, -5, -3, 4, 0, 1, 2, 1, 2, 0, 0, 0, 1, 1, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0 };
static s32 star_a3_s1_ctbl376nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 2, -1, -4, -1, -2, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0 };
static s32 star_a3_s1_ctbl384nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -2, -3, -2, -1, -1, 0, 0, 0, -2, -2, -2, 1, 1, 1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 1, 0 };
static s32 star_a3_s1_ctbl392nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 1, 3, 3, 2, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, -1, -1, 0, 1, 0, 0, -1, 0, 0, 1, 0 };
static s32 star_a3_s1_ctbl400nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct dimming_lut star_a3_s1_dimming_lut[] = {
	DIM_LUT_V0_INIT(2, 105, GAMMA_2_15, star_a3_s1_rtbl2nit, star_a3_s1_ctbl2nit),
	DIM_LUT_V0_INIT(3, 105, GAMMA_2_15, star_a3_s1_rtbl3nit, star_a3_s1_ctbl3nit),
	DIM_LUT_V0_INIT(4, 105, GAMMA_2_15, star_a3_s1_rtbl4nit, star_a3_s1_ctbl4nit),
	DIM_LUT_V0_INIT(5, 105, GAMMA_2_15, star_a3_s1_rtbl5nit, star_a3_s1_ctbl5nit),
	DIM_LUT_V0_INIT(6, 105, GAMMA_2_15, star_a3_s1_rtbl6nit, star_a3_s1_ctbl6nit),
	DIM_LUT_V0_INIT(7, 105, GAMMA_2_15, star_a3_s1_rtbl7nit, star_a3_s1_ctbl7nit),
	DIM_LUT_V0_INIT(8, 105, GAMMA_2_15, star_a3_s1_rtbl8nit, star_a3_s1_ctbl8nit),
	DIM_LUT_V0_INIT(9, 105, GAMMA_2_15, star_a3_s1_rtbl9nit, star_a3_s1_ctbl9nit),
	DIM_LUT_V0_INIT(10, 105, GAMMA_2_15, star_a3_s1_rtbl10nit, star_a3_s1_ctbl10nit),
	DIM_LUT_V0_INIT(11, 105, GAMMA_2_15, star_a3_s1_rtbl11nit, star_a3_s1_ctbl11nit),
	DIM_LUT_V0_INIT(12, 105, GAMMA_2_15, star_a3_s1_rtbl12nit, star_a3_s1_ctbl12nit),
	DIM_LUT_V0_INIT(13, 105, GAMMA_2_15, star_a3_s1_rtbl13nit, star_a3_s1_ctbl13nit),
	DIM_LUT_V0_INIT(14, 105, GAMMA_2_15, star_a3_s1_rtbl14nit, star_a3_s1_ctbl14nit),
	DIM_LUT_V0_INIT(15, 105, GAMMA_2_15, star_a3_s1_rtbl15nit, star_a3_s1_ctbl15nit),
	DIM_LUT_V0_INIT(16, 105, GAMMA_2_15, star_a3_s1_rtbl16nit, star_a3_s1_ctbl16nit),
	DIM_LUT_V0_INIT(17, 105, GAMMA_2_15, star_a3_s1_rtbl17nit, star_a3_s1_ctbl17nit),
	DIM_LUT_V0_INIT(18, 105, GAMMA_2_15, star_a3_s1_rtbl18nit, star_a3_s1_ctbl18nit),
	DIM_LUT_V0_INIT(19, 105, GAMMA_2_15, star_a3_s1_rtbl19nit, star_a3_s1_ctbl19nit),
	DIM_LUT_V0_INIT(20, 105, GAMMA_2_15, star_a3_s1_rtbl20nit, star_a3_s1_ctbl20nit),
	DIM_LUT_V0_INIT(21, 105, GAMMA_2_15, star_a3_s1_rtbl21nit, star_a3_s1_ctbl21nit),
	DIM_LUT_V0_INIT(23, 105, GAMMA_2_15, star_a3_s1_rtbl23nit, star_a3_s1_ctbl23nit),
	DIM_LUT_V0_INIT(24, 105, GAMMA_2_15, star_a3_s1_rtbl24nit, star_a3_s1_ctbl24nit),
	DIM_LUT_V0_INIT(26, 105, GAMMA_2_15, star_a3_s1_rtbl26nit, star_a3_s1_ctbl26nit),
	DIM_LUT_V0_INIT(27, 105, GAMMA_2_15, star_a3_s1_rtbl27nit, star_a3_s1_ctbl27nit),
	DIM_LUT_V0_INIT(29, 105, GAMMA_2_15, star_a3_s1_rtbl29nit, star_a3_s1_ctbl29nit),
	DIM_LUT_V0_INIT(31, 105, GAMMA_2_15, star_a3_s1_rtbl31nit, star_a3_s1_ctbl31nit),
	DIM_LUT_V0_INIT(33, 105, GAMMA_2_15, star_a3_s1_rtbl33nit, star_a3_s1_ctbl33nit),
	DIM_LUT_V0_INIT(35, 105, GAMMA_2_15, star_a3_s1_rtbl35nit, star_a3_s1_ctbl35nit),
	DIM_LUT_V0_INIT(37, 105, GAMMA_2_15, star_a3_s1_rtbl37nit, star_a3_s1_ctbl37nit),
	DIM_LUT_V0_INIT(39, 105, GAMMA_2_15, star_a3_s1_rtbl39nit, star_a3_s1_ctbl39nit),
	DIM_LUT_V0_INIT(42, 105, GAMMA_2_15, star_a3_s1_rtbl42nit, star_a3_s1_ctbl42nit),
	DIM_LUT_V0_INIT(45, 105, GAMMA_2_15, star_a3_s1_rtbl45nit, star_a3_s1_ctbl45nit),
	DIM_LUT_V0_INIT(48, 105, GAMMA_2_15, star_a3_s1_rtbl48nit, star_a3_s1_ctbl48nit),
	DIM_LUT_V0_INIT(51, 105, GAMMA_2_15, star_a3_s1_rtbl51nit, star_a3_s1_ctbl51nit),
	DIM_LUT_V0_INIT(54, 105, GAMMA_2_15, star_a3_s1_rtbl54nit, star_a3_s1_ctbl54nit),
	DIM_LUT_V0_INIT(57, 107, GAMMA_2_15, star_a3_s1_rtbl57nit, star_a3_s1_ctbl57nit),
	DIM_LUT_V0_INIT(61, 114, GAMMA_2_15, star_a3_s1_rtbl61nit, star_a3_s1_ctbl61nit),
	DIM_LUT_V0_INIT(65, 119, GAMMA_2_15, star_a3_s1_rtbl65nit, star_a3_s1_ctbl65nit),
	DIM_LUT_V0_INIT(69, 126, GAMMA_2_15, star_a3_s1_rtbl69nit, star_a3_s1_ctbl69nit),
	DIM_LUT_V0_INIT(73, 132, GAMMA_2_15, star_a3_s1_rtbl73nit, star_a3_s1_ctbl73nit),
	DIM_LUT_V0_INIT(78, 141, GAMMA_2_15, star_a3_s1_rtbl78nit, star_a3_s1_ctbl78nit),
	DIM_LUT_V0_INIT(83, 149, GAMMA_2_15, star_a3_s1_rtbl83nit, star_a3_s1_ctbl83nit),
	DIM_LUT_V0_INIT(88, 160, GAMMA_2_15, star_a3_s1_rtbl88nit, star_a3_s1_ctbl88nit),
	DIM_LUT_V0_INIT(94, 168, GAMMA_2_15, star_a3_s1_rtbl94nit, star_a3_s1_ctbl94nit),
	DIM_LUT_V0_INIT(100, 177, GAMMA_2_15, star_a3_s1_rtbl100nit, star_a3_s1_ctbl100nit),
	DIM_LUT_V0_INIT(106, 186, GAMMA_2_15, star_a3_s1_rtbl106nit, star_a3_s1_ctbl106nit),
	DIM_LUT_V0_INIT(113, 197, GAMMA_2_15, star_a3_s1_rtbl113nit, star_a3_s1_ctbl113nit),
	DIM_LUT_V0_INIT(120, 209, GAMMA_2_15, star_a3_s1_rtbl120nit, star_a3_s1_ctbl120nit),
	DIM_LUT_V0_INIT(128, 219, GAMMA_2_15, star_a3_s1_rtbl128nit, star_a3_s1_ctbl128nit),
	DIM_LUT_V0_INIT(136, 232, GAMMA_2_15, star_a3_s1_rtbl136nit, star_a3_s1_ctbl136nit),
	DIM_LUT_V0_INIT(145, 242, GAMMA_2_15, star_a3_s1_rtbl145nit, star_a3_s1_ctbl145nit),
	DIM_LUT_V0_INIT(154, 256, GAMMA_2_15, star_a3_s1_rtbl154nit, star_a3_s1_ctbl154nit),
	DIM_LUT_V0_INIT(164, 266, GAMMA_2_15, star_a3_s1_rtbl164nit, star_a3_s1_ctbl164nit),
	DIM_LUT_V0_INIT(174, 280, GAMMA_2_15, star_a3_s1_rtbl174nit, star_a3_s1_ctbl174nit),
	DIM_LUT_V0_INIT(185, 280, GAMMA_2_15, star_a3_s1_rtbl185nit, star_a3_s1_ctbl185nit),
	DIM_LUT_V0_INIT(197, 280, GAMMA_2_15, star_a3_s1_rtbl197nit, star_a3_s1_ctbl197nit),
	DIM_LUT_V0_INIT(210, 280, GAMMA_2_15, star_a3_s1_rtbl210nit, star_a3_s1_ctbl210nit),
	DIM_LUT_V0_INIT(223, 280, GAMMA_2_15, star_a3_s1_rtbl223nit, star_a3_s1_ctbl223nit),
	DIM_LUT_V0_INIT(237, 283, GAMMA_2_15, star_a3_s1_rtbl237nit, star_a3_s1_ctbl237nit),
	DIM_LUT_V0_INIT(253, 298, GAMMA_2_15, star_a3_s1_rtbl253nit, star_a3_s1_ctbl253nit),
	DIM_LUT_V0_INIT(269, 310, GAMMA_2_15, star_a3_s1_rtbl269nit, star_a3_s1_ctbl269nit),
	DIM_LUT_V0_INIT(286, 325, GAMMA_2_15, star_a3_s1_rtbl286nit, star_a3_s1_ctbl286nit),
	DIM_LUT_V0_INIT(301, 341, GAMMA_2_15, star_a3_s1_rtbl301nit, star_a3_s1_ctbl301nit),
	DIM_LUT_V0_INIT(317, 357, GAMMA_2_15, star_a3_s1_rtbl317nit, star_a3_s1_ctbl317nit),
	DIM_LUT_V0_INIT(333, 370, GAMMA_2_15, star_a3_s1_rtbl333nit, star_a3_s1_ctbl333nit),
	DIM_LUT_V0_INIT(340, 380, GAMMA_2_15, star_a3_s1_rtbl340nit, star_a3_s1_ctbl340nit),
	DIM_LUT_V0_INIT(347, 383, GAMMA_2_15, star_a3_s1_rtbl347nit, star_a3_s1_ctbl347nit),
	DIM_LUT_V0_INIT(354, 383, GAMMA_2_15, star_a3_s1_rtbl354nit, star_a3_s1_ctbl354nit),
	DIM_LUT_V0_INIT(362, 383, GAMMA_2_15, star_a3_s1_rtbl362nit, star_a3_s1_ctbl362nit),
	DIM_LUT_V0_INIT(369, 383, GAMMA_2_15, star_a3_s1_rtbl369nit, star_a3_s1_ctbl369nit),
	DIM_LUT_V0_INIT(376, 383, GAMMA_2_15, star_a3_s1_rtbl376nit, star_a3_s1_ctbl376nit),
	DIM_LUT_V0_INIT(384, 386, GAMMA_2_15, star_a3_s1_rtbl384nit, star_a3_s1_ctbl384nit),
	DIM_LUT_V0_INIT(392, 390, GAMMA_2_15, star_a3_s1_rtbl392nit, star_a3_s1_ctbl392nit),
	DIM_LUT_V0_INIT(400, 400, GAMMA_2_20, star_a3_s1_rtbl400nit, star_a3_s1_ctbl400nit),
};

#if PANEL_BACKLIGHT_PAC_STEPS == 512
static unsigned int star_a3_s1_brt_to_step_tbl[S6E3HA8_STAR_TOTAL_PAC_STEPS] = {
	0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500,
	1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100,
	3200, 3300, 3400, 3500, 3514, 3528, 3542, 3556, 3570, 3584, 3600, 3629, 3658, 3687, 3716, 3745,
	3774, 3800, 3829, 3858, 3887, 3916, 3945, 3974, 4000, 4033, 4066, 4099, 4132, 4165, 4200, 4233,
	4266, 4299, 4332, 4365, 4400, 4433, 4466, 4499, 4532, 4565, 4600, 4633, 4666, 4699, 4732, 4765,
	4800, 4833, 4866, 4899, 4932, 4965, 5000, 5033, 5066, 5099, 5132, 5165, 5200, 5233, 5266, 5299,
	5332, 5365, 5400, 5433, 5466, 5499, 5532, 5565, 5600, 5617, 5634, 5651, 5668, 5685, 5700, 5733,
	5766, 5799, 5832, 5865, 5900, 5933, 5966, 5999, 6032, 6065, 6100, 6129, 6158, 6187, 6216, 6245,
	6274, 6300, 6333, 6366, 6399, 6432, 6465, 6500, 6529, 6558, 6587, 6616, 6645, 6674, 6700, 6733,
	6766, 6799, 6832, 6865, 6900, 6914, 6928, 6942, 6956, 6970, 6984, 7000, 7029, 7058, 7087, 7116,
	7145, 7174, 7200, 7229, 7258, 7287, 7316, 7345, 7374, 7400, 7433, 7466, 7499, 7532, 7565, 7600,
	7633, 7666, 7699, 7732, 7765, 7800, 7833, 7866, 7899, 7932, 7965, 8000, 8029, 8058, 8087, 8116,
	8145, 8174, 8200, 8229, 8258, 8287, 8316, 8345, 8374, 8400, 8429, 8458, 8487, 8516, 8545, 8574,
	8600, 8629, 8658, 8687, 8716, 8745, 8774, 8800, 8829, 8858, 8887, 8916, 8945, 8974, 9000, 9029,
	9058, 9087, 9116, 9145, 9174, 9200, 9229, 9258, 9287, 9316, 9345, 9374, 9400, 9429, 9458, 9487,
	9516, 9545, 9574, 9600, 9629, 9658, 9687, 9716, 9745, 9774, 9800, 9825, 9850, 9875, 9900, 9925,
	9950, 9975, 10000, 10025, 10050, 10075, 10100, 10125, 10150, 10175, 10200, 10225, 10250, 10275, 10300, 10325,
	10350, 10375, 10400, 10425, 10450, 10475, 10500, 10525, 10550, 10575, 10600, 10625, 10650, 10675, 10700, 10725,
	10750, 10775, 10800, 10822, 10844, 10866, 10888, 10910, 10932, 10954, 10976, 11000, 11022, 11044, 11066, 11088,
	11110, 11132, 11154, 11176, 11200, 11222, 11244, 11266, 11288, 11310, 11332, 11354, 11376, 11400, 11425, 11450,
	11475, 11500, 11525, 11550, 11575, 11600, 11622, 11644, 11666, 11688, 11710, 11732, 11754, 11776, 11800, 11820,
	11840, 11860, 11880, 11900, 11920, 11940, 11960, 11980, 12000, 12022, 12044, 12066, 12088, 12110, 12132, 12154,
	12176, 12200, 12220, 12240, 12260, 12280, 12300, 12320, 12340, 12360, 12380, 12400, 12420, 12440, 12460, 12480,
	12500, 12520, 12540, 12560, 12580, 12600, 12618, 12636, 12654, 12672, 12690, 12708, 12726, 12744, 12762, 12780,
	12800, 12900, 13000, 13100, 13200, 13300, 13400, 13500, 13600, 13700, 13800, 13900, 14000, 14100, 14200, 14300,
	14400, 14500, 14600, 14700, 14800, 14900, 15000, 15100, 15200, 15300, 15400, 15500, 15600, 15700, 15800, 15900,
	16000, 16100, 16200, 16300, 16400, 16500, 16600, 16700, 16800, 16900, 17000, 17100, 17200, 17300, 17400, 17500,
	17600, 17700, 17800, 17900, 18000, 18100, 18200, 18300, 18400, 18500, 18600, 18700, 18800, 18900, 19000, 19100,
	19200, 19300, 19400, 19500, 19600, 19700, 19800, 19900, 20000, 20100, 20200, 20300, 20400, 20500, 20600, 20700,
	20800, 20900, 21000, 21100, 21200, 21300, 21400, 21500, 21600, 21700, 21800, 21900, 22000, 22100, 22200, 22300,
	22400, 22500, 22600, 22700, 22800, 22900, 23000, 23100, 23200, 23300, 23400, 23500, 23600, 23700, 23800, 23900,
	24000, 24100, 24200, 24300, 24400, 24500, 24600, 24700, 24800, 24900, 25000, 25100, 25200, 25300, 25400, 25500,
	/* HBM */
#ifndef CONFIG_LCD_HBM_60_STEP
	27100, 28700, 30300, 31900, 33500, 35100, 36700, 38300, 39900, 41500, 43100, 44700,
#else
	25800, 26100, 26500, 26800, 27100, 27400, 27700, 28100, 28400, 28700, 29000, 29300, 29700, 30000, 30300, 30600,
	30900, 31300, 31600, 31900, 32200, 32500, 32900, 33200, 33500, 33800, 34100, 34500, 34800, 35100, 35400, 35700,
	36100, 36400, 36700, 37000, 37300, 37700, 38000, 38300, 38600, 38900, 39300, 39600, 39900, 40200, 40500, 40900,
	41200, 41500, 41800, 42100, 42500, 42800, 43100, 43400, 43700, 44100, 44400, 44700,
#endif
};
#elif PANEL_BACKLIGHT_PAC_STEPS == 256
static unsigned int star_a3_s1_brt_to_step_tbl[S6E3HA8_STAR_TOTAL_PAC_STEPS] = {
	BRT(0), BRT(1), BRT(2), BRT(3), BRT(4), BRT(5), BRT(6), BRT(7), BRT(8), BRT(9), BRT(10), BRT(11), BRT(12), BRT(13), BRT(14), BRT(15),
	BRT(16), BRT(17), BRT(18), BRT(19), BRT(20), BRT(21), BRT(22), BRT(23), BRT(24), BRT(25), BRT(26), BRT(27), BRT(28), BRT(29), BRT(30), BRT(31),
	BRT(32), BRT(33), BRT(34), BRT(35), BRT(36), BRT(37), BRT(38), BRT(39), BRT(40), BRT(41), BRT(42), BRT(43), BRT(44), BRT(45), BRT(46), BRT(47),
	BRT(48), BRT(49), BRT(50), BRT(51), BRT(52), BRT(53), BRT(54), BRT(55), BRT(56), BRT(57), BRT(58), BRT(59), BRT(60), BRT(61), BRT(62), BRT(63),
	BRT(64), BRT(65), BRT(66), BRT(67), BRT(68), BRT(69), BRT(70), BRT(71), BRT(72), BRT(73), BRT(74), BRT(75), BRT(76), BRT(77), BRT(78), BRT(79),
	BRT(80), BRT(81), BRT(82), BRT(83), BRT(84), BRT(85), BRT(86), BRT(87), BRT(88), BRT(89), BRT(90), BRT(91), BRT(92), BRT(93), BRT(94), BRT(95),
	BRT(96), BRT(97), BRT(98), BRT(99), BRT(100), BRT(101), BRT(102), BRT(103), BRT(104), BRT(105), BRT(106), BRT(107), BRT(108), BRT(109), BRT(110), BRT(111),
	BRT(112), BRT(113), BRT(114), BRT(115), BRT(116), BRT(117), BRT(118), BRT(119), BRT(120), BRT(121), BRT(122), BRT(123), BRT(124), BRT(125), BRT(126), BRT(127),
	BRT(128), BRT(129), BRT(130), BRT(131), BRT(132), BRT(133), BRT(134), BRT(135), BRT(136), BRT(137), BRT(138), BRT(139), BRT(140), BRT(141), BRT(142), BRT(143),
	BRT(144), BRT(145), BRT(146), BRT(147), BRT(148), BRT(149), BRT(150), BRT(151), BRT(152), BRT(153), BRT(154), BRT(155), BRT(156), BRT(157), BRT(158), BRT(159),
	BRT(160), BRT(161), BRT(162), BRT(163), BRT(164), BRT(165), BRT(166), BRT(167), BRT(168), BRT(169), BRT(170), BRT(171), BRT(172), BRT(173), BRT(174), BRT(175),
	BRT(176), BRT(177), BRT(178), BRT(179), BRT(180), BRT(181), BRT(182), BRT(183), BRT(184), BRT(185), BRT(186), BRT(187), BRT(188), BRT(189), BRT(190), BRT(191),
	BRT(192), BRT(193), BRT(194), BRT(195), BRT(196), BRT(197), BRT(198), BRT(199), BRT(200), BRT(201), BRT(202), BRT(203), BRT(204), BRT(205), BRT(206), BRT(207),
	BRT(208), BRT(209), BRT(210), BRT(211), BRT(212), BRT(213), BRT(214), BRT(215), BRT(216), BRT(217), BRT(218), BRT(219), BRT(220), BRT(221), BRT(222), BRT(223),
	BRT(224), BRT(225), BRT(226), BRT(227), BRT(228), BRT(229), BRT(230), BRT(231), BRT(232), BRT(233), BRT(234), BRT(235), BRT(236), BRT(237), BRT(238), BRT(239),
	BRT(240), BRT(241), BRT(242), BRT(243), BRT(244), BRT(245), BRT(246), BRT(247), BRT(248), BRT(249), BRT(250), BRT(251), BRT(252), BRT(253), BRT(254), BRT(255),
	/* HBM */
#ifndef CONFIG_LCD_HBM_60_STEP
	BRT(271), BRT(287), BRT(303), BRT(319), BRT(335), BRT(351), BRT(367), BRT(383), BRT(399), BRT(415), BRT(431), BRT(447),
#else
	BRT(258), BRT(261), BRT(265), BRT(268), BRT(271), BRT(274), BRT(277), BRT(281), BRT(284), BRT(287), BRT(290), BRT(293), BRT(297), BRT(300), BRT(303), BRT(306),
	BRT(309), BRT(313), BRT(316), BRT(319), BRT(322), BRT(325), BRT(329), BRT(332), BRT(335), BRT(338), BRT(341), BRT(345), BRT(348), BRT(351), BRT(354), BRT(357),
	BRT(361), BRT(364), BRT(367), BRT(370), BRT(373), BRT(377), BRT(380), BRT(383), BRT(386), BRT(389), BRT(393), BRT(396), BRT(399), BRT(402), BRT(405), BRT(409),
	BRT(412), BRT(415), BRT(418), BRT(421), BRT(425), BRT(428), BRT(431), BRT(434), BRT(437), BRT(441), BRT(444), BRT(447),
#endif
};
#endif	/* PANEL_BACKLIGHT_PAC_STEPS  */

static unsigned int star_a3_s1_brt_to_brt_tbl[S6E3HA8_STAR_TOTAL_PAC_STEPS][2] = {
};

static unsigned int star_a3_s1_brt_tbl[S6E3HA8_STAR_TOTAL_NR_LUMINANCE] = {
	BRT(0), BRT(7), BRT(14), BRT(21), BRT(28), BRT(35), BRT(36), BRT(38), BRT(40), BRT(42),
	BRT(44), BRT(46), BRT(48), BRT(50), BRT(52), BRT(54), BRT(56), BRT(57), BRT(59), BRT(61),
	BRT(63), BRT(65), BRT(67), BRT(69), BRT(70), BRT(72), BRT(74), BRT(76), BRT(78), BRT(80),
	BRT(82), BRT(84), BRT(86), BRT(88), BRT(90), BRT(92), BRT(94), BRT(96), BRT(98), BRT(100),
	BRT(102), BRT(104), BRT(106), BRT(108), BRT(110), BRT(112), BRT(114), BRT(116), BRT(118), BRT(120),
	BRT(122), BRT(124), BRT(126), BRT(128), BRT(135), BRT(142), BRT(149), BRT(157), BRT(165), BRT(174),
	BRT(183), BRT(193), BRT(201), BRT(210), BRT(219), BRT(223), BRT(227), BRT(230), BRT(234), BRT(238),
	BRT(242), BRT(246), BRT(250), BRT(255),
	/* HBM */
#ifndef CONFIG_LCD_HBM_60_STEP
	BRT(271), BRT(287), BRT(303), BRT(319), BRT(335), BRT(351), BRT(367), BRT(383), BRT(399), BRT(415),
	BRT(431), BRT(447),
#else
	BRT(258), BRT(261), BRT(265), BRT(268), BRT(271), BRT(274), BRT(277), BRT(281), BRT(284), BRT(287),
	BRT(290), BRT(293), BRT(297), BRT(300), BRT(303), BRT(306), BRT(309), BRT(313), BRT(316), BRT(319),
	BRT(322), BRT(325), BRT(329), BRT(332), BRT(335), BRT(338), BRT(341), BRT(345), BRT(348), BRT(351),
	BRT(354), BRT(357), BRT(361), BRT(364), BRT(367), BRT(370), BRT(373), BRT(377), BRT(380), BRT(383),
	BRT(386), BRT(389), BRT(393), BRT(396), BRT(399), BRT(402), BRT(405), BRT(409), BRT(412), BRT(415),
	BRT(418), BRT(421), BRT(425), BRT(428), BRT(431), BRT(434), BRT(437), BRT(441), BRT(444), BRT(447),
#endif
};

static unsigned int star_a3_s1_lum_tbl[S6E3HA8_STAR_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	23, 24, 26, 27, 29, 31, 33, 35, 37, 39,
	42, 45, 48, 51, 54, 57, 61, 65, 69, 73,
	78, 83, 88, 94, 100, 106, 113, 120, 128, 136,
	145, 154, 164, 174, 185, 197, 210, 223, 237, 253,
	269, 286, 301, 317, 333, 340, 347, 354, 362, 369,
	376, 384, 392, 400,
	/* HBM */
#ifndef CONFIG_LCD_HBM_60_STEP
	425, 450, 475, 500, 525, 550, 575, 600, 625, 650,
	675, 700,
#else
	405, 410, 415, 420, 425, 430, 435, 440, 445, 450,
	455, 460, 465, 470, 475, 480, 485, 490, 495, 500,
	505, 510, 515, 520, 525, 530, 535, 540, 545, 550,
	555, 560, 565, 570, 575, 580, 585, 590, 595, 600,
	605, 610, 615, 620, 625, 630, 635, 640, 645, 650,
	655, 660, 665, 670, 675, 680, 685, 690, 695, 700,
#endif
};

struct brightness_table s6e3ha8_star_a3_s1_panel_brightness_table = {
	.brt = star_a3_s1_brt_tbl,
	.sz_brt = ARRAY_SIZE(star_a3_s1_brt_tbl),
	.lum = star_a3_s1_lum_tbl,
	.sz_lum = ARRAY_SIZE(star_a3_s1_lum_tbl),
	.sz_ui_lum = S6E3HA8_NR_LUMINANCE,
	.sz_hbm_lum = S6E3HA8_STAR_NR_HBM_LUMINANCE,
	.sz_ext_hbm_lum = 0,
#if PANEL_BACKLIGHT_PAC_STEPS == 512 || PANEL_BACKLIGHT_PAC_STEPS == 256
	.brt_to_step = star_a3_s1_brt_to_step_tbl,
	.sz_brt_to_step = ARRAY_SIZE(star_a3_s1_brt_to_step_tbl),
#else
	.brt_to_step = star_a3_s1_brt_tbl,
	.sz_brt_to_step = ARRAY_SIZE(star_a3_s1_brt_tbl),
#endif
	.brt_to_brt = star_a3_s1_brt_to_brt_tbl,
	.sz_brt_to_brt = ARRAY_SIZE(star_a3_s1_brt_to_brt_tbl),
};

static struct panel_dimming_info s6e3ha8_star_a3_s1_panel_dimming_info = {
	.name = "s6e3ha8_star_a3_s0",
	.dim_init_info = {
		.name = "s6e3ha8_star_a3_s0",
		.nr_tp = S6E3HA8_NR_TP,
		.tp = s6e3ha8_tp,
		.nr_luminance = S6E3HA8_NR_LUMINANCE,
		.vregout = 114085069LL, /* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3HA8_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = star_a3_s1_dimming_lut,
	},
	.target_luminance = S6E3HA8_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA8_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HA8_STAR_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HA8_STAR_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha8_star_a3_s1_panel_brightness_table,
};
#endif /* __S6E3HA8_STAR_A3_S1_PANEL_DIMMING_H___ */
