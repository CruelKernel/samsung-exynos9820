/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Samsung's Panel Driver
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
#if defined(CONFIG_TDMB_NOTIFIER)
#include <linux/tdmb_notifier.h>
#endif

static char *panel_state_names[] = {
	"OFF",		/* POWER OFF */
	"ON",		/* POWER ON */
	"NORMAL",	/* SLEEP OUT */
	"LPM",		/* LPM */
};

static int boot_panel_id = 0;
int panel_log_level = 6;
#ifdef CONFIG_SUPPORT_PANEL_SWAP
int panel_reprobe(struct panel_device *panel);
#endif

#ifdef CONFIG_SUPPORT_DOZE
#define CONFIG_SET_1p5_ALPM
#define BUCK_ALPM_VOLT		1500000
#define BUCK_NORMAL_VOLT	1600000
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

static int panel_disp_det_state(struct panel_device *panel)
{
	struct panel_pad *pad = &panel->pad;

	if (!pad->gpio_disp_det)
		return -EINVAL;

	return (gpio_get_value(pad->gpio_disp_det) == PANEL_EL_ON ?
			PANEL_STATE_OK : PANEL_STATE_NOK);
}

static int ub_con_det_state(struct panel_device *panel)
{
	struct panel_pad *pad = &panel->pad;

	if (!pad->gpio_ub_con_det)
		return -EINVAL;

	pr_debug("%s: ub_con_det %s\n", __func__,
			(gpio_get_value(pad->gpio_ub_con_det) == PANEL_UB_CONNECTED ?
			 "connected" : "disconnected"));

	return (gpio_get_value(pad->gpio_ub_con_det) == PANEL_UB_CONNECTED ?
			PANEL_STATE_OK : PANEL_STATE_NOK);
}

bool ub_con_disconnected(struct panel_device *panel)
{
	int state;

	state = ub_con_det_state(panel);
	if (state < 0)
		return false;

	return !state;
}

void clear_disp_det_pend(struct panel_device *panel)
{
	int pend_disp_det;
	struct panel_pad *pad = &panel->pad;

	if (pad->pend_reg_disp_det != NULL) {
		pend_disp_det = readl(pad->pend_reg_disp_det);
		pend_disp_det = pend_disp_det & ~(pad->pend_bit_disp_det);
		writel(pend_disp_det, pad->pend_reg_disp_det);
	}

	return;
}

#define SSD_CURRENT_DOZE	8000
#define SSD_CURRENT_NORMAL	2000

int __set_panel_power(struct panel_device *panel, int power)
{
	int i;
	int ret = 0;
	struct panel_pad *pad = &panel->pad;
#ifdef CONFIG_DISP_PMIC_SSD
	struct regulator *elvss = regulator_get(NULL, "elvss");
#endif
	if (panel->state.power == power) {
		panel_warn("PANEL:WARN:%s:same status.. skip..\n", __func__);
		goto set_err;
	}

	if (power == PANEL_POWER_ON) {
#ifdef CONFIG_DISP_PMIC_SSD
		if (!IS_ERR_OR_NULL(elvss)) {
			ret = regulator_set_short_detection(elvss, true, SSD_CURRENT_NORMAL);
			if (ret)
				panel_err("PANEL:ERR:%s:failed to ssd current to normal\n", __func__);
			panel_info("PANEL:INFO:set SSD to %duA : %d\n", SSD_CURRENT_NORMAL, ret);
		} else {
			panel_err("PANEL:ERR:%s:failed to get elvss regulator\n", __func__);
		}
#endif
		for (i = 0; i < REGULATOR_MAX; i++) {
			ret = regulator_enable(pad->regulator[i]);
			if (ret) {
				panel_err("PANEL:%s:faield to enable regulator : %d :%d\n",
					__func__, i, ret);
				goto set_err;
			}
		}
		usleep_range(10000, 10000);
		gpio_direction_output(pad->gpio_reset, 1);
		usleep_range(5000, 5000);
	} else {
		gpio_direction_output(pad->gpio_reset, 0);
		for (i = REGULATOR_MAX - 1; i >= 0; i--) {
			ret = regulator_disable(pad->regulator[i]);
			if (ret) {
				panel_err("PANEL:%s:faield to disable regulator : %d :%d\n",
					__func__, i, ret);
				goto set_err;
			}
		}
		pr_info("%s reset panel (%s)\n", __func__,
				gpio_get_value(pad->gpio_reset) ? "high" : "low");
	}

	pr_info("%s, power(%s) gpio_reset(%s)\n", __func__,
			power == PANEL_POWER_ON ? "on" : "off",
			gpio_get_value(pad->gpio_reset) ? "high" : "low");
	panel->state.power = power;
set_err:
#ifdef CONFIG_DISP_PMIC_SSD
	regulator_put(elvss);
#endif
	return ret;
}

