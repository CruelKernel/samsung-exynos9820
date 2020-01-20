/*
 *  wacom_i2c.c - Wacom Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#include <linux/usb/manager/usb_typec_manager_notifier.h>

#include "wacom.h"

#define COM_QUERY_RETRY		3
#define WACOM_I2C_RETRY		3

#define WACOM_FW_PATH_SDCARD	"/sdcard/FIRMWARE/WACOM/wacom_firm.fw"

#define WACOM_INVALID_IRQ_COUNT	2


static struct wacom_features wacom_feature_EMR = {
	.comstat = COM_QUERY,
	.fw_version = 0x0,
	.update_status = FW_UPDATE_PASS,
};

struct wacom_i2c *wacom_get_drv_data(void *data)
{
	static void *drv_data;

	if (unlikely(data))
		drv_data = data;

	return (struct wacom_i2c *)drv_data;
}

int coordc;

int set_spen_mode(int mode) {
	return 0;
}

void forced_release(struct wacom_i2c *wac_i2c)
{
	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);

	if (wac_i2c->dex_mode & DEX_MODE_MOUSE) {
		input_report_key(wac_i2c->input_dev, BTN_LEFT, 0);
		input_report_rel(wac_i2c->input_dev, REL_X, 0);
		input_report_rel(wac_i2c->input_dev, REL_Y, 0);
		input_report_key(wac_i2c->input_dev, BTN_RIGHT, 0);
	} else {
		input_report_abs(wac_i2c->input_dev, ABS_X, 0);
		input_report_abs(wac_i2c->input_dev, ABS_Y, 0);
		input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
		input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
	}

	input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
	input_sync(wac_i2c->input_dev);

	input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOOL_RUBBER, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
	input_sync(wac_i2c->input_dev);

	wac_i2c->pen_prox = 0;
	wac_i2c->pen_pressed = 0;
	wac_i2c->side_pressed = 0;
	wac_i2c->mcount = 0;
	wac_i2c->virtual_tracking = EPEN_POS_NONE;
	/*wac_i2c->pen_pdct = PDCT_NOSIGNAL; */
}

int wacom_i2c_send(struct wacom_i2c *wac_i2c,
		   const char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ? wac_i2c->client_boot : wac_i2c->client;
	int retry = WACOM_I2C_RETRY;
	int ret;

	do {
		if (!wac_i2c->power_enable) {
			input_err(true, &wac_i2c->client->dev, "%s: Power status off\n",
					__func__);
			return -EIO;
		}

		ret = i2c_master_send(client, buf, count);
		if (ret == count)
			break;

		if (retry < WACOM_I2C_RETRY) {
			input_err(true, &wac_i2c->client->dev, "%s: I2C retry(%d)\n",
					__func__, WACOM_I2C_RETRY - retry);
			wac_i2c->i2c_fail_count++;
		}
	} while (--retry);

	return ret;
}

int wacom_i2c_recv(struct wacom_i2c *wac_i2c, char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ? wac_i2c->client_boot : wac_i2c->client;
	int retry = WACOM_I2C_RETRY;
	int ret;

	do {
		if (!wac_i2c->power_enable) {
			input_err(true, &wac_i2c->client->dev, "%s: Power status off\n",
					__func__);
			return -EIO;
		}

		ret = i2c_master_recv(client, buf, count);
		if (ret == count)
			break;

		if (retry < WACOM_I2C_RETRY) {
			input_err(true, &wac_i2c->client->dev, "%s: I2C retry(%d)\n",
					__func__, WACOM_I2C_RETRY - retry);
			wac_i2c->i2c_fail_count++;
		}
	} while (--retry);

	return ret;
}

int wacom_checksum(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int ret = 0, retry = 10;
	int i = 0;
	u8 buf[5] = { 0, };

	buf[0] = COM_CHECKSUM;

	while (retry--) {
		ret = wacom_i2c_send(wac_i2c, &buf[0], 1,
				     WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev, "i2c fail, retry, %d\n",
				  __LINE__);
			continue;
		}

		msleep(200);

		ret = wacom_i2c_recv(wac_i2c, buf, 5, WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev, "i2c fail, retry, %d\n",
				  __LINE__);
			continue;
		}

		if (buf[0] == 0x1f)
			break;

		input_info(true, &client->dev, "buf[0]: 0x%x, checksum retry\n",
			   buf[0]);
	}

	if (ret >= 0) {
		input_info(true, &client->dev,
			   "received checksum %x, %x, %x, %x, %x\n", buf[0],
			   buf[1], buf[2], buf[3], buf[4]);
	}

	for (i = 0; i < 5; ++i) {
		if (buf[i] != wac_i2c->fw_chksum[i]) {
			input_info(true, &client->dev,
				   "checksum fail %dth %x %x\n", i, buf[i],
				   wac_i2c->fw_chksum[i]);
			break;
		}
	}

	wac_i2c->checksum_result = (i == 5);

	return wac_i2c->checksum_result;
}

int wacom_i2c_query(struct wacom_i2c *wac_i2c)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	struct wacom_features *wac_feature = wac_i2c->wac_feature;
	struct i2c_client *client = wac_i2c->client;
	u8 data[COM_QUERY_BUFFER] = { 0, };
	u8 *query = data + COM_QUERY_POS;
	int read_size = COM_QUERY_BUFFER;
	int ret;
	int i;
	int max_x, max_y, pressure, height, x_tilt, y_tilt;

	for (i = 0; i < COM_QUERY_RETRY; i++) {
		ret = wacom_i2c_recv(wac_i2c, data, read_size,
				     WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev, "%s: failed to recv\n",
				  __func__);
			continue;
		}

		input_info(true, &client->dev,
			   "%s: %dth ret of wacom query=%d\n", __func__, i,
			   ret);

		if (read_size != ret) {
			input_err(true, &client->dev,
				  "%s: read size error %d of %d\n", __func__,
				  ret, read_size);
			continue;
		}

		if (query[EPEN_REG_HEADER] == 0x0f) {
			wac_feature->fw_version =
			    ((u16)query[EPEN_REG_FWVER1] << 8) +
			    query[EPEN_REG_FWVER2];
			break;
		}
	}

	input_info(true, &client->dev,
		   "%X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
		   query[0], query[1], query[2], query[3], query[4],
		   query[5], query[6], query[7],  query[8], query[9], query[10],
		   query[11], query[12], query[13], query[14]);

	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read query\n",
			  __func__);
		wac_feature->fw_version = 0;
		wac_i2c->query_status = false;

		return ret;
	}

	wac_i2c->query_status = true;

	max_x = ((u16)query[EPEN_REG_X1] << 8) + query[EPEN_REG_X2];
	max_y = ((u16)query[EPEN_REG_Y1] << 8) + query[EPEN_REG_Y2];
	pressure =
	    ((u16)query[EPEN_REG_PRESSURE1] << 8) + query[EPEN_REG_PRESSURE2];
	x_tilt = query[EPEN_REG_TILT_X];
	y_tilt = query[EPEN_REG_TILT_Y];
	height = query[EPEN_REG_HEIGHT];

	if (!pdata->use_dt_coord) {
		pdata->max_x = max_x;
		pdata->max_y = max_y;
	}
	pdata->max_pressure = pressure;
	pdata->max_x_tilt = x_tilt;
	pdata->max_y_tilt = y_tilt;
	pdata->max_height = height;

	input_info(true, &client->dev, "use_dt_coord=%d, max_x=%d max_y=%d, max_pressure=%d\n",
		   pdata->use_dt_coord, pdata->max_x, pdata->max_y, pdata->max_pressure);
	input_info(true, &client->dev, "fw_version=0x%X\n",
		   wac_feature->fw_version);
	input_info(true, &client->dev, "mpu %#x, bl %#x, tx %d, ty %d, h %d\n",
		   query[EPEN_REG_MPUVER], query[EPEN_REG_BLVER], x_tilt,
		   y_tilt, height);

	return 0;
}

int wacom_i2c_set_sense_mode(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int retval;
	char data[4] = { 0, 0, 0, 0 };

	input_info(true, &wac_i2c->client->dev, "%s cmd mod(%d)\n", __func__,
		   wac_i2c->wcharging_mode);

	if (wac_i2c->wcharging_mode == 1)
		data[0] = COM_LOW_SENSE_MODE;
	else if (wac_i2c->wcharging_mode == 3)
		data[0] = COM_LOW_SENSE_MODE2;
	else {
		/* it must be 0 */
		data[0] = COM_NORMAL_SENSE_MODE;
	}

	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send wacom i2c mode, %d\n", __func__,
			  retval);
		return retval;
	}

	msleep(60);

	data[0] = COM_SAMPLERATE_STOP;
	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send1, %d\n", __func__,
			  retval);
		return retval;
	}

	wac_i2c->samplerate_state = false;
	msleep(60);

#if 0				/* temp block not to receive gabage irq by cmd */
	data[1] = COM_REQUEST_SENSE_MODE;
	retval = wacom_i2c_send(wac_i2c, &data[1], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send2, %d\n", __func__,
			  retval);
		return retval;
	}

	msleep(60);

	retval = wacom_i2c_recv(wac_i2c, &data[2], 2, WACOM_I2C_MODE_NORMAL);
	if (retval != 2) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c recv, %d\n", __func__,
			  retval);
		return retval;
	}

	input_info(true, &client->dev, "%s: mode:%X, %X\n", __func__, data[2],
		   data[3]);

	data[0] = COM_SAMPLERATE_STOP;
	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send3, %d\n", __func__,
			  retval);
		return retval;
	}

	msleep(60);
#endif
	data[0] = COM_SAMPLERATE_START;
	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send4, %d\n", __func__,
			  retval);
		return retval;
	}

	wac_i2c->samplerate_state = true;
	msleep(60);

	return data[3];
}

void wacom_select_survey_mode(struct wacom_i2c *wac_i2c, bool enable)
{
	struct i2c_client *client = wac_i2c->client;

	if (enable) {
		if (wac_i2c->epen_blocked ||
		    (wac_i2c->battery_saving_mode && !(wac_i2c->function_result & EPEN_EVENT_PEN_OUT))) {
			if (wac_i2c->pdata->use_garage) {
				input_info(true, &client->dev,
					   "%s: %s & garage on. garage only mode\n",
					   __func__,
					   wac_i2c->epen_blocked ? "epen blocked" : "ps on & pen in");

				wacom_i2c_set_survey_mode(wac_i2c,
							  EPEN_SURVEY_MODE_GARAGE_ONLY);
			} else {
				input_info(true, &client->dev,
					   "%s: %s & garage off. power off\n", __func__,
					   wac_i2c->epen_blocked ? "epen blocked" : "ps on & pen in");

				wacom_enable_irq(wac_i2c, false);
				wacom_enable_pdct_irq(wac_i2c, false);
				wacom_power(wac_i2c, false);

				wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
				wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
			}
		} else if (wac_i2c->survey_mode) {
			input_info(true, &client->dev, "%s: exit aop mode\n",
				   __func__);

			wacom_i2c_set_survey_mode(wac_i2c,
						  EPEN_SURVEY_MODE_NONE);
		} else {
			input_info(true, &client->dev, "%s: power on\n",
				   __func__);

			wacom_power(wac_i2c, true);
			msleep(100);

			wacom_enable_irq(wac_i2c, true);
			wacom_enable_pdct_irq(wac_i2c, true);
		}
	} else {
		if (wac_i2c->epen_blocked ||
		    (wac_i2c->battery_saving_mode && !(wac_i2c->function_result & EPEN_EVENT_PEN_OUT))) {
			if (wac_i2c->pdata->use_garage) {
				input_info(true, &client->dev,
					   "%s: %s & garage on. garage only mode\n",
					   __func__,
					   wac_i2c->epen_blocked ? "epen blocked" : "ps on & pen in");

				wacom_i2c_set_survey_mode(wac_i2c,
							  EPEN_SURVEY_MODE_GARAGE_ONLY);
			} else {
				input_info(true, &client->dev,
					   "%s: %s & garage off. power off\n", __func__,
					   wac_i2c->epen_blocked ? "epen blocked" : "ps on & pen in");

				wacom_enable_irq(wac_i2c, false);
				wacom_enable_pdct_irq(wac_i2c, false);
				wacom_power(wac_i2c, false);

				wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
				wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
			}
		} else if (!(wac_i2c->function_set & EPEN_SETMODE_AOP)) {
			if (wac_i2c->pdata->use_garage) {
				input_info(true, &client->dev,
					   "%s: aop off & garage on. garage only mode\n",
					   __func__);

				wacom_i2c_set_survey_mode(wac_i2c,
						EPEN_SURVEY_MODE_GARAGE_ONLY);
			} else {
				input_info(true, &client->dev,
					   "%s: aop off & garage off. power off\n",
					   __func__);

				wacom_enable_irq(wac_i2c, false);
				wacom_enable_pdct_irq(wac_i2c, false);
				wacom_power(wac_i2c, false);

				wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
				wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
			}
		} else {
			/* aop on => (aod : screen off memo = 1:1 or 1:0 or 0:1)
			 * double tab & hover + button event will be occurred,
			 * but some of them will be skipped at reporting by mode
			 */
			input_info(true, &client->dev,
				   "%s: aop on. aop mode\n", __func__);

			if (!wac_i2c->power_enable) {
				input_info(true, &client->dev, "%s: power on\n",
					   __func__);

				wacom_power(wac_i2c, true);
				msleep(100);

				wacom_enable_irq(wac_i2c, true);
				wacom_enable_pdct_irq(wac_i2c, true);
			}

			wacom_i2c_set_survey_mode(wac_i2c,
						  EPEN_SURVEY_MODE_GARAGE_AOP);
		}
	}

	if (wac_i2c->power_enable) {
		input_info(true, &client->dev, "%s: screen %s, survey mode:%d, result:%d\n",
			   __func__, enable ? "on" : "off",
			   wac_i2c->survey_mode,
			   wac_i2c->function_result & EPEN_EVENT_SURVEY);

		if ((wac_i2c->function_result & EPEN_EVENT_SURVEY) != wac_i2c->survey_mode) {
			input_err(true, &client->dev, "%s: survey mode cmd failed\n",
				  __func__);

			wacom_i2c_set_survey_mode(wac_i2c,
						  wac_i2c->survey_mode);
		}
	}
}

