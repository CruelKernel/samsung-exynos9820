/* --------------- (C) COPYRIGHT 2012 STMicroelectronics ----------------------
 *
 * File Name	: stm_fsr_fwu.c
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
 * 01/23/2019| Change WRITE_CHUNK_SIZE_D3 to 1024 to speed up
 * 01/28/2019| Change Reset command to fsr_systemreset(info, 10)
 * -----------------------------------------------------------------------------
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "stm_fsr_sidekey.h"

#define FSR_DEFAULT_UMS_FW	"/sdcard/Firmware/stm/sidekey.fw"
#define FSR_DEFAULT_FFU_FW	"ffu_sidekey.bin"
#define FW_IMAGE_NAME		"stm/sidekey.fw"
#define	FW_IMAGE_SIZE_D3	(256 * 1024)
#define FTBFILE_SIGNATURE 	0xAA55AA55

#define FSR_MAX_FW_PATH	64


/* Location for config version in binary file with header */
#define CONFIG_OFFSET_BIN_D3	0x3F022

#define	SIGNEDKEY_SIZE			(256)

enum {
	BUILT_IN = 0,
	UMS,
	NONE,
	FFU,
};

struct ftb_header {
	unsigned int signature;
	unsigned int ftb_ver;
	unsigned int target;
	unsigned int fw_id;
	unsigned int fw_ver;
	unsigned int cfg_id;
	unsigned int cfg_ver;
	unsigned int reseved1;
	unsigned int reseved2;
	unsigned int bl_fw_ver;
	unsigned int ext_rel_ver;
	unsigned int sec0_size;
	unsigned int sec1_size;
	unsigned int sec2_size;
	unsigned int sec3_size;
	unsigned int hdr_crc;
};

int FSR_Check_DMA_Done(struct fsr_sidekey_info *info)
{
	int timeout = 60;
	unsigned char regAdd[2] = { 0xF9, 0x05};
	unsigned char val[1];

	do {
		fsr_read_reg(info, &regAdd[0], 2, (unsigned char*)val, 1);

		if ((val[0] & 0x80) != 0x80)
			break;

		fsr_delay(50);
		timeout--;
	} while (timeout != 0);

	if (timeout == 0)
		return -1;

	return 0;
}

static int FSR_Check_Erase_Done(struct fsr_sidekey_info *info, unsigned char page)
{
	int timeout = 60;  // 3 sec timeout
	unsigned char regAdd[2] = {0xF9, 0x02};
	unsigned char val[1];

	fsr_read_reg(info, &regAdd[0], 2, (unsigned char*)val, 1);
	if ((val[0] & 0x80) != 0x80) {
		input_err(true, &info->client->dev, "%s: Flash erasing mode has been exited while erasing page %d,%d\n",
			__func__, val[0], page);
		return -FSR_ERROR_FW_UPDATE_FAIL;
	}
	fsr_delay(50);

	do {
		fsr_read_reg(info, &regAdd[0], 2, (unsigned char*)val, 1);
		if (val[0] == page)
			break;

		fsr_delay(50);
		timeout--;
	} while (timeout != 0);

	if (timeout == 0) {
		input_err(true, &info->client->dev, "%s: Time Over page %d\n", __func__, page);
		return -FSR_ERROR_TIMEOUT;
	}

	return 0;
}

