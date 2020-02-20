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

#include <linux/delay.h>
#include <linux/shm_ipc.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_dump.h"
#include "link_device_memory.h"

struct dit_dev_info dit_dev;
struct packet_descriptor *dit_pd;
struct sk_buff *dit_skb[MAX_DIT_SKB_NUM];

#if defined(CONFIG_CP_PKTPROC)
int pio_rx_dit_on_pktproc(struct link_device *ld, u32 num_frames)
{
	unsigned int qlen = MAX_PKTPROC_DESC_NUM;
	unsigned int out = pktproc_gdesc->rear_ptr;
	unsigned int data_len;

	struct mem_link_device *mld = ld_to_mem_link_device(ld);

	u8 *src;

	int i, num_alloc_free = num_frames;
	int ret;
	int counter = 0;
	u32 reg_val;

	if (!num_frames) {
		dit_debug("num_frames is 0\n");
		return 0;
	}
	if (num_frames > MAX_DIT_SKB_NUM) {
		dit_debug("num_frames %d is over %d\n", num_frames, MAX_DIT_SKB_NUM);
		num_frames = MAX_DIT_SKB_NUM;
	}
	if (readl(dit_dev.reg_base + DIT_INT_PENDING) & 0x2) {
		dit_debug("DIT RX pending. return\n");
		return -EACCES;
	}
	if (readl(dit_dev.reg_base + DIT_STATUS) != 0x0) {
		dit_debug("DIT status:0x%08x. return\n", readl(dit_dev.reg_base + DIT_STATUS));
		return -EACCES;
	}

	dit_debug("num_frames:%d\n", num_frames);
	for (i = 0; i < num_frames; i++) {
		/* get buffer */
		data_len = pktproc_desc[out].length;
		src = shm_get_pktproc_data_base_addr() +
			(pktproc_desc[out].data_addr & ~PKTPROC_CP_BASE_ADDR_MASK) - PKTPROC_DATA_BASE_OFFSET;
		if ((src < shm_get_pktproc_data_base_addr()) ||
			(src > shm_get_pktproc_data_base_addr() + shm_get_pktproc_data_size())) {
			mif_err_limited("src range error:%pK base:%llx size:0x%x\n",
				src, virt_to_phys(shm_get_pktproc_data_base_addr()), shm_get_pktproc_data_size());
			ret = -EINVAL;
			num_alloc_free = i;
			goto rx_error;
		}
		__inval_dcache_area(src, data_len);
		dit_debug("addr:0x%08X len:%d ch:%d\n",
				pktproc_desc[out].data_addr, data_len, pktproc_desc[out].channel_id);
#ifdef DIT_DEBUG_PKT
		pr_buffer("in", (char *)src, (size_t)data_len, (size_t)40);
#endif

#ifdef CONFIG_LINK_DEVICE_NAPI
		dit_skb[i] = napi_alloc_skb(&mld->mld_napi, data_len);
#else
		dit_skb[i] = dev_alloc_skb(data_len);
#endif
		if (unlikely(!dit_skb[i])) {
			mif_err_limited("alloc_skb() fail\n");
			ret = -ENOMEM;
			num_alloc_free = i;
			goto rx_error;
		}

		/* packet descriptor */
		dit_pd[i].src_addr = virt_to_phys(src);
		dit_pd[i].dst_addr = virt_to_phys(skb_put(dit_skb[i], data_len));
		dit_pd[i].size = data_len;
		dit_pd[i].control = 0;
		dit_pd[i].control |= (0x1 << 5); /* csum */
		if (i == 0)
			dit_pd[i].control |= (0x1 << 7); /* head */
		if (i == num_frames - 1) {
			dit_pd[i].control |= (0x1 << 6); /* tail */
			dit_pd[i].control |= (0x1 << 4); /* int */
		}

		dit_debug("pd%d src:0x%llx dst:0x%llx size:%d control:0x%x\n",
				i, dit_pd[i].src_addr, dit_pd[i].dst_addr,
				dit_pd[i].size, dit_pd[i].control);

		out = circ_new_ptr(qlen, out, 1);
	}

	/* start dit transfer */
	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}
	writel(dit_dev.dma & 0xffffffff, dit_dev.reg_base + DIT_NAT_RX_DESC_ADDR_0);
	writel((dit_dev.dma >> 32) & 0xf, dit_dev.reg_base + DIT_NAT_RX_DESC_ADDR_1);
	writel(0x4, dit_dev.reg_base + DIT_SW_COMMAND);

	/* wait */
	counter = 0;
	while (1) {
		ndelay(100);
		reg_val = readl(dit_dev.reg_base + DIT_INT_PENDING);
		if ((reg_val & (0x1 << 1)) != 0) {
			reg_val |= (0x1 << 1);
			writel(reg_val, dit_dev.reg_base + DIT_INT_PENDING);
			dit_debug("Rx Done(%d)\n", counter);
			break;
		}
		if (counter++ >= 20000) {
			mif_err_limited("Can't get DIT RX\n");
			ret = -EPERM;
			num_alloc_free = num_frames;
			goto rx_error;
		}
	}

	/* received */
	dit_dev.pkt_on_pktproc_cnt++;
	counter = 0;
	for (i = 0; i < num_frames; i++) {
		if ((dit_skb[i]->data[0] == 0xff) || (dit_skb[i]->data[0] == 0)) {
			mif_err_limited("dit recv error:0x%x %d rear:%d\n",
						dit_skb[i]->data[0], i, pktproc_gdesc->rear_ptr);
			pktproc_gdesc->rear_ptr = circ_new_ptr(pktproc_gdesc->num_of_desc, pktproc_gdesc->rear_ptr, 1);
			continue;
		}
#ifdef DIT_DEBUG_PKT
		pr_buffer("dit", (char *)(dit_skb[i]->data), (size_t)(dit_skb[i]->len), (size_t)40);
#endif
		skbpriv(dit_skb[i])->lnk_hdr = 0;
		skbpriv(dit_skb[i])->sipc_ch = pktproc_desc[pktproc_gdesc->rear_ptr].channel_id;
		skbpriv(dit_skb[i])->iod = link_get_iod_with_channel(ld, skbpriv(dit_skb[i])->sipc_ch);
		skbpriv(dit_skb[i])->ld = ld;
		dit_skb[i]->ip_summed = CHECKSUM_UNNECESSARY;
		skb_trim(dit_skb[i], dit_pd[i].size);

		counter++;
		pktproc_gdesc->rear_ptr = circ_new_ptr(pktproc_gdesc->num_of_desc, pktproc_gdesc->rear_ptr, 1);

		pass_skb_to_net(mld, dit_skb[i]);
	}

	return counter;

