/*
 * Samsung Exynos5 SoC series ois driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_DEVICE_OIS_H
#define FIMC_IS_DEVICE_OIS_H

#define OIS_CAL_NUM			9
#define POSITION_NUM			512
#define AF_BOUNDARY			(1 << 7)
#define OIS_FW_UPDATE_ERROR_STATUS	0x1163

#define OIS_BIN_HEADER			28657
#define OIS_BOOT_FW_SIZE		(1024 * 4)
#define OIS_FW_SIZE			(1024 * 24)
#define FW_TRANS_SIZE			256

#define FIMC_IS_OIS_FW_VER_SIZE		7
#define FIMC_IS_OIS_CAL_VER_SIZE	4
#define FIMC_IS_MAX_OIS_FW_SIZE		(12 * 1024)

#define FW_OIS_VERSION			0
#define FW_GYRO_SENSOR			1
#define FW_DRIVER_IC			2

#define OIS_FW_VERSION_ADDR_OFFSET	0x40
#define OIS_CAL_VERSION_ADDR_OFFSET	0x48

#endif

