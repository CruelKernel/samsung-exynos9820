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
#if defined(CONFIG_SUPPORT_KERNEL_4_9)
#include <linux/sched.h>
#else
#include <linux/sched/types.h>
#endif
#include <linux/of_address.h>
#include <linux/pinctrl/consumer.h>
#include <linux/irq.h>
#include <media/v4l2-subdev.h>
#if defined(CONFIG_EXYNOS_ALT_DVFS)
#include <soc/samsung/exynos-alt.h>
#endif

#include "decon.h"
#include "dsim.h"
#include "dpp.h"
//#include "../../../../soc/samsung/pwrcal/pwrcal.h"
//#include "../../../../soc/samsung/pwrcal/S5E8890/S5E8890-vclk.h"
#include "../../../../../kernel/irq/internals.h"
#ifdef CONFIG_EXYNOS_ALT_DVFS
struct task_struct *devfreq_change_task;
#endif

/* DECON irq handler for DSI interface */
static irqreturn_t decon_irq_handler(int irq, void *dev_data)
{
	struct decon_device *decon = dev_data;
	u32 irq_sts_reg;
	u32 ext_irq = 0;
#if defined(CONFIG_EXYNOS_COMMON_PANEL)
	ktime_t timestamp = ktime_get();
#endif

	spin_lock(&decon->slock);
	if (IS_DECON_OFF_STATE(decon))
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
		decon->hiber.frame_cnt++;
		decon_hiber_trig_reset(decon);
		if (decon->state == DECON_STATE_TUI)
			decon_info("%s:%d TUI Frame Done\n", __func__, __LINE__);
#if defined(CONFIG_EXYNOS_COMMON_PANEL)
		decon->fsync.timestamp = timestamp;
		wake_up_interruptible_all(&decon->fsync.wait);
#endif
	}

	if (ext_irq & DPU_RESOURCE_CONFLICT_INT_PEND)
		DPU_EVENT_LOG(DPU_EVT_RSC_CONFLICT, &decon->sd, ktime_set(0, 0));

	if (ext_irq & DPU_TIME_OUT_INT_PEND) {
		decon_err("%s: DECON%d timeout irq occurs\n", __func__, decon->id);
#if defined(DPU_DUMP_BUFFER_IRQ)
		dpu_dump_afbc_info();
#endif
		BUG();
	}

irq_end:
	spin_unlock(&decon->slock);
	return IRQ_HANDLED;
}

#ifdef CONFIG_EXYNOS_ALT_DVFS
static int decon_devfreq_change_task(void *data)
{
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);

		schedule();

		set_current_state(TASK_RUNNING);

		exynos_alt_call_chain();
	}

	return 0;
}
#endif

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

int decon_get_clocks(struct decon_device *decon)
{
	return 0;
}

void decon_set_clocks(struct decon_device *decon)
{
}

#if defined(CONFIG_EXYNOS_COMMON_PANEL)
int decon_error_cb(struct decon_device *decon,
		struct disp_check_cb_info *info)
{
	int ret = 0;
	int retry = 0;
	int status = 0;
	enum decon_state prev_state;

	if (decon == NULL || info == NULL) {
		decon_warn("%s invalid param\n", __func__);
		return -EINVAL;
	}

	decon_info("%s +\n", __func__);
	decon_bypass_on(decon);
	mutex_lock(&decon->lock);
	prev_state = decon->state;
	if (decon->state == DECON_STATE_OFF) {
		decon_warn("%s decon-%d already off state\n",
				__func__, decon->id);
		decon_bypass_off(decon);
		mutex_unlock(&decon->lock);
		return 0;
	}

	decon_hiber_block_exit(decon);
	kthread_flush_worker(&decon->up.worker);
	usleep_range(16900, 17000);
	if (prev_state == DECON_STATE_DOZE_SUSPEND) {
		ret = _decon_enable(decon, DECON_STATE_DOZE);
		if (ret < 0) {
			decon_err("decon-%d failed to set DECON_STATE_DOZE (ret %d)\n",
					decon->id, ret);
		}
	}

	while (++retry <= DISP_ERROR_CB_RETRY_CNT) {
		decon_warn("%s try recovery(%d times)\n",
				__func__, retry);
		if (IS_DECON_DOZE_STATE(decon)) {
			ret = _decon_disable(decon, DECON_STATE_OFF);
			if (ret < 0) {
				decon_err("decon-%d failed to set DECON_STATE_OFF (ret %d)\n",
						decon->id, ret);
			}

			status = disp_check_status(info);
			if (IS_DISP_CHECK_STATUS_DISCONNECTED(status)) {
				decon_info("%s ub disconnected(status 0x%x)\n",
						__func__, status);
				break;
			}

			ret = _decon_enable(decon, DECON_STATE_DOZE);
			if (ret < 0) {
				decon_err("decon-%d failed to set DECON_STATE_DOZE (ret %d)\n",
						decon->id, ret);
			}
		} else {
			ret = _decon_disable(decon, DECON_STATE_OFF);
			if (ret < 0) {
				decon_err("decon-%d failed to set DECON_STATE_OFF (ret %d)\n",
						decon->id, ret);
			}

			status = disp_check_status(info);
			if (IS_DISP_CHECK_STATUS_DISCONNECTED(status)) {
				decon_info("%s ub disconnected(status 0x%x)\n",
						__func__, status);
				break;
			}

			ret = _decon_enable(decon, DECON_STATE_ON);
			if (ret < 0) {
				decon_err("decon-%d failed to set DECON_STATE_ON (ret %d)\n",
						decon->id, ret);
			}
		}

		/*
		 * disp_check_status() will check only not-ok status
		 * if not go through and try update last regs
		 */
		status = disp_check_status(info);
		if (IS_DISP_CHECK_STATUS_NOK(status)) {
			decon_err("%s failed to recovery subdev(status 0x%x)\n",
					__func__, status);
			continue;
		}

		DPU_FULL_RECT(&decon->last_regs.up_region, decon->lcd_info);
		decon->last_regs.need_update = true;
		ret = decon_update_last_regs(decon, &decon->last_regs);
		if (ret < 0) {
			decon_err("%s failed to update last image(ret %d)\n",
					__func__, ret);
			continue;
		}
		decon_info("%s recovery successfully(retry %d times)\n",
				__func__, retry);
		break;
	}

