/*
 *  wacom_i2c_func.c - Wacom G5 Digitizer Controller (I2C bus)
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
#include <linux/delay.h>

#if defined(CONFIG_DRV_SAMSUNG)
#include <linux/sec_class.h>
#else
extern struct class *sec_class;
#endif

#include "wacom.h"

static void get_chip_name(void *device_data);
int calibration_trx_data(struct wacom_i2c *wac_i2c);
void calculate_ratio(struct wacom_i2c *wac_i2c);
void make_decision(struct wacom_i2c *wac_i2c, u16* arrResult);
void print_elec_data(struct wacom_i2c *wac_i2c);
void print_trx_data(struct wacom_i2c *wac_i2c);
void print_cal_trx_data(struct wacom_i2c *wac_i2c);
void print_ratio_trx_data(struct wacom_i2c *wac_i2c);
void print_difference_ratio_trx_data(struct wacom_i2c *wac_i2c);

static void run_elec_test(void *device_data);
static void ready_elec_test(void *device_data);
static void fw_update(void *device_data);
static void not_support_cmd(void *device_data);

static struct sec_cmd sec_cmds[] = {
	{SEC_CMD("get_chip_name", get_chip_name),},
	{SEC_CMD("run_elec_test", run_elec_test),},
	{SEC_CMD("ready_elec_test", ready_elec_test),},
	{SEC_CMD("fw_update", fw_update),},
	{SEC_CMD("not_support_cmd", not_support_cmd),},
};

static inline int power(int num)
{
	int i, ret;

	for (i = 0, ret = 1; i < num; i++)
		ret = ret * 10;

	return ret;
}

#ifdef CONFIG_SEC_FACTORY
static ssize_t epen_fac_select_firmware_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	u8 fw_update_way = FW_NONE;
	int ret = 0;

	input_info(true, &client->dev, "%s\n", __func__);

	switch (*buf) {
	case 'k':
	case 'K':
		fw_update_way = FW_BUILT_IN;
		break;
	case 't':
	case 'T':
		fw_update_way = FW_FACTORY_GARAGE;
		break;
	case 'u':
	case 'U':
		fw_update_way = FW_FACTORY_UNIT;
		break;
	default:
		input_err(true, &client->dev, "wrong parameter\n");
		return count;
	}

	ret = wacom_i2c_load_fw(wac_i2c, fw_update_way);
	if (ret < 0) {
		input_info(true, &client->dev, "failed to load fw data\n");
		goto out;
	}

	wacom_i2c_unload_fw(wac_i2c);
out:
	return count;
}
#endif

static ssize_t epen_firm_update_status_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int status = wac_i2c->update_status;
	int ret = 0;

	input_info(true, &client->dev, "%s:(%d)\n", __func__, status);

	if (status == FW_UPDATE_PASS)
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	else if (status == FW_UPDATE_RUNNING)
		ret = snprintf(buf, PAGE_SIZE, "DOWNLOADING\n");
	else if (status == FW_UPDATE_FAIL)
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	else
		ret = 0;

	return ret;
}

static ssize_t epen_firm_version_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: 0x%x|0x%X\n", __func__,
		   wac_i2c->fw_ver_ic, wac_i2c->fw_ver_bin);

	return snprintf(buf, PAGE_SIZE, "%04X\t%04X\n",
			wac_i2c->fw_ver_ic, wac_i2c->fw_ver_bin);
}

static ssize_t epen_firmware_update_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	u8 fw_update_way = FW_NONE;

	input_info(true, &client->dev, "%s\n", __func__);

	switch (*buf) {
	case 'i':
	case 'I':
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		fw_update_way = FW_IN_SDCARD;
#else
		fw_update_way = FW_IN_SDCARD_SIGNED;
#endif
		break;
	case 'k':
	case 'K':
		fw_update_way = FW_BUILT_IN;
		break;
#ifdef CONFIG_SEC_FACTORY
	case 't':
	case 'T':
		fw_update_way = FW_FACTORY_GARAGE;
		break;
	case 'u':
	case 'U':
		fw_update_way = FW_FACTORY_UNIT;
		break;
#endif
	default:
		input_err(true, &client->dev, "wrong parameter\n");
		return count;
	}

	wacom_fw_update(wac_i2c, fw_update_way, true);

	return count;
}

static ssize_t epen_firm_version_of_ic_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);

	input_info(true, &wac_i2c->client->dev, "%s: %X\n", __func__, wac_i2c->fw_ver_ic);

	return snprintf(buf, PAGE_SIZE, "%04X", wac_i2c->fw_ver_ic);
}

static ssize_t epen_firm_version_of_bin_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);

	input_info(true, &wac_i2c->client->dev, "%s: %X\n", __func__, wac_i2c->fw_ver_bin);

	return snprintf(buf, PAGE_SIZE, "%04X", wac_i2c->fw_ver_bin);
}


static ssize_t epen_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);
	if (!val)
		goto out;

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
	wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;

	/* Reset IC */
	wacom_reset_hw(wac_i2c);
	/* I2C Test */
	wacom_i2c_query(wac_i2c);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	input_info(true, &client->dev,
		   "%s: result %d\n", __func__, wac_i2c->query_status);
	return count;

out:
	input_err(true, &client->dev, "%s: invalid value %d\n", __func__, val);

	return count;
}

static ssize_t epen_reset_result_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int ret = 0;

	if (wac_i2c->query_status) {
		input_info(true, &client->dev, "%s: PASS\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	} else {
		input_err(true, &client->dev, "%s: FAIL\n", __func__);
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	}

	return ret;
}

static ssize_t epen_checksum_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);
	if (!val)
		goto out;

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	wacom_checksum(wac_i2c);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	input_info(true, &client->dev,
		   "%s: result %d\n", __func__, wac_i2c->checksum_result);

	return count;

out:
	input_err(true, &client->dev, "%s: invalid value %d\n", __func__, val);

	return count;
}

static ssize_t epen_checksum_result_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int ret;

	if (wac_i2c->checksum_result) {
		input_info(true, &client->dev, "checksum, PASS\n");
		ret = snprintf(buf, PAGE_SIZE, "PASS\n");
	} else {
		input_err(true, &client->dev, "checksum, FAIL\n");
		ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
	}

	return ret;
}

int wacom_open_test(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	u8 cmd = 0;
	u8 buf[COM_COORD_NUM + 1] = { 0, };
	int ret = 0, retry = 0, retval = 0;

	wac_i2c->connection_check = false;
	wac_i2c->fail_channel = 0;
	wac_i2c->min_adc_val = 0;
	wac_i2c->error_cal = 0;
	wac_i2c->min_cal_val = 0;

	/* change normal mode for open test */
	if (wac_i2c->pdata->use_garage) {
		mutex_lock(&wac_i2c->mode_lock);
		wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_NONE);
		// prevent I2C fail
		msleep(300);
		mutex_unlock(&wac_i2c->mode_lock);
	}

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev, "failed to send data(%02x)\n", cmd);
		retval = -1;
		goto err;
	}

	wac_i2c->samplerate_state = false;
	msleep(50);

	retry = 5;
	while (retry--) {
		/* clear garbage irq */
		if (wacom_get_irq_state(wac_i2c) > 0) {
			u8 data[COM_COORD_NUM] = { 0, };

			ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM,
						WACOM_I2C_MODE_NORMAL);
			if (ret != COM_COORD_NUM) {
				input_err(true, &client->dev,
					  "%s: failed to recv\n", __func__);
			} else {
				input_info(true, &client->dev,
					  "%s: ignore garbage data(%02x %02x)\n",
					  __func__, data[0], data[1]);
			}
		} else {
			break;
		}

		usleep_range(4500, 5500);
	}

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	cmd = COM_OPEN_CHECK_START;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev, "failed to send data(%02x)\n", cmd);
		retval = -1;
	
		wacom_enable_irq(wac_i2c, true);
		wacom_enable_pdct_irq(wac_i2c, true);

		goto err;
	}

	msleep(2000);

	retry = 10;
	cmd = COM_OPEN_CHECK_STATUS;
	do {
		input_info(true, &client->dev, "read status, retry %d\n", retry);
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev,
				  "failed to send data(%02x)\n", cmd);
			usleep_range(4500, 5500);
			continue;
		}

		usleep_range(500, 500);

		ret = wacom_i2c_recv(wac_i2c, buf, COM_COORD_NUM, WACOM_I2C_MODE_NORMAL);
		if (ret != COM_COORD_NUM) {
			input_err(true, &client->dev, "failed to recv\n");
			usleep_range(4500, 5500);
			continue;
		}

		if (buf[0] != 0x0E && buf[1] != 0x01) {
			input_err(true, &client->dev,
				  "invalid packet(%02x %02x %02x)\n",
				  buf[0], buf[1], buf[3]);
			msleep(50);
			continue;
		}
		/*
		 *      status value
		 *      0 : data is not ready
		 *      1 : PASS
		 *      2 : Fail (coil function error)
		 *      3 : Fail (All coil function error)
		 */
		if (buf[2] == 1) {
			input_info(true, &client->dev, "Open check Pass\n");
			break;
		}

		msleep(50);
	} while (retry--);

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	if (ret == COM_COORD_NUM) {
		if (wac_i2c->pdata->module_ver == 0x2)
			wac_i2c->connection_check = ((buf[2] == 1) && !buf[6]);
		else
			wac_i2c->connection_check = (buf[2] == 1);
	} else {
		wac_i2c->connection_check = false;
		retval = -1;
		goto err;
	}

	wac_i2c->fail_channel = buf[3];
	wac_i2c->min_adc_val = buf[4] << 8 | buf[5];
	wac_i2c->error_cal = buf[6];
	wac_i2c->min_cal_val = buf[7] << 8 | buf[8];

	input_info(true, &client->dev,
		   "%s: %s buf[3]:%d, buf[4]:%d, buf[5]:%d, buf[6]:%d, buf[7]:%d, buf[8]:%d\n",
		   __func__, wac_i2c->connection_check ? "Pass" : "Fail",
		   buf[3], buf[4], buf[5], buf[6], buf[7], buf[8]);

	if (!wac_i2c->connection_check)
		retval = -1;

