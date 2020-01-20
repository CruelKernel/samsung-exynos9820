/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/v4l2-mediabus.h>
#include <linux/bug.h>

#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"

#include "fimc-is-device-sensor.h"

ulong frame_fcount(struct fimc_is_frame *frame, void *data)
{
	return (ulong)frame->fcount - (ulong)data;
}

int put_frame(struct fimc_is_framemgr *this, struct fimc_is_frame *frame,
			enum fimc_is_frame_state state)
{
	if (state == FS_INVALID)
		return -EINVAL;

	if (!frame) {
		err("invalid frame");
		return -EFAULT;
	}

	frame->state = state;

	list_add_tail(&frame->list, &this->queued_list[state]);
	this->queued_count[state]++;

#ifdef TRACE_FRAME
	print_frame_queue(this, state);
#endif

	return 0;
}

struct fimc_is_frame *get_frame(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state)
{
	struct fimc_is_frame *frame;

	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	frame = list_first_entry(&this->queued_list[state],
						struct fimc_is_frame, list);
	list_del(&frame->list);
	this->queued_count[state]--;

	frame->state = FS_INVALID;

	return frame;
}

int trans_frame(struct fimc_is_framemgr *this, struct fimc_is_frame *frame,
			enum fimc_is_frame_state state)
{
	if (!frame) {
		err("invalid frame");
		return -EFAULT;
	}

	if ((frame->state == FS_INVALID) || (state == FS_INVALID))
		return -EINVAL;

	if (!this->queued_count[frame->state]) {
		err("%s frame queue is empty (%s)", frame_state_name[frame->state],
							this->name);
		return -EINVAL;
	}

	list_del(&frame->list);
	this->queued_count[frame->state]--;

	if (state == FS_PROCESS && (!(this->id & FRAMEMGR_ID_HW)))
		frame->bak_flag = frame->out_flag;

	return put_frame(this, frame, state);
}

struct fimc_is_frame *peek_frame(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state)
{
	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	return list_first_entry(&this->queued_list[state],
						struct fimc_is_frame, list);
}

struct fimc_is_frame *peek_frame_tail(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state)
{
	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state])
		return NULL;

	return list_last_entry(&this->queued_list[state],
						struct fimc_is_frame, list);
}

struct fimc_is_frame *find_frame(struct fimc_is_framemgr *this,
		enum fimc_is_frame_state state,
		ulong (*fn)(struct fimc_is_frame *, void *), void *data)
{
	struct fimc_is_frame *frame;

	if (state == FS_INVALID)
		return NULL;

	if (!this->queued_count[state]) {
		err("[F%lu] %s frame queue is empty (%s)",
			(ulong)data, frame_state_name[state], this->name);
		return NULL;
	}

	list_for_each_entry(frame, &this->queued_list[state], list) {
		if (!fn(frame, data))
			return frame;
	}

	return NULL;
}

void print_frame_queue(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state)
{
	struct fimc_is_frame *frame, *temp;

	if (!(TRACE_ID & this->id))
			return;

	pr_info("[FRM] %s(%s, %d) :", frame_state_name[state],
					this->name, this->queued_count[state]);

	list_for_each_entry_safe(frame, temp, &this->queued_list[state], list)
		pr_cont("%d[%d]->", frame->index, frame->fcount);

	pr_cont("X\n");
}

#ifndef ENABLE_IS_CORE
void print_frame_info_queue(struct fimc_is_framemgr *this,
			enum fimc_is_frame_state state)
{
	unsigned long long when[MAX_FRAME_INFO];
	unsigned long usec[MAX_FRAME_INFO];
	struct fimc_is_frame *frame, *temp;

	if (!(TRACE_ID & this->id))
			return;

	pr_info("[FRM_INFO] %s(%s, %d) :", hw_frame_state_name[state],
					this->name, this->queued_count[state]);

