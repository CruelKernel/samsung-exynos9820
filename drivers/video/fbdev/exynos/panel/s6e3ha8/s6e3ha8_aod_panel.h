/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3ha8/s6e3ha8_aod_panel.h
 *
 * Header file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA8_AOD_PANEL_H__
#define __S6E3HA8_AOD_PANEL_H__

#include "s6e3ha8_aod.h"
#include "s6e3ha8_self_mask_img.h"
#include "s6e3ha8_self_icon_img.h"
#include "s6e3ha8_analog_clock_img.h"
#include "s6e3ha8_digital_clock_img.h"

static u8 S6E3HA8_AOD_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 S6E3HA8_AOD_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };

static u8 S6E3HA8_AOD_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };
static u8 S6E3HA8_AOD_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };

static DEFINE_STATIC_PACKET(s6e3ha8_aod_level2_key_enable, DSI_PKT_TYPE_WR, S6E3HA8_AOD_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(s6e3ha8_aod_level2_key_disable, DSI_PKT_TYPE_WR, S6E3HA8_AOD_KEY2_DISABLE, 0);

static DEFINE_STATIC_PACKET(s6e3ha8_aod_level3_key_enable, DSI_PKT_TYPE_WR, S6E3HA8_AOD_KEY3_ENABLE, 0);
static DEFINE_STATIC_PACKET(s6e3ha8_aod_level3_key_disable, DSI_PKT_TYPE_WR, S6E3HA8_AOD_KEY3_DISABLE, 0);

static DEFINE_PANEL_KEY(s6e3ha8_aod_level2_key_enable, CMD_LEVEL_2,
	KEY_ENABLE, &PKTINFO(s6e3ha8_aod_level2_key_enable));
static DEFINE_PANEL_KEY(s6e3ha8_aod_level2_key_disable, CMD_LEVEL_2,
	KEY_DISABLE, &PKTINFO(s6e3ha8_aod_level2_key_disable));

static DEFINE_PANEL_KEY(s6e3ha8_aod_level3_key_enable, CMD_LEVEL_3,
	KEY_ENABLE, &PKTINFO(s6e3ha8_aod_level3_key_enable));
static DEFINE_PANEL_KEY(s6e3ha8_aod_level3_key_disable, CMD_LEVEL_3,
	KEY_DISABLE, &PKTINFO(s6e3ha8_aod_level3_key_disable));

static struct keyinfo KEYINFO(s6e3ha8_aod_level2_key_enable);
static struct keyinfo KEYINFO(s6e3ha8_aod_level2_key_disable);
static struct keyinfo KEYINFO(s6e3ha8_aod_level3_key_enable);
static struct keyinfo KEYINFO(s6e3ha8_aod_level3_key_disable);


static char s6e3ha8_aod_self_move_pos_tbl[][90] = {
	{
		0x22,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3,	0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3
	},
	{
		0x55,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3
	},
	{
		0xdd,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3,
	},
	{
		0xdd,
		0x00, 0x10, 0x00, 0x10, 0x00,
		0x10, 0x00, 0x10, 0x00, 0x01,
		0x00, 0x50, 0x00, 0x50, 0x00,
		0x50, 0x00, 0x50, 0x00, 0x05,
		0x00, 0x10, 0x00, 0x10, 0x00,
		0x10, 0x00, 0x10, 0x00, 0x01,
		0x00, 0x50, 0x00, 0x50, 0x00,
		0x50, 0x00, 0x50, 0x00, 0x05,
		0x00, 0x10, 0x00, 0x10, 0x00,
		0x10, 0x00, 0x10, 0x00, 0x01,
		0x00, 0x50, 0x00, 0x50, 0x00,
		0x50, 0x00, 0x50, 0x00, 0x04,
		0x04, 0x04, 0x04, 0x01, 0x0C,
		0x0C, 0x0C, 0x0C, 0x03, 0x04,
		0x04, 0x04, 0x04, 0x01, 0x0C,
		0x0C, 0x0C, 0x0C, 0x03, 0x04,
		0x04, 0x04, 0x04, 0x01, 0x0C,
		0x0C, 0x0C, 0x0C, 0x03,
	},
	{
		0x00,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x05,
		0x10, 0x10, 0x10, 0x10, 0x01,
		0x50, 0x50, 0x50, 0x50, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3, 0x44,
		0x44, 0x1C, 0xCC, 0xC3,
	},
};


static struct maptbl s6e3ha8_aod_maptbl[] = {
	[SELF_MASK_CTRL_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_self_mask,
		init_common_table, NULL, s6e3ha8_copy_self_mask_ctrl),
	[DIGITAL_POS_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_digital_pos,
		init_common_table, NULL, s6e3ha8_copy_digital_pos),
	[DIGITAL_BLK_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_digital_blink,
		init_common_table, NULL, s6e3ha8_copy_digital_blink),
	[SET_TIME_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_set_time,
		init_common_table, NULL, s6e3ha8_copy_set_time_ctrl),
	[ICON_GRID_ON_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_icon_grid_on,
		init_common_table, NULL, s6e3ha8_copy_icon_grid_on_ctrl),
	[SELF_MOVE_MAPTBL] = DEFINE_0D_MAPTBL(star2_s3_sa_self_move_on,
		init_common_table, NULL, s6e3ha8_copy_self_move_on_ctrl),
	[ANALOG_POS_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_analog_pos,
		init_common_table, NULL, s6e3ha8_copy_analog_pos_ctrl),
	[ANALOG_CLK_CTRL_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_analog_clock,
		init_common_table, NULL, s6e3ha8_copy_analog_clock_ctrl),
	[DIGITAL_CLK_CTRL_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_digital_clock,
		init_common_table, NULL, s6e3ha8_copy_digital_clock_ctrl),
	[SELF_MOVE_POS_MAPTBL] = DEFINE_2D_MAPTBL(s6e3ha8_aod_self_move_pos_tbl,
		init_common_table, s6e3ha8_getidx_self_mode_pos, copy_common_maptbl),
	[SELF_MOVE_RESET_MAPTBL] = DEFINE_0D_MAPTBL(s6e3ha8_aod_self_move_reset,
		init_common_table, NULL, s6e3ha8_copy_self_move_reset),
};

// --------------------- Image for self mask image ---------------------
static char S6E3HA8_AOD_SELF_MASK_SD_PATH[] = {
	0x75,
	0x10,
};
static DEFINE_STATIC_PACKET(s6e3ha8_aod_self_mask_sd_path, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MASK_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3ha8_aod_self_mask_img_pkt, DSI_PKT_TYPE_WR_SR, STAR_SELF_MASK_IMG, 0);
static void *s6e3ha8_aod_self_mask_img_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_self_mask_sd_path),
	&PKTINFO(s6e3ha8_aod_self_mask_img_pkt),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};