err:
	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev, "failed to send data(%02x)\n", cmd);
		retval = -2;
		goto out;
	}

	cmd = COM_SAMPLERATE_START;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev, "failed to send data(%02x)\n", cmd);
		retval = -2;
		goto out;
	}

	wac_i2c->samplerate_state = true;

out:
	/* recovery wacom mode */
	if (wac_i2c->pdata->use_garage)
		wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);

	return retval;
}

static ssize_t epen_connection_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int retry = 2;
	int ret;

#ifdef CONFIG_SEC_FACTORY
	ret = wacom_check_ub(wac_i2c);
	if (!ret) {
		input_info(true, &client->dev, "%s: digitizer is not attached\n", __func__);
		goto out;
	}
#endif
	mutex_lock(&wac_i2c->lock);

	input_info(true, &client->dev, "%s\n", __func__);

	wac_i2c->is_open_test = true;

	if (wac_i2c->is_tsp_block) {
		input_info(true, &wac_i2c->client->dev, "%s: full scan OUT\n", __func__);
		wac_i2c->tsp_scan_mode = set_scan_mode(DISABLE_TSP_SCAN_BLCOK);
		wac_i2c->is_tsp_block = false;
	}

	while (retry--) {
		ret = wacom_open_test(wac_i2c);
		if (!ret)
			break;

		input_err(true, &client->dev, "failed. retry %d\n", retry);

		wacom_power(wac_i2c, false);
		wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;
		wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
 		/* recommended delay in spec */
 		msleep(100);
 		wacom_power(wac_i2c, true);

		if (ret == -1) {
			msleep(150);
			continue;
		} else if (ret == -2) {
			break;
		}
	}

	if (!wac_i2c->samplerate_state) {
		char cmd = COM_SAMPLERATE_START;

		input_info(true, &client->dev, "%s: samplerate state is %d, need to recovery\n",
			   __func__, wac_i2c->samplerate_state);

		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev,
				  "failed to sned start cmd %d\n", ret);
		} else {
			wac_i2c->samplerate_state = true;
		}
	}

	mutex_unlock(&wac_i2c->lock);

	input_info(true, &client->dev,
		   "connection_check : %d\n", wac_i2c->connection_check);

	wac_i2c->is_open_test = false;

#ifdef CONFIG_SEC_FACTORY
out:
#endif
	if (wac_i2c->pdata->module_ver == 0x2) {
		return snprintf(buf, PAGE_SIZE, "%s %d %d %d %s %d\n",
				wac_i2c->connection_check ? "OK" : "NG",
				wac_i2c->pdata->module_ver, wac_i2c->fail_channel,
				wac_i2c->min_adc_val, wac_i2c->error_cal ? "NG" : "OK",
				wac_i2c->min_cal_val);
	} else {
		return snprintf(buf, PAGE_SIZE, "%s %d %d %d\n",
				wac_i2c->connection_check ? "OK" : "NG",
				wac_i2c->pdata->module_ver, wac_i2c->fail_channel,
				wac_i2c->min_adc_val);
	}
}

static ssize_t epen_saving_mode_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	static u64 call_count;
	int val;

	call_count++;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->battery_saving_mode = !(!val);

	input_info(true, &client->dev, "%s: ps %s & pen %s [%lu]\n",
		   __func__, wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_result & EPEN_EVENT_PEN_OUT) ? "out" : "in",
		   call_count);

	if (!wac_i2c->power_enable && wac_i2c->battery_saving_mode) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: already power off, save & return\n", __func__);
		return count;
	}

	if (!(wac_i2c->function_result & EPEN_EVENT_PEN_OUT)) {
		wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);

		if (wac_i2c->battery_saving_mode)
			forced_release_fullscan(wac_i2c);
	}

	return count;
}

static ssize_t epen_insert_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	int pen_state = (wac_i2c->function_result & EPEN_EVENT_PEN_OUT);

	input_info(true, &wac_i2c->client->dev, "%s : pen is %s\n", __func__,
		   pen_state ? "OUT" : "IN");

	return snprintf(buf, PAGE_SIZE, "%d\n", pen_state ? 0 : 1);
}

static ssize_t epen_screen_off_memo_enable_store(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support screen off memo mode(%d) in Factory Bin\n",
		   __func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wac_i2c->function_set |= EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO;
	else
		wac_i2c->function_set &= (~EPEN_SETMODE_AOP_OPTION_SCREENOFFMEMO);

	input_info(true, &client->dev,
		   "%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
		   wac_i2c->function_set, wac_i2c->function_result);

	if (!wac_i2c->screen_on)
		wacom_select_survey_mode(wac_i2c, false);

	return count;
}

static ssize_t epen_aod_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support aod mode(%d) in Factory Bin\n",
		   __func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wac_i2c->function_set |= EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF;
	else
		wac_i2c->function_set &= ~EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON;

	input_info(true, &client->dev,
		   "%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
		   wac_i2c->function_set, wac_i2c->function_result);

	if (!wac_i2c->screen_on)
		wacom_select_survey_mode(wac_i2c, false);

	return count;
}

static ssize_t epen_aod_lcd_onoff_status_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support aod mode(%d) in Factory Bin\n",
		   __func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wac_i2c->function_set |= EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS;
	else
		wac_i2c->function_set &= ~EPEN_SETMODE_AOP_OPTION_AOD_LCD_STATUS;

	if (!(wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOD) || wac_i2c->screen_on)
		goto out;

	if (wac_i2c->survey_mode == EPEN_SURVEY_MODE_GARAGE_AOP) {
		if ((wac_i2c->function_set & EPEN_SETMODE_AOP_OPTION_AOD_LCD_ON)
				== EPEN_SETMODE_AOP_OPTION_AOD_LCD_OFF) {
			forced_release_fullscan(wac_i2c);
		}

		mutex_lock(&wac_i2c->mode_lock);
		wacom_i2c_set_survey_mode(wac_i2c, wac_i2c->survey_mode);
		mutex_unlock(&wac_i2c->mode_lock);
	}

out:
	input_info(true, &client->dev, "%s: screen %s, survey mode:0x%x, set:0x%x\n",
		   __func__, wac_i2c->screen_on ? "on" : "off",
		   wac_i2c->survey_mode, wac_i2c->function_set);

	return count;
}

static ssize_t epen_aot_enable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

#ifdef CONFIG_SEC_FACTORY
	input_info(true, &client->dev,
		   "%s : Not support aot mode(%d) in Factory Bin\n",
		   __func__, val);

	return count;
#endif

	val = !(!val);

	if (val)
		wac_i2c->function_set |= EPEN_SETMODE_AOP_OPTION_AOT;
	else
		wac_i2c->function_set &= (~EPEN_SETMODE_AOP_OPTION_AOT);

	input_info(true, &client->dev,
		   "%s: ps %s aop(%d) set(0x%x) ret(0x%x)\n", __func__,
		   wac_i2c->battery_saving_mode ? "on" : "off",
		   (wac_i2c->function_set & EPEN_SETMODE_AOP) ? 1 : 0,
		   wac_i2c->function_set, wac_i2c->function_result);

	if (!wac_i2c->screen_on)
		wacom_select_survey_mode(wac_i2c, false);

	return count;
}

static ssize_t epen_wcharging_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %s\n", __func__,
		   !wac_i2c->wcharging_mode ? "NORMAL" : "LOWSENSE");

	return snprintf(buf, PAGE_SIZE, "%s\n",
			!wac_i2c->wcharging_mode ? "NORMAL" : "LOWSENSE");
}

static ssize_t epen_wcharging_mode_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->wcharging_mode = retval;

	input_info(true, &client->dev, "%s: %d\n", __func__,
		   wac_i2c->wcharging_mode);

	if (!wac_i2c->power_enable) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: power off, save & return\n", __func__);
		return count;
	}

	retval = wacom_i2c_set_sense_mode(wac_i2c);
	if (retval < 0) {
		input_err(true, &client->dev,
			  "%s: do not set sensitivity mode, %d\n", __func__,
			  retval);
	}

	return count;

}

