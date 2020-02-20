/*
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __S6E3HA9_PROFILER_PANEL_H__
#define __S6E3HA9_PROFILER_PANEL_H__


#include "s6e3ha9_profiler.h"
#include "s6e3ha9_profiler_font.h"

static u8 HA9_PROFILE_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 HA9_PROFILE_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };

static u8 HA9_PROFILE_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };
static u8 HA9_PROFILE_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };

static DEFINE_STATIC_PACKET(ha9_profile_key2_enable, DSI_PKT_TYPE_WR, HA9_PROFILE_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(ha9_profile_key2_disable, DSI_PKT_TYPE_WR, HA9_PROFILE_KEY2_DISABLE, 0);

static DEFINE_STATIC_PACKET(ha9_profile_key3_enable, DSI_PKT_TYPE_WR, HA9_PROFILE_KEY3_ENABLE, 0);
static DEFINE_STATIC_PACKET(ha9_profile_key3_disable, DSI_PKT_TYPE_WR, HA9_PROFILE_KEY3_DISABLE, 0);

static DEFINE_PANEL_KEY(ha9_profile_key2_enable, CMD_LEVEL_2,
	KEY_ENABLE, &PKTINFO(ha9_profile_key2_enable));
static DEFINE_PANEL_KEY(ha9_profile_key2_disable, CMD_LEVEL_2,
	KEY_DISABLE, &PKTINFO(ha9_profile_key2_disable));

static DEFINE_PANEL_KEY(ha9_profile_key3_enable, CMD_LEVEL_3,
	KEY_ENABLE, &PKTINFO(ha9_profile_key3_enable));
static DEFINE_PANEL_KEY(ha9_profile_key3_disable, CMD_LEVEL_3,
	KEY_DISABLE, &PKTINFO(ha9_profile_key3_disable));

static struct keyinfo KEYINFO(ha9_profile_key2_enable);
static struct keyinfo KEYINFO(ha9_profile_key2_disable);
static struct keyinfo KEYINFO(ha9_profile_key3_enable);
static struct keyinfo KEYINFO(ha9_profile_key3_disable);

static struct maptbl ha9_profiler_maptbl[] = {
	[PROFILE_WIN_UPDATE_MAP] = DEFINE_0D_MAPTBL(ha9_set_self_grid,
		init_common_table, NULL, ha9_profile_win_update),
	[DISPLAY_PROFILE_FPS_MAP] = DEFINE_0D_MAPTBL(ha9_display_profile_fps,
		init_common_table, NULL, ha9_profile_display_fps),
	[PROFILE_SET_COLOR_MAP] = DEFINE_0D_MAPTBL(ha9_set_fps_color,
		init_common_table, NULL, ha9_profile_set_color),
	[PROFILE_SET_CIRCLE] = DEFINE_0D_MAPTBL(ha9_set_circle,
		init_common_table, NULL, ha9_profile_circle),
};

static char HA9_SET_SELF_GRID[] = {
	0x7C,
	0x01, 0x01, 0x40, 0x1F,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x01, 0x00,
	0x11,
};
static DEFINE_PKTUI(ha9_set_self_grid, &ha9_profiler_maptbl[PROFILE_WIN_UPDATE_MAP], 0);
static DEFINE_VARIABLE_PACKET(ha9_set_self_grid, DSI_PKT_TYPE_WR, HA9_SET_SELF_GRID, 0);


static void *ha9_profile_win_update_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_set_self_grid),
	&KEYINFO(ha9_profile_key2_disable),
};

static char ha9_disable_self_grid[] = {
	0x7C,
	0x00, 0x00,
};
static DEFINE_STATIC_PACKET(ha9_disable_self_grid, DSI_PKT_TYPE_WR, HA9_SET_SELF_GRID, 0);

static void *ha9_profile_win_disable_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_disable_self_grid),
	&KEYINFO(ha9_profile_key2_disable),
};


static char HA9_SET_PROFILE_SD_DIGIT[] = {
	0x75,
	0x02,
};
static DEFINE_STATIC_PACKET(ha9_set_profile_sd_digit, DSI_PKT_TYPE_WR, HA9_SET_PROFILE_SD_DIGIT, 0);


static char HA9_RESET_PROFILE_SD_DIGIT[] = {
	0x75,
	0x00,
};
static DEFINE_STATIC_PACKET(ha9_reset_profile_sd_digit, DSI_PKT_TYPE_WR, HA9_RESET_PROFILE_SD_DIGIT, 0);

static DEFINE_STATIC_PACKET(ha9_profile_font, DSI_PKT_TYPE_WR_SR, HA9_PROFILE_FONT, 0);

static void *ha9_send_profile_font_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_set_profile_sd_digit),
	&PKTINFO(ha9_profile_font),
	&PKTINFO(ha9_reset_profile_sd_digit),
	&KEYINFO(ha9_profile_key2_disable),
};

static char HA9_INIT_PROFILE_FPS[] = {
	0x80,
	0x00, 0x03, 0x02, 0x1F,
	0x03, 0xE8, 0x01, 0x90,
	0x04, 0x1A, 0x01, 0x90,
	/* invisiable M2 x : 2230*/
	0x04, 0x4C, 0x01, 0x90,
	0x04, 0x4C, 0x01, 0x90,
	0x00, 0x28, 0x00, 0x4B,
	0x80, 0x0F, 0xFF, 0x0F,
	0x00, 0x00,
};
static DEFINE_STATIC_PACKET(ha9_init_profile_fps, DSI_PKT_TYPE_WR, HA9_INIT_PROFILE_FPS, 0);


