/*
 * Copyright (C) 2016 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/mutex.h>
#include <linux/pm_qos.h>

#include "tz_boost.h"
#include "tz_hotplug.h"

static int tz_boost_users = 0;
static struct pm_qos_request tz_boost_qos;
static unsigned int cpu_boost_mask;
static DEFINE_MUTEX(tz_boost_lock);

void tz_boost_set_boost_mask(unsigned int big_cpus_mask)
{
	cpu_boost_mask = big_cpus_mask;
}

void tz_boost_enable(void)
{
	mutex_lock(&tz_boost_lock);

	if (!tz_boost_users) {
		tz_hotplug_update_nwd_cpu_mask(cpu_boost_mask);
		pm_qos_add_request(&tz_boost_qos, TZ_BOOST_CPU_FREQ_MIN,
				TZ_BOOST_CPU_FREQ_MAX_DEFAULT_VALUE);
	}

	tz_boost_users++;

	mutex_unlock(&tz_boost_lock);
}

void tz_boost_disable(void)
{
	mutex_lock(&tz_boost_lock);

	tz_boost_users--;
	BUG_ON(tz_boost_users < 0);

	if (!tz_boost_users) {
		pm_qos_remove_request(&tz_boost_qos);
		tz_hotplug_update_nwd_cpu_mask(0x0);
	}

	mutex_unlock(&tz_boost_lock);
}
