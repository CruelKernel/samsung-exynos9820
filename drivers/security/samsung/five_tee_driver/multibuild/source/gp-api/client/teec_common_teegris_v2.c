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
 * Author: Iaroslav Makarchuk (i.makarchuk@samsung.com)
 * Created: 12 Oct 2016
 *
 * Copyright (C) 2016 Samsung Electronics, Inc.
 */

#include <linux/types.h>
#include <tzdev/kernel_api.h>
#include <tzdev/iwnotify.h>
#include <tzdev/tzdev.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "teec_common.h"
#include "protocol.h"
#include "teec_param_utils.h"

/* Timeout in seconds */
#define FIVE_IWNOTIFY_TIMEOUT	3
#define FIVE_IWNOTIFICATION	TZ_IWNOTIFY_OEM_NOTIFICATION_FLAG_13

static DECLARE_COMPLETION(kcmd_event);

static int __kcmd_notifier(struct notifier_block *nb, unsigned long action,
			   void *data)
{
	complete(&kcmd_event);
	return 0;
}

static struct notifier_block kcmd_notifier = {
	.notifier_call	= __kcmd_notifier,
};

static volatile uint8_t g_buffer[ALIGN(sizeof(ProtocolCmd), PAGE_SIZE)]
	__aligned(PAGE_SIZE) = {0};

typedef struct TeegrisSessionStruct {
	int client_id;
	ProtocolCmd *tci_buffer;
	void *parent_context;
	uint32_t shmem_id;
	struct page *shmem_page;
	size_t tci_size;
} TeegrisSession;

static TEEC_Result TeegrisSendCommand(void *ta_session,
			     uint32_t command_id,
			     TEEC_Operation *operation,
			     uint32_t *return_origin,
			     int32_t timeout)
{
	int res = 0;
	TEEC_Result ret = TEEC_ERROR_BAD_PARAMETERS;
	TeegrisSession *session = (TeegrisSession *)ta_session;
	ProtocolParam cmd;

	if (!session)
		goto exit;

	/* tci_buffer is supposed to be not NULL */
	session->tci_buffer->return_origin = TEEC_ORIGIN_API;

	session->tci_buffer->cmd_id = command_id;
	session->tci_buffer->param_types = 0;
	session->tci_buffer->cmd_ret = TEEC_ERROR_COMMUNICATION;

	if (operation) {
		session->tci_buffer->param_types = operation->paramTypes;
		ret = FillCommandArgs(session->tci_buffer,
				      operation, session);

		if (ret != TEEC_SUCCESS)
			goto exit;
	}

	/* Return origin: "COMMS" to handle several conditions below */
	session->tci_buffer->return_origin = TEEC_ORIGIN_COMMS;

	/* Call */
	cmd.memref.buffer = session->shmem_id;
	cmd.memref.size = (uint32_t)session->tci_size;
	res = tzdev_kapi_send(session->client_id, &cmd, sizeof(cmd));
	if (res < 0) {
		pr_err("FIVE: Can't send command tzdev_kapi: %d\n", res);
		ret = TEEC_ERROR_COMMUNICATION;
		goto exit;
	}

	res = wait_for_completion_timeout(&kcmd_event,
					  FIVE_IWNOTIFY_TIMEOUT * HZ);
	if (res <= 0) {
		pr_err("FIVE: Timeout expired on waiting for tzdev_kapi: %d\n",
		       res);
		ret = TEEC_ERROR_COMMUNICATION;
		goto exit;
	}

	res = tzdev_kapi_recv(session->client_id, NULL, 0);
	if (res < 0) {
		pr_err("FIVE: Can't receive answer from tzdev_kapi: %d\n", res);
		ret = TEEC_ERROR_COMMUNICATION;
		goto exit;
	}

	/* Return origin: "TEE" or "TRUSTED_APP" after communication with TEE. */
	session->tci_buffer->return_origin = TEEC_ORIGIN_TRUSTED_APP;

	/* On Success: */
	if (return_origin)
		*return_origin = TEEC_ORIGIN_TRUSTED_APP;

	ret = session->tci_buffer->cmd_ret;

	if (ret == TEEC_SUCCESS && operation) {
		ret = FillOperationArgs(operation, session->tci_buffer);
		if (ret != TEEC_SUCCESS)
			session->tci_buffer->return_origin = TEEC_ORIGIN_API;
	}

exit:
	if (return_origin && session) {
		/* tci_buffer is supposed to be not NULL */
		*return_origin = session->tci_buffer->return_origin;
	}

	return ret;
}

