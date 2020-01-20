/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Interface file between DECON and DSIM for Samsung EXYNOS DPU driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>
#include <linux/sched/types.h>
#include <linux/of_address.h>
#include <linux/pinctrl/consumer.h>
#include <linux/irq.h>
#include <media/v4l2-subdev.h>
#if defined(CONFIG_EXYNOS_WD_DVFS)
#include <linux/exynos-wd.h>
#endif

#include "decon.h"
#include "dsim.h"
#include "dpp.h"
//#include "../../../../soc/samsung/pwrcal/pwrcal.h"
//#include "../../../../soc/samsung/pwrcal/S5E8890/S5E8890-vclk.h"
#include "../../../../../kernel/irq/internals.h"
#ifdef CONFIG_EXYNOS_WD_DVFS
struct task_struct *devfreq_change_task;
#endif

/* DECON irq handler for DSI interface */
static irqreturn_t decon_irq_handler(int irq, void *dev_data)
{
	struct decon_device *decon = dev_data;
	u32 irq_sts_reg;
	u32 ext_irq = 0;

	spin_lock(&decon->slock);
	if (decon->state == DECON_STATE_OFF)
		goto irq_end;

	irq_sts_reg = decon_reg_get_interrupt_and_clear(decon->id, &ext_irq);
	decon_dbg("%s: irq_sts_reg = %x, ext_irq = %x\n", __func__,
			irq_sts_reg, ext_irq);

	if (irq_sts_reg & DPU_FRAME_START_INT_PEND) {
		/* VSYNC interrupt, accept it */
		decon->frame_cnt++;
		wake_up_interruptible_all(&decon->wait_vstatus);
		if (decon->state == DECON_STATE_TUI)
			decon_info("%s:%d TUI Frame Start\n", __func__, __LINE__);
	}

	if (irq_sts_reg & DPU_FRAME_DONE_INT_PEND) {
		DPU_EVENT_LOG(DPU_EVT_DECON_FRAMEDONE, &decon->sd, ktime_set(0, 0));
		decon_hiber_trig_reset(decon);
		if (decon->state == DECON_STATE_TUI)
			decon_info("%s:%d TUI Frame Done\n", __func__, __LINE__);
	}

	if (ext_irq & DPU_RESOURCE_CONFLICT_INT_PEND)
		DPU_EVENT_LOG(DPU_EVT_RSC_CONFLICT, &decon->sd, ktime_set(0, 0));

	if (ext_irq & DPU_TIME_OUT_INT_PEND) {
		decon_err("%s: DECON%d timeout irq occurs\n", __func__, decon->id);
#if defined(CONFIG_EXYNOS_AFBC)
		dpu_dump_afbc_info();
		BUG();
#endif
	}

irq_end:
	spin_unlock(&decon->slock);
	return IRQ_HANDLED;
}

#ifdef CONFIG_EXYNOS_WD_DVFS
static int decon_devfreq_change_task(void *data)
{
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);

		schedule();

		set_current_state(TASK_RUNNING);

		exynos_wd_call_chain();
	}

	return 0;
}
#endif