void wacom_i2c_set_survey_mode(struct wacom_i2c *wac_i2c, int mode)
{
	struct i2c_client *client = wac_i2c->client;
	int retval;
	char data[4] = { 0, 0, 0, 0 };

	switch (mode) {
	case EPEN_SURVEY_MODE_NONE:
		data[0] = COM_SURVEYEXIT;
		break;
	case EPEN_SURVEY_MODE_GARAGE_ONLY:
		if (!wac_i2c->pdata->use_garage) {
			input_err(true, &client->dev,
				  "%s: garage mode is not supported\n", __func__);
			return;
		}

		data[0] = COM_SURVEY_TARGET_GARAGEONLY;
		break;
	case EPEN_SURVEY_MODE_GARAGE_AOP:
		if ((wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) == EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
			data[0] = COM_SURVEYSYNCSCAN;
		else
			data[0] = COM_SURVEYSCAN;
		break;
	default:
		input_err(true, &client->dev,
			  "%s: wrong param %d\n", __func__, mode);
		return;
	}

	wac_i2c->survey_mode = mode;
	input_info(true, &client->dev, "%s: ps %s & mode : %d cmd(0x%2X)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off", mode, data[0]);

	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send survey, %d\n",
			  __func__, retval);
		wac_i2c->reset_flag = true;

		return;
	}

	if (mode)
		msleep(300);

	wac_i2c->reset_flag = false;
	wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
	wac_i2c->function_result |= mode;

	return;
}

static int keycode[] = {
	KEY_RECENT, KEY_BACK,
};

void forced_release_key(struct wacom_i2c *wac_i2c)
{
	input_info(true, &wac_i2c->client->dev, "%s : [%x/%x]!\n",
		   __func__, wac_i2c->soft_key_pressed[0],
		   wac_i2c->soft_key_pressed[1]);

	input_report_key(wac_i2c->input_dev, KEY_RECENT, 0);
	input_report_key(wac_i2c->input_dev, KEY_BACK, 0);
	input_sync(wac_i2c->input_dev);
}

void wacom_i2c_softkey(struct wacom_i2c *wac_i2c, s16 key, s16 pressed)
{
	struct i2c_client *client = wac_i2c->client;

#ifdef WACOM_USE_SOFTKEY_BLOCK
	if (wac_i2c->block_softkey && pressed) {
		cancel_delayed_work_sync(&wac_i2c->softkey_block_work);
		input_info(true, &client->dev, "block p\n");
		return;
	} else if (wac_i2c->block_softkey && !pressed) {
		input_info(true, &client->dev, "block r\n");
		wac_i2c->block_softkey = false;
		return;
	}
#endif
	input_report_key(wac_i2c->input_dev, keycode[key], pressed);
	input_sync(wac_i2c->input_dev);

	wac_i2c->soft_key_pressed[key] = pressed;
	if (pressed)
		cancel_delayed_work(&wac_i2c->gxscan_work);

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	input_info(true, &client->dev, "%d %s\n",
		   keycode[key], !(!pressed) ? "PRESS" : "RELEASE");
#else
	input_info(true, &client->dev, "softkey %s\n",
		   !(!pressed) ? "PRESS" : "RELEASE");
#endif
}

void forced_release_fullscan(struct wacom_i2c *wac_i2c)
{
	input_info(true, &wac_i2c->client->dev, "%s: full scan OUT\n", __func__);
	cancel_delayed_work(&wac_i2c->gxscan_work);
	cancel_delayed_work(&wac_i2c->fullscan_work);
	wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
	wac_i2c->fullscan_mode = false;
}

int wacom_get_scan_mode(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int retval;
	char data[COM_COORD_NUM + 1] = { 0, };
	int i, retry = 0, temp = 0, ret = 0;

	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);

	data[0] = COM_SAMPLERATE_STOP;
	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send (stop), %d\n",
			  __func__, retval);
		return -EIO;
	}

	wac_i2c->samplerate_state = false;
	msleep(30);

	retry = 5;
	while (retry--) {
		if (wacom_get_irq_state(wac_i2c) > 0) {
			u8 data[COM_COORD_NUM + 1] = { 0, };

			retval = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM + 1,
						WACOM_I2C_MODE_NORMAL);
			if (retval < 0) {
				input_err(true, &client->dev,
					  "%s: failed to receive\n", __func__);
			} else {
				input_err(true, &client->dev,
					  "%s: ignore gabage data\n", __func__);
			}
		} else {
			break;
		}

		usleep_range(4500, 5500);
	}

	retry = 3;
	while (retry--) {
		data[0] = COM_REQUESTSCANMODE;
		retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
		if (retval != 1) {
			input_err(true, &client->dev,
				  "%s: failed to read wacom i2c send (request data), %d\n",
				  __func__, retval);
			usleep_range(4500, 5500);
			continue;
		}
		msleep(55);

		retval = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM + 1,
					WACOM_I2C_MODE_NORMAL);
		if (retval < 0) {
			input_err(true, &client->dev,
				  "%s: failed to read wacom i2c send survey, %d\n",
				  __func__, retval);
			usleep_range(4500, 5500);
			continue;
		}
		temp = 0;
		for (i = 0; i < COM_COORD_NUM; i++)
			temp += data[i];

		input_info(true, &client->dev,
			   "%x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12], data[13]);

		if (temp == 0) {	/* unlock */
			ret = EPEN_GLOBAL_SCAN_MODE;
			break;
		} else if (data[12] == 0x01) {	/* send noise mode to tsp */
			/* skip high noise mode after crown, ret = EPEN_HIGH_NOISE_MODE; */
			ret = -EPERM;

			if (!wac_i2c->screen_on && !(temp - data[12]))
				ret = -EPERM;

			break;
		}
		msleep(10);
	}

	input_info(true, &client->dev,
		   "data[0] = %x, data[12] = %x, retry(%d) ret(%d)\n", data[0],
		   data[12], 3 - retry, ret);

	data[0] = COM_SAMPLERATE_133;
	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev,
			  "%s: failed to read wacom i2c send (start), %d\n",
			  __func__, retval);
		return -EIO;
	}

	wac_i2c->samplerate_state = true;

	return ret;
}

bool wacom_get_status_data(struct wacom_i2c *wac_i2c, char *data)
{
	struct i2c_client *client = wac_i2c->client;
	int retval;
	int retry = 5;
	bool tsp = false, rdy = false;

	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);

	while (retry--) {
		retval = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM + 1,
					WACOM_I2C_MODE_NORMAL);

		if (retval >= 0)
			break;

		input_err(true, &client->dev, "%s: failed to read i2c retry(%d)\n",
			  __func__, retry);
	}

	if (retval < 0) {
		input_err(true, &client->dev, "%s: failed to read status data, %d\n",
			  __func__, retval);
		return false;
	}

	if (coordc < 100)
		coordc++;

	rdy = data[0] & 0x80;
	tsp = data[12] & 0x01;

	if (wac_i2c->pdata->table_swap && data[0] == TABLE_SWAP_DATA &&
			data[2] == 0 && data[3] == 0 && data[4] == 0) {
		wac_i2c->dp_connect_state = data[1];
		input_info(true, &client->dev,
			   "%s: usb typec DP %sconnected\n",
			   __func__, wac_i2c->dp_connect_state ? "" : "dis");
		return false;
	}

	/* Noise status : 11 11 = High noise, 22 22 = Low noise */
	if (data[0] == 0x0F) {
		input_info(true, &client->dev,
			   "%x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12], data[13]);

		if ((data[10] == WACOM_NOISE_HIGH) && (data[11] == WACOM_NOISE_HIGH)) {
			input_err(true, &wac_i2c->client->dev, "11 11 high-noise mode\n");
			wac_i2c->wacom_noise_state = WACOM_NOISE_HIGH;
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_HIGH_NOISE_MODE);
		} else if ((data[10] == WACOM_NOISE_LOW) && (data[11] == WACOM_NOISE_LOW)) {
			input_err(true, &wac_i2c->client->dev, "22 22 low-noise mode\n");
			wac_i2c->wacom_noise_state = WACOM_NOISE_LOW;
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
		}

		return false;
	}

	/* AOP status   : BB BB = Button+Hover, DD DD = Double Tab gesture */
	if ((data[10] == AOP_BUTTON_HOVER) || (data[10] == AOP_DOUBLE_TAB)) {
		if (data[10] != data[11]) {
			input_err(true, &client->dev,
				  "%s: invalid status data\n", __func__);
			return false;
		}

		input_info(true, &client->dev, "%s: data[10] = %x, data[11] = %x\n",
			   __func__, data[10], data[11]);
		return true;
	}

	if (tsp && !wac_i2c->fullscan_mode) {
		input_info(true, &client->dev,
			   "%x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x coordc(%d)\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12], data[13], coordc);

		if (data[10] == HSYNC_COUNTER_UMAGIC && data[11] == HSYNC_COUNTER_LMAGIC) {
			input_info(true, &client->dev, "x scan IN, rdy(%d) tsp(%d)\n",
				   rdy, tsp);
		}

		wac_i2c->fullscan_mode = true;

		if (wac_i2c->wacom_noise_state != WACOM_NOISE_HIGH)
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_LOCAL_SCAN_MODE);
		else
			input_info(true, &client->dev, "high noise mode, skip tsp (F3 1)\n");

		coordc = 0;
		cancel_delayed_work(&wac_i2c->gxscan_work);
		schedule_delayed_work(&wac_i2c->gxscan_work,
				      msecs_to_jiffies(1500));
	}

	if (!tsp) {
		input_info(true, &client->dev, "full scan out (%02X %02X)\n", data[10], data[11]);
		if (wac_i2c->wacom_noise_state != WACOM_NOISE_HIGH)
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
		else
			input_info(true, &client->dev, "high noise mode, skip tsp (F3 0)\n");

		cancel_delayed_work(&wac_i2c->gxscan_work);
		wac_i2c->fullscan_mode = false;
	}

	return false;
}

#ifdef LCD_FREQ_SYNC
void wacom_i2c_lcd_freq_check(struct wacom_i2c *wac_i2c, u8 *data)
{
	u32 lcd_freq = 0;

	if (wac_i2c->lcd_freq_wait == false) {
		lcd_freq = ((u16) data[10] << 8) + (u16) data[11];
		wac_i2c->lcd_freq = 2000000000 / (lcd_freq + 1);
		wac_i2c->lcd_freq_wait = true;
		schedule_work(&wac_i2c->lcd_freq_work);
	}
}
#endif

void wacom_gxscan_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, gxscan_work.work);
	int ret;

	input_info(true, &wac_i2c->client->dev,
		   "gxscan_work irq-cnt(%d)\n", coordc);
	if (coordc <= WACOM_INVALID_IRQ_COUNT) {
		wacom_enable_irq(wac_i2c, false);
		ret = wacom_get_scan_mode(wac_i2c);
		if (ret < 0) {
			input_info(true, &wac_i2c->client->dev, "work - stay scan mode\n");
		} else if (ret == EPEN_GLOBAL_SCAN_MODE) {
			if (wac_i2c->wacom_noise_state != WACOM_NOISE_HIGH) {
				input_info(true, &wac_i2c->client->dev, "work - full scan OUT\n");
				wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
				wac_i2c->fullscan_mode = false;
			} else {
				input_info(true, &wac_i2c->client->dev, "high noise mode, skip tsp (F3 0)\n");
			}
		} else if (ret == EPEN_HIGH_NOISE_MODE) {
			if (wac_i2c->wacom_noise_state != WACOM_NOISE_HIGH) {
				input_info(true, &wac_i2c->client->dev, "work - wacom noise mode\n");
				wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_HIGH_NOISE_MODE);
			} else {
				input_info(true, &wac_i2c->client->dev, "high noise mode, skip tsp (F3 2)\n");
			}
		} else {
			input_err(true, &wac_i2c->client->dev, "unexpected status\n");
		}
		wacom_enable_irq(wac_i2c, true);
	}
}

void wacom_fullscan_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
		container_of(work, struct wacom_i2c, fullscan_work.work);
	int ret;
	char data[4] = { 0, 0, 0, 0 };

	input_info(true, &wac_i2c->client->dev,
		   "fullscan_work  fullscan_mode(%d)\n", wac_i2c->fullscan_mode);
	if (wac_i2c->fullscan_mode) {
		input_info(true, &wac_i2c->client->dev, "reset dsp\n");

		data[0] = COM_RESET_DSP;
		ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &wac_i2c->client->dev,
				  "%s: failed to send wacom i2c mode, %d\n", __func__,
				  ret);
		}

		wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
		wac_i2c->fullscan_mode = false;

		/* 	we expect dsp_reset cmd will recover scan mdoe and send fullscan out cmd 
		wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
		wac_i2c->fullscan_mode = false;
		wacom_i2c_set_survey_mode(wac_i2c,
			  EPEN_SURVEY_MODE_GARAGE_ONLY);
		msleep(30);
		wacom_i2c_set_survey_mode(wac_i2c,
					  EPEN_SURVEY_MODE_NONE);
		*/
	}
}

