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
#include "fimc-is-device-module-5f1.h"

#define SENSOR_NAME "S5K5F1"

#define SENSOR_REG_DURATION_MSB			(0x6026)
#define SENSOR_REG_STREAM_ON			(0x0100)
#define SENSOR_REG_STREAM_OFF			(0x0100)
#define SENSOR_REG_EXPOSURE_TIME		(0x0202)
#define SENSOR_REG_GAIN_ADJUST			(0x0204)
#define SENSOR_REG_SHUTTERSPEED_ADJUST		(0x0202)
#define SENSOR_REG_FRAME_LENGTH_LINE		(0x0340)
#define SENSOR_REG_VERSION			(0x0002)
#define SENSOR_REG_MIPI_CLK_CTRL		(0x3929)
#define SENSOR_FPS_30		33333333
#define SENSOR_FPS_15		66666666
#define VT_PIX_CLK_FREQ_HZ	(99666667)
#define LINE_LENGTH_PCK		(0x0A38)

#define SENSOR_SCENARIO_SECURE      6

bool is_final_cam_module_iris;
bool is_iris_ver_read;
bool is_iris_mtf_test_check;
u8 is_iris_mtf_read_data[3];	/* 0:year 1:month 2:company */

static bool is_need_init_setting;

/* S5K5F1S_EVT0_Ver_0.5_20170710.xlsx */
static u16 setfile_vision_5f1_reset[][2] = {
	{0xFCFC, 0x4000},
	{0x6010, 0x0001},
};

static u16 setfile_vision_5f1_open_clock[][2] = {
	{0xFCFC, 0x4000},
	{0x6214, 0x7971},
	{0x6218, 0x7150},
};

static u16 setfile_vision_5f1_fps_30_26MHz[][2] = {
	{0xF43E, 0x24CE},
	{0xF440, 0x402F},
	{0x354C, 0x0004},
	{0x3544, 0x110B},
	{0x3540, 0x110B},
	{0x3082, 0x0100},
	{0x3168, 0x00A0},
	{0x31A6, 0x0100},
	{0xF470, 0x0000},
	{0xF43A, 0x0010},
	{0x3572, 0x0012},
	{0xF420, 0x0013},
	{0xF422, 0x0000},
	{0xF424, 0x000B},
	{0xF426, 0x000E},
	{0xF488, 0x0008},
	{0x3534, 0x0708},
	{0x320E, 0x0000},
	{0x3304, 0x0094},
	{0xF4BA, 0x0008},
	{0x319E, 0x0001},
	{0x3078, 0x0340},
	{0x31C6, 0x0000},
	{0x6028, 0x2000},
	{0x602A, 0x0E1E},
	{0x6F12, 0x8110},
	{0x0306, 0x008A},
	{0x030E, 0x009D},
	{0x3560, 0x0050},
	{0x0342, 0x0A38},
	{0x0340, 0x13D7},
	{0x0202, 0x0339},
	{0xB134, 0x0180},
	{0x0344, 0x0008},
	{0x0346, 0x0008},
	{0x0348, 0x0967},
	{0x034A, 0x0967},
	{0x034C, 0x0960},
	{0x034E, 0x0960},
	{0x0112, 0x0A08},
	{0xF1A2, 0x0200},
	{0xF1A8, 0x09B0},
	{0xF1AA, 0x09B0},
	{0x30BE, 0x0100},
	{0x6028, 0x2000},
	{0x602A, 0x0F10},
	{0x6F12, 0x0001},
	{0x602A, 0x0F32},
	{0x6F12, 0x0001},
	{0x602A, 0x0F1E},
	{0x6F12, 0x0487},
	{0x6F12, 0x0725},
};

static struct fimc_is_sensor_cfg config_5f1[] = {
	/* 2400x2400@30fps */
	FIMC_IS_SENSOR_CFG(2400, 2400, 30, 0, 0, CSI_DATA_LANES_2, 2041, CSI_MODE_CH0_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW8, 2400, 2400), VC_OUT(HW_FORMAT_RAW8, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_USER, 0, 0), VC_OUT(HW_FORMAT_USER, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0))
};

static void sensor_5f1_vsync_work(struct work_struct *data)
{
	struct fimc_is_module_enum *module
		= container_of(data, struct fimc_is_module_enum, vsync_work);

	if (CALL_MOPS(module, stream_off, module->subdev))
		err("failed to stream off for 5F1 at recovery mode");

	if (CALL_MOPS(module, stream_on, module->subdev))
		err("failed to stream on for 5F1 at recovery mode");

	info("5f1 was recovered\n");
}

