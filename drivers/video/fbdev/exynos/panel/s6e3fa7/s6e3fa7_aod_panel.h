/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7_aod_panel.h
 *
 * Header file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3FA7_AOD_PANEL_H__
#define __S6E3FA7_AOD_PANEL_H__

#include "s6e3fa7_aod.h"
#include "s6e3fa7_self_icon_img.h"
#include "s6e3fa7_analog_clock_img.h"
#include "s6e3fa7_digital_clock_img.h"

static u8 S6E3FA7_AOD_KEY1_ENABLE[] = { 0x9F, 0xA5, 0xA5 };
static u8 S6E3FA7_AOD_KEY1_DISABLE[] = { 0x9F, 0x5A, 0x5A };

static u8 S6E3FA7_AOD_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 S6E3FA7_AOD_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };

static u8 S6E3FA7_AOD_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };
static u8 S6E3FA7_AOD_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };

static DEFINE_STATIC_PACKET(s6e3fa7_aod_level1_key_enable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_KEY1_ENABLE, 0);
static DEFINE_STATIC_PACKET(s6e3fa7_aod_level1_key_disable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_KEY1_DISABLE, 0);

static DEFINE_STATIC_PACKET(s6e3fa7_aod_level2_key_enable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(s6e3fa7_aod_level2_key_disable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_KEY2_DISABLE, 0);

static DEFINE_STATIC_PACKET(s6e3fa7_aod_level3_key_enable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_KEY3_ENABLE, 0);
static DEFINE_STATIC_PACKET(s6e3fa7_aod_level3_key_disable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_KEY3_DISABLE, 0);

static DEFINE_PANEL_MDELAY(s6e3fa7_aod_move_off_delay, 17);
static DEFINE_PANEL_UDELAY(s6e3fa7_aod_self_mask_checksum_1frame_delay, 16700);
static DEFINE_PANEL_UDELAY(s6e3fa7_aod_self_mask_checksum_2frame_delay, 33400);
static DEFINE_PANEL_MDELAY(s6e3fa7_aod_set_timer_delay, 34);

static DEFINE_PANEL_KEY(s6e3fa7_aod_level1_key_enable, CMD_LEVEL_1,
	KEY_ENABLE, &PKTINFO(s6e3fa7_aod_level1_key_enable));
static DEFINE_PANEL_KEY(s6e3fa7_aod_level1_key_disable, CMD_LEVEL_1,
	KEY_DISABLE, &PKTINFO(s6e3fa7_aod_level1_key_disable));

static DEFINE_PANEL_KEY(s6e3fa7_aod_level2_key_enable, CMD_LEVEL_2,
	KEY_ENABLE, &PKTINFO(s6e3fa7_aod_level2_key_enable));
static DEFINE_PANEL_KEY(s6e3fa7_aod_level2_key_disable, CMD_LEVEL_2,
	KEY_DISABLE, &PKTINFO(s6e3fa7_aod_level2_key_disable));

static DEFINE_PANEL_KEY(s6e3fa7_aod_level3_key_enable, CMD_LEVEL_3,
	KEY_ENABLE, &PKTINFO(s6e3fa7_aod_level3_key_enable));
static DEFINE_PANEL_KEY(s6e3fa7_aod_level3_key_disable, CMD_LEVEL_3,
	KEY_DISABLE, &PKTINFO(s6e3fa7_aod_level3_key_disable));

static struct keyinfo KEYINFO(s6e3fa7_aod_level1_key_enable);
static struct keyinfo KEYINFO(s6e3fa7_aod_level1_key_disable);
static struct keyinfo KEYINFO(s6e3fa7_aod_level2_key_enable);
static struct keyinfo KEYINFO(s6e3fa7_aod_level2_key_disable);
static struct keyinfo KEYINFO(s6e3fa7_aod_level3_key_enable);
static struct keyinfo KEYINFO(s6e3fa7_aod_level3_key_disable);

static char s6e3fa7_aod_self_move_pos_tbl[][90] = {
	{
		0x22, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x77, 0x23, 0x27, 0x23,
		0x77, 0x73, 0x27, 0x33, 0x17, 0x73, 0x77, 0x23,
		0x27, 0x23, 0x77, 0x73, 0x27, 0x33, 0x17, 0x73,
		0x77, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x44, 0x44, 0x1C, 0xCC,
		0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44,
		0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3,
		0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C,
		0xCC, 0xC3
	},
	{
		0x55, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x77, 0x23, 0x27, 0x23,
		0x77, 0x73, 0x27, 0x33, 0x17, 0x73, 0x77, 0x23,
		0x27, 0x23, 0x77, 0x73, 0x27, 0x33, 0x17, 0x73,
		0x77, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x44, 0x44, 0x1C, 0xCC,
		0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44,
		0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3,
		0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C,
		0xCC, 0xC3
	},
	{
		0xDD, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x77, 0x23, 0x27, 0x23,
		0x77, 0x73, 0x27, 0x33, 0x17, 0x73, 0x77, 0x23,
		0x27, 0x23, 0x77, 0x73, 0x27, 0x33, 0x17, 0x73,
		0x77, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x44, 0x44, 0x1C, 0xCC,
		0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44,
		0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3,
		0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C,
		0xCC, 0xC3
	},
	{
		0xDD, 0x00, 0x23, 0x00, 0x27, 0x00, 0x23, 0x00,
		0x77, 0x00, 0x73, 0x00, 0x27, 0x00, 0x33, 0x00,
		0x17, 0x00, 0x73, 0x00, 0x77, 0x00, 0x23, 0x00,
		0x27, 0x00, 0x23, 0x00, 0x77, 0x00, 0x73, 0x00,
		0x27, 0x00, 0x33, 0x00, 0x17, 0x00, 0x73, 0x00,
		0x77, 0x00, 0x23, 0x00, 0x27, 0x00, 0x23, 0x00,
		0x77, 0x00, 0x73, 0x00, 0x27, 0x00, 0x33, 0x00,
		0x17, 0x00, 0x73, 0x00, 0x04, 0x04, 0x04, 0x04,
		0x01, 0x0C, 0x0C, 0x0C, 0x0C, 0x03, 0x04, 0x04,
		0x04, 0x04, 0x01, 0x0C, 0x0C, 0x0C, 0x0C, 0x03,
		0x04, 0x04, 0x04, 0x04, 0x01, 0x0C, 0x0C, 0x0C,
		0x0C, 0x03
	},
	{
		0x00, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x77, 0x23, 0x27, 0x23,
		0x77, 0x73, 0x27, 0x33, 0x17, 0x73, 0x77, 0x23,
		0x27, 0x23, 0x77, 0x73, 0x27, 0x33, 0x17, 0x73,
		0x77, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
		0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
		0x27, 0x33, 0x17, 0x73, 0x44, 0x44, 0x1C, 0xCC,
		0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44,
		0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3,
		0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C,
		0xCC, 0xC3,
	},
};

#ifdef SUPPORT_NORMAL_SELF_MOVE
static char s6e3fa7_aod_self_move_patt_tbl[][76] = {
	{
		0x7D,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x10,
		0x01, 0x50, 0x05, 0x10, 0x01, 0x50, 0x05, 0x10,
		0x01, 0x50, 0x05, 0x10, 0x01, 0x50, 0x05, 0x10,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x41, 0xC3, 0x41, 0xC3, 0x41, 0xC3,
		0x41, 0xC3, 0x41,

	},
	{
		0x7D,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x10,
		0x01, 0x50, 0x05, 0x10, 0x01, 0x50, 0x05, 0x10,
		0x01, 0x50, 0x05, 0x10, 0x01, 0x50, 0x05, 0x10,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x41, 0xC3, 0x41, 0xC3, 0x41, 0xC3,
		0x41, 0xC3, 0x41,
	},
};
#endif

static struct maptbl s6e3fa7_aod_maptbl[] = {
	[SELF_MASK_CTRL_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_self_mask,
		s6e3fa7_init_self_mask_ctrl, NULL, s6e3fa7_copy_self_mask_ctrl),
	[DIGITAL_POS_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_digital_pos,
		init_common_table, NULL, s6e3fa7_copy_digital_pos),
	[DIGITAL_BLK_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_digital_blink,
		init_common_table, NULL, s6e3fa7_copy_digital_blink),
	[SET_TIME_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_set_time,
		init_common_table, NULL, s6e3fa7_copy_set_time_ctrl),
	[ICON_GRID_ON_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_icon_grid_on,
		init_common_table, NULL, s6e3fa7_copy_icon_grid_on_ctrl),
	[SELF_MOVE_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_s3_sa_self_move_on,
		init_common_table, NULL, s6e3fa7_copy_self_move_on_ctrl),
	[ANALOG_POS_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_analog_pos,
		init_common_table, NULL, s6e3fa7_copy_analog_pos_ctrl),
	[ANALOG_CLK_CTRL_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_analog_clock,
		init_common_table, NULL, s6e3fa7_copy_analog_clock_ctrl),
	[DIGITAL_CLK_CTRL_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_digital_clock,
		init_common_table, NULL, s6e3fa7_copy_digital_clock_ctrl),
	[SELF_MOVE_POS_MAPTBL] = DEFINE_2D_MAPTBL(s6e3fa7_aod_self_move_pos_tbl,
		init_common_table, s6e3fa7_getidx_self_mode_pos, copy_common_maptbl),
	[SELF_MOVE_RESET_MAPTBL] = DEFINE_0D_MAPTBL(s6e3fa7_aod_self_move_reset,
		init_common_table, NULL, s6e3fa7_copy_self_move_reset),
#ifdef SUPPORT_NORMAL_SELF_MOVE
	[SELF_MOVE_PATTERN_MAPTBL] = DEFINE_2D_MAPTBL(s6e3fa7_aod_self_move_patt_tbl,
		init_common_table, s6e3fa7_getidx_self_pattern, s6e3fa7_copy_self_move_pattern),
#endif
};

// --------------------- Image for self mask image ---------------------
static char S6E3FA7_AOD_SELF_MASK_SD_PATH[] = {
	0x75,
	0x10,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_sd_path, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_SD_PATH, 0);

// --------------------- End of self mask image ---------------------


// --------------------- Image for self mask control ---------------------
static char S6E3FA7_AOD_SELF_MASK_ENA[] = {
#ifdef CONFIG_SELFMASK_FACTORY
	0x7A,
	0x01, 0x08, 0xE8, 0x09, 0x7D,
	0x09, 0x7E, 0x0A, 0x13, 0x08,
	0x10, 0x00, 0x00, 0x00, 0x00
#else
	0x7A,
	0x01, 0x00, 0x00, 0x00, 0x95,
	0x08, 0x52, 0x08, 0xE7, 0x08,
	0x10, 0x00, 0x00, 0x00, 0x00
#endif
};

static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_ctrl_ena, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_ENA, 0);

