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

#ifndef TEEC_PARAM_UTILS_H_
#define TEEC_PARAM_UTILS_H_

#include "tee_client_api.h"
#include "protocol.h"

inline uint32_t GetParamType(uint32_t param_index, uint32_t param_types);
TEEC_Result FillCommandArgs(ProtocolCmd *command,
				TEEC_Operation *operation, void *ta_session);
TEEC_Result FillOperationArgs(TEEC_Operation *operation, ProtocolCmd *command);

#endif /* TEEC_PARAM_UTILS_H_ */
