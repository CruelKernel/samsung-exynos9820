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
#include <linux/spu-verify.h>

#include <linux/usb/manager/usb_typec_manager_notifier.h>

#include "wacom.h"

#ifdef CONFIG_DISPLAY_SAMSUNG
int get_lcd_attached(char *mode);
#endif

#ifdef CONFIG_EXYNOS_DPU20
int get_lcd_info(char *arg);
#endif

#define WACOM_I2C_RETRY		3

#define WACOM_PATH_EXTERNAL_FW			"/sdcard/FIRMWARE/WACOM/wacom.bin"
#define WACOM_PATH_EXTERNAL_FW_SIGNED	"/sdcard/FIRMWARE/WACOM/wacom_signed.bin"
#define WACOM_PATH_SPU_FW_SIGNED		"/spu/WACOM/ffu_wacom.bin"

#define WACOM_INVALID_IRQ_COUNT	2


enum WAKEUP_ID {
	HOVER_WAKEUP		= 2,
	SINGLETAP_WAKEUP,
	DOUBLETAP_WAKEUP,
};

enum SCAN_MODE {
	GLOBAL_Y_MODE		= 1,
	GLOBAL_X_MODE,
	FULL_MODE,
	LOCAL_MODE,
};

struct wacom_i2c *wacom_get_drv_data(void *data)
{
	static void *drv_data;

	if (unlikely(data))
		drv_data = data;

	return (struct wacom_i2c *)drv_data;
}

int coordc;

void forced_release(struct wacom_i2c *wac_i2c)
{
	input_info(true, &wac_i2c->client->dev, "%s\n", __func__);

	input_report_abs(wac_i2c->input_dev, ABS_X, 0);
	input_report_abs(wac_i2c->input_dev, ABS_Y, 0);
	input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
	input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
	input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
	/* prevent invalid operation of input booster */
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
}

int wacom_i2c_send(struct wacom_i2c *wac_i2c,
		   const char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ? wac_i2c->client_boot : wac_i2c->client;
	int retry = WACOM_I2C_RETRY;
	int ret;
	u8 *buff;

	buff = kzalloc(count, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	memcpy(buff, buf, count);

	do {
		if (!wac_i2c->power_enable) {
			input_err(true, &wac_i2c->client->dev, "%s: Power status off\n",
					__func__);
			ret = -EIO;

			goto out;
		}

		ret = i2c_master_send(client, buff, count);
		if (ret == count)
			break;

		if (retry < WACOM_I2C_RETRY) {
			input_err(true, &wac_i2c->client->dev, "%s: I2C retry(%d)\n",
					__func__, WACOM_I2C_RETRY - retry);
			wac_i2c->i2c_fail_count++;
		}
	} while (--retry);

out:
	kfree(buff);

	return ret;
}

int wacom_i2c_recv(struct wacom_i2c *wac_i2c, char *buf, int count, bool mode)
{
	struct i2c_client *client = mode ? wac_i2c->client_boot : wac_i2c->client;
	int retry = WACOM_I2C_RETRY;
	int ret;
	u8 *buff;

	buff = kzalloc(count, GFP_KERNEL);
	if (!buff)
		return -ENOMEM;

	do {
		if (!wac_i2c->power_enable) {
			input_err(true, &wac_i2c->client->dev, "%s: Power status off\n",
					__func__);
			ret = -EIO;

			goto out;
		}

		ret = i2c_master_recv(client, buff, count);
		if (ret == count)
			break;

		if (retry < WACOM_I2C_RETRY) {
			input_err(true, &wac_i2c->client->dev, "%s: I2C retry(%d)\n",
					__func__, WACOM_I2C_RETRY - retry);
			wac_i2c->i2c_fail_count++;
		}
	} while (--retry);

	memcpy(buf, buff, count);
out:
	kfree(buff);

	return ret;
}

int wacom_checksum(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int ret = 0, retry = 10;
	int i = 0;
	u8 buf[5] = { 0, };

	while (retry--) {
		buf[0] = COM_CHECKSUM;
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
	struct i2c_client *client = wac_i2c->client;
	u8 data[COM_QUERY_BUFFER] = { 0, };
	u8 *query = data + COM_QUERY_POS;
	int read_size = COM_QUERY_BUFFER;
	int ret;
	int i;
	int max_x, max_y, pressure, height, x_tilt, y_tilt;

	for (i = 0; i < RETRY_COUNT; i++) {
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
			wac_i2c->fw_ver_ic =
			    ((u16)query[EPEN_REG_FWVER1] << 8) +
			    query[EPEN_REG_FWVER2];
			break;
		}
	}

	input_info(true, &client->dev,
		   "%X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X, %X\n",
		   query[0], query[1], query[2], query[3], query[4], query[5],
		   query[6], query[7],  query[8], query[9], query[10], query[11],
		   query[12], query[13], query[14]);

	if (ret < 0) {
		input_err(true, &client->dev, "%s: failed to read query\n",
			  __func__);
		wac_i2c->fw_ver_ic = 0;
		wac_i2c->query_status = false;

		return ret;
	}

	wac_i2c->query_status = true;

	max_x = ((u16)query[EPEN_REG_X1] << 8) + query[EPEN_REG_X2];
	max_y = ((u16)query[EPEN_REG_Y1] << 8) + query[EPEN_REG_Y2];
	pressure = ((u16)query[EPEN_REG_PRESSURE1] << 8) + query[EPEN_REG_PRESSURE2];
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
	input_info(true, &client->dev, "fw_ver_ic=0x%X\n", wac_i2c->fw_ver_ic);
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

	mutex_lock(&wac_i2c->mode_lock);

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

			wac_i2c->reset_flag = true;
			msleep(200);
			input_info(true, &client->dev, "%s: reset_flag %d\n",
				   __func__, wac_i2c->reset_flag);
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

	mutex_unlock(&wac_i2c->mode_lock);
}

int wacom_i2c_set_survey_mode(struct wacom_i2c *wac_i2c, int mode)
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
			return -EPERM;
		}

		data[0] = COM_SURVEY_TARGET_GARAGEONLY;
		break;
	case EPEN_SURVEY_MODE_GARAGE_AOP:
		if ((wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
		    == EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
			data[0] = COM_SURVEYSYNCSCAN;
		else
			data[0] = COM_SURVEYSCAN;
		break;
	default:
		input_err(true, &client->dev,
			  "%s: wrong param %d\n", __func__, mode);
		return -EINVAL;
	}

	wac_i2c->survey_mode = mode;
	input_info(true, &client->dev, "%s: ps %s & mode : %d cmd(0x%2X)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off", mode, data[0]);

	retval = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (retval != 1) {
		input_err(true, &client->dev, "%s: failed to send data(%02x %d)\n",
			  __func__, data[0], retval);
		wac_i2c->reset_flag = true;

		return retval;
	}

	wac_i2c->check_survey_mode = mode;

	if (mode)
		msleep(300);

	wac_i2c->reset_flag = false;
	wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
	wac_i2c->function_result |= mode;

	return 0;
}

void forced_release_fullscan(struct wacom_i2c *wac_i2c)
{
	input_info(true, &wac_i2c->client->dev, "%s: full scan OUT\n", __func__);
	wac_i2c->tsp_scan_mode = set_scan_mode(DISABLE_TSP_SCAN_BLCOK);
	wac_i2c->is_tsp_block = false;
}

