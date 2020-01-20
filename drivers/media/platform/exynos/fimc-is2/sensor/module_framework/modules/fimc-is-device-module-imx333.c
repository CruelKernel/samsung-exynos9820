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
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-resourcemgr.h"
#include "fimc-is-dt.h"

#include "fimc-is-device-module-base.h"

#define IMX333_PDAF_MAXWIDTH	0 /* MAX witdh size */
#define IMX333_PDAF_MAXHEIGHT	0 /* MAX height size */
#define IMX333_PDAF_ELEMENT	0 /* V4L2_PIX_FMT_SBGGR16 */

#define IMX333_MIPI_MAXWIDTH	4032 /* MAX width size */
#define IMX333_MIPI_MAXHEIGHT	14 /* MAX height size */
#define IMX333_MIPI_ELEMENT	1 /* V4L2_PIX_FMT_SBGGR16 */

static struct fimc_is_sensor_cfg config_imx333[] = {
	/* 4032x3024@30fps */
	FIMC_IS_SENSOR_CFG_EXT(4032, 3024,  30, 14, 0, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 4032, 14), 0),
	/* 4032x2268@30fps */
	FIMC_IS_SENSOR_CFG_EXT(4032, 2268,  30, 14, 1, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 4032, 14), 0),
	/* 3024x3024@30fps */
	FIMC_IS_SENSOR_CFG_EXT(3024, 3024,  30, 14, 2, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 3024, 19), 0),
	/* 4032x2268@60fps */
	FIMC_IS_SENSOR_CFG_EXT(4032, 2268,  60, 21, 3, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 4032, 4), 0),
	/* 2016x1134@120fps */
	FIMC_IS_SENSOR_CFG_EXT(2016, 1134, 120, 21, 4, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 2016, 8), 0),
	/* 2016x1134@240fps */
	FIMC_IS_SENSOR_CFG_EXT(2016, 1134, 240, 21, 5, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 2016, 8), 0),
	/* 1008x 756@120fps */
	FIMC_IS_SENSOR_CFG_EXT(1008,  756, 120, 21, 6, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 1008, 1), 0),
	/* 2016x 1512@30fps */
	FIMC_IS_SENSOR_CFG_EXT(2016, 1512,  30, 14, 7, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 2016, 8), 0), 
	/* 1504x 1504@30fps */
	FIMC_IS_SENSOR_CFG_EXT(1504, 1504,  30, 14, 8, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 1504, 11), 0),
	/* 2016x 1134@30fps */
	FIMC_IS_SENSOR_CFG_EXT(2016, 1134,  30, 14, 9, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 2016, 8), 0),
	/* 4032x 1960@30fps */
	FIMC_IS_SENSOR_CFG_EXT(4032, 1960,  30, 14, 10, CSI_DATA_LANES_4, 2145, 0, SET_VC(VC_MIPI_STAT, 4032, 14), 0),
};

static struct fimc_is_vci vci_imx333[] = {
	{
		.pixelformat = V4L2_PIX_FMT_SBGGR10,
		.config = {{0, HW_FORMAT_RAW10, VCI_DMA_NORMAL},
                           {1, HW_FORMAT_RAW10, VCI_DMA_NORMAL},
                           {0, HW_FORMAT_USER, VCI_DMA_INTERNAL},
                           {3, 0, VCI_DMA_NORMAL}}
	}, {
		.pixelformat = V4L2_PIX_FMT_SBGGR12,
		.config = {{0, HW_FORMAT_RAW10, VCI_DMA_NORMAL},
                           {1, HW_FORMAT_RAW10, VCI_DMA_NORMAL},
                           {0, HW_FORMAT_USER, VCI_DMA_INTERNAL},
                           {3, 0, VCI_DMA_NORMAL}}
	}, {
		.pixelformat = V4L2_PIX_FMT_SBGGR16,
		.config = {{0, HW_FORMAT_RAW10, VCI_DMA_NORMAL},
                           {1, HW_FORMAT_RAW10, VCI_DMA_NORMAL},
                           {0, HW_FORMAT_USER, VCI_DMA_INTERNAL},
                           {3, 0, VCI_DMA_NORMAL}}
	}
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
	//.s_mbus_fmt = sensor_module_s_format,
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = sensor_module_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

#ifdef CONFIG_OF
static int sensor_imx333_power_setpin(struct platform_device *pdev,
	struct exynos_platform_fimc_is_module *pdata)
{
	struct device *dev;
	struct device_node *dnode;
	int gpio_reset = 0;
#ifdef CAMERA_REAR2_AF
	int gpio_cam_af_2p8_en = 0; //SUB CAM AF 2.8
#endif
	int gpio_prep_reset = 0;
	int gpio_none = 0;
	int gpio_camio_1p8_en = 0;
	int gpio_ois_reset = 0;
	u32 power_seq_id = 0;
	int ret;

