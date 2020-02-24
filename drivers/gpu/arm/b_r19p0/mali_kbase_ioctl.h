/*
 *
 * (C) COPYRIGHT 2017-2019 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _KBASE_IOCTL_H_
#define _KBASE_IOCTL_H_

#ifdef __cpluscplus
extern "C" {
#endif

#include <asm-generic/ioctl.h>
#include <linux/types.h>

#define KBASE_IOCTL_TYPE 0x80

/*
 * 11.1:
 * - Add BASE_MEM_TILER_ALIGN_TOP under base_mem_alloc_flags
 * 11.2:
 * - KBASE_MEM_QUERY_FLAGS can return KBASE_REG_PF_GROW and KBASE_REG_SECURE,
 *   which some user-side clients prior to 11.2 might fault if they received
 *   them
 * 11.3:
 * - New ioctls KBASE_IOCTL_STICKY_RESOURCE_MAP and
 *   KBASE_IOCTL_STICKY_RESOURCE_UNMAP
 * 11.4:
 * - New ioctl KBASE_IOCTL_MEM_FIND_GPU_START_AND_OFFSET
 * 11.5:
 * - New ioctl: KBASE_IOCTL_MEM_JIT_INIT (old ioctl renamed to _OLD)
 * 11.6:
 * - Added flags field to base_jit_alloc_info structure, which can be used to
 *   specify pseudo chunked tiler alignment for JIT allocations.
 * 11.7:
 * - Removed UMP support
 * 11.8:
 * - Added BASE_MEM_UNCACHED_GPU under base_mem_alloc_flags
 * 11.9:
 * - Added BASE_MEM_PERMANENT_KERNEL_MAPPING and BASE_MEM_FLAGS_KERNEL_ONLY
 *   under base_mem_alloc_flags
 * 11.10:
 * - Enabled the use of nr_extres field of base_jd_atom_v2 structure for
 *   JIT_ALLOC and JIT_FREE type softjobs to enable multiple JIT allocations
 *   with one softjob.
 * 11.11:
 * - Added BASE_MEM_GPU_VA_SAME_4GB_PAGE under base_mem_alloc_flags
 * 11.12:
 * - Removed ioctl: KBASE_IOCTL_GET_PROFILING_CONTROLS
 * 11.13:
 * - New ioctl: KBASE_IOCTL_MEM_EXEC_INIT
 * 11.14:
 * - Add BASE_MEM_GROUP_ID_MASK, base_mem_group_id_get, base_mem_group_id_set
 *   under base_mem_alloc_flags
 * 11.15:
 * - Added BASEP_CONTEXT_MMU_GROUP_ID_MASK under base_context_create_flags.
 * - Require KBASE_IOCTL_SET_FLAGS before BASE_MEM_MAP_TRACKING_HANDLE can be
 *   passed to mmap().
 * 11.16:
 * - Extended ioctl KBASE_IOCTL_MEM_SYNC to accept imported dma-buf.
 * - Modified (backwards compatible) ioctl KBASE_IOCTL_MEM_IMPORT behavior for
 *   dma-buf. Now, buffers are mapped on GPU when first imported, no longer
 *   requiring external resource or sticky resource tracking. UNLESS,
 *   CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND is enabled.
 * 11.17:
 * - Skipped
 * 11.18:
 * - New ioctl: KBASE_IOCTL_KINSTR_JM_FD
 */
#define BASE_UK_VERSION_MAJOR 11
#define BASE_UK_VERSION_MINOR 18

/**
 * struct kbase_ioctl_version_check - Check version compatibility with kernel
 *
 * @major: Major version number
 * @minor: Minor version number
 */
struct kbase_ioctl_version_check {
	__u16 major;
	__u16 minor;
};

#define KBASE_IOCTL_VERSION_CHECK \
	_IOWR(KBASE_IOCTL_TYPE, 0, struct kbase_ioctl_version_check)

/**
 * struct kbase_ioctl_set_flags - Set kernel context creation flags
 *
 * @create_flags: Flags - see base_context_create_flags
 */
struct kbase_ioctl_set_flags {
	__u32 create_flags;
};

#define KBASE_IOCTL_SET_FLAGS \
	_IOW(KBASE_IOCTL_TYPE, 1, struct kbase_ioctl_set_flags)

/**
 * struct kbase_ioctl_job_submit - Submit jobs/atoms to the kernel
 *
 * @addr: Memory address of an array of struct base_jd_atom_v2
 * @nr_atoms: Number of entries in the array
 * @stride: sizeof(struct base_jd_atom_v2)
 */
