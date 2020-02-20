/* --------------- (C) COPYRIGHT 2012 STMicroelectronics ----------------------
 *
 * File Name	: stm_fsr_functions.c
 * Authors		: AMS(Analog Mems Sensor) Team
 * Description	: Firmware update for Strain gauge sidekey controller (Formosa + D3)
 *
 * -----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 * -----------------------------------------------------------------------------
 * REVISON HISTORY
 * DATE		 | DESCRIPTION
 * 01/03/2019| First Release
 * 01/30/2019| Add fsr_read_system_info in fsr_functions_init
 * -----------------------------------------------------------------------------
 */

#include "stm_fsr_sidekey.h"

static void fw_update(void *device_data);
static void get_chip_vendor(void *device_data);
static void get_chip_name(void *device_data);
static void get_fw_ver_bin(void *device_data);
static void get_fw_ver_ic(void *device_data);
static void get_config_ver(void *device_data);
static void get_ch_num(void *device_data);
static void get_checksum_data(void *device_data);
static void check_fw_corruption(void *device_data);
static void get_cx_data(void *device_data);
static void run_cx_data_read(void *device_data);
static void run_rawcap_read(void *device_data);
static void get_rawcap(void *device_data);
static void run_delta_read(void *device_data);
static void get_delta(void *device_data);
static void run_reference_read(void *device_data);
static void get_reference(void *device_data);
static void run_jitter_read(void *device_data);
static void run_force_calibration(void *device_data);
static void set_press_mode(void *device_data);
static void get_press_strength(void *device_data);
static void set_calibration_mode(void *device_data);
static void set_calibration_clear(void *device_data);
static void set_calibration_strength(void *device_data);
static void get_calibration_strength(void *device_data);
static void get_threshold(void *device_data);
static void set_threshold(void *device_data);
static void factory_cmd_result_all(void *device_data);
static void reset(void *device_data);
static void debug(void *device_data);
static void not_support_cmd(void *device_data);

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("get_chip_vendor", get_chip_vendor),},
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("get_fw_ver_bin", get_fw_ver_bin),},
	{SEC_CMD("get_fw_ver_ic", get_fw_ver_ic),},
	{SEC_CMD("get_config_ver", get_config_ver),},
	{SEC_CMD("get_ch_num", get_ch_num),},
	{SEC_CMD("get_checksum_data", get_checksum_data),},
	{SEC_CMD("get_crc_check", check_fw_corruption),},
	{SEC_CMD("run_cx_data_read", run_cx_data_read),},
	{SEC_CMD("get_cx_data", get_cx_data),},
	{SEC_CMD("run_rawcap_read", run_rawcap_read),},
	{SEC_CMD("get_rawcap", get_rawcap),},
	{SEC_CMD("run_delta_read", run_delta_read),},
	{SEC_CMD("get_delta", get_delta),},
	{SEC_CMD("run_reference_read", run_reference_read),},
	{SEC_CMD("get_reference", get_reference),},
	{SEC_CMD("run_jitter_read", run_jitter_read),},
	{SEC_CMD("set_press_mode", set_press_mode),},
	{SEC_CMD("run_force_calibration", run_force_calibration),},
	{SEC_CMD("get_press_strength", get_press_strength),},
	{SEC_CMD("set_calibration_mode", set_calibration_mode),},
	{SEC_CMD("set_calibration_clear", set_calibration_clear),},
	{SEC_CMD("set_calibration_strength", set_calibration_strength),},
	{SEC_CMD("get_calibration_strength", get_calibration_strength),},
	{SEC_CMD("get_threshold", get_threshold),},
	{SEC_CMD("set_threshold", set_threshold),},
	{SEC_CMD("factory_cmd_result_all", factory_cmd_result_all),},
	{SEC_CMD("reset", reset),},
	{SEC_CMD("debug", debug),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

int fsr_functions_init(struct fsr_sidekey_info *info)
{
	int retval;

	retval = sec_cmd_init(&info->sec, sec_cmds,
			ARRAY_SIZE(sec_cmds), SEC_CLASS_DEVT_SIDEKEY);
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to sec_cmd_init\n", __func__);
		goto exit;
	}

	retval = sysfs_create_link(&info->sec.fac_dev->kobj,
			&info->input_dev->dev.kobj, "input");
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to create input symbolic link\n",
				__func__);
		goto exit;
	}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	retval = fsr_vendor_functions_init(info);
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to create sysfs attributes\n", __func__);
		goto exit;
	}
#endif

	// Read system info from IC
	fsr_read_system_info(info);

	return 0;

exit:
	return retval;
}

