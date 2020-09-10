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
#include <mobicore_driver_api.h>
#include <linux/slab.h>
#include <linux/firmware.h>

#include "teec_common.h"
#include "protocol.h"
#include "teec_param_utils.h"

#define DEVICE_ID MC_DEVICE_ID_DEFAULT
#define MC_SPID_SYSTEM 0xFFFFFFFE

typedef struct TbaseSessionMapInfoStruct {
	void *src_address;
	struct mc_bulk_map map_info;
} TbaseSessionMapInfo;

typedef struct TbaseSessionStruct {
	struct mc_session_handle ta_session;
	ProtocolCmd *tci_buffer;
	PersObjectCmd *persistent_object;
	TbaseSessionMapInfo maps[PARAMS_NUM];
	void *parent_context;
} TbaseSession;

static TEEC_Result ReadPersistentObjectFromFS(TbaseSession *session,
					      const uint8_t *id,
					      uint8_t *data,
					      uint32_t *data_len)
{
	TEEC_Result result = TEEC_ERROR_NOT_SUPPORTED;
	return result;
}

static TEEC_Result WritePersistentObjectToFS(TbaseSession *session,
					     const uint8_t *id,
					     const uint8_t *data,
					     uint32_t data_len)
{
	TEEC_Result result = TEEC_ERROR_NOT_SUPPORTED;
	return result;
}

static void TbaseInitSessionMaps(TbaseSession *session)
{
	if (session)
		memset(session->maps, 0x00, sizeof(session->maps));
}

static void TbaseCleanSessionMaps(TbaseSession *session)
{
	int i = 0;

	if (session) {
		for (i = 0; i < PARAMS_NUM; i++) {
			if (session->maps[i].src_address) {
				mc_unmap(&(session->ta_session),
					 session->maps[i].src_address,
					 &(session->maps[i].map_info));
			}
		}
		memset(session->maps, 0x00, sizeof(session->maps));
	}
}

TEEC_Result TbaseSendCommand(void *ta_session,
			     uint32_t command_id,
			     TEEC_Operation *operation,
			     uint32_t *return_origin,
			     int32_t timeout)
{
	enum mc_result mc_res = MC_DRV_OK;
	TEEC_Result ret = TEEC_ERROR_BAD_PARAMETERS;
	TbaseSession *tbase_session = (TbaseSession *)ta_session;

	if (!tbase_session)
		goto exit;

	/* tci_buffer is supposed to be not NULL */
	tbase_session->tci_buffer->return_origin = TEEC_ORIGIN_API;

	tbase_session->tci_buffer->cmd_id = command_id;
	tbase_session->tci_buffer->param_types = 0;
	tbase_session->tci_buffer->cmd_ret = TEEC_ERROR_COMMUNICATION;

	TbaseInitSessionMaps(tbase_session);

	if (operation) {
		tbase_session->tci_buffer->param_types = operation->paramTypes;
		ret = FillCommandArgs(tbase_session->tci_buffer,
				      operation, tbase_session);

		if (ret != TEEC_SUCCESS)
			goto exit;
	}

	do {
		/* Clear PO command! */
		tbase_session->persistent_object->cmd_id = 0x00;

		/* Call */
		mc_res = mc_notify(&tbase_session->ta_session);

		/* Return origin: "COMMS" to handle several conditions below */
		tbase_session->tci_buffer->return_origin = TEEC_ORIGIN_COMMS;

		if (mc_res != MC_DRV_OK) {
			ret = TEEC_ERROR_COMMUNICATION;
			goto exit;
		}

		mc_res = mc_wait_notification(&tbase_session->ta_session,
				timeout);

		if (mc_res != MC_DRV_OK) {
			ret = TEEC_ERROR_COMMUNICATION;
			goto exit;
		}

		/* Return origin: "TRUSTED_APP" to handle PO command emulation */
		tbase_session->tci_buffer->return_origin =
							TEEC_ORIGIN_TRUSTED_APP;

		if (tbase_session->persistent_object->cmd_id) {
			switch (tbase_session->persistent_object->cmd_id) {
			case PROTOCOL_COMMAND_READF: {
				uint32_t cmd_ret;

				cmd_ret = ReadPersistentObjectFromFS(
				  tbase_session,
				  tbase_session->persistent_object->id,
				  tbase_session->persistent_object->data,
				  &tbase_session->persistent_object->data_len);

				tbase_session->tci_buffer->cmd_ret = cmd_ret;
				break;
			}
			case PROTOCOL_COMMAND_WRITEF: {
				uint32_t cmd_ret;

				cmd_ret = WritePersistentObjectToFS(
				  tbase_session,
				  tbase_session->persistent_object->id,
				  tbase_session->persistent_object->data,
				  tbase_session->persistent_object->data_len);

				tbase_session->tci_buffer->cmd_ret = cmd_ret;
			break;
			}
			default: {
				uint32_t cmd_ret = TEEC_ERROR_NOT_SUPPORTED;

				tbase_session->tci_buffer->cmd_ret = cmd_ret;
				break;
			}
			}
		}

	} while (tbase_session->persistent_object->cmd_id);

	/* Return origin: "TEE" or "TRUSTED_APP" after communication with TEE. */

	/* On Success: */
	if (return_origin)
		*return_origin = TEEC_ORIGIN_TRUSTED_APP;

	ret = tbase_session->tci_buffer->cmd_ret;

	if (ret == TEEC_SUCCESS && operation) {
		ret = FillOperationArgs(operation, tbase_session->tci_buffer);
		if (ret != TEEC_SUCCESS)
			tbase_session->tci_buffer->return_origin =
								TEEC_ORIGIN_API;
	}

exit:
	if (return_origin && tbase_session) {
		/* tci_buffer is supposed to be not NULL */
		*return_origin = tbase_session->tci_buffer->return_origin;
	}

	TbaseCleanSessionMaps(tbase_session);
	return ret;
}

