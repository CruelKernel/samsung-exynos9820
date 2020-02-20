/*
 * linux/drivers/video/fbdev/exynos/panel/s6e3fa7/s6e3fa7.c
 *
 * S6E3FA7 Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of_gpio.h>
#include <video/mipi_display.h>
/* TODO : remove dsim dependent code */
#if defined(CONFIG_EXYNOS_DPU20)
#include "../../dpu20/decon.h"
#else
#include "../../dpu_9810/decon.h"
#endif
#include "../panel.h"
#include "s6e3fa7.h"
#include "s6e3fa7_panel.h"
#ifdef CONFIG_PANEL_AID_DIMMING
#include "../dimming.h"
#include "../panel_dimming.h"
#endif
#include "../panel_drv.h"

#ifdef CONFIG_PANEL_AID_DIMMING
static int gamma_ctoi(s32 (*dst)[MAX_COLOR], u8 *src, int nr_tp)
{
	unsigned int i, c, pos = 2;

	for (i = nr_tp - 1; i > 0; i--) {
		for_each_color(c) {
			if (i == nr_tp - 1)
				dst[i][c] = (((src[0] >> (MAX_COLOR - c - 1)) & 0x01) << 8) | src[pos++];
			else
				dst[i][c] = src[pos++];
		}
	}

	dst[0][RED] = (src[0] >> 4) & 0xF;
	dst[0][GREEN] = (src[1] >> 4) & 0xF;
	dst[0][BLUE] = src[1] & 0xF;

	return 0;
}

static int gamma_ctoi_with_dummy(s32 (*dst)[MAX_COLOR], u8 *src, int nr_tp)
{
	unsigned int i, c, pos = 2;

	for (i = nr_tp - 1; i > 0; i--) {
		for_each_color(c) {
			if (i == nr_tp - 1) {
				dst[i][c] = (((src[0] >> (MAX_COLOR - c - 1)) & 0x01) << 8) | src[pos++];
				if (c == GREEN)
					pos++;
			} else {
				dst[i][c] = src[pos++];
			}
		}
	}

	dst[0][RED] = (src[0] >> 4) & 0xF;
	dst[0][GREEN] = (src[1] >> 4) & 0xF;
	dst[0][BLUE] = src[1] & 0xF;

	return 0;
}

static void mtp_ctoi(s32 (*dst)[MAX_COLOR], u8 *src, int nr_tp)
{
	int v, c, sign;
	int signed_bit[S6E3FA7_NR_TP] = {
		-1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8
	};
	int value_mask[S6E3FA7_NR_TP] = {
		0xF, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xFF
	};
	int value;

	gamma_ctoi(dst, src, nr_tp);
	for (v = 0; v < nr_tp; v++) {
		for_each_color(c) {
			sign = (signed_bit[v] < 0) ? 1 :
				(dst[v][c] & (0x1 << signed_bit[v])) ? -1 : 1;
			value = dst[v][c] & value_mask[v];
			dst[v][c] = sign * value;
		}
	}
}

static void copy_tpout_center(u8 *output, u32 value, u32 v, u32 color)
{
	int index = (S6E3FA7_NR_TP - v - 1);

	if (v == S6E3FA7_NR_TP - 1) {
		output[0] &= ~(0x1 << (MAX_COLOR - color - 1));
		output[0] |= ((value >> 8) & 0x01) << (MAX_COLOR - color - 1);
		if (color != BLUE)
			output[color + 2] = value & 0x00FF;
		else
			output[color + 3] = value & 0x00FF;
		if (value > 511)
			pr_err("%s, error : exceed output range tp:%d c:%d value:%d!!\n",
					__func__, index, color, value);
	} else if (v == 0) {
		if (color == RED) {
			output[0] &= ~(0xF << 4);
			output[0] |= (value & 0xF) << 4;
		} else if (color == GREEN) {
			output[1] &= ~(0xF << 4);
			output[1] |= (value & 0xF) << 4;
		} else if (color == BLUE) {
			output[1] &= ~0xF;
			output[1] |= (value & 0xF);
		}
		if (value > 15)
			pr_err("%s, error : exceed output range tp:%d c:%d value:%d!!\n",
					__func__, index, color, value);
	} else {
		output[index * MAX_COLOR + color + 3] = value & 0x00FF;
		if (value > 255)
			pr_err("%s, error : exceed output range tp:%d c:%d value:%d!!\n",
					__func__, index, color, value);
	}
}

static void gamma_itoc(u8 *dst, s32(*src)[MAX_COLOR], int nr_tp)
{
	int v, c;

	for (v = 0; v < nr_tp; v++) {
		for_each_color(c) {
			copy_tpout_center(dst, src[v][c], v, c);
		}
	}
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static int copy_from_dim_flash(struct panel_info *panel_data,
		u8 *dst, enum dim_flash_items item, int row, int col, int size)
{
	static struct resinfo *res[MAX_DIM_FLASH];
	char *name = s6e3fa7_dim_flash_info[item].name;
	int nrow = s6e3fa7_dim_flash_info[item].nrow;
	int ncol = s6e3fa7_dim_flash_info[item].ncol;

	if (!res[item])
		res[item] = find_panel_resource(panel_data, name);

	if (unlikely(!res[item])) {
		pr_warn("%s resource(%s) not found\n", __func__, name);
		return -EIO;
	}

	memcpy(dst, &res[item]->data[(nrow - row - 1) * ncol + col], size);
	return 0;
}
#endif

static void print_gamma_table(struct panel_info *panel_data, int id)
{
	int i, j, len, luminance;
	char strbuf[1024];
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct maptbl *gamma_maptbl = NULL;
	struct brightness_table *brt_tbl;

#ifndef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		return;
#endif

	brt_tbl = &panel_bl->subdev[id].brt_tbl;
#ifdef CONFIG_SUPPORT_HMD
	gamma_maptbl = find_panel_maptbl_by_index(panel_data,
			(id == PANEL_BL_SUBDEV_TYPE_HMD) ? HMD_GAMMA_MAPTBL : GAMMA_MAPTBL);
#else
	gamma_maptbl = find_panel_maptbl_by_index(panel_data, GAMMA_MAPTBL);
#endif
	if (unlikely(!gamma_maptbl))
		return;

	for (i = 0; i < brt_tbl->sz_lum; i++) {
		luminance = brt_tbl->lum[i];
		len = snprintf(strbuf, 1024, "gamma[%3d] : ", luminance);
		for (j = 0; j < S6E3FA7_GAMMA_CMD_CNT - 1; j++)
			len += snprintf(strbuf + len, max(1024 - len, 0),
					"%02X ", gamma_maptbl->arr[i * sizeof_row(gamma_maptbl) + j]);
		pr_info("%s\n", strbuf);
	}
}

static int generate_hbm_gamma_table(struct panel_info *panel_data, int id)
{
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_dimming_info *panel_dim_info;
	struct maptbl *gamma_maptbl = NULL;
	struct resinfo *hbm_gamma_res;
	struct brightness_table *brt_tbl;
	s32 (*out_gamma_tbl)[MAX_COLOR] = NULL;
	s32 (*nor_gamma_tbl)[MAX_COLOR] = NULL;
	s32 (*hbm_gamma_tbl)[MAX_COLOR] = NULL;
	int luminance, nr_luminance, nr_hbm_luminance, nr_extend_hbm_luminance;
	int i, nr_tp, ret = 0;

	panel_dim_info = panel_data->panel_dim_info[id];
	brt_tbl = &panel_bl->subdev[id].brt_tbl;
	nr_tp = panel_dim_info->dim_init_info.nr_tp;
	nr_luminance = panel_dim_info->nr_luminance;
	nr_hbm_luminance = panel_dim_info->nr_hbm_luminance;
	nr_extend_hbm_luminance = panel_dim_info->nr_extend_hbm_luminance;

	hbm_gamma_res = find_panel_resource(panel_data, "hbm_gamma");
	if (unlikely(!hbm_gamma_res)) {
		pr_warn("%s panel_bl-%d hbm_gamma not found\n", __func__, id);
		return -EIO;
	}
	out_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	nor_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	hbm_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);

#ifdef CONFIG_SUPPORT_HMD
	gamma_maptbl = find_panel_maptbl_by_index(panel_data,
			(id == PANEL_BL_SUBDEV_TYPE_HMD) ? HMD_GAMMA_MAPTBL : GAMMA_MAPTBL);
#else
	gamma_maptbl = find_panel_maptbl_by_index(panel_data, GAMMA_MAPTBL);
#endif
	if (unlikely(!gamma_maptbl)) {
		pr_err("%s panel_bl-%d gamma_maptbl not found\n", __func__, id);
		ret = -EINVAL;
		goto err;
	}

	gamma_ctoi_with_dummy(nor_gamma_tbl, &gamma_maptbl->arr[(nr_luminance - 1) *
			sizeof_row(gamma_maptbl)], nr_tp);
	gamma_ctoi(hbm_gamma_tbl, hbm_gamma_res->data, nr_tp);
	for (i = nr_luminance; i < nr_luminance +
			nr_hbm_luminance + nr_extend_hbm_luminance; i++) {
		luminance = brt_tbl->lum[i];
		if (i < nr_luminance + nr_hbm_luminance)
			/* UI MAX BRIGHTNESS ~ HBM MAX BRIGHTNESS */
			gamma_table_interpolation(nor_gamma_tbl, hbm_gamma_tbl, out_gamma_tbl,
					nr_tp, luminance - panel_dim_info->target_luminance,
					panel_dim_info->hbm_target_luminance - panel_dim_info->target_luminance);
		else
			/* UI MAX BRIGHTNESS ~ EXTENDED HBM MAX BRIGHTNESS */
			memcpy(out_gamma_tbl, hbm_gamma_tbl, sizeof(s32) * nr_tp * MAX_COLOR);

		gamma_itoc((u8 *)&gamma_maptbl->arr[i * sizeof_row(gamma_maptbl)],
				out_gamma_tbl, nr_tp);
	}

err:
	kfree(out_gamma_tbl);
	kfree(nor_gamma_tbl);
	kfree(hbm_gamma_tbl);

	return ret;
}