	if (retry > DISP_ERROR_CB_RETRY_CNT) {
		decon_err("DECON:ERR:%s:failed to recover(retry %d times)\n",
				__func__, DISP_ERROR_CB_RETRY_CNT);
		decon_dump(decon, REQ_DSI_DUMP);
#ifdef CONFIG_LOGGING_BIGDATA_BUG
		log_decon_bigdata(decon);
#endif
		BUG();
	}

	if (prev_state == DECON_STATE_DOZE_SUSPEND) {
		/* sleep for image update & display on */
		msleep(34);
		ret = _decon_disable(decon, DECON_STATE_DOZE_SUSPEND);
		if (ret < 0) {
			decon_err("decon-%d failed to set DECON_STATE_DOZE_SUSPEND (ret %d)\n",
					decon->id, ret);
		}
	}

	decon_bypass_off(decon);
	decon_hiber_unblock(decon);
	mutex_unlock(&decon->lock);
	decon_info("%s -\n", __func__);

	return ret;
}
#endif

int decon_get_out_sd(struct decon_device *decon)
{
#if defined(CONFIG_EXYNOS_COMMON_PANEL)
	struct disp_error_cb_info decon_error_cb_info = {
		.error_cb = (disp_error_cb *)decon_error_cb,
		.data = decon,
	};
#endif

#ifdef CONFIG_DYNAMIC_FREQ
	int ret = 0;
	struct df_status_info *status;
#endif

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

#ifdef CONFIG_DYNAMIC_FREQ
	ret = v4l2_subdev_call(decon->panel_sd, core, ioctl,
		PANEL_IOC_GET_DF_STATUS, NULL);
	if (ret < 0) {
		decon_err("DECON:ERR:%s:failed to get df status\n", __func__);
		goto err_get_df;
	}
	status = (struct df_status_info*)v4l2_get_subdev_hostdata(decon->panel_sd);
	if (status != NULL)
		decon->df_status = status;

	decon_info("[DYN_FREQ]:INFO:%s:req,tar,cur:%d,%d:%d\n", __func__,
		decon->df_status->request_df, decon->df_status->target_df, decon->df_status->current_df);
err_get_df:
#endif
	decon_info("lcd_info: hfp %d hbp %d hsa %d vfp %d vbp %d vsa %d",
			decon->lcd_info->hfp, decon->lcd_info->hbp,
			decon->lcd_info->hsa, decon->lcd_info->vfp,
			decon->lcd_info->vbp, decon->lcd_info->vsa);
	decon_info("xres %d yres %d\n",
			decon->lcd_info->xres, decon->lcd_info->yres);
#ifdef CONFIG_EXYNOS_COMMON_PANEL
	v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			DSIM_IOC_SET_ERROR_CB, &decon_error_cb_info);

	if (IS_ERR_OR_NULL(decon->panel_sd)) {
		decon_err("DECON:ERR:%s:failed to get panel'sd", __func__);
		return -ENOMEM;
	}
	v4l2_subdev_call(decon->panel_sd, core, ioctl,
		PANEL_IOC_GET_PANEL_STATE, NULL);
	decon->panel_state =
		(struct panel_state *)v4l2_get_subdev_hostdata(decon->panel_sd);
	if (decon->panel_state == NULL) {
		decon_err("DECON:ERR:%s:failed to set subdev hostdata\n",
			__func__);
		return -EINVAL;
	}
	decon_info("DECON:INFO:%s:decon connected state : %d\n",
		__func__, decon->panel_state->connect_panel);
#endif

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

	if (decon->hiber.enabled && decon->state == DECON_STATE_ON &&
			decon->dt.out_type == DECON_OUT_DSI) {
		if (decon_min_lock_cond(decon)) {
			decon->hiber.timestamp = timestamp;
#if defined(CONFIG_EXYNOS_HIBERNATION_THREAD)
			wake_up_interruptible_all(&decon->hiber.wait);
#else
			if (list_empty(&decon->hiber.worker.work_list)) {
				atomic_inc(&decon->hiber.remaining_hiber);
				kthread_queue_work(&decon->hiber.worker, &decon->hiber.work);
			}
#endif
		}
	}

	decon_systrace(decon, 'C', "decon_te_signal", 0);
	decon->vsync.timestamp = timestamp;
	wake_up_interruptible_all(&decon->vsync.wait);

	spin_unlock(&decon->slock);
#ifdef CONFIG_EXYNOS_ALT_DVFS
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

#ifdef CONFIG_EXYNOS_ALT_DVFS
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
#if defined(CONFIG_SUPPORT_KERNEL_4_9)
		int ret = wait_event_interruptible(decon->vsync.wait,
			!ktime_equal(timestamp, decon->vsync.timestamp) &&
			decon->vsync.active);
#else
		int ret = wait_event_interruptible(decon->vsync.wait,
			(timestamp != decon->vsync.timestamp) &&
			decon->vsync.active);
#endif
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