int wacom_i2c_coord(struct wacom_i2c *wac_i2c)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	struct i2c_client *client = wac_i2c->client;
	u8 data[COM_COORD_NUM + 1] = { 0, };
	bool prox = false;
	bool rdy = false;
	int ret = 0, tsp = 0;
	int stylus;
	static int old_x = 0, old_y = 0;
	s16 x, y, pressure;
	s16 tmp;
	u8 gain = 0;
	s16 softkey, pressed, keycode;
	s8 tilt_x = 0;
	s8 tilt_y = 0;
	s8 retry = 3;

	while (retry--) {
		ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM + 1,
				     WACOM_I2C_MODE_NORMAL);
		if (ret >= 0)
			break;

		input_err(true, &client->dev,
			  "%s failed to read i2c.retry %d.L%d\n", __func__,
			  retry, __LINE__);
	}

	if (ret < 0) {
		input_err(true, &client->dev, "i2c err, exit %s\n", __func__);
		forced_release(wac_i2c);
		wac_i2c->reset_flag = true;
		if (work_busy(&wac_i2c->resume_work.work) == 0) {
			input_err(true, &client->dev,
				"%s schedule resume work\n", __func__);
			cancel_delayed_work(&wac_i2c->resume_work);
			schedule_delayed_work(&wac_i2c->resume_work,
				msecs_to_jiffies(EPEN_RESUME_DELAY));
		} else {
			input_err(true, &client->dev,
				"%s resume work is busy\n", __func__);
		}
		return 0;
	}
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
#if 0
	input_info(true, &client->dev,
		   "%x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x\n",
		   data[0], data[1], data[2], data[3], data[4], data[5],
		   data[6], data[7], data[8], data[9], data[10], data[11],
		   data[12], data[13]);
#endif
#endif

	rdy = data[0] & 0x80;

#ifdef LCD_FREQ_SYNC
	if (!rdy && !data[1] && !data[2] && !data[3] && !data[4]) {
		if (likely(wac_i2c->use_lcd_freq_sync)) {
			if (unlikely(!wac_i2c->pen_prox))
				wacom_i2c_lcd_freq_check(wac_i2c, data);
		}
	}
#endif
	if (coordc < 100)
		coordc++;

	tsp = data[12] & 0x01;

	if (wac_i2c->pdata->table_swap && data[0] == TABLE_SWAP_DATA &&
			data[2] == 0 && data[3] == 0 && data[4] == 0) {
		wac_i2c->dp_connect_state = data[1];
		input_info(true, &client->dev,
			   "%s: usb typec DP %sconnected\n",
			   __func__, wac_i2c->dp_connect_state ? "" : "dis");
		return 0;
	}

	if (data[0] == 0x0F) {
		input_info(true, &client->dev,
			   "%x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12], data[13]);
		if ((data[10] == WACOM_NOISE_HIGH) && (data[11] == WACOM_NOISE_HIGH)) {
			input_err(true, &wac_i2c->client->dev, "11 11 high-noise mode\n");
			wac_i2c->wacom_noise_state = WACOM_NOISE_HIGH;
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_HIGH_NOISE_MODE);
		} else if ((data[10] == WACOM_NOISE_LOW) && (data[11] == WACOM_NOISE_LOW)) {
			input_err(true, &wac_i2c->client->dev, "22 22 low-noise mode\n");
			wac_i2c->wacom_noise_state = WACOM_NOISE_LOW;
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
		}

		return 0;
	}

	if (tsp && !wac_i2c->fullscan_mode) {
		input_info(true, &client->dev,
			   "%x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x coordc(%d)\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12], data[13], coordc);

		wac_i2c->fullscan_mode = true;
		coordc = 0;

		if (wac_i2c->wacom_noise_state != WACOM_NOISE_HIGH)
			wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_LOCAL_SCAN_MODE);
		else
			input_info(true, &client->dev, "high noise mode, skip tsp (F3 1)\n");

		if (data[10] == HSYNC_COUNTER_UMAGIC && data[11] == HSYNC_COUNTER_LMAGIC) {
			input_info(true, &client->dev, "x scan IN, rdy(%d) tsp(%d)\n",
				   rdy, tsp);
		} else if (data[10] == FULL_SCAN_UMAGIC && data[11] == FULL_SCAN_LMAGIC) {
			input_info(true, &client->dev, "fullscan IN rdy(%d) tsp(%d) without x scan\n",
				   rdy, tsp);
			cancel_delayed_work(&wac_i2c->fullscan_work);
			schedule_delayed_work(&wac_i2c->fullscan_work,
						  msecs_to_jiffies(2000));
			return 0;
		}

		cancel_delayed_work(&wac_i2c->gxscan_work);
		schedule_delayed_work(&wac_i2c->gxscan_work,
				      msecs_to_jiffies(1500));
		return 0;
	} else if  (tsp && wac_i2c->fullscan_mode) {  
		if (data[10] == FULL_SCAN_UMAGIC && data[11] == FULL_SCAN_LMAGIC) {
			input_info(true, &client->dev, "fullscan IN rdy(%d) tsp(%d)\n",
				   rdy, tsp);
			cancel_delayed_work(&wac_i2c->fullscan_work);
			schedule_delayed_work(&wac_i2c->fullscan_work,
						  msecs_to_jiffies(2000));
		}
	}

	if (rdy) {
		
		if (!wac_i2c->localscan_mode)
			cancel_delayed_work(&wac_i2c->fullscan_work);
		wac_i2c->localscan_mode = true;
		/* checking softkey */
		softkey = !(!(data[12] & 0x80));
		if (unlikely(softkey)) {
			if (unlikely(wac_i2c->pen_prox))
				forced_release(wac_i2c);

			pressed = !(!(data[12] & 0x40));
			keycode = (data[12] & 0x30) >> 4;

			if (!wac_i2c->pdata->use_virtual_softkey)
				wacom_i2c_softkey(wac_i2c, keycode, pressed);
			else
				input_info(true, &client->dev, "wacom use virtual softkey\n");

			return 0;
		}

		prox = !(!(data[0] & 0x10));
		stylus = !(!(data[0] & 0x20));
		x = ((u16) data[1] << 8) + (u16) data[2];
		y = ((u16) data[3] << 8) + (u16) data[4];
		pressure = ((u16) data[5] << 8) + (u16) data[6];
		gain = data[7];
		tilt_x = (s8) data[8];
		tilt_y = (s8) data[9];

		/* origin */
		x = x - pdata->origin[0];
		y = y - pdata->origin[1];

		/* change axis from wacom to lcd */
		if (pdata->x_invert) {
			x = pdata->max_x - x;
			tilt_x = -tilt_x;
		}

		if (pdata->y_invert) {
			y = pdata->max_y - y;
			tilt_y = -tilt_y;
		}

		if (pdata->xy_switch) {
			tmp = x;
			x = y;
			y = tmp;

			tmp = tilt_x;
			tilt_x = tilt_y;
			tilt_y = tmp;
		}

		if (wac_i2c->keyboard_cover_mode == true) {
			if (y > KEYBOARD_COVER_BOUNDARY)
				wac_i2c->keyboard_area = true;
			else
				wac_i2c->keyboard_area = false;

			if (wac_i2c->keyboard_area == true &&
				wac_i2c->virtual_tracking == EPEN_POS_NONE) {
				/* input_info(true, &client->dev,
				 *"skip  - approching on keyboard area  prox(%d)\n", prox);
				 */
				return 0;
			} else if (wac_i2c->keyboard_area == true &&
				wac_i2c->virtual_tracking == EPEN_POS_COVER) {
				/*input_info(true, &client->dev,
				 *"skip  keyboard area  prox(%d)\n", prox);
				 */
				return 0;
			}

			/*input_info(true, &client->dev,
			 *"go through, prox(%d) y(%d) area(%d)\n", prox,y,wac_i2c->keyboard_area);
			 */
		}

		/* validation check */
		if (unlikely
		    (x < 0 || y < 0 || x > pdata->max_y || y > pdata->max_x)) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev, "raw data x=%d, y=%d\n",
				   x, y);
#endif
			return 0;
		}

		if (wac_i2c->dex_mode & DEX_MODE_EDGE_CROP) {
			int max_x = wac_i2c->pdata->max_x;
			if (pdata->xy_switch)
				max_x = wac_i2c->pdata->max_y;

			if (wac_i2c->dex_mode & DEX_MODE_MOUSE) {
				if (x < max_x / 10)
					x = max_x / 10;
				else if (x > max_x * 9 / 10)
					x = max_x * 9 / 10;
			} else {
				if (x < max_x / 10)
					x = 0;
				else if (x > max_x * 9 / 10)
					x = max_x;
				else
					x = x * 5 / 4 - max_x / 8;
			}
		}

		if (wac_i2c->dex_mode & DEX_MODE_IRIS) {
			int max_y = wac_i2c->pdata->max_y;
			if (pdata->xy_switch)
				max_y = wac_i2c->pdata->max_x;

			if (y < max_y / 2) {
				x = old_x;
				y = old_y;
			}
		}

		if (!wac_i2c->pen_prox) {
			if (!wac_i2c->pdata->use_garage) {
				/* check pdct */
				if (unlikely(wac_i2c->pen_pdct == PDCT_NOSIGNAL)) {
					input_info(true, &client->dev,
						   "pdct is not active\n");
					return 0;
				}
			}

			wac_i2c->pen_prox = 1;
			wac_i2c->virtual_tracking = EPEN_POS_VIEW;

			if (wac_i2c->dex_mode & DEX_MODE_MOUSE) {
				input_report_rel(wac_i2c->input_dev, REL_X, 0);
				input_report_rel(wac_i2c->input_dev, REL_Y, 0);
				old_x = x;
				old_y = y;
				input_sync(wac_i2c->input_dev);
			}

			if (data[0] & 0x40)
				wac_i2c->tool = BTN_TOOL_RUBBER;
			else
				wac_i2c->tool = BTN_TOOL_PEN;
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev, "hover in(%s)\n",
				   wac_i2c->tool ==
				   BTN_TOOL_PEN ? "pen" : "rubber");
#else
			input_info(true, &client->dev, "hover in\n");
#endif
			return 0;
		}

		if (wac_i2c->keyboard_cover_mode == true &&
			wac_i2c->keyboard_area == true &&
			wac_i2c->virtual_tracking == EPEN_POS_VIEW) {
			input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
			input_sync(wac_i2c->input_dev);

			input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
			input_report_key(wac_i2c->input_dev,
					 BTN_TOOL_RUBBER, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
			input_sync(wac_i2c->input_dev);

#ifdef WACOM_USE_SOFTKEY_BLOCK
			if (wac_i2c->pen_pressed) {
				cancel_delayed_work_sync
					(&wac_i2c->softkey_block_work);
				wac_i2c->block_softkey = true;
				schedule_delayed_work
					(&wac_i2c->softkey_block_work,
					 SOFTKEY_BLOCK_DURATION);
			}
#endif
			input_info(true, &client->dev,
				"virtual hover out by keyboard cover\n");
			wac_i2c->pen_prox = 0;
			wac_i2c->pen_pressed = 0;
			wac_i2c->side_pressed = 0;
			wac_i2c->virtual_tracking = EPEN_POS_COVER;
			return 0;
		}

		/* report info */
		if (wac_i2c->dex_mode & DEX_MODE_MOUSE) {
			input_report_key(wac_i2c->input_dev, BTN_LEFT, prox);
 			input_report_rel(wac_i2c->input_dev, REL_X,
					x / wac_i2c->pdata->dex_rate -
					(old_x / wac_i2c->pdata->dex_rate));
			input_report_rel(wac_i2c->input_dev, REL_Y,
					y / wac_i2c->pdata->dex_rate -
					(old_y / wac_i2c->pdata->dex_rate));
			input_report_key(wac_i2c->input_dev, BTN_RIGHT, stylus);

		} else {
			input_report_abs(wac_i2c->input_dev, ABS_X, x);
			input_report_abs(wac_i2c->input_dev, ABS_Y, y);
			input_report_key(wac_i2c->input_dev, BTN_STYLUS, stylus);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, prox);
		}
		input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, pressure);
		input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, gain);
		input_report_abs(wac_i2c->input_dev, ABS_TILT_X, tilt_x);
		input_report_abs(wac_i2c->input_dev, ABS_TILT_Y, tilt_y);
		input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
		input_sync(wac_i2c->input_dev);
		old_x = x;
		old_y = y;
		wac_i2c->mcount++;

		/* log */
		if (prox && !wac_i2c->pen_pressed) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev,
				   "[P] x:%d, y:%d, mc:%d, pre:%d, tool:%x ps:%d dex:%d dp:%d\n",
				   x, y, wac_i2c->mcount, pressure, wac_i2c->tool,
				   wac_i2c->battery_saving_mode, wac_i2c->dex_mode, wac_i2c->dp_connect_state);
#else
			input_info(true, &client->dev, "[P] mc:%d ps:%d dex:%d dp:%d\n",
				   wac_i2c->mcount, wac_i2c->battery_saving_mode,
				   wac_i2c->dex_mode, wac_i2c->dp_connect_state);