static int generate_gamma_table_using_lut(struct panel_info *panel_data, int id)
{
	struct panel_dimming_info *panel_dim_info;
	struct dimming_info *dim_info;
	struct maptbl *gamma_maptbl = NULL;
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct brightness_table *brt_tbl;
	struct resinfo *res;
	s32 (*mtp)[MAX_COLOR] = NULL;
	int i, ret, nr_tp;
	u32 nr_luminance;

#ifndef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		return -EINVAL;
#endif

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s panel_bl-%d invalid id\n", __func__, id);
		return -EINVAL;
	}

	brt_tbl = &panel_bl->subdev[id].brt_tbl;
	panel_dim_info = panel_data->panel_dim_info[id];
	nr_tp = panel_dim_info->dim_init_info.nr_tp;
	nr_luminance = panel_dim_info->nr_luminance;

#ifdef CONFIG_SUPPORT_PANEL_SWAP
	if (panel_data->dim_info[id]) {
		kfree(panel_data->dim_info[id]);
		panel_data->dim_info[id] = NULL;
		pr_info("%s panel_bl-%d free dimming info\n", __func__, id);
	}
#endif

	dim_info = kzalloc(sizeof(struct dimming_info), GFP_KERNEL);
	if (unlikely(!dim_info)) {
		pr_err("%s panel_bl-%d failed to alloc dimming_info\n", __func__, id);
		return -ENOMEM;
	}

	ret = init_dimming_info(dim_info, &panel_dim_info->dim_init_info);
	if (unlikely(ret)) {
		pr_err("%s panel_bl-%d failed to init_dimming_info\n", __func__, id);
		ret = -EINVAL;
		goto err;
	}

	if (panel_dim_info->dimming_maptbl) {
		gamma_maptbl = &panel_dim_info->dimming_maptbl[DIMMING_GAMMA_MAPTBL];
	} else {
#ifdef CONFIG_SUPPORT_HMD
		gamma_maptbl = find_panel_maptbl_by_index(panel_data,
				(id == PANEL_BL_SUBDEV_TYPE_HMD) ?
				HMD_GAMMA_MAPTBL : GAMMA_MAPTBL);
#else
		gamma_maptbl = find_panel_maptbl_by_index(panel_data, GAMMA_MAPTBL);
#endif
	}
	if (unlikely(!gamma_maptbl)) {
		pr_err("%s panel_bl-%d gamma_maptbl not found\n", __func__, id);
		ret = -EINVAL;
		goto err;
	}

	/* GET MTP OFFSET */
	res = find_panel_resource(panel_data, "mtp");
	if (unlikely(!res)) {
		pr_warn("%s panel_bl-%d resource(mtp) not found\n", __func__, id);
		ret = -EINVAL;
		goto err;
	}

	mtp = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	mtp_ctoi(mtp, res->data, nr_tp);
	init_dimming_mtp(dim_info, mtp);
	process_dimming(dim_info);

	for (i = 0; i < nr_luminance; i++)
		get_dimming_gamma(dim_info, brt_tbl->lum[i],
				(u8 *)&gamma_maptbl->arr[i * sizeof_row(gamma_maptbl)],
				copy_tpout_center);

	panel_data->dim_info[id] = dim_info;

	pr_info("%s panel_bl-%d done\n", __func__, id);
	kfree(mtp);

	return 0;

err:
	kfree(dim_info);
	return ret;
}

#if 0
void show_brt_step(struct brightness_table *brt_tbl)
{
	int i;
	char buf[1024];
	int len = 0;
	if (unlikely(!brt_tbl || !brt_tbl->brt_to_step)) {
		pr_err("%s, invalid parameter\n", __func__);
		return ;
	}
	for(i = 1; i <= brt_tbl->sz_brt_to_step; i++) {
		len += snprintf(buf + len, sizeof(buf) - len, "%d ", brt_tbl->brt_to_step[i - 1]);
		if((i != 0) && (i % 10 == 0)) {
			pr_info("%s %d %s\n", __func__, brt_tbl->sz_brt_to_step, buf);
			len = 0;
		}
	}
	pr_info("%s %d %s\n", __func__, brt_tbl->sz_brt_to_step, buf);
}
#endif

static int generate_brt_step_table(struct brightness_table *brt_tbl)
{
	int ret = 0;
	int i = 0, j = 0, k = 0;

	if (unlikely(!brt_tbl || !brt_tbl->brt || !brt_tbl->lum)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}
	if (unlikely(!brt_tbl->step_cnt)) {
		if(likely(brt_tbl->brt_to_step)) {
			pr_info("%s we use static step table\n", __func__);
			return ret;
		} else {
			pr_err("%s, invalid parameter, all table is NULL\n", __func__);
			return -EINVAL;
		}
	}

	brt_tbl->sz_brt_to_step = 0;
	for(i = 0; i < brt_tbl->sz_step_cnt; i++)
		brt_tbl->sz_brt_to_step += brt_tbl->step_cnt[i];

	brt_tbl->brt_to_step = (u32 *)kmalloc(brt_tbl->sz_brt_to_step * sizeof(u32), GFP_KERNEL);

	if (unlikely(!brt_tbl->brt_to_step)) {
		pr_err("%s, alloc fail\n", __func__);
		return -EINVAL;
	}
	brt_tbl->brt_to_step[0] = brt_tbl->brt[0];
	i = 1;
	while (i < brt_tbl->sz_brt_to_step) {
		for (k = 1; k < brt_tbl->sz_brt; k++) {
			for (j = 1; j <= brt_tbl->step_cnt[k]; j++, i++) {
				brt_tbl->brt_to_step[i] = interpolation(brt_tbl->brt[k - 1] * disp_pow(10, 2),
					brt_tbl->brt[k] * disp_pow(10, 2), j, brt_tbl->step_cnt[k]);
				brt_tbl->brt_to_step[i] = disp_pow_round(brt_tbl->brt_to_step[i], 2);
				brt_tbl->brt_to_step[i] = disp_div64(brt_tbl->brt_to_step[i], disp_pow(10, 2));
				if(brt_tbl->brt[brt_tbl->sz_ui_lum - 1] < brt_tbl->brt_to_step[i]) {
					brt_tbl->brt_to_step[i] = disp_pow_round(brt_tbl->brt_to_step[i], 2);
				}
				if(i >= brt_tbl->sz_brt_to_step) {
					pr_err("%s step cnt over %d %d\n", __func__, i, brt_tbl->sz_brt_to_step);
					break;
				}
			}
		}
	}
	return ret;
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static int adjust_gamma_table(struct panel_info *panel_data, int id)
{
	struct panel_dimming_info *panel_dim_info;
	struct maptbl *tbl = NULL;
	s32 (*out_gamma_tbl)[MAX_COLOR] = NULL;
	s32 (*nor_gamma_tbl)[MAX_COLOR] = NULL;
	s32 (*ofs_gamma_tbl)[MAX_COLOR] = NULL;
	struct tp *tp;
	int nr_luminance;
	int i, nr_tp, ret = 0;

	panel_dim_info = panel_data->panel_dim_info[id];
	tp = panel_dim_info->dim_init_info.tp;
	nr_tp = panel_dim_info->dim_init_info.nr_tp;
	nr_luminance = panel_dim_info->nr_luminance;

	out_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	nor_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);

#ifdef CONFIG_SUPPORT_HMD
	tbl = find_panel_maptbl_by_index(panel_data,
			(id == PANEL_BL_SUBDEV_TYPE_HMD) ? HMD_GAMMA_MAPTBL : GAMMA_MAPTBL);
#else
	tbl = find_panel_maptbl_by_index(panel_data, GAMMA_MAPTBL);
#endif

	if (unlikely(!tbl)) {
		pr_err("%s panel_bl-%d gamma_maptbl not found\n", __func__, id);
		ret = -EINVAL;
		goto err;
	}

	for (i = 0; i < nr_luminance; i++) {
		ofs_gamma_tbl = (s32 (*)[MAX_COLOR])&panel_dim_info->dim_flash_gamma_offset[i * nr_tp * MAX_COLOR];
		gamma_ctoi_with_dummy(nor_gamma_tbl, &tbl->arr[i *
				sizeof_row(tbl)], nr_tp);
		gamma_table_add_offset(nor_gamma_tbl, ofs_gamma_tbl, out_gamma_tbl, tp, nr_tp);
		gamma_itoc((u8 *)&tbl->arr[i * sizeof_row(tbl)],
				out_gamma_tbl, nr_tp);
	}

	pr_info("%s done\n", __func__);

err:
	kfree(out_gamma_tbl);
	kfree(nor_gamma_tbl);

	return ret;
}

static int generate_gamma_table_using_flash(struct panel_info *panel_data, int id)
{
	struct maptbl *tbl = NULL;
	int i, ret;
	u32 nr_luminance;

#ifndef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		return -EINVAL;
#endif

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s panel_bl-%d invalid id\n", __func__, id);
		return -EINVAL;
	}

	nr_luminance = panel_data->panel_dim_info[id]->nr_luminance;
#ifdef CONFIG_SUPPORT_HMD
	tbl = find_panel_maptbl_by_index(panel_data,
			(id == PANEL_BL_SUBDEV_TYPE_HMD) ? HMD_GAMMA_MAPTBL : GAMMA_MAPTBL);
#else
	tbl = find_panel_maptbl_by_index(panel_data, GAMMA_MAPTBL);
#endif
	if (unlikely(!tbl)) {
		pr_err("%s panel_bl-%d gamma_maptbl not found\n", __func__, id);
		return -EINVAL;
	}

	for (i = 0; i < nr_luminance; i++)
		ret = copy_from_dim_flash(panel_data,
				tbl->arr + maptbl_index(tbl, 0, i, 0),
				(id == PANEL_BL_SUBDEV_TYPE_HMD) ?
				DIM_FLASH_HMD_GAMMA : DIM_FLASH_GAMMA,
				i, 0, sizeof_row(tbl));

	if (panel_data->panel_dim_info[id]->dim_flash_gamma_offset)
		ret = adjust_gamma_table(panel_data, id);

	pr_info("%s panel_bl-%d done\n", __func__, id);

	return 0;
}

