/*
 * Copyright (C) 2010 Samsung Electronics.
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
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/device.h>
#include <linux/module.h>

#ifdef CONFIG_MCPS
#include "../../mcps/mcps.h"
#endif

#include <trace/events/napi.h>
#include <net/ip.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/netdevice.h>
#include <uapi/linux/net_dropdump.h>

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
#include <linux/modem_notifier.h>
#endif

#ifdef CONFIG_LINK_FORWARD
#include <linux/linkforward.h>
#endif

#include "modem_prj.h"
#include "modem_utils.h"
#include "modem_klat.h"

int receive_first_ipc;

static u8 sipc5_build_config(struct io_device *iod, struct link_device *ld,
			     unsigned int count);

static void sipc5_build_header(struct io_device *iod, u8 *buff, u8 cfg,
		unsigned int tx_bytes, unsigned int remains);

static ssize_t show_waketime(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int msec;
	char *p = buf;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct io_device *iod = container_of(miscdev, struct io_device,
			miscdev);

	msec = jiffies_to_msecs(iod->waketime);

	p += sprintf(buf, "raw waketime : %ums\n", msec);

	return p - buf;
}

static ssize_t store_waketime(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long msec;
	int ret;
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct io_device *iod = container_of(miscdev, struct io_device,
			miscdev);

	if (!iod) {
		pr_err("mif: %s: INVALID IO device\n", miscdev->name);
		return -EINVAL;
	}

	ret = kstrtoul(buf, 10, &msec);
	if (ret)
		return count;

	if (!msec) {
		mif_info("%s: (%ld) is not valied, use previous value(%d)\n",
			iod->name, msec,
			jiffies_to_msecs(iod->mc->iod->waketime));
		return count;
	}

	iod->waketime = msecs_to_jiffies(msec);
#ifdef DEBUG_MODEM_IF
	mif_err("%s: waketime = %lu ms\n", iod->name, msec);
#endif

	if (iod->format == IPC_MULTI_RAW) {
		struct modem_shared *msd = iod->msd;
		unsigned int i;

		for (i = SIPC_CH_ID_PDP_0; i < SIPC_CH_ID_BT_DUN; i++) {
			iod = get_iod_with_channel(msd, i);
			if (iod) {
				iod->waketime = msecs_to_jiffies(msec);
#ifdef DEBUG_MODEM_IF
				mif_err("%s: waketime = %lu ms\n",
					iod->name, msec);
#endif
			}
		}
	}

	return count;
}

static struct device_attribute attr_waketime =
	__ATTR(waketime, S_IRUGO | S_IWUSR, show_waketime, store_waketime);

static ssize_t show_loopback(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct modem_shared *msd =
		container_of(miscdev, struct io_device, miscdev)->msd;
	unsigned char *ip = (unsigned char *)&msd->loopback_ipaddr;
	char *p = buf;

	p += sprintf(buf, "%u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);

	return p - buf;
}

static ssize_t store_loopback(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct modem_shared *msd =
		container_of(miscdev, struct io_device, miscdev)->msd;

	msd->loopback_ipaddr = ipv4str_to_be32(buf, count);

	return count;
}

static struct device_attribute attr_loopback =
	__ATTR(loopback, S_IRUGO | S_IWUSR, show_loopback, store_loopback);

static void iodev_showtxlink(struct io_device *iod, void *args)
{
	char **p = (char **)args;
	struct link_device *ld = get_current_link(iod);

	if (iod->io_typ == IODEV_NET && IS_CONNECTED(iod, ld))
		*p += sprintf(*p, "%s<->%s\n", iod->name, ld->name);
}

static ssize_t show_txlink(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct miscdevice *miscdev = dev_get_drvdata(dev);
	struct modem_shared *msd =
		container_of(miscdev, struct io_device, miscdev)->msd;
	char *p = buf;

	iodevs_for_each(msd, iodev_showtxlink, &p);

	return p - buf;
}

static ssize_t store_txlink(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	/* don't change without gpio dynamic switching */
	return -EINVAL;
}

static struct device_attribute attr_txlink =
	__ATTR(txlink, S_IRUGO | S_IWUSR, show_txlink, store_txlink);

static inline void iodev_lock_wlock(struct io_device *iod)
{
	wake_lock_timeout(&iod->wakelock,
		iod->waketime ?: msecs_to_jiffies(200));
}

static int queue_skb_to_iod(struct sk_buff *skb, struct io_device *iod)
{
	struct sk_buff_head *rxq = &iod->sk_rx_q;
	int len = skb->len;

	if (iod->attrs & IODEV_ATTR(ATTR_NO_CHECK_MAXQ))
		goto enqueue;

	if (rxq->qlen > MAX_IOD_RXQ_LEN) {
		mif_err_limited("%s: %s may be dead (rxq->qlen %d > %d)\n",
			iod->name, iod->app ? iod->app : "corresponding",
			rxq->qlen, MAX_IOD_RXQ_LEN);
		dev_kfree_skb_any(skb);
		goto exit;
	}

enqueue:
	mif_debug("%s: rxq->qlen = %d\n", iod->name, rxq->qlen);
	skb_queue_tail(rxq, skb);

exit:
#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
	if (iod->link_types == LINKTYPE(LINKDEV_PCIE)) {
		struct modem_ctl *mc = iod->mc;
		struct link_device *ld = get_current_link(iod);
		struct mem_link_device *mld = ld_to_mem_link_device(ld);

		if (atomic_read(&mc->pcie_pwron)) {
			mod_timer(&mld->cp_not_work, jiffies + mld->not_work_time * HZ);
		}
	}
#endif

	wake_up(&iod->wq);
	return len;
}

static int rx_drain(struct sk_buff *skb)
{
	dev_kfree_skb_any(skb);
	return 0;
}

static int rx_loopback(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod;
	struct link_device *ld = skbpriv(skb)->ld;
	int ret;

	ret = ld->send(ld, iod, skb);
	if (ret < 0) {
		mif_err("%s->%s: ERR! ld->send fail (err %d)\n",
			iod->name, ld->name, ret);
	}

	return ret;
}

static int gather_multi_frame(struct sipc5_link_header *hdr,
			      struct sk_buff *skb)
{
	struct multi_frame_control ctrl = hdr->ctrl;
	struct io_device *iod = skbpriv(skb)->iod;
	struct modem_ctl *mc = iod->mc;
	struct sk_buff_head *multi_q = &iod->sk_multi_q[ctrl.id];
	int len = skb->len;

#ifdef DEBUG_MODEM_IF
	/* If there has been no multiple frame with this ID, ... */
	if (skb_queue_empty(multi_q)) {
		struct sipc_fmt_hdr *fh = (struct sipc_fmt_hdr *)skb->data;
		mif_err("%s<-%s: start of multi-frame (ID:%d len:%d)\n",
			iod->name, mc->name, ctrl.id, fh->len);
	}
#endif
	skb_queue_tail(multi_q, skb);

