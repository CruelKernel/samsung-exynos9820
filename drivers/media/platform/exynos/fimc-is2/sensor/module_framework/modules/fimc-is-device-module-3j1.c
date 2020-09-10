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
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"

#include "fimc-is-device-module-base.h"

/* 3J1 sensor setting version - S5K3J1S_EVT0_Ver_0.10_180718_forVendor */
/* 0: 3648 x 2736 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 1: 2736 x 2736 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 2: 3968 x 2232 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 3: 3968 x 1880 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 4: 2944 x 2208 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 5: 3216 x 1808 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 6: 3216 x 1528 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 7: 2208 x 2208 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 8: 1824 x 1368 @30fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 9: 1988 x 1120 @120.4fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 10: 1988 x 1120 @240.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 11:  912 x  684 @120.4fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26 */
/* 12: 3648 x 2736 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26  PD None */
/* 13: 3968 x 2232 @30.1fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26  PD None */
/* 14: 1824 x 1368 @30fps MIPI lane: 4, MIPI data rate(Mbps/lane): 1807, MIPI Mclk(Mhz): 26  PD None */

static struct fimc_is_sensor_cfg config_module_3j1[] = {
			/* width, height, fps, settle, mode, lane, speed, interleave, pd_mode */
	FIMC_IS_SENSOR_CFG(3648, 2736, 30, 0, 0, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 3648, 2736), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3648, 2736),
		VC_IN(1, HW_FORMAT_RAW10, 3648, 684), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(2736, 2736, 30, 0, 1, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 2736, 2736), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 2736, 2736),
		VC_IN(1, HW_FORMAT_RAW10, 2736, 684), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(3968, 2232, 30, 0, 2, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 3968, 2232), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3968, 2232),
		VC_IN(1, HW_FORMAT_RAW10, 3968, 558), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(3968, 1880, 30, 0, 3, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 3968, 1880), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3968, 1880),
		VC_IN(1, HW_FORMAT_RAW10, 3968, 470), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(2944, 2208, 30, 0, 4, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 2944, 2208), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 2944, 2208),
		VC_IN(1, HW_FORMAT_RAW10, 2944, 552), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(3216, 1808, 30, 0, 5, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 3216, 1808), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3216, 1808),
		VC_IN(1, HW_FORMAT_RAW10, 3216, 452), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(3216, 1528, 30, 0, 6, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 3216, 1528), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3216, 1528),
		VC_IN(1, HW_FORMAT_RAW10, 3216, 382), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(2208, 2208, 30, 0, 7, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 2208, 2208), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 2208, 2208),
		VC_IN(1, HW_FORMAT_RAW10, 2208, 552), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(1824, 1368, 30, 0, 8, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 1824, 1368), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1824, 1368),
		VC_IN(1, HW_FORMAT_RAW10, 1824, 342), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(1988, 1120, 120, 0, 9, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 1988, 1120), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1988, 1120),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(1988, 1120, 240, 0, 10, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 1988, 1120), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1988, 1120),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(912, 684, 120, 0, 11, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 912, 684), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 912, 684),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG_EX(3648, 2736, 30, 0, 12, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE, EX_PDAF_OFF,
		VC_IN(0, HW_FORMAT_RAW10, 3648, 2736), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3648, 2736),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG_EX(3968, 2232, 30, 0, 13, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE, EX_PDAF_OFF,
		VC_IN(0, HW_FORMAT_RAW10, 3968, 2232), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3968, 2232),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG_EX(1824, 1368, 30, 0, 14, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE, EX_PDAF_OFF,
		VC_IN(0, HW_FORMAT_RAW10, 1824, 1368), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1824, 1368),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(1472, 1104, 30, 0, 15, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 1472, 1104), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1472, 1104),
		VC_IN(1, HW_FORMAT_RAW10, 1472, 276), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(1616, 904, 30, 0, 16, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 1616, 904), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1616, 904),
		VC_IN(1, HW_FORMAT_RAW10, 1616, 226), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(1616, 768, 30, 0, 17, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 1616, 768), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1616, 768),
		VC_IN(1, HW_FORMAT_RAW10, 1616, 192), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	FIMC_IS_SENSOR_CFG(1104, 1104, 30, 0, 18, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 1104, 1104), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 1104, 1104),
		VC_IN(1, HW_FORMAT_RAW10, 1104, 276), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
	/* 2PD LR RAW : Face Recognition */
	FIMC_IS_SENSOR_CFG_EX(3648, 2736, 30, 0, 0, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_NONE, EX_PDSTAT_OFF,
		VC_IN(0, HW_FORMAT_RAW10, 3648, 2736), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3648, 2736),
		VC_IN(1, HW_FORMAT_RAW10, 3648, 684), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3648, 684),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(3648, 2736, 60, 0, 19, CSI_DATA_LANES_4, 1872, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 3648, 2736), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3648, 2736),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(3968, 2232, 60, 0, 20, CSI_DATA_LANES_4, 1872, CSI_MODE_VC_ONLY, PD_NONE,
		VC_IN(0, HW_FORMAT_RAW10, 3968, 2232), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3968, 2232),
		VC_IN(1, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_UNKNOWN, VC_NOTHING, 0, 0)),
	FIMC_IS_SENSOR_CFG(3216, 2208, 30, 0, 21, CSI_DATA_LANES_4, 1807, CSI_MODE_VC_ONLY, PD_MOD3,
		VC_IN(0, HW_FORMAT_RAW10, 3216, 2208), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 3216, 2208),
		VC_IN(1, HW_FORMAT_RAW10, 3216, 552), VC_OUT(HW_FORMAT_RAW10, VC_NOTHING, 0, 0),
		VC_IN(2, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_MIPISTAT, 39424, 1),
		VC_IN(3, HW_FORMAT_UNKNOWN, 0, 0), VC_OUT(HW_FORMAT_USER, VC_PRIVATE, 1756, 1)),
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

