/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef FIMC_IS_COMPANION_H
#define FIMC_IS_COMPANION_H

#include <linux/vmalloc.h>
#include <linux/firmware.h>

#include "fimc-is-config.h"

#define CRC_RETRY_COUNT         40

#define FIMC_IS_ISP_CV	"/data/vendor/camera/ISP_CV"
#define USE_SPI

#ifdef CONFIG_COMPANION_C3_USE
#define FIMC_IS_COMPANION_VERSION_EVT0 0x00A0
#define FIMC_IS_COMPANION_VERSION_EVT1 0x00A1
#define COMP_DEFAULT_VOUT_VAL 825000
#define COMP_DEFAULT_VOUT_STR "0.825"
#else
#define FIMC_IS_COMPANION_VERSION_EVT0 0x00A0
#define FIMC_IS_COMPANION_VERSION_EVT1 0x00B0
#define COMP_DEFAULT_VOUT_VAL 850000
#define COMP_DEFAULT_VOUT_STR "0.850"
#endif

#ifdef CONFIG_COMPANION_DCDC_USE
enum dcdc_vendor{
	DCDC_VENDOR_NONE = 0,
	DCDC_VENDOR_FAN53555,
	DCDC_VENDOR_NCP6335B,
	DCDC_VENDOR_PMIC,
	DCDC_VENDOR_MAX,
};

struct dcdc_power {
	enum dcdc_vendor type;
	struct i2c_client *client;
	char *name;	/* DCDC name */

	/* Get default vout value and string */
	int (*get_default_vout_val)(int*, const char **);

	/* Get i2c register value to set vout of dcdc regulator */
	int (*get_vout_val)(int);

	/* Get voltage name of vout as string */
	const char *(*get_vout_str)(int);

	/* Set dcdc vout with i2c register value */
	int (*set_vout)(struct i2c_client *client, int vout);
};
#endif

int fimc_is_comp_is_valid(void *core_data);
int fimc_is_comp_loadfirm(void *core_data);
#ifdef CONFIG_PREPROCESSOR_STANDBY_USE
int fimc_is_comp_retention(void *core_data);
#endif
int fimc_is_comp_loadsetf(void *core_data);
int fimc_is_comp_loadcal(void *core_data, int position);
u8 fimc_is_comp_is_compare_ver(void *core_data);
#ifdef CONFIG_COMPANION_DCDC_USE
int fimc_is_power_binning(void *core_data);
#endif
void fimc_is_s_int_comb_isp(void *core_data, bool on, u32 ch);
u16 fimc_is_comp_get_ver(void);
#ifndef CONFIG_COMPANION_DCDC_USE
int fimc_is_comp_set_voltage(char *, int);
#endif
int fimc_is_comp_i2c_read(struct i2c_client *client, u16 addr, u16 *data);
int fimc_is_comp_i2c_write(struct i2c_client *client, u16 addr, u16 data);
#endif /* FIMC_IS_COMPANION_H */
