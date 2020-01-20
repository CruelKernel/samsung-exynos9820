/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7_beyond_panel_poc.h
 *
 * Header file for POC Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA7_BEYOND_PANEL_POC_H__
#define __S6E3FA7_BEYOND_PANEL_POC_H__

#include "../panel.h"
#include "../panel_poc.h"

#define BEYOND_BDIV	(BIT_RATE_DIV_4)
#define BEYOND_EXEC_USEC	(20)
#define BEYOND_SET_DONE_MDELAY		(30)
#define BEYOND_RD_DONE_UDELAY		(250)
#define BEYOND_WR_DONE_UDELAY		(4000)
#ifdef CONFIG_SUPPORT_POC_FLASH
#define BEYOND_ER_4K_DONE_MDELAY		(400)
#define BEYOND_ER_32K_DONE_MDELAY		(800)
#define BEYOND_ER_64K_DONE_MDELAY		(1000)
#endif

#define BEYOND_POC_IMG_ADDR	(0)
#define BEYOND_POC_IMG_SIZE	(310846)

#ifdef CONFIG_SUPPORT_DIM_FLASH
#define BEYOND_POC_DIM_DATA_ADDR	(0x70000)
#define BEYOND_POC_DIM_DATA_SIZE (S6E3FA7_DIM_FLASH_DATA_SIZE)
#define BEYOND_POC_DIM_CHECKSUM_ADDR	(BEYOND_POC_DIM_DATA_ADDR + S6E3FA7_DIM_FLASH_CHECKSUM_OFS)
#define BEYOND_POC_DIM_CHECKSUM_SIZE (S6E3FA7_DIM_FLASH_CHECKSUM_LEN)
#define BEYOND_POC_DIM_MAGICNUM_ADDR	(BEYOND_POC_DIM_DATA_ADDR + S6E3FA7_DIM_FLASH_MAGICNUM_OFS)
#define BEYOND_POC_DIM_MAGICNUM_SIZE (S6E3FA7_DIM_FLASH_MAGICNUM_LEN)
#define BEYOND_POC_DIM_TOTAL_SIZE (S6E3FA7_DIM_FLASH_TOTAL_SIZE)

#define BEYOND_POC_MTP_DATA_ADDR	(0x72000)
#define BEYOND_POC_MTP_DATA_SIZE (S6E3FA7_DIM_FLASH_MTP_DATA_SIZE)
#define BEYOND_POC_MTP_CHECKSUM_ADDR	(BEYOND_POC_MTP_DATA_ADDR + S6E3FA7_DIM_FLASH_MTP_CHECKSUM_OFS)
#define BEYOND_POC_MTP_CHECKSUM_SIZE (S6E3FA7_DIM_FLASH_MTP_CHECKSUM_LEN)
#define BEYOND_POC_MTP_TOTAL_SIZE (S6E3FA7_DIM_FLASH_MTP_TOTAL_SIZE)
#endif

#define BEYOND_POC_MCD_ADDR	(0xB8000)
#define BEYOND_POC_MCD_SIZE (S6E3FA7_FLASH_MCD_LEN)