int wacom_power(struct wacom_i2c *wac_i2c, bool on)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	struct i2c_client *client = wac_i2c->client;
	static struct timeval off_time = { 0, 0 };
	struct timeval cur_time = { 0, 0 };
	int retval = 0;
	static struct regulator *vdd, *vddio;

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

	if (pdata->use_vddio && !vddio) {
		vddio = devm_regulator_get(&client->dev, "vddio");

		if (IS_ERR(vddio)) {
			input_err(true, &client->dev,
				  "%s: could not get vddio, rc = %ld\n",
				  __func__, PTR_ERR(vddio));
			vddio = NULL;
			return -ENODEV;
		}
		retval = regulator_set_voltage(vddio, 1800000, 1800000);
		if (retval)
			input_err(true, &client->dev,
				  "%s: unable to set vddio voltage to 1.8V\n",
				  __func__);
		input_err(true, &client->dev, "%s: 1.8V is enabled %s\n",
			  __func__,
			  regulator_is_enabled(vddio) ? "TRUE" : "FALSE");

	}

	if (!vdd) {
		vdd = devm_regulator_get(&client->dev, "vdd");

		if (IS_ERR(vdd)) {
			input_err(true, &client->dev,
				  "%s: could not get vdd, rc = %ld\n",
				  __func__, PTR_ERR(vdd));
			vdd = NULL;
			return -ENODEV;
		}
		retval = regulator_set_voltage(vdd, 3300000, 3300000);
		if (retval)
			input_err(true, &client->dev,
				  "%s: unable to set vdd voltage to 3.3V\n",
				  __func__);
		input_err(true, &client->dev, "%s: 3.3V is enabled %s\n",
			  __func__,
			  regulator_is_enabled(vdd) ? "TRUE" : "FALSE");

	}

	if (on) {
		if (pdata->use_vddio) {
			retval = regulator_enable(vddio);
			if (retval) {
				input_err(true, &client->dev,
					  "%s: Fail to enable regulator vddio[%d]\n",
					  __func__, retval);
			} else {
				input_err(true, &client->dev, "%s: vddio is enabled[OK]\n",
					  __func__);
			}
		}

		retval = regulator_enable(vdd);
		if (retval) {
			input_err(true, &client->dev,
				  "%s: Fail to enable regulator vdd[%d]\n",
				  __func__, retval);
		} else {
			input_err(true, &client->dev, "%s: vdd is enabled[OK]\n",
				  __func__);
		}
	} else {
		if (pdata->use_vddio) {
			if (regulator_is_enabled(vddio)) {
				retval = regulator_disable(vddio);
				if (retval) {
					input_err(true, &client->dev,
						  "%s: Fail to disable regulator vddio[%d]\n",
						  __func__, retval);

				} else {
					input_err(true, &client->dev,
						  "%s: vddio is disabled[OK]\n",
						  __func__);
				}
			} else {
				input_err(true, &client->dev,
					  "%s: vddio is already disabled\n",
					  __func__);
			}
		}

		if (regulator_is_enabled(vdd)) {
			retval = regulator_disable(vdd);
			if (retval) {
				input_err(true, &client->dev,
					  "%s: Fail to disable regulator vdd[%d]\n",
					  __func__, retval);

			} else {
				input_err(true, &client->dev,
					  "%s: vdd is disabled[OK]\n", __func__);
			}
		} else {
			input_err(true, &client->dev,
				  "%s: vdd is already disabled\n", __func__);
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

	cancel_delayed_work(&wac_i2c->work_print_info);
	wac_i2c->print_info_cnt_open = 0;
	wac_i2c->tsp_block_cnt = 0;
	schedule_work(&wac_i2c->work_print_info.work);

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
	cancel_delayed_work_sync(&wac_i2c->work_print_info);
	wacom_print_info(wac_i2c);

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

static void wac_i2c_table_swap_reply(struct wacom_i2c *wac_i2c, char *data)
{
	u8 table_id;

	table_id = data[3];

	if (table_id == 1) {
		if (wac_i2c->pdata->table_swap == TABLE_SWAP_DEX_STATION)
			wac_i2c->dp_connect_state = true;

		if (wac_i2c->pdata->table_swap == TABLE_SWAP_KBD_COVER)
			wac_i2c->kbd_cur_conn_state = true;
	} else if (!table_id) {
		if (wac_i2c->pdata->table_swap == TABLE_SWAP_DEX_STATION)
			wac_i2c->dp_connect_state = false;

		if (wac_i2c->pdata->table_swap == TABLE_SWAP_KBD_COVER)
			wac_i2c->kbd_cur_conn_state = false;
	}

}

static void wacom_i2c_reply_handler(struct wacom_i2c *wac_i2c, char *data)
{
	char pack_sub_id;
#if !defined(CONFIG_SEC_FACTORY) && defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	char buff;
	int ret;
#endif

	pack_sub_id = data[1];

	switch (pack_sub_id) {
	case SWAP_PACKET:
		wac_i2c_table_swap_reply(wac_i2c, data);
		break;
	case ELEC_TEST_PACKET:
		wac_i2c->check_elec++;
#if !defined(CONFIG_SEC_FACTORY) && defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		buff = COM_SAMPLERATE_STOP;
		ret = wacom_i2c_send(wac_i2c, &buff, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &wac_i2c->client->dev,
				  "%s: failed to send data(%02x %02x)\n",
				  __func__, buff, ret);
		} else {
			wac_i2c->samplerate_state = false;
		}

		msleep(50);

		buff = COM_SAMPLERATE_133;
		ret = wacom_i2c_send(wac_i2c, &buff, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &wac_i2c->client->dev,
				  "%s: failed to send data(%02x %02x)\n",
				  __func__, buff, ret);
			return;
		} else {
			wac_i2c->samplerate_state = true;
		}
#endif
		break;
	default:
		input_info(true, &wac_i2c->client->dev, "%s: unexpected packet %d\n",
			   __func__, pack_sub_id);
		break;
	};
}

static void wac_i2c_cmd_noti_handler(struct wacom_i2c *wac_i2c, char *data)
{
	wac_i2c->reset_flag = false;
	input_info(true, &wac_i2c->client->dev, "%s: reset_flag %d\n", __func__,
		   wac_i2c->reset_flag);
}

static void wac_i2c_block_tsp_scan(struct wacom_i2c *wac_i2c, char *data)
{
	bool tsp;
	u8 wacom_mode, scan_mode;

	wacom_mode = (data[2] & 0xF0) >> 4;
	scan_mode = data[2] & 0x0F;
	tsp = !!(data[5] & 0x80);

	if (scan_mode == 2)
		wac_i2c->is_mode_change = true;

	if (tsp && !wac_i2c->is_tsp_block) {
		input_info(true, &wac_i2c->client->dev, "%s: tsp:%d wacom mode:%d scan mode:%d\n",
			   __func__, tsp, wacom_mode, scan_mode);
		wac_i2c->is_tsp_block = true;
		wac_i2c->tsp_scan_mode = set_scan_mode(ENABLE_TSP_SCAN_BLCOK);
		wac_i2c->tsp_block_cnt++;
	} else if (!tsp && wac_i2c->is_tsp_block) {
		input_info(true, &wac_i2c->client->dev, "%s: tsp:%d wacom mode:%d scan mode:%d\n",
			   __func__, tsp, wacom_mode, scan_mode);
		wac_i2c->tsp_scan_mode = set_scan_mode(DISABLE_TSP_SCAN_BLCOK);
		wac_i2c->is_tsp_block = false;
	}
}

