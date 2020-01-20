/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Register interface file for Exynos Scaler driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "scaler.h"
#include "scaler-regs.h"

#define COEF(val_l, val_h) ((((val_h) & 0x7FF) << 16) | ((val_l) & 0x7FF))
extern int sc_set_blur;

/* Scaling coefficient value for 10bit supporting */
static const __u32 sc_coef_8t_org[7][16][4] = {
	{	/* 8:8 or Zoom-in */
		{COEF( 0,   0), COEF(  0,   0), COEF(512,   0), COEF(  0,   0)},
		{COEF(-1,   2), COEF( -9,  30), COEF(509, -25), COEF(  8,  -2)},
		{COEF(-1,   5), COEF(-19,  64), COEF(499, -46), COEF( 14,  -4)},
		{COEF(-2,   8), COEF(-30, 101), COEF(482, -62), COEF( 20,  -5)},
		{COEF(-3,  12), COEF(-41, 142), COEF(458, -73), COEF( 23,  -6)},
		{COEF(-3,  15), COEF(-53, 185), COEF(429, -80), COEF( 25,  -6)},
		{COEF(-4,  19), COEF(-63, 228), COEF(395, -83), COEF( 26,  -6)},
		{COEF(-5,  21), COEF(-71, 273), COEF(357, -82), COEF( 25,  -6)},
		{COEF(-5,  23), COEF(-78, 316), COEF(316, -78), COEF( 23,  -5)},
	}, {	/* 8:7 Zoom-out */
		{COEF( 0,  12), COEF(-32,  52), COEF(444,  56), COEF(-32,  12)},
		{COEF(-3,  13), COEF(-39,  82), COEF(445,  29), COEF(-24,   9)},
		{COEF(-3,  14), COEF(-46, 112), COEF(438,   6), COEF(-16,   7)},
		{COEF(-3,  15), COEF(-52, 144), COEF(426, -14), COEF( -9,   5)},
		{COEF(-3,  16), COEF(-58, 177), COEF(410, -30), COEF( -3,   3)},
		{COEF(-3,  16), COEF(-63, 211), COEF(390, -43), COEF(  2,   2)},
		{COEF(-2,  16), COEF(-66, 244), COEF(365, -53), COEF(  7,   1)},
		{COEF(-2,  15), COEF(-66, 277), COEF(338, -60), COEF( 10,   0)},
		{COEF(-1,  13), COEF(-65, 309), COEF(309, -65), COEF( 13,  -1)},
	}, {	/* 8:6 or Zoom-in */
		{COEF( 0,   8), COEF(-44, 100), COEF(384, 100), COEF(-44,   8)},
		{COEF( 0,   8), COEF(-47, 123), COEF(382,  77), COEF(-40,   9)},
		{COEF( 1,   7), COEF(-49, 147), COEF(377,  57), COEF(-36,   8)},
		{COEF( 2,   5), COEF(-49, 171), COEF(369,  38), COEF(-32,   8)},
		{COEF( 2,   3), COEF(-48, 196), COEF(358,  20), COEF(-27,   8)},
		{COEF( 3,   1), COEF(-47, 221), COEF(344,   5), COEF(-22,   7)},
		{COEF( 3,  -2), COEF(-43, 245), COEF(329,  -9), COEF(-18,   7)},
		{COEF( 4,  -5), COEF(-37, 268), COEF(310, -20), COEF(-13,   5)},
		{COEF( 5,  -9), COEF(-30, 290), COEF(290, -30), COEF( -9,   5)},
	}, {	/* 8:5 Zoom-out */
		{COEF( 0,  -3), COEF(-31, 130), COEF(320, 130), COEF(-31,  -3)},
		{COEF( 3,  -6), COEF(-29, 147), COEF(319, 113), COEF(-32,  -3)},
		{COEF( 3,  -8), COEF(-26, 165), COEF(315,  97), COEF(-33,  -1)},
		{COEF( 3, -11), COEF(-22, 182), COEF(311,  81), COEF(-32,   0)},
		{COEF( 3, -13), COEF(-17, 199), COEF(304,  66), COEF(-31,   1)},
		{COEF( 3, -16), COEF(-11, 216), COEF(296,  52), COEF(-30,   2)},
		{COEF( 3, -18), COEF( -3, 232), COEF(286,  38), COEF(-28,   2)},
		{COEF( 3, -21), COEF(  5, 247), COEF(274,  26), COEF(-25,   3)},
		{COEF( 3, -23), COEF( 15, 261), COEF(261,  15), COEF(-23,   3)},
	}, {	/* 8:4 Zoom-out */
		{COEF( 0, -12), COEF(  0, 140), COEF(255, 140), COEF(  0, -11)},
		{COEF( 0, -13), COEF(  5, 151), COEF(254, 129), COEF( -4, -10)},
		{COEF(-1, -14), COEF( 10, 163), COEF(253, 117), COEF( -7,  -9)},
		{COEF(-1, -15), COEF( 16, 174), COEF(250, 106), COEF(-10,  -8)},
		{COEF(-1, -16), COEF( 22, 185), COEF(246,  95), COEF(-12,  -7)},
		{COEF(-2, -16), COEF( 29, 195), COEF(241,  85), COEF(-14,  -6)},
		{COEF(-2, -17), COEF( 37, 204), COEF(236,  74), COEF(-15,  -5)},
		{COEF(-3, -17), COEF( 46, 214), COEF(229,  64), COEF(-16,  -5)},
		{COEF(-4, -17), COEF( 55, 222), COEF(222,  55), COEF(-17,  -4)},
	}, {	/* 8:3 or Zoom-in */
		{COEF( 0,  -6), COEF( 31, 133), COEF(195, 133), COEF( 31,  -5)},
		{COEF(-3,  -4), COEF( 37, 139), COEF(195, 126), COEF( 27,  -5)},
		{COEF(-3,  -3), COEF( 41, 146), COEF(194, 119), COEF( 23,  -5)},
		{COEF(-4,  -2), COEF( 47, 152), COEF(193, 112), COEF( 19,  -5)},
		{COEF(-4,  -2), COEF( 53, 158), COEF(191, 105), COEF( 16,  -5)},
		{COEF(-4,   0), COEF( 59, 163), COEF(189,  98), COEF( 12,  -5)},
		{COEF(-4,   1), COEF( 65, 169), COEF(185,  91), COEF( 10,  -5)},
		{COEF(-4,   3), COEF( 71, 174), COEF(182,  84), COEF(  7,  -5)},
		{COEF(-5,   5), COEF( 78, 178), COEF(178,  78), COEF(  5,  -5)},
	}, {	/* 8:2 Zoom-out */
		{COEF( 0,  10), COEF( 52, 118), COEF(152, 118), COEF( 52,  10)},
		{COEF( 0,  11), COEF( 56, 122), COEF(152, 114), COEF( 48,   9)},
		{COEF( 1,  13), COEF( 60, 125), COEF(151, 110), COEF( 45,   7)},
		{COEF( 1,  15), COEF( 64, 129), COEF(150, 106), COEF( 41,   6)},
		{COEF( 1,  17), COEF( 68, 132), COEF(149, 102), COEF( 38,   5)},
		{COEF( 1,  19), COEF( 72, 135), COEF(148,  98), COEF( 35,   4)},
		{COEF( 1,  21), COEF( 77, 138), COEF(146,  94), COEF( 31,   4)},
		{COEF( 2,  23), COEF( 81, 140), COEF(145,  89), COEF( 29,   3)},
		{COEF( 2,  26), COEF( 85, 143), COEF(143,  85), COEF( 26,   2)},
	}
};

