/*
 * sec_tclm.c - samsung tclm command driver
 *
 * Copyright (C) 2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/input/sec_tclm_v2.h>
#include <linux/input.h>

struct sec_cal_position sec_cal_positions[CALPOSITION_MAX] = {
	{CAL_POS_CMD("NONE",	'N'),}, /* 0, NONe */
	{CAL_POS_CMD("INIT",	'I'),},	/* 1, INIT case, calcount is 00 or FF */
	{CAL_POS_CMD("FACT",	'F'),},	/* 2, FACTory line, run_force_calibration without value */
	{CAL_POS_CMD("OUTS",	'O'),},	/* 3, OUTSide of factory */
	{CAL_POS_CMD("LCIA",	'L'),},	/* 4, LCIA in factory line */
	{CAL_POS_CMD("CENT",	'C'),},	/* 5, svc CENTer, cal from service center */
	{CAL_POS_CMD("ABNO",	'A'),},	/* 6, ABNOrmal case */
	{CAL_POS_CMD("BOOT",	'B'),},	/* 7, BOOT firmup, when firmup in booting time */
	{CAL_POS_CMD("SPEC",	'S'),},	/* 8, SPECout case */
	{CAL_POS_CMD("TUNE",	'V'),},	/* 9, TUNE Version up, when afe version is lage than tune version */
	{CAL_POS_CMD("EVER",	'E'),},	/* 10, EVERytime, always cal in Booting */
	{CAL_POS_CMD("TEST",	'T'),},	/* 11, TESTmode, firmup case in *#2663# */
	{CAL_POS_CMD("UNDE",	'U'),},	/* 12, UNDEfine, undefined value */
	{CAL_POS_CMD("UNDE",	'U'),},	/* 13 */
	{CAL_POS_CMD("UNDE",	'U'),},	/* 14 */
	{CAL_POS_CMD("UNDE",	'U'),}	/* 15 */
};

void sec_tclm_case(struct sec_tclm_data *data, int tclm_case)
{
	switch (tclm_case) {
	case 0:
	case 'F':
	case 'f':
		if (data->external_factory == true)
			sec_tclm_root_of_cal(data, CALPOSITION_OUTSIDE);
		else
			sec_tclm_root_of_cal(data, CALPOSITION_FACTORY);
		break;

	case 'L':
	case 'l':
		sec_tclm_root_of_cal(data, CALPOSITION_LCIA);
		break;

	case 'C':
	case 'c':
		sec_tclm_root_of_cal(data, CALPOSITION_SVCCENTER);
		break;

	case 'O':
	case 'o':
		sec_tclm_root_of_cal(data, CALPOSITION_OUTSIDE);
		break;

	default:
		sec_tclm_root_of_cal(data, CALPOSITION_ABNORMAL);
	}
}

