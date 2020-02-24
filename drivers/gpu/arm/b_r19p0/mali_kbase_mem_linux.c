/*
 *
 * (C) COPYRIGHT 2010-2019 ARM Limited. All rights reserved.
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



/**
 * @file mali_kbase_mem_linux.c
 * Base kernel memory APIs, Linux implementation.
 */

#include <linux/compat.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(4, 8, 0))
#include <linux/dma-attrs.h>
#endif /* LINUX_VERSION_CODE >= 3.5.0 && < 4.8.0 */
#ifdef CONFIG_DMA_SHARED_BUFFER
#include <linux/dma-buf.h>
#endif				/* defined(CONFIG_DMA_SHARED_BUFFER) */
#include <linux/shrinker.h>
#include <linux/cache.h>
#include <linux/memory_group_manager.h>

#if KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE
#include <linux/ion.h>
#else
#include <linux/ion_exynos.h>
#endif

#include <mali_kbase.h>
#include <mali_kbase_mem_linux.h>
#include <mali_kbase_tracepoints.h>
#include <mali_kbase_ioctl.h>

#if KERNEL_VERSION(4, 17, 2) > LINUX_VERSION_CODE
/* Enable workaround for ion for versions prior to v4.17.2 to avoid the potentially
 * disruptive warnings which can come if begin_cpu_access and end_cpu_access
 * methods are not called in pairs.
 *
 * dma_sync_sg_for_* calls will be made directly as a workaround.
 *
 * Note that some long term maintenance kernel versions (e.g. 4.9.x, 4.14.x) only require this
 * workaround on their earlier releases. However it is still safe to use it on such releases, and
 * it simplifies the version check.
 *
 * This will also address the case on kernels prior to 4.12, where ion lacks
 * the cache maintenance in begin_cpu_access and end_cpu_access methods.
 */
#define KBASE_MEM_ION_SYNC_WORKAROUND
#endif


static int kbase_vmap_phy_pages(struct kbase_context *kctx,
		struct kbase_va_region *reg, u64 offset_bytes, size_t size,
		struct kbase_vmap_struct *map);
static void kbase_vunmap_phy_pages(struct kbase_context *kctx,
		struct kbase_vmap_struct *map);

static int kbase_tracking_page_setup(struct kbase_context *kctx, struct vm_area_struct *vma);

/* Retrieve the associated region pointer if the GPU address corresponds to
 * one of the event memory pages. The enclosing region, if found, shouldn't
 * have been marked as free.
 */
static struct kbase_va_region *kbase_find_event_mem_region(
			struct kbase_context *kctx, u64 gpu_addr)
{

	return NULL;
}

/**
 * kbase_phy_alloc_mapping_init - Initialize the kernel side permanent mapping
 *                                of the physical allocation belonging to a
 *                                region
 * @kctx:  The kernel base context @reg belongs to.
 * @reg:   The region whose physical allocation is to be mapped
 * @vsize: The size of the requested region, in pages
 * @size:  The size in pages initially committed to the region
 *
 * Return: 0 on success, otherwise an error code indicating failure
 *
 * Maps the physical allocation backing a non-free @reg, so it may be
 * accessed directly from the kernel. This is only supported for physical
 * allocations of type KBASE_MEM_TYPE_NATIVE, and will fail for other types of
 * physical allocation.
 *
 * The mapping is stored directly in the allocation that backs @reg. The
 * refcount is not incremented at this point. Instead, use of the mapping should
 * be surrounded by kbase_phy_alloc_mapping_get() and
 * kbase_phy_alloc_mapping_put() to ensure it does not disappear whilst the
 * client is accessing it.
 *
 * Both cached and uncached regions are allowed, but any sync operations are the
 * responsibility of the client using the permanent mapping.
 *
 * A number of checks are made to ensure that a region that needs a permanent
 * mapping can actually be supported:
 * - The region must be created as fully backed
 * - The region must not be growable
 *
 * This function will fail if those checks are not satisfied.
 *
 * On success, the region will also be forced into a certain kind:
 * - It will no longer be growable
 */
static int kbase_phy_alloc_mapping_init(struct kbase_context *kctx,
		struct kbase_va_region *reg, size_t vsize, size_t size)
{
	size_t size_bytes = (size << PAGE_SHIFT);
	struct kbase_vmap_struct *kern_mapping;
	int err = 0;

	/* Can only map in regions that are always fully committed
	 * Don't setup the mapping twice
	 * Only support KBASE_MEM_TYPE_NATIVE allocations
	 */
	if (vsize != size || reg->cpu_alloc->permanent_map != NULL ||
			reg->cpu_alloc->type != KBASE_MEM_TYPE_NATIVE)
		return -EINVAL;

	if (size > (KBASE_PERMANENTLY_MAPPED_MEM_LIMIT_PAGES -
			kctx->permanent_mapped_pages)) {
		dev_warn(kctx->kbdev->dev, "Request for %llu more pages mem needing a permanent mapping would breach limit %lu, currently at %lu pages",
				(u64)size,
				KBASE_PERMANENTLY_MAPPED_MEM_LIMIT_PAGES,
				kctx->permanent_mapped_pages);
		return -ENOMEM;
	}

	kern_mapping = kzalloc(sizeof(*kern_mapping), GFP_KERNEL);
	if (!kern_mapping)
		return -ENOMEM;

	err = kbase_vmap_phy_pages(kctx, reg, 0u, size_bytes, kern_mapping);
	if (err < 0)
		goto vmap_fail;

	/* No support for growing or shrinking mapped regions */
	reg->flags &= ~KBASE_REG_GROWABLE;

	reg->cpu_alloc->permanent_map = kern_mapping;
	kctx->permanent_mapped_pages += size;

	return 0;
vmap_fail:
	kfree(kern_mapping);
	return err;
}

void kbase_phy_alloc_mapping_term(struct kbase_context *kctx,
		struct kbase_mem_phy_alloc *alloc)
{
	WARN_ON(!alloc->permanent_map);
	kbase_vunmap_phy_pages(kctx, alloc->permanent_map);
	kfree(alloc->permanent_map);

	alloc->permanent_map = NULL;

	/* Mappings are only done on cpu_alloc, so don't need to worry about
	 * this being reduced a second time if a separate gpu_alloc is
	 * freed
	 */
	WARN_ON(alloc->nents > kctx->permanent_mapped_pages);
	kctx->permanent_mapped_pages -= alloc->nents;
}

void *kbase_phy_alloc_mapping_get(struct kbase_context *kctx,
		u64 gpu_addr,
		struct kbase_vmap_struct **out_kern_mapping)
{
	struct kbase_va_region *reg;
	void *kern_mem_ptr = NULL;
	struct kbase_vmap_struct *kern_mapping;
	u64 mapping_offset;

	WARN_ON(!kctx);
	WARN_ON(!out_kern_mapping);

	kbase_gpu_vm_lock(kctx);

	/* First do a quick lookup in the list of event memory regions */
	reg = kbase_find_event_mem_region(kctx, gpu_addr);

	if (!reg) {
		reg = kbase_region_tracker_find_region_enclosing_address(
			kctx, gpu_addr);
	}

	if (kbase_is_region_invalid_or_free(reg))
		goto out_unlock;

	kern_mapping = reg->cpu_alloc->permanent_map;
	if (kern_mapping == NULL)
		goto out_unlock;

	mapping_offset = gpu_addr - (reg->start_pfn << PAGE_SHIFT);

	/* Refcount the allocations to prevent them disappearing */
	WARN_ON(reg->cpu_alloc != kern_mapping->cpu_alloc);
	WARN_ON(reg->gpu_alloc != kern_mapping->gpu_alloc);
	(void)kbase_mem_phy_alloc_get(kern_mapping->cpu_alloc);
	(void)kbase_mem_phy_alloc_get(kern_mapping->gpu_alloc);

	kern_mem_ptr = (void *)(uintptr_t)((uintptr_t)kern_mapping->addr + mapping_offset);
	*out_kern_mapping = kern_mapping;
out_unlock:
	kbase_gpu_vm_unlock(kctx);
	return kern_mem_ptr;
}

void kbase_phy_alloc_mapping_put(struct kbase_context *kctx,
		struct kbase_vmap_struct *kern_mapping)
{
	WARN_ON(!kctx);
	WARN_ON(!kern_mapping);

	WARN_ON(kctx != kern_mapping->cpu_alloc->imported.native.kctx);
	WARN_ON(kern_mapping != kern_mapping->cpu_alloc->permanent_map);

	kbase_mem_phy_alloc_put(kern_mapping->cpu_alloc);
	kbase_mem_phy_alloc_put(kern_mapping->gpu_alloc);

	/* kern_mapping and the gpu/cpu phy allocs backing it must not be used
	 * from now on
	 */
}

struct kbase_va_region *kbase_mem_alloc(struct kbase_context *kctx,
		u64 va_pages, u64 commit_pages, u64 extent, u64 *flags,
		u64 *gpu_va)
{
	int zone;
	struct kbase_va_region *reg;
	struct rb_root *rbtree;
	struct device *dev;

	KBASE_DEBUG_ASSERT(kctx);
	KBASE_DEBUG_ASSERT(flags);
	KBASE_DEBUG_ASSERT(gpu_va);

	dev = kctx->kbdev->dev;
	dev_dbg(dev, "Allocating %lld va_pages, %lld commit_pages, %lld extent, 0x%llX flags\n",
		va_pages, commit_pages, extent, *flags);

	*gpu_va = 0; /* return 0 on failure */

	if (!kbase_check_alloc_flags(*flags)) {
		dev_warn(dev,
				"kbase_mem_alloc called with bad flags (%llx)",
				(unsigned long long)*flags);
		goto bad_flags;
	}

#ifdef CONFIG_DEBUG_FS
	if (unlikely(kbase_ctx_flag(kctx, KCTX_INFINITE_CACHE))) {
		/* Mask coherency flags if infinite cache is enabled to prevent
		 * the skipping of syncs from BASE side.
		 */
		*flags &= ~(BASE_MEM_COHERENT_SYSTEM_REQUIRED |
			    BASE_MEM_COHERENT_SYSTEM);
	}
#endif

	if ((*flags & BASE_MEM_UNCACHED_GPU) != 0 &&
			(*flags & BASE_MEM_COHERENT_SYSTEM_REQUIRED) != 0) {
		/* Remove COHERENT_SYSTEM_REQUIRED flag if uncached GPU mapping is requested */
		*flags &= ~BASE_MEM_COHERENT_SYSTEM_REQUIRED;
	}
	if ((*flags & BASE_MEM_COHERENT_SYSTEM_REQUIRED) != 0 &&
			!kbase_device_is_cpu_coherent(kctx->kbdev)) {
		dev_warn(dev, "kbase_mem_alloc call required coherent mem when unavailable");
		goto bad_flags;
	}
	if ((*flags & BASE_MEM_COHERENT_SYSTEM) != 0 &&
			!kbase_device_is_cpu_coherent(kctx->kbdev)) {
		/* Remove COHERENT_SYSTEM flag if coherent mem is unavailable */
		*flags &= ~BASE_MEM_COHERENT_SYSTEM;
	}

	if (kbase_check_alloc_sizes(kctx, *flags, va_pages, commit_pages, extent))
		goto bad_sizes;

#ifdef CONFIG_MALI_MEMORY_FULLY_BACKED
	/* Ensure that memory is fully physically-backed. */
	if (*flags & BASE_MEM_GROW_ON_GPF)
		commit_pages = va_pages;
#endif

	/* find out which VA zone to use */
	if (*flags & BASE_MEM_SAME_VA) {
		rbtree = &kctx->reg_rbtree_same;
		zone = KBASE_REG_ZONE_SAME_VA;
	} else if ((*flags & BASE_MEM_PROT_GPU_EX) && kbase_has_exec_va_zone(kctx)) {
		rbtree = &kctx->reg_rbtree_exec;
		zone = KBASE_REG_ZONE_EXEC_VA;
	} else {
		rbtree = &kctx->reg_rbtree_custom;
		zone = KBASE_REG_ZONE_CUSTOM_VA;
	}

	reg = kbase_alloc_free_region(rbtree, 0, va_pages, zone);
	if (!reg) {
		dev_err(dev, "Failed to allocate free region");
		goto no_region;
	}

	if (kbase_update_region_flags(kctx, reg, *flags) != 0)
		goto invalid_flags;

	if (kbase_reg_prepare_native(reg, kctx,
				base_mem_group_id_get(*flags)) != 0) {
		dev_err(dev, "Failed to prepare region");
		goto prepare_failed;
	}

	if (*flags & (BASE_MEM_GROW_ON_GPF|BASE_MEM_TILER_ALIGN_TOP)) {
		/* kbase_check_alloc_sizes() already checks extent is valid for
		 * assigning to reg->extent */
		reg->extent = extent;
	} else {
		reg->extent = 0;
	}

	if (kbase_alloc_phy_pages(reg, va_pages, commit_pages) != 0) {
		dev_warn(dev, "Failed to allocate %lld pages (va_pages=%lld)",
				(unsigned long long)commit_pages,
				(unsigned long long)va_pages);
		goto no_mem;
	}
	reg->initial_commit = commit_pages;

	kbase_gpu_vm_lock(kctx);

	if (reg->flags & KBASE_REG_PERMANENT_KERNEL_MAPPING) {
		/* Permanent kernel mappings must happen as soon as
		 * reg->cpu_alloc->pages is ready. Currently this happens after
		 * kbase_alloc_phy_pages(). If we move that to setup pages
		 * earlier, also move this call too
		 */
		int err = kbase_phy_alloc_mapping_init(kctx, reg, va_pages,
				commit_pages);
		if (err < 0) {
			kbase_gpu_vm_unlock(kctx);
			goto no_kern_mapping;
		}
	}


	/* mmap needed to setup VA? */
	if (*flags & BASE_MEM_SAME_VA) {
		unsigned long prot = PROT_NONE;
		unsigned long va_size = va_pages << PAGE_SHIFT;
		unsigned long va_map = va_size;
		unsigned long cookie, cookie_nr;
		unsigned long cpu_addr;

		/* Bind to a cookie */
		if (!kctx->cookies) {
			dev_err(dev, "No cookies available for allocation!");
			kbase_gpu_vm_unlock(kctx);
			goto no_cookie;
		}
		/* return a cookie */
		cookie_nr = __ffs(kctx->cookies);
		kctx->cookies &= ~(1UL << cookie_nr);
		BUG_ON(kctx->pending_regions[cookie_nr]);
		kctx->pending_regions[cookie_nr] = reg;

		/* relocate to correct base */
		cookie = cookie_nr + PFN_DOWN(BASE_MEM_COOKIE_BASE);
		cookie <<= PAGE_SHIFT;

		/*
		 * 10.1-10.4 UKU userland relies on the kernel to call mmap.
		 * For all other versions we can just return the cookie
		 */
		if (kctx->api_version < KBASE_API_VERSION(10, 1) ||
		    kctx->api_version > KBASE_API_VERSION(10, 4)) {
			*gpu_va = (u64) cookie;
			kbase_gpu_vm_unlock(kctx);
			return reg;
		}

		kbase_va_region_alloc_get(kctx, reg);
		kbase_gpu_vm_unlock(kctx);

		if (*flags & BASE_MEM_PROT_CPU_RD)
			prot |= PROT_READ;
		if (*flags & BASE_MEM_PROT_CPU_WR)
			prot |= PROT_WRITE;

		cpu_addr = vm_mmap(kctx->filp, 0, va_map, prot,
				MAP_SHARED, cookie);

		kbase_gpu_vm_lock(kctx);

		/* Since vm lock was released, check if the region has already
		 * been freed meanwhile. This could happen if User was able to
		 * second guess the cookie or the CPU VA and free the region
		 * through the guessed value.
		 */
		if (reg->flags & KBASE_REG_VA_FREED) {
			kbase_va_region_alloc_put(kctx, reg);
			reg = NULL;
		} else if (IS_ERR_VALUE(cpu_addr)) {
			/* Once the vm lock is released, multiple scenarios can
			 * arise under which the cookie could get re-assigned
			 * to some other region.
			 */
			if (!WARN_ON(kctx->pending_regions[cookie_nr] &&
				     (kctx->pending_regions[cookie_nr] != reg))) {
				kctx->pending_regions[cookie_nr] = NULL;
				kctx->cookies |= (1UL << cookie_nr);
			}

			/* Region has not been freed and we can be sure that
			 * User won't be able to free the region now. So we
			 * can free it ourselves.
			 * If the region->start_pfn isn't zero then the
			 * allocation will also be unmapped from GPU side.
			 */
			kbase_mem_free_region(kctx, reg);
			kbase_va_region_alloc_put(kctx, reg);
			reg = NULL;
		} else {
			kbase_va_region_alloc_put(kctx, reg);
			*gpu_va = (u64) cpu_addr;
		}

		kbase_gpu_vm_unlock(kctx);
	} else /* we control the VA */ {
		if (kbase_gpu_mmap(kctx, reg, 0, va_pages, 1) != 0) {
			dev_warn(dev, "Failed to map memory on GPU");
			kbase_gpu_vm_unlock(kctx);
			goto no_mmap;
		}
		/* return real GPU VA */
		*gpu_va = reg->start_pfn << PAGE_SHIFT;

		kbase_gpu_vm_unlock(kctx);
	}

	return reg;

no_mmap:
no_cookie:
no_kern_mapping:
no_mem:
	kbase_mem_phy_alloc_put(reg->cpu_alloc);
	kbase_mem_phy_alloc_put(reg->gpu_alloc);
invalid_flags:
prepare_failed:
	kfree(reg);
no_region:
bad_sizes:
bad_flags:
	return NULL;
}
KBASE_EXPORT_TEST_API(kbase_mem_alloc);

