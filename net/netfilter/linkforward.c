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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/tcp.h>
#include <linux/linkforward.h>

#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_l3proto.h>
#include <net/netfilter/nf_nat_l4proto.h>

#define MAX_CONNECTION_CNT 20

//#define DEBUG_LINK_FORWARD
//#define DEBUG_TEST_UDP

struct nf_linkforward nf_linkfwd = {
	.use = false,
	.mode = 1,	/* RX only */
	.brdg.inner = NULL,
	.brdg.outter = NULL,
};

/* Table structures */
struct linkforward_connection {
	bool enabled;
	__be16 dst_port;
	u32 cnt_reply;
	u32 cnt_orign;
	struct net_device *netdev;
	struct nf_conntrack_tuple t[IP_CT_DIR_MAX];
};

static int last_conn_idx;
static struct linkforward_connection conn[MAX_CONNECTION_CNT];

/* work */
static struct delayed_work linkforward_dwork;

/* Test Devices & Addresses */
#define	TEST_PC_IP_ADDR		0xc0a82a75 /* 192.168.42.117 */
struct in_addr test_host_ip_addr = {
	.s_addr = htonl(TEST_PC_IP_ADDR)
};

/* Controlling link forward */
void set_linkforward_mode(int mode)
{
	nf_linkfwd.mode = mode;
}

void linkforward_enable(void)
{
	pr_info("linkforward_enable\n");
	nf_linkfwd.use = 1;
}

void linkforward_disable(void)
{
	pr_info("linkforward_disable\n");
	nf_linkfwd.use = 0;
}

void clat_enable_dev(char *dev_name)
{
	if (strstr(dev_name, "rmnet0"))
		nf_linkfwd.ctun[0].enable = 1;
	else if (strstr(dev_name, "rmnet1"))
		nf_linkfwd.ctun[1].enable = 1;

	pr_info("clat_enable_dev %s\n", dev_name);
}

void clat_disable_dev(char *dev_name)
{
	if (strstr(dev_name, "rmnet0"))
		nf_linkfwd.ctun[0].enable = 0;
	else if (strstr(dev_name, "rmnet1"))
		nf_linkfwd.ctun[1].enable = 0;

	pr_info("clat_disable_dev %s\n", dev_name);
}

void linkforward_table_init(void)
{
	int i;

	last_conn_idx = 0;
	for (i = 0; i < MAX_CONNECTION_CNT; i++) {
		conn[i].enabled = false;
		conn[i].dst_port = 0;
		conn[i].cnt_reply = 0;
		conn[i].cnt_orign = 0;
	}
}

#ifdef DEBUG_LINK_FORWARD
static void print_linkforward_list(void)
{
	int i;

	pr_info("MAX_CONNECTION_CNT:%d last_conn_idx:%d\n", MAX_CONNECTION_CNT,
		last_conn_idx);

	for (i = 0; i < MAX_CONNECTION_CNT; i++)
		pr_info("[%d/%s] : enabled:%d %hu\n",
			i, conn[i].netdev->name,
			conn[i].enabled,
			ntohs(conn[i].dst_port));
}
#endif

ssize_t linkforward_get_state(char *buf)
{
	int i;
	ssize_t count = 0;

	for (i = 0; i < MAX_CONNECTION_CNT; i++) {
		if (conn[i].enabled) {
			count += sprintf(&buf[count], "[%d/%s] %hu (tx = %u, rx = %u)\n",
						i, conn[i].netdev->name, ntohs(conn[i].dst_port),
						conn[i].cnt_orign, conn[i].cnt_reply);
		}
	}
	return count;
}

