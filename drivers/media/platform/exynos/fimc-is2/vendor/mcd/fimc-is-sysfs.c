/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm_qos.h>
#include <linux/device.h>

#include "fimc-is-sysfs.h"
#include "fimc-is-core.h"
#include "fimc-is-err.h"
#include "fimc-is-sec-define.h"
#if defined (CONFIG_OIS_USE)
#include "fimc-is-device-ois.h"
#endif
#ifdef CONFIG_AF_HOST_CONTROL
#include "fimc-is-device-af.h"
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
#include "fimc-is-device-sensor-peri.h"
#endif
#include "fimc-is-vender-specific.h"
#include "fimc-is-interface-library.h"
#include "fimc-is-device-ois_s6.h"

/* #define FORCE_CAL_LOAD */
#define SYSFS_MAX_READ_SIZE	4096

extern struct device *fimc_is_dev;
struct class *camera_class = NULL;
struct device *camera_front_dev;
struct device *camera_rear_dev;
#ifdef SECURE_CAMERA_IRIS
struct device *camera_secure_dev;
#endif
#if defined (CONFIG_OIS_USE)
struct device *camera_ois_dev;
#endif
static struct fimc_is_core *sysfs_core;
extern struct kset *devices_kset;
extern struct fimc_is_lib_support gPtr_lib_support;

static struct fimc_is_cam_info cam_infos[CAM_INFO_MAX];
static struct fimc_is_common_cam_info common_cam_infos;

extern bool force_caldata_dump;
#if defined (CONFIG_OIS_USE)
static bool check_ois_power = false;
int ois_threshold = 150;
#ifdef CAMERA_2ND_OIS
int ois_threshold_rear2 = 150;
#endif
long x_init_raw = 0;
long y_init_raw = 0;
#endif
static bool check_module_init = false;

#ifdef SECURE_CAMERA_IRIS
extern bool is_final_cam_module_iris;
extern bool is_iris_ver_read;
extern bool is_iris_mtf_test_check;
extern u8 is_iris_mtf_read_data[3];	/* 0:year 1:month 2:company */
#endif

struct ssrm_camera_data {
	int operation;
	int cameraID;
	int previewSizeWidth;
	int previewSizeHeight;
	int previewMinFPS;
	int previewMaxFPS;
	int sensorOn;
};

enum ssrm_camerainfo_operation {
	SSRM_CAMERA_INFO_CLEAR,
	SSRM_CAMERA_INFO_SET,
	SSRM_CAMERA_INFO_UPDATE,
};

struct ssrm_camera_data SsrmCameraInfo[FIMC_IS_SENSOR_COUNT];

/* read firmware */
static int read_from_firmware_version(int rom_id)
{
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;
	int ret = 0;
	struct fimc_is_rom_info *finfo;

	fimc_is_vender_check_hw_init_running();
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	if (!test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state)) {
		ret = fimc_is_sec_run_fw_sel(is_dev, rom_id);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
		}
	}

	return 0;
}

static int read_from_firmware_version_reload(int rom_id)
{
	int ret = 0;
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;

	ret = fimc_is_sec_run_fw_sel(is_dev, rom_id);
	if (ret)
		err("fimc_is_sec_run_fw_sel is fail(%d)", ret);

	return 0;
}

/* common utility */
int fimc_is_get_cam_info(struct fimc_is_cam_info **caminfo)
{
	*caminfo = cam_infos;
	return 0;
}

int fimc_is_get_cam_info_from_index(struct fimc_is_cam_info **caminfo, enum fimc_is_cam_info_index cam_index)
{
	*caminfo = &(cam_infos[cam_index]);
	return 0;
}

void fimc_is_get_common_cam_info(struct fimc_is_common_cam_info **caminfo)
{
	*caminfo = &common_cam_infos;
}

static int fimc_is_get_sensor_data(char *maker, char *name, int position)
{
	struct fimc_is_module_enum *module;

	fimc_is_vendor_get_module_from_position(position, &module);

	if (module) {
		if (maker != NULL)
			sprintf(maker, "%s", module->sensor_maker ?
					module->sensor_maker : "UNKNOWN");
		if (name != NULL)
			sprintf(name, "%s", module->sensor_name ?
					module->sensor_name : "UNKNOWN");
		return 0;
	}

	err("%s: there's no matched sensor id", __func__);

	return -ENODEV;
}

/* common function for sysfs */
static ssize_t camera_info_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	char camera_info[130] = {0, };
#ifdef CONFIG_OF
	struct fimc_is_cam_info *cam_info;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	if (!cam_info->valid) {
		strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

		if (cam_index == CAM_INFO_REAR2) {
			strncat(camera_info, ";DUALOPEN=N", strlen(";DUALOPEN=N"));
		}

		return sprintf(buf, "%s\n", camera_info);
	} else {
		strcpy(camera_info, "ISP=");
		switch (cam_info->isp) {
			case CAM_INFO_ISP_TYPE_INTERNAL:
				strncat(camera_info, "INT;", strlen("INT;"));
				break;
			case CAM_INFO_ISP_TYPE_EXTERNAL:
				strncat(camera_info, "EXT;", strlen("EXT;"));
				break;
			case CAM_INFO_ISP_TYPE_SOC:
				strncat(camera_info, "SOC;", strlen("SOC;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CALMEM=", strlen("CALMEM="));
		switch (cam_info->cal_memory) {
			case CAM_INFO_CAL_MEM_TYPE_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CAL_MEM_TYPE_FROM:
			case CAM_INFO_CAL_MEM_TYPE_EEPROM:
			case CAM_INFO_CAL_MEM_TYPE_OTP:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "READVER=", strlen("READVER="));
		switch (cam_info->read_version) {
			case CAM_INFO_READ_VER_SYSFS:
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_READ_VER_CAMON:
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "COREVOLT=", strlen("COREVOLT="));
		switch (cam_info->core_voltage) {
			case CAM_INFO_CORE_VOLT_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_CORE_VOLT_USE:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "UPGRADE=", strlen("UPGRADE="));
		switch (cam_info->upgrade) {
			case CAM_INFO_FW_UPGRADE_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_UPGRADE_SYSFS:
				strncat(camera_info, "SYSFS;", strlen("SYSFS;"));
				break;
			case CAM_INFO_FW_UPGRADE_CAMON:
				strncat(camera_info, "CAMON;", strlen("CAMON;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWWRITE=", strlen("FWWRITE="));
		switch (cam_info->fw_write) {
			case CAM_INFO_FW_WRITE_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_WRITE_OS:
				strncat(camera_info, "OS;", strlen("OS;"));
				break;
			case CAM_INFO_FW_WRITE_SD:
				strncat(camera_info, "SD;", strlen("SD;"));
				break;
			case CAM_INFO_FW_WRITE_ALL:
				strncat(camera_info, "ALL;", strlen("ALL;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "FWDUMP=", strlen("FWDUMP="));
		switch (cam_info->fw_dump) {
			case CAM_INFO_FW_DUMP_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_FW_DUMP_USE:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "CC=", strlen("CC="));
		switch (cam_info->companion) {
			case CAM_INFO_COMPANION_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_COMPANION_USE:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "OIS=", strlen("OIS="));
		switch (cam_info->ois) {
			case CAM_INFO_OIS_NONE:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_OIS_USE:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		strncat(camera_info, "VALID=", strlen("VALID="));
		switch (cam_info->valid) {
			case CAM_INFO_INVALID:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_VALID:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
		}

		if (cam_index == CAM_INFO_REAR2) {
			strncat(camera_info, "DUALOPEN=", strlen("DUALOPEN="));
			switch (cam_info->dual_open) {
			case CAM_INFO_SINGLE_OPEN:
				strncat(camera_info, "N;", strlen("N;"));
				break;
			case CAM_INFO_DUAL_OPEN:
				strncat(camera_info, "Y;", strlen("Y;"));
				break;
			default:
				strncat(camera_info, "NULL;", strlen("NULL;"));
				break;
			}
		}

		return sprintf(buf, "%s\n", camera_info);
	}
#endif
	strcpy(camera_info, "ISP=NULL;CALMEM=NULL;READVER=NULL;COREVOLT=NULL;UPGRADE=NULL;"
		"FWWRITE=NULL;FWDUMP=NULL;CC=NULL;OIS=NULL;VALID=N");

	if (cam_index == CAM_INFO_REAR2) {
		strncat(camera_info, ";DUALOPEN=N", strlen(";DUALOPEN=N"));
	}

	return sprintf(buf, "%s\n", camera_info);
}

static ssize_t camera_camfw_show(char *buf, enum fimc_is_cam_info_index cam_index, bool camfw_full)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_rom_info *pinfo;
	struct fimc_is_cam_info *cam_info;
	char command_ack[20] = {0, };
	char sensor_name[20] = {0, };
	char *cam_fw;
	char *loaded_fw;
	char *phone_fw;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	int ret;
	bool other_vendor;
	bool reload;

	fimc_is_vender_check_hw_init_running();
	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		ret = fimc_is_get_sensor_data(NULL, sensor_name, position);

		if (ret < 0)
			return sprintf(buf, "UNKNOWN UNKNOWN\n");
		else
			return sprintf(buf, "%s N\n", sensor_name);
	}

	if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d][%d]", __func__, cam_index, position, rom_id);
		goto err_camfw;
	}

	info("%s: [%d][%d][%d]", __func__, cam_index, position, rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_sysfs_pinfo(&pinfo, rom_id);

	reload = test_bit(FIMC_IS_ROM_STATE_CAL_RELOAD, &finfo->rom_state);

	if (fimc_is_vendor_check_camera_running(position)) {
		info("%s: camera running [%d][%d][%d]", __func__, cam_index, position, rom_id);
		reload = false;
	}

	if (!camfw_full && reload) {
		read_from_firmware_version_reload(rom_id);
	} else {
		read_from_firmware_version(rom_id);
	}

	if (!fimc_is_sec_check_rom_ver(sysfs_core, rom_id)) {
		err("%s: invalid ROM version [%d][%d][%d]", __func__, cam_index, position, rom_id);
		goto err_camfw;
	}

	if (rom_cal_index == 1)
		cam_fw = finfo->header2_ver;
	else
		cam_fw = finfo->header_ver;

	if (rom_type == ROM_TYPE_FROM) {
		fimc_is_sec_get_loaded_fw(&loaded_fw);
	} else {
		switch(position) {
			case SENSOR_POSITION_REAR:
				loaded_fw = pinfo->header_ver;
				break;
			case SENSOR_POSITION_REAR_TOF:
			case SENSOR_POSITION_FRONT_TOF:
				loaded_fw = "N";
				break;
			default :
				loaded_fw = cam_fw;
				break;
		}
	}

	if (position == SENSOR_POSITION_REAR)
		phone_fw = pinfo->header_ver;
	else
		phone_fw = "N";

	other_vendor = test_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state);

	if (!test_bit(FIMC_IS_ROM_STATE_INVALID_ROM_VERSION, &finfo->rom_state)) {
		if (finfo->crc_error == 0 && !other_vendor) {
			if (camfw_full)
				return sprintf(buf, "%s %s %s\n", cam_fw, phone_fw, loaded_fw);
			else
				return sprintf(buf, "%s %s\n", cam_fw, loaded_fw);
		} else {
			err("%s: NG CRC Check Fail [%d][%d][%d]", __func__, cam_index, position, rom_id);
			if (position == SENSOR_POSITION_REAR_TOF ||
				position == SENSOR_POSITION_FRONT_TOF) {
				strcpy(command_ack, "N");
			} else {
				strcpy(command_ack, "NG");
			}

			if (camfw_full)
				return sprintf(buf, "%s %s %s\n", cam_fw, phone_fw, command_ack);
			else
				return sprintf(buf, "%s %s\n", cam_fw, command_ack);
		}
	} else {
		err("%s: FW VER CRC Check Fail [%d][%d][%d]", __func__, cam_index, position, rom_id);
	}

err_camfw:
	if (position == SENSOR_POSITION_REAR_TOF ||
		position == SENSOR_POSITION_FRONT_TOF) {
		strcpy(command_ack, "N");
	} else {
		strcpy(command_ack, "NG");
	}

	cam_fw = "NULL";

	if (position == SENSOR_POSITION_REAR)
		phone_fw = "NULL";
	else
		phone_fw = "N";

	if (camfw_full)
		return sprintf(buf, "%s %s %s\n", cam_fw, phone_fw, command_ack);
	else
		return sprintf(buf, "%s %s\n", cam_fw, command_ack);
}

static ssize_t camera_checkfw_user_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_vender_check_hw_init_running();
	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d][%d]", __func__, cam_index, position, rom_id);
		goto err_user_checkfw;
	}

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	read_from_firmware_version(rom_id);

	if (!fimc_is_sec_check_rom_ver(sysfs_core, rom_id)) {
		err(" NG, invalid ROM version");
		goto err_user_checkfw;
	}

	if (!test_bit(FIMC_IS_ROM_STATE_INVALID_ROM_VERSION, &finfo->rom_state)) {
		if (test_bit(FIMC_IS_CRC_ERROR_FIRMWARE, &finfo->crc_error)
			|| test_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error)
			|| test_bit(FIMC_IS_CRC_ERROR_DUAL_CAMERA, &finfo->crc_error)) {
			if (!test_bit(FIMC_IS_ROM_STATE_LATEST_MODULE, &finfo->rom_state)) {
				err(" NG, not latest cam module");
				return sprintf(buf, "%s\n", "NG");
			} else {
				return sprintf(buf, "%s\n", "OK");
			}
		} else {
			err(" NG, crc check fail");
			return sprintf(buf, "%s\n", "NG");
		}
	} else {
		err(" NG, fw ver crc check fail");
	}

err_user_checkfw:
	return sprintf(buf, "%s\n", "NG");
}

static ssize_t camera_checkfw_factory_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_vender_check_hw_init_running();
	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d][%d]", __func__, cam_index, position, rom_id);
		goto err_factory_checkfw;
	}

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	read_from_firmware_version(rom_id);

	if (!fimc_is_sec_check_rom_ver(sysfs_core, rom_id)) {
		err(" NG, invalid ROM version");
		goto err_factory_checkfw;
	}

	if (!test_bit(FIMC_IS_ROM_STATE_INVALID_ROM_VERSION, &finfo->rom_state)) {
		if (finfo->crc_error == 0) {
			if (!test_bit(FIMC_IS_ROM_STATE_FINAL_MODULE, &finfo->rom_state)) {
				err(" NG, not final cam module");
				return sprintf(buf, "%s\n", "NG_VER");
			} else {
				return sprintf(buf, "%s\n", "OK");
			}
		} else {
			err(" NG, crc check fail");
			return sprintf(buf, "%s\n", "NG_CRC");
		}
	} else {
		err(" NG, fw ver crc check fail");
	}

err_factory_checkfw:
	return sprintf(buf, "%s\n", "NG_VER");
}

static ssize_t camera_sensorid_exif_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_sensorid_exif;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err_sensorid_exif;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	if (rom_cal_index == 1)
		memcpy(buf, finfo->rom_sensor2_id, FIMC_IS_SENSOR_ID_SIZE);
	else
		memcpy(buf, finfo->rom_sensor_id, FIMC_IS_SENSOR_ID_SIZE);

	return FIMC_IS_SENSOR_ID_SIZE;

err_sensorid_exif:
	memset(buf, '\0', FIMC_IS_SENSOR_ID_SIZE);

	return FIMC_IS_SENSOR_ID_SIZE;
}

static ssize_t camera_mtf_exif_show(char *buf, enum fimc_is_cam_info_index cam_index, int f_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	int mtf_offset;
	char *cal_buf;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_mtf_exif;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err_mtf_exif;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	if (rom_cal_index == 1) {
		mtf_offset = finfo->rom_header_sensor2_mtf_data_addr;
	} else {
		switch(f_index) {
		case FNUMBER_1ST :
			mtf_offset = finfo->rom_header_mtf_data_addr;
			break;
		case FNUMBER_2ND :
			mtf_offset = finfo->rom_header_f2_mtf_data_addr;
			break;
		case FNUMBER_3RD :
			mtf_offset = finfo->rom_header_f3_mtf_data_addr;
			break;
		default :
			err("%s: invalid f_index [%d][%d], so set default", __func__, position, f_index);
			mtf_offset = finfo->rom_header_mtf_data_addr;
			break;
		}
	}

	if (mtf_offset != -1)
		memcpy(buf, &cal_buf[mtf_offset], FIMC_IS_RESOLUTION_DATA_SIZE);
	else
		memset(buf, 0, FIMC_IS_RESOLUTION_DATA_SIZE);

	return FIMC_IS_RESOLUTION_DATA_SIZE;

err_mtf_exif:
	memset(buf, '\0', FIMC_IS_RESOLUTION_DATA_SIZE);

	return FIMC_IS_RESOLUTION_DATA_SIZE;
}

static ssize_t camera_moduleid_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;

	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_moduleid;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err_moduleid;
	}

	read_from_firmware_version(rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "%c%c%c%c%c%02X%02X%02X%02X%02X\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2],
			finfo->rom_module_id[3], finfo->rom_module_id[4], finfo->rom_module_id[5],
			finfo->rom_module_id[6], finfo->rom_module_id[7], finfo->rom_module_id[8],
			finfo->rom_module_id[9]);
	}

err_moduleid:
	return sprintf(buf, "%s\n", "0000000000");
}

