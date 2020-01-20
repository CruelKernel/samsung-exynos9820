/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * DPU Event log file for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/ktime.h>
#include <linux/debugfs.h>
#include <media/v4l2-subdev.h>
#include <video/mipi_display.h>

#include "decon.h"
#include "dsim.h"
#include "dpp.h"

/* logging a event related with DECON */
static inline void dpu_event_log_decon
	(dpu_event_t type, struct v4l2_subdev *sd, ktime_t time)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];

	if (time)
		log->time = time;
	else
		log->time = ktime_get();
	log->type = type;

	switch (type) {
	case DPU_EVT_DECON_SUSPEND:
	case DPU_EVT_DECON_RESUME:
	case DPU_EVT_ENTER_HIBER:
	case DPU_EVT_EXIT_HIBER:
		log->data.pm.pm_status = pm_runtime_active(decon->dev);
		log->data.pm.elapsed = ktime_sub(ktime_get(), log->time);
		break;
	case DPU_EVT_WIN_CONFIG:
	case DPU_EVT_TRIG_UNMASK:
	case DPU_EVT_TRIG_MASK:
	case DPU_EVT_FENCE_RELEASE:
	case DPU_EVT_DECON_FRAMEDONE:
		log->data.fence.timeline_value = atomic_read(&decon->fence.timeline);
		log->data.fence.timeline_max = atomic_read(&decon->fence.timeline);
		break;
	case DPU_EVT_WB_SW_TRIGGER:
		break;
	case DPU_EVT_TE_INTERRUPT:
	case DPU_EVT_UNDERRUN:
	case DPU_EVT_LINECNT_ZERO:
		break;
	case DPU_EVT_CURSOR_POS:	/* cursor async */
		log->data.cursor.xpos = decon->cursor.xpos;
		log->data.cursor.ypos = decon->cursor.ypos;
		log->data.cursor.elapsed = ktime_sub(ktime_get(), log->time);
		break;
	default:
		/* Any remaining types will be log just time and type */
		break;
	}
}

/* logging a event related with DSIM */
static inline void dpu_event_log_dsim
	(dpu_event_t type, struct v4l2_subdev *sd, ktime_t time)
{
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);
	struct decon_device *decon = get_decon_drvdata(dsim->id);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];

	if (time)
		log->time = time;
	else
		log->time = ktime_get();
	log->type = type;

	switch (type) {
	case DPU_EVT_DSIM_SUSPEND:
	case DPU_EVT_DSIM_RESUME:
	case DPU_EVT_ENTER_ULPS:
	case DPU_EVT_EXIT_ULPS:
		log->data.pm.pm_status = pm_runtime_active(dsim->dev);
		log->data.pm.elapsed = ktime_sub(ktime_get(), log->time);
		break;
	default:
		/* Any remaining types will be log just time and type */
		break;
	}
}

/* get decon's id used by dpp */
static int __get_decon_id_for_dpp(struct v4l2_subdev *sd)
{
	struct decon_device *decon;
	struct dpp_device *dpp = v4l2_get_subdevdata(sd);
	int idx;
	int ret = 0;

	for (idx = 0; idx < MAX_DECON_CNT; idx++) {
		decon = get_decon_drvdata(idx);
		if (!decon || IS_ERR_OR_NULL(decon->d.debug_event))
			continue;
		if (test_bit(dpp->id, &decon->prev_used_dpp))
			ret = decon->id;
	}

	return ret;
}

