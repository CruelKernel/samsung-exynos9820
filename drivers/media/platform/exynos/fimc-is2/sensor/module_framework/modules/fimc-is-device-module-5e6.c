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
#include "fimc-is-device-module-base.h"
#include "fimc-is-device-module-5e6.h"

#define SENSOR_NAME "S5K5E6"

#define SENSOR_REG_DURATION_MSB			(0x6026)
#define SENSOR_REG_STREAM_ON_0			(0x3C0D)
#define SENSOR_REG_STREAM_ON_1			(0x0100)
#define SENSOR_REG_STREAM_ON_2			(0x3C22)
#define SENSOR_REG_STREAM_OFF			(0x0100)
#define SENSOR_REG_EXPOSURE_TIME		(0x0202)
#define SENSOR_REG_GAIN_ADJUST_0		(0x0204)
#define SENSOR_REG_GAIN_ADJUST_1		(0x0205)
#define SENSOR_REG_SHUTTERSPEED_ADJUST_0	(0x0202)
#define SENSOR_REG_SHUTTERSPEED_ADJUST_1	(0x0203)
#define SENSOR_REG_VERSION			(0x0010)
#define SENSOR_REG_MIPI_CLK_CTRL		(0x3929)
#define SENSOR_FPS_30		33333333
#define SENSOR_FPS_15		66666666

#define SENSOR_SCENARIO_SECURE      6

bool is_final_cam_module_iris;
bool is_iris_ver_read;
bool is_iris_mtf_test_check;
u8 is_iris_mtf_read_data[3];	/* 0:year 1:month 2:company */

static u16 setfile_vision_5e6[][2] = {
	{0x3303, 0x02},
	{0x3400, 0x01},
	{0x3906, 0x7E},
	{0x3C01, 0x0F},
	{0x3C14, 0x00},
	{0x3235, 0x08},
	{0x3063, 0x35},
	{0x307A, 0x10},
	{0x307B, 0x0E},
	{0x3079, 0x20},
	{0x3070, 0x05},
	{0x3067, 0x06},
	{0x3071, 0x62},
	{0x3072, 0x13},
	{0x3203, 0x43},
	{0x3205, 0x43},
	{0x320b, 0x42},
	{0x3007, 0x00},
	{0x3008, 0x14},
	{0x3020, 0x58},
	{0x300D, 0x34},
	{0x300E, 0x17},
	{0x3021, 0x02},
	{0x3010, 0x59},
	{0x3002, 0x01},
	{0x3005, 0x01},
	{0x3008, 0x04},
	{0x300F, 0x70},
	{0x3010, 0x69},
	{0x3017, 0x10},
	{0x3019, 0x19},
	{0x300C, 0x62},
	{0x3064, 0x10},
	{0x3C02, 0x0F},
	{0x3C08, 0x5D},
	{0x3C09, 0xC0},
	{0x3C31, 0x54},
	{0x3C32, 0x60},
	{0x3941, 0x04},
	{0x3942, 0xB0},
	{0x3924, 0x2E},
	{0x3925, 0xE0},
};

static u16 setfile_vision_5e6_fps_15[][2] = {
	{0x0136, 0x1A},
	{0x0137, 0x00},
	{0x0305, 0x06},
	{0x0306, 0x18},
	{0x0307, 0x9C},
	{0x0308, 0x27},
	{0x0309, 0x42},
	{0x3C1F, 0x00},
	{0x3C17, 0x00},
	{0x3C0B, 0x04},
	{0x3C1C, 0x47},
	{0x3C1D, 0x15},
	{0x3C14, 0x04},
	{0x3C16, 0x00},
	{0x0820, 0x02},
	{0x0821, 0xA8},
	{0x0114, 0x01},
	{0x0344, 0x01},
	{0x0345, 0x58},
	{0x0346, 0x00},
	{0x0347, 0x14},
	{0x0348, 0x08},
	{0x0349, 0xD7},
	{0x034A, 0x07},
	{0x034B, 0x93},
	{0x034C, 0x07},
	{0x034D, 0x80},
	{0x034E, 0x07},
	{0x034F, 0x80},
	{0x0900, 0x00},
	{0x0901, 0x00},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0340, 0x0F},
	{0x0341, 0x68},
	{0x0342, 0x0B},
	{0x0343, 0x28},
	{0x0200, 0x00},
	{0x0201, 0x00},
	{0x0202, 0x03},
	{0x0203, 0xDE},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0112, 0x08},
	{0x0113, 0x08},
};

