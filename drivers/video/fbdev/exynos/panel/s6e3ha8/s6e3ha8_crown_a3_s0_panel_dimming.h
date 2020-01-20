/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_crown_a3_s0_panel_dimming.h
 *
 * Header file for S6E3HA8 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_CROWN_A3_S0_PANEL_DIMMING_H___
#define __S6E3HA8_CROWN_A3_S0_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3ha8_dimming.h"

#include "s6e3ha8_crown_irc.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3HA8
 * PANEL : CROWN_A3_S0
 */
/* gray scale offset values */
static s32 crown_a3_s0_rtbl2nit[S6E3HA8_NR_TP] = { 0, 0, 0, 14, 16, 17, 13, 9, 9, 2, 1, 0 };
static s32 crown_a3_s0_rtbl3nit[S6E3HA8_NR_TP] = { 0, 0, 0, 12, 14, 12, 8, 4, 4, 0, 0, 0 };
static s32 crown_a3_s0_rtbl4nit[S6E3HA8_NR_TP] = { 0, 0, 0, 11, 13, 10, 6, 1, 4, 0, 0, 0 };
static s32 crown_a3_s0_rtbl5nit[S6E3HA8_NR_TP] = { 0, 0, 0, 10, 12, 8, 3, 1, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl6nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 11, 7, 3, 1, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl7nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 11, 7, 3, 1, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl8nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, 0, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl9nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, 0, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl10nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, 0, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl11nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, 0, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl12nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, -1, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl13nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, -1, 5, 0, 0, 0 };
static s32 crown_a3_s0_rtbl14nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 10, 6, 2, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl15nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 9, 6, 2, -1, 2, 0, 0, 0 };
static s32 crown_a3_s0_rtbl16nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 9, 6, 2, -1, 2, 0, 0, 0 };
static s32 crown_a3_s0_rtbl17nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 9, 6, 2, -1, 2, 0, 0, 0 };
static s32 crown_a3_s0_rtbl18nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 9, 5, 1, -1, 2, 0, 0, 0 };
static s32 crown_a3_s0_rtbl19nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 8, 5, 1, -1, 2, 0, 0, 0 };
static s32 crown_a3_s0_rtbl20nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 8, 5, 1, -1, 2, 0, 0, 0 };
static s32 crown_a3_s0_rtbl21nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 8, 5, 1, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl23nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 8, 4, 0, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl24nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 7, 4, 0, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl26nit[S6E3HA8_NR_TP] = { 0, 0, 0, 9, 6, 4, 0, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl27nit[S6E3HA8_NR_TP] = { 0, 0, 0, 8, 6, 3, -1, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl29nit[S6E3HA8_NR_TP] = { 0, 0, 0, 7, 5, 2, -1, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl31nit[S6E3HA8_NR_TP] = { 0, 0, 0, 7, 5, 2, -1, -1, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl33nit[S6E3HA8_NR_TP] = { 0, 0, 0, 5, 5, 2, -1, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl35nit[S6E3HA8_NR_TP] = { 0, 0, 0, 5, 4, 1, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl37nit[S6E3HA8_NR_TP] = { 0, 0, 0, 5, 3, 1, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl39nit[S6E3HA8_NR_TP] = { 0, 0, 0, 5, 3, 1, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl42nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 3, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl45nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 3, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl48nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 3, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl51nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl54nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl57nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl61nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl65nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl69nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl73nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl78nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 2, 0, -2, -2, 3, 0, 0, 0 };
static s32 crown_a3_s0_rtbl83nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, 0, -2, -2, 2, 0, 1, 0 };
static s32 crown_a3_s0_rtbl88nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, -1, -2, -2, 2, 0, 1, 0 };
static s32 crown_a3_s0_rtbl94nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 0, -2, -2, 2, 0, 1, 0 };
static s32 crown_a3_s0_rtbl100nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, -1, 0, -1, 2, 0, 1, 0 };
static s32 crown_a3_s0_rtbl106nit[S6E3HA8_NR_TP] = { 0, 0, 0, 4, 2, 1, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl113nit[S6E3HA8_NR_TP] = { 0, 0, 0, 3, 1, 1, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl120nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl128nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl136nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl145nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 1, 2, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl154nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 0, 1, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl164nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 0, 1, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl174nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 0, 1, 1, 2, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl185nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 0, 1, 0, 0, 3, 0, 1, 0 };
static s32 crown_a3_s0_rtbl197nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 0, 1, 0, 0, 2, 0, 1, 0 };
static s32 crown_a3_s0_rtbl210nit[S6E3HA8_NR_TP] = { 0, 0, 0, 2, 0, 1, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl223nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl237nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl253nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, -1, 0, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl269nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, -1, 0, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl286nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, -1, 0, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl301nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, -1, 0, 0, 0, 1, 0, 1, 0 };
static s32 crown_a3_s0_rtbl317nit[S6E3HA8_NR_TP] = { 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl333nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl340nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl347nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -2, -1, -1, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl354nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -2, -1, -1, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl362nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -2, -1, -1, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl369nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl376nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 };
static s32 crown_a3_s0_rtbl384nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 };
static s32 crown_a3_s0_rtbl392nit[S6E3HA8_NR_TP] = { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 1, 0 };
static s32 crown_a3_s0_rtbl400nit[S6E3HA8_NR_TP] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* rgb color offset values */
static s32 crown_a3_s0_ctbl2nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 5, -5, -11, -4, 2, -6, -2, 6, -4, 0, 3, -2, 2, 4, 1, -1, 1, 0, -3, 1, -2 };
static s32 crown_a3_s0_ctbl3nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -2, -4, -9, -4, 2, -6, -3, 6, -4, 0, 3, -2, 2, 3, 1, 0, 2, 0, -1, 1, -1 };
static s32 crown_a3_s0_ctbl4nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -2, -4, -12, -5, 2, -6, -3, 6, -4, 0, 3, -2, 2, 3, 1, 0, 2, 0, -1, 0, -1 };
static s32 crown_a3_s0_ctbl5nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -3, -4, -12, -5, 2, -6, -1, 6, -2, 0, 3, -2, 2, 2, 1, 0, 2, 0, -1, 0, -1 };
static s32 crown_a3_s0_ctbl6nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -3, -3, -12, -3, 2, -6, 0, 6, -2, 0, 2, -2, 2, 2, 1, 0, 2, 0, -1, 0, -1 };
static s32 crown_a3_s0_ctbl7nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -3, -3, -12, -3, 2, -6, 0, 6, -2, 0, 2, -2, 2, 2, 1, 0, 2, 0, -1, 0, -1 };
static s32 crown_a3_s0_ctbl8nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -3, -2, -14, 0, 2, -7, 0, 6, -2, 0, 2, -2, 2, 2, 1, 1, 2, 0, 0, 1, 1 };
static s32 crown_a3_s0_ctbl9nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -3, -2, -14, 0, 2, -7, 0, 6, -2, 0, 2, -2, 2, 2, 1, 1, 2, 0, 0, 1, 1 };
static s32 crown_a3_s0_ctbl10nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -3, -2, -14, 0, 2, -7, 0, 6, -2, 0, 2, -2, 2, 2, 1, 1, 1, 0, 0, 1, 1 };
static s32 crown_a3_s0_ctbl11nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -1, -15, 1, 3, -7, 0, 6, -2, 0, 1, -2, 2, 2, 1, 1, 1, 0, 0, 1, 1 };
static s32 crown_a3_s0_ctbl12nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -1, -15, 1, 3, -7, 3, 9, 1, 0, 1, -2, 2, 2, 1, 1, 1, 0, 0, 1, 1 };
static s32 crown_a3_s0_ctbl13nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 2, 0, -16, 2, 3, -7, 3, 9, 1, 0, 1, -2, 2, 2, 1, 1, 1, 0, 0, 1, 1 };
static s32 crown_a3_s0_ctbl14nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 4, 1, -16, 2, 2, -8, 3, 9, 1, 0, 2, -2, 2, 2, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl15nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 4, 1, -16, 3, 2, -8, 4, 9, 1, 0, 2, -2, 2, 2, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl16nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -2, -19, 3, 2, -8, 4, 9, 1, 0, 2, -2, 2, 2, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl17nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -2, -19, 3, 2, -8, 2, 8, 0, 0, 2, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl18nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -5, 1, -2, -19, 3, 3, -8, 3, 8, 1, 0, 2, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl19nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -3, -20, 1, 2, -8, 3, 8, 1, 0, 2, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl20nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -2, -20, -2, 1, -8, 4, 8, 1, 0, 1, -2, 2, 1, 1, 1, 1, 0, 1, 0, 2 };
static s32 crown_a3_s0_ctbl21nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, 0, -2, -20, -2, 1, -8, 3, 8, 1, 0, 0, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl23nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -2, -4, -20, -2, 1, -8, 6, 10, 3, 0, 0, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl24nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -2, -4, -20, -2, 1, -8, 5, 9, 2, 0, 0, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl26nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 3, 0, -7, -4, -5, -20, -2, 1, -8, 3, 6, 1, 0, 0, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl27nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 10, -1, -7, -5, -4, -19, 2, 3, -6, 3, 5, 1, 0, 0, -2, 2, 1, 1, 1, 1, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl29nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 10, -1, -10, -1, 3, -15, 2, 3, -6, 3, 5, 1, 0, 0, -2, 2, 1, 1, 0, 1, 0, 1, 0, 2 };
static s32 crown_a3_s0_ctbl31nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -2, -3, 10, -1, -10, -1, 3, -14, 1, 1, -6, 2, 4, 0, 0, 0, -2, 2, 1, 1, 0, 1, 0, 1, 0, 2 };
static s32 crown_a3_s0_ctbl33nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, -1, -3, 10, -1, -10, -2, 4, -11, 2, 2, -5, 3, 5, 1, 0, 0, -2, 2, 1, 1, 0, 1, 0, 0, -1, 1 };
static s32 crown_a3_s0_ctbl35nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -3, 10, -1, -10, 0, 5, -10, 4, 4, -3, 3, 5, 1, -1, -1, -2, 2, 1, 1, 0, 1, 0, 0, -1, 1 };
static s32 crown_a3_s0_ctbl37nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -3, 10, -1, -10, 0, 5, -9, 4, 4, -1, 3, 5, 1, -1, -1, -2, 2, 1, 1, 0, 1, 0, 0, -1, 1 };
static s32 crown_a3_s0_ctbl39nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -3, 9, -1, -13, 0, 4, -9, 4, 4, -1, 3, 5, 1, -1, -1, -2, 2, 1, 1, 0, 1, 0, 0, -1, 1 };
static s32 crown_a3_s0_ctbl42nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 4, 9, -1, -13, 0, 3, -8, 4, 4, -1, 3, 4, 1, -1, -1, -2, 2, 1, 1, 0, 1, 0, 0, -1, 1 };
static s32 crown_a3_s0_ctbl45nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 2, 10, 0, -12, 0, 3, -7, 4, 4, -1, 3, 3, 1, -1, -1, -2, 1, 1, 1, 0, 1, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl48nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, 0, 10, 0, -12, 0, 2, -6, 4, 4, 0, 3, 3, 1, -1, -1, -2, 1, 1, 1, 0, 1, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl51nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 0, -1, 10, 2, -10, 2, 2, -5, 4, 4, 0, 3, 3, 1, -1, -2, -2, 1, 1, 1, 0, 1, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl54nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, -1, -4, 10, 2, -10, 2, 2, -5, 2, 2, 0, 3, 3, 1, -1, -2, -2, 1, 1, 1, 0, 1, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl57nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -3, -3, -8, 7, 1, -9, 2, 2, -5, 2, 2, 0, 2, 2, 1, -1, -2, -2, 0, 1, 1, 0, 1, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl61nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -5, 5, 1, -9, 2, 2, -5, 2, 1, 0, 2, 1, 0, -1, -2, -2, 0, 1, 1, -1, 0, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl65nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -5, 5, 1, -9, 2, 2, -5, 1, 2, 0, 2, 2, 1, -1, -2, -2, 0, 1, 1, -1, 0, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl69nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, -7, 6, 2, -8, 0, 2, -3, 1, 2, 0, 2, 2, 1, -1, -2, -2, 0, 1, 1, 0, 0, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl73nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -2, -6, 8, 4, -8, -2, 2, -3, 1, 2, 0, 2, 2, 1, -1, -2, -2, 0, 1, 1, 0, 0, 0, 1, -1, 1 };
static s32 crown_a3_s0_ctbl78nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -2, -7, 8, 4, -8, 0, 3, -1, 0, 1, -1, 3, 2, 1, -2, -2, -2, 0, 1, 1, 1, 0, 0, 0, -1, 0 };
static s32 crown_a3_s0_ctbl83nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -2, -7, 6, 4, -7, 2, 2, -2, 0, 2, 0, 2, 2, 1, -2, -2, -2, 0, 1, 1, 1, 0, 0, 0, -1, 1 };
static s32 crown_a3_s0_ctbl88nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -2, -7, 6, 4, -7, 1, 2, 0, 0, 2, 0, 3, 2, 1, -2, -2, -2, 0, 1, 1, 1, 0, 0, 0, 0, 1 };
static s32 crown_a3_s0_ctbl94nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -3, -7, -1, 1, -12, 3, 4, 0, 1, 2, -2, 3, 2, 1, -2, -2, -2, 0, 1, 1, 1, 0, 0, -1, -2, 0 };
static s32 crown_a3_s0_ctbl100nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -1, -6, -1, 2, -14, 1, 0, 0, 1, 1, 0, 2, 2, 1, -2, -2, -2, 0, 1, 1, 1, 0, 0, -1, -2, 0 };
static s32 crown_a3_s0_ctbl106nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -3, -8, 3, 2, -10, 1, 1, 0, 1, 1, 0, 2, 2, 1, -1, -2, -2, 0, 1, 1, 1, 0, 0, -1, -2, 0 };
static s32 crown_a3_s0_ctbl113nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 2, -2, 3, 3, -7, 2, 1, 0, 1, 1, 0, 2, 2, 1, -1, -2, -2, 0, 1, 1, 1, 0, 0, -2, -2, 0 };
static s32 crown_a3_s0_ctbl120nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 2, -2, 3, 4, -6, 1, 1, 0, 1, 1, 0, 3, 2, 1, -1, -2, -2, 0, 1, 1, 1, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl128nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 2, -1, 4, 4, -5, 1, 1, 0, 1, 2, 0, 2, 1, 1, -2, -2, -2, 0, 1, 1, 1, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl136nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 2, -1, 3, 3, -6, 0, 0, -1, 1, 2, 0, 1, 1, 1, -2, -3, -2, 0, 1, 1, 1, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl145nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 2, -1, 2, 3, -7, -1, -2, -1, 1, 1, 0, 2, 2, 1, -2, -2, -2, 1, 2, 1, 1, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl154nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 2, -3, 3, 4, -3, 0, 0, 0, 1, 1, 0, 2, 2, 1, -2, -3, -2, 1, 2, 1, 0, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl164nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 1, -3, 4, 5, -4, 1, 2, -1, 1, 0, 0, 1, 0, 1, -3, -3, -2, 1, 2, 1, 0, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl174nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 1, -3, 3, 1, -5, 0, 1, 0, 1, 0, 0, 2, 2, 1, -3, -3, -2, 1, 2, 1, 0, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl185nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -4, -6, 0, 0, -2, 0, 0, 0, 0, 0, 0, 2, 2, 1, -3, -3, -2, 1, 2, 1, 0, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl197nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -5, -6, 0, -1, -2, 0, 0, 0, 0, 0, 0, 2, 1, 1, -3, -3, -2, 1, 2, 1, 0, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl210nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -6, -7, 0, -1, -2, -1, -1, 0, 0, 0, 0, 2, 1, 1, -3, -3, -2, 1, 2, 1, 0, 0, 0, 0, -1, 0 };
static s32 crown_a3_s0_ctbl223nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -2, -4, 1, 0, -2, -1, -1, 0, 1, 0, 0, 2, 1, 1, -3, -3, -2, 1, 2, 1, 0, 0, 0, -1, -2, -1 };
static s32 crown_a3_s0_ctbl237nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, -4, -4, 1, 0, -2, -1, -1, 0, 0, 0, 0, 2, 0, 1, -3, -3, -2, 1, 1, 0, -1, -1, -1, 0, -1, 0 };
static s32 crown_a3_s0_ctbl253nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -6, -4, 2, -1, 0, -1, -1, 0, 1, 0, 0, 1, 1, 1, -3, -3, -2, 2, 2, 1, 0, 0, 0, 1, 0, 1 };
static s32 crown_a3_s0_ctbl269nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -5, -4, 2, -2, 0, -1, -1, 0, 1, 0, 0, 1, 1, 1, -3, -3, -2, 2, 2, 1, 0, -1, 0, 1, 0, 1 };
static s32 crown_a3_s0_ctbl286nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -4, 2, -1, 0, -1, -1, 0, 1, 0, 0, 1, 1, 1, -2, -2, -2, 1, 2, 1, 0, -1, 0, 1, 0, 1 };
static s32 crown_a3_s0_ctbl301nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -4, 2, -1, 0, -1, 0, 0, 1, 0, 0, 1, 1, 1, -3, -3, -2, 1, 2, 1, 0, -1, 0, 1, 0, 1 };
static s32 crown_a3_s0_ctbl317nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, -2, 3, -1, -3, -2, -1, 0, 0, 0, 0, 0, 1, 1, 1, -3, -3, -2, 1, 2, 1, 0, -1, -1, 1, 0, 1 };
static s32 crown_a3_s0_ctbl333nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, -4, 2, -2, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, -5, -5, -4, 0, 1, 0, 0, -1, -1, 0, 0, 1 };
static s32 crown_a3_s0_ctbl340nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, -2, 3, 2, -1, 0, -3, -3, -2, 3, 2, 2, 1, 1, 1, -3, -3, -2, 0, 1, 1, 0, -1, -1, 1, 1, 1 };
static s32 crown_a3_s0_ctbl347nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -2, 1, 2, -1, 1, 0, -1, 0, 1, 1, 0, 1, 1, 1, -3, -2, -2, 0, 1, 1, 0, -1, -1, 0, 0, 0 };
static s32 crown_a3_s0_ctbl354nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, -2, 1, 1, -2, 0, 0, -1, 0, 1, 1, 0, 1, 1, 1, -3, -2, -2, 0, 1, 1, 0, -1, -1, 0, 0, 0 };
static s32 crown_a3_s0_ctbl362nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -5, -2, 4, 0, 2, 0, -1, 0, 1, 1, 0, 1, 1, 1, -3, -2, -2, 0, 1, 1, 0, -1, -1, 0, 0, 0 };
static s32 crown_a3_s0_ctbl369nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, -1, 1, 0, -4, -2, 0, -1, 0, 1, 1, 0, 1, 1, 1, -3, -2, -2, 0, 1, 1, 0, -1, -1, 0, 0, 0 };
static s32 crown_a3_s0_ctbl376nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 2, 1, 1, -3, -2, -2, -1, 0, 0, 1, 0, 0, 0, 0, 0 };
static s32 crown_a3_s0_ctbl384nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 2, 1, 1, -3, -2, -2, -1, 0, 0, 1, 0, 0, 0, 0, 0 };
static s32 crown_a3_s0_ctbl392nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, -2, -1, -1, -1, 0, 0, 0, -1, -1, 0, 0, 0 };
static s32 crown_a3_s0_ctbl400nit[S6E3HA8_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct dimming_lut crown_a3_s0_dimming_lut[] = {
	DIM_LUT_V0_INIT(2, 104, GAMMA_2_15, crown_a3_s0_rtbl2nit, crown_a3_s0_ctbl2nit),
	DIM_LUT_V0_INIT(3, 104, GAMMA_2_15, crown_a3_s0_rtbl3nit, crown_a3_s0_ctbl3nit),
	DIM_LUT_V0_INIT(4, 104, GAMMA_2_15, crown_a3_s0_rtbl4nit, crown_a3_s0_ctbl4nit),
	DIM_LUT_V0_INIT(5, 104, GAMMA_2_15, crown_a3_s0_rtbl5nit, crown_a3_s0_ctbl5nit),
	DIM_LUT_V0_INIT(6, 104, GAMMA_2_15, crown_a3_s0_rtbl6nit, crown_a3_s0_ctbl6nit),
	DIM_LUT_V0_INIT(7, 104, GAMMA_2_15, crown_a3_s0_rtbl7nit, crown_a3_s0_ctbl7nit),
	DIM_LUT_V0_INIT(8, 104, GAMMA_2_15, crown_a3_s0_rtbl8nit, crown_a3_s0_ctbl8nit),
	DIM_LUT_V0_INIT(9, 104, GAMMA_2_15, crown_a3_s0_rtbl9nit, crown_a3_s0_ctbl9nit),
	DIM_LUT_V0_INIT(10, 104, GAMMA_2_15, crown_a3_s0_rtbl10nit, crown_a3_s0_ctbl10nit),
	DIM_LUT_V0_INIT(11, 104, GAMMA_2_15, crown_a3_s0_rtbl11nit, crown_a3_s0_ctbl11nit),
	DIM_LUT_V0_INIT(12, 104, GAMMA_2_15, crown_a3_s0_rtbl12nit, crown_a3_s0_ctbl12nit),
	DIM_LUT_V0_INIT(13, 104, GAMMA_2_15, crown_a3_s0_rtbl13nit, crown_a3_s0_ctbl13nit),
	DIM_LUT_V0_INIT(14, 104, GAMMA_2_15, crown_a3_s0_rtbl14nit, crown_a3_s0_ctbl14nit),
	DIM_LUT_V0_INIT(15, 104, GAMMA_2_15, crown_a3_s0_rtbl15nit, crown_a3_s0_ctbl15nit),
	DIM_LUT_V0_INIT(16, 104, GAMMA_2_15, crown_a3_s0_rtbl16nit, crown_a3_s0_ctbl16nit),
	DIM_LUT_V0_INIT(17, 104, GAMMA_2_15, crown_a3_s0_rtbl17nit, crown_a3_s0_ctbl17nit),
	DIM_LUT_V0_INIT(18, 104, GAMMA_2_15, crown_a3_s0_rtbl18nit, crown_a3_s0_ctbl18nit),
	DIM_LUT_V0_INIT(19, 104, GAMMA_2_15, crown_a3_s0_rtbl19nit, crown_a3_s0_ctbl19nit),
	DIM_LUT_V0_INIT(20, 104, GAMMA_2_15, crown_a3_s0_rtbl20nit, crown_a3_s0_ctbl20nit),
	DIM_LUT_V0_INIT(21, 104, GAMMA_2_15, crown_a3_s0_rtbl21nit, crown_a3_s0_ctbl21nit),
	DIM_LUT_V0_INIT(23, 104, GAMMA_2_15, crown_a3_s0_rtbl23nit, crown_a3_s0_ctbl23nit),
	DIM_LUT_V0_INIT(24, 104, GAMMA_2_15, crown_a3_s0_rtbl24nit, crown_a3_s0_ctbl24nit),
	DIM_LUT_V0_INIT(26, 104, GAMMA_2_15, crown_a3_s0_rtbl26nit, crown_a3_s0_ctbl26nit),
	DIM_LUT_V0_INIT(27, 104, GAMMA_2_15, crown_a3_s0_rtbl27nit, crown_a3_s0_ctbl27nit),
	DIM_LUT_V0_INIT(29, 104, GAMMA_2_15, crown_a3_s0_rtbl29nit, crown_a3_s0_ctbl29nit),
	DIM_LUT_V0_INIT(31, 104, GAMMA_2_15, crown_a3_s0_rtbl31nit, crown_a3_s0_ctbl31nit),
	DIM_LUT_V0_INIT(33, 104, GAMMA_2_15, crown_a3_s0_rtbl33nit, crown_a3_s0_ctbl33nit),
	DIM_LUT_V0_INIT(35, 104, GAMMA_2_15, crown_a3_s0_rtbl35nit, crown_a3_s0_ctbl35nit),
	DIM_LUT_V0_INIT(37, 104, GAMMA_2_15, crown_a3_s0_rtbl37nit, crown_a3_s0_ctbl37nit),
	DIM_LUT_V0_INIT(39, 104, GAMMA_2_15, crown_a3_s0_rtbl39nit, crown_a3_s0_ctbl39nit),
	DIM_LUT_V0_INIT(42, 104, GAMMA_2_15, crown_a3_s0_rtbl42nit, crown_a3_s0_ctbl42nit),
	DIM_LUT_V0_INIT(45, 104, GAMMA_2_15, crown_a3_s0_rtbl45nit, crown_a3_s0_ctbl45nit),
	DIM_LUT_V0_INIT(48, 104, GAMMA_2_15, crown_a3_s0_rtbl48nit, crown_a3_s0_ctbl48nit),
	DIM_LUT_V0_INIT(51, 104, GAMMA_2_15, crown_a3_s0_rtbl51nit, crown_a3_s0_ctbl51nit),
	DIM_LUT_V0_INIT(54, 104, GAMMA_2_15, crown_a3_s0_rtbl54nit, crown_a3_s0_ctbl54nit),
	DIM_LUT_V0_INIT(57, 105, GAMMA_2_15, crown_a3_s0_rtbl57nit, crown_a3_s0_ctbl57nit),
	DIM_LUT_V0_INIT(61, 112, GAMMA_2_15, crown_a3_s0_rtbl61nit, crown_a3_s0_ctbl61nit),
	DIM_LUT_V0_INIT(65, 118, GAMMA_2_15, crown_a3_s0_rtbl65nit, crown_a3_s0_ctbl65nit),
	DIM_LUT_V0_INIT(69, 123, GAMMA_2_15, crown_a3_s0_rtbl69nit, crown_a3_s0_ctbl69nit),
	DIM_LUT_V0_INIT(73, 131, GAMMA_2_15, crown_a3_s0_rtbl73nit, crown_a3_s0_ctbl73nit),
	DIM_LUT_V0_INIT(78, 140, GAMMA_2_15, crown_a3_s0_rtbl78nit, crown_a3_s0_ctbl78nit),
	DIM_LUT_V0_INIT(83, 149, GAMMA_2_15, crown_a3_s0_rtbl83nit, crown_a3_s0_ctbl83nit),
	DIM_LUT_V0_INIT(88, 157, GAMMA_2_15, crown_a3_s0_rtbl88nit, crown_a3_s0_ctbl88nit),
	DIM_LUT_V0_INIT(94, 168, GAMMA_2_15, crown_a3_s0_rtbl94nit, crown_a3_s0_ctbl94nit),
	DIM_LUT_V0_INIT(100, 177, GAMMA_2_15, crown_a3_s0_rtbl100nit, crown_a3_s0_ctbl100nit),
	DIM_LUT_V0_INIT(106, 186, GAMMA_2_15, crown_a3_s0_rtbl106nit, crown_a3_s0_ctbl106nit),
	DIM_LUT_V0_INIT(113, 197, GAMMA_2_15, crown_a3_s0_rtbl113nit, crown_a3_s0_ctbl113nit),
	DIM_LUT_V0_INIT(120, 210, GAMMA_2_15, crown_a3_s0_rtbl120nit, crown_a3_s0_ctbl120nit),
	DIM_LUT_V0_INIT(128, 220, GAMMA_2_15, crown_a3_s0_rtbl128nit, crown_a3_s0_ctbl128nit),
	DIM_LUT_V0_INIT(136, 230, GAMMA_2_15, crown_a3_s0_rtbl136nit, crown_a3_s0_ctbl136nit),
	DIM_LUT_V0_INIT(145, 245, GAMMA_2_15, crown_a3_s0_rtbl145nit, crown_a3_s0_ctbl145nit),
	DIM_LUT_V0_INIT(154, 258, GAMMA_2_15, crown_a3_s0_rtbl154nit, crown_a3_s0_ctbl154nit),
	DIM_LUT_V0_INIT(164, 269, GAMMA_2_15, crown_a3_s0_rtbl164nit, crown_a3_s0_ctbl164nit),
	DIM_LUT_V0_INIT(174, 284, GAMMA_2_15, crown_a3_s0_rtbl174nit, crown_a3_s0_ctbl174nit),
	DIM_LUT_V0_INIT(185, 288, GAMMA_2_15, crown_a3_s0_rtbl185nit, crown_a3_s0_ctbl185nit),
	DIM_LUT_V0_INIT(197, 288, GAMMA_2_15, crown_a3_s0_rtbl197nit, crown_a3_s0_ctbl197nit),
	DIM_LUT_V0_INIT(210, 288, GAMMA_2_15, crown_a3_s0_rtbl210nit, crown_a3_s0_ctbl210nit),
	DIM_LUT_V0_INIT(223, 288, GAMMA_2_15, crown_a3_s0_rtbl223nit, crown_a3_s0_ctbl223nit),
	DIM_LUT_V0_INIT(237, 289, GAMMA_2_15, crown_a3_s0_rtbl237nit, crown_a3_s0_ctbl237nit),
	DIM_LUT_V0_INIT(253, 298, GAMMA_2_15, crown_a3_s0_rtbl253nit, crown_a3_s0_ctbl253nit),
	DIM_LUT_V0_INIT(269, 312, GAMMA_2_15, crown_a3_s0_rtbl269nit, crown_a3_s0_ctbl269nit),
	DIM_LUT_V0_INIT(286, 329, GAMMA_2_15, crown_a3_s0_rtbl286nit, crown_a3_s0_ctbl286nit),
	DIM_LUT_V0_INIT(301, 342, GAMMA_2_15, crown_a3_s0_rtbl301nit, crown_a3_s0_ctbl301nit),
	DIM_LUT_V0_INIT(317, 356, GAMMA_2_15, crown_a3_s0_rtbl317nit, crown_a3_s0_ctbl317nit),
	DIM_LUT_V0_INIT(333, 365, GAMMA_2_15, crown_a3_s0_rtbl333nit, crown_a3_s0_ctbl333nit),
	DIM_LUT_V0_INIT(340, 373, GAMMA_2_15, crown_a3_s0_rtbl340nit, crown_a3_s0_ctbl340nit),
	DIM_LUT_V0_INIT(347, 384, GAMMA_2_15, crown_a3_s0_rtbl347nit, crown_a3_s0_ctbl347nit),
	DIM_LUT_V0_INIT(354, 384, GAMMA_2_15, crown_a3_s0_rtbl354nit, crown_a3_s0_ctbl354nit),
	DIM_LUT_V0_INIT(362, 384, GAMMA_2_15, crown_a3_s0_rtbl362nit, crown_a3_s0_ctbl362nit),
	DIM_LUT_V0_INIT(369, 384, GAMMA_2_15, crown_a3_s0_rtbl369nit, crown_a3_s0_ctbl369nit),
	DIM_LUT_V0_INIT(376, 384, GAMMA_2_15, crown_a3_s0_rtbl376nit, crown_a3_s0_ctbl376nit),
	DIM_LUT_V0_INIT(384, 388, GAMMA_2_15, crown_a3_s0_rtbl384nit, crown_a3_s0_ctbl384nit),
	DIM_LUT_V0_INIT(392, 392, GAMMA_2_15, crown_a3_s0_rtbl392nit, crown_a3_s0_ctbl392nit),
	DIM_LUT_V0_INIT(400, 400, GAMMA_2_20, crown_a3_s0_rtbl400nit, crown_a3_s0_ctbl400nit),
};

static u8 crown_a3_s0_dimming_gamma_table[S6E3HA8_CROWN_TOTAL_NR_LUMINANCE][S6E3HA8_GAMMA_CMD_CNT - 1];

static u8 crown_a3_s0_dimming_aor_table[S6E3HA8_CROWN_TOTAL_NR_LUMINANCE][2] = {
	{ 0x0B, 0x72 }, { 0x0B, 0x57 }, { 0x0B, 0x3A }, { 0x0B, 0x20 }, { 0x0B, 0x01 }, { 0x0A, 0xE9 }, { 0x0A, 0xC9 }, { 0x0A, 0xA9 }, { 0x0A, 0x84 }, { 0x0A, 0x62 },
	{ 0x0A, 0x3E }, { 0x0A, 0x19 }, { 0x09, 0xE1 }, { 0x09, 0xB3 }, { 0x09, 0x92 }, { 0x09, 0x74 }, { 0x09, 0x52 }, { 0x09, 0x34 }, { 0x09, 0x16 }, { 0x08, 0xF6 },
	{ 0x08, 0xB8 }, { 0x08, 0x9A }, { 0x08, 0x54 }, { 0x08, 0x16 }, { 0x07, 0xF8 }, { 0x07, 0xB2 }, { 0x07, 0x71 }, { 0x07, 0x0F }, { 0x06, 0xCC }, { 0x06, 0x84 },
	{ 0x06, 0x20 }, { 0x05, 0xBA }, { 0x05, 0x5C }, { 0x04, 0xEE }, { 0x04, 0x86 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 },
	{ 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 },
	{ 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x04, 0x22 }, { 0x03, 0xEE }, { 0x03, 0x42 }, { 0x02, 0xE2 }, { 0x02, 0x30 }, { 0x01, 0x46 }, { 0x01, 0x46 },
	{ 0x01, 0x46 }, { 0x01, 0x46 }, { 0x01, 0x46 }, { 0x01, 0x46 }, { 0x01, 0x46 }, { 0x01, 0x46 }, { 0x01, 0x3C }, { 0x00, 0xFD }, { 0x00, 0xB5 }, { 0x00, 0x79 },
	{ 0x00, 0x34 }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C },
	/* HBM */
	{ 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C }, { 0x00, 0x0C },
	{ 0x00, 0x0C }, { 0x00, 0x0C },
};