static ssize_t camera_afcal_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;

	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	bool is_front = false;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_afcal;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err_afcal;
	}

	read_from_firmware_version(rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	if (position == SENSOR_POSITION_FRONT ||
		position == SENSOR_POSITION_FRONT2 ||
		position == SENSOR_POSITION_FRONT3 ||
		position == SENSOR_POSITION_FRONT4 ) {
		is_front = true;
	}

	if (rom_cal_index == 1) {
		if (finfo->rom_sensor2_af_cal_macro_addr == -1) {
			return sprintf(buf, "10 N N N N %d N N N %d\n",
				*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_d50_addr]),
				*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_pan_addr]));
		} else if (finfo->rom_sensor2_af_cal_d50_addr != -1) {
			return sprintf(buf, "10 %d N N N %d N N N %d\n",
				*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_macro_addr]),
				*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_d50_addr]),
				*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_pan_addr]));
		} else {
			if (is_front)
				return sprintf(buf, "1 %d %d\n",
					*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_macro_addr]),
					*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_pan_addr]));
			else
				return sprintf(buf, "10 %d N N N N N N N %d\n",
					*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_macro_addr]),
					*((s32*)&cal_buf[finfo->rom_sensor2_af_cal_pan_addr]));
		}
	} else {
		if (finfo->rom_af_cal_macro_addr == -1) {
			return sprintf(buf, "10 N N N N %d N N N %d\n",
				*((s32*)&cal_buf[finfo->rom_af_cal_d50_addr]),
				*((s32*)&cal_buf[finfo->rom_af_cal_pan_addr]));
		} else if (finfo->rom_af_cal_d50_addr != -1) {
			return sprintf(buf, "10 %d N N N %d N N N %d\n",
				*((s32*)&cal_buf[finfo->rom_af_cal_macro_addr]),
				*((s32*)&cal_buf[finfo->rom_af_cal_d50_addr]),
				*((s32*)&cal_buf[finfo->rom_af_cal_pan_addr]));
		} else {
			if (is_front)
				return sprintf(buf, "1 %d %d\n",
					*((s32*)&cal_buf[finfo->rom_af_cal_macro_addr]),
					*((s32*)&cal_buf[finfo->rom_af_cal_pan_addr]));
			else
				return sprintf(buf, "10 %d N N N N N N N %d\n",
					*((s32*)&cal_buf[finfo->rom_af_cal_macro_addr]),
					*((s32*)&cal_buf[finfo->rom_af_cal_pan_addr]));
		}
	}

err_afcal:
	return sprintf(buf, "1 N N\n");
}

static ssize_t camera_awb_master_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_awb_master;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err_awb_master;
	}

	read_from_firmware_version(rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	if (rom_cal_index == 1) {
		memcpy(buf, &cal_buf[finfo->rom_sensor2_awb_master_addr], FIMC_IS_AWB_MASTER_DATA_SIZE);
	} else {
		memcpy(buf, &cal_buf[finfo->rom_awb_master_addr], FIMC_IS_AWB_MASTER_DATA_SIZE);
	}

	return FIMC_IS_AWB_MASTER_DATA_SIZE;

err_awb_master:
	memset(buf, 0xFF, FIMC_IS_AWB_MASTER_DATA_SIZE);

	return FIMC_IS_AWB_MASTER_DATA_SIZE;
}


static ssize_t camera_awb_module_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_afcal;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err_afcal;
	}

	read_from_firmware_version(rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	if (rom_cal_index == 1) {
		memcpy(buf, &cal_buf[finfo->rom_sensor2_awb_module_addr], FIMC_IS_AWB_MODULE_DATA_SIZE);
	} else {
		memcpy(buf, &cal_buf[finfo->rom_awb_module_addr], FIMC_IS_AWB_MODULE_DATA_SIZE);
	}

	return FIMC_IS_AWB_MODULE_DATA_SIZE;

err_afcal:
	memset(buf, 0xFF, FIMC_IS_AWB_MODULE_DATA_SIZE);

	return FIMC_IS_AWB_MODULE_DATA_SIZE;
}

static ssize_t camera_tilt_show(char *buf, enum fimc_is_cam_info_index cam_index)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	char *cal_buf;
	int position;
	int rom_type;
	int rom_dualcal_id;
	int rom_dualcal_index;
	char tempbuf[25] = {0, };
	int i = 0;

	fimc_is_get_cam_info_from_index(&cam_info, cam_index);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_dualcal_info_from_position(position, &rom_type, &rom_dualcal_id, &rom_dualcal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, cam_index, position);
		goto err_tilt;
	} else if (rom_dualcal_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_dualcal_id);
		goto err_tilt;
	}

	read_from_firmware_version(rom_dualcal_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_dualcal_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_dualcal_id);

	if (!fimc_is_sec_check_rom_ver(sysfs_core, rom_dualcal_id)){
		err(" NG, invalid ROM version");
		goto err_tilt;
	}

	if (test_bit(FIMC_IS_ROM_STATE_INVALID_ROM_VERSION, &finfo->rom_state)
		|| test_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state)
		|| test_bit(FIMC_IS_CRC_ERROR_DUAL_CAMERA, &finfo->crc_error)) {
		err(" NG, invalid ROM version");
		goto err_tilt;
	}

	strcat(buf, "1");

	switch (rom_dualcal_index)
	{
	case ROM_DUALCAL_SLAVE0:
		if (finfo->rom_dualcal_slave0_tilt_list_len < 0
			|| finfo->rom_dualcal_slave0_tilt_list_len > FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
			err(" NG, invalid ROM dual tilt value, dualcal_index[%d]", rom_dualcal_index);
			goto err_tilt;
		}

		for (i = 0; i < FIMC_IS_ROM_DUAL_TILT_MAX_LIST - 1; i++) {
			sprintf(tempbuf, " %d", *((s32 *)&cal_buf[finfo->rom_dualcal_slave0_tilt_list[i]]));
			strncat(buf, tempbuf, strlen(tempbuf));
			memset(tempbuf, 0, sizeof(tempbuf));
		}

		if (finfo->rom_dualcal_slave0_tilt_list_len == FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
			strcat(buf, " ");
			memcpy(tempbuf, &cal_buf[finfo->rom_dualcal_slave0_tilt_list[i]], FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE);
			tempbuf[FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE] = '\0';
			for (i = 0; i < FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE; i++) {
				if (tempbuf[i] == 0xFF && i == 0) {
					sprintf(tempbuf, "NONE");
					break;
				}
				if (tempbuf[i] == 0) {
					tempbuf[i] = '\0';
					break;
				}
			}
			strncat(buf, tempbuf, strlen(tempbuf));
		}
		strncat(buf, "\n", strlen("\n"));

		return strlen(buf);
	case ROM_DUALCAL_SLAVE1:
		if (finfo->rom_dualcal_slave1_tilt_list_len < 0
			|| finfo->rom_dualcal_slave1_tilt_list_len > FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
			err(" NG, invalid ROM dual tilt value, dualcal_index[%d]", rom_dualcal_index);
			goto err_tilt;
		}

		for (i = 0; i < FIMC_IS_ROM_DUAL_TILT_MAX_LIST - 1; i++) {
			sprintf(tempbuf, " %d", *((s32 *)&cal_buf[finfo->rom_dualcal_slave1_tilt_list[i]]));
			strncat(buf, tempbuf, strlen(tempbuf));
			memset(tempbuf, 0, sizeof(tempbuf));
		}

		if (finfo->rom_dualcal_slave1_tilt_list_len == FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
			strcat(buf, " ");
			memcpy(tempbuf, &cal_buf[finfo->rom_dualcal_slave1_tilt_list[i]], FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE);
			tempbuf[FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE] = '\0';
			for (i = 0; i < FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE; i++) {
				if (tempbuf[i] == 0xFF && i == 0) {
					sprintf(tempbuf, "NONE");
					break;
				}
				if (tempbuf[i] == 0) {
					tempbuf[i] = '\0';
					break;
				}
			}
			strncat(buf, tempbuf, strlen(tempbuf));
		}
		strncat(buf, "\n", strlen("\n"));

		return strlen(buf);
	case ROM_DUALCAL_SLAVE2:
		if (finfo->rom_dualcal_slave2_tilt_list_len < 0
			|| finfo->rom_dualcal_slave2_tilt_list_len > FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
			err(" NG, invalid ROM dual tilt value, dualcal_index[%d]", rom_dualcal_index);
			goto err_tilt;
		}

		for (i = 0; i < FIMC_IS_ROM_DUAL_TILT_MAX_LIST - 1; i++) {
			sprintf(tempbuf, " %d", *((s32 *)&cal_buf[finfo->rom_dualcal_slave2_tilt_list[i]]));
			strncat(buf, tempbuf, strlen(tempbuf));
			memset(tempbuf, 0, sizeof(tempbuf));
		}

		if (finfo->rom_dualcal_slave2_tilt_list_len == FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
			strcat(buf, " ");
			memcpy(tempbuf, &cal_buf[finfo->rom_dualcal_slave2_tilt_list[i]], FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE);
			tempbuf[FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE] = '\0';
			for (i = 0; i < FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE; i++) {
				if (tempbuf[i] == 0xFF && i == 0) {
					sprintf(tempbuf, "NONE");
					break;
				}
				if (tempbuf[i] == 0) {
					tempbuf[i] = '\0';
					break;
				}
			}
			strncat(buf, tempbuf, strlen(tempbuf));
		}
		strncat(buf, "\n", strlen("\n"));

		return strlen(buf);
	default:
		err("not defined tilt cal values");
		break;
	}

err_tilt:
	return sprintf(buf, "%s\n", "NG");
}

/* PAF offset read */
static ssize_t camera_paf_offset_show(char *buf, bool is_mid, int f_index)
{
	char *cal_buf;
	char tempbuf[10];
	int i;
	int divider;
	int paf_offset_count;
	u32 paf_offset_base = 0;

	switch(f_index) {
	case FNUMBER_1ST :
#ifdef ROM_PAF_CAL_DATA_START_ADDR
		if (is_mid)
			paf_offset_base = ROM_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_MID_OFFSET;
		else
			paf_offset_base = ROM_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_PAN_OFFSET;
#endif
		break;
	case FNUMBER_2ND :
#ifdef ROM_F2_PAF_CAL_DATA_START_ADDR
		if (is_mid)
			paf_offset_base = ROM_F2_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_MID_OFFSET;
		else
			paf_offset_base = ROM_F2_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_PAN_OFFSET;
#endif
		break;
	case FNUMBER_3RD :
#ifdef ROM_F3_PAF_CAL_DATA_START_ADDR
		if (is_mid)
			paf_offset_base = ROM_F3_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_MID_OFFSET;
		else
			paf_offset_base = ROM_F3_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_PAN_OFFSET;
#endif
		break;
	default :
		err("%s: invalid f_index [%d][%d], so set default", __func__, is_mid, f_index);
#ifdef ROM_PAF_CAL_DATA_START_ADDR
		if (is_mid)
			paf_offset_base = ROM_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_MID_OFFSET;
		else
			paf_offset_base = ROM_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_OFFSET_PAN_OFFSET;
#endif
		break;
	}

	if (paf_offset_base == 0) {
		strncat(buf, "\n", strlen("\n"));
		err("%s: no definition for paf offset mid/pan[%d] f[%d]", __func__, is_mid, f_index);
		return strlen(buf);
	}

	if (is_mid) {
		divider = 8;
		paf_offset_count = FIMC_IS_PAF_OFFSET_MID_SIZE / divider;
	} else {
		divider = 2;
		paf_offset_count = FIMC_IS_PAF_OFFSET_PAN_SIZE / divider;
	}

	memset(tempbuf, 0, sizeof(tempbuf));
	read_from_firmware_version(ROM_ID_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);

	for (i = 0; i < paf_offset_count - 1; i++) {
		sprintf(tempbuf, "%d,", *((s16 *)&cal_buf[paf_offset_base + divider * i]));
		strncat(buf, tempbuf, strlen(tempbuf));
		memset(tempbuf, 0, sizeof(tempbuf));
	}
	sprintf(tempbuf, "%d", *((s16 *)&cal_buf[paf_offset_base + divider * i]));
	strncat(buf, tempbuf, strlen(tempbuf));

	strncat(buf, "\n", strlen("\n"));
	return strlen(buf);
}

/* end of common function for sysfs */

#if defined (CONFIG_OIS_USE)
static bool read_ois_version(void)
{
	bool ret = true;
	struct fimc_is_vender_specific *specific = sysfs_core->vender.private_data;
	bool camera_running;

	camera_running = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR);

	if (!camera_running) {
		if (!specific->ois_ver_read || force_caldata_dump) {
			fimc_is_ois_gpio_on(sysfs_core);
			msleep(150);

			ret = fimc_is_ois_check_fw(sysfs_core);
			if (!camera_running) {
				fimc_is_ois_gpio_off(sysfs_core);
			}
		}
	}
	return ret;
}
#endif

static ssize_t camera_rear_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;
	int ret = 0;
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;

	if (force_caldata_dump == false) {
		ret = fimc_is_sec_run_fw_sel(is_dev, ROM_ID_REAR);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
		}
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear_sensor_id);
}

static ssize_t camera_rear2_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear2_sensor_id);
}

static ssize_t camera_rear3_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear3_sensor_id);
}

static ssize_t camera_rear_tof_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear_tof_sensor_id);
}

static ssize_t camera_front_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	int ret = 0;
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;

	if (force_caldata_dump == false) {
		ret = fimc_is_sec_run_fw_sel(is_dev, ROM_ID_FRONT);
		if (ret) {
			err("fimc_is_sec_run_fw_sel is fail(%d)", ret);
		}
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;

	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->front_sensor_id);
}

static ssize_t camera_front2_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->front2_sensor_id);
}

static ssize_t camera_front_tof_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->front_tof_sensor_id);
}

#ifdef SECURE_CAMERA_IRIS
static ssize_t camera_secure_sensorid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;

	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->secure_sensor_id);
}
#endif

static ssize_t camera_ssrm_camera_info_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct ssrm_camera_data temp;
	int ret_count;
	int index = -1;
	int i = 0;

	memset(&temp, 0, sizeof(temp));
	temp.cameraID = -1;

	ret_count = sscanf(buf, "%d%d%d%d%d%d%d", &temp.operation, &temp.cameraID, &temp.previewMinFPS,
		&temp.previewMaxFPS, &temp.previewSizeWidth,  &temp.previewSizeHeight, &temp.sensorOn);

	if (ret_count > sizeof(SsrmCameraInfo)/sizeof(int)) {
		return -EINVAL;
	}

	switch (temp.operation) {
	case SSRM_CAMERA_INFO_CLEAR:
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) { /* clear */
			if (SsrmCameraInfo[i].cameraID == temp.cameraID) {
				SsrmCameraInfo[i].previewMaxFPS = 0;
				SsrmCameraInfo[i].previewMinFPS = 0;
				SsrmCameraInfo[i].previewSizeHeight = 0;
				SsrmCameraInfo[i].previewSizeWidth = 0;
				SsrmCameraInfo[i].sensorOn = 0;
				SsrmCameraInfo[i].cameraID = -1;
			}
		}
		break;

	case SSRM_CAMERA_INFO_SET:
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) { /* find empty space*/
			if (SsrmCameraInfo[i].cameraID == -1) {
				index = i;
				break;
			}
		}

		if (index == -1)
			return -EPERM;

		memcpy(&SsrmCameraInfo[i], &temp, sizeof(temp));
		break;

	case SSRM_CAMERA_INFO_UPDATE:
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
			if (SsrmCameraInfo[i].cameraID == temp.cameraID) {
				SsrmCameraInfo[i].previewMaxFPS = temp.previewMaxFPS;
				SsrmCameraInfo[i].previewMinFPS = temp.previewMinFPS;
				SsrmCameraInfo[i].previewSizeHeight = temp.previewSizeHeight;
				SsrmCameraInfo[i].previewSizeWidth = temp.previewSizeWidth;
				SsrmCameraInfo[i].sensorOn = temp.sensorOn;
				break;
			}
		}
		break;
	default:
		break;
	}

	return count;
}

static ssize_t camera_ssrm_camera_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp_buffer[50] = {0,};
	int i = 0;

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		if (SsrmCameraInfo[i].cameraID != -1) {
			strncat(buf, "ID=", strlen("ID="));
			sprintf(temp_buffer, "%d;", SsrmCameraInfo[i].cameraID);
			strncat(buf, temp_buffer, strlen(temp_buffer));

			strncat(buf, "ON=", strlen("ON="));
			sprintf(temp_buffer, "%d;", SsrmCameraInfo[i].sensorOn);
			strncat(buf, temp_buffer, strlen(temp_buffer));

			if (SsrmCameraInfo[i].previewMinFPS && SsrmCameraInfo[i].previewMaxFPS) {
				strncat(buf, "FPS=", strlen("FPS="));
				sprintf(temp_buffer, "%d,%d;",
					SsrmCameraInfo[i].previewMinFPS, SsrmCameraInfo[i].previewMaxFPS);
				strncat(buf, temp_buffer, strlen(temp_buffer));
			}
			if (SsrmCameraInfo[i].previewSizeWidth && SsrmCameraInfo[i].previewSizeHeight) {
				strncat(buf, "SIZE=", strlen("SIZE="));
				sprintf(temp_buffer, "%d,%d;",
					SsrmCameraInfo[i].previewSizeWidth, SsrmCameraInfo[i].previewSizeHeight);
				strncat(buf, temp_buffer, strlen(temp_buffer));
			}
			strncat(buf, "\n", strlen("\n"));
		}
	}
	return strlen(buf);
}

static ssize_t camera_front_camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_maker[50];
	char sensor_name[50];
	int ret;

	ret = fimc_is_get_sensor_data(sensor_maker, sensor_name, SENSOR_POSITION_FRONT);

	if (ret < 0)
		return sprintf(buf, "UNKNOWN_UNKNOWN_FIMC_IS\n");
	else
		return sprintf(buf, "%s_%s_FIMC_IS\n", sensor_maker, sensor_name);
}

static ssize_t camera_front_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_FRONT, false);
}

static ssize_t camera_front_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_FRONT, true);
}

static ssize_t camera_front_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_FRONT);
}

static ssize_t camera_front_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_FRONT);
}

#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
static ssize_t camera_front_mipi_clock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis = NULL;
	char mipi_string[20] = {0, };
	int i;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		dev_err(dev, "%s: core is NULL", __func__);
		return -EINVAL;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		fimc_is_search_sensor_module_with_position(&core->sensor[i],
				SENSOR_POSITION_FRONT, &module);
		if (module)
			break;
	}

	WARN_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	WARN_ON(!sensor_peri);

	if (sensor_peri->subdev_cis) {
		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		CALL_CISOPS(cis, cis_get_mipi_clock_string, sensor_peri->subdev_cis, mipi_string);
	}

	return sprintf(buf, "%s\n", mipi_string);
}
#endif

#ifdef CAMERA_FRONT2
static ssize_t camera_front2_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_FRONT2);
}

static ssize_t camera_front2_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_FRONT2, false);
}

static ssize_t camera_front2_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_FRONT2, true);
}

static ssize_t camera_front2_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_FRONT2);
}

static ssize_t camera_front2_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_FRONT2, FNUMBER_1ST);
}

static ssize_t camera_front2_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_FRONT2);
}

