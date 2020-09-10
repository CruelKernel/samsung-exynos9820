// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of_gpio.h>
#include "panel.h"
#include "panel_drv.h"
#include "copr.h"

static struct copr_reg_info copr_reg_v0_list[] = {
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v0, copr_gamma) },
	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v0, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v0, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v0, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v0, copr_eb) },
	{ .name = "copr_roi_on=", .offset = offsetof(struct copr_reg_v0, roi_on) },
	{ .name = "copr_roi_x_s=", .offset = offsetof(struct copr_reg_v0, roi_xs) },
	{ .name = "copr_roi_y_s=", .offset = offsetof(struct copr_reg_v0, roi_ys) },
	{ .name = "copr_roi_x_e=", .offset = offsetof(struct copr_reg_v0, roi_xe) },
	{ .name = "copr_roi_y_e=", .offset = offsetof(struct copr_reg_v0, roi_ye) },
};

static struct copr_reg_info copr_reg_v1_list[] = {
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v1, cnt_re) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v1, copr_gamma) },
	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v1, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v1, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v1, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v1, copr_eb) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v1, max_cnt) },
	{ .name = "copr_roi_on=", .offset = offsetof(struct copr_reg_v1, roi_on) },
	{ .name = "copr_roi_x_s=", .offset = offsetof(struct copr_reg_v1, roi_xs) },
	{ .name = "copr_roi_y_s=", .offset = offsetof(struct copr_reg_v1, roi_ys) },
	{ .name = "copr_roi_x_e=", .offset = offsetof(struct copr_reg_v1, roi_xe) },
	{ .name = "copr_roi_y_e=", .offset = offsetof(struct copr_reg_v1, roi_ye) },
};

static struct copr_reg_info copr_reg_v2_list[] = {
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v2, cnt_re) },
	{ .name = "copr_ilc=", .offset = offsetof(struct copr_reg_v2, copr_ilc) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v2, copr_gamma) },

	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v2, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v2, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v2, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v2, copr_eb) },
	{ .name = "copr_erc=", .offset = offsetof(struct copr_reg_v2, copr_erc) },
	{ .name = "copr_egc=", .offset = offsetof(struct copr_reg_v2, copr_egc) },
	{ .name = "copr_ebc=", .offset = offsetof(struct copr_reg_v2, copr_ebc) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v2, max_cnt) },
	{ .name = "copr_roi_on=", .offset = offsetof(struct copr_reg_v2, roi_on) },
	{ .name = "copr_roi_x_s=", .offset = offsetof(struct copr_reg_v2, roi_xs) },
	{ .name = "copr_roi_y_s=", .offset = offsetof(struct copr_reg_v2, roi_ys) },
	{ .name = "copr_roi_x_e=", .offset = offsetof(struct copr_reg_v2, roi_xe) },
	{ .name = "copr_roi_y_e=", .offset = offsetof(struct copr_reg_v2, roi_ye) },
};

static struct copr_reg_info copr_reg_v3_list[] = {
	{ .name = "copr_mask=", .offset = offsetof(struct copr_reg_v3, copr_mask) },
	{ .name = "copr_cnt_re=", .offset = offsetof(struct copr_reg_v3, cnt_re) },
	{ .name = "copr_ilc=", .offset = offsetof(struct copr_reg_v3, copr_ilc) },
	{ .name = "copr_gamma=", .offset = offsetof(struct copr_reg_v3, copr_gamma) },

