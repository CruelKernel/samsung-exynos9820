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

#include <asm/cacheflush.h>
#include <linux/shm_ipc.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"

static struct dit_adaptor *g_dit;	/* For external access */

/* Initialize H/W */
static int dit_init_hw(struct dit_adaptor *dit)
{
	writel(0x4020, dit->reg_dit + DIT_DMA_INIT_DATA);
	writel(0x1, dit->reg_dit + DIT_SW_COMMAND);

	writel(0x7, dit->reg_dit + DIT_INT_ENABLE);
	writel(0x7, dit->reg_dit + DIT_INT_MASK);

	writel(0x0, dit->reg_dit + DIT_DMA_CHKSUM_OFF);
	writel(0x0, dit->reg_dit + DIT_SIZE_UPDATE_OFF);
	writel(0xf, dit->reg_dit + DIT_NAT_ZERO_CHK_OFF);

	writel(0x80, dit->reg_dit + DIT_TX_DESC_CTRL);
	writel(0x80, dit->reg_dit + DIT_TX_HEAD_CTRL);
	writel(0x20, dit->reg_dit + DIT_TX_MOD_HD_CTRL);
	writel(0x0, dit->reg_dit + DIT_TX_PKT_CTRL);
	writel(0x20, dit->reg_dit + DIT_TX_CHKSUM_CTRL);

	writel(0xc0, dit->reg_dit + DIT_RX_DESC_CTRL);
	writel(0xc0, dit->reg_dit + DIT_RX_HEAD_CTRL);
	writel(0x30, dit->reg_dit + DIT_RX_MOD_HD_CTRL);
	writel(0x0, dit->reg_dit + DIT_RX_PKT_CTRL);
	writel(0x30, dit->reg_dit + DIT_RX_CHKSUM_CTRL);

	writel(dit->dma_base & 0xffffffff, dit->reg_dit + DIT_RX_RING_ST_ADDR_0);
	writel((dit->dma_base >> 32) & 0xf, dit->reg_dit + DIT_RX_RING_ST_ADDR_1);

#ifdef DIT_CLOCK_ALWAYS_ON
	writel(0x3f, dit->reg_dit + DIT_CLK_GT_OFF);
#endif

	writel(0x1, dit->reg_busc + 0x1700);
	writel(0x6, dit->reg_busc + 0x1704);

	return 0;
}

/* Pass RX SKB to DIT descriptor */
int pass_skb_to_dit(struct dit_adaptor *dit, struct sk_buff *skb)
{
	int ret = 0;
	u32 inkey = dit->w_key;
	u32 outkey = dit->r_key;

	if (circ_get_space(BANK_ID(dit->num_desc_rx), BANK_ID(inkey), BANK_ID(outkey)) <= 1) {
		/* Pre-detect bank full */
		dit->stat.err_bankfull++;
		ret = -EBUSY;	/* To kick DIT */
		dit->busy = 1;
	}

	if (UNITS_IN_BANK(inkey) >= (BANK_SIZE_UNITS - 2)) {	/* Pre-detect desc full */
		dit->stat.err_descfull++;
		ret = -EBUSY;	/* To kick DIT */
		dit->busy = 1;
	}

#ifdef CONFIG_LINK_DEVICE_NAPI
	dit->skb_rx[inkey] = napi_alloc_skb(&dit->napi, skb->len);
#else
	dit->skb_rx[inkey] = dev_alloc_skb(skb->len);
#endif
	if (unlikely(!dit->skb_rx[inkey])) {
		mif_err_limited("alloc_skb() fail\n");
		dev_kfree_skb_any(skb);
		dit->stat.err_nomem++;
		return -ENOMEM;
	}

	skbpriv(dit->skb_rx[inkey])->lnk_hdr = skbpriv(skb)->lnk_hdr;
	skbpriv(dit->skb_rx[inkey])->sipc_ch = skbpriv(skb)->sipc_ch;
	skbpriv(dit->skb_rx[inkey])->iod = skbpriv(skb)->iod;
	skbpriv(dit->skb_rx[inkey])->ld = skbpriv(skb)->ld;
	skbpriv(dit->skb_rx[inkey])->src_skb = skb;
	dit->skb_rx[inkey]->ip_summed = skb->ip_summed;

	/* Set descriptor */
	dit->desc_rx[inkey].src_addr = virt_to_phys(skb->data);
	dit->desc_rx[inkey].dst_addr = virt_to_phys(skb_put(dit->skb_rx[inkey], skb->len));
	dit->desc_rx[inkey].size = skb->len;
	dit->desc_rx[inkey].control = (0x1 << 5);	/* CSUM */

	dit->w_key++;
	dit->stat.pkt_cnt++;

	return ret;
}