#ifdef CAMERA_FRONT2_MODULEID
static ssize_t camera_front2_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_FRONT2);
}
#endif

#ifndef CAMERA_FRONT2_USE_DEFAULT_AWB_CAL
static ssize_t camera_front2_awb_master_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_master_show(buf, CAM_INFO_FRONT2);
}

static ssize_t camera_front2_awb_module_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_module_show(buf, CAM_INFO_FRONT2);
}
#endif

#ifdef CAMERA_FRONT2_TILT
static ssize_t camera_front2_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_tilt_show(buf, CAM_INFO_FRONT2);
}
#endif

#endif

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
static ssize_t camera_rear_writefw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
/* Disabled for product. Security Hole */
#if 0
	struct device *is_dev = &sysfs_core->ischain[0].pdev->dev;
	int ret = 0;

	ret = fimc_is_sec_write_fw(sysfs_core, is_dev);

	if (ret)
		return sprintf(buf, "NG\n");
	else
		return sprintf(buf, "OK\n");
#else
	return sprintf(buf, "OK\n");
#endif
}
#endif

static ssize_t camera_rear_camtype_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_maker[50];
	char sensor_name[50];
	int ret;

	ret = fimc_is_get_sensor_data(sensor_maker, sensor_name, SENSOR_POSITION_REAR);

	if (ret < 0)
		return sprintf(buf, "UNKNOWN_UNKNOWN_FIMC_IS\n");
	else
		return sprintf(buf, "%s_%s_FIMC_IS\n", sensor_maker, sensor_name);
}

static ssize_t camera_rear_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR, false);
}

static ssize_t camera_rear_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR, true);
}

static ssize_t camera_rear_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_user_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_rear_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_rear_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_supported_cameraIds_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char temp_buf[FIMC_IS_SENSOR_COUNT];
	char *end = "\n";
	int i;

	for(i = 0; i < common_cam_infos.max_supported_camera; i++)	{
		sprintf(temp_buf, "%d ", common_cam_infos.supported_camera_ids[i]);
		strncat(buf, temp_buf, strlen(temp_buf));
	}
	strncat(buf, end, strlen(end));

	return strlen(buf);
}

#ifdef CAMERA_REAR_DUAL_CAL
static ssize_t camera_rear_dualcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	int copy_size = 0;
	char *temp_buf;

	ret = fimc_is_get_dual_cal_buf(SENSOR_POSITION_REAR, &temp_buf, &copy_size);
	if (ret < 0) {
		err("%s: Fail to get dualcal of %d", __func__, SENSOR_POSITION_REAR);
	} else {
		info("%s: success to get dualcal of %d", __func__, SENSOR_POSITION_REAR);
		memcpy(buf, temp_buf, copy_size);
	}

	return copy_size;
}

static ssize_t camera_rear2_dualcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	int copy_size = 0;
	char *temp_buf;

	ret = fimc_is_get_dual_cal_buf(SENSOR_POSITION_REAR3, &temp_buf, &copy_size);
	if (ret < 0) {
		err("%s: Fail to get dualcal of %d", __func__, SENSOR_POSITION_REAR3);
	} else {
		info("%s: success to get dualcal of %d", __func__, SENSOR_POSITION_REAR3);
		memcpy(buf, temp_buf, copy_size);
	}

	return copy_size;
}

static ssize_t camera_rear3_dualcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	int copy_size = 0;
	char *temp_buf;

	ret = fimc_is_get_dual_cal_buf(SENSOR_POSITION_REAR2, &temp_buf, &copy_size);
	if (ret < 0) {
		err("%s: Fail to get dualcal of %d", __func__, SENSOR_POSITION_REAR2);
	} else {
		info("%s: success to get dualcal of %d", __func__, SENSOR_POSITION_REAR2);
		memcpy(buf, temp_buf, copy_size);
	}

	return copy_size;
}
#endif

#ifdef CAMERA_FRONT_DUAL_CAL
static ssize_t camera_front_dualcal_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int ret;
	int copy_size = 0;
	char *temp_buf;

	ret = fimc_is_get_dual_cal_buf(SENSOR_POSITION_FRONT, &temp_buf, &copy_size);
	if (ret < 0) {
		err("%s: Fail to get dualcal of %d", __func__, SENSOR_POSITION_FRONT);
	} else {
		info("%s: success to get dualcal of %d", __func__, SENSOR_POSITION_FRONT);
		memcpy(buf, temp_buf, copy_size);
	}

	return copy_size;
}

static ssize_t camera_front2_dualcal_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int ret;
	int copy_size = 0;
	char *temp_buf;

	ret = fimc_is_get_dual_cal_buf(SENSOR_POSITION_FRONT2, &temp_buf, &copy_size);
	if (ret < 0) {
		err("%s: Fail to get dualcal of %d", __func__, SENSOR_POSITION_FRONT2);
	} else {
		info("%s: success to get dualcal of %d", __func__, SENSOR_POSITION_FRONT2);
		memcpy(buf, temp_buf, copy_size);
	}

	return copy_size;
}
#endif

#ifdef CAMERA_REAR2
static ssize_t camera_rear2_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_REAR2);
}

static ssize_t camera_rear2_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR2, false);
}

static ssize_t camera_rear2_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR2, true);
}

static ssize_t camera_rear2_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_user_show(buf, CAM_INFO_REAR2);
}

static ssize_t camera_rear2_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_REAR2);
}

static ssize_t camera_rear2_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_REAR2);
}

static ssize_t camera_rear2_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_REAR2, FNUMBER_1ST);
}

#ifdef CAMERA_REAR2_MODULEID
static ssize_t camera_rear2_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_REAR2);
}
#endif

static ssize_t camera_rear2_awb_master_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_master_show(buf, CAM_INFO_REAR2);
}

static ssize_t camera_rear2_awb_module_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_module_show(buf, CAM_INFO_REAR2);
}

#ifdef CAMERA_REAR2_AFCAL
static ssize_t camera_rear2_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_afcal_show(buf, CAM_INFO_REAR2);
}
#endif

#ifdef CAMERA_REAR2_TILT
static ssize_t camera_rear2_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_tilt_show(buf, CAM_INFO_REAR2);
}
#endif
#endif

#ifdef CAMERA_REAR3
static ssize_t camera_rear3_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_REAR3);
}

static ssize_t camera_rear3_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR3, false);
}

static ssize_t camera_rear3_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR3, true);
}

static ssize_t camera_rear3_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_user_show(buf, CAM_INFO_REAR3);
}

static ssize_t camera_rear3_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_REAR3);
}

static ssize_t camera_rear3_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_REAR3);
}

static ssize_t camera_rear3_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_REAR3, FNUMBER_1ST);
}

static ssize_t camera_rear3_paf_cal_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;
	u8 data[5] = {0, };
	u32 paf_err_data_result = 0;

	read_from_firmware_version(ROM_ID_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);

	memcpy(data, &cal_buf[ROM_REAR3_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_CAL_ERR_CHECK_OFFSET], 4);
	paf_err_data_result = *data | ( *(data + 1) << 8) | ( *(data + 2) << 16) | (*(data + 3) << 24);

	return sprintf(buf, "%08X\n", paf_err_data_result);
}

#ifdef CAMERA_REAR3_MODULEID
static ssize_t camera_rear3_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_REAR3);
}
#endif

static ssize_t camera_rear3_awb_master_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_master_show(buf, CAM_INFO_REAR3);
}

static ssize_t camera_rear3_awb_module_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_module_show(buf, CAM_INFO_REAR3);
}

#ifdef CAMERA_REAR3_AFCAL
static ssize_t camera_rear3_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_afcal_show(buf, CAM_INFO_REAR3);
}
#endif

#ifdef CAMERA_REAR3_TILT
static ssize_t camera_rear3_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_tilt_show(buf, CAM_INFO_REAR3);
}
#endif

#endif

#ifdef CAMERA_REAR_TOF
static ssize_t camera_rear_tof_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_REAR_TOF);
}

static ssize_t camera_rear_tof_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR_TOF, false);
}

static ssize_t camera_rear_tof_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_REAR_TOF, true);
}

static ssize_t camera_rear_tof_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_REAR_TOF);
}

static ssize_t camera_rear_tof_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_REAR_TOF);
}

#ifdef CAMERA_REAR4_TOF_MODULEID
static ssize_t camera_rear_tof_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_REAR_TOF);
}
#endif

#ifdef CAMERA_REAR_TOF_CAL
static ssize_t camera_rear_tof_get_validation_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	u16 val_data[2];

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	val_data[0] = *(u16*)&cal_buf[finfo->rom_tof_cal_validation_addr[1]]; 		/* 2Byte from 121E (300mm validation) */
	val_data[1] = *(u16*)&cal_buf[finfo->rom_tof_cal_validation_addr[0]];		/* 2Byte from 1200 (500mm validation) */

	return sprintf(buf, "%d,%d", val_data[0]/100, val_data[1]/100);

err:
	return 0;
}

static ssize_t camera_rear_tofcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = *((s32*)&cal_buf[finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len-1]]) +
		(finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len - 1] - finfo->rom_tof_cal_start_addr + 4);

	if (cal_size > FIMC_IS_TOF_CAL_SIZE_ONCE)
		cal_size = FIMC_IS_TOF_CAL_SIZE_ONCE;

	memcpy(buf, &cal_buf[finfo->rom_tof_cal_start_addr], cal_size);
	memcpy(buf + cal_size, &cal_buf[finfo->rom_tof_cal_validation_addr[0]], 2); /* 2byte validation data */
	memcpy(buf + cal_size + 2, &cal_buf[finfo->rom_tof_cal_validation_addr[1]], 2); /* 2byte validation data */

	cal_size += 4;

	return cal_size;

err:
	return 0;
}

static ssize_t camera_rear_tofcal_extra_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = *((s32*)&cal_buf[finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len-1]]) +
		(finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len - 1] - finfo->rom_tof_cal_start_addr + 4);

	cal_size -= FIMC_IS_TOF_CAL_SIZE_ONCE;

	if (cal_size > FIMC_IS_TOF_CAL_SIZE_ONCE)
		cal_size = FIMC_IS_TOF_CAL_SIZE_ONCE;

	memcpy(buf, &cal_buf[finfo->rom_tof_cal_start_addr + FIMC_IS_TOF_CAL_SIZE_ONCE], cal_size);
	return cal_size;

err:
	return 0;
}

static ssize_t camera_rear_tofcal_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = *((s32*)&cal_buf[finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len-1]]) +
		(finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len - 1] - finfo->rom_tof_cal_start_addr + 4);

	cal_size += 4;	/* 4 bytes for validation data <rom_tof_cal_validation_addr> */

	return sprintf(buf, "%d\n", cal_size);

err:
	return sprintf(buf, "0");
}

static ssize_t camera_rear_tofcal_uid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->rear_tof_uid[0]);
}

static ssize_t camera_rear_tof_dual_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = finfo->rom_dualcal_slave2_size;

	memcpy(buf, &cal_buf[finfo->rom_dualcal_slave2_start_addr], cal_size);
	return cal_size;

err:
	return 0;
}

static ssize_t camera_rear_tof_cal_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	if (cal_buf[finfo->rom_tof_cal_result_addr] == FIMC_IS_TOF_CAL_CAL_RESULT_OK         /* CAT TREE */
		&& cal_buf[finfo->rom_tof_cal_result_addr + 2] == FIMC_IS_TOF_CAL_CAL_RESULT_OK  /* TRUN TABLE */
		&& cal_buf[finfo->rom_tof_cal_result_addr + 4] == FIMC_IS_TOF_CAL_CAL_RESULT_OK) /* VALIDATION */ {
		return sprintf(buf, "OK\n");
	} else {
		return sprintf(buf, "NG\n");
	}

err:
	return sprintf(buf, "NG\n");
}
#endif
#ifdef CAMERA_REAR_TOF_TILT
static ssize_t camera_rear_tof_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_tilt_show(buf, CAM_INFO_REAR_TOF);
}
#endif

#ifdef CAMERA_REAR2_TOF_TILT
static ssize_t camera_rear2_tof_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	char *cal_buf;
	int position;
	int rom_type;
	int rom_dualcal_id;
	int rom_dualcal_index;
	s32 *x = NULL, *y = NULL, *z = NULL, *sx = NULL, *sy = NULL;
	s32 *range = NULL, *max_err = NULL, *avg_err = NULL, *dll_version = NULL;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_dualcal_info_from_position(position, &rom_type, &rom_dualcal_id, &rom_dualcal_index);

	if (rom_type == ROM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_REAR_TOF, position);
		goto err_tilt;
	} else if (rom_dualcal_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_dualcal_id);
		goto err_tilt;
	}

	read_from_firmware_version(rom_dualcal_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_dualcal_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_dualcal_id);

	if (!fimc_is_sec_check_rom_ver(sysfs_core, rom_dualcal_id)){
		err(" NG, invalid ROM version");
		goto err_tilt;
	}

	if (test_bit(FIMC_IS_ROM_STATE_INVALID_ROM_VERSION, &finfo->rom_state)
		|| test_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state)
		|| test_bit(FIMC_IS_CRC_ERROR_DUAL_CAMERA, &finfo->crc_error)) {
		err(" NG, invalid ROM version");
		goto err_tilt;
	}

	if (finfo->rom_dualcal_slave3_tilt_list_len < 0
		|| finfo->rom_dualcal_slave3_tilt_list_len > FIMC_IS_ROM_DUAL_TILT_MAX_LIST) {
		err(" NG, invalid ROM dual tilt value, dualcal_index[%d]", rom_dualcal_index);
		goto err_tilt;
	}
	x = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[0]];
	y = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[1]];
	z = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[2]];
	sx = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[3]];
	sy = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[4]];
	range = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[5]];
	max_err = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[6]];
	avg_err = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[7]];
	dll_version = (s32 *)&cal_buf[finfo->rom_dualcal_slave3_tilt_list[8]];

	return sprintf(buf, "1 %d %d %d %d %d %d %d %d %d\n",
							*x, *y, *z, *sx, *sy, *range, *max_err, *avg_err, *dll_version);
err_tilt:
	return sprintf(buf, "%s\n", "NG");
}
#endif

static int camera_tof_set_laser_current(int position, u8 value)
{
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis = NULL;
	int i;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("%s: core is NULL", __func__);
		return -EINVAL;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		fimc_is_search_sensor_module_with_position(&core->sensor[i],
				position, &module);
		if (module)
			break;
	}

	WARN_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	WARN_ON(!sensor_peri);

	if (sensor_peri->subdev_cis) {
		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		CALL_CISOPS(cis, cis_set_laser_current, sensor_peri->subdev_cis, value);
	}

	return 0;
}

static int camera_tof_get_laser_photo_diode(int position, u16 *value)
{
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis = NULL;
	int i;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("%s: core is NULL", __func__);
		return -EINVAL;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		fimc_is_search_sensor_module_with_position(&core->sensor[i],
				position, &module);
		if (module)
			break;
	}

	WARN_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	WARN_ON(!sensor_peri);

	if (sensor_peri->subdev_cis) {
		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		return CALL_CISOPS(cis, cis_get_laser_photo_diode, sensor_peri->subdev_cis, value);
	}

	return 0;
}

static ssize_t camera_rear_tof_check_pd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u16 value;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	if (camera_tof_get_laser_photo_diode(SENSOR_POSITION_REAR_TOF, &value) < 0) {
		return sprintf(buf, "NG\n");
	}

	return sprintf(buf, "%d\n", value);
}

static ssize_t camera_rear_tof_check_pd_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret_count;
	u32 value;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	ret_count = sscanf(buf, "%d", &value);
	camera_tof_set_laser_current(SENSOR_POSITION_REAR_TOF, value);
	return count;
}

#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_REAR4_TOF_MODULEID
static ssize_t rear_tof_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIR4_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR4_AF\":\"%d\","
			"\"I2CR4_COM\":\"%d\",\"I2CR4_OIS\":\"%d\",\"I2CR4_SEN\":\"%d\",\"MIPIR4_COM\":\"%d\",\"MIPIR4_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2], finfo->rom_module_id[3],
			finfo->rom_module_id[4], finfo->rom_module_id[7], finfo->rom_module_id[8], finfo->rom_module_id[9],
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIR4_ID\":\"MIR4_ERR\",\"I2CR4_AF\":\"%d\","
			"\"I2CR4_COM\":\"%d\",\"I2CR4_OIS\":\"%d\",\"I2CR4_SEN\":\"%d\",\"MIPIR4_COM\":\"%d\",\"MIPIR4_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t rear_tof_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_REAR_TOF);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}
#endif
#endif

#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
static ssize_t camera_rear_tof_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int value = -1;
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis = NULL;
	int i;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("%s: core is NULL", __func__);
		return -EINVAL;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];

		fimc_is_search_sensor_module_with_position(&core->sensor[i],
				SENSOR_POSITION_REAR_TOF, &module);
		if (module)
			break;
	}

	WARN_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	WARN_ON(!sensor_peri);

	if (sensor_peri->subdev_cis) {
		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		CALL_CISOPS(cis, cis_get_tof_tx_freq, sensor_peri->subdev_cis, &value);
	}

	return sprintf(buf, "%d\n", value);
}
#endif
#endif

#ifdef CAMERA_FRONT_TOF
static ssize_t camera_front_tof_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_FRONT_TOF);
}

static ssize_t camera_front_tof_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_FRONT_TOF, false);
}

static ssize_t camera_front_tof_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_camfw_show(buf, CAM_INFO_FRONT_TOF, true);
}

static ssize_t camera_front_tof_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_checkfw_factory_show(buf, CAM_INFO_FRONT_TOF);
}

static ssize_t camera_front_tof_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_FRONT_TOF);
}

#ifdef CAMERA_FRONT2_TOF_MODULEID
static ssize_t camera_front_tof_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_FRONT_TOF);
}
#endif

#ifdef CAMERA_FRONT_TOF_CAL
static ssize_t camera_front_tofcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_FRONT_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = *((s32*)&cal_buf[finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len-1]]) +
		(finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len - 1] - finfo->rom_tof_cal_start_addr + 4);

	if (cal_size > FIMC_IS_TOF_CAL_SIZE_ONCE)
		cal_size = FIMC_IS_TOF_CAL_SIZE_ONCE;

	memcpy(buf, &cal_buf[finfo->rom_tof_cal_start_addr], cal_size);
	return cal_size;

err:
	return 0;
}