static u8 crown_a3_s0_dimming_vint_table[][S6E3HA8_CROWN_TOTAL_NR_LUMINANCE][1] = {
	{
		/* OVER_ZERO */
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		/* HBM */
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x24 }, { 0x25 },
		{ 0x25 }, { 0x26 },
	},
	{
		/* UNDER_ZERO */
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		/* HBM */
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x24 }, { 0x25 },
		{ 0x25 }, { 0x26 },
	},
	{
		/* UNDER_MINUS_FIFTEEN */
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 },
		/* HBM */
		{ 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x23 }, { 0x24 }, { 0x25 },
		{ 0x25 }, { 0x26 },
	},
};

static u8 crown_a3_s0_dimming_elvss_table[3][S6E3HA8_CROWN_TOTAL_NR_LUMINANCE][1] = {
	{
		/* OVER_ZERO */
		{ 0x12 }, { 0x12 }, { 0x13 }, { 0x13 }, { 0x14 }, { 0x15 }, { 0x16 }, { 0x17 }, { 0x18 }, { 0x1A },
		{ 0x1C }, { 0x1E }, { 0x20 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x20 },
		{ 0x20 }, { 0x20 }, { 0x1F }, { 0x1E }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1C },
		{ 0x1B }, { 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x17 }, { 0x17 },
		{ 0x17 }, { 0x17 }, { 0x16 }, { 0x16 },
		/* HBM */
		{ 0x24 }, { 0x22 }, { 0x21 }, { 0x20 }, { 0x1F }, { 0x1D }, { 0x1C }, { 0x1B }, { 0x19 }, { 0x18 },
		{ 0x17 }, { 0x16 },
	},
	{
		/* UNDER_ZERO */
		{ 0x1B }, { 0x1B }, { 0x1B }, { 0x1B }, { 0x1B }, { 0x1B }, { 0x1B }, { 0x1F }, { 0x1F }, { 0x1F },
		{ 0x1F }, { 0x1F }, { 0x1F }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x20 },
		{ 0x20 }, { 0x20 }, { 0x1F }, { 0x1E }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1C },
		{ 0x1B }, { 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x17 }, { 0x17 },
		{ 0x17 }, { 0x17 }, { 0x16 }, { 0x16 },
		/* HBM */
		{ 0x24 }, { 0x22 }, { 0x21 }, { 0x20 }, { 0x1F }, { 0x1D }, { 0x1C }, { 0x1B }, { 0x19 }, { 0x18 },
		{ 0x17 }, { 0x16 },
	},
	{
		/* UNDER_MINUS_FIFTEEN */
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x22 },
		{ 0x22 }, { 0x22 }, { 0x22 }, { 0x22 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x20 },
		{ 0x20 }, { 0x20 }, { 0x1F }, { 0x1E }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1D }, { 0x1C },
		{ 0x1B }, { 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x17 }, { 0x17 },
		{ 0x17 }, { 0x17 }, { 0x16 }, { 0x16 },
		/* HBM */
		{ 0x24 }, { 0x22 }, { 0x21 }, { 0x20 }, { 0x1F }, { 0x1D }, { 0x1C }, { 0x1B }, { 0x19 }, { 0x18 },
		{ 0x17 }, { 0x16 },
	},
};

