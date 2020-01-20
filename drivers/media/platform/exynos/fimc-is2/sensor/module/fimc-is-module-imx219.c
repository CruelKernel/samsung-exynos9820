/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-module-imx219.h"

#define SENSOR_NAME "IMX219"
#define DEFAULT_SENSOR_WIDTH	184
#define DEFAULT_SENSOR_HEIGHT	104
#define SENSOR_MEMSIZE DEFAULT_SENSOR_WIDTH * DEFAULT_SENSOR_HEIGHT

#define SENSOR_REG_VIS_DURATION_MSB			(0x6026)
#define SENSOR_REG_VIS_DURATION_LSB			(0x6027)
#define SENSOR_REG_VIS_FRAME_LENGTH_LINE_ALV_MSB	(0x4340)
#define SENSOR_REG_VIS_FRAME_LENGTH_LINE_ALV_LSB	(0x4341)
#define SENSOR_REG_VIS_LINE_LENGTH_PCLK_ALV_MSB		(0x4342)
#define SENSOR_REG_VIS_LINE_LENGTH_PCLK_ALV_LSB		(0x4343)
#define SENSOR_REG_VIS_GAIN_RED				(0x6029)
#define SENSOR_REG_VIS_GAIN_GREEN			(0x602A)
#define SENSOR_REG_VIS_AE_TARGET			(0x600A)
#define SENSOR_REG_VIS_AE_SPEED				(0x5034)
#define SENSOR_REG_VIS_AE_NUMBER_OF_PIXEL_MSB		(0x5030)
#define SENSOR_REG_VIS_AE_NUMBER_OF_PIXEL_LSB		(0x5031)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_1x1_2		(0x6000)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_1x3_4		(0x6001)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_2x1_2		(0x6002)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_2x3_4		(0x6003)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_3x1_2		(0x6004)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_3x3_4		(0x6005)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_4x1_2		(0x6006)
#define SENSOR_REG_VIS_AE_WINDOW_WEIGHT_4x3_4		(0x6007)
#define SENSOR_REG_VIS_AE_MANUAL_EXP_MSB		(0x5039)
#define SENSOR_REG_VIS_AE_MANUAL_EXP_LSB		(0x503A)
#define SENSOR_REG_VIS_AE_MANUAL_ANG_MSB		(0x503B)
#define SENSOR_REG_VIS_AE_MANUAL_ANG_LSB		(0x503C)
#define SENSOR_REG_VIS_BIT_CONVERTING_MSB		(0x602B)
#define SENSOR_REG_VIS_BIT_CONVERTING_LSB		(0x7203)
#define SENSOR_REG_VIS_AE_OFF				(0x5000)

