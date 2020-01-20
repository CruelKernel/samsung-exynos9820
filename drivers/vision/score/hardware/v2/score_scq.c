/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
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

#include "score_log.h"
#include "score_scq.h"
#include "score_regs.h"
#include "score_packet.h"
#include "score_system.h"
#include "score_context.h"
#include "score_frame.h"

#if defined(CONFIG_EXYNOS_SCORE)
#define MC_INIT_TIMEOUT		100
#else /* FPGA */
#define MC_INIT_TIMEOUT		300
#endif

/**
 * __score_scq_wait_for_mc
 *@brief : wait for mc_init_done before scq_write
 *
 */
static int __score_scq_wait_mc(struct score_scq *scq, struct score_frame *frame)
{
	int ready;
	int time_count = MC_INIT_TIMEOUT;
	int i;

	score_enter();

	for (i = 0; i < time_count; i++) {
		ready = readl(scq->sfr + MC_INIT_DONE_CHECK);
		if ((ready & 0xffff0000) != 0)
			return 0;
		mdelay(1);
	}

	score_err("Init time(%dms)is over\n", MC_INIT_TIMEOUT);
	score_leave();
	return -ETIMEDOUT;
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
		score_err("Fail to alloc packet for pending frame\n");
		goto p_err;
	}

	memcpy(frame->pending_packet, packet, size);
	frame->pended = true;

	score_leave();
p_err:
	return ret;
}

/**
 * __score_scq_create
 *@brief : copy command from packet to scq and setting basic scq parameter
 *
 */
static int __score_scq_create(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	struct score_host_packet *packet;
	struct score_host_packet_info *packet_info;
	unsigned char *target_packet;
	unsigned int *data;
	unsigned int size;
	int i;

	score_enter();
	if (frame->pended)
		packet = frame->pending_packet;
	else
		packet = frame->packet;

	packet_info = (struct score_host_packet_info *)&packet->payload[0];
	target_packet = (unsigned char *)packet_info + packet->packet_offset;

	data = (unsigned int *)target_packet;
	size = packet_info->valid_size >> 2;
	if (size > MAX_SCQ_SIZE) {
		score_err("size(%u) is too big compared to scq(%u)\n",
				size, MAX_SCQ_SIZE);
		ret = -EINVAL;
		goto p_err;
	}

	for (i = 0; i < size; i++)
		scq->out_param[i] = data[i];

	scq->out_size = size;
	scq->slave_id = 2;  //MASTER 2
	scq->priority = 0;

	score_leave();
p_err:
	return ret;
}

/**
 * __score_scq_write
 *@brief : atomic scq write sequence
 */
static int __score_scq_write(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	int elem;
	int scq_mset = 0;
	int scq_minfo;

	score_enter();
	//[17:8]	size of command msg to be send
	scq_mset |= ((scq->out_size) << 8);
	//[7:3]		sender ID
	scq_mset |= ((scq->slave_id) << 3);
	// [1] - priority / 0 : normal / 1: high
	scq_mset |= (scq->priority << 1);
	// [0] - queue_set / 1 : valid
	scq_mset |= 0x1;
	//SCQ_MSET write
	writel(scq_mset, scq->sfr + SCQ_MSET);
	//SCQ_MINFO read
	scq_minfo = readl(scq->sfr + SCQ_MINFO);

	frame->timestamp[SCORE_TIME_SCQ_WRITE] = score_util_get_timespec();
	// [31] - error / 0 : success / 1 : fail
	if ((scq_minfo & 0x80000000) == 0x80000000) {
		score_warn("Request(%u-%u, %u) to scq failed [%8x]\n",
				frame->sctx->id, frame->frame_id,
				frame->kernel_id, scq_minfo);
#if defined(SCORE_EVT1)
		score_warn("Packet count of each core (%2u/%2u/%2u/%2u/%2u)\n",
				readl(scq->sfr + SCQ_APCPU_PACKET_NUM),
				readl(scq->sfr + SCQ_MC_PACKET_NUM),
				readl(scq->sfr + SCQ_KC1_PACKET_NUM),
				readl(scq->sfr + SCQ_KC2_PACKET_NUM),
				readl(scq->sfr + SCQ_IVA_PACKET_NUM));
#endif
		ret = -EBUSY;
		goto p_err;
	} else {
#if defined(SCORE_EVT0)
		for (elem = 0; elem < scq->out_size; ++elem)
			writel(scq->out_param[elem], scq->sfr + SCQ_SRAM);
#else
		//SCQ out_param write to SRAM, SRAM1
		for (elem = 0; elem < (scq->out_size & 0x3fe); elem += 2) {
			writel(scq->out_param[elem], scq->sfr + SCQ_SRAM);
			writel(scq->out_param[elem + 1], scq->sfr + SCQ_SRAM1);
		}
		if (scq->out_size & 0x1)
			writel(scq->out_param[scq->out_size - 1],
					scq->sfr + SCQ_SRAM);
#endif
	}
	score_leave();
p_err:
	return ret;
}

/**
 * __score_scq_read
 *@brief : atomic scq read sequence
 */