int __panel_seq_display_on(struct panel_device *panel)
{
	int ret = 0;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DISPLAY_ON_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

	return ret;
}

int __panel_seq_display_off(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DISPLAY_OFF_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

	return ret;
}

static int __panel_seq_res_init(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_RES_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write resource init seqtbl\n",
				__func__);
	}

	return ret;
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static int __panel_seq_dim_flash_res_init(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DIM_FLASH_RES_INIT_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write dimming flash resource init seqtbl\n",
				__func__);
	}

	return ret;
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
		usleep_range(1000, 1000);
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

	clear_disp_det_pend(panel);

	enable_irq(panel->pad.irq_disp_det);

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

	disable_irq(panel->pad.irq_disp_det);

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
#ifdef CONFIG_SET_1p5_ALPM
	int volt;
	struct panel_pad *pad = &panel->pad;
#endif
	panel_info("PANEL:INFO:%s:was called\n", __func__);

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_exit_from_lpm(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to exit_lpm ops\n", __func__);
#endif

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
#ifdef CONFIG_SET_1p5_ALPM
	volt = regulator_get_voltage(pad->regulator[REGULATOR_1p6V]);
	panel_info("PANEL:INFO:%s:regulator voltage  : %d\n", __func__, volt);
	if (volt != BUCK_NORMAL_VOLT) {
		ret = regulator_set_voltage(pad->regulator[REGULATOR_1p6V],
			BUCK_NORMAL_VOLT, BUCK_NORMAL_VOLT);
		if (ret)
			panel_warn("PANEL:WARN:%s:failed to set volt to 1.6\n",
				__func__);
	}
#endif

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_EXIT_SEQ);
	if (ret)
		panel_err("PANEL:ERR:%s, failed to alpm-exit\n", __func__);
#ifdef CONFIG_SUPPORT_AOD_BL
	panel->panel_data.props.lpm_brightness = -1;
	panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);
	panel_update_brightness(panel);

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
		usleep_range(delay, delay);
	}
	panel_info("PANEL:INFO:%stotal elapsed time : %d\n", __func__,
		(int)ktime_to_us(ktime_sub(ktime_get(), panel->ktime_panel_on)));
exit_delay:
	return;
}

static int __panel_seq_set_alpm(struct panel_device *panel)
{
	int ret = 0;
	int volt = 0;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
#ifdef CONFIG_SET_1p5_ALPM
	struct panel_pad *pad = &panel->pad;
#endif
#ifdef CONFIG_DISP_PMIC_SSD
	struct regulator *elvss = regulator_get(NULL, "elvss");
#endif
	panel_info("PANEL:INFO:%s:was called\n", __func__);

#ifdef CONFIG_DISP_PMIC_SSD
	if (!IS_ERR_OR_NULL(elvss)) {
		ret = regulator_set_short_detection(elvss, true, SSD_CURRENT_DOZE);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to ssd current to lpm\n", __func__);
		panel_info("PANEL:INFO:set SSD to %duA : %d\n", SSD_CURRENT_DOZE, ret);
	} else {
		panel_err("PANEL:ERR:%s:failed to get elvss regulator\n", __func__);
	}
#endif
	__delay_normal_alpm(panel);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_ENTER_SEQ);
	if (ret)
		panel_err("PANEL:ERR:%s, failed to alpm-enter\n", __func__);
#ifdef CONFIG_SUPPORT_AOD_BL
	panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_AOD);