/* logging a event related with DPP */
static inline void dpu_event_log_dpp
	(dpu_event_t type, struct v4l2_subdev *sd, ktime_t time)
{
	struct decon_device *decon = get_decon_drvdata(__get_decon_id_for_dpp(sd));
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];
	struct dpp_device *dpp = v4l2_get_subdevdata(sd);

	if (time)
		log->time = time;
	else
		log->time = ktime_get();
	log->type = type;

	switch (type) {
	case DPU_EVT_DPP_SUSPEND:
	case DPU_EVT_DPP_RESUME:
		log->data.pm.pm_status = pm_runtime_active(dpp->dev);
		log->data.pm.elapsed = ktime_sub(ktime_get(), log->time);
		break;
	case DPU_EVT_DPP_FRAMEDONE:
	case DPU_EVT_DPP_STOP:
	case DPU_EVT_DMA_FRAMEDONE:
	case DPU_EVT_DMA_RECOVERY:
		log->data.dpp.id = dpp->id;
		log->data.dpp.done_cnt = dpp->d.done_count;
		break;
	case DPU_EVT_DPP_WINCON:
		log->data.dpp.id = dpp->id;
		memcpy(&log->data.dpp.src, &dpp->config->src, sizeof(struct decon_frame));
		memcpy(&log->data.dpp.dst, &dpp->config->dst, sizeof(struct decon_frame));
		break;
	default:
		log->data.dpp.id = dpp->id;
		break;
	}

	return;
}

/* If event are happend continuously, then ignore */
static bool dpu_event_ignore
	(dpu_event_t type, struct decon_device *decon)
{
	int latest = atomic_read(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log;
	int idx;

	/* Seek a oldest from current index */
	idx = (latest + DPU_EVENT_LOG_MAX - DECON_ENTER_HIBER_CNT) % DPU_EVENT_LOG_MAX;
	do {
		if (++idx >= DPU_EVENT_LOG_MAX)
			idx = 0;

		log = &decon->d.event_log[idx];
		if (log->type != type)
			return false;
	} while (latest != idx);

	return true;
}

/* ===== EXTERN APIs ===== */
/* Common API to log a event related with DECON/DSIM/DPP */
void DPU_EVENT_LOG(dpu_event_t type, struct v4l2_subdev *sd, ktime_t time)
{
	struct decon_device *decon = get_decon_drvdata(0);

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event))
		return;

	/* log a eventy softly */
	switch (type) {
	case DPU_EVT_TE_INTERRUPT:
	case DPU_EVT_UNDERRUN:
		/* If occurs continuously, skipped. It is a burden */
		if (dpu_event_ignore(type, decon))
			break;
	case DPU_EVT_BLANK:
	case DPU_EVT_UNBLANK:
	case DPU_EVT_ENTER_HIBER:
	case DPU_EVT_EXIT_HIBER:
	case DPU_EVT_DECON_SUSPEND:
	case DPU_EVT_DECON_RESUME:
	case DPU_EVT_LINECNT_ZERO:
	case DPU_EVT_TRIG_MASK:
	case DPU_EVT_TRIG_UNMASK:
	case DPU_EVT_FENCE_RELEASE:
	case DPU_EVT_DECON_FRAMEDONE:
	case DPU_EVT_DECON_FRAMEDONE_WAIT:
	case DPU_EVT_WIN_CONFIG:
	case DPU_EVT_WB_SW_TRIGGER:
	case DPU_EVT_DECON_SHUTDOWN:
	case DPU_EVT_RSC_CONFLICT:
	case DPU_EVT_DECON_FRAMESTART:
	case DPU_EVT_CURSOR_POS:	/* cursor async */
		dpu_event_log_decon(type, sd, time);
		break;
	case DPU_EVT_DSIM_FRAMEDONE:
	case DPU_EVT_ENTER_ULPS:
	case DPU_EVT_EXIT_ULPS:
	case DPU_EVT_DSIM_SHUTDOWN:
		dpu_event_log_dsim(type, sd, time);
		break;
	case DPU_EVT_DPP_FRAMEDONE:
	case DPU_EVT_DPP_STOP:
	case DPU_EVT_DPP_WINCON:
	case DPU_EVT_DMA_FRAMEDONE:
	case DPU_EVT_DMA_RECOVERY:
		dpu_event_log_dpp(type, sd, time);
		break;
	default:
		break;
	}

	if (decon->d.event_log_level == DPU_EVENT_LEVEL_LOW)
		return;

	/* additionally logging hardly */
	switch (type) {
	case DPU_EVT_ACT_VSYNC:
	case DPU_EVT_DEACT_VSYNC:
	case DPU_EVT_WB_SET_BUFFER:
	case DPU_EVT_DECON_SET_BUFFER:
		dpu_event_log_decon(type, sd, time);
		break;
	case DPU_EVT_DSIM_SUSPEND:
	case DPU_EVT_DSIM_RESUME:
		dpu_event_log_dsim(type, sd, time);
		break;
	case DPU_EVT_DPP_SUSPEND:
	case DPU_EVT_DPP_RESUME:
	case DPU_EVT_DPP_UPDATE_DONE:
	case DPU_EVT_DPP_SHADOW_UPDATE:
		dpu_event_log_dpp(type, sd, time);
	default:
		break;
	}
}

