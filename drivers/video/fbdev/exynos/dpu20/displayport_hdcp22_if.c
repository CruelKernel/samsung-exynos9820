/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC DisplayPort driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/pm_runtime.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/module.h>
#include <video/mipi_display.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-dv-timings.h>
#if defined(CONFIG_CPU_IDLE)
#include <soc/samsung/exynos-powermode.h>
#endif
#if defined(CONFIG_SND_SOC_SAMSUNG_DISPLAYPORT)
#include <sound/samsung/dp_ado.h>
#endif
#include <linux/smc.h>
#include <linux/exynos_iovmm.h>

#if defined(CONFIG_PHY_EXYNOS_USBDRD)
#include "../../../drivers/phy/samsung/phy-exynos-usbdrd.h"
#endif
#include "displayport.h"
#include "displayport_hdcp22_if.h"
#include "decon.h"

int displayport_hdcp22_irq_handler(void)
{
#if defined(CONFIG_EXYNOS_HDCP2)
	struct displayport_device *displayport = get_displayport_drvdata();
	uint8_t rxstatus = 0;
	int ret = 0;
	int active;

	active = pm_runtime_active(displayport->dev);
	if (!active) {
		displayport_info("displayport power(%d), state(%d)\n",
				active, displayport->state);
		spin_unlock(&displayport->slock);
		return IRQ_HANDLED;
	}

	hdcp_dplink_get_rxstatus(&rxstatus);

	if (rxstatus & DPCD_HDCP22_RXSTATUS_LINK_INTEGRITY_FAIL) {
		/* hdcp22 disable while re-authentication */
		ret = hdcp_dplink_set_integrity_fail();

		displayport_reg_set_hdcp22_system_enable(0);
		displayport_reg_set_hdcp22_mode(0);
		displayport_reg_set_hdcp22_encryption_enable(0);
		displayport_info("LINK_INTEGRITY_FAIL HDCP2 enc off\n");
		queue_delayed_work(displayport->hdcp2_wq,
				&displayport->hdcp22_work, msecs_to_jiffies(2000));
	}

	else if (rxstatus & DPCD_HDCP22_RXSTATUS_REAUTH_REQ) {
		/* hdcp22 disable while re-authentication */
		hdcp_dplink_cancel_auth();
		ret = hdcp_dplink_set_reauth();

		displayport_reg_set_hdcp22_system_enable(0);
		displayport_reg_set_hdcp22_mode(0);
		displayport_reg_set_hdcp22_encryption_enable(0);

		queue_delayed_work(displayport->hdcp2_wq,
			&displayport->hdcp22_work, msecs_to_jiffies(1000));
		displayport_info("REAUTH_REQ HDCP2 enc off\n");
	} else if (rxstatus & DPCD_HDCP22_RXSTATUS_PAIRING_AVAILABLE) {
		/* set pairing avaible flag */
		ret = hdcp_dplink_set_paring_available();
	} else if (rxstatus & DPCD_HDCP22_RXSTATUS_HPRIME_AVAILABLE) {
		/* set hprime avaible flag */
		ret = hdcp_dplink_set_hprime_available();
	} else if (rxstatus & DPCD_HDCP22_RXSTATUS_READY) {
		/* set ready avaible flag */
		/* todo update stream Management */
		ret = hdcp_dplink_set_rp_ready();
		return hdcp_dplink_auth_check(HDCP_RP_READY);
	} else {
		displayport_info("undefined RxStatus(0x%x). ignore\n", rxstatus);
		ret = -EINVAL;
	}

	return ret;
#else
	displayport_info("Not compiled EXYNOS_HDCP2 driver\n");
	return 0;
#endif
}

#if defined(CONFIG_DRM_SYSFS)
static ssize_t secdp_drm_show(struct class *class, struct class_attribute *attr, char *buf)
{
	struct displayport_device *displayport = get_displayport_drvdata();
	displayport_info("DRM state %d\n", displayport->drm_start_state);

	return sprintf(buf, "DRM state %d\n", displayport->drm_start_state);
}

static ssize_t secdp_drm_store(struct class *dev, struct class_attribute *attr, const char *buf, size_t size)
{
	struct displayport_device *displayport = get_displayport_drvdata();
	int val[3] = {0, };

	get_options(buf, 2, val);

	displayport->drm_start_state = val[1];

	displayport_err("drm %s!!\n", (displayport->drm_start_state ? "start" : "end"));
	queue_delayed_work(displayport->hdcp2_wq,
			&displayport->hdcp22_work, msecs_to_jiffies(0));

	return size;
}

static CLASS_ATTR_RW(secdp_drm);
#endif
void reset_dp_hdcp_module(void)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	displayport_info("DP Reset to rest HDCP module\n");
	queue_delayed_work(displayport->dp_wq,
			&displayport->hpd_unplug_work, 0);
}

int displayport_hdcp22_authenticate()
{
#ifndef CONFIG_HDCP2_FUNC_TEST_MODE
	struct displayport_device *displayport = get_displayport_drvdata();
	enum auth_signal hdcp22_signal;
#endif
#ifdef CONFIG_HDCP2_FUNC_TEST_MODE
	displayport_info("HDCP22 DRM Always On.\n");
	return hdcp_dplink_auth_check(HDCP_DRM_ON);
#else
	if (displayport->drm_start_state)
		hdcp22_signal = HDCP_DRM_ON;
	else
		hdcp22_signal = HDCP_DRM_OFF;

	displayport_info("HDCP22 DRM mode : (%x)\n", hdcp22_signal);
	if (displayport->hpd_state)
		return hdcp_dplink_auth_check(hdcp22_signal);

	return 1;
#endif
}

