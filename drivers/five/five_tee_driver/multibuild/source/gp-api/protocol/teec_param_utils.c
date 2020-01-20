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
#include <linux/string.h>
#include <linux/kernel.h>
#include "teec_param_utils.h"
#include "teec_common.h"
#include "protocol.h"

static inline uint32_t GetParamTypeByFlags(uint32_t flags)
{
	uint32_t param_type = 0;

	if ((flags & TEEC_MEM_INPUT) && (flags & TEEC_MEM_OUTPUT))
		param_type = TEEC_MEMREF_TEMP_INOUT;
	else if (flags & TEEC_MEM_INPUT)
		param_type = TEEC_MEMREF_TEMP_INPUT;
	else if (flags & TEEC_MEM_OUTPUT)
		param_type = TEEC_MEMREF_TEMP_OUTPUT;

	return param_type;
}

inline uint32_t GetParamType(uint32_t param_index, uint32_t param_types)
{
	uint32_t mask = 0xF << (param_index * 4);

	return (param_types & mask) >> (param_index * 4);
}

static inline void SetParamType(uint32_t param_index,
				uint32_t *param_types, uint32_t param_type)
{
	uint32_t mask = param_type << (param_index * 4);
	*param_types &= ~(0xF << (param_index * 4));
	*param_types |= mask;
}

TEEC_Result FillCommandArgs(ProtocolCmd *command,
			    TEEC_Operation *operation, void *ta_session)
{
	TEEC_Result ret = TEEC_SUCCESS;
	uint32_t i = 0;

	if (!command || !operation) {
		ret = TEEC_ERROR_BAD_PARAMETERS;
		goto exit;
	}

	for (i = 0; i < PARAMS_NUM; i++) {
		switch (GetParamType(i, operation->paramTypes)) {
		case TEEC_VALUE_INPUT:
		case TEEC_VALUE_INOUT: {
			command->params[i].value.a =
						operation->params[i].value.a;
			command->params[i].value.b =
						operation->params[i].value.b;
			break;
		}
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_MEMREF_TEMP_INOUT: {
			if (operation->params[i].tmpref.size > UINT_MAX ||
				operation->params[i].tmpref.size >
					sizeof(command->param_buffers[i])) {
				ret = TEEC_ERROR_EXCESS_DATA;
				goto exit;
			}

			command->params[i].memref.buffer = 0;
			memcpy((void *)command->param_buffers[i],
					operation->params[i].tmpref.buffer,
					operation->params[i].tmpref.size);

			command->params[i].memref.size =
				(uint32_t)operation->params[i].tmpref.size;
			break;
		}
		case TEEC_MEMREF_TEMP_OUTPUT: {
			if (operation->params[i].tmpref.size > UINT_MAX ||
				operation->params[i].tmpref.size >
					sizeof(command->param_buffers[i])) {
				ret = TEEC_ERROR_EXCESS_DATA;
				goto exit;
			}
			command->params[i].memref.buffer = 0;
			command->params[i].memref.size =
				(uint32_t)operation->params[i].tmpref.size;
			break;
		}
		case TEEC_NONE:
		case TEEC_VALUE_OUTPUT: {
			break;
		}
		default: {
			ret = TEEC_ERROR_NOT_SUPPORTED;
			goto exit;
		}
		}
	}

exit:
	return ret;
}

TEEC_Result FillOperationArgs(TEEC_Operation *operation, ProtocolCmd *command)
{
	TEEC_Result ret = TEEC_SUCCESS;
	uint32_t i = 0;

	if (!operation || !command) {
		ret = TEEC_ERROR_BAD_PARAMETERS;
		goto exit;
	}

	for (i = 0; i < PARAMS_NUM; i++) {
		switch (GetParamType(i, operation->paramTypes)) {
		case TEEC_VALUE_OUTPUT:
		case TEEC_VALUE_INOUT: {
			operation->params[i].value.a =
						command->params[i].value.a;
			operation->params[i].value.b =
						command->params[i].value.b;
			break;
		}
		case TEEC_MEMREF_TEMP_OUTPUT:
		case TEEC_MEMREF_TEMP_INOUT: {
			if (command->params[i].memref.size <=
					operation->params[i].tmpref.size) {
				memcpy(operation->params[i].tmpref.buffer,
						command->param_buffers[i],
						command->params[i].memref.size);
			} else {
				ret = TEEC_ERROR_SHORT_BUFFER;
			}
			operation->params[i].tmpref.size =
						command->params[i].memref.size;
			break;
		}
		case TEEC_NONE:
		case TEEC_MEMREF_TEMP_INPUT:
		case TEEC_VALUE_INPUT: {
			break;
		}
		default: {
			ret = TEEC_ERROR_NOT_SUPPORTED;
			goto exit;
		}
		}
	}

exit:
	return ret;
}
