/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7_beyond0_a3_s0_panel_dimming.h
 *
 * Header file for S6E3FA7 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA7_BEYOND0_A3_S0_PANEL_DIMMING_H___
#define __S6E3FA7_BEYOND0_A3_S0_PANEL_DIMMING_H___
#include "../dimming.h"
#include "../panel_dimming.h"
#include "s6e3fa7_dimming.h"
#include "s6e3fa7_beyond_irc.h"

/*
 * PANEL INFORMATION
 * LDI : S6E3FA7
 * PANEL : BEYOND0_A3_S0
 */
/* gray scale offset values */
static s32 beyond0_a3_s0_rtbl2nit[S6E3FA7_NR_TP] = { 0, 0, 27, 31, 30, 29, 28, 17, 8, 2, 0 };
static s32 beyond0_a3_s0_rtbl3nit[S6E3FA7_NR_TP] = { 0, 0, 22, 25, 22, 22, 20, 13, 5, 1, 0 };
static s32 beyond0_a3_s0_rtbl4nit[S6E3FA7_NR_TP] = { 0, 0, 21, 24, 22, 20, 17, 10, 4, 1, 0 };
static s32 beyond0_a3_s0_rtbl5nit[S6E3FA7_NR_TP] = { 0, 0, 18, 21, 18, 17, 13, 9, 3, 1, 0 };
static s32 beyond0_a3_s0_rtbl6nit[S6E3FA7_NR_TP] = { 0, 0, 18, 20, 17, 16, 12, 8, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl7nit[S6E3FA7_NR_TP] = { 0, 0, 18, 19, 16, 14, 10, 6, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl8nit[S6E3FA7_NR_TP] = { 0, 0, 17, 18, 15, 13, 9, 5, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl9nit[S6E3FA7_NR_TP] = { 0, 0, 17, 18, 15, 13, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl10nit[S6E3FA7_NR_TP] = { 0, 0, 16, 17, 14, 12, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl11nit[S6E3FA7_NR_TP] = { 0, 0, 16, 17, 14, 12, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl12nit[S6E3FA7_NR_TP] = { 0, 0, 16, 17, 14, 12, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl13nit[S6E3FA7_NR_TP] = { 0, 0, 16, 17, 14, 12, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl14nit[S6E3FA7_NR_TP] = { 0, 0, 16, 17, 14, 12, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl15nit[S6E3FA7_NR_TP] = { 0, 0, 15, 16, 13, 11, 9, 5, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl16nit[S6E3FA7_NR_TP] = { 0, 0, 14, 15, 12, 10, 8, 4, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl17nit[S6E3FA7_NR_TP] = { 0, 0, 14, 15, 12, 9, 8, 4, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl19nit[S6E3FA7_NR_TP] = { 0, 0, 13, 14, 11, 8, 7, 3, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl20nit[S6E3FA7_NR_TP] = { 0, 0, 14, 15, 11, 8, 7, 3, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl21nit[S6E3FA7_NR_TP] = { 0, 0, 14, 15, 11, 8, 7, 3, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl22nit[S6E3FA7_NR_TP] = { 0, 0, 13, 14, 11, 7, 6, 3, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl24nit[S6E3FA7_NR_TP] = { 0, 0, 11, 12, 9, 6, 6, 2, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl25nit[S6E3FA7_NR_TP] = { 0, 0, 10, 11, 8, 5, 5, 2, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl27nit[S6E3FA7_NR_TP] = { 0, 0, 10, 11, 8, 5, 5, 2, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl29nit[S6E3FA7_NR_TP] = { 0, 0, 10, 11, 7, 4, 5, 2, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl30nit[S6E3FA7_NR_TP] = { 0, 0, 10, 11, 7, 4, 5, 2, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl32nit[S6E3FA7_NR_TP] = { 0, 0, 9, 10, 6, 3, 4, 1, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl34nit[S6E3FA7_NR_TP] = { 0, 0, 9, 10, 6, 3, 4, 1, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl37nit[S6E3FA7_NR_TP] = { 0, 0, 8, 9, 5, 2, 3, 1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl39nit[S6E3FA7_NR_TP] = { 0, 0, 8, 9, 5, 2, 3, 1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl41nit[S6E3FA7_NR_TP] = { 0, 0, 8, 9, 5, 2, 3, 1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl44nit[S6E3FA7_NR_TP] = { 0, 0, 7, 8, 4, 1, 2, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl47nit[S6E3FA7_NR_TP] = { 0, 0, 6, 7, 3, 1, 2, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl50nit[S6E3FA7_NR_TP] = { 0, 0, 6, 7, 3, 1, 2, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl53nit[S6E3FA7_NR_TP] = { 0, 0, 5, 6, 3, 1, 1, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl56nit[S6E3FA7_NR_TP] = { 0, 0, 5, 6, 3, 1, 1, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl60nit[S6E3FA7_NR_TP] = { 0, 0, 4, 5, 3, 1, 1, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl64nit[S6E3FA7_NR_TP] = { 0, 0, 4, 5, 3, 1, 0, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl68nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 3, 2, 1, 0, 0, 1, 0 };
static s32 beyond0_a3_s0_rtbl72nit[S6E3FA7_NR_TP] = { 0, 0, 3, 3, 1, 0, 0, 0, 0, 1, 0 };
static s32 beyond0_a3_s0_rtbl77nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 2, 1, 0, 0, 0, 2, 0 };
static s32 beyond0_a3_s0_rtbl82nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 3, 1, 0, 1, 2, 2, 0 };
static s32 beyond0_a3_s0_rtbl87nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 2, 1, 0, 0, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl93nit[S6E3FA7_NR_TP] = { 0, 0, 4, 5, 2, 1, 0, 0, 1, 1, 0 };
static s32 beyond0_a3_s0_rtbl98nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 1, 0, 0, 0, 1, 1, 0 };
static s32 beyond0_a3_s0_rtbl105nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 2, 1, 0, 0, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl111nit[S6E3FA7_NR_TP] = { 0, 0, 5, 6, 3, 2, 1, 0, 2, 1, 0 };
static s32 beyond0_a3_s0_rtbl119nit[S6E3FA7_NR_TP] = { 0, 0, 4, 5, 3, 2, 1, 0, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl126nit[S6E3FA7_NR_TP] = { 0, 0, 4, 5, 3, 2, 1, 0, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl134nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 3, 2, 1, 0, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl143nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 3, 2, 1, 0, 2, -1, 0 };
static s32 beyond0_a3_s0_rtbl152nit[S6E3FA7_NR_TP] = { 0, 0, 3, 4, 2, 2, 1, 0, 2, -1, 0 };
static s32 beyond0_a3_s0_rtbl162nit[S6E3FA7_NR_TP] = { 0, 0, 2, 3, 2, 2, 1, 0, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl172nit[S6E3FA7_NR_TP] = { 0, 0, 1, 2, 1, 1, 0, 0, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl183nit[S6E3FA7_NR_TP] = { 0, 0, 2, 2, 1, 1, 0, 0, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl195nit[S6E3FA7_NR_TP] = { 0, 0, 2, 2, 1, 1, 0, 0, 2, 0, 0 };
static s32 beyond0_a3_s0_rtbl207nit[S6E3FA7_NR_TP] = { 0, 0, 2, 2, 1, 1, 0, 0, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl220nit[S6E3FA7_NR_TP] = { 0, 0, 2, 2, 1, 1, 0, 0, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl234nit[S6E3FA7_NR_TP] = { 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl249nit[S6E3FA7_NR_TP] = { 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl265nit[S6E3FA7_NR_TP] = { 0, 0, 1, 1, -1, -2, -2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl282nit[S6E3FA7_NR_TP] = { 0, 0, 0, 0, -1, -2, -2, -1, 0, 1, 0 };
static s32 beyond0_a3_s0_rtbl300nit[S6E3FA7_NR_TP] = { 0, 0, 0, -1, -1, -2, -2, -1, 1, 0, 0 };
static s32 beyond0_a3_s0_rtbl316nit[S6E3FA7_NR_TP] = { 0, 0, 0, -1, -1, -2, -2, -1, 0, 1, 0 };
static s32 beyond0_a3_s0_rtbl333nit[S6E3FA7_NR_TP] = { 0, 0, 0, -1, -1, -2, -2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl350nit[S6E3FA7_NR_TP] = { 0, 0, 0, -1, -1, -2, -2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl357nit[S6E3FA7_NR_TP] = { 0, 0, 0, -1, -2, -2, -2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl365nit[S6E3FA7_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_rtbl372nit[S6E3FA7_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -1, -1, -1, 0 };
static s32 beyond0_a3_s0_rtbl380nit[S6E3FA7_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -2, -1, -1, 0 };
static s32 beyond0_a3_s0_rtbl387nit[S6E3FA7_NR_TP] = { 0, 0, 0, -2, -2, -2, -3, -3, -2, -1, 0 };
static s32 beyond0_a3_s0_rtbl395nit[S6E3FA7_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -2, -2, 0, 0 };
static s32 beyond0_a3_s0_rtbl403nit[S6E3FA7_NR_TP] = { 0, 0, 0, -2, -2, -2, -2, -3, -2, -2, 0 };
static s32 beyond0_a3_s0_rtbl412nit[S6E3FA7_NR_TP] = { 0, 0, 0, -1, -1, -2, -2, -3, -2, -2, 0 };
static s32 beyond0_a3_s0_rtbl420nit[S6E3FA7_NR_TP] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


/* rgb color offset values */
static s32 beyond0_a3_s0_ctbl2nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 3, 1, 0, 2, 0, 0, 1, -1, -2, -1, -3, -27, -2, -22, -15, 3, -11, -7, 0, -5, -1, 3, 0, -3, 2, 0 };
static s32 beyond0_a3_s0_ctbl3nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 1, 2, 0, 1, 2, 0, -8, 1, -6, -24, 0, -20, -12, 1, -8, -6, 0, -4, -1, 3, 0, -1, 1, 0 };
static s32 beyond0_a3_s0_ctbl4nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 1, -1, 2, 3, 1, -22, -1, -20, -18, 1, -14, -12, 2, -8, -5, 0, -3, 0, 2, 0, 0, 1, 0 };
static s32 beyond0_a3_s0_ctbl5nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 5, 5, 5, 0, 1, -1, 4, 6, 3, -28, -4, -24, -13, 6, -9, -11, 0, -9, -3, 1, -2, -1, 2, 0, 0, 0, -1 };
static s32 beyond0_a3_s0_ctbl6nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, -1, 4, 6, 3, -27, -5, -24, -15, 3, -11, -10, 0, -8, -2, 1, 0, -1, 2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_ctbl7nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 1, -2, 3, 4, 1, -28, -6, -27, -14, 4, -10, -7, 1, -6, -2, 1, 0, -1, 2, -1, 0, 0, 0 };
static s32 beyond0_a3_s0_ctbl8nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 1, -2, 3, 4, 1, -29, -6, -27, -12, 4, -9, -7, 2, -4, -4, 0, -2, 1, 2, 0, 0, 0, 0 };
static s32 beyond0_a3_s0_ctbl9nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 1, -2, 3, 4, 1, -28, -6, -27, -13, 3, -9, -7, 2, -5, -2, 0, -1, -1, 2, 0, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl10nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 0, 2, -1, 1, -2, 3, 4, 1, -26, -4, -25, -13, 2, -9, -7, 1, -6, -2, 0, -1, -1, 2, 1, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl11nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 0, 2, -1, 1, -2, 3, 4, 1, -24, -4, -24, -12, 2, -9, -7, 1, -6, -2, 0, 0, -1, 2, 0, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl12nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 0, 2, -1, 1, -2, 3, 4, 1, -23, -4, -24, -12, 2, -9, -7, 1, -6, -1, 0, 0, -1, 2, 0, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl13nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 1, -1, 3, 4, 2, -21, -3, -23, -14, 0, -11, -6, 1, -5, -2, 1, -1, 0, 1, 0, -1, -1, -2 };
static s32 beyond0_a3_s0_ctbl14nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 1, -1, 3, 4, 2, -21, -3, -23, -13, 0, -11, -6, 1, -5, -2, 1, -1, 0, 1, 0, -1, -1, -2 };
static s32 beyond0_a3_s0_ctbl15nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 0, 2, 3, 3, 2, 3, 5, 2, -18, -1, -22, -11, 1, -10, -6, 1, -5, -2, 1, -1, 0, 1, 0, -1, -1, -2 };
static s32 beyond0_a3_s0_ctbl16nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 1, 2, 6, 4, 5, 3, 5, 3, -18, -1, -22, -11, 1, -10, -4, 1, -4, -2, 1, -1, -1, 1, 0, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl17nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 1, 2, 6, 4, 4, 0, 1, -1, -17, 2, -21, -11, 0, -10, -4, 1, -4, -2, 1, 0, -1, 1, -1, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl19nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 1, 2, 6, 5, 5, 0, 1, -1, -19, 0, -23, -11, 0, -9, -4, 1, -4, -1, 1, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl20nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 1, 2, -2, 0, -2, -2, 0, -10, -18, 0, -20, -10, 0, -8, -4, 1, -4, -1, 1, -1, 0, 2, 0, -1, -1, -1 };
static s32 beyond0_a3_s0_ctbl21nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 1, 2, -1, 0, -2, -4, 0, -12, -17, 0, -19, -11, 0, -9, -4, 0, -4, -1, 1, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl22nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 3, 1, 2, 8, 7, 7, -15, -3, -23, -17, -1, -19, -8, 2, -6, -4, 0, -4, -1, 1, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl24nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 4, 1, 4, 12, 9, 11, -14, 2, -20, -15, 1, -15, -9, 0, -8, -3, 1, -3, -1, 0, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl25nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 4, 2, 4, 12, 9, 11, -12, 3, -18, -14, 1, -14, -7, 2, -6, -3, 1, -3, -1, 0, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl27nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 4, 2, 4, 12, 9, 11, -12, 2, -19, -16, 0, -15, -7, 0, -6, -3, 1, -3, -1, 0, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl29nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 4, 2, 4, 1, 5, -12, -10, 3, -16, -14, 2, -14, -7, 0, -6, -3, 0, -3, -1, 0, -1, 0, 2, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl30nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 5, 2, 4, 1, 6, -10, -12, 3, -17, -14, 0, -14, -6, 1, -5, -4, 0, -4, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl32nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 5, 2, 4, 1, 5, -10, -10, 4, -15, -14, 1, -13, -6, 1, -5, -2, 1, -2, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl34nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 5, 2, 4, 1, 5, -11, -13, 3, -17, -14, 0, -13, -6, 0, -5, -2, 1, -2, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl37nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 5, 2, 5, 1, 5, -12, -13, 2, -18, -13, 0, -12, -4, 3, -3, -2, 0, -2, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl39nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 5, 2, 5, 1, 5, -11, -15, 1, -19, -13, 0, -12, -4, 2, -3, -2, 0, -2, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl41nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 6, 3, 6, -2, 5, -13, -15, 0, -19, -13, -1, -12, -5, 1, -4, -2, 0, -2, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl44nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 6, 2, 5, -1, 6, -14, -16, 0, -19, -10, -1, -9, -3, 2, -2, -1, 1, -1, -1, 0, -1, 0, 1, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl47nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 8, 2, 7, -1, 5, -15, -12, 4, -15, -11, -2, -10, -4, 1, -3, 0, 1, 0, -1, 1, -1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl50nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 7, 1, 6, -1, 5, -15, -14, 2, -17, -10, -3, -9, -5, 0, -4, -1, 1, -1, 0, 1, 0, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl53nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 9, 0, 8, -9, 4, -20, -10, 1, -12, -4, 3, -2, -2, 1, -2, -1, 1, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl56nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 9, 0, 8, -9, 2, -20, -11, 0, -12, -3, 2, -2, -2, 2, -2, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl60nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 10, -2, 9, -7, 7, -18, -11, 0, -11, -3, 1, -2, -3, 0, -3, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl64nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 10, -2, 9, -9, 6, -19, -12, -2, -12, -4, 0, -3, -2, 1, -1, 0, 0, 0, -2, 0, -1, -1, 0, 0, 1, -1, -1 };
static s32 beyond0_a3_s0_ctbl68nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 11, -2, 10, -9, 6, -19, -6, 3, -6, -6, -2, -6, -4, -1, -3, 0, 1, 0, -1, 0, -1, 0, 0, 0, -1, -3, -3 };
static s32 beyond0_a3_s0_ctbl72nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 11, -2, 10, -9, 6, -20, -7, 2, -6, 0, 2, 0, -1, 1, -1, 0, 1, 0, -2, -1, -2, 0, 0, 0, 0, -2, -1 };
static s32 beyond0_a3_s0_ctbl77nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 19, 3, 18, -9, 3, -19, -7, 2, -6, -3, 1, -3, -1, 1, -1, -1, 0, -1, -2, 0, -2, -1, 0, 0, 1, -1, -1 };
static s32 beyond0_a3_s0_ctbl82nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 17, 1, 16, -9, 6, -19, -7, 0, -7, -6, -1, -5, -2, 1, -1, 1, 1, 0, -2, -1, -2, -1, -1, -1, 0, -2, -1 };
static s32 beyond0_a3_s0_ctbl87nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 11, -1, 10, -11, 4, -19, -9, -1, -8, -5, -1, -4, -1, 2, -1, 1, 1, 1, -1, 0, -1, -1, 0, -2, 1, -1, 0 };
static s32 beyond0_a3_s0_ctbl93nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 9, 0, 7, -15, -1, -26, -4, 2, -3, -6, -2, -6, -2, 0, -2, 1, 2, 1, -2, 0, -1, 0, 0, 0, 0, -1, 0 };
static s32 beyond0_a3_s0_ctbl98nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 12, 10, 2, -16, 1, -22, -4, 0, -4, -3, 2, -2, -3, 0, -1, 0, 0, 0, -1, 0, -1, -1, -1, -1, -1, -2, -2 };
static s32 beyond0_a3_s0_ctbl105nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 14, 10, 1, -17, 2, -18, -5, -1, -5, -3, 1, -3, -4, -2, -3, 2, 2, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl111nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 12, 12, 1, -20, 1, -21, -8, -3, -7, -4, -2, -4, -4, -2, -4, 1, 1, 1, -1, 1, 0, -2, -1, -1, 0, -2, -1 };
static s32 beyond0_a3_s0_ctbl119nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 13, 10, 1, -15, 4, -15, -4, 0, -3, -5, -2, -5, -5, -3, -4, 2, 3, 2, -1, 0, -1, -1, -1, -1, 1, -1, 0 };
static s32 beyond0_a3_s0_ctbl126nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 15, 11, 1, -15, 4, -15, -5, -1, -3, -4, 0, -3, -4, -3, -4, 1, 2, 1, 0, 0, -1, -2, -1, 0, 1, -1, 0 };
static s32 beyond0_a3_s0_ctbl134nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 13, 12, 2, -11, 6, -12, -8, -3, -6, -5, -2, -4, -3, -2, -3, 0, 1, 1, 0, 0, -1, -2, -1, -1, 1, -1, 0 };
static s32 beyond0_a3_s0_ctbl143nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 11, 11, 1, -12, 5, -12, -5, -1, -4, -4, -2, -4, -4, -2, -3, 1, 1, 1, -1, 0, -1, 0, -1, -1, 1, -1, 0 };
static s32 beyond0_a3_s0_ctbl152nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 11, 11, 0, -12, 3, -12, -3, 0, -3, -5, -2, -4, -3, -3, -3, 1, 2, 1, 0, 1, 0, -1, -1, -2, 0, -1, 1 };
static s32 beyond0_a3_s0_ctbl162nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 11, 9, 1, -8, 7, -7, 0, 3, 0, -4, -3, -3, -3, -2, -3, 1, 1, 1, 0, 1, 0, -2, -1, -2, 2, 0, 1 };
static s32 beyond0_a3_s0_ctbl172nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 21, 14, 10, -7, 7, -6, -1, 2, -1, -3, -2, -2, -2, -1, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, -1, 0 };
static s32 beyond0_a3_s0_ctbl183nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 10, 9, 1, -9, 4, -8, -1, 2, -1, -2, 0, -1, -1, -1, -1, 0, 1, 0, 0, 1, 0, -1, -1, -1, 1, 0, 2 };
static s32 beyond0_a3_s0_ctbl195nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 10, 9, 0, -11, 2, -9, -1, 1, -1, -2, 0, -1, -1, -2, -1, 0, 1, 0, 0, 1, 0, -1, -1, -1, 1, -1, 0 };
static s32 beyond0_a3_s0_ctbl207nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 9, 9, 0, -14, -1, -12, -1, 0, -1, -2, 0, -1, -2, -2, -2, 0, 0, 0, 0, 1, 0, -1, -1, -1, 2, 0, 2 };
static s32 beyond0_a3_s0_ctbl220nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 8, 8, 0, -14, 0, -12, -1, -1, -1, -3, -1, -2, -1, -1, -1, -1, -1, -1, 1, 1, 1, -1, -1, -2, 1, -1, 1 };
static s32 beyond0_a3_s0_ctbl234nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 8, 9, 1, -14, -3, -12, -1, -1, -1, -3, -2, -2, -1, 0, -1, 0, -1, 0, 0, 1, 1, 0, -1, -2, 0, -1, 0 };
static s32 beyond0_a3_s0_ctbl249nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 8, 8, 1, -15, -4, -12, 2, 1, 2, -1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, -1, -1, -1, 0, -1, 0 };
static s32 beyond0_a3_s0_ctbl265nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 3, -4, -15, -4, -12, 1, -1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, -1, -1, 0, 0, -1 };
static s32 beyond0_a3_s0_ctbl282nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 1, 4, -6, -4, 3, -1, -1, -2, -1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, -1, 0, -1, -2, -2 };
static s32 beyond0_a3_s0_ctbl300nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 2, 3, -3, -4, 0, -1, -1, -3, -2, 2, 3, 2, 2, 2, 2, 0, 1, 0, 0, 1, 0, -1, -1, -1, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl316nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -1, 3, -6, -4, 0, -1, 1, 0, 1, 1, 2, 1, 2, 1, 1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 };
static s32 beyond0_a3_s0_ctbl333nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 2, -6, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, -1, 1, -1, 0, -1, 0, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl350nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -3, 0, -5, 1, 3, 2, 0, -1, 0, 1, 2, 3, 2, 1, 1, -1, 0, 0, -1, 0, 0, 1, 0, 0, -1, -2, -2 };
static s32 beyond0_a3_s0_ctbl357nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -4, 1, -6, 1, 1, 4, 3, 1, 2, -1, 0, 1, 2, 1, 1, -1, 1, 0, -1, 0, 0, 1, 0, 1, 0, -1, -2 };
static s32 beyond0_a3_s0_ctbl365nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -8, -1, -8, 2, 2, 3, 3, 2, 3, 1, 1, 2, 0, 0, 0, 0, 1, 0, -1, 0, 0, 1, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl372nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -7, -1, -8, 1, 1, 2, 3, 2, 3, 1, 1, 2, 0, 0, 0, 1, 1, 1, -1, -1, -1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl380nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -9, -1, -9, 1, 0, 3, 2, 1, 2, 1, 1, 2, 0, 0, 0, 1, 1, 1, -1, -1, -1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl387nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -7, -3, -9, 2, 1, 4, 2, 1, 1, -1, -1, 0, 0, 1, 1, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl395nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, -1, -7, 3, 1, 4, 3, 2, 2, 1, 1, 2, 0, 0, 0, -1, -1, -1, 1, 1, 1, 0, 0, 0, 0, -1, -1 };
static s32 beyond0_a3_s0_ctbl403nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, -6, -2, -6, 3, 1, 4, 3, 2, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 };
static s32 beyond0_a3_s0_ctbl412nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 0, 1, -2, -2, -3, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 2, 1, 2 };
static s32 beyond0_a3_s0_ctbl420nit[S6E3FA7_NR_TP*3] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static struct dimming_lut beyond0_a3_s0_dimming_lut[] = {
	DIM_LUT_V0_INIT(2, 107, GAMMA_2_15, beyond0_a3_s0_rtbl2nit, beyond0_a3_s0_ctbl2nit),
	DIM_LUT_V0_INIT(3, 107, GAMMA_2_15, beyond0_a3_s0_rtbl3nit, beyond0_a3_s0_ctbl3nit),
	DIM_LUT_V0_INIT(4, 107, GAMMA_2_15, beyond0_a3_s0_rtbl4nit, beyond0_a3_s0_ctbl4nit),
	DIM_LUT_V0_INIT(5, 107, GAMMA_2_15, beyond0_a3_s0_rtbl5nit, beyond0_a3_s0_ctbl5nit),
	DIM_LUT_V0_INIT(6, 107, GAMMA_2_15, beyond0_a3_s0_rtbl6nit, beyond0_a3_s0_ctbl6nit),
	DIM_LUT_V0_INIT(7, 107, GAMMA_2_15, beyond0_a3_s0_rtbl7nit, beyond0_a3_s0_ctbl7nit),
	DIM_LUT_V0_INIT(8, 107, GAMMA_2_15, beyond0_a3_s0_rtbl8nit, beyond0_a3_s0_ctbl8nit),
	DIM_LUT_V0_INIT(9, 107, GAMMA_2_15, beyond0_a3_s0_rtbl9nit, beyond0_a3_s0_ctbl9nit),
	DIM_LUT_V0_INIT(10, 107, GAMMA_2_15, beyond0_a3_s0_rtbl10nit, beyond0_a3_s0_ctbl10nit),
	DIM_LUT_V0_INIT(11, 107, GAMMA_2_15, beyond0_a3_s0_rtbl11nit, beyond0_a3_s0_ctbl11nit),
	DIM_LUT_V0_INIT(12, 107, GAMMA_2_15, beyond0_a3_s0_rtbl12nit, beyond0_a3_s0_ctbl12nit),
	DIM_LUT_V0_INIT(13, 107, GAMMA_2_15, beyond0_a3_s0_rtbl13nit, beyond0_a3_s0_ctbl13nit),
	DIM_LUT_V0_INIT(14, 107, GAMMA_2_15, beyond0_a3_s0_rtbl14nit, beyond0_a3_s0_ctbl14nit),
	DIM_LUT_V0_INIT(15, 107, GAMMA_2_15, beyond0_a3_s0_rtbl15nit, beyond0_a3_s0_ctbl15nit),
	DIM_LUT_V0_INIT(16, 107, GAMMA_2_15, beyond0_a3_s0_rtbl16nit, beyond0_a3_s0_ctbl16nit),
	DIM_LUT_V0_INIT(17, 107, GAMMA_2_15, beyond0_a3_s0_rtbl17nit, beyond0_a3_s0_ctbl17nit),
	DIM_LUT_V0_INIT(19, 107, GAMMA_2_15, beyond0_a3_s0_rtbl19nit, beyond0_a3_s0_ctbl19nit),
	DIM_LUT_V0_INIT(20, 107, GAMMA_2_15, beyond0_a3_s0_rtbl20nit, beyond0_a3_s0_ctbl20nit),
	DIM_LUT_V0_INIT(21, 107, GAMMA_2_15, beyond0_a3_s0_rtbl21nit, beyond0_a3_s0_ctbl21nit),
	DIM_LUT_V0_INIT(22, 107, GAMMA_2_15, beyond0_a3_s0_rtbl22nit, beyond0_a3_s0_ctbl22nit),
	DIM_LUT_V0_INIT(24, 107, GAMMA_2_15, beyond0_a3_s0_rtbl24nit, beyond0_a3_s0_ctbl24nit),
	DIM_LUT_V0_INIT(25, 107, GAMMA_2_15, beyond0_a3_s0_rtbl25nit, beyond0_a3_s0_ctbl25nit),
	DIM_LUT_V0_INIT(27, 107, GAMMA_2_15, beyond0_a3_s0_rtbl27nit, beyond0_a3_s0_ctbl27nit),
	DIM_LUT_V0_INIT(29, 107, GAMMA_2_15, beyond0_a3_s0_rtbl29nit, beyond0_a3_s0_ctbl29nit),
	DIM_LUT_V0_INIT(30, 107, GAMMA_2_15, beyond0_a3_s0_rtbl30nit, beyond0_a3_s0_ctbl30nit),
	DIM_LUT_V0_INIT(32, 107, GAMMA_2_15, beyond0_a3_s0_rtbl32nit, beyond0_a3_s0_ctbl32nit),
	DIM_LUT_V0_INIT(34, 107, GAMMA_2_15, beyond0_a3_s0_rtbl34nit, beyond0_a3_s0_ctbl34nit),
	DIM_LUT_V0_INIT(37, 107, GAMMA_2_15, beyond0_a3_s0_rtbl37nit, beyond0_a3_s0_ctbl37nit),
	DIM_LUT_V0_INIT(39, 107, GAMMA_2_15, beyond0_a3_s0_rtbl39nit, beyond0_a3_s0_ctbl39nit),
	DIM_LUT_V0_INIT(41, 107, GAMMA_2_15, beyond0_a3_s0_rtbl41nit, beyond0_a3_s0_ctbl41nit),
	DIM_LUT_V0_INIT(44, 107, GAMMA_2_15, beyond0_a3_s0_rtbl44nit, beyond0_a3_s0_ctbl44nit),
	DIM_LUT_V0_INIT(47, 107, GAMMA_2_15, beyond0_a3_s0_rtbl47nit, beyond0_a3_s0_ctbl47nit),
	DIM_LUT_V0_INIT(50, 107, GAMMA_2_15, beyond0_a3_s0_rtbl50nit, beyond0_a3_s0_ctbl50nit),
	DIM_LUT_V0_INIT(53, 107, GAMMA_2_15, beyond0_a3_s0_rtbl53nit, beyond0_a3_s0_ctbl53nit),
	DIM_LUT_V0_INIT(56, 107, GAMMA_2_15, beyond0_a3_s0_rtbl56nit, beyond0_a3_s0_ctbl56nit),
	DIM_LUT_V0_INIT(60, 107, GAMMA_2_15, beyond0_a3_s0_rtbl60nit, beyond0_a3_s0_ctbl60nit),
	DIM_LUT_V0_INIT(64, 109, GAMMA_2_15, beyond0_a3_s0_rtbl64nit, beyond0_a3_s0_ctbl64nit),
	DIM_LUT_V0_INIT(68, 115, GAMMA_2_15, beyond0_a3_s0_rtbl68nit, beyond0_a3_s0_ctbl68nit),
	DIM_LUT_V0_INIT(72, 120, GAMMA_2_15, beyond0_a3_s0_rtbl72nit, beyond0_a3_s0_ctbl72nit),
	DIM_LUT_V0_INIT(77, 127, GAMMA_2_15, beyond0_a3_s0_rtbl77nit, beyond0_a3_s0_ctbl77nit),
	DIM_LUT_V0_INIT(82, 135, GAMMA_2_15, beyond0_a3_s0_rtbl82nit, beyond0_a3_s0_ctbl82nit),
	DIM_LUT_V0_INIT(87, 142, GAMMA_2_15, beyond0_a3_s0_rtbl87nit, beyond0_a3_s0_ctbl87nit),
	DIM_LUT_V0_INIT(93, 151, GAMMA_2_15, beyond0_a3_s0_rtbl93nit, beyond0_a3_s0_ctbl93nit),
	DIM_LUT_V0_INIT(98, 161, GAMMA_2_15, beyond0_a3_s0_rtbl98nit, beyond0_a3_s0_ctbl98nit),
	DIM_LUT_V0_INIT(105, 170, GAMMA_2_15, beyond0_a3_s0_rtbl105nit, beyond0_a3_s0_ctbl105nit),
	DIM_LUT_V0_INIT(111, 181, GAMMA_2_15, beyond0_a3_s0_rtbl111nit, beyond0_a3_s0_ctbl111nit),
	DIM_LUT_V0_INIT(119, 192, GAMMA_2_15, beyond0_a3_s0_rtbl119nit, beyond0_a3_s0_ctbl119nit),
	DIM_LUT_V0_INIT(126, 202, GAMMA_2_15, beyond0_a3_s0_rtbl126nit, beyond0_a3_s0_ctbl126nit),
	DIM_LUT_V0_INIT(134, 214, GAMMA_2_15, beyond0_a3_s0_rtbl134nit, beyond0_a3_s0_ctbl134nit),
	DIM_LUT_V0_INIT(143, 228, GAMMA_2_15, beyond0_a3_s0_rtbl143nit, beyond0_a3_s0_ctbl143nit),
	DIM_LUT_V0_INIT(152, 241, GAMMA_2_15, beyond0_a3_s0_rtbl152nit, beyond0_a3_s0_ctbl152nit),
	DIM_LUT_V0_INIT(162, 252, GAMMA_2_15, beyond0_a3_s0_rtbl162nit, beyond0_a3_s0_ctbl162nit),
	DIM_LUT_V0_INIT(172, 268, GAMMA_2_15, beyond0_a3_s0_rtbl172nit, beyond0_a3_s0_ctbl172nit),
	DIM_LUT_V0_INIT(183, 277, GAMMA_2_15, beyond0_a3_s0_rtbl183nit, beyond0_a3_s0_ctbl183nit),
	DIM_LUT_V0_INIT(195, 277, GAMMA_2_15, beyond0_a3_s0_rtbl195nit, beyond0_a3_s0_ctbl195nit),
	DIM_LUT_V0_INIT(207, 277, GAMMA_2_15, beyond0_a3_s0_rtbl207nit, beyond0_a3_s0_ctbl207nit),
	DIM_LUT_V0_INIT(220, 277, GAMMA_2_15, beyond0_a3_s0_rtbl220nit, beyond0_a3_s0_ctbl220nit),
	DIM_LUT_V0_INIT(234, 286, GAMMA_2_15, beyond0_a3_s0_rtbl234nit, beyond0_a3_s0_ctbl234nit),
	DIM_LUT_V0_INIT(249, 292, GAMMA_2_15, beyond0_a3_s0_rtbl249nit, beyond0_a3_s0_ctbl249nit),
	DIM_LUT_V0_INIT(265, 308, GAMMA_2_15, beyond0_a3_s0_rtbl265nit, beyond0_a3_s0_ctbl265nit),
	DIM_LUT_V0_INIT(282, 324, GAMMA_2_15, beyond0_a3_s0_rtbl282nit, beyond0_a3_s0_ctbl282nit),
	DIM_LUT_V0_INIT(300, 341, GAMMA_2_15, beyond0_a3_s0_rtbl300nit, beyond0_a3_s0_ctbl300nit),
	DIM_LUT_V0_INIT(316, 355, GAMMA_2_15, beyond0_a3_s0_rtbl316nit, beyond0_a3_s0_ctbl316nit),
	DIM_LUT_V0_INIT(333, 371, GAMMA_2_15, beyond0_a3_s0_rtbl333nit, beyond0_a3_s0_ctbl333nit),
	DIM_LUT_V0_INIT(350, 387, GAMMA_2_15, beyond0_a3_s0_rtbl350nit, beyond0_a3_s0_ctbl350nit),
	DIM_LUT_V0_INIT(357, 395, GAMMA_2_15, beyond0_a3_s0_rtbl357nit, beyond0_a3_s0_ctbl357nit),
	DIM_LUT_V0_INIT(365, 402, GAMMA_2_15, beyond0_a3_s0_rtbl365nit, beyond0_a3_s0_ctbl365nit),
	DIM_LUT_V0_INIT(372, 402, GAMMA_2_15, beyond0_a3_s0_rtbl372nit, beyond0_a3_s0_ctbl372nit),
	DIM_LUT_V0_INIT(380, 402, GAMMA_2_15, beyond0_a3_s0_rtbl380nit, beyond0_a3_s0_ctbl380nit),
	DIM_LUT_V0_INIT(387, 402, GAMMA_2_15, beyond0_a3_s0_rtbl387nit, beyond0_a3_s0_ctbl387nit),
	DIM_LUT_V0_INIT(395, 404, GAMMA_2_15, beyond0_a3_s0_rtbl395nit, beyond0_a3_s0_ctbl395nit),
	DIM_LUT_V0_INIT(403, 406, GAMMA_2_15, beyond0_a3_s0_rtbl403nit, beyond0_a3_s0_ctbl403nit),
	DIM_LUT_V0_INIT(412, 409, GAMMA_2_15, beyond0_a3_s0_rtbl412nit, beyond0_a3_s0_ctbl412nit),
	DIM_LUT_V0_INIT(420, 420, GAMMA_2_20, beyond0_a3_s0_rtbl420nit, beyond0_a3_s0_ctbl420nit),
};

static u8 beyond0_a3_s0_dimming_gamma_table[S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE][S6E3FA7_GAMMA_CMD_CNT - 1];

static u8 beyond0_a3_s0_dimming_aor_table[S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE][2] = {
	{ 0x08, 0xD4 }, { 0x08, 0xBA }, { 0x08, 0xA6 }, { 0x08, 0x94 }, { 0x08, 0x7A }, { 0x08, 0x66 }, { 0x08, 0x53 }, { 0x08, 0x3A }, { 0x08, 0x25 }, { 0x08, 0x08 },
	{ 0x07, 0xF4 }, { 0x07, 0xD6 }, { 0x07, 0xC0 }, { 0x07, 0xA2 }, { 0x07, 0x86 }, { 0x07, 0x74 }, { 0x07, 0x44 }, { 0x07, 0x2E }, { 0x07, 0x18 }, { 0x07, 0x04 },
	{ 0x06, 0xD3 }, { 0x06, 0xB8 }, { 0x06, 0x8B }, { 0x06, 0x58 }, { 0x06, 0x44 }, { 0x06, 0x15 }, { 0x05, 0xE8 }, { 0x05, 0x9E }, { 0x05, 0x73 }, { 0x05, 0x3F },
	{ 0x04, 0xF9 }, { 0x04, 0xB2 }, { 0x04, 0x66 }, { 0x04, 0x1E }, { 0x03, 0xD4 }, { 0x03, 0x64 }, { 0x03, 0x16 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 },
	{ 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 },
	{ 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x02 }, { 0x03, 0x00 }, { 0x02, 0x62 }, { 0x02, 0x05 }, { 0x01, 0x80 }, { 0x01, 0x23 }, { 0x00, 0xE0 }, { 0x00, 0xE0 },
	{ 0x00, 0xE0 }, { 0x00, 0xE0 }, { 0x00, 0xE0 }, { 0x00, 0xE0 }, { 0x00, 0xE0 }, { 0x00, 0xE0 }, { 0x00, 0xDE }, { 0x00, 0xAC }, { 0x00, 0x84 }, { 0x00, 0x52 },
	{ 0x00, 0x1E }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 },
	/* HBM */
	{ 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 },	{ 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 },
	{ 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 }, { 0x00, 0x14 },
};

static u8 beyond0_a3_s0_dimming_vint_table[][S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE][1] = {
	{
		/* OVER_ZERO */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		/* HBM */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1F }, { 0x20 },
		{ 0x21 }, { 0x22 }, { 0x23 }, { 0x24 }, { 0x24 }, { 0x24 },
	},
	{
		/* UNDER_ZERO */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		/* HBM */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1F }, { 0x20 },
		{ 0x21 }, { 0x22 }, { 0x23 }, { 0x24 }, { 0x24 }, { 0x24 },

	},
	{
		/* UNDER_MINUS_FIFTEEN */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		/* HBM */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1F }, { 0x20 },
		{ 0x21 }, { 0x22 }, { 0x23 }, { 0x24 }, { 0x24 }, { 0x24 },
	},

};

