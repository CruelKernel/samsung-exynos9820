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

#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/vmalloc.h>
#include <linux/netdevice.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"

#ifdef GROUP_MEM_FLOW_CONTROL

void sbd_txq_stop(struct sbd_ring_buffer *rb)
{
	if (sipc_ps_ch(rb->ch) && atomic_read(&rb->busy) == 0) {
		struct link_device *ld = rb->ld;

		if (!test_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask)) {
			unsigned long flags;

			spin_lock_irqsave(&rb->lock, flags);

			atomic_set(&rb->busy, 1);
			set_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask);
			stop_net_ifaces(ld);

			spin_unlock_irqrestore(&rb->lock, flags);

			/* Currently,
			 * CP is not doing anything when CP receive req_ack
			 * command from AP. So, we'll skip this scheme.
			 */
			/* send_req_ack(mld, dev); */
			mif_err_limited("%s, tx_flowctrl=0x%04lx\n",
				rb->iod->name, ld->tx_flowctrl_mask);
		}
	}
}

void sbd_txq_start(struct sbd_ring_buffer *rb)
{
	if (sipc_ps_ch(rb->ch) && atomic_read(&rb->busy) > 0) {
		struct link_device *ld = rb->ld;

		if (test_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask)) {
			unsigned long flags;

			spin_lock_irqsave(&rb->lock, flags);

			atomic_set(&rb->busy, 0);
			clear_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask);

			if (ld->tx_flowctrl_mask == 0) {
				resume_net_ifaces(ld);
				mif_err_limited("%s, tx_flowctrl=0x%04lx\n",
					rb->iod->name, ld->tx_flowctrl_mask);
			}

			spin_unlock_irqrestore(&rb->lock, flags);
		}
	}
}

int sbd_under_tx_flow_ctrl(struct sbd_ring_buffer *rb)
{
	return atomic_read(&rb->busy);
}

int sbd_check_tx_flow_ctrl(struct sbd_ring_buffer *rb)
{
	struct link_device *ld = rb->ld;
	struct modem_ctl *mc = ld->mc;
	int busy_count = atomic_read(&rb->busy);

	if (rb_empty(rb)) {
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
		if (cp_online(mc)) {
			mif_err("%s TXQ: No RES_ACK, but EMPTY (busy_cnt %d)\n",
				rb->iod->name, busy_count);
		}
#endif
		sbd_txq_start(rb);
		return 0;
	}

	atomic_inc(&rb->busy);

	if (cp_online(mc) && count_flood(busy_count, BUSY_COUNT_MASK)) {
		/* Currently,
		 * CP is not doing anything when CP receive req_ack
		 * command from AP. So, we'll skip this scheme.
		 */
		 /* send_req_ack(mld, dev); */
		return -ETIME;
	}

	return -EBUSY;
}

void txq_stop(struct mem_link_device *mld, struct mem_ipc_device *dev)
{
#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	if (dev->id == IPC_MAP_HPRIO_RAW && atomic_read(&dev->txq.busy) == 0) {
#else
	if (dev->id == IPC_MAP_NORM_RAW && atomic_read(&dev->txq.busy) == 0) {
#endif
		struct link_device *ld = &mld->link_dev;
		if (!test_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask)) {
			unsigned long flags;

			spin_lock_irqsave(&dev->txq.lock, flags);

			atomic_set(&dev->txq.busy, 1);
			set_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask);
			stop_net_ifaces(&mld->link_dev);

			spin_unlock_irqrestore(&dev->txq.lock, flags);

			send_req_ack(mld, dev);
			mif_err_limited("%s: %s TXQ BUSY, tx_flowctrl_mask=0x%04lx\n",
				ld->name, dev->name, ld->tx_flowctrl_mask);
		}
	}
}

void tx_flowctrl_suspend(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;

	if (!test_bit(TX_SUSPEND_MASK, &ld->tx_flowctrl_mask)) {
		unsigned long flags;
#ifdef CONFIG_MODEM_IF_LEGACY_QOS
		struct mem_ipc_device *dev = mld->dev[IPC_MAP_HPRIO_RAW];
#else
		struct mem_ipc_device *dev = mld->dev[IPC_MAP_NORM_RAW];
#endif

		spin_lock_irqsave(&dev->txq.lock, flags);

		set_bit(TX_SUSPEND_MASK, &ld->tx_flowctrl_mask);
		stop_net_ifaces(&mld->link_dev);

		spin_unlock_irqrestore(&dev->txq.lock, flags);

		mif_err_limited("%s: %s TX suspended, tx_flowctrl_mask=0x%04lx\n",
			ld->name, dev->name, ld->tx_flowctrl_mask);
	}
}

