/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <exynos-fimc-is-sensor.h>
#include "fimc-is-core.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-interface.h"
#include "fimc-is-sec-define.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-dt.h"
#include "fimc-is-device-ois.h"
#include "fimc-is-vender-specific.h"
#ifdef CONFIG_AF_HOST_CONTROL
#include "fimc-is-device-af.h"
#endif
#include <linux/pinctrl/pinctrl.h>
#if defined (CONFIG_OIS_USE_RUMBA_S4)
#include "fimc-is-device-ois_s4.h"
#elif defined (CONFIG_OIS_USE_RUMBA_S6)
#include "fimc-is-device-ois_s6.h"
#elif defined (CONFIG_OIS_USE_RUMBA_SA)
#include "fimc-is-device-ois_sa.h"
#elif defined (CONFIG_CAMERA_USE_MCU)
#include "fimc-is-device-ois_mcu.h"
#endif

#define FIMC_IS_OIS_DEV_NAME		"exynos-fimc-is-ois"
#define OIS_I2C_RETRY_COUNT	2

struct fimc_is_ois_info ois_minfo;
struct fimc_is_ois_info ois_pinfo;
struct fimc_is_ois_info ois_uinfo;
struct fimc_is_ois_exif ois_exif_data;
#ifdef USE_OIS_SLEEP_MODE
struct fimc_is_ois_shared_info ois_shared_info;
#endif

struct i2c_client *fimc_is_ois_i2c_get_client(struct fimc_is_core *core)
{
	struct i2c_client *client = NULL;
#ifndef CONFIG_CAMERA_USE_MCU
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	u32 sensor_idx = specific->ois_sensor_index;
#endif

#ifdef CONFIG_CAMERA_USE_MCU
	client = fimc_is_mcu_i2c_get_client(core);
#else
	if (core->sensor[sensor_idx].ois != NULL)
		client = core->sensor[sensor_idx].ois->client;
#endif

	return client;
};

int fimc_is_ois_i2c_read(struct i2c_client *client, u16 addr, u8 *data)
{
	int err;
	u8 txbuf[2], rxbuf[1];
	struct i2c_msg msg[2];

	*data = 0;
	txbuf[0] = (addr & 0xff00) >> 8;
	txbuf[1] = (addr & 0xff);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = txbuf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = rxbuf;

	err = i2c_transfer(client->adapter, msg, 2);
	if (unlikely(err != 2)) {
		err("%s: register read fail err = %d\n", __func__, err);
		return -EIO;
	}

	*data = rxbuf[0];
	return 0;
}

int fimc_is_ois_i2c_write(struct i2c_client *client ,u16 addr, u8 data)
{
        int retries = OIS_I2C_RETRY_COUNT;
        int ret = 0, err = 0;
        u8 buf[3] = {0,};
        struct i2c_msg msg = {
                .addr   = client->addr,
                .flags  = 0,
                .len    = 3,
                .buf    = buf,
        };

        buf[0] = (addr & 0xff00) >> 8;
        buf[1] = addr & 0xff;
        buf[2] = data;

#if 0
        info("%s : W(0x%02X%02X %02X)\n",__func__, buf[0], buf[1], buf[2]);
#endif

        do {
                ret = i2c_transfer(client->adapter, &msg, 1);
                if (likely(ret == 1))
                        break;

                usleep_range(10000,11000);
                err = ret;
        } while (--retries > 0);

        /* Retry occured */
        if (unlikely(retries < OIS_I2C_RETRY_COUNT)) {
                err("i2c_write: error %d, write (%04X, %04X), retry %d\n",
                        err, addr, data, retries);
        }

        if (unlikely(ret != 1)) {
                err("I2C does not work\n\n");
                return -EIO;
        }

        return 0;
}