	FIMC_BUG(!pdev);

	dev = &pdev->dev;
	dnode = dev->of_node;

	dev_info(dev, "%s E v4\n", __func__);

#ifdef CAMERA_REAR2_AF
	gpio_cam_af_2p8_en = of_get_named_gpio(dnode, "gpio_cam_af_2p8_en", 0);
	if (!gpio_is_valid(gpio_cam_af_2p8_en)) {
		dev_err(dev, "failed to get gpio_cam_af_2p8_en\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_cam_af_2p8_en, GPIOF_OUT_INIT_LOW, "SUBCAM_AF_2P8_EN");
		gpio_free(gpio_cam_af_2p8_en);
	}
#endif

	gpio_prep_reset = of_get_named_gpio(dnode, "gpio_prep_reset", 0);
	if (!gpio_is_valid(gpio_prep_reset)) {
		dev_err(dev, "failed to get main comp reset gpio\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_prep_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_prep_reset);
	}

	gpio_reset = of_get_named_gpio(dnode, "gpio_reset", 0);
	if (!gpio_is_valid(gpio_reset)) {
		dev_err(dev, "failed to get PIN_RESET\n");
		return -EINVAL;
	} else {
		gpio_request_one(gpio_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
		gpio_free(gpio_reset);
	}

        /* Check for support Rumba-S6 */
	ret = of_property_read_u32(dnode, "power_seq_id", &power_seq_id);
	if (ret) {
		dev_err(dev, "power_seq_id read is fail(%d)", ret);
		power_seq_id = 0;
	}
       
    if (power_seq_id == 1) {
        gpio_ois_reset = of_get_named_gpio(dnode, "gpio_ois_reset", 0);
        if (gpio_is_valid(gpio_ois_reset)) {
            gpio_request_one(gpio_ois_reset, GPIOF_OUT_INIT_LOW, "CAM_GPIO_OUTPUT_LOW");
            gpio_free(gpio_ois_reset);
        }
    }

	/* F-Rom LDO */
	gpio_camio_1p8_en = of_get_named_gpio(dnode, "gpio_camio_1p8_en", 0);
	if (!gpio_is_valid(gpio_camio_1p8_en)) {
		dev_info(dev, "failed to get gpio_camio_1p8_en\n");
	} else {
		gpio_request_one(gpio_camio_1p8_en, GPIOF_OUT_INIT_LOW, "SUBCAM_CAMIO_1P8_EN");
		gpio_free(gpio_camio_1p8_en);
	}

	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF);
#ifdef CONFIG_PREPROCESSOR_STANDBY_USE
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF);
#endif
#ifdef CONFIG_OIS_USE
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF);
#endif
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON);
	SET_PIN_INIT(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF);