struct kbase_ioctl_job_submit {
	__u64 addr;
	__u32 nr_atoms;
	__u32 stride;
};

#define KBASE_IOCTL_JOB_SUBMIT \
	_IOW(KBASE_IOCTL_TYPE, 2, struct kbase_ioctl_job_submit)

/**
 * struct kbase_ioctl_get_gpuprops - Read GPU properties from the kernel
 *
 * @buffer: Pointer to the buffer to store properties into
 * @size: Size of the buffer
 * @flags: Flags - must be zero for now
 *
 * The ioctl will return the number of bytes stored into @buffer or an error
 * on failure (e.g. @size is too small). If @size is specified as 0 then no
 * data will be written but the return value will be the number of bytes needed
 * for all the properties.
 *
 * @flags may be used in the future to request a different format for the
 * buffer. With @flags == 0 the following format is used.
 *
 * The buffer will be filled with pairs of values, a u32 key identifying the
 * property followed by the value. The size of the value is identified using
 * the bottom bits of the key. The value then immediately followed the key and
 * is tightly packed (there is no padding). All keys and values are
 * little-endian.
 *
 * 00 = u8
 * 01 = u16
 * 10 = u32
 * 11 = u64
 */
struct kbase_ioctl_get_gpuprops {
	__u64 buffer;
	__u32 size;
	__u32 flags;
};

#define KBASE_IOCTL_GET_GPUPROPS \
	_IOW(KBASE_IOCTL_TYPE, 3, struct kbase_ioctl_get_gpuprops)

#define KBASE_IOCTL_POST_TERM \
	_IO(KBASE_IOCTL_TYPE, 4)

/**
 * union kbase_ioctl_mem_alloc - Allocate memory on the GPU
 *
 * @va_pages: The number of pages of virtual address space to reserve
 * @commit_pages: The number of physical pages to allocate
 * @extent: The number of extra pages to allocate on each GPU fault which grows
 *          the region
 * @flags: Flags
 * @gpu_va: The GPU virtual address which is allocated
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_mem_alloc {
	struct {
		__u64 va_pages;
		__u64 commit_pages;
		__u64 extent;
		__u64 flags;
	} in;
	struct {
		__u64 flags;
		__u64 gpu_va;
	} out;
};

#define KBASE_IOCTL_MEM_ALLOC \
	_IOWR(KBASE_IOCTL_TYPE, 5, union kbase_ioctl_mem_alloc)

/**
 * struct kbase_ioctl_mem_query - Query properties of a GPU memory region
 * @gpu_addr: A GPU address contained within the region
 * @query: The type of query
 * @value: The result of the query
 *
 * Use a %KBASE_MEM_QUERY_xxx flag as input for @query.
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_mem_query {
	struct {
		__u64 gpu_addr;
		__u64 query;
	} in;
	struct {
		__u64 value;
	} out;
};

#define KBASE_IOCTL_MEM_QUERY \
	_IOWR(KBASE_IOCTL_TYPE, 6, union kbase_ioctl_mem_query)

#define KBASE_MEM_QUERY_COMMIT_SIZE	((u64)1)
#define KBASE_MEM_QUERY_VA_SIZE		((u64)2)
#define KBASE_MEM_QUERY_FLAGS		((u64)3)

/**
 * struct kbase_ioctl_mem_free - Free a memory region
 * @gpu_addr: Handle to the region to free
 */
struct kbase_ioctl_mem_free {
	__u64 gpu_addr;
};

#define KBASE_IOCTL_MEM_FREE \
	_IOW(KBASE_IOCTL_TYPE, 7, struct kbase_ioctl_mem_free)

/**
 * struct kbase_ioctl_hwcnt_reader_setup - Setup HWC dumper/reader
 * @buffer_count: requested number of dumping buffers
 * @jm_bm:        counters selection bitmask (JM)
 * @shader_bm:    counters selection bitmask (Shader)
 * @tiler_bm:     counters selection bitmask (Tiler)
 * @mmu_l2_bm:    counters selection bitmask (MMU_L2)
 *
 * A fd is returned from the ioctl if successful, or a negative value on error
 */
struct kbase_ioctl_hwcnt_reader_setup {
	__u32 buffer_count;
	__u32 jm_bm;
	__u32 shader_bm;
	__u32 tiler_bm;
	__u32 mmu_l2_bm;
};