	if (ctrl.more) {
		/* The last frame has not arrived yet. */
		mif_err("%s<-%s: recv multi-frame (ID:%d rcvd:%d)\n",
			iod->name, mc->name, ctrl.id, skb->len);
	} else {
		struct sk_buff_head *rxq = &iod->sk_rx_q;
		unsigned long flags;

		/* It is the last frame because the "more" bit is 0. */
		mif_err("%s<-%s: end of multi-frame (ID:%d rcvd:%d)\n",
			iod->name, mc->name, ctrl.id, skb->len);

		spin_lock_irqsave(&rxq->lock, flags);
		skb_queue_splice_tail_init(multi_q, rxq);
		spin_unlock_irqrestore(&rxq->lock, flags);

		wake_up(&iod->wq);
	}

	return len;
}

static inline int rx_frame_with_link_header(struct sk_buff *skb)
{
	struct sipc5_link_header *hdr;
	bool multi_frame = sipc5_multi_frame(skb->data);
	int hdr_len = sipc5_get_hdr_len(skb->data);

	/* Remove SIPC5 link header */
	hdr = (struct sipc5_link_header *)skb->data;
	skb_pull(skb, hdr_len);

	if (multi_frame)
		return gather_multi_frame(hdr, skb);
	else
		return queue_skb_to_iod(skb, skbpriv(skb)->iod);
}

static int rx_fmt_ipc(struct sk_buff *skb)
{
	if (skbpriv(skb)->lnk_hdr)
		return rx_frame_with_link_header(skb);
	else
		return queue_skb_to_iod(skb, skbpriv(skb)->iod);
}

static int rx_raw_misc(struct sk_buff *skb)
{
	struct io_device *iod = skbpriv(skb)->iod;

	if (skbpriv(skb)->lnk_hdr) {
		/* Remove the SIPC5 link header */
		skb_pull(skb, sipc5_get_hdr_len(skb->data));
	}

	return queue_skb_to_iod(skb, iod);
}

#ifdef CONFIG_MODEM_IF_NET_GRO
static int check_gro_support(struct sk_buff *skb)
{
	switch (skb->data[0] & 0xF0) {
	case 0x40:
		return (ip_hdr(skb)->protocol == IPPROTO_TCP);

	case 0x60:
		return (ipv6_hdr(skb)->nexthdr == IPPROTO_TCP);
	}
	return 0;
}
#else
static int check_gro_support(struct sk_buff *skb)
{
	return 0;
}
#endif

static int rx_multi_pdp(struct sk_buff *skb)
{
	struct link_device *ld = skbpriv(skb)->ld;
	struct io_device *iod = skbpriv(skb)->iod;
	struct net_device *ndev;
	struct iphdr *iphdr;
	int len = skb->len;
	int ret, l2forward = 0;

	ndev = iod->ndev;
	if (!ndev) {
		mif_info("%s: ERR! no iod->ndev\n", iod->name);
		return -ENODEV;
	}

	if (skbpriv(skb)->lnk_hdr) {
		/* Remove the SIPC5 link header */
		skb_pull(skb, sipc5_get_hdr_len(skb->data));
	}

	skb->dev = ndev;

	/* check the version of IP */
	iphdr = (struct iphdr *)skb->data;
	if (iphdr->version == IPv6)
		skb->protocol = htons(ETH_P_IPV6);
	else
		skb->protocol = htons(ETH_P_IP);

	if (iod->use_handover) {
		struct ethhdr *ehdr;
		const char source[ETH_ALEN] = SOURCE_MAC_ADDR;

		ehdr = (struct ethhdr *)skb_push(skb, sizeof(struct ethhdr));
		memcpy(ehdr->h_dest, ndev->dev_addr, ETH_ALEN);
		memcpy(ehdr->h_source, source, ETH_ALEN);
		ehdr->h_proto = skb->protocol;
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb_reset_mac_header(skb);
		skb_pull(skb, sizeof(struct ethhdr));
	}

#ifdef DEBUG_MODEM_IF_IP_DATA
	print_ipv4_packet(skb->data, RX);
#endif
#if defined(DEBUG_MODEM_IF_IODEV_RX) && defined(DEBUG_MODEM_IF_PS_DATA)
	mif_pkt(iod->id, "IOD-RX", skb);
#endif

	skb_reset_transport_header(skb);
	skb_reset_network_header(skb);
	skb_reset_mac_header(skb);

#ifdef CONFIG_LINK_FORWARD
	/* Link Forward */
#ifdef CONFIG_CP_DIT
	if (skbpriv(skb)->support_dit)
#endif
		l2forward = (get_linkforward_mode() & 0x1) ? linkforward_manip_skb(skb, LINK_FORWARD_DIR_REPLY) : 0;
#endif

	if (!l2forward) {
		/* klat */
		klat_rx(skb, skbpriv(skb)->sipc_ch - SIPC_CH_ID_PDP_0);

		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += skb->len;

#ifdef CONFIG_MCPS
		if(!mcps_try_gro(skb)) {
			return len;
		}
#endif
		if (check_gro_support(skb)) {
			ret = napi_gro_receive(napi_get_current(), skb);
			if (ret == GRO_DROP) {
				mif_err_limited("%s: %s<-%s: ERR! napi_gro_receive\n",
						ld->name, iod->name, iod->mc->name);
			}

			if (ld->gro_flush)
				ld->gro_flush(ld);
			return len;
		}
	} else {
		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += skb->len;
	}
#ifdef CONFIG_LINK_DEVICE_NAPI
	ret = netif_receive_skb(skb);
#else /* !CONFIG_LINK_DEVICE_NAPI */
	if (in_interrupt())
		ret = netif_rx(skb);
	else
		ret = netif_rx_ni(skb);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	if (ret != NET_RX_SUCCESS) {
		mif_err_limited("%s: %s<-%s: ERR! netif_rx\n",
				ld->name, iod->name, iod->mc->name);
	}
	return len;
}

static int rx_demux(struct link_device *ld, struct sk_buff *skb)
{
	struct io_device *iod;
	u8 ch = skbpriv(skb)->sipc_ch;

	if (unlikely(ch == 0)) {
		mif_err("%s: ERR! invalid ch# %d\n", ld->name, ch);
		return -ENODEV;
	}

	/* IP loopback */
	if (ch == DATA_LOOPBACK_CHANNEL && ld->msd->loopback_ipaddr)
		ch = SIPC_CH_ID_PDP_0;

	iod = link_get_iod_with_channel(ld, ch);
	if (unlikely(!iod)) {
		mif_err("%s: ERR! no iod with ch# %d\n", ld->name, ch);
		return -ENODEV;
	}

	/* Don't care whether or not DATA_DRAIN_CHANNEL is opened */
	if (iod->id == DATA_DRAIN_CHANNEL)
		return rx_drain(skb);

	/* Don't care whether or not CP2AP_LOOPBACK_CHANNEL is opened. */
	if (iod->id == CP2AP_LOOPBACK_CHANNEL)
		return rx_loopback(skb);

	if (atomic_read(&iod->opened) <= 0) {
		mif_err_limited("%s: ERR! %s is not opened\n",
				ld->name, iod->name);
		return -ENODEV;
	}

	if (sipc5_fmt_ch(ch)) {
		receive_first_ipc = 1;
		return rx_fmt_ipc(skb);
	} else if (sipc_ps_ch(ch))
		return rx_multi_pdp(skb);
	else
		return rx_raw_misc(skb);
}

