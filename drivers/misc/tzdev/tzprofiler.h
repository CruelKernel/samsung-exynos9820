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

#ifndef _TZPROFILER_H_
#define _TZPROFILER_H_

#include "tz_cred.h"
#include "tz_iwcbuf.h"
#include "tzdev.h"

#define TZPROFILER_NAME			"tzprofiler"

#define TZDEV_PROFILER_BUF_SIZE	(CONFIG_TZPROFILER_BUF_PG_CNT * PAGE_SIZE - sizeof(struct tz_iwcbuf))

struct profiler_buf_entry {
	struct list_head list;
	struct tz_iwcbuf *tzio_buf;
};

#define TZPROFILER_IOC_MAGIC		'p'
#define TZPROFILER_INCREASE_POOL	_IOW(TZPROFILER_IOC_MAGIC, 0, uint32_t)
#define TZPROFILER_START		_IO(TZPROFILER_IOC_MAGIC, 1)
#define TZPROFILER_STOP			_IO(TZPROFILER_IOC_MAGIC, 2)
#define TZPROFILER_SET_DEPTH		_IOW(TZPROFILER_IOC_MAGIC, 3, uint32_t)
#define TZPROFILER_SET_UUID		_IOW(TZPROFILER_IOC_MAGIC, 4, struct tz_uuid)
#define TZPROFILER_SET_START_ADDR	_IOW(TZPROFILER_IOC_MAGIC, 5, uint64_t)
#define TZPROFILER_SET_STOP_ADDR	_IOW(TZPROFILER_IOC_MAGIC, 6, uint64_t)
#define TZPROFILER_SET_STEPS_NUMBER	_IOW(TZPROFILER_IOC_MAGIC, 7, uint32_t)

#if defined(CONFIG_TZPROFILER)
int tzprofiler_initialize(void);
void tzprofiler_enter_sw(void);
void tzprofiler_exit_sw(void);
void tzprofiler_wait_for_bufs(void);

#else /* CONFIG_TZPROFILER */
static inline int tzprofiler_initialize(void)
{
	return 0;
}

static inline void tzprofiler_enter_sw(void)
{
}

static inline void tzprofiler_exit_sw(void)
{
}

static inline void tzprofiler_wait_for_bufs(void)
{
}

#endif /* CONFIG_TZPROFILER */
#endif /* _TZPROFILER_H_ */
