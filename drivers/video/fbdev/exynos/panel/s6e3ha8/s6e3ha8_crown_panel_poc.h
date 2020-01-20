/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_crown_panel_poc.h
 *
 * Header file for POC Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_CROWN_PANEL_POC_H__
#define __S6E3HA8_CROWN_PANEL_POC_H__

#include "../panel.h"
#include "../panel_poc.h"

#define CROWN_BDIV	(BIT_RATE_DIV_4)
#define CROWN_EXEC_USEC	(2)
#define CROWN_QD_DONE_MDELAY		(30)
#define CROWN_RD_DONE_UDELAY		(250)
#define CROWN_WR_DONE_UDELAY		(4000)

#define CROWN_POC_IMG_ADDR	(0)
#define CROWN_POC_IMG_SIZE	(553772)

#ifdef CONFIG_SUPPORT_DIM_FLASH
#define CROWN_POC_DIM_DATA_ADDR	(0xA0000)
#define CROWN_POC_DIM_DATA_SIZE (S6E3HA8_DIM_FLASH_DATA_SIZE)
#define CROWN_POC_DIM_CHECKSUM_ADDR	(CROWN_POC_DIM_DATA_ADDR + S6E3HA8_DIM_FLASH_CHECKSUM_OFS)
#define CROWN_POC_DIM_CHECKSUM_SIZE (S6E3HA8_DIM_FLASH_CHECKSUM_LEN)
#define CROWN_POC_DIM_MAGICNUM_ADDR	(CROWN_POC_DIM_DATA_ADDR + S6E3HA8_DIM_FLASH_MAGICNUM_OFS)
#define CROWN_POC_DIM_MAGICNUM_SIZE (S6E3HA8_DIM_FLASH_MAGICNUM_LEN)
#define CROWN_POC_DIM_TOTAL_SIZE (S6E3HA8_DIM_FLASH_TOTAL_SIZE)

#define CROWN_POC_MTP_DATA_ADDR	(0xA2000)
#define CROWN_POC_MTP_DATA_SIZE (S6E3HA8_DIM_FLASH_MTP_DATA_SIZE)
#define CROWN_POC_MTP_CHECKSUM_ADDR	(CROWN_POC_MTP_DATA_ADDR + S6E3HA8_DIM_FLASH_MTP_CHECKSUM_OFS)
#define CROWN_POC_MTP_CHECKSUM_SIZE (S6E3HA8_DIM_FLASH_MTP_CHECKSUM_LEN)
#define CROWN_POC_MTP_TOTAL_SIZE (S6E3HA8_DIM_FLASH_MTP_TOTAL_SIZE)
#endif

#define CROWN_POC_MCD_ADDR	(0xB8000)
#define CROWN_POC_MCD_SIZE (S6E3HA8_FLASH_MCD_LEN)

