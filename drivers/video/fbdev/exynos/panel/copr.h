/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __COPR_H__
#define __COPR_H__

#include "panel.h"
#include "timenval.h"
#include <linux/ktime.h>
#include <linux/wait.h>

#define CONFIG_SUPPORT_COPR_AVG

#define COPR_V1_SPI_RX_SIZE (8)
#define COPR_V1_DSI_RX_SIZE	(10)
#define COPR_V2_SPI_RX_SIZE (9)
#define COPR_V2_DSI_RX_SIZE	(12)
#define COPR_V3_SPI_RX_SIZE (41)
#define COPR_V3_DSI_RX_SIZE	(41)

enum {
	COPR_SET_SEQ,
	/* if necessary, add new seq */
	COPR_CLR_CNT_ON_SEQ,
	COPR_CLR_CNT_OFF_SEQ,
	COPR_SPI_GET_SEQ,
	COPR_DSI_GET_SEQ,
	MAX_COPR_SEQ,
};

enum {
	COPR_MAPTBL,
	/* if necessary, add new maptbl */
	MAX_COPR_MAPTBL,
};

enum COPR_STATE {
	COPR_UNINITIALIZED,
	COPR_REG_ON,
	COPR_REG_OFF,
	MAX_COPR_REG_STATE,
};

enum COPR_VER {
	COPR_VER_0,
	COPR_VER_1,
	COPR_VER_2,
	COPR_VER_3,
	MAX_COPR_VER,
};

struct copr_options {
	u8 thread_on;			/* copr_thread 0: off, 1: on */
	u8 check_avg;			/* check_avg when lcd on/off  0: not update avg, 1 : updateavg */
};

struct copr_reg_info {
	const char *name;
	u32 offset;
};

struct copr_ergb {
	u16 copr_er;
	u16 copr_eg;
	u16 copr_eb;
};

struct copr_roi {
	u32 roi_xs;
	u32 roi_ys;
	u32 roi_xe;
	u32 roi_ye;
};

struct copr_reg_v0 {
	u32 copr_gamma;		/* 0:GAMMA_1, 1:GAMMA_2.2 */
	u32 copr_en;
	u32 copr_er;
	u32 copr_eg;
	u32 copr_eb;
	u32 roi_on;
	u32 roi_xs;
	u32 roi_ys;
	u32 roi_xe;
	u32 roi_ye;
};

struct copr_reg_v1 {
	u32 cnt_re;
	u32 copr_gamma;		/* 0:GAMMA_1, 1:GAMMA_2.2 */
	u32 copr_en;
	u32 copr_er;
	u32 copr_eg;
	u32 copr_eb;
	u32 max_cnt;
	u32 roi_on;
	u32 roi_xs;
	u32 roi_ys;
	u32 roi_xe;
	u32 roi_ye;
};

struct copr_reg_v2 {
	u32 cnt_re;
	u32 copr_ilc;
	u32 copr_gamma;		/* 0:GAMMA_1, 1:GAMMA_2.2 */
	u32 copr_en;
	u32 copr_er;
	u32 copr_eg;
	u32 copr_eb;
	u32 copr_erc;
	u32 copr_egc;
	u32 copr_ebc;
	u32 max_cnt;
	u32 roi_on;
	u32 roi_xs;
	u32 roi_ys;
	u32 roi_xe;
	u32 roi_ye;
};

struct copr_reg_v3 {
	u32 copr_mask;
	u32 cnt_re;
	u32 copr_ilc;
	u32 copr_gamma;		/* 0:GAMMA_1, 1:GAMMA_2.2 */
	u32 copr_en;
	u32 copr_er;
	u32 copr_eg;
	u32 copr_eb;
	u32 copr_erc;
	u32 copr_egc;
	u32 copr_ebc;
	u32 max_cnt;
	u32 roi_on;
	struct copr_roi roi[6];
};

struct copr_reg {
	union {
		struct copr_reg_v0 v0;
		struct copr_reg_v1 v1;
		struct copr_reg_v2 v2;
		struct copr_reg_v3 v3;
	};
};

struct copr_properties {
	bool support;
	u32 version;
	u32 enable;
	u32 state;
	struct copr_reg reg;
	struct copr_options options;