void DPU_EVENT_LOG_WINCON(struct v4l2_subdev *sd, struct decon_reg_data *regs)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];
	int win = 0;
	bool window_updated = false;

	log->time = ktime_get();
	log->type = DPU_EVT_UPDATE_HANDLER;

	for (win = 0; win < MAX_DECON_WIN; win++) {
		if (regs->win_regs[win].wincon & WIN_EN_F(win)) {
			memcpy(&log->data.reg.win_regs[win], &regs->win_regs[win],
				sizeof(struct decon_window_regs));
			memcpy(&log->data.reg.win_config[win], &regs->dpp_config[win],
				sizeof(struct decon_win_config));
		} else {
			log->data.reg.win_config[win].state =
						DECON_WIN_STATE_DISABLED;
		}
	}

	/* window update case : last window */
	win  = DECON_WIN_UPDATE_IDX;
	if (regs->dpp_config[win].state == DECON_WIN_STATE_UPDATE) {
		window_updated = true;
		memcpy(&log->data.reg.win_config[win], &regs->dpp_config[win],
				sizeof(struct decon_win_config));
	}

	if (window_updated) {
		log->data.reg.win.x = regs->dpp_config[win].dst.x;
		log->data.reg.win.y = regs->dpp_config[win].dst.y;
		log->data.reg.win.w = regs->dpp_config[win].dst.w;
		log->data.reg.win.h = regs->dpp_config[win].dst.h;
	} else {
		log->data.reg.win.x = 0;
		log->data.reg.win.y = 0;
		log->data.reg.win.w = decon->lcd_info->xres;
		log->data.reg.win.h = decon->lcd_info->yres;
	}
}

extern void *return_address(int);

/* Common API to log a event related with DSIM COMMAND */
void DPU_EVENT_LOG_CMD(struct v4l2_subdev *sd, u32 cmd_id, unsigned long data)
{
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);
	struct decon_device *decon = get_decon_drvdata(dsim->id);
	int idx, i;
	struct dpu_log *log;

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event))
		return;

	idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	log = &decon->d.event_log[idx];

	log->time = ktime_get();
	log->type = DPU_EVT_DSIM_COMMAND;
	log->data.cmd_buf.id = cmd_id;
	if (cmd_id == MIPI_DSI_DCS_LONG_WRITE)
		log->data.cmd_buf.buf = *(u8 *)(data);
	else
		log->data.cmd_buf.buf = (u8)data;

	for (i = 0; i < DPU_CALLSTACK_MAX; i++)
		log->data.cmd_buf.caller[i] = (void *)((size_t)return_address(i + 1));
}

/* cursor async */
void DPU_EVENT_LOG_CURSOR(struct v4l2_subdev *sd, struct decon_reg_data *regs)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];
	int win = 0;

	log->time = ktime_get();
	log->type = DPU_EVT_CURSOR_UPDATE;

	for (win = 0; win < MAX_DECON_WIN; win++) {
		if (regs->is_cursor_win[win] && regs->win_regs[win].wincon & WIN_EN_F(win)) {
			memcpy(&log->data.reg.win_regs[win], &regs->win_regs[win],
				sizeof(struct decon_window_regs));
			memcpy(&log->data.reg.win_config[win], &regs->dpp_config[win],
				sizeof(struct decon_win_config));
		} else {
			log->data.reg.win_config[win].state =
						DECON_WIN_STATE_DISABLED;
		}
	}
	win  = DECON_WIN_UPDATE_IDX;
	log->data.reg.win_config[win].state = DECON_WIN_STATE_DISABLED;
}