static u8 beyond0_a3_s0_dimming_elvss_table[3][S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE][1] = {
	{
		/* OVER_ZERO */
		{ 0x11 }, { 0x11 }, { 0x12 }, { 0x12 }, { 0x13 }, { 0x14 }, { 0x15 }, { 0x16 }, { 0x17 }, { 0x19 },
		{ 0x1B }, { 0x1D }, { 0x1F }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x20 }, { 0x20 },
		{ 0x20 }, { 0x1F }, { 0x1F }, { 0x1F }, { 0x1F }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1D }, { 0x1D },
		{ 0x1C }, { 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x17 }, { 0x17 },
		{ 0x17 }, { 0x17 }, { 0x16 }, { 0x16 },
		/* HBM */
		{ 0x26 }, { 0x25 }, { 0x24 }, { 0x23 }, { 0x22 }, { 0x21 }, { 0x20 }, { 0x1E }, { 0x1D }, { 0x1C },
		{ 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x17 }, { 0x16 },
	},
	{
		/* UNDER_ZERO */
		{ 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x19 }, { 0x1D }, { 0x1D }, { 0x1D },
		{ 0x1D }, { 0x1D }, { 0x1D }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x20 }, { 0x20 },
		{ 0x20 }, { 0x1F }, { 0x1F }, { 0x1F }, { 0x1F }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1D }, { 0x1D },
		{ 0x1C }, { 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x17 }, { 0x17 },
		{ 0x17 }, { 0x17 }, { 0x16 }, { 0x16 },
		/* HBM */
		{ 0x26 }, { 0x25 }, { 0x24 }, { 0x23 }, { 0x22 }, { 0x21 }, { 0x20 }, { 0x1E }, { 0x1D }, { 0x1C },
		{ 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x17 }, { 0x16 },
	},
	{
		/* UNDER_MINUS_FIFTEEN */
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1E },
		{ 0x1E }, { 0x1E }, { 0x1E }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 },
		{ 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x21 }, { 0x20 }, { 0x20 },
		{ 0x20 }, { 0x1F }, { 0x1F }, { 0x1F }, { 0x1F }, { 0x1E }, { 0x1E }, { 0x1E }, { 0x1D }, { 0x1D },
		{ 0x1C }, { 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x18 }, { 0x17 }, { 0x17 },
		{ 0x17 }, { 0x17 }, { 0x16 }, { 0x16 },
		/* HBM */
		{ 0x26 }, { 0x25 }, { 0x24 }, { 0x23 }, { 0x22 }, { 0x21 }, { 0x20 }, { 0x1E }, { 0x1D }, { 0x1C },
		{ 0x1B }, { 0x1A }, { 0x19 }, { 0x18 }, { 0x17 }, { 0x16 },
	},
};

