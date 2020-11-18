#ifndef _NPU_MEMORY_H_
#define _NPU_MEMORY_H_

#include <linux/platform_device.h>
#include <media/videobuf2-core.h>
#if defined(CONFIG_VIDEOBUF2_CMA_PHYS)
#include <media/videobuf2-cma-phys.h>
#elif defined(CONFIG_VIDEOBUF2_ION)
#include <media/videobuf2-ion.h>
#endif

#define MODERN_ION

struct npu_memory_buffer {
	struct list_head		list;
	struct dma_buf			*dma_buf;
	struct dma_buf_attachment	*attachment;
	struct sg_table			*sgt;
	dma_addr_t			daddr;
	void				*vaddr;
	size_t				size;
	int				fd;
};

struct npu_memory;
struct npu_mem_ops {
	int (*resume)(struct npu_memory *memory);
	int (*suspend)(struct npu_memory *memory);
};

struct npu_memory {
	struct device			*dev;
	struct ion_client		*client;
	const struct npu_mem_ops	*npu_mem_ops;
	const struct vb2_mem_ops	*vb2_mem_ops;
	atomic_t			refcount;

	spinlock_t			map_lock;
	struct list_head		map_list;
	u32				map_count;

	spinlock_t			alloc_lock;
	struct list_head		alloc_list;
	u32				alloc_count;
};

int npu_memory_probe(struct npu_memory *memory, struct device *dev);
int npu_memory_open(struct npu_memory *memory);
int npu_memory_close(struct npu_memory *memory);
int npu_memory_map(struct npu_memory *memory, struct npu_memory_buffer *buffer);
void npu_memory_unmap(struct npu_memory *memory, struct npu_memory_buffer *buffer);
int npu_memory_alloc(struct npu_memory *memory, struct npu_memory_buffer *buffer);
void npu_memory_free(struct npu_memory *memory, struct npu_memory_buffer *buffer);

#endif