static u8 crown_a3_s0_dimming_irc_table[S6E3HA8_CROWN_TOTAL_NR_LUMINANCE][S6E3HA8_IRC_VALUE_LEN] = {
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x05, 0x05, 0x05, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x06, 0x06, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x06, 0x06, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x07, 0x07, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x07, 0x07, 0x07, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x07, 0x07, 0x07, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x08, 0x08, 0x08, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x08, 0x08, 0x08, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x09, 0x09, 0x09, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0A, 0x0A, 0x0A, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0A, 0x0A, 0x0A, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0B, 0x0B, 0x0B, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0B, 0x0B, 0x0B, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0C, 0x0C, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0D, 0x0D, 0x0D, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0E, 0x0E, 0x0E, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x0F, 0x0F, 0x0F, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x10, 0x10, 0x10, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x11, 0x11, 0x11, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x12, 0x12, 0x12, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x13, 0x13, 0x13, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x14, 0x14, 0x14, 0x07, 0x07, 0x07, 0x06, 0x06, 0x06 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x15, 0x15, 0x15, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x17, 0x17, 0x17, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x18, 0x18, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x1A, 0x1A, 0x1A, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x1B, 0x1B, 0x1B, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x1D, 0x1D, 0x1D, 0x0A, 0x0A, 0x0A, 0x09, 0x09, 0x09 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x1F, 0x1F, 0x1F, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x21, 0x21, 0x21, 0x0B, 0x0B, 0x0B, 0x0A, 0x0A, 0x0A },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x23, 0x23, 0x23, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x25, 0x25, 0x25, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x27, 0x27, 0x27, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x2A, 0x2A, 0x2A, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x2C, 0x2C, 0x2C, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x2F, 0x2F, 0x2F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x32, 0x32, 0x32, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x36, 0x36, 0x36, 0x12, 0x12, 0x12, 0x11, 0x11, 0x11 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x38, 0x38, 0x38, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x3B, 0x3B, 0x3B, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x3E, 0x3E, 0x3E, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x40, 0x40, 0x40, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x41, 0x41, 0x41, 0x16, 0x16, 0x16, 0x15, 0x15, 0x15 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x42, 0x42, 0x42, 0x17, 0x17, 0x17, 0x16, 0x16, 0x16 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x44, 0x44, 0x44, 0x17, 0x17, 0x17, 0x16, 0x16, 0x16 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x45, 0x45, 0x45, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x47, 0x47, 0x47, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x48, 0x48, 0x48, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x4A, 0x4A, 0x4A, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x4B, 0x4B, 0x4B, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19 },
	/* HBM */
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x50, 0x50, 0x50, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x54, 0x54, 0x54, 0x1D, 0x1D, 0x1D, 0x1C, 0x1C, 0x1C },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x59, 0x59, 0x59, 0x1E, 0x1E, 0x1E, 0x1D, 0x1D, 0x1D },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x5E, 0x5E, 0x5E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x62, 0x62, 0x62, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x67, 0x67, 0x67, 0x23, 0x23, 0x23, 0x22, 0x22, 0x22 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x6C, 0x6C, 0x6C, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x71, 0x71, 0x71, 0x25, 0x25, 0x25, 0x26, 0x26, 0x26 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x75, 0x75, 0x75, 0x27, 0x27, 0x27, 0x27, 0x27, 0x27 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x7A, 0x7A, 0x7A, 0x29, 0x29, 0x29, 0x28, 0x28, 0x28 },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x7F, 0x7F, 0x7F, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A, 0x2A },
	{ 0x25, 0x69, 0xD6, 0x62, 0x06, 0x57, 0x53, 0x89, 0x83, 0x83, 0x83, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C },
};

