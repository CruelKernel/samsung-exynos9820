/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Cursor Async file for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "decon.h"
#include "dpp.h"

void decon_set_cursor_reset(struct decon_device *decon,
		struct decon_reg_data *regs)
{
	if (!decon->cursor.enabled)
		return;

	mutex_lock(&decon->cursor.lock);
	decon->cursor.unmask = false;
	memcpy(&decon->cursor.regs, regs, sizeof(struct decon_reg_data));
	mutex_unlock(&decon->cursor.lock);
}

void decon_set_cursor_unmask(struct decon_device *decon, bool unmask)
{
	if (!decon->cursor.enabled)
		return;

	mutex_lock(&decon->cursor.lock);
	decon->cursor.unmask = unmask;
	mutex_unlock(&decon->cursor.lock);
}

static void decon_set_cursor_pos(struct decon_device *decon, int x, int y)
{
	if (!decon->cursor.enabled)
		return;

	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;

	decon->cursor.xpos = x;
	decon->cursor.ypos = y;
}

static int decon_set_cursor_dpp_config(struct decon_device *decon,
		struct decon_reg_data *regs)
{
	int i, ret = 0, err_cnt = 0;
	struct v4l2_subdev *sd;
	struct decon_win *win;
	struct dpp_config dpp_config;
	unsigned long aclk_khz;

	if (!decon->cursor.enabled)
		return 0;

	if (!regs->is_cursor_win[regs->cursor_win])
		return -1;

	i = regs->cursor_win;
	win = decon->win[i];
	if (!test_bit(win->dpp_id, &decon->cur_using_dpp))
		return -2;

	aclk_khz = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			EXYNOS_DPU_GET_ACLK, NULL) / 1000U;

	sd = decon->dpp_sd[win->dpp_id];
	memcpy(&dpp_config.config, &regs->dpp_config[i],
			sizeof(struct decon_win_config));
	dpp_config.rcv_num = aclk_khz;
	ret = v4l2_subdev_call(sd, core, ioctl, DPP_WIN_CONFIG, &dpp_config);
	if (ret) {
		decon_err("failed to config (WIN%d : DPP%d)\n",
						i, win->dpp_id);
		regs->win_regs[i].wincon &= (~WIN_EN_F(i));
		decon_reg_set_win_enable(decon->id, i, false);
		if (regs->num_of_window != 0)
			regs->num_of_window--;

		clear_bit(win->dpp_id, &decon->cur_using_dpp);
		set_bit(win->dpp_id, &decon->dpp_err_stat);
		err_cnt++;
	}
	return err_cnt;
}

void dpu_cursor_win_update_config(struct decon_device *decon,
		struct decon_reg_data *regs)
{
	struct decon_frame src, dst;
	struct v4l2_subdev *sd;
	struct dpp_ch_restriction ch_res;
	struct dpp_restriction *res;
	unsigned short cur = regs->cursor_win;

	if (!decon->cursor.enabled)
		return;

	if (!decon->id) {
		decon_dbg("%s, decon[%d] is not support cursor a-sync\n",
				__func__, decon->id);
		return;
	}
	if (!(regs->win_regs[cur].wincon & WIN_EN_F(cur))) {
		decon_err("%s, window[%d] is not enabled\n", __func__, cur);
		return;
	}

	if (!regs->is_cursor_win[cur]) {
		decon_err("%s, window[%d] is not cursor layer\n",
				__func__, cur);
		return;
	}

	sd = decon->dpp_sd[0];
	v4l2_subdev_call(sd, core, ioctl, DPP_GET_RESTRICTION, &ch_res);
	res = &ch_res.restriction;

	memcpy(&src, &regs->dpp_config[cur].src, sizeof(struct decon_frame));
	memcpy(&dst, &regs->dpp_config[cur].dst, sizeof(struct decon_frame));

	dst.x = decon->cursor.xpos;
	dst.y = decon->cursor.ypos;

	if ((dst.x + dst.w) > decon->lcd_info->xres)
		dst.w = dst.w - ((dst.x + dst.w) - decon->lcd_info->xres);
	if ((dst.y + dst.h) > decon->lcd_info->yres)
		dst.h = dst.h - ((dst.y + dst.h) - decon->lcd_info->yres);

	if (dst.w > res->src_f_w.max || dst.w < res->src_f_w.min ||
		dst.h > res->src_f_h.max || dst.h < res->src_f_h.min) {
		decon_info("not supported cursor: [%d] [%d %d] ",
				cur, decon->lcd_info->xres,
				decon->lcd_info->yres);
		decon_info("src:(%d,%d,%d,%d)", src.x, src.y, src.w, src.h);
		decon_info("dst:(%d,%d,%d,%d) (%d,%d,%d,%d)\n",
				regs->dpp_config[cur].dst.x,
				regs->dpp_config[cur].dst.y,
				regs->dpp_config[cur].dst.w,
				regs->dpp_config[cur].dst.h,
				dst.x, dst.y, dst.w, dst.h);
		memcpy(&dst, &regs->dpp_config[cur].dst,
				sizeof(struct decon_frame));
	}