int fimc_is_ois_i2c_write_multi(struct i2c_client *client ,u16 addr, u8 *data, size_t size)
{
	int retries = OIS_I2C_RETRY_COUNT;
	int ret = 0, err = 0;
	ulong i = 0;
	u8 buf[258] = {0,};
	struct i2c_msg msg = {
                .addr   = client->addr,
                .flags  = 0,
                .len    = size,
                .buf    = buf,
	};

	buf[0] = (addr & 0xFF00) >> 8;
	buf[1] = addr & 0xFF;

	for (i = 0; i < size - 2; i++) {
	        buf[i + 2] = *(data + i);
	}
#if 0
        info("OISLOG %s : W(0x%02X%02X%02X)\n", __func__, buf[0], buf[1], buf[2]);
#endif
        do {
                ret = i2c_transfer(client->adapter, &msg, 1);
                if (likely(ret == 1))
                        break;

                usleep_range(10000,11000);
                err = ret;
        } while (--retries > 0);

        /* Retry occured */
        if (unlikely(retries < OIS_I2C_RETRY_COUNT)) {
                err("i2c_write: error %d, write (%04X, %04X), retry %d\n",
                        err, addr, *data, retries);
        }

        if (unlikely(ret != 1)) {
                err("I2C does not work\n\n");
                return -EIO;
	}

        return 0;
}

int fimc_is_ois_i2c_read_multi(struct i2c_client *client, u16 addr, u8 *data, size_t size)
{
	int err;
	u8 rxbuf[256], txbuf[2];
	struct i2c_msg msg[2];

	txbuf[0] = (addr & 0xff00) >> 8;
	txbuf[1] = (addr & 0xff);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = txbuf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = size;
	msg[1].buf = rxbuf;

	err = i2c_transfer(client->adapter, msg, 2);
	if (unlikely(err != 2)) {
		err("%s: register read fail", __func__);
		return -EIO;
	}

	memcpy(data, rxbuf, size);
	return 0;
}

