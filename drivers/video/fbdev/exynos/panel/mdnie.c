// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/backlight.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/pm_runtime.h>
#include "panel_drv.h"
#include "mdnie.h"
#include "copr.h"
#ifdef CONFIG_DISPLAY_USE_INFO
#include "dpui.h"
#endif

#ifdef MDNIE_SELF_TEST
int g_coord_x = MIN_WCRD_X;
int g_coord_y = MIN_WCRD_Y;
#endif

static const char * const mdnie_maptbl_name[] = {
	[MDNIE_UI_MAPTBL] = "ui",
	[MDNIE_VIDEO_MAPTBL] = "video",
	[MDNIE_CAMERA_MAPTBL] = "camera",
	[MDNIE_GALLERY_MAPTBL] = "gallery",
	[MDNIE_VT_MAPTBL] = "vt",
	[MDNIE_BROWSER_MAPTBL] = "browser",
	[MDNIE_EBOOK_MAPTBL] = "ebook",
	[MDNIE_EMAIL_MAPTBL] = "email",
	[MDNIE_GAME_LOW_MAPTBL] = "game_low",
	[MDNIE_GAME_MID_MAPTBL] = "game_mid",
	[MDNIE_GAME_HIGH_MAPTBL] = "game_high",
	[MDNIE_VIDEO_ENHANCER_MAPTBL] = "video_enhancer",
	[MDNIE_VIDEO_ENHANCER_THIRD_MAPTBL] = "video_enhancer_third",
	[MDNIE_HMD_8_MAPTBL] = "hmd_8",
	[MDNIE_HMD_16_MAPTBL] = "hmd_16",
#if defined(CONFIG_TDMB)
	[MDNIE_DMB_MAPTBL] = "dmb",
#endif
	/* ACCESSIBILITY */
	[MDNIE_NEGATIVE_MAPTBL] = "negative",
	[MDNIE_COLOR_BLIND_MAPTBL] = "color_blind",
	[MDNIE_SCREEN_CURTAIN_MAPTBL] = "screen_curtain",
	[MDNIE_GRAYSCALE_MAPTBL] = "grayscale",
	[MDNIE_GRAYSCALE_NEGATIVE_MAPTBL] = "grayscale_negative",
	[MDNIE_COLOR_BLIND_HBM_MAPTBL] = "color_blind_hbm",
	/* BYPASS */
	[MDNIE_BYPASS_MAPTBL] = "bypass",
	/* HBM */
	[MDNIE_HBM_MAPTBL] = "hbm",
	/* HMD */
	[MDNIE_HMD_MAPTBL] = "hmd",
	/* HDR */
	[MDNIE_HDR_MAPTBL] = "hdr",
	/* NIGHT */
	[MDNIE_NIGHT_MAPTBL] = "night",
	/* LIGHT_NOTIFICATION */
	[MDNIE_LIGHT_NOTIFICATION_MAPTBL] = "light_notification",
	/* COLOR_LENS */
	[MDNIE_COLOR_LENS_MAPTBL] = "color_lens",
};

static const char * const scr_white_mode_name[] = {
	[SCR_WHITE_MODE_NONE] = "none",
	[SCR_WHITE_MODE_COLOR_COORDINATE] = "coordinate",
	[SCR_WHITE_MODE_ADJUST_LDU] = "adjust-ldu",
	[SCR_WHITE_MODE_SENSOR_RGB] = "sensor-rgb",
};

static const char * const mdnie_mode_name[] = {
	[MDNIE_OFF_MODE] = "off",
	[MDNIE_BYPASS_MODE] = "bypass",
	[MDNIE_LIGHT_NOTIFICATION_MODE] = "light_notification",
	[MDNIE_ACCESSIBILITY_MODE] = "accessibility",
	[MDNIE_COLOR_LENS_MODE] = "color_lens",
	[MDNIE_HDR_MODE] = "hdr",
	[MDNIE_HMD_MODE] = "hmd",
	[MDNIE_NIGHT_MODE] = "night",
	[MDNIE_HBM_MODE] = "hbm",
	[MDNIE_DMB_MODE] = "dmb",
	[MDNIE_SCENARIO_MODE] = "scenario",
};

static const char * const scenario_mode_name[] = {
	[DYNAMIC] = "dynamic",
	[STANDARD] = "standard",
	[NATURAL] = "natural",
	[MOVIE] = "movie",
	[AUTO] = "auto",
};

static const char * const scenario_name[] = {
	[UI_MODE] = "ui",
	[VIDEO_NORMAL_MODE] = "video_normal",
	[CAMERA_MODE] = "camera",
	[NAVI_MODE] = "navi",
	[GALLERY_MODE] = "gallery",
	[VT_MODE] = "vt",
	[BROWSER_MODE] = "browser",
	[EBOOK_MODE] = "ebook",
	[EMAIL_MODE] = "email",
	[GAME_LOW_MODE] = "game_low",
	[GAME_MID_MODE] = "game_mid",
	[GAME_HIGH_MODE] = "game_high",
	[VIDEO_ENHANCER] = "video_enhancer",
	[VIDEO_ENHANCER_THIRD] = "video_enhancer_third",
	[HMD_8_MODE] = "hmd_8",
	[HMD_16_MODE] = "hmd_16",
	[DMB_NORMAL_MODE] = "dmb_normal",
};

static const char * const accessibility_name[] = {
	[ACCESSIBILITY_OFF] = "off",
	[NEGATIVE] = "negative",
	[COLOR_BLIND] = "color_blind",
	[SCREEN_CURTAIN] = "screen_curtain",
	[GRAYSCALE] = "grayscale",
	[GRAYSCALE_NEGATIVE] = "grayscale_negative",
	[COLOR_BLIND_HBM] = "color_blind_hbm",
};

int mdnie_current_state(struct mdnie_info *mdnie)
{
	struct panel_device *panel =
		container_of(mdnie, struct panel_device, mdnie);
	int mdnie_mode;

	if (IS_BYPASS_MODE(mdnie))
		mdnie_mode = MDNIE_BYPASS_MODE;
	else if (IS_LIGHT_NOTIFICATION_MODE(mdnie))
		mdnie_mode = MDNIE_LIGHT_NOTIFICATION_MODE;
	else if (IS_ACCESSIBILITY_MODE(mdnie))
		mdnie_mode = MDNIE_ACCESSIBILITY_MODE;
	else if (IS_COLOR_LENS_MODE(mdnie))
		mdnie_mode = MDNIE_COLOR_LENS_MODE;
	else if (IS_HMD_MODE(mdnie))
		mdnie_mode = MDNIE_HMD_MODE;
	else if (IS_NIGHT_MODE(mdnie))
		mdnie_mode = MDNIE_NIGHT_MODE;
	else if (IS_HBM_MODE(mdnie))
		mdnie_mode = MDNIE_HBM_MODE;
	else if (IS_HDR_MODE(mdnie))
		mdnie_mode = MDNIE_HDR_MODE;
#if defined(CONFIG_TDMB)
	else if (IS_DMB_MODE(mdnie))
		mdnie_mode = MDNIE_DMB_MODE;
#endif
	else if (IS_SCENARIO_MODE(mdnie))
		mdnie_mode = MDNIE_SCENARIO_MODE;
	else
		mdnie_mode = MDNIE_OFF_MODE;

	if (panel->state.cur_state == PANEL_STATE_ALPM &&
	((mdnie_mode == MDNIE_ACCESSIBILITY_MODE &&
	(mdnie->props.accessibility == NEGATIVE ||
	mdnie->props.accessibility == GRAYSCALE_NEGATIVE)) ||
	(mdnie_mode == MDNIE_SCENARIO_MODE && !IS_LDU_MODE(mdnie)) ||
	mdnie_mode == MDNIE_COLOR_LENS_MODE ||
	mdnie_mode == MDNIE_DMB_MODE ||
	mdnie_mode == MDNIE_HDR_MODE ||
	mdnie_mode == MDNIE_LIGHT_NOTIFICATION_MODE ||
	mdnie_mode == MDNIE_HMD_MODE)) {
		pr_debug("%s block mdnie (%s->%s) in doze mode\n",
				__func__, mdnie_mode_name[mdnie_mode],
				mdnie_mode_name[MDNIE_BYPASS_MODE]);
		mdnie_mode = MDNIE_BYPASS_MODE;
	}

	return mdnie_mode;
}