enum hrtimer_restart sensor_5f1_vsync_timeout(struct hrtimer *timer)
{
	struct fimc_is_module_enum *module
		= container_of(timer, struct fimc_is_module_enum, vsync_timer);

	schedule_work(&module->vsync_work);

	info("no stream from 5f1, start recover\n");

	return HRTIMER_NORESTART;
}

static int sensor_5f1_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_5f1_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	pr_info("%s\n", __func__);
	return 0;
}
static int sensor_5f1_registered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
	return 0;
}

static void sensor_5f1_unregistered(struct v4l2_subdev *sd)
{
	pr_info("%s\n", __func__);
}

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_5f1_open,
	.close = sensor_5f1_close,
	.registered = sensor_5f1_registered,
	.unregistered = sensor_5f1_unregistered,
};

static int sensor_5f1_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	ulong i;
	struct fimc_is_module_enum *module;
	struct fimc_is_module_5f1 *module_5f1;
	struct i2c_client *client;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct fimc_is_device_sensor *device = NULL;
#endif
	u8 sensor_test[3] = {0,};
	u8 page_info = 0;
	u16 page_select = 0;
	u8 sensor_ver = 0;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	module_5f1 = module->private_data;
	client = module->client;

	module_5f1->system_clock = 146 * 1000 * 1000;
	module_5f1->line_length_pck = 146 * 1000 * 1000;

	/* sensor reset */
	for (i = 0; i < (int)ARRAY_SIZE(setfile_vision_5f1_reset); i++) {
		ret = fimc_is_sensor_write16(client, setfile_vision_5f1_reset[i][0],
				(u16)setfile_vision_5f1_reset[i][1]);
		if (ret) {
#ifdef USE_CAMERA_HW_BIG_DATA
			device = v4l2_get_subdev_hostdata(subdev);
			fimc_is_sec_get_hw_param(&hw_param, device->position);
			if (hw_param)
				hw_param->i2c_sensor_err_cnt++;
#endif
			err("i2c transfer failed.%x", client->addr);
			ret = -EINVAL;
			goto exit;
		}
	}

	/* add 5 ms delay */
	usleep_range(5000, 5010);

	/* sensor open clock */
	for (i = 0; i < (int)ARRAY_SIZE(setfile_vision_5f1_open_clock); i++) {
		ret = fimc_is_sensor_write16(client, setfile_vision_5f1_open_clock[i][0],
				(u16)setfile_vision_5f1_open_clock[i][1]);
		if (ret) {
#ifdef USE_CAMERA_HW_BIG_DATA
			device = v4l2_get_subdev_hostdata(subdev);
			fimc_is_sec_get_hw_param(&hw_param, device->position);
			if (hw_param)
				hw_param->i2c_sensor_err_cnt++;
#endif
			err("i2c transfer failed.");
			ret = -EINVAL;
			goto exit;
		}
	}

	is_need_init_setting = true;

	/* read sensor version */
	if (!is_iris_ver_read) {
		/* Streaming ON */
		ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON, 0x01);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

		usleep_range(4000, 4010);

		/* page select */
		/* set the PAGE 62 for reading page_info number */
		ret = fimc_is_sensor_write16(client, 0x0A02, 0x3E00);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

		/* read start */
		ret = fimc_is_sensor_write16(client, 0x0A00, 0x0100);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

		usleep_range(350, 360);

		/* read page_info number 0x01(p.62), 0x03(p.63) */
		ret = fimc_is_sensor_read8(client, 0x0A12, &page_info);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

		/* set page_select */
		pr_info("%s(page_select:0x%x)\n", __func__, page_info);
		if (page_info == 0x01 || page_info == 0x00) {
			page_select = 0x3E00;
		} else if (page_info == 0x03) {
			page_select = 0x3F00;
		} else {
			is_iris_mtf_test_check = false;
			err("[%s][%d] page read fail read data=%d", __func__, __LINE__, page_select);
			goto streamoff_exit;
		}

		/* set the PAGE of OTP */
		ret = fimc_is_sensor_write16(client, 0x0A02, page_select);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

		/* read start */
		ret = fimc_is_sensor_write16(client, 0x0A00, 0x0100);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

		usleep_range(350, 360);

		/* read pass or fail /year/month/company information */
		/* read test result */
		ret = fimc_is_sensor_read8(client, 0x0A04, &sensor_test[0]); /* focusing */
		ret |= fimc_is_sensor_read8(client, 0x0A05, &sensor_test[1]); /* macro */
		ret |= fimc_is_sensor_read8(client, 0x0A06, &sensor_test[2]); /* pan */
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		} else {
			if (sensor_test[0] == 0x01 && sensor_test[1] == 0x01 && sensor_test[2] == 0x01)
				is_iris_mtf_test_check = true;
			else
				is_iris_mtf_test_check = false;

			pr_info("[%s][%d] sensor_test = %d, %d, %d\n",
				__func__, __LINE__, sensor_test[0], sensor_test[1], sensor_test[2]);
		}

		ret = fimc_is_sensor_read8(client, 0x0A07, &is_iris_mtf_read_data[0]); /* read year */
		ret |= fimc_is_sensor_read8(client, 0x0A08, &is_iris_mtf_read_data[1]); /* read month */
		ret |= fimc_is_sensor_read8(client, 0x0A09, &is_iris_mtf_read_data[2]); /* read company */
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}
		pr_info("%s : is_iris_mtf_read_data : year(0x%2X), month(0x%2X), company(0x%2X)\n", __func__,
			is_iris_mtf_read_data[0], is_iris_mtf_read_data[1], is_iris_mtf_read_data[2]);

		/* check final sensor */
		ret = fimc_is_sensor_read8(client, SENSOR_REG_VERSION, &sensor_ver);
		if (ret == 0) {
			is_iris_ver_read = true;
			if (sensor_ver == 0xA1) {
				is_final_cam_module_iris = true;
			} else {
				is_final_cam_module_iris = false;
				module->sensor_name = "S5K5F1";
			}
			pr_info("%s : is_final_cam_module_iris = %s, sensor version = 0x%02x\n",
				__func__, is_final_cam_module_iris ? "true" : "false", sensor_ver);
		}

		/* make initial state */
		ret = fimc_is_sensor_write16(client, 0x0A00, 0x0000);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto streamoff_exit;
		}

