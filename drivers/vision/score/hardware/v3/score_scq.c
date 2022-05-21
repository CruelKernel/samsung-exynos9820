/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#if (KERNEL_VERSION(4, 11, 0) <= LINUX_VERSION_CODE)
#include <uapi/linux/sched/types.h>
#endif

#include "score_log.h"
#include "score_scq.h"
#include "score_regs.h"
#include "score_packet.h"
#include "score_system.h"
#include "score_context.h"
#include "score_frame.h"
#include "score_dpmu.h"

#if defined(CONFIG_EXYNOS_SCORE)
#define INIT_TIMEOUT		100
#else /* FPGA */
#define INIT_TIMEOUT		300
#endif

/* pending debug log */
/* TODO remove debug log */
#define DEBUG_PENDING_LOG

static int __score_scq_wait_init(struct score_scq *scq,
		struct score_frame *frame)
{
	int ret = -ETIMEDOUT;
	int ready;
	int time_count = INIT_TIMEOUT;
	int i;

	score_enter();

	for (i = 0; i < time_count; i++) {
		ready = readl(scq->sfr + TS_INIT_DONE_CHECK);
		if (ready & 0xffff0000) {
			ret = 0;
			break;
		}
		mdelay(1);
	}

	if (ret) {
		score_err("Init time(%dms)is over\n", INIT_TIMEOUT);
		score_dpmu_print_all();
	} else {
		scq->high_max_count = readl(scq->sfr + SCHEDULE_HIGH_COUNT);
		scq->normal_max_count = readl(scq->sfr + SCHEDULE_NORMAL_COUNT);
		scq->state = BIT(SCORE_SCQ_STATE_START);
	}

	score_leave();
	return ret;
}

static int __score_scq_pending_create(struct score_scq *scq,
		struct score_frame *frame)
{
	int ret = 0;
	struct score_host_packet *packet;
	unsigned int size;

	score_enter();
	if (frame->pended)
		return 0;

	packet = frame->packet;
	size = packet->size;

	frame->pending_packet = kmalloc(size, GFP_ATOMIC);
	if (!frame->pending_packet) {
		ret = -ENOMEM;
		score_err("Fail to alloc packet for pending frame (%u/%u,%u)\n",
				frame->sctx->id, frame->frame_id,
				frame->kernel_id);
		goto p_err;
	}

	memcpy(frame->pending_packet, packet, size);
	frame->pended = true;

	score_leave();
p_err:
	return ret;
}

