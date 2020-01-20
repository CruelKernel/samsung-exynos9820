/*
 * Samsung Exynos5 SoC series ois driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-core.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-helper-ois-i2c.h"
#include "fimc-is-ois-rumbaS4.h"
#include "fimc-is-ois.h"
#include "fimc-is-device-ois.h"
#include "fimc-is-vender-specific.h"

#define OIS_NAME "OIS_RUMBA_S4"

static int ois_shift_x[POSITION_NUM];
static int ois_shift_y[POSITION_NUM];

#ifdef CONFIG_OIS_DIRECT_FW_CONTROL
static u8 fw_data[OIS_BOOT_FW_SIZE + OIS_FW_SIZE] = {0,};
static char ois_fw_ver[FIMC_IS_OIS_FW_VER_SIZE];
static char p_ois_fw_ver[FIMC_IS_OIS_FW_VER_SIZE];
#endif

static const struct v4l2_subdev_ops subdev_ops;

#ifdef CONFIG_OIS_DIRECT_FW_CONTROL
int fimc_is_ois_read_cal(struct fimc_is_ois *ois)
{
	int ret = 0;
	u8 version[20] = {0, };
	u8 read_ver[3] = {0, };

	WARN_ON(!ois);

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

	ret = fimc_is_ois_write_multi(ois->client, 0x0100, version, 0x16);
	if (ret < 0) {
		err("ois read version fail");
		return ret;
	}

	ret |= fimc_is_ois_read(ois->client, 0x0118, &read_ver[0]);
	ret |= fimc_is_ois_read(ois->client, 0x0119, &read_ver[1]);
	ret |= fimc_is_ois_read(ois->client, 0x011A, &read_ver[2]);
	if (ret < 0) {
		err("i2c cmd fail\n");
		ret = -EINVAL;
		return ret;
	}

	ois_fw_ver[FW_OIS_VERSION] = read_ver[0];

	info("%s: fw_ver(%c)\n", __func__,
			ois_fw_ver[FW_OIS_VERSION]);

	return ret;
}

int fimc_is_ois_read_user_data(struct fimc_is_ois *ois)
{
	int ret = 0;
	int i = 0;
	u8 send_data[2];
	u8 read_data[73] = {0, };

	WARN_ON(!ois);

	/* OIS servo OFF */
	ret = fimc_is_ois_write(ois->client ,0x0000, 0x00);
	if (ret < 0) {
		err("ois servo off fail");
		return ret;
	}

	/* User Data Area & Address Setting step1 */
	send_data[0] = 0x00;
	send_data[1] = 0x00;
	ret |= fimc_is_ois_write(ois->client ,0x000F, 0x12);
	ret |= fimc_is_ois_write_multi(ois->client, 0x0010, send_data, 4);
	ret |= fimc_is_ois_write(ois->client ,0x000E, 0x04);
	if (ret < 0) {
		err("ois step1 setting fail");
		return ret;
	}

	ret = fimc_is_ois_read_multi(ois->client, 0x0100, read_data, 73);
	if (ret < 0) {
		err("ois read fail");
		return ret;
	}

	for (i = 0; i < 72; i = i + 4) {
		info("OIS[user data::0x%02x%02x%02x%02x]\n",
				read_data[i], read_data[i + 1], read_data[i + 2], read_data[i + 3]);
	}

	/* User Data Area & Address Setting step2 */
	send_data[0] = 0x00;
	send_data[1] = 0x04;
	ret |= fimc_is_ois_write(ois->client ,0x000F, 0x0E);
	ret |= fimc_is_ois_write_multi(ois->client, 0x0010, send_data, 4);
	ret |= fimc_is_ois_write(ois->client ,0x000E, 0x04);
	if (ret < 0) {
		err("ois step2 setting fail");
		return ret;
	}

	return ret;
}

int fimc_is_ois_fw_ver_copy(struct fimc_is_ois *ois, u8 *buf, long size)
{
	int ret = 0;

	WARN_ON(!ois);

	memcpy(p_ois_fw_ver, (void *)buf + OIS_BIN_HEADER, size);
	memcpy(&p_ois_fw_ver[3], (void *)buf + OIS_BIN_HEADER - 4, 4);
	memcpy(&fw_data, (void *)buf, OIS_BOOT_FW_SIZE + OIS_FW_SIZE);

	return ret;
}