/* Pass DIT RX SKB to upper layer */
static int dit_pass_skb_to_net(struct dit_adaptor *dit, int start_idx)
{
	int i = start_idx, tail;
	u8 status;

	do {
		status = dit->desc_rx[i].status;
		if (~status & 0x01) {
			dit_debug("i=%04x (wkey=%04x) status(0x%02x) NOT DONE control(0x%02x)\n",
				i, dit->w_key, status, dit->desc_rx[i].control);
			goto fast_return;
		}

		dit->desc_rx[i].status = 0;

		tail = dit->desc_rx[i].control & (0x1 << 6);
		dit->desc_rx[i].control = 0;

		if ((dit->skb_rx[i]->data[0] == 0xff) || (dit->skb_rx[i]->data[0] == 0))
			dit->stat.err_trans++;

		if (dit->desc_rx[i].size < dit->skb_rx[i]->len)
			skb_trim(dit->skb_rx[i], dit->desc_rx[i].size);

#ifdef DIT_DEBUG_PKT
		pr_buffer("dit", (char *)(dit->skb_rx[i]->data), (size_t)(dit->skb_rx[i]->len), (size_t)40);
#endif

		if (skbpriv(dit->skb_rx[i])->src_skb) {
			dev_kfree_skb_any(skbpriv(dit->skb_rx[i])->src_skb);
			skbpriv(dit->skb_rx[i])->src_skb = NULL;
		}

		skbpriv(dit->skb_rx[i])->support_dit = 1;

		dit->mld->pass_skb_to_net(dit->mld, dit->skb_rx[i]);
	} while (!tail && (i++ < start_idx + BANK_SIZE_UNITS));

	dit->r_key = circ_new_ptr(dit->num_desc_rx, BANK_START_INDEX(dit->r_key), BANK_SIZE_UNITS);	/* Next bank */
	dit_debug("Passed to net:%d\n", (i - start_idx) + 1);

	return i - start_idx + 1;

fast_return:
	dit->r_key = i;

	return i - start_idx + 1;
}

/* Kick DIT H/W */
int dit_kick(struct dit_adaptor *dit)
{
	u32 start = BANK_START_INDEX(dit->w_key);
	u32 units = UNITS_IN_BANK(dit->w_key);
	u32 offset;

	if (readl(dit->reg_dit + DIT_DMA_INIT_DATA) == 0)
		dit_init_hw(dit);

	if (readl(dit->reg_dit + DIT_INT_PENDING) & 0x2) {
		dit->stat.err_pendig++;
		return -EACCES;
	}
	if (readl(dit->reg_dit + DIT_STATUS) != 0x0) {
		dit->stat.err_status++;
		return -EACCES;
	}

	dit->stat.kick_cnt++;

	dit->desc_rx[start].control = (0x1 << 7) | (0x1 << 5);	/* Head | CSUM */
	dit->desc_rx[start + units-1].control = (0x1 << 6) | (0x1 << 5) | (0x1 << 4);	/* Tail | CSUM | Int */

	offset = start * sizeof(struct dit_desc);
	writel((dit->dma_base + offset) & 0xffffffff, dit->reg_dit + DIT_NAT_RX_DESC_ADDR_0);
	writel(((dit->dma_base + offset) >> 32) & 0xf, dit->reg_dit + DIT_NAT_RX_DESC_ADDR_1);

	dit->w_key = circ_new_ptr(dit->num_desc_rx, BANK_START_INDEX(dit->w_key), BANK_SIZE_UNITS); /* next bank */

	dit_debug("w_key=%d, r_key=%d (units=%d)\n", dit->w_key, dit->r_key, units);

	writel(0x4, dit->reg_dit + DIT_SW_COMMAND);

	return 1;
}

