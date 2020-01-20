/* linux/drivers/iommu/exynos_iovmm.c
 *
 * Copyright (c) 2011-2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef CONFIG_EXYNOS_IOMMU_DEBUG
#define DEBUG
#endif

#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/delay.h>

#include <linux/exynos_iovmm.h>

#include "exynos-iommu.h"

#define sg_physically_continuous(sg) (sg_next(sg) == NULL)

/* alloc_iovm_region - Allocate IO virtual memory region
 * vmm: virtual memory allocator
 * size: total size to allocate vm region from @vmm.
 * align: alignment constraints of the allocated virtual address
 * max_align: maximum alignment of allocated virtual address. allocated address
 *            does not need to satisfy larger alignment than max_align.
 * section_offset: page size-aligned offset of iova start address within an 1MB
 *            boundary. The caller of alloc_iovm_region will obtain the
 *            allocated iova + section_offset. This is provided just for the
 *            physically contiguous memory.
 * page_offset: must be smaller than PAGE_SIZE. Just a valut to be added to the
 *         allocated virtual address. This does not effect to the allocaded size
 *         and address.
 *
 * This function returns allocated IO virtual address that satisfies the given
 * constraints: the caller will get the allocated virtual address plus
 * (section_offset + page_offset). Returns 0 if this function is not able
 * to allocate IO virtual memory.
 */
static dma_addr_t alloc_iovm_region(struct exynos_iovmm *vmm, size_t size,
			size_t section_offset,
			off_t page_offset)
{
	u32 index = 0;
	u32 vstart;
	u32 vsize;
	unsigned long end, i;
	struct exynos_vm_region *region;
	size_t align = SZ_1M;

	BUG_ON(page_offset >= PAGE_SIZE);

	/* To avoid allocating prefetched iovm region */
	vsize = (ALIGN(size + SZ_128K, SZ_128K) + section_offset) >> PAGE_SHIFT;
	align >>= PAGE_SHIFT;
	section_offset >>= PAGE_SHIFT;

	spin_lock(&vmm->bitmap_lock);
again:
	index = find_next_zero_bit(vmm->vm_map,
			IOVM_NUM_PAGES(vmm->iovm_size), index);

	if (align) {
		index = ALIGN(index, align);
		if (index >= IOVM_NUM_PAGES(vmm->iovm_size)) {
			spin_unlock(&vmm->bitmap_lock);
			return 0;
		}

		if (test_bit(index, vmm->vm_map))
			goto again;
	}

	end = index + vsize;

	if (end >= IOVM_NUM_PAGES(vmm->iovm_size)) {
		spin_unlock(&vmm->bitmap_lock);
		return 0;
	}

	i = find_next_bit(vmm->vm_map, end, index);
	if (i < end) {
		index = i + 1;
		goto again;
	}

	bitmap_set(vmm->vm_map, index, vsize);

	spin_unlock(&vmm->bitmap_lock);

	vstart = (index << PAGE_SHIFT) + vmm->iova_start + page_offset;

	region = kmalloc(sizeof(*region), GFP_KERNEL);
	if (unlikely(!region)) {
		spin_lock(&vmm->bitmap_lock);
		bitmap_clear(vmm->vm_map, index, vsize);
		spin_unlock(&vmm->bitmap_lock);
		return 0;
	}

	INIT_LIST_HEAD(&region->node);
	region->start = vstart;
	region->size = vsize << PAGE_SHIFT;
	region->dummy_size = region->size - size;
	region->section_off = (unsigned int)(section_offset << PAGE_SHIFT);

	spin_lock(&vmm->vmlist_lock);
	list_add_tail(&region->node, &vmm->regions_list);
	vmm->allocated_size += region->size;
	vmm->num_areas++;
	vmm->num_map++;
	spin_unlock(&vmm->vmlist_lock);

	return region->start + region->section_off;
}

struct exynos_vm_region *find_iovm_region(struct exynos_iovmm *vmm,
							dma_addr_t iova)
{
	struct exynos_vm_region *region;

	spin_lock(&vmm->vmlist_lock);

	list_for_each_entry(region, &vmm->regions_list, node) {
		if (region->start <= iova &&
			(region->start + region->size) > iova) {
			spin_unlock(&vmm->vmlist_lock);
			return region;
		}
	}

	spin_unlock(&vmm->vmlist_lock);

	return NULL;
}

