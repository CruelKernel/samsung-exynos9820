/**
@file		link_device_memory_main.c
@brief		common functions for all types of memory interface media
@date		2014/02/05
@author		Hankook Jang (hankook.jang@samsung.com)
*/

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

#include <soc/samsung/exynos-modem-ctrl.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "include/sbd.h"
#include <linux/shm_ipc.h>

#ifdef GROUP_MEM_LINK_SBD
/**
@weakgroup group_mem_link_sbd
@{
*/

#ifdef GROUP_MEM_LINK_SETUP
/**
@weakgroup group_mem_link_setup
@{
*/

static void print_sbd_config(struct sbd_link_device *sl)
{
#ifdef DEBUG_MODEM_IF
	int i;

	pr_err("mif: SBD_IPC {shmem_base:0x%lX shmem_size:%d}\n",
		(unsigned long)sl->shmem, sl->shmem_size);

	pr_err("mif: SBD_IPC {version:%d num_channels:%d rbps_offset:%d}\n",
		sl->g_desc->version, sl->g_desc->num_channels,
		sl->g_desc->rbps_offset);

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_rb_channel *rb_ch = &sl->g_desc->rb_ch[i];
		struct sbd_rb_desc *rbd;

		rbd = &sl->g_desc->rb_desc[i][UL];
		pr_err("mif: RB_DESC[%-2d][UL](offset:%d) = "
			"{id:%-2d ch:%-3d dir:%s} "
			"{sbdv_offset:%-5d rb_len:%-3d} "
			"{buff_size:%-4d payload_offset:%d}\n",
			i, rb_ch->ul_rbd_offset, rbd->id, rbd->ch,
			udl_str(rbd->direction), rb_ch->ul_sbdv_offset,
			rbd->length, rbd->buff_size, rbd->payload_offset);

		rbd = &sl->g_desc->rb_desc[i][DL];
		pr_err("mif: RB_DESC[%-2d][DL](offset:%d) = "
			"{id:%-2d ch:%-3d dir:%s} "
			"{sbdv_offset:%-5d rb_len:%-3d} "
			"{buff_size:%d payload_offset:%d}\n",
			i, rb_ch->dl_rbd_offset, rbd->id, rbd->ch,
			udl_str(rbd->direction), rb_ch->dl_sbdv_offset,
			rbd->length, rbd->buff_size, rbd->payload_offset);
	}
#endif
}

static void init_desc_alloc(struct sbd_link_device *sl, unsigned int offset)
{
	sl->desc_alloc_offset = offset;
}

static void *desc_alloc(struct sbd_link_device *sl, size_t size)
{
	u8 *desc = (sl->shmem + sl->desc_alloc_offset);
	sl->desc_alloc_offset += size;
	return desc;
}

static void init_buff_alloc(struct sbd_link_device *sl, unsigned int offset)
{
	sl->buff_alloc_offset = offset;
}

static u8 *buff_alloc(struct sbd_link_device *sl, unsigned int size)
{
	u8 *buff = (sl->shmem + sl->buff_alloc_offset);
	sl->buff_alloc_offset += size;
	return buff;
}

/**
@brief		set up an SBD RB descriptor in SHMEM
*/
static void setup_sbd_rb_desc(struct sbd_rb_desc *rb_desc,
			      struct sbd_ring_buffer *rb)
{
	rb_desc->ch = rb->ch;

	rb_desc->direction = rb->dir;
	rb_desc->signaling = 1;

	rb_desc->sig_mask = MASK_INT_VALID | MASK_SEND_DATA;

	rb_desc->length = rb->len;
	rb_desc->id = rb->id;

	rb_desc->buff_size = rb->buff_size;
	rb_desc->payload_offset = rb->payload_offset;
}

/**
@brief		set up an SBD RB

(1) build an SBD RB instance in the kernel space\n
(2) allocate an SBD array in SHMEM\n
(3) allocate a data buffer array in SHMEM if possible\n
*/
static int setup_sbd_rb(struct sbd_link_device *sl, struct sbd_ring_buffer *rb,
			enum direction dir, struct sbd_link_attr *link_attr)
{
	size_t alloc_size;
	unsigned int i;

