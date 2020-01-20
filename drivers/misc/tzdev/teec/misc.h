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

#ifndef __TZDEV_TEEC_MISC_H__
#define __TZDEV_TEEC_MISC_H__

#include <linux/types.h>

#include "iw_messages.h"
#include "tz_iwsock.h"

#define PARAM_TYPE_GET(types, nr)	(((types) >> ((nr) * 4)) & 0xF)
#define PARAM_TYPE_SET(types, nr)	(((types) & 0xF) << ((nr) * 4))

#define MAX_PARAM_COUNT				4

int tzdev_teec_connect(struct sock_desc *socket, char *name,
				uint32_t *result, uint32_t *origin);
void tzdev_teec_disconnect(struct sock_desc *socket);

int tzdev_teec_send(struct sock_desc *socket, void *data, uint32_t size, int flags,
		uint32_t *result, uint32_t *origin);
int tzdev_teec_recv(struct sock_desc *socket, void *data, uint32_t size, int flags,
		uint32_t *result, uint32_t *origin);
int tzdev_teec_send_then_recv(struct sock_desc *socket,
		void *send_data, uint32_t send_size, int send_flags,
		void *recv_data, uint32_t recv_size, int recv_flags,
		uint32_t *result, uint32_t *origin);

int tzdev_teec_check_reply(struct cmd_reply *reply, uint32_t cmd,
		uint32_t serial, uint32_t *result, uint32_t *origin);

uint32_t tzdev_teec_error_to_tee_error(int error);
void tzdev_teec_fixup_origin(uint32_t result, uint32_t *origin);

#endif /* __TZDEV_TEEC_MISC_H__ */