static u16 setfile_vision_5e6_fps_30[][2] = {
	{0x0136, 0x1A},
	{0x0137, 0x00},
	{0x0305, 0x06},
	{0x0306, 0x18},
	{0x0307, 0x9C},
	{0x0308, 0x27},
	{0x0309, 0x42},
	{0x3C1F, 0x00},
	{0x3C17, 0x00},
	{0x3C0B, 0x04},
	{0x3C1C, 0x47},
	{0x3C1D, 0x15},
	{0x3C14, 0x04},
	{0x3C16, 0x00},
	{0x0820, 0x02},
	{0x0821, 0xA8},
	{0x0114, 0x01},
	{0x0344, 0x01},
	{0x0345, 0x58},
	{0x0346, 0x00},
	{0x0347, 0x14},
	{0x0348, 0x08},
	{0x0349, 0xD7},
	{0x034A, 0x07},
	{0x034B, 0x93},
	{0x034C, 0x07},
	{0x034D, 0x80},
	{0x034E, 0x07},
	{0x034F, 0x80},
	{0x0900, 0x00},
	{0x0901, 0x00},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x0340, 0x07},
	{0x0341, 0xB4},
	{0x0342, 0x0B},
	{0x0343, 0x28},
	{0x0200, 0x00},
	{0x0201, 0x00},
	{0x0202, 0x03},
	{0x0203, 0xDE},
	{0x0204, 0x00},
	{0x0205, 0x20},
	{0x0112, 0x08},
	{0x0113, 0x08},
	{0x3931, 0x02},
};

static struct fimc_is_sensor_cfg config_5e6[] = {
			/* width, height, fps, settle, mode, lane, speed, interleave, pd_mode */
	FIMC_IS_SENSOR_CFG(1920, 1920, 30, 0, 0, CSI_DATA_LANES_2, 1400, CSI_MODE_VC_DT, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW8, 1920, 1920), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
};

static void sensor_5e6_vsync_work(struct work_struct *data)
{
	struct fimc_is_module_enum *module
		= container_of(data, struct fimc_is_module_enum, vsync_work);

	if (CALL_MOPS(module, stream_off, module->subdev))
		err("failed to stream off for 5E6 at recovery mode");

	if (CALL_MOPS(module, stream_on, module->subdev))
		err("failed to stream on for 5E6 at recovery mode");

	info("5e6 was recovered\n");
}

enum hrtimer_restart sensor_5e6_vsync_timeout(struct hrtimer *timer)
{
	struct fimc_is_module_enum *module
		= container_of(timer, struct fimc_is_module_enum, vsync_timer);

	schedule_work(&module->vsync_work);

	info("no stream from 5e6, start recover\n");

	return HRTIMER_NORESTART;
}

static int sensor_5e6_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_5e6_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_5e6_registered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
	return 0;
}

static void sensor_5e6_unregistered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
}

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_5e6_open,
	.close = sensor_5e6_close,
	.registered = sensor_5e6_registered,
	.unregistered = sensor_5e6_unregistered,
};

static int sensor_5e6_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	ulong i;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_5e6 *module_5e6;
	struct i2c_client *client;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *iris_hw_param;