void fsr_functions_remove(struct fsr_sidekey_info *info)
{
	input_err(true, &info->client->dev, "%s\n", __func__);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	fsr_vendor_functions_remove(info);
#endif

	sec_cmd_exit(&info->sec, SEC_CLASS_DEVT_SIDEKEY);
}

static void reset(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int retval;
	unsigned int delay;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] == 0)
		delay = 20;
	else
		delay = sec->cmd_param[0];

	retval = fsr_systemreset_gpio(info, delay, false);
	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[64] = { 0 };
	int retval = 0;

	sec_cmd_set_default_result(sec);

	retval = fsr_fw_update_on_hidden_menu(info, sec->cmd_param[0]);

	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: failed [%d]\n", __func__, retval);
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_OK;
		input_info(true, &info->client->dev, "%s: success [%d]\n", __func__, retval);
	}
}

static void get_chip_vendor(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[16] = { 0 };

	strncpy(buff, "STM", sizeof(buff));
	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_VENDOR");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[16] = { 0 };

	if (info->board->firmware_name)
		memcpy(buff, info->board->firmware_name + 8, 8);
	else
		strncpy(buff, "FTS", sizeof(buff));

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "IC_NAME");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_bin(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[16] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "ST%04X",
			info->fw_main_version_of_bin);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_BIN");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_fw_ver_ic(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[16] = { 0 };
	int rc;

	sec_cmd_set_default_result(sec);

	rc = fsr_get_version_info(info);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "ST%04X", info->fw_main_version_of_ic);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "FW_VER_IC");
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_config_ver(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[20] = { 0 };

	snprintf(buff, sizeof(buff), "%s_ST_%04X",
			info->board->model_name ?: info->board->project_name ?: "STM",
			info->config_version_of_ic);

	sec_cmd_set_default_result(sec);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_ch_num(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[20] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%d", info->fsr_sys_info.linelen * info->fsr_sys_info.afelen);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_checksum_data(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[20] = { 0 };
	u32 checksum_data = 0;

	sec_cmd_set_default_result(sec);

	checksum_data = ((u32) info->fsr_sys_info.crc_msb << 16) + (u32) info->fsr_sys_info.crc_lsb;

	snprintf(buff, sizeof(buff), "%08X", checksum_data);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void check_fw_corruption(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[16] = { 0 };
	int rc;

	sec_cmd_set_default_result(sec);

	if (info->irq >= 0)
		disable_irq(info->irq);

	fsr_command(info, CMD_NOTI_AP_BOOTUP);
	rc = fsr_crc_check(info);
	if (rc == -FSR_ERROR_FW_CORRUPTION) {
		snprintf(buff, sizeof(buff), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	if (info->irq >= 0)
		enable_irq(info->irq);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static int fsr_check_index(struct fsr_sidekey_info *info)
{
	struct sec_cmd_data *sec = &info->sec;
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int node;

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] >= 6) {

		snprintf(buff, sizeof(buff), "%s", "NG");
		sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		input_err(true, &info->client->dev, "%s: parameter error: %u\n",
				__func__, sec->cmd_param[0]);
		node = -1;
		return node;
	}
	node = sec->cmd_param[0];
	input_info(true, &info->client->dev, "%s: node = %d\n", __func__, node);
	return node;
}

static void fsr_print_cx(struct fsr_sidekey_info *info)
{
	u8 ChannelName[6] = {6,12,7,14,8,15};
	u8 ChannelRemap[6] = {1,0,3,2,5,4};
	u8 pTmp[16] = { 0 };
	u8 *pStr = NULL;
	int i = 0;

	pStr = kzalloc(BUFFER_MAX, GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 7; i++) {
		snprintf(pTmp, 8, "-------");
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	snprintf(pTmp, 8, "       ");
	strncat(pStr, pTmp, 8);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, "  CH%02d  ", ChannelName[i]);
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 7; i++) {
		snprintf(pTmp, 8, "-------");
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	snprintf(pTmp, 7, " CX1  ");
	strncat(pStr, pTmp, 7);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, " %6d", info->ch_cx_data[ChannelRemap[i]].cx1);
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	snprintf(pTmp, 7, " CX2  ");
	strncat(pStr, pTmp, 7);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, " %6d", info->ch_cx_data[ChannelRemap[i]].cx2);
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	snprintf(pTmp, 7, " Total");
	strncat(pStr, pTmp, 7);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, " %6d", info->ch_cx_data[ChannelRemap[i]].total);
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 7; i++) {
		snprintf(pTmp, 8, "-------");
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	kfree(pStr);
}

