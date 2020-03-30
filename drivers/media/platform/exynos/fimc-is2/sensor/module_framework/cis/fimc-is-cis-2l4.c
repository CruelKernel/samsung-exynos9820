/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-param.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"
#include "fimc-is-cis-2l4.h"
#ifndef USE_FLIP_WIDE_SENSOR
#include "fimc-is-cis-2l4-setA.h"
#else
#include "fimc-is-cis-2l4-setB.h"
#endif

#include "fimc-is-helper-i2c.h"

#define SENSOR_NAME "S5K2L4"
/* #define DEBUG_2L4_PLL */

static const u32 *sensor_2l4_reset_tnp;
static u32 sensor_2l4_reset_tnp_size;
static const u32 *sensor_2l4_global;
static u32 sensor_2l4_global_size;
static const u32 *sensor_2l4_dram_test_global;
static u32 sensor_2l4_dram_test_global_size;
static const u32 **sensor_2l4_setfiles;
static const u32 *sensor_2l4_setfile_sizes;
static const struct sensor_pll_info_compact **sensor_2l4_pllinfos;
static u32 sensor_2l4_max_setfile_num;
#ifdef CONFIG_SENSOR_RETENTION_USE
static const u32 *sensor_2l4_global_retention;
static u32 sensor_2l4_global_retention_size;
static const u32 **sensor_2l4_retention;
static const u32 *sensor_2l4_retention_size;
static u32 sensor_2l4_max_retention_num;
static const u32 **sensor_2l4_load_sram;
static const u32 *sensor_2l4_load_sram_size;
#endif

static int sensor_2l4_ln_mode_delay_count;
static u8 sensor_2l4_ln_mode_frame_count;
static bool sensor_2l4_load_retention;

/* For Recovery */
static u32 sensor_2l4_frame_duration_backup;
static struct ae_param sensor_2l4_again_backup;
static struct ae_param sensor_2l4_dgain_backup;
static struct ae_param sensor_2l4_target_exp_backup;

static bool sensor_2l4_cis_is_wdr_mode_on(cis_shared_data *cis_data)
{
	unsigned int mode = cis_data->sens_config_index_cur;

	if (!fimc_is_vender_wdr_mode_on(cis_data))
		return false;

	if (mode < 0 || mode >= SENSOR_2L4_MODE_MAX) {
		err("invalid mode(%d)!!", mode);
		return false;
	}

	return sensor_2l4_support_wdr[mode];
}

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
static const struct cam_mipi_sensor_mode *sensor_2l4_mipi_sensor_mode;
static u32 sensor_2l4_mipi_sensor_mode_size;
static const int *sensor_2l4_verify_sensor_mode;
static int sensor_2l4_verify_sensor_mode_size;

static int sensor_2l4_cis_set_mipi_clock(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;
	int mode = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

	mode = cis->cis_data->sens_config_index_cur;

	dbg_sensor(1, "%s : mipi_clock_index_cur(%d), new(%d)\n", __func__,
		cis->mipi_clock_index_cur, cis->mipi_clock_index_new);

	if (mode >= sensor_2l4_mipi_sensor_mode_size) {
		err("sensor mode is out of bound");
		return -1;
	}

	if (cis->mipi_clock_index_cur != cis->mipi_clock_index_new
		&& cis->mipi_clock_index_new >= 0) {
		cur_mipi_sensor_mode = &sensor_2l4_mipi_sensor_mode[mode];

		if (cur_mipi_sensor_mode->sensor_setting == NULL) {
			dbg_sensor(1, "no mipi setting for current sensor mode\n");
		} else if (cis->mipi_clock_index_new < cur_mipi_sensor_mode->sensor_setting_size) {
			info("%s: change mipi clock [%d %d]\n", __func__, mode, cis->mipi_clock_index_new);
			sensor_cis_set_registers(subdev,
				cur_mipi_sensor_mode->sensor_setting[cis->mipi_clock_index_new].setting,
				cur_mipi_sensor_mode->sensor_setting[cis->mipi_clock_index_new].setting_size);

			cis->mipi_clock_index_cur = cis->mipi_clock_index_new;
		} else {
			err("sensor setting index is out of bound %d %d",
				cis->mipi_clock_index_new, cur_mipi_sensor_mode->sensor_setting_size);
		}
	}

	return ret;
}
#endif

#ifdef USE_CAMERA_EMBEDDED_HEADER
#define SENSOR_2L4_PAGE_LENGTH 256
#define SENSOR_2L4_VALID_TAG 0x5A
#define SENSOR_2L4_FRAME_ID_PAGE 1
#define SENSOR_2L4_FRAME_ID_OFFSET 194
#define SENSOR_2L4_FLL_MSB_PAGE 1
#define SENSOR_2L4_FLL_MSB_OFFSET 48
#define SENSOR_2L4_FLL_LSB_PAGE 1
#define SENSOR_2L4_FLL_LSB_OFFSET 50
#define SENSOR_2L4_FRAME_COUNT_PAGE 0
#define SENSOR_2L4_FRAME_COUNT_OFFSET 16

#ifdef USE_CAMERA_SSM_TEST
static int record_status;
#endif
static u32 frame_id_idx = (SENSOR_2L4_PAGE_LENGTH * SENSOR_2L4_FRAME_ID_PAGE) + SENSOR_2L4_FRAME_ID_OFFSET;
static u32 fll_msb_idx = (SENSOR_2L4_PAGE_LENGTH * SENSOR_2L4_FLL_MSB_PAGE) + SENSOR_2L4_FLL_MSB_OFFSET;
static u32 fll_lsb_idx = (SENSOR_2L4_PAGE_LENGTH * SENSOR_2L4_FLL_LSB_PAGE) + SENSOR_2L4_FLL_LSB_OFFSET;
static u32 llp_msb_idx = (SENSOR_2L4_PAGE_LENGTH * SENSOR_2L4_FLL_MSB_PAGE) + SENSOR_2L4_FLL_MSB_OFFSET+4;
static u32 llp_lsb_idx = (SENSOR_2L4_PAGE_LENGTH * SENSOR_2L4_FLL_LSB_PAGE) + SENSOR_2L4_FLL_LSB_OFFSET+4;
static u32 frame_count_idx = (SENSOR_2L4_PAGE_LENGTH * SENSOR_2L4_FRAME_COUNT_PAGE) + SENSOR_2L4_FRAME_COUNT_OFFSET;

static int sensor_2l4_cis_get_frame_id(struct v4l2_subdev *subdev, u8 *embedded_buf, u16 *frame_id)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (embedded_buf[frame_id_idx-1] == SENSOR_2L4_VALID_TAG) {
		*frame_id = embedded_buf[frame_id_idx];

		dbg_sensor(1, "%s - frame_count(%d)", __func__, embedded_buf[frame_count_idx]);
		dbg_sensor(1, "%s - frame_id(%d)", __func__, *frame_id);
		dbg_sensor(1, "%s - frame length line(0x%x)",
				__func__, ((embedded_buf[fll_msb_idx]<<8)|embedded_buf[fll_lsb_idx]));
		dbg_sensor(1, "%s - line length pclk(0x%x)",
				__func__, ((embedded_buf[llp_msb_idx]<<8)|embedded_buf[llp_lsb_idx]));

#ifdef USE_CAMERA_SSM_TEST
		if (embedded_buf[frame_count_idx] == 254) {
			switch (record_status) {
			case 0:
				fimc_is_sensor_write8(cis->client, 0x0A52, 0x01);
				record_status++;
				break;
			case 1:
				fimc_is_sensor_write8(cis->client, 0x0A54, 0x01);
				record_status++;
				break;
			case 5:
				fimc_is_sensor_write8(cis->client, 0x0A53, 0x01);
				record_status++;
				break;
			case 10:
				record_status = 0;
				break;
			default:
				record_status++;
				break;
			}
		}
#endif
	} else {
		err("%s : invalid valid tag(0x%x)", __func__, embedded_buf[frame_id_idx-1]);
		*frame_id = 1;
	}

	return ret;
}
#endif

static void sensor_2l4_set_integration_max_margin(u32 mode, cis_shared_data *cis_data)
{
	WARN_ON(!cis_data);

	switch (mode) {
	case SENSOR_2L4_4032X3024_30FPS:
	case SENSOR_2L4_4032X2268_60FPS:
	case SENSOR_2L4_4032X2268_30FPS:
	case SENSOR_2L4_4032X1908_30FPS:
	case SENSOR_2L4_3024X3024_30FPS:
	case SENSOR_2L4_4032X3024_24FPS:
	case SENSOR_2L4_4032X2268_24FPS:
	case SENSOR_2L4_4032X1908_24FPS:
	case SENSOR_2L4_3024X3024_24FPS:
		cis_data->max_margin_coarse_integration_time = 0x18;
		break;
	case SENSOR_2L4_2016X1512_30FPS:
	case SENSOR_2L4_2016X1134_30FPS:
	case SENSOR_2L4_1504X1504_30FPS:
	case SENSOR_2L4_2016X1134_240FPS_MODE2:
	case SENSOR_2L4_1008X756_120FPS_MODE2:
	case SENSOR_2L4_2016X1134_60FPS_MODE2_SSM_960:
	case SENSOR_2L4_2016X1134_60FPS_MODE2_SSM_480:
	case SENSOR_2L4_1280X720_60FPS_MODE2_SSM_960:
	case SENSOR_2L4_1280X720_60FPS_MODE2_SSM_480:
	case SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1:
	case SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2:
		cis_data->max_margin_coarse_integration_time = SENSOR_2L4_COARSE_INTEGRATION_TIME_MAX_MARGIN;
		break;
	default:
		err("[%s] Unsupport 2l4 sensor mode\n", __func__);
		cis_data->max_margin_coarse_integration_time = SENSOR_2L4_COARSE_INTEGRATION_TIME_MAX_MARGIN;
		break;
	}

	dbg_sensor(1, "max_margin_coarse_integration_time(%d)\n", cis_data->max_margin_coarse_integration_time);
}

static void sensor_2l4_cis_data_calculation(const struct sensor_pll_info_compact *pll_info_compact,
						cis_shared_data *cis_data)
{
	u32 vt_pix_clk_hz = 0;
	u32 frame_rate = 0, max_fps = 0, frame_valid_us = 0;

	WARN_ON(!pll_info_compact);

	/* 1. get pclk value from pll info */
	vt_pix_clk_hz = pll_info_compact->pclk;

	dbg_sensor(1, "ext_clock(%d), mipi_datarate(%d), pclk(%d)\n",
			pll_info_compact->ext_clk, pll_info_compact->mipi_datarate, pll_info_compact->pclk);

	/* 2. the time of processing one frame calculation (us) */
	cis_data->min_frame_us_time = (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck
					/ (vt_pix_clk_hz / (1000 * 1000)));
	cis_data->cur_frame_us_time = cis_data->min_frame_us_time;
#ifdef CAMERA_REAR2
	cis_data->min_sync_frame_us_time = cis_data->min_frame_us_time;
#endif
	/* 3. FPS calculation */
	frame_rate = vt_pix_clk_hz / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	dbg_sensor(1, "frame_rate (%d) = vt_pix_clk_hz(%d) / "
		KERN_CONT "(pll_info_compact->frame_length_lines(%d) * pll_info_compact->line_length_pck(%d))\n",
		frame_rate, vt_pix_clk_hz, pll_info_compact->frame_length_lines, pll_info_compact->line_length_pck);

	/* calculate max fps */
	max_fps = (vt_pix_clk_hz * 10) / (pll_info_compact->frame_length_lines * pll_info_compact->line_length_pck);
	max_fps = (max_fps % 10 >= 5 ? frame_rate + 1 : frame_rate);

	cis_data->pclk = vt_pix_clk_hz;
	cis_data->max_fps = max_fps;
	cis_data->frame_length_lines = pll_info_compact->frame_length_lines;
	cis_data->line_length_pck = pll_info_compact->line_length_pck;
	cis_data->line_readOut_time = sensor_cis_do_div64((u64)cis_data->line_length_pck *
				(u64)(1000 * 1000 * 1000), cis_data->pclk);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;
	cis_data->stream_on = false;

	/* Frame valid time calcuration */
	frame_valid_us = sensor_cis_do_div64((u64)cis_data->cur_height *
				(u64)cis_data->line_length_pck * (u64)(1000 * 1000), cis_data->pclk);
	cis_data->frame_valid_us_time = (int)frame_valid_us;

	dbg_sensor(1, "%s\n", __func__);
	dbg_sensor(1, "Sensor size(%d x %d) setting: SUCCESS!\n",
					cis_data->cur_width, cis_data->cur_height);
	dbg_sensor(1, "Frame Valid(us): %d\n", frame_valid_us);
	dbg_sensor(1, "rolling_shutter_skew: %lld\n", cis_data->rolling_shutter_skew);

	dbg_sensor(1, "Fps: %d, max fps(%d)\n", frame_rate, cis_data->max_fps);
	dbg_sensor(1, "min_frame_time(%d us)\n", cis_data->min_frame_us_time);
	dbg_sensor(1, "Pixel rate(Mbps): %d\n", cis_data->pclk / 1000000);

	/* Frame period calculation */
	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis_data->rolling_shutter_skew = (cis_data->cur_height - 1) * cis_data->line_readOut_time;

	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n", __func__,
		cis_data->frame_time, cis_data->rolling_shutter_skew);

	/* Constant values */
	cis_data->min_fine_integration_time = SENSOR_2L4_FINE_INTEGRATION_TIME_MIN;
	cis_data->max_fine_integration_time = SENSOR_2L4_FINE_INTEGRATION_TIME_MAX;
	cis_data->min_coarse_integration_time = SENSOR_2L4_COARSE_INTEGRATION_TIME_MIN;
}

void sensor_2l4_cis_data_calc(struct v4l2_subdev *subdev, u32 mode)
{
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (mode >= sensor_2l4_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		return;
	}


	if (cis->cis_data->stream_on) {
		info("[%s] call mode change in stream on state\n", __func__);
		sensor_cis_wait_streamon(subdev);
		sensor_2l4_cis_stream_off(subdev);
		sensor_cis_wait_streamoff(subdev);
		info("[%s] stream off done\n", __func__);
	}

	sensor_2l4_cis_data_calculation(sensor_2l4_pllinfos[mode], cis->cis_data);
}

static int sensor_2l4_wait_stream_off_status(cis_shared_data *cis_data)
{
	int ret = 0;
	u32 timeout = 0;

	WARN_ON(!cis_data);

#define STREAM_OFF_WAIT_TIME 250
	while (timeout < STREAM_OFF_WAIT_TIME) {
		if (cis_data->is_active_area == false &&
				cis_data->stream_on == false) {
			pr_debug("actual stream off\n");
			break;
		}
		timeout++;
	}

	if (timeout == STREAM_OFF_WAIT_TIME) {
		pr_err("actual stream off wait timeout\n");
		ret = -1;
	}

	return ret;
}

