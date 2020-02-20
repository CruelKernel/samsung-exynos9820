/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_SEC_DEFINE_H
#define FIMC_IS_SEC_DEFINE_H

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
#include <linux/syscalls.h>
#include <linux/vmalloc.h>

#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/zlib.h>

#include "fimc-is-core.h"
#include "fimc-is-cmd.h"
#include "fimc-is-regs.h"
#include "fimc-is-err.h"
#include "fimc-is-video.h"

#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"
#include "crc32.h"
#include "fimc-is-companion.h"
#include "fimc-is-device-from.h"
#include "fimc-is-dt.h"
#include "fimc-is-device-ois.h"

#define FW_CORE_VER		0
#define FW_PIXEL_SIZE		1
#define FW_ISP_COMPANY		3
#define FW_SENSOR_MAKER		4
#define FW_PUB_YEAR		5
#define FW_PUB_MON		6
#define FW_PUB_NUM		7
#define FW_MODULE_COMPANY	9
#define FW_VERSION_INFO		10

#define FW_ISP_COMPANY_BROADCOMM	'B'
#define FW_ISP_COMPANY_TN		'C'
#define FW_ISP_COMPANY_FUJITSU		'F'
#define FW_ISP_COMPANY_INTEL		'I'
#define FW_ISP_COMPANY_LSI		'L'
#define FW_ISP_COMPANY_MARVELL		'M'
#define FW_ISP_COMPANY_QUALCOMM		'Q'
#define FW_ISP_COMPANY_RENESAS		'R'
#define FW_ISP_COMPANY_STE		'S'
#define FW_ISP_COMPANY_TI		'T'
#define FW_ISP_COMPANY_DMC		'D'

#define FW_SENSOR_MAKER_SF		'F'
#define FW_SENSOR_MAKER_SLSI		'L'			/* rear_front*/
#define FW_SENSOR_MAKER_SONY		'S'			/* rear_front*/
#define FW_SENSOR_MAKER_SLSI_SONY		'X'		/* rear_front*/
#define FW_SENSOR_MAKER_SONY_LSI		'Y'		/* rear_front*/

#define FW_SENSOR_MAKER_SLSI		'L'

#define FW_MODULE_COMPANY_SEMCO		'S'
#define FW_MODULE_COMPANY_GUMI		'O'
#define FW_MODULE_COMPANY_CAMSYS	'C'
#define FW_MODULE_COMPANY_PATRON	'P'
#define FW_MODULE_COMPANY_MCNEX		'M'
#define FW_MODULE_COMPANY_LITEON	'L'
#define FW_MODULE_COMPANY_VIETNAM	'V'
#define FW_MODULE_COMPANY_SHARP		'J'
#define FW_MODULE_COMPANY_NAMUGA	'N'
#define FW_MODULE_COMPANY_POWER_LOGIX	'A'
#define FW_MODULE_COMPANY_DI		'D'

#define FW_2L4_L		"L12XL"		/* BEYOND */
#define FW_3J1_X		"X10LL"		/* BEYOND */

#define SDCARD_FW

#define FIMC_IS_DDK						"fimc_is_lib.bin"
#define FIMC_IS_RTA						"fimc_is_rta.bin"

/*#define FIMC_IS_FW_SDCARD			"/data/media/0/fimc_is_fw.bin" */
/* Rear setfile */
#define FIMC_IS_2L4_SETF			"setfile_2l4.bin"
#define FIMC_IS_3M3_SETF			"setfile_3m3.bin"
#define FIMC_IS_3M5_SETF			"setfile_3m5.bin"

/* Front setfile */
#define FIMC_IS_3J1_SETF			"setfile_3j1.bin"
#define FIMC_IS_4HA_SETF			"setfile_4ha.bin"

#define FIMC_IS_CAL_SDCARD_FRONT		"/data/cal_data_front.bin"
#define FIMC_IS_FW_FROM_SDCARD		"/data/media/0/CamFW_Main.bin"
#define FIMC_IS_SETFILE_FROM_SDCARD		"/data/media/0/CamSetfile_Main.bin"
#define FIMC_IS_KEY_FROM_SDCARD		"/data/media/0/1q2w3e4r.key"

