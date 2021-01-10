/*
 * Monitoring code for network dropped packet alerts
 *
 * Copyright (C) 2018 SAMSUNG Electronics, Co,LTD
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/string.h>
#include <linux/inetdevice.h>
#include <linux/inet.h>
#include <linux/interrupt.h>
#include <linux/netpoll.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/net_dropdump.h>
#include <linux/percpu.h>
#include <linux/timer.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <net/ip.h>
#include <net/netevent.h>

#include <trace/events/skb.h>
#include <trace/events/napi.h>

#include <asm/unaligned.h>

struct list_head ptype_log __read_mostly;

int netdev_support_dropdump = 1;
EXPORT_SYMBOL(netdev_support_dropdump);

DEFINE_RATELIMIT_STATE(dd_ratelimit_state, 10 * HZ, 5);
#define pr_drop(...)				\
do {						\
	if (__ratelimit(&dd_ratelimit_state))	\
		pr_info(__VA_ARGS__);		\
} while (0)

static inline int deliver_skb(struct sk_buff *skb,
			      struct packet_type *pt_prev,
			      struct net_device *orig_dev)
{
	if (unlikely(skb_orphan_frags_rx(skb, GFP_ATOMIC)))
		return -ENOMEM;
	refcount_inc(&skb->users);
	return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
}

static inline void net_timestamp_set(struct sk_buff *skb)
{
	/* we always need timestamp for dropdump */
	__net_timestamp(skb);
}

static void dropdump_print_skb(struct sk_buff *skb)
{
	if (!(skb->dropmask & PACKET_NOKLOG)) {
		struct iphdr *ip4hdr = (struct iphdr *)skb_network_header(skb);

		if (ip4hdr->version == 4) {
			pr_drop("[%u.%s] src:%pI4 | dst:%pI4 | %*ph\n",
				skb->dropid, dropdump_drop_str[skb->dropid], 
				&ip4hdr->saddr, &ip4hdr->daddr, 48, ip4hdr);
		} else if (ip4hdr->version == 6) {
			struct ipv6hdr *ip6hdr = (struct ipv6hdr *)ip4hdr;
			pr_drop("[%u.%s] src:%pI6c | dst:%pI6c | %*ph\n",
				skb->dropid, dropdump_drop_str[skb->dropid], 
				&ip6hdr->saddr, &ip6hdr->daddr, 48, ip6hdr);
		}
	}
}

#define invalid_dev() { \
	pr_drop("invalid dev <%pF><%pF>\n", \
	__builtin_return_address(2), __builtin_return_address(3)); \
	return false; \
}

