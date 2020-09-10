// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * JiHoon Kim <jihoonn.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ctype.h>
#include <linux/lcd.h>

#include "panel.h"
#include "panel_drv.h"
#include "panel_bl.h"
#include "copr.h"
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
#include "mdnie.h"
#endif
#ifdef CONFIG_PANEL_AID_DIMMING
#include "dimming.h"
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
#ifdef CONFIG_DYNAMIC_FREQ
#include "dynamic_freq.h"
#endif
static DEFINE_MUTEX(sysfs_lock);

char *mcd_rs_name[MAX_MCD_RS] = {
	"MCD1_R", "MCD1_L", "MCD2_R", "MCD2_L",
};

extern struct kset *devices_kset;

#ifdef CONFIG_EXYNOS_LCD_ENG
#ifdef CONFIG_SUPPORT_ISC_TUNE_TEST
static ssize_t isc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.isc_threshold);

	return strlen(buf);
}

static ssize_t isc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.isc_threshold == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.isc_threshold = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel, PANEL_ISC_THRESHOLD_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write isc threshold seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, isc N %d\n",
			__func__, panel_data->props.isc_threshold);

	return size;
}

int print_stm_info(u8 *stm_field, char *buf)
{
	snprintf(buf, PAGE_SIZE, "CTRL EN=%d, MAX_OPT=%d, DEFAULT_OPT=%d, DIM_STEP=%d, FRAME_PERIOD=%d, MIN_SECT=%d, PIXEL_PERIOD=%d, LINE_PERIOD=%d, MIN_MOVE=%d, M_THRES=%d, V_THRES=%d\n",
	stm_field[STM_CTRL_EN], stm_field[STM_MAX_OPT], stm_field[STM_DEFAULT_OPT],
	stm_field[STM_DIM_STEP], stm_field[STM_FRAME_PERIOD], stm_field[STM_MIN_SECT], stm_field[STM_PIXEL_PERIOD],
	stm_field[STM_LINE_PERIOD], stm_field[STM_MIN_MOVE], stm_field[STM_M_THRES], stm_field[STM_V_THRES]);

	return strlen(buf);
}
static ssize_t stm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	return print_stm_info(panel_data->props.stm_field_info, buf);
}

int set_stm_info(char *user_set, u8 *stm_field)
{
	int i;
	int val = 0, ret;

	for (i = STM_CTRL_EN; i < STM_FIELD_MAX; i++) {
		if (strncmp(user_set, str_stm_fied[i], strlen(str_stm_fied[i])) == 0) {
			ret = sscanf(user_set + strlen(str_stm_fied[i]), "%d", &val);
			stm_field[i] = val;
			return 0;
		}
	}
	return -EINVAL;
}

static ssize_t stm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	char *recv_buf;
	char *ptr = NULL;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	recv_buf = (char *)buf;
	while ((ptr = strsep(&recv_buf, " \t")) != NULL) {
		if (*ptr) {
			ret = set_stm_info(ptr, panel_data->props.stm_field_info);
			if (ret < 0)
				pr_info("%s invalid input %s\n", __func__, ptr);
		}
	}
	ret = panel_do_seqtbl_by_index(panel, PANEL_STM_TUNE_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write stm_tune\n", __func__);
		return ret;
	}
	dev_info(dev, "%s\n", __func__);

	return size;
}

#endif


unsigned char readbuf[256] = { 0xff, };
int readreg, readpos, readlen;

static ssize_t read_mtp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, len;

	mutex_lock(&sysfs_lock);
	if (readreg <= 0 || readreg > 0xFF || readlen <= 0 || readlen > 255 ||
			readpos < 0 || readpos > 255) {
		mutex_unlock(&sysfs_lock);
		return -EINVAL;
	}

	len = snprintf(buf, PAGE_SIZE,
			"addr:0x%02X pos:%d size:%d\n",
			readreg, readpos, readlen);
	for (i = 0; i < readlen; i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "0x%02X%s", readbuf[i],
				(((i + 1) % 16) == 0) || (i == readlen - 1) ? "\n" : " ");

	readreg = 0;
	readpos = 0;
	readlen = 0;
	mutex_unlock(&sysfs_lock);

	return len;
}

static ssize_t read_mtp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	int ret, i;

	if (!IS_PANEL_ACTIVE(panel))
		return -EIO;

	mutex_lock(&sysfs_lock);
	ret = sscanf(buf, "%x %d %d", &readreg, &readpos, &readlen);
	if (ret != 3 || readreg <= 0 || readreg > 0xFF ||
			readlen <= 0 || readlen > 255 || readpos < 0 || readpos > 255) {

		ret = -EINVAL;
		pr_info("%s %x %d %d\n", __func__, readreg, readpos, readlen);
		goto store_err;
	}

	mutex_lock(&panel->op_lock);
	panel_set_key(panel, 3, true);
	ret = panel_rx_nbytes(panel, DSI_PKT_TYPE_RD, readbuf, readreg, readpos, readlen);
	panel_set_key(panel, 3, false);
	mutex_unlock(&panel->op_lock);

	if (unlikely(ret != readlen)) {
		pr_err("%s, failed to read reg %02Xh pos %d len %d\n",
				__func__, readreg, readpos, readlen);
		ret = -EIO;
		goto store_err;
	}

	pr_info("READ_Reg addr: %02x, pos : %d len : %d\n",
			readreg, readpos, readlen);
	for (i = 0; i < readlen; i++)
		pr_info("READ_Reg %dth : %02x\n", i + 1, readbuf[i]);
	mutex_unlock(&sysfs_lock);

	return size;

store_err:
	readreg = 0;
	readpos = 0;
	readlen = 0;
	mutex_unlock(&sysfs_lock);

	return ret;
}

static ssize_t write_mtp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;

	return len;
}

static ssize_t write_mtp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	int i = 0, val = 0, ret = 0, send_len = 0;
	char *recv_buf = (char *)buf;
	char *ptr = NULL;
	u8 *tx_buf = NULL;
	struct panel_info *panel_data;

	if (!IS_PANEL_ACTIVE(panel))
		return -EIO;

	ptr = strsep(&recv_buf, " ");
	ret = sscanf(ptr, "%d", &send_len);

	if (ret < 1 || send_len < 1) {
		panel_err("PANEL:ERR:%s: invalid parameter. %d %d\n", __func__, ret, send_len);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;

	tx_buf = kzalloc(sizeof(u8) * (send_len), GFP_KERNEL);

	if (tx_buf == NULL) {
		panel_err("PANEL:ERR:%s: fail to allloc buffer\n", __func__);
		return -ENOMEM;
	}
	pr_info("%s len: %d\n", __func__, send_len);

	for (i = 0; i < send_len; i++) {
		ptr = strsep(&recv_buf, " \t");
		if (ptr == NULL) {
			//adjust send_len by filled buffer
			send_len = i;
			break;
		}
		ret = sscanf(ptr, "%02x", &val);
		if (ret < 1) {
			panel_err("PANEL:ERR:%s: wrong parameter: %d\n", __func__, i);
			ret = -EINVAL;
			goto write_mtp_store_out;
		}
		tx_buf[i] = val;
		pr_info("%s %d: 0x%02x\n", __func__, i, val);
	}

	mutex_lock(&panel->op_lock);

	panel_set_key(panel, 3, true);

	ret = panel_tx_nbytes(panel, DSI_PKT_TYPE_WR, tx_buf, tx_buf[0], 0, send_len);

	panel_set_key(panel, 3, false);

	mutex_unlock(&panel->op_lock);

	if (unlikely(ret != send_len)) {
		pr_err("%s, failed to write reg %02Xh len %d %d\n",
				__func__, buf[0], send_len, ret);
		ret = -EIO;
		goto write_mtp_store_out;
	}
	pr_info("%s %d byte(s) sent.\n", __func__, ret);
	ret = size;

write_mtp_store_out:
	kfree(tx_buf);
	return ret;
}

static ssize_t gamma_interpolation_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	if (panel_data->props.gamma_control_buf != NULL) {
		snprintf(buf, PAGE_SIZE, "%x %x %x %x %x %x\n",
			panel_data->props.gamma_control_buf[0], panel_data->props.gamma_control_buf[1],
			panel_data->props.gamma_control_buf[2], panel_data->props.gamma_control_buf[3],
			panel_data->props.gamma_control_buf[4], panel_data->props.gamma_control_buf[5]);
	}
	return strlen(buf);
}

static ssize_t gamma_interpolation_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data;
	int ret;
	u8 write_buf[6] = { 0x00, };
	int i;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	ret = sscanf(buf, "%x %x %x %x %x %x",
						&write_buf[0], &write_buf[1], &write_buf[2],
						&write_buf[3], &write_buf[4], &write_buf[5]);
	if (ret != 6) {
		panel_err("PANEL:ERR:%s:invalid input %d\n", __func__, ret);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	kfree(panel_data->props.gamma_control_buf);

	panel_data->props.gamma_control_buf = kzalloc(sizeof(write_buf), GFP_KERNEL);
	mutex_lock(&panel->op_lock);
	memcpy(panel_data->props.gamma_control_buf, write_buf, sizeof(write_buf));
	mutex_unlock(&panel->op_lock);
	for (i = 0; i < 6; i++)
		pr_info("%s %x %x\n", __func__, write_buf[i], panel_data->props.gamma_control_buf[i]);

	ret = panel_do_seqtbl_by_index(panel, PANEL_GAMMA_INTER_CONTROL_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write gamma interpolation control seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s\n", __func__);
	return size;
}

#endif


//void g_tracing_mark_write( char id, char* str1, int value );
int fingerprint_value = -1;
static ssize_t fingerprint_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%u\n", fingerprint_value);

	return strlen(buf);
}

static ssize_t fingerprint_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;

	rc = kstrtouint(buf, 0, &fingerprint_value);
	if (rc < 0)
		return rc;

	//g_tracing_mark_write( 'C', "BCDS_hbm", fingerprint_value & 4);
	//g_tracing_mark_write( 'C', "BCDS_alpha", fingerprint_value & 2);

	dev_info(dev, "%s: %d\n", __func__, fingerprint_value);

	return size;
}

static ssize_t lcd_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "SDC_%02X%02X%02X\n",
			panel_data->id[0], panel_data->id[1], panel_data->id[2]);

	return strlen(buf);
}

static ssize_t window_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%02x %02x %02x\n",
			panel_data->id[0], panel_data->id[1], panel_data->id[2]);
	pr_info("%s %02x %02x %02x\n",
			__func__, panel_data->id[0], panel_data->id[1], panel_data->id[2]);
	return strlen(buf);
}

static ssize_t manufacture_code_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 code[5] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, code, "code");

	snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X%02X\n",
		code[0], code[1], code[2], code[3], code[4]);

	return strlen(buf);
}

static ssize_t SVC_OCTA_DDI_CHIPID_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return manufacture_code_show(dev, attr, buf);
}

static ssize_t cell_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 date[PANEL_DATE_LEN] = { 0, }, coordinate[4] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, date, "date");
	resource_copy_by_name(panel_data, coordinate, "coordinate");

	snprintf(buf, PAGE_SIZE, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
		date[0], date[1], date[2], date[3], date[4], date[5], date[6],
		coordinate[0], coordinate[1], coordinate[2], coordinate[3]);

	return strlen(buf);
}

static ssize_t SVC_OCTA_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return cell_id_show(dev, attr, buf);
}