// --------------------- End of self mask image ---------------------


// --------------------- Image for self mask control ---------------------
static char S6E3HA8_AOD_SELF_MASK_ENA[] = {
#ifdef CONFIG_SELFMASK_FACTORY
	0x7A,
	0x01, 0x0B, 0x90, 0x0C,
	0x25, 0x0C, 0x26, 0x0C,
	0xBB, 0x09, 0x0F, 0x00,
	0x00, 0x00, 0x00
#else
	0x7A,
	0x01, 0x00, 0x00, 0x00,
	0x95, 0x0a, 0xfa, 0x0b,
	0x8f, 0x09, 0x0f, 0x00,
	0x00, 0x00, 0x00
#endif
};

#ifdef AOD_TEST
static DEFINE_PKTUI(s6e3ha8_aod_self_mask_ctrl_pkt, &s6e3ha8_aod_maptbl[SELF_MASK_CTRL_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_self_mask_ctrl_pkt, DSI_PKT_TYPE_COMP, S6E3HA8_AOD_SELF_MASK_CTRL, 0);
#else
static DEFINE_STATIC_PACKET(s6e3ha8_aod_self_mask_ctrl_ena, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MASK_ENA, 0);
#endif
static void *s6e3ha8_aod_self_mask_ena_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_self_mask_ctrl_ena),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

static char S6E3HA8_AOD_SELF_MASK_DISABLE[] = {
	0x7A,
	0x00,
};
static DEFINE_STATIC_PACKET(s6e3ha8_aod_self_mask_disable, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MASK_DISABLE, 0);