static void wacom_i2c_noti_handler(struct wacom_i2c *wac_i2c, char *data)
{
	char pack_sub_id;

	pack_sub_id = data[1];

	switch (pack_sub_id) {
	case ERROR_PACKET:
		if (data[4] == 0x10 && data[5] == 0x11) {
			wac_i2c->ble_disable_flag = true;
			input_err(true, &wac_i2c->client->dev, "%s: ble mode is disabled!\n", __func__);
		}
	case TABLE_SWAP_PACKET:
	case EM_NOISE_PACKET:
		break;
	case TSP_STOP_PACKET:
		wac_i2c_block_tsp_scan(wac_i2c, data);
		break;
	case OOK_PACKET:
		break;
	case CMD_PACKET:
		wac_i2c_cmd_noti_handler(wac_i2c, data);
		break;
	default:
		input_info(true, &wac_i2c->client->dev, "%s: unexpected packet %d\n",
			   __func__, pack_sub_id);
		break;
	};
}

static void wacom_i2c_aop_handler(struct wacom_i2c *wac_i2c, char *data)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	bool stylus, prox = false;
	s16 x, y, tmp;
	u8 wacom_mode, wakeup_id;

	if (wac_i2c->screen_on || !wac_i2c->survey_mode ||
	    !(wac_i2c->function_set & EPEN_SETMODE_AOP)) {
		input_info(true, &wac_i2c->client->dev, "%s: unexpected status(%d %d %d)\n",
			   __func__, wac_i2c->screen_on, wac_i2c->survey_mode,
			   wac_i2c->function_set & EPEN_SETMODE_AOP);
		return;
	}

	prox = !!(data[0] & 0x10);
	stylus = !!(data[0] & 0x20);
	x = ((u16)data[1] << 8) + (u16)data[2];
	y = ((u16)data[3] << 8) + (u16)data[4];
	wacom_mode = (data[6] & 0xF0) >> 4;
	wakeup_id = data[6] & 0x0F;

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

	if (data[0] & 0x40)
		wac_i2c->tool = BTN_TOOL_RUBBER;
	else
		wac_i2c->tool = BTN_TOOL_PEN;

	/* [for checking */
	input_info(true, &wac_i2c->client->dev, "wacom mode %d, wakeup id %d\n", wacom_mode, wakeup_id);
	/* for checking] */

	if (stylus && (wakeup_id == HOVER_WAKEUP) &&
	    (wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO)) {
		input_info(true, &wac_i2c->client->dev, "Hover & Side Button detected\n");

		input_report_key(wac_i2c->input_dev, KEY_WAKEUP_UNLOCK, 1);
		input_sync(wac_i2c->input_dev);

		input_report_key(wac_i2c->input_dev, KEY_WAKEUP_UNLOCK, 0);
		input_sync(wac_i2c->input_dev);

		wac_i2c->survey_pos.id = EPEN_POS_ID_SCREEN_OF_MEMO;
		wac_i2c->survey_pos.x = x;
		wac_i2c->survey_pos.y = y;
	} else if (wakeup_id == DOUBLETAP_WAKEUP) {
		if ((wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) == EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON) {
			input_info(true, &wac_i2c->client->dev, "Double Tab detected in AOD\n");

			/* make press / release event for AOP double tab gesture */
			input_report_abs(wac_i2c->input_dev, ABS_X, x);
			input_report_abs(wac_i2c->input_dev, ABS_Y, y);
			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 1);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 1);
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
			input_sync(wac_i2c->input_dev);

			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
			input_sync(wac_i2c->input_dev);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &wac_i2c->client->dev,
				   "[P/R] x:%d y:%d tool:%x\n", x, y, wac_i2c->tool);
#else
			input_info(true, &wac_i2c->client->dev, "[P/R] tool:%x\n",
				   wac_i2c->tool);
#endif
		} else if (wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOT) {
			input_info(true, &wac_i2c->client->dev, "Double Tab detected\n");

			input_report_key(wac_i2c->input_dev, KEY_HOMEPAGE, 1);
			input_sync(wac_i2c->input_dev);
			input_report_key(wac_i2c->input_dev, KEY_HOMEPAGE, 0);
			input_sync(wac_i2c->input_dev);
		}
	} else {
		input_info(true, &wac_i2c->client->dev,
			   "unexpected AOP data : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15]);
	}
}

#define EPEN_LOCATION_DETECT_SIZE	6
void epen_location_detect(struct wacom_i2c *wac_i2c, char *loc, int x, int y)
{
	int i;
	int max_x = wac_i2c->pdata->max_x;
	int max_y = wac_i2c->pdata->max_y;

	if (wac_i2c->pdata->xy_switch) {
		max_x = wac_i2c->pdata->max_y;
		max_y = wac_i2c->pdata->max_x;
	}

	if (wac_i2c->pdata->area_indicator == 0){
		/* approximately value */
		wac_i2c->pdata->area_indicator = max_y * 4 / 100;
		wac_i2c->pdata->area_navigation = max_y * 6 / 100;
		wac_i2c->pdata->area_edge = max_y * 3 / 100;

		input_raw_info(true, &wac_i2c->client->dev,
		   "MAX XY(%d/%d), area_edge %d, area_indicator %d, area_navigation %d\n",
		   max_x, max_y, wac_i2c->pdata->area_edge,
		   wac_i2c->pdata->area_indicator, wac_i2c->pdata->area_navigation);
	}

	for (i = 0; i < EPEN_LOCATION_DETECT_SIZE; ++i)
		loc[i] = 0;

	if (x < wac_i2c->pdata->area_edge)
		strcat(loc, "E.");
	else if (x < (max_x - wac_i2c->pdata->area_edge))
		strcat(loc, "C.");
	else
		strcat(loc, "e.");

	if (y < wac_i2c->pdata->area_indicator)
		strcat(loc, "S");
	else if (y < (max_y - wac_i2c->pdata->area_navigation))
		strcat(loc, "C");
	else
		strcat(loc, "N");
}

static void wacom_i2c_coord_handler(struct wacom_i2c *wac_i2c, char *data)
{
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	struct i2c_client *client = wac_i2c->client;
	bool prox = false;
	bool rdy = false;
	bool stylus;
	static int old_x = 0, old_y = 0;
	s16 x, y, pressure;
	s16 tmp;
	u8 height = 0;
	s8 tilt_x = 0;
	s8 tilt_y = 0;
	char location[EPEN_LOCATION_DETECT_SIZE] = { 0, };

	rdy = !!(data[0] & 0x80);

	x = ((u16)data[1] << 8) + (u16)data[2];
	y = ((u16)data[3] << 8) + (u16)data[4];
	pressure = ((u16)(data[5] & 0x0F) << 8) + (u16)data[6];
	height = (u8)data[7];
	tilt_x = (s8)data[8];
	tilt_y = (s8)data[9];

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

	if (rdy) {
		prox = !!(data[0] & 0x10);
		stylus = !!(data[0] & 0x20);

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
				return;
			} else if (wac_i2c->keyboard_area == true &&
				wac_i2c->virtual_tracking == EPEN_POS_COVER) {
				/*input_info(true, &client->dev,
				 *"skip  keyboard area  prox(%d)\n", prox);
				 */
				return;
			}

			/*input_info(true, &client->dev,
			 *"go through, prox(%d) y(%d) area(%d)\n", prox,y,wac_i2c->keyboard_area);
			 */
		}

		/* validation check */
		if (unlikely (x < 0 || y < 0 || x > pdata->max_y || y > pdata->max_x)) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev, "raw data x=%d, y=%d\n", x, y);
