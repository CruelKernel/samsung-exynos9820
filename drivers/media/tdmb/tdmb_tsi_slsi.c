/* drivers/media/tdmb/tdmb_tsi.c
 *
 * Driver file for Samsung Transport Stream Interface
 *
 *  Copyright (c) 2009 Samsung Electronics
 *	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/pm_qos.h>
#include <linux/pinctrl/consumer.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include <mach/map.h>
#include <mach/regs-clock.h>
#include "tdmb.h"

#define EXYNOS5_TSIREG(x)			(x)

#define	EXYNOS5_TS_CLKCON			EXYNOS5_TSIREG(0x00)
#define	EXYNOS5_TS_CON			EXYNOS5_TSIREG(0x04)
#define	EXYNOS5_TS_SYNC			EXYNOS5_TSIREG(0x08)
#define	EXYNOS5_TS_CNT			EXYNOS5_TSIREG(0x0C)
#define	EXYNOS5_TS_BASE			EXYNOS5_TSIREG(0x10)
#define	EXYNOS5_TS_SIZE			EXYNOS5_TSIREG(0x14)
#define	EXYNOS5_TS_CADDR			EXYNOS5_TSIREG(0x18)
#define	EXYNOS5_TS_INTMASK			EXYNOS5_TSIREG(0x1C)
#define	EXYNOS5_TS_INT			EXYNOS5_TSIREG(0x20)
#define	EXYNOS5_TS_PID0			EXYNOS5_TSIREG(0x24)
#define	EXYNOS5_TS_PID1			EXYNOS5_TSIREG(0x28)
#define	EXYNOS5_TS_PID2			EXYNOS5_TSIREG(0x2C)
#define	EXYNOS5_TS_PID3			EXYNOS5_TSIREG(0x30)
#define	EXYNOS5_TS_PID4			EXYNOS5_TSIREG(0x34)
#define	EXYNOS5_TS_PID5			EXYNOS5_TSIREG(0x38)
#define	EXYNOS5_TS_PID6			EXYNOS5_TSIREG(0x3C)
#define	EXYNOS5_TS_PID7			EXYNOS5_TSIREG(0x40)
#define	EXYNOS5_TS_PID8			EXYNOS5_TSIREG(0x44)
#define	EXYNOS5_TS_PID9			EXYNOS5_TSIREG(0x48)
#define	EXYNOS5_TS_PID10			EXYNOS5_TSIREG(0x4C)
#define	EXYNOS5_TS_PID11			EXYNOS5_TSIREG(0x50)
#define	EXYNOS5_TS_PID12			EXYNOS5_TSIREG(0x54)
#define	EXYNOS5_TS_PID13			EXYNOS5_TSIREG(0x58)
#define	EXYNOS5_TS_PID14			EXYNOS5_TSIREG(0x5C)
#define	EXYNOS5_TS_PID15			EXYNOS5_TSIREG(0x60)
#define	EXYNOS5_TS_PID16			EXYNOS5_TSIREG(0x64)
#define	EXYNOS5_TS_PID17			EXYNOS5_TSIREG(0x68)
#define	EXYNOS5_TS_PID18			EXYNOS5_TSIREG(0x6C)
#define	EXYNOS5_TS_PID19			EXYNOS5_TSIREG(0x70)
#define	EXYNOS5_TS_PID20			EXYNOS5_TSIREG(0x74)
#define	EXYNOS5_TS_PID21			EXYNOS5_TSIREG(0x78)
#define	EXYNOS5_TS_PID22			EXYNOS5_TSIREG(0x7C)
#define	EXYNOS5_TS_PID23			EXYNOS5_TSIREG(0x80)
#define	EXYNOS5_TS_PID24			EXYNOS5_TSIREG(0x84)
#define	EXYNOS5_TS_PID25			EXYNOS5_TSIREG(0x88)
#define	EXYNOS5_TS_PID26			EXYNOS5_TSIREG(0x8C)
#define	EXYNOS5_TS_PID27			EXYNOS5_TSIREG(0x90)
#define	EXYNOS5_TS_PID28			EXYNOS5_TSIREG(0x94)
#define	EXYNOS5_TS_PID29			EXYNOS5_TSIREG(0x98)
#define	EXYNOS5_TS_PID30			EXYNOS5_TSIREG(0x9C)
#define	EXYNOS5_TS_PID31			EXYNOS5_TSIREG(0xA0)
#define	EXYNOS5_TS_BYTE_SWAP		EXYNOS5_TSIREG(0xBC)

/* #define TS_TIMEOUT_CNT_MAX	(0x00FFFFFF) */
#define TS_NUM_PKT			(4)
#define TS_PKT_SIZE			47
#define TS_PKT_BUF_SIZE			(TS_PKT_SIZE*TS_NUM_PKT)
#define TSI_CLK_START			1
#define TSI_CLK_STOP			0