	u32 copr_ready;
	u32 cur_cnt;
	u32 cur_copr;
	u32 avg_copr;
	u32 s_cur_cnt;
	u32 s_avg_copr;
	u32 comp_copr;
	u32 copr_roi_r[5][3];

	struct copr_roi roi[32];
	int nr_roi;
};

struct copr_wq {
	wait_queue_head_t wait;
	atomic_t count;
	struct task_struct *thread;
};

struct copr_info {
	struct device *dev;
	struct class *class;
	struct mutex lock;
	atomic_t stop;
	struct copr_wq wq;
	struct timenval res;
	struct copr_properties props;
	struct notifier_block fb_notif;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
};

struct panel_copr_data {
	struct copr_reg reg;
	u32 version;
	struct copr_options options;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
	struct copr_roi roi[32];
	int nr_roi;
};

static inline int get_copr_ver(struct copr_info *copr)
{
	return copr->props.version;
}

static inline void SET_COPR_REG_GAMMA(struct copr_info *copr, bool copr_gamma)
{
	if (copr->props.version == COPR_VER_0)
		copr->props.reg.v0.copr_gamma = copr_gamma;
	else if (copr->props.version == COPR_VER_1)
		copr->props.reg.v1.copr_gamma = copr_gamma;
	else if (copr->props.version == COPR_VER_2)
		copr->props.reg.v2.copr_gamma = copr_gamma;
	else if (copr->props.version == COPR_VER_3)
		copr->props.reg.v3.copr_gamma = copr_gamma;
	else
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
}

static inline void SET_COPR_REG_E(struct copr_info *copr, int r, int g, int b)
{
	if (copr->props.version == COPR_VER_0) {
		copr->props.reg.v0.copr_er = r;
		copr->props.reg.v0.copr_eg = g;
		copr->props.reg.v0.copr_eb = b;
	} else if (copr->props.version == COPR_VER_1) {
		copr->props.reg.v1.copr_er = r;
		copr->props.reg.v1.copr_eg = g;
		copr->props.reg.v1.copr_eb = b;
	} else if (copr->props.version == COPR_VER_2) {
		copr->props.reg.v2.copr_er = r;
		copr->props.reg.v2.copr_eg = g;
		copr->props.reg.v2.copr_eb = b;
	} else if (copr->props.version == COPR_VER_3) {
		copr->props.reg.v3.copr_er = r;
		copr->props.reg.v3.copr_eg = g;
		copr->props.reg.v3.copr_eb = b;
	} else {
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
	}
}

static inline void SET_COPR_REG_EC(struct copr_info *copr, int r, int g, int b)
{
	if (copr->props.version == COPR_VER_0) {
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
	} else if (copr->props.version == COPR_VER_1) {
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
	} else if (copr->props.version == COPR_VER_2) {
		copr->props.reg.v2.copr_erc = r;
		copr->props.reg.v2.copr_egc = g;
		copr->props.reg.v2.copr_ebc = b;
	} else if (copr->props.version == COPR_VER_3) {
		copr->props.reg.v3.copr_erc = r;
		copr->props.reg.v3.copr_egc = g;
		copr->props.reg.v3.copr_ebc = b;
	} else {
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
	}
}

static inline void SET_COPR_REG_CNT_RE(struct copr_info *copr, int cnt_re)
{
	if (copr->props.version == COPR_VER_0)
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
	else if (copr->props.version == COPR_VER_1)
		copr->props.reg.v1.cnt_re = cnt_re;
	else if (copr->props.version == COPR_VER_2)
		copr->props.reg.v2.cnt_re = cnt_re;
	else if (copr->props.version == COPR_VER_3)
		copr->props.reg.v3.cnt_re = cnt_re;
	else
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);
}