static struct maptbl crown_a3_s0_dimming_param_maptbl[MAX_DIMMING_MAPTBL] = {
	[DIMMING_GAMMA_MAPTBL] = DEFINE_2D_MAPTBL(crown_a3_s0_dimming_gamma_table, init_gamma_table, getidx_dimming_maptbl, copy_gamma_maptbl),
	[DIMMING_AOR_MAPTBL] = DEFINE_2D_MAPTBL(crown_a3_s0_dimming_aor_table, init_aor_table, getidx_dimming_maptbl, copy_aor_maptbl),
	[DIMMING_VINT_MAPTBL] = DEFINE_3D_MAPTBL(crown_a3_s0_dimming_vint_table, init_vint_table, getidx_elvss_temp_table, copy_common_maptbl),
	[DIMMING_ELVSS_MAPTBL] = DEFINE_3D_MAPTBL(crown_a3_s0_dimming_elvss_table, init_elvss_table, getidx_elvss_temp_table, copy_common_maptbl),
	[DIMMING_IRC_MAPTBL] = DEFINE_2D_MAPTBL(crown_a3_s0_dimming_irc_table, init_irc_table, getidx_dimming_maptbl, copy_irc_maptbl),
};

static unsigned int crown_a3_s0_brt_to_step_tbl[] = {
	0, 25, 50, 75, 100, 125, 150, 175, 200, 225, 250, 275, 300, 325, 350, 375,
	400, 425, 450, 475, 500, 525, 550, 575, 600, 625, 650, 675, 700, 725, 750, 775,
	800, 825, 850, 875, 900, 925, 950, 975, 1000, 1025, 1050, 1075, 1100, 1125, 1150, 1175,
	1200, 1225, 1250, 1275, 1300, 1325, 1350, 1375, 1400, 1425, 1450, 1475, 1500, 1525, 1550, 1575,
	1600, 1625, 1650, 1675, 1700, 1725, 1750, 1775, 1800, 1825, 1850, 1875, 1900, 1925, 1950, 1975,
	2000, 2025, 2050, 2075, 2100, 2125, 2150, 2175, 2200, 2225, 2250, 2275, 2300, 2325, 2350, 2375,
	2400, 2425, 2450, 2475, 2500, 2525, 2550, 2575, 2600, 2625, 2650, 2675, 2700, 2725, 2750, 2775,
	2800, 2825, 2850, 2875, 2900, 2925, 2950, 2975, 3000, 3025, 3050, 3075, 3100, 3125, 3150, 3175,
	3200, 3225, 3250, 3275, 3300, 3325, 3350, 3375, 3400, 3425, 3450, 3475, 3500, 3504, 3507, 3511,
	3514, 3518, 3521, 3525, 3529, 3532, 3536, 3539, 3543, 3546, 3550, 3554, 3557, 3561, 3564, 3568,
	3571, 3575, 3579, 3582, 3586, 3589, 3593, 3596, 3600, 3607, 3614, 3621, 3629, 3636, 3643, 3650,
	3657, 3664, 3671, 3679, 3686, 3693, 3700, 3707, 3714, 3721, 3729, 3736, 3743, 3750, 3757, 3764,
	3771, 3779, 3786, 3793, 3800, 3807, 3814, 3821, 3829, 3836, 3843, 3850, 3857, 3864, 3871, 3879,
	3886, 3893, 3900, 3907, 3914, 3921, 3929, 3936, 3943, 3950, 3957, 3964, 3971, 3979, 3986, 3993,
	4000, 4007, 4014, 4021, 4029, 4036, 4043, 4050, 4057, 4064, 4071, 4079, 4086, 4093, 4100, 4107,
	4114, 4121, 4129, 4136, 4143, 4150, 4157, 4164, 4171, 4179, 4186, 4193, 4200, 4207, 4214, 4221,
	4229, 4236, 4243, 4250, 4257, 4264, 4271, 4279, 4286, 4293, 4300, 4307, 4314, 4321, 4329, 4336,
	4343, 4350, 4357, 4364, 4371, 4379, 4386, 4393, 4400, 4407, 4414, 4421, 4429, 4436, 4443, 4450,
	4457, 4464, 4471, 4479, 4486, 4493, 4500, 4507, 4514, 4521, 4529, 4536, 4543, 4550, 4557, 4564,
	4571, 4579, 4586, 4593, 4600, 4607, 4614, 4621, 4629, 4636, 4643, 4650, 4657, 4664, 4671, 4679,
	4686, 4693, 4700, 4707, 4714, 4721, 4729, 4736, 4743, 4750, 4757, 4764, 4771, 4779, 4786, 4793,
	4800, 4807, 4814, 4821, 4829, 4836, 4843, 4850, 4857, 4864, 4871, 4879, 4886, 4893, 4900, 4907,
	4914, 4921, 4929, 4936, 4943, 4950, 4957, 4964, 4971, 4979, 4986, 4993, 5000, 5007, 5014, 5021,
	5029, 5036, 5043, 5050, 5057, 5064, 5071, 5079, 5086, 5093, 5100, 5107, 5114, 5121, 5129, 5136,
	5143, 5150, 5157, 5164, 5171, 5179, 5186, 5193, 5200, 5207, 5214, 5221, 5229, 5236, 5243, 5250,
	5257, 5264, 5271, 5279, 5286, 5293, 5300, 5307, 5314, 5321, 5329, 5336, 5343, 5350, 5357, 5364,
	5371, 5379, 5386, 5393, 5400, 5407, 5414, 5421, 5429, 5436, 5443, 5450, 5457, 5464, 5471, 5479,
	5486, 5493, 5500, 5507, 5514, 5521, 5529, 5536, 5543, 5550, 5557, 5564, 5571, 5579, 5586, 5593,
	5600, 5604, 5607, 5611, 5614, 5618, 5621, 5625, 5629, 5632, 5636, 5639, 5643, 5646, 5650, 5654,
	5657, 5661, 5664, 5668, 5671, 5675, 5679, 5682, 5686, 5689, 5693, 5696, 5700, 5707, 5714, 5721,
	5729, 5736, 5743, 5750, 5757, 5764, 5771, 5779, 5786, 5793, 5800, 5807, 5814, 5821, 5829, 5836,
	5843, 5850, 5857, 5864, 5871, 5879, 5886, 5893, 5900, 5907, 5914, 5921, 5929, 5936, 5943, 5950,
	5957, 5964, 5971, 5979, 5986, 5993, 6000, 6007, 6014, 6021, 6029, 6036, 6043, 6050, 6057, 6064,
	6071, 6079, 6086, 6093, 6100, 6107, 6114, 6121, 6129, 6136, 6143, 6150, 6157, 6164, 6171, 6179,
	6186, 6193, 6200, 6207, 6214, 6221, 6229, 6236, 6243, 6250, 6257, 6264, 6271, 6279, 6286, 6293,
	6300, 6307, 6314, 6321, 6329, 6336, 6343, 6350, 6357, 6364, 6371, 6379, 6386, 6393, 6400, 6407,
	6414, 6421, 6429, 6436, 6443, 6450, 6457, 6464, 6471, 6479, 6486, 6493, 6500, 6507, 6514, 6521,
	6529, 6536, 6543, 6550, 6557, 6564, 6571, 6579, 6586, 6593, 6600, 6607, 6614, 6621, 6629, 6636,
	6643, 6650, 6657, 6664, 6671, 6679, 6686, 6693, 6700, 6707, 6714, 6721, 6729, 6736, 6743, 6750,
	6757, 6764, 6771, 6779, 6786, 6793, 6800, 6807, 6814, 6821, 6829, 6836, 6843, 6850, 6857, 6864,
	6871, 6879, 6886, 6893, 6900, 6904, 6907, 6911, 6914, 6918, 6921, 6925, 6929, 6932, 6936, 6939,
	6943, 6946, 6950, 6954, 6957, 6961, 6964, 6968, 6971, 6975, 6979, 6982, 6986, 6989, 6993, 6996,
	7000, 7007, 7014, 7021, 7029, 7036, 7043, 7050, 7057, 7064, 7071, 7079, 7086, 7093, 7100, 7107,
	7114, 7121, 7129, 7136, 7143, 7150, 7157, 7164, 7171, 7179, 7186, 7193, 7200, 7207, 7214, 7221,
	7229, 7236, 7243, 7250, 7257, 7264, 7271, 7279, 7286, 7293, 7300, 7307, 7314, 7321, 7329, 7336,
	7343, 7350, 7357, 7364, 7371, 7379, 7386, 7393, 7400, 7407, 7414, 7421, 7429, 7436, 7443, 7450,
	7457, 7464, 7471, 7479, 7486, 7493, 7500, 7507, 7514, 7521, 7529, 7536, 7543, 7550, 7557, 7564,
	7571, 7579, 7586, 7593, 7600, 7607, 7614, 7621, 7629, 7636, 7643, 7650, 7657, 7664, 7671, 7679,
	7686, 7693, 7700, 7707, 7714, 7721, 7729, 7736, 7743, 7750, 7757, 7764, 7771, 7779, 7786, 7793,
	7800, 7807, 7814, 7821, 7829, 7836, 7843, 7850, 7857, 7864, 7871, 7879, 7886, 7893, 7900, 7907,
	7914, 7921, 7929, 7936, 7943, 7950, 7957, 7964, 7971, 7979, 7986, 7993, 8000, 8007, 8014, 8021,
	8029, 8036, 8043, 8050, 8057, 8064, 8071, 8079, 8086, 8093, 8100, 8107, 8114, 8121, 8129, 8136,
	8143, 8150, 8157, 8164, 8171, 8179, 8186, 8193, 8200, 8207, 8214, 8221, 8229, 8236, 8243, 8250,
	8257, 8264, 8271, 8279, 8286, 8293, 8300, 8307, 8314, 8321, 8329, 8336, 8343, 8350, 8357, 8364,
	8371, 8379, 8386, 8393, 8400, 8407, 8414, 8421, 8429, 8436, 8443, 8450, 8457, 8464, 8471, 8479,
	8486, 8493, 8500, 8507, 8514, 8521, 8529, 8536, 8543, 8550, 8557, 8564, 8571, 8579, 8586, 8593,
	8600, 8607, 8614, 8621, 8629, 8636, 8643, 8650, 8657, 8664, 8671, 8679, 8686, 8693, 8700, 8707,
	8714, 8721, 8729, 8736, 8743, 8750, 8757, 8764, 8771, 8779, 8786, 8793, 8800, 8807, 8814, 8821,
	8829, 8836, 8843, 8850, 8857, 8864, 8871, 8879, 8886, 8893, 8900, 8907, 8914, 8921, 8929, 8936,
	8943, 8950, 8957, 8964, 8971, 8979, 8986, 8993, 9000, 9007, 9014, 9021, 9029, 9036, 9043, 9050,
	9057, 9064, 9071, 9079, 9086, 9093, 9100, 9107, 9114, 9121, 9129, 9136, 9143, 9150, 9157, 9164,
	9171, 9179, 9186, 9193, 9200, 9207, 9214, 9221, 9229, 9236, 9243, 9250, 9257, 9264, 9271, 9279,
	9286, 9293, 9300, 9307, 9314, 9321, 9329, 9336, 9343, 9350, 9357, 9364, 9371, 9379, 9386, 9393,
	9400, 9407, 9414, 9421, 9429, 9436, 9443, 9450, 9457, 9464, 9471, 9479, 9486, 9493, 9500, 9507,
	9514, 9521, 9529, 9536, 9543, 9550, 9557, 9564, 9571, 9579, 9586, 9593, 9600, 9607, 9614, 9621,
	9629, 9636, 9643, 9650, 9657, 9664, 9671, 9679, 9686, 9693, 9700, 9707, 9714, 9721, 9729, 9736,
	9743, 9750, 9757, 9764, 9771, 9779, 9786, 9793, 9800, 9807, 9814, 9821, 9829, 9836, 9843, 9850,
	9857, 9864, 9871, 9879, 9886, 9893, 9900, 9907, 9914, 9921, 9929, 9936, 9943, 9950, 9957, 9964,
	9971, 9979, 9986, 9993, 10000, 10007, 10014, 10021, 10029, 10036, 10043, 10050, 10057, 10064, 10071, 10079,
	10086, 10093, 10100, 10107, 10114, 10121, 10129, 10136, 10143, 10150, 10157, 10164, 10171, 10179, 10186, 10193,
	10200, 10207, 10214, 10221, 10229, 10236, 10243, 10250, 10257, 10264, 10271, 10279, 10286, 10293, 10300, 10307,
	10314, 10321, 10329, 10336, 10343, 10350, 10357, 10364, 10371, 10379, 10386, 10393, 10400, 10407, 10414, 10421,
	10429, 10436, 10443, 10450, 10457, 10464, 10471, 10479, 10486, 10493, 10500, 10507, 10514, 10521, 10529, 10536,
	10543, 10550, 10557, 10564, 10571, 10579, 10586, 10593, 10600, 10607, 10614, 10621, 10629, 10636, 10643, 10650,
	10657, 10664, 10671, 10679, 10686, 10693, 10700, 10707, 10714, 10721, 10729, 10736, 10743, 10750, 10757, 10764,
	10771, 10779, 10786, 10793, 10800, 10807, 10814, 10821, 10829, 10836, 10843, 10850, 10857, 10864, 10871, 10879,
	10886, 10893, 10900, 10907, 10914, 10921, 10929, 10936, 10943, 10950, 10957, 10964, 10971, 10979, 10986, 10993,
	11000, 11007, 11014, 11021, 11029, 11036, 11043, 11050, 11057, 11064, 11071, 11079, 11086, 11093, 11100, 11107,
	11114, 11121, 11129, 11136, 11143, 11150, 11157, 11164, 11171, 11179, 11186, 11193, 11200, 11207, 11214, 11221,
	11229, 11236, 11243, 11250, 11257, 11264, 11271, 11279, 11286, 11293, 11300, 11307, 11314, 11321, 11329, 11336,
	11343, 11350, 11357, 11364, 11371, 11379, 11386, 11393, 11400, 11407, 11414, 11421, 11429, 11436, 11443, 11450,
	11457, 11464, 11471, 11479, 11486, 11493, 11500, 11507, 11514, 11521, 11529, 11536, 11543, 11550, 11557, 11564,
	11571, 11579, 11586, 11593, 11600, 11607, 11614, 11621, 11628, 11634, 11641, 11648, 11655, 11662, 11669, 11676,
	11683, 11690, 11697, 11703, 11710, 11717, 11724, 11731, 11738, 11745, 11752, 11759, 11766, 11772, 11779, 11786,
	11793, 11800, 11807, 11814, 11821, 11828, 11834, 11841, 11848, 11855, 11862, 11869, 11876, 11883, 11890, 11897,
	11903, 11910, 11917, 11924, 11931, 11938, 11945, 11952, 11959, 11966, 11972, 11979, 11986, 11993, 12000, 12007,
	12014, 12021, 12028, 12034, 12041, 12048, 12055, 12062, 12069, 12076, 12083, 12090, 12097, 12103, 12110, 12117,
	12124, 12131, 12138, 12145, 12152, 12159, 12166, 12172, 12179, 12186, 12193, 12200, 12207, 12214, 12221, 12229,
	12236, 12243, 12250, 12257, 12264, 12271, 12279, 12286, 12293, 12300, 12307, 12314, 12321, 12329, 12336, 12343,
	12350, 12357, 12364, 12371, 12379, 12386, 12393, 12400, 12407, 12414, 12421, 12429, 12436, 12443, 12450, 12457,
	12464, 12471, 12479, 12486, 12493, 12500, 12507, 12514, 12521, 12529, 12536, 12543, 12550, 12557, 12564, 12571,
	12579, 12586, 12593, 12600, 12607, 12614, 12621, 12629, 12636, 12643, 12650, 12657, 12664, 12671, 12679, 12686,
	12693, 12700, 12707, 12714, 12721, 12729, 12736, 12743, 12750, 12757, 12764, 12771, 12779, 12786, 12793, 12800,
	12825, 12850, 12875, 12900, 12925, 12950, 12975, 13000, 13025, 13050, 13075, 13100, 13125, 13150, 13175, 13200,
	13225, 13250, 13275, 13300, 13325, 13350, 13375, 13400, 13425, 13450, 13475, 13500, 13525, 13550, 13575, 13600,
	13625, 13650, 13675, 13700, 13725, 13750, 13775, 13800, 13825, 13850, 13875, 13900, 13925, 13950, 13975, 14000,
	14025, 14050, 14075, 14100, 14125, 14150, 14175, 14200, 14225, 14250, 14275, 14300, 14325, 14350, 14375, 14400,
	14425, 14450, 14475, 14500, 14525, 14550, 14575, 14600, 14625, 14650, 14675, 14700, 14725, 14750, 14775, 14800,
	14825, 14850, 14875, 14900, 14929, 14957, 14986, 15014, 15043, 15071, 15100, 15129, 15157, 15186, 15214, 15243,
	15271, 15300, 15329, 15357, 15386, 15414, 15443, 15471, 15500, 15529, 15557, 15586, 15614, 15643, 15671, 15700,
	15729, 15757, 15786, 15814, 15843, 15871, 15900, 15929, 15957, 15986, 16014, 16043, 16071, 16100, 16129, 16157,
	16186, 16214, 16243, 16271, 16300, 16329, 16357, 16386, 16414, 16443, 16471, 16500, 16532, 16564, 16596, 16629,
	16661, 16693, 16725, 16757, 16789, 16821, 16854, 16886, 16918, 16950, 16982, 17014, 17046, 17079, 17111, 17143,
	17175, 17207, 17239, 17271, 17304, 17336, 17368, 17400, 17432, 17464, 17496, 17529, 17561, 17593, 17625, 17657,
	17689, 17721, 17754, 17786, 17818, 17850, 17882, 17914, 17946, 17979, 18011, 18043, 18075, 18107, 18139, 18171,
	18204, 18236, 18268, 18300, 18336, 18371, 18407, 18443, 18479, 18514, 18550, 18586, 18621, 18657, 18693, 18729,
	18764, 18800, 18836, 18871, 18907, 18943, 18979, 19014, 19050, 19086, 19121, 19157, 19193, 19229, 19264, 19300,
	19329, 19357, 19386, 19414, 19443, 19471, 19500, 19529, 19557, 19586, 19614, 19643, 19671, 19700, 19729, 19757,
	19786, 19814, 19843, 19871, 19900, 19929, 19957, 19986, 20014, 20043, 20071, 20100, 20132, 20164, 20196, 20229,
	20261, 20293, 20325, 20357, 20389, 20421, 20454, 20486, 20518, 20550, 20582, 20614, 20646, 20679, 20711, 20743,
	20775, 20807, 20839, 20871, 20904, 20936, 20968, 21000, 21032, 21064, 21096, 21129, 21161, 21193, 21225, 21257,
	21289, 21321, 21354, 21386, 21418, 21450, 21482, 21514, 21546, 21579, 21611, 21643, 21675, 21707, 21739, 21771,
	21804, 21836, 21868, 21900, 21914, 21929, 21943, 21957, 21971, 21986, 22000, 22014, 22029, 22043, 22057, 22071,
	22086, 22100, 22114, 22129, 22143, 22157, 22171, 22186, 22200, 22214, 22229, 22243, 22257, 22271, 22286, 22300,
	22314, 22329, 22343, 22357, 22371, 22386, 22400, 22414, 22429, 22443, 22457, 22471, 22486, 22500, 22514, 22529,
	22543, 22557, 22571, 22586, 22600, 22614, 22629, 22643, 22657, 22671, 22686, 22700, 22711, 22721, 22732, 22743,
	22754, 22764, 22775, 22786, 22796, 22807, 22818, 22829, 22839, 22850, 22861, 22871, 22882, 22893, 22904, 22914,
	22925, 22936, 22946, 22957, 22968, 22979, 22989, 23000, 23014, 23029, 23043, 23057, 23071, 23086, 23100, 23114,
	23129, 23143, 23157, 23171, 23186, 23200, 23214, 23229, 23243, 23257, 23271, 23286, 23300, 23314, 23329, 23343,
	23357, 23371, 23386, 23400, 23414, 23429, 23443, 23457, 23471, 23486, 23500, 23514, 23529, 23543, 23557, 23571,
	23586, 23600, 23614, 23629, 23643, 23657, 23671, 23686, 23700, 23714, 23729, 23743, 23757, 23771, 23786, 23800,
	23814, 23829, 23843, 23857, 23871, 23886, 23900, 23914, 23929, 23943, 23957, 23971, 23986, 24000, 24014, 24029,
	24043, 24057, 24071, 24086, 24100, 24114, 24129, 24143, 24157, 24171, 24186, 24200, 24214, 24229, 24243, 24257,
	24271, 24286, 24300, 24314, 24329, 24343, 24357, 24371, 24386, 24400, 24414, 24429, 24443, 24457, 24471, 24486,
	24500, 24514, 24529, 24543, 24557, 24571, 24586, 24600, 24614, 24629, 24643, 24657, 24671, 24686, 24700, 24714,
	24729, 24743, 24757, 24771, 24786, 24800, 24814, 24829, 24843, 24857, 24871, 24886, 24900, 24914, 24929, 24943,
	24957, 24971, 24986, 25000, 25018, 25036, 25054, 25071, 25089, 25107, 25125, 25143, 25161, 25179, 25196, 25214,
	25232, 25250, 25268, 25286, 25304, 25321, 25339, 25357, 25375, 25393, 25411, 25429, 25446, 25464, 25482, 25500,
	/* HBM */
	25800, 26100, 26500, 26800, 27100, 27400, 27700, 28100, 28400, 28700, 29000, 29300, 29700, 30000, 30300, 30600,
	30900, 31300, 31600, 31900, 32200, 32500, 32900, 33200, 33500, 33800, 34100, 34500, 34800, 35100, 35400, 35700,
	36100, 36400, 36700, 37000, 37300, 37700, 38000, 38300, 38600, 38900, 39200, 39500, 39800, 40100, 40400, 40800,
	41100, 41400, 41700, 42000, 42400, 42700, 43000, 43300, 43600, 44000, 44300, 44600,
};

