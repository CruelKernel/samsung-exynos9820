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

#ifndef __LINK_RX_PKTPROC_H__
#define __LINK_RX_PKTPROC_H__

/* #define PKTPROC_DEBUG */

#define MAX_PKTPROC_DESC_NUM	(8192)
#define MAX_PKTPROC_SKB_SIZE	(2048)

#define PKTPROC_CP_BASE_ADDR_MASK	0x50000000
#define PKTPROC_DESC_BASE_OFFSET	0x00000100
#define PKTPROC_DATA_BASE_OFFSET	0x00100000

/* pktproc global descriptor */
struct pktproc_global_desc {
	u32 desc_base_addr;
	u32 num_of_desc;
	u32 data_base_addr;
	u32 fore_ptr;
	u32 rear_ptr;
} __packed;

/* pktproc descriptor */
struct pktproc_desc {
	u32 data_addr;
	u32 information;
	u32 reserve1;
	u16 reserve2;
	u16 filter_result;
	u16 length;
	u8 channel_id;
	u8 reserve3;
	u32 reserve4;
} __packed;

extern struct pktproc_global_desc *pktproc_gdesc;
extern struct pktproc_desc *pktproc_desc;
extern u8 __iomem *pktproc_data;

extern int setup_pktproc(struct platform_device *pdev);
extern int pktproc_pio_rx(struct link_device *ld);

#endif /* __LINK_RX_PKTPROC_H__ */