int mdnie_get_maptbl_index(struct mdnie_info *mdnie)
{
	int index;
	int mdnie_mode = mdnie_current_state(mdnie);

	switch (mdnie_mode) {
	case MDNIE_BYPASS_MODE:
		index = MDNIE_BYPASS_MAPTBL;
		break;
	case MDNIE_LIGHT_NOTIFICATION_MODE:
		index = MDNIE_LIGHT_NOTIFICATION_MAPTBL;
		break;
	case MDNIE_ACCESSIBILITY_MODE:
		index = MAPTBL_IDX_ACCESSIBILITY(mdnie->props.accessibility);
		break;
	case MDNIE_COLOR_LENS_MODE:
		index = MDNIE_COLOR_LENS_MAPTBL;
		break;
	case MDNIE_HDR_MODE:
		index = MDNIE_HDR_MAPTBL;
		break;
	case MDNIE_HMD_MODE:
		index = MDNIE_HMD_MAPTBL;
		break;
	case MDNIE_NIGHT_MODE:
		index = MDNIE_NIGHT_MAPTBL;
		break;
	case MDNIE_HBM_MODE:
		index = MDNIE_HBM_MAPTBL;
		break;
#if defined(CONFIG_TDMB)
	case MDNIE_DMB_MODE:
		index = MDNIE_DMB_MAPTBL;
		break;
#endif
	case MDNIE_SCENARIO_MODE:
		index = MAPTBL_IDX_SCENARIO(mdnie->props.scenario);
		break;
	default:
		index = -EINVAL;
		pr_err("%s, unknown mdnie\n", __func__);
		break;
	}

	if (index >= 0 && index < MAX_MDNIE_MAPTBL) {
#ifdef DEBUG_PANEL
		panel_dbg("%s, mdnie %s(%d), maptbl %s(%d) found\n",
				__func__, mdnie_mode_name[mdnie_mode],
				mdnie_mode, mdnie_maptbl_name[index], index);
#endif
	} else {
		panel_err("%s, mdnie %s(%d), maptbl not found!! (%d)\n",
				__func__, mdnie_mode_name[mdnie_mode],
				mdnie_mode, index);
		index = MDNIE_UI_MAPTBL;
	}

	return index;
}

struct maptbl *mdnie_find_maptbl(struct mdnie_info *mdnie)
{
	int index = mdnie_get_maptbl_index(mdnie);

	if (unlikely(index < 0 || index >= MAX_MDNIE_MAPTBL)) {
		pr_err("%s, failed to find maptbl %d\n", __func__, index);
		return NULL;
	}
	return &mdnie->maptbl[index];
}

struct maptbl *mdnie_find_etc_maptbl(struct mdnie_info *mdnie, int index)
{
	if (unlikely(index < 0 || index >= mdnie->nr_etc_maptbl)) {
		pr_err("%s, failed to find maptbl %d\n", __func__, index);
		return NULL;
	}
	return &mdnie->etc_maptbl[index];
}

struct maptbl *mdnie_find_scr_white_maptbl(struct mdnie_info *mdnie, int index)
{
	if (unlikely(index < 0 || index >= mdnie->nr_scr_white_maptbl)) {
		pr_err("%s, failed to find maptbl %d\n", __func__, index);
		return NULL;
	}
	return &mdnie->scr_white_maptbl[index];
}

static int mdnie_get_coordinate(struct mdnie_info *mdnie, int *x, int *y)
{
	struct panel_device *panel =
		container_of(mdnie, struct panel_device, mdnie);
	struct panel_info *panel_data = &panel->panel_data;
	u8 coordinate[PANEL_COORD_LEN] = { 0, };
	int ret;

	if (!mdnie || !x || !y) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	ret = resource_copy_by_name(panel_data, coordinate, "coordinate");
	if (ret < 0) {
		pr_err("%s, failed to copy 'coordinate' resource\n", __func__);
		return -EINVAL;
	}

	*x = (coordinate[0] << 8) | coordinate[1];
	*y = (coordinate[2] << 8) | coordinate[3];
#ifdef MDNIE_SELF_TEST
	*x = g_coord_x;
	*y = g_coord_y;
#endif

	if (*x < MIN_WCRD_X || *x > MAX_WCRD_X ||
			*y < MIN_WCRD_Y || *y > MAX_WCRD_Y)
		pr_warn("%s, need to check coord_x:%d coord_y:%d)\n", __func__, *x, *y);

	return 0;
}

static bool mdnie_coordinate_changed(struct mdnie_info *mdnie)
{
	int x, y;

	mdnie_get_coordinate(mdnie, &x, &y);

	return (mdnie->props.wcrd_x != x || mdnie->props.wcrd_y != y);
}

static int mdnie_coordinate_area(struct mdnie_info *mdnie, int x, int y)
{
	s64 result[MAX_CAL_LINE];
	int i, area;

	for (i = 0; i < MAX_CAL_LINE; i++)
		result[i] = COLOR_OFFSET_FUNC(x, y,
				mdnie->props.line[i].num,
				mdnie->props.line[i].den,
				mdnie->props.line[i].con);

	area = RECOGNIZE_REGION(result[H_LINE], result[V_LINE]);
	pr_debug("%s, coord %d %d area Q%d, result %lld %lld\n",
			__func__, x, y, area + 1, result[H_LINE], result[V_LINE]);

	return area;
}

static int mdnie_coordinate_tune_x(struct mdnie_info *mdnie, int x, int y)
{
	int res, area;

	area = mdnie_coordinate_area(mdnie, x, y);
	res = ((mdnie->props.coef[area].a * x) + (mdnie->props.coef[area].b * y) +
		(((mdnie->props.coef[area].c * x + (1L << 9)) >> 10) * y) +
		(mdnie->props.coef[area].d * 10000) + (1L << 9)) >> 10;

	return max(min(res, 1024), 0);
}