int fsr_read_cx(struct fsr_sidekey_info *info)
{
	u8 regAdd[3] = {0xD0, 0x00, 0x00};
	u8 ch_cx1_loc[6] = {1, 5, 25, 29, 37, 41};
	struct channel_cx_data_raw ch_cx_data;
	u16 cxAddr;
	u8 *data = (u8 *)&ch_cx_data;
	struct cx_data_raw *ch_data;
	int retval = 0;
	int i = 0;

	input_info(true, &info->client->dev, "%s: Size of struct cx_data_raw is %d\n", __func__, sizeof(struct cx_data_raw));

	cxAddr = info->fsr_sys_info.compensationAddr + 12; // Add offset value 12.
	regAdd[1] = (cxAddr >> 8) & 0xff;
	regAdd[2] = (cxAddr) & 0xff;
	retval = fsr_read_reg(info, &regAdd[0], 3, (u8 *)&ch_cx_data, sizeof(struct channel_cx_data_raw));
	if (retval < 0)
		return -1;

	for (i = 0; i < 6; i++) {
		ch_data = (struct cx_data_raw *)(data + ch_cx1_loc[i]);
		info->ch_cx_data[i].cx1 = (ch_data->cx1.sign ? (-1 * ch_data->cx1.data) : ch_data->cx1.data);
		info->ch_cx_data[i].cx2 = (ch_data->cx2.sign ? (-1 * ch_data->cx2.data) : ch_data->cx2.data);
		info->ch_cx_data[i].total = (info->ch_cx_data[i].cx1 * G1[info->fsr_sys_info.gain1]) +
				(info->ch_cx_data[i].cx2 * G2[info->fsr_sys_info.gain1][info->fsr_sys_info.gain2]);
		info->ch_cx_data[i].total = info->ch_cx_data[i].total / 1000;
	}

	fsr_print_cx(info);

	return 0;
}

static void run_cx_data_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[64] = { 0 };
	int retval = 0;

	sec_cmd_set_default_result(sec);

	retval = fsr_read_cx(info);
	if (retval < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
			info->ch_cx_data[1].total, info->ch_cx_data[0].total,
			info->ch_cx_data[3].total, info->ch_cx_data[2].total,
			info->ch_cx_data[5].total, info->ch_cx_data[4].total);

	sec->cmd_state = SEC_CMD_STATUS_OK;

err:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "TOTAL_CX");
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_cx_data(void *device_data)
{
	u8 ChannelRemap[6] = {1,0,3,2,5,4};
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int node = 0;

	sec_cmd_set_default_result(sec);

	node = fsr_check_index(info);
	if (node < 0)
		return;

	snprintf(buff, sizeof(buff), "%d,%d,%d",
					info->ch_cx_data[ChannelRemap[node]].cx1,
					info->ch_cx_data[ChannelRemap[node]].cx2,
					info->ch_cx_data[ChannelRemap[node]].total);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void fsr_print_frame(struct fsr_sidekey_info *info, struct fsr_frame_data_info *data)
{
	u8 ChannelName[6] = {6,12,7,14,8,15};
	u8 ChannelRemap[6] = {1,0,3,2,5,4};
	u8 pTmp[16] = { 0 };
	u8 *pStr = NULL;
	short *fdata = (short *)data;
	int i = 0;

	pStr = kzalloc(BUFFER_MAX, GFP_KERNEL);
	if (pStr == NULL)
		return;

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, "-------");
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, "   CH%02d", ChannelName[i]);
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, "-------");
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, " %6d", fdata[ChannelRemap[i]]);
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	memset(pStr, 0x0, BUFFER_MAX);
	for (i = 0; i < 6; i++) {
		snprintf(pTmp, 8, "-------");
		strncat(pStr, pTmp, 8);
	}
	input_raw_info(true, &info->client->dev, "%s: %s\n", __func__, pStr);

	kfree(pStr);
}