static ssize_t octa_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, site, rework, poc;
	u8 cell_id[16], octa_id[PANEL_OCTA_ID_LEN] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int len = 0;
	bool cell_id_exist = true;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	resource_copy_by_name(panel_data, octa_id, "octa_id");

	site = (octa_id[0] >> 4) & 0x0F;
	rework = octa_id[0] & 0x0F;
	poc = octa_id[1] & 0x0F;

	pr_debug("site (%d), rework (%d), poc (%d)\n",
			site, rework, poc);

	pr_debug("<CELL ID>\n");
	for (i = 0; i < 16; i++) {
		cell_id[i] = isalnum(octa_id[i + 4]) ? octa_id[i + 4] : '\0';
		pr_debug("%x -> %c\n", octa_id[i + 4], cell_id[i]);
		if (cell_id[i] == '\0') {
			cell_id_exist = false;
			break;
		}
	}

	len += snprintf(buf + len, PAGE_SIZE - len, "%d%d%d%02x%02x",
			site, rework, poc, octa_id[2], octa_id[3]);
	if (cell_id_exist) {
		for (i = 0; i < 16; i++)
			len += snprintf(buf + len, PAGE_SIZE - len, "%c", cell_id[i]);
	}
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return strlen(buf);
}

static ssize_t SVC_OCTA_CHIPID_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return octa_id_show(dev, attr, buf);
}

static ssize_t color_coordinate_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u8 coordinate[4] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, coordinate, "coordinate");

	snprintf(buf, PAGE_SIZE, "%u, %u\n", /* X, Y */
			coordinate[0] << 8 | coordinate[1],
			coordinate[2] << 8 | coordinate[3]);
	return strlen(buf);
}

static ssize_t manufacture_date_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	u16 year;
	u8 month, day, hour, min, date[PANEL_DATE_LEN] = { 0, };
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	resource_copy_by_name(panel_data, date, "date");

	year = ((date[0] & 0xF0) >> 4) + 2011;
	month = date[0] & 0xF;
	day = date[1] & 0x1F;
	hour = date[2] & 0x1F;
	min = date[3] & 0x3F;

	snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d:%d\n",
			year, month, day, hour, min);
	return strlen(buf);
}

static ssize_t brightness_table_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;
	int br, len = 0, recv_len = 0, prev_br = 0;
	int actual_brightness = 0, prev_actual_brightness = 0;
	char recv_buf[50] = {0, };
	int recv_buf_len = ARRAY_SIZE(recv_buf);
	int max_brightness = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	max_brightness = get_max_brightness(panel_bl);

	mutex_lock(&panel_bl->lock);
	for (br = 0; br <= max_brightness; br++) {
		actual_brightness = get_actual_brightness(panel_bl, br);
		if (recv_len == 0) {
			recv_len += snprintf(recv_buf, recv_buf_len, "%5d", prev_br);
			prev_actual_brightness = actual_brightness;
		}
		if ((prev_actual_brightness != actual_brightness) || (br == max_brightness))  {
			if (recv_len < recv_buf_len) {
				recv_len += snprintf(recv_buf + recv_len, recv_buf_len - recv_len,
					"~%5d %3d\n", prev_br, prev_actual_brightness);
				len += snprintf(buf + len, PAGE_SIZE - len, "%s", recv_buf);
			}
			recv_len = 0;
			memset(recv_buf, 0x00, sizeof(recv_buf));
		}
		prev_actual_brightness = actual_brightness;
		prev_br = br;
		if (len >= PAGE_SIZE) {
			pr_info("%s print buffer overflow %d\n", __func__, len);
			len = PAGE_SIZE - 1;
			goto exit;
		}
	}
	len += snprintf(buf + len, PAGE_SIZE - len, "%s", recv_buf);
exit:
	mutex_unlock(&panel_bl->lock);

	return len;
}

static ssize_t adaptive_control_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n",
			panel_data->props.adaptive_control);

	return strlen(buf);
}

static ssize_t adaptive_control_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;
	int value, rc;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value < 0 || value >= ACL_OPR_MAX) {
		panel_err("PANEL:ERR:%s:invalid adaptive_control %d\n",
				__func__, value);
		return -EINVAL;
	}

	if (panel_data->props.adaptive_control == value)
		return size;

	mutex_lock(&panel_bl->lock);
	panel_data->props.adaptive_control = value;
	mutex_unlock(&panel_bl->lock);
	panel_update_brightness(panel);

	dev_info(dev, "%s, adaptive_control %d\n",
			__func__, panel_data->props.adaptive_control);

	return size;
}

static ssize_t siop_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.siop_enable);

	return strlen(buf);
}

static ssize_t siop_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int value, rc;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.siop_enable == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.siop_enable = value;
	mutex_unlock(&panel->op_lock);
	panel_update_brightness(panel);

	dev_info(dev, "%s, siop_enable %d\n",
			__func__, panel_data->props.siop_enable);
	return size;
}

static ssize_t temperature_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char temp[] = "-15, -14, 0, 1\n";

	strcat(buf, temp);
	return strlen(buf);
}

static ssize_t temperature_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int value, rc;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtoint(buf, 10, &value);
	if (rc < 0)
		return rc;

	mutex_lock(&panel->op_lock);
	panel_data->props.temperature = value;
	mutex_unlock(&panel->op_lock);
	panel_update_brightness(panel);

	dev_info(dev, "%s, temperature %d\n",
			__func__, panel_data->props.temperature);

	return size;
}

static ssize_t mcd_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n", panel_data->props.mcd_on);

	return strlen(buf);
}

static ssize_t mcd_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.mcd_on == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.mcd_on = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_MCD_ON_SEQ : PANEL_MCD_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, mcd %s\n",
			__func__, panel_data->props.mcd_on ? "on" : "off");

	return size;
}

static void print_mcd_resistance(u8 *mcd_nok, int size)
{
	int code, len;
	char buf[1024];

	len = snprintf(buf, sizeof(buf),
			"MCD CHECK [b7:MCD1_R, b6:MCD2_R, b3:MCD1_L, b2:MCD2_L]\n");
	for (code = 0; code < size; code++) {
		if (!(code % 0x10))
			len += snprintf(buf + len, sizeof(buf) - len, "[%02X] ", code);
		len += snprintf(buf + len, sizeof(buf) - len, "%02X%s",
				mcd_nok[code], (!((code + 1) % 0x10)) ? "\n" : " ");
	}
	pr_info("%s\n", buf);
}

static int read_mcd_resistance(struct panel_device *panel)
{
	int i, ret, code;
	u8 mcd_nok[128];
	struct panel_info *panel_data = &panel->panel_data;
	int stt, end;
	u8 mcd_rs_mask[MAX_MCD_RS] = {
		(1U << 7),
		(1U << 3),
		(1U << 6),
		(1U << 2),
	};
	s64 elapsed_usec;
	struct timespec cur_ts, last_ts, delta_ts;

	ktime_get_ts(&last_ts);
	disable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_MCD_RS_ON_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd_3_0_on seq\n", __func__);
		goto out;
	}

	memset(mcd_nok, 0, sizeof(mcd_nok));
	for (code = 0; code < 0x80; code++) {
		panel_data->props.mcd_resistance = code;
		ret = panel_do_seqtbl_by_index_nolock(panel,
				PANEL_MCD_RS_READ_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write mcd_rs_read seq\n", __func__);
			goto out;
		}

		ret = resource_copy_n_clear_by_name(panel_data,
				&mcd_nok[code], "mcd_resistance");
		if (unlikely(ret < 0)) {
			pr_err("%s failed to copy resource(mcd_resistance) (ret %d)\n",
					__func__, ret);
			goto out;
		}
		pr_debug("%s %02X : %02X\n", __func__, code, mcd_nok[code]);
	}
	print_mcd_resistance(mcd_nok, ARRAY_SIZE(mcd_nok));

	for (i = 0; i < MAX_MCD_RS; i++) {
		for (code = 0, stt = -1, end = -1; code < 0x80; code++) {
			if (mcd_nok[code] & mcd_rs_mask[i]) {
				if (stt == -1)
					stt = code;
				end = code;
			}
		}
		panel_data->props.mcd_rs_range[i][0] = stt;
		panel_data->props.mcd_rs_range[i][1] = end;
	}

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_MCD_RS_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd_3_0_off seq\n", __func__);
		goto out;
	}

out:
	clear_pending_bit(panel->gpio[PANEL_GPIO_DISP_DET].irq);
	enable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);

	ktime_get_ts(&cur_ts);
	delta_ts = timespec_sub(cur_ts, last_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_info("%s done (elapsed %2lld.%03lld msec)\n",
			__func__, elapsed_usec / 1000, elapsed_usec % 1000);

	return ret;
}

static ssize_t mcd_resistance_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int i, len = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	mutex_lock(&panel->op_lock);
	for (i = 0; i < MAX_MCD_RS; i++)
		len += snprintf(buf + len, PAGE_SIZE - len,
				"SDC_%s:%d%s", mcd_rs_name[i],
				panel_data->props.mcd_rs_flash_range[i][1],
				(i != MAX_MCD_RS - 1) ? " " : "\n");

	for (i = 0; i < MAX_MCD_RS; i++)
		len += snprintf(buf + len, PAGE_SIZE - len,
				"%s:%d%s", mcd_rs_name[i],
				panel_data->props.mcd_rs_range[i][1],
				(i != MAX_MCD_RS - 1) ? " " : "\n");
	mutex_unlock(&panel->op_lock);

	return len;
}

static ssize_t mcd_resistance_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int i, value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
#ifdef CONFIG_SUPPORT_DDI_FLASH
	u8 flash_mcd[8];
#endif

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (!!value) {
		mutex_lock(&panel->op_lock);
		/* clear variable */
		memset(panel_data->props.mcd_rs_range, -1,
				sizeof(panel_data->props.mcd_rs_range));

		ret = read_mcd_resistance(panel);
		mutex_unlock(&panel->op_lock);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to check mcd resistance\n", __func__);
			return ret;
		}

		for (i = 0; i < MAX_MCD_RS; i++)
			pr_info("%s %s:(%d, %d)\n", __func__, mcd_rs_name[i],
					panel_data->props.mcd_rs_range[i][0],
					panel_data->props.mcd_rs_range[i][1]);

#ifdef CONFIG_SUPPORT_DDI_FLASH
		panel_wake_lock(panel);
		ret = set_panel_poc(&panel->poc_dev, POC_OP_MCD_READ, NULL);
		if (unlikely(ret)) {
			pr_err("%s, failed to read mcd(ret %d)\n",
					__func__, ret);
			panel_wake_unlock(panel);
			return ret;
		}
		panel_wake_unlock(panel);
		ret = panel_resource_update_by_name(panel, "flash_mcd");
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to update flash_mcd res (ret %d)\n",
					__func__, ret);
			return ret;
		}

		ret = resource_copy_by_name(&panel->panel_data, flash_mcd, "flash_mcd");
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to copy flash_mcd res (ret %d)\n",
					__func__, ret);
			return ret;
		}

		panel_data->props.mcd_rs_flash_range[MCD_RS_1_RIGHT][1] = flash_mcd[0];
		panel_data->props.mcd_rs_flash_range[MCD_RS_2_RIGHT][1] = flash_mcd[1];
		panel_data->props.mcd_rs_flash_range[MCD_RS_1_LEFT][1] = flash_mcd[4];
		panel_data->props.mcd_rs_flash_range[MCD_RS_2_LEFT][1] = flash_mcd[5];
		for (i = 0; i < MAX_MCD_RS; i++)
			pr_info("SDC_%s %s:(%d, %d)\n", __func__, mcd_rs_name[i],
					panel_data->props.mcd_rs_flash_range[i][0],
					panel_data->props.mcd_rs_flash_range[i][1]);
#endif
	}

	return size;
}

static ssize_t irc_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n", panel_data->props.irc_mode);

	return strlen(buf);
}

static ssize_t irc_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	mutex_lock(&panel->op_lock);
	panel_data->props.irc_mode = !!value;
	mutex_unlock(&panel->op_lock);
	panel_update_brightness(panel);
	dev_info(dev, "%s, irc_mode %s\n",
			__func__, panel_data->props.irc_mode ? "on" : "off");

	return size;
}

