/*
 * linux/drivers/video/fbdev/exynos/panel/dimming.c
 *
 * Samsung AID Dimming Driver.
 *
 * Copyright (c) 2016 Samsung Electronics
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "dimming.h"
#include "dimming_gamma.h"

static int NR_LUMINANCE;
static int NR_TP;
static int TP_VT = 0;
static int TP_V0 = -1;
static int TP_V255 = -1;
static int TARGET_LUMINANCE;
static enum gamma_degree TARGET_G_CURVE_DEGREE;

static int *calc_gray_order;
static int *calc_vout_order;

const char *tag_name[] = {
	"[DIM_GLOBAL_INFO_START]",
	"[DIM_GLOBAL_INFO_END]",
	"[TP_LUT_INFO_START]",
	"[TP_LUT_INFO_END]",
	"[TP_LUT_START]",
	"[TP_LUT_END]",
	"[DIM_LUT_INFO_START]",
	"[DIM_LUT_INFO_END]",
	"[DIM_LUT_START]",
	"[DIM_LUT_END]",
	"[MTP_OFFSET_START]",
	"[MTP_OFFSET_END]",
	"[TP_VOLT_START]",
	"[TP_VOLT_END]",
	"[GRAY_SCALE_VOLT_START]",
	"[GRAY_SCALE_VOLT_END]",
	"[GAMMA_CENTER_START]",
	"[GAMMA_CENTER_END]",
	"[MTP_OFFSET_TEST_RANGE_START]",
	"[MTP_OFFSET_TEST_RANGE_END]",
};

const char *global_dim_info_name[] = {
	"NR_TP",
	"NR_LUMINANCE",
	"VREGOUT",
	"VREF",
	"GAMMA",
	"VT_VOLTAGE",
};

const char *tp_lut_field_name[] = {
	"TP_NONE",
	"TP_LEVEL",
	"TP_NAME",
	"TP_VOLT_SRC",
	"TP_GAMMA_CENTER",
	"TP_GAMMA_CALC",
};

const char *dim_lut_field_name[] = {
	"DIM_NONE" ,
	"DIM_LUMINANCE",
	"DIM_AOR",
	"DIM_BASE_LUMINANCE",
	"DIM_GAMMA",
	"DIM_GRAY_SCALE_OFFSET",
	"DIM_COLOR_SHIFT_OFFSET",
};

const char *color_name[] = {
	"RED", "GREEN", "BLUE",
};

const char *volt_src_name[] = {
	"VREG_OUT", "V0_OUT", "VT_OUT",
};

struct gamma_curve gamma_curve_lut[] = {
#ifndef GENERATE_GAMMA_CURVE
	{ 0, NULL },
	{ 160, gamma_curve_1p60 },
	{ 165, gamma_curve_1p65 },
	{ 170, gamma_curve_1p70 },
	{ 175, gamma_curve_1p75 },
	{ 180, gamma_curve_1p80 },
	{ 185, gamma_curve_1p85 },
	{ 190, gamma_curve_1p90 },
	{ 195, gamma_curve_1p95 },
	{ 200, gamma_curve_2p00 },
	{ 205, gamma_curve_2p05 },
	{ 210, gamma_curve_2p10 },
	{ 212, gamma_curve_2p12 },
	{ 213, gamma_curve_2p13 },
	{ 215, gamma_curve_2p15 },
	{ 220, gamma_curve_2p20 },
	{ 225, gamma_curve_2p25 },
#else
	{ 0, NULL },
	{ 160, NULL },
	{ 165, NULL },
	{ 170, NULL },
	{ 175, NULL },
	{ 180, NULL },
	{ 185, NULL },
	{ 190, NULL },
	{ 195, NULL },
	{ 200, NULL },
	{ 205, NULL },
	{ 210, NULL },
	{ 212, NULL },
	{ 213, NULL },
	{ 215, NULL },
	{ 220, NULL },
	{ 225, NULL },
#endif	/* GENERATE_GAMMA_CURVE */
};

s64 disp_pow(s64 num, u32 digits)
{
	s64 res = num;
	u32 i;

	if (digits == 0)
		return 1;

	for (i = 1; i < digits; i++)
		res *= num;
	return res;
}

s64 disp_round(s64 num, u32 digits)
{
	int sign = (num < 0 ? -1 : 1);
	u64 tnum;

	tnum = num * sign;
	tnum *= disp_pow(10, digits + 1);
	tnum += 5;
	do_div(tnum, (u32)disp_pow(10, digits + 1));

	return sign * (s64)tnum;
}

static s64 scale_down_round(s64 num, u32 digits)
{
	int sign = (num < 0 ? -1 : 1);
	u64 tnum, rem;

	tnum = num * sign;
	rem = do_div(tnum, (1U << BIT_SHIFT));

	rem *= disp_pow(10, digits + 1);
	rem >>= BIT_SHIFT;
	rem += 5;
	do_div(rem, (u32)disp_pow(10, digits + 1));

	return sign * (s64)(tnum + rem);
}

static s64 scale_down_rem(s64 num, u32 digits)
{
	int sign = (num < 0 ? -1 : 1);
	u64 tnum, rem;

	tnum = num * sign;
	rem = do_div(tnum, (1U << BIT_SHIFT));

	rem *= disp_pow(10, digits + 1);
	rem >>= BIT_SHIFT;
	rem += 5;
	do_div(rem, 10);

	/* round up and minus in remainder */
	if (rem >= (u64)disp_pow(10, digits))
		rem -= (u64)disp_pow(10, digits);

	return sign * rem;
}

#if BIT_SHIFT > 26
static int msb64(s64 num)
{
	int i;
	for (i = 63; i >= 0; i--)
		if (num & (1ULL << i))
			return i;

	return 0;
}
#endif

s64 disp_div64(s64 num, s64 den)
{
#if BIT_SHIFT > 26
	if (num > ((1LL << (63 - BIT_SHIFT)) - 1)) {
		int bit_shift = 62 - msb64(num);

		pr_err("out of range num %lld\n", num);
		return ((num << bit_shift) / den) >> bit_shift;
	}
#endif

	int sign_num = (num < 0 ? -1 : 1);
	int sign_den = (den < 0 ? -1 : 1);
	int sign = sign_num * sign_den;
	u64 tnum, tden;

	if (unlikely(den == 0)) {
		pr_err("%s, den should not be zero (%llu %llu)\n", __func__, num, den);
		return 0;
	}

	tnum = num * sign_num;
	tden = den * sign_den;

	if (tden >= (1ULL << 31))
		pr_err("%s, out of range denominator %llu\n",
				__func__, tden);

	do_div(tnum, (u32)tden);

	return sign * tnum;
}

#ifdef DIMMING_CALC_PRECISE
s64 disp_div64_round(s64 num, s64 den, u32 digits)
{
	int sign_num = (num < 0 ? -1 : 1);
	int sign_den = (den < 0 ? -1 : 1);
	int sign = sign_num * sign_den;
	u64 tnum, tden, rem;

	if (unlikely(den == 0)) {
		pr_err("%s, den should not be zero (%llu %llu %u)\n",
				__func__, num, den, digits);
		return 0;
	}

	tnum = num * sign_num;
	tden = den * sign_den;

	rem = do_div(tnum, (u32)tden);
	rem *= disp_pow(10, digits + 2);
	do_div(rem, (u32)tden);

	/*
	 * floating point should not be used.
	 * Some variables are inaccurate in most case (e.g. vregout...)
	 * To compensate it, round_up function use (+ 6) instead of (+ 5).
	 */
	rem += 5;
	do_div(rem, (u32)disp_pow(10, digits + 2));

	return sign * (tnum + rem);
}
#endif	/* DIMMING_CALC_PRECISE */