TEEC_Result TbaseUnloadTA(void *ta_session)
{
	enum mc_result mc_res;
	TEEC_Result status = TEEC_SUCCESS;
	TbaseSession *session = (TbaseSession *)ta_session;

	mc_res = mc_close_session(&(session->ta_session));

	if (mc_res != MC_DRV_OK)
		status = TEEC_ERROR_GENERIC;

	if (session->tci_buffer != NULL) {
		mc_res = mc_free_wsm(DEVICE_ID, (uint8_t *)session->tci_buffer);
		if (mc_res != MC_DRV_OK)
			status = TEEC_ERROR_GENERIC;

		session->tci_buffer = NULL;
		session->persistent_object = NULL;
	}

	return status;
}

TEEC_Result TbaseLoadTA(void *ta_session, const TEEC_UUID *uuid)
{
	enum mc_result mc_res;
	TEEC_Result status = TEEC_SUCCESS;
	TbaseSession *session = (TbaseSession *)ta_session;
	void *po_cmd;
	const struct firmware *fw = NULL;

	if (!ta_session || !uuid) {
		status = TEEC_ERROR_BAD_PARAMETERS;
		goto error;
	}

	session->tci_buffer = NULL;
	mc_res = mc_open_device(DEVICE_ID);

	if (mc_res != MC_DRV_OK) {
		status = TEEC_ERROR_GENERIC;
		goto error;
	}

	mc_res = mc_malloc_wsm(DEVICE_ID, 0,
			sizeof(ProtocolCmd) + sizeof(PersObjectCmd),
			(uint8_t **)&session->tci_buffer, 0);
	if (mc_res != MC_DRV_OK) {
		status = TEEC_ERROR_OUT_OF_MEMORY;
		goto error_close;
	}

	po_cmd = (void *)((uint8_t *)session->tci_buffer + sizeof(ProtocolCmd));
	session->persistent_object = (PersObjectCmd *)po_cmd;

	if (request_firmware_direct(&fw, CONFIG_FIVE_TRUSTLET_PATH, NULL)) {
		status = TEEC_ERROR_NO_DATA;
		goto error_free;
	}

	mc_res = mc_open_trustlet(&(session->ta_session),
#if MCDRVMODULEAPI_VERSION_MAJOR < 8
				MC_SPID_SYSTEM,
#endif
				(u8 *)fw->data,
				fw->size,
				(u8 *)session->tci_buffer,
				sizeof(ProtocolCmd) + sizeof(PersObjectCmd));

	if (mc_res != MC_DRV_OK) {
		status = TEEC_ERROR_GENERIC;
		goto error_free;
	}

	release_firmware(fw);

	return status;

error_free:
	release_firmware(fw);
	mc_free_wsm(DEVICE_ID, (uint8_t *)session->tci_buffer);
error_close:
	mc_close_device(DEVICE_ID);
error:
	return status;
}

PlatformClientImpl g_tbase_platform_impl = {
	TbaseLoadTA,
	TbaseUnloadTA,
	TbaseSendCommand,
	NULL,
	NULL,
	NULL,
	NULL
};

PlatformClientImpl *GetPlatformClientImpl(void)
{
	return &g_tbase_platform_impl;
}

size_t GetPlatformClientImplSize(void)
{
	return sizeof(TbaseSession);
}

void ClientImplSetParentContext(void *session, void *context)
{
	TbaseSession *tbase_session = (TbaseSession *)session;

	if (tbase_session)
		tbase_session->parent_context = context;
}

void *ClientImplGetParentContext(void *session)
{
	void *ret = NULL;
	TbaseSession *tbase_session = (TbaseSession *)session;

	if (tbase_session)
		ret = tbase_session->parent_context;

	return ret;
}