int sensor_2l4_cis_select_setfile(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u16 rev = 0;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	rev = cis->cis_data->cis_rev;

	switch (rev) {
	case 0xA002: /* 3stack */
	case 0xA102: /* 3stack SHBN */
		pr_info("%s 3 stack sensor revision\n", __func__);
#ifndef USE_FLIP_WIDE_SENSOR
		sensor_2l4_reset_tnp = sensor_2l4_setfile_A_Reset_TnP;
		sensor_2l4_reset_tnp_size  = ARRAY_SIZE(sensor_2l4_setfile_A_Reset_TnP);
		sensor_2l4_global = sensor_2l4_setfile_A_Global;
		sensor_2l4_global_size = ARRAY_SIZE(sensor_2l4_setfile_A_Global);
		sensor_2l4_dram_test_global = sensor_2l4_setfile_A_dram_test_Global;
		sensor_2l4_dram_test_global_size = ARRAY_SIZE(sensor_2l4_setfile_A_dram_test_Global);
		sensor_2l4_setfiles = sensor_2l4_setfiles_A;
		sensor_2l4_setfile_sizes = sensor_2l4_setfile_A_sizes;
		sensor_2l4_pllinfos = sensor_2l4_pllinfos_A;
		sensor_2l4_max_setfile_num = ARRAY_SIZE(sensor_2l4_setfiles_A);
#ifdef CONFIG_SENSOR_RETENTION_USE
		sensor_2l4_global_retention = sensor_2l4_setfile_A_Global_retention;
		sensor_2l4_global_retention_size = ARRAY_SIZE(sensor_2l4_setfile_A_Global_retention);
		sensor_2l4_retention = sensor_2l4_setfiles_A_retention;
		sensor_2l4_retention_size = sensor_2l4_setfile_A_sizes_retention;
		sensor_2l4_max_retention_num = ARRAY_SIZE(sensor_2l4_setfiles_A_retention);
		sensor_2l4_load_sram = sensor_2l4_setfile_A_load_sram;
		sensor_2l4_load_sram_size = sensor_2l4_setfile_A_sizes_load_sram;
#endif
#else
		sensor_2l4_reset_tnp = sensor_2l4_setfile_B_Reset_TnP;
		sensor_2l4_reset_tnp_size  = ARRAY_SIZE(sensor_2l4_setfile_B_Reset_TnP);
		sensor_2l4_global = sensor_2l4_setfile_B_Global;
		sensor_2l4_global_size = ARRAY_SIZE(sensor_2l4_setfile_B_Global);
		sensor_2l4_dram_test_global = sensor_2l4_setfile_B_dram_test_Global;
		sensor_2l4_dram_test_global_size = ARRAY_SIZE(sensor_2l4_setfile_B_dram_test_Global);
		sensor_2l4_setfiles = sensor_2l4_setfiles_B;
		sensor_2l4_setfile_sizes = sensor_2l4_setfile_B_sizes;
		sensor_2l4_pllinfos = sensor_2l4_pllinfos_B;
		sensor_2l4_max_setfile_num = ARRAY_SIZE(sensor_2l4_setfiles_B);
#ifdef CONFIG_SENSOR_RETENTION_USE
		sensor_2l4_global_retention = sensor_2l4_setfile_B_Global_retention;
		sensor_2l4_global_retention_size = ARRAY_SIZE(sensor_2l4_setfile_B_Global_retention);
		sensor_2l4_retention = sensor_2l4_setfiles_B_retention;
		sensor_2l4_retention_size = sensor_2l4_setfile_B_sizes_retention;
		sensor_2l4_max_retention_num = ARRAY_SIZE(sensor_2l4_setfiles_B_retention);
		sensor_2l4_load_sram = sensor_2l4_setfile_B_load_sram;
		sensor_2l4_load_sram_size = sensor_2l4_setfile_B_sizes_load_sram;
#endif
#endif
		break;
	default:
		err("Unsupported 2l4 sensor revision(%#x)\n", rev);
		break;
	}

	return ret;
}

int sensor_2l4_cis_set_global_setting_internal(struct v4l2_subdev *subdev);

/* CIS OPS */
int sensor_2l4_cis_init(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	u32 setfile_index = 0;
	cis_setting_info setinfo;
	setinfo.param = NULL;
	setinfo.return_value = 0;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	WARN_ON(!cis->cis_data);
#if !defined(CONFIG_VENDER_MCD)
	memset(cis->cis_data, 0, sizeof(cis_shared_data));

	ret = sensor_cis_check_rev(cis);
	if (ret < 0) {
		warn("sensor_2l4_check_rev is fail when cis init");
		ret = -EINVAL;
		goto p_err;
	}
#endif

	sensor_2l4_cis_select_setfile(subdev);

	cis->cis_data->stream_on = false;
	cis->cis_data->product_name = cis->id;
	cis->cis_data->cur_width = SENSOR_2L4_MAX_WIDTH;
	cis->cis_data->cur_height = SENSOR_2L4_MAX_HEIGHT;
	cis->cis_data->low_expo_start = 33000;
	cis->cis_data->pre_lownoise_mode = FIMC_IS_CIS_LNOFF;
	cis->cis_data->cur_lownoise_mode = FIMC_IS_CIS_LNOFF;
	cis->need_mode_change = false;
	cis->long_term_mode.sen_strm_off_on_step = 0;
	cis->long_term_mode.sen_strm_off_on_enable = false;
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
	cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
#endif

	sensor_2l4_load_retention = false;
	sensor_2l4_cis_data_calculation(sensor_2l4_pllinfos[setfile_index], cis->cis_data);
	sensor_2l4_set_integration_max_margin(setfile_index, cis->cis_data);

	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_exposure_time, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max exposure time : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_analog_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max again : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_min_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] min dgain : %d\n", __func__, setinfo.return_value);
	setinfo.return_value = 0;
	CALL_CISOPS(cis, cis_get_max_digital_gain, subdev, &setinfo.return_value);
	dbg_sensor(1, "[%s] max dgain : %d\n", __func__, setinfo.return_value);

	/* CALL_CISOPS(cis, cis_log_status, subdev); */

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif
p_err:
	return ret;
}

int sensor_2l4_cis_deinit(struct v4l2_subdev *subdev)
{
	int ret = 0;

	if (sensor_2l4_load_retention == false) {
		pr_info("%s: need to load retention\n", __func__);
		sensor_2l4_cis_stream_on(subdev);
		sensor_cis_wait_streamon(subdev);
		sensor_2l4_cis_stream_off(subdev);
		sensor_cis_wait_streamoff(subdev);
		pr_info("%s: complete to load retention\n", __func__);
	}

	return ret;
}

int sensor_2l4_cis_log_status(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client = NULL;
	u8 data8 = 0;
	u16 data16 = 0;
#ifdef CONFIG_SENSOR_RETENTION_USE
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;
#endif

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -ENODEV;
		goto p_err;
	}

#ifdef CONFIG_SENSOR_RETENTION_USE
	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	FIMC_BUG(!ext_info);

	ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
