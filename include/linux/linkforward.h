/* Copyright (C) 2015 Samsung Electronics.
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
#ifdef CONFIG_LINK_FORWARD
#ifndef __LINKFORWARD_H__
#define __LINKFORWARD_H__

#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_l3proto.h>
#include <net/netfilter/nf_nat_l4proto.h>

#define SIGNATURE_LINK_FORWRD_REPLY	0xD01070B0
#define SIGNATURE_LINK_FORWRD_ORIGN	0xD01070A0
#define SIGNATURE_LINK_FORWRD_MASK	0xD0107000

#define CHECK_LINK_FORWARD(x)		(((x) & 0xFFFFFF00) == SIGNATURE_LINK_FORWRD_MASK)

#define THRESHOLD_LINK_FORWARD	30  /* Threshold for linkforward */

#define LINK_CLAT_DEV_MAX	2

enum linkforward_dir {
	LINK_FORWARD_DIR_ORIGN,
	LINK_FORWARD_DIR_REPLY,
	LINK_FORWARD_DIR_MAX
};

struct forward_bridge {
	struct net_device	*inner;
	struct net_device	*outter;
	char	if_mac[ETH_ALEN];	/* inner interface device mac address */
	char	ldev_mac[ETH_ALEN];	/* inner linked/connected ethernet devices mac */
};

struct tunnel_if {
	struct net_device *dev;
	union {
		struct in6_addr		addr6;
		struct in_addr		addr4;
	} in;
};

struct clat_tunnel {
	int enable;
	struct tunnel_if v6;
	struct tunnel_if v4;
};

struct nf_linkforward {
	int	use;
	int	mode;
	atomic_t	qmask[LINK_FORWARD_DIR_MAX];
	struct Qdisc	*q[LINK_FORWARD_DIR_MAX];
	struct forward_bridge	brdg;

	struct in6_addr		plat_prfix; /* the plat /96 prefix */
	struct clat_tunnel	ctun[LINK_CLAT_DEV_MAX];
};

extern struct nf_linkforward nf_linkfwd;

static inline int get_linkforward_mode(void)
{
	return nf_linkfwd.mode;
}

void set_linkforward_mode(int mode);

void linkforward_enable(void);
void linkforward_disable(void);

void clat_enable_dev(char *dev_name);
void clat_disable_dev(char *dev_name);

void linkforward_table_init(void);
ssize_t linkforward_get_state(char *buf);


int linkforward_add(__be16 src_port, struct nf_conntrack_tuple *t_org,
		struct nf_conntrack_tuple *t_rpl, struct net_device *netdev);
int linkforward_delete(__be16 dst_port);

static inline struct net_device *get_linkforwd_inner_dev(void)
{
	return nf_linkfwd.brdg.inner;
}

static inline struct net_device *get_linkforwd_outter_dev(void)
{
	return nf_linkfwd.brdg.outter;
}

static inline void set_linkforwd_inner_dev(struct net_device *dev)
{
	nf_linkfwd.brdg.inner = dev;
}

static inline void set_linkforwd_outter_dev(struct net_device *dev)
{
	nf_linkfwd.brdg.outter = dev;
}

int __linkforward_manip_skb(struct sk_buff *skb, enum linkforward_dir dir);

static inline int linkforward_manip_skb(struct sk_buff *skb, enum linkforward_dir dir)
{
	if (!(nf_linkfwd.use && get_linkforwd_inner_dev() && get_linkforwd_outter_dev())) {
		/* check is_tethering_enabled */
		return 0;
	}
	return __linkforward_manip_skb(skb, dir);
}

static inline int linkforward_get_tether_mode(void)
{
	if (!(nf_linkfwd.use && get_linkforwd_inner_dev() && get_linkforwd_outter_dev())) {
		/* check is_tethering_enabled */
		return 0;
	}

	return 1;
}

void linkforward_init(void);

/* Device core functions */
int dev_linkforward_queue_xmit(struct sk_buff *skb);

/* Netfilter linkforward API */
int nf_linkforward_add(struct nf_conn *ct);
int nf_linkforward_delete(struct nf_conn *ct);
void nf_linkforward_monitor(struct nf_conn *ct);

/* extern function call */
extern void gether_get_host_addr_u8(struct net_device *net, u8 host_mac[ETH_ALEN]);

#if defined(CONFIG_CP_DIT)
extern void dit_set_nat_local_addr(u32 addr);
extern void dit_set_nat_filter(u32 id, u8 proto, u32 sa, u16 sp, u16 dp);
extern void dit_del_nat_filter(u32 id);
#endif
#endif /*__LINKFORWARD_H__*/
#endif