#ifdef CONFIG_LINK_DEVICE_NAPI
/* NAPI */
static int dit_poll(struct napi_struct *napi, int budget)
{
	struct dit_adaptor *dit = container_of(napi, struct dit_adaptor, napi);
	int ret = 0;

	dit_debug("read bank %02x (key:%04x) write bank %02x (key:%04x)\n",
			BANK_ID(dit->r_key), dit->r_key, BANK_ID(dit->w_key), dit->w_key);

	ret = dit_pass_skb_to_net(dit, dit->r_key);
	if (ret < 0) {
		mif_err_limited("dit_pass_skb_to_net() error %d\n", ret);
		return budget;
	}

	if (circ_get_usage(BANK_ID(dit->num_desc_rx), BANK_ID(dit->w_key), BANK_ID(dit->r_key)) > 1) {
		dit_debug("Poll again - read bank %02x (key:%04x) write bank %02x (key:%04x)\n",
				BANK_ID(dit->r_key), dit->r_key, BANK_ID(dit->w_key), dit->w_key);
		return budget;
	}

	napi_complete_done(napi, ret);
	dit_enable_irq(&dit->irq_rx);
	return ret;
}
#endif

/* IRQ handler */
static irqreturn_t dit_bus_err_handler(int irq, void *arg)
{
	struct mem_link_device *mld = (struct mem_link_device *)arg;
	struct dit_adaptor *dit = &mld->dit;

	mif_err_limited("0x%08x\n", readl(dit->reg_dit + DIT_INT_PENDING));

	writel((0x1 << 2), dit->reg_dit + DIT_INT_PENDING);

	mif_err_limited("DIT_STATUS:0x%08x\n", readl(dit->reg_dit + DIT_STATUS));
	mif_err_limited("DIT_BUS_ERR:0x%08x\n", readl(dit->reg_dit + DIT_BUS_ERROR));

	dit_init(dit);

	return IRQ_HANDLED;
}

static irqreturn_t dit_rx_handler(int irq, void *arg)
{
	struct mem_link_device *mld = (struct mem_link_device *)arg;
	struct dit_adaptor *dit = &mld->dit;

	writel((0x1 << 1), dit->reg_dit + DIT_INT_PENDING);

	dit_debug("dit->r_key = %d\n", dit->r_key);

#ifdef CONFIG_LINK_DEVICE_NAPI
	if (napi_schedule_prep(&dit->napi)) {
		dit_disable_irq(&dit->irq_rx);
		__napi_schedule(&dit->napi);
	}
#else
	dit_pass_skb_to_net(dit, dit->r_key);
#endif

	return IRQ_HANDLED;
}

static irqreturn_t dit_tx_handler(int irq, void *arg)
{
	struct mem_link_device *mld = (struct mem_link_device *)arg;
	struct dit_adaptor *dit = &mld->dit;

	dit_debug("0x%08x\n", readl(dit->reg_dit + DIT_INT_PENDING));

	writel((0x1 << 0), dit->reg_dit + DIT_INT_PENDING);

	return IRQ_HANDLED;
}

/* NAT local addr */
void dit_set_nat_local_addr(u32 addr)
{
	struct dit_adaptor *dit;
	unsigned long flags;

	if (!dit_check_ready(g_dit)) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}
	dit = g_dit;

	spin_lock_irqsave(&dit->lock, flags);
	if (readl(dit->reg_dit + DIT_DMA_INIT_DATA) == 0)
		dit_init_hw(dit);

	writel(addr, dit->reg_dit + DIT_NAT_LOCAL_ADDR);
	spin_unlock_irqrestore(&dit->lock, flags);

	mif_info("local address:0x%08x\n", readl(dit->reg_dit + DIT_NAT_LOCAL_ADDR));
}

/* NAT filter */
void dit_set_nat_filter(u32 id, u8 proto, u32 sa, u16 sp, u16 dp)
{
	struct dit_adaptor *dit;
	u32 val;
	unsigned long flags;

	if (!dit_check_ready(g_dit)) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}
	dit = g_dit;

	spin_lock_irqsave(&dit->lock, flags);

	if (readl(dit->reg_dit + DIT_DMA_INIT_DATA) == 0)
		dit_init_hw(dit);

	/* Protocol */
	val = readl(dit->reg_dit + DIT_NAT_PROTOCOL_FILTER_0 + (id/4)*4);
	val &= ~(0xFF << ((id%4)*8));
	val |= proto << ((id%4)*8);
	writel(val, dit->reg_dit + DIT_NAT_PROTOCOL_FILTER_0 + (id/4)*4);

	if (proto == IPPROTO_UDP)
		writel(0x5, dit->reg_dit + DIT_NAT_ZERO_CHK_OFF);
	else
		writel(0xf, dit->reg_dit + DIT_NAT_ZERO_CHK_OFF);

	/* Source address */
	writel(sa, dit->reg_dit + DIT_NAT_SA_FILTER_0 + id*4);

	/* Source port */
	val = readl(dit->reg_dit + DIT_NAT_SP_FILTER_0 + (id/2)*4);
	val &= ~(0xFFFF << ((id%2)*16));
	val |= sp << (id%2)*16;
	writel(val, dit->reg_dit + DIT_NAT_SP_FILTER_0 + (id/2)*4);

	/* Destination port */
	val = readl(dit->reg_dit + DIT_NAT_DP_FILTER_0 + (id/2)*4);
	val &= ~(0xFFFF << ((id%2)*16));
	val |= dp << (id%2)*16;
	writel(val, dit->reg_dit + DIT_NAT_DP_FILTER_0 + (id/2)*4);

	spin_unlock_irqrestore(&dit->lock, flags);

	mif_info("id:%d proto:0x%08x sa:0x%08x sp:0x%08x dp:0x%08x\n",
		id,
		readl(dit->reg_dit + DIT_NAT_PROTOCOL_FILTER_0 + (id/4)*4),
		readl(dit->reg_dit + DIT_NAT_SA_FILTER_0 + id*4),
		readl(dit->reg_dit + DIT_NAT_SP_FILTER_0 + (id/2)*4),
		readl(dit->reg_dit + DIT_NAT_DP_FILTER_0 + (id/2)*4));
}

