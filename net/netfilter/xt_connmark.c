/*
 *	xt_connmark - Netfilter module to operate on connection marks
 *
 *	Copyright (C) 2002,2004 MARA Systems AB <http://www.marasystems.com>
 *	by Henrik Nordstrom <hno@marasystems.com>
 *	Copyright © CC Computer Consultants GmbH, 2007 - 2008
 *	Jan Engelhardt <jengelh@medozas.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_connmark.h>

// ------------- START of KNOX_VPN ------------------//
#include <linux/types.h>
#include <linux/tcp.h>
#include <linux/ip.h>
#include <net/ip.h>
// ------------- END of KNOX_VPN -------------------//

MODULE_AUTHOR("Henrik Nordstrom <hno@marasystems.com>");
MODULE_DESCRIPTION("Xtables: connection mark operations");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_CONNMARK");
MODULE_ALIAS("ip6t_CONNMARK");
MODULE_ALIAS("ipt_connmark");
MODULE_ALIAS("ip6t_connmark");

// ------------- START of KNOX_VPN ------------------//
/* KNOX framework uses mark value 100 to 500
 * when the special meta data is added
 * This will indicate to the kernel code that
 * it needs to append meta data to the packets
 */

#define META_MARK_BASE_LOWER 100
#define META_MARK_BASE_UPPER 500

/* Structure to hold metadata values
 * intended for VPN clients to make
 * more intelligent decisions
 * when the KNOX meta mark
 * feature is enabled
 */

struct knox_meta_param {
    uid_t uid;
    pid_t pid;
};

static unsigned int knoxvpn_uidpid(struct sk_buff *skb, u_int32_t newmark)
{
	int szMetaData;
	struct skb_shared_info *knox_shinfo = NULL;

	szMetaData = sizeof(struct knox_meta_param);
	if (skb != NULL) {
		knox_shinfo = skb_shinfo(skb);
	} else {
		pr_err("KNOX: NULL SKB - no KNOX processing");
		return -1;
	}

	if( skb->sk == NULL) {
		pr_err("KNOX: skb->sk value is null");
		return -1;
	}

	if( knox_shinfo == NULL) {
		pr_err("KNOX: knox_shinfo is null");
		return -1;
	}

	if (newmark < META_MARK_BASE_LOWER || newmark > META_MARK_BASE_UPPER) {
		pr_err("KNOX: The mark is out of range");
		return -1;
	} else {
		knox_shinfo->uid = skb->sk->knox_uid;
		knox_shinfo->pid = skb->sk->knox_pid;
		knox_shinfo->knox_mark = newmark;
	}

	return 0;
}
// ------------- END of KNOX_VPN -------------------//

static unsigned int
connmark_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_connmark_tginfo1 *info = par->targinfo;
	enum ip_conntrack_info ctinfo;
	struct nf_conn *ct;
	u_int32_t newmark;

	ct = nf_ct_get(skb, &ctinfo);
	if (ct == NULL)
		return XT_CONTINUE;

	switch (info->mode) {
	case XT_CONNMARK_SET:
		newmark = (ct->mark & ~info->ctmask) ^ info->ctmark;
		if (ct->mark != newmark) {
			ct->mark = newmark;
			nf_conntrack_event_cache(IPCT_MARK, ct);
		}
		break;
	case XT_CONNMARK_SAVE:
		newmark = (ct->mark & ~info->ctmask) ^
		          (skb->mark & info->nfmask);
		if (ct->mark != newmark) {
			ct->mark = newmark;
			nf_conntrack_event_cache(IPCT_MARK, ct);
		}
		break;
	case XT_CONNMARK_RESTORE:
		newmark = (skb->mark & ~info->nfmask) ^
		          (ct->mark & info->ctmask);
		skb->mark = newmark;
		// ------------- START of KNOX_VPN -----------------//
		knoxvpn_uidpid(skb, newmark);
		// ------------- END of KNOX_VPN -------------------//
		break;
	}

	return XT_CONTINUE;
}

static int connmark_tg_check(const struct xt_tgchk_param *par)
{
	int ret;

	ret = nf_ct_netns_get(par->net, par->family);
	if (ret < 0)
		pr_info("cannot load conntrack support for proto=%u\n",
			par->family);
	return ret;
}

static void connmark_tg_destroy(const struct xt_tgdtor_param *par)
{
	nf_ct_netns_put(par->net, par->family);
}

static bool
connmark_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_connmark_mtinfo1 *info = par->matchinfo;
	enum ip_conntrack_info ctinfo;
	const struct nf_conn *ct;

	ct = nf_ct_get(skb, &ctinfo);
	if (ct == NULL)
		return false;

	return ((ct->mark & info->mask) == info->mark) ^ info->invert;
}

static int connmark_mt_check(const struct xt_mtchk_param *par)
{
	int ret;

	ret = nf_ct_netns_get(par->net, par->family);
	if (ret < 0)
		pr_info("cannot load conntrack support for proto=%u\n",
			par->family);
	return ret;
}

static void connmark_mt_destroy(const struct xt_mtdtor_param *par)
{
	nf_ct_netns_put(par->net, par->family);
}

static struct xt_target connmark_tg_reg __read_mostly = {
	.name           = "CONNMARK",
	.revision       = 1,
	.family         = NFPROTO_UNSPEC,
	.checkentry     = connmark_tg_check,
	.target         = connmark_tg,
	.targetsize     = sizeof(struct xt_connmark_tginfo1),
	.destroy        = connmark_tg_destroy,
	.me             = THIS_MODULE,
};

static struct xt_match connmark_mt_reg __read_mostly = {
	.name           = "connmark",
	.revision       = 1,
	.family         = NFPROTO_UNSPEC,
	.checkentry     = connmark_mt_check,
	.match          = connmark_mt,
	.matchsize      = sizeof(struct xt_connmark_mtinfo1),
	.destroy        = connmark_mt_destroy,
	.me             = THIS_MODULE,
};

static int __init connmark_mt_init(void)
{
	int ret;

	ret = xt_register_target(&connmark_tg_reg);
	if (ret < 0)
		return ret;
	ret = xt_register_match(&connmark_mt_reg);
	if (ret < 0) {
		xt_unregister_target(&connmark_tg_reg);
		return ret;
	}
	return 0;
}

static void __exit connmark_mt_exit(void)
{
	xt_unregister_match(&connmark_mt_reg);
	xt_unregister_target(&connmark_tg_reg);
}

module_init(connmark_mt_init);
module_exit(connmark_mt_exit);
