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

#include <linux/smc.h>
#include <linux/pmu_cp.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "modem_dump.h"

#ifdef GROUP_MEM_LINK_DEVICE

void mem_irq_handler(struct mem_link_device *mld, struct mst_buff *msb)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	u16 intr = msb->snapshot.int2ap;

	if (unlikely(!int_valid(intr))) {
		mif_err("%s: ERR! invalid intr 0x%X\n", ld->name, intr);
		msb_free(msb);
		return;
	}

	if (unlikely(!rx_possible(mc))) {
		mif_err("%s: ERR! %s.state == %s\n", ld->name, mc->name,
			mc_state(mc));
		msb_free(msb);
		return;
	}

	msb_queue_tail(&mld->msb_rxq, msb);

	tasklet_schedule(&mld->rx_tsk);
}

static inline bool ipc_active(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (unlikely(!cp_online(mc))) {
		mif_err("%s<->%s: %s.state %s != ONLINE <%pf>\n",
			ld->name, mc->name, mc->name, mc_state(mc), CALLER);
		return false;
	}

	if (mld->dpram_magic) {
		unsigned int magic = get_magic(mld);
		unsigned int access = get_access(mld);
		if (magic != MEM_IPC_MAGIC || access != 1) {
			mif_err("%s<->%s: ERR! magic:0x%X access:%d <%pf>\n",
				ld->name, mc->name, magic, access, CALLER);
			return false;
		}
	}

	if (atomic_read(&mc->forced_cp_crash)) {
		mif_err("%s<->%s: ERR! forced_cp_crash:%d <%pf>\n",
			ld->name, mc->name, atomic_read(&mc->forced_cp_crash),
			CALLER);
		return false;
	}

	return true;
}

static inline void stop_tx(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	stop_net_ifaces(ld);
}

static inline void purge_txq(struct mem_link_device *mld)
{
	int i;
#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	struct link_device *ld = &mld->link_dev;

	/* Purge the skb_q in every TX RB */
	if (ld->sbd_ipc) {
		struct sbd_link_device *sl = &mld->sbd_link_dev;
		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, TX);
			skb_queue_purge(&rb->skb_q);
		}
	}
#endif

	/* Purge the skb_txq in every IPC device
	 * (IPC_MAP_FMT, IPC_MAP_NORM_RAW, etc.)
	 */
	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		skb_queue_purge(dev->skb_txq);
	}
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_IPC_TX

static inline int check_txq_space(struct mem_link_device *mld,
				  struct mem_ipc_device *dev,
				  unsigned int qsize, unsigned int in,
				  unsigned int out, unsigned int count)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned int usage;
	unsigned int space;

	if (!circ_valid(qsize, in, out)) {
		mif_err("%s: ERR! Invalid %s_TXQ{qsize:%d in:%d out:%d}\n",
			ld->name, dev->name, qsize, in, out);
		return -EIO;
	}

	usage = circ_get_usage(qsize, in, out);
	if (unlikely(usage > SHM_UL_USAGE_LIMIT) && cp_online(mc)) {
		mif_debug("%s: CAUTION! BUSY in %s_TXQ{qsize:%d in:%d out:%d "
			"usage:%d (count:%d)}\n", ld->name, dev->name, qsize,
			in, out, usage, count);
		return -EBUSY;
	}

	space = circ_get_space(qsize, in, out);
	if (unlikely(space < count)) {
		if (cp_online(mc)) {
			mif_err("%s: CAUTION! NOSPC in %s_TXQ{qsize:%d in:%d "
				"out:%d space:%d count:%d}\n", ld->name,
				dev->name, qsize, in, out, space, count);
		}
		return -ENOSPC;
	}

	return space;
}

static int txq_write(struct mem_link_device *mld, struct mem_ipc_device *dev,
		     struct sk_buff *skb)
{
	char *src = skb->data;
	unsigned int count = skb->len;
	char *dst = get_txq_buff(dev);
	unsigned int qsize = get_txq_buff_size(dev);
	unsigned int in = get_txq_head(dev);
	unsigned int out = get_txq_tail(dev);
	int space;

	space = check_txq_space(mld, dev, qsize, in, out, count);
	if (unlikely(space < 0))
		return space;

	barrier();

	circ_write(dst, src, qsize, in, count);

	barrier();

	set_txq_head(dev, circ_new_ptr(qsize, in, count));

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

static int tx_frames_to_dev(struct mem_link_device *mld,
			    struct mem_ipc_device *dev)
{
	struct sk_buff_head *skb_txq = dev->skb_txq;
	int tx_bytes = 0;
	int ret = 0;

	while (1) {
		struct sk_buff *skb;

		skb = skb_dequeue(skb_txq);
		if (unlikely(!skb))
			break;

		ret = txq_write(mld, dev, skb);
		if (unlikely(ret < 0)) {
			/* Take the skb back to the skb_txq */
			skb_queue_head(skb_txq, skb);
			break;
		}

		tx_bytes += ret;

#ifdef DEBUG_MODEM_IF_LINK_TX
		mif_pkt(skbpriv(skb)->sipc_ch, "LNK-TX", skb);
#endif

		dev_kfree_skb_any(skb);
	}

	return (ret < 0) ? ret : tx_bytes;
}

static enum hrtimer_restart tx_timer_func(struct hrtimer *timer)
{
	struct mem_link_device *mld;
	struct link_device *ld;
	struct modem_ctl *mc;
	int i;
	bool need_schedule;
	u16 mask;
	unsigned long flags;

	mld = container_of(timer, struct mem_link_device, tx_timer);
	ld = &mld->link_dev;
	mc = ld->mc;

	need_schedule = false;
	mask = 0;

	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(!ipc_active(mld)))
		goto exit;

#ifdef CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM
	if (mld->link_active) {
		if (!mld->link_active(mld)) {
			need_schedule = true;
			goto exit;
		}
	}
#endif

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		int ret;

		if (unlikely(under_tx_flow_ctrl(mld, dev))) {
			ret = check_tx_flow_ctrl(mld, dev);
			if (ret < 0) {
				if (ret == -EBUSY || ret == -ETIME) {
					need_schedule = true;
					continue;
				} else {
					mem_forced_cp_crash(mld);
					need_schedule = false;
					goto exit;
				}
			}
		}

		ret = tx_frames_to_dev(mld, dev);
		if (unlikely(ret < 0)) {
			if (ret == -EBUSY || ret == -ENOSPC) {
				need_schedule = true;
				start_tx_flow_ctrl(mld, dev);
				continue;
			} else {
				mem_forced_cp_crash(mld);
				need_schedule = false;
				goto exit;
			}
		}

		if (ret > 0)
			mask |= msg_mask(dev);

		if (!skb_queue_empty(dev->skb_txq))
			need_schedule = true;
	}

	if (!need_schedule) {
		for (i = 0; i < MAX_SIPC_MAP; i++) {
			if (!txq_empty(mld->dev[i])) {
				need_schedule = true;
				break;
			}
		}
	}

	if (mask)
		send_ipc_irq(mld, mask2int(mask));