static ssize_t camera_front_tofcal_extra_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_FRONT_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = *((s32*)&cal_buf[finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len-1]]) +
		(finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len - 1] - finfo->rom_tof_cal_start_addr + 4);

	cal_size -= FIMC_IS_TOF_CAL_SIZE_ONCE;

	if (cal_size > FIMC_IS_TOF_CAL_SIZE_ONCE)
		cal_size = FIMC_IS_TOF_CAL_SIZE_ONCE;

	memcpy(buf, &cal_buf[finfo->rom_tof_cal_start_addr + FIMC_IS_TOF_CAL_SIZE_ONCE], cal_size);
	return cal_size;

err:
	return 0;
}

static ssize_t camera_front_tofcal_size_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_FRONT_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = *((s32*)&cal_buf[finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len-1]]) +
		(finfo->rom_tof_cal_size_addr[finfo->rom_tof_cal_size_addr_len - 1] - finfo->rom_tof_cal_start_addr + 4);

	return sprintf(buf, "%d\n", cal_size);

err:
	return sprintf(buf, "0");
}

static ssize_t camera_front_tofcal_uid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_core *core = NULL;
	struct fimc_is_vender_specific *specific;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	dev_info(dev, "%s: E", __func__);

	return sprintf(buf, "%d\n", specific->front_tof_uid[0]);
}

static ssize_t camera_front_tof_dual_cal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;
	s32 cal_size;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_FRONT, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	cal_size = finfo->rom_dualcal_slave0_size;

	memcpy(buf, &cal_buf[finfo->rom_dualcal_slave0_start_addr], cal_size);
	return cal_size;

err:
	return 0;
}

static ssize_t camera_front_tof_cal_result_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;
	char *cal_buf;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == CAM_INFO_CAL_MEM_TYPE_NONE) {
		err("%s: not support, no rom for camera[%d][%d]", __func__, CAM_INFO_FRONT_TOF, position);
		goto err;
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		goto err;
	}

	read_from_firmware_version(rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&cal_buf, rom_id);

	if (cal_buf[finfo->rom_tof_cal_result_addr] == FIMC_IS_TOF_CAL_CAL_RESULT_OK         /* CAT TREE */
		&& cal_buf[finfo->rom_tof_cal_result_addr + 2] == FIMC_IS_TOF_CAL_CAL_RESULT_OK  /* TRUN TABLE */
		&& cal_buf[finfo->rom_tof_cal_result_addr + 4] == FIMC_IS_TOF_CAL_CAL_RESULT_OK) /* VALIDATION */ {
		return sprintf(buf, "OK\n");
	} else {
		return sprintf(buf, "NG\n");
	}

err:
	return sprintf(buf, "NG\n");
}
#endif

#ifdef CAMERA_FRONT_TOF_TILT
static ssize_t camera_front_tof_tilt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_tilt_show(buf, CAM_INFO_FRONT_TOF);
}
#endif

static ssize_t camera_front_tof_check_pd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u16 value;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	if (camera_tof_get_laser_photo_diode(SENSOR_POSITION_FRONT_TOF, &value) < 0){
		return sprintf(buf, "NG\n");
	}

	return sprintf(buf, "%d\n", value);
}

static ssize_t camera_front_tof_check_pd_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret_count;
	u8 value;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	ret_count = sscanf(buf, "%d", &value);
	camera_tof_set_laser_current(SENSOR_POSITION_FRONT_TOF, value);
	return count;
}

#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_FRONT2_TOF_MODULEID
static ssize_t front_tof_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT_TOF);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIF2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF2_AF\":\"%d\","
			"\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\",\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2],
			finfo->rom_module_id[3], finfo->rom_module_id[4], finfo->rom_module_id[7],
			finfo->rom_module_id[8], finfo->rom_module_id[9], ec_param->i2c_af_err_cnt,
			ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
			ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIF2_ID\":\"MIR_ERR\",\"I2CF2_AF\":\"%d\","
			"\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\",\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt,
			ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t front_tof_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, SENSOR_POSITION_FRONT_TOF);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}
#endif
#endif

#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
static ssize_t camera_front_tof_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 value = 0;
	struct fimc_is_core *core;
	struct fimc_is_device_sensor *device;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_cis *cis = NULL;
	int i;

	if (!fimc_is_dev) {
		dev_err(dev, "%s: fimc_is_dev is not yet probed", __func__);
		return -ENODEV;
	}

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("%s: core is NULL", __func__);
		return -EINVAL;
	}

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		device = &core->sensor[i];
		fimc_is_search_sensor_module_with_position(&core->sensor[i],
				SENSOR_POSITION_FRONT_TOF, &module);
		if (module)
			break;
	}

	WARN_ON(!module);

	sensor_peri = (struct fimc_is_device_sensor_peri *)module->private_data;

	WARN_ON(!sensor_peri);

	if (sensor_peri->subdev_cis) {
		cis = (struct fimc_is_cis *)v4l2_get_subdevdata(sensor_peri->subdev_cis);
		CALL_CISOPS(cis, cis_get_tof_tx_freq, sensor_peri->subdev_cis, &value);
	}

	return sprintf(buf, "%d\n", value);
}
#endif
#endif

static ssize_t camera_rear_sensor_standby(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	switch (buf[0]) {
	case '0':
		break;
	case '1':
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}

	return count;
}

static ssize_t camera_rear_sensor_standby_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Rear sensor standby \n");
}

static ssize_t camera_rear_camfw_write(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;

	if ((size == 1 || size == 2) && (buf[0] == 'F' || buf[0] == 'f')) {
		fimc_is_sec_set_force_caldata_dump(true);
		ret = size;
	} else {
		fimc_is_sec_set_force_caldata_dump(false);
	}
	return ret;
}

static ssize_t camera_rear_calcheck_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	char rear_sensor[10] = {0, };
	char front_sensor[10] = {0, };
	int position;
	int rom_type;
	int rom_id;
	int rom_cal_index;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		strcpy(rear_sensor, "Null");
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		strcpy(rear_sensor, "Null");
	} else {
		read_from_firmware_version(rom_id);

		fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

		if (fimc_is_sec_check_rom_ver(sysfs_core, position)
			&& !test_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state)
			&& !test_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error))
			strcpy(rear_sensor, "Normal");
		else
			strcpy(rear_sensor, "Abnormal");
	}

	/* front check */
	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT);

	position = cam_info->internal_id;
	fimc_is_vendor_get_rom_info_from_position(position, &rom_type, &rom_id, &rom_cal_index);

	if (rom_type == ROM_TYPE_NONE) {
		strcpy(front_sensor, "Null");
	} else if (rom_id == ROM_ID_NOTHING) {
		err("%s: invalid ROM ID [%d][%d]", __func__, position, rom_id);
		strcpy(front_sensor, "Null");
	} else {
		read_from_firmware_version(rom_id);

		fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

		if (fimc_is_sec_check_rom_ver(sysfs_core, position)
			&& !test_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state)
			&& !test_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error))
			strcpy(front_sensor, "Normal");
		else
			strcpy(front_sensor, "Abnormal");
	}

	return sprintf(buf, "%s %s\n", rear_sensor, front_sensor);
}

static ssize_t camera_hw_init_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_vender *vender;
	int i;

	vender = &sysfs_core->vender;

	fimc_is_load_ctrl_lock();
	if (!check_module_init
		|| gPtr_lib_support.binary_code_load_flg != BINARY_LOAD_ALL_DONE) {
		fimc_is_vender_hw_init(vender);
		check_module_init = true;
		for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
			SsrmCameraInfo[i].cameraID = -1;
		}
	}
	fimc_is_load_ctrl_unlock();

	return sprintf(buf, "%s\n", "HW init done.");
}

#if defined (CONFIG_OIS_USE)
static ssize_t camera_ois_power_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)

{
	bool camera_running;

	fimc_is_vender_check_hw_init_running();

	camera_running = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR);

	switch (buf[0]) {
	case '0':
		if (!camera_running) {
			fimc_is_ois_gpio_off(sysfs_core);
		} else {
			err("cannot control OIS PowerOff  because camera is working.\n");
		}
		check_ois_power = false;
		break;
	case '1':
		fimc_is_ois_gpio_on(sysfs_core);
		check_ois_power = true;
		msleep(150);
		break;
	default:
		pr_debug("%s: %c\n", __func__, buf[0]);
		break;
	}
	return count;
}

static ssize_t camera_ois_autotest_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)

{
	int value = 0;

	if (kstrtoint(buf, 10, &value)) {
		err("convert fail");
	}

	ois_threshold = value;

	return count;
}

static ssize_t camera_ois_autotest_show(struct device *dev,
				    struct device_attribute *attr, char *buf)

{
	bool x_result = false, y_result = false;
	int sin_x = 0, sin_y = 0;

	if (check_ois_power) {
		fimc_is_ois_auto_test(sysfs_core, ois_threshold,
			&x_result, &y_result, &sin_x, &sin_y);

		if (x_result && y_result) {
			return sprintf(buf, "%s,%d,%s,%d\n", "pass", 0, "pass", 0);
		} else if (x_result) {
			return sprintf(buf, "%s,%d,%s,%d\n", "pass", 0, "fail", sin_y);
		} else if (y_result) {
			return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "pass", 0);
		} else {
			return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "fail", sin_y);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "fail", sin_y);
	}
}

#ifdef CAMERA_2ND_OIS
static ssize_t camera_ois_autotest_2nd_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	int value = 0;

	if (kstrtoint(buf, 10, &value)) {
		err("convert fail");
	}

	ois_threshold_rear2 = value;

	return count;
}

static ssize_t camera_ois_autotest_2nd_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	bool x_result = false, y_result = false;
	bool x_result_2nd = false, y_result_2nd = false;
	int sin_x = 0, sin_y = 0;
	int sin_x_2nd = 0, sin_y_2nd = 0;
	char wide_buffer[60] = {0,};
	char tele_buffer[30] = {0,};

	if (check_ois_power) {
		fimc_is_ois_auto_test_rear2(sysfs_core, ois_threshold_rear2,
			&x_result, &y_result, &sin_x, &sin_y,
			&x_result_2nd, &y_result_2nd, &sin_x_2nd, &sin_y_2nd);

		if (x_result && y_result) {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "pass", 0, "pass", 0);
		} else if (x_result) {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "pass", 0, "fail", sin_y);
		} else if (y_result) {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "fail", sin_x, "pass", 0);
		} else {
			sprintf(wide_buffer, "%s,%d,%s,%d,", "fail", sin_x, "fail", sin_y);
		}

		if (x_result_2nd && y_result_2nd) {
			sprintf(tele_buffer, "%s,%d,%s,%d", "pass", 0, "pass", 0);
		} else if (x_result_2nd) {
			sprintf(tele_buffer, "%s,%d,%s,%d", "pass", 0, "fail", sin_y_2nd);
		} else if (y_result_2nd) {
			sprintf(tele_buffer, "%s,%d,%s,%d", "fail", sin_x_2nd, "pass", 0);
		} else {
			sprintf(tele_buffer, "%s,%d,%s,%d", "fail", sin_x_2nd, "fail", sin_y_2nd);
		}

		strncat(wide_buffer, tele_buffer, strlen(tele_buffer));
		return sprintf(buf, "%s\n", wide_buffer);
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%s,%d,%s,%d\n", "fail", sin_x, "fail", sin_y);
	}
}

static ssize_t camera_ois_rear3_gain_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_pinfo = NULL;
	u32 xgain = 0;
	u32 ygain = 0;

	fimc_is_ois_get_phone_version(&ois_pinfo);
	xgain = (ois_pinfo->tele_xgg[3] << 24) | (ois_pinfo->tele_xgg[2] << 16) | (ois_pinfo->tele_xgg[1] << 8) | (ois_pinfo->tele_xgg[0]);
	ygain = (ois_pinfo->tele_ygg[3] << 24) | (ois_pinfo->tele_ygg[2] << 16) | (ois_pinfo->tele_ygg[1] << 8) | (ois_pinfo->tele_ygg[0]);

	if ((ois_pinfo->tele_xgg[0] == 'F' || ois_pinfo->tele_xgg[0] == 'f') &&
		(ois_pinfo->tele_ygg[0] == 'F' || ois_pinfo->tele_ygg[0] == 'f')) {
		return sprintf(buf, "%d\n", 2);
	} else if (ois_pinfo->tele_cal_mark[0] != 0xBB) {
		return sprintf(buf, "%d\n", 1);
	} else {
		return sprintf(buf, "%d,0x%08x,0x%08x\n", 0, xgain, ygain);
	}

}

static ssize_t camera_ois_rear3_supperssion_ratio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_pinfo = NULL;
	u16 xratio = 0;
	u16 yratio = 0;

	fimc_is_ois_get_phone_version(&ois_pinfo);
	xratio = (ois_pinfo->tele_supperssion_xratio[1] << 8) | (ois_pinfo->tele_supperssion_xratio[0]);
	yratio = (ois_pinfo->tele_supperssion_yratio[1] << 8) | (ois_pinfo->tele_supperssion_yratio[0]);

	if ((ois_pinfo->tele_supperssion_xratio[0] == 'F' || ois_pinfo->tele_supperssion_xratio[0] == 'f') &&
		(ois_pinfo->tele_supperssion_yratio[0] == 'F' || ois_pinfo->tele_supperssion_yratio[0] == 'f')) {
		return sprintf(buf, "%d\n", 2);
	} else if (ois_pinfo->tele_cal_mark[0] != 0xBB) {
		return sprintf(buf, "%d\n", 1);
	} else {
		return sprintf(buf, "%d,%d.%d,%d.%d\n", 0, xratio / 100, xratio % 100, yratio / 100, yratio % 100);
	}
}
#endif

static ssize_t camera_ois_selftest_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int result_total = 0;
	bool result_offset = 0, result_selftest = 0;
	int selftest_ret = 0, offsettest_ret = 0;
	long raw_data_x = 0, raw_data_y = 0;

	if (check_ois_power) {
		offsettest_ret = fimc_is_ois_offset_test(sysfs_core, &raw_data_x, &raw_data_y);
		msleep(50);
		selftest_ret = fimc_is_ois_self_test(sysfs_core);

		if (selftest_ret == 0x0) {
			result_selftest = true;
		} else {
			result_selftest = false;
		}

		if (abs(raw_data_x) > CAMERA_OIS_GYRO_OFFSET_SPEC || abs(raw_data_y) > CAMERA_OIS_GYRO_OFFSET_SPEC
			|| abs(raw_data_x - x_init_raw) > CAMERA_OIS_GYRO_OFFSET_SPEC
			|| abs(raw_data_y - y_init_raw) > CAMERA_OIS_GYRO_OFFSET_SPEC
			|| offsettest_ret == false)  {
			result_offset = false;
		} else {
			result_offset = true;
		}

		if (result_offset && result_selftest) {
			result_total = 0;
		} else if (!result_offset && !result_selftest) {
			result_total = 3;
		} else if (!result_offset) {
			result_total = 1;
		} else if (!result_selftest) {
			result_total = 2;
		}

		if (raw_data_x < 0 && raw_data_y < 0) {
			return sprintf(buf, "%d,-%ld.%03ld,-%ld.%03ld\n", result_total, abs(raw_data_x / 1000),
				abs(raw_data_x % 1000), abs(raw_data_y / 1000), abs(raw_data_y % 1000));
		} else if (raw_data_x < 0) {
			return sprintf(buf, "%d,-%ld.%03ld,%ld.%03ld\n", result_total, abs(raw_data_x / 1000),
				abs(raw_data_x % 1000), raw_data_y / 1000, raw_data_y % 1000);
		} else if (raw_data_y < 0) {
			return sprintf(buf, "%d,%ld.%03ld,-%ld.%03ld\n", result_total, raw_data_x / 1000,
				raw_data_x % 1000, abs(raw_data_y / 1000), abs(raw_data_y % 1000));
		} else {
			return sprintf(buf, "%d,%ld.%03ld,%ld.%03ld\n", result_total, raw_data_x / 1000,
				raw_data_x % 1000, raw_data_y / 1000, raw_data_y % 1000);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%d,%ld.%03ld,%ld.%03ld\n", result_total, raw_data_x / 1000,
			 raw_data_x % 1000, raw_data_y / 1000, raw_data_y % 1000);
	}
}

static ssize_t camera_ois_rawdata_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	long raw_data_x = 0, raw_data_y = 0;

	if (check_ois_power) {
		fimc_is_ois_get_offset_data(sysfs_core, &raw_data_x, &raw_data_y);

		x_init_raw = raw_data_x;
		y_init_raw = raw_data_y;

		if (raw_data_x < 0 && raw_data_y < 0) {
			return sprintf(buf, "-%ld.%03ld,-%ld.%03ld\n", abs(raw_data_x / 1000),
				abs(raw_data_x % 1000), abs(raw_data_y / 1000), abs(raw_data_y % 1000));
		} else if (raw_data_x < 0) {
			return sprintf(buf, "-%ld.%03ld,%ld.%03ld\n", abs(raw_data_x / 1000),
				abs(raw_data_x % 1000), raw_data_y / 1000, raw_data_y % 1000);
		} else if (raw_data_y < 0) {
			return sprintf(buf, "%ld.%03ld,-%ld.%03ld\n", raw_data_x / 1000, raw_data_x % 1000,
				abs(raw_data_y / 1000), abs(raw_data_y % 1000));
		} else {
			return sprintf(buf, "%ld.%03ld,%ld.%03ld\n", raw_data_x / 1000, raw_data_x % 1000,
				raw_data_y / 1000, raw_data_y % 1000);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%ld.%03ld,%ld.%03ld\n", raw_data_x / 1000, raw_data_x % 1000,
			raw_data_y / 1000, raw_data_y % 1000);
	}
}

