// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Author: Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#include <linux/dma-buf.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/debugfs.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/ctype.h>
#include <video/mipi_display.h>
#include <linux/regulator/consumer.h>
#include "../../../../../kernel/irq/internals.h"

#if defined(CONFIG_EXYNOS_DPU20)
#include "../dpu20/decon.h"
#else
#include "../dpu_9810/decon.h"
#endif

#include "panel.h"
#include "panel_drv.h"
#include "dpui.h"
#include "mdnie.h"
#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
#include "spi.h"
#endif
#ifdef CONFIG_ACTIVE_CLOCK
#include "active_clock.h"
#endif
#ifdef CONFIG_SUPPORT_DDI_FLASH
#include "panel_poc.h"
#endif
#ifdef CONFIG_EXTEND_LIVE_CLOCK
#include "./aod/aod_drv.h"
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
#include "panel_spi.h"
#endif
#if defined(CONFIG_TDMB_NOTIFIER)
#include <linux/tdmb_notifier.h>
#endif
#ifdef CONFIG_DYNAMIC_FREQ
#include "dynamic_freq.h"
#endif

static char *panel_state_names[] = {
	"OFF",		/* POWER OFF */
	"ON",		/* POWER ON */
	"NORMAL",	/* SLEEP OUT */
	"LPM",		/* LPM */
};

/* panel workqueue */
static char *panel_work_names[] = {
	[PANEL_WORK_DISP_DET] = "disp-det",
	[PANEL_WORK_PCD] = "pcd",
	[PANEL_WORK_ERR_FG] = "err-fg",
	[PANEL_WORK_CONN_DET] = "conn-det",
#ifdef CONFIG_SUPPORT_DIM_FLASH
	[PANEL_WORK_DIM_FLASH] = "dim-flash",
#endif
	[PANEL_WORK_CHECK_CONDITION] = "panel-condition-check",
};

static void disp_det_handler(struct work_struct *data);
static void conn_det_handler(struct work_struct *data);
static void err_fg_handler(struct work_struct *data);
static void dim_flash_handler(struct work_struct *work);
static void panel_condition_handler(struct work_struct *work);

static panel_wq_handler panel_wq_handlers[] = {
	[PANEL_WORK_DISP_DET] = disp_det_handler,
	[PANEL_WORK_PCD] = NULL,
	[PANEL_WORK_ERR_FG] = err_fg_handler,
	[PANEL_WORK_CONN_DET] = conn_det_handler,
#ifdef CONFIG_SUPPORT_DIM_FLASH
	[PANEL_WORK_DIM_FLASH] = dim_flash_handler,
#endif
	[PANEL_WORK_CHECK_CONDITION] = panel_condition_handler,
};

/* panel gpio */
static char *panel_gpio_names[PANEL_GPIO_MAX] = {
	[PANEL_GPIO_RESET] = PANEL_GPIO_NAME_RESET,
	[PANEL_GPIO_DISP_DET] = PANEL_GPIO_NAME_DISP_DET,
	[PANEL_GPIO_PCD] = PANEL_GPIO_NAME_PCD,
	[PANEL_GPIO_ERR_FG] = PANEL_GPIO_NAME_ERR_FG,
	[PANEL_GPIO_CONN_DET] = PANEL_GPIO_NAME_CONN_DET,
};

/* panel regulator */
static char *panel_regulator_names[PANEL_REGULATOR_MAX] = {
	[PANEL_REGULATOR_DDI_3P0] = PANEL_REGULATOR_NAME_DDI_3P0,
	[PANEL_REGULATOR_DDI_1P8] = PANEL_REGULATOR_NAME_DDI_1P8,
	[PANEL_REGULATOR_DDR_1P6] = PANEL_REGULATOR_NAME_DDR_1P6,
	[PANEL_REGULATOR_SSD] = PANEL_REGULATOR_NAME_SSD,
};

static int boot_panel_id;
int panel_log_level = 6;
#ifdef CONFIG_SUPPORT_PANEL_SWAP
int panel_reprobe(struct panel_device *panel);
#endif

/**
 * get_lcd info - get lcd global information.
 * @arg: key string of lcd information
 *
 * if get lcd info successfully, return 0 or positive value.
 * if not, return -EINVAL.
 */
int get_lcd_info(char *arg)
{
	if (!arg) {
		panel_err("%s invalid arg\n", __func__);
		return -EINVAL;
	}

	if (!strncmp(arg, "connected", 9))
		return (boot_panel_id < 0) ? 0 : 1;
	else if (!strncmp(arg, "id", 2))
		return (boot_panel_id < 0) ? 0 : boot_panel_id;
	else if (!strncmp(arg, "window_color", 12))
		return (boot_panel_id < 0) ? 0 : ((boot_panel_id & 0x0F0000) >> 16);
	else
		return -EINVAL;
}
EXPORT_SYMBOL(get_lcd_info);

bool panel_gpio_valid(struct panel_gpio *gpio)
{
	if (!gpio->name || !gpio_is_valid(gpio->num)) {
		pr_err("%s invalid gpio(%d)\n", __func__, gpio->num);
		return false;
	}
	return true;
}

static int panel_gpio_state(struct panel_gpio *gpio)
{
	if (!panel_gpio_valid(gpio))
		return -EINVAL;
	if (gpio->active_low)
		return gpio_get_value(gpio->num) ?
			PANEL_STATE_OK : PANEL_STATE_NOK;
	else
		return gpio_get_value(gpio->num) ?
			PANEL_STATE_NOK : PANEL_STATE_OK;
}

static int panel_disp_det_state(struct panel_device *panel)
{
	int state;

	state = panel_gpio_state(&panel->gpio[PANEL_GPIO_DISP_DET]);
	if (state >= 0)
		pr_debug("%s disp_det:%s\n",
				__func__, state ? "EL-OFF" : "EL-ON");

	return state;
}

static int panel_conn_det_state(struct panel_device *panel)
{
	int state;

	state = panel_gpio_state(&panel->gpio[PANEL_GPIO_CONN_DET]);
	if (state >= 0)
		pr_debug("%s conn_det:%s\n",
				__func__, state ? "connected" : "disconnected");

	return state;
}

bool ub_con_disconnected(struct panel_device *panel)
{
	int state;

	state = panel_conn_det_state(panel);
	if (state < 0)
		return false;

	return !state;
}

void clear_pending_bit(int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (desc->irq_data.chip->irq_ack) {
		desc->irq_data.chip->irq_ack(&desc->irq_data);
		desc->istate &= ~IRQS_PENDING;
	}
};

static int panel_regulator_enable(struct panel_device *panel)
{
	struct panel_regulator *regulator = panel->regulator;
	int i, ret;

	for (i = 0; i < PANEL_REGULATOR_MAX; i++) {
		if (IS_ERR_OR_NULL(regulator[i].reg))
			continue;

		if (regulator[i].type != PANEL_REGULATOR_TYPE_PWR)
			continue;

		ret = regulator_enable(regulator[i].reg);
		if (ret != 0) {
			panel_err("PANEL:%s:faield to enable regulator(%d:%s), ret:%d\n",
					__func__, i, regulator[i].name, ret);
			return ret;
		}

		pr_info("enable regulator(%d:%s)\n", i, regulator[i].name);
	}

	return 0;
}

static int panel_regulator_disable(struct panel_device *panel)
{
	struct panel_regulator *regulator = panel->regulator;
	int i, ret;

	for (i = PANEL_REGULATOR_MAX - 1; i >= 0; i--) {
		if (IS_ERR_OR_NULL(regulator[i].reg))
			continue;

		if (regulator[i].type != PANEL_REGULATOR_TYPE_PWR)
			continue;

		ret = regulator_disable(regulator[i].reg);
		if (ret != 0) {
			panel_err("PANEL:%s:faield to disable regulator(%d:%s), ret:%d\n",
					__func__, i, regulator[i].name, ret);
			return ret;
		}
		pr_info("disable regulator(%d:%s)\n", i, regulator[i].name);
	}

	return 0;
}

static int panel_regulator_set_voltage(struct panel_device *panel, int state)
{
	struct panel_regulator *regulator = panel->regulator;
	int i, old_uv, new_uv;
	int ret = 0;

	for (i = 0; i < PANEL_REGULATOR_MAX; i++) {
		if (IS_ERR_OR_NULL(regulator[i].reg))
			continue;

		if (regulator[i].type != PANEL_REGULATOR_TYPE_PWR)
			continue;

		new_uv = (state == PANEL_STATE_ALPM) ?
			regulator[i].lpm_voltage : regulator[i].def_voltage;
		if (new_uv == 0)
			continue;

		old_uv = regulator_get_voltage(regulator[i].reg);
		if (old_uv < 0) {
			panel_err("PANEL:%s:faield to get regulator(%d:%s) voltage, ret:%d\n",
					__func__, i, regulator[i].name, old_uv);
			ret = -EINVAL;
			return ret;
		}

		if (new_uv == old_uv)
			continue;

		ret = regulator_set_voltage(regulator[i].reg, new_uv, new_uv);
		if (ret < 0) {
			panel_err("PANEL:%s:faield to set regulator(%d:%s) target voltage(%d), ret:%d\n",
					__func__, i, regulator[i].name, new_uv, ret);
			ret = -EINVAL;
			return ret;
		}

		panel_info("PANEL:INFO:%s: voltage:%duV\n", __func__, new_uv);
	}

	return ret;
}

#ifdef CONFIG_DISP_PMIC_SSD
static int panel_regulator_set_short_detection(struct panel_device *panel, int state)
{
	struct panel_regulator *regulator = panel->regulator;
	int i, ret, new_ua;
	bool en = true;

	for (i = 0; i < PANEL_REGULATOR_MAX; i++) {
		if (regulator[i].type != PANEL_REGULATOR_TYPE_SSD)
			continue;

		//for ha9: ssd off when exit lpm
		if (state == PANEL_STATE_OFF)
			en = false;

		new_ua = (state == PANEL_STATE_ALPM) ?
			regulator[i].lpm_current : regulator[i].def_current;
		if (new_ua == 0)
			continue;

		ret = regulator_set_short_detection(regulator[i].reg, en, new_ua);
		if (ret < 0) {
			panel_err("PANEL:%s:faield to set short detection regulator(%d:%s), ret:%d enable:%s\n",
					__func__, i, regulator[i].name, new_ua, (en ? "true" : "false"));
			regulator_put(regulator[i].reg);
			return ret;
		}

		panel_info("PANEL:INFO:%s:set regulator(%s) SSD:%duA, state:%d enable:%s\n",
				__func__, regulator[i].name, new_ua, state, (en ? "true" : "false"));
	}

	return 0;
}
#else
static inline int panel_regulator_set_short_detection(struct panel_device *panel, int state)
{
	return 0;
}
#endif

int __set_panel_power(struct panel_device *panel, int power)
{
	int ret = 0;
	struct panel_gpio *gpio = panel->gpio;

	if (panel->state.power == power) {
		panel_warn("PANEL:WARN:%s:same status.. skip..\n", __func__);
		return 0;
	}

	if (power == PANEL_POWER_ON) {
		ret = panel_regulator_set_short_detection(panel, PANEL_STATE_ON);
		if (ret < 0)
			panel_err("PANEL:ERR:%s:failed to set ssd current, ret:%d\n",
								__func__, ret);
		ret = panel_regulator_enable(panel);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:faield to panel_regulator_enable, ret:%d\n",
					__func__, ret);
			return ret;
		}
		usleep_range(10000, 10000 + 10);
		gpio_direction_output(gpio[PANEL_GPIO_RESET].num, 1);
		usleep_range(5000, 5000 + 10);
	} else {
		gpio_direction_output(gpio[PANEL_GPIO_RESET].num, 0);
		ret = panel_regulator_disable(panel);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:faield to panel_regulator_disable, ret:%d\n",
					__func__, ret);
			return ret;
		}
	}

	pr_info("%s, power(%s) gpio_reset(%s)\n", __func__,
			power == PANEL_POWER_ON ? "on" : "off",
			gpio_get_value(gpio[PANEL_GPIO_RESET].num) ? "high" : "low");
	panel->state.power = power;

	return 0;
}

static int __panel_seq_display_on(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DISPLAY_ON_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s:failed to seqtbl(PANEL_DISPLAY_ON_SEQ)\n",
				__func__);
		return ret;
	}

	return 0;
}

static int __panel_seq_display_off(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DISPLAY_OFF_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s:failed to seqtbl(PANEL_DISPLAY_OFF_SEQ)\n",
				__func__);
		return ret;
	}

	return 0;
}

static int __panel_seq_res_init(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_RES_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to seqtbl(PANEL_RES_INIT_SEQ)\n",
				__func__);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static int __panel_seq_dim_flash_res_init(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DIM_FLASH_RES_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to seqtbl(PANEL_DIM_FLASH_RES_INIT_SEQ)\n",
				__func__);
		return ret;
	}

	return 0;
}
#endif

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
static int panel_mipi_freq_update(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;
	struct adaptive_idx *adap_idx = &panel->adap_idx;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		return -ENODEV;
	}

	if (state->cur_state == PANEL_STATE_OFF ||
		state->cur_state == PANEL_STATE_ON ||
		!IS_PANEL_ACTIVE(panel))
		return 0;

	mutex_lock(&panel->op_lock);
	if (adap_idx->cur_freq_idx ==
		panel->panel_data.props.cur_ffc_idx) {
		pr_debug("[ADAP_FREQ] %s:cur_ffc_idx(%d) already set to %d\n",
				__func__, panel->panel_data.props.cur_ffc_idx,
				adap_idx->cur_freq_idx);
		goto exit_changed;
	}

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_FFC_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("[ADAP_FREQ] %s:failed to set PANEL_FFC_SEQ cur_ffc_idx(%d)\n",
				__func__, panel->panel_data.props.cur_ffc_idx);
		goto exit_changed;
	}
	panel_info("[ADAP_FREQ] %s:cur_ffc_idx(%d)\n",
			__func__, panel->panel_data.props.cur_ffc_idx);