#if defined(CONFIG_SOC_EXYNOS9810)
int decon_register_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	struct resource *res;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* 1: FRAME START */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	ret = devm_request_irq(dev, res->start, decon_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install FRAME START irq\n");
		return ret;
	}

	/* 2: FRAME DONE */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	ret = devm_request_irq(dev, res->start, decon_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install FRAME DONE irq\n");
		return ret;
	}

	/* 3: EXTRA: resource conflict, timeout and error irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	ret = devm_request_irq(dev, res->start, decon_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install EXTRA irq\n");
		return ret;
	}

	/*
	 * If below IRQs are needed, please define irq number sequence
	 * like below
	 *
	 * DECON0
	 * 4: DIMMING_START
	 * 5: DIMMING_END
	 * 6: DQE_DIMMING_START
	 * 7: DQE_DIMMING_END
	 *
	 * DECON2
	 * 4: VSTATUS
	 */

	return ret;
}
#else
int decon_register_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	struct resource *res;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	if (decon->dt.psr_mode == DECON_VIDEO_MODE) {
		/* Get IRQ resource and register IRQ handler. */
		/* 0: FIFO irq */
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		ret = devm_request_irq(dev, res->start, decon_irq_handler, 0,
				pdev->name, decon);
		if (ret) {
			decon_err("failed to install FIFO irq\n");
			return ret;
		}
	}

	/* 1: FRAME START */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	ret = devm_request_irq(dev, res->start, decon_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install FRAME START irq\n");
		return ret;
	}

	/* 2: FRAME DONE */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 2);
	ret = devm_request_irq(dev, res->start, decon_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install FRAME DONE irq\n");
		return ret;
	}

	/* 3: EXTRA: resource conflict, timeout and error irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 3);
	ret = devm_request_irq(dev, res->start, decon_irq_handler,
			0, pdev->name, decon);
	if (ret) {
		decon_err("failed to install EXTRA irq\n");
		return ret;
	}

	/*
	 * If below IRQs are needed, please define irq number sequence
	 * like below
	 *
	 * DECON0
	 * 4: DIMMING_START
	 * 5: DIMMING_END
	 * 6: DQE_DIMMING_START
	 * 7: DQE_DIMMING_END
	 *
	 * DECON2
	 * 4: VSTATUS
	 */

	return ret;
}
#endif

int decon_get_clocks(struct decon_device *decon)
{
	return 0;
}

void decon_set_clocks(struct decon_device *decon)
{
}

int decon_get_out_sd(struct decon_device *decon)
{
	decon->out_sd[0] = decon->dsim_sd[decon->dt.out_idx[0]];
	if (IS_ERR_OR_NULL(decon->out_sd[0])) {
		decon_err("failed to get dsim%d sd\n", decon->dt.out_idx[0]);
		return -ENOMEM;
	}

	if (decon->dt.dsi_mode == DSI_MODE_DUAL_DSI) {
		decon->out_sd[1] = decon->dsim_sd[decon->dt.out_idx[1]];
		if (IS_ERR_OR_NULL(decon->out_sd[1])) {
			decon_err("failed to get 2nd dsim%d sd\n",
					decon->dt.out_idx[1]);
			return -ENOMEM;
		}
	}

	v4l2_subdev_call(decon->out_sd[0], core, ioctl, DSIM_IOC_GET_LCD_INFO, NULL);
	decon->lcd_info =
		(struct decon_lcd *)v4l2_get_subdev_hostdata(decon->out_sd[0]);
	if (IS_ERR_OR_NULL(decon->lcd_info)) {
		decon_err("failed to get lcd information\n");
		return -EINVAL;
	}

	decon_info("lcd_info: hfp %d hbp %d hsa %d vfp %d vbp %d vsa %d",
			decon->lcd_info->hfp, decon->lcd_info->hbp,
			decon->lcd_info->hsa, decon->lcd_info->vfp,
			decon->lcd_info->vbp, decon->lcd_info->vsa);
	decon_info("xres %d yres %d\n",
			decon->lcd_info->xres, decon->lcd_info->yres);

	return 0;
}

