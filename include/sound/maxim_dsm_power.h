/*
 * maxim_dsm_power.c -- Module for Power Measurement
 *
 * Copyright 2015 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOUND_MAXIM_DSM_POWER_H__

#define MAXDSM_POWER_WQ_NAME	"maxdsm_power_wq"
#define MAXDSM_POWER_CLASS_NAME "maxdsm_power"
#define MAXDSM_POWER_DSM_NAME	"dsm"

#define MAXDSM_POWER_START_DELAY 1000

struct maxim_dsm_power_info {
	int interval;
	int duration;
	int remaining;
	unsigned long previous_jiffies;
};

struct maxim_dsm_power_values {
	uint32_t status;
	int power;
	uint64_t avg;
	int count;
};

struct maxim_dsm_power {
	struct device *dev;
	struct class *class;
	struct mutex mutex;
	struct workqueue_struct *wq;
	struct delayed_work work;
	struct maxim_dsm_power_values values;
	struct maxim_dsm_power_info info;
	uint32_t platform_type;
};

#endif /* __SOUND_MAXIM_DSM_POWER_H__ */
