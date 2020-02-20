/*
 * Wacom Penabled Driver for I2C
 *
 * Copyright (c) 2011-2014 Tatsunosuke Tobita, Wacom.
 * <tobita.tatsunosuke@wacom.co.jp>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version of 2 of the License,
 * or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "wacom.h"
#include "w9020_flash.h"

static bool wacom_i2c_set_feature(struct wacom_i2c *wac_i2c, u8 report_id,
			   unsigned int buf_size, u8 *data, u16 cmdreg,
			   u16 datareg)
{
	int i, ret = -1;
	int total = SFEATURE_SIZE + buf_size;
	u8 *sFeature = NULL;
	bool bRet = false;

	sFeature = kzalloc(sizeof(u8) * total, GFP_KERNEL);
	if (!sFeature) {
		input_err(true, &wac_i2c->client->dev,
			  "%s cannot preserve memory\n", __func__);
		goto out;
	}

	memset(sFeature, 0, sizeof(u8) * total);

	sFeature[0] = (u8)(cmdreg & 0x00ff);
	sFeature[1] = (u8)((cmdreg & 0xff00) >> 8);
	sFeature[2] = (RTYPE_FEATURE << 4) | report_id;
	sFeature[3] = CMD_SET_FEATURE;
	sFeature[4] = (u8)(datareg & 0x00ff);
	sFeature[5] = (u8)((datareg & 0xff00) >> 8);

	if ((buf_size + 2) > 255) {
		sFeature[6] = (u8)((buf_size + 2) & 0x00ff);
		sFeature[7] = (u8)(((buf_size + 2) & 0xff00) >> 8);
	} else {
		sFeature[6] = (u8)(buf_size + 2);
		sFeature[7] = (u8)(0x00);
	}

	for (i = 0; i < buf_size; i++)
		sFeature[i + SFEATURE_SIZE] = *(data + i);

	ret = wacom_i2c_send(wac_i2c, sFeature, total, WACOM_I2C_MODE_BOOT);
	if (ret != total) {
		input_err(true, &wac_i2c->client->dev,
			  "Sending Set_Feature failed sent bytes: %d\n", ret);
		goto err;
	}

	usleep_range(60, 61);
	bRet = true;
err:
	kfree(sFeature);
	sFeature = NULL;

out:
	return bRet;
}

static bool wacom_i2c_get_feature(struct wacom_i2c *wac_i2c, u8 report_id,
			   unsigned int buf_size, u8 *data, u16 cmdreg,
			   u16 datareg, int delay)
{
	/*"+ 2", adding 2 more spaces for organizeing again later in the passed data, "data" */
	unsigned int total = buf_size + 2;
	int ret = -1;
	u8 *recv = NULL;
	bool bRet = false;
	u8 gFeature[] = {
		(u8)(cmdreg & 0x00ff),
		(u8)((cmdreg & 0xff00) >> 8),
		(RTYPE_FEATURE << 4) | report_id,
		CMD_GET_FEATURE,
		(u8)(datareg & 0x00ff),
		(u8)((datareg & 0xff00) >> 8)
	};

	recv = kzalloc(sizeof(u8) * total, GFP_KERNEL);
	if (!recv) {
		input_err(true, &wac_i2c->client->dev,
			  "%s cannot preserve memory\n", __func__);
		goto err_alloc_memory;
	}

	/*Append 2 bytes for length low and high of the byte */
	memset(recv, 0, sizeof(u8) * total);

	ret = wacom_i2c_send(wac_i2c, gFeature, GFEATURE_SIZE,
			   WACOM_I2C_MODE_BOOT);
	if (ret != GFEATURE_SIZE) {
		input_info(true, &wac_i2c->client->dev,
			   "%s Sending Get_Feature failed; sent bytes: %d\n",
			   __func__, ret);

		udelay(delay);

		goto err_fail_i2c;
	}

	udelay(delay);

	ret = wacom_i2c_recv(wac_i2c, recv, total, WACOM_I2C_MODE_BOOT);
	if (ret != total) {
		input_err(true, &wac_i2c->client->dev,
			  "%s Receiving data failed; recieved bytes: %d\n",
			  __func__, ret);
		goto err_fail_i2c;
	}

	/*Coppy data pointer, subtracting the first two bytes of the length */
	memcpy(data, (recv + 2), buf_size);

	bRet = true;