/*
 * generate_order() is an helper function can make an order
 * that mixed an initial order value and range based order values.
 * @s : initial value
 * @from - starting of the range
 * @to - end of the range including 'to'.
 * @return - an allocated array will be set as like { s, [from, to] }.
 */
static int *generate_order(int s, int from, int to) {
	int i, len = abs(from - to) + 2;
	int sign = ((to - from) < 0) ? -1 : 1;
	int *t_order = (int *)kmalloc(len * sizeof(int), GFP_KERNEL);

	if (!t_order) {
		pr_err("%s, failed to allocate memory\n", __func__);
		return NULL;
	}

	t_order[0] = s;
	t_order[1] = from;
	for (i = 2; i < len; i++)
		t_order[i] = t_order[i - 1] + sign;

	return t_order;
}

static int find_name_in_table(const char **table, int sz_table, char *s)
{
	int i;

	for (i = 0; i < sz_table; i++)
		if (!strncmp(s, table[i], 128))
			return i;
	return -EINVAL;
}

int find_tag_name(char *s)
{
	return find_name_in_table(tag_name, ARRAY_SIZE(tag_name), s);
}

int find_global_dim_info(char *s)
{
	return find_name_in_table(global_dim_info_name,
			ARRAY_SIZE(global_dim_info_name), s);
}

int find_voltage_source(char *s)
{
	return find_name_in_table(volt_src_name, ARRAY_SIZE(volt_src_name), s);
}

int find_tp_lut_field(char *s)
{
	return find_name_in_table(tp_lut_field_name, ARRAY_SIZE(tp_lut_field_name), s);
}

int find_dim_lut_field(char *s)
{
	return find_name_in_table(dim_lut_field_name, ARRAY_SIZE(dim_lut_field_name), s);
}

#ifdef GENERATE_GAMMA_CURVE
static void generate_gamma_curve(int gamma, int *gamma_curve)
{
	int i;
	double gamma_f = (double)gamma / 100;

	pr_info("%s, generate %d gamma\n", __func__, gamma);
	for (i = 0; i < GRAY_SCALE_MAX; i++) {
		gamma_curve[i] = (int)(pow(((double)i / 255), gamma_f) * (double)(1 << BIT_SHIFT) + 0.5f);
		pr_info("%d ", gamma_curve[i]);
		if (!((i + 1) % 8))
			pr_info("\n");
	}
}

void remove_gamma_curve(void)
{
	size_t index;

	for (index = 0; index < ARRAY_SIZE(gamma_curve_lut); index++)
		if (gamma_curve_lut[index].gamma_curve_table) {
			free(gamma_curve_lut[index].gamma_curve_table);
			gamma_curve_lut[index].gamma_curve_table = NULL;
		}
}
#endif	/* GENERATE_GAMMA_CURVE */

int find_gamma_curve(int gamma)
{
	size_t index;
#ifdef GENERATE_GAMMA_CURVE
	int *gamma_curve_table;
#endif

	for (index = 0; index < ARRAY_SIZE(gamma_curve_lut); index++)
		if (gamma_curve_lut[index].gamma == gamma)
			break;

	if (index == ARRAY_SIZE(gamma_curve_lut)) {
		pr_err("%s, gamma %d curve not found\n", __func__, gamma);
		return -1;
	}

#ifdef GENERATE_GAMMA_CURVE
	if (!gamma_curve_lut[index].gamma_curve_table) {
		pr_info("%s, generate gamma curve %d\n", __func__, gamma);
		gamma_curve_table = kmalloc(GRAY_SCALE_MAX * sizeof(int), GFP_KERNEL);
		generate_gamma_curve(gamma, gamma_curve_table);
		gamma_curve_lut[index].gamma_curve_table = gamma_curve_table;
	}
#endif

	return (int)index;
}

enum gamma_degree gamma_value_to_degree(int gamma)
{
	int g_curve_index;
	g_curve_index = find_gamma_curve(gamma);
	if (g_curve_index < 0) {
		pr_err("%s, gamma %d not exist\n", __func__, gamma);
		return GAMMA_NONE;
	}

	return (enum gamma_degree)g_curve_index;
}

/*
 * @ g_curve : gamma curve
 * @ idx : gray scale level 0 ~ 255.
 * @ return ((idx/255)^g_curve)*(2^22)
 */
int gamma_curve_value(enum gamma_degree g_curve, int idx)
{
	if (g_curve == GAMMA_NONE ||
			idx >= GRAY_SCALE_MAX)
		return -1;

#ifdef GENERATE_GAMMA_CURVE
	if (!gamma_curve_lut[(int)g_curve].gamma_curve_table) {
		int *gamma_curve_table = (int *)kmalloc(GRAY_SCALE_MAX * sizeof(int), GFP_KERNEL);
		int gamma = gamma_curve_lut[(int)g_curve].gamma;

		pr_info("%s, generate gamma curve %d\n", __func__, gamma);
		generate_gamma_curve(gamma, gamma_curve_table);
		gamma_curve_lut[(int)g_curve].gamma_curve_table = gamma_curve_table;
	}
#endif

	return gamma_curve_lut[(int)g_curve].gamma_curve_table[idx];
}

bool gamma_in_range(int gamma)
{
	return !(gamma < gamma_curve_lut[GAMMA_MIN].gamma ||
			gamma > gamma_curve_lut[GAMMA_MAX - 1].gamma);
}

void print_mtp_offset(struct dimming_info *dim_info)
{
	int i;

	for (i = 0; i < dim_info->nr_tp; i++)
		pr_info("%-7s %-4d %-4d %-4d\n",
				dim_info->tp[i].name,
				dim_info->tp[i].offset[RED],
				dim_info->tp[i].offset[GREEN],
				dim_info->tp[i].offset[BLUE]);
}

void print_hbm_gamma_tbl(struct dimming_info *dim_info)
{
	int i;

	if (unlikely(!dim_info->hbm_gamma_tbl)) {
		pr_warn("%s, hbm_gamma_tbl not exist\n", __func__);
		return;
	}

	if (unlikely(!dim_info->tp)) {
		pr_warn("%s, tp not exist\n", __func__);
		return;
	}

	for (i = 0; i < dim_info->nr_tp; i++)
		pr_info("%-7s %-4d %-4d %-4d\n",
				dim_info->tp[i].name,
				dim_info->hbm_gamma_tbl[i][RED],
				dim_info->hbm_gamma_tbl[i][GREEN],
				dim_info->hbm_gamma_tbl[i][BLUE]);
}