static void *s6e3fa7_aod_self_mask_ena_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_self_mask_ctrl_ena),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

static char S6E3FA7_AOD_SELF_MASK_DISABLE[] = {
	0x7A,
	0x00,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_disable, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_DISABLE, 0);

static void *s6e3fa7_aod_self_mask_dis_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_self_mask_disable),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};
// --------------------- End of self mask control ---------------------



// --------------------- Image for self icon ---------------------
static char S6E3FA7_AOD_SELF_ICON_SD_PATH[] = {
	0x75,
	0x08,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_icon_sd_path, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_ICON_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_icon_img, DSI_PKT_TYPE_WR_SR, S6E3FA7_SELF_ICON_IMG, 0);
static void *s6e3fa7_aod_self_icon_img_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_self_icon_sd_path),
	&PKTINFO(s6e3fa7_aod_self_icon_img),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};
// --------------------- End of self icon ---------------------


// --------------------- Control for self icon ---------------------
static char S6E3FA7_AOD_ICON_GRID_ON[] = {
	0x7C,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1f, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x1E
};
static DEFINE_PKTUI(s6e3fa7_aod_icon_grid_on, &s6e3fa7_aod_maptbl[ICON_GRID_ON_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_aod_icon_grid_on, DSI_PKT_TYPE_WR, S6E3FA7_AOD_ICON_GRID_ON, 0);
static void *s6e3fa7_aod_icon_grid_on_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_icon_grid_on),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

