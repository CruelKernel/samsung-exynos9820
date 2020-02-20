/*
 * Copyright (C) 2018-2019, Samsung Electronics.
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

#ifndef __LINK_RX_DIT_H__
#define __LINK_RX_DIT_H__

#ifdef CONFIG_LINK_FORWARD
#include <linux/linkforward.h>
#endif
#include "modem_utils.h"

/* Debug */
/* #define DIT_DEBUG */
/* #define DIT_DEBUG_PKT */
#ifdef DIT_DEBUG
#define dit_debug(fmt, ...) mif_info(fmt, ##__VA_ARGS__)
#else
#define dit_debug(fmt, ...) no_printk(fmt, ##__VA_ARGS__)
#endif

/* Bank */
#define BANK_SIZE_BIT			8
#define BANK_SIZE_UNITS			(1 << BANK_SIZE_BIT)	/* 256 */
#define UNITS_IN_BANK(bank_key)		((bank_key) & (BANK_SIZE_UNITS - 1))
#define BANK_ID(bank_key)		((bank_key) >> BANK_SIZE_BIT)
#define BANK_START_INDEX(bank_key)	(BANK_ID(bank_key) << BANK_SIZE_BIT)

/* DIT descriptor */
struct dit_desc {
	u64 src_addr;
	u64 dst_addr;
	u16 size;
	u16 reserve3;
	u8 status;
	u8 reserve1;
	u8 control;
	u8 reserve2;
} __packed;

/* IRQ */
struct dit_irq {
	int num;
	bool active;
	bool registered;
	spinlock_t lock;
};

/* Statistics */
struct dit_statistics {
	u32 pkt_cnt;
	u32 kick_cnt;
	u32 err_trans;
	u32 err_nomem;
	u32 err_descfull;
	u32 err_bankfull;
	u32 err_pendig;
	u32 err_status;
	u32 err_busypoll;
};

/* DIT adaptor */
struct dit_adaptor {
	bool support;

	u8 version;

	atomic_t ready;

	spinlock_t lock;
	struct mem_link_device *mld;

	/* RX */
	u32 num_desc_rx;
	struct dit_desc *desc_rx;	/* DIT RX descriptor */
	struct sk_buff **skb_rx;	/* DIT RX SKB */

	/* Register */
	void __iomem *reg_dit;
	void __iomem *reg_busc;

	/* IRQ */
	struct dit_irq irq_bus_err;
	struct dit_irq irq_rx;
	struct dit_irq irq_tx;

	dma_addr_t dma_base;

	/* Key */
	u32 w_key;	/* write key: (bank id << bank_size_bits) | bank units) */
	u32 r_key;	/* read key: (bank id << bank_size_bits) | bank units) */

	u32 busy;	/* To kick */

#ifdef CONFIG_LINK_DEVICE_NAPI
	/* NAPI */
	struct net_device netdev;
	struct napi_struct napi;
#endif

	/* Debug */
	int force_enable;

	/* Statistics */
	struct dit_statistics stat;
};

/* DIT control */
#define DIT_SW_COMMAND		0x0
#define DIT_CLK_GT_OFF		0x4
#define DIT_DMA_INIT_DATA	0x8
#define DIT_TX_DESC_CTRL	0xC
#define DIT_TX_HEAD_CTRL	0x10
#define DIT_TX_MOD_HD_CTRL	0x14
#define DIT_TX_PKT_CTRL		0x18
#define DIT_TX_CHKSUM_CTRL	0x1C
#define DIT_RX_DESC_CTRL	0x20
#define DIT_RX_HEAD_CTRL	0x24
#define DIT_RX_MOD_HD_CTRL	0x28
#define DIT_RX_PKT_CTRL		0x2C
#define DIT_RX_CHKSUM_CTRL	0x30
#define DIT_DMA_CHKSUM_OFF	0x34
#define DIT_SIZE_UPDATE_OFF	0x38
#define DIT_TX_RING_ST_ADDR_0	0x3C
#define DIT_TX_RING_ST_ADDR_1	0x40
#define DIT_RX_RING_ST_ADDR_0	0x44
#define DIT_RX_RING_ST_ADDR_1	0x48
#define DIT_INT_ENABLE		0x4C
#define DIT_INT_MASK		0x50
#define DIT_INT_PENDING		0x54
#define DIT_STATUS		0x58
#define DIT_BUS_ERROR		0x5C

/* CLAT filter */
#define DIT_CLAT_TX_FILTER_A		0x2000
#define DIT_CLAT_TX_PLAT_PREFIX_A0	0x2008
#define DIT_CLAT_TX_PLAT_PREFIX_A1	0x200C
#define DIT_CLAT_TX_PLAT_PREFIX_A2	0x2010
#define DIT_CLAT_TX_CLAT_SRC_A0		0x2020
#define DIT_CLAT_TX_CLAT_SRC_A1		0x2024
#define DIT_CLAT_TX_CLAT_SRC_A2		0x2028
#define DIT_CLAT_TX_CLAT_SRC_A3		0x202C