exit_changed:
	mutex_unlock(&panel->op_lock);
	return ret;
}

static int panel_mipi_freq_changed(struct panel_device *panel, void *arg)
{
	return panel_mipi_freq_update(panel);
}
#endif

static int __panel_seq_init(struct panel_device *panel)
{
	int ret = 0;
	int retry = 20;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	if (panel_disp_det_state(panel) == PANEL_STATE_OK) {
		panel_warn("PANEL:WARN:%s:panel already initialized\n", __func__);
		return 0;
	}

#ifdef CONFIG_SUPPORT_PANEL_SWAP
	ret = panel_reprobe(panel);
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:failed to panel_reprobe\n", __func__);
		return ret;
	}
#endif

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
#ifdef CONFIG_SUPPORT_AOD_BL
	panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
		goto err_init_seq;
	}
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);


check_disp_det:
	if (panel_disp_det_state(panel) == PANEL_STATE_NOK) {
		usleep_range(1000, 1000 + 10);
		if (--retry >= 0)
			goto check_disp_det;
		panel_err("PANEL:ERR:%s:check disp det .. not ok\n", __func__);
		return -EAGAIN;
	}

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_init_panel(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to aod init_panel\n", __func__);
#endif

	clear_pending_bit(panel->gpio[PANEL_GPIO_DISP_DET].irq);
	enable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);

	panel_info("PANEL:INFO:%s:check disp det .. success\n", __func__);
	return 0;

err_init_seq:
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);
	return -EAGAIN;
}

static int __panel_seq_exit(struct panel_device *panel)
{
	int ret;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	disable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
#ifdef CONFIG_SUPPORT_AOD_BL
	panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_EXIT_SEQ);
	if (unlikely(ret < 0))
		panel_err("PANEL:ERR:%s, failed to write exit seqtbl\n", __func__);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

	return ret;
}

#ifdef CONFIG_SUPPORT_HMD
static int __panel_seq_hmd_on(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state;

	if (!panel) {
		panel_err("PANEL:ERR:%s:pane is null\n", __func__);
		return 0;
	}
	state = &panel->state;

	panel_info("PANEK:INFO:%s:hmd was on, setting hmd on seq\n", __func__);
	ret = panel_do_seqtbl_by_index(panel, PANEL_HMD_ON_SEQ);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to set hmd on seq\n",
			__func__);
	}

	return ret;
}
#endif

#ifdef CONFIG_SUPPORT_DOZE
static int __panel_seq_exit_alpm(struct panel_device *panel)
{
	int ret = 0;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	panel_info("PANEL:INFO:%s:was called\n", __func__);

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_exit_from_lpm(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to exit_lpm ops\n", __func__);
#endif

	if (panel->panel_data.ddi_props.ssd_off_lpm_to_normal) {
		ret = panel_regulator_set_short_detection(panel, PANEL_STATE_OFF);
		if (ret < 0)
			panel_err("PANEL:ERR:%s:failed to set ssd current, ret:%d\n", __func__, ret);
	}

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
	ret = panel_regulator_set_voltage(panel, PANEL_STATE_NORMAL);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set voltage\n",
				__func__);

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_EXIT_SEQ);
	if (ret)
		panel_err("PANEL:ERR:%s, failed to alpm-exit\n", __func__);
#ifdef CONFIG_SUPPORT_AOD_BL
	panel->panel_data.props.lpm_brightness = -1;
	panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
	if (panel->panel_data.props.panel_partial_disp != -1)
		panel->panel_data.props.panel_partial_disp = 0;
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);
	panel_update_brightness(panel);
	msleep(34);
	return ret;
}


/* delay to prevent current leackage when alpm */
/* according to ha6 opmanual, the dealy value is 126msec */
static void __delay_normal_alpm(struct panel_device *panel)
{
	u32 gap;
	u32 delay = 0;
	struct seqinfo *seqtbl;
	struct delayinfo *delaycmd;

	seqtbl = find_index_seqtbl(&panel->panel_data, PANEL_ALPM_DELAY_SEQ);
	if (unlikely(!seqtbl))
		goto exit_delay;

	delaycmd = (struct delayinfo *)seqtbl->cmdtbl[0];
	if (delaycmd->type != CMD_TYPE_DELAY) {
		panel_err("PANEL:ERR:%s:can't find value\n", __func__);
		goto exit_delay;
	}

	if (ktime_after(ktime_get(), panel->ktime_panel_on)) {
		gap = ktime_to_us(ktime_sub(ktime_get(), panel->ktime_panel_on));
		if (gap > delaycmd->usec)
			goto exit_delay;

		delay = delaycmd->usec - gap;
		usleep_range(delay, delay + 10);
	}
	panel_info("PANEL:INFO:%stotal elapsed time : %d\n", __func__,
		(int)ktime_to_us(ktime_sub(ktime_get(), panel->ktime_panel_on)));
exit_delay:
	return;
}

static int __panel_seq_set_alpm(struct panel_device *panel)
{
	int ret;
	struct panel_bl_device *panel_bl = &panel->panel_bl;

	panel_info("PANEL:INFO:%s:was called\n", __func__);
	ret = panel_regulator_set_short_detection(panel, PANEL_STATE_ALPM);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set ssd current, ret:%d\n",
				__func__, ret);
	__delay_normal_alpm(panel);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_ENTER_SEQ);
	if (ret)
		panel_err("PANEL:ERR:%s, failed to alpm-enter\n", __func__);
#ifdef CONFIG_SUPPORT_AOD_BL
	panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_AOD);
#endif

	ret = panel_regulator_set_voltage(panel, PANEL_STATE_ALPM);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set voltage\n",
				__func__);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_enter_to_lpm(panel);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to enter to lpm\n", __func__);
		return ret;
	}
#endif

	return 0;
}
#endif

#ifdef CONFIG_ACTIVE_CLOCK
static int __panel_seq_active_clock(struct panel_device *panel, int send_img)
{
	int ret = 0;
	struct act_clock_dev *act_dev = &panel->act_clk_dev;
	struct act_clk_info *act_info = &act_dev->act_info;
	struct act_blink_info *blink_info = &act_dev->blink_info;

	if (act_info->en) {
		panel_dbg("PANEL:DBG:%s:active clock was enabed\n", __func__);
		if (send_img) {
			panel_dbg("PANEL:DBG:%s:send active clock's image\n", __func__);

			if (act_info->update_img == IMG_UPDATE_NEED) {
				ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_IMG_SEQ);
				if (unlikely(ret < 0))
					panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);

				act_info->update_img = IMG_UPDATE_DONE;
			}
		}
		usleep_range(5, 5);

		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

	if (blink_info->en) {
		panel_dbg("PANEL:DBG:%s:active blink was enabed\n", __func__);

		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}
	return ret;
}
#endif

static int __panel_seq_dump(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DUMP_SEQ);
	if (unlikely(ret < 0))
		panel_err("PANEL:ERR:%s, failed to write dump seqtbl\n", __func__);

	return ret;
}

static int panel_debug_dump(struct panel_device *panel)
{
	int ret;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		goto dump_exit;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_info("PANEL:INFO:Current state:%d\n", panel->state.cur_state);
		goto dump_exit;
	}

	ret = __panel_seq_dump(panel);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to dump\n", __func__);
		return ret;
	}

	panel_info("PANEL:INFO:%s: disp_det_state:%s\n", __func__,
			panel_disp_det_state(panel) == PANEL_STATE_OK ? "OK" : "NOK");
dump_exit:
	return 0;
}

int panel_display_on(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		goto do_exit;
	}

	if (state->cur_state == PANEL_STATE_OFF ||
		state->cur_state == PANEL_STATE_ON) {
		panel_warn("PANEL:WARN:%s:invalid state\n", __func__);
		goto do_exit;
	}

	mdnie_enable(&panel->mdnie);
	ret = __panel_seq_display_on(panel);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to display on\n", __func__);
		return ret;
	}
	state->disp_on = PANEL_DISPLAY_ON;
	copr_enable(&panel->copr);

	return 0;

do_exit:
	return ret;
}

static int panel_display_off(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		goto do_exit;
	}

	if (state->cur_state == PANEL_STATE_OFF ||
		state->cur_state == PANEL_STATE_ON) {
		panel_warn("PANEL:WARN:%s:invalid state\n", __func__);
		goto do_exit;
	}

	ret = __panel_seq_display_off(panel);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n",
			__func__);
	}
	state->disp_on = PANEL_DISPLAY_OFF;

	return 0;
do_exit:
	return ret;
}

static struct common_panel_info *panel_detect(struct panel_device *panel)
{
	u8 id[3];
	u32 panel_id;
	int ret = 0;
	struct common_panel_info *info;
	struct panel_info *panel_data;
	bool detect = true;

	if (panel == NULL) {
		panel_err("%s, panel is null\n", __func__);
		return NULL;
	}
	panel_data = &panel->panel_data;

	memset(id, 0, sizeof(id));
	ret = read_panel_id(panel, id);
	if (unlikely(ret < 0)) {
		panel_err("%s, failed to read id(ret %d)\n", __func__, ret);
		detect = false;
	}

	panel_id = (id[0] << 16) | (id[1] << 8) | id[2];
	memcpy(panel_data->id, id, sizeof(id));

#ifdef CONFIG_SUPPORT_PANEL_SWAP
	if ((boot_panel_id >= 0) && (detect == true)) {
		boot_panel_id = (id[0] << 16) | (id[1] << 8) | id[2];
		panel_info("PANEL:INFO:%s:boot_panel_id : 0x%x\n",
				__func__, boot_panel_id);
	}
#endif

	info = find_panel(panel, panel_id);
	if (unlikely(!info)) {
		panel_err("%s, panel not found (id 0x%08X)\n",
				__func__, panel_id);
		return NULL;
	}

	return info;
}

static int panel_prepare(struct panel_device *panel, struct common_panel_info *info)
{
	int i;
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->op_lock);
	panel_data->maptbl = info->maptbl;
	panel_data->nr_maptbl = info->nr_maptbl;
	panel_data->seqtbl = info->seqtbl;
	panel_data->nr_seqtbl = info->nr_seqtbl;
	panel_data->rditbl = info->rditbl;
	panel_data->nr_rditbl = info->nr_rditbl;
	panel_data->restbl = info->restbl;
	panel_data->nr_restbl = info->nr_restbl;
	panel_data->dumpinfo = info->dumpinfo;
	panel_data->nr_dumpinfo = info->nr_dumpinfo;
	for (i = 0; i < MAX_PANEL_BL_SUBDEV; i++)
		panel_data->panel_dim_info[i] = info->panel_dim_info[i];
	for (i = 0; i < panel_data->nr_maptbl; i++)
		panel_data->maptbl[i].pdata = panel;
	for (i = 0; i < panel_data->nr_restbl; i++)
		panel_data->restbl[i].state = RES_UNINITIALIZED;
	memcpy(&panel_data->ddi_props, &info->ddi_props,
			sizeof(panel_data->ddi_props));
	mutex_unlock(&panel->op_lock);

	return 0;
}

static int panel_resource_init(struct panel_device *panel)
{
	__panel_seq_res_init(panel);

	return 0;
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static int panel_dim_flash_resource_init(struct panel_device *panel)
{
	return __panel_seq_dim_flash_res_init(panel);
}
#endif

static int panel_maptbl_init(struct panel_device *panel)
{
	int i;
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->op_lock);
	for (i = 0; i < panel_data->nr_maptbl; i++)
		maptbl_init(&panel_data->maptbl[i]);
	mutex_unlock(&panel->op_lock);

	return 0;
}