static int init_subdev_gamma_table_using_flash(struct maptbl *tbl, int id)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	struct panel_dimming_info *panel_dim_info;
	int ret;

	if (unlikely(!tbl || !tbl->pdata)) {
		panel_err("%s panel_bl-%d invalid param (tbl %p, pdata %p)\n",
				__func__, id, tbl, tbl ? tbl->pdata : NULL);
		return -EINVAL;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s panel_bl-%d invalid id\n", __func__, id);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;
	panel_dim_info = panel_data->panel_dim_info[id];
	if (unlikely(!panel_dim_info)) {
		pr_err("%s panel_bl-%d panel_dim_info is null\n", __func__, id);
		return -EINVAL;
	}
	if(id == PANEL_BL_SUBDEV_TYPE_DISP)
		generate_brt_step_table(panel_dim_info->brt_tbl);

	/* initialize brightness_table */
	memcpy(&panel->panel_bl.subdev[id].brt_tbl,
			panel_dim_info->brt_tbl, sizeof(struct brightness_table));

	if (!panel_resource_initialized(panel_data,
			(id == PANEL_BL_SUBDEV_TYPE_HMD) ?
			s6e3fa7_dim_flash_info[DIM_FLASH_HMD_GAMMA].name :
			s6e3fa7_dim_flash_info[DIM_FLASH_GAMMA].name)) {
		pr_warn("%s resource(%s) not initialized\n", __func__,
				(id == PANEL_BL_SUBDEV_TYPE_HMD) ?
				s6e3fa7_dim_flash_info[DIM_FLASH_HMD_GAMMA].name :
				s6e3fa7_dim_flash_info[DIM_FLASH_GAMMA].name);
		return -EINVAL;
	}

	ret = generate_gamma_table_using_flash(panel_data, id);
	if (ret < 0) {
		pr_err("%s failed to generate gamma using flash\n", __func__);
		return ret;
	}

	if (id == PANEL_BL_SUBDEV_TYPE_DISP) {
		ret = generate_hbm_gamma_table(panel_data, id);
		if (ret < 0) {
			pr_err("%s failed to generate hbm gamma\n", __func__);
			return ret;
		}
	}

	panel_info("%s panel_bl-%d gamma_table initialized\n", __func__, id);
	print_gamma_table(panel_data, id);

	return 0;
}
#endif /* CONFIG_SUPPORT_DIM_FLASH */

static int init_subdev_gamma_table_using_lut(struct maptbl *tbl, int id)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	struct panel_dimming_info *panel_dim_info;
	int ret;

	if (unlikely(!tbl || !tbl->pdata)) {
		panel_err("%s panel_bl-%d invalid param (tbl %p, pdata %p)\n",
				__func__, id, tbl, tbl ? tbl->pdata : NULL);
		return -EINVAL;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s panel_bl-%d invalid id\n", __func__, id);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;
	panel_dim_info = panel_data->panel_dim_info[id];
	if (unlikely(!panel_dim_info)) {
		pr_err("%s panel_bl-%d panel_dim_info is null\n", __func__, id);
		return -EINVAL;
	}
	if(id == PANEL_BL_SUBDEV_TYPE_DISP)
		generate_brt_step_table(panel_dim_info->brt_tbl);

	/* initialize brightness_table */
	memcpy(&panel->panel_bl.subdev[id].brt_tbl,
			panel_dim_info->brt_tbl, sizeof(struct brightness_table));

	ret = generate_gamma_table_using_lut(panel_data, id);
	if (ret < 0) {
		pr_err("%s failed to generate gamma using lut\n", __func__);
		return ret;
	}

	if (panel_dim_info->dimming_maptbl)
		maptbl_memcpy(tbl,
				&panel_dim_info->dimming_maptbl[DIMMING_GAMMA_MAPTBL]);

	if (id == PANEL_BL_SUBDEV_TYPE_DISP) {
		ret = generate_hbm_gamma_table(panel_data, id);
		if (ret < 0) {
			pr_err("%s failed to generate hbm gamma\n", __func__);
			return ret;
		}
	}

	panel_info("%s panel_bl-%d gamma_table initialized\n", __func__, id);
	print_gamma_table(panel_data, id);

	return 0;
}

#else
static int init_subdev_gamma_table_using_lut(struct maptbl *tbl, int id)
{
	panel_err("%s aid dimming unspported\n", __func__);
	return -ENODEV;
}
#endif /* CONFIG_PANEL_AID_DIMMING */

int init_common_table(struct maptbl *tbl)
{
	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int init_gamma_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_subdev_gamma_table_using_flash(tbl, PANEL_BL_SUBDEV_TYPE_DISP);
	else
		return init_subdev_gamma_table_using_lut(tbl, PANEL_BL_SUBDEV_TYPE_DISP);
#else
	return init_subdev_gamma_table_using_lut(tbl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
}

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
static int getidx_common_maptbl(struct maptbl *tbl)
{
	return 0;
}
#endif

static int getidx_dimming_maptbl(struct maptbl *tbl)
{
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	int index;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_bl = &panel->panel_bl;
	index = get_actual_brightness_index(panel_bl,
			panel_bl->props.brightness);

	return maptbl_index(tbl, 0, index, 0);
}

#ifdef CONFIG_SUPPORT_HMD
static int init_hmd_gamma_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_subdev_gamma_table_using_flash(tbl, PANEL_BL_SUBDEV_TYPE_HMD);
	else
		return init_subdev_gamma_table_using_lut(tbl, PANEL_BL_SUBDEV_TYPE_HMD);
#else
	return init_subdev_gamma_table_using_lut(tbl, PANEL_BL_SUBDEV_TYPE_HMD);
#endif
}

static int getidx_hmd_dimming_mtptbl(struct maptbl *tbl)
{
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	int index = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_bl = &panel->panel_bl;
	index = get_subdev_actual_brightness_index(panel_bl,
			PANEL_BL_SUBDEV_TYPE_HMD,
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_HMD].brightness);

	return maptbl_index(tbl, 0, index, 0);
}
#endif /* CONFIG_SUPPORT_HMD */

#ifdef CONFIG_SUPPORT_DOZE
#ifdef CONFIG_SUPPORT_AOD_BL
static int init_aod_dimming_table(struct maptbl *tbl)
{
	int id = PANEL_BL_SUBDEV_TYPE_AOD;
	struct panel_device *panel;
	struct panel_bl_device *panel_bl;

	if (unlikely(!tbl || !tbl->pdata)) {
		panel_err("%s panel_bl-%d invalid param (tbl %p, pdata %p)\n",
				__func__, id, tbl, tbl ? tbl->pdata : NULL);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_bl = &panel->panel_bl;

	if (unlikely(!panel->panel_data.panel_dim_info[id])) {
		pr_err("%s panel_bl-%d panel_dim_info is null\n", __func__, id);
		return -EINVAL;
	}

	memcpy(&panel_bl->subdev[id].brt_tbl,
			panel->panel_data.panel_dim_info[id]->brt_tbl,
			sizeof(struct brightness_table));

	return 0;
}
#endif
#endif

#if 0
#if (PANEL_BACKLIGHT_PAC_STEPS == 512 || PANEL_BACKLIGHT_PAC_STEPS == 256)
static int getidx_brt_tbl(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct panel_bl_device *panel_bl;
	int index;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	index = get_brightness_pac_step(panel_bl, panel_bl->props.brightness);

	if (index < 0) {
		panel_err("PANEL:ERR:%s:invalid brightness %d, index %d\n",
				__func__, panel_bl->props.brightness, index);
		return -EINVAL;
	}

	if (index > tbl->nrow - 1)
		index = tbl->nrow - 1;

	return maptbl_index(tbl, 0, index, 0);
}

static int getidx_aor_table(struct maptbl *tbl)
{
	return getidx_brt_tbl(tbl);
}

static int getidx_irc_table(struct maptbl *tbl)
{
	return getidx_brt_tbl(tbl);
}
#endif
#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
static int getidx_dynamic_ffc_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct panel_info *panel_data;
	struct adaptive_idx *adap_idx;
	int row = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (tbl == NULL)
		return -EINVAL;

	panel_data = &panel->panel_data;
	adap_idx = &panel->adap_idx;

	if (adap_idx->cur_freq_idx >= 0 ||
		adap_idx->cur_freq_idx < tbl->nrow) {
		row = adap_idx->cur_freq_idx;
	}
	panel->panel_data.props.cur_ffc_idx = row;
	panel_info("[ADAP_FREQ] FFC:index:%d\n", row);

	return maptbl_index(tbl, 0, row, 0);
}