static char S6E3FA7_AOD_ICON_GRID_OFF[] = {
	0x7C,
	0x00, 0x00,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_icon_grid_off, DSI_PKT_TYPE_WR, S6E3FA7_AOD_ICON_GRID_OFF, 0);

static void *s6e3fa7_aod_icon_grid_off_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_icon_grid_off),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

// --------------------- End of self icon control ---------------------

// --------------------- check sum control ----------------------------
static char S6E3FA7_AOD_SELF_MASK_CRC_ON1[] = {
	0xFE,
	0x80,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_crc_on1, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_CRC_ON1, 0x1B);

static char S6E3FA7_AOD_SELF_MASK_CRC_ON2[] = {
	0xFE,
	0x25,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_crc_on2, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_CRC_ON2, 0x2F);

static char S6E3FA7_AOD_SELF_MASK_DBIST_ON[] = {
	0xBF,
	0x01, 0x07, 0x00, 0x00, 0x00, 0x10
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_dbist_on, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_DBIST_ON, 0);

static char S6E3FA7_AOD_SELF_MASK_DBIST_OFF[] = {
	0xBF,
	0x00, 0x07, 0xFF, 0x00, 0x00, 0x10
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_dbist_off, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_DBIST_OFF, 0);

static char S6E3FA7_AOD_SELF_MASK_ENABLE_FOR_CHECKSUM[] = {
	0x7A,
	0x01, 0x00, 0x00, 0x00, 0x95,
	0x08, 0x52, 0x08, 0xE7, 0x08,
	0x10, 0xFF, 0xFF, 0xFF, 0xFF
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_for_checksum, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_ENABLE_FOR_CHECKSUM, 0);