static const __u32 sc_coef_4t_org[7][16][2] = {
	{ /* 8:8 or Zoom-in */
		{COEF(  0,   0), COEF(512,   0)},
		{COEF( -1,  20), COEF(508, -15)},
		{COEF( -3,  45), COEF(495, -25)},
		{COEF( -5,  75), COEF(473, -31)},
		{COEF( -8, 110), COEF(443, -33)},
		{COEF(-11, 148), COEF(408, -33)},
		{COEF(-14, 190), COEF(367, -31)},
		{COEF(-19, 234), COEF(324, -27)},
		{COEF(-23, 279), COEF(279, -23)},
	}, { /* 8:7 Zoom-out */
		{COEF(  0,  32), COEF(448,  32)},
		{COEF( -6,  55), COEF(446,  17)},
		{COEF( -7,  79), COEF(437,   3)},
		{COEF( -9, 107), COEF(421,  -7)},
		{COEF(-11, 138), COEF(399, -14)},
		{COEF(-13, 170), COEF(373, -18)},
		{COEF(-15, 204), COEF(343, -20)},
		{COEF(-18, 240), COEF(310, -20)},
		{COEF(-19, 275), COEF(275, -19)},
	}, { /* 8:6 Zoom-out */
		{COEF(  0,  61), COEF(390,  61)},
		{COEF( -7,  83), COEF(390,  46)},
		{COEF( -8, 106), COEF(383,  31)},
		{COEF( -8, 130), COEF(371,  19)},
		{COEF( -9, 156), COEF(356,   9)},
		{COEF(-10, 183), COEF(337,   2)},
		{COEF(-10, 210), COEF(315,  -3)},
		{COEF(-10, 238), COEF(291,  -7)},
		{COEF( -9, 265), COEF(265,  -9)},
	}, { /* 8:5 Zoom-out */
		{COEF(  0,  86), COEF(341,  85)},
		{COEF( -5, 105), COEF(341,  71)},
		{COEF( -4, 124), COEF(336,  56)},
		{COEF( -4, 145), COEF(328,  43)},
		{COEF( -3, 166), COEF(317,  32)},
		{COEF( -2, 187), COEF(304,  23)},
		{COEF( -1, 209), COEF(288,  16)},
		{COEF(  1, 231), COEF(271,   9)},
		{COEF(  5, 251), COEF(251,   5)},
	}, { /* 8:4 Zoom-out */
		{COEF(  0, 104), COEF(304, 104)},
		{COEF(  1, 120), COEF(302,  89)},
		{COEF(  2, 136), COEF(298,  76)},
		{COEF(  3, 153), COEF(293,  63)},
		{COEF(  5, 170), COEF(285,  52)},
		{COEF(  7, 188), COEF(275,  42)},
		{COEF( 10, 205), COEF(264,  33)},
		{COEF( 14, 221), COEF(251,  26)},
		{COEF( 20, 236), COEF(236,  20)},
	}, { /* 8:3 Zoom-out */
		{COEF(  0, 118), COEF(276, 118)},
		{COEF(  7, 129), COEF(273, 103)},
		{COEF(  9, 143), COEF(270,  90)},
		{COEF( 11, 157), COEF(266,  78)},
		{COEF( 14, 171), COEF(260,  67)},
		{COEF( 17, 185), COEF(253,  57)},
		{COEF( 21, 199), COEF(244,  48)},
		{COEF( 27, 211), COEF(234,  40)},
		{COEF( 33, 223), COEF(223,  33)},
	}, { /* 8:2 Zoom-out */
		{COEF(  0, 127), COEF(258, 127)},
		{COEF( 14, 135), COEF(252, 111)},
		{COEF( 15, 147), COEF(250, 100)},
		{COEF( 18, 159), COEF(247,  88)},
		{COEF( 21, 171), COEF(242,  78)},
		{COEF( 25, 182), COEF(237,  68)},
		{COEF( 30, 193), COEF(230,  59)},
		{COEF( 36, 204), COEF(222,  50)},
		{COEF( 43, 213), COEF(213,  43)},
	},
};