void dit_del_nat_filter(u32 id)
{
	dit_set_nat_filter(id, 0xFF, 0xFFFFFFFF, 0xFFFF, 0xFFFF);
}

/* PLAT prefix of IPv6 addr */
void dit_set_clat_plat_prfix(u32 id, struct in6_addr addr)
{
	struct dit_adaptor *dit;
	unsigned long flags;

	if (!dit_check_ready(g_dit)) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}
	dit = g_dit;

	spin_lock_irqsave(&dit->lock, flags);
	if (readl(dit->reg_dit + DIT_DMA_INIT_DATA) == 0)
		dit_init_hw(dit);

	writel(addr.s6_addr32[0], dit->reg_dit + DIT_CLAT_TX_PLAT_PREFIX_A0 + id*12);
	writel(addr.s6_addr32[1], dit->reg_dit + DIT_CLAT_TX_PLAT_PREFIX_A1 + id*12);
	writel(addr.s6_addr32[2], dit->reg_dit + DIT_CLAT_TX_PLAT_PREFIX_A2 + id*12);
	spin_unlock_irqrestore(&dit->lock, flags);

	mif_info("plat_prfix(%d) address:%08x %08x %08x\n", id,
		readl(dit->reg_dit + DIT_CLAT_TX_PLAT_PREFIX_A0 + id*12),
		readl(dit->reg_dit + DIT_CLAT_TX_PLAT_PREFIX_A1 + id*12),
		readl(dit->reg_dit + DIT_CLAT_TX_PLAT_PREFIX_A2 + id*12));
}

void dit_del_clat_plat_prfix(u32 id)
{
	struct in6_addr addr = IN6ADDR_ANY_INIT;

	dit_set_clat_plat_prfix(id, addr);
}

/* CLAT src addr of IPv6 addr */
void dit_set_clat_saddr(u32 id, struct in6_addr addr)
{
	struct dit_adaptor *dit;
	unsigned long flags;

	if (!dit_check_ready(g_dit)) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}
	dit = g_dit;

	spin_lock_irqsave(&dit->lock, flags);
	if (readl(dit->reg_dit + DIT_DMA_INIT_DATA) == 0)
		dit_init_hw(dit);

	writel(addr.s6_addr32[0], dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A0 + id*16);
	writel(addr.s6_addr32[1], dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A1 + id*16);
	writel(addr.s6_addr32[2], dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A2 + id*16);
	writel(addr.s6_addr32[3], dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A3 + id*16);
	spin_unlock_irqrestore(&dit->lock, flags);

	mif_info("clat filter(%d) address:%08x %08x %08x %08x\n", id,
		readl(dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A0 + id*16),
		readl(dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A1 + id*16),
		readl(dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A2 + id*16),
		readl(dit->reg_dit + DIT_CLAT_TX_CLAT_SRC_A3 + id*16));
}

void dit_del_clat_saddr(u32 id)
{
	struct in6_addr addr = IN6ADDR_ANY_INIT;

	dit_set_clat_saddr(id, addr);
}

