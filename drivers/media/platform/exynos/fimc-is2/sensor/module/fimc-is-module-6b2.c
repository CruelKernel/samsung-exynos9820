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
#include "fimc-is-dt.h"
#include "fimc-is-module-6b2.h"

#define SENSOR_NAME "S5K6B2"
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

static struct fimc_is_sensor_cfg config_6b2[] = {
	/* 1936x1090@30fps */
	FIMC_IS_SENSOR_CFG(1936, 1090, 30, 16, 0, CSI_DATA_LANES_1),
	/* 1936x1090@24fps */
	FIMC_IS_SENSOR_CFG(1936, 1090, 24, 13, 1, CSI_DATA_LANES_1),
	/* 1296x730@30fps */
	FIMC_IS_SENSOR_CFG(1296, 730, 30, 13, 2, CSI_DATA_LANES_1),
};

static struct fimc_is_vci vci_6b2[] = {
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR10,
		.config = {{0, HW_FORMAT_RAW10}, {1, HW_FORMAT_UNKNOWN}, {2, HW_FORMAT_UNKNOWN}, {3, 0}}
	}, {
		.pixelformat = V4L2_PIX_FMT_SBGGR12,
		.config = {{0, HW_FORMAT_RAW10}, {1, HW_FORMAT_UNKNOWN}, {2, HW_FORMAT_UNKNOWN}, {3, 0}}
	}, {
		.pixelformat = V4L2_PIX_FMT_SBGGR16,
		.config = {{0, HW_FORMAT_RAW10}, {1, HW_FORMAT_UNKNOWN}, {2, HW_FORMAT_UNKNOWN}, {3, 0}}
	}
};

static int sensor_6b2_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_6b2_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_6b2_registered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
	return 0;
}

static void sensor_6b2_unregistered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
}

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_6b2_open,
	.close = sensor_6b2_close,
	.registered = sensor_6b2_registered,
	.unregistered = sensor_6b2_unregistered,
};