err_fail_i2c:
	kfree(recv);
	recv = NULL;

err_alloc_memory:
	return bRet;
}

static int wacom_flash_cmd(struct wacom_i2c *wac_i2c)
{
	u8 command[10] = { 0 };
	int len = 0;
	int ret = -1;

	command[len++] = 0x0d;
	command[len++] = FLASH_START0;
	command[len++] = FLASH_START1;
	command[len++] = FLASH_START2;
	command[len++] = FLASH_START3;
	command[len++] = FLASH_START4;
	command[len++] = FLASH_START5;
	command[len++] = 0x0d;

	ret = wacom_i2c_send(wac_i2c, command, len, WACOM_I2C_MODE_BOOT);
	if (ret < 0) {
		input_err(true, &wac_i2c->client->dev,
			  "Sending flash command failed\n");
		return -EXIT_FAIL;
	}

	msleep(300);

	return 0;
}

static int flash_query_w9020(struct wacom_i2c *wac_i2c)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = { 0 };
	u8 response[BOOT_RSP_SIZE] = { 0 };
	int ECH = 0, len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[len++] = BOOT_QUERY;		/* Report:Boot Query command */
	command[len++] = ECH = 7;		/* Report:echo */

	bRet = wacom_i2c_set_feature(wac_i2c, REPORT_ID_1, len, command,
					COMM_REG, DATA_REG);
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to set feature\n", __func__);
		return -EXIT_FAIL_SEND_QUERY_COMMAND;
	}

	bRet = wacom_i2c_get_feature(wac_i2c, REPORT_ID_2, BOOT_RSP_SIZE, response,
					COMM_REG, DATA_REG, (10 * 1000));
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to get feature\n", __func__);
		return -EXIT_FAIL_SEND_QUERY_COMMAND;
	}

	if ((response[1] != BOOT_CMD_REPORT_ID) || (response[2] != ECH) ) {
		input_err(true, &wac_i2c->client->dev, "%s res1:%x res2:%x\n",
			__func__, response[1], response[2]);
		return -EXIT_FAIL_SEND_QUERY_COMMAND;
	}

	if (response[3] != QUERY_RSP) {
		input_err(true, &wac_i2c->client->dev, "%s res3:%x \n",
				__func__, response[3]);
		return -EXIT_FAIL_SEND_QUERY_COMMAND;
	}

	/*
	if ((response[3] != QUERY_CMD) || (response[4] != ECH)) {
		input_err(true, &wac_i2c->client->dev, "%s res3:%x res4:%x\n",
			  __func__, response[3], response[4]);
		return -EXIT_FAIL_SEND_QUERY_COMMAND;
	}

	if (response[5] != QUERY_RSP) {
		input_err(true, &wac_i2c->client->dev, "%s res5:%x\n",
			  __func__, response[5]);
		return -EXIT_FAIL_SEND_QUERY_COMMAND;
	}
	*/
	input_info(true, &wac_i2c->client->dev, "QUERY SUCCEEDED\n");

	return 0;
}

static bool flash_blver_w9020(struct wacom_i2c *wac_i2c, int *blver)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = { 0 };
	u8 response[BOOT_RSP_SIZE] = { 0 };
	int ECH = 0, len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[len++] = BOOT_BLVER;		/* Report:Boot Version command */
	command[len++] = ECH = 7;		/* Report:echo */

	bRet = wacom_i2c_set_feature(wac_i2c, REPORT_ID_1, len, command, COMM_REG,
					DATA_REG);
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to set feature1\n", __func__);
		return false;
	}

	bRet = wacom_i2c_get_feature(wac_i2c, REPORT_ID_2, BOOT_RSP_SIZE, response,
				  COMM_REG, DATA_REG, (10 * 1000));
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s 2 failed to set feature\n", __func__);
		return false;
	}

	if ((response[1] != BOOT_BLVER) || (response[2] != ECH)) {
		input_err(true, &wac_i2c->client->dev, "%s res1:%x res2:%x \n",
				__func__, response[1], response[2]);
		return false;
	}
	/*
	if ((response[3] != BOOT_CMD) || (response[4] != ECH)) {
		input_err(true, &wac_i2c->client->dev, "%s res3:%x res4:%x\n",
			  __func__, response[3], response[4]);
		return false;
	}
	*/
	*blver = (int)response[3];

	return true;
}