static int io_dev_recv_skb_single_from_link_dev(struct io_device *iod,
						struct link_device *ld,
						struct sk_buff *skb)
{
	int err;

	iodev_lock_wlock(iod);

	if (skbpriv(skb)->lnk_hdr && ld->aligned) {
		/* Cut off the padding in the current SIPC5 frame */
		skb_trim(skb, sipc5_get_frame_len(skb->data));
	}

	err = rx_demux(ld, skb);
	if (err < 0) {
		mif_err_limited("%s<-%s: ERR! rx_demux fail (err %d)\n",
				iod->name, ld->name, err);
	}

	return err;
}

/*
@brief	called by a link device with the "recv_net_skb" method to upload each PS
	data packet to the network protocol stack
*/
static int io_dev_recv_net_skb_from_link_dev(struct io_device *iod,
					     struct link_device *ld,
					     struct sk_buff *skb)
{
#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
	if (iod->link_types == LINKTYPE(LINKDEV_PCIE)) {
		iod = get_static_rmnet_iod_with_channel(iod->id);
		skbpriv(skb)->iod = iod;
	}
#endif
	if (unlikely(atomic_read(&iod->opened) <= 0)) {
		struct modem_ctl *mc = iod->mc;
		mif_err_limited("%s: %s<-%s: ERR! %s is not opened\n",
				ld->name, iod->name, mc->name, iod->name);
		return -ENODEV;
	}

	iodev_lock_wlock(iod);

	return rx_multi_pdp(skb);
}

/* inform the IO device that the modem is now online or offline or
 * crashing or whatever...
 */
static void io_dev_modem_state_changed(struct io_device *iod,
				       enum modem_state state)
{
	struct modem_ctl *mc = iod->mc;
	enum modem_state old_state = mc->phone_state;

	if (state == old_state)
		goto exit;

	mc->phone_state = state;
	mif_err("%s->state changed (%s -> %s)\n", mc->name,
		cp_state_str(old_state), cp_state_str(state));

exit:
	if (state == STATE_CRASH_RESET
	    || state == STATE_CRASH_EXIT
	    || state == STATE_NV_REBUILDING
	    || state == STATE_CRASH_WATCHDOG)
		wake_up(&iod->wq);
}

static void io_dev_sim_state_changed(struct io_device *iod, bool sim_online)
{
	if (atomic_read(&iod->opened) == 0) {
		mif_info("%s: ERR! not opened\n", iod->name);
	} else if (iod->mc->sim_state.online == sim_online) {
		mif_info("%s: SIM state not changed\n", iod->name);
	} else {
		iod->mc->sim_state.online = sim_online;
		iod->mc->sim_state.changed = true;
		mif_info("%s: SIM state changed {online %d, changed %d}\n",
			iod->name, iod->mc->sim_state.online,
			iod->mc->sim_state.changed);
		wake_up(&iod->wq);
	}
}

static void iodev_dump_status(struct io_device *iod, void *args)
{
	if (iod->format == IPC_RAW && iod->io_typ == IODEV_NET) {
		struct link_device *ld = get_current_link(iod);
		mif_com_log(iod->mc->msd, "%s: %s\n", iod->name, ld->name);
	}
}

static int misc_open(struct inode *inode, struct file *filp)
{
	struct io_device *iod = to_io_device(filp->private_data);
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;
	int ret;

	filp->private_data = (void *)iod;

	atomic_inc(&iod->opened);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->init_comm) {
			ret = ld->init_comm(ld, iod);
			if (ret < 0) {
				mif_err("%s<->%s: ERR! init_comm fail(%d)\n",
					iod->name, ld->name, ret);
				atomic_dec(&iod->opened);
				return ret;
			}
		}
	}

	mif_err("%s (opened %d) by %s\n",
		iod->name, atomic_read(&iod->opened), current->comm);

	return 0;
}

static int misc_release(struct inode *inode, struct file *filp)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;
	int i;

	if (atomic_dec_and_test(&iod->opened)) {
		skb_queue_purge(&iod->sk_rx_q);

		/* purge multi_frame queue */
		for (i = 0; i < NUM_SIPC_MULTI_FRAME_IDS; i++)
			skb_queue_purge(&iod->sk_multi_q[i]);
	}

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->terminate_comm)
			ld->terminate_comm(ld, iod);
	}

	mif_err("%s (opened %d) by %s\n",
		iod->name, atomic_read(&iod->opened), current->comm);

	return 0;
}

static unsigned int misc_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct modem_ctl *mc;
	struct sk_buff_head *rxq;

	if (!iod)
		return POLLERR;

	mc = iod->mc;
	rxq = &iod->sk_rx_q;

	if (skb_queue_empty(rxq))
		poll_wait(filp, &iod->wq, wait);

	switch (mc->phone_state) {
	case STATE_BOOTING:
	case STATE_ONLINE:
		if (!mc->sim_state.changed) {
			if (!skb_queue_empty(rxq))
				return POLLIN | POLLRDNORM;
			else /* wq is waken up without rx, return for wait */
				return 0;
		}
		/* fall through, if sim_state has been changed */
	case STATE_CRASH_EXIT:
	case STATE_CRASH_RESET:
	case STATE_NV_REBUILDING:
	case STATE_CRASH_WATCHDOG:
		/* report crash only if iod is fmt/boot device */
		if (iod->format == IPC_FMT) {
			mif_err("%s: %s.state == %s\n", iod->name, mc->name,
				mc_state(mc));
			return POLLHUP;
		} else if (iod->format == IPC_BOOT || sipc5_boot_ch(iod->id)) {
			mif_err("%s: %s.state == %s\n", iod->name, mc->name,
				mc_state(mc));
			return POLLHUP;
		} else if (iod->format == IPC_DUMP || sipc5_dump_ch(iod->id)) {
			if (!skb_queue_empty(rxq))
				return POLLIN | POLLRDNORM;
			else
				return 0;
		} else {
			mif_err("%s: %s.state == %s\n", iod->name, mc->name,
				mc_state(mc));

			/* give delay to prevent infinite sys_poll call from
			 * select() in APP layer without 'sleep' user call takes
			 * almost 100% cpu usage when it is looked up by 'top'
			 * command.
			 */
			msleep(20);
		}
		break;

	case STATE_OFFLINE:
		if (iod->id == SIPC_CH_ID_CASS)
			return POLLHUP;
	default:
		break;
	}

	return 0;
}

