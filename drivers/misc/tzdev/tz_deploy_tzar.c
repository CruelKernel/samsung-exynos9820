/*
 * Copyright (C) 2012-2018 Samsung Electronics, Inc.
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

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include "tz_common.h"
#include "tz_deploy_tzar.h"
#include "tz_iwsock.h"
#include "tz_mem.h"
#include "tzlog.h"
#include "teec/iw_messages.h"

#define MAX_LOADING_ATTEMPTS 10
#define MAX_CONNECTION_ATTEMPTS 20
#define CONNECTION_ATTEMPT_TIMEOUT 100

__asm__ (
  ".section .init.data,\"aw\"\n"
  "tzdev_tzar_begin:\n"
  ".incbin \"startup.tzar\"\n"
  "tzdev_tzar_end:\n"
  ".previous\n"
);
extern char tzdev_tzar_begin[], tzdev_tzar_end[];

enum iw_startup_loader_cmd_type {
	CMD_STARTUP_LOADER_INVALID_ID,
	CMD_STARTUP_LOADER_UPLOAD_CONTAINER,
	CMD_STARTUP_LOADER_REPLY,
	CMD_STARTUP_LOADER_REPLY_UPLOAD_CONTAINER,
	CMD_STARTUP_LOADER_MAX
};

struct cmd_startup_loader_upload_container {
	struct cmd_request base;
	struct shared_buffer_description buf_desc;
} IW_STRUCTURE;

struct cmd_startup_loader_reply_upload_container {
	struct cmd_reply base;
} IW_STRUCTURE;

static __ref int tz_deploy_handler(void *arg)
{
	int ret, retry_cnt;
	size_t size;
	struct sock_desc *sd;
	struct cmd_startup_loader_upload_container command;
	struct cmd_startup_loader_reply_upload_container reply;
	void *tzar;
	unsigned int attempt;
	(void)arg;

	/* initialize return value and reply structure */
	ret = -1;
	reply.base.result = -1;

	size = tzdev_tzar_end - tzdev_tzar_begin;
	tzdev_print(0, "[debug] tzar size = %zu\n", size);

	for (retry_cnt = 0; retry_cnt < MAX_LOADING_ATTEMPTS; retry_cnt++) {
		tzar = vmalloc(size);
		if (!tzar) {
			tzdev_print(0, "vmalloc fail\n");
			ret = -ENOMEM;
			continue;
		}

		memcpy(tzar, tzdev_tzar_begin, size);

		size = DIV_ROUND_UP(size, PAGE_SIZE) * PAGE_SIZE;
		ret = tzdev_mem_register(tzar, size, 0, NULL, NULL);
		if (ret < 0) {
			tzdev_print(0, "tzdev_mem_register fail, ret=%d\n", ret);
			goto out_tzar;
		}

		command.base.cmd = CMD_STARTUP_LOADER_UPLOAD_CONTAINER;
		command.buf_desc.id = ret;
		command.buf_desc.size = (unsigned int)size;

		sd = tz_iwsock_socket(1);
		if (IS_ERR(sd)) {
			ret = PTR_ERR(sd);
			tzdev_print(0, "tz_iwsock_socket fail, ret=%d\n", ret);
			goto out_mem;
		}

		for (attempt = 0; attempt < MAX_CONNECTION_ATTEMPTS; attempt++) {
			ret = tz_iwsock_connect(sd, STARTUP_LOADER_SOCK, 0);
			if (!ret)
				break;

			tzdev_print(0, "Failed to connect to startup loader socket, "
				"error = %d, retrying...\n",
				ret);

			msleep(CONNECTION_ATTEMPT_TIMEOUT);
		}
		if (ret < 0) {
			tzdev_print(0, "tz_iwsock_connect fail, ret=%d\n", ret);
			goto out_sock;
		}

		ret = tz_iwsock_write(sd, &command, sizeof(command), 0);
		if (ret != sizeof(command)) {
			tzdev_print(0, "tz_iwsock_write fail, ret=%d\n", ret);
			goto out_sock;
		}

		ret = tz_iwsock_read(sd, &reply, sizeof(reply), 0);
		if (ret < 0 || reply.base.result != 0) {
			tzdev_print(0, "startup load result, ret=%d\n", reply.base.result);
			tzdev_print(0, "tz_iwsock_read fail, ret=%d\n", ret);
			goto out_sock;
		}

		ret = 0;
		tzdev_print(0, "[debug] Startuploader complete\n");
		break;

		out_sock:
			tz_iwsock_release(sd);
		out_mem:
			tzdev_mem_release(command.buf_desc.id);
		out_tzar:
			vfree(tzar);

		msleep(CONNECTION_ATTEMPT_TIMEOUT);
	}
	if (reply.base.result)
		tzdev_print(0, "tzdev: iwsock error. result=%d\n", reply.base.result);

	if (reply.base.base.cmd != CMD_STARTUP_LOADER_REPLY_UPLOAD_CONTAINER)
		tzdev_print(0, "tzdev: iwsock wrong cmd. cmd=%u\n", reply.base.base.cmd);

	if (ret)
		tzdev_print(0, "tzdev: Startuploader failed\n");
	else {
		tz_iwsock_release(sd);
		tzdev_mem_release(command.buf_desc.id);
		vfree(tzar);
	}

	return ret;
}

int tzdev_deploy_tzar(void)
{
	struct task_struct *t;

	t = kthread_run(tz_deploy_handler, NULL, "tzdev_deploy_tzar");
	if (IS_ERR(t)) {
		printk("Can't start startup.tzar deployment thread\n");
		return PTR_ERR(t);
	}
	return 0;
}