#if defined(CONFIG_EXYNOS_COMMON_PANEL)
static int decon_fsync_thread(void *data)
{
	struct decon_device *decon = data;
	struct v4l2_event ev;
	ktime_t timestamp;
	int num_dsi = (decon->dt.dsi_mode == DSI_MODE_DUAL_DSI) ? 2 : 1;
	int ret, i;

	while (!kthread_should_stop()) {
		timestamp = decon->fsync.timestamp;

#if defined(CONFIG_SUPPORT_KERNEL_4_9)
		ret = wait_event_interruptible(decon->fsync.wait,
			!ktime_equal(timestamp, decon->fsync.timestamp) &&
			decon->fsync.active);
#else
		ret = wait_event_interruptible(decon->fsync.wait,
			(timestamp != decon->fsync.timestamp) &&
			decon->fsync.active);
#endif
		if (!ret) {
			ev.timestamp = ktime_to_timespec(decon->fsync.timestamp);
			ev.type = V4L2_EVENT_DECON_FRAME_DONE;
			for (i = 0; i < num_dsi; i++) {
				v4l2_subdev_call(decon->out_sd[i], core, ioctl,
						DSIM_IOC_NOTIFY, (void *)&ev);
				decon_dbg("%s notify frame done event\n", __func__);
			}
		}
	}

	return 0;
}

int decon_create_fsync_thread(struct decon_device *decon)
{
	char name[16];

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_info("frame sync thread is only needed for DSI path\n");
		return 0;
	}

	sprintf(name, "decon%d-fsync", decon->id);
	decon->fsync.thread = kthread_run(decon_fsync_thread, decon, name);
	if (IS_ERR_OR_NULL(decon->fsync.thread)) {
		decon_err("failed to run fsync thread\n");
		decon->fsync.thread = NULL;
		return PTR_ERR(decon->fsync.thread);
	}

	return 0;
}