#define KBASE_IOCTL_HWCNT_READER_SETUP \
	_IOW(KBASE_IOCTL_TYPE, 8, struct kbase_ioctl_hwcnt_reader_setup)

/**
 * struct kbase_ioctl_hwcnt_enable - Enable hardware counter collection
 * @dump_buffer:  GPU address to write counters to
 * @jm_bm:        counters selection bitmask (JM)
 * @shader_bm:    counters selection bitmask (Shader)
 * @tiler_bm:     counters selection bitmask (Tiler)
 * @mmu_l2_bm:    counters selection bitmask (MMU_L2)
 */
struct kbase_ioctl_hwcnt_enable {
	__u64 dump_buffer;
	__u32 jm_bm;
	__u32 shader_bm;
	__u32 tiler_bm;
	__u32 mmu_l2_bm;
};

#define KBASE_IOCTL_HWCNT_ENABLE \
	_IOW(KBASE_IOCTL_TYPE, 9, struct kbase_ioctl_hwcnt_enable)

#define KBASE_IOCTL_HWCNT_DUMP \
	_IO(KBASE_IOCTL_TYPE, 10)

#define KBASE_IOCTL_HWCNT_CLEAR \
	_IO(KBASE_IOCTL_TYPE, 11)

/**
 * struct kbase_ioctl_hwcnt_values - Values to set dummy the dummy counters to.
 * @data:    Counter samples for the dummy model.
 * @size:    Size of the counter sample data.
 * @padding: Padding.
 */
struct kbase_ioctl_hwcnt_values {
	__u64 data;
	__u32 size;
	__u32 padding;
};

#define KBASE_IOCTL_HWCNT_SET \
	_IOW(KBASE_IOCTL_TYPE, 32, struct kbase_ioctl_hwcnt_values)

/**
 * struct kbase_ioctl_disjoint_query - Query the disjoint counter
 * @counter:   A counter of disjoint events in the kernel
 */
struct kbase_ioctl_disjoint_query {
	__u32 counter;
};

#define KBASE_IOCTL_DISJOINT_QUERY \
	_IOR(KBASE_IOCTL_TYPE, 12, struct kbase_ioctl_disjoint_query)

/**
 * struct kbase_ioctl_get_ddk_version - Query the kernel version
 * @version_buffer: Buffer to receive the kernel version string
 * @size: Size of the buffer
 * @padding: Padding
 *
 * The ioctl will return the number of bytes written into version_buffer
 * (which includes a NULL byte) or a negative error code
 *
 * The ioctl request code has to be _IOW because the data in ioctl struct is
 * being copied to the kernel, even though the kernel then writes out the
 * version info to the buffer specified in the ioctl.
 */
struct kbase_ioctl_get_ddk_version {
	__u64 version_buffer;
	__u32 size;
	__u32 padding;
};

#define KBASE_IOCTL_GET_DDK_VERSION \
	_IOW(KBASE_IOCTL_TYPE, 13, struct kbase_ioctl_get_ddk_version)

/**
 * struct kbase_ioctl_mem_jit_init_old - Initialise the JIT memory allocator
 *
 * @va_pages: Number of VA pages to reserve for JIT
 *
 * Note that depending on the VA size of the application and GPU, the value
 * specified in @va_pages may be ignored.
 *
 * New code should use KBASE_IOCTL_MEM_JIT_INIT instead, this is kept for
 * backwards compatibility.
 */
struct kbase_ioctl_mem_jit_init_old {
	__u64 va_pages;
};

#define KBASE_IOCTL_MEM_JIT_INIT_OLD \
	_IOW(KBASE_IOCTL_TYPE, 14, struct kbase_ioctl_mem_jit_init_old)

/**
 * struct kbase_ioctl_mem_jit_init - Initialise the JIT memory allocator
 *
 * @va_pages: Number of VA pages to reserve for JIT
 * @max_allocations: Maximum number of concurrent allocations
 * @trim_level: Level of JIT allocation trimming to perform on free (0 - 100%)
 * @group_id: Group ID to be used for physical allocations
 * @padding: Currently unused, must be zero
 *
 * Note that depending on the VA size of the application and GPU, the value
 * specified in @va_pages may be ignored.
 */
struct kbase_ioctl_mem_jit_init {
	__u64 va_pages;
	__u8 max_allocations;
	__u8 trim_level;
	__u8 group_id;
	__u8 padding[5];
};

