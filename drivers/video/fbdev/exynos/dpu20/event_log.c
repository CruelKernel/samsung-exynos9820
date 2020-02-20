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
#ifdef CONFIG_SUPPORT_RDX_DUMP
#include <linux/bootmem.h>
#include <linux/slab.h>
#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif
#endif

#include "decon.h"
#include "dsim.h"
#include "dpp.h"

#if defined(CONFIG_EXYNOS_COMMON_PANEL)
const char dpu_event_dtype_name[MAX_DPU_EVT_DTYPE][MAX_EVENT_NAME_SIZE] = {
	[DPU_EVT_DTYPE_NONE] = "NONE",
	[DPU_EVT_DTYPE_DPP] = "DPP",
	[DPU_EVT_DTYPE_UPDATE_REG] = "UPDATE_REG",
	[DPU_EVT_DTYPE_WIN_REG] = "WIN_REG",
	[DPU_EVT_DTYPE_WIN_CONFIG] = "WIN_CONFIG",
	[DPU_EVT_DTYPE_WIN] = "WIN",
	[DPU_EVT_DTYPE_CMD_BUF] = "CMD_BUF",
	[DPU_EVT_DTYPE_PM] = "PM",
	[DPU_EVT_DTYPE_FENCE] = "FENCE",
	[DPU_EVT_DTYPE_CURSOR] = "CURSOR",
	[DPU_EVT_DTYPE_WINUP] = "WINUP",
};

const char dpu_event_name[DPU_EVT_MAX][MAX_EVENT_NAME_SIZE] = {
	/* Related with FB interface */
	[DPU_EVT_BLANK] = "BLANK",
	[DPU_EVT_UNBLANK] = "UNBLANK",
	[DPU_EVT_ACT_VSYNC] = "ACT_VSYNC",
	[DPU_EVT_DEACT_VSYNC] = "DEACT_VSYNC",
	[DPU_EVT_WIN_CONFIG] = "WIN_CONFIG",
	[DPU_EVT_DISP_INFO] = "DISP_INFO",

	/* Related with interrupt */
	[DPU_EVT_TE_INTERRUPT] = "TE_INTERRUPT",
	[DPU_EVT_UNDERRUN] = "UNDERRUN",
	[DPU_EVT_DECON_FRAMEDONE] = "DECON_FRAMEDONE",
	[DPU_EVT_DSIM_FRAMEDONE] = "DSIM_FRAMEDONE",
	[DPU_EVT_RSC_CONFLICT] = "RSC_CONFLICT",

	/* Related with async event */
	[DPU_EVT_UPDATE_HANDLER] = "UPDATE_HANDLER",

	[DPU_EVT_UPDATE_HANDLER_WIN_REG_0] = "WIN_REG_0",
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_1] = "WIN_REG_1",
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_2] = "WIN_REG_2",
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_3] = "WIN_REG_3",
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_4] = "WIN_REG_4",
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_5] = "WIN_REG_5",
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_6] = "WIN_REG_6",

	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_0] = "WIN_CONFIG_0",
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_1] = "WIN_CONFIG_1",
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_2] = "WIN_CONFIG_2",
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_3] = "WIN_CONFIG_3",
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_4] = "WIN_CONFIG_4",
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_5] = "WIN_CONFIG_5",
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_6] = "WIN_CONFIG_6",

	[DPU_EVT_UPDATE_HANDLER_WIN] = "WIN",

	[DPU_EVT_DSIM_COMMAND] = "DSIM_COMMAND",
	[DPU_EVT_TRIG_MASK] = "TRIG_MASK",
	[DPU_EVT_TRIG_UNMASK] = "TRIG_UNMASK",
	[DPU_EVT_FENCE_RELEASE] = "FENCE_RELEASE",
	[DPU_EVT_DECON_FRAMEDONE_WAIT] = "DECON_FRAMEDONE_WAIT",
	[DPU_EVT_DECON_SHUTDOWN] = "DECON_SHUTDOWN",
	[DPU_EVT_DSIM_SHUTDOWN] = "DSIM_SHUTDOWN",
	[DPU_EVT_DECON_FRAMESTART] = "DECON_FRAMESTART",

	/* Related with DPP */
	[DPU_EVT_DPP_WINCON] = "DPP_WINCON",
	[DPU_EVT_DPP_FRAMEDONE] = "DPP_FRAMEDONE",
	[DPU_EVT_DPP_STOP] = "DPP_STOP",
	[DPU_EVT_DPP_UPDATE_DONE] = "DPP_UPDATE_DONE",
	[DPU_EVT_DPP_SHADOW_UPDATE] = "DPP_SHADOW_UPDATE",
	[DPU_EVT_DPP_SUSPEND] = "DPP_SUSPEND",
	[DPU_EVT_DPP_RESUME] = "DPP_RESUME",

	/* Related with PM */
	[DPU_EVT_DECON_SUSPEND] = "DECON_SUSPEND",
	[DPU_EVT_DECON_RESUME] = "DECON_RESUME",
	[DPU_EVT_ENTER_HIBER] = "ENTER_HIBER",
	[DPU_EVT_EXIT_HIBER] = "EXIT_HIBER",
	[DPU_EVT_DSIM_SUSPEND] = "DSIM_SUSPEND",
	[DPU_EVT_DSIM_RESUME] = "DSIM_RESUME",
	[DPU_EVT_ENTER_ULPS] = "ENTER_ULPS",
	[DPU_EVT_EXIT_ULPS] = "EXIT_ULPS",

	[DPU_EVT_LINECNT_ZERO] = "LINECNT_ZERO",

	/* write-back events */
	[DPU_EVT_WB_SET_BUFFER] = "WB_SET_BUFFER",
	[DPU_EVT_WB_SW_TRIGGER] = "WB_SW_TRIGGER",

	[DPU_EVT_DMA_FRAMEDONE] = "DMA_FRAMEDONE",
	[DPU_EVT_DMA_RECOVERY] = "DMA_RECOVERY",

	[DPU_EVT_DECON_SET_BUFFER] = "DECON_SET_BUFFER",
	/* cursor async */
	[DPU_EVT_CURSOR_POS] = "CURSOR_POS",
	[DPU_EVT_CURSOR_UPDATE] = "CURSOR_UPDATE",

	/* window update */
	[DPU_EVT_WINUP_UPDATE_REGION] = "WINUP_UPDATE_REGION",
	[DPU_EVT_WINUP_FLAGS] = "WINUP_FLAGS",
	[DPU_EVT_WINUP_APPLY_REGION] = "WINUP_APPLY_REGION",

	[DPU_EVT_DOZE] = "DOZE",
	[DPU_EVT_DOZE_SUSPEND] = "DOZE_SUSPEND",
};