static const __u32 sc_coef_8t_blur1[7][16][4] = {
	{ /* 8:8  or zoom-in */
		{COEF( 0, -12), COEF(  0, 140), COEF(256, 140), COEF(  0,-12)},
		{COEF( 0, -12), COEF(  4, 152), COEF(256, 124), COEF( -4, -8)},
		{COEF( 0, -12), COEF(  8, 164), COEF(252, 116), COEF( -8, -8)},
		{COEF( 0, -16), COEF( 16, 176), COEF(248, 104), COEF( -8, -8)},
		{COEF( 0, -16), COEF( 32, 184), COEF(248,  92), COEF(-12, -8)},
		{COEF( 0, -16), COEF( 28, 196), COEF(240,  80), COEF(-12, -4)},
		{COEF(-4, -16), COEF( 36, 208), COEF(236,  72), COEF(-16, -4)},
		{COEF(-4, -16), COEF( 48, 212), COEF(228,  64), COEF(-16, -4)},
		{COEF(-4, -16), COEF( 56, 224), COEF(220,  52), COEF(-16, -4)},
		{COEF(-4, -16), COEF( 48, 212), COEF(228,  64), COEF(-16, -4)},
		{COEF(-4, -16), COEF( 36, 208), COEF(236,  72), COEF(-16, -4)},
		{COEF( 0, -16), COEF( 28, 196), COEF(240,  80), COEF(-12, -4)},
		{COEF( 0, -16), COEF( 24, 184), COEF(248,  92), COEF(-12, -8)},
		{COEF( 0, -16), COEF( 16, 176), COEF(248, 104), COEF( -8, -8)},
		{COEF( 0, -12), COEF(  8, 164), COEF(252, 116), COEF( -8, -8)},
		{COEF( 0, -12), COEF(  4, 152), COEF(256, 124), COEF( -4, -8)}
	}, { /* 8:7 Zoom-out */
		{COEF(-4,  -8), COEF( 16, 140), COEF(224, 136), COEF( 16, -8)},
		{COEF(-4, -12), COEF( 24, 148), COEF(224, 128), COEF( 12, -8)},
		{COEF(-4, -12), COEF( 28, 156), COEF(224, 120), COEF(  8, -8)},
		{COEF(-4, -12), COEF( 32, 164), COEF(220, 112), COEF(  4, -8)},
		{COEF(-4, -12), COEF( 40, 172), COEF(216, 104), COEF(  0, -8)},
		{COEF(-4,  -8), COEF( 48, 176), COEF(212,  92), COEF(  0, -8)},
		{COEF(-4,  -8), COEF( 56, 188), COEF(208,  84), COEF( -4, -8)},
		{COEF(-4,  -8), COEF( 60, 192), COEF(204,  76), COEF( -4, -4)},
		{COEF(-4,  -8), COEF( 68, 200), COEF(200,  68), COEF( -8, -4)},
		{COEF(-4,  -8), COEF( 60, 192), COEF(204,  76), COEF( -4, -4)},
		{COEF(-4,  -8), COEF( 56, 188), COEF(208,  84), COEF( -4, -8)},
		{COEF(-4,  -8), COEF( 48, 180), COEF(212,  92), COEF(  0, -8)},
		{COEF(-4, -12), COEF( 40, 176), COEF(216, 104), COEF(  0, -8)},
		{COEF(-4, -12), COEF( 32, 168), COEF(220, 112), COEF(  4, -8)},
		{COEF(-4, -12), COEF( 28, 156), COEF(224, 120), COEF(  8, -8)},
		{COEF(-4, -12), COEF( 24, 148), COEF(224, 128), COEF( 12, -8)}
	}, { /* 8:6 Zoom-out */
		{COEF( 0,  -4), COEF( 32, 132), COEF(196, 128), COEF( 32, -4)},
		{COEF(-4,  -4), COEF( 36, 140), COEF(196, 124), COEF( 28, -4)},
		{COEF(-4,  -4), COEF( 44, 144), COEF(196, 116), COEF( 24, -4)},
		{COEF(-4,  -4), COEF( 48, 152), COEF(192, 112), COEF( 20, -4)},
		{COEF(-4,   0), COEF( 48, 160), COEF(192, 104), COEF( 16, -4)},
		{COEF(-4,   0), COEF( 60, 164), COEF(188,  96), COEF( 12, -4)},
		{COEF(-4,   0), COEF( 68, 168), COEF(184,  88), COEF( 12, -4)},
		{COEF(-4,   4), COEF( 72, 172), COEF(180,  84), COEF(  8, -4)},
		{COEF(-4,   4), COEF( 80, 180), COEF(176,  76), COEF(  4, -4)},
		{COEF(-4,   4), COEF( 72, 172), COEF(180,  84), COEF(  8, -4)},
		{COEF(-4,   0), COEF( 68, 168), COEF(184,  88), COEF( 12, -4)},
		{COEF(-4,   0), COEF( 60, 164), COEF(188,  96), COEF( 12, -4)},
		{COEF(-4,   0), COEF( 48, 160), COEF(192, 104), COEF( 16, -4)},
		{COEF(-4,  -4), COEF( 48, 152), COEF(192, 112), COEF( 20, -4)},
		{COEF(-4,  -4), COEF( 44, 144), COEF(196, 116), COEF( 24, -4)},
		{COEF(-4,  -4), COEF( 36, 140), COEF(196, 124), COEF( 28, -4)}
	}, { /* 8:5 Zoom-out */
		{COEF( 0,   4), COEF( 44, 128), COEF(168, 120), COEF( 44,  4)},
		{COEF(-4,   4), COEF( 48, 132), COEF(172, 120), COEF( 40,  0)},
		{COEF(-4,   4), COEF( 52, 136), COEF(172, 116), COEF( 36,  0)},
		{COEF(-4,   8), COEF( 56, 140), COEF(172, 108), COEF( 32,  0)},
		{COEF(-4,   8), COEF( 64, 144), COEF(168, 104), COEF( 28,  0)},
		{COEF(-4,   8), COEF( 68, 148), COEF(168, 100), COEF( 24,  0)},
		{COEF( 0,  12), COEF( 72, 152), COEF(164,  92), COEF( 20,  0)},
		{COEF( 0,  12), COEF( 80, 156), COEF(160,  88), COEF( 16,  0)},
		{COEF( 0,  16), COEF( 84, 156), COEF(160,  80), COEF( 16,  0)},
		{COEF( 0,  12), COEF( 80, 156), COEF(160,  88), COEF( 16,  0)},
		{COEF( 0,  12), COEF( 72, 152), COEF(164,  92), COEF( 20,  0)},
		{COEF(-4,   8), COEF( 68, 148), COEF(168, 100), COEF( 24,  0)},
		{COEF(-4,   8), COEF( 64, 144), COEF(168, 104), COEF( 28,  0)},
		{COEF(-4,   8), COEF( 56, 140), COEF(172, 108), COEF( 32,  0)},
		{COEF(-4,   4), COEF( 52, 136), COEF(172, 116), COEF( 36,  0)},
		{COEF(-4,   4), COEF( 48, 132), COEF(172, 120), COEF( 40,  0)}
	}, { /* 8:4 Zoom-out */
		{COEF( 0,   8), COEF( 52, 120), COEF(156, 116), COEF( 52,  8)},
		{COEF( 0,  12), COEF( 56, 124), COEF(152, 112), COEF( 48,  8)},
		{COEF( 0,  12), COEF( 60, 128), COEF(152, 108), COEF( 44,  8)},
		{COEF( 0,  16), COEF( 64, 132), COEF(152, 104), COEF( 40,  4)},
		{COEF( 0,  16), COEF( 68, 136), COEF(148, 104), COEF( 36,  4)},
		{COEF( 0,  24), COEF( 72, 136), COEF(148,  96), COEF( 32,  4)},
		{COEF( 0,  20), COEF( 76, 140), COEF(148,  92), COEF( 32,  4)},
		{COEF( 0,  24), COEF( 84, 140), COEF(144,  88), COEF( 28,  4)},
		{COEF( 4,  24), COEF( 88, 144), COEF(140,  84), COEF( 24,  4)},
		{COEF( 0,  24), COEF( 84, 140), COEF(144,  88), COEF( 28,  4)},
		{COEF( 0,  20), COEF( 76, 140), COEF(148,  92), COEF( 32,  4)},
		{COEF( 0,  24), COEF( 72, 136), COEF(148,  96), COEF( 32,  4)},
		{COEF( 0,  16), COEF( 68, 136), COEF(148, 104), COEF( 36,  4)},
		{COEF( 0,  16), COEF( 64, 132), COEF(152, 104), COEF( 40,  4)},
		{COEF( 0,  12), COEF( 60, 128), COEF(152, 108), COEF( 44,  8)},
		{COEF( 0,  12), COEF( 56, 124), COEF(152, 112), COEF( 48,  8)}
	}, { /* 8:3 Zoom-out */
		{COEF( 0,  16), COEF( 60, 112), COEF(140, 112), COEF( 56, 16)},
		{COEF( 4,  20), COEF( 64, 116), COEF(136, 108), COEF( 52, 12)},
		{COEF( 4,  24), COEF( 64, 120), COEF(136, 104), COEF( 48, 12)},
		{COEF( 4,  24), COEF( 68, 120), COEF(136, 100), COEF( 48, 12)},
		{COEF( 4,  24), COEF( 72, 124), COEF(136, 100), COEF( 44,  8)},
		{COEF( 4,  28), COEF( 76, 124), COEF(136,  96), COEF( 40,  8)},
		{COEF( 8,  28), COEF( 80, 128), COEF(132,  92), COEF( 36,  8)},
		{COEF( 8,  32), COEF( 84, 128), COEF(132,  88), COEF( 32,  8)},
		{COEF( 8,  32), COEF( 88, 128), COEF(132,  88), COEF( 32,  8)},
		{COEF( 8,  32), COEF( 84, 128), COEF(132,  92), COEF( 32,  8)},
		{COEF( 8,  28), COEF( 80, 128), COEF(132,  96), COEF( 36,  8)},
		{COEF( 4,  28), COEF( 76, 124), COEF(136,  72), COEF( 40,  8)},
		{COEF( 4,  24), COEF( 72, 124), COEF(136, 100), COEF( 44,  8)},
		{COEF( 4,  24), COEF( 68, 120), COEF(136, 100), COEF( 48, 12)},
		{COEF( 4,  24), COEF( 64, 120), COEF(136, 104), COEF( 48, 12)},
		{COEF( 4,  20), COEF( 64, 116), COEF(136, 108), COEF( 52, 12)}
	}, { /* 8:2 Zoom-out */
		{COEF( 0,  20), COEF( 64, 108), COEF(132, 108), COEF( 60, 20)},
		{COEF( 8,  24), COEF( 64, 108), COEF(128, 104), COEF( 56, 20)},
		{COEF( 8,  24), COEF( 68, 112), COEF(128, 100), COEF( 52, 20)},
		{COEF( 8,  24), COEF( 72, 116), COEF(128, 100), COEF( 48, 16)},
		{COEF( 8,  28), COEF( 76, 116), COEF(124,  96), COEF( 48, 16)},
		{COEF( 8,  32), COEF( 80, 120), COEF(124,  92), COEF( 44, 12)},
		{COEF( 8,  32), COEF( 80, 120), COEF(124,  92), COEF( 44, 12)},
		{COEF( 8,  36), COEF( 84, 120), COEF(124,  88), COEF( 40, 12)},
		{COEF(12,  40), COEF( 88, 124), COEF(120,  84), COEF( 36,  8)},
		{COEF( 8,  36), COEF( 84, 120), COEF(124,  88), COEF( 40, 12)},
		{COEF( 8,  32), COEF( 80, 120), COEF(124,  92), COEF( 44, 12)},
		{COEF( 8,  32), COEF( 80, 120), COEF(124,  92), COEF( 44, 12)},
		{COEF( 8,  28), COEF( 76, 116), COEF(124,  96), COEF( 48, 16)},
		{COEF( 8,  24), COEF( 72, 116), COEF(128, 100), COEF( 48, 16)},
		{COEF( 8,  24), COEF( 68, 112), COEF(128, 100), COEF( 52, 20)},
		{COEF( 8,  24), COEF( 64, 108), COEF(128, 100), COEF( 56, 20)}
	}
};