rx_error:
	for (i = 0; i < num_alloc_free; i++) {
		if (dit_skb[i])
			dev_kfree_skb_any(dit_skb[i]);
	}

	return ret;
}
#endif /* CONFIG_CP_PKTPROC */

#if defined(CONFIG_CP_ZEROCOPY)
static inline u64 recv_offset_from_zerocopy_adaptor_for_dit(struct zerocopy_adaptor *zdptr, u16 out)
{
	struct sbd_ring_buffer *rb = zdptr->rb;
	u8 *src = rb->buff[out] + rb->payload_offset;
	u64 offset;

	memcpy(&offset, src, sizeof(offset));

	return offset;
}

static inline u8 *data_offset_to_buffer_for_dit(u64 offset, struct sbd_ring_buffer *rb)
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
			mif_err("invalid buf (1st pool) : %pK\n", buf);
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

static inline int set_skb_priv_zerocopy_adaptor_for_dit(struct sbd_ring_buffer *rb, struct sk_buff *skb, u16 out)
{
	/* Record the IO device, the link device, etc. into &skb->cb */
	if (sipc_ps_ch(rb->ch)) {
		unsigned int ch = (rb->size_v[out] >> 16) & 0xffff;

		skbpriv(skb)->iod = link_get_iod_with_channel(rb->ld, ch);
		if (!skbpriv(skb)->iod)
			return -1;
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = ch;
	} else {
		skbpriv(skb)->iod = rb->iod;
		skbpriv(skb)->ld = rb->ld;
		skbpriv(skb)->sipc_ch = rb->ch;
	}

	return 0;
}

