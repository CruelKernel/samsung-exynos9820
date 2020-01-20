/*
 * linux/drivers/gpu/exynos/g2d/g2d_perf.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
 *
 * Contact: Hyesoo Yu <hyesoo.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "g2d.h"
#include "g2d_perf.h"
#include "g2d_task.h"
#include "g2d_uapi.h"

#include <linux/workqueue.h>

#ifdef CONFIG_PM_DEVFREQ
static void g2d_pm_qos_update_devfreq(struct pm_qos_request *req, u32 freq)
{
	if (!pm_qos_request_active(req))
		pm_qos_add_request(req, PM_QOS_DEVICE_THROUGHPUT, 0);

	pm_qos_update_request(req, freq);
}

static void g2d_pm_qos_remove_devfreq(struct pm_qos_request *req)
{
	if (pm_qos_request_active(req))
		pm_qos_remove_request(req);
}
#else
#define g2d_pm_qos_update_devfreq(req, freq) do { } while (0)
#define g2d_pm_qos_remove_devfreq(req) do { } while (0)
#endif

static bool g2d_still_need_perf(struct g2d_device *g2d_dev)
{
	struct g2d_task *task;
	unsigned long flags;

	spin_lock_irqsave(&g2d_dev->lock_task, flags);
	for (task = g2d_dev->tasks; task != NULL; task = task->next) {
		if (!is_task_state_idle(task)) {
			spin_unlock_irqrestore(&g2d_dev->lock_task, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&g2d_dev->lock_task, flags);

	return false;
}

/*
 * The reference point is pixelcount scaling ratio that both width
 * and height are 1 times, 1/2 times, 1/3 times or 1/4 times.
 * To eliminate decimal point, shift to the left by 10 and
 * that value divided by the reference value is as follows.
 */
static u32 perf_basis[PPC_SC] = {1024, 1023, 256, 113, 64, 0};

static char perf_index_sc(struct g2d_performance_layer_data *layer)
{
	u32 ratio = (((u64)layer->window_w * layer->window_h) << 10) /
			((u32)layer->crop_w * layer->crop_h);
	int i;

	for (i = 0; i < PPC_SC; i++) {
		if (ratio > perf_basis[i])
			return i;
	}

	return PPC_SC_DOWN_16;
}

static void g2d_set_device_frequency(struct g2d_context *g2d_ctx,
					  struct g2d_performance_data *data)
{
	struct g2d_device *g2d_dev = g2d_ctx->g2d_dev;
	struct g2d_performance_frame_data *frame;
	struct g2d_performance_layer_data *layer;
	u32 (*ppc)[PPC_ROT][PPC_SC] = (u32 (*)[PPC_ROT][PPC_SC])g2d_dev->hw_ppc;
	unsigned int cycle, ip_clock, crop, window;
	int i, j;
	int sc, fmt, rot;

	cycle = 0;

	for (i = 0; i < data->num_frame; i++) {
		frame = &data->frame[i];

		rot = 0;
		for (j = 0; j < frame->num_layers; j++) {
			if (perf_index_rotate(&frame->layer[j])) {
				rot++;
				break;
			}
		}

		for (j = 0; j < frame->num_layers; j++) {
			layer = &frame->layer[j];

			crop = (u32)layer->crop_w * layer->crop_h;
			window = (u32)layer->window_w * layer->window_h;

			fmt = perf_index_fmt(layer);
			sc = perf_index_sc(layer);

			if (fmt == PPC_FMT)
				return;

			cycle += max(crop, window) / ppc[fmt][rot][sc];

			/*
			 * If frame has colorfill layer on the bottom,
			 * upper layaer is treated as opaque.
			 * In this case, colorfill is not be processed
			 * as much as the overlapping area.
			 */
			if (!j && is_perf_frame_colorfill(frame)) {
				unsigned int pixelcount;

				pixelcount = frame->target_pixelcount - window;
				if (pixelcount > 0)
					cycle += pixelcount /
						g2d_dev->hw_ppc[PPC_COLORFILL];

			}
		}
	}

	/* ip_clock(Mhz) = cycles / time_in_ms * 1000 * 10% */
	ip_clock = (cycle / 7) * 1100;