	rb->sl = sl;

	rb->lnk_hdr = link_attr->lnk_hdr;
	rb->zerocopy = link_attr->zerocopy;

	rb->more = false;
	rb->total = 0;
	rb->rcvd = 0;

	/*
	Initialize an SBD RB instance in the kernel space.
	*/
	rb->id = link_attr->id;
	rb->ch = link_attr->ch ?: SIPC_CH_ID_PDP_0;
	rb->dir = dir;
	rb->len = link_attr->rb_len[dir];
	rb->buff_size = link_attr->buff_size[dir];
	rb->payload_offset = 0;

	/*
	Prepare array of pointers to the data buffer for each SBD
	*/
	alloc_size = (rb->len * sizeof(u8 *));
	if (!rb->buff)
		rb->buff = kmalloc(alloc_size, GFP_ATOMIC);
	if (!rb->buff)
		return -ENOMEM;

	/*
	(1) Allocate an array of data buffers in SHMEM.
	(2) Register the address of each data buffer.
	*/
	alloc_size = ((u32)rb->len * (u32)rb->buff_size);
	rb->buff_rgn = (u8 *)buff_alloc(sl, alloc_size);
	if (!rb->buff_rgn)
		return -ENOMEM;

	for (i = 0; i < rb->len; i++)
		rb->buff[i] = rb->buff_rgn + (i * rb->buff_size);

	mif_err("RB[%d:%d][%s] buff_rgn {addr:0x%pK offset:%d size:%lu}\n",
		rb->id, rb->ch, udl_str(dir), rb->buff_rgn,
		calc_offset(rb->buff_rgn, sl->shmem), alloc_size);

	/*
	Prepare SBD array in SHMEM.
	*/
	rb->rp = &sl->rp[rb->dir][rb->id];
	rb->wp = &sl->wp[rb->dir][rb->id];

	alloc_size = (rb->len * sizeof(u32));

	rb->addr_v = (u32 *)desc_alloc(sl, alloc_size);
	if (!rb->addr_v)
		return -ENOMEM;

	rb->size_v = (u32 *)desc_alloc(sl, alloc_size);
	if (!rb->size_v)
		return -ENOMEM;

	/*
	Register each data buffer to the corresponding SBD.
	*/
	for (i = 0; i < rb->len; i++) {
		rb->addr_v[i] = calc_offset(rb->buff[i], sl->shmem);
		rb->size_v[i] = 0;
	}

	rb->iod = link_get_iod_with_channel(sl->ld, rb->ch);
	rb->ld = sl->ld;

	return 0;
}

static void setup_desc_rgn(struct sbd_link_device *sl)
{
	size_t size;

#if 1
	mif_err("SHMEM {base:0x%pK size:%d}\n",
		sl->shmem, sl->shmem_size);
#endif

	/*
	Allocate @g_desc.
	*/
	size = sizeof(struct sbd_global_desc);
	sl->g_desc = (struct sbd_global_desc *)desc_alloc(sl, size);

#if 1
	mif_err("G_DESC_OFFSET = %d(0x%pK)\n",
		calc_offset(sl->g_desc, sl->shmem),
		sl->g_desc);

	mif_err("RB_CH_OFFSET = %d (0x%pK)\n",
		calc_offset(sl->g_desc->rb_ch, sl->shmem),
		sl->g_desc->rb_ch);

	mif_err("RBD_PAIR_OFFSET = %d (0x%pK)\n",
		calc_offset(sl->g_desc->rb_desc, sl->shmem),
		sl->g_desc->rb_desc);
#endif

	size = sizeof(u16) * ULDL * RDWR * sl->num_channels;
	sl->rbps = (u16 *)desc_alloc(sl, size);
#if 1
	mif_err("RBP_SET_OFFSET = %d (0x%pK)\n",
		calc_offset(sl->rbps, sl->shmem), sl->rbps);
#endif

	/*
	Set up @g_desc.
	*/
	sl->g_desc->version = sl->version;
	sl->g_desc->num_channels = sl->num_channels;
	sl->g_desc->rbps_offset = calc_offset(sl->rbps, sl->shmem);

	/*
	Set up pointers to each RBP array.
	*/
	sl->rp[UL] = sl->rbps + sl->num_channels * 0;
	sl->wp[UL] = sl->rbps + sl->num_channels * 1;
	sl->rp[DL] = sl->rbps + sl->num_channels * 2;
	sl->wp[DL] = sl->rbps + sl->num_channels * 3;

#if 1
	mif_err("Complete!!\n");
#endif
}

