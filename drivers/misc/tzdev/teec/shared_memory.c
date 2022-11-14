/*
 * Copyright (C) 2012-2018, Samsung Electronics Co., Ltd.
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

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/page.h>

#include <tzdev/tee_client_api.h>

#include "iw_messages.h"
#include "misc.h"
#include "types.h"
#include "tz_mem.h"
#include "tzlog.h"

#define PTR_ALIGN_PGDN(p)	((typeof(p))(((uintptr_t)(p)) & PAGE_MASK))

static void tzdev_teec_release_shared_memory(void *data)
{
	struct tzdev_teec_shared_memory *shm = data;
	complete(&shm->released);
}

static uint32_t tzdev_teec_shared_memory_init(TEEC_Context *context, TEEC_SharedMemory *sharedMem,
		struct page **pages, unsigned int num_pages)
{
	struct tzdev_teec_shared_memory *shm;
	uint32_t result;
	void *addr;
	size_t size;
	unsigned int offset;
	int ret;

	tzdev_teec_debug("Enter, context = %pK sharedMem = %pK buffer = %pK size = %zu flags = %x\n",
			context, sharedMem, sharedMem->buffer, sharedMem->size, sharedMem->flags);

	shm = kmalloc(sizeof(struct tzdev_teec_shared_memory), GFP_KERNEL);
	if (!shm) {
		tzdev_teec_error("Failed to allocate shared memory struct\n");
		result = TEEC_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	addr = PTR_ALIGN_PGDN(sharedMem->buffer);
	offset = (unsigned int)OFFSET_IN_PAGE((uintptr_t)sharedMem->buffer);
	size = PAGE_ALIGN(sharedMem->size + offset);

	shm->id = 0;

	if (sharedMem->size) {
		ret = tzdev_mem_register(addr, size,
				(sharedMem->flags & TEEC_MEM_OUTPUT) ? 1 : 0,
				tzdev_teec_release_shared_memory, shm);
		if (ret < 0) {
			tzdev_teec_error("Failed to register shared memory, ret = %d\n", ret);
			result = tzdev_teec_error_to_tee_error(ret);
			kfree(shm);
			goto out;
		}

		shm->id = ret;
	}

	init_completion(&shm->released);
	shm->context = context;
	shm->pages = pages;
	shm->num_pages = num_pages;
	shm->offset = offset;

	sharedMem->imp = shm;

	result = TEEC_SUCCESS;

out:
	tzdev_teec_debug("Exit, context = %pK sharedMem = %pK buffer = %pK size = %zu flags = %x\n",
			context, sharedMem, sharedMem->buffer, sharedMem->size, sharedMem->flags);

	return result;
}

static void tzdev_teec_shared_memory_fini(TEEC_SharedMemory *sharedMem)
{
	struct tzdev_teec_shared_memory *shm = sharedMem->imp;

	tzdev_teec_debug("Enter, sharedMem = %pK id = %u\n", sharedMem, shm->id);

	sharedMem->imp = NULL;

	if (shm->id) {
		BUG_ON(tzdev_mem_release(shm->id));
		wait_for_completion(&shm->released);
	}

	kfree(shm);

	tzdev_teec_debug("Exit, sharedMem = %pK\n", sharedMem);
}

static uint32_t tzdev_teec_grant_shared_memory_rights(TEEC_SharedMemory *sharedMem, uint32_t *origin)
{
	struct tzdev_teec_shared_memory *shm = sharedMem->imp;
	struct tzdev_teec_context *ctx = shm->context->imp;
	struct cmd_set_shared_memory_rights cmd;
	struct cmd_reply_set_shared_memory_rights ack;
	uint32_t result;
	int ret;

	tzdev_teec_debug("Enter, sharedMem = %pK\n", sharedMem);

	*origin = TEEC_ORIGIN_API;

	cmd.base.cmd = CMD_SET_SHMEM_RIGHTS;
	cmd.base.serial = ctx->serial;
	cmd.buf_desc.offset = shm->offset;
	cmd.buf_desc.size = (unsigned int)(sharedMem->size);
	cmd.buf_desc.id = shm->id;

	ret = tzdev_teec_send_then_recv(ctx->socket,
			&cmd, sizeof(cmd), 0x0,
			&ack, sizeof(ack), 0x0,
			&result, origin);
	if (ret < 0) {
		tzdev_teec_error("Failed to xmit set shmem rights, ret = %d\n", ret);
		goto out;
	}

	ret = tzdev_teec_check_reply(&ack.base, CMD_REPLY_SET_SHMEM_RIGHTS,
			ctx->serial, &result, origin);
	if (ret) {
		tzdev_teec_error("Failed to check set shmem rights reply, ret = %d\n", ret);
		goto out;
	}

	result = ack.base.result;
	*origin = ack.base.origin;

out:
	ctx->serial++;

	tzdev_teec_fixup_origin(result, origin);

	tzdev_teec_debug("Exit, sharedMem = %pK\n", sharedMem);

	return result;
}

static uint32_t tzdev_teec_register_shared_memory(TEEC_Context *context,
		TEEC_SharedMemory *sharedMem, struct page **pages,
		unsigned int num_pages, uint32_t *origin)
{
	struct tzdev_teec_context *ctx = context->imp;
	uint32_t result;

	tzdev_teec_debug("Enter, context = %pK sharedMem = %pK\n", context, sharedMem);

	*origin = TEEC_ORIGIN_API;

	result = tzdev_teec_shared_memory_init(context, sharedMem, pages, num_pages);
	if (result != TEEC_SUCCESS) {
		tzdev_teec_error("Failed to create shared memory, context = %pK sharedMem = %pK\n",
				context, sharedMem);
		goto out;
	}

	if (sharedMem->size) {
		mutex_lock(&ctx->mutex);
		result = tzdev_teec_grant_shared_memory_rights(sharedMem, origin);
		mutex_unlock(&ctx->mutex);

		if (result != TEEC_SUCCESS) {
			tzdev_teec_error("Failed to grant shared memory rights, context = %pK sharedMem = %pK\n",
					context, sharedMem);
			tzdev_teec_shared_memory_fini(sharedMem);
			goto out;
		}
	}

out:
	tzdev_teec_fixup_origin(result, origin);

	tzdev_teec_debug("Exit, context = %pK sharedMem = %pK\n", context, sharedMem);

	return result;
}

static uint32_t tzdev_teec_shared_memory_check_args(TEEC_Context *context,
		TEEC_SharedMemory *sharedMem, bool null_buffer)
{
	if (!sharedMem) {
		tzdev_teec_error("Null shared memory passed\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (null_buffer)
		sharedMem->buffer = NULL;

	if (!context) {
		tzdev_teec_error("Null context passed\n");
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (sharedMem->size > TEEC_CONFIG_SHAREDMEM_MAX_SIZE) {
		tzdev_teec_error("Too big shared memory requested, size = %zu max = %u\n",
				sharedMem->size, TEEC_CONFIG_SHAREDMEM_MAX_SIZE);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (sharedMem->flags & ~(TEEC_MEM_INPUT | TEEC_MEM_OUTPUT)) {
		tzdev_teec_error("Invalid flags passed, flags = %x\n", sharedMem->flags);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

	if (!sharedMem->buffer && !null_buffer) {
		tzdev_teec_error("No buffer provided for shared memory\n");
		return TEEC_ERROR_NO_DATA;
	}

	return TEEC_SUCCESS;
}

TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context, TEEC_SharedMemory *sharedMem)
{
	struct tzdev_teec_shared_memory *shm;
	uint32_t result;
	uint32_t origin;

	tzdev_teec_debug("Enter, context = %pK sharedMem = %pK\n", context, sharedMem);

	origin = TEEC_ORIGIN_API;

	result = tzdev_teec_shared_memory_check_args(context, sharedMem, false);
	if (result != TEEC_SUCCESS)
		goto out;

	result = tzdev_teec_register_shared_memory(context, sharedMem, NULL, 0, &origin);
	if (result != TEEC_SUCCESS)
		goto out;

	shm = sharedMem->imp;

	tzdev_teec_debug("Success, context = %pK sharedMem = %pK id = %u\n",
			context, sharedMem, shm->id);

out:
	tzdev_teec_debug("Exit, context = %pK sharedMem = %pK result = %x origin = %u\n",
			context, sharedMem, result, origin);

	return result;
}

TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context, TEEC_SharedMemory *sharedMem)
{
	struct tzdev_teec_shared_memory *shm;
	struct page **pages;
	unsigned int num_pages;
	unsigned int i, j;
	uint32_t result;
	uint32_t origin;

	tzdev_teec_debug("Enter, context = %pK sharedMem = %pK\n", context, sharedMem);

	origin = TEEC_ORIGIN_API;

	result = tzdev_teec_shared_memory_check_args(context, sharedMem, true);
	if (result != TEEC_SUCCESS)
		goto out;

	num_pages = (unsigned int)NUM_PAGES(sharedMem->size);
	if (!num_pages)
		num_pages = 1;

	pages = kcalloc(num_pages, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		tzdev_teec_error("Failed to allocate pages for shared memory\n");
		result = TEEC_ERROR_OUT_OF_MEMORY;
		goto out;
	}

	for (i = 0; i < num_pages; ++i) {
		pages[i] = alloc_page(GFP_KERNEL);
		if (!pages[i])
			goto out_free_pages;
	}

	sharedMem->buffer = vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);
	if (!sharedMem->buffer) {
		tzdev_teec_error("Failed to vmap shared memory pages\n");
		result = TEEC_ERROR_OUT_OF_MEMORY;
		goto out_free_pages;
	}

	result = tzdev_teec_register_shared_memory(context, sharedMem, pages, num_pages, &origin);
	if (result != TEEC_SUCCESS)
		goto out_unmap_buffer;

	shm = sharedMem->imp;

	tzdev_teec_debug("Success, context = %pK sharedMem = %pK id = %u\n",
			context, sharedMem, shm->id);
	goto out;

out_unmap_buffer:
	vunmap(sharedMem->buffer);
	sharedMem->buffer = NULL;
out_free_pages:
	for (j = 0; j < i; ++j)
		__free_page(pages[j]);
	kfree(pages);
out:
	tzdev_teec_debug("Exit, context = %pK sharedMem = %pK result = %x origin = %u\n",
			context, sharedMem, result, origin);

	return result;
}

void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMem)
{
	struct tzdev_teec_shared_memory *shm;
	unsigned int i;
	uint32_t result = TEEC_SUCCESS;
	uint32_t origin;

	tzdev_teec_debug("Enter, sharedMem = %pK\n", sharedMem);

	origin = TEEC_ORIGIN_API;

	if (!sharedMem || !sharedMem->imp) {
		tzdev_teec_error("Null shared memory passed\n");
		result = TEEC_ERROR_BAD_PARAMETERS;
		goto out;
	}

	shm = sharedMem->imp;
	if (shm->num_pages) {
		vunmap(sharedMem->buffer);

		for (i = 0; i < shm->num_pages; ++i)
			__free_page(shm->pages[i]);

		kfree(shm->pages);

		sharedMem->buffer = NULL;
		sharedMem->size = 0;
	}

	tzdev_teec_shared_memory_fini(sharedMem);

out:
	tzdev_teec_debug("Exit, sharedMem = %pK result = %x origin = %u\n",
			sharedMem, result, origin);
}