int kbase_mem_query(struct kbase_context *kctx,
		u64 gpu_addr, u64 query, u64 * const out)
{
	struct kbase_va_region *reg;
	int ret = -EINVAL;

	KBASE_DEBUG_ASSERT(kctx);
	KBASE_DEBUG_ASSERT(out);

	if (gpu_addr & ~PAGE_MASK) {
		dev_warn(kctx->kbdev->dev, "mem_query: gpu_addr: passed parameter is invalid");
		return -EINVAL;
	}

	kbase_gpu_vm_lock(kctx);

	/* Validate the region */
	reg = kbase_region_tracker_find_region_base_address(kctx, gpu_addr);
	if (kbase_is_region_invalid_or_free(reg))
		goto out_unlock;

	switch (query) {
	case KBASE_MEM_QUERY_COMMIT_SIZE:
		if (reg->cpu_alloc->type != KBASE_MEM_TYPE_ALIAS) {
			*out = kbase_reg_current_backed_size(reg);
		} else {
			size_t i;
			struct kbase_aliased *aliased;
			*out = 0;
			aliased = reg->cpu_alloc->imported.alias.aliased;
			for (i = 0; i < reg->cpu_alloc->imported.alias.nents; i++)
				*out += aliased[i].length;
		}
		break;
	case KBASE_MEM_QUERY_VA_SIZE:
		*out = reg->nr_pages;
		break;
	case KBASE_MEM_QUERY_FLAGS:
	{
		*out = 0;
		if (KBASE_REG_CPU_WR & reg->flags)
			*out |= BASE_MEM_PROT_CPU_WR;
		if (KBASE_REG_CPU_RD & reg->flags)
			*out |= BASE_MEM_PROT_CPU_RD;
		if (KBASE_REG_CPU_CACHED & reg->flags)
			*out |= BASE_MEM_CACHED_CPU;
		if (KBASE_REG_GPU_WR & reg->flags)
			*out |= BASE_MEM_PROT_GPU_WR;
		if (KBASE_REG_GPU_RD & reg->flags)
			*out |= BASE_MEM_PROT_GPU_RD;
		if (!(KBASE_REG_GPU_NX & reg->flags))
			*out |= BASE_MEM_PROT_GPU_EX;
		if (KBASE_REG_SHARE_BOTH & reg->flags)
			*out |= BASE_MEM_COHERENT_SYSTEM;
		if (KBASE_REG_SHARE_IN & reg->flags)
			*out |= BASE_MEM_COHERENT_LOCAL;
		if (kctx->api_version >= KBASE_API_VERSION(11, 2)) {
			/* Prior to 11.2, these were known about by user-side
			 * but we did not return them. Returning some of these
			 * caused certain clients that were not expecting them
			 * to fail, so we omit all of them as a special-case
			 * for compatibility reasons */
			if (KBASE_REG_PF_GROW & reg->flags)
				*out |= BASE_MEM_GROW_ON_GPF;
			if (KBASE_REG_SECURE & reg->flags)
				*out |= BASE_MEM_SECURE;
		}
		if (KBASE_REG_TILER_ALIGN_TOP & reg->flags)
			*out |= BASE_MEM_TILER_ALIGN_TOP;
		if (!(KBASE_REG_GPU_CACHED & reg->flags))
			*out |= BASE_MEM_UNCACHED_GPU;
		if (KBASE_REG_GPU_VA_SAME_4GB_PAGE & reg->flags)
			*out |= BASE_MEM_GPU_VA_SAME_4GB_PAGE;

		*out |= base_mem_group_id_set(reg->cpu_alloc->group_id);

		WARN(*out & ~BASE_MEM_FLAGS_QUERYABLE,
				"BASE_MEM_FLAGS_QUERYABLE needs updating\n");
		*out &= BASE_MEM_FLAGS_QUERYABLE;
		break;
	}
	default:
		*out = 0;
		goto out_unlock;
	}

	ret = 0;

out_unlock:
	kbase_gpu_vm_unlock(kctx);
	return ret;
}

/**
 * kbase_mem_evictable_reclaim_count_objects - Count number of pages in the
 * Ephemeral memory eviction list.
 * @s:        Shrinker
 * @sc:       Shrinker control
 *
 * Return: Number of pages which can be freed.
 */
static
unsigned long kbase_mem_evictable_reclaim_count_objects(struct shrinker *s,
		struct shrink_control *sc)
{
	struct kbase_context *kctx;
	struct kbase_mem_phy_alloc *alloc;
	unsigned long pages = 0;

	kctx = container_of(s, struct kbase_context, reclaim);

	mutex_lock(&kctx->jit_evict_lock);

	list_for_each_entry(alloc, &kctx->evict_list, evict_node)
		pages += alloc->nents;

	mutex_unlock(&kctx->jit_evict_lock);
	return pages;
}

/**
 * kbase_mem_evictable_reclaim_scan_objects - Scan the Ephemeral memory eviction
 * list for pages and try to reclaim them.
 * @s:        Shrinker
 * @sc:       Shrinker control
 *
 * Return: Number of pages freed (can be less then requested) or -1 if the
 * shrinker failed to free pages in its pool.
 *
 * Note:
 * This function accesses region structures without taking the region lock,
 * this is required as the OOM killer can call the shrinker after the region
 * lock has already been held.
 * This is safe as we can guarantee that a region on the eviction list will
 * not be freed (kbase_mem_free_region removes the allocation from the list
 * before destroying it), or modified by other parts of the driver.
 * The eviction list itself is guarded by the eviction lock and the MMU updates
 * are protected by their own lock.
 */
static
unsigned long kbase_mem_evictable_reclaim_scan_objects(struct shrinker *s,
		struct shrink_control *sc)
{
	struct kbase_context *kctx;
	struct kbase_mem_phy_alloc *alloc;
	struct kbase_mem_phy_alloc *tmp;
	unsigned long freed = 0;

	kctx = container_of(s, struct kbase_context, reclaim);
	mutex_lock(&kctx->jit_evict_lock);

	list_for_each_entry_safe(alloc, tmp, &kctx->evict_list, evict_node) {
		int err;

		err = kbase_mem_shrink_gpu_mapping(kctx, alloc->reg,
				0, alloc->nents);
		if (err != 0) {
			/*
			 * Failed to remove GPU mapping, tell the shrinker
			 * to stop trying to shrink our slab even though we
			 * have pages in it.
			 */
			freed = -1;
			goto out_unlock;
		}

		/*
		 * Update alloc->evicted before freeing the backing so the
		 * helper can determine that it needs to bypass the accounting
		 * and memory pool.
		 */
		alloc->evicted = alloc->nents;

		kbase_free_phy_pages_helper(alloc, alloc->evicted);
		freed += alloc->evicted;
		list_del_init(&alloc->evict_node);

		/*
		 * Inform the JIT allocator this region has lost backing
		 * as it might need to free the allocation.
		 */
		kbase_jit_backing_lost(alloc->reg);

		/* Enough pages have been freed so stop now */
		if (freed > sc->nr_to_scan)
			break;
	}
out_unlock:
	mutex_unlock(&kctx->jit_evict_lock);

	return freed;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
static int kbase_mem_evictable_reclaim_shrink(struct shrinker *s,
		struct shrink_control *sc)
{
	if (sc->nr_to_scan == 0)
		return kbase_mem_evictable_reclaim_count_objects(s, sc);

	return kbase_mem_evictable_reclaim_scan_objects(s, sc);
}
#endif

int kbase_mem_evictable_init(struct kbase_context *kctx)
{
	INIT_LIST_HEAD(&kctx->evict_list);
	mutex_init(&kctx->jit_evict_lock);

	/* Register shrinker */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
	kctx->reclaim.shrink = kbase_mem_evictable_reclaim_shrink;
#else
	kctx->reclaim.count_objects = kbase_mem_evictable_reclaim_count_objects;
	kctx->reclaim.scan_objects = kbase_mem_evictable_reclaim_scan_objects;
#endif
	kctx->reclaim.seeks = DEFAULT_SEEKS;
	/* Kernel versions prior to 3.1 :
	 * struct shrinker does not define batch */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0)
	kctx->reclaim.batch = 0;
#endif
	register_shrinker(&kctx->reclaim);
	return 0;
}

void kbase_mem_evictable_deinit(struct kbase_context *kctx)
{
	unregister_shrinker(&kctx->reclaim);
}

/**
 * kbase_mem_evictable_mark_reclaim - Mark the pages as reclaimable.
 * @alloc: The physical allocation
 */
void kbase_mem_evictable_mark_reclaim(struct kbase_mem_phy_alloc *alloc)
{
	struct kbase_context *kctx = alloc->imported.native.kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	int __maybe_unused new_page_count;

	kbase_process_page_usage_dec(kctx, alloc->nents);
	new_page_count = atomic_sub_return(alloc->nents,
		&kctx->used_pages);
	atomic_sub(alloc->nents, &kctx->kbdev->memdev.used_pages);

	KBASE_TLSTREAM_AUX_PAGESALLOC(
			kbdev,
			kctx->id,
			(u64)new_page_count);
}

/**
 * kbase_mem_evictable_unmark_reclaim - Mark the pages as no longer reclaimable.
 * @alloc: The physical allocation
 */
static
void kbase_mem_evictable_unmark_reclaim(struct kbase_mem_phy_alloc *alloc)
{
	struct kbase_context *kctx = alloc->imported.native.kctx;
	struct kbase_device *kbdev = kctx->kbdev;
	int __maybe_unused new_page_count;

	new_page_count = atomic_add_return(alloc->nents,
		&kctx->used_pages);
	atomic_add(alloc->nents, &kctx->kbdev->memdev.used_pages);

	/* Increase mm counters so that the allocation is accounted for
	 * against the process and thus is visible to the OOM killer,
	 */
	kbase_process_page_usage_inc(kctx, alloc->nents);

	KBASE_TLSTREAM_AUX_PAGESALLOC(
			kbdev,
			kctx->id,
			(u64)new_page_count);
}

