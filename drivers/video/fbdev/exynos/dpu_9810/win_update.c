/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Window update file for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <video/mipi_display.h>

#include "decon.h"
#include "dpp.h"
#include "dsim.h"

static void win_update_adjust_region(struct decon_device *decon,
		struct decon_win_config *win_config,
		struct decon_reg_data *regs)
{
	int i;
	int div_w, div_h;
	struct decon_rect r1, r2;
	struct decon_win_config *update_config = &win_config[DECON_WIN_UPDATE_IDX];
	struct decon_win_config *config;
	struct decon_frame adj_region;

	regs->need_update = false;
	DPU_FULL_RECT(&regs->up_region, decon->lcd_info);

	if (!decon->win_up.enabled)
		return;

	if (update_config->state != DECON_WIN_STATE_UPDATE)
		return;

	if ((update_config->dst.x < 0) || (update_config->dst.y < 0)) {
		update_config->state = DECON_WIN_STATE_DISABLED;
		return;
	}

	r1.left = update_config->dst.x;
	r1.top = update_config->dst.y;
	r1.right = r1.left + update_config->dst.w - 1;
	r1.bottom = r1.top + update_config->dst.h - 1;

	for (i = 0; i < decon->dt.max_win; i++) {
		config = &win_config[i];
		if (config->state != DECON_WIN_STATE_DISABLED) {
			if (config->dpp_parm.rot || is_scaling(config)) {
				update_config->state = DECON_WIN_STATE_DISABLED;
				return;
			}
		}
	}

	DPU_DEBUG_WIN("original update region[%d %d %d %d]\n",
			update_config->dst.x, update_config->dst.y,
			update_config->dst.w, update_config->dst.h);

	r2.left = (r1.left / decon->win_up.rect_w) * decon->win_up.rect_w;
	r2.top = (r1.top / decon->win_up.rect_h) * decon->win_up.rect_h;

	div_w = (r1.right + 1) / decon->win_up.rect_w;
	div_w = (div_w * decon->win_up.rect_w == r1.right + 1) ? div_w : div_w + 1;
	r2.right = div_w * decon->win_up.rect_w - 1;

	div_h = (r1.bottom + 1) / decon->win_up.rect_h;
	div_h = (div_h * decon->win_up.rect_h == r1.bottom + 1) ? div_h : div_h + 1;
	r2.bottom = div_h * decon->win_up.rect_h - 1;

	/* TODO: Now, 4 slices must be used. This will be modified */
	r2.left = 0;
	r2.right = decon->lcd_info->xres - 1;

	memcpy(&regs->up_region, &r2, sizeof(struct decon_rect));

	memset(&adj_region, 0, sizeof(struct decon_frame));
	adj_region.x = regs->up_region.left;
	adj_region.y = regs->up_region.top;
	adj_region.w = regs->up_region.right - regs->up_region.left + 1;
	adj_region.h = regs->up_region.bottom - regs->up_region.top + 1;
	DPU_EVENT_LOG_UPDATE_REGION(&decon->sd, &update_config->dst, &adj_region);

	DPU_DEBUG_WIN("adjusted update region[%d %d %d %d]\n",
			adj_region.x, adj_region.y, adj_region.w, adj_region.h);
}

static void win_update_check_limitation(struct decon_device *decon,
		struct decon_win_config *win_config,
		struct decon_reg_data *regs)
{
	struct decon_win_config *config;
	struct decon_win_rect update;
	struct decon_rect r;
	int i;
	int sz_align = 1;
	int adj_src_x = 0, adj_src_y = 0;

	for (i = 0; i < decon->dt.max_win; i++) {
		config = &win_config[i];
		if (config->state == DECON_WIN_STATE_DISABLED)
			continue;

		r.left = config->dst.x;
		r.top = config->dst.y;
		r.right = config->dst.w + config->dst.x - 1;
		r.bottom = config->dst.h + config->dst.y - 1;

		if (!decon_intersect(&regs->up_region, &r))
			continue;

		decon_intersection(&regs->up_region, &r, &r);

		if (!(r.right - r.left) && !(r.bottom - r.top))
			continue;

		if (is_yuv(config)) {
			/* check alignment for NV12/NV21 format */
			update.x = regs->up_region.left;
			update.y = regs->up_region.top;
			sz_align = 2;

			if (update.y > config->dst.y)
				adj_src_y = config->src.y + (update.y - config->dst.y);
			if (update.x > config->dst.x)
				adj_src_x = config->src.x + (update.x - config->dst.x);

			if (adj_src_x & 0x1 || adj_src_y & 0x1)
				goto change_full;
		}

		if (((r.right - r.left) < (SRC_WIDTH_MIN * sz_align)) ||
				((r.bottom - r.top) < (SRC_HEIGHT_MIN * sz_align))) {
			goto change_full;
		}

		/* cursor async */
		if (((r.right - r.left) > decon->lcd_info->xres) ||
			((r.bottom - r.top) > decon->lcd_info->yres)) {
			goto change_full;
		}
	}

