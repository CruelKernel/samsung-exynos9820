/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "hrmsensor.h"
#ifdef CONFIG_SENSORS_HRM_MAX86902
#include "hrm_max86902.h"
static struct hrm_func max869_func = {
	.i2c_read = max869_i2c_read,
	.i2c_write = max869_i2c_write,
	.init_device = max869_init_device,
	.deinit_device = max869_deinit_device,
	.enable = max869_enable,
	.disable = max869_disable,
	.set_sampling_rate = NULL,
	.get_led_current = max869_get_current,
	.set_led_current = max869_set_current,
	.get_led_test = NULL,
	.read_data = max869_read_data,
	.get_chipid = max869_get_chipid,
	.get_part_type = max869_get_part_type,
	.get_i2c_err_cnt = max869_get_i2c_err_cnt,
	.set_i2c_err_cnt = max869_set_i2c_err_cnt,
	.get_curr_adc = max869_get_curr_adc,
	.set_curr_adc = max869_set_curr_adc,
	.get_name_chipset = max869_get_name_chipset,
	.get_name_vendor = max869_get_name_vendor,
	.get_threshold = max869_get_threshold,
	.set_threshold = max869_set_threshold,
	.set_eol_enable = max869_set_eol_enable,
	.get_eol_result = max869_get_eol_result,
	.get_eol_status = max869_get_eol_status,
	.set_pre_eol_enable = NULL,
	.hrm_debug_set = max869_debug_set,
	.get_fac_cmd = max869_get_fac_cmd,
	.get_version = max869_get_version,
	.get_sensor_info = NULL,
	.get_xtalk_code = NULL,
	.set_xtalk_code = NULL,
	.get_sdk_enabled = NULL,
	.set_sdk_enabled = NULL,
};
#endif
#ifdef CONFIG_SENSORS_HRM_MAX86915
#include "hrm_max86915.h"
static struct hrm_func max869_func = {
	.i2c_read = max869_i2c_read,
	.i2c_write = max869_i2c_write,
	.init_device = max869_init_device,
	.deinit_device = max869_deinit_device,
	.enable = max869_enable,
	.disable = max869_disable,
	.set_sampling_rate = max869_set_sampling_rate,
	.get_led_current = max869_get_current,
	.set_led_current = max869_set_current,
	.get_led_test = max869_get_led_test,
	.read_data = max869_read_data,
	.get_chipid = max869_get_chipid,
	.get_part_type = max869_get_part_type,
	.get_i2c_err_cnt = max869_get_i2c_err_cnt,
	.set_i2c_err_cnt = max869_set_i2c_err_cnt,
	.get_curr_adc = max869_get_curr_adc,
	.set_curr_adc = max869_set_curr_adc,
	.get_name_chipset = max869_get_name_chipset,
	.get_name_vendor = max869_get_name_vendor,
	.get_threshold = max869_get_threshold,
	.set_threshold = max869_set_threshold,
	.set_eol_enable = max869_set_eol_enable,
	.get_eol_result = max869_get_eol_result,
	.get_eol_status = max869_get_eol_status,
	.set_pre_eol_enable = max869_set_pre_eol_enable,
	.hrm_debug_set = max869_debug_set,
	.get_fac_cmd = max869_get_fac_cmd,
	.get_version = max869_get_version,
	.get_sensor_info = NULL,
	.get_xtalk_code = max869_get_xtalk,
	.set_xtalk_code = max869_set_xtalk,
	.get_sdk_enabled = max869_get_sdk_enabled,
	.set_sdk_enabled = max869_set_sdk_enabled,
};
#endif

#define MODULE_NAME_HRM		"hrm_sensor"
#define DEFAULT_THRESHOLD -4194303
#define SLAVE_ADDR_MAX 0x57

#define VERSION				"33"

int hrm_debug = 1;
int hrm_info;

module_param(hrm_debug, int, S_IRUGO | S_IWUSR);
module_param(hrm_info, int, S_IRUGO | S_IWUSR);

/* #define DEBUG_HRMSENSOR */
#ifdef DEBUG_HRMSENSOR
static void __hrm_debug_device_data(struct hrm_device_data *data)
{
	HRM_dbg("===== %s =====\n", __func__);
	HRM_dbg("%s hrm_i2c_client %p slave_addr 0x%x\n", __func__,
		data->hrm_i2c_client, data->hrm_i2c_client->addr);
	HRM_dbg("%s dev %p\n", __func__, data->dev);
	HRM_dbg("%s hrm_input_dev %p\n", __func__, data->hrm_input_dev);
	HRM_dbg("%s hrm_pinctrl %p\n", __func__, data->hrm_pinctrl);
	HRM_dbg("%s pins_sleep %p\n", __func__, data->pins_sleep);
	HRM_dbg("%s pins_idle %p\n", __func__, data->pins_idle);
	HRM_dbg("%s led_3p3 %s\n", __func__, data->led_3p3);
	HRM_dbg("%s vdd_1p8 %s\n", __func__, data->vdd_1p8);
	HRM_dbg("%s i2c_1p8 %s\n", __func__, data->i2c_1p8);
	HRM_dbg("%s h_func %p\n", __func__, data->h_func);
	HRM_dbg("%s hrm_enabled_mode %d\n", __func__, data->hrm_enabled_mode);
	HRM_dbg("%s hrm_sampling_rate %d\n", __func__, data->hrm_sampling_rate);
	HRM_dbg("%s regulator_state %d\n", __func__, data->regulator_state);
	HRM_dbg("%s hrm_int %d\n", __func__, data->hrm_int);
	HRM_dbg("%s hrm_en %d\n", __func__, data->hrm_en);
	HRM_dbg("%s hrm_irq %d\n", __func__, data->hrm_irq);
	HRM_dbg("%s irq_state %d\n", __func__, data->irq_state);
	HRM_dbg("%s led_current %d\n", __func__, data->led_current);
	HRM_dbg("%s xtalk_code %d\n", __func__, data->xtalk_code);
	HRM_dbg("%s hrm_threshold %d\n", __func__, data->hrm_threshold);
	HRM_dbg("%s eol_test_is_enable %d\n", __func__,
		data->eol_test_is_enable);
	HRM_dbg("%s eol_test_status %d\n", __func__,
		data->eol_test_status);
	HRM_dbg("%s pre_eol_test_is_enable %d\n", __func__,
		data->pre_eol_test_is_enable);
	HRM_dbg("%s lib_ver %s\n", __func__, data->lib_ver);
	HRM_dbg("===== %s =====\n", __func__);
}
#endif

static void hrm_init_device_data(struct hrm_device_data *data)
{
	data->hrm_i2c_client = NULL;
	data->dev = NULL;
	data->hrm_input_dev = NULL;
	data->hrm_pinctrl = NULL;
	data->pins_sleep = NULL;
	data->pins_idle = NULL;
	data->led_3p3 = NULL;
	data->vdd_1p8 = NULL;
	data->i2c_1p8 = NULL;
	data->h_func = NULL;
	data->hrm_enabled_mode = 0;
	data->sampling_period_ns = 0;
	data->mode_sdk_enabled = 0;
	data->regulator_state = 0;
	data->irq_state = 0;
	data->hrm_threshold = DEFAULT_THRESHOLD;
	data->prox_threshold = 0;
	data->eol_test_is_enable = 0;
	data->eol_test_status = 0;
	data->pre_eol_test_is_enable = 0;
	data->reg_read_buf = 0;
	data->lib_ver = NULL;
	data->pm_state = PM_RESUME;
	data->hrm_prev_mode = 0;
	data->mode_sdk_prev = 0;
}

static void hrm_irq_set_state(struct hrm_device_data *data, int irq_enable)
{
	HRM_info("%s - irq_enable : %d, irq_state : %d\n",
		__func__, irq_enable, data->irq_state);

	if (irq_enable) {
		if (data->irq_state++ == 0)
			enable_irq(data->hrm_irq);
	} else {
		if (data->irq_state == 0)
			return;
		if (--data->irq_state <= 0) {
			disable_irq(data->hrm_irq);
			data->irq_state = 0;
		}
	}
}

