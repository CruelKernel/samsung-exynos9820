/*
 * drivers/staging/android/ion/ion_hpa_heap.c
 *
 * Copyright (C) 2016, 2018 Samsung Electronics Co., Ltd.
 * Author: <pullip.cho@samsung.com> for Exynos SoCs
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/scatterlist.h>

#include <asm/cacheflush.h>

#include "ion.h"
#include "ion_exynos.h"

#define ION_HPA_CHUNK_SIZE(heap)  (PAGE_SIZE << (heap)->order)
#define ION_HPA_PAGE_COUNT(len, heap) \
	(ALIGN(len, ION_HPA_CHUNK_SIZE(heap)) / ION_HPA_CHUNK_SIZE(heap))
#define HPA_MAX_CHUNK_COUNT ((PAGE_SIZE * 2) / sizeof(struct page *))

struct ion_hpa_heap {
	struct ion_heap heap;
	unsigned int order;
	unsigned int protection_id;
	phys_addr_t (*exception_areas)[2];
	int exception_count;
	bool secure;
};

#define to_hpa_heap(x) container_of(x, struct ion_hpa_heap, heap)

static int ion_hpa_compare_pages(const void *p1, const void *p2)
{
	if (*((unsigned long *)p1) > (*((unsigned long *)p2)))
		return 1;
	else if (*((unsigned long *)p1) < (*((unsigned long *)p2)))
		return -1;
	return 0;
}

static int ion_hpa_allocate(struct ion_heap *heap,
				 struct ion_buffer *buffer, unsigned long len,
				 unsigned long flags)
{
	struct ion_hpa_heap *hpa_heap = to_hpa_heap(heap);
	unsigned int count =
		(unsigned int)ION_HPA_PAGE_COUNT((unsigned int)len, hpa_heap);
	bool zero = !(flags & ION_FLAG_NOZEROED);
	bool cacheflush = !(flags & ION_FLAG_CACHED) ||
			  ((flags & ION_FLAG_SYNC_FORCE) != 0);
	bool protected = hpa_heap->secure && (flags & ION_FLAG_PROTECTED);
	size_t desc_size = sizeof(struct page *) * count;
	struct page **pages;
	unsigned long *phys;
	struct sg_table *sgt;
	struct scatterlist *sg;
	int ret, i;

	if (count > HPA_MAX_CHUNK_COUNT) {
		pr_info("ION HPA heap does not allow buffers > %zu\n",
			HPA_MAX_CHUNK_COUNT * ION_HPA_CHUNK_SIZE(hpa_heap));
		return -ENOMEM;
	}

	pages = kmalloc(desc_size, GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	sgt = kmalloc(sizeof(*sgt), GFP_KERNEL);
	if (!sgt) {
		ret = -ENOMEM;
		goto err_sgt;
	}

	ret = sg_alloc_table(sgt, count, GFP_KERNEL);
	if (ret)
		goto err_sg;

	i = protected ? hpa_heap->exception_count : 0;
	ret = alloc_pages_highorder_except(hpa_heap->order, pages, count,
					   hpa_heap->exception_areas, i);
	if (ret)
		goto err_pages;

	sort(pages, count, sizeof(*pages), ion_hpa_compare_pages, NULL);

	if (protected) {
		cacheflush = true;
		zero = false;
	}

	/*
	 * convert a page descriptor into its corresponding physical address
	 * in place to reduce memory allocation
	 */
	phys = (unsigned long *)pages;

	for_each_sg(sgt->sgl, sg, sgt->orig_nents, i) {
		if (zero)
			memset(page_address(pages[i]), 0,
			       ION_HPA_CHUNK_SIZE(hpa_heap));
		if (cacheflush)
			__flush_dcache_area(page_address(pages[i]),
					    ION_HPA_CHUNK_SIZE(hpa_heap));

		sg_set_page(sg, pages[i], ION_HPA_CHUNK_SIZE(hpa_heap), 0);
		phys[i] = page_to_phys(pages[i]);
	}

	if (protected) {
		buffer->priv_virt = ion_buffer_protect_multi(
					hpa_heap->protection_id, count,
					ION_HPA_CHUNK_SIZE(hpa_heap), phys,
					ION_HPA_CHUNK_SIZE(hpa_heap));
		if (IS_ERR(buffer->priv_virt)) {
			ret = PTR_ERR(buffer->priv_virt);
			goto err_prot;
		}
	} else {
		kfree(pages);
	}

	buffer->sg_table = sgt;

	return 0;
err_prot:
	for_each_sg(sgt->sgl, sg, sgt->orig_nents, i)
		__free_pages(sg_page(sg), hpa_heap->order);
err_pages:
	sg_free_table(sgt);
err_sg:
	kfree(sgt);
err_sgt:
	kfree(pages);

	return ret;
}

static void ion_hpa_free(struct ion_buffer *buffer)
{
	struct ion_hpa_heap *hpa_heap = to_hpa_heap(buffer->heap);
	bool protected = hpa_heap->secure &&
			 (buffer->flags & ION_FLAG_PROTECTED);
	struct sg_table *sgt = buffer->sg_table;
	struct scatterlist *sg;
	int i;
	int unprot_err = 0;

	if (protected)
		unprot_err = ion_buffer_unprotect(buffer->priv_virt);

	if (!unprot_err)
		for_each_sg(sgt->sgl, sg, sgt->orig_nents, i)
			__free_pages(sg_page(sg), hpa_heap->order);
	sg_free_table(sgt);
	kfree(sgt);
}

static void hpa_heap_query(struct ion_heap *heap, struct ion_heap_data *data)
{
	struct ion_hpa_heap *hpa_heap = to_hpa_heap(heap);

	if (hpa_heap->secure)
		data->heap_flags |= ION_HEAPDATA_FLAGS_ALLOW_PROTECTION;
}

static struct ion_heap_ops ion_hpa_ops = {
	.allocate = ion_hpa_allocate,
	.free = ion_hpa_free,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.query_heap = hpa_heap_query,
};

struct ion_heap *ion_hpa_heap_create(struct ion_platform_heap *data,
				     phys_addr_t except_areas[][2],
				     int n_except_areas)
{
	struct ion_hpa_heap *heap;

	heap = kzalloc(sizeof(*heap), GFP_KERNEL);
	if (!heap)
		return ERR_PTR(-ENOMEM);

	heap->heap.ops = &ion_hpa_ops;
	heap->heap.type = ION_HEAP_TYPE_HPA;
	heap->heap.name = kstrndup(data->name, MAX_HEAP_NAME - 1, GFP_KERNEL);
	heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;
	heap->order = get_order(data->align);
	heap->protection_id = data->id;
	heap->secure = data->secure;
	heap->exception_areas = except_areas;
	heap->exception_count = n_except_areas;

	return &heap->heap;
}