/* ===================================================================================== */
/* ============================== [S6E3FA7 MAPPING TABLE] ============================== */
/* ===================================================================================== */
static struct maptbl beyond_poc_maptbl[] = {
	[POC_WR_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(beyond_poc_wr_addr_table, init_common_table, NULL, copy_poc_wr_addr_maptbl),
	[POC_RD_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(beyond_poc_rd_addr_table, init_common_table, NULL, copy_poc_rd_addr_maptbl),
	[POC_WR_DATA_MAPTBL] = DEFINE_0D_MAPTBL(beyond_poc_wr_data_table, init_common_table, NULL, copy_poc_wr_data_maptbl),
#ifdef CONFIG_SUPPORT_POC_FLASH
	[POC_ER_ADDR_MAPTBL] = DEFINE_0D_MAPTBL(beyond_poc_er_addr_table, init_common_table, NULL, copy_poc_er_addr_maptbl),
#endif
};

/* ===================================================================================== */
/* ============================== [S6E3FA7 COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 BEYOND_KEY1_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 BEYOND_KEY1_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 BEYOND_KEY2_ENABLE[] = { 0xF1, 0xF1, 0xA2 };
static u8 BEYOND_KEY2_DISABLE[] = { 0xF1, 0xA5, 0xA5 };
static u8 BEYOND_POC_PGM_ENABLE[] = { 0xC0, 0x02 };
static u8 BEYOND_POC_PGM_DISABLE[] = { 0xC0, 0x00 };
static u8 BEYOND_POC_EXEC[] = { 0xC0, 0x03 };

static u8 BEYOND_POC_PRE_01[] = { 0xEB, 0x4E };
static u8 BEYOND_POC_PRE_02[] = { 0xEB, 0x6A };

static u8 BEYOND_POC_RW_CTRL_01[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
static u8 BEYOND_POC_RW_CTRL_02[] = { 0xC1, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10 };

static u8 BEYOND_POC_RD_STT[] = { 0xC1, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01 };

static u8 BEYOND_POC_WR_ENABLE[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static u8 BEYOND_POC_WR_STT[] = { 0xC1, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01 };
static u8 BEYOND_POC_WR_END[] = { 0xC1, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01 };
static u8 BEYOND_POC_WR_DAT[] = { 0xC1, 0x00 };

#ifdef CONFIG_SUPPORT_POC_FLASH
static u8 BEYOND_POC_ER_CTRL_01[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };
static u8 BEYOND_POC_ER_CTRL_02[] = { 0xC1, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04 };

static u8 BEYOND_POC_ER_ENABLE[] = { 0xC1, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };
static u8 BEYOND_POC_ER_4K_STT[] = { 0xC1, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
static u8 BEYOND_POC_ER_32K_STT[] = { 0xC1, 0x00, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
static u8 BEYOND_POC_ER_64K_STT[] = { 0xC1, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04 };
#endif

static DEFINE_STATIC_PACKET(beyond_level1_key_enable, DSI_PKT_TYPE_WR, BEYOND_KEY1_ENABLE, 0);
static DEFINE_STATIC_PACKET(beyond_level1_key_disable, DSI_PKT_TYPE_WR, BEYOND_KEY1_DISABLE, 0);
static DEFINE_STATIC_PACKET(beyond_level2_key_enable, DSI_PKT_TYPE_WR, BEYOND_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(beyond_level2_key_disable, DSI_PKT_TYPE_WR, BEYOND_KEY2_DISABLE, 0);
static DEFINE_STATIC_PACKET(beyond_poc_pgm_enable, DSI_PKT_TYPE_WR, BEYOND_POC_PGM_ENABLE, 0);
static DEFINE_STATIC_PACKET(beyond_poc_pgm_disable, DSI_PKT_TYPE_WR, BEYOND_POC_PGM_DISABLE, 0);
static DEFINE_STATIC_PACKET(beyond_poc_pre_01, DSI_PKT_TYPE_WR, BEYOND_POC_PRE_01, 0);
static DEFINE_STATIC_PACKET(beyond_poc_pre_02, DSI_PKT_TYPE_WR, BEYOND_POC_PRE_02, 6);
static DEFINE_STATIC_PACKET(beyond_poc_exec, DSI_PKT_TYPE_WR, BEYOND_POC_EXEC, 0);

static DEFINE_STATIC_PACKET(beyond_poc_rw_ctrl_01, DSI_PKT_TYPE_WR, BEYOND_POC_RW_CTRL_01, 0);
static DEFINE_STATIC_PACKET(beyond_poc_rw_ctrl_02, DSI_PKT_TYPE_WR, BEYOND_POC_RW_CTRL_02, 0);

static DEFINE_PKTUI(beyond_poc_rd_stt, &beyond_poc_maptbl[POC_RD_ADDR_MAPTBL], 6);
static DEFINE_VARIABLE_PACKET(beyond_poc_rd_stt, DSI_PKT_TYPE_WR, BEYOND_POC_RD_STT, 0);

static DEFINE_STATIC_PACKET(beyond_poc_wr_enable, DSI_PKT_TYPE_WR, BEYOND_POC_WR_ENABLE, 0);
static DEFINE_PKTUI(beyond_poc_wr_stt, &beyond_poc_maptbl[POC_WR_ADDR_MAPTBL], 6);
static DEFINE_VARIABLE_PACKET(beyond_poc_wr_stt, DSI_PKT_TYPE_WR, BEYOND_POC_WR_STT, 0);
static DEFINE_STATIC_PACKET(beyond_poc_wr_end, DSI_PKT_TYPE_WR, BEYOND_POC_WR_END, 0);
static DEFINE_PKTUI(beyond_poc_wr_dat, &beyond_poc_maptbl[POC_WR_DATA_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(beyond_poc_wr_dat, DSI_PKT_TYPE_WR, BEYOND_POC_WR_DAT, 0);

static DEFINE_PANEL_UDELAY_NO_SLEEP(beyond_poc_wait_exec, BEYOND_EXEC_USEC);
static DEFINE_PANEL_UDELAY_NO_SLEEP(beyond_poc_wait_rd_done, BEYOND_RD_DONE_UDELAY);
static DEFINE_PANEL_UDELAY_NO_SLEEP(beyond_poc_wait_wr_done, BEYOND_WR_DONE_UDELAY);
static DEFINE_PANEL_MDELAY(beyond_poc_wait_set_done, BEYOND_SET_DONE_MDELAY);

#ifdef CONFIG_SUPPORT_POC_FLASH
static DEFINE_STATIC_PACKET(beyond_poc_er_ctrl_01, DSI_PKT_TYPE_WR, BEYOND_POC_ER_CTRL_01, 0);
static DEFINE_STATIC_PACKET(beyond_poc_er_ctrl_02, DSI_PKT_TYPE_WR, BEYOND_POC_ER_CTRL_02, 0);
static DEFINE_STATIC_PACKET(beyond_poc_er_enable, DSI_PKT_TYPE_WR, BEYOND_POC_ER_ENABLE, 0);
static DEFINE_PKTUI(beyond_poc_er_4k_stt, &beyond_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(beyond_poc_er_4k_stt, DSI_PKT_TYPE_WR, BEYOND_POC_ER_4K_STT, 0);
static DEFINE_PANEL_MDELAY(beyond_poc_wait_er_4k_done, BEYOND_ER_4K_DONE_MDELAY);
static DEFINE_PKTUI(beyond_poc_er_32k_stt, &beyond_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(beyond_poc_er_32k_stt, DSI_PKT_TYPE_WR, BEYOND_POC_ER_32K_STT, 0);
static DEFINE_PANEL_MDELAY(beyond_poc_wait_er_32k_done, BEYOND_ER_32K_DONE_MDELAY);
static DEFINE_PKTUI(beyond_poc_er_64k_stt, &beyond_poc_maptbl[POC_ER_ADDR_MAPTBL], 3);
static DEFINE_VARIABLE_PACKET(beyond_poc_er_64k_stt, DSI_PKT_TYPE_WR, BEYOND_POC_ER_64K_STT, 0);
static DEFINE_PANEL_MDELAY(beyond_poc_wait_er_64k_done, BEYOND_ER_64K_DONE_MDELAY);

static void *beyond_poc_erase_enter_cmdtbl[] = {
	&PKTINFO(beyond_level1_key_enable),
	&PKTINFO(beyond_level2_key_enable),
	&PKTINFO(beyond_poc_pre_01),
	&PKTINFO(beyond_poc_pre_02),
	&PKTINFO(beyond_poc_pgm_enable),
	&PKTINFO(beyond_poc_er_ctrl_01),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_er_ctrl_02),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_set_done),
};

static void *beyond_poc_erase_4k_cmdtbl[] = {
	&PKTINFO(beyond_poc_er_enable),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_er_4k_stt),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_er_4k_done),
};

static void *beyond_poc_erase_32k_cmdtbl[] = {
	&PKTINFO(beyond_poc_er_enable),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_er_32k_stt),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_er_32k_done),
};

static void *beyond_poc_erase_64k_cmdtbl[] = {
	&PKTINFO(beyond_poc_er_enable),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_er_64k_stt),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_er_64k_done),
};

static void *beyond_poc_erase_exit_cmdtbl[] = {
	&PKTINFO(beyond_poc_pgm_disable),
	&PKTINFO(beyond_level1_key_disable),
	&PKTINFO(beyond_level2_key_disable),
};
#endif

static void *beyond_poc_wr_enter_cmdtbl[] = {
	&PKTINFO(beyond_level1_key_enable),
	&PKTINFO(beyond_level2_key_enable),
	&PKTINFO(beyond_poc_pre_01),
	&PKTINFO(beyond_poc_pre_02),
	&PKTINFO(beyond_poc_pgm_enable),
	&PKTINFO(beyond_poc_rw_ctrl_01),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_rw_ctrl_02),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_set_done),
};

static void *beyond_poc_wr_stt_cmdtbl[] = {
	&PKTINFO(beyond_poc_wr_enable),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_wr_stt),
};

static void *beyond_poc_wr_dat_cmdtbl[] = {
	&PKTINFO(beyond_poc_wr_dat),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
};

static void *beyond_poc_wr_end_cmdtbl[] = {
	/* continue write end */
	&PKTINFO(beyond_poc_wr_end),
	&DLYINFO(beyond_poc_wait_wr_done),
};

static void *beyond_poc_wr_exit_cmdtbl[] = {
	&PKTINFO(beyond_poc_pgm_disable),
	&PKTINFO(beyond_level1_key_disable),
	&PKTINFO(beyond_level2_key_disable),
};

static void *beyond_poc_rd_pre_enter_cmdtbl[] = {
	&PKTINFO(beyond_level1_key_enable),
	&PKTINFO(beyond_level2_key_enable),
	&PKTINFO(beyond_poc_pre_01),
	&PKTINFO(beyond_poc_pre_02),
	&PKTINFO(beyond_level1_key_disable),
	&PKTINFO(beyond_level2_key_disable),
};

static void *beyond_poc_rd_enter_cmdtbl[] = {
	&PKTINFO(beyond_level1_key_enable),
	&PKTINFO(beyond_level2_key_enable),
	&PKTINFO(beyond_poc_pgm_enable),
	&PKTINFO(beyond_poc_rw_ctrl_01),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_exec),
	&PKTINFO(beyond_poc_rw_ctrl_02),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_set_done),
};