static int hrm_power_ctrl(struct hrm_device_data *data, int onoff)
{
	int rc = 0;
	static int i2c_1p8_enable;
#ifndef CONFIG_SENSORS_HRM_MAX86915
	struct regulator *regulator_led_3p3;
#endif
	struct regulator *regulator_vdd_1p8;
	struct regulator *regulator_i2c_1p8 = 0;

	HRM_dbg("%s - onoff : %d, state : %d\n",
		__func__, onoff, data->regulator_state);

	if (onoff == HRM_ON) {
		if (data->regulator_state != 0) {
			HRM_dbg("%s - duplicate regulator : %d\n",
				__func__, onoff);
			data->regulator_state++;
			return 0;
		}
		data->regulator_state++;
		data->pm_state = PM_RESUME;
	} else {
		if (data->regulator_state == 0) {
			HRM_dbg("%s - already off the regulator : %d\n",
				__func__, onoff);
			return 0;
		} else if (data->regulator_state != 1) {
			HRM_dbg("%s - duplicate regulator : %d\n",
				__func__, onoff);
			data->regulator_state--;
			return 0;
		}
		data->regulator_state--;
	}

	if (data->i2c_1p8 != NULL) {
		regulator_i2c_1p8 = regulator_get(NULL, data->i2c_1p8);
		if (IS_ERR(regulator_i2c_1p8) || regulator_i2c_1p8 == NULL) {
			HRM_dbg("%s - i2c_1p8 regulator_get fail\n", __func__);
			rc = PTR_ERR(regulator_i2c_1p8);
			regulator_i2c_1p8 = NULL;
			goto get_i2c_1p8_failed;
		}
	}

#ifdef CONFIG_ARCH_QCOM
	regulator_vdd_1p8 =
		regulator_get(&data->hrm_i2c_client->dev, "hrmsensor_1p8");
#else /* EXYNOS */
	regulator_vdd_1p8 = regulator_get(NULL, data->vdd_1p8);
#endif
	if (IS_ERR(regulator_vdd_1p8) || regulator_vdd_1p8 == NULL) {
		HRM_dbg("%s - vdd_1p8 regulator_get fail\n", __func__);
		rc = PTR_ERR(regulator_vdd_1p8);
		regulator_vdd_1p8 = NULL;
		goto get_vdd_1p8_failed;
	}

#ifndef CONFIG_SENSORS_HRM_MAX86915
#ifdef CONFIG_ARCH_QCOM
	regulator_led_3p3 =
		regulator_get(&data->hrm_i2c_client->dev, "hrmsensor_3p3");
#else /* EXYNOS */
	regulator_led_3p3 = regulator_get(NULL, data->led_3p3);
#endif
	if (IS_ERR(regulator_led_3p3) || regulator_led_3p3 == NULL) {
		HRM_dbg("%s - led_3p3 regulator_get fail\n", __func__);
		rc = PTR_ERR(regulator_led_3p3);
		regulator_led_3p3 = NULL;
		goto get_led_3p3_failed;
	}
#endif
	HRM_dbg("%s - onoff = %d\n", __func__, onoff);

	if (onoff == HRM_ON) {
		if (data->i2c_1p8 != NULL && i2c_1p8_enable == 0) {
			rc = regulator_enable(regulator_i2c_1p8);
			i2c_1p8_enable = 1;
			if (rc) {
				HRM_dbg("enable i2c_1p8 failed, rc=%d\n", rc);
				goto enable_i2c_1p8_failed;
			}
		}
#if !defined(CONFIG_ARCH_QCOM) /* EXYNOS */
		rc = regulator_set_voltage(regulator_vdd_1p8,
			1800000, 1800000);
		if (rc < 0) {
			HRM_dbg("%s - set vdd_1p8 failed, rc=%d\n",
				__func__, rc);
			goto enable_vdd_1p8_failed;
		}
#endif
		rc = regulator_enable(regulator_vdd_1p8);
		if (rc) {
			HRM_dbg("%s - enable vdd_1p8 failed, rc=%d\n",
				__func__, rc);
			goto enable_vdd_1p8_failed;
		}
#ifdef CONFIG_SENSORS_HRM_MAX86915
		rc = gpio_direction_output(data->hrm_en, 1);
		if (rc) {
			HRM_dbg("%s - gpio direction output failed, rc=%d\n",
				__func__, rc);
			goto gpio_direction_output_failed;
		}
#else
#if !defined(CONFIG_ARCH_QCOM) /* EXYNOS */
		rc = regulator_set_voltage(regulator_led_3p3,
			3300000, 3300000);
		if (rc < 0) {
			HRM_dbg("%s - set led_3p3 failed, rc=%d\n",
				__func__, rc);
			goto enable_led_3p3_failed;
		}
#endif
		rc = regulator_enable(regulator_led_3p3);
		if (rc) {
			HRM_dbg("%s - enable led_3p3 failed, rc=%d\n",
				__func__, rc);
			goto enable_led_3p3_failed;
		}
#endif
		usleep_range(1000, 1100);
	} else {
		rc = regulator_disable(regulator_vdd_1p8);
		if (rc) {
			HRM_dbg("%s - disable vdd_1p8 failed, rc=%d\n",
				__func__, rc);
			goto done;
		}
#ifdef CONFIG_SENSORS_HRM_MAX86915
		gpio_set_value(data->hrm_en, 0);
		rc = gpio_direction_input(data->hrm_en);
		if (rc) {
			HRM_dbg("%s - gpio direction input failed, rc=%d\n",
				__func__, rc);
		}
#else
		rc = regulator_disable(regulator_led_3p3);
		if (rc) {
			HRM_dbg("%s - disable led_3p3 failed, rc=%d\n",
				__func__, rc);
			goto done;
		}
#endif
#ifdef I2C_1P8_DISABLE
		if (data->i2c_1p8 != NULL) {
			rc = regulator_disable(regulator_i2c_1p8);
			i2c_1p8_enable = 0;
			if (rc) {
				HRM_dbg("disable i2c_1p8 failed, rc=%d\n", rc);
				goto done;
			}
		}
#endif
	}

	goto done;

#ifdef CONFIG_SENSORS_HRM_MAX86915
gpio_direction_output_failed:
#else
enable_led_3p3_failed:
#endif
	regulator_disable(regulator_vdd_1p8);
enable_vdd_1p8_failed:
#ifdef I2C_1P8_DISABLE
	if (data->i2c_1p8 != NULL) {
		regulator_disable(regulator_i2c_1p8);
		i2c_1p8_enable = 0;
	}
#endif
enable_i2c_1p8_failed:
done:
#ifndef CONFIG_SENSORS_HRM_MAX86915
	regulator_put(regulator_led_3p3);
get_led_3p3_failed:
#endif
	regulator_put(regulator_vdd_1p8);
get_vdd_1p8_failed:
	if (data->i2c_1p8 != NULL)
		regulator_put(regulator_i2c_1p8);
get_i2c_1p8_failed:
	return rc;

}

static int hrm_init_device(struct hrm_device_data *data)
{
	int err;

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	err = data->h_func->init_device(data->hrm_i2c_client);
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		return err;
	}

	return err;
}

static int hrm_deinit_device(struct hrm_device_data *data)
{
	int err;

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	err = data->h_func->deinit_device();
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		return err;
	}
	return err;
}

static int hrm_enable(struct hrm_device_data *data, enum hrm_mode mode)
{
	int err;
	s32 threshold;

	HRM_dbg("%s - debug = %d, info = %d\n", __func__, hrm_debug, hrm_info);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	err = data->h_func->get_threshold(&threshold);
	if (err < 0) {
		HRM_dbg("%s get_threshold fail err = %d\n",
			__func__, err);
		return err;
	}
	if (data->hrm_threshold != DEFAULT_THRESHOLD) {
		if (threshold != data->hrm_threshold) {
			err = data->h_func->set_threshold(data->hrm_threshold);
			if (err < 0) {
				HRM_dbg("%s set_threshold fail err = %d\n",
					__func__, err);
				return err;
			}
		}
	} else
		data->hrm_threshold = threshold;

	err = data->h_func->enable(mode);
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		return err;
	}

	return 0;
}

static int hrm_disable(struct hrm_device_data *data, enum hrm_mode mode)
{
	int err;

	HRM_dbg("%s\n", __func__);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	err = data->h_func->disable(mode);
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		return err;
	}

	return 0;
}

static int hrm_read_data(struct hrm_device_data *data,
	struct hrm_output_data *read_data)
{
	int err = 0;

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	err = data->h_func->read_data(read_data);
	if (err < 0) {
		if (err != -ENODATA)
			HRM_dbg("%s read_data fail err = %d\n", __func__, err);

		return err;
	}

	return err;
}

