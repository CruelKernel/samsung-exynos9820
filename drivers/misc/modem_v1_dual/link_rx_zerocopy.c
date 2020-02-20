/*
 * Copyright (C) 2018 Samsung Electronics.
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

#include <soc/samsung/exynos-modem-ctrl.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "include/sbd.h"


static u64 recv_offset_from_zerocopy_adaptor(struct zerocopy_adaptor *zdptr)
{
	struct sbd_ring_buffer *rb = zdptr->rb;
	u16 out = zdptr->pre_rp;
	u8 *src = rb->buff[out] + rb->payload_offset;
	u64 offset;

	memcpy(&offset, src, sizeof(offset));

	return offset;
}

static u8 *data_offset_to_buffer(u64 offset, struct sbd_ring_buffer *rb)
{
	struct sbd_link_device *sl = rb->sl;
	struct device *dev = sl->ld->dev;
	u8 *v_zmb = shm_get_zmb_region();
	unsigned int zmb_size = shm_get_zmb_size();
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	dma_addr_t dma_addr;
	int buf_offset;
	u8 *buf = NULL;

	if (offset < (sl->shmem_size + zmb_size)) {
		buf_offset = offset - NET_HEADROOM;
		buf = v_zmb + (buf_offset - sl->shmem_size);
		if (!(buf >= v_zmb && buf < (v_zmb + zmb_size))) {
			mif_err("invalid buf (1st pool) : %p\n", buf);
			return NULL;
		}
	} else {
		mif_err("unexpected offset : %llx\n", offset);
		return NULL;
	}

	if (kfifo_out_spinlocked(&zdptr->fifo, &dma_addr, sizeof(dma_addr),
				&zdptr->lock_kfifo) != sizeof(dma_addr)) {
		mif_err("ERR! kfifo_out fails\n");
		mif_err("kfifo_len:%d\n", kfifo_len(&zdptr->fifo));
		mif_err("kfifo_is_empty:%d\n", kfifo_is_empty(&zdptr->fifo));
		mif_err("kfifo_is_full:%d\n", kfifo_is_full(&zdptr->fifo));
		mif_err("kfifo_avail:%d\n", kfifo_avail(&zdptr->fifo));
		return NULL;
	}
	dma_unmap_single(dev, dma_addr, MIF_BUFF_DEFAULT_CELL_SIZE, DMA_FROM_DEVICE);

	return buf;
}

static u8 *unused_data_offset_to_buffer(u64 offset, struct sbd_ring_buffer *rb)
{
	struct sbd_link_device *sl = rb->sl;
	u8 *v_zmb = shm_get_zmb_region();
	unsigned int zmb_size = shm_get_zmb_size();
	int buf_offset;
	u8 *buf = NULL;

	if (offset < (sl->shmem_size + zmb_size)) {
		buf_offset = offset - NET_HEADROOM;
		buf = v_zmb + (buf_offset - sl->shmem_size);
		if (!(buf >= v_zmb && buf < (v_zmb + zmb_size))) {
			mif_err("invalid buf (1st pool) : %p\n", buf);
			return NULL;
		}
	} else {
		mif_err("unexpected offset : %llx\n", offset);
		return NULL;
	}

	return buf;
}

static inline void free_zerocopy_data(struct sbd_ring_buffer *rb, u16 *out)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int qlen = zdptr->len;
	u64 offset;
	u8 *buff;
	u8 *src = rb->buff[*out] + rb->payload_offset;

	memcpy(&offset, src, sizeof(offset));

	buff = unused_data_offset_to_buffer(offset, rb);

	free_mif_buff(g_mif_buff_mng, buff);

	*out = circ_new_ptr(qlen, *out, 1);
}

static void __reset_zerocopy(struct mem_link_device *mld, struct sbd_ring_buffer *rb)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	u16 out = *zdptr->rp;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	mif_err("__reset_zerocopy [ch : %d]\n", rb->ch);

	/* cancel timer */
	spin_lock_irqsave(&mc->lock, flags);
	if (hrtimer_active(&zdptr->datalloc_timer))
		hrtimer_cancel(&zdptr->datalloc_timer);
	spin_unlock_irqrestore(&mc->lock, flags);

	/* free data */
	spin_lock_irqsave(&zdptr->lock, flags);
	while (*zdptr->wp != out)
		free_zerocopy_data(rb, &out);
	spin_unlock_irqrestore(&zdptr->lock, flags);
}