exit:
	if (need_schedule) {
		ktime_t ktime = ktime_set(0, ms2ns(TX_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

	spin_unlock_irqrestore(&mc->lock, flags);

	return HRTIMER_NORESTART;
}

static inline void start_tx_timer(struct mem_link_device *mld,
				  struct hrtimer *timer)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&mc->lock, flags);

	if (unlikely(cp_offline(mc)))
		goto exit;

	if (!hrtimer_is_queued(timer)) {
		ktime_t ktime = ktime_set(0, ms2ns(TX_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

exit:
	spin_unlock_irqrestore(&mc->lock, flags);
}

static inline void cancel_tx_timer(struct mem_link_device *mld,
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

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
static int tx_frames_to_rb(struct sbd_ring_buffer *rb)
{
	struct sk_buff_head *skb_txq = &rb->skb_q;
	int tx_bytes = 0;
	int ret = 0;

	while (1) {
		struct sk_buff *skb;
#ifdef DEBUG_MODEM_IF
		u8 *hdr;
#endif

		skb = skb_dequeue(skb_txq);
		if (unlikely(!skb))
			break;

		ret = sbd_pio_tx(rb, skb);
		if (unlikely(ret < 0)) {
			/* Take the skb back to the skb_txq */
			skb_queue_head(skb_txq, skb);
			break;
		}

		tx_bytes += ret;

#ifdef DEBUG_MODEM_IF
		hdr = skbpriv(skb)->lnk_hdr ? skb->data : NULL;
#ifdef DEBUG_MODEM_IF_IP_DATA
		if (sipc_ps_ch(rb->ch)) {
			u8 *ip_pkt = skb->data;
			if (hdr)
				ip_pkt += sipc5_get_hdr_len(hdr);
			print_ipv4_packet(ip_pkt, TX);
		}
#endif
#ifdef DEBUG_MODEM_IF_LINK_TX
		mif_pkt(rb->ch, "LNK-TX", skb);
#endif
#endif
		dev_kfree_skb_any(skb);
	}

	return (ret < 0) ? ret : tx_bytes;
}

static enum hrtimer_restart sbd_tx_timer_func(struct hrtimer *timer)
{
	struct mem_link_device *mld;
	struct link_device *ld;
	struct modem_ctl *mc;
	struct sbd_link_device *sl;
	int i;
	bool need_schedule;
	u16 mask;
	unsigned long flags = 0;

	mld = container_of(timer, struct mem_link_device, sbd_tx_timer);
	ld = &mld->link_dev;
	mc = ld->mc;
	sl = &mld->sbd_link_dev;

	need_schedule = false;
	mask = 0;

	spin_lock_irqsave(&mc->lock, flags);
	if (unlikely(!ipc_active(mld))) {
		spin_unlock_irqrestore(&mc->lock, flags);
		goto exit;
	}
	spin_unlock_irqrestore(&mc->lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM
	if (mld->link_active) {
		if (!mld->link_active(mld)) {
			need_schedule = true;
			goto exit;
		}
	}
#endif

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, TX);
		int ret;

		ret = tx_frames_to_rb(rb);
		if (unlikely(ret < 0)) {
			if (ret == -EBUSY || ret == -ENOSPC) {
				need_schedule = true;
				mask = MASK_SEND_DATA;
				continue;
			} else {
				mem_forced_cp_crash(mld);
				need_schedule = false;
				goto exit;
			}
		}

		if (ret > 0)
			mask = MASK_SEND_DATA;

		if (!skb_queue_empty(&rb->skb_q))
			need_schedule = true;
	}

	if (!need_schedule) {
		for (i = 0; i < sl->num_channels; i++) {
			struct sbd_ring_buffer *rb;

			rb = sbd_id2rb(sl, i, TX);
			if (!rb_empty(rb)) {
				need_schedule = true;
				break;
			}
		}
	}

	if (mask) {
		spin_lock_irqsave(&mc->lock, flags);
		if (unlikely(!ipc_active(mld))) {
			spin_unlock_irqrestore(&mc->lock, flags);
			need_schedule = false;
			goto exit;
		}
		send_ipc_irq(mld, mask2int(mask));
		spin_unlock_irqrestore(&mc->lock, flags);
	}

exit:
	if (need_schedule) {
		ktime_t ktime = ktime_set(0, ms2ns(TX_PERIOD_MS));
		hrtimer_start(timer, ktime, HRTIMER_MODE_REL);
	}

	return HRTIMER_NORESTART;
}

static int xmit_ipc_to_rb(struct mem_link_device *mld, enum sipc_ch_id ch,
			  struct sk_buff *skb)
{
	int ret;
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = ld->mc;
	struct sbd_ring_buffer *rb = sbd_ch2rb(&mld->sbd_link_dev, ch, TX);
	struct sk_buff_head *skb_txq;
	unsigned long flags;

	if (!rb) {
		mif_err("%s: %s->%s: ERR! NO SBD RB {ch:%d}\n",
			ld->name, iod->name, mc->name, ch);
		return -ENODEV;
	}

	skb_txq = &rb->skb_q;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->forbid_cp_sleep)
		mld->forbid_cp_sleep(mld);
#endif

	spin_lock_irqsave(&rb->lock, flags);

	if (unlikely(skb_txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err_limited("%s: %s->%s: ERR! {ch:%d} "
				"skb_txq.len %d >= limit %d\n",
				ld->name, iod->name, mc->name, ch,
				skb_txq->qlen, MAX_SKB_TXQ_DEPTH);
		ret = -EBUSY;
	} else {
		ret = skb->len;
		skb_queue_tail(skb_txq, skb);
		start_tx_timer(mld, &mld->sbd_tx_timer);
	}

	spin_unlock_irqrestore(&rb->lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->permit_cp_sleep)
		mld->permit_cp_sleep(mld);
#endif

	return ret;
}
#endif

static int xmit_ipc_to_dev(struct mem_link_device *mld, enum sipc_ch_id ch,
			   struct sk_buff *skb)
{
	int ret;
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = ld->mc;
	struct mem_ipc_device *dev = mld->dev[get_mmap_idx(ch, skb)];
	struct sk_buff_head *skb_txq;
	unsigned long flags;

	if (!dev) {
		mif_err("%s: %s->%s: ERR! NO IPC DEV {ch:%d}\n",
			ld->name, iod->name, mc->name, ch);
		return -ENODEV;
	}

	skb_txq = dev->skb_txq;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->forbid_cp_sleep)
		mld->forbid_cp_sleep(mld);
#endif

	spin_lock_irqsave(dev->tx_lock, flags);

	if (unlikely(skb_txq->qlen >= MAX_SKB_TXQ_DEPTH)) {
		mif_err_limited("%s: %s->%s: ERR! %s TXQ.qlen %d >= limit %d\n",
				ld->name, iod->name, mc->name, dev->name,
				skb_txq->qlen, MAX_SKB_TXQ_DEPTH);
		ret = -EBUSY;
	} else {
		ret = skb->len;
		skb_queue_tail(dev->skb_txq, skb);
		start_tx_timer(mld, &mld->tx_timer);
	}

	spin_unlock_irqrestore(dev->tx_lock, flags);

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (cp_online(mc) && mld->permit_cp_sleep)
		mld->permit_cp_sleep(mld);
#endif

	return ret;
}

static int xmit_ipc(struct mem_link_device *mld, struct io_device *iod,
		    enum sipc_ch_id ch, struct sk_buff *skb)
{
	if (unlikely(!ipc_active(mld)))
		return -EIO;

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	if (iod->sbd_ipc) {
		if (likely(sbd_active(&mld->sbd_link_dev)))
			return xmit_ipc_to_rb(mld, ch, skb);
		else
			return -ENODEV;
	} else {
		return xmit_ipc_to_dev(mld, ch, skb);
	}
#else
	return xmit_ipc_to_dev(mld, ch, skb);
#endif
}

static inline int check_udl_space(struct mem_link_device *mld,
				  struct mem_ipc_device *dev,
				  unsigned int qsize, unsigned int in,
				  unsigned int out, unsigned int count)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int space;

	if (!circ_valid(qsize, in, out)) {
		mif_err("%s: ERR! Invalid %s_TXQ{qsize:%d in:%d out:%d}\n",
			ld->name, dev->name, qsize, in, out);
		return -EIO;
	}

	space = circ_get_space(qsize, in, out);
	if (unlikely(space < count)) {
		mif_err("%s: NOSPC in %s_TXQ{qsize:%d in:%d "
			"out:%d space:%d count:%d}\n", ld->name,
			dev->name, qsize, in, out, space, count);
		return -ENOSPC;
	}

	return 0;
}

static inline int udl_write(struct mem_link_device *mld,
			    struct mem_ipc_device *dev, struct sk_buff *skb)
{
	unsigned int count = skb->len;
	char *src = skb->data;
	char *dst = get_txq_buff(dev);
	unsigned int qsize = get_txq_buff_size(dev);
	unsigned int in = get_txq_head(dev);
	unsigned int out = get_txq_tail(dev);
	int space;

	space = check_udl_space(mld, dev, qsize, in, out, count);
	if (unlikely(space < 0))
		return space;

	barrier();

	circ_write(dst, src, qsize, in, count);

	barrier();

	set_txq_head(dev, circ_new_ptr(qsize, in, count));

	/* Commit the item before incrementing the head */
	smp_mb();

	return count;
}

static int xmit_udl(struct mem_link_device *mld, struct io_device *iod,
		    enum sipc_ch_id ch, struct sk_buff *skb)
{
	int ret;
	struct mem_ipc_device *dev = mld->dev[IPC_MAP_NORM_RAW];
	int count = skb->len;
	int tried = 0;

	while (1) {
		ret = udl_write(mld, dev, skb);
		if (ret == count)
			break;

		if (ret != -ENOSPC)
			goto exit;

		tried++;
		if (tried >= 20)
			goto exit;

		if (in_interrupt())
			mdelay(50);
		else
			msleep(50);
	}

#ifdef DEBUG_MODEM_IF_LINK_TX
	mif_pkt(ch, "LNK-TX", skb);
#endif

	dev_kfree_skb_any(skb);

exit:
	return ret;
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_IPC_RX

static void pass_skb_to_demux(struct mem_link_device *mld, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;
	struct io_device *iod = skbpriv(skb)->iod;
	int ret;
	u8 ch = skbpriv(skb)->sipc_ch;

	if (unlikely(!iod)) {
		mif_err("%s: ERR! No IOD for CH.%d\n", ld->name, ch);
		dev_kfree_skb_any(skb);
		mem_forced_cp_crash(mld);
		return;
	}

#ifdef DEBUG_MODEM_IF_LINK_RX
	mif_pkt(ch, "LNK-RX", skb);
#endif

	ret = iod->recv_skb_single(iod, ld, skb);
	if (unlikely(ret < 0)) {
		struct modem_ctl *mc = ld->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s->recv_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
		dev_kfree_skb_any(skb);
	}
}

static inline void link_to_demux(struct mem_link_device  *mld)
{
	int i;

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		struct sk_buff_head *skb_rxq = dev->skb_rxq;

		while (1) {
			struct sk_buff *skb;

			skb = skb_dequeue(skb_rxq);
			if (!skb)
				break;

			pass_skb_to_demux(mld, skb);
		}
	}
}

static void link_to_demux_work(struct work_struct *ws)
{
	struct link_device *ld;
	struct mem_link_device *mld;

	ld = container_of(ws, struct link_device, rx_delayed_work.work);
	mld = to_mem_link_device(ld);

	link_to_demux(mld);
}

static inline void schedule_link_to_demux(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct delayed_work *dwork = &ld->rx_delayed_work;

	/*queue_delayed_work(ld->rx_wq, dwork, 0);*/
	queue_work_on(7, ld->rx_wq, &dwork->work);
}

static struct sk_buff *rxq_read(struct mem_link_device *mld,
				struct mem_ipc_device *dev,
				unsigned int in)
{
	struct link_device *ld = &mld->link_dev;
	struct sk_buff *skb;
	char *src = get_rxq_buff(dev);
	unsigned int qsize = get_rxq_buff_size(dev);
	unsigned int out = get_rxq_tail(dev);
	unsigned int rest = circ_get_usage(qsize, in, out);
	unsigned int len;
	char hdr[SIPC5_MIN_HEADER_SIZE];

	/* Copy the header in a frame to the header buffer */
	circ_read(hdr, src, qsize, out, SIPC5_MIN_HEADER_SIZE);

	/* Check the config field in the header */
	if (unlikely(!sipc5_start_valid(hdr))) {
		mif_err("%s: ERR! %s BAD CFG 0x%02X (in:%d out:%d rest:%d)\n",
			ld->name, dev->name, hdr[SIPC5_CONFIG_OFFSET],
			in, out, rest);
		goto bad_msg;
	}

	/* Verify the length of the frame (data + padding) */
	len = sipc5_get_total_len(hdr);
	if (unlikely(len > rest)) {
		mif_err("%s: ERR! %s BAD LEN %d > rest %d\n",
			ld->name, dev->name, len, rest);
		goto bad_msg;
	}

	/* Allocate an skb */
	skb = mem_alloc_skb(len);
	if (!skb) {
		mif_err("%s: ERR! %s mem_alloc_skb(%d) fail\n",
			ld->name, dev->name, len);
		goto no_mem;
	}

	/* Read the frame from the RXQ */
	circ_read(skb_put(skb, len), src, qsize, out, len);

	/* Update tail (out) pointer to the frame to be read in the future */
	set_rxq_tail(dev, circ_new_ptr(qsize, out, len));

	/* Finish reading data before incrementing tail */
	smp_mb();

#ifdef DEBUG_MODEM_IF
	/* Record the time-stamp */
	getnstimeofday(&skbpriv(skb)->ts);
#endif

	return skb;

bad_msg:
	mif_err("%s%s%s: ERR! BAD MSG: %02x %02x %02x %02x\n",
		ld->name, arrow(RX), ld->mc->name,
		hdr[0], hdr[1], hdr[2], hdr[3]);
	set_rxq_tail(dev, in);	/* Reset tail (out) pointer */
	mem_forced_cp_crash(mld);

no_mem:
	return NULL;
}

static int rx_frames_from_dev(struct mem_link_device *mld,
			      struct mem_ipc_device *dev)
{
	struct link_device *ld = &mld->link_dev;
	struct sk_buff_head *skb_rxq = dev->skb_rxq;
	unsigned int qsize = get_rxq_buff_size(dev);
	unsigned int in = get_rxq_head(dev);
	unsigned int out = get_rxq_tail(dev);
	unsigned int size = circ_get_usage(qsize, in, out);
	int rcvd = 0;

	if (unlikely(circ_empty(in, out)))
		return 0;

	while (rcvd < size) {
		struct sk_buff *skb;
		u8 ch;
		struct io_device *iod;

		skb = rxq_read(mld, dev, in);
		if (!skb)
			break;

		ch = sipc5_get_ch(skb->data);
		iod = link_get_iod_with_channel(ld, ch);
		if (!iod) {
			mif_err("%s: ERR! No IOD for CH.%d\n", ld->name, ch);
			dev_kfree_skb_any(skb);
			mem_forced_cp_crash(mld);
			break;
		}

		/* Record the IO device and the link device into the &skb->cb */
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		skbpriv(skb)->lnk_hdr = iod->link_header;
		skbpriv(skb)->sipc_ch = ch;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd += skb->len;

		if (likely(sipc_ps_ch(ch)))
			skb_queue_tail(skb_rxq, skb);
		else
			pass_skb_to_demux(mld, skb);
	}

	if (rcvd < size) {
		struct link_device *ld = &mld->link_dev;
		mif_err("%s: WARN! rcvd %d < size %d\n", ld->name, rcvd, size);
	}

	return rcvd;
}

static void recv_ipc_frames(struct mem_link_device *mld,
			    struct mem_snapshot *mst)
{
	int i;
	u16 intr = mst->int2ap;

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];
		int rcvd;

		if (req_ack_valid(dev, intr))
			recv_req_ack(mld, dev, mst);

		rcvd = rx_frames_from_dev(mld, dev);
		if (rcvd < 0)
			break;

		schedule_link_to_demux(mld);

		if (req_ack_valid(dev, intr))
			send_res_ack(mld, dev);

		if (res_ack_valid(dev, intr))
			recv_res_ack(mld, dev, mst);
	}
}

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
static void pass_skb_to_net(struct mem_link_device *mld, struct sk_buff *skb)
{
	struct link_device *ld = &mld->link_dev;
	struct skbuff_private *priv;
	struct io_device *iod;
	int ret;

	priv = skbpriv(skb);
	if (unlikely(!priv)) {
		mif_err("%s: ERR! No PRIV in skb@%pK\n", ld->name, skb);
		dev_kfree_skb_any(skb);
		mem_forced_cp_crash(mld);
		return;
	}

	iod = priv->iod;
	if (unlikely(!iod)) {
		mif_err("%s: ERR! No IOD in skb@%pK\n", ld->name, skb);
		dev_kfree_skb_any(skb);
		mem_forced_cp_crash(mld);
		return;
	}

#if defined(DEBUG_MODEM_IF_LINK_RX) && defined(DEBUG_MODEM_IF_PS_DATA)
	mif_pkt(iod->id, "LNK-RX", skb);
#endif

	ret = iod->recv_net_skb(iod, ld, skb);
	if (unlikely(ret < 0)) {
		struct modem_ctl *mc = ld->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s->recv_net_skb fail (%d)\n",
				ld->name, iod->name, mc->name, iod->name, ret);
		dev_kfree_skb_any(skb);
	}
}

