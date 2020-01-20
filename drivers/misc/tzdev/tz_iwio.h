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

#ifndef __TZ_IWIO_H__
#define __TZ_IWIO_H__

enum {
	TZ_IWIO_CONNECT_LOG,
	TZ_IWIO_CONNECT_SERVICE,
	TZ_IWIO_CONNECT_TRANSPORT,
	TZ_IWIO_CONNECT_PROFILER,
	TZ_IWIO_CONNECT_PANIC_DUMP,
	TZ_IWIO_CONNECT_SOCKET_EVENTS,
	TZ_IWIO_CONNECT_SOCKET,
	TZ_IWIO_CONNECT_NWFS,
	TZ_IWIO_CONNECT_CNT
};

#define TZ_IWIO_AUX_BUF_SIZE PAGE_SIZE

struct tz_iwio_aux_channel {
	char buffer[TZ_IWIO_AUX_BUF_SIZE];
} __packed;

typedef int (*tz_iwio_callback_t)(void *buf, unsigned long num_pages, void *ext_data);
typedef void (*tz_iwio_callback_cleanup_t)(void *buf,
		unsigned long num_pages, void *ext_data);

struct tz_iwio_aux_channel *tz_iwio_get_aux_channel(void);
void tz_iwio_put_aux_channel(void);
int tz_iwio_alloc_aux_channel(int cpu);
void *tz_iwio_alloc_iw_channel(unsigned int mode, unsigned int num_pages,
		tz_iwio_callback_t pre_callback, tz_iwio_callback_cleanup_t pre_callback_cleanup,
		void *calback_data);
void tz_iwio_free_iw_channel(void *ch);

#endif /* __TZ_IWIO_H__ */