int panel_is_changed(struct panel_device *panel)
{
	struct panel_info *panel_data = &panel->panel_data;
	u8 date[7] = { 0, }, coordinate[4] = { 0, };
	int ret;

	ret = resource_copy_by_name(panel_data, date, "date");
	if (ret < 0)
		return ret;

	ret = resource_copy_by_name(panel_data, coordinate, "coordinate");
	if (ret < 0)
		return ret;

	pr_info("%s cell_id(old) : %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
			__func__, panel_data->date[0], panel_data->date[1],
			panel_data->date[2], panel_data->date[3], panel_data->date[4],
			panel_data->date[5], panel_data->date[6], panel_data->coordinate[0],
			panel_data->coordinate[1], panel_data->coordinate[2],
			panel_data->coordinate[3]);

	pr_info("%s cell_id(new) : %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
			__func__, date[0], date[1], date[2], date[3], date[4], date[5],
			date[6], coordinate[0], coordinate[1], coordinate[2],
			coordinate[3]);

	if (memcmp(panel_data->date, date, sizeof(panel_data->date)) ||
		memcmp(panel_data->coordinate, coordinate, sizeof(panel_data->coordinate))) {
		memcpy(panel_data->date, date, sizeof(panel_data->date));
		memcpy(panel_data->coordinate, coordinate, sizeof(panel_data->coordinate));
		pr_info("%s panel is changed\n", __func__);
		return 1;
	}

	return 0;
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
int panel_update_dim_type(struct panel_device *panel, u32 dim_type)
{
	struct dim_flash_result *result = &panel->dim_flash_result;
	int state = 0;
	int ret;

	if (dim_type == DIM_TYPE_DIM_FLASH) {
		ret = set_panel_poc(&panel->poc_dev, POC_OP_DIM_READ, NULL);
		if (unlikely(ret)) {
			pr_err("%s, failed to read gamma flash(ret %d)\n",
					__func__, ret);
			if (!state)
				state = GAMMA_FLASH_ERROR_READ_FAIL;
		}

		if (state != GAMMA_FLASH_ERROR_READ_FAIL &&
				panel->poc_dev.nr_partition > POC_DIM_PARTITION) {
			result->dim_chksum_ok =
				panel->poc_dev.partition[POC_DIM_PARTITION].chksum_ok;
			result->dim_chksum_by_calc =
				panel->poc_dev.partition[POC_DIM_PARTITION].chksum_by_calc;
			result->dim_chksum_by_read =
				panel->poc_dev.partition[POC_DIM_PARTITION].chksum_by_read;
		}

		ret = check_poc_partition_exists(&panel->poc_dev, POC_DIM_PARTITION);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s failed to check dim_flash exist\n",
					__func__);
			if (!state)
				state = GAMMA_FLASH_ERROR_READ_FAIL;
		}

		if (ret != PARTITION_EXISTS) {
			pr_err("%s dim partition not exist(%d)\n", __func__, ret);
			if (!state)
				state = GAMMA_FLASH_ERROR_NOT_EXIST;
		}

		if (result->dim_chksum_by_calc != result->dim_chksum_by_read) {
			pr_err("%s dim flash checksum mismatch calc:%04X read:%04X\n",
					__func__, result->dim_chksum_by_calc,
					result->dim_chksum_by_read);
			if (!state)
				state = GAMMA_FLASH_ERROR_CHECKSUM_MISMATCH;
		}

		ret = set_panel_poc(&panel->poc_dev, POC_OP_MTP_READ, NULL);
		if (unlikely(ret)) {
			pr_err("%s, failed to read mtp flash(ret %d)\n",
					__func__, ret);
			if (!state)
				state = GAMMA_FLASH_ERROR_READ_FAIL;
		}

		/* update dimming flash, mtp, hbm_gamma resources */
		ret = panel_dim_flash_resource_init(panel);
		if (unlikely(ret)) {
			pr_err("%s, failed to resource init\n", __func__);
			if (!state)
				state = GAMMA_FLASH_ERROR_READ_FAIL;
		}

		ret = resource_copy_by_name(&panel->panel_data,
				result->mtp_flash, "dim_flash_mtp_offset");
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to copy resource dim_flash_mtp_offset (ret %d)\n",
					__func__, ret);
			if (!state)
				state = GAMMA_FLASH_ERROR_READ_FAIL;
		}

		ret = resource_copy_by_name(&panel->panel_data,
				result->mtp_reg, "mtp");
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to copy resource mtp (ret %d)\n",
					__func__, ret);
			if (!state)
				state = GAMMA_FLASH_ERROR_READ_FAIL;
		}

		if (state != GAMMA_FLASH_ERROR_READ_FAIL &&
				panel->poc_dev.nr_partition > POC_MTP_PARTITION) {
			result->mtp_chksum_ok = panel->poc_dev.partition[POC_MTP_PARTITION].chksum_ok;
			result->mtp_chksum_by_calc = panel->poc_dev.partition[POC_MTP_PARTITION].chksum_by_calc;
			result->mtp_chksum_by_read = panel->poc_dev.partition[POC_MTP_PARTITION].chksum_by_read;
		}

		if (result->mtp_chksum_by_calc != result->mtp_chksum_by_read ||
			memcmp(result->mtp_reg, result->mtp_flash, sizeof(result->mtp_reg))) {

			pr_info("[MTP FROM PANEL]\n");
			print_data(result->mtp_reg, sizeof(result->mtp_reg));

			pr_info("[MTP FROM FLASH]\n");
			print_data(result->mtp_flash, sizeof(result->mtp_flash));

			if (!state)
				state = GAMMA_FLASH_ERROR_MTP_OFFSET;
		}
	}

	if (state < 0)
		return state;

	mutex_lock(&panel->op_lock);
	panel->panel_data.props.cur_dim_type = dim_type;
	mutex_unlock(&panel->op_lock);

	ret = panel_maptbl_init(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to resource init\n", __func__);
		return -ENODEV;
	}

	return state;
}
#endif

int panel_reprobe(struct panel_device *panel)
{
	struct common_panel_info *info;
	int ret;

	info = panel_detect(panel);
	if (unlikely(!info)) {
		panel_err("PANEL:ERR:%s:panel detection failed\n", __func__);
		return -ENODEV;
	}

	ret = panel_prepare(panel, info);
	if (unlikely(ret)) {
		panel_err("PANEL:ERR:%s, failed to panel_prepare\n", __func__);
		return ret;
	}

	ret = panel_resource_init(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to resource init\n", __func__);
		return ret;
	}

#ifdef CONFIG_SUPPORT_DDI_FLASH
	ret = panel_poc_probe(panel, info->poc_data);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe poc driver\n", __func__);
		return -ENODEV;
	}
#endif /* CONFIG_SUPPORT_DDI_FLASH */

#ifdef CONFIG_SUPPORT_DIM_FLASH
	if (panel->panel_data.props.cur_dim_type == DIM_TYPE_DIM_FLASH) {
		ret = panel_dim_flash_resource_init(panel);
		if (unlikely(ret)) {
			pr_err("%s, failed to dim flash resource init\n", __func__);
			return -ENODEV;
		}
	}
#endif /* CONFIG_SUPPORT_DIM_FLASH */

	ret = panel_maptbl_init(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to maptbl init\n", __func__);
		return -ENODEV;
	}

	ret = panel_bl_probe(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe backlight driver\n", __func__);
		return -ENODEV;
	}

	return 0;
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static void dim_flash_handler(struct work_struct *work)
{
	struct panel_work *w = container_of(to_delayed_work(work),
			struct panel_work, dwork);
	struct panel_device *panel =
		container_of(w, struct panel_device, work[PANEL_WORK_DIM_FLASH]);
	int ret;

	if (atomic_read(&w->running)) {
		pr_info("%s already running\n", __func__);
		return;
	}

	atomic_set(&w->running, 1);
	mutex_lock(&panel->panel_bl.lock);
	pr_info("%s +\n", __func__);
	ret = panel_update_dim_type(panel, DIM_TYPE_DIM_FLASH);
	if (ret < 0) {
		pr_err("%s, failed to update dim_flash %d\n",
				__func__, ret);
		w->ret = ret;
	} else {
		pr_info("%s, update dim_flash done %d\n",
				__func__, ret);
		w->ret = GAMMA_FLASH_SUCCESS;
	}
	pr_info("%s -\n", __func__);
	mutex_unlock(&panel->panel_bl.lock);
	panel_update_brightness(panel);
	atomic_set(&w->running, 0);
}
#endif



void clear_check_wq_var(struct panel_condition_check *pcc)
{
	pcc->check_state = NO_CHECK_STATE;
	pcc->is_panel_check = false;
	pcc->frame_cnt = 0;
}

bool show_copr_value(struct panel_device *panel, int frame_cnt)
{
	int ret = 0;
	bool retVal = false;
#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	struct copr_info *copr = &panel->copr;
	char write_buf[200] = {0, };
	int c = 0, i = 0, len = 0;

	if (copr_is_enabled(copr)) {
		ret = copr_get_value(copr);
		if (ret < 0) {
			panel_err("%s failed to get copr\n", __func__);
			return retVal;
		}
		len += snprintf(write_buf + len, sizeof(write_buf) - len, "cur:%d avg:%d ",
			copr->props.cur_copr, copr->props.avg_copr);
		if (copr->props.nr_roi > 0) {
			len += snprintf(write_buf + len, sizeof(write_buf) - len, "roi:");
			for (i = 0; i < copr->props.nr_roi; i++) {
				for (c = 0; c < 3; c++) {
					if (sizeof(write_buf) - len > 0) {
						len += snprintf(write_buf + len, sizeof(write_buf) - len, "%d%s",
							copr->props.copr_roi_r[i][c],
							((i == copr->props.nr_roi - 1) && c == 2) ? "\n" : " ");
					}
				}
			}
		} else {
			len += snprintf(write_buf + len, sizeof(write_buf) - len, "%s", "\n");
		}
		panel_info("%s: copr(frame_cnt=%d) -> %s", __func__, frame_cnt, write_buf);
		if (copr->props.cur_copr > 0) /* not black */
			retVal = true;
	} else {
		panel_info("%s: copr do not support\n", __func__);
	}
#else
	panel_info("%s: copr feature is disabled\n", __func__);
#endif
	return retVal;
}

static void panel_condition_handler(struct work_struct *work)
{
	int ret = 0;
	struct panel_work *w = container_of(to_delayed_work(work),
			struct panel_work, dwork);
	struct panel_device *panel =
		container_of(w, struct panel_device, work[PANEL_WORK_CHECK_CONDITION]);
	struct panel_condition_check *condition = &panel->condition_check;

	if (atomic_read(&w->running)) {
		pr_info("%s already running\n", __func__);
		return;
	}
	panel_wake_lock(panel);
	atomic_set(&w->running, 1);
	mutex_lock(&w->lock);
	panel_info("%s: %s\n", __func__, condition->str_state[condition->check_state]);

	switch (condition->check_state) {
	case PRINT_NORMAL_PANEL_INFO:
		// print rddpm
		ret = panel_do_seqtbl_by_index(panel, PANEL_CHECK_CONDITION_SEQ);
		if (unlikely(ret < 0))
			panel_err("%s failed to write panel check\n", __func__);
		if (show_copr_value(panel, condition->frame_cnt))
			clear_check_wq_var(condition);
		else
			condition->check_state = CHECK_NORMAL_PANEL_INFO;
		break;
	case PRINT_DOZE_PANEL_INFO:
		ret = panel_do_seqtbl_by_index(panel, PANEL_CHECK_CONDITION_SEQ);
		if (unlikely(ret < 0))
			panel_err("%s failed to write panel check\n", __func__);
		clear_check_wq_var(condition);
		break;
	case CHECK_NORMAL_PANEL_INFO:
		if (show_copr_value(panel, condition->frame_cnt))
			clear_check_wq_var(condition);
		break;
	default:
		pr_info("%s state %d\n", __func__, condition->check_state);
		clear_check_wq_var(condition);
		break;
	}
	mutex_unlock(&w->lock);
	atomic_set(&w->running, 0);
	panel_wake_unlock(panel);
}

int init_check_wq(struct panel_condition_check *condition)
{
	clear_check_wq_var(condition);
	strcpy(condition->str_state[NO_CHECK_STATE], STR_NO_CHECK);
	strcpy(condition->str_state[PRINT_NORMAL_PANEL_INFO], STR_NOMARL_ON);
	strcpy(condition->str_state[CHECK_NORMAL_PANEL_INFO], STR_NOMARL_100FRAME);
	strcpy(condition->str_state[PRINT_DOZE_PANEL_INFO], STR_AOD_ON);

	return 0;
}

void panel_check_ready(struct panel_device *panel)
{
	struct panel_condition_check *pcc = &panel->condition_check;
	struct panel_work *pw = &panel->work[PANEL_WORK_CHECK_CONDITION];

	mutex_lock(&pw->lock);
	pcc->is_panel_check = true;
	if (panel->state.cur_state == PANEL_STATE_NORMAL)
		pcc->check_state = PRINT_NORMAL_PANEL_INFO;
	if (panel->state.cur_state == PANEL_STATE_ALPM)
		pcc->check_state = PRINT_DOZE_PANEL_INFO;
	mutex_unlock(&pw->lock);
}

static void panel_check_start(struct panel_device *panel)
{
	struct panel_condition_check *pcc = &panel->condition_check;
	struct panel_work *pw = &panel->work[PANEL_WORK_CHECK_CONDITION];

	mutex_lock(&pw->lock);
	if (pcc->frame_cnt < 100) {
		pcc->frame_cnt++;
		switch (pcc->check_state) {
		case PRINT_NORMAL_PANEL_INFO:
		case PRINT_DOZE_PANEL_INFO:
			if (pcc->frame_cnt == 1)
				queue_delayed_work(pw->wq, &pw->dwork, msecs_to_jiffies(0));
			break;
		case CHECK_NORMAL_PANEL_INFO:
			if (pcc->frame_cnt % 10 == 0)
				queue_delayed_work(pw->wq, &pw->dwork, msecs_to_jiffies(0));
			break;
		case NO_CHECK_STATE:
			// do nothing
			break;
		default:
			panel_warn("%s state is invalid %d %d %d\n",
				__func__, pcc->is_panel_check, pcc->frame_cnt, pcc->check_state);
			clear_check_wq_var(pcc);
			break;
		}
	} else {
		if (pcc->check_state == CHECK_NORMAL_PANEL_INFO) {
			panel_warn("%s screen is black in first 100 frame %d %d %d\n",
				__func__, pcc->is_panel_check, pcc->frame_cnt, pcc->check_state);
		} else {
			panel_warn("%s state is invalid %d %d %d\n",
				__func__, pcc->is_panel_check, pcc->frame_cnt, pcc->check_state);
		}
		clear_check_wq_var(pcc);
	}
	mutex_unlock(&pw->lock);
}