static u8 beyond0_a3_s0_dimming_irc_table[S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE][S6E3FA7_IRC_VALUE_LEN] = {
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x05, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x06, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x07, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x09, 0x09, 0x09, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x09, 0x09, 0x09, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0A, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x0B, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x0B, 0x0B, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x0D, 0x0D, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x0D, 0x0D, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x0E, 0x0E, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x12, 0x12, 0x12, 0x07, 0x07, 0x07, 0x06, 0x06, 0x06 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x15, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x16, 0x16, 0x16, 0x08, 0x08, 0x08, 0x07, 0x07, 0x07 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x08, 0x08, 0x08, 0x07, 0x07, 0x07 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x19, 0x19, 0x19, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x1B, 0x1B, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x1C, 0x1C, 0x0A, 0x0A, 0x0A, 0x09, 0x09, 0x09 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x0C, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x24, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x26, 0x26, 0x26, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x28, 0x28, 0x28, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x2B, 0x2B, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x2B, 0x2B, 0x0F, 0x0F, 0x0F, 0x0E, 0x0E, 0x0E },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x2C, 0x2C, 0x2C, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x2D, 0x2D, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x2E, 0x2E, 0x10, 0x10, 0x10, 0x0F, 0x0F, 0x0F },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x2F, 0x2F, 0x10, 0x10, 0x10, 0x0F, 0x0F, 0x0F },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x31, 0x31, 0x31, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x32, 0x32, 0x32, 0x11, 0x11, 0x11, 0x10, 0x10, 0x10 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x33, 0x33, 0x33, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 },
	/* HBM */
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x35, 0x35, 0x35, 0x12, 0x12, 0x12, 0x11, 0x11, 0x11 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x37, 0x37, 0x37, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x38, 0x38, 0x38, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x3A, 0x3A, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x3B, 0x3B, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x3D, 0x3D, 0x3D, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x3E, 0x3E, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x15, 0x15, 0x15, 0x15, 0x15, 0x15 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x16, 0x16, 0x16, 0x15, 0x15, 0x15 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x16, 0x16, 0x16, 0x15, 0x15, 0x15 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x43, 0x43, 0x43, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x43, 0x43, 0x43, 0x17, 0x17, 0x17, 0x16, 0x16, 0x16 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17 },
	{ 0x80, 0x00, 0x00, 0x00, 0x80, 0x61, 0x45, 0x42, 0x7D, 0xFC, 0x33, 0x69, 0x12, 0x7A, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x17, 0x17, 0x17, 0x16, 0x16, 0x16 },
};