#endif
		} else if (!prox && wac_i2c->pen_pressed) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev,
				   "[R] x:%d, y:%d, mc:%d pre:%d, tool:%x dex:%d dp:%d [0x%x]\n",
				   x, y, wac_i2c->mcount, pressure, wac_i2c->tool,
				   wac_i2c->dex_mode, wac_i2c->dp_connect_state,
				   wac_i2c->wac_feature->fw_version);
#else
			input_info(true, &client->dev, "[R] mc:%d dex:%d dp:%d [0x%x]\n",
				   wac_i2c->mcount, wac_i2c->dex_mode, wac_i2c->dp_connect_state,
				   wac_i2c->wac_feature->fw_version);
#endif
		}
		wac_i2c->pen_pressed = prox;

		/* check side */
		if (stylus && !wac_i2c->side_pressed)
			input_info(true, &client->dev, "side on\n");
		else if (!stylus && wac_i2c->side_pressed)
			input_info(true, &client->dev, "side off\n");

		wac_i2c->side_pressed = stylus;
	} else {
		if (wac_i2c->pen_prox) {
			if (wac_i2c->dex_mode & DEX_MODE_MOUSE) {
				input_report_key(wac_i2c->input_dev, BTN_LEFT, 0);
				input_report_key(wac_i2c->input_dev, BTN_RIGHT, 0);
			} else {
				input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
				input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
			}
			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
			input_sync(wac_i2c->input_dev);

			input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
			input_report_key(wac_i2c->input_dev,
					 BTN_TOOL_RUBBER, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
			input_sync(wac_i2c->input_dev);

#ifdef WACOM_USE_SOFTKEY_BLOCK
			if (wac_i2c->pen_pressed) {
				cancel_delayed_work_sync
				    (&wac_i2c->softkey_block_work);
				wac_i2c->block_softkey = true;
				schedule_delayed_work
				    (&wac_i2c->softkey_block_work,
				     SOFTKEY_BLOCK_DURATION);
			}
#endif

			if (wac_i2c->pen_pressed) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
				x = ((u16) data[1] << 8) + (u16) data[2];
				y = ((u16) data[3] << 8) + (u16) data[4];
				/* origin */
				x = x - pdata->origin[0];
				y = y - pdata->origin[1];

				/* change axis from wacom to lcd */
				if (pdata->x_invert)
					x = pdata->max_x - x;
				if (pdata->y_invert)
					y = pdata->max_y - y;

				if (pdata->xy_switch) {
					tmp = x;
					x = y;
					y = tmp;
				}

				input_info(true, &client->dev,
					   "[R] x:%d, y:%d, mc:%d dex:%d dp:%d [0x%x] & hover out\n",
					   x, y, wac_i2c->mcount, wac_i2c->dex_mode,
					   wac_i2c->dp_connect_state, wac_i2c->wac_feature->fw_version);
#else
				input_info(true, &client->dev, "[R] mc:%d dex:%d dp:%d [0x%x] & hover out\n",
					   wac_i2c->mcount, wac_i2c->dex_mode,
					   wac_i2c->dp_connect_state, wac_i2c->wac_feature->fw_version);
#endif
			} else {
				input_info(true, &client->dev, "hover out mc:%d\n", wac_i2c->mcount);
			}

		} else {
			input_info(true, &client->dev,
			   "status data : %x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x coordc(%d)\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12], data[13], coordc);
		}

		if (!tsp) {
			input_info(true, &client->dev, "full scan out (%02X %02X)\n", data[10], data[11]);
			if (wac_i2c->wacom_noise_state != WACOM_NOISE_HIGH)
				wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);
			else
				input_info(true, &client->dev, "high noise mode, skip tsp (F3 0)\n");

			cancel_delayed_work(&wac_i2c->gxscan_work);
			cancel_delayed_work(&wac_i2c->fullscan_work);
			wac_i2c->fullscan_mode = false;
		}

		wac_i2c->pen_prox = 0;
		wac_i2c->pen_pressed = 0;
		wac_i2c->side_pressed = 0;
		wac_i2c->mcount = 0;
		wac_i2c->virtual_tracking = EPEN_POS_NONE;
		wac_i2c->localscan_mode = false;
	}

	return 0;
}

int wacom_power(struct wacom_i2c *wac_i2c, bool on)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	struct i2c_client *client = wac_i2c->client;
	static struct timeval off_time = { 0, 0 };
	struct timeval cur_time = { 0, 0 };
	int retval = 0;
	static struct regulator *vddo;

	input_info(true, &client->dev, "power %s\n",
		   on ? "enabled" : "disabled");

	if (!pdata) {
		input_err(true, &client->dev, "%s, pdata is null\n", __func__);
		return -EINVAL;
	}

	if (wac_i2c->power_enable == on) {
		input_info(true, &client->dev, "pwr already %s\n",
			   on ? "enabled" : "disabled");
		return 0;
	}

	if (on) {
		long sec, usec;

		do_gettimeofday(&cur_time);
		sec = cur_time.tv_sec - off_time.tv_sec;
		usec = cur_time.tv_usec - off_time.tv_usec;
		if (!sec) {
			usec = EPEN_OFF_TIME_LIMIT - usec;
			if (usec > 500) {
				usleep_range(usec, usec);
				input_info(true, &client->dev,
					   "%s, pwr on usleep %d\n", __func__,
					   (int)usec);
			}
		}
	}

	if (!vddo) {
		vddo = devm_regulator_get(&client->dev, "vddo");

		if (IS_ERR(vddo)) {
			input_err(true, &client->dev,
				  "%s: could not get vddo, rc = %ld\n",
				  __func__, PTR_ERR(vddo));
			vddo = NULL;
			return -ENODEV;
		}
		retval = regulator_set_voltage(vddo, 3300000, 3300000);
		if (retval)
			input_err(true, &client->dev,
				  "%s: unable to set vddo voltage to 3.3V\n",
				  __func__);
		input_err(true, &client->dev, "%s: 3.3V is enabled %s\n",
			  __func__,
			  regulator_is_enabled(vddo) ? "TRUE" : "FALSE");

	}

	if (on) {
		retval = regulator_enable(vddo);
		if (retval) {
			input_err(true, &client->dev,
				  "%s: Fail to enable regulator vddo[%d]\n",
				  __func__, retval);
		}
		input_err(true, &client->dev, "%s: vddo is enabled[OK]\n",
			  __func__);
	} else {
		if (regulator_is_enabled(vddo)) {
			retval = regulator_disable(vddo);
			if (retval) {
				input_err(true, &client->dev,
					  "%s: Fail to disable regulator vddo[%d]\n",
					  __func__, retval);

			}
			input_err(true, &client->dev,
				  "%s: vddo is disabled[OK]\n", __func__);
		} else {
			input_err(true, &client->dev,
				  "%s: vddo is already disabled\n", __func__);
		}
	}

	wac_i2c->power_enable = on;

	return 0;
}

void wacom_reset_hw(struct wacom_i2c *wac_i2c)
{
	wacom_power(wac_i2c, false);
	/* recommended delay in spec */
	msleep(100);
	wacom_power(wac_i2c, true);

	msleep(200);
}

void wacom_compulsory_flash_mode(struct wacom_i2c *wac_i2c, bool enable)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;

	if (pdata)
		gpio_direction_output(pdata->fwe_gpio, enable);
}

int wacom_get_irq_state(struct wacom_i2c *wac_i2c)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	int level;

	if (!pdata)
		return -EINVAL;

	level = gpio_get_value(pdata->irq_gpio);

	if (pdata->irq_type & (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_LOW))
		return !level;

	return level;
}

void wacom_enable_irq(struct wacom_i2c *wac_i2c, bool enable)
{
	static int depth;

	mutex_lock(&wac_i2c->irq_lock);
	if (enable) {
		if (depth) {
			--depth;
			enable_irq(wac_i2c->irq);
		}
	} else {
		if (!depth) {
			++depth;
			disable_irq(wac_i2c->irq);
		}
	}
	mutex_unlock(&wac_i2c->irq_lock);

#ifdef WACOM_IRQ_DEBUG
	input_info(true, &wac_i2c->client->dev,
		   "%s: Enable %d, depth %d\n", __func__, (int)enable, depth);
#endif
}

void wacom_enable_pdct_irq(struct wacom_i2c *wac_i2c, bool enable)
{
	static int depth;

	mutex_lock(&wac_i2c->irq_lock);
	if (enable) {
		if (depth) {
			--depth;
			enable_irq(wac_i2c->irq_pdct);
		}
	} else {
		if (!depth) {
			++depth;
			disable_irq(wac_i2c->irq_pdct);
		}
	}
	mutex_unlock(&wac_i2c->irq_lock);

#ifdef WACOM_IRQ_DEBUG
	input_info(true, &wac_i2c->client->dev,
		   "%s: Enable %d, depth %d\n", __func__, (int)enable, depth);
#endif
}

static void wacom_enable_irq_wake(struct wacom_i2c *wac_i2c, bool enable)
{
	static int depth;

	if (enable) {
		if (depth++ == 0) {
			enable_irq_wake(wac_i2c->irq);

			if (wac_i2c->pdata->use_garage)
				enable_irq_wake(wac_i2c->irq_pdct);
		}
	} else {
		if (depth == 0) {
			input_info(true, &wac_i2c->client->dev,
				   "Unbalanced IRQ wake disable\n");
		} else if (--depth == 0) {
			disable_irq_wake(wac_i2c->irq);

			if (wac_i2c->pdata->use_garage)
				disable_irq_wake(wac_i2c->irq_pdct);
		}
	}
}

void wacom_wakeup_sequence(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;

	mutex_lock(&wac_i2c->lock);

	input_info(true, &client->dev,
		   "%s: start, garage %s, ps %s, pen %s, epen %s, cover %s, count(%u,%u), ble_mode(0x%x)[0x%x]\n",
		   __func__, wac_i2c->pdata->use_garage ? "used" : "unused",
		   wac_i2c->battery_saving_mode ?  "on" : "off",
		   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
		   wac_i2c->epen_blocked ? "blocked" : "unblocked",
		   wac_i2c->keyboard_cover_mode ? "on" : "off",
		   wac_i2c->i2c_fail_count, wac_i2c->abnormal_reset_count,
		   wac_i2c->ble_mode, wac_i2c->wac_feature->fw_version);

#ifdef CONFIG_SEC_FACTORY
	if (wac_i2c->fac_garage_mode)
		input_info(true, &client->dev, "%s: garage mode\n", __func__);
#endif

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		input_info(true, &client->dev,
			   "fw wake lock active. pass %s\n", __func__);
		goto out_power_on;
	}

	if (wac_i2c->screen_on) {
		input_info(true, &client->dev,
			   "already enabled. pass %s\n", __func__);
		goto out_power_on;
	}

	cancel_delayed_work_sync(&wac_i2c->resume_work);
	schedule_delayed_work(&wac_i2c->resume_work,
			      msecs_to_jiffies(EPEN_RESUME_DELAY));

	if (device_may_wakeup(&client->dev))
		wacom_enable_irq_wake(wac_i2c, false);

	wac_i2c->screen_on = true;

out_power_on:
	mutex_unlock(&wac_i2c->lock);

	input_info(true, &client->dev, "%s: end\n", __func__);
}

void wacom_sleep_sequence(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int retry = 1;

	mutex_lock(&wac_i2c->lock);

	input_info(true, &client->dev,
		   "%s: start, garage %s, ps %s, pen %s, epen %s, cover %s, count(%u,%u), set(0x%x), ret(0x%x), ble_mode(0x%x)[0x%x]\n",
		   __func__, wac_i2c->pdata->use_garage ? "used" : "unused",
		   wac_i2c->battery_saving_mode ?  "on" : "off",
		   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
		   wac_i2c->epen_blocked ? "blocked" : "unblocked",
		   wac_i2c->keyboard_cover_mode ? "on" : "off",
		   wac_i2c->i2c_fail_count, wac_i2c->abnormal_reset_count,
		   wac_i2c->function_set, wac_i2c->function_result,
		   wac_i2c->ble_mode, wac_i2c->wac_feature->fw_version);

#ifdef CONFIG_SEC_FACTORY
	if (wac_i2c->fac_garage_mode)
		input_info(true, &client->dev, "%s: garage mode\n", __func__);

#endif

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		input_info(true, &client->dev,
			   "fw wake lock active. pass %s\n", __func__);

		goto out_power_off;
	}

	if (!wac_i2c->screen_on) {
		input_info(true, &client->dev,
			   "already disabled. pass %s\n", __func__);
		goto out_power_off;
	}

	cancel_delayed_work_sync(&wac_i2c->resume_work);
	cancel_delayed_work_sync(&wac_i2c->gxscan_work);
	cancel_delayed_work_sync(&wac_i2c->fullscan_work);

#ifdef LCD_FREQ_SYNC
	cancel_work_sync(&wac_i2c->lcd_freq_work);
	cancel_delayed_work_sync(&wac_i2c->lcd_freq_done_work);
	wac_i2c->lcd_freq_wait = false;
#endif
#ifdef WACOM_USE_SOFTKEY_BLOCK
	cancel_delayed_work_sync(&wac_i2c->softkey_block_work);
	wac_i2c->block_softkey = false;
#endif