static ssize_t camera_ois_calibrationtest_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	bool result = false;
	long raw_data_x = 0, raw_data_y = 0;

	if (check_ois_power) {
#ifdef CONFIG_CAMERA_USE_MCU
		result = fimc_is_ois_gyrocal_test(sysfs_core, &raw_data_x, &raw_data_y);
#endif
		if (raw_data_x < 0 && raw_data_y < 0) {
			return sprintf(buf, "%d,-%ld.%03ld,-%ld.%03ld\n", result, abs(raw_data_x / 1000),
				abs(raw_data_x % 1000), abs(raw_data_y / 1000), abs(raw_data_y % 1000));
		} else if (raw_data_x < 0) {
			return sprintf(buf, "%d,-%ld.%03ld,%ld.%03ld\n", result, abs(raw_data_x / 1000),
				abs(raw_data_x % 1000), raw_data_y / 1000, raw_data_y % 1000);
		} else if (raw_data_y < 0) {
			return sprintf(buf, "%d,%ld.%03ld,-%ld.%03ld\n", result, raw_data_x / 1000,
				raw_data_x % 1000, abs(raw_data_y / 1000), abs(raw_data_y % 1000));
		} else {
			return sprintf(buf, "%d,%ld.%03ld,%ld.%03ld\n", result, raw_data_x / 1000,
				raw_data_x % 1000, raw_data_y / 1000, raw_data_y % 1000);
		}
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%d,%ld.%03ld,%ld.%03ld\n", result, raw_data_x / 1000,
			 raw_data_x % 1000, raw_data_y / 1000, raw_data_y % 1000);
	}
}

static ssize_t camera_rear_aperture_halltest_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	u16 hall_value = 0;
	bool result = false;

#ifdef CONFIG_CAMERA_USE_MCU
	result = fimc_is_aperture_hall_test(sysfs_core, &hall_value);
#endif
	if (result)
		return sprintf(buf, "%d,0x%04x\n", result, hall_value);
	else
		return sprintf(buf, "%d,0x%04x\n", result, hall_value);
}

static ssize_t camera_ois_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_minfo = NULL;
	struct fimc_is_ois_info *ois_pinfo = NULL;
	bool ret = false;

	fimc_is_vender_check_hw_init_running();
	ret = read_ois_version();
	fimc_is_ois_get_module_version(&ois_minfo);
	fimc_is_ois_get_phone_version(&ois_pinfo);

	if (ois_minfo->checksum != 0x00 || !ret) {
		return sprintf(buf, "%s %s\n", "NG_FW2", "NULL");
	} else if (ois_minfo->caldata != 0x00) {
		return sprintf(buf, "%s %s\n", "NG_CD2", ois_pinfo->header_ver);
	} else {
		return sprintf(buf, "%s %s\n", ois_minfo->header_ver, ois_pinfo->header_ver);
	}
}

static ssize_t camera_ois_diff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int x_diff = 0, y_diff = 0;

#ifndef CONFIG_CAMERA_USE_MCU
	if (check_ois_power) {
		result = fimc_is_ois_diff_test(sysfs_core, &x_diff, &y_diff);

		return sprintf(buf, "%d,%d,%d\n", result == true ? 0 : 1, x_diff, y_diff);
	} else {
		err("OIS power is not enabled.");
		return sprintf(buf, "%d,%d,%d\n", 0, x_diff, y_diff);
	}
#else
	return sprintf(buf, "%d,%d,%d\n", 0, x_diff, y_diff);
#endif
}

static ssize_t camera_ois_rear_gain_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_pinfo = NULL;
	u32 xgain = 0;
	u32 ygain = 0;

	fimc_is_ois_get_phone_version(&ois_pinfo);
	xgain = (ois_pinfo->wide_xgg[3] << 24) | (ois_pinfo->wide_xgg[2] << 16) | (ois_pinfo->wide_xgg[1] << 8) | (ois_pinfo->wide_xgg[0]);
	ygain = (ois_pinfo->wide_ygg[3] << 24) | (ois_pinfo->wide_ygg[2] << 16) | (ois_pinfo->wide_ygg[1] << 8) | (ois_pinfo->wide_ygg[0]);

	if ((ois_pinfo->wide_xgg[0] == 'F' || ois_pinfo->wide_xgg[0] == 'f') &&
		(ois_pinfo->wide_ygg[0] == 'F' || ois_pinfo->wide_ygg[0] == 'f')) {
		return sprintf(buf, "%d\n", 2);
	} else if (ois_pinfo->wide_cal_mark[0]!= 0xBB) {
		return sprintf(buf, "%d\n", 1);
	} else {
		return sprintf(buf, "%d,0x%08x,0x%08x\n", 0, xgain, ygain);
	}

}

static ssize_t camera_ois_rear_supperssion_ratio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_pinfo = NULL;
	u16 xratio = 0;
	u16 yratio = 0;

	fimc_is_ois_get_phone_version(&ois_pinfo);
	xratio = (ois_pinfo->wide_supperssion_xratio[1] << 8) | (ois_pinfo->wide_supperssion_xratio[0]);
	yratio = (ois_pinfo->wide_supperssion_yratio[1] << 8) | (ois_pinfo->wide_supperssion_yratio[0]);

	if ((ois_pinfo->wide_supperssion_xratio[0] == 'F' || ois_pinfo->wide_supperssion_xratio[0] == 'f') &&
		(ois_pinfo->wide_supperssion_yratio[0] == 'F' || ois_pinfo->wide_supperssion_yratio[0] == 'f')) {
		return sprintf(buf, "%d\n", 2);
	} else if (ois_pinfo->wide_cal_mark[0] != 0xBB) {
		return sprintf(buf, "%d\n", 1);
	} else {
		return sprintf(buf, "%d,%d.%d,%d.%d\n", 0, xratio / 100, xratio % 100, yratio / 100, yratio % 100);
	}
}

static ssize_t camera_ois_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_minfo = NULL;
	struct fimc_is_ois_info *ois_pinfo = NULL;
	struct fimc_is_ois_info *ois_uinfo = NULL;
	struct fimc_is_ois_exif *ois_exif = NULL;

	fimc_is_ois_get_module_version(&ois_minfo);
	fimc_is_ois_get_phone_version(&ois_pinfo);
	fimc_is_ois_get_user_version(&ois_uinfo);
	fimc_is_ois_get_exif_data(&ois_exif);

	return sprintf(buf, "%s %s %s %d %d", ois_minfo->header_ver, ois_pinfo->header_ver,
		ois_uinfo->header_ver, ois_exif->error_data, ois_exif->status_data);
}

static ssize_t camera_ois_reset_check(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fimc_is_ois_info *ois_pinfo = NULL;

	fimc_is_ois_get_phone_version(&ois_pinfo);

	return sprintf(buf, "%d", ois_pinfo->reset_check);
}
#endif

#ifdef FORCE_CAL_LOAD
static ssize_t camera_rear_force_cal_load_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	fimc_is_sec_set_force_caldata_dump(true);
	return sprintf(buf, "FORCE CALDATA LOAD\n");
}
#endif

static ssize_t camera_rear_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_afcal_show(buf, CAM_INFO_REAR);
}

/* PAF MID offset read */
static ssize_t camera_rear_paf_offset_mid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_paf_offset_show(buf, true, FNUMBER_1ST);
}

/* PAF PAN offset read */
static ssize_t camera_rear_paf_offset_far_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_paf_offset_show(buf, false, FNUMBER_1ST);
}

static ssize_t camera_rear_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_rear_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_REAR, FNUMBER_1ST);
}

static ssize_t camera_rear_paf_cal_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;
	u8 data[5] = {0, };
	u32 paf_err_data_result = 0;

	read_from_firmware_version(ROM_ID_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);

	memcpy(data, &cal_buf[ROM_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_CAL_ERR_CHECK_OFFSET], 4);
	paf_err_data_result = *data | ( *(data + 1) << 8) | ( *(data + 2) << 16) | (*(data + 3) << 24);

	return sprintf(buf, "%08X\n", paf_err_data_result);
}

#ifdef ROM_SUPPORT_APERTURE_F2
static ssize_t camera_rear_f2_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_REAR, FNUMBER_2ND);
}

/* F2 PAF MID offset read */
static ssize_t camera_rear_f2_paf_offset_mid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_paf_offset_show(buf, true, FNUMBER_2ND);
}

/* F2 PAF PAN offset read */
static ssize_t camera_rear_f2_paf_offset_far_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_paf_offset_show(buf, false, FNUMBER_2ND);
}

static ssize_t camera_rear_f2_paf_cal_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;
	u8 data[5] = {0, };
	u32 paf_err_data_result = 0;

	read_from_firmware_version(ROM_ID_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);

	memcpy(data, &cal_buf[ROM_F2_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_CAL_ERR_CHECK_OFFSET], 4);
	paf_err_data_result = *data | ( *(data + 1) << 8) | ( *(data + 2) << 16) | (*(data + 3) << 24);

	return sprintf(buf, "%08X\n", paf_err_data_result);
}
#endif

#ifdef ROM_SUPPORT_APERTURE_F3
static ssize_t camera_rear_f3_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_REAR, FNUMBER_3RD);
}

/* F3 PAF MID offset read */
static ssize_t camera_rear_f3_paf_offset_mid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_paf_offset_show(buf, true, FNUMBER_3RD);
}

/* F3 PAF PAN offset read */
static ssize_t camera_rear_f3_paf_offset_far_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_paf_offset_show(buf, false, FNUMBER_3RD);
}

static ssize_t camera_rear_f3_paf_cal_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;
	u8 data[5] = {0, };
	u32 paf_err_data_result = 0;

	read_from_firmware_version(ROM_ID_REAR);
	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);

	memcpy(data, &cal_buf[ROM_F3_PAF_CAL_DATA_START_ADDR + FIMC_IS_PAF_CAL_ERR_CHECK_OFFSET], 4);
	paf_err_data_result = *data | ( *(data + 1) << 8) | ( *(data + 2) << 16) | (*(data + 3) << 24);

	return sprintf(buf, "%08X\n", paf_err_data_result);
}
#endif

static ssize_t camera_rear_awb_master_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_master_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_rear_awb_module_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_module_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_rear_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_REAR);
}

static ssize_t camera_front_afcal_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_afcal_show(buf, CAM_INFO_FRONT);
}

static ssize_t camera_front_moduleid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_moduleid_show(buf, CAM_INFO_FRONT);
}

static ssize_t camera_front_mtf_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_mtf_exif_show(buf, CAM_INFO_FRONT, FNUMBER_1ST);
}

static ssize_t camera_front_paf_cal_check_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *cal_buf;
	u8 data[5] = {0, };
	u32 paf_err_data_result = 0;

	read_from_firmware_version(ROM_ID_FRONT);
	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_FRONT);

	memcpy(data, &cal_buf[ROM_PAF_CAL_DATA_START_ADDR_FRONT + ROM_PAF_CAL_ERR_CHECK_OFFSET], 4);
	paf_err_data_result = *data | ( *(data + 1) << 8) | ( *(data + 2) << 16) | (*(data + 3) << 24);

	return sprintf(buf, "%08X\n", paf_err_data_result);
}

static ssize_t camera_front_sensorid_exif_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_sensorid_exif_show(buf, CAM_INFO_FRONT);
}

static ssize_t camera_front_awb_master_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_master_show(buf, CAM_INFO_FRONT);
}

static ssize_t camera_front_awb_module_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_awb_module_show(buf, CAM_INFO_FRONT);
}

int camera_fw_show_sub(char *path, char *buf, int startPos, int readsize)
{
	struct file *fp = NULL;
	long fsize, nread;
	int ret = 0;

	fp = filp_open(path, O_RDONLY, 0660);

	fsize = fp->f_path.dentry->d_inode->i_size;

	if (IS_ERR(fp)) {
		err("Camera: Failed open phone firmware");
		ret = -EIO;
		fp = NULL;
		goto exit;
	}

	if (startPos < 0)
		fp->f_pos = fsize + startPos;
	else
		fp->f_pos = startPos;

	fsize = readsize;
	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		err("failed to read firmware file, %ld Bytes", nread);
		ret = -EIO;
	}

exit:
	if (fp) {
		filp_close(fp, current->files);
		fp = NULL;
	}

	return ret;
}

char *rta_fw_list[] = {
#ifdef RTA_FW_LL
	RTA_FW_LL,
#endif
#ifdef RTA_FW_LX
	RTA_FW_LX,
#endif
#ifdef RTA_FW_LY
	RTA_FW_LY,
#endif
#ifdef RTA_FW_LS
	RTA_FW_LS
#endif
};

char *companion_fw_list[] = {
#ifdef COMPANION_FW_LL
	COMPANION_FW_LL,
#endif
#ifdef COMPANION_FW_LX
	COMPANION_FW_LX,
#endif
#ifdef COMPANION_FW_LY
	COMPANION_FW_LY,
#endif
#ifdef COMPANION_FW_LS
	COMPANION_FW_LS
#endif
};

static ssize_t camera_rear_camfw_all_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	#define VER_MAX_SIZE 100
	int ret;
	mm_segment_t old_fs;
	char fw_ver[VER_MAX_SIZE] = {0, };
	char output[1024] = {0, };
	char path[128] = {0, };
	int cnt = 0, loop = 0, i;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sprintf(output, "[RCAM]");
	cnt = 0;

	loop = sizeof(rta_fw_list) / sizeof(char *);
	for (i = 0; i < loop; i++) {
		sprintf(path, FIMC_IS_FW_PATH"%s", rta_fw_list[i]);
		ret = camera_fw_show_sub(path, fw_ver,
						(-FIMC_IS_HEADER_VER_OFFSET), FIMC_IS_HEADER_VER_SIZE);
		if (ret == 0) {
			if (cnt++ > 0)
				strcat(output, ",");
			strcat(output, fw_ver);
			memset(fw_ver, 0, sizeof(fw_ver));
		}
	}

	strcat(output, ";[COMPANION]");
	cnt = 0;

	loop = sizeof(companion_fw_list) / sizeof(char *);
	for (i = 0; i < loop; i++) {
		sprintf(path, FIMC_IS_FW_PATH"%s", companion_fw_list[i]);
		ret = camera_fw_show_sub(path, fw_ver,
						(-16), FIMC_IS_HEADER_VER_SIZE);
		if (ret == 0) {
			if (cnt++ > 0)
				strcat(output, ",");
			strcat(output, fw_ver);
			memset(fw_ver, 0, sizeof(fw_ver));
		}
	}

	set_fs(old_fs);

#ifdef CONFIG_OIS_USE
	strcat(output, ";[OIS]");
	ret = fimc_is_ois_read_fw_ver(sysfs_core, FIMC_IS_FW_PATH FIMC_OIS_FW_NAME_SEC, fw_ver);
	if (ret == 0) {
		strcat(output, fw_ver);
		memset(fw_ver, 0, sizeof(fw_ver));
	}

	ret = fimc_is_ois_read_fw_ver(sysfs_core, FIMC_IS_FW_PATH FIMC_OIS_FW_NAME_DOM, fw_ver);
	if (ret == 0) {
		strcat(output, ",");
		strcat(output, fw_ver);
		memset(fw_ver, 0, sizeof(fw_ver));
	}
#endif

	return sprintf(buf, "%s\n", output);
}

#ifdef SECURE_CAMERA_IRIS
static ssize_t camera_iris_camfw_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_name[50];
	int ret;

	info("%s: is_iris_ver_read[%d]\n", __func__, is_iris_ver_read);

	if (is_iris_ver_read) {
		ret = fimc_is_get_sensor_data(NULL, sensor_name, SENSOR_POSITION_SECURE);

		if (ret < 0) {
			return sprintf(buf, "UNKNOWN N\n");
		} else {
			return sprintf(buf, "%s N\n", sensor_name);
		}
	} else {
		return sprintf(buf, "UNKNOWN N\n");
	}
}

static ssize_t camera_iris_camfw_full_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char sensor_name[50];
	int ret;

	info("%s: is_iris_ver_read[%d]\n", __func__, is_iris_ver_read);

	if (is_iris_ver_read) {
		ret = fimc_is_get_sensor_data(NULL, sensor_name, SENSOR_POSITION_SECURE);

		if (ret < 0) {
			return sprintf(buf, "UNKNOWN N N\n");
		} else {
			return sprintf(buf, "%s N N\n", sensor_name);
		}
	} else {
		return sprintf(buf, "UNKNOWN N N\n");
	}
}

static ssize_t camera_iris_checkfw_factory_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u8 year_month_company[3] = {'0', '0', '0'};

	info("%s: is_iris_ver_read[%d]\n", __func__, is_iris_ver_read);

	if (is_iris_mtf_read_data[0] != 0x00 && is_iris_mtf_read_data[0] >= 'A' && is_iris_mtf_read_data[0] <= 'Z')
		year_month_company[0] = is_iris_mtf_read_data[0];

	if (is_iris_mtf_read_data[1] != 0x00 && is_iris_mtf_read_data[1] >= 'A' && is_iris_mtf_read_data[1] <= 'Z')
		year_month_company[1] = is_iris_mtf_read_data[1];

	if (is_iris_mtf_read_data[2] != 0x00 && is_iris_mtf_read_data[2] >= 'A' && is_iris_mtf_read_data[2] <= 'Z')
		year_month_company[2] = is_iris_mtf_read_data[2];

	if (is_iris_mtf_test_check == false) {
		return sprintf(buf, "NG_RES %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
	}
	if (is_iris_ver_read) {
		if (is_final_cam_module_iris) {
			return sprintf(buf, "OK %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
		} else {
			return sprintf(buf, "NG_VER %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
		}
	} else {
		return sprintf(buf, "NG_VER %c %c %c\n", year_month_company[0], year_month_company[1], year_month_company[2]);
	}
}
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
static ssize_t rear_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR);
	position = cam_info->internal_id;

	fimc_is_sec_get_sysfs_finfo(&finfo, fimc_is_vendor_get_rom_id_from_position(position));
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIR_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR_AF\":\"%d\","
			"\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\",\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2], finfo->rom_module_id[3],
			finfo->rom_module_id[4], finfo->rom_module_id[7], finfo->rom_module_id[8], finfo->rom_module_id[9],
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIR_ID\":\"MIR_ERR\",\"I2CR_AF\":\"%d\","
			"\"I2CR_COM\":\"%d\",\"I2CR_OIS\":\"%d\",\"I2CR_SEN\":\"%d\",\"MIPIR_COM\":\"%d\",\"MIPIR_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t rear_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR);
	position = cam_info->internal_id;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, position);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}

static ssize_t front_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT);
	position = cam_info->internal_id;

	fimc_is_sec_get_sysfs_finfo(&finfo, fimc_is_vendor_get_rom_id_from_position(position));
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIF_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF_AF\":\"%d\","
			"\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\",\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2],
			finfo->rom_module_id[3], finfo->rom_module_id[4], finfo->rom_module_id[7],
			finfo->rom_module_id[8], finfo->rom_module_id[9], ec_param->i2c_af_err_cnt,
			ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
			ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIF_ID\":\"MIR_ERR\",\"I2CF_AF\":\"%d\","
			"\"I2CF_COM\":\"%d\",\"I2CF_OIS\":\"%d\",\"I2CF_SEN\":\"%d\",\"MIPIF_COM\":\"%d\",\"MIPIF_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt,
			ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t front_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT);
	position = cam_info->internal_id;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, position);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}