void DPU_EVENT_LOG_UPDATE_REGION(struct v4l2_subdev *sd,
		struct decon_frame *req_region, struct decon_frame *adj_region)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event))
		return;

	log->time = ktime_get();
	log->type = DPU_EVT_WINUP_UPDATE_REGION;

	memcpy(&log->data.winup.req_region, req_region, sizeof(struct decon_frame));
	memcpy(&log->data.winup.adj_region, adj_region, sizeof(struct decon_frame));
}

void DPU_EVENT_LOG_WINUP_FLAGS(struct v4l2_subdev *sd, bool need_update,
		bool reconfigure)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event))
		return;

	log->time = ktime_get();
	log->type = DPU_EVT_WINUP_FLAGS;

	log->data.winup.need_update = need_update;
	log->data.winup.reconfigure = reconfigure;
}

void DPU_EVENT_LOG_APPLY_REGION(struct v4l2_subdev *sd,
		struct decon_rect *apl_rect)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log = &decon->d.event_log[idx];

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event))
		return;

	log->time = ktime_get();
	log->type = DPU_EVT_WINUP_APPLY_REGION;

	log->data.winup.apl_region.x = apl_rect->left;
	log->data.winup.apl_region.y = apl_rect->top;
	log->data.winup.apl_region.w = apl_rect->right - apl_rect->left + 1;
	log->data.winup.apl_region.h = apl_rect->bottom - apl_rect->top + 1;
}