reset:
	if (wac_i2c->reset_flag) {
		input_info(true, &client->dev,
			   "%s: IC reset start\n", __func__);

		wac_i2c->abnormal_reset_count++;
		wac_i2c->reset_flag = false;
		wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
		wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;

		wacom_enable_irq(wac_i2c, false);
		wacom_enable_pdct_irq(wac_i2c, false);

		wacom_reset_hw(wac_i2c);

		wac_i2c->pen_pdct =
		    gpio_get_value(wac_i2c->pdata->pdct_gpio);

		input_info(true, &client->dev,
			   "%s : IC reset end, pdct(%d)\n", __func__,
			   wac_i2c->pen_pdct);

		if (wac_i2c->pdata->use_garage) {
			if (wac_i2c->pen_pdct)
				wac_i2c->function_result &= ~EPEN_EVENT_PEN_OUT;
			else
				wac_i2c->function_result |= EPEN_EVENT_PEN_OUT;
		}

		wacom_enable_irq(wac_i2c, true);
		wacom_enable_pdct_irq(wac_i2c, true);
	}

	wacom_select_survey_mode(wac_i2c, false);

	/* release pen, if it is pressed */
	if (wac_i2c->pen_pressed || wac_i2c->side_pressed || wac_i2c->pen_prox)
		forced_release(wac_i2c);

	if (!wac_i2c->pdata->use_virtual_softkey) {
		if (wac_i2c->soft_key_pressed[0] || wac_i2c->soft_key_pressed[1])
			forced_release_key(wac_i2c);
	}

	forced_release_fullscan(wac_i2c);

	if (wac_i2c->reset_flag && retry--)
		goto reset;

	if (device_may_wakeup(&client->dev))
		wacom_enable_irq_wake(wac_i2c, true);

	wac_i2c->screen_on = false;

out_power_off:
	mutex_unlock(&wac_i2c->lock);

	input_info(true, &client->dev, "%s end\n", __func__);
}

static irqreturn_t wacom_interrupt(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	s16 x, y, pressure;
	s16 tmp;
	u8 gain = 0;
	s8 tilt_x = 0;
	s8 tilt_y = 0;
	int ret = 0;
	char data[COM_COORD_NUM + 1] = { 0, };

	if (!wac_i2c->screen_on && wac_i2c->survey_mode) {
		input_info(true, &wac_i2c->client->dev,
			   "%s: lcd off & survey mode on\n", __func__);

		/* in LPM, waiting blsp block resume */
		if (wac_i2c->pm_suspend) {
			wake_lock_timeout(&wac_i2c->wakelock,
					  msecs_to_jiffies(3 * MSEC_PER_SEC));
			/* waiting for blsp block resuming, if not occurs
			 * i2c error
			 */
			ret =
			    wait_for_completion_interruptible_timeout(
			     &wac_i2c->resume_done,
			     msecs_to_jiffies(1 * MSEC_PER_SEC));
			if (ret == 0) {
				input_err(true, &wac_i2c->client->dev,
					  "%s: LPM: pm resume is not handled [timeout]\n",
					  __func__);
				return IRQ_HANDLED;
			}
		}

		ret = wacom_get_status_data(wac_i2c, data);
		if (!ret)
			return IRQ_HANDLED;

		if (wac_i2c->function_set & EPEN_SETMODE_AOP) {
			if (data[10] == AOP_BUTTON_HOVER) {
				if (wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO) {
					input_info(true, &wac_i2c->client->dev, "Hover & Side Button detected\n");

					input_report_key(wac_i2c->input_dev,
							 KEY_WAKEUP_UNLOCK, 1);
					input_sync(wac_i2c->input_dev);

					input_report_key(wac_i2c->input_dev,
							 KEY_WAKEUP_UNLOCK, 0);
					input_sync(wac_i2c->input_dev);

					x = ((u16) data[1] << 8) + (u16) data[2];
					y = ((u16) data[3] << 8) + (u16) data[4];

					/* origin */
					x = x - pdata->origin[0];
					y = y - pdata->origin[1];
					/* change axis from wacom to lcd */
					if (pdata->x_invert)
						x = pdata->max_x - x;

					if (pdata->y_invert)
						y = pdata->max_y - y;

					if (pdata->xy_switch) {
						tmp = x;
						x = y;
						y = tmp;
					}

					wac_i2c->survey_pos.id = EPEN_POS_ID_SCREEN_OF_MEMO;
					wac_i2c->survey_pos.x = x;
					wac_i2c->survey_pos.y = y;
				} else {
					input_info(true, &wac_i2c->client->dev,
						   "AOP detected but skip report, screen_off_memo disabled\n");
				}
			} else if (data[10] == AOP_DOUBLE_TAB) {
				if ((wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) == EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) {
					input_info(true, &wac_i2c->client->dev, "Double Tab detected in AOD\n");

					x = ((u16) data[1] << 8) + (u16) data[2];
					y = ((u16) data[3] << 8) + (u16) data[4];
					pressure = ((u16) data[5] << 8) + (u16) data[6];
					gain = data[7];
					tilt_x = (s8) data[8];
					tilt_y = (s8) data[9];

					/* origin */
					x = x - pdata->origin[0];
					y = y - pdata->origin[1];
					/* change axis from wacom to lcd */
					if (pdata->x_invert) {
						x = pdata->max_x - x;
						tilt_x = -tilt_x;
					}

					if (pdata->y_invert) {
						y = pdata->max_y - y;
						tilt_y = -tilt_y;
					}

					if (pdata->xy_switch) {
						tmp = x;
						x = y;
						y = tmp;

						tmp = tilt_x;
						tilt_x = tilt_y;
						tilt_y = tmp;
					}

					if (data[0] & 0x40)
						wac_i2c->tool = BTN_TOOL_RUBBER;
					else
						wac_i2c->tool = BTN_TOOL_PEN;

					/* make press / release event for AOP double tab gesture */
					input_report_abs(wac_i2c->input_dev, ABS_X, x);
					input_report_abs(wac_i2c->input_dev, ABS_Y, y);
					input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
					input_sync(wac_i2c->input_dev);

					input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, pressure);
					input_report_key(wac_i2c->input_dev, BTN_TOUCH, 1);
					input_sync(wac_i2c->input_dev);

					input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
					input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
					input_sync(wac_i2c->input_dev);

					input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
					input_sync(wac_i2c->input_dev);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
					input_info(true, &wac_i2c->client->dev,
						   "x(%d), y(%d) P / R event\n", x, y);
#else
					input_info(true, &wac_i2c->client->dev, "P / R event\n");
#endif
				} else if (wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOT) {
					input_info(true, &wac_i2c->client->dev, "Double Tab detected\n");

					input_report_key(wac_i2c->input_dev, KEY_HOMEPAGE, 1);
					input_sync(wac_i2c->input_dev);
					input_report_key(wac_i2c->input_dev, KEY_HOMEPAGE, 0);
					input_sync(wac_i2c->input_dev);
				} else {
					input_info(true, &wac_i2c->client->dev,
						   "AOP Double Tab detected but skip report, aod & aot disabled\n");
				}
			} else {
				input_info(true, &wac_i2c->client->dev, "unknown AOP status\n");
			}
		} else {
			input_info(true, &wac_i2c->client->dev,
				   "AOP Disabled \n");
		}

		return IRQ_HANDLED;
	}

	wacom_i2c_coord(wac_i2c);

	return IRQ_HANDLED;
}

static irqreturn_t wacom_interrupt_pdct(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	struct i2c_client *client = wac_i2c->client;
	int ret;

	if (wac_i2c->query_status == false)
		return IRQ_HANDLED;

	wac_i2c->pen_pdct = gpio_get_value(wac_i2c->pdata->pdct_gpio);

	if (wac_i2c->pdata->use_garage) {
#if defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		input_info(true, &client->dev, "%s: pen is %s garage\n",
			   __func__, wac_i2c->pen_pdct ? "IN " : "OUT of");
#else
		input_info(true, &client->dev, "%s: pen is %s garage(%d)\n",
			   __func__, wac_i2c->pen_pdct ? "IN" : "OUT of",
			   (wacom_get_irq_state(wac_i2c) > 0));
#endif
		if (wac_i2c->pen_pdct)
			wac_i2c->function_result &= ~EPEN_EVENT_PEN_OUT;
		else
			wac_i2c->function_result |= EPEN_EVENT_PEN_OUT;

		if (!wac_i2c->screen_on) {
			/* in LPM, waiting blsp block resume */
			if (wac_i2c->pm_suspend) {
				wake_lock_timeout(&wac_i2c->wakelock,
						  msecs_to_jiffies(3 * MSEC_PER_SEC));
				/* waiting for blsp block resuming, if not occurs
				 * i2c error
				 */
				ret =
				    wait_for_completion_interruptible_timeout(
				     &wac_i2c->resume_done,
				     msecs_to_jiffies(1 * MSEC_PER_SEC));
				if (ret == 0) {
					input_err(true, &wac_i2c->client->dev,
						  "%s: LPM: pm resume is not handled [timeout]\n",
						  __func__);
					return IRQ_HANDLED;
				}
			}
		}

		input_report_switch(wac_i2c->input_dev_pen, SW_PEN_INSERT,
				    (wac_i2c->function_result & EPEN_EVENT_PEN_OUT));
		input_sync(wac_i2c->input_dev_pen);

		if (wac_i2c->function_result & EPEN_EVENT_PEN_OUT)
			wac_i2c->pen_out_count++;

		if (wac_i2c->epen_blocked ||
		    (wac_i2c->battery_saving_mode && !(wac_i2c->function_result & EPEN_EVENT_PEN_OUT))) {
			input_info(true, &client->dev,
				   "%s: %s & garage on. garage only mode\n", __func__,
				   wac_i2c->epen_blocked ? "epen blocked" : "ps on & pen in");
			wacom_i2c_set_survey_mode(wac_i2c,
						  EPEN_SURVEY_MODE_GARAGE_ONLY);
		} else if (wac_i2c->screen_on && wac_i2c->survey_mode) {
			input_info(true, &client->dev,
				   "%s: ps %s & pen %s & lcd on. normal mode\n",
				   __func__,
				   wac_i2c->battery_saving_mode? "on" : "off",
				   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in");

			wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_NONE);
		} else {
			input_info(true, &client->dev,
				   "%s: ps %s & pen %s & lcd %s. keep current mode(%s)\n",
				   __func__,
				   wac_i2c->battery_saving_mode? "on" : "off",
				   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
				   wac_i2c->screen_on ? "on" : "off",
				   wac_i2c->function_result & EPEN_EVENT_SURVEY ? "survey" : "normal");
		}
	}

	return IRQ_HANDLED;
}

static void pen_insert_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, pen_insert_dwork.work);

	if (wac_i2c->pdata->use_garage) {
		wac_i2c->pen_pdct = gpio_get_value(wac_i2c->pdata->pdct_gpio);

		input_info(true, &wac_i2c->client->dev, "%s: pdct(%d)\n",
			   __func__, wac_i2c->pen_pdct);

		if (wac_i2c->pen_pdct)
			wac_i2c->function_result &= ~EPEN_EVENT_PEN_OUT;
		else
			wac_i2c->function_result |= EPEN_EVENT_PEN_OUT;
	}

	input_report_switch(wac_i2c->input_dev_pen, SW_PEN_INSERT,
				(wac_i2c->function_result & EPEN_EVENT_PEN_OUT));
	input_sync(wac_i2c->input_dev_pen);

	input_info(true, &wac_i2c->client->dev, "%s : pen is %s\n", __func__,
			(wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "OUT" : "IN");
}

static void init_pen_insert(struct wacom_i2c *wac_i2c)
{
	INIT_DELAYED_WORK(&wac_i2c->pen_insert_dwork, pen_insert_work);

	/* update the current status */
	schedule_delayed_work(&wac_i2c->pen_insert_dwork, HZ * 5);
}

static int wacom_i2c_input_open(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);
	int ret = 0;

	input_info(true, &wac_i2c->client->dev, "%s(%s)\n", __func__,
		   dev->name);

#ifdef CONFIG_SEC_FACTORY
	if (wac_i2c->epen_blocked){
		input_err(true, &wac_i2c->client->dev, "%s : FAC epen_blocked SKIP!!\n", __func__);
		return ret;
	}
#endif

	wacom_wakeup_sequence(wac_i2c);

	return ret;
}

static void wacom_i2c_input_close(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);

#ifdef CONFIG_SEC_FACTORY
	if (wac_i2c->epen_blocked){
		input_err(true, &wac_i2c->client->dev, "%s : FAC epen_blocked SKIP!!\n", __func__);
		return;
	}
#endif

	input_info(true, &wac_i2c->client->dev, "%s(%s)\n", __func__,
		  dev->name);

	wacom_sleep_sequence(wac_i2c);
}

static void wacom_i2c_set_input_values(struct wacom_i2c *wac_i2c,
				       struct input_dev *input_dev, u8 propbit)
{
	struct i2c_client *client = wac_i2c->client;
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	/* Set input values before registering input device */

	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	input_dev->open = wacom_i2c_input_open;
	input_dev->close = wacom_i2c_input_close;