void print_tpout_center(struct dimming_info *dim_info)
{
	int i, len, ilum, nr_luminance, nr_tp;
	char buf[MAX_PRINT_BUF_SIZE];
	struct dimming_lut *lut;

	if (unlikely(!dim_info)) {
		pr_err("%s, invalid dim_info\n", __func__);
		return;
	}

	lut = dim_info->dim_lut;
	nr_tp = dim_info->nr_tp;
	nr_luminance = dim_info->nr_luminance;

	if (unlikely(!lut)) {
		pr_err("%s, invalid dim_lut\n", __func__);
		return;
	}

	if ((nr_tp * 15UL) >= ARRAY_SIZE(buf)) {
		pr_err("%s, exceed linebuf size (%lu)\n",
				__func__, nr_tp * 15UL);
		return;
	}

	for (ilum = 0; ilum < nr_luminance; ilum++) {
		len = snprintf(buf, MAX_PRINT_BUF_SIZE,
					"%-3d   ", lut[ilum].luminance);
#ifdef DEBUG_DIMMING_RESULT_ASCENDING
		for (i = 1; i < nr_tp; i++) {
			len += snprintf(buf + len, max(MAX_PRINT_BUF_SIZE - len, 0),
					"%-3d %-3d %-3d ",
					lut[ilum].tpout[i].center[RED],
					lut[ilum].tpout[i].center[GREEN],
					lut[ilum].tpout[i].center[BLUE]);
		}
		len += snprintf(buf + len, max(MAX_PRINT_BUF_SIZE - len, 0),
				"%-3d %-3d %-3d ",
				lut[ilum].tpout[TP_VT].center[RED],
				lut[ilum].tpout[TP_VT].center[GREEN],
				lut[ilum].tpout[TP_VT].center[BLUE]);
#else
		for (i = nr_tp - 1; i >= 0; i--) {
			len += snprintf(buf + len, max(MAX_PRINT_BUF_SIZE - len, 0),
					"%-3d %-3d %-3d ",
					lut[ilum].tpout[i].center[RED],
					lut[ilum].tpout[i].center[GREEN],
					lut[ilum].tpout[i].center[BLUE]);
		}
#endif
		pr_info("%s\n", buf);
	}

}

void print_tpout_center_debug(struct dimming_info *dim_info)
{
	int i, ilum, len, nr_tp, nr_luminance;
	char buf[MAX_PRINT_BUF_SIZE];
	struct dimming_lut *lut;

	if (unlikely(!dim_info)) {
		pr_err("%s, invalid dim_info\n", __func__);
		return;
	}

	lut = dim_info->dim_lut;
	nr_tp = dim_info->nr_tp;
	nr_luminance = dim_info->nr_luminance;

	if (unlikely(!lut)) {
		pr_err("%s, invalid dim_lut\n", __func__);
		return;
	}

	if ((nr_tp * 15UL) >= ARRAY_SIZE(buf)) {
		pr_err("%s, exceed linebuf size (%lu)\n",
				__func__, nr_tp * 15UL);
		return;
	}

	for (ilum = 0; ilum < nr_luminance; ilum++) {
		len = snprintf(buf, MAX_PRINT_BUF_SIZE,
				"%-3d   ", lut[ilum].luminance);
		for (i = nr_tp - 1; i >= 0; i--) {
			len += snprintf(buf + len,
					max(MAX_PRINT_BUF_SIZE - len, 0),
					"%-3d %-3d %-3d ",
					lut[ilum].tpout[i].center_debug[RED],
					lut[ilum].tpout[i].center_debug[GREEN],
					lut[ilum].tpout[i].center_debug[BLUE]);
		}
		pr_info("%s\n", buf);
	}
}

void print_tp_vout(struct dimming_info *dim_info)
{
	int i, nr_tp;
	struct tp *tp;

	if (unlikely(!dim_info)) {
		pr_err("%s, invalid dim_info\n", __func__);
		return;
	}

	tp = dim_info->tp;
	nr_tp = dim_info->nr_tp;
	if (unlikely(!tp)) {
		pr_err("%s, invalid tp\n", __func__);
		return;
	}

	for (i = 0; i < nr_tp; i++)
		pr_info("%1lld.%04lld  %1lld.%04lld  %1lld.%04lld\n",
				scale_down_round(tp[i].vout[RED], 4), scale_down_rem(tp[i].vout[RED], 4),
				scale_down_round(tp[i].vout[GREEN], 4), scale_down_rem(tp[i].vout[GREEN], 4),
				scale_down_round(tp[i].vout[BLUE], 4), scale_down_rem(tp[i].vout[BLUE], 4));
}

void print_gray_scale_vout(struct gray_scale *gray_scale_lut)
{
	int i;

	for (i = 0; i < GRAY_SCALE_MAX; i++) {
		pr_info("%1lld.%04lld  %1lld.%04lld  %1lld.%04lld\n",
				scale_down_round(gray_scale_lut[i].vout[RED], 4), scale_down_rem(gray_scale_lut[i].vout[RED], 4),
				scale_down_round(gray_scale_lut[i].vout[GREEN], 4), scale_down_rem(gray_scale_lut[i].vout[GREEN], 4),
				scale_down_round(gray_scale_lut[i].vout[BLUE], 4), scale_down_rem(gray_scale_lut[i].vout[BLUE], 4));
	}
}

static u64 mtp_offset_to_vregout(struct dimming_info *dim_info,
		int v, enum color c)
{
	s64 num, den, vreg, vsrc, res = 0;
	struct tp *tp = dim_info->tp;
	u32 *vt_voltage = dim_info->vt_voltage;
	u32 *v0_voltage = dim_info->v0_voltage;
	s64 VREGOUT = dim_info->vregout;
	s64 VREF = dim_info->vref;

	if (!tp || v < 0 || v >= NR_TP) {
		pr_err("%s, invalid tp %d\n", __func__, v);
		return -EINVAL;
	}

	den = (s64)tp[v].denominator;

	if (tp[v].volt_src == VREG_OUT || tp[v].volt_src == V0_OUT)
		vsrc = VREGOUT;
	else if (tp[v].volt_src == VT_OUT)
		vsrc = tp[TP_VT].vout[c];
	else {
		vsrc = tp[TP_VT].vout[c];
		pr_warn("%s unknown tp[%d].volt_src %d\n",
				__func__, v, tp[v].volt_src);
	}

	if (v == TP_VT) {
		s32 offset = tp[v].center[c] + tp[v].offset[c];
		if ((offset > 0xF) || (offset < 0x0)) {
			pr_warn("%s, warning : ivt (%d) out of range(0x0 ~ 0xF) replace to %s\n",
				__func__, offset, offset > 0xF ? "0xF" : "0x0" );
			offset = offset > 0xF ? 0xF : 0x0;
		}
		vreg = vsrc - VREF;
		num = vreg * (s64)vt_voltage[offset];
		res = vsrc - disp_div64(num, den);
	} else if (v == TP_V0) {
		s32 offset = tp[v].center[c] + tp[v].offset[c];
		if ((offset > 0xF) || (offset < 0x0)) {
			pr_warn("%s, warning : ivt (%d) out of range(0x0 ~ 0xF) replace to %s\n",
				__func__, offset, offset > 0xF ? "0xF" : "0x0" );
			offset = offset > 0xF ? 0xF : 0x0;
		}
		vreg = vsrc - VREF;
		num = vreg * (s64)v0_voltage[offset];
		res = vsrc - disp_div64(num, den);
	} else if (v == TP_V255) {
		vreg = vsrc - VREF;
		num = vreg * ((s64)tp[v].numerator + tp[v].center[c] + tp[v].offset[c]);
		res = vsrc - disp_div64(num, den);
	} else {
		vreg = vsrc - tp[v + 1].vout[c];
		num = vreg * ((s64)tp[v].numerator + tp[v].center[c] + tp[v].offset[c]);
		res = vsrc - disp_div64(num, den);
	}

#ifdef DEBUG_DIMMING
	pr_info("%s, %s %s vreg %lld, vsrc %lld, num %lld, den %lld, res %lld\n",
			__func__, tp[(int)v].name, color_name[(int)c], vreg, vsrc, num, den, res);
#endif
	if (res < 0)
		pr_err("%s, res %lld should not be minus value\n", __func__, res);

	return res;
}

