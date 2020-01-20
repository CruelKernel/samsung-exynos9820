/* linux/arch/arm/plat-samsung/dma-ops.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung DMA Operations
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/amba/pl330.h>
#include <linux/scatterlist.h>
#include <linux/export.h>
#include <linux/dma/dma-pl330.h>

static unsigned long samsung_dmadev_request(enum dma_ch dma_ch,
				struct samsung_dma_req *param,
				struct device *dev, char *ch_name)
{
	dma_cap_mask_t mask;
	unsigned long channel;

	dma_cap_zero(mask);
	dma_cap_set(param->cap, mask);

	if (dev->of_node)
	{
		channel = (unsigned long)dma_request_slave_channel(dev, ch_name);
		return channel;
	} else {
		pr_err("No channel: %s\n", __func__);
		return 0;
	}
}

static int samsung_dmadev_release(unsigned long ch, void *param)
{
	dma_release_channel((struct dma_chan *)ch);

	return 0;
}

static int samsung_dmadev_config(unsigned long ch,
				struct samsung_dma_config *param)
{
	struct dma_chan *chan = (struct dma_chan *)ch;
	struct dma_slave_config slave_config;

	if (param->direction == DMA_DEV_TO_MEM) {
		memset(&slave_config, 0, sizeof(struct dma_slave_config));
		slave_config.direction = param->direction;
		slave_config.src_addr = param->fifo;
		slave_config.src_addr_width = param->width;
		if (param->maxburst)
			slave_config.src_maxburst = param->maxburst;
		else
			slave_config.src_maxburst = 1;
		dmaengine_slave_config(chan, &slave_config);
	} else if (param->direction == DMA_MEM_TO_DEV) {
		memset(&slave_config, 0, sizeof(struct dma_slave_config));
		slave_config.direction = param->direction;
		slave_config.dst_addr = param->fifo;
		slave_config.dst_addr_width = param->width;
		if (param->maxburst)
			slave_config.dst_maxburst = param->maxburst;
		else
			slave_config.dst_maxburst = 1;
		dmaengine_slave_config(chan, &slave_config);
	} else {
		pr_warn("unsupported direction\n");
		return -EINVAL;
	}

	return 0;
}

static int samsung_dmadev_prepare(unsigned long ch,
			struct samsung_dma_prep *param)
{
	struct scatterlist sg;
	struct dma_chan *chan = (struct dma_chan *)ch;
	struct dma_async_tx_descriptor *desc;

	switch (param->cap) {
	case DMA_SLAVE:
		sg_init_table(&sg, 1);
		sg_dma_len(&sg) = (unsigned int)param->len;
		sg_set_page(&sg, pfn_to_page(PFN_DOWN(param->buf)),
			    param->len, offset_in_page(param->buf));
		sg_dma_address(&sg) = param->buf;

		desc = dmaengine_prep_slave_sg(chan,
			&sg, 1, param->direction, DMA_PREP_INTERRUPT);
		break;
	case DMA_CYCLIC:
		desc = chan->device->device_prep_dma_cyclic(chan,
				param->buf, param->len, param->period,
				param->direction, true, &param->infiniteloop);
		break;
	default:
		dev_err(&chan->dev->device, "unsupported format\n");
		return -EFAULT;
	}

	if (!desc) {
		dev_err(&chan->dev->device, "cannot prepare cyclic dma\n");
		return -EFAULT;
	}

	desc->callback = param->fp;
	desc->callback_param = param->fp_param;

	dmaengine_submit((struct dma_async_tx_descriptor *)desc);

	return 0;
}

static inline int samsung_dmadev_trigger(unsigned long ch)
{
	dma_async_issue_pending((struct dma_chan *)ch);

	return 0;
}

static inline int samsung_dmadev_getposition(unsigned long ch,
		dma_addr_t *src, dma_addr_t *dst)
{
	return pl330_dma_getposition((struct dma_chan *)ch, src, dst);
}

static inline int samsung_dmadev_flush(unsigned long ch)
{
	return dmaengine_terminate_all((struct dma_chan *)ch);
}

static inline int samsung_dmadev_debug(unsigned long ch)
{
	return pl330_dma_debug((struct dma_chan *)ch);
}

static struct samsung_dma_ops dmadev_ops = {
	.request	= samsung_dmadev_request,
	.release	= samsung_dmadev_release,
	.config		= samsung_dmadev_config,
	.prepare	= samsung_dmadev_prepare,
	.trigger	= samsung_dmadev_trigger,
	.started	= NULL,
	.getposition	= samsung_dmadev_getposition,
	.flush		= samsung_dmadev_flush,
	.stop		= samsung_dmadev_flush,
	.debug		= samsung_dmadev_debug,
};

void *samsung_dmadev_get_ops(void)
{
	return &dmadev_ops;
}
EXPORT_SYMBOL(samsung_dmadev_get_ops);