static u16 setfile_vision_imx219[][2] = {
	{0x4200, 0x02},
	{0x4201, 0xAE},
	{0x4301, 0x04},
	{0x4309, 0x04},
	{0x4345, 0x08},
	{0x4347, 0x08},
	{0x4348, 0x0A},
	{0x4349, 0x01},
	{0x434A, 0x05},
	{0x434B, 0xA0},
	{0x434C, 0x01},
	{0x434D, 0x40},
	{0x434E, 0x00},
	{0x434F, 0xB4},
	{0x4381, 0x01},
	{0x4383, 0x07},
	{0x4385, 0x08},
	{0x4387, 0x08},
	{0x5004, 0x01},
	{0x5005, 0x1E},
	{0x5014, 0x11},
	{0x5015, 0x9E},
	{0x5016, 0x00},
	{0x5017, 0x02},
	{0x5030, 0x1C},
	{0x5031, 0x20},
	{0x5034, 0x00},
	{0x5035, 0x02},
	{0x5036, 0x00},
	{0x5037, 0x06},
	{0x5038, 0xC0},
	{0x5039, 0x00},
	{0x503A, 0x00},
	{0x503B, 0x00},
	{0x503C, 0x00},
	{0x503D, 0x20},
	{0x503E, 0x70},
	{0x503F, 0x02},
	{0x600A, 0x2A},
	{0x600E, 0x05},
	{0x6014, 0x27},
	{0x6015, 0x1D},
	{0x6018, 0x01},
	{0x6026, 0x00},
	{0x6027, 0x68},
	{0x6029, 0x08},
	{0x602A, 0x08},
	{0x602B, 0x00},
	{0x602C, 0x00},
	{0x7008, 0x00},
	{0x7009, 0x10},
	{0x700A, 0x00},
	{0x700B, 0x10},
	{0x7014, 0x2B},
	{0x7015, 0x91},
	{0x7016, 0x82},
	{0x701B, 0x16},
	{0x701D, 0x0B},
	{0x701F, 0x0B},
	{0x7026, 0x1A},
	{0x7027, 0x46},
	{0x7029, 0x14},
	{0x702A, 0x02},
	{0x7038, 0x01},
	{0x7039, 0x14},
	{0x703A, 0x32},
	{0x703B, 0x22},
	{0x7040, 0x01},
	{0x7041, 0x14},
	{0x7042, 0x32},
	{0x7043, 0x22},
	{0x7050, 0x0A},
	{0x7051, 0xA8},
	{0x7052, 0x35},
	{0x7053, 0x54},
	{0x7054, 0x00},
	{0x7055, 0x00},
	{0x7056, 0x00},
	{0x7057, 0x00},
	{0x705E, 0x0E},
	{0x705F, 0x10},
	{0x7060, 0x01},
	{0x7064, 0x05},
	{0x7065, 0x3C},
	{0x7066, 0x00},
	{0x7067, 0x00},
	{0x7068, 0x4A},
	{0x706C, 0x01},
	{0x7077, 0x88},
	{0x7078, 0x88},
	{0x7082, 0x90},
	{0x7091, 0x05},
	{0x7098, 0x00},
	{0x7112, 0x01},
	{0x720A, 0x06},
	{0x720B, 0x80},
	{0x7245, 0xC1},
	{0x7301, 0x01},
	{0x7305, 0x13},
	{0x7306, 0x01},
	{0x7323, 0x01},
	{0x7339, 0x07},
	{0x7351, 0x01},
	{0x7352, 0x24},
	{0x7405, 0x28},
	{0x7406, 0x28},
	{0x7407, 0xC0},
	{0x7454, 0x01},
	{0x7460, 0x01},
	{0x7461, 0x20},
	{0x7462, 0xC0},
	{0x7463, 0x1E},
	{0x7464, 0x02},
	{0x7465, 0x4B},
	{0x7467, 0x20},
	{0x7468, 0x20},
	{0x7469, 0x20},
	{0x746A, 0x20},
	{0x746B, 0x20},
	{0x746C, 0x20},
	{0x746D, 0x09},
	{0x746E, 0xFF},
	{0x746F, 0x01},
	{0x7472, 0x00},
	{0x7473, 0x02},
	{0x7474, 0xC1},
	{0x7475, 0x00},
	{0x7476, 0x00},
	{0x7477, 0x00},
	{0x7478, 0x00},
	{0x4100, 0x01},
};

static struct fimc_is_sensor_cfg config_imx219[] = {
	/* 3280X2458@20fps */
	FIMC_IS_SENSOR_CFG(3280, 2458, 20, 19, 0, CSI_DATA_LANES_2),
	/* 3280X1846@20fps */
	FIMC_IS_SENSOR_CFG(3280, 1846, 20, 15, 1, CSI_DATA_LANES_2),
	/* 1640X1228@24fps */
	FIMC_IS_SENSOR_CFG(1640, 1228, 24, 12, 2, CSI_DATA_LANES_2),
	/* 3280X1846@24fps */
	FIMC_IS_SENSOR_CFG(3280, 1846, 24, 17, 3, CSI_DATA_LANES_2),
};

static int sensor_imx219_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_imx219_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_imx219_registered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
	return 0;
}