bool dropdump_queue_skb(struct sk_buff *skb, int mode)
{
	struct packet_type *ptype;
	struct packet_type *pt_prev = NULL;
	struct sk_buff *skb2 = NULL;
	struct list_head *ptype_list = &ptype_log;
	struct net_device *dev = NULL;
	struct net_device *old_dev;
	struct iphdr *iphdr = (struct iphdr *)(skb_network_header(skb));
	struct ipv6hdr *ip6hdr = (struct ipv6hdr *)iphdr;
	bool ret = false;
	unsigned int len = 0;
	static ktime_t tstamp_bck;

	old_dev = skb->dev;
	if (skb->dev)
		dev = skb->dev;
	else if (skb_dst(skb) && skb_dst(skb)->dev)
		dev = skb_dst(skb)->dev;
	else
		dev = init_net.loopback_dev;

	/* final check */
	if (!virt_addr_valid(dev))
		invalid_dev();

	if (dev->name[0] == 'r') // rmnet
		goto queueing;
	if (dev->name[0] == 'w') // wlan
		goto queueing;
	if (dev->name[0] == 'v') // v4-rmnet
		goto queueing;
	if (dev->name[0] == 'l') // lo
		goto queueing;
	if (dev->name[0] == 's') // swlan
		goto queueing;
	if (dev->name[0] == 't') // tun
		goto queueing;

	invalid_dev();

queueing:
	rcu_read_lock_bh();

	skb->dev = dev;
	skb->dropmask |= PACKET_LOGED;

	if (iphdr->version == 4) {
		len = ntohs(iphdr->tot_len);
	} else if (iphdr->version == 6) {
		len = skb_network_header_len(skb) + ntohs(ip6hdr->payload_len);
	} else {
		pr_drop("%s: invalid ip data\n", __func__);
		return false;
	}

	list_for_each_entry_rcu(ptype, ptype_list, list) {
		if (pt_prev) {
			deliver_skb(skb2, pt_prev, dev);
			pt_prev = ptype;
			continue;
		}

		if (mode) {
			len = min(0x80u, len);
			tstamp_bck = skb->tstamp;
		}

		skb2 = netdev_alloc_skb(dev, len);
		if (unlikely(!skb2)) {
			pr_drop("%s: alloc fail\n", __func__);
			goto out_unlock;
		}

		skb2->protocol		= skb->protocol;
		skb2->tstamp		= mode ? skb->tstamp : tstamp_bck;
		skb2->pkt_type		= skb->dropmask & PACKET_IN ?
					  PACKET_HOST : PACKET_OUTGOING;

		skb2->dropmask		= skb->dropmask;
		skb2->dropid		= skb->dropid;
		

		skb_put(skb2, len);
		skb_reset_mac_header(skb2);
		skb_reset_network_header(skb2);
		skb_set_transport_header(skb2, skb_network_header_len(skb));

		memcpy(skb_network_header(skb2), skb_network_header(skb), len);

		iphdr = (struct iphdr *)(skb_network_header(skb2));
		if (iphdr->version == 4) {
			iphdr->ttl = (u8)skb->dropid;
		} else if (iphdr->version == 6) {
			ip6hdr = (struct ipv6hdr *)iphdr;
			ip6hdr->hop_limit = (u8)skb->dropid;
		}

		pt_prev = ptype;
		ret = true;
	}

out_unlock:
	if (pt_prev) {
		if (!skb_orphan_frags_rx(skb2, GFP_ATOMIC))
			pt_prev->func(skb2, skb->dev, pt_prev, skb->dev);
		else
			__kfree_skb(skb2);
	}

	skb->dev = old_dev;
	rcu_read_unlock_bh();

	return ret;
}

#include <linux/ptrace.h>
#include <linux/sched/task_stack.h>
#include <asm/stack_pointer.h>
#include <asm/stacktrace.h>
#include <asm/traps.h>

#define ST_MAX		32
#define ST_SIZE		0x30

int pr_stack(struct sk_buff *skb, char *st_str)
{
	struct task_struct *tsk = current;
	struct stackframe frame;
	unsigned long stack;
	int depth = 0, n;

	if (unlikely(!try_get_task_stack(tsk)))
		return -1;

	/* skip callstack under kfree_skb() */
	frame.fp = (unsigned long)__builtin_frame_address(3);
	frame.pc = (unsigned long)__builtin_frame_address(2);

	for (; depth < ST_MAX; depth++) {
		if (unlikely(unwind_frame(tsk, &frame) < 0))
			break;

		if (likely(in_entry_text(frame.pc))) {
			stack = frame.fp - offsetof(struct pt_regs, stackframe);
			if (unlikely(on_accessible_stack(tsk, stack)))
				break;
		}

		n = snprintf(st_str + ST_SIZE * depth, ST_SIZE, "%pF", (void *)(frame.pc));
		memset(st_str + ST_SIZE * depth + n, 0, ST_SIZE - n);
	}
	put_task_stack(tsk);

	return depth - 4; /* do not use root stacks */
}

struct sk_buff *get_dummy(struct sk_buff *skb)
{
	struct sk_buff *dummy = NULL;

	/* initialize dummy packet with ipv4 : iphdr + udp_hdr + align */
	unsigned int hdr_len, hdr_tot_len, hdr_align;
	unsigned int data_len, data_offset;
	struct udphdr *udph;

 	static char stack[NR_CPUS][ST_SIZE * ST_MAX];
	int st_depth = 1; // default value for tagging Error codename
	char *__stack = (char *)stack + (ST_SIZE * ST_MAX) * (unsigned long)get_cpu();
	put_cpu();

	if (skb->protocol == htons(ETH_P_IP)) {
		hdr_len   = sizeof(struct iphdr);
		hdr_align = 4;
	} else {
		hdr_len   = sizeof(struct ipv6hdr);
		hdr_align = 0;
	}
	hdr_tot_len = hdr_len + sizeof(struct udphdr);
	data_len    = hdr_tot_len + hdr_align;
	data_offset = data_len;