/* NAT filter */
#define DIT_NAT_TX_DESC_ADDR_0		0x4000
#define DIT_NAT_TX_DESC_ADDR_1		0x4004
#define DIT_NAT_RX_DESC_ADDR_0		0x4008
#define DIT_NAT_RX_DESC_ADDR_1		0x400C
#define DIT_NAT_PROTOCOL_FILTER_0	0x4010
#define DIT_NAT_SA_FILTER_0		0x4024
#define DIT_NAT_DP_FILTER_0		0x4074
#define DIT_NAT_SP_FILTER_0		0x409C
#define DIT_NAT_LOCAL_ADDR		0x40C4
#define DIT_NAT_UE_ADDR			0x40C8
#define DIT_NAT_ZERO_CHK_OFF		0x40CC

/* Function */
#if defined(CONFIG_CP_DIT)
extern int dit_create(struct platform_device *pdev, struct mem_link_device *mld);
extern int dit_init(struct dit_adaptor *dit);
extern int pass_skb_to_dit(struct dit_adaptor *dit, struct sk_buff *skb);
extern int dit_kick(struct dit_adaptor *dit);

static inline int dit_enable_irq(struct dit_irq *irq)
{
	unsigned long flags;

	spin_lock_irqsave(&irq->lock, flags);

	if (irq->active) {
		spin_unlock_irqrestore(&irq->lock, flags);
		return 0;
	}

	enable_irq(irq->num);
	irq->active = 1;

	spin_unlock_irqrestore(&irq->lock, flags);

	return 0;
}

static inline int dit_disable_irq(struct dit_irq *irq)
{
	unsigned long flags;

	spin_lock_irqsave(&irq->lock, flags);

	if (!irq->active) {
		spin_unlock_irqrestore(&irq->lock, flags);
		return 0;
	}

	disable_irq_nosync(irq->num);
	irq->active = 0;

	spin_unlock_irqrestore(&irq->lock, flags);

	return 0;
}

static inline int dit_check_support(struct dit_adaptor *dit)
{
	return dit->support;
}
static inline int dit_check_ready(struct dit_adaptor *dit)
{
	return atomic_read(&dit->ready);
}
static inline int dit_check_active(struct dit_adaptor *dit)
{
	int enable = 0;

	if (!dit_check_support(dit) || !dit_check_ready(dit))
		return 0;

#ifdef CONFIG_LINK_FORWARD
	enable |= linkforward_get_tether_mode();
#endif
	enable |= dit->force_enable;

	return enable;
}

static inline int dit_check_busy(struct dit_adaptor *dit)
{
	if (!dit_check_support(dit) || !dit_check_ready(dit))
		return 0;

	if (!dit->busy)
		return 0;

	if (UNITS_IN_BANK(dit->w_key)) {
		dit->stat.err_busypoll++;
		return 1;
	}

	dit->busy = 0;

	return 0;
}
static inline int dit_check_kick(struct dit_adaptor *dit, int unit_cnt)
{
	if (!dit_check_support(dit) || !dit_check_ready(dit))
		return 0;

	if ((UNITS_IN_BANK(dit->w_key) >= unit_cnt) && !circ_full(BANK_ID(dit->num_desc_rx), BANK_ID(dit->w_key), BANK_ID(dit->r_key)))
		return dit_kick(dit);

	return 0;
}

extern void dit_set_nat_local_addr(u32 addr);
extern void dit_set_nat_filter(u32 id, u8 proto, u32 sa, u16 sp, u16 dp);
extern void dit_del_nat_filter(u32 id);
extern void dit_set_clat_plat_prfix(u32 id, struct in6_addr addr);
extern void dit_del_clat_plat_prfix(u32 id);
extern void dit_set_clat_saddr(u32 id, struct in6_addr addr);
extern void dit_del_clat_saddr(u32 id);
extern void dit_set_clat_filter(u32 id, u32 addr);
extern void dit_del_clat_filter(u32 id);
#else
static inline int dit_create(struct platform_device *pdev, struct mem_link_device *mld) { return 0; }
static inline int dit_init(struct dit_adaptor *dit) { return 0; }
static inline int pass_skb_to_dit(struct dit_adaptor *dit, struct sk_buff *skb) { return 0; }
static inline int dit_kick(struct dit_adaptor *dit) { return 0; }

static inline int dit_check_support(struct dit_adaptor *dit) { return 0; }
static inline int dit_check_ready(struct dit_adaptor *dit) { return 0; }
static inline int dit_check_active(struct dit_adaptor *dit) { return 0; };

static inline int dit_check_busy(struct dit_adaptor *dit) { return 0; };
static inline int dit_check_kick(struct dit_adaptor *dit, int unit_cnt) { return 0; };

static inline void dit_set_nat_local_addr(u32 addr) { return; }
static inline void dit_set_nat_filter(u32 id, u8 proto, u32 sa, u16 sp, u16 dp) { return; }
static inline void dit_del_nat_filter(u32 id) { return; }
static inline void dit_set_clat_plat_prfix(u32 id, struct in6_addr addr) { return; }
static inline void dit_del_clat_plat_prfix(u32 id) { return; }
static inline void dit_set_clat_saddr(u32 id, struct in6_addr addr) { return; }
static inline void dit_del_clat_saddr(u32 id) { return; }
static inline void dit_set_clat_filter(u32 id, u32 addr) { return; }
static inline void dit_del_clat_filter(u32 id) { return; }
#endif

#endif /* __LINK_RX_DIT_H__ */