#endif
	u8 sensor_ver = 0;
	u8 sensor_otp = 0;
	u8 page_select = 0;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	module_5e6 = module->private_data;
	client = module->client;

	module_5e6->system_clock = 146 * 1000 * 1000;
	module_5e6->line_length_pck = 146 * 1000 * 1000;

	/* sensor init */
	for (i = 0; i < ARRAY_SIZE(setfile_vision_5e6); i++) {
		ret = fimc_is_sensor_write8(client, setfile_vision_5e6[i][0],
				(u8)setfile_vision_5e6[i][1]);
		if (ret) {
#ifdef USE_CAMERA_HW_BIG_DATA
			fimc_is_sec_get_hw_param(&iris_hw_param, SENSOR_POSITION_SECURE);
			if (iris_hw_param)
				iris_hw_param->i2c_sensor_err_cnt++;
#endif
			err("i2c transfer failed.");
			ret = -EINVAL;
			goto exit;
		}

	}

	/* read sensor version */
	if (!is_iris_ver_read) {
		/* 1. read page_select */
		/* make initial state */
		ret = fimc_is_sensor_write8(client, 0x0A00, 0x04);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* set the PAGE of OTP */
		ret = fimc_is_sensor_write8(client, 0x0A02, 0x04);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* set read mode of NVM controller Interface1 */
		ret = fimc_is_sensor_write8(client, 0x0A00, 0x01);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* to wait Tmin = 47us(the time to transfer 1page data from OTP */
		usleep_range(50, 50);

		/* page select 0x01(4page), 0x03(5page) */
		ret = fimc_is_sensor_read8(client, 0x0A12, &page_select);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}

		/* 2. set page_select */
		if (page_select == 0x01 || page_select == 0x00) {
			page_select = 0x04;
		} else if (page_select == 0x03) {
			page_select = 0x05;
		} else {
			is_iris_mtf_test_check = false;
			err("[%s][%d] page read fail read data=%d", __func__, __LINE__, page_select);
			goto exit;
		}

		/* 3. read pass or fail /year/month/company information */
		/* make initial state */
		ret = fimc_is_sensor_write8(client, 0x0A00, 0x04);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* set the PAGE of OTP */
		ret = fimc_is_sensor_write8(client, 0x0A02, page_select);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* set read mode of NVM controller Interface1 */
		ret = fimc_is_sensor_write8(client, 0x0A00, 0x01);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* to wait Tmin = 47us(the time to transfer 1page data from OTP */
		usleep_range(50, 50);

		/* read the 1st byte data from buffer */
		ret = fimc_is_sensor_read8(client, 0x0A04, &sensor_otp);
		if (ret == 0) {
			if (sensor_otp == 0x01)
				is_iris_mtf_test_check = true;
			else
				is_iris_mtf_test_check = false;
			pr_info("[%s][%d] otpValue = %d\n", __func__, __LINE__, sensor_otp);
		} else if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}

		/* read year */
		ret = fimc_is_sensor_read8(client, 0x0A05, &is_iris_mtf_read_data[0]);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* read month */
		ret = fimc_is_sensor_read8(client, 0x0A06, &is_iris_mtf_read_data[1]);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
		/* read company */
		ret = fimc_is_sensor_read8(client, 0x0A07, &is_iris_mtf_read_data[2]);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}

		ret = fimc_is_sensor_read8(client, SENSOR_REG_VERSION, &sensor_ver);
		if (ret == 0) {
			is_iris_ver_read = true;
			if (sensor_ver == 0x10) {
				is_final_cam_module_iris = true;
			} else {
				is_final_cam_module_iris = false;
				module->sensor_name = "S5K5E8";
			}
			pr_info("is_final_cam_module_iris = %s, sensor version = 0x%02x\n",
				is_final_cam_module_iris ? "true" : "false", sensor_ver);
		}
	}

	hrtimer_init(&module->vsync_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	module->vsync_timer.function = sensor_5e6_vsync_timeout;
	INIT_WORK(&module->vsync_work, sensor_5e6_vsync_work);

	pr_info("[MOD:D:%d] %s(%d)\n", module->sensor_id, __func__, val);

exit:
	return ret;
}