dpu_event_t dpu_event_dtype[DPU_EVT_MAX] = {
	/* Related with FB interface */
	[DPU_EVT_BLANK] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_UNBLANK] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_ACT_VSYNC] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_DEACT_VSYNC] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_WIN_CONFIG] = DPU_EVT_DTYPE_FENCE,
	[DPU_EVT_DISP_INFO] = DPU_EVT_DTYPE_NONE,

	/* Related with interrupt */
	[DPU_EVT_TE_INTERRUPT] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_UNDERRUN] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_DECON_FRAMEDONE] = DPU_EVT_DTYPE_FENCE,
	[DPU_EVT_DSIM_FRAMEDONE] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_RSC_CONFLICT] = DPU_EVT_DTYPE_NONE,

	/* Related with async event */
	[DPU_EVT_UPDATE_HANDLER] = DPU_EVT_DTYPE_UPDATE_REG,

	[DPU_EVT_UPDATE_HANDLER_WIN_REG_0] = DPU_EVT_DTYPE_WIN_REG,
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_1] = DPU_EVT_DTYPE_WIN_REG,
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_2] = DPU_EVT_DTYPE_WIN_REG,
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_3] = DPU_EVT_DTYPE_WIN_REG,
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_4] = DPU_EVT_DTYPE_WIN_REG,
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_5] = DPU_EVT_DTYPE_WIN_REG,
	[DPU_EVT_UPDATE_HANDLER_WIN_REG_6] = DPU_EVT_DTYPE_WIN_REG,

	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_0] = DPU_EVT_DTYPE_WIN_CONFIG,
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_1] = DPU_EVT_DTYPE_WIN_CONFIG,
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_2] = DPU_EVT_DTYPE_WIN_CONFIG,
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_3] = DPU_EVT_DTYPE_WIN_CONFIG,
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_4] = DPU_EVT_DTYPE_WIN_CONFIG,
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_5] = DPU_EVT_DTYPE_WIN_CONFIG,
	[DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_6] = DPU_EVT_DTYPE_WIN_CONFIG,

	[DPU_EVT_UPDATE_HANDLER_WIN] = DPU_EVT_DTYPE_WIN,

	[DPU_EVT_DSIM_COMMAND] = DPU_EVT_DTYPE_CMD_BUF,
	[DPU_EVT_TRIG_MASK] = DPU_EVT_DTYPE_FENCE,
	[DPU_EVT_TRIG_UNMASK] = DPU_EVT_DTYPE_FENCE,
	[DPU_EVT_FENCE_RELEASE] = DPU_EVT_DTYPE_FENCE,
	[DPU_EVT_DECON_FRAMEDONE_WAIT] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_DECON_SHUTDOWN] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_DSIM_SHUTDOWN] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_DECON_FRAMESTART] = DPU_EVT_DTYPE_NONE,

	/* Related with DPP */
	[DPU_EVT_DPP_WINCON] = DPU_EVT_DTYPE_DPP,
	[DPU_EVT_DPP_FRAMEDONE] = DPU_EVT_DTYPE_DPP,
	[DPU_EVT_DPP_STOP] = DPU_EVT_DTYPE_DPP,
	[DPU_EVT_DPP_UPDATE_DONE] = DPU_EVT_DTYPE_DPP,
	[DPU_EVT_DPP_SHADOW_UPDATE] = DPU_EVT_DTYPE_DPP,
	[DPU_EVT_DPP_SUSPEND] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_DPP_RESUME] = DPU_EVT_DTYPE_PM,

	/* Related with PM */
	[DPU_EVT_DECON_SUSPEND] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_DECON_RESUME] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_ENTER_HIBER] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_EXIT_HIBER] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_DSIM_SUSPEND] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_DSIM_RESUME] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_ENTER_ULPS] = DPU_EVT_DTYPE_PM,
	[DPU_EVT_EXIT_ULPS] = DPU_EVT_DTYPE_PM,

	[DPU_EVT_LINECNT_ZERO] = DPU_EVT_DTYPE_NONE,

	/* write-back events */
	[DPU_EVT_WB_SET_BUFFER] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_WB_SW_TRIGGER] = DPU_EVT_DTYPE_NONE,

	[DPU_EVT_DMA_FRAMEDONE] = DPU_EVT_DTYPE_DPP,
	[DPU_EVT_DMA_RECOVERY] = DPU_EVT_DTYPE_DPP,

	[DPU_EVT_DECON_SET_BUFFER] = DPU_EVT_DTYPE_NONE,
	/* cursor async */
	[DPU_EVT_CURSOR_POS] = DPU_EVT_DTYPE_CURSOR,
	[DPU_EVT_CURSOR_UPDATE] = DPU_EVT_DTYPE_UPDATE_REG,

	/* window update */
	[DPU_EVT_WINUP_UPDATE_REGION] = DPU_EVT_DTYPE_WINUP,
	[DPU_EVT_WINUP_FLAGS] = DPU_EVT_DTYPE_WINUP,
	[DPU_EVT_WINUP_APPLY_REGION] = DPU_EVT_DTYPE_WINUP,

	[DPU_EVT_DOZE] = DPU_EVT_DTYPE_NONE,
	[DPU_EVT_DOZE_SUSPEND] = DPU_EVT_DTYPE_NONE,
};