/* CLKCON */
#define EXYNOS5_TSI_ON			(0x1<<0)
#define EXYNOS5_TSI_ON_MASK			(0x1<<0)
#define EXYNOS5_TSI_BLK_READY		(0x1<<1)

/* TS_CON */
#define EXYNOS5_TSI_SWRESET			(0x1 << 31)
#define EXYNOS5_TSI_SWRESET_MASK		(0x1 << 31)
#define EXYNOS5_TSI_CLKFILTER_ON		(0x1 << 30)
#define EXYNOS5_TSI_CLKFILTER_MASK		(0x1 << 30)
#define EXYNOS5_TSI_CLKFILTER_SHIFT		30
#define EXYNOS5_TSI_BURST_LEN_0		(0x0 << 28)
#define EXYNOS5_TSI_BURST_LEN_4		(0x1 << 28)
#define EXYNOS5_TSI_BURST_LEN_8		(0x2 << 28)
#define EXYNOS5_TSI_BURST_LEN_MASK		(0x3 << 28)
#define EXYNOS5_TSI_BURST_LEN_SHIFT		(28)

#define EXYNOS5_TSI_OUT_BUF_FULL_INT_ENA	(0x1 << 27)
#define EXYNOS5_TSI_OUT_BUF_FULL_INT_MASK	(0x1 << 27)
#define EXYNOS5_TSI_INT_FIFO_FULL_INT_ENA   (0x1 << 26)
#define EXYNOS5_TSI_INT_FIFO_FULL_INT_ENA_MASK   (0x1 << 26)

#define EXYNOS5_TSI_SYNC_MISMATCH_INT_SKIP	(0x2 << 24)
#define EXYNOS5_TSI_SYNC_MISMATCH_INT_STOP	(0x3 << 24)
#define EXYNOS5_TSI_SYNC_MISMATCH_INT_MASK	(0x3 << 24)

#define EXYNOS5_TSI_PSUF_INT_SKIP		(0x2 << 22)
#define EXYNOS5_TSI_PSUF_INT_STOP		(0x3 << 22)
#define EXYNOS5_TSI_PSUF_INT_MASK		(0x3 << 22)

#define EXYNOS5_TSI_PSOF_INT_SKIP		(0x2 << 20)
#define EXYNOS5_TSI_PSOF_INT_STOP		(0x3 << 20)
#define EXYNOS5_TSI_PSOF_INT_MASK		(0x3 << 20)

#define EXYNOS5_TSI_TS_CLK_TIME_OUT_INT	(0x1 << 19)
#define EXYNOS5_TSI_TS_CLK_TIME_OUT_INT_MASK (0x1 << 19)

#define EXYNOS5_TSI_TS_ERROR_SKIP_SIZE_INT		(4<<16)
#define EXYNOS5_TSI_TS_ERROR_STOP_SIZE_INT		(5<<16)
#define EXYNOS5_TSI_TS_ERROR_SKIP_PKT_INT		(6<<16)
#define EXYNOS5_TSI_TS_ERROR_STOP_PKT_INT		(7<<16)
#define EXYNOS5_TSI_TS_ERROR_MASK			(7<<16)
#define EXYNOS5_TSI_PAD_PATTERN_SHIFT		(8)
#define EXYNOS5_TSI_PID_FILTER_ENA			(1 << 7)
#define EXYNOS5_TSI_PID_FILTER_MASK			(1 << 7)
#define EXYNOS5_TSI_PID_FILTER_SHIFT		(7)
#define EXYNOS5_TSI_ERROR_ACTIVE_LOW		(1<<6)
#define EXYNOS5_TSI_ERROR_ACTIVE_HIGH		(0<<6)
#define EXYNOS5_TSI_ERROR_ACTIVE_MASK		(1<<6)

#define EXYNOS5_TSI_DATA_BYTE_ORDER_M2L		(0 << 5)
#define EXYNOS5_TSI_DATA_BYTE_ORDER_L2M		(1 << 5)
#define EXYNOS5_TSI_DATA_BYTE_ORDER_MASK		(1 << 5)
#define EXYNOS5_TSI_DATA_BYTE_ORDER_SHIFT		(5)
#define EXYNOS5_TSI_TS_VALID_ACTIVE_HIGH		(0<<4)
#define EXYNOS5_TSI_TS_VALID_ACTIVE_LOW		(1<<4)
#define EXYNOS5_TSI_TS_VALID_ACTIVE_MASK		(1<<4)

#define EXYNOS5_TSI_SYNC_ACTIVE_HIGH		(0 << 3)
#define EXYNOS5_TSI_SYNC_ACTIVE_LOW			(1 << 3)
#define EXYNOS5_TSI_SYNC_ACTIVE_MASK		(1 << 3)

#define EXYNOS5_TSI_CLK_INVERT_HIGH			(0 << 2)
#define EXYNOS5_TSI_CLK_INVERT_LOW			(1 << 2)
#define EXYNOS5_TSI_CLK_INVERT_MASK			(1 << 2)