static int rx_net_frames_from_rb(struct sbd_ring_buffer *rb)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned int num_frames = rb_usage(rb);

	while (rcvd < num_frames) {
		struct sk_buff *skb;

		skb = sbd_pio_rx(rb);
		if (!skb)
			break;

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_net(). */
		rcvd++;

		pass_skb_to_net(mld, skb);
	}

	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct link_device *ld = rb->ld;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}

	return rcvd;
}

static int rx_ipc_frames_from_rb(struct sbd_ring_buffer *rb)
{
	int rcvd = 0;
	struct link_device *ld = rb->ld;
	struct mem_link_device *mld = ld_to_mem_link_device(ld);
	unsigned int qlen = rb->len;
	unsigned int in = *rb->wp;
	unsigned int out = *rb->rp;
	unsigned int num_frames = circ_get_usage(qlen, in, out);

	while (rcvd < num_frames) {
		struct sk_buff *skb;

		skb = sbd_pio_rx(rb);
		if (!skb) {
			/* TODO : Replace with panic() */
			mem_forced_cp_crash(mld);
			break;
		}

		/* The $rcvd must be accumulated here, because $skb can be freed
		   in pass_skb_to_demux(). */
		rcvd++;

		if (skbpriv(skb)->lnk_hdr) {
			u8 ch = rb->ch;
			u8 fch = sipc5_get_ch(skb->data);
			if (fch != ch) {
				mif_err("frm.ch:%d != rb.ch:%d\n", fch, ch);
				dev_kfree_skb_any(skb);
				continue;
			}
		}

		pass_skb_to_demux(mld, skb);
	}

	if (rcvd < num_frames) {
		struct io_device *iod = rb->iod;
		struct modem_ctl *mc = ld->mc;
		mif_err("%s: %s<-%s: WARN! rcvd %d < num_frames %d\n",
			ld->name, iod->name, mc->name, rcvd, num_frames);
	}

	return rcvd;
}

