/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha9/s6e3ha9_beyondx_a3_s0_panel_copr.h
 *
 * Header file for COPR Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA9_BEYONDX_A3_S0_PANEL_COPR_H__
#define __S6E3HA9_BEYONDX_A3_S0_PANEL_COPR_H__

#include "../panel.h"
#include "../copr.h"
#include "s6e3ha9_beyondx_a3_s0_panel.h"

#define S6E3HA9_BEYONDX_A3_S0_COPR_EN	(1)
#define S6E3HA9_BEYONDX_A3_S0_COPR_GAMMA	(0)		/* 0 : GAMMA_1, 1 : GAMMA_2_2 */
#define S6E3HA9_BEYONDX_A3_S0_COPR_ILC	(1)
#define S6E3HA9_BEYONDX_A3_S0_COPR_MASK	(0)

#define S6E3HA9_BEYONDX_A3_S0_COPR_CLR_CNT_ON	(\
		(S6E3HA9_BEYONDX_A3_S0_COPR_MASK << 4) | \
		(1 << 3) | \
		(S6E3HA9_BEYONDX_A3_S0_COPR_ILC	<< 2) | \
		(S6E3HA9_BEYONDX_A3_S0_COPR_GAMMA << 1) | \
		(S6E3HA9_BEYONDX_A3_S0_COPR_EN << 0))

#define S6E3HA9_BEYONDX_A3_S0_COPR_CLR_CNT_OFF	(\
		(S6E3HA9_BEYONDX_A3_S0_COPR_MASK << 4) | \
		(0 << 3) | \
		(S6E3HA9_BEYONDX_A3_S0_COPR_ILC	<< 2) | \
		(S6E3HA9_BEYONDX_A3_S0_COPR_GAMMA << 1) | \
		(S6E3HA9_BEYONDX_A3_S0_COPR_EN << 0))

#define S6E3HA9_BEYONDX_A3_S0_COPR_ER		(768)
#define S6E3HA9_BEYONDX_A3_S0_COPR_EG		(768)
#define S6E3HA9_BEYONDX_A3_S0_COPR_EB		(768)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ERC		(768)
#define S6E3HA9_BEYONDX_A3_S0_COPR_EGC		(768)
#define S6E3HA9_BEYONDX_A3_S0_COPR_EBC		(768)
#define S6E3HA9_BEYONDX_A3_S0_COPR_MAX_CNT	(3)

#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_X_S	(0)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_Y_S	(0)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_X_E	(1439)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_Y_E	(83)

#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_X_S	(0)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_Y_S	(84)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_X_E	(1439)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_Y_E	(184)

#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_X_S	(0)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_Y_S	(185)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_X_E	(1439)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_Y_E	(2791)

#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_X_S	(0)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_Y_S	(2792)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_X_E	(1439)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_Y_E	(2959)

#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_S	(516)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_S	(131)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_E	(551)
#define S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_E	(162)

static struct seqinfo beyondx_a3_s0_copr_seqtbl[MAX_COPR_SEQ];
static struct pktinfo PKTINFO(beyondx_a3_s0_level2_key_enable);
static struct pktinfo PKTINFO(beyondx_a3_s0_level2_key_disable);

/* ===================================================================================== */
/* ============================== [S6E3HA9 MAPPING TABLE] ============================== */
/* ===================================================================================== */
static struct maptbl beyondx_a3_s0_copr_maptbl[] = {
	[COPR_MAPTBL] = DEFINE_0D_MAPTBL(beyondx_a3_s0_copr_table, init_common_table, NULL, copy_copr_maptbl),
};