static struct exynos_vm_region *remove_iovm_region(struct exynos_iovmm *vmm,
							dma_addr_t iova)
{
	struct exynos_vm_region *region;

	spin_lock(&vmm->vmlist_lock);

	list_for_each_entry(region, &vmm->regions_list, node) {
		if (region->start + region->section_off == iova) {
			list_del(&region->node);
			vmm->allocated_size -= region->size;
			vmm->num_areas--;
			vmm->num_unmap++;
			spin_unlock(&vmm->vmlist_lock);
			return region;
		}
	}

	spin_unlock(&vmm->vmlist_lock);

	return NULL;
}

static void free_iovm_region(struct exynos_iovmm *vmm,
				struct exynos_vm_region *region)
{
	if (!region)
		return;

	spin_lock(&vmm->bitmap_lock);
	bitmap_clear(vmm->vm_map,
		(region->start - vmm->iova_start) >> PAGE_SHIFT,
			region->size >> PAGE_SHIFT);
	spin_unlock(&vmm->bitmap_lock);

	SYSMMU_EVENT_LOG_IOVMM_UNMAP(IOVMM_TO_LOG(vmm),
			region->start, region->start + region->size);

	kfree(region);
}

static dma_addr_t add_iovm_region(struct exynos_iovmm *vmm,
					dma_addr_t start, size_t size)
{
	struct exynos_vm_region *region, *pos;

	region = kzalloc(sizeof(*region), GFP_KERNEL);
	if (!region)
		return 0;

	INIT_LIST_HEAD(&region->node);
	region->start = start;
	region->size = (u32)size;

	spin_lock(&vmm->vmlist_lock);

	list_for_each_entry(pos, &vmm->regions_list, node) {
		if ((start < (pos->start + pos->size)) &&
					((start + size) > pos->start)) {
			spin_unlock(&vmm->vmlist_lock);
			kfree(region);
			return 0;
		}
	}

	list_add(&region->node, &vmm->regions_list);

	spin_unlock(&vmm->vmlist_lock);

	return start;
}

static void show_iovm_regions(struct exynos_iovmm *vmm)
{
	struct exynos_vm_region *pos;

	pr_err("LISTING IOVMM REGIONS...\n");
	spin_lock(&vmm->vmlist_lock);
	list_for_each_entry(pos, &vmm->regions_list, node) {
		pr_err("REGION: %#x (SIZE: %#x, +[%#x, %#x])\n",
				pos->start, pos->size,
				pos->section_off, pos->dummy_size);
	}
	spin_unlock(&vmm->vmlist_lock);
	pr_err("END OF LISTING IOVMM REGIONS...\n");
}

int iovmm_activate(struct device *dev)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);

	if (!vmm) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return -EINVAL;
	}

	return iommu_attach_device(vmm->domain, dev);
}

void iovmm_deactivate(struct device *dev)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);

	if (!vmm) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return;
	}

	iommu_detach_device(vmm->domain, dev);
}

struct iommu_domain *get_domain_from_dev(struct device *dev)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);

	if (!vmm) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return NULL;
	}

	return vmm->domain;
}

/* iovmm_map - allocate and map IO virtual memory for the given device
 * dev: device that has IO virtual address space managed by IOVMM
 * sg: list of physically contiguous memory chunks. The preceding chunk needs to
 *     be larger than the following chunks in sg for efficient mapping and
 *     performance. If elements of sg are more than one, physical address of
 *     each chunk needs to be aligned by its size for efficent mapping and TLB
 *     utilization.
 * offset: offset in bytes to be mapped and accessed by dev.
 * size: size in bytes to be mapped and accessed by dev.
 *
 * This function allocates IO virtual memory for the given device and maps the
 * given physical memory conveyed by sg into the allocated IO memory region.
 * Returns allocated IO virtual address if it allocates and maps successfull.
 * Otherwise, minus error number. Caller must check if the return value of this
 * function with IS_ERR_VALUE().
 */