static struct maptbl beyond0_a3_s0_dimming_param_maptbl[MAX_DIMMING_MAPTBL] = {
	[DIMMING_GAMMA_MAPTBL] = DEFINE_2D_MAPTBL(beyond0_a3_s0_dimming_gamma_table, init_gamma_table, getidx_dimming_maptbl, copy_gamma_maptbl),
	[DIMMING_AOR_MAPTBL] = DEFINE_2D_MAPTBL(beyond0_a3_s0_dimming_aor_table, init_aor_table, getidx_dimming_maptbl, copy_aor_maptbl),
	[DIMMING_VINT_MAPTBL] = DEFINE_3D_MAPTBL(beyond0_a3_s0_dimming_vint_table, init_vint_table, getidx_elvss_temp_table, copy_common_maptbl),
	[DIMMING_ELVSS_MAPTBL] = DEFINE_3D_MAPTBL(beyond0_a3_s0_dimming_elvss_table, init_elvss_table, getidx_elvss_temp_table, copy_common_maptbl),
	[DIMMING_IRC_MAPTBL] = DEFINE_2D_MAPTBL(beyond0_a3_s0_dimming_irc_table, init_irc_table, getidx_dimming_maptbl, copy_irc_maptbl),
};

static s32 beyond0_a3_s0_dim_flash_gamma_offset[S6E3FA7_NR_LUMINANCE * S6E3FA7_NR_TP * MAX_COLOR] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -5, 1, -17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -5, 2, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 2, -15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 2, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -3, 3, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 4, -14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -5, 4, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 6, -15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 5, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -2, 5, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -11, 4, -22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -10, 4, -21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -12, 4, -23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -13, 4, -23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -13, 4, -23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -10, 6, -21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -7, 3, -17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -10, 4, -20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -13, 1, -21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, -8, 3, -19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 12, -7, 0, 1, -4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -3, 6, -15, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -2, 6, -12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -4, 4, -15, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 6, -13, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -2, 7, -12, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -4, 6, -13, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -4, 6, -14, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -6, 5, -15, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 11, -8, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -7, 5, -15, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -7, 5, -16, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -9, 4, -17, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -9, 4, -16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -10, 4, -16, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -7, 7, -13, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -8, 5, -13, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -10, 6, -16, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -7, 5, -11, 0, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -8, 4, -12, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -7, 3, -9, 0, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -1, 2, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, -1, 3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, -1, -3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, -2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static unsigned int beyond0_a3_s0_brt_to_step_tbl[] = {
	0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500,
	1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100,
	3200, 3300, 3400, 3500, 3517, 3533, 3550, 3567, 3583, 3600, 3633, 3667, 3700, 3733, 3767, 3800,
	3833, 3867, 3900, 3933, 3967, 4000, 4029, 4057, 4086, 4114, 4143, 4171, 4200, 4233, 4267, 4300,
	4333, 4367, 4400, 4429, 4457, 4486, 4514, 4543, 4571, 4600, 4629, 4657, 4686, 4714, 4743, 4771,
	4800, 4829, 4857, 4886, 4914, 4943, 4971, 5000, 5033, 5067, 5100, 5133, 5167, 5200, 5229, 5257,
	5286, 5314, 5343, 5371, 5400, 5415, 5431, 5446, 5462, 5477, 5492, 5508, 5523, 5538, 5554, 5569,
	5585, 5600, 5614, 5629, 5643, 5657, 5671, 5686, 5700, 5729, 5757, 5786, 5814, 5843, 5871, 5900,
	5929, 5957, 5986, 6014, 6043, 6071, 6100, 6115, 6131, 6146, 6162, 6177, 6192, 6208, 6223, 6238,
	6254, 6269, 6285, 6300, 6329, 6357, 6386, 6414, 6443, 6471, 6500, 6529, 6557, 6586, 6614, 6643,
	6671, 6700, 6740, 6780, 6820, 6860, 6900, 6933, 6967, 7000, 7040, 7080, 7120, 7160, 7200, 7240,
	7280, 7320, 7360, 7400, 7429, 7457, 7486, 7514, 7543, 7571, 7600, 7650, 7700, 7750, 7800, 7840,
	7880, 7920, 7960, 8000, 8033, 8067, 8100, 8133, 8167, 8200, 8233, 8267, 8300, 8333, 8367, 8400,
	8433, 8467, 8500, 8533, 8567, 8600, 8633, 8667, 8700, 8733, 8767, 8800, 8840, 8880, 8920, 8960,
	9000, 9025, 9050, 9075, 9100, 9125, 9150, 9175, 9200, 9233, 9267, 9300, 9333, 9367, 9400, 9429,
	9457, 9486, 9514, 9543, 9571, 9600, 9633, 9667, 9700, 9733, 9767, 9800, 9825, 9850, 9875, 9900,
	9925, 9950, 9975, 10000, 10025, 10050, 10075, 10100, 10125, 10150, 10175, 10200, 10229, 10257, 10286, 10314,
	10343, 10371, 10400, 10425, 10450, 10475, 10500, 10525, 10550, 10575, 10600, 10629, 10657, 10686, 10714, 10743,
	10771, 10800, 10820, 10840, 10860, 10880, 10900, 10920, 10940, 10960, 10980, 11000, 11029, 11057, 11086, 11114,
	11143, 11171, 11200, 11220, 11240, 11260, 11280, 11300, 11320, 11340, 11360, 11380, 11400, 11425, 11450, 11475,
	11500, 11525, 11550, 11575, 11600, 11620, 11640, 11660, 11680, 11700, 11720, 11740, 11760, 11780, 11800, 11820,
	11840, 11860, 11880, 11900, 11920, 11940, 11960, 11980, 12000, 12022, 12044, 12067, 12089, 12111, 12133, 12156,
	12178, 12200, 12220, 12240, 12260, 12280, 12300, 12320, 12340, 12360, 12380, 12400, 12420, 12440, 12460, 12480,
	12500, 12520, 12540, 12560, 12580, 12600, 12618, 12636, 12655, 12673, 12691, 12709, 12727, 12745, 12764, 12782,
	12800, 12900, 13000, 13100, 13200, 13300, 13400, 13500, 13600, 13700, 13800, 13900, 14000, 14100, 14200, 14300,
	14400, 14500, 14600, 14700, 14800, 14900, 15000, 15100, 15200, 15300, 15400, 15500, 15600, 15700, 15800, 15900,
	16000, 16100, 16200, 16300, 16400, 16500, 16600, 16700, 16800, 16900, 17000, 17100, 17200, 17300, 17400, 17500,
	17600, 17700, 17800, 17900, 18000, 18100, 18200, 18300, 18400, 18500, 18600, 18700, 18800, 18900, 19000, 19100,
	19200, 19300, 19400, 19500, 19600, 19700, 19800, 19900, 20000, 20100, 20200, 20300, 20400, 20500, 20600, 20700,
	20800, 20900, 21000, 21100, 21200, 21300, 21400, 21500, 21600, 21700, 21800, 21900, 22000, 22100, 22200, 22300,
	22400, 22500, 22600, 22700, 22800, 22900, 23000, 23100, 23200, 23300, 23400, 23500, 23600, 23700, 23800, 23900,
	24000, 24100, 24200, 24300, 24400, 24500, 24600, 24700, 24800, 24900, 25000, 25100, 25200, 25300, 25400, 25500,
	/* HBM */
	25800, 26100, 26300, 26600, 26900, 27200, 27500, 27800, 28100, 28400, 28700, 29000, 29200, 29500, 29800, 30100,
	30400, 30700, 31000, 31300, 31600, 31900, 32100, 32400, 32700, 33000, 33300, 33600, 33900, 34200, 34500, 34800,
	35000, 35300, 35600, 35900, 36200, 36400, 36700, 37000, 37300, 37600, 37900, 38200, 38500, 38800, 39100, 39300,
	39600, 39900, 40200, 40500, 40800, 41100, 41400, 41700, 42000, 42200, 42500, 42800, 43100, 43400, 43600, 43900,
	44200, 44500, 44800, 45100, 45400, 45700, 46000, 46300, 46500, 46800, 47100, 47400, 47700, 48000, 48300, 48600,
};