static const __u32 sc_coef_4t_blur1[7][16][2] = {
	{ /* 8:8  or zoom-in */
		{COEF(  0, 108), COEF(304, 100)},
		{COEF(  0, 124), COEF(304,  84)},
		{COEF(  0, 140), COEF(300,  72)},
		{COEF(  4, 156), COEF(292,  60)},
		{COEF(  4, 176), COEF(284,  48)},
		{COEF(  8, 192), COEF(276,  36)},
		{COEF( 12, 212), COEF(260,  28)},
		{COEF( 16, 228), COEF(244,  24)},
		{COEF( 20, 244), COEF(232,  16)},
		{COEF( 24, 244), COEF(228,  16)},
		{COEF( 28, 260), COEF(212,  12)},
		{COEF( 36, 276), COEF(192,   8)},
		{COEF( 48, 284), COEF(176,   4)},
		{COEF( 60, 292), COEF(156,   4)},
		{COEF( 72, 300), COEF(140,   0)},
		{COEF( 84, 304), COEF(124,   0)}
	}, { /* 8:7 Zoom-out  */
		{COEF(  0, 116), COEF(292, 104)},
		{COEF(  4, 128), COEF(288,  92)},
		{COEF(  4, 144), COEF(288,  76)},
		{COEF(  8, 160), COEF(280,  64)},
		{COEF(  8, 176), COEF(272,  56)},
		{COEF( 12, 192), COEF(264,  44)},
		{COEF( 16, 208), COEF(252,  36)},
		{COEF( 20, 224), COEF(240,  28)},
		{COEF( 28, 232), COEF(228,  24)},
		{COEF( 28, 240), COEF(224,  20)},
		{COEF( 36, 252), COEF(208,  16)},
		{COEF( 44, 264), COEF(192,  12)},
		{COEF( 56, 272), COEF(176,   8)},
		{COEF( 64, 280), COEF(160,   8)},
		{COEF( 76, 288), COEF(144,   4)},
		{COEF( 92, 288), COEF(128,   4)}
	}, { /* 8:6 Zoom-out  */
		{COEF(  0, 120), COEF(276, 116)},
		{COEF(  8, 132), COEF(276,  96)},
		{COEF(  8, 148), COEF(272,  84)},
		{COEF( 12, 164), COEF(264,  72)},
		{COEF( 12, 176), COEF(260,  64)},
		{COEF( 16, 192), COEF(252,  52)},
		{COEF( 20, 204), COEF(244,  44)},
		{COEF( 28, 216), COEF(232,  36)},
		{COEF( 32, 232), COEF(220,  28)},
		{COEF( 36, 232), COEF(216,  28)},
		{COEF( 44, 244), COEF(204,  20)},
		{COEF( 52, 252), COEF(192,  16)},
		{COEF( 64, 260), COEF(176,  12)},
		{COEF( 72, 264), COEF(164,  12)},
		{COEF( 84, 272), COEF(148,   8)},
		{COEF( 96, 276), COEF(132,   8)}
	}, { /* 8:5 Zoom-out  */
		{COEF(  0, 128), COEF(264, 120)},
		{COEF( 12, 136), COEF(264, 100)},
		{COEF( 12, 152), COEF(260,  88)},
		{COEF( 16, 164), COEF(256,  76)},
		{COEF( 16, 176), COEF(252,  68)},
		{COEF( 20, 192), COEF(244,  56)},
		{COEF( 28, 200), COEF(236,  48)},
		{COEF( 32, 216), COEF(224,  50)},
		{COEF( 40, 224), COEF(216,  32)},
		{COEF( 40, 224), COEF(216,  32)},
		{COEF( 48, 236), COEF(200,  28)},
		{COEF( 56, 244), COEF(192,  20)},
		{COEF( 68, 252), COEF(176,  16)},
		{COEF( 76, 256), COEF(164,  16)},
		{COEF( 88, 260), COEF(152,  12)},
		{COEF(100, 264), COEF(136,  12)}
	}, { /* 8:4 Zoom-out  */
		{COEF(  0, 136), COEF(256, 120)},
		{COEF( 12, 140), COEF(256, 104)},
		{COEF( 16, 152), COEF(252,  92)},
		{COEF( 16, 164), COEF(248,  84)},
		{COEF( 20, 176), COEF(244,  72)},
		{COEF( 24, 188), COEF(240,  60)},
		{COEF( 32, 200), COEF(228,  52)},
		{COEF( 36, 212), COEF(220,  44)},
		{COEF( 44, 220), COEF(212,  36)},
		{COEF( 52, 200), COEF(228,  32)},
		{COEF( 52, 228), COEF(200,  32)},
		{COEF( 60, 240), COEF(188,  24)},
		{COEF( 72, 244), COEF(176,  20)},
		{COEF( 84, 248), COEF(164,  16)},
		{COEF( 92, 252), COEF(152,  16)},
		{COEF(104, 256), COEF(140,  12)}
	}, { /* 8:3 Zoom-out */
		{COEF(  0, 128), COEF(248, 128)},
		{COEF( 16, 140), COEF(248, 108)},
		{COEF( 16, 152), COEF(244, 100)},
		{COEF( 20, 164), COEF(240,  88)},
		{COEF( 24, 176), COEF(236,  76)},
		{COEF( 28, 188), COEF(232,  64)},
		{COEF( 36, 196), COEF(224,  56)},
		{COEF( 40, 208), COEF(216,  48)},
		{COEF( 48, 216), COEF(208,  40)},
		{COEF( 48, 216), COEF(208,  40)},
		{COEF( 56, 224), COEF(196,  36)},
		{COEF( 64, 232), COEF(188,  28)},
		{COEF( 76, 236), COEF(176,  24)},
		{COEF( 88, 240), COEF(164,  20)},
		{COEF(100, 244), COEF(152,  16)},
		{COEF(108, 248), COEF(140,  16)}
	}, { /* 8:2 Zoom-out  */
		{COEF(  8, 132), COEF(244, 128)},
		{COEF( 20, 144), COEF(244, 104)},
		{COEF( 20, 152), COEF(240, 100)},
		{COEF( 24, 164), COEF(236,  88)},
		{COEF( 28, 176), COEF(232,  76)},
		{COEF( 32, 184), COEF(228,  68)},
		{COEF( 36, 196), COEF(220,  60)},
		{COEF( 44, 204), COEF(212,  52)},
		{COEF( 52, 212), COEF(204,  44)},
		{COEF( 52, 212), COEF(204,  44)},
		{COEF( 60, 220), COEF(196,  36)},
		{COEF( 68, 228), COEF(184,  32)},
		{COEF( 76, 232), COEF(176,  28)},
		{COEF( 88, 236), COEF(164,  24)},
		{COEF(100, 240), COEF(152,  20)},
		{COEF(104, 244), COEF(144,  20)}
	},
};

