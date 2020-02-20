/* linux/arch/arm/mach-exynos/include/mach/fimc-is-sysfs.h
 *
 * Copyright (C) 2011 Samsung Electronics, Co. Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FIMC_IS_SYSFS_H_
#define _FIMC_IS_SYSFS_H_

#include "fimc-is-core.h"

enum fimc_is_cam_info_isp {
	CAM_INFO_ISP_TYPE_INTERNAL = 0,
	CAM_INFO_ISP_TYPE_EXTERNAL,
	CAM_INFO_ISP_TYPE_SOC,
};

enum fimc_is_cam_info_cal_mem {
	CAM_INFO_CAL_MEM_TYPE_NONE = 0,
	CAM_INFO_CAL_MEM_TYPE_FROM,
	CAM_INFO_CAL_MEM_TYPE_EEPROM,
	CAM_INFO_CAL_MEM_TYPE_OTP,
};

enum fimc_is_cam_info_read_ver {
	CAM_INFO_READ_VER_SYSFS = 0,
	CAM_INFO_READ_VER_CAMON,
};

enum fimc_is_cam_info_core_voltage {
	CAM_INFO_CORE_VOLT_NONE = 0,
	CAM_INFO_CORE_VOLT_USE,
};

enum fimc_is_cam_info_upgrade {
	CAM_INFO_FW_UPGRADE_NONE = 0,
	CAM_INFO_FW_UPGRADE_SYSFS,
	CAM_INFO_FW_UPGRADE_CAMON,
};

enum fimc_is_cam_info_fw_write {
	CAM_INFO_FW_WRITE_NONE = 0,
	CAM_INFO_FW_WRITE_OS,
	CAM_INFO_FW_WRITE_SD,
	CAM_INFO_FW_WRITE_ALL,
};

enum fimc_is_cam_info_fw_dump {
	CAM_INFO_FW_DUMP_NONE = 0,
	CAM_INFO_FW_DUMP_USE,
};

enum fimc_is_cam_info_companion {
	CAM_INFO_COMPANION_NONE = 0,
	CAM_INFO_COMPANION_USE,
};

enum fimc_is_cam_info_ois {
	CAM_INFO_OIS_NONE = 0,
	CAM_INFO_OIS_USE,
};

enum fimc_is_cam_info_valid {
	CAM_INFO_INVALID = 0,
	CAM_INFO_VALID,
};

enum fimc_is_cam_info_dual_open {
	CAM_INFO_SINGLE_OPEN = 0,
	CAM_INFO_DUAL_OPEN,
};

enum fimc_is_cam_info_index {
	CAM_INFO_REAR = 0,
	CAM_INFO_FRONT,
	CAM_INFO_REAR2,
	CAM_INFO_FRONT2,
	CAM_INFO_REAR3,
	CAM_INFO_FRONT3,
	CAM_INFO_REAR4,
	CAM_INFO_FRONT4,
	CAM_INFO_REAR_TOF,
	CAM_INFO_FRONT_TOF,
	CAM_INFO_IRIS,
	CAM_INFO_MAX
};

struct fimc_is_cam_info {
	unsigned int internal_id;
	unsigned int isp;
	unsigned int cal_memory;
	unsigned int read_version;
	unsigned int core_voltage;
	unsigned int upgrade;
	unsigned int fw_write;
	unsigned int fw_dump;
	unsigned int companion;
	unsigned int ois;
	unsigned int valid;
	unsigned int dual_open;
};

struct fimc_is_common_cam_info {
	unsigned int supported_camera_ids[11];	/*FIMC_IS_SENSOR_COUNT*/
	unsigned int max_supported_camera;
};

int fimc_is_get_cam_info(struct fimc_is_cam_info **caminfo);
void fimc_is_get_common_cam_info(struct fimc_is_common_cam_info **caminfo);

#endif /* _FIMC_IS_SYSFS_H_ */
