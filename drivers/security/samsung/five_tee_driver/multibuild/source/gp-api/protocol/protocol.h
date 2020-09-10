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

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <linux/types.h>

#define PROTOCOL_COMMAND_UNLOAD 0xFF01
#define PROTOCOL_COMMAND_LOAD   0xFF02
#define PROTOCOL_COMMAND_READF  0xFF03
#define PROTOCOL_COMMAND_WRITEF 0xFF04

/*
 * Size of command buffer in Linux kernel for the case of kinibi must be less
 * than 32 KBytes (8 pages):
 *  | struct ProcolCmd | struct PersObjectCmd |
 *  | 20544 bytes      | 4180 bytes           | = 24724 bytes (7 pages)
 *
 * The size of persistent object data should be comparable to the size of one
 * command parameter.
 */
#define PROTOCOL_MAX_PARAM_DATA_LEN    (4096 + 1024)
#define PARAMS_NUM 4
#define MAX_SHAREDMEM_SIZE (PARAMS_NUM * PROTOCOL_MAX_PARAM_DATA_LEN)

#define MAX_PERSISTENT_OBJECT_DATA_LEN 4096
#define MAX_PERSISTENT_OBJECT_ID_LEN 64

/*
 * All structures which travels across IW boundary must be packed
 */
#define IW_STRUCTURE __attribute__((packed))

//TEE_Result is the type used for return codes from the APIs.
typedef uint32_t TEE_Result;

typedef union {
	struct {
		uint32_t buffer;
		uint32_t size;
	} IW_STRUCTURE memref;
	struct {
		uint32_t a, b;
	} IW_STRUCTURE value;
} IW_STRUCTURE ProtocolParam;

/*
 * Size of ProtocolCmdStruct should be aligned to size_t, so we added
 * dummy field to keep alignment
 */
typedef struct ProtocolCmdStruct {
	uint32_t cmd_id;
	uint32_t cmd_ret_origin;
	uint32_t param_types;
	ProtocolParam params[PARAMS_NUM];
	uint8_t param_buffers[PARAMS_NUM][PROTOCOL_MAX_PARAM_DATA_LEN];
	TEE_Result cmd_ret;
	uint32_t return_origin;
	uint32_t dummy;
	uint64_t teec_oper;
} IW_STRUCTURE ProtocolCmd;

typedef struct PersObjectCmdStruct {
	uint32_t cmd_id;
	uint32_t data_len;
	uint32_t id_len;
	uint8_t  id[MAX_PERSISTENT_OBJECT_ID_LEN];
	uint8_t  data[MAX_PERSISTENT_OBJECT_DATA_LEN];
	uint64_t teec_oper;
} IW_STRUCTURE PersObjectCmd;

#endif /* !PROTOCOL_H */
