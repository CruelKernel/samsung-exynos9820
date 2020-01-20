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

#ifndef __TEE_CLIENT_API_H__
#define __TEE_CLIENT_API_H__

#include <linux/types.h>

#define TEEC_CONFIG_SHAREDMEM_MAX_SIZE		0x1000000

/* standard error codes */
#define TEEC_SUCCESS				0x00000000
#define TEEC_ERROR_GENERIC			0xFFFF0000
#define TEEC_ERROR_ACCESS_DENIED		0xFFFF0001
#define TEEC_ERROR_CANCEL			0xFFFF0002
#define TEEC_ERROR_ACCESS_CONFLICT		0xFFFF0003
#define TEEC_ERROR_EXCESS_DATA			0xFFFF0004
#define TEEC_ERROR_BAD_FORMAT			0xFFFF0005
#define TEEC_ERROR_BAD_PARAMETERS		0xFFFF0006
#define TEEC_ERROR_BAD_STATE			0xFFFF0007
#define TEEC_ERROR_ITEM_NOT_FOUND		0xFFFF0008
#define TEEC_ERROR_NOT_IMPLEMENTED		0xFFFF0009
#define TEEC_ERROR_NOT_SUPPORTED		0xFFFF000A
#define TEEC_ERROR_NO_DATA			0xFFFF000B
#define TEEC_ERROR_OUT_OF_MEMORY		0xFFFF000C
#define TEEC_ERROR_BUSY				0xFFFF000D
#define TEEC_ERROR_COMMUNICATION		0xFFFF000E
#define TEEC_ERROR_SECURITY			0xFFFF000F
#define TEEC_ERROR_SHORT_BUFFER			0xFFFF0010
#define TEEC_ERROR_TARGET_DEAD			0xFFFF3024
#define TEEC_ERROR_SIGNATURE_INVALID		0xFFFF3072
/* implementation-defined error codes */
#define TEEC_ERROR_BAD_FIPS_MODE		0x80000004
#define TEEC_ERROR_TA_AUTHORITY_UNKNOWN		0x80000005
#define TEEC_ERROR_DEVICE_ACCESS_DENIED		0x80000007
#define TEEC_ERROR_CORRUPT_OBJECT		0xF0100001
#define TEEC_ERROR_STORAGE_NOT_AVAILABLE	0xF0100003

#define TEEC_ORIGIN_API				0x00000001
#define TEEC_ORIGIN_COMMS			0x00000002
#define TEEC_ORIGIN_TEE				0x00000003
#define TEEC_ORIGIN_TRUSTED_APP			0x00000004

#define TEEC_MEM_INPUT				0x00000001
#define TEEC_MEM_OUTPUT				0x00000002

#define TEEC_NONE				0x00000000
#define TEEC_VALUE_INPUT			0x00000001
#define TEEC_VALUE_OUTPUT			0x00000002
#define TEEC_VALUE_INOUT			0x00000003
#define TEEC_MEMREF_TEMP_INPUT			0x00000005
#define TEEC_MEMREF_TEMP_OUTPUT			0x00000006
#define TEEC_MEMREF_TEMP_INOUT			0x00000007
#define TEEC_MEMREF_WHOLE			0x0000000C
#define TEEC_MEMREF_PARTIAL_INPUT		0x0000000D
#define TEEC_MEMREF_PARTIAL_OUTPUT		0x0000000E
#define TEEC_MEMREF_PARTIAL_INOUT		0x0000000F

#define TEEC_LOGIN_PUBLIC			0x00000000
#define TEEC_LOGIN_USER				0x00000001
#define TEEC_LOGIN_GROUP			0x00000002
#define TEEC_LOGIN_APPLICATION			0x00000004
#define TEEC_LOGIN_USER_APPLICATION		0x00000005
#define TEEC_LOGIN_GROUP_APPLICATION		0x00000006

#define TEEC_PARAM_TYPES(p0, p1, p2, p3)	\
	((uint32_t)(((p0) & 0xF)		\
	| (((p1) & 0xF) << 4)			\
	| (((p2) & 0xF) << 8)			\
	| (((p3) & 0xF) << 12)))

typedef uint32_t TEEC_Result;

typedef struct {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[8];
} TEEC_UUID;

typedef struct {
	void *imp;
} TEEC_Context;

typedef struct {
	void *imp;
} TEEC_Session;

typedef struct {
	void *buffer;
	size_t size;
	uint32_t flags;
	void *imp;
} TEEC_SharedMemory;

typedef struct {
	void *buffer;
	size_t size;
} TEEC_TempMemoryReference;

typedef struct {
	TEEC_SharedMemory *parent;
	size_t size;
	size_t offset;
} TEEC_RegisteredMemoryReference;

typedef struct {
	uint32_t a;
	uint32_t b;
} TEEC_Value;

typedef union {
	TEEC_TempMemoryReference tmpref;
	TEEC_RegisteredMemoryReference memref;
	TEEC_Value value;
} TEEC_Parameter;

typedef struct {
	uint32_t started;
	uint32_t paramTypes;
	TEEC_Parameter params[4];
	void *imp;
} TEEC_Operation;

TEEC_Result TEEC_InitializeContext(
		const char *name,
		TEEC_Context *context);

void TEEC_FinalizeContext(
		TEEC_Context *context);

TEEC_Result TEEC_RegisterSharedMemory(
		TEEC_Context *context,
		TEEC_SharedMemory *sharedMem);

TEEC_Result TEEC_AllocateSharedMemory(
		TEEC_Context *context,
		TEEC_SharedMemory *sharedMem);

void TEEC_ReleaseSharedMemory(
		TEEC_SharedMemory *sharedMem);

TEEC_Result TEEC_OpenSession(
		TEEC_Context *context,
		TEEC_Session *session,
		const TEEC_UUID *destination,
		uint32_t connectionMethod,
		void *connectionData,
		TEEC_Operation *operation,
		uint32_t *returnOrigin);

TEEC_Result TEECS_OpenSession(
		TEEC_Context *context,
		TEEC_Session *session,
		const TEEC_UUID *destination,
		const void *src_ta_image,
		const size_t src_image_size,
		uint32_t connectionMethod,
		void *connectionData,
		TEEC_Operation *operation,
		uint32_t *returnOrigin);

void TEEC_CloseSession(
		TEEC_Session *session);

TEEC_Result TEEC_InvokeCommand(
		TEEC_Session *session,
		uint32_t commandID,
		TEEC_Operation *operation,
		uint32_t *returnOrigin);

void TEEC_RequestCancellation(
		TEEC_Operation *operation);

#endif /* __TEE_CLIENT_API_H__ */