static ssize_t dia_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int len = 0;

	return len;
}

static ssize_t dia_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	mutex_lock(&panel->op_lock);
	panel_data->props.dia_mode = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel, PANEL_DIA_ONOFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mcd seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, set %s\n",
			__func__, panel_data->props.dia_mode ? "on" : "off");
	return size;
}


static ssize_t partial_disp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.panel_partial_disp);

	return strlen(buf);
}

static ssize_t partial_disp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;
	if (panel_data->props.panel_partial_disp == -1) {
		panel_warn("PANEL:WARN:%s do not support\n", __func__);
		return -EINVAL;
	}

	if ((panel->state.cur_state != PANEL_STATE_ALPM) && (value != 0)) {
		panel_warn("PANEL:WARN:%s panel state is Normal(state:%d, value:%d)\n",
			__func__, panel->state.cur_state, value);
		return -EINVAL;
	}

	panel_wake_lock(panel);
	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_PARTIAL_DISP_ON_SEQ : PANEL_PARTIAL_DISP_OFF_SEQ);
	panel_wake_unlock(panel);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write partial_disp seq %d\n", __func__, value);
		return ret;
	}
	mutex_lock(&panel->op_lock);
	dev_info(dev, "%s, prev set %s(%d), cur set %s(%d)\n", __func__,
		panel_data->props.panel_partial_disp ? "on" : "off", panel_data->props.panel_partial_disp,
		value ? "on" : "off", value);
	panel_data->props.panel_partial_disp = value;
	mutex_unlock(&panel->op_lock);
	return size;
}

static void prepare_self_mask_check(struct panel_device *panel)
{
	decon_bypass_on_global(0);
	disable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);
}

static void clear_self_mask_check(struct panel_device *panel)
{
	clear_pending_bit(panel->gpio[PANEL_GPIO_DISP_DET].irq);
	enable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);
	decon_bypass_off_global(0);
}

static ssize_t self_mask_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct aod_dev_info *aod;
	struct panel_info *panel_data;
	u8 success_check = 1;
	u8 *recv_checksum = NULL;
	int ret = 0, i = 0;
	int len = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	panel_data = &panel->panel_data;

	if (aod->props.self_mask_checksum_len) {
		recv_checksum = kmalloc_array(aod->props.self_mask_checksum_len, sizeof(u8), GFP_KERNEL);
		if (!recv_checksum) {
			panel_err("PANEL:ERR:%s:failed to mem alloc\n", __func__);
			return -ENOMEM;
		}
		prepare_self_mask_check(panel);

		ret = panel_do_aod_seqtbl_by_index(aod, SELF_MASK_CHECKSUM_SEQ);
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s:failed to send cmd selfmask checksum\n", __func__);
			kfree(recv_checksum);
			return ret;
		}

		ret = resource_copy_n_clear_by_name(panel_data,	recv_checksum, "self_mask_checksum");
		if (unlikely(ret < 0)) {
			panel_err("PANEL:ERR:%s:failed to get selfmask checksum\n", __func__);
			kfree(recv_checksum);
			return ret;
		}
		clear_self_mask_check(panel);

		for (i = 0; i < aod->props.self_mask_checksum_len; i++) {
			if (aod->props.self_mask_checksum[i] != recv_checksum[i]) {
				success_check = 0;
				break;
			}
		}
		len = snprintf(buf, PAGE_SIZE, "%d", success_check);
		for (i = 0; i < aod->props.self_mask_checksum_len; i++)
			len += snprintf(buf + len, PAGE_SIZE - len, " %02x", recv_checksum[i]);
		len += snprintf(buf + len, PAGE_SIZE - len, "\n", recv_checksum[i]);
		kfree(recv_checksum);
	} else {
		snprintf(buf, PAGE_SIZE, "-1\n");
	}
	return strlen(buf);
}


#ifdef CONFIG_SUPPORT_MST
static ssize_t mst_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.mst_on);

	return strlen(buf);
}

static ssize_t mst_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.mst_on == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.mst_on = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_MST_ON_SEQ : PANEL_MST_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write mst seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, mst %s\n",
			__func__, panel_data->props.mst_on ? "on" : "off");

	return size;
}
#endif


#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
u8 checksum[4] = { 0x12, 0x34, 0x56, 0x78 };
static bool gct_chksum_is_valid(struct panel_device *panel)
{
	int i;
	struct panel_info *panel_data = &panel->panel_data;

	for (i = 0; i < 4; i++)
		if (checksum[i] != panel_data->props.gct_valid_chksum)
			return false;
	return true;
}

static void prepare_gct_mode(struct panel_device *panel)
{
	decon_bypass_on_global(0);
	usleep_range(90000, 100000);
	disable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);
}

static void clear_gct_mode(struct panel_device *panel)
{
	struct panel_info *panel_data = &panel->panel_data;
	int ret;

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_EXIT_SEQ);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed exit-seq\n", __func__);

	ret = __set_panel_power(panel, PANEL_POWER_OFF);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set power off\n", __func__);

	ret = __set_panel_power(panel, PANEL_POWER_ON);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed to set power on\n", __func__);

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_INIT_SEQ);
	if (ret < 0)
		panel_err("PANEL:ERR:%s:failed init-seq\n", __func__);

	clear_pending_bit(panel->gpio[PANEL_GPIO_DISP_DET].irq);
	enable_irq(panel->gpio[PANEL_GPIO_DISP_DET].irq);
	panel_data->props.gct_on = GRAM_TEST_OFF;
	panel->state.cur_state = PANEL_STATE_NORMAL;
	panel->state.disp_on = PANEL_DISPLAY_OFF;
	decon_bypass_off_global(0);
	msleep(20);
}

static ssize_t gct_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u 0x%x\n",
			gct_chksum_is_valid(panel) ? 1 : 0,
			*(u32 *)checksum);

	return strlen(buf);
}

static ssize_t gct_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret, result = 0;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct seqinfo *seqtbl;
	int i, index = 0, vddm = 0, pattern = 0;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value != GRAM_TEST_ON)
		return -EINVAL;

	/* clear checksum buffer */
	checksum[0] = 0x12;
	checksum[1] = 0x34;
	checksum[2] = 0x56;
	checksum[3] = 0x78;

	mutex_lock(&panel->io_lock);
	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		mutex_unlock(&panel->io_lock);
		return -EAGAIN;
	}

	if (panel->state.cur_state == PANEL_STATE_ALPM) {
		pr_warn("%s gct not supported on LPM\n", __func__);
		mutex_unlock(&panel->io_lock);
		return -EINVAL;
	}

	copr_disable(&panel->copr);
	mdnie_disable(&panel->mdnie);

	mutex_lock(&panel->mdnie.lock);
	mutex_lock(&panel->op_lock);
	prepare_gct_mode(panel);
	panel_data->props.gct_on = value;

#ifdef CONFIG_SUPPORT_AFC
	if (panel->mdnie.props.afc_on &&
			panel->mdnie.nr_seqtbl > MDNIE_AFC_OFF_SEQ) {
		pr_info("%s afc off\n", __func__);
		ret = panel_do_seqtbl(panel, &panel->mdnie.seqtbl[MDNIE_AFC_OFF_SEQ]);
		if (unlikely(ret < 0))
			pr_err("%s, failed to write afc off seqtbl\n", __func__);
	}
#endif

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_GCT_ENTER_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write gram-checksum-test-enter seq\n", __func__);
		result = ret;
		goto out;
	}

	for (vddm = VDDM_LV; vddm < MAX_VDDM; vddm++) {
		panel_data->props.gct_vddm = vddm;
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_GCT_VDDM_SEQ);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to write gram-checksum-on seq\n", __func__);
			result = ret;
			goto out;
		}

		for (pattern = GCT_PATTERN_1; pattern < MAX_GCT_PATTERN; pattern++) {
			panel_data->props.gct_pattern = pattern;
			seqtbl = find_index_seqtbl(&panel->panel_data,
					PANEL_GCT_IMG_UPDATE_SEQ);
			ret = panel_do_seqtbl_by_index_nolock(panel,
					(seqtbl && seqtbl->cmdtbl) ? PANEL_GCT_IMG_UPDATE_SEQ :
					((pattern == GCT_PATTERN_1) ?
					 PANEL_GCT_IMG_0_UPDATE_SEQ : PANEL_GCT_IMG_1_UPDATE_SEQ));
			if (unlikely(ret < 0)) {
				pr_err("%s, failed to write gram-img-update seq\n", __func__);
				result = ret;
				goto out;
			}

			ret = resource_copy_n_clear_by_name(panel_data,
					&checksum[index], "gram_checksum");
			if (unlikely(ret < 0)) {
				pr_err("%s failed to copy gram_checksum[%d] (ret %d)\n",
						__func__, index, ret);
				result = ret;
				goto out;
			}
			pr_info("%s gram_checksum[%d] 0x%x\n",
					__func__, index, checksum[index]);
			index++;
		}
	}

	ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_GCT_EXIT_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write gram-checksum-off seq\n", __func__);
		result = ret;
	}
out:
	clear_gct_mode(panel);
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel->mdnie.lock);
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	ret = panel_aod_init_panel(panel);
	if (ret)
		panel_err("PANEL:ERR:%s:failed to aod init_panel\n", __func__);
#endif
	mutex_unlock(&panel->io_lock);

	for (i = 0; i < 20; i++) {
		if (panel->state.disp_on == PANEL_DISPLAY_ON)
			break;
		msleep(50);
	}

	if (i == 20) {
		panel_info("%s display on\n", __func__);
		ret = panel_display_on(panel);
		if (ret < 0)
			panel_err("PANEL:ERR:%s:failed to display on\n", __func__);
	}

	if (result < 0)
		return result;

	pr_info("%s chksum %s 0x%0x\n", __func__,
			gct_chksum_is_valid(panel) ? "ok" : "nok",
			*(u32 *)checksum);

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_XTALK_MODE
static ssize_t xtalk_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.xtalk_mode);

	return strlen(buf);
}

static ssize_t xtalk_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_info *panel_data;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	panel_bl = &panel->panel_bl;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.xtalk_mode == value)
		return size;

	mutex_lock(&panel_bl->lock);
	panel_data->props.xtalk_mode = value;
	mutex_unlock(&panel_bl->lock);
	panel_update_brightness(panel);

	dev_info(dev, "%s, xtalk_mode %d\n",
			__func__, panel_data->props.xtalk_mode);

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_POC_FLASH
static ssize_t poc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;
	int ret;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return -EAGAIN;
	}

	poc_dev = &panel->poc_dev;
	poc_info = &poc_dev->poc_info;

	panel_wake_lock(panel);
	ret = set_panel_poc(poc_dev, POC_OP_CHECKPOC, NULL);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to chkpoc (ret %d)\n", __func__, ret);
		panel_wake_unlock(panel);
		return ret;
	}

	ret = set_panel_poc(poc_dev, POC_OP_CHECKSUM, NULL);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to chksum (ret %d)\n", __func__, ret);
		panel_wake_unlock(panel);
		return ret;
	}
	panel_wake_unlock(panel);

	snprintf(buf, PAGE_SIZE, "%d %d %02x\n", poc_info->poc,
			poc_info->poc_chksum[4], poc_info->poc_ctrl[3]);

	dev_info(dev, "%s poc:%d chk:%d gray:%02x\n", __func__, poc_info->poc,
			poc_info->poc_chksum[4], poc_info->poc_ctrl[3]);

	return strlen(buf);
}