int tclm_test_command(struct sec_tclm_data *data, int test_case, int cmd_param1, int cmd_param2, char *buff)
{
	int ret = 1;
	const int buff_size = 256;
	switch (test_case) {
	case 0:	// get tclm_level,afe_base
		snprintf(buff, buff_size, "%d,%04X", data->tclm_level, data->afe_base);
		break;
	case 1: /* change cal_position & history
		  * cmd_param[1]: cal_position enum
		  * cmd_param[2]: tune_fix_ver */
		if (cmd_param1 == 0xff)
			sec_tclm_root_of_cal(data, 0);
		else
			sec_tclm_root_of_cal(data, cmd_param1);

		if (data->root_of_calibration != data->nvdata.cal_position) {
			sec_tclm_reposition_history(data);
			data->nvdata.cal_count = 0;
		}
		data->nvdata.cal_count++;
		if (cmd_param1 == 0) {
			data->nvdata.cal_count = 0;

			data->nvdata.cal_pos_hist_cnt = 0;
			data->nvdata.cal_pos_hist_lastp = 0;

			cmd_param2 = 0;
		} else if (cmd_param1 == 0xff) {
			data->nvdata.cal_count = 0xff;

			data->nvdata.cal_pos_hist_cnt = 0;
			data->nvdata.cal_pos_hist_lastp = 0;

			cmd_param2 = 0xffff;
		}
		data->nvdata.cal_position = data->root_of_calibration;
		data->nvdata.tune_fix_ver = cmd_param2;
		ret = data->tclm_write(data->client, SEC_TCLM_NVM_ALL_DATA);
		if (ret < 0) {
			input_info(true,&data->client->dev, "%s failed\n", __func__);
			snprintf(buff, buff_size, "%s", "FAIL");
			return ret;
		}
		sec_tclm_root_of_cal(data, CALPOSITION_NONE);

		input_info(true,&data->client->dev, "%s,1: cal_pos: %d, tune_fix_ver:0x%04X\n",
			__func__, cmd_param1, cmd_param2);

		sec_tclm_position_history(data);

		snprintf(buff, buff_size, "%s", "OK");
		break;
	case 2: /* change tclm_level, afe_base
		  * cmd_param[1]: tclm_level
		  * cmd_param[2]: afe_base */
		{

			data->tclm[0] = (cmd_param1 & 0xFF);
			data->tclm[1] = ((cmd_param2 >> 8) & 0xFF);
			data->tclm[2] = (cmd_param2 & 0xFF);

			ret = data->tclm_write(data->client, SEC_TCLM_NVM_TEST);
			if (ret < 0) {
				input_info(true,&data->client->dev, "%s failed\n", __func__);
				snprintf(buff, buff_size, "%s", "FAIL");
				return ret;
			}

			memset(data->tclm, 0x00, SEC_TCLM_NVM_OFFSET_LENGTH);
			ret = data->tclm_read(data->client, SEC_TCLM_NVM_TEST);
			if (ret < 0) {
				input_info(true,&data->client->dev, "%s failed\n", __func__);
				snprintf(buff, buff_size, "%s", "FAIL");
				return ret;
			}
			data->tclm_level = data->tclm[0];
			data->afe_base = (data->tclm[1] << 8) | data->tclm[2];

			input_err(true, &data->client->dev, "%s,2: tclm_level %d, sec_afe_base %04X\n", __func__, data->tclm_level, data->afe_base);
			snprintf(buff, buff_size, "%s", "OK");
		}
		break;
	case 3: /* clear tclm_level, afe_base nv & set to dt_data */
		{
			data->tclm[0]= 0xff;
			data->tclm[1]= 0xff;
			data->tclm[2]= 0xff;

			/* clear tclm_level, afe_base nvm to 0xff */
			ret = data->tclm_write(data->client, SEC_TCLM_NVM_TEST);
			if (ret < 0) {
				input_info(true,&data->client->dev, "%s failed\n", __func__);
				snprintf(buff, buff_size, "%s", "FAIL");
				return ret;
			}

			/* get dt_data again */
			data->tclm_parse_dt(data->client, data);

			input_err(true, &data->client->dev, "%s,3: tclm_level %d, sec_afe_base %04X\n", __func__, data->tclm_level, data->afe_base);
			snprintf(buff, buff_size, "%s", "OK");
		}
		break;
	}

	return ret;
}

int sec_tclm_test_on_probe(struct sec_tclm_data *data)
{
	int retry = 3;
	int ret = 0;

	while (retry--) {
		ret = data->tclm_read(data->client, SEC_TCLM_NVM_TEST);
		if (ret >= 0)
			break;
	}

	if (ret < 0)
		input_err(true, &data->client->dev, "%s: failed ret:%d\n", __func__, ret);

	return ret;
}