static int fsr_fw_burn_d3(struct fsr_sidekey_info *info, unsigned char *fw_data)
{
	int rc;
	const unsigned long int FSR_CONFIG_SIZE = (4 * 1024);	// Total 4kB for Config
	const unsigned long int DRAM_LEN = (64 * 1024);	// 64KB
	const unsigned int CODE_ADDR_START = (0x0000);
	const unsigned int CONFIG_ADDR_START = (0xFC00);
	const unsigned int WRITE_CHUNK_SIZE_D3 = 2048;

	unsigned char *config_data = NULL;

	unsigned long int size = 0;
	unsigned long int i;
	unsigned long int j;
	unsigned long int k;
	unsigned long int r;
	unsigned long int dataLen;
	unsigned long int len = 0;
	unsigned long int writeAddr = 0;
	unsigned char *buf = NULL;
	unsigned char regAdd[8] = {0};
	int cnt;

	const struct ftb_header *header;
	header = (struct ftb_header *)fw_data;

	// Enable warm boot CRC check  (w B6 00 1E 38)
	input_info(true, &info->client->dev, "%s: Enable warm boot CRC check\n", __func__);
	regAdd[0] = 0xB6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x1E;
	regAdd[3] = 0x38;
	fsr_write_reg(info, &regAdd[0], 4);
	fsr_delay(20);

	//==================== System reset ====================
	if (info->board->rst_invalid) {
		//System Reset ==> B6 00 28 80
		input_info(true, &info->client->dev, "%s: System reset\n", __func__);
		regAdd[0] = 0xB6;
		regAdd[1] = 0x00;
		regAdd[2] = 0x28;
		regAdd[3] = 0x80;
		fsr_write_reg(info, &regAdd[0], 4);
	} else {
		fsr_systemreset_gpio(info, 50, false);
	}

	//==================== M3 NO SLEEP ====================
	input_info(true, &info->client->dev, "%s: M3 NO SLEEP\n", __func__);
	regAdd[0] = 0xB6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x1F;
	regAdd[3] = 0x80;
	fsr_write_reg(info, &regAdd[0], 4);
	if (info->board->rst_invalid) {
		fsr_delay(50);
	}

	//==================== M3 HOLD ====================
	input_info(true, &info->client->dev, "%s: M3 HOLD\n", __func__);
	regAdd[0] = 0xB6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x28;
	regAdd[3] = 0x01;
	fsr_write_reg(info, &regAdd[0], 4);

	//==================== Disable Watchdog warm reset ====================
	input_info(true, &info->client->dev, "%s: Disable Watchdog warm reset\n", __func__);
	regAdd[0] = 0xB6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x21;
	regAdd[3] = 0x00;
	fsr_write_reg(info, &regAdd[0], 4);

	//==================== Disable Watchdog ====================
	input_info(true, &info->client->dev, "%s: Disable Watchdog\n", __func__);
	regAdd[0] = 0xB6;
	regAdd[1] = 0x00;
	regAdd[2] = 0x4A;
	regAdd[3] = 0x00;
	fsr_write_reg(info, &regAdd[0], 4);

	//==================== Unlock Flash ====================
	//Unlock Flash Command ==> F7 74 45
	input_info(true, &info->client->dev, "%s: Unlock Flash\n", __func__);
	regAdd[0] = 0xF7;
	regAdd[1] = 0x74;
	regAdd[2] = 0x45;
	fsr_write_reg(info, &regAdd[0], 3);
	fsr_delay(100);

	//==================== Unlock Erase Operation ====================
	input_info(true, &info->client->dev, "%s: Unlock Erase Operation\n", __func__);
	regAdd[0] = 0xFA;
	regAdd[1] = 0x72;
	regAdd[2] = 0x01;
	fsr_write_reg(info, &regAdd[0], 3);
	fsr_delay(100);

	//==================== Erase Partial Flash ====================
	input_info(true, &info->client->dev, "%s: Erase Partial Flash\n", __func__);
	for (i = 0; i < 64; i++) {
		if ( (i == 61) || (i == 62) )   // skip CX2 area (page 61 and page 62)
			continue;

		regAdd[0] = 0xFA;
		regAdd[1] = 0x02;
		regAdd[2] = (0x80 + i) & 0xFF;
		fsr_write_reg(info, &regAdd[0], 3);
		rc = FSR_Check_Erase_Done(info, i);
		if (rc < 0) {
			input_info(true, &info->client->dev, "%s: Retry erasing page %d\n", __func__, i);

			//==================== Unlock Flash ====================
			//Unlock Flash Command ==> F7 74 45
			input_info(true, &info->client->dev, "%s: Unlock Flash\n", __func__);
			regAdd[0] = 0xF7;
			regAdd[1] = 0x74;
			regAdd[2] = 0x45;
			fsr_write_reg(info, &regAdd[0], 3);
			fsr_delay(100);

			//==================== Unlock Erase Operation ====================
			input_info(true, &info->client->dev, "%s: Unlock Erase Operation\n", __func__);
			regAdd[0] = 0xFA;
			regAdd[1] = 0x72;
			regAdd[2] = 0x01;
			fsr_write_reg(info, &regAdd[0], 3);
			fsr_delay(100);

			regAdd[0] = 0xFA;
			regAdd[1] = 0x02;
			regAdd[2] = (0x80 + i) & 0xFF;
			fsr_write_reg(info, &regAdd[0], 3);
			rc = FSR_Check_Erase_Done(info, i);
			if (rc < 0) {
				input_err(true, &info->client->dev, "%s: failed, %d\n", __func__, __LINE__);
				return rc;
			}
		}
	}

	//==================== Unlock Flash ====================
	//Unlock Flash Command ==> F7 74 45
	input_info(true, &info->client->dev, "%s: Unlock Flash\n", __func__);
	regAdd[0] = 0xF7;
	regAdd[1] = 0x74;
	regAdd[2] = 0x45;
	fsr_write_reg(info, &regAdd[0], 3);
	fsr_delay(20);

	//==================== Unlock Programming operation ====================
	input_info(true, &info->client->dev, "%s: Unlock Programming operation\n", __func__);
	regAdd[0] = 0xFA;
	regAdd[1] = 0x72;
	regAdd[2] = 0x02;
	fsr_write_reg(info, &regAdd[0], 3);

	//========================== Write to FLASH ==========================
	// Main Code Programming
	input_info(true, &info->client->dev, "%s: Write to FLASH\n", __func__);
	buf = kzalloc(WRITE_CHUNK_SIZE_D3 + 3, GFP_KERNEL);
	if (!buf) {
		input_err(true, &info->client->dev, "%s: failed, %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	i = 0;
	k = 0;

	size = header->sec0_size;
	while(i < size) {
		j = 0;
		dataLen = size - i;

		r = dataLen;
		if (r>DRAM_LEN) r=DRAM_LEN;

		while ((j < DRAM_LEN) && (j < dataLen)) {	//DRAM_LEN = 64*1024
			writeAddr = j & 0xFFFF;

			cnt = 0;
			buf[cnt++] = 0xF8;
			buf[cnt++] = (writeAddr >> 8) & 0xFF;
			buf[cnt++] = (writeAddr >> 0) & 0xFF;

			if (r>=WRITE_CHUNK_SIZE_D3) {
				memcpy(&buf[cnt], &fw_data[sizeof(struct ftb_header) + i], WRITE_CHUNK_SIZE_D3);
				r -= WRITE_CHUNK_SIZE_D3;
			} else {
				memset(&buf[cnt], 0xff, WRITE_CHUNK_SIZE_D3-3);
				memcpy(&buf[cnt], &fw_data[sizeof(struct ftb_header) + i], r);
				r = 0;
			}
			cnt += WRITE_CHUNK_SIZE_D3;

			fsr_write_reg(info, &buf[0], cnt);

			i += WRITE_CHUNK_SIZE_D3;
			j += WRITE_CHUNK_SIZE_D3;
		}
		input_info(true, &info->client->dev, "%s: Write to Flash - Total %ld bytes\n", __func__, i);

		 //===================configure flash DMA=====================
		len = j / 4 - 1; 	// 64*1024 / 4 - 1

		buf[0] = 0xFA;
		buf[1] = 0x06;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = (CODE_ADDR_START +( (k * DRAM_LEN) >> 2)) & 0xFF;			// k * 64 * 1024 / 4
		buf[5] = (CODE_ADDR_START + ((k * DRAM_LEN) >> (2+8))) & 0xFF;		// k * 64 * 1024 / 4 / 256
		buf[6] = (len >> 0) & 0xFF;    //DMA length in word
		buf[7] = (len >> 8) & 0xFF;    //DMA length in word
		buf[8] = 0x00;
		fsr_write_reg(info, &buf[0], 9);

		fsr_delay(100);

		//===================START FLASH DMA=====================
		buf[0] = 0xFA;
		buf[1] = 0x05;
		buf[2] = 0xC0;
		fsr_write_reg(info, &buf[0], 3);

		rc = FSR_Check_DMA_Done(info);
		if (rc < 0) {
			kfree(buf);
			input_err(true, &info->client->dev, "%s: failed, %d\n", __func__, __LINE__);
			return rc;
		}
		k++;
	}
	input_info(true, &info->client->dev, "%s: Total write %ld kbytes for Main Code\n", __func__, i / 1024);

	//=============================================================
	// Config Programming
	//=============================================================

	config_data = kzalloc(FSR_CONFIG_SIZE, GFP_KERNEL);
	if (!config_data) {
		kfree(buf);
		input_err(true, &info->client->dev, "%s: failed, %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	memcpy(&config_data[0], &fw_data[sizeof(struct ftb_header) + (header->sec0_size)], FSR_CONFIG_SIZE);

	i = 0;
	size = FSR_CONFIG_SIZE;
	j = 0;
	while ((j < DRAM_LEN) && (j < size)) {		//DRAM_LEN = 64*1024
		writeAddr = j & 0xFFFF;

		cnt = 0;
		buf[cnt++] = 0xF8;
		buf[cnt++] = (writeAddr >> 8) & 0xFF;
		buf[cnt++] = (writeAddr >> 0) & 0xFF;

		memcpy(&buf[cnt], &config_data[i], WRITE_CHUNK_SIZE_D3);
		cnt += WRITE_CHUNK_SIZE_D3;

		fsr_write_reg(info, &buf[0], cnt);

		i += WRITE_CHUNK_SIZE_D3;
		j += WRITE_CHUNK_SIZE_D3;
	}
	kfree(config_data);

	//===================configure flash DMA=====================
	input_info(true, &info->client->dev, "%s: configure flash DMA\n", __func__);
	len = j / 4 - 1;

	buf[0] = 0xFA;
	buf[1] = 0x06;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = (CONFIG_ADDR_START) & 0xFF;
	buf[5] = (CONFIG_ADDR_START >> 8) & 0xFF;
	buf[6] = (len >> 0) & 0xFF;    //DMA length in word
	buf[7] = (len >> 8) & 0xFF;    //DMA length in word
	buf[8] = 0x00;
	fsr_write_reg(info, &buf[0], 9);

	fsr_delay(100);

	//===================START FLASH DMA=====================
	input_info(true, &info->client->dev, "%s: START FLASH DMA\n", __func__);
	buf[0] = 0xFA;
	buf[1] = 0x05;
	buf[2] = 0xC0;
	fsr_write_reg(info, &buf[0], 3);

	kfree(buf);

	rc = FSR_Check_DMA_Done(info);
	if (rc < 0) {
		input_err(true, &info->client->dev, "%s: failed, %d\n", __func__, __LINE__);
		return rc;
	}

	input_info(true, &info->client->dev, "%s: Total write %ld kbytes for Config\n", __func__, i / 1024);

	//==================== System reset ====================

	fsr_systemreset(info, 20);

	return 0;
}

int fsr_fw_wait_for_event(struct fsr_sidekey_info *info, unsigned char eid)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FSR_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FSR_EVENT_SIZE);

	regAdd = CMD_READ_EVENT;
	rc = -1;
	while (fsr_read_reg(info, &regAdd, 1, (unsigned char *)data, FSR_EVENT_SIZE)) {
		input_err(true, &info->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		if (data[0] == EID_STATUS_EVENT || data[0] == EID_ERROR) {
			if ((data[0] == EID_STATUS_EVENT) && (data[1] == eid)) {
				rc = 0;
				break;
			}
		}
		if (retry++ > FSR_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, &info->client->dev, "%s: Time Over (%2X,%2X,%2X,%2X)\n",
				__func__, data[0], data[1], data[2], data[3]);
			break;
		}
		fsr_delay(20);
	}

	return rc;
}

int fsr_fw_wait_for_event_D3(struct fsr_sidekey_info *info, unsigned char eid0, unsigned char eid1)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FSR_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FSR_EVENT_SIZE);

	regAdd = CMD_READ_EVENT;
	rc = -1;
	while (fsr_read_reg(info, &regAdd, 1, (unsigned char *)data, FSR_EVENT_SIZE)) {
		input_err(true, &info->client->dev, "%s: %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		if (data[0] == EID_STATUS_EVENT || data[0] == EID_ERROR) {
			if ((data[0] == EID_STATUS_EVENT) && (data[1] == eid0) && (data[2] == eid1)) {
				rc = 0;
				break;
			}
		}
		if (retry++ > FSR_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, &info->client->dev, "%s: Time Over (%2X,%2X,%2X,%2X)\n",
				__func__, data[0], data[1], data[2], data[3]);
			break;
		}
		fsr_delay(20);
	}

	return rc;
}