static ssize_t poc_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data;
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_spi_dev *spi_dev = &panel->panel_spi_dev;
#endif
	int rc, ret;
	unsigned int value;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;
	poc_dev = &panel->poc_dev;
	poc_info = &poc_dev->poc_info;

	rc = sscanf(buf, "%d", &value);
	if (rc < 1) {
		pr_err("%s poc_op required\n", __func__);
		return -EINVAL;
	}

	if (!IS_VALID_POC_OP(value)) {
		pr_warn("%s invalid poc_op %d\n", __func__, value);
		return -EINVAL;
	}

	if (value == POC_OP_WRITE || value == POC_OP_READ) {
		pr_warn("%s unsupported poc_op %d\n", __func__, value);
		return size;
	}

#ifdef CONFIG_SUPPORT_POC_SPI
	if (value == POC_OP_SET_SPI_SPEED) {
		rc = sscanf(buf, "%*d %d", &value);
		if (rc < 1) {
			pr_warn("%s SET_SPI_SPEED need 2 params\n", __func__);
			return -EINVAL;
		}
		spi_dev->speed_hz = value;
		value = POC_OP_SET_SPI_SPEED;
		return size;
	}
#endif

	if (value == POC_OP_CANCEL) {
		atomic_set(&poc_dev->cancel, 1);
	} else {
		panel_wake_lock(panel);
		mutex_lock(&panel->io_lock);
		ret = set_panel_poc(poc_dev, value, buf);
		if (unlikely(ret < 0)) {
			pr_err("%s, failed to poc_op %d(ret %d)\n", __func__, value, ret);
			mutex_unlock(&panel->io_lock);
			panel_wake_unlock(panel);
			return -EINVAL;
		}
		mutex_unlock(&panel->io_lock);
		panel_wake_unlock(panel);
	}

	mutex_lock(&panel->op_lock);
	panel_data->props.poc_op = value;
	mutex_unlock(&panel->op_lock);

	dev_info(dev, "%s poc_op %d\n",
			__func__, panel_data->props.poc_op);

	return size;
}

static ssize_t poc_mca_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	int ret;
	u8 chksum_data[256];
	int i, len;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return -EAGAIN;
	}

	panel_set_key(panel, 2, true);
	ret = panel_resource_update_by_name(panel, "poc_mca_chksum");
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to update poc_mca_chksum res (ret %d)\n",
				__func__, ret);
		return ret;
	}
	panel_set_key(panel, 2, false);

	ret = resource_copy_by_name(&panel->panel_data, chksum_data, "poc_mca_chksum");
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to copy poc_mca_chksum res (ret %d)\n",
				__func__, ret);
		return ret;
	}

	len = get_resource_size_by_name(&panel->panel_data, "poc_mca_chksum");
	buf[0] = '\0';
	for (i = 0; i < len; i++)
		snprintf(buf, PAGE_SIZE, "%s%02X ", buf, chksum_data[i]);

	dev_info(dev, "%s poc_mca_checksum: %s\n", __func__, buf);

	return strlen(buf);
}

static ssize_t poc_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_poc_device *poc_dev;
	struct panel_poc_info *poc_info;
	int ret;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	poc_dev = &panel->poc_dev;
	poc_info = &poc_dev->poc_info;

	ret = get_poc_partition_size(poc_dev, POC_IMG_PARTITION);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to get poc partition size (ret %d)\n", __func__, ret);
		return ret;
	}

	snprintf(buf, PAGE_SIZE, "poc_mca_image_size %d\n", ret);

	dev_info(dev, "%s: %s\n", __func__, buf);

	return strlen(buf);
}
#endif

#ifdef CONFIG_SUPPORT_DIM_FLASH
static u16 calc_checksum_16bit(u8 *arr, int size)
{
	u16 chksum = 0;
	int i;

	for (i = 0; i < size; i++)
		chksum += arr[i];

	return chksum;
}

/*
 * gamma_flash_show() function returns read state and checksum.
 * @read_state(-1 : FAILED, 0 : PROGRESS, 1 : DONE)
 * @checksum(XXXXXXXX XXXXXXXX)
 * The first checksum is by calculation of parameters
 * and the other one is by reading checksum parameter.
 */
static ssize_t gamma_flash_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct dim_flash_result *result = &panel->dim_flash_result;
	int ret;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	/* thread is running */
	if (atomic_read(&panel->work[PANEL_WORK_DIM_FLASH].running))
		ret = GAMMA_FLASH_PROGRESS;
	else
		ret = panel->work[PANEL_WORK_DIM_FLASH].ret;

	pr_info("%s result %d, dim chksum(calc:%04X read:%04X), mtp chksum(reg:%04X, calc:%04X, read:%04X)\n",
			__func__, ret, result->dim_chksum_by_calc, result->dim_chksum_by_read,
			calc_checksum_16bit(result->mtp_reg, sizeof(result->mtp_reg)),
			result->mtp_chksum_by_calc, result->mtp_chksum_by_read);

	return snprintf(buf, PAGE_SIZE, "%d %08X %08X %08X %08X %08X\n",
			ret, result->dim_chksum_by_calc, result->dim_chksum_by_read,
			calc_checksum_16bit(result->mtp_reg, sizeof(result->mtp_reg)),
			result->mtp_chksum_by_calc, result->mtp_chksum_by_read);
}

static ssize_t gamma_flash_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	int rc;
	unsigned int value;

	if (!IS_PANEL_ACTIVE(panel))
		return -ENODEV;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (atomic_read(&panel->work[PANEL_WORK_DIM_FLASH].running))
		return -EBUSY;

	panel->work[PANEL_WORK_DIM_FLASH].ret = GAMMA_FLASH_ERROR_NOT_EXIST;
	memset(&panel->dim_flash_result, 0, sizeof(struct dim_flash_result));
	if (value == 0) {
		panel_update_dim_type(panel, DIM_TYPE_AID_DIMMING);
		panel_update_brightness(panel);
	} else if (value == 1) {
		queue_delayed_work(panel->work[PANEL_WORK_DIM_FLASH].wq,
				&panel->work[PANEL_WORK_DIM_FLASH].dwork, msecs_to_jiffies(0));
	}

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
static ssize_t grayspot_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.grayspot);

	return strlen(buf);
}

static ssize_t grayspot_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (panel_data->props.grayspot == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.grayspot = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_GRAYSPOT_ON_SEQ : PANEL_GRAYSPOT_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write grayspot on/off seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, grayspot %s\n",
			__func__, panel_data->props.grayspot ? "on" : "off");

	return size;
}
#endif

#ifdef CONFIG_SUPPORT_HMD
static ssize_t hmt_bright_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = dev_get_drvdata(dev);
	int size;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	mutex_lock(&panel_bl->lock);
	if (panel_bl->props.id == PANEL_BL_SUBDEV_TYPE_DISP) {
		size = snprintf(buf, 30, "HMD off state\n");
	} else {
		size = snprintf(buf, PAGE_SIZE, "index : %d, brightenss : %d\n",
				panel_bl->props.actual_brightness_index,
				BRT_USER(panel_bl->props.brightness));
	}
	mutex_unlock(&panel_bl->lock);
	return size;
}

static ssize_t hmt_bright_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int value, rc;
	struct panel_bl_device *panel_bl;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_bl = &panel->panel_bl;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	panel_info("PANEL:INFO:%s: brightness : %d\n",
			__func__, value);

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);

	if (panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_HMD].brightness != BRT(value))
		panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_HMD].brightness = BRT(value);

	if (panel->state.hmd_on != PANEL_HMD_ON) {
		panel_info("PANEL:WARN:%s: hmd off\n", __func__);
		goto exit_store;
	}

	ret = panel_bl_set_brightness(panel_bl, PANEL_BL_SUBDEV_TYPE_HMD, 1);
	if (ret) {
		pr_err("%s : fail to set brightness\n", __func__);
		goto exit_store;
	}

exit_store:
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);
	return size;
}

static ssize_t hmt_on_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_state *state;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	state = &panel->state;

	snprintf(buf, PAGE_SIZE, "%u\n", state->hmd_on);

	return strlen(buf);
}

static ssize_t hmt_on_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int value, rc;
	struct backlight_device *bd;
	struct panel_state *state;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_bl = &panel->panel_bl;
	bd = panel_bl->bd;
	state = &panel->state;

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value != PANEL_HMD_OFF && value != PANEL_HMD_ON) {
		pr_err("%s invalid parameter %d\n", __func__, value);
		return -EINVAL;
	}

	panel_info("PANEL:INFO:%s: hmt %s\n",
			__func__, (value == PANEL_HMD_ON) ? "on" : "off");

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);

	if (value == state->hmd_on) {
		panel_warn("PANEL:WARN:%s:already set : %d\n",
			__func__, value);
	} else {
		ret = panel_do_seqtbl_by_index_nolock(panel,
				(value == PANEL_HMD_ON) ? PANEL_HMD_ON_SEQ : PANEL_HMD_OFF_SEQ);
		if (ret < 0)
			pr_err("%s failed to set hmd %s seq\n", __func__,
					(value == PANEL_HMD_ON) ? "on" : "off");
	}

	ret = panel_bl_set_brightness(panel_bl, (value == PANEL_HMD_ON) ?
			PANEL_BL_SUBDEV_TYPE_HMD : PANEL_BL_SUBDEV_TYPE_DISP, 1);
	if (ret < 0)
		pr_err("%s failed to set %s brightness\n", __func__,
					(value == PANEL_HMD_ON) ? "hmd" : "normal");
	state->hmd_on = value;

	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

	return size;
}
#endif /* CONFIG_SUPPORT_HMD */

#ifdef CONFIG_SUPPORT_DOZE
static int set_alpm_mode(struct panel_device *panel, int mode)
{
	int ret = 0;
#ifdef CONFIG_SUPPORT_AOD_BL
	int lpm_ver = (mode & 0x00FF0000) >> 16;
#endif
	int lpm_mode = (mode & 0xFF);
#ifdef CONFIG_SEC_FACTORY
	static int backup_br;
#endif
	struct panel_info *panel_data = &panel->panel_data;
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct backlight_device *bd = panel_bl->bd;

	switch (lpm_mode) {
	case ALPM_OFF:
		mutex_lock(&panel_bl->lock);
		mutex_lock(&panel->op_lock);
#ifdef CONFIG_SEC_FACTORY
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_EXIT_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s, failed to alpm-exit\n", __func__);
#endif
		panel_data->props.alpm_mode = lpm_mode;
#ifdef CONFIG_SUPPORT_AOD_BL
		panel->panel_data.props.lpm_brightness = -1;
		panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP);
#endif
#ifdef CONFIG_SEC_FACTORY
		if (backup_br)
			bd->props.brightness = backup_br;
#endif
		mutex_unlock(&panel->op_lock);
		mutex_unlock(&panel_bl->lock);
		panel_update_brightness(panel);
#ifdef CONFIG_SEC_FACTORY
		msleep(34);
#endif
		break;
	case ALPM_LOW_BR:
	case HLPM_LOW_BR:
	case ALPM_HIGH_BR:
	case HLPM_HIGH_BR:
		mutex_lock(&panel_bl->lock);
#ifdef CONFIG_SEC_FACTORY
		if (panel_data->props.alpm_mode == ALPM_OFF) {
			backup_br = bd->props.brightness;
			bd->props.brightness = UI_MIN_BRIGHTNESS;
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_DISP].brightness =
				UI_MIN_BRIGHTNESS;
			ret = panel_bl_set_brightness(panel_bl,
					PANEL_BL_SUBDEV_TYPE_DISP, 1);
			if (ret)
				pr_err("%s, failed to set_brightness (ret %d)\n",
						__func__, ret);
		}
#endif
		mutex_lock(&panel->op_lock);
		panel_data->props.alpm_mode = lpm_mode;

#ifndef CONFIG_SEC_FACTORY
		if (panel->state.cur_state != PANEL_STATE_ALPM) {
			panel_info("PANEL:INFO:%s:panel state(%d) is not lpm mode\n",
					__func__, panel->state.cur_state);
			mutex_unlock(&panel->op_lock);
			mutex_unlock(&panel_bl->lock);
			return ret;
		}
