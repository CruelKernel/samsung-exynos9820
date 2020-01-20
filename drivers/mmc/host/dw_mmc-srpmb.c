/*
 * Secure RPMB Driver for Exynos MMC RPMB
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/smc.h>
#include <linux/blkdev.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/core.h>
#include <linux/mmc/ioctl.h>
#include <linux/mmc/mmc.h>
#include <linux/delay.h>
#include <linux/wakelock.h>

#include "dw_mmc-srpmb.h"

#define MMC_SRPMB_DEVICE_PROPNAME	"samsung,mmc-srpmb"
#define MMC_BLOCK_NAME			"/dev/block/mmcblk0rpmb"

#if defined(DEBUG_SRPMB)
static void dump_packet(u8 *data, int len)
{
	u8 s[17];
	int i, j;

	s[16]='\0';
	for (i = 0; i < len; i += 16) {
		printk("%06x :", i);
		for (j=0; j<16; j++) {
			printk(" %02x", data[i+j]);
			s[j] = (data[i+j]<' ' ? '.' : (data[i+j]>'}' ? '.' : data[i+j]));
		}
		printk(" |%s|\n",s);
	}
	printk("\n");
}
#endif

static void swap_packet(u8 *p, u8 *d)
{
	int i;
	for (i = 0; i < RPMB_PACKET_SIZE; i++)
		d[i] = p[RPMB_PACKET_SIZE - 1 - i];
}

static void mmc_cmd_init(struct mmc_ioc_cmd *icmd)
{
	icmd->is_acmd = false;
	icmd->arg = 0;
	icmd->flags = MMC_RSP_R1;
	icmd->blksz = RPMB_PACKET_SIZE;
	icmd->blocks = 1;
	icmd->postsleep_min_us = 0;
	icmd->postsleep_max_us = 0;
	icmd->data_timeout_ns = 0;
	icmd->cmd_timeout_ms = 0;
}

static int mmc_rpmb_access(struct _mmc_rpmb_ctx *ctx, struct _mmc_rpmb_req *req)
{
	int ret;
	struct device *dev = ctx->dev;
	static struct block_device *bdev = NULL;
	struct gendisk *disk;
	static const struct block_device_operations *fops;
	struct mmc_ioc_cmd icmd;
	struct rpmb_packet packet;
	u8 *result_buf = NULL;

	/* get block device for mmc rpmb */
	if (bdev == NULL) {
		bdev = blkdev_get_by_path(MMC_BLOCK_NAME,
				FMODE_READ|FMODE_WRITE, NULL);
		if (!bdev) {
			dev_err(dev, "Fail to get block device for mmc srpmb\n");
			return -EINVAL;
		}

		disk = bdev->bd_disk;
		fops = disk->fops;
		if (!fops->srpmb_access) {
			dev_err(dev, "No function pointer for srpmb access\n");
			return -ENOTTY;
		}
	}

	wake_lock(&ctx->wakelock);

	/* Initialize mmc ioc command */
	mmc_cmd_init(&icmd);

	switch(req->type) {
	case GET_WRITE_COUNTER:
		icmd.write_flag = true;
		icmd.flags = MMC_RSP_R1;
		icmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		icmd.data_ptr = (unsigned long)req->rpmb_data;

		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = WRITE_COUNTER_SECURITY_OUT_ERROR;
			dev_err(dev, "Fail to execute for srpmb write counter \
				security out: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0, RPMB_PACKET_SIZE);
		icmd.write_flag = false;
		icmd.flags = MMC_RSP_R1;
		icmd.opcode = MMC_READ_MULTIPLE_BLOCK;

		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = WRITE_COUNTER_SECURITY_IN_ERROR;
			dev_err(dev, "Fail to execute for srpmb write counter \
				security in: %x\n", ret);
			break;
		}
		req->status_flag = PASS_STATUS;
		break;
	case WRITE_DATA:
		icmd.write_flag = RELIABLE_WRITE_REQ_SET;
		icmd.flags = MMC_RSP_R1;
		icmd.blocks = req->data_len / RPMB_PACKET_SIZE;
		icmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		icmd.data_ptr = (unsigned long)req->rpmb_data;

		/* program data packet */
		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = WRITE_DATA_SECURITY_OUT_ERROR;
			dev_err(dev, "Fail to write block for program data: %x\n", ret);
			break;
		}

		result_buf = (u8 *)kzalloc(RPMB_PACKET_SIZE, GFP_KERNEL);
		if (result_buf == NULL) {
			dev_err(dev, "Memory allocation failed\n");
			ret = -1;
			break;
		}
		icmd.write_flag = true;
		icmd.blocks = 1;
		icmd.data_ptr = (unsigned long)result_buf;
		memset(&packet, 0, RPMB_PACKET_SIZE);
		packet.request = RESULT_READ_REQ;
		swap_packet((u8 *)&packet, result_buf);

		/* result read request */
		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = WRITE_DATA_SECURITY_OUT_ERROR;
			dev_err(dev, "Fail to write block for result: %x\n", ret);
			goto wout;
		}

		memset(result_buf, 0, RPMB_PACKET_SIZE);
		icmd.write_flag = false;
		icmd.blocks = 1;
		icmd.opcode = MMC_READ_MULTIPLE_BLOCK;

		/* read multiple block for response */
		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = WRITE_DATA_SECURITY_IN_ERROR;
			dev_err(dev, "Fail to read block for response: %x\n", ret);
			goto wout;
		}
		memcpy(req->rpmb_data, result_buf, RPMB_PACKET_SIZE);
		req->status_flag = PASS_STATUS;