#endif

	I2C_MUTEX_LOCK(cis->i2c_lock);

	pr_info("[%s] *******************************\n", __func__);
	ret = fimc_is_sensor_write16(client, 0xFCFC, 0x2000);
	if (unlikely(!ret)) pr_info("0x2000 page\n");
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0xEA2A, &data16);
	if (unlikely(!ret)) pr_info("0xEA2A(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0xEA2C, &data16);
	if (unlikely(!ret)) pr_info("0xEA2C(0x%x)\n", data16);
	else goto i2c_err;

	ret = fimc_is_sensor_write16(client, 0xFCFC, 0x4000);
	if (unlikely(!ret)) pr_info("0x4000 page\n");
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0000, &data16);
	if (unlikely(!ret)) pr_info("model_id(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0002, &data16);
	if (unlikely(!ret)) pr_info("revision_number(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read8(client, 0x0005, &data8);
	if (unlikely(!ret)) pr_info("frame_count(0x%x)\n", data8);
	else goto i2c_err;
	ret = fimc_is_sensor_read8(client, 0x0100, &data8);
	if (unlikely(!ret)) pr_info("0x0100(0x%x)\n", data8);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0136, &data16);
	if (unlikely(!ret)) pr_info("0x0136(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0202, &data16);
	if (unlikely(!ret)) pr_info("0x0202(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0204, &data16);
	if (unlikely(!ret)) pr_info("0x0204(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x020E, &data16);
	if (unlikely(!ret)) pr_info("0x020E(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x021E, &data16);
	if (unlikely(!ret)) pr_info("0x021E(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0226, &data16);
	if (unlikely(!ret)) pr_info("0x0226(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0340, &data16);
	if (unlikely(!ret)) pr_info("0x0340(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0700, &data16);
	if (unlikely(!ret)) pr_info("0x0700(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0702, &data16);
	if (unlikely(!ret)) pr_info("0x0702(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0A70, &data16);
	if (unlikely(!ret)) pr_info("0x0A70(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0A72, &data16);
	if (unlikely(!ret)) pr_info("0x0A72(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0A78, &data16);
	if (unlikely(!ret)) pr_info("0x0A78(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0A7A, &data16);
	if (unlikely(!ret)) pr_info("0x0A7A(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0A7C, &data16);
	if (unlikely(!ret)) pr_info("0x0A7C(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read16(client, 0x0C80, &data16);
	if (unlikely(!ret)) pr_info("0x0C80(0x%x)\n", data16);
	else goto i2c_err;
	ret = fimc_is_sensor_read8(client, 0x3000, &data8);
	if (unlikely(!ret)) pr_info("0x3000(0x%x)\n", data8);
	else goto i2c_err;
	pr_info("[%s] *******************************\n", __func__);

i2c_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
p_err:
	return ret;
}

#if USE_GROUP_PARAM_HOLD
static int sensor_2l4_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct i2c_client *client = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (hold == cis->cis_data->group_param_hold) {
		pr_debug("already group_param_hold (%d)\n", cis->cis_data->group_param_hold);
		goto p_err;
	}

	ret = fimc_is_sensor_write8(client, 0x0104, hold);
	if (ret < 0)
		goto p_err;

	cis->cis_data->group_param_hold = hold;
	ret = 1;
p_err:
	return ret;
}
#else
static inline int sensor_2l4_cis_group_param_hold_func(struct v4l2_subdev *subdev, unsigned int hold)
{ return 0; }
#endif

/* Input
 *	hold : true - hold, flase - no hold
 * Output
 *      return: 0 - no effect(already hold or no hold)
 *		positive - setted by request
 *		negative - ERROR value
 */
int sensor_2l4_cis_group_param_hold(struct v4l2_subdev *subdev, bool hold)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = sensor_2l4_cis_group_param_hold_func(subdev, hold);
	if (ret < 0)
		goto p_err;

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_2l4_cis_set_global_setting_dramtest(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	info("[%s] sensor_2l4_dram_test_global start\n", __func__);
	ret = sensor_cis_set_registers(subdev, sensor_2l4_dram_test_global, sensor_2l4_dram_test_global_size);

	if (ret < 0) {
		err("sensor_2l4_set_registers fail!!");
		goto p_err;
	}

	info("[%s] sensor_2l4_dram_test_global done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_2l4_cis_set_global_setting_internal(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	info("[%s] global setting start\n", __func__);
	/* setfile global setting is at camera entrance */
	ret |= sensor_cis_set_registers(subdev, sensor_2l4_reset_tnp, sensor_2l4_reset_tnp_size);
	ret |= sensor_cis_set_registers(subdev, sensor_2l4_global, sensor_2l4_global_size);
	if (ret < 0) {
		err("sensor_2l4_set_registers fail!!");
		goto p_err;
	}

	info("[%s] global setting done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

#ifdef CONFIG_SENSOR_RETENTION_USE
int sensor_2l4_cis_set_global_setting_retention(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	info("[%s] global retention setting start\n", __func__);
	/* setfile global retention setting is at camera entrance */
	ret = sensor_cis_set_registers(subdev, sensor_2l4_global_retention, sensor_2l4_global_retention_size);
	if (ret < 0) {
		err("sensor_2l4_set_registers fail!!");
		goto p_err;
	}

	info("[%s] global retention setting done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

#ifdef CAMERA_REAR2
int sensor_2l4_cis_retention_crc_enable(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct i2c_client *client;

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	switch (mode) {
	case SENSOR_2L4_1008X756_120FPS_MODE2:
	case SENSOR_2L4_2016X1134_60FPS_MODE2_SSM_960:
	case SENSOR_2L4_2016X1134_60FPS_MODE2_SSM_480:
	case SENSOR_2L4_1280X720_60FPS_MODE2_SSM_960:
	case SENSOR_2L4_1280X720_60FPS_MODE2_SSM_480:
		break;
	default:
		/* Sensor stream on */
#ifdef USE_FLIP_WIDE_SENSOR
		info("%s 180 degrees rotated\n", __func__);
		fimc_is_sensor_write16(client, 0x0100, 0x0100);
#else
		fimc_is_sensor_write16(client, 0x0100, 0x0103);
#endif
		/* retention mode CRC check register enable */
		fimc_is_sensor_write8(client, 0x010E, 0x01);
		info("[MOD:D:%d] %s : retention enable CRC check\n", cis->id, __func__);

		/* Sensor stream off */
		fimc_is_sensor_write8(client, 0x0100, 0x00);
		break;
	}

p_err:
	return ret;
}
#endif
#endif

static void sensor_2l4_cis_set_paf_stat_enable(u32 mode, cis_shared_data *cis_data)
{
	WARN_ON(!cis_data);

	switch (mode) {
	case SENSOR_2L4_4032X3024_30FPS:
	case SENSOR_2L4_4032X2268_60FPS:
	case SENSOR_2L4_4032X2268_30FPS:
	case SENSOR_2L4_4032X1908_30FPS:
	case SENSOR_2L4_3024X3024_30FPS:
	case SENSOR_2L4_2016X1512_30FPS:
	case SENSOR_2L4_2016X1134_30FPS:
	case SENSOR_2L4_1504X1504_30FPS:
	case SENSOR_2L4_4032X3024_24FPS:
	case SENSOR_2L4_4032X2268_24FPS:
	case SENSOR_2L4_4032X1908_24FPS:
	case SENSOR_2L4_3024X3024_24FPS:
		cis_data->is_data.paf_stat_enable = true;
		break;
	default:
		cis_data->is_data.paf_stat_enable = false;
		break;
	}
}

bool sensor_2l4_cis_get_lownoise_supported(cis_shared_data *cis_data)
{
	WARN_ON(!cis_data);

	if (cis_data->cis_rev >= 0xA002) { // support ln mode higher than 3 stack
		switch (cis_data->sens_config_index_cur) {
		case SENSOR_2L4_4032X3024_30FPS:
		case SENSOR_2L4_4032X2268_30FPS:
		case SENSOR_2L4_4032X1908_30FPS:
		case SENSOR_2L4_3024X3024_30FPS:
		case SENSOR_2L4_4032X3024_24FPS:
		case SENSOR_2L4_4032X2268_24FPS:
		case SENSOR_2L4_4032X1908_24FPS:
		case SENSOR_2L4_3024X3024_24FPS:
			return true;
		default:
			break;
		}
	}

	return false;
}

int sensor_2l4_cis_mode_change(struct v4l2_subdev *subdev, u32 mode)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	if (mode >= sensor_2l4_max_setfile_num) {
		err("invalid mode(%d)!!", mode);
		ret = -EINVAL;
		goto p_err;
	}

	sensor_2l4_ln_mode_delay_count = 0;

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

#if 0 /* cis_data_calculation is called in module_s_format */
	sensor_2l4_cis_data_calculation(sensor_2l4_pllinfos[mode], cis->cis_data);
#endif
	sensor_2l4_set_integration_max_margin(mode, cis->cis_data);

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
#endif

	if (mode == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1) {
		sensor_2l4_cis_set_global_setting_dramtest(subdev);
	}

	sensor_2l4_cis_set_paf_stat_enable(mode, cis->cis_data);

	I2C_MUTEX_LOCK(cis->i2c_lock);

#ifdef CONFIG_SENSOR_RETENTION_USE
	info("[%s] cancel retention mode\n", __func__);
	fimc_is_sensor_write16(cis->client, 0x602A, 0x1090);
	fimc_is_sensor_write16(cis->client, 0x6F12, 0x0000);
	fimc_is_sensor_write16(cis->client, 0x6F12, 0x0000);

	/* Retention mode sensor mode select */
	if (ext_info->use_retention_mode == SENSOR_RETENTION_ACTIVATED) {
		sensor_2l4_load_retention = false;

		switch (mode) {
		case SENSOR_2L4_4032X3024_30FPS:
			info("[%s] retention mode: SENSOR_2L4_4032X3024_30FPS\n", __func__);
			ret = sensor_cis_set_registers(subdev,
				sensor_2l4_load_sram[SENSOR_2L4_4032x3024_30FPS_LOAD_SRAM],
				sensor_2l4_load_sram_size[SENSOR_2L4_4032x3024_30FPS_LOAD_SRAM]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}
			break;
		case SENSOR_2L4_4032X2268_30FPS:
			info("[%s] retention mode: SENSOR_2L4_4032X2268_30FPS\n", __func__);
			ret = sensor_cis_set_registers(subdev,
				sensor_2l4_load_sram[SENSOR_2L4_4032x2268_30FPS_LOAD_SRAM],
				sensor_2l4_load_sram_size[SENSOR_2L4_4032x2268_30FPS_LOAD_SRAM]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}
			break;
		case SENSOR_2L4_4032X2268_60FPS:
			info("[%s] retention mode: SENSOR_2L4_4032X2268_60FPS\n", __func__);
			ret = sensor_cis_set_registers(subdev,
				sensor_2l4_load_sram[SENSOR_2L4_4032x2268_60FPS_LOAD_SRAM],
				sensor_2l4_load_sram_size[SENSOR_2L4_4032x2268_60FPS_LOAD_SRAM]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}
			break;
		case SENSOR_2L4_1008X756_120FPS_MODE2:
			info("[%s] retention mode: SENSOR_2L4_1008X756_120FPS_MODE2\n", __func__);
			ret = sensor_cis_set_registers(subdev,
				sensor_2l4_load_sram[SENSOR_2L4_1008x756_120FPS_LOAD_SRAM],
				sensor_2l4_load_sram_size[SENSOR_2L4_1008x756_120FPS_LOAD_SRAM]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}
			break;
		case SENSOR_2L4_4032X3024_24FPS:
			info("[%s] retention mode: SENSOR_2L4_4032X3024_24FPS\n", __func__);
			ret = sensor_cis_set_registers(subdev,
				sensor_2l4_load_sram[SENSOR_2L4_4032x3024_24FPS_LOAD_SRAM],
				sensor_2l4_load_sram_size[SENSOR_2L4_4032x3024_24FPS_LOAD_SRAM]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}
			break;
		case SENSOR_2L4_4032X2268_24FPS:
			info("[%s] retention mode: SENSOR_2L4_4032X2268_24FPS\n", __func__);
			ret = sensor_cis_set_registers(subdev,
				sensor_2l4_load_sram[SENSOR_2L4_4032x2268_24FPS_LOAD_SRAM],
				sensor_2l4_load_sram_size[SENSOR_2L4_4032x2268_24FPS_LOAD_SRAM]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}
			break;
		default:
			info("[%s] not support retention sensor mode(%d)\n", __func__, mode);
			ret = sensor_cis_set_registers(subdev, sensor_2l4_setfiles[mode],
								sensor_2l4_setfile_sizes[mode]);
			if (ret < 0) {
				err("sensor_2l4_set_registers fail!!");
				goto p_err_i2c_unlock;
			}

			if (mode == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1)
				ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
			break;
		}
	} else
#endif
	{
		info("[%s] sensor mode(%d)\n", __func__, mode);
		ret = sensor_cis_set_registers(subdev, sensor_2l4_setfiles[mode],
								sensor_2l4_setfile_sizes[mode]);
		if (ret < 0) {
			err("sensor_2l4_set_registers fail!!");
			goto p_err_i2c_unlock;
		}
	}

	if (sensor_2l4_cis_get_lownoise_supported(cis->cis_data)) {
		cis->cis_data->pre_lownoise_mode = FIMC_IS_CIS_LN2;
		cis->cis_data->cur_lownoise_mode = FIMC_IS_CIS_LN2;
		sensor_2l4_cis_set_lownoise_mode_change(subdev);
	} else {
		cis->cis_data->pre_lownoise_mode = FIMC_IS_CIS_LNOFF;
		cis->cis_data->cur_lownoise_mode = FIMC_IS_CIS_LNOFF;
	}

	info("[%s] mode changed(%d)\n", __func__, mode);

p_err_i2c_unlock:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

p_err:
	/* sensor_2l4_cis_log_status(subdev); */

	return ret;
}

int sensor_2l4_cis_set_lownoise_mode_change(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	unsigned int mode = 0;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	mode = cis->cis_data->sens_config_index_cur;

	if (!sensor_2l4_cis_get_lownoise_supported(cis->cis_data)) {
		pr_info("[%s] not support mode %d evt %x\n", __func__,
			mode, cis->cis_data->cis_rev);
		cis->cis_data->pre_lownoise_mode = cis->cis_data->cur_lownoise_mode;
		return ret;
	}

	fimc_is_sensor_read8(cis->client, 0x0005, &sensor_2l4_ln_mode_frame_count);
	pr_info("[%s] lownoise mode changed(%d) cur_mode(%d) sensor_2l4_ln_mode_frame_count(0x%x)\n",
		__func__, cis->cis_data->cur_lownoise_mode, mode, sensor_2l4_ln_mode_frame_count);

	sensor_2l4_ln_mode_delay_count = 3;

	ret = fimc_is_sensor_write16(cis->client, 0x0104, 0x0101);

	switch (cis->cis_data->cur_lownoise_mode) {
	case FIMC_IS_CIS_LNOFF:
		dbg_sensor(1, "[%s] FIMC_IS_CIS_LNOFF\n", __func__);
		ret |= fimc_is_sensor_write16(cis->client, 0x6028, 0x2000);
		ret |= fimc_is_sensor_write16(cis->client, 0x602A, 0xFF22);
		ret |= fimc_is_sensor_write16(cis->client, 0x6F12, 0x0001);
		ret |= fimc_is_sensor_write16(cis->client, 0x3012, 0x0101);

#ifdef CAMERA_REAR2
		switch (mode) {
		case SENSOR_2L4_4032X3024_30FPS:
		case SENSOR_2L4_4032X2268_30FPS:
		case SENSOR_2L4_4032X1908_30FPS:
		case SENSOR_2L4_3024X3024_30FPS:
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7A, 0x0FE0);
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7C, 0x00D0);
			break;
		case SENSOR_2L4_4032X3024_24FPS:
		case SENSOR_2L4_4032X2268_24FPS:
		case SENSOR_2L4_4032X1908_24FPS:
		case SENSOR_2L4_3024X3024_24FPS:
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7A, 0x1660);
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7C, 0x00B0);
			break;
		}
#endif
		cis->cis_data->max_margin_coarse_integration_time = 24; /* 0x18 */
		break;
	case FIMC_IS_CIS_LN2:
		dbg_sensor(1, "[%s] FIMC_IS_CIS_LN2\n", __func__);
		ret |= fimc_is_sensor_write16(cis->client, 0x6028, 0x2000);
		ret |= fimc_is_sensor_write16(cis->client, 0x602A, 0xFF22);
		ret |= fimc_is_sensor_write16(cis->client, 0x6F12, 0x0001);
		ret |= fimc_is_sensor_write16(cis->client, 0x3012, 0x0201);

#ifdef CAMERA_REAR2
		switch (mode) {
		case SENSOR_2L4_4032X3024_30FPS:
		case SENSOR_2L4_4032X2268_30FPS:
		case SENSOR_2L4_4032X1908_30FPS:
		case SENSOR_2L4_3024X3024_30FPS:
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7A, 0x0000);
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7C, 0x00D0);
			break;
		case SENSOR_2L4_4032X3024_24FPS:
		case SENSOR_2L4_4032X2268_24FPS:
		case SENSOR_2L4_4032X1908_24FPS:
		case SENSOR_2L4_3024X3024_24FPS:
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7A, 0x0000);
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7C, 0x00B0);
			break;
		}
#endif
		cis->cis_data->max_margin_coarse_integration_time = 48; /* 0x30 */
		break;
	case FIMC_IS_CIS_LN4:
		dbg_sensor(1, "[%s] FIMC_IS_CIS_LN4\n", __func__);
		ret |= fimc_is_sensor_write16(cis->client, 0x6028, 0x2000);
		ret |= fimc_is_sensor_write16(cis->client, 0x602A, 0xFF22);
		ret |= fimc_is_sensor_write16(cis->client, 0x6F12, 0x0001);
		ret |= fimc_is_sensor_write16(cis->client, 0x3012, 0x0401);

#ifdef CAMERA_REAR2
		switch (mode) {
		case SENSOR_2L4_4032X3024_30FPS:
		case SENSOR_2L4_4032X2268_30FPS:
		case SENSOR_2L4_4032X1908_30FPS:
		case SENSOR_2L4_3024X3024_30FPS:
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7A, 0x0000);
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7C, 0x00D0);
			break;
		case SENSOR_2L4_4032X3024_24FPS:
		case SENSOR_2L4_4032X2268_24FPS:
		case SENSOR_2L4_4032X1908_24FPS:
		case SENSOR_2L4_3024X3024_24FPS:
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7A, 0x0000);
			ret |= fimc_is_sensor_write16(cis->client, 0x0A7C, 0x00B0);
			break;
		}
#endif
		cis->cis_data->max_margin_coarse_integration_time = 96; /* 0x60 */
		break;
	case FIMC_IS_CIS_LN2_PEDESTAL128:
		dbg_sensor(1, "[%s] FIMC_IS_CIS_LN2_PEDESTAL128\n", __func__);
		ret |= fimc_is_sensor_write16(cis->client, 0x0B30, 0x0200);
		ret |= fimc_is_sensor_write16(cis->client, 0x0BC0, 0x0080); /* pedestal 128 */

#ifdef CAMERA_REAR2
		ret |= fimc_is_sensor_write16(cis->client, 0x30D0, 0x0000);
		ret |= fimc_is_sensor_write16(cis->client, 0x30D2, 0x0500);
#endif
		ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
		break;
	case FIMC_IS_CIS_LN4_PEDESTAL128:
		dbg_sensor(1, "[%s] FIMC_IS_CIS_LN4_PEDESTAL128\n", __func__);
		ret |= fimc_is_sensor_write16(cis->client, 0x0B30, 0x0300);
		ret |= fimc_is_sensor_write16(cis->client, 0x0BC0, 0x0080); /* pedestal 128 */

#ifdef CAMERA_REAR2
		ret |= fimc_is_sensor_write16(cis->client, 0x30D0, 0x0000);
		ret |= fimc_is_sensor_write16(cis->client, 0x30D2, 0x0500);
#endif
		ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
		break;
	default:
		dbg_sensor(1, "[%s] not support lownoise mode(%d)\n",
				__func__, cis->cis_data->cur_lownoise_mode);
	}

	pr_info("[%s] max_margin_coarse_integration_time(%d)\n", __func__,
		cis->cis_data->max_margin_coarse_integration_time);

	ret |= fimc_is_sensor_write16(cis->client, 0x0104, 0x0001);

	if (ret < 0) {
		err("sensor_2l4_set_registers fail!!");
		goto p_err;
	}

	cis->cis_data->pre_lownoise_mode = cis->cis_data->cur_lownoise_mode;

p_err:
	return ret;
}

int sensor_2l4_cis_set_global_setting(struct v4l2_subdev *subdev)
{
	int ret = 0;
#ifdef CONFIG_SENSOR_RETENTION_USE
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	/* setfile global setting is at camera entrance */
	if (ext_info->use_retention_mode == SENSOR_RETENTION_INACTIVE) {
		sensor_2l4_cis_set_global_setting_internal(subdev);
		sensor_2l4_cis_retention_prepare(subdev);
		ext_info->use_retention_mode = SENSOR_RETENTION_ACTIVATED;
	} else if (ext_info->use_retention_mode == SENSOR_RETENTION_ACTIVATED) {
		sensor_2l4_cis_retention_crc_check(subdev);
	} else { /* SENSOR_RETENTION_UNSUPPORTED */
		sensor_2l4_cis_set_global_setting_internal(subdev);
	}
#else
	WARN_ON(!subdev);
	sensor_2l4_cis_set_global_setting_internal(subdev);
#endif

	return ret;
}

#ifdef CONFIG_SENSOR_RETENTION_USE
int sensor_2l4_cis_retention_prepare(struct v4l2_subdev *subdev)
{
	int ret = 0;
	int i = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	for (i = 0; i < sensor_2l4_max_retention_num; i++) {
		ret = sensor_cis_set_registers(subdev, sensor_2l4_retention[i], sensor_2l4_retention_size[i]);
		if (ret < 0) {
			err("sensor_2l4_set_registers fail!!");
			goto p_err;
		}
	}

	info("[%s] retention sensor RAM write done\n", __func__);

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_retention_crc_check(struct v4l2_subdev *subdev)
{
	int ret = 0;
	u8 crc_check = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	I2C_MUTEX_LOCK(cis->i2c_lock);

	/* retention mode CRC check */
	fimc_is_sensor_read8(cis->client, 0x100E, &crc_check);

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	if (crc_check == 0x01) {
		info("[%s] retention SRAM CRC check: pass!\n", __func__);

		ret = sensor_2l4_cis_set_global_setting_retention(subdev);
		if (ret < 0) {
			err("write retention global setting failed");
			goto p_err;
		}
	} else {
		info("[%s] retention SRAM CRC check: fail!\n", __func__);
		info("retention CRC Check register value: 0x%x\n", crc_check);
		info("[%s] rewrite retention modes to SRAM\n", __func__);

		ret = sensor_2l4_cis_set_global_setting_internal(subdev);
		if (ret < 0) {
			err("CRC error recover: rewrite sensor global setting failed");
			goto p_err;
		}

		ret = sensor_2l4_cis_retention_prepare(subdev);
		if (ret < 0) {
			err("CRC error recover: retention prepare failed");
			goto p_err;
		}
	}

p_err:

	return ret;
}
#endif

/* TODO: Sensor set size sequence(sensor done, sensor stop, 3AA done in FW case */
int sensor_2l4_cis_set_size(struct v4l2_subdev *subdev, cis_shared_data *cis_data)
{
	int ret = 0;
	bool binning = false;
	u32 ratio_w = 0, ratio_h = 0, start_x = 0, start_y = 0, end_x = 0, end_y = 0;
	u32 even_x = 0, odd_x = 0, even_y = 0, odd_y = 0;
	struct i2c_client *client = NULL;
	struct fimc_is_cis *cis = NULL;
#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif
	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	if (unlikely(!cis_data)) {
		err("cis data is NULL");
		if (unlikely(!cis->cis_data)) {
			ret = -EINVAL;
			goto p_err;
		} else {
			cis_data = cis->cis_data;
		}
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* Wait actual stream off */
	ret = sensor_2l4_wait_stream_off_status(cis_data);
	if (ret) {
		err("Must stream off\n");
		ret = -EINVAL;
		goto p_err;
	}

	binning = cis_data->binning;
	if (binning) {
		ratio_w = (SENSOR_2L4_MAX_WIDTH / cis_data->cur_width);
		ratio_h = (SENSOR_2L4_MAX_HEIGHT / cis_data->cur_height);
	} else {
		ratio_w = 1;
		ratio_h = 1;
	}

	if (((cis_data->cur_width * ratio_w) > SENSOR_2L4_MAX_WIDTH) ||
		((cis_data->cur_height * ratio_h) > SENSOR_2L4_MAX_HEIGHT)) {
		err("Config max sensor size over~!!\n");
		ret = -EINVAL;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* 1. page_select */
	ret = fimc_is_sensor_write16(client, 0x6028, 0x2000);
	if (ret < 0)
		goto p_err;

	/* 2. pixel address region setting */
	start_x = ((SENSOR_2L4_MAX_WIDTH - cis_data->cur_width * ratio_w) / 2) & (~0x1);
	start_y = ((SENSOR_2L4_MAX_HEIGHT - cis_data->cur_height * ratio_h) / 2) & (~0x1);
	end_x = start_x + (cis_data->cur_width * ratio_w - 1);
	end_y = start_y + (cis_data->cur_height * ratio_h - 1);

	if (!(end_x & (0x1)) || !(end_y & (0x1))) {
		err("Sensor pixel end address must odd\n");
		ret = -EINVAL;
		goto p_err;
	}

	ret = fimc_is_sensor_write16(client, 0x0344, start_x);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x0346, start_y);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x0348, end_x);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x034A, end_y);
	if (ret < 0)
		goto p_err;

	/* 3. output address setting */
	ret = fimc_is_sensor_write16(client, 0x034C, cis_data->cur_width);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x034E, cis_data->cur_height);
	if (ret < 0)
		goto p_err;

	/* If not use to binning, sensor image should set only crop */
	if (!binning) {
		dbg_sensor(1, "Sensor size set is not binning\n");
		goto p_err;
	}

	/* 4. sub sampling setting */
	even_x = 1;	/* 1: not use to even sampling */
	even_y = 1;
	odd_x = (ratio_w * 2) - even_x;
	odd_y = (ratio_h * 2) - even_y;

	ret = fimc_is_sensor_write16(client, 0x0380, even_x);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x0382, odd_x);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x0384, even_y);
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write16(client, 0x0386, odd_y);
	if (ret < 0)
		goto p_err;

#if 0
	/* 5. binnig setting */
	ret = fimc_is_sensor_write8(client, 0x0900, binning);	/* 1:  binning enable, 0: disable */
	if (ret < 0)
		goto p_err;
	ret = fimc_is_sensor_write8(client, 0x0901, (ratio_w << 4) | ratio_h);
	if (ret < 0)
		goto p_err;
#endif

	/* 6. scaling setting: but not use */
	/* scaling_mode (0: No scaling, 1: Horizontal, 2: Full, 4:Separate vertical) */
	ret = fimc_is_sensor_write16(client, 0x0400, 0x0000);
	if (ret < 0)
		goto p_err;
	/* down_scale_m: 1 to 16 upwards (scale_n: 16(fixed))
	 * down scale factor = down_scale_m / down_scale_n
	 */
	ret = fimc_is_sensor_write16(client, 0x0404, 0x0010);
	if (ret < 0)
		goto p_err;

	cis_data->frame_time = (cis_data->line_readOut_time * cis_data->cur_height / 1000);
	cis->cis_data->rolling_shutter_skew = (cis->cis_data->cur_height - 1) * cis->cis_data->line_readOut_time;
	dbg_sensor(1, "[%s] frame_time(%d), rolling_shutter_skew(%lld)\n",
		__func__, cis->cis_data->frame_time, cis->cis_data->rolling_shutter_skew);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec) * 1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	I2C_MUTEX_UNLOCK(cis->i2c_lock);
	return ret;
}

int sensor_2l4_cis_wait_streamon(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	u32 wait_cnt = 0, time_out_cnt = 250;
	u8 sensor_fcount = 0;
	u8 data8 = 0;

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (unlikely(!cis)) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	if (unlikely(!cis_data)) {
		err("cis_data is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);

	ret = fimc_is_sensor_read8(client, 0x0005, &sensor_fcount);
	if (ret < 0)
		err("i2c transfer fail addr(0x%x), val(0x%x), ret = %d\n", 0x0005, sensor_fcount, ret);

	ret = fimc_is_sensor_read8(client, 0x3000, &data8);
	if (ret < 0)
		err("i2c transfer fail addr(0x%x), val(0x%x), ret = %d\n", 0x3000, data8, ret);

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	/*
	 * Read sensor frame counter (sensor_fcount address = 0x0005)
	 * stream on (0x00 ~ 0xFE), stream off (0xFF)
	 */
	while (sensor_fcount == 0xff || data8 != 0x0) {
		usleep_range(CIS_STREAM_ON_WAIT_TIME, CIS_STREAM_ON_WAIT_TIME);
		wait_cnt++;

		I2C_MUTEX_LOCK(cis->i2c_lock);

		ret = fimc_is_sensor_read8(client, 0x0005, &sensor_fcount);
		if (ret < 0)
			err("i2c transfer fail addr(0x%x), val(0x%x), ret = %d\n", 0x0005, sensor_fcount, ret);

		ret = fimc_is_sensor_read8(client, 0x3000, &data8);
		if (ret < 0)
			err("i2c transfer fail addr(0x%x), val(0x%x), ret = %d\n", 0x3000, data8, ret);

		I2C_MUTEX_UNLOCK(cis->i2c_lock);

		if (wait_cnt >= time_out_cnt) {
			err("[MOD:D:%d] %s, Don't sensor stream on and time out, wait_limit(%d) > time_out(%d), sensor_fcount(%d), 0x3000(%d)",
				cis->id, __func__, wait_cnt, time_out_cnt, sensor_fcount, data8);
			ret = -EINVAL;
			goto p_err;
		}

		dbg_sensor(1, "[MOD:D:%d] %s, sensor_fcount(%d), (wait_limit(%d) < time_out(%d))\n",
				cis->id, __func__, sensor_fcount, wait_cnt, time_out_cnt);
	}

#ifdef CONFIG_SENSOR_RETENTION_USE
	/* retention mode CRC wait calculation */
	usleep_range(1000, 1000);
#endif
p_err:
	return ret;
}


int sensor_2l4_cis_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;

#ifdef CAMERA_REAR2
	u32 mode;
#endif

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	WARN_ON(!sensor_peri);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	sensor_2l4_cis_set_mipi_clock(subdev);
#endif

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

#ifdef DEBUG_2L4_PLL
	{
		u16 pll;

		fimc_is_sensor_read16(client, 0x0300, &pll);
		dbg_sensor(1, "______ vt_pix_clk_div(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0302, &pll);
		dbg_sensor(1, "______ vt_sys_clk_div(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0304, &pll);
		dbg_sensor(1, "______ vt_pre_pll_clk_div(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0306, &pll);
		dbg_sensor(1, "______ vt_pll_multiplier(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0308, &pll);
		dbg_sensor(1, "______ op_pix_clk_div(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x030a, &pll);
		dbg_sensor(1, "______ op_sys_clk_div(0x%x)\n", pll);

		fimc_is_sensor_read16(client, 0x030c, &pll);
		dbg_sensor(1, "______ vt_pll_post_scaler(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x030e, &pll);
		dbg_sensor(1, "______ op_pre_pll_clk_dv(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0310, &pll);
		dbg_sensor(1, "______ op_pll_multiplier(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0312, &pll);
		dbg_sensor(1, "______ op_pll_post_scalar(0x%x)\n", pll);

		fimc_is_sensor_read16(client, 0x0314, &pll);
		dbg_sensor(1, "______ DRAM_pre_pll_clk_div(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0316, &pll);
		dbg_sensor(1, "______ DRAM_pll_multiplier(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0318, &pll);
		dbg_sensor(1, "______ DRAM_pll_post_scalar(0x%x)\n", pll);

		fimc_is_sensor_read16(client, 0x0340, &pll);
		dbg_sensor(1, "______ frame_length_lines(0x%x)\n", pll);
		fimc_is_sensor_read16(client, 0x0342, &pll);
		dbg_sensor(1, "______ line_length_pck(0x%x)\n", pll);
	}
#endif

	/*
	 * If a companion is used,
	 * then 8 ms waiting is needed before the StreamOn of a sensor (SAK2L4).
	 */
	if (test_bit(FIMC_IS_SENSOR_PREPROCESSOR_AVAILABLE, &sensor_peri->peri_state))
		mdelay(8);

	/* Sensor stream on */
#ifdef USE_FLIP_WIDE_SENSOR
	info("%s 180 degrees rotated\n", __func__);
	fimc_is_sensor_write16(client, 0x0100, 0x0100);
#else
	info("%s\n", __func__);
	fimc_is_sensor_write16(client, 0x0100, 0x0103);
#endif

	ret = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream on");

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis_data->stream_on = true;
	sensor_2l4_load_retention = true;

#ifdef CAMERA_REAR2
	mode = cis_data->sens_config_index_cur;
	dbg_sensor(1, "[%s] sens_config_index_cur=%d\n", __func__, mode);

	switch (mode) {
	case SENSOR_2L4_4032X3024_30FPS:
	case SENSOR_2L4_4032X2268_30FPS:
	case SENSOR_2L4_3024X3024_30FPS:
	case SENSOR_2L4_4032X1908_30FPS:
		cis->cis_data->min_sync_frame_us_time = cis->cis_data->min_frame_us_time = 33333;
		break;
	default:
		break;
	}
#endif

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_stream_off(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;
	u8 cur_frame_count = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	dbg_sensor(1, "[MOD:D:%d] %s\n", cis->id, __func__);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	ret = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
	if (ret < 0)
		err("group_param_hold_func failed at stream off");

	/* LN Off -> LN2 -> N+2 frame -> Stream Off */
	if (sensor_2l4_ln_mode_delay_count > 0) {
		info("%s: sensor_2l4_ln_mode_delay_count : %d ->(%d ms)\n",
			__func__, sensor_2l4_ln_mode_delay_count, 100 * sensor_2l4_ln_mode_delay_count);
		msleep(100 * sensor_2l4_ln_mode_delay_count);
	}

#ifdef CONFIG_SENSOR_RETENTION_USE
	/* retention mode CRC check register enable */
	if (cis->cis_data->cis_rev >= 0xA002) { // support retention higher than 3 stack
		fimc_is_sensor_write8(client, 0x010E, 0x01);
		info("[MOD:D:%d] %s : retention enable CRC check\n", cis->id, __func__);
	}
#endif

	fimc_is_sensor_read8(client, 0x0005, &cur_frame_count);
	info("%s: frame_count(0x%x)\n", __func__, cur_frame_count);

	fimc_is_sensor_write8(client, 0x0100, 0x00);
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	cis_data->stream_on = false;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_set_exposure_time(struct v4l2_subdev *subdev, struct ae_param *target_exposure)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

	u32 vt_pic_clk_freq_mhz = 0;
	u16 long_coarse_int = 0;
	u16 short_coarse_int = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;
	u16 coarse_integration_time_shifter = 0;

	u16 remainder_cit = 0;

	u16 cit_shifter_array[17] = {0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5};
	u16 cit_shifter_val = 0;
	int cit_shifter_idx = 0;
	u8 cit_denom_array[6] = {1, 2, 4, 8, 16, 32};

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!target_exposure);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_2l4_target_exp_backup.short_val = target_exposure->short_val;
	sensor_2l4_target_exp_backup.long_val = target_exposure->long_val;

	if ((target_exposure->long_val <= 0) || (target_exposure->short_val <= 0)) {
		err("[%s] invalid target exposure(%d, %d)\n", __func__,
				target_exposure->long_val, target_exposure->short_val);
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	if (cis->long_term_mode.sen_strm_off_on_enable == false) {
		switch(cis_data->sens_config_index_cur) {
		case SENSOR_2L4_2016X1512_30FPS:
		case SENSOR_2L4_2016X1134_30FPS:
		case SENSOR_2L4_1504X1504_30FPS:
			if (MAX(target_exposure->long_val, target_exposure->short_val) > 80000) {
				cit_shifter_idx = MIN(MAX(MAX(target_exposure->long_val, target_exposure->short_val) / 80000, 0), 16);
				cit_shifter_val = MAX(cit_shifter_array[cit_shifter_idx], cis_data->frame_length_lines_shifter);
			} else {
				cit_shifter_val = (u16)(cis_data->frame_length_lines_shifter);
			}
			target_exposure->long_val = target_exposure->long_val / cit_denom_array[cit_shifter_val];
			target_exposure->short_val = target_exposure->short_val / cit_denom_array[cit_shifter_val];
			coarse_integration_time_shifter = ((cit_shifter_val<<8) & 0xFF00) + (cit_shifter_val & 0x00FF);
			break;
		default:
			if (MAX(target_exposure->long_val, target_exposure->short_val) > 160000) {
				cit_shifter_idx = MIN(MAX(MAX(target_exposure->long_val, target_exposure->short_val) / 160000, 0), 16);
				cit_shifter_val = MAX(cit_shifter_array[cit_shifter_idx], cis_data->frame_length_lines_shifter);
			} else {
				cit_shifter_val = (u16)(cis_data->frame_length_lines_shifter);
			}
			target_exposure->long_val = target_exposure->long_val / cit_denom_array[cit_shifter_val];
			target_exposure->short_val = target_exposure->short_val / cit_denom_array[cit_shifter_val];
			coarse_integration_time_shifter = ((cit_shifter_val<<8) & 0xFF00) + (cit_shifter_val & 0x00FF);
			break;
		}
	}

	if (cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1
		|| cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2) {
		dbg_sensor(1, "[%s] skip for dram test\n", __func__);
		goto p_err;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), target long(%d), short(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, target_exposure->long_val, target_exposure->short_val);

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	switch (cis->cis_data->cur_lownoise_mode) {
	case FIMC_IS_CIS_LNOFF:
		long_coarse_int = ((target_exposure->long_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		remainder_cit = long_coarse_int % 2;
		long_coarse_int -= remainder_cit;
		if (long_coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, long_coarse_int, cis_data->min_coarse_integration_time);
			long_coarse_int = cis_data->min_coarse_integration_time;
		}
		short_coarse_int = ((target_exposure->short_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		remainder_cit = short_coarse_int % 2;
		short_coarse_int -= remainder_cit;
		if (short_coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, short_coarse_int, cis_data->min_coarse_integration_time);
			short_coarse_int = cis_data->min_coarse_integration_time;
		}
		break;
	case FIMC_IS_CIS_LN2:
		long_coarse_int = ((target_exposure->long_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		remainder_cit = long_coarse_int % 4;
		long_coarse_int -= remainder_cit;
		if (long_coarse_int < cis_data->min_coarse_integration_time * 2) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, long_coarse_int, cis_data->min_coarse_integration_time * 2);
			long_coarse_int = cis_data->min_coarse_integration_time * 2;
		}
		short_coarse_int = ((target_exposure->short_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		remainder_cit = short_coarse_int % 4;
		short_coarse_int -= remainder_cit;
		if (short_coarse_int < cis_data->min_coarse_integration_time * 2) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, short_coarse_int, cis_data->min_coarse_integration_time * 2);
			short_coarse_int = cis_data->min_coarse_integration_time * 2;
		}
		break;
	case FIMC_IS_CIS_LN4:
		long_coarse_int = ((target_exposure->long_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		remainder_cit = long_coarse_int % 8;
		long_coarse_int -= remainder_cit;
		if (long_coarse_int < cis_data->min_coarse_integration_time * 4) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, long_coarse_int, cis_data->min_coarse_integration_time * 4);
			long_coarse_int = cis_data->min_coarse_integration_time * 4;
		}
		short_coarse_int = ((target_exposure->short_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		remainder_cit = short_coarse_int % 8;
		short_coarse_int -= remainder_cit;
		if (short_coarse_int < cis_data->min_coarse_integration_time * 4) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, short_coarse_int, cis_data->min_coarse_integration_time * 4);
			short_coarse_int = cis_data->min_coarse_integration_time * 4;
		}
		break;
	default:
		long_coarse_int = ((target_exposure->long_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		if (long_coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, long_coarse_int, cis_data->min_coarse_integration_time);
			long_coarse_int = cis_data->min_coarse_integration_time;
		}
		short_coarse_int = ((target_exposure->short_val * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)
												/ line_length_pck;
		if (short_coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, short_coarse_int, cis_data->min_coarse_integration_time);
			short_coarse_int = cis_data->min_coarse_integration_time;
		}
		break;
	}

	if (long_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) max(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, long_coarse_int, cis_data->max_coarse_integration_time);
		long_coarse_int = cis_data->max_coarse_integration_time;
	}

	if (short_coarse_int > cis_data->max_coarse_integration_time) {
		dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), short coarse(%d) max(%d)\n", cis->id, __func__,
			cis_data->sen_vsync_count, short_coarse_int, cis_data->max_coarse_integration_time);
		short_coarse_int = cis_data->max_coarse_integration_time;
	}

	cis_data->cur_long_exposure_coarse = long_coarse_int;
	cis_data->cur_short_exposure_coarse = short_coarse_int;

	I2C_MUTEX_LOCK(cis->i2c_lock);
	hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/* WDR mode */
	if (sensor_2l4_cis_is_wdr_mode_on(cis_data)) {
		fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
		fimc_is_sensor_write16(cis->client, 0x021E, 0x0100);
	} else {
		fimc_is_sensor_write16(cis->client, 0x021E, 0x0000);
	}

	/* Short exposure */
	ret = fimc_is_sensor_write16(client, 0x0202, short_coarse_int);
	if (ret < 0)
		goto p_err;

	/* Long exposure */
	if (sensor_2l4_cis_is_wdr_mode_on(cis_data)) {
		ret = fimc_is_sensor_write16(client, 0x0226, long_coarse_int);
		if (ret < 0)
			goto p_err;
	}

	/* CIT shifter */
	if (cis->long_term_mode.sen_strm_off_on_enable == false) {
		ret = fimc_is_sensor_write16(client, 0x0702, coarse_integration_time_shifter);
		if (ret < 0)
			goto p_err;
	}

	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), vt_pic_clk_freq_mhz (%d),"
		KERN_CONT "line_length_pck(%d), min_fine_int (%d)\n",
		cis->id, __func__, cis_data->sen_vsync_count, vt_pic_clk_freq_mhz/1000,
		line_length_pck, min_fine_int);
	dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), frame_length_lines(%#x),"
		KERN_CONT "long_coarse_int %#x, short_coarse_int %#x coarse_integration_time_shifter %#x\n",
		cis->id, __func__, cis_data->sen_vsync_count, cis_data->frame_length_lines,
		long_coarse_int, short_coarse_int, coarse_integration_time_shifter);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	if (hold > 0) {
		hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_get_min_exposure_time(struct v4l2_subdev *subdev, u32 *min_expo)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	cis_shared_data *cis_data = NULL;
	u32 min_integration_time = 0;
	u32 min_coarse = 0;
	u32 min_fine = 0;
	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!min_expo);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000);
	if (vt_pic_clk_freq_mhz == 0) {
		pr_err("[MOD:D:%d] %s, Invalid vt_pic_clk_freq_mhz(%d)\n", cis->id, __func__, vt_pic_clk_freq_mhz/1000);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	min_coarse = cis_data->min_coarse_integration_time;
	min_fine = cis_data->min_fine_integration_time;

	min_integration_time = (u32)((u64)((line_length_pck * min_coarse) + min_fine) * 1000 / vt_pic_clk_freq_mhz);
	*min_expo = min_integration_time;

	dbg_sensor(1, "[%s] min integration time %d\n", __func__, min_integration_time);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_get_max_exposure_time(struct v4l2_subdev *subdev, u32 *max_expo)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;
	u32 max_integration_time = 0;
	u32 max_coarse_margin = 0;
	u32 max_fine_margin = 0;
	u32 max_coarse = 0;
	u32 max_fine = 0;
	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!max_expo);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000);
	if (vt_pic_clk_freq_mhz == 0) {
		pr_err("[MOD:D:%d] %s, Invalid vt_pic_clk_freq_mhz(%d)\n", cis->id, __func__, vt_pic_clk_freq_mhz/1000);
		goto p_err;
	}
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = cis_data->frame_length_lines;

	max_coarse_margin = cis_data->max_margin_coarse_integration_time;
	max_fine_margin = line_length_pck - cis_data->min_fine_integration_time;
	max_coarse = frame_length_lines - max_coarse_margin;
	max_fine = cis_data->max_fine_integration_time;

	max_integration_time = (u32)((u64)((line_length_pck * max_coarse) + max_fine) * 1000 / vt_pic_clk_freq_mhz);

	*max_expo = max_integration_time;

	/* TODO: Is this values update hear? */
	cis_data->max_margin_fine_integration_time = max_fine_margin;
	cis_data->max_coarse_integration_time = max_coarse;

	dbg_sensor(1, "[%s] max integration time %d, max margin fine integration %d, max coarse integration %d\n",
			__func__, max_integration_time,
			cis_data->max_margin_fine_integration_time, cis_data->max_coarse_integration_time);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_adjust_frame_duration(struct v4l2_subdev *subdev,
						u32 input_exposure_time,
						u32 *target_duration)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u32 frame_length_lines = 0;
	u32 frame_duration = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!target_duration);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	frame_length_lines = (u32)((((u64)(vt_pic_clk_freq_mhz) * input_exposure_time) / 1000
						- cis_data->min_fine_integration_time) / line_length_pck);
	frame_length_lines += cis_data->max_margin_coarse_integration_time;

	frame_duration = (u32)(((u64)frame_length_lines * line_length_pck) * 1000 / vt_pic_clk_freq_mhz);

	dbg_sensor(1, "[%s](vsync cnt = %d) input exp(%d), adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count,
			input_exposure_time, frame_duration, cis_data->min_frame_us_time);
	dbg_sensor(1, "[%s](vsync cnt = %d) adj duration, frame duraion(%d), min_frame_us(%d)\n",
			__func__, cis_data->sen_vsync_count, frame_duration, cis_data->min_frame_us_time);

	*target_duration = MAX(frame_duration, cis_data->min_frame_us_time);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

	return ret;
}

int sensor_2l4_cis_set_frame_duration(struct v4l2_subdev *subdev, u32 frame_duration)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u16 frame_length_lines = 0;
	u8 frame_length_lines_shifter = 0;

	u8 fll_shifter_array[17] = {0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5};
	int fll_shifter_idx = 0;
	u8 fll_denom_array[6] = {1, 2, 4, 8, 16, 32};

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;

	if (frame_duration < cis_data->min_frame_us_time) {
		dbg_sensor(1, "frame duration is less than min(%d)\n", frame_duration);
		frame_duration = cis_data->min_frame_us_time;
	}

	sensor_2l4_frame_duration_backup = frame_duration;

	if (sensor_2l4_ln_mode_delay_count > 0)
		sensor_2l4_ln_mode_delay_count--;

	if (cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1
		|| cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2) {
		dbg_sensor(1, "[%s] skip for dram test\n", __func__);
		goto p_err;
	}

	if (cis->long_term_mode.sen_strm_off_on_enable == false) {
		switch(cis_data->sens_config_index_cur) {
		case SENSOR_2L4_2016X1512_30FPS:
		case SENSOR_2L4_2016X1134_30FPS:
		case SENSOR_2L4_1504X1504_30FPS:
			if (frame_duration > 80000) {
				fll_shifter_idx = MIN(MAX(frame_duration / 80000, 0), 16);
				frame_length_lines_shifter = fll_shifter_array[fll_shifter_idx];
				frame_duration = frame_duration / fll_denom_array[frame_length_lines_shifter];
			} else {
				frame_length_lines_shifter = 0x0;
			}
			break;
		default:
			if (frame_duration > 160000) {
				fll_shifter_idx = MIN(MAX(frame_duration / 160000, 0), 16);
				frame_length_lines_shifter = fll_shifter_array[fll_shifter_idx];
				frame_duration = frame_duration / fll_denom_array[frame_length_lines_shifter];
			} else {
				frame_length_lines_shifter = 0x0;
			}
			break;
		}
	}

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;

	frame_length_lines = (u16)(((u64)(vt_pic_clk_freq_mhz) * frame_duration) / (line_length_pck * 1000));

	dbg_sensor(1, "[MOD:D:%d] %s, vt_pic_clk_freq_mhz(%#x) frame_duration = %d us,"
			KERN_CONT "(line_length_pck%#x), frame_length_lines(%#x), frame_length_lines_shifter(%#x)\n",
			cis->id, __func__, vt_pic_clk_freq_mhz/1000, frame_duration,
			line_length_pck, frame_length_lines, frame_length_lines_shifter);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	if (cis->cis_data->cur_lownoise_mode != cis->cis_data->pre_lownoise_mode)
		ret |= sensor_2l4_cis_set_lownoise_mode_change(subdev);

	ret |= fimc_is_sensor_write16(client, 0x0340, frame_length_lines);

	if (ret < 0)
		goto p_err;

	/* frame duration shifter */
	if (cis->long_term_mode.sen_strm_off_on_enable == false) {
		ret = fimc_is_sensor_write8(client, 0x0701, frame_length_lines_shifter);
		if (ret < 0)
			goto p_err;
	}

	cis_data->cur_frame_us_time = frame_duration;
	cis_data->frame_length_lines = frame_length_lines;
	cis_data->max_coarse_integration_time = cis_data->frame_length_lines - cis_data->max_margin_coarse_integration_time;
	cis_data->frame_length_lines_shifter = frame_length_lines_shifter;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	if (hold > 0) {
		hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_set_frame_rate(struct v4l2_subdev *subdev, u32 min_fps)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u32 frame_duration = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	if (min_fps > cis_data->max_fps) {
		err("[MOD:D:%d] %s, request FPS is too high(%d), set to max(%d)\n",
			cis->id, __func__, min_fps, cis_data->max_fps);
		min_fps = cis_data->max_fps;
	}

	if (min_fps == 0) {
		err("[MOD:D:%d] %s, request FPS is 0, set to min FPS(1)\n",
			cis->id, __func__);
		min_fps = 1;
	}

	frame_duration = (1 * 1000 * 1000) / min_fps;

	dbg_sensor(1, "[MOD:D:%d] %s, set FPS(%d), frame duration(%d)\n",
			cis->id, __func__, min_fps, frame_duration);

	ret = sensor_2l4_cis_set_frame_duration(subdev, frame_duration);
	if (ret < 0) {
		err("[MOD:D:%d] %s, set frame duration is fail(%d)\n",
			cis->id, __func__, ret);
		goto p_err;
	}

#ifdef CAMERA_REAR2
	cis_data->min_frame_us_time = MAX(frame_duration, cis_data->min_sync_frame_us_time);
#else
	cis_data->min_frame_us_time = frame_duration;
#endif

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:

	return ret;
}

int sensor_2l4_cis_adjust_analog_gain(struct v4l2_subdev *subdev, u32 input_again, u32 *target_permile)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u32 again_code = 0;
	u32 again_permile = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!target_permile);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	cis_data = cis->cis_data;

	again_code = sensor_cis_calc_again_code(input_again);

	if (again_code > cis_data->max_analog_gain[0])
		again_code = cis_data->max_analog_gain[0];
	else if (again_code < cis_data->min_analog_gain[0])
		again_code = cis_data->min_analog_gain[0];

	again_permile = sensor_cis_calc_again_permile(again_code);

	dbg_sensor(1, "[%s] min again(%d), max(%d), input_again(%d), code(%d), permile(%d)\n", __func__,
			cis_data->max_analog_gain[0],
			cis_data->min_analog_gain[0],
			input_again,
			again_code,
			again_permile);

	*target_permile = again_permile;

	return ret;
}

int sensor_2l4_cis_set_analog_gain(struct v4l2_subdev *subdev, struct ae_param *again)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;

	u16 analog_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_2l4_again_backup.short_val = again->short_val;
	sensor_2l4_again_backup.long_val = again->long_val;

	if (cis->cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1
		|| cis->cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2) {
		dbg_sensor(1, "[%s] skip for dram test\n", __func__);
		goto p_err;
	}

	analog_gain = (u16)sensor_cis_calc_again_code(again->val);

	if (analog_gain < cis->cis_data->min_analog_gain[0]) {
		info("[%s] not proper analog_gain value, reset to min_analog_gain\n", __func__);
		analog_gain = cis->cis_data->min_analog_gain[0];
	}

	if (analog_gain > cis->cis_data->max_analog_gain[0]) {
		info("[%s] not proper analog_gain value, reset to max_analog_gain\n", __func__);
		analog_gain = cis->cis_data->max_analog_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_again = %d us, analog_gain(%#x)\n",
		cis->id, __func__, cis->cis_data->sen_vsync_count, again->val, analog_gain);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = fimc_is_sensor_write16(client, 0x0204, analog_gain);
	if (ret < 0)
		goto p_err;

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	if (hold > 0) {
		hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_get_analog_gain(struct v4l2_subdev *subdev, u32 *again)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;

	u16 analog_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	ret = fimc_is_sensor_read16(client, 0x0204, &analog_gain);
	if (ret < 0)
		goto p_err;

	*again = sensor_cis_calc_again_permile(analog_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_again = %d us, analog_gain(%#x)\n",
			cis->id, __func__, *again, analog_gain);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	if (hold > 0) {
		hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_get_min_analog_gain(struct v4l2_subdev *subdev, u32 *min_again)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!min_again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	cis_data->min_analog_gain[0] = 0x20; /* x1, gain=x/0x20 */
	cis_data->min_analog_gain[1] = sensor_cis_calc_again_permile(cis_data->min_analog_gain[0]);

	*min_again = cis_data->min_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->min_analog_gain[0],
		cis_data->min_analog_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_get_max_analog_gain(struct v4l2_subdev *subdev, u32 *max_again)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!max_again);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	cis_data->max_analog_gain[0] = 0x200; /* x16, gain=x/0x20 */
	cis_data->max_analog_gain[1] = sensor_cis_calc_again_permile(cis_data->max_analog_gain[0]);

	*max_again = cis_data->max_analog_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->max_analog_gain[0],
		cis_data->max_analog_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_set_digital_gain(struct v4l2_subdev *subdev, struct ae_param *dgain)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

	u16 long_gain = 0;
	u16 short_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	sensor_2l4_dgain_backup.short_val = dgain->short_val;
	sensor_2l4_dgain_backup.long_val = dgain->long_val;

	cis_data = cis->cis_data;

	if (cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION1
		|| cis_data->sens_config_index_cur == SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2) {
		dbg_sensor(1, "[%s] skip for dram test\n", __func__);
		goto p_err;
	}

	long_gain = (u16)sensor_cis_calc_dgain_code(dgain->long_val);
	short_gain = (u16)sensor_cis_calc_dgain_code(dgain->short_val);

	if (long_gain < cis_data->min_digital_gain[0]) {
		info("[%s] not proper long_gain value, reset to min_digital_gain\n", __func__);
		long_gain = cis_data->min_digital_gain[0];
	}

	if (long_gain > cis_data->max_digital_gain[0]) {
		info("[%s] not proper long_gain value, reset to max_digital_gain\n", __func__);
		long_gain = cis_data->max_digital_gain[0];
	}

	if (short_gain < cis_data->min_digital_gain[0]) {
		info("[%s] not proper short_gain value, reset to min_digital_gain\n", __func__);
		short_gain = cis_data->min_digital_gain[0];
	}

	if (short_gain > cis_data->max_digital_gain[0]) {
		info("[%s] not proper short_gain value, reset to max_digital_gain\n", __func__);
		short_gain = cis_data->max_digital_gain[0];
	}

	dbg_sensor(1, "[MOD:D:%d] %s(vsync cnt = %d), input_dgain = %d/%d us,"
			KERN_CONT "long_gain(%#x), short_gain(%#x)\n",
			cis->id, __func__, cis->cis_data->sen_vsync_count,
			dgain->long_val, dgain->short_val, long_gain, short_gain);

	I2C_MUTEX_LOCK(cis->i2c_lock);
	hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/*
	 * NOTE : In SAK2L4, digital gain is long/short seperated, should set 2 registers like below,
	 * Write same value to : 0x020E : short // GreenB
	 * Write same value to : 0x0214 : short // GreenR
	 * Write same value to : Need To find : long
	 */

	/* Short digital gain */
	ret = fimc_is_sensor_write16(client, 0x020E, short_gain);
	if (ret < 0)
		goto p_err;

	/* Long digital gain */
	if (sensor_2l4_cis_is_wdr_mode_on(cis_data)) {
		ret = fimc_is_sensor_write16(client, 0x0C80, long_gain);
		if (ret < 0)
			goto p_err;
	}

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	if (hold > 0) {
		hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_get_digital_gain(struct v4l2_subdev *subdev, u32 *dgain)
{
	int ret = 0;
	int hold = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;

	u16 digital_gain = 0;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	I2C_MUTEX_LOCK(cis->i2c_lock);
	hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x01);
	if (hold < 0) {
		ret = hold;
		goto p_err;
	}

	/*
	 * NOTE : In SAK2L4, digital gain is long/short seperated, should set 2 registers like below,
	 * Write same value to : 0x020E : short // GreenB
	 * Write same value to : 0x0214 : short // GreenR
	 * Write same value to : Need To find : long
	 */

	ret = fimc_is_sensor_read16(client, 0x020E, &digital_gain);
	if (ret < 0)
		goto p_err;

	*dgain = sensor_cis_calc_dgain_permile(digital_gain);

	dbg_sensor(1, "[MOD:D:%d] %s, cur_dgain = %d us, digital_gain(%#x)\n",
			cis->id, __func__, *dgain, digital_gain);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	if (hold > 0) {
		hold = sensor_2l4_cis_group_param_hold_func(subdev, 0x00);
		if (hold < 0)
			ret = hold;
	}
	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	return ret;
}

int sensor_2l4_cis_get_min_digital_gain(struct v4l2_subdev *subdev, u32 *min_dgain)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!min_dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	cis_data->min_digital_gain[0] = 0x100;
	cis_data->min_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->min_digital_gain[0]);

	*min_dgain = cis_data->min_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->min_digital_gain[0],
		cis_data->min_digital_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_get_max_digital_gain(struct v4l2_subdev *subdev, u32 *max_dgain)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct i2c_client *client;
	cis_shared_data *cis_data;

#ifdef DEBUG_SENSOR_TIME
	struct timeval st, end;

	do_gettimeofday(&st);
#endif

	WARN_ON(!subdev);
	WARN_ON(!max_dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);

	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	client = cis->client;
	if (unlikely(!client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	cis_data = cis->cis_data;
	cis_data->max_digital_gain[0] = 0x8000;
	cis_data->max_digital_gain[1] = sensor_cis_calc_dgain_permile(cis_data->max_digital_gain[0]);

	*max_dgain = cis_data->max_digital_gain[1];

	dbg_sensor(1, "[%s] code %d, permile %d\n", __func__, cis_data->max_digital_gain[0],
		cis_data->max_digital_gain[1]);

#ifdef DEBUG_SENSOR_TIME
	do_gettimeofday(&end);
	dbg_sensor(1, "[%s] time %lu us\n", __func__, (end.tv_sec - st.tv_sec)*1000000 + (end.tv_usec - st.tv_usec));
#endif

p_err:
	return ret;
}

int sensor_2l4_cis_long_term_exposure(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	struct fimc_is_long_term_expo_mode *lte_mode;
	unsigned char cit_lshift_val = 0;
	unsigned char shift_count = 0;
#ifdef USE_SENSOR_LONG_EXPOSURE_SHOT
	u32 lte_expousre = 0;
#endif
	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	lte_mode = &cis->long_term_mode;

	I2C_MUTEX_LOCK(cis->i2c_lock);
	/* LTE mode or normal mode set */
	if (lte_mode->sen_strm_off_on_enable) {
		if (lte_mode->expo[0] > 125000) {
#ifdef USE_SENSOR_LONG_EXPOSURE_SHOT
			lte_expousre = lte_mode->expo[0];
			cit_lshift_val = (unsigned char)(lte_mode->expo[0] / 125000);
			while (cit_lshift_val) {
				cit_lshift_val = cit_lshift_val / 2;
				lte_expousre = lte_expousre / 2;
				shift_count++;
			}
			lte_mode->expo[0] = lte_expousre;
#else
			cit_lshift_val = (unsigned char)(lte_mode->expo[0] / 125000);
			while (cit_lshift_val) {
			    cit_lshift_val = cit_lshift_val / 2;
			    if (cit_lshift_val > 0)
			        shift_count++;
			}
			lte_mode->expo[0] = 125000;
#endif 
			ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
			ret |= fimc_is_sensor_write8(cis->client, 0x0701, shift_count);
			ret |= fimc_is_sensor_write8(cis->client, 0x0702, shift_count);
		}
	} else {
		cit_lshift_val = 0;
		ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
		ret |= fimc_is_sensor_write8(cis->client, 0x0701, cit_lshift_val);
		ret |= fimc_is_sensor_write8(cis->client, 0x0702, cit_lshift_val);
	}

	I2C_MUTEX_UNLOCK(cis->i2c_lock);

	info("%s enable(%d)", __func__, lte_mode->sen_strm_off_on_enable);

	if (ret < 0) {
		pr_err("ERR[%s]: LTE register setting fail\n", __func__);
		return ret;
	}

	return ret;
}

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
static int sensor_2l4_cis_update_mipi_info(struct v4l2_subdev *subdev)
{
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_device_sensor *device;
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;
	int found = -1;

	device = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (device == NULL) {
		err("device is NULL");
		return -1;
	}

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (cis == NULL) {
		err("cis is NULL");
		return -1;
	}

	if (device->cfg->mode >= sensor_2l4_mipi_sensor_mode_size) {
		err("sensor mode is out of bound");
		return -1;
	}

	cur_mipi_sensor_mode = &sensor_2l4_mipi_sensor_mode[device->cfg->mode];

	if (cur_mipi_sensor_mode->mipi_channel_size == 0 ||
		cur_mipi_sensor_mode->mipi_channel == NULL) {
		dbg_sensor(1, "skip select mipi channel\n");
		return -1;
	}

	found = fimc_is_vendor_select_mipi_by_rf_channel(cur_mipi_sensor_mode->mipi_channel,
				cur_mipi_sensor_mode->mipi_channel_size);
	if (found != -1) {
		if (found < cur_mipi_sensor_mode->sensor_setting_size) {
			device->cfg->mipi_speed = cur_mipi_sensor_mode->sensor_setting[found].mipi_rate;
			cis->mipi_clock_index_new = found;
			info("%s - update mipi rate : %d\n", __func__, device->cfg->mipi_speed);
		} else {
			err("sensor setting size is out of bound");
		}
	}

	return 0;
}

static int sensor_2l4_cis_get_mipi_clock_string(struct v4l2_subdev *subdev, char *cur_mipi_str)
{
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_device_sensor *device;
	const struct cam_mipi_sensor_mode *cur_mipi_sensor_mode;
	int mode = 0;

	cur_mipi_str[0] = '\0';

	device = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev);
	if (device == NULL) {
		err("device is NULL");
		return -1;
	}

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (cis == NULL) {
		err("cis is NULL");
		return -1;
	}

	if (cis->cis_data->stream_on) {
		mode = cis->cis_data->sens_config_index_cur;

		if (mode >= sensor_2l4_mipi_sensor_mode_size) {
			err("sensor mode is out of bound");
			return -1;
		}

		cur_mipi_sensor_mode = &sensor_2l4_mipi_sensor_mode[mode];

		if (cur_mipi_sensor_mode->sensor_setting_size == 0 ||
			cur_mipi_sensor_mode->sensor_setting == NULL) {
			err("sensor_setting is not available");
			return -1;
		}

		if (cis->mipi_clock_index_new < 0 ||
			cur_mipi_sensor_mode->sensor_setting[cis->mipi_clock_index_new].str_mipi_clk == NULL) {
			err("mipi_clock_index_new is not available");
			return -1;
		}

		sprintf(cur_mipi_str, "%s",
			cur_mipi_sensor_mode->sensor_setting[cis->mipi_clock_index_new].str_mipi_clk);
	}

	return 0;
}
#endif

int sensor_2l4_cis_set_frs_control(struct v4l2_subdev *subdev, u32 command)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	switch (command) {
	case FRS_SSM_START:
		pr_info("[%s] SUPER_SLOW_MOTION_START\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A52, 0x01); /* start_user_record */
		/* ret |= fimc_is_sensor_write8(cis->client, 0x0A51, 0x01); *//* enable_preview_during_recording */
		/* ret |= fimc_is_sensor_write8(cis->client, 0x0A55, 0x08); *//* tg_to_oif_ratio */
		/* ret |= fimc_is_sensor_write8(cis->client, 0x0A56, 0x02); *//* tg_to_sg_ratio */
		/* ret |= fimc_is_sensor_write16(cis->client, 0x0A58, 0x0010); *//* q_mask_frames */
		/* ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0010); *//* before_q_frames */
		/* ret |= fimc_is_sensor_write16(cis->client, 0x0A60, 0x0050); *//* dram_frame_num */
		break;
	case FRS_SSM_MANUAL_CUE_ENABLE:
		pr_info("[%s] SUPER_SLOW_MOTION_START_MANUAL_CUE\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A54, 0x01); /* Manual Q Enable */
		break;
	case FRS_SSM_STOP:
		pr_info("[%s] SUPER_SLOW_MOTION_STOP\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A53, 0x01); /* stop_user_record */
		break;
	case FRS_SSM_MODE_AUTO_MANUAL_CUE_16:
		pr_info("[%s] SUPER_SLOW_MOTION_MODE_AUTO_MANUAL_CUE_16\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A50, 0x02);    /* Enable Manual /Auto Q */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0010); /* before_q_frames = 16 */
		break;
	case FRS_SSM_MODE_AUTO_MANUAL_CUE_32:
		pr_info("[%s] SUPER_SLOW_MOTION_MODE_AUTO_MANUAL_CUE_32\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A50, 0x02);    /* Enable Manual /Auto Q */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0020); /* before_q_frames = 32 */
		break;
	case FRS_SSM_MODE_AUTO_MANUAL_CUE_48:
		pr_info("[%s] SUPER_SLOW_MOTION_MODE_AUTO_MANUAL_CUE_48\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A50, 0x02);    /* Enable Manual /Auto Q */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0030); /* before_q_frames = 48 */
		break;
	case FRS_SSM_MODE_AUTO_MANUAL_CUE_64:
		pr_info("[%s] SUPER_SLOW_MOTION_MODE_AUTO_MANUAL_CUE_64\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A50, 0x02);    /* Enable Manual /Auto Q */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0040); /* before_q_frames = 64 */
		break;
	case FRS_SSM_MODE_ONLY_MANUAL_CUE:
		ret |= fimc_is_sensor_write8(cis->client, 0x0A50, 0x01);    /* Enable Manual Q Only */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A58, 0x0000); /* q_mask_frames */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0000); /* before_q_frames = 0 */
		break;
	case FRS_SSM_MODE_FACTORY_TEST:
		pr_info("[%s] SUPER_SLOW_MOTION_MODE_FACTORY_TEST\n", __func__);
		ret |= fimc_is_sensor_write8(cis->client, 0x0A50, 0x01);    /* Enable Manual Q Only */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A58, 0x0000); /* q_mask_frames */
		ret |= fimc_is_sensor_write16(cis->client, 0x0A5A, 0x0010); /* before_q_frames = 16 */
		ret |= fimc_is_sensor_write8(cis->client, 0x0A52, 0x01); /* start_user_record */
		ret |= fimc_is_sensor_write16(cis->client, 0xF40A, 0x0009); /* test */
		ret |= fimc_is_sensor_write16(cis->client, 0xF404, 0xFFF7); /* test */

		ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
		break;
	case FRS_DRAM_TEST_SECTION2:
		if (sensor_2l4_max_setfile_num > SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2) {
			/* stream off > set dram stage2 > stream on*/
			fimc_is_sensor_write8(cis->client, 0x0100, 0x00);
			pr_info("[%s] FRS dram test section 1 stream off\n", __func__);

			sensor_cis_wait_streamoff(subdev);
			ret = sensor_cis_set_registers(subdev,
					sensor_2l4_setfiles[SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2],
					sensor_2l4_setfile_sizes[SENSOR_2L4_4032X3024_30FPS_MODE2_DRAM_TEST_SECTION2]);
#ifdef USE_FLIP_WIDE_SENSOR
			info("%s 180 degrees rotated\n", __func__);
			fimc_is_sensor_write16(cis->client, 0x0100, 0x0100);
#else
			fimc_is_sensor_write16(cis->client, 0x0100, 0x0103);
#endif
			pr_info("[%s] FRS dram test section 2 stream on\n", __func__);

			sensor_cis_wait_streamon(subdev);
			pr_info("[%s] FRS dram section 2 frame reached\n", __func__);
		} else {
			pr_info("[%s] FRS dram section 2 setting is not found\n", __func__);
		}
		break;
	default:
		pr_info("[%s] not support command(%d)\n", __func__, command);
	}

	if (ret < 0) {
		pr_err("ERR[%s]: super slow control setting fail\n", __func__);
		return ret;
	}

	return ret;
}

int sensor_2l4_cis_set_super_slow_motion_roi(struct v4l2_subdev *subdev, struct v4l2_rect *ssm_roi)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	pr_info("[%s] : left(%d), width(%d), top(%d), height(%d)\n", __func__,
		ssm_roi->left, ssm_roi->width, ssm_roi->top, ssm_roi->height);

	ret |= fimc_is_sensor_write16(cis->client, 0x0A64, ssm_roi->left);
	ret |= fimc_is_sensor_write16(cis->client, 0x0A66, ssm_roi->width);
	ret |= fimc_is_sensor_write16(cis->client, 0x0A68, ssm_roi->top);
	ret |= fimc_is_sensor_write16(cis->client, 0x0A6A, ssm_roi->height);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow roi setting fail\n", __func__);
		return ret;
	}

	return ret;
}

int sensor_2l4_cis_set_super_slow_motion_threshold(struct v4l2_subdev *subdev, u32 threshold)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u8 final_threshold = (u8)threshold;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_write16(cis->client, 0x6028, 0x2001);
	ret |= fimc_is_sensor_write16(cis->client, 0x602A, 0x2CC0);
	ret |= fimc_is_sensor_write16(cis->client, 0x6F12, final_threshold);
	ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow threshold setting fail\n", __func__);
	}

	pr_info("[%s] : super slow threshold(%d)\n", __func__, threshold);

	return ret;
}

int sensor_2l4_cis_get_super_slow_motion_threshold(struct v4l2_subdev *subdev, u32 *threshold)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u8 final_threshold = 0;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_write16(cis->client, 0x602C, 0x2000);
	ret |= fimc_is_sensor_write16(cis->client, 0x602E, 0xFF75);
	ret |= fimc_is_sensor_read8(cis->client, 0x6F12, &final_threshold);
	ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow threshold getting fail\n", __func__);
		*threshold = 0;
		return ret;
	}

	*threshold = final_threshold;

	pr_info("[%s] : super slow threshold(%d)\n", __func__, *threshold);

	return ret;
}

int sensor_2l4_cis_get_super_slow_motion_gmc(struct v4l2_subdev *subdev, u32 *gmc)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u8 gmc_state = 0;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_write16(cis->client, 0x602C, 0x2001);
	ret |= fimc_is_sensor_write16(cis->client, 0x602E, 0x0E34);
	ret |= fimc_is_sensor_read8(cis->client, 0x6F12, &gmc_state);
	ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow gmc getting fail\n", __func__);
		*gmc = 0;
		return ret;
	}

	*gmc = gmc_state;

	pr_info("[%s] : super slow gmc(%d)\n", __func__, *gmc);

	return ret;
}

int sensor_2l4_cis_get_super_slow_motion_frame_id(struct v4l2_subdev *subdev, u32 *frameid)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u8 frame_id = 0;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_read8(cis->client, 0x1A0C, &frame_id);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow frame id getting fail\n", __func__);
		*frameid = 0;
		return ret;
	}

	*frameid = frame_id;

	pr_info("[%s] : super slow frame_id(%d)\n", __func__, *frameid);

	return ret;
}

int sensor_2l4_cis_set_super_slow_motion_flicker(struct v4l2_subdev *subdev, u32 flicker)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u8 final_flicker = (u8)flicker;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_write8(cis->client, 0x0C21, final_flicker);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow flicker setting fail\n", __func__);
	}

	pr_info("[%s] : super slow flicker(%d)\n", __func__, flicker);

	return ret;
}