	return;

change_full:
	DPU_DEBUG_WIN("changed full: win(%d) idma(%d) [%d %d %d %d]\n",
			i, config->idma_type,
			config->dst.x, config->dst.y,
			config->dst.w, config->dst.h);
	DPU_FULL_RECT(&regs->up_region, decon->lcd_info);
}

static void win_update_reconfig_coordinates(struct decon_device *decon,
		struct decon_win_config *win_config,
		struct decon_reg_data *regs)
{
	struct decon_win_config *config;
	struct decon_win_rect update;
	struct decon_frame origin_dst, origin_src;
	struct decon_rect r;
	int i;

	/* Assume that, window update doesn't support in case of scaling */
	for (i = 0; i < decon->dt.max_win; i++) {
		config = &win_config[i];

		if (config->state == DECON_WIN_STATE_DISABLED)
			continue;

		r.left = config->dst.x;
		r.top = config->dst.y;
		r.right = r.left + config->dst.w - 1;
		r.bottom = r.top + config->dst.h - 1;
		if (!decon_intersect(&regs->up_region, &r)) {
			config->state = DECON_WIN_STATE_DISABLED;
			continue;
		}

		update.x = regs->up_region.left;
		update.y = regs->up_region.top;
		update.w = regs->up_region.right - regs->up_region.left + 1;
		update.h = regs->up_region.bottom - regs->up_region.top + 1;

		memcpy(&origin_dst, &config->dst, sizeof(struct decon_frame));
		memcpy(&origin_src, &config->src, sizeof(struct decon_frame));

		/* reconfigure destination coordinates */
		if (update.x > config->dst.x)
			config->dst.w = min(update.w,
					config->dst.x + config->dst.w - update.x);
		else if (update.x + update.w < config->dst.x + config->dst.w)
			config->dst.w = min(config->dst.w,
					update.w + update.x - config->dst.x);

		if (update.y > config->dst.y)
			config->dst.h = min(update.h,
					config->dst.y + config->dst.h - update.y);
		else if (update.y + update.h < config->dst.y + config->dst.h)
			config->dst.h = min(config->dst.h,
					update.h + update.y - config->dst.y);
		config->dst.x = max(config->dst.x - update.x, 0);
		config->dst.y = max(config->dst.y - update.y, 0);

		/* reconfigure source coordinates */
		if (update.y > origin_dst.y)
			config->src.y += (update.y - origin_dst.y);
		if (update.x > origin_dst.x)
			config->src.x += (update.x - origin_dst.x);
		config->src.w = config->dst.w;
		config->src.h = config->dst.h;

		DPU_DEBUG_WIN("win(%d), idma(%d)\n", i, config->idma_type);
		DPU_DEBUG_WIN("src: origin[%d %d %d %d] -> change[%d %d %d %d]\n",
				origin_src.x, origin_src.y,
				origin_src.w, origin_src.h,
				config->src.x, config->src.y,
				config->src.w, config->src.h);
		DPU_DEBUG_WIN("dst: origin[%d %d %d %d] -> change[%d %d %d %d]\n",
				origin_dst.x, origin_dst.y,
				origin_dst.w, origin_dst.h,
				config->dst.x, config->dst.y,
				config->dst.w, config->dst.h);
	}
}

void dpu_prepare_win_update_config(struct decon_device *decon,
		struct decon_win_config_data *win_data,
		struct decon_reg_data *regs)
{
	struct decon_win_config *win_config = win_data->config;
	bool reconfigure = false;
	struct decon_rect r;

	if (!decon->win_up.enabled)
		return;

	if (decon->dt.out_type != DECON_OUT_DSI)
		return;

	/* find adjusted update region on LCD */
	win_update_adjust_region(decon, win_config, regs);