#endif

#ifdef CONFIG_SET_1p5_ALPM
	volt = regulator_get_voltage(pad->regulator[REGULATOR_1p6V]);
	panel_info("PANEL:INFO:%s:regulator voltage : %d\n", __func__, volt);
	if (volt != BUCK_ALPM_VOLT) {
		ret = regulator_set_voltage(pad->regulator[REGULATOR_1p6V],
			BUCK_ALPM_VOLT, BUCK_ALPM_VOLT);
		if (ret)
			panel_warn("PANEL:WARN:%s:failed to set volt to 1.5\n",
				__func__);
	}
#endif
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_enter_to_lpm(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to enter to lpm\n", __func__);
#endif

#ifdef CONFIG_DISP_PMIC_SSD
	regulator_put(elvss);
#endif
	return ret;
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
				if (unlikely(ret < 0)) {
					panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
				}
				act_info->update_img = IMG_UPDATE_DONE;
			}
		}
		usleep_range(5, 5);

		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
		}
	}

	if (blink_info->en) {
		panel_dbg("PANEL:DBG:%s:active blink was enabed\n", __func__);

		ret = panel_do_seqtbl_by_index(panel, PANEL_ACTIVE_CLK_CTRL_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
		}
	}
	return ret;
}
#endif

static int __panel_seq_dump(struct panel_device *panel)
{
	int ret;

	ret = panel_do_seqtbl_by_index(panel, PANEL_DUMP_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write dump seqtbl\n", __func__);
	}

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
	struct dim_flash_result *result = &panel->dim_flash_work.result;
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
static void panel_update_dim_flash_work(struct work_struct *work)
{
	struct panel_work *pn_work = container_of(to_delayed_work(work),
			struct panel_work, dwork);
	struct panel_device *panel =
		container_of(pn_work, struct panel_device, dim_flash_work);
	int ret;

	if (atomic_read(&panel->dim_flash_work.running)) {
		pr_info("%s already running\n", __func__);
		return;
	}

	atomic_set(&panel->dim_flash_work.running, 1);
	mutex_lock(&panel->panel_bl.lock);
	pr_info("%s +\n", __func__);
	ret = panel_update_dim_type(panel, DIM_TYPE_DIM_FLASH);
	if (ret < 0) {
		pr_err("%s, failed to update dim_flash %d\n",
				__func__, ret);
		pn_work->ret = ret;
	} else {
		pr_info("%s, update dim_flash done %d\n",
				__func__, ret);
		pn_work->ret = GAMMA_FLASH_SUCCESS;
	}
	pr_info("%s -\n", __func__);
	mutex_unlock(&panel->panel_bl.lock);
	panel_update_brightness(panel);
	atomic_set(&panel->dim_flash_work.running, 0);
}
#endif


void test_work_handler(struct work_struct *work)
{
	unsigned int i, j;

	panel_info("workqueue_test : %s : stage 1\n", __func__);

	for (j = 2; j < 4; j++) {
		for (i = 0; i < 5000; i++)
			//usleep_range(100, 100);
			udelay(10);

		panel_info("workqueue_test : %s : stage %d\n", __func__, j);
	}

}

void test_load_work_handler(struct work_struct *work)
{
	unsigned int i, j;

	struct panel_device *panel =
		container_of(work, struct panel_device, test_load_work_item);

	panel_info("workqueue_test : %s : stage 1\n", __func__);

	for (j = 2; j < 4; j++) {
		for (i = 0; i < 5000; i++)
			//usleep_range(100, 100);
			udelay(10);
		panel_info("workqueue_test : %s : stage %d\n", __func__, j);
		if (j == 2) {
			queue_work(panel->test1_workqueue, &panel->test1_work_item);
		}
	}
}

int test_workqueue(struct panel_device *panel)
{
	/* fix load workqueue to create workqueue*/
	INIT_WORK(&panel->test_load_work_item, test_load_work_handler);
	panel->test_load_workqueue = create_workqueue("test_load");


	INIT_WORK(&panel->test1_work_item, test_work_handler);
	//panel->test1_workqueue = create_workqueue("test_work1");
	//panel->test1_workqueue = create_singlethread_workqueue("test_work1");
	panel->test1_workqueue = alloc_workqueue("test_work", WQ_UNBOUND, 3);
	//panel->test1_workqueue = alloc_workqueue("test_work", 0, 3);

	return 0;
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
	/* TODO : move to parse dt function
	   1. get panel_spi device node.
	   2. get spi_device of node
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
	panel_data->props.irc_mode = 0;
	mutex_unlock(&panel->panel_bl.lock);

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	panel_data->props.cur_ffc_idx = 0;
#endif

#ifdef CONFIG_SUPPORT_DSU
	panel_data->props.mres_updated = false;
#endif

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
		queue_delayed_work(panel->dim_flash_work.wq,
				&panel->dim_flash_work.dwork, msecs_to_jiffies(500));
#endif /* CONFIG_SUPPORT_DIM_FLASH */

	test_workqueue(panel);
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

	panel->state.ub_connected = ub_con_det_state(panel);
	if (panel->state.ub_connected < 0) {
		pr_debug("PANEL:INFO:%s:ub_con_det unsupported\n", __func__);
	} else if (panel->state.ub_connected == PANEL_STATE_NOK) {
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

	panel->state.ub_connected = ub_con_det_state(panel);
	if (panel->state.ub_connected < 0) {
		pr_debug("PANEL:INFO:%s:ub_con_det unsupported\n", __func__);
	} else if (panel->state.ub_connected == PANEL_STATE_NOK) {
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
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

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
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s, failed to write init seqtbl\n", __func__);
	}

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

	if (ub_con_det_state(panel) == PANEL_STATE_NOK)
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

	if(*cmd == 0) {
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
	if (ret) {
		pr_err("%s : fail to set brightness\n", __func__);
	}

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

	switch(cmd) {
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
			panel->state.ub_connected = ub_con_det_state(panel);
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

		case PANEL_IOC_PANEL_DUMP :
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
			}
			copr_update_start(&panel->copr, 3);
#ifdef CONFIG_SUPPORT_DSU
			if (panel->panel_data.props.mres_updated &&
					(++mres_updated_frame_cnt > 1)) {
				panel->panel_data.props.mres_updated = false;
				mres_updated_frame_cnt = 0;
			}
#endif
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
		default:
			panel_err("PANEL:ERR:%s:undefined command\n", __func__);
			ret = -EINVAL;
			break;
	}

	if (ret) {
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
	int ret = 0;
	int rst_val = -1, det_val = -1;
	struct panel_pad *pad = &panel->pad;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:pad is null\n", __func__);
		return -EINVAL;
	}

	if (pad->gpio_reset <= 0) {
		panel_err("PANEL:ERR:%s:gpio_reset not exist\n", __func__);
		return -EINVAL;
	}

	ret = gpio_request(pad->gpio_reset, "lcd_reset");
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:failed to request gpio reset\n",
				__func__);
		return -EINVAL;
	}
	rst_val = gpio_get_value(pad->gpio_reset);

	if (pad->gpio_disp_det <= 0) {
		panel_err("PANEL:ERR:%s:gpio_disp_det not exist\n", __func__);
		return -EINVAL;
	}

	ret = gpio_request_one(pad->gpio_disp_det, GPIOF_IN, "disp_det");
	if (ret < 0) {
		panel_err("PANEL:ERR:%s:failed to request gpio disp det : %d\n",
				__func__, ret);
		return -EINVAL;
	}
	det_val = gpio_get_value(pad->gpio_disp_det);

	/*
	 * panel state is decided by rst, ub_con_det and disp_det pin
	 *
	 * @rst_val
	 *  0 : need to init panel in kernel
	 *  1 : already initialized in bootloader
	 *
	 * @ub_con_det
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

	/* ub_connected : ub_con_det is connected or not */
	panel->state.ub_connected = ub_con_det_state(panel);

	/* connect_panel : decide to use or ignore panel */
	panel->state.connect_panel =
		(panel->state.init_at == PANEL_INIT_KERNEL || det_val == 1) ?
		PANEL_CONNECT : PANEL_DISCONNECT;

	if (panel->state.init_at == PANEL_INIT_KERNEL || det_val == 0) {
		gpio_direction_output(pad->gpio_reset, 0);
		panel->state.cur_state = PANEL_STATE_OFF;
		panel->state.power = PANEL_POWER_OFF;
		panel->state.disp_on = PANEL_DISPLAY_OFF;
	} else {
		gpio_direction_output(pad->gpio_reset, 1);
		panel->state.cur_state = PANEL_STATE_NORMAL;
		panel->state.power = PANEL_POWER_ON;
		panel->state.disp_on = PANEL_DISPLAY_ON;
	}

	panel_info("PANEL:INFO:%s: rst:%d, disp_det:%d (init_at:%s, ub_con:%d(%s) panel:%d(%s))\n",
		__func__, rst_val, det_val,
		(panel->state.init_at ? "BL" : "KERNEL"),
		panel->state.ub_connected,
		(panel->state.ub_connected < 0 ? "UNSUPPORTED" :
		(panel->state.ub_connected == true ? "CONNECTED" : "DISCONNECTED")),
		panel->state.connect_panel,
		(panel->state.connect_panel == PANEL_CONNECT ? "USE" : "NO USE"));

	return 0;
}

