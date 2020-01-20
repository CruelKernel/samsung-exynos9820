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
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/slab.h>
#include <linux/smc.h>
#include <linux/suspend.h>

#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_ioctl.h>

#include "scsi_srpmb.h"

#include "../misc/tzdev/tz_iwsock.h"

#define SRPMB_DEVICE_PROPNAME	"samsung,ufs-srpmb"

#define RPMB_SOCKET_NAME	"rpmb_socket"
#define RPMB_REQUEST_MAGIC	0x44444444
#define RPMB_REPLY_MAGIC	0x66666666

#define IOCTL_MAX_RETRY     (5)

struct scsi_srpmb_ctx {
	struct platform_device *pdev;
	struct wakeup_source wakesrc;
	Rpmb_Req *req;
	dma_addr_t wsm_phyaddr;
	void *wsm_virtaddr;
	spinlock_t lock;
	irq_hw_number_t hwirq;
	int irq;
};

static DECLARE_COMPLETION(scsi_srpmb_sdev_ready);
static struct scsi_device *scsi_srpmb_sdev;
static struct task_struct *scsi_srpmb_kthread;

/*
 * This function is public API of SCSI RPMB driver.
 * It called from SD subsystem init that provide actual SCSI device to work.
 */
int init_wsm(struct device *dev)
{
	if (!dev) {
		dev_err(dev, "Invalid device passed to init_wsm\n");
		return -EINVAL;
	}

	if (!scsi_is_sdev_device(dev)) {
		dev_err(dev, "Device is not SCSI device\n");
		return -ENODEV;
	}

	if (scsi_srpmb_sdev) {
		dev_err(dev, "Another SCSI device already assigned\n");
		return -EBUSY;
	}

	scsi_srpmb_sdev = to_scsi_device(dev);
	complete(&scsi_srpmb_sdev_ready);

	return 0;
}

static int srpmb_scsi_ioctl_helper(struct scsi_device *sdev, Rpmb_Req *req)
{
	int ret;
	unsigned int cnt = 0;

	do {
		ret = srpmb_scsi_ioctl(sdev, req);
	/* unit attention status. need to call again */
	} while (ret == 0x08000002 && ++cnt < IOCTL_MAX_RETRY);

	return ret;
}

static void swap_packet(void *src, void *dst)
{
	unsigned int i;
	char *src_buf = (char *)src;
	char *dst_buf = (char *)dst;

	for (i = 0; i < RPMB_PACKET_SIZE; i++)
		dst_buf[i] = src_buf[RPMB_PACKET_SIZE - 1 - i];
}

static void scsi_srpmb_update_status_flag(struct scsi_srpmb_ctx *ctx, unsigned int status)
{
	unsigned long flags;

	spin_lock_irqsave(&ctx->lock, flags);
	ctx->req->status_flag = status;
	spin_unlock_irqrestore(&ctx->lock, flags);
}

static void scsi_srpmb_get_write_counter(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;
	unsigned int status;
	int ret;

	__pm_stay_awake(&ctx->wakesrc);

	if (ctx->req->data_len != RPMB_PACKET_SIZE) {
		dev_err(dev, "write counter data len is invalid\n");
		status = WRITE_COUTNER_DATA_LEN_ERROR;
		goto out;
	}

	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
	ctx->req->outlen = RPMB_PACKET_SIZE;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "write counter ioctl read error: %x\n", ret);
		status = WRITE_COUTNER_SECURITY_OUT_ERROR;
		goto out;
	}

	memset(ctx->req->rpmb_data, 0x0, ctx->req->data_len);
	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
	ctx->req->inlen = ctx->req->data_len;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "write counter ioctl write error: %x\n", ret);
		status = WRITE_COUTNER_SECURITY_IN_ERROR;
		goto out;
	}
	if (ctx->req->rpmb_data[RPMB_RESULT] || ctx->req->rpmb_data[RPMB_RESULT+1]) {
		dev_info(dev, "GET_WRITE_COUNTER: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
			 ctx->req->rpmb_data[RPMB_REQRES], ctx->req->rpmb_data[RPMB_REQRES+1],
			 ctx->req->rpmb_data[RPMB_RESULT], ctx->req->rpmb_data[RPMB_RESULT+1]);
	}

	status = RPMB_PASSED;

out:
	scsi_srpmb_update_status_flag(ctx, status);
	__pm_relax(&ctx->wakesrc);
}