static void recv_sbd_ipc_frames(struct mem_link_device *mld,
				struct mem_snapshot *mst)
{
	struct sbd_link_device *sl = &mld->sbd_link_dev;
	int i;

	for (i = 0; i < sl->num_channels; i++) {
		struct sbd_ring_buffer *rb = sbd_id2rb(sl, i, RX);

		if (unlikely(rb_empty(rb)))
			continue;

		if (likely(sipc_ps_ch(rb->ch)))
			rx_net_frames_from_rb(rb);
		else
			rx_ipc_frames_from_rb(rb);
	}
}
#endif

static void ipc_rx_func(struct mem_link_device *mld)
{
#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	struct sbd_link_device *sl = &mld->sbd_link_dev;
#endif

	while (1) {
		struct mst_buff *msb;
		u16 intr;

		msb = msb_dequeue(&mld->msb_rxq);
		if (!msb)
			break;

		intr = msb->snapshot.int2ap;

		if (cmd_valid(intr))
			mld->cmd_handler(mld, int2cmd(intr));

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
		if (sbd_active(sl))
			recv_sbd_ipc_frames(mld, &msb->snapshot);
		else
			recv_ipc_frames(mld, &msb->snapshot);
#else
		recv_ipc_frames(mld, &msb->snapshot);
#endif

		msb_free(msb);
	}
}

