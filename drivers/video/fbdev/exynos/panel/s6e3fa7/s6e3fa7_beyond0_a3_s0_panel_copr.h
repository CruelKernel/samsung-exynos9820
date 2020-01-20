/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7_beyond0_a3_s0_panel_copr.h
 *
 * Header file for COPR Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA7_BEYOND0_A3_S0_PANEL_COPR_H__
#define __S6E3FA7_BEYOND0_A3_S0_PANEL_COPR_H__

#include "../panel.h"
#include "../copr.h"
#include "s6e3fa7_beyond0_a3_s0_panel.h"

#define S6E3FA7_BEYOND0_A3_S0_COPR_EN	(1)
#define S6E3FA7_BEYOND0_A3_S0_COPR_GAMMA	(1)		/* 0 : GAMMA_1, 1 : GAMMA_2_2 */

#define S6E3FA7_BEYOND0_A3_S0_COPR_CLR_CNT_ON	(\
		(1 << 2) | \
		(S6E3FA7_BEYOND0_A3_S0_COPR_GAMMA << 1) | \
		(S6E3FA7_BEYOND0_A3_S0_COPR_EN << 0))

#define S6E3FA7_BEYOND0_A3_S0_COPR_CLR_CNT_OFF	(\
		(0 << 2) | \
		(S6E3FA7_BEYOND0_A3_S0_COPR_GAMMA << 1) | \
		(S6E3FA7_BEYOND0_A3_S0_COPR_EN << 0))

#define S6E3FA7_BEYOND0_A3_S0_COPR_ER		(262)
#define S6E3FA7_BEYOND0_A3_S0_COPR_EG		(413)
#define S6E3FA7_BEYOND0_A3_S0_COPR_EB		(93)
#define S6E3FA7_BEYOND0_A3_S0_COPR_MAX_CNT	(3)

#define S6E3FA7_BEYOND0_A3_S0_COPR_ROI_X_S	(800)
#define S6E3FA7_BEYOND0_A3_S0_COPR_ROI_Y_S	(138)
#define S6E3FA7_BEYOND0_A3_S0_COPR_ROI_X_E	(835)
#define S6E3FA7_BEYOND0_A3_S0_COPR_ROI_Y_E	(165)

static struct seqinfo beyond0_a3_s0_copr_seqtbl[MAX_COPR_SEQ];
static struct pktinfo PKTINFO(beyond0_a3_s0_level2_key_enable);
static struct pktinfo PKTINFO(beyond0_a3_s0_level2_key_disable);

/* ===================================================================================== */
/* ============================== [S6E3FA7 MAPPING TABLE] ============================== */
/* ===================================================================================== */
static struct maptbl beyond0_a3_s0_copr_maptbl[] = {
	[COPR_MAPTBL] = DEFINE_0D_MAPTBL(beyond0_a3_s0_copr_table, init_common_table, NULL, copy_copr_maptbl),
};