int sbd_pio_rx_dit_on_zerocopy(struct sbd_ring_buffer *rb, u32 num_frames)
{
	struct zerocopy_adaptor *zdptr = rb->zdptr;
	unsigned int qlen = zdptr->len;
	unsigned int out = zdptr->pre_rp;
	unsigned int data_len;

	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);

	u64 offset;
	u8 *src;

	int i, num_alloc_free = num_frames;
	int ret;
	int counter = 0;
	u32 reg_val;

	if (!num_frames) {
		dit_debug("num_frames is 0\n");
		return 0;
	}
	if (num_frames > MAX_DIT_SKB_NUM) {
		dit_debug("num_frames %d is over %d\n", num_frames, MAX_DIT_SKB_NUM);
		num_frames = MAX_DIT_SKB_NUM;
	}
	if (readl(dit_dev.reg_base + DIT_INT_PENDING) & 0x2) {
		dit_debug("DIT RX pending. return\n");
		return -EACCES;
	}
	if (readl(dit_dev.reg_base + DIT_STATUS) != 0x0) {
		dit_debug("DIT status:0x%08x. return\n", readl(dit_dev.reg_base + DIT_STATUS));
		return -EACCES;
	}

	dit_debug("num_frames:%d\n", num_frames);
	for (i = 0; i < num_frames; i++) {
		/* get buffer */
		data_len = rb->size_v[out] & 0xFFFF;
		offset = recv_offset_from_zerocopy_adaptor_for_dit(zdptr, out);
		dit_dev.zmb_buff[i] = data_offset_to_buffer_for_dit(offset, rb);
		if (!dit_dev.zmb_buff[i]) {
			mif_err_limited("data_offset_to_buffer() fail\n");
			ret = -ENOMEM;
			num_alloc_free = i;
			goto rx_error;
		}

		src = dit_dev.zmb_buff[i] + NET_HEADROOM;
#ifdef DIT_DEBUG_PKT
		pr_buffer("in", (char *)src, (size_t)data_len, (size_t)40);
#endif

#ifdef CONFIG_LINK_DEVICE_NAPI
		dit_skb[i] = napi_alloc_skb(&mld->mld_napi, data_len);
#else
		dit_skb[i] = dev_alloc_skb(data_len);
#endif
		if (!dit_skb[i]) {
			mif_err_limited("alloc_skb() fail\n");
			ret = -ENOMEM;
			num_alloc_free = i;
			goto rx_error;
		}

		/* packet descriptor */
		dit_pd[i].src_addr = virt_to_phys(src);
		dit_pd[i].dst_addr = virt_to_phys(skb_put(dit_skb[i], data_len));
		dit_pd[i].size = data_len;
		dit_pd[i].control = 0;
		dit_pd[i].control |= (0x1 << 5); /* csum */
		if (i == 0)
			dit_pd[i].control |= (0x1 << 7); /* head */
		if (i == num_frames - 1) {
			dit_pd[i].control |= (0x1 << 6); /* tail */
			dit_pd[i].control |= (0x1 << 4); /* int */
		}

		dit_debug("pd%d src:0x%llx dst:0x%llx size:%d control:0x%x\n",
				i, dit_pd[i].src_addr, dit_pd[i].dst_addr,
				dit_pd[i].size, dit_pd[i].control);

		out = circ_new_ptr(qlen, out, 1);
	}

	/* start dit transfer */
	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}
	writel(dit_dev.dma & 0xffffffff, dit_dev.reg_base + DIT_NAT_RX_DESC_ADDR_0);
	writel((dit_dev.dma >> 32) & 0xf, dit_dev.reg_base + DIT_NAT_RX_DESC_ADDR_1);
	writel(0x4, dit_dev.reg_base + DIT_SW_COMMAND);

	/* wait */
	counter = 0;
	while (1) {
		ndelay(100);
		reg_val = readl(dit_dev.reg_base + DIT_INT_PENDING);
		if ((reg_val & (0x1 << 1)) != 0) {
			reg_val |= (0x1 << 1);
			writel(reg_val, dit_dev.reg_base + DIT_INT_PENDING);
			dit_debug("Rx Done(%d)\n", counter);
			break;
		}
		if (counter++ >= 20000) {
			mif_err_limited("Can't get DIT RX\n");
			ret = -EPERM;
			num_alloc_free = num_frames;
			goto rx_error;
		}
	}

	/* received */
	dit_dev.pkt_on_zmb_cnt++;
	counter = 0;
	for (i = 0; i < num_frames; i++) {
		if ((dit_skb[i]->data[0] == 0xff) || (dit_skb[i]->data[0] == 0)) {
			mif_err_limited("dit recv error:0x%x %d pre_rp:%d\n",
						dit_skb[i]->data[0], i, zdptr->pre_rp);
			zdptr->pre_rp = circ_new_ptr(qlen, zdptr->pre_rp, 1);
			continue;
		}
#ifdef DIT_DEBUG_PKT
		pr_buffer("dit", (char *)(dit_skb[i]->data), (size_t)(dit_skb[i]->len), (size_t)40);
#endif
		set_lnk_hdr(rb, dit_skb[i]);
		ret = set_skb_priv_zerocopy_adaptor_for_dit(rb, dit_skb[i], zdptr->pre_rp);
		check_more(rb, dit_skb[i]);
		dit_skb[i]->ip_summed = CHECKSUM_UNNECESSARY;
		skb_trim(dit_skb[i], dit_pd[i].size);

		zdptr->pre_rp = circ_new_ptr(qlen, zdptr->pre_rp, 1);
		counter++;

		if (ret != 0) {
			mif_err_limited("recv error i:%d pre_rp:%d\n", i, zdptr->pre_rp);
			continue;
		}

		pass_skb_to_net(mld, dit_skb[i]);
	}

	for (i = 0; i < num_frames; i++)
		free_mif_buff(g_mif_buff_mng, dit_dev.zmb_buff[i]);

	return counter;