	/* Normal on */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDA_1.8V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDA_1.8V_COMP", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_COMP", PIN_REGULATOR, 1, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 1, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 1, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_1.2V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDA_2.9V_CAM", PIN_REGULATOR, 1, 0, 2800000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_CORE_1.0V_COMP", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_CORE_0.8V_COMP", PIN_REGULATOR, 1, 0);
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_RET_1.0V_COMP", PIN_REGULATOR, 1, 0, 1050000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "VDDD_NORET_0.9V_COMP", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 2, 0);

    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_ois_reset, "ois_rst high", PIN_OUTPUT, 1, 0);
    }

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_ON, gpio_prep_reset, "prep_rst high", PIN_OUTPUT, 1, 9250);

	/* Normal off */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_prep_reset, "prep_rst low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 0, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_NORET_0.9V_COMP", PIN_REGULATOR, 0, 2500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_RET_1.0V_COMP", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_CORE_0.8V_COMP", PIN_REGULATOR, 0, 1500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_CORE_1.0V_COMP", PIN_REGULATOR, 0, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst", PIN_OUTPUT, 0, 1);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_COMP", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDA_1.8V_COMP", PIN_REGULATOR, 0, 0);

    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_ois_reset, "ois_rst low", PIN_OUTPUT, 0, 0);
    }

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 0, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDA_2.9V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDA_1.8V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_OFF, gpio_none, "VDDD_1.2V_CAM", PIN_REGULATOR, 0, 0);

#ifdef CONFIG_PREPROCESSOR_STANDBY_USE
#ifdef CAMERA_PARALLEL_RETENTION_SEQUENCE
	/* STANDBY DISABLE */
	/* Sensor */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "VDDA_1.8V_CAM", PIN_REGULATOR, 1, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 1, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 1, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "VDDD_1.2V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_SENSOR, gpio_none, "VDDA_2.9V_CAM", PIN_REGULATOR, 1, 0, 2800000);

	/* Companion */
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR, gpio_none, "VDDD_RET_1.0V_COMP", PIN_REGULATOR, 1, 2200, 1050000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR, gpio_none, "VDDA_1.8V_COMP", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR, gpio_none, "VDDD_CORE_1.0V_COMP", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR, gpio_none, "VDDD_CORE_0.8V_COMP", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF_PREPROCESSOR, gpio_none, "VDDD_NORET_0.9V_COMP", PIN_REGULATOR, 1, 100);

	/* MCLK & reset */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "pin", PIN_FUNCTION, 2, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_prep_reset, "prep_rst high", PIN_OUTPUT, 1, 300); //cap issue: 0->300

    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_ois_reset, "ois_rst high", PIN_OUTPUT, 1, 0);
    }

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 2000);
#else /* !CAMERA_PARALLEL_RETENTION_SEQUENCE */
	/* STANDBY DISABLE */
	/* Sensor */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDA_1.8V_CAM", PIN_REGULATOR, 1, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 1, 2500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 1, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 1, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDD_1.2V_CAM", PIN_REGULATOR, 1, 0);
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDA_2.9V_CAM", PIN_REGULATOR, 1, 0, 2800000);

	/* Companion */
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDD_RET_1.0V_COMP", PIN_REGULATOR, 1, 2200, 1050000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDA_1.8V_COMP", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDD_CORE_1.0V_COMP", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDD_CORE_0.8V_COMP", PIN_REGULATOR, 1, 100);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "VDDD_NORET_0.9V_COMP", PIN_REGULATOR, 1, 100);

	/* MCLK & reset */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_none, "pin", PIN_FUNCTION, 2, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_prep_reset, "prep_rst high", PIN_OUTPUT, 1, 300); //cap issue: 0->300
        
    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_ois_reset, "ois_rst high", PIN_OUTPUT, 1, 0);
    }

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_OFF, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 2000);
#endif /* CAMERA_PARALLEL_RETENTION_SEQUENCE */
	/* STANDBY ENABLE */
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_reset, "sen_rst", PIN_OUTPUT, 0, 0);
        
    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_ois_reset, "ois_rst low", PIN_OUTPUT, 0, 0);
    }

	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_prep_reset, "prep_rst low", PIN_OUTPUT, 0, 10);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDD_NORET_0.9V_COMP", PIN_REGULATOR, 0, 2500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDD_CORE_0.8V_COMP", PIN_REGULATOR, 0, 1500);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDD_CORE_1.0V_COMP", PIN_REGULATOR, 0, 2000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDA_1.8V_COMP", PIN_REGULATOR, 0, 0);
	SET_PIN_VOLTAGE(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDD_RET_1.0V_COMP", PIN_REGULATOR, 1, 0, 700000);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDA_2.9V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDA_1.8V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDD_1.2V_CAM", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "VDDIO_1.8V_CAM", PIN_REGULATOR, 0, 0);