#endif
			return;
		}

		if (!wac_i2c->pen_prox) {
			wac_i2c->pen_prox = true;
			wac_i2c->virtual_tracking = EPEN_POS_VIEW;

			if (data[0] & 0x40)
				wac_i2c->tool = BTN_TOOL_RUBBER;
			else
				wac_i2c->tool = BTN_TOOL_PEN;

			epen_location_detect(wac_i2c, location, x, y);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev, "[HI] x:%d y:%d loc:%s (%s) \n",
					x, y, location, wac_i2c->tool == BTN_TOOL_PEN ? "pen" : "rubber");
#else
			input_info(true, &client->dev, "[HI] loc:%s (%s)\n",
					location, wac_i2c->tool == BTN_TOOL_PEN ? "p" : "r");
#endif
			return;
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

			input_info(true, &client->dev,
				"virtual hover out by keyboard cover\n");
			wac_i2c->pen_prox = 0;
			wac_i2c->pen_pressed = 0;
			wac_i2c->side_pressed = 0;
			wac_i2c->virtual_tracking = EPEN_POS_COVER;
			return;
		}

		/* report info */
		input_report_abs(wac_i2c->input_dev, ABS_X, x);
		input_report_abs(wac_i2c->input_dev, ABS_Y, y);
		input_report_key(wac_i2c->input_dev, BTN_STYLUS, stylus);
		input_report_key(wac_i2c->input_dev, BTN_TOUCH, prox);

		input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, pressure);
		input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, height);
		input_report_abs(wac_i2c->input_dev, ABS_TILT_X, tilt_x);
		input_report_abs(wac_i2c->input_dev, ABS_TILT_Y, tilt_y);
		input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
		input_sync(wac_i2c->input_dev);

		old_x = x;
		old_y = y;
		wac_i2c->mcount++;

		/* log */
		if (prox && !wac_i2c->pen_pressed) {
			epen_location_detect(wac_i2c, location, x, y);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev,
				   "[P] x:%d y:%d p:%d loc:%s tool:%x mc:%d\n",
					x, y, pressure, location, wac_i2c->tool, wac_i2c->mcount);
#else
			input_info(true, &client->dev, "[P] p:%d loc:%s tool:%x mc:%d\n",
				   pressure, location, wac_i2c->tool, wac_i2c->mcount);
#endif
		} else if (!prox && wac_i2c->pen_pressed) {
			epen_location_detect(wac_i2c, location, x, y);
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			input_info(true, &client->dev,
				   "[R] lx:%d ly:%d loc:%s tool:%x mc:%d\n",
				   x, y, location, wac_i2c->tool, wac_i2c->mcount);
#else
			input_info(true, &client->dev, "[R] loc:%s tool:%x mc:%d\n",
				   location, wac_i2c->tool, wac_i2c->mcount);
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
			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
			input_report_key(wac_i2c->input_dev, BTN_STYLUS, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
			/* prevent invalid operation of input booster */
			input_sync(wac_i2c->input_dev);

			input_report_abs(wac_i2c->input_dev, ABS_DISTANCE, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOOL_RUBBER, 0);
			input_report_key(wac_i2c->input_dev, BTN_TOOL_PEN, 0);
			input_sync(wac_i2c->input_dev);

			epen_location_detect(wac_i2c, location, x, y);

			if (wac_i2c->pen_pressed) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &client->dev,
					   "[R] lx:%d ly:%d loc:%s tool:%x mc:%d & [HO]\n",
					   x, y, location, wac_i2c->tool, wac_i2c->mcount);
#else
				input_info(true, &client->dev,
					   "[R] loc:%s tool:%x mc:%d & [HO]\n",
					   location, wac_i2c->tool, wac_i2c->mcount);
#endif
			} else {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
				input_info(true, &client->dev, "[HO] lx:%d ly:%d loc:%s mc:%d\n",
						x, y, location, wac_i2c->mcount);
#else
				input_info(true, &client->dev, "[HO] loc:%s mc:%d\n",
						location, wac_i2c->mcount);
#endif
			}

		} else {
			input_info(true, &client->dev,
			   "unexpected data : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
			   data[0], data[1], data[2], data[3], data[4], data[5],
			   data[6], data[7], data[8], data[9], data[10], data[11],
			   data[12], data[13], data[14], data[15]);
		}

		wac_i2c->pen_prox = 0;
		wac_i2c->pen_pressed = 0;
		wac_i2c->side_pressed = 0;
		wac_i2c->mcount = 0;
		wac_i2c->virtual_tracking = EPEN_POS_NONE;
	}

	return;
}

static void wacom_event_handler(struct wacom_i2c *wac_i2c, char *buff)
{
	char pack_id;
	bool debug = true;

	pack_id = *buff & 0x0F;

	switch (pack_id) {
	case COORD_PACKET:
		wacom_i2c_coord_handler(wac_i2c, buff);
		debug = false;
		break;
	case AOP_PACKET:
		wacom_i2c_aop_handler(wac_i2c, buff);
		break;
	case NOTI_PACKET:
		wacom_i2c_noti_handler(wac_i2c, buff);
		break;
	case REPLY_PACKET:
		wacom_i2c_reply_handler(wac_i2c, buff);
		break;
	case SEPC_PACKET:
		break;
	default:
		input_info(true, &wac_i2c->client->dev, "%s: unexpected packet %d\n",
			   __func__, pack_id);
		break;
	};

	if (debug) {
		input_info(true, &wac_i2c->client->dev,
		   "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
		   buff[0], buff[1], buff[2], buff[3], buff[4], buff[5],
		   buff[6], buff[7], buff[8], buff[9], buff[10], buff[11],
		   buff[12], buff[13], buff[14], buff[15]);
	}

}

static irqreturn_t wacom_interrupt(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	char data[COM_COORD_NUM + 1] = { 0, };
	int ret = 0;

	/* in LPM, waiting blsp block resume */
	if (wac_i2c->pm_suspend) {
		wake_lock_timeout(&wac_i2c->wakelock, msecs_to_jiffies(500));
		/* waiting for blsp block resuming, if not occurs i2c error */
		ret = wait_for_completion_interruptible_timeout(
				&wac_i2c->resume_done, msecs_to_jiffies(500));
		if (ret <= 0) {
			input_err(true, &wac_i2c->client->dev,
				  "LPM: pm resume is not handled [%d]\n",
				  ret);
			goto out;
		}

		input_info(true, &wac_i2c->client->dev, "run LPM interrupt handler [%d]\n",
				  jiffies_to_msecs(ret));
	}

	ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM + 1,
			     WACOM_I2C_MODE_NORMAL);
	if (ret > 0) {
		wacom_event_handler(wac_i2c, data);

		goto out;
	}

	forced_release(wac_i2c);
	forced_release_fullscan(wac_i2c);

	wac_i2c->reset_flag = true;
	if (!work_busy(&wac_i2c->resume_work.work)) {
		input_info(true, &wac_i2c->client->dev,
			   "%s schedule resume work\n", __func__);

		cancel_delayed_work(&wac_i2c->resume_work);
		schedule_delayed_work(&wac_i2c->resume_work,
			msecs_to_jiffies(EPEN_RESUME_DELAY));
	} else {
		input_err(true, &wac_i2c->client->dev,
			  "%s resume work is busy\n", __func__);
	}

out:
	return IRQ_HANDLED;
}