streamoff_exit:
		/* Streaming OFF */
		ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON, 0x00);
		if (ret) {
			err("[%s][%d] i2c transfer failed", __func__, __LINE__);
			ret = -EINVAL;
			goto exit;
		}
	}

	hrtimer_init(&module->vsync_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	module->vsync_timer.function = sensor_5f1_vsync_timeout;
	INIT_WORK(&module->vsync_work, sensor_5f1_vsync_work);

	pr_info("[MOD:D:%d] %s(%d)\n", module->sensor_id, __func__, val);

exit:
	return ret;
}

long sensor_5f1_ioctl(struct v4l2_subdev *subdev, unsigned int cmd, void *arg)
{
	struct fimc_is_module_enum *module;

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);

	if (module) {
		if (cmd == V4L2_CID_SENSOR_DEINIT)
			cancel_work_sync(&module->vsync_work);
		else if (cmd == V4L2_CID_SENSOR_NOTIFY_VSYNC)
			hrtimer_cancel(&module->vsync_timer);
		else if (cmd == V4L2_CID_SENSOR_NOTIFY_VBLANK)
			hrtimer_cancel(&module->vsync_timer);
	}

	return 0;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_5f1_init,
	.g_ctrl = sensor_module_g_ctrl,
	.s_ctrl = sensor_module_s_ctrl,
	.g_ext_ctrls = sensor_module_g_ext_ctrls,
	.s_ext_ctrls = sensor_module_s_ext_ctrls,
	.ioctl = sensor_5f1_ioctl,
	.log_status = sensor_module_log_status,
};

static int sensor_5f1_s_routing(struct v4l2_subdev *sd,
		u32 input, u32 output, u32 config) {

	return 0;
}

#define VSYNC_TIMEOUT_IN_NSEC	(300 * NSEC_PER_MSEC) /* msec */
static int sensor_5f1_s_stream(struct v4l2_subdev *subdev, int enable)
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
			err("stream_on is fail(%d)", ret);
			goto p_err;
		}

		hrtimer_start(&module->vsync_timer,
		ktime_set(0,  VSYNC_TIMEOUT_IN_NSEC), HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&module->vsync_timer);
		cancel_work_sync(&module->vsync_work);

		ret = CALL_MOPS(module, stream_off, subdev);
		if (ret) {
			err("stream_off is fail(%d)", ret);
			goto p_err;
		}
	}

p_err:
	return ret;
}

static int sensor_5f1_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
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

	duration = ((u64)tpf->numerator * 1000 * 1000 * 1000) /
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