int decon_get_pinctrl(struct decon_device *decon)
{
	int ret = 0;

	if ((decon->dt.out_type != DECON_OUT_DSI) ||
			(decon->dt.psr_mode == DECON_VIDEO_MODE) ||
			(decon->dt.trig_mode != DECON_HW_TRIG)) {
		decon_warn("decon%d doesn't need pinctrl\n", decon->id);
		return 0;
	}

	decon->res.pinctrl = devm_pinctrl_get(decon->dev);
	if (IS_ERR(decon->res.pinctrl)) {
		decon_err("failed to get decon-%d pinctrl\n", decon->id);
		ret = PTR_ERR(decon->res.pinctrl);
		decon->res.pinctrl = NULL;
		goto err;
	}

	decon->res.hw_te_on = pinctrl_lookup_state(decon->res.pinctrl, "hw_te_on");
	if (IS_ERR(decon->res.hw_te_on)) {
		decon_err("failed to get hw_te_on pin state\n");
		ret = PTR_ERR(decon->res.hw_te_on);
		decon->res.hw_te_on = NULL;
		goto err;
	}
	decon->res.hw_te_off = pinctrl_lookup_state(decon->res.pinctrl, "hw_te_off");
	if (IS_ERR(decon->res.hw_te_off)) {
		decon_err("failed to get hw_te_off pin state\n");
		ret = PTR_ERR(decon->res.hw_te_off);
		decon->res.hw_te_off = NULL;
		goto err;
	}

err:
	return ret;
}

static irqreturn_t decon_ext_irq_handler(int irq, void *dev_id)
{
	struct decon_device *decon = dev_id;
	struct decon_mode_info psr;
	ktime_t timestamp = ktime_get();

	decon_systrace(decon, 'C', "decon_te_signal", 1);
	DPU_EVENT_LOG(DPU_EVT_TE_INTERRUPT, &decon->sd, timestamp);

	spin_lock(&decon->slock);

	if (decon->dt.trig_mode == DECON_SW_TRIG) {
		decon_to_psr_info(decon, &psr);
		decon_reg_set_trigger(decon->id, &psr, DECON_TRIG_ENABLE);
	}

#ifdef CONFIG_DECON_HIBER
	if (decon->state == DECON_STATE_ON && (decon->dt.out_type == DECON_OUT_DSI)) {
		if (decon_min_lock_cond(decon))
			kthread_queue_work(&decon->hiber.worker, &decon->hiber.work);
	}
#endif
	decon_systrace(decon, 'C', "decon_te_signal", 0);
	decon->vsync.timestamp = timestamp;
	wake_up_interruptible_all(&decon->vsync.wait);

	spin_unlock(&decon->slock);
#ifdef CONFIG_EXYNOS_WD_DVFS
	if (devfreq_change_task)
		wake_up_process(devfreq_change_task);
#endif

	return IRQ_HANDLED;
}

int decon_register_ext_irq(struct decon_device *decon)
{
	struct device *dev = decon->dev;
	struct platform_device *pdev;
	int gpio = -EINVAL, gpio1 = -EINVAL;
	int ret = 0;

	pdev = container_of(dev, struct platform_device, dev);

	/* Get IRQ resource and register IRQ handler. */
	if (of_get_property(dev->of_node, "gpios", NULL) != NULL) {
		gpio = of_get_gpio(dev->of_node, 0);
		if (gpio < 0) {
			decon_err("failed to get proper gpio number\n");
			return -EINVAL;
		}

		gpio1 = of_get_gpio(dev->of_node, 1);
		if (gpio1 < 0)
			decon_info("This board doesn't support TE GPIO of 2nd LCD\n");
	} else {
		decon_err("failed to find gpio node from device tree\n");
		return -EINVAL;
	}

	decon->res.irq = gpio_to_irq(gpio);

	decon_info("%s: gpio(%d)\n", __func__, decon->res.irq);
	ret = devm_request_irq(dev, decon->res.irq, decon_ext_irq_handler,
			IRQF_TRIGGER_RISING, pdev->name, decon);

	decon->eint_status = 1;

#ifdef CONFIG_EXYNOS_WD_DVFS
	devfreq_change_task =
		kthread_create(decon_devfreq_change_task, NULL,
				"devfreq_change");
	if (IS_ERR(devfreq_change_task))
		return PTR_ERR(devfreq_change_task);
#endif

	return ret;
}