/* CLAT filter of IPv4 addr */
void dit_set_clat_filter(u32 id, u32 addr)
{
	struct dit_adaptor *dit;
	unsigned long flags;

	if (!dit_check_ready(g_dit)) {
		mif_err_limited("DIT is not initialized\n");
		return;
	}
	dit = g_dit;

	spin_lock_irqsave(&dit->lock, flags);
	if (readl(dit->reg_dit + DIT_DMA_INIT_DATA) == 0)
		dit_init_hw(dit);

	writel(addr, dit->reg_dit + DIT_CLAT_TX_FILTER_A + id*4);
	spin_unlock_irqrestore(&dit->lock, flags);

	mif_info("clat filter(%d) address:0x%08x\n", id, readl(dit->reg_dit + DIT_CLAT_TX_FILTER_A + id*4));
}

void dit_del_clat_filter(u32 id)
{
	u32 addr = INADDR_ANY;

	dit_set_clat_filter(id, addr);
}

/* Debug */
static ssize_t force_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct dit_adaptor *dit = &mld->dit;

	return sprintf(buf, "DIT force enable:%d\n", dit->force_enable);
}

static ssize_t force_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct dit_adaptor *dit = &mld->dit;
	int ret;
	int id = 0;

	ret = sscanf(buf, "%u", &id);

	dit->force_enable = id;

	ret = count;
	return ret;
}

static ssize_t status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct dit_adaptor *dit = &mld->dit;

	return sprintf(buf, "Version:%d num_desc_rx:%d\n"
			"pkt_cnt:%d (kick_cnt:%d)\n"
			"r_key=%04x, w_key=%04x\n"
			"err trans/nomem: %d/%d\n"
			"pendig/status: %d/%d\n"
			"busy:%d busypoll:%d descfull:%d bankfull:%d)\n",
			dit->version, dit->num_desc_rx,
			dit->stat.pkt_cnt, dit->stat.kick_cnt,
			dit->r_key, dit->w_key,
			dit->stat.err_trans, dit->stat.err_nomem,
			dit->stat.err_pendig, dit->stat.err_status,
			dit->busy, dit->stat.err_busypoll, dit->stat.err_descfull, dit->stat.err_bankfull);
}

static DEVICE_ATTR_RW(force_enable);
static DEVICE_ATTR_RO(status);

static struct attribute *dit_attrs[] = {
	&dev_attr_force_enable.attr,
	&dev_attr_status.attr,
	NULL,
};

static const struct attribute_group dit_group = {
	.attrs = dit_attrs,
	.name = "dit",
};

/* Initialize DIT */
int dit_init(struct dit_adaptor *dit)
{
	u32 start = BANK_START_INDEX(dit->w_key);
	u32 units = UNITS_IN_BANK(dit->w_key);
	int i;

	if (!dit_check_support(dit))
		return 0;

	mif_info("Init DIT\n");

	atomic_set(&dit->ready, 0);

	dit_init_hw(dit);

	for (i = start; i < units; i++) {
		if (dit->skb_rx[i]) {
			if (skbpriv(dit->skb_rx[i])->src_skb)
				dev_kfree_skb_any(skbpriv(dit->skb_rx[i])->src_skb);

			dev_kfree_skb_any(dit->skb_rx[i]);
		}
	}
	memset(dit->desc_rx, 0, sizeof(struct dit_desc) * dit->num_desc_rx);

	memset(&dit->stat, 0, sizeof(struct dit_statistics));

	dit->w_key = 0;
	dit->r_key = 0;
	dit->busy = 0;

	atomic_set(&dit->ready, 1);

	return 0;
}

