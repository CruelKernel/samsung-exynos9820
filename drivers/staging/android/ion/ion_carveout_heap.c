/*
 * drivers/staging/android/ion/ion_carveout_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <asm/cacheflush.h>

#include "ion.h"
#include "ion_exynos.h"
#include "ion_debug.h"

#define ION_CARVEOUT_ALLOCATE_FAIL	-1

struct ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	phys_addr_t base;
	size_t size;
	size_t alloc_align;
	unsigned int protection_id;
	bool secure;
	bool untouchable;
};

static phys_addr_t ion_carveout_allocate(struct ion_carveout_heap *heap,
					 unsigned long size)
{
	unsigned long offset = gen_pool_alloc(heap->pool, size);

	if (!offset)
		return ION_CARVEOUT_ALLOCATE_FAIL;

	return offset;
}

static void ion_carveout_free(struct ion_carveout_heap *carveout_heap,
			      phys_addr_t addr, unsigned long size)
{
	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(carveout_heap->pool, addr,
		      ALIGN(size, carveout_heap->alloc_align));
}

static int ion_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size,
				      unsigned long flags)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);
	struct sg_table *table;
	unsigned long aligned_size = ALIGN(size, carveout_heap->alloc_align);
	phys_addr_t paddr;
	int ret;

	if (carveout_heap->untouchable && !(flags & ION_FLAG_PROTECTED)) {
		perrfn("ION_FLAG_PROTECTED needed by untouchable heap %s",
		       heap->name);
		return -EACCES;
	}

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret) {
		perrfn("failed to allocate scatterlist (err %d)", ret);
		goto err_free;
	}

	paddr = ion_carveout_allocate(carveout_heap, aligned_size);
	if (paddr == ION_CARVEOUT_ALLOCATE_FAIL) {
		perrfn("failed to allocate from %s(id %d), size %lu",
		       heap->name, heap->id, size);
		ret = -ENOMEM;
		goto err_free_table;
	}

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), size, 0);
	buffer->sg_table = table;

	if (carveout_heap->secure && (flags & ION_FLAG_PROTECTED)) {
		buffer->priv_virt = ion_buffer_protect_single(
						carveout_heap->protection_id,
						(unsigned int)aligned_size,
						paddr,
						carveout_heap->alloc_align);
		if (IS_ERR(buffer->priv_virt)) {
			ret = PTR_ERR(buffer->priv_virt);
			goto err_prot;
		}
	}

	return 0;
err_prot:
	ion_carveout_free(carveout_heap, paddr, size);
err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	ion_contig_heap_show_buffers(NULL, &carveout_heap->heap,
				     carveout_heap->base, carveout_heap->size);
	return ret;
}

static void ion_carveout_heap_free(struct ion_buffer *buffer)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(buffer->heap, struct ion_carveout_heap, heap);
	struct sg_table *table = buffer->sg_table;
	struct page *page = sg_page(table->sgl);
	phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));
	unsigned long size = buffer->size;
	int unprot_err = 0;

	if (carveout_heap->secure && (buffer->flags & ION_FLAG_PROTECTED)) {
		unprot_err = ion_buffer_unprotect(buffer->priv_virt);
		size = ALIGN(size, carveout_heap->alloc_align);
	}

	if (!unprot_err) {
		if (!carveout_heap->untouchable) {
			ion_heap_buffer_zero(buffer);
			/* free pages in carveout pool should be cache cold */
			if ((buffer->flags & ION_FLAG_CACHED) != 0)
				__flush_dcache_area(page_to_virt(page), size);
		}

		ion_carveout_free(carveout_heap, paddr, size);
	}
	sg_free_table(table);
	kfree(table);
}

static int carveout_heap_map_user(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  struct vm_area_struct *vma)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	if (carveout_heap->untouchable) {
		perrfn("mmap of %s heap unallowed", heap->name);
		return -EACCES;
	}

	return ion_heap_map_user(heap, buffer, vma);
}

static void *carveout_heap_map_kernel(struct ion_heap *heap,
				      struct ion_buffer *buffer)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	if (carveout_heap->untouchable) {
		perrfn("mapping %s heap unallowed", heap->name);
		return ERR_PTR(-EACCES);
	}

	return ion_heap_map_kernel(heap, buffer);
}

static void carveout_heap_query(struct ion_heap *heap,
				struct ion_heap_data *data)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	data->size = (__u32)carveout_heap->size;
	if (carveout_heap->secure)
		data->heap_flags |= ION_HEAPDATA_FLAGS_ALLOW_PROTECTION;
	if (carveout_heap->untouchable)
		data->heap_flags |= ION_HEAPDATA_FLAGS_UNTOUCHABLE;
}

static struct ion_heap_ops carveout_heap_ops = {
	.allocate = ion_carveout_heap_allocate,
	.free = ion_carveout_heap_free,
	.map_user = carveout_heap_map_user,
	.map_kernel = carveout_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.query_heap = carveout_heap_query,
};

static int ion_carveout_heap_debug_show(struct ion_heap *heap,
					struct seq_file *s,
					void *unused)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

	ion_contig_heap_show_buffers(s, &carveout_heap->heap,
				     carveout_heap->base, carveout_heap->size);

	return 0;
}

struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_carveout_heap *carveout_heap;
	int ret;

	struct page *page;
	size_t size;

	page = pfn_to_page(PFN_DOWN(heap_data->base));
	size = heap_data->size;

	if (!heap_data->untouchable) {
		ret = ion_heap_pages_zero(page, size, PAGE_KERNEL);
		if (ret)
			return ERR_PTR(ret);

		__flush_dcache_area(page_to_virt(page), size);
	}

	carveout_heap = kzalloc(sizeof(*carveout_heap), GFP_KERNEL);
	if (!carveout_heap)
		return ERR_PTR(-ENOMEM);

	carveout_heap->pool = gen_pool_create(
				get_order(heap_data->align) + PAGE_SHIFT, -1);
	if (!carveout_heap->pool) {
		kfree(carveout_heap);
		return ERR_PTR(-ENOMEM);
	}
	carveout_heap->base = heap_data->base;
	gen_pool_add(carveout_heap->pool, carveout_heap->base, heap_data->size,
		     -1);
	carveout_heap->heap.ops = &carveout_heap_ops;
	carveout_heap->heap.type = ION_HEAP_TYPE_CARVEOUT;
	carveout_heap->heap.name = kstrndup(heap_data->name,
					    MAX_HEAP_NAME - 1, GFP_KERNEL);
	if (!carveout_heap->heap.name) {
		gen_pool_destroy(carveout_heap->pool);
		kfree(carveout_heap);
		return ERR_PTR(-ENOMEM);
	}
	carveout_heap->size = heap_data->size;
	carveout_heap->alloc_align = heap_data->align;
	carveout_heap->protection_id = heap_data->id;
	carveout_heap->secure = heap_data->secure;
	carveout_heap->untouchable = heap_data->untouchable;
	carveout_heap->heap.debug_show = ion_carveout_heap_debug_show;

	return &carveout_heap->heap;
}