static void setup_link_attr(struct sbd_link_attr *link_attr, u16 id, u16 ch,
			    struct modem_io_t *io_dev)
{
	link_attr->id = id;
	link_attr->ch = ch;

	if (io_dev->attrs & IODEV_ATTR(ATTR_NO_LINK_HEADER))
		link_attr->lnk_hdr = false;
	else
		link_attr->lnk_hdr = true;

	link_attr->rb_len[UL] = io_dev->ul_num_buffers;
	link_attr->buff_size[UL] = io_dev->ul_buffer_size;
	link_attr->rb_len[DL] = io_dev->dl_num_buffers;
	link_attr->buff_size[DL] = io_dev->dl_buffer_size;

	if (io_dev->attrs & IODEV_ATTR(ATTR_ZEROCOPY))
		link_attr->zerocopy = true;
	else
		link_attr->zerocopy = false;
}

static u64 recv_offset_from_zerocopy_adaptor(struct zerocopy_adaptor *zdptr)
{
	struct sbd_ring_buffer *rb = zdptr->rb;
	u16 out = zdptr->pre_rp;
	u8* src = rb->buff[out] + rb->payload_offset;
	u64 offset;

	memcpy(&offset, src, sizeof(offset));

	return offset;
}

/**
@brief		convert data offset to buffer

@param rb	the pointer to sbd_ring_buffer

@param offset	the offset of data from shared memory base
		buffer + NET_HEADROOM is address of data
@return buf	the pointer to buffer managed by buffer manager
*/
static u8* data_offset_to_buffer(u64 offset, struct sbd_ring_buffer *rb)
{
	struct sbd_link_device *sl = rb->sl;
	struct device *dev = sl->ld->dev;
	u8* v_zmb = shm_get_zmb_region();
	unsigned int zmb_size = shm_get_zmb_size();
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	dma_addr_t dma_addr;
	int buf_offset;
	u8* buf = NULL;

	if (offset < (sl->shmem_size + zmb_size)) {
		buf_offset = offset - NET_HEADROOM;
		buf = v_zmb + (buf_offset - sl->shmem_size);
		if (!(buf >= v_zmb && buf < (v_zmb + zmb_size))) {
			mif_err("invalid buf (1st pool) : %pK\n", buf);
			return NULL;
		}
	} else {
		mif_err("unexpected offset : %lx\n", (long unsigned int)offset);
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

static u8* unused_data_offset_to_buffer(u64 offset, struct sbd_ring_buffer *rb)
{
	struct sbd_link_device *sl = rb->sl;
	u8* v_zmb = shm_get_zmb_region();
	unsigned int zmb_size = shm_get_zmb_size();
	int buf_offset;
	u8* buf = NULL;

	if (offset < (sl->shmem_size + zmb_size)) {
		buf_offset = offset - NET_HEADROOM;
		buf = v_zmb + (buf_offset - sl->shmem_size);
		if (!(buf >= v_zmb && buf < (v_zmb + zmb_size))) {
			mif_err("invalid buf (1st pool) : %pK\n", buf);
			return NULL;
		}
	} else {
		mif_err("unexpected offset : %lx\n", (long unsigned int)offset);
		return NULL;
	}

	return buf;
}

static inline void free_zerocopy_data(struct sbd_ring_buffer *rb, u16 *out)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int qlen = zdptr->len;
	u64 offset;
	u8* buff;
	u8* src = rb->buff[*out] + rb->payload_offset;

	memcpy(&offset, src, sizeof(offset));

	buff = unused_data_offset_to_buffer(offset, rb);

	free_mif_buff(g_mif_buff_mng, buff);

	*out = circ_new_ptr(qlen, *out, 1);
}

static inline void cancel_datalloc_timer(struct mem_link_device *mld,
				   struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (hrtimer_active(timer))
		hrtimer_cancel(timer);

	spin_unlock_irqrestore(&mc->lock, flags);
}

void __reset_zerocopy(struct mem_link_device *mld, struct sbd_ring_buffer *rb)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	u16 out = *zdptr->rp;
	unsigned long flags;

	mif_err("__reset_zerocopy [ch : %d]\n", rb->ch);

	cancel_datalloc_timer(mld, &zdptr->datalloc_timer);

	spin_lock_irqsave(&zdptr->lock, flags);
	while (*zdptr->wp != out) {
		free_zerocopy_data(rb, &out);
	}
	spin_unlock_irqrestore(&zdptr->lock, flags);
}