#ifdef CAMERA_FRONT2
static ssize_t front2_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT2);
	position = cam_info->internal_id;

	fimc_is_sec_get_sysfs_finfo(&finfo, fimc_is_vendor_get_rom_id_from_position(position));
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIF2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CF2_AF\":\"%d\","
			"\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\",\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2],
			finfo->rom_module_id[3], finfo->rom_module_id[4], finfo->rom_module_id[7],
			finfo->rom_module_id[8], finfo->rom_module_id[9], ec_param->i2c_af_err_cnt,
			ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt,
			ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIF2_ID\":\"MIR_ERR\",\"I2CF2_AF\":\"%d\","
			"\"I2CF2_COM\":\"%d\",\"I2CF2_OIS\":\"%d\",\"I2CF2_SEN\":\"%d\",\"MIPIF2_COM\":\"%d\",\"MIPIF2_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt, ec_param->i2c_sensor_err_cnt,
			ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t front2_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_FRONT2);
	position = cam_info->internal_id;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, position);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}
#endif

#ifdef CAMERA_REAR2
static ssize_t rear2_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR2);
	position = cam_info->internal_id;

	fimc_is_sec_get_sysfs_finfo(&finfo, fimc_is_vendor_get_rom_id_from_position(position));
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIR2_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR2_AF\":\"%d\","
			"\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\",\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2], finfo->rom_module_id[3],
			finfo->rom_module_id[4], finfo->rom_module_id[7], finfo->rom_module_id[8], finfo->rom_module_id[9],
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIR2_ID\":\"MIR_ERR\",\"I2CR2_AF\":\"%d\","
			"\"I2CR2_COM\":\"%d\",\"I2CR2_OIS\":\"%d\",\"I2CR2_SEN\":\"%d\",\"MIPIR2_COM\":\"%d\",\"MIPIR2_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t rear2_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR2);
	position = cam_info->internal_id;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, position);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}
#endif

#ifdef CAMERA_REAR3
static ssize_t rear3_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_rom_info *finfo;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR3);
	position = cam_info->internal_id;

	fimc_is_sec_get_sysfs_finfo(&finfo, fimc_is_vendor_get_rom_id_from_position(position));
	fimc_is_sec_get_hw_param(&ec_param, position);

	if (fimc_is_sec_is_valid_moduleid(finfo->rom_module_id)) {
		return sprintf(buf, "\"CAMIR3_ID\":\"%c%c%c%c%cXX%02X%02X%02X\",\"I2CR3_AF\":\"%d\","
			"\"I2CR3_COM\":\"%d\",\"I2CR3_OIS\":\"%d\",\"I2CR3_SEN\":\"%d\",\"MIPIR3_COM\":\"%d\",\"MIPIR3_SEN\":\"%d\"\n",
			finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2], finfo->rom_module_id[3],
			finfo->rom_module_id[4], finfo->rom_module_id[7], finfo->rom_module_id[8], finfo->rom_module_id[9],
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	} else {
		return sprintf(buf, "\"CAMIR3_ID\":\"MIR_ERR\",\"I2CR3_AF\":\"%d\","
			"\"I2CR3_COM\":\"%d\",\"I2CR3_OIS\":\"%d\",\"I2CR3_SEN\":\"%d\",\"MIPIR3_COM\":\"%d\",\"MIPIR3_SEN\":\"%d\"\n",
			ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
			ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
	}
}

static ssize_t rear3_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_REAR3);
	position = cam_info->internal_id;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, position);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}
#endif

#ifdef SECURE_CAMERA_IRIS
static ssize_t iris_camera_hw_param_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_IRIS);
	position = cam_info->internal_id;

	fimc_is_sec_get_hw_param(&ec_param, position);

	return sprintf(buf, "\"CAMII_ID\":\"MI_NO\",\"I2CI_AF\":\"%d\",\"I2CI_COM\":\"%d\",\"I2CI_OIS\":\"%d\","
		"\"I2CI_SEN\":\"%d\",\"MIPII_COM\":\"%d\",\"MIPII_SEN\":\"%d\"\n",
		ec_param->i2c_af_err_cnt, ec_param->i2c_comp_err_cnt, ec_param->i2c_ois_err_cnt,
		ec_param->i2c_sensor_err_cnt, ec_param->mipi_comp_err_cnt, ec_param->mipi_sensor_err_cnt);
}

static ssize_t iris_camera_hw_param_store(struct device *dev,
				    struct device_attribute *attr, const char *buf, size_t count)
{
	struct cam_hw_param *ec_param = NULL;
	struct fimc_is_cam_info *cam_info;
	int position;

	fimc_is_get_cam_info_from_index(&cam_info, CAM_INFO_IRIS);
	position = cam_info->internal_id;

	if (!strncmp(buf, "c", 1)) {
		fimc_is_sec_get_hw_param(&ec_param, position);

		if (ec_param)
			fimc_is_sec_init_err_cnt(ec_param);
	}

	return count;
}
#endif
#endif

#ifdef SECURE_CAMERA_IRIS
static ssize_t camera_iris_checkfw_user_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	info("%s: is_iris_ver_read[%d]\n", __func__, is_iris_ver_read);

	if (is_iris_ver_read) {
		if (is_final_cam_module_iris)
			return sprintf(buf, "%s\n", "OK");
		else
			return sprintf(buf, "%s\n", "NG");
	} else {
		return sprintf(buf, "%s\n", "NG");
	}
}

static ssize_t camera_iris_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return camera_info_show(buf, CAM_INFO_IRIS);
}
#endif

static DEVICE_ATTR(rear_sensorid, S_IRUGO, camera_rear_sensorid_show, NULL);
static DEVICE_ATTR(rear2_sensorid, S_IRUGO, camera_rear2_sensorid_show, NULL);
static DEVICE_ATTR(rear3_sensorid, S_IRUGO, camera_rear3_sensorid_show, NULL);
static DEVICE_ATTR(rear_tof_sensorid, S_IRUGO, camera_rear_tof_sensorid_show, NULL);
static DEVICE_ATTR(front_sensorid, S_IRUGO, camera_front_sensorid_show, NULL);
static DEVICE_ATTR(front2_sensorid, S_IRUGO, camera_front2_sensorid_show, NULL);
static DEVICE_ATTR(front_tof_sensorid, S_IRUGO, camera_front_tof_sensorid_show, NULL);
#ifdef SECURE_CAMERA_IRIS
static DEVICE_ATTR(secure_sensorid, S_IRUGO, camera_secure_sensorid_show, NULL);
#endif

static DEVICE_ATTR(supported_cameraIds, S_IRUGO, camera_supported_cameraIds_show, NULL);

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
static DEVICE_ATTR(from_write, S_IRUGO, camera_rear_writefw_show, NULL);
#endif

static DEVICE_ATTR(rear_camtype, S_IRUGO, camera_rear_camtype_show, NULL);
static DEVICE_ATTR(rear_calcheck, S_IRUGO, camera_rear_calcheck_show, NULL);

static DEVICE_ATTR(rear_caminfo, S_IRUGO, camera_rear_info_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO, camera_rear_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(rear_camfw_full, S_IRUGO, camera_rear_camfw_full_show, NULL);
static DEVICE_ATTR(rear_checkfw_user, S_IRUGO, camera_rear_checkfw_user_show, NULL);
static DEVICE_ATTR(rear_checkfw_factory, S_IRUGO, camera_rear_checkfw_factory_show, NULL);
static DEVICE_ATTR(rear_sensorid_exif, S_IRUGO, camera_rear_sensorid_exif_show, NULL);
static DEVICE_ATTR(rear_mtf_exif, S_IRUGO, camera_rear_mtf_exif_show, NULL);
static DEVICE_ATTR(rear_afcal, S_IRUGO, camera_rear_afcal_show, NULL);
static DEVICE_ATTR(rear_paf_cal_check, S_IRUGO, camera_rear_paf_cal_check_show, NULL);
static DEVICE_ATTR(rear_awb_master, S_IRUGO, camera_rear_awb_master_show, NULL);
static DEVICE_ATTR(rear_awb_module, S_IRUGO, camera_rear_awb_module_show, NULL);
static DEVICE_ATTR(rear_moduleid, S_IRUGO, camera_rear_moduleid_show, NULL);
static DEVICE_ATTR(SVC_rear_module, S_IRUGO, camera_rear_moduleid_show, NULL);

#ifdef CAMERA_REAR_DUAL_CAL
static DEVICE_ATTR(rear_dualcal, S_IRUGO, camera_rear_dualcal_show, NULL);
static DEVICE_ATTR(rear2_dualcal, S_IRUGO, camera_rear2_dualcal_show, NULL);
static DEVICE_ATTR(rear3_dualcal, S_IRUGO, camera_rear3_dualcal_show, NULL);
#endif

#ifdef CAMERA_FRONT_DUAL_CAL
static DEVICE_ATTR(front_dualcal, S_IRUGO, camera_front_dualcal_show, NULL);
static DEVICE_ATTR(front2_dualcal, S_IRUGO, camera_front2_dualcal_show, NULL);
#endif

#ifdef CAMERA_REAR2
static DEVICE_ATTR(rear2_caminfo, S_IRUGO, camera_rear2_info_show, NULL);
static DEVICE_ATTR(rear2_camfw, S_IRUGO, camera_rear2_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(rear2_camfw_full, S_IRUGO, camera_rear2_camfw_full_show, NULL);
static DEVICE_ATTR(rear2_checkfw_user, S_IRUGO, camera_rear2_checkfw_user_show, NULL);
static DEVICE_ATTR(rear2_checkfw_factory, S_IRUGO, camera_rear2_checkfw_factory_show, NULL);
static DEVICE_ATTR(rear2_sensorid_exif, S_IRUGO, camera_rear2_sensorid_exif_show, NULL);
static DEVICE_ATTR(rear2_mtf_exif, S_IRUGO, camera_rear2_mtf_exif_show, NULL);
#ifdef CAMERA_REAR2_AFCAL
static DEVICE_ATTR(rear2_afcal, S_IRUGO, camera_rear2_afcal_show, NULL);
#endif
#ifdef CAMERA_REAR2_TILT
static DEVICE_ATTR(rear2_tilt, S_IRUGO, camera_rear2_tilt_show, NULL);
#endif
static DEVICE_ATTR(rear2_awb_master, S_IRUGO, camera_rear2_awb_master_show, NULL);
static DEVICE_ATTR(rear2_awb_module, S_IRUGO, camera_rear2_awb_module_show, NULL);
#ifdef CAMERA_REAR2_MODULEID
static DEVICE_ATTR(rear2_moduleid, S_IRUGO, camera_rear2_moduleid_show, NULL);
static DEVICE_ATTR(SVC_rear_module2, S_IRUGO, camera_rear2_moduleid_show, NULL);
#endif
#endif
#ifdef CAMERA_REAR3
static DEVICE_ATTR(rear3_caminfo, S_IRUGO, camera_rear3_info_show, NULL);
static DEVICE_ATTR(rear3_camfw, S_IRUGO, camera_rear3_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(rear3_camfw_full, S_IRUGO, camera_rear3_camfw_full_show, NULL);
static DEVICE_ATTR(rear3_checkfw_user, S_IRUGO, camera_rear3_checkfw_user_show, NULL);
static DEVICE_ATTR(rear3_checkfw_factory, S_IRUGO, camera_rear3_checkfw_factory_show, NULL);
static DEVICE_ATTR(rear3_sensorid_exif, S_IRUGO, camera_rear3_sensorid_exif_show, NULL);
static DEVICE_ATTR(rear3_mtf_exif, S_IRUGO, camera_rear3_mtf_exif_show, NULL);
static DEVICE_ATTR(rear3_paf_cal_check, S_IRUGO, camera_rear3_paf_cal_check_show, NULL);
#ifdef CAMERA_REAR3_AFCAL
static DEVICE_ATTR(rear3_afcal, S_IRUGO, camera_rear3_afcal_show, NULL);
#endif
#ifdef CAMERA_REAR3_TILT
static DEVICE_ATTR(rear3_tilt, S_IRUGO, camera_rear3_tilt_show, NULL);
#endif
static DEVICE_ATTR(rear3_awb_master, S_IRUGO, camera_rear3_awb_master_show, NULL);
static DEVICE_ATTR(rear3_awb_module, S_IRUGO, camera_rear3_awb_module_show, NULL);
#ifdef CAMERA_REAR3_MODULEID
static DEVICE_ATTR(rear3_moduleid, S_IRUGO, camera_rear3_moduleid_show, NULL);
static DEVICE_ATTR(SVC_rear_module3, S_IRUGO, camera_rear3_moduleid_show, NULL);
#endif
#endif

#ifdef CAMERA_REAR_TOF
static DEVICE_ATTR(rear_tof_caminfo, S_IRUGO, camera_rear_tof_info_show, NULL);
static DEVICE_ATTR(rear_tof_camfw, S_IRUGO, camera_rear_tof_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(rear_tof_camfw_full, S_IRUGO, camera_rear_tof_camfw_full_show, NULL);
static DEVICE_ATTR(rear_tof_checkfw_factory, S_IRUGO, camera_rear_tof_checkfw_factory_show, NULL);
static DEVICE_ATTR(rear_tof_sensorid_exif, S_IRUGO, camera_rear_tof_sensorid_exif_show, NULL);
#ifdef CAMERA_REAR4_TOF_MODULEID
static DEVICE_ATTR(rear4_moduleid, S_IRUGO, camera_rear_tof_moduleid_show, NULL);
static DEVICE_ATTR(SVC_rear_module4, S_IRUGO, camera_rear_tof_moduleid_show, NULL);
#endif
#ifdef CAMERA_REAR_TOF_CAL
static DEVICE_ATTR(rear_tofcal, S_IRUGO, camera_rear_tofcal_show, NULL);
static DEVICE_ATTR(rear_tofcal_extra, S_IRUGO, camera_rear_tofcal_extra_show, NULL);
static DEVICE_ATTR(rear_tofcal_size, S_IRUGO, camera_rear_tofcal_size_show, NULL);
static DEVICE_ATTR(rear_tofcal_uid, S_IRUGO, camera_rear_tofcal_uid_show, NULL);
static DEVICE_ATTR(rear_tof_dual_cal, S_IRUGO, camera_rear_tof_dual_cal_show, NULL);
static DEVICE_ATTR(rear_tof_cal_result, S_IRUGO, camera_rear_tof_cal_result_show, NULL);
static DEVICE_ATTR(rear_tof_get_validation, S_IRUGO, camera_rear_tof_get_validation_data_show, NULL);
#endif
#ifdef CAMERA_REAR_TOF_TILT
static DEVICE_ATTR(rear_tof_tilt, S_IRUGO, camera_rear_tof_tilt_show, NULL);
#endif
#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
static DEVICE_ATTR(rear_tof_freq, S_IRUGO, camera_rear_tof_freq_show, NULL);
#endif
#ifdef CAMERA_REAR2_TOF_TILT
static DEVICE_ATTR(rear2_tof_tilt, S_IRUGO, camera_rear2_tof_tilt_show, NULL);
#endif
static DEVICE_ATTR(rear_tof_check_pd, 0664, camera_rear_tof_check_pd_show, camera_rear_tof_check_pd_store);

#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_REAR4_TOF_MODULEID
static DEVICE_ATTR(rear4_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				rear_tof_camera_hw_param_show, rear_tof_camera_hw_param_store);
#endif
#endif
#endif

static DEVICE_ATTR(front_caminfo, S_IRUGO, camera_front_info_show, NULL);
static DEVICE_ATTR(front_camtype, S_IRUGO, camera_front_camtype_show, NULL);
static DEVICE_ATTR(front_camfw, S_IRUGO, camera_front_camfw_show, NULL);
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
static DEVICE_ATTR(front_mipi_clock, 0440, camera_front_mipi_clock_show, NULL);
#endif
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
static DEVICE_ATTR(front_camfw_full, S_IRUGO, camera_front_camfw_full_show, NULL);
static DEVICE_ATTR(front_checkfw_factory, S_IRUGO, camera_front_checkfw_factory_show, NULL);
static DEVICE_ATTR(front_afcal, S_IRUGO, camera_front_afcal_show, NULL);
static DEVICE_ATTR(front_mtf_exif, S_IRUGO, camera_front_mtf_exif_show, NULL);
static DEVICE_ATTR(front_paf_cal_check, S_IRUGO, camera_front_paf_cal_check_show, NULL);
static DEVICE_ATTR(front_sensorid_exif, S_IRUGO, camera_front_sensorid_exif_show, NULL);
static DEVICE_ATTR(front_awb_master, S_IRUGO, camera_front_awb_master_show, NULL);
static DEVICE_ATTR(front_awb_module, S_IRUGO, camera_front_awb_module_show, NULL);
static DEVICE_ATTR(front_moduleid, S_IRUGO, camera_front_moduleid_show, NULL);
static DEVICE_ATTR(SVC_front_module, S_IRUGO, camera_front_moduleid_show, NULL);
#endif
#ifdef CAMERA_FRONT2
static DEVICE_ATTR(front2_caminfo, S_IRUGO, camera_front2_info_show, NULL);
static DEVICE_ATTR(front2_camfw, S_IRUGO, camera_front2_camfw_show, NULL);
static DEVICE_ATTR(front2_camfw_full, S_IRUGO,	camera_front2_camfw_full_show, NULL);
static DEVICE_ATTR(front2_checkfw_factory, S_IRUGO, camera_front2_checkfw_factory_show, NULL);
static DEVICE_ATTR(front2_sensorid_exif, S_IRUGO, camera_front2_sensorid_exif_show, NULL);
static DEVICE_ATTR(front2_mtf_exif, S_IRUGO, camera_front2_mtf_exif_show, NULL);
#ifdef CAMERA_FRONT2_TILT
static DEVICE_ATTR(front2_tilt, S_IRUGO, camera_front2_tilt_show, NULL);
#endif
#ifndef CAMERA_FRONT2_USE_DEFAULT_AWB_CAL
static DEVICE_ATTR(front2_awb_master, S_IRUGO, camera_front2_awb_master_show, NULL);
static DEVICE_ATTR(front2_awb_module, S_IRUGO, camera_front2_awb_module_show, NULL);
#endif
#ifdef CAMERA_FRONT2_MODULEID
static DEVICE_ATTR(front2_moduleid, S_IRUGO, camera_front2_moduleid_show, NULL);
static DEVICE_ATTR(SVC_front_module2, S_IRUGO, camera_front2_moduleid_show, NULL);
#endif
#endif

#ifdef CAMERA_FRONT_TOF
static DEVICE_ATTR(front_tof_caminfo, S_IRUGO, camera_front_tof_info_show, NULL);
static DEVICE_ATTR(front_tof_camfw, S_IRUGO, camera_front_tof_camfw_show, camera_rear_camfw_write);
static DEVICE_ATTR(front_tof_camfw_full, S_IRUGO, camera_front_tof_camfw_full_show, NULL);
static DEVICE_ATTR(front_tof_checkfw_factory, S_IRUGO, camera_front_tof_checkfw_factory_show, NULL);
static DEVICE_ATTR(front_tof_sensorid_exif, S_IRUGO, camera_front_tof_sensorid_exif_show, NULL);
#ifdef CAMERA_FRONT2_TOF_MODULEID
static DEVICE_ATTR(front2_moduleid, S_IRUGO, camera_front_tof_moduleid_show, NULL);
static DEVICE_ATTR(SVC_front_module2, S_IRUGO, camera_front_tof_moduleid_show, NULL);
#endif
#ifdef CAMERA_FRONT_TOF_CAL
static DEVICE_ATTR(front_tofcal, S_IRUGO, camera_front_tofcal_show, NULL);
static DEVICE_ATTR(front_tofcal_extra, S_IRUGO, camera_front_tofcal_extra_show, NULL);
static DEVICE_ATTR(front_tofcal_size, S_IRUGO, camera_front_tofcal_size_show, NULL);
static DEVICE_ATTR(front_tofcal_uid, S_IRUGO, camera_front_tofcal_uid_show, NULL);
static DEVICE_ATTR(front_tof_dual_cal, S_IRUGO, camera_front_tof_dual_cal_show, NULL);
static DEVICE_ATTR(front_tof_cal_result, S_IRUGO, camera_front_tof_cal_result_show, NULL);
#endif
#ifdef CAMERA_FRONT_TOF_TILT
static DEVICE_ATTR(front_tof_tilt, S_IRUGO, camera_front_tof_tilt_show, NULL);
#endif
static DEVICE_ATTR(front_tof_check_pd, 0644, camera_front_tof_check_pd_show, camera_front_tof_check_pd_store);
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
static DEVICE_ATTR(front_tof_freq, S_IRUGO, camera_front_tof_freq_show, NULL);
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_FRONT2_TOF_MODULEID
static DEVICE_ATTR(front2_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				front_tof_camera_hw_param_show, front_tof_camera_hw_param_store);
#endif
#endif
#endif

static DEVICE_ATTR(rear_sensor_standby, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_rear_sensor_standby_show, camera_rear_sensor_standby);

static DEVICE_ATTR(fw_update, S_IRUGO, camera_hw_init_show, NULL);
#if defined (CONFIG_OIS_USE)
static DEVICE_ATTR(selftest, S_IRUGO, camera_ois_selftest_show, NULL);
static DEVICE_ATTR(ois_power, S_IWUSR, NULL, camera_ois_power_store);
static DEVICE_ATTR(autotest, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_ois_autotest_show, camera_ois_autotest_store);
#ifdef CAMERA_2ND_OIS
static DEVICE_ATTR(autotest_2nd, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
		camera_ois_autotest_2nd_show, camera_ois_autotest_2nd_store);
static DEVICE_ATTR(ois_gain_rear3, S_IRUGO, camera_ois_rear3_gain_show, NULL);
static DEVICE_ATTR(ois_supperssion_ratio_rear3, S_IRUGO, camera_ois_rear3_supperssion_ratio_show, NULL);
#endif
static DEVICE_ATTR(ois_rawdata, S_IRUGO, camera_ois_rawdata_show, NULL);
static DEVICE_ATTR(calibrationtest, S_IRUGO, camera_ois_calibrationtest_show, NULL);
static DEVICE_ATTR(rear_aperture_halltest, S_IRUGO, camera_rear_aperture_halltest_show, NULL);
static DEVICE_ATTR(oisfw, S_IRUGO, camera_ois_version_show, NULL);
static DEVICE_ATTR(ois_diff, S_IRUGO, camera_ois_diff_show, NULL);
static DEVICE_ATTR(ois_exif, S_IRUGO, camera_ois_exif_show, NULL);
static DEVICE_ATTR(ois_gain_rear, S_IRUGO, camera_ois_rear_gain_show, NULL);
static DEVICE_ATTR(ois_supperssion_ratio_rear, S_IRUGO, camera_ois_rear_supperssion_ratio_show, NULL);
static DEVICE_ATTR(reset_check, S_IRUGO, camera_ois_reset_check, NULL);
#endif
#ifdef FORCE_CAL_LOAD
static DEVICE_ATTR(rear_force_cal_load, S_IRUGO, camera_rear_force_cal_load_show, NULL);
#endif
static DEVICE_ATTR(rear_paf_offset_mid, S_IRUGO, camera_rear_paf_offset_mid_show, NULL);
static DEVICE_ATTR(rear_paf_offset_far, S_IRUGO, camera_rear_paf_offset_far_show, NULL);
#ifdef ROM_SUPPORT_APERTURE_F2
static DEVICE_ATTR(rear_mtf2_exif, S_IRUGO, camera_rear_f2_mtf_exif_show, NULL);
static DEVICE_ATTR(rear_f2_paf_offset_mid, S_IRUGO, camera_rear_f2_paf_offset_mid_show, NULL);
static DEVICE_ATTR(rear_f2_paf_offset_far, S_IRUGO, camera_rear_f2_paf_offset_far_show, NULL);
static DEVICE_ATTR(rear_f2_paf_cal_check, S_IRUGO, camera_rear_f2_paf_cal_check_show, NULL);
#endif
#ifdef ROM_SUPPORT_APERTURE_F3
static DEVICE_ATTR(rear_mtf3_exif, S_IRUGO, camera_rear_f3_mtf_exif_show, NULL);
static DEVICE_ATTR(rear_f3_paf_offset_mid, S_IRUGO, camera_rear_f3_paf_offset_mid_show, NULL);
static DEVICE_ATTR(rear_f3_paf_offset_far, S_IRUGO, camera_rear_f3_paf_offset_far_show, NULL);
static DEVICE_ATTR(rear_f3_paf_cal_check, S_IRUGO, camera_rear_f3_paf_cal_check_show, NULL);
#endif
static DEVICE_ATTR(rear_camfw_all, S_IRUGO, camera_rear_camfw_all_show, NULL);
static DEVICE_ATTR(ssrm_camera_info, 0644, camera_ssrm_camera_info_show, camera_ssrm_camera_info_store);

#ifdef SECURE_CAMERA_IRIS
static DEVICE_ATTR(iris_camfw_full, S_IRUGO, camera_iris_camfw_full_show, NULL);
static DEVICE_ATTR(iris_checkfw_factory, S_IRUGO, camera_iris_checkfw_factory_show, NULL);
static DEVICE_ATTR(iris_checkfw_user, S_IRUGO, camera_iris_checkfw_user_show, NULL);
static DEVICE_ATTR(iris_caminfo, S_IRUGO, camera_iris_info_show, NULL);
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
static DEVICE_ATTR(rear_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				rear_camera_hw_param_show, rear_camera_hw_param_store);
static DEVICE_ATTR(front_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				front_camera_hw_param_show, front_camera_hw_param_store);
#ifdef CAMERA_FRONT2
static DEVICE_ATTR(front2_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				front2_camera_hw_param_show, front2_camera_hw_param_store);
#endif

#ifdef CAMERA_REAR2
static DEVICE_ATTR(rear2_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				rear2_camera_hw_param_show, rear2_camera_hw_param_store);
#endif
#ifdef CAMERA_REAR3
static DEVICE_ATTR(rear3_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				rear3_camera_hw_param_show, rear3_camera_hw_param_store);
#endif

#ifdef SECURE_CAMERA_IRIS
static DEVICE_ATTR(iris_hwparam, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH,
				iris_camera_hw_param_show, iris_camera_hw_param_store);
#endif
#endif

int svc_cheating_prevent_device_file_create(struct kobject **obj)
{
	struct kernfs_node *svc_sd;
	struct kobject *data;
	struct kobject *Camera;

	/* To find svc kobject */
	svc_sd = sysfs_get_dirent(devices_kset->kobj.sd, "svc");
	if (IS_ERR_OR_NULL(svc_sd)) {
		/* try to create svc kobject */
		data = kobject_create_and_add("svc", &devices_kset->kobj);
		if (IS_ERR_OR_NULL(data))
			pr_info("Failed to create sys/devices/svc already exist svc : 0x%pK\n", data);
		else
			pr_info("Success to create sys/devices/svc svc : 0x%pK\n", data);
	} else {
		data = (struct kobject *)svc_sd->priv;
		pr_info("Success to find svc_sd : 0x%pK svc : 0x%pK\n", svc_sd, data);
	}

	Camera = kobject_create_and_add("Camera", data);
	if (IS_ERR_OR_NULL(Camera))
		pr_info("Failed to create sys/devices/svc/Camera : 0x%pK\n", Camera);
	else
		pr_info("Success to create sys/devices/svc/Camera : 0x%pK\n", Camera);

	*obj = Camera;

	return 0;
}

int fimc_is_create_sysfs(struct fimc_is_core *core)
{
	struct kobject *svc = 0;

	if (!core) {
		err("fimc_is_core is null");
		return -EINVAL;
	}

	svc_cheating_prevent_device_file_create(&svc);

	if (camera_class == NULL) {
		camera_class = class_create(THIS_MODULE, "camera");
		if (IS_ERR(camera_class)) {
			pr_err("Failed to create class(camera)!\n");
			return PTR_ERR(camera_class);
		}
	}

	camera_front_dev = device_create(camera_class, NULL, 0, NULL, "front");
	if (IS_ERR(camera_front_dev)) {
		pr_err("failed to create front device!\n");
	} else {
		if (device_create_file(camera_front_dev, &dev_attr_front_sensorid) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_sensorid.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_sensorid) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_sensorid.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_sensorid) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_tof_sensorid.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_camtype) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_camtype.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_camfw) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_camfw.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_caminfo) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_caminfo.attr.name);
		}
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
		if (device_create_file(camera_front_dev, &dev_attr_front_camfw_full) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_camfw_full.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_checkfw_factory) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_sensorid_exif) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_mtf_exif) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_mtf_exif.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_paf_cal_check) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_paf_cal_check.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_afcal) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_afcal.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_awb_master) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_awb_master.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_awb_module) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_awb_module.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_moduleid) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_front_module.attr) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_SVC_front_module.attr.name);
		}