int sensor_2l4_cis_get_super_slow_motion_md_threshold(struct v4l2_subdev *subdev, u32 *threshold)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u16 md_threshold = 0;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_read16(cis->client, 0xDB0E, &md_threshold);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow threshold getting fail\n", __func__);
		*threshold = 0;
		return ret;
	}

	*threshold = md_threshold;

	pr_info("[%s] : super slow threshold(%d)\n", __func__, *threshold);

	return ret;
}

int sensor_2l4_cis_set_super_slow_motion_gmc_table_idx(struct v4l2_subdev *subdev, u32 index)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u16 table_idx = (u16)index;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_write16(cis->client, 0x6028, 0x2001);
	ret |= fimc_is_sensor_write16(cis->client, 0x602A, 0x3D14);
	ret |= fimc_is_sensor_write16(cis->client, 0x6F12, table_idx);
	ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow gmc table idx setting fail\n", __func__);
	}

	pr_info("[%s] : super slow table_idx(%d)\n", __func__, table_idx);

	return ret;
}

int sensor_2l4_cis_set_super_slow_motion_gmc_block_with_md_low(struct v4l2_subdev *subdev, u32 block)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	u16 gmc_block = (u16)block;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	ret |= fimc_is_sensor_write16(cis->client, 0x6028, 0x2001);
	ret |= fimc_is_sensor_write16(cis->client, 0x602A, 0x3D68);
	ret |= fimc_is_sensor_write16(cis->client, 0x6F12, gmc_block);
	ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
	if (ret < 0) {
		pr_err("ERR[%s]: super slow gmc block with md low setting fail\n", __func__);
	}

	pr_info("[%s] : super slow gmc_block(%d)\n", __func__, gmc_block);

	return ret;
}