int linkforward_add(__be16 dst_port, struct nf_conntrack_tuple *t_rpl, struct nf_conntrack_tuple *t_org, struct net_device *netdev)
{
	int i;
	bool room_found = false;

	if (ntohs(t_org->src.u.all) != ntohs(t_rpl->dst.u.all))
		return -1; /* No supporting port translation */

#ifdef DEBUG_LINK_FORWARD
	pr_info("Add Connection: %s [%hu] %pI4:%hu -> %pI4:%hu (reply %pI4:%hu -> %pI4:%hu)\n",
		netdev->name,
		ntohs(dst_port),
		&t_org->src.u3.ip,
		ntohs(t_org->src.u.all),
		&t_org->dst.u3.ip,
		ntohs(t_org->dst.u.all),
		&t_rpl->src.u3.ip,
		ntohs(t_rpl->src.u.all),
		&t_rpl->dst.u3.ip,
		ntohs(t_rpl->dst.u.all));
#endif

	for (i = last_conn_idx; i < MAX_CONNECTION_CNT; i++)
		if (!conn[i].enabled) {
			conn[i].enabled = true;
			conn[i].dst_port = dst_port;
			conn[i].netdev = netdev;
			memcpy(&conn[i].t[0], t_org, sizeof(struct nf_conntrack_tuple));
			memcpy(&conn[i].t[1], t_rpl, sizeof(struct nf_conntrack_tuple));
			last_conn_idx = i;
			room_found = true;

#ifdef CONFIG_CP_DIT
			dit_set_nat_local_addr(t_org->src.u3.ip);
			dit_set_nat_filter(i, IPPROTO_TCP, 0xffffffff, 0xffff, dst_port);
#endif
			break;
		}

	if (!room_found) {
		for (i = 0; i < last_conn_idx; i++)
			if (!conn[i].enabled) {
				conn[i].enabled = true;
				conn[i].dst_port = dst_port;
				conn[i].netdev = netdev;
				memcpy(&conn[i].t[0], t_org, sizeof(struct nf_conntrack_tuple));
				memcpy(&conn[i].t[1], t_rpl, sizeof(struct nf_conntrack_tuple));
				last_conn_idx = i;
				room_found = true;

#ifdef CONFIG_CP_DIT
				dit_set_nat_local_addr(t_org->src.u3.ip);
				dit_set_nat_filter(i, IPPROTO_TCP, 0xffffffff, 0xffff, dst_port);
#endif
				break;
			}
	}

	if (!room_found) {
		last_conn_idx++;
		if (last_conn_idx == MAX_CONNECTION_CNT)
			last_conn_idx = 0;
			i = last_conn_idx;
			conn[i].enabled = true;
			conn[i].dst_port = dst_port;
			conn[i].netdev = netdev;
			memcpy(&conn[i].t[0], t_org, sizeof(struct nf_conntrack_tuple));
			memcpy(&conn[i].t[1], t_rpl, sizeof(struct nf_conntrack_tuple));

#ifdef CONFIG_CP_DIT
			dit_set_nat_local_addr(t_org->src.u3.ip);
			dit_set_nat_filter(i, IPPROTO_TCP, 0xffffffff, 0xffff, dst_port);
#endif
	}

#ifdef DEBUG_LINK_FORWARD
	pr_info("Added at index:%d\n", last_conn_idx);
	print_linkforward_list();
#endif

	return 0;
}

int linkforward_delete(__be16 dst_port)
{
	int i;

	for (i = last_conn_idx; i < MAX_CONNECTION_CNT; i++)
		if (conn[i].enabled && conn[i].dst_port == dst_port) {
			conn[i].enabled = false;
			conn[i].dst_port = 0;
			conn[i].netdev = NULL;
			conn[i].cnt_reply = 0;
			conn[i].cnt_orign = 0;

#ifdef CONFIG_CP_DIT
			dit_del_nat_filter(i);
#endif
#ifdef DEBUG_LINK_FORWARD
			pr_info("Delete connection %d was found (port =%hu)\n", i, ntohs(dst_port));
			print_linkforward_list();
#endif
			return 0;
		}

	for (i = 0; i < last_conn_idx; i++)
		if (conn[i].enabled && conn[i].dst_port == dst_port) {
			conn[i].enabled = false;
			conn[i].dst_port = 0;
			conn[i].netdev = NULL;
			conn[i].cnt_reply = 0;
			conn[i].cnt_orign = 0;

#ifdef CONFIG_CP_DIT
			dit_del_nat_filter(i);
#endif
#ifdef DEBUG_LINK_FORWARD
			pr_info("Delete connection %d was found (port =%hu)\n", i, ntohs(dst_port));
			print_linkforward_list();
#endif
			return 0;
		}

	return -1;
}

static struct nf_conntrack_tuple *__linkforward_check_tuple(__be16 src_port, __be16 dst_port, enum linkforward_dir dir)
{
	int i;
	__be16 port = dir ? dst_port : src_port;
	struct nf_conntrack_tuple *target;

#ifdef DEBUG_LINK_FORWARD
	pr_info("find Connection: [dir=%d] port:%hu\n",
		dir, dir ? ntohs(dst_port) : ntohs(src_port));
#endif