#define FIMC_IS_HEADER_VER_SIZE      11
#define FIMC_IS_CAM_VER_SIZE         11
#define FIMC_IS_OEM_VER_SIZE         11
#define FIMC_IS_AWB_VER_SIZE         11
#define FIMC_IS_SHADING_VER_SIZE     11
#define FIMC_IS_CAL_MAP_VER_SIZE     4
#define FIMC_IS_PROJECT_NAME_SIZE    8
#define FIMC_IS_ISP_SETFILE_VER_SIZE 6
#define FIMC_IS_SENSOR_ID_SIZE       16
#define FIMC_IS_CAL_DLL_VERSION_SIZE 4
#define FIMC_IS_MODULE_ID_SIZE       10
#define FIMC_IS_RESOLUTION_DATA_SIZE 54
#define FIMC_IS_AWB_MASTER_DATA_SIZE 8
#define FIMC_IS_AWB_MODULE_DATA_SIZE 8
#define FIMC_IS_OIS_GYRO_DATA_SIZE 4
#define FIMC_IS_OIS_COEF_DATA_SIZE 2
#define FIMC_IS_OIS_SUPPERSSION_RATIO_DATA_SIZE 2
#define FIMC_IS_OIS_CAL_MARK_DATA_SIZE 1

#define FIMC_IS_PAF_OFFSET_MID_OFFSET		(0x0730) /* REAR PAF OFFSET MID (30CM, WIDE) */
#define FIMC_IS_PAF_OFFSET_MID_SIZE			936
#define FIMC_IS_PAF_OFFSET_PAN_OFFSET		(0x0CD0) /* REAR PAF OFFSET FAR (1M, WIDE) */
#define FIMC_IS_PAF_OFFSET_PAN_SIZE			234
#define FIMC_IS_PAF_CAL_ERR_CHECK_OFFSET	0x14

#define FIMC_IS_ROM_CRC_MAX_LIST 30
#define FIMC_IS_ROM_DUAL_TILT_MAX_LIST 10
#define FIMC_IS_DUAL_TILT_PROJECT_NAME_SIZE 20
#define FIMC_IS_ROM_OIS_MAX_LIST 14

#ifdef USE_BINARY_PADDING_DATA_ADDED
#define FIMC_IS_HEADER_VER_OFFSET   (FIMC_IS_HEADER_VER_SIZE+IS_SIGNATURE_LEN)
#else
#define FIMC_IS_HEADER_VER_OFFSET   (FIMC_IS_HEADER_VER_SIZE)
#endif
#define FIMC_IS_MAX_TUNNING_BUFFER_SIZE (15 * 1024)

#if defined(CAMERA_REAR_TOF_CAL) || defined(CAMERA_FRONT_TOF_CAL)
#define FIMC_IS_TOF_CAL_SIZE_ONCE	4095
#define FIMC_IS_TOF_CAL_CAL_RESULT_OK	0x11
#endif

#define FROM_VERSION_V002 '2'
#define FROM_VERSION_V003 '3'
#define FROM_VERSION_V004 '4'
#define FROM_VERSION_V005 '5'

/* #define DEBUG_FORCE_DUMP_ENABLE */

/* #define CONFIG_RELOAD_CAL_DATA 1 */

enum {
        CC_BIN1 = 0,
        CC_BIN2,
        CC_BIN3,
        CC_BIN4,
        CC_BIN5,
        CC_BIN6,
        CC_BIN7,
        CC_BIN_MAX,
};

enum fnumber_index {
	FNUMBER_1ST = 0,
	FNUMBER_2ND,
	FNUMBER_3RD,
	FNUMBER_MAX
};

enum fimc_is_rom_state {
	FIMC_IS_ROM_STATE_FINAL_MODULE,
	FIMC_IS_ROM_STATE_LATEST_MODULE,
	FIMC_IS_ROM_STATE_INVALID_ROM_VERSION,
	FIMC_IS_ROM_STATE_OTHER_VENDOR,	/* Q module */
	FIMC_IS_ROM_STATE_CAL_RELOAD,
	FIMC_IS_ROM_STATE_CAL_READ_DONE,
	FIMC_IS_ROM_STATE_FW_FIND_DONE,
	FIMC_IS_ROM_STATE_CHECK_BIN_DONE,
	FIMC_IS_ROM_STATE_SKIP_CAL_LOADING,
	FIMC_IS_ROM_STATE_SKIP_CRC_CHECK,
	FIMC_IS_ROM_STATE_SKIP_HEADER_LOADING,
	FIMC_IS_ROM_STATE_MAX
};

enum fimc_is_crc_error {
	FIMC_IS_CRC_ERROR_HEADER,
	FIMC_IS_CRC_ERROR_ALL_SECTION,
	FIMC_IS_CRC_ERROR_DUAL_CAMERA,
	FIMC_IS_CRC_ERROR_FIRMWARE,
	FIMC_IS_CRC_ERROR_SETFILE_1,
	FIMC_IS_CRC_ERROR_SETFILE_2,
	FIMC_IS_CRC_ERROR_HIFI_TUNNING,
	FIMC_IS_ROM_ERROR_MAX
};

struct fimc_is_rom_info {
	unsigned long	rom_state;
	unsigned long	crc_error;