wout:
		kfree(result_buf);
		break;
	case READ_DATA:
		icmd.write_flag = true;
		icmd.flags = MMC_RSP_R1;
		icmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		icmd.data_ptr = (unsigned long)req->rpmb_data;

		result_buf = (u8 *)kzalloc(RPMB_PACKET_SIZE, GFP_KERNEL);
		if (result_buf == NULL) {
			dev_err(dev, "Memory allocation failed\n");
			ret = -1;
			break;
		}

		/* At read data with MMC, block count has to '0' */
		swap_packet(req->rpmb_data, result_buf);
		((struct rpmb_packet *)(result_buf))->count = 0;
		swap_packet(result_buf, req->rpmb_data);

		kfree(result_buf);

		/* read data packet */
		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = READ_DATA_SECURITY_OUT_ERROR;
			dev_err(dev, "Fail to write block for read data: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0, req->data_len);
		icmd.write_flag = false;
		icmd.opcode = MMC_READ_MULTIPLE_BLOCK;
		icmd.blocks = req->data_len/RPMB_PACKET_SIZE;

		/* read multiple block for response */
		ret = fops->srpmb_access(bdev, &icmd);
		if (ret != 0) {
			req->status_flag = READ_DATA_SECURITY_IN_ERROR;
			dev_err(dev, "Fail to read block for response: %x\n", ret);
			break;
		}
		req->status_flag = PASS_STATUS;
		break;
	default:
		dev_err(dev, "Fail to invalid command: %x\n", ret);
	}

	wake_unlock(&ctx->wakelock);

	return 0;
}

static void mmc_rpmb_worker(struct work_struct *work)
{
	int ret;
	struct device *dev;
	struct _mmc_rpmb_ctx *ctx;
	struct _mmc_rpmb_req *req;

	if (!work) {
		printk(KERN_ERR "Fail to get work for mmc rpmb\n");
		return;
	}

	ctx = container_of(work, struct _mmc_rpmb_ctx, work);
	if (!ctx) {
		printk(KERN_ERR "Fail to get mmc rpmb context\n");
		return;
	}

	dev = ctx->dev;
	req = (struct _mmc_rpmb_req *)ctx->wsm_virtaddr;
	if (!req) {
		dev_err(dev, "Invalid wsm virtual address for rpmb\n");
		return;
	}

	ret = mmc_rpmb_access(ctx, req);
	if (ret) {
		dev_err(dev, "Fail to access mmc rpmb\n");
		return;
	}

	return;
}

static irqreturn_t mmc_rpmb_interrupt(int intr, void *arg)
{
	struct _mmc_rpmb_ctx *ctx = (struct _mmc_rpmb_ctx *)arg;

	queue_work(ctx->srpmb_queue, &ctx->work);

	return IRQ_HANDLED;
}