void reset_zerocopy(struct link_device *ld)
{
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	struct sbd_ipc_device *ipc_dev =  sl->ipc_dev;
	struct sbd_ring_buffer *rb;
	int i;

	if (sl->reset_zerocopy_done) {
		return;
	}

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

static int setup_zerocopy_adaptor(struct sbd_ipc_device *ipc_dev)
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
	if ((shm_get_zmb_size() != 0) && (ld->mif_buff_mng == NULL)) {
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

	if (kfifo_alloc(&zdptr->fifo, zdptr->len * sizeof(dma_addr_t), GFP_ATOMIC)) {
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

static int init_sbd_ipc(struct sbd_link_device *sl,
			struct sbd_ipc_device ipc_dev[],
			struct sbd_link_attr link_attr[])
{
	int i;

	setup_desc_rgn(sl);

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_rb_channel *rb_ch = &sl->g_desc->rb_ch[i];
		struct sbd_rb_desc *rb_desc;
		struct sbd_ring_buffer *rb;
		int ret;

		ipc_dev[i].id = link_attr[i].id;
		ipc_dev[i].ch = link_attr[i].ch;
		ipc_dev[i].zerocopy = link_attr[i].zerocopy;

		/*
		Setup UL Ring Buffer in the ipc_dev[$i]
		*/
		rb = &ipc_dev[i].rb[UL];
		ret = setup_sbd_rb(sl, rb, UL, &link_attr[i]);
		if (ret < 0)
			return ret;

		/*
		Setup UL RB_DESC & UL RB_CH in the g_desc
		*/
		rb_desc = &sl->g_desc->rb_desc[i][UL];
		setup_sbd_rb_desc(rb_desc, rb);
		rb_ch->ul_rbd_offset = calc_offset(rb_desc, sl->shmem);
		rb_ch->ul_sbdv_offset = calc_offset(rb->addr_v, sl->shmem);

		/*
		Setup DL Ring Buffer in the ipc_dev[$i]
		*/
		rb = &ipc_dev[i].rb[DL];
		ret = setup_sbd_rb(sl, rb, DL, &link_attr[i]);
		if (ret < 0)
			return ret;

		/*
		Setup DL RB_DESC & DL RB_CH in the g_desc
		*/
		rb_desc = &sl->g_desc->rb_desc[i][DL];
		setup_sbd_rb_desc(rb_desc, rb);
		rb_ch->dl_rbd_offset = calc_offset(rb_desc, sl->shmem);
		rb_ch->dl_sbdv_offset = calc_offset(rb->addr_v, sl->shmem);

#ifdef CONFIG_CP_ZEROCOPY
		/*
		Setup zerocopy_adaptor if zerocopy ipc_dev
		*/
		ret = setup_zerocopy_adaptor(&ipc_dev[i]);
		if (ret < 0)
			return ret;
#endif
	}

	return 0;
}

static void init_ipc_device(struct sbd_link_device *sl, u16 id,
			    struct sbd_ipc_device *ipc_dev)
{
	u16 ch = sbd_id2ch(sl, id);
	struct sbd_ring_buffer *rb;

	ipc_dev->id = id;
	ipc_dev->ch = ch;

	atomic_set(&ipc_dev->config_done, 0);

	rb = &ipc_dev->rb[UL];
	spin_lock_init(&rb->lock);
	skb_queue_head_init(&rb->skb_q);
	atomic_set(&rb->busy, 0);

	rb = &ipc_dev->rb[DL];
	spin_lock_init(&rb->lock);
	skb_queue_head_init(&rb->skb_q);
	atomic_set(&rb->busy, 0);
}

/**
@return		the number of actual link channels
*/
static unsigned int init_ctrl_tables(struct sbd_link_device *sl, int num_iodevs,
				     struct modem_io_t iodevs[])
{
	int i;
	unsigned int id;
	unsigned int qos_prio = QOS_HIPRIO;

	/*
	Fill ch2id array with MAX_LINK_CHANNELS value to prevent sbd_ch2id()
	from returning 0 for unused channels.
	*/
	for (i = 0; i < MAX_SIPC_CHANNELS; i++)
		sl->ch2id[i] = MAX_LINK_CHANNELS;

	for (id = 0, i = 0; i < num_iodevs; i++) {
		int ch = iodevs[i].id;

		if ((sipc5_ipc_ch(ch) && !sipc_ps_ch(ch)) ||
			iodevs[i].format == IPC_MULTI_RAW) {
			/* Skip making rb if mismatch region info */
			if (iodevs[i].attrs & IODEV_ATTR(ATTR_OPTION_REGION) &&
				strcmp(iodevs[i].option_region, CONFIG_OPTION_REGION))
				continue;

			/* Change channel to Qos priority */
			if (iodevs[i].format == IPC_MULTI_RAW)
				ch = qos_prio++;

			/* Save CH# to LinkID-to-CH conversion table. */
			sl->id2ch[id] = ch;

			/* Save LinkID to CH-to-LinkID conversion table. */
			sl->ch2id[ch] = id;

			/* Set up the attribute table entry of a LinkID. */
			setup_link_attr(&sl->link_attr[id], id, ch, &iodevs[i]);

			++id;
		}
	}

#ifndef CONFIG_MODEM_IF_QOS
	for (i = 0; i < num_iodevs; i++) {
		int ch = iodevs[i].id;
		if (sipc_ps_ch(ch))
			sl->ch2id[ch] = sl->ch2id[QOS_HIPRIO];
	}
#endif

	/* Finally, id has the number of actual link channels. */
	return id;
}

int init_sbd_link(struct sbd_link_device *sl)
{
	int err;

	if (!sl)
		return -ENOMEM;

	memset(sl->shmem + DESC_RGN_OFFSET, 0, DESC_RGN_SIZE);

	init_desc_alloc(sl, DESC_RGN_OFFSET);
	init_buff_alloc(sl, BUFF_RGN_OFFSET);

	err = init_sbd_ipc(sl, sl->ipc_dev, sl->link_attr);
	if (!err)
		print_sbd_config(sl);

	return err;
}

int create_sbd_link_device(struct link_device *ld, struct sbd_link_device *sl,
			   u8 *shmem_base, unsigned int shmem_size)
{
	int i;
	int num_iodevs;
	struct modem_io_t *iodevs;

	if (!ld || !sl || !shmem_base)
		return -EINVAL;

	if (!ld->mdm_data)
		return -EINVAL;

	num_iodevs = ld->mdm_data->num_iodevs;
	iodevs = ld->mdm_data->iodevs;

	sl->ld = ld;

	sl->version = 1;

	sl->shmem = shmem_base;
#ifdef CONFIG_CP_LINEAR_WA
	/*
	 * CP linear mode for Makalu (Exynos9820)
	 * ZMB region for CP:0x5000_0000 AP:0xD000_0000
	 * sl->shmem_size is used to calculate offset of ZMB
	 * 0x5000_0000 = End of IPC region + 0x0490_0000
	 */
	sl->shmem_size = shmem_size + 0x4900000;
#else
	sl->shmem_size = shmem_size;
#endif

	sl->num_channels = init_ctrl_tables(sl, num_iodevs, iodevs);

	for (i = 0; i < sl->num_channels; i++)
		init_ipc_device(sl, i, sbd_id2dev(sl, i));

	sl->reset_zerocopy_done = 1;

	return 0;
}

/**
@}
*/
#endif

#ifdef GROUP_MEM_IPC_TX
/**
@weakgroup group_mem_ipc_tx
@{
*/

/**
@brief		check the free space in a SBD RB

@param rb	the pointer to an SBD RB instance

@retval "> 0"	the size of free space in the @b @@dev TXQ
@retval "< 0"	an error code
*/
static inline int check_rb_space(struct sbd_ring_buffer *rb, unsigned int qlen,
				 unsigned int in, unsigned int out)
{
	unsigned int space;

	if (!circ_valid(qlen, in, out)) {
		mif_err("ERR! TXQ[%d:%d] DIRTY (qlen:%d in:%d out:%d)\n",
			rb->id, rb->ch, qlen, in, out);
		return -EIO;
	}

	space = circ_get_space(qlen, in, out);
	if (unlikely(space < 1)) {
		mif_err_limited("TXQ[%d:%d] NOSPC (qlen:%d in:%d out:%d)\n",
				rb->id, rb->ch, qlen, in, out);
		return -ENOSPC;
	}

	return space;
}

int sbd_pio_tx(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	int ret;
	unsigned int qlen = rb->len;
	unsigned int in = *rb->wp;
	unsigned int out = *rb->rp;
	unsigned int count = skb->len;
	unsigned int space = (rb->buff_size - rb->payload_offset);
	u8 *dst;

	ret = check_rb_space(rb, qlen, in, out);
	if (unlikely(ret < 0))
		return ret;

	if (unlikely(count > space)) {
		mif_err("ERR! {id:%d ch:%d} count %d > space %d\n",
			rb->id, rb->ch, count, space);
		return -ENOSPC;
	}

	barrier();

	dst = rb->buff[in] + rb->payload_offset;

	barrier();

	skb_copy_from_linear_data(skb, dst, count);

	if (sipc_ps_ch(rb->ch)) {
		struct io_device *iod = skbpriv(skb)->iod;
		unsigned int ch = iod->id;

		rb->size_v[in] = (skb->len & 0xFFFF);
		rb->size_v[in] |= (ch << 16);
	} else {
		rb->size_v[in] = skb->len;
	}

	barrier();

	*rb->wp = circ_new_ptr(qlen, in, 1);

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

/**
@}
*/
#endif

#ifdef GROUP_MEM_IPC_RX
/**
@weakgroup group_mem_ipc_rx
@{
*/

static inline struct sk_buff *recv_data(struct sbd_ring_buffer *rb, u16 out)
{
	struct sk_buff *skb;
	u8 *src;
	unsigned int len = rb->size_v[out] & 0xFFFF;
	unsigned int space = (rb->buff_size - rb->payload_offset);

	if (unlikely(len > space)) {
		mif_err("ERR! {id:%d ch:%d} size %d > space %d\n",
			rb->id, rb->ch, len, space);
		return NULL;
	}

	skb = dev_alloc_skb(len);
	if (unlikely(!skb)) {
		mif_err("ERR! {id:%d ch:%d} alloc_skb(%d) fail\n",
			rb->id, rb->ch, len);
		return NULL;
	}

	src = rb->buff[out] + rb->payload_offset;
	skb_put(skb, len);
	skb_copy_to_linear_data(skb, src, len);

	return skb;
}

static inline void set_lnk_hdr(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	skbpriv(skb)->lnk_hdr = rb->lnk_hdr && !rb->more;
}

static inline void set_skb_priv_zerocopy_adaptor(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int out = zdptr->pre_rp;

	/* Record the IO device, the link device, etc. into &skb->cb */
	if (sipc_ps_ch(rb->ch)) {
		unsigned ch = (rb->size_v[out] >> 16) & 0xffff;
		skbpriv(skb)->iod = link_get_iod_with_channel(rb->ld, ch);
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = ch;
	} else {
		skbpriv(skb)->iod = rb->iod;
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = rb->ch;
	}
}

static inline void set_skb_priv(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	unsigned int out = *rb->rp;

	/* Record the IO device, the link device, etc. into &skb->cb */
	if (sipc_ps_ch(rb->ch)) {
		unsigned ch = (rb->size_v[out] >> 16) & 0xffff;
		skbpriv(skb)->iod = link_get_iod_with_channel(rb->ld, ch);
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = ch;
	} else {
		skbpriv(skb)->iod = rb->iod;
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = rb->ch;
	}
}

static inline void check_more(struct sbd_ring_buffer *rb, struct sk_buff *skb)
{
	if (rb->lnk_hdr) {
		if (!rb->more) {
			if (sipc5_get_frame_len(skb->data) > rb->buff_size) {
				rb->more = true;
				rb->total = sipc5_get_frame_len(skb->data);
				rb->rcvd = skb->len;
			}
		} else {
			rb->rcvd += skb->len;
			if (rb->rcvd >= rb->total) {
				rb->more = false;
				rb->total = 0;
				rb->rcvd = 0;
			}
		}
	}
}

struct sk_buff *sbd_pio_rx(struct sbd_ring_buffer *rb)
{
	struct sk_buff *skb;
	unsigned int qlen = rb->len;
	unsigned int out = *rb->rp;

	skb = recv_data(rb, out);
	if (unlikely(!skb))
		return NULL;

	set_lnk_hdr(rb, skb);

	set_skb_priv(rb, skb);

	check_more(rb, skb);

	*rb->rp = circ_new_ptr(qlen, out, 1);

	return skb;
}

struct sk_buff *zerocopy_alloc_skb(u8* buf, unsigned int data_len)
{
	struct sk_buff *skb;

	skb = __build_skb(buf, SKB_DATA_ALIGN(data_len + NET_HEADROOM)
			+ SKB_DATA_ALIGN(sizeof(struct skb_shared_info)));

	if (unlikely(!skb))
		return NULL;

	skb_reserve(skb, NET_HEADROOM);
	skb_put(skb, data_len);

	return skb;
}

struct sk_buff *zerocopy_alloc_skb_with_memcpy(u8* buf, unsigned int data_len)
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
	u8* buff;

	offset = recv_offset_from_zerocopy_adaptor(zdptr);
	buff = data_offset_to_buffer(offset, rb);

	if (!buff)
		return NULL;

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

/**
@brief		convert buffer to data offset

@param buf	the pointer to buffer managed by buffer manager
@param rb	the pointer to sbd_ring_buffer

@return offset	the offset of data from shared memory base
		buffer + NET_HEADROOM is address of data
*/
static u64 buffer_to_data_offset(u8* buf, struct sbd_ring_buffer *rb)
{
	struct sbd_link_device *sl = rb->sl;
	struct device *dev = sl->ld->dev;
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	dma_addr_t dma_addr;
	u8* v_zmb = shm_get_zmb_region();
	unsigned int zmb_size = shm_get_zmb_size();
	u8* data;
	u64 offset;

	data = buf + NET_HEADROOM;

	if (buf >= v_zmb && buf < (v_zmb + zmb_size) ) {
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

/**
@}
*/
#endif

/**
// End of group_mem_link_sbd
@}
*/
#endif