	{ .name = "copr_en=", .offset = offsetof(struct copr_reg_v3, copr_en) },
	{ .name = "copr_er=", .offset = offsetof(struct copr_reg_v3, copr_er) },
	{ .name = "copr_eg=", .offset = offsetof(struct copr_reg_v3, copr_eg) },
	{ .name = "copr_eb=", .offset = offsetof(struct copr_reg_v3, copr_eb) },
	{ .name = "copr_erc=", .offset = offsetof(struct copr_reg_v3, copr_erc) },
	{ .name = "copr_egc=", .offset = offsetof(struct copr_reg_v3, copr_egc) },
	{ .name = "copr_ebc=", .offset = offsetof(struct copr_reg_v3, copr_ebc) },
	{ .name = "copr_max_cnt=", .offset = offsetof(struct copr_reg_v3, max_cnt) },
	{ .name = "copr_roi_ctrl=", .offset = offsetof(struct copr_reg_v3, roi_on) },
	/* ROI1 */
	{ .name = "copr_roi1_x_s=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_xs) },
	{ .name = "copr_roi1_y_s=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_ys) },
	{ .name = "copr_roi1_x_e=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_xe) },
	{ .name = "copr_roi1_y_e=", .offset = offsetof(struct copr_reg_v3, roi[0].roi_ye) },
	/* ROI2 */
	{ .name = "copr_roi2_x_s=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_xs) },
	{ .name = "copr_roi2_y_s=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_ys) },
	{ .name = "copr_roi2_x_e=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_xe) },
	{ .name = "copr_roi2_y_e=", .offset = offsetof(struct copr_reg_v3, roi[1].roi_ye) },
	/* ROI3 */
	{ .name = "copr_roi3_x_s=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_xs) },
	{ .name = "copr_roi3_y_s=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_ys) },
	{ .name = "copr_roi3_x_e=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_xe) },
	{ .name = "copr_roi3_y_e=", .offset = offsetof(struct copr_reg_v3, roi[2].roi_ye) },
	/* ROI4 */
	{ .name = "copr_roi4_x_s=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_xs) },
	{ .name = "copr_roi4_y_s=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_ys) },
	{ .name = "copr_roi4_x_e=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_xe) },
	{ .name = "copr_roi4_y_e=", .offset = offsetof(struct copr_reg_v3, roi[3].roi_ye) },
	/* ROI5 */
	{ .name = "copr_roi5_x_s=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_xs) },
	{ .name = "copr_roi5_y_s=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_ys) },
	{ .name = "copr_roi5_x_e=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_xe) },
	{ .name = "copr_roi5_y_e=", .offset = offsetof(struct copr_reg_v3, roi[4].roi_ye) },
	/* ROI6 */
	{ .name = "copr_roi6_x_s=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_xs) },
	{ .name = "copr_roi6_y_s=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_ys) },
	{ .name = "copr_roi6_x_e=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_xe) },
	{ .name = "copr_roi6_y_e=", .offset = offsetof(struct copr_reg_v3, roi[5].roi_ye) },
};

int get_copr_reg_size(int version)
{
	if (version == COPR_VER_0)
		return ARRAY_SIZE(copr_reg_v0_list);
	else if (version == COPR_VER_1)
		return ARRAY_SIZE(copr_reg_v1_list);
	else if (version == COPR_VER_2)
		return ARRAY_SIZE(copr_reg_v2_list);
	else if (version == COPR_VER_3)
		return ARRAY_SIZE(copr_reg_v3_list);
	else
		return 0;
}

const char *get_copr_reg_name(int version, int index)
{
	if (version == COPR_VER_0)
		return copr_reg_v0_list[index].name;
	else if (version == COPR_VER_1)
		return copr_reg_v1_list[index].name;
	else if (version == COPR_VER_2)
		return copr_reg_v2_list[index].name;
	else if (version == COPR_VER_3)
		return copr_reg_v3_list[index].name;
	else
		return NULL;
}

int get_copr_reg_offset(int version, int index)
{
	if (version == COPR_VER_0)
		return copr_reg_v0_list[index].offset;
	else if (version == COPR_VER_1)
		return copr_reg_v1_list[index].offset;
	else if (version == COPR_VER_2)
		return copr_reg_v2_list[index].offset;
	else if (version == COPR_VER_3)
		return copr_reg_v3_list[index].offset;
	else
		return -EINVAL;
}

u32 *get_copr_reg_ptr(struct copr_reg *reg, int version, int index)
{
	int offset = get_copr_reg_offset(version, index);

	if (offset < 0)
		return NULL;

	if (version == COPR_VER_0)
		return (u32 *)((void *)&reg->v0 + offset);
	else if (version == COPR_VER_1)
		return (u32 *)((void *)&reg->v1 + offset);
	else if (version == COPR_VER_2)
		return (u32 *)((void *)&reg->v2 + offset);
	else if (version == COPR_VER_3)
		return (u32 *)((void *)&reg->v3 + offset);
	else
		return NULL;
}

int find_copr_reg_by_name(int version, char *s)
{
	int i;
	const char *name;

	if (s == NULL)
		return -EINVAL;

	for (i = 0; i < get_copr_reg_size(version); i++) {
		name = get_copr_reg_name(version, i);
		if (name == NULL)
			continue;

		if (!strncmp(name, s, strlen(name)))
			return i;
	}

	return -EINVAL;
}

ssize_t copr_reg_show(struct copr_info *copr, char *buf)
{
	int i, len = 0, version, size;
	const char *name;
	u32 *ptr;

	version = copr->props.version;
	size = get_copr_reg_size(version);
	for (i = 0; i < size; i++) {
		name = get_copr_reg_name(version, i);
		ptr = get_copr_reg_ptr(&copr->props.reg, version, i);
		if (name != NULL && ptr != NULL)
			len += snprintf(buf + len, PAGE_SIZE - len, "%s%d\n", name, *ptr);
	}

	return len;
}

int copr_reg_store(struct copr_info *copr, int index, u32 value)
{
	int version, size;
	const char *name;
	u32 *ptr;

	version = copr->props.version;
	size = get_copr_reg_size(version);

	if (index >= size)
		return -EINVAL;

	name = get_copr_reg_name(version, index);
	ptr = get_copr_reg_ptr(&copr->props.reg, version, index);
	if (name != NULL && ptr != NULL)
		*ptr = value;
	else
		return -EINVAL;

	return 0;
}

static int panel_do_copr_seqtbl_by_index(struct copr_info *copr, int index)
{
	struct panel_device *panel = to_panel_device(copr);
	struct seqinfo *tbl;
	int ret;

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_warn("WARN:PANEL:%s:panel inactive state\n", __func__);
		return -EINVAL;
	}

	tbl = panel->copr.seqtbl;
	mutex_lock(&panel->op_lock);
	if (unlikely(index < 0 || index >= MAX_COPR_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		ret = -EINVAL;
		goto do_exit;
	}

#ifdef DEBUG_PANEL
	pr_info("%s, %s start\n", __func__, tbl[index].name);
#endif

	ret = panel_do_seqtbl(panel, &tbl[index]);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl->name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:
	mutex_unlock(&panel->op_lock);
#ifdef DEBUG_PANEL
	pr_info("%s, %s end\n", __func__, tbl[index].name);
#endif
	return 0;
}

static int panel_set_copr(struct copr_info *copr)
{
	int ret;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_SET_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to do seqtbl\n", __func__);
		return -EIO;
	}

	msleep(34);
	if (get_copr_reg_copr_en(copr))
		copr->props.state = COPR_REG_ON;
	else
		copr->props.state = COPR_REG_OFF;

	return 0;
}

#ifdef CONFIG_SUPPORT_COPR_AVG
static int panel_clear_copr(struct copr_info *copr)
{
	int ret = 0;

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_CLR_CNT_ON_SEQ);
	if (unlikely(ret < 0))
		pr_err("%s, failed to do seqtbl\n", __func__);

	msleep(34);

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_CLR_CNT_OFF_SEQ);
	if (unlikely(ret < 0))
		pr_err("%s, failed to do seqtbl\n", __func__);

	msleep(34);

	pr_debug("%s copr clear seq\n", __func__);

	return ret;
}
#endif

#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
static int panel_read_copr_spi(struct copr_info *copr)
{
	u8 *buf = NULL;
	int i, c, index, ret, size;
	struct panel_device *panel = to_panel_device(copr);
	struct panel_info *panel_data;
	struct copr_properties *props = &copr->props;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -ENODEV;
	}
	panel_data = &panel->panel_data;

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_SPI_GET_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to do seqtbl\n", __func__);
		ret = -EIO;
		goto get_copr_error;
	}

	size = get_resource_size_by_name(panel_data, "copr_spi");
	if (size < 0) {
		pr_err("%s, failed to get copr size (ret %d)\n", __func__, size);
		ret = -EINVAL;
		goto get_copr_error;
	}

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto get_copr_error;
	}

	ret = resource_copy_by_name(panel_data, (u8 *)buf, "copr_spi");
	if (ret < 0) {
		pr_err("%s, failed to get copr (ret %d)\n", __func__, ret);
		ret = -EIO;
		goto get_copr_error;
	}

	if (props->version == COPR_VER_3) {
		props->copr_ready = buf[0] & 0x01;
		props->cur_cnt = (buf[1] << 8) | buf[2];
		props->cur_copr = (buf[3] << 8) | buf[4];
		props->avg_copr = (buf[5] << 8) | buf[6];
		props->s_cur_cnt = (buf[7] << 8) | buf[8];
		props->s_avg_copr = (buf[9] << 8) | buf[10];
		for (i = 0; i < 6; i++) {
			for (c = 0; c < 3; c++) {
				index = 11 + i * 6 + c * 2;
				props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
			}
			pr_debug("copr_spi: copr_roi_r[%d] %d %d %d\n",
					i, props->copr_roi_r[i][RED],
					props->copr_roi_r[i][GREEN],
					props->copr_roi_r[i][BLUE]);
		}
		pr_debug("copr_spi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready, props->comp_copr);
	} else if (props->version == COPR_VER_2) {
		props->copr_ready = ((buf[0] & 0x80) ? 1 : 0);
		props->cur_cnt = ((buf[0] & 0x7F) << 9) |
			(buf[1] << 1) | ((buf[2] & 0x80) ? 1 : 0);
		props->cur_copr = ((buf[2] & 0x7F) << 2) | ((buf[3] & 0xC0) >> 6);
		props->avg_copr = ((buf[3] & 0x3F) << 3) | ((buf[4] & 0xE0) >> 5);
		props->s_cur_cnt = ((buf[4] & 0x1F) << 11) |
			(buf[5] << 3) | ((buf[6] & 0xE0) >> 5);
		props->s_avg_copr = ((buf[6] & 0x1F) << 4) | ((buf[7] & 0xF0) >> 4);
		props->comp_copr = ((buf[7] & 0x0F) << 5) | ((buf[8] & 0xF8) >> 3);
		pr_debug("copr_spi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready, props->comp_copr);
	} else if (props->version == COPR_VER_1) {
		props->copr_ready = ((buf[0] & 0x80) ? 1 : 0);
		props->cur_cnt = ((buf[0] & 0x7F) << 9) |
			(buf[1] << 1) | ((buf[2] & 0x80) ? 1 : 0);
		props->cur_copr = ((buf[2] & 0x7F) << 2) | ((buf[3] & 0xC0) >> 6);
		props->avg_copr = ((buf[3] & 0x3F) << 3) | ((buf[4] & 0xE0) >> 5);
		props->s_cur_cnt = ((buf[4] & 0x1F) << 11) |
			(buf[5] << 3) | ((buf[6] & 0xE0) >> 5);
		props->s_avg_copr = ((buf[6] & 0x1F) << 4) | ((buf[7] & 0xF0) >> 4);
		pr_debug("copr_spi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready);
	} else if (props->version == COPR_VER_0) {
		props->cur_copr = buf[0];
	}

get_copr_error:
	kfree(buf);

	return ret;
}
#else
static int panel_read_copr_dsi(struct copr_info *copr)
{
	u8 *buf = NULL;
	int i, c, index, ret, size;
	struct panel_device *panel = to_panel_device(copr);
	struct panel_info *panel_data;
	struct copr_properties *props = &copr->props;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -ENODEV;
	}
	panel_data = &panel->panel_data;

	ret = panel_do_copr_seqtbl_by_index(copr, COPR_DSI_GET_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to do seqtbl\n", __func__);
		ret = -EIO;
		goto get_copr_error;
	}

	size = get_resource_size_by_name(panel_data, "copr_dsi");
	if (size < 0) {
		pr_err("%s, failed to get copr size (ret %d)\n", __func__, size);
		ret = -EINVAL;
		goto get_copr_error;
	}

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf) {
		ret = -ENOMEM;
		goto get_copr_error;
	}

	ret = resource_copy_by_name(panel_data, (u8 *)buf, "copr_dsi");
	if (ret < 0) {
		pr_err("%s, failed to get copr (ret %d)\n", __func__, ret);
		ret = -EIO;
		goto get_copr_error;
	}

	if (props->version == COPR_VER_3) {
		props->copr_ready = buf[0] & 0x01;
		props->cur_cnt = (buf[1] << 8) | buf[2];
		props->cur_copr = (buf[3] << 8) | buf[4];
		props->avg_copr = (buf[5] << 8) | buf[6];
		props->s_cur_cnt = (buf[7] << 8) | buf[8];
		props->s_avg_copr = (buf[9] << 8) | buf[10];
		for (i = 0; i < 5; i++) {
			for (c = 0; c < MAX_COLOR; c++) {
				index = 11 + i * (MAX_COLOR * 2) + c * 2;
				props->copr_roi_r[i][c] = (buf[index] << 8) | buf[index + 1];
			}
			pr_debug("copr_dsi: copr_roi_r[%d] %d %d %d\n",
					i, props->copr_roi_r[i][RED],
					props->copr_roi_r[i][GREEN],
					props->copr_roi_r[i][BLUE]);
		}
		pr_debug("copr_dsi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready);
	} else if (props->version == COPR_VER_2) {
		props->copr_ready = ((buf[10] & 0x80) ? 1 : 0);
		props->cur_cnt = (buf[0] << 8) | buf[1];
		props->cur_copr = (buf[2] << 8) | buf[3];
		props->avg_copr = (buf[4] << 8) | buf[5];
		props->s_cur_cnt = (buf[6] << 8) | buf[7];
		props->s_avg_copr = (buf[8] << 8) | buf[9];
		props->comp_copr = (((buf[10] & 0x01) ? 1 : 0) << 8) | buf[11];
		pr_debug("copr_dsi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d, comp_copr %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready, props->comp_copr);
	} else if (props->version == COPR_VER_1) {
		props->copr_ready = ((buf[8] & 0x80) ? 1 : 0);
		props->cur_cnt = (buf[0] << 8) | buf[1];
		props->cur_copr = (buf[2] << 8) | buf[3];
		props->avg_copr = (buf[4] << 8) | buf[5];
		props->s_cur_cnt = (buf[6] << 8) | buf[7];
		props->s_avg_copr = (buf[8] << 8) | buf[9];
		pr_debug("copr_dsi: cur_cnt %d, cur_copr %d, avg_copr %d, s_cur_cnt %d, s_avg_copr %d, copr_ready %d\n",
				props->cur_cnt, props->cur_copr, props->avg_copr,
				props->s_cur_cnt, props->s_avg_copr, props->copr_ready);
	} else {
		props->cur_copr = buf[0];
	}

get_copr_error:
	kfree(buf);

	return ret;
}
#endif

static int panel_get_copr(struct copr_info *copr)
{
	struct timespec cur_ts, last_ts, delta_ts;
	struct copr_properties *props = &copr->props;
	s64 elapsed_usec;
	int ret;

	if (unlikely(!props->support))
		return -ENODEV;

	ktime_get_ts(&cur_ts);
	if (props->state != COPR_REG_ON) {
		pr_debug("%s copr reg is not on state %d\n",
				__func__, props->state);
		ret = -EINVAL;
		goto get_copr_error;
	}

	if (atomic_read(&copr->stop)) {
		panel_warn("%s copr_stop\n", __func__);
		ret = -EINVAL;
		goto get_copr_error;
	}

#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
	panel_read_copr_spi(copr);
#else
	panel_read_copr_dsi(copr);
#endif

	ktime_get_ts(&last_ts);

	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_debug("%s elapsed_usec %lld usec (%lld.%lld %lld.%lld)\n",
			__func__, elapsed_usec,
			timespec_to_ns(&cur_ts) / 1000000000,
			(timespec_to_ns(&cur_ts) % 1000000000) / 1000,
			timespec_to_ns(&last_ts) / 1000000000,
			(timespec_to_ns(&last_ts) % 1000000000) / 1000);

	return 0;

get_copr_error:
	return ret;
}

bool copr_is_enabled(struct copr_info *copr)
{
	return (copr->props.support && get_copr_reg_copr_en(copr) && copr->props.enable);
}

static int copr_res_init(struct copr_info *copr)
{
	timenval_init(&copr->res);

	return 0;
}

static int copr_res_start(struct copr_info *copr, int value, struct timespec cur_ts)
{
	timenval_start(&copr->res, value, cur_ts);

	return 0;
}

int copr_update_start(struct copr_info *copr, int count)
{
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (atomic_read(&copr->wq.count) < count)
		atomic_set(&copr->wq.count, count);
	wake_up_interruptible_all(&copr->wq.wait);

	return 0;
}

int copr_update_average(struct copr_info *copr)
{
	struct copr_properties *props = &copr->props;
	struct timespec cur_ts;
	int ret;
	int cur_copr;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		return -EIO;
	}

	ktime_get_ts(&cur_ts);
	if (props->state == COPR_UNINITIALIZED) {
		panel_set_copr(copr);
		panel_info("%s copr register updated\n", __func__);
	}

	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("%s failed to get copr (ret %d)\n",
				__func__, ret);
		return -EINVAL;
	}

	if (copr->props.version == COPR_VER_3 ||
		copr->props.version == COPR_VER_2) {
#ifdef CONFIG_SUPPORT_COPR_AVG
		ret = panel_clear_copr(copr);
		if (unlikely(ret < 0))
			pr_err("%s, failed to reset copr\n", __func__);
		cur_copr = props->avg_copr;
		timenval_update_average(&copr->res, cur_copr, cur_ts);
#else
		cur_copr = props->cur_copr;
		timenval_update_snapshot(&copr->res, cur_copr, cur_ts);
#endif
	} else {
		cur_copr = props->cur_copr;
		timenval_update_snapshot(&copr->res, cur_copr, cur_ts);
	}

	return 0;
}

int copr_get_value(struct copr_info *copr)
{
	struct copr_properties *props = &copr->props;
	int ret;
	int cur_copr;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		mutex_unlock(&copr->lock);
		return -EIO;
	}

	if (props->state == COPR_UNINITIALIZED) {
		panel_set_copr(copr);
		panel_info("%s copr register updated\n", __func__);
	}

	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("%s failed to get copr (ret %d)\n",
				__func__, ret);
		mutex_unlock(&copr->lock);
		return -EINVAL;
	}
	cur_copr = props->cur_copr;

	mutex_unlock(&copr->lock);

	return cur_copr;
}

int copr_iter_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out)
{
	struct copr_properties *props = &copr->props;
	struct timespec cur_ts;
	int ret;
	int i, c, cur_copr;
	struct copr_reg reg;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	/* update using last value or avg_copr */
	mutex_lock(&copr->lock);
	ret = copr_update_average(copr);
	if (ret < 0) {
		panel_err("%s failed to update average(ret %d)\n",
				__func__, ret);
		mutex_unlock(&copr->lock);
		return ret;
	}

	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		mutex_unlock(&copr->lock);
		return -EIO;
	}

	pr_debug("%s set roi\n", __func__);
	memcpy(&reg, &copr->props.reg, sizeof(reg));
	SET_COPR_REG_GAMMA(copr, false);
	if (copr->props.version == COPR_VER_2) {
		for (i = 0; i < size; i++) {
			SET_COPR_REG_ROI(copr, &roi[i], 1);
			SET_COPR_REG_E(copr, 0x300, 0, 0);
			SET_COPR_REG_EC(copr, 0, 0x300, 0);
			panel_set_copr(copr);
			ret = panel_get_copr(copr);
			if (ret < 0) {
				panel_err("%s failed to get copr (ret %d)\n",
						__func__, ret);
				/* restore r/g/b efficiency & roi */
				memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
				mutex_unlock(&copr->lock);
				return -EINVAL;
			}
			out[i * 3 + 0] = props->cur_copr;
			out[i * 3 + 1] = props->comp_copr;

			SET_COPR_REG_E(copr, 0, 0, 0x300);
			SET_COPR_REG_EC(copr, 0, 0, 0);
			panel_set_copr(copr);
			ret = panel_get_copr(copr);
			if (ret < 0) {
				panel_err("%s failed to get copr (ret %d)\n",
						__func__, ret);
				/* restore r/g/b efficiency & roi */
				memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
				mutex_unlock(&copr->lock);
				return -EINVAL;
			}
			out[i * 3 + 2] = props->cur_copr;
		}
	} else if (copr->props.version == COPR_VER_1) {
		for (i = 0; i < size; i++) {
			for (c = 0; c < 3; c++) {
				SET_COPR_REG_ROI(copr, &roi[i], 1);
				if (c == 0)
					SET_COPR_REG_E(copr, 0x300, 0, 0);
				else if (c == 1)
					SET_COPR_REG_E(copr, 0, 0x300, 0);
				else if (c == 2)
					SET_COPR_REG_E(copr, 0, 0, 0x300);

				panel_set_copr(copr);
				ret = panel_get_copr(copr);
				if (ret < 0) {
					panel_err("%s failed to get copr (ret %d)\n",
							__func__, ret);
					/* restore r/g/b efficiency & roi */
					memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
					mutex_unlock(&copr->lock);
					return -EINVAL;
				}
				out[i * 3 + c] = props->cur_copr;
			}
		}
	}

	/* restore r/g/b efficiency & roi */
	memcpy(&copr->props.reg, &reg, sizeof(copr->props.reg));
#ifdef CONFIG_SUPPORT_COPR_AVG
	if (copr->props.version == COPR_VER_2 ||
		copr->props.version == COPR_VER_1) {
		ret = panel_do_copr_seqtbl_by_index(copr, COPR_CLR_CNT_ON_SEQ);
		if (unlikely(ret < 0))
			pr_err("%s, failed to do seqtbl\n", __func__);
		msleep(34);
	}
#endif
	panel_set_copr(copr);
	msleep(34);
	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("%s failed to get copr (ret %d)\n",
				__func__, ret);
		mutex_unlock(&copr->lock);
		return -EINVAL;
	}

	pr_debug("%s restore roi\n", __func__);

	/*
	 * exclude elapsed time of copr roi snapshot reading
	 * in copr_sum_update. so update last_ts as cur_ts.
	 */
	ktime_get_ts(&cur_ts);
	cur_copr = props->cur_copr;
	copr_res_start(copr, cur_copr, cur_ts);
	mutex_unlock(&copr->lock);

	return 0;
}

int copr_cur_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out)
{
	struct copr_properties *props = &copr->props;
	int i, c, max_size = 5, ret;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	mutex_lock(&copr->lock);
	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		mutex_unlock(&copr->lock);
		return -EIO;
	}

	if (props->state == COPR_UNINITIALIZED) {
		SET_COPR_REG_ROI(copr, roi, (int)min(size, max_size));
		panel_set_copr(copr);
		panel_info("%s copr register updated\n", __func__);
	}

	ret = panel_get_copr(copr);
	if (ret < 0) {
		panel_err("%s failed to get copr (ret %d)\n",
				__func__, ret);
		mutex_unlock(&copr->lock);
		return -EINVAL;
	}

	for (i = 0; i < (int)min(size, max_size); i++)
		for (c = 0; c < 3; c++)
			out[i * 3 + c] = props->copr_roi_r[i][c];

	mutex_unlock(&copr->lock);

	return 0;
}

int copr_roi_set_value(struct copr_info *copr, struct copr_roi *roi, int size)
{
	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr)) {
		panel_dbg("%s copr disabled\n", __func__);
		return -EIO;
	}

	mutex_lock(&copr->lock);
	SET_COPR_REG_ROI(copr, roi, (int)min(size, 6));
	panel_set_copr(copr);
	mutex_unlock(&copr->lock);

	return 0;
}

/**
 * copr_roi_get_value - get copr value of each roi
 *
 * @ copr : copr_info
 * @ roi : a pointer of roi array.
 * @ size : size of roi array.
 * @ out : a pointer roi output value to be stored.
 *
 * Get copr snapshot for each roi's r/g/b.
 * This function returns 0 if the average is valid.
 * If not this function returns -ERRNO.
 */
int copr_roi_get_value(struct copr_info *copr, struct copr_roi *roi, int size, u32 *out)
{
	if (copr->props.version == COPR_VER_3)
		return copr_cur_roi_get_value(copr, roi, size, out);
	else
		return copr_iter_roi_get_value(copr, roi, size, out);
}

int copr_clear_average(struct copr_info *copr)
{
	timenval_clear_average(&copr->res);

	return 0;
}

/**
 * copr_get_average_and_clear - get copr average.
 *
 * @ copr : copr_info
 * @ index : a index of copr result.
 *
 * Get copr average from sum and elapsed_msec.
 * And clears copr_res's sum and elapsed_msec variables.
 * This function returns 0 if the average is valid.
 * If not this function returns -ERRNO.
 */
int copr_get_average_and_clear(struct copr_info *copr)
{
	int avg;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	mutex_lock(&copr->lock);
	copr_update_average(copr);
	avg = copr->res.avg;
	copr_clear_average(copr);
	mutex_unlock(&copr->lock);

	return avg;
}

static int set_spi_gpios(struct panel_device *panel, int en)
{
	int err_num = 0;
	struct spi_device *spi = panel->spi;
	struct copr_spi_gpios *gpio_info = &panel->spi_gpio;

	if (!spi) {
		pr_debug("%s:spi is null\n", __func__);
		return 0;
	}

	panel_info("%s en : %d\n", __func__, en);
	if (en) {
		if (gpio_direction_output(gpio_info->gpio_sck, 0))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_miso))
			goto set_exit;
		err_num++;

		if (gpio_direction_output(gpio_info->gpio_mosi, 0))
			goto set_exit;
		err_num++;

		if (gpio_direction_output(gpio_info->gpio_cs, 0))
			goto set_exit;
	} else {
		if (gpio_direction_input(gpio_info->gpio_sck))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_miso))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_mosi))
			goto set_exit;
		err_num++;

		if (gpio_direction_input(gpio_info->gpio_cs))
			goto set_exit;
	}
	return 0;