static inline int panel_get_gpio(struct device *dev ,char *name)
{
	int ret = 0;
	ret = of_gpio_named_count(dev->of_node, name);
	if (ret != 1) {
		panel_err("PANEL:ERR:%s:can't find gpio named : %s\n",
			__func__, name);
		return -EINVAL;
	}
	ret = of_get_named_gpio(dev->of_node, name, 0);
	if (!gpio_is_valid(ret)) {
		panel_err("PANEL:ERR:%s:failed to get gpio named : %s\n",
			__func__, name);
		return -EINVAL;
	}
	return ret;
}

static int panel_parse_gpio(struct panel_device *panel)
{
	int i;
	int ret;
	struct device_node *pend_disp_det;
	struct device *dev = panel->dev;
	struct panel_pad *pad = &panel->pad;
	int gpio_result[PANEL_GPIO_MAX];
	char *gpio_lists[PANEL_GPIO_MAX] = {
		GPIO_NAME_RESET,
		GPIO_NAME_DISP_DET,
		GPIO_NAME_PCD,
		GPIO_NAME_ERR_FG,
		GPIO_NAME_UB_CON_DET,
	};

	for (i = 0; i < PANEL_GPIO_MAX; i++) {
		ret = panel_get_gpio(dev, gpio_lists[i]);
		if (ret <= 0)
			 ret = 0;
		gpio_result[i] = ret;
	}
	pad->gpio_reset = gpio_result[PANEL_GPIO_RESET];
	pad->gpio_disp_det = gpio_result[PANEL_GPIO_DISP_DET];
	pad->gpio_pcd = gpio_result[PANEL_GPIO_PCD];
	pad->gpio_err_fg = gpio_result[PANEL_GPIO_ERR_FG];
	pad->gpio_ub_con_det = gpio_result[PANEL_GPIO_UB_CON_DET];

	ret = panel_drv_set_gpios(panel);
	if (pad->gpio_disp_det) {
		pad->irq_disp_det = gpio_to_irq(pad->gpio_disp_det);
		pend_disp_det = of_get_child_by_name(dev->of_node, "pend,disp-det");
		if (!pend_disp_det) {
			panel_warn("PANEL:WARN:%s:pend,disp_det not exist\n", __func__);
		} else {
			pad->pend_reg_disp_det = of_iomap(pend_disp_det, 0);
			if (!pad->pend_reg_disp_det) {
				panel_err("PANEL:ERR:%s:failed to get pend_disp_det\n",
						__func__);
				return -EINVAL;
			}
			of_property_read_u32(pend_disp_det, "pend-bit",
				&pad->pend_bit_disp_det);
			panel_info("PANEL:INFO:%s:pend_disp_det:0x%x (mask:0x%x)\n",
				__func__, readl(pad->pend_reg_disp_det), pad->pend_bit_disp_det);
		}
	}

	if (pad->gpio_pcd)
		pad->irq_pcd = gpio_to_irq(pad->gpio_pcd);

	if (pad->gpio_err_fg)
		pad->irq_err_fg = gpio_to_irq(pad->gpio_err_fg);

	if (pad->gpio_ub_con_det)
		pad->irq_ub_con_det = gpio_to_irq(pad->gpio_ub_con_det);

	if (pad->gpio_reset == 0) {
		panel_err("PANEL:ERR:%s:can't find gpio for panel reset\n",
			__func__);
		return -EINVAL;
	}

	return ret;
}