/* ===================================================================================== */
/* ============================== [S6E3HA9 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 BEYONDX_A3_S0_COPR[] = {
	0xE1,
	0x05, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x05, 0x9F, 0x00, 0x53,
	0x00, 0x00, 0x00, 0x54, 0x05, 0x9F, 0x00, 0xB8, 0x00, 0x00,
	0x00, 0xB9, 0x05, 0x9F, 0x0A, 0xE7, 0x00, 0x00, 0x0A, 0xE8,
	0x05, 0x9F, 0x0B, 0x8F, 0x02, 0x04, 0x00, 0x83, 0x02, 0x27,
	0x00, 0xA2, 0x02, 0x04, 0x00, 0x83, 0x02, 0x27, 0x00, 0xA2
};

static u8 BEYONDX_A3_S0_COPR_CLR_CNT_ON[] = { 0xE1, S6E3HA9_BEYONDX_A3_S0_COPR_CLR_CNT_ON };
static u8 BEYONDX_A3_S0_COPR_CLR_CNT_OFF[] = { 0xE1, S6E3HA9_BEYONDX_A3_S0_COPR_CLR_CNT_OFF };

static DEFINE_PKTUI(beyondx_a3_s0_copr, &beyondx_a3_s0_copr_maptbl[COPR_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(beyondx_a3_s0_copr, DSI_PKT_TYPE_WR, BEYONDX_A3_S0_COPR, 0);

static DEFINE_STATIC_PACKET(beyondx_a3_s0_copr_clr_cnt_on, DSI_PKT_TYPE_WR, BEYONDX_A3_S0_COPR_CLR_CNT_ON, 0);
static DEFINE_STATIC_PACKET(beyondx_a3_s0_copr_clr_cnt_off, DSI_PKT_TYPE_WR, BEYONDX_A3_S0_COPR_CLR_CNT_OFF, 0);

static void *beyondx_a3_s0_set_copr_cmdtbl[] = {
	&PKTINFO(beyondx_a3_s0_level2_key_enable),
	&PKTINFO(beyondx_a3_s0_copr),
	&PKTINFO(beyondx_a3_s0_level2_key_disable),
};

static void *beyondx_a3_s0_clr_copr_cnt_on_cmdtbl[] = {
	&PKTINFO(beyondx_a3_s0_level2_key_enable),
	&PKTINFO(beyondx_a3_s0_copr_clr_cnt_on),
	&PKTINFO(beyondx_a3_s0_level2_key_disable),
};

static void *beyondx_a3_s0_clr_copr_cnt_off_cmdtbl[] = {
	&PKTINFO(beyondx_a3_s0_level2_key_enable),
	&PKTINFO(beyondx_a3_s0_copr_clr_cnt_off),
	&PKTINFO(beyondx_a3_s0_level2_key_disable),
};

static void *beyondx_a3_s0_get_copr_spi_cmdtbl[] = {
	&s6e3ha9_restbl[RES_COPR_SPI],
};

static void *beyondx_a3_s0_get_copr_dsi_cmdtbl[] = {
	&s6e3ha9_restbl[RES_COPR_DSI],
};

static struct seqinfo beyondx_a3_s0_copr_seqtbl[MAX_COPR_SEQ] = {
	[COPR_SET_SEQ] = SEQINFO_INIT("set-copr-seq", beyondx_a3_s0_set_copr_cmdtbl),
	[COPR_CLR_CNT_ON_SEQ] = SEQINFO_INIT("clr-copr-cnt-on-seq", beyondx_a3_s0_clr_copr_cnt_on_cmdtbl),
	[COPR_CLR_CNT_OFF_SEQ] = SEQINFO_INIT("clr-copr-cnt-off-seq", beyondx_a3_s0_clr_copr_cnt_off_cmdtbl),
	[COPR_SPI_GET_SEQ] = SEQINFO_INIT("get-copr-spi-seq", beyondx_a3_s0_get_copr_spi_cmdtbl),
	[COPR_DSI_GET_SEQ] = SEQINFO_INIT("get-copr-dsi-seq", beyondx_a3_s0_get_copr_dsi_cmdtbl),
};

static struct panel_copr_data s6e3ha9_beyondx_a3_s0_copr_data = {
	.seqtbl = beyondx_a3_s0_copr_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(beyondx_a3_s0_copr_seqtbl),
	.maptbl = (struct maptbl *)beyondx_a3_s0_copr_maptbl,
	.nr_maptbl = (sizeof(beyondx_a3_s0_copr_maptbl) / sizeof(struct maptbl)),
	.version = COPR_VER_3,
	.options = {
		.thread_on = false,
		.check_avg = false,
	},
	.reg.v3 = {
		.copr_mask = S6E3HA9_BEYONDX_A3_S0_COPR_MASK,
		.cnt_re = false,
		.copr_ilc = S6E3HA9_BEYONDX_A3_S0_COPR_ILC,
		.copr_en = S6E3HA9_BEYONDX_A3_S0_COPR_EN,
		.copr_gamma = S6E3HA9_BEYONDX_A3_S0_COPR_GAMMA,
		.copr_er = S6E3HA9_BEYONDX_A3_S0_COPR_ER,
		.copr_eg = S6E3HA9_BEYONDX_A3_S0_COPR_EG,
		.copr_eb = S6E3HA9_BEYONDX_A3_S0_COPR_EB,
		.copr_erc = S6E3HA9_BEYONDX_A3_S0_COPR_ERC,
		.copr_egc = S6E3HA9_BEYONDX_A3_S0_COPR_EGC,
		.copr_ebc = S6E3HA9_BEYONDX_A3_S0_COPR_EBC,
		.max_cnt = S6E3HA9_BEYONDX_A3_S0_COPR_MAX_CNT,
		.roi_on = 0x3F,
		.roi = {
			[0] = {
				.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_Y_S,
				.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_Y_E,
			},
			[1] = {
				.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_Y_S,
				.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_Y_E,
			},
			[2] = {
				.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_Y_S,
				.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_Y_E,
			},
			[3] = {
				.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_Y_S,
				.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_Y_E,
			},
			[4] = {
				.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_S,
				.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_E,
			},
			[5] = {
				.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_S,
				.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_E,
			},
		},
	},
	.roi = {
		[0] = {
			.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_Y_S,
			.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI1_Y_E,
		},
		[1] = {
			.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_Y_S,
			.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI2_Y_E,
		},
		[2] = {
			.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_Y_S,
			.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI3_Y_E,
		},
		[3] = {
			.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_Y_S,
			.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI4_Y_E,
		},
		[4] = {
			.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_S,
			.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_E,
		},
		[5] = {
			.roi_xs = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_S, .roi_ys = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_S,
			.roi_xe = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_X_E, .roi_ye = S6E3HA9_BEYONDX_A3_S0_COPR_ROI5_Y_E,
		},
	},
	.nr_roi = 5,
};

#endif /* __S6E3HA9_BEYONDX_A3_S0_PANEL_COPR_H__ */
