/*
 * Samsung EXYNOS FIMC-IS (Imaging Subsystem) driver
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_HW_DM_H
#define FIMC_IS_HW_DM_H

#include "fimc-is-metadata.h"

int copy_ctrl_to_dm(struct camera2_shot *shot);

#endif	/* FIMC_IS_HW_DM_H */