dma_addr_t iovmm_map(struct device *dev, struct scatterlist *sg, off_t offset,
		     size_t size, enum dma_data_direction direction, int prot)
{
	off_t start_off;
	dma_addr_t addr, start = 0;
	size_t mapped_size = 0;
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);
	size_t section_offset = 0; /* section offset of contig. mem */
	int ret = 0;
	int idx;
	struct scatterlist *tsg;
	struct exynos_vm_region *region;

	if (vmm == NULL) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return -EINVAL;
	}

	for (; (sg != NULL) && (sg->length < offset); sg = sg_next(sg))
		offset -= sg->length;

	if (sg == NULL) {
		dev_err(dev, "IOVMM: invalid offset to %s.\n", __func__);
		return -EINVAL;
	}

	tsg = sg;

	start_off = offset_in_page(sg_phys(sg) + offset);
	size = PAGE_ALIGN(size + start_off);

	if (sg_physically_continuous(sg)) {
		size_t aligned_pad_size;
		phys_addr_t phys = page_to_phys(sg_page(sg));
		section_offset = phys & (~SECT_MASK);
		aligned_pad_size = ALIGN(phys, SECT_SIZE) - phys;
		if ((sg->length - aligned_pad_size) < SECT_SIZE) {
			aligned_pad_size = ALIGN(phys, LPAGE_SIZE) - phys;
			if ((sg->length - aligned_pad_size) >= LPAGE_SIZE)
				section_offset = phys & (~LPAGE_MASK);
			else
				section_offset = 0;
		}
	}
	start = alloc_iovm_region(vmm, size, section_offset, start_off);
	if (!start) {
		spin_lock(&vmm->vmlist_lock);
		dev_err(dev, "%s: Not enough IOVM space to allocate %#zx\n",
				__func__, size);
		dev_err(dev, "%s: Total %#zx, Allocated %#zx , Chunks %d\n",
				__func__, vmm->iovm_size,
				vmm->allocated_size, vmm->num_areas);
		spin_unlock(&vmm->vmlist_lock);
		ret = -ENOMEM;
		goto err_map_nomem;
	}

	addr = start - start_off;

	do {
		phys_addr_t phys;
		size_t len;

		phys = sg_phys(sg);
		len = sg->length;

		/* if back to back sg entries are contiguous consolidate them */
		while (sg_next(sg) &&
		       sg_phys(sg) + sg->length == sg_phys(sg_next(sg))) {
			len += sg_next(sg)->length;
			sg = sg_next(sg);
		}

		if (offset > 0) {
			len -= offset;
			phys += offset;
			offset = 0;
		}

		if (offset_in_page(phys)) {
			len += offset_in_page(phys);
			phys = round_down(phys, PAGE_SIZE);
		}

		len = PAGE_ALIGN(len);

		if (len > (size - mapped_size))
			len = size - mapped_size;

		ret = iommu_map(vmm->domain, addr, phys, len, prot);
		if (ret) {
			dev_err(dev, "iommu_map failed w/ err: %d\n", ret);
			break;
		}

		addr += len;
		mapped_size += len;
	} while ((sg = sg_next(sg)) && (mapped_size < size));

	BUG_ON(mapped_size > size);

	if (mapped_size < size) {
		dev_err(dev, "mapped_size(%#zx) is smaller than size(%#zx)\n",
				mapped_size, size);
		if (!ret) {
			dev_err(dev, "ret: %d\n", ret);
			ret = -EINVAL;
		}
		goto err_map_map;
	}

	region = find_iovm_region(vmm, start);
	BUG_ON(!region);

	/*
	 * If pretched SLPD is a fault SLPD in zero_l2_table, FLPD cache
	 * or prefetch buffer caches the address of zero_l2_table.
	 * This function replaces the zero_l2_table with new L2 page
	 * table to write valid mappings.
	 * Accessing the valid area may cause page fault since FLPD
	 * cache may still caches zero_l2_table for the valid area
	 * instead of new L2 page table that have the mapping
	 * information of the valid area
	 * Thus any replacement of zero_l2_table with other valid L2
	 * page table must involve FLPD cache invalidation if the System
	 * MMU have prefetch feature and FLPD cache (version 3.3).
	 * FLPD cache invalidation is performed with TLB invalidation
	 * by VPN without blocking. It is safe to invalidate TLB without
	 * blocking because the target address of TLB invalidation is
	 * not currently mapped.
	 */

	/* TODO: for sysmmu v6, remove it later */
	exynos_sysmmu_tlb_invalidate(vmm->domain, region->start, region->size);

	dev_dbg(dev, "IOVMM: Allocated VM region @ %#x/%#x bytes.\n",
				(unsigned int)start, (unsigned int)size);

	SYSMMU_EVENT_LOG_IOVMM_MAP(IOVMM_TO_LOG(vmm), start, start + size,
						region->size - size);

	return start;

