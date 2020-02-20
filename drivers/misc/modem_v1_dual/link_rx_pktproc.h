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

#ifndef __LINK_RX_PKTPROC_H__
#define __LINK_RX_PKTPROC_H__

/* Debug */
/* #define PKTPROC_DEBUG */
/* #define PKTPROC_DEBUG_PKT */
#ifdef PKTPROC_DEBUG
#define pp_debug(fmt, ...) mif_info(fmt, ##__VA_ARGS__)
#else
#define pp_debug(fmt, ...) no_printk(fmt, ##__VA_ARGS__)
#endif

/* Numbers */
#define PKTPROC_MAX_QUEUE	4
#define PKTPROC_MAX_PACKET_SIZE	2048

/*
 * PktProc info region
 */
/* Queue info */
struct pktproc_q_info {
	u32 desc_base;
	u32 num_desc;
	u32 data_base;
	u32 fore_ptr;
	u32 rear_ptr;
} __packed;

/* Info for V1 */
struct pktproc_info_v1 {
	struct pktproc_q_info q_info;
} __packed;

/* Info for V2 */
struct pktproc_info_v2 {
	u32 control;
	struct pktproc_q_info q_info[PKTPROC_MAX_QUEUE];
} __packed;

/*
 * PktProc descriptor region
 */
/* RingBuf mode */
struct pktproc_desc_ringbuf {
	u32 data_addr;
	u32 information;
	u32 reserve1;
	u16 reserve2;
	u16 filter_result;
	u16 length;
	u8 channel_id;
	u8 reserve3;
	u8 status;
	u8 reserve4;
	u16 reserve5;
} __packed;

/* SktBuf mode */
struct pktproc_desc_sktbuf {
	u32 data_addr;
	u8 control;
	u8 reserved1;
	u8 status;
	u8 reserved2;
	u16 length;
	u16 filter_result;
	u16 information;
	u8 channel_id;
	u8 reserved3;
} __packed;

/* Statistics */
struct pktproc_statistics {
	u64 pass_cnt;
	u64 err_len;
	u64 err_chid;
	u64 err_addr;
	u64 err_nomem;
	u64 err_bm_nomem;
	u64 err_csum;
	u64 use_memcpy_cnt;
};

/* Logical view for each queue */
struct pktproc_queue {
	u32 q_idx;
	atomic_t active;
	spinlock_t lock;

	struct mem_link_device *mld;
	struct pktproc_adaptor *ppa;

	u32 *fore_ptr;	/* Pointer to fore_ptr of q_info. Increased AP when desc_mode is ringbuf mode */
	u32 *rear_ptr;	/* Pointer to rear_ptr of q_info. Increased CP when desc_mode is sktbuf mode */
	u32 done_ptr;	/* Follow rear_ptr when desc_mode is sktbuf mode */

	/* Pointer to info region by version */
	union {
		struct {
			struct pktproc_info_v1 *info_v1;
		};
		struct {
			struct pktproc_info_v2 *info_v2;
		};
	};
	struct pktproc_q_info *q_info;	/* Pointer to q_info of info_v */

	/* Pointer to desc region by addr mode */
	union {
		struct {
			struct pktproc_desc_ringbuf *desc_ringbuf;	/* RingBuf mode */
		};
		struct {
			struct pktproc_desc_sktbuf *desc_sktbuf;	/* SktBuf mode */
		};
	};
	u32 desc_size;

	/* Pointer to data buffer */
	u8 __iomem *data;
	u32 data_size;

	/* Buffer manager */
	struct mif_buff_mng *manager;	/* Pointer to buffer manager */
	bool use_memcpy;	/* memcpy mode on sktbuf mode */

	/* IRQ */
	int irq;

#ifdef CONFIG_LINK_DEVICE_NAPI
	/* NAPI */
	struct net_device netdev;
	struct napi_struct napi;
#endif

	/* Statistics */
	struct pktproc_statistics stat;

	/* Func */
	irqreturn_t (*irq_handler)(int irq, void *arg);
	int (*get_packet)(struct pktproc_queue *q, struct sk_buff **new_skb);
	int (*clean_rx_ring)(struct pktproc_queue *q, int budget, int *work_done);
	int (*alloc_rx_buf)(struct pktproc_queue *q);
};

/*
 * Descriptor structure mode
 * 0: RingBuf mode. Destination data address is decided by CP
 * 1: SktBuf mode. Destination data address is decided by AP
 */
enum pktproc_desc_mode {
	DESC_MODE_RINGBUF,
	DESC_MODE_SKTBUF,
	MAX_DESC_MODE
};

/*
 * PktProc version
 * 1: Single queue, ringbuf desc mode
 * 2: Multi queue, ringbuf/sktbuf desc mode with restriction
 * 3: Multi queue, dedicated interrupt for all queues, ringbuf/sktbuf desc mode
 */
enum pktproc_version {
	PKTPROC_V1 = 1,
	PKTPROC_V2,
	PKTPROC_V3,
	MAX_VERSION
};

/* PktProc adaptor */
struct pktproc_adaptor {
	bool support;	/* Is support PktProc feature? */
	enum pktproc_version version;	/* Version */

	u32 cp_base;		/* CP base address for pktproc */
	u32 info_rgn_offset;	/* Offset of info region */
	u32 info_rgn_size;	/* Size of info region */
	u32 desc_rgn_offset;	/* Offset of descriptor region */
	u32 desc_rgn_size;	/* Size of descriptor region */
	u32 data_rgn_offset;	/* Offset of data buffer region */
	u32 data_rgn_size;	/* Size of data buffer region */

	enum pktproc_desc_mode desc_mode;	/* Descriptor structure mode */
	u32 num_queue;		/* Number of queue */
	bool use_exclusive_irq;	/* Exclusive interrupt */
	bool use_hw_iocc;	/* H/W IO cache coherency */

	struct mif_buff_mng *manager;	/* Buffer manager */

	void __iomem *info_base;	/* Physical region for information */
	void __iomem *desc_base;	/* Physical region for descriptor */
	void __iomem *data_base;	/* Physical region for data buffer */
	struct pktproc_queue *q[PKTPROC_MAX_QUEUE];	/* Logical queue */
};

#if defined(CONFIG_CP_PKTPROC) || defined(CONFIG_CP_PKTPROC_V2)
extern int pktproc_create(struct platform_device *pdev, struct mem_link_device *mld, u32 memaddr, u32 memsize);
extern int pktproc_init(struct pktproc_adaptor *ppa);
static inline int pktproc_check_support(struct pktproc_adaptor *ppa)
{
	return ppa->support;
}
static inline int pktproc_check_active(struct pktproc_adaptor *ppa, u32 q_idx)
{
	if (!pktproc_check_support(ppa))
		return 0;

	if (!ppa->q[q_idx])
		return 0;

	return atomic_read(&ppa->q[q_idx]->active);
}
#else
static inline int pktproc_create(struct platform_device *pdev, struct mem_link_device *mld,
				u32 memaddr, u32 memsize) { return 0; }
static inline int pktproc_init(struct pktproc_adaptor *ppa) { return 0; }
static inline int pktproc_check_support(struct pktproc_adaptor *ppa) { return 0; }
static inline int pktproc_check_active(struct pktproc_adaptor *ppa, u32 q_idx) { return 0; }
#endif

#endif /* __LINK_RX_PKTPROC_H__ */