#define KBASE_IOCTL_MEM_JIT_INIT \
	_IOW(KBASE_IOCTL_TYPE, 14, struct kbase_ioctl_mem_jit_init)

/**
 * struct kbase_ioctl_mem_sync - Perform cache maintenance on memory
 *
 * @handle: GPU memory handle (GPU VA)
 * @user_addr: The address where it is mapped in user space
 * @size: The number of bytes to synchronise
 * @type: The direction to synchronise: 0 is sync to memory (clean),
 * 1 is sync from memory (invalidate). Use the BASE_SYNCSET_OP_xxx constants.
 * @padding: Padding to round up to a multiple of 8 bytes, must be zero
 */
struct kbase_ioctl_mem_sync {
	__u64 handle;
	__u64 user_addr;
	__u64 size;
	__u8 type;
	__u8 padding[7];
};

#define KBASE_IOCTL_MEM_SYNC \
	_IOW(KBASE_IOCTL_TYPE, 15, struct kbase_ioctl_mem_sync)

/**
 * union kbase_ioctl_mem_find_cpu_offset - Find the offset of a CPU pointer
 *
 * @gpu_addr: The GPU address of the memory region
 * @cpu_addr: The CPU address to locate
 * @size: A size in bytes to validate is contained within the region
 * @offset: The offset from the start of the memory region to @cpu_addr
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_mem_find_cpu_offset {
	struct {
		__u64 gpu_addr;
		__u64 cpu_addr;
		__u64 size;
	} in;
	struct {
		__u64 offset;
	} out;
};

#define KBASE_IOCTL_MEM_FIND_CPU_OFFSET \
	_IOWR(KBASE_IOCTL_TYPE, 16, union kbase_ioctl_mem_find_cpu_offset)

/**
 * struct kbase_ioctl_get_context_id - Get the kernel context ID
 *
 * @id: The kernel context ID
 */
struct kbase_ioctl_get_context_id {
	__u32 id;
};

#define KBASE_IOCTL_GET_CONTEXT_ID \
	_IOR(KBASE_IOCTL_TYPE, 17, struct kbase_ioctl_get_context_id)

/**
 * struct kbase_ioctl_tlstream_acquire - Acquire a tlstream fd
 *
 * @flags: Flags
 *
 * The ioctl returns a file descriptor when successful
 */
struct kbase_ioctl_tlstream_acquire {
	__u32 flags;
};

#define KBASE_IOCTL_TLSTREAM_ACQUIRE \
	_IOW(KBASE_IOCTL_TYPE, 18, struct kbase_ioctl_tlstream_acquire)

#define KBASE_IOCTL_TLSTREAM_FLUSH \
	_IO(KBASE_IOCTL_TYPE, 19)

/**
 * struct kbase_ioctl_mem_commit - Change the amount of memory backing a region
 *
 * @gpu_addr: The memory region to modify
 * @pages:    The number of physical pages that should be present
 *
 * The ioctl may return on the following error codes or 0 for success:
 *   -ENOMEM: Out of memory
 *   -EINVAL: Invalid arguments
 */
struct kbase_ioctl_mem_commit {
	__u64 gpu_addr;
	__u64 pages;
};

#define KBASE_IOCTL_MEM_COMMIT \
	_IOW(KBASE_IOCTL_TYPE, 20, struct kbase_ioctl_mem_commit)

/**
 * union kbase_ioctl_mem_alias - Create an alias of memory regions
 * @flags: Flags, see BASE_MEM_xxx
 * @stride: Bytes between start of each memory region
 * @nents: The number of regions to pack together into the alias
 * @aliasing_info: Pointer to an array of struct base_mem_aliasing_info
 * @gpu_va: Address of the new alias
 * @va_pages: Size of the new alias
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_mem_alias {
	struct {
		__u64 flags;
		__u64 stride;
		__u64 nents;
		__u64 aliasing_info;
	} in;
	struct {
		__u64 flags;
		__u64 gpu_va;
		__u64 va_pages;
	} out;
};

#define KBASE_IOCTL_MEM_ALIAS \
	_IOWR(KBASE_IOCTL_TYPE, 21, union kbase_ioctl_mem_alias)

/**
 * union kbase_ioctl_mem_import - Import memory for use by the GPU
 * @flags: Flags, see BASE_MEM_xxx
 * @phandle: Handle to the external memory
 * @type: Type of external memory, see base_mem_import_type
 * @padding: Amount of extra VA pages to append to the imported buffer
 * @gpu_va: Address of the new alias
 * @va_pages: Size of the new alias
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_mem_import {
	struct {
		__u64 flags;
		__u64 phandle;
		__u32 type;
		__u32 padding;
	} in;
	struct {
		__u64 flags;
		__u64 gpu_va;
		__u64 va_pages;
	} out;
};

#define KBASE_IOCTL_MEM_IMPORT \
	_IOWR(KBASE_IOCTL_TYPE, 22, union kbase_ioctl_mem_import)

/**
 * struct kbase_ioctl_mem_flags_change - Change the flags for a memory region
 * @gpu_va: The GPU region to modify
 * @flags: The new flags to set
 * @mask: Mask of the flags to modify
 */