static unsigned int crown_a3_s0_brt_tbl[S6E3HA8_CROWN_TOTAL_NR_LUMINANCE] = {
	BRT(0), BRT(7), BRT(14), BRT(21), BRT(28), BRT(35), BRT(36), BRT(38), BRT(40), BRT(42),
	BRT(44), BRT(46), BRT(48), BRT(50), BRT(52), BRT(54), BRT(56), BRT(57), BRT(59), BRT(61),
	BRT(63), BRT(65), BRT(67), BRT(69), BRT(70), BRT(72), BRT(74), BRT(76), BRT(78), BRT(80),
	BRT(82), BRT(84), BRT(86), BRT(88), BRT(90), BRT(92), BRT(94), BRT(96), BRT(98), BRT(100),
	BRT(102), BRT(104), BRT(106), BRT(108), BRT(110), BRT(112), BRT(114), BRT(116), BRT(118), BRT(120),
	BRT(122), BRT(124), BRT(126), BRT(128), BRT(135), BRT(142), BRT(149), BRT(157), BRT(165), BRT(174),
	BRT(183), BRT(193), BRT(201), BRT(210), BRT(219), BRT(223), BRT(227), BRT(230), BRT(234), BRT(238),
	BRT(242), BRT(246), BRT(250), BRT(255),
	/* HBM */
	BRT(271), BRT(287), BRT(303), BRT(319), BRT(335), BRT(351), BRT(367), BRT(383), BRT(398), BRT(414),
	BRT(430), BRT(446),
};

