/*
 * drivers/media/platform/exynos/mfc/mfc_queue.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_QUEUE_H
#define __MFC_QUEUE_H __FILE__

#include  "mfc_common.h"

/**
 * enum mfc_queue_used_type
 */
enum mfc_queue_used_type {
	MFC_BUF_NO_TOUCH_USED	= -1,
	MFC_BUF_RESET_USED	= 0,
	MFC_BUF_SET_USED	= 1,
};

/**
 * enum mfc_queue_top_type
 */
enum mfc_queue_top_type {
	MFC_QUEUE_ADD_BOTTOM	= 0,
	MFC_QUEUE_ADD_TOP	= 1,
};

static inline unsigned int mfc_get_queue_count(spinlock_t *plock, struct mfc_buf_queue *queue)
{
	unsigned long flags;
	unsigned int ret = 0;

	spin_lock_irqsave(plock, flags);
	ret = queue->count;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline int mfc_is_queue_count_same(spinlock_t *plock, struct mfc_buf_queue *queue,
		unsigned int value)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(plock, flags);
	if (queue->count == value)
		ret = 1;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline int mfc_is_queue_count_greater(spinlock_t *plock, struct mfc_buf_queue *queue,
		unsigned int value)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(plock, flags);
	if (queue->count > value)
		ret = 1;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline int mfc_is_queue_count_smaller(spinlock_t *plock, struct mfc_buf_queue *queue,
		unsigned int value)
{
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(plock, flags);
	if (queue->count < value)
		ret = 1;
	spin_unlock_irqrestore(plock, flags);

	return ret;
}

static inline void mfc_init_queue(struct mfc_buf_queue *queue)
{
	INIT_LIST_HEAD(&queue->head);
	queue->count = 0;
}

static inline void mfc_create_queue(struct mfc_buf_queue *queue)
{
	mfc_init_queue(queue);
}

static inline void mfc_delete_queue(struct mfc_buf_queue *queue)
{
	mfc_init_queue(queue);
}

void mfc_add_tail_buf(spinlock_t *plock, struct mfc_buf_queue *queue,
		struct mfc_buf *mfc_buf);

struct mfc_buf *mfc_get_buf(spinlock_t *plock, struct mfc_buf_queue *queue,
		enum mfc_queue_used_type used);
struct mfc_buf *mfc_get_del_buf(spinlock_t *plock, struct mfc_buf_queue *queue,
		enum mfc_queue_used_type used);
struct mfc_buf *mfc_get_del_if_consumed(struct mfc_ctx *ctx, struct mfc_buf_queue *queue,
		unsigned long consumed, unsigned int min_bytes, int err, int *deleted);
struct mfc_buf *mfc_get_move_buf(spinlock_t *plock,
		struct mfc_buf_queue *to_queue, struct mfc_buf_queue *from_queue,
		enum mfc_queue_used_type used, enum mfc_queue_top_type top);
struct mfc_buf *mfc_get_move_buf_used(spinlock_t *plock,
		struct mfc_buf_queue *to_queue, struct mfc_buf_queue *from_queue);
struct mfc_buf *mfc_get_move_buf_addr(spinlock_t *plock,
		struct mfc_buf_queue *to_queue, struct mfc_buf_queue *from_queue,
		dma_addr_t addr);

struct mfc_buf *mfc_find_first_buf(spinlock_t *plock, struct mfc_buf_queue *queue,
		dma_addr_t addr);
struct mfc_buf *mfc_find_buf(spinlock_t *plock, struct mfc_buf_queue *queue,
		dma_addr_t addr);
struct mfc_buf *mfc_find_del_buf(spinlock_t *plock, struct mfc_buf_queue *queue,
		dma_addr_t addr);
struct mfc_buf *mfc_find_move_buf(spinlock_t *plock,
		struct mfc_buf_queue *to_queue, struct mfc_buf_queue *from_queue,
		dma_addr_t addr, unsigned int released_flag);
struct mfc_buf *mfc_find_move_buf_used(spinlock_t *plock,
		struct mfc_buf_queue *to_queue, struct mfc_buf_queue *from_queue,
		dma_addr_t addr);

void mfc_move_all_bufs(spinlock_t *plock, struct mfc_buf_queue *to_queue,
		struct mfc_buf_queue *from_queue, enum mfc_queue_top_type top);

void mfc_cleanup_queue(spinlock_t *plock, struct mfc_buf_queue *queue);

void mfc_handle_released_info(struct mfc_ctx *ctx,
		unsigned int released_flag, int index);

struct mfc_buf *mfc_move_reuse_buffer(struct mfc_ctx *ctx, int release_index);

void mfc_cleanup_enc_src_queue(struct mfc_ctx *ctx);
void mfc_cleanup_enc_dst_queue(struct mfc_ctx *ctx);

struct mfc_buf *mfc_search_for_dpb(struct mfc_ctx *ctx, unsigned int dynamic_used);
struct mfc_buf *mfc_search_move_dpb_nal_q(struct mfc_ctx *ctx, unsigned int dynamic_used);
void mfc_store_dpb(struct mfc_ctx *ctx, struct vb2_buffer *vb);

void mfc_cleanup_nal_queue(struct mfc_ctx *ctx);

int mfc_check_buf_vb_flag(struct mfc_ctx *ctx, enum mfc_vb_flag f);

#endif /* __MFC_QUEUE_H */
