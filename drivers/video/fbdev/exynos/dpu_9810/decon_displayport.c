/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Interface file betwen DECON and DISPLAYPORT for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk-provider.h>
#include <linux/videodev2_exynos_media.h>
#include <media/v4l2-subdev.h>
#if defined(CONFIG_CAL_IF)
#include <soc/samsung/cal-if.h>
#endif
#include <dt-bindings/clock/exynos9810.h>
#include "decon.h"
#include "displayport.h"

#if !defined(CONFIG_SOC_EXYNOS9810)
static void decon_displayport_underrun_info(void)
{
#if defined(CONFIG_EXYNOS9810_BTS)
	struct decon_device *decon = get_decon_drvdata(2);

	decon_info("decon%d: underrun MIF(%lu), INT(%lu), DISP(%lu), total bw(%u, %u)\n",
			decon->id,
			cal_dfs_cached_get_rate(ACPM_DVFS_MIF),
			cal_dfs_cached_get_rate(ACPM_DVFS_INT),
			cal_dfs_cached_get_rate(ACPM_DVFS_DISP),
			decon->bts.prev_total_bw,
			decon->bts.total_bw);
#endif
}
#endif

static irqreturn_t decon_displayport_irq_handler(int irq, void *dev_data)
{
	struct decon_device *decon = dev_data;
	u32 irq_sts_reg;
	u32 ext_irq = 0;

	spin_lock(&decon->slock);
	if (decon->state == DECON_STATE_OFF)
		goto irq_end;

	irq_sts_reg = decon_reg_get_interrupt_and_clear(decon->id, &ext_irq);

	if (irq_sts_reg & DPU_FRAME_DONE_INT_PEND)
		DPU_EVENT_LOG(DPU_EVT_DECON_FRAMEDONE, &decon->sd, ktime_set(0, 0));

	if (ext_irq & DPU_TIME_OUT_INT_PEND)
		decon_err("%s: DECON%d timeout irq occurs\n", __func__, decon->id);

irq_end:
	spin_unlock(&decon->slock);
	return IRQ_HANDLED;
}

