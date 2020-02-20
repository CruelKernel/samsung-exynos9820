/*
 * Secure RPMB Driver for Exynos scsi rpmb
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/smc.h>

#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_ioctl.h>

#include "scsi_srpmb.h"

#define SRPMB_DEVICE_PROPNAME	"samsung,ufs-srpmb"

struct platform_device *sr_pdev;

#if defined(DEBUG_SRPMB)
static void dump_packet(u8 *data, int len)
{
	u8 s[17];
	int i, j;

	s[16] = '\0';
	for (i = 0; i < len; i += 16) {
		printk("%06x :", i);
		for (j = 0; j < 16; j++) {
			printk(" %02x", data[i+j]);
			s[j] = (data[i+j] < ' ' ? '.' : (data[i+j] > '}' ? '.' : data[i+j]));
		}
		printk(" |%s|\n", s);
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

static void update_rpmb_status_flag(struct rpmb_irq_ctx *ctx,
				Rpmb_Req *req, int status)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->lock, flags);
	req->status_flag = status;
	spin_unlock_irqrestore(&ctx->lock, flags);
}

static void srpmb_worker(struct work_struct *data)
{
	int ret;
	struct rpmb_packet packet;
	struct rpmb_irq_ctx *rpmb_ctx;
	struct scsi_device *sdp;
	Rpmb_Req *req;

	if (!data) {
		dev_err(&sr_pdev->dev, "rpmb work_struct data invalid\n");
		return ;
	}
	rpmb_ctx = container_of(data, struct rpmb_irq_ctx, work);
	if (!rpmb_ctx->dev) {
		dev_err(&sr_pdev->dev, "rpmb_ctx->dev invalid\n");
		return ;
	}
	sdp = to_scsi_device(rpmb_ctx->dev);

	if (!rpmb_ctx->vir_addr) {
		dev_err(&sr_pdev->dev, "rpmb_ctx->vir_addr invalid\n");
		return ;
	}
	req = (Rpmb_Req *)rpmb_ctx->vir_addr;

	__pm_stay_awake(&rpmb_ctx->wakesrc);

	dev_info(&sr_pdev->dev, "start rpmb workqueue with command(%d)\n", req->type);

	switch (req->type) {
	case GET_WRITE_COUNTER:
		if (req->data_len != RPMB_PACKET_SIZE) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_COUTNER_DATA_LEN_ERROR);
			dev_err(&sr_pdev->dev, "data len is invalid\n");
			break;
		}

		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_COUTNER_SECURITY_OUT_ERROR);
			dev_err(&sr_pdev->dev, "ioctl read_counter error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
		req->inlen = req->data_len;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_COUTNER_SECURITY_IN_ERROR);
			dev_err(&sr_pdev->dev, "ioctl error : %x\n", ret);
			break;
		}
		if (req->rpmb_data[RPMB_RESULT] || req->rpmb_data[RPMB_RESULT+1]) {
			dev_info(&sr_pdev->dev, "GET_WRITE_COUNTER: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
				req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
				req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
		}

		update_rpmb_status_flag(rpmb_ctx, req, RPMB_PASSED);

		break;
	case WRITE_DATA:
		if (req->data_len < RPMB_PACKET_SIZE ||
			req->data_len > RPMB_PACKET_SIZE * 64) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_LEN_ERROR);
			dev_err(&sr_pdev->dev, "data len is invalid\n");
			break;
		}

		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = req->data_len;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_SECURITY_OUT_ERROR);
			dev_err(&sr_pdev->dev, "ioctl write data error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		memset(&packet, 0x0, RPMB_PACKET_SIZE);
		packet.request = RESULT_READ_REQ;
		swap_packet((uint8_t *)&packet, req->rpmb_data);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_RESULT_SECURITY_OUT_ERROR);
			dev_err(&sr_pdev->dev,
					"ioctl write_data result error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
		req->inlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					WRITE_DATA_SECURITY_IN_ERROR);
			dev_err(&sr_pdev->dev,
					"ioctl write_data result error: %x\n", ret);
			break;
		}
		if (req->rpmb_data[RPMB_RESULT] || req->rpmb_data[RPMB_RESULT+1]) {
			dev_info(&sr_pdev->dev, "WRITE_DATA: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
				req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
				req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
		}

		update_rpmb_status_flag(rpmb_ctx, req, RPMB_PASSED);

		break;
	case READ_DATA:
		if (req->data_len < RPMB_PACKET_SIZE ||
			req->data_len > RPMB_PACKET_SIZE * 64) {
			update_rpmb_status_flag(rpmb_ctx, req, READ_LEN_ERROR);
			dev_err(&sr_pdev->dev, "data len is invalid\n");
			break;
		}

		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
		req->outlen = RPMB_PACKET_SIZE;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					READ_DATA_SECURITY_OUT_ERROR);
			dev_err(&sr_pdev->dev, "ioctl read data error: %x\n", ret);
			break;
		}

		memset(req->rpmb_data, 0x0, req->data_len);
		req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
		req->inlen = req->data_len;

		ret = srpmb_scsi_ioctl(sdp, req);
		if (ret < 0) {
			update_rpmb_status_flag(rpmb_ctx, req,
					READ_DATA_SECURITY_IN_ERROR);
			dev_err(&sr_pdev->dev,
					"ioctl result read data error : %x\n", ret);
			break;
		}
		if (req->rpmb_data[RPMB_RESULT] || req->rpmb_data[RPMB_RESULT+1]) {
			dev_info(&sr_pdev->dev, "READ_DATA: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
				req->rpmb_data[RPMB_REQRES], req->rpmb_data[RPMB_REQRES+1],
				req->rpmb_data[RPMB_RESULT], req->rpmb_data[RPMB_RESULT+1]);
		}

		update_rpmb_status_flag(rpmb_ctx, req, RPMB_PASSED);

		break;
	default:
		dev_err(&sr_pdev->dev, "invalid requset type : %x\n", req->type);
	}

	__pm_relax(&rpmb_ctx->wakesrc);
	dev_info(&sr_pdev->dev, "finish rpmb workqueue with command(%d)\n", req->type);
}

static int srpmb_suspend_notifier(struct notifier_block *nb, unsigned long event,
				void *dummy)
{
	struct rpmb_irq_ctx *rpmb_ctx;
	struct device *dev;
	Rpmb_Req *req;

	if (!nb) {
		dev_err(&sr_pdev->dev, "noti_blk work_struct data invalid\n");
		return -1;
	}
	rpmb_ctx = container_of(nb, struct rpmb_irq_ctx, pm_notifier);
	dev = rpmb_ctx->dev;
	req = (Rpmb_Req *)rpmb_ctx->vir_addr;
	if (!req) {
		dev_err(dev, "Invalid wsm address for rpmb\n");
		return -EINVAL;
	}

	switch (event) {
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
	case PM_RESTORE_PREPARE:
		flush_workqueue(rpmb_ctx->srpmb_queue);
		update_rpmb_status_flag(rpmb_ctx, req, RPMB_FAIL_SUSPEND_STATUS);
		break;
	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		update_rpmb_status_flag(rpmb_ctx, req, 0);
		break;
	default:
		break;
	}

	return 0;
}

static irqreturn_t rpmb_irq_handler(int intr, void *arg)
{
	struct rpmb_irq_ctx *rpmb_ctx = (struct rpmb_irq_ctx *)arg;
	struct device *dev;
	Rpmb_Req *req;

	dev = rpmb_ctx->dev;
	req = (Rpmb_Req *)rpmb_ctx->vir_addr;
	if (!req) {
		dev_err(dev, "Invalid wsm address for rpmb\n");
		return IRQ_HANDLED;
	}

	update_rpmb_status_flag(rpmb_ctx, req, RPMB_IN_PROGRESS);
	queue_work(rpmb_ctx->srpmb_queue, &rpmb_ctx->work);

	return IRQ_HANDLED;
}

int init_wsm(struct device *dev)
{
	int ret;
	struct rpmb_irq_ctx *rpmb_ctx;
	struct irq_data *rpmb_irqd = NULL;
	irq_hw_number_t hwirq = 0;

	rpmb_ctx = kzalloc(sizeof(struct rpmb_irq_ctx), GFP_KERNEL);
	if (!rpmb_ctx) {
		dev_err(&sr_pdev->dev, "kzalloc failed\n");
		goto out_srpmb_ctx_alloc_fail;
	}

	/* buffer init */
	rpmb_ctx->vir_addr = dma_alloc_coherent(&sr_pdev->dev,
			sizeof(Rpmb_Req) + RPMB_BUF_MAX_SIZE,
			&rpmb_ctx->phy_addr, GFP_KERNEL);

	if (rpmb_ctx->vir_addr && rpmb_ctx->phy_addr) {
		dev_info(dev, "srpmb dma addr: virt_pK(%pK), phy(%llx)\n",
			rpmb_ctx->vir_addr, (uint64_t)rpmb_ctx->phy_addr);

		rpmb_ctx->irq = irq_of_parse_and_map(sr_pdev->dev.of_node, 0);
		if (rpmb_ctx->irq <= 0) {
			dev_err(&sr_pdev->dev, "No IRQ number, aborting\n");
			goto out_srpmb_init_fail;
		}

		/* Get irq_data for secure log */
		rpmb_irqd = irq_get_irq_data(rpmb_ctx->irq);
		if (!rpmb_irqd) {
			dev_err(&sr_pdev->dev, "Fail to get irq_data\n");
			goto out_srpmb_init_fail;
		}

		/* Get hardware interrupt number */
		hwirq = irqd_to_hwirq(rpmb_irqd);
		dev_dbg(&sr_pdev->dev, "hwirq for srpmb (%ld)\n", hwirq);

		rpmb_ctx->dev = dev;
		rpmb_ctx->srpmb_queue = alloc_workqueue("srpmb_wq",
				WQ_MEM_RECLAIM | WQ_UNBOUND | WQ_HIGHPRI, 1);
		if (!rpmb_ctx->srpmb_queue) {
			dev_err(&sr_pdev->dev,
				"Fail to alloc workqueue for ufs sprmb\n");
			goto out_srpmb_init_fail;
		}

		ret = request_irq(rpmb_ctx->irq, rpmb_irq_handler,
				IRQF_TRIGGER_RISING, sr_pdev->name, rpmb_ctx);
		if (ret) {
			dev_err(&sr_pdev->dev, "request irq failed: %x\n", ret);
			goto out_srpmb_init_fail;
		}

		rpmb_ctx->pm_notifier.notifier_call = srpmb_suspend_notifier;
		ret = register_pm_notifier(&rpmb_ctx->pm_notifier);
		if (ret) {
			dev_err(&sr_pdev->dev, "Failed to setup pm notifier\n");
			goto out_srpmb_init_fail;
		}

		ret = exynos_smc(SMC_SRPMB_WSM, rpmb_ctx->phy_addr, hwirq, 0);
		if (ret) {
			dev_err(&sr_pdev->dev, "wsm smc init failed: %x\n", ret);
			goto out_srpmb_unregister_pm;
		}

		wakeup_source_init(&rpmb_ctx->wakesrc, "srpmb");
		spin_lock_init(&rpmb_ctx->lock);
		INIT_WORK(&rpmb_ctx->work, srpmb_worker);

	} else {
		dev_err(&sr_pdev->dev, "wsm dma alloc failed\n");
		goto out_srpmb_dma_alloc_fail;
	}

	return 0;