static TEEC_Result TeegrisUnloadTA(void *ta_session)
{
	int res;
	TEEC_Result status = TEEC_SUCCESS;
	TeegrisSession *session = (TeegrisSession *)ta_session;

	res = tzdev_kapi_mem_revoke(session->client_id, session->shmem_id);
	if (res)
		status = TEEC_ERROR_GENERIC;

	res = tzdev_kapi_mem_release(session->shmem_id);
	if (res)
		status = TEEC_ERROR_GENERIC;

	res = tzdev_kapi_close(session->client_id);
	if (res)
		status = TEEC_ERROR_GENERIC;

	res = tz_iwnotify_chain_unregister(FIVE_IWNOTIFICATION, &kcmd_notifier);
	if (res < 0)
		status = TEEC_ERROR_GENERIC;

	if (session->tci_buffer != NULL) {
		session->tci_buffer = NULL;
		session->shmem_page = NULL;
	}

	return status;
}

static TEEC_Result TeegrisLoadTA(void *ta_session, const TEEC_UUID *uuid)
{
	int res;
	TEEC_Result status = TEEC_SUCCESS;
	TeegrisSession *session = (TeegrisSession *)ta_session;

	if (!ta_session || !uuid) {
		status = TEEC_ERROR_BAD_PARAMETERS;
		goto exit;
	}

	session->shmem_page = virt_to_page(g_buffer);
	if (!session->shmem_page) {
		status = TEEC_ERROR_OUT_OF_MEMORY;
		goto exit;
	}

	if (!tzdev_is_up())
		panic("FIVE: tzdev is not ready\n");

	init_completion(&kcmd_event);
	res = tz_iwnotify_chain_register(FIVE_IWNOTIFICATION, &kcmd_notifier);
	if (res < 0) {
		pr_err("FIVE: Can't register iwnotify tzdev: %d\n", res);
		status = TEEC_ERROR_COMMUNICATION;
		goto exit;
	}

	session->tci_buffer = (ProtocolCmd *)g_buffer;
	session->tci_size = ALIGN(sizeof(ProtocolCmd), PAGE_SIZE);

	(void)BUILD_BUG_ON_ZERO((sizeof(struct tz_uuid) != sizeof(TEEC_UUID)));
	res = tzdev_kapi_open((const struct tz_uuid *)uuid);
	if (res < 0) {
		pr_err("FIVE: Can't open tzdev_kapi: %d\n", res);
		status = TEEC_ERROR_GENERIC;
		goto error_chain;
	}

	session->client_id = res;

	res = tzdev_kapi_mem_register(session->tci_buffer,
		session->tci_size, 1);
	if (res < 0) {
		pr_err("FIVE: Can't register tzdev_kapi_mem: %d\n", res);
		status = TEEC_ERROR_GENERIC;
		goto error_close;
	}

	session->shmem_id = res;

	res = tzdev_kapi_mem_grant(session->client_id, session->shmem_id);
	if (res) {
		pr_err("FIVE: Can't grant memory tzdev_kapi_mem: %d\n", res);
		status = TEEC_ERROR_GENERIC;
		goto error_mem;
	}

	return status;

error_chain:
	res = tz_iwnotify_chain_unregister(FIVE_IWNOTIFICATION, &kcmd_notifier);
	if (res < 0)
		pr_err("FIVE: Can't unregister iwnotify tzdev: %d\n", res);

error_mem:
	tzdev_kapi_mem_release(session->shmem_id);

error_close:
	tzdev_kapi_close(session->client_id);

exit:
	return status;
}

static PlatformClientImpl g_teegris_platform_impl = {
	TeegrisLoadTA,
	TeegrisUnloadTA,
	TeegrisSendCommand,
	NULL,
	NULL,
	NULL,
	NULL
};

PlatformClientImpl *GetPlatformClientImpl(void)
{
	return &g_teegris_platform_impl;
}

size_t GetPlatformClientImplSize(void)
{
	return sizeof(TeegrisSession);
}

void ClientImplSetParentContext(void *ta_session, void *context)
{
	TeegrisSession *session = (TeegrisSession *)ta_session;

	if (session)
		session->parent_context = context;
}

void *ClientImplGetParentContext(void *ta_session)
{
	void *ret = NULL;
	TeegrisSession *session = (TeegrisSession *)ta_session;

	if (session)
		ret = session->parent_context;

	return ret;
}
