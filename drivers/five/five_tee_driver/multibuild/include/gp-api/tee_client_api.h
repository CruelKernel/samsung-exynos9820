/*
 * tee_client_api.h
 *
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
 *
 * TEE client library
 */

#ifndef __TEE_CLIENT_API_H__
#define __TEE_CLIENT_API_H__

#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TEE Client API types and constants definitions */
#ifndef __TEEC_ERROR_CODES__
#define __TEEC_ERROR_CODES__
// The operation was successful
#define TEEC_SUCCESS			0x00000000
// Non-specific cause
#define TEEC_ERROR_GENERIC		0xFFFF0000
// Access privileges are not sufficient
#define TEEC_ERROR_ACCESS_DENIED	0xFFFF0001
// The operation was cancelled
#define TEEC_ERROR_CANCEL		0xFFFF0002
// Concurrent accesses caused conflict
#define TEEC_ERROR_ACCESS_CONFLICT	0xFFFF0003
// Too much data for the requested operation was passed
#define TEEC_ERROR_EXCESS_DATA		0xFFFF0004
// Input data was of invalid format
#define TEEC_ERROR_BAD_FORMAT		0xFFFF0005
// Input parameters were invalid
#define TEEC_ERROR_BAD_PARAMETERS	0xFFFF0006
// Operation is not valid in the current state
#define TEEC_ERROR_BAD_STATE		0xFFFF0007
// The requested data item is not found
#define TEEC_ERROR_ITEM_NOT_FOUND	0xFFFF0008
// The requested operation should exist but is not yet implemented
#define TEEC_ERROR_NOT_IMPLEMENTED	0xFFFF0009
// The requested operation is valid but is not supported in this Implementation
#define TEEC_ERROR_NOT_SUPPORTED	0xFFFF000A
// Expected data was missing
#define TEEC_ERROR_NO_DATA		0xFFFF000B
// System ran out of resources
#define TEEC_ERROR_OUT_OF_MEMORY	0xFFFF000C
// The system is busy working on something else.
#define TEEC_ERROR_BUSY			0xFFFF000D
// Communication with a remote party failed.
#define TEEC_ERROR_COMMUNICATION	0xFFFF000E
// A security fault was detected.
#define TEEC_ERROR_SECURITY		0xFFFF000F
// The supplied buffer is too short for the generated output.
#define TEEC_ERROR_SHORT_BUFFER		0xFFFF0010

#define TEEC_ERROR_TARGET_DEAD		0xFFFF3024 // Targed TA panic'ed
#define TEEC_ERROR_CORRUPT_OBJECT	0xF0100001
#define TEEC_ERROR_STORAGE_NOT_AVAILABLE 0xF0100003
#define TEEC_ERROR_SIGNATURE_INVALID	0xFFFF3072 // Invalid TA signature
#define TEEC_IMP_MAX			0xFFFEFFFF
#define TEEC_RFU_MIN			0xFFFF0011
#define TEEC_RFU_MAX			0xFFFFFFFF

/* Implementation defined error codes */
#define TEEC_IMP_MIN			0x00000001
// Requested FIPS on/off state failed
#define TEEC_ERROR_BAD_FIPS_MODE	0x80000004
#define TEEC_ERROR_TA_AUTHORITY_UNKNOWN	0x80000005 // TA AUthority is unknown.
#define TEEC_ERROR_DEVICE_ACCESS_DENIED	0x80000007

#endif

#define TEEC_ORIGIN_API		0x1
#define TEEC_ORIGIN_COMMS	0x2
#define TEEC_ORIGIN_TEE		0x3
#define TEEC_ORIGIN_TRUSTED_APP	0x4

typedef uint32_t TEEC_Result;

#ifndef __TEEC_UUID__
#define __TEEC_UUID__
/*
 * Althought this structure is not traveling between worlds, but it used in
 * tzdaemon, so try to keep TEE_UUID and TEEC_UUID identical.
 */
