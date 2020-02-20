/* sec_nadbalancer.h
 *
 * NAD Balancer Driver
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Hyeokseon Yu <hyeokseon.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SEC_NAD_BALANCER_H
#define SEC_NAD_BALANCER_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sec_class.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/pm_qos.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/io.h>
#include <linux/alarmtimer.h>

#if defined(CONFIG_SEC_FACTORY)
#define BALANCER_CMD	6
#define NAD_BUFF_SIZE	256
#define MAX_TMU_COUNT	5

#define UPDATE_FIRST_TEMPERATURE	1
#define UPDATE_CONTI_TEMPERATURE	2
#define UPDATE_FINAL_TEMPERATURE	3

#define NAD_MIN_FREQ	0
#define NAD_MAX_FREQ	1

#define NAD_BALANCER_MODE_SINGLE	(1)
#define NAD_BALANCER_MODE_DUAL		(2)
#define NAD_BALANCER_MODE_QUAD		(4)

struct device *sec_nad_balancer;
static bool nad_balancer_enable;
static bool sleep_test_enable;

static DEFINE_MUTEX(nad_balancer_lock);

struct freq_table {
	unsigned int freq;
};

struct nad_balancer_pm_qos {
	const char *desc;
	struct task_struct *thread;
	struct freq_table *tables;
	int delay_time;
	int table_size;

	/* big turbo support - single / dual / quad */
	struct freq_table *single_tables;
	struct freq_table *dual_tables;
	struct freq_table *quad_tables;
	int current_mode;
	int big_turbo_enable;
	int single_table_size;
	int dual_table_size;
	int quad_table_size;

	int policy;

	struct pm_qos_request big_qos;
	struct pm_qos_request lit_qos;
	struct pm_qos_request mid_qos;
	struct pm_qos_request mif_qos;
	struct pm_qos_request int_qos;

	int temperature[MAX_TMU_COUNT];
};

struct nad_balancer_sleep_info {
	int suspend_threshold;
	int resume_threshold;
};

struct nad_balancer_platform_data {
	struct nad_balancer_pm_qos *qos_items;
	struct nad_balancer_sleep_info *sleep_items;

	unsigned int nItem;
	unsigned int nQos;

	unsigned int timeout;
	unsigned int nSleep;
};

struct nad_balancer_info {
	struct device *dev;
	struct nad_balancer_platform_data *pdata;
	struct delayed_work sec_nad_balancer_work;
};

static int sec_nad_balancer_probe(struct platform_device *pdev);
static int sec_nad_balancer_resume(struct device *dev);
static int sec_nad_balancer_remove(struct platform_device *pdev);
static int sec_nad_balancer_prepare(struct device *dev);

#define UPDATE_PM_QOS(req, class_id, arg) ({ \
if (arg) { \
	if (pm_qos_request_active(req)) \
		pm_qos_update_request(req, arg); \
	else \
		pm_qos_add_request(req, class_id, arg); \
} \
})

#define REMOVE_PM_QOS(req) ({ \
if (pm_qos_request_active(req)) \
	pm_qos_remove_request(req); \
})
#endif
#endif
