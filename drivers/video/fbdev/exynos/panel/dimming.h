/*
 * linux/drivers/video/fbdev/exynos/panel/dimming.h
 *
 * Header file for Samsung AID Dimming Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DIMMING_H_
#define _DIMMING_H_

//#define DEBUG_DIMMING
#define DEBUG_EXCUTION_TIME
#define DEBUG_DIMMING_RESULT
//#define DEBUG_DIMMING_RESULT_ASCENDING
#define MAX_PRINT_BUF_SIZE	(512)

#ifndef __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define EINVAL      (22)

#define DIM_ERR     "[err] "
#define DIM_WARN    "[warn] "
/* #define DIM_INFO    "[info] " */
#define DIM_INFO    ""
#define DIM_DEBUG   "[dbg] "

#define pr_err(fmt, ...)	printf(DIM_ERR fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)	printf(DIM_WARN fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)	printf(DIM_INFO fmt, ##__VA_ARGS__)
#ifdef DEBUG_DIMMING
#define pr_debug(fmt, ...)	printf(DIM_DEBUG fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...) do {} while(0)
#endif
#define unlikely
#define __func__	__FUNCTION__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
typedef int bool;
enum {
	false   = 0,
	true    = 1,
};

static u64 do_div64(u64 *num, u32 den)
{
	u64 rem = *num % den;
	*num /= den;
	return rem;
}

#define do_div(num, den)	(do_div64(&(num), den))
#define GFP_KERNEL	(0)
#define typeof		typeid
#define kfree		free
#define kmalloc(size, flags)	malloc((size))
#define kzalloc(size, flags)	kmalloc((size), (flags))
#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) > (b) ? (a) : (b))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#else
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#endif	/* _LINUX_ */

#define DIMMING_CALC_PRECISE
#define BIT_SHIFT	(24)
#define DIMMING_BITSHIFT	(BIT_SHIFT)
#define GRAY_SCALE_MAX	(256)
#define ERROR_DETECT_BOUND	(1)

#if (BIT_SHIFT != 22) && (BIT_SHIFT != 24) && (BIT_SHIFT != 26)
#ifndef __KERNEL__
#define GENERATE_GAMMA_CURVE
#include <math.h>
#else
#error "Unable to generate gamma curve in KERNEL"
#endif /* __KERNEL */
#endif /* BIT_SHIFT CONDITION */

enum tag {
	TAG_DIM_GLOBAL_INFO_START,
	TAG_DIM_GLOBAL_INFO_END,
	TAG_TP_LUT_INFO_START,
	TAG_TP_LUT_INFO_END,
	TAG_TP_LUT_START,
	TAG_TP_LUT_END,
	TAG_DIM_LUT_INFO_START,
	TAG_DIM_LUT_INFO_END,
	TAG_DIM_LUT_START,
	TAG_DIM_LUT_END,
	TAG_MTP_OFFSET_START,
	TAG_MTP_OFFSET_END,
	TAG_TP_VOLT_START,
	TAG_TP_VOLT_END,
	TAG_GRAY_SCALE_VOLT_START,
	TAG_GRAY_SCALE_VOLT_END,
	TAG_GAMMA_CENTER_START,
	TAG_GAMMA_CENTER_END,
	TAG_MTP_OFFSET_TEST_RANGE_START,
	TAG_MTP_OFFSET_TEST_RANGE_END,
	MAX_INPUT_TAG,
};

enum {
	GLOBAL_DIM_INFO_NR_TP,
	GLOBAL_DIM_INFO_NR_LUMINANCE,
	GLOBAL_DIM_INFO_VREGOUT,
	GLOBAL_DIM_INFO_VREF,
	GLOBAL_DIM_INFO_GAMMA,
	GLOBAL_DIM_INFO_VT_VOLTAGE,
};

enum {
	TP_LUT_NONE,
	TP_LUT_LEVEL,
	TP_LUT_NAME,
	TP_LUT_VOLT_SRC,
	TP_LUT_GAMMA_CENTER,
	TP_LUT_GAMMA_CALC,
	MAX_TP_LUT_FIELD,
};

