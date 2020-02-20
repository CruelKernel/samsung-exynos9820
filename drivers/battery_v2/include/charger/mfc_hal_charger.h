/*
 * MFC_charger.h
 * Samsung MFC Charger Header
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __WIRELESS_CHARGER_MFC_HAL_H
#define __WIRELESS_CHARGER_MFC_HAL_H __FILE__

#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/alarmtimer.h>
#include "../sec_charging_common.h"

/* REGISTER MAPS */
#define MFC_HAL_CHIP_ID_L_REG					0x00
#define MFC_HAL_CHIP_ID_H_REG					0x01
#define MFC_HAL_FW_MAJOR_REV_L_REG				0x04
#define MFC_HAL_FW_MAJOR_REV_H_REG				0x05
#define MFC_HAL_FW_MINOR_REV_L_REG				0x06
#define MFC_HAL_FW_MINOR_REV_H_REG				0x07

#define MFC_CHIP_ID_P9320		0x20
#define MFC_CHIP_ID_S2MIW04		0x04

struct mfc_hal_charger_platform_data {
	int wpc_det;
	char *wired_charger_name;
};

#define mfc_hal_charger_platform_data_t \
	struct mfc_hal_charger_platform_data

struct mfc_charger_data {
	struct i2c_client				*client;
	struct device					*dev;
	mfc_hal_charger_platform_data_t 	*pdata;
	struct mutex io_lock;
};
#endif /* WIRELESS_CHARGER_MFC_HAL_H */