static void copy_dynamic_ffc_maptbl(struct maptbl *tbl, u8 *dst)
{
	copy_common_maptbl(tbl, dst);
	panel_info("[ADAP_FREQ] FFC:[0]:0x%02x, [1]:0x%02x, [2]:0x%02x, [3]:0x%02x, [4]:0x%02x, [5]:0x%02x\n",
		dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
}
#endif
static int generate_hbm_gamma(struct panel_info *panel_data,
		struct maptbl *gamma_maptbl, u8 *dst)
{
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_dimming_info *panel_dim_info;
	struct resinfo *hbm_gamma_res;
	struct brightness_table *brt_tbl;
	s32 (*out_gamma_tbl)[MAX_COLOR] = NULL;
	s32 (*nor_gamma_tbl)[MAX_COLOR] = NULL;
	s32 (*hbm_gamma_tbl)[MAX_COLOR] = NULL;
	int luminance, nr_luminance, nr_hbm_luminance, nr_extend_hbm_luminance;
	int id, nr_tp, brightness, ret = 0;
	int upper_idx, lower_idx, nor_max_idx, hbm_max_idx;
	u32 upper_lum, lower_lum, nor_max_lum, hbm_max_lum;
	u32 upper_brt, lower_brt, nor_max_brt, hbm_max_brt;

	id = panel_bl->props.id;
	panel_dim_info = panel_data->panel_dim_info[id];
	brt_tbl = &panel_bl->subdev[id].brt_tbl;
	brightness = panel_bl->props.brightness_of_step;

	nr_tp = panel_dim_info->dim_init_info.nr_tp;
	nr_luminance = panel_dim_info->nr_luminance;
	nr_hbm_luminance = panel_dim_info->nr_hbm_luminance;
	nr_extend_hbm_luminance = panel_dim_info->nr_extend_hbm_luminance;

	upper_idx = panel_bl->props.actual_brightness_index;
	lower_idx = max(0, (upper_idx - 1));
	upper_lum = brt_tbl->lum[upper_idx];
	lower_lum = brt_tbl->lum[lower_idx];
	upper_brt = brt_tbl->brt[upper_idx];
	lower_brt = brt_tbl->brt[lower_idx];

	hbm_max_idx = nr_luminance + nr_hbm_luminance - 1;
	nor_max_idx = nr_luminance - 1;
	hbm_max_lum = brt_tbl->lum[hbm_max_idx];
	nor_max_lum = brt_tbl->lum[nor_max_idx];
	hbm_max_brt = brt_tbl->brt[hbm_max_idx];
	nor_max_brt = brt_tbl->brt[nor_max_idx];

	hbm_gamma_res = find_panel_resource(panel_data, "hbm_gamma");
	if (unlikely(!hbm_gamma_res)) {
		pr_warn("%s panel_bl-%d hbm_gamma not found\n", __func__, id);
		return -EIO;
	}
	out_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	nor_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);
	hbm_gamma_tbl = kzalloc(sizeof(s32) * nr_tp * MAX_COLOR, GFP_KERNEL);

	gamma_ctoi_with_dummy(nor_gamma_tbl, &gamma_maptbl->arr[(nr_luminance - 1) *
			sizeof_row(gamma_maptbl)], nr_tp);
	gamma_ctoi(hbm_gamma_tbl, hbm_gamma_res->data, nr_tp);

	luminance = interpolation(lower_lum, upper_lum,
			(s32)((u64)brightness - lower_brt), (s32)(upper_brt - lower_brt));

	if (luminance <= panel_dim_info->hbm_target_luminance)
		/* UI MAX BRIGHTNESS ~ HBM MAX BRIGHTNESS */
		gamma_table_interpolation(nor_gamma_tbl, hbm_gamma_tbl, out_gamma_tbl,
				nr_tp, luminance - nor_max_lum, hbm_max_lum - nor_max_lum);
	else
		/* UI MAX BRIGHTNESS ~ EXTENDED HBM MAX BRIGHTNESS */
		memcpy(out_gamma_tbl, hbm_gamma_tbl, sizeof(s32) * nr_tp * MAX_COLOR);

	gamma_itoc(dst, out_gamma_tbl, nr_tp);

	kfree(out_gamma_tbl);
	kfree(nor_gamma_tbl);
	kfree(hbm_gamma_tbl);

	return ret;
}

static void copy_gamma_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct panel_bl_device *panel_bl;
	int id, brightness, ret;

	if (!tbl || !dst) {
		pr_err("%s, invalid parameter (tbl %p, dst %p\n",
				__func__, tbl, dst);
		return;
	}

	panel = (struct panel_device *)tbl->pdata;
	panel_bl = &panel->panel_bl;
	id = panel_bl->props.id;
	brightness = panel_bl->props.brightness;

	if (get_actual_brightness(panel_bl, brightness)
			<= S6E3FA7_TARGET_LUMINANCE) {
		copy_common_maptbl(tbl, dst);
		return;
	}

	ret = generate_hbm_gamma(&panel->panel_data, tbl, dst);
	if (ret < 0) {
		pr_err("%s, invalid gamma (ret %d)\n", __func__, ret);
		return;
	}
}

static void copy_aor_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct panel_bl_device *panel_bl;
	struct brightness_table *brt_tbl;
	int id, aor, brightness;

	if (!tbl || !dst) {
		pr_err("%s, invalid parameter (tbl %p, dst %p\n",
				__func__, tbl, dst);
		return;
	}

	panel = (struct panel_device *)tbl->pdata;
	panel_bl = &panel->panel_bl;
	id = panel_bl->props.id;
	brightness = panel_bl->props.brightness;
	brt_tbl = &panel_bl->subdev[id].brt_tbl;

	if (get_actual_brightness(panel_bl, brightness)
			> S6E3FA7_TARGET_LUMINANCE) {
		copy_common_maptbl(tbl, dst);
		aor = dst[0] << 8 | dst[1];
		panel_bl->props.aor_ratio = AOR_TO_RATIO(aor, brt_tbl->vtotal);
		return;
	}

	aor = panel_bl_aor_interpolation(panel_bl, id, (u8 (*)[2])tbl->arr);
	if (aor < 0) {
		pr_err("%s, invalid aor %d\n", __func__, aor);
		return;
	}
	dst[0] = (aor >> 8) & 0xFF;
	dst[1] = aor & 0xFF;

	panel_bl->props.aor_ratio = AOR_TO_RATIO(aor, brt_tbl->vtotal);
}

static void copy_irc_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct panel_bl_device *panel_bl;
	struct brightness_table *brt_tbl;
	struct panel_irc_info *irc_info;
	struct panel_dimming_info *panel_dim_info;
	int id, brightness, ret;
	int size_ui_lum;

	if (!tbl || !dst) {
		pr_err("%s, invalid parameter (tbl %p, dst %p\n",
				__func__, tbl, dst);
		return;
	}

	panel = (struct panel_device *)tbl->pdata;
	panel_bl = &panel->panel_bl;
	id = panel_bl->props.id;
	brightness = panel_bl->props.brightness;
	brt_tbl = &panel_bl->subdev[id].brt_tbl;
	panel_dim_info = panel->panel_data.panel_dim_info[id];
	irc_info = panel_dim_info->irc_info;
	size_ui_lum = (brt_tbl->sz_panel_dim_ui_lum != 0) ?
		brt_tbl->sz_panel_dim_ui_lum : brt_tbl->sz_ui_lum;

	memcpy(irc_info->ref_tbl,
			&tbl->arr[(size_ui_lum - 1) * irc_info->total_len],
			irc_info->total_len);
	ret = panel_bl_irc_interpolation(panel_bl, id, irc_info);
	if (ret < 0) {
		pr_err("%s, invalid irc (ret %d)\n", __func__, ret);
		return;
	}
	memcpy(dst, irc_info->buffer, irc_info->total_len);
}

static int getidx_irc_mode_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	return maptbl_index(tbl, 0, !!panel->panel_data.props.irc_mode, 0);
}

static int getidx_mps_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct panel_bl_device *panel_bl;
	int row;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	row = (get_actual_brightness(panel_bl,
			panel_bl->props.brightness) <= 39) ? 0 : 1;

	return maptbl_index(tbl, 0, row, 0);
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static int init_maptbl_from_table(struct maptbl *tbl, enum dim_flash_items item)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	struct panel_dimming_info *panel_dim_info;
	int index, id;

	if (tbl == NULL || tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;
	id = (item == DIM_FLASH_HMD_GAMMA || item == DIM_FLASH_HMD_AOR) ?
		PANEL_BL_SUBDEV_TYPE_HMD : PANEL_BL_SUBDEV_TYPE_DISP;
	panel_dim_info = panel_data->panel_dim_info[id];

	if (!panel_dim_info->dimming_maptbl) {
		panel_err("PANEL:ERR:%s:dimming_maptbl is null\n", __func__);
		return -EINVAL;
	}

	/* TODO : HMD DIMMING MAPTBL */
	if (item == DIM_FLASH_GAMMA || item == DIM_FLASH_HMD_GAMMA) {
		index = DIMMING_GAMMA_MAPTBL;
	} else if (item == DIM_FLASH_AOR || item == DIM_FLASH_HMD_AOR)
		index = DIMMING_AOR_MAPTBL;
	else if (item == DIM_FLASH_VINT)
		index = DIMMING_VINT_MAPTBL;
	else if (item == DIM_FLASH_ELVSS)
		index = DIMMING_ELVSS_MAPTBL;
	else if (item == DIM_FLASH_IRC)
		index = DIMMING_IRC_MAPTBL;
	else
		return -EINVAL;

	if (!sizeof_maptbl(&panel_dim_info->dimming_maptbl[index]))
		return -EINVAL;

	maptbl_memcpy(tbl, &panel_dim_info->dimming_maptbl[index]);

	pr_info("%s copy from %s to %s, size %d, item:%d, index:%d\n",
			__func__, panel_dim_info->dimming_maptbl[index].name, tbl->name,
			sizeof_maptbl(tbl), item, index);

	//print_maptbl(tbl);

	return 0;
}

static int init_maptbl_from_flash(struct maptbl *tbl, enum dim_flash_items item)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	int layer, row, ret;

	if (tbl == NULL || tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;
	if (!panel_resource_initialized(panel_data,
				s6e3fa7_dim_flash_info[item].name)) {
		pr_warn("%s resource(%s) not initialized\n",
				__func__, s6e3fa7_dim_flash_info[item].name);
		return init_common_table(tbl);
	}

	for_each_layer(tbl, layer) {
		for_each_row(tbl, row) {
			if (row >= s6e3fa7_dim_flash_info[item].nrow)
				continue;
			if (item == DIM_FLASH_ELVSS) {
				ret = copy_from_dim_flash(panel_data,
						tbl->arr + maptbl_index(tbl, layer, row, 0),
						item, row, layer, tbl->sz_copy);
			} else if (item == DIM_FLASH_GAMMA || item == DIM_FLASH_AOR ||
					item == DIM_FLASH_VINT || item == DIM_FLASH_HMD_GAMMA ||
					item == DIM_FLASH_HMD_AOR) {
				ret = copy_from_dim_flash(panel_data,
						tbl->arr + maptbl_index(tbl, layer, row, 0),
						item, row, 0, tbl->sz_copy);
			} else if (item == DIM_FLASH_IRC) {
				ret = copy_from_dim_flash(panel_data,
						tbl->arr + maptbl_index(tbl, layer, row, 0),
						item, row, 0, tbl->sz_copy);
			}
		}
	}


	//print_maptbl(tbl);

	return 0;
}
#endif

static int init_elvss_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_maptbl_from_flash(tbl, DIM_FLASH_ELVSS);
	else
		return init_maptbl_from_table(tbl, DIM_FLASH_ELVSS);

	return 0;
#else
	return init_common_table(tbl);