static int panel_parse_regulator(struct panel_device *panel)
{
	int i;
	int ret = 0;
	char *tmp_str;
	struct device *dev = panel->dev;
	struct panel_pad *pad = &panel->pad;
	struct regulator *reg[REGULATOR_MAX];
	char *reg_lists[REGULATOR_MAX] = {
		REGULATOR_3p0_NAME,
		REGULATOR_1p8_NAME,
		REGULATOR_1p6_NAME,
	};

	for (i = 0; i < REGULATOR_MAX; i++) {
		ret = of_property_read_string(dev->of_node, reg_lists[i],
			(const char **)&tmp_str);
		if (ret < 0) {
			panel_err("PANEL:ERR:%s:failed to get property:%s (ret %d)\n",
				__func__, reg_lists[i], ret);
			goto parse_regulator_err;
		}

		reg[i] = regulator_get(NULL, tmp_str);
		if (IS_ERR(reg[i])) {
			panel_err("PANEL:ERR:%s:failed to get regulator:%s:%s\n",
				__func__, reg_lists[i], tmp_str);
			ret = -EINVAL;
			goto parse_regulator_err;
		}
		if (panel->state.init_at == PANEL_INIT_BOOT) {
			ret = regulator_enable(reg[i]);
			if (ret) {
				panel_err("PANEL:ERR:%s:failed to enable regualtor:%s\n",
					__func__, tmp_str);
				goto parse_regulator_err;
			}
#ifdef CONFIG_SUPPORT_DOZE
#ifdef CONFIG_SET_1p5_ALPM
			if (i == REGULATOR_1p6V) {
				ret = regulator_set_voltage(reg[i], 1600000, 1600000);
				if (ret) {
					panel_err("PANEL:%s:failed to set volatege\n",
						__func__);
					goto parse_regulator_err;
				}
			}
#endif
#endif
			if (panel->state.connect_panel == PANEL_DISCONNECT) {
				panel_info("PANEL:INFO:%s:panel no use: disable : %s\n",
					__func__, reg_lists[i]);
				ret = regulator_disable(reg[i]);
				if (ret) {
					panel_err("PANEL:ERR:%s:failed to disable regualtor : %s\n",
						__func__, tmp_str);
					goto parse_regulator_err;
				}
			}
		}
	}
	pad->regulator[REGULATOR_3p0V] = reg[REGULATOR_3p0V];
	pad->regulator[REGULATOR_1p8V] = reg[REGULATOR_1p8V];
	pad->regulator[REGULATOR_1p6V] = reg[REGULATOR_1p6V];

	ret = 0;

	panel_dbg("PANEL:INFO:%s:panel parse regulator : done\n", __func__);

parse_regulator_err:
	return ret;
}