static irqreturn_t wacom_interrupt_pdct(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	struct i2c_client *client = wac_i2c->client;
	int ret;
	struct timeval current_time;

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
		if (wac_i2c->pen_pdct) {
			do_gettimeofday(&current_time);
			wac_i2c->chg_time_stamp = (int)(current_time.tv_sec);
			input_err(true, &client->dev, "%s: chg_time_stamp(%d)\n",
				  __func__, wac_i2c->chg_time_stamp);
		}

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

		input_report_switch(wac_i2c->input_dev, SW_PEN_INSERT,
				    (wac_i2c->function_result & EPEN_EVENT_PEN_OUT));
		input_sync(wac_i2c->input_dev);

		if (wac_i2c->function_result & EPEN_EVENT_PEN_OUT)
			wac_i2c->pen_out_count++;

		if (wac_i2c->epen_blocked ||
		    (wac_i2c->battery_saving_mode && !(wac_i2c->function_result & EPEN_EVENT_PEN_OUT))) {
			input_info(true, &client->dev,
				   "%s: %s & garage on. garage only mode\n", __func__,
				   wac_i2c->epen_blocked ? "epen blocked" : "ps on & pen in");

			mutex_lock(&wac_i2c->mode_lock);
			wacom_i2c_set_survey_mode(wac_i2c,
						  EPEN_SURVEY_MODE_GARAGE_ONLY);
			mutex_unlock(&wac_i2c->mode_lock);
		} else if (wac_i2c->screen_on && wac_i2c->survey_mode) {
			input_info(true, &client->dev,
				   "%s: ps %s & pen %s & lcd on. normal mode\n",
				   __func__,
				   wac_i2c->battery_saving_mode? "on" : "off",
				   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in");

			mutex_lock(&wac_i2c->mode_lock);
			wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_NONE);
			mutex_unlock(&wac_i2c->mode_lock);
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

	input_report_switch(wac_i2c->input_dev, SW_PEN_INSERT,
				(wac_i2c->function_result & EPEN_EVENT_PEN_OUT));
	input_sync(wac_i2c->input_dev);

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

	input_info(true, &wac_i2c->client->dev, "%s(%s)\n", __func__,
		  dev->name);

#ifdef CONFIG_SEC_FACTORY
	if (wac_i2c->epen_blocked){
		input_err(true, &wac_i2c->client->dev, "%s : FAC epen_blocked SKIP!!\n", __func__);
		return;
	}
#endif

	wacom_sleep_sequence(wac_i2c);
}

static void wacom_i2c_set_input_values(struct wacom_i2c *wac_i2c,
				       struct input_dev *input_dev)
{
	struct i2c_client *client = wac_i2c->client;
	struct wacom_g5_platform_data *pdata = wac_i2c->pdata;
	/* Set input values before registering input device */

	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	input_dev->open = wacom_i2c_input_open;
	input_dev->close = wacom_i2c_input_close;

	input_set_abs_params(input_dev, ABS_PRESSURE, 0, pdata->max_pressure,
			     0, 0);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, pdata->max_height,
			     0, 0);
	input_set_abs_params(input_dev, ABS_TILT_X, -pdata->max_x_tilt,
			     pdata->max_x_tilt, 0, 0);
	input_set_abs_params(input_dev, ABS_TILT_Y, -pdata->max_y_tilt,
			     pdata->max_y_tilt, 0, 0);

	if (pdata->xy_switch) {
		input_set_abs_params(input_dev, ABS_X, 0, pdata->max_y, 4, 0);
		input_set_abs_params(input_dev, ABS_Y, 0, pdata->max_x, 4, 0);
	} else {
		input_set_abs_params(input_dev, ABS_X, 0, pdata->max_x, 4, 0);
		input_set_abs_params(input_dev, ABS_Y, 0, pdata->max_y, 4, 0);
	}

	input_set_capability(input_dev, EV_KEY, BTN_TOOL_PEN);
	input_set_capability(input_dev, EV_KEY, BTN_TOOL_RUBBER);
	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);
	input_set_capability(input_dev, EV_KEY, BTN_STYLUS);

	input_set_capability(input_dev, EV_SW, SW_PEN_INSERT);

	/* AOP */
	input_set_capability(input_dev, EV_KEY, KEY_WAKEUP_UNLOCK);
	input_set_capability(input_dev, EV_KEY, KEY_HOMEPAGE);

	input_set_drvdata(input_dev, wac_i2c);
}
void wacom_print_info(struct wacom_i2c *wac_i2c)
{
	if (!wac_i2c)
		return;

	if (!wac_i2c->client)
		return;

	wac_i2c->print_info_cnt_open++;

	if (wac_i2c->print_info_cnt_open > 0xfff0)
		wac_i2c->print_info_cnt_open = 0;
	if (wac_i2c->scan_info_fail_cnt > 1000)
		wac_i2c->scan_info_fail_cnt = 1000;

	input_info(true, &wac_i2c->client->dev,
		"%s: garage %s, ps %s, pen %s, chg %s, ble_mode(0x%x%s), epen %s, cover %s, count(%u,%u,%u), mode(%d), block_cnt(%d), check(%d), ver[0x%x] #%d\n",
		__func__, wac_i2c->pdata->use_garage ? "used" : "unused",
		wac_i2c->battery_saving_mode ?  "on" : "off",
		(wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
		wac_i2c->is_mode_change ? "true" : "false", wac_i2c->ble_mode,
		wac_i2c->ble_disable_flag ? ":disabled" : "",
		wac_i2c->epen_blocked ? "blocked" : "unblocked",
		wac_i2c->keyboard_cover_mode ? "on" : "off",
		wac_i2c->i2c_fail_count, wac_i2c->abnormal_reset_count, wac_i2c->scan_info_fail_cnt,
		wac_i2c->check_survey_mode, wac_i2c->tsp_block_cnt, wac_i2c->check_elec,
		wac_i2c->fw_ver_ic, wac_i2c->print_info_cnt_open);
}

static void wacom_print_info_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c = container_of(work, struct wacom_i2c, work_print_info.work);

	wacom_print_info(wac_i2c);
	schedule_delayed_work(&wac_i2c->work_print_info, msecs_to_jiffies(30000));	// 30s
}