void decon_destroy_fsync_thread(struct decon_device *decon)
{
	if (decon->fsync.thread)
		kthread_stop(decon->fsync.thread);
}
#endif

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
	int len, slice_w, slice_h;

	len = sprintf(p, "%d\n", decon->dt.psr_mode);
	len += sprintf(p + len, "%d\n", mres_info->mres_number);
	for (i = 0; i < mres_info->mres_number; i++) {
		slice_w = 0;
		slice_h = 0;
		if (mres_info->res_info[i].dsc_en)
			slice_w = mres_info->res_info[i].dsc_width;
		else if (lcd_info->partial_width[i])
			slice_w = lcd_info->partial_width[i];

		if (mres_info->res_info[i].dsc_en)
			slice_h = mres_info->res_info[i].dsc_height;
		else if (lcd_info->partial_height[i])
			slice_h = lcd_info->partial_height[i];

		if (slice_w < MIN_WIN_BLOCK_WIDTH)
			slice_w = MIN_WIN_BLOCK_WIDTH;
		if (slice_h < MIN_WIN_BLOCK_HEIGHT)
			slice_h = MIN_WIN_BLOCK_HEIGHT;

		len += sprintf(p + len, "%d\n%d\n%d\n%d\n%d\n",
				mres_info->res_info[i].width,
				mres_info->res_info[i].height,
				slice_w, slice_h,
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

#if defined(CONFIG_EXYNOS_COMMON_PANEL)


static char *idma_to_string(enum decon_idma_type type)
{
	switch (type) {
	case 0:
		return "IDMA_GF0";
	case 1:
		return "IDMA_VGRFS";
	case 2:
		return "IDMA_GF1";
	case 3:
		return "IDMA_VGF";
	case 4:
		return "IDMA_VG";
	case 5:
		return "IDMA_VGS";
	default:
		return "INVALID";
	}
}


static char *format_to_string(enum decon_pixel_format format)
{
	switch(format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
		return "ARGB_8888";
	case DECON_PIXEL_FORMAT_ABGR_8888:
		return 
"ABGR_8888";
	case DECON_PIXEL_FORMAT_RGBA_8888:
		return "RGBA_8888";
	case DECON_PIXEL_FORMAT_BGRA_8888:
		return "BGRA_8888";
	case DECON_PIXEL_FORMAT_XRGB_8888:
		return "XRGB_8888";
	case DECON_PIXEL_FORMAT_XBGR_8888:
		return "XBGR_8888";
	case DECON_PIXEL_FORMAT_RGBX_8888:
		return "RGBX_8888";
	case DECON_PIXEL_FORMAT_BGRX_8888:
		return "BGRX_8888";
	case DECON_PIXEL_FORMAT_RGBA_5551:
		return "RGBA_5551";
	case DECON_PIXEL_FORMAT_BGRA_5551:
		return "BGRA_5551";
	case DECON_PIXEL_FORMAT_ABGR_4444:
		return "ABGR_4444";
	case DECON_PIXEL_FORMAT_RGBA_4444:
		return "RGBA_4444";
	case DECON_PIXEL_FORMAT_BGRA_4444:
		return "BGRA_4444";
	case DECON_PIXEL_FORMAT_RGB_565:
		return "RGB_565";
	case DECON_PIXEL_FORMAT_BGR_565:
		return "BGR_565";
	case DECON_PIXEL_FORMAT_ARGB_2101010:
		return "ARGB_2101010";
	case DECON_PIXEL_FORMAT_ABGR_2101010:
		return "ABGR_2101010";
	case DECON_PIXEL_FORMAT_RGBA_1010102:
		return "RGBA_1010102";
	case DECON_PIXEL_FORMAT_BGRA_1010102:
		return "BGRA_1010102";
	case DECON_PIXEL_FORMAT_NV16:
		return "NV16";
	case DECON_PIXEL_FORMAT_NV61:
		return "NV61";
	case DECON_PIXEL_FORMAT_YVU422_3P:
		return "YVU422_3P";
	case DECON_PIXEL_FORMAT_NV12:
		return "NV12";
	case DECON_PIXEL_FORMAT_NV21:
		return "NV21";
	case DECON_PIXEL_FORMAT_NV12M:
		return "NV12M";
	case DECON_PIXEL_FORMAT_NV21M:
		return "NV21M";
	case DECON_PIXEL_FORMAT_YUV420:
		return "YUV420";
	case DECON_PIXEL_FORMAT_YVU420:
		return "YVU420";
	case DECON_PIXEL_FORMAT_YUV420M:
		return "YUV420M";
	case DECON_PIXEL_FORMAT_YVU420M:
		return "YVU420M";
	case DECON_PIXEL_FORMAT_NV12N:
		return "NV12N";
	case DECON_PIXEL_FORMAT_NV12N_10B:
		return "NV12N_10B";
	case DECON_PIXEL_FORMAT_NV12M_P010:
		return "NV12M_P010";
	case DECON_PIXEL_FORMAT_NV21M_P010:
		return "NV21M_P010";
	case DECON_PIXEL_FORMAT_NV12M_S10B:
		return "NV12M_S10B";
	case DECON_PIXEL_FORMAT_NV21M_S10B:
		return "NV21M_S10B";
	case DECON_PIXEL_FORMAT_NV16M_P210:
		return "NV16M_P210";
	case DECON_PIXEL_FORMAT_NV61M_P210:
		return "NV61M_P210";
	case DECON_PIXEL_FORMAT_NV16M_S10B:
		return "NV16M_S10B";
	case DECON_PIXEL_FORMAT_NV61M_S10B:
		return "NV61M_S10B";
	case DECON_PIXEL_FORMAT_NV12_P010:
		return "NV12_P010";
	default:
		return "INVALID_FMT";
	}
}


static char *comp_src_to_string(enum dpp_comp_src src)
{
	switch(src) {
	case DPP_COMP_SRC_G2D:
		return "G2D";
	case DPP_COMP_SRC_GPU:
		return "GPU";
	default:
		return "NONE";
	}
}

static char *rot_to_string(enum dpp_rotate rot)
{
	switch(rot) {
	case DPP_ROT_NORMAL:
		return "NORMAL";
	case DPP_ROT_XFLIP:
		return "XFLIP";
	case DPP_ROT_YFLIP:
		return "YFLIP";
	case DPP_ROT_180:
		return "180";
	case DPP_ROT_90:
		return "90";
	case DPP_ROT_90_XFLIP:
		return "90_XFLIP";
	case DPP_ROT_90_YFLIP:
		return "90_YFLIP";
	case DPP_ROT_270:
		return "270";
	default:
		return "UNDEFINED";
	}
}

static ssize_t decon_show_last_update(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	int len = 0;
	struct decon_win_config *win_config;
	struct decon_device *decon = dev_get_drvdata(dev);
	//struct decon_reg_data *last_update = &decon->last_regs;

	len = sprintf(buf, "============================== LAST UPDATE ==============================\n");
	len += sprintf(buf + len, "Color Mode : %d\n", decon->color_mode);

	for (i = 0; i < decon->dt.max_win; i++) {
		//win_config = last_update.dpp_config[i];
		win_config = &decon->last_regs.dpp_config[i];

		len += sprintf(buf + len, "* Win: %d: State:", i);
		
		switch (win_config->state) {
		case DECON_WIN_STATE_DISABLED:
			len += sprintf(buf + len, "DISABLED\n");
			break;
		case DECON_WIN_STATE_COLOR:
			len += sprintf(buf + len, "COLOR: %x\n", win_config->color);
			break;
		case DECON_WIN_STATE_BUFFER:
			len += sprintf(buf + len, "BUFFER, ");
			len += sprintf(buf + len, "DPP:%12s, format:%14s ", 
				idma_to_string(win_config->idma_type),
				format_to_string(win_config->format));

			if (win_config->compression) {
				 len += sprintf(buf + len, "afbc :%d, src :%s",
				 	win_config->compression,
				 	comp_src_to_string(win_config->dpp_parm.comp_src));
			}

			len += sprintf(buf + len, "\n\t  SRC: (%4d,%4d), (%4d,%4d)   ",
				win_config->src.x, win_config->src.y,
				win_config->src.x + win_config->src.w,
				win_config->src.y + win_config->src.h);

			len += sprintf(buf + len, "DST: (%4d,%4d), (%4d,%4d)",
				win_config->dst.x, win_config->dst.y,
				win_config->dst.x + win_config->dst.w,
				win_config->dst.y + win_config->dst.h);

			len += sprintf(buf + len, "\n\t  EQ: %x, HDR: %x, Rot :%s\n",
				win_config->dpp_parm.eq_mode,
				win_config->dpp_parm.hdr_std,
				rot_to_string(win_config->dpp_parm.rot));
			break;
		case DECON_WIN_STATE_UPDATE:
			len += sprintf(buf + len, "UPDATE\n");
			break;
		case DECON_WIN_STATE_CURSOR:
			len += sprintf(buf + len, "CURSOR\n");
			break;
		case DECON_WIN_STATE_MRESOL:
			len += sprintf(buf + len, "MRESOL\n");
			break;
		default:
			len += sprintf(buf + len, "Invalid state : %d\n", win_config->state);
			break;
		}
	}

//exit_show:
	return len;
}

static DEVICE_ATTR(last_update, S_IRUGO, decon_show_last_update, NULL);

int decon_create_last_info(struct decon_device *decon)
{
	int ret = 0;

	if (decon->id != 0)
		goto create_exit;

	ret = device_create_file(decon->dev, &dev_attr_last_update);
	if (ret)
		decon_err("DECON:ERR:%s:failed to create last update\n", __func__);

create_exit:
	return ret;
}

void decon_destroy_last_info(struct decon_device *decon)
{
	device_remove_file(decon->dev, &dev_attr_last_update);
}


#endif
	



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

	if ((!IS_DECON_HIBER_STATE(decon) && IS_DECON_OFF_STATE(decon)) ||
			decon->state == DECON_STATE_INIT)
		return 0;

	memset(&win_regs, 0, sizeof(struct decon_window_regs));

	decon_hiber_block_exit(decon);

	decon_reg_wait_update_done_timeout(decon->id, SHADOW_UPDATE_TIMEOUT);
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
	win_regs.ch = decon->dt.dft_ch;
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

	if (IS_DECON_OFF_STATE(decon))
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
	int dpp_id = decon->dt.dft_ch;
	struct dpp_config dpp_config;
	unsigned long aclk_khz;
	struct decon_reg_data regs;

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_warn("%s: decon%d unspported on out_type(%d)\n",
				__func__, decon->id, decon->dt.out_type);
		return 0;
	}

	if ((!IS_DECON_HIBER_STATE(decon) && IS_DECON_OFF_STATE(decon)) ||
			decon->state == DECON_STATE_INIT) {
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
		config.format = DECON_PIXEL_FORMAT_ABGR_8888;
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
	config.state = DECON_WIN_STATE_BUFFER;
#ifdef CONFIG_EXYNOS_MCD_HDR
	config.wcg_mode = decon->color_mode;
#endif

	if (decon_check_limitation(decon, decon->dt.dft_win, &config) < 0)
		return -EINVAL;

	decon_hiber_block_exit(decon);

	decon_to_psr_info(decon, &psr);

	memset(&regs, 0, sizeof(struct decon_reg_data));
	memcpy(&regs.dpp_config[0], &config, sizeof(struct decon_win_config));
#if defined(CONFIG_EXYNOS_BTS)
	/* add calc and update bw : cur > prev */
	decon->bts.ops->bts_calc_bw(decon, &regs);
	decon->bts.ops->bts_update_bw(decon, &regs, 0);
#endif

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

	set_bit(dpp_id, &decon->cur_using_dpp);
	set_bit(dpp_id, &decon->prev_used_dpp);
	sd = decon->dpp_sd[dpp_id];

	aclk_khz = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			EXYNOS_DPU_GET_ACLK, NULL) / 1000U;

	memcpy(&dpp_config.config, &config, sizeof(struct decon_win_config));
	dpp_config.rcv_num = aclk_khz;

	if (v4l2_subdev_call(sd, core, ioctl, DPP_WIN_CONFIG, &dpp_config)) {
		decon_err("%s: Failed to config DPP-%d\n", __func__, win->dpp_id);
		decon_reg_win_enable_and_update(decon->id, decon->dt.dft_win, false);
		clear_bit(dpp_id, &decon->cur_using_dpp);
		set_bit(dpp_id, &decon->dpp_err_stat);
		goto err;
	}

	decon_reg_all_win_shadow_update_req(decon->id);

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
	struct decon_param p;
	struct decon_mode_info psr;
	enum decon_state prev_state;

	DPU_EVENT_START();

	if (!decon->hiber.enabled)
		return 0;

	decon_hiber_block(decon);


#if !defined(CONFIG_EXYNOS_HIBERNATION_THREAD)
	if (atomic_read(&decon->hiber.remaining_hiber))
		kthread_flush_worker(&decon->hiber.worker);
#endif
	mutex_lock(&decon->hiber.lock);
	prev_state = decon->state;

	if (decon->state != DECON_STATE_HIBER)
		goto err;

	decon_dbg("enable decon-%d\n", decon->id);

	ret = decon_set_out_sd_state(decon, DECON_STATE_ON);
	if (ret < 0) {
		decon_err("%s decon-%d failed to set subdev EXIT_ULPS state\n",
				__func__, decon->id);
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

	decon_dbg("decon-%d %s - (state:%d -> %d)\n",
			decon->id, __func__, prev_state, decon->state);
	decon->hiber.exit_cnt++;
	DPU_EVENT_LOG(DPU_EVT_EXIT_HIBER, &decon->sd, start);
	decon_hiber_finish(decon);

err:
	decon_hiber_unblock(decon);
	mutex_unlock(&decon->hiber.lock);

	return ret;
}

int decon_enter_hiber(struct decon_device *decon)
{
	int ret = 0;
	struct decon_mode_info psr;
	enum decon_state prev_state;

	DPU_EVENT_START();

	if (!decon->hiber.enabled)
		return 0;

	mutex_lock(&decon->hiber.lock);
	prev_state = decon->state;

	if (decon_is_enter_shutdown(decon))
		goto err2;

	if (decon_is_hiber_blocked(decon))
		goto err2;

	decon_hiber_block(decon);
	if (decon->state != DECON_STATE_ON)
		goto err;

	decon_dbg("decon-%d %s +\n", decon->id, __func__);
	decon_hiber_trig_reset(decon);

	if (atomic_read(&decon->up.remaining_frame))
		kthread_flush_worker(&decon->up.worker);

	decon_to_psr_info(decon, &psr);
	decon_reg_set_int(decon->id, &psr, 0);

	if (!decon->id && (decon->vsync.irq_refcount <= 0) &&
			decon->eint_status) {
		disable_irq(decon->res.irq);
		decon->eint_status = 0;
	}

	ret = decon_reg_stop(decon->id, decon->dt.out_idx[0], &psr, true,
			decon->lcd_info->fps);
	if (ret < 0)
		decon_dump(decon, REQ_DSI_DUMP);

	/* DMA protection disable must be happen on dpp domain is alive */
	if (decon->dt.out_type != DECON_OUT_WB) {
#if defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
		decon_set_protected_content(decon, NULL);
#endif
		decon->cur_using_dpp = 0;
		decon_dpp_stop(decon, false);
	}

#if defined(CONFIG_EXYNOS_BTS)
	decon->bts.ops->bts_release_bw(decon);
#endif

	ret = decon_set_out_sd_state(decon, DECON_STATE_HIBER);
	if (ret < 0)
		decon_err("%s decon-%d failed to set subdev ENTER_ULPS state\n",
				__func__, decon->id);

	decon->state = DECON_STATE_HIBER;

	decon->hiber.enter_cnt++;
	DPU_EVENT_LOG(DPU_EVT_ENTER_HIBER, &decon->sd, start);
	decon_hiber_start(decon);

err:
	decon_hiber_unblock(decon);
err2:
	mutex_unlock(&decon->hiber.lock);

	decon_dbg("decon-%d %s - (state:%d -> %d)\n",
			decon->id, __func__, prev_state, decon->state);

	return ret;
}

int decon_hiber_block_exit(struct decon_device *decon)
{
	int ret = 0;

	if (!decon || !decon->hiber.enabled)
		return 0;

	decon_hiber_block(decon);
	ret = decon_exit_hiber(decon);

	return ret;
}

#if defined(CONFIG_EXYNOS_HIBERNATION_THREAD)
static int decon_hiber_thread(void *data)
{
	struct decon_device *decon = data;
	ktime_t timestamp;
	int ret;

	while (!kthread_should_stop()) {
		timestamp = decon->hiber.timestamp;

#if defined(CONFIG_SUPPORT_KERNEL_4_9)
		ret = wait_event_interruptible(decon->hiber.wait,
				!ktime_equal(timestamp, decon->hiber.timestamp)
				&& decon->hiber.enabled);
#else
		ret = wait_event_interruptible(decon->hiber.wait,
				(timestamp != decon->hiber.timestamp)
				&& decon->hiber.enabled);
#endif
		if (!ret) {
			if (decon_hiber_enter_cond(decon))
				decon_enter_hiber(decon);
		}
	}

	return 0;
}

int decon_register_hiber_work(struct decon_device *decon)
{
	struct sched_param param;

	mutex_init(&decon->hiber.lock);

	decon->hiber.enabled = false;

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_info("hiber thread is only needed for DSI path\n");
		return 0;
	}

	if (!IS_ENABLED(CONFIG_EXYNOS_HIBERNATION)) {
		decon_info("display doesn't support hibernation mode\n");
		return 0;
	}

	atomic_set(&decon->hiber.trig_cnt, 0);
	atomic_set(&decon->hiber.block_cnt, 0);

	decon->hiber.thread = kthread_run(decon_hiber_thread,
			decon, "decon_hiber");
	if (IS_ERR(decon->hiber.thread)) {
		decon->hiber.thread = NULL;
		decon_err("failed to run hibernation thread\n");
		return PTR_ERR(decon->hiber.thread);
	}
	param.sched_priority = 20;
	sched_setscheduler_nocheck(decon->hiber.thread, SCHED_FIFO, &param);

	decon->hiber.hiber_enter_cnt = DECON_ENTER_HIBER_CNT;
	decon->hiber.enabled = true;
	decon_info("display supports hibernation mode\n");

	return 0;
}