int panel_probe(struct panel_device *panel)
{
	int i, ret = 0;
	struct decon_lcd *lcd_info;
	struct panel_info *panel_data;
	struct common_panel_info *info;

	panel_dbg("%s was callled\n", __func__);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	lcd_info = &panel->lcd_info;
	panel_data = &panel->panel_data;

	info = panel_detect(panel);
	if (unlikely(!info)) {
		panel_err("PANEL:ERR:%s:panel detection failed\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
	/*
	 * TODO : move to parse dt function
	 * 1. get panel_spi device node.
	 * 2. get spi_device of node
	 */
	panel->spi = of_find_panel_spi_by_node(NULL);
	if (!panel->spi)
		panel_warn("%s, panel spi device unsupported\n", __func__);
#endif

	mutex_lock(&panel->op_lock);
	panel_data->props.temperature = NORMAL_TEMPERATURE;
	panel_data->props.alpm_mode = ALPM_OFF;
	panel_data->props.cur_alpm_mode = ALPM_OFF;
	panel_data->props.lpm_opr = 250;		/* default LPM OPR 2.5 */
	panel_data->props.cur_lpm_opr = 250;	/* default LPM OPR 2.5 */
	panel_data->props.panel_partial_disp = 0;
	panel_data->props.dia_mode = 0;
	panel_data->props.irc_mode = 0;
	panel_data->props.panel_partial_disp = (info->ddi_props.support_partial_disp) ? 0 : -1;

	memset(panel_data->props.mcd_rs_range, -1,
			sizeof(panel_data->props.mcd_rs_range));

#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	panel_data->props.gct_on = GRAM_TEST_OFF;
	panel_data->props.gct_vddm = VDDM_ORIG;
	panel_data->props.gct_pattern = GCT_PATTERN_NONE;
#endif
#ifdef CONFIG_SUPPORT_DYNAMIC_HLPM
	panel_data->props.dynamic_hlpm = 0;
#endif
#ifdef CONFIG_SUPPORT_TDMB_TUNE
	panel_data->props.tdmb_on = false;
	panel_data->props.cur_tdmb_on = false;
#endif
#ifdef CONFIG_SUPPORT_DIM_FLASH
	panel_data->props.cur_dim_type = DIM_TYPE_AID_DIMMING;
#endif
	for (i = 0; i < MAX_CMD_LEVEL; i++)
		panel_data->props.key[i] = 0;
	mutex_unlock(&panel->op_lock);

	mutex_lock(&panel->panel_bl.lock);
	panel_data->props.adaptive_control = ACL_OPR_MAX - 1;
#ifdef CONFIG_SUPPORT_XTALK_MODE
	panel_data->props.xtalk_mode = XTALK_OFF;
#endif
	panel_data->props.poc_onoff = POC_ONOFF_ON;
	mutex_unlock(&panel->panel_bl.lock);

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	panel_data->props.cur_ffc_idx = 0;
#endif

#ifdef CONFIG_SUPPORT_DSU
	panel_data->props.mres_updated = false;
#endif
	panel_data->props.ub_con_cnt = 0;
	panel_data->props.conn_det_enable = 0;

	ret = panel_prepare(panel, info);
	if (unlikely(ret)) {
		pr_err("%s, failed to prepare common panel driver\n", __func__);
		return -ENODEV;
	}

	panel->lcd = lcd_device_register("panel", panel->dev, panel, NULL);
	if (IS_ERR(panel->lcd)) {
		panel_err("%s, faield to register lcd device\n", __func__);
		return PTR_ERR(panel->lcd);
	}

	ret = panel_resource_init(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to resource init\n", __func__);
		return -ENODEV;
	}

	resource_copy_by_name(panel_data, panel_data->date, "date");
	resource_copy_by_name(panel_data, panel_data->coordinate, "coordinate");

#ifdef CONFIG_SUPPORT_DDI_FLASH
	ret = panel_poc_probe(panel, info->poc_data);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe poc driver\n", __func__);
		return -ENODEV;
	}
#endif /* CONFIG_SUPPORT_DDI_FLASH */

	ret = panel_maptbl_init(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to resource init\n", __func__);
		return -ENODEV;
	}

	ret = panel_bl_probe(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe backlight driver\n", __func__);
		return -ENODEV;
	}

	ret = panel_sysfs_probe(panel);
	if (unlikely(ret)) {
		pr_err("%s, failed to init sysfs\n", __func__);
		return -ENODEV;
	}

	ret = mdnie_probe(panel, info->mdnie_tune);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe mdnie driver\n", __func__);
		return -ENODEV;
	}

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
	ret = copr_probe(panel, info->copr_data);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe copr driver\n", __func__);
		BUG();
		return -ENODEV;
	}
#endif

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = aod_drv_probe(panel, info->aod_tune);
	if (unlikely(ret)) {
		pr_err("%s, failed to probe aod driver\n", __func__);
		BUG();
		return -ENODEV;
	}
#endif

#ifdef CONFIG_SUPPORT_DISPLAY_PROFILER
	ret = profiler_probe(panel, info->profile_tune);
	if (unlikely(ret))
		panel_err("PANEL:ERR:%s:failed to probe profiler\n", __func__);
#endif

#ifdef CONFIG_SUPPORT_POC_SPI
	ret = panel_spi_drv_probe(panel, info->spi_data);

	if (unlikely(ret)) {
		pr_err("%s, failed to probe panel spi driver\n", __func__);
		return -ENODEV;
	}
#endif

#ifdef CONFIG_DYNAMIC_FREQ
	ret = dynamic_freq_probe(panel, info->df_freq_tbl);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to register dynamic freq module\n",
			__func__);
#endif
#ifdef CONFIG_SUPPORT_DIM_FLASH
	mutex_lock(&panel->panel_bl.lock);
	mutex_lock(&panel->op_lock);
	for (i = 0; i < MAX_PANEL_BL_SUBDEV; i++) {
		if (panel_data->panel_dim_info[i]->dim_flash_on) {
			panel_data->props.dim_flash_on = true;
			pr_info("%s dim_flash : on\n", __func__);
			break;
		}
	}
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->panel_bl.lock);

	if (panel_data->props.dim_flash_on)
		queue_delayed_work(panel->work[PANEL_WORK_DIM_FLASH].wq,
				&panel->work[PANEL_WORK_DIM_FLASH].dwork,
				msecs_to_jiffies(500));
#endif /* CONFIG_SUPPORT_DIM_FLASH */
	init_check_wq(&panel->condition_check);
	return 0;
}

static int panel_sleep_in(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;
	enum panel_active_state prev_state = state->cur_state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		goto do_exit;
	}

	switch (state->cur_state) {
	case PANEL_STATE_ON:
		panel_warn("PANEL:WARN:%s:panel already %s state\n",
				__func__, panel_state_names[state->cur_state]);
		goto do_exit;
	case PANEL_STATE_NORMAL:
	case PANEL_STATE_ALPM:
		copr_disable(&panel->copr);
		mdnie_disable(&panel->mdnie);
		ret = panel_display_off(panel);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s, failed to write display_off seqtbl\n",
				__func__);
		}
		ret = __panel_seq_exit(panel);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s, failed to write exit seqtbl\n",
				__func__);
		}
		break;
	default:
		panel_err("PANEL:ERR:%s:invalid state(%d)\n",
				__func__, state->cur_state);
		goto do_exit;
	}

	state->cur_state = PANEL_STATE_ON;
	panel_info("%s panel_state:%s -> %s\n", __func__,
			panel_state_names[prev_state], panel_state_names[state->cur_state]);

#ifdef CONFIG_SUPPORT_DISPLAY_PROFILER
	panel->profiler.flag_font = 0;
#endif

	return 0;

do_exit:
	return ret;
}

static int panel_power_on(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;
	enum panel_active_state prev_state = state->cur_state;

	if (panel->state.connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		return -ENODEV;
	}

	panel->state.connected = panel_conn_det_state(panel);
	if (panel->state.connected < 0) {
		pr_debug("PANEL:INFO:%s:conn unsupported\n", __func__);
	} else if (panel->state.connected == PANEL_STATE_NOK) {
		panel_warn("PANEL:WARN:%s:ub disconnected\n", __func__);
#if defined(CONFIG_SUPPORT_PANEL_SWAP)
		return -ENODEV;
#endif
	} else {
		pr_debug("PANEL:INFO:%s:ub connected\n", __func__);
	}

	if (state->cur_state == PANEL_STATE_OFF) {
		ret = __set_panel_power(panel, PANEL_POWER_ON);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to panel power on\n",
				__func__);
			goto do_exit;
		}
		state->cur_state = PANEL_STATE_ON;
	}
	panel_info("%s panel_state:%s -> %s\n", __func__,
			panel_state_names[prev_state], panel_state_names[state->cur_state]);

	return 0;

do_exit:
	return ret;
}

static int panel_power_off(struct panel_device *panel)
{
	int ret = -EINVAL;
	struct panel_state *state = &panel->state;
	enum panel_active_state prev_state = state->cur_state;
#ifdef CONFIG_ACTIVE_CLOCK
	struct act_clock_dev *act_clk;
#endif

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		goto do_exit;
	}

	switch (state->cur_state) {
	case PANEL_STATE_OFF:
		panel_warn("PANEL:WARN:%s:panel already %s state\n",
				__func__, panel_state_names[state->cur_state]);
		goto do_exit;
	case PANEL_STATE_ALPM:
	case PANEL_STATE_NORMAL:
		panel_sleep_in(panel);
	case PANEL_STATE_ON:
		ret = __set_panel_power(panel, PANEL_POWER_OFF);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to panel power off\n",
				__func__);
			goto do_exit;
		}
		break;
	default:
		panel_err("PANEL:ERR:%s:invalid state(%d)\n",
				__func__, state->cur_state);
		goto do_exit;
	}

	state->cur_state = PANEL_STATE_OFF;
#ifdef CONFIG_ACTIVE_CLOCK
    /*clock image for active clock on ddi side ram was remove when ddi was turned off */
	act_clk = &panel->act_clk_dev;
	act_clk->act_info.update_img = IMG_UPDATE_NEED;
#endif
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_power_off(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to aod power off\n", __func__);
#endif
	panel_info("%s panel_state:%s -> %s\n", __func__,
			panel_state_names[prev_state], panel_state_names[state->cur_state]);

	return 0;

do_exit:
	return ret;
}

static int panel_sleep_out(struct panel_device *panel)
{
	int ret = 0;
	static int retry = 3;
	struct panel_state *state = &panel->state;
	enum panel_active_state prev_state = state->cur_state;

	if (panel->state.connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		return -ENODEV;
	}

	panel->state.connected = panel_conn_det_state(panel);
	if (panel->state.connected < 0) {
		pr_debug("PANEL:INFO:%s:conn unsupported\n", __func__);
	} else if (panel->state.connected == PANEL_STATE_NOK) {
		panel_warn("PANEL:WARN:%s:ub disconnected\n", __func__);
#if defined(CONFIG_SUPPORT_PANEL_SWAP)
		return -ENODEV;
#endif
	} else {
		panel_info("PANEL:INFO:%s:ub connected\n", __func__);
	}

	switch (state->cur_state) {
	case PANEL_STATE_NORMAL:
		panel_warn("PANEL:WARN:%s:panel already %s state\n",
				__func__, panel_state_names[state->cur_state]);
		goto do_exit;
	case PANEL_STATE_ALPM:
#ifdef CONFIG_SUPPORT_DOZE
		ret = __panel_seq_exit_alpm(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to panel exit alpm\n",
				__func__);
			goto do_exit;
		}
#endif
		break;
	case PANEL_STATE_OFF:
		ret = panel_power_on(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to power on\n", __func__);
			goto do_exit;
		}
	case PANEL_STATE_ON:
		ret = __panel_seq_init(panel);
		if (ret) {
			if (--retry >= 0 && ret == -EAGAIN) {
				panel_power_off(panel);
				msleep(100);
				goto do_exit;
			}
			panel_err("PANEL:ERR:%s:failed to panel init seq\n",
					__func__);
			BUG();
		}
		retry = 3;
		break;
	default:
		panel_err("PANEL:ERR:%s:invalid state(%d)\n",
				__func__, state->cur_state);
		goto do_exit;
	}
	state->cur_state = PANEL_STATE_NORMAL;
	state->disp_on = PANEL_DISPLAY_OFF;
	panel->ktime_panel_on = ktime_get();

	mutex_lock(&panel->work[PANEL_WORK_CHECK_CONDITION].lock);
	clear_check_wq_var(&panel->condition_check);
	mutex_unlock(&panel->work[PANEL_WORK_CHECK_CONDITION].lock);
#ifdef CONFIG_SUPPORT_HMD
	if (state->hmd_on == PANEL_HMD_ON) {
		panel_info("PANEL:INFO:%s:hmd was on, setting hmd on seq\n", __func__);
		ret = __panel_seq_hmd_on(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to set hmd on seq\n",
				__func__);
		}

		ret = panel_bl_set_brightness(&panel->panel_bl,
				PANEL_BL_SUBDEV_TYPE_HMD, 1);
		if (ret)
			pr_err("%s : fail to set hmd brightness\n", __func__);
	}
#endif
	panel_info("%s panel_state:%s -> %s\n", __func__,
			panel_state_names[prev_state],
			panel_state_names[state->cur_state]);

	return 0;

do_exit:
	return ret;
}

#ifdef CONFIG_SUPPORT_DOZE
static int panel_doze(struct panel_device *panel, unsigned int cmd)
{
	int ret = 0;
	struct panel_state *state = &panel->state;
	enum panel_active_state prev_state = state->cur_state;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		goto do_exit;
	}

	switch (state->cur_state) {
	case PANEL_STATE_ALPM:
		panel_warn("PANEL:WANR:%s:panel already %s state\n",
				__func__, panel_state_names[state->cur_state]);
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
		panel_mipi_freq_update(panel);
#endif
		goto do_exit;
	case PANEL_POWER_ON:
	case PANEL_POWER_OFF:
		ret = panel_sleep_out(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s:failed to set normal\n", __func__);
			goto do_exit;
		}
	case PANEL_STATE_NORMAL:
#ifdef CONFIG_ACTIVE_CLOCK
		ret = __panel_seq_active_clock(panel, 1);
		if (ret) {
			panel_err("PANEL:ERR:%s, failed to write active seqtbl\n",
			__func__);
		}
#endif
		ret = __panel_seq_set_alpm(panel);
		if (ret) {
			panel_err("PANEL:ERR:%s, failed to write alpm\n",
				__func__);
		}
		state->cur_state = PANEL_STATE_ALPM;
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
		panel_mipi_freq_update(panel);
#endif
		panel_mdnie_update(panel);
		break;
	default:
		break;
	}
	mutex_lock(&panel->work[PANEL_WORK_CHECK_CONDITION].lock);
	clear_check_wq_var(&panel->condition_check);
	mutex_unlock(&panel->work[PANEL_WORK_CHECK_CONDITION].lock);

	panel_info("%s panel_state:%s -> %s\n", __func__,
			panel_state_names[prev_state], panel_state_names[state->cur_state]);