int sensor_2l4_cis_set_factory_control(struct v4l2_subdev *subdev, u32 command)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info = NULL;

	WARN_ON(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	WARN_ON(!cis);
	WARN_ON(!cis->cis_data);

	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	WARN_ON(!ext_info);

	switch (command) {
	case FAC_CTRL_BIT_TEST:
		pr_info("[%s] FAC_CTRL_BIT_TEST\n", __func__);
		ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x4000);
		ret |= fimc_is_sensor_write16(cis->client, 0xF44A, 0x0009); // TG 2.55v -> 2.45v
		ret |= fimc_is_sensor_write16(cis->client, 0xF44C, 0x0009); // TG 2.55v -> 2.45v
		msleep(50);
		ret |= fimc_is_sensor_write16(cis->client, 0xFCFC, 0x2000);
		ret |= fimc_is_sensor_write16(cis->client, 0xAE16, 0x0010); // RG 3.70v -> 3.30v
		ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
		break;
	default:
		pr_info("[%s] not support command(%d)\n", __func__, command);
	}

	return ret;
}

int sensor_2l4_cis_compensate_gain_for_extremely_br(struct v4l2_subdev *subdev, u32 expo, u32 *again, u32 *dgain)
{
	int ret = 0;
	struct fimc_is_cis *cis;
	cis_shared_data *cis_data;

	u32 vt_pic_clk_freq_mhz = 0;
	u32 line_length_pck = 0;
	u32 min_fine_int = 0;
	u32 coarse_int = 0;
	u32 compensated_again = 0;
	u32 remainder_cit = 0;

	FIMC_BUG(!subdev);
	FIMC_BUG(!again);
	FIMC_BUG(!dgain);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	if (!cis) {
		err("cis is NULL");
		ret = -EINVAL;
		goto p_err;
	}
	cis_data = cis->cis_data;

	vt_pic_clk_freq_mhz = cis_data->pclk / (1000);
	line_length_pck = cis_data->line_length_pck;
	min_fine_int = cis_data->min_fine_integration_time;

	if (line_length_pck <= 0) {
		err("[%s] invalid line_length_pck(%d)\n", __func__, line_length_pck);
		goto p_err;
	}

	switch (cis->cis_data->cur_lownoise_mode) {
	case FIMC_IS_CIS_LNOFF:
		coarse_int = ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int) / line_length_pck;
		remainder_cit = coarse_int % 2;
		coarse_int -= remainder_cit;
		if (coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_int, cis_data->min_coarse_integration_time);
			coarse_int = cis_data->min_coarse_integration_time;
		}
		break;
	case FIMC_IS_CIS_LN2:
		coarse_int = ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int) / line_length_pck;
		remainder_cit = coarse_int % 4;
		coarse_int -= remainder_cit;
		if (coarse_int < cis_data->min_coarse_integration_time * 2) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_int, cis_data->min_coarse_integration_time * 2);
			coarse_int = cis_data->min_coarse_integration_time * 2;
		}
		break;
	case FIMC_IS_CIS_LN4:
		coarse_int = ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int) / line_length_pck;
		remainder_cit = coarse_int % 8;
		coarse_int -= remainder_cit;
		if (coarse_int < cis_data->min_coarse_integration_time * 4) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_int, cis_data->min_coarse_integration_time * 4);
			coarse_int = cis_data->min_coarse_integration_time * 4;
		}
		break;
	default:
		coarse_int = ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int) / line_length_pck;
		if (coarse_int < cis_data->min_coarse_integration_time) {
			dbg_sensor(1, "[MOD:D:%d] %s, vsync_cnt(%d), long coarse(%d) min(%d)\n", cis->id, __func__,
				cis_data->sen_vsync_count, coarse_int, cis_data->min_coarse_integration_time);
			coarse_int = cis_data->min_coarse_integration_time;
		}
		break;
	}

	if (coarse_int <= 1024) {
		compensated_again = (*again * ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)) / (line_length_pck * coarse_int);

		if (compensated_again < cis_data->min_analog_gain[1]) {
			*again = cis_data->min_analog_gain[1];
		} else if (*again >= cis_data->max_analog_gain[1]) {
			*dgain = (*dgain * ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)) / (line_length_pck * coarse_int);
		} else {
			//*again = compensated_again;
			*dgain = (*dgain * ((expo * (u64)(vt_pic_clk_freq_mhz)) / 1000 - min_fine_int)) / (line_length_pck * coarse_int);
		}

		dbg_sensor(1, "[%s] exp(%d), again(%d), dgain(%d), coarse_int(%d), compensated_again(%d)\n",
			__func__, expo, *again, *dgain, coarse_int, compensated_again);
	}