enum {
	DIM_LUT_NONE,
	DIM_LUT_LUMINANCE,
	DIM_LUT_AOR,
	DIM_LUT_BASE_LUMINANCE,
	DIM_LUT_GAMMA,
	DIM_LUT_GRAY_SCALE_OFFSET,
	DIM_LUT_COLOR_SHIFT_OFFSET,
	MAX_DIM_LUT_FIELD,
};

enum {
	R_OFFSET,
	G_OFFSET,
	B_OFFSET,
	RGB_MAX_OFFSET,
};

enum color {
	RED,
	GREEN,
	BLUE,
	MAX_COLOR,
};

enum gamma_degree {
	GAMMA_NONE = 0,
	GAMMA_MIN,
	GAMMA_1_60 = GAMMA_MIN,
	GAMMA_1_65,
	GAMMA_1_70,
	GAMMA_1_75,
	GAMMA_1_80,
	GAMMA_1_85,
	GAMMA_1_90,
	GAMMA_1_95,
	GAMMA_2_00,
	GAMMA_2_05,
	GAMMA_2_10,
	GAMMA_2_12,
	GAMMA_2_13,
	GAMMA_2_15,
	GAMMA_2_20,
	GAMMA_2_25,
	GAMMA_MAX,
};

enum {
	VREG_OUT,
	V0_OUT,
	VT_OUT,
	MAX_VOLT_SRC,
};


extern const char *tag_name[];
extern const char *global_dim_info_name[];
extern const char *dim_lut_field_name[];
extern const char *color_name[];
extern const char *volt_src_name[];

struct gamma_curve {
	int gamma;
	int *gamma_curve_table;
};

typedef s32(*s32_TP_COLORs)[MAX_COLOR];
typedef s64(*s64_TP_COLORs)[MAX_COLOR];
typedef u32(*u32_TP_COLORs)[MAX_COLOR];
typedef u64(*u64_TP_COLORs)[MAX_COLOR];

typedef s32(*s32_TPs);
typedef s64(*s64_TPs);
typedef u32(*u32_TPs);
typedef u64(*u64_TPs);

struct _range {
	int from, to, step;
};

#ifdef DEBUG_EXCUTION_TIME
#ifdef __KERNEL__
typedef long long mytime_t;
#define gettime(t)	((t) = jiffies)
#define difftime(t1, t2)	((jiffies_to_msecs((t2) - (t1))) * 1000)
#else
#ifdef _WIN32
typedef long long mytime_t;
#define gettime(t)	((t) = clock())
#define difftime(t1, t2)	(((t2) - (t1)) * 1000000 / CLOCKS_PER_SEC)
#else
typedef struct timespec mytime_t;
#define gettime(t)	(clock_gettime(CLOCK_MONOTONIC, &(t)))
#define BILLION 1000000000L
#define difftime(t1, t2)	((BILLION * ((t2).tv_sec - (t1).tv_sec) + (t2).tv_nsec - (t1).tv_nsec) / 1000)
#endif
#endif	/* __KERNEL__ */
#endif	/* DEBUG_EXCUTION_TIME */

#define in_range(i, r)					\
	((r).from <= (r).to) ?				\
	((r).from <= (i) && (i) < (r).to) :	\
	((r).from >= (i) && (i) > (r).to)

#define for_each_range(i, r)	\
for ((i) = (r).from;		\
	(i) != (r).to && in_range(i, r);			\
	(i) += (r).step)

#define for_each_tp(__pos__)	\
	for ((__pos__) = 0;			\
		(__pos__) < (NR_TP);	\
		(__pos__)++)

#define for_each_color(__pos__) \
	for ((__pos__) = 0;			\
		(__pos__) < (MAX_COLOR);\
		(__pos__)++)

#define for_each_tp_color(v, c)	\
	for_each_tp(v)				\
	for_each_color(c)