static void wacom_i2c_resume_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, resume_work.work);
	struct i2c_client *client = wac_i2c->client;
	u8 irq_state = 0;
	int retry = 1;
	int ret = 0;

	if (!wac_i2c->reset_flag && wac_i2c->ble_disable_flag) {
		input_info(true, &client->dev,
			   "%s: ble is diabled & send enable cmd!\n", __func__);

		mutex_lock(&wac_i2c->ble_charge_mode_lock);
		wacom_ble_charge_mode(wac_i2c, EPEN_BLE_C_ENABLE);
		wac_i2c->ble_disable_flag = false;
		mutex_unlock(&wac_i2c->ble_charge_mode_lock);
	}

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

	if (wac_i2c->tsp_scan_mode < 0) {
		wac_i2c->tsp_scan_mode = set_scan_mode(DISABLE_TSP_SCAN_BLCOK);
		wac_i2c->is_tsp_block = false;
	}

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
		input_err(true, &wac_i2c->client->dev,
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

static int load_fw_sdcard(struct wacom_i2c *wac_i2c, const char *file_path)
{
	struct i2c_client *client = wac_i2c->client;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	int ret = 0;
	unsigned int nSize;
	unsigned long nSize2;
	u8 *ums_data;
	long spu_fsize, spu_ret;
	u8 *spu_ums_data;

	nSize = wacom_i2c_get_fw_size(wac_i2c);
	nSize2 = nSize + sizeof(struct fw_image);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(file_path, O_RDONLY, S_IRUSR);

	if (IS_ERR(fp)) {
		input_err(true, &client->dev, "failed to open %s.\n",
			  file_path);
		ret = -ENOENT;
		set_fs(old_fs);
		return ret;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	input_info(true, &client->dev, "start, file path %s, size %ld Bytes\n",
		   file_path, fsize);

	if (strncmp(file_path, WACOM_PATH_EXTERNAL_FW_SIGNED, strlen(WACOM_PATH_EXTERNAL_FW_SIGNED)) == 0
		|| strncmp(file_path, WACOM_PATH_SPU_FW_SIGNED, strlen(WACOM_PATH_SPU_FW_SIGNED)) == 0) {
		/* name 5, digest 32, signature 512 */
		spu_fsize = fsize;
		fsize -= SPU_METADATA_SIZE(WACOM);
	}

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

	if (strncmp(file_path, WACOM_PATH_EXTERNAL_FW_SIGNED, strlen(WACOM_PATH_EXTERNAL_FW_SIGNED)) == 0
		|| strncmp(file_path, WACOM_PATH_SPU_FW_SIGNED, strlen(WACOM_PATH_SPU_FW_SIGNED)) == 0) {
		spu_ums_data = kmalloc(spu_fsize, GFP_KERNEL);
		if (!spu_ums_data) {
			input_err(true, &client->dev, "%s, kmalloc failed\n", __func__);
			ret = -EFAULT;
			kfree(ums_data);
			goto out;
		}

		nread = vfs_read(fp, (char __user *)spu_ums_data, spu_fsize, &fp->f_pos);
		input_info(true, &client->dev, "nread %ld Bytes\n", nread);
		if (nread != spu_fsize) {
			input_err(true, &client->dev,
				  "failed to read spu firmware file, nread %ld Bytes\n",
				  nread);
			ret = -EIO;
		
			kfree(spu_ums_data);
			kfree(ums_data);
			goto out;
		}

		spu_ret = spu_firmware_signature_verify("WACOM", spu_ums_data, spu_fsize);
		
		input_info(true, &client->dev, "%s: spu_ret : %ld, spu_fsize : %ld // fsize:%ld\n", __func__, spu_ret, spu_fsize, fsize);

		if (spu_ret != fsize) {
			input_err(true, &client->dev, "%s: signature verify failed, %ld\n", __func__, spu_ret);
			ret = -ENOENT;
			kfree(spu_ums_data);
			kfree(ums_data);
			goto out;
		}

		memcpy(ums_data, spu_ums_data, fsize);
		kfree(spu_ums_data);

	} else {

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

#if 0
static int load_fw_ffu(struct wacom_i2c *wac_i2c)
{
	int ret;
	const char *fw_load_path = WACOM_FW_PATH_FFU;

	input_info(true, &wac_i2c->client->dev, "%s: (%s)\n", __func__, fw_load_path);

	ret = request_firmware(&wac_i2c->firm_data, fw_load_path, &wac_i2c->client->dev);
	if (ret < 0) {
		input_err(true, &wac_i2c->client->dev, "Unable to open firmware. ret %d\n",ret);
		return ret;
	}

	wac_i2c->fw_img = (struct fw_image *)wac_i2c->firm_data->data;

	return 0;
}
#endif

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
		ret = load_fw_sdcard(wac_i2c, WACOM_PATH_EXTERNAL_FW);
		break;
	case FW_IN_SDCARD_SIGNED:
		ret = load_fw_sdcard(wac_i2c, WACOM_PATH_EXTERNAL_FW_SIGNED);
		break;
	case FW_SPU:
		ret = load_fw_sdcard(wac_i2c, WACOM_PATH_SPU_FW_SIGNED);
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
		if ((fw_path == FW_BUILT_IN) ||
		    (fw_path == FW_FACTORY_UNIT) || (fw_path == FW_FACTORY_GARAGE)) {
#endif
			wac_i2c->fw_ver_bin = fw_img->fw_ver1;
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
	case FW_IN_SDCARD_SIGNED:
	case FW_SPU:
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
	u32 fw_ver_ic = wac_i2c->fw_ver_ic;
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

	if (wac_i2c->is_tsp_block)
		forced_release_fullscan(wac_i2c);

	ret = wacom_i2c_load_fw(wac_i2c, fw_update_way);
	if (ret < 0) {
		input_info(true, &client->dev, "failed to load fw data\n");
		wac_i2c->update_status = FW_UPDATE_FAIL;
		goto err_update_load_fw;
	}
	wac_i2c->fw_update_way = fw_update_way;

	/* firmware info */
	input_info(true, &client->dev,
		   "wacom fw ver : 0x%x, new fw ver : 0x%x\n",
		   wac_i2c->fw_ver_ic, wac_i2c->fw_ver_bin);

	if (wac_i2c->pdata->bringup == 1) {
		input_info(true, &client->dev, "bringup. do not update\n");
		wac_i2c->update_status = FW_UPDATE_FAIL;
		goto out_update_fw;
	}

	if (wac_i2c->pdata->bringup == 3) {
		input_info(true, &client->dev, "bringup. force update\n");
		bforced = true;
	}

	/* If FFU firmware version is lower than IC's version, do not run update routine */
	if (fw_update_way == FW_SPU && fw_ver_ic >= wac_i2c->fw_ver_bin) {
		input_info(true, &client->dev, "FFU. update is skipped\n");
		wac_i2c->update_status = FW_UPDATE_PASS;

		wacom_i2c_unload_fw(wac_i2c);
		wacom_enable_irq(wac_i2c, true);
		wacom_enable_pdct_irq(wac_i2c, true);
		mutex_unlock(&wac_i2c->update_lock);

		wac_i2c->probe_done = true;
		return 0;
	}

	if (!bforced) {
		if (fw_ver_ic == wac_i2c->fw_ver_bin) {
			input_info(true, &client->dev, "pass fw update\n");
			wac_i2c->do_crc_check = true;
			/* need to check crc */
		} else if (fw_ver_ic > wac_i2c->fw_ver_bin) {
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

	wac_i2c->probe_done = true;

#ifdef CONFIG_SEC_FACTORY
	ret = wacom_check_ub(wac_i2c);
	if (!ret) {
		wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_GARAGE_AOP);
		input_info(true, &client->dev, "Change mode for garage scan\n");
	}
#endif

	return -1;
}

static void wacom_i2c_update_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
	    container_of(work, struct wacom_i2c, update_work);
	struct i2c_client *client = wac_i2c->client;
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

	wac_i2c->update_status = FW_UPDATE_RUNNING;

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
		wac_i2c->update_status = FW_UPDATE_FAIL;
		wac_i2c->fw_ver_ic = 0;
		goto end_fw_update;
	}

	ret = wacom_i2c_query(wac_i2c);
	if (ret < 0) {
		input_info(true, &client->dev, "failed to query to IC(%d)\n",
			   ret);
		wac_i2c->update_status = FW_UPDATE_FAIL;
		goto end_fw_update;
	}

	wac_i2c->update_status = FW_UPDATE_PASS;

end_fw_update:
	wacom_i2c_unload_fw(wac_i2c);
#ifndef CONFIG_SEC_FACTORY
	ret = wacom_open_test(wac_i2c);
	if (ret) {
		input_err(true, &client->dev, "open test check failed %d\n", ret);
		wacom_reset_hw(wac_i2c);
	}
#endif

	wake_unlock(&wac_i2c->fw_wakelock);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	if (wac_i2c->update_status == FW_UPDATE_FAIL) {
		input_err(true, &client->dev,
				"%s : failed to download FW & unload wacom driver\n", __func__);

		wacom_sec_remove(wac_i2c);

		devm_free_irq(&client->dev, wac_i2c->irq, wac_i2c);
		devm_free_irq(&client->dev, wac_i2c->irq_pdct, wac_i2c);

		input_unregister_device(wac_i2c->input_dev);
	}

	wac_i2c->probe_done = true;

#ifdef CONFIG_SEC_FACTORY
	ret = wacom_check_ub(wac_i2c);
	if (!ret) {
		wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_GARAGE_AOP);
		input_info(true, &client->dev, "Change mode for garage scan\n");
	}
#endif
}

#if defined(CONFIG_MUIC_SUPPORT_KEYBOARDDOCK)
static void wac_i2c_kbd_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c = container_of(work, struct wacom_i2c, kbd_work);
	char data;
	int ret;

	if (wac_i2c->kbd_conn_state == wac_i2c->kbd_cur_conn_state) {
		input_info(true, &wac_i2c->client->dev, "%s: already %sconnected\n",
			   __func__, wac_i2c->kbd_conn_state ? "" : "dis");
		return;
	}

	input_info(true, &wac_i2c->client->dev, "%s: %d\n", __func__,
		   wac_i2c->kbd_conn_state);

	if (!wac_i2c->power_enable) {
		input_err(true, &wac_i2c->client->dev,
				"%s: powered off\n", __func__);
		return;
	}

	if (wake_lock_active(&wac_i2c->fw_wakelock)) {
		input_err(true, &wac_i2c->client->dev,
			   "%s: fw update is running\n", __func__);
		return;
	}

	data = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send stop cmd %d\n",
			  __func__, ret);
		return;
	}
	wac_i2c->samplerate_state = false;
	msleep(50);

	if (wac_i2c->kbd_conn_state)
		data = COM_SPECIAL_COMPENSATION;
	else
		data = COM_NORMAL_COMPENSATION;

	ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send table swap cmd %d\n",
			  __func__, ret);
	}

	data = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send stop cmd %d\n",
			  __func__, ret);
		return;
	}

	msleep(50);

	data = COM_SAMPLERATE_133;
	ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send start cmd, %d\n",
			  __func__, ret);
		return;
	}
	wac_i2c->samplerate_state = true;

	input_info(true, &wac_i2c->client->dev, "%s: %s\n",
			__func__, wac_i2c->kbd_conn_state ? "on" : "off");
}

