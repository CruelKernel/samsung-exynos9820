/*
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * This code is originated from Samsung Electronics proprietary sources.
 * Author: Oleksandr Fadieiev (o.fadieiev@samsung.com)
 * Author: Iaroslav Makarchuk (i.makarchuk@samsung.com)
 * Created: 03 Oct, 2016
 * Modified: Dec 27, 2018
 *
 * Copyright (C) 2015 Samsung Electronics, Inc.
 */

#include "socket_utils.h"
#include "protocol.h"

#include <linux/delay.h>
#include <linux/types.h>
#include <net/sock.h>

#define SOCKET_RETRY_CNT    5
#define SOCKET_RETRY_DELAY  1

SocketStatus ReadDataFromSocket(struct socket *socket, uint8_t *data,
					uint32_t data_len)
{
	SocketStatus status = SocketStatusError;
	int read_res = 0;
	uint32_t curr_len = 0;
	struct msghdr msg;

	memset(&msg, 0, sizeof(msg));
	msg.msg_flags = MSG_DONTWAIT;

	do {
		struct kvec vec;

		vec.iov_base = data + curr_len;
		vec.iov_len = data_len - curr_len;

		read_res = kernel_recvmsg(socket, &msg, &vec, 1, vec.iov_len,
					  MSG_DONTWAIT);
		if (read_res == -EAGAIN || read_res == -ERESTARTSYS)
			continue;

		if (read_res <= 0)
			break;

		curr_len += read_res;
	} while (curr_len < data_len);

	if (curr_len == data_len)
		status = SocketStatusOk;

	return status;
}

SocketStatus WriteDataToSocket(struct socket *socket, uint8_t *data,
					uint32_t data_len)
{
	SocketStatus status = SocketStatusError;
	uint32_t curr_len = 0;
	int write_res = 0;
	struct msghdr msg;

	memset(&msg, 0, sizeof(msg));
	msg.msg_flags = MSG_DONTWAIT;

	while (curr_len < data_len) {
		struct kvec vec;

		vec.iov_base = data + curr_len;
		vec.iov_len = data_len - curr_len;

		write_res = kernel_sendmsg(socket, &msg, &vec, 1, vec.iov_len);

		if (write_res == -EAGAIN || write_res == -ERESTARTSYS)
			continue;

		if (write_res <= 0) {
			pr_err("FIVE: write error: %d\n", write_res);
			break;
		}

		curr_len += write_res;
	}

	if (curr_len == data_len)
		status = SocketStatusOk;

	return status;
}

SocketStatus OpenSocket(struct sockaddr *sa, struct socket **socket)
{
	int retry_cnt = SOCKET_RETRY_CNT;
	SocketStatus status = SocketStatusError;

	BUG_ON(!sa || !socket);

	/*
	 * Connect to proxy between kernel and emulator.
	 * The FIVE trustlet emulator and FIVE proxy are supposed to be
	 * launched before the first user request to the kernel.
	 * User (sign/verify) -> FIVE driver (TEEC protocol request) -> TCP ->
	 *                       FIVE proxy (raw data) -> Socket ->
	 *                       FIVE emulator (TEEC protocol response)
	 */
	if (sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, socket) < 0) {
		pr_err("FIVE: error: while creating TCP socket.\n");
		return status;
	}

	while (retry_cnt--) {
		if (kernel_connect(*socket, sa, sizeof(*sa), 0) == 0) {
			status = SocketStatusOk;
			break;
		}

		mdelay(500);
	}

	if (SocketStatusOk != status) {
		sock_release(*socket);
		status = SocketStatusError;
	}

	return status;
}