/* display logged events related with DECON */
void DPU_EVENT_SHOW(struct seq_file *s, struct decon_device *decon)
{
	int idx = atomic_read(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	struct dpu_log *log;
	int latest = idx;
	struct timeval tv;
	ktime_t prev_ktime;
	struct dsim_device *dsim;

	dsim = get_dsim_drvdata(decon->id);

	/* TITLE */
	seq_printf(s, "-------------------DECON%d EVENT LOGGER ----------------------\n",
			decon->id);
	seq_printf(s, "-- STATUS: Hibernation(%s) ",
			IS_ENABLED(CONFIG_DECON_HIBER) ? "on" : "off");
	seq_printf(s, "BlockMode(%s) ",
			IS_ENABLED(CONFIG_DECON_BLOCKING_MODE) ? "on" : "off");
	seq_printf(s, "Window_Update(%s)\n",
			IS_ENABLED(CONFIG_FB_WINDOW_UPDATE) ? "on" : "off");
	seq_printf(s, "-- Total underrun count(%d)\n", dsim->total_underrun_cnt);
	seq_printf(s, "-- Hibernation enter/exit count(%d %d)\n",
			decon->hiber.enter_cnt, decon->hiber.exit_cnt);
	seq_puts(s, "-------------------------------------------------------------\n");
	seq_printf(s, "%14s  %20s  %20s\n",
		"Time", "Event ID", "Remarks");
	seq_puts(s, "-------------------------------------------------------------\n");

	/* Seek a oldest from current index */
	idx = (idx + DPU_EVENT_LOG_MAX - DPU_EVENT_PRINT_MAX) % DPU_EVENT_LOG_MAX;
	prev_ktime = ktime_set(0, 0);
	do {
		if (++idx >= DPU_EVENT_LOG_MAX)
			idx = 0;

		/* Seek a index */
		log = &decon->d.event_log[idx];

		/* TIME */
		tv = ktime_to_timeval(log->time);
		seq_printf(s, "[%6ld.%06ld] ", tv.tv_sec, tv.tv_usec);

		/* If there is no timestamp, then exit directly */
		if (!tv.tv_sec)
			break;

		/* EVETN ID + Information */
		switch (log->type) {
		case DPU_EVT_BLANK:
			seq_printf(s, "%20s  %20s", "FB_BLANK", "-\n");
			break;
		case DPU_EVT_UNBLANK:
			seq_printf(s, "%20s  %20s", "FB_UNBLANK", "-\n");
			break;
		case DPU_EVT_ACT_VSYNC:
			seq_printf(s, "%20s  %20s", "ACT_VSYNC", "-\n");
			break;
		case DPU_EVT_DEACT_VSYNC:
			seq_printf(s, "%20s  %20s", "DEACT_VSYNC", "-\n");
			break;
		case DPU_EVT_WIN_CONFIG:
			seq_printf(s, "%20s  %20s", "WIN_CONFIG", "-\n");
			break;
		case DPU_EVT_TE_INTERRUPT:
			prev_ktime = ktime_sub(log->time, prev_ktime);
			seq_printf(s, "%20s  ", "TE_INTERRUPT");
			seq_printf(s, "time_diff=[%ld.%04lds]\n",
					ktime_to_timeval(prev_ktime).tv_sec,
					ktime_to_timeval(prev_ktime).tv_usec/100);
			/* Update for latest DPU_EVT_TE time */
			prev_ktime = log->time;
			break;
		case DPU_EVT_UNDERRUN:
			seq_printf(s, "%20s  %20s", "UNDER_RUN", "-\n");
			break;
		case DPU_EVT_DECON_FRAMEDONE:
			seq_printf(s, "%20s  %20s", "DECON_FRAME_DONE", "-\n");
			break;
		case DPU_EVT_DSIM_FRAMEDONE:
			seq_printf(s, "%20s  %20s", "DSIM_FRAME_DONE", "-\n");
			break;
		case DPU_EVT_RSC_CONFLICT:
			seq_printf(s, "%20s  %20s", "RSC_CONFLICT", "-\n");
			break;
		case DPU_EVT_UPDATE_HANDLER:
			seq_printf(s, "%20s  ", "UPDATE_HANDLER");
			seq_printf(s, "Partial Size (%d,%d,%d,%d)\n",
					log->data.reg.win.x,
					log->data.reg.win.y,
					log->data.reg.win.w,
					log->data.reg.win.h);
			break;
		case DPU_EVT_DSIM_COMMAND:
			seq_printf(s, "%20s  ", "DSIM_COMMAND");
			seq_printf(s, "id=0x%x, command=0x%x\n",
					log->data.cmd_buf.id,
					log->data.cmd_buf.buf);
			break;
		case DPU_EVT_TRIG_MASK:
			seq_printf(s, "%20s  %20s", "TRIG_MASK", "-\n");
			break;
		case DPU_EVT_TRIG_UNMASK:
			seq_printf(s, "%20s  %20s", "TRIG_UNMASK", "-\n");
			break;
		case DPU_EVT_FENCE_RELEASE:
			seq_printf(s, "%20s  %20s", "FENCE_RELEASE", "-\n");
			break;
		case DPU_EVT_DECON_SHUTDOWN:
			seq_printf(s, "%20s  %20s", "DECON_SHUTDOWN", "-\n");
			break;
		case DPU_EVT_DSIM_SHUTDOWN:
			seq_printf(s, "%20s  %20s", "DSIM_SHUTDOWN", "-\n");
			break;
		case DPU_EVT_DECON_FRAMESTART:
			seq_printf(s, "%20s  %20s", "DECON_FRAMESTART", "-\n");
			break;
		case DPU_EVT_DPP_WINCON:
			seq_printf(s, "%20s  ", "DPP_WINCON");
			seq_printf(s, "ID:%d, start= %d, done= %d\n",
					log->data.dpp.id,
					log->data.dpp.start_cnt,
					log->data.dpp.done_cnt);
			break;
		case DPU_EVT_DPP_FRAMEDONE:
			seq_printf(s, "%20s  ", "DPP_FRAMEDONE");
			seq_printf(s, "ID:%d, start=%d, done=%d\n",
					log->data.dpp.id,
					log->data.dpp.start_cnt,
					log->data.dpp.done_cnt);
			break;
		case DPU_EVT_DPP_STOP:
			seq_printf(s, "%20s  ", "DPP_STOP");
			seq_printf(s, "(id:%d)\n", log->data.dpp.id);
			break;
		case DPU_EVT_DPP_SUSPEND:
			seq_printf(s, "%20s  %20s", "DPP_SUSPEND", "-\n");
			break;
		case DPU_EVT_DPP_RESUME:
			seq_printf(s, "%20s  %20s", "DPP_RESUME", "-\n");
			break;
		case DPU_EVT_DECON_SUSPEND:
			seq_printf(s, "%20s  %20s", "DECON_SUSPEND", "-\n");
			break;
		case DPU_EVT_DECON_RESUME:
			seq_printf(s, "%20s  %20s", "DECON_RESUME", "-\n");
			break;
		case DPU_EVT_ENTER_HIBER:
			seq_printf(s, "%20s  ", "ENTER_HIBER");
			tv = ktime_to_timeval(log->data.pm.elapsed);
			seq_printf(s, "pm=%s, elapsed=[%ld.%03lds]\n",
					log->data.pm.pm_status ? "active " : "suspend",
					tv.tv_sec, tv.tv_usec/1000);
			break;
		case DPU_EVT_EXIT_HIBER:
			seq_printf(s, "%20s  ", "EXIT_HIBER");
			tv = ktime_to_timeval(log->data.pm.elapsed);
			seq_printf(s, "pm=%s, elapsed=[%ld.%03lds]\n",
					log->data.pm.pm_status ? "active " : "suspend",
					tv.tv_sec, tv.tv_usec/1000);
			break;
		case DPU_EVT_DSIM_SUSPEND:
			seq_printf(s, "%20s  %20s", "DSIM_SUSPEND", "-\n");
			break;
		case DPU_EVT_DSIM_RESUME:
			seq_printf(s, "%20s  %20s", "DSIM_RESUME", "-\n");
			break;
		case DPU_EVT_ENTER_ULPS:
			seq_printf(s, "%20s  ", "ENTER_ULPS");
			tv = ktime_to_timeval(log->data.pm.elapsed);
			seq_printf(s, "pm=%s, elapsed=[%ld.%03lds]\n",
					log->data.pm.pm_status ? "active " : "suspend",
					tv.tv_sec, tv.tv_usec/1000);
			break;
		case DPU_EVT_EXIT_ULPS:
			seq_printf(s, "%20s  ", "EXIT_ULPS");
			tv = ktime_to_timeval(log->data.pm.elapsed);
			seq_printf(s, "pm=%s, elapsed=[%ld.%03lds]\n",
					log->data.pm.pm_status ? "active " : "suspend",
					tv.tv_sec, tv.tv_usec/1000);
			break;
		case DPU_EVT_DMA_FRAMEDONE:
			seq_printf(s, "%20s  ", "DPP_FRAMEDONE");
			seq_printf(s, "ID:%d\n", log->data.dpp.id);
			break;
		case DPU_EVT_DMA_RECOVERY:
			seq_printf(s, "%20s  %20s", "DMA_FRAMEDONE", "-\n");
			break;
		default:
			seq_printf(s, "%20s  (%2d)\n", "NO_DEFINED", log->type);
			break;
		}
	} while (latest != idx);

	seq_puts(s, "-------------------------------------------------------------\n");

	return;
}

static int decon_debug_event_show(struct seq_file *s, void *unused)
{
	struct decon_device *decon = s->private;
	DPU_EVENT_SHOW(s, decon);
	return 0;
}

static int decon_debug_event_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_event_show, inode->i_private);
}

