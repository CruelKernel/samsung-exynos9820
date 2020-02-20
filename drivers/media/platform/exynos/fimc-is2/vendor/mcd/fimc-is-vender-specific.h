/*
* Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is vender functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_VENDER_SPECIFIC_H
#define FIMC_IS_VENDER_SPECIFIC_H

#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>

#ifdef CONFIG_COMPANION_DCDC_USE
#include "fimc-is-pmic.h"
#endif

#if !defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
typedef enum FRomPowersource{
    FROM_POWER_SOURCE_REAR	= 0,  /*wide*/
    FROM_POWER_SOURCE_REAR_SECOND  /*tele*/
} FRomPowersource;
#endif

/* #define USE_ION_ALLOC */
#define FIMC_IS_COMPANION_CRC_SIZE	4
#ifdef CONFIG_COMPANION_C3_USE
#define FIMC_IS_COMPANION_CRC_OBJECT	4
#else
#define FIMC_IS_COMPANION_CRC_OBJECT	6
#endif
#define I2C_RETRY_COUNT			5

#ifdef CONFIG_PREPROCESSOR_STANDBY_USE
#define SENSOR_STATE_OFF		0x0
#define SENSOR_STATE_ON			0x1
#define SENSOR_STATE_STANDBY		0x2
#define SENSOR_STATE_UNKNOWN		0x3
#define SENSOR_MASK			0x3
#define SENSOR_STATE_SENSOR		0
#define SENSOR_STATE_COMPANION		2

#define SET_SENSOR_STATE(state, sensor, value) \
	(state = (state & ~(SENSOR_MASK << sensor)) | value << sensor)
#define GET_SENSOR_STATE(state, sensor) \
	(((state & (SENSOR_MASK << sensor)) >> sensor) & SENSOR_MASK)
#endif

struct fimc_is_companion_retention {
	int firmware_size;
	char firmware_crc32[FIMC_IS_COMPANION_CRC_SIZE];
};

struct fimc_is_vender_specific {
#ifdef USE_ION_ALLOC
	struct ion_client	*fimc_ion_client;
#endif
	struct mutex		rom_lock;
	struct mutex		hw_init_lock;
#ifdef CONFIG_OIS_USE
	bool			ois_ver_read;
#endif /* CONFIG_OIS_USE */

	struct i2c_client	*eeprom_client[ROM_ID_MAX];
	bool		rom_valid[ROM_ID_MAX];

	/* dt */
	u32			rear_sensor_id;
	u32			front_sensor_id;
	u32			rear2_sensor_id;
	u32			front2_sensor_id;
	u32			rear3_sensor_id;
	u32			rear_tof_sensor_id;
	u32			front_tof_sensor_id;
#ifdef SECURE_CAMERA_IRIS
	u32			secure_sensor_id;
#endif
	u32			ois_sensor_index;
	u32			mcu_sensor_index;
	u32			aperture_sensor_index;
	bool			check_sensor_vendor;
	bool			use_ois_hsi2c;
	bool			use_ois;
	bool			use_module_check;

	bool			suspend_resume_disable;
	bool			need_cold_reset;
	bool			zoom_running;
	int32_t			rear_tof_uid[TOF_CAL_UID_MAX];
	int32_t			front_tof_uid[TOF_CAL_UID_MAX];

#if !defined(CONFIG_CAMERA_EEPROM_SUPPORT_REAR)
	FRomPowersource		f_rom_power;
#endif
};

#endif