static ssize_t decon_show_vsync(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct decon_device *decon = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%llu\n",
			ktime_to_ns(decon->vsync.timestamp));
}
static DEVICE_ATTR(vsync, S_IRUGO, decon_show_vsync, NULL);

static int decon_vsync_thread(void *data)
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

int decon_create_vsync_thread(struct decon_device *decon)
{
	int ret = 0;
	char name[16];

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_info("vsync thread is only needed for DSI path\n");
		return 0;
	}

	ret = device_create_file(decon->dev, &dev_attr_vsync);
	if (ret) {
		decon_err("failed to create vsync file\n");
		return ret;
	}

	sprintf(name, "decon%d-vsync", decon->id);
	decon->vsync.thread = kthread_run(decon_vsync_thread, decon, name);
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

void decon_destroy_vsync_thread(struct decon_device *decon)
{
	device_remove_file(decon->dev, &dev_attr_vsync);

	if (decon->vsync.thread)
		kthread_stop(decon->vsync.thread);
}

/*
 * Variable Descriptions
 *   dsc_en : comp_mode (0=No Comp, 1=DSC, 2=MIC, 3=LEGO)
 *   dsc_width : min_window_update_width depending on compression mode
 *   dsc_height : min_window_update_height depending on compression mode
 */
static ssize_t decon_show_psr_info(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct decon_device *decon = dev_get_drvdata(dev);
	struct decon_lcd *lcd_info = decon->lcd_info;
	int i;
	char *p = buf;
	struct lcd_mres_info *mres_info = &lcd_info->dt_lcd_mres;
	int len;

	len = sprintf(p, "%d\n", decon->dt.psr_mode);
	len += sprintf(p + len, "%d\n", mres_info->mres_number);
	for (i = 0; i < mres_info->mres_number; i++) {
		if (mres_info->res_info[i].dsc_en)
			len += sprintf(p + len, "%d\n%d\n%d\n%d\n%d\n",
				mres_info->res_info[i].width,
				mres_info->res_info[i].height,
				mres_info->res_info[i].dsc_width,
				mres_info->res_info[i].dsc_height,
				mres_info->res_info[i].dsc_en);
		else
			len += sprintf(p + len, "%d\n%d\n%d\n%d\n%d\n",
				mres_info->res_info[i].width,
				mres_info->res_info[i].height,
				MIN_WIN_BLOCK_WIDTH,
				MIN_WIN_BLOCK_HEIGHT,
				mres_info->res_info[i].dsc_en);
	}
	return len;
}
static DEVICE_ATTR(psr_info, S_IRUGO, decon_show_psr_info, NULL);

int decon_create_psr_info(struct decon_device *decon)
{
	int ret = 0;

	/* It's enought to make a file for PSR information */
	if (decon->id != 0)
		return 0;

	ret = device_create_file(decon->dev, &dev_attr_psr_info);
	if (ret) {
		decon_err("failed to create psr info file\n");
		return ret;
	}

	return ret;
}

void decon_destroy_psr_info(struct decon_device *decon)
{
	device_remove_file(decon->dev, &dev_attr_psr_info);
}


/* Framebuffer interface related callback functions */
static u32 fb_visual(u32 bits_per_pixel, unsigned short palette_sz)
{
	switch (bits_per_pixel) {
	case 32:
	case 24:
	case 16:
	case 12:
		return FB_VISUAL_TRUECOLOR;
	case 8:
		if (palette_sz >= 256)
			return FB_VISUAL_PSEUDOCOLOR;
		else
			return FB_VISUAL_TRUECOLOR;
	case 1:
		return FB_VISUAL_MONO01;
	default:
		return FB_VISUAL_PSEUDOCOLOR;
	}
}

static inline u32 fb_linelength(u32 xres_virtual, u32 bits_per_pixel)
{
	return (xres_virtual * bits_per_pixel) / 8;
}

static u16 fb_panstep(u32 res, u32 res_virtual)
{
	return res_virtual > res ? 1 : 0;
}