int fimc_is_ois_fw_version_read(struct fimc_is_ois *ois)
{
	int ret = 0;
	char version[7] = {0, };

	WARN_ON(!ois);

	ret = fimc_is_ois_read(ois->client, 0x00FB, &version[0]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	ret = fimc_is_ois_read(ois->client, 0x00F9, &version[1]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	ret = fimc_is_ois_read(ois->client, 0x00F8, &version[2]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	ret = fimc_is_ois_read(ois->client, 0x007C, &version[3]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	ret = fimc_is_ois_read(ois->client, 0x007D, &version[4]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	ret = fimc_is_ois_read(ois->client, 0x007E, &version[5]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	ret = fimc_is_ois_read(ois->client, 0x007F, &version[6]);
	if (ret < 0) {
		err("i2c read fail\n");
		return ret;
	}

	memcpy(ois_fw_ver, version, 7);

	info("%s: ois_fw_ver[0](%c)\n", __func__,
			ois_fw_ver[0]);

	return ret;
}

int fimc_is_ois_fw_ready(struct fimc_is_ois *ois)
{
	int ret = 0;

	u8 ois_status[4] = {0, };
	u32 ois_status32 = 0;

	WARN_ON(!ois);

	ret = fimc_is_ois_read_multi(ois->client, 0x00FC, ois_status, 4);
	if (ret < 0) {
		err("ois i2c status read fail\n");
		return ret;
	}

	ois_status32 = (ois_status[3] << 24) | (ois_status[2] << 16) |
		(ois_status[1] << 8) | (ois_status[0]);

	info("%s: ois_status32(0x%x)\n", __func__, ois_status32);
	if (ois_status32 == OIS_FW_UPDATE_ERROR_STATUS) {
		ret = fimc_is_ois_read_cal(ois);
		if (ret < 0) {
			err("Failed to ois read cal");
			return ret;
		}
	} else {
		ret = fimc_is_ois_fw_version_read(ois);
		if (ret < 0) {
			err("Failed to ois read cal");
			return ret;
		}

		ret = fimc_is_ois_read_user_data(ois);
		if (ret < 0) {
			err("Failed to ois read user data");
			return ret;
		}
	}

	switch(ois_fw_ver[FW_OIS_VERSION]) {
	case 'A':
	case 'C':
		ret = fimc_is_ois_fw_open(ois, FIMC_IS_OIS_FW_NAME_DOM);
		if (ret < 0) {
			err("OIS %c load is fail\n", ois_fw_ver[FW_OIS_VERSION]);
			return ret;
		}
		break;
	case 'B':
	case 'D':
		ret = fimc_is_ois_fw_open(ois, FIMC_IS_OIS_FW_NAME_SEC);
		if (ret < 0) {
			err("OIS %c load is fail\n", ois_fw_ver[FW_OIS_VERSION]);
			return ret;
		}
		break;
	default:
		err("OIS FW binary load fail\n");
		ret = -EINVAL;
		return ret;
	}

	return ret;
}

int fimc_is_ois_fw_download(struct fimc_is_ois *ois)
{
	int position, block = 0;
	int ret = 0;
	u8 send_data[256];

	WARN_ON(!ois);

	/* Update a User Program */
	/* SET FWUP_CTRL REG */
	/* FWUP_DSIZE=256Byte, FWUP_MODE=PROG, FWUPEN=Start */
	position = 0;
	/* FWUPCTRL REG(0x000C) 1Byte Send */
	send_data[0] = 0x75;
	ret = fimc_is_ois_write(ois->client, 0x000C, send_data[0]);
	if (ret < 0) {
		err("%s: Firmware setting is fail\n", __func__);
		return ret;
	}

	/* Write UserProgram Data */
	for (block = 0; block < 112; block++) {
		/* 0x1000 - 0x6FFF (RUMBA-SA) */
		memcpy(send_data, &fw_data[position], (size_t)FW_TRANS_SIZE);
		/* FLS_DATA REG(0x0100) 256Byte Send */
		ret = fimc_is_ois_write_multi(ois->client, 0x0100, send_data, FW_TRANS_SIZE + 2);
		if (ret < 0) {
			err("ois fw download is fail");
			return ret;
		}
		position += FW_TRANS_SIZE;
	}

	info("%s: ois fw download success\n", __func__);
	return ret;
}

int fimc_is_ois_firmware_update(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_ois *ois = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		ret = -EINVAL;
		return ret;
	}

	/* OIS open Firmware */
	ret = fimc_is_ois_fw_ready(ois);
	if (ret < 0) {
		err("OIS Firmware check fail");
		return ret;
	}

	/* OIS Firmware download in OIS */
	ret = fimc_is_ois_fw_download(ois);
	if (ret < 0) {
		err("OIS Firmware download fail");
		return ret;
	}

	return ret;
}
#endif

int fimc_is_ois_shift_compensation(struct v4l2_subdev *subdev, int position)
{
	int ret = 0;
	struct fimc_is_ois *ois;
	u32 af_value;
	u8 write_data[4] = {0,};
	u8 shift_x = 0;
	u8 shift_y = 0;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	af_value = position / AF_BOUNDARY;

	shift_x = (2 * ((ois_shift_x[af_value + 1] - ois_shift_x[af_value]) *
				((position - AF_BOUNDARY * af_value) / 2)) / AF_BOUNDARY + ois_shift_x[af_value]);
	shift_y = (2 * ((ois_shift_y[af_value + 1] - ois_shift_y[af_value]) *
				((position - AF_BOUNDARY * af_value) / 2)) / AF_BOUNDARY + ois_shift_y[af_value]);

	write_data[0] = (shift_x & 0xFF);
	write_data[1] = (shift_x & 0xFF00) >> 8;
	write_data[2] = (shift_y & 0xFF);
	write_data[3] = (shift_y & 0xFF00) >> 8;

	dbg_sensor("%s: shift_x(%d), shift_y(%d), \
			write_data[0](%d), write_data[1](%d), write_data[2](%d), write_data[3](%d)\n", __func__,
			shift_x, shift_y,
			write_data[0], write_data[1], write_data[2], write_data[3]);

	ret = fimc_is_ois_write_multi(ois->client, 0x004C, write_data, 4);
	if (ret < 0)
		err("i2c write multi is fail\n");

p_err:
	return ret;
}

int fimc_is_set_ois_mode(struct v4l2_subdev *subdev, int mode)
{
	int ret = 0;
	struct fimc_is_ois *ois;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if (!ois) {
		err("ois is NULL");
		ret = -EINVAL;
		return ret;
	}

	client = ois->client;
	if (!client) {
		err("client is NULL");
		ret = -EINVAL;
		return ret;
	}

	switch(mode) {
	case OPTICAL_STABILIZATION_MODE_STILL:
		fimc_is_ois_write(client, 0x0002, 0x00);
		fimc_is_ois_write(client, 0x0000, 0x01);
		break;
	case OPTICAL_STABILIZATION_MODE_VIDEO:
		fimc_is_ois_write(client, 0x0002, 0x01);
		fimc_is_ois_write(client, 0x0000, 0x01);
		break;
	case OPTICAL_STABILIZATION_MODE_CENTERING:
		fimc_is_ois_write(client, 0x0002, 0x05);
		fimc_is_ois_write(client, 0x0000, 0x01);
		break;
	default:
		dbg_sensor("%s: ois_mode value(%d)\n", __func__, mode);
		break;
	}

	return ret;
}

int fimc_is_ois_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int i = 0;
	struct fimc_is_ois *ois = NULL;
	u16 ois_shift_x_cal = 0;
	u16 ois_shift_y_cal = 0;
	u8 cal_data[2];

	WARN_ON(!subdev);

	ois = (struct fimc_is_ois *)v4l2_get_subdevdata(subdev);
	if(!ois) {
		err("%s, ois subdev is NULL", __func__);
		ret = -EINVAL;
		return ret;
	}

	ois->ois_mode = OPTICAL_STABILIZATION_MODE_OFF;

	/* use user data */
	ret |= fimc_is_ois_write(ois->client, 0x000F, 0x0E);
	ret |= fimc_is_ois_write(ois->client, 0x0010, 0x0004);
	ret |= fimc_is_ois_write(ois->client, 0x000E, 0x04);
	if (ret < 0) {
		err("ois user data write is fail");
		return ret;
	}

	/* OIS Shift CAL DATA */
	for (i = 0; i < 9; i++) {
		cal_data[0] = 0;
		cal_data[1] = 0;
		fimc_is_ois_read_multi(ois->client, 0x0108 + (i * 2), cal_data, 2);
		ois_shift_x_cal = (cal_data[1] << 8) | (cal_data[0]);

		cal_data[0] = 0;
		cal_data[1] = 0;
		fimc_is_ois_read_multi(ois->client, 0x0120 + (i * 2), cal_data, 2);
		ois_shift_y_cal = (cal_data[1] << 8) | (cal_data[0]);

		info("OIS CAL[%d]:X[%d], Y[%d]\n", i, ois_shift_x_cal, ois_shift_y_cal);

		if (ois_shift_x_cal > (short)32767)
			ois_shift_x[i] = ois_shift_x_cal - 65536;
		else
			ois_shift_x[i] = ois_shift_x_cal;

		if (ois_shift_y_cal > (short)32767)
			ois_shift_y[i] = ois_shift_y_cal - 65536;
		else
			ois_shift_y[i] = ois_shift_y_cal;

		info("%s: ois_shift_x[%d](%d), ois_shift_y[%d](%d)\n", __func__,
				i, ois_shift_x[i],
				i, ois_shift_y[i]);
	}

	/* CACTRL */
	ret = fimc_is_ois_write(ois->client, 0x0039, 0x01);
	if (ret < 0) {
		err("ois cactrl write is fail");
		return ret;
	}

	return ret;
}

static struct fimc_is_ois_ops ois_ops = {
	.ois_init = fimc_is_ois_init,
	.ois_set_mode = fimc_is_set_ois_mode,
	.ois_shift_compensation = fimc_is_ois_shift_compensation,
#ifdef CONFIG_OIS_DIRECT_FW_CONTROL
	.ois_fw_update = fimc_is_ois_firmware_update,
#endif
};

static int sensor_ois_rumbaS4_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core= NULL;
	struct v4l2_subdev *subdev_ois = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct fimc_is_ois *ois;
	struct device *dev;
	struct device_node *dnode;
	u32 sensor_id = 0;
	struct fimc_is_device_ois *ois_device;
	struct fimc_is_vender_specific *specific;

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

	ret = of_property_read_u32(dnode, "id", &sensor_id);
	if (ret) {
		err("id read is fail(%d)", ret);
		goto p_err;
	}

	probe_info("%s sensor_id %d\n", __func__, sensor_id);

	device = &core->sensor[sensor_id];
	if (!device) {
		err("sensor device is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	sensor_peri = find_peri_by_ois_id(device, OIS_NAME_RUMBA_S4);
	if (!sensor_peri) {
		probe_info("sensor peri is not yet probed");
		return -EPROBE_DEFER;
	}

	ois = &sensor_peri->ois;
	if (!ois) {
		err("ois is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	ois->ois_ops = &ois_ops;

	subdev_ois = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_ois) {
		err("subdev_ois is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	sensor_peri->subdev_ois = subdev_ois;

	ois_device = kzalloc(sizeof(struct fimc_is_device_ois), GFP_KERNEL);
	if (!ois_device) {
		err("fimc_is_device_ois is NULL");
		return -ENOMEM;
	}
	ois_device->ois_hsi2c_status = false;
	specific = core->vender.private_data;
	specific->ois_ver_read = false;

	ois->id = OIS_NAME_RUMBA_S4;
	ois->subdev = subdev_ois;
	ois->device = sensor_id;
	ois->client = client;
	ois->ois_mode = OPTICAL_STABILIZATION_MODE_OFF;

	v4l2_i2c_subdev_init(subdev_ois, client, &subdev_ops);
	v4l2_set_subdevdata(subdev_ois, ois);
	v4l2_set_subdev_hostdata(subdev_ois, ois_device);
	i2c_set_clientdata(client, ois_device);
	snprintf(subdev_ois->name, V4L2_SUBDEV_NAME_SIZE, "ois->subdev.%d", ois->id);

	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id sensor_ois_rumbaS4_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-ois-rumbaS4",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_ois_rumbaS4_match);

static const struct i2c_device_id sensor_ois_rumbaS4_idt[] = {
	{ OIS_NAME, 0 },
	{},
};

static struct i2c_driver sensor_ois_rumbaS4_driver = {
	.probe	= sensor_ois_rumbaS4_probe,
	.driver = {
		.name	= OIS_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_ois_rumbaS4_match
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_ois_rumbaS4_idt
};

static int __init sensor_ois_rumbaS4_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_ois_rumbaS4_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_ois_rumbaS4_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_ois_rumbaS4_init);