#endif
#ifdef CONFIG_SUPPORT_AOD_BL
		if (lpm_ver == 0) {
			bd->props.brightness =
				(lpm_mode <= HLPM_LOW_BR) ? BRT(0) : BRT(94);
			panel_bl->subdev[PANEL_BL_SUBDEV_TYPE_AOD].brightness =
				(lpm_mode <= HLPM_LOW_BR) ? BRT(0) : BRT(94);
		}
#endif
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_ALPM_ENTER_SEQ);
		if (ret)
			panel_err("PANEL:ERR:%s, failed to alpm-enter\n", __func__);
#ifdef CONFIG_SUPPORT_AOD_BL
		panel_bl_set_subdev(panel_bl, PANEL_BL_SUBDEV_TYPE_AOD);
#endif
		mutex_unlock(&panel->op_lock);
		mutex_unlock(&panel_bl->lock);
		break;
	default:
		panel_err("PANEL:ERR:%s:invalid alpm_mode: %d\n",
				__func__, lpm_mode);
		break;
	}

	return ret;
}
#endif

static ssize_t alpm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->io_lock);
	rc = kstrtoint(buf, 0, &value);
	if (rc < 0) {
		pr_warn("%s invalid param (ret %d)\n",
				__func__, rc);
		mutex_unlock(&panel->io_lock);
		return rc;
	}

#ifdef CONFIG_SUPPORT_DOZE
	rc = set_alpm_mode(panel, value);
	if (rc) {
		panel_err("PANEL:ERR:%s:failed to set alpm (value %d, ret %d)\n",
				__func__, value, rc);
		goto exit_store;
	}
#endif
	panel_info("PANEL:INFO:%s:value %d, alpm_mode %d\n",
			__func__, value, panel_data->props.alpm_mode);

#ifdef CONFIG_SUPPORT_DOZE
exit_store:
#endif
	mutex_unlock(&panel->io_lock);

	return size;
}

static ssize_t alpm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.alpm_mode);
	panel_dbg("%s: %d\n", __func__, panel_data->props.alpm_mode);

	return strlen(buf);
}

static ssize_t conn_det_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0) {
		pr_warn("%s invalid param (ret %d)\n",
				__func__, rc);
		return rc;
	}

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	if (!panel_gpio_valid(&panel->gpio[PANEL_GPIO_CONN_DET])) {
		panel_err("%s conn det is unsupported\n", __func__);
		return -EINVAL;
	}

	if (panel->panel_data.props.conn_det_enable != value) {
		panel->panel_data.props.conn_det_enable = value;
		pr_info("%s set %d %s\n",
			__func__, panel->panel_data.props.conn_det_enable,
			panel->panel_data.props.conn_det_enable ? "enable" : "disable");
		if (panel->panel_data.props.conn_det_enable) {
			if (ub_con_disconnected(panel))
				panel_send_ubconn_uevent(panel);
		}
	} else {
		pr_info("%s already set %d %s\n",
			__func__, panel->panel_data.props.conn_det_enable,
			panel->panel_data.props.conn_det_enable ? "enable" : "disable");
	}
	return size;
}

static ssize_t conn_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (panel_gpio_valid(&panel->gpio[PANEL_GPIO_CONN_DET]))
		snprintf(buf, PAGE_SIZE, "%s\n", ub_con_disconnected(panel) ? "disconnected" : "connected");
	else
		snprintf(buf, PAGE_SIZE, "%d\n", -1);
	pr_info("%s %s", __func__, buf);
	return strlen(buf);
}

static ssize_t lpm_opr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_info *panel_data = &panel->panel_data;

	mutex_lock(&panel->io_lock);
	rc = kstrtoint(buf, 0, &value);
	if (rc < 0) {
		pr_warn("%s invalid param (ret %d)\n",
				__func__, rc);
		mutex_unlock(&panel->io_lock);
		return rc;
	}

	mutex_lock(&panel->op_lock);
	panel_data->props.lpm_opr = value;
	mutex_unlock(&panel->op_lock);
	panel_update_brightness(panel);

	panel_info("PANEL:INFO:%s:value %d, lpm_opr %d\n",
			__func__, value, panel_data->props.lpm_opr);

	mutex_unlock(&panel->io_lock);
	return size;
}

static ssize_t lpm_opr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.lpm_opr);

	return strlen(buf);
}

#ifdef CONFIG_PANEL_AID_DIMMING_DEBUG
char *normal_tbl_names[] = {
	"gamma_table",
	"aor_table",
	"vint_table",
	"elvss_table",
	"irc_table",
};
char *hmd_tbl_names[] = {
	"hmd_gamma_table",
	"hmd_aor_table",
};

char *brt_res_names[] = {
	"gamma",
	"aor",
	"vint",
	"elvss_t",
	"irc",
};
char *line[4096];

static int buffer_backup(u8 *buf, int size, char *name)
{
	struct file *fp;
	mm_segment_t old_fs;

	if (!name)
		return -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pr_info("%s filename %s size %d\n", __func__, name, size);
	fp = filp_open(name, O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0660);
	if (IS_ERR(fp)) {
		pr_err("%s, fail to open %s file\n", __func__, name);
		goto open_err;
	}

	vfs_write(fp, (u8 __user *)buf, size, &fp->f_pos);
	pr_info("%s filename %s write %d bytes done!!\n",
			__func__, name, size);

	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;

 open_err:
	set_fs(old_fs);
	return -1;
}

static void show_brt_param(struct panel_info *panel_data, int id, int type)
{
	char reg_val[256];
	char *buf;
	int i, num, size, count = 0, len = 0, ret = 0;
	size_t ires, itemp;
	struct resinfo *res;
	int sz_tbl;
	u32 *tbl;
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_bl_sub_dev *subdev;
	int orig_temperature, temperatures[] = { 23, 0, -15 };
	const char * const path[] = {
		"/data/brightness.csv",
		"/data/hmd_brightness.csv",
		"/data/aod_brightness.csv"
	};

	if (unlikely(!panel_data)) {
		panel_err("%s:panel is NULL\n", __func__);
		return;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return;
	}

	subdev = &panel_bl->subdev[id];

	if (type == 1) {
		/* brightness mode table */
		tbl = subdev->brt_tbl.brt;
		sz_tbl = subdev->brt_tbl.sz_brt;
	} else if (type == 2) {
		/* detail brightness mode table */
		tbl = subdev->brt_tbl.brt_to_step;
		sz_tbl = subdev->brt_tbl.sz_brt_to_step;
	}

	/* columns */
	buf = kmalloc(SZ_1K, GFP_KERNEL);
	if (!buf)
		return;

	line[count++] = buf;
	len = snprintf(buf, SZ_1K, "Brightness");
	for (ires = 0; ires < ARRAY_SIZE(brt_res_names); ires++) {
		res = find_panel_resource(panel_data, brt_res_names[ires]);
		size = get_panel_resource_size(res);
		if (!strncmp(brt_res_names[ires], "elvss_t", 8))
			for (itemp = 0; itemp < ARRAY_SIZE(temperatures); itemp++)
				len += snprintf(buf + len, SZ_1K - len, ",%selvss(T:%d)",
						(id == PANEL_BL_SUBDEV_TYPE_HMD) ? "hmd_" : "",
						temperatures[itemp]);
		else
			for (num = 0; num < size; num++)
				len += snprintf(buf + len, SZ_1K - len, ",%s%s_%d",
						(id == PANEL_BL_SUBDEV_TYPE_HMD) ? "hmd_" : "",
						brt_res_names[ires], (1 + num + res->resui->rditbl->offset));
	}

	mutex_lock(&panel_bl->lock);
	mutex_lock(&panel->op_lock);
#ifdef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_HMD_ON_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s:failed to write hmd-on seq\n", __func__);
	}
#endif

	for (i = 0; i < sz_tbl; i++) {
		panel_bl->bd->props.brightness = tbl[i];
		subdev->brightness = tbl[i];
		ret = panel_bl_set_brightness(panel_bl, id, 1);
		if (ret < 0) {
			pr_err("%s, failed to set_brightness (ret %d)\n",
					__func__, ret);
			break;
		}

		buf = kmalloc(SZ_1K, GFP_KERNEL);
		if (!buf)
			break;

		line[count++] = buf;
		len = snprintf(buf, SZ_1K, "%3d", tbl[i]);

		panel_set_key(panel, 3, true);
		/* store temperature */
		orig_temperature = panel_data->props.temperature;
		for (ires = 0; ires < ARRAY_SIZE(brt_res_names); ires++) {
			res = find_panel_resource(panel_data, brt_res_names[ires]);
			size = get_panel_resource_size(res);
			if (!strncmp(brt_res_names[ires], "elvss_t", 8)) {
				for (itemp = 0; itemp < ARRAY_SIZE(temperatures); itemp++) {
					/* update temperature */
					panel_data->props.temperature = temperatures[itemp];
					ret = panel_bl_set_brightness(panel_bl, id, 1);
					if (ret < 0) {
						pr_err("%s, failed to set_brightness (ret %d)\n",
								__func__, ret);
						break;
					}

					ret = panel_resource_update(panel, res);
					if (ret < 0) {
						pr_err("%s, failed to resource_update (ret %d)\n",
								__func__, ret);
						break;
					}

					resource_copy(reg_val, res);
					len += snprintf(buf + len, SZ_1K - len, ",0x%02X",
							reg_val[0]);
				}
			} else {
				ret = panel_resource_update(panel, res);
				if (ret < 0) {
					pr_err("%s, failed to resource_update (ret %d)\n",
							__func__, ret);
					break;
				}
				resource_copy(reg_val, res);
				for (num = 0; num < size; num++)
					len += snprintf(buf + len, SZ_1K - len, ",0x%02X",
							reg_val[num]);
			}
		}
		/* restore temperature */
		panel_data->props.temperature = orig_temperature;
		panel_set_key(panel, 3, false);

		if (ret < 0)
			break;
	}

#ifdef CONFIG_SUPPORT_HMD
	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_HMD_OFF_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s:failed to write hmd-off seq\n", __func__);

		ret = panel_bl_set_brightness(panel_bl, PANEL_BL_SUBDEV_TYPE_DISP, 1);
		if (unlikely(ret < 0))
			pr_err("%s, failed to set_brightness (ret %d)\n", __func__, ret);
	}
#endif

	mutex_unlock(&panel->op_lock);
	mutex_unlock(&panel_bl->lock);

	buf = kmalloc(SZ_1K * count, GFP_KERNEL);
	if (buf) {
		len = 0;
		for (i = 0; i < count; i++)
			len += snprintf(buf + len,
					SZ_1K * count - len, "%s\n", line[i]);
		buffer_backup(buf, len, (char *)path[id]);
		kfree(buf);
	}

	for (i = 0; i < count; i++) {
		pr_info("brt_param:%s\n", line[i]);
		kfree(line[i]);
	}
}