	list_for_each_entry_safe(frame, temp, &this->queued_list[state], list) {
		when[INFO_FRAME_START]    = frame->frame_info[INFO_FRAME_START].when;
		when[INFO_CONFIG_LOCK]    = frame->frame_info[INFO_CONFIG_LOCK].when;
		when[INFO_FRAME_END_PROC] = frame->frame_info[INFO_FRAME_END_PROC].when;

		usec[INFO_FRAME_START]    = do_div(when[INFO_FRAME_START], NSEC_PER_SEC);
		usec[INFO_CONFIG_LOCK]    = do_div(when[INFO_CONFIG_LOCK], NSEC_PER_SEC);
		usec[INFO_FRAME_END_PROC] = do_div(when[INFO_FRAME_END_PROC], NSEC_PER_SEC);

		pr_cont("%d[%d][%d]([%5lu.%06lu],[%5lu.%06lu],[%5lu.%06lu][C:0x%lx])->",
			frame->index, frame->fcount, frame->type,
			(unsigned long)when[INFO_FRAME_START],    usec[INFO_FRAME_START] / NSEC_PER_USEC,
			(unsigned long)when[INFO_CONFIG_LOCK],    usec[INFO_CONFIG_LOCK] / NSEC_PER_USEC,
			(unsigned long)when[INFO_FRAME_END_PROC], usec[INFO_FRAME_END_PROC] / NSEC_PER_USEC,
			frame->core_flag);
	}

	pr_cont("X\n");
}
#endif

int frame_manager_probe(struct fimc_is_framemgr *this, u32 id, const char *name)
{
	this->id = id;
	snprintf(this->name, sizeof(this->name), "%s", name);
	spin_lock_init(&this->slock);
	this->frames = NULL;

	return 0;
}

static void default_frame_work_fn(struct kthread_work *work)
{
	struct fimc_is_groupmgr *groupmgr;
	struct fimc_is_group *group;
	struct fimc_is_frame *frame;

	frame = container_of(work, struct fimc_is_frame, work);
	groupmgr = frame->groupmgr;
	group = frame->group;

	fimc_is_group_shot(groupmgr, group, frame);
}

int frame_manager_open(struct fimc_is_framemgr *this, u32 buffers)
{
	u32 i;
	unsigned long flag;

	/*
	 * We already have frames allocated, so we should free them first.
	 * reqbufs(n) could be called multiple times from userspace after
	 * each video node was opened.
	 */
	if (this->frames)
		vfree(this->frames);

	this->frames = vzalloc(sizeof(struct fimc_is_frame) * buffers);
	if (!this->frames) {
		err("failed to allocate frames");
		return -ENOMEM;
	}

	spin_lock_irqsave(&this->slock, flag);

	this->num_frames = buffers;

	for (i = 0; i < NR_FRAME_STATE; i++) {
		this->queued_count[i] = 0;
		INIT_LIST_HEAD(&this->queued_list[i]);
	}

	for (i = 0; i < buffers; ++i) {
		this->frames[i].index = i;
		put_frame(this, &this->frames[i], FS_FREE);
		kthread_init_work(&this->frames[i].work, default_frame_work_fn);
	}

	spin_unlock_irqrestore(&this->slock, flag);

	return 0;
}

int frame_manager_close(struct fimc_is_framemgr *this)
{
	u32 i;
	unsigned long flag;

	spin_lock_irqsave(&this->slock, flag);

	if (this->frames) {
		vfree_atomic(this->frames);
		this->frames = NULL;
	}

	this->num_frames = 0;

	for (i = 0; i < NR_FRAME_STATE; i++) {
		this->queued_count[i] = 0;
		INIT_LIST_HEAD(&this->queued_list[i]);
	}

	spin_unlock_irqrestore(&this->slock, flag);

	return 0;
}

int frame_manager_flush(struct fimc_is_framemgr *this)
{
	unsigned long flag;
	struct fimc_is_frame *frame, *temp;
	enum fimc_is_frame_state i;

	spin_lock_irqsave(&this->slock, flag);

	for (i = FS_REQUEST; i < FS_INVALID; i++) {
		list_for_each_entry_safe(frame, temp, &this->queued_list[i], list)
			trans_frame(this, frame, FS_FREE);
	}

	spin_unlock_irqrestore(&this->slock, flag);

	FIMC_BUG(this->queued_count[FS_FREE] != this->num_frames);

	return 0;
}

void frame_manager_print_queues(struct fimc_is_framemgr *this)
{
	int i;

	for (i = 0; i < NR_FRAME_STATE; i++)
		print_frame_queue(this, (enum fimc_is_frame_state)i);
}

void frame_manager_print_info_queues(struct fimc_is_framemgr *this)
{
#ifndef ENABLE_IS_CORE
	int i;

	for (i = 0; i < NR_FRAME_STATE; i++)
		print_frame_info_queue(this, (enum fimc_is_frame_state)i);
#endif
}