static irqreturn_t panel_disp_det_irq(int irq, void *dev_id)
{
	struct panel_device *panel = (struct panel_device *)dev_id;

	queue_work(panel->disp_det_workqueue, &panel->disp_det_work);

	return IRQ_HANDLED;
}

int panel_register_isr(struct panel_device *panel)
{
	int ret = 0;
	struct panel_pad *pad = &panel->pad;

	if (panel->state.connect_panel == PANEL_DISCONNECT)
		return -ENODEV;

	clear_disp_det_pend(panel);
	if (pad->gpio_disp_det) {
		ret = devm_request_irq(panel->dev, pad->irq_disp_det, panel_disp_det_irq,
			IRQF_TRIGGER_FALLING, "disp_det", panel);
		if (ret) {
			panel_dbg("PANEL:ERR:%s:failed to register disp det irq\n", __func__);
			return ret;
		}
	}
	return 0;
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
#if 0 //minwoo
	if (panel_info->mode == DECON_MIPI_COMMAND_MODE) {
		of_property_read_u32(node, "cmd_underrun_lp_ref",
			&panel_info->cmd_underrun_lp_ref[decon->mres_mode]);
		panel_dbg("PANEL:DBG:cmd_underrun_lp_ref(%d)\n",
			panel_info->cmd_underrun_lp_ref[decon->mres_mode]);
	} else {
		of_property_read_u32(node, "vt_compensation",
			&panel_info->vt_compensation);
		panel_dbg("PANEL:DBG:vt_compensation(%d)\n",
			panel_info->vt_compensation);
	}
#endif
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
		panel_warn("PANEL:WARN:%s:sz_lut(%d) should be multiple of 4"
				" and less than MAX_PANEL_LUT\n", __func__, sz_lut);
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
	panel_dbg("PANEL:INFO:%s:panel-id:%d\n", __func__, panel->id);

	panel_parse_gpio(panel);

	panel_parse_regulator(panel);

	panel_parse_panel_lookup(panel);

	panel_parse_lcd_info(panel);

	return ret;
}