void decon_destroy_hiber_thread(struct decon_device *decon)
{
	if (decon->hiber.thread)
		kthread_stop(decon->hiber.thread);
}
#else
static void decon_hiber_handler(struct kthread_work *work)
{
	struct decon_hiber *hiber =
		container_of(work, struct decon_hiber, work);
	struct decon_device *decon =
		container_of(hiber, struct decon_device, hiber);

	if (!decon || !decon->hiber.enabled) {
		atomic_dec(&decon->hiber.remaining_hiber);
		return;
	}

	if (decon_hiber_enter_cond(decon))
		decon_enter_hiber(decon);

	atomic_dec(&decon->hiber.remaining_hiber);
}

int decon_register_hiber_work(struct decon_device *decon)
{
	struct sched_param param;

	mutex_init(&decon->hiber.lock);

	decon->hiber.enabled = false;

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_info("hiber thread is only needed for DSI path\n");
		return 0;
	}

	if (!IS_ENABLED(CONFIG_EXYNOS_HIBERNATION)) {
		decon_info("display doesn't support hibernation mode\n");
		return 0;
	}

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

	decon->hiber.hiber_enter_cnt = DECON_ENTER_HIBER_CNT;
	decon->hiber.enabled = true;
	decon_info("display supports hibernation mode\n");

	return 0;
}
#endif