	input_set_capability(input_dev, EV_KEY, BTN_TOOL_PEN);
	input_set_capability(input_dev, EV_KEY, BTN_TOOL_RUBBER);

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, pdata->max_pressure,
			     0, 0);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, pdata->max_height,
			     0, 0);
	input_set_abs_params(input_dev, ABS_TILT_X, -pdata->max_x_tilt,
			     pdata->max_x_tilt, 0, 0);
	input_set_abs_params(input_dev, ABS_TILT_Y, -pdata->max_y_tilt,
			     pdata->max_y_tilt, 0, 0);

	/* AOP */
	input_set_capability(input_dev, EV_KEY, KEY_WAKEUP_UNLOCK);
	input_set_capability(input_dev, EV_KEY, KEY_HOMEPAGE);

	if (propbit & DEX_MODE_MOUSE) {
		input_set_capability(input_dev, EV_REL, REL_X);
		input_set_capability(input_dev, EV_REL, REL_Y);

		input_set_capability(input_dev, EV_KEY, BTN_LEFT);
		input_set_capability(input_dev, EV_KEY, BTN_RIGHT);
		/* input_set_capability(input_dev, EV_KEY, BTN_MIDDLE); */
	} else {
		int max_x, max_y;

		input_set_capability(input_dev, EV_SW, SW_PEN_INSERT);
		input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
		input_set_capability(input_dev, EV_KEY, BTN_STYLUS);

		/* input_set_capability(input_dev, EV_KEY, KEY_UNKNOWN); */
		/* input_set_capability(input_dev, EV_KEY, BTN_TOOL_SPEN_SCAN); */
		/* input_set_capability(input_dev, EV_KEY, KEY_PEN_PDCT); */
		/* input_set_capability(input_dev, EV_KEY, BTN_STYLUS2); */
		/* input_set_capability(input_dev, EV_KEY, ABS_MISC); */

		/* softkey */
		if (!pdata->use_virtual_softkey) {
			input_set_capability(input_dev, EV_KEY, KEY_RECENT);
			input_set_capability(input_dev, EV_KEY, KEY_BACK);
		}

		max_x = pdata->max_x;
		max_y = pdata->max_y;

		if (pdata->xy_switch) {
			input_set_abs_params(input_dev, ABS_X, 0, max_y, 4, 0);
			input_set_abs_params(input_dev, ABS_Y, 0, max_x, 4, 0);
		} else {
			input_set_abs_params(input_dev, ABS_X, 0, max_x, 4, 0);
			input_set_abs_params(input_dev, ABS_Y, 0, max_y, 4, 0);
		}
	}

	input_set_drvdata(input_dev, wac_i2c);
}

static void wacom_i2c_resume_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, resume_work.work);
	struct i2c_client *client = wac_i2c->client;
	u8 irq_state = 0;
	int retry = 1;
	int ret = 0;

reset:
	if (wac_i2c->reset_flag) {
		input_info(true, &client->dev,
			   "%s: IC reset start\n", __func__);

		wac_i2c->abnormal_reset_count++;
		wac_i2c->reset_flag = false;
		wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
		wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;

		wacom_enable_irq(wac_i2c, false);
		wacom_enable_pdct_irq(wac_i2c, false);

		wacom_reset_hw(wac_i2c);

		wac_i2c->pen_pdct =
		    gpio_get_value(wac_i2c->pdata->pdct_gpio);

		input_info(true, &client->dev,
			   "%s: IC reset end, pdct(%d)\n", __func__,
			   wac_i2c->pen_pdct);

		if (wac_i2c->pdata->use_garage) {
			if (wac_i2c->pen_pdct)
				wac_i2c->function_result &= ~EPEN_EVENT_PEN_OUT;
			else
				wac_i2c->function_result |= EPEN_EVENT_PEN_OUT;
		}

		wacom_enable_irq(wac_i2c, true);
		wacom_enable_pdct_irq(wac_i2c, true);
	}

	wacom_select_survey_mode(wac_i2c, true);

	if (wac_i2c->reset_flag && retry--)
		goto reset;

	if (wac_i2c->wcharging_mode)
		wacom_i2c_set_sense_mode(wac_i2c);

	if (wac_i2c->tsp_noise_mode < 0)
		wac_i2c->tsp_noise_mode = set_spen_mode(EPEN_GLOBAL_SCAN_MODE);

	irq_state = wacom_get_irq_state(wac_i2c);
	if (unlikely(irq_state > 0)) {
		u8 data[COM_COORD_NUM + 1] = { 0, };

		input_info(true, &client->dev, "%s: irq was enabled\n",
			   __func__);

		ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM + 1,
				     WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev,
				  "%s: failed to receive\n", __func__);
		}

		input_info(true, &client->dev,
			   "%x %x %x %x %x, %x %x %x %x %x, %x %x %x\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10],
			   data[11], data[12]);
	}

	if (!wac_i2c->samplerate_state) {
		char cmd = COM_SAMPLERATE_START;

		input_info(true, &client->dev, "%s: samplerate state is %d, need to recovery\n",
			   __func__, wac_i2c->samplerate_state);

		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev,
				  "failed to sned start cmd %d\n",
				  ret);
		} else {
			wac_i2c->samplerate_state = true;
		}
	}

	ret = gpio_get_value(wac_i2c->pdata->pdct_gpio);

	input_info(true, &client->dev,
		   "%s: i(%d) p(%d) set(0x%x) ret(0x%x) samplerate(%d)\n",
		   __func__, irq_state, ret, wac_i2c->function_set,
		   wac_i2c->function_result, wac_i2c->samplerate_state);
}

#ifdef LCD_FREQ_SYNC
#define SYSFS_WRITE_LCD	"/sys/class/lcd/panel/ldi_fps"
static void wacom_i2c_sync_lcd_freq(struct wacom_i2c *wac_i2c)
{
	int ret = 0;
	mm_segment_t old_fs;
	struct file *write_node;
	char freq[12] = { 0, };
	int lcd_freq = wac_i2c->lcd_freq;

	mutex_lock(&wac_i2c->freq_write_lock);

	snprintf(freq, 12, "%d", lcd_freq);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/* write_node = filp_open(SYSFS_WRITE_LCD, O_RDONLY | O_SYNC, 0664); */
	write_node = filp_open(SYSFS_WRITE_LCD, O_RDWR | O_SYNC, 0664);
	if (IS_ERR(write_node)) {
		ret = PTR_ERR(write_node);
		input_err(true, &wac_i2c->client->dev,
			  "%s: node file open fail, %d\n", __func__, ret);
		goto err_open_node;
	}

	ret = write_node->f_op->write(write_node, (char __user *)freq,
				      strlen(freq), &write_node->f_pos);
	if (ret != strlen(freq)) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: Can't write node data\n", __func__);
	}
	input_info(true, &wac_i2c->client->dev, "%s write freq %s\n",
		   __func__, freq);

	filp_close(write_node, current->files);

err_open_node:
	set_fs(old_fs);
	mutex_unlock(&wac_i2c->freq_write_lock);

	schedule_delayed_work(&wac_i2c->lcd_freq_done_work, HZ * 5);
}

static void wacom_i2c_finish_lcd_freq_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, lcd_freq_done_work.work);

	wac_i2c->lcd_freq_wait = false;

	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);
}

static void wacom_i2c_lcd_freq_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, lcd_freq_work);

	wacom_i2c_sync_lcd_freq(wac_i2c);
}
#endif

#ifdef WACOM_USE_SOFTKEY_BLOCK
static void wacom_i2c_block_softkey_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, softkey_block_work.work);

	wac_i2c->block_softkey = false;
}
#endif

int load_fw_built_in(struct wacom_i2c *wac_i2c, int fw_index)
{
	int retry = 3;
	int ret;
	const char *fw_load_path = NULL;
#ifdef CONFIG_SEC_FACTORY
	int index = 0;
#endif

	input_info(true, &wac_i2c->client->dev, "%s: load_fw_built_in (%d)\n",
		   __func__, fw_index);

	fw_load_path = wac_i2c->pdata->fw_path;

#ifdef CONFIG_SEC_FACTORY
	if (fw_index != FW_BUILT_IN) {
		if (fw_index == FW_FACTORY_GARAGE)
			index = 0;
		else if (fw_index == FW_FACTORY_UNIT)
			index = 1;

		ret = of_property_read_string_index(wac_i2c->client->dev.of_node,
						    "wacom,fw_fac_path", index,
						    &wac_i2c->pdata->fw_fac_path);
		if (ret) {
			input_err(true, &wac_i2c->client->dev,
				  "%s: failed to read fw_fac_path %d\n",
				  __func__, ret);
			wac_i2c->pdata->fw_fac_path =  NULL;
		}

		fw_load_path = wac_i2c->pdata->fw_fac_path;

		input_info(true, &wac_i2c->client->dev, "%s: load %s firmware\n",
			   __func__, fw_load_path);
	}
#endif

	if (fw_load_path == NULL) {
		input_err(true, wac_i2c->dev,
			  "Unable to open firmware. fw_path is NULL\n");
		return -EINVAL;
	}

	while (retry--) {
		ret =
		    request_firmware(&wac_i2c->firm_data,
				     fw_load_path,
				     &wac_i2c->client->dev);
		if (ret < 0) {
			input_err(true, &wac_i2c->client->dev,
				  "Unable to open firmware. ret %d retry %d\n",
				  ret, retry);
			continue;
		}
		break;
	}

	if (ret < 0)
		return ret;

	wac_i2c->fw_img = (struct fw_image *)wac_i2c->firm_data->data;

	return ret;
}

static int wacom_i2c_get_fw_size(struct wacom_i2c *wac_i2c)
{
	u32 fw_size = 0;

	if (wac_i2c->pdata->ic_type == 9020)
		fw_size = 144 * 1024;
	else
		fw_size = 128 * 1024;

	return fw_size;
}

static int load_fw_sdcard(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;
	unsigned int nSize;
	unsigned long nSize2;
	u8 *ums_data;

	nSize = wacom_i2c_get_fw_size(wac_i2c);
	nSize2 = nSize + sizeof(struct fw_image);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(WACOM_FW_PATH_SDCARD, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		input_err(true, &client->dev, "failed to open %s.\n",
			  WACOM_FW_PATH_SDCARD);
		ret = -ENOENT;
		set_fs(old_fs);
		return ret;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	input_info(true, &client->dev, "start, file path %s, size %ld Bytes\n",
		   WACOM_FW_PATH_SDCARD, fsize);

	if ((fsize != nSize) && (fsize != nSize2)) {
		input_err(true, &client->dev,
			  "UMS firmware size is different\n");
		ret = -EFBIG;
		goto out;
	}

	ums_data = kmalloc(fsize, GFP_KERNEL);
	if (!ums_data) {
		input_err(true, &client->dev, "%s, kmalloc failed\n", __func__);
		ret = -EFAULT;
		goto out;
	}

	nread = vfs_read(fp, (char __user *)ums_data, fsize, &fp->f_pos);
	input_info(true, &client->dev, "nread %ld Bytes\n", nread);
	if (nread != fsize) {
		input_err(true, &client->dev,
			  "failed to read firmware file, nread %ld Bytes\n",
			  nread);
		ret = -EIO;
		kfree(ums_data);
		goto out;
	}

	filp_close(fp, current->files);
	set_fs(old_fs);

	wac_i2c->fw_img = (struct fw_image *)ums_data;

	return 0;

out:
	filp_close(fp, current->files);
	set_fs(old_fs);
	return ret;
}

int wacom_i2c_load_fw(struct wacom_i2c *wac_i2c, u8 fw_path)
{
	int ret = 0;
	struct fw_image *fw_img;
	struct i2c_client *client = wac_i2c->client;

	switch (fw_path) {
	case FW_BUILT_IN:
#ifdef CONFIG_SEC_FACTORY
	case FW_FACTORY_GARAGE:
	case FW_FACTORY_UNIT:
#endif
		ret = load_fw_built_in(wac_i2c, fw_path);
		break;
	case FW_IN_SDCARD:
		ret = load_fw_sdcard(wac_i2c);
		break;
	default:
		input_info(true, &client->dev, "unknown path(%d)\n", fw_path);
		goto err_load_fw;
	}

	if (ret < 0)
		goto err_load_fw;

	fw_img = wac_i2c->fw_img;

	/* header check */
	if (fw_img->hdr_ver == 1 && fw_img->hdr_len == sizeof(struct fw_image)) {
		wac_i2c->fw_data = (u8 *) fw_img->data;
#if !defined(CONFIG_SEC_FACTORY)
		if (fw_path == FW_BUILT_IN) {
#else
		if ((fw_path == FW_BUILT_IN) || (fw_path == FW_FACTORY_GARAGE) ||
		    (fw_path == FW_FACTORY_UNIT)) {
#endif
			wac_i2c->fw_ver_file = fw_img->fw_ver1;
			memcpy(wac_i2c->fw_chksum, fw_img->checksum, 5);
		}
	} else {
		input_err(true, &client->dev, "no hdr\n");
		wac_i2c->fw_data = (u8 *) fw_img;
	}

	return ret;

err_load_fw:
	wac_i2c->fw_data = NULL;
	return ret;
}

void wacom_i2c_unload_fw(struct wacom_i2c *wac_i2c)
{
	switch (wac_i2c->fw_update_way) {
	case FW_BUILT_IN:
#ifdef CONFIG_SEC_FACTORY
	case FW_FACTORY_GARAGE:
	case FW_FACTORY_UNIT:
#endif
		release_firmware(wac_i2c->firm_data);
		break;
	case FW_IN_SDCARD:
		kfree(wac_i2c->fw_img);
		break;
	default:
		break;
	}

	wac_i2c->fw_img = NULL;
	wac_i2c->fw_update_way = FW_NONE;
	wac_i2c->firm_data = NULL;
	wac_i2c->fw_data = NULL;
}

int wacom_fw_update(struct wacom_i2c *wac_i2c, u8 fw_update_way, bool bforced)
{
	struct i2c_client *client = wac_i2c->client;
	u32 fw_ver_ic = wac_i2c->wac_feature->fw_version;
	int ret;

	input_info(true, &client->dev, "%s\n", __func__);

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		input_info(true, &client->dev,
			   "update is already running. pass\n");
		return 0;
	}

	mutex_lock(&wac_i2c->update_lock);
	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	/* release pen, if it is pressed */
	if (wac_i2c->pen_pressed || wac_i2c->side_pressed || wac_i2c->pen_prox)
		forced_release(wac_i2c);

	if (wac_i2c->fullscan_mode)
		forced_release_fullscan(wac_i2c);

	ret = wacom_i2c_load_fw(wac_i2c, fw_update_way);
	if (ret < 0) {
		input_info(true, &client->dev, "failed to load fw data\n");
		wac_i2c->wac_feature->update_status = FW_UPDATE_FAIL;
		goto err_update_load_fw;
	}
	wac_i2c->fw_update_way = fw_update_way;

	/* firmware info */
	input_info(true, &client->dev,
		   "wacom fw ver : 0x%x, new fw ver : 0x%x\n",
		   wac_i2c->wac_feature->fw_version, wac_i2c->fw_ver_file);

	if (!bforced) {
		if (fw_ver_ic == wac_i2c->fw_ver_file) {
			input_info(true, &client->dev, "pass fw update\n");
			wac_i2c->do_crc_check = true;
			/* need to check crc */
		} else if (fw_ver_ic > wac_i2c->fw_ver_file) {
			input_info(true, &client->dev,
				   "dont need to update fw\n");
			goto out_update_fw;
		}

		/* ic < file then update */
	}

	cancel_work_sync(&wac_i2c->update_work);
	schedule_work(&wac_i2c->update_work);
	mutex_unlock(&wac_i2c->update_lock);

	return 0;

out_update_fw:
	wacom_i2c_unload_fw(wac_i2c);
err_update_load_fw:
	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);
	mutex_unlock(&wac_i2c->update_lock);

	return 0;
}