	for (i = 0; i < g2d_dev->dvfs_table_cnt; i++) {
		if (ip_clock > g2d_dev->dvfs_table[i].freq) {
			ip_clock = (i == 0) ?
					g2d_dev->dvfs_table[i].lv :
					g2d_dev->dvfs_table[i - 1].lv;
			break;
		}
	}

	if (!ip_clock)
		g2d_pm_qos_remove_devfreq(&g2d_ctx->req);
	else if (ip_clock)
		g2d_pm_qos_update_devfreq(&g2d_ctx->req, ip_clock);
}

static void g2d_set_qos_frequency(struct g2d_context *g2d_ctx,
					  struct g2d_performance_data *data)
{
	struct g2d_device *g2d_dev = g2d_ctx->g2d_dev;
	struct g2d_performance_frame_data *frame;
	u32 cur_rbw, rbw;
	u32 cur_wbw, wbw;
	int i;

	cur_rbw = 0;
	cur_wbw = 0;
	rbw = 0;
	wbw = 0;

	for (i = 0; i < data->num_frame; i++) {
		frame = &data->frame[i];

		rbw += frame->bandwidth_read;
		wbw += frame->bandwidth_write;
	}

	if (list_empty(&g2d_ctx->qos_node) && !rbw && !wbw)
		return;

	if (!list_empty(&g2d_dev->qos_contexts)) {
		struct g2d_context *ctx_qos;

		ctx_qos = list_first_entry(&g2d_dev->qos_contexts,
					   struct g2d_context, qos_node);
		cur_rbw = ctx_qos->r_bw;
		cur_wbw = ctx_qos->w_bw;
	}

	/* this works although ctx is not attached to qos_contexts */
	list_del_init(&g2d_ctx->qos_node);

	g2d_ctx->r_bw = rbw;
	g2d_ctx->w_bw = wbw;

	if (rbw || wbw) {
		struct list_head *node;

		for (node = g2d_dev->qos_contexts.prev;
				node != &g2d_dev->qos_contexts;
						node = node->prev) {
			struct g2d_context *curctx = list_entry(node,
					struct g2d_context, qos_node);
			if ((curctx->r_bw + curctx->w_bw) > (rbw + wbw))
				break;
		}
		/*
		 * node always points to the head node or the smallest bw node
		 * among the larger bw nodes than qosnode
		 */
		list_add(&g2d_ctx->qos_node, node);
	}

	if (!list_empty(&g2d_dev->qos_contexts)) {
		struct g2d_context *ctx_qos;

		ctx_qos = list_first_entry(&g2d_dev->qos_contexts,
				      struct g2d_context, qos_node);
		/* bandwidth request is changed */
		rbw = ctx_qos->r_bw;
		wbw = ctx_qos->w_bw;
	}

	if ((rbw != cur_rbw) || (wbw != cur_wbw)) {
		/*
		 * FIXME: BTS is not available for now
		 * struct bts_bw bw;
		 *
		 * bw.peak = ((rbw + wbw) / 1000) * BTS_PEAK_FPS_RATIO / 2;
		 * bw.write = wbw;
		 * bw.read = rbw;
		 * bts_update_bw(BTS_BW_G2D, bw);
		 */
	}
}

void g2d_set_performance(struct g2d_context *ctx,
				struct g2d_performance_data *data, bool release)
{
	struct g2d_device *g2d_dev = ctx->g2d_dev;
	int i;

	if (data->num_frame > G2D_PERF_MAX_FRAMES)
		return;

	for (i = 0; i < data->num_frame; i++) {
		if (data->frame[i].num_layers > g2d_dev->max_layers)
			return;
	}

	mutex_lock(&g2d_dev->lock_qos);

	if (!data->num_frame) {
		if (g2d_still_need_perf(g2d_dev) && !release) {
			mod_delayed_work(system_wq, &ctx->dwork,
					 msecs_to_jiffies(50));
			mutex_unlock(&g2d_dev->lock_qos);
			return;
		}
		cancel_delayed_work(&ctx->dwork);
	} else {
		mod_delayed_work(system_wq, &ctx->dwork,
			msecs_to_jiffies(50));
	}

	g2d_set_qos_frequency(ctx, data);
	g2d_set_device_frequency(ctx, data);

	mutex_unlock(&g2d_dev->lock_qos);
}

void g2d_put_performance(struct g2d_context *ctx, bool release)
{
	struct g2d_performance_data data;

	data.num_frame = 0;

	g2d_set_performance(ctx, &data, release);
}