#endif
}

static int init_vint_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_maptbl_from_flash(tbl, DIM_FLASH_VINT);
	else
		return init_maptbl_from_table(tbl, DIM_FLASH_VINT);

	return 0;
#else
	return init_common_table(tbl);
#endif
}

static int init_aor_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_maptbl_from_flash(tbl, DIM_FLASH_AOR);
	else
		return init_maptbl_from_table(tbl, DIM_FLASH_AOR);

	return 0;
#else
	return init_common_table(tbl);
#endif
}

static int init_irc_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_maptbl_from_flash(tbl, DIM_FLASH_IRC);
	else
		return init_maptbl_from_table(tbl, DIM_FLASH_IRC);

	return 0;
#else
	return init_common_table(tbl);
#endif
}

#ifdef CONFIG_SUPPORT_HMD
static int init_hmd_aor_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct panel_device *panel = tbl->pdata;
	struct panel_info *panel_data = &panel->panel_data;

	if (panel_data->props.cur_dim_type == DIM_TYPE_DIM_FLASH)
		return init_maptbl_from_flash(tbl, DIM_FLASH_HMD_AOR);
	else
		return init_maptbl_from_table(tbl, DIM_FLASH_HMD_AOR);

	return 0;
#else
	return init_common_table(tbl);
#endif
}
#endif

static int init_elvss_temp_table(struct maptbl *tbl)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	int i, layer, ret;
	u8 elvss_temp_0_otp_value = 0;
	u8 elvss_temp_1_otp_value = 0;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	panel = tbl->pdata;
	panel_data = &panel->panel_data;

	ret = resource_copy_by_name(panel_data, &elvss_temp_0_otp_value, "elvss_temp_0");
	if (unlikely(ret)) {
		pr_err("%s, elvss_temp not found in panel resource\n", __func__);
		return -EINVAL;
	}

	ret = resource_copy_by_name(panel_data, &elvss_temp_1_otp_value, "elvss_temp_1");
	if (unlikely(ret)) {
		pr_err("%s, elvss_temp not found in panel resource\n", __func__);
		return -EINVAL;
	}

	for_each_layer(tbl, layer) {
		for_each_row(tbl, i) {
			tbl->arr[layer * sizeof_layer(tbl) + i] =
			(i < S6E3FA7_NR_LUMINANCE) ?
			elvss_temp_0_otp_value : elvss_temp_1_otp_value;
		}
	}

	return 0;
}

static int getidx_elvss_temp_table(struct maptbl *tbl)
{
	int row, plane;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;

	plane = (UNDER_MINUS_15(panel_data->props.temperature) ? UNDER_MINUS_FIFTEEN :
			(UNDER_0(panel_data->props.temperature) ? UNDER_ZERO : OVER_ZERO));
	row = get_actual_brightness_index(panel_bl, panel_bl->props.brightness);

	return maptbl_index(tbl, plane, row, 0);
}

#ifdef CONFIG_SUPPORT_XTALK_MODE
static int getidx_vgh_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct panel_info *panel_data;
	int row = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;

	row = ((panel_data->props.xtalk_mode) ? 1 : 0);
	panel_info("%s xtalk_mode %d\n", __func__, row);

	return maptbl_index(tbl, 0, row, 0);
}
#endif

static int getidx_acl_onoff_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	return maptbl_index(tbl, 0, panel_bl_get_acl_pwrsave(&panel->panel_bl), 0);
}

static int getidx_hbm_onoff_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct panel_bl_device *panel_bl;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;

	return maptbl_index(tbl, 0,
			is_hbm_brightness(panel_bl, panel_bl->props.brightness), 0);
}

static int getidx_acl_opr_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	return maptbl_index(tbl, 0, panel_bl_get_acl_opr(&panel->panel_bl), 0);
}

static int getidx_resolution_table(struct maptbl *tbl)
{
	int row = 0;
#ifdef CONFIG_SUPPORT_DSU
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct decon_lcd *lcd_info = &panel->lcd_info;

	panel_info("%s : mres_mode : %d\n", __func__, lcd_info->mres_mode);

	switch (lcd_info->mres_mode) {
	case DSU_MODE_1:
		row = 0;
		break;
	case DSU_MODE_2:
		row = 1;
		break;
	case DSU_MODE_3:
		row = 2;
		break;
	default:
		panel_err("PANEL:ERR:%s:Invalid dsu mode : %d\n", __func__, lcd_info->mres_mode);
		row = 0;
		break;
	}
#endif

	return maptbl_index(tbl, 0, row, 0);
}

static int init_lpm_table(struct maptbl *tbl)
{
#ifdef CONFIG_SUPPORT_AOD_BL
	return init_aod_dimming_table(tbl);
#else
	return init_common_table(tbl);
#endif
}

static int getidx_lpm_table(struct maptbl *tbl)
{
	int layer = 0, row = 0;
	struct panel_device *panel;
	struct panel_bl_device *panel_bl;
	struct panel_properties *props;

	panel = (struct panel_device *)tbl->pdata;
	panel_bl = &panel->panel_bl;
	props = &panel->panel_data.props;

#ifdef CONFIG_SUPPORT_DOZE
#ifdef CONFIG_SUPPORT_AOD_BL
	switch (props->alpm_mode) {
	case ALPM_LOW_BR:
	case ALPM_HIGH_BR:
		layer = ALPM_MODE;
		break;
	case HLPM_LOW_BR:
	case HLPM_HIGH_BR:
		layer = HLPM_MODE;
		break;
	default:
		panel_err("PANEL:ERR:%s:Invalid alpm mode : %d\n",
				__func__, props->alpm_mode);
		break;
	}

	panel_bl = &panel->panel_bl;
	row = get_subdev_actual_brightness_index(panel_bl,
			PANEL_BL_SUBDEV_TYPE_AOD,
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_AOD].brightness);

	props->lpm_brightness =
		panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_AOD].brightness;

	pr_info("%s alpm_mode %d, brightness %d, layer %d row %d\n",
			__func__, props->alpm_mode,
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_AOD].brightness,
			layer, row);

#else
	switch (props->alpm_mode) {
	case ALPM_LOW_BR:
		layer = ALPM_MODE;
		row = 0;
		break;
	case HLPM_LOW_BR:
		layer = HLPM_MODE;
		row = 0;
		break;
	case ALPM_HIGH_BR:
		layer = ALPM_MODE;
		row = tbl->nrow - 1;
		break;
	case HLPM_HIGH_BR:
		layer = HLPM_MODE;
		row = tbl->nrow - 1;
		break;
	default:
		panel_err("PANEL:ERR:%s:Invalid alpm mode : %d\n",
				__func__, props->alpm_mode);
		break;
	}

	pr_debug("%s alpm_mode %d, layer %d row %d\n",
			__func__, props->alpm_mode, layer, row);
#endif
#endif
	props->cur_alpm_mode = props->alpm_mode;

	return maptbl_index(tbl, layer, row, 0);
}

#ifdef CONFIG_DYNAMIC_FREQ
static int getidx_dyn_ffc_table(struct maptbl *tbl)
{
	int row = 0;
	struct df_status_info *status;
	struct panel_device *panel = (struct panel_device *)tbl->pdata;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	status = &panel->df_status;

	panel_info("[DYN_FREQ]INFO:%s:ffc idx: %d\n", __func__, status->ffc_df);

	row = status->ffc_df;

	return maptbl_index(tbl, 0, row, 0);
}
#endif


static int getidx_lpm_dyn_vlin_table(struct maptbl *tbl)
{
	int row = 0;
	struct panel_device *panel;
	struct panel_properties *props;

	panel = (struct panel_device *)tbl->pdata;
	props = &panel->panel_data.props;

	if (props->lpm_opr < 250)
		row = 0;
	else
		row = 1;

	props->cur_lpm_opr = props->lpm_opr;

	return maptbl_index(tbl, 0, row, 0);
}

#ifdef CONFIG_SUPPORT_TDMB_TUNE
static int s6e3fa7_getidx_tdmb_tune_table(struct maptbl *tbl)
{
	struct panel_device *panel = (struct panel_device *)tbl->pdata;
	struct panel_properties *props = &panel->panel_data.props;

	return maptbl_index(tbl, 0, props->tdmb_on, 0);
}
#endif

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
static void copy_dummy_maptbl(struct maptbl *tbl, u8 *dst)
{
	return;
}
#endif

static void copy_common_maptbl(struct maptbl *tbl, u8 *dst)
{
	int idx;

	if (!tbl || !dst) {
		pr_err("%s, invalid parameter (tbl %p, dst %p\n",
				__func__, tbl, dst);
		return;
	}

	idx = maptbl_getidx(tbl);
	if (idx < 0) {
		pr_err("%s, failed to getidx %d\n", __func__, idx);
		return;
	}

	memcpy(dst, &(tbl->arr)[idx], sizeof(u8) * tbl->sz_copy);
#ifdef DEBUG_PANEL
	panel_dbg("%s copy from %s %d %d\n",
			__func__, tbl->name, idx, tbl->sz_copy);
	print_data(dst, tbl->sz_copy);
#endif
}

static void copy_tset_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct panel_info *panel_data;

	if (!tbl || !dst)
		return;

	panel = (struct panel_device *)tbl->pdata;
	if (unlikely(!panel))
		return;

	panel_data = &panel->panel_data;

	*dst = (panel_data->props.temperature < 0) ?
		BIT(7) | abs(panel_data->props.temperature) :
		panel_data->props.temperature;
}

