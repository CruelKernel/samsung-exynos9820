/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MDNIE_H__
#define __MDNIE_H__

#include "panel.h"
#include "dpui.h"

typedef u8 mdnie_t;

#define MDNIE_SYSFS_PREFIX      "/sdcard/mdnie/"

#define IS_MDNIE_ENABLED(_mdnie_)	(!!(_mdnie_)->props.enable)
#define IS_DMB(idx)			(idx == DMB_NORMAL_MODE)
#define IS_SCENARIO(idx)		(idx < SCENARIO_MAX && !(idx > VIDEO_NORMAL_MODE && idx < CAMERA_MODE))
#define IS_ACCESSIBILITY(idx)		(idx && idx < ACCESSIBILITY_MAX)
#define IS_HBM(idx)			(idx && idx < HBM_MAX)
#define IS_HMD(idx)			(idx && idx < HMD_MDNIE_MAX)

#define SCENARIO_IS_VALID(idx)		(IS_DMB(idx) || IS_SCENARIO(idx))
#define IS_NIGHT_MODE_ON(idx)		(idx == NIGHT_MODE_ON)
#define IS_COLOR_LENS_ON(idx)		(idx == COLOR_LENS_ON)
#define IS_HDR(idx)			(idx >= HDR_ON && idx < HDR_MAX)
#define IS_LIGHT_NOTIFICATION(idx)			(idx >= LIGHT_NOTIFICATION_ON && idx < LIGHT_NOTIFICATION_MAX)

#define COLOR_OFFSET_FUNC(x, y, num, den, con)  \
	    (((s64)y * 1024LL) + ((((s64)x * 1024LL) * ((s64)num)) / ((s64)den)) + ((s64)con * 1024LL))
#define RECOGNIZE_REGION(compV, compH)	\
		((((compV) > 0) ? (0 + (((compH) > 0) ? 0 : 1)) : (2 + (((compH) < 0) ? 0 : 1))))

#define MAPTBL_IDX_SCENARIO(_idx_)	(MDNIE_SCENARIO_MAPTBL_START + (_idx_))
#define MAPTBL_IDX_ACCESSIBILITY(_idx_)	(MDNIE_ACCESSIBILITY_MAPTBL_START + (_idx_) - 1)

#define IS_BYPASS_MODE(_mdnie_)	(((_mdnie_)->props.bypass >= BYPASS_ON) && ((_mdnie_)->props.bypass < BYPASS_MAX))
#define IS_ACCESSIBILITY_MODE(_mdnie_)	(((_mdnie_)->props.accessibility > ACCESSIBILITY_OFF) && ((_mdnie_)->props.accessibility < ACCESSIBILITY_MAX))
#define IS_LIGHT_NOTIFICATION_MODE(_mdnie_)			(((_mdnie_)->props.light_notification >= LIGHT_NOTIFICATION_ON) && ((_mdnie_)->props.light_notification < LIGHT_NOTIFICATION_MAX))
#define IS_HDR_MODE(_mdnie_)			(((_mdnie_)->props.hdr >= HDR_ON) && ((_mdnie_)->props.hdr < HDR_MAX))
#define IS_HMD_MODE(_mdnie_)			(((_mdnie_)->props.hmd >= HMD_MDNIE_ON) && ((_mdnie_)->props.hmd < HMD_MDNIE_MAX))
#define IS_NIGHT_MODE(_mdnie_)			(((_mdnie_)->props.night >= NIGHT_MODE_ON) && ((_mdnie_)->props.night < NIGHT_MODE_MAX))
#define IS_COLOR_LENS_MODE(_mdnie_)		(((_mdnie_)->props.color_lens >= COLOR_LENS_ON) && ((_mdnie_)->props.color_lens < COLOR_LENS_MAX))
#define IS_HBM_MODE(_mdnie_)			(((_mdnie_)->props.hbm >= HBM_ON) && ((_mdnie_)->props.hbm < HBM_MAX))
#define IS_DMB_MODE(_mdnie_)			((_mdnie_)->props.scenario == DMB_NORMAL_MODE)
#define IS_SCENARIO_MODE(_mdnie_)		(((_mdnie_)->props.scenario >= UI_MODE && (_mdnie_)->props.scenario <= VIDEO_NORMAL_MODE) || \
		((_mdnie_)->props.scenario >= CAMERA_MODE && (_mdnie_)->props.scenario < SCENARIO_MAX))