static void udl_rx_work(struct work_struct *ws)
{
	struct mem_link_device *mld;

	mld = container_of(ws, struct mem_link_device, udl_rx_dwork.work);

	ipc_rx_func(mld);
}

static void mem_rx_task(unsigned long data)
{
	struct mem_link_device *mld = (struct mem_link_device *)data;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (likely(cp_online(mc)))
		ipc_rx_func(mld);
	else
		queue_delayed_work(ld->rx_wq, &mld->udl_rx_dwork, 0);
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_CP_CRASH

static void set_modem_state(struct mem_link_device *mld, enum modem_state state)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	/* Change the modem state to STATE_CRASH_EXIT for the FMT IO device */
	spin_lock_irqsave(&mc->lock, flags);
	if (mc->iod)
		mc->iod->modem_state_changed(mc->iod, state);
	spin_unlock_irqrestore(&mc->lock, flags);

	/* time margin for taking state changes by rild */
	if (mc->iod)
		mdelay(100);

	/* Change the modem state to STATE_CRASH_EXIT for the BOOT IO device */
	spin_lock_irqsave(&mc->lock, flags);
	if (mc->bootd)
		mc->bootd->modem_state_changed(mc->bootd, state);
	spin_unlock_irqrestore(&mc->lock, flags);
}

void mem_handle_cp_crash(struct mem_link_device *mld, enum modem_state state)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

#ifdef CONFIG_LINK_POWER_MANAGEMENT
	if (mld->stop_pm)
		mld->stop_pm(mld);
#endif

	/* Disable normal IPC */
	set_magic(mld, MEM_CRASH_MAGIC);
	set_access(mld, 0);

	stop_tx(mld);
	purge_txq(mld);

	if (cp_online(mc))
		set_modem_state(mld, state);

	atomic_set(&mc->forced_cp_crash, 0);
}

static void handle_no_cp_crash_ack(unsigned long arg)
{
	struct mem_link_device *mld = (struct mem_link_device *)arg;
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;

	if (cp_crashed(mc)) {
		mif_debug("%s: STATE_CRASH_EXIT without CRASH_ACK\n",
			ld->name);
	} else {
		mif_err("%s: ERR! No CRASH_ACK from CP\n", ld->name);
		mem_handle_cp_crash(mld, STATE_CRASH_EXIT);
	}
}

void mem_forced_cp_crash(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	bool duplicated = false;
	unsigned long flags;

	/* Disable normal IPC */
	set_magic(mld, MEM_CRASH_MAGIC);
	set_access(mld, 0);

	spin_lock_irqsave(&mld->lock, flags);
	if (atomic_read(&mc->forced_cp_crash))
		duplicated = true;
	else
		atomic_set(&mc->forced_cp_crash, 1);
	spin_unlock_irqrestore(&mld->lock, flags);

	if (duplicated) {
		mif_err("%s: ALREADY in progress <%pf>\n",
			ld->name, CALLER);
		return;
	}

	if (!cp_online(mc)) {
		mif_err("%s: %s.state %s != ONLINE <%pf>\n",
			ld->name, mc->name, mc_state(mc), CALLER);
		return;
	}

	if (mc->wake_lock) {
		if (!wake_lock_active(mc->wake_lock)) {
			wake_lock(mc->wake_lock);
			mif_err("%s->wake_lock locked\n", mc->name);
		}
	}

	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP)) {
		stop_net_ifaces(ld);

		if (mld->debug_info)
			mld->debug_info();

		/**
		 * If there is no CRASH_ACK from CP in a timeout,
		 * handle_no_cp_crash_ack() will be executed.
		 */
		mif_add_timer(&mc->crash_ack_timer, FORCE_CRASH_ACK_TIMEOUT,
			      handle_no_cp_crash_ack, (unsigned long)mld);

		/* Send CRASH_EXIT command to a CP */
		send_ipc_irq(mld, cmd2int(CMD_CRASH_EXIT));
	} else {
		modemctl_notify_event(MDM_EVENT_CP_FORCE_CRASH);
	}

	mif_err("%s->%s: CP_CRASH_REQ <%pf>\n", ld->name, mc->name, CALLER);

#ifdef DEBUG_MODEM_IF
	if (in_interrupt())
		queue_work(system_nrt_wq, &mld->dump_work);
	else
		save_mem_dump(mld);
#endif
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_LINK_COMMAND

static inline void reset_ipc_map(struct mem_link_device *mld)
{
	int i;

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];

		set_txq_head(dev, 0);
		set_txq_tail(dev, 0);
		set_rxq_head(dev, 0);
		set_rxq_tail(dev, 0);
	}
}

int mem_reset_ipc_link(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int magic;
	unsigned int access;
	int i;

	set_access(mld, 0);
	set_magic(mld, 0);

	reset_ipc_map(mld);

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		struct mem_ipc_device *dev = mld->dev[i];

		skb_queue_purge(dev->skb_txq);
		atomic_set(&dev->txq.busy, 0);
		dev->req_ack_cnt[TX] = 0;

		skb_queue_purge(dev->skb_rxq);
		atomic_set(&dev->rxq.busy, 0);
		dev->req_ack_cnt[RX] = 0;
	}

	atomic_set(&ld->netif_stopped, 0);

	set_magic(mld, MEM_IPC_MAGIC);
	set_access(mld, 1);

	magic = get_magic(mld);
	access = get_access(mld);
	if (magic != MEM_IPC_MAGIC || access != 1)
		return -EACCES;

	return 0;
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_LINK_METHOD