typedef struct {
	uint32_t timeLow;
	uint16_t timeMid;
	uint16_t timeHiAndVersion;
	uint8_t clockSeqAndNode[8];
} __attribute__((packed)) TEEC_UUID;
#endif

typedef struct {
	void *imp;
} TEEC_Context;

typedef struct {
	void *imp;
} TEEC_Session;

#define TEEC_CONFIG_SHAREDMEM_MAX_SIZE 0x040000

#define TEEC_MEM_INPUT	(1 << 0)
#define TEEC_MEM_OUTPUT	(1 << 1)

#define TEEC_NONE			0x00000000
#define TEEC_VALUE_INPUT		0x00000001
#define TEEC_VALUE_OUTPUT		0x00000002
#define TEEC_VALUE_INOUT		0x00000003
#define TEEC_MEMREF_TEMP_INPUT		0x00000005
#define TEEC_MEMREF_TEMP_OUTPUT		0x00000006
#define TEEC_MEMREF_TEMP_INOUT		0x00000007

#define TEEC_SHMEM_IMP_NONE		0x00000000
#define TEEC_SHMEM_IMP_ALLOCED	0x00000001

typedef struct {
	TEEC_Context *context;
	uint32_t flags;
} TEEC_SharedMemoryImp;

typedef struct {
	void *buffer;
	size_t size;
	uint32_t flags;
	TEEC_SharedMemoryImp imp;	/* See above */
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

#define TEEC_LOGIN_PUBLIC		0x00000000
#define TEEC_LOGIN_USER			0x00000001
#define TEEC_LOGIN_GROUP		0x00000002
#define TEEC_LOGIN_APPLICATION		0x00000004
#define TEEC_LOGIN_USER_APPLICATION	0x00000005
#define TEEC_LOGIN_GROUP_APPLICATION	0x00000006
#define TEEC_LOGIN_IMP_MIN		0x80000000
#define TEEC_LOGIN_IMP_MAX		0xEFFFFFFF

typedef struct {
		uint32_t started;
		uint32_t paramTypes;
		TEEC_Parameter params[4];
		void *imp;
} TEEC_Operation;

/* This is protection from already implemented GP interface: */
#define TEEC_InitializeContext MultibuildInternal_TEEC_InitializeContext
#define TEEC_FinalizeContext MultibuildInternal_TEEC_FinalizeContext
#define TEEC_OpenSession MultibuildInternal_TEEC_OpenSession
#define TEEC_CloseSession MultibuildInternal_TEEC_CloseSession
#define TEEC_InvokeCommand MultibuildInternal_TEEC_InvokeCommand
#define TEEC_RegisterSharedMemory MultibuildInternal_TEEC_RegisterSharedMemory
#define TEEC_AllocateSharedMemory MultibuildInternal_TEEC_AllocateSharedMemory
#define TEEC_ReleaseSharedMemory MultibuildInternal_TEEC_ReleaseSharedMemory

/* TEE Client API functions and macros definitions */

TEEC_Result TEEC_InitializeContext(const char *name,
				   TEEC_Context *context);

void TEEC_FinalizeContext(TEEC_Context *context);

TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context,
				      TEEC_SharedMemory *sharedMem);

TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context,
				      TEEC_SharedMemory *sharedMem);

void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMem);

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
			     TEEC_Session *session,
			     const TEEC_UUID *destination,
			     uint32_t connectionMethod,
			     const void *connectionData,
			     TEEC_Operation *operation,
			     uint32_t *returnOrigin);

void TEEC_CloseSession(TEEC_Session *session);

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin);

#define TEEC_PARAM_TYPES(param0Type, param1Type, param2Type, param3Type) \
		(uint32_t)(((param0Type) & 0xf) | \
		(((param1Type) & 0xf) << 4) | \
		(((param2Type) & 0xf) << 8) | \
		(((param3Type) & 0xf) << 12))

#ifdef __cplusplus
}
#endif

#endif /* __TEE_CLIENT_API_H__ */