static int hrm_eol_test_onoff(struct hrm_device_data *data, int onoff)
{
	int err;

	HRM_dbg("%s: %d\n", __func__, onoff);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}

	if (onoff == HRM_ON) {
		data->eol_test_is_enable = 1;

		err = data->h_func->set_eol_enable(data->eol_test_is_enable);
		if (err < 0) {
			HRM_dbg("%s fail err = %d\n", __func__, err);
			return err;
		}
	} else {
		data->eol_test_is_enable = 0;

		err = data->h_func->set_eol_enable(data->eol_test_is_enable);
		if (err < 0) {
			HRM_dbg("%s fail err = %d\n", __func__, err);
			return err;
		}
	}
	return err;
}

static int hrm_pre_eol_test_onoff(struct hrm_device_data *data, int onoff)
{
	int err;

	HRM_dbg("%s: %d\n", __func__, onoff);

	if (data->h_func == NULL || data->h_func->set_eol_enable == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}

	if (onoff == HRM_OFF) {
		data->pre_eol_test_is_enable = 0;

		err = data->h_func->set_pre_eol_enable(onoff);
		if (err < 0) {
			HRM_dbg("%s fail err = %d\n", __func__, err);
			return err;
		}
	} else {
		data->pre_eol_test_is_enable = onoff;

		err = data->h_func->set_pre_eol_enable(onoff);
		if (err < 0) {
			HRM_dbg("%s fail err = %d\n", __func__, err);
			return err;
		}
	}

	return err;
}

void hrm_mode_enable(struct hrm_device_data *data,
	int onoff, enum hrm_mode mode)
{
	int err;

	if (onoff == HRM_ON) {
		hrm_irq_set_state(data, HRM_ON);

		err = hrm_power_ctrl(data, HRM_ON);
		if (err < 0)
			HRM_dbg("%s hrm_regulator_on fail err = %d\n",
				__func__, err);
		data->hrm_enabled_mode = mode;
		err = hrm_enable(data, mode);
		if (err != 0)
			HRM_dbg("enable err : %d\n", err);
		if (err < 0 && mode == MODE_AMBIENT) {
			input_report_rel(data->hrm_input_dev,
				REL_Y, -5 + 1); /* F_ERR_I2C -5 detected i2c error */
			input_sync(data->hrm_input_dev);
			HRM_dbg("%s - awb mode enable error\n",
				__func__);
		}
	} else {
		if (data->regulator_state == 0) {
			HRM_dbg("%s - already power off - disable skip\n",
				__func__);
			return;
		}
		if (data->eol_test_is_enable)
			hrm_eol_test_onoff(data, HRM_OFF);

		data->hrm_enabled_mode = 0;
		data->mode_sdk_enabled = 0;
		err = hrm_disable(data, mode);
		if (err != 0)
			HRM_dbg("disable err : %d\n", err);
		err = hrm_power_ctrl(data, HRM_OFF);
		if (err < 0)
			HRM_dbg("%s hrm_regulator_off fail err = %d\n",
				__func__, err);
		hrm_irq_set_state(data, HRM_OFF);
	}
	HRM_dbg("%s - onoff = %d m : %d c : %d\n",
		__func__, onoff, mode, data->hrm_enabled_mode);

}

/* hrm input enable/disable sysfs */
static ssize_t hrm_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, strlen(buf), "%d\n", data->hrm_enabled_mode);
}

static ssize_t hrm_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int on_off;
	enum hrm_mode mode;

	mutex_lock(&data->activelock);
	if (sysfs_streq(buf, "0")) {
		on_off = HRM_OFF;
		mode = MODE_NONE;
	} else if (sysfs_streq(buf, "1")) {
		on_off = HRM_ON;
		mode = MODE_HRM;
		data->mode_cnt.hrm_cnt++;
	} else if (sysfs_streq(buf, "2")) {
		on_off = HRM_ON;
		mode = MODE_AMBIENT;
		data->mode_cnt.amb_cnt++;
	} else if (sysfs_streq(buf, "3")) {
		on_off = HRM_ON;
		mode = MODE_PROX;
		data->mode_cnt.prox_cnt++;
	} else if (sysfs_streq(buf, "4")) {
		on_off = HRM_ON;
		mode = MODE_HRMLED_IR;
	} else if (sysfs_streq(buf, "5")) {
		on_off = HRM_ON;
		mode = MODE_HRMLED_RED;
	} else if (sysfs_streq(buf, "6")) {
		on_off = HRM_ON;
		mode = MODE_HRMLED_BOTH;
	} else if (sysfs_streq(buf, "10")) {
		on_off = HRM_ON;
		mode = MODE_SDK_IR;
		data->mode_cnt.sdk_cnt++;
	} else if (sysfs_streq(buf, "11")) {
		on_off = HRM_ON;
		mode = MODE_SDK_RED;
	} else if (sysfs_streq(buf, "12")) {
		on_off = HRM_ON;
		mode = MODE_SDK_GREEN;
	} else if (sysfs_streq(buf, "13")) {
		on_off = HRM_ON;
		mode = MODE_SDK_BLUE;
	} else if (sysfs_streq(buf, "-10")) {
		on_off = HRM_OFF;
		mode = MODE_SDK_IR;
	} else if (sysfs_streq(buf, "-11")) {
		on_off = HRM_OFF;
		mode = MODE_SDK_RED;
	} else if (sysfs_streq(buf, "-12")) {
		on_off = HRM_OFF;
		mode = MODE_SDK_GREEN;
	} else if (sysfs_streq(buf, "-13")) {
		on_off = HRM_OFF;
		mode = MODE_SDK_BLUE;
	} else {
		HRM_dbg("%s - invalid value %d\n", __func__, *buf);
		data->mode_cnt.unkn_cnt++;
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}

	if (mode == MODE_SDK_IR || mode == MODE_SDK_RED
		|| mode == MODE_SDK_GREEN || mode == MODE_SDK_BLUE) {
		HRM_dbg("%s - Check SDK enable %d, %d, %d\n", __func__, on_off, mode, data->mode_sdk_enabled);
		if (on_off == HRM_ON) {
			if (data->mode_sdk_enabled & (1<<(mode - MODE_SDK_IR))) { /* already enabled */
				/* Do Nothing */
				HRM_dbg("%s - SDK mode already enabled %d\n", __func__, mode);
				mutex_unlock(&data->activelock);
				return count;
			} else {
				/* Oring enable mode */
				if (data->h_func->set_sdk_enabled != NULL)
					data->h_func->set_sdk_enabled((mode - MODE_SDK_IR), HRM_ON);
				if (data->mode_sdk_enabled == 0) {
					data->mode_sdk_enabled |= (1<<(mode - MODE_SDK_IR));
					mode = MODE_SDK_IR;
					data->mode_cnt.sdk_cnt++;
				} else {
					data->mode_sdk_enabled |= (1<<(mode - MODE_SDK_IR));
					mutex_unlock(&data->activelock);
					return count;
				}
			}
		} else {
			if ((data->mode_sdk_enabled & (1<<(mode - MODE_SDK_IR))) == 0) { /* Not Enabled */
				/* Do Nothing */
				HRM_dbg("%s - SDK mode not enabled %d\n", __func__, mode);
				mutex_unlock(&data->activelock);
				return count;
			} else {
				/* Oring disable mode */
				data->mode_sdk_enabled &= ~(1<<(mode - MODE_SDK_IR));
				if (data->h_func->set_sdk_enabled != NULL)
					data->h_func->set_sdk_enabled((mode - MODE_SDK_IR), HRM_OFF);
				if (data->mode_sdk_enabled == 0) {
					mode = MODE_NONE;
				} else {
					mutex_unlock(&data->activelock);
					return count;
				}
			}
		}
		HRM_dbg("%s - Current SDK Mode %02x\n", __func__, data->mode_sdk_enabled);
	}

	HRM_dbg("%s en : %d m : %d c : %d\n",
		__func__, on_off, mode, data->hrm_enabled_mode);
	hrm_mode_enable(data, on_off, mode);
	mutex_unlock(&data->activelock);

	return count;
}

static ssize_t hrm_poll_delay_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->sampling_period_ns);
}

static ssize_t hrm_poll_delay_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	u32 sampling_period_ns = 0;
	int err = 0;

	mutex_lock(&data->activelock);

	err = kstrtoint(buf, 10, &sampling_period_ns);

	if (err < 0) {
		HRM_dbg("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}

	err = data->h_func->set_sampling_rate(sampling_period_ns);

	if (err > 0)
		data->sampling_period_ns = err;

	HRM_dbg("%s - hrm sensor sampling rate is setted as %dns\n", __func__, sampling_period_ns);

	mutex_unlock(&data->activelock);

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
	hrm_enable_show, hrm_enable_store);
static DEVICE_ATTR(poll_delay, S_IRUGO|S_IWUSR|S_IWGRP,
	hrm_poll_delay_show, hrm_poll_delay_store);

static struct attribute *hrm_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group hrm_attribute_group = {
	.attrs = hrm_sysfs_attrs,
};

/* hrm_sensor sysfs */
static ssize_t hrm_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	char chip_name[NAME_LEN];

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	data->h_func->get_name_chipset(chip_name);
	return snprintf(buf, PAGE_SIZE, "%s\n", chip_name);
}