/* CSC(Color Space Conversion) coefficient value */
static struct sc_csc_tab sc_no_csc = {
	{ 0x200, 0x000, 0x000, 0x000, 0x200, 0x000, 0x000, 0x000, 0x200 },
};

static struct sc_csc_tab sc_y2r = {
	/* REC.601 Narrow */
	{ 0x0254, 0x0000, 0x0331, 0x0254, 0xFF37, 0xFE60, 0x0254, 0x0409, 0x0000 },
	/* REC.601 Wide */
	{ 0x0200, 0x0000, 0x02BE, 0x0200, 0xFF54, 0xFE9B, 0x0200, 0x0377, 0x0000 },
	/* REC.709 Narrow */
	{ 0x0254, 0x0000, 0x0396, 0x0254, 0xFF93, 0xFEEF, 0x0254, 0x043A, 0x0000 },
	/* REC.709 Wide */
	{ 0x0200, 0x0000, 0x0314, 0x0200, 0xFFA2, 0xFF16, 0x0200, 0x03A1, 0x0000 },
	/* BT.2020 Narrow */
	{ 0x0254, 0x0000, 0x035B, 0x0254, 0xFFA0, 0xFEB3, 0x0254, 0x0449, 0x0000 },
	/* BT.2020 Wide */
	{ 0x0200, 0x0000, 0x02E2, 0x0200, 0xFFAE, 0xFEE2, 0x0200, 0x03AE, 0x0000 },
};

static struct sc_csc_tab sc_r2y = {
	/* REC.601 Narrow */
	{ 0x0083, 0x0102, 0x0032, 0xFFB4, 0xFF6B, 0x00E1, 0x00E1, 0xFF44, 0xFFDB },
	/* REC.601 Wide  */
	{ 0x0099, 0x012D, 0x003A, 0xFFA8, 0xFF53, 0x0106, 0x0106, 0xFF25, 0xFFD5 },
	/* REC.709 Narrow */
	{ 0x005D, 0x013A, 0x0020, 0xFFCC, 0xFF53, 0x00E1, 0x00E1, 0xFF34, 0xFFEB },
	/* REC.709 Wide */
	{ 0x006D, 0x016E, 0x0025, 0xFFC4, 0xFF36, 0x0106, 0x0106, 0xFF12, 0xFFE8 },
	/* BT.2020 Narrow */
	{ 0x0074, 0x012A, 0x001A, 0xFFC1, 0xFF5E, 0x00E1, 0x00E1, 0xFF31, 0xFFEE },
	/* BT.2020 Wide */
	{ 0x0087, 0x015B, 0x001E, 0xFFB7, 0xFF43, 0x0106, 0x0106, 0xFF0F, 0xFFEB },
};

static struct sc_csc_tab *sc_csc_list[] = {
	[0] = &sc_no_csc,
	[1] = &sc_y2r,
	[2] = &sc_r2y,
};

static struct sc_bl_op_val sc_bl_op_tbl[] = {
	/* Sc,	 Sa,	Dc,	Da */
	{ZERO,	 ZERO,	ZERO,	ZERO},		/* CLEAR */
	{ ONE,	 ONE,	ZERO,	ZERO},		/* SRC */
	{ZERO,	 ZERO,	ONE,	ONE},		/* DST */
	{ ONE,	 ONE,	INV_SA,	INV_SA},	/* SRC_OVER */
	{INV_DA, ONE,	ONE,	INV_SA},	/* DST_OVER */
	{DST_A,	 DST_A,	ZERO,	ZERO},		/* SRC_IN */
	{ZERO,	 ZERO,	SRC_A,	SRC_A},		/* DST_IN */
	{INV_DA, INV_DA, ZERO,	ZERO},		/* SRC_OUT */
	{ZERO,	 ZERO,	INV_SA,	INV_SA},	/* DST_OUT */
	{DST_A,	 ZERO,	INV_SA,	ONE},		/* SRC_ATOP */
	{INV_DA, ONE,	SRC_A,	ZERO},		/* DST_ATOP */
	{INV_DA, ONE,	INV_SA,	ONE},		/* XOR: need to WA */
	{INV_DA, ONE,	INV_SA,	INV_SA},	/* DARKEN */
	{INV_DA, ONE,	INV_SA,	INV_SA},	/* LIGHTEN */
	{INV_DA, ONE,	INV_SA,	INV_SA},	/* MULTIPLY */
	{ONE,	 ONE,	INV_SC,	INV_SA},	/* SCREEN */
	{ONE,	 ONE,	ONE,	ONE},		/* ADD */
};

int sc_hwset_src_image_format(struct sc_dev *sc, const struct sc_fmt *fmt)
{
	writel(fmt->cfg_val, sc->regs + SCALER_SRC_CFG);
	return 0;
}

int sc_hwset_dst_image_format(struct sc_dev *sc, const struct sc_fmt *fmt)
{
	writel(fmt->cfg_val, sc->regs + SCALER_DST_CFG);

	/*
	 * When output format is RGB,
	 * CSC_Y_OFFSET_DST_EN should be 0
	 * to avoid color distortion
	 */
	if (fmt->is_rgb) {
		writel(readl(sc->regs + SCALER_CFG) &
					~SCALER_CFG_CSC_Y_OFFSET_DST,
				sc->regs + SCALER_CFG);
	}

	return 0;
}

void sc_hwset_pre_multi_format(struct sc_dev *sc, bool src, bool dst)
{
	unsigned long cfg = readl(sc->regs + SCALER_SRC_CFG);

	if (sc->version == SCALER_VERSION(4, 0, 1)) {
		if (src != dst)
			dev_err(sc->dev,
			"pre-multi fmt should be same between src and dst\n");
		return;
	}

	if (src && ((cfg & SCALER_CFG_FMT_MASK) == SCALER_CFG_FMT_ARGB8888)) {
		cfg &= ~SCALER_CFG_FMT_MASK;
		cfg |= SCALER_CFG_FMT_P_ARGB8888;
		writel(cfg, sc->regs + SCALER_SRC_CFG);
	}

	cfg = readl(sc->regs + SCALER_DST_CFG);
	if (dst && ((cfg & SCALER_CFG_FMT_MASK) == SCALER_CFG_FMT_ARGB8888)) {
		cfg &= ~SCALER_CFG_FMT_MASK;
		cfg |= SCALER_CFG_FMT_P_ARGB8888;
		writel(cfg, sc->regs + SCALER_DST_CFG);
	}
}

