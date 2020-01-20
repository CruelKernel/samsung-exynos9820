/*
 * Copyright (C) 2011 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"

#ifdef GROUP_MEM_LINK_SNAPSHOT

static struct kmem_cache *msb_kmem_cache;

int msb_init(void)
{
	msb_kmem_cache = kmem_cache_create("msb_kmem_cache",
					   sizeof(struct mst_buff),
					   0,
					   (SLAB_HWCACHE_ALIGN | SLAB_PANIC),
					   NULL);
	if (!msb_kmem_cache)
		return -ENOMEM;

	return 0;
}

struct mst_buff *msb_alloc(void)
{
	return kmem_cache_zalloc(msb_kmem_cache, GFP_ATOMIC);
}

void msb_free(struct mst_buff *msb)
{
	kmem_cache_free(msb_kmem_cache, msb);
}

static inline void __msb_queue_head_init(struct mst_buff_head *list)
{
	list->prev = list->next = (struct mst_buff *)list;
	list->qlen = 0;
}

void msb_queue_head_init(struct mst_buff_head *list)
{
	spin_lock_init(&list->lock);
	__msb_queue_head_init(list);
}

static inline void __msb_insert(struct mst_buff *msb,
				struct mst_buff *prev, struct mst_buff *next,
				struct mst_buff_head *list)
{
	msb->next = next;
	msb->prev = prev;
	next->prev = prev->next = msb;
	list->qlen++;
}

static inline void __msb_queue_before(struct mst_buff_head *list,
				      struct mst_buff *next,
				      struct mst_buff *msb)
{
	__msb_insert(msb, next->prev, next, list);
}

static inline void __msb_queue_after(struct mst_buff_head *list,
				     struct mst_buff *prev,
				     struct mst_buff *msb)
{
	__msb_insert(msb, prev, prev->next, list);
}

static inline void __msb_queue_tail(struct mst_buff_head *list,
				    struct mst_buff *msb)
{
	__msb_queue_before(list, (struct mst_buff *)list, msb);
}

static inline void __msb_queue_head(struct mst_buff_head *list,
				    struct mst_buff *msb)
{
	__msb_queue_after(list, (struct mst_buff *)list, msb);
}

void msb_queue_tail(struct mst_buff_head *list, struct mst_buff *msb)
{
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);

	__msb_queue_tail(list, msb);

	spin_unlock_irqrestore(&list->lock, flags);
}

void msb_queue_head(struct mst_buff_head *list, struct mst_buff *msb)
{
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);

	__msb_queue_head(list, msb);

	spin_unlock_irqrestore(&list->lock, flags);
}

static inline struct mst_buff *__msb_peek(const struct mst_buff_head *list_)
{
	struct mst_buff *list = ((const struct mst_buff *)list_)->next;
	if (list == (struct mst_buff *)list_)
		list = NULL;
	return list;
}

static inline void __msb_unlink(struct mst_buff *msb,
				struct mst_buff_head *list)
{
	struct mst_buff *next, *prev;

	list->qlen--;

	next = msb->next;
	prev = msb->prev;

	msb->next = msb->prev = NULL;

	next->prev = prev;
	prev->next = next;
}

static inline struct mst_buff *__msb_dequeue(struct mst_buff_head *list)
{
	struct mst_buff *msb = __msb_peek(list);

	if (msb)
		__msb_unlink(msb, list);

	return msb;
}

struct mst_buff *msb_dequeue(struct mst_buff_head *list)
{
	unsigned long flags;
	struct mst_buff *result;

	spin_lock_irqsave(&list->lock, flags);

	result = __msb_dequeue(list);

	spin_unlock_irqrestore(&list->lock, flags);

	return result;
}

void msb_queue_purge(struct mst_buff_head *list)
{
	struct mst_buff *msb;
	while ((msb = msb_dequeue(list)) != NULL)
		msb_free(msb);
}

static void __take_sbd_status(struct mem_link_device *mld, enum direction dir,
			      struct mem_snapshot *mst)
{
	getnstimeofday(&mst->ts);

	mst->dir = dir;

	mst->magic = get_magic(mld);
	mst->access = get_access(mld);

	if (mld->recv_cp2ap_irq)
		mst->int2ap = mld->recv_cp2ap_irq(mld);
	else
		mst->int2ap = 0;

	if (mld->read_ap2cp_irq)
		mst->int2cp = mld->read_ap2cp_irq(mld);
	else
		mst->int2cp = 0;
}

static void __take_mem_status(struct mem_link_device *mld, enum direction dir,
			      struct mem_snapshot *mst)
{
	int i;

	getnstimeofday(&mst->ts);

	mst->dir = dir;

	mst->magic = get_magic(mld);
	mst->access = get_access(mld);

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		mst->head[i][TX] = get_txq_head(dev);
		mst->tail[i][TX] = get_txq_tail(dev);
		mst->head[i][RX] = get_rxq_head(dev);
		mst->tail[i][RX] = get_rxq_tail(dev);
	}

	if (mld->recv_cp2ap_irq)
		mst->int2ap = mld->recv_cp2ap_irq(mld);
	else
		mst->int2ap = 0;

	if (mld->read_ap2cp_irq)
		mst->int2cp = mld->read_ap2cp_irq(mld);
	else
		mst->int2cp = 0;
}

struct mst_buff *mem_take_snapshot(struct mem_link_device *mld,
				   enum direction dir)
{
	struct mst_buff *msb = msb_alloc();

	if (!msb)
		return NULL;

	if (sbd_active(&mld->sbd_link_dev))
		__take_sbd_status(mld, dir, &msb->snapshot);
	else
		__take_mem_status(mld, dir, &msb->snapshot);

	return msb;
}

#endif
