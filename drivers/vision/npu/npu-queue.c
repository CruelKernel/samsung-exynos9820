/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "npu-queue.h"
#include "npu-log.h"

/*
 * queue: npu_queue instance strucure
 * memory: Reference to memory management helper
 * lock: Session-wide mutex to protect access to vb_queue
 */

int npu_queue_open(struct npu_queue *queue,
	struct npu_memory *memory,
	struct mutex *lock)
{
	int ret = 0;
	struct vb_queue *inq, *otq;

	queue->qops = &npu_queue_ops;
	inq = &queue->inqueue;
	otq = &queue->otqueue;
	inq->private_data = queue;
	otq->private_data = queue;

	ret = vb_queue_init(inq, memory->dev, memory->vb2_mem_ops, &vb_ops, lock, VS4L_DIRECTION_IN);
	if (ret) {
		npu_err("fail(%d) in vb_queue_init\n", ret);
		goto p_err;
	}

	ret = vb_queue_init(otq, memory->dev, memory->vb2_mem_ops, &vb_ops, lock, VS4L_DIRECTION_OT);
	if (ret) {
		npu_err("fail(%d) in vb_queue_init\n", ret);
		goto p_err;
	}
p_err:
	return ret;
}

int npu_queue_s_format(struct npu_queue *queue, struct vs4l_format_list *f)
{
	int ret = 0;
	struct vb_queue *q, *inq, *otq;

	BUG_ON(!queue);
	BUG_ON(!f);

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	if (f->direction == VS4L_DIRECTION_IN)
		q = inq;
	else
		q = otq;

	ret = vb_queue_s_format(q, f);
	if (ret) {
		npu_err("fail(%d) in vb_queue_s_format\n", ret);
		goto p_err;
	}

	npu_trace("dir(%u), q->state = %lu\n", f->direction, q->state);
	ret = CALL_QOPS(queue, format, f);
	if (ret) {
		npu_err("fail(%d) in CALL_QOPS(format)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int npu_queue_start(struct npu_queue *queue)
{
	int ret = 0;
	struct vb_queue *inq, *otq;

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	ret = vb_queue_start(inq);
	if (ret) {
		npu_err("fail(%d) in vb_queue_init\n", ret);
		goto p_err;
	}

	ret = vb_queue_start(otq);
	if (ret) {
		npu_err("fail(%d) in vb_queue_init\n", ret);
		goto p_err;
	}

	ret = CALL_QOPS(queue, start);
	if (ret) {
		npu_err("fail(%d) in CALL_QOPS(start)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int npu_queue_streamoff(struct npu_queue *queue)
{
	int ret = 0;

	ret = CALL_QOPS(queue, streamoff);
	if (ret) {
		npu_err("fail(%d) in CALL_QOPS(stop)\n", ret);
		goto p_err;
	}
p_err:
	return ret;
}

int npu_queue_stop(struct npu_queue *queue, int is_forced)
{
	int ret = 0;
	struct vb_queue *inq, *otq;

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	if (!test_bit(VB_QUEUE_STATE_START, &inq->state) && test_bit(VB_QUEUE_STATE_FORMAT, &inq->state)) {
		npu_info("already npu_queue_stop inq done\n");
		goto p_err;
	}

	if (!test_bit(VB_QUEUE_STATE_START, &otq->state) && test_bit(VB_QUEUE_STATE_FORMAT, &otq->state)) {
		npu_info("already npu_queue_stop otq done\n");
		goto p_err;
	}

	if (!is_forced)
		ret = vb_queue_stop(inq);
	else
		ret = vb_queue_stop_forced(inq);

	if (ret) {
		npu_err("fail(%d) in vb_queue_stop%s(inq)\n", ret, (is_forced)?"_forced":"");
		goto p_err;
	}

	if (!is_forced)
		ret = vb_queue_stop(otq);
	else
		ret = vb_queue_stop_forced(otq);

	if (ret) {
		npu_err("fail(%d) in vb_queue_stop%s(otq)\n", ret, (is_forced)?"_forced":"");
		goto p_err;
	}

	ret = CALL_QOPS(queue, stop);
	if (ret) {
		npu_err("fail(%d) in CALL_QOPS(stop)\n", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int npu_queue_poll(struct npu_queue *queue, struct file *file, poll_table *poll)
{
	int ret = 0;
	struct vb_queue *inq, *otq;
	unsigned long events;
	u32 done_cnt;

	BUG_ON(!queue);
	BUG_ON(!file);
	BUG_ON(!poll);

	events = poll_requested_events(poll);

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	if (events & POLLOUT) {
		done_cnt = (u32)atomic_read(&inq->done_count);
		if (list_empty(&inq->done_list)) {
			poll_wait(file, &inq->done_wq, poll);
		}

		if (!list_empty(&inq->done_list)) {
			ret |= POLLOUT | POLLWRNORM;
		}
	}

	if (events & POLLIN) {
		done_cnt = (u32)atomic_read(&otq->done_count);
		if (list_empty(&otq->done_list)) {
			poll_wait(file, &otq->done_wq, poll);
		}

		if (!list_empty(&otq->done_list)) {
			ret |= POLLIN | POLLWRNORM;
		}
	}

	return ret;
}

int npu_queue_qbuf(struct npu_queue *queue, struct vs4l_container_list *c)
{
	int ret = 0;
	struct vb_queue *q, *inq, *otq;
	struct vb_bundle *invb, *otvb;
	struct vb_bundle *tmp_vb;

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	if (c->direction == VS4L_DIRECTION_IN)
		q = inq;
	else
		q = otq;


	ret = vb_queue_qbuf(q, c);
	if (ret) {
		npu_err("vb_queue_qbuf is fail(%d)\n", ret);
		goto p_err;
	}

	invb = NULL;
	list_for_each_entry(tmp_vb, &inq->queued_list, queued_entry) {
		if (tmp_vb->state == VB_BUF_STATE_QUEUED) {
			invb = tmp_vb;
			break;
		}
	}

	otvb = NULL;
	list_for_each_entry(tmp_vb, &otq->queued_list, queued_entry) {
		if (tmp_vb->state == VB_BUF_STATE_QUEUED) {
			otvb = tmp_vb;
			break;
		}
	}

	if (invb == NULL || otvb == NULL) {
		goto p_err;
	}

	vb_queue_process(inq, invb);
	vb_queue_process(otq, otvb);

	ret = CALL_QOPS(queue, queue, &invb->clist, &otvb->clist);
	if (ret) {
		npu_err("fail(%d) in CALL_QOPS(queue)\n", ret);
		goto p_err;
	}
p_err:
	return ret;
}

int npu_queue_prepare(struct npu_queue *queue, struct vs4l_container_list *c)
{
	int ret = 0;
	struct vb_queue *q, *inq, *otq;

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	if (c->direction == VS4L_DIRECTION_IN)
		q = inq;
	else
		q = otq;

	ret = vb_queue_prepare(q, c);
	if (ret) {
		npu_err("vb_queue_prepare is fail(%d)\n", ret);
		goto p_err;
	}
p_err:
	return ret;
}

int npu_queue_unprepare(struct npu_queue *queue, struct vs4l_container_list *c)
{
	int ret = 0;
	struct vb_queue *q, *inq, *otq;

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	if (c->direction == VS4L_DIRECTION_IN)
		q = inq;
	else
		q = otq;

	ret = vb_queue_unprepare(q, c);
	if (ret) {
		npu_err("vb_queue_prepare is fail(%d)\n", ret);
		goto p_err;
	}
p_err:
	return ret;
}

int npu_queue_dqbuf(struct npu_queue *queue, struct vs4l_container_list *c, bool nonblocking)
{
	int ret = 0;
	struct vb_queue *q;
	struct vb_bundle *bundle;

	BUG_ON(!queue);
	if (c->direction == VS4L_DIRECTION_IN)
		q = &queue->inqueue;
	else
		q = &queue->otqueue;

	ret = vb_queue_dqbuf(q, c, nonblocking);
	if (ret) {
		if (ret != -EWOULDBLOCK)
			npu_err("fail(%d) in vb_queue_dqbuf\n", ret);
		goto p_err;
	}

	if (c->index >= NPU_MAX_BUFFER) {
		npu_err("invalid in container index(%d)\n", c->index);
		ret = -EINVAL;
		goto p_err;
	}

	bundle = q->bufs[c->index];
	if (!bundle) {
		npu_err("null in bundle(%d)\n", c->index);
		ret = -EINVAL;
		goto p_err;
	}

	if (bundle->clist.index != c->index) {
		npu_err("not matched(%d != %d) in index\n", bundle->clist.index, c->index);
		ret = -EINVAL;
		goto p_err;
	}

	ret = CALL_QOPS(queue, deque, &bundle->clist);
	if (ret) {
		npu_err("fail(%d) in CALL_QOPS(deque)\n", ret);
		goto p_err;
	}
p_err:
	return ret;
}

void npu_queue_done(struct npu_queue *queue,
	struct vb_container_list *incl,
	struct vb_container_list *otcl,
	unsigned long flags)
{
	struct vb_queue *inq, *otq;
	struct vb_bundle *invb, *otvb;
	u32 count;

	BUG_ON(!queue);
	BUG_ON(!incl);
	BUG_ON(!otcl);

	inq = &queue->inqueue;
	otq = &queue->otqueue;

	count = (u32)atomic_read(&inq->process_count);
	if (list_empty(&inq->process_list)) {
		npu_err("empty in inqueue\n");
		BUG();
	}

	if (list_empty(&otq->process_list)) {
		npu_err("empty in otqueue\n");
		BUG();
	}

	invb = container_of(incl, struct vb_bundle, clist);
	otvb = container_of(otcl, struct vb_bundle, clist);

	if (invb->state != VB_BUF_STATE_PROCESS) {
		npu_err("invalid in invb state(%d)\n", invb->state);
		BUG();
	}

	if (otvb->state != VB_BUF_STATE_PROCESS) {
		npu_err("invalid in otvb state(%d)\n", otvb->state);
		BUG();
	}

	otvb->flags |= flags;
	vb_queue_done(otq, otvb);

	invb->flags |= flags;
	vb_queue_done(inq, invb);
}

void __npu_queue_print(struct vb_queue *q)
{
	int qcnt = atomic_read(&q->queued_count);
	int pcnt = atomic_read(&q->process_count);
	int dcnt = atomic_read(&q->done_count);
	u32 dir = q->direction;
	struct list_head *head;
	struct list_head *pos;

	npu_info("[npuQ] qcnt(%d), pcnt(%d), dcnt(%d)\n", qcnt, pcnt, dcnt);
	npu_info("[npuQ] dir(0x%x), &queued_list(0x%pK), &process_list(0x%pK), &done_list(0x%pK)\n",
			dir, &q->queued_list, &q->process_list, &q->done_list);

	head = &q->queued_list;

	for (pos = head->next ; pos != head ; pos = pos->next) {
		npu_info("[npuQ] pos(0x%pK)\n", pos);
		npu_info("[npuQ] pos->prev(0x%pK)\n", pos->prev);
		npu_info("[npuQ] pos->next(0x%pK)\n", pos->next);
	}

	head = &q->process_list;

	for (pos = head->next ; pos != head ; pos = pos->next) {
		npu_info("[npuQ] pos(0x%pK)\n", pos);
		npu_info("[npuQ] pos->prev(0x%pK)\n", pos->prev);
		npu_info("[npuQ] pos->next(0x%pK)\n", pos->next);
	}

	head = &q->done_list;

	for (pos = head->next ; pos != head ; pos = pos->next) {
		npu_info("[npuQ] pos(0x%pK)\n", pos);
		npu_info("[npuQ] pos->prev(0x%pK)\n", pos->prev);
		npu_info("[npuQ] pos->next(0x%pK)\n", pos->next);
	}
	return;
}
