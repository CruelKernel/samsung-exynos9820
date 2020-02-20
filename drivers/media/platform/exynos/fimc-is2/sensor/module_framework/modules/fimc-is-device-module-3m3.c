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
#include <linux/of_gpio.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-hw.h"
#include "fimc-is-core.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"

#include "fimc-is-device-module-base.h"

static struct fimc_is_sensor_cfg config_module_3m3[] = {
			/* width, height, fps, settle, mode, lane, speed, interleave, pd_mode */
	FIMC_IS_SENSOR_CFG(4032, 3024, 30, 0, 0, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4032, 3024), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 128, 736), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 128, 736),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(4032, 2268, 30, 0, 1, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4032, 2268), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 128, 544), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 128, 544),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(4032, 1960, 30, 0, 2, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4032, 1960), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 128, 480), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 128, 480),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(4032, 1908, 30, 0, 3, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 4032, 1908), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 128, 448), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 128, 448),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(3024, 3024, 30, 0, 4, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 3024, 3024), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 96, 736), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 96, 736),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(2016, 1512, 30, 0, 5, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 2016, 1512), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 128, 736), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 128, 736),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(1504, 1504, 30, 0, 6, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 1504, 1504), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 96, 704), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 96, 704),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(1920, 1080, 60, 0, 7, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 1920, 1080), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 60, 256), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 60, 256),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(1344, 756, 120, 0, 8, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 1344, 756), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(2016, 1134, 30, 0, 9, CSI_DATA_LANES_4, 1196, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 2016, 1134), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(1, HW_FORMAT_RAW10, 128, 544), VC_OUT(HW_FORMAT_RAW10, VC_TAILPDAF, 128, 544),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_module_init,
	.g_ctrl = sensor_module_g_ctrl,
	.s_ctrl = sensor_module_s_ctrl,
	.g_ext_ctrls = sensor_module_g_ext_ctrls,
	.s_ext_ctrls = sensor_module_s_ext_ctrls,
	.ioctl = sensor_module_ioctl,
	.log_status = sensor_module_log_status,
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_routing = sensor_module_s_routing,
	.s_stream = sensor_module_s_stream,
	.s_parm = sensor_module_s_param
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_module_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

static int sensor_module_3m3_power_setpin(struct device *dev,
	struct exynos_platform_fimc_is_module *pdata)
{
	struct fimc_is_core *core;
	struct device_node *dnode;
	int gpio_reset = 0;
#ifdef CONFIG_OIS_USE
	int gpio_ois_reset = 0;
#endif
	int gpio_none = 0;
	int gpio_subcam_sel = 0;
	u32 power_seq_id = 0;
	bool use_mclk_share = false;

	FIMC_BUG(!dev);

	dnode = dev->of_node;
	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
			err("core is NULL");
			return -EINVAL;
	}

	dev_info(dev, "%s E v4\n", __func__);

	gpio_reset = of_get_named_gpio(dnode, "gpio_reset", 0);
	if (!gpio_is_valid(gpio_reset)) {
		dev_err(dev, "failed to get gpio_reset\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_reset);
	}
#ifdef CONFIG_OIS_USE
	gpio_ois_reset = of_get_named_gpio(dnode, "gpio_ois_reset", 0);
	if (!gpio_is_valid(gpio_ois_reset)) {
		dev_err(dev, "failed to get gpio_ois_reset\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_ois_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_ois_reset);
	}
#endif

	gpio_subcam_sel = of_get_named_gpio(dnode, "gpio_subcam_sel", 0);
	if (!gpio_is_valid(gpio_subcam_sel)) {
		dev_err(dev, "failed to get gpio_subcam_sel\n");
	} else {
		gpio_request_one(gpio_subcam_sel, GPIOF_OUT_INIT_LOW, "SUBCAM_SEL");
		gpio_free(gpio_subcam_sel);
	}

	if (of_property_read_u32(dnode, "power_seq_id", &power_seq_id)) {
		dev_err(dev, "power_seq_id read is fail");
		power_seq_id = 0;
	}

	use_mclk_share = of_property_read_bool(dnode, "use_mclk_share");
	if (use_mclk_share) {
		dev_info(dev, "use_mclk_share(%d)", use_mclk_share);
	}

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF);

	/* TELE CAMERA - POWER ON */
#ifdef USE_BUCK2_REGULATOR_CONTROL
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDD_EXT_1P3_PB02", PIN_REGULATOR, 1, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN7], &core->shared_rsc_count[SHARED_PIN7], 1);
#endif
	if (gpio_is_valid(gpio_subcam_sel))
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON,
				gpio_subcam_sel, "gpio_subcam_sel high", PIN_OUTPUT, 1, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDA_2.8V_SUB", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_1.05V_SUB", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDAF_2.8V_SUB",
		PIN_REGULATOR, 1, 2000);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDD_VM_2.8V_OIS", PIN_REGULATOR, 1, 100);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN3], &core->shared_rsc_count[SHARED_PIN3], 1);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_1.8V_OIS", PIN_REGULATOR, 1, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN2], &core->shared_rsc_count[SHARED_PIN2], 1);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 1, 100);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN6], &core->shared_rsc_count[SHARED_PIN6], 1);
	if (power_seq_id == 1) {
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_FRONTSUB", PIN_REGULATOR, 1, 100);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_SUB", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "on_i2c", PIN_I2C, 1, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 2, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN0], &core->shared_rsc_count[SHARED_PIN0], 1);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "MCLK", PIN_MCLK, 1, 0);
	if (use_mclk_share) {
		SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
				&core->shared_rsc_slock[SHARED_PIN8], &core->shared_rsc_count[SHARED_PIN8], 1);
	}

	/* 10ms delay is needed for I2C communication of the AK7371 actuator */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 8000);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_ois_reset, "ois_rst high", PIN_OUTPUT, 1, 20000);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN1], &core->shared_rsc_count[SHARED_PIN1], 1);
