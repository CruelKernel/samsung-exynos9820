/*
 * Copyright (C) 2018 Samsung Electronics.
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

#include <asm/cacheflush.h>

#define ENABLE_DIT_ON_TETHERING
/* #define DIT_USE_RXTX_DONE_INT */
/* #define DIT_CLOCK_ALWAYS_ON */

/* #define DIT_DEBUG */
/* #define DIT_DEBUG_PKT */

#ifdef DIT_DEBUG
#define dit_debug(fmt, ...) mif_info(fmt, ##__VA_ARGS__)
#else
#define dit_debug(fmt, ...) no_printk(fmt, ##__VA_ARGS__)
#endif

/* DIT packet descriptor */
#define MAX_DIT_SKB_NUM		(1024)
#define DEFAULT_SKB_SIZE	(2048)

struct packet_descriptor {
	u64 src_addr;
	u64 dst_addr;
	u16 size;
	u16 reserve3;
	u8 status;
	u8 reserve1;
	u8 control;
	u8 reserve2;
} __packed;

/* DIT device */
struct dit_dev_info {
	void __iomem *reg_base;
	void __iomem *busc_reg_base;
	dma_addr_t dma;

	int bus_err_int;
	int rx_done_int;
	int tx_done_int;

	struct device *dev;
	spinlock_t lock;

	int forced_enable;

	u32 pkt_on_pktproc_cnt;
	u32 pkt_on_zmb_cnt;

#if defined(CONFIG_CP_ZEROCOPY)
	u8 **zmb_buff;
#endif
};

/******************************************************************************
 * DIT control
 *****************************************************************************/
#define DIT_SW_COMMAND		0x0
#define DIT_CLK_GT_OFF		0x4
#define DIT_DMA_INIT_DATA	0x8

/* TX control */
#define DIT_TX_DESC_CTRL	0xC
#define DIT_TX_HEAD_CTRL	0x10
#define DIT_TX_MOD_HD_CTRL	0x14
#define DIT_TX_PKT_CTRL		0x18
#define DIT_TX_CHKSUM_CTRL	0x1C

/* RX control */
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

/******************************************************************************
 * CLAT
 *****************************************************************************/
/*
 * TCP filter
 */
#define DIT_CLAT_TX_FILTER_A	0x2000
#define DIT_CLAT_TX_FILTER_B	0x2004
/*
 * PLAT prefix and CLAT local address
 * IPv4 to IPv6 for TX when packet header is matched with filter
 */
#define DIT_CLAT_TX_PLAT_PREFIX_A0	0x2008
#define DIT_CLAT_TX_PLAT_PREFIX_A1	0x200C
#define DIT_CLAT_TX_PLAT_PREFIX_A2	0x2010
#define DIT_CLAT_TX_PLAT_PREFIX_B0	0x2014
#define DIT_CLAT_TX_PLAT_PREFIX_B1	0x2018
#define DIT_CLAT_TX_PLAT_PREFIX_B2	0x201C
/*
 * CLAT source address
 * IPv6 to IPv4 for RX when packet header is matched with filter
 * calculate IP header checksum
 */
#define DIT_CLAT_TX_CLAT_SRC_A0	0x2020
#define DIT_CLAT_TX_CLAT_SRC_A1	0x2024
#define DIT_CLAT_TX_CLAT_SRC_A2	0x2028
#define DIT_CLAT_TX_CLAT_SRC_A3	0x202C
#define DIT_CLAT_TX_CLAT_SRC_B0	0x2030
#define DIT_CLAT_TX_CLAT_SRC_B1	0x2034
#define DIT_CLAT_TX_CLAT_SRC_B2	0x2038
#define DIT_CLAT_TX_CLAT_SRC_B3	0x203C

/******************************************************************************
 * NAT
 *****************************************************************************/
#define DIT_NAT_TX_DESC_ADDR_0		0x4000
#define DIT_NAT_TX_DESC_ADDR_1		0x4004
#define DIT_NAT_RX_DESC_ADDR_0		0x4008
#define DIT_NAT_RX_DESC_ADDR_1		0x400C