struct kbase_ioctl_mem_flags_change {
	__u64 gpu_va;
	__u64 flags;
	__u64 mask;
};

#define KBASE_IOCTL_MEM_FLAGS_CHANGE \
	_IOW(KBASE_IOCTL_TYPE, 23, struct kbase_ioctl_mem_flags_change)

/**
 * struct kbase_ioctl_stream_create - Create a synchronisation stream
 * @name: A name to identify this stream. Must be NULL-terminated.
 *
 * Note that this is also called a "timeline", but is named stream to avoid
 * confusion with other uses of the word.
 *
 * Unused bytes in @name (after the first NULL byte) must be also be NULL bytes.
 *
 * The ioctl returns a file descriptor.
 */
struct kbase_ioctl_stream_create {
	char name[32];
};

#define KBASE_IOCTL_STREAM_CREATE \
	_IOW(KBASE_IOCTL_TYPE, 24, struct kbase_ioctl_stream_create)

/**
 * struct kbase_ioctl_fence_validate - Validate a fd refers to a fence
 * @fd: The file descriptor to validate
 */
struct kbase_ioctl_fence_validate {
	int fd;
};

#define KBASE_IOCTL_FENCE_VALIDATE \
	_IOW(KBASE_IOCTL_TYPE, 25, struct kbase_ioctl_fence_validate)

/**
 * struct kbase_ioctl_mem_profile_add - Provide profiling information to kernel
 * @buffer: Pointer to the information
 * @len: Length
 * @padding: Padding
 *
 * The data provided is accessible through a debugfs file
 */
struct kbase_ioctl_mem_profile_add {
	__u64 buffer;
	__u32 len;
	__u32 padding;
};

#define KBASE_IOCTL_MEM_PROFILE_ADD \
	_IOW(KBASE_IOCTL_TYPE, 27, struct kbase_ioctl_mem_profile_add)

/**
 * struct kbase_ioctl_soft_event_update - Update the status of a soft-event
 * @event: GPU address of the event which has been updated
 * @new_status: The new status to set
 * @flags: Flags for future expansion
 */
struct kbase_ioctl_soft_event_update {
	__u64 event;
	__u32 new_status;
	__u32 flags;
};

#define KBASE_IOCTL_SOFT_EVENT_UPDATE \
	_IOW(KBASE_IOCTL_TYPE, 28, struct kbase_ioctl_soft_event_update)

/**
 * struct kbase_ioctl_sticky_resource_map - Permanently map an external resource
 * @count: Number of resources
 * @address: Array of u64 GPU addresses of the external resources to map
 */
struct kbase_ioctl_sticky_resource_map {
	__u64 count;
	__u64 address;
};

#define KBASE_IOCTL_STICKY_RESOURCE_MAP \
	_IOW(KBASE_IOCTL_TYPE, 29, struct kbase_ioctl_sticky_resource_map)

/**
 * struct kbase_ioctl_sticky_resource_map - Unmap a resource mapped which was
 *                                          previously permanently mapped
 * @count: Number of resources
 * @address: Array of u64 GPU addresses of the external resources to unmap
 */
struct kbase_ioctl_sticky_resource_unmap {
	__u64 count;
	__u64 address;
};

#define KBASE_IOCTL_STICKY_RESOURCE_UNMAP \
	_IOW(KBASE_IOCTL_TYPE, 30, struct kbase_ioctl_sticky_resource_unmap)