static s32 mtp_vregout_to_offset(struct dimming_info *dim_info,
		int v, enum color c, int luminance_index)
{
	s64 num, den, res, vsrc;
	u32 upper;
	s32(*rgb_offset)[MAX_COLOR];
	s64 VREGOUT = dim_info->vregout;
	s64 VREF = dim_info->vref;
	struct dimming_lut *dim_lut = dim_info->dim_lut;
	struct tp *tp = dim_info->tp;
	struct dimming_tp_output *tpout;

	if (!tp || v < 0 || v >= NR_TP) {
		pr_err("%s, invalid tp %d\n", __func__, v);
		return -EINVAL;
	}

	if (luminance_index < 0 || luminance_index >= NR_LUMINANCE) {
		pr_err("%s, out of range luminance index (%d)\n", __func__, luminance_index);
		return 0;
	}

	if (tp[v].bits > 0)
		upper = (1 << tp[v].bits) - 1;
	else
		upper = ((v == TP_V255) ? 511: 255);

	rgb_offset = dim_lut[luminance_index].rgb_color_offset;
	tpout = dim_lut[luminance_index].tpout;

	if (tp[v].volt_src == VREG_OUT || tp[v].volt_src == V0_OUT)
		vsrc = VREGOUT;
	else if (tp[v].volt_src == VT_OUT)
		vsrc = tpout[TP_VT].vout[c];
	else {
		vsrc = tpout[TP_VT].vout[c];
		pr_warn("%s unknown tp[%d].volt_src %d\n",
				__func__, v, tp[v].volt_src);
	}

	if (v == TP_V255) {
		num = (vsrc - tpout[v].vout[c]) * (s64)tp[v].denominator;
		den = vsrc - VREF;
	} else {
		num = (vsrc - tpout[v].vout[c]) * (s64)tp[v].denominator;
		den = vsrc - tpout[v + 1].vout[c];
	}

#ifdef DIMMING_CALC_PRECISE
	/* To get precise value, use disp_div64_round() function */
	num -= den * (tp[v].numerator);
	num = disp_round(num, 7) - den * (s64)tp[v].offset[c];
	res = disp_div64_round(num, den, 2);
	pr_debug("lum %d, %s %s  num %lld, den %lld, res %lld\n", dim_lut[luminance_index].luminance, tp[v].name, color_name[c], num, den, res);
#else
	num -= den * ((s64)tp[v].numerator + (s64)tp[v].offset[c]);
	res = disp_div64(num, den);
#endif
	//if (in_range(v, dim_lut_info->rgb_color_offset_range))
	res += rgb_offset[v][c];

#ifdef DEBUG_DIMMING
	pr_debug("luminance %3d %5s %5s num %lld\t den %lld\t rgb_offset %d\t res %lld\n",
			dim_lut[luminance_index].luminance, tp[v].name, color_name[c], num, den, rgb_offset[v][c], res);
#endif

	if (res < 0)
		res = 0;
	if (res > upper)
		res = upper;

	return (s32)res;
}

/*
 * ########### BASE LUMINANCE GRAY SCALE TABLE FUNCTIONS ###########
 */

/*
 * sams as excel macro function min_diff_gray().
 */
static int find_gray_scale_gamma_value(struct gray_scale *gray_scale_lut, u64 gamma_value)
{
	int l = 0, m, r = GRAY_SCALE_MAX - 1;
	s64 delta_l, delta_r;

	if (unlikely(!gray_scale_lut)) {
		pr_err("%s, gray_scale_lut not exist\n", __func__);
		return -EINVAL;
	}

	if ((gray_scale_lut[l].gamma_value > gamma_value) ||
			(gray_scale_lut[r].gamma_value < gamma_value)) {
		pr_err("%s, out of range([l:%d, r:%d], [%lld, %lld]) candela(%lld)\n",
				__func__, l, r, gray_scale_lut[l].gamma_value,
				gray_scale_lut[r].gamma_value, gamma_value);
		return -EINVAL;
	}

	while (l <= r) {
		m = l + (r - l) / 2;
		if (gray_scale_lut[m].gamma_value == gamma_value)
			return m;
		if (gray_scale_lut[m].gamma_value < gamma_value)
			l = m + 1;
		else
			r = m - 1;
	}
	delta_l = gamma_value - gray_scale_lut[r].gamma_value;
	delta_r = gray_scale_lut[l].gamma_value - gamma_value;

	return delta_l <= delta_r ? r : l;
}

s64 interpolation(s64 from, s64 to, int cur_step, int total_step)
{
	s64 num, den;

	if (cur_step == total_step || from == to)
		return to;

	num = (to - from) * (s64)cur_step;
	den = (s64)total_step;
	num = from + disp_div64(num, den);

	return num;
}

int gamma_table_add_offset(s32 (*src)[MAX_COLOR], s32 (*ofs)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], struct tp *tp, int nr_tp)
{
	int v, c, upper, res;

	if (unlikely(!tp || nr_tp == 0 || !src|| !ofs || !out)) {
		pr_err("%s, invalid parameter (tp %d, nr_tp %d, src %d, ofs %d, out %d)\n",
				__func__, !!tp, nr_tp, !!src, !!ofs, !!out);
		return -EINVAL;
	}

	for (v = 0; v < nr_tp; v++) {
		upper = (1 << tp[v].bits) - 1;
		for_each_color(c) {
			res = src[v][c] + ofs[v][c];
			if (res < 0)
				res = 0;
			if (res > upper)
				res = upper;

			out[v][c] = res;
#ifdef DEBUG_DIMMING
			pr_info("%s, src %d, ofs %d, out %d\n",
					__func__, src[v][c], ofs[v][c], out[v][c]);
#endif
		}
	}

	return 0;
}

int gamma_table_interpolation(s32 (*from)[MAX_COLOR], s32 (*to)[MAX_COLOR],
		s32 (*out)[MAX_COLOR], int nr_tp, int cur_step, int total_step)
{
	int i, c;

	if (unlikely(!from || !to || !out)) {
		pr_err("%s, invalid parameter (from %p, to %p, out %p)\n",
				__func__, from, to, out);
		return -EINVAL;
	}

	if (unlikely(nr_tp < 0 || cur_step > total_step)) {
		pr_err("%s, out of range (nr_tp %d, cur_step %d, total_step %d)\n",
				__func__, nr_tp, cur_step, total_step);
		return -EINVAL;
	}

	for (i = 0; i < nr_tp; i++) {
		for_each_color(c) {
			out[i][c] =
				interpolation(from[i][c], to[i][c], cur_step, total_step);
#ifdef DEBUG_DIMMING
			pr_info("%s, from %d, to %d, cur_step %d, total_step %d\n",
					__func__, from[i][c], to[i][c], cur_step, total_step);
#endif
		}
	}

	return 0;
}