static int wacom_i2c_keyboard_notification_cb(struct notifier_block *nb,
					      unsigned long action, void *data)
{
	struct wacom_i2c *wac_i2c = container_of(nb, struct wacom_i2c, kbd_nb);
	int state = !!action;

	if (wac_i2c->kbd_conn_state == state)
		goto out;

	cancel_work_sync(&wac_i2c->kbd_work);
	wac_i2c->kbd_conn_state = state;
	input_info(true, &wac_i2c->client->dev, "%s: current %d change to %d\n",
		   __func__, wac_i2c->kbd_cur_conn_state, state);
	schedule_work(&wac_i2c->kbd_work);

out:
	return NOTIFY_DONE;
}
#endif

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
static void wacom_i2c_usb_typec_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c = container_of(work, struct wacom_i2c, typec_work);
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

	data[0] = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send stop cmd %d\n",
			  __func__, ret);
		return;
	}

	msleep(50);

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

static int wacom_i2c_usb_typec_notification_cb(struct notifier_block *nb,
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

	cancel_work(&wac_i2c->typec_work);
	wac_i2c->dp_connect_cmd = usb_typec_info.sub1;
	schedule_work(&wac_i2c->typec_work);
out:
	return 0;
}
#endif

static void wacom_i2c_nb_register_work(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c = container_of(work, struct wacom_i2c,
							nb_reg_work.work);
	u32 table_swap = wac_i2c->pdata->table_swap;
	u32 count = 0;
	int ret;

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
	if (table_swap == TABLE_SWAP_DEX_STATION)
		INIT_WORK(&wac_i2c->typec_work, wacom_i2c_usb_typec_work);
#endif
#ifdef CONFIG_MUIC_SUPPORT_KEYBOARDDOCK
	if (table_swap == TABLE_SWAP_KBD_COVER)
		INIT_WORK(&wac_i2c->kbd_work, wac_i2c_kbd_work);
#endif

	do {
#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
		if (table_swap == TABLE_SWAP_DEX_STATION) {
			static bool manager_flag = false;

			if (!manager_flag) {
				ret = manager_notifier_register(&wac_i2c->typec_nb,
								wacom_i2c_usb_typec_notification_cb,
								MANAGER_NOTIFY_CCIC_WACOM);
				if (!ret) {
					manager_flag = true;
					input_info(true, &wac_i2c->client->dev,
						  "%s: typec notifier register success\n",
						  __func__);
					break;
				}
			}
		}
#endif
#ifdef CONFIG_MUIC_SUPPORT_KEYBOARDDOCK
		if (table_swap == TABLE_SWAP_KBD_COVER) {
			static bool kbd_flag = false;

			if (!kbd_flag) {
				ret = keyboard_notifier_register(&wac_i2c->kbd_nb,
								 wacom_i2c_keyboard_notification_cb,
								 KEYBOARD_NOTIFY_DEV_WACOM);
				if (!ret) {
					kbd_flag = true;
					input_info(true, &wac_i2c->client->dev,
						  "%s: kbd notifier register success\n",
						  __func__);
					break;
				}
			}
		}
#endif
		count++;
		msleep(30);
	} while (count < 100);
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

#ifdef CONFIG_SEC_FACTORY
bool wacom_check_ub(struct wacom_i2c *wac_i2c)
{
	int lcdtype = 0;
#ifdef CONFIG_EXYNOS_DPU20
	int connected;
#endif

#ifdef CONFIG_DISPLAY_SAMSUNG
	lcdtype = get_lcd_attached("GET");
	if (lcdtype == 0xFFFFFF) {
		input_info(true, &wac_i2c->client->dev, "lcd is not attached\n");
		return false;
	}
#endif

#ifdef CONFIG_EXYNOS_DPU20
	connected = get_lcd_info("connected");
	if (connected < 0) {
		input_info(true, &wac_i2c->client->dev,
			   "Failed to get connection info\n");
		return false;
	}

	if (!connected) {
		input_info(true, &wac_i2c->client->dev, "lcd is not attached\n");
		return false;
	}

	lcdtype = get_lcd_info("id");
	if (lcdtype < 0) {
		input_info(true, &wac_i2c->client->dev, "Failed to get id info\n");
		return false;
	}
#endif

	input_info(true, &wac_i2c->client->dev, "%s: lcdtype 0x%08x\n",
		   __func__, lcdtype);

	return true;
}
#endif

#ifdef CONFIG_OF
static struct wacom_g5_platform_data *wacom_parse_dt(struct i2c_client *client)
{
	struct wacom_g5_platform_data *pdata;
	struct device *dev = &client->dev;
	struct device_node *np = dev->of_node;
	u32 tmp[5] = { 0, };
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
			  "failed to read trigger type of irq %d\n", ret);
		return ERR_PTR(-EINVAL);
	}
	pdata->irq_type = tmp[0];

	ret = of_property_read_u32(np, "wacom,pdct_type", tmp);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read trigger type of pdct %d\n", ret);
		return ERR_PTR(-EINVAL);
	}
	pdata->pdct_type = tmp[0];

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

	ret = of_property_read_u32_array(np, "wacom,max_tilt", tmp, 2);
	if (ret != -EINVAL) {
		if (ret) {
			input_err(true, dev,
				  "failed to read max x tilt %d\n", ret);
		}
		pdata->max_x_tilt = tmp[0];
		pdata->max_y_tilt = tmp[1];
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

	ret = of_property_read_u32(np, "wacom,module_ver", &pdata->module_ver);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read module_ver %d\n", ret);
		/* default setting to open test */
		pdata->module_ver = 1;
	}

	pdata->use_garage = of_property_read_bool(np, "wacom,use_garage");

	ret = of_property_read_u32(np, "wacom,table_swap", &pdata->table_swap);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read module_ver %d\n", ret);
		pdata->table_swap = 0;
	}

	pdata->use_vddio = of_property_read_bool(np, "vddio-supply");

	ret = of_property_read_u32(np, "wacom,bringup", &pdata->bringup);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read bringup %d\n", ret);
		/* default setting to open test */
		pdata->bringup = 0;
	}

	input_info(true, &client->dev,
		   "boot_addr: 0x%X, origin: (%d,%d), max_coords: (%d,%d), "
		   "max_pressure: %d, max_height: %d, max_tilt: (%d,%d) "
		   "invert: (%d,%d,%d), fw_path: %s, ic_type: %d, "
		   "table_swap:%d%s%s\n",
		   pdata->boot_addr, pdata->origin[0], pdata->origin[1],
		   pdata->max_x, pdata->max_y, pdata->max_pressure,
		   pdata->max_height, pdata->max_x_tilt, pdata->max_y_tilt,
		   pdata->x_invert, pdata->y_invert, pdata->xy_switch,
		   pdata->fw_path, pdata->ic_type, pdata->table_swap,
		   pdata->use_garage ? ", use garage" : "",
		   pdata->use_vddio ? ", control vddio" : "");

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
	wac_i2c->tsp_scan_mode = DISABLE_TSP_SCAN_BLCOK;
	wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
	wac_i2c->function_result = EPEN_EVENT_PEN_OUT;
	/* Consider about factory, it may be need to as default 1 */