static unsigned int crown_a3_s0_lum_tbl[S6E3HA8_CROWN_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	23, 24, 26, 27, 29, 31, 33, 35, 37, 39,
	42, 45, 48, 51, 54, 57, 61, 65, 69, 73,
	78, 83, 88, 94, 100, 106, 113, 120, 128, 136,
	145, 154, 164, 174, 185, 197, 210, 223, 237, 253,
	269, 286, 301, 317, 333, 340, 347, 354, 362, 369,
	376, 384, 392, 400,
	/* HBM */
	425, 450, 475, 500, 525, 550, 575, 600, 625, 650,
	675, 700,
};

struct brightness_table s6e3ha8_crown_a3_s0_panel_brightness_table = {
	.brt = crown_a3_s0_brt_tbl,
	.sz_brt = ARRAY_SIZE(crown_a3_s0_brt_tbl),
	.lum = crown_a3_s0_lum_tbl,
	.sz_lum = ARRAY_SIZE(crown_a3_s0_lum_tbl),
	.sz_ui_lum = S6E3HA8_NR_LUMINANCE,
	.sz_hbm_lum = S6E3HA8_CROWN_NR_HBM_LUMINANCE,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = crown_a3_s0_brt_to_step_tbl,
	.sz_brt_to_step = ARRAY_SIZE(crown_a3_s0_brt_to_step_tbl),
	.vtotal = 2984,
};

