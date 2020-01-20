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
#ifdef CONFIG_OIS_FW_UPDATE_THREAD_USE
#include <linux/kthread.h>
#endif

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
#include "fimc-is-device-ois_s6.h"

#define OIS_NAME "OIS_RUMBA_S6"
#define OIS_GYRO_SCALE_FACTOR_IDG	175
#define OIS_GYRO_SCALE_FACTOR_K2G	131

static int ois_shift_x[POSITION_NUM];
static int ois_shift_y[POSITION_NUM];
#ifdef OIS_CENTERING_SHIFT_ENABLE
static int ois_centering_shift_x;
static int ois_centering_shift_y;
static int ois_centering_shift_x_rear2;
static int ois_centering_shift_y_rear2;
static bool ois_centering_shift_enable;
#endif
static int ois_shift_x_rear2[POSITION_NUM];
static int ois_shift_y_rear2[POSITION_NUM];
#ifdef USE_OIS_SLEEP_MODE
static bool ois_wide_start;
static bool ois_tele_start;
#else
static bool ois_wide_init;
static bool ois_tele_init;
#endif
static bool ois_fadeupdown;
static const struct v4l2_subdev_ops subdev_ops;
static u8 progCode[OIS_BOOT_FW_SIZE + OIS_PROG_FW_SIZE] = {0,};
static bool fw_sdcard;
static u16 ois_center_x;
static u16 ois_center_y;
extern struct fimc_is_ois_info ois_minfo;
extern struct fimc_is_ois_info ois_pinfo;
extern struct fimc_is_ois_info ois_uinfo;
extern struct fimc_is_ois_exif ois_exif_data;

void fimc_is_ois_enable_s6(struct fimc_is_core *core)
{
	int ret = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	dbg_ois("%s : E\n", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x02, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_write(client, 0x00, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	dbg_ois("%s : X\n", __func__);
}

void fimc_is_ois_version_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 val_c = 0, val_d = 0;
	u16 version = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x00FC, &val_c);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_read(client, 0x00FD, &val_d);
	if (ret) {
		err("i2c write fail\n");
	}

	version = (val_d << 8) | val_c;
	dbg_ois("OIS version = 0x%04x\n", version);
}

void fimc_is_ois_offset_test_s6(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	int ret = 0, i = 0;
	u8 val = 0, x = 0, y = 0;
	int x_sum = 0, y_sum = 0, sum = 0;
	int retries = 0, avg_count = 20;
	int scale_factor = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	dbg_ois("%s : E\n", __func__);

	if (ois_minfo.header_ver[0] == '6') {
		scale_factor = OIS_GYRO_SCALE_FACTOR_IDG;
	} else {
		scale_factor = OIS_GYRO_SCALE_FACTOR_K2G;
	}

	ret = fimc_is_ois_i2c_write(client, 0x0014, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	retries = avg_count;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0014, &val);
		if (ret != 0) {
			break;
		}
		msleep(10);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	sum = 0;
	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x0248, &val);
		x = val;
		fimc_is_ois_i2c_read(client, 0x0249, &val);
		x_sum = (val << 8) | x;
		if (x_sum > 0x7FFF) {
			x_sum = -((x_sum ^ 0xFFFF) + 1);
		}
		sum += x_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_x = sum * 1000 / scale_factor / 10;

	sum = 0;
	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x024A, &val);
		y = val;
		fimc_is_ois_i2c_read(client, 0x024B, &val);
		y_sum = (val << 8) | y;
		if (y_sum > 0x7FFF) {
			y_sum = -((y_sum ^ 0xFFFF) + 1);
		}
		sum += y_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_y = sum * 1000 / scale_factor / 10;

	fimc_is_ois_version_s6(core);
	dbg_ois("%s : X\n", __func__);
	return;
}

void fimc_is_ois_get_offset_data_s6(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y)
{
	int i = 0;
	u8 val = 0, x = 0, y = 0;
	int x_sum = 0, y_sum = 0, sum = 0;
	int retries = 0, avg_count = 20;
	int scale_factor = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	dbg_ois("%s : E\n", __func__);

	if (ois_minfo.header_ver[0] == '6') {
		scale_factor = OIS_GYRO_SCALE_FACTOR_IDG;
	} else {
		scale_factor = OIS_GYRO_SCALE_FACTOR_K2G;
	}

	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x0248, &val);
		x = val;
		fimc_is_ois_i2c_read(client, 0x0249, &val);
		x_sum = (val << 8) | x;
		if (x_sum > 0x7FFF) {
			x_sum = -((x_sum ^ 0xFFFF) + 1);
		}
		sum += x_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_x = sum * 1000 / scale_factor / 10;

	sum = 0;
	retries = avg_count;
	for (i = 0; i < retries; retries--) {
		fimc_is_ois_i2c_read(client, 0x024A, &val);
		y = val;
		fimc_is_ois_i2c_read(client, 0x024B, &val);
		y_sum = (val << 8) | y;
		if (y_sum > 0x7FFF) {
			y_sum = -((y_sum ^ 0xFFFF) + 1);
		}
		sum += y_sum;
	}
	sum = sum * 10 / avg_count;
	*raw_data_y = sum * 1000 / scale_factor / 10;

	fimc_is_ois_version_s6(core);
	dbg_ois("%s : X\n", __func__);
	return;
}

int fimc_is_ois_self_test_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 val = 0;
	u8 reg_val = 0, x = 0, y = 0;
	u16 x_gyro_log = 0, y_gyro_log = 0;
	int retries = 20;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	dbg_ois("%s : E\n", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x0014, 0x08);
	if (ret) {
		err("i2c write fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0014, &val);
		if (ret != 0) {
			val = -EIO;
			break;
		}
		msleep(10);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0004, &val);
	if (ret != 0) {
		val = -EIO;
	}

	/* Gyro selfTest result */
	fimc_is_ois_i2c_read(client, 0x00EC, &reg_val);
	x = reg_val;
	fimc_is_ois_i2c_read(client, 0x00ED, &reg_val);
	x_gyro_log = (reg_val << 8) | x;

	fimc_is_ois_i2c_read(client, 0x00EE, &reg_val);
	y = reg_val;
	fimc_is_ois_i2c_read(client, 0x00EF, &reg_val);
	y_gyro_log = (reg_val << 8) | y;

	dbg_ois("%s(GSTLOG0=%d, GSTLOG1=%d)\n", __func__, x_gyro_log, y_gyro_log);

	dbg_ois("%s(%d) : X\n", __func__, val);
	return (int)val;
}

