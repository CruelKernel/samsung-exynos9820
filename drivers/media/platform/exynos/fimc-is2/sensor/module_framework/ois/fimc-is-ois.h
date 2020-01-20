/*
 * Samsung Exynos5 SoC series OIS driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_OIS_FW_H
#define FIMC_IS_OIS_FW_H

#define FIMC_IS_OIS_SDCARD_PATH			"/data/media/0/"
#define FIMC_IS_OIS_FW_NAME_SEC			"ois_fw_sec.bin"
#define FIMC_IS_OIS_FW_NAME_DOM			"ois_fw_dom.bin"

#ifdef CONFIG_OIS_DIRECT_FW_CONTROL
int fimc_is_ois_fw_open(struct fimc_is_ois *ois, char *name);
int fimc_is_ois_fw_ver_copy(struct fimc_is_ois *ois, u8 *buf, long size);
#endif
#endif