static int sensor_6b2_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_6b2 *module_6b2;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_6b2 = module->private_data;
	if (unlikely(!module_6b2)) {
		err("module_6b2 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_6b2->system_clock = 146 * 1000 * 1000;
	module_6b2->line_length_pck = 146 * 1000 * 1000;

	pr_info("%s\n", __func__);
	/* sensor init */
	/* 8 bit mode */
	fimc_is_sensor_write8(client, 0x7203, 0x40);
	fimc_is_sensor_write8(client, 0x602B, 0x02);
	fimc_is_sensor_write8(client, 0x702A, 0x3D);
	fimc_is_sensor_write8(client, 0x702B, 0xB0);
	fimc_is_sensor_write8(client, 0x7030, 0x0E);
	fimc_is_sensor_write8(client, 0x7031, 0x2F);

	/* Analog Tuning */
	fimc_is_sensor_write8(client, 0x7067, 0x00);
	fimc_is_sensor_write8(client, 0x7073, 0xFF);
	fimc_is_sensor_write8(client, 0x7074, 0x22);

	/* Dark Tuning */
	fimc_is_sensor_write8(client, 0x7042, 0x1F);
	fimc_is_sensor_write8(client, 0x7403, 0xC0);
	fimc_is_sensor_write8(client, 0x7245, 0x04);
	fimc_is_sensor_write8(client, 0x7205, 0xA1);

	/* Remove Dark Band */
	fimc_is_sensor_write8(client, 0x7430, 0x07);
	fimc_is_sensor_write8(client, 0x705C, 0x7E);

	/* Remove  Sun spot */
	fimc_is_sensor_write8(client, 0x702C, 0x3C);
	fimc_is_sensor_write8(client, 0x7075, 0x3D);

	/* Remove CFPN */
	fimc_is_sensor_write8(client, 0x7066, 0x0C);

	/* AE setting */
	fimc_is_sensor_write8(client, 0x6000, 0x44);
	fimc_is_sensor_write8(client, 0x6001, 0x44);
	fimc_is_sensor_write8(client, 0x6002, 0x44);
	fimc_is_sensor_write8(client, 0x6003, 0x44);
	fimc_is_sensor_write8(client, 0x6004, 0x44);
	fimc_is_sensor_write8(client, 0x6005, 0x44);
	fimc_is_sensor_write8(client, 0x6006, 0x44);
	fimc_is_sensor_write8(client, 0x6007, 0x44);

	/* AE target */
	fimc_is_sensor_write8(client, 0x600A, 0xB4);

	/* speed */
	fimc_is_sensor_write8(client, 0x5034, 0x00);

	/* Cintc_min */
	fimc_is_sensor_write8(client, 0x5017, 0x01);

	/* Number of pixel */
	fimc_is_sensor_write8(client, 0x5030, 0x4A);
	fimc_is_sensor_write8(client, 0x5031, 0xC0);

	/* G + R Setting */
	/* Vision Senser Data = 0.5*Gr + 0.5*R */
	fimc_is_sensor_write8(client, 0x6029, 0x02);
	fimc_is_sensor_write8(client, 0x602A, 0x02);

	/* For Analog Gain 16x */
	fimc_is_sensor_write8(client, 0x7018, 0xCF);
	fimc_is_sensor_write8(client, 0x7019, 0xDB);
	fimc_is_sensor_write8(client, 0x702A, 0x8D);
	fimc_is_sensor_write8(client, 0x702B, 0x60);
	fimc_is_sensor_write8(client, 0x5035, 0x02);

	/* BIT_RATE_MBPS_alv */
	fimc_is_sensor_write8(client, 0x7351, 0x02);
	fimc_is_sensor_write8(client, 0x7352, 0x48);
	fimc_is_sensor_write8(client, 0x7353, 0x00);
	fimc_is_sensor_write8(client, 0x7354, 0x00);

	fimc_is_sensor_write8(client, 0x7339, 0x03);

	/* Analog gain */
	fimc_is_sensor_write8(client, 0x4204, 0x00);
	fimc_is_sensor_write8(client, 0x4205, 0x32);

	/* frame rate - default 10fps*/
	fimc_is_sensor_write8(client, SENSOR_REG_VIS_DURATION_MSB, 0x00);
	fimc_is_sensor_write8(client, SENSOR_REG_VIS_DURATION_LSB, 0x6A);

	pr_info("[MOD:D:%d] %s(%d)\n", module->sensor_id, __func__, val);

p_err:
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_6b2_init
};

static int sensor_6b2_s_stream(struct v4l2_subdev *subdev, int enable)
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
		if (ret) {
			err("stream_on is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = CALL_MOPS(sensor, stream_off, subdev);
		if (ret) {
			err("stream_off is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return 0;
}

static int sensor_6b2_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_module_enum *sensor;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;
	u64 numerator, denominator;
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

	numerator = (u64)tpf->numerator;
	denominator = (u64)tpf->denominator;
	duration = (numerator * 1000 * 1000 * 1000) / denominator;

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

static int sensor_6b2_s_format(struct v4l2_subdev *subdev, struct v4l2_mbus_framefmt *fmt)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_6b2 *module_6b2;
	struct i2c_client *client;

	FIMC_BUG(!subdev);
	FIMC_BUG(!fmt);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("module is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	module_6b2 = module->private_data;
	if (unlikely(!module_6b2)) {
		err("module_6b2 is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = sensor_6b2_s_stream,
	.s_parm = sensor_6b2_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_6b2_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int sensor_6b2_stream_on(struct v4l2_subdev *subdev)
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
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

int sensor_6b2_stream_off(struct v4l2_subdev *subdev)
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
		ret = -EINVAL;
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
int sensor_6b2_s_duration(struct v4l2_subdev *subdev, u64 duration)
{
	int ret = 0;
	u8 value[2];
	u32 framerate, result;
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

	framerate = 1000 * 1000 * 1000 / (u32)duration;
	result = 1060 / framerate;
	value[0] = result & 0xFF;
	value[1] = (result >> 8) & 0xFF;

	fimc_is_sensor_write8(client, SENSOR_REG_VIS_DURATION_MSB, value[1]);
	fimc_is_sensor_write8(client, SENSOR_REG_VIS_DURATION_LSB, value[0]);

p_err:
	return ret;
}

int sensor_6b2_g_min_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_g_max_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_s_exposure(struct v4l2_subdev *subdev, u64 exposure)
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

int sensor_6b2_g_min_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_g_max_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_s_again(struct v4l2_subdev *subdev, u64 sensitivity)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	return ret;
}

int sensor_6b2_g_min_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_g_max_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_s_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_g_min_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_6b2_g_max_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

struct fimc_is_sensor_ops module_6b2_ops = {
	.stream_on	= sensor_6b2_stream_on,
	.stream_off	= sensor_6b2_stream_off,
	.s_duration	= sensor_6b2_s_duration,
	.g_min_duration	= sensor_6b2_g_min_duration,
	.g_max_duration	= sensor_6b2_g_max_duration,
	.s_exposure	= sensor_6b2_s_exposure,
	.g_min_exposure	= sensor_6b2_g_min_exposure,
	.g_max_exposure	= sensor_6b2_g_max_exposure,
	.s_again	= sensor_6b2_s_again,
	.g_min_again	= sensor_6b2_g_min_again,
	.g_max_again	= sensor_6b2_g_max_again,
	.s_dgain	= sensor_6b2_s_dgain,
	.g_min_dgain	= sensor_6b2_g_min_dgain,
	.g_max_dgain	= sensor_6b2_g_max_dgain
};

#ifdef CONFIG_OF
static int sensor_6b2_power_setpin(struct platform_device *pdev,
		struct exynos_platform_fimc_is_module *pdata)
{
	struct device *dev;
	struct device_node *dnode;
	int gpio_none = 0, gpio_reset = 0, gpio_standby = 0;

	FIMC_BUG(!pdev);

	dev = &pdev->dev;
	dnode = dev->of_node;

	gpio_reset = of_get_named_gpio(dnode, "gpio_reset", 0);
	if (!gpio_is_valid(gpio_reset)) {
		dev_err(dev, "failed to get PIN_RESET\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_reset);
	}

	gpio_standby = of_get_named_gpio(dnode, "gpio_standby", 0);
	if (!gpio_is_valid(gpio_standby)) {
		dev_err(dev, "failed to get gpio_standby\n");
	} else {
		gpio_request_one(gpio_standby, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_standby);
	}

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF);

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF);

	/* FRONT CAMERA - POWER ON */
#ifdef CONFIG_MACH_ESPRESSO7420
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_reset, "sen_rst low", PIN_OUTPUT, 0, 0);
#else
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_standby, "sen_standby low", PIN_OUTPUT, 0, 0);
#endif
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDA30_VTCAMSEN", PIN_REGULATOR, 1, 0, 2800000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDD18_VTCAMIO", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 0);

	/* FRONT CAMERA - POWER OFF */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst", PIN_RESET, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst input", PIN_INPUT, 0 ,0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDA30_VTCAMSEN", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDD18_VTCAMIO", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);

	/* FRONT CAEMRA - VISION POWER ON */
#ifdef CONFIG_MACH_ESPRESSO7420
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_reset, "sen_rst low", PIN_OUTPUT, 0, 0);
#else
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_standby, "sen_standby low", PIN_OUTPUT, 0, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, GPIO_SCENARIO_ON, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 0);

	/* FRONT CAEMRA - VISION POWER OFF */
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst", PIN_RESET, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst input", PIN_INPUT, 0 ,0);

	dev_info(dev, "%s X v4\n", __func__);

	return 0;
}
#endif