/*
 * Protocol filter for NAT
 */
#define DIT_NAT_PROTOCOL_FILTER_0	0x4010
#define DIT_NAT_PROTOCOL_FILTER_1	0x4014
#define DIT_NAT_PROTOCOL_FILTER_2	0x4018
#define DIT_NAT_PROTOCOL_FILTER_3	0x401C
#define DIT_NAT_PROTOCOL_FILTER_4	0x4020

/*
 * Source address filter for NAT
 */
#define DIT_NAT_SA_FILTER_0		0x4024
#define DIT_NAT_SA_FILTER_1		0x4028
#define DIT_NAT_SA_FILTER_2		0x402C
#define DIT_NAT_SA_FILTER_3		0x4030
#define DIT_NAT_SA_FILTER_4		0x4034
#define DIT_NAT_SA_FILTER_5		0x4038
#define DIT_NAT_SA_FILTER_6		0x403C
#define DIT_NAT_SA_FILTER_7		0x4040
#define DIT_NAT_SA_FILTER_8		0x4044
#define DIT_NAT_SA_FILTER_9		0x4048
#define DIT_NAT_SA_FILTER_10		0x404C
#define DIT_NAT_SA_FILTER_11		0x4050
#define DIT_NAT_SA_FILTER_12		0x4054
#define DIT_NAT_SA_FILTER_13		0x4058
#define DIT_NAT_SA_FILTER_14		0x405C
#define DIT_NAT_SA_FILTER_15		0x4060
#define DIT_NAT_SA_FILTER_16		0x4064
#define DIT_NAT_SA_FILTER_17		0x4068
#define DIT_NAT_SA_FILTER_18		0x406C
#define DIT_NAT_SA_FILTER_19		0x4070

#define DIT_NAT_DP_FILTER_0		0x4074
#define DIT_NAT_DP_FILTER_1		0x4078
#define DIT_NAT_DP_FILTER_2		0x407C
#define DIT_NAT_DP_FILTER_3		0x4080
#define DIT_NAT_DP_FILTER_4		0x4084
#define DIT_NAT_DP_FILTER_5		0x4088
#define DIT_NAT_DP_FILTER_6		0x408C
#define DIT_NAT_DP_FILTER_7		0x4090
#define DIT_NAT_DP_FILTER_8		0x4094
#define DIT_NAT_DP_FILTER_9		0x4098

#define DIT_NAT_SP_FILTER_0		0x409C
#define DIT_NAT_SP_FILTER_1		0x40A0
#define DIT_NAT_SP_FILTER_2		0x40A4
#define DIT_NAT_SP_FILTER_3		0x40A8
#define DIT_NAT_SP_FILTER_4		0x40AC
#define DIT_NAT_SP_FILTER_5		0x40B0
#define DIT_NAT_SP_FILTER_6		0x40B4
#define DIT_NAT_SP_FILTER_7		0x40B8
#define DIT_NAT_SP_FILTER_8		0x40BC
#define DIT_NAT_SP_FILTER_9		0x40C0

#define DIT_NAT_LOCAL_ADDR		0x40C4
#define DIT_NAT_UE_ADDR			0x40C8
#define DIT_NAT_ZERO_CHK_OFF		0x40CC

/******************************************************************************
 *
 *****************************************************************************/
#if defined(CONFIG_CP_DIT)
extern struct dit_dev_info dit_dev;

extern int dit_init(struct platform_device *pdev);
extern void dit_init_hw(void);
extern int pio_rx_dit_on_pktproc(struct link_device *ld, u32 num_frames);
extern int sbd_pio_rx_dit_on_zerocopy(struct sbd_ring_buffer *rb, u32 num_frames);
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
static inline int dit_init(struct platform_device *pdev) { return 0; }
static inline void dit_init_hw(void) { return; }
static inline int pio_rx_dit_on_pktproc(struct link_device *ld, u32 num_frames) { return 0; }
extern inline int sbd_pio_rx_dit_on_zerocopy(struct sbd_ring_buffer *rb, u32 num_frames) { return 0; }
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
