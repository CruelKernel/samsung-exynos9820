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
 * 03/05/2019| First Release
 * -----------------------------------------------------------------------------
 */

#include "stm_fsr_sidekey.h"

static ssize_t read_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	unsigned char buff[10] = { 0 };

	snprintf(buff, sizeof(buff), "ST%04X",
				info->fw_main_version_of_ic);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%s", buff);
}

static ssize_t read_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%04X,%04X,%04X",
				info->fw_main_version_of_ic,
				info->fw_version_of_ic,
				info->config_version_of_ic);
}

static ssize_t read_totalcx_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	u16 cxAddr;
	u8 regAdd[3] = {0xD0, 0x00, 0x00};
	u8 ch_cx1_loc[6] = {1, 5, 25, 29, 37, 41};
	struct channel_cx_data_raw ch_cx_data;
	struct cx_data_raw *ch_data;
	u8 *data = (u8 *)&ch_cx_data;
	int retval = 0;
	int i = 0;

	cxAddr = info->fsr_sys_info.compensationAddr + 12; // Add offset value 12.
	regAdd[1] = (cxAddr >> 8) & 0xff;
	regAdd[2] = (cxAddr) & 0xff;
	retval = fsr_read_reg(info, &regAdd[0], 3, (u8 *)&ch_cx_data, sizeof(struct channel_cx_data_raw));
	if (retval < 0) {
		input_err(true, &info->client->dev, "%s: failed [%d]\n", __func__, retval);
		return -1;
	} else {
		for (i = 0; i < 6; i++) {
			ch_data = (struct cx_data_raw *)(data + ch_cx1_loc[i]);
			info->ch_cx_data[i].cx1 = (ch_data->cx1.sign ? (-1 * ch_data->cx1.data) : ch_data->cx1.data);
			info->ch_cx_data[i].cx2 = (ch_data->cx2.sign ? (-1 * ch_data->cx2.data) : ch_data->cx2.data);
			info->ch_cx_data[i].total = (info->ch_cx_data[i].cx1 * G1[info->fsr_sys_info.gain1])
				+ (info->ch_cx_data[i].cx2 * G2[info->fsr_sys_info.gain1][info->fsr_sys_info.gain2]);
			info->ch_cx_data[i].total = info->ch_cx_data[i].total / 1000;
		}
		input_info(true, &info->client->dev, "%s: success [%d]\n", __func__, retval);
	}

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d,%d",
				info->ch_cx_data[1].total, info->ch_cx_data[0].total,
				info->ch_cx_data[3].total, info->ch_cx_data[2].total,
				info->ch_cx_data[5].total, info->ch_cx_data[4].total);
}

static ssize_t read_rawdata_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	struct fsr_frame_data_info fsr_frame_data;

	fsr_read_frame(info, TYPE_RAW_DATA, &fsr_frame_data, true);
	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d,%d",
				fsr_frame_data.ch06, fsr_frame_data.ch12,
				fsr_frame_data.ch07, fsr_frame_data.ch14,
				fsr_frame_data.ch08, fsr_frame_data.ch15);
}

static ssize_t read_jitter_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	const int read_count = 100;
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	u8 regAdd[3] = {0xD0, 0x00, 0x00};
	u8 regData[6*2+1];
	u16 addr;
	int retval = 0;
	int retcnt = 0;
	u16 *data;
	int i;

	// Set to active mode always
	regAdd[1] = 0xAF;
	regAdd[2] = 0x01;
	fsr_write_reg(info, &regAdd[1], 2);

	addr = info->fsr_sys_info.normFrcTouchAddr;
	regAdd[1] = (addr >> 8) & 0xff;
	regAdd[2] = (addr) & 0xff;

	input_info(true, &info->client->dev, "%s: Frame Addr 0x%4x\n", __func__, addr);
	for (i=0; i<read_count; i++) {
		retval = fsr_read_reg(info, &regAdd[0], 3, (u8*)regData, 6*2+1);
		if (retval <= 0) {
			input_err(true, &info->client->dev, "%s: read failed [%d]\n", __func__, retval);
			break;
		}
		data = (u16 *)&regData[1];

		retcnt += snprintf(buf + retcnt, 30, "%x,%x,%x,%x,%x,%x;",
				data[1], data[0],
				data[3], data[2],
				data[5], data[4]);
		fsr_delay(10);
	}

	// Release active mode always
	regAdd[1] = 0xAF;
	regAdd[2] = 0x00;
	fsr_write_reg(info, &regAdd[1], 2);

	return retcnt;
}