char ble_charge_command[] = {
	COM_BLE_C_DISABLE, /* Disable charge (charging mode enable) */
	COM_BLE_C_ENABLE, /* Enable charge (charging mode disable) */
	COM_BLE_C_RESET, /* Reset (make reset pattern + 1m charge) */
	COM_BLE_C_START, /* Start (make start patter + 1m charge) */
	COM_BLE_C_KEEP_ON, /* Keep on charge (you need send this cmd within a minute after Start cmd) */
	COM_BLE_C_KEEP_Off, /* Keep off charge */
	COM_BLE_C_M_RETURN, /* Request charging mode */
	COM_RESET_DSP, /* DSP reset */
	COM_BLE_C_FULL, /* Full charge (depend on fw) */
};

int wacom_ble_charge_mode(struct wacom_i2c *wac_i2c, int mode)
{
	struct i2c_client *client = wac_i2c->client;
	int ret = 0;
	char data;
	struct timeval current_time;

	if (wac_i2c->is_open_test) {
		input_err(true, &client->dev, "%s: other cmd is working\n",
			  __func__);
		return -EPERM;
	}

	if (wac_i2c->ble_block_flag && (mode != EPEN_BLE_C_DIABLE)) {
		input_err(true, &client->dev, "%s: operation not permitted\n",
			  __func__);
		return -EPERM;
	}

	if (mode >= EPEN_BLE_C_MAX || mode < EPEN_BLE_C_DIABLE) {
		input_err(true, &client->dev, "%s: wrong mode, %d\n",
			  __func__,mode);
		return -EINVAL;
	}

	/* need to check mode */
	if (ble_charge_command[mode] == COM_BLE_C_ENABLE
			|| ble_charge_command[mode] == COM_BLE_C_RESET
			|| ble_charge_command[mode] == COM_BLE_C_START) {
		do_gettimeofday(&current_time);
		wac_i2c->chg_time_stamp = (int)(current_time.tv_sec);
		input_err(true, &client->dev, "%s: chg_time_stamp(%d)\n",
			  __func__, wac_i2c->chg_time_stamp);
	} else {
		wac_i2c->chg_time_stamp = 0;
	}

	if (mode == EPEN_BLE_C_DSPX) {
		/* add abnormal ble reset case (while pen in status, do not ble charge) */
		mutex_lock(&wac_i2c->mode_lock);
		ret = wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_NONE);
		mutex_unlock(&wac_i2c->mode_lock);
		if (ret)
			return ret;

		msleep(250);

		data = ble_charge_command[mode];
		ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev, "%s: failed to send data(%02x %d)\n",
				  __func__, data, ret);
			wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);
			return ret;
		}

		msleep(130);

		wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);
	} else {
		data = ble_charge_command[mode];
		ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev, "%s: failed to send data(%02x %d)\n",
				  __func__, data, ret);
			return ret;
		}

		switch (data) {
		case COM_BLE_C_RESET:
		case COM_BLE_C_START:
		case COM_BLE_C_KEEP_ON:
		case COM_BLE_C_FULL:
			wac_i2c->is_mode_change = true;
			break;
		}
	}

	input_info(true, &client->dev, "%s: now %02X, prev %02X\n", __func__, data,
		   wac_i2c->ble_mode ? wac_i2c->ble_mode : 0);

	wac_i2c->ble_mode = ble_charge_command[mode];

	return 0;
}

static ssize_t epen_ble_charging_mode_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	u8 buff[COM_COORD_NUM + 1] = { 0, };
	char data;
	int retry = RETRY_COUNT;
	int ret;

	if (wac_i2c->is_open_test) {
		input_err(true, &client->dev, "%s: other cmd is working\n",
			  __func__);

		ret = snprintf(buf, PAGE_SIZE, "NG\n");
		return ret;
	}

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	do {
		input_info(true, &client->dev,
			   "read status, retry %d\n", retry);

		data = COM_BLE_C_M_RETURN;
		ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev, "%s: failed to send data(%02x %d)\n",
				  __func__, data, ret);
			usleep_range(4500, 5500);

			continue;
		}

		ret = wacom_i2c_recv(wac_i2c, buff, COM_COORD_NUM,
						WACOM_I2C_MODE_NORMAL);
		if (ret != COM_COORD_NUM) {
			input_err(true, &client->dev,
				  "%s: failed to recv data(%d)\n", __func__, ret);
			usleep_range(4500, 5500);

			continue;
		}

		input_info(true, &client->dev,
			   "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",
			   buff[0], buff[1], buff[2], buff[3], buff[4], buff[5],
			   buff[6], buff[7], buff[8], buff[9], buff[10], buff[11],
			   buff[12], buff[13], buff[14], buff[15]);

		if ((((buff[0] & 0x0F) == NOTI_PACKET) && (buff[1] == OOK_PACKET)) ||
		    (((buff[0] & 0x0F) == REPLY_PACKET) && (buff[1] == GARAGE_CHARGE_PACKET))) {
			if (!(buff[4] & 80)) {
				break;
			} else {
				input_err(true, &client->dev, "OOK fail %02x\n", buff[4]);
			}
		}

		usleep_range(4500, 5500);
	} while (--retry);

	if (!retry) {
		ret = snprintf(buf, PAGE_SIZE, "NG\n");
		goto out;
	}

	switch (buff[2] & 0x0F) {
	case BLE_C_AFTER_START: case BLE_C_AFTER_RESET:
	case BLE_C_ON_KEEP_1: case BLE_C_ON_KEEP_2:
	case BLE_C_FULL:
		ret = snprintf(buf, PAGE_SIZE, "CHARGE\n");
		break;
	case BLE_C_OFF:
	case BLE_C_OFF_KEEP_1: case BLE_C_OFF_KEEP_2:
		ret = snprintf(buf, PAGE_SIZE, "DISCHARGE\n");
		break;
	default:
		input_info(true, &wac_i2c->client->dev, "unknow status: %x\n",
			   buff[2] & 0x0F);

		ret = snprintf(buf, PAGE_SIZE, "NG\n");
		break;
	}

out:
	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	input_info(true, &wac_i2c->client->dev, "%s: %s", __func__, buf);

	return ret;
}

static ssize_t epen_ble_charging_mode_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	if (kstrtoint(buf, 0, &retval)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	if (!wac_i2c->power_enable) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: power off return\n", __func__);
		return count;
	}

	mutex_lock(&wac_i2c->ble_charge_mode_lock);
	wacom_ble_charge_mode(wac_i2c, retval);
	mutex_unlock(&wac_i2c->ble_charge_mode_lock);

	return count;
}

static ssize_t epen_keyboard_mode_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->keyboard_cover_mode = !(!val);

	input_info(true, &client->dev, "%s: %d\n", __func__, wac_i2c->keyboard_cover_mode);

	return count;
}

#ifdef CONFIG_SEC_FACTORY
/* epen_disable_mode : Use in Factory mode for test test
 * 0 : wacom ic on
 * 1 : wacom ic off
 */
void epen_disable_mode(int mode)
{
	struct wacom_i2c *wac_i2c = wacom_get_drv_data(NULL);
	struct i2c_client *client = wac_i2c->client;

	if (wac_i2c->epen_blocked == mode){
		input_info(true, &client->dev, "%s: duplicate call %d!\n", __func__, mode);
		return;
	}

	if (mode) {
		input_info(true, &client->dev, "%s: power off\n", __func__);
		wac_i2c->epen_blocked = mode;

		wacom_enable_irq(wac_i2c, false);
		wacom_enable_pdct_irq(wac_i2c, false);
		wacom_power(wac_i2c, false);

		wac_i2c->survey_mode = EPEN_SURVEY_MODE_NONE;
		wac_i2c->function_result &= ~EPEN_EVENT_SURVEY;

		wac_i2c->tsp_scan_mode = set_scan_mode(DISABLE_TSP_SCAN_BLCOK);
		wac_i2c->is_tsp_block = false;
		forced_release(wac_i2c);
	} else {
		input_info(true, &client->dev, "%s: power on\n", __func__);

		wacom_power(wac_i2c, true);
		msleep(500);

		wacom_enable_irq(wac_i2c, true);
		wacom_enable_pdct_irq(wac_i2c, true);
		wac_i2c->epen_blocked = mode;
	}
	input_info(true, &client->dev, "%s: done %d!\n", __func__, mode);
}
#else
/* epen_disable_mode : Use in Secure mode(TUI)
 * 0 : enable wacom
 * 1 : disable wacom
 */
void epen_disable_mode(int mode)
{
	struct wacom_i2c *wac_i2c = wacom_get_drv_data(NULL);
	struct i2c_client *client = wac_i2c->client;
	static int depth;

	if (mode) {
		if (!depth++)
			goto out;
	} else {
		if (!(--depth))
			goto out;

		if (depth < 0)
			depth = 0;
	}

	input_info(true, &client->dev, "%s: %s(%d)!\n",
					__func__, mode ? "on" : "off", depth);

	return;

out:
	wac_i2c->epen_blocked = mode;

	if (!wac_i2c->power_enable && wac_i2c->epen_blocked) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: already power off, return\n", __func__);
		return;
	}

	wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);

	if (wac_i2c->epen_blocked)
		forced_release_fullscan(wac_i2c);

	input_info(true, &client->dev, "%s: %s(%d)!\n",
					__func__, mode ? "on" : "off", depth);
}
#endif
EXPORT_SYMBOL(epen_disable_mode);