#define IS_LDU_MODE(_mdnie_)			(((_mdnie_)->props.ldu > LDU_MODE_OFF) && ((_mdnie_)->props.ldu < MAX_LDU_MODE))

#define MAX_MDNIE_SCR_LEN	(24)
#define MAX_COLOR (3)

#define GET_MSB_8BIT(x)		((x >> 8) & (BIT(8) - 1))
#define GET_LSB_8BIT(x)		((x >> 0) & (BIT(8) - 1))

#define MIN_WRGB_OFS	(-40)
#define MAX_WRGB_OFS	(0)
#define IS_VALID_WRGB_OFS(_ofs_)	(((int)(_ofs_) <= MAX_WRGB_OFS) && ((int)(_ofs_) >= (MIN_WRGB_OFS)))

#define MIN_WCRD_X	(2930)
#define MAX_WCRD_X	(3060)
#define MIN_WCRD_Y	(3050)
#define MAX_WCRD_Y	(3260)

#ifdef CONFIG_SUPPORT_AFC
#define MAX_AFC_ROI_LEN	(12)
#endif

enum {
	MDNIE_OFF_MODE,
	MDNIE_BYPASS_MODE,
	MDNIE_ACCESSIBILITY_MODE,
	MDNIE_LIGHT_NOTIFICATION_MODE,
	MDNIE_COLOR_LENS_MODE,
	MDNIE_HDR_MODE,
	MDNIE_HMD_MODE,
	MDNIE_NIGHT_MODE,
	MDNIE_HBM_MODE,
	MDNIE_DMB_MODE,
	MDNIE_SCENARIO_MODE,
	MAX_MDNIE_MODE,
};

enum SCENARIO_MODE {
	DYNAMIC,
	STANDARD,
	NATURAL,
	MOVIE,
	AUTO,
	MODE_MAX
};

enum SCENARIO {
	UI_MODE,
	VIDEO_NORMAL_MODE,
	CAMERA_MODE = 4,
	NAVI_MODE,
	GALLERY_MODE,
	VT_MODE,
	BROWSER_MODE,
	EBOOK_MODE,
	EMAIL_MODE,
	GAME_LOW_MODE,
	GAME_MID_MODE,
	GAME_HIGH_MODE,
	VIDEO_ENHANCER,
	VIDEO_ENHANCER_THIRD,
	HMD_8_MODE,
	HMD_16_MODE,
	SCENARIO_MAX,
	DMB_NORMAL_MODE = 20,
	DMB_MODE_MAX
};

enum BYPASS {
	BYPASS_OFF = 0,
	BYPASS_ON,
	BYPASS_MAX
};

enum ACCESSIBILITY {
	ACCESSIBILITY_OFF,
	NEGATIVE,
	COLOR_BLIND,
	SCREEN_CURTAIN,
	GRAYSCALE,
	GRAYSCALE_NEGATIVE,
	COLOR_BLIND_HBM,
	ACCESSIBILITY_MAX
};

enum HBM {
	HBM_OFF,
	HBM_ON,
	HBM_MAX
};

enum HMD_MODE {
	HMD_MDNIE_OFF,
	HMD_MDNIE_ON,
	HMD_3000K = HMD_MDNIE_ON,
	HMD_4000K,
	HMD_5000K,
	HMD_6500K,
	HMD_7500K,
	HMD_MDNIE_MAX
};

enum NIGHT_MODE {
	NIGHT_MODE_OFF,
	NIGHT_MODE_ON,
	NIGHT_MODE_MAX
};

enum NIGHT_LEVEL {
	NIGHT_LEVEL_6500K,
	NIGHT_LEVEL_6100K,
	NIGHT_LEVEL_5700K,
	NIGHT_LEVEL_5300K,
	NIGHT_LEVEL_4900K,
	NIGHT_LEVEL_4500K,
	NIGHT_LEVEL_4100K,
	NIGHT_LEVEL_3700K,
	NIGHT_LEVEL_3300K,
	NIGHT_LEVEL_2900K,
	NIGHT_LEVEL_2500K,
	NIGHT_LEVEL_7000K,
	MAX_NIGHT_LEVEL,
};