static ssize_t read_numberofchannel_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);

	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d",
				info->fsr_sys_info.linelen * info->fsr_sys_info.afelen);
}

// Jitter & Jitter Stdev
static ssize_t read_JitterTest_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct fsr_sidekey_info *info = container_of(sec, struct fsr_sidekey_info, sec);
	u8 ChannelName[6] = {12, 6, 14, 7, 15, 8};
	u8 regAdd;
	u8 data[8];
	s16 timeover = 150; // 10ms x 150 = 1.5 sec
	s16 result[6][2];
	u8 ChannelID;
	s16 Jitter;
	s16 JitterStdev;

	// Sense Off First
	fsr_command(info, CMD_SENSEOFF);

	// Interrupt Disable
	fsr_interrupt_set(info, false);

	// Send command to prepare the jitter specification data.
	fsr_command(info, CMD_SPEC_JITTER);
	fsr_delay(1000);

	// Read the result
	regAdd = CMD_READ_EVENT;
	while (fsr_read_reg(info, &regAdd, 1, (u8*)data, 8)) {
		if (data[0] == EID_SPEC_JITTER_RESULT) {
			ChannelID 	= data[1];
			Jitter 		= (data[2] << 8) + data[3];
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
			fsr_delay(10); // 10ms
		}

		if (timeover-- < 0) {
			input_err(true, &info->client->dev,
				"%s: Time over to read the result of the spec data!!\n", __func__);
			break;
		}
	}

	// IC Reset
	fsr_systemreset(info, 10);

	if (timeover-- < 0) {
		buf = NULL;
		return 0;
	}

	// Channel mapping
	//--------------------------
	// ChannelID | FSR1AD04
	//     0     |   CH12
	//     1     |   CH06
	//     2     |   CH14
	//     3     |   CH07
	//     4     |   CH15
	//     5     |   CH08
	// Output Format
	//-------------------------------------------------------------------------
	//                Jitter              |            Jitter Stdev
	//-------------------------------------------------------------------------
	// CH06, CH12, CH07, CH14, CH08, CH15 | CH06, CH12, CH07, CH14, CH08, CH15
	//-------------------------------------------------------------------------
	return snprintf(buf, SEC_CMD_BUF_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
				result[1][0], result[0][0],
				result[3][0], result[2][0],
				result[5][0], result[4][0],
				result[1][1], result[0][1],
				result[3][1], result[2][1],
				result[5][1], result[4][1]);
}

static DEVICE_ATTR(vendor, 0444, read_vendor_show, NULL);
static DEVICE_ATTR(version, 0444, read_version_show, NULL);
static DEVICE_ATTR(totalcx, 0444, read_totalcx_show, NULL);
static DEVICE_ATTR(rawdata, 0444, read_rawdata_show, NULL);
static DEVICE_ATTR(jitter, 0444, read_jitter_show, NULL);
static DEVICE_ATTR(numberofchannel, 0444, read_numberofchannel_show, NULL);
static DEVICE_ATTR(JitterTest, 0444, read_JitterTest_show, NULL);

static struct attribute *cmd_attributes[] = {
	&dev_attr_vendor.attr,
	&dev_attr_version.attr,
	&dev_attr_totalcx.attr,
	&dev_attr_rawdata.attr,
	&dev_attr_jitter.attr,
	&dev_attr_numberofchannel.attr,
	&dev_attr_JitterTest.attr,
	NULL,
};

static struct attribute_group cmd_attr_group = {
	.attrs = cmd_attributes,
};

int fsr_vendor_functions_init(struct fsr_sidekey_info *info)
{
	int retval;

	retval = sysfs_create_group(&info->sec.fac_dev->kobj,
			&cmd_attr_group);
	if (retval < 0) {
		input_err(true, &info->client->dev,
				"%s: Failed to create sysfs attributes\n", __func__);
		goto exit;
	}

	return 0;

exit:
	return retval;
}

void fsr_vendor_functions_remove(struct fsr_sidekey_info *info)
{
	input_err(true, &info->client->dev, "%s\n", __func__);

	sysfs_remove_group(&info->sec.fac_dev->kobj,
			&cmd_attr_group);
}