	/* check DPP hw limitation if violated, update region is changed to full */
	win_update_check_limitation(decon, win_config, regs);

	/*
	 * If update region is changed, need_update flag is set.
	 * That means hw configuration is needed
	 */
	if (is_decon_rect_differ(&decon->win_up.prev_up_region, &regs->up_region))
		regs->need_update = true;
	else
		regs->need_update = false;

	/*
	 * If partial update region is requested, source and destination
	 * coordinates are needed to change if overlapped with update region.
	 */
	DPU_FULL_RECT(&r, decon->lcd_info);
	if (is_decon_rect_differ(&regs->up_region, &r))
		reconfigure = true;

	if (regs->need_update || reconfigure) {
		DPU_DEBUG_WIN("need_update(%d), reconfigure(%d)\n",
				regs->need_update, reconfigure);
		DPU_EVENT_LOG_WINUP_FLAGS(&decon->sd, regs->need_update, reconfigure);
	}

	/* Reconfigure source and destination coordinates, if needed. */
	if (reconfigure)
		win_update_reconfig_coordinates(decon, win_config, regs);
}

static int win_update_send_partial_command(struct dsim_device *dsim,
		struct decon_rect *rect)
{
	char column[5];
	char page[5];
	int retry;

	DPU_DEBUG_WIN("SET: [%d %d %d %d]\n", rect->left, rect->top,
			rect->right - rect->left + 1, rect->bottom - rect->top + 1);

	column[0] = MIPI_DCS_SET_COLUMN_ADDRESS;
	column[1] = (rect->left >> 8) & 0xff;
	column[2] = rect->left & 0xff;
	column[3] = (rect->right >> 8) & 0xff;
	column[4] = rect->right & 0xff;

	page[0] = MIPI_DCS_SET_PAGE_ADDRESS;
	page[1] = (rect->top >> 8) & 0xff;
	page[2] = rect->top & 0xff;
	page[3] = (rect->bottom >> 8) & 0xff;
	page[4] = rect->bottom & 0xff;