bool fimc_is_ois_diff_test_s6(struct fimc_is_core *core, int *x_diff, int *y_diff)
{
	int ret = 0;
	u8 val = 0, x = 0, y = 0;
	u16 x_min = 0, y_min = 0, x_max = 0, y_max = 0;
	int retries = 20, default_diff = 1100;
	u8 read_x[2], read_y[2];
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

#ifdef CONFIG_AF_HOST_CONTROL
	fimc_is_af_move_lens(core);
	msleep(30);
#endif

	dbg_ois("(%s) : E\n", __func__);

	ret = fimc_is_ois_i2c_read_multi(client, 0x021A, read_x, 2);
	ret |= fimc_is_ois_i2c_read_multi(client, 0x021C, read_y, 2);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_write_multi(client, 0x0022, read_x, 4);
	ret |= fimc_is_ois_i2c_write_multi(client, 0x0024, read_y, 4);
	ret |= fimc_is_ois_i2c_write(client, 0x0002, 0x02);
	ret |= fimc_is_ois_i2c_write(client, 0x0000, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}
	msleep(400);
	dbg_ois("(%s) : OIS Position = Center\n", __func__);

	ret = fimc_is_ois_i2c_write(client, 0x0000, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0001, &val);
		if (ret != 0) {
			break;
		}
		msleep(10);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val != 1);

	ret = fimc_is_ois_i2c_write(client, 0x0230, 0x64);
	ret |= fimc_is_ois_i2c_write(client, 0x0231, 0x00);
	ret |= fimc_is_ois_i2c_write(client, 0x0232, 0x64);
	ret |= fimc_is_ois_i2c_write(client, 0x0233, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_read(client, 0x0230, &val);
	dbg_ois("OIS[read_val_0x0230::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0231, &val);
	dbg_ois("OIS[read_val_0x0231::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0232, &val);
	dbg_ois("OIS[read_val_0x0232::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0233, &val);
	dbg_ois("OIS[read_val_0x0233::0x%04x]\n", val);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_write(client, 0x020E, 0x6E);
	ret |= fimc_is_ois_i2c_write(client, 0x020F, 0x6E);
	ret |= fimc_is_ois_i2c_write(client, 0x0210, 0x1E);
	ret |= fimc_is_ois_i2c_write(client, 0x0211, 0x1E);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_write(client, 0x0013, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

#if 0
	ret = fimc_is_ois_i2c_write(client, 0x0012, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	retries = 30;
	do { //polarity check
		ret = fimc_is_ois_i2c_read(client, 0x0012, &val);
		if (ret != 0) {
			break;
		}
		msleep(100);
		if (--retries < 0) {
			err("Polarity check is not done or not [read_val_0x0012::0x%04x]\n", val);
			break;
		}
	} while (val);
	fimc_is_ois_i2c_read(client, 0x0200, &val);
	err("OIS[read_val_0x0200::0x%04x]\n", val);
#endif

	retries = 120;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0013, &val);
		if (ret != 0) {
			break;
		}
		msleep(100);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	fimc_is_ois_i2c_read(client, 0x0004, &val);
	dbg_ois("OIS[read_val_0x0004::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0005, &val);
	dbg_ois("OIS[read_val_0x0005::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0006, &val);
	dbg_ois("OIS[read_val_0x0006::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0022, &val);
	dbg_ois("OIS[read_val_0x0022::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0023, &val);
	dbg_ois("OIS[read_val_0x0023::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0024, &val);
	dbg_ois("OIS[read_val_0x0024::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0025, &val);
	dbg_ois("OIS[read_val_0x0025::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0200, &val);
	dbg_ois("OIS[read_val_0x0200::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x020E, &val);
	dbg_ois("OIS[read_val_0x020E::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x020F, &val);
	dbg_ois("OIS[read_val_0x020F::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0210, &val);
	dbg_ois("OIS[read_val_0x0210::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0211, &val);
	dbg_ois("OIS[read_val_0x0211::0x%04x]\n", val);

	fimc_is_ois_i2c_read(client, 0x0212, &val);
	x = val;
	dbg_ois("OIS[read_val_0x0212::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0213, &val);
	x_max = (val << 8) | x;
	dbg_ois("OIS[read_val_0x0213::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0214, &val);
	x = val;
	dbg_ois("OIS[read_val_0x0214::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0215, &val);
	x_min = (val << 8) | x;
	dbg_ois("OIS[read_val_0x0215::0x%04x]\n", val);

	fimc_is_ois_i2c_read(client, 0x0216, &val);
	y = val;
	dbg_ois("OIS[read_val_0x0216::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0217, &val);
	y_max = (val << 8) | y;
	dbg_ois("OIS[read_val_0x0217::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0218, &val);
	y = val;
	dbg_ois("OIS[read_val_0x0218::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0219, &val);
	y_min = (val << 8) | y;
	dbg_ois("OIS[read_val_0x0219::0x%04x]\n", val);

	fimc_is_ois_i2c_read(client, 0x021A, &val);
	dbg_ois("OIS[read_val_0x021A::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x021B, &val);
	dbg_ois("OIS[read_val_0x021B::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x021C, &val);
	dbg_ois("OIS[read_val_0x021C::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x021D, &val);
	dbg_ois("OIS[read_val_0x021D::0x%04x]\n", val);

	*x_diff = abs(x_max - x_min);
	*y_diff = abs(y_max - y_min);

	dbg_ois("(%s) : X (default_diff:%d)(%d,%d)\n", __func__,
			default_diff, *x_diff, *y_diff);

	ret = fimc_is_ois_i2c_read_multi(client, 0x021A, read_x, 2);
	ret |= fimc_is_ois_i2c_read_multi(client, 0x021C, read_y, 2);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_write_multi(client, 0x0022, read_x, 4);
	ret |= fimc_is_ois_i2c_write_multi(client, 0x0024, read_y, 4);
	ret |= fimc_is_ois_i2c_write(client, 0x0002, 0x02);
	ret |= fimc_is_ois_i2c_write(client, 0x0000, 0x01);
	msleep(400);
	ret |= fimc_is_ois_i2c_write(client, 0x0000, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	if (*x_diff > default_diff && *y_diff > default_diff) {
		return true;
	} else {
		return false;
	}
}

bool fimc_is_ois_sine_wavecheck_s6(struct fimc_is_core *core,
	int threshold, int *sinx, int *siny, int *result)
{
	u8 buf = 0, val = 0;
	int ret = 0, retries = 10;
	int sinx_count = 0, siny_count = 0;
	u8 u8_sinx_count[2] = {0, }, u8_siny_count[2] = {0, };
	u8 u8_sinx[2] = {0, }, u8_siny[2] = {0, };
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_write(client, 0x0052, (u8)threshold); /* error threshold level. */
	ret |= fimc_is_ois_i2c_write(client, 0x00BE, 0x01); /* OIS SEL (wide : 1 , tele : 2, both : 3 ). */
	ret |= fimc_is_ois_i2c_write(client, 0x0053, 0x0); /* count value for error judgement level. */
	ret |= fimc_is_ois_i2c_write(client, 0x0054, 0x05); /* frequency level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0055, 0x2A); /* amplitude level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0056, 0x03); /* dummy pulse setting. */
	ret |= fimc_is_ois_i2c_write(client, 0x0057, 0x02); /* vyvle level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0050, 0x01); /* start sine wave check operation */
	if (ret) {
		err("i2c write fail\n");
		goto exit;
	}

	retries = 10;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0050, &val);
		if (ret) {
			err("i2c read fail\n");
			goto exit;
		}

		msleep(100);

		if (--retries < 0) {
			err("sine wave operation fail.\n");
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0051, &buf);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	*result = (int)buf;

	ret = fimc_is_ois_i2c_read_multi(client, 0x00C0, u8_sinx_count, 2);
	sinx_count = (u8_sinx_count[1] << 8) | u8_sinx_count[0];
	if (sinx_count > 0x7FFF) {
		sinx_count = -((sinx_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C2, u8_siny_count, 2);
	siny_count = (u8_siny_count[1] << 8) | u8_siny_count[0];
	if (siny_count > 0x7FFF) {
		siny_count = -((siny_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C4, u8_sinx, 2);
	*sinx = (u8_sinx[1] << 8) | u8_sinx[0];
	if (*sinx > 0x7FFF) {
		*sinx = -((*sinx ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C6, u8_siny, 2);
	*siny = (u8_siny[1] << 8) | u8_siny[0];
	if (*siny > 0x7FFF) {
		*siny = -((*siny ^ 0xFFFF) + 1);
	}
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	dbg_ois("threshold = %d, sinx = %d, siny = %d, sinx_count = %d, syny_count = %d\n",
		threshold, *sinx, *siny, sinx_count, siny_count);

	if (buf == 0x0) {
		return true;
	} else {
		return false;
	}

exit:
	*sinx = -1;
	*siny = -1;

	return false;
}

bool fimc_is_ois_auto_test_s6(struct fimc_is_core *core,
					int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y)
{
	int result = 0;
	int ret = 0;
	bool value = false;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);
	struct fimc_is_device_sensor *device;

#ifdef CONFIG_AF_HOST_CONTROL
	fimc_is_af_move_lens(core);
	msleep(100);
#endif

	fimc_is_ois_i2c_write(client, 0x0002, 0x05); /* ois centering mode */
	ret = fimc_is_ois_i2c_write(client, 0x00, 0x01); /* ois servo on */
	if (ret) {
		err("i2c write fail\n");
	}
	msleep(100);

	device = &core->sensor[SENSOR_POSITION_REAR];
	if (device->subdev_aperture != NULL) {
		ret = CALL_APERTUREOPS(device->aperture, prepare_ois_autotest, device->subdev_aperture);
		if (ret < 0)
			err("[%s] aperture set fail\n", __func__);
	}

	ret = fimc_is_ois_i2c_write(client, 0x00, 0x00); /* ois servo off */
	if (ret) {
		err("i2c write fail\n");
	}
	msleep(100);

	value = fimc_is_ois_sine_wavecheck_s6(core, threshold, sin_x, sin_y, &result);
	if (*sin_x == -1 && *sin_y == -1) {
		err("OIS device is not prepared.");
		*x_result = false;
		*y_result = false;
		*sin_x = 0;
		*sin_y = 0;

		return false;
	}

	if (value == true) {
		*x_result = true;
		*y_result = true;

		return true;
	} else {
		dbg_ois("OIS autotest is failed value = 0x%x\n", result);
		if ((result & 0x03) == 0x01) {
			*x_result = false;
			*y_result = true;
		} else if ((result & 0x03) == 0x02) {
			*x_result = true;
			*y_result = false;
		} else {
			*x_result = false;
			*y_result = false;
		}

		return false;
	}
}

#ifdef CAMERA_REAR2_OIS
bool fimc_is_ois_sine_wavecheck_rear2_s6(struct fimc_is_core *core,
					int threshold, int *sinx, int *siny, int *result,
					int *sinx_2nd, int *siny_2nd)
{
	u8 buf = 0, val = 0;
	int ret = 0, retries = 10;
	int sinx_count = 0, siny_count = 0;
	int sinx_count_2nd = 0, siny_count_2nd = 0;
	u8 u8_sinx_count[2] = {0, }, u8_siny_count[2] = {0, };
	u8 u8_sinx[2] = {0, }, u8_siny[2] = {0, };
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x03); /* OIS SEL (wide : 1 , tele : 2, both : 3 ). */
	ret |= fimc_is_ois_i2c_write(client, 0x0052, (u8)threshold); /* error threshold level. */
	ret |= fimc_is_ois_i2c_write(client, 0x005B, (u8)threshold); /* error threshold level. */
	ret |= fimc_is_ois_i2c_write(client, 0x0053, 0x0); /* count value for error judgement level. */
	ret |= fimc_is_ois_i2c_write(client, 0x0054, 0x05); /* frequency level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0055, 0x2A); /* amplitude level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0056, 0x03); /* dummy pulse setting. */
	ret |= fimc_is_ois_i2c_write(client, 0x0057, 0x02); /* vyvle level for measurement. */
	ret |= fimc_is_ois_i2c_write(client, 0x0050, 0x01); /* start sine wave check operation */
	if (ret) {
		err("i2c write fail\n");
		goto exit;
	}

	retries = 10;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0050, &val);
		if (ret) {
			err("i2c read fail\n");
			goto exit;
		}

		msleep(100);

		if (--retries < 0) {
			err("sine wave operation fail.\n");
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0051, &buf);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	*result = (int)buf;

	ret = fimc_is_ois_i2c_read_multi(client, 0x00C0, u8_sinx_count, 2);
	sinx_count = (u8_sinx_count[1] << 8) | u8_sinx_count[0];
	if (sinx_count > 0x7FFF) {
		sinx_count = -((sinx_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C2, u8_siny_count, 2);
	siny_count = (u8_siny_count[1] << 8) | u8_siny_count[0];
	if (siny_count > 0x7FFF) {
		siny_count = -((siny_count ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C4, u8_sinx, 2);
	*sinx = (u8_sinx[1] << 8) | u8_sinx[0];
	if (*sinx > 0x7FFF) {
		*sinx = -((*sinx ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00C6, u8_siny, 2);
	*siny = (u8_siny[1] << 8) | u8_siny[0];
	if (*siny > 0x7FFF) {
		*siny = -((*siny ^ 0xFFFF) + 1);
	}

	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E4, u8_sinx_count, 2);
	sinx_count_2nd = (u8_sinx_count[1] << 8) | u8_sinx_count[0];
	if (sinx_count_2nd > 0x7FFF) {
		sinx_count_2nd = -((sinx_count_2nd ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E6, u8_siny_count, 2);
	siny_count_2nd = (u8_siny_count[1] << 8) | u8_siny_count[0];
	if (siny_count_2nd > 0x7FFF) {
		siny_count_2nd = -((siny_count_2nd ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00E8, u8_sinx, 2);
	*sinx_2nd = (u8_sinx[1] << 8) | u8_sinx[0];
	if (*sinx_2nd > 0x7FFF) {
		*sinx_2nd = -((*sinx_2nd ^ 0xFFFF) + 1);
	}
	ret |= fimc_is_ois_i2c_read_multi(client, 0x00EA, u8_siny, 2);
	*siny_2nd = (u8_siny[1] << 8) | u8_siny[0];
	if (*siny_2nd > 0x7FFF) {
		*siny_2nd = -((*siny_2nd ^ 0xFFFF) + 1);
	}

	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	dbg_ois("threshold = %d, sinx = %d, siny = %d, sinx_count = %d, syny_count = %d\n",
		threshold, *sinx, *siny, sinx_count, siny_count);

	dbg_ois("threshold = %d, sinx_2nd = %d, siny_2nd = %d, sinx_count_2nd = %d, syny_count_2nd = %d\n",
		threshold, *sinx_2nd, *siny_2nd, sinx_count_2nd, siny_count_2nd);

	if (buf == 0x0) {
		return true;
	} else {
		return false;
	}

exit:
	*sinx = -1;
	*siny = -1;
	*sinx_2nd = -1;
	*siny_2nd = -1;
	return false;
}

bool fimc_is_ois_auto_test_rear2_s6(struct fimc_is_core *core,
					int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y,
					bool *x_result_2nd, bool *y_result_2nd, int *sin_x_2nd, int *sin_y_2nd)
{
	int result = 0;
	int ret = 0;
	bool value = false;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);
	struct fimc_is_device_sensor *device;

#ifdef CONFIG_AF_HOST_CONTROL
#ifdef CAMERA_REAR2_AF
	fimc_is_af_move_lens_rear2(core);
	msleep(100);
#endif
	fimc_is_af_move_lens(core);
	msleep(100);
#endif

	fimc_is_ois_i2c_write(client, 0x0002, 0x05); /* ois centering mode */
	ret = fimc_is_ois_i2c_write(client, 0x00, 0x01); /* ois servo on */
	if (ret) {
		err("i2c write fail\n");
	}
	msleep(100);

	device = &core->sensor[SENSOR_POSITION_REAR];
	if (device->subdev_aperture != NULL) {
		ret = CALL_APERTUREOPS(device->aperture, prepare_ois_autotest, device->subdev_aperture);
		if (ret < 0)
			err("[%s] aperture set fail\n", __func__);
	}

	ret = fimc_is_ois_i2c_write(client, 0x00, 0x00); /* ois servo off */
	if (ret) {
		err("i2c write fail\n");
	}
	msleep(100);

	value = fimc_is_ois_sine_wavecheck_rear2_s6(core, threshold, sin_x, sin_y, &result,
				sin_x_2nd, sin_y_2nd);

	if (*sin_x == -1 && *sin_y == -1) {
		err("OIS device is not prepared.");
		*x_result = false;
		*y_result = false;
		*sin_x = 0;
		*sin_y = 0;

		return false;
	}

	if (*sin_x_2nd == -1 && *sin_y_2nd == -1) {
		err("OIS 2 device is not prepared.");
		*x_result_2nd = false;
		*y_result_2nd = false;
		*sin_x_2nd = 0;
		*sin_y_2nd = 0;
		return false;
	}

	if (value == true) {
		*x_result = true;
		*y_result = true;
		*x_result_2nd = true;
		*y_result_2nd = true;

		return true;
	} else {
		err("OIS autotest_2nd is failed result (0x0051) = 0x%x\n", result);
		if ((result & 0x03) == 0x00) {
			*x_result = true;
			*y_result = true;
		} else if ((result & 0x03) == 0x01) {
			*x_result = false;
			*y_result = true;
		} else if ((result & 0x03) == 0x02) {
			*x_result = true;
			*y_result = false;
		} else {
			*x_result = false;
			*y_result = false;
		}

		if ((result & 0x30) == 0x00) {
			*x_result_2nd = true;
			*y_result_2nd = true;
		} else if ((result & 0x30) == 0x10) {
			*x_result_2nd = false;
			*y_result_2nd = true;
		} else if ((result & 0x30) == 0x20) {
			*x_result_2nd = true;
			*y_result_2nd = false;
		} else {
			*x_result_2nd = false;
			*y_result_2nd = false;
		}

		return false;
	}
}
#endif

void fimc_is_ois_gyro_sleep_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 val = 0;
	int retries = 20;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_write(client, 0x0000, 0x00);
	if (ret) {
		err("i2c read fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0001, &val);
		if (ret != 0) {
			break;
		}

		if (val == 0x01 || val == 0x13)
			break;

		msleep(1);
	} while (--retries > 0);

	if (retries <= 0) {
		err("Read register failed!!!!, data = 0x%04x\n", val);
	}

	ret = fimc_is_ois_i2c_write(client, 0x0030, 0x03);
	if (ret) {
		err("i2c read fail\n");
	}
	msleep(1);

	return;
}

void fimc_is_ois_exif_data_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 error_reg[2], status_reg;
	u16 error_sum;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x0004, &error_reg[0]);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_read(client, 0x0005, &error_reg[1]);
	if (ret) {
		err("i2c read fail\n");
	}

	error_sum = (error_reg[1] << 8) | error_reg[0];

	ret = fimc_is_ois_i2c_read(client, 0x0001, &status_reg);
	if (ret) {
		err("i2c read fail\n");
	}

	ois_exif_data.error_data = error_sum;
	ois_exif_data.status_data = status_reg;

	return;
}

u8 fimc_is_ois_read_status_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 status = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x0006, &status);
	if (ret) {
		err("i2c read fail\n");
	}

	return status;
}

u8 fimc_is_ois_read_cal_checksum_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 status = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x0005, &status);
	if (ret) {
		err("i2c read fail\n");
	}

	return status;
}

int fimc_is_ois_read_manual_cal_s6(struct fimc_is_core *core)
{
	int ret = 0;
	u8 version[20] = {0, };
	u8 read_ver[3] = {0, };
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	version[0] = 0x21;
	version[1] = 0x43;
	version[2] = 0x65;
	version[3] = 0x87;
	version[4] = 0x23;
	version[5] = 0x01;
	version[6] = 0xEF;
	version[7] = 0xCD;
	version[8] = 0x00;
	version[9] = 0x74;
	version[10] = 0x00;
	version[11] = 0x00;
	version[12] = 0x04;
	version[13] = 0x00;
	version[14] = 0x00;
	version[15] = 0x00;
	version[16] = 0x01;
	version[17] = 0x00;
	version[18] = 0x00;
	version[19] = 0x00;

	ret = fimc_is_ois_i2c_write_multi(client, 0x0100, version, 0x16);
	msleep(5);
	ret |= fimc_is_ois_i2c_read(client, 0x0118, &read_ver[0]);
	ret |= fimc_is_ois_i2c_read(client, 0x0119, &read_ver[1]);
	ret |= fimc_is_ois_i2c_read(client, 0x011A, &read_ver[2]);
	if (ret) {
		err("i2c cmd fail\n");
		ret = -EINVAL;
		goto exit;
	}

	ois_minfo.header_ver[FW_CORE_VERSION] = read_ver[0];
	ois_minfo.header_ver[FW_GYRO_SENSOR] = read_ver[1];
	ois_minfo.header_ver[FW_DRIVER_IC] = read_ver[2];

exit:
	return ret;
}

bool fimc_is_ois_fw_version_s6(struct fimc_is_core *core)
{
	int ret = 0;
	char version[7] = {0, };
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x00FB, &version[0]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x00F9, &version[1]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x00F8, &version[2]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007C, &version[3]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007D, &version[4]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007E, &version[5]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007F, &version[6]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	memcpy(ois_minfo.header_ver, version, 7);
	specific->ois_ver_read = true;

	fimc_is_ois_fw_status(core);

	return true;

exit:
	return false;
}

int fimc_is_ois_fw_revision_s6(char *fw_ver)
{
	int revision = 0;
	revision = revision + ((int)fw_ver[FW_RELEASE_YEAR] - 58) * 10000;
	revision = revision + ((int)fw_ver[FW_RELEASE_MONTH] - 64) * 100;
	revision = revision + ((int)fw_ver[FW_RELEASE_COUNT] - 48) * 10;
	revision = revision + (int)fw_ver[FW_RELEASE_COUNT + 1] - 48;

	return revision;
}

bool fimc_is_ois_read_userdata_s6(struct fimc_is_core *core)
{
	u8 SendData[2];
	u8 Read_data[0x2ff] = {0, };
	int retries = 0;
	int ret = 0;
	u8 val = 0;
	u8 ois_shift_info = 0;
	int i = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	/* OIS servo OFF */
	fimc_is_ois_i2c_write(client ,0x0000, 0x00);
	retries = 50;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0000, &val);
		if (ret != 0) {
			break;
		}
		msleep(100);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%02x\n", val);
			break;
		}
	} while (val == 0x01);

	/* User Data Area & Address Setting step1 */
	fimc_is_ois_i2c_write(client ,0x000F, 0x40);
	SendData[0] = 0x00;
	SendData[1] = 0x00;
	fimc_is_ois_i2c_write_multi(client, 0x0010, SendData, 4);
	fimc_is_ois_i2c_write(client ,0x000E, 0x04);

	retries = 50;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x000E, &val);
		if (ret != 0) {
			break;
		}
		msleep(100);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%02x\n", val);
			break;
		}
	} while (val != 0x14);

	fimc_is_ois_i2c_read_multi(client, 0x0100, Read_data, USER_FW_SIZE);
	memcpy(ois_uinfo.header_ver, Read_data, 7);
	info("OIS Read register uinfo, header_ver = %s\n", ois_uinfo.header_ver);
	for (i = 0; i < 255; i = i + 4) {
		dbg_ois("OIS[user data::0x%02x%02x%02x%02x]\n",
			Read_data[i], Read_data[i + 1], Read_data[i + 2], Read_data[i + 3]);
	}

	/* User Data Area & Address Setting step2 */
	fimc_is_ois_i2c_write(client ,0x000F, 0x40);
	SendData[0] = 0x00;
	SendData[1] = 0x01;
	fimc_is_ois_i2c_write_multi(client, 0x0010, SendData, 4);
	fimc_is_ois_i2c_write(client ,0x000E, 0x04);

	retries = 50;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x000E, &val);
		if (ret != 0) {
			break;
		}
		msleep(10);

		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%02x\n", val);
			break;
		}
	} while (val != 0x14);

	fimc_is_ois_i2c_read_multi(client, 0x0100, Read_data + 0x100, USER_FW_SIZE);

	/* User Data Area & Address Setting step3 */
	fimc_is_ois_i2c_write(client ,0x000F, 0x40);
	SendData[0] = 0x00;
	SendData[1] = 0x02;
	fimc_is_ois_i2c_write_multi(client, 0x0010, SendData, 4);
	fimc_is_ois_i2c_write(client ,0x000E, 0x04);
	fimc_is_ois_i2c_read(client, 0x0100, &ois_shift_info);
	info("OIS Shift Info : 0x%x\n", ois_shift_info);

	if (ois_shift_info == 0x11) {
		u16 ois_shift_checksum = 0;
		u16 ois_shift_x_diff = 0;
		u16 ois_shift_y_diff = 0;
		s16 ois_shift_x_cal = 0;
		s16 ois_shift_y_cal = 0;
		u8 calData[2];

		/* OIS Shift CheckSum */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0102, calData, 2);
		ois_shift_checksum = (calData[1] << 8) | (calData[0]);
		info("OIS Shift CheckSum = 0x%x\n", ois_shift_checksum);

		/* OIS Shift X Diff */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0104, calData, 2);
		ois_shift_x_diff = (calData[1] << 8) | (calData[0]);
		info("OIS Shift X Diff = 0x%x\n", ois_shift_x_diff);

		/* OIS Shift Y Diff */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0106, calData, 2);
		ois_shift_y_diff = (calData[1] << 8) | (calData[0]);
		info("OIS Shift Y Diff = 0x%x\n", ois_shift_y_diff);

		/* OIS Shift CAL DATA */
		for (i = 0; i < 9; i++) {
			calData[0] = 0;
			calData[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0110 + i*2, calData, 2);
			ois_shift_x_cal = (calData[1] << 8) | (calData[0]);

			calData[0] = 0;
			calData[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0122 + i*2, calData, 2);
			ois_shift_y_cal = (calData[1] << 8) | (calData[0]);
			info("OIS CAL[%d]:X[%d], Y[%d]\n", i, ois_shift_x_cal, ois_shift_y_cal);
		}
	}

	ois_shift_info = 0;
	fimc_is_ois_i2c_read(client, 0x0101, &ois_shift_info);
	info("OIS Shift Info : 0x%x\n", ois_shift_info);

	if (ois_shift_info == 0x11) {
		u16 ois_shift_checksum = 0;
		u16 ois_shift_x_diff = 0;
		u16 ois_shift_y_diff = 0;
		s16 ois_shift_x_cal = 0;
		s16 ois_shift_y_cal = 0;
		u8 calData[2];

		/* OIS Shift CheckSum */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0102, calData, 2);
		ois_shift_checksum = (calData[1] << 8) | (calData[0]);
		info("OIS Shift CheckSum = 0x%x\n", ois_shift_checksum);

		/* REAR2 OIS Shift X Diff */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0108, calData, 2);
		ois_shift_x_diff = (calData[1] << 8) | (calData[0]);
		info("REAR2 OIS Shift X Diff = 0x%x\n", ois_shift_x_diff);

		/* OIS Shift Y Diff */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x010A, calData, 2);
		ois_shift_y_diff = (calData[1] << 8) | (calData[0]);
		info("REAR2 OIS Shift Y Diff = 0x%x\n", ois_shift_y_diff);

		/* OIS Shift CAL DATA */
		for (i = 0; i < 9; i++) {
			calData[0] = 0;
			calData[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0140 + i*2, calData, 2);
			ois_shift_x_cal = (calData[1] << 8) | (calData[0]);

			calData[0] = 0;
			calData[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0152 + i*2, calData, 2);
			ois_shift_y_cal = (calData[1] << 8) | (calData[0]);
			info("REAR2 OIS CAL[%d]:X[%d], Y[%d]\n", i, ois_shift_x_cal, ois_shift_y_cal);
		}
	}

	return true;
}

int fimc_is_ois_open_fw_s6(struct fimc_is_core *core, char *name, u8 **buf)
{
	int ret = 0;
	ulong size = 0;
	const struct firmware *fw_blob = NULL;
	static char fw_name[100];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;
	int fw_requested = 1;
	int retry_count = 0;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = i2c_get_clientdata(client);

	fw_sdcard = false;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(fw_name, sizeof(fw_name), "%s%s", FIMC_IS_OIS_SDCARD_PATH, name);
	fp = filp_open(fw_name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		info("failed to open SDCARD fw!!!\n");
		goto request_fw;
	}

	fw_requested = 0;
	size = fp->f_path.dentry->d_inode->i_size;
	info("start read sdcard, file path %s, size %lu Bytes\n", fw_name, size);

	*buf = vmalloc(size);
	if (!(*buf)) {
		err("failed to allocate memory");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = vfs_read(fp, (char __user *)(*buf), size, &fp->f_pos);
	if (nread != size) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto p_err;
	}

	memcpy(ois_pinfo.header_ver, *buf + OIS_BIN_HEADER, 3);
	memcpy(&ois_pinfo.header_ver[3], *buf + OIS_BIN_HEADER - 4, 4);
	memcpy(progCode, *buf, OIS_BOOT_FW_SIZE + OIS_PROG_FW_SIZE);

	fw_sdcard = true;
	if (OIS_BIN_LEN >= nread) {
		ois_device->not_crc_bin = true;
		err("ois fw binary size = %ld.\n", nread);
	}

request_fw:
	if (fw_requested) {
		snprintf(fw_name, sizeof(fw_name), "%s", name);
		set_fs(old_fs);
		retry_count = 3;
		ret = request_firmware(&fw_blob, fw_name, &client->dev);
		while (--retry_count && ret == -EAGAIN) {
			err("request_firmware retry(count:%d)", retry_count);
			ret = request_firmware(&fw_blob, fw_name, &client->dev);
		}

		if (ret) {
			err("request_firmware is fail(ret:%d)", ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (!fw_blob) {
			err("fw_blob is NULL");
			ret = -EINVAL;
			goto p_err;
		}

		if (!fw_blob->data) {
			err("fw_blob->data is NULL");
			ret = -EINVAL;
			goto p_err;
		}

		size = fw_blob->size;

		*buf = vmalloc(size);
		if (!(*buf)) {
			err("failed to allocate memory");
			ret = -ENOMEM;
			goto p_err;
		}

		memcpy((void *)(*buf), fw_blob->data, size);
		memcpy(ois_pinfo.header_ver, *buf + OIS_BIN_HEADER, 3);
		memcpy(&ois_pinfo.header_ver[3], *buf + OIS_BIN_HEADER - 4, 4);
		memcpy(progCode, *buf, OIS_BOOT_FW_SIZE + OIS_PROG_FW_SIZE);

		if (OIS_BIN_LEN >= size) {
			ois_device->not_crc_bin = true;
			err("ois fw binary size = %lu.\n", size);
		}
		info("OIS firmware is loaded from Phone binary.\n");
	}

p_err:
	if (!fw_requested) {
		if (!IS_ERR_OR_NULL(fp)) {
			filp_close(fp, current->files);
		}
		set_fs(old_fs);
	} else {
		if (!IS_ERR_OR_NULL(fw_blob)) {
			release_firmware(fw_blob);
		}
	}

	return ret;
}

bool fimc_is_ois_check_fw_s6(struct fimc_is_core *core)
{
	u8 *buf = NULL;
	int ret = 0;

	ret = fimc_is_ois_fw_version_s6(core);

	if (!ret) {
		err("Failed to read ois fw version.");
		return false;
	}

	fimc_is_ois_read_userdata_s6(core);

	switch (ois_minfo.header_ver[FW_CORE_VERSION]) {
	case 'A':
	case 'C':
	case 'E':
	case 'G':
	case 'I':
	case 'K':
	case 'M':
	case 'O':
		ret = fimc_is_ois_open_fw_s6(core, FIMC_OIS_FW_NAME_DOM, &buf);
		break;
	case 'B':
	case 'D':
	case 'F':
 	case 'H':
	case 'J':
	case 'L':
	case 'N':
	case 'P':
		ret = fimc_is_ois_open_fw_s6(core, FIMC_OIS_FW_NAME_SEC, &buf);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret) {
		err("fimc_is_ois_open_fw_s6 failed(%d)\n", ret);
	}

	if (buf) {
		vfree(buf);
	}

	return true;
}

int fimc_is_ois_read_fw_ver_s6(char *name, char *ver)
{
	int ret = 0;
	ulong size = 0;
	char buf[100] = {0, };
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		info("failed to open fw!!!\n");
		ret = -EIO;
		goto exit;
	}

	size = 7;
	fp->f_pos = (OIS_BIN_HEADER - 4);

	nread = vfs_read(fp, (char __user *)(buf), size, &fp->f_pos);
	if (nread != size) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto exit;
	}

exit:
	if (!IS_ERR_OR_NULL(fp))
		filp_close(fp, current->files);
	set_fs(old_fs);

	memcpy(ver, &buf[4], 3);
	memcpy(&ver[3], buf, 4);

	return ret;
}

#ifdef CAMERA_USE_OIS_EXT_CLK
void fimc_is_ois_check_extclk_s6(struct fimc_is_core *core)
{
	u8 cur_clk[4] = {0, };
	u8 new_clk[4] = {0, };
	u32 cur_clk_32 = 0;
	u32 new_clk_32 = 0;
	u8 pll_multi = 0;
	u8 pll_divide = 0;
	int ret = 0;
	u8 status = 0;
	int retries = 5;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0001, &status);
		if (ret != 0) {
			status = -EIO;
			err("i2c read fail\n");
			break;
		}
		msleep(10);
		if (--retries < 0) {
			err("Read register failed!!!!,clk data = 0x%02x\n", status);
			break;
		}
	} while (status != 0x01 && status != 0x13);

	ret = fimc_is_ois_i2c_read_multi(client, 0x03F0, cur_clk, 4);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	cur_clk_32 = (cur_clk[3] << 24) | (cur_clk[2] << 16) |
		(cur_clk[1] << 8) | (cur_clk[0]);

	if (cur_clk_32 != CAMERA_OIS_EXT_CLK) {
		new_clk_32 = CAMERA_OIS_EXT_CLK;
		new_clk[0] = CAMERA_OIS_EXT_CLK & 0xFF;
		new_clk[1] = (CAMERA_OIS_EXT_CLK >> 8) & 0xFF;
		new_clk[2] = (CAMERA_OIS_EXT_CLK >> 16) & 0xFF;
		new_clk[3] = (CAMERA_OIS_EXT_CLK >> 24) & 0xFF;

		switch (new_clk_32) {
		case 0xB71B00:
			pll_multi = 0x08;
			pll_divide = 0x03;
			break;
		case 0x1036640:
			pll_multi = 0x09;
			pll_divide = 0x05;
			break;
		case 0x124F800:
			pll_multi = 0x05;
			pll_divide = 0x03;
			break;
		case 0x16E3600:
			pll_multi = 0x04;
			pll_divide = 0x03;
			break;
		case 0x18CBA80:
			pll_multi = 0x06;
			pll_divide = 0x05;
			break;
		case 0x1E84800:
			pll_multi = 0x05;
			pll_divide = 0x05;
			break;
		default:
			info("cur_clk: 0x%08x\n", cur_clk_32);
			goto exit;
		}

		/* write EXTCLK */
		ret = fimc_is_ois_i2c_write_multi(client, 0x03F0, new_clk, 6);
		if (ret) {
			err("i2c write fail\n");
			goto exit;
		}

		/* write PLLMULTIPLE */
		ret = fimc_is_ois_i2c_write(client, 0x03F4, pll_multi);
		if (ret) {
			err("i2c write fail\n");
			goto exit;
		}

		/* write PLLDIVIDE */
		ret = fimc_is_ois_i2c_write(client, 0x03F5, pll_divide);
		if (ret) {
			err("i2c write fail\n");
			goto exit;
		}

		/* FLASH TO OIS MODULE */
		ret = fimc_is_ois_i2c_write(client, 0x0003, 0x01);
		if (ret) {
			err("i2c write fail\n");
			goto exit;
		}
		msleep(200);

		/* S/W Reset */
		ret = fimc_is_ois_i2c_write(client, 0x000D, 0x01);
		ret |= fimc_is_ois_i2c_write(client, 0x000E, 0x06);
		if (ret) {
			err("i2c write fail\n");
			goto exit;
		}
		msleep(50);

		dbg_ois("Apply EXTCLK for ois 0x%08x\n", new_clk_32);
	}else {
		dbg_ois("Keep current EXTCLK 0x%08x\n", cur_clk_32);
	}

exit:
	return;
}
#endif

void fimc_is_ois_fw_update_s6(struct fimc_is_core *core)
{
	u8 SendData[256], RcvData;
	u16 RcvDataShort = 0;
	int block, position = 0;
	u16 checkSum;
	u8 *buf = NULL;
	int ret = 0, sum_size = 0, retry_count = 3;
	int module_ver = 0, binary_ver = 0;
	bool forced_update = false, crc_result = false;
	bool need_reset = false;
	u8 ois_status[4] = {0, };
	u32 ois_status_32 = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	/* OIS Status Check */
	ret = fimc_is_ois_i2c_read_multi(client, 0x00FC, ois_status, 4);
	if (ret) {
		err("i2c read fail\n");
		goto p_err;
	}

	ois_status_32 = (ois_status[3] << 24) | (ois_status[2] << 16) |
		(ois_status[1] << 8) | (ois_status[0]);

	if (ois_status_32 == OIS_FW_UPDATE_ERROR_STATUS) {
		forced_update = true;
		fimc_is_ois_read_manual_cal_s6(core);
	} else {
#ifdef CAMERA_USE_OIS_EXT_CLK
		/* Check EXTCLK value */
		fimc_is_ois_check_extclk_s6(core);
#endif
	}

	if (!forced_update) {
		ret = fimc_is_ois_fw_version_s6(core);
		if (!ret) {
			err("Failed to read ois fw version.");
			return;
		}
		fimc_is_ois_read_userdata_s6(core);
	}

	switch (ois_minfo.header_ver[FW_CORE_VERSION]) {
	case 'A':
	case 'C':
	case 'E':
	case 'G':
	case 'I':
	case 'K':
	case 'M':
	case 'O':
		ret = fimc_is_ois_open_fw_s6(core, FIMC_OIS_FW_NAME_DOM, &buf);
		break;
	case 'B':
	case 'D':
	case 'F':
 	case 'H':
	case 'J':
	case 'L':
	case 'N':
	case 'P':
		ret = fimc_is_ois_open_fw_s6(core, FIMC_OIS_FW_NAME_SEC, &buf);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret) {
		err("fimc_is_ois_open_fw_s6 failed(%d)", ret);
	}

	if (ois_minfo.header_ver[FW_CORE_VERSION] != CAMERA_OIS_DOM_UPDATE_VERSION
		&& ois_minfo.header_ver[FW_CORE_VERSION] != CAMERA_OIS_SEC_UPDATE_VERSION) {
		info("Do not update ois Firmware. FW version is low.\n");
		goto p_err;
	}

	if (buf == NULL) {
		err("buf is NULL. OIS FW Update failed.");
		return;
	}

	if (!forced_update) {
		if (ois_uinfo.header_ver[0] == 0xFF && ois_uinfo.header_ver[1] == 0xFF &&
			ois_uinfo.header_ver[2] == 0xFF) {
			err("OIS userdata is not valid.");
			goto p_err;
		}
	}

	/*Update OIS FW when Gyro sensor/Driver IC/ Core version is same*/
	if (!forced_update) {
		if (!fimc_is_ois_version_compare(ois_minfo.header_ver, ois_pinfo.header_ver,
			ois_uinfo.header_ver)) {
			err("Does not update ois fw. OIS module ver = %s, binary ver = %s, userdata ver = %s",
				ois_minfo.header_ver, ois_pinfo.header_ver, ois_uinfo.header_ver);
			goto p_err;
		}
	} else {
		if (!fimc_is_ois_version_compare_default(ois_minfo.header_ver, ois_pinfo.header_ver)) {
			err("Does not update ois fw. OIS module ver = %s, binary ver = %s",
				ois_minfo.header_ver, ois_pinfo.header_ver);
			goto p_err;
		}
	}

	crc_result = fimc_is_ois_crc_check(core, buf);
	if (crc_result == false) {
		err("OIS CRC32 error.\n");
		goto p_err;
	}

	if (!fw_sdcard && !forced_update) {
		module_ver = fimc_is_ois_fw_revision_s6(ois_minfo.header_ver);
		binary_ver = fimc_is_ois_fw_revision_s6(ois_pinfo.header_ver);

		if (binary_ver <= module_ver) {
			err("OIS module ver = %d, binary ver = %d, module ver >= bin ver", module_ver, binary_ver);
			goto p_err;
		}
	}
	info("OIS fw update started. module ver = %s, binary ver = %s, userdata ver = %s\n",
		ois_minfo.header_ver, ois_pinfo.header_ver, ois_uinfo.header_ver);

retry:
	if (need_reset) {
		fimc_is_ois_gpio_off(core);
		msleep(30);
		fimc_is_ois_gpio_on(core);
		msleep(150);
	}

	fimc_is_ois_i2c_read(client, 0x0001, &RcvData); /* OISSTS Read */

	if (RcvData != 0x01) {/* OISSTS != IDLE */
		err("RCV data return!!");
		goto p_err;
	}

	/* Update a User Program */
	/* SET FWUP_CTRL REG */
	position = 0;
	SendData[0] = 0xB5;		/* FWUP_DSIZE=256Byte, FWUP_MODE=PROG, FWUPEN=Start */
	fimc_is_ois_i2c_write(client, 0x000C, SendData[0]);		/* FWUPCTRL REG(0x000C) 1Byte Send */
	msleep(55);

	/* Write UserProgram Data */
	for (block = 0; block < 176; block++) {		/* 0x1000 -0x 6FFF (RUMBA-SA)*/
		memcpy(SendData, &progCode[position], (size_t)FW_TRANS_SIZE);
		fimc_is_ois_i2c_write_multi(client, 0x0100, SendData, FW_TRANS_SIZE + 2); /* FLS_DATA REG(0x0100) 256Byte Send */
		position += FW_TRANS_SIZE;
		msleep(10);
	}

	/* CHECKSUM (Program) */
	sum_size = OIS_PROG_FW_SIZE + OIS_BOOT_FW_SIZE;
	checkSum = fimc_is_ois_calc_checksum(&progCode[0], sum_size);
	SendData[0] = (checkSum & 0x00FF);
	SendData[1] = (checkSum & 0xFF00) >> 8;
	SendData[2] = 0;		/* Don't Care */
	SendData[3] = 0x80;		/* Self Reset Request */

	fimc_is_ois_i2c_write_multi(client, 0x08, SendData, 6); /* FWUP_CHKSUM REG(0x0008) 4Byte Send */
	msleep(190);		/* (Wait RUMBA Self Reset) */
	fimc_is_ois_i2c_read(client, 0x6, (u8*)&RcvDataShort); /* Error Status read */

	if (RcvDataShort == 0x00) {
		/* F/W Update Success Process */
		info("%s: OISLOG OIS program update success\n", __func__);
	} else {
		/* Retry Process */
		if (retry_count > 0) {
			err("OISLOG OIS program update fail, retry update. count = %d", retry_count);
			retry_count--;
			need_reset = true;
			goto retry;
		}

		/* F/W Update Error Process */
		err("OISLOG OIS program update fail");
		goto p_err;
	}

	/* S/W Reset */
	fimc_is_ois_i2c_write(client ,0x000D, 0x01);
	fimc_is_ois_i2c_write(client ,0x000E, 0x06);
	msleep(50);

	/* Param init - Flash to Rumba */
	fimc_is_ois_i2c_write(client ,0x0036, 0x03);
	msleep(200);
	info("%s: OISLOG OIS param init done.\n", __func__);

	ret = fimc_is_ois_fw_version_s6(core);
	if (!ret) {
		err("Failed to read ois fw version.");
		goto p_err;
	}

	if (!fimc_is_ois_version_compare_default(ois_minfo.header_ver, ois_pinfo.header_ver)) {
		err("After update ois fw is not correct. OIS module ver = %s, binary ver = %s",
			ois_minfo.header_ver, ois_pinfo.header_ver);
		goto p_err;
	}

p_err:
	info("OIS module ver = %s, binary ver = %s, userdata ver = %s\n",
		ois_minfo.header_ver, ois_pinfo.header_ver, ois_uinfo.header_ver);

	if (buf) {
		vfree(buf);
	}
	return;
}

int fimc_is_ois_shift_compensation_s6(struct v4l2_subdev *subdev, int position, int resolution)
{
	int ret = 0;
	struct fimc_is_ois *ois;
	u32 af_value;
	int af_boundary = 0;
	u8 write_data[4] = {0,};
	short shift_x = 0;
	short shift_y = 0;
	struct fimc_is_device_sensor *device = NULL;
	u32 setfile;
	u32 scene;
	u32 sub_scene;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (ois->ois_shift_available == false)
		goto p_err;

	device = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(ois->sensor_peri->subdev_cis);
	if (unlikely(!device)) {
		err("device sensor is null");
		goto p_err;
	}

	if (device->ischain) {
		setfile = device->ischain->setfile;
	} else {
		err("device->ischain is null");
		ret = -EINVAL;
		goto p_err;
	}

	sub_scene = (setfile & FIMC_IS_SETFILE_MASK);
	scene = (setfile & FIMC_IS_SCENARIO_MASK) >> FIMC_IS_SCENARIO_SHIFT;
#ifdef OIS_CENTERING_SHIFT_ENABLE
	if ((sub_scene == ISS_SUB_SCENARIO_FHD_60FPS) || (scene == FIMC_IS_SCENARIO_SWVDIS)
		|| (scene == FIMC_IS_SCENARIO_AUTO_DUAL)) /* scene == FIMC_IS_SCENARIO_SWVDIS : UHD */
		ois_centering_shift_enable = true;
	else
		ois_centering_shift_enable = false;
#endif
	/* Usage
		1) AF position size bit : 10bit => AF Boundary : (1 << 7)
		2) AF position size bit : 9bit => AF Boundary : (1 << 6)
	*/
	if (resolution >= ACTUATOR_POS_SIZE_8BIT && resolution <= ACTUATOR_POS_SIZE_10BIT) {
		af_boundary = (1 << (resolution - 3));
	} else { /* Default value */
		af_boundary = AF_BOUNDARY;
	}

	af_value = position / af_boundary;

	if (ois->device == SENSOR_POSITION_REAR) {
		shift_x = (short)ois_shift_x[af_value];
		shift_y = (short)ois_shift_y[af_value];

		shift_x = (2 * (((short)ois_shift_x[af_value + 1] - shift_x) *
					(short)((position - AF_BOUNDARY * af_value) / 2)) / (short)AF_BOUNDARY + shift_x);
		shift_y = (2 * (((short)ois_shift_y[af_value + 1] - shift_y) *
					(short)((position - AF_BOUNDARY * af_value) / 2)) / (short)AF_BOUNDARY + shift_y);
#ifdef OIS_CENTERING_SHIFT_ENABLE
		if (ois_centering_shift_enable == true) {
			shift_x += (short)ois_centering_shift_x;
			shift_y += (short)ois_centering_shift_y;
		}
#endif
	} else if (ois->device == SENSOR_POSITION_REAR2) {
		shift_x = (short)ois_shift_x_rear2[af_value];
		shift_y = (short)ois_shift_y_rear2[af_value];

		shift_x = (2 * (((short)ois_shift_x_rear2[af_value + 1] - shift_x) *
					(short)((position - AF_BOUNDARY * af_value) / 2)) / (short)AF_BOUNDARY + shift_x);
		shift_y = (2 * (((short)ois_shift_y_rear2[af_value + 1] - shift_y) *
					(short)((position - AF_BOUNDARY * af_value) / 2)) / (short)AF_BOUNDARY + shift_y);
#ifdef OIS_CENTERING_SHIFT_ENABLE
		if (ois_centering_shift_enable == true) {
			shift_x += (short)ois_centering_shift_x_rear2;
			shift_y += (short)ois_centering_shift_y_rear2;
		}
#endif
	}
	write_data[0] = (shift_x & 0xFF);
	write_data[1] = (shift_x & 0xFF00) >> 8;
	write_data[2] = (shift_y & 0xFF);
	write_data[3] = (shift_y & 0xFF00) >> 8;

	dbg_ois("%s: shift_x(%d), shift_y(%d)\n", __func__, shift_x, shift_y);
	dbg_ois("%s: write_data[0](%d), write_data[1](%d), write_data[2](%d), write_data[3](%d)\n",
		__func__, write_data[0], write_data[1], write_data[2], write_data[3]);

	I2C_MUTEX_LOCK(ois->i2c_lock);
	if (ois->device == SENSOR_POSITION_REAR)
		ret = fimc_is_ois_i2c_write_multi(ois->client, 0x004C, write_data, 6);
	else if (ois->device == SENSOR_POSITION_REAR2)
		ret = fimc_is_ois_i2c_write_multi(ois->client, 0x0098, write_data, 6);

	if (ret < 0)
		err("i2c write multi is fail\n");
	I2C_MUTEX_UNLOCK(ois->i2c_lock);

p_err:
	return ret;
}

u8 fimc_is_read_ois_mode_s6(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 mode = OPTICAL_STABILIZATION_MODE_OFF;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		return OPTICAL_STABILIZATION_MODE_OFF;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		return OPTICAL_STABILIZATION_MODE_OFF;
	}

	ret = fimc_is_ois_i2c_read(client, 0x0002, &mode);
	if (ret) {
		err("i2c read fail\n");
		return OPTICAL_STABILIZATION_MODE_OFF;
	}

	switch(mode) {
		case 0x00:
			mode = OPTICAL_STABILIZATION_MODE_STILL;
			break;
		case 0x01:
			mode = OPTICAL_STABILIZATION_MODE_VIDEO;
			break;
		case 0x05:
			mode = OPTICAL_STABILIZATION_MODE_CENTERING;
			break;
		case 0x13:
			mode = OPTICAL_STABILIZATION_MODE_STILL_ZOOM;
			break;
		case 0x14:
			mode = OPTICAL_STABILIZATION_MODE_VDIS;
			break;
		default:
			dbg_ois("%s: ois_mode value(%d)\n", __func__, mode);
			break;
	}

	return mode;
}

int fimc_is_set_ois_mode_s6(struct v4l2_subdev *subdev, int mode)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		ret = -EINVAL;
		return ret;
	}

	if (ois->fadeupdown == false) {
		if (ois_fadeupdown == false) {
			ois_fadeupdown = true;
			fimc_is_ois_set_ggfadeupdown_s6(subdev, 1000, 1000);
		}
		ois->fadeupdown = true;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	if (mode == ois->pre_ois_mode) {
		return ret;
	}

	ois->pre_ois_mode = mode;

	I2C_MUTEX_LOCK(ois->i2c_lock);
	switch(mode) {
		case OPTICAL_STABILIZATION_MODE_STILL:
			fimc_is_ois_i2c_write(client, 0x0002, 0x00);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_VIDEO:
			fimc_is_ois_i2c_write(client, 0x0002, 0x01);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_CENTERING:
			fimc_is_ois_i2c_write(client, 0x0002, 0x05);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_STILL_ZOOM:
			fimc_is_ois_i2c_write(client, 0x0002, 0x13);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_VDIS:
			fimc_is_ois_i2c_write(client, 0x0002, 0x14);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			if (ois->pre_coef == 255)
				ois->pre_coef = 100;
			break;
		case OPTICAL_STABILIZATION_MODE_SINE_X:
			fimc_is_ois_i2c_write(client, 0x0018, 0x01);
			fimc_is_ois_i2c_write(client, 0x0019, 0x01);
			fimc_is_ois_i2c_write(client, 0x001A, 0x2D);
			fimc_is_ois_i2c_write(client, 0x0002, 0x03);
			msleep(20);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		case OPTICAL_STABILIZATION_MODE_SINE_Y:
			fimc_is_ois_i2c_write(client, 0x0018, 0x02);
			fimc_is_ois_i2c_write(client, 0x0019, 0x01);
			fimc_is_ois_i2c_write(client, 0x001A, 0x2D);
			fimc_is_ois_i2c_write(client, 0x0002, 0x03);
			msleep(20);
			fimc_is_ois_i2c_write(client, 0x0000, 0x01);
			break;
		default:
			dbg_ois("%s: ois_mode value(%d)\n", __func__, mode);
			break;
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}

void fimc_is_status_check_s6(struct i2c_client *client)
{
	u8 ois_status_check = 0;
	int retry_count = 0;

	do {
		fimc_is_ois_i2c_read(client, 0x000E, &ois_status_check);
		if (ois_status_check == 0x14)
			break;
		usleep_range(1000,1000);
		retry_count++;
	} while (retry_count < OIS_MEM_STATUS_RETRY);

	if (retry_count == OIS_MEM_STATUS_RETRY)
		err("%s, ois status check fail, retry_count(%d)\n", __func__, retry_count);

	if (ois_status_check == 0)
		err("%s, ois Memory access fail\n", __func__);
}

signed long long hex2float_kernel(unsigned int hex_data, int endian)
{
	const signed long long scale = SCALE;
	unsigned int s,e,m;
	signed long long res;

	if(endian == eBIG_ENDIAN) hex_data = SWAP32(hex_data);

	s = hex_data>>31, e = (hex_data>>23)&0xff, m = hex_data&0x7fffff;
	res = (e >= 150) ? ((scale*(8388608 + m))<<(e-150)) : ((scale*(8388608 + m))>>(150-e));
	if(s == 1) res *= -1;

	return res;
}

int fimc_calculate_shift_value(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int i = 0;
	short ois_shift_x_cal = 0;
	short ois_shift_y_cal = 0;
	u8 cal_data[2];
	u8 shift_available = 0;
	struct fimc_is_ois *ois = NULL;
#ifdef OIS_CENTERING_SHIFT_ENABLE
	char *cal_buf;
	u32 Wide_XGG_Hex, Wide_YGG_Hex, Tele_XGG_Hex, Tele_YGG_Hex;
	signed long long Wide_XGG, Wide_YGG, Tele_XGG, Tele_YGG;
	u32 Image_Shift_x_Hex, Image_Shift_y_Hex;
	signed long long Image_Shift_x, Image_Shift_y;
	u8 read_multi[4] = {0, };
	signed long long Coef_angle_X , Coef_angle_Y;
	signed long long Wide_Xshift, Tele_Xshift, Wide_Yshift, Tele_Yshift;
	const signed long long scale = SCALE*SCALE;
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor *device = NULL;
#endif

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if(!ois) {
		err("%s, ois subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	I2C_MUTEX_LOCK(ois->i2c_lock);

	/* use user data */
	ret |= fimc_is_ois_i2c_write(ois->client, 0x000F, 0x40);
	cal_data[0] = 0x00;
	cal_data[1] = 0x02;
	ret |= fimc_is_ois_i2c_write_multi(ois->client, 0x0010, cal_data, 4);
	ret |= fimc_is_ois_i2c_write(ois->client, 0x000E, 0x04);
	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		device = v4l2_get_subdev_hostdata(subdev);
		if (device) {
			fimc_is_sec_get_hw_param(&hw_param, device->position);
		}
		if (hw_param)
			hw_param->i2c_ois_err_cnt++;
#endif
		err("ois user data write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	fimc_is_status_check_s6(ois->client);
#ifdef OIS_CENTERING_SHIFT_ENABLE
	fimc_is_ois_i2c_read_multi(ois->client, 0x0254, read_multi, 4);
	Wide_XGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	fimc_is_ois_i2c_read_multi(ois->client, 0x0554, read_multi, 4);
	Tele_XGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	fimc_is_ois_i2c_read_multi(ois->client, 0x0258, read_multi, 4);
	Wide_YGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	fimc_is_ois_i2c_read_multi(ois->client, 0x0558, read_multi, 4);
	Tele_YGG_Hex = (read_multi[3] << 24) | (read_multi[2] << 16) | (read_multi[1] << 8) | (read_multi[0]);

	Wide_XGG = hex2float_kernel(Wide_XGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Wide_YGG = hex2float_kernel(Wide_YGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Tele_XGG = hex2float_kernel(Tele_XGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Tele_YGG = hex2float_kernel(Tele_YGG_Hex,eLIT_ENDIAN); // unit : 1/SCALE

	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);

	Image_Shift_x_Hex = (cal_buf[0x6C7C+3] << 24) | (cal_buf[0x6C7C+2] << 16) | (cal_buf[0x6C7C+1] << 8) | (cal_buf[0x6C7C]);
	Image_Shift_y_Hex = (cal_buf[0x6C80+3] << 24) | (cal_buf[0x6C80+2] << 16) | (cal_buf[0x6C80+1] << 8) | (cal_buf[0x6C80]);

	Image_Shift_x = hex2float_kernel(Image_Shift_x_Hex,eLIT_ENDIAN); // unit : 1/SCALE
	Image_Shift_y = hex2float_kernel(Image_Shift_y_Hex,eLIT_ENDIAN); // unit : 1/SCALE

	Image_Shift_y += 90 * SCALE;

	#define ABS(a)				((a) > 0 ? (a) : -(a))

	// Calc w/t x shift
	//=======================================================
	Coef_angle_X = (ABS(Image_Shift_x) > SH_THRES) ? Coef_angle_max : RND_DIV(ABS(Image_Shift_x), 228);

	Wide_Xshift = Gyrocode * Coef_angle_X * Wide_XGG;
	Tele_Xshift = Gyrocode * Coef_angle_X * Tele_XGG;

	Wide_Xshift = (Image_Shift_x > 0) ? Wide_Xshift	: Wide_Xshift*-1;
	Tele_Xshift = (Image_Shift_x > 0) ? Tele_Xshift*-1 : Tele_Xshift;

	// Calc w/t y shift
	//=======================================================
	Coef_angle_Y = (ABS(Image_Shift_y) > SH_THRES) ? Coef_angle_max : RND_DIV(ABS(Image_Shift_y), 228);

	Wide_Yshift = Gyrocode * Coef_angle_Y * Wide_YGG;
	Tele_Yshift = Gyrocode * Coef_angle_Y * Tele_YGG;

	Wide_Yshift = (Image_Shift_y > 0) ? Wide_Yshift*-1 : Wide_Yshift;
	Tele_Yshift = (Image_Shift_y > 0) ? Tele_Yshift*-1 : Tele_Yshift;

	// Calc output variable
	//=======================================================
	ois_centering_shift_x = (int)RND_DIV(Wide_Xshift, scale);
	ois_centering_shift_y = (int)RND_DIV(Wide_Yshift, scale);
	ois_centering_shift_x_rear2 = (int)RND_DIV(Tele_Xshift, scale);
	ois_centering_shift_y_rear2 = (int)RND_DIV(Tele_Yshift, scale);
#endif

	fimc_is_ois_i2c_read(ois->client, 0x0100, &shift_available);
	if (shift_available != 0x11) {
		ois->ois_shift_available = false;
		dbg_ois("%s, OIS AF CAL(0x%x) does not installed.\n", __func__, shift_available);
	} else {
		ois->ois_shift_available = true;

		/* OIS Shift CAL DATA */
		for (i = 0; i < 9; i++) {
			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(ois->client, 0x0110 + (i * 2), cal_data, 2);
			ois_shift_x_cal = (cal_data[1] << 8) | (cal_data[0]);

			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(ois->client, 0x0122 + (i * 2), cal_data, 2);
			ois_shift_y_cal = (cal_data[1] << 8) | (cal_data[0]);

			if (ois_shift_x_cal > (short)32767)
				ois_shift_x[i] = ois_shift_x_cal - 65536;
			else
				ois_shift_x[i] = ois_shift_x_cal;

			if (ois_shift_y_cal > (short)32767)
				ois_shift_y[i] = ois_shift_y_cal - 65536;
			else
				ois_shift_y[i] = ois_shift_y_cal;
		}
	}

	shift_available = 0;
	fimc_is_ois_i2c_read(ois->client, 0x0101, &shift_available);
	if (shift_available != 0x11) {
		dbg_ois("%s, REAR2 OIS AF CAL(0x%x) does not installed.\n", __func__, shift_available);
	} else {
		/* OIS Shift CAL DATA REAR2 */
		for (i = 0; i < 9; i++) {
			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(ois->client, 0x0140 + (i * 2), cal_data, 2);
			ois_shift_x_cal = (cal_data[1] << 8) | (cal_data[0]);

			cal_data[0] = 0;
			cal_data[1] = 0;
			fimc_is_ois_i2c_read_multi(ois->client, 0x0152 + (i * 2), cal_data, 2);
			ois_shift_y_cal = (cal_data[1] << 8) | (cal_data[0]);

			if (ois_shift_x_cal > (short)32767)
				ois_shift_x_rear2[i] = ois_shift_x_cal - 65536;
			else
				ois_shift_x_rear2[i] = ois_shift_x_cal;

			if (ois_shift_y_cal > (short)32767)
				ois_shift_y_rear2[i] = ois_shift_y_cal - 65536;
			else
				ois_shift_y_rear2[i] = ois_shift_y_cal;

		}
	}

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}

int fimc_is_ois_init_s6(struct v4l2_subdev *subdev)
{
	int ret = 0;
#ifdef USE_OIS_SLEEP_MODE
	u8 read_gyrocalcen = 0;
#endif
	u8 data[2] = {0, };
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if(!ois) {
		err("%s, ois subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	sensor_peri = ois->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = ois->client;
	if (!client) {
		err("%s, client is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois->ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
	ois->pre_ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
	ois->coef = 0;
	ois->pre_coef = 255;
	ois->fadeupdown = false;
	ois->initial_centering_mode = false;
#ifdef CAMERA_REAR2_OIS
	ois->ois_power_mode = -1;
#endif

#ifdef USE_OIS_SLEEP_MODE
	I2C_MUTEX_LOCK(ois->i2c_lock);
	fimc_is_ois_i2c_read(ois->client, 0x00BF, &read_gyrocalcen);
	if ((read_gyrocalcen == 0x01 && module->position == SENSOR_POSITION_REAR2) || //tele already enabled
		(read_gyrocalcen == 0x02 && module->position == SENSOR_POSITION_REAR)) { //wide already enabled
		ois->ois_shift_available = true;
		info("%s %d sensor(%d) is already initialized.\n", __func__, __LINE__, ois->device);
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BF, 0x03);
		if (ret < 0)
			err("ois 0x00BF write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);
#else
	if ((ois_wide_init == true && module->position == SENSOR_POSITION_REAR2) ||
		(ois_tele_init == true && module->position == SENSOR_POSITION_REAR)) {
		info("%s %d sensor(%d) is already initialized.\n", __func__, __LINE__, ois->device);
		ois_wide_init = ois_tele_init = true;
		ois->ois_shift_available = true;

		return ret;
	}
#endif

	fimc_calculate_shift_value(subdev);

	I2C_MUTEX_LOCK(ois->i2c_lock);

#ifdef USE_OIS_SLEEP_MODE
	if (module->position == SENSOR_POSITION_REAR2) {
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BF, 0x02);
		if (ret < 0)
			err("ois 0x00BF write is fail");
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x02);
		if (ret < 0)
			err("ois 0x00BE write is fail");
		else
			fimc_is_ois_set_oissel_info(0x02);

	} else if (module->position == SENSOR_POSITION_REAR) {
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BF, 0x01);
		if (ret < 0)
			err("ois 0x00BF write is fail");
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x01);
		if (ret < 0)
			err("ois 0x00BE write is fail");
		else
			fimc_is_ois_set_oissel_info(0x01);

	} else {
		info("%s : module->position = %d is invalid\n", __func__, module->position);
	}
#else
	if (module->position == SENSOR_POSITION_REAR2) {
		ois_tele_init = true;
	} else if (module->position == SENSOR_POSITION_REAR) {
		ois_wide_init = true;
	}
#endif

	ret = fimc_is_ois_i2c_read_multi(client, 0x021A, data, 2);
	ois_center_x = (data[1] << 8) | (data[0]);
	ret |= fimc_is_ois_i2c_read_multi(client, 0x021C, data, 2);
	ois_center_y = (data[1] << 8) | (data[0]);
	if (ret < 0)
		err("i2c read fail\n");

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

#if defined(USE_OIS_SHIFT_FOR_APERTURE)
	ois_center_y += OIS_SHIFT_OFFSET_VALUE_NORMAL;
#endif

	info("%s\n", __func__);
	return ret;
}

int fimc_is_ois_deinit_s6(struct v4l2_subdev *subdev)
{
	int ret = 0;

	WARN_ON(!subdev);

	ois_fadeupdown = false;

	dbg_ois("%s\n", __func__);

	return ret;
}

#ifdef USE_OIS_SLEEP_MODE
int fimc_is_ois_start_s6(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if(!ois) {
		err("%s, ois subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	sensor_peri = ois->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = ois->client;
	if (!client) {
		err("%s, client is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	I2C_MUTEX_LOCK(ois->i2c_lock);


	if ((ois_wide_start == true && module->position == SENSOR_POSITION_REAR2) || //tele already enabled
		(ois_tele_start == true && module->position == SENSOR_POSITION_REAR)) {
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x03);
		if (ret < 0)
			err("ois 0x00BE write is fail");
		else
			fimc_is_ois_set_oissel_info(0x03);
	} else if (ois_tele_start == false && module->position == SENSOR_POSITION_REAR) {
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x01);
		if (ret < 0)
			err("ois 0x00BE write is fail");
		else
			fimc_is_ois_set_oissel_info(0x01);
	} else if (ois_wide_start == false && module->position == SENSOR_POSITION_REAR2) {
		ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x02);
		if (ret < 0)
			err("ois 0x00BE write is fail");
		else
			fimc_is_ois_set_oissel_info(0x02);
	}else {
	}

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	if (module->position == SENSOR_POSITION_REAR) {
		ois_wide_start = true;
	} else if(module->position == SENSOR_POSITION_REAR2) {
		ois_tele_start = true;
	}

	dbg_ois("%s Camera[%d]\n", __func__, module->position);
	return ret;
}

int fimc_is_ois_stop_s6(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u8 read_oissel = 0;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if(!ois) {
		err("%s, ois subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	sensor_peri = ois->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = ois->client;
	if (!client) {
		err("%s, client is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	I2C_MUTEX_LOCK(ois->i2c_lock);

	read_oissel = fimc_is_ois_get_oissel_info();

	if (read_oissel == 0x03) {
		if (module->position == SENSOR_POSITION_REAR2) {
			ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x01);
			if (ret < 0)
				err("ois 0x00BE write is fail");
			else
				fimc_is_ois_set_oissel_info(0x01);
		}else if (module->position == SENSOR_POSITION_REAR) {
			ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x02);
			if (ret < 0)
				err("ois 0x00BE write is fail");
			else
				fimc_is_ois_set_oissel_info(0x02);
		}
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	if (module->position == SENSOR_POSITION_REAR) {
		ois_wide_start = false;
	} else if(module->position == SENSOR_POSITION_REAR2) {
		ois_tele_start = false;
	}

	dbg_ois("%s Camera[%d]\n", __func__, module->position);
	return ret;
}
#endif

int fimc_is_ois_set_coef_s6(struct v4l2_subdev *subdev, u8 coef)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		return -EINVAL;
	}

	if (ois->pre_coef == coef)
		return ret;

	client = ois->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	dbg_ois("%s %d\n", __func__, coef);

	I2C_MUTEX_LOCK(ois->i2c_lock);
	ret = fimc_is_ois_i2c_write(client, 0x005e, coef);
	if (ret) {
		err("%s i2c write fail\n", __func__);
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	ois->pre_coef = coef;

	return ret;
}

int fimc_is_ois_set_centering_s6(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		return -EINVAL;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	ret = fimc_is_ois_i2c_write(client, 0x0002, 0x05);
	if (ret) {
		err("i2c write fail\n");
		return ret;
	}

	ois->pre_ois_mode = OPTICAL_STABILIZATION_MODE_CENTERING;

	return ret;
}

int fimc_is_ois_set_still_s6(struct fimc_is_core *core)
{
	int ret = 0;
	struct i2c_client *client = NULL;

	client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_write(client, 0x0002, 0x00);
	if (ret) {
		err("i2c write fail\n");
		return ret;
	}

	return ret;
}

int fimc_is_ois_shift_s6(struct v4l2_subdev *subdev)
{
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	u8 data[2];
	int ret = 0;

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		return -EINVAL;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	I2C_MUTEX_LOCK(ois->i2c_lock);

	data[0] = (ois_center_x & 0xFF);
	data[1] = (ois_center_x & 0xFF00) >> 8;
	ret = fimc_is_ois_i2c_write_multi(ois->client, 0x0022, data, 4);
	if (ret < 0)
		err("i2c write multi is fail\n");

	data[0] = (ois_center_y & 0xFF);
	data[1] = (ois_center_y & 0xFF00) >> 8;
	ret = fimc_is_ois_i2c_write_multi(ois->client, 0x0024, data, 4);
	if (ret < 0)
		err("i2c write multi is fail\n");

	ret = fimc_is_ois_i2c_write(client, 0x0002, 0x02);
	if (ret < 0)
		err("i2c write multi is fail\n");

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}

#ifdef CAMERA_REAR2_OIS
int fimc_is_ois_set_power_mode(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	struct fimc_is_core *core;
	struct fimc_is_dual_info *dual_info = NULL;
	struct fimc_is_module_enum *module = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		return -EINVAL;
	}

	sensor_peri = ois->sensor_peri;
	if (!sensor_peri) {
		err("%s, sensor_peri is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	module = sensor_peri->module;
	if (!module) {
		err("%s, module is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is null");
		return -EINVAL;
	}

	dual_info = &core->dual_info;

	I2C_MUTEX_LOCK(ois->i2c_lock);

	if ((dual_info->mode != FIMC_IS_DUAL_MODE_NOTHING)
		|| (dual_info->mode == FIMC_IS_DUAL_MODE_NOTHING &&
			module->position == SENSOR_POSITION_REAR2)) {
		ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x03);
		ois->ois_power_mode = OIS_POWER_MODE_DUAL;
	} else {
		ret = fimc_is_ois_i2c_write(client, 0x00BE, 0x01);
		ois->ois_power_mode = OIS_POWER_MODE_SINGLE;
	}

	if (ret < 0)
		err("ois dual setting is fail");
	else
		info("%s ois power setting is %d\n", __func__, ois->ois_power_mode);

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	return ret;
}
#endif

int fimc_is_ois_set_ggfadeupdown_s6(struct v4l2_subdev *subdev, int up, int down)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;
	struct i2c_client *client = NULL;
	u8 status = 0;
	int retries = 100;
	u8 data[2];
	u8 write_data[4] = {0,};
#ifdef USE_OIS_SLEEP_MODE
	u8 read_sensorStart = 0;
#endif

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		return -EINVAL;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		return -EINVAL;
	}

	dbg_ois("%s up:%d down:%d\n", __func__, up, down);

	I2C_MUTEX_LOCK(ois->i2c_lock);

	ret = fimc_is_ois_i2c_write(ois->client, 0x00BE, 0x03);
	if (ret < 0) {
		err("ois Actuator output write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	ret = fimc_is_ois_i2c_write(ois->client, 0x0039, 0x01);
	if (ret < 0) {
		err("ois cactrl write is fail");
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}

	/* angle compensation 1.5->1.25
	 * before addr:0x0000, data:0x01
	 * write 0x3F558106
	 * write 0x3F558106
	 */
	write_data[0] = 0x06;
	write_data[1] = 0x81;
	write_data[2] = 0x55;
	write_data[3] = 0x3F;
	fimc_is_ois_i2c_write_multi(ois->client, 0x0348, write_data, 6);

	write_data[0] = 0x06;
	write_data[1] = 0x81;
	write_data[2] = 0x55;
	write_data[3] = 0x3F;
	fimc_is_ois_i2c_write_multi(ois->client, 0x03D8, write_data, 6);

#ifdef USE_OIS_SLEEP_MODE
	/* if camera is already started, skip VDIS setting */
	fimc_is_ois_i2c_read(ois->client, 0x00BF, &read_sensorStart);
	if (read_sensorStart == 0x03) {
		I2C_MUTEX_UNLOCK(ois->i2c_lock);
		return ret;
	}
#endif
	/* set fadeup */
	data[0] = up & 0xFF;
	data[1] = (up >> 8) & 0xFF;
	ret = fimc_is_ois_i2c_write_multi(client, 0x0238, data, 4);
	if (ret < 0)
		err("%s i2c write fail\n", __func__);

	/* set fadedown */
	data[0] = down & 0xFF;
	data[1] = (down >> 8) & 0xFF;
	ret = fimc_is_ois_i2c_write_multi(client, 0x023a, data, 4);
	if (ret < 0)
		err("%s i2c write fail\n", __func__);

	/* wait idle status
	 * 100msec delay is needed between "ois_power_on" and "ois_mode_s6".
	 */
	do {
		fimc_is_ois_i2c_read(client, 0x0001, &status);
		if (status == 0x01 || status == 0x13)
			break;
		if (--retries < 0) {
			err("%s : read register fail!. status: 0x%x\n", __func__, status);
			ret = -1;
			break;
		}
		usleep_range(1000, 1100);
	} while (status != 0x01);

	I2C_MUTEX_UNLOCK(ois->i2c_lock);

	dbg_ois("%s retryCount = %d , status = 0x%x\n", __func__, 100 - retries, status);

	return ret;
}

static struct fimc_is_ois_ops ois_ops_s6 = {
	.ois_init = fimc_is_ois_init_s6,
	.ois_deinit = fimc_is_ois_deinit_s6,
#ifdef USE_OIS_SLEEP_MODE
	.ois_start = fimc_is_ois_start_s6,
	.ois_stop = fimc_is_ois_stop_s6,
#endif
	.ois_set_mode = fimc_is_set_ois_mode_s6,
	.ois_shift_compensation = fimc_is_ois_shift_compensation_s6,
	.ois_fw_update = fimc_is_ois_fw_update_s6,
	.ois_self_test = fimc_is_ois_self_test_s6,
	.ois_diff_test = fimc_is_ois_diff_test_s6,
	.ois_auto_test = fimc_is_ois_auto_test_s6,
#ifdef CAMERA_REAR2_OIS
	.ois_auto_test_rear2 = fimc_is_ois_auto_test_rear2_s6,
	.ois_set_power_mode = fimc_is_ois_set_power_mode,
#endif
	.ois_check_fw = fimc_is_ois_check_fw_s6,
	.ois_enable = fimc_is_ois_enable_s6,
	.ois_offset_test = fimc_is_ois_offset_test_s6,
	.ois_get_offset_data = fimc_is_ois_get_offset_data_s6,
	.ois_gyro_sleep = fimc_is_ois_gyro_sleep_s6,
	.ois_exif_data = fimc_is_ois_exif_data_s6,
	.ois_read_status = fimc_is_ois_read_status_s6,
	.ois_read_cal_checksum = fimc_is_ois_read_cal_checksum_s6,
	.ois_set_coef = fimc_is_ois_set_coef_s6,
	.ois_read_fw_ver = fimc_is_ois_read_fw_ver_s6,
	.ois_center_shift = fimc_is_ois_shift_s6,
	.ois_set_center = fimc_is_ois_set_centering_s6,
	.ois_read_mode = fimc_is_read_ois_mode_s6,
};

int ois_rumbaS6_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct v4l2_subdev *subdev_ois;
	struct fimc_is_ois *ois;
	struct device *dev;
	struct device_node *dnode;
	u32 sensor_id[FIMC_IS_SENSOR_COUNT] = {0, };
	struct fimc_is_device_ois *ois_device;
	struct fimc_is_vender_specific *specific;
	const u32 *sensor_id_spec;
	u32 sensor_id_len;
	int i = 0;
	ois_wide_init = false;
	ois_tele_init = false;
	ois_fadeupdown = false;

	WARN_ON(!fimc_is_dev);
	WARN_ON(!client);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	sensor_id_spec = of_get_property(dnode, "id", &sensor_id_len);
	if (!sensor_id_spec) {
		err("sensor_id num read is fail(%d)", ret);
		goto p_err;
	}

	sensor_id_len /= (unsigned int)sizeof(*sensor_id_spec);

	ret = of_property_read_u32_array(dnode, "id", sensor_id, sensor_id_len);
	if (ret) {
		err("sensor_id read is fail(%d)", ret);
		goto p_err;
	}

	for (i = 0; i < sensor_id_len; i++) {
		device = &core->sensor[sensor_id[i]];
		if (!device) {
			err("sensor device is NULL");
			ret = -EPROBE_DEFER;
			goto p_err;
		}
	}

	ois = kzalloc(sizeof(struct fimc_is_ois) * sensor_id_len, GFP_KERNEL);
	if (!ois) {
		err("fimc_is_ois is NULL");
		return -ENOMEM;
	}

	subdev_ois = kzalloc(sizeof(struct v4l2_subdev) * sensor_id_len, GFP_KERNEL);
	if (!subdev_ois) {
		err("subdev_ois is NULL");
		ret = -ENOMEM;
		kfree(ois);
		goto p_err;
	}

	ois_device = kzalloc(sizeof(struct fimc_is_device_ois), GFP_KERNEL);
	if (!ois_device) {
		err("fimc_is_device_ois is NULL");
		kfree(ois);
		kfree(subdev_ois);
		return -ENOMEM;
	}
	ois_device->ois_ops = &ois_ops_s6;
	ois_device->ois_hsi2c_status = false;

	for (i = 0; i < sensor_id_len; i++) {
		probe_info("%s sensor_id %d\n", __func__, sensor_id[i]);

		ois[i].ois_ops = &ois_ops_s6;
		ois[i].id = OIS_NAME_RUMBA_S6;
		ois[i].subdev = &subdev_ois[i];
		ois[i].device = sensor_id[i];
		ois[i].client = client;
		ois[i].ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
		ois[i].pre_ois_mode = OPTICAL_STABILIZATION_MODE_OFF;
		ois[i].ois_shift_available = false;
		ois[i].i2c_lock = NULL;

		device = &core->sensor[sensor_id[i]];
		device->subdev_ois = &subdev_ois[i];
		device->ois = &ois[i];

		v4l2_i2c_subdev_init(&subdev_ois[i], client, &subdev_ops);
		v4l2_set_subdevdata(&subdev_ois[i], &ois[i]);
		v4l2_set_subdev_hostdata(&subdev_ois[i], ois_device);
		snprintf(subdev_ois[i].name, V4L2_SUBDEV_NAME_SIZE, "ois->subdev.%d", ois[i].id);

		probe_info("%s done\n", __func__);
	}

	/* reset i2c client's data to ois_device */
	i2c_set_clientdata(client, ois_device);

	specific = core->vender.private_data;
	specific->ois_ver_read = false;

p_err:
	return ret;
}

static int ois_rumbaS6_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

static const struct of_device_id exynos_fimc_is_ois_rumbaS6_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-ois-rumbaS6",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_ois_rumbaS6_match);

static const struct i2c_device_id ois_rumbaS6_idt[] = {
	{ OIS_NAME, 0 },
	{},
};

static struct i2c_driver ois_rumbaS6_driver = {
	.driver = {
		.name	= OIS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_fimc_is_ois_rumbaS6_match
	},
	.probe	= ois_rumbaS6_probe,
	.remove	= ois_rumbaS6_remove,
	.id_table = ois_rumbaS6_idt
};
module_i2c_driver(ois_rumbaS6_driver);

MODULE_DESCRIPTION("OIS driver for Rumba S6");
MODULE_AUTHOR("kyoungho yun <kyoungho.yun@samsung.com>");
MODULE_LICENSE("GPL v2");