static void scsi_srpmb_write(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;
	struct rpmb_packet packet;
	unsigned int status;
	int ret;

	__pm_stay_awake(&ctx->wakesrc);

	if (ctx->req->data_len < RPMB_PACKET_SIZE || ctx->req->data_len > RPMB_PACKET_SIZE * 64) {
		dev_err(dev, "write data len is invalid\n");
		status = WRITE_DATA_LEN_ERROR;
		goto out;
	}

	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
	ctx->req->outlen = ctx->req->data_len;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "ioctl write data error: %x\n", ret);
		status = WRITE_DATA_SECURITY_OUT_ERROR;
		goto out;
	}

	memset(ctx->req->rpmb_data, 0x0, ctx->req->data_len);
	memset(&packet, 0x0, RPMB_PACKET_SIZE);

	packet.request = RESULT_READ_REQ;
	swap_packet(&packet, ctx->req->rpmb_data);
	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
	ctx->req->outlen = RPMB_PACKET_SIZE;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "ioctl write_data result error: %x\n", ret);
		status = WRITE_DATA_RESULT_SECURITY_OUT_ERROR;
		goto out;
	}

	memset(ctx->req->rpmb_data, 0x0, ctx->req->data_len);
	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
	ctx->req->inlen = RPMB_PACKET_SIZE;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "ioctl write_data result error: %x\n", ret);
		status = WRITE_DATA_SECURITY_IN_ERROR;
		goto out;
	}
	if (ctx->req->rpmb_data[RPMB_RESULT] || ctx->req->rpmb_data[RPMB_RESULT+1]) {
		dev_info(dev, "WRITE_DATA: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
			 ctx->req->rpmb_data[RPMB_REQRES], ctx->req->rpmb_data[RPMB_REQRES+1],
			 ctx->req->rpmb_data[RPMB_RESULT], ctx->req->rpmb_data[RPMB_RESULT+1]);
	}

	status = RPMB_PASSED;

out:
	scsi_srpmb_update_status_flag(ctx, status);
	__pm_relax(&ctx->wakesrc);
}

static void scsi_srpmb_read(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;
	unsigned int status;
	int ret;

	__pm_stay_awake(&ctx->wakesrc);

	if (ctx->req->data_len < RPMB_PACKET_SIZE || ctx->req->data_len > RPMB_PACKET_SIZE * 64) {
		dev_err(dev, "read data len is invalid\n");
		status = READ_LEN_ERROR;
		goto out;
	}

	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_OUT;
	ctx->req->outlen = RPMB_PACKET_SIZE;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "ioctl read data error: %x\n", ret);
		status = READ_DATA_SECURITY_OUT_ERROR;
		goto out;
	}

	memset(ctx->req->rpmb_data, 0x0, ctx->req->data_len);
	ctx->req->cmd = SCSI_IOCTL_SECURITY_PROTOCOL_IN;
	ctx->req->inlen = ctx->req->data_len;

	ret = srpmb_scsi_ioctl_helper(scsi_srpmb_sdev, ctx->req);
	if (ret < 0) {
		dev_err(dev, "ioctl result read data error : %x\n", ret);
		status = READ_DATA_SECURITY_IN_ERROR;
		goto out;
	}
	if (ctx->req->rpmb_data[RPMB_RESULT] || ctx->req->rpmb_data[RPMB_RESULT+1]) {
		dev_info(dev, "READ_DATA: REQ/RES = %02x%02x, RESULT = %02x%02x\n",
			 ctx->req->rpmb_data[RPMB_REQRES], ctx->req->rpmb_data[RPMB_REQRES+1],
			 ctx->req->rpmb_data[RPMB_RESULT], ctx->req->rpmb_data[RPMB_RESULT+1]);
	}


	status = RPMB_PASSED;

out:
	scsi_srpmb_update_status_flag(ctx, status);
	__pm_relax(&ctx->wakesrc);
}

static struct sock_desc *scsi_srpmb_accept_swd_connection(struct device *dev)
{
	int ret;
	struct sock_desc *srpmb_conn;
	struct sock_desc *srpmb_listen;

	srpmb_listen = tz_iwsock_socket(1);
	if (IS_ERR(srpmb_listen)) {
		dev_err(dev, "failed to create iwd socket, err = %ld\n", PTR_ERR(srpmb_listen));
		return srpmb_listen;
	}
	dev_info(dev, "Created socket\n");

	ret = tz_iwsock_listen(srpmb_listen, RPMB_SOCKET_NAME);
	if (ret) {
		dev_err(dev, "failed make iwd socket listening, err = %d\n", ret);
		srpmb_conn = ERR_PTR(ret);
		goto out;
	}
	dev_info(dev, "Make socket listening\n");

	srpmb_conn = tz_iwsock_accept(srpmb_listen);
	if (IS_ERR(srpmb_conn)) {
		dev_err(dev, "failed to accept connection, err = %ld\n", PTR_ERR(srpmb_conn));
		goto out;
	}
	dev_info(dev, "Accepted connection\n");

out:
	tz_iwsock_release(srpmb_listen);

	return srpmb_conn;
}

static void scsi_srpmb_release_connection(struct sock_desc *srpmb_conn)
{
	tz_iwsock_release(srpmb_conn);
}