enum COLOR_LENS {
	COLOR_LENS_OFF,
	COLOR_LENS_ON,
	COLOR_LENS_MAX
};

enum COLOR_LENS_COLOR {
	COLOR_LENS_COLOR_BLUE,
	COLOR_LENS_COLOR_AZURE,
	COLOR_LENS_COLOR_CYAN,
	COLOR_LENS_COLOR_SPRING_GREEN,
	COLOR_LENS_COLOR_GREEN,
	COLOR_LENS_COLOR_CHARTREUSE_GREEN,
	COLOR_LENS_COLOR_YELLOW,
	COLOR_LENS_COLOR_ORANGE,
	COLOR_LENS_COLOR_RED,
	COLOR_LENS_COLOR_ROSE,
	COLOR_LENS_COLOR_MAGENTA,
	COLOR_LENS_COLOR_VIOLET,
	COLOR_LENS_COLOR_MAX
};

enum COLOR_LENS_LEVEL {
	COLOR_LENS_LEVEL_20P,
	COLOR_LENS_LEVEL_25P,
	COLOR_LENS_LEVEL_30P,
	COLOR_LENS_LEVEL_35P,
	COLOR_LENS_LEVEL_40P,
	COLOR_LENS_LEVEL_45P,
	COLOR_LENS_LEVEL_50P,
	COLOR_LENS_LEVEL_55P,
	COLOR_LENS_LEVEL_60P,
	COLOR_LENS_LEVEL_MAX,
};

enum HDR {
	HDR_OFF,
	HDR_ON,
	HDR_1 = HDR_ON,
	HDR_2,
	HDR_3,
	HDR_MAX
};

enum LIGHT_NOTIFICATION {
	LIGHT_NOTIFICATION_OFF = 0,
	LIGHT_NOTIFICATION_ON,
	LIGHT_NOTIFICATION_MAX
};

enum {
	CCRD_PT_NONE,
	CCRD_PT_1,
	CCRD_PT_2,
	CCRD_PT_3,
	CCRD_PT_4,
	CCRD_PT_5,
	CCRD_PT_6,
	CCRD_PT_7,
	CCRD_PT_8,
	CCRD_PT_9,
	MAX_CCRD_PT,
};

enum LDU_MODE {
	LDU_MODE_OFF,
	LDU_MODE_1,
	LDU_MODE_2,
	LDU_MODE_3,
	LDU_MODE_4,
	LDU_MODE_5,
	MAX_LDU_MODE,
};

enum SCR_WHITE_MODE {
	SCR_WHITE_MODE_NONE,
	SCR_WHITE_MODE_COLOR_COORDINATE,
	SCR_WHITE_MODE_ADJUST_LDU,
	SCR_WHITE_MODE_SENSOR_RGB,
	MAX_SCR_WHITE_MODE,
};

enum TRANS_MODE {
	TRANS_OFF,
	TRANS_ON,
	MAX_TRANS_MODE,
};