static ssize_t epen_disable_mode_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val = 0;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);

	input_info(true, &client->dev, "%s: %d\n", __func__, val);

	epen_disable_mode(val);

	return count;
}

static ssize_t epen_pen_out_count_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %u\n", __func__,
		   wac_i2c->pen_out_count);

	return snprintf(buf, PAGE_SIZE, "%u", wac_i2c->pen_out_count);
}

static ssize_t epen_clear_pen_out_count_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	if (buf[0] != 'c') {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->pen_out_count = 0;

	input_info(true, &client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t epen_abnormal_reset_count_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %u\n", __func__,
		   wac_i2c->abnormal_reset_count);

	return snprintf(buf, PAGE_SIZE, "%u", wac_i2c->abnormal_reset_count);
}

static ssize_t epen_clear_abnormal_reset_count_store(struct device *dev,
						     struct device_attribute *attr,
						     const char *buf,
						     size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	if (buf[0] != 'c') {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->abnormal_reset_count = 0;

	input_info(true, &client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t epen_i2c_fail_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: %u\n", __func__,
		   wac_i2c->i2c_fail_count);

	return snprintf(buf, PAGE_SIZE, "%u", wac_i2c->i2c_fail_count);
}

static ssize_t epen_clear_i2c_fail_count_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	if (buf[0] != 'c') {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	wac_i2c->i2c_fail_count = 0;

	input_info(true, &client->dev, "%s: clear\n", __func__);

	return count;
}

static ssize_t epen_connection_check_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: SDCONN:%d,SECCNT:%d,SADCVAL:%d,SECLVL:%d,SCALLVL:%d\n",
			__func__, wac_i2c->connection_check,
			wac_i2c->fail_channel, wac_i2c->min_adc_val,
			wac_i2c->error_cal, wac_i2c->min_cal_val);

	return snprintf(buf, PAGE_SIZE,
			"\"SDCONN\":\"%d\",\"SECCNT\":\"%d\",\"SADCVAL\":\"%d\",\"SECLVL\":\"%d\",\"SCALLVL\":\"%d\"",
			wac_i2c->connection_check, wac_i2c->fail_channel,
			wac_i2c->min_adc_val, wac_i2c->error_cal,
			wac_i2c->min_cal_val);
}

#ifdef CONFIG_SEC_FACTORY
static ssize_t epen_fac_garage_mode_enable(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int val;
	u8 cmd;
	int ret;

	if (kstrtoint(buf, 0, &val)) {
		input_err(true, &client->dev, "%s: failed to get param\n",
			  __func__);
		return count;
	}

	val = !(!val);
	if (!val) {
		wac_i2c->fac_garage_mode = 0;
	} else {
		/* change normal mode for garage monitoring */
		mutex_lock(&wac_i2c->mode_lock);
		wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_NONE);
		mutex_unlock(&wac_i2c->mode_lock);
	}

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret < 0) {
		input_err(true, &client->dev, "failed to send stop cmd %d\n",
			  ret);

		msleep(50);

		wac_i2c->fac_garage_mode = 0;

		goto out;
	}

	wac_i2c->samplerate_state = false;

	if (val) {
		msleep(50);

		wac_i2c->fac_garage_mode = 1;
	} else {
		cmd = COM_SAMPLERATE_START;
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret < 0) {
			input_err(true, &client->dev,
				  "failed to sned start cmd %d\n",
				  ret);
			goto out;
		}

		wac_i2c->samplerate_state = true;

		/* recovery wacom mode */
		wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);
	}

	input_info(true, &client->dev, "%s: %d\n", __func__, val);

	return count;

out:
	/* recovery wacom mode */
	wacom_select_survey_mode(wac_i2c, wac_i2c->screen_on);

	return count;
}

static ssize_t epen_fac_garage_mode_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;

	input_info(true, &client->dev, "%s: garage mode %s\n", __func__,
		   wac_i2c->fac_garage_mode ? "IN" : "OUT");

	return snprintf(buf, PAGE_SIZE, "garage mode %s",
			wac_i2c->fac_garage_mode ? "IN" : "OUT");
}

static ssize_t epen_fac_garage_rawdata_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	char data[17] = { 0, };
	int ret, retry = 1;
	u8 cmd;

	if (!wac_i2c->fac_garage_mode) {
		input_err(true, &client->dev, "not in factory garage mode\n");
		return snprintf(buf, PAGE_SIZE, "NG");
	}

get_garage_retry:

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	cmd = COM_REQUESTGARAGEDATA;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret < 0) {
		input_err(true, &client->dev,
			  "failed to send request garage data command %d\n",
			  ret);
		msleep(30);

		goto out;
	}

	msleep(30);

	ret = wacom_i2c_recv(wac_i2c, data, sizeof(data), WACOM_I2C_MODE_NORMAL);
	if (ret < 0) {
		input_err(true, &client->dev,
			  "failed to read garage raw data, %d\n", ret);

		wac_i2c->garage_freq0 = wac_i2c->garage_freq1 = 0;
		wac_i2c->garage_gain0 = wac_i2c->garage_gain1 = 0;

		goto out;
	}

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	input_info(true, &client->dev, "%x %x %x %x %x %x %x %x %x %x\n",
		   data[2], data[3], data[4], data[5], data[6], data[7],
		   data[8], data[9], data[10], data[11]);

	wac_i2c->garage_gain0 = data[6];
	wac_i2c->garage_freq0 = ((u16)data[7] << 8) + data[8];

	wac_i2c->garage_gain1 = data[9];
	wac_i2c->garage_freq1 = ((u16)data[10] << 8) + data[11];

	if (wac_i2c->garage_freq0 == 0 && retry > 0) {
		retry--;
		goto get_garage_retry;
	}

	input_info(true, &client->dev, "%s: %d, %d, %d, %d\n", __func__,
		   wac_i2c->garage_gain0, wac_i2c->garage_freq0,
		   wac_i2c->garage_gain1, wac_i2c->garage_freq1);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d", wac_i2c->garage_gain0,
			wac_i2c->garage_freq0, wac_i2c->garage_gain1,
			wac_i2c->garage_freq1);

out:
	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	return snprintf(buf, PAGE_SIZE, "NG");
}
#endif

static ssize_t get_epen_pos_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct sec_cmd_data *sec = dev_get_drvdata(dev);
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct i2c_client *client = wac_i2c->client;
	int max_x, max_y;

	if (wac_i2c->pdata->xy_switch) {
		max_x = wac_i2c->pdata->max_y;
		max_y = wac_i2c->pdata->max_x;
	} else {
		max_x = wac_i2c->pdata->max_x;
		max_y = wac_i2c->pdata->max_y;
	}

	input_info(true, &client->dev,
			"%s: id(%d), x(%d), y(%d), max_x(%d), max_y(%d)\n", __func__,
			wac_i2c->survey_pos.id, wac_i2c->survey_pos.x, wac_i2c->survey_pos.y,
			max_x, max_y);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d\n",
				wac_i2c->survey_pos.id, wac_i2c->survey_pos.x, wac_i2c->survey_pos.y,
				max_x, max_y);
}

/* firmware update */
static DEVICE_ATTR(epen_firm_update, (S_IWUSR | S_IWGRP),
		   NULL, epen_firmware_update_store);
/* return firmware update status */
static DEVICE_ATTR(epen_firm_update_status, S_IRUGO,
		   epen_firm_update_status_show, NULL);
/* return firmware version */
static DEVICE_ATTR(epen_firm_version, S_IRUGO, epen_firm_version_show, NULL);
static DEVICE_ATTR(ver_of_ic, S_IRUGO, epen_firm_version_of_ic_show, NULL);
static DEVICE_ATTR(ver_of_bin, S_IRUGO, epen_firm_version_of_bin_show, NULL);

/* For SMD Test */
static DEVICE_ATTR(epen_reset, (S_IWUSR | S_IWGRP), NULL, epen_reset_store);
static DEVICE_ATTR(epen_reset_result, S_IRUGO, epen_reset_result_show, NULL);
/* For SMD Test. Check checksum */
static DEVICE_ATTR(epen_checksum, (S_IWUSR | S_IWGRP),
		   NULL, epen_checksum_store);
static DEVICE_ATTR(epen_checksum_result, S_IRUGO,
		   epen_checksum_result_show, NULL);
static DEVICE_ATTR(epen_connection, S_IRUGO, epen_connection_show, NULL);
static DEVICE_ATTR(epen_saving_mode, (S_IWUSR | S_IWGRP),
		   NULL, epen_saving_mode_store);
static DEVICE_ATTR(epen_insert, S_IRUGO,
			epen_insert_show, NULL);