#ifdef CONFIG_EXYNOS_COMMON_PANEL
static int decon_doze_suspend_thread(void *data)
{
	struct decon_device *decon = data;
	ktime_t timestamp;
	int ret;

	while (!kthread_should_stop()) {
		timestamp = decon->doze_hiber.doze_suspend_timestamp;
#if defined(CONFIG_SUPPORT_KERNEL_4_9)
		ret = wait_event_interruptible(decon->doze_hiber.doze_suspend_wait,
				!ktime_equal(timestamp, decon->doze_hiber.doze_suspend_timestamp)
				&& decon->doze_hiber.enabled);
#else
		ret = wait_event_interruptible(decon->doze_hiber.doze_suspend_wait,
				(timestamp != decon->doze_hiber.doze_suspend_timestamp)
				&& decon->doze_hiber.enabled);
#endif

		if (!ret) {
			mutex_lock(&decon->pwr_state_lock);
			mutex_lock(&decon->doze_hiber.lock);
			if (decon_doze_suspend_enter_cond(decon)) {
				decon_dbg("%s (%s) cnt:%d +\n",
						__func__, get_decon_state_name(decon->state),
						atomic_read(&decon->doze_hiber.block_cnt));
				ret = decon_doze_suspend(decon);
				if (ret < 0)
					pr_warn("%s failed to doze suspend\n", __func__);
				decon_dbg("%s (%s) cnt:%d -\n",
						__func__, get_decon_state_name(decon->state),
						atomic_read(&decon->doze_hiber.block_cnt));
			}
			mutex_unlock(&decon->doze_hiber.lock);
			mutex_unlock(&decon->pwr_state_lock);
		}
	}

	return 0;
}

