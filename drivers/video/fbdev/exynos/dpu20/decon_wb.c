/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Interface file betwen DECON and Writeback for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk-provider.h>
#include "decon.h"

static irqreturn_t decon_wb_irq_handler(int irq, void *dev_data)
{
	struct decon_device *decon = dev_data;
	u32 irq_sts_reg, ext_irq;
	ext_irq = 0;

	spin_lock(&decon->slock);
	if (IS_DECON_OFF_STATE(decon))
		goto irq_end;

	irq_sts_reg = decon_reg_get_interrupt_and_clear(decon->id, &ext_irq);

	if (irq_sts_reg & DPU_FRAME_DONE_INT_EN)
		DPU_EVENT_LOG(DPU_EVT_DECON_FRAMEDONE, &decon->sd, ktime_set(0, 0));

	if (ext_irq & DPU_RESOURCE_CONFLICT_INT_EN) {
		DPU_EVENT_LOG(DPU_EVT_RSC_CONFLICT, &decon->sd, ktime_set(0, 0));
		decon_err("DECON%d Resource Conflict(ext_irq=0x%x, irq_sts=0x%x)\n",
				decon->id, ext_irq, irq_sts_reg);
	}
irq_end:
	spin_unlock(&decon->slock);
	return IRQ_HANDLED;
}

int decon_wb_register_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	struct resource *res;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* Get IRQ resource and register IRQ handler. */
	/* 0: Under Flow irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	ret = devm_request_irq(dev, res->start, decon_wb_irq_handler, 0,
			pdev->name, decon);
	if (ret) {
		decon_err("failed to install irq\n");
		return ret;
	}

	/* 1: FrameStart irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	ret = devm_request_irq(dev, res->start, decon_wb_irq_handler, 0,
			pdev->name, decon);
	if (ret) {
		decon_err("failed to install irq\n");
		return ret;
	}

	/* 2: FrameDone irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	ret = devm_request_irq(dev, res->start, decon_wb_irq_handler, 0,
			pdev->name, decon);
	if (ret) {
		decon_err("failed to install irq\n");
		return ret;
	}

	/* 3: Extra irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 3);
	ret = devm_request_irq(dev, res->start, decon_wb_irq_handler, 0,
			pdev->name, decon);
	if (ret) {
		decon_err("failed to install irq\n");
		return ret;
	}

	return ret;
}

void decon_wb_free_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	struct resource *res;

	pdev = container_of(dev, struct platform_device, dev);

	/* Unregister IRQ handler. */
	/* 0: Under Flow irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	devm_free_irq(dev, res->start, decon);

	/* 1: FrameStart irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	devm_free_irq(dev, res->start, decon);

	/* 2: FrameDone irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	devm_free_irq(dev, res->start, decon);

	/* 3: Extra irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 3);
	devm_free_irq(dev, res->start, decon);
}

int decon_wb_get_clocks(struct decon_device *decon)
{
	decon->res.aclk = devm_clk_get(decon->dev, "aclk");
	if (IS_ERR_OR_NULL(decon->res.aclk)) {
		decon_err("failed to get aclk\n");
		return PTR_ERR(decon->res.aclk);
	}
	decon->res.busd = devm_clk_get(decon->dev, "busd");
	if (IS_ERR_OR_NULL(decon->res.busd)) {
		decon_err("failed to get decon_busd\n");
		return PTR_ERR(decon->res.busd);
	}
	decon->res.busp = devm_clk_get(decon->dev, "busp");
	if (IS_ERR_OR_NULL(decon->res.busp)) {
		decon_err("failed to get decon_busp\n");
		return PTR_ERR(decon->res.busp);
	}

	return 0;
}

void decon_wb_set_clocks(struct decon_device *decon)
{
}

static int decon_wb_set_lcd_info(struct decon_device *decon)
{
	struct decon_lcd *lcd_info;

	if (decon->lcd_info == NULL) {
		lcd_info = kzalloc(sizeof(struct decon_lcd), GFP_KERNEL);
		if (!lcd_info) {
			decon_err("could not allocate decon_lcd for wb\n");
			return -ENOMEM;
		}

		decon->lcd_info = lcd_info;
	}

	decon->lcd_info->width = 1440;
	decon->lcd_info->height = 2560;
	decon->lcd_info->xres = 1440;
	decon->lcd_info->yres = 2560;
	decon->lcd_info->vfp = 2;
	decon->lcd_info->vbp = 20;
	decon->lcd_info->hfp = 20;
	decon->lcd_info->hbp = 20;
	decon->lcd_info->vsa = 2;
	decon->lcd_info->hsa = 20;
	decon->lcd_info->fps = 60;
	decon->dt.out_type = DECON_OUT_WB;
	decon->dt.psr_mode = DECON_MIPI_COMMAND_MODE;
	decon->dt.trig_mode = DECON_SW_TRIG;

	decon_info("decon_%d output size for writeback %dx%d\n", decon->id,
			decon->lcd_info->width, decon->lcd_info->height);

	return 0;
}

int decon_wb_get_out_sd(struct decon_device *decon)
{
	decon->out_sd[0] = decon->dpp_sd[ODMA_WB];
	if (IS_ERR_OR_NULL(decon->out_sd[0])) {
		decon_err("failed to get dpp%d sd\n", ODMA_WB);
		return -ENOMEM;
	}
	decon->out_sd[1] = NULL;

	decon_wb_set_lcd_info(decon);

	return 0;
}