static DEVICE_ATTR(epen_wcharging_mode, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_wcharging_mode_show, epen_wcharging_mode_store);
static DEVICE_ATTR(epen_ble_charging_mode, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_ble_charging_mode_show, epen_ble_charging_mode_store);
static DEVICE_ATTR(keyboard_mode, (S_IWUSR | S_IWGRP),
		   NULL, epen_keyboard_mode_store);
static DEVICE_ATTR(epen_disable_mode, (S_IWUSR | S_IWGRP),
		   NULL, epen_disable_mode_store);
static DEVICE_ATTR(screen_off_memo_enable, (S_IWUSR | S_IWGRP),
		   NULL, epen_screen_off_memo_enable_store);
static DEVICE_ATTR(get_epen_pos,
			S_IRUGO, get_epen_pos_show, NULL);
static DEVICE_ATTR(aod_enable, (S_IWUSR | S_IWGRP),
		   NULL, epen_aod_enable_store);
static DEVICE_ATTR(aod_lcd_onoff_status, (S_IWUSR | S_IWGRP),
		   NULL, epen_aod_lcd_onoff_status_store);
static DEVICE_ATTR(aot_enable, (S_IWUSR | S_IWGRP),
		   NULL, epen_aot_enable_store);
static DEVICE_ATTR(pen_out_count, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_pen_out_count_show, epen_clear_pen_out_count_store);
static DEVICE_ATTR(abnormal_reset_count, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_abnormal_reset_count_show,
		   epen_clear_abnormal_reset_count_store);
static DEVICE_ATTR(i2c_fail_count, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_i2c_fail_count_show, epen_clear_i2c_fail_count_store);
static DEVICE_ATTR(epen_connection_check, S_IRUGO,
		   epen_connection_check_show, NULL);
#ifdef CONFIG_SEC_FACTORY
static DEVICE_ATTR(epen_fac_select_firmware, (S_IWUSR | S_IWGRP),
		   NULL, epen_fac_select_firmware_store);
static DEVICE_ATTR(epen_fac_garage_mode, (S_IRUGO | S_IWUSR | S_IWGRP),
		   epen_fac_garage_mode_show, epen_fac_garage_mode_enable);
static DEVICE_ATTR(epen_fac_garage_rawdata, S_IRUGO,
		   epen_fac_garage_rawdata_show, NULL);
#endif

static struct attribute *epen_attributes[] = {
	&dev_attr_epen_firm_update.attr,
	&dev_attr_epen_firm_update_status.attr,
	&dev_attr_epen_firm_version.attr,
	&dev_attr_ver_of_ic.attr,
	&dev_attr_ver_of_bin.attr,
	&dev_attr_epen_reset.attr,
	&dev_attr_epen_reset_result.attr,
	&dev_attr_epen_checksum.attr,
	&dev_attr_epen_checksum_result.attr,
	&dev_attr_epen_connection.attr,
	&dev_attr_epen_saving_mode.attr,
	&dev_attr_epen_insert.attr,
	&dev_attr_epen_wcharging_mode.attr,
	&dev_attr_epen_ble_charging_mode.attr,
	&dev_attr_keyboard_mode.attr,
	&dev_attr_epen_disable_mode.attr,
	&dev_attr_screen_off_memo_enable.attr,
	&dev_attr_get_epen_pos.attr,
	&dev_attr_aod_enable.attr,
	&dev_attr_aod_lcd_onoff_status.attr,
	&dev_attr_aot_enable.attr,
	&dev_attr_pen_out_count.attr,
	&dev_attr_abnormal_reset_count.attr,
	&dev_attr_i2c_fail_count.attr,
	&dev_attr_epen_connection_check.attr,
#ifdef CONFIG_SEC_FACTORY
	&dev_attr_epen_fac_select_firmware.attr,
	&dev_attr_epen_fac_garage_mode.attr,
	&dev_attr_epen_fac_garage_rawdata.attr,
#endif
	NULL,
};

static struct attribute_group epen_attr_group = {
	.attrs = epen_attributes,
};

static void print_spec_data(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	struct i2c_client *client = wac_i2c->client;
	u8 tmp_buf[CMD_RESULT_WORD_LEN] = { 0 };
	u8 *buff;
	int buff_size;
	int i;

	buff_size = edata->max_x_ch > edata->max_y_ch ? edata->max_x_ch : edata->max_y_ch;
	buff_size = CMD_RESULT_WORD_LEN * (buff_size + 1);

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (!buff)
		return;

	input_info(true, &client->dev, "%s: shift_value %d, spec ver %d", __func__,
		   edata->shift_value, edata->spec_ver);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->xx_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->xy_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->yx_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->yy_ref[i] * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->xx_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "xy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->xy_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->yx_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "yy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->yy_spec[i] / POWER_OFFSET * power(edata->shift_value));
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "rxx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->rxx_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "rxy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->rxy_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "ryx_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->ryx_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "ryy_ref: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->ryy_ref[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "drxx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->drxx_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "drxy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->drxy_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "dryx_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->dryx_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "dryy_spec: ");
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, CMD_RESULT_WORD_LEN, "%ld ",
			 edata->dryy_spec[i] * power(edata->shift_value) / POWER_OFFSET);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, CMD_RESULT_WORD_LEN);
	}

	input_info(true, &client->dev, "%s\n", buff);
	memset(buff, 0x00, buff_size);

	kfree(buff);
}

