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

#include <linux/kthread.h>
#include <linux/module.h>
#include "tz_iwsock.h"
#include "tz_ree_time.h"

MODULE_AUTHOR("Konstantin Karasev");
MODULE_DESCRIPTION("REE Time service");
MODULE_LICENSE("GPL");

#define ERR(...)		pr_alert("[ree_time] ERR : " __VA_ARGS__)

#define TZ_REE_TIME_SOCK_NAME	"ree_time_socket"

static struct task_struct *ree_time_kthread;

static int tz_ree_time_kthread(void *data)
{
	struct sock_desc *ree_time_listen;
	struct sock_desc *ree_time_conn;
	ssize_t len;
	int req;
	int ret;
	struct timespec ts;
	struct tz_ree_time ree_time;

	(void)data;

	/* Create socket */
	ree_time_listen = tz_iwsock_socket(1);
	if (IS_ERR(ree_time_listen))
		return PTR_ERR(ree_time_listen);

	ret = tz_iwsock_listen(ree_time_listen, TZ_REE_TIME_SOCK_NAME);
	if (ret)
		goto out;

	while (!kthread_should_stop()) {
		/* Accept connection */
		ree_time_conn = tz_iwsock_accept(ree_time_listen);
		if (IS_ERR(ree_time_conn)) {
			ret = PTR_ERR(ree_time_conn);
			goto out;
		}

		/* Process REE time requests from SWd */
		while (!kthread_should_stop()) {
			if ((len = tz_iwsock_read(ree_time_conn, &req,
							sizeof(req), 0))
					!= sizeof(req)) {
				ERR("failed to fetch request, err = %zd\n", len);
				break;
			}

			getnstimeofday(&ts);
			ree_time.sec = ts.tv_sec;
			ree_time.nsec = (unsigned int)ts.tv_nsec;
			if ((len = tz_iwsock_write(ree_time_conn, &ree_time,
							sizeof(ree_time), 0))
					!= sizeof(ree_time)) {
				ERR("failed to send time, err = %zd\n", len);
				break;
			}
		}
		tz_iwsock_release(ree_time_conn);
	}
out:
	tz_iwsock_release(ree_time_listen);

	return ret;
}

static int __init tz_ree_time_init(void)
{
	ree_time_kthread = kthread_run(tz_ree_time_kthread, NULL, "ree_time");
	if (IS_ERR(ree_time_kthread))
		return PTR_ERR(ree_time_kthread);

	return 0;
}

static void __init tz_ree_time_fini(void)
{
	kthread_stop(ree_time_kthread);
}

module_init(tz_ree_time_init);
module_exit(tz_ree_time_fini);
