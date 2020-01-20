/*
 * Samsung Exynos5 SoC series FIMC-IS OIS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define OIS_BIN_LEN		45056
#define FW_CORE_VERSION		0
#define FW_GYRO_SENSOR		1
#define FW_DRIVER_IC		2
#define FW_RELEASE_YEAR		3
#define FW_RELEASE_MONTH	4
#define FW_RELEASE_COUNT		5
#define USER_FW_SIZE		256
#define USER_SHIFT_SIZE		57
#define OIS_FW_HEADER_SIZE	7
#define OIS_BIN_HEADER		45041
#define FW_TRANS_SIZE		256
#define OIS_VER_OFFSET	14
#define OIS_FW_UPDATE_ERROR_STATUS	0x1163
#define CAMERA_OIS_EXT_CLK_12MHZ 0xB71B00
#define CAMERA_OIS_EXT_CLK_17MHZ 0x1036640
#define CAMERA_OIS_EXT_CLK_19MHZ 0x124F800
#define CAMERA_OIS_EXT_CLK_24MHZ 0x16E3600
#define CAMERA_OIS_EXT_CLK_26MHZ 0x18CBA80
#define CAMERA_OIS_EXT_CLK_32MHZ 0x1E84800
#define CAMERA_OIS_EXT_CLK CAMERA_OIS_EXT_CLK_32MHZ
#define AF_BOUNDARY					(1 << 6)
#define POSITION_NUM				512
#define OIS_MEM_STATUS_RETRY 6 // This is a guide from OIS FW(CDG).
#define OIS_BOOT_FW_SIZE	4096
#define OIS_PROG_FW_SIZE	(1024 * 40)
#define SCALE				10000
#define Coef_angle_max		3500		// unit : 1/SCALE, OIS Maximum compensation angle, 0.35*SCALE
#define SH_THRES			798000		// unit : 1/SCALE, 39.9*SCALE
#define Gyrocode			1000			// Gyro input code for 1 angle degree
#define SWAP32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#define RND_DIV(num, den) ((num > 0) ? (num+(den>>1))/den : (num-(den>>1))/den)

enum{
	eBIG_ENDIAN = 0, // big endian
	eLIT_ENDIAN = 1  // little endian
};

enum fimc_is_ois_power_mode {
	OIS_POWER_MODE_SINGLE = 0,
	OIS_POWER_MODE_DUAL,
};

bool fimc_is_ois_read_userdata_s6(struct fimc_is_core *core);
bool fimc_is_ois_fw_version_s6(struct fimc_is_core *core);
int fimc_is_ois_open_fw_s6(struct fimc_is_core *core, char *name, u8 **buf);
int fimc_is_ois_fw_revision_s6(char *fw_ver);
int fimc_is_set_ois_mode_s6(struct v4l2_subdev *subdev, int mode);
int fimc_is_ois_shift_compensation_s6(struct v4l2_subdev *subdev, int position, int resolution);
int fimc_calculate_shift_value(struct v4l2_subdev *subdev);
int fimc_is_ois_init_s6(struct v4l2_subdev *subdev);
int fimc_is_ois_deinit_s6(struct v4l2_subdev *subdev);

int fimc_is_ois_self_test_s6(struct fimc_is_core *core);
bool fimc_is_ois_diff_test_s6(struct fimc_is_core *core, int *x_diff, int *y_diff);
void fimc_is_ois_fw_update_s6(struct fimc_is_core *core);
bool fimc_is_ois_check_fw_s6(struct fimc_is_core *core);
bool fimc_is_ois_auto_test_s6(struct fimc_is_core *core,
	int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y);
#ifdef CAMERA_REAR2_OIS
bool fimc_is_ois_auto_test_rear2_s6(struct fimc_is_core *core,
	int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y,
	bool *x_result_2nd, bool *y_result_2nd, int *sin_x_2nd, int *sin_y_2nd);
#endif
void fimc_is_ois_offset_test_s6(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y);
void fimc_is_ois_get_offset_data_s6(struct fimc_is_core *core, long *raw_data_x, long *raw_data_y);
void fimc_is_ois_gyro_sleep_s6(struct fimc_is_core *core);
void fimc_is_ois_exif_data_s6(struct fimc_is_core *core);
u8 fimc_is_ois_read_status_s6(struct fimc_is_core *core);
u8 fimc_is_ois_read_cal_checksum_s6(struct fimc_is_core *core);
int fimc_is_ois_set_coef_s6(struct v4l2_subdev *subdev, u8 coef);

int fimc_is_ois_set_centering_s6(struct v4l2_subdev *subdev);
int fimc_is_ois_set_still_s6(struct fimc_is_core *core);

int fimc_is_ois_set_ggfadeupdown_s6(struct v4l2_subdev *subdev, int up, int down);
