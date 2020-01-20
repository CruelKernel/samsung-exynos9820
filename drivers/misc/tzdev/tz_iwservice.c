/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
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

#include <linux/errno.h>
#include <linux/mm.h>

#include "tz_hotplug.h"
#include "tz_iwio.h"
#include "tz_kthread_pool.h"
#include "tzdev.h"
#include "tzlog.h"

struct iw_service_channel {
	uint32_t cpu_mask;
	uint32_t user_cpu_mask;
} __attribute__((__packed__));

static struct iw_service_channel *iw_channel;

int tz_iwservice_alloc(void)
{
	struct iw_service_channel *ch;

	ch = tz_iwio_alloc_iw_channel(TZ_IWIO_CONNECT_SERVICE, 1, NULL, NULL, NULL);
	if (!ch)
		return -ENOMEM;

	iw_channel = ch;

	return 0;
}

unsigned long tz_iwservice_get_cpu_mask(void)
{
	if (!iw_channel)
		return 0;

	return (iw_channel->cpu_mask | iw_channel->user_cpu_mask);
}

unsigned long tz_iwservice_get_user_cpu_mask(void)
{
	if (!iw_channel)
		return 0;

	return iw_channel->user_cpu_mask;
}

static void tz_iwservice_notify_swd_cpu_mask_update(void)
{
	cpumask_t new_cpus_mask;
	unsigned long sk_cpu_mask;

	/* Check cores activation */
	sk_cpu_mask = tz_iwservice_get_cpu_mask();
	cpumask_andnot(&new_cpus_mask, to_cpumask(&sk_cpu_mask), cpu_online_mask);

	if (!cpumask_empty(&new_cpus_mask))
		tz_hotplug_notify_swd_cpu_mask_update();

	if (sk_cpu_mask)
		tz_kthread_pool_wake_up_all();
}

void tz_iwservice_handle_swd_request(void)
{
	if (!iw_channel)
		return;

	tz_iwservice_notify_swd_cpu_mask_update();
}