/* ===================================================================================== */
/* ============================== [S6E3FA7 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 BEYOND0_A3_S0_COPR[] = {
	0xE1,
	0x03, 0x14, 0x06, 0x9D, 0x5D, 0x00, 0x03, 0x0B, 0x20, 0x00,
	0x8A, 0x03, 0x43, 0x00, 0xA5
};

static u8 BEYOND0_A3_S0_COPR_CLR_CNT_ON[] = { 0xE1, S6E3FA7_BEYOND0_A3_S0_COPR_CLR_CNT_ON };
static u8 BEYOND0_A3_S0_COPR_CLR_CNT_OFF[] = { 0xE1, S6E3FA7_BEYOND0_A3_S0_COPR_CLR_CNT_OFF };

static DEFINE_PKTUI(beyond0_a3_s0_copr, &beyond0_a3_s0_copr_maptbl[COPR_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(beyond0_a3_s0_copr, DSI_PKT_TYPE_WR, BEYOND0_A3_S0_COPR, 0);

static DEFINE_STATIC_PACKET(beyond0_a3_s0_copr_clr_cnt_on, DSI_PKT_TYPE_WR, BEYOND0_A3_S0_COPR_CLR_CNT_ON, 0);
static DEFINE_STATIC_PACKET(beyond0_a3_s0_copr_clr_cnt_off, DSI_PKT_TYPE_WR, BEYOND0_A3_S0_COPR_CLR_CNT_OFF, 0);

static void *beyond0_a3_s0_set_copr_cmdtbl[] = {
	&PKTINFO(beyond0_a3_s0_level2_key_enable),
	&PKTINFO(beyond0_a3_s0_copr),
	&PKTINFO(beyond0_a3_s0_level2_key_disable),
};

static void *beyond0_a3_s0_clr_copr_cnt_on_cmdtbl[] = {
	&PKTINFO(beyond0_a3_s0_level2_key_enable),
	&PKTINFO(beyond0_a3_s0_copr_clr_cnt_on),
	&PKTINFO(beyond0_a3_s0_level2_key_disable),
};

static void *beyond0_a3_s0_clr_copr_cnt_off_cmdtbl[] = {
	&PKTINFO(beyond0_a3_s0_level2_key_enable),
	&PKTINFO(beyond0_a3_s0_copr_clr_cnt_off),
	&PKTINFO(beyond0_a3_s0_level2_key_disable),
};

static void *beyond0_a3_s0_get_copr_spi_cmdtbl[] = {
	&s6e3fa7_restbl[RES_COPR_SPI],
};

static void *beyond0_a3_s0_get_copr_dsi_cmdtbl[] = {
	&s6e3fa7_restbl[RES_COPR_DSI],
};

static struct seqinfo beyond0_a3_s0_copr_seqtbl[MAX_COPR_SEQ] = {
	[COPR_SET_SEQ] = SEQINFO_INIT("set-copr-seq", beyond0_a3_s0_set_copr_cmdtbl),
	[COPR_CLR_CNT_ON_SEQ] = SEQINFO_INIT("clr-copr-cnt-on-seq", beyond0_a3_s0_clr_copr_cnt_on_cmdtbl),
	[COPR_CLR_CNT_OFF_SEQ] = SEQINFO_INIT("clr-copr-cnt-off-seq", beyond0_a3_s0_clr_copr_cnt_off_cmdtbl),
	[COPR_SPI_GET_SEQ] = SEQINFO_INIT("get-copr-spi-seq", beyond0_a3_s0_get_copr_spi_cmdtbl),
	[COPR_DSI_GET_SEQ] = SEQINFO_INIT("get-copr-dsi-seq", beyond0_a3_s0_get_copr_dsi_cmdtbl),
};

static struct panel_copr_data s6e3fa7_beyond0_a3_s0_copr_data = {
	.seqtbl = beyond0_a3_s0_copr_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(beyond0_a3_s0_copr_seqtbl),
	.maptbl = (struct maptbl *)beyond0_a3_s0_copr_maptbl,
	.nr_maptbl = (sizeof(beyond0_a3_s0_copr_maptbl) / sizeof(struct maptbl)),
	.version = COPR_VER_1,
	.options = {
		.thread_on = false,
		.check_avg = false,
	},
	.reg.v1 = {
		.cnt_re = false,
		.copr_en = S6E3FA7_BEYOND0_A3_S0_COPR_EN,
		.copr_gamma = S6E3FA7_BEYOND0_A3_S0_COPR_GAMMA,
		.copr_er = S6E3FA7_BEYOND0_A3_S0_COPR_ER,
		.copr_eg = S6E3FA7_BEYOND0_A3_S0_COPR_EG,
		.copr_eb = S6E3FA7_BEYOND0_A3_S0_COPR_EB,
		.max_cnt = S6E3FA7_BEYOND0_A3_S0_COPR_MAX_CNT,
		.roi_on = 1,
		.roi_xs = S6E3FA7_BEYOND0_A3_S0_COPR_ROI_X_S,
		.roi_ys = S6E3FA7_BEYOND0_A3_S0_COPR_ROI_Y_S,
		.roi_xe = S6E3FA7_BEYOND0_A3_S0_COPR_ROI_X_E,
		.roi_ye = S6E3FA7_BEYOND0_A3_S0_COPR_ROI_Y_E,
	},
};

#endif /* __S6E3FA7_BEYOND0_A3_S0_PANEL_COPR_H__ */