void disp_det_handler(struct work_struct *data)
{
	int ret, disp_det_state;
	struct panel_device *panel =
			container_of(data, struct panel_device, disp_det_work);
	struct panel_pad *pad = &panel->pad;
	struct panel_state *state = &panel->state;

	disp_det_state = panel_disp_det_state(panel);
	panel_info("PANEL:INFO:%s: disp_det_state:%s\n",
			__func__, disp_det_state == PANEL_STATE_OK ? "OK" : "NOK");

	switch (state->cur_state) {
	case PANEL_STATE_ALPM:
	case PANEL_STATE_NORMAL:
		if (disp_det_state == PANEL_STATE_NOK) {
			disable_irq(pad->irq_disp_det);
			/* delay for disp_det deboundce */
			usleep_range(10000, 11000);

			panel_err("PANEL:ERR:%s:disp_det is abnormal state\n",
				__func__);
			ret = panel_error_cb(panel);
			if (ret)
				panel_err("PANEL:ERR:%s:failed to recover panel\n",
						__func__);
			clear_disp_det_pend(panel);
			enable_irq(pad->irq_disp_det);
			}
		break;
	default:
		break;
	}

	return;
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
			"%d", panel->dim_flash_work.ret);
	set_dpui_field(DPUI_KEY_PNGFLS, tbuf, size);
#endif

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
	panel->state.ub_connected = true;
	panel->state.cur_state = PANEL_STATE_OFF;
	panel->state.power = PANEL_POWER_OFF;
	panel->state.disp_on = PANEL_DISPLAY_OFF;
	panel->ktime_panel_on = ktime_get();
#ifdef CONFIG_SUPPORT_HMD
	panel->state.hmd_on = PANEL_HMD_OFF;
#endif
#if 0
#ifdef CONFIG_SUPPORT_DSU
	panel->lcd_info.mres_mode = DSU_MODE_1;
#endif
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

	panel_init_v4l2_subdev(panel);

	platform_set_drvdata(pdev, panel);

	INIT_WORK(&panel->disp_det_work, disp_det_handler);

	panel->disp_det_workqueue = create_singlethread_workqueue("disp_det");
	if (panel->disp_det_workqueue == NULL) {
		panel_err("PANEL:ERR:%s:failed to create workqueue for disp det\n", __func__);
		ret = -ENOMEM;
		goto probe_err;
	}

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
#ifdef CONFIG_SUPPORT_DIM_FLASH
	INIT_DELAYED_WORK(&panel->dim_flash_work.dwork, panel_update_dim_flash_work);
	panel->dim_flash_work.wq = create_singlethread_workqueue("dim_flash");
	if (panel->dim_flash_work.wq == NULL) {
		panel_err("PANEL:ERR:%s:failed to create workqueue for dim_flash\n", __func__);
		return -EINVAL;
	}
	atomic_set(&panel->dim_flash_work.running, 0);
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

static int __init panel_drv_init (void)
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
