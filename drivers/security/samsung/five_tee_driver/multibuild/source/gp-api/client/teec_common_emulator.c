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
 * Created: 14 Dec 2018
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <net/sock.h>

#include "teec_common.h"
#include "protocol.h"
#include "teec_param_utils.h"
#include "socket_utils.h"

static const uint16_t FIVE_EMUL_PORT = 0x514e;

typedef struct EmulSessionStruct {
	struct socket *socket;
	ProtocolCmd *command;
} EmulSession;

static uint32_t create_address(const char *ip_addr)
{
	uint8_t ip[4] = {0};

	sscanf(ip_addr, "%hhu.%hhu.%hhu.%hhu", &ip[0], &ip[1], &ip[2], &ip[3]);
	return (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
}

static void CloseTASocket(EmulSession *session)
{
	sock_release(session->socket);
}

static SocketStatus OpenTASocket(EmulSession *session)
{
	struct sockaddr_in sa;

	BUG_ON(!session);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(create_address(CONFIG_FIVE_EMULATOR_IP));
	sa.sin_port = htons(FIVE_EMUL_PORT);

	return OpenSocket((struct sockaddr *)&sa, &session->socket);
}

TEEC_Result EmulSendCommand(void *ta_session,
			     uint32_t command_id,
			     TEEC_Operation *operation,
			     uint32_t *return_origin,
			     int32_t timeout)
{
	int status = 0;
	TEEC_Result ret = TEEC_ERROR_BAD_PARAMETERS;
	EmulSession *emul_session = (EmulSession *)ta_session;
	ProtocolCmd *command;
	struct socket *socket;
	const int cmd_sz = sizeof(ProtocolCmd);

	BUG_ON(!emul_session);

	command = emul_session->command;

	BUG_ON(!command);

	memset(command, 0x00, sizeof(ProtocolCmd));

	/* Return origin: "API" is rewritten always at the exit condition. */
	command->return_origin = TEEC_ORIGIN_API;

	command->cmd_id = command_id;
	command->cmd_ret = TEEC_ERROR_COMMUNICATION;
	command->param_types = 0;

	if (operation) {
		command->param_types = operation->paramTypes;
		ret = FillCommandArgs(command, operation, NULL);

		if (TEEC_SUCCESS != ret)
			goto exit;
	}

	/* Return origin: "COMMS" by default because of possible communication errors. */
	command->return_origin = TEEC_ORIGIN_COMMS;

	status = OpenTASocket(emul_session);
	if (SocketStatusOk != status) {
		pr_err("FIVE: error: connect to proxy failed: %d\n", status);
		ret = TEEC_ERROR_COMMUNICATION;
		goto exit;
	}

	socket = emul_session->socket;

	status = WriteDataToSocket(socket, (uint8_t *)command, cmd_sz);
	if (SocketStatusOk != status) {
		ret = TEEC_ERROR_COMMUNICATION;
		goto close;
	}

	status = ReadDataFromSocket(socket, (uint8_t *)command, cmd_sz);
	if (SocketStatusOk != status) {
		ret = TEEC_ERROR_COMMUNICATION;
		goto close;
	}

	/* Return origin: "TEE" or "TRUSTED_APP" after communication with TEE. */

	ret = command->cmd_ret;

	if (TEEC_SUCCESS == ret && operation) {
		ret = FillOperationArgs(operation, command);
		if (TEEC_SUCCESS != ret)
			command->return_origin = TEEC_ORIGIN_API;
	}

close:
	CloseTASocket(emul_session);
exit:
	if (return_origin)
		*return_origin = command->return_origin;

	return ret;
}

TEEC_Result EmulUnloadTA(void *ta_session)
{
	EmulSession *session = (EmulSession *)ta_session;

	kfree(session->command);
	return TEEC_SUCCESS;
}

TEEC_Result EmulLoadTA(void *ta_session, const TEEC_UUID *uuid)
{
	EmulSession *session = (EmulSession *)ta_session;

	session->command = NULL;
	session->command = kmalloc(sizeof(ProtocolCmd), GFP_KERNEL);

	if (!session->command)
		return TEEC_ERROR_OUT_OF_MEMORY;

	/* Do nothing. Open a connection on the first command invocation. */
	return TEEC_SUCCESS;
}

PlatformClientImpl g_emul_platform_impl = {
	EmulLoadTA,
	EmulUnloadTA,
	EmulSendCommand,
	NULL,
	NULL,
	NULL,
	NULL
};

PlatformClientImpl *GetPlatformClientImpl(void)
{
	return &g_emul_platform_impl;
}

size_t GetPlatformClientImplSize(void)
{
	return sizeof(EmulSession);
}