int fimc_is_ois_gpio_on(struct fimc_is_core *core)
{
	int ret = 0;
	struct exynos_platform_fimc_is_module *module_pdata;
	struct fimc_is_module_enum *module = NULL;
	int i = 0;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		fimc_is_search_sensor_module_with_position(&core->sensor[i], SENSOR_POSITION_REAR, &module);
		if (module)
			break;
	}

	if (!module) {
		err("%s: Could not find sensor id.", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	module_pdata = module->pdata;

	if (!module_pdata->gpio_cfg) {
		err("gpio_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = module_pdata->gpio_cfg(module, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON);
	if (ret) {
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_ois_gpio_off(struct fimc_is_core *core)
{
	int ret = 0;
	struct exynos_platform_fimc_is_module *module_pdata;
	struct fimc_is_module_enum *module = NULL;
	int i = 0;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		fimc_is_search_sensor_module_with_position(&core->sensor[i], SENSOR_POSITION_REAR, &module);
		if (module)
			break;
	}

	if (!module) {
		err("%s: Could not find sensor id.", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	module_pdata = module->pdata;

	if (!module_pdata->gpio_cfg) {
		err("gpio_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = module_pdata->gpio_cfg(module, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF);
	if (ret) {
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

struct fimc_is_device_ois *fimc_is_ois_get_device(struct i2c_client *client)
{
	struct fimc_is_device_ois *ois_device = NULL;

#ifdef CONFIG_CAMERA_USE_MCU
	struct fimc_is_mcu *mcu = i2c_get_clientdata(client);
	ois_device = mcu->ois_device;
#else
	ois_device = i2c_get_clientdata(client);
#endif

	return ois_device;
}

int fimc_is_sec_get_ois_minfo(struct fimc_is_ois_info **minfo)
{
	*minfo = &ois_minfo;
	return 0;
}

int fimc_is_sec_get_ois_pinfo(struct fimc_is_ois_info **pinfo)
{
	*pinfo = &ois_pinfo;
	return 0;
}

void fimc_is_ois_enable(struct fimc_is_core *core)
{
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	CALL_OISOPS(ois_device, ois_enable, core);
}

bool fimc_is_ois_offset_test(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	bool result = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	result = CALL_OISOPS(ois_device, ois_offset_test, core, raw_data_x, raw_data_y);

	return result;
}

void fimc_is_ois_get_offset_data(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	CALL_OISOPS(ois_device, ois_get_offset_data, core, raw_data_x, raw_data_y);
}

int fimc_is_ois_self_test(struct fimc_is_core *core)
{
	int ret = 0;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	ret = CALL_OISOPS(ois_device, ois_self_test, core);

	return ret;
}

#ifdef CONFIG_CAMERA_USE_MCU
bool fimc_is_ois_gyrocal_test(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	bool result = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	result = CALL_OISOPS(ois_device, ois_calibration_test, core, raw_data_x, raw_data_y);

	return result;
}

bool fimc_is_aperture_hall_test(struct fimc_is_core *core, u16 *hall_value)
{
	struct fimc_is_device_sensor *device = NULL;
	bool result = false;

	device = &core->sensor[0];

	if (device->mcu && device->mcu->aperture)
		result = fimc_is_mcu_halltest_aperture(device->mcu->subdev, hall_value);

	return result;
}
#endif

#ifndef CONFIG_CAMERA_USE_MCU
bool fimc_is_ois_diff_test(struct fimc_is_core *core, int *x_diff, int *y_diff)
{
	bool result = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	result = CALL_OISOPS(ois_device, ois_diff_test, core, x_diff, y_diff);

	return result;
}
#endif

bool fimc_is_ois_auto_test(struct fimc_is_core *core,
				int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y)
{
	bool result = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);

	result = CALL_OISOPS(ois_device, ois_auto_test, core,
			threshold, x_result, y_result, sin_x, sin_y);

	return result;
}

#ifdef CAMERA_2ND_OIS
bool fimc_is_ois_auto_test_rear2(struct fimc_is_core *core,
				int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y,
				bool *x_result_2nd, bool *y_result_2nd, int *sin_x_2nd, int *sin_y_2nd)
{
	bool result = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	result = CALL_OISOPS(ois_device, ois_auto_test_rear2, core,
			threshold, x_result, y_result, sin_x, sin_y,
			x_result_2nd, y_result_2nd, sin_x_2nd, sin_y_2nd);

	return result;
}
#endif

u16 fimc_is_ois_calc_checksum(u8 *data, int size)
{
	int i = 0;
	u16 result = 0;

	for(i = 0; i < size; i += 2) {
		result = result + (0xFFFF & (((*(data + i + 1)) << 8) | (*(data + i))));
	}

	return result;
}

void fimc_is_ois_gyro_sleep(struct fimc_is_core *core)
{
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	CALL_OISOPS(ois_device, ois_gyro_sleep, core);
}

void fimc_is_ois_exif_data(struct fimc_is_core *core)
{
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	CALL_OISOPS(ois_device, ois_exif_data, core);
}

int fimc_is_ois_get_exif_data(struct fimc_is_ois_exif **exif_info)
{
	*exif_info = &ois_exif_data;
	return 0;
}

int fimc_is_ois_get_module_version(struct fimc_is_ois_info **minfo)
{
	*minfo = &ois_minfo;
	return 0;
}

int fimc_is_ois_get_phone_version(struct fimc_is_ois_info **pinfo)
{
	*pinfo = &ois_pinfo;
	return 0;
}

int fimc_is_ois_get_user_version(struct fimc_is_ois_info **uinfo)
{
	*uinfo = &ois_uinfo;
	return 0;
}

bool fimc_is_ois_version_compare(char *fw_ver1, char *fw_ver2, char *fw_ver3)
{
	if (fw_ver1[FW_GYRO_SENSOR] != fw_ver2[FW_GYRO_SENSOR]
		|| fw_ver1[FW_DRIVER_IC] != fw_ver2[FW_DRIVER_IC]
		|| fw_ver1[FW_CORE_VERSION] != fw_ver2[FW_CORE_VERSION]) {
		return false;
	}

	if (fw_ver2[FW_GYRO_SENSOR] != fw_ver3[FW_GYRO_SENSOR]
		|| fw_ver2[FW_DRIVER_IC] != fw_ver3[FW_DRIVER_IC]
		|| fw_ver2[FW_CORE_VERSION] != fw_ver3[FW_CORE_VERSION]) {
		return false;
	}

	return true;
}

bool fimc_is_ois_version_compare_default(char *fw_ver1, char *fw_ver2)
{
	if (fw_ver1[FW_GYRO_SENSOR] != fw_ver2[FW_GYRO_SENSOR]
		|| fw_ver1[FW_DRIVER_IC] != fw_ver2[FW_DRIVER_IC]
		|| fw_ver1[FW_CORE_VERSION] != fw_ver2[FW_CORE_VERSION]) {
		return false;
	}

	return true;
}

u8 fimc_is_ois_read_status(struct fimc_is_core *core)
{
	u8 status = 0;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	status = CALL_OISOPS(ois_device, ois_read_status, core);

	return status;
}

u8 fimc_is_ois_read_cal_checksum(struct fimc_is_core *core)
{
	u8 status = 0;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	status = CALL_OISOPS(ois_device, ois_read_cal_checksum, core);

	return status;
}

void fimc_is_ois_fw_status(struct fimc_is_core *core)
{
	ois_minfo.checksum = fimc_is_ois_read_status(core);
	ois_minfo.caldata = fimc_is_ois_read_cal_checksum(core);

	return;
}

bool fimc_is_ois_crc_check(struct fimc_is_core *core, char *buf)
{
	u8 check_8[4] = {0, };
	u32 *buf32 = NULL;
	u32 checksum;
	u32 checksum_bin;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);

	if (ois_device->not_crc_bin) {
		err("ois binary does not conatin crc checksum.\n");
		return false;
	}

	if (buf == NULL) {
		err("buf is NULL. CRC check failed.");
		return false;
	}

	buf32 = (u32 *)buf;

	memcpy(check_8, buf + OIS_BIN_LEN, 4);
	checksum_bin = (check_8[3] << 24) | (check_8[2] << 16) | (check_8[1] << 8) | (check_8[0]);

	checksum = (u32)getCRC((u16 *)&buf32[0], OIS_BIN_LEN, NULL, NULL);
	if (checksum != checksum_bin) {
		return false;
	} else {
		return true;
	}
}

bool fimc_is_ois_check_fw(struct fimc_is_core *core)
{
	bool ret = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	ret = CALL_OISOPS(ois_device, ois_check_fw, core);

	return ret;
}

bool fimc_is_ois_read_fw_ver(struct fimc_is_core *core, char *name, char *ver)
{
	bool ret = false;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = fimc_is_ois_get_device(client);
	ret = CALL_OISOPS(ois_device, ois_read_fw_ver, name, ver);

	return ret;
}

void fimc_is_ois_fw_update_from_sensor(void *ois_core)
{
	struct fimc_is_device_ois *ois_device = NULL;
	struct fimc_is_core *core = (struct fimc_is_core *)ois_core;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);
	struct fimc_is_vender_specific *specific = core->vender.private_data;

	if (client) {
		ois_device = fimc_is_ois_get_device(client);
	} else {
		err("failed to get client");
		return;
	}

	mutex_lock(&specific->hw_init_lock);

	fimc_is_ois_gpio_on(core);
	msleep(50);
	CALL_OISOPS(ois_device, ois_fw_update, core);
	fimc_is_ois_gpio_off(core);

	mutex_unlock(&specific->hw_init_lock);

	return;
}

void fimc_is_ois_fw_update(struct fimc_is_core *core)
{
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	if (client) {
		ois_device = fimc_is_ois_get_device(client);
	} else {
		err("failed to get client");
		return;
	}

	fimc_is_ois_gpio_on(core);
	msleep(50);
	CALL_OISOPS(ois_device, ois_fw_update, core);
	fimc_is_ois_gpio_off(core);

	return;
}

#ifdef USE_OIS_SLEEP_MODE
void fimc_is_ois_set_oissel_info(int oissel)
{
	ois_shared_info.oissel = oissel;
}
int fimc_is_ois_get_oissel_info(void)
{
	return ois_shared_info.oissel;
}
#endif
MODULE_DESCRIPTION("OIS driver for Rumba");
MODULE_AUTHOR("kyoungho yun <kyoungho.yun@samsung.com>");
MODULE_LICENSE("GPL v2");