/* TS_SYNC */
#define EXYNOS5_TSI_SYNC_DET_MODE_TS_SYNC8			(0<<0)
#define EXYNOS5_TSI_SYNC_DET_MODE_TS_SYNC1			(1<<0)
#define EXYNOS5_TSI_SYNC_DET_MODE_TS_SYNC_BYTE		(2<<0)
#define EXYNOS5_TSI_SYNC_DET_MODE_TS_SYNC_MASK		(3<<0)

/* TS_INT_MASK */
#define EXYNOS5_TSI_DMA_COMPLETE_ENA			(1 << 7)
#define EXYNOS5_TSI_OUTPUT_BUF_FULL_ENA			(1 << 6)
#define EXYNOS5_TSI_INT_FIFO_FULL_ENA			(1 << 5)
#define EXYNOS5_TSI_SYNC_MISMATCH_ENA			(1 << 4)
#define EXYNOS5_TSI_PKT_SIZE_UNDERFLOW_ENA			(1 << 3)
#define EXYNOS5_TSI_PKT_SIZE_OVERFLOW_ENA			(1 << 2)
#define EXYNOS5_TSI_TS_CLK_ENA				(1 << 1)
#define EXYNOS5_TSI_TS_ERROR_ENA				(1 << 0)

/* TS_INT_FLAG */
#define EXYNOS5_TSI_DMA_COMPLETE				(1<<7)
#define EXYNOS5_TSI_OUT_BUF_FULL				(1<<6)
#define EXYNOS5_TSI_INT_FIFO_FULL				(1<<5)
#define EXYNOS5_TSI_SYNC_MISMATCH				(1<<4)
#define EXYNOS5_TSI_PKT_UNDERFLOW				(1<<3)
#define EXYNOS5_TSI_PKT_OVERFLOW				(1<<2)
#define EXYNOS5_TSI_PKT_CLK					(1<<1)
#define EXYNOS5_TSI_ERROR					(1<<0)

#define TSI_BUF_SIZE	(128*1024)
#define TSI_PKT_CNT	16

enum filter_mode	{
	OFF,
	ON
};

enum pid_filter_mode	{
	BYPASS = 0,
	FILTERING
};

enum data_byte_order	{
	MSB2LSB = 0,
	LSB2MSB
};
struct tsi_pkt {
	struct list_head list;
	dma_addr_t addr;
	void *buf;
	u32 len;
};

struct exynos5_tsi_conf	{
	enum filter_mode flt_mode;
	enum pid_filter_mode pid_flt_mode;
	enum data_byte_order byte_order;
	u16  burst_len;
	u8  sync_detect;
	u8  byte_swap;
	u16 pad_pattern;
	u16 num_packet;
};

struct tsi_dev {
	spinlock_t tsi_lock;
	struct clk *gate_tsi;
	void __iomem	*tsi_base;
	int tsi_irq;
	int running;
	dma_addr_t	tsi_buf_phy;
	void	*tsi_buf_virt;
	u32		tsi_buf_size;
	struct exynos5_tsi_conf *tsi_conf;
	struct list_head free_list;
	struct list_head full_list;
	struct list_head partial_list;
	struct pinctrl *tdmb_tsi_pinctrl;
	struct pinctrl_state *tsi_on, *tsi_off;
};

struct tsi_dev *tsi_priv;
static struct platform_device *exynos5_tsi_dev;
void (*tsi_data_callback)(u8 *data, u32 length) = NULL;

static void tdmb_tsi_pull_data(struct work_struct *work);
static struct workqueue_struct *tdmb_tsi_workqueue;
static DECLARE_WORK(tdmb_tsi_work, tdmb_tsi_pull_data);
static struct pm_qos_request mif_handle;

/* #define CONFIG_TSI_LIST_DEBUG */
#ifdef CONFIG_TSI_LIST_DEBUG
static void list_debug(struct list_head *head, const char *str)
{
	int i;
	struct tsi_pkt *pkt;
	/* DPRINTK("DEBUGGING FREE LIST\n"); */
	i = 1;
	list_for_each_entry(pkt, head, list)	{
		/* DPRINTK("%s node %d node_addr %p physical add %x virt add %p size %d\n",*/
		/*			str, i, pkt, pkt->addr, pkt->buf, pkt->len); */
		i++;
	}
	DPRINTK("%s: %s %d\n", __func__, str, i - 1);
}
#endif

static void exynos5_tsi_set_gpio(struct tsi_dev *tsi, bool on)
{
	DPRINTK("%s: %d\n", __func__, on);
	if (on) {
		if (pinctrl_select_state(tsi->tdmb_tsi_pinctrl,
							tsi->tsi_on))
			DPRINTK("%s: Failed to configure tdmb_tsi_on\n", __func__);
	} else {
		if (pinctrl_select_state(tsi->tdmb_tsi_pinctrl,
							tsi->tsi_off))
			DPRINTK("%s: Failed to configure tdmb_tsi_off\n", __func__);
	}
}