static int scsi_srpmb_send_reply(struct sock_desc *srpmb_conn, struct device *dev)
{
	unsigned int sock_data = RPMB_REPLY_MAGIC;
	ssize_t len;
	int ret;

	len = tz_iwsock_write(srpmb_conn, &sock_data, sizeof(sock_data), 0);
	if (len != sizeof(sock_data)) {
		ret = len >= 0 ? -EMSGSIZE : len;
		dev_err(dev, "failed to send reply, err = %d\n", ret);
		return ret;
	} else {
		dev_info(dev, "Sent reply\n");
	}

	return 0;
}

static int scsi_srpmb_wait_request(struct sock_desc *srpmb_conn, struct device *dev)
{
	int ret;
	ssize_t len;
	unsigned int sock_data;

	len = tz_iwsock_read(srpmb_conn, &sock_data, sizeof(sock_data), 0);
	if (len > 0 && len != sizeof(sock_data)) {
		dev_err(dev, "failed to receive request, invalid len = %zd\n", len);
		return -EMSGSIZE;
	} else if (!len) {
		dev_err(dev, "connection was reset by peer\n");
		return -ECONNRESET;
	} else if (len < 0) {
		ret = len;
		dev_err(dev, "error while receiving request from SWd, err = %u\n", ret);
		return ret;
	}

	if (sock_data != RPMB_REQUEST_MAGIC) {
		dev_err(dev, "received invalid request, data = %u\n", sock_data);
		return -EINVAL;
	}

	return 0;
}

static int scsi_srpmb_kthread_work(void *data)
{
	struct scsi_srpmb_ctx *ctx = (struct scsi_srpmb_ctx *)data;
	struct device *dev = &ctx->pdev->dev;
	struct sock_desc *srpmb_conn;
	int ret;

	srpmb_conn = scsi_srpmb_accept_swd_connection(dev);
	if (IS_ERR(srpmb_conn))
		return PTR_ERR(srpmb_conn);

	if (!wait_for_completion_timeout(&scsi_srpmb_sdev_ready, msecs_to_jiffies(5000))) {
		dev_err(dev, "timeout occurred while waiting for scsi device\n");
		ret = -ETIMEDOUT;
		BUG_ON(1);
		goto out;
	}

	while (!kthread_should_stop()) {
		ret = scsi_srpmb_wait_request(srpmb_conn, dev);
		if (ret) {
			dev_err(dev, "Failed to wait request, ret = %d\n", ret);
			goto out;
		}

		switch(ctx->req->type) {
		case GET_WRITE_COUNTER:
			scsi_srpmb_get_write_counter(ctx);
			break;
		case WRITE_DATA:
			scsi_srpmb_write(ctx);
			break;
		case READ_DATA:
			scsi_srpmb_read(ctx);
			break;
		default:
			dev_err(dev, "Received unsupported request: %x\n", ctx->req->type);
			ret = -EINVAL;
		}
		if (ret)
			continue;

		ret = scsi_srpmb_send_reply(srpmb_conn, dev);
		if (ret)
			dev_err(dev, "Failed to send reply, ret = %d\n", ret);
	}

out:
	scsi_srpmb_release_connection(srpmb_conn);

	return ret;
}

static struct scsi_srpmb_ctx *scsi_srpmb_create_ctx(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct scsi_srpmb_ctx *ctx;

	ctx = kzalloc(sizeof(struct scsi_srpmb_ctx), GFP_KERNEL);
	if (!ctx) {
		dev_err(dev, "Fail to alloc for scsi rpmb context\n");
		return ERR_PTR(-ENOMEM);
	}

	ctx->pdev = pdev;

	spin_lock_init(&ctx->lock);
	wakeup_source_init(&ctx->wakesrc, "srpmb");

	return ctx;
}

static void scsi_srpmb_destroy_ctx(struct scsi_srpmb_ctx *ctx)
{
	wakeup_source_trash(&ctx->wakesrc);
	kfree(ctx);
}

static int scsi_srpmb_init_wsm(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;

	ctx->wsm_virtaddr = dma_alloc_coherent(dev,
			sizeof(Rpmb_Req) + RPMB_BUF_MAX_SIZE,
			&ctx->wsm_phyaddr, GFP_KERNEL);
	if (!ctx->wsm_virtaddr || !ctx->wsm_phyaddr) {
		dev_err(dev, "Fail to alloc for srpmb wsm (world shared memory)\n");
		return -ENOMEM;
	}

	ctx->req = (Rpmb_Req *)ctx->wsm_virtaddr;

	return 0;
}

static void scsi_srpmb_release_wsm(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;

	dma_free_coherent(dev, RPMB_BUF_MAX_SIZE, ctx->wsm_virtaddr, ctx->wsm_phyaddr);
}

static irqreturn_t scsi_srpmb_interrupt(int intr, void *arg)
{
	return IRQ_HANDLED;
}