static inline void dpu_event_log_header(struct decon_device *decon, u32 log_cnt)
{
	struct dpu_log_header *header;

	if (IS_ERR_OR_NULL(decon->d.event_log_header))
		return;

	header = decon->d.event_log_header;
	header->magic = DPU_LOG_VALID_MARK;
	header->header_size = (u32)sizeof(struct dpu_log_header);
	header->log_size = (u32)sizeof(struct dpu_log) * log_cnt;
	header->log_cnt = log_cnt;
	header->name_len = MAX_EVENT_NAME_SIZE;

	header->event_name_size = (u32)sizeof(dpu_event_name);
	header->event_dtype_name_size = (u32)sizeof(dpu_event_dtype_name);
	header->event_dtype_size = (u32)sizeof(dpu_event_dtype);

	memcpy(header->event_name, dpu_event_name, sizeof(dpu_event_name));
	memcpy(header->event_dtype_name, dpu_event_dtype_name, sizeof(dpu_event_dtype_name));
	memcpy(header->event_dtype, dpu_event_dtype, sizeof(dpu_event_dtype));
}
#endif /* CONFIG_EXYNOS_COMMON_PANEL */

/* DPU fence event logger function */
void DPU_F_EVT_LOG(dpu_f_evt_t type, struct v4l2_subdev *sd,
		struct dpu_fence_info *fence_info)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	struct dpu_fence_log *log;
	int idx;

	if (!decon || IS_ERR_OR_NULL(decon->d.f_evt_log))
		return;

	idx = atomic_inc_return(&decon->d.f_evt_log_idx) % decon->d.f_evt_log_cnt;
	log = &decon->d.f_evt_log[idx];

	log->time = ktime_get();
	log->type = type;
	memcpy(&log->fence_info, fence_info, sizeof(struct dpu_fence_info));
}

/* logging a event related with DECON */
static inline void dpu_event_log_decon
	(dpu_event_t type, struct v4l2_subdev *sd, ktime_t time)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpu_log *log;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;

#if defined(CONFIG_SUPPORT_KERNEL_4_9)
	if (time.tv64)
#else
	if (time)
#endif
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
#if defined(CONFIG_SUPPORT_LEGACY_FENCE)
		log->data.fence.timeline_value = decon->timeline->value;
		log->data.fence.timeline_max = decon->timeline_max;
#else
		log->data.fence.timeline_value = atomic_read(&decon->fence.timeline);
		log->data.fence.timeline_max = atomic_read(&decon->fence.timeline);
#endif
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
	int idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpu_log *log;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;

#if defined(CONFIG_SUPPORT_KERNEL_4_9)
	if (time.tv64)
#else
	if (time)
#endif
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
	int decon_cnt;

	decon_cnt = get_decon_drvdata(0)->dt.decon_cnt;

	for (idx = 0; idx < decon_cnt; idx++) {
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
	int idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpp_device *dpp = v4l2_get_subdevdata(sd);
	struct dpu_log *log;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;

#if defined(CONFIG_SUPPORT_KERNEL_4_9)
	if (time.tv64)
#else
	if (time)
#endif
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
		log->data.dpp.comp = dpp->dpp_config->config.compression;
		log->data.dpp.rot = dpp->dpp_config->config.dpp_parm.rot;
		log->data.dpp.hdr_std = dpp->dpp_config->config.dpp_parm.hdr_std;
		memcpy(&log->data.dpp.src, &dpp->dpp_config->config.src, sizeof(struct decon_frame));
		memcpy(&log->data.dpp.dst, &dpp->dpp_config->config.dst, sizeof(struct decon_frame));
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
	int latest = atomic_read(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpu_log *log;
	int idx;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return true;

	/* Seek a oldest from current index */
	idx = (latest + decon->d.event_log_cnt - DPU_EVENT_KEEP_CNT) % decon->d.event_log_cnt;
	do {
		if (++idx >= decon->d.event_log_cnt)
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

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event) ||
			IS_ERR_OR_NULL(decon->d.event_log))
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
	struct dpu_log *log;
	int idx;
	int win = 0;
	bool window_updated = false;
	ktime_t time = ktime_get();

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	for (win = 0; win < decon->dt.max_win; win++) {
		if (!(regs->win_regs[win].wincon & WIN_EN_F(win)))
			continue;

		idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
		log = &decon->d.event_log[idx];
		log->magic = DPU_LOG_VALID_MARK;
		log->time = time;
		log->type = DPU_EVT_UPDATE_HANDLER_WIN_REG_0 + win;
		memcpy(&log->data.win_regs, &regs->win_regs[win],
				sizeof(struct decon_window_regs));
	}

	for (win = 0; win < decon->dt.max_win; win++) {
		if (!(regs->win_regs[win].wincon & WIN_EN_F(win)))
			continue;

		idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
		log = &decon->d.event_log[idx];
		log->magic = DPU_LOG_VALID_MARK;
		log->time = time;
		log->type = DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_0 + win;
		memcpy(&log->data.win_config, &regs->dpp_config[win],
				sizeof(struct decon_win_config));
	}

	win  = DECON_WIN_UPDATE_IDX;

	if ((regs->dpp_config[win].state == DECON_WIN_STATE_UPDATE) ||
		(regs->dpp_config[win].state == DECON_WIN_STATE_MRESOL) ||
		(decon->dt.out_type == DECON_OUT_WB)) {
		/* window update case : last window
			OR
	       write-back case : last window
		*/
		idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
		log = &decon->d.event_log[idx];
		log->magic = DPU_LOG_VALID_MARK;
		log->time = time;
		log->type = DPU_EVT_UPDATE_HANDLER_WIN_CONFIG_0 + win;

		window_updated = true;
		memcpy(&log->data.win_config, &regs->dpp_config[win],
				sizeof(struct decon_win_config));
	}

	idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;
	log->time = time;
	log->type = DPU_EVT_UPDATE_HANDLER_WIN;

	if (window_updated) {
		log->data.win.x = regs->dpp_config[win].dst.x;
		log->data.win.y = regs->dpp_config[win].dst.y;
		log->data.win.w = regs->dpp_config[win].dst.w;
		log->data.win.h = regs->dpp_config[win].dst.h;
	} else {
		log->data.win.x = 0;
		log->data.win.y = 0;
		log->data.win.w = decon->lcd_info->xres;
		log->data.win.h = decon->lcd_info->yres;
	}
}