err_map_map:
	iommu_unmap(vmm->domain, start - start_off, mapped_size);
	free_iovm_region(vmm, remove_iovm_region(vmm, start));

	dev_err(dev,
	"Failed(%d) to map IOVMM REGION %pa (SIZE: %#zx, mapped: %#zx)\n",
		ret, &start, size, mapped_size);
	idx = 0;
	do {
		pr_err("SGLIST[%d].size = %#x\n", idx++, tsg->length);
	} while ((tsg = sg_next(tsg)));

	show_iovm_regions(vmm);

err_map_nomem:
	dev_dbg(dev,
		"IOVMM: Failed to allocated VM region for %#x bytes.\n",
							(unsigned int)size);
	return (dma_addr_t)ret;
}

void iovmm_unmap(struct device *dev, dma_addr_t iova)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);
	struct exynos_vm_region *region;
	size_t unmap_size;

	/* This function must not be called in IRQ handlers */
	BUG_ON(in_irq());

	if (vmm == NULL) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return;
	}

	region = remove_iovm_region(vmm, iova);
	if (region) {
		u32 start = region->start + region->section_off;
		u32 size = region->size - region->dummy_size;

		/* clear page offset */
		if (WARN_ON(start != iova)) {
			dev_err(dev, "IOVMM: "
			   "iova %pa and region %#x(+%#x)@%#x(-%#x) mismatch\n",
				&iova, region->size, region->dummy_size,
				region->start, region->section_off);
			show_iovm_regions(vmm);
			/* reinsert iovm region */
			add_iovm_region(vmm, region->start, region->size);
			kfree(region);
			return;
		}
		unmap_size = iommu_unmap(vmm->domain, start & SPAGE_MASK, size);
		if (unlikely(unmap_size != size)) {
			dev_err(dev,
				"Failed to unmap REGION of %#x:\n", start);
			dev_err(dev, "(SIZE: %#x, iova: %pa, unmapped: %#zx)\n",
					 size, &iova, unmap_size);
			show_iovm_regions(vmm);
			kfree(region);
			BUG();
			return;
		}

		exynos_sysmmu_tlb_invalidate(vmm->domain, region->start, region->size);

		/* TODO: for sysmmu v6, remove it later */
		/* 60us is required to guarantee that PTW ends itself */
		udelay(60);

		free_iovm_region(vmm, region);

		dev_dbg(dev, "IOVMM: Unmapped %#x bytes from %#x.\n",
				(unsigned int)unmap_size, (unsigned int)iova);
	} else {
		dev_err(dev, "IOVMM: No IOVM region %pa to free.\n", &iova);
	}
}

/*
 * NOTE:
 * exynos_iovmm_map_userptr() should be called under current->mm.mmap_sem held.
 */
dma_addr_t exynos_iovmm_map_userptr(struct device *dev, unsigned long vaddr,
				    size_t size, int prot)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);
	unsigned long eaddr = vaddr + size;
	off_t offset = offset_in_page(vaddr);
	int ret = -EINVAL;
	struct vm_area_struct *vma;
	dma_addr_t start;
	struct exynos_vm_region *region;

	vma = find_vma(current->mm, vaddr);
	if (!vma || (vaddr < vma->vm_start)) {
		dev_err(dev, "%s: invalid address %#lx\n", __func__, vaddr);
		goto err;
	}

	if (!!(vma->vm_flags & VM_PFNMAP))
		prot |= IOMMU_PFNMAP;

	/* non-cached user mapping should be treated as non-shareable mapping */
	if ((pgprot_val(pgprot_writecombine(vma->vm_page_prot)) ==
					pgprot_val(vma->vm_page_prot)) ||
		(pgprot_val(pgprot_noncached(vma->vm_page_prot)) ==
					pgprot_val(vma->vm_page_prot)))
		prot &= ~IOMMU_CACHE;
	else if (device_get_dma_attr(dev) == DEV_DMA_COHERENT)
		prot |= IOMMU_CACHE;

	while (eaddr > vma->vm_end) {
		if (!!(vma->vm_flags & VM_PFNMAP)) {
			dev_err(dev, "%s: non-linear pfnmap is not supported\n",
				__func__);
			goto err;
		}

		if ((vma->vm_next == NULL) ||
				(vma->vm_end != vma->vm_next->vm_start)) {
			dev_err(dev, "%s: invalid size %zu\n", __func__, size);
			goto err;
		}

		vma = vma->vm_next;
	}

	size = PAGE_ALIGN(size + offset);
	start = alloc_iovm_region(vmm, size, 0, offset);
	if (!start) {
		spin_lock(&vmm->vmlist_lock);
		dev_err(dev, "%s: Not enough IOVM space to allocate %#zx\n",
				__func__, size);
		dev_err(dev, "%s: Total %#zx, Allocated %#zx , Chunks %d\n",
				__func__, vmm->iovm_size,
				vmm->allocated_size, vmm->num_areas);
		spin_unlock(&vmm->vmlist_lock);
		ret = -ENOMEM;
		goto err;
	}

	ret = exynos_iommu_map_userptr(vmm->domain, vaddr - offset,
					start - offset, size, prot);
	if (ret < 0)
		goto err_map;

	region = find_iovm_region(vmm, start);
	BUG_ON(!region);

	SYSMMU_EVENT_LOG_IOVMM_MAP(IOVMM_TO_LOG(vmm), start, start + size,
						region->size - size);
	return start;