static void exynos5_tsi_reset(struct tsi_dev *tsi)
{
	u32 tscon;

	tscon = readl((tsi->tsi_base + EXYNOS5_TS_CON));
	tscon |= EXYNOS5_TSI_SWRESET;
	writel(tscon, (tsi->tsi_base + EXYNOS5_TS_CON));
}
/*
 *static void exynos5_tsi_set_timeout(u32 count, struct tsi_dev *tsi)
 *{
 *	writel(count, (tsi->tsi_base + EXYNOS5_TS_CNT));
 *}
 */
static struct tsi_pkt *tsi_get_pkt(struct tsi_dev *tsi, struct list_head *head)
{
	unsigned long flags;
	struct tsi_pkt *pkt;

	spin_lock_irqsave(&tsi->tsi_lock, flags);

	if (list_empty(head))	{
		/* DPRINTK("TSI %p list is null\n", head); */
		spin_unlock_irqrestore(&tsi->tsi_lock, flags);
		return NULL;
	}
	pkt = list_first_entry(head, struct tsi_pkt, list);
	spin_unlock_irqrestore(&tsi->tsi_lock, flags);

	return pkt;
}

static void exynos5_tsi_set_dest_addr(dma_addr_t addr, void __iomem *reg)
{
	 writel(addr, reg);
}

static void exynos5_tsi_set_sync_mode(u8 mode, void __iomem *reg)
{
	u32 val = 0;

	val |= (0xff & mode);
	writel(val, reg);
}

static void exynos5_tsi_set_clock(u8 enable, void __iomem *reg)
{
	u32 val = 0;

	if (enable)
		val |= 0x1;
	writel(val, reg);
}

static int exynos5_tsi_clk_enable(struct tsi_dev *tsi)
{
	if (clk_prepare_enable(tsi->gate_tsi))
		return -ENOMEM;
	return 0;
}
static void exynos5_tsi_clk_disable(struct tsi_dev *tsi)
{
	clk_disable_unprepare(tsi->gate_tsi);
}

static void tsi_enable_interrupts(struct tsi_dev *tsi)
{
	u32 mask;
	/* Enable all the interrupts... */
	mask = 0xFF;
	writel(mask, (tsi->tsi_base + EXYNOS5_TS_INTMASK));
}

static void tsi_disable_interrupts(struct tsi_dev *tsi)
{
	writel(0, (tsi->tsi_base + EXYNOS5_TS_INTMASK));
}

static bool tdmb_tsi_create_workqueue(void)
{
	tdmb_tsi_workqueue = create_singlethread_workqueue("ktdmbtsi");
	if (tdmb_tsi_workqueue)
		return true;
	else
		return false;
}

static bool tdmb_tsi_destroy_workqueue(void)
{
	if (tdmb_tsi_workqueue) {
		flush_workqueue(tdmb_tsi_workqueue);
		destroy_workqueue(tdmb_tsi_workqueue);
		tdmb_tsi_workqueue = NULL;
	}
	return true;
}

static void exynos5_tsi_setup(struct tsi_dev *tsi)
{
	u32 tscon;
	struct exynos5_tsi_conf *conf = tsi->tsi_conf;

	exynos5_tsi_reset(tsi);
	/* exynos5_tsi_set_timeout(TS_TIMEOUT_CNT_MAX, tsi); */

	tscon = readl((tsi->tsi_base + EXYNOS5_TS_CON));

	tscon &= ~(EXYNOS5_TSI_SWRESET_MASK|EXYNOS5_TSI_CLKFILTER_MASK|
		EXYNOS5_TSI_BURST_LEN_MASK | EXYNOS5_TSI_INT_FIFO_FULL_INT_ENA_MASK |
		EXYNOS5_TSI_SYNC_MISMATCH_INT_MASK | EXYNOS5_TSI_PSUF_INT_MASK|
		EXYNOS5_TSI_PSOF_INT_MASK | EXYNOS5_TSI_TS_CLK_TIME_OUT_INT_MASK |
		EXYNOS5_TSI_TS_ERROR_MASK | EXYNOS5_TSI_PID_FILTER_MASK |
		EXYNOS5_TSI_ERROR_ACTIVE_MASK | EXYNOS5_TSI_DATA_BYTE_ORDER_MASK |
		EXYNOS5_TSI_TS_VALID_ACTIVE_MASK | EXYNOS5_TSI_SYNC_ACTIVE_MASK |
		EXYNOS5_TSI_CLK_INVERT_MASK);

	tscon |= (conf->flt_mode << EXYNOS5_TSI_CLKFILTER_SHIFT);
	tscon |= (conf->pid_flt_mode << EXYNOS5_TSI_PID_FILTER_SHIFT);
	tscon |= (conf->byte_order << EXYNOS5_TSI_DATA_BYTE_ORDER_SHIFT);
	tscon |= (conf->burst_len << EXYNOS5_TSI_BURST_LEN_SHIFT);
	tscon |= (conf->pad_pattern << EXYNOS5_TSI_PAD_PATTERN_SHIFT);

	tscon |= (EXYNOS5_TSI_OUT_BUF_FULL_INT_ENA | EXYNOS5_TSI_INT_FIFO_FULL_INT_ENA);
	tscon |= (/*EXYNOS5_TSI_SYNC_MISMATCH_INT_SKIP |*/ EXYNOS5_TSI_PSUF_INT_SKIP |
					EXYNOS5_TSI_PSOF_INT_SKIP);
	/* tscon |= (EXYNOS5_TSI_TS_CLK_TIME_OUT_INT); */
	/* These values are bd dependent? */
	tscon |= (EXYNOS5_TSI_TS_VALID_ACTIVE_HIGH | EXYNOS5_TSI_CLK_INVERT_LOW);
	writel(tscon, (tsi->tsi_base + EXYNOS5_TS_CON));
	DPRINTK("%s 0x%x\n", __func__, tscon);
	exynos5_tsi_set_sync_mode(conf->sync_detect, tsi->tsi_base + EXYNOS5_TS_SYNC);
}