#if 0
void DPU_EVENT_LOG_WINCON(struct v4l2_subdev *sd, struct decon_reg_data *regs)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	struct dpu_log *log;
	//int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	int idx;
	//int idx = atomic_add_return(MAX_DECON_WIN * 2 + 2, &decon->d.event_log_idx);
	int win = 0;
	bool window_updated = false;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	for (i = 0; i < MAX_DECON_WIN * 2 + 2; i++) {
		log = &decon->d.event_log[idx];
		log->magic = DPU_LOG_VALID_MARK;
		log->time = ktime_get();
		log->type = DPU_EVT_UPDATE_HANDLER_WIN_REG_0 + i;
	}

	for (win = 0; win < decon->dt.max_win; win++) {

	}
	/*
	for (win = 0; win < decon->dt.max_win; win++) {
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
	*/

	/* window update case : last window */
	win  = DECON_WIN_UPDATE_IDX;
	if (regs->dpp_config[win].state == DECON_WIN_STATE_UPDATE) {
		window_updated = true;
		memcpy(&log->data.reg.win_config[win], &regs->dpp_config[win],
				sizeof(struct decon_win_config));
	}

	/* write-back case : last window */
	if (decon->dt.out_type == DECON_OUT_WB)
		memcpy(&log->data.reg.win_config[win], &regs->dpp_config[win],
				sizeof(struct decon_win_config));

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
#endif

extern void *return_address(int);

/* Common API to log a event related with DSIM COMMAND */
void DPU_EVENT_LOG_CMD(struct v4l2_subdev *sd, u32 cmd_id, unsigned long data)
{
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);
	struct decon_device *decon = get_decon_drvdata(dsim->id);
	int idx, i;
	struct dpu_log *log;

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event) ||
			IS_ERR_OR_NULL(decon->d.event_log))
		return;

	idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;

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
#if 0
void DPU_EVENT_LOG_CURSOR(struct v4l2_subdev *sd, struct decon_reg_data *regs)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	struct dpu_log *log;
	int idx = atomic_inc_return(&decon->d.event_log_idx) % DPU_EVENT_LOG_MAX;
	int win = 0;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;
	log->time = ktime_get();
	log->type = DPU_EVT_CURSOR_UPDATE;

	for (win = 0; win < decon->dt.max_win; win++) {
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
#else
void DPU_EVENT_LOG_CURSOR(struct v4l2_subdev *sd, struct decon_reg_data *regs)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	struct dpu_log *log;
	int idx;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;
	idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;
	log->time = ktime_get();
	log->type = DPU_EVT_CURSOR_UPDATE;

	if (regs->is_cursor_win[regs->cursor_win]) {
		memcpy(&log->data.win_regs, &regs->win_regs[regs->cursor_win],
			sizeof(struct decon_window_regs));
		memcpy(&log->data.win_config, &regs->dpp_config[regs->cursor_win],
			sizeof(struct decon_win_config));
	} else {
		log->data.win_config.state = DECON_WIN_STATE_DISABLED;
	}
}
#endif

void DPU_EVENT_LOG_UPDATE_REGION(struct v4l2_subdev *sd,
		struct decon_frame *req_region, struct decon_frame *adj_region)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpu_log *log;

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event) ||
			IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;
	log->time = ktime_get();
	log->type = DPU_EVT_WINUP_UPDATE_REGION;

	memcpy(&log->data.winup.req_region, req_region, sizeof(struct decon_frame));
	memcpy(&log->data.winup.adj_region, adj_region, sizeof(struct decon_frame));
}

void DPU_EVENT_LOG_WINUP_FLAGS(struct v4l2_subdev *sd, bool need_update,
		bool reconfigure)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	struct dpu_log *log;
	int idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event) ||
			IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;

	log->time = ktime_get();
	log->type = DPU_EVT_WINUP_FLAGS;

	log->data.winup.need_update = need_update;
	log->data.winup.reconfigure = reconfigure;
}

