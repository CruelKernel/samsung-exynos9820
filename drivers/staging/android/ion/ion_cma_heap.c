/*
 * drivers/staging/android/ion/ion_cma_heap.c
 *
 * Copyright (C) Linaro 2012
 * Author: <benjamin.gaignard@linaro.org> for ST-Ericsson.
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

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cma.h>
#include <linux/scatterlist.h>
#include <linux/highmem.h>

#include <asm/cacheflush.h>

#include "ion.h"
#include "ion_exynos.h"
#include "ion_debug.h"

struct ion_cma_heap {
	struct ion_heap heap;
	struct cma *cma;
	unsigned int align_order;
	unsigned int protection_id;
	bool secure;
};

#define to_cma_heap(x) container_of(x, struct ion_cma_heap, heap)

/* ION CMA heap operations functions */
static int ion_cma_allocate(struct ion_heap *heap, struct ion_buffer *buffer,
			    unsigned long len,
			    unsigned long flags)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);
	struct sg_table *table;
	struct page *pages;
	unsigned long size = PAGE_ALIGN(len);
	unsigned long align = cma_heap->align_order;
	unsigned long nr_pages = ALIGN(size >> PAGE_SHIFT, 1 << align);
	bool cacheflush = !(flags & ION_FLAG_CACHED) ||
			  ((flags & ION_FLAG_SYNC_FORCE) != 0);
	bool protected = cma_heap->secure && (flags & ION_FLAG_PROTECTED);
	int ret = -ENOMEM;

	pages = cma_alloc(cma_heap->cma, nr_pages, align, GFP_KERNEL);
	if (!pages) {
		perrfn("failed to allocate from %s(id %d), size %lu",
		       cma_heap->heap.name, cma_heap->heap.id, len);
		goto err;
	}

	if (!(flags & ION_FLAG_NOZEROED)) {
		if (PageHighMem(pages)) {
			unsigned long nr_clear_pages = nr_pages;
			struct page *page = pages;

			while (nr_clear_pages > 0) {
				void *vaddr = kmap_atomic(page);

				memset(vaddr, 0, PAGE_SIZE);
				kunmap_atomic(vaddr);
				page++;
				nr_clear_pages--;
			}
		} else {
			memset(page_address(pages), 0, size);
		}
	}

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		goto err_table;

	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret) {
		perrfn("failed to alloc sgtable(err %d)", -ret);
		goto free_mem;
	}

	sg_set_page(table->sgl, pages, size, 0);

	buffer->sg_table = table;

	/*
	 * No need to flush more than the requiered size. But clearing dirty
	 * data from the CPU caches should be performed on the entire area
	 * to be protected because writing back from the CPU caches with non-
	 * secure property to the protected area results system error.
	 */
	if (cacheflush || protected)
		__flush_dcache_area(page_to_virt(pages),
				    nr_pages << PAGE_SHIFT);

	if (protected) {
		buffer->priv_virt = ion_buffer_protect_single(
					cma_heap->protection_id,
					(unsigned int)len,
					page_to_phys(pages),
					PAGE_SIZE << cma_heap->align_order);
		if (IS_ERR(buffer->priv_virt)) {
			ret = PTR_ERR(buffer->priv_virt);
			goto err_prot;
		}
	}

	return 0;
err_prot:
	sg_free_table(buffer->sg_table);
free_mem:
	kfree(table);
err_table:
	cma_release(cma_heap->cma, pages, nr_pages);
err:
	ion_contig_heap_show_buffers(NULL, &cma_heap->heap,
				     cma_get_base(cma_heap->cma),
				     cma_get_size(cma_heap->cma));
	return ret;
}

static void ion_cma_free(struct ion_buffer *buffer)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(buffer->heap);
	bool protected = cma_heap->secure &&
			 (buffer->flags & ION_FLAG_PROTECTED);
	unsigned long nr_pages = ALIGN(PAGE_ALIGN(buffer->size) >> PAGE_SHIFT,
				       1 << cma_heap->align_order);
	int unprot_err = 0;

	if (protected)
		unprot_err = ion_buffer_unprotect(buffer->priv_virt);
	/* release memory */
	if (!unprot_err)
		cma_release(cma_heap->cma, sg_page(buffer->sg_table->sgl),
			    nr_pages);
	/* release sg table */
	sg_free_table(buffer->sg_table);
	kfree(buffer->sg_table);
}

static void cma_heap_query(struct ion_heap *heap, struct ion_heap_data *data)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);

	data->size = (u32)cma_get_size(cma_heap->cma);
	if (cma_heap->secure)
		data->heap_flags |= ION_HEAPDATA_FLAGS_ALLOW_PROTECTION;
}

static struct ion_heap_ops ion_cma_ops = {
	.allocate = ion_cma_allocate,
	.free = ion_cma_free,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.query_heap = cma_heap_query,
};

#ifdef CONFIG_ION_EXYNOS
static int ion_cma_heap_debug_show(struct ion_heap *heap, struct seq_file *s,
				   void *unused)
{
	struct ion_cma_heap *cma_heap = to_cma_heap(heap);

	ion_contig_heap_show_buffers(s, &cma_heap->heap,
				     cma_get_base(cma_heap->cma),
				     cma_get_size(cma_heap->cma));

	return 0;
}

struct ion_heap *ion_cma_heap_create(struct cma *cma,
				     struct ion_platform_heap *heap_data)
{
	struct ion_cma_heap *cma_heap;

	cma_heap = kzalloc(sizeof(*cma_heap), GFP_KERNEL);
	if (!cma_heap)
		return ERR_PTR(-ENOMEM);

	cma_heap->heap.ops = &ion_cma_ops;
	cma_heap->cma = cma;
	cma_heap->heap.type = ION_HEAP_TYPE_DMA;
	cma_heap->heap.debug_show = ion_cma_heap_debug_show;
	cma_heap->heap.name = kstrndup(heap_data->name,
				       MAX_HEAP_NAME - 1, GFP_KERNEL);
	cma_heap->align_order = get_order(heap_data->align);
	cma_heap->secure = heap_data->secure;
	cma_heap->protection_id = heap_data->id;

	return &cma_heap->heap;
}
#else /* !CONFIG_ION_EXYNOS */
static struct ion_heap *__ion_cma_heap_create(struct cma *cma)
{
	struct ion_cma_heap *cma_heap;

	cma_heap = kzalloc(sizeof(*cma_heap), GFP_KERNEL);

	if (!cma_heap)
		return ERR_PTR(-ENOMEM);

	cma_heap->heap.ops = &ion_cma_ops;
	/*
	 * get device from private heaps data, later it will be
	 * used to make the link with reserved CMA memory
	 */
	cma_heap->cma = cma;
	cma_heap->heap.type = ION_HEAP_TYPE_DMA;
	return &cma_heap->heap;
}

static int __ion_add_cma_heaps(struct cma *cma, void *data)
{
	struct ion_heap *heap;

	heap = __ion_cma_heap_create(cma);
	if (IS_ERR(heap))
		return PTR_ERR(heap);

	heap->name = cma_get_name(cma);

	ion_device_add_heap(heap);
	return 0;
}

static int ion_add_cma_heaps(void)
{
	cma_for_each_area(__ion_add_cma_heaps, NULL);
	return 0;
}
device_initcall(ion_add_cma_heaps);
#endif /* CONFIG_ION_EXYNOS */