/*
static int fsr_fw_wait_for_specific_event(struct fsr_sidekey_info *info, unsigned char eid0, unsigned char eid1, unsigned char eid2)
{
	int rc;
	unsigned char regAdd;
	unsigned char data[FSR_EVENT_SIZE];
	int retry = 0;

	memset(data, 0x0, FSR_EVENT_SIZE);

	regAdd = CMD_READ_EVENT;
	rc = -1;
	while (fsr_read_reg(info, &regAdd, 1, (unsigned char *)data, FSR_EVENT_SIZE)) {
		if (data[0]) {
			if ((data[0] == eid0) && (data[1] == eid1) && (data[2] == eid2)) {
				rc = 0;
				break;
			} else {
				input_info(true, &info->client->dev, "%s: %2X,%2X,%2X,%2X\n",
					__func__, data[0], data[1], data[2], data[3]);
			}
		}
		if (retry++ > FSR_RETRY_COUNT * 15) {
			rc = -1;
			input_err(true, &info->client->dev, "%s: Time Over ( %2X,%2X,%2X,%2X )\n",
				__func__, data[0], data[1], data[2], data[3]);
			break;
		}
		fsr_delay(20);
	}

	return rc;
}

static void fsr_fw_set_pure_autotune(struct fsr_sidekey_info *info)
{
	int result;
	unsigned char regAdd;
	unsigned char data[FSR_EVENT_SIZE];
	int retry = 0;

	input_info(true, &info->client->dev, "%s\n", __func__);

	fsr_command(info, CMD_SET_PURE_AUTOTUNE);

	memset(data, 0x0, FSR_EVENT_SIZE);

	regAdd = CMD_READ_EVENT;
	result = -1;
	while (fsr_read_reg(info, &regAdd, 1, (unsigned char *)data, FSR_EVENT_SIZE)) {
		if (data[0]) {
			if ((data[0] == EID_STATUS_EVENT) && (data[1] == STATUS_EVENT_PURE_SET_DONE)) {
				result = data[2];
				break;
			} else {
				input_info(true, &info->client->dev, "%s: %2X,%2X,%2X,%2X\n",
					__func__, data[0], data[1], data[2], data[3]);
			}
		}
		if (retry++ > FSR_RETRY_COUNT * 15) {
			result = -1;
			input_err(true, &info->client->dev, "%s: Time Over ( %2X,%2X,%2X,%2X )\n",
				__func__, data[0], data[1], data[2], data[3]);
			break;
		}
		fsr_delay(20);
	}

	if (result == 0) {
		input_info(true, &info->client->dev, "%s The pure set is already set.\n", __func__);
	} else if (result == 1) {
		input_info(true, &info->client->dev, "%s The pure set is set successfully\n", __func__);
	}
}*/

