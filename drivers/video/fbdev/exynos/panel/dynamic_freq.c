// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "panel.h"
#include "panel_drv.h"
#include "dynamic_freq.h"
#include "../dpu20/panels/decon_lcd.h"

#include <linux/dev_ril_bridge.h>


static int search_dynamic_freq_idx(struct panel_device *panel, int band_idx, int freq)
{
	int i, ret = 0;
	int min, max, array_idx;
	struct df_freq_tbl_info *df_tbl;
	struct dynamic_freq_range *array;

	if (band_idx >= FREQ_RANGE_MAX) {
		panel_err("[DYN_FREQ]:ERR:%s: exceed max band idx : %d\n", __func__, band_idx);
		ret = -1;
		goto search_exit;
	}

	df_tbl = &panel->df_freq_tbl[band_idx];
	if (df_tbl == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:failed to find band_idx : %d\n", __func__, band_idx);
		ret = -1;
		goto search_exit;
	}
	array_idx = df_tbl->size;

	if (array_idx == 1) {
		array = &df_tbl->array[0];
		panel_info("[DYN_FREQ]:INFO:%s:Found adap_freq idx(0) : %d\n",
			__func__, array->freq_idx);
		ret = array->freq_idx;
	} else {
		for (i = 0; i < array_idx; i++) {
			array = &df_tbl->array[i];
			panel_info("min : %d, max : %d\n", array->min, array->max);

			min = (int)freq - array->min;
			max = (int)freq - array->max;

			if ((min >= 0) && (max <= 0)) {
				panel_info("[DYN_FREQ]:INFO:%s:Found adap_freq idx : %d\n",
					__func__, array->freq_idx);
				ret = array->freq_idx;
				break;
			}
		}

		if (i >= array_idx) {
			panel_err("[DYN_FREQ]:ERR:%s:Can't found freq idx\n", __func__);
			ret = -1;
			goto search_exit;
		}
	}
search_exit:
	return ret;
}


int set_dynamic_freq_ffc(struct panel_device *panel)
{
	int ret = 0;
	struct panel_state *state = &panel->state;
	struct df_status_info *status = &panel->df_status;

	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", __func__);
		return -ENODEV;
	}

	if (state->cur_state == PANEL_STATE_OFF ||
		state->cur_state == PANEL_STATE_ON || !IS_PANEL_ACTIVE(panel))
		return 0;

	mutex_lock(&panel->op_lock);

	if (status->target_df != status->ffc_df) {

		status->ffc_df = status->target_df;

		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_DYNAMIC_FFC_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("[DYN_FREQ]:ERR:%s:failed to set PANEL_FFC_SEQ\n", __func__);
			goto exit_changed;
		}
	}

exit_changed:
	mutex_unlock(&panel->op_lock);
	return ret;
}


int dynamic_freq_update(struct panel_device *panel, int idx)
{
	struct df_setting_info *df_setting;
	struct df_status_info *status = &panel->df_status;

	if (idx >= panel->lcd_info.df_set_info.df_cnt) {
		panel_err("[DYN_FREQ]:ERR:%s:invalid idx : %d\n", __func__, idx);
		return -EINVAL;
	}
	df_setting = &panel->lcd_info.df_set_info.setting_info[idx];
	panel_info("[DYN_FREQ]:INFO:%s:IDX : %d Setting HS : %d\n",
		__func__, idx, df_setting->hs);

	status->request_df = idx;
	status->context = DF_CONTEXT_RIL;

	return 0;
}


static int df_notifier(struct notifier_block *self, unsigned long size, void *buf)
{
	int df_idx;
	struct panel_device *panel;
	struct dev_ril_bridge_msg *msg;
	struct ril_noti_info *ch_info;
	struct df_status_info *dyn_status;

	panel = container_of(self, struct panel_device, df_noti);
	if (panel == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:panel is null\n", __func__);
		goto exit_notifier;
	}

	dyn_status = &panel->df_status;
	if (dyn_status == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:dymanic status is null\n", __func__);
		goto exit_notifier;
	};

	if (!dyn_status->enabled) {
		panel_err("[DYN_FREQ]:ERR:%s:df is disabled\n", __func__);
		goto exit_notifier;
	}

	msg = (struct dev_ril_bridge_msg *)buf;
	if (msg == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:msg is null\n", __func__);
		goto exit_notifier;
	}

	if (msg->dev_id == IPC_SYSTEM_CP_CHANNEL_INFO &&
		msg->data_len == sizeof(struct ril_noti_info)) {

		ch_info = (struct ril_noti_info *)msg->data;
		if (ch_info == NULL) {
			panel_err("[DYN_FREQ]:ERR:%s:ch_info is null\n", __func__);
			goto exit_notifier;
		}

		panel_info("[DYN_FREQ]:INFO:%s: (b:%d, c:%d)\n",
			__func__, ch_info->band, ch_info->channel);

		df_idx = search_dynamic_freq_idx(panel, ch_info->band, ch_info->channel);
		if (df_idx < 0) {
			panel_info("[DYN_FREQ]:ERR:%s:failed to search freq idx\n", __func__);
			goto exit_notifier;
		}

		if (df_idx != dyn_status->current_df)
			dynamic_freq_update(panel, df_idx);

	}
exit_notifier:
	return 0;
}