do_exit:
	return ret;
}
#endif //CONFIG_SUPPORT_DOZE

#ifdef CONFIG_SUPPORT_DSU
#ifdef CONFIG_EXYNOS_MULTIRESOLUTION
static int panel_set_mres(struct panel_device *panel, int *mres_idx)
{
	int ret = 0;
	int actual_mode;
	struct panel_state *state;
	struct decon_lcd *lcd_info;
	struct lcd_mres_info *dt_lcd_mres;
	struct dsc_slice *dsc_slice_info;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		ret = -EINVAL;
		goto do_exit;
	}

	state = &panel->state;
	lcd_info = &panel->lcd_info;
	dt_lcd_mres = &lcd_info->dt_lcd_mres;
	dsc_slice_info = &lcd_info->dt_dsc_slice;
	if (dt_lcd_mres->mres_en == 0) {
		panel_err("PANEL:ERR:%s:multi-resolution unsupported!!\n",
			__func__);
		ret = -EINVAL;
		goto do_exit;
	}

	actual_mode = *mres_idx;
	if (actual_mode >= dt_lcd_mres->mres_number) {
		panel_err("PANEL:ERR:%s:invalid mres idx:%d, number:%d\n",
			__func__, actual_mode, dt_lcd_mres->mres_number);
		actual_mode = 0;
	}

	lcd_info->mres_mode = actual_mode + DSU_MODE_1;
	lcd_info->xres = dt_lcd_mres->res_info[actual_mode].width;
	lcd_info->yres = dt_lcd_mres->res_info[actual_mode].height;
	lcd_info->dsc_enabled = dt_lcd_mres->res_info[actual_mode].dsc_en;
	lcd_info->dsc_slice_h = dt_lcd_mres->res_info[actual_mode].dsc_height;
	lcd_info->dsc_dec_sw = dsc_slice_info->dsc_dec_sw[actual_mode];
	lcd_info->dsc_enc_sw = dsc_slice_info->dsc_enc_sw[actual_mode];

	if (lcd_info->dsc_enabled)
		panel_info("PANEL:INFO:%s: dsu mode:%d, resol:%dx%d, dsc:%s, slice_h:%d\n",
			__func__, lcd_info->mres_mode, lcd_info->xres, lcd_info->yres,
			lcd_info->dsc_enabled ? "on" : "off", lcd_info->dsc_slice_h);
	else
		panel_info("PANEL:INFO:%s: dsu mode:%d, resol:%dx%d, dsc:%s, partial WxH:%dx%d\n",
			__func__, lcd_info->mres_mode, lcd_info->xres, lcd_info->yres,
			lcd_info->dsc_enabled ? "on" : "off", lcd_info->partial_width[actual_mode], lcd_info->partial_height[actual_mode]);

	panel->panel_data.props.mres_updated = true;
	ret = panel_do_seqtbl_by_index(panel, PANEL_DSU_SEQ);
	if (unlikely(ret < 0))
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);

	return 0;

do_exit:
	return ret;
}
#else
static int panel_set_dsu(struct panel_device *panel, struct dsu_info *dsu)
{
	int ret = 0;
	int xres, yres;
	int actual_mode;
	struct panel_state *state;
	struct decon_lcd *lcd_info;
	struct lcd_mres_info *dt_lcd_mres;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		ret = -EINVAL;
		goto do_exit;
	}
	if (unlikely(!dsu)) {
		panel_err("PANEL:ERR:%s:dsu is null\n", __func__);
		ret = -EINVAL;
		goto do_exit;
	}

	panel_info("PANEL:INFO:%s: Mode:%d, Res: %d, %d, %d, %d\n",
		__func__, dsu->mode, dsu->left, dsu->top,
		dsu->right, dsu->bottom);

	state = &panel->state;
	lcd_info = &panel->lcd_info;

	dt_lcd_mres = &lcd_info->dt_lcd_mres;

	if (dt_lcd_mres->mres_en == 0) {
		panel_err("PANEL:ERR:%s:this panel does not support dsu\n",
			__func__);
		ret = -EINVAL;
		goto do_exit;
	}

	if (dsu->right > dsu->left)
		xres = dsu->right - dsu->left;
	else
		xres = dsu->left - dsu->right;

	if (dsu->bottom > dsu->top)
		yres = dsu->bottom - dsu->top;
	else
		yres = dsu->top - dsu->bottom;

	panel_info("PANEL:INFO:%s: dsu mode:%d, %d:%d-%d:%d",
		__func__, dsu->mode, dsu->left, dsu->top,
		dsu->right, dsu->bottom);
	panel_info("PANEL:INFO:%s: dsu: xres: %d, yres: %d\n",
		__func__, xres, yres);

	actual_mode = dsu->mode - DSU_MODE_1;
	if (actual_mode >= dt_lcd_mres->mres_number) {
		panel_err("PANEL:ERR:%s:Wrong actual mode : %d , number : %d\n",
			__func__, actual_mode, dt_lcd_mres->mres_number);
		actual_mode = 0;
	}

	lcd_info->xres = xres;
	lcd_info->yres = yres;
	lcd_info->mres_mode = dsu->mode;

	lcd_info->dsc_enabled = dt_lcd_mres->res_info[actual_mode].dsc_en;
	lcd_info->dsc_slice_h = dt_lcd_mres->res_info[actual_mode].dsc_height;

	panel->panel_data.props.mres_updated = true;
	ret = panel_do_seqtbl_by_index(panel, PANEL_DSU_SEQ);
	if (unlikely(ret < 0))
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);

	return 0;

do_exit:
	return ret;
}
#endif /* CONFIG_EXYNOS_MULTIRESOLUTION */
#endif /* CONFIG_SUPPORT_DSU */

static int panel_ioctl_dsim_probe(struct v4l2_subdev *sd, void *arg)
{
	int *param = (int *)arg;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	panel_info("PANEL:INFO:%s:PANEL_IOC_DSIM_PROBE\n", __func__);
	if (param == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}
	panel->dsi_id = *param;
	panel_info("PANEL:INFO:%s:panel id : %d, dsim id : %d\n",
		__func__, panel->id, panel->dsi_id);
	v4l2_set_subdev_hostdata(sd, &panel->lcd_info);

	return 0;
}

static int panel_ioctl_dsim_ops(struct v4l2_subdev *sd)
{
	struct mipi_drv_ops *mipi_ops;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);

	panel_info("PANEL:INFO:%s:PANEL_IOC_MIPI_OPS\n", __func__);
	mipi_ops = (struct mipi_drv_ops *)v4l2_get_subdev_hostdata(sd);
	if (mipi_ops == NULL) {
		panel_err("PANEL:ERR:%s:mipi_ops is null\n", __func__);
		return -EINVAL;
	}
	panel->mipi_drv.read = mipi_ops->read;
	panel->mipi_drv.write = mipi_ops->write;
	panel->mipi_drv.get_state = mipi_ops->get_state;
	panel->mipi_drv.parse_dt = mipi_ops->parse_dt;


	return 0;
}

static int panel_ioctl_display_on(struct panel_device *panel, void *arg)
{
	int ret = 0;
	int *disp_on = (int *)arg;

	if (disp_on == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}
	if (*disp_on == 0)
		ret = panel_display_off(panel);
	else
		ret = panel_display_on(panel);

	return ret;
}

static int panel_ioctl_set_power(struct panel_device *panel, void *arg)
{
	int ret = 0;
	int *disp_on = (int *)arg;

	if (disp_on == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}
	if (*disp_on == 0)
		ret = panel_power_off(panel);
	else
		ret = panel_power_on(panel);

	return ret;
}

static int panel_set_error_cb(struct v4l2_subdev *sd)
{
	struct panel_device *panel = container_of(sd, struct panel_device, sd);
	struct disp_error_cb_info *error_cb_info;

	error_cb_info = (struct disp_error_cb_info *)v4l2_get_subdev_hostdata(sd);
	if (!error_cb_info) {
		panel_err("PANEL:ERR:%s:error error_cb info is null\n", __func__);
		return -EINVAL;
	}
	panel->error_cb_info.error_cb = error_cb_info->error_cb;
	panel->error_cb_info.data = error_cb_info->data;

	return 0;
}

static int panel_check_cb(struct panel_device *panel)
{
	int status = DISP_CHECK_STATUS_OK;

	if (panel_conn_det_state(panel) == PANEL_STATE_NOK)
		status |= DISP_CHECK_STATUS_NODEV;
	if (panel_disp_det_state(panel) == PANEL_STATE_NOK)
		status |= DISP_CHECK_STATUS_ELOFF;

	return status;
}

static int panel_error_cb(struct panel_device *panel)
{
	struct disp_error_cb_info *error_cb_info = &panel->error_cb_info;
	struct disp_check_cb_info panel_check_info = {
		.check_cb = (disp_check_cb *)panel_check_cb,
		.data = panel,
	};
	int ret = 0;

	if (error_cb_info->error_cb) {
		ret = error_cb_info->error_cb(error_cb_info->data,
				&panel_check_info);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to recovery panel\n", __func__);
	}

	return ret;
}

#ifdef CONFIG_SUPPORT_INDISPLAY
static int panel_set_finger_layer(struct panel_device *panel, void *arg)
{
	int ret = 0;
	int max_br, cur_br;
	int *cmd = (int *)arg;
	struct panel_bl_device *panel_bl;
	struct backlight_device *bd;

	panel_info("+ %s\n", __func__);

	panel_bl = &panel->panel_bl;
	if (panel_bl == NULL) {
		panel_err("PANEL:ERR:%s:bl is null\n", __func__);
		return -EINVAL;
	}
	bd = panel_bl->bd;
	if (bd == NULL) {
		panel_err("PANEL:ERR:%s:bd is null\n", __func__);
		return -EINVAL;
	}
	if (cmd == NULL) {
		panel_err("PANEL:ERR:%s:invalid arg\n", __func__);
		return -EINVAL;
	}

	max_br = bd->props.max_brightness;
	cur_br = bd->props.brightness;

	panel_info("%s:max : %d, cur : %d\n", __func__, max_br, cur_br);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);

	if (*cmd == 0) {
		panel_info("PANEL:INFO:%s:disable finger layer\n", __func__);
		panel_bl->finger_layer = false;
		panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_DISP].brightness = panel_bl->saved_br;
	} else {
		panel_info("PANEL:INFO:%s:enable finger layer\n", __func__);
		panel_bl->finger_layer = true;
		panel_bl->saved_br = cur_br;
		panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_DISP].brightness = max_br;
	}

	ret = panel_bl_set_brightness(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP, 1);
	if (ret)
		pr_err("%s : fail to set brightness\n", __func__);

	panel_info("- %s\n", __func__);

	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

	return ret;
}
#endif
static long panel_core_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct panel_device *panel = container_of(sd, struct panel_device, sd);
#ifdef CONFIG_SUPPORT_DSU
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	static int mres_updated_frame_cnt;
#endif
#endif

	mutex_lock(&panel->io_lock);

	switch (cmd) {
	case PANEL_IOC_DSIM_PROBE:
		ret = panel_ioctl_dsim_probe(sd, arg);
		break;
	case PANEL_IOC_DSIM_PUT_MIPI_OPS:
		ret = panel_ioctl_dsim_ops(sd);
		break;

	case PANEL_IOC_REG_RESET_CB:
		panel_info("PANEL:INFO:%s:PANEL_IOC_REG_PANEL_RESET\n", __func__);
		ret = panel_set_error_cb(sd);
		break;

	case PANEL_IOC_GET_PANEL_STATE:
		panel_info("PANEL:INFO:%s:PANEL_IOC_GET_PANEL_STATE\n", __func__);
		panel->state.connected = panel_conn_det_state(panel);
		v4l2_set_subdev_hostdata(sd, &panel->state);
		break;

	case PANEL_IOC_PANEL_PROBE:
		panel_info("PANEL:INFO:%s:PANEL_IOC_PANEL_PROBE\n", __func__);
		ret = panel_probe(panel);
		break;

	case PANEL_IOC_SLEEP_IN:
		panel_info("PANEL:INFO:%s:PANEL_IOC_SLEEP_IN\n", __func__);
		ret = panel_sleep_in(panel);
		break;

	case PANEL_IOC_SLEEP_OUT:
		panel_info("PANEL:INFO:%s:PANEL_IOC_SLEEP_OUT\n", __func__);
		ret = panel_sleep_out(panel);
		break;

	case PANEL_IOC_SET_POWER:
		panel_info("PANEL:INFO:%s:PANEL_IOC_SET_POWER\n", __func__);
		ret = panel_ioctl_set_power(panel, arg);
		break;

	case PANEL_IOC_PANEL_DUMP:
		panel_info("PANEL:INFO:%s:PANEL_IOC_PANEL_DUMP\n", __func__);
		ret = panel_debug_dump(panel);
		break;
#ifdef CONFIG_SUPPORT_DOZE
	case PANEL_IOC_DOZE:
	case PANEL_IOC_DOZE_SUSPEND:
		panel_info("PANEL:INFO:%s:PANEL_IOC_%s\n", __func__,
			cmd == PANEL_IOC_DOZE ? "DOZE" : "DOZE_SUSPEND");
		ret = panel_doze(panel, cmd);
		break;
#endif
#ifdef CONFIG_SUPPORT_DSU
	case PANEL_IOC_SET_DSU:
		panel_info("PANEL:INFO:%s:PANEL_IOC_SET_DSU\n", __func__);
#ifdef CONFIG_EXYNOS_MULTIRESOLUTION
		ret = panel_set_mres(panel, arg);
#else
		ret = panel_set_dsu(panel, arg);
#endif
		break;
#endif
	case PANEL_IOC_DISP_ON:
		panel_info("PANEL:INFO:%s:PANEL_IOC_DISP_ON\n", __func__);
		ret = panel_ioctl_display_on(panel, arg);
		break;

	case PANEL_IOC_EVT_FRAME_DONE:
		if (panel->state.cur_state != PANEL_STATE_NORMAL &&
				panel->state.cur_state != PANEL_STATE_ALPM) {
			panel_warn("PANEL:WARN:%s:FRAME_DONE (panel_state:%s, disp_on:%s)\n",
					__func__, panel_state_names[panel->state.cur_state],
					panel->state.disp_on ? "on" : "off");
			break;
		}

		if (panel->state.disp_on == PANEL_DISPLAY_OFF) {
			panel_info("PANEL:INFO:%s:FRAME_DONE (panel_state:%s, display on)\n",
					__func__, panel_state_names[panel->state.cur_state]);
			ret = panel_display_on(panel);
			panel_check_ready(panel);
		}
		copr_update_start(&panel->copr, 3);
#ifdef CONFIG_SUPPORT_DSU
		if (panel->panel_data.props.mres_updated &&
				(++mres_updated_frame_cnt > 1)) {
			panel->panel_data.props.mres_updated = false;
			mres_updated_frame_cnt = 0;
		}
#endif
		if (panel->condition_check.is_panel_check)
			panel_check_start(panel);
		break;
	case PANEL_IOC_EVT_VSYNC:
		//panel_dbg("PANEL:INFO:%s:PANEL_IOC_EVT_VSYNC\n", __func__);
		break;
#ifdef CONFIG_SUPPORT_INDISPLAY
	case PANEL_IOC_SET_FINGER_SET:
		ret = panel_set_finger_layer(panel, arg);
		break;
#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	case PANEL_IOC_MIPI_FREQ_CHANGED:
		ret = panel_mipi_freq_changed(panel, arg);
		break;
#endif

#ifdef CONFIG_DYNAMIC_FREQ
	case PANEL_IOC_GET_DF_STATUS:
		v4l2_set_subdev_hostdata(sd, &panel->df_status);
		break;

	case PANEL_IOC_DYN_FREQ_FFC:
		ret = set_dynamic_freq_ffc(panel);
		break;
#endif
	default:
		panel_err("PANEL:ERR:%s:undefined command\n", __func__);
		ret = -EINVAL;
		break;
	}

	if (ret < 0) {
		panel_err("PANEL:ERR:%s:failed to ioctl panel cmd : %d\n",
			__func__,  _IOC_NR(cmd));
	}
	mutex_unlock(&panel->io_lock);

	return (long)ret;
}