	retry = 2;
	while (dsim_write_data(dsim, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned long)column, ARRAY_SIZE(column)) != 0) {
		dsim_err("failed to write COLUMN_ADDRESS\n");
		dsim_reg_function_reset(dsim->id);
		if (--retry <= 0) {
			dsim_err("COLUMN_ADDRESS is failed: exceed retry count\n");
			return -EINVAL;
		}
	}

	retry = 2;
	while (dsim_write_data(dsim, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned long)page, ARRAY_SIZE(page)) != 0) {
		dsim_err("failed to write PAGE_ADDRESS\n");
		dsim_reg_function_reset(dsim->id);
		if (--retry <= 0) {
			dsim_err("PAGE_ADDRESS is failed: exceed retry count\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void win_update_find_included_slice(struct decon_lcd *lcd,
		struct decon_rect *rect, bool in_slice[])
{
	int slice_left, slice_right, slice_width;
	int i;

	slice_left = 0;
	slice_right = 0;
	slice_width = lcd->xres / lcd->dsc_slice_num;

	for (i = 0; i < lcd->dsc_slice_num; ++i) {
		slice_left = slice_width * i;
		slice_right = slice_left + slice_width - 1;
		in_slice[i] = false;

		if ((slice_left >= rect->left) && (slice_right <= rect->right))
			in_slice[i] = true;

		DPU_DEBUG_WIN("slice_left(%d), right(%d)\n", slice_left, slice_right);
		DPU_DEBUG_WIN("slice[%d] is %s\n", i,
				in_slice[i] ? "included" : "not included");
	}
}

static void win_update_set_partial_size(struct decon_device *decon,
		struct decon_rect *rect)
{
	struct decon_lcd lcd_info;
	struct dsim_device *dsim = get_dsim_drvdata(0);
	bool in_slice[MAX_DSC_SLICE_CNT];

	memcpy(&lcd_info, decon->lcd_info, sizeof(struct decon_lcd));
	lcd_info.xres = rect->right - rect->left + 1;
	lcd_info.yres = rect->bottom - rect->top + 1;

	lcd_info.hfp = decon->lcd_info->hfp +
		((decon->lcd_info->xres - lcd_info.xres) >> 1);
	lcd_info.vfp = decon->lcd_info->vfp + decon->lcd_info->yres - lcd_info.yres;

	dsim_reg_set_partial_update(dsim->id, &lcd_info);

	win_update_find_included_slice(decon->lcd_info, rect, in_slice);
	decon_reg_set_partial_update(decon->id, decon->dt.dsi_mode,
			decon->lcd_info, in_slice,
			lcd_info.xres, lcd_info.yres);
	DPU_DEBUG_WIN("SET: vfp %d vbp %d vsa %d hfp %d hbp %d hsa %d w %d h %d\n",
			lcd_info.vfp, lcd_info.vbp, lcd_info.vsa,
			lcd_info.hfp, lcd_info.hbp, lcd_info.hsa,
			lcd_info.xres, lcd_info.yres);
}

void dpu_set_win_update_config(struct decon_device *decon,
		struct decon_reg_data *regs)
{
	struct dsim_device *dsim = get_dsim_drvdata(0);
	bool in_slice[MAX_DSC_SLICE_CNT];
	bool full_partial_update = false;

	if (!decon->win_up.enabled)
		return;

	if (decon->dt.out_type != DECON_OUT_DSI)
		return;

	if (regs == NULL) {
		regs = kzalloc(sizeof(struct decon_reg_data), GFP_KERNEL);
		if (!regs) {
			decon_err("%s:set window update config fail\
					reg_data allocation fail\n", __func__);
			return;
		}
		DPU_FULL_RECT(&regs->up_region, decon->lcd_info);
		regs->need_update = true;
		full_partial_update = true;
	}

	if (regs->need_update) {
		win_update_find_included_slice(decon->lcd_info,
				&regs->up_region, in_slice);

		/* TODO: Is waiting framedone irq needed in KC ? */

		/*
		 * hw configuration related to partial update must be set
		 * without DMA operation
		 */
		decon_reg_wait_idle_status_timeout(decon->id, IDLE_WAIT_TIMEOUT);
		win_update_send_partial_command(dsim, &regs->up_region);
		win_update_set_partial_size(decon, &regs->up_region);
		DPU_EVENT_LOG_APPLY_REGION(&decon->sd, &regs->up_region);
	}

	if (full_partial_update)
		kfree(regs);
}

void dpu_set_win_update_partial_size(struct decon_device *decon,
		struct decon_rect *up_region)
{
	if (!decon->win_up.enabled)
		return;

	win_update_set_partial_size(decon, up_region);
}

void dpu_init_win_update(struct decon_device *decon)
{
	struct decon_lcd *lcd = decon->lcd_info;

	decon->win_up.enabled = false;
	decon->cursor.xpos = lcd->xres / 2;
	decon->cursor.ypos = lcd->yres / 2;

	if (!IS_ENABLED(CONFIG_FB_WINDOW_UPDATE)) {
		decon_info("window update feature is disabled\n");
		return;
	}

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_info("out_type(%d) doesn't support window update\n",
				decon->dt.out_type);
		return;
	}

	if (lcd->dsc_enabled) {
		decon->win_up.rect_w = lcd->xres / lcd->dsc_slice_num;
		decon->win_up.rect_h = lcd->dsc_slice_h;
	} else {
		decon->win_up.rect_w = MIN_WIN_BLOCK_WIDTH;
		decon->win_up.rect_h = MIN_WIN_BLOCK_HEIGHT;
	}

	DPU_FULL_RECT(&decon->win_up.prev_up_region, lcd);

	decon->win_up.hori_cnt = decon->lcd_info->xres / decon->win_up.rect_w;
	if (decon->lcd_info->xres - decon->win_up.hori_cnt * decon->win_up.rect_w) {
		decon_warn("%s: parameters is wrong. lcd w(%d), win rect w(%d)\n",
				__func__, decon->lcd_info->xres,
				decon->win_up.rect_w);
		return;
	}

	decon->win_up.verti_cnt = decon->lcd_info->yres / decon->win_up.rect_h;
	if (decon->lcd_info->yres - decon->win_up.verti_cnt * decon->win_up.rect_h) {
		decon_warn("%s: parameters is wrong. lcd h(%d), win rect h(%d)\n",
				__func__, decon->lcd_info->yres,
				decon->win_up.rect_h);
		return;
	}

	decon_info("window update is enabled: win rectangle w(%d), h(%d)\n",
			decon->win_up.rect_w, decon->win_up.rect_h);
	decon_info("horizontal count(%d), vertical count(%d)\n",
			decon->win_up.hori_cnt, decon->win_up.verti_cnt);

	decon->win_up.enabled = true;
}
