/* x_tables module for setting the IPv4/IPv6 DSCP field, Version 1.8
 *
 * (C) 2002 by Harald Welte <laforge@netfilter.org>
 * based on ipt_FTOS.c (C) 2000 by Matthew G. Marsh <mgm@paktronix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * See RFC2474 for a description of the DSCP field within the IP Header.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_PRIO.h>

MODULE_AUTHOR("SAMSUNG Electronics, IPC team <@samsung.com>");
MODULE_DESCRIPTION("Xtables: PRIO modification");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_PRIO");
MODULE_ALIAS("ip6t_PRIO");

#define XT_PRIO_MAX	0xFFFFFFFF
#define HIGH_PRIO	0x10

static unsigned int
prio_tg(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct xt_PRIO_info *dinfo = par->targinfo;

	skb->priomark = dinfo->prio;

	return XT_CONTINUE;
}

static int prio_tg_check(const struct xt_tgchk_param *par)
{
	const struct xt_PRIO_info *info = par->targinfo;

	if (info->prio > XT_PRIO_MAX) {
		pr_info("prio %x out of range\n", info->prio);
		return -EDOM;
	}
	return 0;
}

static struct xt_target prio_tg_reg[] __read_mostly = {
	{
		.name		= "PRIO",
		.family		= NFPROTO_IPV4,
		.checkentry	= prio_tg_check,
		.target		= prio_tg,
		.targetsize	= sizeof(struct xt_PRIO_info),
		.table		= "mangle",
		.me		= THIS_MODULE,
	},
	{
		.name		= "PRIO",
		.family		= NFPROTO_IPV6,
		.checkentry	= prio_tg_check,
		.target		= prio_tg,
		.targetsize	= sizeof(struct xt_PRIO_info),
		.table		= "mangle",
		.me		= THIS_MODULE,
	},
};

static int __init prio_tg_init(void)
{
	return xt_register_targets(prio_tg_reg, ARRAY_SIZE(prio_tg_reg));
}

static void __exit prio_tg_exit(void)
{
	xt_unregister_targets(prio_tg_reg, ARRAY_SIZE(prio_tg_reg));
}

module_init(prio_tg_init);
module_exit(prio_tg_exit);