p_err:
	return ret;
}

int sensor_2l4_cis_recover_stream_on(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_cis *cis = NULL;
#ifdef CONFIG_SENSOR_RETENTION_USE
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	struct sensor_open_extended *ext_info;
#endif

	FIMC_BUG(!subdev);

	cis = (struct fimc_is_cis *)v4l2_get_subdevdata(subdev);
	FIMC_BUG(!cis);
	FIMC_BUG(!cis->cis_data);

#ifdef CONFIG_SENSOR_RETENTION_USE
	sensor_peri = container_of(cis, struct fimc_is_device_sensor_peri, cis);
	module = sensor_peri->module;
	ext_info = &module->ext;
	FIMC_BUG(!ext_info);

	ext_info->use_retention_mode = SENSOR_RETENTION_INACTIVE;
#endif

	info("%s start\n", __func__);

	ret = sensor_2l4_cis_set_global_setting(subdev);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_mode_change(subdev, cis->cis_data->sens_config_index_cur);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_set_frame_duration(subdev, sensor_2l4_frame_duration_backup);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_set_analog_gain(subdev, &sensor_2l4_again_backup);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_set_digital_gain(subdev, &sensor_2l4_dgain_backup);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_set_exposure_time(subdev, &sensor_2l4_target_exp_backup);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_stream_on(subdev);
	if (ret < 0) goto p_err;
	ret = sensor_2l4_cis_wait_streamon(subdev);
	if (ret < 0) goto p_err;

	info("%s end\n", __func__);
p_err:
	return ret;
}