static int generate_gray_scale(struct dimming_info *dim_info)
{
	int i, c, iv = 0, iv_upper = 0, iv_lower = 0, ret = 0, cur_step, total_step = 0;
	s64 vout, v_upper, v_lower;
	struct gray_scale *gray_scale_lut = dim_info->gray_scale_lut;
	struct tp *tp = dim_info->tp;
	int nr_tp = dim_info->nr_tp;

	for (i = 0, iv = (TP_V0 > 0 ? TP_V0 : TP_VT);
			iv < nr_tp && i < GRAY_SCALE_MAX; i++) {
		if (i == tp[iv].level) {
			iv_upper = tp[((iv == nr_tp - 1) ? iv : iv + 1)].level;
			iv_lower = tp[iv].level;
			total_step = iv_upper - iv_lower;
			iv++;
			continue;
		}

		cur_step = i - iv_lower;
		for_each_color(c) {
			v_upper = (s64)gray_scale_lut[iv_upper].vout[c];
			v_lower = (s64)gray_scale_lut[iv_lower].vout[c];
			vout = interpolation(v_lower, v_upper, cur_step, total_step);
#ifdef DEBUG_DIMMING
			pr_info("lower %3d, upper %3d, "
					"cur_step %3d, total_step %3d, vout[%d]\t "
					"from %2lld.%04lld, vout %2lld.%04lld, to %2lld.%04lld\n",
					iv_lower, iv_upper, cur_step, total_step, c,
					scale_down_round(v_lower, 4), scale_down_rem(v_lower, 4),
					scale_down_round(vout, 4), scale_down_rem(vout, 4),
					scale_down_round(v_upper, 4), scale_down_rem(v_upper, 4));
#endif
			if (vout < 0) {
				pr_warn("%s, from %2lld.%4lld, to %2lld.%4lld (idx %-3d %-3d), cur_step %-3d, total_step %-3d\n",
						__func__, scale_down_round(v_lower, 4), scale_down_rem(v_lower, 4),
						scale_down_round(v_upper, 4), scale_down_rem(v_upper, 4),
						iv_lower, iv_upper, cur_step, total_step);
				ret = -EINVAL;
			}
			gray_scale_lut[i].vout[c] = vout;
		}
	}

	for (i = 0; i < GRAY_SCALE_MAX; i++) {
		/* base luminance gamma curve value */
		gray_scale_lut[i].gamma_value = TARGET_LUMINANCE * (u64)gamma_curve_value(TARGET_G_CURVE_DEGREE, i);
#ifdef DEBUG_DIMMING
		pr_info("%-4d gamma %3lld.%02lld\n", i,
				scale_down_round(gray_scale_lut[i].gamma_value, 2),
				scale_down_rem(gray_scale_lut[i].gamma_value, 2));
#endif
	}

	return ret;
}

/*
 * ########### GAMMA CORRECTION TABLE FUNCTIONS ###########
 */
int find_luminance(struct dimming_info *dim_info, u32 luminance)
{
	int index, nr_luminance;
	struct dimming_lut *dim_lut;

	if (unlikely(!dim_info)) {
		pr_err("%s, invalid dim_info\n", __func__);
		return -1;
	}

	dim_lut = dim_info->dim_lut;
	nr_luminance = dim_info->nr_luminance;

	if (unlikely(!dim_lut)) {
		pr_err("%s, invalid dim_lut\n", __func__);
		return -1;
	}

	for (index = 0; index < nr_luminance; index++)
		if (dim_lut[index].luminance == luminance)
			return index;

	return -1;
}

int get_luminance(struct dimming_info *dim_info, u32 index)
{
	int nr_luminance;
	struct dimming_lut *dim_lut;

	if (unlikely(!dim_info)) {
		pr_err("%s, invalid dim_info\n", __func__);
		return 0;
	}

	dim_lut = dim_info->dim_lut;
	nr_luminance = dim_info->nr_luminance;

	if (unlikely(!dim_lut)) {
		pr_err("%s, invalid dim_lut\n", __func__);
		return 0;
	}

	if (index >= nr_luminance) {
		pr_warn("%s, exceed max %d, index %d\n",
				__func__, nr_luminance - 1, index);
		return dim_lut[nr_luminance - 1].luminance;
	}
	return dim_lut[index].luminance;
}

static void copy_tpout_center(struct dimming_info *dim_info, u32 luminance, u8 *output,
		void (*copy)(u8 *output, u32 value, u32 index, u32 color))
{
	struct dimming_lut *dim_lut = dim_info->dim_lut;
	struct tp *tp = dim_info->tp;
	int ilum = find_luminance(dim_info, luminance);
	int i, c, value;

	if (unlikely(!tp || !dim_lut)) {
		pr_err("%s, invalid tp or dim_lut\n", __func__);
		return;
	}

	if (unlikely(ilum < 0)) {
		pr_err("%s, luminance(%d) not found\n", __func__, luminance);
		return;
	}

	for (i = TP_V255; i >= TP_VT; i--) {
		for_each_color(c) {
			value = dim_lut[ilum].tpout[i].center[c];
			copy(output, value, i, c);
		}
	}
}

static void copy_tpout_hbm_center(struct dimming_info *dim_info, u32 luminance, u8 *output,
		void (*copy)(u8 *output, u32 value, u32 index, u32 color))
{
	struct dimming_lut *dim_lut = dim_info->dim_lut;
	struct tp *tp = dim_info->tp;
	int i, c, value, ilum;
	int nr_tp = dim_info->nr_tp;
	s32 (*target_gamma_tbl)[MAX_COLOR];
	s32 (*output_gamma_tbl)[MAX_COLOR];

	if (unlikely(!tp || !dim_lut)) {
		pr_err("%s, invalid tp or dim_lut\n", __func__);
		return;
	}

	target_gamma_tbl =
		(s32 (*)[MAX_COLOR])kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	if (unlikely(!target_gamma_tbl)) {
		pr_err("%s. failed to allocate target_gamma_tbl\n", __func__);
		goto err;
	}

	output_gamma_tbl =
		(s32 (*)[MAX_COLOR])kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	if (unlikely(!output_gamma_tbl)) {
		pr_err("%s. failed to allocate output_gamma_tbl\n", __func__);
		goto err_alloc;
	}

	/* make target gamma table */
	ilum = find_luminance(dim_info, dim_info->target_luminance);
	if (unlikely(ilum < 0)) {
		pr_err("%s, target_luminance(%d) not found\n",
				__func__, dim_info->target_luminance);
		goto err_find;
	}

	for (i = 0; i < nr_tp; i++)
		for_each_color(c)
			target_gamma_tbl[i][c] = dim_lut[ilum].tpout[i].center[c];

	gamma_table_interpolation(target_gamma_tbl, dim_info->hbm_gamma_tbl, output_gamma_tbl,
			dim_info->nr_tp, luminance - dim_info->target_luminance,
			dim_info->hbm_luminance - dim_info->target_luminance);

	for (i = TP_V255; i >= TP_VT; i--) {
		for_each_color(c) {
			value = output_gamma_tbl[i][c];
			copy(output, value, i, c);
		}
	}

err_find:
	kfree(output_gamma_tbl);
err_alloc:
	kfree(target_gamma_tbl);
err:

	return;
}

void get_dimming_gamma(struct dimming_info *dim_info, u32 luminance, u8 *output,
		void (*copy)(u8 *output, u32 value, u32 index, u32 color))
{
	if (unlikely(!dim_info || !output)) {
		pr_err("%s, invalid dim_info or output\n", __func__);
		return;
	}

	if (luminance <= dim_info->target_luminance)
		copy_tpout_center(dim_info, luminance, output, copy);
	else if (dim_info->hbm_luminance && luminance <= dim_info->hbm_luminance)
		copy_tpout_hbm_center(dim_info, luminance, output, copy);
	else
		pr_warn("%s, out of range (hbm_luminance %d, luminance %d)\n",
				__func__, dim_info->hbm_luminance, luminance);

	return;
}

