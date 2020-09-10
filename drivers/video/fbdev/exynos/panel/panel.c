// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <video/mipi_display.h>
#include <linux/lcd.h>
#include <linux/spi/spi.h>
//#include "../dpu/mipi_panel_drv.h"
#include "panel_drv.h"
#include "panel.h"
#include "panel_bl.h"
//#include "mdnie.h"
#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
#include "spi.h"
#endif

#ifdef CONFIG_PANEL_AID_DIMMING
#include "dimming.h"
#include "panel_dimming.h"
#endif

const char *cmd_type_name[] = {
	[CMD_TYPE_NONE] = "NONE",
	[CMD_TYPE_DELAY] = "DELAY",
	[CMD_TYPE_DELAY_NO_SLEEP] = "DELAY_NO_SLEEP",
	[CMD_TYPE_PINCTL] = "PINCTL",
	[CMD_PKT_TYPE_NONE] = "PKT_NONE",
	[SPI_PKT_TYPE_WR] = "SPI_WR",
	[DSI_PKT_TYPE_WR] = "DSI_WR",
	[DSI_PKT_TYPE_COMP] = "DSI_DSC_WR",
	[DSI_PKT_TYPE_WR_SR] = "DSI_WR_SR",
	[DSI_PKT_TYPE_WR_MEM] = "DSI_WR_MEM",
	[DSI_PKT_TYPE_PPS] = "DSI_WR_PPS",
	[SPI_PKT_TYPE_RD] = "SPI_RD",
	[DSI_PKT_TYPE_RD] = "DSI_RD",
#ifdef CONFIG_SUPPORT_DDI_FLASH
	[DSI_PKT_TYPE_RD_POC] = "DSI_RD_POC",
#endif
	[CMD_TYPE_RES] = "RES",
	[CMD_TYPE_SEQ] = "SEQ",
	[CMD_TYPE_KEY] = "KEY",
	[CMD_TYPE_MAP] = "MAP",
	[CMD_TYPE_DMP] = "DUMP",
};

void print_data(char *data, int size)
{
	char buf[256];
	int i, len = 0, sz_buf = ARRAY_SIZE(buf);
	bool newline = true;
	char *delim = "==============================================";

	if (data == NULL || size <= 0 || size > 512) {
		pr_warn("%s, invalid parameter (size %d)\n",
				__func__, size);
		return;
	}

	pr_info("%s\n", delim);
	for (i = 0; i < size; i++) {
		if (newline)
			len += snprintf(buf + len, sz_buf - len,
					"[%02Xh]  ", i);
		len += snprintf(buf + len, sz_buf - len,
				"0x%02X", data[i]);
		if (!((i + 1) % 8) || (i + 1 == size)) {
			len += snprintf(buf + len, sz_buf - len, "\n");
			newline = true;
		} else {
			len += snprintf(buf + len, sz_buf - len, " ");
			newline = false;
		}

		if (newline) {
			pr_info("%s", buf);
			len = 0;
		}
	}
	pr_info("%s\n", delim);
}

void print_maptbl(struct maptbl *tbl)
{
	char strbuf[100];
	char *space[3] = {"", "\t", "\t\t"};
	int layer, row, col, len;
	int depth = 0;

	pr_info("MAPTBL %s (%d layer, %d row, %d col)\n",
			tbl->name, tbl->nlayer, tbl->nrow, tbl->ncol);
	for_each_layer(tbl, layer) {
		for_each_row(tbl, row) {
			depth++;
			len = snprintf(strbuf, sizeof(strbuf),
					"%s[%3d] : ", space[depth], row);
			for_each_col(tbl, col) {
				len += snprintf(strbuf + len,
						max((int)sizeof(strbuf) - len, 0), "%02X ",
						tbl->arr[sizeof_layer(tbl) * layer +
						sizeof_row(tbl) * row + col]);
			}
			pr_info("%s\n", strbuf);
			depth--;
		}
		pr_info("\n");
	}
}