static int wacom_i2c_remove(struct i2c_client *client);

static void wacom_i2c_update_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, update_work);
	struct i2c_client *client = wac_i2c->client;
	struct wacom_features *feature = wac_i2c->wac_feature;
	int ret = 0;
	int retry = 3;

	wake_lock(&wac_i2c->fw_wakelock);

	if (wac_i2c->fw_update_way == FW_NONE)
		goto end_fw_update;

	/* CRC Check */
	if (wac_i2c->do_crc_check) {
		wac_i2c->do_crc_check = false;

		ret = wacom_checksum(wac_i2c);
		if (ret) {
			input_info(true, &client->dev, "crc ok, do not update\n");
			goto end_fw_update;
		}

		input_info(true, &client->dev, "crc err, do update\n");
	}

	feature->update_status = FW_UPDATE_RUNNING;

	while (retry--) {
		ret = wacom_i2c_flash(wac_i2c);
		if (ret) {
			input_info(true, &client->dev,
				   "failed to flash fw(%d)\n", ret);
			continue;
		}
		break;
	}
	if (ret) {
		feature->update_status = FW_UPDATE_FAIL;
		feature->fw_version = 0;
		goto end_fw_update;
	}

	ret = wacom_i2c_query(wac_i2c);
	if (ret < 0) {
		input_info(true, &client->dev, "failed to query to IC(%d)\n",
			   ret);
		feature->update_status = FW_UPDATE_FAIL;
		goto end_fw_update;
	}

	feature->update_status = FW_UPDATE_PASS;

end_fw_update:
	wacom_i2c_unload_fw(wac_i2c);
#ifndef CONFIG_SEC_FACTORY
//	ret = wacom_open_test(wac_i2c);
//	if (ret)
//		input_err(true, &client->dev, "open test check failed\n");
#endif

	wake_unlock(&wac_i2c->fw_wakelock);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	/* ble reset signal is break by crc check and open test
	 * so, for recovery of ble signal need to turn off and on ble charging func.
	 */
//	wacom_ble_charge_mode(wac_i2c, EPEN_BLE_C_RESET);

	if (feature->update_status == FW_UPDATE_FAIL) {
		input_err(true, &client->dev,
				"%s : failed to download FW & unload wacom driver\n", __func__);
		wacom_i2c_remove(wac_i2c->client);
	}
}

static void wacom_usb_typec_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c = container_of(work, struct wacom_i2c, usb_typec_work.work);
	char data[5] = { 0 };
	int ret;

	if (wac_i2c->dp_connect_state == wac_i2c->dp_connect_cmd)
		return;

	if (!wac_i2c->power_enable) {
		input_err(true, &wac_i2c->client->dev,
				"%s: powered off now\n", __func__);
		return;
	}

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		input_err(true, &wac_i2c->client->dev,
			   "%s: fw update is running\n", __func__);
		return;
	}

	data[0] = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send stop cmd %d\n",
			  __func__, ret);
		return;
	}

	wac_i2c->samplerate_state = false;
	msleep(50);

	if (wac_i2c->dp_connect_cmd)
		data[0] = COM_SPECIAL_COMPENSATION;
	else
		data[0] = COM_NORMAL_COMPENSATION;

	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send table swap cmd %d\n",
			  __func__, ret);
		return;
	}

	msleep(30);

	data[0] = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send stop cmd %d\n",
			  __func__, ret);
		return;
	}

	data[0] = COM_SAMPLERATE_133;
	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send start cmd, %d\n",
			  __func__, ret);
		return;
	}
	wac_i2c->samplerate_state = true;

	input_info(true, &wac_i2c->client->dev, "%s: %s\n",
			__func__, wac_i2c->dp_connect_cmd ? "on" : "off");
}

static int wacom_usb_typec_notification_cb(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct wacom_i2c *wac_i2c = container_of(nb, struct wacom_i2c, typec_nb);
	CC_NOTI_TYPEDEF usb_typec_info = *(CC_NOTI_TYPEDEF *)data;

	if (usb_typec_info.src != CCIC_NOTIFY_DEV_CCIC ||
			usb_typec_info.dest != CCIC_NOTIFY_DEV_DP ||
			usb_typec_info.id != CCIC_NOTIFY_ID_DP_CONNECT)
		goto out;

	input_info(true, &wac_i2c->client->dev,
			"%s: %sed (vid:0x%04X pid:0x%04X)\n",
			__func__, usb_typec_info.sub1 ? "attach" : "detach",
			usb_typec_info.sub2, usb_typec_info.sub3);

	switch (usb_typec_info.sub1) {
	case CCIC_NOTIFY_ATTACH:
		if (usb_typec_info.sub2 != 0x04E8 ||
				usb_typec_info.sub3 != 0xA020)
			goto out;
		break;
	case CCIC_NOTIFY_DETACH:
		break;
	default:
		input_err(true, &wac_i2c->client->dev,
			"%s: invalid value %d\n", __func__, usb_typec_info.sub1);
		goto out;
	}

	cancel_delayed_work(&wac_i2c->usb_typec_work);
	wac_i2c->dp_connect_cmd = usb_typec_info.sub1;
	schedule_delayed_work(&wac_i2c->usb_typec_work, msecs_to_jiffies(1));
out:
	return 0;
}

static void wacom_usb_typec_nb_register_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c = container_of(work, struct wacom_i2c,
							typec_nb_reg_work.work);
	int ret;
	static int count;

	if (!wac_i2c || count > 100)
		return;

	ret = manager_notifier_register(&wac_i2c->typec_nb,
					wacom_usb_typec_notification_cb,
					MANAGER_NOTIFY_CCIC_WACOM);
	if (ret) {
		count++;
		schedule_delayed_work(&wac_i2c->typec_nb_reg_work, msecs_to_jiffies(10));
	} else {
		input_err(true, &wac_i2c->client->dev, "%s: success\n", __func__);
	}
}

static int wacom_request_gpio(struct i2c_client *client,
			      struct wacom_g5_platform_data *pdata)
{
	int ret;

	ret = devm_gpio_request(&client->dev, pdata->irq_gpio, "wacom_irq");
	if (ret) {
		input_err(true, &client->dev,
			  "unable to request gpio for irq [%d]\n",
			  pdata->irq_gpio);
		return ret;
	}

	ret = devm_gpio_request(&client->dev, pdata->pdct_gpio, "wacom_pdct");
	if (ret) {
		input_err(true, &client->dev,
			  "unable to request gpio for pdct [%d]\n",
			  pdata->pdct_gpio);
		return ret;
	}

	ret = devm_gpio_request(&client->dev, pdata->fwe_gpio, "wacom_fwe");
	if (ret) {
		input_err(true, &client->dev,
			  "unable to request gpio for fwe [%d]\n",
			  pdata->fwe_gpio);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_OF
static struct wacom_g5_platform_data *wacom_parse_dt(struct i2c_client *client)
{
	struct wacom_g5_platform_data *pdata;
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	u32 tmp[5] = { 0, };
	bool flag;
	int ret;

	if (!np)
		return ERR_PTR(-ENODEV);

	pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->irq_gpio = of_get_named_gpio(np, "wacom,irq-gpio", 0);
	if (!gpio_is_valid(pdata->irq_gpio)) {
		input_err(true, &client->dev, "failed to get irq-gpio\n");
		return ERR_PTR(-EINVAL);
	}

	pdata->pdct_gpio = of_get_named_gpio(np, "wacom,pdct-gpio", 0);
	if (!gpio_is_valid(pdata->pdct_gpio)) {
		input_err(true, &client->dev, "failed to get pdct-gpio\n");
		return ERR_PTR(-EINVAL);
	}

	pdata->fwe_gpio = of_get_named_gpio(np, "wacom,fwe-gpio", 0);
	if (!gpio_is_valid(pdata->fwe_gpio)) {
		input_err(true, &client->dev, "failed to get fwe-gpio\n");
		return ERR_PTR(-EINVAL);
	}

	/* get features */
	ret = of_property_read_u32(np, "wacom,irq_type", tmp);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read trigger type %d\n", ret);
		return ERR_PTR(-EINVAL);
	}
	pdata->irq_type = tmp[0];

	ret = of_property_read_u32(np, "wacom,boot_addr", tmp);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read boot address %d\n", ret);
		return ERR_PTR(-EINVAL);
	}
	pdata->boot_addr = tmp[0];

	ret = of_property_read_u32_array(np, "wacom,origin", pdata->origin, 2);
	if (ret) {
		input_err(true, dev, "failed to read origin %d\n", ret);
		return ERR_PTR(-EINVAL);
	}

	ret = of_property_read_u32_array(np, "wacom,max_coords", tmp, 2);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev,
				  "failed to read max coords %d\n", ret);
		}
		pdata->max_x = tmp[0];
		pdata->max_y = tmp[1];
	}
	pdata->use_dt_coord = of_property_read_bool(np, "wacom,use_dt_coord");

	ret = of_property_read_u32(np, "wacom,max_pressure", tmp);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev,
				  "failed to read max pressure %d\n", ret);
		}
		pdata->max_pressure = tmp[0];
	}

	ret = of_property_read_u32(np, "wacom,max_x_tilt", tmp);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev,
				  "failed to read max x tilt %d\n", ret);
		}
		pdata->max_x_tilt = tmp[0];
	}

	ret = of_property_read_u32(np, "wacom,max_y_tilt", tmp);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev,
				  "failed to read max y tilt %d\n", ret);
		}
		pdata->max_y_tilt = tmp[0];
	}

	ret = of_property_read_u32(np, "wacom,max_height", tmp);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev,
				  "failed to read max height %d\n", ret);
		}
		pdata->max_height = tmp[0];
	}

	ret = of_property_read_u32_array(np, "wacom,invert", tmp, 3);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read inverts %d\n", ret);
		return ERR_PTR(-EINVAL);
	}
	pdata->x_invert = tmp[0];
	pdata->y_invert = tmp[1];
	pdata->xy_switch = tmp[2];

	ret = of_property_read_u32(np, "wacom,ic_type", &pdata->ic_type);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read ic_type %d\n", ret);
		return ERR_PTR(-EINVAL);
	}

	ret = of_property_read_string(np, "wacom,fw_path", &pdata->fw_path);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read fw_path %d\n", ret);
	}

	ret = of_property_read_string_index(np, "wacom,project_name", 0,
					    &pdata->project_name);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read project name %d\n", ret);
	}

	ret = of_property_read_string_index(np, "wacom,project_name", 1,
					    &pdata->model_name);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read model name %d\n", ret);
	}

	flag = of_property_read_bool(np, "wacom,use_virtual_softkey");
	pdata->use_virtual_softkey = flag;

	flag = of_property_read_bool(np, "wacom,use_garage");
	pdata->use_garage = flag;

	flag = of_property_read_bool(np, "wacom,support_dex_mode");
	pdata->support_dex = flag;

	if (pdata->support_dex) {
		ret = of_property_read_u32(np, "wacom,dex_rate",
					   &pdata->dex_rate);
		if (ret) {
			input_err(true, &client->dev,
				  "failed to read dex_rate %d, default value is 10\n",
				  ret);
			pdata->dex_rate = 10;
		}
	}

	pdata->table_swap = of_property_read_bool(np, "wacom,table_swap_for_dex_station");

	input_info(true, &client->dev,
		   "boot_addr: 0x%X, origin: (%d,%d), max_coords: (%d,%d), "
		   "max_pressure: %d, max_height: %d, max_tilt: (%d,%d) "
		   "project_name: (%s,%s), invert: (%d,%d,%d), fw_path: %s, "
		   "ic_type: %d, %s virtual softkey, %s garage, support_dex:%d, dex_rate:%d, table_swap:%d\n",
		   pdata->boot_addr, pdata->origin[0], pdata->origin[1],
		   pdata->max_x, pdata->max_y, pdata->max_pressure,
		   pdata->max_height, pdata->max_x_tilt, pdata->max_y_tilt,
		   pdata->project_name, pdata->model_name, pdata->x_invert,
		   pdata->y_invert, pdata->xy_switch, pdata->fw_path,
		   pdata->ic_type,
		   pdata->use_virtual_softkey ? "enabled" : "disabled",
		   pdata->use_garage ? "enabled" : "disabled",
		   pdata->support_dex, pdata->dex_rate, pdata->table_swap);

	return pdata;
}
#else
static struct wacom_g5_platform_data *wacom_parse_dt(struct i2c_client *client)
{
	input_err(true, &client->dev, "no platform data specified\n");
	return ERR_PTR(-EINVAL);
}
#endif