	/* set by dts parsing */
	u32		rom_power_position;
	u32		rom_size;

	char	camera_module_es_version;
	u32		cal_map_es_version;

	u32		header_crc_check_list[FIMC_IS_ROM_CRC_MAX_LIST];
	u32		header_crc_check_list_len;
	u32		crc_check_list[FIMC_IS_ROM_CRC_MAX_LIST];
	u32		crc_check_list_len;
	u32		dual_crc_check_list[FIMC_IS_ROM_CRC_MAX_LIST];
	u32		dual_crc_check_list_len;
	u32		rom_dualcal_slave0_tilt_list[FIMC_IS_ROM_DUAL_TILT_MAX_LIST];
	u32		rom_dualcal_slave0_tilt_list_len;
	u32		rom_dualcal_slave1_tilt_list[FIMC_IS_ROM_DUAL_TILT_MAX_LIST];
	u32		rom_dualcal_slave1_tilt_list_len;
	u32		rom_dualcal_slave2_tilt_list[FIMC_IS_ROM_DUAL_TILT_MAX_LIST];
	u32		rom_dualcal_slave2_tilt_list_len;
	u32		rom_dualcal_slave3_tilt_list[FIMC_IS_ROM_DUAL_TILT_MAX_LIST];
	u32		rom_dualcal_slave3_tilt_list_len;
	u32		rom_ois_list[FIMC_IS_ROM_OIS_MAX_LIST];
	u32		rom_ois_list_len;

	/* set -1 if not support */
	int32_t		rom_header_cal_data_start_addr;
	int32_t		rom_header_version_start_addr;
	int32_t		rom_header_cal_map_ver_start_addr;
	int32_t		rom_header_isp_setfile_ver_start_addr;
	int32_t		rom_header_project_name_start_addr;
	int32_t		rom_header_module_id_addr;
	int32_t		rom_header_sensor_id_addr;
	int32_t		rom_header_mtf_data_addr;
	int32_t		rom_header_f2_mtf_data_addr;
	int32_t		rom_header_f3_mtf_data_addr;
	int32_t		rom_awb_master_addr;
	int32_t		rom_awb_module_addr;
	int32_t		rom_af_cal_macro_addr;
	int32_t		rom_af_cal_d50_addr;
	int32_t		rom_af_cal_pan_addr;

	int32_t		rom_header_sensor2_id_addr;
	int32_t		rom_header_sensor2_version_start_addr;
	int32_t		rom_header_sensor2_mtf_data_addr;
	int32_t		rom_sensor2_awb_master_addr;
	int32_t		rom_sensor2_awb_module_addr;
	int32_t		rom_sensor2_af_cal_macro_addr;
	int32_t		rom_sensor2_af_cal_d50_addr;
	int32_t		rom_sensor2_af_cal_pan_addr;

	int32_t		rom_dualcal_slave0_start_addr;
	int32_t		rom_dualcal_slave0_size;
	int32_t		rom_dualcal_slave1_start_addr;
	int32_t		rom_dualcal_slave1_size;
	int32_t		rom_dualcal_slave2_start_addr;
	int32_t		rom_dualcal_slave2_size;

	int32_t		rom_tof_cal_size_addr[TOF_CAL_SIZE_MAX];
	int32_t		rom_tof_cal_size_addr_len;
	int32_t		rom_tof_cal_start_addr;
	int32_t		rom_tof_cal_uid_addr[TOF_CAL_UID_MAX];
	int32_t		rom_tof_cal_uid_addr_len;
	int32_t		rom_tof_cal_result_addr;
	int32_t		rom_tof_cal_validation_addr[TOF_CAL_VALID_MAX];
	int32_t		rom_tof_cal_validation_addr_len;

	u32		bin_start_addr;		/* DDK */
	u32		bin_end_addr;
#ifdef USE_RTA_BINARY
	u32		rta_bin_start_addr;	/* RTA */
	u32		rta_bin_end_addr;
#endif
	u32		setfile_start_addr;
	u32		setfile_end_addr;
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	u32		setfile2_start_addr;
	u32		setfile2_end_addr;
#endif
	u32		front_setfile_start_addr;
	u32		front_setfile_end_addr;

	u32		hifi_tunning_start_addr;	/* HIFI TUNNING */
	u32		hifi_tunning_end_addr;

	char		header_ver[FIMC_IS_HEADER_VER_SIZE + 1];
	char		header2_ver[FIMC_IS_HEADER_VER_SIZE + 1];
	char		cal_map_ver[FIMC_IS_CAL_MAP_VER_SIZE + 1];
	char		setfile_ver[SETFILE_VER_LEN + 1];

	char		load_fw_name[50];		/* DDK */
	char		load_rta_fw_name[50];	/* RTA */