static int mdnie_coordinate_tune_y(struct mdnie_info *mdnie, int x, int y)
{
	int res, area;

	area = mdnie_coordinate_area(mdnie, x, y);
	res = ((mdnie->props.coef[area].e * x) + (mdnie->props.coef[area].f * y) +
		(((mdnie->props.coef[area].g * x + (1L << 9)) >> 10) * y) +
		(mdnie->props.coef[area].h * 10000) + (1L << 9)) >> 10;

	return max(min(res, 1024), 0);
}

static void mdnie_coordinate_tune_rgb(struct mdnie_info *mdnie, int x, int y, u8 (*tune_rgb)[MAX_COLOR])
{
	static int pt[MAX_QUAD][MAX_RGB_PT] = {
		{ CCRD_PT_5, CCRD_PT_2, CCRD_PT_6, CCRD_PT_3 },	/* QUAD_1 */
		{ CCRD_PT_5, CCRD_PT_2, CCRD_PT_4, CCRD_PT_1 },	/* QUAD_2 */
		{ CCRD_PT_5, CCRD_PT_8, CCRD_PT_4, CCRD_PT_7 },	/* QUAD_3 */
		{ CCRD_PT_5, CCRD_PT_8, CCRD_PT_6, CCRD_PT_9 },	/* QUAD_4 */
	};
	int i, c, type, area, tune_x, tune_y, res[MAX_COLOR][MAX_RGB_PT];
	s64 result[MAX_CAL_LINE];

	area = mdnie_coordinate_area(mdnie, x, y);

	if (((x - mdnie->props.cal_x_center) * (x - mdnie->props.cal_x_center) + (y - mdnie->props.cal_y_center) * (y - mdnie->props.cal_y_center)) <= mdnie->props.cal_boundary_center) {
		tune_x = 0;
		tune_y = 0;
	} else {
		tune_x = mdnie_coordinate_tune_x(mdnie, x, y);
		tune_y = mdnie_coordinate_tune_y(mdnie, x, y);
	}

	for (i = 0; i < MAX_CAL_LINE; i++)
		result[i] = COLOR_OFFSET_FUNC(x, y,
				mdnie->props.line[i].num,
				mdnie->props.line[i].den,
				mdnie->props.line[i].con);

	for (type = 0; type < MAX_WCRD_TYPE; type++) {
		for (c = 0; c < MAX_COLOR; c++) {
			for (i = 0; i < MAX_RGB_PT; i++)
				res[c][i] = mdnie->props.vtx[type][pt[area][i]][c];
			tune_rgb[type][c] =
				((((res[c][RGB_00] * (1024 - tune_x) + (res[c][RGB_10] * tune_x)) * (1024 - tune_y)) +
				  ((res[c][RGB_01] * (1024 - tune_x) + (res[c][RGB_11] * tune_x)) * tune_y)) + (1L << 19)) >> 20;
		}
	}
	panel_info("coord (x:%4d y:%4d Q%d compV:%8lld compH:%8lld)\t"
			"tune_coord (%4d %4d) tune_rgb[ADT] (%3d %3d %3d) tune_rgb[D65] (%3d %3d %3d)\n",
			x, y, area + 1, result[H_LINE], result[V_LINE], tune_x, tune_y,
			tune_rgb[WCRD_TYPE_ADAPTIVE][0], tune_rgb[WCRD_TYPE_ADAPTIVE][1],
			tune_rgb[WCRD_TYPE_ADAPTIVE][2], tune_rgb[WCRD_TYPE_D65][0],
			tune_rgb[WCRD_TYPE_D65][1], tune_rgb[WCRD_TYPE_D65][2]);

}

static void mdnie_coordinate_tune(struct mdnie_info *mdnie)
{
	int x, y;

	mdnie_get_coordinate(mdnie, &x, &y);
	mdnie->props.wcrd_x = x;
	mdnie->props.wcrd_y = y;
	mdnie_coordinate_tune_rgb(mdnie, x, y, mdnie->props.coord_wrgb);
}

static int panel_set_mdnie(struct panel_device *panel)
{
	int ret;
	struct mdnie_info *mdnie = &panel->mdnie;
	int mdnie_mode = mdnie_current_state(mdnie);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel))
		return 0;

#ifdef CONFIG_SUPPORT_AFC
	pr_info("%s, do mdnie-seq (mode:%s, afc:%s)\n",
			__func__, mdnie_mode_name[mdnie_mode],
			!mdnie->props.afc_on ? "off" : "on");
#else
	pr_info("%s, do mdnie-seq (mode:%s)\n",
			__func__, mdnie_mode_name[mdnie_mode]);
#endif

	ret = 0;
	mutex_lock(&panel->op_lock);
	ret = panel_do_seqtbl(panel, &mdnie->seqtbl[MDNIE_SET_SEQ]);
	if (unlikely(ret < 0))
		pr_err("%s, failed to write seqtbl\n", __func__);

#ifdef CONFIG_SUPPORT_AFC
	ret = panel_do_seqtbl(panel, !mdnie->props.afc_on ?
			&mdnie->seqtbl[MDNIE_AFC_OFF_SEQ] :
			&mdnie->seqtbl[MDNIE_AFC_ON_SEQ]);
	if (unlikely(ret < 0))
		pr_err("%s, failed to write seqtbl\n", __func__);
#endif

	mutex_unlock(&panel->op_lock);

	return ret;
}

static void mdnie_maptbl_init(struct mdnie_info *mdnie, int index)
{
	int i;

	if (((index + 1) * mdnie->nr_reg) > mdnie->nr_maptbl) {
		pr_err("%s out of range index %d\n", __func__, index);
		return;
	}

	for (i = 0; i < mdnie->nr_reg; i++)
		maptbl_init(&mdnie->maptbl[index * mdnie->nr_reg + i]);
}

static void scr_white_maptbl_init(struct mdnie_info *mdnie, int index)
{
	if ((index + 1) > mdnie->nr_scr_white_maptbl) {
		pr_err("%s out of range index %d\n", __func__, index);
		return;
	}
	maptbl_init(&mdnie->scr_white_maptbl[index]);
}

static void scr_white_maptbls_init(struct mdnie_info *mdnie)
{
	int i;

	for (i = 0; i < mdnie->nr_scr_white_maptbl; i++)
		maptbl_init(&mdnie->scr_white_maptbl[i]);
}

static void mdnie_update_scr_white_mode(struct mdnie_info *mdnie)
{
	int mdnie_mode = mdnie_current_state(mdnie);

	if (mdnie_mode == MDNIE_SCENARIO_MODE) {
		if ((IS_LDU_MODE(mdnie)) && (mdnie->props.scenario != EBOOK_MODE)) {
			mdnie->props.scr_white_mode = SCR_WHITE_MODE_ADJUST_LDU;
		} else if (mdnie->props.update_sensorRGB &&
				mdnie->props.mode == AUTO &&
				(mdnie->props.scenario == BROWSER_MODE ||
				 mdnie->props.scenario == EBOOK_MODE)) {
			mdnie->props.scr_white_mode = SCR_WHITE_MODE_SENSOR_RGB;
			mdnie->props.update_sensorRGB = false;
		} else if (mdnie->props.scenario <= SCENARIO_MAX &&
				mdnie->props.scenario != EBOOK_MODE) {
			mdnie->props.scr_white_mode =
				SCR_WHITE_MODE_COLOR_COORDINATE;
		} else {
			mdnie->props.scr_white_mode = SCR_WHITE_MODE_NONE;
		}
	} else if (mdnie_mode == MDNIE_HBM_MODE) {
		mdnie->props.scr_white_mode =
				SCR_WHITE_MODE_COLOR_COORDINATE;
	} else {
		mdnie->props.scr_white_mode = SCR_WHITE_MODE_NONE;
	}

#ifdef DEBUG_PANEL
	panel_info("%s scr_white_mode %s\n", __func__,
			scr_white_mode_name[mdnie->props.scr_white_mode]);
#endif
}

