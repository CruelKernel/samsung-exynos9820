/*
 * Copyright (C) 2012-2017 Samsung Electronics, Inc.
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

#ifndef __TZ_BOOST_H__
#define __TZ_BOOST_H__

#include <linux/errno.h>
#include <linux/pm_qos.h>

#ifdef CONFIG_TZDEV_BOOST
void tz_boost_enable(void);
void tz_boost_disable(void);
void tz_boost_set_boost_mask(unsigned int big_cpus_mask);
#else
static inline void tz_boost_enable(void)
{
}

static inline void tz_boost_disable(void)
{
}

static inline void tz_boost_set_boost_mask(unsigned int big_cpus_mask)
{
	(void) big_cpus_mask;
}
#endif /* CONFIG_TZDEV_BOOST */

#if defined(TZDEV_BOOST_CLUSTER_1)
/* cluster 1 is a high performance cluster */
#define TZ_BOOST_CPU_FREQ_MAX_DEFAULT_VALUE	PM_QOS_CLUSTER1_FREQ_MAX_DEFAULT_VALUE
#define TZ_BOOST_CPU_FREQ_MIN			PM_QOS_CLUSTER1_FREQ_MIN
#elif defined(TZDEV_BOOST_CLUSTER_2)
/* cluster 2 is a high performance cluster */
#define TZ_BOOST_CPU_FREQ_MAX_DEFAULT_VALUE	PM_QOS_CLUSTER2_FREQ_MAX_DEFAULT_VALUE
#define TZ_BOOST_CPU_FREQ_MIN			PM_QOS_CLUSTER2_FREQ_MIN
#endif /* TZDEV_BOOST_CLUSTER */

#endif /* __TZ_BOOST_H__ */