int decon_set_par(struct fb_info *info)
{
	struct fb_var_screeninfo *var = &info->var;
	struct decon_win *win = info->par;
	struct decon_device *decon = win->decon;
	struct decon_window_regs win_regs;
	int win_no = win->idx;

	decon_info("%s: state %d +\n", __func__, decon->state);

	if ((decon->dt.out_type == DECON_OUT_DSI &&
			decon->state == DECON_STATE_INIT) ||
			decon->state == DECON_STATE_OFF)
		return 0;

	memset(&win_regs, 0, sizeof(struct decon_window_regs));

	decon_hiber_block_exit(decon);

	decon_reg_wait_for_update_timeout(decon->id, SHADOW_UPDATE_TIMEOUT);
	info->fix.visual = fb_visual(var->bits_per_pixel, 0);

	info->fix.line_length = fb_linelength(var->xres_virtual,
			var->bits_per_pixel);
	info->fix.xpanstep = fb_panstep(var->xres, var->xres_virtual);
	info->fix.ypanstep = fb_panstep(var->yres, var->yres_virtual);

	win_regs.wincon |= wincon(var->transp.length, 0, 0xFF,
				0xFF, DECON_BLENDING_NONE, win_no);
	win_regs.start_pos = win_start_pos(0, 0);
	win_regs.end_pos = win_end_pos(0, 0, var->xres, var->yres);
	win_regs.pixel_count = (var->xres * var->yres);
	win_regs.whole_w = var->xoffset + var->xres;
	win_regs.whole_h = var->yoffset + var->yres;
	win_regs.offset_x = var->xoffset;
	win_regs.offset_y = var->yoffset;
	win_regs.type = decon->dt.dft_idma;
	decon_reg_set_window_control(decon->id, win_no, &win_regs, false);

	decon_hiber_unblock(decon);
	return 0;
}
EXPORT_SYMBOL(decon_set_par);

int decon_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct decon_win *win = info->par;
	struct decon_device *decon = win->decon;

	var->xres_virtual = max(var->xres_virtual, var->xres);
	var->yres_virtual = max(var->yres_virtual, var->yres);

	if (!decon_validate_x_alignment(decon, 0, var->xres,
			var->bits_per_pixel))
		return -EINVAL;

	/* always ensure these are zero, for drop through cases below */
	var->transp.offset = 0;
	var->transp.length = 0;

	switch (var->bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
		var->red.offset		= 4;
		var->green.offset	= 2;
		var->blue.offset	= 0;
		var->red.length		= 5;
		var->green.length	= 3;
		var->blue.length	= 2;
		var->transp.offset	= 7;
		var->transp.length	= 1;
		break;

	case 19:
		/* 666 with one bit alpha/transparency */
		var->transp.offset	= 18;
		var->transp.length	= 1;
	case 18:
		var->bits_per_pixel	= 32;

		/* 666 format */
		var->red.offset		= 12;
		var->green.offset	= 6;
		var->blue.offset	= 0;
		var->red.length		= 6;
		var->green.length	= 6;
		var->blue.length	= 6;
		break;

	case 16:
		/* 16 bpp, 565 format */
		var->red.offset		= 11;
		var->green.offset	= 5;
		var->blue.offset	= 0;
		var->red.length		= 5;
		var->green.length	= 6;
		var->blue.length	= 5;
		break;

	case 32:
	case 28:
	case 25:
		var->transp.length	= var->bits_per_pixel - 24;
		var->transp.offset	= 24;
		/* drop through */
	case 24:
		/* our 24bpp is unpacked, so 32bpp */
		var->bits_per_pixel	= 32;
		var->red.offset		= 16;
		var->red.length		= 8;
		var->green.offset	= 8;
		var->green.length	= 8;
		var->blue.offset	= 0;
		var->blue.length	= 8;
		break;

	default:
		decon_err("invalid bpp %d\n", var->bits_per_pixel);
		return -EINVAL;
	}

	decon_dbg("xres:%d, yres:%d, v_xres:%d, v_yres:%d, bpp:%d\n",
			var->xres, var->yres, var->xres_virtual,
			var->yres_virtual, var->bits_per_pixel);

	return 0;
}