static char HA9_INIT_PROFILE_TIMER[] = {
	0x81,
	0x00, 0x03, 0x09, 0x09,
	0x1E, 0x00, 0x1E, 0x03,
	0x00, 0x00
};
static DEFINE_STATIC_PACKET(ha9_init_profile_timer, DSI_PKT_TYPE_WR, HA9_INIT_PROFILE_TIMER, 0);

static char HA9_UPDATE_PROFILE_FPS[] = {
	0x78,
	0x01,
};
static DEFINE_STATIC_PACKET(ha9_update_profile_fps, DSI_PKT_TYPE_WR, HA9_UPDATE_PROFILE_FPS, 0);

void *ha9_init_profile_fps_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_init_profile_fps),
	&PKTINFO(ha9_init_profile_timer),
	&PKTINFO(ha9_update_profile_fps),
	&KEYINFO(ha9_profile_key2_disable),
};


static char HA9_DISPLAY_PROFILE_FSP[] = {
	0x81,
	0x0A, 0x0A,
};
static DEFINE_PKTUI(ha9_display_profile_fps, &ha9_profiler_maptbl[DISPLAY_PROFILE_FPS_MAP], 0);
static DEFINE_VARIABLE_PACKET(ha9_display_profile_fps, DSI_PKT_TYPE_WR, HA9_DISPLAY_PROFILE_FSP, 2);


void *ha9_display_profile_fps_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_display_profile_fps),
	&PKTINFO(ha9_update_profile_fps),
	&KEYINFO(ha9_profile_key2_disable),
};

static char HA9_SET_FPS_COLOR[] = {
	0x80,
	0x80, 0xFF, 0x0F, 0x0F,
};
static DEFINE_PKTUI(ha9_set_fps_color, &ha9_profiler_maptbl[PROFILE_SET_COLOR_MAP], 0);
static DEFINE_VARIABLE_PACKET(ha9_set_fps_color, DSI_PKT_TYPE_WR, HA9_SET_FPS_COLOR, 24);


void *ha9_profile_set_color_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_set_fps_color),
	&KEYINFO(ha9_profile_key2_disable),
};

static char HA9_SET_CIRCLE[] = {
	0x79,
	0x00, 0x88, 0x00, 0x00, 0x00,
	0x00, 0x57, 0x01, 0x30, 0x0A,
	0x00, 0xFF, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0x64, 0x01, 0x2C,
	0x00, 0x00, 0x00, 0x00, 0x01,
	0x00,
};


static DEFINE_PKTUI(ha9_set_circle, &ha9_profiler_maptbl[PROFILE_SET_CIRCLE], 0);
static DEFINE_VARIABLE_PACKET(ha9_set_circle, DSI_PKT_TYPE_WR, HA9_SET_CIRCLE, 0);


void *ha9_profile_circle_cmdtbl[] = {
	&KEYINFO(ha9_profile_key2_enable),
	&PKTINFO(ha9_set_circle),
	&KEYINFO(ha9_profile_key2_disable),
};

static struct seqinfo ha9_profiler_seqtbl[MAX_PROFILER_SEQ] = {
	[PROFILE_WIN_UPDATE_SEQ] = SEQINFO_INIT("update_profile_win", ha9_profile_win_update_cmdtbl),
	[PROFILE_DISABLE_WIN_SEQ] = SEQINFO_INIT("disable_profile_win", ha9_profile_win_disable_cmdtbl),
	[SEND_FROFILE_FONT_SEQ] = SEQINFO_INIT("send_profile_font", ha9_send_profile_font_cmdtbl),
	[INIT_PROFILE_FPS_SEQ] = SEQINFO_INIT("init_profile_fps", ha9_init_profile_fps_cmdtbl),
	[DISPLAY_PROFILE_FPS_SEQ] = SEQINFO_INIT("display_profile_fps", ha9_display_profile_fps_cmdtbl),
	[PROFILE_SET_COLOR_SEQ] = SEQINFO_INIT("profile_set_color", ha9_profile_set_color_cmdtbl),
	[PROFILER_SET_CIRCLR_SEQ] = SEQINFO_INIT("profile_circle", ha9_profile_circle_cmdtbl),
};

static struct profiler_tune ha9_profiler_tune = {
	.name = "s6e3ha9_profiler",
	.nr_seqtbl = ARRAY_SIZE(ha9_profiler_seqtbl),
	.seqtbl = ha9_profiler_seqtbl,
	.nr_maptbl = ARRAY_SIZE(ha9_profiler_maptbl),
	.maptbl = ha9_profiler_maptbl,
};

#endif //__S6E3HA9_PROFILER_PANEL_H__