static void *s6e3ha8_aod_self_mask_dis_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_self_mask_disable),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};
// --------------------- End of self mask control ---------------------



// --------------------- Image for self icon ---------------------
static char S6E3HA8_AOD_SELF_ICON_SD_PATH[] = {
	0x75,
	0x08,
};
static DEFINE_STATIC_PACKET(s6e3ha8_aod_self_icon_sd_path, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_ICON_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3ha8_aod_self_icon_img, DSI_PKT_TYPE_WR_SR, STAR_SELF_ICON_IMG, 0);
static void *s6e3ha8_aod_self_icon_img_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_self_icon_sd_path),
	&PKTINFO(s6e3ha8_aod_self_icon_img),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};
// --------------------- End of self icon ---------------------


// --------------------- Control for self icon ---------------------
static char S6E3HA8_AOD_ICON_GRID_ON[] = {
	0x7C,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1f, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x1E
};
static DEFINE_PKTUI(s6e3ha8_aod_icon_grid_on, &s6e3ha8_aod_maptbl[ICON_GRID_ON_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_icon_grid_on, DSI_PKT_TYPE_WR, S6E3HA8_AOD_ICON_GRID_ON, 0);
static void *s6e3ha8_aod_icon_grid_on_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_icon_grid_on),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

static char S6E3HA8_AOD_ICON_GRID_OFF[] = {
	0x7C,
	0x00, 0x00,
};
static DEFINE_STATIC_PACKET(s6e3ha8_aod_icon_grid_off, DSI_PKT_TYPE_WR, S6E3HA8_AOD_ICON_GRID_OFF, 0);

static void *s6e3ha8_aod_icon_grid_off_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_icon_grid_off),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

// --------------------- End of self icon control ---------------------


static char S6E3HA8_AOD_INIT_SIDE_RAM[] = {
	0x77,
	0x00, 0x00,
};

static DEFINE_STATIC_PACKET(s6e3ha8_aod_init_side_ram, DSI_PKT_TYPE_WR, S6E3HA8_AOD_INIT_SIDE_RAM, 0);


// --------------------- Image for analog clock ---------------------
static char S6E3HA8_AOD_ANALOG_SD_PATH[] = {
	0x75,
	0x01,
};
static DEFINE_STATIC_PACKET(s6e3ha8_aod_analog_sd_path, DSI_PKT_TYPE_WR, S6E3HA8_AOD_ANALOG_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3ha8_aod_analog_img, DSI_PKT_TYPE_WR_SR, STAR_ANALOG_IMG, 0);

static void *s6e3ha8_aod_analog_img_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_init_side_ram),
	&PKTINFO(s6e3ha8_aod_analog_sd_path),
	&PKTINFO(s6e3ha8_aod_analog_img),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};


static char S6E3HA8_AOD_ANALOG_POS[] = {
	0x77,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x78, 0x13, 0x78, 0x13, 0x78, 0x13
};