static void run_elec_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	struct wacom_elec_data *edata = wac_i2c->pdata->edata;
	struct i2c_client *client = wac_i2c->client;
	u8 *buff = NULL;
	u8 tmp_buf[CMD_RESULT_WORD_LEN] = { 0 };
	u8 data[COM_ELEC_NUM] = { 0, };
	u16 tmp, offset;
	u8 cmd;
	int i, tx, rx;
	int retry = RETRY_COUNT;
	int ret;
	int max_ch;
	int buff_size;
	u32 count;
	int remain_data_count;
	u16 *elec_result = NULL;

	sec_cmd_set_default_result(sec);

	wacom_enable_irq(wac_i2c, false);
	wacom_enable_pdct_irq(wac_i2c, false);

	if (!edata)
		goto out;

	max_ch = edata->max_x_ch + edata->max_y_ch;
	buff_size = max_ch * 2 * CMD_RESULT_WORD_LEN + SEC_CMD_STR_LEN;

	buff = kzalloc(buff_size, GFP_KERNEL);
	if (!buff)
		goto out;

	elec_result = kzalloc((max_ch + 1) * sizeof(u16), GFP_KERNEL);
	if (!elec_result)
		goto out;

	/* change wacom mode to 0x2D */
	mutex_lock(&wac_i2c->mode_lock);
	wacom_i2c_set_survey_mode(wac_i2c, EPEN_SURVEY_MODE_NONE);
	mutex_unlock(&wac_i2c->mode_lock);

	/* wait for status event for normal mode of wacom */
	msleep(250);
	do {
		ret = wacom_i2c_recv(wac_i2c, data, COM_COORD_NUM, WACOM_I2C_MODE_NORMAL);
		if (ret != COM_COORD_NUM) {
			input_err(true, &client->dev, "%s: failed to recv status data\n",
				  __func__);
			msleep(50);
			continue;
		}

		/* check packet ID */
		if ((data[0] & 0x0F) != NOTI_PACKET && (data[1] != CMD_PACKET)) {
			input_info(true, &client->dev,
				   "%s: invalid status data %d\n",
				   __func__, (RETRY_COUNT - retry + 1));
		} else {
			break;
		}

		msleep(50);
	} while(retry--);

	retry = RETRY_COUNT;
	cmd = COM_ELEC_XSCAN;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send 0x%02X\n", __func__, cmd);
		goto out;
	}
	msleep(30);

	cmd = COM_ELEC_TRSX0;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send 0x%02X\n", __func__, cmd);
		goto out;
	}
	msleep(30);

	cmd = COM_ELEC_SCAN_START;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send 0x%02X\n", __func__, cmd);
		goto out;
	}
	msleep(30);

	for (tx = 0; tx < max_ch; tx++) {
		/* x scan mode receive data through x coils */
		remain_data_count = edata->max_x_ch;

		do {
			cmd = COM_ELEC_REQUESTDATA;
			ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
			if (ret != 1) {
				input_err(true, &client->dev,
					  "%s: failed to send 0x%02X\n",
					  __func__, cmd);
				goto out;
			}
			msleep(30);

			ret = wacom_i2c_recv(wac_i2c, data, COM_ELEC_NUM, WACOM_I2C_MODE_NORMAL);
			if (ret != COM_ELEC_NUM) {
				input_err(true, &client->dev,
					  "%s: failed to receive elec data\n",
					  __func__);
				goto out;
			}
#if 0
			input_info(true, &client->dev, "%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
					data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32], data[33], data[34], data[35], data[36], data[37]);
#endif
			/* check packet ID & sub packet ID */
			if (((data[0] & 0x0F) != 0x0E) && (data[1] != 0x65)) {
				input_info(true, &client->dev,
					   "%s: %d data of wacom elec\n",
					   __func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_err(true, &client->dev,
						  "%s: invalid elec data %d %d\n",
						  __func__, (data[0] & 0x0F), data[1]);
					goto out;
				}
			}

			if (data[11] != 3) {
				input_info(true, &client->dev,
					   "%s: %d data of wacom elec\n",
					   __func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_info(true, &client->dev,
						   "%s: invalid elec status %d\n",
						   __func__, data[11]);
					goto out;
				}
			}

			for (i = 0; i < COM_ELEC_DATA_NUM; i++) {
				if (!remain_data_count)
					break;

				offset = COM_ELEC_DATA_POS + (i * 2);
				tmp = ((u16)(data[offset]) << 8) + data[offset + 1];
				rx = data[13] + i;

				edata->elec_data[rx * max_ch + tx] = tmp;

				remain_data_count--;
			}
		} while (remain_data_count);

		cmd = COM_ELEC_TRSCON;
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev, "%s: failed to send 0x%02X\n",
				  __func__, cmd);
			goto out;
		}
		msleep(30);
	}

	cmd = COM_ELEC_YSCAN;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send 0x%02X\n", __func__, cmd);
		goto out;
	}
	msleep(30);

	cmd = COM_ELEC_TRSX0;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send 0x%02X\n", __func__, cmd);
		goto out;
	}
	msleep(30);

	cmd = COM_ELEC_SCAN_START;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "%s: failed to send 0x%02X\n", __func__, cmd);
		goto out;
	}
	msleep(30);

	for (tx = 0; tx < max_ch; tx++) {
		/* y scan mode receive data through y coils */
		remain_data_count = edata->max_y_ch;

		do {
			cmd = COM_ELEC_REQUESTDATA;
			ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
			if (ret != 1) {
				input_err(true, &client->dev,
					  "%s: failed to send 0x%02X\n",
					  __func__, cmd);
				goto out;
			}
			msleep(30);

			ret = wacom_i2c_recv(wac_i2c, data, COM_ELEC_NUM, WACOM_I2C_MODE_NORMAL);
			if (ret != COM_ELEC_NUM) {
				input_err(true, &client->dev,
					  "%s: failed to receive elec data\n",
					  __func__);
				goto out;
			}
#if 0
			input_info(true, &client->dev, "%X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X %X\n",
					data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31], data[32], data[33], data[34], data[35], data[36], data[37]);
#endif
			/* check packet ID & sub packet ID */
			if (((data[0] & 0x0F) != 0x0E) && (data[1] != 0x65)) {
				input_info(true, &client->dev,
					   "%s: %d data of wacom elec\n",
					   __func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_err(true, &client->dev,
						  "%s: invalid elec data %d %d\n",
						  __func__, (data[0] & 0x0F), data[1]);
					goto out;
				}
			}

			if (data[11] != 3) {
				input_info(true, &client->dev,
					   "%s: %d data of wacom elec\n",
					   __func__, (RETRY_COUNT - retry + 1));
				if (--retry) {
					continue;
				} else {
					input_info(true, &client->dev,
						   "%s: invalid elec status %d\n",
						   __func__, data[11]);
					goto out;
				}
			}

			for (i = 0; i < COM_ELEC_DATA_NUM; i++) {
				if (!remain_data_count)
					break;

				offset = COM_ELEC_DATA_POS + (i * 2);
				tmp = ((u16)(data[offset]) << 8) + data[offset + 1];
				rx = edata->max_x_ch + data[13] + i;

				edata->elec_data[rx * max_ch + tx] = tmp;

				remain_data_count--;
			}
		} while (remain_data_count);

		cmd = COM_ELEC_TRSCON;
		ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
		if (ret != 1) {
			input_err(true, &client->dev, "%s: failed to send 0x%02X\n",
				  __func__, cmd);
			goto out;
		}
		msleep(30);
	}

	print_spec_data(wac_i2c);

	print_elec_data(wac_i2c);

	/* tx about x coil */
	for (tx = 0, i = 0; tx < edata->max_x_ch; tx++, i++) {
		edata->xx[i] = 0;
		edata->xy[i] = 0;
		/* rx about x coil */
		for (rx = 0; rx < edata->max_x_ch; rx++) {
			offset = rx * max_ch + tx;
			if ((edata->elec_data[offset] > edata->xx[i]) && (tx != rx))
				edata->xx[i] = edata->elec_data[offset];
		}
		/* rx about y coil */
		for (rx = edata->max_x_ch; rx < max_ch; rx++) {
			offset = rx * max_ch + tx;
			if (edata->elec_data[offset] > edata->xy[i])
				edata->xy[i] = edata->elec_data[offset];
		}
	}

	/* tx about y coil */
	for (tx = edata->max_x_ch, i = 0; tx < max_ch; tx++, i++) {
		edata->yx[i] = 0;
		edata->yy[i] = 0;
		/* rx about x coil */
		for (rx = 0; rx < edata->max_x_ch; rx++) {
			offset = rx * max_ch + tx;
			if (edata->elec_data[offset] > edata->yx[i])
				edata->yx[i] = edata->elec_data[offset];
		}
		/* rx about y coil */
		for (rx = edata->max_x_ch; rx < max_ch; rx++) {
			offset = rx * max_ch + tx;
			if ((edata->elec_data[offset] > edata->yy[i]) && (rx != tx))
				edata->yy[i] = edata->elec_data[offset];
		}
	}

	print_trx_data(wac_i2c);

	calibration_trx_data(wac_i2c);

	print_cal_trx_data(wac_i2c);

	calculate_ratio(wac_i2c);

	print_ratio_trx_data(wac_i2c);

	make_decision(wac_i2c, elec_result);

	print_difference_ratio_trx_data(wac_i2c);

	count = elec_result[0] >> 8;
	if (!count) {
		snprintf(tmp_buf, sizeof(tmp_buf), "0_");
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	} else {
		if (count > 3)
			snprintf(tmp_buf, sizeof(tmp_buf), "3,");
		else
			snprintf(tmp_buf, sizeof(tmp_buf), "%d,", count);

		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));

		for (i = 0, count = 0; i < max_ch; i++) {
			if ((elec_result[i + 1] & SEC_SHORT) && count < 3) {
				if (i < edata->max_x_ch)
					snprintf(tmp_buf, sizeof(tmp_buf), "X%d,", i);
				else
					snprintf(tmp_buf, sizeof(tmp_buf), "Y%d,", i - edata->max_x_ch);

				strlcat(buff, tmp_buf, buff_size);
				memset(tmp_buf, 0x00, sizeof(tmp_buf));
				count++;
			}
		}
		buff[strlen(buff) - 1] = '_';
	}

	count = elec_result[0] & 0x00FF;
	if (!count) {
		snprintf(tmp_buf, sizeof(tmp_buf), "0_");
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	} else {
		if (count > 3)
			snprintf(tmp_buf, sizeof(tmp_buf), "3,");
		else
			snprintf(tmp_buf, sizeof(tmp_buf), "%d,", count);

		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));

		for (i = 0, count = 0; i < max_ch; i++) {
			if ((elec_result[i + 1] & SEC_OPEN) && count < 3) {
				if (i < edata->max_x_ch)
					snprintf(tmp_buf, sizeof(tmp_buf), "X%d,", i);
				else
					snprintf(tmp_buf, sizeof(tmp_buf), "Y%d,", i - edata->max_x_ch);

				strlcat(buff, tmp_buf, buff_size);
				memset(tmp_buf, 0x00, sizeof(tmp_buf));
				count++;
			}
		}
		buff[strlen(buff) - 1] = '_';
	}

	snprintf(tmp_buf, sizeof(tmp_buf), "%d,%d_", edata->max_x_ch, edata->max_y_ch);
	strlcat(buff, tmp_buf, buff_size);
	memset(tmp_buf, 0x00, sizeof(tmp_buf));

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,",  edata->xx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_x_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->xy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->yx[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	for (i = 0; i < edata->max_y_ch; i++) {
		snprintf(tmp_buf, sizeof(tmp_buf), "%d,", edata->yy[i]);
		strlcat(buff, tmp_buf, buff_size);
		memset(tmp_buf, 0x00, sizeof(tmp_buf));
	}

	sec_cmd_set_cmd_result(sec, buff, buff_size);
	sec->cmd_state = SEC_CMD_STATUS_OK;

	kfree(elec_result);
	kfree(buff);

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "failed to send stop cmd for end\n");
	}

	wac_i2c->samplerate_state = false;

	data[0] = COM_SAMPLERATE_133;
	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send start cmd, %d\n",
			  __func__, ret);
	} else {
		wac_i2c->samplerate_state = true;
	}

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	return;

out:
	if (elec_result)
		kfree(elec_result);

	if (buff)
		kfree(buff);

	snprintf(tmp_buf, sizeof(tmp_buf), "NG");
	sec_cmd_set_cmd_result(sec, tmp_buf, sizeof(tmp_buf));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;

	cmd = COM_SAMPLERATE_STOP;
	ret = wacom_i2c_send(wac_i2c, &cmd, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &client->dev,
			  "failed to send stop cmd for end\n");
	}

	wac_i2c->samplerate_state = false;

	data[0] = COM_SAMPLERATE_133;
	ret = wacom_i2c_send(wac_i2c, &data[0], 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send start cmd, %d\n",
			  __func__, ret);
	} else {
		wac_i2c->samplerate_state = true;
	}

	wacom_enable_irq(wac_i2c, true);
	wacom_enable_pdct_irq(wac_i2c, true);

	return;
}