int kbase_mem_evictable_make(struct kbase_mem_phy_alloc *gpu_alloc)
{
	struct kbase_context *kctx = gpu_alloc->imported.native.kctx;

	lockdep_assert_held(&kctx->reg_lock);

	kbase_mem_shrink_cpu_mapping(kctx, gpu_alloc->reg,
			0, gpu_alloc->nents);

	mutex_lock(&kctx->jit_evict_lock);
	/* This allocation can't already be on a list. */
	WARN_ON(!list_empty(&gpu_alloc->evict_node));

	/*
	 * Add the allocation to the eviction list, after this point the shrink
	 * can reclaim it.
	 */
	list_add(&gpu_alloc->evict_node, &kctx->evict_list);
	mutex_unlock(&kctx->jit_evict_lock);
	kbase_mem_evictable_mark_reclaim(gpu_alloc);

	gpu_alloc->reg->flags |= KBASE_REG_DONT_NEED;
	return 0;
}

bool kbase_mem_evictable_unmake(struct kbase_mem_phy_alloc *gpu_alloc)
{
	struct kbase_context *kctx = gpu_alloc->imported.native.kctx;
	int err = 0;

	lockdep_assert_held(&kctx->reg_lock);

	mutex_lock(&kctx->jit_evict_lock);
	/*
	 * First remove the allocation from the eviction list as it's no
	 * longer eligible for eviction.
	 */
	list_del_init(&gpu_alloc->evict_node);
	mutex_unlock(&kctx->jit_evict_lock);

	if (gpu_alloc->evicted == 0) {
		/*
		 * The backing is still present, update the VM stats as it's
		 * in use again.
		 */
		kbase_mem_evictable_unmark_reclaim(gpu_alloc);
	} else {
		/* If the region is still alive ... */
		if (gpu_alloc->reg) {
			/* ... allocate replacement backing ... */
			err = kbase_alloc_phy_pages_helper(gpu_alloc,
					gpu_alloc->evicted);

			/*
			 * ... and grow the mapping back to its
			 * pre-eviction size.
			 */
			if (!err)
				err = kbase_mem_grow_gpu_mapping(kctx,
						gpu_alloc->reg,
						gpu_alloc->evicted, 0);

			gpu_alloc->evicted = 0;
		}
	}

	/* If the region is still alive remove the DONT_NEED attribute. */
	if (gpu_alloc->reg)
		gpu_alloc->reg->flags &= ~KBASE_REG_DONT_NEED;

	return (err == 0);
}

int kbase_mem_flags_change(struct kbase_context *kctx, u64 gpu_addr, unsigned int flags, unsigned int mask)
{
	struct kbase_va_region *reg;
	int ret = -EINVAL;
	unsigned int real_flags = 0;
	unsigned int new_flags = 0;
	bool prev_needed, new_needed;

	KBASE_DEBUG_ASSERT(kctx);

	if (!gpu_addr)
		return -EINVAL;

	if ((gpu_addr & ~PAGE_MASK) && (gpu_addr >= PAGE_SIZE))
		return -EINVAL;

	/* nuke other bits */
	flags &= mask;

	/* check for only supported flags */
	if (flags & ~(BASE_MEM_FLAGS_MODIFIABLE))
		goto out;

	/* mask covers bits we don't support? */
	if (mask & ~(BASE_MEM_FLAGS_MODIFIABLE))
		goto out;

	/* convert flags */
	if (BASE_MEM_COHERENT_SYSTEM & flags)
		real_flags |= KBASE_REG_SHARE_BOTH;
	else if (BASE_MEM_COHERENT_LOCAL & flags)
		real_flags |= KBASE_REG_SHARE_IN;

	/* now we can lock down the context, and find the region */
	down_write(&current->mm->mmap_sem);
	kbase_gpu_vm_lock(kctx);

	/* Validate the region */
	reg = kbase_region_tracker_find_region_base_address(kctx, gpu_addr);
	if (kbase_is_region_invalid_or_free(reg))
		goto out_unlock;

	/* Is the region being transitioning between not needed and needed? */
	prev_needed = (KBASE_REG_DONT_NEED & reg->flags) == KBASE_REG_DONT_NEED;
	new_needed = (BASE_MEM_DONT_NEED & flags) == BASE_MEM_DONT_NEED;
	if (prev_needed != new_needed) {
		/* Aliased allocations can't be made ephemeral */
		if (atomic_read(&reg->cpu_alloc->gpu_mappings) > 1)
			goto out_unlock;

		if (new_needed) {
			/* Only native allocations can be marked not needed */
			if (reg->cpu_alloc->type != KBASE_MEM_TYPE_NATIVE) {
				ret = -EINVAL;
				goto out_unlock;
			}
			ret = kbase_mem_evictable_make(reg->gpu_alloc);
			if (ret)
				goto out_unlock;
		} else {
			kbase_mem_evictable_unmake(reg->gpu_alloc);
		}
	}

	/* limit to imported memory */
	if (reg->gpu_alloc->type != KBASE_MEM_TYPE_IMPORTED_UMM)
		goto out_unlock;

	/* shareability flags are ignored for GPU uncached memory */
	if (!(reg->flags & KBASE_REG_GPU_CACHED)) {
		ret = 0;
		goto out_unlock;
	}

	/* no change? */
	if (real_flags == (reg->flags & (KBASE_REG_SHARE_IN | KBASE_REG_SHARE_BOTH))) {
		ret = 0;
		goto out_unlock;
	}

	new_flags = reg->flags & ~(KBASE_REG_SHARE_IN | KBASE_REG_SHARE_BOTH);
	new_flags |= real_flags;

	/* Currently supporting only imported memory */
#ifdef CONFIG_DMA_SHARED_BUFFER
	if (reg->gpu_alloc->type != KBASE_MEM_TYPE_IMPORTED_UMM) {
		ret = -EINVAL;
		goto out_unlock;
	}

	if (IS_ENABLED(CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND)) {
		/* Future use will use the new flags, existing mapping
		 * will NOT be updated as memory should not be in use
		 * by the GPU when updating the flags.
		 */
		WARN_ON(reg->gpu_alloc->imported.umm.current_mapping_usage_count);
		ret = 0;
	} else if (reg->gpu_alloc->imported.umm.current_mapping_usage_count) {
		/*
		 * When CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND is not enabled the
		 * dma-buf GPU mapping should always be present, check that
		 * this is the case and warn and skip the page table update if
		 * not.
		 *
		 * Then update dma-buf GPU mapping with the new flags.
		 *
		 * Note: The buffer must not be in use on the GPU when
		 * changing flags. If the buffer is in active use on
		 * the GPU, there is a risk that the GPU may trigger a
		 * shareability fault, as it will see the same
		 * addresses from buffer with different shareability
		 * properties.
		 */
		dev_dbg(kctx->kbdev->dev,
			"Updating page tables on mem flag change\n");
		ret = kbase_mmu_update_pages(kctx, reg->start_pfn,
				kbase_get_gpu_phy_pages(reg),
				kbase_reg_current_backed_size(reg),
				new_flags,
				reg->gpu_alloc->group_id);
		if (ret)
			dev_warn(kctx->kbdev->dev,
				 "Failed to update GPU page tables on flag change: %d\n",
				 ret);
	} else
		WARN_ON(!reg->gpu_alloc->imported.umm.current_mapping_usage_count);
#else
	/* Reject when dma-buf support is not enabled. */
	ret = -EINVAL;
#endif /* CONFIG_DMA_SHARED_BUFFER */

	/* If everything is good, then set the new flags on the region. */
	if (!ret)
		reg->flags = new_flags;

out_unlock:
	kbase_gpu_vm_unlock(kctx);
	up_write(&current->mm->mmap_sem);
out:
	return ret;
}

#define KBASE_MEM_IMPORT_HAVE_PAGES (1UL << BASE_MEM_FLAGS_NR_BITS)

int kbase_mem_do_sync_imported(struct kbase_context *kctx,
		struct kbase_va_region *reg, enum kbase_sync_type sync_fn)
{
	int ret = 0;
#ifdef CONFIG_DMA_SHARED_BUFFER
	struct dma_buf *dma_buf;
	enum dma_data_direction dir = DMA_BIDIRECTIONAL;

	lockdep_assert_held(&kctx->reg_lock);

	/* We assume that the same physical allocation object is used for both
	 * GPU and CPU for imported buffers.
	 */
	WARN_ON(reg->cpu_alloc != reg->gpu_alloc);

	/* Currently only handle dma-bufs */
	if (reg->gpu_alloc->type != KBASE_MEM_TYPE_IMPORTED_UMM)
		return -EINVAL;
	/*
	 * Attempting to sync with CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND
	 * enabled can expose us to a Linux Kernel issue between v4.6 and
	 * v4.19. We will not attempt to support cache syncs on dma-bufs that
	 * are mapped on demand (i.e. not on import), even on pre-4.6, neither
	 * on 4.20 or newer kernels, because this makes it difficult for
	 * userspace to know when they can rely on the cache sync.
	 * Instead, only support syncing when we always map dma-bufs on import,
	 * or if the particular buffer is mapped right now.
	 */
	if (IS_ENABLED(CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND) &&
	    !reg->gpu_alloc->imported.umm.current_mapping_usage_count)
		return -EINVAL;

	dma_buf = reg->gpu_alloc->imported.umm.dma_buf;