static inline void SET_COPR_REG_ROI(struct copr_info *copr, struct copr_roi *roi, int nr_roi)
{
	struct copr_properties *props = &copr->props;
	int i;

	if (copr->props.version == COPR_VER_2) {
		if (roi == NULL) {
			props->reg.v2.roi_xs = 0;
			props->reg.v2.roi_ys = 0;
			props->reg.v2.roi_xe = 0;
			props->reg.v2.roi_ye = 0;
			props->reg.v2.roi_on = 0;
		} else {
			props->reg.v2.roi_xs = roi[0].roi_xs;
			props->reg.v2.roi_ys = roi[0].roi_ys;
			props->reg.v2.roi_xe = roi[0].roi_xe;
			props->reg.v2.roi_ye = roi[0].roi_ye;
			props->reg.v2.roi_on = true;
		}
	} else if (copr->props.version == COPR_VER_3) {
		if (roi == NULL) {
			props->reg.v3.roi_on = 0;
			memset(props->reg.v3.roi, 0, sizeof(props->reg.v3.roi));
		} else {
			props->reg.v3.roi_on = 0;
			for (i = 0; i < min_t(int, ARRAY_SIZE(props->reg.v3.roi), nr_roi); i++) {
				props->reg.v3.roi[i].roi_xs = roi[i].roi_xs;
				props->reg.v3.roi[i].roi_ys = roi[i].roi_ys;
				props->reg.v3.roi[i].roi_xe = roi[i].roi_xe;
				props->reg.v3.roi[i].roi_ye = roi[i].roi_ye;
				props->reg.v3.roi_on |= 0x1 << i;
			}
		}
	}
}

static inline int get_copr_reg_copr_en(struct copr_info *copr)
{
	int copr_en = COPR_REG_OFF;

	if (copr->props.version == COPR_VER_0)
		copr_en = copr->props.reg.v0.copr_en;
	else if (copr->props.version == COPR_VER_1)
		copr_en = copr->props.reg.v1.copr_en;
	else if (copr->props.version == COPR_VER_2)
		copr_en = copr->props.reg.v2.copr_en;
	else if (copr->props.version == COPR_VER_3)
		copr_en = copr->props.reg.v3.copr_en;
	else
		pr_warn("%s unsupprted in ver%d\n",
				__func__, copr->props.version);

	return copr_en;
}

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
bool copr_is_enabled(struct copr_info *copr);
int copr_enable(struct copr_info *copr);
int copr_disable(struct copr_info *copr);
int copr_update_start(struct copr_info *copr, int count);
int copr_update_average(struct copr_info *copr);
int copr_get_value(struct copr_info *copr);
//int copr_get_average(struct copr_info *, int *, int *);
int copr_get_average_and_clear(struct copr_info *copr);
int copr_roi_set_value(struct copr_info *copr, struct copr_roi *roi, int size);
int copr_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out);
int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data);
int copr_res_update(struct copr_info *copr, int index, int cur_value, struct timespec cur_ts);
int get_copr_reg_size(int version);
const char *get_copr_reg_name(int version, int index);
int get_copr_reg_offset(int version, int index);
u32 *get_copr_reg_ptr(struct copr_reg *reg, int version, int index);
int find_copr_reg_by_name(int version, char *s);
ssize_t copr_reg_show(struct copr_info *copr, char *buf);
int copr_reg_store(struct copr_info *copr, int index, u32 value);
#else
static inline bool copr_is_enabled(struct copr_info *copr) { return 0; }
static inline int copr_enable(struct copr_info *copr) { return 0; }
static inline int copr_disable(struct copr_info *copr) { return 0; }
static inline int copr_update_start(struct copr_info *copr, int count) { return 0; }
static inline int copr_update_average(struct copr_info *copr) { return 0; }
static inline int copr_get_value(struct copr_info *copr) { return 0; }
static inline int copr_get_average_and_clear(struct copr_info *copr) { return 0; }
static inline int copr_roi_set_value(struct copr_info *copr, struct copr_roi *roi, int size) { return 0; }
static inline int copr_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out) { return 0; }
static inline int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data) { return 0; }
static inline int copr_res_update(struct copr_info *copr, int index, int cur_value, struct timespec cur_ts) { return 0; }
static inline int get_copr_reg_size(int version) { return 0; }
static inline const char *get_copr_reg_name(int version, int index) { return NULL; }
static inline int get_copr_reg_offset(int version, int index) { return 0; }
static inline u32 *get_copr_reg_ptr(struct copr_reg *reg, int version, int index) { return NULL; }
static inline int find_copr_reg_by_name(int version, char *s) { return -EINVAL; }
static inline ssize_t copr_reg_show(struct copr_info *copr, char *buf) { return 0; }
static inline int copr_reg_store(struct copr_info *copr, int index, u32 value) { return 0; }
#endif
#endif /* __COPR_H__ */