static void ready_elec_test(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	char data;
	int ret;

	sec_cmd_set_default_result(sec);

	if (sec->cmd_param[0])
		data = COM_ASYNC_VSYNC;
	else
		data = COM_SYNC_VSYNC;

	ret = wacom_i2c_send(wac_i2c, &data, 1, WACOM_I2C_MODE_NORMAL);
	if (ret != 1) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send data(%02x, %d)\n",
			  __func__, data, ret);
		snprintf(buff, sizeof(buff), "NG\n");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

		goto out;
	}

	msleep(30);

	ret = wacom_ble_charge_mode(wac_i2c, !sec->cmd_param[0]);
	if (ret) {
		input_err(true, &wac_i2c->client->dev,
			  "%s: failed to send charge cmd(%02x, %d)\n",
			  __func__, ble_charge_command[!sec->cmd_param[0]], ret);

		snprintf(buff, sizeof(buff), "NG\n");
		sec->cmd_state = SEC_CMD_STATUS_FAIL;

		goto out;
	}

	snprintf(buff, sizeof(buff), "OK\n");
	sec->cmd_state = SEC_CMD_STATUS_OK;

out:
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &wac_i2c->client->dev, "%s: %02x %02x %s\n", __func__,
		  data, ble_charge_command[!sec->cmd_param[0]], buff);

	return;
}

static void get_chip_name(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);

	if (wac_i2c->pdata->ic_type == 9020)
		snprintf(buff, sizeof(buff), "W9020");
	else if (wac_i2c->pdata->ic_type == 9021)
		snprintf(buff, sizeof(buff), "W9021");
	else
		snprintf(buff, sizeof(buff), "N/A");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_OK;
}

enum {
	WACOM_BUILT_IN = 0,
	WACOM_UMS,
	WACOM_NONE,
	WACOM_SPU,
	WACOM_TEST,
};

/* Factory cmd for firmware update
 * argument represent what is source of firmware like below.
 *
 * 0 : [BUILT_IN] Getting firmware which is for user.
 * 1 : [UMS] Getting firmware from sd card.
 * 2 : none
 * 3 : [FFU] Getting firmware from apk.
 */

static void fw_update(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	struct wacom_i2c *wac_i2c = container_of(sec, struct wacom_i2c, sec);
	char buff[SEC_CMD_STR_LEN] = { 0 };
	int ret = 0;

	sec_cmd_set_default_result(sec);

	if (!wac_i2c->power_enable) {
		input_err(true, &wac_i2c->client->dev, "%s: [ERROR] Wacom is stopped\n", __func__);
		goto err_out;
	}

	if (sec->cmd_param[0] < 0 || sec->cmd_param[0] > WACOM_TEST) {
		input_err(true, &wac_i2c->client->dev, "%s: [ERROR] parm error![%d]\n",
						__func__, sec->cmd_param[0]);
		goto err_out;
	}

	switch (sec->cmd_param[0]) {
	case WACOM_BUILT_IN:
		ret = wacom_fw_update(wac_i2c, FW_BUILT_IN, true);
		break;
	case WACOM_UMS:
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
		ret = wacom_fw_update(wac_i2c, FW_IN_SDCARD, true);
#else
		ret = wacom_fw_update(wac_i2c, FW_IN_SDCARD_SIGNED, true);
#endif
		break;
	case WACOM_SPU:
		ret = wacom_fw_update(wac_i2c, FW_SPU, true);
		break;
	case WACOM_TEST:
		ret = wacom_fw_update(wac_i2c, FW_IN_SDCARD_SIGNED, true);
		break;
	default:
		input_err(true, &wac_i2c->client->dev, "%s: Not support command[%d]\n",
				__func__, sec->cmd_param[0]);
		goto err_out;
	}

	if (ret) {
		input_err(true, &wac_i2c->client->dev, "%s: Wacom fw upate(%d) fail ret [%d]\n",
					__func__, sec->cmd_param[0], ret);
		goto err_out;
	}

	input_info(true, &wac_i2c->client->dev, "%s: Wacom fw upate[%d]\n", __func__, sec->cmd_param[0]);

	snprintf(buff, sizeof(buff), "OK\n");
	sec->cmd_state = SEC_CMD_STATUS_OK;
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));

	input_info(true, &wac_i2c->client->dev, "%s: Done\n", __func__);

	return;

err_out:
	snprintf(buff, sizeof(buff), "NG");
	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_FAIL;
	return;

}

static void not_support_cmd(void *device_data)
{
	struct sec_cmd_data *sec = (struct sec_cmd_data *)device_data;
	char buff[SEC_CMD_STR_LEN] = { 0 };

	sec_cmd_set_default_result(sec);
	snprintf(buff, sizeof(buff), "NA");

	sec_cmd_set_cmd_result(sec, buff, strnlen(buff, sizeof(buff)));
	sec->cmd_state = SEC_CMD_STATUS_NOT_APPLICABLE;
	sec_cmd_set_cmd_exit(sec);
}

#define EPEN_CHARGER_OFF_TIME	120
enum epen_scan_info{
	EPEN_CHARGE_OFF			= 0,
	EPEN_CHARGE_ON			= 1,
	EPEN_CHARGE_HAPPENED	= 2,
};

int get_wacom_scan_info(bool mode)
{
	struct wacom_i2c *wac_i2c = wacom_get_drv_data(NULL);
	struct timeval current_time;
	u32 diff_time;

	do_gettimeofday(&current_time);
	diff_time = (u32)(current_time.tv_sec) - wac_i2c->chg_time_stamp;
	input_info(true, &wac_i2c->client->dev, "%s: mode[%d] & ble[%x] & diff time[%d]\n",
			__func__, mode, wac_i2c->ble_mode, diff_time);

	if (!wac_i2c->probe_done || !wac_i2c->power_enable || wac_i2c->is_open_test) {
		input_err(true, &wac_i2c->client->dev, "%s: wacom is not ready\n",
			  __func__);
		return -EPERM;
	}

	if (mode) {
		wac_i2c->is_mode_change = false;
	} else if (!mode && wac_i2c->is_mode_change) {
		input_info(true, &wac_i2c->client->dev, "#2 wacom mode change was happen\n");
		return EPEN_CHARGE_HAPPENED;
	}

	if (!wac_i2c->pen_pdct){
		input_err(true, &wac_i2c->client->dev, "%s: #0 pen out!\n", __func__);
		return EPEN_CHARGE_OFF;
	}

	if (wac_i2c->ble_mode == COM_BLE_C_KEEP_ON
		|| wac_i2c->ble_mode == COM_BLE_C_FULL) {
		input_err(true, &wac_i2c->client->dev, "%s: #1 charging[%x]\n",
					__func__, wac_i2c->ble_mode);
		return EPEN_CHARGE_ON;
	}

	if (wac_i2c->ble_mode == COM_BLE_C_ENABLE
			|| wac_i2c->ble_mode == COM_BLE_C_RESET
			|| wac_i2c->ble_mode == COM_BLE_C_START) {

		if (diff_time > EPEN_CHARGER_OFF_TIME) {
			input_info(true, &wac_i2c->client->dev, "%s: #0 over time BLE chg[%x], diff[%d]\n",
						__func__, wac_i2c->ble_mode, diff_time);
			return EPEN_CHARGE_OFF;
			
		} else {
			input_err(true, &wac_i2c->client->dev, "%s: #1 under time BLE chg[%x], diff[%d]\n",
						__func__, wac_i2c->ble_mode, diff_time);
			return EPEN_CHARGE_ON;
		}
	}

	if ((wac_i2c->chg_time_stamp != 0) && (diff_time < EPEN_CHARGER_OFF_TIME)) {
		input_err(true, &wac_i2c->client->dev, "%s: #1 pen in under time BLE chg[%x], diff[%d]\n",
					__func__, wac_i2c->ble_mode, diff_time);
		return EPEN_CHARGE_ON;
	}

	input_info(true, &wac_i2c->client->dev, "%s: #0 mode[%d] & ble[%x] & diff time[%d]\n",
			__func__, mode, wac_i2c->ble_mode, diff_time);
	return EPEN_CHARGE_OFF;
}
EXPORT_SYMBOL(get_wacom_scan_info);

int set_wacom_ble_charge_mode(bool mode)
{
	struct wacom_i2c *wac_i2c = wacom_get_drv_data(NULL);
	int ret = 0;

	input_info(true, &wac_i2c->client->dev, "%s start(%d)\n", __func__, mode);

	mutex_lock(&wac_i2c->ble_charge_mode_lock);
	if (!mode) {
		ret = wacom_ble_charge_mode(wac_i2c, EPEN_BLE_C_DIABLE);
		if (!ret)
			wac_i2c->ble_block_flag = true;
	} else {
		wac_i2c->ble_block_flag = false;
#ifdef CONFIG_SEC_FACTORY
		ret = wacom_ble_charge_mode(wac_i2c, EPEN_BLE_C_ENABLE);
#endif
	}
	mutex_unlock(&wac_i2c->ble_charge_mode_lock);

	input_info(true, &wac_i2c->client->dev, "%s done(%d)\n", __func__, mode);

	return ret;
}
EXPORT_SYMBOL(set_wacom_ble_charge_mode);