int panel_mdnie_update(struct panel_device *panel)
{
	int ret;
	struct mdnie_info *mdnie = &panel->mdnie;

	mutex_lock(&mdnie->lock);
	if (!IS_MDNIE_ENABLED(mdnie)) {
		dev_err(mdnie->dev, "mdnie is off state\n");
		mutex_unlock(&mdnie->lock);
		return -EINVAL;
	}

	if (mdnie_coordinate_changed(mdnie)) {
		mdnie_coordinate_tune(mdnie);
		scr_white_maptbls_init(mdnie);
	}
	mdnie_update_scr_white_mode(mdnie);

	ret = panel_set_mdnie(panel);
	if (ret < 0) {
		pr_err("%s, failed to set mdnie %d\n", __func__, ret);
		mutex_unlock(&mdnie->lock);
		return ret;
	}
	mutex_unlock(&mdnie->lock);

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	copr_update_start(&panel->copr, 3);
#endif

	return 0;
}

static int mdnie_update(struct mdnie_info *mdnie)
{
	int ret;
	struct panel_device *panel =
		container_of(mdnie, struct panel_device, mdnie);

	panel_wake_lock(panel);
	ret = panel_mdnie_update(panel);
	panel_wake_unlock(panel);

	return ret;
}

#ifdef MDNIE_SELF_TEST
static void mdnie_coordinate_tune_test(struct mdnie_info *mdnie)
{
	int i, x, y;
	u8 tune_rgb[MAX_WCRD_TYPE][MAX_COLOR];
	int input[27][2] = {
		{ 2936, 3144 }, { 2987, 3203 }, { 3032, 3265 }, { 2954, 3108 }, { 2991, 3158 }, { 3041, 3210 }, { 2967, 3058 }, { 3004, 3091 },
		{ 3063, 3156 }, { 2985, 3206 }, { 3032, 3238 }, { 2955, 3094 }, { 2997, 3151 }, { 3045, 3191 }, { 2971, 3047 }, { 3020, 3100 },
		{ 3066, 3144 }, { 2983, 3131 }, { 2987, 3170 }, { 2975, 3159 }, { 3021, 3112 }, { 2982, 3122 }, { 2987, 3130 }, { 2930, 3260 },
		{ 2930, 3050 }, { 3060, 3050 }, { 3060, 3260 },
	};
	u8 output[27][MAX_COLOR] = {
		{ 0xFF, 0xFA, 0xF9 }, { 0xFE, 0xFA, 0xFE }, { 0xF8, 0xF6, 0xFF }, { 0xFF, 0xFD, 0xFB },
		{ 0xFF, 0xFE, 0xFF }, { 0xF9, 0xFA, 0xFF }, { 0xFC, 0xFF, 0xF9 }, { 0xFB, 0xFF, 0xFB },
		{ 0xF9, 0xFF, 0xFF }, { 0xFE, 0xFA, 0xFE }, { 0xF9, 0xF8, 0xFF }, { 0xFE, 0xFE, 0xFA },
		{ 0xFE, 0xFF, 0xFF }, { 0xF8, 0xFC, 0xFF }, { 0xFA, 0xFF, 0xF8 }, { 0xF9, 0xFF, 0xFC },
		{ 0xF8, 0xFF, 0xFF }, { 0xFE, 0xFF, 0xFD }, { 0xFF, 0xFD, 0xFF }, { 0xFF, 0xFD, 0xFD },
		{ 0xF9, 0xFF, 0xFC }, { 0xFE, 0xFF, 0xFD }, { 0xFE, 0xFF, 0xFD }, { 0xFF, 0xFF, 0xFF },
		{ 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF },
	};

	for (x = MIN_WCRD_X; x <= MAX_WCRD_X; x += 10)
		for (y = MIN_WCRD_Y; y <= MAX_WCRD_Y; y += 10)
			mdnie_coordinate_tune_rgb(mdnie, x, y, tune_rgb);

	for (i = 0; i < 27; i++) {
		g_coord_x = input[i][0];
		g_coord_y = input[i][1];
		mdnie_update(mdnie);
		panel_info("compare %02X %02X %02X : %02X %02X %02X (%s)\n",
				output[i][0], output[i][1], output[i][2],
				mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][0],
				mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][1],
				mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][2],
				output[i][0] == mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][0] &&
				output[i][1] == mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][1] &&
				output[i][2] == mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][2] ?  "SUCCESS" : "FAILED");
	}
}
#endif

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.mode);
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value = 0;
	int ret;

	ret = kstrtouint(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (value >= MODE_MAX) {
		dev_err(dev, "%s: invalid value=%d\n", __func__, value);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	mdnie->props.mode = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t scenario_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.scenario);
}

static ssize_t scenario_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtouint(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (!SCENARIO_IS_VALID(value)) {
		dev_err(dev, "%s: invalid scenario %d\n", __func__, value);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	mdnie->props.scenario = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t accessibility_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.accessibility);
}

static ssize_t accessibility_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int s[12] = {0, };
	int i, value = 0, ret;

	ret = sscanf(buf, "%d %x %x %x %x %x %x %x %x %x %x %x %x",
		&value, &s[0], &s[1], &s[2], &s[3],
		&s[4], &s[5], &s[6], &s[7], &s[8], &s[9], &s[10], &s[11]);

	if (ret <= 0 || ret > ARRAY_SIZE(s) + 1 ||
			((ret - 1) > (MAX_MDNIE_SCR_LEN / 2))) {
		dev_err(dev, "%s: invalid size %d\n", __func__, ret);
		return -EINVAL;
	}

	if (value < 0 || value >= ACCESSIBILITY_MAX) {
		dev_err(dev, "%s: unknown accessibility %d\n", __func__, value);
		return -EINVAL;
	}

	dev_info(dev, "%s: value: %d, cnt: %d\n", __func__, value, ret);

	mutex_lock(&mdnie->lock);
	mdnie->props.accessibility = value;
	if (ret > 1 && (value == COLOR_BLIND || value == COLOR_BLIND_HBM)) {
		for (i = 0; i < ret - 1; i++) {
			mdnie->props.scr[i * 2 + 0] = GET_LSB_8BIT(s[i]);
			mdnie->props.scr[i * 2 + 1] = GET_MSB_8BIT(s[i]);
		}
		mdnie->props.sz_scr = (ret - 1) * 2;
		mdnie_maptbl_init(mdnie,
			MAPTBL_IDX_ACCESSIBILITY(mdnie->props.accessibility));
	}
	mutex_unlock(&mdnie->lock);

	dev_info(dev, "%s: %s\n", __func__, buf);
	mdnie_update(mdnie);

	return count;
}