/* Create DIT */
int dit_create(struct platform_device *pdev, struct mem_link_device *mld)
{
	struct device_node *np = pdev->dev.of_node;
	struct dit_adaptor *dit = &mld->dit;
	struct resource *res;
	u32 alloc_size = 0;
	int ret = 0;
	u32 affinity;

	mif_info("+++\n");

	if (!np) {
		mif_err("of_node is null\n");
		return -EINVAL;
	}
	if (!dit) {
		mif_err("dit is null\n");
		return -EINVAL;
	}

	/* Resource */
	mif_dt_read_u32_noerr(np, "dit_support", dit->support);
	if (!dit->support) {
		mif_info("dit_support is 0. Just return\n");
		return 0;
	}
	mif_dt_read_u32(np, "dit_version", dit->version);
	mif_dt_read_u32(np, "dit_num_desc_rx", dit->num_desc_rx);
	mif_info("support:%d version:%d num_desc_rx:0x%08x\n", dit->support, dit->version, dit->num_desc_rx);

	atomic_set(&dit->ready, 0);
	spin_lock_init(&dit->lock);
	dit->mld = mld;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dit");
	dit->reg_dit = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dit->reg_dit)) {
		mif_err("devm_ioremap_resource() error:dit\n");
		return PTR_ERR(dit->reg_dit);
	}
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sysreg_busc");
	dit->reg_busc = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dit->reg_busc)) {
		mif_err("devm_ioremap_resource() error:busc\n");
		return PTR_ERR(dit->reg_busc);
	}

	/* IRQ handler */
	dit->irq_bus_err.num = platform_get_irq_byname(pdev, "DIT_BUS_ERR");
	ret = request_irq(dit->irq_bus_err.num, dit_bus_err_handler, 0, "DIT_BUS_ERR", mld);
	if (ret) {
		mif_err("request_irq() error:dit_bus_err %d\n", ret);
		goto create_error;
	}
	dit->irq_bus_err.registered = 1;
	dit->irq_bus_err.active = 1;
	spin_lock_init(&dit->irq_bus_err.lock);

	dit->irq_rx.num = platform_get_irq_byname(pdev, "DIT_RX");
	ret = request_irq(dit->irq_rx.num, dit_rx_handler, 0, "DIT_RX", mld);
	if (ret) {
		mif_err("request_irq() error:dit_rx %d\n", ret);
		goto create_error;
	}
	dit->irq_rx.registered = 1;
	dit->irq_rx.active = 1;
	spin_lock_init(&dit->irq_rx.lock);

	dit->irq_tx.num = platform_get_irq_byname(pdev, "DIT_TX");
	ret = request_irq(dit->irq_tx.num, dit_tx_handler, 0, "DIT_TX", mld);
	if (ret) {
		mif_err("request_irq() error:dit_tx %d\n", ret);
		goto create_error;
	}
	dit->irq_tx.registered = 1;
	dit->irq_tx.active = 1;
	spin_lock_init(&dit->irq_tx.lock);

	mif_info("IRQ - Bus error:%d RX:%d TX:%d\n", dit->irq_bus_err.num, dit->irq_rx.num, dit->irq_tx.num);

	/* IRQ affinity */
	mif_dt_read_u32(np, "dit_irq_affinity_rx", affinity);
	ret = irq_set_affinity(dit->irq_rx.num, cpumask_of(affinity));
	if (ret)
		mif_err("irq_set_affinity() error %d\n", ret);
	mif_info("IRQ affinity for RX:%d\n", affinity);

#ifdef CONFIG_LINK_DEVICE_NAPI
	/* NAPI */
	init_dummy_netdev(&dit->netdev);
	netif_napi_add(&dit->netdev, &dit->napi, dit_poll, BANK_SIZE_UNITS);
	napi_enable(&dit->napi);
#endif

	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(36));

	/* RX descriptor */
	alloc_size = sizeof(struct dit_desc) * dit->num_desc_rx;
	dit->desc_rx = dma_zalloc_coherent(&pdev->dev, alloc_size, &dit->dma_base, GFP_KERNEL);
	if (!dit->desc_rx) {
		mif_err("dma_zalloc_coherent() error:desc\n");
		ret = -ENOMEM;
		goto create_error;
	}

	/* RX SKB */
	alloc_size = sizeof(struct sk_buff *) * dit->num_desc_rx;
	dit->skb_rx = kzalloc(alloc_size, GFP_KERNEL);
	if (!dit->skb_rx) {
		mif_err("kzalloc() error:skb\n");
		ret = -ENOMEM;
		goto create_error;
	}

	g_dit = dit;	/* For external access */

	/* Init */
	dit_init(dit);

	/* Debug */
	ret = sysfs_create_group(&pdev->dev.kobj, &dit_group);
	if (ret) {
		mif_err("sysfs_create_group() error %d\n", ret);
		goto create_error;
	}

	mif_info("---\n");

	return 0;

create_error:
	kfree(dit->skb_rx);
	if (dit->desc_rx)
		dma_free_coherent(&pdev->dev, alloc_size, dit->desc_rx, dit->dma_base);

	if (dit->irq_tx.registered)
		free_irq(dit->irq_tx.num, mld);
	if (dit->irq_rx.registered)
		free_irq(dit->irq_rx.num, mld);
	if (dit->irq_bus_err.registered)
		free_irq(dit->irq_bus_err.num, mld);

	if (dit->reg_busc)
		devm_iounmap(&pdev->dev, dit->reg_busc);
	if (dit->reg_dit)
		devm_iounmap(&pdev->dev, dit->reg_dit);

	return ret;
}
