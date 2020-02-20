/*
* Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is vender functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>

#include "fimc-is-vender.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-core.h"
#include "fimc-is-interface-library.h"

static u32  rear_sensor_id;
static u32  front_sensor_id;
static u32  rear2_sensor_id;
#ifdef CONFIG_SECURE_CAMERA_USE
static u32  secure_sensor_id;
#endif
static u32  front2_sensor_id;
static u32  rear3_sensor_id;
static u32  ois_sensor_index;
static u32  aperture_sensor_index;

void fimc_is_vendor_csi_stream_on(struct fimc_is_device_csi *csi)
{

}

void fimc_is_vender_csi_err_handler(struct fimc_is_device_csi *csi)
{

}

int fimc_is_vender_probe(struct fimc_is_vender *vender)
{
	int ret = 0;
	struct fimc_is_vender_specific *priv;

	BUG_ON(!vender);

	snprintf(vender->fw_path, sizeof(vender->fw_path), "%s%s", FIMC_IS_FW_DUMP_PATH, FIMC_IS_FW);
	snprintf(vender->request_fw_path, sizeof(vender->request_fw_path), "%s", FIMC_IS_FW);

	fimc_is_load_ctrl_init();

	priv = (struct fimc_is_vender_specific *)kzalloc(
					sizeof(struct fimc_is_vender_specific), GFP_KERNEL);
	if (!priv) {
		probe_err("failed to allocate vender specific");
		return -ENOMEM;
	}

	priv->rear_sensor_id = rear_sensor_id;
	priv->front_sensor_id = front_sensor_id;
	priv->rear2_sensor_id = rear2_sensor_id;
#ifdef CONFIG_SECURE_CAMERA_USE
	priv->secure_sensor_id = secure_sensor_id;
#endif
	priv->front2_sensor_id = front2_sensor_id;
	priv->rear3_sensor_id = rear3_sensor_id;
	priv->ois_sensor_index = ois_sensor_index;
	priv->aperture_sensor_index = aperture_sensor_index;

	vender->private_data = priv;

	return ret;
}

int fimc_is_vender_dt(struct device_node *np)
{
	int ret = 0;

	ret = of_property_read_u32(np, "rear_sensor_id", &rear_sensor_id);
	if (ret)
		probe_err("rear_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "front_sensor_id", &front_sensor_id);
	if (ret)
		probe_err("front_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "rear2_sensor_id", &rear2_sensor_id);
	if (ret)
		probe_err("rear2_sensor_id read is fail(%d)", ret);

#ifdef CONFIG_SECURE_CAMERA_USE
	ret = of_property_read_u32(np, "secure_sensor_id", &secure_sensor_id);
	if (ret)
		probe_err("secure_sensor_id read is fail(%d)", ret);
#endif
	ret = of_property_read_u32(np, "front2_sensor_id", &front2_sensor_id);
	if (ret)
		probe_err("front2_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "rear3_sensor_id", &rear3_sensor_id);
	if (ret)
		probe_err("rear3_sensor_id read is fail(%d)", ret);

	ret = of_property_read_u32(np, "ois_sensor_index", &ois_sensor_index);
	if (ret)
		probe_err("ois_sensor_index read is fail(%d)", ret);

	ret = of_property_read_u32(np, "aperture_sensor_index", &aperture_sensor_index);
	if (ret)
		probe_err("aperture_sensor_index read is fail(%d)", ret);

	return ret;
}

int fimc_is_vender_fw_prepare(struct fimc_is_vender *vender)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_fw_filp_open(struct fimc_is_vender *vender, struct file **fp, int bin_type)
{
	return FW_SKIP;
}

int fimc_is_vender_preproc_fw_load(struct fimc_is_vender *vender)
{
	int ret = 0;

	return ret;
}

void fimc_is_vender_resource_get(struct fimc_is_vender *vender)
{

}

void fimc_is_vender_resource_put(struct fimc_is_vender *vender)
{

}

#if !defined(ENABLE_CAL_LOAD)
int fimc_is_vender_cal_load(struct fimc_is_vender *vender,
	void *module_data)
{
	int ret = 0;

	return ret;
}
#else
int fimc_is_vender_cal_load(struct fimc_is_vender *vender,
	void *module_data)
{
	struct fimc_is_core *core;
	struct fimc_is_module_enum *module = module_data;
	struct fimc_is_binary cal_bin;
	ulong cal_addr = 0;
	int ret = 0;

	core = container_of(vender, struct fimc_is_core, vender);

	setup_binary_loader(&cal_bin, 0, 0, NULL, NULL);
	if (module->position == SENSOR_POSITION_REAR) {
		/* Load calibration data from file system */
		ret = request_binary(&cal_bin, FIMC_IS_REAR_CAL_SDCARD_PATH,
								FIMC_IS_REAR_CAL, NULL);
		if (ret) {
			err("[Vendor]: request_binary filed: %s%s",
					FIMC_IS_REAR_CAL_SDCARD_PATH, FIMC_IS_REAR_CAL);
			goto exit;
		}