set_exit:
	panel_err("%s : failed to gpio : %d\n", __func__, err_num);
	return -EIO;
}

static int get_spi_gpios_dt(struct panel_device *panel)
{
	int ret = 0;
	struct device_node *np;
	struct spi_device *spi = panel->spi;
	struct copr_spi_gpios *gpio_info = &panel->spi_gpio;

	if (!spi) {
		panel_err("%s:spi or gpio_info is null\n", __func__);
		goto error_dt;
	}

	np = spi->master->dev.of_node;
	if (!np) {
		panel_err("%s:dev_of_node is null\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_sck = of_get_named_gpio(np, "gpio-sck", 0);
	if (gpio_info->gpio_sck < 0) {
		panel_err("%s failed to get gpio_sck from dt\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_miso = of_get_named_gpio(np, "gpio-miso", 0);
	if (gpio_info->gpio_miso < 0) {
		panel_err("%s failed to get miso from dt\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_mosi = of_get_named_gpio(np, "gpio-mosi", 0);
	if (gpio_info->gpio_mosi < 0) {
		panel_err("%s failed to get mosi from dt\n", __func__);
		goto error_dt;
	}

	gpio_info->gpio_cs = of_get_named_gpio(np, "cs-gpios", 0);
	if (gpio_info->gpio_cs < 0) {
		panel_err("%s failed to get cs from dt\n", __func__);
		goto error_dt;
	}

error_dt:
	return ret;
}

int copr_enable(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	struct panel_state *state = &panel->state;
	struct copr_properties *props = &copr->props;
	int ret;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (copr_is_enabled(copr)) {
		pr_info("%s already enabled\n", __func__);
		return 0;
	}

	if (set_spi_gpios(panel, 1))
		panel_err("%s:failed to set spio gpio\n", __func__);

	pr_info("%s +\n", __func__);
	atomic_set(&copr->stop, 0);
	mutex_lock(&copr->lock);
	copr->props.enable = true;
	if (state->disp_on == PANEL_DISPLAY_ON) {
		/*
		 * TODO : check whether "copr-set" is includued in "init-seq".
		 * If COPR_SET_SEQ is included in INIT_SEQ, set state COPR_REG_ON.
		 * If not, copr state should be COPR_UNINITIALIZED.
		 */
		props->state = COPR_REG_ON;
	}
	if (copr->props.options.check_avg) {
		copr_res_init(copr);
#ifdef CONFIG_SUPPORT_COPR_AVG
		if (copr->props.version == COPR_VER_3 ||
			copr->props.version == COPR_VER_2 ||
			copr->props.version == COPR_VER_1) {
			ret = panel_clear_copr(copr);
			if (unlikely(ret < 0))
				pr_err("%s, failed to reset copr\n", __func__);
		}
#endif
		copr_update_average(copr);
	}
	mutex_unlock(&copr->lock);

	pr_info("%s -\n", __func__);

	return 0;
}

int copr_disable(struct copr_info *copr)
{
	struct panel_device *panel = to_panel_device(copr);
	struct copr_properties *props = &copr->props;

	if (unlikely(!copr->props.support))
		return -ENODEV;

	if (!copr_is_enabled(copr)) {
		pr_info("%s already disabled\n", __func__);
		return 0;
	}

	pr_info("%s +\n", __func__);
	atomic_set(&copr->stop, 1);
	mutex_lock(&copr->lock);
	if (copr->props.options.check_avg) {
		if (copr->props.version < COPR_VER_2)
			copr_update_average(copr);
	}
	if (props->enable) {
		props->enable = false;
		props->state = COPR_UNINITIALIZED;
	}
	mutex_unlock(&copr->lock);
	if (set_spi_gpios(panel, 0))
		panel_err("%s:failed to set spio gpio\n", __func__);
	pr_info("%s -\n", __func__);

	return 0;
}

static int copr_thread(void *data)
{
	struct copr_info *copr = data;
	int last_value;
	int ret;
#ifdef CONFIG_SUPPORT_COPR_AVG
	bool runnable = (copr->props.version < COPR_VER_2);
#else
	bool runnable = true;
#endif

	last_value = copr->res.last_value;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(copr->wq.wait,
				(atomic_read(&copr->wq.count) > 0) &&
				copr_is_enabled(copr) && runnable);

		if (last_value != copr->res.last_value)
			atomic_dec(&copr->wq.count);

		if (!ret) {
			mutex_lock(&copr->lock);
			copr_update_average(copr);
			last_value = copr->res.last_value;
			mutex_unlock(&copr->lock);
			usleep_range(16660, 16670);
		}
	}

	return 0;
}

static int copr_create_thread(struct copr_info *copr)
{
	if (unlikely(!copr->props.support)) {
		panel_warn("copr unsupported\n");
		return 0;
	}

	copr->wq.thread = kthread_run(copr_thread, copr, "copr-thread");
	if (IS_ERR_OR_NULL(copr->wq.thread)) {
		panel_err("%s failed to run copr thread\n", __func__);
		copr->wq.thread = NULL;
		return PTR_ERR(copr->wq.thread);
	}

	return 0;
}

static int copr_fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct copr_info *copr;
	struct fb_event *evdata = data;
	int fb_blank;
	int early_blank;

	switch (event) {
	case FB_EVENT_BLANK:
		early_blank = 0;
		break;
	case FB_EARLY_EVENT_BLANK:
		early_blank = 1;
		break;
	default:
		return 0;
	}

	copr = container_of(self, struct copr_info, fb_notif);

	fb_blank = *(int *)evdata->data;
	pr_debug("%s: %d\n", __func__, fb_blank);

	if (evdata->info->node != 0)
		return 0;
	if (unlikely(!copr->props.support))
		return 0;

	return 0;
}

static int copr_register_fb(struct copr_info *copr)
{
	memset(&copr->fb_notif, 0, sizeof(copr->fb_notif));
	copr->fb_notif.notifier_call = copr_fb_notifier_callback;
	return fb_register_client(&copr->fb_notif);
}

int copr_probe(struct panel_device *panel, struct panel_copr_data *copr_data)
{
	struct copr_info *copr;
	int i;

	if (!panel || !copr_data) {
		pr_err("%s panel(%p) or copr_data(%p) not exist\n",
				__func__, panel, copr_data);
		return -EINVAL;
	}

	copr = &panel->copr;

	atomic_set(&copr->stop, 0);
	mutex_lock(&copr->lock);
	memcpy(&copr->props.reg, &copr_data->reg, sizeof(struct copr_reg));
	copr->props.version = copr_data->version;
	memcpy(&copr->props.options, &copr_data->options, sizeof(struct copr_options));
	memcpy(&copr->props.roi, &copr_data->roi, sizeof(copr->props.roi));
	copr->props.nr_roi = copr_data->nr_roi;
	copr->seqtbl = copr_data->seqtbl;
	copr->nr_seqtbl = copr_data->nr_seqtbl;
	copr->maptbl = copr_data->maptbl;
	copr->nr_maptbl = copr_data->nr_maptbl;

	for (i = 0; i < copr->nr_maptbl; i++)
		copr->maptbl[i].pdata = copr;

	init_waitqueue_head(&copr->wq.wait);
	copr->props.support = true;
	copr_register_fb(copr);
	get_spi_gpios_dt(panel);

	for (i = 0; i < copr->nr_maptbl; i++)
		maptbl_init(&copr->maptbl[i]);

	if (IS_PANEL_ACTIVE(panel) &&
			get_copr_reg_copr_en(copr)) {
		copr->props.enable = true;
		copr->props.state = COPR_UNINITIALIZED;
		atomic_set(&copr->wq.count, 5);
		if (copr->props.options.thread_on)
			copr_create_thread(copr);
	}
	mutex_unlock(&copr->lock);
	pr_info("%s registered successfully\n", __func__);

	return 0;
}