#endif

	/* TELE CAMERA - POWER OFF */
	if (gpio_is_valid(gpio_subcam_sel))
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF,
				gpio_subcam_sel, "gpio_subcam_sel low", PIN_OUTPUT, 0, 9);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDAF_2.8V_SUB",
		PIN_REGULATOR, 0, 9);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
		&core->shared_rsc_slock[SHARED_PIN6], &core->shared_rsc_count[SHARED_PIN6], 0);
	if (power_seq_id == 1) {
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_FRONTSUB", PIN_REGULATOR, 0, 0);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_SUB", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDA_2.8V_SUB", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_1.05V_SUB",
		PIN_REGULATOR, 0, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_ois_reset, "ois_rst low", PIN_OUTPUT, 0, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN1], &core->shared_rsc_count[SHARED_PIN1], 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_1.8V_OIS", PIN_REGULATOR, 0, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN2], &core->shared_rsc_count[SHARED_PIN2], 0);
#endif
	/* Mclock disable */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "MCLK", PIN_MCLK, 0, 0);
	if (use_mclk_share) {
		SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
				&core->shared_rsc_slock[SHARED_PIN8], &core->shared_rsc_count[SHARED_PIN8], 0);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN0], &core->shared_rsc_count[SHARED_PIN0], 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDD_VM_2.8V_OIS", PIN_REGULATOR, 0, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN3], &core->shared_rsc_count[SHARED_PIN3], 0);
#endif
#ifdef USE_BUCK2_REGULATOR_CONTROL
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDD_EXT_1P3_PB02", PIN_REGULATOR, 0, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN7], &core->shared_rsc_count[SHARED_PIN7], 0);
#endif

	/* VISION - POWER ON */
#ifdef USE_BUCK2_REGULATOR_CONTROL
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDD_EXT_1P3_PB02", PIN_REGULATOR, 1, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN7], &core->shared_rsc_count[SHARED_PIN7], 1);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDDA_2.8V_SUB", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDDD_1.05V_SUB", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDDAF_2.8V_SUB",
		PIN_REGULATOR, 1, 2000);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDDD_1.8V_OIS", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDD_VM_2.8V_OIS", PIN_REGULATOR, 1, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_SUB",
		PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "on_i2c", PIN_I2C, 1, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 2, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_none, "MCLK", PIN_MCLK, 1, 0);
	/* 10ms delay is needed for I2C communication of the AK7371 actuator */
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 8000);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_ON, gpio_ois_reset, "ois_rst high", PIN_OUTPUT, 1, 20000);
#endif

	/* TELE CAMERA - POWER OFF */
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDDAF_2.8V_SUB",
		PIN_REGULATOR, 0, 9);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_SUB",
		PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDDA_2.8V_SUB", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDDD_1.05V_SUB",
		PIN_REGULATOR, 0, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_ois_reset, "ois_rst low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDD_VM_2.8V_OIS", PIN_REGULATOR, 0, 0);
#endif
	/* Mclock disable */
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "MCLK", PIN_MCLK, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDDD_1.8V_OIS", PIN_REGULATOR, 0, 0);
#endif
#ifdef USE_BUCK2_REGULATOR_CONTROL
	SET_PIN(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, gpio_none, "VDD_EXT_1P3_PB02", PIN_REGULATOR, 0, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_VISION, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN7], &core->shared_rsc_count[SHARED_PIN7], 0);
#endif

	dev_info(dev, "%s X v4\n", __func__);

	return 0;
}