	char		load_setfile_name[50];
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	char		load_setfile2_name[50];  /*rear 2*/
#endif
	char		load_front_setfile_name[50];
	char		load_tunning_hifills_name[50];
	char		project_name[FIMC_IS_PROJECT_NAME_SIZE + 1];
	char		rom_sensor_id[FIMC_IS_SENSOR_ID_SIZE + 1];
	char		rom_sensor2_id[FIMC_IS_SENSOR_ID_SIZE + 1];
	u8		rom_module_id[FIMC_IS_MODULE_ID_SIZE + 1];
	unsigned long		fw_size;
#ifdef USE_RTA_BINARY
	unsigned long		rta_fw_size;
#endif
	unsigned long		setfile_size;
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	unsigned long		setfile2_size;
#endif
	unsigned long		front_setfile_size;
	unsigned long		hifi_tunning_size;
};

bool fimc_is_sec_get_force_caldata_dump(void);
int fimc_is_sec_set_force_caldata_dump(bool fcd);

ssize_t write_data_to_file(char *name, char *buf, size_t count, loff_t *pos);
ssize_t read_data_rom_file(char *name, char *buf, size_t count, loff_t *pos);
bool fimc_is_sec_file_exist(char *name);

int fimc_is_sec_get_sysfs_finfo(struct fimc_is_rom_info **finfo, int rom_id);
int fimc_is_sec_get_sysfs_pinfo(struct fimc_is_rom_info **pinfo, int rom_id);
int fimc_is_sec_get_front_cal_buf(char **buf, int rom_id);

int fimc_is_sec_get_cal_buf(char **buf, int rom_id);
int fimc_is_sec_get_loaded_fw(char **buf);
int fimc_is_sec_set_loaded_fw(char *buf);
int fimc_is_sec_get_loaded_c1_fw(char **buf);
int fimc_is_sec_set_loaded_c1_fw(char *buf);

int fimc_is_sec_get_camid(void);
int fimc_is_sec_set_camid(int id);
int fimc_is_sec_get_pixel_size(char *header_ver);
int fimc_is_sec_sensorid_find(struct fimc_is_core *core);
int fimc_is_sec_sensorid_find_front(struct fimc_is_core *core);
int fimc_is_sec_sensorid_find_rear_tof(struct fimc_is_core *core);
int fimc_is_sec_fw_find(struct fimc_is_core *core);
int fimc_is_sec_run_fw_sel(struct device *dev, int rom_id);

int fimc_is_sec_readfw(struct fimc_is_core *core);
int fimc_is_sec_read_setfile(struct fimc_is_core *core);
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
int fimc_is_sec_inflate_fw(u8 **buf, unsigned long *size);
#endif
int fimc_is_sec_fw_sel_eeprom(struct device *dev, int id, bool headerOnly);
int fimc_is_sec_write_fw(struct fimc_is_core *core, struct device *dev);
#if defined(CONFIG_CAMERA_FROM)
int fimc_is_sec_readcal(struct fimc_is_core *core);
int fimc_is_sec_fw_sel(struct fimc_is_core *core, struct device *dev, bool headerOnly);
int fimc_is_sec_check_bin_files(struct fimc_is_core *core);
#endif
int fimc_is_sec_fw_revision(char *fw_ver);
int fimc_is_sec_fw_revision(char *fw_ver);
bool fimc_is_sec_fw_module_compare(char *fw_ver1, char *fw_ver2);
int fimc_is_sec_compare_ver(int rom_id);
bool fimc_is_sec_check_rom_ver(struct fimc_is_core *core, int rom_id);

bool fimc_is_sec_check_fw_crc32(char *buf, u32 checksum_seed, unsigned long size);
bool fimc_is_sec_check_cal_crc32(char *buf, int id);
void fimc_is_sec_make_crc32_table(u32 *table, u32 id);

int fimc_is_sec_gpio_enable(struct exynos_platform_fimc_is *pdata, char *name, bool on);
int fimc_is_sec_core_voltage_select(struct device *dev, char *header_ver);
int fimc_is_sec_ldo_enable(struct device *dev, char *name, bool on);
int fimc_is_sec_rom_power_on(struct fimc_is_core *core, int position);
int fimc_is_sec_rom_power_off(struct fimc_is_core *core, int position);
void remove_dump_fw_file(void);
int fimc_is_get_dual_cal_buf(int slave_position, char **buf, int *size);
int fimc_is_sec_sensor_find_front_tof_uid(struct fimc_is_core *core, char *buf);
int fimc_is_sec_sensor_find_rear_tof_uid(struct fimc_is_core *core, char *buf);
#endif /* FIMC_IS_SEC_DEFINE_H */