static char S6E3FA7_AOD_SELF_MASK_RESTORE[] = {
	0x7A,
	0x01, 0x08, 0xE8, 0x09, 0x7D,
	0x09, 0x7E, 0x0A, 0x13, 0x08,
	0x10, 0x00, 0x00, 0x00, 0x00
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_self_mask_restore, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MASK_RESTORE, 0);

// --------------------- end of check sum control ----------------------------


static char S6E3FA7_AOD_INIT_SIDE_RAM[] = {
	0x77,
	0x00, 0x00,
};

static DEFINE_STATIC_PACKET(s6e3fa7_aod_init_side_ram, DSI_PKT_TYPE_WR, S6E3FA7_AOD_INIT_SIDE_RAM, 0);


// --------------------- Image for analog clock ---------------------
static char S6E3FA7_AOD_ANALOG_SD_PATH[] = {
	0x75,
	0x01,
};
static DEFINE_STATIC_PACKET(s6e3fa7_aod_analog_sd_path, DSI_PKT_TYPE_WR, S6E3FA7_AOD_ANALOG_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3fa7_aod_analog_img, DSI_PKT_TYPE_WR_SR, S6E3FA7_ANALOG_IMG, 0);

static void *s6e3fa7_aod_analog_img_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_init_side_ram),
	&PKTINFO(s6e3fa7_aod_analog_sd_path),
	&PKTINFO(s6e3fa7_aod_analog_img),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};


static char S6E3FA7_AOD_ANALOG_POS[] = {
	0x77,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x5A, 0x14, 0x5A, 0x14, 0x5A, 0x14
};

