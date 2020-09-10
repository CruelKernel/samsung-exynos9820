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

#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include "protocol.h"

#include <linux/types.h>
#include <net/sock.h>

typedef enum SocketStatusEnum {
	SocketStatusOk = 0,
	SocketStatusError,
} SocketStatus;

SocketStatus OpenSocket(struct sockaddr *sa, struct socket **socket);

SocketStatus ReadDataFromSocket(struct socket *socket, uint8_t *data,
					uint32_t data_len);

SocketStatus WriteDataToSocket(struct socket *socket, uint8_t *data,
					uint32_t data_len);

#endif /* SOCKET_UTILS_H_ */