enum MDNIE_MAPTBL {
	/* SCENARIO */
	MDNIE_SCENARIO_MAPTBL_START,
	MDNIE_UI_MAPTBL = MDNIE_SCENARIO_MAPTBL_START,
	MDNIE_VIDEO_MAPTBL,
	MDNIE_CAMERA_MAPTBL = 4,
	MDNIE_NAVI_MAPTBL,
	MDNIE_GALLERY_MAPTBL,
	MDNIE_VT_MAPTBL,
	MDNIE_BROWSER_MAPTBL,
	MDNIE_EBOOK_MAPTBL,
	MDNIE_EMAIL_MAPTBL,
	MDNIE_GAME_LOW_MAPTBL,
	MDNIE_GAME_MID_MAPTBL,
	MDNIE_GAME_HIGH_MAPTBL,
	MDNIE_VIDEO_ENHANCER_MAPTBL,
	MDNIE_VIDEO_ENHANCER_THIRD_MAPTBL,
	MDNIE_HMD_8_MAPTBL,
	MDNIE_HMD_16_MAPTBL,
#if defined(CONFIG_TDMB)
	MDNIE_DMB_MAPTBL,
	MDNIE_SCENARIO_MAPTBL_END = MDNIE_DMB_MAPTBL,
#else
	MDNIE_SCENARIO_MAPTBL_END = MDNIE_HMD_16_MAPTBL,
#endif
	/* ACCESSIBILITY */
	MDNIE_ACCESSIBILITY_MAPTBL_START,
	MDNIE_NEGATIVE_MAPTBL = MDNIE_ACCESSIBILITY_MAPTBL_START,
	MDNIE_COLOR_BLIND_MAPTBL,
	MDNIE_SCREEN_CURTAIN_MAPTBL,
	MDNIE_GRAYSCALE_MAPTBL,
	MDNIE_GRAYSCALE_NEGATIVE_MAPTBL,
	MDNIE_COLOR_BLIND_HBM_MAPTBL,
	MDNIE_ACCESSIBILITY_MAPTBL_END = MDNIE_COLOR_BLIND_HBM_MAPTBL,
	/* BYPASS */
	MDNIE_BYPASS_MAPTBL,
	/* HBM */
	MDNIE_HBM_MAPTBL,
	/* HMD */
	MDNIE_HMD_MAPTBL,
	/* HDR */
	MDNIE_HDR_MAPTBL,
	/* NIGHT */
	MDNIE_NIGHT_MAPTBL,
	/* LIGHT_NOTIFICATION */
	MDNIE_LIGHT_NOTIFICATION_MAPTBL,
	/* COLOR LENS */
	MDNIE_COLOR_LENS_MAPTBL,
	MAX_MDNIE_MAPTBL,
};

#ifdef CONFIG_SUPPORT_AFC
enum AFC_MAPTBL {
	AFC_ON_MAPTBL,
	MAX_AFC_MAPTBL,
};
#endif

enum MDNIE_SEQ {
	MDNIE_SET_SEQ,
#ifdef CONFIG_SUPPORT_AFC
	MDNIE_AFC_OFF_SEQ,
	MDNIE_AFC_ON_SEQ,
#endif
	/* if necessary, add new seq */
	MAX_MDNIE_SEQ,
};

enum {
	MDNIE_SCR_WHITE_NONE_MAPTBL,
	MDNIE_COLOR_COORDINATE_MAPTBL,
	MDNIE_ADJUST_LDU_MAPTBL,
	MDNIE_SENSOR_RGB_MAPTBL,
	MAX_SCR_WHITE_MAPTBL,
};

enum {
	MDNIE_ETC_NONE_MAPTBL,
	MDNIE_ETC_TRANS_MAPTBL,
	MDNIE_ETC_NIGHT_MAPTBL,
	MDNIE_ETC_COLOR_LENS_MAPTBL,
	MAX_MDNIE_ETC_MAPTBL,
};

enum {
	WCRD_TYPE_ADAPTIVE,
	WCRD_TYPE_D65,
	MAX_WCRD_TYPE,
};

enum {
	H_LINE,
	V_LINE,
	MAX_CAL_LINE,
};

enum {
	RGB_00,
	RGB_01,
	RGB_10,
	RGB_11,
	MAX_RGB_PT,
};

enum {
	QUAD_1,
	QUAD_2,
	QUAD_3,
	QUAD_4,
	MAX_QUAD,
};

struct cal_center {
	int x;
	int y;
	int thres;
};

struct cal_line {
	int num;
	int den;
	int con;
};

struct cal_coef {
	int a, b, c, d;
	int e, f, g, h;
};

struct mdnie_tune {
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct maptbl *scr_white_maptbl;
	u32 nr_scr_white_maptbl;
	struct maptbl *etc_maptbl;
	u32 nr_etc_maptbl;
#ifdef CONFIG_SUPPORT_AFC
	struct maptbl *afc_maptbl;
	u32 nr_afc_maptbl;
#endif
	struct cal_center center;
	struct cal_line line[MAX_CAL_LINE];
	struct cal_coef coef[MAX_QUAD];
	u32 cal_x_center;
	u32 cal_y_center;
	u32 cal_boundary_center;
	u8 vtx[MAX_WCRD_TYPE][MAX_CCRD_PT][MAX_COLOR];
	u32 num_ldu_mode;
	u32 num_night_level;
	u32 num_color_lens_color;
	u32 num_color_lens_level;
};

