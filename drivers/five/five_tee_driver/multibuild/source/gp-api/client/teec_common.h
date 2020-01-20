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

#ifndef TEEC_COMMON_H_
#define TEEC_COMMON_H_

#include "tee_client_api.h"
#include <linux/types.h>

#define MB_INFINITE_TIMEOUT (-1)

typedef TEEC_Result (*ClientImplLoadTA)(void *session, const TEEC_UUID *uuid);
typedef TEEC_Result (*ClientImplSendCommand)(void *session,
	uint32_t command_id,
	TEEC_Operation *operation, uint32_t *return_origin, int timeout);

typedef TEEC_Result (*ClientImplUnloadTA)(void *session);
typedef void *(*ClientImplAllocateForSession)(void *session, size_t size);
typedef void (*ClientImplFreeForSession)(void *session, void *ptr);
typedef int  (*ClientImplSessionCanRelease)(void *session, void *ptr);

typedef uint32_t (*ClientMapBuffer)(void *session, void *ptr, size_t size);

typedef struct PlatformClientImplStruct {
	ClientImplLoadTA LoadTA;
	ClientImplUnloadTA UnloadTA;
	ClientImplSendCommand SendCommand;
	ClientMapBuffer MapBuffer;
	ClientImplAllocateForSession AllocateBuffer;
	ClientImplFreeForSession FreeBuffer;
	ClientImplSessionCanRelease CanRelease;
} PlatformClientImpl;

PlatformClientImpl *GetPlatformClientImpl(void);
size_t GetPlatformClientImplSize(void);
#endif /* TEEC_COMMON_H_ */