	/* trace callstack */
	st_depth += pr_stack(skb, __stack);
	if (st_depth > 0)
		data_len += ST_SIZE * st_depth;

	dummy = alloc_skb(data_len, GFP_ATOMIC);
	if (likely(dummy)) {
		/* set skb info */
		memset(dummy->data, 0, data_offset);
		dummy->dev              = skb->dev;
		dummy->hdr_len          = hdr_tot_len;
		dummy->len	        = data_len;
		dummy->transport_header = hdr_len;
		dummy->protocol         = skb->protocol;
		dummy->dropmask         = skb->dropmask | PACKET_DUMMY;
		dummy->dropid           = skb->dropid;

		/* set ip_hdr info */
		if (dummy->protocol == htons(ETH_P_IP)) {
			struct iphdr *iph = (struct iphdr *)dummy->data;
			memcpy(dummy->data, ip_hdr(skb), hdr_len);
			
			iph->protocol = 17; // UDP
			iph->ttl      = skb->dropid;
			iph->tot_len  = htons(data_len);
		} else {
			struct ipv6hdr *ip6h = (struct ipv6hdr *)dummy->data;
			memcpy(dummy->data, ipv6_hdr(skb), hdr_len);

			ip6h->nexthdr     = 17; // UDP
			ip6h->hop_limit   = skb->dropid;
			ip6h->payload_len = htons(data_len - hdr_len);
		}

		/* set udp_hdr info */
		udph = udp_hdr(dummy);
		udph->len = htons(data_len - hdr_len);

		/* copy error code string */
		memset(dummy->data + data_offset, 0, ST_SIZE);
		memcpy(dummy->data + data_offset,
			(void *)dropdump_drop_str[dummy->dropid],
			strlen(dropdump_drop_str[dummy->dropid]));

		/* copy callstack info */
		memcpy(dummy->data + data_offset + ST_SIZE, __stack, ST_SIZE * (st_depth - 1));
	} else {
		pr_drop("fail to alloc dummy..\n");
	}

	return dummy;
}

int skb_validate(struct sk_buff *skb)
{
	/* how to more smart.. */
	struct iphdr *ip4hdr = (struct iphdr *)skb_network_header(skb);

	if (unlikely(!skb->dropmask))
		return -1;
	if (unlikely((ip4hdr->version != 4 && ip4hdr->version != 6)
			|| ip4hdr->id == 0x6b6b))
		return -1;
	if (unlikely(!skb->len))
		return -1;
	if (unlikely(skb->len > skb->tail))
		return -1;
	if (unlikely(skb->data < skb->head))
		return -1;
	if (unlikely(skb->tail > skb->end))
		return -1;
	if (unlikely(skb->dropmask & (PACKET_LOGED | PACKET_DUMMY)))
		return -1;

	return 0;
}

void dropdump_queue(struct sk_buff *skb)
{
	struct sk_buff *dmy;
	static bool logging = false;

	if (unlikely(skb_validate(skb)))
		return;

#if 0
	/* use another field to expand id? : qos(1 byte) */
	if (unlikely(skb->dropid >= NET_DROPDUMP_CUSTOM_END))
		return;
#endif

#if 1
	/* if TTL(drop id) field is 0, you can see RED wall at wireshark :p */
	if (!skb->dropid) {
		skb->dropid = NET_DROPDUMP_CUSTOM_END;
		skb->dropmask |= PACKET_NOKLOG;
	}
#endif

	dropdump_print_skb(skb);

	/* skip packet logging when support_dropdump set to 0 */
	if (unlikely(!netdev_support_dropdump))
		return;

	logging = dropdump_queue_skb(skb, 1);

	if (unlikely(logging)) {
		dmy = get_dummy(skb);
		if (likely(dmy)) {
			dropdump_queue_skb(dmy, 0);
			__kfree_skb(dmy);
		}
	}
}

static int __init init_net_drop_dump(void)
{
	INIT_LIST_HEAD(&ptype_log);

	return 0;
}

static void exit_net_drop_dump(void)
{
}

module_init(init_net_drop_dump);
module_exit(exit_net_drop_dump);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung dropdump module");