err_map:
	free_iovm_region(vmm, remove_iovm_region(vmm, start));
err:
	return (dma_addr_t)ret;
}

void exynos_iovmm_unmap_userptr(struct device *dev, dma_addr_t iova)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);
	struct exynos_vm_region *region;

	region = remove_iovm_region(vmm, iova);
	if (region) {
		u32 start = region->start + region->section_off;
		u32 size = region->size - region->dummy_size;

		/* clear page offset */
		if (WARN_ON(start != iova)) {
			dev_err(dev, "IOVMM: "
			   "iova %pa and region %#x(+%#x)@%#x(-%#x) mismatch\n",
				&iova, region->size, region->dummy_size,
				region->start, region->section_off);
			show_iovm_regions(vmm);
			/* reinsert iovm region */
			add_iovm_region(vmm, region->start, region->size);
			kfree(region);
			return;
		}

		exynos_iommu_unmap_userptr(vmm->domain,
					   start & SPAGE_MASK, size);

		free_iovm_region(vmm, region);
	} else {
		dev_err(dev, "IOVMM: No IOVM region %pa to free.\n", &iova);
	}
}

int iovmm_map_oto(struct device *dev, phys_addr_t phys, size_t size)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);
	int ret;

	BUG_ON(!IS_ALIGNED(phys, PAGE_SIZE));
	BUG_ON(!IS_ALIGNED(size, PAGE_SIZE));

	if (vmm == NULL) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return -EINVAL;
	}

	if (WARN_ON((phys < vmm->iova_start) ||
			(phys + size) >= (vmm->iova_start + vmm->iovm_size))) {
		dev_err(dev,
			"Unable to create one to one mapping for %#zx @ %pa\n",
			size, &phys);
		return -EINVAL;
	}

	if (!add_iovm_region(vmm, (dma_addr_t)phys, size))
		return -EADDRINUSE;

	ret = iommu_map(vmm->domain, (dma_addr_t)phys, phys, size, 0);
	if (ret < 0)
		free_iovm_region(vmm,
				remove_iovm_region(vmm, (dma_addr_t)phys));

	return ret;
}

void iovmm_unmap_oto(struct device *dev, phys_addr_t phys)
{
	struct exynos_iovmm *vmm = exynos_get_iovmm(dev);
	struct exynos_vm_region *region;
	size_t unmap_size;

	/* This function must not be called in IRQ handlers */
	BUG_ON(in_irq());
	BUG_ON(!IS_ALIGNED(phys, PAGE_SIZE));

	if (vmm == NULL) {
		dev_err(dev, "%s: IOVMM not found\n", __func__);
		return;
	}

	region = remove_iovm_region(vmm, (dma_addr_t)phys);
	if (region) {
		unmap_size = iommu_unmap(vmm->domain, (dma_addr_t)phys,
							region->size);
		WARN_ON(unmap_size != region->size);

		exynos_sysmmu_tlb_invalidate(vmm->domain, (dma_addr_t)phys,
					     region->size);

		free_iovm_region(vmm, region);

		dev_dbg(dev, "IOVMM: Unmapped %#zx bytes from %pa.\n",
						unmap_size, &phys);
	}
}

static struct dentry *exynos_iovmm_debugfs_root;
static struct dentry *exynos_iommu_debugfs_root;

static int exynos_iovmm_create_debugfs(void)
{
	exynos_iovmm_debugfs_root = debugfs_create_dir("iovmm", NULL);
	if (!exynos_iovmm_debugfs_root)
		pr_err("IOVMM: Failed to create debugfs entry\n");
	else
		pr_info("IOVMM: Created debugfs entry at debugfs/iovmm\n");

	exynos_iommu_debugfs_root = debugfs_create_dir("iommu", NULL);
	if (!exynos_iommu_debugfs_root)
		pr_err("IOMMU: Failed to create debugfs entry\n");
	else
		pr_info("IOMMU: Created debugfs entry at debugfs/iommu\n");

	return 0;
}
core_initcall(exynos_iovmm_create_debugfs);