static void *beyond_poc_rd_dat_cmdtbl[] = {
	&PKTINFO(beyond_poc_rd_stt),
	&PKTINFO(beyond_poc_exec),
	&DLYINFO(beyond_poc_wait_rd_done),
	&s6e3fa7_restbl[RES_POC_DATA],
};

static void *beyond_poc_rd_exit_cmdtbl[] = {
	&PKTINFO(beyond_poc_pgm_disable),
	&PKTINFO(beyond_level1_key_disable),
	&PKTINFO(beyond_level2_key_disable),
};

static struct seqinfo beyond_poc_seqtbl[MAX_POC_SEQ] = {
#ifdef CONFIG_SUPPORT_POC_FLASH
	/* poc erase */
	[POC_ERASE_ENTER_SEQ] = SEQINFO_INIT("poc-erase-enter-seq", beyond_poc_erase_enter_cmdtbl),
	[POC_ERASE_4K_SEQ] = SEQINFO_INIT("poc-erase-4k-seq", beyond_poc_erase_4k_cmdtbl),
	[POC_ERASE_32K_SEQ] = SEQINFO_INIT("poc-erase-32k-seq", beyond_poc_erase_32k_cmdtbl),
	[POC_ERASE_64K_SEQ] = SEQINFO_INIT("poc-erase-64k-seq", beyond_poc_erase_64k_cmdtbl),
	[POC_ERASE_EXIT_SEQ] = SEQINFO_INIT("poc-erase-exit-seq", beyond_poc_erase_exit_cmdtbl),
#endif