void print_dimming_tp_output(struct dimming_lut *dim_lut, struct tp *tp, int size)
{
	int i, v;
	struct dimming_tp_output *tpout;
	u32 luminance;

	for (i = 0; i < size; i++) {
		tpout = dim_lut[i].tpout;
		luminance = dim_lut[i].luminance;
		for (v = 0; v < NR_TP; v++) {
			pr_info("luminance %3d %5s "
				"L %3lld.%05lld\t  M_GRAY %3u "
				"R %3lld.%05lld\t G %3lld.%05lld\t B %3lld.%05lld\t "
				"%3X %3X %3X\n",
				luminance, tp[v].name,
				scale_down_round(tpout[v].L, 5), scale_down_rem(tpout[v].L, 5), tpout[v].M_GRAY,
				scale_down_round(tpout[v].vout[RED], 5), scale_down_rem(tpout[v].vout[RED], 5),
				scale_down_round(tpout[v].vout[GREEN], 5), scale_down_rem(tpout[v].vout[GREEN], 5),
				scale_down_round(tpout[v].vout[BLUE], 5), scale_down_rem(tpout[v].vout[BLUE], 5),
				tpout[v].center[RED], tpout[v].center[GREEN], tpout[v].center[BLUE]);
		}
	}
	pr_info("\n");
}

static int generate_gamma_table(struct dimming_info *dim_info)
{
	int i, v, c, ilum;
	enum gamma_degree g_curve_degree;
	struct dimming_tp_output *tpout;
	struct dimming_lut *dim_lut = dim_info->dim_lut;
	struct gray_scale *gray_scale_lut = dim_info->gray_scale_lut;
	struct tp *tp = dim_info->tp;
	int NR_TP = dim_info->nr_tp;
	int TARGET_LUMINANCE = dim_info->target_luminance;
	enum gamma_degree TARGET_G_CURVE_DEGREE = dim_info->target_g_curve_degree;
	u32 luminance;

	if (unlikely(!calc_gray_order)) {
		pr_err("%s, calc_gray_order not exist!!\n", __func__);
		return -EINVAL;
	}

	if (unlikely(!calc_vout_order)) {
		pr_err("%s, calc_vout_order not exist!!\n", __func__);
		return -EINVAL;
	}

	for (ilum = 0; ilum < NR_LUMINANCE; ilum++) {
		luminance = dim_lut[ilum].luminance;
		tpout = dim_lut[ilum].tpout;
		for (i = 0; i < NR_TP; i++) {
			v = calc_gray_order[i];
			g_curve_degree = dim_lut[ilum].g_curve_degree;

			/* TODO : need to optimize */
			if (v == TP_V255)
				tpout[v].L = (dim_lut[ilum].base_luminance) ? ((u64)dim_lut[ilum].base_luminance << BIT_SHIFT) :
					(u64)TARGET_LUMINANCE * (u64)gamma_curve_value(TARGET_G_CURVE_DEGREE, dim_lut[ilum].base_gray);
			else if (v == TP_VT)
				tpout[v].L = (luminance < 251) ? 0 :
					((tpout[TP_V255].L * (u64)gamma_curve_value(g_curve_degree, tp[v].level)) >> BIT_SHIFT);
			else
				tpout[v].L = (tpout[TP_V255].L * (u64)gamma_curve_value(g_curve_degree, tp[v].level)) >> BIT_SHIFT;

			/*
			 * In two special case, M_GRAY should be same with tp value
			 * 1. tp is VT (v == TP_VT)
			 * 2. tp's value is 1 (tp[v].level == 1)
			 */
			tpout[v].M_GRAY = (tp[v].level <= 1) ? tp[v].level :
				find_gray_scale_gamma_value(gray_scale_lut, tpout[v].L) +
				dim_lut[ilum].gray_scale_offset[v];

			if (tpout[v].M_GRAY > 255)
				tpout[v].M_GRAY = 255;

			for_each_color(c) {
				dim_lut[ilum].tpout[v].vout[c] =
					(v == TP_VT) ? tp[v].vout[c] :
					gray_scale_lut[tpout[v].M_GRAY].vout[c];
			}
		}

		for (i = 0; i < NR_TP; i++) {
			v = calc_vout_order[i];
			for_each_color(c) {
				dim_lut[ilum].tpout[v].center[c] =
					(v == TP_VT || v == TP_V0) ? tp[v].center[c] :
					mtp_vregout_to_offset(dim_info, v, (enum color)c, ilum);
			}
		}
#ifdef DEBUG_DIMMING
		print_dimming_tp_output(&dim_lut[ilum], tp, 1);
#endif
	}

	return 0;
}

int alloc_dimming_info(struct dimming_info *dim_info, int nr_tp, int nr_luminance)
{
	int i;

	dim_info->nr_tp = nr_tp;
	dim_info->nr_luminance = nr_luminance;

	dim_info->tp = kzalloc(sizeof(struct tp) * nr_tp, GFP_KERNEL);

	/* allocation of struct dimming_lut lut and members */
	dim_info->dim_lut =
		(struct dimming_lut *)kzalloc(sizeof(struct dimming_lut) * nr_luminance, GFP_KERNEL);
	for (i = 0; i < nr_luminance; i++) {
		dim_info->dim_lut[i].tpout =
			kzalloc(sizeof(struct dimming_tp_output) * nr_tp, GFP_KERNEL);
		dim_info->dim_lut[i].gray_scale_offset =
			(s32 *)kzalloc(sizeof(s32) * nr_tp, GFP_KERNEL);
		dim_info->dim_lut[i].rgb_color_offset =
			(s32 (*)[MAX_COLOR])kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	}

	return 0;
}

int free_dimming_info(struct dimming_info *dim_info)
{
	int i;

	for (i = 0; i < dim_info->nr_luminance; i++) {
		kfree(dim_info->dim_lut[i].tpout);
		kfree(dim_info->dim_lut[i].gray_scale_offset);
		kfree(dim_info->dim_lut[i].rgb_color_offset);
	}
	kfree(dim_info->dim_lut);
	kfree(dim_info->tp);

	dim_info->nr_luminance = 0;
	dim_info->nr_tp = 0;

	return 0;
}

int find_tp_index(struct tp *tp, int nr_tp, char *name)
{
	int i;

	for (i = 0; i < nr_tp; i++)
		if (!strcmp(tp[i].name, name))
			return i;

	return -1;
}