#define for_each_tp_order(__pos__, idx, order)		\
	for ((idx) = 0, (__pos__) = (order)[(idx)];		\
		(idx) < (NR_TP);						\
		(__pos__) = (order)[++(idx)])

/* DIM_LUT_V0 : INSERT BASE_LUMINANCE DIRECTLY */
#define DIM_LUT_V0_INIT(l, bl, gma, rtbl, ctbl)		\
{													\
	.luminance = (l),								\
	.base_luminance = (bl),							\
	.base_gray = (0),								\
	.g_curve_degree = (gma),						\
	.gray_scale_offset = (rtbl),					\
	.rgb_color_offset = (s32 (*)[MAX_COLOR])(ctbl),	\
}

/* DIM_LUT_V1 : INSERT BASE_GRAY and Calculate BASE_LUMINANCE */
#define DIM_LUT_V1_INIT(l, bg, gma, rtbl, ctbl)		\
{													\
	.luminance = (l),								\
	.base_luminance = (0),							\
	.base_gray = (bg),								\
	.g_curve_degree = (gma),						\
	.gray_scale_offset = (rtbl),					\
	.rgb_color_offset = (s32 (*)[MAX_COLOR])(ctbl),	\
}

#define DIM_LUT_INIT(l, bl, gma, rtbl, ctbl)		\
	DIM_LUT_V0_INIT(l, bl, gma, rtbl, ctbl)

struct dimming_lut_info {
	int nrow;
	int ncol;
	int fields[20];
	struct _range gray_scale_offset_range;
	struct _range rgb_color_offset_range;
};

struct dimming_tp_output {
	u64 L;
	u32 M_GRAY;
	s64 vout[MAX_COLOR];
	s32 center[MAX_COLOR];
	s32 center_debug[MAX_COLOR];
};

struct dimming_lut {
	/* input data from user setting */
	u32 luminance;
	u32 aor;
	u32 base_luminance;
	u32 base_gray;
	enum gamma_degree g_curve_degree;
	s32 *gray_scale_offset;
	s32(*rgb_color_offset)[MAX_COLOR];

	/* output(reversed distortion) data will be stored */
	struct dimming_tp_output *tpout;
	struct dimming_tp_output *tpout_debug;
};

struct gray_scale {
	u64 gamma_value;
	s64 vout[MAX_COLOR];
};

struct tp_lut_info {
	int nrow;
	int ncol;
	int fields[20];
};

struct tp {
	int level;				/* gray_level of each tp */
	int volt_src;			/* voltage source of each tp*/
	char name[10];			/* name of each tp */
	s32 center[MAX_COLOR];	/* center gamma's fixed value of tp */
	s32 offset[MAX_COLOR];	/* mtp offset value of tp */
	s64 vout[MAX_COLOR];	/* voltage output of tp */
	u32 denominator;		/* denominator of tp */
	u32 numerator;			/* numerator of tp */
	u32 bits;				/* available bit of tp */
	struct _range range;		/* tp input range for Test Case Generation */
};

struct dimming_info {
	s64 vregout;			/* vregout * (1 << bitshift) */
	s64 vref;				/* vref * (1 << bitshift) */
	u32 vt_voltage[16];		/* vt voltage table */
	u32 v0_voltage[16];		/* v0 voltage table */
	struct tp_lut_info tp_lut_info;
	int nr_tp;				/* number of turning point */
	struct tp *tp;			/* informations of panel's tuning point */

	int nr_luminance;		/* number of luminance */
	struct gray_scale gray_scale_lut[GRAY_SCALE_MAX];
	struct dimming_lut_info dim_lut_info;
	struct dimming_lut *dim_lut;
	int target_luminance;
	enum gamma_degree target_g_curve_degree;

	s32 hbm_luminance;
	s32 (*hbm_gamma_tbl)[MAX_COLOR];
};