	switch (sync_fn) {
	case KBASE_SYNC_TO_DEVICE:
		dev_dbg(kctx->kbdev->dev,
			"Syncing imported buffer at GPU VA %llx to GPU\n",
			reg->start_pfn);

#if KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE
		if (ion_cached_needsync_dmabuf(dma_buf)) {
#else
		if (ion_cached_dmabuf(dma_buf)) {
#endif
#ifdef KBASE_MEM_ION_SYNC_WORKAROUND
		if (!WARN_ON(!reg->gpu_alloc->imported.umm.dma_attachment)) {
			struct dma_buf_attachment *attachment = reg->gpu_alloc->imported.umm.dma_attachment;
			struct sg_table *sgt = reg->gpu_alloc->imported.umm.sgt;

			dma_sync_sg_for_device(attachment->dev, sgt->sgl,
					sgt->nents, dir);
			ret = 0;
		}
#else
	/* Though the below version check could be superfluous depending upon the version condition
	 * used for enabling KBASE_MEM_ION_SYNC_WORKAROUND, we still keep this check here to allow
	 * ease of modification for non-ION systems or systems where ION has been patched.
	 */
#if KERNEL_VERSION(4, 6, 0) > LINUX_VERSION_CODE && !defined(CONFIG_CHROMEOS)
		dma_buf_end_cpu_access(dma_buf,
				0, dma_buf->size,
				dir);
		ret = 0;
#else
		ret = dma_buf_end_cpu_access(dma_buf,
				dir);
#endif
#endif /* KBASE_MEM_ION_SYNC_WORKAROUND */
		}
		break;
	case KBASE_SYNC_TO_CPU:
		dev_dbg(kctx->kbdev->dev,
			"Syncing imported buffer at GPU VA %llx to CPU\n",
			reg->start_pfn);

#if KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE
		if (ion_cached_needsync_dmabuf(dma_buf)) {
#else
		if (ion_cached_dmabuf(dma_buf)) {
#endif
#ifdef KBASE_MEM_ION_SYNC_WORKAROUND
		if (!WARN_ON(!reg->gpu_alloc->imported.umm.dma_attachment)) {
			struct dma_buf_attachment *attachment = reg->gpu_alloc->imported.umm.dma_attachment;
			struct sg_table *sgt = reg->gpu_alloc->imported.umm.sgt;

			dma_sync_sg_for_cpu(attachment->dev, sgt->sgl,
					sgt->nents, dir);
			ret = 0;
		}
#else
		ret = dma_buf_begin_cpu_access(dma_buf,
#if KERNEL_VERSION(4, 6, 0) > LINUX_VERSION_CODE && !defined(CONFIG_CHROMEOS)
				0, dma_buf->size,
#endif
				dir);
#endif /* KBASE_MEM_ION_SYNC_WORKAROUND */
		}
		break;
	};

	if (unlikely(ret))
		dev_warn(kctx->kbdev->dev,
			 "Failed to sync mem region %pK at GPU VA %llx: %d\n",
			 reg, reg->start_pfn, ret);

#else /* CONFIG_DMA_SHARED_BUFFER */
	CSTD_UNUSED(kctx);
	CSTD_UNUSED(reg);
	CSTD_UNUSED(sync_fn);
#endif /* CONFIG_DMA_SHARED_BUFFER */

	return ret;
}

#ifdef CONFIG_DMA_SHARED_BUFFER
/**
 * kbase_mem_umm_unmap_attachment - Unmap dma-buf attachment
 * @kctx: Pointer to kbase context
 * @alloc: Pointer to allocation with imported dma-buf memory to unmap
 *
 * This will unmap a dma-buf. Must be called after the GPU page tables for the
 * region have been torn down.
 */
static void kbase_mem_umm_unmap_attachment(struct kbase_context *kctx,
					   struct kbase_mem_phy_alloc *alloc)
{
	struct tagged_addr *pa = alloc->pages;

	dma_buf_unmap_attachment(alloc->imported.umm.dma_attachment,
				 alloc->imported.umm.sgt, DMA_BIDIRECTIONAL);
	alloc->imported.umm.sgt = NULL;

	memset(pa, 0xff, sizeof(*pa) * alloc->nents);
	alloc->nents = 0;
}

/**
 * kbase_mem_umm_map_attachment - Prepare attached dma-buf for GPU mapping
 * @kctx: Pointer to kbase context
 * @reg: Pointer to region with imported dma-buf memory to map
 *
 * Map the dma-buf and prepare the page array with the tagged Mali physical
 * addresses for GPU mapping.
 *
 * Return: 0 on success, or negative error code
 */
static int kbase_mem_umm_map_attachment(struct kbase_context *kctx,
		struct kbase_va_region *reg)
{
	struct sg_table *sgt;
	struct scatterlist *s;
	int i;
	struct tagged_addr *pa;
	int err;
	size_t count = 0;
	struct kbase_mem_phy_alloc *alloc = reg->gpu_alloc;

	WARN_ON_ONCE(alloc->type != KBASE_MEM_TYPE_IMPORTED_UMM);
	WARN_ON_ONCE(alloc->imported.umm.sgt);

	sgt = dma_buf_map_attachment(alloc->imported.umm.dma_attachment,
			DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(sgt))
		return -EINVAL;

	/* save for later */
	alloc->imported.umm.sgt = sgt;

	pa = kbase_get_gpu_phy_pages(reg);

	for_each_sg(sgt->sgl, s, sgt->nents, i) {
		size_t j, pages = PFN_UP(sg_dma_len(s));

		WARN_ONCE(sg_dma_len(s) & (PAGE_SIZE-1),
		"sg_dma_len(s)=%u is not a multiple of PAGE_SIZE\n",
		sg_dma_len(s));

		WARN_ONCE(sg_dma_address(s) & (PAGE_SIZE-1),
		"sg_dma_address(s)=%llx is not aligned to PAGE_SIZE\n",
		(unsigned long long) sg_dma_address(s));

		for (j = 0; (j < pages) && (count < reg->nr_pages); j++, count++)
			*pa++ = as_tagged(sg_dma_address(s) +
				(j << PAGE_SHIFT));
		WARN_ONCE(j < pages,
		"sg list from dma_buf_map_attachment > dma_buf->size=%zu\n",
		alloc->imported.umm.dma_buf->size);
	}

	if (!(reg->flags & KBASE_REG_IMPORT_PAD) &&
			WARN_ONCE(count < reg->nr_pages,
			"sg list from dma_buf_map_attachment < dma_buf->size=%zu\n",
			alloc->imported.umm.dma_buf->size)) {
		err = -EINVAL;
		goto err_unmap_attachment;
	}

	/* Update nents as we now have pages to map */
	alloc->nents = count;

	return 0;

err_unmap_attachment:
	kbase_mem_umm_unmap_attachment(kctx, alloc);

	return err;
}

int kbase_mem_umm_map(struct kbase_context *kctx,
		struct kbase_va_region *reg)
{
	int err;
	struct kbase_mem_phy_alloc *alloc;
	unsigned long gwt_mask = ~0;

	lockdep_assert_held(&kctx->reg_lock);

	alloc = reg->gpu_alloc;

	alloc->imported.umm.current_mapping_usage_count++;
	if (alloc->imported.umm.current_mapping_usage_count != 1) {
		if (IS_ENABLED(CONFIG_MALI_DMA_BUF_LEGACY_COMPAT)) {
			if (!kbase_is_region_invalid_or_free(reg)) {
				err = kbase_mem_do_sync_imported(kctx, reg,
						KBASE_SYNC_TO_DEVICE);
				WARN_ON_ONCE(err);
			}
		}
		return 0;
	}

	err = kbase_mem_umm_map_attachment(kctx, reg);
	if (err)
		goto bad_map_attachment;

#ifdef CONFIG_MALI_CINSTR_GWT
	if (kctx->gwt_enabled)
		gwt_mask = ~KBASE_REG_GPU_WR;
#endif

	err = kbase_mmu_insert_pages(kctx->kbdev,
				     &kctx->mmu,
				     reg->start_pfn,
				     kbase_get_gpu_phy_pages(reg),
				     kbase_reg_current_backed_size(reg),
				     reg->flags & gwt_mask,
				     kctx->as_nr,
				     alloc->group_id);
	if (err)
		goto bad_insert;

	if (reg->flags & KBASE_REG_IMPORT_PAD &&
			!WARN_ON(reg->nr_pages < alloc->nents)) {
		/* For padded imported dma-buf memory, map the dummy aliasing
		 * page from the end of the dma-buf pages, to the end of the
		 * region using a read only mapping.
		 *
		 * Assume alloc->nents is the number of actual pages in the
		 * dma-buf memory.
		 */
		err = kbase_mmu_insert_single_page(kctx,
				reg->start_pfn + alloc->nents,
				kctx->aliasing_sink_page,
				reg->nr_pages - alloc->nents,
				(reg->flags | KBASE_REG_GPU_RD) &
				~KBASE_REG_GPU_WR,
				KBASE_MEM_GROUP_SINK);
		if (err)
			goto bad_pad_insert;
	}

	return 0;

bad_pad_insert:
	kbase_mmu_teardown_pages(kctx->kbdev,
				 &kctx->mmu,
				 reg->start_pfn,
				 alloc->nents,
				 kctx->as_nr);
bad_insert:
	kbase_mem_umm_unmap_attachment(kctx, alloc);
bad_map_attachment:
	alloc->imported.umm.current_mapping_usage_count--;

	return err;
}

void kbase_mem_umm_unmap(struct kbase_context *kctx,
		struct kbase_va_region *reg, struct kbase_mem_phy_alloc *alloc)
{
	alloc->imported.umm.current_mapping_usage_count--;
	if (alloc->imported.umm.current_mapping_usage_count) {
		if (IS_ENABLED(CONFIG_MALI_DMA_BUF_LEGACY_COMPAT)) {
			if (!kbase_is_region_invalid_or_free(reg)) {
				int err = kbase_mem_do_sync_imported(kctx, reg,
						KBASE_SYNC_TO_CPU);
				WARN_ON_ONCE(err);
			}
		}
		return;
	}

	if (!kbase_is_region_invalid_or_free(reg) && reg->gpu_alloc == alloc) {
		int err;

		err = kbase_mmu_teardown_pages(kctx->kbdev,
					       &kctx->mmu,
					       reg->start_pfn,
					       reg->nr_pages,
					       kctx->as_nr);
		WARN_ON(err);
	}

	kbase_mem_umm_unmap_attachment(kctx, alloc);
}

static int get_umm_memory_group_id(struct kbase_context *kctx,
		struct dma_buf *dma_buf)
{
	int group_id = BASE_MEM_GROUP_DEFAULT;

	if (kctx->kbdev->mgm_dev->ops.mgm_get_import_memory_id) {
		struct memory_group_manager_import_data mgm_import_data;

		mgm_import_data.type =
			MEMORY_GROUP_MANAGER_IMPORT_TYPE_DMA_BUF;
		mgm_import_data.u.dma_buf = dma_buf;

		group_id = kctx->kbdev->mgm_dev->ops.mgm_get_import_memory_id(
			kctx->kbdev->mgm_dev, &mgm_import_data);
	}

	return group_id;
}

/**
 * kbase_mem_from_umm - Import dma-buf memory into kctx
 * @kctx: Pointer to kbase context to import memory into
 * @fd: File descriptor of dma-buf to import
 * @va_pages: Pointer where virtual size of the region will be output
 * @flags: Pointer to memory flags
 * @padding: Number of read only padding pages to be inserted at the end of the
 * GPU mapping of the dma-buf
 *
 * Return: Pointer to new kbase_va_region object of the imported dma-buf, or
 * NULL on error.
 *
 * This function imports a dma-buf into kctx, and created a kbase_va_region
 * object that wraps the dma-buf.
 */
static struct kbase_va_region *kbase_mem_from_umm(struct kbase_context *kctx,
		int fd, u64 *va_pages, u64 *flags, u32 padding)
{
	struct kbase_va_region *reg;
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *dma_attachment;
	bool shared_zone = false;
	int group_id;

	/* 64-bit address range is the max */
	if (*va_pages > (U64_MAX / PAGE_SIZE))
		return NULL;

	dma_buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dma_buf))
		return NULL;

	dma_attachment = dma_buf_attach(dma_buf, kctx->kbdev->dev);
	if (IS_ERR_OR_NULL(dma_attachment)) {
		dma_buf_put(dma_buf);
		return NULL;
	}

	*va_pages = (PAGE_ALIGN(dma_buf->size) >> PAGE_SHIFT) + padding;

	if (!*va_pages) {
		dma_buf_detach(dma_buf, dma_attachment);
		dma_buf_put(dma_buf);
		return NULL;
	}

	/* ignore SAME_VA */
	*flags &= ~BASE_MEM_SAME_VA;

	/*
	 * Force CPU cached flag.
	 *
	 * We can't query the dma-buf exporter to get details about the CPU
	 * cache attributes of CPU mappings, so we have to assume that the
	 * buffer may be cached, and call into the exporter for cache
	 * maintenance, and rely on the exporter to do the right thing when
	 * handling our calls.
	 */
	*flags |= BASE_MEM_CACHED_CPU;

	if (*flags & BASE_MEM_IMPORT_SHARED)
		shared_zone = true;

#ifdef CONFIG_64BIT
	if (!kbase_ctx_flag(kctx, KCTX_COMPAT)) {
		/*
		 * 64-bit tasks require us to reserve VA on the CPU that we use
		 * on the GPU.
		 */
		shared_zone = true;
	}
#endif

	if (shared_zone) {
		*flags |= BASE_MEM_NEED_MMAP;
		reg = kbase_alloc_free_region(&kctx->reg_rbtree_same,
				0, *va_pages, KBASE_REG_ZONE_SAME_VA);
	} else {
		reg = kbase_alloc_free_region(&kctx->reg_rbtree_custom,
				0, *va_pages, KBASE_REG_ZONE_CUSTOM_VA);
	}

	if (!reg) {
		dma_buf_detach(dma_buf, dma_attachment);
		dma_buf_put(dma_buf);
		return NULL;
	}

	group_id = get_umm_memory_group_id(kctx, dma_buf);

	reg->gpu_alloc = kbase_alloc_create(kctx, *va_pages,
			KBASE_MEM_TYPE_IMPORTED_UMM, group_id);
	if (IS_ERR_OR_NULL(reg->gpu_alloc))
		goto no_alloc;

	reg->cpu_alloc = kbase_mem_phy_alloc_get(reg->gpu_alloc);

	if (kbase_update_region_flags(kctx, reg, *flags) != 0)
		goto error_out;

	/* No pages to map yet */
	reg->gpu_alloc->nents = 0;

	reg->flags &= ~KBASE_REG_FREE;
	reg->flags |= KBASE_REG_GPU_NX;	/* UMM is always No eXecute */
	reg->flags &= ~KBASE_REG_GROWABLE;	/* UMM cannot be grown */

	if (*flags & BASE_MEM_SECURE)
		reg->flags |= KBASE_REG_SECURE;

	if (padding)
		reg->flags |= KBASE_REG_IMPORT_PAD;

	reg->gpu_alloc->type = KBASE_MEM_TYPE_IMPORTED_UMM;
	reg->gpu_alloc->imported.umm.sgt = NULL;
	reg->gpu_alloc->imported.umm.dma_buf = dma_buf;
	reg->gpu_alloc->imported.umm.dma_attachment = dma_attachment;
	reg->gpu_alloc->imported.umm.current_mapping_usage_count = 0;
	reg->extent = 0;

	if (!IS_ENABLED(CONFIG_MALI_DMA_BUF_MAP_ON_DEMAND)) {
		int err;

		reg->gpu_alloc->imported.umm.current_mapping_usage_count = 1;

		err = kbase_mem_umm_map_attachment(kctx, reg);
		if (err) {
			dev_warn(kctx->kbdev->dev,
				 "Failed to map dma-buf %pK on GPU: %d\n",
				 dma_buf, err);
			goto error_out;
		}

		*flags |= KBASE_MEM_IMPORT_HAVE_PAGES;
	}

	return reg;

error_out:
	kbase_mem_phy_alloc_put(reg->gpu_alloc);
	kbase_mem_phy_alloc_put(reg->cpu_alloc);
no_alloc:
	kfree(reg);

	return NULL;
}
#endif  /* CONFIG_DMA_SHARED_BUFFER */

u32 kbase_get_cache_line_alignment(struct kbase_device *kbdev)
{
	u32 cpu_cache_line_size = cache_line_size();
	u32 gpu_cache_line_size =
		(1UL << kbdev->gpu_props.props.l2_props.log2_line_size);

	return ((cpu_cache_line_size > gpu_cache_line_size) ?
				cpu_cache_line_size :
				gpu_cache_line_size);
}