static inline unsigned int chan_to_field(unsigned int chan,
					 struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

int decon_setcolreg(unsigned regno,
			    unsigned red, unsigned green, unsigned blue,
			    unsigned transp, struct fb_info *info)
{
	struct decon_win *win = info->par;
	struct decon_device *decon = win->decon;
	unsigned int val;

	printk("@@@@ %s\n", __func__);
	decon_dbg("%s: win %d: %d => rgb=%d/%d/%d\n", __func__, win->idx,
			regno, red, green, blue);

	if (decon->state == DECON_STATE_OFF)
		return 0;

	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/* true-colour, use pseudo-palette */

		if (regno < 16) {
			u32 *pal = info->pseudo_palette;

			val  = chan_to_field(red,   &info->var.red);
			val |= chan_to_field(green, &info->var.green);
			val |= chan_to_field(blue,  &info->var.blue);

			pal[regno] = val;
		}
		break;
	default:
		return 1;	/* unknown type */
	}

	return 0;
}
EXPORT_SYMBOL(decon_setcolreg);

int decon_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct decon_win *win = info->par;
	struct decon_device *decon = win->decon;
	struct v4l2_subdev *sd = NULL;
	struct decon_win_config config;
	int ret = 0;
	struct decon_mode_info psr;

	if ((decon->dt.out_type == DECON_OUT_DSI &&
			decon->state == DECON_STATE_INIT) ||
			decon->state == DECON_STATE_OFF) {
		decon_warn("%s: decon%d state(%d), UNBLANK missed\n",
				__func__, decon->id, decon->state);
		return 0;
	}

	decon_dbg("%s: [%d %d %d %d %d %d]\n", __func__,
			var->xoffset, var->yoffset,
			var->xres, var->yres,
			var->xres_virtual, var->yres_virtual);

	memset(&config, 0, sizeof(struct decon_win_config));
	switch (var->bits_per_pixel) {
	case 16:
		config.format = DECON_PIXEL_FORMAT_RGB_565;
		break;
	case 24:
	case 32:
		config.format = DECON_PIXEL_FORMAT_BGRA_8888;
		break;
	default:
		decon_err("%s: Not supported bpp %d\n", __func__,
				var->bits_per_pixel);
		return -EINVAL;
	}

	config.dpp_parm.addr[0] = info->fix.smem_start;
	config.src.x =  var->xoffset;
	config.src.y =  var->yoffset;
	config.src.w = var->xres;
	config.src.h = var->yres;
	config.src.f_w = var->xres_virtual;
	config.src.f_h = var->yres_virtual;
	config.dst.w = config.src.w;
	config.dst.h = config.src.h;
	config.dst.f_w = decon->lcd_info->xres;
	config.dst.f_h = decon->lcd_info->yres;
	if (decon_check_limitation(decon, decon->dt.dft_win, &config) < 0)
		return -EINVAL;

	decon_hiber_block_exit(decon);

	decon_to_psr_info(decon, &psr);

	/*
	 * info->var is old parameters and var is new requested parameters.
	 * var must be copied to info->var before decon_set_par function
	 * is called.
	 *
	 * If not, old parameters are set to window configuration
	 * and new parameters are set to DMA and DPP configuration.
	 */
	memcpy(&info->var, var, sizeof(struct fb_var_screeninfo));
	decon_set_par(info);

	set_bit(decon->dt.dft_idma, &decon->cur_using_dpp);
	set_bit(decon->dt.dft_idma, &decon->prev_used_dpp);
	sd = decon->dpp_sd[decon->dt.dft_idma];
	if (v4l2_subdev_call(sd, core, ioctl, DPP_WIN_CONFIG, &config)) {
		decon_err("%s: Failed to config DPP-%d\n", __func__, win->dpp_id);
		decon_reg_set_win_enable(decon->id, decon->dt.dft_win, false);
		clear_bit(decon->dt.dft_idma, &decon->cur_using_dpp);
		set_bit(decon->dt.dft_idma, &decon->dpp_err_stat);
		goto err;
	}

	decon_reg_update_req_window(decon->id, win->idx);

	decon_reg_start(decon->id, &psr);