static int nr_panel;
static struct common_panel_info *panel_list[MAX_PANEL];
int register_common_panel(struct common_panel_info *info)
{
	int i;

	if (unlikely(!info)) {
		pr_err("%s, invalid panel_info\n", __func__);
		return -EINVAL;
	}

	if (unlikely(nr_panel >= MAX_PANEL)) {
		pr_warn("%s, exceed max seqtbl\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < nr_panel; i++) {
		if (!strcmp(panel_list[i]->name, info->name) &&
				panel_list[i]->id == info->id &&
				panel_list[i]->rev == info->rev) {
			pr_warn("%s, already exist panel (%s id-0x%06X rev-%d)\n",
					__func__, info->name, info->id, info->rev);
			return -EINVAL;
		}
	}
	panel_list[nr_panel++] = info;
	pr_info("%s, panel:%s id:0x%06X rev:%d) registered\n",
			__func__, info->name, info->id, info->rev);

	return 0;
}

static struct common_panel_info *find_common_panel_with_name(const char *name)
{
	int i;

	if (!name) {
		panel_err("%s, invalid name\n", __func__);
		return NULL;
	}

	for (i = 0; i < nr_panel; i++) {
		if (!panel_list[i]->name)
			continue;
		if (!strncmp(panel_list[i]->name, name, 128)) {
			pr_info("%s, found panel:%s id:0x%06X rev:%d\n",
					__func__, panel_list[i]->name,
					panel_list[i]->id, panel_list[i]->rev);
			return panel_list[i];
		}
	}

	return NULL;
}

void print_panel_lut(struct panel_lut_info *lut_info)
{
	int i;

	for (i = 0; i < lut_info->nr_panel; i++)
		panel_dbg("PANEL:DBG:panel_lut names[%d] %s\n", i, lut_info->names[i]);

	for (i = 0; i < lut_info->nr_lut; i++)
		panel_dbg("PANEL:DBG:panel_lut[%d] id:0x%08X mask:0x%08X index:%d(%s)\n",
				i, lut_info->lut[i].id, lut_info->lut[i].mask,
				lut_info->lut[i].index, lut_info->names[lut_info->lut[i].index]);
}

int find_panel_lut(struct panel_device *panel, u32 id)
{
	struct panel_lut_info *lut_info = &panel->panel_data.lut_info;
	int i;

	for (i = 0; i < lut_info->nr_lut; i++) {
		if ((lut_info->lut[i].id & lut_info->lut[i].mask)
				== (id & lut_info->lut[i].mask)) {
			panel_info("%s, found %s (id:0x%08X lut[%d].id:0x%08X lut[%d].mask:0x%08X)\n",
					__func__, lut_info->names[lut_info->lut[i].index],
					id, i, lut_info->lut[i].id, i, lut_info->lut[i].mask);
			return lut_info->lut[i].index;
		}
		pr_debug("%s, id:0x%08X lut[%d].id:0x%08X lut[%d].mask:0x%08X (0x%08X 0x%08X) - not same\n",
				__func__, id, i, lut_info->lut[i].id, i, lut_info->lut[i].mask,
				lut_info->lut[i].id & lut_info->lut[i].mask,
				id & lut_info->lut[i].mask);
	}

	panel_err("%s, panel not found!! (id 0x%08X)\n", __func__, id);

	return -ENODEV;
}

struct common_panel_info *find_panel(struct panel_device *panel, u32 id)
{
	struct panel_lut_info *lut_info = &panel->panel_data.lut_info;
	int index;

	index = find_panel_lut(panel, id);
	if (index < 0) {
		panel_err("%s, failed to find panel lookup table\n", __func__);
		return NULL;
	}

	if (index >= lut_info->nr_panel) {
		panel_err("%s, index %d exceeded nr_panel of lut\n", __func__, index);
		return NULL;
	}

	return find_common_panel_with_name(lut_info->names[index]);
}

int find_panel_ddi_lut(struct panel_device *panel, u32 id)
{
	struct panel_lut_info *lut_info = &panel->panel_data.lut_info;
	int i;

	for (i = 0; i < lut_info->nr_lut; i++) {
		if ((lut_info->lut[i].id & lut_info->lut[i].mask)
				== (id & lut_info->lut[i].mask)) {
			panel_info("%s, found %s (id:0x%08X lut[%d].id:0x%08X lut[%d].mask:0x%08X)\n",
					__func__, lut_info->ddi_node[lut_info->lut[i].ddi_index]->name,
					id, i, lut_info->lut[i].id, i, lut_info->lut[i].mask);
			return lut_info->lut[i].ddi_index;
		}
		pr_debug("%s, id:0x%08X lut[%d].id:0x%08X lut[%d].mask:0x%08X (0x%08X 0x%08X) - not same\n",
				__func__, id, i, lut_info->lut[i].id, i, lut_info->lut[i].mask,
				lut_info->lut[i].id & lut_info->lut[i].mask,
				id & lut_info->lut[i].mask);
	}

	panel_err("%s, panel not found!! (id 0x%08X)\n", __func__, id);

	return -ENODEV;
}

struct device_node *find_panel_ddi_node(struct panel_device *panel, u32 id)
{
	struct panel_lut_info *lut_info = &panel->panel_data.lut_info;
	int index;

	index = find_panel_ddi_lut(panel, id);
	if (index < 0) {
		panel_err("%s, failed to find panel lookup table\n", __func__);
		return NULL;
	}

	if (index >= lut_info->nr_panel_ddi) {
		panel_err("%s, index %d exceeded nr_panel_ddi of lut\n", __func__, index);
		return NULL;
	}

	return lut_info->ddi_node[index];
}

struct seqinfo *find_panel_seqtbl(struct panel_info *panel_data, char *name)
{
	int i;

	if (unlikely(!panel_data->seqtbl)) {
		panel_err("%s, seqtbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < panel_data->nr_seqtbl; i++) {
		if (unlikely(!panel_data->seqtbl[i].name))
			continue;
		if (!strcmp(panel_data->seqtbl[i].name, name)) {
			panel_dbg("%s, found %s panel seqtbl\n", __func__, name);
			return &panel_data->seqtbl[i];
		}
	}
	return NULL;
}


struct seqinfo *find_index_seqtbl(struct panel_info *panel_data, u32 index)
{
	struct seqinfo *tbl;

	if (unlikely(!panel_data->seqtbl)) {
		panel_err("%s, seqtbl not exist\n", __func__);
		return NULL;
	}

	if (unlikely(index >= MAX_PANEL_SEQ)) {
		panel_err("%s, invalid paramter (index %d)\n",
				__func__, index);
		return NULL;
	}

	tbl = &panel_data->seqtbl[index];
	if (tbl != NULL)
		pr_debug("%s, found %s panel seqtbl\n", __func__, tbl->name);

	return tbl;

}


struct pktinfo *alloc_static_packet(char *name, u32 type, u8 *data, u32 dlen)
{
	struct pktinfo *info = kzalloc(sizeof(struct pktinfo), GFP_KERNEL);

	info->name = name;
	info->type = type;
	info->data = data;
	info->dlen = dlen;

	return info;
}

struct pktinfo *find_packet(struct seqinfo *seqtbl, char *name)
{
	int i;
	void **cmdtbl;

	if (unlikely(!seqtbl || !name)) {
		pr_err("%s, invalid paramter (seqtbl %p, name %p)\n",
				__func__, seqtbl, name);
		return NULL;
	}

	cmdtbl = seqtbl->cmdtbl;
	if (unlikely(!cmdtbl)) {
		pr_err("%s, invalid command table\n", __func__);
		return NULL;
	}

	for (i = 0; i < seqtbl->size; i++) {
		if (cmdtbl[i] == NULL) {
			pr_debug("%s, end of cmdtbl %d\n", __func__, i);
			break;
		}
		if (!strcmp(((struct cmdinfo *)cmdtbl[i])->name, name))
			return cmdtbl[i];
	}

	return NULL;
}


struct pktinfo *find_packet_suffix(struct seqinfo *seqtbl, char *name)
{
	int i, j, len;

	void **cmdtbl;

	if (unlikely(!seqtbl || !name)) {
		pr_err("%s, invalid paramter (seqtbl %p, name %p)\n",
				__func__, seqtbl, name);
		return NULL;
	}

	cmdtbl = seqtbl->cmdtbl;
	if (unlikely(!cmdtbl)) {
		pr_err("%s, invalid command table\n", __func__);
		return NULL;
	}

	for (i = 0; i < seqtbl->size; i++) {
		if (cmdtbl[i] == NULL) {
			pr_debug("%s, end of cmdtbl %d\n", __func__, i);
			break;
		}
		len = strlen(((struct cmdinfo *)cmdtbl[i])->name);
		for (j = 0; j < len; j++) {
			if (!strcmp(&((struct cmdinfo *)cmdtbl[i])->name[j], name))
				return cmdtbl[i];
		}
	}

	return NULL;
}

struct maptbl *find_panel_maptbl_by_index(struct panel_info *panel, int index)
{
	if (unlikely(!panel || !panel->maptbl)) {
		pr_err("%s, maptbl not exist\n", __func__);
		return NULL;
	}

	if (index < 0 || index >= panel->nr_maptbl) {
		pr_err("%s, out of range (index %d)\n", __func__, index);
		return NULL;
	}

	return &panel->maptbl[index];
}

struct maptbl *find_panel_maptbl_by_name(struct panel_info *panel_data, char *name)
{
	int i = 0;

	if (unlikely(!panel_data || !panel_data->maptbl)) {
		pr_err("%s, maptbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < panel_data->nr_maptbl; i++)
		if (strstr(panel_data->maptbl[i].name, name))
			return &panel_data->maptbl[i];
	return NULL;
}

struct panel_dimming_info *
find_panel_dimming(struct panel_info *panel_data, char *name)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(panel_data->panel_dim_info); i++) {
		if (!strcmp(panel_data->panel_dim_info[i]->name, name)) {
			panel_dbg("%s, found %s panel dimming\n", __func__, name);
			return panel_data->panel_dim_info[i];
		}
	}
	return NULL;
}

u32 maptbl_index(struct maptbl *tbl, int layer, int row, int col)
{
	return (tbl->nrow * tbl->ncol * layer) + (tbl->ncol * row) + col;
}

int maptbl_init(struct maptbl *tbl)
{
	int ret;

	if (!tbl || !tbl->ops.init)
		return -EINVAL;

	ret = tbl->ops.init(tbl);
	if (ret < 0) {
		pr_err("%s failed to init maptbl(%s)\n",
				__func__, tbl->name);
		return ret;
	}

	tbl->initialized = true;
	return 0;
}

int maptbl_getidx(struct maptbl *tbl)
{
	if (!tbl || !tbl->initialized ||
			!tbl->ops.getidx)
		return -EINVAL;

	return tbl->ops.getidx(tbl);
}

u8 *maptbl_getptr(struct maptbl *tbl)
{
	int index = maptbl_getidx(tbl);

	if (unlikely(index < 0)) {
		pr_err("%s, failed to get index\n", __func__);
		return NULL;
	}

	if (unlikely(!tbl->arr)) {
		pr_err("%s, failed to get pointer\n", __func__);
		return NULL;
	}
	return &tbl->arr[index];
}

void maptbl_copy(struct maptbl *tbl, u8 *dst)
{
	if (!tbl || !tbl->initialized ||
			!dst || !tbl->ops.copy)
		return;

	tbl->ops.copy(tbl, dst);
}

void maptbl_memcpy(struct maptbl *dst, struct maptbl *src)
{
	if (!dst || !src || dst->nlayer != src->nlayer ||
		dst->nrow != src->nrow || dst->ncol != src->ncol) {
		pr_err("%s failed to copy from:%s to:%s size:%d\n",
				__func__, (!src || !src->name) ? "" : src->name,
				(!dst || !dst->name) ? "" : dst->name,
				(!dst) ? 0 : sizeof_maptbl(dst));
		return;
	}

	memcpy(dst->arr, src->arr, sizeof_maptbl(dst));
}

struct maptbl *maptbl_find(struct panel_info *panel, char *name)
{
	int i;

	if (unlikely(!panel || !name)) {
		pr_err("%s, invalid parameter\n", __func__);
		return NULL;
	}

	if (unlikely(!panel->maptbl)) {
		pr_err("%s, maptbl not exist\n", __func__);
		return NULL;
	}

	for (i = 0; i < panel->nr_maptbl; i++)
		if (!strcmp(name, panel->maptbl[i].name))
			return &panel->maptbl[i];
	return NULL;
}

int pktui_maptbl_getidx(struct pkt_update_info *pktui)
{
	if (pktui && pktui->getidx)
		return pktui->getidx(pktui);
	return -EINVAL;
}

void panel_update_packet_data(struct panel_device *panel, struct pktinfo *info)
{
	struct pkt_update_info *pktui;
	struct maptbl *maptbl;
	int i, idx, offset;

	if (!info || !info->data || !info->pktui || !info->nr_pktui) {
		pr_warn("%s, invalid pkt update info\n", __func__);
		return;
	}

	pktui = info->pktui;
	/*
	 * TODO : move out pktui->pdata initial code
	 * panel_pkt_update_info_init();
	 */
	for (i = 0; i < info->nr_pktui; i++) {
		pktui[i].pdata = panel;
		offset = pktui[i].offset;
		if (pktui[i].nr_maptbl > 1 && pktui[i].getidx) {
			idx = pktui_maptbl_getidx(&pktui[i]);
			if (unlikely(idx < 0)) {
				pr_err("%s, failed to pktui_maptbl_getidx %d\n", __func__, idx);
				return;
			}
			maptbl = &pktui[i].maptbl[idx];
		} else {
			maptbl = pktui[i].maptbl;
		}
		if (unlikely(!maptbl)) {
			pr_err("%s, invalid info (offset %d, maptbl %p)\n",
					__func__, pktui[i].offset, pktui[i].maptbl);
			return;
		}

		if (!maptbl->initialized) {
			pr_info("%s init maptbl(%s) +\n", __func__, maptbl->name);
			maptbl_init(maptbl);
			pr_info("%s init maptbl(%s) -\n", __func__, maptbl->name);
		}

		if (unlikely(offset + maptbl->sz_copy > info->dlen)) {
			pr_err("%s, out of range (offset %d, sz_copy %d, dlen %d)\n",
					__func__, offset, maptbl->sz_copy, info->dlen);
			return;
		}
		maptbl_copy(maptbl, &info->data[offset]);
#ifdef DEBUG_PANEL
		pr_info("%s, copy %d bytes from %s to %s[%d]\n",
				__func__, maptbl->sz_copy, maptbl->name, info->name, offset);
#endif
	}
}

static int panel_do_delay(struct panel_device *panel, struct delayinfo *info, ktime_t s_time)
{
	ktime_t e_time = ktime_get();
	int remained_usec = 0;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (ktime_after(e_time, ktime_add_us(s_time, info->usec))) {
		pr_debug("%s skip delay (elapsed %lld usec >= requested %d usec)\n",
				__func__, ktime_to_us(ktime_sub(e_time, s_time)), info->usec);
		return 0;
	}

	if (info->usec >= (u32)ktime_to_us(ktime_sub(e_time, s_time)))
		remained_usec = info->usec - (u32)ktime_to_us(ktime_sub(e_time, s_time));

	if (remained_usec > 0)
		usleep_range(remained_usec, remained_usec + 1);

	return 0;
}

static int panel_do_delay_no_sleep(struct panel_device *panel, struct delayinfo *info, ktime_t s_time)
{
	ktime_t e_time = ktime_get();
	int remained_usec = 0;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (ktime_after(e_time, ktime_add_us(s_time, info->usec))) {
		pr_debug("%s skip delay (elapsed %lld usec >= requested %d usec)\n",
				__func__, ktime_to_us(ktime_sub(e_time, s_time)), info->usec);
		return 0;
	}

	if (info->usec >= (u32)ktime_to_us(ktime_sub(e_time, s_time)))
		remained_usec = info->usec - (u32)ktime_to_us(ktime_sub(e_time, s_time));

	if (remained_usec > 0)
		udelay(remained_usec);

	return 0;
}

static int panel_do_pinctl(struct panel_device *panel, struct pininfo *info)
{
	if (unlikely(!panel || !info)) {
		pr_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (info->pin) {
		// TODO : control external pin
#ifdef DEBUG_PANEL
		pr_info("%s, set %d pin %s\n",
				__func__, info->pin, info->onoff ? "on" : "off");
#endif
	}

	return 0;
}
#ifdef CONFIG_SUPPORT_POC_SPI
static int panel_spi_read_data(struct panel_device *panel,
			u8 cmd_id, u8 *buf, int size)
{
	struct panel_spi_dev *spi_dev;

	if (!panel)
		return -EINVAL;

	spi_dev = &panel->panel_spi_dev;
	if (!spi_dev)
		return -EINVAL;

	return spi_dev->ops->read(spi_dev, cmd_id, buf, size);
}
#endif

static int panel_dsi_write_data(struct panel_device *panel,
		u8 cmd_id, const u8 *buf, u8 ofs, int size, bool block)
{
	u32 option = 0;

	if (unlikely(!panel || !panel->mipi_drv.write))
		return -EINVAL;

	if ((panel->panel_data.ddi_props.gpara &
					DDI_SUPPORT_POINT_GPARA))
		option |= DSIM_OPTION_POINT_GPARA;

	if (block)
		option |= DSIM_OPTION_WAIT_TX_DONE;

	return panel->mipi_drv.write(panel->dsi_id, cmd_id, buf, ofs, size, option, true);
}
static int panel_dsi_write_data_no_wakable(struct panel_device *panel,
		u8 cmd_id, const u8 *buf, u8 ofs, int size, bool block)
{
	u32 option = 0;

	if (unlikely(!panel || !panel->mipi_drv.write))
		return -EINVAL;

	if ((panel->panel_data.ddi_props.gpara &
					DDI_SUPPORT_POINT_GPARA))
		option |= DSIM_OPTION_POINT_GPARA;

	if (block)
		option |= DSIM_OPTION_WAIT_TX_DONE;

	return panel->mipi_drv.write(panel->dsi_id, cmd_id, buf, ofs, size, option, false);
}


/* Todo need to move dt file */
#define SRAM_BYTE_ALIGN	16

static int panel_dsi_write_mem(struct panel_device *panel,
		u8 cmd_id, const u8 *buf, u8 ofs, int size)
{
	u8 c_start = 0, c_next = 0;
	/* TODO: 512 NEED TO CHANGE AS DSIM_FIFO_SIZE */
	u8 cmdbuf[512];
	int tx_size, ret, len = 0;
	int remained = size;

	if (unlikely(!panel || !panel->mipi_drv.write))
		return -EINVAL;

	if (cmd_id == MIPI_DSI_WR_GRAM_CMD) {
		c_start = MIPI_DCS_WRITE_GRAM_START;
		c_next = MIPI_DCS_WRITE_GRAM_CONTINUE;
	} else if (cmd_id == MIPI_DSI_WR_SRAM_CMD) {
		c_start = MIPI_DCS_WRITE_SIDE_RAM_START;
		c_next = MIPI_DCS_WRITE_SIDE_RAM_CONTINUE;
	} else {
		panel_err("%s:invalid cmd_id %d\n",
				__func__, cmd_id);
		return -EINVAL;
	}

	do {
		cmdbuf[0] = (size == remained) ? c_start : c_next;
		tx_size = min(remained, 511);
		tx_size -= (tx_size % SRAM_BYTE_ALIGN);
		memcpy(cmdbuf + 1, buf + len, tx_size);
		ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
				cmdbuf, 0, tx_size + 1, false);
		if (ret != tx_size + 1) {
			panel_err("%s:failed to write command\n", __func__);
			return -EINVAL;
		}
		len += tx_size;
		remained -= tx_size;
		pr_debug("%s tx_size %d len %d, remained %d\n",
				__func__, tx_size, len, remained);
	} while (remained > 0);

	return len;
}

static int panel_dsi_read_data(struct panel_device *panel,
		u8 addr, u8 ofs, u8 *buf, int size)
{
	u32 option = 0;

	if (unlikely(!panel || !panel->mipi_drv.read))
		return -EINVAL;

	if ((panel->panel_data.ddi_props.gpara &
					DDI_SUPPORT_POINT_GPARA))
		option |= DSIM_OPTION_POINT_GPARA;

	return panel->mipi_drv.read(panel->dsi_id, addr, ofs, buf, size, option);
}

static int panel_dsi_get_state(struct panel_device *panel)
{
	if (unlikely(!panel || !panel->mipi_drv.get_state))
		return -EINVAL;

	return panel->mipi_drv.get_state(panel->dsi_id);
}

int panel_set_key(struct panel_device *panel, int level, bool on)
{
	int ret = 0;
	static const unsigned char SEQ_TEST_KEY_ON_9F[] = { 0x9F, 0xA5, 0xA5 };
	static const unsigned char SEQ_TEST_KEY_ON_F0[] = { 0xF0, 0x5A, 0x5A };
	static const unsigned char SEQ_TEST_KEY_ON_FC[] = { 0xFC, 0x5A, 0x5A };
	static const unsigned char SEQ_TEST_KEY_OFF_9F[] = { 0x9F, 0x5A, 0x5A };
	static const unsigned char SEQ_TEST_KEY_OFF_F0[] = { 0xF0, 0xA5, 0xA5 };
	static const unsigned char SEQ_TEST_KEY_OFF_FC[] = { 0xFC, 0xA5, 0xA5 };

	if (unlikely(!panel)) {
		panel_err("ERR:PANEL:%s: panel is null\n", __func__);
		return -EINVAL;
	}

	if (on) {
		if (level >= 1) {
			ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
					SEQ_TEST_KEY_ON_9F, 0, ARRAY_SIZE(SEQ_TEST_KEY_ON_9F), false);
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_9F)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_9F\n", __func__);
				return -EIO;
			}
		}

		if (level >= 2) {
			ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
					SEQ_TEST_KEY_ON_F0, 0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0), false);
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
				return -EIO;
			}
		}

		if (level >= 3) {
			ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
					SEQ_TEST_KEY_ON_FC, 0, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC), false);
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
				return -EIO;
			}
		}
	} else {
		if (level >= 3) {
			ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
					SEQ_TEST_KEY_OFF_FC, 0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC), false);
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_FC)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
				return -EIO;
			}
		}

		if (level >= 2) {
			ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
					SEQ_TEST_KEY_OFF_F0, 0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0), false);
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
				return -EIO;
			}
		}

		if (level >= 1) {
			ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD,
					SEQ_TEST_KEY_OFF_9F, 0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_9F), false);
			if (ret != ARRAY_SIZE(SEQ_TEST_KEY_ON_9F)) {
				pr_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_9F\n", __func__);
				return -EIO;
			}
		}
	}

	return 0;
}