static struct kbase_va_region *kbase_mem_from_user_buffer(
		struct kbase_context *kctx, unsigned long address,
		unsigned long size, u64 *va_pages, u64 *flags)
{
	long i;
	struct kbase_va_region *reg;
	struct rb_root *rbtree;
	long faulted_pages;
	int zone = KBASE_REG_ZONE_CUSTOM_VA;
	bool shared_zone = false;
	u32 cache_line_alignment = kbase_get_cache_line_alignment(kctx->kbdev);
	struct kbase_alloc_import_user_buf *user_buf;
	struct page **pages = NULL;

	if ((address & (cache_line_alignment - 1)) != 0 ||
			(size & (cache_line_alignment - 1)) != 0) {
		if (*flags & BASE_MEM_UNCACHED_GPU) {
			dev_warn(kctx->kbdev->dev,
					"User buffer is not cache line aligned and marked as GPU uncached\n");
			goto bad_size;
		}

		/* Coherency must be enabled to handle partial cache lines */
		if (*flags & (BASE_MEM_COHERENT_SYSTEM |
			BASE_MEM_COHERENT_SYSTEM_REQUIRED)) {
			/* Force coherent system required flag, import will
			 * then fail if coherency isn't available
			 */
			*flags |= BASE_MEM_COHERENT_SYSTEM_REQUIRED;
		} else {
			dev_warn(kctx->kbdev->dev,
					"User buffer is not cache line aligned and no coherency enabled\n");
			goto bad_size;
		}
	}

	*va_pages = (PAGE_ALIGN(address + size) >> PAGE_SHIFT) -
		PFN_DOWN(address);
	if (!*va_pages)
		goto bad_size;

	if (*va_pages > (UINT64_MAX / PAGE_SIZE))
		/* 64-bit address range is the max */
		goto bad_size;

	/* SAME_VA generally not supported with imported memory (no known use cases) */
	*flags &= ~BASE_MEM_SAME_VA;

	if (*flags & BASE_MEM_IMPORT_SHARED)
		shared_zone = true;

#ifdef CONFIG_64BIT
	if (!kbase_ctx_flag(kctx, KCTX_COMPAT)) {
		/*
		 * 64-bit tasks require us to reserve VA on the CPU that we use
		 * on the GPU.
		 */
		shared_zone = true;
	}
#endif

	if (shared_zone) {
		*flags |= BASE_MEM_NEED_MMAP;
		zone = KBASE_REG_ZONE_SAME_VA;
		rbtree = &kctx->reg_rbtree_same;
	} else
		rbtree = &kctx->reg_rbtree_custom;

	reg = kbase_alloc_free_region(rbtree, 0, *va_pages, zone);

	if (!reg)
		goto no_region;

	reg->gpu_alloc = kbase_alloc_create(
		kctx, *va_pages, KBASE_MEM_TYPE_IMPORTED_USER_BUF,
		BASE_MEM_GROUP_DEFAULT);
	if (IS_ERR_OR_NULL(reg->gpu_alloc))
		goto no_alloc_obj;

	reg->cpu_alloc = kbase_mem_phy_alloc_get(reg->gpu_alloc);

	if (kbase_update_region_flags(kctx, reg, *flags) != 0)
		goto invalid_flags;

	reg->flags &= ~KBASE_REG_FREE;
	reg->flags |= KBASE_REG_GPU_NX; /* User-buffers are always No eXecute */
	reg->flags &= ~KBASE_REG_GROWABLE; /* Cannot be grown */

	user_buf = &reg->gpu_alloc->imported.user_buf;

	user_buf->size = size;
	user_buf->address = address;
	user_buf->nr_pages = *va_pages;
	user_buf->mm = current->mm;
#if KERNEL_VERSION(4, 11, 0) > LINUX_VERSION_CODE
	atomic_inc(&current->mm->mm_count);
#else
	mmgrab(current->mm);
#endif
	if (reg->gpu_alloc->properties & KBASE_MEM_PHY_ALLOC_LARGE)
		user_buf->pages = vmalloc(*va_pages * sizeof(struct page *));
	else
		user_buf->pages = kmalloc_array(*va_pages,
				sizeof(struct page *), GFP_KERNEL);

	if (!user_buf->pages)
		goto no_page_array;

	/* If the region is coherent with the CPU then the memory is imported
	 * and mapped onto the GPU immediately.
	 * Otherwise get_user_pages is called as a sanity check, but with
	 * NULL as the pages argument which will fault the pages, but not
	 * pin them. The memory will then be pinned only around the jobs that
	 * specify the region as an external resource.
	 */
	if (reg->flags & KBASE_REG_SHARE_BOTH) {
		pages = user_buf->pages;
		*flags |= KBASE_MEM_IMPORT_HAVE_PAGES;
	}

	down_read(&current->mm->mmap_sem);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 6, 0)
	faulted_pages = get_user_pages(current, current->mm, address, *va_pages,
#if KERNEL_VERSION(4, 4, 168) <= LINUX_VERSION_CODE && \
KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE
			reg->flags & KBASE_REG_GPU_WR ? FOLL_WRITE : 0,
			pages, NULL);
#else
			reg->flags & KBASE_REG_GPU_WR, 0, pages, NULL);
#endif
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0)
	faulted_pages = get_user_pages(address, *va_pages,
			reg->flags & KBASE_REG_GPU_WR, 0, pages, NULL);
#else
	faulted_pages = get_user_pages(address, *va_pages,
			reg->flags & KBASE_REG_GPU_WR ? FOLL_WRITE : 0,
			pages, NULL);
#endif

	up_read(&current->mm->mmap_sem);

	if (faulted_pages != *va_pages)
		goto fault_mismatch;

	reg->gpu_alloc->nents = 0;
	reg->extent = 0;

	if (pages) {
		struct device *dev = kctx->kbdev->dev;
		unsigned long local_size = user_buf->size;
		unsigned long offset = user_buf->address & ~PAGE_MASK;
		struct tagged_addr *pa = kbase_get_gpu_phy_pages(reg);

		/* Top bit signifies that this was pinned on import */
		user_buf->current_mapping_usage_count |= PINNED_ON_IMPORT;

		for (i = 0; i < faulted_pages; i++) {
			dma_addr_t dma_addr;
			unsigned long min;

			min = MIN(PAGE_SIZE - offset, local_size);
			dma_addr = dma_map_page(dev, pages[i],
					offset, min,
					DMA_BIDIRECTIONAL);
			if (dma_mapping_error(dev, dma_addr))
				goto unwind_dma_map;

			user_buf->dma_addrs[i] = dma_addr;
			pa[i] = as_tagged(page_to_phys(pages[i]));

			local_size -= min;
			offset = 0;
		}

		reg->gpu_alloc->nents = faulted_pages;
	}

	return reg;

unwind_dma_map:
	while (i--) {
		dma_unmap_page(kctx->kbdev->dev,
				user_buf->dma_addrs[i],
				PAGE_SIZE, DMA_BIDIRECTIONAL);
	}
fault_mismatch:
	if (pages) {
		for (i = 0; i < faulted_pages; i++)
			put_page(pages[i]);
	}
no_page_array:
invalid_flags:
	kbase_mem_phy_alloc_put(reg->cpu_alloc);
	kbase_mem_phy_alloc_put(reg->gpu_alloc);
no_alloc_obj:
	kfree(reg);
no_region:
bad_size:
	return NULL;

}