	/* poc write */
	[POC_WRITE_ENTER_SEQ] = SEQINFO_INIT("poc-wr-enter-seq", beyond_poc_wr_enter_cmdtbl),
	[POC_WRITE_STT_SEQ] = SEQINFO_INIT("poc-wr-stt-seq", beyond_poc_wr_stt_cmdtbl),
	[POC_WRITE_DAT_SEQ] = SEQINFO_INIT("poc-wr-dat-seq", beyond_poc_wr_dat_cmdtbl),
	[POC_WRITE_END_SEQ] = SEQINFO_INIT("poc-wr-end-seq", beyond_poc_wr_end_cmdtbl),
	[POC_WRITE_EXIT_SEQ] = SEQINFO_INIT("poc-wr-exit-seq", beyond_poc_wr_exit_cmdtbl),

	/* poc read */
	[POC_READ_PRE_ENTER_SEQ] = SEQINFO_INIT("poc-rd-pre-enter-seq", beyond_poc_rd_pre_enter_cmdtbl),
	[POC_READ_ENTER_SEQ] = SEQINFO_INIT("poc-rd-enter-seq", beyond_poc_rd_enter_cmdtbl),
	[POC_READ_DAT_SEQ] = SEQINFO_INIT("poc-rd-dat-seq", beyond_poc_rd_dat_cmdtbl),
	[POC_READ_EXIT_SEQ] = SEQINFO_INIT("poc-rd-exit-seq", beyond_poc_rd_exit_cmdtbl),
};