int sec_tclm_get_nvm_all(struct sec_tclm_data *data)
{
	int ret;
	int retry = 3;

	/* just don't read tune_fix_version, because this is write_only_value. */
	while (retry--) {
		ret = data->tclm_read(data->client, SEC_TCLM_NVM_ALL_DATA);
		if (ret >= 0)
			break;
	}

	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: failed ret:%d\n", __func__, ret);
		return ret;
	}

	if (data->nvdata.cal_count == 0xFF || data->nvdata.cal_position >= CALPOSITION_MAX) {
		data->nvdata.cal_count = 0;
		data->nvdata.cal_position = 0;
		data->nvdata.tune_fix_ver = 0;
		data->nvdata.cal_pos_hist_cnt = 0;
		data->nvdata.cal_pos_hist_lastp = 0;
		input_info(true, &data->client->dev, "%s: cal data is abnormal\n", __func__);
		return TCLM_RESULT_ABNORMAL;
	}

	if (data->nvdata.cal_pos_hist_cnt > CAL_HISTORY_QUEUE_MAX)
		data->nvdata.cal_pos_hist_cnt = 0;	/* error case */

	input_info(true, &data->client->dev, "%s: cal_count:%d, pos:%d(%4s), hist_count:%d, lastp:%d\n",
		__func__, data->nvdata.cal_count, data->nvdata.cal_position,
		data->tclm_string[data->nvdata.cal_position].f_name,
		data->nvdata.cal_pos_hist_cnt, data->nvdata.cal_pos_hist_lastp);

	sec_tclm_position_history(data);

	return TCLM_RESULT_DONE;
}

void sec_tclm_position_history(struct sec_tclm_data *data)
{
	int i;
	int now_lastp = data->nvdata.cal_pos_hist_lastp;
	unsigned char *pStr = NULL;
	unsigned char pTmp[5] = { 0 };

	if (data->nvdata.cal_pos_hist_cnt > CAL_HISTORY_QUEUE_MAX
		|| data->nvdata.cal_pos_hist_lastp >= CAL_HISTORY_QUEUE_MAX) {
		input_info(true, &data->client->dev, "%s: not initial case, count:%X, p:%X\n", __func__,
			data->nvdata.cal_pos_hist_cnt, data->nvdata.cal_pos_hist_lastp);
		return;
	}

	input_info(true, &data->client->dev, "%s: [Now] %4s%d\n", __func__,
		data->tclm_string[data->nvdata.cal_position].f_name, data->nvdata.cal_count);

	pStr = kzalloc(CAL_HISTORY_QUEUE_MAX * 5, GFP_KERNEL);
	if (pStr == NULL)
		return;

	for (i = 0; i < data->nvdata.cal_pos_hist_cnt; i++) {
		snprintf(pTmp, sizeof(pTmp), "%c%d", data->tclm_string[data->nvdata.cal_pos_hist_queue[2 * now_lastp]].s_name, data->nvdata.cal_pos_hist_queue[2 * now_lastp + 1]);
		strlcat(pStr, pTmp, CAL_HISTORY_QUEUE_MAX * 5);
		if (i < CAL_HISTORY_QUEUE_SHORT_DISPLAY) {
			data->cal_pos_hist_last3[2 * i] = data->tclm_string[data->nvdata.cal_pos_hist_queue[2 * now_lastp]].s_name;
			data->cal_pos_hist_last3[2 * i + 1] = data->nvdata.cal_pos_hist_queue[2 * now_lastp + 1];
		}

		if (now_lastp <= 0)
			now_lastp = CAL_HISTORY_QUEUE_MAX - 1;
		else
			now_lastp--;
	}

	input_info(true, &data->client->dev, "%s: [Old] %s\n", __func__, pStr);

	if (i < CAL_HISTORY_QUEUE_SHORT_DISPLAY)
		data->cal_pos_hist_last3[2 * i] = 0;
	else
		data->cal_pos_hist_last3[6] = 0;

	kfree(pStr);
}

void sec_tclm_debug_info(struct sec_tclm_data *data)
{
	sec_tclm_position_history(data);
}

void sec_tclm_root_of_cal(struct sec_tclm_data *data, int pos)
{
	data->root_of_calibration = pos;
	input_info(true, &data->client->dev, "%s: root - %d(%4s)\n", __func__,
		pos, data->tclm_string[pos].f_name);
}