static bool flash_mputype_w9020(struct wacom_i2c *wac_i2c, int *pMpuType)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = { 0 };
	u8 response[BOOT_RSP_SIZE] = { 0 };
	int ECH = 0, len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;	/* Report:ReportID */
	command[len++] = BOOT_MPU;	/* Report:Boot Query command */
	command[len++] = ECH = 7;	/* Report:echo */

	bRet = wacom_i2c_set_feature(wac_i2c, REPORT_ID_1, len, command, COMM_REG,
					DATA_REG);
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to set feature\n", __func__);
		return false;
	}

	bRet = wacom_i2c_get_feature(wac_i2c, REPORT_ID_2, BOOT_RSP_SIZE, response,
					COMM_REG, DATA_REG, (10 * 1000));
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to get feature\n", __func__);
		return false;
	}

	if ((response[1] != BOOT_MPU) || (response[2] != ECH) ) {
		input_err(true, &wac_i2c->client->dev, "%s res1:%x res2:%x \n",
				__func__, response[1], response[2]);
		return false;
	}
	/*
	if ((response[3] != MPU_CMD) || (response[4] != ECH)) {
		input_err(true, &wac_i2c->client->dev, "%s res3:%x res4:%x\n",
			  __func__, response[3], response[4]);
		return false;
	}
	*/
	*pMpuType = (int)response[3];
	return true;
}

static bool flash_end_w9020(struct wacom_i2c *wac_i2c)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = { 0 };
	int len = 0;

	command[len++] = BOOT_CMD_REPORT_ID;
	command[len++] = BOOT_EXIT;
	command[len++] = 0;

	bRet = wacom_i2c_set_feature(wac_i2c, REPORT_ID_1, len, command, COMM_REG,
					DATA_REG);
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to set feature 1\n", __func__);
		return false;
	}

	return true;
}

static int check_progress(struct wacom_i2c *wac_i2c, u8 *data, size_t size, u8 cmd, u8 ech)
{
	if (data[0] != cmd || data[1] != ech) {
		input_err(true, &wac_i2c->client->dev, "%s failed to erase\n",
				__func__);
		return -EXIT_FAIL;
	}

	switch (data[2]) {
	case PROCESS_CHKSUM1_ERR:
	case PROCESS_CHKSUM2_ERR:
	case PROCESS_TIMEOUT_ERR:
		input_err(true, &wac_i2c->client->dev, "%s error: %x\n",
			__func__, data[2]);
		return -EXIT_FAIL;
	}

	return data[2];
}

static bool flash_erase_all(struct wacom_i2c *wac_i2c)
{
	bool bRet = false;
	u8 command[BOOT_CMD_SIZE] = { 0 };
	u8 response[BOOT_RSP_SIZE] = { 0 };
	int i = 0, len = 0;
	int ECH = 0, sum = 0;
	int ret = -1;

	command[len++] = 7;
	command[len++] = ERS_ALL_CMD;
	command[len++] = ECH = 2;
	command[len++] = ERS_ECH2;

	/* Preliminarily stored data that cannnot appear here, but in wacom_set_feature() */
	sum += 0x05;
	sum += 0x07;
	for (i = 0; i < len; i++)
		sum += command[i];

	command[len++] = ~sum + 1;

	bRet = wacom_i2c_set_feature(wac_i2c, REPORT_ID_1, len, command, COMM_REG,
					DATA_REG);
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to set feature\n", __func__);
		return false;
	}

	do {
		bRet = wacom_i2c_get_feature(wac_i2c, REPORT_ID_2, BOOT_RSP_SIZE,
						response, COMM_REG, DATA_REG, 0);
		if (!bRet) {
			input_err(true, &wac_i2c->client->dev,
				  "%s failed to set feature\n", __func__);
			return false;
		}

		ret = check_progress(wac_i2c, &response[1], (BOOT_RSP_SIZE - 3), ERS_ALL_CMD, ECH);
		if (ret < 0)
			return false;

	} while (ret == PROCESS_INPROGRESS);

	return true;
}