	for (i = last_conn_idx; i < MAX_CONNECTION_CNT; i++)
		if (conn[i].enabled && conn[i].dst_port == port) {
			if (dir) {
				target = &conn[i].t[0]; conn[i].cnt_reply++;
			} else {
				target = &conn[i].t[1]; conn[i].cnt_orign++;
			}

#ifdef DEBUG_LINK_FORWARD
			pr_info("connection %d was found at %s\n\t[%d] %pI4:%hu -> %pI4:%hu\n",
				i, conn[i].netdev,
				ntohs(port),
				&target->src.u3.ip,
				ntohs(target->src.u.all),
				&target->dst.u3.ip,
				ntohs(target->dst.u.all));
#endif

			return target;
		}

	for (i = 0; i < last_conn_idx; i++)
		if (conn[i].enabled && conn[i].dst_port == port) {
			if (dir) {
				target = &conn[i].t[0]; conn[i].cnt_reply++;
			} else {
				target = &conn[i].t[1]; conn[i].cnt_orign++;
			}

#ifdef DEBUG_LINK_FORWARD
			pr_info("connection %d was found at %s\n\t[%d] %pI4:%hu -> %pI4:%hu\n",
				i, conn[i].netdev,
				ntohs(port),
				&target->src.u3.ip,
				ntohs(target->src.u.all),
				&target->dst.u3.ip,
				ntohs(target->dst.u.all));
#endif

			return target;
		}

#ifdef DEBUG_LINK_FORWARD
	pr_info("connection was NOT found\n");
#endif

	return NULL;
}

/* manipulation */
int __linkforward_manip_skb(struct sk_buff *skb, enum linkforward_dir dir)
{
	unsigned char *packet = skb->data;
	struct iphdr *iphdr;

	struct nf_conntrack_tuple *target;

	/* check the version of IP */
	iphdr = (struct iphdr *)packet;

	if (iphdr->version == 4) {
		__be32 addr, addr_new;
		struct ethhdr *ehdr;

		switch (/*iphdr->frag_off & htons(IP_OFFSET) ? 0 : */iphdr->protocol) {
		case IPPROTO_TCP:
		{
			struct tcphdr *tcph;
#ifndef CONFIG_CP_DIT
			__be16 *portptr, newport, oldport;
#endif

			tcph = (void *)(packet + iphdr->ihl * 4);

			target = __linkforward_check_tuple(tcph->source, tcph->dest, dir);
			if (target) {
#ifndef CONFIG_CP_DIT
				if (dir == LINK_FORWARD_DIR_REPLY) {
					/* destination IP/Port manipulation */
					addr = iphdr->daddr;
					iphdr->daddr = target->src.u3.ip;
					csum_replace4(&iphdr->check, addr, target->src.u3.ip);

					/* ToDo: port translation */
					newport = target->src.u.tcp.port;
					portptr = &tcph->dest;

					oldport = *portptr;
					*portptr = newport;

#ifdef DEBUG_LINK_FORWARD
					pr_info("manip: [dir=%d] newport %pI4:%hu <- oldport %pI4:%hu (Seq=0x%08x, Ack=0x%08x)\n",
						dir, &iphdr->daddr, ntohs(newport), &addr, ntohs(oldport),
						ntohl(tcph->seq), ntohl(tcph->ack_seq));
#endif

					csum_replace2(&tcph->check, oldport, newport);
					csum_replace4(&tcph->check, addr, target->src.u3.ip);

				} else {
					/* source IP/Port manipulation */
					addr = iphdr->saddr;
					iphdr->saddr = target->dst.u3.ip;
					csum_replace4(&iphdr->check, addr, target->dst.u3.ip);

					/* ToDo: port translation */
					newport = target->dst.u.tcp.port;
					portptr = &tcph->source;

					oldport = *portptr;
					*portptr = newport;

#ifdef DEBUG_LINK_FORWARD
					pr_info("manip: [dir=%d] oldport %pI4:%hu -> newport %pI4:%hu (Seq=0x%08x, Ack=0x%08x)\n",
						dir,  &addr, ntohs(oldport), &iphdr->saddr, ntohs(newport),
						ntohl(tcph->seq), ntohl(tcph->ack_seq));
#endif

					csum_replace2(&tcph->check, oldport, newport);
					csum_replace4(&tcph->check, addr, target->dst.u3.ip);

				}
#endif

				skb->hash = tcph->dest;
				skb->sw_hash = 1;

			} else {
				return 0;
			}

			break;
		}
#ifdef DEBUG_TEST_UDP
		case IPPROTO_UDP:
		{
			struct udphdr *udph;

			udph = (void *)(packet + iphdr->ihl * 4);

			if (udph->dest != htons(5001)) /* TEST-ONLY for iperf */
				return 0;

#ifndef CONFIG_CP_DIT
			addr_new = test_host_ip_addr.s_addr;

			addr = iphdr->daddr;
			iphdr->daddr = addr_new;
			csum_replace4(&iphdr->check, addr, addr_new);

			/* ToDo: port translation */
			/*csum_replace2(sum, old, new);*/

			if (udph->check != 0000)
				csum_replace4(&udph->check, addr, addr_new);
#endif
			skb->hash = udph->dest;
			skb->sw_hash = 1;

			break;
		}
#endif
		case IPPROTO_ICMP:
		{
			struct icmphdr *icmph;

			return 0; /* Not handling ICMP FORCE RETURN */

			icmph = (void *)(packet + iphdr->ihl * 4);
			csum_replace4(&icmph->checksum, addr, addr_new);

			break;
		}
		default:
			return 0; /* FORCE RETURN */
		}

		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb->priority = iphdr->tos;

		if (dir == LINK_FORWARD_DIR_REPLY) {
			/* mac header */
			skb_reset_network_header(skb);
			ehdr = (struct ethhdr *)skb_push(skb, sizeof(struct ethhdr));
			memcpy(ehdr->h_dest, nf_linkfwd.brdg.ldev_mac, ETH_ALEN);
			memcpy(ehdr->h_source, nf_linkfwd.brdg.if_mac, ETH_ALEN);
			ehdr->h_proto = skb->protocol;
			skb_reset_mac_header(skb);

			skb->protocol = htons(ETH_P_ALL);
			skb->dev = get_linkforwd_inner_dev();
		} else {
			skb_reset_network_header(skb);
			skb_reset_mac_header(skb);
			skb->dev = get_linkforwd_outter_dev();
		}

		*((u32 *)&skb->cb) = dir ? SIGNATURE_LINK_FORWRD_REPLY : SIGNATURE_LINK_FORWRD_ORIGN;

		return 1;
	}

	return 0;
}