void DPU_EVENT_LOG_APPLY_REGION(struct v4l2_subdev *sd,
		struct decon_rect *apl_rect)
{
	struct decon_device *decon = container_of(sd, struct decon_device, sd);
	int idx = atomic_inc_return(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpu_log *log;

	if (!decon || IS_ERR_OR_NULL(decon->d.debug_event) ||
			IS_ERR_OR_NULL(decon->d.event_log))
		return;

	log = &decon->d.event_log[idx];
	log->magic = DPU_LOG_VALID_MARK;

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
	int idx = atomic_read(&decon->d.event_log_idx) % decon->d.event_log_cnt;
	struct dpu_log *log;
	int latest = idx;
	struct timeval tv;
	ktime_t prev_ktime;
	struct dsim_device *dsim;

	if (IS_ERR_OR_NULL(decon->d.event_log))
		return;

	if (!decon->id)
		dsim = get_dsim_drvdata(decon->id);

	/* TITLE */
	seq_printf(s, "-------------------DECON%d EVENT LOGGER ----------------------\n",
			decon->id);
	seq_printf(s, "-- STATUS: Hibernation(%s) ",
			IS_ENABLED(CONFIG_EXYNOS_HIBERNATION) ? "on" : "off");
	seq_printf(s, "BlockMode(%s) ",
			IS_ENABLED(CONFIG_EXYNOS_BLOCK_MODE) ? "on" : "off");
	seq_printf(s, "Window_Update(%s)\n",
			decon->win_up.enabled ? "on" : "off");
	if (!decon->id)
		seq_printf(s, "-- Total underrun count(%d)\n",
				dsim->total_underrun_cnt);
	seq_printf(s, "-- Hibernation enter/exit count(%d %d)\n",
			decon->hiber.enter_cnt, decon->hiber.exit_cnt);
	seq_puts(s, "-------------------------------------------------------------\n");
	seq_printf(s, "%14s  %20s  %20s\n",
		"Time", "Event ID", "Remarks");
	seq_puts(s, "-------------------------------------------------------------\n");

	/* Seek a oldest from current index */
	idx = (idx + decon->d.event_log_cnt / 2) % decon->d.event_log_cnt;
	prev_ktime = ktime_set(0, 0);
	do {
		if (++idx >= decon->d.event_log_cnt)
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
			/*
			seq_printf(s, "Partial Size (%d,%d,%d,%d)\n",
					log->data.reg.win.x,
					log->data.reg.win.y,
					log->data.reg.win.w,
					log->data.reg.win.h);
			*/
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
			seq_printf(s, "ID:%d, comp= %d, rot= %d, hdr= %d\n",
					log->data.dpp.id,
					log->data.dpp.comp,
					log->data.dpp.rot,
					log->data.dpp.hdr_std);
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

	if (!IS_DECON_ON_STATE(decon)) {
		decon_info("%s: decon is not ON(%d)\n", __func__, decon->state);
		return 0;
	}
	decon_dump(decon, IGN_DSI_DUMP);
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

	if (count <= 0)
		return count;

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

	if (count <= 0)
		return count;

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

static int decon_debug_mres_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%u\n", dpu_mres_log_level);

	return 0;
}

static int decon_debug_mres_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_mres_show, inode->i_private);
}

static ssize_t decon_debug_mres_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &dpu_mres_log_level);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_mres_fops = {
	.open = decon_debug_mres_open,
	.write = decon_debug_mres_write,
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

	if (count <= 0)
		return count;

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

	if (count <= 0)
		return count;

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
	seq_printf(s, "%u\n", dsim->lcd_info.mres_mode);

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
	unsigned int cmd_lp_ref = 0;
	struct dsim_device *dsim;
	int idx;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &cmd_lp_ref);
	if (ret != 1)
		goto out;

	dsim = get_dsim_drvdata(0);

	idx = dsim->lcd_info.mres_mode;
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
#if 0 /* TODO: This will be implemented */
	seq_printf(s, "VGF0[%u] VGF1[%u]\n",
			get_dpp_drvdata(DPU_DMA2CH(IDMA_VGF0))->d.recovery_cnt,
			get_dpp_drvdata(DPU_DMA2CH(IDMA_VGF1))->d.recovery_cnt);
#endif
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

static int decon_debug_low_persistence_show(struct seq_file *s, void *unused)
{
	struct decon_device *decon = get_decon_drvdata(0);
	seq_printf(s, "%u\n", decon->low_persistence);

	return 0;
}

static int decon_debug_low_persistence_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_low_persistence_show, inode->i_private);
}

static ssize_t decon_debug_low_persistence_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	struct decon_device *decon;
	char *buf_data;
	int ret;
	unsigned int low_persistence;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &low_persistence);
	if (ret < 0)
		goto out;

	decon = get_decon_drvdata(0);
	decon->low_persistence = low_persistence;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_low_persistence_fops = {
	.open = decon_debug_low_persistence_open,
	.write = decon_debug_low_persistence_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

void dpu_memmap_inc(struct decon_device *decon, dma_addr_t target)
{
#if defined(CONFIG_EXYNOS_MEMMAP_DEBUG)
	int i;
	if (decon->d.addr_n >= MAX_BUF_MEMMAP) {
		decon_info("%s:%d not enough room for the memmap\n",
				__func__, __LINE__);
		return;
	}
	for (i = 0; i < decon->d.addr_n; i++) {
		if (target == decon->d.mmap_info[i].addr_q) {
			decon->d.mmap_info[i].map_cnt++;
			return;
		}
	}
	decon->d.mmap_info[decon->d.addr_n].map_cnt = 1;
	decon->d.mmap_info[decon->d.addr_n].unmap_cnt = 0;
	decon->d.mmap_info[decon->d.addr_n].addr_q = target;
	decon->d.addr_n++;
#endif
}

void dpu_memmap_dec(struct decon_device *decon, dma_addr_t target)
{
#if defined(CONFIG_EXYNOS_MEMMAP_DEBUG)
	int i;
	for (i = 0; i < decon->d.addr_n; i++) {
		if (target == decon->d.mmap_info[i].addr_q) {
			decon->d.mmap_info[i].unmap_cnt++;
			return;
		}
	}
	decon_err("%s:%d address(0x%x) not found in the memmap list\n",
				__func__, __LINE__, target);
#endif
}

#if defined(CONFIG_EXYNOS_MEMMAP_DEBUG)
static int decon_n;
static int decon_debug_memmap_ref_cnt_show(struct seq_file *s, void *unused)
{
	int ret = 0;
	struct decon_device *decon;
	int i;

	decon = get_decon_drvdata(decon_n);
	if (!decon) {
		seq_printf(s, "decon%d not probed yet\n", decon_n);
		goto out;
	}

	seq_printf(s, "< Memory map count info : decon%d >\n", decon_n);
	for (i = 0; i < decon->d.addr_n; i++) {
		seq_printf(s, "0x%x : mapcnt(%u) unmapcnt(%u)\n",
				decon->d.mmap_info[i].addr_q,
				decon->d.mmap_info[i].map_cnt,
				decon->d.mmap_info[i].unmap_cnt);
	}
	if (!decon->d.addr_n)
		seq_printf(s, " - List empty\n");
	seq_printf(s, "--------------------------------\n");
out:
	return ret;
}

static int decon_debug_memmap_ref_cnt_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_memmap_ref_cnt_show, inode->i_private);
}