static int tsi_setup_bufs(struct tsi_dev *dev, struct list_head *head, int packet_cnt)
{
	struct tsi_pkt *pkt;
	u32 tsi_virt, tsi_size, buf_size;
	u16 num_buf;
	dma_addr_t tsi_phy;
	int i;

	tsi_phy = dev->tsi_buf_phy;
	tsi_virt = (u32) dev->tsi_buf_virt;
	tsi_size = dev->tsi_buf_size;
	/* TSI generates interrupt after filling this many bytes */
	buf_size = dev->tsi_conf->num_packet * TS_PKT_SIZE * packet_cnt;
	num_buf = (tsi_size / buf_size);

	for (i = 0; i < num_buf; i++)	{
		pkt = kmalloc(sizeof(struct tsi_pkt), GFP_KERNEL);
		if (!pkt)
			return list_empty(head) ? -ENOMEM : 0;
		/*	Address should be byte-aligned	*/
		/*	Commented by sjinu, 2009_03_18	*/
		pkt->addr = ((u32)tsi_phy + i*buf_size);
		pkt->buf = (void *)(u8 *)((u32)tsi_virt + i*buf_size);
		pkt->len = buf_size;
		list_add_tail(&pkt->list, head);
	}
	DPRINTK("total nodes calulated %d buf_size %d\n", num_buf, buf_size);
#ifdef CONFIG_TSI_LIST_DEBUG1
	list_debug(head, "free_list");
#endif

	return 0;

}

static void tsi_free_packets(struct tsi_dev *tsi)
{
	struct tsi_pkt *pkt;
	unsigned long flags;
	struct list_head *full = &tsi->full_list;
	struct list_head *partial = &tsi->partial_list;
	struct list_head *head = &(tsi->free_list);

	spin_lock_irqsave(&tsi->tsi_lock, flags);
	/* move all the packets from partial and full list to free list */
	while (!list_empty(full))	{
		pkt = list_entry(full->next, struct tsi_pkt, list);
		list_move_tail(&pkt->list, &tsi->free_list);
	}
	while (!list_empty(partial))	{
		pkt = list_entry(partial->next, struct tsi_pkt, list);
		list_move_tail(&pkt->list, &tsi->free_list);
	}
	spin_unlock_irqrestore(&tsi->tsi_lock, flags);

	while (!list_empty(head))	{
		pkt = list_entry(head->next, struct tsi_pkt, list);
		list_del(&pkt->list);
		kfree(pkt);
	}
}