rx_error:
	for (i = 0; i < num_alloc_free; i++) {
		if (dit_skb[i])
			dev_kfree_skb_any(dit_skb[i]);
	}

	return ret;
}
#endif /* CONFIG_CP_ZEROCOPY */

static irqreturn_t dit_bus_err_handler(int irq, void *arg)
{
	mif_err_limited("0x%08x\n", readl(dit_dev.reg_base + DIT_INT_PENDING));

	writel((0x1 << 2), dit_dev.reg_base + DIT_INT_PENDING);

	mif_err_limited("DIT_STATUS:0x%08x\n", readl(dit_dev.reg_base + DIT_STATUS));
	mif_err_limited("DIT_BUS_ERR:0x%08x\n", readl(dit_dev.reg_base + DIT_BUS_ERROR));

	return IRQ_HANDLED;
}

static irqreturn_t dit_rx_done_handler(int irq, void *arg)
{
	dit_debug("0x%08x\n", readl(dit_dev.reg_base + DIT_INT_PENDING));

	writel((0x1 << 1), dit_dev.reg_base + DIT_INT_PENDING);

	return IRQ_HANDLED;
}

static irqreturn_t dit_tx_done_handler(int irq, void *arg)
{
	dit_debug("0x%08x\n", readl(dit_dev.reg_base + DIT_INT_PENDING));

	writel((0x1 << 0), dit_dev.reg_base + DIT_INT_PENDING);

	return IRQ_HANDLED;
}

void dit_init_hw(void)
{
	if (!dit_dev.reg_base) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}

	writel(0x4020, dit_dev.reg_base + DIT_DMA_INIT_DATA);
	writel(0x1, dit_dev.reg_base + DIT_SW_COMMAND);

	writel(0x7, dit_dev.reg_base + DIT_INT_ENABLE);
	writel(0x7, dit_dev.reg_base + DIT_INT_MASK);

	writel(0x0, dit_dev.reg_base + DIT_DMA_CHKSUM_OFF);
	writel(0x0, dit_dev.reg_base + DIT_SIZE_UPDATE_OFF);
	writel(0xf, dit_dev.reg_base + DIT_NAT_ZERO_CHK_OFF);

	writel(0x80, dit_dev.reg_base + DIT_TX_DESC_CTRL);
	writel(0x80, dit_dev.reg_base + DIT_TX_HEAD_CTRL);
	writel(0x20, dit_dev.reg_base + DIT_TX_MOD_HD_CTRL);
	writel(0x0, dit_dev.reg_base + DIT_TX_PKT_CTRL);
	writel(0x20, dit_dev.reg_base + DIT_TX_CHKSUM_CTRL);
	writel(0xc0, dit_dev.reg_base + DIT_RX_DESC_CTRL);
	writel(0xc0, dit_dev.reg_base + DIT_RX_HEAD_CTRL);
	writel(0x30, dit_dev.reg_base + DIT_RX_MOD_HD_CTRL);
	writel(0x0, dit_dev.reg_base + DIT_RX_PKT_CTRL);
	writel(0x30, dit_dev.reg_base + DIT_RX_CHKSUM_CTRL);

	writel(dit_dev.dma & 0xffffffff, dit_dev.reg_base + DIT_RX_RING_ST_ADDR_0);
	writel((dit_dev.dma >> 32) & 0xf, dit_dev.reg_base + DIT_RX_RING_ST_ADDR_1);