void get_blend_value(unsigned int *cfg, u32 val, bool pre_multi)
{
	unsigned int tmp;

	*cfg &= ~(SCALER_SEL_INV_MASK | SCALER_SEL_MASK |
			SCALER_OP_SEL_INV_MASK | SCALER_OP_SEL_MASK);

	if (val == 0xff) {
		*cfg |= (1 << SCALER_SEL_INV_SHIFT);
	} else {
		if (pre_multi)
			*cfg |= (1 << SCALER_SEL_SHIFT);
		else
			*cfg |= (2 << SCALER_SEL_SHIFT);
		tmp = val & 0xf;
		*cfg |= (tmp << SCALER_OP_SEL_SHIFT);
	}

	if (val >= BL_INV_BIT_OFFSET)
		*cfg |= (1 << SCALER_OP_SEL_INV_SHIFT);
}

void sc_hwset_blend(struct sc_dev *sc, enum sc_blend_op bl_op, bool pre_multi,
		unsigned char g_alpha)
{
	unsigned int cfg = readl(sc->regs + SCALER_CFG);
	int idx = bl_op - 1;

	cfg |= SCALER_CFG_BLEND_EN;
	writel(cfg, sc->regs + SCALER_CFG);

	cfg = readl(sc->regs + SCALER_SRC_BLEND_COLOR);
	get_blend_value(&cfg, sc_bl_op_tbl[idx].src_color, pre_multi);
	if (g_alpha < 0xff)
		cfg |= (SRC_GA << SCALER_OP_SEL_SHIFT);
	writel(cfg, sc->regs + SCALER_SRC_BLEND_COLOR);
	sc_dbg("src_blend_color is 0x%x, %d\n", cfg, pre_multi);

	cfg = readl(sc->regs + SCALER_SRC_BLEND_ALPHA);
	get_blend_value(&cfg, sc_bl_op_tbl[idx].src_alpha, 1);
	if (g_alpha < 0xff)
		cfg |= (SRC_GA << SCALER_OP_SEL_SHIFT) | (g_alpha << 0);
	writel(cfg, sc->regs + SCALER_SRC_BLEND_ALPHA);
	sc_dbg("src_blend_alpha is 0x%x\n", cfg);

	cfg = readl(sc->regs + SCALER_DST_BLEND_COLOR);
	get_blend_value(&cfg, sc_bl_op_tbl[idx].dst_color, pre_multi);
	if (g_alpha < 0xff)
		cfg |= ((INV_SAGA & 0xf) << SCALER_OP_SEL_SHIFT);
	writel(cfg, sc->regs + SCALER_DST_BLEND_COLOR);
	sc_dbg("dst_blend_color is 0x%x\n", cfg);

	cfg = readl(sc->regs + SCALER_DST_BLEND_ALPHA);
	get_blend_value(&cfg, sc_bl_op_tbl[idx].dst_alpha, 1);
	if (g_alpha < 0xff)
		cfg |= ((INV_SAGA & 0xf) << SCALER_OP_SEL_SHIFT);
	writel(cfg, sc->regs + SCALER_DST_BLEND_ALPHA);
	sc_dbg("dst_blend_alpha is 0x%x\n", cfg);

	/*
	 * If dst format is non-premultiplied format
	 * and blending operation is enabled,
	 * result image should be divided by alpha value
	 * because the result is always pre-multiplied.
	 */
	if (!pre_multi) {
		cfg = readl(sc->regs + SCALER_CFG);
		cfg |= SCALER_CFG_BL_DIV_ALPHA_EN;
		writel(cfg, sc->regs + SCALER_CFG);
	}
}

void sc_hwset_color_fill(struct sc_dev *sc, unsigned int val)
{
	unsigned int cfg = readl(sc->regs + SCALER_CFG);

	cfg |= SCALER_CFG_FILL_EN;
	writel(cfg, sc->regs + SCALER_CFG);

	cfg = readl(sc->regs + SCALER_FILL_COLOR);
	cfg = val;
	writel(cfg, sc->regs + SCALER_FILL_COLOR);
	sc_dbg("color filled is 0x%08x\n", val);
}

void sc_hwset_dith(struct sc_dev *sc, unsigned int val)
{
	unsigned int cfg = readl(sc->regs + SCALER_DITH_CFG);

	cfg &= ~(SCALER_DITH_R_MASK | SCALER_DITH_G_MASK | SCALER_DITH_B_MASK);
	cfg |= val;
	writel(cfg, sc->regs + SCALER_DITH_CFG);
}

void sc_hwset_csc_coef(struct sc_dev *sc, enum sc_csc_idx idx,
			struct sc_csc *csc)
{
	unsigned int i, j, tmp;
	unsigned long cfg;
	int *csc_eq_val;

	if (idx == NO_CSC) {
		csc_eq_val = sc_csc_list[idx]->narrow_601;
	} else {
		if (csc->csc_eq == V4L2_COLORSPACE_REC709) {
			if (csc->csc_range == SC_CSC_NARROW)
				csc_eq_val = sc_csc_list[idx]->narrow_709;
			else
				csc_eq_val = sc_csc_list[idx]->wide_709;
		} else if (csc->csc_eq == V4L2_COLORSPACE_BT2020) {
			if (csc->csc_range == SC_CSC_NARROW)
				csc_eq_val = sc_csc_list[idx]->narrow_2020;
			else
				csc_eq_val = sc_csc_list[idx]->wide_2020;
		} else {
			if (csc->csc_range == SC_CSC_NARROW)
				csc_eq_val = sc_csc_list[idx]->narrow_601;
			else
				csc_eq_val = sc_csc_list[idx]->wide_601;
		}
	}

	tmp = SCALER_CSC_COEF22 - SCALER_CSC_COEF00;

	for (i = 0, j = 0; i < 9; i++, j += 4) {
		cfg = readl(sc->regs + SCALER_CSC_COEF00 + j);
		cfg &= ~SCALER_CSC_COEF_MASK;
		cfg |= csc_eq_val[i];
		writel(cfg, sc->regs + SCALER_CSC_COEF00 + j);
		sc_dbg("csc value %d - %d\n", i, csc_eq_val[i]);
	}

	/* set CSC_Y_OFFSET_EN */
	cfg = readl(sc->regs + SCALER_CFG);
	if (idx == CSC_Y2R) {
		if (csc->csc_range == SC_CSC_WIDE)
			cfg &= ~SCALER_CFG_CSC_Y_OFFSET_SRC;
		else
			cfg |= SCALER_CFG_CSC_Y_OFFSET_SRC;
	} else if (idx == CSC_R2Y) {
		if (csc->csc_range == SC_CSC_WIDE)
			cfg &= ~SCALER_CFG_CSC_Y_OFFSET_DST;
		else
			cfg |= SCALER_CFG_CSC_Y_OFFSET_DST;
	}
	writel(cfg, sc->regs + SCALER_CFG);
}

static const __u32 sc_scaling_ratio[] = {
	1048576,	/* 0: 8:8 scaing or zoom-in */
	1198372,	/* 1: 8:7 zoom-out */
	1398101,	/* 2: 8:6 zoom-out */
	1677721,	/* 3: 8:5 zoom-out */
	2097152,	/* 4: 8:4 zoom-out */
	2796202,	/* 5: 8:3 zoom-out */
	/* higher ratio -> 6: 8:2 zoom-out */
};

static unsigned int sc_get_scale_filter(unsigned int ratio)
{
	unsigned int filter;

	for (filter = 0; filter < ARRAY_SIZE(sc_scaling_ratio); filter++)
		if (ratio <= sc_scaling_ratio[filter])
			return filter;

	return filter;
}

