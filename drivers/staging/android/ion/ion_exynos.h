/*
 * drivers/staging/android/ion/ion_exynos.h
 *
 * Copyright (C) 2018 Samsung Electronics Co., Ltd.
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

#ifndef _ION_EXYNOS_H_
#define _ION_EXYNOS_H_

#include <linux/dma-direction.h>
#include <linux/dma-buf.h>

struct cma;
struct dma_buf;
struct ion_heap;
struct ion_platform_heap;
struct ion_device;
struct ion_buffer;
struct dma_buf_attachment;

/**
 * struct ion_buffer_prot_info - buffer protection information
 * @chunk_count: number of physically contiguous memory chunks to protect
 *               each chunk should has the same size.
 * @dma_addr:    device virtual address for protected memory access
 * @flags:       protection flags but actually, protection_id
 * @chunk_size:  length in bytes of each chunk.
 * @bus_address: if @chunk_count is 1, this is the physical address the chunk.
 *               if @chunk_count > 1, this is the physical address of unsigned
 *               long array of @chunk_count elements that contains the physical
 *               address of each chunk.
 */
struct ion_buffer_prot_info {
	unsigned int chunk_count;
	unsigned int dma_addr;
	unsigned int flags;
	unsigned int chunk_size;
	unsigned long bus_address;
};

struct ion_iovm_map {
	struct list_head list;
	struct device *dev;
	struct iommu_domain *domain;
	dma_addr_t iova;
	atomic_t mapcnt;
	int prop;
};

#ifdef CONFIG_ION_CARVEOUT_HEAP
extern struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *);
#else
#define ion_carveout_heap_create(p) ERR_PTR(-ENODEV)
#endif

#if defined(CONFIG_ION_CMA_HEAP) && defined(CONFIG_ION_EXYNOS)
extern struct ion_heap *ion_cma_heap_create(struct cma *cma,
					    struct ion_platform_heap *pheap);
#else
#define ion_cma_heap_create(cma, p) ERR_PTR(-ENODEV)
#endif

#ifdef CONFIG_ION_RBIN_HEAP
extern struct ion_heap *ion_rbin_heap_create(struct ion_platform_heap *pheap);
#endif

#if defined(CONFIG_ION_HPA_HEAP)
extern struct ion_heap *ion_hpa_heap_create(struct ion_platform_heap *pheap,
					    phys_addr_t except_areas[][2],
					    int n_except_areas);
#else
#define ion_hpa_heap_create(p, areas, n_areas) ERR_PTR(-ENODEV)
#endif

#if defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION) && defined(CONFIG_ION_EXYNOS)
int __init ion_secure_iova_pool_create(void);
#else
static inline int ion_secure_iova_pool_create(void)
{
	return 0;
}
#endif

#ifdef CONFIG_ION_EXYNOS
void *ion_buffer_protect_single(unsigned int protection_id, unsigned int size,
				unsigned long phys, unsigned int protalign);
void *ion_buffer_protect_multi(unsigned int protection_id, unsigned int count,
			       unsigned int chunk_size, unsigned long *phys_arr,
			       unsigned int protalign);
int ion_buffer_unprotect(void *priv);
void exynos_ion_fixup(struct ion_device *idev);
int exynos_ion_alloc_fixup(struct ion_device *idev, struct ion_buffer *buffer);
void exynos_ion_free_fixup(struct ion_buffer *buffer);
struct sg_table *ion_exynos_map_dma_buf_area(
				struct dma_buf_attachment *attachment,
				enum dma_data_direction direction,
				size_t size);
void ion_exynos_unmap_dma_buf_area(struct dma_buf_attachment *attachment,
				   struct sg_table *table,
				   enum dma_data_direction direction,
				   size_t size);
struct sg_table *ion_exynos_map_dma_buf(struct dma_buf_attachment *attachment,
					enum dma_data_direction direction);
void ion_exynos_unmap_dma_buf(struct dma_buf_attachment *attachment,
			      struct sg_table *table,
			      enum dma_data_direction direction);
int ion_exynos_dma_buf_begin_cpu_access(struct dma_buf *dmabuf,
					enum dma_data_direction direction);
int ion_exynos_dma_buf_end_cpu_access(struct dma_buf *dmabuf,
				      enum dma_data_direction direction);
void ion_debug_initialize(struct ion_device *idev);
void ion_debug_heap_init(struct ion_heap *heap);

#else
static inline void *ion_buffer_protect_single(unsigned int protection_id,
					      unsigned int size,
					      unsigned long phys,
					      unsigned int protalign)
{
	return NULL;
}

static inline void *ion_buffer_protect_multi(unsigned int protection_id,
					     unsigned int count,
					     unsigned int chunk_size,
					     unsigned long *phys_arr,
					     unsigned int protalign)
{
	return NULL;
}
#define ion_buffer_unprotect(priv) do { } while (0)
#define exynos_ion_fixup(idev) do { } while (0)
static inline int exynos_ion_alloc_fixup(struct ion_device *idev,
					 struct ion_buffer *buffer)
{
	return 0;
}

#define exynos_ion_free_fixup(buffer) do { } while (0)

static inline struct sg_table *ion_exynos_map_dma_buf(
					struct dma_buf_attachment *att,
					enum dma_data_direction dir)
{
	return ERR_PTR(-ENODEV);
}

#define ion_exynos_unmap_dma_buf(attachment, table, direction) do { } while (0)

static inline int ion_exynos_dma_buf_begin_cpu_access(
					struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{
	return 0;
}

static inline int ion_exynos_dma_buf_end_cpu_access(
					struct dma_buf *dmabuf,
					enum dma_data_direction direction)
{
	return 0;
}

#define ion_debug_initialize(idev) do { } while (0)
#define ion_debug_heap_init(idev) do { } while (0)
#endif

extern const struct dma_buf_ops ion_dma_buf_ops;

struct ion_heap *ion_get_heap_by_name(const char *heap_name);
struct dma_buf *__ion_alloc(size_t len, unsigned int heap_id_mask,
			    unsigned int flags);
void exynos_ion_init_camera_heaps(void);

#endif /* _ION_EXYNOS_H_ */