static DEFINE_PKTUI(s6e3ha8_aod_analog_pos, &s6e3ha8_aod_maptbl[ANALOG_POS_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_analog_pos, DSI_PKT_TYPE_WR, S6E3HA8_AOD_ANALOG_POS, 9);

static char S6E3HA8_AOD_CLOCK_CTRL[] = {
	0x77,
	0x00, 0x00, 0x00
};

static DEFINE_PKTUI(s6e3ha8_aod_analog_clock, &s6e3ha8_aod_maptbl[ANALOG_CLK_CTRL_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_analog_clock, DSI_PKT_TYPE_WR, S6E3HA8_AOD_CLOCK_CTRL, 0);

static void *s6e3ha8_aod_analog_ctrl_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_analog_pos),
	&PKTINFO(s6e3ha8_aod_analog_clock),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

// --------------------- End of analog clock ---------------------



// --------------------- Image for digital clock ---------------------
static char S6E3HA8_AOD_DIGITAL_BLINK[] = {
	0x79,
	0x00, 0xC0, 0x00, 0x00, 0x00,
	0x00, 0x57, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x1D,
	0x1D
};

static DEFINE_PKTUI(s6e3ha8_aod_digital_blink, &s6e3ha8_aod_maptbl[DIGITAL_BLK_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_digital_blink, DSI_PKT_TYPE_WR, S6E3HA8_AOD_DIGITAL_BLINK, 0);

static char S6E3HA8_AOD_DIGITAL_BLINK_OFF[] = {
	0x79,
	0x00, 0x00,
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_digital_blink_off, DSI_PKT_TYPE_WR, S6E3HA8_AOD_DIGITAL_BLINK_OFF, 0);


static char S6E3HA8_AOD_DIGITAL_POS[] = {
	0x77,
	0x01, 0x2C, 0x01, 0x90,
	0x01, 0xF4, 0x01, 0x90,
	0x01, 0x2C, 0x02, 0xF4,
	0x02, 0xF4, 0x01, 0xF4,
	0x00, 0xC8,
	0x01, 0x64
};
static DEFINE_PKTUI(s6e3ha8_aod_digital_pos, &s6e3ha8_aod_maptbl[DIGITAL_POS_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_digital_pos, DSI_PKT_TYPE_WR, S6E3HA8_AOD_DIGITAL_POS, 21);

#if 0
static char S6E3HA8_AOD_DIG_INTERVAL[] = {
	0x77,
/*for debug : 0x10, 0x03*/
	0x1E, 0x13,
};
static DEFINE_STATIC_PACKET(s6e3ha8_aod_dig_interval, DSI_PKT_TYPE_WR, S6E3HA8_AOD_DIG_INTERVAL, 7);
#endif


static DEFINE_PKTUI(s6e3ha8_aod_digital_clock, &s6e3ha8_aod_maptbl[DIGITAL_CLK_CTRL_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_digital_clock, DSI_PKT_TYPE_WR, S6E3HA8_AOD_CLOCK_CTRL, 0);

static void *s6e3ha8_aod_digital_ctrl_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_digital_pos),
	&PKTINFO(s6e3ha8_aod_digital_blink),
	&PKTINFO(s6e3ha8_aod_digital_clock),
	//&PKTINFO(s6e3ha8_aod_dig_interval),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

static char S6E3HA8_AOD_DIGITAL_SD_PATH[] = {
	0x75,
	0x02,
};

static DEFINE_STATIC_PACKET(s6e3ha8_aod_digital_sd_path, DSI_PKT_TYPE_WR, S6E3HA8_AOD_DIGITAL_SD_PATH, 0);
static DEFINE_STATIC_PACKET(s6e3ha8_aod_digital_img, DSI_PKT_TYPE_WR_SR, STAR_DIGITAL_IMG, 0);

static void *s6e3ha8_aod_digital_img_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_init_side_ram),
	&PKTINFO(s6e3ha8_aod_digital_sd_path),
	&PKTINFO(s6e3ha8_aod_digital_img),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};
// --------------------- End of digital clock ---------------------


/* -------------------------- Value for timer */
static char S6E3HA8_AOD_SET_TIME[] = {
	0x77,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x1e, 0x13, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};
DEFINE_PKTUI(s6e3ha8_aod_set_time, &s6e3ha8_aod_maptbl[SET_TIME_MAPTBL], 0);
DEFINE_VARIABLE_PACKET(s6e3ha8_aod_set_time, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SET_TIME, 0);

static char S6E3HA8_AOD_UPDATE_TIME_CTRL[] = {
	0x78,
	0x01,
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_update_time_ctrl, DSI_PKT_TYPE_WR, S6E3HA8_AOD_UPDATE_TIME_CTRL, 0);

static void *s6e3ha8_aod_set_time_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_set_time),
	&PKTINFO(s6e3ha8_aod_update_time_ctrl),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

static char S6E3HA8_AOD_TIMER_OFF_CTRL[] = {
	0x77,
	0x00, 0x00,
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_timer_off_ctrl, DSI_PKT_TYPE_WR, S6E3HA8_AOD_TIMER_OFF_CTRL, 0);
/* -------------------------- end of timer */

static char S6E3HA8_AOD_MOVE_SETTING[] = {
	0xFE,
	0x00
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_move_setting, DSI_PKT_TYPE_WR, S6E3HA8_AOD_MOVE_SETTING, 0x15);