u64 kbase_mem_alias(struct kbase_context *kctx, u64 *flags, u64 stride,
		    u64 nents, struct base_mem_aliasing_info *ai,
		    u64 *num_pages)
{
	struct kbase_va_region *reg;
	u64 gpu_va;
	size_t i;
	bool coherent;

	KBASE_DEBUG_ASSERT(kctx);
	KBASE_DEBUG_ASSERT(flags);
	KBASE_DEBUG_ASSERT(ai);
	KBASE_DEBUG_ASSERT(num_pages);

	/* mask to only allowed flags */
	*flags &= (BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR |
		   BASE_MEM_COHERENT_SYSTEM | BASE_MEM_COHERENT_LOCAL |
		   BASE_MEM_PROT_CPU_RD | BASE_MEM_COHERENT_SYSTEM_REQUIRED);

	if (!(*flags & (BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_GPU_WR))) {
		dev_warn(kctx->kbdev->dev,
				"kbase_mem_alias called with bad flags (%llx)",
				(unsigned long long)*flags);
		goto bad_flags;
	}
	coherent = (*flags & BASE_MEM_COHERENT_SYSTEM) != 0 ||
			(*flags & BASE_MEM_COHERENT_SYSTEM_REQUIRED) != 0;

	if (!stride)
		goto bad_stride;

	if (!nents)
		goto bad_nents;

	if ((nents * stride) > (U64_MAX / PAGE_SIZE))
		/* 64-bit address range is the max */
		goto bad_size;

	/* calculate the number of pages this alias will cover */
	*num_pages = nents * stride;

#ifdef CONFIG_64BIT
	if (!kbase_ctx_flag(kctx, KCTX_COMPAT)) {
		/* 64-bit tasks must MMAP anyway, but not expose this address to
		 * clients */
		*flags |= BASE_MEM_NEED_MMAP;
		reg = kbase_alloc_free_region(&kctx->reg_rbtree_same, 0,
				*num_pages,
				KBASE_REG_ZONE_SAME_VA);
	} else {
#else
	if (1) {
#endif
		reg = kbase_alloc_free_region(&kctx->reg_rbtree_custom,
				0, *num_pages,
				KBASE_REG_ZONE_CUSTOM_VA);
	}

	if (!reg)
		goto no_reg;

	/* zero-sized page array, as we don't need one/can support one */
	reg->gpu_alloc = kbase_alloc_create(kctx, 0, KBASE_MEM_TYPE_ALIAS,
		BASE_MEM_GROUP_DEFAULT);
	if (IS_ERR_OR_NULL(reg->gpu_alloc))
		goto no_alloc_obj;

	reg->cpu_alloc = kbase_mem_phy_alloc_get(reg->gpu_alloc);

	if (kbase_update_region_flags(kctx, reg, *flags) != 0)
		goto invalid_flags;

	reg->gpu_alloc->imported.alias.nents = nents;
	reg->gpu_alloc->imported.alias.stride = stride;
	reg->gpu_alloc->imported.alias.aliased = vzalloc(sizeof(*reg->gpu_alloc->imported.alias.aliased) * nents);
	if (!reg->gpu_alloc->imported.alias.aliased)
		goto no_aliased_array;

	kbase_gpu_vm_lock(kctx);

	/* validate and add src handles */
	for (i = 0; i < nents; i++) {
		if (ai[i].handle.basep.handle < BASE_MEM_FIRST_FREE_ADDRESS) {
			if (ai[i].handle.basep.handle !=
			    BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE)
				goto bad_handle; /* unsupported magic handle */
			if (!ai[i].length)
				goto bad_handle; /* must be > 0 */
			if (ai[i].length > stride)
				goto bad_handle; /* can't be larger than the
						    stride */
			reg->gpu_alloc->imported.alias.aliased[i].length = ai[i].length;
		} else {
			struct kbase_va_region *aliasing_reg;
			struct kbase_mem_phy_alloc *alloc;

			aliasing_reg = kbase_region_tracker_find_region_base_address(
				kctx,
				(ai[i].handle.basep.handle >> PAGE_SHIFT) << PAGE_SHIFT);

			/* validate found region */
			if (kbase_is_region_invalid_or_free(aliasing_reg))
				goto bad_handle; /* Not found/already free */
			if (aliasing_reg->flags & KBASE_REG_DONT_NEED)
				goto bad_handle; /* Ephemeral region */
			if (!(aliasing_reg->flags & KBASE_REG_GPU_CACHED))
				goto bad_handle; /* GPU uncached memory */
			if (!aliasing_reg->gpu_alloc)
				goto bad_handle; /* No alloc */
			if (aliasing_reg->gpu_alloc->type != KBASE_MEM_TYPE_NATIVE)
				goto bad_handle; /* Not a native alloc */
			if (coherent != ((aliasing_reg->flags & KBASE_REG_SHARE_BOTH) != 0))
				goto bad_handle;
				/* Non-coherent memory cannot alias
				   coherent memory, and vice versa.*/

			/* check size against stride */
			if (!ai[i].length)
				goto bad_handle; /* must be > 0 */
			if (ai[i].length > stride)
				goto bad_handle; /* can't be larger than the
						    stride */

			alloc = aliasing_reg->gpu_alloc;

			/* check against the alloc's size */
			if (ai[i].offset > alloc->nents)
				goto bad_handle; /* beyond end */
			if (ai[i].offset + ai[i].length > alloc->nents)
				goto bad_handle; /* beyond end */

			reg->gpu_alloc->imported.alias.aliased[i].alloc = kbase_mem_phy_alloc_get(alloc);
			reg->gpu_alloc->imported.alias.aliased[i].length = ai[i].length;
			reg->gpu_alloc->imported.alias.aliased[i].offset = ai[i].offset;
		}
	}

#ifdef CONFIG_64BIT
	if (!kbase_ctx_flag(kctx, KCTX_COMPAT)) {
		/* Bind to a cookie */
		if (!kctx->cookies) {
			dev_err(kctx->kbdev->dev, "No cookies available for allocation!");
			goto no_cookie;
		}
		/* return a cookie */
		gpu_va = __ffs(kctx->cookies);
		kctx->cookies &= ~(1UL << gpu_va);
		BUG_ON(kctx->pending_regions[gpu_va]);
		kctx->pending_regions[gpu_va] = reg;

		/* relocate to correct base */
		gpu_va += PFN_DOWN(BASE_MEM_COOKIE_BASE);
		gpu_va <<= PAGE_SHIFT;
	} else /* we control the VA */ {
#else
	if (1) {
#endif
		if (kbase_gpu_mmap(kctx, reg, 0, *num_pages, 1) != 0) {
			dev_warn(kctx->kbdev->dev, "Failed to map memory on GPU");
			goto no_mmap;
		}
		/* return real GPU VA */
		gpu_va = reg->start_pfn << PAGE_SHIFT;
	}

	reg->flags &= ~KBASE_REG_FREE;
	reg->flags &= ~KBASE_REG_GROWABLE;

	kbase_gpu_vm_unlock(kctx);

	return gpu_va;

#ifdef CONFIG_64BIT
no_cookie:
#endif
no_mmap:
bad_handle:
	kbase_gpu_vm_unlock(kctx);
no_aliased_array:
invalid_flags:
	kbase_mem_phy_alloc_put(reg->cpu_alloc);
	kbase_mem_phy_alloc_put(reg->gpu_alloc);
no_alloc_obj:
	kfree(reg);
no_reg:
bad_size:
bad_nents:
bad_stride:
bad_flags:
	return 0;
}

int kbase_mem_import(struct kbase_context *kctx, enum base_mem_import_type type,
		void __user *phandle, u32 padding, u64 *gpu_va, u64 *va_pages,
		u64 *flags)
{
	struct kbase_va_region *reg;

	KBASE_DEBUG_ASSERT(kctx);
	KBASE_DEBUG_ASSERT(gpu_va);
	KBASE_DEBUG_ASSERT(va_pages);
	KBASE_DEBUG_ASSERT(flags);

	if ((!kbase_ctx_flag(kctx, KCTX_COMPAT)) &&
			kbase_ctx_flag(kctx, KCTX_FORCE_SAME_VA))
		*flags |= BASE_MEM_SAME_VA;

	if (!kbase_check_import_flags(*flags)) {
		dev_warn(kctx->kbdev->dev,
				"kbase_mem_import called with bad flags (%llx)",
				(unsigned long long)*flags);
		goto bad_flags;
	}

	if ((*flags & BASE_MEM_UNCACHED_GPU) != 0 &&
			(*flags & BASE_MEM_COHERENT_SYSTEM_REQUIRED) != 0) {
		/* Remove COHERENT_SYSTEM_REQUIRED flag if uncached GPU mapping is requested */
		*flags &= ~BASE_MEM_COHERENT_SYSTEM_REQUIRED;
	}
	if ((*flags & BASE_MEM_COHERENT_SYSTEM_REQUIRED) != 0 &&
			!kbase_device_is_cpu_coherent(kctx->kbdev)) {
		dev_warn(kctx->kbdev->dev,
				"kbase_mem_import call required coherent mem when unavailable");
		goto bad_flags;
	}
	if ((*flags & BASE_MEM_COHERENT_SYSTEM) != 0 &&
			!kbase_device_is_cpu_coherent(kctx->kbdev)) {
		/* Remove COHERENT_SYSTEM flag if coherent mem is unavailable */
		*flags &= ~BASE_MEM_COHERENT_SYSTEM;
	}

	if ((padding != 0) && (type != BASE_MEM_IMPORT_TYPE_UMM)) {
		dev_warn(kctx->kbdev->dev,
				"padding is only supported for UMM");
		goto bad_flags;
	}

	switch (type) {
#ifdef CONFIG_DMA_SHARED_BUFFER
	case BASE_MEM_IMPORT_TYPE_UMM: {
		int fd;

		if (get_user(fd, (int __user *)phandle))
			reg = NULL;
		else
			reg = kbase_mem_from_umm(kctx, fd, va_pages, flags,
					padding);
	}
	break;
#endif /* CONFIG_DMA_SHARED_BUFFER */
	case BASE_MEM_IMPORT_TYPE_USER_BUFFER: {
		struct base_mem_import_user_buffer user_buffer;
		void __user *uptr;

		if (copy_from_user(&user_buffer, phandle,
				sizeof(user_buffer))) {
			reg = NULL;
		} else {
#ifdef CONFIG_COMPAT
			if (kbase_ctx_flag(kctx, KCTX_COMPAT))
				uptr = compat_ptr(user_buffer.ptr);
			else
#endif
				uptr = u64_to_user_ptr(user_buffer.ptr);

			reg = kbase_mem_from_user_buffer(kctx,
					(unsigned long)uptr, user_buffer.length,
					va_pages, flags);
		}
		break;
	}
	default: {
		reg = NULL;
		break;
	}
	}

	if (!reg)
		goto no_reg;

	kbase_gpu_vm_lock(kctx);

	/* mmap needed to setup VA? */
	if (*flags & (BASE_MEM_SAME_VA | BASE_MEM_NEED_MMAP)) {
		/* Bind to a cookie */
		if (!kctx->cookies)
			goto no_cookie;
		/* return a cookie */
		*gpu_va = __ffs(kctx->cookies);
		kctx->cookies &= ~(1UL << *gpu_va);
		BUG_ON(kctx->pending_regions[*gpu_va]);
		kctx->pending_regions[*gpu_va] = reg;

		/* relocate to correct base */
		*gpu_va += PFN_DOWN(BASE_MEM_COOKIE_BASE);
		*gpu_va <<= PAGE_SHIFT;

	} else if (*flags & KBASE_MEM_IMPORT_HAVE_PAGES)  {
		/* we control the VA, mmap now to the GPU */
		if (kbase_gpu_mmap(kctx, reg, 0, *va_pages, 1) != 0)
			goto no_gpu_va;
		/* return real GPU VA */
		*gpu_va = reg->start_pfn << PAGE_SHIFT;
	} else {
		/* we control the VA, but nothing to mmap yet */
		if (kbase_add_va_region(kctx, reg, 0, *va_pages, 1) != 0)
			goto no_gpu_va;
		/* return real GPU VA */
		*gpu_va = reg->start_pfn << PAGE_SHIFT;
	}

	/* clear out private flags */
	*flags &= ((1UL << BASE_MEM_FLAGS_NR_BITS) - 1);

	kbase_gpu_vm_unlock(kctx);

	return 0;

no_gpu_va:
no_cookie:
	kbase_gpu_vm_unlock(kctx);
	kbase_mem_phy_alloc_put(reg->cpu_alloc);
	kbase_mem_phy_alloc_put(reg->gpu_alloc);
	kfree(reg);
no_reg:
bad_flags:
	*gpu_va = 0;
	*va_pages = 0;
	*flags = 0;
	return -ENOMEM;
}

int kbase_mem_grow_gpu_mapping(struct kbase_context *kctx,
		struct kbase_va_region *reg,
		u64 new_pages, u64 old_pages)
{
	struct tagged_addr *phy_pages;
	u64 delta = new_pages - old_pages;
	int ret = 0;

	lockdep_assert_held(&kctx->reg_lock);

	/* Map the new pages into the GPU */
	phy_pages = kbase_get_gpu_phy_pages(reg);
	ret = kbase_mmu_insert_pages(kctx->kbdev, &kctx->mmu,
		reg->start_pfn + old_pages, phy_pages + old_pages, delta,
		reg->flags, kctx->as_nr, reg->gpu_alloc->group_id);

	return ret;
}

void kbase_mem_shrink_cpu_mapping(struct kbase_context *kctx,
		struct kbase_va_region *reg,
		u64 new_pages, u64 old_pages)
{
	u64 gpu_va_start = reg->start_pfn;

	if (new_pages == old_pages)
		/* Nothing to do */
		return;

	unmap_mapping_range(kctx->filp->f_inode->i_mapping,
			(gpu_va_start + new_pages)<<PAGE_SHIFT,
			(old_pages - new_pages)<<PAGE_SHIFT, 1);
}

int kbase_mem_shrink_gpu_mapping(struct kbase_context *kctx,
		struct kbase_va_region *reg,
		u64 new_pages, u64 old_pages)
{
	u64 delta = old_pages - new_pages;
	int ret = 0;

	ret = kbase_mmu_teardown_pages(kctx->kbdev, &kctx->mmu,
			reg->start_pfn + new_pages, delta, kctx->as_nr);

	return ret;
}

int kbase_mem_commit(struct kbase_context *kctx, u64 gpu_addr, u64 new_pages)
{
	u64 old_pages;
	u64 delta;
	int res = -EINVAL;
	struct kbase_va_region *reg;
	bool read_locked = false;

	KBASE_DEBUG_ASSERT(kctx);
	KBASE_DEBUG_ASSERT(gpu_addr != 0);

	if (gpu_addr & ~PAGE_MASK) {
		dev_warn(kctx->kbdev->dev, "kbase:mem_commit: gpu_addr: passed parameter is invalid");
		return -EINVAL;
	}

	down_write(&current->mm->mmap_sem);
	kbase_gpu_vm_lock(kctx);

	/* Validate the region */
	reg = kbase_region_tracker_find_region_base_address(kctx, gpu_addr);
	if (kbase_is_region_invalid_or_free(reg))
		goto out_unlock;

	KBASE_DEBUG_ASSERT(reg->cpu_alloc);
	KBASE_DEBUG_ASSERT(reg->gpu_alloc);

	if (reg->gpu_alloc->type != KBASE_MEM_TYPE_NATIVE)
		goto out_unlock;

	if (0 == (reg->flags & KBASE_REG_GROWABLE))
		goto out_unlock;

	/* Would overflow the VA region */
	if (new_pages > reg->nr_pages)
		goto out_unlock;

	/* can't be mapped more than once on the GPU */
	if (atomic_read(&reg->gpu_alloc->gpu_mappings) > 1)
		goto out_unlock;
	/* can't grow regions which are ephemeral */
	if (reg->flags & KBASE_REG_DONT_NEED)
		goto out_unlock;

#ifdef CONFIG_MALI_MEMORY_FULLY_BACKED
	/* Reject resizing commit size */
	if (reg->flags & KBASE_REG_PF_GROW)
		new_pages = reg->nr_pages;
#endif

	if (new_pages == reg->gpu_alloc->nents) {
		/* no change */
		res = 0;
		goto out_unlock;
	}

	old_pages = kbase_reg_current_backed_size(reg);
	if (new_pages > old_pages) {
		delta = new_pages - old_pages;

		/*
		 * No update to the mm so downgrade the writer lock to a read
		 * lock so other readers aren't blocked after this point.
		 */
		downgrade_write(&current->mm->mmap_sem);
		read_locked = true;

		/* Allocate some more pages */
		if (kbase_alloc_phy_pages_helper(reg->cpu_alloc, delta) != 0) {
			res = -ENOMEM;
			goto out_unlock;
		}
		if (reg->cpu_alloc != reg->gpu_alloc) {
			if (kbase_alloc_phy_pages_helper(
					reg->gpu_alloc, delta) != 0) {
				res = -ENOMEM;
				kbase_free_phy_pages_helper(reg->cpu_alloc,
						delta);
				goto out_unlock;
			}
		}

		/* No update required for CPU mappings, that's done on fault. */

		/* Update GPU mapping. */
		res = kbase_mem_grow_gpu_mapping(kctx, reg,
				new_pages, old_pages);

		/* On error free the new pages */
		if (res) {
			kbase_free_phy_pages_helper(reg->cpu_alloc, delta);
			if (reg->cpu_alloc != reg->gpu_alloc)
				kbase_free_phy_pages_helper(reg->gpu_alloc,
						delta);
			res = -ENOMEM;
			goto out_unlock;
		}
	} else {
		delta = old_pages - new_pages;

		/* Update all CPU mapping(s) */
		kbase_mem_shrink_cpu_mapping(kctx, reg,
				new_pages, old_pages);

		/* Update the GPU mapping */
		res = kbase_mem_shrink_gpu_mapping(kctx, reg,
				new_pages, old_pages);
		if (res) {
			res = -ENOMEM;
			goto out_unlock;
		}

		kbase_free_phy_pages_helper(reg->cpu_alloc, delta);
		if (reg->cpu_alloc != reg->gpu_alloc)
			kbase_free_phy_pages_helper(reg->gpu_alloc, delta);
	}

out_unlock:
	kbase_gpu_vm_unlock(kctx);
	if (read_locked)
		up_read(&current->mm->mmap_sem);
	else
		up_write(&current->mm->mmap_sem);

	return res;
}

static void kbase_cpu_vm_open(struct vm_area_struct *vma)
{
	struct kbase_cpu_mapping *map = vma->vm_private_data;

	KBASE_DEBUG_ASSERT(map);
	KBASE_DEBUG_ASSERT(map->count > 0);
	/* non-atomic as we're under Linux' mm lock */
	map->count++;
}

static void kbase_cpu_vm_close(struct vm_area_struct *vma)
{
	struct kbase_cpu_mapping *map = vma->vm_private_data;

	KBASE_DEBUG_ASSERT(map);
	KBASE_DEBUG_ASSERT(map->count > 0);

	/* non-atomic as we're under Linux' mm lock */
	if (--map->count)
		return;

	KBASE_DEBUG_ASSERT(map->kctx);
	KBASE_DEBUG_ASSERT(map->alloc);

	kbase_gpu_vm_lock(map->kctx);

	if (map->free_on_close) {
		KBASE_DEBUG_ASSERT((map->region->flags & KBASE_REG_ZONE_MASK) ==
				KBASE_REG_ZONE_SAME_VA);
		/* Avoid freeing memory on the process death which results in
		 * GPU Page Fault. Memory will be freed in kbase_destroy_context
		 */
		if (!(current->flags & PF_EXITING))
			kbase_mem_free_region(map->kctx, map->region);
	}

	list_del(&map->mappings_list);

	kbase_va_region_alloc_put(map->kctx, map->region);
	kbase_gpu_vm_unlock(map->kctx);

	kbase_mem_phy_alloc_put(map->alloc);
	kfree(map);
}

KBASE_EXPORT_TEST_API(kbase_cpu_vm_close);

static struct kbase_aliased *get_aliased_alloc(struct vm_area_struct *vma,
					struct kbase_va_region *reg,
					pgoff_t *start_off,
					size_t nr_pages)
{
	struct kbase_aliased *aliased =
		reg->cpu_alloc->imported.alias.aliased;

	if (!reg->cpu_alloc->imported.alias.stride ||
			reg->nr_pages < (*start_off + nr_pages)) {
		return NULL;
	}

	while (*start_off >= reg->cpu_alloc->imported.alias.stride) {
		aliased++;
		*start_off -= reg->cpu_alloc->imported.alias.stride;
	}

	if (!aliased->alloc) {
		/* sink page not available for dumping map */
		return NULL;
	}

	if ((*start_off + nr_pages) > aliased->length) {
		/* not fully backed by physical pages */
		return NULL;
	}

	return aliased;
}

#if (KERNEL_VERSION(4, 11, 0) > LINUX_VERSION_CODE)
static vm_fault_t kbase_cpu_vm_fault(struct vm_area_struct *vma,
			struct vm_fault *vmf)
{
#else
static vm_fault_t kbase_cpu_vm_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
#endif
	struct kbase_cpu_mapping *map = vma->vm_private_data;
	pgoff_t map_start_pgoff;
	pgoff_t fault_pgoff;
	size_t i;
	pgoff_t addr;
	size_t nents;
	struct tagged_addr *pages;
	vm_fault_t ret = VM_FAULT_SIGBUS;
	struct memory_group_manager_device *mgm_dev;

	KBASE_DEBUG_ASSERT(map);
	KBASE_DEBUG_ASSERT(map->count > 0);
	KBASE_DEBUG_ASSERT(map->kctx);
	KBASE_DEBUG_ASSERT(map->alloc);

	map_start_pgoff = vma->vm_pgoff - map->region->start_pfn;

	kbase_gpu_vm_lock(map->kctx);
	if (unlikely(map->region->cpu_alloc->type == KBASE_MEM_TYPE_ALIAS)) {
		struct kbase_aliased *aliased =
		      get_aliased_alloc(vma, map->region, &map_start_pgoff, 1);

		if (!aliased)
			goto exit;

		nents = aliased->length;
		pages = aliased->alloc->pages + aliased->offset;
	} else  {
		nents = map->alloc->nents;
		pages = map->alloc->pages;
	}

	fault_pgoff = map_start_pgoff + (vmf->pgoff - vma->vm_pgoff);

	if (fault_pgoff >= nents)
		goto exit;

	/* Fault on access to DONT_NEED regions */
	if (map->alloc->reg && (map->alloc->reg->flags & KBASE_REG_DONT_NEED))
		goto exit;

	/* We are inserting all valid pages from the start of CPU mapping and
	 * not from the fault location (the mmap handler was previously doing
	 * the same).
	 */
	i = map_start_pgoff;
	addr = (pgoff_t)(vma->vm_start >> PAGE_SHIFT);
	mgm_dev = map->kctx->kbdev->mgm_dev;
	while (i < nents && (addr < vma->vm_end >> PAGE_SHIFT)) {

		ret = mgm_dev->ops.mgm_vmf_insert_pfn_prot(mgm_dev,
			map->alloc->group_id, vma, addr << PAGE_SHIFT,
			PFN_DOWN(as_phys_addr_t(pages[i])), vma->vm_page_prot);

		if (ret != VM_FAULT_NOPAGE)
			goto exit;

		i++; addr++;
	}

exit:
	kbase_gpu_vm_unlock(map->kctx);
	return ret;
}

const struct vm_operations_struct kbase_vm_ops = {
	.open  = kbase_cpu_vm_open,
	.close = kbase_cpu_vm_close,
	.fault = kbase_cpu_vm_fault
};

static int kbase_cpu_mmap(struct kbase_context *kctx,
		struct kbase_va_region *reg,
		struct vm_area_struct *vma,
		void *kaddr,
		size_t nr_pages,
		unsigned long aligned_offset,
		int free_on_close)
{
	struct kbase_cpu_mapping *map;
	int err = 0;

	map = kzalloc(sizeof(*map), GFP_KERNEL);

	if (!map) {
		WARN_ON(1);
		err = -ENOMEM;
		goto out;
	}

	/*
	 * VM_DONTCOPY - don't make this mapping available in fork'ed processes
	 * VM_DONTEXPAND - disable mremap on this region
	 * VM_IO - disables paging
	 * VM_DONTDUMP - Don't include in core dumps (3.7 only)
	 * VM_MIXEDMAP - Support mixing struct page*s and raw pfns.
	 *               This is needed to support using the dedicated and
	 *               the OS based memory backends together.
	 */
	/*
	 * This will need updating to propagate coherency flags
	 * See MIDBASE-1057
	 */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	vma->vm_flags |= VM_DONTCOPY | VM_DONTDUMP | VM_DONTEXPAND | VM_IO;
#else
	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED | VM_IO;
#endif
	vma->vm_ops = &kbase_vm_ops;
	vma->vm_private_data = map;

	if (reg->cpu_alloc->type == KBASE_MEM_TYPE_ALIAS && nr_pages) {
		pgoff_t rel_pgoff = vma->vm_pgoff - reg->start_pfn +
					(aligned_offset >> PAGE_SHIFT);
		struct kbase_aliased *aliased =
			get_aliased_alloc(vma, reg, &rel_pgoff, nr_pages);

		if (!aliased) {
			err = -EINVAL;
			kfree(map);
			goto out;
		}
	}

	if (!(reg->flags & KBASE_REG_CPU_CACHED) &&
	    (reg->flags & (KBASE_REG_CPU_WR|KBASE_REG_CPU_RD))) {
		/* We can't map vmalloc'd memory uncached.
		 * Other memory will have been returned from
		 * kbase_mem_pool which would be
		 * suitable for mapping uncached.
		 */
		BUG_ON(kaddr);
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	}

	if (!kaddr) {
		vma->vm_flags |= VM_PFNMAP;
	} else {
		WARN_ON(aligned_offset);
		/* MIXEDMAP so we can vfree the kaddr early and not track it after map time */
		vma->vm_flags |= VM_MIXEDMAP;
		/* vmalloc remaping is easy... */
		err = remap_vmalloc_range(vma, kaddr, 0);
		WARN_ON(err);
	}

	if (err) {
		kfree(map);
		goto out;
	}

	map->region = kbase_va_region_alloc_get(kctx, reg);
	map->free_on_close = free_on_close;
	map->kctx = kctx;
	map->alloc = kbase_mem_phy_alloc_get(reg->cpu_alloc);
	map->count = 1; /* start with one ref */

	if (reg->flags & KBASE_REG_CPU_CACHED)
		map->alloc->properties |= KBASE_MEM_PHY_ALLOC_ACCESSED_CACHED;

	list_add(&map->mappings_list, &map->alloc->mappings);

 out:
	return err;
}

#ifdef CONFIG_MALI_VECTOR_DUMP
static void kbase_free_unused_jit_allocations(struct kbase_context *kctx)
{
	/* Free all cached/unused JIT allocations as their contents are not
	 * really needed for the replay. The GPU writes to them would already
	 * have been captured through the GWT mechanism.
	 * This considerably reduces the size of mmu-snapshot-file and it also
	 * helps avoid segmentation fault issue during vector dumping of
	 * complex contents when the unused JIT allocations are accessed to
	 * dump their contents (as they appear in the page tables snapshot)
	 * but they got freed by the shrinker under low memory scenarios
	 * (which do occur with complex contents).
	 */
	while (kbase_jit_evict(kctx))
		;
}
#endif

static int kbase_mmu_dump_mmap(struct kbase_context *kctx,
			struct vm_area_struct *vma,
			struct kbase_va_region **const reg,
			void **const kmap_addr)
{
	struct kbase_va_region *new_reg;
	void *kaddr;
	u32 nr_pages;
	size_t size;
	int err = 0;

	dev_dbg(kctx->kbdev->dev, "in kbase_mmu_dump_mmap\n");
	size = (vma->vm_end - vma->vm_start);
	nr_pages = size >> PAGE_SHIFT;

#ifdef CONFIG_MALI_VECTOR_DUMP
	kbase_free_unused_jit_allocations(kctx);
#endif

	kaddr = kbase_mmu_dump(kctx, nr_pages);

	if (!kaddr) {
		err = -ENOMEM;
		goto out;
	}

	new_reg = kbase_alloc_free_region(&kctx->reg_rbtree_same, 0, nr_pages,
			KBASE_REG_ZONE_SAME_VA);
	if (!new_reg) {
		err = -ENOMEM;
		WARN_ON(1);
		goto out;
	}

	new_reg->cpu_alloc = kbase_alloc_create(kctx, 0, KBASE_MEM_TYPE_RAW,
		BASE_MEM_GROUP_DEFAULT);
	if (IS_ERR_OR_NULL(new_reg->cpu_alloc)) {
		err = -ENOMEM;
		new_reg->cpu_alloc = NULL;
		WARN_ON(1);
		goto out_no_alloc;
	}

	new_reg->gpu_alloc = kbase_mem_phy_alloc_get(new_reg->cpu_alloc);

	new_reg->flags &= ~KBASE_REG_FREE;
	new_reg->flags |= KBASE_REG_CPU_CACHED;
	if (kbase_add_va_region(kctx, new_reg, vma->vm_start, nr_pages, 1) != 0) {
		err = -ENOMEM;
		WARN_ON(1);
		goto out_va_region;
	}

	*kmap_addr = kaddr;
	*reg = new_reg;

	dev_dbg(kctx->kbdev->dev, "kbase_mmu_dump_mmap done\n");
	return 0;

out_no_alloc:
out_va_region:
	kbase_free_alloced_region(new_reg);
out:
	return err;
}


void kbase_os_mem_map_lock(struct kbase_context *kctx)
{
	struct mm_struct *mm = current->mm;
	(void)kctx;
	down_read(&mm->mmap_sem);
}

void kbase_os_mem_map_unlock(struct kbase_context *kctx)
{
	struct mm_struct *mm = current->mm;
	(void)kctx;
	up_read(&mm->mmap_sem);
}

static int kbasep_reg_mmap(struct kbase_context *kctx,
			   struct vm_area_struct *vma,
			   struct kbase_va_region **regm,
			   size_t *nr_pages, size_t *aligned_offset)

{
	int cookie = vma->vm_pgoff - PFN_DOWN(BASE_MEM_COOKIE_BASE);
	struct kbase_va_region *reg;
	int err = 0;

	*aligned_offset = 0;

	dev_dbg(kctx->kbdev->dev, "in kbasep_reg_mmap\n");

	/* SAME_VA stuff, fetch the right region */
	reg = kctx->pending_regions[cookie];
	if (!reg) {
		err = -ENOMEM;
		goto out;
	}

	if ((reg->flags & KBASE_REG_GPU_NX) && (reg->nr_pages != *nr_pages)) {
		/* incorrect mmap size */
		/* leave the cookie for a potential later
		 * mapping, or to be reclaimed later when the
		 * context is freed */
		err = -ENOMEM;
		goto out;
	}

	if ((vma->vm_flags & VM_READ && !(reg->flags & KBASE_REG_CPU_RD)) ||
	    (vma->vm_flags & VM_WRITE && !(reg->flags & KBASE_REG_CPU_WR))) {
		/* VM flags inconsistent with region flags */
		err = -EPERM;
		dev_err(kctx->kbdev->dev, "%s:%d inconsistent VM flags\n",
							__FILE__, __LINE__);
		goto out;
	}

	/* adjust down nr_pages to what we have physically */
	*nr_pages = kbase_reg_current_backed_size(reg);

	if (kbase_gpu_mmap(kctx, reg, vma->vm_start + *aligned_offset,
						reg->nr_pages, 1) != 0) {
		dev_err(kctx->kbdev->dev, "%s:%d\n", __FILE__, __LINE__);
		/* Unable to map in GPU space. */
		WARN_ON(1);
		err = -ENOMEM;
		goto out;
	}
	/* no need for the cookie anymore */
	kctx->pending_regions[cookie] = NULL;
	kctx->cookies |= (1UL << cookie);

	/*
	 * Overwrite the offset with the region start_pfn, so we effectively
	 * map from offset 0 in the region. However subtract the aligned
	 * offset so that when user space trims the mapping the beginning of
	 * the trimmed VMA has the correct vm_pgoff;
	 */
	vma->vm_pgoff = reg->start_pfn - ((*aligned_offset)>>PAGE_SHIFT);
out:
	*regm = reg;
	dev_dbg(kctx->kbdev->dev, "kbasep_reg_mmap done\n");

	return err;
}

int kbase_context_mmap(struct kbase_context *const kctx,
	struct vm_area_struct *const vma)
{
	struct kbase_va_region *reg = NULL;
	void *kaddr = NULL;
	size_t nr_pages = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	int err = 0;
	int free_on_close = 0;
	struct device *dev = kctx->kbdev->dev;
	size_t aligned_offset = 0;

	dev_dbg(dev, "kbase_mmap\n");

	if (!(vma->vm_flags & VM_READ))
		vma->vm_flags &= ~VM_MAYREAD;
	if (!(vma->vm_flags & VM_WRITE))
		vma->vm_flags &= ~VM_MAYWRITE;

	if (0 == nr_pages) {
		err = -EINVAL;
		goto out;
	}

	if (!(vma->vm_flags & VM_SHARED)) {
		err = -EINVAL;
		goto out;
	}

	kbase_gpu_vm_lock(kctx);

	if (vma->vm_pgoff == PFN_DOWN(BASE_MEM_MAP_TRACKING_HANDLE)) {
		/* The non-mapped tracking helper page */
		err = kbase_tracking_page_setup(kctx, vma);
		goto out_unlock;
	}

	/* if not the MTP, verify that the MTP has been mapped */
	rcu_read_lock();
	/* catches both when the special page isn't present or
	 * when we've forked */
	if (rcu_dereference(kctx->process_mm) != current->mm) {
		err = -EINVAL;
		rcu_read_unlock();
		goto out_unlock;
	}
	rcu_read_unlock();

	switch (vma->vm_pgoff) {
	case PFN_DOWN(BASEP_MEM_INVALID_HANDLE):
	case PFN_DOWN(BASEP_MEM_WRITE_ALLOC_PAGES_HANDLE):
		/* Illegal handle for direct map */
		err = -EINVAL;
		goto out_unlock;
	case PFN_DOWN(BASE_MEM_MMU_DUMP_HANDLE):
		/* MMU dump */
		err = kbase_mmu_dump_mmap(kctx, vma, &reg, &kaddr);
		if (0 != err)
			goto out_unlock;
		/* free the region on munmap */
		free_on_close = 1;
		break;
	case PFN_DOWN(BASE_MEM_COOKIE_BASE) ...
	     PFN_DOWN(BASE_MEM_FIRST_FREE_ADDRESS) - 1: {
		err = kbasep_reg_mmap(kctx, vma, &reg, &nr_pages,
							&aligned_offset);
		if (0 != err)
			goto out_unlock;
		/* free the region on munmap */
		free_on_close = 1;
		break;
	}
	default: {
		reg = kbase_region_tracker_find_region_enclosing_address(kctx,
					(u64)vma->vm_pgoff << PAGE_SHIFT);

		if (!kbase_is_region_invalid_or_free(reg)) {
			/* will this mapping overflow the size of the region? */
			if (nr_pages > (reg->nr_pages -
					(vma->vm_pgoff - reg->start_pfn))) {
				err = -ENOMEM;
				goto out_unlock;
			}

			if ((vma->vm_flags & VM_READ &&
					!(reg->flags & KBASE_REG_CPU_RD)) ||
					(vma->vm_flags & VM_WRITE &&
					!(reg->flags & KBASE_REG_CPU_WR))) {
				/* VM flags inconsistent with region flags */
				err = -EPERM;
				dev_err(dev, "%s:%d inconsistent VM flags\n",
					__FILE__, __LINE__);
				goto out_unlock;
			}

#ifdef CONFIG_DMA_SHARED_BUFFER
			if (KBASE_MEM_TYPE_IMPORTED_UMM ==
							reg->cpu_alloc->type) {
				if (0 != (vma->vm_pgoff - reg->start_pfn)) {
					err = -EINVAL;
					dev_warn(dev, "%s:%d attempt to do a partial map in a dma_buf: non-zero offset to dma_buf mapping!\n",
						__FILE__, __LINE__);
					goto out_unlock;
				}
				err = dma_buf_mmap(
					reg->cpu_alloc->imported.umm.dma_buf,
					vma, vma->vm_pgoff - reg->start_pfn);
				goto out_unlock;
			}
#endif /* CONFIG_DMA_SHARED_BUFFER */

			if (reg->cpu_alloc->type == KBASE_MEM_TYPE_ALIAS) {
				/* initial params check for aliased dumping map */
				if (nr_pages > reg->gpu_alloc->imported.alias.stride ||
					!reg->gpu_alloc->imported.alias.stride ||
					!nr_pages) {
					err = -EINVAL;
					dev_warn(dev, "mmap aliased: invalid params!\n");
					goto out_unlock;
				}
			}
			else if (reg->cpu_alloc->nents <
					(vma->vm_pgoff - reg->start_pfn + nr_pages)) {
				/* limit what we map to the amount currently backed */
				if ((vma->vm_pgoff - reg->start_pfn) >= reg->cpu_alloc->nents)
					nr_pages = 0;
				else
					nr_pages = reg->cpu_alloc->nents - (vma->vm_pgoff - reg->start_pfn);
			}
		} else {
			err = -ENOMEM;
			goto out_unlock;
		}
	} /* default */
	} /* switch */

	err = kbase_cpu_mmap(kctx, reg, vma, kaddr, nr_pages, aligned_offset,
			free_on_close);

	if (vma->vm_pgoff == PFN_DOWN(BASE_MEM_MMU_DUMP_HANDLE)) {
		/* MMU dump - userspace should now have a reference on
		 * the pages, so we can now free the kernel mapping */
		vfree(kaddr);
	}

out_unlock:
	kbase_gpu_vm_unlock(kctx);
out:
	if (err)
		dev_err(dev, "mmap failed %d\n", err);

	return err;
}

KBASE_EXPORT_TEST_API(kbase_context_mmap);

void kbase_sync_mem_regions(struct kbase_context *kctx,
		struct kbase_vmap_struct *map, enum kbase_sync_type dest)
{
	size_t i;
	off_t const offset = map->offset_in_page;
	size_t const page_count = PFN_UP(offset + map->size);

	/* Sync first page */
	size_t sz = MIN(((size_t) PAGE_SIZE - offset), map->size);
	struct tagged_addr cpu_pa = map->cpu_pages[0];
	struct tagged_addr gpu_pa = map->gpu_pages[0];

	kbase_sync_single(kctx, cpu_pa, gpu_pa, offset, sz, dest);

	/* Sync middle pages (if any) */
	for (i = 1; page_count > 2 && i < page_count - 1; i++) {
		cpu_pa = map->cpu_pages[i];
		gpu_pa = map->gpu_pages[i];
		kbase_sync_single(kctx, cpu_pa, gpu_pa, 0, PAGE_SIZE, dest);
	}

	/* Sync last page (if any) */
	if (page_count > 1) {
		cpu_pa = map->cpu_pages[page_count - 1];
		gpu_pa = map->gpu_pages[page_count - 1];
		sz = ((offset + map->size - 1) & ~PAGE_MASK) + 1;
		kbase_sync_single(kctx, cpu_pa, gpu_pa, 0, sz, dest);
	}
}

static int kbase_vmap_phy_pages(struct kbase_context *kctx,
		struct kbase_va_region *reg, u64 offset_bytes, size_t size,
		struct kbase_vmap_struct *map)
{
	unsigned long page_index;
	unsigned int offset_in_page = offset_bytes & ~PAGE_MASK;
	size_t page_count = PFN_UP(offset_in_page + size);
	struct tagged_addr *page_array;
	struct page **pages;
	void *cpu_addr = NULL;
	pgprot_t prot;
	size_t i;

	if (!size || !map || !reg->cpu_alloc || !reg->gpu_alloc)
		return -EINVAL;

	/* check if page_count calculation will wrap */
	if (size > ((size_t)-1 / PAGE_SIZE))
		return -EINVAL;

	page_index = offset_bytes >> PAGE_SHIFT;

	/* check if page_index + page_count will wrap */
	if (-1UL - page_count < page_index)
		return -EINVAL;

	if (page_index + page_count > kbase_reg_current_backed_size(reg))
		return -ENOMEM;

	if (reg->flags & KBASE_REG_DONT_NEED)
		return -EINVAL;

	prot = PAGE_KERNEL;
	if (!(reg->flags & KBASE_REG_CPU_CACHED)) {
		/* Map uncached */
		prot = pgprot_writecombine(prot);
	}

	page_array = kbase_get_cpu_phy_pages(reg);
	if (!page_array)
		return -ENOMEM;

	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	for (i = 0; i < page_count; i++)
		pages[i] = as_page(page_array[page_index + i]);

	/* Note: enforcing a RO prot_request onto prot is not done, since:
	 * - CPU-arch-specific integration required
	 * - kbase_vmap() requires no access checks to be made/enforced */

	cpu_addr = vmap(pages, page_count, VM_MAP, prot);

	kfree(pages);

	if (!cpu_addr)
		return -ENOMEM;

	map->offset_in_page = offset_in_page;
	map->cpu_alloc = reg->cpu_alloc;
	map->cpu_pages = &kbase_get_cpu_phy_pages(reg)[page_index];
	map->gpu_alloc = reg->gpu_alloc;
	map->gpu_pages = &kbase_get_gpu_phy_pages(reg)[page_index];
	map->addr = (void *)((uintptr_t)cpu_addr + offset_in_page);
	map->size = size;
	map->sync_needed = ((reg->flags & KBASE_REG_CPU_CACHED) != 0) &&
		!kbase_mem_is_imported(map->gpu_alloc->type);

	if (map->sync_needed)
		kbase_sync_mem_regions(kctx, map, KBASE_SYNC_TO_CPU);

	return 0;
}

void *kbase_vmap_prot(struct kbase_context *kctx, u64 gpu_addr, size_t size,
		      unsigned long prot_request, struct kbase_vmap_struct *map)
{
	struct kbase_va_region *reg;
	void *addr = NULL;
	u64 offset_bytes;
	struct kbase_mem_phy_alloc *cpu_alloc;
	struct kbase_mem_phy_alloc *gpu_alloc;
	int err;

	kbase_gpu_vm_lock(kctx);

	reg = kbase_region_tracker_find_region_enclosing_address(kctx,
			gpu_addr);
	if (kbase_is_region_invalid_or_free(reg))
		goto out_unlock;

	/* check access permissions can be satisfied
	 * Intended only for checking KBASE_REG_{CPU,GPU}_{RD,WR}
	 */
	if ((reg->flags & prot_request) != prot_request)
		goto out_unlock;

	offset_bytes = gpu_addr - (reg->start_pfn << PAGE_SHIFT);
	cpu_alloc = kbase_mem_phy_alloc_get(reg->cpu_alloc);
	gpu_alloc = kbase_mem_phy_alloc_get(reg->gpu_alloc);

	err = kbase_vmap_phy_pages(kctx, reg, offset_bytes, size, map);
	if (err < 0)
		goto fail_vmap_phy_pages;

	addr = map->addr;

out_unlock:
	kbase_gpu_vm_unlock(kctx);
	return addr;

fail_vmap_phy_pages:
	kbase_gpu_vm_unlock(kctx);
	kbase_mem_phy_alloc_put(cpu_alloc);
	kbase_mem_phy_alloc_put(gpu_alloc);

	return NULL;
}

void *kbase_vmap(struct kbase_context *kctx, u64 gpu_addr, size_t size,
		struct kbase_vmap_struct *map)
{
	/* 0 is specified for prot_request to indicate no access checks should
	 * be made.
	 *
	 * As mentioned in kbase_vmap_prot() this means that a kernel-side
	 * CPU-RO mapping is not enforced to allow this to work */
	return kbase_vmap_prot(kctx, gpu_addr, size, 0u, map);
}
KBASE_EXPORT_TEST_API(kbase_vmap);

static void kbase_vunmap_phy_pages(struct kbase_context *kctx,
		struct kbase_vmap_struct *map)
{
	void *addr = (void *)((uintptr_t)map->addr & PAGE_MASK);
	vunmap(addr);

	if (map->sync_needed)
		kbase_sync_mem_regions(kctx, map, KBASE_SYNC_TO_DEVICE);

	map->offset_in_page = 0;
	map->cpu_pages = NULL;
	map->gpu_pages = NULL;
	map->addr = NULL;
	map->size = 0;
	map->sync_needed = false;
}

void kbase_vunmap(struct kbase_context *kctx, struct kbase_vmap_struct *map)
{
	kbase_vunmap_phy_pages(kctx, map);
	map->cpu_alloc = kbase_mem_phy_alloc_put(map->cpu_alloc);
	map->gpu_alloc = kbase_mem_phy_alloc_put(map->gpu_alloc);
}
KBASE_EXPORT_TEST_API(kbase_vunmap);

void kbasep_os_process_page_usage_update(struct kbase_context *kctx, int pages)
{
	struct mm_struct *mm;

	rcu_read_lock();
	mm = rcu_dereference(kctx->process_mm);
	if (mm) {
		atomic_add(pages, &kctx->nonmapped_pages);
#ifdef SPLIT_RSS_COUNTING
		add_mm_counter(mm, MM_FILEPAGES, pages);
#else
		spin_lock(&mm->page_table_lock);
		add_mm_counter(mm, MM_FILEPAGES, pages);
		spin_unlock(&mm->page_table_lock);
#endif
	}
	rcu_read_unlock();
}

static void kbasep_os_process_page_usage_drain(struct kbase_context *kctx)
{
	int pages;
	struct mm_struct *mm;

	spin_lock(&kctx->mm_update_lock);
	mm = rcu_dereference_protected(kctx->process_mm, lockdep_is_held(&kctx->mm_update_lock));
	if (!mm) {
		spin_unlock(&kctx->mm_update_lock);
		return;
	}

	rcu_assign_pointer(kctx->process_mm, NULL);
	spin_unlock(&kctx->mm_update_lock);
	synchronize_rcu();

	pages = atomic_xchg(&kctx->nonmapped_pages, 0);
#ifdef SPLIT_RSS_COUNTING
	add_mm_counter(mm, MM_FILEPAGES, -pages);
#else
	spin_lock(&mm->page_table_lock);
	add_mm_counter(mm, MM_FILEPAGES, -pages);
	spin_unlock(&mm->page_table_lock);
#endif
}

static void kbase_special_vm_close(struct vm_area_struct *vma)
{
	struct kbase_context *kctx;

	kctx = vma->vm_private_data;
	kbasep_os_process_page_usage_drain(kctx);
}

static const struct vm_operations_struct kbase_vm_special_ops = {
	.close = kbase_special_vm_close,
};

static int kbase_tracking_page_setup(struct kbase_context *kctx, struct vm_area_struct *vma)
{
	/* check that this is the only tracking page */
	spin_lock(&kctx->mm_update_lock);
	if (rcu_dereference_protected(kctx->process_mm, lockdep_is_held(&kctx->mm_update_lock))) {
		spin_unlock(&kctx->mm_update_lock);
		return -EFAULT;
	}

	rcu_assign_pointer(kctx->process_mm, current->mm);

	spin_unlock(&kctx->mm_update_lock);

	/* no real access */
	vma->vm_flags &= ~(VM_READ | VM_MAYREAD | VM_WRITE | VM_MAYWRITE | VM_EXEC | VM_MAYEXEC);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0))
	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP | VM_IO;
#else
	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED | VM_IO;
#endif
	vma->vm_ops = &kbase_vm_special_ops;
	vma->vm_private_data = kctx;

	return 0;
}