#ifdef CONFIG_SUPPORT_DIM_FLASH
static void show_aid_log(struct panel_info *panel_data, int id)
{
	struct dimming_info *dim_info;
	struct maptbl *tbl = NULL;
	int layer, row, col, i, len = 0;
	char *buf;
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_bl_sub_dev *subdev;
	char **tbl_names;
	int count = 0, nr_tbl_names;

	if (unlikely(!panel_data)) {
		panel_err("%s:panel is NULL\n", __func__);
		return;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return;
	}

	subdev = &panel_bl->subdev[id];

	dim_info = panel_data->dim_info[id];
	if (!dim_info) {
		panel_warn("%s bl-%d dim_info is null\n", __func__, id);
		return;
	}

	pr_info("[====================== [%s] ======================]\n",
			(id == PANEL_BL_SUBDEV_TYPE_HMD ? "HMD" : "DISP"));
	print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
	print_dimming_info(dim_info, TAG_GAMMA_CENTER_START);

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		tbl_names = hmd_tbl_names;
		nr_tbl_names = ARRAY_SIZE(hmd_tbl_names);
	} else {
		tbl_names = normal_tbl_names;
		nr_tbl_names = ARRAY_SIZE(normal_tbl_names);
	}

	for (i = 0; i < nr_tbl_names; i++) {
		tbl = find_panel_maptbl_by_name(panel_data, tbl_names[i]);
		buf = kmalloc(SZ_1K, GFP_KERNEL);
		if (!buf) {
			pr_err("%s failed to alloc\n", __func__);
			goto out;
		}

		line[count++] = buf;
		len = snprintf(buf, SZ_1K, "[MAPTBL:%s]", tbl_names[i]);
		for_each_layer(tbl, layer) {
			for_each_row(tbl, row) {
				buf = kmalloc(SZ_1K, GFP_KERNEL);
				if (!buf) {
					pr_err("%s failed to alloc\n", __func__);
					goto out;
				}

				line[count++] = buf;
				len = snprintf(buf, SZ_1K, "lum[%3d] : ",
						subdev->brt_tbl.lum[row]);
				for_each_col(tbl, col)
					len += snprintf(buf + len, SZ_1K - len, "%02X ",
							tbl->arr[row * sizeof_row(tbl) + col]);
			}
		}
	}

out:
	for (i = 0; i < count; i++) {
		pr_info("%s\n", line[i]);
		kfree(line[i]);
	}
}
#else
static void show_aid_log(struct panel_info *panel_data, int id)
{
	struct dimming_info *dim_info;
	struct maptbl *tbl = NULL, *aor_tbl = NULL;
	struct maptbl *irc_tbl = NULL, *elvss_tbl = NULL, *vint_tbl = NULL;
	int layer, row, col, i, len = 0;
	char buf[1024];
	struct panel_device *panel = to_panel_device(panel_data);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	struct panel_bl_sub_dev *subdev;

	if (unlikely(!panel_data)) {
		panel_err("%s:panel is NULL\n", __func__);
		return;
	}

	if (id >= MAX_PANEL_BL_SUBDEV) {
		panel_err("%s invalid bl-%d\n", __func__, id);
		return;
	}

	subdev = &panel_bl->subdev[id];

	pr_info("[====================== [%s] ======================]\n",
			(id == PANEL_BL_SUBDEV_TYPE_HMD ? "HMD" : "DISP"));
	dim_info = panel_data->dim_info[id];
	if (!dim_info) {
		panel_warn("%s bl-%d dim_info is null\n", __func__, id);
	} else {
		print_dimming_info(dim_info, TAG_MTP_OFFSET_START);
		print_dimming_info(dim_info, TAG_TP_VOLT_START);
		print_dimming_info(dim_info, TAG_GRAY_SCALE_VOLT_START);
		print_dimming_info(dim_info, TAG_GAMMA_CENTER_START);
	}

	/* TODO : 0 means GAMMA_MAPTBL.
	 * To use commonly in panel driver some maptbl index should be same
	 */
	if (id == PANEL_BL_SUBDEV_TYPE_HMD)
		tbl = find_panel_maptbl_by_name(panel_data, "hmd_gamma_table");
	else
		tbl = find_panel_maptbl_by_name(panel_data, "gamma_table");

	if (tbl) {
		pr_info("[GAMMA MAPTBL] HEXA-DECIMAL\n");
		for_each_layer(tbl, layer) {
			for_each_row(tbl, row) {
				len = snprintf(buf, sizeof(buf), "gamma[%3d] : ",
						subdev->brt_tbl.lum[row]);
				for_each_col(tbl, col)
					len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
							tbl->arr[row * sizeof_row(tbl) + col]);
				pr_info("%s\n", buf);
			}
		}
	}

	if (id == PANEL_BL_SUBDEV_TYPE_HMD) {
		aor_tbl = find_panel_maptbl_by_name(panel_data, "hmd_aor_table");
	} else {
		aor_tbl = find_panel_maptbl_by_name(panel_data, "aor_table");
		vint_tbl = find_panel_maptbl_by_name(panel_data, "vint_table");
		elvss_tbl = find_panel_maptbl_by_name(panel_data, "elvss_table");
		irc_tbl = find_panel_maptbl_by_name(panel_data, "irc_table");
	}

	panel_info("\n[BRIGHTNESS, %s %s %s %s table]\n",
			aor_tbl ? "AOR" : "", vint_tbl ? "VINT" : "",
			elvss_tbl ? "ELVSS" : "", irc_tbl ? "IRC" : "");
	len = snprintf(buf, sizeof(buf), "[idx] platform   luminance  ");
	if (aor_tbl)
		len += snprintf(buf + len, sizeof(buf) - len, "|  aor  ");
	if (vint_tbl)
		len += snprintf(buf + len, sizeof(buf) - len,
				"| vint ");
	if (elvss_tbl)
		len += snprintf(buf + len, sizeof(buf) - len,
				"|  elvss   ");
	if (irc_tbl)
		len += snprintf(buf + len, sizeof(buf) - len,
				"| ====================== irc =======================");
	pr_info("%s\n", buf);

	for (i = 0; i < subdev->brt_tbl.sz_brt_to_step; i++) {
		len = snprintf(buf, sizeof(buf),
				"[%3d]   %-5d   %3d(%3d.%02d) ",
				i, subdev->brt_tbl.brt_to_step[i],
				get_subdev_actual_brightness(panel_bl, id,
					subdev->brt_tbl.brt_to_step[i]),
				get_subdev_actual_brightness_interpolation(panel_bl, id,
					subdev->brt_tbl.brt_to_step[i]) / 100,
				get_subdev_actual_brightness_interpolation(panel_bl, id,
					subdev->brt_tbl.brt_to_step[i]) % 100);
		if (aor_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_col(aor_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						aor_tbl->arr[i * sizeof_row(aor_tbl) + col]);
		}

		if (vint_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_col(vint_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, " %02X  ",
						vint_tbl->arr[i * sizeof_row(vint_tbl) + col]);
		}

		if (elvss_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_layer(elvss_tbl, layer)
				for_each_col(elvss_tbl, col)
					len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						elvss_tbl->arr[maptbl_index(elvss_tbl, layer, i, col)]);
		}

		if (irc_tbl) {
			len += snprintf(buf + len, sizeof(buf) - len, "| ");
			for_each_col(irc_tbl, col)
				len += snprintf(buf + len, sizeof(buf) - len, "%02X ",
						irc_tbl->arr[i * sizeof_row(irc_tbl) + col]);
		}

		pr_info("%s\n", buf);
	}
}
#endif /* CONFIG_SUPPORT_DIM_FLASH */

static ssize_t aid_log_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	print_panel_resource(panel);

	show_aid_log(panel_data, PANEL_BL_SUBDEV_TYPE_DISP);
#ifdef CONFIG_SUPPORT_HMD
	show_aid_log(panel_data, PANEL_BL_SUBDEV_TYPE_HMD);
#endif

	return strlen(buf);
}

static ssize_t aid_log_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value == 1 || value == 2) {
		/* read brightness & make csv */
		show_brt_param(&panel->panel_data, PANEL_BL_SUBDEV_TYPE_DISP, value);
#ifdef CONFIG_SUPPORT_HMD
		show_brt_param(&panel->panel_data, PANEL_BL_SUBDEV_TYPE_HMD, value);
#endif
	}

	return size;
}
#endif /* CONFIG_PANEL_AID_DIMMING_DEBUG */

#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
static ssize_t lux_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.lux);

	return strlen(buf);
}

static ssize_t lux_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	int value;
	struct mdnie_info *mdnie;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	mdnie = &panel->mdnie;
	panel_data = &panel->panel_data;

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	if (panel_data->props.lux != value) {
		mutex_lock(&panel->op_lock);
		panel_data->props.lux = value;
		mutex_unlock(&panel->op_lock);
		attr_store_for_each(mdnie->class, attr->attr.name, buf, size);
	}

	return size;
}
#endif /* CONFIG_EXYNOS_DECON_MDNIE_LITE */

#ifdef CONFIG_EXYNOS_DECON_LCD_COPR
static ssize_t copr_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;

	return copr_reg_show(copr, buf);
}

static ssize_t copr_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	char *p, *arg = (char *)buf;
	const char *name;
	int index, rc;
	u32 value;

	mutex_lock(&copr->lock);
	while ((p = strsep(&arg, " \t")) != NULL) {
		if (!*p)
			continue;
		index = find_copr_reg_by_name(copr->props.version, p);
		if (index < 0) {
			pr_err("%s arg(%s) not found\n", __func__, p);
			continue;
		}

		name = get_copr_reg_name(copr->props.version, index);
		if (name == NULL) {
			pr_err("%s arg(%s) not found\n", __func__, p);
			continue;
		}

		rc = sscanf(p + strlen(name), "%i", &value);
		if (rc != 1) {
			pr_err("%s invalid argument name:%s ret:%d\n",
					__func__, name, rc);
			continue;
		}

		rc = copr_reg_store(copr, index, value);
		if (rc < 0) {
			pr_err("%s failed to store to copr_reg (index %d, value %d)\n",
					__func__, index, value);
			continue;
		}
	}

	copr->props.state = COPR_UNINITIALIZED;
	copr_update_average(copr);
	pr_info("%s copr %s\n", __func__,
			get_copr_reg_copr_en(copr) ? "enable" : "disable");
	mutex_unlock(&copr->lock);
	copr_update_start(&panel->copr, 1);

	return size;
}

