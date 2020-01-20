/*
 * maxim_dsm_cal.c -- Module for Rdc calibration
 *
 * Copyright 2014 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOUND_MAXIM_DSM_CAL_H__
#define __SOUND_MAXIM_DSM_CAL_H__

#define DRIVER_AUTHOR		"Kyounghun Jeon<hun.jeon@maximintegrated.com>"
#define DRIVER_DESC			"For Rdc calibration of MAX98xxx"
#define DRIVER_SUPPORTED	"MAX98xxx"

#define WQ_NAME				"maxdsm_wq"

#define FILEPATH_TEMP_CAL	"/efs/maxim/temp_cal"
#define FILEPATH_RDC_CAL	"/efs/maxim/rdc_cal"
#define FILEPATH_RDC_CAL_R	"/efs/maxim/rdc_cal_r"

#define CLASS_NAME			"maxdsm_cal"
#define DSM_NAME			"dsm"

#define ADDR_RDC			0x2A0050
#define ADDR_FEATURE_ENABLE 0x2A006A

struct maxim_dsm_cal_info {
	uint32_t min;
	uint32_t max;
	uint32_t feature_en;
	int interval;
	int duration;
	int remaining;
	int ignored_t;
	unsigned long previous_jiffies;
};

struct maxim_dsm_cal_values {
	uint32_t status;
	int rdc;
	int rdc_r;
	int temp;
	uint64_t avg;
	uint64_t avg_r;
	int count;
};

struct maxim_dsm_cal {
	struct device *dev;
	struct class *class;
	struct mutex mutex;
	struct workqueue_struct *wq;
	struct delayed_work work;
	struct maxim_dsm_cal_values values;
	struct maxim_dsm_cal_info info;
	struct regmap *regmap;
	uint32_t platform_type;
};

extern struct class *maxdsm_cal_get_class(void);
extern struct regmap *maxdsm_cal_set_regmap(
		struct regmap *regmap);
extern int maxdsm_cal_get_temp(int *temp);
extern int maxdsm_cal_set_temp(int temp);
extern int maxdsm_cal_get_rdc(int *rdc);
extern int maxdsm_cal_get_rdc_r(int *rdc);
extern int maxdsm_cal_set_rdc(int rdc);
extern int maxdsm_cal_set_rdc_r(int rdc);
#endif /* __SOUND_MAXIM_DSM_CAL_H__ */