static struct fimc_is_cis_ops cis_ops_2l4 = {
	.cis_init = sensor_2l4_cis_init,
	.cis_deinit = sensor_2l4_cis_deinit,
	.cis_log_status = sensor_2l4_cis_log_status,
	.cis_group_param_hold = sensor_2l4_cis_group_param_hold,
	.cis_set_global_setting = sensor_2l4_cis_set_global_setting,
	.cis_mode_change = sensor_2l4_cis_mode_change,
	.cis_set_size = sensor_2l4_cis_set_size,
	.cis_stream_on = sensor_2l4_cis_stream_on,
	.cis_stream_off = sensor_2l4_cis_stream_off,
	.cis_set_exposure_time = sensor_2l4_cis_set_exposure_time,
	.cis_get_min_exposure_time = sensor_2l4_cis_get_min_exposure_time,
	.cis_get_max_exposure_time = sensor_2l4_cis_get_max_exposure_time,
	.cis_adjust_frame_duration = sensor_2l4_cis_adjust_frame_duration,
	.cis_set_frame_duration = sensor_2l4_cis_set_frame_duration,
	.cis_set_frame_rate = sensor_2l4_cis_set_frame_rate,
	.cis_adjust_analog_gain = sensor_2l4_cis_adjust_analog_gain,
	.cis_set_analog_gain = sensor_2l4_cis_set_analog_gain,
	.cis_get_analog_gain = sensor_2l4_cis_get_analog_gain,
	.cis_get_min_analog_gain = sensor_2l4_cis_get_min_analog_gain,
	.cis_get_max_analog_gain = sensor_2l4_cis_get_max_analog_gain,
	.cis_set_digital_gain = sensor_2l4_cis_set_digital_gain,
	.cis_get_digital_gain = sensor_2l4_cis_get_digital_gain,
	.cis_get_min_digital_gain = sensor_2l4_cis_get_min_digital_gain,
	.cis_get_max_digital_gain = sensor_2l4_cis_get_max_digital_gain,
	.cis_compensate_gain_for_extremely_br = sensor_2l4_cis_compensate_gain_for_extremely_br,
	.cis_wait_streamoff = sensor_cis_wait_streamoff,
	.cis_wait_streamon = sensor_2l4_cis_wait_streamon,
	.cis_data_calculation = sensor_2l4_cis_data_calc,
	.cis_set_long_term_exposure = sensor_2l4_cis_long_term_exposure,
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	.cis_update_mipi_info = sensor_2l4_cis_update_mipi_info,
	.cis_get_mipi_clock_string = sensor_2l4_cis_get_mipi_clock_string,
#endif
#ifdef USE_CAMERA_EMBEDDED_HEADER
	.cis_get_frame_id = sensor_2l4_cis_get_frame_id,
#endif
	.cis_set_frs_control = sensor_2l4_cis_set_frs_control,
	.cis_set_super_slow_motion_roi = sensor_2l4_cis_set_super_slow_motion_roi,
	.cis_check_rev_on_init = sensor_cis_check_rev_on_init,
	.cis_set_super_slow_motion_threshold = sensor_2l4_cis_set_super_slow_motion_threshold,
	.cis_get_super_slow_motion_threshold = sensor_2l4_cis_get_super_slow_motion_threshold,
	.cis_set_initial_exposure = sensor_cis_set_initial_exposure,
	.cis_get_super_slow_motion_gmc = sensor_2l4_cis_get_super_slow_motion_gmc,
	.cis_get_super_slow_motion_frame_id = sensor_2l4_cis_get_super_slow_motion_frame_id,
	.cis_set_super_slow_motion_flicker = sensor_2l4_cis_set_super_slow_motion_flicker,
	.cis_get_super_slow_motion_md_threshold = sensor_2l4_cis_get_super_slow_motion_md_threshold,
	.cis_set_super_slow_motion_gmc_table_idx = sensor_2l4_cis_set_super_slow_motion_gmc_table_idx,
	.cis_set_super_slow_motion_gmc_block_with_md_low = sensor_2l4_cis_set_super_slow_motion_gmc_block_with_md_low,
	.cis_recover_stream_on = sensor_2l4_cis_recover_stream_on,
	.cis_set_factory_control = sensor_2l4_cis_set_factory_control,
};