/* Device core functions */
static inline int __dev_linkforward_queue_skb(struct sk_buff *skb, struct Qdisc *q)
{
	spinlock_t *root_lock = qdisc_lock(q);
	struct sk_buff *to_free = NULL;
	bool contended;
	int rc;
	enum linkforward_dir dir;

	qdisc_calculate_pkt_len(skb, q);
	contended = qdisc_is_running(q);
	if (unlikely(contended))
		spin_lock(&q->busylock);

	spin_lock(root_lock);
	if (unlikely(test_bit(__QDISC_STATE_DEACTIVATED, &q->state))) {
		__qdisc_drop(skb, &to_free);
		rc = NET_XMIT_DROP;
	} else {
		rc = q->enqueue(skb, q, &to_free) & NET_XMIT_MASK;
		if (unlikely(contended)) {
			spin_unlock(&q->busylock);
			contended = false;
		}

		dir = ((*((u32 *)&skb->cb)) == SIGNATURE_LINK_FORWRD_REPLY) ?
						LINK_FORWARD_DIR_REPLY : LINK_FORWARD_DIR_ORIGN;

		atomic_set(&nf_linkfwd.qmask[dir], 1);
		nf_linkfwd.q[dir] = q;

		queue_delayed_work(system_wq, &linkforward_dwork, 0);
	}
	spin_unlock(root_lock);
	if (unlikely(to_free))
		kfree_skb_list(to_free);
	if (unlikely(contended))
		spin_unlock(&q->busylock);
	return rc;
}