void prepare_dim_info(struct dimming_info *dim_info)
{
#ifdef DEBUG_DIMMING
	int i, len;
	char buf[MAX_PRINT_BUF_SIZE];
#endif
	struct tp *tp = dim_info->tp;

	NR_TP = dim_info->nr_tp;
	NR_LUMINANCE = dim_info->nr_luminance;
	TARGET_LUMINANCE = dim_info->target_luminance;
	TARGET_G_CURVE_DEGREE = dim_info->target_g_curve_degree;

	TP_VT = find_tp_index(tp, dim_info->nr_tp, "VT");
	TP_V0 = find_tp_index(tp, dim_info->nr_tp, "V0");
	TP_V255 = dim_info->nr_tp - 1;


#ifdef DEBUG_DIMMING
	pr_info("NR_TP %d\n", NR_TP);
#endif

	if (!calc_gray_order) {
		calc_gray_order = generate_order(NR_TP - 1, 0, NR_TP - 2);
		if (unlikely(!calc_gray_order)) {
			pr_err("%s, calc_gray_order not exist!!\n", __func__);
			return;
		}
#ifdef DEBUG_DIMMING
		len = snprintf(buf, MAX_PRINT_BUF_SIZE, "[calc_gray_order]\n");
		for (i = 0; i < NR_TP; i++)
			len += snprintf(buf + len,
					max(MAX_PRINT_BUF_SIZE - len, 0), "[%d]%s ", i,
					((calc_gray_order[i] >= 0 && calc_gray_order[i] < NR_TP) ?
					tp[calc_gray_order[i]].name : "TP_ERR"));
		pr_info("%s\n", buf);
#endif
	}

	if (!calc_vout_order) {
		calc_vout_order = generate_order(0, NR_TP - 1, 1);
		if (unlikely(!calc_vout_order)) {
			pr_err("%s, calc_vout_order not exist!!\n", __func__);
			return;
		}
#ifdef DEBUG_DIMMING
		len = snprintf(buf, MAX_PRINT_BUF_SIZE, "[calc_vout_order]\n");
		for (i = 0; i < NR_TP; i++)
			len += snprintf(buf + len,
					max(MAX_PRINT_BUF_SIZE - len, 0), "[%d]%s ", i,
					((calc_vout_order[i] >= 0 && calc_vout_order[i] < NR_TP) ?
					tp[calc_vout_order[i]].name : "TP_ERR"));
		pr_info("%s\n", buf);
#endif
	}
}

void print_tp_lut(struct dimming_info *dim_info)
{
	int i, len, col, ncol, *fields;
	char buf[MAX_PRINT_BUF_SIZE];

	ncol = dim_info->tp_lut_info.ncol;
	fields = dim_info->tp_lut_info.fields;

	for (i = 0; i < dim_info->nr_tp; i++) {
		for (len = 0, col = 0; col < ncol; col++) {
			switch (fields[col]) {
			case TP_LUT_LEVEL:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%-3d ", dim_info->tp[i].level);
				break;
			case TP_LUT_VOLT_SRC:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%-10s ", volt_src_name[dim_info->tp[i].volt_src]);
				break;
			case TP_LUT_GAMMA_CENTER:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%-3X %-3X %-3X  ",
						dim_info->tp[i].center[RED],
						dim_info->tp[i].center[GREEN],
						dim_info->tp[i].center[BLUE]);
				break;
			case TP_LUT_GAMMA_CALC:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%-3d %-3d", dim_info->tp[i].numerator,
						dim_info->tp[i].denominator);
				break;
			default:
				break;
			}
		}
		pr_info("%s\n", buf);
	}
}

void print_dim_lut(struct dimming_info *dim_info)
{
	int ilum, i, c, len, col, ncol, *fields;
	char buf[MAX_PRINT_BUF_SIZE];
	struct dimming_lut_info *dim_lut_info;

	dim_lut_info = &dim_info->dim_lut_info;
	ncol = dim_info->dim_lut_info.ncol;
	fields = dim_info->dim_lut_info.fields;

	for (ilum = 0; ilum < dim_info->nr_luminance; ilum++) {
		struct dimming_lut *arr = &dim_info->dim_lut[ilum];
		for (len = 0, col = 0; col < ncol; col++) {
			switch (fields[col]) {
			case DIM_LUT_LUMINANCE:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%-3d    ", arr->luminance);
				break;
			case DIM_LUT_AOR:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%04X  ", arr->aor);
				break;
			case DIM_LUT_BASE_LUMINANCE:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%3d  ", arr->base_luminance);
				break;
			case DIM_LUT_GAMMA:
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0),
						"%3d    ", gamma_curve_lut[arr->g_curve_degree].gamma);
				break;
			case DIM_LUT_GRAY_SCALE_OFFSET:
				for_each_range(i, dim_lut_info->gray_scale_offset_range)
					len += snprintf(buf + len,
							max(MAX_PRINT_BUF_SIZE - len, 0),
							"%3d ", arr->gray_scale_offset[i]);
				len += snprintf(buf + len,
						max(MAX_PRINT_BUF_SIZE - len, 0), "\t ");
				break;
			case DIM_LUT_COLOR_SHIFT_OFFSET:
				for_each_range(i, dim_lut_info->rgb_color_offset_range)
					for_each_color(c)
						len += snprintf(buf + len,
								max(MAX_PRINT_BUF_SIZE - len, 0),
								"%3d ", arr->rgb_color_offset[i][c]);
				break;
			default:
				break;
			}
		}
		pr_info("%s\n", buf);
	}
}

void print_dimming_info(struct dimming_info *dim_info, int tag)
{
	static char buf[MAX_PRINT_BUF_SIZE];
	size_t i;
	int len, col, ncol, *fields;
	struct dimming_lut_info *dim_lut_info = &dim_info->dim_lut_info;

	pr_info("%s\n", tag_name[tag]);
	switch (tag) {
	case TAG_DIM_GLOBAL_INFO_START:
		pr_info("%-15s %u\n", global_dim_info_name[GLOBAL_DIM_INFO_NR_TP],
				dim_info->nr_tp);
		pr_info("%-15s %u\n", global_dim_info_name[GLOBAL_DIM_INFO_NR_LUMINANCE],
				dim_info->nr_luminance);
		pr_info("%-15s %lld %d\n", global_dim_info_name[GLOBAL_DIM_INFO_VREGOUT],
				dim_info->vregout, DIMMING_BITSHIFT);
		pr_info("%-15s %lld %d\n", global_dim_info_name[GLOBAL_DIM_INFO_VREF],
				dim_info->vref, DIMMING_BITSHIFT);
		pr_info("%-15s %d\n", global_dim_info_name[GLOBAL_DIM_INFO_GAMMA],
				gamma_curve_lut[(int)dim_info->target_g_curve_degree].gamma);
		len = snprintf(buf, MAX_PRINT_BUF_SIZE,
				"%-15s ", global_dim_info_name[GLOBAL_DIM_INFO_VT_VOLTAGE]);
		for (i = 0; i < ARRAY_SIZE(dim_info->vt_voltage); i++)
			len += snprintf(buf + len,
					max(MAX_PRINT_BUF_SIZE - len, 0),
					"%d  ", dim_info->vt_voltage[i]);
		pr_info("%s\n", buf);
		break;
	case TAG_TP_LUT_INFO_START:
		ncol = dim_info->tp_lut_info.ncol;
		fields = dim_info->tp_lut_info.fields;
		for (len = 0, col = 0; col < ncol; col++)
			len += snprintf(buf + len,
					max(MAX_PRINT_BUF_SIZE - len, 0),
					"%s    ", tp_lut_field_name[fields[col]]);
		pr_info("%s\n", buf);
		break;
	case TAG_TP_LUT_START:
		print_tp_lut(dim_info);
		break;
	case TAG_DIM_LUT_INFO_START:
		ncol = dim_lut_info->ncol;
		fields = dim_lut_info->fields;
		for (len = 0, col = 0; col < ncol; col++) {
			len += snprintf(buf + len,
					max(MAX_PRINT_BUF_SIZE - len, 0),
					"%s    ", dim_lut_field_name[fields[col]]);
			if (fields[col] == DIM_LUT_GRAY_SCALE_OFFSET) {
				len += snprintf(buf + len, max(MAX_PRINT_BUF_SIZE - len, 0), "%s  %s    ",
						dim_info->tp[dim_lut_info->gray_scale_offset_range.from].name,
						dim_info->tp[dim_lut_info->gray_scale_offset_range.to -
						dim_lut_info->gray_scale_offset_range.step].name);
			} else if (fields[col] == DIM_LUT_COLOR_SHIFT_OFFSET) {
				len += snprintf(buf + len, max(MAX_PRINT_BUF_SIZE - len, 0), "%s  %s    ",
						dim_info->tp[dim_lut_info->rgb_color_offset_range.from].name,
						dim_info->tp[dim_lut_info->rgb_color_offset_range.to -
						dim_lut_info->rgb_color_offset_range.step].name);
			}
		}
		pr_info("%s\n", buf);
		break;
	case TAG_DIM_LUT_START:
		print_dim_lut(dim_info);
		break;
	case TAG_MTP_OFFSET_START:
		print_mtp_offset(dim_info);
		break;
	case TAG_GAMMA_CENTER_START:
		print_tpout_center(dim_info);
		break;
	case TAG_TP_VOLT_START:
		print_tp_vout(dim_info);
		break;
	case TAG_GRAY_SCALE_VOLT_START:
		print_gray_scale_vout(dim_info->gray_scale_lut);
		break;
	}
	pr_info("%s\n\n", tag_name[tag + 1]);
}