static int __init sensor_module_3m3_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	struct exynos_platform_fimc_is_module *pdata;
	struct device *dev;
	int t;
	struct pinctrl_state *s;

	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_info("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &pdev->dev;

	fimc_is_module_parse_dt(dev, sensor_module_3m3_power_setpin);

	pdata = dev_get_platdata(dev);
	device = &core->sensor[pdata->id];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	probe_info("%s pdta->id(%d), module_enum id = %d \n", __func__, pdata->id, atomic_read(&device->module_count));
	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_S5K3M3;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = NULL;
	module->active_width = 4032;
	module->active_height = 3024;
	module->margin_left = 0;
	module->margin_right = 0;
	module->margin_top = 0;
	module->margin_bottom = 0;
	module->pixel_width = module->active_width;
	module->pixel_height = module->active_height;
	module->max_framerate = 120;
	module->position = pdata->position;
	module->bitwidth = 10;
	module->sensor_maker = "SLSI";
	module->sensor_name = "S5K3M3";
	module->setfile_name = "setfile_3m3.bin";
	module->cfgs = ARRAY_SIZE(config_module_3m3);
	module->cfg = config_module_3m3;
	module->ops = NULL;

	for (t = VC_BUF_DATA_TYPE_SENSOR_STAT1; t < VC_BUF_DATA_TYPE_MAX; t++) {
		module->vc_extra_info[t].stat_type = VC_STAT_TYPE_INVALID;
		module->vc_extra_info[t].sensor_mode = VC_SENSOR_MODE_INVALID;
		module->vc_extra_info[t].max_width = 0;
		module->vc_extra_info[t].max_height = 0;
		module->vc_extra_info[t].max_element = 0;

/* HACK: disable tail mode with PDP */
#if 0
		if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
			switch (t) {
			case VC_BUF_DATA_TYPE_SENSOR_STAT1:
				if (!IS_ENABLED(CONFIG_SOC_EXYNOS9820) ||
						IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_TAIL_FOR_SW_PDAF;
					module->vc_extra_info[t].sensor_mode
						= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
					module->vc_extra_info[t].max_width = 128;
					module->vc_extra_info[t].max_height = 736;
					module->vc_extra_info[t].max_element = 2;
				}
				break;
			case VC_BUF_DATA_TYPE_GENERAL_STAT1:
				/* PDP STAT0: SFR, Size: 1756(1756)Bytes */
				if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
						!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_1_PDAF_STAT0;
					module->vc_extra_info[t].sensor_mode
						= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
					module->vc_extra_info[t].max_width = 1756;
					module->vc_extra_info[t].max_height = 1;
					module->vc_extra_info[t].max_element = 1;
				}
				break;
			case VC_BUF_DATA_TYPE_GENERAL_STAT2:
				/* PDP STAT1/2: DMA, Size: 17920+21504(17556+21168)Bytes */
				if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
						!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_1_PDAF_STAT1;
					module->vc_extra_info[t].sensor_mode
						= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
					module->vc_extra_info[t].max_width = 39424;
					module->vc_extra_info[t].max_height = 1;
					module->vc_extra_info[t].max_element = 1;
				}
				break;
			}
		} else {
			switch (t) {
			case VC_BUF_DATA_TYPE_SENSOR_STAT1:
				module->vc_extra_info[t].stat_type
					= VC_STAT_TYPE_TAIL_FOR_SW_PDAF;
				module->vc_extra_info[t].sensor_mode
					= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
				module->vc_extra_info[t].max_width = 128;
				module->vc_extra_info[t].max_height = 736;
				module->vc_extra_info[t].max_element = 2;
				break;
			}
		}
#else
		switch (t) {
			case VC_BUF_DATA_TYPE_SENSOR_STAT1:
				module->vc_extra_info[t].stat_type
					= VC_STAT_TYPE_TAIL_FOR_SW_PDAF;
				module->vc_extra_info[t].sensor_mode
					= VC_SENSOR_MODE_MSPD_GLOBAL_TAIL;
				module->vc_extra_info[t].max_width = 128;
				module->vc_extra_info[t].max_height = 736;
				module->vc_extra_info[t].max_element = 2;
				break;
		}
#endif
	}

	/* Sensor peri */
	module->private_data = kzalloc(sizeof(struct fimc_is_device_sensor_peri), GFP_KERNEL);
	if (!module->private_data) {
		probe_err("fimc_is_device_sensor_peri is NULL");
		ret = -ENOMEM;
		goto p_err;
	}
	fimc_is_sensor_peri_probe((struct fimc_is_device_sensor_peri*)module->private_data);
	PERI_SET_MODULE(module);

	ext = &module->ext;
#ifdef CONFIG_SENSOR_RETENTION_USE
	ext->use_retention_mode = SENSOR_RETENTION_UNSUPPORTED;