int scsi_srpmb_init_irq(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;
	struct irq_data *rpmb_irqd;
	int ret;

	ctx->irq = irq_of_parse_and_map(dev->of_node, 0);
	if (ctx->irq <= 0) {
		dev_err(dev, "Fail to get irq number for scsi rpmb\n");
		return -ENOENT;
	}
	dev_info(dev, "Requested irq resource\n");

	/* Get irq_data from irq number */
	rpmb_irqd = irq_get_irq_data(ctx->irq);
	if (!rpmb_irqd) {
		dev_err(dev, "Fail to get irq_data from irq number\n");
		return -ENOENT;
	}
	dev_info(dev, "Requested irq data\n");

	/* Get hwirq from irq_data */
	ctx->hwirq = irqd_to_hwirq(rpmb_irqd);

	ret = request_irq(ctx->irq, scsi_srpmb_interrupt,
			IRQF_TRIGGER_RISING, ctx->pdev->name, ctx);
	if (ret) {
		dev_err(dev, "Fail to request irq handler for scsi srpmb, error=%d\n", ret);
		return ret;
	}
	dev_info(dev, "Requested irq %d\n", ctx->irq);

	return 0;
}

static void scsi_srpmb_release_irq(struct scsi_srpmb_ctx *ctx)
{
	free_irq(ctx->irq, ctx);
}

static int scsi_srpmb_register_resources(struct scsi_srpmb_ctx *ctx)
{
	struct device *dev = &ctx->pdev->dev;
	int ret;

	/* Smc call to transfer wsm address to secure world */
	ret = exynos_smc(SMC_SRPMB_WSM, ctx->wsm_phyaddr, ctx->hwirq, 0);
	if (ret)
		dev_err(dev, "Fail to smc call to registed scsi srpmb resources: error=%d\n", ret);

	dev_info(dev, "Registered WSM\n");

	return ret;
}

static int scsi_srpmb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct scsi_srpmb_ctx *ctx;
	int ret;

	BUILD_BUG_ON(sizeof(struct rpmb_packet) != RPMB_PACKET_SIZE);

	ctx = scsi_srpmb_create_ctx(pdev);
	if (IS_ERR(ctx)) {
		dev_err(dev, "Fail to alloc context: error=%ld\n", PTR_ERR(ctx));
		return PTR_ERR(ctx);
	}

	ret = scsi_srpmb_init_wsm(ctx);
	if (ret) {
		dev_err(dev, "Fail to initialize wsm\n");
		goto destroy_ctx;
	}

	ret = scsi_srpmb_init_irq(ctx);
	if (ret) {
		dev_err(dev, "Fail to initialize irq\n");
		goto release_wsm;
	}

	ret = scsi_srpmb_register_resources(ctx);
	if (ret) {
		dev_err(dev, "Fail to register resources\n");
		goto release_irq;
	}

	platform_set_drvdata(pdev, ctx);
	dev_info(dev, "Published  context\n");

	scsi_srpmb_kthread = kthread_run(scsi_srpmb_kthread_work, ctx, "scsi_srpmb_worker");
	if (IS_ERR(scsi_srpmb_kthread)) {
		/* Here we can't release IRQ or WSM due to ATF
		 * doesn't support such option, so system can crash
		 * if we release IRQ or WSM here without releasing it in ATF */
		return PTR_ERR(scsi_srpmb_kthread);
	}

	return 0;

release_irq:
	scsi_srpmb_release_irq(ctx);
release_wsm:
	scsi_srpmb_release_wsm(ctx);
destroy_ctx:
	scsi_srpmb_destroy_ctx(ctx);

	return ret;
}

static int scsi_srpmb_remove(struct platform_device *pdev)
{
	struct scsi_srpmb_ctx *ctx = platform_get_drvdata(pdev);

	kthread_stop(scsi_srpmb_kthread);

	scsi_srpmb_release_irq(ctx);
	scsi_srpmb_release_wsm(ctx);
	scsi_srpmb_destroy_ctx(ctx);

	return 0;
}

static const struct of_device_id of_match_table[] = {
	{ .compatible = SRPMB_DEVICE_PROPNAME },
	{ }
};

static struct platform_driver scsi_srpmb_plat_driver = {
	.probe = scsi_srpmb_probe,
	.driver = {
		.name = "exynos-ufs-srpmb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_table,
	},
	.remove = scsi_srpmb_remove,
};

static int __init scsi_srpmb_init(void)
{
	return platform_driver_register(&scsi_srpmb_plat_driver);
}

static void __exit scsi_srpmb_exit(void)
{
	platform_driver_unregister(&scsi_srpmb_plat_driver);
}

subsys_initcall(scsi_srpmb_init);
module_exit(scsi_srpmb_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("UFS SRPMB driver");