#ifdef CONFIG_OIS_USE
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_NORMAL, GPIO_SCENARIO_STANDBY_ON, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 0, 0);
#endif
#endif /* CONFIG_PREPROCESSOR_STANDBY_USE */

#ifdef CONFIG_OIS_USE
	/* OIS_FACTORY	- POWER ON */
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 1, 0);
#ifdef CAMERA_REAR2_AF
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_cam_af_2p8_en, "SUBCAM_AF_2P8_EN", PIN_OUTPUT, 1, 20000);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_camio_1p8_en, "camio_1p8_en high", PIN_OUTPUT, 1, 0);
#endif
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_none, "pin", PIN_FUNCTION, 2, 1);
        
    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_ois_reset, "ois_rst high", PIN_OUTPUT, 1, 10000);
    } else {
        SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_ON, gpio_reset, "sen_rst high", PIN_OUTPUT, 1, 10000);
    }

    /* OIS_FACTORY	- POWER OFF */
    if (power_seq_id == 1) {
        if (gpio_is_valid(gpio_ois_reset))
            SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_ois_reset, "ois_rst low", PIN_OUTPUT, 0, 0);
    } else {
        SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_reset, "sen_rst", PIN_OUTPUT, 0, 0);
    }

	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 1, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "pin", PIN_FUNCTION, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "OIS_VDD_2.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "OIS_VM_2.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_none, "VDDAF_2.8V_CAM", PIN_REGULATOR, 0, 0);
#ifdef CAMERA_REAR2_AF
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_cam_af_2p8_en, "SUBCAM_AF_2P8_EN low", PIN_OUTPUT, 0, 0);
	SET_PIN(pdata, SENSOR_SCENARIO_OIS_FACTORY, GPIO_SCENARIO_OFF, gpio_camio_1p8_en, "camio_1p8_en low", PIN_OUTPUT, 0, 0);
#endif
#endif

	if (!gpio_is_valid(gpio_camio_1p8_en)) {
		/* READ_ROM - POWER ON */
		SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 1, 0);
		/* READ_ROM - POWER OFF */
		SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_none, "OIS_VDD_1.8V", PIN_REGULATOR, 0, 0);
	} else {
		/* READ_ROM - POWER ON */
		SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON, gpio_camio_1p8_en, "camio_1p8_en", PIN_OUTPUT, 1, 0);
		/* READ_ROM - POWER OFF */
		SET_PIN(pdata, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF, gpio_camio_1p8_en, "camio_1p8_en", PIN_INPUT, 0, 0);
	}

	dev_info(dev, "%s X v4\n", __func__);

	return 0;
}
#endif /* CONFIG_OF */

static int __init sensor_module_imx333_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;
	struct sensor_open_extended *ext;
	struct exynos_platform_fimc_is_module *pdata;
	struct device *dev;
	int vc_idx;
	struct pinctrl_state *s;

	FIMC_BUG(!fimc_is_dev);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		probe_err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	dev = &pdev->dev;

#ifdef CONFIG_OF
	fimc_is_sensor_module_parse_dt(pdev, sensor_imx333_power_setpin);