static int wacom_sec_elec_init(struct wacom_i2c *wac_i2c)
{
	struct wacom_elec_data *edata = NULL;
	struct i2c_client *client = wac_i2c->client;
	struct device_node *np = wac_i2c->client->dev.of_node;
	u32 tmp[2];
	int max_len;
	int ret;
	int i;

	np = of_get_child_by_name(np, "wacom_elec");
	if (!np) {
		input_info(true, &client->dev,
			   "%s: node is not exist for elec\n", __func__);
		return 0;
	}

	edata = devm_kzalloc(&client->dev, sizeof(*edata), GFP_KERNEL);
	if (!edata)
		return -ENOMEM;

	wac_i2c->pdata->edata = edata;

	ret = of_property_read_u32(np, "spec_ver", tmp);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read sepc ver %d\n", ret);
		return -EINVAL;
	}
	edata->spec_ver = tmp[0];

	ret = of_property_read_u32_array(np, "max_channel", tmp, 2);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read max channel %d\n", ret);
		return -EINVAL;
	}
	edata->max_x_ch = tmp[0];
	edata->max_y_ch = tmp[1];

	ret = of_property_read_u32(np, "shift_value", tmp);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read max channel %d\n", ret);
		return -EINVAL;
	}
	edata->shift_value = tmp[0];

	input_info(true, &client->dev, "channel(%d %d), spec_ver %d shift_value %d\n",
		   edata->max_x_ch, edata->max_y_ch, edata->spec_ver, edata->shift_value);

	max_len = edata->max_x_ch + edata->max_y_ch;

	edata->elec_data = devm_kzalloc(&client->dev,
					max_len * max_len * sizeof(u16),
					GFP_KERNEL);
	if (!edata->elec_data)
		return -ENOMEM;

	edata->xx = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(u16), GFP_KERNEL);
	edata->xy = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(u16), GFP_KERNEL);
	edata->yx = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(u16), GFP_KERNEL);
	edata->yy = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(u16), GFP_KERNEL);
	if (!edata->xx || !edata->xy || !edata->yx || !edata->yy)
		return -ENOMEM;

	edata->xx_xx = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_xy = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_yx = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_yy = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_xx || !edata->xy_xy || !edata->yx_yx || !edata->yy_yy)
		return -ENOMEM;

	edata->rxx = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->rxy = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->ryx = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->ryy = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->rxx || !edata->rxy || !edata->ryx || !edata->ryy)
		return -ENOMEM;

	edata->drxx = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->drxy = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->dryx = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->dryy = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->drxx || !edata->drxy || !edata->dryx || !edata->dryy)
		return -ENOMEM;

	edata->xx_ref = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_ref = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_ref = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_ref = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_ref || !edata->xy_ref || !edata->yx_ref || !edata->yy_ref)
		return -ENOMEM;

	ret = of_property_read_u64_array(np, "xx_ref", edata->xx_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xx reference data %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u64_array(np, "xy_ref", edata->xy_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xy reference data %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u64_array(np, "yx_ref", edata->yx_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yx reference data %d\n", ret);
		return -EINVAL;
	}

	ret = of_property_read_u64_array(np, "yy_ref", edata->yy_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yy reference data %d\n", ret);
		return -EINVAL;
	}

	edata->xx_spec = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->xy_spec = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->yx_spec = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->yy_spec = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->xx_spec || !edata->xy_spec || !edata->yx_spec || !edata->yy_spec)
		return -ENOMEM;

	ret = of_property_read_u64_array(np, "xx_spec", edata->xx_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xx spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->xx_spec[i] = edata->xx_spec[i] * POWER_OFFSET;

	ret = of_property_read_u64_array(np, "xy_spec", edata->xy_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xy spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->xy_spec[i] = edata->xy_spec[i] * POWER_OFFSET;

	ret = of_property_read_u64_array(np, "yx_spec", edata->yx_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yx spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->yx_spec[i] = edata->yx_spec[i] * POWER_OFFSET;

	ret = of_property_read_u64_array(np, "yy_spec", edata->yy_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yy spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->yy_spec[i] = edata->yy_spec[i] * POWER_OFFSET;

	edata->rxx_ref = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->rxy_ref = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->ryx_ref = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->ryy_ref = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->rxx_ref || !edata->rxy_ref || !edata->ryx_ref || !edata->ryy_ref)
		return -ENOMEM;

	ret = of_property_read_u64_array(np, "rxx_ref", edata->rxx_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xx ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->rxx_ref[i] = edata->rxx_ref[i] * POWER_OFFSET / power(edata->shift_value);

	ret = of_property_read_u64_array(np, "rxy_ref", edata->rxy_ref, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xy ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->rxy_ref[i] = edata->rxy_ref[i] * POWER_OFFSET / power(edata->shift_value);

	ret = of_property_read_u64_array(np, "ryx_ref", edata->ryx_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yx ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->ryx_ref[i] = edata->ryx_ref[i] * POWER_OFFSET / power(edata->shift_value);

	ret = of_property_read_u64_array(np, "ryy_ref", edata->ryy_ref, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yy ratio reference data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->ryy_ref[i] = edata->ryy_ref[i] * POWER_OFFSET / power(edata->shift_value);

	edata->drxx_spec = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->drxy_spec = devm_kzalloc(&client->dev, edata->max_x_ch * sizeof(long long), GFP_KERNEL);
	edata->dryx_spec = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	edata->dryy_spec = devm_kzalloc(&client->dev, edata->max_y_ch * sizeof(long long), GFP_KERNEL);
	if (!edata->drxx_spec || !edata->drxy_spec || !edata->dryx_spec || !edata->dryy_spec)
		return -ENOMEM;

	ret = of_property_read_u64_array(np, "drxx_spec", edata->drxx_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xx difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->drxx_spec[i] = edata->drxx_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = of_property_read_u64_array(np, "drxy_spec", edata->drxy_spec, edata->max_x_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read xy difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_x_ch; i++)
		edata->drxy_spec[i] = edata->drxy_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = of_property_read_u64_array(np, "dryx_spec", edata->dryx_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yx difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->dryx_spec[i] = edata->dryx_spec[i] * POWER_OFFSET / power(edata->shift_value);

	ret = of_property_read_u64_array(np, "dryy_spec", edata->dryy_spec, edata->max_y_ch);
	if (ret) {
		input_err(true, &client->dev,
			  "failed to read yy difference ratio spec data %d\n", ret);
		return -EINVAL;
	}

	for (i = 0; i < edata->max_y_ch; i++)
		edata->dryy_spec[i] = edata->dryy_spec[i] * POWER_OFFSET / power(edata->shift_value);

	return 1;
}

int wacom_sec_init(struct wacom_i2c *wac_i2c)
{
	struct i2c_client *client = wac_i2c->client;
	int retval = 0;

	retval = sec_cmd_init(&wac_i2c->sec, sec_cmds, ARRAY_SIZE(sec_cmds),
			      SEC_CLASS_DEVT_WACOM);
	if (retval < 0) {
		input_err(true, &client->dev, "failed to sec_cmd_init\n");
		return retval;
	}

	retval = sysfs_create_group(&wac_i2c->sec.fac_dev->kobj, &epen_attr_group);
	if (retval) {
		input_err(true, &client->dev, "failed to create sysfs attributes\n");
		goto err_sysfs_create_group;
	}

	retval = sysfs_create_link(&wac_i2c->sec.fac_dev->kobj,
				   &wac_i2c->input_dev->dev.kobj, "input");
	if (retval) {
		input_err(true, &client->dev, "failed to create sysfs link\n");
		goto err_create_symlink;
	}

	retval = wacom_sec_elec_init(wac_i2c);
	if (retval < 0) {
		input_err(true, &client->dev, "failed to init elec data\n");
		goto err_init_elec;
	}

	return 0;

err_init_elec:
	sysfs_delete_link(&wac_i2c->sec.fac_dev->kobj,
			  &wac_i2c->input_dev->dev.kobj, "input");
err_create_symlink:
	sysfs_remove_group(&wac_i2c->sec.fac_dev->kobj, &epen_attr_group);
err_sysfs_create_group:
	sec_cmd_exit(&wac_i2c->sec, SEC_CLASS_DEVT_WACOM);

	return retval;
}

void wacom_sec_remove(struct wacom_i2c *wac_i2c)
{
	sysfs_delete_link(&wac_i2c->sec.fac_dev->kobj,
			  &wac_i2c->input_dev->dev.kobj, "input");
	sysfs_remove_group(&wac_i2c->sec.fac_dev->kobj, &epen_attr_group);
	sec_cmd_exit(&wac_i2c->sec, SEC_CLASS_DEVT_WACOM);
}