static void sensor_imx219_unregistered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
}

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_imx219_open,
	.close = sensor_imx219_close,
	.registered = sensor_imx219_registered,
	.unregistered = sensor_imx219_unregistered,
};

static int sensor_imx219_init(struct v4l2_subdev *subdev, u32 val)
{
	int i, ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_imx219 *module_imx219;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	module_imx219 = module->private_data;
	client = module->client;

	module_imx219->system_clock = 146 * 1000 * 1000;
	module_imx219->line_length_pck = 146 * 1000 * 1000;

	pr_info("%s\n", __func__);

	/* sensor init */
	for (i = 0; i < ARRAY_SIZE(setfile_vision_imx219); i++) {
		fimc_is_sensor_write8(client, setfile_vision_imx219[i][0],
				(u8)setfile_vision_imx219[i][1]);
	}

	pr_info("[MOD:D:%d] %s(%d)\n", module->id, __func__, val);

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_imx219_init
};

static int sensor_imx219_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;

	pr_info("%s\n", __func__);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!sensor) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (enable) {
		ret = CALL_MOPS(sensor, stream_on, subdev);
		if (ret < 0) {
			err("s_duration is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = CALL_MOPS(sensor, stream_off, subdev);
		if (ret < 0) {
			err("s_duration is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return 0;
}

static int sensor_imx219_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;
	u64 duration;

	FIMC_BUG(!subdev);
	FIMC_BUG(!param);

	pr_info("%s\n", __func__);

	cp = &param->parm.capture;
	tpf = &cp->timeperframe;

	if (!tpf->denominator) {
		err("denominator is 0");
		ret = -EINVAL;
		goto p_err;
	}

	if (!tpf->numerator) {
		err("numerator is 0");
		ret = -EINVAL;
		goto p_err;
	}

	duration = (u64)(tpf->numerator * 1000 * 1000 * 1000) /
					(u64)(tpf->denominator);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!sensor) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = CALL_MOPS(sensor, s_duration, subdev, duration);
	if (ret) {
		err("s_duration is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_imx219_s_format(struct v4l2_subdev *subdev, struct v4l2_mbus_framefmt *fmt)
{
	/* TODO */
	return 0;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = sensor_imx219_s_stream,
	.s_parm = sensor_imx219_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_imx219_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int sensor_imx219_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!sensor)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = sensor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, 0x4100, 1);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int sensor_imx219_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!sensor)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = sensor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, 0x4100, 0);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

/*
 * @ brief
 * frame duration time
 * @ unit
 * nano second
 * @ remarks
 */
int sensor_imx219_s_duration(struct v4l2_subdev *subdev, u64 duration)
{
	int ret = 0;
	u8 value[2];
	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	pr_info("%s\n", __func__);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!sensor)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = sensor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/*
	 * forcely set 10fps for IMX219,
	 */
	value[0] = 0x52;
	value[1] = 0x0;

	fimc_is_sensor_write8(client, SENSOR_REG_VIS_DURATION_MSB, value[1]);
	fimc_is_sensor_write8(client, SENSOR_REG_VIS_DURATION_LSB, value[0]);

p_err:
	return ret;
}

int sensor_imx219_g_min_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_g_max_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_s_exposure(struct v4l2_subdev *subdev, u64 exposure)
{
	int ret = 0;
	u8 value;
	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	pr_info("%s(%d)\n", __func__, (u32)exposure);

	sensor = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!sensor)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = sensor->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	value = exposure & 0xFF;

	fimc_is_sensor_write8(client, SENSOR_REG_VIS_AE_TARGET, value);

p_err:
	return ret;
}

int sensor_imx219_g_min_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_g_max_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_s_again(struct v4l2_subdev *subdev, u64 sensitivity)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	return ret;
}

int sensor_imx219_g_min_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_g_max_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_s_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_g_min_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_imx219_g_max_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