int fsr_execute_autotune(struct fsr_sidekey_info *info)
{
	int rc = 0;

	input_info(true, &info->client->dev, "%s\n", __func__);

	fsr_command(info, CMD_CLEAR_ALL_EVENT);
	fsr_delay(50);

	fsr_command(info, CMD_SENSEOFF);
	fsr_interrupt_set(info, INT_DISABLE);
	fsr_delay(50);

	fsr_command(info, CMD_TRIM_LOW_POWER_OSC);
	fsr_fw_wait_for_event_D3(info, STATUS_EVENT_CAL_DONE, 0x00);

	fsr_command(info, CMD_PERFORM_AUTOTUNE);
	fsr_fw_wait_for_event_D3(info, STATUS_EVENT_CAL_DONE, 0x01);

	fsr_command(info, CMD_SAVE_TUNNING_DATA);
	rc = fsr_fw_wait_for_event(info, STATUS_EVENT_SAVE_CX_DONE);

	// Check Final AFE
	//if (info->fsr_sys_info.finalAFE > 0) {
	//	fsr_fw_set_pure_autotune(info);
	//}

	fsr_command(info, CMD_SENSEON);
	fsr_delay(20);

	fsr_command(info, CMD_NOTI_AP_BOOTUP);
	fsr_delay(10);

	fsr_interrupt_set(info, INT_ENABLE);
	fsr_delay(20);

	return rc;
}

