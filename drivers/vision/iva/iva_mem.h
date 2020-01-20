/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_MEM_H__
#define __IVA_MEM_H__

#include "iva_ctrl.h"
#include "iva_ctrl_ioctl.h"

#ifdef CONFIG_ION_EXYNOS
#include <linux/ion_exynos.h>
#define REQ_ION_HEAP_ID		EXYNOS_ION_HEAP_SYSTEM_MASK
#else
#define REQ_ION_HEAP_ID		ZONE_BIT(ION_HEAP_TYPE_EXT)
#endif

/* iva iova mapping flags */
#define IVA_ALLOC_CACHE_MASK		(0x3)
#define IVA_ALLOC_CACHE_SHIFT		(8)
#define GET_IVA_CACHE_TYPE(flag)	((flag >> IVA_ALLOC_CACHE_SHIFT) &	\
							IVA_ALLOC_CACHE_MASK)
#define SET_IVA_CACHE_TYPE(flag, a)	(flag |= (a & IVA_ALLOC_CACHE_MASK) <<	\
							IVA_ALLOC_CACHE_SHIFT)

#define IVA_FREE_REQ_MASK		(0x1)
#define IVA_FREE_REQ_SHIFT		(4)	/* free requested */
#define IVA_FREE_REQUESTED		(IVA_FREE_REQ_MASK << IVA_FREE_REQ_SHIFT)
#define FREE_REQUESTED(flag)		(flag & IVA_FREE_REQUESTED)

#define IVA_ALLOC_TYPE_MASK		(0x3)
#define IVA_ALLOC_TYPE_SHIFT		(0)
#define IVA_ALLOC_TYPE_IMPORTED		(0x2)	/* from outside */
#define IVA_ALLOC_TYPE_ALLOCATED	(0x1)	/* from inside */
#define IVA_ALLOC_TYPE_NOT_DEFINED	(0)	/* not allocated or freed */

#define GET_IVA_ALLOC_TYPE(flag) \
		((flag >> IVA_ALLOC_TYPE_SHIFT) & IVA_ALLOC_TYPE_MASK)
#define SET_IVA_ALLOC_TYPE(flag, a)	\
		(flag |= (a & IVA_ALLOC_TYPE_MASK) << IVA_ALLOC_TYPE_SHIFT)

#define ALLOC_INSIDE(flag)	(flag & IVA_ALLOC_TYPE_ALLOCATED)
#define ALLOC_OUTSIDE(flag)	(flag & IVA_ALLOC_TYPE_IMPORTED)

#ifndef CONFIG_ION_EXYNOS
extern struct ion_device *idev; /* CONFIG_ION_DUMMY */
#endif

struct iva_mem_map {
	/* TO DO: ref */
	struct list_head	node;		/* global */
	struct hlist_node	h_node;		/* per process */
	int			flags;
	struct kref		map_ref;	/* iova mapping */
	struct device		*dev;

	int			shared_fd;
	struct dma_buf		*dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table		*sg_table;
	unsigned long		io_va;
	uint32_t		req_size;	/* requested size */
	uint32_t		act_size;	/* actual_size */
};

extern int	iva_mem_map_read_refcnt(struct iva_mem_map *iva_map);
extern void	iva_mem_show_global_mapped_list_lock(struct iva_dev_data *iva,
			bool lock);
static inline void iva_mem_show_global_mapped_list(struct iva_dev_data *iva)
{
	iva_mem_show_global_mapped_list_lock(iva, true);
}

static inline void iva_mem_show_global_mapped_list_nolock(struct iva_dev_data *iva)
{
	iva_mem_show_global_mapped_list_lock(iva, false);
}

extern struct iva_mem_map *iva_mem_find_proc_map_with_fd(struct iva_proc *proc,
			int shared_fd);

static inline struct dma_buf *iva_mem_get_dmabuf_with_fd(struct iva_proc *proc,
			int shared_fd)
{
	struct iva_mem_map *iva_map_node;

	iva_map_node = iva_mem_find_proc_map_with_fd(proc, shared_fd);
	if (!iva_map_node)
		return NULL;

	return iva_map_node->dmabuf;
}

extern long	iva_mem_ioctl(struct iva_proc *proc,
			unsigned int cmd, void __user *p);

extern void	iva_mem_init_proc_mem(struct iva_proc *proc);
extern void	iva_mem_deinit_proc_mem(struct iva_proc *proc);

extern int	iva_mem_create_ion_client(struct iva_dev_data *iva);
extern int	iva_mem_destroy_ion_client(struct iva_dev_data *iva);

#endif /* __IVA_MBOX_H */