static bool sec_tclm_check_condition_valid(struct sec_tclm_data *data)
{

	input_err(true, &data->client->dev, "%s tclm_level:%02X, last pos:%d(%4s), now pos:%d(%4s)\n",
		__func__, data->tclm_level, data->nvdata.cal_position, data->tclm_string[data->nvdata.cal_position].f_name,
		data->root_of_calibration, data->tclm_string[data->root_of_calibration].f_name);

	/* enter case */
	switch (data->tclm_level) {
	case TCLM_LEVEL_LOCKDOWN:
		if ((data->root_of_calibration == CALPOSITION_TUNEUP)
			|| (data->root_of_calibration == CALPOSITION_INITIAL)) {
			return true;
		} else if ((data->root_of_calibration == CALPOSITION_TESTMODE)
			&& ((data->nvdata.cal_position == CALPOSITION_TESTMODE)
			|| (data->nvdata.cal_position == CALPOSITION_TUNEUP)
			|| (data->nvdata.cal_position == CALPOSITION_FIRMUP))) {
			return true;
		}
		break;

	case TCLM_LEVEL_CLEAR_NV:
		return true;

	case TCLM_LEVEL_EVERYTIME:
		return true;

	case TCLM_LEVEL_NONE:
		if ((data->root_of_calibration == CALPOSITION_TESTMODE)
			|| (data->root_of_calibration == CALPOSITION_INITIAL)) {
			return true;
		} else {
			return false;
		}
	}

	return false;
}

void sec_tclm_reposition_history(struct sec_tclm_data *data)
{
	/* current data of cal count,position save cal history queue */

	if (data->nvdata.cal_pos_hist_cnt > CAL_HISTORY_QUEUE_MAX || data->nvdata.cal_pos_hist_lastp >= CAL_HISTORY_QUEUE_MAX) {
		/* queue nvm clear case */
		data->nvdata.cal_pos_hist_cnt = 0;
		data->nvdata.cal_pos_hist_lastp = 0;
	}

	/*calculate queue lastpointer */
	if (data->nvdata.cal_pos_hist_cnt == 0)
		data->nvdata.cal_pos_hist_lastp = 0;
	else if (data->nvdata.cal_pos_hist_lastp >= (CAL_HISTORY_QUEUE_MAX - 1))
		data->nvdata.cal_pos_hist_lastp = 0;
	else
		data->nvdata.cal_pos_hist_lastp++;

	/*calculate queue count */
	if (data->nvdata.cal_pos_hist_cnt >= CAL_HISTORY_QUEUE_MAX)
		data->nvdata.cal_pos_hist_cnt = CAL_HISTORY_QUEUE_MAX;
	else
		data->nvdata.cal_pos_hist_cnt++;

	data->nvdata.cal_pos_hist_queue[data->nvdata.cal_pos_hist_lastp * 2] = data->nvdata.cal_position;
	data->nvdata.cal_pos_hist_queue[data->nvdata.cal_pos_hist_lastp * 2 + 1] = data->nvdata.cal_count;
}