long sensor_5e6_ioctl(struct v4l2_subdev *subdev, unsigned int cmd, void *arg)
{
	struct fimc_is_module_enum *module;

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);

	if (module) {
		if (cmd == V4L2_CID_SENSOR_DEINIT) {
			cancel_work_sync(&module->vsync_work);
		} else if (cmd == V4L2_CID_SENSOR_NOTIFY_VSYNC) {
			hrtimer_cancel(&module->vsync_timer);
		} else if (cmd == V4L2_CID_SENSOR_NOTIFY_VBLANK) {
			hrtimer_cancel(&module->vsync_timer);
		}
	}

	return 0;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_5e6_init,
	.g_ctrl = sensor_module_g_ctrl,
	.s_ctrl = sensor_module_s_ctrl,
	.g_ext_ctrls = sensor_module_g_ext_ctrls,
	.s_ext_ctrls = sensor_module_s_ext_ctrls,
	.ioctl = sensor_5e6_ioctl,
	.log_status = sensor_module_log_status,
};

static int sensor_5e6_s_routing(struct v4l2_subdev *sd,
		u32 input, u32 output, u32 config) {

	return 0;
}

#define VSYNC_TIMEOUT_IN_NSEC	(300 * NSEC_PER_MSEC) /* msec */
static int sensor_5e6_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_module_enum *module;

	pr_info("%s\n", __func__);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!module) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (enable) {
		ret = CALL_MOPS(module, stream_on, subdev);
		if (ret) {
			err("s_duration is fail(%d)", ret);
			goto p_err;
		}

		hrtimer_start(&module->vsync_timer,
		ktime_set(0,  VSYNC_TIMEOUT_IN_NSEC), HRTIMER_MODE_REL);

	} else {
		hrtimer_cancel(&module->vsync_timer);
		cancel_work_sync(&module->vsync_work);

		ret = CALL_MOPS(module, stream_off, subdev);
		if (ret) {
			err("s_duration is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int sensor_5e6_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
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

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (!module) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = CALL_MOPS(module, s_duration, subdev, duration);
	if (ret) {
		err("s_duration is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

static int sensor_5e6_s_format(struct v4l2_subdev *subdev,	struct v4l2_subdev_pad_config *cfg,	struct v4l2_subdev_format *fmt)
{
	/* TODO */
	return 0;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_routing = sensor_5e6_s_routing,
	.s_stream = sensor_5e6_s_stream,
	.s_parm = sensor_5e6_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_5e6_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int sensor_5e6_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	pr_info("%s\n", __func__);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_MIPI_CLK_CTRL, 0x01);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON_0, 0x04);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON_1, 0x01);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON_2, 0x00);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON_2, 0x00);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON_0, 0x00);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	mdelay(1);
	ret = fimc_is_sensor_write8(client, SENSOR_REG_MIPI_CLK_CTRL, 0x07);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_MIPI_CLK_CTRL, 0x06);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_MIPI_CLK_CTRL, 0x07);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_MIPI_CLK_CTRL, 0x01);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_MIPI_CLK_CTRL, 0x0F);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int sensor_5e6_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	if (unlikely(!module)) {
		err("sensor is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = module->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_OFF, 0x00);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}
	/* add 1 frame delay for stream off */
	usleep_range(40000, 44000);

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
int sensor_5e6_s_duration(struct v4l2_subdev *subdev, u64 duration)
{
	ulong i = 0;
	int ret = 0;
	struct fimc_is_module_enum *module;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	client = module->client;

	/* sensor set fps */
	if (duration == SENSOR_FPS_30) {
		for (i = 0; i < ARRAY_SIZE(setfile_vision_5e6_fps_30); i++) {
			ret = fimc_is_sensor_write8(client, setfile_vision_5e6_fps_30[i][0],
					(u8)setfile_vision_5e6_fps_30[i][1]);
			if (ret) {
				err("i2c transfer failed.");
				ret = -EINVAL;
				goto exit;
			}
		}
	} else if (duration == SENSOR_FPS_15) {
		for (i = 0; i < ARRAY_SIZE(setfile_vision_5e6_fps_15); i++) {
			ret = fimc_is_sensor_write8(client, setfile_vision_5e6_fps_15[i][0],
					(u8)setfile_vision_5e6_fps_15[i][1]);
			if (ret) {
				err("i2c transfer failed.");
				ret = -EINVAL;
				goto exit;
			}
		}
	} else {
		err("set frame rate failed. Invalid value (%d)", (u32)duration);
	}

exit:
	return ret;
}

int sensor_5e6_g_min_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_g_max_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_s_exposure(struct v4l2_subdev *subdev, u64 exposure)
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

	fimc_is_sensor_write8(client, SENSOR_REG_EXPOSURE_TIME, value);

	p_err:
	return ret;
}

int sensor_5e6_g_min_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_g_max_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_s_again(struct v4l2_subdev *subdev, u64 gain)
{
	int ret = 0;
	u8 value;
	u16 calc_value = 0;

	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	pr_info("%s(%d)\n", __func__, (u32)gain);

	calc_value = (u16)((gain * 32) / 10);

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

	value = calc_value >> 8;
	fimc_is_sensor_write8(client, SENSOR_REG_GAIN_ADJUST_0, value);
	value = calc_value & 0xFF;
	fimc_is_sensor_write8(client, SENSOR_REG_GAIN_ADJUST_1, value);

	p_err:
	return ret;
}

int sensor_5e6_g_min_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_g_max_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_s_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_g_min_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_g_max_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_s_shutterspeed(struct v4l2_subdev *subdev, u64 shutterspeed)
{
	int ret = 0;
	u8 value;
	u16 calc_value = 0;

	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	pr_info("%s(%d)\n", __func__, (u32)shutterspeed);

	calc_value = (u16)(((shutterspeed * 169 * 1000) / 2856) / 10);

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

	value = calc_value >> 8;
	fimc_is_sensor_write8(client, SENSOR_REG_SHUTTERSPEED_ADJUST_0, value);
	value = calc_value & 0xFF;
	fimc_is_sensor_write8(client, SENSOR_REG_SHUTTERSPEED_ADJUST_1, value);

p_err:
	return ret;

}

int sensor_5e6_g_min_shutterspeed(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5e6_g_max_shutterspeed(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

struct fimc_is_sensor_ops module_5e6_ops = {
	.stream_on	= sensor_5e6_stream_on,
	.stream_off	= sensor_5e6_stream_off,
	.s_duration	= sensor_5e6_s_duration,
	.g_min_duration	= sensor_5e6_g_min_duration,
	.g_max_duration	= sensor_5e6_g_max_duration,
	.s_exposure	= sensor_5e6_s_exposure,
	.g_min_exposure	= sensor_5e6_g_min_exposure,
	.g_max_exposure	= sensor_5e6_g_max_exposure,
	.s_again	= sensor_5e6_s_again,
	.g_min_again	= sensor_5e6_g_min_again,
	.g_max_again	= sensor_5e6_g_max_again,
	.s_dgain	= sensor_5e6_s_dgain,
	.g_min_dgain	= sensor_5e6_g_min_dgain,
	.g_max_dgain	= sensor_5e6_g_max_dgain,
	.s_shutterspeed = sensor_5e6_s_shutterspeed,
	.g_min_shutterspeed = sensor_5e6_g_min_shutterspeed,
	.g_max_shutterspeed = sensor_5e6_g_min_shutterspeed
};

#ifdef CONFIG_OF
static int sensor_5e6_power_setpin(struct device *dev,
		struct exynos_platform_fimc_is_module *pdata)
{
	struct device_node *dnode;
	int gpio_none = 0, gpio_reset = 0;
	int gpio_mclk = 0, gpio_iris_2p8_en = 0;
	int gpio_iris_en = 0;
	u32 power_seq_id = 0;
	int ret;

	FIMC_BUG(!dev);

	pr_info("%s\n", __func__);

	dnode = dev->of_node;

	gpio_reset = of_get_named_gpio(dnode, "gpio_reset", 0);
	if (!gpio_is_valid(gpio_reset)) {
		dev_err(dev, "%s: failed to get PIN_RESET\n", __func__);
		return -EINVAL;
	} else {
		gpio_request_one(gpio_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_reset);
	}

	gpio_mclk = of_get_named_gpio(dnode, "gpio_mclk", 0);
	if (!gpio_is_valid(gpio_mclk)) {
		dev_err(dev, "%s: failed to get mclk\n", __func__);
		return -EINVAL;
	} else {
		if (gpio_request_one(gpio_mclk, GPIOF_OUT_INIT_LOW, "CAM_MCLK_OUTPUT_LOW")) {
			dev_err(dev, "%s: failed to gpio request mclk\n", __func__);
			return -ENODEV;
		}
		gpio_free(gpio_mclk);
	}

	gpio_iris_en = of_get_named_gpio(dnode, "gpio_iris_en", 0);
	if (!gpio_is_valid(gpio_iris_en)) {
		dev_err(dev, "%s: failed to get gpio_iris_en\n", __func__);
		return -EINVAL;
	} else {
		gpio_request_one(gpio_iris_en, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_iris_en);
	}

	ret = of_property_read_u32(dnode, "power_seq_id", &power_seq_id);
	if (ret) {
		dev_err(dev, "power_seq_id read is fail(%d)", ret);
		power_seq_id = 0;
	}

	/* before board Revision 0.2 for dream2*/
	if (power_seq_id == 0) {
		gpio_iris_2p8_en = of_get_named_gpio(dnode, "gpio_iris_2p8_en", 0);
		if (!gpio_is_valid(gpio_iris_2p8_en)) {
			dev_err(dev, "%s: failed to get gpio_iris_2p8_en\n", __func__);
			return -EINVAL;
		} else {
			gpio_request_one(gpio_iris_2p8_en, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
			gpio_free(gpio_iris_2p8_en);
		}
	}

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF);

	/* IRIS CAMERA  - POWER ON */
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_iris_en, "gpio_iris_en", PIN_OUTPUT, 1, 0);  // iris_en
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_none, "VDD_IRIS_1P8", PIN_REGULATOR, 1, 0);  //    1P8
	/* before board Revision 0.2 for dream2*/
	if (power_seq_id == 0) {
		SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_iris_2p8_en, "gpio_iris_2p8_en", PIN_OUTPUT, 1, 0);   //    2P8
	} else {
		SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_none, "VDD_IRIS_2P8", PIN_REGULATOR, 1, 0);  //    2P8
	}
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_none, "VDD_IRIS_1P2", PIN_REGULATOR, 1, 1);  //    1P2
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_reset, "rst_high", PIN_OUTPUT, 1, 0);    // reset high
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 2, 1000);

	/* IRIS CAMERA  - POWER OFF */
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_reset, "rst_low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_none, "VDD_IRIS_1P2", PIN_REGULATOR, 0, 0);
	/* before board Revision 0.2 for dream2*/
	if (power_seq_id == 0) {
		SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_iris_2p8_en, "gpio_iris_2p8_en", PIN_OUTPUT, 0, 0);   //    2P8
	} else {
		SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_none, "VDD_IRIS_2P8", PIN_REGULATOR, 0, 0);  //    2P8
	}
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_none, "VDD_IRIS_1P8", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF, gpio_iris_en, "gpio_iris_en", PIN_OUTPUT, 0, 0);  // iris_en
	return 0;
}
#endif /* CONFIG_OF */