static ssize_t bypass_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.bypass);
}

static ssize_t bypass_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtouint(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (value >= BYPASS_MAX) {
		dev_err(dev, "%s: invalid value=%d\n", __func__, value);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value);

	value = (value) ? BYPASS_ON : BYPASS_OFF;

	mutex_lock(&mdnie->lock);
	mdnie->props.bypass = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t lux_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.hbm);
}

static ssize_t lux_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int ret, value;

	ret = kstrtoint(buf, 0, &value);
	if (ret < 0)
		return ret;

	dev_info(dev, "%s: %d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	/* TODO : IS HBM ON CONDITION ALWAYS 10000 LUX ? */
	mdnie->props.hbm = (value < 40000) ? 0 : 1;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

/* Temporary solution: Do not use this sysfs as official purpose */
static ssize_t mdnie_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int maptbl_index = mdnie_get_maptbl_index(mdnie);
	int mdnie_mode = mdnie_current_state(mdnie);
	unsigned int i, len = 0;

	if (!IS_MDNIE_ENABLED(mdnie)) {
		dev_err(mdnie->dev, "mdnie state is off\n");
		return -EINVAL;
	}

	len += snprintf(buf + len, PAGE_SIZE - len,
			"mdnie %s-mode, maptbl %s(%d)\n",
			mdnie_mode_name[mdnie_mode], (maptbl_index < 0) ?
			"invalid" : mdnie_maptbl_name[maptbl_index], maptbl_index);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"accessibility %s(%d), hdr %d, hmd %d, hbm %d\n",
			accessibility_name[mdnie->props.accessibility],
			mdnie->props.accessibility, mdnie->props.hdr,
			mdnie->props.hmd, mdnie->props.hbm);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"scenario %s(%d), mode %s(%d)\n",
			scenario_name[mdnie->props.scenario], mdnie->props.scenario,
			scenario_mode_name[mdnie->props.mode], mdnie->props.mode);
	len += snprintf(buf + len, PAGE_SIZE - len, "scr_white_mode %s\n",
			scr_white_mode_name[mdnie->props.scr_white_mode]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"mdnie_ldu %d, coord x %d, y %d area Q%d\n",
			mdnie->props.ldu, mdnie->props.wcrd_x, mdnie->props.wcrd_y,
			mdnie_coordinate_area(mdnie, mdnie->props.wcrd_x, mdnie->props.wcrd_y) + 1);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"coord_wrgb[adpt] r:%d(%02X) g:%d(%02X) b:%d(%02X)\n",
			mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][0],
			mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][0],
			mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][1],
			mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][1],
			mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][2],
			mdnie->props.coord_wrgb[WCRD_TYPE_ADAPTIVE][2]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"coord_wrgb[d65] r:%d(%02X) g:%d(%02X) b:%d(%02X)\n",
			mdnie->props.coord_wrgb[WCRD_TYPE_D65][0],
			mdnie->props.coord_wrgb[WCRD_TYPE_D65][0],
			mdnie->props.coord_wrgb[WCRD_TYPE_D65][1],
			mdnie->props.coord_wrgb[WCRD_TYPE_D65][1],
			mdnie->props.coord_wrgb[WCRD_TYPE_D65][2],
			mdnie->props.coord_wrgb[WCRD_TYPE_D65][2]);
	len += snprintf(buf + len, PAGE_SIZE - len, "cur_wrgb r:%d(%02X) g:%d(%02X) b:%d(%02X)\n",
			mdnie->props.cur_wrgb[0], mdnie->props.cur_wrgb[0],
			mdnie->props.cur_wrgb[1], mdnie->props.cur_wrgb[1],
			mdnie->props.cur_wrgb[2], mdnie->props.cur_wrgb[2]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"ssr_wrgb r:%d(%02X) g:%d(%02X) b:%d(%02X)\n",
			mdnie->props.ssr_wrgb[0], mdnie->props.ssr_wrgb[0],
			mdnie->props.ssr_wrgb[1], mdnie->props.ssr_wrgb[1],
			mdnie->props.ssr_wrgb[2], mdnie->props.ssr_wrgb[2]);
	len += snprintf(buf + len, PAGE_SIZE - len,
			"def_wrgb r:%d(%02X) g:%d(%02X) b:%d(%02X) offset r:%d g:%d b:%d\n",
			mdnie->props.def_wrgb[0], mdnie->props.def_wrgb[0],
			mdnie->props.def_wrgb[1], mdnie->props.def_wrgb[1],
			mdnie->props.def_wrgb[2], mdnie->props.def_wrgb[2],
			mdnie->props.def_wrgb_ofs[0], mdnie->props.def_wrgb_ofs[1],
			mdnie->props.def_wrgb_ofs[2]);

	len += snprintf(buf + len, PAGE_SIZE - len, "scr : ");
	if (mdnie->props.sz_scr) {
		for (i = 0; i < mdnie->props.sz_scr; i++)
			len += snprintf(buf + len, PAGE_SIZE - len,
					"%02x ", mdnie->props.scr[i]);
	} else {
		len += snprintf(buf + len, PAGE_SIZE - len, "none");
	}
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");


	len += snprintf(buf + len, PAGE_SIZE - len,
			"night_mode %s, level %d\n",
			mdnie->props.night ? "on" : "off",
			mdnie->props.night_level);

#ifdef MDNIE_SELF_TEST
	mdnie_coordinate_tune_test(mdnie);
#endif

	return len;
}

static ssize_t sensorRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d %d %d\n", mdnie->props.cur_wrgb[0],
			mdnie->props.cur_wrgb[1], mdnie->props.cur_wrgb[2]);
}

static ssize_t sensorRGB_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int white_red, white_green, white_blue;
	int mdnie_mode = mdnie_current_state(mdnie), ret;

	ret = sscanf(buf, "%d %d %d",
		&white_red, &white_green, &white_blue);
	if (ret != 3)
		return -EINVAL;

	 dev_info(dev, "%s: white_r %d, white_g %d, white_b %d\n",
			 __func__, white_red, white_green, white_blue);

	if (mdnie_mode == MDNIE_SCENARIO_MODE &&
			mdnie->props.mode == AUTO &&
		(mdnie->props.scenario == BROWSER_MODE ||
		 mdnie->props.scenario == EBOOK_MODE)) {
		mutex_lock(&mdnie->lock);
		mdnie->props.ssr_wrgb[0] = white_red;
		mdnie->props.ssr_wrgb[1] = white_green;
		mdnie->props.ssr_wrgb[2] = white_blue;
		mdnie->props.update_sensorRGB = true;
		scr_white_maptbl_init(mdnie, MDNIE_SENSOR_RGB_MAPTBL);
		mutex_unlock(&mdnie->lock);
		mdnie_update(mdnie);
	}

	return count;
}

static ssize_t whiteRGB_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d %d %d\n", mdnie->props.def_wrgb_ofs[0],
			mdnie->props.def_wrgb_ofs[1], mdnie->props.def_wrgb_ofs[2]);
}