static int exynos5_tsi_start(struct tsi_dev *tsi, void (*callback)(u8 *data, u32 length), int packet_cnt)
{
	unsigned long flags;
	u32 pkt_size;
	struct tsi_pkt *pkt;
	int ret;

	pm_qos_add_request(&mif_handle, PM_QOS_BUS_THROUGHPUT, 413000);

	exynos5_tsi_set_gpio(tsi, true);
	if (exynos5_tsi_clk_enable(tsi)) {
		DPRINTK("%s: clk_prepare_enable failed\n", __func__);
		ret = -ENOMEM;
		goto err_gpio;
	}
	exynos5_tsi_setup(tsi);

	if (tsi_setup_bufs(tsi, &tsi->free_list, packet_cnt)) {
		DPRINTK("TSI failed to setup pkt list");
		ret = -ENOMEM;
		goto err_clk;
	}

	if (tdmb_tsi_create_workqueue() == false) {
		DPRINTK("tdmb_tsi_create_workqueue fail\n");
		ret = -ENOMEM;
		goto err_packets;
	}

	pkt =	tsi_get_pkt(tsi, &tsi->free_list);
	if (pkt == NULL) {
		DPRINTK("Failed to start TSI--No buffers avaialble\n");
		ret = -ENOMEM;
		goto err_wq;
	}
	pkt_size = pkt->len;
	tsi_data_callback = callback;
	DPRINTK("%s: pkt_size %d\n", __func__, pkt_size);
	/*when set the TS BUF SIZE to the EXYNOS5_TS_SIZE,
	 *if you want get a 10-block TS from TSIF,
	 *you should set the value of EXYNOS5_TS_SIZE as 47*10(not 188*10)
	 *This register get a value of word-multiple values.
	 *So, pkt_size which is counted to BYTES must be divided by 4
	 *Commented by sjinu, 2009_03_18
	 */
	writel(pkt_size>>2, (tsi->tsi_base + EXYNOS5_TS_SIZE));
	exynos5_tsi_set_dest_addr(pkt->addr, tsi->tsi_base + EXYNOS5_TS_BASE);
	spin_lock_irqsave(&tsi->tsi_lock, flags);
	list_move_tail(&pkt->list, &tsi->partial_list);
	spin_unlock_irqrestore(&tsi->tsi_lock, flags);

	/* start the clock */
	exynos5_tsi_set_clock(TSI_CLK_START, tsi->tsi_base + EXYNOS5_TS_CLKCON);

	/* set the second shadow base address */
	pkt =	tsi_get_pkt(tsi, &tsi->free_list);
	if (pkt == NULL) {
		DPRINTK("Failed to start TSI--No buffers avaialble\n");
		ret = -ENOMEM;
		goto err_wq;
	}
	exynos5_tsi_set_dest_addr(pkt->addr, tsi->tsi_base + EXYNOS5_TS_BASE);
	spin_lock_irqsave(&tsi->tsi_lock, flags);
	list_move_tail(&pkt->list, &tsi->partial_list);
	spin_unlock_irqrestore(&tsi->tsi_lock, flags);

	tsi_enable_interrupts(tsi);

	tsi->running = 1;
#ifdef CONFIG_TSI_LIST_DEBUG1
	list_debug(&tsi->partial_list, "partial_list");
	list_debug(&tsi->free_list, "free_list");
#endif
	return 0;

err_wq:
	tdmb_tsi_destroy_workqueue();
err_packets:
	tsi_free_packets(tsi);
err_clk:
	exynos5_tsi_clk_disable(tsi);
err_gpio:
	exynos5_tsi_set_gpio(tsi, false);
	pm_qos_remove_request(&mif_handle);

	return ret;
}

int tdmb_tsi_start(void (*callback)(u8 *data, u32 length), int packet_cnt)
{
	if (exynos5_tsi_dev) {
		struct tsi_dev *tsi = platform_get_drvdata(exynos5_tsi_dev);

		DPRINTK("%s: packet_cnt %d run %d\n", __func__, packet_cnt, tsi->running);
		if (tsi->running)
			return 0;
		return exynos5_tsi_start(tsi, callback, packet_cnt);
	}
	DPRINTK("%s: exynos5_tsi_dev is null\n", __func__);
	return -1;
}
EXPORT_SYMBOL_GPL(tdmb_tsi_start);

static int exynos5_tsi_stop(struct tsi_dev *tsi)
{
	tsi->running = 0;

	tsi_disable_interrupts(tsi);
	exynos5_tsi_set_clock(TSI_CLK_STOP, tsi->tsi_base + EXYNOS5_TS_CLKCON);
	exynos5_tsi_set_gpio(tsi, false);
	exynos5_tsi_clk_disable(tsi);

	tdmb_tsi_destroy_workqueue();
	tsi_data_callback = NULL;
	tsi_free_packets(tsi);
	pm_qos_remove_request(&mif_handle);

	return 0;
}
int tdmb_tsi_stop(void)
{
	if (exynos5_tsi_dev) {
		struct tsi_dev *tsi = platform_get_drvdata(exynos5_tsi_dev);

		DPRINTK("%s: run %d\n", __func__, tsi->running);
		if (!tsi->running)
			return 0;
		return exynos5_tsi_stop(tsi);
	}
	DPRINTK("%s: exynos5_tsi_dev is null\n", __func__);
	return -1;
}
EXPORT_SYMBOL_GPL(tdmb_tsi_stop);

static void tdmb_tsi_pull_data(struct work_struct *work)
{
	/* DPRINTK("%s\n", __func__); */
	if (exynos5_tsi_dev) {
		struct tsi_dev *tsi = platform_get_drvdata(exynos5_tsi_dev);
		struct tsi_pkt *pkt;
		unsigned long flags;

		if (!tsi->running)
			return;

#ifdef CONFIG_TSI_LIST_DEBUG
		list_debug(&tsi->free_list, "free_list");
		/* list_debug(&tsi->partial_list, "partial_list"); */
		/* list_debug(&tsi->full_list, "full_list"); */
#endif
		while ((pkt = tsi_get_pkt(tsi, &tsi->full_list)) != NULL) {
			if (tsi_data_callback)
				tsi_data_callback(pkt->buf, pkt->len);
			spin_lock_irqsave(&tsi->tsi_lock, flags);
			list_move(&pkt->list, &tsi->free_list);
			spin_unlock_irqrestore(&tsi->tsi_lock, flags);
		}
	}
}

