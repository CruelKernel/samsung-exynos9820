/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * DP bigdata
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/displayport_bigdata.h>

#define EDID_BUF_SIZE 512
#define ERR_DATA_BUF_SIZE 1024
#define COL_NAME_SIZE 20

enum DP_ITEM_TYPE {
	INT = 1,
	HEX = 2,
	STR = 4,
	CHR = 8,
	ERR = 16,
};

enum DP_STATUS {
	STATUS_NO_CONNECTION,
	STATUS_CONNECTION,
	STATUS_ERROR_OCCURRED,
};

struct bd_item_info {
	char name[COL_NAME_SIZE];
	char type;
	void *data;
	int str_max_len;
};

struct bd_error_data {
	int limit;
	int count;
};

static char err_data_buf[ERR_DATA_BUF_SIZE];
static struct bd_item_info item_to_column[BD_ITEM_MAX];
static enum DP_STATUS dp_status;

static void secdp_bigdata_save_data(void);
static void secdp_bigdata_init_item(enum DP_BD_ITEM_LIST item, char *col_name, enum DP_ITEM_TYPE type, ...);
static void secdp_bigdata_init_error(enum DP_BD_ITEM_LIST item, char *col_name, int err_limit);
static void secdp_bigdata_save_item_int(enum DP_BD_ITEM_LIST item, int val);
static void secdp_bigdata_save_item_hex(enum DP_BD_ITEM_LIST item, int val);
static void secdp_bigdata_save_item_char(enum DP_BD_ITEM_LIST item, char val);
static void secdp_bigdata_save_item_str(enum DP_BD_ITEM_LIST item, char *val);

static ssize_t dp_error_info_show(struct class *class,
				struct class_attribute *attr, char *buf)
{
	if (dp_status == STATUS_NO_CONNECTION)
		return 0;

	return scnprintf(buf, ERR_DATA_BUF_SIZE, "%s", err_data_buf);
}

static ssize_t dp_error_info_store(struct class *dev,
				struct class_attribute *attr, const char *buf, size_t size)
{
	if ((buf[0] | 0x20) == 'c')
		dp_status = STATUS_NO_CONNECTION;

	return size;
}

static CLASS_ATTR_RW(dp_error_info);

void secdp_bigdata_init(struct class *dp_class)
{
	int ret;

	ret = class_create_file(dp_class, &class_attr_dp_error_info);
	if (ret)
		pr_err("failed to create attr_dp_bigdata(%d)\n", ret);

	secdp_bigdata_init_item(BD_LINK_CONFIGURE, "LINK_CFG", CHR);
	secdp_bigdata_init_item(BD_ADAPTER_HWID, "ADT_HWID", HEX);
	secdp_bigdata_init_item(BD_ADAPTER_FWVER, "ADT_FWVER", HEX);
	secdp_bigdata_init_item(BD_ADAPTER_TYPE, "ADT_TYPE", STR, 20);
	secdp_bigdata_init_item(BD_MAX_LANE_COUNT, "MLANE_CNT", INT);
	secdp_bigdata_init_item(BD_MAX_LINK_RATE, "MLINK_RATE", INT);
	secdp_bigdata_init_item(BD_CUR_LANE_COUNT, "CLANE_CNT", INT);
	secdp_bigdata_init_item(BD_CUR_LINK_RATE, "CLINK_RATE", INT);
	secdp_bigdata_init_item(BD_HDCP_VER, "HDCP_VER", STR, 10);
	secdp_bigdata_init_item(BD_ORIENTATION, "ORIENTATION", STR, 10);
	secdp_bigdata_init_item(BD_RESOLUTION, "RESOLUTION", STR, 20);
	secdp_bigdata_init_item(BD_EDID, "EDID", STR, EDID_BUF_SIZE);
	secdp_bigdata_init_item(BD_ADT_VID, "ADT_VID", HEX);
	secdp_bigdata_init_item(BD_ADT_PID, "ADT_PID", HEX);
	secdp_bigdata_init_item(BD_DP_MODE, "DP_MODE", STR, 10);
	secdp_bigdata_init_item(BD_SINK_NAME, "SINK_NAME", STR, 14);
	secdp_bigdata_init_item(BD_AUD_CH, "AUD_CH", INT);
	secdp_bigdata_init_item(BD_AUD_FREQ, "AUD_FREQ", INT);
	secdp_bigdata_init_item(BD_AUD_BIT, "AUD_BIT", INT);

	secdp_bigdata_init_error(ERR_AUX, "ERR_AUX", 3);
	secdp_bigdata_init_error(ERR_EDID, "ERR_EDID", 1);
	secdp_bigdata_init_error(ERR_HDCP_AUTH, "ERR_HDCP", 5);
	secdp_bigdata_init_error(ERR_LINK_TRAIN, "ERR_LT_TRAIN", 1);
	secdp_bigdata_init_error(ERR_INF_IRQHPD, "ERR_INF_IRQHPD", 10);
}