	regs->win_regs[cur].start_pos = win_start_pos(dst.x, dst.y);
	regs->win_regs[cur].end_pos = win_end_pos(dst.x, dst.y, dst.w, dst.h);

	if ((dst.w != regs->dpp_config[cur].dst.w) ||
			(dst.h != regs->dpp_config[cur].dst.h)) {
		decon_dbg("cursor update: [%d] [%d %d] src:(%d,%d,%d,%d)",
				cur, decon->lcd_info->xres,
				decon->lcd_info->yres,
				src.x, src.y, src.w, src.h);
		decon_dbg("dst:(%d,%d,%d,%d) (%d,%d,%d,%d)\n",
				regs->dpp_config[cur].dst.x,
				regs->dpp_config[cur].dst.y,
				regs->dpp_config[cur].dst.w,
				regs->dpp_config[cur].dst.h,
				dst.x, dst.y, dst.w, dst.h);
	}
	regs->dpp_config[cur].src.w = dst.w;
	regs->dpp_config[cur].src.h = dst.h;
	regs->dpp_config[cur].dst.x = dst.x;
	regs->dpp_config[cur].dst.y = dst.y;
	regs->dpp_config[cur].dst.w = dst.w;
	regs->dpp_config[cur].dst.h = dst.h;
}

int decon_set_cursor_win_config(struct decon_device *decon, int x, int y)
{
	int err_cnt = 0;
	struct decon_reg_data *regs;
	struct decon_mode_info psr;
	int ret = 0;

	DPU_EVENT_START();

	if (!decon->cursor.enabled)
		return 0;

	decon_set_cursor_pos(decon, x, y);

	mutex_lock(&decon->cursor.lock);

	decon_to_psr_info(decon, &psr);

	if (decon->state == DECON_STATE_OFF ||
		decon->state == DECON_STATE_TUI) {
		decon_info("decon%d: cursor win is not active :%d\n",
			decon->id, decon->state);
		ret = -EINVAL;
		goto end;
	}

	regs = &decon->cursor.regs;
	if (!regs) {
		decon_err("decon%d: cursor regs is null\n",
			decon->id);
		ret = -EINVAL;
		goto end;
	}

	if (!regs->is_cursor_win[regs->cursor_win]) {
		decon_err("decon%d: cursor win(%d) disable\n",
			decon->id, regs->cursor_win);
		ret = -EINVAL;
		goto end;
	}

	if (!decon->cursor.unmask) {
		decon_dbg("decon%d: cursor win(%d) update not ready\n",
			decon->id, regs->cursor_win);
		ret = -EINVAL;
		goto end;
	}

	if (psr.trig_mode == DECON_HW_TRIG)
		decon_reg_set_trigger(decon->id, &psr, DECON_TRIG_DISABLE);

	decon_reg_update_req_window_mask(decon->id, regs->cursor_win);

	dpu_cursor_win_update_config(decon, regs);

	DPU_EVENT_LOG_CURSOR(&decon->sd, regs);

	err_cnt = decon_set_cursor_dpp_config(decon, regs);
	if (err_cnt) {
		decon_err("decon%d: cursor win(%d) during dpp_config(err_cnt:%d)\n",
			decon->id, regs->cursor_win, err_cnt);
		if (psr.trig_mode == DECON_HW_TRIG)
			decon_reg_set_trigger(decon->id, &psr, DECON_TRIG_ENABLE);
		ret = -EINVAL;
		goto end;
	}

	/* set decon registers for each window */
	decon_reg_set_window_control(decon->id, regs->cursor_win,
				&regs->win_regs[regs->cursor_win],
				regs->win_regs[regs->cursor_win].winmap_state);

	decon_reg_all_win_shadow_update_req(decon->id);

	if (psr.trig_mode == DECON_HW_TRIG)
		decon_reg_set_trigger(decon->id, &psr, DECON_TRIG_ENABLE);

end:
	if (psr.trig_mode == DECON_HW_TRIG)
		decon->cursor.unmask = false;

	mutex_unlock(&decon->cursor.lock);

	DPU_EVENT_LOG(DPU_EVT_CURSOR_POS, &decon->sd, start);

	return ret;
}

void dpu_init_cursor_mode(struct decon_device *decon)
{
	decon->cursor.enabled = false;

	if (!IS_ENABLED(CONFIG_EXYNOS_CURSOR)) {
		decon_info("display doesn't support cursor async mode\n");
		return;
	}

	decon->cursor.enabled = true;
	decon_info("display supports cursor async mode\n");
}