static const struct v4l2_subdev_core_ops panel_v4l2_sd_core_ops = {
	.ioctl = panel_core_ioctl,
};

static const struct v4l2_subdev_ops panel_subdev_ops = {
	.core = &panel_v4l2_sd_core_ops,
};

static void panel_init_v4l2_subdev(struct panel_device *panel)
{
	struct v4l2_subdev *sd = &panel->sd;

	v4l2_subdev_init(sd, &panel_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = 0;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "panel-sd", panel->id);

	v4l2_set_subdevdata(sd, panel);
}

static int panel_drv_set_gpios(struct panel_device *panel)
{
	int rst_val = -1, det_val = -1;
	struct panel_gpio *gpio = panel->gpio;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!gpio_is_valid(gpio[PANEL_GPIO_RESET].num)) {
		panel_err("PANEL:ERR:%s:gpio(%s) not exist\n",
				__func__, panel_gpio_names[PANEL_GPIO_RESET]);
		return -EINVAL;
	}
	rst_val = gpio_get_value(gpio[PANEL_GPIO_RESET].num);

	if (!gpio_is_valid(gpio[PANEL_GPIO_DISP_DET].num)) {
		panel_err("PANEL:ERR:%s:gpio(%s) not exist\n",
				__func__, panel_gpio_names[PANEL_GPIO_RESET]);
		return -EINVAL;
	}
	det_val = gpio_get_value(gpio[PANEL_GPIO_DISP_DET].num);

	/*
	 * panel state is decided by rst, conn_det and disp_det pin
	 *
	 * @rst_val
	 *  0 : need to init panel in kernel
	 *  1 : already initialized in bootloader
	 *
	 * @conn_det
	 *  < 0 : unsupported
	 *  = 0 : ub connector is disconnected
	 *  = 1 : ub connector is connected
	 *
	 * @det_val
	 *  0 : panel is "sleep in" state
	 *  1 : panel is "sleep out" state
	 */
	panel->state.init_at = (rst_val == 1) ?
		PANEL_INIT_BOOT : PANEL_INIT_KERNEL;

	/* connected : conn_det is connected or not */
	panel->state.connected = panel_conn_det_state(panel);

	/* connect_panel : decide to use or ignore panel */
	panel->state.connect_panel =
		(panel->state.init_at == PANEL_INIT_KERNEL || det_val == 1) ?
		PANEL_CONNECT : PANEL_DISCONNECT;

	if (panel->state.init_at == PANEL_INIT_KERNEL || det_val == 0) {
		gpio_direction_output(gpio[PANEL_GPIO_RESET].num, 0);
		panel->state.cur_state = PANEL_STATE_OFF;
		panel->state.power = PANEL_POWER_OFF;
		panel->state.disp_on = PANEL_DISPLAY_OFF;
	} else {
		gpio_direction_output(gpio[PANEL_GPIO_RESET].num, 1);
		panel->state.cur_state = PANEL_STATE_NORMAL;
		panel->state.power = PANEL_POWER_ON;
		panel->state.disp_on = PANEL_DISPLAY_ON;
	}

	panel_info("PANEL:INFO:%s: rst:%d, disp_det:%d (init_at:%s, ub_con:%d(%s) panel:%d(%s))\n",
		__func__, rst_val, det_val,
		(panel->state.init_at ? "BL" : "KERNEL"),
		panel->state.connected,
		(panel->state.connected < 0 ? "UNSUPPORTED" :
		(panel->state.connected == true ? "CONNECTED" : "DISCONNECTED")),
		panel->state.connect_panel,
		(panel->state.connect_panel == PANEL_CONNECT ? "USE" : "NO USE"));

	return 0;
}

static int panel_drv_set_regulators(struct panel_device *panel)
{
	int ret;

	if (panel->state.init_at == PANEL_INIT_BOOT) {
		ret = panel_regulator_set_short_detection(panel, PANEL_STATE_NORMAL);
		if (ret < 0)
			panel_err("PANEL:ERR:%s:failed to set ssd current, ret:%d\n",
					__func__, ret);
		ret = panel_regulator_enable(panel);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:faield to panel_regulator_enable, ret:%d\n",
					__func__, ret);
			return ret;
		}

		if (panel->state.connect_panel == PANEL_DISCONNECT) {
			ret = panel_regulator_disable(panel);
			if (ret < 0) {
				panel_err("PANEL:ERR:%s:faield to panel_regulator_disable, ret:%d\n",
						__func__, ret);
				return ret;
			}
		}
	}

	return 0;
}

static int of_get_panel_gpio(struct device_node *np, struct panel_gpio *gpio)
{
	struct device_node *pend_np;
	enum of_gpio_flags flags;
	int ret;

	if (of_gpio_count(np) < 1)
		return -ENODEV;

	if (of_property_read_string(np, "name",
			(const char **)&gpio->name)) {
		pr_err("%s %s:property('name') not found\n",
				__func__, np->name);
		return -EINVAL;
	}

	gpio->num = of_get_gpio_flags(np, 0, &flags);
	if (!gpio_is_valid(gpio->num)) {
		pr_err("%s %s:invalid gpio %s:%d\n",
				__func__, np->name, gpio->name, gpio->num);
		return -ENODEV;
	}
	gpio->active_low = flags & OF_GPIO_ACTIVE_LOW;

	if (of_property_read_u32(np, "dir", &gpio->dir))
		pr_warn("%s %s:property('dir') not found\n",
				__func__, np->name);

	if ((gpio->dir & GPIOF_DIR_IN) == GPIOF_DIR_OUT) {
		ret = gpio_request(gpio->num, gpio->name);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to request gpio(%s:%d)\n",
					__func__, gpio->num, gpio->name);
			return ret;
		}
	} else {
		ret = gpio_request_one(gpio->num, GPIOF_IN, gpio->name);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to request gpio(%s:%d)\n",
					__func__, gpio->num, gpio->name);
			return ret;
		}
	}

	if (of_property_read_u32(np, "irq-type", &gpio->irq_type))
		pr_warn("%s %s:property('irq-type') not found\n",
				__func__, np->name);

	if (gpio->irq_type > 0) {
		gpio->irq = gpio_to_irq(gpio->num);

		pend_np = of_get_child_by_name(np, "irq-pend");
		if (pend_np) {
			gpio->irq_pend_reg = of_iomap(pend_np, 0);
			if (gpio->irq_pend_reg == NULL) {
				pr_err("%s %s:%s:property('reg') not found\n",
						__func__, np->name, pend_np->name);
			}

			if (of_property_read_u32(pend_np, "bit",
						&gpio->irq_pend_bit)) {
				pr_err("%s %s:%s:property('bit') not found\n",
						__func__, np->name, pend_np->name);
				gpio->irq_pend_bit = -EINVAL;
			}
			of_node_put(pend_np);
		}
	}

	return 0;
}

static int panel_parse_gpio(struct panel_device *panel)
{
	struct device *dev = panel->dev;
	struct device_node *gpios_np, *np;
	struct panel_gpio *gpio = panel->gpio;
	int i;

	gpios_np = of_get_child_by_name(dev->of_node, "gpios");
	if (!gpios_np) {
		pr_err("%s 'gpios' node not found\n", __func__);
		return -EINVAL;
	}

	for_each_child_of_node(gpios_np, np) {
		for (i = 0; i < PANEL_GPIO_MAX; i++)
			if (!of_node_cmp(np->name,
						panel_gpio_names[i]))
				break;

		if (i == PANEL_GPIO_MAX) {
			pr_warn("%s %s not found in panel_gpio list\n",
					__func__, np->name);
			continue;
		}

		if (of_get_panel_gpio(np, &gpio[i]))
			pr_err("%s failed to get gpio %s\n",
					__func__, np->name);

		pr_info("gpio[%d] name:%s active:%s dir:%d irq_type:%d\n",
				i, gpio[i].name, gpio[i].active_low ? "low" : "high",
				gpio[i].dir, gpio[i].irq_type);
	}
	of_node_put(gpios_np);

	return 0;
}

static int of_get_panel_regulator(struct device_node *np,
		struct panel_regulator *regulator)
{
	struct device_node *reg_np;

	reg_np = of_parse_phandle(np, "regulator", 0);
	if (!reg_np) {
		pr_err("%s %s:'regulator' node not found\n",
				__func__, np->name);
		return -EINVAL;
	}

	if (of_property_read_string(reg_np, "regulator-name",
				(const char **)&regulator->name)) {
		pr_err("%s %s:%s:property('regulator-name') not found\n",
				__func__, np->name, reg_np->name);
		of_node_put(reg_np);
		return -EINVAL;
	}
	of_node_put(reg_np);

	regulator->reg = regulator_get(NULL, regulator->name);
	if (IS_ERR(regulator->reg)) {
		pr_err("%s failed to get regulator %s\n",
				__func__, regulator->name);
		return -EINVAL;
	}

	of_property_read_u32(np, "def-voltage", &regulator->def_voltage);
	of_property_read_u32(np, "lpm-voltage", &regulator->lpm_voltage);
	of_property_read_u32(np, "def-current", &regulator->def_current);
	of_property_read_u32(np, "lpm-current", &regulator->lpm_current);

	return 0;
}

static int panel_parse_regulator(struct panel_device *panel)
{
	int i;
	struct device *dev = panel->dev;
	struct panel_regulator *regulator = panel->regulator;
	struct device_node *regulators_np, *np;

	regulators_np = of_get_child_by_name(dev->of_node, "regulators");
	if (!regulators_np) {
		pr_err("%s 'regulators' node not found\n", __func__);
		return -EINVAL;
	}

	for_each_child_of_node(regulators_np, np) {
		for (i = 0; i < PANEL_REGULATOR_MAX; i++)
			if (!of_node_cmp(np->name,
					panel_regulator_names[i]))
				break;

		if (i == PANEL_REGULATOR_MAX) {
			pr_warn("%s %s not found in panel_regulator list\n",
					__func__, np->name);
			continue;
		}

		if (i == PANEL_REGULATOR_SSD)
			regulator[i].type = PANEL_REGULATOR_TYPE_SSD;
		else
			regulator[i].type = PANEL_REGULATOR_TYPE_PWR;

		if (of_get_panel_regulator(np, &regulator[i])) {
			pr_err("%s failed to get regulator %s\n",
					__func__, np->name);
			of_node_put(regulators_np);
			return -EINVAL;
		}
		pr_info("regulator[%d] name:%s type:%d\n",
				i, regulator[i].name, regulator[i].type);
	}
	of_node_put(regulators_np);

	panel_dbg("PANEL:INFO:%s done\n", __func__);

	return 0;
}