static ssize_t hrm_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	char vendor[NAME_LEN];

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	data->h_func->get_name_vendor(vendor);
	return snprintf(buf, PAGE_SIZE, "%s\n", vendor);
}

static ssize_t hrm_led_current_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int err;
	int led1, led2, led3, led4;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	err = sscanf(buf, "%8x", &data->led_current);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	led1 = 0x000000ff & data->led_current;
	led2 = (0x0000ff00 & data->led_current) >> 8;
	led3 = (0x00ff0000 & data->led_current) >> 16;
	led4 = (0xff000000 & data->led_current) >> 24;
	HRM_info("%s 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", __func__, led1, led2, led3, led4);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	err = data->h_func->set_led_current(led1, led2, led3, led4);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t hrm_led_current_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int err;
	u8 led1, led2, led3, led4;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	err = data->h_func->get_led_current(&led1, &led2, &led3, &led4);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	data->led_current = (led1 & 0xff) | ((led2 & 0xff) << 8)
		| ((led3 & 0xff) << 16) | ((led4 & 0xff) << 24);

	mutex_unlock(&data->activelock);
	HRM_info("%s led1 0x%02x, led2 0x%02x, led3 0x%02x, led4 0x%02x\n",
		__func__, led1, led2, led3, led4);

	return snprintf(buf, PAGE_SIZE, "%08X\n", data->led_current);
}

static ssize_t hrm_led_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int err;
	u8 result;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	err = hrm_power_ctrl(data, HRM_ON);
	if (err < 0)
		HRM_dbg("%s hrm_regulator_on fail err = %d\n",
		__func__, err);

	mutex_lock(&data->activelock);

	if (data->h_func == NULL || data->h_func->get_led_test == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	err = data->h_func->get_led_test(&result);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	mutex_unlock(&data->activelock);

	err = hrm_power_ctrl(data, HRM_OFF);
	if (err < 0)
		HRM_dbg("%s hrm_regulator_off fail err = %d\n",
		__func__, err);

	HRM_info("%s result = 0x%02x\n", __func__, result);

	return snprintf(buf, PAGE_SIZE, "0x%02x\n", result);
}

static ssize_t hrm_flush_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int ret = 0;
	u8 handle = 0;

	mutex_lock(&data->activelock);
	ret = kstrtou8(buf, 10, &handle);
	if (ret < 0) {
		HRM_dbg("%s - kstrtou8 failed.(%d)\n", __func__, ret);
		mutex_unlock(&data->activelock);
		return ret;
	}
	HRM_dbg("%s - handle = %d\n", __func__, handle);
	mutex_unlock(&data->activelock);

	input_report_rel(data->hrm_input_dev, REL_MISC, handle);

	return size;
}

static ssize_t hrm_int_pin_check(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	/* need to check if this should be implemented */
	HRM_dbg("%s\n", __func__);
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t hrm_lib_ver_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	size_t buf_len;

	mutex_lock(&data->activelock);
	buf_len = strlen(buf) + 1;
	if (buf_len > NAME_LEN)
		buf_len = NAME_LEN;

	if (data->lib_ver != NULL)
		kfree(data->lib_ver);

	data->lib_ver = kzalloc(sizeof(char) * buf_len, GFP_KERNEL);
	if (data->lib_ver == NULL) {
		HRM_dbg("%s - couldn't allocate memory\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENOMEM;
	}
	strlcpy(data->lib_ver, buf, buf_len);

	HRM_info("%s - lib_ver = %s\n", __func__, data->lib_ver);
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t hrm_lib_ver_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	if (data->lib_ver == NULL) {
		HRM_dbg("%s - data->lib_ver is NULL\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%s\n", "NULL");
	}
	HRM_info("%s - lib_ver = %s\n", __func__, data->lib_ver);
	return snprintf(buf, PAGE_SIZE, "%s\n", data->lib_ver);
}

static ssize_t hrm_threshold_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &data->hrm_threshold);
	if (err < 0) {
		HRM_dbg("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	HRM_info("%s - threshold = %d\n",
		__func__, data->hrm_threshold);

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t hrm_threshold_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	if (data->hrm_threshold) {
		HRM_info("%s - threshold = %d\n",
			__func__, data->hrm_threshold);
		return snprintf(buf, PAGE_SIZE, "%d\n", data->hrm_threshold);
	} else {
		HRM_info("%s - threshold = 0\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d\n", 0);
	}
}

static ssize_t prox_thd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &data->prox_threshold);
	if (err < 0) {
		HRM_dbg("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	HRM_info("%s - prox threshold = %d\n",
		__func__, data->prox_threshold);

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t prox_thd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	if (data->prox_threshold) {
		HRM_info("%s - prox threshold = %d\n",
			__func__, data->prox_threshold);
		return snprintf(buf, PAGE_SIZE, "%d\n", data->prox_threshold);
	} else {
		HRM_info("%s - prox threshold = 0\n", __func__);
		return snprintf(buf, PAGE_SIZE, "%d\n", 0);
	}
}

static ssize_t hrm_eol_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int test_onoff;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	if (sysfs_streq(buf, "1")) { /* eol_test start */
		test_onoff = 1;
	} else if (sysfs_streq(buf, "0")) { /* eol_test stop */
		test_onoff = 0;
		hrm_pre_eol_test_onoff(data, test_onoff);
	} else {
		HRM_dbg("%s: invalid value %d\n", __func__, *buf);
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}
	HRM_dbg("%s: %d\n", __func__, test_onoff);
	if (data->eol_test_is_enable == test_onoff) {
		HRM_dbg("%s: invalid eol status Pre: %d, AF : %d\n", __func__,
			data->eol_test_is_enable, test_onoff);
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}
	if (hrm_eol_test_onoff(data, test_onoff) < 0)
		data->eol_test_is_enable = 0;

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t hrm_eol_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->eol_test_is_enable);
}

static ssize_t hrm_eol_test_result_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	static char eol_result[MAX_BUF_LEN];
	int err;

	mutex_lock(&data->activelock);

	if (data->eol_test_status == 0) {
		HRM_dbg("%s - data->eol_test_status is NULL\n",
		__func__);
		data->eol_test_status = 0;
		mutex_unlock(&data->activelock);
		return snprintf(buf, PAGE_SIZE, "%s\n", "NO_EOL_TEST");
	}
	HRM_dbg("%s - result = %d\n", __func__, data->eol_test_status);
	data->eol_test_status = 0;

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	err = data->h_func->get_eol_result(eol_result);
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}

	mutex_unlock(&data->activelock);

	return snprintf(buf, PAGE_SIZE, "%s\n", eol_result);
}

static ssize_t hrm_eol_test_status_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err;

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		return -ENODEV;
	}
	err = data->h_func->get_eol_status((u8 *)&data->eol_test_status);
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		return err;
	}
	return snprintf(buf, PAGE_SIZE, "%u\n", data->eol_test_status);
}

static ssize_t hrm_pre_eol_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int test_onoff = 0;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	if (sysfs_streq(buf, "1")) { /* PRE EOL SYSTEM NOISE */
		test_onoff = 1;
	} else if (sysfs_streq(buf, "2")) { /* PRE EOL FREQ */
		test_onoff = 2;
	} else if (sysfs_streq(buf, "3")) { /* PRE EOL BOTH */
		test_onoff = 3;
	} else if (sysfs_streq(buf, "0")) { /* pre_eol_test stop */
		test_onoff = 0;
	} else {
		HRM_dbg("%s: invalid value %d\n", __func__, *buf);
		mutex_unlock(&data->activelock);
		return -EINVAL;
	}
	HRM_dbg("%s: %d\n", __func__, test_onoff);

	hrm_pre_eol_test_onoff(data, test_onoff);

	mutex_unlock(&data->activelock);
	return size;
}

static ssize_t hrm_pre_eol_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->pre_eol_test_is_enable);
}


