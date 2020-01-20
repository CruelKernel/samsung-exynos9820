/*
 * Copyright (C) 2012-2018, Samsung Electronics Co., Ltd.
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

#include <linux/completion.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/socket.h>

#include <asm/barrier.h>
#include <asm/cmpxchg.h>

#include <tzdev/tee_client_api.h>

#include "iw_messages.h"
#include "misc.h"
#include "sysdep.h"
#include "types.h"
#include "tz_iwsock.h"
#include "tzlog.h"

enum {
	TZDEV_TEEC_OPERATION_NOT_STARTED,
	TZDEV_TEEC_OPERATION_COMMAND_BEGIN,
	TZDEV_TEEC_OPERATION_COMMAND_END,
	TZDEV_TEEC_OPERATION_CANCEL_BEGIN,
	TZDEV_TEEC_OPERATION_CANCEL_END,
};

static DEFINE_MUTEX(tzdev_teec_operation_lock);
static LIST_HEAD(tzdev_teec_operation_cancel_list);

static void tzdev_teec_convert_uuid(struct tz_uuid *uuid, const TEEC_UUID *teec_uuid)
{
	unsigned int i;

	uuid->time_low = teec_uuid->timeLow;
	uuid->time_mid = teec_uuid->timeMid;
	uuid->time_hi_and_version = teec_uuid->timeHiAndVersion;
	for (i = 0; i < 8; ++i)
		uuid->clock_seq_and_node[i] = teec_uuid->clockSeqAndNode[i];
}

static uint32_t tzdev_teec_session_init(TEEC_Context *context, TEEC_Session *session)
{
	struct tzdev_teec_session *ses;
	uint32_t result;

	tzdev_teec_debug("Enter, context = %pK session = %pK\n", context, session);

	ses = kmalloc(sizeof(struct tzdev_teec_session), GFP_KERNEL);
	if (!ses) {
		tzdev_teec_error("Failed to allocate session struct\n");
		result = TEEC_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	mutex_init(&ses->mutex);
	init_completion(&ses->cancel);
	ses->context = context;
	ses->socket = NULL;
	ses->serial = 0;

	session->imp = ses;

	result = TEEC_SUCCESS;

out:
	tzdev_teec_debug("Exit, context = %pK session = %pK\n", context, session);

	return result;
}

static void tzdev_teec_session_fini(TEEC_Session *session)
{
	struct tzdev_teec_session *ses = session->imp;

	tzdev_teec_debug("Enter, session = %pK\n", session);

	session->imp = NULL;

	mutex_destroy(&ses->mutex);

	kfree(ses);

	tzdev_teec_debug("Exit, session = %pK\n", session);
}

static uint32_t tzdev_teec_operation_begin(TEEC_Session *session, TEEC_Operation *operation)
{
	struct tzdev_teec_session *ses = session->imp;
	struct tzdev_teec_operation *op;
	uint32_t result = TEEC_SUCCESS;

	mutex_lock(&tzdev_teec_operation_lock);

	list_for_each_entry(op, &tzdev_teec_operation_cancel_list, link) {
		if (op->operation == operation) {
			list_del(&op->link);
			result = TEEC_ERROR_CANCEL;
			goto out;
		}
	}

	reinit_completion(&ses->cancel);
	operation->imp = session;

	set_mb(operation->started, TZDEV_TEEC_OPERATION_COMMAND_BEGIN);

out:
	mutex_unlock(&tzdev_teec_operation_lock);

	if (result == TEEC_ERROR_CANCEL)
		kfree(op);

	return result;
}

static void tzdev_teec_operation_end(TEEC_Operation *operation)
{
	TEEC_Session *session = operation->imp;
	struct tzdev_teec_session *ses = session->imp;
	uint32_t old_started;

	old_started = xchg(&operation->started, TZDEV_TEEC_OPERATION_COMMAND_END);

	BUG_ON(old_started != TZDEV_TEEC_OPERATION_COMMAND_BEGIN
			&& old_started != TZDEV_TEEC_OPERATION_CANCEL_BEGIN
			&& old_started != TZDEV_TEEC_OPERATION_CANCEL_END);

	if (unlikely(old_started == TZDEV_TEEC_OPERATION_CANCEL_BEGIN))
		wait_for_completion(&ses->cancel);
}

static uint32_t tzdev_teec_operation_add_to_cancel_list(TEEC_Operation *operation)
{
	struct tzdev_teec_operation *op;

	list_for_each_entry(op, &tzdev_teec_operation_cancel_list, link) {
		if (op->operation == operation)
			return TEEC_ERROR_CANCEL;
	}

	op = kmalloc(sizeof(struct tzdev_teec_operation), GFP_KERNEL);
	if (!op) {
		tzdev_teec_error("Failed to allocate operation struct\n");
		return TEEC_ERROR_OUT_OF_MEMORY;
	}

	INIT_LIST_HEAD(&op->link);
	op->operation = operation;
	list_add(&op->link, &tzdev_teec_operation_cancel_list);

	return TEEC_ERROR_CANCEL;
}

static uint32_t tzdev_teec_operation_cancel_begin(TEEC_Operation *operation)
{
	uint32_t old_started;
	uint32_t result;

	mutex_lock(&tzdev_teec_operation_lock);

	old_started = xchg(&operation->started, TZDEV_TEEC_OPERATION_CANCEL_BEGIN);

	switch (old_started) {
	case TZDEV_TEEC_OPERATION_NOT_STARTED:
		result = tzdev_teec_operation_add_to_cancel_list(operation);
		break;
	case TZDEV_TEEC_OPERATION_COMMAND_BEGIN:
		result = TEEC_SUCCESS;
		break;
	default:
		tzdev_teec_error("Trying to cancel operation with started = %u\n", old_started);
		result = TEEC_ERROR_BAD_STATE;
		break;
	}

	mutex_unlock(&tzdev_teec_operation_lock);

	return result;
}

static void tzdev_teec_operation_cancel_end(TEEC_Operation *operation)
{
	TEEC_Session *session = operation->imp;
	struct tzdev_teec_session *ses = session->imp;
	uint32_t old_started;

	old_started = xchg(&operation->started, TZDEV_TEEC_OPERATION_CANCEL_END);

	BUG_ON(old_started != TZDEV_TEEC_OPERATION_COMMAND_END
			&& old_started != TZDEV_TEEC_OPERATION_CANCEL_BEGIN);

	complete(&ses->cancel);
}

static uint32_t tzdev_teec_operation_init(TEEC_Session *session,
		TEEC_Operation *operation, struct tee_operation *op,
		uint32_t *origin)
{
	TEEC_RegisteredMemoryReference *memref;
	TEEC_SharedMemory *sharedMem;
	struct tzdev_teec_session *ses;
	struct tzdev_teec_shared_memory *shm;
	unsigned int i, j;
	uint32_t type;
	uint32_t result;

	tzdev_teec_debug("Enter, session = %pK operation = %pK\n", session, operation);

	*origin = TEEC_ORIGIN_API;

	result = tzdev_teec_operation_begin(session, operation);
	if (result != TEEC_SUCCESS)
		goto out;

	ses = session->imp;

	for (i = 0; i < MAX_PARAM_COUNT; ++i) {
		type = PARAM_TYPE_GET(operation->paramTypes, i);

		ses->tmpref[i].exists = false;

		switch (type) {
		case TEEC_NONE:
		case TEEC_VALUE_OUTPUT:
			op->tee_param_types |= PARAM_TYPE_SET(type, i);
			memset(&op->tee_params[i], 0x0, sizeof(struct entry_point_params));
			break;
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_INOUT:
			op->tee_param_types |= PARAM_TYPE_SET(type, i);
			op->tee_params[i].val.a = operation->params[i].value.a;
			op->tee_params[i].val.b = operation->params[i].value.b;
			break;
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			op->tee_param_types |= PARAM_TYPE_SET(type, i);

			if (operation->params[i].tmpref.buffer == NULL) {
				op->tee_params[i].shared_buffer_description.offset = 0;
				op->tee_params[i].shared_buffer_description.size = 0;
				op->tee_params[i].shared_buffer_description.id = 0;
			} else {
				ses->tmpref[i].shmem.buffer = operation->params[i].tmpref.buffer;
				ses->tmpref[i].shmem.size = operation->params[i].tmpref.size;
				ses->tmpref[i].exists = true;

				if (type == TEEC_MEMREF_TEMP_INPUT)
					ses->tmpref[i].shmem.flags = TEEC_MEM_INPUT;
				else if (type == TEEC_MEMREF_TEMP_OUTPUT)
					ses->tmpref[i].shmem.flags = TEEC_MEM_OUTPUT;
				else
					ses->tmpref[i].shmem.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;

				result = TEEC_RegisterSharedMemory(ses->context, &ses->tmpref[i].shmem);
				if (result != TEEC_SUCCESS)
					goto out_free_tmpref;

				shm = ses->tmpref[i].shmem.imp;

				op->tee_params[i].shared_buffer_description.offset = shm->offset;
				op->tee_params[i].shared_buffer_description.id = shm->id;
				op->tee_params[i].shared_buffer_description.size = (unsigned int)ses->tmpref[i].shmem.size;
			}

			break;
		case TEEC_MEMREF_WHOLE:
			sharedMem = operation->params[i].memref.parent;

			switch (sharedMem->flags) {
			case TEEC_MEM_INPUT:
				op->tee_param_types |= PARAM_TYPE_SET(TEEC_MEMREF_TEMP_INPUT, i);
				break;
			case TEEC_MEM_OUTPUT:
				op->tee_param_types |= PARAM_TYPE_SET(TEEC_MEMREF_TEMP_OUTPUT, i);
				break;
			case TEEC_MEM_INPUT | TEEC_MEM_OUTPUT:
				op->tee_param_types |= PARAM_TYPE_SET(TEEC_MEMREF_TEMP_INOUT, i);
				break;
			default:
				result = TEEC_ERROR_BAD_PARAMETERS;
				goto out_free_tmpref;
			}

			shm = sharedMem->imp;

			op->tee_params[i].shared_buffer_description.offset = shm->offset;
			op->tee_params[i].shared_buffer_description.id = shm->id;
			op->tee_params[i].shared_buffer_description.size = (unsigned int)sharedMem->size;
			break;
		case TEEC_MEMREF_PARTIAL_INPUT:
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			memref = &operation->params[i].memref;
			sharedMem = memref->parent;

			op->tee_param_types |= PARAM_TYPE_SET((type +
					TEEC_MEMREF_TEMP_INPUT - TEEC_MEMREF_PARTIAL_INPUT), i);

			shm = sharedMem->imp;

			op->tee_params[i].shared_buffer_description.offset = shm->offset + (unsigned int)memref->offset;
			op->tee_params[i].shared_buffer_description.id = shm->id;
			op->tee_params[i].shared_buffer_description.size = (unsigned int)memref->size;
			break;
		default:
			result = TEEC_ERROR_BAD_PARAMETERS;
			goto out_free_tmpref;
		}
	}

	result = TEEC_SUCCESS;
	goto out;

out_free_tmpref:
	for (j = 0; j < i; j++) {
		if (ses->tmpref[j].exists)
			TEEC_ReleaseSharedMemory(&ses->tmpref[j].shmem);
	}

	tzdev_teec_operation_end(operation);
out:
	tzdev_teec_debug("Exit, session = %pK operation = %pK\n", session, operation);

	return result;
}

static void tzdev_teec_operation_fini(TEEC_Operation *operation,
		struct tee_operation *op)
{
	TEEC_Session *session;
	struct tzdev_teec_session *ses;
	unsigned int i;
	uint32_t type;

	tzdev_teec_debug("Enter, operation = %pK\n", operation);

	session = operation->imp;
	ses = session->imp;

	tzdev_teec_operation_end(operation);

	for (i = 0; i < MAX_PARAM_COUNT; ++i) {
		type = PARAM_TYPE_GET(operation->paramTypes, i);

		if (ses->tmpref[i].exists)
			TEEC_ReleaseSharedMemory(&ses->tmpref[i].shmem);

		if (!op)
			continue;

		switch (type) {
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT:
			operation->params[i].value.a = op->tee_params[i].val.a;
			operation->params[i].value.b = op->tee_params[i].val.b;
			break;
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT:
			operation->params[i].tmpref.size = op->tee_params[i].shared_buffer_description.size;
			break;
		case TEEC_MEMREF_PARTIAL_OUTPUT:
		case TEEC_MEMREF_PARTIAL_INOUT:
			operation->params[i].memref.size = op->tee_params[i].shared_buffer_description.size;
			break;
		case TEEC_MEMREF_WHOLE:
			if (operation->params[i].memref.parent->flags & TEEC_MEM_OUTPUT)
				operation->params[i].memref.size = op->tee_params[i].shared_buffer_description.size;
			break;
		default:
			break;
		}
	}

	tzdev_teec_debug("Exit, operation = %pK\n", operation);
}

static uint32_t tzdev_teec_prepare_session(TEEC_Session *session,
		const TEEC_UUID *destination, struct tzdev_teec_shared_memory *shm, size_t shm_size, uint32_t *origin)
{
	struct tzdev_teec_session *ses = session->imp;
	struct tzdev_teec_context *ctx = ses->context->imp;
	struct cmd_prepare_session cmd;
	struct cmd_prepare_session_reply ack;
	uint32_t result;
	int ret;

	tzdev_teec_debug("Enter, session = %pK\n", session);

	*origin = TEEC_ORIGIN_API;

	memset(&cmd, 0, sizeof(cmd));

	cmd.base.cmd = CMD_PREPARE_SESSION;
	cmd.base.serial = ses->serial;
	cmd.ctx_id = ctx->id;

	if (shm && shm_size != 0) {
		cmd.buf_desc.offset = shm->offset;
		cmd.buf_desc.id = shm->id;
		cmd.buf_desc.size = (unsigned int)shm_size;
	}

	tzdev_teec_convert_uuid(&cmd.ta_uuid, destination);

	ret = tzdev_teec_send_then_recv(ses->socket,
			&cmd, sizeof(cmd), 0x0,
			&ack, sizeof(ack), 0x0,
			&result, origin);
	if (ret < 0) {
		tzdev_teec_error("Failed to xmit prepare session, ret = %d\n", ret);
		goto out;
	}

	ret = tzdev_teec_check_reply(&ack.base, CMD_REPLY_PREPARE_SESSION,
			ses->serial, &result, origin);
	if (ret) {
		tzdev_teec_error("Failed to check prepare session reply, ret = %d\n", ret);
		goto out;
	}

	result = ack.base.result;
	*origin = ack.base.origin;

out:
	ses->serial++;

	tzdev_teec_fixup_origin(result, origin);

	tzdev_teec_debug("Exit, session = %pK\n", session);

	return result;
}

static void tzdev_teec_prepare_connection_data(uint32_t conn_meth, void *conn_data, void *buf)
{
	switch(conn_meth) {
	case TEEC_LOGIN_PUBLIC:
	case TEEC_LOGIN_USER:
	case TEEC_LOGIN_APPLICATION:
	case TEEC_LOGIN_USER_APPLICATION:
		break;
	case TEEC_LOGIN_GROUP:
	case TEEC_LOGIN_GROUP_APPLICATION:
		memcpy(buf, conn_data, sizeof(uint32_t));
		break;
	default:
		BUG();
	}
}

static uint32_t tzdev_teec_open_session(TEEC_Session *session, TEEC_Operation *operation,
		uint32_t connectionMethod, void *connectionData, uint32_t *origin)
{
	struct tzdev_teec_session *ses = session->imp;
	struct cmd_open_session cmd;
	struct cmd_reply_open_session ack;
	struct tee_operation *op = NULL;
	uint32_t result;
	int ret;

	tzdev_teec_debug("Enter, session = %pK\n", session);

	*origin = TEEC_ORIGIN_API;

	memset(&cmd, 0, sizeof(cmd));

	if (operation) {
		result = tzdev_teec_operation_init(session, operation, &cmd.op, origin);
		if (result != TEEC_SUCCESS)
			goto out;
	}

	cmd.base.cmd = CMD_OPEN_SESSION;
	cmd.base.serial = ses->serial;
	cmd.cancel_time = IWD_TIMEOUT_INFINITY;
	cmd.conn_meth = connectionMethod;

	tzdev_teec_prepare_connection_data(connectionMethod, connectionData, cmd.conn_data);

	ret = tzdev_teec_send_then_recv(ses->socket,
			&cmd, sizeof(cmd), 0x0,
			&ack, sizeof(ack), 0x0,
			&result, origin);
	if (ret < 0) {
		tzdev_teec_error("Failed to xmit open session, ret = %d\n", ret);
		goto out_fini_op;
	}

	ret = tzdev_teec_check_reply(&ack.base, CMD_REPLY_OPEN_SESSION,
			ses->serial, &result, origin);
	if (ret) {
		tzdev_teec_error("Failed to check open session reply, ret = %d\n", ret);
		goto out_fini_op;
	}

	result = ack.base.result;
	*origin = ack.base.origin;

	op = &ack.op;

out_fini_op:
	if (operation)
		tzdev_teec_operation_fini(operation, op);

	ses->serial++;

out:
	tzdev_teec_fixup_origin(result, origin);

	tzdev_teec_debug("Exit, session = %pK\n", session);

	return result;
}

static uint32_t tzdev_teec_close_session(TEEC_Session *session, uint32_t *origin)
{
	struct tzdev_teec_session *ses = session->imp;
	struct cmd_close_session cmd;
	struct cmd_reply_close_session ack;
	uint32_t result;
	int ret;

	tzdev_teec_debug("Enter, session = %pK\n", session);

	*origin = TEEC_ORIGIN_API;

	memset(&cmd, 0, sizeof(cmd));

	cmd.base.cmd = CMD_CLOSE_SESSION;
	cmd.base.serial = ses->serial;

	ret = tzdev_teec_send_then_recv(ses->socket,
			&cmd, sizeof(cmd), 0x0,
			&ack, sizeof(ack), 0x0,
			&result, origin);
	if (ret < 0) {
		tzdev_teec_error("Failed to xmit close session, ret = %d\n", ret);
		goto out;
	}

	ret = tzdev_teec_check_reply(&ack.base, CMD_REPLY_CLOSE_SESSION,
			ses->serial, &result, origin);
	if (ret) {
		tzdev_teec_error("Failed to check close session reply, ret = %d\n", ret);
		goto out;
	}

	result = ack.base.result;
	*origin = ack.base.origin;

out:
	ses->serial++;

	tzdev_teec_fixup_origin(result, origin);

	tzdev_teec_debug("Exit, session = %pK\n", session);

	return result;
}

static uint32_t tzdev_teec_invoke_cmd(TEEC_Session *session, TEEC_Operation *operation,
		uint32_t cmd_id, uint32_t *origin)
{
	struct tzdev_teec_session *ses = session->imp;
	struct cmd_invoke_command cmd;
	struct cmd_reply_invoke_command ack;
	struct tee_operation *op = NULL;
	uint32_t result;
	int ret;

	tzdev_teec_debug("Enter, session = %pK operation = %pK\n", session, operation);

	*origin = TEEC_ORIGIN_API;

	memset(&cmd, 0, sizeof(cmd));

	if (operation) {
		result = tzdev_teec_operation_init(session, operation, &cmd.op, origin);
		if (result != TEEC_SUCCESS)
			goto out;
	}

	cmd.base.cmd = CMD_INVOKE_COMMAND;
	cmd.base.serial = ses->serial;
	cmd.cancel_time = IWD_TIMEOUT_INFINITY;
	cmd.tee_cmd_id = cmd_id;

	ret = tzdev_teec_send_then_recv(ses->socket,
			&cmd, sizeof(cmd), 0x0,
			&ack, sizeof(ack), 0x0,
			&result, origin);
	if (ret < 0) {
		tzdev_teec_error("Failed to xmit invoke command, ret = %d\n", ret);
		goto out_fini_op;
	}

	ret = tzdev_teec_check_reply(&ack.base, CMD_REPLY_INVOKE_COMMAND,
			ses->serial, &result, origin);
	if (ret) {
		tzdev_teec_error("Failed to check invoke command reply, ret = %d\n", ret);
		goto out_fini_op;
	}

	result = ack.base.result;
	*origin = ack.base.origin;

	op = &ack.op;

out_fini_op:
	if (operation)
		tzdev_teec_operation_fini(operation, op);

	ses->serial++;

out:
	tzdev_teec_fixup_origin(result, origin);

	tzdev_teec_debug("Exit, session = %pK\n", session);

	return result;
}

static uint32_t tzdev_teec_cancel_cmd(TEEC_Operation *operation, uint32_t *origin)
{
	TEEC_Session *session = operation->imp;
	struct tzdev_teec_session *ses = session->imp;
	struct cmd_cancellation cmd;
	uint32_t result;
	int ret;

	cmd.base.cmd = CMD_CANCELLATION;
	cmd.base.serial = 0;
	cmd.op_serial = ses->serial;

	ret = tzdev_teec_send(ses->socket,
			&cmd, sizeof(cmd), MSG_OOB,
			&result, origin);
	if (ret < 0)
		tzdev_teec_error("Failed to send cancellation request, ret = %d\n", ret);
	else if (ret != sizeof(cmd))
		tzdev_teec_error("Failed to send cancellation request due to invalid size, ret = %d\n", ret);

	return result;
}

static uint32_t tzdev_teec_session_check_args(TEEC_Context *context,
		TEEC_Session *session, const TEEC_UUID *destination,
		uint32_t connectionMethod, void *connectionData)
{
	if (!context) {
		tzdev_teec_error("Null context passed\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (!session) {
		tzdev_teec_error("Null session passed\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (!destination) {
		tzdev_teec_error("Null destination passed\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	switch(connectionMethod) {
	case TEEC_LOGIN_PUBLIC:
	case TEEC_LOGIN_USER:
	case TEEC_LOGIN_APPLICATION:
	case TEEC_LOGIN_USER_APPLICATION:
		if (connectionData) {
			tzdev_teec_error("Unexpected non-null connection data passed\n");
			return TEEC_ERROR_BAD_PARAMETERS;
		}
		break;
	case TEEC_LOGIN_GROUP:
	case TEEC_LOGIN_GROUP_APPLICATION:
		if (!connectionData) {
			tzdev_teec_error("Unexpected null connection data passed\n");
			return TEEC_ERROR_BAD_PARAMETERS;
		}
		break;
	default:
		tzdev_teec_error("Unknown connection method passed = %u\n", connectionMethod);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	return TEEC_SUCCESS;
}

TEEC_Result TEEC_OpenSession(TEEC_Context *context, TEEC_Session *session,
		const TEEC_UUID *destination, uint32_t connectionMethod,
		void *connectionData, TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	return TEECS_OpenSession(context, session, destination, NULL, 0,
				connectionMethod, connectionData, operation, returnOrigin);
}

TEEC_Result TEECS_OpenSession(TEEC_Context *context, TEEC_Session *session,
		const TEEC_UUID *destination, const void *src_ta_image,
		const size_t src_image_size, uint32_t connectionMethod,
		void *connectionData, TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	TEEC_SharedMemory sharedMem;
	struct tzdev_teec_session *ses;
	struct sock_desc *sd;
	uint32_t result;
	uint32_t origin;
	int ret;

	tzdev_teec_debug("Enter, context = %pK session = %pK operation = %pK\n",
			context, session, operation);

	origin = TEEC_ORIGIN_API;

	result = tzdev_teec_session_check_args(context, session, destination,
			connectionMethod, connectionData);
	if (result != TEEC_SUCCESS)
		goto out;

	tzdev_teec_session_init(context, session);

	sd = tz_iwsock_socket(1);
	if (IS_ERR(sd))
		goto out_fini_session;

	ret = tzdev_teec_connect(sd, ROOT_TASK_SOCK, &result, &origin);
	if (ret < 0) {
		tzdev_teec_error("Failed to connect to root task, session = %pK ret = %d\n",
				session, ret);
		goto out_disconnect;
	}

	ses = session->imp;
	ses->socket = sd;
	memset(&sharedMem, 0, sizeof(sharedMem));

	if (src_ta_image && src_image_size != 0) {
		sharedMem.size = src_image_size;
		sharedMem.flags = TEEC_MEM_INPUT;
		sharedMem.buffer = (void *)src_ta_image;

		ret = TEEC_RegisterSharedMemory(context, &sharedMem);
		if (result != TEEC_SUCCESS) {
			tzdev_teec_error("Failed to allocate shared memory for TA, session = %pK\n", session);
			goto out_disconnect;
		}
	}

	result = tzdev_teec_prepare_session(session, destination, (struct tzdev_teec_shared_memory *)sharedMem.imp, sharedMem.size, &origin);
	if (result != TEEC_SUCCESS) {
		tzdev_teec_error("Failed to prepare session, session = %pK\n", session);
		goto out_release_shared_memory;
	}
	
	if (src_ta_image && src_image_size != 0)
		TEEC_ReleaseSharedMemory(&sharedMem);

	result = tzdev_teec_open_session(session, operation, connectionMethod,
			connectionData, &origin);
	if (result != TEEC_SUCCESS) {
		tzdev_teec_error("Failed to open session, session = %pK\n", session);
		goto out_disconnect;
	}

	tzdev_teec_debug("Success, context = %pK session = %pK socket = %u\n",
			context, session, ses->socket->id);
	goto out;

out_release_shared_memory:
	if (src_ta_image && src_image_size != 0)
		TEEC_ReleaseSharedMemory(&sharedMem);

out_disconnect:
	tzdev_teec_disconnect(sd);
out_fini_session:
	tzdev_teec_session_fini(session);
out:
	if (returnOrigin)
		*returnOrigin = origin;

	tzdev_teec_debug("Exit, context = %pK session = %pK operation = %pK result = %x origin = %u\n",
			context, session, operation, result, origin);

	return result;
}

void TEEC_CloseSession(TEEC_Session *session)
{
	struct tzdev_teec_session *ses;
	uint32_t result;
	uint32_t origin;

	tzdev_teec_debug("Enter, session = %pK\n", session);

	origin = TEEC_ORIGIN_API;

	if (!session || !session->imp) {
		tzdev_teec_error("Null session passed\n");
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	ses = session->imp;

	result = tzdev_teec_close_session(session, &origin);
	if (result != TEEC_SUCCESS) {
		tzdev_teec_error("Failed to close session, session = %pK socket = %u\n",
				session, ses->socket->id);
		goto out_fini;
	}

	tzdev_teec_debug("Success, session = %pK socket = %u\n", session, ses->socket->id);

out_fini:
	tzdev_teec_disconnect(ses->socket);
	tzdev_teec_session_fini(session);
out:
	tzdev_teec_debug("Exit, session = %pK result = %x origin = %u\n", session, result, origin);
}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session, uint32_t commandID,
		TEEC_Operation *operation, uint32_t *returnOrigin)
{
	struct tzdev_teec_session *ses;
	uint32_t result;
	uint32_t origin;

	tzdev_teec_debug("Enter, session = %pK operation = %pK\n", session, operation);

	origin = TEEC_ORIGIN_API;

	if (!session || !session->imp) {
		tzdev_teec_error("Null session passed\n");
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	ses = session->imp;

	mutex_lock(&ses->mutex);
	result = tzdev_teec_invoke_cmd(session, operation, commandID, &origin);
	mutex_unlock(&ses->mutex);

	if (result != TEEC_SUCCESS) {
		tzdev_teec_error("Failed request invoke command, session = %pK socket = %u\n",
				session, ses->socket->id);
		goto out;
	}

	tzdev_teec_debug("Success, session = %pK socket = %u\n", session, ses->socket->id);

out:
	if (returnOrigin)
		*returnOrigin = origin;

	tzdev_teec_debug("Exit, session = %pK operation = %pK\n", session, operation);

	return result;
}

void TEEC_RequestCancellation(TEEC_Operation *operation)
{
	uint32_t origin;
	uint32_t result;

	tzdev_teec_debug("Enter, operation = %pK\n", operation);

	origin = TEEC_ORIGIN_API;
	result = TEEC_SUCCESS;

	if (!operation) {
		tzdev_teec_error("Null operation passed\n");
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	result = tzdev_teec_operation_cancel_begin(operation);
	if (result != TEEC_SUCCESS)
		goto out;

	result = tzdev_teec_cancel_cmd(operation, &origin);

	tzdev_teec_operation_cancel_end(operation);

out:
	tzdev_teec_debug("Enter, operation = %pK result = %x origin = %u\n", operation, result, origin);
}