void txq_start(struct mem_link_device *mld, struct mem_ipc_device *dev)
{
#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	if (dev->id == IPC_MAP_HPRIO_RAW && atomic_read(&dev->txq.busy) > 0) {
#else
	if (dev->id == IPC_MAP_NORM_RAW && atomic_read(&dev->txq.busy) > 0) {
#endif
		struct link_device *ld = &mld->link_dev;
		if (test_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask)) {
			unsigned long flags;

			spin_lock_irqsave(&dev->txq.lock, flags);

			atomic_set(&dev->txq.busy, 0);
			clear_bit(TXQ_STOP_MASK, &ld->tx_flowctrl_mask);

			if (ld->tx_flowctrl_mask == 0) {
				resume_net_ifaces(&mld->link_dev);
				mif_err_limited("%s:%s TXQ restart, tx_flowctrl_mask=0x%04lx\n",
					ld->name, dev->name, ld->tx_flowctrl_mask);
			}

			spin_unlock_irqrestore(&dev->txq.lock, flags);
		}
	}
}

void tx_flowctrl_resume(struct mem_link_device *mld)
{
	struct link_device *ld = &mld->link_dev;

	if (test_bit(TX_SUSPEND_MASK, &ld->tx_flowctrl_mask)) {
		unsigned long flags;
#ifdef CONFIG_MODEM_IF_LEGACY_QOS
		struct mem_ipc_device *dev = mld->dev[IPC_MAP_HPRIO_RAW];
#else
		struct mem_ipc_device *dev = mld->dev[IPC_MAP_NORM_RAW];
#endif

		spin_lock_irqsave(&dev->txq.lock, flags);

		clear_bit(TX_SUSPEND_MASK, &ld->tx_flowctrl_mask);

		if (ld->tx_flowctrl_mask == 0) {
			resume_net_ifaces(&mld->link_dev);
			mif_err_limited("%s:%s TX resumed, tx_flowctrl_mask=0x%04lx\n",
				ld->name, dev->name, ld->tx_flowctrl_mask);
		}

		spin_unlock_irqrestore(&dev->txq.lock, flags);
	}
}

int under_tx_flow_ctrl(struct mem_link_device *mld, struct mem_ipc_device *dev)
{
	return atomic_read(&dev->txq.busy);
}

int check_tx_flow_ctrl(struct mem_link_device *mld, struct mem_ipc_device *dev)
{
	struct link_device *ld = &mld->link_dev;
	struct modem_ctl *mc = ld->mc;
	int busy_count = atomic_read(&dev->txq.busy);

	if (txq_empty(dev)) {
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
		if (cp_online(mc)) {
			mif_err("%s->%s: %s_TXQ: No RES_ACK, but EMPTY (busy_cnt %d)\n",
				ld->name, mc->name, dev->name, busy_count);
		}
#endif
		txq_start(mld, dev);
		return 0;
	}

	atomic_inc(&dev->txq.busy);

	if (cp_online(mc) && count_flood(busy_count, BUSY_COUNT_MASK)) {
		send_req_ack(mld, dev);
		return -ETIME;
	}

	return -EBUSY;
}

void send_req_ack(struct mem_link_device *mld, struct mem_ipc_device *dev)
{
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	struct mst_buff *msb;
#endif

	send_ipc_irq(mld, mask2int(req_ack_mask(dev)));
	dev->req_ack_cnt[TX] += 1;

#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	msb = mem_take_snapshot(mld, TX);
	if (!msb)
		return;
	print_req_ack(mld, &msb->snapshot, dev, TX);
	msb_free(msb);
#endif
}

void recv_res_ack(struct mem_link_device *mld, struct mem_ipc_device *dev,
		  struct mem_snapshot *mst)
{
	dev->req_ack_cnt[TX] -= 1;

	txq_start(mld, dev);

#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	print_res_ack(mld, mst, dev, RX);
#endif
}

void recv_req_ack(struct mem_link_device *mld, struct mem_ipc_device *dev,
		  struct mem_snapshot *mst)
{
	dev->req_ack_cnt[RX] += 1;

#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	print_req_ack(mld, mst, dev, RX);
#endif
}

void send_res_ack(struct mem_link_device *mld, struct mem_ipc_device *dev)
{
#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	struct mst_buff *msb;
#endif

	send_ipc_irq(mld, mask2int(res_ack_mask(dev)));
	dev->req_ack_cnt[RX] -= 1;

#ifdef DEBUG_MODEM_IF_FLOW_CTRL
	msb = mem_take_snapshot(mld, TX);
	if (!msb)
		return;
	print_res_ack(mld, &msb->snapshot, dev, TX);
	msb_free(msb);
#endif
}

#endif