static ssize_t hrm_read_reg_get(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	HRM_info("%s - val=0x%06x\n", __func__, data->reg_read_buf);

	return snprintf(buf, PAGE_SIZE, "%d\n", data->reg_read_buf);
}

static ssize_t hrm_read_reg_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	int err = -1;
	unsigned int cmd = 0;
	u32 val = 0;
	u32 sz = 0;

	mutex_lock(&data->i2clock);
	if (data->regulator_state == 0) {
		HRM_dbg("%s - need to power on\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}
	err = sscanf(buf, "%8x", &cmd);
	if (err == 0) {
		HRM_dbg("%s - sscanf fail\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->i2clock);
		return -ENODEV;
	}
	err = data->h_func->i2c_read((u32)cmd, &val, &sz);
	if (err != 0) {
		HRM_dbg("%s err=%d, val=0x%06x\n",
			__func__, err, val);
		mutex_unlock(&data->i2clock);
		return size;
	}
	data->reg_read_buf = val;
	mutex_unlock(&data->i2clock);

	return size;
}
static ssize_t hrm_write_reg_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	int err = -1;
	unsigned int cmd = 0;
	unsigned int val = 0;

	mutex_lock(&data->i2clock);
	if (data->regulator_state == 0) {
		HRM_dbg("%s - need to power on.\n", __func__);
		mutex_unlock(&data->i2clock);
		return size;
	}
	err = sscanf(buf, "%8x, %8x", &cmd, &val);
	if (err == 0) {
		HRM_dbg("%s - sscanf fail %s\n", __func__, buf);
		mutex_unlock(&data->i2clock);
		return size;
	}

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->i2clock);
		return -ENODEV;
	}
	err = data->h_func->i2c_write((u32)cmd, (u32)val);
	if (err < 0) {
		HRM_dbg("%s fail err = %d\n", __func__, err);
		mutex_unlock(&data->i2clock);
		return err;
	}
	mutex_unlock(&data->i2clock);

	return size;
}

static ssize_t hrm_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	HRM_info("%s - debug mode = %u\n", __func__, data->debug_mode);

	return snprintf(buf, PAGE_SIZE, "%u\n", data->debug_mode);
}

static ssize_t hrm_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err;
	s32 mode;

	mutex_lock(&data->activelock);
	err = kstrtoint(buf, 10, &mode);
	if (err < 0) {
		HRM_dbg("%s - kstrtoint failed.(%d)\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	HRM_info("%s - mode = %d\n", __func__, mode);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	data->debug_mode = (u8)mode;
	data->h_func->hrm_debug_set((u8)mode);
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t device_id_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err;
	u64 device_id = 0;

	err = hrm_power_ctrl(data, HRM_ON);
	if (err < 0)
		HRM_dbg("%s hrm_regulator_on fail err = %d\n",
		__func__, err);

	mutex_lock(&data->activelock);

	data->h_func->get_chipid(&device_id);

	mutex_unlock(&data->activelock);

	err = hrm_power_ctrl(data, HRM_OFF);
	if (err < 0)
		HRM_dbg("%s hrm_regulator_off fail err = %d\n",
		__func__, err);

	return snprintf(buf, PAGE_SIZE, "%lld\n", device_id);
}

static ssize_t part_type_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err;
	u16 part_type = 0;

	err = hrm_power_ctrl(data, HRM_ON);
	if (err < 0)
		HRM_dbg("%s hrm_regulator_on fail err = %d\n",
		__func__, err);

	mutex_lock(&data->activelock);

	data->h_func->get_part_type(&part_type);

	mutex_unlock(&data->activelock);

	err = hrm_power_ctrl(data, HRM_OFF);
	if (err < 0)
		HRM_dbg("%s hrm_regulator_off fail err = %d\n",
		__func__, err);

	return snprintf(buf, PAGE_SIZE, "%d\n", part_type);
}

static ssize_t i2c_err_cnt_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	u32 err_cnt = 0;

	data->h_func->get_i2c_err_cnt(&err_cnt);

	return snprintf(buf, PAGE_SIZE, "%d\n", err_cnt);
}

static ssize_t i2c_err_cnt_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	data->h_func->set_i2c_err_cnt();

	return size;
}

static ssize_t curr_adc_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	u16 ir_curr = 0;
	u16 red_curr = 0;
	u32 ir_adc = 0;
	u32 red_adc = 0;

	data->h_func->get_curr_adc(&ir_curr, &red_curr, &ir_adc, &red_adc);

	return snprintf(buf, PAGE_SIZE,
		"\"HRIC\":\"%d\",\"HRRC\":\"%d\",\"HRIA\":\"%d\",\"HRRA\":\"%d\"\n",
		ir_curr, red_curr, ir_adc, red_adc);
}

static ssize_t curr_adc_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	data->h_func->set_curr_adc();

	return size;
}

static ssize_t mode_cnt_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE,
		"\"CNT_HRM\":\"%d\",\"CNT_AMB\":\"%d\",\"CNT_PROX\":\"%d\",\"CNT_SDK\":\"%d\",\"CNT_CGM\":\"%d\",\"CNT_UNKN\":\"%d\"\n",
		data->mode_cnt.hrm_cnt, data->mode_cnt.amb_cnt, data->mode_cnt.prox_cnt,
		data->mode_cnt.sdk_cnt, data->mode_cnt.cgm_cnt, data->mode_cnt.unkn_cnt);
}

static ssize_t mode_cnt_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);

	data->mode_cnt.hrm_cnt = 0;
	data->mode_cnt.amb_cnt = 0;
	data->mode_cnt.prox_cnt = 0;
	data->mode_cnt.sdk_cnt = 0;
	data->mode_cnt.cgm_cnt = 0;
	data->mode_cnt.unkn_cnt = 0;

	return size;
}

static ssize_t hrm_factory_cmd_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	static char cmd_result[MAX_BUF_LEN];

	mutex_lock(&data->activelock);

	data->h_func->get_fac_cmd(cmd_result);

	HRM_dbg("%s cmd_result = %s\n", __func__, cmd_result);

	mutex_unlock(&data->activelock);

	return snprintf(buf, PAGE_SIZE, "%s\n", cmd_result);
}

static ssize_t hrm_version_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	static char version[MAX_BUF_LEN];

	mutex_lock(&data->activelock);

	data->h_func->get_version(version);

	HRM_dbg("%s cmd_result = %s.%s\n", __func__, VERSION, version);

	mutex_unlock(&data->activelock);

	return snprintf(buf, PAGE_SIZE, "%s.%s\n", VERSION, version);
}

static ssize_t hrm_sensor_info_show(struct device *dev,
struct device_attribute *attr, char *buf)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	static char sensor_info_data[MAX_BUF_LEN];

	if (data->h_func->get_sensor_info != NULL) {
		data->h_func->get_sensor_info(sensor_info_data);
		HRM_dbg("%s sensor_info_data = %s\n", __func__, sensor_info_data);

		return snprintf(buf, PAGE_SIZE, "%s\n", sensor_info_data);

	} else {
		HRM_dbg("%s sensor_info_data not support\n", __func__);

		return snprintf(buf, PAGE_SIZE, "NOT SUPPORT\n");
	}
}

static ssize_t hrm_xtalk_code_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int err;
	int xtalk1, xtalk2, xtalk3, xtalk4;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);
	err = sscanf(buf, "%8x", &data->xtalk_code);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	xtalk1 = 0x0000001f & data->xtalk_code;
	xtalk2 = (0x00001f00 & data->xtalk_code) >> 8;
	xtalk3 = (0x001f0000 & data->xtalk_code) >> 16;
	xtalk4 = (0x1f000000 & data->xtalk_code) >> 24;
	HRM_info("%s 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", __func__, xtalk1, xtalk2, xtalk3, xtalk4);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	if (data->h_func->set_xtalk_code == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	err = data->h_func->set_xtalk_code(xtalk1, xtalk2, xtalk3, xtalk4);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	mutex_unlock(&data->activelock);

	return size;
}

static ssize_t hrm_xtalk_code_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int err;
	u8 xtalk1, xtalk2, xtalk3, xtalk4;
	struct hrm_device_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->activelock);

	if (data->h_func == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	if (data->h_func->get_xtalk_code == NULL) {
		HRM_dbg("%s - not mapped function\n", __func__);
		mutex_unlock(&data->activelock);
		return -ENODEV;
	}
	err = data->h_func->get_xtalk_code(&xtalk1, &xtalk2, &xtalk3, &xtalk4);
	if (err < 0) {
		HRM_dbg("%s - failed, err = %x\n", __func__, err);
		mutex_unlock(&data->activelock);
		return err;
	}
	data->xtalk_code = (xtalk1 & 0x1f) | ((xtalk2 & 0x1f) << 8)
		| ((xtalk3 & 0x1f) << 16) | ((xtalk4 & 0x1f) << 24);

	mutex_unlock(&data->activelock);
	HRM_info("%s xtalk1 0x%02x, xtalk2 0x%02x, xtalk3 0x%02x, xtalk4 0x%02x\n",
		__func__, xtalk1, xtalk2, xtalk3, xtalk4);

	return snprintf(buf, PAGE_SIZE, "%08X\n", data->xtalk_code);
}