static ssize_t decon_debug_memmap_ref_cnt_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	int ret;
	char *buf_data;
	unsigned int d_n;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		goto out_buf;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &d_n);
	if (ret < 0)
		goto out;

	if (d_n > 2)
		d_n = 0;
	decon_info("decon%u selected for memmap ref cnt check\n", d_n);
	decon_n = d_n;
out:
	kfree(buf_data);
out_buf:
	return count;
}

static const struct file_operations decon_memmap_ref_cnt_fops = {
	.open = decon_debug_memmap_ref_cnt_open,
	.write = decon_debug_memmap_ref_cnt_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif

static int decon_debug_fence_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%u\n", dpu_fence_log_level);

	return 0;
}

static int decon_debug_fence_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_fence_show, inode->i_private);
}

static ssize_t decon_debug_fence_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &dpu_fence_log_level);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_fence_fops = {
	.open = decon_debug_fence_open,
	.write = decon_debug_fence_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

void decon_hiber_start(struct decon_device *decon)
{
	if (!decon->hiber.profile_started)
		return;

	decon->hiber.hiber_entry_time = ktime_get();
	decon->hiber.profile_enter_cnt++;
}

void decon_hiber_finish(struct decon_device *decon)
{
	if (!decon->hiber.profile_started)
		return;

	decon->hiber.hiber_time += ktime_to_us(ktime_sub(ktime_get(),
				decon->hiber.hiber_entry_time));
	decon->hiber.profile_exit_cnt++;
}

static int decon_get_hiber_ratio(struct decon_device *decon)
{
	s64 residency = decon->hiber.hiber_time;

	if (!residency)
		return 0;

	residency *= 100;
	do_div(residency, decon->hiber.profile_time);

	return residency;
}

static void _decon_profile_hiber_show(struct decon_device *decon)
{
	if (decon->hiber.profile_started) {
		decon_info("%s: hibernation profiling is ongoing\n", __func__);
		return;
	}

	decon_info("#########################################\n");
	decon_info("Profiling Time: %llu us\n", decon->hiber.profile_time);
	decon_info("Hibernation Entry Time: %llu us\n", decon->hiber.hiber_time);
	decon_info("Hibernation Entry Ratio: %d %%\n", decon_get_hiber_ratio(decon));
	decon_info("Entry count: %d, Exit count: %d\n", decon->hiber.profile_enter_cnt,
			decon->hiber.profile_exit_cnt);
	decon_info("Framedone count: %d, FPS: %lld\n", decon->hiber.frame_cnt,
			(decon->hiber.frame_cnt * 1000000) / decon->hiber.profile_time);
	decon_info("#########################################\n");
}


static int decon_profile_hiber_show(struct seq_file *s, void *unused)
{
	int ret = 0;
	struct decon_device *decon = get_decon_drvdata(0);

	if (!decon) {
		seq_printf(s, "decon0 is not probed yet\n");
		goto out;
	}

	_decon_profile_hiber_show(decon);

out:
	return ret;
}

static int decon_profile_hiber_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_profile_hiber_show, inode->i_private);
}

static void decon_profile_hiber_start(struct decon_device *decon)
{
	if (decon->hiber.profile_started) {
		decon_err("%s: hibernation profiling is ongoing\n", __func__);
		return;
	}

	/* reset profiling variables */
	decon->hiber.hiber_entry_time = 0;
	decon->hiber.hiber_time = 0;
	decon->hiber.profile_time = 0;
	decon->hiber.profile_enter_cnt = 0;
	decon->hiber.profile_exit_cnt = 0;
	decon->hiber.frame_cnt = 0;
	decon->hiber.fps = 0;

	/* profiling is just started */
	decon->hiber.profile_start_time = ktime_get();
	decon->hiber.profile_started = true;

	/* hibernation status when profiling is started */
	if (IS_DECON_HIBER_STATE(decon))
		decon_hiber_start(decon);

	decon_info("display hibernation profiling is started\n");
}

static void decon_profile_hiber_finish(struct decon_device *decon)
{
	if (!decon->hiber.profile_started) {
		decon_err("%s: hibernation profiling is not started\n", __func__);
		return;
	}

	decon->hiber.profile_time = ktime_to_us(ktime_sub(ktime_get(),
				decon->hiber.profile_start_time));

	/* hibernation status when profiling is finished */
	if (IS_DECON_HIBER_STATE(decon))
		decon_hiber_finish(decon);

	decon->hiber.profile_started = false;

	_decon_profile_hiber_show(decon);

	decon_info("display hibernation profiling is finished\n");
}