struct fimc_is_sensor_ops module_imx219_ops = {
	.stream_on	= sensor_imx219_stream_on,
	.stream_off	= sensor_imx219_stream_off,
	.s_duration	= sensor_imx219_s_duration,
	.g_min_duration	= sensor_imx219_g_min_duration,
	.g_max_duration	= sensor_imx219_g_max_duration,
	.s_exposure	= sensor_imx219_s_exposure,
	.g_min_exposure	= sensor_imx219_g_min_exposure,
	.g_max_exposure	= sensor_imx219_g_max_exposure,
	.s_again	= sensor_imx219_s_again,
	.g_min_again	= sensor_imx219_g_min_again,
	.g_max_again	= sensor_imx219_g_max_again,
	.s_dgain	= sensor_imx219_s_dgain,
	.g_min_dgain	= sensor_imx219_g_min_dgain,
	.g_max_dgain	= sensor_imx219_g_max_dgain
};

#ifdef CONFIG_OF
static int sensor_imx219_power_setpin(struct device *dev)
{
	return 0;
}
#endif

int sensor_imx219_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	static bool probe_retried = false;

	if (!fimc_is_dev)
		goto probe_defer;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	device = &core->sensor[SENSOR_IMX219_INSTANCE];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* IMX219 */
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	module->sensor_id = SENSOR_IMX219_NAME;
	module->subdev = subdev_module;
	module->device = SENSOR_IMX219_INSTANCE;
	module->ops = &module_imx219_ops;
	module->client = client;
	module->active_width = 3264;
	module->active_height = 2448;
	module->pixel_width = module->active_width + 16;
	module->pixel_height = module->active_height + 10;
	module->max_framerate = 24;
	module->position = SENSOR_POSITION_FRONT;
	module->mode = CSI_MODE_CH0_ONLY;
	module->lanes = CSI_DATA_LANES_2;
	module->bitwidth = 10;
	module->sensor_maker = "SONY";
	module->sensor_name = "IMX219";
	module->setfile_name = "setfile_imx219.bin";
	module->cfgs = ARRAY_SIZE(config_imx219);
	module->cfg = config_imx219;
	module->private_data = kzalloc(sizeof(struct fimc_is_module_imx219), GFP_KERNEL);
	if (!module->private_data) {
		probe_err("private_data is NULL");
		ret = -ENOMEM;
		kfree(subdev_module);
		goto p_err;
	}
#ifdef CONFIG_OF
	module->power_setpin = sensor_imx219_power_setpin;
#endif
	ext = &module->ext;
	ext->mipi_lane_num = module->lanes;
	ext->I2CSclk = I2C_L0;
	ext->sensor_con.product_name = SENSOR_NAME_IMX219;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = SENSOR_CONTROL_I2C1;
	ext->sensor_con.peri_setting.i2c.slave_address = 0x34;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->from_con.product_name = FROMDRV_NAME_NOTHING;

	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;

	if (client) {
		v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);
		subdev_module->internal_ops = &internal_ops;
	} else {
		v4l2_subdev_init(subdev_module, &subdev_ops);
	}

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->id);

p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;

probe_defer:
	if (probe_retried) {
		err("probe has already been retried!!");
		BUG();
	}

	probe_retried = true;
	err("core device is not yet probed");
	return -EPROBE_DEFER;
}

static int sensor_imx219_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_imx219_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-sensor-imx219",
	},
	{},
};
#endif

static const struct i2c_device_id sensor_imx219_idt[] = {
	{ SENSOR_NAME, 0 },
};

static struct i2c_driver sensor_imx219_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_imx219_match
#endif
	},
	.probe	= sensor_imx219_probe,
	.remove	= sensor_imx219_remove,
	.id_table = sensor_imx219_idt
};

static int __init sensor_imx219_load(void)
{
        return i2c_add_driver(&sensor_imx219_driver);
}

static void __exit sensor_imx219_unload(void)
{
        i2c_del_driver(&sensor_imx219_driver);
}

module_init(sensor_imx219_load);
module_exit(sensor_imx219_unload);

MODULE_AUTHOR("Gilyeon lim");
MODULE_DESCRIPTION("Sensor IMX219 driver");
MODULE_LICENSE("GPL v2");