static DEVICE_ATTR(name, S_IRUGO, hrm_name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, hrm_vendor_show, NULL);
static DEVICE_ATTR(led_current, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_led_current_show, hrm_led_current_store);
static DEVICE_ATTR(led_test, S_IRUGO, hrm_led_test_show, NULL);
static DEVICE_ATTR(hrm_flush, S_IWUSR | S_IWGRP, NULL, hrm_flush_store);
static DEVICE_ATTR(int_pin_check, S_IRUGO, hrm_int_pin_check, NULL);
static DEVICE_ATTR(lib_ver, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_lib_ver_show, hrm_lib_ver_store);
static DEVICE_ATTR(threshold, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_threshold_show, hrm_threshold_store);
static DEVICE_ATTR(prox_thd, S_IRUGO | S_IWUSR | S_IWGRP,
	prox_thd_show, prox_thd_store);
static DEVICE_ATTR(eol_test, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_eol_test_show, hrm_eol_test_store);
static DEVICE_ATTR(eol_test_result, S_IRUGO, hrm_eol_test_result_show, NULL);
static DEVICE_ATTR(eol_test_status, S_IRUGO, hrm_eol_test_status_show, NULL);
static DEVICE_ATTR(pre_eol_test, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_pre_eol_test_show, hrm_pre_eol_test_store);
static DEVICE_ATTR(read_reg, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_read_reg_get, hrm_read_reg_set);
static DEVICE_ATTR(write_reg, S_IWUSR | S_IWGRP, NULL, hrm_write_reg_store);
static DEVICE_ATTR(hrm_debug, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_debug_show, hrm_debug_store);
static DEVICE_ATTR(device_id, S_IRUGO, device_id_show, NULL);
static DEVICE_ATTR(part_type, S_IRUGO, part_type_show, NULL);
static DEVICE_ATTR(i2c_err_cnt, S_IRUGO | S_IWUSR | S_IWGRP, i2c_err_cnt_show, i2c_err_cnt_store);
static DEVICE_ATTR(curr_adc, S_IRUGO | S_IWUSR | S_IWGRP, curr_adc_show, curr_adc_store);
static DEVICE_ATTR(mode_cnt, S_IRUGO | S_IWUSR | S_IWGRP, mode_cnt_show, mode_cnt_store);
static DEVICE_ATTR(hrm_factory_cmd, S_IRUGO, hrm_factory_cmd_show, NULL);
static DEVICE_ATTR(hrm_version, S_IRUGO, hrm_version_show, NULL);
static DEVICE_ATTR(sensor_info, S_IRUGO, hrm_sensor_info_show, NULL);
static DEVICE_ATTR(xtalk_code, S_IRUGO | S_IWUSR | S_IWGRP,
	hrm_xtalk_code_show, hrm_xtalk_code_store);

static struct device_attribute *hrm_sensor_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_led_current,
	&dev_attr_led_test,
	&dev_attr_hrm_flush,
	&dev_attr_int_pin_check,
	&dev_attr_lib_ver,
	&dev_attr_threshold,
	&dev_attr_prox_thd,
	&dev_attr_eol_test,
	&dev_attr_eol_test_result,
	&dev_attr_eol_test_status,
	&dev_attr_pre_eol_test,
	&dev_attr_read_reg,
	&dev_attr_write_reg,
	&dev_attr_device_id,
	&dev_attr_part_type,
	&dev_attr_i2c_err_cnt,
	&dev_attr_curr_adc,
	&dev_attr_mode_cnt,
	&dev_attr_hrm_debug,
	&dev_attr_hrm_factory_cmd,
	&dev_attr_hrm_version,
	&dev_attr_sensor_info,
	&dev_attr_xtalk_code,
	NULL,
};

void hrm_pin_control(struct hrm_device_data *data, bool pin_set)
{
	int status = 0;

	if (!data->hrm_pinctrl) {
		HRM_dbg("%s hrm_pinctrl is null\n", __func__);
		return;
	}
	if (pin_set) {
		if (!IS_ERR_OR_NULL(data->pins_idle)) {
			status = pinctrl_select_state(data->hrm_pinctrl,
				data->pins_idle);
			if (status)
				HRM_dbg("%s: can't set pin default state\n",
					__func__);
			HRM_info("%s idle\n", __func__);
		}
#ifdef CONFIG_SENSORS_HRM_MAX86915
		if (data->hrm_prev_mode != 0) {
			status = gpio_direction_output(data->hrm_en, 1);
			if (status) {
				HRM_dbg("%s - gpio direction output failed, rc=%d\n",
					__func__, status);
			}
		}
#endif
	} else {
		if (!IS_ERR_OR_NULL(data->pins_sleep)) {
			status = pinctrl_select_state(data->hrm_pinctrl,
				data->pins_sleep);
			if (status)
				HRM_dbg("%s: can't set pin sleep state\n",
					__func__);
			HRM_info("%s sleep\n", __func__);
		}
	}
}

irqreturn_t hrm_irq_handler(int hrm_irq, void *device)
{
	int err;
	struct hrm_device_data *data = device;
	struct hrm_output_data read_data;
	int i;
	static unsigned int sample_cnt;

	memset(&read_data, 0, sizeof(struct hrm_output_data));

	if (data->regulator_state == 0 || data->hrm_enabled_mode == 0) {
		HRM_dbg("%s - return IRQ_HANDLED (reg_state : %d, hrm_mode : %d)\n",
				__func__, data->regulator_state, data->hrm_enabled_mode);
		return IRQ_HANDLED;
	}

#ifdef CONFIG_ARCH_QCOM
	pm_qos_add_request(&data->pm_qos_req_fpm, PM_QOS_CPU_DMA_LATENCY,
		PM_QOS_DEFAULT_VALUE);
#endif
	err = hrm_read_data(data, &read_data);

	if (err == 0) {
		if (data->hrm_input_dev == NULL) {
			HRM_dbg("%s - hrm_input_dev is NULL\n", __func__);
		} else {
			if (read_data.fifo_num) {
				for (i = 0; i < read_data.fifo_num; i++) {
					input_report_rel(data->hrm_input_dev,
						REL_X, read_data.fifo_main[0][i] + 1);
					input_report_rel(data->hrm_input_dev,
						REL_Y,  read_data.fifo_main[1][i] + 1);
					input_report_rel(data->hrm_input_dev,
						REL_Z, read_data.fifo_main[2][i] + 1);
					input_report_rel(data->hrm_input_dev,
						REL_RX,  read_data.fifo_main[3][i] + 1);
					input_sync(data->hrm_input_dev);
				}
			} else {
					for (i = 0; i < read_data.main_num; i++)
						input_report_rel(data->hrm_input_dev,
							REL_X + i, read_data.data_main[i] + 1);

					for (i = 0; i < read_data.sub_num; i++)
						input_report_abs(data->hrm_input_dev,
							ABS_X + i, read_data.data_sub[i] + 1);

					if (read_data.main_num || read_data.sub_num)
						input_sync(data->hrm_input_dev);
			}
			if (sample_cnt++ > 100) {
				HRM_dbg("%s mode:0x%x main:%d,%d,%d,%d sub:%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
						read_data.mode, read_data.data_main[0], read_data.data_main[1],
						read_data.data_main[2], read_data.data_main[3], read_data.data_sub[0],
						read_data.data_sub[1], read_data.data_sub[2], read_data.data_sub[3], read_data.data_sub[4],
						read_data.data_sub[5], read_data.data_sub[6], read_data.data_sub[7]);
				sample_cnt = 0;
			} else {
				HRM_info("%s mode:0x%x main:%d,%d,%d,%d sub:%d,%d,%d,%d,%d,%d,%d,%d\n", __func__,
						read_data.mode, read_data.data_main[0], read_data.data_main[1],
						read_data.data_main[2], read_data.data_main[3], read_data.data_sub[0],
						read_data.data_sub[1], read_data.data_sub[2], read_data.data_sub[3], read_data.data_sub[4],
						read_data.data_sub[5], read_data.data_sub[6], read_data.data_sub[7]);
			}
		}
	}
#ifdef CONFIG_ARCH_QCOM
	pm_qos_remove_request(&data->pm_qos_req_fpm);
#endif

	return IRQ_HANDLED;
}