static DEFINE_PKTUI(s6e3fa7_aod_analog_pos, &s6e3fa7_aod_maptbl[ANALOG_POS_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_aod_analog_pos, DSI_PKT_TYPE_WR, S6E3FA7_AOD_ANALOG_POS, 9);

static char S6E3FA7_AOD_CLOCK_CTRL[] = {
	0x77,
	0x00, 0x00, 0x00
};

static DEFINE_PKTUI(s6e3fa7_aod_analog_clock, &s6e3fa7_aod_maptbl[ANALOG_CLK_CTRL_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_aod_analog_clock, DSI_PKT_TYPE_WR, S6E3FA7_AOD_CLOCK_CTRL, 0);

static void *s6e3fa7_aod_analog_ctrl_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_analog_pos),
	&PKTINFO(s6e3fa7_aod_analog_clock),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

// --------------------- End of analog clock ---------------------



// --------------------- Image for digital clock ---------------------
static char S6E3FA7_AOD_DIGITAL_BLINK[] = {
	0x79,
	0x00, 0xC0, 0x00, 0x00, 0x00,
	0x00, 0x57, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1D,
	0x1D
};

static DEFINE_PKTUI(s6e3fa7_aod_digital_blink, &s6e3fa7_aod_maptbl[DIGITAL_BLK_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_aod_digital_blink, DSI_PKT_TYPE_WR, S6E3FA7_AOD_DIGITAL_BLINK, 0);

static char S6E3FA7_AOD_DIGITAL_BLINK_OFF[] = {
	0x79,
	0x00, 0x00,
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_digital_blink_off, DSI_PKT_TYPE_WR, S6E3FA7_AOD_DIGITAL_BLINK_OFF, 0);


static char S6E3FA7_AOD_DIGITAL_POS[] = {
	0x77,
	0x01, 0x2C, 0x01, 0x90,
	0x01, 0xF4, 0x01, 0x90,
	0x01, 0x2C, 0x02, 0xF4,
	0x02, 0xF4, 0x01, 0xF4,
	0x00, 0xC8,
	0x01, 0x64
};
static DEFINE_PKTUI(s6e3fa7_aod_digital_pos, &s6e3fa7_aod_maptbl[DIGITAL_POS_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_aod_digital_pos, DSI_PKT_TYPE_WR, S6E3FA7_AOD_DIGITAL_POS, 21);


static DEFINE_PKTUI(s6e3fa7_aod_digital_clock, &s6e3fa7_aod_maptbl[DIGITAL_CLK_CTRL_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_aod_digital_clock, DSI_PKT_TYPE_WR, S6E3FA7_AOD_CLOCK_CTRL, 0);

static void *s6e3fa7_aod_digital_ctrl_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_digital_pos),
	&PKTINFO(s6e3fa7_aod_digital_blink),
	&PKTINFO(s6e3fa7_aod_digital_clock),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

static char S6E3FA7_AOD_DIGITAL_SD_PATH[] = {
	0x75,
	0x02,
};

static DEFINE_STATIC_PACKET(s6e3fa7_aod_digital_sd_path, DSI_PKT_TYPE_WR, S6E3FA7_AOD_DIGITAL_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3fa7_aod_digital_img, DSI_PKT_TYPE_WR_SR, S6E3FA7_DIGITAL_IMG, 0);

static void *s6e3fa7_aod_digital_img_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_init_side_ram),
	&PKTINFO(s6e3fa7_aod_digital_sd_path),
	&PKTINFO(s6e3fa7_aod_digital_img),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};
// --------------------- End of digital clock ---------------------


/* -------------------------- Value for timer */
static char S6E3FA7_AOD_SET_TIME[] = {
	0x77,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1e, 0x13, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};
DEFINE_PKTUI(s6e3fa7_aod_set_time, &s6e3fa7_aod_maptbl[SET_TIME_MAPTBL], 0);
DEFINE_VARIABLE_PACKET(s6e3fa7_aod_set_time, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SET_TIME, 0);

static char S6E3FA7_AOD_UPDATE_TIME_CTRL[] = {
	0x78,
	0x01,
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_update_time_ctrl, DSI_PKT_TYPE_WR, S6E3FA7_AOD_UPDATE_TIME_CTRL, 0);

static void *s6e3fa7_aod_set_time_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_set_time),
	&PKTINFO(s6e3fa7_aod_update_time_ctrl),
	&DLYINFO(s6e3fa7_aod_set_timer_delay),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

