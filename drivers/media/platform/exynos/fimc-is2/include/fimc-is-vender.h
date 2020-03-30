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

#ifndef FIMC_IS_VENDER_H
#define FIMC_IS_VENDER_H

#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/videodev2.h>
#include <linux/device.h>

#include "exynos-fimc-is-sensor.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-sensor.h"
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
#include "fimc-is-vendor-mipi.h"
#endif

#define FIMC_IS_PATH_LEN 100
#define VENDER_S_CTRL 0
#define VENDER_G_CTRL 0

struct fimc_is_vender {
	char fw_path[FIMC_IS_PATH_LEN];
	char request_fw_path[FIMC_IS_PATH_LEN];
	char setfile_path[SENSOR_POSITION_MAX][FIMC_IS_PATH_LEN];
	char request_setfile_path[SENSOR_POSITION_MAX][FIMC_IS_PATH_LEN];
	void *private_data;
	int companion_crc_error;
	int opening_hint;
	int closing_hint;
};

enum {
	FW_SKIP,
	FW_SUCCESS,
	FW_FAIL,
};

enum fimc_is_rom_id {
	ROM_ID_REAR		= 0,
	ROM_ID_FRONT	= 1,
	ROM_ID_REAR2	= 2,
	ROM_ID_FRONT2	= 3,
	ROM_ID_REAR3	= 4,
	ROM_ID_FRONT3	= 5,
	ROM_ID_MAX,
	ROM_ID_NOTHING	= 100
};

enum fimc_is_rom_type {
	ROM_TYPE_NONE	= 0,
	ROM_TYPE_FROM	= 1,
	ROM_TYPE_EEPROM	= 2,
	ROM_TYPE_OTPROM	= 3,
	ROM_TYPE_MAX,
};

enum fimc_is_rom_cal_index {
	ROM_CAL_MASTER	= 0,
	ROM_CAL_SLAVE0	= 1,
	ROM_CAL_SLAVE1	= 2,
	ROM_CAL_MAX,
	ROM_CAL_NOTHING	= 100
};

enum fimc_is_rom_dualcal_index {
	ROM_DUALCAL_SLAVE0	= 0,
	ROM_DUALCAL_SLAVE1	= 1,
	ROM_DUALCAL_SLAVE2	= 2,
	ROM_DUALCAL_SLAVE3	= 3,
	ROM_DUALCAL_MAX,
	ROM_DUALCAL_NOTHING	= 100
};

struct capture_intent_info_t {
	u16 captureIntent;
	u16 captureCount;
	s16 captureEV;
};

#define TOF_CAL_SIZE_MAX 10
#define TOF_CAL_UID_MAX 10
#define TOF_CAL_VALID_MAX 10

#ifdef USE_CAMERA_HW_BIG_DATA
#define CAM_HW_ERR_CNT_FILE_PATH "/data/vendor/camera/camera_hw_err_cnt.dat"

struct cam_hw_param {
	u32 i2c_sensor_err_cnt;
	u32 i2c_comp_err_cnt;
	u32 i2c_ois_err_cnt;
	u32 i2c_af_err_cnt;
	u32 i2c_aperture_err_cnt;
	u32 mipi_sensor_err_cnt;
	u32 mipi_comp_err_cnt;
} __attribute__((__packed__));

struct cam_hw_param_collector {
	struct cam_hw_param rear_hwparam;
	struct cam_hw_param rear2_hwparam;
	struct cam_hw_param rear3_hwparam;
	struct cam_hw_param front_hwparam;
	struct cam_hw_param front2_hwparam;
	struct cam_hw_param rear_tof_hwparam;
	struct cam_hw_param front_tof_hwparam;
	struct cam_hw_param iris_hwparam;
} __attribute__((__packed__));

void fimc_is_sec_init_err_cnt(struct cam_hw_param *hw_param);
void fimc_is_sec_get_hw_param(struct cam_hw_param **hw_param, u32 position);
#ifdef CAMERA_HW_BIG_DATA_FILE_IO
bool fimc_is_sec_need_update_to_file(void);
void fimc_is_sec_copy_err_cnt_from_file(void);
void fimc_is_sec_copy_err_cnt_to_file(void);
#endif /* CAMERA_HW_BIG_DATA_FILE_IO */
#endif /* USE_CAMERA_HW_BIG_DATA */
bool fimc_is_sec_is_valid_moduleid(char *moduleid);