int fsr_read_frame(struct fsr_sidekey_info *info, u16 type, struct fsr_frame_data_info *data, bool print)
{
	u8 regAdd[3] = {0xD0, 0x00, 0x00};
	u8 regData[6 * 2 + 1];
	u16 addr = 0;
	int retval = 0;

	// Set to active mode always
	regAdd[1] = 0xAF;
	regAdd[2] = 0x01;
	fsr_write_reg(info, &regAdd[1], 2);

	switch (type) {
	case TYPE_RAW_DATA:
		addr = info->fsr_sys_info.rawFrcTouchAddr;
		break;
	case TYPE_STRENGTH_DATA:
		addr = info->fsr_sys_info.normFrcTouchAddr;
		break;
	case TYPE_BASELINE_DATA:
		addr = info->fsr_sys_info.calibFrcTouchAddr;
		break;
	default:
		input_err(true, &info->client->dev, "%s: undefined type: %02X\n", __func__, type);
		return -1;
	}

	regAdd[1] = (addr >> 8) & 0xff;
	regAdd[2] = (addr) & 0xff;

	if (print)
		input_info(true, &info->client->dev, "%s: Frame Addr 0x%4x\n", __func__, addr);
	retval = fsr_read_reg(info, &regAdd[0], 3, (u8*)regData, 6 * 2 + 1);
	if (retval <= 0) {
		input_err(true, &info->client->dev, "%s: read failed at 0x%4x rc = %d\n", __func__, retval, type);
		return retval;
	}

	memcpy(data, &regData[1], 6 * 2);

	if (print) {
		switch (type) {
		case TYPE_RAW_DATA:
			input_raw_info(true, &info->client->dev, "%s: [Raw Data]\n", __func__);
			break;
		case TYPE_STRENGTH_DATA:
			input_raw_info(true, &info->client->dev, "%s: [Strength Data]\n", __func__);
			break;
		case TYPE_BASELINE_DATA:
			input_raw_info(true, &info->client->dev, "%s: [Baseline Data]\n", __func__);
			break;
		}

		fsr_print_frame(info, data);
	} else {
		short *fdata = (short *)data;

		input_info(true, &info->client->dev, "%s: %5d %5d %5d %5d %5d %5d\n", __func__,
			fdata[1], fdata[0], fdata[3], fdata[2], fdata[5], fdata[4]);
	}

	// Release active mode always
	regAdd[1] = 0xAF;
	regAdd[2] = 0x00;
	fsr_write_reg(info, &regAdd[1], 2);

	return 0;
}

static void run_rawcap_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct fsr_frame_data_info *fsr_frame_data = (struct fsr_frame_data_info *)&info->fsr_frame_data_raw;

	sec_cmd_set_default_result(sec);

	fsr_read_frame(info, TYPE_RAW_DATA, fsr_frame_data, true);
	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
				(*fsr_frame_data).ch06, (*fsr_frame_data).ch12,
				(*fsr_frame_data).ch07, (*fsr_frame_data).ch14,
				(*fsr_frame_data).ch08, (*fsr_frame_data).ch15);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING)
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "RAW_DATA");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_rawcap(void *device_data)
{
	u8 ChannelRemap[6] = {1,0,3,2,5,4};
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short *fdata = (short *)&info->fsr_frame_data_raw;
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	node = fsr_check_index(info);
	if (node < 0)
		return;

	val = fdata[ChannelRemap[node]];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_delta_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct fsr_frame_data_info *fsr_frame_data = (struct fsr_frame_data_info *)&info->fsr_frame_data_delta;

	sec_cmd_set_default_result(sec);

	fsr_read_frame(info, TYPE_STRENGTH_DATA, fsr_frame_data, false);
	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
				(*fsr_frame_data).ch06, (*fsr_frame_data).ch12,
				(*fsr_frame_data).ch07, (*fsr_frame_data).ch14,
				(*fsr_frame_data).ch08, (*fsr_frame_data).ch15);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_delta(void *device_data)
{
	u8 ChannelRemap[6] = {1,0,3,2,5,4};
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short *fdata = (short *)&info->fsr_frame_data_delta;
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	node = fsr_check_index(info);
	if (node < 0)
		return;

	val = fdata[ChannelRemap[node]];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_reference_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	struct fsr_frame_data_info *fsr_frame_data = (struct fsr_frame_data_info *)&info->fsr_frame_data_reference;

	sec_cmd_set_default_result(sec);

	fsr_read_frame(info, TYPE_BASELINE_DATA, fsr_frame_data, true);
	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
				(*fsr_frame_data).ch06, (*fsr_frame_data).ch12,
				(*fsr_frame_data).ch07, (*fsr_frame_data).ch14,
				(*fsr_frame_data).ch08, (*fsr_frame_data).ch15);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}