static int sensor_module_3j1_power_setpin(struct device *dev,
		struct exynos_platform_fimc_is_module *pdata)
{
	struct fimc_is_core *core;
	struct device_node *dnode;
	int gpio_none = 0, gpio_reset = 0;
	int gpio_mclk = 0;
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

	gpio_reset = of_get_named_gpio(dnode, "gpio_reset", 0);
	if (gpio_is_valid(gpio_reset)) {
		gpio_request_one(gpio_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_reset);
	} else {
		dev_err(dev, "%s: failed to get PIN_RESET\n", __func__);
		return -EINVAL;
	}

	gpio_mclk = of_get_named_gpio(dnode, "gpio_mclk", 0);
	if (gpio_is_valid(gpio_mclk)) {
		gpio_request_one(gpio_mclk, GPIOF_OUT_INIT_LOW, "CAM_MCLK_OUTPUT_LOW");
		gpio_free(gpio_mclk);
	} else {
		dev_err(dev, "%s: failed to get mclk\n", __func__);
		return -EINVAL;
	}

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

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF);

	/********** FRONT CAMERA  - POWER ON **********/
#ifdef USE_TOF_IO_DENOISE_FRONT_CAMERA_IO
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_TOF", PIN_REGULATOR, 1, 0);
#endif
#ifdef USE_BUCK2_REGULATOR_CONTROL
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDD_EXT_1P3_PB02", PIN_REGULATOR, 1, 0);
#endif
	if (gpio_is_valid(gpio_subcam_sel)) {
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_subcam_sel, "gpio_subcam_sel high", PIN_OUTPUT, 0, 2000);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDAF_2.8V_VT", PIN_REGULATOR, 1, 0);
	if (power_seq_id == 0) {
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDA_2.95V_VT", PIN_REGULATOR, 1, 0);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_VT", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_1.05V_VT", PIN_REGULATOR, 1, 1000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "on_i2c", PIN_I2C, 1, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_reset, "rst_high", PIN_OUTPUT, 1, 0);
	/* Mclock enable */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 2, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
			&core->shared_rsc_slock[SHARED_PIN5], &core->shared_rsc_count[SHARED_PIN5], 1);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "MCLK", PIN_MCLK, 1, 9000);
	if (use_mclk_share) {
		SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, SRT_ACQUIRE,
				&core->shared_rsc_slock[SHARED_PIN9], &core->shared_rsc_count[SHARED_PIN9], 1);
	}

	/*********** FRONT CAMERA  - POWER OFF **********/
	if (gpio_is_valid(gpio_subcam_sel)) {
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_subcam_sel, "gpio_subcam_sel low", PIN_OUTPUT, 0, 9);
	}
	/* Mclock disable */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "MCLK", PIN_MCLK, 0, 1);
	if (use_mclk_share) {
		SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
				&core->shared_rsc_slock[SHARED_PIN9], &core->shared_rsc_count[SHARED_PIN9], 0);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN_SHARED(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, SRT_RELEASE,
			&core->shared_rsc_slock[SHARED_PIN5], &core->shared_rsc_count[SHARED_PIN5], 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_reset, "rst_low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDAF_2.8V_VT", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_1.05V_VT", PIN_REGULATOR, 0, 0);
	if (power_seq_id == 0) {
		SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDA_2.95V_VT", PIN_REGULATOR, 0, 0);
	}
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_VT", PIN_REGULATOR, 0, 0);
#ifdef USE_BUCK2_REGULATOR_CONTROL
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDD_EXT_1P3_PB02", PIN_REGULATOR, 0, 0);
#endif
#ifdef USE_TOF_IO_DENOISE_FRONT_CAMERA_IO
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_TOF", PIN_REGULATOR, 0, 0);
#endif
	dev_info(dev, "%s X v4\n", __func__);

	/******************** READ_ROM - POWER ON ********************/
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_VT", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON, gpio_none, "on_i2c", PIN_I2C, 1, 10);

	/******************** READ_ROM - POWER OFF ********************/
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_none, "off_i2c", PIN_I2C, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_VT", PIN_REGULATOR, 0, 0);

	return 0;
}