static bool flash_write_block_w9020(struct wacom_i2c *wac_i2c, char *flash_data,
				    unsigned long ulAddress, u8 *pcommand_id,
				    int *ECH)
{
	const int MAX_COM_SIZE = (8 + FLASH_BLOCK_SIZE + 2);	/* 8: num of command[0] to command[7] */
	/* FLASH_BLOCK_SIZE: unit to erase the block */
	/* Num of Last 2 checksums */
	bool bRet = false;
	u8 command[300] = { 0 };
	unsigned char sum = 0;
	int i = 0;

	command[0] = BOOT_CMD_REPORT_ID;		/* Report:ReportID */
	command[1] = BOOT_WRITE_FLASH;			/* Report:program  command */
	command[2] = *ECH = ++(*pcommand_id);		/* Report:echo */
	command[3] = ulAddress & 0x000000ff;
	command[4] = (ulAddress & 0x0000ff00) >> 8;
	command[5] = (ulAddress & 0x00ff0000) >> 16;
	command[6] = (ulAddress & 0xff000000) >> 24;	/* Report:address(4bytes) */
	command[7] = 0x20;

	/*Preliminarily stored data that cannnot appear here, but in wacom_set_feature() */
	sum = (0x05 + 0x0c + 0x01);

	for (i = 0; i < 8; i++)
		sum += command[i];

	command[MAX_COM_SIZE - 2] = ~sum + 1;	/* Report:command checksum */

	sum = 0;

	for (i = 8; i < (FLASH_BLOCK_SIZE + 8); i++) {
		command[i] = flash_data[ulAddress + (i - 8)];
		sum += flash_data[ulAddress + (i - 8)];
	}

	command[MAX_COM_SIZE - 1] = ~sum + 1;	/* Report:data checksum */

	/*Subtract 8 for the first 8 bytes */
	bRet = wacom_i2c_set_feature(wac_i2c, REPORT_ID_1, (BOOT_CMD_SIZE + 4 - 8),
					command, COMM_REG, DATA_REG);
	if (!bRet) {
		input_err(true, &wac_i2c->client->dev,
			  "%s failed to set feature\n", __func__);
		return false;
	}

	udelay(50);

	return true;
}

static bool flash_write_w9020(struct wacom_i2c *wac_i2c,
			      unsigned char *flash_data,
			      unsigned long start_address,
			      unsigned long *max_address)
{
	bool bRet = false;
	u8 command_id = 0;
	u8 response[BOOT_RSP_SIZE] = { 0 };
	int i = 0, j = 0, ECH = 0, ECH_len = 0;
	int ECH_ARRAY[3] = { 0 };
	int ret = -1;
	unsigned long ulAddress = 0;

	j = 0;
	for (ulAddress = start_address; ulAddress < *max_address; ulAddress += FLASH_BLOCK_SIZE) {
		for (i = 0; i < FLASH_BLOCK_SIZE; i++) {
			if (flash_data[ulAddress + i] != 0xFF)
				break;
		}
		if (i == (FLASH_BLOCK_SIZE))
			continue;

		bRet = flash_write_block_w9020(wac_i2c, flash_data, ulAddress,
						&command_id, &ECH);
		if (!bRet)
			return false;

		if (ECH_len == 3)
			ECH_len = 0;

		ECH_ARRAY[ECH_len++] = ECH;
		if (ECH_len == 3) {
			for (j = 0; j < 3; j++) {
				do {
					bRet = wacom_i2c_get_feature(wac_i2c,
								  REPORT_ID_2,
								  BOOT_RSP_SIZE,
								  response,
								  COMM_REG,
								  DATA_REG, 50);
					if (!bRet) {
						input_err(true,
							  &wac_i2c->client->dev,
							  "%s failed to set feature\n",
							  __func__);
						return false;
					}

					ret = check_progress(wac_i2c, &response[1], (BOOT_RSP_SIZE - 3), 0x01, ECH_ARRAY[j]);
					if (ret < 0) {
						input_err(true, &wac_i2c->client->dev,
							  "%s mismatched echo array\n",
							  __func__);
						return false;
					}
				} while (ret == PROCESS_INPROGRESS);
			}
		}
	}

	return true;
}