static ssize_t whiteRGB_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int wr_offset, wg_offset, wb_offset;
	int ret;

	ret = sscanf(buf, "%d %d %d",
		&wr_offset, &wg_offset, &wb_offset);
	if (ret != 3)
		return -EINVAL;

	if (!IS_VALID_WRGB_OFS(wr_offset) ||
			!IS_VALID_WRGB_OFS(wg_offset) ||
			!IS_VALID_WRGB_OFS(wb_offset)) {
		dev_err(dev, "%s: invalid offset %d %d %d\n",
				__func__, wr_offset, wg_offset, wb_offset);
		return -EINVAL;
	}

	dev_info(dev, "%s: wr_offset %d, wg_offset %d, wb_offset %d\n",
		 __func__, wr_offset, wg_offset, wb_offset);

	mutex_lock(&mdnie->lock);
	mdnie->props.def_wrgb_ofs[0] = wr_offset;
	mdnie->props.def_wrgb_ofs[1] = wg_offset;
	mdnie->props.def_wrgb_ofs[2] = wb_offset;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t night_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d %d\n",
			mdnie->props.night, mdnie->props.night_level);
}

static ssize_t night_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int enable, level, ret;

	ret = sscanf(buf, "%d %d", &enable, &level);
	if (ret != 2)
		return -EINVAL;

	if (level < 0 || level >= mdnie->props.num_night_level)
		return -EINVAL;

	dev_info(dev, "%s: night_mode %s level %d\n",
			__func__, enable ? "on" : "off", level);

	mutex_lock(&mdnie->lock);
	mdnie->props.night = !!enable;
	mdnie->props.night_level = level;
	if (enable) {
		/* MDNIE_NIGHT_MAPTBL update using MDNIE_ETC_NIGHT_MAPTBL */
		mdnie_maptbl_init(mdnie, MDNIE_NIGHT_MAPTBL);
	}
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t color_lens_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d %d %d\n",
			mdnie->props.color_lens,
			mdnie->props.color_lens_color,
			mdnie->props.color_lens_level);
}

static ssize_t color_lens_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int enable, level, color, ret;

	ret = sscanf(buf, "%d %d %d", &enable, &color, &level);
	if (ret != 3)
		return -EINVAL;

	if (color < 0 || color >= COLOR_LENS_COLOR_MAX)
		return -EINVAL;

	if (level < 0 || level >= COLOR_LENS_LEVEL_MAX)
		return -EINVAL;

	dev_info(dev, "%s: color_lens_mode %s color %d level %d\n",
			__func__, enable ? "on" : "off", color, level);

	mutex_lock(&mdnie->lock);
	mdnie->props.color_lens = !!enable;
	mdnie->props.color_lens_color = color;
	mdnie->props.color_lens_level = level;
	if (enable) {
		/* MDNIE_COLOR_LENS_MAPTBL update using MDNIE_ETC_COLOR_LENS_MAPTBL */
		mdnie_maptbl_init(mdnie, MDNIE_COLOR_LENS_MAPTBL);
	}
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t hdr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.hdr);
}

static ssize_t hdr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtouint(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (value >= HDR_MAX) {
		dev_err(dev, "%s: invalid value=%d\n", __func__, value);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	mdnie->props.hdr = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t light_notification_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", mdnie->props.light_notification);
}

static ssize_t light_notification_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtouint(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (value >= LIGHT_NOTIFICATION_MAX)
		return -EINVAL;

	dev_info(dev, "%s: value=%d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	mdnie->props.light_notification = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

static ssize_t mdnie_ldu_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d %d %d\n", mdnie->props.cur_wrgb[0],
			mdnie->props.cur_wrgb[1], mdnie->props.cur_wrgb[2]);
}

static ssize_t mdnie_ldu_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int value, ret;

	ret = kstrtoint(buf, 10, &value);
	if (ret < 0)
		return ret;

	if (value < 0 || value >= MAX_LDU_MODE) {
		dev_err(dev, "%s: out of range %d\n",
				__func__, value);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	mdnie->props.ldu = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}

#ifdef CONFIG_SUPPORT_HMD
static ssize_t hmt_color_temperature_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "hmd_mode: %d\n", mdnie->props.hmd);
}

static ssize_t hmt_color_temperature_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int value;
	int ret;

	ret = kstrtouint(buf, 0, &value);
	if (ret < 0)
		return ret;

	if (value >= HMD_MDNIE_MAX)
		return -EINVAL;

	if (value == mdnie->props.hmd)
		return count;

	dev_info(dev, "%s: value=%d\n", __func__, value);

	mutex_lock(&mdnie->lock);
	mdnie->props.hmd = value;
	mutex_unlock(&mdnie->lock);
	mdnie_update(mdnie);

	return count;
}
#endif

#ifdef CONFIG_SUPPORT_AFC
static ssize_t afc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	int len = 0;
	size_t i;

	len += snprintf(buf + len, PAGE_SIZE - len,
			"%d", mdnie->props.afc_on);
	for (i = 0; i < ARRAY_SIZE(mdnie->props.afc_roi); i++)
		len += snprintf(buf + len, PAGE_SIZE - len,
				" %d", mdnie->props.afc_roi[i]);
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return len;
}

static ssize_t afc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct mdnie_info *mdnie = dev_get_drvdata(dev);
	unsigned int s[12] = {0, };
	int value = 0, ret;
	size_t i;

	ret = sscanf(buf, "%i %i %i %i %i %i %i %i %i %i %i %i %i",
		&value, &s[0], &s[1], &s[2], &s[3],
		&s[4], &s[5], &s[6], &s[7], &s[8], &s[9], &s[10], &s[11]);

	if ((ret != 1 && ret != ARRAY_SIZE(s) + 1) ||
			ARRAY_SIZE(s) != MAX_AFC_ROI_LEN) {
		dev_err(dev, "%s: invalid size %d\n", __func__, ret);
		return -EINVAL;
	}

	dev_info(dev, "%s: value=%d, cnt=%d\n", __func__, value, ret);

	mutex_lock(&mdnie->lock);
	mdnie->props.afc_on = !!value;
	for (i = 0; i < ARRAY_SIZE(mdnie->props.afc_roi); i++)
		mdnie->props.afc_roi[i] = s[i] & 0xFF;
	mutex_unlock(&mdnie->lock);

	dev_info(dev, "%s: %s\n", __func__, buf);
	mdnie_update(mdnie);

	return count;
}
#endif

struct device_attribute mdnie_dev_attrs[] = {
	__PANEL_ATTR_RW(mode, 0664),
	__PANEL_ATTR_RW(scenario, 0664),
	__PANEL_ATTR_RW(accessibility, 0664),
	__PANEL_ATTR_RW(bypass, 0664),
	__PANEL_ATTR_RW(lux, 0664),
	__PANEL_ATTR_RO(mdnie, 0444),
	__PANEL_ATTR_RW(sensorRGB, 0664),
	__PANEL_ATTR_RW(whiteRGB, 0664),
	__PANEL_ATTR_RW(night_mode, 0664),
	__PANEL_ATTR_RW(color_lens, 0664),
	__PANEL_ATTR_RW(hdr, 0664),
	__PANEL_ATTR_RW(light_notification, 0664),
	__PANEL_ATTR_RW(mdnie_ldu, 0664),
#ifdef CONFIG_SUPPORT_HMD
	__PANEL_ATTR_RW(hmt_color_temperature, 0664),
#endif
#ifdef CONFIG_SUPPORT_AFC
	__PANEL_ATTR_RW(afc, 0664),
#endif
};