static void get_reference(void *device_data)
{
	u8 ChannelRemap[6] = {1,0,3,2,5,4};
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short *fdata = (short *)&info->fsr_frame_data_reference;
	short val = 0;
	int node = 0;

	sec_cmd_set_default_result(sec);

	node = fsr_check_index(info);
	if (node < 0)
		return;

	val = fdata[ChannelRemap[node]];
	snprintf(buff, sizeof(buff), "%d", val);
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_jitter_read(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char stdev[SEC_CMD_STR_LEN] = { 0 };
	u8 ChannelName[6] = {12, 6, 14, 7, 15, 8};
	u8 regAdd;
	u8 data[8];
	s16 timeover = 150; // 10ms x 150 = 1.5 sec
	s16 result[6][2];
	u8 ChannelID;
	s16 Jitter;
	s16 JitterStdev;

	sec_cmd_set_default_result(sec);

	fsr_command(info, CMD_SENSEOFF);

	fsr_interrupt_set(info, INT_DISABLE);

	fsr_command(info, CMD_SPEC_JITTER);
	fsr_delay(1000);

	regAdd = CMD_READ_EVENT;
	while (fsr_read_reg(info, &regAdd, 1, (u8*)data, 8)) {
		if (data[0] == EID_SPEC_JITTER_RESULT) {
			ChannelID	= data[1];
			Jitter		= (data[2] << 8) + data[3];
			JitterStdev	= (data[4] << 8) + data[5];

			result[ChannelID][0] = Jitter;
			result[ChannelID][1] = JitterStdev;
			input_info(true, &info->client->dev, "%s: ch:%d, jitter:%d, stdev:%d\n",
				__func__, ChannelName[ChannelID], Jitter, JitterStdev);
			continue;
		} else if ((data[0] == EID_STATUS_EVENT) &&
				((data[1] == STATUS_EVENT_SPEC_TEST_DONE) ||
				(data[1] == STATUS_EVENT_ITO_TEST_DONE))) {
			break;
		} else {
			fsr_delay(10);
		}

		if (timeover-- < 0) {
			input_err(true, &info->client->dev,
				"%s: Time over to read the result of the spec data!!\n", __func__);
			break;
		}
	}

	fsr_systemreset(info, 10);

	if (timeover < 0) {
		snprintf(buff, sizeof(buff), "NG");
		snprintf(stdev, sizeof(stdev), "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
				result[1][0], result[0][0],
				result[3][0], result[2][0],
				result[5][0], result[4][0]);
	snprintf(stdev, sizeof(stdev), "%d,%d,%d,%d,%d,%d",
				result[1][1], result[0][1],
				result[3][1], result[2][1],
				result[5][1], result[4][1]);

	sec->cmd_state = SEC_CMD_STATUS_OK;

err:
	if (sec->cmd_all_factory_state == SEC_CMD_STATUS_RUNNING) {
		sec_cmd_set_cmd_result_all(sec, buff, strnlen(buff, sizeof(buff)), "JITTER");
		sec_cmd_set_cmd_result_all(sec, stdev, strnlen(stdev, sizeof(stdev)), "JITTER_STDEV");
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void run_force_calibration(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc;

	sec_cmd_set_default_result(sec);

	rc = fsr_execute_autotune(info);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static bool fsr_is_support_threshold(struct fsr_sidekey_info *info)
{
	if (strncmp(info->board->project_name, "DAVINCI", 7) != 0)
		return true;

	/* D1/D2 DV2 C type sample */
	if ((info->fw_main_version_of_ic & 0xF000) < 0x2000 &&
		(info->fw_main_version_of_ic & 0xFF) >= 0x0E && (info->fw_main_version_of_ic & 0xFF) < 0xF0)
		return true;

	/* Test Firmware */
	if ((info->fw_main_version_of_ic & 0xF000) == 0xF000)
		return true;

	return false;
}

int fsr_get_threshold(struct fsr_sidekey_info *info)
{
	u8 regAdd[3] = {0xD0, 0x00, 0x46};
	u8 data[5] = {0};
	u8 result[3] = {0};
	short threshold;
	int rc;

	if (!fsr_is_support_threshold(info))
		return -1;

	rc = fsr_read_reg(info, regAdd, 3, data, 5);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: faied to read address\n", __func__);	
		return rc;
	}

	/* Read Threshold */
	regAdd[1] = data[2];
	regAdd[2] = data[1];
	rc = fsr_read_reg(info, regAdd, 3, result, 3);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: faied to read threshold\n", __func__);	
		return rc;
	}
	threshold = (result[2] << 8) | result[1];

	/* Read Hysteresis */
	regAdd[1] = data[4];
	regAdd[2] = data[3];
	rc = fsr_read_reg(info, regAdd, 3, result, 3);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: faied to read hysteresis\n", __func__);	
		return rc;
	}
	info->thd_of_ic[1] = (result[2] << 8) | result[1];
	info->thd_of_ic[0] = threshold;

	input_info(true, &info->client->dev, "%s: threshold:%d, hysteresis:%d\n",
		__func__, info->thd_of_ic[0], info->thd_of_ic[1]);

	return 0;
}

static int fsr_set_threshold(struct fsr_sidekey_info *info, short *target)
{
	u8 regAdd[5] = {0x9C, 0};
	int rc;

	if (!fsr_is_support_threshold(info))
		return -1;

	info->thd_of_bin[0] = target[0];
	info->thd_of_bin[1] = target[1];

	input_info(true, &info->client->dev, "%s: threshold:%d, hysteresis:%d\n",
		__func__, info->thd_of_bin[0], info->thd_of_bin[1]);

	regAdd[1] = target[0] & 0xFF;
	regAdd[2] = target[0] >> 8;
	regAdd[3] = target[1] & 0xFF;
	regAdd[4] = target[1] >> 8;
	rc = fsr_write_reg(info, regAdd, 5);

	fsr_get_threshold(info);
	if ((info->thd_of_bin[0] != info->thd_of_ic[0]) || (info->thd_of_bin[1] != info->thd_of_ic[1])) {
		input_err(true, &info->client->dev, "%s: threshold:%d/%d, hysteresis:%d/%d\n", __func__,
			info->thd_of_bin[0], info->thd_of_ic[0], info->thd_of_bin[1], info->thd_of_ic[1]);
		rc = -EINVAL;
	}

	return rc;
}

static void set_press_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[5] = {0xA3, 0};
	int rc;
	short target[2] = {100, 30}; // Threshold:100, Hysteresis:30

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}
	if (!fsr_is_support_threshold(info)) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	fsr_interrupt_set(info, INT_DISABLE);

	regAdd[1] = sec->cmd_param[0];
	fsr_write_reg(info, regAdd, 2);
	rc = fsr_fw_wait_for_event_D3(info, regAdd[0], regAdd[1]);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	if (sec->cmd_param[0] == 1) {
		/* Enter Press Mode: set THD 100 */
		rc = fsr_set_threshold(info, target);
		if (rc < 0) {
			input_err(true, &info->client->dev, "%s: failed to set threshold %d\n", __func__, target[0]);
		}
	} else {
		/* Exit Press Mode: back to default THD */
		fsr_systemreset(info, 20);
		fsr_delay(10);
		fsr_get_threshold(info);
	}

	fsr_interrupt_set(info, INT_ENABLE);

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_average_strength(struct fsr_sidekey_info *info, short *avg, int num)
{
	struct fsr_frame_data_info *fsr_frame_data = (struct fsr_frame_data_info *)&info->fsr_frame_data_delta;
	u8 ChannelRemap[6] = {1, 0, 3, 2, 5, 4};
	short *data;
	int sum[6] = {0};
	int i, j;

	if (num <= 0)
		return;

	for (i = 0; i < num; i++) {
		fsr_read_frame(info, TYPE_STRENGTH_DATA, fsr_frame_data, false);
		data = (short *)fsr_frame_data;
		for (j = 0; j < 6; j++)
			sum[j] += data[ChannelRemap[j]];
	//	input_info(true, &info->client->dev, "%s: sum%d %d,%d,%d,%d,%d,%d\n", __func__, i,
	//			sum[0], sum[1], sum[2], sum[3], sum[4], sum[5]);
		fsr_delay(10);
	}

	for (j = 0; j < 6; j++)
		avg[j] = sum[j] / num;

	input_info(true, &info->client->dev, "%s: avg %d,%d,%d,%d,%d,%d\n", __func__,
			avg[0], avg[1], avg[2], avg[3], avg[4], avg[5]);
}

static void get_press_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short data[6] = {0};

	sec_cmd_set_default_result(sec);

	fsr_delay(2000);
	get_average_strength(info, data, 35);
	snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
				data[0], data[1],
				data[2], data[3],
				data[4], data[5]);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_calibration_mode(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[2] = {0x9A, 0};
	int rc;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 1) {
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto out;
	}

	regAdd[1] = sec->cmd_param[0];
	rc = fsr_write_reg(info, regAdd, 2);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}
	fsr_delay(10);

	if (sec->cmd_param[0] == 0) {
		fsr_systemreset(info, 20);
	}

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static int fsr_set_calibration_strength(struct fsr_sidekey_info *info, short *data, short *target)
{
	u8 ChannelRemap[6] = {1, 0, 3, 2, 5, 4};
	u8 regAdd[13] = {0};
	u8 afe[3] = {0};
	int i, rc;
	int len = 0;

	fsr_interrupt_set(info, INT_DISABLE);

	/* write measured strength data */
	regAdd[0] = 0x9B;
	for (i = 0; i < 6; i++) {
		regAdd[1 + i * 2] = data[i] & 0xFF;
		regAdd[1 + i * 2 + 1] = data[i] >> 8;

		input_info(true, &info->client->dev, "%s: ch%d %d, %04X, %02X %02X\n", __func__,
			ChannelRemap[i] + 1, data[i], data[i], regAdd[1 + i * 2], regAdd[1 + i * 2 + 1]);
	}
	rc = fsr_write_reg(info, regAdd, 13);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to write measured strength data, %d\n", __func__, rc);
		goto err;
	}

	if (fsr_is_support_threshold(info)) {
		/* Write Taget value */
		regAdd[0] = 0x9C;
		regAdd[1] = target[0] & 0xFF;
		regAdd[2] = target[0] >> 8;
		len = 3;
		input_info(true, &info->client->dev, "%s: %d, target:%d\n", __func__, len, target[0]);
	} else {
		/* Write Taget value/Threshold/Hysterisys */
		regAdd[0] = 0x9C;
		regAdd[1] = target[0] & 0xFF;
		regAdd[2] = target[0] >> 8;
		regAdd[3] = target[1] & 0xFF;
		regAdd[4] = target[1] >> 8;
		regAdd[5] = target[2] & 0xFF;
		regAdd[6] = target[2] >> 8;
		len = 7;
		input_info(true, &info->client->dev, "%s: %d, target:%d threshold:%d hysterisys:%d\n", __func__,
			len, target[0], target[1], target[2]);
	}
	rc = fsr_write_reg(info, regAdd, len);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to write target value, %d\n", __func__, rc);
		goto err;
	}
	fsr_delay(50);

	/* Read AFE version and Write */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = 0x44;
	rc = fsr_read_reg(info, regAdd, 3, afe, 3);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to get afe_ver, %d\n", __func__, rc);
		goto err;
	}
	input_info(true, &info->client->dev, "%s: prev_cal_afe_ver: %02X new_afe_ver %02X\n", __func__, afe[2], afe[1]);

	if (!(afe[1] == 0x00 || afe[1] == 0xFF)) {
		regAdd[0] = 0x9D;
		regAdd[1] = afe[1];
		rc = fsr_write_reg(info, regAdd, 2);
		if (rc < 0) {
			input_err(true, &info->client->dev, "%s: failed to write afe_ver, %d\n", __func__, rc);
			goto err;
		}
		rc = fsr_fw_wait_for_event_D3(info, regAdd[0], regAdd[1]);
		if (rc < 0) {
			input_err(true, &info->client->dev, "%s: failed to write afe_ver successfully, %d\n", __func__, rc);
			goto err;
		}
	}

	/* Save Data to Flash */
	fsr_command(info, CMD_SAVE_TUNNING_DATA);
	fsr_delay(200);
	rc = fsr_fw_wait_for_event(info, STATUS_EVENT_SAVE_CX_DONE);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to save data, %d\n", __func__, rc);
		goto err;
	}