static int init_mmc_srpmb(struct platform_device *pdev, struct _mmc_rpmb_ctx *ctx)
{
	int ret;
	struct irq_data *rpmb_irqd;
	irq_hw_number_t hwirq;
	struct device *dev = &pdev->dev;
	struct resource *res;

	if (!ctx) {
		dev_err(dev, "Invalid rpmb context address\n");
		return -ENOMEM;
	}

	/* allocation for wsm(world shared memory) */
	ctx->wsm_virtaddr = dma_alloc_coherent(dev,
				sizeof(struct _mmc_rpmb_req) + RPMB_BUF_MAX_SIZE,
				&ctx->wsm_phyaddr, GFP_KERNEL);
	if (!ctx->wsm_virtaddr || !ctx->wsm_phyaddr) {
		dev_err(dev, "Fail to alloc for srpmb wsm (world shared memory)\n");
		goto alloc_wsm_fail;
	}

	/* get mmc srpmb irq number */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(dev, "Fail to get IRQ resource\n");
		goto get_irq_fail;
	}

	ctx->irq = res->start;
	if (ctx->irq <= 0) {
		dev_err(dev, "Fail to get irq number for mmc rpmb\n");
		goto get_irq_fail;
	}

	/* Get irq_data from irq number */
	rpmb_irqd = irq_get_irq_data(ctx->irq);
	if (!rpmb_irqd) {
		dev_err(dev, "Fail to get irq_data from irq number\n");
		goto get_irq_fail;
	}

	/* Get hwirq from irq_data */
	hwirq = irqd_to_hwirq(rpmb_irqd);
	if (hwirq < 0) {
		dev_err(dev, "Fail to get hwirq from irq data\n");
		goto get_irq_fail;
	}

	/* Smc call to transfer wsm address to secure world */
	ret = exynos_smc(SMC_SRPMB_WSM, ctx->wsm_phyaddr, hwirq, 0);
	if (ret)
		dev_err(dev, "Fail to smc call to initial wsm buffer\n");

	return ret;

get_irq_fail:
	dma_free_coherent(dev, RPMB_BUF_MAX_SIZE, ctx->wsm_virtaddr,
				ctx->wsm_phyaddr);
alloc_wsm_fail:
	kfree(ctx);
	ret = -ENOMEM;
	return ret;
}

static int mmc_srpmb_probe(struct platform_device *pdev)
{
	int ret;
	struct _mmc_rpmb_ctx *ctx;
	struct device *dev = &pdev->dev;

	/* allocation for rpmb context */
	ctx = kzalloc(sizeof(struct _mmc_rpmb_ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(dev, "Fail to alloc for mmc rpmb context\n");
		return -1;
	}

	/* initialize mmc rpmb context */
	ret = init_mmc_srpmb(pdev, ctx);
	if (ret) {
		dev_err(dev, "Fail to initialize mmc srpmb\n");
		goto ctx_kfree;
	}

	/* request irq for mmc rpmb handler */
	ret = request_irq(ctx->irq, mmc_rpmb_interrupt,
			IRQF_TRIGGER_RISING, pdev->name, ctx);
	if (ret) {
		dev_err(dev, "Fail to request irq handler for mmc srpmb\n");
		goto dma_free;
	}

	ctx->dev = dev;
	INIT_WORK(&ctx->work, mmc_rpmb_worker);

	/* initialize workqueue for mmc rpmb handler */
	ctx->srpmb_queue = alloc_workqueue("srpmb_wq",
		WQ_MEM_RECLAIM | WQ_UNBOUND, 1);
	if (!ctx->srpmb_queue) {
		dev_err(dev, "Fail to alloc workqueue for mmc srpmb\n");
		goto dma_free;
	}

	platform_set_drvdata(pdev, ctx);
	wake_lock_init(&ctx->wakelock, WAKE_LOCK_SUSPEND, "srpmb");

	return 0;

dma_free:
	dma_free_coherent(dev, RPMB_BUF_MAX_SIZE, ctx->wsm_virtaddr,
				ctx->wsm_phyaddr);
ctx_kfree:
	kfree(ctx);
	return -1;
}

static int mmc_srpmb_remove(struct platform_device *pdev)
{
	struct _mmc_rpmb_ctx *ctx = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	if (ctx->srpmb_queue)
		destroy_workqueue(ctx->srpmb_queue);

	dma_free_coherent(dev, RPMB_BUF_MAX_SIZE, ctx->wsm_virtaddr,
			ctx->wsm_phyaddr);

	wake_lock_destroy(&ctx->wakelock);

	kfree(ctx);
	return 0;
}

static const struct of_device_id of_match_table[] = {
	{ .compatible = MMC_SRPMB_DEVICE_PROPNAME },
	{ }
};

static struct platform_driver mmc_srpmb_plat_driver = {
	.probe = mmc_srpmb_probe,
	.driver = {
		.name = "exynos-mmc-srpmb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table,
	},
	.remove = mmc_srpmb_remove,
};

static int __init mmc_srpmb_init(void)
{
	return platform_driver_register(&mmc_srpmb_plat_driver);
}

static void __exit mmc_srpmb_exit(void)
{
	platform_driver_unregister(&mmc_srpmb_plat_driver);
}

late_initcall(mmc_srpmb_init);
module_exit(mmc_srpmb_exit);

MODULE_AUTHOR("Yongtaek Kwon <ycool.kwon@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MMC SRPMB driver");