int mdnie_enable(struct mdnie_info *mdnie)
{
	struct panel_device *panel =
		container_of(mdnie, struct panel_device, mdnie);

	if (IS_MDNIE_ENABLED(mdnie)) {
		dev_info(mdnie->dev, "mdnie already enabled\n");
		return 0;
	}

	mutex_lock(&mdnie->lock);
	mdnie->props.enable = 1;
	mdnie->props.light_notification = LIGHT_NOTIFICATION_OFF;
	if (IS_HBM_MODE(mdnie))
		mdnie->props.trans_mode = TRANS_ON;
	mutex_unlock(&mdnie->lock);
	panel_mdnie_update(panel);
	mutex_lock(&mdnie->lock);
	mdnie->props.trans_mode = TRANS_ON;
	mutex_unlock(&mdnie->lock);

	dev_info(mdnie->dev, "%s: done\n", __func__);

	return 0;
}

int mdnie_disable(struct mdnie_info *mdnie)
{
	if (!IS_MDNIE_ENABLED(mdnie)) {
		dev_info(mdnie->dev, "mdnie already disabled\n");
		return 0;
	}

	mutex_lock(&mdnie->lock);
	mdnie->props.enable = 0;
	mdnie->props.trans_mode = TRANS_OFF;
	mdnie->props.update_sensorRGB = false;
	mutex_unlock(&mdnie->lock);

	dev_info(mdnie->dev, "%s: done\n", __func__);

	return 0;
}

static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct mdnie_info *mdnie;
	struct fb_event *evdata = data;
	int fb_blank;

	switch (event) {
	case FB_EVENT_BLANK:
		break;
	default:
		return 0;
	}

	mdnie = container_of(self, struct mdnie_info, fb_notif);

	fb_blank = *(int *)evdata->data;

	pr_debug("%s: %d\n", __func__, fb_blank);

	if (evdata->info->node != 0)
		return 0;

	if (fb_blank == FB_BLANK_UNBLANK) {
		mutex_lock(&mdnie->lock);
		mdnie->props.light_notification = LIGHT_NOTIFICATION_OFF;
		if (IS_HBM_MODE(mdnie))
			mdnie->props.trans_mode = TRANS_ON;
		mutex_unlock(&mdnie->lock);
		mdnie_update(mdnie);
	}

	return 0;
}

static int mdnie_register_fb(struct mdnie_info *mdnie)
{
	memset(&mdnie->fb_notif, 0, sizeof(mdnie->fb_notif));
	mdnie->fb_notif.notifier_call = fb_notifier_callback;
	return fb_register_client(&mdnie->fb_notif);
}

#ifdef CONFIG_DISPLAY_USE_INFO
#define MDNIE_WOFS_ORG_PATH ("/efs/FactoryApp/mdnie")
static int mdnie_get_efs(char *filename, int *value)
{
	mm_segment_t old_fs;
	struct file *filp = NULL;
	int fsize = 0, nread, rc, ret = 0;
	u8 buf[128];

	if (!filename || !value) {
		pr_err("%s invalid parameter\n", __func__);
		return -EINVAL;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0440);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		if (ret == -ENOENT)
			pr_err("%s file(%s) not exist\n", __func__, filename);
		else
			pr_info("%s file(%s) open error(ret %d)\n",
					__func__, filename, ret);
		set_fs(old_fs);
		return -ENOENT;
	}

	if (filp->f_path.dentry && filp->f_path.dentry->d_inode)
		fsize = filp->f_path.dentry->d_inode->i_size;

	if (fsize == 0 || fsize > ARRAY_SIZE(buf)) {
		pr_err("%s invalid file(%s) size %d\n",
				__func__, filename, fsize);
		ret = -EEXIST;
		goto exit;
	}

	memset(buf, 0, sizeof(buf));
	nread = vfs_read(filp, (char __user *)buf, fsize, &filp->f_pos);
	if (nread != fsize) {
		pr_err("%s failed to read (ret %d)\n", __func__, nread);
		ret = -EIO;
		goto exit;
	}

	rc = sscanf(buf, "%d %d %d", &value[0], &value[1], &value[2]);
	if (rc != 3) {
		pr_err("%s failed to kstrtoint %d\n", __func__, rc);
		ret = -EINVAL;
		goto exit;
	}

	pr_info("%s %s(size %d) : %d %d %d\n",
			__func__, filename, fsize, value[0], value[1], value[2]);

exit:
	filp_close(filp, current->files);
	set_fs(old_fs);

	return ret;
}