static int wacom_i2c_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct wacom_g5_platform_data *pdata = dev_get_platdata(&client->dev);
	struct wacom_i2c *wac_i2c;
	struct input_dev *input;
	int ret = 0;

	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret) {
		input_err(true, &client->dev,
			  "I2C functionality not supported\n");
		return -EIO;
	}

	wac_i2c = devm_kzalloc(&client->dev, sizeof(*wac_i2c), GFP_KERNEL);
	if (!wac_i2c)
		return -ENOMEM;

	if (!pdata) {
		pdata = wacom_parse_dt(client);
		if (IS_ERR(pdata)) {
			input_err(true, &client->dev, "failed to parse dt\n");
			return PTR_ERR(pdata);
		}
	}

	ret = wacom_request_gpio(client, pdata);
	if (ret) {
		input_err(true, &client->dev, "failed to request gpio\n");
		return ret;
	}

	/* using managed input device */
	input = devm_input_allocate_device(&client->dev);
	if (!input) {
		input_err(true, &client->dev,
			  "failed to allocate input device\n");
		return -ENOMEM;
	}

	if (pdata->support_dex) {
		/* using new input device for relative coordinate for DeX */
		wac_i2c->input_dev_pad
				= devm_input_allocate_device(&client->dev);
		if (!wac_i2c->input_dev_pad) {
			input_err(true, &client->dev,
				  "failed to allocate input device pad\n");
			return -ENOMEM;
		}

		wac_i2c->input_dev_virtual
				= devm_input_allocate_device(&client->dev);
		if (!wac_i2c->input_dev_virtual) {
			input_err(true, &client->dev,
				  "failed to allocate input device virtual\n");
			return -ENOMEM;
		}
	}

	/* using 2 slave address. one is normal mode, another is boot mode for
	 * fw update.
	 */
	wac_i2c->client_boot = i2c_new_dummy(client->adapter, pdata->boot_addr);
	if (!wac_i2c->client_boot) {
		input_err(true, &client->dev,
			  "failed to register sub client[0x%x]\n",
			  pdata->boot_addr);
		return -ENOMEM;
	}

	wac_i2c->client = client;
	wac_i2c->pdata = pdata;
	wac_i2c->input_dev = input;
	wac_i2c->irq = gpio_to_irq(pdata->irq_gpio);
	wac_i2c->irq_pdct = gpio_to_irq(pdata->pdct_gpio);
	wac_i2c->pen_pdct = PDCT_NOSIGNAL;
	wac_i2c->fw_img = NULL;
	wac_i2c->fw_update_way = FW_NONE;
	wac_i2c->fullscan_mode = false;
	wac_i2c->wacom_noise_state = WACOM_NOISE_LOW;
	wac_i2c->tsp_noise_mode = EPEN_GLOBAL_SCAN_MODE;
	wac_i2c->wac_feature = &wacom_feature_EMR;
	wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
	wac_i2c->function_result = EPEN_EVENT_PEN_OUT;
	wac_i2c->battery_saving_mode = 0;	/* it needs */
	/* Consider about factory, it may be need to as default 1 */
	wac_i2c->reset_flag = false;
	wac_i2c->pm_suspend = false;
	wac_i2c->samplerate_state = true;

	wacom_get_drv_data(wac_i2c);

	/*Set client data */
	i2c_set_clientdata(client, wac_i2c);
	i2c_set_clientdata(wac_i2c->client_boot, wac_i2c);

	/* compensation to protect from flash mode */
	wacom_compulsory_flash_mode(wac_i2c, true);
	wacom_compulsory_flash_mode(wac_i2c, false);

	/* Power on */
	wacom_power(wac_i2c, true);
	wac_i2c->screen_on = true;
	/* need to delay for query */
	msleep(100);

	wacom_i2c_query(wac_i2c);

	input->name = "sec_e-pen";
	wacom_i2c_set_input_values(wac_i2c, input, DEX_MODE_STYLUS);

	if (pdata->support_dex) {
		wac_i2c->input_dev_pad->name = "sec_e-pen-pad";
		wacom_i2c_set_input_values(wac_i2c, wac_i2c->input_dev_pad,
					   DEX_MODE_MOUSE);
		wac_i2c->input_dev_virtual->name = "sec_virtual-e-pen";
		wacom_i2c_set_input_values(wac_i2c, wac_i2c->input_dev_virtual,
					   DEX_MODE_STYLUS);
	}
	wac_i2c->input_dev_pen = input;

	/*Initializing for semaphor */
	mutex_init(&wac_i2c->lock);
	mutex_init(&wac_i2c->update_lock);
	mutex_init(&wac_i2c->irq_lock);

	wake_lock_init(&wac_i2c->fw_wakelock, WAKE_LOCK_SUSPEND, "wacom");
	wake_lock_init(&wac_i2c->wakelock, WAKE_LOCK_SUSPEND, "wacom_wakelock");

	INIT_DELAYED_WORK(&wac_i2c->resume_work, wacom_i2c_resume_work);
	INIT_DELAYED_WORK(&wac_i2c->gxscan_work,
			  wacom_gxscan_work);
	INIT_DELAYED_WORK(&wac_i2c->fullscan_work,
			  wacom_fullscan_work);

	init_completion(&wac_i2c->resume_done);

#ifdef LCD_FREQ_SYNC
	mutex_init(&wac_i2c->freq_write_lock);
	INIT_WORK(&wac_i2c->lcd_freq_work, wacom_i2c_lcd_freq_work);
	INIT_DELAYED_WORK(&wac_i2c->lcd_freq_done_work,
			  wacom_i2c_finish_lcd_freq_work);
	wac_i2c->use_lcd_freq_sync = true;
#endif
#ifdef WACOM_USE_SOFTKEY_BLOCK
	INIT_DELAYED_WORK(&wac_i2c->softkey_block_work,
			  wacom_i2c_block_softkey_work);
	wac_i2c->block_softkey = false;
#endif
	INIT_WORK(&wac_i2c->update_work, wacom_i2c_update_work);

	ret = input_register_device(input);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to register input device\n");
		/* managed input devices need not be explicitly unregistred or freed. */
		goto err_register_input_dev;

	}

	if (pdata->support_dex) {
		ret = input_register_device(wac_i2c->input_dev_pad);
		if (ret) {
			input_err(true, &client->dev,
				  "failed to register input device pad\n");
			goto err_register_input_dev;
		}

		ret = input_register_device(wac_i2c->input_dev_virtual);
		if (ret) {
			input_err(true, &client->dev,
				  "failed to register input device virtual\n");
			goto err_register_input_dev;
		}
	}

	/*Request IRQ */
	ret = devm_request_threaded_irq(&client->dev, wac_i2c->irq, NULL,
					wacom_interrupt, IRQF_ONESHOT |
					pdata->irq_type,
					"sec_epen_irq", wac_i2c);
	if (ret < 0) {
		input_err(true, &client->dev,
			  "failed to request irq(%d) - %d\n", wac_i2c->irq,
			  ret);
		goto err_request_irq;
	}
	input_info(true, &client->dev, "init irq %d\n", wac_i2c->irq);

	ret = devm_request_threaded_irq(&client->dev, wac_i2c->irq_pdct, NULL,
					wacom_interrupt_pdct,
					IRQF_TRIGGER_FALLING | IRQF_ONESHOT |
					IRQF_TRIGGER_RISING,
					"sec_epen_pdct", wac_i2c);
	if (ret < 0) {
		input_err(true, &client->dev,
			  "failed to request pdct irq(%d) - %d\n",
			  wac_i2c->irq_pdct, ret);
		goto err_request_pdct_irq;
	}
	input_info(true, &client->dev, "init pdct %d\n", wac_i2c->irq_pdct);

	init_pen_insert(wac_i2c);

	ret = wacom_sec_init(wac_i2c);
	if (ret)
		goto err_sec_init;

	wacom_fw_update(wac_i2c, FW_BUILT_IN, false);

	device_init_wakeup(&client->dev, true);

	if (wac_i2c->pdata->table_swap) {
		INIT_DELAYED_WORK(&wac_i2c->usb_typec_work, wacom_usb_typec_work);
		INIT_DELAYED_WORK(&wac_i2c->typec_nb_reg_work,
					wacom_usb_typec_nb_register_work);
		schedule_delayed_work(&wac_i2c->typec_nb_reg_work, msecs_to_jiffies(10));
	}

	input_info(true, &client->dev, "probe done\n");

	return 0;

err_sec_init:
err_request_pdct_irq:
err_request_irq:
err_register_input_dev:
	wake_lock_destroy(&wac_i2c->fw_wakelock);
	wake_lock_destroy(&wac_i2c->wakelock);
#ifdef LCD_FREQ_SYNC
	mutex_destroy(&wac_i2c->freq_write_lock);
#endif
	mutex_destroy(&wac_i2c->irq_lock);
	mutex_destroy(&wac_i2c->update_lock);
	mutex_destroy(&wac_i2c->lock);

	i2c_unregister_device(wac_i2c->client_boot);

	input_err(true, &client->dev, "failed to probe\n");

	return ret;
}

#ifdef CONFIG_PM
static int wacom_i2c_suspend(struct device *dev)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	wac_i2c->pm_suspend = true;
	reinit_completion(&wac_i2c->resume_done);
#ifndef USE_OPEN_CLOSE
	if (wac_i2c->input_dev->users)
		wacom_sleep_sequence(wac_i2c);
#endif

	return 0;
}

static int wacom_i2c_resume(struct device *dev)
{
	struct wacom_i2c *wac_i2c = dev_get_drvdata(dev);

	wac_i2c->pm_suspend = false;
	complete_all(&wac_i2c->resume_done);
#ifndef USE_OPEN_CLOSE
	if (wac_i2c->input_dev->users)
		wacom_wakeup_sequence(wac_i2c);
#endif

	return 0;
}

static SIMPLE_DEV_PM_OPS(wacom_pm_ops, wacom_i2c_suspend, wacom_i2c_resume);
#endif

static void wacom_i2c_shutdown(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	if (!wac_i2c)
		return;

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wacom_power(wac_i2c, false);

	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);
}

static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	input_info(true, &wac_i2c->client->dev, "%s called!\n", __func__);

	device_init_wakeup(&client->dev, false);

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wacom_power(wac_i2c, false);

	wake_lock_destroy(&wac_i2c->fw_wakelock);
	wake_lock_destroy(&wac_i2c->wakelock);
	mutex_destroy(&wac_i2c->irq_lock);
	mutex_destroy(&wac_i2c->update_lock);
	mutex_destroy(&wac_i2c->lock);

	wacom_sec_remove(wac_i2c);

	i2c_unregister_device(wac_i2c->client_boot);

	if (wac_i2c->pdata->support_dex) {
		input_unregister_device(wac_i2c->input_dev_pad);
		input_unregister_device(wac_i2c->input_dev_virtual);
		wac_i2c->input_dev = wac_i2c->input_dev_pen;
	}

	input_unregister_device(wac_i2c->input_dev);

	return 0;
}

static const struct i2c_device_id wacom_i2c_id[] = {
	{"wacom_w90xx", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, wacom_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id wacom_dt_ids[] = {
	{.compatible = "wacom,w90xx"},
	{}
};
#endif

static struct i2c_driver wacom_i2c_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "wacom_w90xx",
#ifdef CONFIG_PM
		   .pm = &wacom_pm_ops,
#endif
		   .of_match_table = of_match_ptr(wacom_dt_ids),
		   },
	.probe = wacom_i2c_probe,
	.remove = wacom_i2c_remove,
	.shutdown = wacom_i2c_shutdown,
	.id_table = wacom_i2c_id,
};

static int __init wacom_i2c_init(void)
{
	int ret = 0;

#ifdef CONFIG_BATTERY_SAMSUNG
	if (lpcharge) {
		pr_info("%s: %s: Do not load driver due to : lpm %d\n",
			SECLOG, __func__, lpcharge);
		return ret;
	}
#endif
	ret = i2c_add_driver(&wacom_i2c_driver);
	if (ret)
		pr_err("%s: %s: failed to add i2c driver\n", SECLOG, __func__);

	return ret;
}

static void __exit wacom_i2c_exit(void)
{
	i2c_del_driver(&wacom_i2c_driver);
}

module_init(wacom_i2c_init);
module_exit(wacom_i2c_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("Driver for Wacom Digitizer Controller");
MODULE_LICENSE("GPL");