void fsr_fw_init(struct fsr_sidekey_info *info)
{
	fsr_read_system_info(info);

	input_info(true, &info->client->dev, "%s: %d\n", __func__, info->fsr_sys_info.PureSetFlag);

//	if (info->fsr_sys_info.PureSetFlag == 0) {
		fsr_execute_autotune(info);
//	}

	fsr_interrupt_set(info, INT_ENABLE);
	fsr_delay(20);
}

const int fsr_fw_updater(struct fsr_sidekey_info *info, unsigned char *fw_data, int restore_cal)
{
	const struct ftb_header *header;
	int retval;
	int retry;
	unsigned short fw_main_version;

	if (!fw_data) {
		input_err(true, &info->client->dev, "%s: Firmware data is NULL\n",
			__func__);
		return -ENODEV;
	}

	info->fwup_is_on_going = true;

	header = (struct ftb_header *)fw_data;
	fw_main_version = header->ext_rel_ver;

	input_info(true, &info->client->dev,
		  "%s: Starting firmware update : 0x%04X\n", __func__,
		  fw_main_version);

	fsr_interrupt_set(info, INT_DISABLE);

	retry = 0;
	while (1) {
		retval = fsr_fw_burn_d3(info, fw_data);
		if (retval >= 0) {
			info->product_id_of_ic = 0;
			info->fw_version_of_ic = 0;
			info->config_version_of_ic = 0;
			info->fw_main_version_of_ic = 0;

			fsr_get_version_info(info);

			if (fw_main_version == info->fw_main_version_of_ic) {
				input_info(true, &info->client->dev,
					  "%s: Success Firmware update\n",
					  __func__);

				fsr_fw_init(info);
				retval = 0;
				break;
			}
		}

		if (++retry > 3) {
			input_err(true, &info->client->dev, "%s: Fail Firmware update\n",
				 __func__);
			retval = -1;
			break;
		}
	}

	info->fwup_is_on_going = false;
	fsr_interrupt_set(info, INT_ENABLE);
	return retval;
}
EXPORT_SYMBOL(fsr_fw_updater);