static unsigned int beyond0_a3_s0_brt_tbl[S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE] = {
	BRT(0), BRT(7), BRT(14), BRT(21), BRT(28), BRT(35), BRT(36), BRT(38), BRT(40), BRT(42),
	BRT(44), BRT(46), BRT(48), BRT(50), BRT(52), BRT(54), BRT(56), BRT(57), BRT(59), BRT(61),
	BRT(63), BRT(65), BRT(67), BRT(69), BRT(70), BRT(72), BRT(74), BRT(76), BRT(78), BRT(80),
	BRT(82), BRT(84), BRT(86), BRT(88), BRT(90), BRT(92), BRT(94), BRT(96), BRT(98), BRT(100),
	BRT(102), BRT(104), BRT(106), BRT(108), BRT(110), BRT(112), BRT(114), BRT(116), BRT(118), BRT(120),
	BRT(122), BRT(124), BRT(126), BRT(128), BRT(135), BRT(142), BRT(149), BRT(157), BRT(165), BRT(174),
	BRT(183), BRT(193), BRT(201), BRT(210), BRT(219), BRT(223), BRT(227), BRT(230), BRT(234), BRT(238),
	BRT(242), BRT(246), BRT(250), BRT(255),
	/* HBM */
	BRT(269), BRT(284), BRT(298), BRT(313), BRT(327), BRT(342), BRT(356), BRT(370), BRT(385), BRT(399),
	BRT(414), BRT(428), BRT(442), BRT(457), BRT(471), BRT(486),
};