#endif
#ifdef CAMERA_FRONT_DUAL_CAL
		if (device_create_file(camera_front_dev, &dev_attr_front_dualcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_dualcal.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_dualcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_dualcal.attr.name);
		}
#endif
#ifdef CAMERA_FRONT2
		if (device_create_file(camera_front_dev, &dev_attr_front2_caminfo) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front2_caminfo.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_camfw) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front2_camfw.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_camfw_full) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front2_camfw_full.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_checkfw_factory) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front2_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_sensorid_exif) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front2_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_mtf_exif) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_mtf_exif.attr.name);
		}
#ifndef CAMERA_FRONT2_USE_DEFAULT_AWB_CAL
		if (device_create_file(camera_front_dev, &dev_attr_front2_awb_master) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_awb_master.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front2_awb_module) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_awb_module.attr.name);
		}
#endif
#ifdef CAMERA_FRONT2_TILT
		if (device_create_file(camera_front_dev, &dev_attr_front2_tilt) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_tilt.attr.name);
		}
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_front_dev, &dev_attr_front2_hwparam) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front2_hwparam.attr.name);
		}
#endif
#ifdef CAMERA_FRONT2_MODULEID
		if (sysfs_create_file(camera_front_dev, &dev_attr_front2_moduleid) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_front_module2.attr) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_SVC_front_module2.attr.name);
		}
#endif
#endif

#ifdef CAMERA_FRONT_TOF
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_caminfo) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_tof_caminfo.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_camfw) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_tof_camfw.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_camfw_full) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_tof_camfw_full.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_checkfw_factory) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_tof_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_sensorid_exif) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_tof_sensorid_exif.attr.name);
		}
#ifdef CAMERA_FRONT2_TOF_MODULEID
		if (device_create_file(camera_front_dev, &dev_attr_front2_moduleid) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front2_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_front_module2.attr) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_SVC_front_module2.attr.name);
		}
#endif
#ifdef CAMERA_FRONT_TOF_CAL
		if (device_create_file(camera_front_dev, &dev_attr_front_tofcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_front_tofcal.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tofcal_extra) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_front_tofcal_extra.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tofcal_size) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_front_tofcal_size.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tofcal_uid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_front_tofcal_uid.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_dual_cal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_front_tof_dual_cal.attr.name);
		}
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_cal_result) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_front_tof_cal_result.attr.name);
		}
#endif
#ifdef CAMERA_FRONT_TOF_TILT
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_tilt) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_tof_tilt.attr.name);
		}
#endif
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_freq) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_tof_freq.attr.name);
		}
#endif
		if (device_create_file(camera_front_dev, &dev_attr_front_tof_check_pd) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_front_tof_check_pd.attr.name);
		}
#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_FRONT2_TOF_MODULEID
		if (device_create_file(camera_front_dev, &dev_attr_front2_hwparam) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_front2_hwparam.attr.name);
		}
#endif
#endif
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_front_dev, &dev_attr_front_hwparam) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_hwparam.attr.name);
		}
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		if (device_create_file(camera_front_dev, &dev_attr_front_mipi_clock) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_front_mipi_clock.attr.name);
		}
#endif
	}

	camera_rear_dev = device_create(camera_class, NULL, 1, NULL, "rear");
	if (IS_ERR(camera_rear_dev)) {
		pr_err("failed to create rear device!\n");
	} else {
		if (device_create_file(camera_rear_dev, &dev_attr_rear_sensorid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_sensorid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_sensorid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_sensorid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_sensorid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_sensorid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_sensorid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_sensorid.attr.name);
		}
#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
		if (device_create_file(camera_rear_dev, &dev_attr_from_write) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_from_write.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camtype) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_camtype.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_caminfo) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_caminfo.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camfw) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_camfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camfw_full) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_camfw_full.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_checkfw_user) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_checkfw_user.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_checkfw_factory) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_sensorid_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_mtf_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_mtf_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_afcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_afcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_moduleid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_rear_module.attr) < 0) {
			pr_err("failed to create rear device file, %s\n",
			dev_attr_SVC_rear_module.attr.name);
		}
#ifdef CAMERA_REAR_DUAL_CAL
		if (device_create_file(camera_rear_dev, &dev_attr_rear_dualcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_dualcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_dualcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_dualcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_dualcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_dualcal.attr.name);
		}
#endif

#ifdef CAMERA_REAR2
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_caminfo) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear2_caminfo.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_camfw) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear2_camfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_camfw_full) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear2_camfw_full.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_checkfw_user) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear2_checkfw_user.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_checkfw_factory) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear2_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_sensorid_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_mtf_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_mtf_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_awb_master) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_awb_master.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_awb_module) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_awb_module.attr.name);
		}
#ifdef CAMERA_REAR2_AFCAL
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_afcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_afcal.attr.name);
		}
#endif
#ifdef CAMERA_REAR2_TILT
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_tilt) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_tilt.attr.name);
		}
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_hwparam) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear2_hwparam.attr.name);
		}
#endif
#ifdef CAMERA_REAR2_MODULEID
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_moduleid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_rear_module2.attr) < 0) {
			pr_err("failed to create rear device file, %s\n",
			dev_attr_SVC_rear_module2.attr.name);
		}
#endif
#endif
#ifdef CAMERA_REAR3
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_caminfo) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear3_caminfo.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_camfw) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear3_camfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_camfw_full) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear3_camfw_full.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_checkfw_user) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear3_checkfw_user.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_checkfw_factory) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear3_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_sensorid_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_sensorid_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_mtf_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_mtf_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_paf_cal_check) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_paf_cal_check.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_awb_master) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_awb_master.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_awb_module) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_awb_module.attr.name);
		}
#ifdef CAMERA_REAR3_AFCAL
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_afcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_afcal.attr.name);
		}
#endif
#ifdef CAMERA_REAR3_TILT
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_tilt) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_tilt.attr.name);
		}
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_hwparam) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear3_hwparam.attr.name);
		}
#endif
#ifdef CAMERA_REAR3_MODULEID
		if (device_create_file(camera_rear_dev, &dev_attr_rear3_moduleid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear3_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_rear_module3.attr) < 0) {
			pr_err("failed to create rear device file, %s\n",
			dev_attr_SVC_rear_module3.attr.name);
		}
#endif
#endif

#ifdef CAMERA_REAR_TOF
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_caminfo) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_tof_caminfo.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_camfw) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_tof_camfw.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_camfw_full) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_tof_camfw_full.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_checkfw_factory) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_tof_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_sensorid_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_sensorid_exif.attr.name);
		}
#ifdef CAMERA_REAR4_TOF_MODULEID
		if (device_create_file(camera_rear_dev, &dev_attr_rear4_moduleid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear4_moduleid.attr.name);
		}
		if (sysfs_create_file(svc, &dev_attr_SVC_rear_module4.attr) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_SVC_rear_module4.attr.name);
		}
#endif
#ifdef CAMERA_REAR_TOF_CAL
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tofcal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tofcal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tofcal_extra) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tofcal_extra.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tofcal_size) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tofcal_size.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tofcal_uid) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tofcal_uid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_dual_cal) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_dual_cal.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_cal_result) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_cal_result.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_get_validation) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_cal_result.attr.name);
		}
#endif
#ifdef CAMERA_REAR_TOF_TILT
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_tilt) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_tilt.attr.name);
		}
#endif
#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_freq) < 0) {
			pr_err("failed to create front device file, %s\n",
					dev_attr_rear_tof_freq.attr.name);
		}