static void reset_zerocopy(struct link_device *ld)
{
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	struct sbd_ipc_device *ipc_dev =  sl->ipc_dev;
	struct sbd_ring_buffer *rb;
	int i;

	if (sl->reset_zerocopy_done)
		return;

	mif_err("+++\n");

	for (i = 0; i < sl->num_channels; i++) {
		rb = &ipc_dev[i].rb[DL];
		if (rb->zerocopy)
			__reset_zerocopy(mld, rb);
	}

	/* set done flag 1 as reset_zerocopy func works once */
	sl->reset_zerocopy_done = 1;

	mif_err("---\n");
}

int setup_zerocopy_adaptor(struct sbd_ipc_device *ipc_dev)
{
	struct zerocopy_adaptor *zdptr;
	struct sbd_ring_buffer *rb;
	struct link_device *ld;
	struct sbd_link_device *sl;

	if (ipc_dev->zerocopy == false) {
		ipc_dev->zdptr = NULL;
		return 0;
	}

	if (ipc_dev->zdptr == NULL) {
		ipc_dev->zdptr = kzalloc(sizeof(struct zerocopy_adaptor), GFP_ATOMIC);
		if (!ipc_dev->zdptr) {
			mif_err("fail to allocate memory!\n");
			return -ENOMEM;
		}
	}

	/* register reset_zerocopy func */
	rb = &ipc_dev->rb[DL];
	sl = rb->sl;
	ld = sl->ld;
	ld->reset_zerocopy = reset_zerocopy;

	/*
	 * Initialize memory maps for Zero Memory Copy
	 */
	if (ld->mif_buff_mng == NULL) {
		shm_get_zmb_region();
		mif_info("zmb_base=%pK zmb_size:0x%X\n", shm_get_zmb_region(),
							shm_get_zmb_size());

		ld->mif_buff_mng = init_mif_buff_mng(
					(unsigned char *)shm_get_zmb_region(),
					shm_get_zmb_size(),
					MIF_BUFF_DEFAULT_CELL_SIZE);
		g_mif_buff_mng = ld->mif_buff_mng;
		mif_info("g_mif_buff_mng:0x%pK size:0x%08x\n",
					g_mif_buff_mng, shm_get_zmb_size());
	}

	zdptr = ipc_dev->zdptr;

	/* Setup DL direction RB & Zerocopy adaptor */
	rb->zdptr = zdptr;

	spin_lock_init(&zdptr->lock);
	spin_lock_init(&zdptr->lock_kfifo);
	zdptr->rb = rb;
	zdptr->rp = rb->wp; /* swap wp, rp  when zerocopy DL */
	zdptr->wp = rb->rp; /* swap wp, rp  when zerocopy DL */
	zdptr->pre_rp = *zdptr->rp;
	zdptr->len = rb->len;

	if (kfifo_initialized(&zdptr->fifo)) {
		struct sbd_link_device *sl = rb->sl;
		struct device *dev = sl->ld->dev;
		dma_addr_t dma_addr;

		while (kfifo_out_spinlocked(&zdptr->fifo, &dma_addr, sizeof(dma_addr),
						&zdptr->lock_kfifo) == sizeof(dma_addr)) {
			dma_unmap_single(dev, dma_addr, MIF_BUFF_DEFAULT_CELL_SIZE,
									DMA_FROM_DEVICE);
		}

		kfifo_free(&zdptr->fifo);
	}

	if (kfifo_alloc(&zdptr->fifo, zdptr->len * sizeof(dma_addr_t), GFP_KERNEL)) {
		mif_err("kfifo alloc fail\n");
		return -ENOMEM;
	}

	hrtimer_init(&zdptr->datalloc_timer,
			CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	zdptr->datalloc_timer.function = datalloc_timer_func;

	allocate_data_in_advance(zdptr);

	/* set done flag 0 as reset_zerocopy func works */
	sl->reset_zerocopy_done = 0;

	return 0;
}

static inline void set_skb_priv_zerocopy_adaptor(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int out = zdptr->pre_rp;

	/* Record the IO device, the link device, etc. into &skb->cb */
	if (sipc_ps_ch(rb->ch)) {
		unsigned int ch = (rb->size_v[out] >> 16) & 0xffff;

		skbpriv(skb)->iod = link_get_iod_with_channel(rb->ld, ch);
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = ch;
	} else {
		skbpriv(skb)->iod = rb->iod;
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = rb->ch;
	}
}

struct sk_buff *zerocopy_alloc_skb(u8 *buf, unsigned int data_len)
{
	struct sk_buff *skb;

	skb = __build_skb(buf, SKB_DATA_ALIGN(data_len + NET_HEADROOM)
			+ SKB_DATA_ALIGN(sizeof(struct skb_shared_info)));

	if (unlikely(!skb)) {
		mif_err("skb is null\n");
		return NULL;
	}

	skb_reserve(skb, NET_HEADROOM);
	skb_put(skb, data_len);

	return skb;
}

struct sk_buff *zerocopy_alloc_skb_with_memcpy(u8 *buf, unsigned int data_len)
{
	struct sk_buff *skb;
	u8 *src;

	skb = dev_alloc_skb(data_len);
	if (unlikely(!skb))
		return NULL;

	src = buf + NET_HEADROOM;
	skb_put(skb, data_len);
	skb_copy_to_linear_data(skb, src, data_len);

	free_mif_buff(g_mif_buff_mng, buf);

	return skb;
}

struct sk_buff *sbd_pio_rx_zerocopy_adaptor(struct sbd_ring_buffer *rb, int use_memcpy)
{
	struct sk_buff *skb;
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int qlen = zdptr->len;
	unsigned int out = zdptr->pre_rp;
	unsigned int data_len = rb->size_v[out] & 0xFFFF;
	u64 offset;
	u8 *buff;

	offset = recv_offset_from_zerocopy_adaptor(zdptr);
	buff = data_offset_to_buffer(offset, rb);

	if (unlikely(!buff)) {
		mif_err("ERR! buff doesn't exist\n");
		return NULL;
	}

	if (use_memcpy)
		skb = zerocopy_alloc_skb_with_memcpy(buff, data_len);
	else
		skb = zerocopy_alloc_skb(buff, data_len);

	if (unlikely(!skb)) {
		mif_err("ERR! Socket buffer doesn't exist\n");
		return NULL;
	}

	set_lnk_hdr(rb, skb);

	set_skb_priv_zerocopy_adaptor(rb, skb);

	check_more(rb, skb);

	zdptr->pre_rp = circ_new_ptr(qlen, out, 1);

	return skb;
}

static u64 buffer_to_data_offset(u8 *buf, struct sbd_ring_buffer *rb)
{
	struct sbd_link_device *sl = rb->sl;
	struct device *dev = sl->ld->dev;
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	dma_addr_t dma_addr;
	u8 *v_zmb = shm_get_zmb_region();
	unsigned int zmb_size = shm_get_zmb_size();
	u8 *data;
	u64 offset;

	data = buf + NET_HEADROOM;

	if (buf >= v_zmb && buf < (v_zmb + zmb_size)) {
		offset = data - v_zmb + sl->shmem_size;
	} else {
		mif_err("unexpected buff address : %lx\n", (unsigned long int)virt_to_phys(buf));
		return -EINVAL;
	}

	dma_addr = dma_map_single(dev, buf, MIF_BUFF_DEFAULT_CELL_SIZE, DMA_FROM_DEVICE);
	kfifo_in_spinlocked(&zdptr->fifo, &dma_addr, sizeof(dma_addr), &zdptr->lock_kfifo);

	return offset;
}

int allocate_data_in_advance(struct zerocopy_adaptor *zdptr)
{
	struct sbd_ring_buffer *rb = zdptr->rb;
	struct modem_ctl *mc = rb->sl->ld->mc;
	struct mif_buff_mng *mif_buff_mng = rb->ld->mif_buff_mng;
	unsigned int qlen = rb->len;
	unsigned long flags;
	u8 *buffer;
	u64 offset;
	u8 *dst;
	int alloc_cnt = 0;

	spin_lock_irqsave(&zdptr->lock, flags);
	if (cp_offline(mc)) {
		spin_unlock_irqrestore(&zdptr->lock, flags);
		return 0;
	}

	while (zerocopy_adaptor_space(zdptr) > 0) {
		buffer = alloc_mif_buff(mif_buff_mng);
		if (!buffer) {
			spin_unlock_irqrestore(&zdptr->lock, flags);
			return -ENOMEM;
		}

		offset = buffer_to_data_offset(buffer, rb);

		dst = rb->buff[*zdptr->wp] + rb->payload_offset;

		memcpy(dst, &offset, sizeof(offset));

		barrier();

		*zdptr->wp = circ_new_ptr(qlen, *zdptr->wp, 1);

		alloc_cnt++;

	}
	barrier();
	spin_unlock_irqrestore(&zdptr->lock, flags);

	/* Commit the item before incrementing the head */
	smp_mb();
	return alloc_cnt;
}