/**
 * union kbase_ioctl_mem_find_gpu_start_and_offset - Find the start address of
 *                                                   the GPU memory region for
 *                                                   the given gpu address and
 *                                                   the offset of that address
 *                                                   into the region
 *
 * @gpu_addr: GPU virtual address
 * @size: Size in bytes within the region
 * @start: Address of the beginning of the memory region enclosing @gpu_addr
 *         for the length of @offset bytes
 * @offset: The offset from the start of the memory region to @gpu_addr
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_mem_find_gpu_start_and_offset {
	struct {
		__u64 gpu_addr;
		__u64 size;
	} in;
	struct {
		__u64 start;
		__u64 offset;
	} out;
};

#define KBASE_IOCTL_MEM_FIND_GPU_START_AND_OFFSET \
	_IOWR(KBASE_IOCTL_TYPE, 31, union kbase_ioctl_mem_find_gpu_start_and_offset)


#define KBASE_IOCTL_CINSTR_GWT_START \
	_IO(KBASE_IOCTL_TYPE, 33)

#define KBASE_IOCTL_CINSTR_GWT_STOP \
	_IO(KBASE_IOCTL_TYPE, 34)

/**
 * union kbase_ioctl_gwt_dump - Used to collect all GPU write fault addresses.
 * @addr_buffer: Address of buffer to hold addresses of gpu modified areas.
 * @size_buffer: Address of buffer to hold size of modified areas (in pages)
 * @len: Number of addresses the buffers can hold.
 * @more_data_available: Status indicating if more addresses are available.
 * @no_of_addr_collected: Number of addresses collected into addr_buffer.
 *
 * @in: Input parameters
 * @out: Output parameters
 *
 * This structure is used when performing a call to dump GPU write fault
 * addresses.
 */
union kbase_ioctl_cinstr_gwt_dump {
	struct {
		__u64 addr_buffer;
		__u64 size_buffer;
		__u32 len;
		__u32 padding;

	} in;
	struct {
		__u32 no_of_addr_collected;
		__u8 more_data_available;
		__u8 padding[27];
	} out;
};

#define KBASE_IOCTL_CINSTR_GWT_DUMP \
	_IOWR(KBASE_IOCTL_TYPE, 35, union kbase_ioctl_cinstr_gwt_dump)

#define KBASE_IOCTL_KINSTR_JM_FD \
	_IO(KBASE_IOCTL_TYPE, 52)

/**
 * struct kbase_ioctl_mem_exec_init - Initialise the EXEC_VA memory zone
 *
 * @va_pages: Number of VA pages to reserve for EXEC_VA
 */
struct kbase_ioctl_mem_exec_init {
	__u64 va_pages;
};

#define KBASE_IOCTL_MEM_EXEC_INIT \
	_IOW(KBASE_IOCTL_TYPE, 38, struct kbase_ioctl_mem_exec_init)

/************************
 * MALI_SEC_INTEGRATION *
 ************************/
/* IOCTLs 36-41 are reserved */
/* IOCTL 42 is free for use */

/*
 * struct kbase_ioctl_slsi_combination_boost_flags - Update the status of combination boost flag
 * @flags: Flags for future expansion
 */
struct kbase_ioctl_slsi_combination_boost_flags {
	__u32 flags;
};

#define KBASE_IOCTL_SLSI_COMBINATION_BOOST_FLAGS \
	_IOW(KBASE_IOCTL_TYPE, 42, struct kbase_ioctl_slsi_combination_boost_flags)

/*
 * struct kbase_ioctl_slsi_vk_boost_flags - Update the status of vk boost flag
 * @flags: Flags for future expansion
 */
struct kbase_ioctl_slsi_vk_boost_flags {
	__u32 flags;
};

#define KBASE_IOCTL_SLSI_VK_BOOST_FLAGS \
	_IOW(KBASE_IOCTL_TYPE, 43, struct kbase_ioctl_slsi_vk_boost_flags)

/***************
 * test ioctls *
 ***************/
#if MALI_UNIT_TEST
/* These ioctls are purely for test purposes and are not used in the production
 * driver, they therefore may change without notice
 */

#define KBASE_IOCTL_TEST_TYPE (KBASE_IOCTL_TYPE + 1)

/**
 * struct kbase_ioctl_tlstream_test - Start a timeline stream test
 *
 * @tpw_count: number of trace point writers in each context
 * @msg_delay: time delay between tracepoints from one writer in milliseconds
 * @msg_count: number of trace points written by one writer
 * @aux_msg:   if non-zero aux messages will be included
 */
struct kbase_ioctl_tlstream_test {
	__u32 tpw_count;
	__u32 msg_delay;
	__u32 msg_count;
	__u32 aux_msg;
};

