/*
 * Samsung EXYNOS FIMC-IS2 (Imaging Subsystem) driver
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_SFR_VRA_V20_H
#define FIMC_IS_SFR_VRA_V20_H

#include "fimc-is-hw-api-common.h"

#if 1 /* Deprecated register. These are maintained for backward compatibility */
enum fimc_is_vra_chain0_reg_name {
	VRA_R_CHAIN0_CURRENT_INT,
	VRA_R_CHAIN0_ENABLE_INT,
	VRA_R_CHAIN0_STATUS_INT,
	VRA_R_CHAIN0_CLEAR_INT,
	VRA_CHAIN0_REG_CNT
};

static const struct fimc_is_reg vra_chain0_regs[VRA_CHAIN0_REG_CNT] = {
	{0x1404, "CHAIN0_CURRENT_INT"},
	{0x1408, "CHAIN0_ENABLE_INT"},
	{0x140C, "CHAIN0_STATUS_INT"},
	{0x1410, "CHAIN0_CLEAR_INT"},
};

enum fimc_is_vra_chain0_reg_field {
	VRA_F_CHAIN0_CURRENT_INT,
	VRA_F_CHAIN0_ENABLE_INT,
	VRA_F_CHAIN0_STATUS_INT,
	VRA_F_CHAIN0_CLEAR_INT,
	VRA_CHAIN0_REG_FIELD_CNT
};

static const struct fimc_is_field vra_chain0_fields[VRA_CHAIN0_REG_FIELD_CNT] = {
	/* 1. field_name 2. bit start 3. bit width 4. access type 5. reset */
	{"CHAIN0_CURRENT_INT",		0,	26,	RO,	0},		/* CHAIN0_CURRENT_INT */
	{"CHAIN0_ENABLE_INT",		0,	26,	WRI,	0},		/* CHAIN0_ENABLE_INT */
	{"CHAIN0_STATUS_INT",		0,	26,	RO,	0},		/* CHAIN0_STATUS_INT */
	{"CHAIN0_CLEAR_INT",		0,	26,	WO,	0},		/* CHAIN0_CLEAR_INT */
};
#endif

enum fimc_is_vra_chain1_reg_name {
	VRA_R_CHAIN1_IMAGE_MODE,
	VRA_R_CHAIN1_CURRENT_INT,
	VRA_R_CHAIN1_ENABLE_INT,
	VRA_R_CHAIN1_STATUS_INT,
	VRA_R_CHAIN1_CLEAR_INT,
	VRA_CHAIN1_REG_CNT
};

static const struct fimc_is_reg vra_chain1_regs[VRA_CHAIN1_REG_CNT] = {
	{0x10A4, "CHAIN1_IMAGE_MDOE"},
	{0x1404, "CHAIN1_CURRENT_INT"},
	{0x1408, "CHAIN1_ENABLE_INT"},
	{0x140C, "CHAIN1_STATUS_INT"},
	{0x1410, "CHAIN1_CLEAR_INT"},
};

enum fimc_is_vra_chain1_reg_field {
	VRA_F_CHAIN1_IMAGE_MODE,
	VRA_F_CHAIN1_CURRENT_INT,
	VRA_F_CHAIN1_ENABLE_INT,
	VRA_F_CHAIN1_STATUS_INT,
	VRA_F_CHAIN1_CLEAR_INT,
	VRA_CHAIN1_REG_FIELD_CNT
};

static const struct fimc_is_field vra_chain1_fields[VRA_CHAIN1_REG_FIELD_CNT] = {
	/* 1. field_name 2. bit start 3. bit width 4. access type 5. reset */
	{"CHAIN1_IMAGE_MODE",		0,	1,	RW,	0},		/* CHAIN1_IMAGE_MODE */
	{"CHAIN1_CURRENT_INT",		0,	26,	RO,	0},		/* CHAIN1_CURRENT_INT */
	{"CHAIN1_ENABLE_INT",		0,	26,	WRI,	0},		/* CHAIN1_ENABLE_INT */
	{"CHAIN1_STATUS_INT",		0,	26,	RO,	0},		/* CHAIN1_STATUS_INT */
	{"CHAIN1_CLEAR_INT",		0,	26,	WO,	0},		/* CHAIN1_CLEAR_INT */
};

#endif