#endif
#ifdef CAMERA_REAR2_TOF_TILT
		if (device_create_file(camera_rear_dev, &dev_attr_rear2_tof_tilt) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear2_tof_tilt.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_tof_check_pd) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_tof_check_pd.attr.name);
		}
#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_REAR4_TOF_MODULEID
		if (device_create_file(camera_rear_dev, &dev_attr_rear4_hwparam) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear4_hwparam.attr.name);
		}
#endif
#endif
#endif

		if (device_create_file(camera_rear_dev, &dev_attr_ssrm_camera_info) < 0) {
			pr_err("failed to create front device file, %s\n",
				dev_attr_ssrm_camera_info.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_sensor_standby) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_sensor_standby.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_supported_cameraIds) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_supported_cameraIds.attr.name);
		}
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_rear_dev, &dev_attr_rear_hwparam) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_hwparam.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_calcheck) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_rear_calcheck.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_fw_update) < 0) {
			pr_err("failed to create rear device file, %s\n",
				dev_attr_fw_update.attr.name);
		}
#ifdef FORCE_CAL_LOAD
		if (device_create_file(camera_rear_dev, &dev_attr_rear_force_cal_load) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_force_cal_load.attr.name);
		}
#endif
#ifdef ROM_SUPPORT_APERTURE_F2
		if (device_create_file(camera_rear_dev, &dev_attr_rear_mtf2_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_mtf2_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_f2_paf_offset_mid) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_f2_paf_offset_mid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_f2_paf_offset_far) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_f2_paf_offset_far.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_f2_paf_cal_check) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_f2_paf_cal_check.attr.name);
		}
#endif
#ifdef ROM_SUPPORT_APERTURE_F3
		if (device_create_file(camera_rear_dev, &dev_attr_rear_mtf3_exif) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_mtf3_exif.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_f3_paf_offset_mid) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_f3_paf_offset_mid.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_f3_paf_offset_far) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_f3_paf_offset_far.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_f3_paf_cal_check) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_f3_paf_cal_check.attr.name);
		}
#endif
		if (device_create_file(camera_rear_dev, &dev_attr_rear_awb_master) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_awb_master.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_awb_module) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_awb_module.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_camfw_all) < 0) {
			pr_err("failed to create rear device file, %s\n",
					dev_attr_rear_camfw_all.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_paf_offset_mid) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_paf_offset_far.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_paf_offset_far) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_paf_offset_far.attr.name);
		}
		if (device_create_file(camera_rear_dev, &dev_attr_rear_paf_cal_check) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
					dev_attr_rear_paf_cal_check.attr.name);
		}
#if defined (CONFIG_OIS_USE)
		if (device_create_file(camera_rear_dev, &dev_attr_rear_aperture_halltest) < 0) {
			printk(KERN_ERR "failed to create rear device file, %s\n",
				dev_attr_rear_aperture_halltest.attr.name);
		}
#endif
	}

#if defined (CONFIG_OIS_USE)
	camera_ois_dev = device_create(camera_class, NULL, 2, NULL, "ois");
	if (IS_ERR(camera_ois_dev)) {
		pr_err("failed to create ois device!\n");
	} else {
		if (device_create_file(camera_ois_dev, &dev_attr_selftest) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_selftest.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_power) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_power.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_autotest) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_autotest.attr.name);
		}
#ifdef CAMERA_2ND_OIS
		if (device_create_file(camera_ois_dev, &dev_attr_autotest_2nd) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_autotest_2nd.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_gain_rear3) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_gain_rear3.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_supperssion_ratio_rear3) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_supperssion_ratio_rear3.attr.name);
		}
#endif
		if (device_create_file(camera_ois_dev, &dev_attr_ois_rawdata) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_rawdata.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_calibrationtest) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_calibrationtest.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_oisfw) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_oisfw.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_diff) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_diff.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_exif) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_exif.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_gain_rear) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_gain_rear.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_ois_supperssion_ratio_rear) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_ois_supperssion_ratio_rear.attr.name);
		}
		if (device_create_file(camera_ois_dev, &dev_attr_reset_check) < 0) {
			pr_err("failed to create ois device file, %s\n",
				dev_attr_reset_check.attr.name);
		}
	}
#endif
#ifdef SECURE_CAMERA_IRIS
	camera_secure_dev = device_create(camera_class, NULL, 3, NULL, "secure");
	if (IS_ERR(camera_secure_dev)) {
		pr_err("failed to create secure device!\n");
	} else {
		if (device_create_file(camera_secure_dev, &dev_attr_secure_sensorid) < 0) {
			pr_err("failed to create secure device file, %s\n",
				dev_attr_secure_sensorid.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_camfw) < 0) {
			pr_err("failed to create iris device file, %s\n",
				dev_attr_iris_camfw.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_camfw_full) < 0) {
			pr_err("failed to create iris device file, %s\n",
				dev_attr_iris_camfw_full.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_checkfw_user) < 0) {
			pr_err("failed to create iris device file, %s\n",
				dev_attr_iris_checkfw_user.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_checkfw_factory) < 0) {
			pr_err("failed to create iris device file, %s\n",
				dev_attr_iris_checkfw_factory.attr.name);
		}
		if (device_create_file(camera_secure_dev, &dev_attr_iris_caminfo) < 0) {
			pr_err("failed to create iris device file, %s\n",
				dev_attr_iris_caminfo.attr.name);
		}
#ifdef USE_CAMERA_HW_BIG_DATA
		if (device_create_file(camera_secure_dev, &dev_attr_iris_hwparam) < 0) {
			pr_err("failed to create iris device file, %s\n",
				dev_attr_iris_hwparam.attr.name);
		}
#endif
	}
#endif

	sysfs_core = core;

	return 0;
}

int fimc_is_destroy_sysfs(struct fimc_is_core *core)
{
	if (camera_front_dev) {
		device_remove_file(camera_front_dev, &dev_attr_front_sensorid);
		device_remove_file(camera_front_dev, &dev_attr_front2_sensorid);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_sensorid);
		device_remove_file(camera_front_dev, &dev_attr_front_camtype);
		device_remove_file(camera_front_dev, &dev_attr_front_caminfo);
		device_remove_file(camera_front_dev, &dev_attr_front_camfw);
#if defined(CONFIG_CAMERA_EEPROM_SUPPORT_FRONT)
		device_remove_file(camera_front_dev, &dev_attr_front_afcal);
		device_remove_file(camera_front_dev, &dev_attr_front_camfw_full);
		device_remove_file(camera_front_dev, &dev_attr_front_checkfw_factory);
		device_remove_file(camera_front_dev, &dev_attr_front_moduleid);
		device_remove_file(camera_front_dev, &dev_attr_front_mtf_exif);
		device_remove_file(camera_front_dev, &dev_attr_front_paf_cal_check);
		device_remove_file(camera_front_dev, &dev_attr_front_sensorid_exif);
		device_remove_file(camera_front_dev, &dev_attr_front_awb_master);
		device_remove_file(camera_front_dev, &dev_attr_front_awb_module);
#endif
#ifdef CAMERA_FRONT_DUAL_CAL
		device_remove_file(camera_front_dev, &dev_attr_front_dualcal);
		device_remove_file(camera_front_dev, &dev_attr_front2_dualcal);
#endif
#ifdef CAMERA_FRONT2
		device_remove_file(camera_front_dev, &dev_attr_front2_caminfo);
		device_remove_file(camera_front_dev, &dev_attr_front2_camfw);
		device_remove_file(camera_front_dev, &dev_attr_front2_camfw_full);
		device_remove_file(camera_front_dev, &dev_attr_front2_checkfw_factory);
		device_remove_file(camera_front_dev, &dev_attr_front2_sensorid_exif);
		device_remove_file(camera_front_dev, &dev_attr_front2_mtf_exif);
#ifndef CAMERA_FRONT2_USE_DEFAULT_AWB_CAL
		device_remove_file(camera_front_dev, &dev_attr_front2_awb_master);
		device_remove_file(camera_front_dev, &dev_attr_front2_awb_module);
#endif
#ifdef CAMERA_FRONT2_TILT
		device_remove_file(camera_front_dev, &dev_attr_front2_tilt);
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_front_dev, &dev_attr_front2_hwparam);
#endif
#ifdef CAMERA_FRONT2_MODULEID
		device_remove_file(camera_front_dev, &dev_attr_front2_moduleid);
#endif
#endif

#ifdef CAMERA_FRONT_TOF
		device_remove_file(camera_front_dev, &dev_attr_front_tof_caminfo);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_camfw);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_camfw_full);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_checkfw_factory);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_sensorid_exif);
#ifdef CAMERA_FRONT2_TOF_MODULEID
		device_remove_file(camera_front_dev, &dev_attr_front2_moduleid);
#endif
#ifdef CAMERA_FRONT_TOF_CAL
		device_remove_file(camera_front_dev, &dev_attr_front_tofcal);
		device_remove_file(camera_front_dev, &dev_attr_front_tofcal_extra);
		device_remove_file(camera_front_dev, &dev_attr_front_tofcal_size);
		device_remove_file(camera_front_dev, &dev_attr_front_tofcal_uid);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_dual_cal);
		device_remove_file(camera_front_dev, &dev_attr_front_tof_cal_result);

#endif
#ifdef CAMERA_FRONT_TOF_TILT
		device_remove_file(camera_front_dev, &dev_attr_front_tof_tilt);
#endif
#ifdef USE_CAMERA_FRONT_TOF_TX_FREQ_VARIATION
		device_remove_file(camera_front_dev, &dev_attr_front_tof_freq);
#endif
		device_remove_file(camera_front_dev, &dev_attr_front_tof_check_pd);
#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_FRONT2_TOF_MODULEID
		device_remove_file(camera_front_dev, &dev_attr_front2_hwparam);
#endif
#endif
#endif

#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_front_dev, &dev_attr_front_hwparam);
#endif
#ifdef USE_CAMERA_MIPI_CLOCK_VARIATION
		device_remove_file(camera_front_dev, &dev_attr_front_mipi_clock);
#endif
	}

	if (camera_rear_dev) {
		device_remove_file(camera_rear_dev, &dev_attr_rear_sensorid);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_sensorid);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_sensorid);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_sensorid);

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
		device_remove_file(camera_rear_dev, &dev_attr_from_write);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_camtype);
		device_remove_file(camera_rear_dev, &dev_attr_rear_caminfo);
		device_remove_file(camera_rear_dev, &dev_attr_rear_camfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear_camfw_full);
		device_remove_file(camera_rear_dev, &dev_attr_rear_checkfw_user);
		device_remove_file(camera_rear_dev, &dev_attr_rear_checkfw_factory);
		device_remove_file(camera_rear_dev, &dev_attr_rear_sensorid_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear_mtf_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear_afcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear_moduleid);
#ifdef CAMERA_REAR_DUAL_CAL
		device_remove_file(camera_rear_dev, &dev_attr_rear_dualcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_dualcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_dualcal);
#endif
#ifdef CAMERA_REAR2
		device_remove_file(camera_rear_dev, &dev_attr_rear2_caminfo);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_camfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_camfw_full);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_checkfw_user);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_checkfw_factory);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_sensorid_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_mtf_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_awb_master);
		device_remove_file(camera_rear_dev, &dev_attr_rear2_awb_module);
#ifdef CAMERA_REAR2_AFCAL
		device_remove_file(camera_rear_dev, &dev_attr_rear2_afcal);
#endif
#ifdef CAMERA_REAR2_TILT
		device_remove_file(camera_rear_dev, &dev_attr_rear2_tilt);
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_rear_dev, &dev_attr_rear2_hwparam);
#endif
#ifdef CAMERA_REAR2_MODULEID
		device_remove_file(camera_rear_dev, &dev_attr_rear2_moduleid);
#endif
#endif
#ifdef CAMERA_REAR3
		device_remove_file(camera_rear_dev, &dev_attr_rear3_caminfo);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_camfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_camfw_full);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_checkfw_user);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_checkfw_factory);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_sensorid_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_mtf_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_paf_cal_check);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_awb_master);
		device_remove_file(camera_rear_dev, &dev_attr_rear3_awb_module);
#ifdef CAMERA_REAR3_AFCAL
		device_remove_file(camera_rear_dev, &dev_attr_rear3_afcal);
#endif
#ifdef CAMERA_REAR3_TILT
		device_remove_file(camera_rear_dev, &dev_attr_rear3_tilt);
#endif
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_rear_dev, &dev_attr_rear3_hwparam);
#endif
#ifdef CAMERA_REAR3_MODULEID
		device_remove_file(camera_rear_dev, &dev_attr_rear3_moduleid);
#endif
#endif

#ifdef CAMERA_REAR_TOF
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_caminfo);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_camfw);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_camfw_full);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_checkfw_factory);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_sensorid_exif);
#ifdef CAMERA_REAR4_TOF_MODULEID
		device_remove_file(camera_rear_dev, &dev_attr_rear4_moduleid);
#endif
#ifdef CAMERA_REAR_TOF_CAL
		device_remove_file(camera_rear_dev, &dev_attr_rear_tofcal);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tofcal_extra);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tofcal_size);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tofcal_uid);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_dual_cal);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_cal_result);
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_get_validation);
#endif
#ifdef CAMERA_REAR_TOF_TILT
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_tilt);
#endif
#if defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION) || defined(USE_CAMERA_REAR_TOF_TX_FREQ_VARIATION_SYSFS_ENABLE)
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_freq);
#endif

#ifdef CAMERA_REAR2_TOF_TILT
		device_remove_file(camera_rear_dev, &dev_attr_rear2_tof_tilt);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_tof_check_pd);
#ifdef USE_CAMERA_HW_BIG_DATA
#ifdef CAMERA_REAR4_TOF_MODULEID
		device_remove_file(camera_rear_dev, &dev_attr_rear4_hwparam);
#endif
#endif
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_sensor_standby);
		device_remove_file(camera_rear_dev, &dev_attr_rear_calcheck);
		device_remove_file(camera_rear_dev, &dev_attr_supported_cameraIds);
#ifdef FORCE_CAL_LOAD
		device_remove_file(camera_rear_dev, &dev_attr_rear_force_cal_load);
#endif
#ifdef ROM_SUPPORT_APERTURE_F2
		device_remove_file(camera_rear_dev, &dev_attr_rear_mtf2_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear_f2_paf_offset_mid);
		device_remove_file(camera_rear_dev, &dev_attr_rear_f2_paf_offset_far);
		device_remove_file(camera_rear_dev, &dev_attr_rear_f2_paf_cal_check);
#endif
#ifdef ROM_SUPPORT_APERTURE_F3
		device_remove_file(camera_rear_dev, &dev_attr_rear_mtf3_exif);
		device_remove_file(camera_rear_dev, &dev_attr_rear_f3_paf_offset_mid);
		device_remove_file(camera_rear_dev, &dev_attr_rear_f3_paf_offset_far);
		device_remove_file(camera_rear_dev, &dev_attr_rear_f3_paf_cal_check);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_awb_master);
		device_remove_file(camera_rear_dev, &dev_attr_rear_awb_module);
		device_remove_file(camera_rear_dev, &dev_attr_fw_update);
		device_remove_file(camera_rear_dev, &dev_attr_ssrm_camera_info);
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_rear_dev, &dev_attr_rear_hwparam);
#endif
		device_remove_file(camera_rear_dev, &dev_attr_rear_camfw_all);
		device_remove_file(camera_rear_dev, &dev_attr_rear_paf_offset_mid);
		device_remove_file(camera_rear_dev, &dev_attr_rear_paf_offset_far);
		device_remove_file(camera_rear_dev, &dev_attr_rear_paf_cal_check);
#if defined (CONFIG_OIS_USE)
		device_remove_file(camera_rear_dev, &dev_attr_rear_aperture_halltest);
#endif
	}

#if defined (CONFIG_OIS_USE)
	if (camera_ois_dev) {
		device_remove_file(camera_ois_dev, &dev_attr_selftest);
		device_remove_file(camera_ois_dev, &dev_attr_ois_power);
		device_remove_file(camera_ois_dev, &dev_attr_autotest);
#ifdef CAMERA_2ND_OIS
		device_remove_file(camera_ois_dev, &dev_attr_autotest_2nd);
		device_remove_file(camera_ois_dev, &dev_attr_ois_gain_rear3);
		device_remove_file(camera_ois_dev, &dev_attr_ois_supperssion_ratio_rear3);
#endif
		device_remove_file(camera_ois_dev, &dev_attr_ois_rawdata);
		device_remove_file(camera_ois_dev, &dev_attr_calibrationtest);
		device_remove_file(camera_ois_dev, &dev_attr_oisfw);
		device_remove_file(camera_ois_dev, &dev_attr_ois_diff);
		device_remove_file(camera_ois_dev, &dev_attr_ois_exif);
		device_remove_file(camera_ois_dev, &dev_attr_ois_gain_rear);
		device_remove_file(camera_ois_dev, &dev_attr_ois_supperssion_ratio_rear);
		device_remove_file(camera_ois_dev, &dev_attr_reset_check);
	}
#endif
#ifdef SECURE_CAMERA_IRIS
	if (camera_secure_dev) {
		device_remove_file(camera_secure_dev, &dev_attr_secure_sensorid);
		device_remove_file(camera_secure_dev, &dev_attr_iris_camfw);
		device_remove_file(camera_secure_dev, &dev_attr_iris_camfw_full);
		device_remove_file(camera_secure_dev, &dev_attr_iris_checkfw_user);
		device_remove_file(camera_secure_dev, &dev_attr_iris_checkfw_factory);
		device_remove_file(camera_secure_dev, &dev_attr_iris_caminfo);
#ifdef USE_CAMERA_HW_BIG_DATA
		device_remove_file(camera_secure_dev, &dev_attr_iris_hwparam);
#endif
	}
#endif

	if (camera_class) {
		if (camera_front_dev)
			device_destroy(camera_class, camera_front_dev->devt);

		if (camera_rear_dev)
			device_destroy(camera_class, camera_rear_dev->devt);

#if defined (CONFIG_OIS_USE)
		if (camera_ois_dev)
			device_destroy(camera_class, camera_ois_dev->devt);
#endif
#ifdef SECURE_CAMERA_IRIS
		if (camera_secure_dev)
			device_destroy(camera_class, camera_secure_dev->devt);
#endif
	}

	class_destroy(camera_class);

	return 0;
}