#endif
	ext->sensor_con.product_name = module->sensor_id;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = pdata->sensor_i2c_ch;
	ext->sensor_con.peri_setting.i2c.slave_address = pdata->sensor_i2c_addr;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	if (pdata->af_product_name !=  ACTUATOR_NAME_NOTHING) {
		ext->actuator_con.product_name = pdata->af_product_name;
		ext->actuator_con.peri_type = SE_I2C;
		ext->actuator_con.peri_setting.i2c.channel = pdata->af_i2c_ch;
		ext->actuator_con.peri_setting.i2c.slave_address = pdata->af_i2c_addr;
		ext->actuator_con.peri_setting.i2c.speed = 400000;
	}

	if (pdata->flash_product_name != FLADRV_NAME_NOTHING) {
		ext->flash_con.product_name = pdata->flash_product_name;
		ext->flash_con.peri_type = SE_GPIO;
		ext->flash_con.peri_setting.gpio.first_gpio_port_no = pdata->flash_first_gpio;
		ext->flash_con.peri_setting.gpio.second_gpio_port_no = pdata->flash_second_gpio;
	}

	ext->from_con.product_name = FROMDRV_NAME_NOTHING;

	if (pdata->preprocessor_product_name != PREPROCESSOR_NAME_NOTHING) {
		ext->preprocessor_con.product_name = pdata->preprocessor_product_name;
		ext->preprocessor_con.peri_info0.valid = true;
		ext->preprocessor_con.peri_info0.peri_type = SE_SPI;
		ext->preprocessor_con.peri_info0.peri_setting.spi.channel = pdata->preprocessor_spi_channel;
		ext->preprocessor_con.peri_info1.valid = true;
		ext->preprocessor_con.peri_info1.peri_type = SE_I2C;
		ext->preprocessor_con.peri_info1.peri_setting.i2c.channel = pdata->preprocessor_i2c_ch;
		ext->preprocessor_con.peri_info1.peri_setting.i2c.slave_address = pdata->preprocessor_i2c_addr;
		ext->preprocessor_con.peri_info1.peri_setting.i2c.speed = 400000;
		ext->preprocessor_con.peri_info2.valid = true;
		ext->preprocessor_con.peri_info2.peri_type = SE_DMA;
		ext->preprocessor_con.peri_info2.peri_setting.dma.channel = FLITE_ID_D;
	} else {
		ext->preprocessor_con.product_name = pdata->preprocessor_product_name;
	}

	if (pdata->ois_product_name != OIS_NAME_NOTHING) {
		ext->ois_con.product_name = pdata->ois_product_name;
		ext->ois_con.peri_type = SE_I2C;
		ext->ois_con.peri_setting.i2c.channel = pdata->ois_i2c_ch;
		ext->ois_con.peri_setting.i2c.slave_address = pdata->ois_i2c_addr;
		ext->ois_con.peri_setting.i2c.speed = 400000;
	} else {
		ext->ois_con.product_name = pdata->ois_product_name;
		ext->ois_con.peri_type = SE_NULL;
	}

	if (pdata->mcu_product_name != MCU_NAME_NOTHING) {
		ext->mcu_con.product_name = pdata->mcu_product_name;
		ext->mcu_con.peri_type = SE_I2C;
		ext->mcu_con.peri_setting.i2c.channel = pdata->mcu_i2c_ch;
		ext->mcu_con.peri_setting.i2c.slave_address = pdata->mcu_i2c_addr;
		ext->mcu_con.peri_setting.i2c.speed = 400000;
	} else {
		ext->mcu_con.product_name = pdata->mcu_product_name;
		ext->mcu_con.peri_type = SE_NULL;
	}

	v4l2_subdev_init(subdev_module, &subdev_ops);

	v4l2_set_subdevdata(subdev_module, module);
	v4l2_set_subdev_hostdata(subdev_module, device);
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE, "sensor-subdev.%d", module->sensor_id);

	s = pinctrl_lookup_state(pdata->pinctrl, "release");

	if (pinctrl_select_state(pdata->pinctrl, s) < 0) {
		probe_err("pinctrl_select_state is fail\n");
		goto p_err;
	}

	probe_info("%s done\n", __func__);

p_err:
	return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_module_3m3_match[] = {
	{
		.compatible = "samsung,sensor-module-3m3",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_module_3m3_match);

static struct platform_driver sensor_module_3m3_driver = {
	.driver = {
		.name   = "FIMC-IS-SENSOR-MODULE-3M3",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_module_3m3_match,
	}
};

static int __init fimc_is_sensor_module_3m3_init(void)
{
	int ret;

	ret = platform_driver_probe(&sensor_module_3m3_driver,
				sensor_module_3m3_probe);
	if (ret)
		err("failed to probe %s driver: %d\n",
			sensor_module_3m3_driver.driver.name, ret);

	return ret;
}
late_initcall(fimc_is_sensor_module_3m3_init);