err:
	decon_wait_for_vsync(decon, VSYNC_TIMEOUT_MSEC);

	if (decon_reg_wait_update_done_and_mask(decon->id, &psr, SHADOW_UPDATE_TIMEOUT)
			< 0)
		decon_err("%s: wait_for_update_timeout\n", __func__);

	decon_hiber_unblock(decon);
	return ret;
}
EXPORT_SYMBOL(decon_pan_display);

int decon_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	int ret;
	struct decon_win *win = info->par;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#if defined(CONFIG_FB_TEST)
	ret = dma_buf_mmap(win->fb_buf_data.dma_buf, vma, 0);
#else
	ret = dma_buf_mmap(win->dma_buf_data[0].dma_buf, vma, 0);
#endif

	return ret;
}
EXPORT_SYMBOL(decon_mmap);

int decon_exit_hiber(struct decon_device *decon)
{
	int ret = 0;
#ifdef CONFIG_DECON_HIBER
	struct decon_mode_info psr;
	struct decon_param p;

	DPU_EVENT_START();

	decon_dbg("enable decon-%d\n", decon->id);

	decon_hiber_block(decon);
	kthread_flush_worker(&decon->hiber.worker);
	mutex_lock(&decon->hiber.lock);

	if (decon->state != DECON_STATE_HIBER)
		goto err;

	ret = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			DSIM_IOC_ENTER_ULPS, (unsigned long *)0);
	if (ret) {
		decon_warn("starting stream failed for %s\n",
				decon->out_sd[0]->name);
	}

	if (decon->dt.dsi_mode == DSI_MODE_DUAL_DSI) {
		decon_info("enabled 2nd DSIM and LCD for dual DSI mode\n");
		ret = v4l2_subdev_call(decon->out_sd[1], core, ioctl,
				DSIM_IOC_ENTER_ULPS, (unsigned long *)0);
		if (ret) {
			decon_warn("starting stream failed for %s\n",
					decon->out_sd[1]->name);
		}
	}

	decon_to_init_param(decon, &p);
	decon_reg_init(decon->id, decon->dt.out_idx[0], &p);

	/*
	 * After hibernation exit, If panel is partial size, DECON and DSIM
	 * are also set as same partial size.
	 */
	if (!is_full(&decon->win_up.prev_up_region, decon->lcd_info))
		dpu_set_win_update_partial_size(decon, &decon->win_up.prev_up_region);

	if (!decon->id && !decon->eint_status) {
		struct irq_desc *desc = irq_to_desc(decon->res.irq);
		/* Pending IRQ clear */
		if ((!IS_ERR_OR_NULL(desc)) && (desc->irq_data.chip->irq_ack)) {
			desc->irq_data.chip->irq_ack(&desc->irq_data);
			desc->istate &= ~IRQS_PENDING;
		}
		enable_irq(decon->res.irq);
		decon->eint_status = 1;
	}

	decon->state = DECON_STATE_ON;
	decon_to_psr_info(decon, &psr);
	decon_reg_set_int(decon->id, &psr, 1);

	decon_hiber_trig_reset(decon);

	decon->hiber.exit_cnt++;
	DPU_EVENT_LOG(DPU_EVT_EXIT_HIBER, &decon->sd, start);

err:
	decon_hiber_unblock(decon);
	mutex_unlock(&decon->hiber.lock);
#endif
	return ret;
}