#ifdef DIT_CLOCK_ALWAYS_ON
	writel(0x3f, dit_dev.reg_base + DIT_CLK_GT_OFF);
#endif

	writel(0x1, dit_dev.busc_reg_base + 0x1700);
	writel(0x6, dit_dev.busc_reg_base + 0x1704);
}

/*
 * DIT NAT
 */
void dit_set_nat_local_addr(u32 addr)
{
	unsigned long flags;

	if (!dit_dev.reg_base) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}

	spin_lock_irqsave(&dit_dev.lock, flags);
	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}

	writel(addr, dit_dev.reg_base + DIT_NAT_LOCAL_ADDR);
	spin_unlock_irqrestore(&dit_dev.lock, flags);

	mif_info("local address:0x%x\n", readl(dit_dev.reg_base + DIT_NAT_LOCAL_ADDR));
}

void dit_set_nat_filter(u32 id, u8 proto, u32 sa, u16 sp, u16 dp)
{
	u32 val;
	unsigned long flags;

	if (!dit_dev.reg_base) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}

	spin_lock_irqsave(&dit_dev.lock, flags);

	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}

	/* protocol */
	val = readl(dit_dev.reg_base + DIT_NAT_PROTOCOL_FILTER_0 + (id/4)*4);
	val &= ~(0xFF << ((id%4)*8));
	val |= proto << ((id%4)*8);
	writel(val, dit_dev.reg_base + DIT_NAT_PROTOCOL_FILTER_0 + (id/4)*4);

	if (proto == IPPROTO_UDP)
		writel(0x5, dit_dev.reg_base + DIT_NAT_ZERO_CHK_OFF);
	else
		writel(0xf, dit_dev.reg_base + DIT_NAT_ZERO_CHK_OFF);

	/* source address */
	writel(sa, dit_dev.reg_base + DIT_NAT_SA_FILTER_0 + id*4);

	/* source port */
	val = readl(dit_dev.reg_base + DIT_NAT_SP_FILTER_0 + (id/2)*4);
	val &= ~(0xFFFF << ((id%2)*16));
	val |= sp << (id%2)*16;
	writel(val, dit_dev.reg_base + DIT_NAT_SP_FILTER_0 + (id/2)*4);

	/* destination port */
	val = readl(dit_dev.reg_base + DIT_NAT_DP_FILTER_0 + (id/2)*4);
	val &= ~(0xFFFF << ((id%2)*16));
	val |= dp << (id%2)*16;
	writel(val, dit_dev.reg_base + DIT_NAT_DP_FILTER_0 + (id/2)*4);

	spin_unlock_irqrestore(&dit_dev.lock, flags);

	mif_info("id:%d proto:0x%08x sa:0x%08x sp:0x%08x dp:0x%08x\n",
		id,
		readl(dit_dev.reg_base + DIT_NAT_PROTOCOL_FILTER_0 + (id/4)*4),
		readl(dit_dev.reg_base + DIT_NAT_SA_FILTER_0 + id*4),
		readl(dit_dev.reg_base + DIT_NAT_SP_FILTER_0 + (id/2)*4),
		readl(dit_dev.reg_base + DIT_NAT_DP_FILTER_0 + (id/2)*4));
}

void dit_del_nat_filter(u32 id)
{
	dit_set_nat_filter(id, 0xFF, 0xFFFFFFFF, 0xFFFF, 0xFFFF);
}