int sec_execute_tclm_package(struct sec_tclm_data *data, int factory_mode)
{
	int ret;
	int retry = 3;

	/* first read cal data for compare */

	ret = sec_tclm_get_nvm_all(data);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: sec_tclm_nvm_all_data i2c read fail", __func__);
		goto out;
	}


	input_err(true, &data->client->dev, "%s: tclm_level:%02X, last pos:%d(%4s), now pos:%d(%4s), factory:%d\n",
			__func__, data->tclm_level, data->nvdata.cal_position, data->tclm_string[data->nvdata.cal_position].f_name,
			data->root_of_calibration, data->tclm_string[data->root_of_calibration].f_name,
			factory_mode);

	/* if is run_for_calibration, don't check cal condition */
	if (!factory_mode) {

		/*check cal condition */
		ret = sec_tclm_check_condition_valid(data);
		if (!ret) {
			input_err(true, &data->client->dev, "%s: fail tclm condition,%d, root:%d\n",
				__func__, ret, data->root_of_calibration);
			return TCLM_RESULT_DONE; /* do not need calibration */
		}

		input_err(true, &data->client->dev, "%s: RUN OFFSET CALIBRATION,%d\n", __func__, ret);

		/* execute force cal */
		ret = data->tclm_execute_force_calibration(data->client, TCLM_OFFSET_CAL_SEC);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s: fail to write OFFSET CAL SEC!\n", __func__);
			return ret;
		}
	}

	if ((data->nvdata.cal_count < 1) || (data->nvdata.cal_count >= 0xFF)) {
		/* all nvm clear */
		data->nvdata.cal_count = 0;
		data->nvdata.cal_pos_hist_cnt = 0;
		data->nvdata.cal_pos_hist_lastp = 0;
	} else if (data->root_of_calibration != data->nvdata.cal_position) {
		sec_tclm_reposition_history(data);
		data->nvdata.cal_count = 0;
	}

	if (data->nvdata.cal_count == 0) {
		/* saving cal position */
		data->nvdata.cal_position = data->root_of_calibration;
	}

	data->nvdata.cal_count++;

	/* saving tune_version */
	ret = data->tclm_read(data->client, SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER);
	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: SEC_TCLM_NVM_OFFSET_IC_FIRMWARE_VER i2c read fail", __func__);
		goto out;
	}
	data->nvdata.tune_fix_ver = ret;

	while (retry--) {
		ret = data->tclm_write(data->client, SEC_TCLM_NVM_ALL_DATA);
		if (ret >= 0)
			break;
	}

	if (ret < 0) {
		input_err(true, &data->client->dev, "%s: failed ret:%d\n", __func__, ret);
		goto out;
	}

	sec_tclm_position_history(data);

	return TCLM_RESULT_CAL_DONE;

out:
	return ret;
}

int sec_tclm_check_cal_case(struct sec_tclm_data *data)
{
	int restore_cal = 0;
	int ret = 0;

	if (data->nvdata.cal_count == 0xFF) {
		ret = data->tclm_read(data->client, SEC_TCLM_NVM_ALL_DATA);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s: fail to read SEC_TCLM_NVM_ALL_DATA !\n", __func__);
			return ret;
		}

		input_info(true, &data->client->dev, "%s: cal_count value [%d]\n", __func__, data->nvdata.cal_count);
	}

	if ((data->nvdata.cal_count == 0) || (data->nvdata.cal_count == 0xFF)) {
		input_err(true, &data->client->dev, "%s: Calcount is abnormal,%02X\n", __func__, data->nvdata.cal_count);
		/* nvm uninitialed case */
		sec_tclm_root_of_cal(data, CALPOSITION_INITIAL);
		restore_cal = 1;
	} else if (data->tclm_level == TCLM_LEVEL_EVERYTIME) {
		/* everytime case */
		sec_tclm_root_of_cal(data, CALPOSITION_EVERYTIME);
		restore_cal = 1;
	}

	if (restore_cal) {
		ret = sec_execute_tclm_package(data, 0);
		if (ret < 0) {
			input_err(true, &data->client->dev, "%s: fail sec_execute_tclm_package!\n", __func__);
			return ret;
		}
		sec_tclm_root_of_cal(data, CALPOSITION_NONE);
		return ret;
	}

	ret = sec_tclm_get_nvm_all(data);
	if (ret < 0) {
		input_info(true, &data->client->dev, "%s: sec_tclm_get_nvm_all error \n", __func__);
	}

	return ret;
}

void sec_tclm_initialize(struct sec_tclm_data *data)
{
	data->root_of_calibration = CALPOSITION_NONE;
	data->nvdata.cal_position = 0;
	data->nvdata.cal_pos_hist_cnt = 0;
	data->cal_pos_hist_last3[0] = 0;
	data->tclm_string = sec_cal_positions;
	data->nvdata.cal_count = 0xFF;
}

MODULE_DESCRIPTION("Samsung tclm command");
MODULE_LICENSE("GPL");