int sensor_6b2_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	static bool probe_retried = false;
	struct exynos_platform_fimc_is_module *pdata;
	struct device *dev;

	if (!fimc_is_dev)
		goto probe_defer;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &pdev->dev;

#ifdef CONFIG_OF
	fimc_is_sensor_module_parse_dt(pdev, sensor_6b2_power_setpin);
#endif

	pdata = dev_get_platdata(dev);

	device = &core->sensor[pdata->id];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* S5K6B2 */
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_S5K6B2;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = NULL;
	module->active_width = 1920;
	module->active_height = 1080;
	module->pixel_width = module->active_width + 16;
	module->pixel_height = module->active_height + 10;
	module->max_framerate = 30;
	module->position = pdata->position;
	module->mode = CSI_MODE_CH0_ONLY;
	module->lanes = CSI_DATA_LANES_1;
	module->bitwidth = 10;
	module->vcis = ARRAY_SIZE(vci_6b2);
	module->vci = vci_6b2;
	module->sensor_maker = "SLSI";
	module->sensor_name = "S5K6B2";
	module->setfile_name = "setfile_6b2.bin";
	module->cfgs = ARRAY_SIZE(config_6b2);
	module->cfg = config_6b2;
	module->private_data = kzalloc(sizeof(struct fimc_is_module_6b2), GFP_KERNEL);
	if (!module->private_data) {
		probe_err("private_data is NULL");
		ret = -ENOMEM;
		kfree(subdev_module);
		goto p_err;
	}

	ext = &module->ext;
	ext->mipi_lane_num = module->lanes;
	ext->I2CSclk = 0;

	ext->sensor_con.product_name = SENSOR_NAME_S5K6B2;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = pdata->sensor_i2c_ch;
	ext->sensor_con.peri_setting.i2c.slave_address = pdata->sensor_i2c_addr;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->from_con.product_name = FROMDRV_NAME_NOTHING;

	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;

	ext->actuator_con.product_name = ACTUATOR_NAME_NOTHING;

	ext->flash_con.product_name = FLADRV_NAME_NOTHING;

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->sensor_id);

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

static int sensor_6b2_remove(struct platform_device *pdev)
{
	int ret = 0;
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_6b2_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-sensor-6b2",
	},
	{},
};
#endif
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_4e6_match);

static struct platform_driver sensor_6b2_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_6b2_match
#endif
	},
	.probe	= sensor_6b2_probe,
	.remove	= sensor_6b2_remove,
};

module_platform_driver(sensor_6b2_driver);

static int __init fimc_is_sensor_module_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&sensor_6b2_driver);
	if (ret)
		err("platform_driver_register failed: %d\n", ret);

	return ret;
}

static void __exit fimc_is_sensor_module_exit(void)
{
	platform_driver_unregister(&sensor_6b2_driver);
}
module_init(fimc_is_sensor_module_init);
module_exit(fimc_is_sensor_module_exit);

MODULE_AUTHOR("Gilyeon lim");
MODULE_DESCRIPTION("Sensor 6B2 driver");
MODULE_LICENSE("GPL v2");