/* set/delete plat prefix of IPv6 addr */
void dit_set_clat_plat_prfix(u32 id, struct in6_addr addr)
{
	unsigned long flags;

	if (!dit_dev.reg_base) {
		mif_info("DIT-clat is not initialized\n");
		return;
	}

	spin_lock_irqsave(&dit_dev.lock, flags);
	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}

	writel(addr.s6_addr32[0], dit_dev.reg_base + DIT_CLAT_TX_PLAT_PREFIX_A0 + id*12);
	writel(addr.s6_addr32[1], dit_dev.reg_base + DIT_CLAT_TX_PLAT_PREFIX_A1 + id*12);
	writel(addr.s6_addr32[2], dit_dev.reg_base + DIT_CLAT_TX_PLAT_PREFIX_A2 + id*12);
	spin_unlock_irqrestore(&dit_dev.lock, flags);

	mif_info("plat_prfix(%d) address: %08x %08x %08x\n", id,
		readl(dit_dev.reg_base + DIT_CLAT_TX_PLAT_PREFIX_A0 + id*12),
		readl(dit_dev.reg_base + DIT_CLAT_TX_PLAT_PREFIX_A1 + id*12),
		readl(dit_dev.reg_base + DIT_CLAT_TX_PLAT_PREFIX_A2 + id*12));
}

void dit_del_clat_plat_prfix(u32 id)
{
	struct in6_addr addr = IN6ADDR_ANY_INIT;

	dit_set_clat_plat_prfix(id, addr);
}

/* set/delete clat src addr of IPv6 addr */
void dit_set_clat_saddr(u32 id, struct in6_addr addr)
{
	unsigned long flags;

	if (!dit_dev.reg_base) {
		mif_info("DIT-clat is not initialized\n");
		return;
	}

	spin_lock_irqsave(&dit_dev.lock, flags);
	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}

	writel(addr.s6_addr32[0], dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A0 + id*16);
	writel(addr.s6_addr32[1], dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A1 + id*16);
	writel(addr.s6_addr32[2], dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A2 + id*16);
	writel(addr.s6_addr32[3], dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A3 + id*16);
	spin_unlock_irqrestore(&dit_dev.lock, flags);

	mif_info("clat filter(%d) address: %08x %08x %08x %08x\n", id,
		readl(dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A0 + id*16),
		readl(dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A1 + id*16),
		readl(dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A2 + id*16),
		readl(dit_dev.reg_base + DIT_CLAT_TX_CLAT_SRC_A3 + id*16));
}

void dit_del_clat_saddr(u32 id)
{
	struct in6_addr addr = IN6ADDR_ANY_INIT;

	dit_set_clat_saddr(id, addr);
}

/* set/delete clat filter of IPv4 addr */
void dit_set_clat_filter(u32 id, u32 addr)
{
	unsigned long flags;

	if (!dit_dev.reg_base) {
		mif_info("DIT-clat is not initialized\n");
		return;
	}

	spin_lock_irqsave(&dit_dev.lock, flags);
	if (readl(dit_dev.reg_base + DIT_DMA_INIT_DATA) == 0) {
		mif_info("reinit dit hw\n");
		dit_init_hw();
	}

	writel(addr, dit_dev.reg_base + DIT_CLAT_TX_FILTER_A + id*4);
	spin_unlock_irqrestore(&dit_dev.lock, flags);

	mif_info("clat filter(%d) address:0x%x\n", id, readl(dit_dev.reg_base + DIT_CLAT_TX_FILTER_A + id*4));
}

void dit_del_clat_filter(u32 id)
{
	u32 addr = INADDR_ANY;

	dit_set_clat_filter(id, addr);
}

/*
 * sysfs
 */
static ssize_t dit_forced_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "dit forced_enable:%d\n", dit_dev.forced_enable);
}

static ssize_t dit_forced_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int id = 0;

	ret = sscanf(buf, "%u", &id);

	dit_dev.forced_enable = id;

	ret = count;
	return ret;
}

static ssize_t dit_pkt_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "from pktproc:%d from zmb:%d\n", dit_dev.pkt_on_pktproc_cnt, dit_dev.pkt_on_zmb_cnt);
}

static DEVICE_ATTR_RW(dit_forced_enable);
static DEVICE_ATTR_RO(dit_pkt_count);

static struct attribute *dit_attrs[] = {
	&dev_attr_dit_forced_enable.attr,
	&dev_attr_dit_pkt_count.attr,
	NULL,
};

static const struct attribute_group dit_group = {
	.attrs = dit_attrs,
	.name = "dit",
};