static irqreturn_t exynos5_tsi_irq(int irq, void *dev_id)
{
	u32 intpnd;
	struct tsi_dev *tsi = platform_get_drvdata((struct platform_device *)dev_id);

	if (!tsi->running)
		return IRQ_HANDLED;

	intpnd = readl(tsi->tsi_base + EXYNOS5_TS_INT);
	writel(intpnd, (tsi->tsi_base + EXYNOS5_TS_INT));

	if (intpnd & EXYNOS5_TSI_OUT_BUF_FULL) {
		struct tsi_pkt *pkt;
		/* deque the pcket from partial list to full list*/
		/*	incase the free list is empty, stop the tsi.. */

		pkt = tsi_get_pkt(tsi, &tsi->partial_list);
		/* this situation should not come.. stop_tsi */
		if (pkt == NULL)	{
			DPRINTK("TSI..Receive interrupt without buffer\n");
			return IRQ_HANDLED;
		}
#ifdef CONFIG_TSI_LIST_DEBUG1
		DPRINTK("moving %p node %x phy %p virt to full list\n",
				pkt, pkt->addr, pkt->buf);
#endif
		list_move_tail(&pkt->list, &tsi->full_list);

		pkt = tsi_get_pkt(tsi, &tsi->free_list);
		if (pkt == NULL)	{
			/* this situation should not come.. stop_tsi */
			DPRINTK("TSI..No more free bufs..\n");
			return IRQ_HANDLED;
		}
		list_move_tail(&pkt->list, &tsi->partial_list);
		/*	namkh, request from Abraham
		 *	If there arise a buffer-full interrupt,
		 *	a new ts buffer address should be set.
		 *
		 *	Commented by sjinu, 2009_03_18
		 */
		exynos5_tsi_set_dest_addr(pkt->addr, tsi->tsi_base + EXYNOS5_TS_BASE);
		if (tdmb_tsi_workqueue) {
			int ret;

			ret = queue_work(tdmb_tsi_workqueue, &tdmb_tsi_work);
			if (ret == 0)
				DPRINTK("failed in queue_work\n");
		}
	} else
		DPRINTK("%s 0x%x\n", __func__, intpnd);

	return IRQ_HANDLED;
}