#ifdef ENABLE_IS_CORE
		cal_addr = core->resourcemgr.minfo.kvaddr + CAL_OFFSET0;
#else
		cal_addr = core->resourcemgr.minfo.kvaddr_cal[SENSOR_POSITION_REAR];
#endif
	} else if (module->position == SENSOR_POSITION_FRONT) {
		/* Load calibration data from file system */
		ret = request_binary(&cal_bin, FIMC_IS_REAR_CAL_SDCARD_PATH,
								FIMC_IS_FRONT_CAL, NULL);
		if (ret) {
			err("[Vendor]: request_binary filed: %s%s",
					FIMC_IS_REAR_CAL_SDCARD_PATH, FIMC_IS_FRONT_CAL);
			goto exit;
		}
#ifdef ENABLE_IS_CORE
		cal_addr = core->resourcemgr.minfo.kvaddr + CAL_OFFSET1;
#else
		cal_addr = core->resourcemgr.minfo.kvaddr_cal[SENSOR_POSITION_REAR];
#endif
	} else {
		err("[Vendor]: Invalid sensor position: %d", module->position);
		module->ext.sensor_con.cal_address = 0;
		ret = -EINVAL;
		goto exit;
	}

	memcpy((void *)(cal_addr), (void *)cal_bin.data, cal_bin.size);

	release_binary(&cal_bin);
exit:
	if (ret)
		err("CAL data loading is fail: skip");
	else
		info("CAL data(%d) loading is complete: 0x%lx\n", module->position, cal_addr);

	return 0;
}
#endif

int fimc_is_vender_module_sel(struct fimc_is_vender *vender, void *module_data)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_module_del(struct fimc_is_vender *vender, void *module_data)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_fw_sel(struct fimc_is_vender *vender)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_setfile_sel(struct fimc_is_vender *vender,
	char *setfile_name,
	int position)
{
	int ret = 0;

	BUG_ON(!vender);
	BUG_ON(!setfile_name);

	snprintf(vender->setfile_path[position], sizeof(vender->setfile_path[position]),
		"%s%s", FIMC_IS_SETFILE_SDCARD_PATH, setfile_name);
	snprintf(vender->request_setfile_path[position],
		sizeof(vender->request_setfile_path[position]),
		"%s", setfile_name);

	return ret;
}

int fimc_is_vender_preprocessor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scneario)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_preprocessor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	int ret = 0;
	return ret;
}

int fimc_is_vender_sensor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario)
{
	int ret = 0;
	return ret;
}

int fimc_is_vender_sensor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	int ret = 0;
	return ret;
}

int fimc_is_vender_preprocessor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scneario,
	void *module_data)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_preprocessor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_sensor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario,
	void *module_data)
{
	int ret = 0;

	return ret;
}

int fimc_is_vender_sensor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario)
{
	int ret = 0;

	return ret;
}

void fimc_is_vender_itf_open(struct fimc_is_vender *vender, struct sensor_open_extended *ext_info)
{
	return;
}

int fimc_is_vender_set_torch(struct camera2_shot *shot)
{
	return 0;
}

int fimc_is_vender_video_s_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	return 0;
}

int fimc_is_vender_ssx_video_s_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	return 0;
}

int fimc_is_vender_ssx_video_g_ctrl(struct v4l2_control *ctrl,
	void *device_data)
{
	return 0;
}

void fimc_is_vender_sensor_s_input(struct fimc_is_vender *vender, void *module)
{
	return;
}

bool fimc_is_vender_wdr_mode_on(void *cis_data)
{
	return false;
}

bool fimc_is_vender_enable_wdr(void *cis_data)
{
	return false;
}

/**
  * fimc_is_vender_request_binary: send loading request to the loader
  * @bin: pointer to fimc_is_binary structure
  * @path: path of binary file
  * @name: name of binary file
  * @device: device for which binary is being loaded
  **/
int fimc_is_vender_request_binary(struct fimc_is_binary *bin, const char *path1, const char *path2,
				const char *name, struct device *device)
{

	return 0;
}

int fimc_is_vender_s_ctrl(struct fimc_is_vender *vender)
{
	return 0;
}

int fimc_is_vender_remove_dump_fw_file(void)
{
	return 0;
}