/* setting for aod porch */
char S6E3HA8_AOD_AOD_SETTING[] = {
	0x76,
	0x09, 0x0f, 0x85, 0xA0,
	0x0B, 0x90
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_aod_setting, DSI_PKT_TYPE_WR, S6E3HA8_AOD_AOD_SETTING, 0);

static void *s6e3ha8_aod_enter_aod_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level3_key_enable),
	&PKTINFO(s6e3ha8_aod_move_setting),
	&PKTINFO(s6e3ha8_aod_aod_setting),
	&KEYINFO(s6e3ha8_aod_level3_key_disable),
};

char S6E3HA8_AOD_NORMAL_SETTING[] = {
	0x76,
	0x09, 0x0f, 0x00
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_normal_setting, DSI_PKT_TYPE_WR, S6E3HA8_AOD_NORMAL_SETTING, 0);


static void *s6e3ha8_aod_exit_aod_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_normal_setting),
	&PKTINFO(s6e3ha8_aod_timer_off_ctrl),
	&PKTINFO(s6e3ha8_aod_digital_blink_off),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};


/* -------------------------- Setting for self move --------------------------*/

static char S6E3HA8_AOD_SELF_MOVE_ON[] = {
	0x7D,
	0x00,
	0x00,
	0x00, 0x00,
	0x00, 0x00,
};