static void secdp_bigdata_init_item_str(enum DP_BD_ITEM_LIST item, char *val, int max_len)
{
	kfree(item_to_column[item].data);

	item_to_column[item].data = kzalloc(max_len + 1, GFP_KERNEL);
	if (!item_to_column[item].data)
		return;

	item_to_column[item].str_max_len = max_len;
	strlcpy((char *)item_to_column[item].data, val, max_len + 1);
}

static void secdp_bigdata_init_item(enum DP_BD_ITEM_LIST item, char *col_name, enum DP_ITEM_TYPE type, ...)
{
	va_list  vl;

	va_start(vl, type);

	strlcpy(item_to_column[item].name, col_name, COL_NAME_SIZE);
	item_to_column[item].type = type;

	switch (type) {
	case INT:
	case HEX:
		secdp_bigdata_save_item_int(item, -1);
		break;
	case STR:
		secdp_bigdata_init_item_str(item, "X", (int)va_arg(vl, int));
		break;
	case CHR:
		secdp_bigdata_save_item_char(item, 'X');
		break;
	default:
		break;
	}

	va_end(vl);
}

static void secdp_bigdata_init_error(enum DP_BD_ITEM_LIST item, char *col_name, int err_limit)
{
	struct bd_error_data *err = kzalloc(sizeof(struct bd_error_data), GFP_KERNEL);

	if (err)
		err->limit = err_limit;

	strlcpy(item_to_column[item].name, col_name, COL_NAME_SIZE);
	item_to_column[item].type = ERR;
	item_to_column[item].data = err;
}

static void secdp_bigdata_save_item_int(enum DP_BD_ITEM_LIST item, int val)
{
	if (!item_to_column[item].data) {
		item_to_column[item].data = kzalloc(sizeof(int), GFP_KERNEL);
		if (!item_to_column[item].data)
			return;
	}

	*((int *)item_to_column[item].data) = val;
}

static void secdp_bigdata_save_item_hex(enum DP_BD_ITEM_LIST item, int val)
{
	secdp_bigdata_save_item_int(item, val);
}

static void secdp_bigdata_save_item_char(enum DP_BD_ITEM_LIST item, char val)
{
	if (!item_to_column[item].data) {
		item_to_column[item].data = kzalloc(sizeof(char), GFP_KERNEL);
		if (!item_to_column[item].data)
			return;
	}

	*((char *)item_to_column[item].data) = val;
}

static void secdp_bigdata_save_item_str(enum DP_BD_ITEM_LIST item, char *val)
{
	if (!item_to_column[item].data || !val)
		return;

	if (item == BD_EDID && val[0] != 'X') {
		int ret = 0;
		int i;
		int ext_blk_cnt = val[0x7e] ? 1 : 0;
		int edid_size = 128 * (ext_blk_cnt + 1);

		for (i = 0; i < edid_size; i++) {
			ret += scnprintf(((char *)item_to_column[item].data) + ret,
					EDID_BUF_SIZE + 1 - ret, "%02x",
					val[i]);
		}

	} else {
		strlcpy((char *)item_to_column[item].data, val,
				item_to_column[item].str_max_len + 1);
	}
}