static int __score_scq_create(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	struct score_host_packet *packet;
	struct score_host_packet_info *packet_info;
	unsigned int size;

	score_enter();
	if (frame->pended)
		packet = frame->pending_packet;
	else
		packet = frame->packet;

	packet_info = (struct score_host_packet_info *)&packet->payload[0];
	size = packet_info->valid_size >> 2;
	if (size > MAX_SCQ_SIZE) {
		score_err("size(%u) is too big compared to scq(%u)\n",
				size, MAX_SCQ_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	scq->out_size = size;
	scq->out_param = (unsigned int *)((unsigned long)packet_info +
			packet->packet_offset);
	/*
	 * APCPU : 0x01
	 * BR1   : 0x02
	 * BR2   : 0x04
	 * IVA   : 0x08
	 * TS    : 0x10
	 */
	scq->slave_id = 0x10;
	if (frame->priority)
		scq->priority = 0x1;
	else
		scq->priority = 0x0;

	score_leave();
p_err:
	return ret;
}

static void __score_scq_set_schedule(struct score_scq *scq,
		struct score_frame *frame)
{
	unsigned int pos_addr, pos;
	unsigned int shift, bitmap;
	unsigned int max_count;

	score_enter();
	if (frame->priority) {
		pos_addr = SCHEDULE_HIGH_WPOS;
		shift = 16;
		max_count = scq->high_max_count;
	} else {
		pos_addr = SCHEDULE_NORMAL_WPOS;
		shift = 0;
		max_count = scq->normal_max_count;
	}

	pos = readl(scq->sfr + pos_addr);
	bitmap = readl(scq->sfr + SCHEDULE_BITMAP);

	if (frame->task_type & HOST_TKT_BOUND)
		bitmap |= (0x1 << (shift + pos));
	else
		bitmap &= ~(0x1 << (shift + pos));

	if (pos + 1 > max_count)
		pos = 0;
	else
		pos++;

	writel(bitmap, scq->sfr + SCHEDULE_BITMAP);
	writel(pos, scq->sfr + pos_addr);
	score_leave();
}

static int __score_scq_write(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	unsigned int elem;
	unsigned int scq_cmd = 0;
	unsigned int scq_info;

	score_enter();
	/* [17:8] size of command msg to be send */
	scq_cmd |= ((scq->out_size) << 8);
	/* [7:3] receiver ID */
	scq_cmd |= ((scq->slave_id) << 3);
	/* [1] priority : 0(normal) / 1(high) */
	scq_cmd |= (scq->priority << 1);
	/* [0] queue_set : 1(valid) / 0(invalid) */
	scq_cmd |= 0x1;

	writel(scq_cmd, scq->sfr + SCQ_CMD_SET);
	scq_info = readl(scq->sfr + SCQ_SET_INFO);

	frame->timestamp[SCORE_TIME_SCQ_WRITE] = score_util_get_timespec();
	/* [31] status : 0(pass) / 1(fail) */
	if (scq_info & (0x1 << 30)) {
		score_warn("Writing request(%u-%u,%u) failed [%8x]\n",
				frame->sctx->id, frame->frame_id,
				frame->kernel_id, scq_info);
		ret = -EBUSY;
		goto p_err;
	} else {
		for (elem = 0; elem < scq->out_size; ++elem) {
			writel(scq->out_param[elem],
					scq->sfr + SCQ_SRAM_CMD_DATA);
			if (elem == (scq->out_size - 2))
				__score_scq_set_schedule(scq, frame);
		}
	}
	score_leave();
p_err:
	return ret;
}

static int __score_scq_read(struct score_scq *scq)
{
	int ret = 0;
	unsigned int elem;
	unsigned int scq_info;

	score_enter();
	writel(0x1, scq->sfr + SCQ_CMD_GET);
	scq_info = readl(scq->sfr + SCQ_GET_INFO);

	/* [31] error : 0(pass) / 1(fail) */
	if (scq_info & (0x1 << 30)) {
		score_err("Read queue of scq is empty [%8x]\n", scq_info);
		ret = -ENOMEM;
		goto p_err;
	} else {
		/* [17:8] size of received command msg */
		scq->in_size = (scq_info >> 8) & 0x3ff;
		/* [4:0] master id */
		scq->master_id = scq_info & 0x1f;

		if (scq->in_size != RESULT_SCQ_SIZE) {
			score_err("result size(%u) is invalid (master:%u)\n",
					scq->in_size, scq->master_id);
			for (elem = 0; elem < scq->in_size; ++elem)
				score_err("SCQ_READ[%u] %8x\n", elem,
						readl(scq->sfr +
							SCQ_SRAM_CMD_DATA));
			goto p_err;
		} else {
			for (elem = 0; elem < scq->in_size; ++elem)
				scq->in_param[elem] = readl(scq->sfr +
						SCQ_SRAM_CMD_DATA);
		}
	}
	score_leave();
p_err:
	return ret;
}

int score_scq_read(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (!(scq->state & BIT(SCORE_SCQ_STATE_START))) {
		score_warn("scq is not started\n");
		ret = -ENOSTR;
		goto p_err;
	}

	ret = __score_scq_read(scq);
	if (ret)
		goto p_err;

	/* set return info from packet to frame */
	frame->frame_id = (scq->in_param[1] >> 24);
	frame->ret = scq->in_param[2];
	if (scq->in_param[3]) {
		frame->priority = true;
		scq->high_count--;
	} else {
		frame->priority = false;
		scq->normal_count--;
	}

	scq->count--;

	score_leave();
p_err:
	return ret;
}

int score_scq_write(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (scq->state & BIT(SCORE_SCQ_STATE_CLOSE)) {
		score_err("scq is closed %8x\n", scq->state);
		ret = -ENOSTR;
		goto p_err;
	} else if (scq->state & BIT(SCORE_SCQ_STATE_OPEN)) {
		ret = __score_scq_wait_init(scq, frame);
		if (ret)
			goto p_err;
	}

	if (!frame->pended && score_frame_get_pending_count(frame->owner,
				frame->priority)) {
#ifdef DEBUG_PENDING_LOG
		score_warn("pended task remained (%u/%u,%u/%u)\n",
				scq->normal_count,
				scq->normal_max_count,
				scq->high_count, scq->high_max_count);
#endif
		goto p_pending;
	}

	if (frame->priority) {
		if (scq->high_count + 1 > scq->high_max_count) {
#ifdef DEBUG_PENDING_LOG
			score_warn("high task was pended (%u/%u,%u/%u)\n",
					scq->normal_count,
					scq->normal_max_count,
					scq->high_count, scq->high_max_count);
#endif
			goto p_pending;
		}
	} else {
		if (scq->normal_count + 1 > scq->normal_max_count) {
#ifdef DEBUG_PENDING_LOG
			score_warn("normal task was pended (%u/%u,%u/%u)\n",
					scq->normal_count,
					scq->normal_max_count,
					scq->high_count, scq->high_max_count);
#endif
			goto p_pending;
		}
	}

	ret = __score_scq_create(scq, frame);
	if (ret)
		goto p_err;

	ret = __score_scq_write(scq, frame);
	if (ret)
		goto p_pending;

	scq->count++;
	if (frame->priority)
		scq->high_count++;
	else
		scq->normal_count++;

	score_leave();
p_err:
	if (frame->pended) {
		kfree(frame->pending_packet);
		frame->pended = false;
	}

	return ret;
p_pending:
	ret = __score_scq_pending_create(scq, frame);
	if (!ret)
		ret = -EBUSY;
	return ret;
}

static int __score_scq_translate_buffer(struct score_frame *frame,
		struct score_host_buffer *buffer, unsigned int count,
		unsigned char *data, unsigned int max_size)
{
	int ret;
	unsigned int idx;
	unsigned int offset;
	unsigned int type;

	struct score_mmu_buffer *kbuf;
	unsigned int *taddr;

	for (idx = 0; idx < count; ++idx) {
		offset = buffer[idx].offset;
		if (offset >= max_size) {
			ret = -EINVAL;
			score_err("invalid offset(%u) (max:%u)\n",
					offset, max_size);
			goto p_err;
		}

		type = buffer[idx].type;
		switch (type) {
		case TASK_ID_TYPE:
			data[offset] = (unsigned char)frame->frame_id;
			break;
		case MEMORY_TYPE:
			/* taddr will point to the valid packet memory */
			if (offset > max_size - sizeof(unsigned int) + 1) {
				ret = -EINVAL;
				score_err("invalid offset(%u) (max:%zu)\n",
						offset, max_size -
						sizeof(unsigned int) + 1);
				goto p_err;
			}

			kbuf = kzalloc(sizeof(*kbuf), GFP_KERNEL);
			if (!kbuf) {
				ret = -ENOMEM;
				score_err("Fail to alloc mmu buffer\n");
				goto p_err;
			}

			kbuf->type = buffer[idx].memory_type;
			kbuf->size = buffer[idx].memory_size;
			kbuf->m.mem_info = buffer[idx].m.mem_info;
			kbuf->offset = buffer[idx].addr_offset;
			kbuf->mirror = false;

			ret = score_mmu_map_buffer(frame->sctx->mmu_ctx, kbuf);
			if (ret) {
				kfree(kbuf);
				goto p_err;
			}

			score_frame_add_buffer(frame, kbuf);

			taddr = (unsigned int *)(data + offset);
			*taddr = (unsigned int)(kbuf->dvaddr + kbuf->offset);
			break;
		default:
			ret = -EINVAL;
			score_err("wrong buffer type: %u\n", type);
			goto p_err;
		}
	}

	return 0;
p_err:
	score_err("Failed to translate [%u/%u] buffer\n", idx, count);
	return ret;
}

static int __score_scq_translate_packet(struct score_frame *frame)
{
	int ret = 0;

	struct score_host_packet *packet;
	unsigned int packet_size;
	struct score_host_packet_info *packet_info;
	struct score_host_buffer *buffers;
	unsigned char *target_packet;
	size_t size;
	unsigned int buf_count, valid_size, packet_offset;

	score_enter();
	packet = frame->packet;
	packet_size = packet->size;

	if (packet->version != HOST_PKT_V1)
		score_warn("packet version is not V1 (%d)\n", packet->version);

	if (packet_size > frame->packet_size) {
		ret = -EINVAL;
		score_err("packet size is larger than buffer (%u/%zu)\n",
				packet_size, frame->packet_size);
		goto p_err;
	}

	if (packet_size < MIN_PACKET_SIZE ||
			packet_size > MAX_PACKET_SIZE) {
		ret = -EINVAL;
		score_err("packet size is invalid (%u/MIN:%zu/MAX:%zu)\n",
				packet_size, MIN_PACKET_SIZE, MAX_PACKET_SIZE);
		goto p_err;
	}

	packet_info = (struct score_host_packet_info *)&packet->payload[0];

	/*
	 * check if buf count is valid
	 * packet_size = MIN_PACKET_SIZE + packet_info->buf_count *
	 * sizeof(struct score_host_buffer) + target_packet_size
	 */

	/* host buffers size */
	buf_count = packet_info->buf_count;
	size = buf_count * sizeof(struct score_host_buffer);
	if (size > packet_size - MIN_PACKET_SIZE) {
		ret = -EINVAL;
		score_err("size of host buffers is too large (%zu/%zu)\n",
				size, packet_size - MIN_PACKET_SIZE);
		goto p_err;
	}

	/*
	 * packet->packet_offset is address offset value between
	 * score_host_packet and target_packet and equals to
	 * sizeof(struct sc_host_packet_info) + host buffers size
	 */
	size = sizeof(struct score_host_packet_info) +
		buf_count * sizeof(struct score_host_buffer);
	packet_offset = packet->packet_offset;
	if (packet_offset != size) {
		ret = -EINVAL;
		score_err("packet_offset is invalid (%u != %zu)\n",
				packet_offset, size);
		goto p_err;
	}

	buffers = (struct score_host_buffer *)&packet_info->payload[0];
	target_packet = (unsigned char *)packet_info + packet_offset;

	/*
	 * packet_info->valid_size is size of packet which is being
	 * send to target.
	 */
	valid_size = packet_info->valid_size;
	size = packet_size - (sizeof(struct score_host_packet) +
		sizeof(struct score_host_packet_info) +
		buf_count * sizeof(struct score_host_buffer));
	if (valid_size != size) {
		ret = -EINVAL;
		score_err("size of target packet is invalid (%u != %zu)\n",
				valid_size, size);
		goto p_err;
	}

	ret = __score_scq_translate_buffer(frame, buffers,
			buf_count, target_packet,
			valid_size);
	if (ret)
		goto p_err;

	frame->task_type = packet_info->task_type;
	if (frame->task_type & HOST_TKT_PRIORITY)
		frame->priority = true;
	else
		frame->priority = false;

	score_leave();
p_err:
	return ret;
}

static void score_scq_write_thread(struct kthread_work *work)
{
	int ret;
	unsigned long flags;
	struct score_frame *frame;
	struct score_frame_manager *framemgr;
	struct score_scq *scq;

	score_enter();
	frame = container_of(work, struct score_frame, work);
	framemgr = frame->owner;

	spin_lock_irqsave(&framemgr->slock, flags);
	if (frame->state == SCORE_FRAME_STATE_COMPLETE)
		goto p_exit;

	ret = score_frame_trans_ready_to_process(frame);
	if (ret) {
		score_frame_trans_any_to_complete(frame, ret);
		goto p_exit;
	}

	scq = &frame->sctx->system->scq;
	ret = score_scq_write(scq, frame);
	if (ret == -EBUSY) {
#ifdef DEBUG_PENDING_LOG
		score_warn("frame is pended (%u-%u, %u)\n",
				frame->sctx->id, frame->frame_id,
				frame->kernel_id);
#endif
		score_frame_trans_process_to_pending(frame);
		goto p_exit;
	} else if (ret) {
		score_err("frame is abnormaly completed (%u-%u, %u)\n",
				frame->sctx->id, frame->frame_id,
				frame->kernel_id);
		score_frame_trans_process_to_complete(frame, ret);
		goto p_exit;
	}

	score_leave();
p_exit:
	spin_unlock_irqrestore(&framemgr->slock, flags);
}

int score_scq_send_packet(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	unsigned long flags;

	score_enter();
	ret = __score_scq_translate_packet(frame);
	if (ret) {
		spin_lock_irqsave(&frame->owner->slock, flags);
		score_frame_trans_ready_to_complete(frame, ret);
		spin_unlock_irqrestore(&frame->owner->slock, flags);
	} else {
		kthread_init_work(&frame->work, score_scq_write_thread);
		score_scq_write_thread(&frame->work);
	}

	score_leave();
	return 0;
}

void score_scq_init(struct score_scq *scq)
{
	score_enter();
	scq->count = 0;
	scq->high_count = 0;
	scq->normal_count = 0;

	writel(0x0, scq->sfr + SCHEDULE_BITMAP);
	writel(0x0, scq->sfr + SCHEDULE_NORMAL_WPOS);
	writel(0x0, scq->sfr + SCHEDULE_NORMAL_RPOS);
	writel(0x0, scq->sfr + SCHEDULE_HIGH_WPOS);
	writel(0x0, scq->sfr + SCHEDULE_HIGH_RPOS);
	score_leave();
}

int score_scq_open(struct score_scq *scq)
{
	score_enter();
	score_scq_init(scq);
	scq->state = BIT(SCORE_SCQ_STATE_OPEN);
	score_leave();
	return 0;
}

int score_scq_close(struct score_scq *scq)
{
	score_enter();
	scq->state = BIT(SCORE_SCQ_STATE_CLOSE);
	score_leave();
	return 0;
}

int score_scq_probe(struct score_system *system)
{
	int ret = 0;
	struct score_scq *scq;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	score_enter();
	scq = &system->scq;
	scq->sfr = system->sfr;

	kthread_init_worker(&scq->write_worker);

	scq->write_task = kthread_run(kthread_worker_fn,
			&scq->write_worker, "score_scq");
	if (IS_ERR(scq->write_task)) {
		ret = PTR_ERR(scq->write_task);
		score_err("scq_write kthread is not running (%d)\n", ret);
		goto p_err_run_write;
	}

	ret = sched_setscheduler_nocheck(scq->write_task, SCHED_FIFO, &param);
	if (ret) {
		score_err("scheduler setting of scq_write is fail(%d)\n", ret);
		goto p_err_sched_write;
	}

	scq->state = BIT(SCORE_SCQ_STATE_CLOSE);
	score_leave();
	return ret;
p_err_sched_write:
	kthread_stop(scq->write_task);
p_err_run_write:
	return ret;
}

int score_scq_remove(struct score_scq *scq)
{
	score_enter();
	kthread_stop(scq->write_task);

	scq->sfr = NULL;
	score_leave();
	return 0;
}