void print_input_data(struct dimming_info *dim_info)
{
	print_dimming_info(dim_info, TAG_DIM_GLOBAL_INFO_START);
	print_dimming_info(dim_info, TAG_TP_LUT_INFO_START);
	print_dimming_info(dim_info, TAG_TP_LUT_START);
	print_dimming_info(dim_info, TAG_DIM_LUT_INFO_START);
	print_dimming_info(dim_info, TAG_DIM_LUT_START);
#if 0
	print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
#endif
}

void print_tbl(struct dimming_info *dim_info, s32 (*tbl)[MAX_COLOR])
{
	int i, len;
	char buf[MAX_PRINT_BUF_SIZE];

	for (i = 0, len = 0; i < dim_info->nr_tp; i++)
		len += snprintf(buf + len,
				max(MAX_PRINT_BUF_SIZE - len, 0),
				"0x%03X 0x%03X 0x%03X\n",
				tbl[i][RED], tbl[i][GREEN], tbl[i][RED]);
	pr_info("%s\n", buf);
}

int init_dimming_info(struct dimming_info *dim_info, struct dimming_init_info *src)
{
	int ilum;

	memcpy(dim_info->vt_voltage, src->vt_voltage, sizeof(src->vt_voltage));
	memcpy(dim_info->v0_voltage, src->v0_voltage, sizeof(src->v0_voltage));
	dim_info->vregout = src->vregout;
	dim_info->vref = src->vref;
	dim_info->nr_tp = src->nr_tp;
	dim_info->tp = src->tp;
	dim_info->nr_luminance = src->nr_luminance;
	dim_info->dim_lut = src->dim_lut;

	/* allocation of struct dimming_lut lut and members */
	for (ilum = 0; ilum < dim_info->nr_luminance; ilum++) {
		dim_info->dim_lut[ilum].tpout = (struct dimming_tp_output *)
			kzalloc(sizeof(struct dimming_tp_output) * dim_info->nr_tp, GFP_KERNEL);
	}

	dim_info->target_luminance = src->target_luminance;
	dim_info->target_g_curve_degree =
		gamma_value_to_degree(src->target_gamma);
	if (unlikely(dim_info->target_g_curve_degree == GAMMA_NONE)) {
		pr_err("%s, invalid target gamma degree %d\n",
				__func__, dim_info->target_g_curve_degree);
		return -EINVAL;
	}

	return 0;
}

int init_dimming_mtp(struct dimming_info *dim_info, s32 (*mtp)[MAX_COLOR])
{
	int i, c;

	for (i = 0; i < dim_info->nr_tp; i++)
		for_each_color(c)
			dim_info->tp[i].offset[c] = mtp[i][c];

	pr_info("%s, init mtp offset\n", __func__);
	print_mtp_offset(dim_info);

	return 0;
}

int init_dimming_hbm_info(struct dimming_info *dim_info, s32 (*hbm_gamma_tbl)[MAX_COLOR], u32 hbm_luminance)
{
	int i, c, nr_tp;

	if (unlikely(!dim_info || !hbm_gamma_tbl)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (unlikely(hbm_luminance < dim_info->target_luminance)) {
		pr_err("%s, out of range (hbm_luminance %d < target_luminance %d)\n",
				__func__, hbm_luminance,
				dim_info->target_luminance);
		return -EINVAL;
	}

	nr_tp = dim_info->nr_tp;
	if (!dim_info->hbm_gamma_tbl)
		dim_info->hbm_gamma_tbl =
			(s32 (*)[MAX_COLOR])kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);

	for (i = 0; i < dim_info->nr_tp; i++)
		for_each_color(c)
			dim_info->hbm_gamma_tbl[i][c] = hbm_gamma_tbl[i][c];
	dim_info->hbm_luminance = hbm_luminance;
#ifdef DEBUG_DIMMING
	pr_info("%s, init hbm gamma table (hbm_luminance %d)\n",
			__func__, hbm_luminance);
	print_hbm_gamma_tbl(dim_info);
#endif

	return 0;
}

int process_dimming(struct dimming_info *dim_info)
{
	int i, c, v;
	struct tp *tp;
	s64 VREGOUT;
	s64 VREF;
#ifdef DEBUG_EXCUTION_TIME
	mytime_t ts, te;

	gettime(ts);
#endif

	if (unlikely(!dim_info)) {
		pr_err("%s, invalid dim_info\n", __func__);
		return -EINVAL;
	}
	VREGOUT = dim_info->vregout;
	VREF = dim_info->vref;
	prepare_dim_info(dim_info);
#ifdef DEBUG_DIMMING
	print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
#endif

	tp = dim_info->tp;
	if (unlikely(!tp)) {
		pr_err("%s, tuning point not prepared\n", __func__);
		return -EINVAL;
	}

	/*
	 * Generate RGB voltage output table of TP using MTP(TP_VT ~ TP_V255).
	 */
	for (i = 0; i < dim_info->nr_tp; i++) {
		v = calc_vout_order[i];
		for_each_color(c) {
			tp[v].vout[c] = mtp_offset_to_vregout(dim_info, v, (enum color)c);
			if (tp[v].vout[c] < 0) {
				pr_warn("%s, invalid vout[%s][%s] %lld\n",
						__func__, tp[v].name, color_name[c],
						tp[v].vout[c]);
			}
			dim_info->gray_scale_lut[tp[v].level].vout[c] =
				(v == TP_VT) ? (u64)VREGOUT : (u64)tp[v].vout[c];
		}
	}
#ifdef DEBUG_DIMMING
	print_dimming_info(dim_info, TAG_TP_VOLT_START);
#endif

	/*
	 * Fill in the all RGB voltate output of gray scale by interpolation.
	 */
	generate_gray_scale(dim_info);

#ifdef DEBUG_DIMMING
	print_dimming_info(dim_info, TAG_GRAY_SCALE_VOLT_START);
#endif

	/*
	 * Generate mtp offset of given luminances.
	 */
	generate_gamma_table(dim_info);

#if 0
	print_dimming_tp_output(dim_lut, dim_info->tp, dim_info->nr_luminance);
	print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
#endif
#ifdef DEBUG_DIMMING_RESULT
	print_dimming_info(dim_info, TAG_GAMMA_CENTER_START);
#endif
#ifdef DEBUG_EXCUTION_TIME
	gettime(te);
	pr_info("elapsed time %u us\n", difftime(ts, te));
#endif
	/* TODO : free allocated heap */

	return 0;
}