static int decon_doze_wake_thread(void *data)
{
	struct decon_device *decon = data;
	ktime_t timestamp;
	int ret;

	while (!kthread_should_stop()) {
		timestamp = decon->doze_hiber.doze_wake_timestamp;
#if defined(CONFIG_SUPPORT_KERNEL_4_9)
		ret = wait_event_interruptible(decon->doze_hiber.doze_wake_wait,
				!ktime_equal(timestamp, decon->doze_hiber.doze_wake_timestamp)
				&& decon->doze_hiber.enabled);
#else
		ret = wait_event_interruptible(decon->doze_hiber.doze_wake_wait,
				(timestamp != decon->doze_hiber.doze_wake_timestamp)
				&& decon->doze_hiber.enabled);
#endif

		if (!ret) {
			mutex_lock(&decon->pwr_state_lock);
			mutex_lock(&decon->doze_hiber.lock);
			if (decon->state == DECON_STATE_DOZE_SUSPEND) {
				ret = decon_doze_wake(decon);
				if (ret < 0)
					pr_warn("%s failed to doze wake\n", __func__);
			}
			mutex_unlock(&decon->doze_hiber.lock);
			mutex_unlock(&decon->pwr_state_lock);
		}
	}

	return 0;
}

int decon_register_doze_hiber_work(struct decon_device *decon)
{
	struct sched_param param;

	mutex_init(&decon->doze_hiber.lock);

	decon->doze_hiber.enabled = false;

	if (decon->dt.out_type != DECON_OUT_DSI) {
		decon_info("doze_hiber thread is only needed for DSI path\n");
		return 0;
	}

	atomic_set(&decon->doze_hiber.trig_cnt, 0);
	atomic_set(&decon->doze_hiber.block_cnt, 0);

	/* enter doze suspend thread */
	decon->doze_hiber.doze_suspend_thread = kthread_run(decon_doze_suspend_thread,
			decon, "decon_doze_suspend");
	if (IS_ERR(decon->doze_hiber.doze_suspend_thread)) {
		decon->doze_hiber.doze_suspend_thread = NULL;
		decon_err("failed to run doze_suspend thread\n");
		return PTR_ERR(decon->doze_hiber.doze_suspend_thread);
	}
	param.sched_priority = 20;
	sched_setscheduler_nocheck(decon->doze_hiber.doze_suspend_thread, SCHED_FIFO, &param);

	/* enter doze wake thread */
	decon->doze_hiber.doze_wake_thread = kthread_run(decon_doze_wake_thread,
			decon, "decon_doze_wake");
	if (IS_ERR(decon->doze_hiber.doze_wake_thread)) {
		decon->doze_hiber.doze_wake_thread = NULL;
		decon_err("failed to run doze_wake thread\n");
		return PTR_ERR(decon->doze_hiber.doze_wake_thread);
	}
	param.sched_priority = 20;
	sched_setscheduler_nocheck(decon->doze_hiber.doze_wake_thread, SCHED_FIFO, &param);

	decon->doze_hiber.enabled = true;

	return 0;
}

void decon_destroy_doze_hiber_thread(struct decon_device *decon)
{
	if (decon->doze_hiber.doze_suspend_thread)
		kthread_stop(decon->doze_hiber.doze_suspend_thread);
	if (decon->doze_hiber.doze_wake_thread)
		kthread_stop(decon->doze_hiber.doze_wake_thread);
}
#endif

void decon_init_low_persistence_mode(struct decon_device *decon)
{
	decon->low_persistence = false;

	if (!IS_ENABLED(CONFIG_EXYNOS_LOW_PERSISTENCE)) {
		decon_info("display doesn't support low persistence mode\n");
		return;
	}

	decon->low_persistence = true;
	decon_info("display supports low persistence mode\n");
}

void dpu_init_freq_hop(struct decon_device *decon)
{
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	if (IS_ENABLED(CONFIG_EXYNOS_FREQ_HOP)) {
		decon->freq_hop.enabled = true;
		decon->freq_hop.target_m = decon->lcd_info->dphy_pms.m;
		decon->freq_hop.request_m = decon->lcd_info->dphy_pms.m;
		decon->freq_hop.target_k = decon->lcd_info->dphy_pms.k;
		decon->freq_hop.request_k = decon->lcd_info->dphy_pms.k;
	} else {
		decon->freq_hop.enabled = false;
	}
#endif
}

void dpu_update_freq_hop(struct decon_device *decon)
{
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	if (!decon->freq_hop.enabled)
		return;

	decon->freq_hop.target_m = decon->freq_hop.request_m;
	decon->freq_hop.target_k = decon->freq_hop.request_k;
#endif
}