static ssize_t decon_profile_hiber_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	char *buf_data;
	int ret;
	int input;
	struct decon_device *decon = get_decon_drvdata(0);

	if (!decon) {
		decon_err("decon0 is not probed yet\n");
		goto out_cnt;
	}

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		goto out_cnt;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &input);
	if (ret < 0)
		goto out;

	if (input)
		decon_profile_hiber_start(decon);
	else
		decon_profile_hiber_finish(decon);

out:
	kfree(buf_data);
out_cnt:
	return count;
}

static const struct file_operations decon_profile_hiber_fops = {
	.open = decon_profile_hiber_open,
	.write = decon_profile_hiber_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

#if defined(CONFIG_EXYNOS_CHANGE_HIBER_CNT)
static int decon_hiber_cnt_show(struct seq_file *s, void *unused)
{
	struct decon_device *decon = get_decon_drvdata(0);
	seq_printf(s, "%u\n", decon->hiber.hiber_enter_cnt);

	return 0;
}

static int decon_hiber_cnt_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_hiber_cnt_show, inode->i_private);
}

static ssize_t decon_hiber_cnt_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	struct decon_device *decon = get_decon_drvdata(0);
	char *buf_data;
	int ret;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	ret = sscanf(buf_data, "%u", &decon->hiber.hiber_enter_cnt);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_hiber_cnt_fops = {
	.open = decon_hiber_cnt_open,
	.write = decon_hiber_cnt_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};
#endif

static int decon_debug_freq_hop_show(struct seq_file *s, void *unused)
{
	struct decon_device *decon = get_decon_drvdata(0);

	seq_printf(s, "m(%u) k(%u)\n", decon->freq_hop.request_m,
			decon->freq_hop.request_k);

	return 0;
}

static int decon_debug_freq_hop_open(struct inode *inode, struct file *file)
{
	return single_open(file, decon_debug_freq_hop_show, inode->i_private);
}

static ssize_t decon_debug_freq_hop_write(struct file *file, const char __user *buf,
		size_t count, loff_t *f_ops)
{
	struct decon_device *decon = get_decon_drvdata(0);
	char *buf_data;
	int ret;

	if (!decon->freq_hop.enabled)
		return count;

	if (count <= 0)
		return count;

	buf_data = kmalloc(count, GFP_KERNEL);
	if (buf_data == NULL)
		return count;

	memset(buf_data, 0, count);

	ret = copy_from_user(buf_data, buf, count);
	if (ret < 0)
		goto out;

	/*
	 * The synchronization is required when request_m value is updated
	 * between writing sysfs node of "request_m" and calling
	 * S3CFB_WIN_CONFIG ioctl.
	 */
	mutex_lock(&decon->lock);
	ret = sscanf(buf_data, "%u %u", &decon->freq_hop.request_m,
			&decon->freq_hop.request_k);
	mutex_unlock(&decon->lock);
	if (ret < 0)
		goto out;

out:
	kfree(buf_data);
	return count;
}

