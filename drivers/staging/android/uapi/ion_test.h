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

#ifndef _UAPI_LINUX_ION_TEST_H
#define _UAPI_LINUX_ION_TEST_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * struct ion_test_rw_data - metadata passed to the kernel to read handle
 * @ptr:	a pointer to an area at least as large as size
 * @offset:	offset into the ion buffer to start reading
 * @size:	size to read or write
 * @write:	1 to write, 0 to read
 */
struct ion_test_rw_data {
	__u64 ptr;
	__u64 offset;
	__u64 size;
	int write;
	int __padding;
};

/**
 * struct ion_test_phys_data - metadata passed to the kernel to check phys
 * @cmd:	what to test. One of the followings:
 *		- PHYS_CHUNK_IS_IDENTICAL_SIZE: check if physical memory is
 *		  a set of memory chunks with identical size. If this test
 *		  succeeds, ion_test driver stores the chunk size to
 *		  ion_test_phys_data.result.
 *		- PHYS_IS_ORDERED_IN_ADDRESS: check if physical memory is
 *		  sorted in address order of each physical memory chunks.
 *		- PHYS_IS_RESERVED: check if physically contiguous and its
 *		  carved out(!pfn_valid()) or reserve (PG_reserved)
 *		- PHYS_IS_CMA: check if physically contiguous and its
 *		  migratetype is MIGRATE_CMA
 *		- PHYS_IS_ALIGNED: check if the address of the first physical
 *		  memory chunk is aligned by @arg.
 * @arg:	an argument to @cmd. the meaning of @arg varies according to
 *		@cmd.
 * @result:	result of @cmd is stored if required.
 */
enum {
	PHYS_CHUNK_IS_IDENTICAL_SIZE = 0,
	PHYS_IS_ORDERED_IN_ADDRESS = 1,
	PHYS_IS_RESERVED = 2,
	PHYS_IS_CMA = 3,
	PHYS_IS_ALIGNED = 4,
};

struct ion_test_phys_data {
	__u32 cmd;
	__u32 arg;
	__u32 result;
};

#define ION_IOC_MAGIC		'I'

/**
 * DOC: ION_IOC_TEST_SET_DMA_BUF - attach a dma buf to the test driver
 *
 * Attaches a dma buf fd to the test driver.  Passing a second fd or -1 will
 * release the first fd.
 */
#define ION_IOC_TEST_SET_FD \
			_IO(ION_IOC_MAGIC, 0xf0)

/**
 * DOC: ION_IOC_TEST_DMA_MAPPING - read or write memory from a handle as DMA
 *
 * Reads or writes the memory from a handle using an uncached mapping.  Can be
 * used by unit tests to emulate a DMA engine as close as possible.  Only
 * expected to be used for debugging and testing, may not always be available.
 */
#define ION_IOC_TEST_DMA_MAPPING \
			_IOW(ION_IOC_MAGIC, 0xf1, struct ion_test_rw_data)

/**
 * DOC: ION_IOC_TEST_KERNEL_MAPPING - read or write memory from a handle
 *
 * Reads or writes the memory from a handle using a kernel mapping.  Can be
 * used by unit tests to test heap map_kernel functions.  Only expected to be
 * used for debugging and testing, may not always be available.
 */
#define ION_IOC_TEST_KERNEL_MAPPING \
			_IOW(ION_IOC_MAGIC, 0xf2, struct ion_test_rw_data)

/**
 * DOC: ION_IOC_TEST_PHYS - Study the properties of physical memory
 *
 * Studies the properties of the physical memory of the allocated buffer from
 * ION.
 */
#define ION_IOC_TEST_PHYS \
			_IOWR(ION_IOC_MAGIC, 0xf3, struct ion_test_phys_data)

#endif /* _UAPI_LINUX_ION_H */