static DEFINE_PKTUI(star2_s3_sa_self_move_on, &s6e3ha8_aod_maptbl[SELF_MOVE_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(star2_s3_sa_self_move_on, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MOVE_ON, 0);

char S6E3HA8_AOD_SELF_MOVE_POS[] = {
	0x7D,
	0x33,
	0x10, 0x10, 0x10, 0x10, 0x01,
	0x50, 0x50, 0x50, 0x50, 0x05,
	0x10, 0x10, 0x10, 0x10, 0x01,
	0x50, 0x50, 0x50, 0x50, 0x05,
	0x10, 0x10, 0x10, 0x10, 0x01,
	0x50, 0x50, 0x50, 0x50, 0x05,
	0x10, 0x10, 0x10, 0x10, 0x01,
	0x50, 0x50, 0x50, 0x50, 0x05,
	0x10, 0x10, 0x10, 0x10, 0x01,
	0x50, 0x50, 0x50, 0x50, 0x05,
	0x10, 0x10, 0x10, 0x10, 0x01,
	0x50, 0x50, 0x50, 0x50, 0x44,
	0x44, 0x1C, 0xCC, 0xC3, 0x44,
	0x44, 0x1C, 0xCC, 0xC3, 0x44,
	0x44, 0x1C, 0xCC, 0xC3, 0x44,
	0x44, 0x1C, 0xCC, 0xC3,	0x44,
	0x44, 0x1C, 0xCC, 0xC3, 0x44,
	0x44, 0x1C, 0xCC, 0xC3
};

static DEFINE_PKTUI(star2_s3_sa_self_move_pos, &s6e3ha8_aod_maptbl[SELF_MOVE_POS_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(star2_s3_sa_self_move_pos, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MOVE_POS, 0x6);

static void *s6e3ha8_aod_self_move_on_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(star2_s3_sa_self_move_pos),
	&PKTINFO(star2_s3_sa_self_move_on),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

static char S6E3HA8_AOD_SELF_MOVE_SYNC_OFF[] = {
	0x7D,
	0x01,
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_self_move_sync_off, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MOVE_SYNC_OFF, 0);


static char S6E3HA8_AOD_SELF_MOVE_FORCE_RESET[] = {
	0x7D,
	0x00, 0x00, 0x00, 0x04
};
DEFINE_STATIC_PACKET(s6e3ha8_aod_self_move_force_reset, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MOVE_FORCE_RESET, 0);

static DEFINE_PANEL_MDELAY(s6e3ha8_aod_move_off_delay, 34);

static void *s6e3ha8_aod_self_move_off_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_self_move_force_reset),
	&DLYINFO(s6e3ha8_aod_move_off_delay),
	&PKTINFO(s6e3ha8_aod_self_move_sync_off),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

static char S6E3HA8_AOD_SELF_MOVE_RESET[] = {
	0x7D,
	0x00, 0x00, 0x00, 0x00,
};
#if 0
DEFINE_STATIC_PACKET(s6e3ha8_aod_self_move_reset, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MOVE_RESET, 0);
#else
static DEFINE_PKTUI(s6e3ha8_aod_self_move_reset, &s6e3ha8_aod_maptbl[SELF_MOVE_RESET_MAPTBL], 0);
static DEFINE_VARIABLE_PACKET(s6e3ha8_aod_self_move_reset, DSI_PKT_TYPE_WR, S6E3HA8_AOD_SELF_MOVE_RESET, 0);
#endif

static void *s6e3ha8_aod_self_move_reset_cmdtbl[] = {
	&KEYINFO(s6e3ha8_aod_level2_key_enable),
	&PKTINFO(s6e3ha8_aod_self_move_reset),
	&KEYINFO(s6e3ha8_aod_level2_key_disable),
};

/* -------------------------- End self move --------------------------*/

static struct seqinfo s6e3ha8_aod_seqtbl[MAX_AOD_SEQ] = {
	[SELF_MASK_IMG_SEQ] = SEQINFO_INIT("self_mask_img", s6e3ha8_aod_self_mask_img_cmdtbl),
	[SELF_MASK_ENA_SEQ] = SEQINFO_INIT("self_mask_ena", s6e3ha8_aod_self_mask_ena_cmdtbl),
	[SELF_MASK_DIS_SEQ] = SEQINFO_INIT("self_mask_dis", s6e3ha8_aod_self_mask_dis_cmdtbl),
	[ANALOG_IMG_SEQ] = SEQINFO_INIT("analog_img", s6e3ha8_aod_analog_img_cmdtbl),
	[ANALOG_CTRL_SEQ] = SEQINFO_INIT("analog_ctrl", s6e3ha8_aod_analog_ctrl_cmdtbl),
	[DIGITAL_IMG_SEQ] = SEQINFO_INIT("digital_img", s6e3ha8_aod_digital_img_cmdtbl),
	[DIGITAL_CTRL_SEQ] = SEQINFO_INIT("digital_ctrl", s6e3ha8_aod_digital_ctrl_cmdtbl),
	[ENTER_AOD_SEQ] = SEQINFO_INIT("enter_aod", s6e3ha8_aod_enter_aod_cmdtbl),
	[EXIT_AOD_SEQ] = SEQINFO_INIT("exit_aod", s6e3ha8_aod_exit_aod_cmdtbl),
	[SELF_MOVE_ON_SEQ] = SEQINFO_INIT("self_move_en", s6e3ha8_aod_self_move_on_cmdtbl),
	[SELF_MOVE_OFF_SEQ] = SEQINFO_INIT("self_move_off", s6e3ha8_aod_self_move_off_cmdtbl),
	[SELF_MOVE_RESET_SEQ] = SEQINFO_INIT("self_move_reset", s6e3ha8_aod_self_move_reset_cmdtbl),
	[SELF_ICON_IMG_SEQ] = SEQINFO_INIT("self_icon_img", s6e3ha8_aod_self_icon_img_cmdtbl),
	[ICON_GRID_ON_SEQ] = SEQINFO_INIT("icon_grid_on", s6e3ha8_aod_icon_grid_on_cmdtbl),
	[ICON_GRID_OFF_SEQ] = SEQINFO_INIT("icon_grid_off", s6e3ha8_aod_icon_grid_off_cmdtbl),
	[SET_TIME_SEQ] = SEQINFO_INIT("SET_TIME", s6e3ha8_aod_set_time_cmdtbl),
};

static struct aod_tune s6e3ha8_aod_tune = {
	.name = "s6e3ha8_aod",
	.nr_seqtbl = ARRAY_SIZE(s6e3ha8_aod_seqtbl),
	.seqtbl = s6e3ha8_aod_seqtbl,
	.nr_maptbl = ARRAY_SIZE(s6e3ha8_aod_maptbl),
	.maptbl = s6e3ha8_aod_maptbl,
	.self_mask_en = true,
};
#endif //__S6E3HA8_AOD_PANEL_H__