int decon_displayport_register_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	struct resource *res;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* 1: FRAME START */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	ret = devm_request_irq(dev, res->start, decon_displayport_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install FRAME START irq\n");
		return ret;
	}

	/* 2: FRAME DONE */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	ret = devm_request_irq(dev, res->start, decon_displayport_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install FRAME DONE irq\n");
		return ret;
	}

	/* 3: EXTRA: resource conflict, timeout and error irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	ret = devm_request_irq(dev, res->start, decon_displayport_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install EXTRA irq\n");
		return ret;
	}

	return ret;
}

void decon_displayport_free_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	struct resource *res;

	pdev = container_of(dev, struct platform_device, dev);

	/* 1: FRAME START */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	devm_free_irq(dev, res->start, decon);

	/* 2: FRAME DONE */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	devm_free_irq(dev, res->start, decon);

	/* 3: EXTRA: resource conflict, timeout and error irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	devm_free_irq(dev, res->start, decon);
}

int decon_displayport_get_clocks(struct decon_device *decon)
{
	return 0;
}

static ssize_t decon_displayport_show_vsync(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct decon_device *decon = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%llu\n",
			ktime_to_ns(decon->vsync.timestamp));
}
static DEVICE_ATTR(vsync, S_IRUGO, decon_displayport_show_vsync, NULL);

static int decon_displayport_vsync_thread(void *data)
{
	struct decon_device *decon = data;

	while (!kthread_should_stop()) {
		ktime_t timestamp = decon->vsync.timestamp;
		int ret = wait_event_interruptible(decon->vsync.wait,
			(timestamp != decon->vsync.timestamp) &&
			decon->vsync.active);

		if (!ret)
			sysfs_notify(&decon->dev->kobj, NULL, "vsync");
	}

	return 0;
}

int decon_displayport_create_vsync_thread(struct decon_device *decon)
{
	int ret = 0;
	char name[16];

	if (decon->id != 2) {
		decon_info("decon_displayport_create_vsync_thread is needed for displayport path\n");
		return 0;
	}

	ret = device_create_file(decon->dev, &dev_attr_vsync);
	if (ret) {
		decon_err("decon[%d] failed to create vsync file\n", decon->id);
		return ret;
	}

	sprintf(name, "decon%d-vsync", decon->id);

	decon->vsync.thread = kthread_run(decon_displayport_vsync_thread, decon, name);
	if (IS_ERR_OR_NULL(decon->vsync.thread)) {
		decon_err("failed to run vsync thread\n");
		decon->vsync.thread = NULL;
		ret = PTR_ERR(decon->vsync.thread);
		goto err;
	}

	return 0;

err:
	device_remove_file(decon->dev, &dev_attr_vsync);
	return ret;
}

static int decon_displayport_set_lcd_info(struct decon_device *decon)
{
#if defined(CONFIG_EXYNOS_DISPLAYPORT)
	struct decon_lcd *lcd_info;
	struct displayport_device *displayport = get_displayport_drvdata();

	if (decon->lcd_info == NULL) {
		lcd_info = kzalloc(sizeof(struct decon_lcd), GFP_KERNEL);
		if (!lcd_info) {
			decon_err("could not allocate decon_lcd for displayport\n");
			return -ENOMEM;
		}

		decon->lcd_info = lcd_info;
	}

	decon->lcd_info->width = supported_videos[displayport->cur_video].dv_timings.bt.width;
	decon->lcd_info->height = supported_videos[displayport->cur_video].dv_timings.bt.height;
	decon->lcd_info->xres = supported_videos[displayport->cur_video].dv_timings.bt.width;
	decon->lcd_info->yres = supported_videos[displayport->cur_video].dv_timings.bt.height;
	decon->lcd_info->vfp = supported_videos[displayport->cur_video].dv_timings.bt.vfrontporch;
	decon->lcd_info->vbp = supported_videos[displayport->cur_video].dv_timings.bt.vbackporch;
	decon->lcd_info->hfp = supported_videos[displayport->cur_video].dv_timings.bt.hfrontporch;
	decon->lcd_info->hbp = supported_videos[displayport->cur_video].dv_timings.bt.hbackporch;
	decon->lcd_info->vsa = supported_videos[displayport->cur_video].dv_timings.bt.vsync;
	decon->lcd_info->hsa = supported_videos[displayport->cur_video].dv_timings.bt.hsync;
	decon->lcd_info->fps = supported_videos[displayport->cur_video].fps;
	decon->dt.psr_mode = DECON_VIDEO_MODE;
	decon->dt.trig_mode = DECON_HW_TRIG;
	decon->dt.out_type = DECON_OUT_DP;

	if (displayport->bpc == BPC_10)
		decon->lcd_info->bpc = 10; /* 10pbc */
	else
		decon->lcd_info->bpc = 8; /* 8pbc */

	decon_info("decon_%d output size for displayport %dx%d\n", decon->id,
			decon->lcd_info->width, decon->lcd_info->height);
#else
	decon_info("Not compiled displayport driver\n");
#endif
	return 0;
}

int decon_displayport_get_out_sd(struct decon_device *decon)
{
	decon->out_sd[0] = decon->displayport_sd;
	if (IS_ERR_OR_NULL(decon->out_sd[0])) {
		decon_err("failed to get displayport sd\n");
		return -ENOMEM;
	}
	decon->out_sd[1] = NULL;

	decon_displayport_set_lcd_info(decon);

	return 0;
}

int decon_displayport_get_hdr_capa(struct decon_device *decon,
		struct decon_hdr_capabilities *hdr_capa)
{
#if defined(CONFIG_EXYNOS_DISPLAYPORT)
	struct displayport_device *displayport = get_displayport_drvdata();

	if (displayport->rx_edid_data.hdr_support)
		hdr_capa->out_types[0] = HDR_HDR10;
#else
	decon_info("Not compiled displayport driver\n");
#endif
	return 0;
}

int decon_displayport_get_hdr_capa_info(struct decon_device *decon,
		struct decon_hdr_capabilities_info *hdr_capa_info)
{
#if defined(CONFIG_EXYNOS_DISPLAYPORT)
	struct displayport_device *displayport = get_displayport_drvdata();