struct mdnie_properties {
	u32 enable;
	enum SCENARIO scenario;
	enum SCENARIO_MODE mode;
	enum BYPASS bypass;
	enum HBM hbm;
	enum HMD_MODE hmd;
	enum NIGHT_MODE night;
	enum COLOR_LENS color_lens;
	enum COLOR_LENS_COLOR color_lens_color;
	enum COLOR_LENS_LEVEL color_lens_level;
	enum HDR hdr;
	enum LIGHT_NOTIFICATION light_notification;
	enum ACCESSIBILITY accessibility;
	enum LDU_MODE ldu;
	enum SCR_WHITE_MODE scr_white_mode;
	enum TRANS_MODE trans_mode;
	int night_level;

	/* for color adjustment */
	u8 scr[MAX_MDNIE_SCR_LEN];
	u32 sz_scr;

	int wcrd_x, wcrd_y;

	/* default whiteRGB : color coordinated wrgb */
	u8 coord_wrgb[MAX_WCRD_TYPE][MAX_COLOR];
	/* sensor whiteRGB */
	u8 ssr_wrgb[MAX_COLOR];
	/* current whiteRGB */
	u8 cur_wrgb[MAX_COLOR];
	/* default whiteRGB : color coordinated wrgb */
	u8 def_wrgb[MAX_COLOR];
	s8 def_wrgb_ofs[MAX_COLOR];
#ifdef CONFIG_SUPPORT_AFC
	u8 afc_roi[MAX_AFC_ROI_LEN];
	bool afc_on;
#endif

	u32 num_ldu_mode;
	u32 num_night_level;
	u32 num_color_lens_color;
	u32 num_color_lens_level;
	struct cal_line line[MAX_CAL_LINE];
	struct cal_coef coef[MAX_QUAD];
	u32 cal_x_center;
	u32 cal_y_center;
	u32 cal_boundary_center;
	u8 vtx[MAX_WCRD_TYPE][MAX_CCRD_PT][MAX_COLOR];

	/* support */
	unsigned support_ldu:1;
	unsigned support_trans:1;

	/* dirty flag */
	unsigned tuning:1;				/* tuning file loaded */
	unsigned update_color_adj:1;	/* color adjustement updated */
	unsigned update_scenario:1;		/* scenario updated */
	unsigned update_color_coord:1;	/* color coordinate updated */
	unsigned update_ldu:1;			/* adjust ldu updated */
	unsigned update_sensorRGB:1;	/* sensorRGB scr white updated */

	/* mdnie tuning */
#ifdef CONFIG_EXYNOS_DECON_LCD_TUNING
	char tfilepath[128];			/* tuning file path */
#endif
};

struct mdnie_info {
	struct device *dev;
	struct class *class;
	struct mutex lock;
	struct mdnie_properties props;
	struct notifier_block fb_notif;
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block dpui_notif;
#endif
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct maptbl *scr_white_maptbl;
	u32 nr_scr_white_maptbl;
	struct maptbl *etc_maptbl;
	u32 nr_etc_maptbl;
#ifdef CONFIG_SUPPORT_AFC
	struct maptbl *afc_maptbl;
	u32 nr_afc_maptbl;
#endif
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	u32 nr_reg;
};

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
extern int mdnie_probe(struct panel_device *panel, struct mdnie_tune *mdnie_tune);
extern int mdnie_enable(struct mdnie_info *mdnie);
extern int mdnie_disable(struct mdnie_info *mdnie);
extern int panel_mdnie_update(struct panel_device *panel);
#else
static inline int mdnie_probe(struct panel_device *panel, struct mdnie_tune *mdnie_tune) { return 0; }
static inline int mdnie_enable(struct mdnie_info *mdnie) { return 0; }
static inline int mdnie_disable(struct mdnie_info *mdnie) { return 0; }
static inline int panel_mdnie_update(struct panel_device *panel) { return 0; }
#endif
extern struct maptbl *mdnie_find_maptbl(struct mdnie_info *);
extern struct maptbl *mdnie_find_etc_maptbl(struct mdnie_info *mdnie, int index);
extern int mdnie_get_maptbl_index(struct mdnie_info *mdnie);
extern ssize_t attr_store_for_each(struct class *cls, const char *name, const char *buf, size_t size);
#endif /* __MDNIE_H__ */
