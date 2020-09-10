/*
 * Set option shared memory and operations for TEE access
 *
 * Copyright (C) 2019 Samsung Electronics, Inc.
 *
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 * Viacheslav Vovchenko <v.vovchenko@samsung.com>
 * Yevgen Kopylov <y.kopylov@samsung.com>
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

#ifndef __LINUX_TEEC_OPERATION_H
#define __LINUX_TEEC_OPERATION_H

#include "tee_client_api.h"

void FillOperationSharedMem(TEEC_SharedMemory *sharedMem,
			  TEEC_Operation *operation, bool inout_direction);

#endif // __LINUX_TEEC_OPERATION_H