void fimc_is_vendor_csi_stream_on(struct fimc_is_device_csi *csi);
void fimc_is_vender_csi_err_handler(struct fimc_is_device_csi *csi);
void fimc_is_vender_csi_err_print_debug_log(struct fimc_is_device_sensor *device);

int fimc_is_vender_probe(struct fimc_is_vender *vender);
int fimc_is_vender_dt(struct device_node *np);
int fimc_is_vendor_rom_parse_dt(struct device_node *dnode, int rom_id);
int fimc_is_vender_fw_prepare(struct fimc_is_vender *vender);
int fimc_is_vender_fw_filp_open(struct fimc_is_vender *vender, struct file **fp, int bin_type);
int fimc_is_vender_preproc_fw_load(struct fimc_is_vender *vender);
int fimc_is_vender_s_ctrl(struct fimc_is_vender *vender);
int fimc_is_vender_cal_load(struct fimc_is_vender *vender, void *module_data);
int fimc_is_vender_module_sel(struct fimc_is_vender *vender, void *module_data);
int fimc_is_vender_module_del(struct fimc_is_vender *vender, void *module_data);
int fimc_is_vender_fw_sel(struct fimc_is_vender *vender);
int fimc_is_vender_setfile_sel(struct fimc_is_vender *vender, char *setfile_name, int position);
int fimc_is_vender_preprocessor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario);
int fimc_is_vender_preprocessor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario);
int fimc_is_vender_sensor_gpio_on_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario);
int fimc_is_vender_sensor_gpio_on(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario);
int fimc_is_vender_preprocessor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario,
	void *module_data);
int fimc_is_vender_preprocessor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario);
int fimc_is_vender_sensor_gpio_off_sel(struct fimc_is_vender *vender, u32 scenario, u32 *gpio_scenario,
	void *module_data);
int fimc_is_vender_sensor_gpio_off(struct fimc_is_vender *vender, u32 scenario, u32 gpio_scenario);
#ifdef CONFIG_SENSOR_RETENTION_USE
void fimc_is_vender_check_retention(struct fimc_is_vender *vender, void *module_data);
#endif
void fimc_is_vender_itf_open(struct fimc_is_vender *vender, struct sensor_open_extended *ext_info);
int fimc_is_vender_set_torch(struct camera2_shot *shot);
int fimc_is_vender_video_s_ctrl(struct v4l2_control *ctrl, void *device_data);
int fimc_is_vender_ssx_video_s_ctrl(struct v4l2_control *ctrl, void *device_data);
int fimc_is_vender_ssx_video_g_ctrl(struct v4l2_control *ctrl, void *device_data);
int fimc_is_vender_ssx_video_s_ext_ctrl(struct v4l2_ext_controls *ctrls, void *device_data);
int fimc_is_vender_hw_init(struct fimc_is_vender *vender);
void fimc_is_vender_check_hw_init_running(void);
void fimc_is_vender_sensor_s_input(struct fimc_is_vender *vender, void *module);
bool fimc_is_vender_wdr_mode_on(void *cis_data);
bool fimc_is_vender_enable_wdr(void *cis_data);
int fimc_is_vender_request_binary(struct fimc_is_binary * bin, const char * path1, const char * path2, const char * name, struct device * device);
void fimc_is_vender_resource_get(struct fimc_is_vender *vender);
void fimc_is_vender_resource_put(struct fimc_is_vender *vender);
int fimc_is_vender_remove_dump_fw_file(void);
int fimc_is_vendor_get_module_from_position(int position, struct fimc_is_module_enum ** module);
int fimc_is_vendor_get_rom_id_from_position(int position);
void fimc_is_vendor_get_rom_info_from_position(int position, int *rom_type, int *rom_id, int *rom_cal_index);
void fimc_is_vendor_get_rom_dualcal_info_from_position(int position, int *rom_type, int *rom_dualcal_id, int *rom_dualcal_index);
bool fimc_is_vendor_check_camera_running(int position);
#endif