out_srpmb_unregister_pm:
	unregister_pm_notifier(&rpmb_ctx->pm_notifier);
out_srpmb_init_fail:
	if (rpmb_ctx->srpmb_queue)
		destroy_workqueue(rpmb_ctx->srpmb_queue);

	dma_free_coherent(&sr_pdev->dev, RPMB_BUF_MAX_SIZE,
			rpmb_ctx->vir_addr, rpmb_ctx->phy_addr);

out_srpmb_dma_alloc_fail:
	kfree(rpmb_ctx);

out_srpmb_ctx_alloc_fail:
	return -ENOMEM;
}

static int srpmb_probe(struct platform_device *pdev)
{
	sr_pdev = pdev;

	return 0;
}

static const struct of_device_id of_match_table[] = {
	{ .compatible = SRPMB_DEVICE_PROPNAME },
	{ }
};

static struct platform_driver srpmb_plat_driver = {
	.probe = srpmb_probe,
	.driver = {
		.name = "exynos-ufs-srpmb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table,
	}
};

static int __init srpmb_init(void)
{
	return platform_driver_register(&srpmb_plat_driver);
}

static void __exit srpmb_exit(void)
{
	platform_driver_unregister(&srpmb_plat_driver);
}

subsys_initcall(srpmb_init);
module_exit(srpmb_exit);

MODULE_AUTHOR("Yongtaek Kwon <ycool.kwon@samsung.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("UFS SRPMB driver");