err:
	fsr_interrupt_set(info, INT_ENABLE);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, (rc < 0) ? "FAIL" : "OK");

	return rc;
}

static void set_calibration_clear(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	u8 regAdd[2] = {0x9A, 0};
	short data[6] = {0};
	short target[3] = {0};
	int rc;

	sec_cmd_set_default_result(sec);

	/* Set Calibration Mode On */
	regAdd[1] = 1;
	rc = fsr_write_reg(info, regAdd, 2);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	/* Set ZERO to clear the calibration */
	memset(data, 0, sizeof(data));
	memset(target, 0, sizeof(target));

	rc = fsr_set_calibration_strength(info, data, target);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	/* Set Calibration Mode Off */
	regAdd[1] = 0;
	rc = fsr_write_reg(info, regAdd, 2);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

err:
	fsr_systemreset(info, 20);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

#define TARGET_DATA		150
#define THRESHOLD		100
#define HYSTERISYS		50

static void set_calibration_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	u8 ChannelRemap[6] = {1, 0, 3, 2, 5, 4};
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int i, rc;
	short data[6] = {0};
	short target[3] = {0};

	sec_cmd_set_default_result(sec);

	for (i = 0; i < 6; i++) {
		data[i] = (short)sec->cmd_param[ChannelRemap[i]];
		if (data[i] < TARGET_DATA)
			data[i] = TARGET_DATA;
	}
	target[0] = TARGET_DATA;
	target[1] = THRESHOLD;
	target[2] = HYSTERISYS;

	fsr_command(info, CMD_CLEAR_ALL_EVENT);
	fsr_delay(50);

	rc = fsr_set_calibration_strength(info, data, target);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%s", "OK");
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

int fsr_get_calibration_strength(struct fsr_sidekey_info *info)
{
	u8 regAdd[3] = {0xD0, 0x00, 0x3E};
	u8 data[25] = {0};
	int rc;

	rc = fsr_read_reg(info, regAdd, 3, data, 3);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to get address, %d\n", __func__, rc);
		return rc;
	}
	regAdd[1] = data[2];
	regAdd[2] = data[1];

	rc = fsr_read_reg(info, regAdd, 3, data, 25);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to get calibration strength, %d\n", __func__, rc);
		return rc;
	}

	memcpy(&info->fsr_cal_strength, &data[3], 12);
	memcpy(&info->calibration_target, &data[19], 6);

	/* Read AFE version */
	regAdd[0] = 0xD0;
	regAdd[1] = 0x00;
	regAdd[2] = 0x44;
	data[1] = 0;
	data[2] = 0;
	rc = fsr_read_reg(info, regAdd, 3, data, 3);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to get afe_ver, %d\n", __func__, rc);
	}
	info->calibration_target[3] = data[2];
	input_raw_info(true, &info->client->dev, "%s: fw's afe_ver %02X\n", __func__, data[1]);
	
	input_raw_info(true, &info->client->dev, "%s: [Calibrated Strength Data]\n", __func__);
	fsr_print_frame(info, &info->fsr_cal_strength);
	input_raw_info(true, &info->client->dev, "%s: target:%d, threshold:%d, hysteresis:%d, afe_ver:%02X\n", __func__,
		info->calibration_target[0], info->calibration_target[1],
		info->calibration_target[2], info->calibration_target[3]);

	return 0;
}