static unsigned int beyond0_a3_s0_lum_tbl[S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE] = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	12, 13, 14, 15, 16, 17, 19, 20, 21, 22,
	24, 25, 27, 29, 30, 32, 34, 37, 39, 41,
	44, 47, 50, 53, 56, 60, 64, 68, 72, 77,
	82, 87, 93, 98, 105, 111, 119, 126, 134, 143,
	152, 162, 172, 183, 195, 207, 220, 234, 249, 265,
	282, 300, 316, 333, 350, 357, 365, 372, 380, 387,
	395, 403, 412, 420,
	/* HBM */
	444, 468, 491, 515, 539, 563, 586, 610, 634, 658,
	681, 705, 729, 753, 776, 800,
};

/* input step excel from d.lab */
static unsigned int beyond0_a3_s0_step_cnt_tbl[S6E3FA7_BEYOND_TOTAL_NR_LUMINANCE] = {
	1, 7, 7, 7, 7, 7, 6, 6, 6, 7,
	6, 7, 7, 7, 6, 7, 13, 7, 7, 7,
	13, 7, 7, 5, 3, 5, 5, 7, 4, 5,
	6, 6, 6, 6, 5, 8, 6, 7, 6, 8,
	8, 7, 8, 7, 10, 7, 10, 8, 10, 10,
	9, 10, 10, 11, 7, 7, 7, 8, 8, 9,
	9, 10, 8, 9, 9, 4, 4, 3, 4, 4,
	4, 4, 4, 5,
	/* HBM */
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5,
};