static int dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct mdnie_info *mdnie;
	struct panel_device *panel;
	struct panel_info *panel_data;
	char tbuf[MAX_DPUI_VAL_LEN];
	u8 coordinate[PANEL_COORD_LEN] = { 0, };
	int def_wrgb_ofs_org[3] = { 0, };
	int size;

	mdnie = container_of(self, struct mdnie_info, dpui_notif);
	panel = container_of(mdnie, struct panel_device, mdnie);
	panel_data = &panel->panel_data;

	mutex_lock(&mdnie->lock);

	resource_copy_by_name(panel_data, coordinate, "coordinate");
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			(coordinate[0] << 8) | coordinate[1]);
	set_dpui_field(DPUI_KEY_WCRD_X, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			(coordinate[2] << 8) | coordinate[3]);
	set_dpui_field(DPUI_KEY_WCRD_Y, tbuf, size);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			mdnie->props.def_wrgb_ofs[0]);
	set_dpui_field(DPUI_KEY_WOFS_R, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			mdnie->props.def_wrgb_ofs[1]);
	set_dpui_field(DPUI_KEY_WOFS_G, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d",
			mdnie->props.def_wrgb_ofs[2]);
	set_dpui_field(DPUI_KEY_WOFS_B, tbuf, size);

	mdnie_get_efs(MDNIE_WOFS_ORG_PATH, def_wrgb_ofs_org);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", def_wrgb_ofs_org[0]);
	set_dpui_field(DPUI_KEY_WOFS_R_ORG, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", def_wrgb_ofs_org[1]);
	set_dpui_field(DPUI_KEY_WOFS_G_ORG, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", def_wrgb_ofs_org[2]);
	set_dpui_field(DPUI_KEY_WOFS_B_ORG, tbuf, size);

	mutex_unlock(&mdnie->lock);

	return 0;
}

static int mdnie_register_dpui(struct mdnie_info *mdnie)
{
	memset(&mdnie->dpui_notif, 0, sizeof(mdnie->dpui_notif));
	mdnie->dpui_notif.notifier_call = dpui_notifier_callback;
	return dpui_logging_register(&mdnie->dpui_notif, DPUI_TYPE_PANEL);
}
#endif /* CONFIG_DISPLAY_USE_INFO */

int mdnie_probe(struct panel_device *panel, struct mdnie_tune *mdnie_tune)
{
	struct mdnie_info *mdnie = &panel->mdnie;
	size_t i;
	int ret = 0;

	if (unlikely(!mdnie_tune)) {
		pr_err("%s, mdnie_tune not exist\n", __func__);
		return -ENODEV;
	}

	mdnie->class = class_create(THIS_MODULE, "mdnie");
	if (IS_ERR_OR_NULL(mdnie->class)) {
		pr_err("failed to create mdnie class\n");
		ret = -EINVAL;
		goto error0;
	}

	mdnie->dev = device_create(mdnie->class,
			&panel->lcd->dev, 0, &mdnie, "mdnie");
	if (IS_ERR_OR_NULL(mdnie->dev)) {
		pr_err("failed to create mdnie device\n");
		ret = -EINVAL;
		goto error1;
	}

	dev_set_drvdata(mdnie->dev, mdnie);

	mutex_lock(&mdnie->lock);
	mdnie->props.enable = 0;
	mdnie->props.scenario = UI_MODE;
	mdnie->props.mode = AUTO;
	mdnie->props.tuning = 0;
	mdnie->props.bypass = BYPASS_OFF;
	mdnie->props.hdr = HDR_OFF;
	mdnie->props.light_notification = LIGHT_NOTIFICATION_OFF;
	mdnie->props.hmd = HMD_MDNIE_OFF;
	mdnie->props.night = NIGHT_MODE_OFF;
	mdnie->props.night_level = NIGHT_LEVEL_6500K;
	mdnie->props.color_lens = COLOR_LENS_OFF;
	mdnie->props.color_lens_color = COLOR_LENS_COLOR_BLUE;
	mdnie->props.color_lens_level = COLOR_LENS_LEVEL_20P;
	mdnie->props.accessibility = ACCESSIBILITY_OFF;
	mdnie->props.ldu = LDU_MODE_OFF;
	mdnie->props.scr_white_mode = SCR_WHITE_MODE_NONE;
	mdnie->props.trans_mode = TRANS_ON;
	mdnie->props.update_sensorRGB = false;

	mdnie->props.sz_scr = 0;

	mdnie->props.num_ldu_mode = mdnie_tune->num_ldu_mode;
	mdnie->props.num_night_level = mdnie_tune->num_night_level;
	mdnie->props.num_color_lens_color = mdnie_tune->num_color_lens_color;
	mdnie->props.num_color_lens_level = mdnie_tune->num_color_lens_level;
	memcpy(mdnie->props.line, mdnie_tune->line, sizeof(mdnie->props.line));
	memcpy(mdnie->props.coef, mdnie_tune->coef, sizeof(mdnie->props.coef));
	memcpy(mdnie->props.vtx, mdnie_tune->vtx, sizeof(mdnie->props.vtx));
	mdnie->props.cal_x_center = mdnie_tune->cal_x_center;
	mdnie->props.cal_y_center = mdnie_tune->cal_y_center;
	mdnie->props.cal_boundary_center = mdnie_tune->cal_boundary_center;

	mdnie->seqtbl = mdnie_tune->seqtbl;
	mdnie->nr_seqtbl = mdnie_tune->nr_seqtbl;
	mdnie->etc_maptbl = mdnie_tune->etc_maptbl;
	mdnie->nr_etc_maptbl = mdnie_tune->nr_etc_maptbl;
	mdnie->maptbl = mdnie_tune->maptbl;
	mdnie->nr_maptbl = mdnie_tune->nr_maptbl;
	mdnie->scr_white_maptbl = mdnie_tune->scr_white_maptbl;
	mdnie->nr_scr_white_maptbl = mdnie_tune->nr_scr_white_maptbl;
#ifdef CONFIG_SUPPORT_AFC
	mdnie->afc_maptbl = mdnie_tune->afc_maptbl;
	mdnie->nr_afc_maptbl = mdnie_tune->nr_afc_maptbl;
#endif

	if (mdnie->nr_maptbl % MAX_MDNIE_MAPTBL) {
		pr_err("%s invalid size of maptbl %d\n",
				__func__, mdnie->nr_maptbl);
		ret = -EINVAL;
		goto error2;
	}

	mdnie->nr_reg = mdnie->nr_maptbl / MAX_MDNIE_MAPTBL;
	mdnie_coordinate_tune(mdnie);

	for (i = 0; i < mdnie->nr_etc_maptbl; i++) {
		mdnie->etc_maptbl[i].pdata = mdnie;
		maptbl_init(&mdnie->etc_maptbl[i]);
	}

	for (i = 0; i < mdnie->nr_maptbl; i++) {
		mdnie->maptbl[i].pdata = mdnie;
		maptbl_init(&mdnie->maptbl[i]);
	}

	for (i = 0; i < mdnie->nr_scr_white_maptbl; i++) {
		mdnie->scr_white_maptbl[i].pdata = mdnie;
		maptbl_init(&mdnie->scr_white_maptbl[i]);
	}

#ifdef CONFIG_SUPPORT_AFC
	for (i = 0; i < mdnie->nr_afc_maptbl; i++) {
		mdnie->afc_maptbl[i].pdata = mdnie;
		maptbl_init(&mdnie->afc_maptbl[i]);
	}
#endif

	for (i = 0; i < ARRAY_SIZE(mdnie_dev_attrs); i++) {
		ret = device_create_file(mdnie->dev, &mdnie_dev_attrs[i]);
		if (ret < 0) {
			dev_err(mdnie->dev, "%s: failed to add %s sysfs entries, %d\n",
					__func__, mdnie_dev_attrs[i].attr.name, ret);
			goto error2;
		}
	}

	mdnie_register_fb(mdnie);
#ifdef CONFIG_DISPLAY_USE_INFO
	mdnie_register_dpui(mdnie);
#endif
	mutex_unlock(&mdnie->lock);
	mdnie_enable(mdnie);
	dev_info(mdnie->dev, "registered successfully\n");

	return 0;

error2:
	mutex_unlock(&mdnie->lock);
error1:
	class_destroy(mdnie->class);
error0:
	pr_err("%s failed to probe mdnie\n", __func__);
	return ret;
}

static int attr_find_and_store(struct device *dev,
	const char *name, const char *buf, size_t size)
{
	struct device_attribute *dev_attr;
	struct kernfs_node *kn;
	struct attribute *attr;

	kn = kernfs_find_and_get(dev->kobj.sd, name);
	if (!kn) {
		pr_info("%s: not found: %s\n", __func__, name);
		return 0;
	}

	attr = kn->priv;
	dev_attr = container_of(attr, struct device_attribute, attr);

	if (dev_attr && dev_attr->store)
		dev_attr->store(dev, dev_attr, buf, size);

	kernfs_put(kn);

	return 0;
}

ssize_t attr_store_for_each(struct class *cls,
	const char *name, const char *buf, size_t size)
{
	struct class_dev_iter iter;
	struct device *dev;
	int error = 0;
	struct class *class = cls;

	if (!class)
		return -EINVAL;
	if (!class->p) {
		WARN(1, "%s called for class '%s' before it was initialized",
				__func__, class->name);
		return -EINVAL;
	}

	class_dev_iter_init(&iter, class, NULL, NULL);
	while ((dev = class_dev_iter_next(&iter))) {
		error = attr_find_and_store(dev, name, buf, size);
		if (error)
			break;
	}
	class_dev_iter_exit(&iter);

	return error;
}