static int wacom_i2c_flash_w9020(struct wacom_i2c *wac_i2c, unsigned char *fw_data)
{
	struct i2c_client *client = wac_i2c->client;
	bool bRet = false;
	int iBLVer = 0, iMpuType = 0;
	unsigned long max_address = W9020_END_ADDR;	/* Max.address of Load data */
	unsigned long start_address = W9020_START_ADDR;	/* Start.address of Load data */

	/*Obtain boot loader version */
	if (!flash_blver_w9020(wac_i2c, &iBLVer)) {
		input_err(true, &client->dev,
			  "%s failed to get Boot Loader version\n", __func__);
		return -EXIT_FAIL_GET_BOOT_LOADER_VERSION;
	}
	input_info(true, &client->dev, "BL version: %x\n", iBLVer);

	/*Obtain MPUtype: this can be manually done in user space */
	if (!flash_mputype_w9020(wac_i2c, &iMpuType)) {
		input_err(true, &client->dev,
			  "%s failed to get MPU type\n", __func__);
		return -EXIT_FAIL_GET_MPU_TYPE;
	}
	if (iMpuType != MPU_W9020) {
		input_err(true, &client->dev,
			  "MPU is not for W9020 : %x\n", iMpuType);
		return -EXIT_FAIL_GET_MPU_TYPE;
	}
	input_info(true, &client->dev, "MPU type: %x\n", iMpuType);

	/*-----------------------------------*/
	/*Flashing operation starts from here */

	/*Erase the current loaded program */
	input_info(true, &client->dev,
		   "%s erasing the current firmware\n", __func__);
	bRet = flash_erase_all(wac_i2c);
	if (!bRet) {
		input_err(true, &client->dev,
			  "%s failed to erase the user program\n", __func__);
		return -EXIT_FAIL_ERASE;
	}

	/*Write the new program */
	input_info(true, &client->dev, "%s writing new firmware\n",
		   __func__);
	bRet = flash_write_w9020(wac_i2c, wac_i2c->fw_data, start_address,
			      &max_address);
	if (!bRet) {
		input_err(true, &client->dev,
			  "%s failed to write firmware\n", __func__);
		return -EXIT_FAIL_WRITE_FIRMWARE;
	}

	/*Return to the user mode */
	input_info(true, &client->dev, "%s closing the boot mode\n",
		   __func__);
	bRet = flash_end_w9020(wac_i2c);
	if (!bRet) {
		input_err(true, &client->dev,
			  "%s closing boot mode failed\n", __func__);
		return -EXIT_FAIL_WRITING_MARK_NOT_SET;
	}

	input_info(true, &client->dev,
		   "%s write and verify completed\n", __func__);

	return EXIT_OK;
}

int wacom_i2c_flash(struct wacom_i2c *wac_i2c)
{
	int ret;

	if (wac_i2c->fw_data == NULL) {
		input_err(true, &wac_i2c->client->dev,
			  "epen:Data is NULL. Exit.\n");
		return -EINVAL;
	}

	wacom_compulsory_flash_mode(wac_i2c, true);
	wacom_reset_hw(wac_i2c);

	ret = wacom_flash_cmd(wac_i2c);
	if (ret < 0) {
		input_err(true, &wac_i2c->client->dev,
			  "epen:%s cannot send flash command\n", __func__);
		ret = -EXIT_FAIL;
		goto out;
	}

	ret = flash_query_w9020(wac_i2c);
	if (ret < 0) {
		input_err(true, &wac_i2c->client->dev,
			  "epen:%s Error: cannot send query\n", __func__);
		ret = -EXIT_FAIL;
		goto out;
	}

	ret = wacom_i2c_flash_w9020(wac_i2c, wac_i2c->fw_data);
	if (ret < 0) {
		input_err(true, &wac_i2c->client->dev,
			  "epen:%s Error: flash failed\n", __func__);
		ret = -EXIT_FAIL;
		goto out;
	}

	msleep(200);

out:
	wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
	wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;

	wacom_compulsory_flash_mode(wac_i2c, false);
	wacom_reset_hw(wac_i2c);

	return ret;
}