#ifdef CONFIG_SEC_FACTORY
	wac_i2c->battery_saving_mode = true;
#else
	wac_i2c->battery_saving_mode = false;
#endif
	wac_i2c->reset_flag = false;
	wac_i2c->pm_suspend = false;
	wac_i2c->samplerate_state = true;
	wac_i2c->update_status = FW_UPDATE_RUNNING;

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
	wacom_i2c_set_input_values(wac_i2c, input);

	/*Initializing for semaphor */
	mutex_init(&wac_i2c->lock);
	mutex_init(&wac_i2c->update_lock);
	mutex_init(&wac_i2c->irq_lock);
	mutex_init(&wac_i2c->mode_lock);
	mutex_init(&wac_i2c->ble_charge_mode_lock);

	wake_lock_init(&wac_i2c->fw_wakelock, WAKE_LOCK_SUSPEND, "wacom");
	wake_lock_init(&wac_i2c->wakelock, WAKE_LOCK_SUSPEND, "wacom_wakelock");

	INIT_DELAYED_WORK(&wac_i2c->resume_work, wacom_i2c_resume_work);

	init_completion(&wac_i2c->resume_done);

	INIT_WORK(&wac_i2c->update_work, wacom_i2c_update_work);
	INIT_DELAYED_WORK(&wac_i2c->work_print_info, wacom_print_info_work);

	ret = input_register_device(input);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to register input device\n");
		/* managed input devices need not be explicitly unregistred or freed. */
		goto err_register_input_dev;

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
					wacom_interrupt_pdct, IRQF_ONESHOT |
					pdata->pdct_type,
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
		INIT_DELAYED_WORK(&wac_i2c->nb_reg_work, wacom_i2c_nb_register_work);
		schedule_delayed_work(&wac_i2c->nb_reg_work, msecs_to_jiffies(500));
	}

	input_info(true, &client->dev, "probe done\n");
	input_log_fix();

	return 0;

err_sec_init:
	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
err_request_pdct_irq:
err_request_irq:
err_register_input_dev:
	cancel_delayed_work_sync(&wac_i2c->resume_work);
	wake_lock_destroy(&wac_i2c->fw_wakelock);
	wake_lock_destroy(&wac_i2c->wakelock);

	mutex_destroy(&wac_i2c->irq_lock);
	mutex_destroy(&wac_i2c->update_lock);
	mutex_destroy(&wac_i2c->lock);
	mutex_destroy(&wac_i2c->mode_lock);
	mutex_destroy(&wac_i2c->ble_charge_mode_lock);

	i2c_unregister_device(wac_i2c->client_boot);

	input_err(true, &client->dev, "failed to probe\n");
	input_log_fix();

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

	wac_i2c->probe_done = false;

	input_info(true, &wac_i2c->client->dev, "%s called!\n", __func__);

	if (wac_i2c->pdata->table_swap) {
#ifdef CONFIG_MUIC_SUPPORT_KEYBOARDDOCK
		if (wac_i2c->pdata->table_swap == TABLE_SWAP_KBD_COVER)
			keyboard_notifier_unregister(&wac_i2c->kbd_nb);
#endif

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
		if (wac_i2c->pdata->table_swap == TABLE_SWAP_DEX_STATION)
			manager_notifier_unregister(&wac_i2c->typec_nb);
#endif
	}

	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
	cancel_work_sync(&wac_i2c->update_work);
	cancel_delayed_work_sync(&wac_i2c->work_print_info);

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wacom_power(wac_i2c, false);

	input_info(true, &wac_i2c->client->dev, "%s Done\n", __func__);
}

static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	wac_i2c->probe_done = false;

	input_info(true, &wac_i2c->client->dev, "%s called!\n", __func__);

	if (wac_i2c->pdata->table_swap) {
#ifdef CONFIG_MUIC_SUPPORT_KEYBOARDDOCK
		if (wac_i2c->pdata->table_swap == TABLE_SWAP_KBD_COVER)
			keyboard_notifier_unregister(&wac_i2c->kbd_nb);
#endif

#ifdef CONFIG_USB_TYPEC_MANAGER_NOTIFIER
		if (wac_i2c->pdata->table_swap == TABLE_SWAP_DEX_STATION)
			manager_notifier_unregister(&wac_i2c->typec_nb);
#endif
	}

	cancel_delayed_work_sync(&wac_i2c->pen_insert_dwork);
	cancel_work_sync(&wac_i2c->update_work);
	cancel_delayed_work_sync(&wac_i2c->work_print_info);

	device_init_wakeup(&client->dev, false);

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wacom_power(wac_i2c, false);

	wake_lock_destroy(&wac_i2c->fw_wakelock);
	wake_lock_destroy(&wac_i2c->wakelock);
	mutex_destroy(&wac_i2c->irq_lock);
	mutex_destroy(&wac_i2c->update_lock);
	mutex_destroy(&wac_i2c->lock);
	mutex_destroy(&wac_i2c->mode_lock);
	mutex_destroy(&wac_i2c->ble_charge_mode_lock);

	wacom_sec_remove(wac_i2c);

	i2c_unregister_device(wac_i2c->client_boot);

	input_info(true, &wac_i2c->client->dev, "%s Done\n", __func__);

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