int decon_enter_hiber(struct decon_device *decon)
{
	int ret = 0;
#ifdef CONFIG_DECON_HIBER
	struct decon_mode_info psr;

	DPU_EVENT_START();

	mutex_lock(&decon->hiber.lock);

	if (decon_is_enter_shutdown(decon))
		goto err2;

	if (decon_is_hiber_blocked(decon))
		goto err2;

	decon_hiber_block(decon);
	if (decon->state != DECON_STATE_ON)
		goto err;

	decon_hiber_trig_reset(decon);

	kthread_flush_worker(&decon->up.worker);

	decon_to_psr_info(decon, &psr);
	decon_reg_set_int(decon->id, &psr, 0);

	if (!decon->id && (decon->vsync.irq_refcount <= 0) &&
			decon->eint_status) {
		disable_irq(decon->res.irq);
		decon->eint_status = 0;
	}

	ret = decon_reg_stop(decon->id, decon->dt.out_idx[0], &psr);
	if (ret < 0)
		decon_dump(decon);

	decon_reg_clear_int_all(decon->id);

	/* DMA protection disable must be happen on dpp domain is alive */
#if defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
	decon_set_protected_content(decon, NULL);
#endif
	decon->cur_using_dpp = 0;
	decon_dpp_stop(decon, false);

#if defined(CONFIG_EXYNOS9810_BTS)
	decon->bts.ops->bts_release_bw(decon);
#endif

	decon->state = DECON_STATE_HIBER;

	ret = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			DSIM_IOC_ENTER_ULPS, (unsigned long *)1);
	if (ret)
		decon_warn("failed to stop %s\n", decon->out_sd[0]->name);

	if (decon->dt.dsi_mode == DSI_MODE_DUAL_DSI) {
		ret = v4l2_subdev_call(decon->out_sd[1], core, ioctl,
				DSIM_IOC_ENTER_ULPS, (unsigned long *)1);
		if (ret) {
			decon_warn("stopping stream failed for %s\n",
					decon->out_sd[1]->name);
		}
	}

	decon->hiber.enter_cnt++;
	DPU_EVENT_LOG(DPU_EVT_ENTER_HIBER, &decon->sd, start);

err:
	decon_hiber_unblock(decon);
err2:
	mutex_unlock(&decon->hiber.lock);
#endif

	return ret;
}

int decon_hiber_block_exit(struct decon_device *decon)
{
	int ret = 0;

	if (!decon || !decon->hiber.init_status)
		return 0;

	decon_hiber_block(decon);
	ret = decon_exit_hiber(decon);

	return ret;
}

static void decon_hiber_handler(struct kthread_work *work)
{
	struct decon_hiber *hiber =
		container_of(work, struct decon_hiber, work);
	struct decon_device *decon =
		container_of(hiber, struct decon_device, hiber);

	if (!decon || !decon->hiber.init_status)
		return;

	if (decon_hiber_enter_cond(decon))
		decon_enter_hiber(decon);
}

int decon_register_hiber_work(struct decon_device *decon)
{
	struct sched_param param;

	mutex_init(&decon->hiber.lock);

	atomic_set(&decon->hiber.trig_cnt, 0);
	atomic_set(&decon->hiber.block_cnt, 0);

	kthread_init_worker(&decon->hiber.worker);
	decon->hiber.thread = kthread_run(kthread_worker_fn,
			&decon->hiber.worker, "decon_hiber");
	if (IS_ERR(decon->hiber.thread)) {
		decon->hiber.thread = NULL;
		decon_err("failed to run hibernation thread\n");
		return PTR_ERR(decon->hiber.thread);
	}
	param.sched_priority = 20;
	sched_setscheduler_nocheck(decon->hiber.thread, SCHED_FIFO, &param);
	kthread_init_work(&decon->hiber.work, decon_hiber_handler);

	decon->hiber.init_status = true;

	return 0;
}