static u32 sc_coef_adjust(u32 val)
{
	/*
	 * Truncate LSB 2 bit of two 11 bit value like below.
	 * [26:16] -> [24:16], [10:0] -> [8:0]
	 *
	 * If val has 0x01BC0038, for example, it will return 0x006F000E.
	 */
	return ((val >> 2) & 0x1ff01ff);
}

#define sc_coef_adj(x, val)	(unlikely(x) ? sc_coef_adjust(val) : val)
void sc_hwset_polyphase_hcoef(struct sc_dev *sc,
				unsigned int yratio, unsigned int cratio,
				unsigned int filter)
{
	unsigned int phase;
	unsigned int yfilter = sc_get_scale_filter(yratio);
	unsigned int cfilter = sc_get_scale_filter(cratio);
	const __u32 (*sc_coef_8t)[16][4] = sc_coef_8t_org;
	bool bit_adj = !sc->variant->pixfmt_10bit;

	if (sc_set_blur || filter == SC_FT_BLUR)
		sc_coef_8t = sc_coef_8t_blur1;

	BUG_ON(yfilter >= ARRAY_SIZE(sc_coef_8t_org));
	BUG_ON(cfilter >= ARRAY_SIZE(sc_coef_8t_org));

	BUILD_BUG_ON(ARRAY_SIZE(sc_coef_8t_org[yfilter]) < 9);
	BUILD_BUG_ON(ARRAY_SIZE(sc_coef_8t_org[cfilter]) < 9);

	for (phase = 0; phase < 9; phase++) {
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[yfilter][phase][3]),
				sc->regs + SCALER_YHCOEF + phase * 16);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[yfilter][phase][2]),
				sc->regs + SCALER_YHCOEF + phase * 16 + 4);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[yfilter][phase][1]),
				sc->regs + SCALER_YHCOEF + phase * 16 + 8);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[yfilter][phase][0]),
				sc->regs + SCALER_YHCOEF + phase * 16 + 12);
	}

	for (phase = 0; phase < 9; phase++) {
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[cfilter][phase][3]),
				sc->regs + SCALER_CHCOEF + phase * 16);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[cfilter][phase][2]),
				sc->regs + SCALER_CHCOEF + phase * 16 + 4);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[cfilter][phase][1]),
				sc->regs + SCALER_CHCOEF + phase * 16 + 8);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_8t[cfilter][phase][0]),
				sc->regs + SCALER_CHCOEF + phase * 16 + 12);
	}
}

void sc_hwset_polyphase_vcoef(struct sc_dev *sc,
				unsigned int yratio, unsigned int cratio,
				unsigned int filter)
{
	unsigned int phase;
	unsigned int yfilter = sc_get_scale_filter(yratio);
	unsigned int cfilter = sc_get_scale_filter(cratio);
	const __u32 (*sc_coef_4t)[16][2] = sc_coef_4t_org;
	bool bit_adj = !sc->variant->pixfmt_10bit;

	if (sc_set_blur || filter == SC_FT_BLUR)
		sc_coef_4t = sc_coef_4t_blur1;

	BUG_ON(yfilter >= ARRAY_SIZE(sc_coef_4t_org));
	BUG_ON(cfilter >= ARRAY_SIZE(sc_coef_4t_org));

	BUILD_BUG_ON(ARRAY_SIZE(sc_coef_4t_org[yfilter]) < 9);
	BUILD_BUG_ON(ARRAY_SIZE(sc_coef_4t_org[cfilter]) < 9);

	/* reset value of the coefficient registers are the 8:8 table */
	for (phase = 0; phase < 9; phase++) {
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_4t[yfilter][phase][1]),
				sc->regs + SCALER_YVCOEF + phase * 8);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_4t[yfilter][phase][0]),
				sc->regs + SCALER_YVCOEF + phase * 8 + 4);
	}

	for (phase = 0; phase < 9; phase++) {
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_4t[cfilter][phase][1]),
				sc->regs + SCALER_CVCOEF + phase * 8);
		__raw_writel(sc_coef_adj(bit_adj, sc_coef_4t[cfilter][phase][0]),
				sc->regs + SCALER_CVCOEF + phase * 8 + 4);
	}
}

void sc_hwset_src_imgsize(struct sc_dev *sc, struct sc_frame *frame)
{
	unsigned long cfg = 0;

	cfg &= ~(SCALER_SRC_CSPAN_MASK | SCALER_SRC_YSPAN_MASK);
	cfg |= frame->width;

	/*
	 * TODO: C width should be half of Y width
	 * but, how to get the diffferent c width from user
	 * like AYV12 format
	 */
	if (frame->sc_fmt->num_comp == 2)
		cfg |= (frame->width << frame->sc_fmt->cspan) << 16;
	if (frame->sc_fmt->num_comp == 3) {
		if (sc_fmt_is_ayv12(frame->sc_fmt->pixelformat))
			cfg |= ALIGN(frame->width >> 1, 16) << 16;
		else if (frame->sc_fmt->cspan) /* YUV444 */
			cfg |= frame->width << 16;
		else
			cfg |= (frame->width >> 1) << 16;
	}

	writel(cfg, sc->regs + SCALER_SRC_SPAN);
}

void sc_hwset_intsrc_imgsize(struct sc_dev *sc, int num_comp, __u32 width)
{
	unsigned long cfg = 0;

	cfg &= ~(SCALER_SRC_CSPAN_MASK | SCALER_SRC_YSPAN_MASK);
	cfg |= width;

	/*
	 * TODO: C width should be half of Y width
	 * but, how to get the diffferent c width from user
	 * like AYV12 format
	 */
	if (num_comp == 2)
		cfg |= width << 16;
	if (num_comp == 3)
		cfg |= (width >> 1) << 16;

	writel(cfg, sc->regs + SCALER_SRC_SPAN);
}

void sc_hwset_dst_imgsize(struct sc_dev *sc, struct sc_frame *frame)
{
	unsigned long cfg = 0;

	cfg &= ~(SCALER_DST_CSPAN_MASK | SCALER_DST_YSPAN_MASK);
	cfg |= frame->width;

	/*
	 * TODO: C width should be half of Y width
	 * but, how to get the diffferent c width from user
	 * like AYV12 format
	 */
	if (frame->sc_fmt->num_comp == 2)
		cfg |= (frame->width << frame->sc_fmt->cspan) << 16;
	if (frame->sc_fmt->num_comp == 3) {
		if (sc_fmt_is_ayv12(frame->sc_fmt->pixelformat))
			cfg |= ALIGN(frame->width >> 1, 16) << 16;
		else if (frame->sc_fmt->cspan) /* YUV444 */
			cfg |= frame->width << 16;
		else
			cfg |= (frame->width >> 1) << 16;
	}

	writel(cfg, sc->regs + SCALER_DST_SPAN);
}

int sc_calc_s10b_planesize(u32 pixelformat, u32 width, u32 height,
				u32 *ysize, u32 *csize, bool only_8bit);
static void sc_hwset_src_2bit_addr(struct sc_dev *sc, struct sc_frame *frame)
{
	u32 yaddr_2bit, caddr_2bit;
	unsigned long cfg = 0;

	BUG_ON(frame->sc_fmt->num_comp != 2);

	sc_calc_s10b_planesize(frame->sc_fmt->pixelformat,
				frame->width, frame->height,
				&yaddr_2bit, &caddr_2bit, true);
	yaddr_2bit += frame->addr.ioaddr[SC_PLANE_Y];
	caddr_2bit += frame->addr.ioaddr[SC_PLANE_CB];

	writel(yaddr_2bit, sc->regs + SCALER_SRC_2BIT_Y_BASE);
	writel(caddr_2bit, sc->regs + SCALER_SRC_2BIT_C_BASE);

	cfg &= ~(SCALER_SRC_2BIT_CSPAN_MASK | SCALER_SRC_2BIT_YSPAN_MASK);
	cfg |= ALIGN(frame->width, 16);
	cfg |= (ALIGN(frame->width, 16) << frame->sc_fmt->cspan) << 16;
	writel(cfg, sc->regs + SCALER_SRC_2BIT_SPAN);
}