static const struct file_operations decon_event_fops = {
	.open = decon_debug_event_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int decon_debug_dump_show(struct seq_file *s, void *unused)
{
	struct decon_device *decon = s->private;

	if (decon->state != DECON_STATE_ON) {
		decon_info("%s: decon is not ON(%d)\n", __func__, decon->state);
		return 0;
	}
	decon_dump(decon);
	return 0;
}

static int decon_debug_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_dump_show, inode->i_private);
}

static const struct file_operations decon_dump_fops = {
	.open = decon_debug_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int decon_debug_bts_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%u\n", dpu_bts_log_level);

	return 0;
}

static int decon_debug_bts_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_bts_show, inode->i_private);
}

static ssize_t decon_debug_bts_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &dpu_bts_log_level);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_bts_fops = {
	.open = decon_debug_bts_open,
	.write = decon_debug_bts_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int decon_debug_win_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%u\n", win_update_log_level);

	return 0;
}

static int decon_debug_win_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_win_show, inode->i_private);
}

static ssize_t decon_debug_win_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &win_update_log_level);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_win_fops = {
	.open = decon_debug_win_open,
	.write = decon_debug_win_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int decon_systrace_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%u\n", decon_systrace_enable);
	return 0;
}

static int decon_systrace_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_systrace_show, inode->i_private);
}