#endif

	pdata = dev_get_platdata(dev);
	device = &core->sensor[pdata->id];

	subdev_module = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_module) {
		probe_err("subdev_module is NULL");
		ret = -ENOMEM;
		goto p_err;
	}

	module = &device->module_enum[atomic_read(&device->module_count)];
	atomic_inc(&device->module_count);
	clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
	module->pdata = pdata;
	module->dev = dev;
	module->sensor_id = SENSOR_NAME_IMX333;
	module->subdev = subdev_module;
	module->device = pdata->id;
	module->client = NULL;
	module->active_width = 4032;
	module->active_height = 3024;
	module->margin_left = 0;
	module->margin_right = 0;
	module->margin_top = 0;
	module->margin_bottom = 0;
	module->pixel_width = module->active_width + 0;
	module->pixel_height = module->active_height + 0;
	module->max_framerate = 300;
	module->position = pdata->position;
	module->mode = CSI_MODE_VC_DT;
	module->lanes = CSI_DATA_LANES_4;
	module->bitwidth = 10;
	module->vcis = ARRAY_SIZE(vci_imx333);
	module->vci = vci_imx333;
	module->sensor_maker = "SONY";
	module->sensor_name = "IMX333";
	module->setfile_name = "setfile_imx333.bin";
	module->cfgs = ARRAY_SIZE(config_imx333);
	module->cfg = config_imx333;
	module->ops = NULL;

	for (vc_idx = 0; vc_idx < 2; vc_idx++) {
		switch (vc_idx) {
		case VC_BUF_DATA_TYPE_SENSOR_STAT1:
			module->vc_max_size[vc_idx].width = IMX333_PDAF_MAXWIDTH;
			module->vc_max_size[vc_idx].height = IMX333_PDAF_MAXHEIGHT;
			module->vc_max_size[vc_idx].element_size = IMX333_PDAF_ELEMENT;
			break;
		case VC_BUF_DATA_TYPE_GENERAL_STAT1:
			module->vc_max_size[vc_idx].width = IMX333_MIPI_MAXWIDTH;
			module->vc_max_size[vc_idx].height = IMX333_MIPI_MAXHEIGHT;
			module->vc_max_size[vc_idx].element_size = IMX333_MIPI_ELEMENT;
			break;
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
	ext->mipi_lane_num = module->lanes;
	ext->I2CSclk = 0;
#ifdef CONFIG_SENSOR_RETENTION_USE
	ext->use_retention_mode = SENSOR_RETENTION_DISABLE;
#endif

	ext->sensor_con.product_name = module->sensor_id /*SENSOR_NAME_IMX333*/;
	ext->sensor_con.peri_type = SE_I2C;
	ext->sensor_con.peri_setting.i2c.channel = pdata->sensor_i2c_ch;
	ext->sensor_con.peri_setting.i2c.slave_address = pdata->sensor_i2c_addr;
	ext->sensor_con.peri_setting.i2c.speed = 400000;

	ext->actuator_con.product_name = ACTUATOR_NAME_NOTHING;
	ext->flash_con.product_name = FLADRV_NAME_NOTHING;
	ext->from_con.product_name = FROMDRV_NAME_NOTHING;
	ext->preprocessor_con.product_name = PREPROCESSOR_NAME_NOTHING;
	ext->ois_con.product_name = OIS_NAME_NOTHING;

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

        /* ToDo: ???? */
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
		if (pdata->preprocessor_dma_channel == DMA_CH_NOT_DEFINED)
			ext->preprocessor_con.peri_info2.peri_setting.dma.channel = FLITE_ID_D;
		else
			ext->preprocessor_con.peri_info2.peri_setting.dma.channel = pdata->preprocessor_dma_channel;
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
p_err:
	probe_info("%s(%d)\n", __func__, ret);
	return ret;
}

static const struct of_device_id exynos_fimc_is_sensor_module_imx333_match[] = {
	{
		.compatible = "samsung,sensor-module-imx333",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_sensor_module_imx333_match);

static struct platform_driver sensor_module_imx333_driver = {
	.driver = {
		.name   = "FIMC-IS-SENSOR-MODULE-IMX333",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_sensor_module_imx333_match,
	}
};

static int __init fimc_is_sensor_module_imx333_init(void)
{
	int ret;

	ret = platform_driver_probe(&sensor_module_imx333_driver,
				sensor_module_imx333_probe);
	if (ret)
		err("failed to probe %s driver: %d\n",
			sensor_module_imx333_driver.driver.name, ret);

	return ret;
}
late_initcall(fimc_is_sensor_module_imx333_init);
