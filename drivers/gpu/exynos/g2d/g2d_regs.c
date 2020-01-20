/*
 * linux/drivers/gpu/exynos/g2d/g2d_regs.c
 *
 * Copyright (C) 2017 Samsung Electronics Co., Ltd.
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

#include <linux/kernel.h>
#include <linux/io.h>

#include <asm/cacheflush.h>

#include "g2d.h"
#include "g2d_regs.h"
#include "g2d_task.h"
#include "g2d_uapi.h"
#include "g2d_debug.h"
#include "g2d_secure.h"

static void g2d_hw_push_task_by_smc(struct g2d_device *g2d_dev,
				    struct g2d_task *task)
{
	int i;

	task->sec.secure_layer_mask = 0;

	for (i = 0; i < task->num_source; i++)
		if (!!(task->source[i].flags & G2D_LAYERFLAG_SECURE))
			task->sec.secure_layer_mask |= 1 << i;

	if (!!(task->target.flags & G2D_LAYERFLAG_SECURE) ||
		!!(task->sec.secure_layer_mask))
		task->sec.secure_layer_mask |= 1 << 24;

	__flush_dcache_area(&task->sec, sizeof(task->sec));
	__flush_dcache_area(page_address(task->cmd_page), G2D_CMD_LIST_SIZE);
	if (exynos_smc(SMC_DRM_G2D_CMD_DATA, virt_to_phys(&task->sec), 0, 0)) {
		perrfndev(g2d_dev, "Failed to push %d %d %d %d",
			  task->sec.cmd_count, task->sec.priority,
			  task->sec.job_id, task->sec.secure_layer_mask);

		g2d_dump_info(g2d_dev, task);
		BUG();
	}
}

void g2d_hw_push_task(struct g2d_device *g2d_dev, struct g2d_task *task)
{
	bool self_prot = g2d_dev->caps & G2D_DEVICE_CAPS_SELF_PROTECTION;
	u32 state = g2d_hw_get_job_state(g2d_dev, task->sec.job_id);

	if (state != G2D_JOB_STATE_DONE)
		perrfndev(g2d_dev, "Unexpected state %#x of JOB %d",
			  state, task->sec.job_id);

	if (IS_ENABLED(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)) {
		unsigned int i;

		if (!self_prot) {
			g2d_hw_push_task_by_smc(g2d_dev, task);
			return;
		}

		state = 0;

		for (i = 0; i < task->num_source; i++)
			if (task->source[i].flags & G2D_LAYERFLAG_SECURE)
				state |= 1 << i;

		if ((task->target.flags & G2D_LAYERFLAG_SECURE) || state)
			state |= 1 << 24;

		writel_relaxed(state,
			       g2d_dev->reg +
			       G2D_JOBn_LAYER_SECURE_REG(task->sec.job_id));
	}

	writel_relaxed(G2D_JOB_HEADER_DATA(task->sec.priority,
					   task->sec.job_id),
			g2d_dev->reg + G2D_JOB_HEADER_REG);

	writel_relaxed(G2D_ERR_INT_ENABLE, g2d_dev->reg + G2D_INTEN_REG);

	writel_relaxed(task->cmd_addr, g2d_dev->reg + G2D_JOB_BASEADDR_REG);
	writel_relaxed(task->sec.cmd_count, g2d_dev->reg + G2D_JOB_SFRNUM_REG);
	writel_relaxed(1 << task->sec.job_id,
		       g2d_dev->reg + G2D_JOB_INT_ID_REG);
	writel(G2D_JOBPUSH_INT_ENABLE, g2d_dev->reg + G2D_JOB_PUSH_REG);
}

bool g2d_hw_stuck_state(struct g2d_device *g2d_dev)
{
	int i, val;
	int retry_count = 10;
	bool is_idle = true;
	bool is_running = false;

	while (retry_count-- > 0) {
		for (i = 0; i < G2D_MAX_JOBS; i++) {
			val = readl_relaxed(
				g2d_dev->reg + G2D_JOB_IDn_STATE_REG(i));

			val &= G2D_JOB_STATE_MASK;

			if (val != G2D_JOB_STATE_DONE)
				is_idle = false;

			if (val == G2D_JOB_STATE_RUNNING)
				is_running = true;
		}

		if (is_idle || is_running)
			return false;

		is_idle = true;
	}

	return true;
}

static const char *error_desc[3] = {
	"AFBC Stuck",
	"No read response",
	"No write response",
};

u32 g2d_hw_errint_status(struct g2d_device *g2d_dev)
{
	u32 status = readl_relaxed(g2d_dev->reg + G2D_INTC_PEND_REG);
	u32 errstatus = status;
	int idx;

	/* IRQPEND_SCF should not be set because we don't use ABP mode */
	BUG_ON((status & 0x1) == 1);

	errstatus >>= 16;
	errstatus &= 0x7;

	if (errstatus == 0)
		return 0;

	for (idx = 0; idx < 3; idx++) {
		if (errstatus & (1 << idx))
			perrdev(g2d_dev, "G2D ERROR INTERRUPT: %s",
				error_desc[idx]);
	}

	perrdev(g2d_dev, "G2D FIFO STATUS: %#x", g2d_hw_fifo_status(g2d_dev));

	return status;
}

/*
 * This is called when H/W is judged to be in operation,
 * for example, when a sysmmu fault and an error interrupt occurs.
 * If there is no task in progress, the status of all task on H/W
 * is taken and print for debugging
 */
int g2d_hw_get_current_task(struct g2d_device *g2d_dev)
{
	int i, val;

	for (i = 0; i < G2D_MAX_JOBS; i++) {
		val = readl_relaxed(g2d_dev->reg + G2D_JOB_IDn_STATE_REG(i));
		if ((val & G2D_JOB_STATE_MASK) == G2D_JOB_STATE_RUNNING)
			return i;
	}

	for (i = 0; i < G2D_MAX_JOBS; i++) {
		val = readl_relaxed(g2d_dev->reg + G2D_JOB_IDn_STATE_REG(i));
		perrdev(g2d_dev, "G2D TASK[%03d] STATE : %d", i, val);
	}

	return -1;
}

void g2d_hw_kill_task(struct g2d_device *g2d_dev, unsigned int job_id)
{
	int retry_count = 120;

	writel((0 << 4) | job_id, g2d_dev->reg + G2D_JOB_KILL_REG);

	while (retry_count-- > 0) {
		if (!(readl(g2d_dev->reg + G2D_JOB_PUSHKILL_STATE_REG) & 0x2)) {
			perrdev(g2d_dev, "Killed JOB %d", job_id);
			return;
		}
	}

	perrdev(g2d_dev, "Failed to kill job %d", job_id);
}