static int mem_init_comm(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	struct io_device *check_iod;
	int id = iod->id;
	int fmt2rfs = (SIPC5_CH_ID_RFS_0 - SIPC5_CH_ID_FMT_0);
	int rfs2fmt = (SIPC5_CH_ID_FMT_0 - SIPC5_CH_ID_RFS_0);

	if (atomic_read(&mld->cp_boot_done))
		return 0;

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	if (mld->iosm) {
		struct sbd_link_device *sl = &mld->sbd_link_dev;
		struct sbd_ipc_device *sid = sbd_ch2dev(sl, iod->id);

		if (atomic_read(&sid->config_done)) {
			tx_iosm_message(mld, IOSM_A2C_OPEN_CH, (u32 *)&id);
			return 0;
		} else {
			mif_err("%s isn't configured channel\n", iod->name);
			return -ENODEV;
		}
	}
#endif

	switch (id) {
	case SIPC5_CH_ID_FMT_0 ... SIPC5_CH_ID_FMT_9:
		check_iod = link_get_iod_with_channel(ld, (id + fmt2rfs));
		if (check_iod ? atomic_read(&check_iod->opened) : true) {
			mif_err("%s: %s->INIT_END->%s\n",
				ld->name, iod->name, mc->name);
			send_ipc_irq(mld, cmd2int(CMD_INIT_END));
			atomic_set(&mld->cp_boot_done, 1);
		} else {
			mif_err("%s is not opened yet\n", check_iod->name);
		}
		break;

	case SIPC5_CH_ID_RFS_0 ... SIPC5_CH_ID_RFS_9:
		check_iod = link_get_iod_with_channel(ld, (id + rfs2fmt));
		if (check_iod) {
			if (atomic_read(&check_iod->opened)) {
				mif_err("%s: %s->INIT_END->%s\n",
					ld->name, iod->name, mc->name);
				send_ipc_irq(mld, cmd2int(CMD_INIT_END));
				atomic_set(&mld->cp_boot_done, 1);
			} else {
				mif_err("%s not opened yet\n", check_iod->name);
			}
		}
		break;

	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH

static void mem_terminate_comm(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (mld->iosm)
		tx_iosm_message(mld, IOSM_A2C_CLOSE_CH, (u32 *)&iod->id);
}
#endif

static int mem_send(struct link_device *ld, struct io_device *iod,
		    struct sk_buff *skb)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	enum dev_format id = iod->format;
	u8 ch = iod->id;

	switch (id) {
	case IPC_FMT:
	case IPC_RAW:
	case IPC_RFS:
		if (likely(sipc5_ipc_ch(ch)))
			return xmit_ipc(mld, iod, ch, skb);
		else
			return xmit_udl(mld, iod, ch, skb);

	case IPC_BOOT:
	case IPC_DUMP:
		if (sipc5_udl_ch(ch))
			return xmit_udl(mld, iod, ch, skb);
		break;

	default:
		break;
	}

	mif_err("%s:%s->%s: ERR! Invalid IO device (format:%s id:%d)\n",
		ld->name, iod->name, mc->name, dev_str(id), ch);

	return -ENODEV;
}

static void mem_boot_on(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	unsigned long flags;

	atomic_set(&mld->cp_boot_done, 0);

	spin_lock_irqsave(&ld->lock, flags);
	ld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&ld->lock, flags);

	cancel_tx_timer(mld, &mld->tx_timer);

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
#ifdef CONFIG_LTE_MODEM_XMM7260
	sbd_deactivate(&mld->sbd_link_dev);
#endif
	cancel_tx_timer(mld, &mld->sbd_tx_timer);

	if (mld->iosm) {
		memset(mld->base + CMD_RGN_OFFSET, 0, CMD_RGN_SIZE);
		mif_info("Control message region has been initialized\n");
	}
#endif

	purge_txq(mld);
}

static int mem_xmit_boot(struct link_device *ld, struct io_device *iod,
		     unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	void __iomem *dst;
	void __user *src;
	int err;
	struct modem_firmware mf;
	void __iomem *v_base;
	unsigned valid_space;

	/**
	 * Get the information about the boot image
	 */
	memset(&mf, 0, sizeof(struct modem_firmware));

	err = copy_from_user(&mf, (const void __user *)arg, sizeof(mf));
	if (err) {
		mif_err("%s: ERR! INFO copy_from_user fail\n", ld->name);
		return -EFAULT;
	}

	/* Calculate size of valid space which BL will download */
	valid_space = (mf.mode) ?
		(mld->shm_size - mld->boot_size - mld->size) : mld->boot_size;
	/* Calculate base address (0: BOOT_MODE, 1: DUMP_MODE) */
	v_base = (mf.mode) ? (mld->base + mld->size) : mld->boot_base;

	/**
	 * Check the size of the boot image
	 * fix the integer overflow of "mf.m_offset + mf.len" from Jose Duart
	 */
	if (mf.size > valid_space || mf.len > valid_space
			|| mf.m_offset > valid_space - mf.len) {
		mif_err("%s: ERR! Invalid args: size %x, offset %x, len %x\n",
			ld->name, mf.size, mf.m_offset, mf.len);
		return -EINVAL;
	}

	dst = (void __iomem *)(v_base + mf.m_offset);
	src = (void __user *)mf.binary;
	err = copy_from_user(dst, src, mf.len);
	if (err) {
		mif_err("%s: ERR! BOOT copy_from_user fail\n", ld->name);
		return err;
	}

	return 0;
}

static int mem_security_request(struct link_device *ld, struct io_device *iod,
				unsigned long arg)
{
	unsigned long size, addr;
	int err = 0;
	struct modem_sec_req msr;

	err = copy_from_user(&msr, (const void __user *)arg, sizeof(msr));
	if (err) {
		mif_err("%s: ERR! copy_from_user fail\n", ld->name);
		err = -EFAULT;
		goto exit;
	}

	size = shm_get_security_size(msr.mode, msr.size);
	addr = shm_get_security_addr(msr.mode);
	mif_err("mode=%lu, size=%lu, addr=%lu\n", msr.mode, size, addr);
	err = exynos_smc(SMC_ID, msr.mode, size, addr);
	mif_info("%s: return_value=%d\n", ld->name, err);

	/* To do: will be removed*/
	pmu_cp_reg_dump();

exit:
	return err;
}

static int mem_start_download(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	reset_ipc_map(mld);

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_BOOT))
		sbd_deactivate(&mld->sbd_link_dev);
#endif

	if (mld->attrs & LINK_ATTR(LINK_ATTR_BOOT_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	if (mld->dpram_magic) {
		unsigned int magic;

		set_magic(mld, MEM_BOOT_MAGIC);
		magic = get_magic(mld);
		if (magic != MEM_BOOT_MAGIC) {
			mif_err("%s: ERR! magic 0x%08X != BOOT_MAGIC 0x%08X\n",
				ld->name, magic, MEM_BOOT_MAGIC);
			return -EFAULT;
		}
		mif_err("%s: magic == 0x%08X\n", ld->name, magic);
	}

	return 0;
}

static int mem_update_firm_info(struct link_device *ld, struct io_device *iod,
				unsigned long arg)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret;

	ret = copy_from_user(&mld->img_info, (void __user *)arg,
			     sizeof(struct std_dload_info));
	if (ret) {
		mif_err("ERR! copy_from_user fail!\n");
		return -EFAULT;
	}

	return 0;
}

static int mem_force_dump(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	mif_err("+++\n");
	mem_forced_cp_crash(mld);
	mif_err("---\n");
	return 0;
}

static int mem_start_upload(struct link_device *ld, struct io_device *iod)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP))
		sbd_deactivate(&mld->sbd_link_dev);