static int __init sensor_module_3j1_probe(struct platform_device *pdev)
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

	fimc_is_module_parse_dt(dev, sensor_module_3j1_power_setpin);

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
	module->sensor_id = SENSOR_NAME_S5K3J1;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = NULL;
	module->active_width = 3968;
	module->active_height = 2736;
	module->margin_left = 0;
	module->margin_right = 0;
	module->margin_top = 0;
	module->margin_bottom = 0;
	module->pixel_width = module->active_width;
	module->pixel_height = module->active_height;
	module->max_framerate = 240;
	module->position = pdata->position;
	module->bitwidth = 10;
	module->sensor_maker = "SLSI";
	module->sensor_name = "S5K3J1";
	module->setfile_name = "setfile_3j1.bin";
	module->cfgs = ARRAY_SIZE(config_module_3j1);
	module->cfg = config_module_3j1;
	module->ops = NULL;

	for (t = VC_BUF_DATA_TYPE_SENSOR_STAT1; t < VC_BUF_DATA_TYPE_MAX; t++) {
		module->vc_extra_info[t].stat_type = VC_STAT_TYPE_INVALID;
		module->vc_extra_info[t].sensor_mode = VC_SENSOR_MODE_INVALID;
		module->vc_extra_info[t].max_width = 0;
		module->vc_extra_info[t].max_height = 0;
		module->vc_extra_info[t].max_element = 0;

		if (IS_ENABLED(CONFIG_CAMERA_PDP)) {
			switch (t) {
			case VC_BUF_DATA_TYPE_GENERAL_STAT1:
				/* PDP STAT0: SFR, Size: 1756(1756)Bytes */
				if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
						!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0))
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_1_PDAF_STAT0;
				else
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_0_PDAF_STAT0;

				module->vc_extra_info[t].sensor_mode = VC_SENSOR_MODE_2PD_MODE3;
				module->vc_extra_info[t].max_width = 1756;
				module->vc_extra_info[t].max_height = 1;
				module->vc_extra_info[t].max_element = 1;
				break;
			case VC_BUF_DATA_TYPE_GENERAL_STAT2:
				/* PDP STAT1/2: DMA, Size: 17920+21504(17556+21168)Bytes */
				if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
						!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0))
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_1_PDAF_STAT1;
				else
					module->vc_extra_info[t].stat_type
						= VC_STAT_TYPE_PDP_1_0_PDAF_STAT1;

				module->vc_extra_info[t].sensor_mode = VC_SENSOR_MODE_2PD_MODE3;
				module->vc_extra_info[t].max_width = 39424;
				module->vc_extra_info[t].max_height = 1;
				module->vc_extra_info[t].max_element = 1;
				break;
			}
		}
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
	ext->sensor_con.peri_setting.i2c.speed = 1000000;

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

static const struct of_device_id exynos_fimc_is_sensor_module_3j1_match[] = {
	{
		.compatible = "samsung,sensor-module-3j1",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_module_3j1_match);

static struct platform_driver sensor_module_3j1_driver = {
	.driver = {
		.name   = "FIMC-IS-SENSOR-MODULE-3J1",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_module_3j1_match,
	}
};

static int __init fimc_is_sensor_module_3j1_init(void)
{
	int ret;

	ret = platform_driver_probe(&sensor_module_3j1_driver,
				sensor_module_3j1_probe);
	if (ret)
		err("failed to probe %s driver: %d\n",
			sensor_module_3j1_driver.driver.name, ret);

	return ret;
}
late_initcall(fimc_is_sensor_module_3j1_init);
