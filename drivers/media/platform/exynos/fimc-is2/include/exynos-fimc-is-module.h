/*
 * /include/media/exynos_fimc_is_sensor.h
 *
 * Copyright (C) 2012 Samsung Electronics, Co. Ltd
 *
 * Exynos series exynos_fimc_is_sensor device support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MEDIA_EXYNOS_MODULE_H
#define MEDIA_EXYNOS_MODULE_H

#include <linux/platform_device.h>
#include <dt-bindings/camera/fimc_is.h>

#include "fimc-is-device-sensor.h"

#define MAX_SENSOR_SHARED_RSC	10

enum sensor_shared_rsc_pin_t{
	SHARED_PIN0 = 0,
	SHARED_PIN1,
	SHARED_PIN2,
	SHARED_PIN3,
	SHARED_PIN4,
	SHARED_PIN5,
	SHARED_PIN6,
	SHARED_PIN7,
	SHARED_PIN8,
	SHARED_PIN9,
	SHARED_PIN_MAX,
};

enum shared_rsc_type_t {
	SRT_ACQUIRE = 1,
	SRT_RELEASE,
};

struct exynos_sensor_pin {
	ulong pin;
	u32 delay;
	u32 value;
	char *name;
	u32 act;
	u32 voltage;

	enum shared_rsc_type_t shared_rsc_type;
	spinlock_t *shared_rsc_slock;
	atomic_t *shared_rsc_count;
	int shared_rsc_active;
};

#define SET_PIN_INIT(d, s, c) d->pinctrl_index[s][c] = 0;

#define SET_PIN(d, s, c, p, n, a, v, t)							\
	do {										\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].pin	= p;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].name	= n;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].act	= a;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].value	= v;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].delay	= t;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].voltage	= 0;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_type  = 0;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_slock = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_count = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_active = 0;	\
		(d)->pinctrl_index[s][c]++;						\
	} while (0)

#define SET_PIN_VOLTAGE(d, s, c, p, n, a, v, t, e)					\
	do {										\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].pin	= p;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].name	= n;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].act	= a;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].value	= v;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].delay	= t;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].voltage	= e;		\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_type  = 0;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_slock = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_count = NULL;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c]].shared_rsc_active = 0;	\
		(d)->pinctrl_index[s][c]++;						\
	} while (0)

#define SET_PIN_SHARED(d, s, c, type, slock, count, active)					\
	do {											\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_type  = type;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_slock = slock;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_count = count;	\
		(d)->pin_ctrls[s][c][d->pinctrl_index[s][c] - 1].shared_rsc_active = active;	\
	} while (0)

struct exynos_platform_fimc_is_module {
	int (*gpio_cfg)(struct fimc_is_module_enum *module, u32 scenario, u32 gpio_scenario);
	int (*gpio_dbg)(struct fimc_is_module_enum *module, u32 scenario, u32 gpio_scenario);
	struct exynos_sensor_pin pin_ctrls[SENSOR_SCENARIO_MAX][GPIO_SCENARIO_MAX][GPIO_CTRL_MAX];
	u32 pinctrl_index[SENSOR_SCENARIO_MAX][GPIO_SCENARIO_MAX];
	struct pinctrl *pinctrl;
	u32 position;
	u32 mclk_ch;
	u32 id;
	u32 sensor_i2c_ch;
	u32 sensor_i2c_addr;
	u32 is_bns;
	u32 flash_product_name;
	u32 flash_first_gpio;
	u32 flash_second_gpio;
	u32 af_product_name;
	u32 af_i2c_addr;
	u32 af_i2c_ch;
	u32 aperture_product_name;
	u32 aperture_i2c_addr;
	u32 aperture_i2c_ch;
	u32 ois_product_name;
	u32 ois_i2c_addr;
	u32 ois_i2c_ch;
	u32 mcu_product_name;
	u32 mcu_i2c_addr;
	u32 mcu_i2c_ch;
	u32 rom_id;
	u32 rom_type;
	u32 rom_cal_index;
	u32 rom_dualcal_id;
	u32 rom_dualcal_index;
	u32 preprocessor_product_name;
	u32 preprocessor_spi_channel;
	u32 preprocessor_i2c_addr;
	u32 preprocessor_i2c_ch;
	u32 preprocessor_dma_channel;
	bool power_seq_dt;
};

extern int exynos_fimc_is_module_pins_cfg(struct fimc_is_module_enum *module,
	u32 snenario,
	u32 gpio_scenario);
extern int exynos_fimc_is_module_pins_dbg(struct fimc_is_module_enum *module,
	u32 scenario,
	u32 gpio_scenario);

#endif /* MEDIA_EXYNOS_MODULE_H */