static void copy_mcd_resistance_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct panel_info *panel_data;

	if (!tbl || !dst)
		return;

	panel = (struct panel_device *)tbl->pdata;
	if (unlikely(!panel))
		return;

	panel_data = &panel->panel_data;
	*dst = panel_data->props.mcd_resistance;
	pr_debug("%s mcd resistance %02X\n", __func__, *dst);
}

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static void copy_copr_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct copr_info *copr;
	struct copr_reg_v1 *reg;

	if (!tbl || !dst)
		return;

	copr = (struct copr_info *)tbl->pdata;
	if (unlikely(!copr))
		return;

	reg = &copr->props.reg.v1;

	dst[0] = (reg->cnt_re << 2) | (reg->copr_gamma << 1) | reg->copr_en;
	dst[1] = ((reg->copr_er >> 8) & 0x3) << 4 |
		((reg->copr_eg >> 8) & 0x3) << 2 | ((reg->copr_eb >> 8) & 0x3);
	dst[2] = reg->copr_er;
	dst[3] = reg->copr_eg;
	dst[4] = reg->copr_eb;
	dst[5] = (reg->max_cnt >> 8) & 0xFF;
	dst[6] = reg->max_cnt & 0xFF;
	dst[7] = ((reg->roi_on << 3) | ((reg->roi_xs >> 8) & 0x7));
	dst[8] = reg->roi_xs & 0xFF;
	dst[9] = (reg->roi_ys >> 8) & 0xF;
	dst[10] = reg->roi_ys & 0xFF;
	dst[11] = (reg->roi_xe >> 8) & 0x7;
	dst[12] = reg->roi_xe & 0xFF;
	dst[13] = (reg->roi_ye >> 8) & 0xF;
	dst[14] = reg->roi_ye & 0xFF;

#ifdef DEBUG_PANEL
	print_data(dst, 15);
#endif
}
#endif

#ifdef CONFIG_ACTIVE_CLOCK
#define SELF_CLK_ANA_CLOCK_EN	0x10
#define SELF_CLK_BLINK_EN		0x01
#define TIME_UPDATE				0x80

static void copy_self_clk_update_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct act_clk_info *act_info;

	if (!tbl || !dst)
		return;

	panel_info("active clock update\n");

	panel = (struct panel_device *)tbl->pdata;
	if (unlikely(!panel))
		return;

	act_info = &panel->act_clk_dev.act_info;

	dst[1] = 0;

	if (act_info->en) {
		dst[1] = SELF_CLK_ANA_CLOCK_EN;
		panel_info("PANEL:INFO:%s:active clock enable\n", __func__);
		if (act_info->interval == 100) {
			dst[8] = 0x03;
			dst[9] = 0x41;
		} else {
			dst[8] = 0x1E;
			dst[9] = 0x4A;
		}
	}
}
static void copy_self_clk_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct act_clk_info *act_info;
	struct act_blink_info *blink_info;

	if (!tbl || !dst)
		return;

	panel = (struct panel_device *)tbl->pdata;
	if (unlikely(!panel))
		return;

	act_info = &panel->act_clk_dev.act_info;
	blink_info = &panel->act_clk_dev.blink_info;

	dst[1] = 0;

	if (act_info->en) {
		dst[1] = SELF_CLK_ANA_CLOCK_EN;
		panel_info("PANEL:INFO:%s:active clock enable\n", __func__);
		if (act_info->interval == 100) {
			dst[8] = 0x03;
			dst[9] = 0x41;
		} else {
			dst[8] = 0x1E;
			dst[9] = 0x4A;
		}

		dst[15] = act_info->time_hr % 12;
		dst[16] = act_info->time_min % 60;
		dst[17] = act_info->time_sec % 60;
		dst[18] = act_info->time_ms;

		panel_info("PANEL:INFO:%s:active clock %d:%d:%d\n", __func__,
			dst[15], dst[16], dst[17]);

		if (act_info->update_time)
			dst[9] |= TIME_UPDATE;

		dst[19] = (act_info->pos_x >> 8) & 0xff;
		dst[20] = act_info->pos_x & 0xff;

		dst[21] = (act_info->pos_y >> 8) & 0xff;
		dst[22] = act_info->pos_y & 0xff;
		panel_info("PANEL:INFO:%s:active clock pos %d,%d\n", __func__,
			act_info->pos_x, act_info->pos_y);
	}

	if (blink_info->en) {
		dst[1] = SELF_CLK_BLINK_EN;

		dst[32] = blink_info->interval;
		dst[33] = blink_info->radius;
		/* blink color : Red */
		dst[34] = (unsigned char)(blink_info->color >> 16) & 0xff;
		/* blink color : Blue */
		dst[35] = (unsigned char)(blink_info->color >> 8) & 0xff;
		/* blink color : Green */
		dst[36] = (unsigned char)(blink_info->color & 0xff);

		dst[37] = 0x05;

		dst[38] = (blink_info->pos1_x >> 8) & 0xff;
		dst[39] = (blink_info->pos1_x & 0xff);

		dst[40] = (blink_info->pos1_y >> 8) & 0xff;
		dst[41] = (blink_info->pos1_y & 0xff);

		dst[42] = (blink_info->pos2_x >> 8) & 0xff;
		dst[43] = (blink_info->pos2_x & 0xff);

		dst[44] = (blink_info->pos2_y >> 8) & 0xff;
		dst[45] = (blink_info->pos2_y & 0xff);

	}
	print_data(dst, 46);
}

static void copy_self_drawer(struct maptbl *tbl, u8 *dst)
{
	struct panel_device *panel;
	struct act_clk_info *act_info;
	struct act_drawer_info *draw_info;

	if (!tbl || !dst)
		return;

	panel = (struct panel_device *)tbl->pdata;
	if (unlikely(!panel))
		return;

	draw_info = &panel->act_clk_dev.draw_info;
	act_info = &panel->act_clk_dev.act_info;

	dst[1] = 0;

	if (!act_info->en) {
		panel_info("ACT-CLK:INFO:disable act_clk\n");
		return;
	}

	dst[1] = 0x01;
	panel_info("PANEL:INFO:%s:self drawer\n", __func__);

	dst[29] = (act_info->pos_x >> 8) & 0xff;
	dst[30] = act_info->pos_x & 0xff;

	dst[31] = (act_info->pos_y >> 8) & 0xff;
	dst[32] = act_info->pos_y & 0xff;
	panel_info("PANEL:INFO:%s:active clock pos %d,%d\n", __func__,
		act_info->pos_x, act_info->pos_y);

	/* SD color : Red */
	dst[39] = (unsigned char)(draw_info->sd_line_color >> 16) & 0xff;
	/* SD color : Blue */
	dst[40] = (unsigned char)(draw_info->sd_line_color >> 8) & 0xff;
	/* SD color : Green */
	dst[41] = (unsigned char)(draw_info->sd_line_color & 0xff);

	/* SD2 color : Red */
	dst[42] = (unsigned char)(draw_info->sd2_line_color >> 16) & 0xff;
	/* SD2 color : Blue */
	dst[43] = (unsigned char)(draw_info->sd2_line_color >> 8) & 0xff;
	/* SD2 color : Green */
	dst[44] = (unsigned char)(draw_info->sd2_line_color & 0xff);

	dst[45] = (unsigned char)(draw_info->sd_radius & 0xff);
}
#endif

#ifdef CONFIG_EXYNOS_DECON_MDNIE_LITE
static int init_color_blind_table(struct maptbl *tbl)
{
	struct mdnie_info *mdnie;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	mdnie = tbl->pdata;

	if (S6E3FA7_SCR_CR_OFS + mdnie->props.sz_scr > sizeof_maptbl(tbl)) {
		panel_err("%s invalid size (maptbl_size %d, sz_scr %d)\n",
			__func__, sizeof_maptbl(tbl), mdnie->props.sz_scr);
		return -EINVAL;
	}

	memcpy(&tbl->arr[S6E3FA7_SCR_CR_OFS],
			mdnie->props.scr, mdnie->props.sz_scr);

	return 0;
}

static int getidx_mdnie_scenario_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;

	return tbl->ncol * (mdnie->props.mode);
}

#ifdef CONFIG_SUPPORT_HMD
static int getidx_mdnie_hmd_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;

	return tbl->ncol * (mdnie->props.hmd);
}
#endif

static int getidx_mdnie_hdr_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;

	return tbl->ncol * (mdnie->props.hdr);
}

static int getidx_mdnie_trans_mode_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;

	if (mdnie->props.trans_mode == TRANS_OFF)
		panel_dbg("%s mdnie trans_mode off\n", __func__);
	return tbl->ncol * (mdnie->props.trans_mode);
}

static int getidx_mdnie_night_mode_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;

	return tbl->ncol * (mdnie->props.night_level);
}

static int init_mdnie_night_mode_table(struct maptbl *tbl)
{
	struct mdnie_info *mdnie;
	struct maptbl *night_maptbl;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	mdnie = tbl->pdata;

	night_maptbl = mdnie_find_etc_maptbl(mdnie, MDNIE_ETC_NIGHT_MAPTBL);
	if (!night_maptbl) {
		panel_err("%s, NIGHT_MAPTBL not found\n", __func__);
		return -EINVAL;
	}

	if (sizeof_maptbl(tbl) < (S6E3FA7_NIGHT_MODE_OFS +
			sizeof_row(night_maptbl))) {
		panel_err("%s invalid size (maptbl_size %d, night_maptbl_size %d)\n",
			__func__, sizeof_maptbl(tbl), sizeof_row(night_maptbl));
		return -EINVAL;
	}

	maptbl_copy(night_maptbl, &tbl->arr[S6E3FA7_NIGHT_MODE_OFS]);

	return 0;
}

static int init_mdnie_color_lens_table(struct maptbl *tbl)
{
	struct mdnie_info *mdnie;
	struct maptbl *color_lens_maptbl;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	mdnie = tbl->pdata;

	color_lens_maptbl = mdnie_find_etc_maptbl(mdnie, MDNIE_ETC_COLOR_LENS_MAPTBL);
	if (!color_lens_maptbl) {
		panel_err("%s, COLOR_LENS_MAPTBL not found\n", __func__);
		return -EINVAL;
	}

	if (sizeof_maptbl(tbl) < (S6E3FA7_COLOR_LENS_OFS +
			sizeof_row(color_lens_maptbl))) {
		panel_err("%s invalid size (maptbl_size %d, color_lens_maptbl_size %d)\n",
			__func__, sizeof_maptbl(tbl), sizeof_row(color_lens_maptbl));
		return -EINVAL;
	}

	if (IS_COLOR_LENS_MODE(mdnie))
		maptbl_copy(color_lens_maptbl, &tbl->arr[S6E3FA7_COLOR_LENS_OFS]);

	return 0;
}