int panel_verify_tx_packet(struct panel_device *panel, u8 *src, u8 ofs, u8 len)
{
	u8 *buf;
	int i, ret = 0;
	unsigned char addr = src[0];
	unsigned char *data = src + 1;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel))
		return 0;

	buf = kcalloc(len, sizeof(u8), GFP_KERNEL);
	if (!buf) {
		pr_err("%s, failed to alloc memory\n", __func__);
		return -ENOMEM;
	}

	//mutex_lock(&panel->op_lock);
	panel_rx_nbytes(panel, DSI_PKT_TYPE_RD, buf, addr, 0, len - 1);
	for (i = 0; i < len - 1; i++) {
		if (buf[i] != data[i]) {
			pr_warn("%02Xh[%2d] - (tx 0x%02X, rx 0x%02X) not match\n",
					addr, i, data[i], buf[i]);
			ret = -EINVAL;
		} else {
			pr_info("%02Xh[%2d] - (tx 0x%02X, rx 0x%02X) match\n",
					addr, i, data[i], buf[i]);
		}
	}
	//mutex_unlock(&panel->op_lock);
	kfree(buf);
	return ret;
}

static int panel_do_tx_packet(struct panel_device *panel, struct pktinfo *info, bool block)
{
	int ret;
	u32 type;
	u8 addr = 0, cmd_id = MIPI_DSI_WR_UNKNOWN;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (info->pktui)
		panel_update_packet_data(panel, info);

	type = info->type;
#ifdef DEBUG_PANEL
	panel_dbg("%s, send packet %s - start\n", __func__, info->name);
#endif
	switch (type) {
	case DSI_PKT_TYPE_COMP:
		cmd_id = MIPI_DSI_WR_DSC_CMD;
		break;
	case DSI_PKT_TYPE_PPS:
		cmd_id = MIPI_DSI_WR_PPS_CMD;
		break;
	case DSI_PKT_TYPE_WR:
		cmd_id = MIPI_DSI_WR_GEN_CMD;
		addr = info->data ? info->data[0] : 0;
		break;
	case DSI_PKT_TYPE_WR_NO_WAKE:
		cmd_id = MIPI_DSI_WR_CMD_NO_WAKE;
		addr = info->data ? info->data[0] : 0;
		break;
	case DSI_PKT_TYPE_WR_MEM:
		cmd_id = MIPI_DSI_WR_GRAM_CMD;
		addr = info->data ? info->data[0] : 0;
		break;
	case DSI_PKT_TYPE_WR_SR:
		cmd_id = MIPI_DSI_WR_SRAM_CMD;
		addr = info->data ? info->data[0] : 0;
		break;
	case CMD_PKT_TYPE_NONE:
	case DSI_PKT_TYPE_RD:
	default:
		cmd_id = MIPI_DSI_WR_UNKNOWN;
		break;
	}

	if (cmd_id == MIPI_DSI_WR_UNKNOWN) {
		panel_err("%s, unknown type\n", __func__);
		return -EINVAL;
	}

#ifdef DEBUG_PANEL
	panel_dbg("%s, %s start\n", __func__, info->name);
#endif

	if (info->option & PKT_OPTION_CHECK_TX_DONE ||
		(addr == MIPI_DCS_SOFT_RESET ||
		 addr == MIPI_DCS_SET_DISPLAY_ON ||
		 addr == MIPI_DCS_SET_DISPLAY_OFF ||
		 addr == MIPI_DCS_ENTER_SLEEP_MODE ||
		 addr == MIPI_DCS_EXIT_SLEEP_MODE ||
		 /* poc command */
		 addr == 0xC0 || addr == 0xC1))
		block = true;

	if (cmd_id == MIPI_DSI_WR_GRAM_CMD ||
			cmd_id == MIPI_DSI_WR_SRAM_CMD)
		ret = panel_dsi_write_mem(panel, cmd_id,
				info->data, info->offset, info->dlen);
	else if (cmd_id == MIPI_DSI_WR_CMD_NO_WAKE)
		ret = panel_dsi_write_data_no_wakable(panel, cmd_id,
				info->data, info->offset, info->dlen, block);
	else
		ret = panel_dsi_write_data(panel, cmd_id,
				info->data, info->offset, info->dlen, block);
	if (ret != info->dlen) {
		panel_err("%s, failed to send packet %s (ret %d)\n",
				__func__, info->name, ret);
		return -EINVAL;
	}
#ifdef DEBUG_PANEL
	panel_dbg("%s, %s end\n", __func__, info->name);
	print_data(info->data, info->dlen);
#endif

	return 0;
}