static int sensor_5f1_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg, struct v4l2_subdev_format *fmt)
{
	/* TODO */
	return 0;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_routing = sensor_5f1_s_routing,
	.s_stream = sensor_5f1_s_stream,
	.s_parm = sensor_5f1_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_5f1_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int sensor_5f1_stream_on(struct v4l2_subdev *subdev)
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

	pr_info("%s\n", __func__);

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON, 0x01);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int sensor_5f1_stream_off(struct v4l2_subdev *subdev)
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

	pr_info("%s\n", __func__);

	ret = fimc_is_sensor_write8(client, SENSOR_REG_STREAM_ON, 0x00);
	if (ret < 0) {
		err("fimc_is_sensor_write8 is fail(%d)", ret);
		goto p_err;
	}

	/* add 1 frame delay for stream off */
	usleep_range(40000, 40000);

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
int sensor_5f1_s_duration(struct v4l2_subdev *subdev, u64 duration)
{
	ulong i = 0;
	int ret = 0;
	u16 fll;
	struct fimc_is_module_enum *module;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	module = (struct fimc_is_module_enum *)v4l2_get_subdevdata(subdev);
	client = module->client;

	/* init setting */
	if (is_need_init_setting == true) {
		pr_info("%s init setting start\n", __func__);

		for (i = 0; i < (int)ARRAY_SIZE(setfile_vision_5f1_fps_30_26MHz); i++) {
			ret = fimc_is_sensor_write16(client, setfile_vision_5f1_fps_30_26MHz[i][0],
					(u16)setfile_vision_5f1_fps_30_26MHz[i][1]);
			if (ret) {
				err("i2c transfer failed.");
				ret = -EINVAL;
				goto p_err;
			}
		}

		is_need_init_setting = false;

		pr_info("%s init setting done\n", __func__);
	}

	/* sensor set fps */
	if (duration == SENSOR_FPS_30) {
		fll = 0x13D7;
		ret = fimc_is_sensor_write16(client, SENSOR_REG_FRAME_LENGTH_LINE, fll);
		pr_info("%s set frame rate(%d 0x%x)\n", __func__, (u32)duration, fll);
	} else {
		err("set frame rate failed. Invalid value (%d)", (u32)duration);
	}

p_err:
	return ret;
}

int sensor_5f1_g_min_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_g_max_duration(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_s_exposure(struct v4l2_subdev *subdev, u64 exposure)
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

	value = exposure & 0xFFFF;

	fimc_is_sensor_write16(client, SENSOR_REG_EXPOSURE_TIME, value);

p_err:
	return ret;
}

int sensor_5f1_g_min_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_g_max_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_s_again(struct v4l2_subdev *subdev, u64 gain)
{
	int ret = 0;
	u16 calc_value = 0;

	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	/* gain : (ex) 10 = x1, 160 = x16 */
	calc_value = (u16)((gain * 32) / 10);

	pr_info("%s(%d 0x%x)\n", __func__, (u32)gain, (u32)calc_value);

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

	fimc_is_sensor_write16(client, SENSOR_REG_GAIN_ADJUST, calc_value);

p_err:
	return ret;
}

