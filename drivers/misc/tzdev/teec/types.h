/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
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

#ifndef __TZDEV_TEEC_TYPES_H__
#define __TZDEV_TEEC_TYPES_H__

#include <linux/completion.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>

#include <tzdev/tee_client_api.h>

#include "misc.h"

struct tzdev_teec_context {
	struct mutex mutex;
	struct sock_desc *socket;
	unsigned int serial;
	unsigned int id;
};

struct tzdev_teec_tmpref {
	TEEC_SharedMemory shmem;
	bool exists;
};

struct tzdev_teec_session {
	TEEC_Context *context;
	struct tzdev_teec_tmpref tmpref[MAX_PARAM_COUNT];
	struct mutex mutex;
	struct completion cancel;
	struct sock_desc *socket;
	unsigned int serial;
};

struct tzdev_teec_shared_memory {
	TEEC_Context *context;
	struct completion released;
	struct page **pages;
	unsigned int num_pages;
	unsigned int offset;
	unsigned int id;
};

struct tzdev_teec_operation {
	TEEC_Operation *operation;
	struct list_head link;
};

#endif /* __TZDEV_TEEC_TYPES_H__ */