#ifdef CONFIG_SUPPORT_POC_SPI
static int panel_spi_packet(struct panel_device *panel, struct pktinfo *info)
{
	struct panel_spi_dev *spi_dev;
	int ret = 0;
	u32 type;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}
	spi_dev = &panel->panel_spi_dev;

	if (!spi_dev) {
		panel_err("%s, spi_dev not found\n", __func__);
		return -EINVAL;
	}

	if (!spi_dev->ops) {
		panel_err("%s, spi_dev.ops not found\n", __func__);
		return -EINVAL;
	}

	if (info->pktui)
		panel_update_packet_data(panel, info);

	type = info->type;
	switch (type) {
	case SPI_PKT_TYPE_WR:
		if (!spi_dev->ops->cmd) {
			ret = -ENOTSUPP;
			break;
		}
		ret = spi_dev->ops->cmd(spi_dev, info->data, info->dlen, NULL, 0);
		break;
	case SPI_PKT_TYPE_SETPARAM:
		if (!spi_dev->ops->setparam) {
			ret = -ENOTSUPP;
			break;
		}
		ret = spi_dev->ops->setparam(spi_dev, info->data, info->dlen);
		break;
	default:
		break;
	}

	if (ret < 0) {
		panel_err("%s, failed to send spi packet %s (ret %d type %d)\n", __func__, info->name, ret, type);
		return -EINVAL;
	}
	return 0;

}
#endif