static void get_calibration_strength(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc;

	sec_cmd_set_default_result(sec);

	rc = fsr_get_calibration_strength(info);
	if (rc < 0) {
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
	} else {
		snprintf(buff, sizeof(buff), "%d,%d,%d,%d,%d,%d",
				info->fsr_cal_strength.ch06, info->fsr_cal_strength.ch12,
				info->fsr_cal_strength.ch07, info->fsr_cal_strength.ch14,
				info->fsr_cal_strength.ch08, info->fsr_cal_strength.ch15);
		sec->cmd_state = SEC_CMD_STATUS_OK;
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void get_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int rc;

	sec_cmd_set_default_result(sec);

	if (fsr_is_support_threshold(info)) {
		rc = fsr_get_threshold(info);
		if (rc < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "%d", info->thd_of_ic[0]);
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	} else {
		rc = fsr_get_calibration_strength(info);
		if (rc < 0) {
			snprintf(buff, sizeof(buff), "%s", "NG");
			sec->cmd_state = SEC_CMD_STATUS_FAIL;
		} else {
			snprintf(buff, sizeof(buff), "%d", info->calibration_target[1]);
			sec->cmd_state = SEC_CMD_STATUS_OK;
		}
	}

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void set_threshold(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	short target[2] = {0};
	int rc;

	sec_cmd_set_default_result(sec);

	if (!fsr_is_support_threshold(info)) {
		input_err(true, &info->client->dev, "%s: please check after dv2 sample\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NA");
		sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
		goto err;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > 2000) {
		input_err(true, &info->client->dev, "%s: threshold is not suitable!\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	rc = fsr_get_threshold(info);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to get threshold\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	target[0] = sec->cmd_param[0];
	target[1] = info->thd_of_ic[1];
	rc = fsr_set_threshold(info, target);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed to set threshold\n", __func__);
		snprintf(buff, sizeof(buff), "%s", "NG");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;
		goto err;
	}

	snprintf(buff, sizeof(buff), "%s", "OK");
	sec->cmd_state = SEC_CMD_STATUS_OK;

err:

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void factory_cmd_result_all(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);

	sec->item_count = 0;
	memset(sec->cmd_result_all, 0x00, SEC_CMD_RESULT_STR_LEN);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_RUNNING;

	fsr_systemreset(info, 10);

	fsr_delay(200);

	get_chip_vendor(sec);
	get_chip_name(sec);
	get_fw_ver_bin(sec);
	get_fw_ver_ic(sec);

	run_cx_data_read(sec);
	run_rawcap_read(sec);
	run_jitter_read(sec);

	sec->cmd_all_factory_state = SEC_CMD_STATUS_OK;

	input_info(true, &info->client->dev, "%s: %d%s\n", __func__, sec->item_count, sec->cmd_result_all);
}

static void debug(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	info->debug_string = sec->cmd_param[0];

	input_info(true, &info->client->dev, "%s: command is %d\n", __func__, info->debug_string);

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	sec->cmd_state = SEC_CMD_STATUS_WAITING;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	snprintf(buff, sizeof(buff), "%s", "NA");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);

	input_info(true, &info->client->dev, "%s: %s\n", __func__, buff);
}