/*
 * init
 */
int dit_init(struct platform_device *pdev)
{
	struct resource *dit_res;
	struct resource *busc_res;
	int err = 0;
	u32 alloc_size;

	mif_info("+++\n");

	dit_dev.pkt_on_pktproc_cnt = 0;
	dit_dev.pkt_on_zmb_cnt = 0;

	if (dit_dev.reg_base) {
		mif_info("alread initialized\n");
		return 0;
	}

	/* get resources */
	dit_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dit");
	dit_dev.reg_base = devm_ioremap_resource(&pdev->dev, dit_res);
	if (!dit_dev.reg_base) {
		mif_err("DIT devm_ioremap_resource() error\n");
		return -1;
	}
	mif_info("DIT base address : 0x%llX\n", dit_res->start);

	dit_dev.dev = &pdev->dev;
	spin_lock_init(&dit_dev.lock);

	alloc_size = sizeof(struct packet_descriptor) * MAX_DIT_SKB_NUM;
	dit_pd = dma_zalloc_coherent(&pdev->dev, alloc_size, &dit_dev.dma, GFP_KERNEL);
	if (!dit_pd) {
		mif_err("dma alloc() error\n");
		goto init_error;
	}

	/* IO coherency : DIT_SHARABILITY_CTRL0 */
	busc_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sysreg_busc");
	dit_dev.busc_reg_base = devm_ioremap_resource(&pdev->dev, busc_res);
	if (!dit_dev.busc_reg_base) {
		mif_err("DIT busc devm_ioremap_resource() error\n");
		goto init_error;
	}
	dma_set_mask_and_coherent(dit_dev.dev, DMA_BIT_MASK(36));
	dit_init_hw();

	/* register interrupt handlers */
	dit_dev.bus_err_int = platform_get_irq_byname(pdev, "DIT_BUS_ERR");
	err = request_irq(dit_dev.bus_err_int, dit_bus_err_handler, 0, "DIT BUS Error", NULL);
	if (err) {
		mif_err("Can't request IRQ - dit_bus_err_handler!!!\n");
		goto init_error;
	}

	dit_dev.rx_done_int = platform_get_irq_byname(pdev, "DIT_RX_DONE");
	err = request_irq(dit_dev.rx_done_int, dit_rx_done_handler, 0, "DIT RX Done", NULL);
	if (err) {
		mif_err("Can't request IRQ - dit_rx_done_handler!!!\n");
		goto init_error;
	}
#ifndef DIT_USE_RXTX_DONE_INT
	disable_irq(dit_dev.rx_done_int);
#endif

	dit_dev.tx_done_int = platform_get_irq_byname(pdev, "DIT_TX_DONE");
	err = request_irq(dit_dev.tx_done_int, dit_tx_done_handler, 0, "DIT TX Done", NULL);
	if (err) {
		mif_err("Can't request IRQ - dit_tx_done_handler!!!\n");
		goto init_error;
	}
#ifndef DIT_USE_RXTX_DONE_INT
	disable_irq(dit_dev.tx_done_int);
#endif

	mif_info("DIT interrupts - bus_err:%d, rx_done:%d, tx_done:%d\n",
		dit_dev.bus_err_int, dit_dev.rx_done_int, dit_dev.tx_done_int);

#if defined(CONFIG_CP_ZEROCOPY)
	alloc_size = sizeof(u8 *) * MAX_DIT_SKB_NUM;
	dit_dev.zmb_buff = kmalloc(alloc_size, GFP_KERNEL);
	if (unlikely(!dit_dev.zmb_buff)) {
		mif_err("alloc buff error\n");
		goto init_error;
	}
#endif

	if (sysfs_create_group(&pdev->dev.kobj, &dit_group))
		mif_err("failed to create sysfs node related dit\n");

	dit_dev.forced_enable = 0;

	mif_info("---\n");

	return 0;

init_error:
#if defined(CONFIG_CP_ZEROCOPY)
	kfree(dit_dev.zmb_buff);
#endif
	kfree(dit_pd);
	if (dit_dev.busc_reg_base)
		devm_iounmap(&pdev->dev, dit_dev.busc_reg_base);
	if (dit_dev.reg_base)
		devm_iounmap(&pdev->dev, dit_dev.reg_base);

	return -1;
}