/* ===================================================================================== */
/* ============================== [S6E3HA8 MAPPING TABLE] ============================== */
/* ===================================================================================== */
static struct maptbl crown_poc_maptbl[] = {
	[POC_WR_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(crown_poc_wr_addr_table, init_common_table, NULL, copy_poc_wr_addr_maptbl),
	[POC_RD_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(crown_poc_rd_addr_table, init_common_table, NULL, copy_poc_rd_addr_maptbl),
	[POC_WR_DATA_MAPTBL] = DEFINE_0D_MAPTBL(crown_poc_wr_data_table, init_common_table, NULL, copy_poc_wr_data_maptbl),
};

/* ===================================================================================== */
/* ============================== [S6E3HA8 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 CROWN_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 CROWN_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 CROWN_POC_KEY_ENABLE[] = { 0xF1, 0xF1, 0xA2 };
static u8 CROWN_POC_KEY_DISABLE[] = { 0xF1, 0xA5, 0xA5 };
static u8 CROWN_POC_PGM_ENABLE[] = { 0xC0, 0x02 };
static u8 CROWN_POC_PGM_DISABLE[] = { 0xC0, 0x00 };
static u8 CROWN_POC_EXEC[] = { 0xC0, 0x03 };
#ifdef CONFIG_SUPPORT_POC_FLASH
static u8 CROWN_POC_ERASE[] = { 0xC1, 0x00, 0xC7, 0x00, 0x10, 0x00 };
#endif
static u8 CROWN_POC_WR_ENABLE[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CROWN_BDIV };
static u8 CROWN_POC_QD_ENABLE[] = { 0xC1, 0x00, 0x01, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10 };
static u8 CROWN_POC_WR_STT[] = { 0xC1, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, CROWN_BDIV };
static u8 CROWN_POC_WR_END[] = { 0xC1, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, CROWN_BDIV };
static u8 CROWN_POC_RD_STT[] = { 0xC1, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CROWN_BDIV, 0x01 };
static u8 CROWN_POC_WR_DAT[] = { 0xC1, 0x00 };

static DEFINE_STATIC_PACKET(crown_level2_key_enable, DSI_PKT_TYPE_WR, CROWN_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(crown_level2_key_disable, DSI_PKT_TYPE_WR, CROWN_KEY2_DISABLE, 0);
static DEFINE_STATIC_PACKET(crown_poc_key_enable, DSI_PKT_TYPE_WR, CROWN_POC_KEY_ENABLE, 0);
static DEFINE_STATIC_PACKET(crown_poc_key_disable, DSI_PKT_TYPE_WR, CROWN_POC_KEY_DISABLE, 0);
static DEFINE_STATIC_PACKET(crown_poc_pgm_enable, DSI_PKT_TYPE_WR, CROWN_POC_PGM_ENABLE, 0);
static DEFINE_STATIC_PACKET(crown_poc_pgm_disable, DSI_PKT_TYPE_WR, CROWN_POC_PGM_DISABLE, 0);
#ifdef CONFIG_SUPPORT_POC_FLASH
static DEFINE_STATIC_PACKET(crown_poc_erase, DSI_PKT_TYPE_WR, CROWN_POC_ERASE, 0);
#endif
static DEFINE_STATIC_PACKET(crown_poc_exec, DSI_PKT_TYPE_WR, CROWN_POC_EXEC, 0);
static DEFINE_STATIC_PACKET(crown_poc_wr_enable, DSI_PKT_TYPE_WR, CROWN_POC_WR_ENABLE, 0);
static DEFINE_STATIC_PACKET(crown_poc_qd_enable, DSI_PKT_TYPE_WR, CROWN_POC_QD_ENABLE, 0);
static DEFINE_PKTUI(crown_poc_rd_stt, &crown_poc_maptbl[POC_RD_ADDR_MAPTBL], 6);
static DEFINE_VARIABLE_PACKET(crown_poc_rd_stt, DSI_PKT_TYPE_WR, CROWN_POC_RD_STT, 0);
static DEFINE_PKTUI(crown_poc_wr_stt, &crown_poc_maptbl[POC_WR_ADDR_MAPTBL], 6);
static DEFINE_VARIABLE_PACKET(crown_poc_wr_stt, DSI_PKT_TYPE_WR, CROWN_POC_WR_STT, 0);
static DEFINE_STATIC_PACKET(crown_poc_wr_end, DSI_PKT_TYPE_WR, CROWN_POC_WR_END, 0);
static DEFINE_PKTUI(crown_poc_wr_dat, &crown_poc_maptbl[POC_WR_DATA_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(crown_poc_wr_dat, DSI_PKT_TYPE_WR, CROWN_POC_WR_DAT, 0);

static DEFINE_PANEL_UDELAY_NO_SLEEP(crown_poc_wait_exec, CROWN_EXEC_USEC);
static DEFINE_PANEL_UDELAY_NO_SLEEP(crown_poc_wait_rd_done, CROWN_RD_DONE_UDELAY);
static DEFINE_PANEL_UDELAY_NO_SLEEP(crown_poc_wait_wr_done, CROWN_WR_DONE_UDELAY);
static DEFINE_PANEL_MDELAY(crown_poc_wait_qd_status, CROWN_QD_DONE_MDELAY);

#ifdef CONFIG_SUPPORT_POC_FLASH
static void *crown_poc_erase_enter_cmdtbl[] = {
	&PKTINFO(crown_level2_key_enable),
	&PKTINFO(crown_poc_key_enable),
	&PKTINFO(crown_poc_pgm_enable),
	&PKTINFO(crown_poc_wr_enable),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_exec),
};

static void *crown_poc_erase_cmdtbl[] = {
	&PKTINFO(crown_poc_erase),
	&PKTINFO(crown_poc_exec),
	//&DLYINFO(poc_wait_erase),
};

static void *crown_poc_erase_exit_cmdtbl[] = {
	&PKTINFO(crown_poc_pgm_disable),
	&PKTINFO(crown_poc_key_disable),
	&PKTINFO(crown_level2_key_disable),
};
#endif

static void *crown_poc_wr_enter_cmdtbl[] = {
	&PKTINFO(crown_level2_key_enable),
	&PKTINFO(crown_poc_key_enable),
	&PKTINFO(crown_poc_pgm_enable),
	&PKTINFO(crown_poc_wr_enable),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_exec),
	&PKTINFO(crown_poc_qd_enable),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_qd_status),
};

static void *crown_poc_wr_stt_cmdtbl[] = {
	&PKTINFO(crown_poc_wr_enable),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_exec),
	/* address & continue write begin */
	&PKTINFO(crown_poc_wr_stt),
};

static void *crown_poc_wr_dat_cmdtbl[] = {
	&PKTINFO(crown_poc_wr_dat),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_exec),
};

static void *crown_poc_wr_end_cmdtbl[] = {
	/* continue write end */
	&PKTINFO(crown_poc_wr_end),
	&DLYINFO(crown_poc_wait_wr_done),
};

static void *crown_poc_wr_exit_cmdtbl[] = {
	&PKTINFO(crown_poc_pgm_disable),
	&PKTINFO(crown_poc_key_disable),
	&PKTINFO(crown_level2_key_disable),
};

static void *crown_poc_rd_enter_cmdtbl[] = {
	&PKTINFO(crown_level2_key_enable),
	&PKTINFO(crown_poc_key_enable),
	&PKTINFO(crown_poc_pgm_enable),
	&PKTINFO(crown_poc_wr_enable),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_exec),
	&PKTINFO(crown_poc_qd_enable),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_qd_status),
};