void secdp_bigdata_save_item(enum DP_BD_ITEM_LIST item, ...)
{
	va_list  vl;

	if (item >= BD_ITEM_MAX || item < 0)
		return;

	va_start(vl, item);

	switch (item_to_column[item].type) {
	case INT:
		secdp_bigdata_save_item_hex(item, (int)va_arg(vl, int));
		break;
	case HEX:
		secdp_bigdata_save_item_int(item, (int)va_arg(vl, int));
		break;
	case STR:
		secdp_bigdata_save_item_str(item, (char *)va_arg(vl, char *));
		break;
	case CHR:
		secdp_bigdata_save_item_char(item, (char)va_arg(vl, int));
		break;
	default:
		break;
	}

	va_end(vl);
}

void secdp_bigdata_inc_error_cnt(enum DP_BD_ITEM_LIST err)
{
	if (err >= BD_ITEM_MAX || err < 0)
		return;

	if (item_to_column[err].data && item_to_column[err].type == ERR)
		((struct bd_error_data *)item_to_column[err].data)->count++;
}

void secdp_bigdata_clr_error_cnt(enum DP_BD_ITEM_LIST err)
{
	if (err >= BD_ITEM_MAX || err < 0)
		return;

	if (item_to_column[err].data && item_to_column[err].type == ERR)
		((struct bd_error_data *)item_to_column[err].data)->count = 0;
}

static void secdp_bigdata_save_data(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < BD_ITEM_MAX; i++) {
		switch (item_to_column[i].type) {
		case INT:
			ret += scnprintf(err_data_buf + ret, ERR_DATA_BUF_SIZE - ret,
				"\"%s\":\"%d\",",
				item_to_column[i].name,
				(item_to_column[i].data != NULL) ?
				*((int *)item_to_column[i].data) : -1);
			break;
		case HEX:
			ret += scnprintf(err_data_buf + ret, ERR_DATA_BUF_SIZE - ret,
				"\"%s\":\"0x%x\",",
				item_to_column[i].name,
				(item_to_column[i].data != NULL) ?
				*((int *)item_to_column[i].data) : -1);
			break;
		case STR:
			ret += scnprintf(err_data_buf + ret, ERR_DATA_BUF_SIZE - ret,
				"\"%s\":\"%s\",",
				item_to_column[i].name,
				(item_to_column[i].data != NULL) ?
				(char *)item_to_column[i].data : "X");
			break;
		case CHR:
			ret += scnprintf(err_data_buf + ret, ERR_DATA_BUF_SIZE - ret,
				"\"%s\":\"%c\",",
				item_to_column[i].name,
				(item_to_column[i].data != NULL) ?
				*((char *)item_to_column[i].data) : 'X');
			break;
		case ERR:
			ret += scnprintf(err_data_buf + ret, ERR_DATA_BUF_SIZE - ret,
				"\"%s\":\"%d\",",
				item_to_column[i].name,
				(item_to_column[i].data != NULL) ?
				((struct bd_error_data *)item_to_column[i].data)->count : 0);
			break;
		default:
			break;
		}
	}
	err_data_buf[ret-1] = '\n';
}

static int secdp_bigdata_check_err(void)
{
	int i;
	struct bd_error_data *e_data;

	for (i = 0; i < BD_ITEM_MAX; i++) {
		if (item_to_column[i].type == ERR) {
			e_data = item_to_column[i].data;
			if (e_data != NULL && e_data->count >= e_data->limit)
				return 1;
		}
	}

	return 0;
}

void secdp_bigdata_connection(void)
{
	int i;

	if (dp_status != STATUS_ERROR_OCCURRED)
		dp_status = STATUS_CONNECTION;

	for (i = 0; i < BD_ITEM_MAX; i++) {
		switch (item_to_column[i].type) {
		case INT:
		case HEX:
			secdp_bigdata_save_item_int(i, -1);
			break;
		case STR:
			secdp_bigdata_save_item_str(i, "X");
			break;
		case CHR:
			secdp_bigdata_save_item_char(i, 'X');
			break;
		case ERR:
			secdp_bigdata_clr_error_cnt(i);
			break;
		default:
			break;
		}
	}
}

void secdp_bigdata_disconnection(void)
{
	if (secdp_bigdata_check_err()) {
		dp_status = STATUS_ERROR_OCCURRED;
		secdp_bigdata_save_data();
	}

	if (dp_status != STATUS_ERROR_OCCURRED)
		secdp_bigdata_save_data();

}