static struct panel_dimming_info s6e3ha8_crown_a3_s0_preliminary_panel_dimming_info = {
	.name = "s6e3ha8_crown_a3_s0_preliminary",
	.dim_init_info = {
		.name = "s6e3ha8_crown_a3_s0_preliminary",
		.nr_tp = S6E3HA8_NR_TP,
		.tp = s6e3ha8_crown_tp,
		.nr_luminance = S6E3HA8_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.v0_voltage = {
			0, 6, 12, 18, 24, 30, 36, 42, 48, 54,
			60, 66, 72, 78, 84, 90,
		},
		.target_luminance = S6E3HA8_CROWN_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = crown_a3_s0_dimming_lut,
	},
	.target_luminance = S6E3HA8_CROWN_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA8_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HA8_CROWN_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HA8_CROWN_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha8_crown_a3_s0_panel_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = crown_a3_s0_dimming_param_maptbl,
	.dim_flash_on = false,	/* read dim flash when probe or not */
	.irc_info = &s6e3ha8_crown_irc,
};

static struct panel_dimming_info s6e3ha8_crown_a3_s0_panel_dimming_info = {
	.name = "s6e3ha8_crown_a3_s0",
	.dim_init_info = {
		.name = "s6e3ha8_crown_a3_s0",
		.nr_tp = S6E3HA8_NR_TP,
		.tp = s6e3ha8_crown_tp,
		.nr_luminance = S6E3HA8_NR_LUMINANCE,
		.vregout = 114085069LL,	/* 6.8 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.v0_voltage = {
			0, 6, 12, 18, 24, 30, 36, 42, 48, 54,
			60, 66, 72, 78, 84, 90,
		},
		.target_luminance = S6E3HA8_CROWN_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = crown_a3_s0_dimming_lut,
	},
	.target_luminance = S6E3HA8_CROWN_TARGET_LUMINANCE,
	.nr_luminance = S6E3HA8_NR_LUMINANCE,
	.hbm_target_luminance = S6E3HA8_CROWN_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3HA8_CROWN_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3ha8_crown_a3_s0_panel_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = crown_a3_s0_dimming_param_maptbl,
	.dim_flash_on = true,	/* read dim flash when probe or not */
	.irc_info = &s6e3ha8_crown_irc,
};
#endif /* __S6E3HA8_CROWN_A3_S0_PANEL_DIMMING_H___ */