static ssize_t decon_systrace_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &decon_systrace_enable);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_systrace_fops = {
	.open = decon_systrace_open,
	.write = decon_systrace_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

#if defined(CONFIG_DSIM_CMD_TEST)
static int decon_debug_cmd_show(struct seq_file *s, void *unused)
{
	return 0;
}

static int decon_debug_cmd_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_cmd_show, inode->i_private);
}

static ssize_t decon_debug_cmd_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;
	unsigned int cmd;
	struct dsim_device *dsim;
	u32 id, d1;
	unsigned long d0;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &cmd);
	if (ret < 0)
		goto out;

	dsim = get_dsim_drvdata(0);

	switch (cmd) {
	case 1:
		id = MIPI_DSI_DCS_SHORT_WRITE;
		d0 = (unsigned long)SEQ_DISPLAY_ON[0];
		d1 = 0;
		break;
	case 2:
		id = MIPI_DSI_DCS_SHORT_WRITE;
		d0 = (unsigned long)SEQ_DISPLAY_OFF[0];
		d1 = 0;
		break;
	case 3:
		id = MIPI_DSI_DCS_SHORT_WRITE;
		d0 = (unsigned long)SEQ_ALLPOFF[0];
		d1 = 0;
		break;
	case 4:
		id = MIPI_DSI_DCS_SHORT_WRITE;
		d0 = (unsigned long)SEQ_ALLPON[0];
		d1 = 0;
		break;
	case 5:
		id = MIPI_DSI_DCS_LONG_WRITE;
		d0 = (unsigned long)SEQ_ESD_FG;
		d1 = ARRAY_SIZE(SEQ_ESD_FG);
		break;
	case 6:
		id = MIPI_DSI_DCS_LONG_WRITE;
		d0 = (unsigned long)SEQ_TEST_KEY_OFF_F0;
		d1 = ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0);
		break;
	case 7:
		id = MIPI_DSI_DCS_LONG_WRITE;
		d0 = (unsigned long)SEQ_TEST_KEY_OFF_F1;
		d1 = ARRAY_SIZE(SEQ_TEST_KEY_OFF_F1);
		break;
	default:
		dsim_info("unsupported command(%d)\n", cmd);
		goto out;
	}

	ret = dsim_write_data(dsim, id, d0, d1);
	if (ret < 0) {
		decon_err("failed to write DSIM command(0x%lx)\n",
				(id == MIPI_DSI_DCS_LONG_WRITE) ?
				*(u8 *)(d0) : d0);
		goto out;
	}

	decon_info("success to write DSIM command(0x%lx, %d)\n",
			(id == MIPI_DSI_DCS_LONG_WRITE) ?
			*(u8 *)(d0) : d0, d1);
out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_cmd_fops = {
	.open = decon_debug_cmd_open,
	.write = decon_debug_cmd_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif

static int decon_debug_cmd_lp_ref_show(struct seq_file *s, void *unused)
{
	struct dsim_device *dsim = get_dsim_drvdata(0);
	int i;

	/* DSU_MODE_1 is used in stead of 1 in MCD */
	seq_printf(s, "%u\n", dsim->lcd_info.mres_mode - 1);

	for (i = 0; i < dsim->lcd_info.dt_lcd_mres.mres_number; i++)
		seq_printf(s, "%u\n", dsim->lcd_info.cmd_underrun_lp_ref[i]);

	return 0;
}

static int decon_debug_cmd_lp_ref_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_cmd_lp_ref_show, inode->i_private);
}