static void update_current_scr_white(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie;

	if (!tbl || !tbl->pdata) {
		pr_err("%s, invalid param\n", __func__);
		return;
	}

	mdnie = (struct mdnie_info *)tbl->pdata;
	mdnie->props.cur_wrgb[0] = *dst;
	mdnie->props.cur_wrgb[1] = *(dst + 2);
	mdnie->props.cur_wrgb[2] = *(dst + 4);
}

static int init_color_coordinate_table(struct maptbl *tbl)
{
	struct mdnie_info *mdnie;
	int type, color;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	mdnie = tbl->pdata;

	if (sizeof_row(tbl) != ARRAY_SIZE(mdnie->props.coord_wrgb[0])) {
		panel_err("%s invalid maptbl size %d\n", __func__, tbl->ncol);
		return -EINVAL;
	}

	for_each_row(tbl, type) {
		for_each_col(tbl, color) {
			tbl->arr[sizeof_row(tbl) * type + color] =
				mdnie->props.coord_wrgb[type][color];
		}
	}

	return 0;
}

static int init_sensor_rgb_table(struct maptbl *tbl)
{
	struct mdnie_info *mdnie;
	int i;

	if (tbl == NULL) {
		panel_err("PANEL:ERR:%s:maptbl is null\n", __func__);
		return -EINVAL;
	}

	if (tbl->pdata == NULL) {
		panel_err("PANEL:ERR:%s:pdata is null\n", __func__);
		return -EINVAL;
	}

	mdnie = tbl->pdata;

	if (tbl->ncol != ARRAY_SIZE(mdnie->props.ssr_wrgb)) {
		panel_err("%s invalid maptbl size %d\n", __func__, tbl->ncol);
		return -EINVAL;
	}

	for (i = 0; i < tbl->ncol; i++)
		tbl->arr[i] = mdnie->props.ssr_wrgb[i];

	return 0;
}

static int getidx_color_coordinate_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	static int wcrd_type[MODE_MAX] = {
		WCRD_TYPE_D65, WCRD_TYPE_D65, WCRD_TYPE_D65,
		WCRD_TYPE_ADAPTIVE, WCRD_TYPE_ADAPTIVE,
	};
	if ((mdnie->props.mode < 0) || (mdnie->props.mode >= MODE_MAX)) {
		pr_err("%s, out of mode range %d\n", __func__, mdnie->props.mode);
		return -EINVAL;
	}
	return maptbl_index(tbl, 0, wcrd_type[mdnie->props.mode], 0);
}

static int getidx_adjust_ldu_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;
	static int wcrd_type[MODE_MAX] = {
		WCRD_TYPE_D65, WCRD_TYPE_D65, WCRD_TYPE_D65,
		WCRD_TYPE_ADAPTIVE, WCRD_TYPE_ADAPTIVE,
	};

	if (!IS_LDU_MODE(mdnie))
		return -EINVAL;

	if ((mdnie->props.mode < 0) || (mdnie->props.mode >= MODE_MAX)) {
		pr_err("%s, out of mode range %d\n", __func__, mdnie->props.mode);
		return -EINVAL;
	}
	if ((mdnie->props.ldu < 0) || (mdnie->props.ldu >= MAX_LDU_MODE)) {
		pr_err("%s, out of ldu mode range %d\n", __func__, mdnie->props.ldu);
		return -EINVAL;
	}
	return maptbl_index(tbl, wcrd_type[mdnie->props.mode], mdnie->props.ldu, 0);
}

static int getidx_color_lens_maptbl(struct maptbl *tbl)
{
	struct mdnie_info *mdnie = (struct mdnie_info *)tbl->pdata;

	if (!IS_COLOR_LENS_MODE(mdnie))
		return -EINVAL;

	if ((mdnie->props.color_lens_color < 0) || (mdnie->props.color_lens_color >= COLOR_LENS_COLOR_MAX)) {
		pr_err("%s, out of color lens color range %d\n", __func__, mdnie->props.color_lens_color);
		return -EINVAL;
	}
	if ((mdnie->props.color_lens_level < 0) || (mdnie->props.color_lens_level >= COLOR_LENS_LEVEL_MAX)) {
		pr_err("%s, out of color lens level range %d\n", __func__, mdnie->props.color_lens_level);
		return -EINVAL;
	}
	return maptbl_index(tbl, mdnie->props.color_lens_color, mdnie->props.color_lens_level, 0);
}

static void copy_color_coordinate_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie;
	int i, idx;
	u8 value;

	if (unlikely(!tbl || !dst))
		return;

	mdnie = (struct mdnie_info *)tbl->pdata;
	idx = maptbl_getidx(tbl);
	if (idx < 0 || (idx + MAX_COLOR > sizeof_maptbl(tbl))) {
		panel_err("%s invalid index %d\n", __func__, idx);
		return;
	}

	if (tbl->ncol != MAX_COLOR) {
		panel_err("%s invalid maptbl size %d\n", __func__, tbl->ncol);
		return;
	}

	for (i = 0; i < tbl->ncol; i++) {
		mdnie->props.def_wrgb[i] = tbl->arr[idx + i];
		value = mdnie->props.def_wrgb[i] +
			(char)((mdnie->props.mode == AUTO) ?
				mdnie->props.def_wrgb_ofs[i] : 0);
		mdnie->props.cur_wrgb[i] = value;
		dst[i * 2] = value;

#ifdef DEBUG_PANEL
		if (mdnie->props.mode == AUTO) {
			panel_info("%s cur_wrgb[%d] %d(%02X) def_wrgb[%d] %d(%02X), def_wrgb_ofs[%d] %d\n",
					__func__, i, mdnie->props.cur_wrgb[i], mdnie->props.cur_wrgb[i],
					i, mdnie->props.def_wrgb[i], mdnie->props.def_wrgb[i],
					i, mdnie->props.def_wrgb_ofs[i]);
		} else {
			panel_info("%s cur_wrgb[%d] %d(%02X) def_wrgb[%d] %d(%02X), def_wrgb_ofs[%d] none\n",
					__func__, i, mdnie->props.cur_wrgb[i], mdnie->props.cur_wrgb[i],
					i, mdnie->props.def_wrgb[i], mdnie->props.def_wrgb[i], i);
		}
#endif
	}
}

static void copy_scr_white_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie;
	int i, idx;

	if (unlikely(!tbl || !dst))
		return;

	mdnie = (struct mdnie_info *)tbl->pdata;
	idx = maptbl_getidx(tbl);
	if (idx < 0 || (idx + MAX_COLOR > sizeof_maptbl(tbl))) {
		panel_err("%s invalid index %d\n", __func__, idx);
		return;
	}

	if (tbl->ncol != MAX_COLOR) {
		panel_err("%s invalid maptbl size %d\n", __func__, tbl->ncol);
		return;
	}

	for (i = 0; i < tbl->ncol; i++) {
		mdnie->props.cur_wrgb[i] = tbl->arr[idx + i];
		dst[i * 2] = tbl->arr[idx + i];
#ifdef DEBUG_PANEL
		panel_info("%s cur_wrgb[%d] %d(%02X)\n",
				__func__, i, mdnie->props.cur_wrgb[i], mdnie->props.cur_wrgb[i]);
#endif
	}
}

static void copy_adjust_ldu_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie;
	int i, idx;
	u8 value;

	if (unlikely(!tbl || !dst))
		return;

	mdnie = (struct mdnie_info *)tbl->pdata;
	idx = maptbl_getidx(tbl);
	if (idx < 0 || (idx + MAX_COLOR > sizeof_maptbl(tbl))) {
		panel_err("%s invalid index %d\n", __func__, idx);
		return;
	}

	if (tbl->ncol != MAX_COLOR) {
		panel_err("%s invalid maptbl size %d\n", __func__, tbl->ncol);
		return;
	}

	for (i = 0; i < tbl->ncol; i++) {
		value = tbl->arr[idx + i] +
			(((mdnie->props.mode == AUTO) && (mdnie->props.scenario != EBOOK_MODE)) ?
				mdnie->props.def_wrgb_ofs[i] : 0);
		mdnie->props.cur_wrgb[i] = value;
		dst[i * 2] = value;

#ifdef DEBUG_PANEL
		panel_info("%s cur_wrgb[%d] %d(%02X) (orig:0x%02X offset:%d)\n",
				__func__, i, mdnie->props.cur_wrgb[i], mdnie->props.cur_wrgb[i],
				tbl->arr[idx + i], mdnie->props.def_wrgb_ofs[i]);
#endif
	}
}

static int getidx_trans_maptbl(struct pkt_update_info *pktui)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;

	return (mdnie->props.trans_mode == TRANS_ON) ?
		MDNIE_ETC_NONE_MAPTBL : MDNIE_ETC_TRANS_MAPTBL;
}

static int getidx_mdnie_maptbl(struct pkt_update_info *pktui, int offset)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;
	int row = mdnie_get_maptbl_index(mdnie);
	int index;

	if (row < 0) {
		pr_err("%s, invalid row %d\n", __func__, row);
		return -EINVAL;
	}

	index = row * mdnie->nr_reg + offset;
	if (index >= mdnie->nr_maptbl) {
		panel_err("%s exceeded index %d row %d offset %d\n",
				__func__, index, row, offset);
		return -EINVAL;
	}
	return index;
}

static int getidx_mdnie_0_maptbl(struct pkt_update_info *pktui)
{
	return getidx_mdnie_maptbl(pktui, 0);
}

static int getidx_mdnie_1_maptbl(struct pkt_update_info *pktui)
{
	return getidx_mdnie_maptbl(pktui, 1);
}

static int getidx_mdnie_2_maptbl(struct pkt_update_info *pktui)
{
	return getidx_mdnie_maptbl(pktui, 2);
}

