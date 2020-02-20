/*
 * Copyright (C) 2019 Samsung Electronics.
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

#ifndef __MODEM_KLAT_H__
#define __MODEM_KLAT_H__

#include <linux/types.h>
#include <linux/inet.h>
#include "modem_prj.h"

#ifdef CONFIG_KLAT
/* for supporting max 4 rmnets (rmnet0, rmnet1 ...) */
#define KLAT_MAX_NDEV	4

struct klat {
	int	use[KLAT_MAX_NDEV];

	struct in_addr	xlat_v4_addrs[KLAT_MAX_NDEV];	/* CLAT -> ipv4_local_subnet */
	struct in6_addr	xlat_addrs[KLAT_MAX_NDEV];	/* CLAT -> ipv6_local_subnet */
	struct in6_addr	plat_subnet;			/* CLAT -> plat_subnet */

	struct net_device *tun_device[KLAT_MAX_NDEV];
};

extern struct klat klat_obj;

int klat_rx(struct sk_buff *skb, int ndev_index);
int klat_tx(struct sk_buff *skb, int ndev_index);

ssize_t klat_plat_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count);
ssize_t klat_addrs_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count);
ssize_t klat_v4_addrs_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count);
#else
static inline int klat_rx(struct sk_buff *skb, int ndev_index) { return 0; }
static inline int klat_tx(struct sk_buff *skb, int ndev_index) { return 0; }

static inline
ssize_t klat_plat_store(struct kobject *kobj,
			struct kobj_attribute *attr,
			const char *buf, size_t count) { return 0; }
static inline
ssize_t klat_addrs_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count) { return 0; }
static inline
ssize_t klat_v4_addrs_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count) { return 0; }
#endif /* CONFIG_KLAT */

#endif /*__MODEM_KLAT_H__*/