static int cis_2l4_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct fimc_is_core *core = NULL;
	struct v4l2_subdev *subdev_cis = NULL;
	struct fimc_is_cis *cis = NULL;
	struct fimc_is_device_sensor *device = NULL;
	struct fimc_is_device_sensor_peri *sensor_peri = NULL;
	u32 sensor_id[FIMC_IS_STREAM_COUNT] = {0, };
	u32 sensor_id_len;
	const u32 *sensor_id_spec;
	char const *setfile;
	struct device *dev;
	struct device_node *dnode;
	int i;
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	int index;
#endif

	WARN_ON(!client);
	WARN_ON(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	sensor_id_spec = of_get_property(dnode, "id", &sensor_id_len);
	if (!sensor_id_spec) {
		err("sensor_id num read is fail(%d)", ret);
		goto p_err;
	}

	sensor_id_len /= (unsigned int)sizeof(*sensor_id_spec);

	probe_info("%s sensor_id_spec %d, sensor_id_len %d\n", __func__,
			*sensor_id_spec, sensor_id_len);

	ret = of_property_read_u32_array(dnode, "id", sensor_id, sensor_id_len);
	if (ret) {
		err("sensor_id read is fail(%d)", ret);
		goto p_err;
	}

	for (i = 0; i < sensor_id_len; i++) {
		probe_info("%s sensor_id %d\n", __func__, sensor_id[i]);
		device = &core->sensor[sensor_id[i]];

		sensor_peri = find_peri_by_cis_id(device, SENSOR_NAME_SAK2L4);
		if (!sensor_peri) {
			probe_info("sensor peri is net yet probed");
			return -EPROBE_DEFER;
		}
	}

	for (i = 0; i < sensor_id_len; i++) {
		device = &core->sensor[sensor_id[i]];
		sensor_peri = find_peri_by_cis_id(device, SENSOR_NAME_SAK2L4);

		cis = &sensor_peri->cis;
		subdev_cis = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
		if (!subdev_cis) {
			probe_err("subdev_cis is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		sensor_peri->subdev_cis = subdev_cis;

		cis->id = SENSOR_NAME_SAK2L4;
		cis->subdev = subdev_cis;
		cis->device = sensor_id[i];
		cis->client = client;
		sensor_peri->module->client = cis->client;
		cis->i2c_lock = NULL;
		cis->ctrl_delay = N_PLUS_TWO_FRAME;
#ifdef USE_CAMERA_FACTORY_DRAM_TEST
		cis->factory_dramtest_section2_fcount = SENSOR_2L4_DRAMTEST_SECTION2_FCOUNT;
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		cis->mipi_clock_index_cur = CAM_MIPI_NOT_INITIALIZED;
		cis->mipi_clock_index_new = CAM_MIPI_NOT_INITIALIZED;
#endif
		cis->cis_data = kzalloc(sizeof(cis_shared_data), GFP_KERNEL);
		if (!cis->cis_data) {
			err("cis_data is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		cis->cis_ops = &cis_ops_2l4;

		/* belows are depend on sensor cis. MUST check sensor spec */
		cis->bayer_order = OTF_INPUT_ORDER_BAYER_GR_BG;

		if (of_property_read_bool(dnode, "sensor_f_number")) {
			ret = of_property_read_u32(dnode, "sensor_f_number", &cis->aperture_num);
			if (ret)
				warn("f-number read is fail(%d)", ret);
		} else {
			cis->aperture_num = F1_5;
		}

		probe_info("%s f-number %d\n", __func__, cis->aperture_num);

		cis->use_dgain = true;
		cis->hdr_ctrl_by_again = false;

		v4l2_set_subdevdata(subdev_cis, cis);
		v4l2_set_subdev_hostdata(subdev_cis, device);
		snprintf(subdev_cis->name, V4L2_SUBDEV_NAME_SIZE, "cis-subdev.%d", cis->id);

		sensor_cis_parse_dt(dev, cis->subdev);
	}

	cis->use_initial_ae = of_property_read_bool(dnode, "use_initial_ae");
	probe_info("%s use initial_ae(%d)\n", __func__, cis->use_initial_ae);

	ret = of_property_read_string(dnode, "setfile", &setfile);
	if (ret) {
		err("setfile index read fail(%d), take default setfile!!", ret);
		setfile = "default";
	}

#ifndef USE_FLIP_WIDE_SENSOR
	if (strcmp(setfile, "default") == 0 ||
			strcmp(setfile, "setA") == 0) {
		probe_info("%s setfile_A\n", __func__);
		sensor_2l4_reset_tnp = sensor_2l4_setfile_A_Reset_TnP;
		sensor_2l4_reset_tnp_size  = ARRAY_SIZE(sensor_2l4_setfile_A_Reset_TnP);
		sensor_2l4_global = sensor_2l4_setfile_A_Global;
		sensor_2l4_global_size = ARRAY_SIZE(sensor_2l4_setfile_A_Global);
		sensor_2l4_dram_test_global = sensor_2l4_setfile_A_dram_test_Global;
		sensor_2l4_dram_test_global_size = ARRAY_SIZE(sensor_2l4_setfile_A_dram_test_Global);
		sensor_2l4_setfiles = sensor_2l4_setfiles_A;
		sensor_2l4_setfile_sizes = sensor_2l4_setfile_A_sizes;
		sensor_2l4_pllinfos = sensor_2l4_pllinfos_A;
		sensor_2l4_max_setfile_num = ARRAY_SIZE(sensor_2l4_setfiles_A);
#ifdef CONFIG_SENSOR_RETENTION_USE
		sensor_2l4_global_retention = sensor_2l4_setfile_A_Global_retention;
		sensor_2l4_global_retention_size = ARRAY_SIZE(sensor_2l4_setfile_A_Global_retention);
		sensor_2l4_retention = sensor_2l4_setfiles_A_retention;
		sensor_2l4_retention_size = sensor_2l4_setfile_A_sizes_retention;
		sensor_2l4_max_retention_num = ARRAY_SIZE(sensor_2l4_setfiles_A_retention);
		sensor_2l4_load_sram = sensor_2l4_setfile_A_load_sram;
		sensor_2l4_load_sram_size = sensor_2l4_setfile_A_sizes_load_sram;
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		sensor_2l4_mipi_sensor_mode = sensor_2l4_setfile_A_mipi_sensor_mode;
		sensor_2l4_mipi_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_A_mipi_sensor_mode);
		sensor_2l4_verify_sensor_mode = sensor_2l4_setfile_A_verify_sensor_mode;
		sensor_2l4_verify_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_A_verify_sensor_mode);
#endif
	} else {
		err("%s setfile index out of bound, take default (setfile_A)", __func__);
		sensor_2l4_reset_tnp = sensor_2l4_setfile_A_Reset_TnP;
		sensor_2l4_reset_tnp_size  = ARRAY_SIZE(sensor_2l4_setfile_A_Reset_TnP);
		sensor_2l4_global = sensor_2l4_setfile_A_Global;
		sensor_2l4_global_size = ARRAY_SIZE(sensor_2l4_setfile_A_Global);
		sensor_2l4_dram_test_global = sensor_2l4_setfile_A_dram_test_Global;
		sensor_2l4_dram_test_global_size = ARRAY_SIZE(sensor_2l4_setfile_A_dram_test_Global);
		sensor_2l4_setfiles = sensor_2l4_setfiles_A;
		sensor_2l4_setfile_sizes = sensor_2l4_setfile_A_sizes;
		sensor_2l4_pllinfos = sensor_2l4_pllinfos_A;
		sensor_2l4_max_setfile_num = ARRAY_SIZE(sensor_2l4_setfiles_A);
#ifdef CONFIG_SENSOR_RETENTION_USE
		sensor_2l4_global_retention = sensor_2l4_setfile_A_Global_retention;
		sensor_2l4_global_retention_size = ARRAY_SIZE(sensor_2l4_setfile_A_Global_retention);
		sensor_2l4_retention = sensor_2l4_setfiles_A_retention;
		sensor_2l4_retention_size = sensor_2l4_setfile_A_sizes_retention;
		sensor_2l4_max_retention_num = ARRAY_SIZE(sensor_2l4_setfiles_A_retention);
		sensor_2l4_load_sram = sensor_2l4_setfile_A_load_sram;
		sensor_2l4_load_sram_size = sensor_2l4_setfile_A_sizes_load_sram;
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		sensor_2l4_mipi_sensor_mode = sensor_2l4_setfile_A_mipi_sensor_mode;
		sensor_2l4_mipi_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_A_mipi_sensor_mode);
		sensor_2l4_verify_sensor_mode = sensor_2l4_setfile_A_verify_sensor_mode;
		sensor_2l4_verify_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_A_verify_sensor_mode);
#endif
	}
#else
	if (strcmp(setfile, "default") == 0 ||
			strcmp(setfile, "setB") == 0) {
		probe_info("%s setfile_B\n", __func__);
		sensor_2l4_reset_tnp = sensor_2l4_setfile_B_Reset_TnP;
		sensor_2l4_reset_tnp_size  = ARRAY_SIZE(sensor_2l4_setfile_B_Reset_TnP);
		sensor_2l4_global = sensor_2l4_setfile_B_Global;
		sensor_2l4_global_size = ARRAY_SIZE(sensor_2l4_setfile_B_Global);
		sensor_2l4_dram_test_global = sensor_2l4_setfile_B_dram_test_Global;
		sensor_2l4_dram_test_global_size = ARRAY_SIZE(sensor_2l4_setfile_B_dram_test_Global);
		sensor_2l4_setfiles = sensor_2l4_setfiles_B;
		sensor_2l4_setfile_sizes = sensor_2l4_setfile_B_sizes;
		sensor_2l4_pllinfos = sensor_2l4_pllinfos_B;
		sensor_2l4_max_setfile_num = ARRAY_SIZE(sensor_2l4_setfiles_B);
#ifdef CONFIG_SENSOR_RETENTION_USE
		sensor_2l4_global_retention = sensor_2l4_setfile_B_Global_retention;
		sensor_2l4_global_retention_size = ARRAY_SIZE(sensor_2l4_setfile_B_Global_retention);
		sensor_2l4_retention = sensor_2l4_setfiles_B_retention;
		sensor_2l4_retention_size = sensor_2l4_setfile_B_sizes_retention;
		sensor_2l4_max_retention_num = ARRAY_SIZE(sensor_2l4_setfiles_B_retention);
		sensor_2l4_load_sram = sensor_2l4_setfile_B_load_sram;
		sensor_2l4_load_sram_size = sensor_2l4_setfile_B_sizes_load_sram;
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		sensor_2l4_mipi_sensor_mode = sensor_2l4_setfile_B_mipi_sensor_mode;
		sensor_2l4_mipi_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_B_mipi_sensor_mode);
		sensor_2l4_verify_sensor_mode = sensor_2l4_setfile_B_verify_sensor_mode;
		sensor_2l4_verify_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_B_verify_sensor_mode);
#endif
	} else {
		err("%s setfile index out of bound, take default (setfile_B)", __func__);
		sensor_2l4_reset_tnp = sensor_2l4_setfile_B_Reset_TnP;
		sensor_2l4_reset_tnp_size  = ARRAY_SIZE(sensor_2l4_setfile_B_Reset_TnP);
		sensor_2l4_global = sensor_2l4_setfile_B_Global;
		sensor_2l4_global_size = ARRAY_SIZE(sensor_2l4_setfile_B_Global);
		sensor_2l4_dram_test_global = sensor_2l4_setfile_B_dram_test_Global;
		sensor_2l4_dram_test_global_size = ARRAY_SIZE(sensor_2l4_setfile_B_dram_test_Global);
		sensor_2l4_setfiles = sensor_2l4_setfiles_B;
		sensor_2l4_setfile_sizes = sensor_2l4_setfile_B_sizes;
		sensor_2l4_pllinfos = sensor_2l4_pllinfos_B;
		sensor_2l4_max_setfile_num = ARRAY_SIZE(sensor_2l4_setfiles_B);
#ifdef CONFIG_SENSOR_RETENTION_USE
		sensor_2l4_global_retention = sensor_2l4_setfile_B_Global_retention;
		sensor_2l4_global_retention_size = ARRAY_SIZE(sensor_2l4_setfile_B_Global_retention);
		sensor_2l4_retention = sensor_2l4_setfiles_B_retention;
		sensor_2l4_retention_size = sensor_2l4_setfile_B_sizes_retention;
		sensor_2l4_max_retention_num = ARRAY_SIZE(sensor_2l4_setfiles_B_retention);
		sensor_2l4_load_sram = sensor_2l4_setfile_B_load_sram;
		sensor_2l4_load_sram_size = sensor_2l4_setfile_B_sizes_load_sram;
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		sensor_2l4_mipi_sensor_mode = sensor_2l4_setfile_B_mipi_sensor_mode;
		sensor_2l4_mipi_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_B_mipi_sensor_mode);
		sensor_2l4_verify_sensor_mode = sensor_2l4_setfile_B_verify_sensor_mode;
		sensor_2l4_verify_sensor_mode_size = ARRAY_SIZE(sensor_2l4_setfile_B_verify_sensor_mode);
#endif
	}
#endif

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
	for (i = 0; i < sensor_2l4_verify_sensor_mode_size; i++) {
		index = sensor_2l4_verify_sensor_mode[i];

		if (fimc_is_vendor_verify_mipi_channel(sensor_2l4_mipi_sensor_mode[index].mipi_channel,
					sensor_2l4_mipi_sensor_mode[index].mipi_channel_size)) {
			panic("wrong mipi channel");
			break;
		}
	}
#endif
	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id sensor_cis_2l4_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-cis-2l4",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_cis_2l4_match);

static const struct i2c_device_id sensor_cis_2l4_idt[] = {
	{ SENSOR_NAME, 0 },
	{},
};

static struct i2c_driver sensor_cis_2l4_driver = {
	.probe	= cis_2l4_probe,
	.driver = {
		.name	= SENSOR_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = sensor_cis_2l4_match,
		.suppress_bind_attrs = true,
	},
	.id_table = sensor_cis_2l4_idt
};

static int __init sensor_cis_2l4_init(void)
{
	int ret;

	ret = i2c_add_driver(&sensor_cis_2l4_driver);
	if (ret)
		err("failed to add %s driver: %d\n",
			sensor_cis_2l4_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_cis_2l4_init);