#endif

	reset_ipc_map(mld);

	if (mld->attrs & LINK_ATTR(LINK_ATTR_DUMP_ALIGNED))
		ld->aligned = true;
	else
		ld->aligned = false;

	if (mld->dpram_magic) {
		unsigned int magic;

		set_magic(mld, MEM_DUMP_MAGIC);
		magic = get_magic(mld);
		if (magic != MEM_DUMP_MAGIC) {
			mif_err("%s: ERR! magic 0x%08X != DUMP_MAGIC 0x%08X\n",
				ld->name, magic, MEM_DUMP_MAGIC);
			return -EFAULT;
		}
		mif_err("%s: magic == 0x%08X\n", ld->name, magic);
	}

	return 0;
}

static void mem_close_tx(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct modem_ctl *mc = ld->mc;
	unsigned long flags;

	spin_lock_irqsave(&ld->lock, flags);
	ld->state = LINK_STATE_OFFLINE;
	spin_unlock_irqrestore(&ld->lock, flags);

	if (timer_pending(&mc->crash_ack_timer))
		del_timer(&mc->crash_ack_timer);

	stop_tx(mld);
	purge_txq(mld);
}

#endif

/*============================================================================*/

#ifdef GROUP_MEM_LINK_SETUP

void __iomem *mem_vmap(phys_addr_t pa, size_t size, struct page *pages[])
{
	unsigned int num_pages = (size >> PAGE_SHIFT);
	pgprot_t prot = pgprot_noncached(PAGE_KERNEL);
	int i;

	for (i = 0; i < num_pages; i++) {
		pages[i] = phys_to_page(pa);
		pa += PAGE_SIZE;
	}

	return vmap(pages, num_pages, VM_MAP, prot);
}

void mem_vunmap(void *va)
{
	vunmap(va);
}

int mem_register_boot_rgn(struct mem_link_device *mld, phys_addr_t start,
			  size_t size)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int num_pages = (size >> PAGE_SHIFT);
	struct page **pages;

	pages = kmalloc(sizeof(struct page *) * num_pages, GFP_ATOMIC);
	if (!pages)
		return -ENOMEM;

	mif_err("%s: BOOT_RGN start:%pK size:%zu\n", ld->name, &start, size);

	mld->boot_start = start;
	mld->boot_size = size;
	mld->boot_pages = pages;

	return 0;
}

void mem_unregister_boot_rgn(struct mem_link_device *mld)
{
	kfree(mld->boot_pages);
	mld->boot_pages = NULL;
	mld->boot_size = 0;
	mld->boot_start = 0;
}

int mem_setup_boot_map(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	phys_addr_t start = mld->boot_start;
	size_t size = mld->boot_size;
	struct page **pages = mld->boot_pages;
	char __iomem *base;

	base = mem_vmap(start, size, pages);
	if (!base) {
		mif_err("%s: ERR! mem_vmap fail\n", ld->name);
		return -EINVAL;
	}
	memset(base, 0, size);

	mld->boot_base = (char __iomem *)base;

	mif_err("%s: BOOT_RGN phys_addr:%pK virt_addr:%pK size:%zu\n",
		ld->name, &start, base, size);

	return 0;
}

static void remap_4mb_map_to_ipc_dev(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;
	struct shmem_4mb_phys_map *map;
	struct mem_ipc_device *dev;

	map = (struct shmem_4mb_phys_map *)mld->base;

	/* magic code and access enable fields */
	mld->magic = (u32 __iomem *)&map->magic;
	mld->access = (u32 __iomem *)&map->access;

	/* IPC_MAP_FMT */
	dev = &mld->ipc_dev[IPC_MAP_FMT];

	dev->id = IPC_MAP_FMT;
	strcpy(dev->name, "FMT");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->fmt_tx_head;
	dev->txq.tail = &map->fmt_tx_tail;
	dev->txq.buff = &map->fmt_tx_buff[0];
	dev->txq.size = SHM_4M_FMT_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->fmt_rx_head;
	dev->rxq.tail = &map->fmt_rx_tail;
	dev->rxq.buff = &map->fmt_rx_buff[0];
	dev->rxq.size = SHM_4M_FMT_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_FMT;
	dev->req_ack_mask = MASK_REQ_ACK_FMT;
	dev->res_ack_mask = MASK_RES_ACK_FMT;

	dev->tx_lock = &ld->tx_lock[IPC_MAP_FMT];
	dev->skb_txq = &ld->sk_fmt_tx_q;

	dev->rx_lock = &ld->rx_lock[IPC_MAP_FMT];
	dev->skb_rxq = &ld->sk_fmt_rx_q;

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_MAP_FMT] = dev;

	/* IPC_MAP_NORM_RAW */
	dev = &mld->ipc_dev[IPC_MAP_NORM_RAW];

	dev->id = IPC_MAP_NORM_RAW;
	strcpy(dev->name, "NORM_RAW");

	spin_lock_init(&dev->txq.lock);
	atomic_set(&dev->txq.busy, 0);
	dev->txq.head = &map->raw_tx_head;
	dev->txq.tail = &map->raw_tx_tail;
	dev->txq.buff = &map->raw_tx_buff[0];
	dev->txq.size = SHM_4M_RAW_TX_BUFF_SZ;

	spin_lock_init(&dev->rxq.lock);
	atomic_set(&dev->rxq.busy, 0);
	dev->rxq.head = &map->raw_rx_head;
	dev->rxq.tail = &map->raw_rx_tail;
	dev->rxq.buff = &map->raw_rx_buff[0];
	dev->rxq.size = SHM_4M_RAW_RX_BUFF_SZ;

	dev->msg_mask = MASK_SEND_RAW;
	dev->req_ack_mask = MASK_REQ_ACK_RAW;
	dev->res_ack_mask = MASK_RES_ACK_RAW;

	dev->tx_lock = &ld->tx_lock[IPC_MAP_NORM_RAW];
	dev->skb_txq = &ld->sk_raw_tx_q;

	dev->rx_lock = &ld->rx_lock[IPC_MAP_NORM_RAW];
	dev->skb_rxq = &ld->sk_raw_rx_q;

	dev->req_ack_cnt[TX] = 0;
	dev->req_ack_cnt[RX] = 0;

	mld->dev[IPC_MAP_NORM_RAW] = dev;
}

int mem_register_ipc_rgn(struct mem_link_device *mld, phys_addr_t start,
			 size_t size)
{
	struct link_device *ld = &mld->link_dev;
	unsigned int num_pages = (size >> PAGE_SHIFT);
	struct page **pages;

	pages = kmalloc(sizeof(struct page *) * num_pages, GFP_ATOMIC);
	if (!pages)
		return -ENOMEM;

	mif_err("%s: IPC_RGN start:%pK size:%zu\n", ld->name, &start, size);

	mld->start = start;
	mld->size = size;
	mld->pages = pages;

	return 0;
}

void mem_unregister_ipc_rgn(struct mem_link_device *mld)
{
	kfree(mld->pages);
	mld->pages = NULL;
	mld->size = 0;
	mld->start = 0;
}

