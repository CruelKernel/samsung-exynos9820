/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_star2_a3_s0_panel_copr.h
 *
 * Header file for COPR Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_STAR2_A3_S0_PANEL_COPR_H__
#define __S6E3HA8_STAR2_A3_S0_PANEL_COPR_H__

#include "../panel.h"
#include "../copr.h"
#include "s6e3ha8_star2_a3_s0_panel.h"

#define S6E3HA8_STAR2_A3_S0_COPR_GAMMA	(1)		/* 0 : GAMMA_1, 1 : GAMMA_2_2 */
#define S6E3HA8_STAR2_A3_S0_COPR_ER		(0x100)
#define S6E3HA8_STAR2_A3_S0_COPR_EG		(0x100)
#define S6E3HA8_STAR2_A3_S0_COPR_EB		(0x100)
#define S6E3HA8_STAR2_A3_S0_COPR_ERC	(0x100)
#define S6E3HA8_STAR2_A3_S0_COPR_EGC	(0x100)
#define S6E3HA8_STAR2_A3_S0_COPR_EBC	(0x100)
#define S6E3HA8_STAR2_A3_S0_COPR_MAX_CNT	(0xFFFF)

static struct seqinfo star2_a3_s0_copr_seqtbl[MAX_COPR_SEQ];
static struct pktinfo PKTINFO(star2_a3_s0_level2_key_enable);
static struct pktinfo PKTINFO(star2_a3_s0_level2_key_disable);

/* ===================================================================================== */
/* ============================== [S6E3HA8 MAPPING TABLE] ============================== */
/* ===================================================================================== */
static struct maptbl star2_a3_s0_copr_maptbl[] = {
	[COPR_MAPTBL] = DEFINE_0D_MAPTBL(star2_a3_s0_copr_table, init_common_table, NULL, copy_copr_maptbl),
};

/* ===================================================================================== */
/* ============================== [S6E3HA8 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 STAR2_A3_S0_COPR[] = {
	0xE1,
	0x07, 0x15, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
};

static u8 STAR2_A3_S0_COPR_CLR_CNT_ON[] = { 0xE1, 0x0F };
static u8 STAR2_A3_S0_COPR_CLR_CNT_OFF[] = { 0xE1, 0x07 };

static DEFINE_PKTUI(star2_a3_s0_copr, &star2_a3_s0_copr_maptbl[COPR_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(star2_a3_s0_copr, DSI_PKT_TYPE_WR, STAR2_A3_S0_COPR, 0);

static DEFINE_STATIC_PACKET(star2_a3_s0_copr_clr_cnt_on, DSI_PKT_TYPE_WR, STAR2_A3_S0_COPR_CLR_CNT_ON, 0);
static DEFINE_STATIC_PACKET(star2_a3_s0_copr_clr_cnt_off, DSI_PKT_TYPE_WR, STAR2_A3_S0_COPR_CLR_CNT_OFF, 0);

static void *star2_a3_s0_set_copr_cmdtbl[] = {
	&PKTINFO(star2_a3_s0_level2_key_enable),
	&PKTINFO(star2_a3_s0_copr),
	&PKTINFO(star2_a3_s0_level2_key_disable),
};

static void *star2_a3_s0_clr_copr_cnt_on_cmdtbl[] = {
	&PKTINFO(star2_a3_s0_level2_key_enable),
	&PKTINFO(star2_a3_s0_copr_clr_cnt_on),
	&PKTINFO(star2_a3_s0_level2_key_disable),
};

static void *star2_a3_s0_clr_copr_cnt_off_cmdtbl[] = {
	&PKTINFO(star2_a3_s0_level2_key_enable),
	&PKTINFO(star2_a3_s0_copr_clr_cnt_off),
	&PKTINFO(star2_a3_s0_level2_key_disable),
};

static void *star2_a3_s0_get_copr_spi_cmdtbl[] = {
	&s6e3ha8_restbl[RES_COPR_SPI],
};

static void *star2_a3_s0_get_copr_dsi_cmdtbl[] = {
	&s6e3ha8_restbl[RES_COPR_DSI],
};

static struct seqinfo star2_a3_s0_copr_seqtbl[MAX_COPR_SEQ] = {
	[COPR_SET_SEQ] = SEQINFO_INIT("set-copr-seq", star2_a3_s0_set_copr_cmdtbl),
	[COPR_CLR_CNT_ON_SEQ] = SEQINFO_INIT("clr-copr-cnt-on-seq", star2_a3_s0_clr_copr_cnt_on_cmdtbl),
	[COPR_CLR_CNT_OFF_SEQ] = SEQINFO_INIT("clr-copr-cnt-off-seq", star2_a3_s0_clr_copr_cnt_off_cmdtbl),
	[COPR_SPI_GET_SEQ] = SEQINFO_INIT("get-copr-spi-seq", star2_a3_s0_get_copr_spi_cmdtbl),
	[COPR_DSI_GET_SEQ] = SEQINFO_INIT("get-copr-dsi-seq", star2_a3_s0_get_copr_dsi_cmdtbl),
};

static struct panel_copr_data s6e3ha8_star2_a3_s0_copr_data = {
	.seqtbl = star2_a3_s0_copr_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(star2_a3_s0_copr_seqtbl),
	.maptbl = (struct maptbl *)star2_a3_s0_copr_maptbl,
	.nr_maptbl = (sizeof(star2_a3_s0_copr_maptbl) / sizeof(struct maptbl)),
	.version = COPR_VER_2,
	.reg.v2 = {
		.cnt_re = false,
		.copr_ilc = true,
		.copr_en = true,
		.copr_gamma = S6E3HA8_STAR2_A3_S0_COPR_GAMMA,
		.copr_er = S6E3HA8_STAR2_A3_S0_COPR_ER,
		.copr_eg = S6E3HA8_STAR2_A3_S0_COPR_EG,
		.copr_eb = S6E3HA8_STAR2_A3_S0_COPR_EB,
		.copr_erc = S6E3HA8_STAR2_A3_S0_COPR_ER,
		.copr_egc = S6E3HA8_STAR2_A3_S0_COPR_EG,
		.copr_ebc = S6E3HA8_STAR2_A3_S0_COPR_EB,
		.max_cnt = S6E3HA8_STAR2_A3_S0_COPR_MAX_CNT,
		.roi_on = false,
		.roi_xs = 0,
		.roi_ys = 0,
		.roi_xe = 0,
		.roi_ye = 0,
	},
};

#endif /* __S6E3HA8_STAR2_A3_S0_PANEL_COPR_H__ */