static ssize_t read_copr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	int cur_copr;

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	if (!copr_is_enabled(copr)) {
		panel_err("%s copr is off state\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	cur_copr = copr_get_value(copr);
	if (cur_copr < 0) {
		panel_err("%s failed to get copr\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", cur_copr);
}

static ssize_t copr_roi_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	struct copr_roi roi[6];
	int nr_roi, rc = 0, i;

	memset(roi, -1, sizeof(roi));
	if (copr->props.version == COPR_VER_3) {
		rc = sscanf(buf, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				&roi[0].roi_xs, &roi[0].roi_ys, &roi[0].roi_xe, &roi[0].roi_ye,
				&roi[1].roi_xs, &roi[1].roi_ys, &roi[1].roi_xe, &roi[1].roi_ye,
				&roi[2].roi_xs, &roi[2].roi_ys, &roi[2].roi_xe, &roi[2].roi_ye,
				&roi[3].roi_xs, &roi[3].roi_ys, &roi[3].roi_xe, &roi[3].roi_ye,
				&roi[4].roi_xs, &roi[4].roi_ys, &roi[4].roi_xe, &roi[4].roi_ye);
		if (rc < 0) {
			pr_err("%s invalid roi input(rc %d)\n", __func__, rc);
			return -EINVAL;
		}
		if (rc == 20) {
			/* roi5&6 must be same in copr ver3.0 */
			memcpy(&roi[5], &roi[4], sizeof(struct copr_roi));
		}
	} else if (copr->props.version == COPR_VER_2 ||
			copr->props.version == COPR_VER_1) {
		rc = sscanf(buf, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				&roi[0].roi_xs, &roi[0].roi_ys, &roi[0].roi_xe, &roi[0].roi_ye,
				&roi[1].roi_xs, &roi[1].roi_ys, &roi[1].roi_xe, &roi[1].roi_ye,
				&roi[2].roi_xs, &roi[2].roi_ys, &roi[2].roi_xe, &roi[2].roi_ye,
				&roi[3].roi_xs, &roi[3].roi_ys, &roi[3].roi_xe, &roi[3].roi_ye);
		if (rc < 0) {
			pr_err("%s invalid roi input(rc %d)\n", __func__, rc);
			return -EINVAL;
		}
	} else {
		pr_err("%s roi is unsupported in copr ver%d\n",
				__func__, copr->props.version);
		return -EINVAL;
	}

	mutex_lock(&copr->lock);
	nr_roi = rc / 4;
	for (i = 0; i < nr_roi; i++) {
		if ((int)roi[i].roi_xs == -1 || (int)roi[i].roi_ys == -1 ||
			(int)roi[i].roi_xe == -1 || (int)roi[i].roi_ye == -1)
			continue;
		else
			memcpy(&copr->props.roi[i], &roi[i], sizeof(struct copr_roi));
	}
	if (copr->props.version == COPR_VER_2 ||
			copr->props.version == COPR_VER_1)
		copr->props.nr_roi = nr_roi;
	mutex_unlock(&copr->lock);

	if (copr->props.version == COPR_VER_3) {
		/* apply roi at once in copr ver3.0 */
		copr_roi_set_value(copr, copr->props.roi,
				copr->props.nr_roi);
	}

	for (i = 0; i < nr_roi; i++)
		pr_info("%s set roi[%d] %d %d %d %d\n", __func__, i,
				roi[i].roi_xs, roi[i].roi_ys, roi[i].roi_xe, roi[i].roi_ye);

	for (i = 0; i < copr->props.nr_roi; i++)
		pr_info("%s cur roi[%d] %d %d %d %d\n", __func__, i,
				copr->props.roi[i].roi_xs, copr->props.roi[i].roi_ys,
				copr->props.roi[i].roi_xe, copr->props.roi[i].roi_ye);

	return size;
}

static ssize_t copr_roi_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct copr_info *copr = &panel->copr;
	int i, c, ret, len = 0;
	u32 out[5 * 3] = { 0, };

	if (copr->props.nr_roi == 0) {
		panel_warn("%s copr roi disabled\n", __func__);
		return -ENODEV;
	}

	if (!copr_is_enabled(copr)) {
		panel_err("%s copr is off state\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	ret = copr_roi_get_value(copr, copr->props.roi,
			copr->props.nr_roi, out);
	if (ret < 0) {
		panel_err("%s failed to get copr\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	for (i = 0; i < copr->props.nr_roi; i++) {
		for (c = 0; c < 3; c++) {
			len += snprintf(buf + len, PAGE_SIZE - len,
					"%d%s", out[i * 3 + c],
					((i == copr->props.nr_roi - 1) && c == 2) ? "\n" : " ");
		}
	}

	return len;
}

static ssize_t brt_avg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_bl_device *panel_bl = &panel->panel_bl;
	int brt_avg;

	if (!IS_PANEL_ACTIVE(panel)) {
		panel_err("%s:panel is not active\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	brt_avg = panel_bl_get_average_and_clear(panel_bl, 1);
	if (brt_avg < 0) {
		panel_err("%s failed to get average brt1\n", __func__);
		return snprintf(buf, PAGE_SIZE, "-1\n");
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", brt_avg);
}
#endif

#ifdef CONFIG_DISPLAY_USE_INFO
/*
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpui_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpui_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL);

	return size;
}

/*
 * [DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpui_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpui_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_PANEL);

	return size;
}

/*
 * [AP DEPENDENT ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpci_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpci_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL);

	return size;
}

/*
 * [AP DEPENDENT DEV ONLY]
 * HW PARAM LOGGING SYSFS NODE
 */
static ssize_t dpci_dbg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int ret;

	update_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);
	ret = get_dpui_log(buf, DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);
	if (ret < 0) {
		pr_err("%s failed to get log %d\n", __func__, ret);
		return ret;
	}

	pr_info("%s\n", buf);
	return ret;
}

static ssize_t dpci_dbg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (buf[0] == 'C' || buf[0] == 'c')
		clear_dpui_log(DPUI_LOG_LEVEL_DEBUG, DPUI_TYPE_CTRL);

	return size;
}
#endif

static ssize_t poc_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%d\n", panel_data->props.poc_onoff);

	return strlen(buf);
}

static ssize_t poc_onoff_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc;
	int value;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	pr_info("%s: %d -> %d\n", __func__, panel_data->props.poc_onoff, value);

	if (panel_data->props.poc_onoff != value) {
		mutex_lock(&panel->panel_bl.lock);
		panel_data->props.poc_onoff = value;
		mutex_unlock(&panel->panel_bl.lock);
		panel_update_brightness(panel);
	}

	return size;
}


#ifdef CONFIG_EXTEND_LIVE_CLOCK

static ssize_t self_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	props = &aod->props;

	snprintf(buf, PAGE_SIZE, "%d\n", props->self_mask_en);

	return strlen(buf);
}

static ssize_t self_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc, ret;
	int value;
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	props = &aod->props;
	if ((aod == NULL) || (props == NULL)) {
		panel_err("PANEL:ERR:%s:aod is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	pr_info("%s: %d -> %d\n", __func__, props->self_mask_en, value);

	if (props->self_mask_en != value) {
		if (value == 0) {
			ret = panel_do_aod_seqtbl_by_index(aod, SELF_MASK_DIS_SEQ);
			if (unlikely(ret < 0))
				panel_err("PANEL:ERR:%s:failed to disable self mask\n", __func__);
		} else {
			ret = panel_do_aod_seqtbl_by_index(aod, SELF_MASK_IMG_SEQ);
			if (unlikely(ret < 0))
				panel_err("PANEL:ERR:%s:failed to write self mask image\n", __func__);

			ret = panel_do_aod_seqtbl_by_index(aod, SELF_MASK_ENA_SEQ);
			if (unlikely(ret < 0))
				panel_err("PANEL:ERR:%s:failed to enable self mask\n", __func__);
		}
		props->self_mask_en = value;
	}

	return size;
}
#endif

#ifdef SUPPORT_NORMAL_SELF_MOVE
static ssize_t self_move_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	props = &aod->props;

	snprintf(buf, PAGE_SIZE, "%d\n", props->self_move_pattern);

	return strlen(buf);
}

static ssize_t self_move_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	int pattern;
	struct aod_dev_info *aod;
	struct aod_ioctl_props *props;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	aod = &panel->aod;
	props = &aod->props;
	if ((aod == NULL) || (props == NULL)) {
		panel_err("PANEL:ERR:%s:aod is null\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoint(buf, 0, &pattern);
	if (ret < 0)
		return ret;

	if (pattern < NORMAL_SELF_MOVE_PATTERN_OFF ||
		pattern >= MAX_NORMAL_SELF_MOVE_PATTERN) {
		panel_err("PANEL:ERR:%s:out of range(%d)\n", __func__, pattern);
		return -EINVAL;
	}

	panel_info("PANEL:INFO:%s: pattern : %d\n", __func__, pattern);
	mutex_lock(&panel->io_lock);
	props->self_move_pattern = pattern;
	ret = panel_self_move_pattern_update(panel);
	if (ret < 0)
		panel_info("PANEL:ERR:%s:failed to set self move pattern\n", __func__);

	mutex_unlock(&panel->io_lock);
	return size;
}
#endif

#ifdef CONFIG_SUPPORT_ISC_DEFECT
static ssize_t isc_defect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "support isc defect checkd\n");

	return 0;
}

static ssize_t isc_defect_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc, ret;
	int value;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtoint(buf, 0, &value);

	if (rc < 0)
		return rc;

	panel_info("PANEL:INFO:%s: %d\n", __func__, value);

	mutex_lock(&panel->op_lock);

	if (value) {
		ret = panel_do_seqtbl_by_index_nolock(panel, PANEL_CHECK_ISC_DEFECT_SEQ);
		if (unlikely(ret < 0))
			panel_err("PANEL:ERR:%s:failed to write ics defect seq\n", __func__);
	}

	mutex_unlock(&panel->op_lock);
	return size;
}
#endif

#ifdef CONFIG_SUPPORT_SPI_IF_SEL
static ssize_t spi_if_sel_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "support spi if sel\n");

	return 0;
}

static ssize_t spi_if_sel_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int rc, ret;
	int value;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtoint(buf, 0, &value);
	if (rc < 0)
		return rc;

	panel_info("PANEL:INFO:%s: %d\n", __func__, value);
	mutex_lock(&panel->op_lock);
	ret = panel_do_seqtbl_by_index_nolock(panel,
			value ? PANEL_SPI_IF_ON_SEQ : PANEL_SPI_IF_OFF_SEQ);
	if (unlikely(ret < 0)) {
		panel_err("PANEL:ERR:%s:failed to write spi-if-%s seq\n",
				__func__, value ? "on" : "off");
	}

	mutex_unlock(&panel->op_lock);
	return size;
}
#endif

#ifdef CONFIG_SUPPORT_CCD_TEST
static ssize_t ccd_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);
	int ret = 0, retVal = 0;
	u8 ccd_state = 0xff;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	ret = panel_do_seqtbl_by_index(panel, PANEL_CCD_TEST_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write ccd seq\n", __func__);
		return ret;
	}
	resource_copy_n_clear_by_name(panel_data, &ccd_state, "ccd_state");
	retVal = (ccd_state == 0x00) ? 1 : 0;

	pr_info("%s : %s 0x%02x %d\n",
		__func__, (retVal == 1) ? "Pass" : "Fail", ccd_state, retVal);
	snprintf(buf, PAGE_SIZE, "%d\n", retVal);
	return strlen(buf);
}
#endif

#ifdef CONFIG_SUPPORT_DYNAMIC_HLPM
static ssize_t dynamic_hlpm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	snprintf(buf, PAGE_SIZE, "%u\n",
			panel_data->props.dynamic_hlpm);

	return strlen(buf);
}

static ssize_t dynamic_hlpm_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc, ret;
	struct panel_info *panel_data;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	panel_data = &panel->panel_data;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;
	if ((panel_data->props.alpm_mode != HLPM_HIGH_BR) && (panel_data->props.alpm_mode != HLPM_LOW_BR)) {
		pr_info("%s please set HLPM mode %d\n", __func__, panel_data->props.alpm_mode);
		return size;
	}

	if (panel_data->props.dynamic_hlpm == value)
		return size;

	mutex_lock(&panel->op_lock);
	panel_data->props.dynamic_hlpm = value;
	mutex_unlock(&panel->op_lock);

	ret = panel_do_seqtbl_by_index(panel,
			value ? PANEL_DYNAMIC_HLPM_ON_SEQ : PANEL_DYNAMIC_HLPM_OFF_SEQ);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to write dynamic_hlpm on/off seq\n", __func__);
		return ret;
	}
	dev_info(dev, "%s, dynamic hlpm %s\n",
			__func__, panel_data->props.dynamic_hlpm ? "on" : "off");

	return size;
}
#endif
#ifdef CONFIG_DYNAMIC_FREQ
static ssize_t dynamic_freq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct df_status_info *dyn_status;

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	dyn_status = &panel->df_status;

	snprintf(buf, PAGE_SIZE, "[DYN_FREQ] req: %d current: %d\n", dyn_status->request_df, dyn_status->current_df);

	return strlen(buf);
}

static ssize_t dynamic_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value < 0) {
		panel_err("PANEL:ERR:%s:value is negative : %d\n", __func__, value);
		return -EINVAL;
	}

	dynamic_freq_update(panel, value);

	return size;
}


#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
static ssize_t adap_freq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct adaptive_info *adap_info;
	struct adaptive_idx *adap_idx;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	adap_info = &panel->lcd_info.adaptive_info;
	adap_idx = &panel->adap_idx;

	snprintf(buf, PAGE_SIZE, "CUR IDX : (freq:%u, ffc:%u), HS : %u, REQ IDX : %u\n",
		adap_idx->cur_freq_idx, panel->panel_data.props.cur_ffc_idx,
		adap_info->freq_info[adap_idx->cur_freq_idx].hs_clk,
		adap_idx->req_freq_idx);

	return strlen(buf);
}