static int sensor_module_5e6_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	struct exynos_platform_fimc_is_module *pdata;
	struct device *dev;
	struct pinctrl_state *s;

	FIMC_BUG(!client);

	if (fimc_is_dev == NULL) {
		warn("fimc_is_dev is not yet probed(5e6)");
		client->dev.init_name = SENSOR_NAME;
		return -EPROBE_DEFER;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &client->dev;

#ifdef CONFIG_OF
	fimc_is_module_parse_dt(dev, sensor_5e6_power_setpin);
#endif
	pdata = dev_get_platdata(dev);
	device = &core->sensor[pdata->id];

	probe_info("%s(%d) pdata->id = %d\n", __func__, ret, pdata->id);

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	/* S5K5E6 */
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_S5K5E6;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = client;
	module->active_width = 1920;
	module->active_height = 1920;
	module->pixel_width = module->active_width + 0;
	module->pixel_height = module->active_height + 0;
	module->max_framerate = 30;
	module->position = pdata->position;
	module->bitwidth = 8;
	module->sensor_maker = "SLSI";
	module->sensor_name = "S5K5E6";
	module->setfile_name = "setfile_5e6.bin";

	module->cfgs = ARRAY_SIZE(config_5e6);
	module->cfg = config_5e6;
#if 0
	module->private_data = kzalloc(sizeof(struct fimc_is_module_5e6), GFP_KERNEL);
	if (!module->private_data) {
		err("private_data is NULL");
		ret = -ENOMEM;
		kfree(subdev_module);
		goto p_err;
	}
#else
/* Sensor peri */
	module->private_data = kzalloc(sizeof(struct fimc_is_device_sensor_peri), GFP_KERNEL);
	if (!module->private_data) {
		probe_err("fimc_is_device_sensor_peri is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	(((struct fimc_is_device_sensor_peri*)module->private_data)->cis).cis_data = kzalloc(sizeof(cis_shared_data), GFP_KERNEL);

	fimc_is_sensor_peri_probe((struct fimc_is_device_sensor_peri*)module->private_data);
	PERI_SET_MODULE(module);
#endif
	module->ops = &module_5e6_ops;

	ext = &module->ext;

	ext->sensor_con.product_name = SENSOR_S5K5E6_NAME;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = pdata->sensor_i2c_ch;
	ext->sensor_con.peri_setting.i2c.slave_address = pdata->sensor_i2c_addr;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->actuator_con.product_name = ACTUATOR_NAME_NOTHING;
	ext->flash_con.product_name = FLADRV_NAME_NOTHING;
	ext->from_con.product_name = FROMDRV_NAME_NOTHING;
	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;
	ext->ois_con.product_name = OIS_NAME_NOTHING;

	v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);
	subdev_module->internal_ops = &internal_ops;

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->sensor_id);

	s = pinctrl_lookup_state(pdata->pinctrl, "release");

	if (pinctrl_select_state(pdata->pinctrl, s) < 0) {
		probe_err("pinctrl_select_state is fail\n");
		goto p_err;
	}

p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;
}


#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_module_5e6_match[] = {
	{
		.compatible = "samsung,sensor-module-5e6",
	},
	{},
};
#endif

static const struct i2c_device_id sensor_module_5e6_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_module_5e6_driver = {
	.probe  = sensor_module_5e6_probe,
	.driver = {
		.name	= SENSOR_NAME,
		.owner  = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_module_5e6_match,
#endif
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_module_5e6_idt,
};

static int __init fimc_is_sensor_module_5e6_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_module_5e6_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_module_5e6_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(fimc_is_sensor_module_5e6_init);

MODULE_AUTHOR("Gilyeon lim");
MODULE_DESCRIPTION("Sensor 5E6 driver");
MODULE_LICENSE("GPL v2");

