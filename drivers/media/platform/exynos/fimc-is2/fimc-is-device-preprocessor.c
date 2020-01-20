/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <video/videonode.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/videodev2.h>
#include <linux/videodev2_exynos_camera.h>
#include <linux/v4l2-mediabus.h>
#include <linux/bug.h>
#include <linux/i2c.h>

#include <exynos-fimc-is-preprocessor.h>
#include "fimc-is-video.h"
#include "fimc-is-dt.h"
#include "fimc-is-spi.h"
#include "fimc-is-device-preprocessor.h"
#include "fimc-is-core.h"
#include "fimc-is-dvfs.h"
#include "fimc-is-companion.h"

extern struct pm_qos_request exynos_isp_qos_int;
extern struct pm_qos_request exynos_isp_qos_mem;
extern struct pm_qos_request exynos_isp_qos_cam;
extern struct pm_qos_request exynos_isp_qos_disp;

int fimc_is_preproc_g_module(struct fimc_is_device_preproc *device,
	struct fimc_is_module_enum **module)
{
	int ret = 0;

	FIMC_BUG(!device);

	*module = device->module;
	if (!*module) {
		merr("module is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

	if (!(*module)->pdata) {
		merr("module->pdata is NULL", device);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_preproc_mclk_on(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct exynos_platform_fimc_is_preproc *pdata;
	struct exynos_platform_fimc_is_sensor *pdata_sensor;
	struct fimc_is_device_sensor *device_sensor;
	struct fimc_is_module_enum *module;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdev);
	FIMC_BUG(!device->pdata);

	pdata = device->pdata;
	core = device->private_data;
	device_sensor = &core->sensor[pdata->id];

	FIMC_BUG(!device_sensor);
	FIMC_BUG(!device_sensor->pdev);
	FIMC_BUG(!device_sensor->pdata);

	if (test_bit(FIMC_IS_PREPROC_MCLK_ON, &device->state)) {
		err("%s : already clk on", __func__);
		goto p_err;
	}

	if (pdata->id >= FIMC_IS_SENSOR_COUNT) {
		err("pdata id is inavalid(%d)", pdata->id);
		ret = -EINVAL;
		goto p_err;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	ret = fimc_is_preproc_g_module(device, &module);
	if (ret) {
		merr("fimc_is_sensor_g_module is fail(%d)", device, ret);
		goto p_err;
	}

	pdata_sensor = device_sensor->pdata;
	if (test_bit(FIMC_IS_SENSOR_MCLK_ON, &device_sensor->state)) {
		minfo("%s : already clk on", device_sensor, __func__);
		goto p_preproc_mclk_on;
	}

	if (!pdata_sensor->mclk_on) {
		merr("mclk_on is NULL", device);
		ret = -EINVAL;
		goto p_preproc_mclk_on;
	}

	ret = pdata_sensor->mclk_on(&device_sensor->pdev->dev, pdata->scenario, module->pdata->mclk_ch);
	if (ret) {
		merr("mclk_on is fail(%d)", device, ret);
		goto p_preproc_mclk_on;
	}

	set_bit(FIMC_IS_SENSOR_MCLK_ON, &device_sensor->state);

p_preproc_mclk_on:

	if (!pdata->mclk_on) {
		err("mclk_on is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = pdata->mclk_on(&device->pdev->dev, pdata->scenario, pdata->mclk_ch);
	if (ret) {
		err("mclk_on is fail(%d)", ret);
		goto p_err;
	}

	set_bit(FIMC_IS_PREPROC_MCLK_ON, &device->state);

p_err:
	return ret;
}

static int fimc_is_preproc_mclk_off(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct exynos_platform_fimc_is_preproc *pdata;
	struct exynos_platform_fimc_is_sensor *pdata_sensor;
	struct fimc_is_device_sensor *device_sensor;
	struct fimc_is_module_enum *module;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdev);
	FIMC_BUG(!device->pdata);
	FIMC_BUG(!device->private_data);

	pdata = device->pdata;
	core = device->private_data;
	device_sensor = &core->sensor[pdata->id];

	FIMC_BUG(!device_sensor);
	FIMC_BUG(!device_sensor->pdev);
	FIMC_BUG(!device_sensor->pdata);

	if (!test_bit(FIMC_IS_PREPROC_MCLK_ON, &device->state)) {
		err("%s : already clk off", __func__);
		goto p_err;
	}

	if (pdata->id >= FIMC_IS_SENSOR_COUNT) {
		err("pdata id is inavalid(%d)", pdata->id);
		ret = -EINVAL;
		goto p_err;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is NULL");
		ret = -ENODEV;
		goto p_err;
	}

	ret = fimc_is_preproc_g_module(device, &module);
	if (ret) {
		merr("fimc_is_sensor_g_module is fail(%d)", device, ret);
		goto p_err;
	}

	pdata_sensor = device_sensor->pdata;
	if (!test_bit(FIMC_IS_SENSOR_MCLK_ON, &device_sensor->state)) {
		minfo("%s : already clk off", device_sensor, __func__);
		goto p_preproc_mclk_off;
	}

	if (!pdata_sensor->mclk_off) {
		merr("mclk_off is NULL", device);
		ret = -EINVAL;
		goto p_preproc_mclk_off;
	}

	ret = pdata_sensor->mclk_off(&device_sensor->pdev->dev, pdata->scenario, module->pdata->mclk_ch);
	if (ret) {
		merr("mclk_off is fail(%d)", device, ret);
		goto p_preproc_mclk_off;
	}

	clear_bit(FIMC_IS_SENSOR_MCLK_ON, &device_sensor->state);

p_preproc_mclk_off:

	if (!pdata->mclk_off) {
		err("mclk_off is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = pdata->mclk_off(&device->pdev->dev, pdata->scenario, pdata->mclk_ch);
	if (ret) {
		err("mclk_off is fail(%d)", ret);
		goto p_err;
	}

	clear_bit(FIMC_IS_PREPROC_MCLK_ON, &device->state);

p_err:
	return ret;
}

static int fimc_is_preproc_iclk_on(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	struct platform_device *pdev;
	struct exynos_platform_fimc_is_preproc *pdata;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdev);
	FIMC_BUG(!device->pdata);

	pdev = device->pdev;
	pdata = device->pdata;

	if (test_bit(FIMC_IS_PREPROC_ICLK_ON, &device->state)) {
		err("%s : already clk on", __func__);
		goto p_err;
	}

	if (!pdata->iclk_cfg) {
		err("iclk_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (!pdata->iclk_on) {
		err("iclk_on is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = pdata->iclk_cfg(&pdev->dev, pdata->scenario, 0);
	if (ret) {
		err("iclk_cfg is fail(%d)", ret);
		goto p_err;
	}

	ret = pdata->iclk_on(&pdev->dev, pdata->scenario, 0);
	if (ret) {
		err("iclk_on is fail(%d)", ret);
		goto p_err;
	}

	set_bit(FIMC_IS_PREPROC_ICLK_ON, &device->state);

p_err:
	return ret;
}

static int fimc_is_preproc_iclk_off(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	struct platform_device *pdev;
	struct exynos_platform_fimc_is_preproc *pdata;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdev);
	FIMC_BUG(!device->pdata);

	pdev = device->pdev;
	pdata = device->pdata;

	if (!test_bit(FIMC_IS_PREPROC_ICLK_ON, &device->state)) {
		err("%s : already clk off", __func__);
		goto p_err;
	}

	if (!pdata->iclk_off) {
		err("iclk_off is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = pdata->iclk_off(&pdev->dev, pdata->scenario, 0);
	if (ret) {
		err("iclk_off is fail(%d)", ret);
		goto p_err;
	}

	clear_bit(FIMC_IS_PREPROC_ICLK_ON, &device->state);

p_err:
	return ret;
}

static int fimc_is_preproc_gpio_on(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	u32 scenario, gpio_scenario;
	struct fimc_is_core *core;
	struct fimc_is_module_enum *module;
	struct fimc_is_vender *vender;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdev);
	FIMC_BUG(!device->pdata);
	FIMC_BUG(!device->private_data);

	module = NULL;
	gpio_scenario = GPIO_SCENARIO_ON;
	core = device->private_data;
	scenario = device->pdata->scenario;
	vender = &core->vender;

	if (test_bit(FIMC_IS_PREPROC_GPIO_ON, &device->state)) {
		merr("%s : already gpio on", device, __func__);
		goto p_err;
	}

	ret = fimc_is_preproc_g_module(device, &module);
	if (ret) {
		merr("fimc_is_sensor_g_module is fail(%d)", device, ret);
		goto p_err;
	}

	ret = fimc_is_vender_module_sel(vender, module);
	if (ret) {
		merr("fimc_is_vender_module_sel is fail(%d)", device, ret);
		goto p_err;
	}

	if (!test_and_set_bit(FIMC_IS_MODULE_GPIO_ON, &module->state)) {
		struct exynos_platform_fimc_is_module *pdata;

		ret = fimc_is_vender_preprocessor_gpio_on_sel(vender,
			scenario, &gpio_scenario);
		if (ret) {
			clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("fimc_is_vender_preprocessor_gpio_on_sel is fail(%d)",
				device, ret);
			goto p_err;
		}

		pdata = module->pdata;
		if (!pdata) {
			clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("pdata is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		if (!pdata->gpio_cfg) {
			clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("gpio_cfg is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		ret = pdata->gpio_cfg(module, scenario, gpio_scenario);
		if (ret) {
			clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("gpio_cfg is fail(%d)", device, ret);
			goto p_err;
		}

		ret = fimc_is_vender_preprocessor_gpio_on(vender,
			scenario, gpio_scenario);
		if (ret) {
			merr("fimc_is_vender_preprocessor_gpio_on is fail(%d)",
				device, ret);
			goto p_err;
		}
	}

	set_bit(FIMC_IS_PREPROC_GPIO_ON, &device->state);

p_err:
	return ret;
}

static int fimc_is_preproc_gpio_off(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	u32 scenario, gpio_scenario;
	struct fimc_is_core *core;
	struct fimc_is_module_enum *module;
	struct fimc_is_vender *vender;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdev);
	FIMC_BUG(!device->pdata);
	FIMC_BUG(!device->private_data);

	module = NULL;
	gpio_scenario = GPIO_SCENARIO_OFF;
	core = device->private_data;
	scenario = device->pdata->scenario;
	vender = &core->vender;

	if (!test_bit(FIMC_IS_PREPROC_GPIO_ON, &device->state)) {
		err("%s : already gpio off", __func__);
		goto p_err;
	}

	ret = fimc_is_preproc_g_module(device, &module);
	if (ret) {
		merr("fimc_is_sensor_g_module is fail(%d)", device, ret);
		goto p_err;
	}

	if (test_and_clear_bit(FIMC_IS_MODULE_GPIO_ON, &module->state)) {
		struct exynos_platform_fimc_is_module *pdata;

		fimc_is_sensor_deinit_module(module);

		ret = fimc_is_vender_preprocessor_gpio_off_sel(vender,
			scenario, &gpio_scenario, module);
		if (ret) {
			set_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("fimc_is_vender_preprocessor_gpio_off_sel is fail(%d)",
				device, ret);
			goto p_err;
		}

#ifdef CONFIG_USE_DIRECT_IS_CONTROL
		/* Run FW/Data CRC on Close */
#ifdef CONFIG_PREPROCESSOR_STANDBY_USE
		ret = fimc_is_comp_run_CRC_on_Close(core);
#ifdef CONFIG_COMPANION_C3_USE
		if (!fimc_is_comp_check_CRC_Valid(core))
			merr("crc is not valid", device);
#endif
#endif
#endif
		pdata = module->pdata;
		if (!pdata) {
			set_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("pdata is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		if (!pdata->gpio_cfg) {
			set_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("gpio_cfg is NULL", device);
			ret = -EINVAL;
			goto p_err;
		}

		ret = pdata->gpio_cfg(module, scenario, gpio_scenario);
		if (ret) {
			set_bit(FIMC_IS_MODULE_GPIO_ON, &module->state);
			merr("gpio_cfg is fail(%d)", device, ret);
			goto p_err;
		}

		ret = fimc_is_vender_preprocessor_gpio_off(vender,
			scenario, gpio_scenario);
		if (ret) {
			merr("fimc_is_vender_preprocessor_gpio_off is fail(%d)",
				device, ret);
			goto p_err;
		}
	}

	clear_bit(FIMC_IS_PREPROC_GPIO_ON, &device->state);

p_err:
	if (module != NULL) {
		fimc_is_vender_module_del(vender, module);
	}

	return ret;
}

int fimc_is_preproc_open(struct fimc_is_device_preproc *device,
	struct fimc_is_video_ctx *vctx)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_vender *vender;
	struct exynos_platform_fimc_is_preproc *pdata;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdata);
	FIMC_BUG(!device->private_data);

	core = device->private_data;
	pdata = device->pdata;
	vender = &core->vender;

	if (test_bit(FIMC_IS_PREPROC_OPEN, &device->state)) {
		err("already open");
		ret = -EMFILE;
		goto p_err;
	}

	ret = fimc_is_resource_get(device->resourcemgr, RESOURCE_TYPE_PREPROC);
	if (ret) {
		merr("fimc_is_resource_get is fail", device);
		goto p_err;
	}

	device->vctx = vctx;

	set_bit(FIMC_IS_PREPROC_OPEN, &device->state);

p_err:
	minfo("[PRE:D] %s():%d\n", device, __func__, ret);
	return ret;
}

int fimc_is_preproc_close(struct fimc_is_device_preproc *device)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_vender *vender;
	struct exynos_platform_fimc_is_preproc *pdata;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdata);
	FIMC_BUG(!device->private_data);

	core = device->private_data;
	pdata = device->pdata;
	vender = &core->vender;

	if (!test_bit(FIMC_IS_PREPROC_OPEN, &device->state)) {
		err("already close");
		ret = -EMFILE;
		goto p_err;
	}

#ifndef CONFIG_USE_DIRECT_IS_CONTROL
	/* gpio uninit */
	ret = fimc_is_preproc_gpio_off(device);
	if (ret) {
		err("fimc_is_preproc_gpio_off is fail(%d)", ret);
		goto p_err;
	}

	/* master clock off */
	ret = fimc_is_preproc_mclk_off(device);
	if (ret) {
		err("fimc_is_preproc_mclk_off is fail(%d)", ret);
		goto p_err;
	}
#endif

	ret = fimc_is_resource_put(device->resourcemgr, RESOURCE_TYPE_PREPROC);
	if (ret)
		merr("fimc_is_resource_put is fail(%d)", device, ret);

	clear_bit(FIMC_IS_PREPROC_OPEN, &device->state);
	clear_bit(FIMC_IS_PREPROC_S_INPUT, &device->state);

p_err:
	minfo("[PRE:D] %s(%d)\n", device, __func__, ret);
	return ret;
}

static int fimc_is_preproc_probe(struct platform_device *pdev)
{
	int ret = 0;
	u32 instance = -1;
	atomic_t device_id;
	struct fimc_is_core *core;
	struct fimc_is_device_preproc *device;
	struct exynos_platform_fimc_is_preproc *pdata;

	FIMC_BUG(!pdev);

	if (fimc_is_dev == NULL) {
		warn("fimc_is_dev is not yet probed(preprocessor)");
		pdev->dev.init_name = FIMC_IS_PREPROC_DEV_NAME;
		return -EPROBE_DEFER;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core is NULL");
		return -EINVAL;
	}

#ifdef CONFIG_OF
	ret = fimc_is_preprocessor_parse_dt(pdev);
	if (ret) {
		err("parsing device tree is fail(%d)", ret);
		goto p_err;
	}
#endif

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		err("pdata is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	atomic_set(&device_id, 0);
	device = &core->preproc;

	memset(&device->v4l2_dev, 0, sizeof(struct v4l2_device));
	instance = v4l2_device_set_name(&device->v4l2_dev, "exynos-fimc-is-preprocessor", &device_id);
	device->instance = instance;
	device->resourcemgr = &core->resourcemgr;
	device->pdev = pdev;
	device->private_data = core;
	device->regs = core->regs;
	device->pdata = pdata;
	platform_set_drvdata(pdev, device);
	device_init_wakeup(&pdev->dev, true);

	/* init state */
	clear_bit(FIMC_IS_PREPROC_OPEN, &device->state);
	clear_bit(FIMC_IS_PREPROC_MCLK_ON, &device->state);
	clear_bit(FIMC_IS_PREPROC_ICLK_ON, &device->state);
	clear_bit(FIMC_IS_PREPROC_GPIO_ON, &device->state);
	clear_bit(FIMC_IS_PREPROC_S_INPUT, &device->state);

	ret = v4l2_device_register(&pdev->dev, &device->v4l2_dev);
	if (ret) {
		err("v4l2_device_register is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_pre_video_probe(device);
	if (ret) {
		err("fimc_is_preproc_video_probe is fail(%d)", ret);
		v4l2_device_unregister(&device->v4l2_dev);
		goto p_err;
	}

#if defined(CONFIG_PM)
	pm_runtime_enable(&pdev->dev);
#endif

p_err:
	info("[%d][PRE:D] %s():%d\n", instance, __func__, ret);
	return ret;
}

static int fimc_is_preproc_remove(struct platform_device *pdev)
{
	int ret = 0;

	info("%s\n", __func__);

	return ret;
}

static int fimc_is_preproc_suspend(struct device *dev)
{
	int ret = 0;

	info("%s\n", __func__);

	return ret;
}

static int fimc_is_preproc_resume(struct device *dev)
{
	int ret = 0;

	info("%s\n", __func__);

	return ret;
}

int fimc_is_preproc_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_preproc *device;

	device = (struct fimc_is_device_preproc *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		ret = -EINVAL;
		goto p_err;
	}

#ifdef CONFIG_USE_DIRECT_IS_CONTROL
	/* gpio uninit */
	ret = fimc_is_preproc_gpio_off(device);
	if (ret) {
		err("fimc_is_preproc_gpio_off is fail(%d)", ret);
		goto p_err;
	}
#endif

	/* periperal internal clock off */
	ret = fimc_is_preproc_iclk_off(device);
	if (ret) {
		err("fimc_is_preproc_iclk_off is fail(%d)", ret);
		goto p_err;
	}

#ifdef CONFIG_USE_DIRECT_IS_CONTROL
	/* master clock off */
	ret = fimc_is_preproc_mclk_off(device);
	if (ret) {
		err("fimc_is_preproc_mclk_off is fail(%d)", ret);
		goto p_err;
	}
#endif

	device->module = NULL;

p_err:
	info("[PRE:D] %s():%d\n", __func__, ret);
	return ret;
}

int fimc_is_preproc_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct fimc_is_device_preproc *device;

	device = (struct fimc_is_device_preproc *)dev_get_drvdata(dev);
	if (!device) {
		err("device is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	/* periperal internal clock on */
	ret = fimc_is_preproc_iclk_on(device);
	if (ret) {
		err("fimc_is_preproc_iclk_on is fail(%d)", ret);
		goto p_err;
	}

p_err:
	info("[PRE:D] %s():%d\n", __func__, ret);
	return ret;
}

int fimc_is_preproc_s_input(struct fimc_is_device_preproc *device,
	u32 input,
	u32 scenario)
{
	int ret = 0;
	struct fimc_is_core *core;
	struct fimc_is_vender *vender;
	struct fimc_is_device_sensor *device_sensor;

	FIMC_BUG(!device);
	FIMC_BUG(!device->pdata);
	FIMC_BUG(!device->private_data);
	FIMC_BUG(input >= SENSOR_NAME_END);

	TIME_LAUNCH_STR(LAUNCH_PREPROC_LOAD);

	core = device->private_data;
	vender = &core->vender;
	device_sensor = &core->sensor[device->pdata->id];

	ret = fimc_is_search_sensor_module_with_sensorid(device_sensor, input, &device->module);
	if (ret) {
		err("fimc_is_search_sensor_module_with_sensorid is fail(%d)", ret);
		goto p_err;
	}

	core->current_position = device->module->position;

	if (test_bit(FIMC_IS_PREPROC_S_INPUT, &device->state)) {
		err("already s_input");
		ret = -EMFILE;
		goto p_err;
	}

	ret = fimc_is_preproc_mclk_on(device);
	if (ret) {
		err("fimc_is_preproc_mclk_on is fail(%d)", ret);
		goto p_err;
	}

	/* gpio init */
	ret = fimc_is_preproc_gpio_on(device);
	if (ret) {
		err("fimc_is_preproc_gpio_on is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_vender_fw_prepare(vender);
	if (ret) {
		err("fimc_is_vender_fw_prepare is fail(%d)", ret);
		goto p_err;
	}

	ret = fimc_is_vender_preproc_fw_load(vender);
	if (ret) {
		err("fimc_is_vender_preproc_fw_load is fail(%d)", ret);
		goto p_err;
	}

	set_bit(FIMC_IS_PREPROC_S_INPUT, &device->state);

p_err:
	minfo("[PRE:D] %s(module(%d), scenario(%d)):%d\n", device, __func__, input, scenario, ret);
	TIME_LAUNCH_END(LAUNCH_PREPROC_LOAD);
	return ret;
}

static const struct dev_pm_ops fimc_is_preproc_pm_ops = {
	.suspend		= fimc_is_preproc_suspend,
	.resume			= fimc_is_preproc_resume,
	.runtime_suspend	= fimc_is_preproc_runtime_suspend,
	.runtime_resume		= fimc_is_preproc_runtime_resume,
};

#ifdef CONFIG_OF
static const struct of_device_id exynos_fimc_is_preproc_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-preprocessor",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_preproc_match);

static struct platform_driver fimc_is_preproc_driver = {
	.probe		= fimc_is_preproc_probe,
	.remove 	= fimc_is_preproc_remove,
	.driver = {
		.name	= FIMC_IS_PREPROC_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &fimc_is_preproc_pm_ops,
		.of_match_table = exynos_fimc_is_preproc_match,
	}
};

#else
static struct platform_device_id fimc_is_preproc_driver_ids[] = {
	{
		.name		= FIMC_IS_PREPROC_DEV_NAME,
		.driver_data	= 0,
	},
	{},
};
MODULE_DEVICE_TABLE(platform, fimc_is_preproc_driver_ids);

static struct platform_driver fimc_is_preproc_driver = {
	.probe		= fimc_is_preproc_probe,
	.remove 	= __devexit_p(fimc_is_preproc_remove),
	.id_table	= fimc_is_preproc_driver_ids,
	.driver	  = {
		.name	= FIMC_IS_PREPROC_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &fimc_is_preproc_pm_ops,
	}
};
#endif

static int __init fimc_is_preproc_init(void)
{
	int ret = platform_driver_register(&fimc_is_preproc_driver);
	if (ret)
		err("platform_driver_register failed: %d\n", ret);

	return ret;
}
late_initcall(fimc_is_preproc_init);

static void __exit fimc_is_preproc_exit(void)
{
	platform_driver_unregister(&fimc_is_preproc_driver);
}
module_exit(fimc_is_preproc_exit);

MODULE_AUTHOR("Wooki Min<wooki.min@samsung.com>");
MODULE_DESCRIPTION("Exynos FIMC_IS_PREPROCESSOR driver");
MODULE_LICENSE("GPL");
