// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/bug.h>


#include "panel_irc.h"
#include "panel.h"
#include "panel_drv.h"


bool is_hbm_zone(struct brightness_table *brt_tbl, int brightness)
{
	bool retVal = false;
	int size_ui_lum = (brt_tbl->sz_panel_dim_ui_lum != 0) ?
		brt_tbl->sz_panel_dim_ui_lum : brt_tbl->sz_ui_lum;
	unsigned int normal_max_brt = brt_tbl->brt[size_ui_lum - 1];

	if (normal_max_brt < brightness)
		retVal = true;
	return retVal;
}
bool is_hbm_800(int hbm_max)
{
	bool retVal = false;

	if (hbm_max >= 800)
		retVal = true;
	return retVal;
}

int __generate_irc_v1(struct brightness_table *brt_tbl, struct panel_irc_info *info, u64 luminance, int brightness)
{
	int size_ui_lum = (brt_tbl->sz_panel_dim_ui_lum != 0) ?
		brt_tbl->sz_panel_dim_ui_lum : brt_tbl->sz_ui_lum;
	unsigned int *lum_tbl = brt_tbl->lum;
	int gray = 0, color = 0, i = 0;
	u64 coef = disp_pow(10, SCALEUP_5);
	int normal_max = lum_tbl[size_ui_lum - 1];
	u64 scaleup_normal_max = normal_max * disp_pow(10, SCALEUP_4);
	int hbm_max = lum_tbl[brt_tbl->sz_lum - 1];
	u64 temp_val = 0, lum_gap = 0, brt_gap = 0;
	int dynamic_offset = info->dynamic_offset;

	/*
	 *	hbm 800 : calculate coefficient value refer excel sheet from d.lab
	 *	other : coef is 1
	 */
	if (is_hbm_800(hbm_max)) {
		if (is_hbm_zone(brt_tbl, brightness)) {				// is hbm
			lum_gap = luminance - scaleup_normal_max;
			lum_gap *= disp_pow(10, SCALEUP_4); // for precision scale up
			brt_gap = hbm_max - normal_max;
			temp_val = (lum_gap * info->hbm_coef) / brt_gap;
			temp_val = disp_pow_round(temp_val, SCALEUP_4);
			temp_val /= disp_pow(10, SCALEUP_4); // scanel down
			coef = coef - temp_val;
		}
	}

	for (gray = 0; gray < 3; gray++) {
		for (color = 0; color < 3; color++) {
			for (i = 0, temp_val = 0; i <= gray; i++)
				temp_val += info->ref_tbl[dynamic_offset + i * 3 + color];
			temp_val = disp_div64(temp_val * luminance, lum_tbl[size_ui_lum - 1]);
			temp_val *= coef;
			info->buffer[dynamic_offset + gray * 3 + color] = (u8)(disp_pow_round(temp_val, 9) / disp_pow(10, SCALEUP_9));
			for (i = 0; i < gray; i++)
				info->buffer[dynamic_offset + gray * 3 + color] -= info->buffer[dynamic_offset + i * 3 + color];
		}
	}
	return 0;
}

int __generate_irc_v2(struct brightness_table *brt_tbl, struct panel_irc_info *info, u64 luminance, int brightness)
{
	u64 coef = disp_pow(10, SCALEUP_5);
	int size_ui_lum = (brt_tbl->sz_panel_dim_ui_lum != 0) ?
		brt_tbl->sz_panel_dim_ui_lum : brt_tbl->sz_ui_lum;
	unsigned int *lum_tbl = brt_tbl->lum;
	int normal_max = lum_tbl[size_ui_lum - 1];
	u64 scaleup_normal_max = normal_max * disp_pow(10, SCALEUP_4);
	int hbm_max = lum_tbl[brt_tbl->sz_lum - 1];
	u64 lum_gap = 0, brt_gap = 0, temp_val = 0;
	int i = 0;

	if (is_hbm_zone(brt_tbl, brightness)) {				// is hbm
		lum_gap = luminance - scaleup_normal_max;
		lum_gap *= disp_pow(10, SCALEUP_4); // for precision scale up
		brt_gap = hbm_max - normal_max;
		temp_val = (lum_gap * info->hbm_coef) / brt_gap;
		temp_val = disp_pow_round(temp_val, SCALEUP_4);
		temp_val /= disp_pow(10, SCALEUP_4); // scanel down
		coef = coef - temp_val;
	}

	for (i = info->dynamic_offset; i < info->dynamic_offset + info->dynamic_len; i++) {
		temp_val = coef;
		temp_val *= info->ref_tbl[i];
		temp_val = (u64)disp_pow_round(disp_div64(temp_val * luminance, lum_tbl[size_ui_lum - 1]), SCALEUP_9);
		info->buffer[i] = (u8)(temp_val / (u64)disp_pow(10, SCALEUP_9));
	}

	return 0;
}
bool is_valid_version(int version)
{
	if ((version >= IRC_INTERPOLATION_V1) && (version < IRC_INTERPOLATION_MAX))
		return true;
	else
		return false;
}

u64 get_scaleup_luminance(struct brightness_table *brt_tbl, struct panel_irc_info *info, int brightness)
{
	int upper_idx, lower_idx;
	u64 upper_lum, lower_lum, current_lum;
	u64 upper_brt, lower_brt;

	upper_idx = search_tbl(brt_tbl->brt, brt_tbl->sz_lum, SEARCH_TYPE_UPPER, brightness);
	lower_idx = max(0, (upper_idx - 1));
	upper_lum = brt_tbl->lum[upper_idx];
	lower_lum = brt_tbl->lum[lower_idx];
	upper_brt = brt_tbl->brt[upper_idx];
	lower_brt = brt_tbl->brt[lower_idx];

	current_lum = interpolation(lower_lum * disp_pow(10, SCALEUP_4), upper_lum * disp_pow(10, SCALEUP_4),
		(s32)((u64)brightness - lower_brt), (s32)(upper_brt - lower_brt));
	current_lum = disp_pow_round(current_lum, 2);
	return current_lum;
}

int generate_irc(struct brightness_table *brt_tbl, struct panel_irc_info *info, int brightness)
{
	u64 current_lum = 0;

	int(*gen_irc[IRC_INTERPOLATION_MAX])(struct brightness_table*, struct panel_irc_info*, u64, int) = {
		__generate_irc_v1,
		__generate_irc_v2
	};

	if (info->ref_tbl == NULL) {
		pr_info("%s ref_tbl is NULL\n", __func__);
		return -EINVAL;
	}

	if (is_valid_version(info->irc_version)) {
		current_lum = get_scaleup_luminance(brt_tbl, info, brightness);
		memcpy(info->buffer, info->ref_tbl, info->total_len);
		gen_irc[info->irc_version](brt_tbl, info, current_lum, brightness);
	} else {
		pr_info("unsupport irc interpolation %d", info->irc_version);
	}

	return 0;
}