static int tdmb_tsi_probe(struct platform_device *pdev)
{
	struct resource	*res;
	static int		ret;
	struct exynos5_tsi_conf	*conf;
	dma_addr_t		map_dma;
	struct device *dev = &pdev->dev;

	DPRINTK(" %s\n", __func__);
	tsi_priv = kmalloc(sizeof(struct tsi_dev), GFP_KERNEL);
	if (tsi_priv == NULL)	{
		DPRINTK("NO Memory for tsi allocation\n");
		return -ENOMEM;
	}
	conf = kmalloc(sizeof(struct exynos5_tsi_conf), GFP_KERNEL);
	if (conf == NULL)	{
		DPRINTK("NO Memory for tsi conf allocation\n");
		kfree(tsi_priv);
		return -ENOMEM;
	}
	/* Initialise the default conf parameters..
	 * this should be obtained from the platform data and ioctl
	 * move this to platform later
	 */

	conf->flt_mode    = OFF;
	conf->pid_flt_mode = BYPASS;
	conf->byte_order = MSB2LSB;
	conf->sync_detect = EXYNOS5_TSI_SYNC_DET_MODE_TS_SYNC8;/* EXYNOS5_TSI_SYNC_DET_MODE_TS_SYNC_BYTE */

	/*
	 *to avoid making interrupt during getting the TS from TS buffer,
	 *we use the burst-length as 8 beat.
	 *This burst-length may be changed next time.
	 *Commented by sjinu, 2009_03_18
	 */
	conf->burst_len = 2;
	conf->byte_swap = 1;	/* little endian */
	conf->pad_pattern = 0;	/* this might vary from bd to bd */
	conf->num_packet = TS_NUM_PKT;	/* this might vary from bd to bd */

	tsi_priv->tsi_conf = conf;
	tsi_priv->tsi_buf_size = TSI_BUF_SIZE;

	tsi_priv->gate_tsi = devm_clk_get(dev, "gate_tsi");
	if (tsi_priv->gate_tsi == NULL)	{
		DPRINTK(KERN_ERR "Failed to get TSI clock\n");
		ret = -ENOENT;
		goto err_res;
	}

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		DPRINTK("Failed to get IRQ: %d\n", ret);
		goto err_res;
	}
	tsi_priv->tsi_irq = ret;
	ret = devm_request_irq(dev, tsi_priv->tsi_irq, exynos5_tsi_irq, 0, pdev->name, pdev);
	/* DPRINTK("tsi_irq %d ret %d\n", tsi_priv->tsi_irq, ret); */
	if (ret != 0)	{
		DPRINTK("failed to install irq (%d)\n", ret);
		goto err_res;
	}
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res == NULL) {
		DPRINTK("failed to get memory region resouce\n");
		ret = -ENOENT;
		goto err_irq;
	}
	tsi_priv->tsi_base = devm_ioremap_resource(dev, res);
	/* DPRINTK("base_phy 0x%x base_virt 0x%p\n", res->start, tsi_priv->tsi_base); */
	if (IS_ERR(tsi_priv->tsi_base)) {
		DPRINTK("failed to get base_addr (0x%p)\n", tsi_priv->tsi_base);
		ret = -ENXIO;
		goto err_irq;
	}

	INIT_LIST_HEAD(&tsi_priv->free_list);
	INIT_LIST_HEAD(&tsi_priv->full_list);
	INIT_LIST_HEAD(&tsi_priv->partial_list);
	spin_lock_init(&tsi_priv->tsi_lock);
	tsi_priv->running = 0;

	/* get the dma coherent mem */
	tsi_priv->tsi_buf_virt = dma_alloc_coherent(dev, tsi_priv->tsi_buf_size, &map_dma, GFP_KERNEL);
	if (tsi_priv->tsi_buf_virt == NULL)	{
		DPRINTK("Failed to claim TSI memory\n");
		ret = -ENOMEM;
		goto err_map;
	}
	/* DPRINTK("TSI dev dma mem phy %x virt %p\n", map_dma, tsi_priv->tsi_buf_virt); */
	tsi_priv->tsi_buf_phy = map_dma;

	tsi_priv->tdmb_tsi_pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(tsi_priv->tdmb_tsi_pinctrl))
		goto err_pinctrl;

	tsi_priv->tsi_on = pinctrl_lookup_state(tsi_priv->tdmb_tsi_pinctrl, "tdmb_tsi_on");
	if (IS_ERR(tsi_priv->tsi_on)) {
		ret = -EINVAL;
		DPRINTK("%s : could not get pins tsi_on state (%li)\n",
			__func__, PTR_ERR(tsi_priv->tsi_on));
		goto err_pinctrl_lookup_state;
	}

	tsi_priv->tsi_off = pinctrl_lookup_state(tsi_priv->tdmb_tsi_pinctrl, "tdmb_tsi_off");
	if (IS_ERR(tsi_priv->tsi_off)) {
		ret = -EINVAL;
		DPRINTK("%s : could not get pins tsi_off state (%li)\n",
			__func__, PTR_ERR(tsi_priv->tsi_off));
		goto err_pinctrl_lookup_state;
	}

	platform_set_drvdata(pdev, tsi_priv);
	exynos5_tsi_dev = pdev;

	return 0;
err_pinctrl_lookup_state:
	devm_pinctrl_put(tsi_priv->tdmb_tsi_pinctrl);
err_pinctrl:
	dma_free_coherent(dev, tsi_priv->tsi_buf_size, tsi_priv->tsi_buf_virt, tsi_priv->tsi_buf_phy);
err_map:
	iounmap(tsi_priv->tsi_base);
err_irq:
	free_irq(tsi_priv->tsi_irq, pdev);
err_res:
	kfree(conf);
	kfree(tsi_priv);

	return ret;
}

static int tdmb_tsi_remove(struct platform_device *dev)
{
	struct tsi_dev *tsi = platform_get_drvdata((struct platform_device *)dev);

	if (tsi->running)
		exynos5_tsi_stop(tsi);

	free_irq(tsi->tsi_irq, dev);
	dma_free_coherent(&dev->dev, tsi->tsi_buf_size, tsi->tsi_buf_virt, tsi->tsi_buf_phy);
	kfree(tsi->tsi_conf);
	kfree(tsi);
	return 0;
}

static const struct of_device_id tsi_match_table[] = {
	{.compatible = "samsung,tsi"},
	{}
};

static struct platform_driver tdmb_tsi_driver = {
	.driver		= {
		.name	= "tsi",
		.of_match_table = tsi_match_table,
		.owner	= THIS_MODULE,
	},
	.remove		= tdmb_tsi_remove,
};


static int __init tdmb_tsi_init(void)
{
	DPRINTK(" %s\n", __func__);
	return platform_driver_probe(&tdmb_tsi_driver, tdmb_tsi_probe);
}

static void __exit tdmb_tsi_exit(void)
{
	platform_driver_unregister(&tdmb_tsi_driver);
}

module_init(tdmb_tsi_init);
module_exit(tdmb_tsi_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Exynos TSI Device Driver");
MODULE_LICENSE("GPL");