void sc_hwset_src_addr(struct sc_dev *sc, struct sc_frame *frame)
{
	struct sc_addr *addr = &frame->addr;

	writel(addr->ioaddr[SC_PLANE_Y], sc->regs + SCALER_SRC_Y_BASE);
	writel(addr->ioaddr[SC_PLANE_CB], sc->regs + SCALER_SRC_CB_BASE);
	writel(addr->ioaddr[SC_PLANE_CR], sc->regs + SCALER_SRC_CR_BASE);

	if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat) &&
			sc->variant->pixfmt_10bit)
		sc_hwset_src_2bit_addr(sc, frame);
}

static void sc_hwset_dst_2bit_addr(struct sc_dev *sc, struct sc_frame *frame)
{
	u32 yaddr_2bit, caddr_2bit;
	unsigned long cfg = 0;

	BUG_ON(frame->sc_fmt->num_comp != 2);

	sc_calc_s10b_planesize(frame->sc_fmt->pixelformat,
				frame->width, frame->height,
				&yaddr_2bit, &caddr_2bit, true);
	yaddr_2bit += frame->addr.ioaddr[SC_PLANE_Y];
	caddr_2bit += frame->addr.ioaddr[SC_PLANE_CB];

	writel(yaddr_2bit, sc->regs + SCALER_DST_2BIT_Y_BASE);
	writel(caddr_2bit, sc->regs + SCALER_DST_2BIT_C_BASE);

	cfg &= ~(SCALER_DST_2BIT_CSPAN_MASK | SCALER_DST_2BIT_YSPAN_MASK);
	cfg |= ALIGN(frame->width, 16);
	cfg |= (ALIGN(frame->width, 16) << frame->sc_fmt->cspan) << 16;
	writel(cfg, sc->regs + SCALER_DST_2BIT_SPAN);
}

void sc_hwset_dst_addr(struct sc_dev *sc, struct sc_frame *frame)
{
	struct sc_addr *addr = &frame->addr;

	writel(addr->ioaddr[SC_PLANE_Y], sc->regs + SCALER_DST_Y_BASE);
	writel(addr->ioaddr[SC_PLANE_CB], sc->regs + SCALER_DST_CB_BASE);
	writel(addr->ioaddr[SC_PLANE_CR], sc->regs + SCALER_DST_CR_BASE);

	if (sc_fmt_is_s10bit_yuv(frame->sc_fmt->pixelformat) &&
			sc->variant->pixfmt_10bit)
		sc_hwset_dst_2bit_addr(sc, frame);
}

void sc_hwregs_dump(struct sc_dev *sc)
{
	dev_notice(sc->dev, "Dumping control registers...\n");
	pr_notice("------------------------------------------------\n");

	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x000, 0x044 - 0x000 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x050, 0x058 - 0x050 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x060, 0x134 - 0x060 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x140, 0x214 - 0x140 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x220, 0x240 - 0x220 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x250, 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x260, 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x278, 4, false);
	if (sc->version <= SCALER_VERSION(2, 1, 1))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x280, 0x28C - 0x280 + 4, false);
	if (sc->version >= SCALER_VERSION(5, 0, 0))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x280, 0x288 - 0x280 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x290, 0x298 - 0x290 + 4, false);
	if (sc->version <= SCALER_VERSION(2, 1, 1))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x2A8, 0x2A8 - 0x2A0 + 4, false);
	if (sc->version >= SCALER_VERSION(5, 0, 0))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x2A0, 0x2A8 - 0x2A0 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x2B0, 0x2C4 - 0x2B0 + 4, false);
	if (sc->version >= SCALER_VERSION(3, 0, 0))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x2D0, 0x2DC - 0x2D0 + 4, false);
	if (sc->version >= SCALER_VERSION(5, 0, 0))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x2E0, 0x2E8 - 0x2E0 + 4, false);

	if (sc->version >= SCALER_VERSION(5, 0, 0))
		goto end;

	/* shadow registers */
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1004, 0x1004 - 0x1004 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1010, 0x1044 - 0x1010 + 4, false);

	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1050, 0x1058 - 0x1050 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1060, 0x1134 - 0x1060 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1140, 0x1214 - 0x1140 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1220, 0x1240 - 0x1220 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1250, 4, false);
	if (sc->version <= SCALER_VERSION(2, 1, 1) ||
			sc->version <= SCALER_VERSION(4, 2, 0))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1280, 0x128C - 0x1280 + 4, false);
	print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1290, 0x1298 - 0x1290 + 4, false);
	if (sc->version >= SCALER_VERSION(3, 0, 0))
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x12D0, 0x12DC - 0x12D0 + 4, false);
	if (sc->version >= SCALER_VERSION(4, 2, 0)) {
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1300, 0x1304 - 0x1300 + 4, false);
		print_hex_dump(KERN_NOTICE, "", DUMP_PREFIX_ADDRESS, 16, 4,
			sc->regs + 0x1310, 0x1318 - 0x1310 + 4, false);
	}

end:
	pr_notice("------------------------------------------------\n");
}

/* starts from the second status which is the begining of err status */
const static char *sc_irq_err_status[] = {
	[ 0] = "illigal src color",
	[ 1] = "illigal src Y base",
	[ 2] = "illigal src Cb base",
	[ 3] = "illigal src Cr base",
	[ 4] = "illigal src Y span",
	[ 5] = "illigal src C span",
	[ 6] = "illigal src YH pos",
	[ 7] = "illigal src YV pos",
	[ 8] = "illigal src CH pos",
	[ 9] = "illigal src CV pos",
	[10] = "illigal src width",
	[11] = "illigal src height",
	[12] = "illigal dst color",
	[13] = "illigal dst Y base",
	[14] = "illigal dst Cb base",
	[15] = "illigal dst Cr base",
	[16] = "illigal dst Y span",
	[17] = "illigal dst C span",
	[18] = "illigal dst H pos",
	[19] = "illigal dst V pos",
	[20] = "illigal dst width",
	[21] = "illigal dst height",
	[23] = "illigal scaling ratio",
	[25] = "illigal pre-scaler width/height",
	[28] = "AXI Write Error Response",
	[29] = "AXI Read Error Response",
	[31] = "timeout",
};

static void sc_print_irq_err_status(struct sc_dev *sc, u32 status)
{
	unsigned int i = 0;

	status >>= 1; /* ignore the INT_STATUS_FRAME_END */
	if (status) {
		sc_hwregs_dump(sc);
		if (sc->current_ctx)
			sc_ctx_dump(sc->current_ctx);

		while (status) {
			if (status & 1)
				dev_err(sc->dev,
					"Scaler reported error %u: %s\n",
					i + 1, sc_irq_err_status[i]);
			i++;
			status >>= 1;
		}
	}
}

u32 sc_hwget_and_clear_irq_status(struct sc_dev *sc)
{
	u32 val = __raw_readl(sc->regs + SCALER_INT_STATUS);
	sc_print_irq_err_status(sc, val);
	__raw_writel(val, sc->regs + SCALER_INT_STATUS);
	return val;
}