static long misc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	struct modem_ctl *mc = iod->mc;
	enum modem_state p_state;
	unsigned long size;
	int tx_link;
	int ret;

	switch (cmd) {
	case IOCTL_MODEM_ON:
		if (mc->ops.modem_on) {
			mif_err("%s: IOCTL_MODEM_ON\n", iod->name);
#if defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
			if (ld->security_cp2cp_baaw_req) {
				ret = ld->security_cp2cp_baaw_req(ld, iod, arg);
				if (ret)
					mif_err("ERR ld->security_cp2cp_baaw_req():%d\n",
							ret);
			}
#endif
			ret = mc->ops.modem_on(mc);
			if (ret) {
				mif_err("mc->ops.modem_on() ERR:%d\n", ret);
				return ret;
			}
			return ret;
		}
		mif_err("%s: !mc->ops.modem_on\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_OFF:
		if (mc->ops.modem_off) {
			mif_err("%s: IOCTL_MODEM_OFF\n", iod->name);
			return mc->ops.modem_off(mc);
		}
		mif_err("%s: !mc->ops.modem_off\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_RESET:
		if (mc->ops.modem_reset) {
			mif_err("%s: IOCTL_MODEM_RESET\n", iod->name);
			ret = mc->ops.modem_reset(mc);
			if (ld->reset_zerocopy)
				ld->reset_zerocopy(ld);
			return ret;
		}
		mif_err("%s: !mc->ops.modem_reset\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_BOOT_ON:
		if (mc->ops.modem_boot_on) {
			mif_err("%s: IOCTL_MODEM_BOOT_ON\n", iod->name);
			return mc->ops.modem_boot_on(mc);
		}
		mif_err("%s: !mc->ops.modem_boot_on\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_BOOT_OFF:
		if (mc->ops.modem_boot_off) {
			mif_err("%s: IOCTL_MODEM_BOOT_OFF\n", iod->name);
			return mc->ops.modem_boot_off(mc);
		}
		mif_err("%s: !mc->ops.modem_boot_off\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_BOOT_DONE:
		mif_err("%s: IOCTL_MODEM_BOOT_DONE\n", iod->name);
		if (mc->ops.modem_boot_done)
			return mc->ops.modem_boot_done(mc);
		return 0;

	case IOCTL_MODEM_STATUS:
		mif_debug("%s: IOCTL_MODEM_STATUS\n", iod->name);

		p_state = mc->phone_state;

		if (p_state != STATE_ONLINE) {
			mif_debug("%s: IOCTL_MODEM_STATUS (state %s)\n",
				iod->name, cp_state_str(p_state));
		}

		if (mc->sim_state.changed) {
			enum modem_state s_state = mc->sim_state.online ?
					STATE_SIM_ATTACH : STATE_SIM_DETACH;
			mc->sim_state.changed = false;
			return s_state;
		}

		if (p_state == STATE_NV_REBUILDING)
			mc->phone_state = STATE_ONLINE;

		return p_state;

	case IOCTL_MODEM_XMIT_BOOT:
		if (ld->xmit_boot) {
			return ld->xmit_boot(ld, iod, arg);
		}
		mif_err("%s: !ld->xmit_boot\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DL_START:
		if (ld->dload_start) {
			int ret;
			mif_info("%s: IOCTL_MODEM_DL_START\n", iod->name);
			ret = ld->dload_start(ld, iod);
			mc->ops.modem_boot_confirm(mc);
			return ret;
		}
		mif_err("%s: !ld->dload_start\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_FW_UPDATE:
		if (ld->firm_update) {
			mif_info("%s: IOCTL_MODEM_FW_UPDATE\n", iod->name);
			return ld->firm_update(ld, iod, arg);
		}
		mif_err("%s: !ld->firm_update\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_FORCE_CRASH_EXIT:
		if (mc->ops.modem_force_crash_exit) {
			if (arg)
				ld->crash_type = arg;
			mif_err("%s: IOCTL_MODEM_FORCE_CRASH_EXIT (%d)\n",
				iod->name, ld->crash_type);
			return mc->ops.modem_force_crash_exit(mc);
		}
		mif_err("%s: !mc->ops.modem_force_crash_exit\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DUMP_START:
		if (mc->ops.modem_dump_start) {
			mif_err("%s: IOCTL_MODEM_DUMP_START\n", iod->name);
			return mc->ops.modem_dump_start(mc);
		} else if (ld->dump_start) {
			mif_err("%s: IOCTL_MODEM_DUMP_START\n", iod->name);
			return ld->dump_start(ld, iod);
		}
		mif_err("%s: !dump_start\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_RAMDUMP_START:
		if (ld->dump_start) {
			mif_info("%s: IOCTL_MODEM_RAMDUMP_START\n", iod->name);
			return ld->dump_start(ld, iod);
		}
		mif_err("%s: !ld->dump_start\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_DUMP_UPDATE:
		if (ld->dump_update) {
			mif_info("%s: IOCTL_MODEM_DUMP_UPDATE\n", iod->name);
			return ld->dump_update(ld, iod, arg);
		}
		mif_err("%s: !ld->dump_update\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_RAMDUMP_STOP:
		if (ld->dump_finish) {
			mif_info("%s: IOCTL_MODEM_RAMDUMP_STOP\n", iod->name);
			return ld->dump_finish(ld, iod, arg);
		}
		mif_err("%s: !ld->dump_finish\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_CP_UPLOAD:
	{
		char buff[CP_CRASH_INFO_SIZE];
		void __user *user_buff = (void __user *)arg;

		mif_err("%s: ERR! IOCTL_MODEM_CP_UPLOAD\n", iod->name);
		strcpy(buff, CP_CRASH_TAG);

		if (strncmp(cp_crash_info, "none", 4) || !arg) {
			sprintf(buff + strlen(CP_CRASH_TAG),
				"%s", cp_crash_info);
		} else {
			if (copy_from_user(
				(void *)((unsigned long)buff + strlen(CP_CRASH_TAG)),
				user_buff,
				CP_CRASH_INFO_SIZE - strlen(CP_CRASH_TAG)))
				return -EFAULT;
		}
#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
		if (check_cp_upload_cnt())
			panic("%s", buff);
		else {
			mif_info("Wait another IOCTL_MODEM_CP_UPLOAD\n");
			return 1;
		}
#else
		panic("%s", buff);
#endif
		return 0;
	}

	case IOCTL_MODEM_PROTOCOL_SUSPEND:
		mif_info("%s: IOCTL_MODEM_PROTOCOL_SUSPEND\n", iod->name);
		if (iod->format == IPC_MULTI_RAW) {
			iodevs_for_each(iod->msd, iodev_netif_stop, 0);
			return 0;
		}
		return -EINVAL;

	case IOCTL_MODEM_PROTOCOL_RESUME:
		mif_info("%s: IOCTL_MODEM_PROTOCOL_RESUME\n", iod->name);
		if (iod->format != IPC_MULTI_RAW) {
			iodevs_for_each(iod->msd, iodev_netif_wake, 0);
			return 0;
		}
		return -EINVAL;

	case IOCTL_MIF_LOG_DUMP:
	{
		void __user *user_buff = (void __user *)arg;

		iodevs_for_each(iod->msd, iodev_dump_status, 0);
		size = MAX_MIF_BUFF_SIZE;
		if (copy_to_user(user_buff, &size, sizeof(unsigned long)))
			return -EFAULT;
		mif_dump_log(mc->msd, iod);
		return 0;
	}

	case IOCTL_SHMEM_FULL_DUMP:
		mif_info("%s: IOCTL_SHMEM_FULL_DUMP\n", iod->name);
		if (ld->shmem_dump)
			return ld->shmem_dump(ld, iod, arg);
		else
			return -EINVAL;

	case IOCTL_DATABUF_FULL_DUMP:
		mif_info("%s: IOCTL_DATABUF_FULL_DUMP\n", iod->name);
		if (ld->databuf_dump)
			return ld->databuf_dump(ld, iod, arg);
		else
			return -EINVAL;

	case IOCTL_VSS_FULL_DUMP:
		mif_info("%s: IOCTL_VSS_FULL_DUMP\n", iod->name);
		if (ld->vss_dump)
			return ld->vss_dump(ld, iod, arg);
		else
			return -EINVAL;

	case IOCTL_ACPM_FULL_DUMP:
		mif_info("%s: IOCTL_ACPM_FULL_DUMP\n", iod->name);
		if (ld->acpm_dump)
			return ld->acpm_dump(ld, iod, arg);
		else
			return -EINVAL;

	case IOCTL_CPLOG_FULL_DUMP:
		mif_info("%s: IOCTL_CPLOG_FULL_DUMP\n", iod->name);
		if (ld->cplog_dump)
			return ld->cplog_dump(ld, iod, arg);
		else
			return -EINVAL;

	case IOCTL_MODEM_SET_TX_LINK:
		mif_info("%s: IOCTL_MODEM_SET_TX_LINK\n", iod->name);
		if (copy_from_user(&tx_link, (void __user *)arg, sizeof(int)))
			return -EFAULT;

		mif_info("cur link: %d, new link: %d\n",
				ld->link_type, tx_link);

		if (ld->link_type != tx_link) {
			mif_info("change link: %d -> %d\n",
				ld->link_type, tx_link);
			ld = find_linkdev(iod->msd, tx_link);
			if (!ld) {
				mif_err("find_linkdev(%d) fail\n", tx_link);
				return -ENODEV;
			}

			set_current_link(iod, ld);

			ld = get_current_link(iod);
			mif_info("%s tx_link change success\n",	ld->name);
		}
		return 0;

	case IOCTL_SECURITY_REQ:
		if (ld->security_req) {
			mif_info("%s: IOCTL_SECURITY_REQUEST\n", iod->name);
			return ld->security_req(ld, iod, arg);
		}
		mif_err("%s: !ld->check_security\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_CRASH_REASON:
		if (ld->crash_reason) {
			mif_info("%s: IOCTL_MODEM_CRASH_REASON\n", iod->name);
			return ld->crash_reason(ld, iod, arg);
		}
		mif_err("%s: !ld->crash_reason\n", iod->name);
		return -EINVAL;

	case IOCTL_MODEM_AIRPLANE_MODE:
		if (ld->airplane_mode) {
			mif_info("%s: IOCTL_MODEM_AIRPLANE_MODE\n", iod->name);
			return ld->airplane_mode(ld, iod, arg);
		}
		mif_err("%s: !ld->airplane_mode\n", iod->name);
		return -EINVAL;

#ifdef CONFIG_LINK_DEVICE_PCIE
	case IOCTL_MODEM_DUMP_RESET:
		if (mc->ops.modem_dump_reset) {
			mif_err("%s: IOCTL_MODEM_DUMP_RESET\n", iod->name);
			ret = mc->ops.modem_dump_reset(mc);
			if (ld->reset_zerocopy)
				ld->reset_zerocopy(ld);
			return ret;
		}
		mif_err("%s: !mc->ops.modem_dump_reset\n", iod->name);
		return -EINVAL;

	case IOCTL_REGISTER_PCIE:
		if (ld->register_pcie) {
			mif_info("%s: IOCTL_REGISTER_PCIE\n", iod->name);
			return ld->register_pcie(ld);
		}
		mif_err("%s: !ld->register_pcie\n", iod->name);
		return -EINVAL;
#endif

	default:
		 /* If you need to handle the ioctl for specific link device,
		  * then assign the link ioctl handler to ld->ioctl
		  * It will be call for specific link ioctl */
		if (ld->ioctl)
			return ld->ioctl(ld, iod, cmd, arg);

		mif_info("%s: ERR! undefined cmd 0x%X\n", iod->name, cmd);
		return -EINVAL;
	}

	return 0;
}

static ssize_t misc_write(struct file *filp, const char __user *data,
			  size_t count, loff_t *fpos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct link_device *ld = get_current_link(iod);
	struct modem_ctl *mc = iod->mc;
	struct sk_buff *skb;
	char *buff;
	int ret;
	u8 cfg;
	unsigned int headroom;
	unsigned int tailroom;
	unsigned int tx_bytes;
	unsigned int copied = 0, tot_frame = 0, copied_frm = 0;
	unsigned int remains;
	unsigned int alloc_size;
	/* 64bit prevent */
	unsigned int cnt = (unsigned int)count;
#ifdef DEBUG_MODEM_IF
	struct timespec ts;
#endif

#ifdef DEBUG_MODEM_IF
	/* Record the timestamp */
	getnstimeofday(&ts);
#endif

	if (iod->format <= IPC_RFS && iod->id == 0)
		return -EINVAL;

	if (unlikely(!cp_online(mc)) && sipc5_ipc_ch(iod->id)) {
		mif_debug("%s: ERR! %s->state == %s\n",
			iod->name, mc->name, mc_state(mc));
		return -EPERM;
	}

	if (iod->link_header) {
		cfg = sipc5_build_config(iod, ld, cnt);
		headroom = sipc5_get_hdr_len(&cfg);
	} else {
		cfg = 0;
		headroom = 0;
	}

	if (unlikely(!receive_first_ipc) && sipc5_log_ch(iod->id))
		return -EBUSY;

	while (copied < cnt) {
		remains = cnt - copied;
		alloc_size = min_t(unsigned int, remains + headroom,
			iod->max_tx_size ?: remains + headroom);

		/* Calculate tailroom for padding size */
		if (iod->link_header && ld->aligned)
			tailroom = sipc5_calc_padding_size(alloc_size);
		else
			tailroom = 0;

		alloc_size += tailroom;

		skb = alloc_skb(alloc_size, GFP_KERNEL);
		if (!skb) {
			mif_info("%s: ERR! alloc_skb fail (alloc_size:%d)\n",
				iod->name, alloc_size);
			return -ENOMEM;
		}

		tx_bytes = alloc_size - headroom - tailroom;

		/* Reserve the space for a link header */
		skb_reserve(skb, headroom);

		/* Copy an IPC message from the user space to the skb */
		buff = skb_put(skb, tx_bytes);
		if (copy_from_user(buff, data + copied, tx_bytes)) {
			mif_err("%s->%s: ERR! copy_from_user fail(count %lu)\n",
				iod->name, ld->name, (unsigned long)count);
			dev_kfree_skb_any(skb);
			return -EFAULT;
		}

		/* Update size of copied payload */
		copied += tx_bytes;
		/* Update size of total frame included hdr, pad size */
		tot_frame += alloc_size;

		/* Store the IO device, the link device, etc. */
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;

		skbpriv(skb)->lnk_hdr = iod->link_header;
		skbpriv(skb)->sipc_ch = iod->id;

#ifdef DEBUG_MODEM_IF
		/* Copy the timestamp to the skb */
		memcpy(&skbpriv(skb)->ts, &ts, sizeof(struct timespec));
#endif
#ifdef DEBUG_MODEM_IF_IODEV_TX
		mif_pkt(iod->id, "IOD-TX", skb);
#endif

		/* Build SIPC5 link header*/
		if (cfg) {
			buff = skb_push(skb, headroom);
			sipc5_build_header(iod, buff, cfg,
					tx_bytes, cnt - copied);
		}

		/* Apply padding */
		if (tailroom)
			skb_put(skb, tailroom);

		/**
		 * Send the skb with a link device
		 */
		ret = ld->send(ld, iod, skb);
		if (ret < 0) {
			mif_err("%s->%s: %s->send fail(%d, tx:%d len:%lu)\n",
				iod->name, mc->name, ld->name,
				ret, tx_bytes, (unsigned long)count);
			dev_kfree_skb_any(skb);
			return ret;
		}
		copied_frm += ret;
	}

	if (copied_frm != tot_frame) {
		mif_info("%s->%s: WARN! %s->send ret:%d (len:%lu)\n",
			iod->name, mc->name, ld->name,
			copied_frm, (unsigned long)count);
	}

	return count;
}

static ssize_t misc_read(struct file *filp, char *buf, size_t count,
			loff_t *fpos)
{
	struct io_device *iod = (struct io_device *)filp->private_data;
	struct sk_buff_head *rxq = &iod->sk_rx_q;
	struct sk_buff *skb;
	int copied;

	if (skb_queue_empty(rxq)) {
		long tmo = msecs_to_jiffies(100);
		wait_event_timeout(iod->wq, !skb_queue_empty(rxq), tmo);
	}

	skb = skb_dequeue(rxq);
	if (unlikely(!skb)) {
		mif_info("%s: NO data in RXQ\n", iod->name);
		return 0;
	}

	copied = skb->len > count ? count : skb->len;

	if (copy_to_user(buf, skb->data, copied)) {
		mif_err("%s: ERR! copy_to_user fail\n", iod->name);
		dev_kfree_skb_any(skb);
		return -EFAULT;
	}

	if (iod->id == SIPC_CH_ID_CPLOG1) {
		struct net_device *ndev = iod->ndev;
		if (!ndev) {
			mif_err("%s: ERR! no iod->ndev\n", iod->name);
		} else {
			ndev->stats.rx_packets++;
			ndev->stats.rx_bytes += copied;
		}
	}

#ifdef DEBUG_MODEM_IF_IODEV_RX
	mif_pkt(iod->id, "IOD-RX", skb);
#endif
	mif_debug("%s: data:%d copied:%d qlen:%d\n",
		iod->name, skb->len, copied, rxq->qlen);

	if (skb->len > copied) {
		skb_pull(skb, copied);
		skb_queue_head(rxq, skb);
	} else {
		dev_consume_skb_any(skb);
	}

	return copied;
}

static const struct file_operations misc_io_fops = {
	.owner = THIS_MODULE,
	.open = misc_open,
	.release = misc_release,
	.poll = misc_poll,
	.unlocked_ioctl = misc_ioctl,
	.compat_ioctl = misc_ioctl,
	.write = misc_write,
	.read = misc_read,
};

static int vnet_open(struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);
	struct io_device *iod = vnet->iod;
	struct modem_shared *msd = vnet->iod->msd;
	struct link_device *ld;
	int ret;

	atomic_inc(&iod->opened);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->init_comm) {
			ret = ld->init_comm(ld, iod);
			if (ret < 0) {
				mif_err("%s<->%s: ERR! init_comm fail(%d)\n",
					iod->name, ld->name, ret);
				atomic_dec(&iod->opened);
				return ret;
			}
		}
	}
	list_add(&iod->node_ndev, &iod->msd->activated_ndev_list);

	netif_start_queue(ndev);

#if defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
	update_rmnet_status(iod, true);
#endif

	mif_err("%s (opened %d) by %s\n",
		iod->name, atomic_read(&iod->opened), current->comm);

	return 0;
}

static int vnet_stop(struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);
	struct io_device *iod = vnet->iod;
	struct modem_shared *msd = iod->msd;
	struct link_device *ld;

	if (atomic_dec_and_test(&iod->opened))
		skb_queue_purge(&vnet->iod->sk_rx_q);

	list_for_each_entry(ld, &msd->link_dev_list, list) {
		if (IS_CONNECTED(iod, ld) && ld->terminate_comm)
			ld->terminate_comm(ld, iod);
	}

	spin_lock(&msd->active_list_lock);
	list_del(&iod->node_ndev);
	spin_unlock(&msd->active_list_lock);
	netif_stop_queue(ndev);

#if defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
	update_rmnet_status(iod, false);
#endif

	mif_err("%s (opened %d) by %s\n",
		iod->name, atomic_read(&iod->opened), current->comm);

	return 0;
}

static int vnet_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct vnet *vnet = netdev_priv(ndev);
	struct io_device *iod = vnet->iod;
	struct link_device *ld = get_current_link(iod);
	struct modem_ctl *mc = iod->mc;
	unsigned int count = skb->len;
	struct sk_buff *skb_new = skb;
	char *buff;
	int ret;
	u8 cfg;
	unsigned int headroom;
	unsigned int tailroom;
	unsigned int tx_bytes;
#ifdef DEBUG_MODEM_IF
	struct timespec ts;
#endif

#ifdef DEBUG_MODEM_IF
	/* Record the timestamp */
	getnstimeofday(&ts);
#endif

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
	ld = get_current_link(get_current_rmnet_tx_iod(iod->id));
	mc = ld->mc;
#endif

	if (unlikely(!cp_online(mc))) {
		if (!netif_queue_stopped(ndev))
			netif_stop_queue(ndev);
		/* Just drop the TX packet */
		goto drop;
	}

	/* When use `handover' with Network Bridge,
	 * user -> bridge device(rmnet0) -> real rmnet(xxxx_rmnet0) -> here.
	 * bridge device is ethernet device unlike xxxx_rmnet(net device).
	 * We remove the an ethernet header of skb before using skb->len,
	 * because bridge device added an ethernet header to skb.
	 */
	if (iod->use_handover) {
		if (iod->id >= SIPC_CH_ID_PDP_0 && iod->id <= SIPC_CH_ID_PDP_14)
			skb_pull(skb, sizeof(struct ethhdr));
	}

	if (iod->link_header) {
		cfg = sipc5_build_config(iod, ld, count);
		headroom = sipc5_get_hdr_len(&cfg);
		if (ld->aligned)
			tailroom = sipc5_calc_padding_size(headroom + count);
		else
			tailroom = 0;
	} else {
		cfg = 0;
		headroom = 0;
		tailroom = 0;
	}

	tx_bytes = headroom + count + tailroom;

	if (skb_headroom(skb) < headroom || skb_tailroom(skb) < tailroom) {
		skb_new = skb_copy_expand(skb, headroom, tailroom, GFP_ATOMIC);
		if (!skb_new) {
			mif_info("%s: ERR! skb_copy_expand fail\n", iod->name);
			goto retry;
		}
	}

	/* Store the IO device, the link device, etc. */
	skbpriv(skb_new)->iod = iod;
	skbpriv(skb_new)->ld = ld;

	skbpriv(skb_new)->lnk_hdr = iod->link_header;
	skbpriv(skb_new)->sipc_ch = iod->id;

#ifdef DEBUG_MODEM_IF
	/* Copy the timestamp to the skb */
	memcpy(&skbpriv(skb_new)->ts, &ts, sizeof(struct timespec));
#endif
#if defined(DEBUG_MODEM_IF_IODEV_TX) && defined(DEBUG_MODEM_IF_PS_DATA)
	mif_pkt(iod->id, "IOD-TX", skb_new);
#endif

	/* Build SIPC5 link header*/
	buff = skb_push(skb_new, headroom);
	if (cfg)
		sipc5_build_header(iod, buff, cfg, count, 0);

	/* IP loop-back */
	if (iod->msd->loopback_ipaddr) {
		struct iphdr *ip_header = (struct iphdr *)skb->data;
		if (ip_header->daddr == iod->msd->loopback_ipaddr) {
			swap(ip_header->saddr, ip_header->daddr);
			buff[SIPC5_CH_ID_OFFSET] = DATA_LOOPBACK_CHANNEL;
		}
	}

	/* Apply padding */
	if (tailroom)
		skb_put(skb_new, tailroom);

	ret = ld->send(ld, iod, skb_new);
	if (unlikely(ret < 0)) {
		static DEFINE_RATELIMIT_STATE(_rs, HZ, 100);

		if (ret != -EBUSY) {
			mif_err_limited("%s->%s: ERR! %s->send fail:%d "
					"(tx_bytes:%d len:%d)\n",
					iod->name, mc->name, ld->name, ret,
					tx_bytes, count);
			goto drop;
		}

		/* do 100-retry for every 1sec */
		if (__ratelimit(&_rs))
			goto retry;
		goto drop;
	}

	if (ret != tx_bytes) {
		mif_info("%s->%s: WARN! %s->send ret:%d (tx_bytes:%d len:%d)\n",
			iod->name, mc->name, ld->name, ret, tx_bytes, count);
	}

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += count;

	/*
	If @skb has been expanded to $skb_new, @skb must be freed here.
	($skb_new will be freed by the link device.)
	*/
	if (skb_new != skb)
		dev_consume_skb_any(skb);

	return NETDEV_TX_OK;

retry:
	/*
	If @skb has been expanded to $skb_new, only $skb_new must be freed here
	because @skb will be reused by NET_TX.
	*/
	if (skb_new && skb_new != skb)
		dev_consume_skb_any(skb_new);

	return NETDEV_TX_BUSY;

drop:
	ndev->stats.tx_dropped++;

	DROPDUMP_QPCAP_SKB(skb, NET_DROPDUMP_OPT_MIF_TXFAIL);
	dev_kfree_skb_any(skb);

	/*
	If @skb has been expanded to $skb_new, $skb_new must also be freed here.
	*/
	if (skb_new != skb)
		dev_consume_skb_any(skb_new);

	return NETDEV_TX_OK;
}

#if defined(CONFIG_MODEM_IF_LEGACY_QOS) || defined(CONFIG_MODEM_IF_QOS)
static u16 vnet_select_queue(struct net_device *dev, struct sk_buff *skb,
		void *accel_priv, select_queue_fallback_t fallback)
{
	return (skb && skb->priomark == RAW_HPRIO) ? 1 : 0;
}
#endif

static int dummy_net_open(struct net_device *ndev)
{
	return -EINVAL;
}

static struct net_device_ops dummy_net_ops = {
	.ndo_open = dummy_net_open,
};

static struct net_device_ops vnet_ops = {
	.ndo_open = vnet_open,
	.ndo_stop = vnet_stop,
	.ndo_start_xmit = vnet_xmit,
#if defined(CONFIG_MODEM_IF_LEGACY_QOS) || defined(CONFIG_MODEM_IF_QOS)
	.ndo_select_queue = vnet_select_queue,
#endif
};

static void vnet_setup(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_RAWIP;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	ndev->addr_len = 0;
	ndev->hard_header_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
#ifdef CONFIG_MODEM_IF_NET_GRO
	ndev->features |= NETIF_F_GRO;
#endif
}

static void vnet_setup_ether(struct net_device *ndev)
{
	ndev->netdev_ops = &vnet_ops;
	ndev->type = ARPHRD_ETHER;
	ndev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST | IFF_SLAVE;
	ndev->addr_len = ETH_ALEN;
	random_ether_addr(ndev->dev_addr);
	ndev->hard_header_len = 0;
	ndev->tx_queue_len = 1000;
	ndev->mtu = ETH_DATA_LEN;
	ndev->watchdog_timeo = 5 * HZ;
#ifdef CONFIG_MODEM_IF_NET_GRO
	ndev->features |= NETIF_F_GRO;
#endif
}

static inline void sipc5_inc_info_id(struct io_device *iod)
{
	spin_lock(&iod->info_id_lock);
	iod->info_id = (iod->info_id + 1) & 0x7F;
	spin_unlock(&iod->info_id_lock);
}

static u8 sipc5_build_config(struct io_device *iod, struct link_device *ld,
			     unsigned int count)
{
	u8 cfg = SIPC5_START_MASK;

	if (iod->format > IPC_DUMP)
		return 0;

	if (ld->aligned)
		cfg |= SIPC5_PADDING_EXIST;

	if (iod->max_tx_size > 0 &&
		(count + SIPC5_MIN_HEADER_SIZE) > iod->max_tx_size) {
		mif_info("%s: MULTI_FRAME_CFG: count=%u\n", iod->name, count);
		cfg |= SIPC5_MULTI_FRAME_CFG;
		sipc5_inc_info_id(iod);
	}

	return cfg;
}

static void sipc5_build_header(struct io_device *iod, u8 *buff, u8 cfg,
		unsigned int tx_bytes, unsigned int remains)
{
	u16 *sz16 = (u16 *)(buff + SIPC5_LEN_OFFSET);
	u32 *sz32 = (u32 *)(buff + SIPC5_LEN_OFFSET);
	unsigned int hdr_len = sipc5_get_hdr_len(&cfg);
	u8 ctrl;

	/* Store the config field and the channel ID field */
	buff[SIPC5_CONFIG_OFFSET] = cfg;
	buff[SIPC5_CH_ID_OFFSET] = iod->id;

	/* Store the frame length field */
	if (sipc5_ext_len(buff))
		*sz32 = (u32)(hdr_len + tx_bytes);
	else
		*sz16 = (u16)(hdr_len + tx_bytes);

	/* Store the control field */
	if (sipc5_multi_frame(buff)) {
		ctrl = (remains > 0) ? 1 << 7 : 0;
		ctrl |= iod->info_id;
		buff[SIPC5_CTRL_OFFSET] = ctrl;
		mif_info("MULTI: ctrl=0x%x(tx_bytes:%u, remains:%u)\n",
				ctrl, tx_bytes, remains);
	}
}

int sipc5_init_io_device(struct io_device *iod)
{
	int ret = 0;
	int i;
	struct vnet *vnet;

	if (iod->attrs & IODEV_ATTR(ATTR_SBD_IPC))
		iod->sbd_ipc = true;

	if (iod->attrs & IODEV_ATTR(ATTR_NO_LINK_HEADER))
		iod->link_header = false;
	else
		iod->link_header = true;

	/* Get modem state from modem control device */
	iod->modem_state_changed = io_dev_modem_state_changed;
	iod->sim_state_changed = io_dev_sim_state_changed;

	/* Get data from link device */
	iod->recv_skb_single = io_dev_recv_skb_single_from_link_dev;
	iod->recv_net_skb = io_dev_recv_net_skb_from_link_dev;

	/* Register misc or net device */
	switch (iod->io_typ) {
	case IODEV_MISC:
		init_waitqueue_head(&iod->wq);
		skb_queue_head_init(&iod->sk_rx_q);

		iod->miscdev.minor = MISC_DYNAMIC_MINOR;
		iod->miscdev.name = iod->name;
		iod->miscdev.fops = &misc_io_fops;

		ret = misc_register(&iod->miscdev);
		if (ret)
			mif_info("%s: ERR! misc_register failed\n", iod->name);

		if (iod->id == SIPC_CH_ID_CPLOG1) {
			iod->ndev = alloc_netdev(0, iod->name,
					NET_NAME_UNKNOWN, vnet_setup);
			if (!iod->ndev) {
				mif_info("%s: ERR! alloc_netdev fail\n", iod->name);
				return -ENOMEM;
			}

			iod->ndev->netdev_ops = &dummy_net_ops;
			ret = register_netdev(iod->ndev);
			if (ret) {
				mif_info("%s: ERR! register_netdev fail\n", iod->name);
				free_netdev(iod->ndev);
			}

			vnet = netdev_priv(iod->ndev);
			vnet->iod = iod;
			mif_info("iod:%s, both registerd\n", iod->name);
		}
		break;

	case IODEV_NET:
		skb_queue_head_init(&iod->sk_rx_q);
		INIT_LIST_HEAD(&iod->node_ndev);

		if (iod->use_handover)
			iod->ndev = alloc_netdev_mqs(sizeof(struct vnet),
					iod->name, NET_NAME_UNKNOWN,
					vnet_setup_ether, MAX_NDEV_TX_Q,
					MAX_NDEV_RX_Q);
		else
			iod->ndev = alloc_netdev_mqs(sizeof(struct vnet),
					iod->name, NET_NAME_UNKNOWN, vnet_setup,
					MAX_NDEV_TX_Q, MAX_NDEV_RX_Q);

		if (!iod->ndev) {
			mif_info("%s: ERR! alloc_netdev fail\n", iod->name);
			return -ENOMEM;
		}

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
		if (!strncmp(iod->name, "dummy", 5)) {
			insert_rmnet_iod_with_channel(iod, RMNET_LINK_5G);
			goto jump_reg;
		} else
			insert_rmnet_iod_with_channel(iod, RMNET_LINK_4G);
#endif
		ret = register_netdev(iod->ndev);
		if (ret) {
			mif_info("%s: ERR! register_netdev fail\n", iod->name);
			free_netdev(iod->ndev);
		}

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
jump_reg:
#endif
		mif_debug("iod 0x%pK\n", iod);
		vnet = netdev_priv(iod->ndev);
		mif_debug("vnet 0x%pK\n", vnet);
		vnet->iod = iod;

		break;

	case IODEV_DUMMY:
		skb_queue_head_init(&iod->sk_rx_q);

		iod->miscdev.minor = MISC_DYNAMIC_MINOR;
		iod->miscdev.name = iod->name;

		ret = misc_register(&iod->miscdev);
		if (ret)
			mif_info("%s: ERR! misc_register fail\n", iod->name);

		ret = device_create_file(iod->miscdev.this_device,
					&attr_waketime);
		if (ret)
			mif_info("%s: ERR! device_create_file fail\n",
				iod->name);

		ret = device_create_file(iod->miscdev.this_device,
				&attr_loopback);
		if (ret)
			mif_err("failed to create `loopback file' : %s\n",
					iod->name);

		ret = device_create_file(iod->miscdev.this_device,
				&attr_txlink);
		if (ret)
			mif_err("failed to create `txlink file' : %s\n",
					iod->name);
		break;

	default:
		mif_info("%s: ERR! wrong io_type %d\n", iod->name, iod->io_typ);
		return -EINVAL;
	}

	for (i = 0; i < NUM_SIPC_MULTI_FRAME_IDS; i++)
		skb_queue_head_init(&iod->sk_multi_q[i]);

	return ret;
}

void sipc5_deinit_io_device(struct io_device *iod)
{
	mif_err("%s: io_typ=%d\n", iod->name, iod->io_typ);

	wake_lock_destroy(&iod->wakelock);

	/* De-register misc or net device */
	switch (iod->io_typ) {
	case IODEV_MISC:
		if (iod->id == SIPC_CH_ID_CPLOG1) {
			unregister_netdev(iod->ndev);
			free_netdev(iod->ndev);
		}

		misc_deregister(&iod->miscdev);
		break;

	case IODEV_NET:
		unregister_netdev(iod->ndev);
		free_netdev(iod->ndev);
		break;

	case IODEV_DUMMY:
		device_remove_file(iod->miscdev.this_device, &attr_waketime);
		device_remove_file(iod->miscdev.this_device, &attr_loopback);
		device_remove_file(iod->miscdev.this_device, &attr_txlink);

		misc_deregister(&iod->miscdev);
		break;
	}
}