int sensor_5f1_g_min_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_g_max_again(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_s_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_g_min_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_g_max_dgain(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_s_shutterspeed(struct v4l2_subdev *subdev, u64 shutterspeed)
{
	int ret = 0;
	u16 calc_value = 0;

	struct fimc_is_module_enum *sensor;
	struct i2c_client *client;

	FIMC_BUG(!subdev);

	/* shutterspeed : (ex) 332 = 33.2ms -> (332/10000)s */
	calc_value = (u16)((shutterspeed * VT_PIX_CLK_FREQ_HZ * 4) / 10000 / LINE_LENGTH_PCK);

	pr_info("%s(%d 0x%x)\n", __func__, (u32)shutterspeed, (u32)calc_value);

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

	fimc_is_sensor_write16(client, SENSOR_REG_SHUTTERSPEED_ADJUST, calc_value);

p_err:
	return ret;

}

int sensor_5f1_g_min_shutterspeed(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

int sensor_5f1_g_max_shutterspeed(struct v4l2_subdev *subdev)
{
	int ret = 0;
	return ret;
}

struct fimc_is_sensor_ops module_5f1_ops = {
	.stream_on	= sensor_5f1_stream_on,
	.stream_off	= sensor_5f1_stream_off,
	.s_duration	= sensor_5f1_s_duration,
	.g_min_duration	= sensor_5f1_g_min_duration,
	.g_max_duration	= sensor_5f1_g_max_duration,
	.s_exposure	= sensor_5f1_s_exposure,
	.g_min_exposure	= sensor_5f1_g_min_exposure,
	.g_max_exposure	= sensor_5f1_g_max_exposure,
	.s_again	= sensor_5f1_s_again,
	.g_min_again	= sensor_5f1_g_min_again,
	.g_max_again	= sensor_5f1_g_max_again,
	.s_dgain	= sensor_5f1_s_dgain,
	.g_min_dgain	= sensor_5f1_g_min_dgain,
	.g_max_dgain	= sensor_5f1_g_max_dgain,
	.s_shutterspeed = sensor_5f1_s_shutterspeed,
	.g_min_shutterspeed = sensor_5f1_g_min_shutterspeed,
	.g_max_shutterspeed = sensor_5f1_g_min_shutterspeed
};

#ifdef CONFIG_OF
static int sensor_5f1_power_setpin(struct device *dev,
		struct exynos_platform_fimc_is_module *pdata)
{
	struct device_node *dnode;
	int gpio_none = 0, gpio_reset = 0;
	int gpio_mclk = 0;
	int gpio_iris_en = 0;

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
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF);

	/* IRIS CAMERA  - POWER ON */
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_iris_en, "gpio_iris_en", PIN_OUTPUT, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_none, "VDDIO_1.8V_IRIS", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_none, "VDDA_2.8V_IRIS", PIN_REGULATOR, 1, 0);
#ifdef CONFIG_SEC_FACTORY
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_none, "VDDD_1.05V_IRIS", PIN_REGULATOR, 1, 1000, 1000000);
#else
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_none, "VDDD_1.05V_IRIS", PIN_REGULATOR, 1, 1000, 1050000);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_none, "on_i2c", PIN_I2C, 1, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_reset, "rst_high", PIN_OUTPUT, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_ON,
		gpio_none, "pin", PIN_FUNCTION, 2, 2000);

	/* IRIS CAMERA  - POWER OFF */
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_none, "pin", PIN_FUNCTION, 1, 200);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_reset, "rst_low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_none, "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_none, "VDDD_1.05V_IRIS", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_none, "VDDA_2.8V_IRIS", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_none, "VDDIO_1.8V_IRIS", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_SECURE, GPIO_SCENARIO_OFF,
		gpio_iris_en, "gpio_iris_en", PIN_OUTPUT, 0, 0);

	return 0;
}
#endif /* CONFIG_OF */

int sensor_5f1_probe(struct i2c_client *client,
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
		warn("fimc_is_dev is not yet probed(5f1)");
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
	fimc_is_module_parse_dt(dev, sensor_5f1_power_setpin);
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

	/* S5K5F1 */
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_S5K5F1;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = client;
	module->active_width = 2400;
	module->active_height = 2400;
	module->pixel_width = module->active_width + 0;
	module->pixel_height = module->active_height + 0;
	module->max_framerate = 30;
	module->position = pdata->position;
	module->bitwidth = 8;

	module->sensor_maker = "SLSI";
	module->sensor_name = "S5K5F1";
	module->setfile_name = "setfile_5f1.bin";

	module->cfgs = ARRAY_SIZE(config_5f1);
	module->cfg = config_5f1;

	/* Sensor peri */
	module->private_data = kzalloc(sizeof(struct fimc_is_device_sensor_peri), GFP_KERNEL);
	if (!module->private_data) {
		probe_err("fimc_is_device_sensor_peri is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	(((struct fimc_is_device_sensor_peri *)module->private_data)->cis).cis_data
		= kzalloc(sizeof(cis_shared_data), GFP_KERNEL);

	fimc_is_sensor_peri_probe((struct fimc_is_device_sensor_peri *)module->private_data);
	PERI_SET_MODULE(module);

	module->ops = &module_5f1_ops;

	ext = &module->ext;
	ext->mipi_lane_num = module->cfg->lanes;

	ext->sensor_con.product_name = SENSOR_S5K5F1_NAME;
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

	is_iris_ver_read = false;

p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;
}


static int sensor_5f1_remove(struct i2c_client *client)
{
	int ret = 0;
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_sensor_5f1_match[] = {
	{
		.compatible = "samsung,sensor-module-5f1",
	},
	{},
};
#endif

static const struct i2c_device_id sensor_5f1_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_5f1_driver = {
	.driver = {
		.name	= SENSOR_NAME,
		.owner  = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = exynos_fimc_is_sensor_5f1_match
#endif
	},
	.probe	= sensor_5f1_probe,
	.remove	= sensor_5f1_remove,
	.id_table = sensor_5f1_idt
};

static int __init sensor_5f1_load(void)
{
	probe_info("%s\n", __func__);
	return i2c_add_driver(&sensor_5f1_driver);
}

static void __exit sensor_5f1_unload(void)
{
	i2c_del_driver(&sensor_5f1_driver);
}

module_init(sensor_5f1_load);
module_exit(sensor_5f1_unload);

MODULE_AUTHOR("Gilyeon lim");
MODULE_DESCRIPTION("Sensor 5F1 driver");
MODULE_LICENSE("GPL v2");