static ssize_t adap_freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct adaptive_info *adap_info;
	struct adaptive_idx *adap_idx;
	struct panel_device *panel = dev_get_drvdata(dev);

	if (panel == NULL) {
		panel_err("PANEL:ERR:%s:panel is null\n", __func__);
		return -EINVAL;
	}
	adap_info = &panel->lcd_info.adaptive_info;
	adap_idx = &panel->adap_idx;

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	if (value == adap_idx->cur_freq_idx) {
		panel_info("PANEL:INFO:%s:already set cur : %d, req : %d\n",
			__func__, adap_idx->cur_freq_idx, value);
		goto store_exit;
	}

	if (value >= adap_info->freq_cnt) {
		panel_err("PANEL:ERR:%s:invalid input : %d\n", __func__, value);
		goto store_exit;
	}

	adap_idx->req_freq_idx = value;

	panel_info("[ADAP_FREQ] %s: value : %d\n", __func__, adap_idx->req_freq_idx);

store_exit:
	return size;
}
#endif

#ifdef CONFIG_SUPPORT_POC_SPI
#define SPI_BUF_LEN 2048
u8 spi_flash_readbuf[SPI_BUF_LEN];
u32 spi_flash_readlen;
u8 spi_flash_writebuf[SPI_BUF_LEN];
u32 spi_flash_writelen;
static ssize_t spi_flash_ctrl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, len = 0;

	mutex_lock(&sysfs_lock);
	if (spi_flash_writelen <= 0) {
		mutex_unlock(&sysfs_lock);
		return -EINVAL;
	}

	len += snprintf(buf, PAGE_SIZE, "send %d byte(s):\n", spi_flash_writelen);
	for (i = 0; i < spi_flash_writelen; i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%02X%s", spi_flash_writebuf[i],
				(((i + 1) % 16) == 0) || (i == spi_flash_writelen - 1) ? "\n" : " ");

	len += snprintf(buf + len, PAGE_SIZE - len, "receive %d byte(s):\n", spi_flash_readlen);
	for (i = 0; i < spi_flash_readlen; i++)
		len += snprintf(buf + len, PAGE_SIZE - len, "%02X%s", spi_flash_readbuf[i],
				(((i + 1) % 16) == 0) || (i == spi_flash_readlen - 1) ? "\n" : " ");


	spi_flash_writelen = 0;
	spi_flash_readlen = 0;
	mutex_unlock(&sysfs_lock);

	return len;
}

static ssize_t spi_flash_ctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct panel_device *panel = dev_get_drvdata(dev);
	struct panel_spi_dev *spi_dev = &panel->panel_spi_dev;
	int ret, cmd_scanned, parse, cmd_input;

	mutex_lock(&sysfs_lock);
	mutex_lock(&panel->op_lock);

	memset(spi_flash_readbuf, 0, SPI_BUF_LEN);
	memset(spi_flash_writebuf, 0, SPI_BUF_LEN);
	spi_flash_readlen = spi_flash_writelen = 0;

	ret = sscanf(buf, "%d%n", &spi_flash_readlen, &parse);
	if (ret != 1 || parse <= 0 || spi_flash_readlen < 0 || spi_flash_readlen > SPI_BUF_LEN) {
		ret = -EINVAL;
		goto store_err;
	}
	cmd_scanned = parse;
	while (cmd_scanned < size && spi_flash_writelen < SPI_BUF_LEN) {
		ret = sscanf(buf + cmd_scanned, " %i%n", &cmd_input, &parse);
		if (ret < 1 || parse <= 0)
			break;

		spi_flash_writebuf[spi_flash_writelen++] = cmd_input & 0xFF;
		cmd_scanned += parse;
	}

	pr_info("%s send %d byte(s), receive %d byte(s)\n", __func__, spi_flash_writelen, spi_flash_readlen);
	print_hex_dump(KERN_ERR, __func__, DUMP_PREFIX_OFFSET, 16, 1, spi_flash_writebuf, spi_flash_writelen, false);

	ret = spi_dev->ops->cmd(spi_dev, spi_flash_writebuf, spi_flash_writelen, spi_flash_readbuf, spi_flash_readlen);
	if (ret < 0) {
		pr_err("%s, failed to spi cmd 0x%0x, ret %d\n",	__func__, spi_flash_writebuf[0], ret);
		ret = -EIO;
		goto store_err;
	}

	pr_info("%s received %d byte(s)\n", __func__, ret);
	if (ret > 0)
		print_hex_dump(KERN_ERR, __func__, DUMP_PREFIX_OFFSET, 16, 1, spi_flash_readbuf, ret, false);

	mutex_unlock(&panel->op_lock);
	mutex_unlock(&sysfs_lock);

	return size;

store_err:
	spi_flash_readlen = spi_flash_writelen = 0;
	mutex_unlock(&panel->op_lock);
	mutex_unlock(&sysfs_lock);

	return ret;
}
#endif

struct device_attribute panel_attrs[] = {
	__PANEL_ATTR_RO(lcd_type, 0444),
	__PANEL_ATTR_RO(window_type, 0444),
	__PANEL_ATTR_RO(manufacture_code, 0444),
	__PANEL_ATTR_RO(cell_id, 0444),
	__PANEL_ATTR_RO(octa_id, 0444),
	__PANEL_ATTR_RO(SVC_OCTA, 0444),
	__PANEL_ATTR_RO(SVC_OCTA_CHIPID, 0444),
	__PANEL_ATTR_RO(SVC_OCTA_DDI_CHIPID, 0444),
#ifdef CONFIG_SUPPORT_XTALK_MODE
	__PANEL_ATTR_RW(xtalk_mode, 0664),
#endif
#ifdef CONFIG_SUPPORT_MST
	__PANEL_ATTR_RW(mst, 0664),
#endif
#ifdef CONFIG_SUPPORT_POC_FLASH
	__PANEL_ATTR_RW(poc, 0660),
	__PANEL_ATTR_RO(poc_mca, 0440),
	__PANEL_ATTR_RO(poc_info, 0440),
#endif
#ifdef CONFIG_SUPPORT_DIM_FLASH
	__PANEL_ATTR_RW(gamma_flash, 0660),
#endif
#ifdef CONFIG_SUPPORT_GRAM_CHECKSUM
	__PANEL_ATTR_RW(gct, 0664),
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	__PANEL_ATTR_RW(grayspot, 0664),
#endif
	__PANEL_ATTR_RW(irc_mode, 0664),
	__PANEL_ATTR_RW(dia, 0664),
	__PANEL_ATTR_RO(color_coordinate, 0444),
	__PANEL_ATTR_RO(manufacture_date, 0444),
	__PANEL_ATTR_RO(brightness_table, 0444),
	__PANEL_ATTR_RW(adaptive_control, 0664),
	__PANEL_ATTR_RW(siop_enable, 0664),
	__PANEL_ATTR_RW(temperature, 0664),
#ifdef CONFIG_EXYNOS_LCD_ENG
	__PANEL_ATTR_RW(read_mtp, 0644),
	__PANEL_ATTR_RW(write_mtp, 0644),
	__PANEL_ATTR_RW(gamma_interpolation_test, 0664),
#ifdef CONFIG_SUPPORT_ISC_TUNE_TEST
	__PANEL_ATTR_RW(stm, 0664),
	__PANEL_ATTR_RW(isc, 0664),
#endif
#endif
	__PANEL_ATTR_RW(mcd_mode, 0664),
	__PANEL_ATTR_RW(partial_disp, 0664),
	__PANEL_ATTR_RW(mcd_resistance, 0664),
#ifdef CONFIG_PANEL_AID_DIMMING_DEBUG
	__PANEL_ATTR_RW(aid_log, 0660),
#endif
#if defined(CONFIG_EXYNOS_DECON_MDNIE_LITE)
	__PANEL_ATTR_RW(lux, 0644),
#endif
#if defined(CONFIG_EXYNOS_DECON_LCD_COPR)
	__PANEL_ATTR_RW(copr, 0600),
	__PANEL_ATTR_RO(read_copr, 0440),
	__PANEL_ATTR_RW(copr_roi, 0600),
	__PANEL_ATTR_RO(brt_avg, 0440),
#endif
	__PANEL_ATTR_RW(alpm, 0664),
	__PANEL_ATTR_RW(lpm_opr, 0664),
	__PANEL_ATTR_RW(fingerprint, 0644),
	__PANEL_ATTR_RW(conn_det, 0664),
#ifdef CONFIG_SUPPORT_HMD
	__PANEL_ATTR_RW(hmt_bright, 0664),
	__PANEL_ATTR_RW(hmt_on, 0664),
#endif
#ifdef CONFIG_DISPLAY_USE_INFO
	__PANEL_ATTR_RW(dpui, 0660),
	__PANEL_ATTR_RW(dpui_dbg, 0660),
	__PANEL_ATTR_RW(dpci, 0660),
	__PANEL_ATTR_RW(dpci_dbg, 0660),
#endif
	__PANEL_ATTR_RW(poc_onoff, 0664),
#ifdef CONFIG_EXTEND_LIVE_CLOCK
	__PANEL_ATTR_RW(self_mask, 0664),
#endif
#ifdef SUPPORT_NORMAL_SELF_MOVE
	__PANEL_ATTR_RW(self_move, 0664),
#endif
#ifdef CONFIG_SUPPORT_ISC_DEFECT
	__PANEL_ATTR_RW(isc_defect, 0664),
#endif
#ifdef CONFIG_SUPPORT_SPI_IF_SEL
	__PANEL_ATTR_RW(spi_if_sel, 0664),
#endif
#ifdef CONFIG_SUPPORT_CCD_TEST
	__PANEL_ATTR_RO(ccd_state, 0444),
#endif
#ifdef CONFIG_SUPPORT_DYNAMIC_HLPM
	__PANEL_ATTR_RW(dynamic_hlpm, 0664),
#endif
#ifdef CONFIG_DYNAMIC_FREQ
	__PANEL_ATTR_RW(dynamic_freq, 0664),
#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	__PANEL_ATTR_RW(adap_freq, 0664),
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
	__PANEL_ATTR_RW(spi_flash_ctrl, 0660),
#endif
	__PANEL_ATTR_RO(self_mask_check, 0444),
};

int panel_sysfs_probe(struct panel_device *panel)
{
	struct lcd_device *lcd;
	size_t i;
	int ret;
	struct kernfs_node *svc_sd;
	struct kobject *svc;

	lcd = panel->lcd;
	if (unlikely(!lcd)) {
		pr_err("%s, lcd device not exist\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(panel_attrs); i++) {
		ret = device_create_file(&lcd->dev, &panel_attrs[i]);
		if (ret < 0) {
			dev_err(&lcd->dev, "%s, failed to add %s sysfs entries, %d\n",
					__func__, panel_attrs[i].attr.name, ret);
			return -ENODEV;
		}
	}

	/* to /sys/devices/svc/ */
	svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(svc_sd)) {
		svc = kobject_create_and_add("svc", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(svc))
			pr_err("failed to create /sys/devices/svc already exist svc : 0x%pK\n", svc);
		else
			pr_err("success to create /sys/devices/svc svc : 0x%pK\n", svc);
	} else {
		svc = (struct kobject *)svc_sd->priv;
		pr_info("success to find svc_sd : 0x%pK  svc : 0x%pK\n", svc_sd, svc);
	}

	if (!IS_ERR_OR_NULL(svc)) {
		ret = sysfs_create_link(svc, &lcd->dev.kobj, "OCTA");
		if (ret)
			pr_err("failed to create svc/OCTA/\n");
		else
			pr_info("success to create svc/OCTA/\n");
	} else {
		pr_err("failed to find svc kobject\n");
	}

	return 0;
}
