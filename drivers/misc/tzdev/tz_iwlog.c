/*
 * Copyright (C) 2013-2016 Samsung Electronics, Inc.
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

#include <linux/atomic.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#include "tz_iwcbuf.h"
#include "tz_iwio.h"
#include "tz_iwlog.h"
#include "tzdev.h"

#define TZ_IWLOG_BUF_SIZE		(CONFIG_TZLOG_PG_CNT * PAGE_SIZE - sizeof(struct tz_iwcbuf))

#define TZ_IWLOG_LINE_MAX_LEN		256
#define TZ_IWLOG_PREFIX			KERN_DEFAULT "SW> "

enum {
	TZ_IWLOG_WAIT,
	TZ_IWLOG_BUSY
};

struct tz_iwlog_print_state {
	char line[TZ_IWLOG_LINE_MAX_LEN + 1];	/* one byte for \0 */
	unsigned int line_len;
};

static DEFINE_PER_CPU(struct tz_iwlog_print_state, tz_iwlog_print_state);
static DEFINE_PER_CPU(struct tz_iwcbuf *, tz_iwlog_buffer);

static DECLARE_WAIT_QUEUE_HEAD(tz_iwlog_wq);
static atomic_t tz_iwlog_request = ATOMIC_INIT(0);

static void tz_iwlog_buffer_print(const char *buf, unsigned int count)
{
	struct tz_iwlog_print_state *ps;
	unsigned int avail, bytes_in, bytes_out, bytes_printed, tmp, wait_dta;
	char *p;

	ps = &get_cpu_var(tz_iwlog_print_state);

	wait_dta = 0;
	while (count) {
		avail = TZ_IWLOG_LINE_MAX_LEN - ps->line_len;

		p = memchr(buf, '\n', count);

		if (p) {
			if (p - buf > avail) {
				bytes_in = avail;
				bytes_out = avail;
			} else {
				bytes_in = (unsigned int)(p - buf + 1);
				bytes_out = (unsigned int)(p - buf);
			}
		} else {
			if (count >= avail) {
				bytes_in = avail;
				bytes_out = avail;
			} else {
				bytes_in = count;
				bytes_out = count;

				wait_dta = 1;
			}
		}

		memcpy(&ps->line[ps->line_len], buf, bytes_out);
		ps->line_len += bytes_out;

		if (wait_dta)
			break;

		ps->line[ps->line_len] = 0;

		bytes_printed = 0;
		while (bytes_printed < ps->line_len) {
			tmp = printk(TZ_IWLOG_PREFIX "%s\n", &ps->line[bytes_printed]);
			if (!tmp)
				break;

			bytes_printed += tmp;
		}

		ps->line_len = 0;

		count -= bytes_in;
		buf += bytes_in;
	}

	put_cpu_var(tz_iwlog_print_state);
}

static void tz_iwlog_read_buffers_do(void)
{
	struct tz_iwcbuf *buf;
	unsigned int i, write_count;
	unsigned long count;

	for_each_possible_cpu(i) {
		buf = per_cpu(tz_iwlog_buffer, i);
		if (!buf)
			continue;

		write_count = buf->write_count;
		if (write_count < buf->read_count) {
			count = TZ_IWLOG_BUF_SIZE - buf->read_count;

			tz_iwlog_buffer_print(buf->buffer + buf->read_count, count);
			buf->read_count = 0;
		}

		count = write_count - buf->read_count;

		tz_iwlog_buffer_print(buf->buffer + buf->read_count, count);
		buf->read_count += count;
	}
}

static int tz_iwlog_kthread(void *data)
{
	(void)data;

	while (1) {
		if (!wait_event_interruptible(tz_iwlog_wq,
				atomic_xchg(&tz_iwlog_request, TZ_IWLOG_WAIT) == TZ_IWLOG_BUSY))
			tz_iwlog_read_buffers_do();
	}

	return 0;
}

void tz_iwlog_read_buffers(void)
{
	if (atomic_cmpxchg(&tz_iwlog_request, TZ_IWLOG_WAIT, TZ_IWLOG_BUSY) == TZ_IWLOG_WAIT)
		wake_up(&tz_iwlog_wq);
}

int tz_iwlog_initialize(void)
{
	struct task_struct *th;
	unsigned int i;

	for (i = 0; i < NR_SW_CPU_IDS; ++i) {
		struct tz_iwcbuf *buffer = tz_iwio_alloc_iw_channel(
				TZ_IWIO_CONNECT_LOG, CONFIG_TZLOG_PG_CNT,
				NULL, NULL, NULL);
		if (IS_ERR(buffer))
			return PTR_ERR(buffer);

		if (cpu_possible(i))
			per_cpu(tz_iwlog_buffer, i) = buffer;
	}

	th = kthread_run(tz_iwlog_kthread, NULL, "tzlog");
	if (IS_ERR(th))
		return PTR_ERR(th);

	return 0;
}