static int __dev_linkforward_queue_xmit(struct sk_buff *skb, void *accel_priv)
{
	struct net_device *dev = skb->dev;
	struct netdev_queue *txq;
	struct Qdisc *q;
	int rc = -ENOMEM;

//	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_SCHED_TSTAMP))
//		__skb_tstamp_tx(skb, NULL, skb->sk, SCM_TSTAMP_SCHED);

	/* Disable soft irqs for various locks below. Also
	 * stops preemption for RCU.
	 */
	rcu_read_lock_bh();

	txq = netdev_pick_tx(dev, skb, accel_priv);
	q = rcu_dereference_bh(txq->qdisc);

	if (q->enqueue)
		rc = __dev_linkforward_queue_skb(skb, q);

	rcu_read_unlock_bh();

	return rc;
}

int dev_linkforward_queue_xmit(struct sk_buff *skb)
{
	int ret;

	/* ToDo: IP_UPD_PO_STATS(net, IPSTATS_MIB_OUT, skb->len); */
	ret = __dev_linkforward_queue_xmit(skb, NULL);
	switch (ret) {
	case NET_XMIT_SUCCESS:
		break;
	case NETDEV_TX_BUSY:
		kfree_skb(skb);
	case NET_XMIT_DROP:
	case NET_XMIT_CN:
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL(dev_linkforward_queue_xmit);

/* Netfilter linkforward API */
int nf_linkforward_add(struct nf_conn *ct)
{
	struct nf_conntrack_tuple_hash *hash = &ct->tuplehash[IP_CT_DIR_REPLY];
	struct nf_conntrack_tuple *t = &hash->tuple;
	struct nf_conntrack_tuple_hash *org_h = &ct->tuplehash[IP_CT_DIR_ORIGINAL];

#ifdef DEBUG_LINK_FORWARD
	pr_info("add linkforward [%s] %pI4:%hu -> %pI4:%hu\n",
		ct->netdev->name,
		&t->src.u3.ip,		/* __be32 */
		ntohs(t->src.u.all),	/* __be16 */
		&t->dst.u3.ip,
		ntohs(t->dst.u.all));
#endif

	if (!nf_linkfwd.use || nf_linkfwd.brdg.inner != ct->netdev)
		return -1;

	linkforward_add(t->dst.u.all, t, &org_h->tuple, ct->netdev);

	ct->linkforward_registered = true;

	return 0;
}

int nf_linkforward_delete(struct nf_conn *ct)
{
	struct nf_conntrack_tuple_hash *hash = &ct->tuplehash[IP_CT_DIR_REPLY];
	struct nf_conntrack_tuple *t = &hash->tuple;

	if (!ct->linkforward_registered)
		return -1;

	/* Check IPv4 */
	if (t->src.l3num != AF_INET)
		return -1;

	/* Check TCP Protocol */
	if (t->dst.protonum != SOL_TCP)
		return -1;

#ifdef DEBUG_LINK_FORWARD
	pr_info("delete linkforward: %pI4:%hu -> %pI4:%hu\n",
		&t->src.u3.ip,		/* __be32 */
		ntohs(t->src.u.all),	/* __be16 */
		&t->dst.u3.ip,
		ntohs(t->dst.u.all));
#endif

	linkforward_delete(t->dst.u.all);

	ct->linkforward_registered = false;

	return 0;
}

void nf_linkforward_monitor(struct nf_conn *ct)
{
	struct nf_conntrack_tuple_hash *hash = &ct->tuplehash[0];
	struct nf_conntrack_tuple *t = &hash->tuple;
	static int cnt;

	/* Check IPv4 */
	if (t->src.l3num != AF_INET)
		return;

	/* Check TCP Protocol */
	if (t->dst.protonum != SOL_TCP)
		return;

	cnt++;

	/* Increase packet count */
	ct->packet_count++;

#ifdef DEBUG_LINK_FORWARD
	pr_info("[%d] tuple %pI4:%hu -> %pI4:%hu\n",
		ct->packet_count,
		&t->src.u3.ip,		/* __be32 */
		ntohs(t->src.u.all),	/* __be16 */
		&t->dst.u3.ip,
		ntohs(t->dst.u.all));
#endif

	if (ct->packet_count == THRESHOLD_LINK_FORWARD)
		nf_linkforward_add(ct);
}

/* Net Device Notifier */
static int netdev_linkforward_event(struct notifier_block *this,
					  unsigned long event, void *ptr)
{
	struct net_device *net = netdev_notifier_info_to_dev(ptr);

	if (strstr(net->name, "rndis") || strstr(net->name, "rmnet") || strstr(net->name, "ncm")) {
		switch (event) {
		case NETDEV_CHANGEMTU:
			break;

		case NETDEV_CHANGEADDR:
			break;

		case NETDEV_CHANGE:
			break;

		case NETDEV_FEAT_CHANGE:
			break;

		case NETDEV_GOING_DOWN:
		case NETDEV_DOWN:
			if (strstr(net->name, "rndis") || strstr(net->name, "ncm")) {
				linkforward_disable();
				set_linkforwd_inner_dev(NULL);
#if defined(DEBUG_TEST_UDP) && defined (CONFIG_CP_DIT)
				dit_del_nat_filter(0);
#endif
			} else if (strstr(net->name, "v4-rmnet")) {
				clat_disable_dev(net->name);
			} else if (strstr(net->name, "rmnet")) {
				set_linkforwd_outter_dev(NULL);
			}
			break;

		case NETDEV_UP:
			if (strstr(net->name, "rndis") || strstr(net->name, "ncm")) {
				linkforward_table_init();

				memcpy(nf_linkfwd.brdg.if_mac, net->dev_addr, ETH_ALEN);
				gether_get_host_addr_u8(net, nf_linkfwd.brdg.ldev_mac);

#ifdef DEBUG_LINK_FORWARD
				pr_info(" %s ldev MAC %pM\n", net->name, nf_linkfwd.brdg.ldev_mac);
				pr_info(" %s ifac MAC %pM\n", net->name, nf_linkfwd.brdg.if_mac);
#endif
#if defined(DEBUG_TEST_UDP) && defined (CONFIG_CP_DIT)
				dit_set_nat_local_addr(test_host_ip_addr.s_addr);
				dit_set_nat_filter(0, IPPROTO_UDP, 0xffffffff, 0xffff, htons(5001));
#endif

				set_linkforwd_inner_dev(net);
				linkforward_enable();
			} else if (strstr(net->name, "v4-rmnet")) {
			} else if (strstr(net->name, "rmnet")) {
				set_linkforwd_outter_dev(net);
			}

			break;

		case NETDEV_UNREGISTER:
			if (strstr(net->name, "rndis") || strstr(net->name, "ncm")) {
				linkforward_disable();
				set_linkforwd_inner_dev(NULL);
#if defined(DEBUG_TEST_UDP) && defined (CONFIG_CP_DIT)
				dit_del_nat_filter(0);
#endif
			} else if (strstr(net->name, "v4-rmnet")) {
				clat_disable_dev(net->name);
			} else if (strstr(net->name, "rmnet")) {
				set_linkforwd_outter_dev(NULL);
			}
			break;

		case NETDEV_CHANGENAME:
			break;

		case NETDEV_PRE_TYPE_CHANGE:
			break;

		case NETDEV_RESEND_IGMP:
			break;
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block netdev_linkforward_notifier = {
	.notifier_call	= netdev_linkforward_event,
};

/* work */
static void linkforward_work(struct work_struct *ws)
{
	if (atomic_read(&nf_linkfwd.qmask[LINK_FORWARD_DIR_ORIGN])) {
		atomic_set(&nf_linkfwd.qmask[LINK_FORWARD_DIR_ORIGN], 0);
		__netif_schedule(nf_linkfwd.q[LINK_FORWARD_DIR_ORIGN]);
	}

	if (atomic_read(&nf_linkfwd.qmask[LINK_FORWARD_DIR_REPLY])) {
		atomic_set(&nf_linkfwd.qmask[LINK_FORWARD_DIR_REPLY], 0);
		__netif_schedule(nf_linkfwd.q[LINK_FORWARD_DIR_REPLY]);
	}
}

void linkforward_init(void)
{
	atomic_set(&nf_linkfwd.qmask[LINK_FORWARD_DIR_ORIGN], 0);
	atomic_set(&nf_linkfwd.qmask[LINK_FORWARD_DIR_REPLY], 0);

	nf_linkfwd.q[LINK_FORWARD_DIR_ORIGN] = NULL;
	nf_linkfwd.q[LINK_FORWARD_DIR_REPLY] = NULL;

	register_netdevice_notifier(&netdev_linkforward_notifier);

	INIT_DELAYED_WORK(&linkforward_dwork, linkforward_work);
}