static int hrm_parse_dt(struct hrm_device_data *data)
{
	struct device *dev = &data->hrm_i2c_client->dev;
	struct device_node *dNode = dev->of_node;
	enum of_gpio_flags flags;
	u32 threshold[2];

	if (dNode == NULL)
		return -ENODEV;

	data->hrm_int = of_get_named_gpio_flags(dNode,
		"hrmsensor,hrm_int-gpio", 0, &flags);
	if (data->hrm_int < 0) {
		HRM_dbg("%s - get hrm_int error\n", __func__);
		return -ENODEV;
	}

#ifdef	CONFIG_SENSORS_HRM_MAX86915
	data->hrm_en = of_get_named_gpio_flags(dNode,
	"hrmsensor,hrm_boost_en-gpio", 0, &flags);
	if (data->hrm_en < 0) {
		HRM_dbg("%s - get hrm_en error\n", __func__);
		return -ENODEV;
	}
#endif

#ifdef CONFIG_ARCH_QCOM
	data->hrm_pinctrl = devm_pinctrl_get(dev);
#else
	if (of_property_read_string(dNode, "hrmsensor,vdd_1p8",
		(char const **)&data->vdd_1p8) < 0)
		HRM_dbg("%s - get vdd_1p8 error\n", __func__);

#ifndef CONFIG_SENSORS_HRM_MAX86915
	if (of_property_read_string(dNode, "hrmsensor,led_3p3",
		(char const **)&data->led_3p3) < 0)
		HRM_dbg("%s - get led_3p3 error\n", __func__);
#endif

	if (of_property_read_string(dNode, "hrmsensor,i2c_1p8",
		(char const **)&data->i2c_1p8) < 0)
		HRM_dbg("%s - don't use i2c_1p8\n", __func__);

	data->hrm_pinctrl = pinctrl_get_select_default(dev);
#endif

	if (IS_ERR_OR_NULL(data->hrm_pinctrl)) {
		HRM_dbg("%s: failed pinctrl_get (%li)\n",
			__func__, PTR_ERR(data->hrm_pinctrl));
		data->hrm_pinctrl = NULL;
		return -EINVAL;
	} else
		HRM_dbg("%s: success pinctrl_get (%p)\n",
			__func__, data->hrm_pinctrl);

	data->pins_sleep =
		pinctrl_lookup_state(data->hrm_pinctrl, "sleep");
	if (IS_ERR_OR_NULL(data->pins_sleep)) {
		HRM_dbg("%s : could not get pins sleep_state (%li)\n",
			__func__, PTR_ERR(data->pins_sleep));
#ifdef CONFIG_ARCH_QCOM
		devm_pinctrl_put(data->hrm_pinctrl);
#else
		pinctrl_put(data->hrm_pinctrl);
#endif
		data->pins_sleep = NULL;
		return -EINVAL;
	}

	data->pins_idle =
		pinctrl_lookup_state(data->hrm_pinctrl, "idle");
	if (IS_ERR_OR_NULL(data->pins_idle)) {
		HRM_dbg("%s : could not get pins idle_state (%li)\n",
			__func__, PTR_ERR(data->pins_idle));

#ifdef CONFIG_ARCH_QCOM
		devm_pinctrl_put(data->hrm_pinctrl);
#else
		pinctrl_put(data->hrm_pinctrl);
#endif
		data->pins_idle = NULL;
		return -EINVAL;
	}

	if (of_property_read_u32_array(dNode, "hrmsensor,thd",
		threshold, ARRAY_SIZE(threshold)) < 0) {
		HRM_dbg("%s - no threshold in dt\n", __func__);
	} else {
		data->hrm_threshold = threshold[0];
		data->prox_threshold = threshold[1];
	}	

	HRM_dbg("%s - threshold = %d %d\n", __func__, data->hrm_threshold, data->prox_threshold);	

	if (of_property_read_u32_array(dNode, "hrmsensor,init_curr",
		data->init_current, ARRAY_SIZE(data->init_current)) < 0) {
		HRM_dbg("%s - no init_curr in dt\n", __func__);

		data->init_current[0] = 0;
		data->init_current[1] = 0;
		data->init_current[2] = 0;
		data->init_current[3] = 0;
	}

	HRM_dbg("%s - init_curr = 0x%.2x 0x%.2x 0x%.2x 0x%.2x \n", __func__,
		data->init_current[0], data->init_current[1],
		data->init_current[2], data->init_current[3]);

	return 0;
}
static int hrm_setup_irq(struct hrm_device_data *data)
{
	int errorno = -EIO;

	errorno = request_threaded_irq(data->hrm_irq, NULL,
		hrm_irq_handler, IRQF_TRIGGER_FALLING|IRQF_ONESHOT,
		"hrm_sensor_irq", data);

	if (errorno < 0) {
		HRM_dbg("%s - failed for setup hrm_irq errono= %d\n",
			   __func__, errorno);
		errorno = -ENODEV;
		return errorno;
	}

	disable_irq(data->hrm_irq);

	return errorno;
}

static int hrm_setup_gpio(struct hrm_device_data *data)
{
	int errorno = -EIO;

	errorno = gpio_request(data->hrm_int, "hrm_int");
	if (errorno) {
		HRM_dbg("%s - failed to request hrm_int\n", __func__);
		return errorno;
	}

	errorno = gpio_direction_input(data->hrm_int);
	if (errorno) {
		HRM_dbg("%s - failed to set hrm_int as input\n", __func__);
		goto err_gpio_direction_input;
	}
	data->hrm_irq = gpio_to_irq(data->hrm_int);

#ifdef CONFIG_SENSORS_HRM_MAX86915
	errorno = gpio_request(data->hrm_en, "hrm_en");
	if (errorno) {
		HRM_dbg("%s - failed to request hrm_en\n", __func__);
		goto err_gpio_direction_input;
	}
#endif
	goto done;

err_gpio_direction_input:
	gpio_free(data->hrm_int);
done:
	return errorno;
}

int hrm_set_func(struct hrm_device_data *data)
{
	int err = 0;
	char chip_name[NAME_LEN];

#if defined(CONFIG_SENSORS_HRM_MAX86902) || defined(CONFIG_SENSORS_HRM_MAX86915)
	if (data->hrm_i2c_client->addr == SLAVE_ADDR_MAX) {
		data->h_func = &max869_func;
		err = hrm_init_device(data);
		if (err == 0) {
			err = data->h_func->get_name_chipset(chip_name);
			HRM_info("%s - use %s\n", __func__, chip_name);
			return err;
		}
	}
#endif

	data->h_func = NULL;

	err = hrm_init_device(data);
	if (err < 0)
		HRM_dbg("%s - couldn't get func\n", __func__);
	return err;
}

int hrm_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -ENODEV;

	struct hrm_device_data *data;

	/* check to make sure that the adapter supports I2C */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		HRM_dbg("%s - I2C_FUNC_I2C not supported\n", __func__);
		return -ENODEV;
	}
	/* allocate some memory for the device */
	data = kzalloc(sizeof(struct hrm_device_data), GFP_KERNEL);
	if (data == NULL) {
		HRM_dbg("%s - couldn't allocate memory\n", __func__);
		return -ENOMEM;
	}

	hrm_init_device_data(data);

	data->hrm_i2c_client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->i2clock);
	mutex_init(&data->activelock);
	mutex_init(&data->suspendlock);

	HRM_dbg("%s - start\n", __func__);

	err = hrm_parse_dt(data);
	if (err < 0) {
		HRM_dbg("[SENSOR] %s - of_node error\n", __func__);
		err = -ENODEV;
		goto err_parse_dt;
	}
	err = hrm_setup_gpio(data);
	if (err) {
		HRM_dbg("%s - could not initialize resources\n", __func__);
		goto err_setup_gpio;
	}

	err = hrm_power_ctrl(data, HRM_ON);
	if (err < 0) {
		HRM_dbg("%s hrm_power_ctrl fail(%d, %d)\n", __func__,
			err, HRM_ON);
		goto err_power_on;
	}

	err = hrm_set_func(data);
	if (err < 0) {
		HRM_dbg("%s hrm_set_func fail(%d)\n", __func__,
			err);
		goto hrm_init_device_failed;
	}

	data->hrm_input_dev = input_allocate_device();
	if (!data->hrm_input_dev) {
		HRM_dbg("%s - could not allocate input device\n",
			__func__);
		goto err_input_allocate_device;
	}
	data->hrm_input_dev->name = MODULE_NAME_HRM;
	input_set_drvdata(data->hrm_input_dev, data);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_X);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_Y);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_Z);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_RX);
	input_set_capability(data->hrm_input_dev, EV_REL, REL_MISC);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_X);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_Y);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_Z);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RX);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RY);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RZ);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_THROTTLE);
	input_set_capability(data->hrm_input_dev, EV_ABS, ABS_RUDDER);

	err = input_register_device(data->hrm_input_dev);
	if (err < 0) {
		input_free_device(data->hrm_input_dev);
		HRM_dbg("%s - could not register input device\n", __func__);
		goto err_input_register_device;
	}
	err = sensors_create_symlink(data->hrm_input_dev);
	if (err < 0) {
		HRM_dbg("%s - create_symlink error\n", __func__);
		goto err_sensors_create_symlink;
	}
	err = sysfs_create_group(&data->hrm_input_dev->dev.kobj,
				 &hrm_attribute_group);
	if (err) {
		HRM_dbg("%s - could not create sysfs group\n",
			__func__);
		goto err_sysfs_create_group;
	}
