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

#define OIS_BIN_LEN		28672
#define FW_GYRO_SENSOR		0
#define FW_DRIVER_IC		1
#define FW_CORE_VERSION		2
#define FW_RELEASE_MONTH	3
#define FW_RELEASE_COUNT		4
#define OIS_FW_HEADER_SIZE	6
#define OIS_BIN_HEADER		28658
#define FW_TRANS_SIZE		256
#define OIS_VER_OFFSET	14

bool fimc_is_ois_read_userdata(struct fimc_is_core *core);
bool fimc_is_ois_fw_version(struct fimc_is_core *core);
int fimc_is_ois_open_fw(struct fimc_is_core *core, char *name, u8 **buf);
int fimc_is_ois_fw_revision(char *fw_ver);

int fimc_is_ois_self_test_impl(struct fimc_is_core *core);
bool fimc_is_ois_diff_test_impl(struct fimc_is_core *core, int *x_diff, int *y_diff);
void fimc_is_ois_fw_update_impl(struct fimc_is_core *core);
bool fimc_is_ois_check_fw_impl(struct fimc_is_core *core);
bool fimc_is_ois_auto_test_impl(struct fimc_is_core *core,
		            int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y);