/* partition consists of DATA, CHECKSUM and MAGICNUM */
static struct poc_partition beyond_poc_partition[] = {
	[POC_IMG_PARTITION] = {
		.name = "image",
		.addr = BEYOND_POC_IMG_ADDR,
		.size = BEYOND_POC_IMG_SIZE,
		.need_preload = false
	},
#ifdef CONFIG_SUPPORT_DIM_FLASH
	[POC_DIM_PARTITION] = {
		.name = "dimming",
		.addr = BEYOND_POC_DIM_DATA_ADDR,
		.size = BEYOND_POC_DIM_TOTAL_SIZE,
		.data_addr = BEYOND_POC_DIM_DATA_ADDR,
		.data_size = BEYOND_POC_DIM_DATA_SIZE,
		.checksum_addr = BEYOND_POC_DIM_CHECKSUM_ADDR,
		.checksum_size = BEYOND_POC_DIM_CHECKSUM_SIZE,
		.magicnum_addr = BEYOND_POC_DIM_MAGICNUM_ADDR,
		.magicnum_size = BEYOND_POC_DIM_MAGICNUM_SIZE,
		.need_preload = false,
		.magicnum = 1,
	},
	[POC_MTP_PARTITION] = {
		.name = "mtp",
		.addr = BEYOND_POC_MTP_DATA_ADDR,
		.size = BEYOND_POC_MTP_TOTAL_SIZE,
		.data_addr = BEYOND_POC_MTP_DATA_ADDR,
		.data_size = BEYOND_POC_MTP_DATA_SIZE,
		.checksum_addr = BEYOND_POC_MTP_CHECKSUM_ADDR,
		.checksum_size = BEYOND_POC_MTP_CHECKSUM_SIZE,
		.magicnum_addr = 0,
		.magicnum_size = 0,
		.need_preload = false,
	},
#endif
	[POC_MCD_PARTITION] = {
		.name = "mcd",
		.addr = BEYOND_POC_MCD_ADDR,
		.size = BEYOND_POC_MCD_SIZE,
		.need_preload = false
	},
};

static struct panel_poc_data s6e3fa7_beyond_poc_data = {
	.version = 2,
	.seqtbl = beyond_poc_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(beyond_poc_seqtbl),
	.maptbl = beyond_poc_maptbl,
	.nr_maptbl = ARRAY_SIZE(beyond_poc_maptbl),
	.partition = beyond_poc_partition,
	.nr_partition = ARRAY_SIZE(beyond_poc_partition),
	.wdata_len = 1,
};
#endif /* __S6E3FA7_BEYOND_PANEL_POC_H__ */