static ssize_t decon_debug_cmd_lp_ref_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;
	unsigned int cmd_lp_ref;
	struct dsim_device *dsim;
	int idx;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &cmd_lp_ref);
	if (ret < 0)
		goto out;

	dsim = get_dsim_drvdata(0);

	idx = dsim->lcd_info.mres_mode - 1;
	dsim->lcd_info.cmd_underrun_lp_ref[idx] = cmd_lp_ref;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_cmd_lp_ref_fops = {
	.open = decon_debug_cmd_lp_ref_open,
	.write = decon_debug_cmd_lp_ref_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int decon_debug_rec_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "VGF0[%u] VGF1[%u]\n",
			get_dpp_drvdata(IDMA_VGF0)->d.recovery_cnt,
			get_dpp_drvdata(IDMA_VGF1)->d.recovery_cnt);
	return 0;
}

static int decon_debug_rec_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_rec_show, inode->i_private);
}

static ssize_t decon_debug_rec_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	return count;
}

static const struct file_operations decon_rec_fops = {
	.open = decon_debug_rec_open,
	.write = decon_debug_rec_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

int decon_create_debugfs(struct decon_device *decon)
{
	char name[MAX_NAME_SIZE];
	int ret = 0;

	if (!decon->id) {
		decon->d.debug_root = debugfs_create_dir("decon", NULL);
		if (!decon->d.debug_root) {
			decon_err("failed to create debugfs root directory.\n");
			return -ENOENT;
		}
	}

	if (decon->id == 1 || decon->id == 2)
		decon->d.debug_root = decon_drvdata[0]->d.debug_root;

	snprintf(name, MAX_NAME_SIZE, "event%d", decon->id);
	atomic_set(&decon->d.event_log_idx, -1);
	decon->d.debug_event = debugfs_create_file(name, 0444,
			decon->d.debug_root, decon, &decon_event_fops);
	if (!decon->d.debug_event) {
		decon_err("failed to create debugfs file(%d)\n", decon->id);
		ret = -ENOENT;
		goto err_debugfs;
	}

	snprintf(name, MAX_NAME_SIZE, "dump%d", decon->id);
	decon->d.debug_dump = debugfs_create_file(name, 0444,
			decon->d.debug_root, decon, &decon_dump_fops);
	if (!decon->d.debug_dump) {
		decon_err("failed to create SFR dump debugfs file(%d)\n",
				decon->id);
		ret = -ENOENT;
		goto err_debugfs;
	}

	if (decon->id == 0) {
		decon->d.debug_bts = debugfs_create_file("bts_log", 0444,
				decon->d.debug_root, NULL, &decon_bts_fops);
		if (!decon->d.debug_bts) {
			decon_err("failed to create BTS log level file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
		decon->d.debug_win = debugfs_create_file("win_update_log", 0444,
				decon->d.debug_root, NULL, &decon_win_fops);
		if (!decon->d.debug_win) {
			decon_err("failed to create win update log level file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
		decon->d.debug_systrace = debugfs_create_file("decon_systrace", 0444,
				decon->d.debug_root, NULL, &decon_systrace_fops);
		if (!decon->d.debug_systrace) {
			decon_err("failed to create decon_systrace file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
#if defined(CONFIG_DSIM_CMD_TEST)
		decon->d.debug_cmd = debugfs_create_file("cmd", 0444,
				decon->d.debug_root, NULL, &decon_cmd_fops);
		if (!decon->d.debug_cmd) {
			decon_err("failed to create cmd_rw file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
#endif
		decon->d.debug_recovery_cnt = debugfs_create_file("recovery_cnt",
				0444, decon->d.debug_root, NULL, &decon_rec_fops);
		if (!decon->d.debug_recovery_cnt) {
			decon_err("failed to create recovery_cnt file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
		decon->d.debug_cmd_lp_ref = debugfs_create_file("cmd_lp_ref",
				0444, decon->d.debug_root, NULL, &decon_cmd_lp_ref_fops);
		if (!decon->d.debug_cmd_lp_ref) {
			decon_err("failed to create cmd_lp_ref file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
	}

	return 0;

err_debugfs:
	debugfs_remove_recursive(decon->d.debug_root);
	return ret;
}

void decon_destroy_debugfs(struct decon_device *decon)
{
	if (decon->d.debug_root)
		debugfs_remove(decon->d.debug_root);
	if (decon->d.debug_event)
		debugfs_remove(decon->d.debug_event);
}