#ifdef CONFIG_DYNAMIC_FREQ

int decon_panel_ioc_update_ffc(struct decon_device *decon)
{
	int ret = 0;

	ret = v4l2_subdev_call(decon->panel_sd, core, ioctl,
		PANEL_IOC_DYN_FREQ_FFC, NULL);

	return ret;
}

int dpu_finish_df_update(struct decon_device *decon)
{
	int ret = 0;
	struct df_status_info *status = decon->df_status;

	if (status->current_df == status->target_df)
		panel_info("[DYN_FREQ]:WARN:%s: cur_df:%d, tar_df:%d\n", __func__,
			status->current_df, status->target_df);

	status->current_df = status->target_df;
	status->context = 0;

	return ret;
}

static int dpu_check_df_update(struct decon_device *decon)
{
	int ret = 0;

	if (decon->df_status != NULL) {
		if ((decon->df_status->request_df != decon->df_status->target_df) ||
			(decon->df_status->request_df != decon->df_status->ffc_df)) {
			decon->df_status->target_df = decon->df_status->request_df;
			ret = MAGIC_DF_UPDATED;
		}
	}
	return ret;
}

static int dpu_set_pre_df_dsim(struct decon_device *decon)
{
	int ret = 0;
	struct df_param param;
	struct df_setting_info *df_set;
	struct df_status_info *status = decon->df_status;

	/* copy default value */
	memcpy(&param.pms, &decon->lcd_info->dphy_pms,
		sizeof(struct stdphy_pms));

	param.context = status->context;

	df_set = &decon->lcd_info->df_set_info.setting_info[status->target_df];
	if (df_set == NULL) {
		decon_err("[DYN_FREQ]:ERR:%s:df setting value is null\n",
			__func__);
		return -EINVAL;
	}
	if (df_set->hs == 0) {
		decon_err("[DYN_FREQ]:ERR:%s:df index : %d hs is 0 : %d\n",
			__func__, status->target_df);
		return -EINVAL;
	}

	/* update value for freq hop */
	param.pms.m = df_set->dphy_pms.m;
	param.pms.k = df_set->dphy_pms.k;

	if (status->context)
		dsim_info("[DYN_FREQ]:INFO:%s:target hs : %d, m : %d, k:%d\n", 
			__func__, df_set->hs, param.pms.m, param.pms.k);

	ret = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			DSIM_IOC_SET_PRE_FREQ_HOP, &param);

	return ret;
}

static int dpu_set_post_df_dsim(struct decon_device *decon)
{
	int ret = 0;
	struct df_param param;

	/* copy default value */
	memcpy(&param.pms, &decon->lcd_info->dphy_pms,
		sizeof(struct stdphy_pms));

	if (decon->df_status->context)
		dsim_info("[DYN_FREQ]:INFO:%s:m : %d, k:%d\n", 
			__func__, param.pms.m, param.pms.k);

	ret = v4l2_subdev_call(decon->out_sd[0], core, ioctl,
			DSIM_IOC_SET_POST_FREQ_HOP, &param);

	return ret;
}

void dpu_set_freq_hop(struct decon_device *decon, struct decon_reg_data *regs, bool en)
{
#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	return;
#endif

	if ((decon->dt.out_type != DECON_OUT_DSI) || (!decon->freq_hop.enabled))
		return;

	if (en) {
		regs->df_update = dpu_check_df_update(decon);

		if (regs->df_update == MAGIC_DF_UPDATED) {
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
			/* wakeup PLL if sleeping... */
			decon_reg_set_pll_wakeup(decon->id, true);
			decon_reg_set_pll_sleep(decon->id, false);
#endif
			dpu_set_pre_df_dsim(decon);
		}
	} else {
		if (regs->df_update == MAGIC_DF_UPDATED) {
			dpu_set_post_df_dsim(decon);
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
			decon_reg_set_pll_sleep(decon->id, true);
#endif
			decon_panel_ioc_update_ffc(decon);

			dpu_finish_df_update(decon);
		}
	}
}

#else
void dpu_set_freq_hop(struct decon_device *decon, struct decon_reg_data *regs, bool en)
{
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	struct stdphy_pms *pms;
	struct decon_freq_hop freq_hop;
	u32 target_m = decon->freq_hop.target_m;
	u32 target_k = decon->freq_hop.target_k;

	if ((decon->dt.out_type != DECON_OUT_DSI) || (!decon->freq_hop.enabled))
		return;

	pms = &decon->lcd_info->dphy_pms;
	if ((pms->m != target_m) || (pms->k != target_k)) {
		if (en) {
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
			/* wakeup PLL if sleeping... */
			decon_reg_set_pll_wakeup(decon->id, true);
			decon_reg_set_pll_sleep(decon->id, false);
#endif

			v4l2_subdev_call(decon->out_sd[0], core, ioctl,
					DSIM_IOC_SET_FREQ_HOP, &decon->freq_hop);

		} else {
			pms->m = decon->freq_hop.target_m;
			pms->k = decon->freq_hop.target_k;
			freq_hop.target_m = 0;
			decon_info("%s: pmsk[%d %d %d %d]\n", __func__,
					pms->p, pms->m, pms->s, pms->k);
			v4l2_subdev_call(decon->out_sd[0], core, ioctl,
					DSIM_IOC_SET_FREQ_HOP, &freq_hop);
#if defined(CONFIG_EXYNOS_PLL_SLEEP)
			decon_reg_set_pll_sleep(decon->id, true);
#endif
		}
	}
#endif
}
#endif