	if (displayport->rx_edid_data.hdr_support)
		hdr_capa_info->out_num = 1;
	else
		hdr_capa_info->out_num = 0;

	/* Need CEA-861.3 EDID value calculation on platform part */
	hdr_capa_info->max_luminance =
		displayport->rx_edid_data.max_lumi_data;
	hdr_capa_info->max_average_luminance =
		displayport->rx_edid_data.max_average_lumi_data;
	hdr_capa_info->min_luminance =
		displayport->rx_edid_data.min_lumi_data;
#else
	decon_info("Not compiled displayport driver\n");
#endif
	return 0;
}

int decon_displayport_get_config(struct decon_device *decon,
		struct exynos_displayport_data *displayport_data)
{
	struct v4l2_subdev *displayport_sd;
	struct v4l2_control ctrl;
	int ret = 0;

	decon_dbg("state : %d\n", displayport_data->state);

	ctrl.id = 0;
	ctrl.value = 0;

	displayport_sd = decon->displayport_sd;
	if (displayport_sd == NULL)
		return -EINVAL;

	mutex_lock(&decon->lock);

	switch (displayport_data->state) {
	case EXYNOS_DISPLAYPORT_STATE_PRESET:
		ret = v4l2_subdev_call(displayport_sd, video, g_dv_timings, &displayport_data->timings);

		decon_dbg("%displayport%d@%s %lldHz %s(%#x)\n",
			displayport_data->timings.bt.width,
			displayport_data->timings.bt.height,
			displayport_data->timings.bt.interlaced ? "I" : "P",
			displayport_data->timings.bt.pixelclock,
			displayport_data->timings.type ? "S3D" : "2D",
			displayport_data->timings.type);

		decon_dbg("EXYNOS_DISPLAYPORT_STATE_PRESET\n");
		break;

	case EXYNOS_DISPLAYPORT_STATE_ENUM_PRESET:
		ret = v4l2_subdev_call(displayport_sd, core, ioctl,
			DISPLAYPORT_IOC_GET_ENUM_DV_TIMINGS, &displayport_data->etimings);

		decon_dbg("EXYNOS_DISPLAYPORT_STATE_ENUM_PRESET\n");
		break;

	default:
		decon_warn("unrecongnized state %u\n", displayport_data->state);
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&decon->lock);
	return ret;
}

int decon_displayport_set_config(struct decon_device *decon,
		struct exynos_displayport_data *displayport_data)
{
	struct v4l2_subdev *displayport_sd;
	int ret = 0;

	decon_dbg("state : %d\n", displayport_data->state);

	displayport_sd = decon->displayport_sd;
	if (displayport_sd == NULL)
		return -EINVAL;

	mutex_lock(&decon->lock);

	switch (displayport_data->state) {
	case EXYNOS_DISPLAYPORT_STATE_PRESET:
		ret = v4l2_subdev_call(displayport_sd, video, s_dv_timings, &displayport_data->timings);

		if (ret)
			decon_err("failed to set timings newly\n");

		decon_dbg("%displayport%d@%s %lldHz %s(%#x)\n",
			displayport_data->timings.bt.width,
			displayport_data->timings.bt.height,
			displayport_data->timings.bt.interlaced ? "I" : "P",
			displayport_data->timings.bt.pixelclock,
			displayport_data->timings.type ? "S3D" : "2D",
			displayport_data->timings.type);

		decon_dbg("EXYNOS_DISPLAYPORT_STATE_PRESET\n");
		break;

	case EXYNOS_DISPLAYPORT_STATE_RECONNECTION:
		ret = v4l2_subdev_call(displayport_sd, core, ioctl,
			DISPLAYPORT_IOC_SET_RECONNECTION, &displayport_data->etimings);

		decon_dbg("EXYNOS_DISPLAYPORT_STATE_RECONNECTION\n");
		break;

	default:
		decon_warn("unrecongnized state %u\n", displayport_data->state);
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&decon->lock);
	return ret;
}