#define KBASE_IOCTL_TLSTREAM_TEST \
	_IOW(KBASE_IOCTL_TEST_TYPE, 1, struct kbase_ioctl_tlstream_test)

/**
 * struct kbase_ioctl_tlstream_stats - Read tlstream stats for test purposes
 * @bytes_collected: number of bytes read by user
 * @bytes_generated: number of bytes generated by tracepoints
 */
struct kbase_ioctl_tlstream_stats {
	__u32 bytes_collected;
	__u32 bytes_generated;
};

#define KBASE_IOCTL_TLSTREAM_STATS \
	_IOR(KBASE_IOCTL_TEST_TYPE, 2, struct kbase_ioctl_tlstream_stats)

/**
 * struct kbase_ioctl_cs_event_memory_write - Write an event memory address
 * @cpu_addr: Memory address to write
 * @value: Value to write
 * @padding: Currently unused, must be zero
 */
struct kbase_ioctl_cs_event_memory_write {
	__u64 cpu_addr;
	__u8 value;
	__u8 padding[7];
};

/**
 * union kbase_ioctl_cs_event_memory_read - Read an event memory address
 * @cpu_addr: Memory address to read
 * @value: Value read
 * @padding: Currently unused, must be zero
 *
 * @in: Input parameters
 * @out: Output parameters
 */
union kbase_ioctl_cs_event_memory_read {
	struct {
		__u64 cpu_addr;
	} in;
	struct {
		__u8 value;
		__u8 padding[7];
	} out;
};

#endif

/* Customer extension range */
#define KBASE_IOCTL_EXTRA_TYPE (KBASE_IOCTL_TYPE + 2)

/* If the integration needs extra ioctl add them there
 * like this:
 *
 * struct my_ioctl_args {
 *  ....
 * }
 *
 * #define KBASE_IOCTL_MY_IOCTL \
 *         _IOWR(KBASE_IOCTL_EXTRA_TYPE, 0, struct my_ioctl_args)
 */


/**********************************
 * Definitions for GPU properties *
 **********************************/
#define KBASE_GPUPROP_VALUE_SIZE_U8	(0x0)
#define KBASE_GPUPROP_VALUE_SIZE_U16	(0x1)
#define KBASE_GPUPROP_VALUE_SIZE_U32	(0x2)
#define KBASE_GPUPROP_VALUE_SIZE_U64	(0x3)

#define KBASE_GPUPROP_PRODUCT_ID			1
#define KBASE_GPUPROP_VERSION_STATUS			2
#define KBASE_GPUPROP_MINOR_REVISION			3
#define KBASE_GPUPROP_MAJOR_REVISION			4
/* 5 previously used for GPU speed */
#define KBASE_GPUPROP_GPU_FREQ_KHZ_MAX			6
/* 7 previously used for minimum GPU speed */
#define KBASE_GPUPROP_LOG2_PROGRAM_COUNTER_SIZE		8
#define KBASE_GPUPROP_TEXTURE_FEATURES_0		9
#define KBASE_GPUPROP_TEXTURE_FEATURES_1		10
#define KBASE_GPUPROP_TEXTURE_FEATURES_2		11
#define KBASE_GPUPROP_GPU_AVAILABLE_MEMORY_SIZE		12

#define KBASE_GPUPROP_L2_LOG2_LINE_SIZE			13
#define KBASE_GPUPROP_L2_LOG2_CACHE_SIZE		14
#define KBASE_GPUPROP_L2_NUM_L2_SLICES			15

#define KBASE_GPUPROP_TILER_BIN_SIZE_BYTES		16
#define KBASE_GPUPROP_TILER_MAX_ACTIVE_LEVELS		17

#define KBASE_GPUPROP_MAX_THREADS			18
#define KBASE_GPUPROP_MAX_WORKGROUP_SIZE		19
#define KBASE_GPUPROP_MAX_BARRIER_SIZE			20
#define KBASE_GPUPROP_MAX_REGISTERS			21
#define KBASE_GPUPROP_MAX_TASK_QUEUE			22
#define KBASE_GPUPROP_MAX_THREAD_GROUP_SPLIT		23
#define KBASE_GPUPROP_IMPL_TECH				24

