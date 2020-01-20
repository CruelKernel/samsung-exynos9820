/*
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _EXYNOS_SMU_H_
#define _EXYNOS_SMU_H_

#define ACCESS_CONTROL_ABORT	0x14

enum smu_id {
	SMU_EMBEDDED = 0,
	SMU_UFSCARD = 1,
	SMU_SDCARD = 2,
	SMU_ID_MAX,
};

enum smu_command {
	SMU_INIT = 0,
	SMU_SET = 1,
	SMU_ABORT = 2,
};

int exynos_smu_init(int id, int smu_cmd);
int exynos_smu_resume(int id);
int exynos_smu_abort(int id, int smu_cmd);
#endif /* _EXYNOS_SMU_H_ */