static int panel_do_setkey(struct panel_device *panel, struct keyinfo *info, bool block)
{
	struct panel_info *panel_data;
	bool send_packet = false;
	int ret = 0;

	if (unlikely(!panel || !info)) {
		panel_err("%s, invalid paramter (panel %p, info %p)\n",
				__func__, panel, info);
		return -EINVAL;
	}

	if (info->level < 0 || info->level >= MAX_CMD_LEVEL) {
		panel_err("%s, invalid level %d\n", __func__, info->level);
		return -EINVAL;
	}

	if (info->en != KEY_ENABLE && info->en != KEY_DISABLE) {
		panel_err("%s, invalid key type %d\n", __func__, info->en);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;
	if (info->en == KEY_ENABLE) {
		if (panel_data->props.key[info->level] == 0)
			send_packet = true;
		panel_data->props.key[info->level] += 1;
	} else if (info->en == KEY_DISABLE) {
		if (panel_data->props.key[info->level] < 1) {
			panel_err("%s unbalanced key [%d]%s(%d)\n", __func__, info->level,
			(panel_data->props.key[info->level] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[info->level]);
			return -EINVAL;
		}

		if (panel_data->props.key[info->level] == 1)
			send_packet = true;
		panel_data->props.key[info->level] -= 1;
	}

	if (send_packet) {
		ret = panel_do_tx_packet(panel, info->packet, block);
		if (ret < 0) {
			panel_err("%s failed %s\n", __func__,
					info->packet->name);
		}
	} else {
		pr_warn("%s ignore send key %d %s packet\n",
				__func__, info->level, info->en ? "ENABLED" : "DISABLED");
	}

#ifdef DEBUG_PANEL
	panel_dbg("%s key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n", __func__,
			(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[CMD_LEVEL_1],
			(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[CMD_LEVEL_2],
			(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
			panel_data->props.key[CMD_LEVEL_3]);
#endif

	return 0;
}

int panel_do_init_maptbl(struct panel_device *panel, struct maptbl *maptbl)
{
	int ret;

	if (unlikely(!panel || !maptbl)) {
		panel_err("%s, invalid paramter (panel %p, maptbl %p)\n",
				__func__, panel, maptbl);
		return -EINVAL;
	}

	ret = maptbl_init(maptbl);
	if (ret < 0) {
		panel_err("%s failed to init maptbl(%s) ret %d\n",
				__func__, maptbl->name, ret);
		return ret;
	}

	return 0;
}

int panel_do_seqtbl(struct panel_device *panel, struct seqinfo *seqtbl)
{
	int i, ret = 0;
	u32 type;
	void **cmdtbl;
	bool block = false;
	ktime_t s_time = ktime_get();

	if (unlikely(!panel || !seqtbl)) {
		pr_err("%s, invalid paramter (panel %p, seqtbl %p)\n",
				__func__, panel, seqtbl);
		return -EINVAL;
	}

	cmdtbl = seqtbl->cmdtbl;
	if (unlikely(!cmdtbl)) {
		pr_err("%s, invalid command table\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < seqtbl->size; i++) {
		if (cmdtbl[i] == NULL) {
			pr_debug("%s, end of cmdtbl %d\n", __func__, i);
			break;
		}
		type = *((u32 *)cmdtbl[i]);

		if (type >= MAX_CMD_TYPE) {
			pr_warn("%s invalid cmd type %d\n", __func__, type);
			break;
		}

#ifdef DEBUG_PANEL
		pr_info("SEQ: %s %s %s+\n", seqtbl->name, cmd_type_name[type],
				(((struct cmdinfo *)cmdtbl[i])->name ?
				((struct cmdinfo *)cmdtbl[i])->name : "none"));
#endif
		switch (type) {
		case CMD_TYPE_KEY:
			if (i + 1 < seqtbl->size && cmdtbl[i + 1] &&
					!IS_CMD_TYPE_TX_PKT(*((u32 *)cmdtbl[i + 1])))
				block = true;
			ret = panel_do_setkey(panel, (struct keyinfo *)cmdtbl[i], block);
			break;
		case CMD_TYPE_DELAY:
			ret = panel_do_delay(panel, (struct delayinfo *)cmdtbl[i], s_time);
			break;
		case CMD_TYPE_DELAY_NO_SLEEP:
			ret = panel_do_delay_no_sleep(panel, (struct delayinfo *)cmdtbl[i], s_time);
			break;
		case CMD_TYPE_PINCTL:
			ret = panel_do_pinctl(panel, (struct pininfo *)cmdtbl[i]);
			break;
		case CMD_TYPE_TX_PKT_START ... CMD_TYPE_TX_PKT_END:
			if (i + 1 < seqtbl->size && cmdtbl[i + 1] &&
					!IS_CMD_TYPE_TX_PKT(*((u32 *)cmdtbl[i + 1])))
				block = true;
			ret = panel_do_tx_packet(panel, (struct pktinfo *)cmdtbl[i], block);
			break;
		case CMD_TYPE_RES:
			ret = panel_resource_update(panel, (struct resinfo *)cmdtbl[i]);
			break;
		case CMD_TYPE_SEQ:
			ret = panel_do_seqtbl(panel, (struct seqinfo *)cmdtbl[i]);
			break;
		case CMD_TYPE_MAP:
			ret = panel_do_init_maptbl(panel, (struct maptbl *)cmdtbl[i]);
			break;
		case CMD_TYPE_DMP:
			ret = panel_dumpinfo_update(panel, (struct dumpinfo *)cmdtbl[i]);
			break;
#ifdef CONFIG_SUPPORT_POC_SPI
		case SPI_PKT_TYPE_WR:
		case SPI_PKT_TYPE_SETPARAM:
			ret = panel_spi_packet(panel, (struct pktinfo *)cmdtbl[i]);
			break;
#endif
		case CMD_TYPE_NONE:
		default:
			pr_warn("%s, unknown pakcet type %d\n", __func__, type);
			break;
		}

		if (ret < 0) {
			pr_err("%s, failed to do(seq:%s type:%s cmd:%s)\n", __func__,
					seqtbl->name, cmd_type_name[type],
					(((struct cmdinfo *)cmdtbl[i])->name ?
					 ((struct cmdinfo *)cmdtbl[i])->name : "none"));

			/*
			 * CMD_TYP_DMP seq is intended for debugging only.
			 * So if it's failed, go through the dump seq.
			 */
			if (type != CMD_TYPE_DMP)
				return ret;
		}

		s_time = ktime_get();

#ifdef DEBUG_PANEL
		pr_info("SEQ: %s %s %s-\n", seqtbl->name, cmd_type_name[type],
				(((struct cmdinfo *)cmdtbl[i])->name ?
				((struct cmdinfo *)cmdtbl[i])->name : "none"));
#endif
	}

	return 0;
}

int excute_seqtbl_nolock(struct panel_device *panel, struct seqinfo *seqtbl, int index)
{
	int ret;
	struct seqinfo *tbl;
	struct panel_info *panel_data;
	struct timespec cur_ts, last_ts, delta_ts;
	s64 elapsed_usec;

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (seqtbl == NULL) {
		panel_err("ERR:PANEL:%s:seqtbl is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel))
		return -EIO;

	panel_data = &panel->panel_data;
	tbl = seqtbl;
	ktime_get_ts(&cur_ts);

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s before seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ret = panel_do_seqtbl(panel, &tbl[index]);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl[index].name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:
	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s after seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ktime_get_ts(&last_ts);
	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_debug("seq:%s done (elapsed %2lld.%03lld msec)\n",
			tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);

	return 0;
}

int panel_do_seqtbl_by_index_nolock(struct panel_device *panel, int index)
{
	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:invalid panel\n", __func__);
		return -EINVAL;
	}

	if (panel->panel_data.seqtbl == NULL) {
		panel_err("ERR:PANEL:%s:invalid seqtbl\n", __func__);
		return -EINVAL;
	}

	if (unlikely(index < 0 || index >= MAX_PANEL_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		return -EINVAL;
	}

	return excute_seqtbl_nolock(panel, panel->panel_data.seqtbl, index);
}

int panel_do_seqtbl_by_index(struct panel_device *panel, int index)
{
	int ret;

	mutex_lock(&panel->op_lock);
	ret = panel_do_seqtbl_by_index_nolock(panel, index);
	mutex_unlock(&panel->op_lock);

	return ret;
}

struct resinfo *find_panel_resource(struct panel_info *panel_data, char *name)
{
	int i = 0;

	if (unlikely(!panel_data->restbl))
		return NULL;

	for (i = 0; i < panel_data->nr_restbl; i++)
		if (!strcmp(name, panel_data->restbl[i].name))
			return &panel_data->restbl[i];
	return NULL;
}

bool panel_resource_initialized(struct panel_info *panel_data, char *name)
{
	struct resinfo *res = find_panel_resource(panel_data, name);

	if (unlikely(!res)) {
		panel_err("%s, %s not found in resource\n",
				__func__, name);
		return false;
	}

	return res->state == RES_INITIALIZED;
}

int rescpy(u8 *dst, struct resinfo *res, u32 offset, u32 len)
{
	if (unlikely(!dst || !res)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (unlikely(offset + len > res->dlen)) {
		pr_err("%s, exceed range (offset %d, len %d, res->dlen %d\n",
				__func__, offset, len, res->dlen);
		return -EINVAL;
	}

	memcpy(dst, &res->data[offset], len);
	return 0;
}


int rescpy_by_name(struct panel_info *panel_data, u8 *dst, char *name, u32 offset, u32 len)
{
	struct resinfo *res;

	if (unlikely(!panel_data || !dst || !name)) {
		panel_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		panel_err("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	if (unlikely(offset + len > res->dlen)) {
		panel_err("%s, exceed range (offset %d, len %d, res->dlen %d\n",
				__func__, offset, len, res->dlen);
		return -EINVAL;
	}
	memcpy(dst, &res->data[offset], len);
	return 0;
}

int get_panel_resource_size(struct resinfo *res)
{
	if (unlikely(!res)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	return res->dlen;
}

int resource_clear(struct resinfo *res)
{
	if (!res)
		return -EINVAL;

	res->state = RES_UNINITIALIZED;
	return 0;
}

int resource_clear_by_name(struct panel_info *panel_data, char *name)
{
	struct resinfo *res;

	if (unlikely(!panel_data || !name)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	return resource_clear(res);
}

int resource_copy(u8 *dst, struct resinfo *res)
{
	if (unlikely(!dst || !res)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (res->state == RES_UNINITIALIZED) {
		pr_warn("%s, %s not initialized\n",
				__func__, res->name);
		return -EINVAL;
	}
	return rescpy(dst, res, 0, res->dlen);
}

int resource_copy_by_name(struct panel_info *panel_data, u8 *dst, char *name)
{
	struct resinfo *res;

	if (unlikely(!panel_data || !dst || !name)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	if (res->state == RES_UNINITIALIZED) {
		pr_warn("%s, %s not initialized\n",
				__func__, res->name);
		return -EINVAL;
	}
	return rescpy(dst, res, 0, res->dlen);
}

int resource_copy_n_clear_by_name(struct panel_info *panel_data, u8 *dst, char *name)
{
	struct resinfo *res;
	int ret;

	if (unlikely(!panel_data || !dst || !name)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	if (res->state == RES_UNINITIALIZED) {
		pr_warn("%s, %s not initialized\n",
				__func__, res->name);
		return -EINVAL;
	}
	ret = rescpy(dst, res, 0, res->dlen);
	resource_clear(res);

	return ret;
}

int get_resource_size_by_name(struct panel_info *panel_data, char *name)
{
	struct resinfo *res;

	if (unlikely(!panel_data || !name)) {
		pr_warn("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	return get_panel_resource_size(res);
}

#define MAX_READ_BYTES  (40)
int panel_rx_nbytes(struct panel_device *panel,
		u32 type, u8 *buf, u8 addr, u8 pos, u32 len)
{
	int ret, read_len, remained = len, index = 0;
	static char gpara[] = {0xB0, 0x00};

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_CMD_TYPE_RX_PKT(type)) {
		pr_err("%s, invalid type %d\n", __func__, type);
		return -EINVAL;
	}

#ifdef CONFIG_SUPPORT_POC_SPI
	if (type == SPI_PKT_TYPE_RD) {
		ret = panel_spi_read_data(panel, addr, buf, len);
		if (ret < 0)
			return ret;

		return len;
	}
#endif

	gpara[1] = pos;

	while (remained > 0) {
		read_len = remained < MAX_READ_BYTES ? remained : MAX_READ_BYTES;
		ret = 0;
		if (type == DSI_PKT_TYPE_RD) {
			if ((panel->panel_data.ddi_props.gpara &
					DDI_SUPPORT_READ_GPARA) == false) {
				/*
				 * S6E3HA9 DDI NOT SUPPORTED GPARA FOR READ
				 * SO TEMPORARY DISABLE GPRAR FOR READ FUNCTION
				 */
				char *temp_buf = kmalloc(gpara[1] + read_len, GFP_KERNEL);

				if (!temp_buf)
					return -EINVAL;

				ret = panel_dsi_read_data(panel,
						addr, 0, temp_buf, gpara[1] + read_len);
				ret -= gpara[1];
				memcpy(&buf[index], temp_buf + gpara[1], read_len);
				kfree(temp_buf);
			} else {
				ret = panel_dsi_read_data(panel,
						addr, gpara[1], &buf[index], read_len);
			}
#ifdef CONFIG_EXYNOS_DECON_LCD_SPI
		} else if (type == SPI_PKT_TYPE_RD) {
			if (gpara[1] != 0) {
				ret = panel_dsi_write_data(panel,
						MIPI_DSI_WR_GEN_CMD, gpara, 0, ARRAY_SIZE(gpara), true);
				if (ret != ARRAY_SIZE(gpara))
					panel_err("%s, failed to set gpara %d (ret %d)\n",
							__func__, gpara[1], ret);
			}
			ret = panel_spi_read_data(panel->spi,
					addr, &buf[index], read_len);
#endif
		}

		if (ret != read_len) {
			panel_err("%s, failed to read addr:0x%02X, pos:%d, len:%u ret %d\n",
					__func__, addr, pos, len, ret);
			return -EINVAL;
		}
		index += read_len;
		gpara[1] += read_len;
		remained -= read_len;
	}

#ifdef DEBUG_PANEL
	panel_dbg("%s, addr:%2Xh, pos:%d, len:%u\n", __func__, addr, pos, len);
	print_data(buf, len);
#endif
	return len;
}

int panel_tx_nbytes(struct panel_device *panel,
		u32 type, u8 *buf, u8 addr, u8 pos, u32 len)
{
	int ret;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_CMD_TYPE_TX_PKT(type)) {
		pr_err("%s, invalid type %d\n", __func__, type);
		return -EINVAL;
	}

	ret = panel_dsi_write_data(panel, MIPI_DSI_WR_GEN_CMD, buf, pos, len, false);
	if (ret != len) {
		panel_err("%s, failed to write addr:0x%02X, pos:%d, len:%u ret %d\n",
				__func__, addr, pos, len, ret);
		return -EINVAL;
	}

#ifdef DEBUG_PANEL
	panel_dbg("%s, addr:%2Xh, pos:%d, len:%u\n", __func__, buf[0], pos, len);
	print_data(buf, len);
#endif
	return ret;
}


int read_panel_id(struct panel_device *panel, u8 *buf)
{
	int len, ret = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel))
		return -ENODEV;

	mutex_lock(&panel->op_lock);
	panel_set_key(panel, 3, true);
	len = panel_rx_nbytes(panel, DSI_PKT_TYPE_RD, buf, PANEL_ID_REG, 0, 3);
	if (len != 3) {
		pr_err("%s, failed to read id\n", __func__);
		ret = -EINVAL;
		goto read_err;
	}

read_err:
	panel_set_key(panel, 3, false);
	mutex_unlock(&panel->op_lock);
	return ret;
}

static struct rdinfo *find_panel_rdinfo(struct panel_info *panel_data, char *name)
{
	int i;

	for (i = 0; i < panel_data->nr_rditbl; i++)
		if (!strcmp(name, panel_data->rditbl[i].name))
			return &panel_data->rditbl[i];

	return NULL;
}

int panel_rdinfo_update(struct panel_device *panel, struct rdinfo *rdi)
{
	int ret;

	if (unlikely(!panel || !rdi)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	kfree(rdi->data);
	rdi->data = kcalloc(rdi->len, sizeof(u8), GFP_KERNEL);

#ifdef CONFIG_SUPPORT_DDI_FLASH
	if (rdi->type == DSI_PKT_TYPE_RD_POC)
		ret = copy_poc_partition(&panel->poc_dev, rdi->data, rdi->addr, rdi->offset, rdi->len);
	else
		ret = panel_rx_nbytes(panel, rdi->type, rdi->data, rdi->addr, rdi->offset, rdi->len);
#else
	ret = panel_rx_nbytes(panel, rdi->type, rdi->data, rdi->addr, rdi->offset, rdi->len);
#endif
	if (unlikely(ret != rdi->len)) {
		pr_err("%s, read failed\n", __func__);
		return -EIO;
	}

	return 0;
}

int panel_rdinfo_update_by_name(struct panel_device *panel, char *name)
{
	int ret;
	struct rdinfo *rdi;
	struct panel_info *panel_data;

	if (unlikely(!panel || !name)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rdi = find_panel_rdinfo(panel_data, name);
	if (unlikely(!rdi)) {
		pr_err("%s, read info %s not found\n", __func__, name);
		return -ENODEV;
	}

	ret = panel_rdinfo_update(panel, rdi);
	if (ret) {
		pr_err("%s, failed to update read info\n", __func__);
		return -EINVAL;
	}

	return 0;
}

void print_panel_resource(struct panel_device *panel)
{
	struct panel_info *panel_data;
	int i;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return;
	}

	panel_data = &panel->panel_data;
	if (unlikely(!panel_data)) {
		panel_err("PANEL:ERR:%s:panel_data is null\n", __func__);
		return;
	}

	for (i = 0; i < panel_data->nr_restbl; i++) {
		panel_info("[panel_resource : %12s %3d bytes] %s\n",
				panel_data->restbl[i].name, panel_data->restbl[i].dlen,
				(panel_data->restbl[i].state == RES_UNINITIALIZED ?
				 "UNINITIALIZED" : "INITIALIZED"));
#ifdef DEBUG_PANEL
		if (panel_data->restbl[i].state == RES_INITIALIZED)
			print_data(panel_data->restbl[i].data,
					panel_data->restbl[i].dlen);
#endif
	}
}

int panel_resource_update(struct panel_device *panel, struct resinfo *res)
{
	int i, ret;
	struct rdinfo *rdi;

	if (unlikely(!panel || !res)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < res->nr_resui; i++) {
		rdi = res->resui[i].rditbl;
		if (unlikely(!rdi)) {
			panel_warn("%s read info not exist\n", __func__);
			return -EINVAL;
		}
		ret = panel_rdinfo_update(panel, rdi);
		if (ret) {
			pr_err("%s, failed to update read info\n", __func__);
			return -EINVAL;
		}

		if (!res->data) {
			panel_warn("%s resource data not exist\n", __func__);
			return -EINVAL;
		}
		memcpy(res->data + res->resui[i].offset, rdi->data, rdi->len);
		res->state = RES_INITIALIZED;
	}

#ifdef DEBUG_PANEL
	panel_info("[panel_resource : %12s %3d bytes] %s\n",
			res->name, res->dlen, (res->state == RES_UNINITIALIZED ?
				"UNINITIALIZED" : "INITIALIZED"));
	if (res->state == RES_INITIALIZED)
		print_data(res->data, res->dlen);
#endif

	return 0;
}

int panel_resource_update_by_name(struct panel_device *panel, char *name)
{
	int ret;
	struct resinfo *res;
	struct panel_info *panel_data;

	if (unlikely(!panel || !name)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	res = find_panel_resource(panel_data, name);
	if (unlikely(!res)) {
		pr_warn("%s, %s not found in resource\n",
				__func__, name);
		return -EINVAL;
	}

	ret = panel_resource_update(panel, res);
	if (unlikely(ret)) {
		pr_err("%s, failed to update resource\n", __func__);
		return ret;
	}

	return 0;
}

int panel_dumpinfo_update(struct panel_device *panel, struct dumpinfo *info)
{
	int ret;

	if (unlikely(!panel || !info || !info->res || !info->callback)) {
		pr_err("%s, invalid parameter\n", __func__);
		return -EINVAL;
	}

	ret = panel_resource_update(panel, info->res);
	if (unlikely(ret < 0)) {
		pr_err("%s, dump:%s failed to update resource\n",
				__func__, info->name);
		return ret;
	}
	info->callback(info);

	return 0;
}

int check_panel_active(struct panel_device *panel, const char *caller)
{
	struct panel_state *state;
	int dsi_state;

	if (unlikely(!panel)) {
		panel_err("PANEL:ERR:%s:panel is null\n", caller);
		return 0;
	}

	state = &panel->state;
	if (state->connect_panel == PANEL_DISCONNECT) {
		panel_warn("PANEL:WARN:%s:panel no use\n", caller);
		return 0;
	}
	dsi_state = panel_dsi_get_state(panel);
	if (dsi_state == DSIM_STATE_OFF ||
		dsi_state == DSIM_STATE_DOZE_SUSPEND) {
		panel_err("PANEL:WARN:%s:dsim %s\n",
				caller, (dsi_state == DSIM_STATE_OFF) ?
				"off" : "doze_suspend");
		return 0;
	}

#if defined(CONFIG_SUPPORT_PANEL_SWAP)
	if (ub_con_disconnected(panel)) {
		panel_warn("PANEL:WARN:%s:ub disconnected\n", __func__);
		return 0;
	}
#endif

	return 1;
}