static int __score_scq_read(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;
	int elem;
	int scq_sinfo;

	score_enter();
	//SCQ_TAKEN
	writel(0x1, scq->sfr + SCQ_STAKEN);
	//SCQ_SINFO read
	scq_sinfo = readl(scq->sfr + SCQ_SINFO);

	// [31] - error / 0 : success / 1 : fail
	if ((scq_sinfo & 0x80000000) == 0x80000000) {
		score_err("Read queue of scq is empty\n");
		ret = -ENOMEM;
		goto p_err;
	} else {
		// [17:8] - size of received command msg
		scq->in_size = (scq_sinfo >> 8) & 0x3ff;
		// [4:0] - master id
		scq->master_id = (scq_sinfo) & 0x1f;
#if defined(SCORE_EVT0)
		for (elem = 0; elem < scq->in_size; ++elem)
			scq->in_param[elem] = readl(scq->sfr + SCQ_SRAM);
#else
		//SCQ in_param read from SRAM, SRAM1
		for (elem = 0; elem < (scq->in_size & 0x3fe); elem += 2) {
			scq->in_param[elem] = readl(scq->sfr + SCQ_SRAM);
			scq->in_param[elem + 1] = readl(scq->sfr + SCQ_SRAM1);
		}
		if (scq->in_size & 0x1)
			scq->in_param[scq->in_size - 1] = readl(scq->sfr +
					SCQ_SRAM);
#endif
	}
	score_leave();
p_err:
	return ret;
}

int score_scq_read(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (!(scq->state & BIT(SCORE_SCQ_STATE_OPEN))) {
		score_warn("reading scq is already closed\n");
		ret = -ENOSTR;
		goto p_err;
	}

	ret = __score_scq_read(scq, frame);
	if (ret)
		goto p_err;

	// set return info from packet to frame
	frame->frame_id = (scq->in_param[1] >> 24);
	frame->ret = scq->in_param[2];

	scq->count--;

	score_leave();
p_err:
	return ret;
}

int score_scq_write(struct score_scq *scq, struct score_frame *frame)
{
	int ret = 0;

	score_enter();
	if (!(scq->state & BIT(SCORE_SCQ_STATE_OPEN))) {
		score_warn("writing scq is already closed\n");
		ret = -ENOSTR;
		goto p_err;
	}

	if (scq->count + 1 > MAX_HOST_TASK_COUNT) {
		score_warn("task will be pending (size:%d/%d)\n",
				scq->count, MAX_HOST_TASK_COUNT);
		ret = __score_scq_pending_create(scq, frame);
		if (!ret)
			return -EBUSY;
		goto p_err;
	}

	ret = __score_scq_create(scq, frame);
	if (ret)
		goto p_err;

	if (scq->count == 0) {
		ret = __score_scq_wait_mc(scq, frame);
		if (ret)
			goto p_err;
	}

	ret = __score_scq_write(scq, frame);
	if (ret) {
		ret = __score_scq_pending_create(scq, frame);
		if (!ret)
			return -EBUSY;
		goto p_err;
	}

	scq->count++;

	score_leave();
p_err:
	if (frame->pended) {
		kfree(frame->pending_packet);
		frame->pended = false;
	}

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

	score_enter();
	packet = frame->packet;
	packet_size = packet->size;

	if (packet_size > frame->packet_size) {
		ret = -EINVAL;
		score_err("packet size is larger than buffer (%u/%zu)\n",
				packet_size, frame->packet_size);
		goto p_err;
	}

	if (packet_size < MIN_PACKET_SIZE ||
			packet_size > MAX_PACKET_SIZE) {
		ret = -EINVAL;
		score_err("packet size is invalid (%u/MIN:%lu/MAX:%u)\n",
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
	size = packet_info->buf_count * sizeof(struct score_host_buffer);
	if (size > packet_size - MIN_PACKET_SIZE) {
		ret = -EINVAL;
		score_err("size of host buffers is too large (%zu/%lu)\n",
				size, packet_size - MIN_PACKET_SIZE);
		goto p_err;
	}

	/*
	 * packet->packet_offset is address offset value between
	 * score_host_packet and target_packet and equals to
	 * sizeof(struct sc_host_packet_info) + host buffers size
	 */
	size = sizeof(struct score_host_packet_info) +
		packet_info->buf_count * sizeof(struct score_host_buffer);
	if (packet->packet_offset != size) {
		ret = -EINVAL;
		score_err("packet_offset is invalid (%u != %zu)\n",
				packet->packet_offset, size);
		goto p_err;
	}

	buffers = (struct score_host_buffer *)&packet_info->payload[0];
	target_packet = (unsigned char *)packet_info + packet->packet_offset;

	/*
	 * packet_info->valid_size is size of packet which is being
	 * send to target.
	 */
	size = packet_size - (sizeof(struct score_host_packet) +
		sizeof(struct score_host_packet_info) +
		packet_info->buf_count * sizeof(struct score_host_buffer));
	if (packet_info->valid_size != size) {
		ret = -EINVAL;
		score_err("size of target packet is invalid (%u != %zu)\n",
				packet_info->valid_size, size);
		goto p_err;
	}

	ret = __score_scq_translate_buffer(frame, buffers,
			packet_info->buf_count, target_packet,
			packet_info->valid_size);
	if (ret)
		goto p_err;

	/* priority and optional task type are  not supported at V2 */
	frame->task_type = 0;
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
	if (ret) {
		if (ret == -EBUSY) {
			score_warn("frame is pended (%u-%u, %u)\n",
					frame->sctx->id, frame->frame_id,
					frame->kernel_id);
			score_frame_trans_process_to_pending(frame);
		} else {
			score_err("frame is abnormaly completed (%u-%u, %u)\n",
					frame->sctx->id, frame->frame_id,
					frame->kernel_id);
			score_frame_trans_process_to_complete(frame, ret);
		}

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