#ifdef CONFIG_ARCH_QCOM
	err = sensors_register(&data->dev, data, hrm_sensor_attrs,
			MODULE_NAME_HRM);
#else
	err = sensors_register(data->dev, data, hrm_sensor_attrs,
			MODULE_NAME_HRM);
#endif
	if (err) {
		HRM_dbg("%s - cound not register hrm_sensor(%d).\n",
			__func__, err);
		goto hrm_sensor_register_failed;
	}

	err = hrm_setup_irq(data);
	if (err) {
		HRM_dbg("%s - could not setup hrm_irq\n", __func__);
		goto err_setup_irq;
	}

	err = hrm_power_ctrl(data, HRM_OFF);
	if (err < 0) {
		HRM_dbg("%s hrm_power_ctrl fail(%d, %d)\n", __func__,
			err, HRM_OFF);
		goto dev_set_drvdata_failed;
	}

	HRM_dbg("%s success\n", __func__);
	goto done;

dev_set_drvdata_failed:
	free_irq(data->hrm_irq, data);
err_setup_irq:
	sensors_unregister(data->dev, hrm_sensor_attrs);
hrm_sensor_register_failed:
	sysfs_remove_group(&data->hrm_input_dev->dev.kobj,
			   &hrm_attribute_group);
err_sysfs_create_group:
	sensors_remove_symlink(data->hrm_input_dev);
err_sensors_create_symlink:
	input_unregister_device(data->hrm_input_dev);
err_input_register_device:
err_input_allocate_device:
hrm_init_device_failed:
	hrm_power_ctrl(data, HRM_OFF);
err_power_on:
	gpio_free(data->hrm_int);
#ifdef CONFIG_SENSORS_HRM_MAX86915
	gpio_free(data->hrm_en);
#endif
err_setup_gpio:
err_parse_dt:
	if (data->hrm_pinctrl) {
#ifdef CONFIG_ARCH_QCOM
		devm_pinctrl_put(data->hrm_pinctrl);
#else
		pinctrl_put(data->hrm_pinctrl);
#endif
		data->hrm_pinctrl = NULL;
	}
	if (data->pins_idle)
		data->pins_idle = NULL;
	if (data->pins_sleep)
		data->pins_sleep = NULL;

	mutex_destroy(&data->i2clock);
	mutex_destroy(&data->activelock);
	mutex_destroy(&data->suspendlock);
	kfree(data);
	HRM_dbg("%s failed\n", __func__);
done:
	return err;
}


int hrm_remove(struct i2c_client *client)
{
	struct hrm_device_data *data = i2c_get_clientdata(client);
	int err;

	HRM_dbg("%s\n", __func__);
	hrm_power_ctrl(data, HRM_OFF);
	err = hrm_deinit_device(data);
	if (err)
		HRM_dbg("%s hrm_deinit device fail err = %d\n",
			__func__, err);

	sensors_unregister(data->dev, hrm_sensor_attrs);
	sysfs_remove_group(&data->hrm_input_dev->dev.kobj,
			   &hrm_attribute_group);
	sensors_remove_symlink(data->hrm_input_dev);
	input_unregister_device(data->hrm_input_dev);

	if (data->hrm_pinctrl) {
#ifdef CONFIG_ARCH_QCOM
		devm_pinctrl_put(data->hrm_pinctrl);
#else
		pinctrl_put(data->hrm_pinctrl);
#endif
		data->hrm_pinctrl = NULL;
	}
	if (data->pins_idle)
		data->pins_idle = NULL;
	if (data->pins_sleep)
		data->pins_sleep = NULL;
	disable_irq(data->hrm_irq);
	free_irq(data->hrm_irq, data);
	gpio_free(data->hrm_int);
#ifdef CONFIG_SENSORS_HRM_MAX86915
	gpio_free(data->hrm_en);
#endif
	mutex_destroy(&data->i2clock);
	mutex_destroy(&data->activelock);
	mutex_destroy(&data->suspendlock);
	kfree(data->lib_ver);
	kfree(data);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static void hrm_shutdown(struct i2c_client *client)
{
	HRM_dbg("%s\n", __func__);
}

#ifdef CONFIG_PM
static int hrm_pm_suspend(struct device *dev)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err = 0;

	HRM_dbg("%s %d\n", __func__, data->hrm_enabled_mode);

	data->hrm_prev_mode = data->hrm_enabled_mode;
	data->mode_sdk_prev = data->mode_sdk_enabled;

	if (data->hrm_enabled_mode != 0)
		hrm_enable_store(dev, &dev_attr_enable, "0", 2);

	mutex_lock(&data->suspendlock);

	data->pm_state = PM_SUSPEND;

	hrm_pin_control(data, false);

	mutex_unlock(&data->suspendlock);

	return err;
}

static int hrm_pm_resume(struct device *dev)
{
	struct hrm_device_data *data = dev_get_drvdata(dev);
	int err = 0;
	char buf[3];

	HRM_dbg("%s %d\n", __func__, data->hrm_prev_mode);

	mutex_lock(&data->suspendlock);

	hrm_pin_control(data, true);

	data->pm_state = PM_RESUME;

	mutex_unlock(&data->suspendlock);

	if (data->hrm_prev_mode != 0) {
		if (data->hrm_prev_mode == MODE_SDK_IR) {
			if (data->mode_sdk_prev & 1)
				hrm_enable_store(dev, &dev_attr_enable, "10", 2);
			if (data->mode_sdk_prev & 2)
				hrm_enable_store(dev, &dev_attr_enable, "11", 2);
			if (data->mode_sdk_prev & 4)
				hrm_enable_store(dev, &dev_attr_enable, "12", 2);
			if (data->mode_sdk_prev & 8)
				hrm_enable_store(dev, &dev_attr_enable, "13", 2);
		} else {
			sprintf(buf, "%d", data->hrm_prev_mode);
			hrm_enable_store(dev, &dev_attr_enable, buf, 2);
		}
	}

	return err;
}

static const struct dev_pm_ops hrm_pm_ops = {
	.suspend = hrm_pm_suspend,
	.resume = hrm_pm_resume
};
#endif

static struct of_device_id hrm_match_table[] = {
	{ .compatible = "hrmsensor",},
	{},
};

static const struct i2c_device_id hrm_device_id[] = {
	{ "hrmsensor", 0 },
	{ }
};
/* descriptor of the hrmsensor I2C driver */
static struct i2c_driver hrm_i2c_driver = {
	.driver = {
	    .name = "hrmsensor",
	    .owner = THIS_MODULE,
#if defined(CONFIG_PM)
	    .pm = &hrm_pm_ops,
#endif
	    .of_match_table = hrm_match_table,
	},
	.probe = hrm_probe,
	.remove = hrm_remove,
	.shutdown = hrm_shutdown,
	.id_table = hrm_device_id,
};

/* initialization and exit functions */
static int __init hrm_init(void)
{
	if (!lpcharge)
		return i2c_add_driver(&hrm_i2c_driver);
	else
		return 0;
}

static void __exit hrm_exit(void)
{
	i2c_del_driver(&hrm_i2c_driver);
}

module_init(hrm_init);
module_exit(hrm_exit);

MODULE_DESCRIPTION("HRM Sensor Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