static char S6E3FA7_AOD_TIMER_OFF_CTRL[] = {
	0x77,
	0x00, 0x00,
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_timer_off_ctrl, DSI_PKT_TYPE_WR, S6E3FA7_AOD_TIMER_OFF_CTRL, 0);
/* -------------------------- end of timer */

static char S6E3FA7_AOD_MOVE_SETTING[] = {
	0xFE,
	0x24
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_move_setting, DSI_PKT_TYPE_WR, S6E3FA7_AOD_MOVE_SETTING, 0x33);

/* setting for aod porch */
char S6E3FA7_AOD_AOD_SETTING[] = {
	0x76,
	0x08, 0x10, 0x84, 0x38,
	0x08, 0xE8
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_aod_setting, DSI_PKT_TYPE_WR, S6E3FA7_AOD_AOD_SETTING, 0);

static void *s6e3fa7_aod_enter_aod_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level3_key_enable),
	&PKTINFO(s6e3fa7_aod_move_setting),
	&PKTINFO(s6e3fa7_aod_aod_setting),
	&KEYINFO(s6e3fa7_aod_level3_key_disable),
};

char S6E3FA7_AOD_NORMAL_SETTING[] = {
	0x76,
	0x08, 0x10, 0x04, 0x38, 0x08, 0xE8
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_normal_setting, DSI_PKT_TYPE_WR, S6E3FA7_AOD_NORMAL_SETTING, 0);


static void *s6e3fa7_aod_exit_aod_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_normal_setting),
	&PKTINFO(s6e3fa7_aod_timer_off_ctrl),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};


/* -------------------------- Setting for self move --------------------------*/

static char S6E3FA7_AOD_SELF_MOVE_ON[] = {
	0x7D,
	0x00,
	0x00,
	0x00, 0x00,
	0x00, 0x00,
};

static DEFINE_PKTUI(s6e3fa7_s3_sa_self_move_on, &s6e3fa7_aod_maptbl[SELF_MOVE_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3fa7_s3_sa_self_move_on, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MOVE_ON, 0);

char S6E3FA7_AOD_SELF_MOVE_POS[] = {
	0x7D,
	0x00, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
	0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
	0x27, 0x33, 0x17, 0x73, 0x77, 0x23, 0x27, 0x23,
	0x77, 0x73, 0x27, 0x33, 0x17, 0x73, 0x77, 0x23,
	0x27, 0x23, 0x77, 0x73, 0x27, 0x33, 0x17, 0x73,
	0x77, 0x23, 0x27, 0x23, 0x77, 0x73, 0x27, 0x33,
	0x17, 0x73, 0x77, 0x23, 0x27, 0x23, 0x77, 0x73,
	0x27, 0x33, 0x17, 0x73, 0x44, 0x44, 0x1C, 0xCC,
	0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44,
	0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C, 0xCC, 0xC3,
	0x44, 0x44, 0x1C, 0xCC, 0xC3, 0x44, 0x44, 0x1C,
	0xCC, 0xC3,
};

static DEFINE_PKTUI(s6e3fa7_s3_sa_self_move_pos, &s6e3fa7_aod_maptbl[SELF_MOVE_POS_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(s6e3fa7_s3_sa_self_move_pos, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MOVE_POS, 0x6);

static void *s6e3fa7_aod_self_move_on_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_s3_sa_self_move_pos),
	&PKTINFO(s6e3fa7_s3_sa_self_move_on),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

static char S6E3FA7_AOD_SELF_MOVE_SYNC_OFF[] = {
	0x7D,
	0x01,
};
DEFINE_STATIC_PACKET(s6e3fa7_aod_self_move_sync_off, DSI_PKT_TYPE_WR, S6E3FA7_AOD_SELF_MOVE_SYNC_OFF, 0);

static void *s6e3fa7_aod_self_move_off_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_self_move_sync_off),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

static void *s6e3fa7_aod_self_move_reset_cmdtbl[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_aod_self_move_sync_off),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};