static const struct file_operations decon_freq_hop_fops = {
	.open = decon_debug_freq_hop_open,
	.write = decon_debug_freq_hop_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

#ifdef CONFIG_SUPPORT_RDX_DUMP
static unsigned int *rdx_mem_ptr;
static void *rdx_mem_buf;
static size_t rdx_mem_pos;
static size_t rdx_mem_size;

void *rdx_mem_alloc(size_t s)
{
	void *ret;

	if (s > rdx_mem_size)
		return NULL;

	ret = rdx_mem_buf + rdx_mem_pos;
	rdx_mem_pos += s;

	return ret;
}

static int __init decon_event_log_setup(char *str)
{
	size_t size = memparse(str, &str);
	unsigned long base = 0;
	unsigned int *rdx_mem_magic;

	pr_info("%s :%s\n", __func__, str);

	if (!size || size != roundup_pow_of_two(size) ||
	    *str != '@' || kstrtoul(str + 1, 0, &base))
		goto setup_exit;

#ifdef CONFIG_NO_BOOTMEM
	if (memblock_is_region_reserved(base - 8, size + 8) ||
	    memblock_reserve(base - 8, size + 8)) {
#else
	if (reserve_bootmem(base - 8, size + 8, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_err("%s: failed reserving size %d at base 0x%lx\n",
		       __func__, size, base);
		goto setup_exit;
	}
	rdx_mem_magic = phys_to_virt(base) - 8;
	rdx_mem_ptr = phys_to_virt(base) - 4;
	rdx_mem_buf = phys_to_virt(base);
	rdx_mem_size = size;

	pr_info("%s: *disp_rdx_log_ptr:%x\n", __func__, *rdx_mem_ptr);
	pr_info("%s: disp_rdx_log_buf:%p disp_rdx_log_size:0x%llx\n",
		__func__, rdx_mem_buf, rdx_mem_size);

	return 1;

setup_exit:
	return 0;
}
__setup("sec_disp_log=", decon_event_log_setup);
#endif

int decon_create_debugfs(struct decon_device *decon)
{
	char name[MAX_NAME_SIZE];
	int ret = 0;
	int i;
	u32 event_cnt;
	size_t real_size = 0;

	decon->d.event_log = NULL;
	event_cnt = DPU_EVENT_LOG_MAX;
	for (i = 0; i < DPU_EVENT_LOG_RETRY; ++i) {
		event_cnt = event_cnt >> i;
#ifdef CONFIG_SUPPORT_RDX_DUMP
		if (decon->id == 0) {
			decon->d.event_log_header = rdx_mem_alloc(sizeof(struct dpu_log_header));
			if (IS_ERR_OR_NULL(decon->d.event_log_header)) {
				decon_warn("failed to alloc event log header buf[%d]. retry\n",
						sizeof(struct dpu_log_header));
				continue;
			}
			real_size = sizeof(struct dpu_log_header) + sizeof(struct dpu_log) * event_cnt;
			pr_info("%s alloc total size %llx\n", __func__, real_size);
			if (real_size >= rdx_mem_size) {
				decon_warn("failed to alloc because over size[%d]. retry\n",
						real_size);
				continue;
			}
			dpu_event_log_header(decon, event_cnt);
			decon->d.event_log = rdx_mem_alloc(sizeof(struct dpu_log) * event_cnt);
		} else {
			decon->d.event_log = kzalloc(sizeof(struct dpu_log) * event_cnt,
					GFP_KERNEL);
		}
#else
		decon->d.event_log = kzalloc(sizeof(struct dpu_log) * event_cnt,
				GFP_KERNEL);
#endif
		if (IS_ERR_OR_NULL(decon->d.event_log)) {
			decon_warn("failed to alloc event log buf[%d]. retry\n",
					event_cnt);
			continue;
		}

		decon_info("#%d event log buffers are allocated\n", event_cnt);
		break;
	}
	decon->d.event_log_cnt = event_cnt;
	atomic_set(&decon->d.event_log_idx, -1);

	decon->d.f_evt_log = NULL;
	event_cnt = DPU_FENCE_EVENT_LOG_MAX;
	for (i = 0; i < DPU_FENCE_EVENT_LOG_RETRY; ++i) {
		event_cnt = event_cnt >> i;
		decon->d.f_evt_log = kzalloc(sizeof(struct dpu_fence_log) * event_cnt,
				GFP_KERNEL);
		if (IS_ERR_OR_NULL(decon->d.f_evt_log)) {
			decon_warn("failed to alloc fence event log buf[%d]. retry\n",
					event_cnt);
			continue;
		}

		decon_info("#%d fence event log buffers are allocated\n", event_cnt);
		break;
	}
	decon->d.f_evt_log_cnt = event_cnt;
	atomic_set(&decon->d.f_evt_log_idx, -1);

	if (!decon->id) {
		decon->d.debug_root = debugfs_create_dir("decon", NULL);
		if (!decon->d.debug_root) {
			decon_err("failed to create debugfs root directory.\n");
			ret = -ENOENT;
			goto err_event_log;
		}
	}

	if (decon->id == 1 || decon->id == 2)
		decon->d.debug_root = decon_drvdata[0]->d.debug_root;

	snprintf(name, MAX_NAME_SIZE, "event%d", decon->id);
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
		decon->d.debug_mres = debugfs_create_file("mres_log", 0444,
				decon->d.debug_root, NULL, &decon_mres_fops);
		if (!decon->d.debug_mres) {
			decon_err("failed to create mres log level file\n");
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
		decon->d.debug_low_persistence = debugfs_create_file("low_persistence",
				0444, decon->d.debug_root, NULL, &decon_low_persistence_fops);
		if (!decon->d.debug_low_persistence) {
			decon_err("failed to create low persistence file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}

#if defined(CONFIG_EXYNOS_MEMMAP_DEBUG)
		decon->d.debug_memmap_ref_cnt = debugfs_create_file("memmap_ref_cnt",
				0444, decon->d.debug_root, NULL, &decon_memmap_ref_cnt_fops);
		if (!decon->d.debug_memmap_ref_cnt) {
			decon_err("failed to create memory map ref cnt file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
#endif

		decon->hiber.profile = debugfs_create_file("profile_hiber",
				0444, decon->d.debug_root, NULL,
				&decon_profile_hiber_fops);
		if (!decon->hiber.profile) {
			decon_err("failed to create hibernation profiling\n");
			ret = -ENOENT;
			goto err_debugfs;
		}

#if defined(CONFIG_EXYNOS_CHANGE_HIBER_CNT)
		decon->hiber.hiber_cnt = debugfs_create_file("hiber_enter_cnt",
				0444, decon->d.debug_root, NULL,
				&decon_hiber_cnt_fops);
		if (!decon->hiber.hiber_cnt) {
			decon_err("failed to create hibernation entry count\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
#endif
		decon->d.debug_freq_hop = debugfs_create_file("request_mk",
				0444, decon->d.debug_root, NULL, &decon_freq_hop_fops);
		if (!decon->d.debug_freq_hop) {
			decon_err("failed to create request m value file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}

		decon->d.debug_fence = debugfs_create_file("fence_log", 0444,
				decon->d.debug_root, NULL, &decon_fence_fops);
		if (!decon->d.debug_fence) {
			decon_err("failed to create fence log level file\n");
			ret = -ENOENT;
			goto err_debugfs;
		}
	}

	return 0;

err_debugfs:
	debugfs_remove_recursive(decon->d.debug_root);
err_event_log:
#ifdef CONFIG_SUPPORT_RDX_DUMP
	if (decon->id != 0)
		kfree(decon->d.event_log);
#else
	kfree(decon->d.event_log);
#endif
	kfree(decon->d.f_evt_log);
	decon->d.event_log = NULL;
	decon->d.f_evt_log = NULL;
	return ret;
}

void decon_destroy_debugfs(struct decon_device *decon)
{
	if (decon->d.debug_root)
		debugfs_remove(decon->d.debug_root);
	if (decon->d.debug_event)
		debugfs_remove(decon->d.debug_event);
}