static int init_dynamic_freq_status(struct panel_device *panel)
{
	int ret = 0;
	int cur_idx;
	struct df_status_info *status;
	struct df_setting_info *tune;

	status = &panel->df_status;
	if (status == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:dynamic status is null\n", __func__);
		ret = -EINVAL;
		goto init_exit;
	}
	cur_idx = panel->lcd_info.df_set_info.dft_index;
	tune = &panel->lcd_info.df_set_info.setting_info[cur_idx];

	status->enabled = true;

	status->request_df = panel->lcd_info.df_set_info.dft_index;

	status->current_df = MAX_DYNAMIC_FREQ;
	status->target_df = MAX_DYNAMIC_FREQ;
	status->ffc_df = MAX_DYNAMIC_FREQ;

init_exit:
	return ret;
}


static int parse_dynamic_freq(struct panel_device *panel)
{
	int ret = 0, i, cnt = 0;
	unsigned int dft_hs = 0;
	struct device_node *freq_node;
	struct df_setting_info *set_info;
	struct device *dev = panel->dev;
	struct device_node *node = panel->ddi_node;
	struct decon_lcd *lcd_info = &panel->lcd_info;
	struct df_dt_info *df = &lcd_info->df_set_info;

	if (node == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:ddi node is NULL\n", __func__);
		node = of_parse_phandle(dev->of_node, "ddi_info", 0);
	}

	cnt = of_property_count_u32_elems(node, "dynamic_freq");
	if (cnt  <= 0) {
		panel_warn("[DYN_FREQ]:WARN:%s:Can't found dynamic freq info\n", __func__);
		return -EINVAL;
	}

	if (cnt > MAX_DYNAMIC_FREQ) {
		panel_info("[DYN_FREQ]:ERR:%s:freq cnt exceed max freq num (%d:%d)\n",
			__func__, cnt, MAX_DYNAMIC_FREQ);
		cnt = MAX_DYNAMIC_FREQ;
	}
	df->df_cnt = cnt;

	of_property_read_u32(node, "timing,dsi-hs-clk", &dft_hs);
	panel_info("[DYN_FREQ]:INFO:%s default hs clock(%d)\n", __func__, dft_hs);
	panel_info("[DYN_FREQ]:INFO:%s:FREQ CNT : %d\n", __func__, cnt);

	for (i = 0; i < cnt; i++) {
		freq_node = of_parse_phandle(node, "dynamic_freq", i);
		set_info = &df->setting_info[i];

		of_property_read_u32(freq_node, "hs-clk", &set_info->hs);
		if (dft_hs == set_info->hs) {
			df->dft_index = i;
			panel_info("[DYN_FREQ]:INFO:%s:found default hs idx  : %d\n",
				__func__, df->dft_index);
		}

		of_property_read_u32_array(freq_node, "cmd_underrun_lp_ref",
				set_info->cmd_underrun_lp_ref,
				lcd_info->dt_lcd_mres.mres_number);

		of_property_read_u32_array(freq_node, "pmsk", (u32 *)&set_info->dphy_pms,
			sizeof(struct stdphy_pms)/sizeof(unsigned int));

		panel_info("[DYN_FREQ]:INFO:HS_FREQ : %d\n", set_info->hs);
		panel_info("[DYN_FREQ]:INFO:PMS[p] : %d\n", set_info->dphy_pms.p);
		panel_info("[DYN_FREQ]:INFO:PMS[m] : %d\n", set_info->dphy_pms.m);
		panel_info("[DYN_FREQ]:INFO:PMS[s] : %d\n", set_info->dphy_pms.s);
		panel_info("[DYN_FREQ]:INFO:PMS[k] : %d\n", set_info->dphy_pms.k);
	}

	return ret;
}


int dynamic_freq_probe(struct panel_device *panel, struct df_freq_tbl_info *freq_tbl)
{
	int ret = 0;

	if (freq_tbl == NULL) {
		panel_err("[DYN_FREQ]:ERR:%s:frequence set is null", __func__);
		panel_err("[DYN_FREQ]:ERR:%s:can't support DF\n", __func__);
		goto exit_probe;
	}

	ret = parse_dynamic_freq(panel);
	if (ret) {
		panel_err("[DYN_FREQ]:ERR:%s:faied to parse df\n", __func__);
		goto exit_probe;
	}
	panel->df_freq_tbl = freq_tbl;

	panel->df_noti.notifier_call = df_notifier;
	register_dev_ril_bridge_event_notifier(&panel->df_noti);

	init_dynamic_freq_status(panel);

exit_probe:
	return ret;
}