static irqreturn_t panel_work_isr(int irq, void *dev_id)
{
	struct panel_work *w = (struct panel_work *)dev_id;

	queue_delayed_work(w->wq, &w->dwork, msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

int panel_register_isr(struct panel_device *panel)
{
	int i, iw, ret = 0;
	struct panel_gpio *gpio = panel->gpio;
	char *name = NULL;

	if (panel->state.connect_panel == PANEL_DISCONNECT)
		return -ENODEV;
	for (i = 0; i < PANEL_GPIO_MAX; i++) {
		if (!panel_gpio_valid(gpio) || gpio[i].irq_type <= 0)
			continue;

		for (iw = 0; iw < PANEL_WORK_MAX; iw++) {
			if (!strncmp(panel_gpio_names[i],
					panel_work_names[iw], 32))
				break;
		}

		if (iw == PANEL_WORK_MAX)
			continue;
		name = kzalloc(sizeof(char) * 64, GFP_KERNEL);
		if (!name) {
			panel_err("PANEL:ERR:%s:failed to alloc name buffer(%d)\n",
				__func__, iw);
			ret = -ENOMEM;
			break;

		}
		snprintf(name, 64, "panel%d:%s",
				panel->id, panel_work_names[iw]);
		ret = devm_request_irq(panel->dev, gpio[i].irq, panel_work_isr,
				gpio[i].irq_type, name, &panel->work[iw]);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to register irq(%s:%d)\n",
					__func__, name, gpio[i].irq);
			break;
		}
	}
	return ret;
}

#ifdef CONFIG_EXYNOS_COMMON_PANEL
int panel_wake_lock(struct panel_device *panel)
{
	int ret = 0;

	ret = decon_wake_lock_global(0, WAKE_TIMEOUT_MSEC);

	return ret;
}

void panel_wake_unlock(struct panel_device *panel)
{
	decon_wake_unlock_global(0);
}

extern void parse_lcd_info(struct device_node *node, struct decon_lcd *lcd_info);

static int panel_parse_lcd_info(struct panel_device *panel)
{
	int ret = 0;
	struct device_node *node;
	struct device *dev = panel->dev;

	node = find_panel_ddi_node(panel, boot_panel_id);
	if (!node) {
		panel_err("%s, panel not found (boot_panel_id 0x%08X)\n",
				__func__, boot_panel_id);
		node = of_parse_phandle(dev->of_node, "ddi-info", 0);
	}
	panel->ddi_node = node;

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	panel->lcd_info.adaptive_info.adap_idx =
		&panel->adap_idx;
#endif
	parse_lcd_info(node, &panel->lcd_info);

	return ret;
}
#else
static int panel_parse_lcd_info(struct panel_device *panel)
{
	int ret = 0;
	struct device_node *node;
	struct device *dev = panel->dev;
	unsigned int res[3];
	struct decon_lcd *panel_info = &panel->lcd_info;

#ifdef CONFIG_SUPPORT_DSU
	int dsu_number, i;
	unsigned int dsu_res[MAX_DSU_RES_NUMBER];
	struct dsu_info_dt *dt_dsu_info = &panel_info->dt_dsu_info;
#endif

	node = of_parse_phandle(dev->of_node, "ddi_info", 0);

	of_property_read_u32(node, "mode", &panel_info->mode);
	panel_dbg("PANEL:DBG:mode : %s\n", panel_info->mode ? "command" : "video");

	of_property_read_u32_array(node, "resolution", res, 2);
	panel_info->xres = res[0];
	panel_info->yres = res[1];
	panel_dbg("PAENL:DBG: LCD(%s) resolution: xres(%d), yres(%d)\n",
			of_node_full_name(node), panel_info->xres, panel_info->yres);

	of_property_read_u32_array(node, "size", res, 2);
	panel_info->width = res[0];
	panel_info->height = res[1];
	panel_dbg("LCD size: width(%d), height(%d)\n", res[0], res[1]);

	of_property_read_u32(node, "timing,refresh", &panel_info->fps);
	panel_dbg("PANEL:DBG:LCD refresh rate(%d)\n", panel_info->fps);

	of_property_read_u32_array(node, "timing,h-porch", res, 3);
	panel_info->hbp = res[0];
	panel_info->hfp = res[1];
	panel_info->hsa = res[2];
	panel_dbg("PANEL:DBG:hbp(%d), hfp(%d), hsa(%d)\n",
		panel_info->hbp, panel_info->hfp, panel_info->hsa);

	of_property_read_u32_array(node, "timing,v-porch", res, 3);
	panel_info->vbp = res[0];
	panel_info->vfp = res[1];
	panel_info->vsa = res[2];
	panel_dbg("PANEL:DBG:vbp(%d), vfp(%d), vsa(%d)\n",
		panel_info->vbp, panel_info->vfp, panel_info->vsa);


	of_property_read_u32(node, "timing,dsi-hs-clk", &panel_info->hs_clk);
	//dsim->clks.hs_clk = dsim->lcd_info.hs_clk;
	panel_dbg("PANEL:DBG:requested hs clock(%d)\n", panel_info->hs_clk);

	of_property_read_u32_array(node, "timing,pms", res, 3);
	panel_info->dphy_pms.p = res[0];
	panel_info->dphy_pms.m = res[1];
	panel_info->dphy_pms.s = res[2];
	panel_dbg("PANEL:DBG:p(%d), m(%d), s(%d)\n",
		panel_info->dphy_pms.p, panel_info->dphy_pms.m, panel_info->dphy_pms.s);


	of_property_read_u32(node, "timing,dsi-escape-clk", &panel_info->esc_clk);
	//dsim->clks.esc_clk = panel_dbg->esc_clk;
	panel_dbg("PANEL:DBG:requested escape clock(%d)\n", panel_info->esc_clk);


	of_property_read_u32(node, "mic_en", &panel_info->mic_enabled);
	panel_dbg("PANEL:DBG:mic enabled (%d)\n", panel_info->mic_enabled);

	of_property_read_u32(node, "type_of_ddi", &panel_info->ddi_type);
	panel_dbg("PANEL:DBG:ddi type(%d)\n",  panel_info->ddi_type);

	of_property_read_u32(node, "dsc_en", &panel_info->dsc_enabled);
	panel_dbg("PANEL:DBG:dsc is %s\n", panel_info->dsc_enabled ? "enabled" : "disabled");

	if (panel_info->dsc_enabled) {
		of_property_read_u32(node, "dsc_cnt", &panel_info->dsc_cnt);
		panel_dbg("PANEL:DBG:dsc count(%d)\n", panel_info->dsc_cnt);

		of_property_read_u32(node, "dsc_slice_num", &panel_info->dsc_slice_num);
		panel_dbg("PANEL:DBG:dsc slice count(%d)\n", panel_info->dsc_slice_num);

		of_property_read_u32(node, "dsc_slice_h", &panel_info->dsc_slice_h);
		panel_dbg("PANEL:DBG:dsc slice height(%d)\n", panel_info->dsc_slice_h);
	}
	of_property_read_u32(node, "data_lane",
		&panel_info->data_lane);
	panel_dbg("PANEL:DBG:using data lane count(%d)\n",
		panel_info->data_lane);
#ifdef CONFIG_SUPPORT_DSU
	of_property_read_u32(node, "dsu_en", &dt_dsu_info->dsu_en);
	panel_info("PANEL:INFO:%s:dsu_en : %d\n", __func__, dt_dsu_info->dsu_en);

	if (dt_dsu_info->dsu_en) {
		of_property_read_u32(node, "dsu_number", &dsu_number);
		panel_info("PANEL:INFO:%s:dsu_number : %d\n", __func__, dsu_number);
		if (dsu_number > MAX_DSU_RES_NUMBER) {
			panel_err("PANEL:ERR:%s:Exceed dsu res number : %d\n",
				__func__, dsu_number);
			dsu_number = MAX_DSU_RES_NUMBER;
		}
		dt_dsu_info->dsu_number = dsu_number;

		of_property_read_u32_array(node, "dsu_width", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].width = dsu_res[i];
			panel_info("PANEL:INFO:%s:width[%d]:%d\n", __func__, i, dsu_res[i]);
		}
		of_property_read_u32_array(node, "dsu_height", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].height = dsu_res[i];
			panel_info("PANEL:INFO:%s:height[%d]:%d\n", __func__, i, dsu_res[i]);
		}
		of_property_read_u32_array(node, "dsu_dsc_en", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].dsc_en = dsu_res[i];
			panel_info("PANEL:INFO:%s:dsc_en[%d]:%d\n", __func__, i, dsu_res[i]);
		}

		of_property_read_u32_array(node, "dsu_dsc_width", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].dsc_width = dsu_res[i];
			panel_info("PANEL:INFO:%s:dsc_width[%d]:%d\n", __func__, i, dsu_res[i]);
		}
		of_property_read_u32_array(node, "dsu_dsc_height", dsu_res, dsu_number);
		for (i = 0; i < dsu_number; i++) {
			dt_dsu_info->res_info[i].dsc_height = dsu_res[i];
			panel_info("PANEL:INFO:%s:dsc_height[%d]:%d\n", __func__, i, dsu_res[i]);
		}
	}
#endif
	return ret;
}
#endif

static int panel_parse_panel_lookup(struct panel_device *panel)
{
	struct device *dev = panel->dev;
	struct panel_info *panel_data = &panel->panel_data;
	struct panel_lut_info *lut_info = &panel_data->lut_info;
	struct device_node *np;
	int ret, i, sz, sz_lut;

	np = of_get_child_by_name(dev->of_node, "panel-lookup");
	if (unlikely(!np)) {
		panel_warn("PANEL:WARN:%s:No DT node for panel-lookup\n", __func__);
		return -EINVAL;
	}

	sz = of_property_count_strings(np, "panel-name");
	if (sz <= 0) {
		panel_warn("PANEL:WARN:%s:No panel-name property\n", __func__);
		return -EINVAL;
	}

	if (sz >= ARRAY_SIZE(lut_info->names)) {
		panel_warn("PANEL:WARN:%s:exceeded MAX_PANEL size\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < sz; i++) {
		ret = of_property_read_string_index(np,
				"panel-name", i, &lut_info->names[i]);
		if (ret) {
			panel_warn("PANEL:WARN:%s:failed to read panel-name[%d]\n",
					__func__, i);
			return -EINVAL;
		}
	}
	lut_info->nr_panel = sz;

	sz = of_property_count_u32_elems(np, "panel-ddi-info");
	if (sz <= 0) {
		panel_warn("PANEL:WARN:%s:No ddi-info property\n", __func__);
		return -EINVAL;
	}

	if (sz >= ARRAY_SIZE(lut_info->ddi_node)) {
		panel_warn("PANEL:WARN:%s:exceeded MAX_PANEL_DDI size\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < sz; i++) {
		lut_info->ddi_node[i] = of_parse_phandle(np, "panel-ddi-info", i);
		if (!lut_info->ddi_node[i]) {
			panel_warn("PANEL:WARN:%s:failed to get phandle of ddi-info[%d]\n",
					__func__, i);
			return -EINVAL;
		}
	}
	lut_info->nr_panel_ddi = sz;

	sz_lut = of_property_count_u32_elems(np, "panel-lut");
	if ((sz_lut % 4) || (sz_lut >= MAX_PANEL_LUT)) {
		panel_warn("PANEL:WARN:%s:sz_lut(%d) should be multiple of 4 and less than MAX_PANEL_LUT\n",
			__func__, sz_lut);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "panel-lut",
			(u32 *)lut_info->lut, sz_lut);
	if (ret) {
		panel_warn("PANEL:WARN:%s:failed to read panel-lut\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < sz_lut / 4; i++) {
		if (lut_info->lut[i].index >= lut_info->nr_panel) {
			panel_warn("PANEL:WARN:%s:invalid panel index(%d)\n",
					__func__, lut_info->lut[i].index);
			return -EINVAL;
		}
	}
	lut_info->nr_lut = sz_lut / 4;

	print_panel_lut(lut_info);

	return 0;
}

static int panel_parse_dt(struct panel_device *panel)
{
	int ret = 0;
	struct device *dev = panel->dev;

	if (IS_ERR_OR_NULL(dev->of_node)) {
		panel_err("PANEL:ERR:%s:failed to get dt info\n", __func__);
		return -EINVAL;
	}

	panel->id = of_alias_get_id(dev->of_node, "panel");
	if (panel->id < 0) {
		panel_err("PANEL:ERR:%s:invalid panel's id : %d\n",
			__func__, panel->id);
		return panel->id;
	}
	panel_info("PANEL:INFO:%s:panel-%d\n", __func__, panel->id);

	ret = panel_parse_gpio(panel);
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:panel-%d:failed to parse gpio\n",
			__func__, panel->id);
		return ret;
	}

	ret = panel_parse_regulator(panel);
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:panel-%d:failed to parse regulator\n",
			__func__, panel->id);
		return ret;
	}

	ret = panel_parse_panel_lookup(panel);
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:panel-%d:failed to parse panel lookup\n",
			__func__, panel->id);
		return ret;
	}

	ret = panel_parse_lcd_info(panel);
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:panel-%d:failed to parse lcd_info\n",
			__func__, panel->id);
		return ret;
	}

	return 0;
}