void mem_setup_ipc_map(struct mem_link_device *mld)
{
	remap_4mb_map_to_ipc_dev(mld);
}

static int mem_rx_setup(struct link_device *ld)
{
	struct mem_link_device *mld = to_mem_link_device(ld);

	if (!zalloc_cpumask_var(&mld->dmask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mld->imask, GFP_KERNEL))
		return -ENOMEM;
	if (!zalloc_cpumask_var(&mld->tmask, GFP_KERNEL))
		return -ENOMEM;

#ifdef CONFIG_ARGOS
	/* Below hard-coded mask values should be removed later on.
	 * Like net-sysfs, argos module also should support sysfs knob,
	 * so that user layer must be able to control these cpu mask. */
#ifdef CONFIG_SCHED_HMP
	cpumask_copy(mld->dmask, &hmp_slow_cpu_mask);
#endif

	cpumask_or(mld->imask, mld->imask, cpumask_of(3));

	argos_irq_affinity_setup_label(217, "IPC", mld->imask, mld->dmask);
#endif

	ld->tx_wq = create_singlethread_workqueue("mem_tx_work");
	if (!ld->tx_wq) {
		mif_err("%s: ERR! fail to create tx_wq\n", ld->name);
		return -ENOMEM;
	}

	ld->rx_wq = alloc_workqueue(
			"mem_rx_work", WQ_HIGHPRI | WQ_CPU_INTENSIVE, 1);
	if (!ld->rx_wq) {
		mif_err("%s: ERR! fail to create rx_wq\n", ld->name);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&ld->rx_delayed_work, link_to_demux_work);

	return 0;
}

struct mem_link_device *mem_create_link_device(enum mem_iface_type type,
					       struct modem_data *modem)
{
	struct mem_link_device *mld;
	struct link_device *ld;
	int i;
	mif_err("+++\n");

	if (modem->ipc_version < SIPC_VER_50) {
		mif_err("%s<->%s: ERR! IPC version %d < SIPC_VER_50\n",
			modem->link_name, modem->name, modem->ipc_version);
		return NULL;
	}

	/*
	** Alloc an instance of mem_link_device structure
	*/
	mld = kzalloc(sizeof(struct mem_link_device), GFP_KERNEL);
	if (!mld) {
		mif_err("%s<->%s: ERR! mld kzalloc fail\n",
			modem->link_name, modem->name);
		return NULL;
	}

	/*
	** Retrieve modem-specific attributes value
	*/
	mld->type = type;
	mld->attrs = modem->link_attrs;

	/*====================================================================*\
		Initialize "memory snapshot buffer (MSB)" framework
	\*====================================================================*/
	if (msb_init() < 0) {
		mif_err("%s<->%s: ERR! msb_init() fail\n",
			modem->link_name, modem->name);
		goto error;
	}

	/*====================================================================*\
		Set attributes as a "link_device"
	\*====================================================================*/
	ld = &mld->link_dev;

	ld->name = modem->link_name;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_SBD_IPC)) {
		mif_err("%s<->%s: LINK_ATTR_SBD_IPC\n", ld->name, modem->name);
		ld->sbd_ipc = true;
	}

	if (mld->attrs & LINK_ATTR(LINK_ATTR_IPC_ALIGNED)) {
		mif_err("%s<->%s: LINK_ATTR_IPC_ALIGNED\n",
			ld->name, modem->name);
		ld->aligned = true;
	}

	ld->ipc_version = modem->ipc_version;

	ld->mdm_data = modem;

	/*
	Set up link device methods
	*/
	ld->init_comm = mem_init_comm;
#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	ld->terminate_comm = mem_terminate_comm;
#endif
	ld->send = mem_send;

	ld->boot_on = mem_boot_on;
	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_BOOT)) {
		if (mld->attrs & LINK_ATTR(LINK_ATTR_XMIT_BTDLR))
			ld->xmit_boot = mem_xmit_boot;
		ld->dload_start = mem_start_download;
		ld->firm_update = mem_update_firm_info;
		ld->security_req = mem_security_request;
	}

	ld->force_dump = mem_force_dump;

	if (mld->attrs & LINK_ATTR(LINK_ATTR_MEM_DUMP)) {
		ld->dump_start = mem_start_upload;
	}

	ld->close_tx = mem_close_tx;

	INIT_LIST_HEAD(&ld->list);

	skb_queue_head_init(&ld->sk_fmt_tx_q);
	skb_queue_head_init(&ld->sk_raw_tx_q);

	skb_queue_head_init(&ld->sk_fmt_rx_q);
	skb_queue_head_init(&ld->sk_raw_rx_q);

	for (i = 0; i < MAX_SIPC_MAP; i++) {
		spin_lock_init(&ld->tx_lock[i]);
		spin_lock_init(&ld->rx_lock[i]);
	}

	spin_lock_init(&ld->netif_lock);
	atomic_set(&ld->netif_stopped, 0);

	if (mem_rx_setup(ld) < 0)
		goto error;

	/*====================================================================*\
		Set attributes as a "memory link_device"
	\*====================================================================*/
	if (mld->attrs & LINK_ATTR(LINK_ATTR_DPRAM_MAGIC)) {
		mif_err("%s<->%s: LINK_ATTR_DPRAM_MAGIC\n",
			ld->name, modem->name);
		mld->dpram_magic = true;
	}

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	if (mld->attrs & LINK_ATTR(LINK_ATTR_IOSM_MESSAGE)) {
		mif_err("%s<->%s: MODEM_ATTR_IOSM_MESSAGE\n",
			ld->name, modem->name);
		mld->iosm = true;
		mld->cmd_handler = iosm_event_bh;
	} else {
		mld->cmd_handler = mem_cmd_handler;
	}
#else
	mld->cmd_handler = mem_cmd_handler;
#endif

	/*====================================================================*\
		Initialize MEM locks, completions, bottom halves, etc
	\*====================================================================*/
	spin_lock_init(&mld->lock);

	/*
	** Initialize variables for TX & RX
	*/
	msb_queue_head_init(&mld->msb_rxq);
	msb_queue_head_init(&mld->msb_log);

	tasklet_init(&mld->rx_tsk, mem_rx_task, (unsigned long)mld);

	hrtimer_init(&mld->tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mld->tx_timer.function = tx_timer_func;

#ifdef CONFIG_LINK_DEVICE_WITH_SBD_ARCH
	hrtimer_init(&mld->sbd_tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mld->sbd_tx_timer.function = sbd_tx_timer_func;

	INIT_WORK(&mld->iosm_w, iosm_event_work);
#endif

	/*
	** Initialize variables for CP booting and crash dump
	*/
	INIT_DELAYED_WORK(&mld->udl_rx_dwork, udl_rx_work);

#ifdef DEBUG_MODEM_IF
	INIT_WORK(&mld->dump_work, mem_dump_work);
#endif

	mif_err("---\n");
	return mld;

error:
	kfree(mld);
	mif_err("xxx\n");
	return NULL;
}

#endif
