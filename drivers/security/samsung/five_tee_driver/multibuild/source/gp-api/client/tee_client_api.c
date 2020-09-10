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

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/printk.h>
#include "tee_client_api.h"
#include "protocol.h"
#include "teec_common.h"

#define CONTEXT_INITIALIZED 0xA7A6A5A4
#define MEM_SLOTS_NUM       4

typedef struct TeecContextStruct {
	uint32_t initialized;
	TEEC_SharedMemory *allocated_shmem[MEM_SLOTS_NUM];
} TeecContext;

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin)
{
	TEEC_Result ret = TEEC_ERROR_BAD_PARAMETERS;
	PlatformClientImpl *client_impl = GetPlatformClientImpl();

	if (!session || !session->imp)
		goto exit;

	if (returnOrigin)
		*returnOrigin = TEEC_ORIGIN_API;

	if (!client_impl || !client_impl->SendCommand) {
		ret = TEEC_ERROR_NOT_IMPLEMENTED;
		goto exit;
	}

	ret = client_impl->SendCommand(session->imp,
				       commandID, operation, returnOrigin,
				       MB_INFINITE_TIMEOUT);

exit:
	return ret;
}

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	TEEC_Result res = TEEC_ERROR_BAD_PARAMETERS;

	(void) name;
	if (context) {
		TeecContext *teec_ctx = kzalloc(sizeof(TeecContext),
						GFP_KERNEL);
		if (teec_ctx) {
			teec_ctx->initialized = CONTEXT_INITIALIZED;
			context->imp = (void *)teec_ctx;
			res = TEEC_SUCCESS;
		}
	}

	return res;
}

void TEEC_FinalizeContext(TEEC_Context *context)
{
	if (context) {
		TeecContext *teec_ctx = (TeecContext *)context->imp;

		if (teec_ctx) {
			teec_ctx->initialized = 0;
			kfree(context->imp);
			context->imp = NULL;
		}
	}
}

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
			     TEEC_Session *session,
			     const TEEC_UUID *destination,
			     uint32_t connectionMethod,
			     const void *connectionData,
			     TEEC_Operation *operation,
			     uint32_t *returnOrigin)
{
	TEEC_Result ret = TEEC_ERROR_BAD_PARAMETERS;
	PlatformClientImpl *client_impl = GetPlatformClientImpl();
	/* Fixed waiting initialization of DCI thread in TBase driver */
	const uint32_t kNumTries = 5;
	const int32_t kTimeoutMs = 1000;
	uint32_t elapsed_tries = 0;

	if (!context || !context->imp ||
	    ((TeecContext *)(context->imp))->initialized != CONTEXT_INITIALIZED
	    || !session || !destination) {
		goto exit;
	}

	if (returnOrigin)
		*returnOrigin = TEEC_ORIGIN_API;

	if (!client_impl || !client_impl->SendCommand || !client_impl->LoadTA) {
		ret = TEEC_ERROR_NOT_IMPLEMENTED;
		goto exit;
	}

	session->imp = kzalloc(GetPlatformClientImplSize(), GFP_KERNEL);

	if (!session->imp) {
		ret = TEEC_ERROR_OUT_OF_MEMORY;
		goto exit;
	}

	ret = client_impl->LoadTA(session->imp, destination);

	if (ret != TEEC_SUCCESS)
		goto cleanup;

	do {
		ret = client_impl->SendCommand(session->imp,
				PROTOCOL_COMMAND_LOAD, operation, returnOrigin,
				kTimeoutMs);

		if (TEEC_SUCCESS != ret) {
			pr_debug(
			  "Try to send %u of %u command in driver: 0x%x.\n",
			  elapsed_tries + 1, kNumTries, ret);
		}
	} while ((ret != TEEC_SUCCESS) && (++elapsed_tries < kNumTries));

	if (TEEC_SUCCESS != ret) {
		pr_debug("Could not send %d commands in driver: 0x%x.\n",
				kNumTries, ret);
		client_impl->SendCommand(session->imp, PROTOCOL_COMMAND_UNLOAD,
				NULL, returnOrigin, MB_INFINITE_TIMEOUT);
		client_impl->UnloadTA(session->imp);
	}

cleanup:
	if (ret != TEEC_SUCCESS) {
		kfree(session->imp);
		session->imp = NULL;
	}

exit:
	return ret;
}

void TEEC_CloseSession(TEEC_Session *session)
{
	uint32_t return_origin;
	PlatformClientImpl *client_impl = GetPlatformClientImpl();

	if (session && session->imp) {
		if (client_impl &&
		    client_impl->SendCommand && client_impl->UnloadTA) {
			client_impl->SendCommand(session->imp,
				 PROTOCOL_COMMAND_UNLOAD, NULL, &return_origin,
				 MB_INFINITE_TIMEOUT);
			client_impl->UnloadTA(session->imp);
		}
		kfree(session->imp);
		session->imp = NULL;
	}
}

TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context,
				      TEEC_SharedMemory *sharedMem)
{
	TEEC_Result result = TEEC_ERROR_BAD_PARAMETERS;

	if (!context || !sharedMem || !sharedMem->buffer)
		goto exit;

	if (sharedMem->size > TEEC_CONFIG_SHAREDMEM_MAX_SIZE) {
		result = TEEC_ERROR_OUT_OF_MEMORY;
		goto exit;
	}

	sharedMem->imp.context = context;
	result = TEEC_SUCCESS;
exit:
	return result;
}

static inline int GetShmemSlotIndex(TeecContext *ctx, TEEC_SharedMemory *mem)
{
	int ret = -1;
	int i = 0;

	for (i = 0; i < MEM_SLOTS_NUM; i++) {
		if (ctx->allocated_shmem[i] == mem) {
			ret = i;
			break;
		}
	}
	return ret;
}

TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context,
				      TEEC_SharedMemory *sharedMem)
{
	TEEC_Result result = TEEC_ERROR_BAD_PARAMETERS;
	int ctx_shmem_index = -1;

	if (!context || !context->imp || !sharedMem ||
	  ((TeecContext *)(context->imp))->initialized != CONTEXT_INITIALIZED) {
		goto exit;
	}

	ctx_shmem_index =
			GetShmemSlotIndex((TeecContext *)(context->imp), NULL);

	result = TEEC_ERROR_OUT_OF_MEMORY;
	if (sharedMem->size > TEEC_CONFIG_SHAREDMEM_MAX_SIZE ||
	    ctx_shmem_index < 0) {
		goto exit;
	}

	sharedMem->buffer = kzalloc(sharedMem->size, GFP_KERNEL);
	if (!sharedMem->buffer)
		goto exit;

	sharedMem->imp.context = context;
	((TeecContext *)(context->imp))->allocated_shmem[ctx_shmem_index]
								    = sharedMem;
	result = TEEC_SUCCESS;

exit:
	return result;
}

void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMem)
{
	if (sharedMem &&
		sharedMem->imp.context && sharedMem->imp.context->imp) {
		int ctx_shmem_index = GetShmemSlotIndex(
				(TeecContext *)(sharedMem->imp.context->imp),
				sharedMem);

		if (ctx_shmem_index >= 0) {
			((TeecContext *)(sharedMem->imp.context->imp))->
					allocated_shmem[ctx_shmem_index] = NULL;
			kzfree(sharedMem->buffer);
		}
	}
}
