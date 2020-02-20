/*
 * drivers/staging/android/uapi/ion.h
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

#ifndef _UAPI_LINUX_ION_H
#define _UAPI_LINUX_ION_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * enum ion_heap_types - list of all possible types of heaps
 * @ION_HEAP_TYPE_SYSTEM:	 memory allocated via vmalloc
 * @ION_HEAP_TYPE_SYSTEM_CONTIG: memory allocated via kmalloc
 * @ION_HEAP_TYPE_CARVEOUT:	 memory allocated from a prereserved
 *				 carveout heap, allocations are physically
 *				 contiguous
 * @ION_HEAP_TYPE_DMA:		 memory allocated via DMA API
 * @ION_NUM_HEAPS:		 helper for iterating over heaps, a bit mask
 *				 is used to identify the heaps, so only 32
 *				 total heap types are supported
 */
enum ion_heap_type {
	ION_HEAP_TYPE_SYSTEM,
	ION_HEAP_TYPE_SYSTEM_CONTIG,
	ION_HEAP_TYPE_CARVEOUT,
	ION_HEAP_TYPE_CHUNK,
	ION_HEAP_TYPE_DMA,
	ION_HEAP_TYPE_CUSTOM, /*
			       * must be last so device specific heaps always
			       * are at the end of this enum
			       */
	ION_HEAP_TYPE_CUSTOM2,
	ION_HEAP_TYPE_HPA = ION_HEAP_TYPE_CUSTOM,
};

/* Samsung specific ION heap types */
#define ION_HEAP_TYPE_RBIN		ION_HEAP_TYPE_CUSTOM2

#define ION_NUM_HEAP_IDS		(sizeof(unsigned int) * 8)

/**
 * allocation flags - the lower 16 bits are used by core ion, the upper 16
 * bits are reserved for use by the heaps themselves.
 */

/*
 * mappings of this buffer should be cached, ion will do cache maintenance
 * when the buffer is mapped for dma
 */
#define ION_FLAG_CACHED 1
/*
 * the allocated buffer is not cleared with zeroes to avoid initialization
 * overhead. Mapping to userspace is not allowed.
 */
#define ION_FLAG_NOZEROED 8
/*
 * the allocated buffer is not allowed to access without a proper permission.
 * Both of mmap() and dmabuf kmap/vmap will fail. Acessing by any other mapping
 * will generate data abort exception and get oops.
 * ION_FLAG_PROTECTED is only applicable to the heaps with security property.
 * Other heaps ignore this flag.
 */
#define ION_FLAG_PROTECTED 16
/*
 * the allocated buffer does not have dirty cache line allocated. In other
 * words, ION flushes the cache even though allocation flags includes
 * ION_FLAG_CACHED. This is required by some H/W drivers that wants to reduce
 * overhead by explicit cache maintenance.
 */
#define ION_FLAG_SYNC_FORCE 32
/*
 * the allocated buffer is possibly written by H/W(GPU) before initializing by
 * S/W except buffer initialization by ION on allocation.
 */
#define ION_FLAG_MAY_HWRENDER 64

/**
 * DOC: Ion Userspace API
 *
 * create a client by opening /dev/ion
 * most operations handled via following ioctls
 *
 */

/**
 * struct ion_allocation_data - metadata passed from userspace for allocations
 * @len:		size of the allocation
 * @heap_id_mask:	mask of heap ids to allocate from
 * @flags:		flags passed to heap
 * @handle:		pointer that will be populated with a cookie to use to
 *			refer to this allocation
 *
 * Provided by userspace as an argument to the ioctl
 */
struct ion_allocation_data {
	__u64 len;
	__u32 heap_id_mask;
	__u32 flags;
	__u32 fd;
	__u32 unused;
};

#define MAX_HEAP_NAME			32

/**
 * freed buffer to the heap may not be returned to the free pool immediately
 */
#define ION_HEAPDATA_FLAGS_DEFER_FREE		1
/**
 * ION_FLAG_PROTECTED is applicable.
 */
#define ION_HEAPDATA_FLAGS_ALLOW_PROTECTION	2
/**
 * access to the buffer from this heap is not allowed in both of userland and
 * kernel space. mmap() and dmabuf kmap/vmap always fail.
 */
#define ION_HEAPDATA_FLAGS_UNTOUCHABLE		4
/**
 * struct ion_heap_data - data about a heap
 * @name - first 32 characters of the heap name
 * @type - heap type
 * @heap_id - heap id for the heap
 * @size - size of the memory pool if the heap type is dma, carveout and chunk
 * @heap_flags - properties of heap
 *
 */
struct ion_heap_data {
	char name[MAX_HEAP_NAME];
	__u32 type;
	__u32 heap_id;
	__u32 size;       /* reserved0 */
	__u32 heap_flags; /* reserved1 */
	__u32 reserved2;
};

/**
 * struct ion_heap_query - collection of data about all heaps
 * @cnt - total number of heaps to be copied
 * @heaps - buffer to copy heap data
 */
struct ion_heap_query {
	__u32 cnt; /* Total number of heaps to be copied */
	__u32 reserved0; /* align to 64bits */
	__u64 heaps; /* buffer to be populated */
	__u32 reserved1;
	__u32 reserved2;
};

#define ION_IOC_MAGIC		'I'

/**
 * DOC: ION_IOC_ALLOC - allocate memory
 *
 * Takes an ion_allocation_data struct and returns it with the handle field
 * populated with the opaque handle for the allocation.
 */
#define ION_IOC_ALLOC		_IOWR(ION_IOC_MAGIC, 0, \
				      struct ion_allocation_data)

/**
 * DOC: ION_IOC_HEAP_QUERY - information about available heaps
 *
 * Takes an ion_heap_query structure and populates information about
 * available Ion heaps.
 */
#define ION_IOC_HEAP_QUERY     _IOWR(ION_IOC_MAGIC, 8, \
					struct ion_heap_query)

#endif /* _UAPI_LINUX_ION_H */