struct brightness_table s6e3fa7_beyond0_a3_s0_panel_brightness_table = {
	.brt = beyond0_a3_s0_brt_tbl,
	.sz_brt = ARRAY_SIZE(beyond0_a3_s0_brt_tbl),
	.lum = beyond0_a3_s0_lum_tbl,
	.sz_lum = ARRAY_SIZE(beyond0_a3_s0_lum_tbl),
	.sz_ui_lum = S6E3FA7_NR_LUMINANCE,
	.sz_hbm_lum = S6E3FA7_BEYOND_NR_HBM_LUMINANCE,
	.sz_ext_hbm_lum = 0,
	.brt_to_step = NULL,
	.sz_brt_to_step = 0,
	.step_cnt = beyond0_a3_s0_step_cnt_tbl,
	.sz_step_cnt = ARRAY_SIZE(beyond0_a3_s0_step_cnt_tbl),
	.vtotal = 2304,
};

static struct panel_dimming_info s6e3fa7_beyond0_a3_s0_preliminary_panel_dimming_info = {
	.name = "s6e3fa7_beyond0_a3_s0_preliminary",
	.dim_init_info = {
		.name = "s6e3fa7_beyond0_a3_s0_preliminary",
		.nr_tp = S6E3FA7_NR_TP,
		.tp = s6e3fa7_beyond0_tp,
		.nr_luminance = S6E3FA7_NR_LUMINANCE,
		.vregout = 109051904LL, /* 6.5 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3FA7_BEYOND_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = beyond0_a3_s0_dimming_lut,
	},
	.target_luminance = S6E3FA7_BEYOND_TARGET_LUMINANCE,
	.nr_luminance = S6E3FA7_NR_LUMINANCE,
	.hbm_target_luminance = S6E3FA7_BEYOND_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3FA7_BEYOND_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3fa7_beyond0_a3_s0_panel_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyond0_a3_s0_dimming_param_maptbl,
	.dim_flash_on = false,	/* read dim flash when probe or not */
	.dim_flash_gamma_offset = NULL,
	.irc_info = &s6e3fa7_beyond_irc,
};

static struct panel_dimming_info s6e3fa7_beyond0_a3_s0_panel_dimming_info = {
	.name = "s6e3fa7_beyond0_a3_s0",
	.dim_init_info = {
		.name = "s6e3fa7_beyond0_a3_s0",
		.nr_tp = S6E3FA7_NR_TP,
		.tp = s6e3fa7_beyond0_tp,
		.nr_luminance = S6E3FA7_NR_LUMINANCE,
		.vregout = 109051904LL, /* 6.5 * 2^24 */
		.bitshift = 24,
		.vt_voltage = {
			0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
			138, 148, 158, 168, 178, 186,
		},
		.target_luminance = S6E3FA7_BEYOND_TARGET_LUMINANCE,
		.target_gamma = 220,
		.dim_lut = beyond0_a3_s0_dimming_lut,
	},
	.target_luminance = S6E3FA7_BEYOND_TARGET_LUMINANCE,
	.nr_luminance = S6E3FA7_NR_LUMINANCE,
	.hbm_target_luminance = S6E3FA7_BEYOND_TARGET_HBM_LUMINANCE,
	.nr_hbm_luminance = S6E3FA7_BEYOND_NR_HBM_LUMINANCE,
	.extend_hbm_target_luminance = -1,
	.nr_extend_hbm_luminance = 0,
	.brt_tbl = &s6e3fa7_beyond0_a3_s0_panel_brightness_table,
	/* dimming parameters */
	.dimming_maptbl = beyond0_a3_s0_dimming_param_maptbl,
	.dim_flash_on = true,	/* read dim flash when probe or not */
	.dim_flash_gamma_offset = beyond0_a3_s0_dim_flash_gamma_offset,
	.irc_info = &s6e3fa7_beyond_irc,
};
#endif /* __S6E3FA7_BEYOND0_A3_S0_PANEL_DIMMING_H___ */