#define KBASE_GPUPROP_RAW_SHADER_PRESENT		25
#define KBASE_GPUPROP_RAW_TILER_PRESENT			26
#define KBASE_GPUPROP_RAW_L2_PRESENT			27
#define KBASE_GPUPROP_RAW_STACK_PRESENT			28
#define KBASE_GPUPROP_RAW_L2_FEATURES			29
#define KBASE_GPUPROP_RAW_CORE_FEATURES			30
#define KBASE_GPUPROP_RAW_MEM_FEATURES			31
#define KBASE_GPUPROP_RAW_MMU_FEATURES			32
#define KBASE_GPUPROP_RAW_AS_PRESENT			33
#define KBASE_GPUPROP_RAW_JS_PRESENT			34
#define KBASE_GPUPROP_RAW_JS_FEATURES_0			35
#define KBASE_GPUPROP_RAW_JS_FEATURES_1			36
#define KBASE_GPUPROP_RAW_JS_FEATURES_2			37
#define KBASE_GPUPROP_RAW_JS_FEATURES_3			38
#define KBASE_GPUPROP_RAW_JS_FEATURES_4			39
#define KBASE_GPUPROP_RAW_JS_FEATURES_5			40
#define KBASE_GPUPROP_RAW_JS_FEATURES_6			41
#define KBASE_GPUPROP_RAW_JS_FEATURES_7			42
#define KBASE_GPUPROP_RAW_JS_FEATURES_8			43
#define KBASE_GPUPROP_RAW_JS_FEATURES_9			44
#define KBASE_GPUPROP_RAW_JS_FEATURES_10		45
#define KBASE_GPUPROP_RAW_JS_FEATURES_11		46
#define KBASE_GPUPROP_RAW_JS_FEATURES_12		47
#define KBASE_GPUPROP_RAW_JS_FEATURES_13		48
#define KBASE_GPUPROP_RAW_JS_FEATURES_14		49
#define KBASE_GPUPROP_RAW_JS_FEATURES_15		50
#define KBASE_GPUPROP_RAW_TILER_FEATURES		51
#define KBASE_GPUPROP_RAW_TEXTURE_FEATURES_0		52
#define KBASE_GPUPROP_RAW_TEXTURE_FEATURES_1		53
#define KBASE_GPUPROP_RAW_TEXTURE_FEATURES_2		54
#define KBASE_GPUPROP_RAW_GPU_ID			55
#define KBASE_GPUPROP_RAW_THREAD_MAX_THREADS		56
#define KBASE_GPUPROP_RAW_THREAD_MAX_WORKGROUP_SIZE	57
#define KBASE_GPUPROP_RAW_THREAD_MAX_BARRIER_SIZE	58
#define KBASE_GPUPROP_RAW_THREAD_FEATURES		59
#define KBASE_GPUPROP_RAW_COHERENCY_MODE		60

#define KBASE_GPUPROP_COHERENCY_NUM_GROUPS		61
#define KBASE_GPUPROP_COHERENCY_NUM_CORE_GROUPS		62
#define KBASE_GPUPROP_COHERENCY_COHERENCY		63
#define KBASE_GPUPROP_COHERENCY_GROUP_0			64
#define KBASE_GPUPROP_COHERENCY_GROUP_1			65
#define KBASE_GPUPROP_COHERENCY_GROUP_2			66
#define KBASE_GPUPROP_COHERENCY_GROUP_3			67
#define KBASE_GPUPROP_COHERENCY_GROUP_4			68
#define KBASE_GPUPROP_COHERENCY_GROUP_5			69
#define KBASE_GPUPROP_COHERENCY_GROUP_6			70
#define KBASE_GPUPROP_COHERENCY_GROUP_7			71
#define KBASE_GPUPROP_COHERENCY_GROUP_8			72
#define KBASE_GPUPROP_COHERENCY_GROUP_9			73
#define KBASE_GPUPROP_COHERENCY_GROUP_10		74
#define KBASE_GPUPROP_COHERENCY_GROUP_11		75
#define KBASE_GPUPROP_COHERENCY_GROUP_12		76
#define KBASE_GPUPROP_COHERENCY_GROUP_13		77
#define KBASE_GPUPROP_COHERENCY_GROUP_14		78
#define KBASE_GPUPROP_COHERENCY_GROUP_15		79

#define KBASE_GPUPROP_TEXTURE_FEATURES_3		80
#define KBASE_GPUPROP_RAW_TEXTURE_FEATURES_3		81

#define KBASE_GPUPROP_NUM_EXEC_ENGINES                  82

#define KBASE_GPUPROP_RAW_THREAD_TLS_ALLOC		83
#define KBASE_GPUPROP_TLS_ALLOC				84

#ifdef __cpluscplus
}
#endif

#endif