static void disp_det_handler(struct work_struct *work)
{
	int ret, disp_det_state;
	struct panel_work *w = container_of(to_delayed_work(work),
			struct panel_work, dwork);
	struct panel_device *panel =
		container_of(w, struct panel_device, work[PANEL_WORK_DISP_DET]);
	struct panel_gpio *gpio = panel->gpio;
	struct panel_state *state = &panel->state;

	disp_det_state = panel_disp_det_state(panel);
	panel_info("PANEL:INFO:%s: disp_det_state:%s\n",
			__func__, disp_det_state == PANEL_STATE_OK ? "OK" : "NOK");

	switch (state->cur_state) {
	case PANEL_STATE_ALPM:
	case PANEL_STATE_NORMAL:
		if (disp_det_state == PANEL_STATE_NOK) {
			disable_irq(gpio[PANEL_GPIO_DISP_DET].irq);
			/* delay for disp_det deboundce */
			usleep_range(10000, 11000);

			panel_err("PANEL:ERR:%s:disp_det is abnormal state\n",
					__func__);
			ret = panel_error_cb(panel);
			if (ret)
				panel_err("PANEL:ERR:%s:failed to recover panel\n",
						__func__);
			clear_pending_bit(panel->gpio[PANEL_GPIO_DISP_DET].irq);
			enable_irq(gpio[PANEL_GPIO_DISP_DET].irq);
		}
		break;
	default:
		break;
	}
}

void panel_send_ubconn_uevent(struct panel_device *panel)
{
	char *uevent_conn_str[3] = {"CONNECTOR_NAME=UB_CONNECT", "CONNECTOR_TYPE=HIGH_LEVEL", NULL};

	kobject_uevent_env(&panel->lcd->dev.kobj, KOBJ_CHANGE, uevent_conn_str);
	panel_info("%s, %s, %s\n", __func__, uevent_conn_str[0], uevent_conn_str[1]);
}

void conn_det_handler(struct work_struct *data)
{
	struct panel_work *w = container_of(to_delayed_work(data),
		struct panel_work, dwork);
	struct panel_device *panel =
		container_of(w, struct panel_device, work[PANEL_WORK_CONN_DET]);
	panel_info("%s state:%d cnt:%d\n",
		__func__, ub_con_disconnected(panel), panel->panel_data.props.ub_con_cnt);
	if (!ub_con_disconnected(panel))
		return;
	if (panel->panel_data.props.conn_det_enable)
		panel_send_ubconn_uevent(panel);
	panel->panel_data.props.ub_con_cnt++;
}

void err_fg_handler(struct work_struct *data)
{
	pr_info("%s\n", __func__);
}

static int panel_fb_notifier(struct notifier_block *self, unsigned long event, void *data)
{
	int *blank = NULL;
	struct panel_device *panel;
	struct fb_event *fb_event = data;

	switch (event) {
	case FB_EARLY_EVENT_BLANK:
	case FB_EVENT_BLANK:
		break;
	case FB_EVENT_FB_REGISTERED:
		panel_dbg("PANEL:INFO:%s:FB Registeted\n", __func__);
		return 0;
	default:
		return 0;
	}

	panel = container_of(self, struct panel_device, fb_notif);
	blank = fb_event->data;
	if (!blank || !panel) {
		panel_err("PANEL:ERR:%s:blank is null\n", __func__);
		return 0;
	}

	switch (*blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_NORMAL:
		if (event == FB_EARLY_EVENT_BLANK)
			panel_dbg("PANEL:INFO:%s:EARLY BLANK POWER DOWN\n", __func__);
		else
			panel_dbg("PANEL:INFO:%s:BLANK POWER DOWN\n", __func__);
		break;
	case FB_BLANK_UNBLANK:
		if (event == FB_EARLY_EVENT_BLANK)
			panel_dbg("PANEL:INFO:%s:EARLY UNBLANK\n", __func__);
		else
			panel_dbg("PANEL:INFO:%s:UNBLANK\n", __func__);
		break;
	}
	return 0;
}

#ifdef CONFIG_DISPLAY_USE_INFO
unsigned int g_rddpm = 0xFF;
unsigned int g_rddsm = 0xFF;

unsigned int get_panel_bigdata(void)
{
	unsigned int val = 0;

	val = (g_rddsm << 8) | g_rddpm;

	return val;
}

static int panel_dpui_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	struct common_panel_info *info;
	int panel_id;
	struct dpui_info *dpui = data;
	char tbuf[MAX_DPUI_VAL_LEN];
	u8 panel_datetime[7] = { 0, };
	u8 panel_coord[4] = { 0, };
	int i, site, rework, poc;
	u8 cell_id[16], octa_id[PANEL_OCTA_ID_LEN] = { 0, };
	bool cell_id_exist = true;
	int size;

	if (dpui == NULL) {
		panel_err("%s: dpui is null\n", __func__);
		return 0;
	}

	panel = container_of(self, struct panel_device, panel_dpui_notif);
	panel_data = &panel->panel_data;
	panel_id = panel_data->id[0] << 16 | panel_data->id[1] << 8 | panel_data->id[2];

	info = find_panel(panel, panel_id);
	if (unlikely(!info)) {
		panel_err("%s, panel not found\n", __func__);
		return -ENODEV;
	}

	resource_copy_by_name(panel_data, panel_datetime, "date");
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%04d%02d%02d %02d%02d%02d",
			((panel_datetime[0] & 0xF0) >> 4) + 2011, panel_datetime[0] & 0xF, panel_datetime[1] & 0x1F,
			panel_datetime[2] & 0x1F, panel_datetime[3] & 0x3F, panel_datetime[4] & 0x3F);
	set_dpui_field(DPUI_KEY_MAID_DATE, tbuf, size);

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", panel_data->id[0]);
	set_dpui_field(DPUI_KEY_LCDID1, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", panel_data->id[1]);
	set_dpui_field(DPUI_KEY_LCDID2, tbuf, size);
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d", panel_data->id[2]);
	set_dpui_field(DPUI_KEY_LCDID3, tbuf, size);

	resource_copy_by_name(panel_data, panel_coord, "coordinate");
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
		panel_datetime[0], panel_datetime[1], panel_datetime[2], panel_datetime[3],
		panel_datetime[4], panel_datetime[5], panel_datetime[6],
		panel_coord[0], panel_coord[1], panel_coord[2], panel_coord[3]);
	set_dpui_field(DPUI_KEY_CELLID, tbuf, size);

	/* OCTAID */
	resource_copy_by_name(panel_data, octa_id, "octa_id");
	site = (octa_id[0] >> 4) & 0x0F;
	rework = octa_id[0] & 0x0F;
	poc = octa_id[1] & 0x0F;

	for (i = 0; i < 16; i++) {
		cell_id[i] = isalnum(octa_id[i + 4]) ? octa_id[i + 4] : '\0';
		if (cell_id[i] == '\0') {
			cell_id_exist = false;
			break;
		}
	}
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%d%d%d%02x%02x",
			site, rework, poc, octa_id[2], octa_id[3]);
	if (cell_id_exist) {
		for (i = 0; i < 16; i++)
			size += snprintf(tbuf + size, MAX_DPUI_VAL_LEN - size, "%c", cell_id[i]);
	}
	set_dpui_field(DPUI_KEY_OCTAID, tbuf, size);

#ifdef CONFIG_SUPPORT_DIM_FLASH
	size = snprintf(tbuf, MAX_DPUI_VAL_LEN,
			"%d", panel->work[PANEL_WORK_DIM_FLASH].ret);
	set_dpui_field(DPUI_KEY_PNGFLS, tbuf, size);
#endif

	inc_dpui_u32_field(DPUI_KEY_UB_CON, panel->panel_data.props.ub_con_cnt);
	panel->panel_data.props.ub_con_cnt = 0;

	return 0;
}
#endif /* CONFIG_DISPLAY_USE_INFO */

#ifdef CONFIG_SUPPORT_TDMB_TUNE
static int panel_tdmb_notifier_callback(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct panel_info *panel_data;
	struct panel_device *panel;
	struct tdmb_notifier_struct *value = data;
	int ret;

	panel = container_of(nb, struct panel_device, tdmb_notif);
	panel_data = &panel->panel_data;

	mutex_lock(&panel->io_lock);
	mutex_lock(&panel->op_lock);
	switch (value->event) {
	case TDMB_NOTIFY_EVENT_TUNNER:
		panel_data->props.tdmb_on = value->tdmb_status.pwr;
		if (!IS_PANEL_ACTIVE(panel)) {
			pr_info("%s keep tdmb state (%s) and affect later\n",
					__func__, panel_data->props.tdmb_on ? "on" : "off");
			break;
		}
		pr_info("%s tdmb state (%s)\n",
				__func__, panel_data->props.tdmb_on ? "on" : "off");
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_TDMB_TUNE_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s, failed to write tdmb-tune seqtbl\n", __func__);
		panel_data->props.cur_tdmb_on = panel_data->props.tdmb_on;
		break;
	default:
		break;
	}
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->io_lock);
	return 0;
}
#endif

static int panel_init_work(struct panel_work *w,
		char *name, panel_wq_handler handler)
{
	if (w == NULL || name == NULL || handler == NULL) {
		panel_err("PANEL:ERR:%s:invalid parameter\n", __func__);
		return -EINVAL;
	}

	mutex_init(&w->lock);
	INIT_DELAYED_WORK(&w->dwork, handler);
	w->wq = create_singlethread_workqueue(name);
	if (w->wq == NULL) {
		panel_err("PANEL:ERR:%s:failed to create %s workqueue\n",
				__func__, name);
		return -ENOMEM;
	}
	atomic_set(&w->running, 0);

	pr_info("%s %s done\n", __func__, name);
	return 0;
}

static int panel_drv_init_work(struct panel_device *panel)
{
	int i, ret;

	for (i = 0; i < PANEL_WORK_MAX; i++) {
		if (!panel_wq_handlers[i])
			continue;

		ret = panel_init_work(&panel->work[i],
				panel_work_names[i], panel_wq_handlers[i]);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to initialize panel_work(%s)\n",
					__func__, panel_work_names[i]);
			return ret;
		}
	}

	return 0;
}

static int panel_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct panel_device *panel = NULL;

	panel = devm_kzalloc(dev, sizeof(struct panel_device), GFP_KERNEL);
	if (!panel) {
		panel_err("failed to allocate dsim device.\n");
		ret = -ENOMEM;
		goto probe_err;
	}
	panel->dev = dev;

	panel->state.init_at = PANEL_INIT_BOOT;
	panel->state.connect_panel = PANEL_CONNECT;
	panel->state.connected = true;
	panel->state.cur_state = PANEL_STATE_OFF;
	panel->state.power = PANEL_POWER_OFF;
	panel->state.disp_on = PANEL_DISPLAY_OFF;
	panel->ktime_panel_on = ktime_get();
#ifdef CONFIG_SUPPORT_HMD
	panel->state.hmd_on = PANEL_HMD_OFF;
#endif
#ifdef CONFIG_ACTIVE_CLOCK
	panel->act_clk_dev.act_info.update_img = IMG_UPDATE_NEED;
#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	/* Caution!! Check default freq @ mipi dt files */
	/* adap_idx was initialized to undefined value.. */
	/* after 1'st ril notifier call this value was changed to appropriate value */
	panel->adap_idx.cur_freq_idx = 0;
	panel->adap_idx.req_freq_idx = 0;
#endif

	mutex_init(&panel->op_lock);
	mutex_init(&panel->data_lock);
	mutex_init(&panel->io_lock);
	mutex_init(&panel->panel_bl.lock);
	mutex_init(&panel->mdnie.lock);
	mutex_init(&panel->copr.lock);

	panel_parse_dt(panel);

	panel_drv_set_gpios(panel);

	panel_drv_set_regulators(panel);

	panel_init_v4l2_subdev(panel);

	platform_set_drvdata(pdev, panel);

	panel_drv_init_work(panel);

	panel->fb_notif.notifier_call = panel_fb_notifier;
	ret = fb_register_client(&panel->fb_notif);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register fb notifier callback\n", __func__);
		goto probe_err;
	}

#ifdef CONFIG_DISPLAY_USE_INFO
	panel->panel_dpui_notif.notifier_call = panel_dpui_notifier_callback;
	ret = dpui_logging_register(&panel->panel_dpui_notif, DPUI_TYPE_PANEL);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register dpui notifier callback\n", __func__);
		goto probe_err;
	}
#endif
#ifdef CONFIG_SUPPORT_TDMB_TUNE
	ret = tdmb_notifier_register(&panel->tdmb_notif,
			panel_tdmb_notifier_callback, TDMB_NOTIFY_DEV_LCD);
	if (ret) {
		panel_err("PANEL:ERR:%s:failed to register tdmb notifier callback\n",
				__func__);
		goto probe_err;
	}
#endif

	panel_register_isr(panel);
probe_err:
	return ret;
}

static const struct of_device_id panel_drv_of_match_table[] = {
	{ .compatible = "samsung,panel-drv", },
	{ },
};
MODULE_DEVICE_TABLE(of, panel_drv_of_match_table);

static struct platform_driver panel_driver = {
	.probe = panel_drv_probe,
	.driver = {
		.name = "panel-drv",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(panel_drv_of_match_table),
	}
};

static int __init get_boot_panel_id(char *arg)
{
	get_option(&arg, &boot_panel_id);
	panel_info("PANEL:INFO:%s:boot_panel_id : 0x%x\n",
			__func__, boot_panel_id);

	return 0;
}

early_param("lcdtype", get_boot_panel_id);

static int __init panel_drv_init(void)
{
	return platform_driver_register(&panel_driver);
}

static void __exit panel_drv_exit(void)
{
	platform_driver_unregister(&panel_driver);
}

module_init(panel_drv_init);
module_exit(panel_drv_exit);
MODULE_DESCRIPTION("Samsung's Panel Driver");
MODULE_AUTHOR("<minwoo7945.kim@samsung.com>");
MODULE_LICENSE("GPL");