struct dimming_init_info {
	const char *name;
	int nr_tp;				/* number of turning point */
	struct tp *tp;			/* informations of panel's tuning point */
	int nr_luminance;		/* number of luminance */
	s64 vregout;			/* vregout * (1 << bitshift) */
	s64 vref;				/* vref * (1 << bitshift) */
	int bitshift;			/* bitshift for precise calculation */
	u32 vt_voltage[16];		/* vt voltage table */
	u32 v0_voltage[16];		/* v0 voltage table */
	int target_luminance;	/* target luminance */
	int target_gamma;		/* target gamma value */
	struct dimming_lut *dim_lut;

	s32 hbm_luminance;
};

#ifdef CONFIG_PANEL_AID_DIMMING
int init_dimming_info(struct dimming_info *, struct dimming_init_info *);
int init_dimming_mtp(struct dimming_info *, s32 (*)[MAX_COLOR]);
int init_dimming_hbm_info(struct dimming_info *, s32 (*)[MAX_COLOR], u32);
s64 interpolation(s64 from, s64 to, int cur_step, int total_step);
s64 disp_round(s64 num, u32 digits);
s64 disp_div64(s64 num, s64 den);
s64 disp_pow(s64 num, u32 digits);
#ifdef DIMMING_CALC_PRECISE
s64 disp_div64_round(s64 num, s64 den, u32 digits);
#endif
static inline s64 disp_pow_round(s64 num, u32 digits)
{
	if (digits < 1)
		return 0;

	return disp_div64(num + 5 * disp_pow(10, digits - 1),
			disp_pow(10, digits)) * disp_pow(10, digits);
}
int process_dimming(struct dimming_info *);
int gamma_table_add_offset(s32 (*src)[MAX_COLOR], s32 (*ofs)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], struct tp *tp, int nr_tp);
int gamma_table_interpolation(s32 (*from)[MAX_COLOR], s32 (*to)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], int nr_tp, int cur_step, int total_step);
void get_dimming_gamma(struct dimming_info *dim_info, u32 luminance, u8 *output,
		void (*copy)(u8 *output, u32 value, u32 index, u32 color));
int gamma_table_interpolation(s32 (*from)[MAX_COLOR], s32 (*to)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], int nr_tp, int cur_step, int total_step);
/* for debug */
void print_dimming_info(struct dimming_info *dim_info, int tag);
#else
static inline int init_dimming_info(struct dimming_info *dim_info, struct dimming_init_info *src) { return 0; }
static inline int init_dimming_mtp(struct dimming_info *dim_info, s32 (*mtp)[MAX_COLOR]) { return 0; }
static inline int init_dimming_hbm_info(struct dimming_info *dim_info, s32 (*hbm_gamma_tbl)[MAX_COLOR], u32 hbm_luminance) { return 0; }
static inline s64 interpolation(s64 from, s64 to, int cur_step, int total_step) { return 0; }
static inline s64 disp_round(s64 num, u32 digits) { return 0; }
static inline s64 disp_div64(s64 num, s64 den) { return 0; }
#ifdef DIMMING_CALC_PRECISE
static inline s64 disp_div64_round(s64 num, s64 den, u32 digits) { return 0; }
#endif
static inline int process_dimming(struct dimming_info *dim_info) { return 0; }
static inline int gamma_table_interpolation(s32 (*from)[MAX_COLOR], s32 (*to)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], int nr_tp, int cur_step, int total_step) { return 0; }
void get_dimming_gamma(struct dimming_info *dim_info, u32 luminance, u8 *output,
		void (*copy)(u8 *output, u32 value, u32 index, u32 color)) {}
static inline int gamma_table_interpolation(s32 (*from)[MAX_COLOR], s32 (*to)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], int nr_tp, int cur_step, int total_step) { return 0; }
/* for debug */
static inline void print_dimming_info(struct dimming_info *dim_info, int tag) {}
#endif	/* CONFIG_PANEL_AID_DIMMING */
#endif	/* _DIMMING_H_ */