static int getidx_mdnie_scr_white_maptbl(struct pkt_update_info *pktui)
{
	struct panel_device *panel = pktui->pdata;
	struct mdnie_info *mdnie = &panel->mdnie;
	int index;

	if (mdnie->props.scr_white_mode < 0 ||
			mdnie->props.scr_white_mode >= MAX_SCR_WHITE_MODE) {
		pr_warn("%s, out of range %d\n",
				__func__, mdnie->props.scr_white_mode);
		return -1;
	}

	if (mdnie->props.scr_white_mode == SCR_WHITE_MODE_COLOR_COORDINATE) {
		pr_debug("%s, coordinate maptbl\n", __func__);
		index = MDNIE_COLOR_COORDINATE_MAPTBL;
	} else if (mdnie->props.scr_white_mode == SCR_WHITE_MODE_ADJUST_LDU) {
		pr_debug("%s, adjust ldu maptbl\n", __func__);
		index = MDNIE_ADJUST_LDU_MAPTBL;
	} else if (mdnie->props.scr_white_mode == SCR_WHITE_MODE_SENSOR_RGB) {
		pr_debug("%s, sensor rgb maptbl\n", __func__);
		index = MDNIE_SENSOR_RGB_MAPTBL;
	} else {
		pr_debug("%s, empty maptbl\n", __func__);
		index = MDNIE_SCR_WHITE_NONE_MAPTBL;
	}

	return index;
}

#ifdef CONFIG_SUPPORT_AFC
static void copy_afc_maptbl(struct maptbl *tbl, u8 *dst)
{
	struct mdnie_info *mdnie;

	if (!tbl || !dst) {
		pr_err("%s, invalid parameter (tbl %p, dst %p\n",
				__func__, tbl, dst);
		return;
	}
	mdnie = (struct mdnie_info *)tbl->pdata;
	memcpy(dst, mdnie->props.afc_roi,
			sizeof(mdnie->props.afc_roi));

#ifdef DEBUG_PANEL
	pr_info("%s afc_on %d\n", __func__, mdnie->props.afc_on);
	print_data(dst, sizeof(mdnie->props.afc_roi));
#endif
}
#endif
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */

static void show_rddpm(struct dumpinfo *info)
{
	int ret;
	struct resinfo *res = info->res;
	u8 rddpm[S6E3FA7_RDDPM_LEN] = { 0, };
#ifdef CONFIG_LOGGING_BIGDATA_BUG
	extern unsigned int g_rddpm;
#endif

	if (!res || ARRAY_SIZE(rddpm) != res->dlen) {
		pr_err("%s invalid resource\n", __func__);
		return;
	}

	ret = resource_copy(rddpm, info->res);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy rddpm resource\n", __func__);
		return;
	}

	panel_info("========== SHOW PANEL [0Ah:RDDPM] INFO ==========\n");
	panel_info("* Reg Value : 0x%02x, Result : %s\n",
			rddpm[0], ((rddpm[0] & 0x9C) == 0x9C) ? "GOOD" : "NG");
	panel_info("* Bootster Mode : %s\n", rddpm[0] & 0x80 ? "ON (GD)" : "OFF (NG)");
	panel_info("* Idle Mode     : %s\n", rddpm[0] & 0x40 ? "ON (NG)" : "OFF (GD)");
	panel_info("* Partial Mode  : %s\n", rddpm[0] & 0x20 ? "ON" : "OFF");
	panel_info("* Sleep Mode    : %s\n", rddpm[0] & 0x10 ? "OUT (GD)" : "IN (NG)");
	panel_info("* Normal Mode   : %s\n", rddpm[0] & 0x08 ? "OK (GD)" : "SLEEP (NG)");
	panel_info("* Display ON    : %s\n", rddpm[0] & 0x04 ? "ON (GD)" : "OFF (NG)");
	panel_info("=================================================\n");
#ifdef CONFIG_LOGGING_BIGDATA_BUG
	g_rddpm = (unsigned int)rddpm[0];
#endif
}

static void show_rddsm(struct dumpinfo *info)
{
	int ret;
	struct resinfo *res = info->res;
	u8 rddsm[S6E3FA7_RDDSM_LEN] = { 0, };
#ifdef CONFIG_LOGGING_BIGDATA_BUG
	extern unsigned int g_rddsm;
#endif

	if (!res || ARRAY_SIZE(rddsm) != res->dlen) {
		pr_err("%s invalid resource\n", __func__);
		return;
	}

	ret = resource_copy(rddsm, info->res);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy rddsm resource\n", __func__);
		return;
	}

	panel_info("========== SHOW PANEL [0Eh:RDDSM] INFO ==========\n");
	panel_info("* Reg Value : 0x%02x, Result : %s\n",
			rddsm[0], (rddsm[0] == 0x80) ? "GOOD" : "NG");
	panel_info("* TE Mode : %s\n", rddsm[0] & 0x80 ? "ON(GD)" : "OFF(NG)");
	panel_info("=================================================\n");
#ifdef CONFIG_LOGGING_BIGDATA_BUG
	g_rddsm = (unsigned int)rddsm[0];
#endif
}

static void show_err(struct dumpinfo *info)
{
	int ret;
	struct resinfo *res = info->res;
	u8 err[S6E3FA7_ERR_LEN] = { 0, }, err_15_8, err_7_0;

	if (!res || ARRAY_SIZE(err) != res->dlen) {
		pr_err("%s invalid resource\n", __func__);
		return;
	}

	ret = resource_copy(err, info->res);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy err resource\n", __func__);
		return;
	}

	err_15_8 = err[0];
	err_7_0 = err[1];

	panel_info("========== SHOW PANEL [EAh:DSIERR] INFO ==========\n");
	panel_info("* Reg Value : 0x%02x%02x, Result : %s\n", err_15_8, err_7_0,
			(err[0] || err[1] || err[2] || err[3] || err[4]) ? "NG" : "GOOD");

	if (err_15_8 & 0x80)
		panel_info("* DSI Protocol Violation\n");

	if (err_15_8 & 0x40)
		panel_info("* Data P Lane Contention Detetion\n");

	if (err_15_8 & 0x20)
		panel_info("* Invalid Transmission Length\n");

	if (err_15_8 & 0x10)
		panel_info("* DSI VC ID Invalid\n");

	if (err_15_8 & 0x08)
		panel_info("* DSI Data Type Not Recognized\n");

	if (err_15_8 & 0x04)
		panel_info("* Checksum Error\n");

	if (err_15_8 & 0x02)
		panel_info("* ECC Error, multi-bit (detected, not corrected)\n");

	if (err_15_8 & 0x01)
		panel_info("* ECC Error, single-bit (detected and corrected)\n");

	if (err_7_0 & 0x80)
		panel_info("* Data Lane Contention Detection\n");

	if (err_7_0 & 0x40)
		panel_info("* False Control Error\n");

	if (err_7_0 & 0x20)
		panel_info("* HS RX Timeout\n");

	if (err_7_0 & 0x10)
		panel_info("* Low-Power Transmit Sync Error\n");

	if (err_7_0 & 0x08)
		panel_info("* Escape Mode Entry Command Error");

	if (err_7_0 & 0x04)
		panel_info("* EoT Sync Error\n");

	if (err_7_0 & 0x02)
		panel_info("* SoT Sync Error\n");

	if (err_7_0 & 0x01)
		panel_info("* SoT Error\n");

	panel_info("* CRC Error Count : %d\n", err[2]);
	panel_info("* ECC1 Error Count : %d\n", err[3]);
	panel_info("* ECC2 Error Count : %d\n", err[4]);

	panel_info("==================================================\n");
}

static void show_err_fg(struct dumpinfo *info)
{
	int ret;
	u8 err_fg[S6E3FA7_ERR_FG_LEN] = { 0, };
	struct resinfo *res = info->res;

	if (!res || ARRAY_SIZE(err_fg) != res->dlen) {
		pr_err("%s invalid resource\n", __func__);
		return;
	}

	ret = resource_copy(err_fg, res);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy err_fg resource\n", __func__);
		return;
	}

	panel_info("========== SHOW PANEL [EEh:ERR_FG] INFO ==========\n");
	panel_info("* Reg Value : 0x%02x, Result : %s\n",
			err_fg[0], (err_fg[0] & 0x4C) ? "NG" : "GOOD");

	if (err_fg[0] & 0x04) {
		panel_info("* VLOUT3 Error\n");
		inc_dpui_u32_field(DPUI_KEY_PNVLO3E, 1);
	}

	if (err_fg[0] & 0x08) {
		panel_info("* ELVDD Error\n");
		inc_dpui_u32_field(DPUI_KEY_PNELVDE, 1);
	}

	if (err_fg[0] & 0x40) {
		panel_info("* VLIN1 Error\n");
		inc_dpui_u32_field(DPUI_KEY_PNVLI1E, 1);
	}

	panel_info("==================================================\n");
}

static void show_dsi_err(struct dumpinfo *info)
{
	int ret;
	struct resinfo *res = info->res;
	u8 dsi_err[S6E3FA7_DSI_ERR_LEN] = { 0, };

	if (!res || ARRAY_SIZE(dsi_err) != res->dlen) {
		pr_err("%s invalid resource\n", __func__);
		return;
	}

	ret = resource_copy(dsi_err, res);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy dsi_err resource\n", __func__);
		return;
	}

	panel_info("========== SHOW PANEL [05h:DSIE_CNT] INFO ==========\n");
	panel_info("* Reg Value : 0x%02x, Result : %s\n",
			dsi_err[0], (dsi_err[0]) ? "NG" : "GOOD");
	if (dsi_err[0])
		panel_info("* DSI Error Count : %d\n", dsi_err[0]);
	panel_info("====================================================\n");

	inc_dpui_u32_field(DPUI_KEY_PNDSIE, dsi_err[0]);
}

static void show_self_diag(struct dumpinfo *info)
{
	int ret;
	struct resinfo *res = info->res;
	u8 self_diag[S6E3FA7_SELF_DIAG_LEN] = { 0, };

	ret = resource_copy(self_diag, res);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy self_diag resource\n", __func__);
		return;
	}

	panel_info("========== SHOW PANEL [0Fh:SELF_DIAG] INFO ==========\n");
	panel_info("* Reg Value : 0x%02x, Result : %s\n",
			self_diag[0], (self_diag[0] & 0x80) ? "GOOD" : "NG");
	if ((self_diag[0] & 0x80) == 0)
		panel_info("* OTP Reg Loading Error\n");
	panel_info("=====================================================\n");

	inc_dpui_u32_field(DPUI_KEY_PNSDRE, (self_diag[0] & 0x80) ? 0 : 1);
}