int fsr_fw_update_on_probe(struct fsr_sidekey_info *info)
{
	int retval = 0;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;
	char fw_path[FSR_MAX_FW_PATH];
	const struct ftb_header *header;
	int restore_cal = 0;

	if (info->board->bringup == 1)
		return FSR_NOT_ERROR;

	if (info->board->firmware_name) {
		info->firmware_name = info->board->firmware_name;
	}
	else {
		input_err(true, &info->client->dev, "%s: firmware name does not declair in dts\n", __func__);
		goto exit_fwload;
	}

	snprintf(fw_path, FSR_MAX_FW_PATH, "%s", info->firmware_name);
	input_info(true, &info->client->dev, "%s: Load firmware : %s\n", __func__, fw_path);

	retval = request_firmware(&fw_entry, fw_path, &info->client->dev);
	if (retval) {
		input_err(true, &info->client->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct ftb_header *)fw_data;

	/*if (header->signature == FTBFILE_SIGNATURE) {
		input_err(true, &info->client->dev,
			"%s: Firmware image %s not available for FTS D3\n", __func__,
			fw_path);
		goto done;
	}*/

	info->fw_version_of_bin = header->fw_ver;
	info->fw_main_version_of_bin = header->ext_rel_ver;
	info->config_version_of_bin = header->cfg_ver;

	input_info(true, &info->client->dev,
		"%s: [BIN] Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n", __func__,
		info->fw_version_of_bin,
		info->config_version_of_bin,
		info->fw_main_version_of_bin);

	if (info->board->bringup == 2) {
		input_err(true, &info->client->dev, "%s: skip fw_update for bringup\n", __func__);
		retval = FSR_NOT_ERROR;
		goto done;
	}

	if ((info->fw_main_version_of_ic < info->fw_main_version_of_bin)
		|| (info->config_version_of_ic < info->config_version_of_bin)
		|| (info->fw_version_of_ic < info->fw_version_of_bin))
		retval = fsr_fw_updater(info, fw_data, restore_cal);
	else
		retval = FSR_NOT_ERROR;

done:
	if (fw_entry)
		release_firmware(fw_entry);
exit_fwload:
	return retval;
}
EXPORT_SYMBOL(fsr_fw_update_on_probe);

static int fsr_load_fw_from_kernel(struct fsr_sidekey_info *info,
				 const char *fw_path)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;

	if (info->board->bringup == 1) {
		input_err(true, &info->client->dev, "%s: can't update for bringup\n",
			__func__);
		return -EINVAL;
	}

	if (!fw_path) {
		input_err(true, &info->client->dev, "%s: Firmware name is not defined\n",
			__func__);
		return -EINVAL;
	}

	input_info(true, &info->client->dev, "%s: Load firmware : %s\n", __func__,
		 fw_path);

	retval = request_firmware(&fw_entry, fw_path, &info->client->dev);

	if (retval) {
		input_err(true, &info->client->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;

	if (info->irq >= 0)
		disable_irq(info->irq);

	fsr_systemreset(info, 20);

	/* use virtual pat_control - magic cal 1 */
	retval = fsr_fw_updater(info, fw_data, 1);
	if (retval)
		input_err(true, &info->client->dev, "%s: failed update firmware\n",
			__func__);

	if (info->irq >= 0)
		enable_irq(info->irq);
 done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

static int fsr_load_fw_from_ums(struct fsr_sidekey_info *info)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fw_size, nread;
	int error = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(FSR_DEFAULT_UMS_FW, O_RDONLY, S_IRUSR);
	if (IS_ERR(fp)) {
		input_err(true, &info->client->dev, "%s: failed to open %s.\n", __func__,
			FSR_DEFAULT_UMS_FW);
		error = -ENOENT;
		goto open_err;
	}

	fw_size = fp->f_path.dentry->d_inode->i_size;

	if (0 < fw_size) {
		unsigned char *fw_data;
		const struct ftb_header *header;

		fw_data = kzalloc(fw_size, GFP_KERNEL);
		if (!fw_data) {
			input_err(true, &info->client->dev, "%s: failed to alloc mem\n",
					__func__);
			error = -ENOMEM;
			goto alloc_err;
		}

		nread = vfs_read(fp, (char __user *)fw_data,
				 fw_size, &fp->f_pos);

		input_info(true, &info->client->dev,
			 "%s: start, file path %s, size %ld Bytes\n",
			 __func__, FSR_DEFAULT_UMS_FW, fw_size);

		if (nread != fw_size) {
			input_err(true, &info->client->dev,
				"%s: failed to read firmware file, nread %ld Bytes\n",
				__func__, nread);
			error = -EIO;
		} else {
			header = (struct ftb_header *)fw_data;
			if (header->signature == FTBFILE_SIGNATURE) {

				if (info->irq >= 0)
					disable_irq(info->irq);

				fsr_systemreset(info, 20);

				input_info(true, &info->client->dev,
					"%s: [UMS] Firmware Ver: 0x%04X, Main Version : 0x%04X\n",
					__func__, header->fw_ver,
					header->ext_rel_ver);

				/* use virtual pat_control - magic cal 1 */
				error = fsr_fw_updater(info, fw_data, 1);

				if (info->irq >= 0)
					enable_irq(info->irq);

			} else {
				error = -1;
				input_err(true, &info->client->dev,
					 "%s: File type is not match with FTS64 file. [%8x]\n",
					 __func__, header->signature);
			}
		}

		if (error < 0)
			input_err(true, &info->client->dev, "%s: failed update firmware\n",
				__func__);

		kfree(fw_data);
	}

alloc_err:
	filp_close(fp, NULL);

 open_err:
	set_fs(old_fs);
	return error;
}

static int fsr_load_fw_from_ffu(struct fsr_sidekey_info *info)
{
	int retval;
	const struct firmware *fw_entry = NULL;
	unsigned char *fw_data = NULL;
	const char *fw_path = FSR_DEFAULT_FFU_FW;
	const struct ftb_header *header;

	if (!fw_path) {
		input_err(true, &info->client->dev, "%s: Firmware name is not defined\n",
			__func__);
		return -EINVAL;
	}

	input_info(true, &info->client->dev, "%s: Load firmware : %s\n", __func__,
		 fw_path);

	retval = request_firmware(&fw_entry, fw_path, &info->client->dev);

	if (retval) {
		input_err(true, &info->client->dev,
			"%s: Firmware image %s not available\n", __func__,
			fw_path);
		goto done;
	}

	fw_data = (unsigned char *)fw_entry->data;
	header = (struct ftb_header *)fw_data;

	info->fw_version_of_bin = header->fw_ver;
	info->fw_main_version_of_bin = header->ext_rel_ver;
	info->config_version_of_bin = header->cfg_ver;

	input_info(true, &info->client->dev,
		"%s: [FFU] Firmware Ver: 0x%04X, Config Ver: 0x%04X, Main Ver: 0x%04X\n",
		__func__, info->fw_version_of_bin, info->config_version_of_bin,
		info->fw_main_version_of_bin);

	if (info->irq >= 0)
		disable_irq(info->irq);

	fsr_systemreset(info, 20);

	/* use virtual pat_control - magic cal 0 */
	retval = fsr_fw_updater(info, fw_data, 0);
	if (retval)
		input_err(true, &info->client->dev, "%s: failed update firmware\n", __func__);

	if (info->irq >= 0)
		enable_irq(info->irq);

done:
	if (fw_entry)
		release_firmware(fw_entry);

	return retval;
}

int fsr_fw_update_on_hidden_menu(struct fsr_sidekey_info *info, int update_type)
{
	int retval = 0;

	/* Factory cmd for firmware update
	 * argument represent what is source of firmware like below.
	 *
	 * 0 : [BUILT_IN] Getting firmware which is for user.
	 * 1 : [UMS] Getting firmware from sd card.
	 * 2 : none
	 * 3 : [FFU] Getting firmware from air.
	 */
	switch (update_type) {
	case BUILT_IN:
		retval = fsr_load_fw_from_kernel(info, info->firmware_name);
		break;

	case UMS:
		retval = fsr_load_fw_from_ums(info);
		break;

	case FFU:
		retval = fsr_load_fw_from_ffu(info);
		break;

	default:
		input_err(true, &info->client->dev, "%s: Not support command[%d]\n",
			__func__, update_type);
		break;
	}

	return retval;
}
EXPORT_SYMBOL(fsr_fw_update_on_hidden_menu);