static void *crown_poc_rd_dat_cmdtbl[] = {
	&PKTINFO(crown_poc_rd_stt),
	&PKTINFO(crown_poc_exec),
	&DLYINFO(crown_poc_wait_rd_done),
	&s6e3ha8_restbl[RES_POC_DATA],
};

static void *crown_poc_rd_exit_cmdtbl[] = {
	&PKTINFO(crown_poc_pgm_disable),
	&PKTINFO(crown_poc_key_disable),
	&PKTINFO(crown_level2_key_disable),
};

static struct seqinfo crown_poc_seqtbl[MAX_POC_SEQ] = {
#ifdef CONFIG_SUPPORT_POC_FLASH
	/* poc erase */
	[POC_ERASE_ENTER_SEQ] = SEQINFO_INIT("poc-erase-enter-seq", crown_poc_erase_enter_cmdtbl),
	[POC_ERASE_SEQ] = SEQINFO_INIT("poc-erase-seq", crown_poc_erase_cmdtbl),
	[POC_ERASE_EXIT_SEQ] = SEQINFO_INIT("poc-erase-exit-seq", crown_poc_erase_exit_cmdtbl),
#endif

	/* poc write */
	[POC_WRITE_ENTER_SEQ] = SEQINFO_INIT("poc-wr-enter-seq", crown_poc_wr_enter_cmdtbl),
	[POC_WRITE_STT_SEQ] = SEQINFO_INIT("poc-wr-stt-seq", crown_poc_wr_stt_cmdtbl),
	[POC_WRITE_DAT_SEQ] = SEQINFO_INIT("poc-wr-dat-seq", crown_poc_wr_dat_cmdtbl),
	[POC_WRITE_END_SEQ] = SEQINFO_INIT("poc-wr-end-seq", crown_poc_wr_end_cmdtbl),
	[POC_WRITE_EXIT_SEQ] = SEQINFO_INIT("poc-wr-exit-seq", crown_poc_wr_exit_cmdtbl),

	/* poc read */
	[POC_READ_ENTER_SEQ] = SEQINFO_INIT("poc-rd-enter-seq", crown_poc_rd_enter_cmdtbl),
	[POC_READ_DAT_SEQ] = SEQINFO_INIT("poc-rd-dat-seq", crown_poc_rd_dat_cmdtbl),
	[POC_READ_EXIT_SEQ] = SEQINFO_INIT("poc-rd-exit-seq", crown_poc_rd_exit_cmdtbl),
};

/* partition consists of DATA, CHECKSUM and MAGICNUM */
static struct poc_partition crown_poc_partition[] = {
	[POC_IMG_PARTITION] = {
		.name = "image",
		.addr = CROWN_POC_IMG_ADDR,
		.size = CROWN_POC_IMG_SIZE,
		.need_preload = false
	},
#ifdef CONFIG_SUPPORT_DIM_FLASH
	[POC_DIM_PARTITION] = {
		.name = "dimming",
		.addr = CROWN_POC_DIM_DATA_ADDR,
		.size = CROWN_POC_DIM_TOTAL_SIZE,
		.data_addr = CROWN_POC_DIM_DATA_ADDR,
		.data_size = CROWN_POC_DIM_DATA_SIZE,
		.checksum_addr = CROWN_POC_DIM_CHECKSUM_ADDR,
		.checksum_size = CROWN_POC_DIM_CHECKSUM_SIZE,
		.magicnum_addr = CROWN_POC_DIM_MAGICNUM_ADDR,
		.magicnum_size = CROWN_POC_DIM_MAGICNUM_SIZE,
		.need_preload = false,
		.magicnum = 1,
	},
	[POC_MTP_PARTITION] = {
		.name = "mtp",
		.addr = CROWN_POC_MTP_DATA_ADDR,
		.size = CROWN_POC_MTP_TOTAL_SIZE,
		.data_addr = CROWN_POC_MTP_DATA_ADDR,
		.data_size = CROWN_POC_MTP_DATA_SIZE,
		.checksum_addr = CROWN_POC_MTP_CHECKSUM_ADDR,
		.checksum_size = CROWN_POC_MTP_CHECKSUM_SIZE,
		.magicnum_addr = 0,
		.magicnum_size = 0,
		.need_preload = false,
	},
#endif
	[POC_MCD_PARTITION] = {
		.name = "mcd",
		.addr = CROWN_POC_MCD_ADDR,
		.size = CROWN_POC_MCD_SIZE,
		.need_preload = false
	},
};

static struct panel_poc_data s6e3ha8_crown_poc_data = {
	.version = 2,
	.seqtbl = crown_poc_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(crown_poc_seqtbl),
	.maptbl = crown_poc_maptbl,
	.nr_maptbl = ARRAY_SIZE(crown_poc_maptbl),
	.partition = crown_poc_partition,
	.nr_partition = ARRAY_SIZE(crown_poc_partition),
};
#endif /* __S6E3HA8_CROWN_PANEL_POC_H__ */