#ifdef SUPPORT_NORMAL_SELF_MOVE
char S6E3FA7_DEF_MOVE_PATTERN[] = {
	0x7D,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55,
	0x10, 0x01, 0x50, 0x05, 0x10, 0x01, 0x50, 0x05,
	0x10, 0x01, 0x50, 0x05, 0x10, 0x01, 0x50, 0x05,
	0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x41, 0xC3, 0x41, 0xC3, 0x41,
	0xC3, 0x41, 0xC3, 0x41,
};

DEFINE_PKTUI(s6e3fa7_self_move_pattern, &s6e3fa7_aod_maptbl[SELF_MOVE_PATTERN_MAPTBL], 0);
DEFINE_VARIABLE_PACKET(s6e3fa7_self_move_pattern, DSI_PKT_TYPE_WR, S6E3FA7_DEF_MOVE_PATTERN, 0);

static char S6E3FA7_SELF_TIMER_ON[] = {
	0x77,
#ifdef FAST_TIMER
	0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x03, 0x00
#else
	0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00
#endif
};

DEFINE_STATIC_PACKET(s6e3fa7_self_timer_on, DSI_PKT_TYPE_WR, S6E3FA7_SELF_TIMER_ON, 0);

static char S6E3FA7_SELF_MOVE_SET[] = {
	0x7C,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00
};

DEFINE_STATIC_PACKET(s6e3fa7_self_move_set, DSI_PKT_TYPE_WR, S6E3FA7_SELF_MOVE_SET, 0);

static char S6E3FA7_SELF_MOVE_TIMER_OFF[] = {
	0x77,
	0x00, 0x00
};
DEFINE_STATIC_PACKET(s6e3fa7_self_move_timer_off, DSI_PKT_TYPE_WR, S6E3FA7_SELF_MOVE_TIMER_OFF, 0);

static void *s6e3fa7_enable_self_move[] = {
	&KEYINFO(s6e3fa7_aod_level3_key_enable),
	&KEYINFO(s6e3fa7_aod_level2_key_enable),

	&PKTINFO(s6e3fa7_aod_move_setting),
	&PKTINFO(s6e3fa7_aod_aod_setting),

	&PKTINFO(s6e3fa7_self_timer_on),
	&PKTINFO(s6e3fa7_self_move_set),

	&PKTINFO(s6e3fa7_self_move_timer_off),
	&DLYINFO(s6e3fa7_aod_move_off_delay),
	&PKTINFO(s6e3fa7_self_timer_on),

	&PKTINFO(s6e3fa7_self_move_pattern),

	&KEYINFO(s6e3fa7_aod_level2_key_disable),
	&KEYINFO(s6e3fa7_aod_level3_key_disable),
};

static void *s6e3fa7_disable_self_move[] = {
	&KEYINFO(s6e3fa7_aod_level2_key_enable),
	&PKTINFO(s6e3fa7_self_move_timer_off),
	&DLYINFO(s6e3fa7_aod_move_off_delay),
	&PKTINFO(s6e3fa7_aod_self_move_sync_off),
	&PKTINFO(s6e3fa7_aod_normal_setting),
	&KEYINFO(s6e3fa7_aod_level2_key_disable),
};
#endif

/* -------------------------- End self move --------------------------*/
#endif //__S6E3FA7_AOD_PANEL_H__