static int iovmm_debug_show(struct seq_file *s, void *unused)
{
	struct exynos_iovmm *vmm = s->private;

	seq_printf(s, "%10.s  %10.s  %10.s  %6.s\n",
			"VASTART", "SIZE", "FREE", "CHUNKS");
	seq_puts(s, "---------------------------------------------\n");

	spin_lock(&vmm->vmlist_lock);
	seq_printf(s, "  %#x  %#10zx  %#10zx  %d\n",
			vmm->iova_start, vmm->iovm_size,
			vmm->iovm_size - vmm->allocated_size,
			vmm->num_areas);
	seq_puts(s, "---------------------------------------------\n");
	seq_printf(s, "Total number of mappings  : %d\n", vmm->num_map);
	seq_printf(s, "Total number of unmappings: %d\n", vmm->num_unmap);
	spin_unlock(&vmm->vmlist_lock);

	return 0;
}

static int iovmm_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, iovmm_debug_show, inode->i_private);
}

static ssize_t iovmm_debug_write(struct file *filp, const char __user *p,
				size_t len, loff_t *off)
{
	struct seq_file *s = filp->private_data;
	struct exynos_iovmm *vmm = s->private;
	/* clears the map count in IOVMM */
	spin_lock(&vmm->vmlist_lock);
	vmm->num_map = 0;
	vmm->num_unmap = 0;
	spin_unlock(&vmm->vmlist_lock);
	return len;
}

static const struct file_operations iovmm_debug_fops = {
	.open = iovmm_debug_open,
	.read = seq_read,
	.write = iovmm_debug_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void iovmm_register_debugfs(struct exynos_iovmm *vmm)
{
	if (!exynos_iovmm_debugfs_root)
		return;

	debugfs_create_file(vmm->domain_name, 0664,
			exynos_iovmm_debugfs_root, vmm, &iovmm_debug_fops);
}

struct exynos_iovmm *exynos_create_single_iovmm(const char *name,
					unsigned int start, unsigned int end)
{
	struct exynos_iovmm *vmm;
	int ret = 0;

	vmm = kzalloc(sizeof(*vmm), GFP_KERNEL);
	if (!vmm) {
		ret = -ENOMEM;
		goto err_alloc_vmm;
	}

	vmm->iovm_size = (size_t)(end - start);
	vmm->iova_start = start;
	vmm->vm_map = kzalloc(IOVM_BITMAP_SIZE(vmm->iovm_size), GFP_KERNEL);
	if (!vmm->vm_map) {
		ret = -ENOMEM;
		goto err_setup_domain;
	}

	vmm->domain = iommu_domain_alloc(&platform_bus_type);
	if (!vmm->domain) {
		ret = -ENOMEM;
		goto err_setup_domain;
	}

	ret = exynos_iommu_init_event_log(IOVMM_TO_LOG(vmm), IOVMM_LOG_LEN);
	if (!ret) {
		iovmm_add_log_to_debugfs(exynos_iovmm_debugfs_root,
				IOVMM_TO_LOG(vmm), name);
		iommu_add_log_to_debugfs(exynos_iommu_debugfs_root,
				IOMMU_TO_LOG(vmm->domain), name);
	} else {
		goto err_init_event_log;
	}

	spin_lock_init(&vmm->vmlist_lock);
	spin_lock_init(&vmm->bitmap_lock);

	INIT_LIST_HEAD(&vmm->regions_list);

	vmm->domain_name = name;

	iovmm_register_debugfs(vmm);

	pr_debug("%s IOVMM: Created %#zx B IOVMM from %#x.\n",
			name, vmm->iovm_size, vmm->iova_start);
	return vmm;

err_init_event_log:
	iommu_domain_free(vmm->domain);
err_setup_domain:
	kfree(vmm);
err_alloc_vmm:
	pr_err("%s IOVMM: Failed to create IOVMM (%d)\n", name, ret);

	return ERR_PTR(ret);
}

void iovmm_set_fault_handler(struct device *dev,
			     iommu_fault_handler_t handler, void *token)
{
	int ret;

	ret = exynos_iommu_add_fault_handler(dev, handler, token);
	if (ret)
		dev_err(dev, "Failed to add fault handler\n");
}
