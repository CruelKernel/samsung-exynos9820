/* /include/linux/exynos_iovmm.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_PLAT_IOVMM_H
#define __ASM_PLAT_IOVMM_H

#include <linux/dma-direction.h>
#include <linux/iommu.h>

#include <asm/page.h>
#include <linux/mm_types.h>

#define IOMMU_PFNMAP	(1 << 5) /* VM_PFNMAP is set */

struct scatterlist;
struct device;

typedef u32 exynos_iova_t;

#define SYSMMU_FAULT_BITS	4
#define SYSMMU_FAULT_SHIFT	16
#define SYSMMU_FAULT_MASK	((1 << SYSMMU_FAULT_BITS) - 1)
#define SYSMMU_FAULT_FLAG(id) (((id) & SYSMMU_FAULT_MASK) << SYSMMU_FAULT_SHIFT)
#define SYSMMU_FAULT_ID(fg)   (((fg) >> SYSMMU_FAULT_SHIFT) & SYSMMU_FAULT_MASK)

#define SYSMMU_FAULT_PTW_ACCESS   0
#define SYSMMU_FAULT_PAGE_FAULT   1
#define SYSMMU_FAULT_TLB_MULTIHIT 2
#define SYSMMU_FAULT_ACCESS       3
#define SYSMMU_FAULT_SECURITY     4
#define SYSMMU_FAULT_UNKNOWN      5

#define IOMMU_FAULT_EXYNOS_PTW_ACCESS SYSMMU_FAULT_FLAG(SYSMMU_FAULT_PTW_ACCESS)
#define IOMMU_FAULT_EXYNOS_PAGE_FAULT SYSMMU_FAULT_FLAG(SYSMMU_FAULT_PAGE_FAULT)
#define IOMMU_FAULT_EXYNOS_TLB_MULTIHIT \
				SYSMMU_FAULT_FLAG(SYSMMU_FAULT_TLB_MULTIHIT)
#define IOMMU_FAULT_EXYNOS_ACCESS     SYSMMU_FAULT_FLAG(SYSMMU_FAULT_ACCESS)
#define IOMMU_FAULT_EXYNOS_SECURITY   SYSMMU_FAULT_FLAG(SYSMMU_FAULT_SECURITY)
#define IOMMU_FAULT_EXYNOS_UNKNOWN    SYSMMU_FAULT_FLAG(SYSMMU_FAULT_UNKOWN)

/* TODO: PB related for sysmmu v6, remove it later */
#define SYSMMU_PBUFCFG_TLB_UPDATE      (1 << 16)
#define SYSMMU_PBUFCFG_ASCENDING       (1 << 12)
#define SYSMMU_PBUFCFG_DESCENDING      (0 << 12)
#define SYSMMU_PBUFCFG_PREFETCH                (1 << 8)
#define SYSMMU_PBUFCFG_WRITE           (1 << 4)
#define SYSMMU_PBUFCFG_READ            (0 << 4)

#define SYSMMU_PBUFCFG_DEFAULT_INPUT   (SYSMMU_PBUFCFG_TLB_UPDATE | \
					SYSMMU_PBUFCFG_ASCENDING |  \
					SYSMMU_PBUFCFG_PREFETCH |   \
					SYSMMU_PBUFCFG_READ)
#define SYSMMU_PBUFCFG_DEFAULT_OUTPUT  (SYSMMU_PBUFCFG_TLB_UPDATE | \
					SYSMMU_PBUFCFG_ASCENDING |  \
					SYSMMU_PBUFCFG_PREFETCH |   \
					SYSMMU_PBUFCFG_WRITE)

#define SYSMMU_PBUFCFG_ASCENDING_INPUT   (SYSMMU_PBUFCFG_TLB_UPDATE | \
					SYSMMU_PBUFCFG_ASCENDING |  \
					SYSMMU_PBUFCFG_PREFETCH |   \
					SYSMMU_PBUFCFG_READ)

#define SYSMMU_PBUFCFG_DESCENDING_INPUT   (SYSMMU_PBUFCFG_TLB_UPDATE | \
					SYSMMU_PBUFCFG_DESCENDING |  \
					SYSMMU_PBUFCFG_PREFETCH |   \
					SYSMMU_PBUFCFG_READ)
/* SYSMMU PPC Event ID */
enum sysmmu_ppc_event {
	READ_TOTAL,
	READ_L1TLB_MISS,
	READ_L2TLB_MISS,
	READ_FLPD_MISS,
	READ_PB_LOOKUP,
	READ_PB_MISS,
	READ_BLOCK_NUM_BY_PREFETCH,
	READ_BLOCK_CYCLE_BRY_PREFETCH,
	READ_TLB_MISS,
	READ_FLPD_MISS_PREFETCH,
	WRITE_TOTAL = 0x10,
	WRITE_L1TLB_MISS,
	WRITE_L2TLB_MISS,
	WRITE_FLPD_MISS,
	WRITE_PB_LOOKUP,
	WRITE_PB_MISS,
	WRITE_BLOCK_NUM_BY_PREFETCH,
	WRITE_BLOCK_CYCLE_BY_PREFETCH,
	WRITE_TLB_MISS,
	WRITE_FLPD_MISS_PREFETCH,
	TOTAL_ID_NUM,
};

struct sysmmu_prefbuf {
	unsigned long base;
	unsigned long size;
	unsigned long config;
};

#if defined(CONFIG_EXYNOS_IOVMM)
int iovmm_activate(struct device *dev);
void iovmm_deactivate(struct device *dev);
struct iommu_domain *get_domain_from_dev(struct device *dev);

/* iovmm_map() - Maps a list of physical memory chunks
 * @dev: the owner of the IO address space where the mapping is created
 * @sg: list of physical memory chunks to map
 * @offset: length in bytes where the mapping starts
 * @size: how much memory to map in bytes. @offset + @size must not exceed
 *        total size of @sg
 * @direction: dma data direction for iova
 * @prot: iommu mapping property
 *
 * This function returns mapped IO address in the address space of @dev.
 * Returns minus error number if mapping fails.
 * Caller must check its return code with IS_ERROR_VALUE() if the function
 * succeeded.
 *
 * The caller of this function must ensure that iovmm_cleanup() is not called
 * while this function is called.
 *
 */
dma_addr_t iovmm_map(struct device *dev, struct scatterlist *sg, off_t offset,
		size_t size, enum dma_data_direction direction, int prot);

/* iovmm_unmap() - unmaps the given IO address
 * @dev: the owner of the IO address space where @iova belongs
 * @iova: IO address that needs to be unmapped and freed.
 *
 * The caller of this function must ensure that iovmm_cleanup() is not called
 * while this function is called.
 */
void iovmm_unmap(struct device *dev, dma_addr_t iova);

/*
 * flags to option_iplanes and option_oplanes.
 * inplanes and onplanes is 'input planes' and 'output planes', respectively.
 *
 * default value to option_iplanes:
 *    (TLB_UPDATE | ASCENDING | PREFETCH)
 * default value to option_oplanes:
 *    (TLB_UPDATE | ASCENDING | PREFETCH | WRITE)
 *
 * SYSMMU_PBUFCFG_READ and SYSMMU_PBUFCFG_WRITE are ignored because they are
 * implicitly set from 'inplanes' and 'onplanes' arguments to
 * iovmm_set_prefetch_buffer().
 *
 * Guide to setting flags:
 * - Clear SYSMMU_BUFCFG_TLB_UPDATE if a buffer is accessed by the device
 *   for rotation.
 * - Set SYSMMU_PBUFCFG_DESCENDING if the device access a buffer in reversed
 *   order
 * - Clear SYSMMU_PBUFCFG_PREFETCH if access to a buffer has poor locality.
 * - Otherwise, always set flags as default value.
 */
#else
#define iovmm_activate(dev)		(-ENOSYS)
#define iovmm_deactivate(dev)		do { } while (0)
#define iovmm_map(dev, sg, offset, size, direction, prot) (-ENOSYS)
#define iovmm_unmap(dev, iova)		do { } while (0)
#define get_domain_from_dev(dev)	NULL
static inline dma_addr_t exynos_iovmm_map_userptr(struct device *dev,
			unsigned long vaddr, size_t size, int prot)
{
	return (dma_addr_t)-ENODEV;
}
#define exynos_iovmm_unmap_userptr(dev, iova) do { } while (0)

#endif /* CONFIG_EXYNOS_IOVMM */

#if defined(CONFIG_EXYNOS_IOMMU)
/**
 * exynos_sysmmu_map_user_pages() - maps all pages by fetching from
 * user page table entries.
 * @dev: The device whose System MMU is about to be disabled.
 * @mm: mm struct of user requested to map
 * @vaddr: start vaddr in valid vma
 * @iova: start io vaddr to be mapped
 * @size: size to map
 * @write: set if buffer may be written
 * @shareable: set shareable bit if true
 *
 * This function maps all user pages into sysmmu page table.
 */
int exynos_sysmmu_map_user_pages(struct device *dev,
					struct mm_struct *mm,
					unsigned long vaddr,
					exynos_iova_t iova,
					size_t size, bool write,
					bool shareable);

/**
 * exynos_sysmmu_unmap_user_pages() - unmaps all mapped pages
 * @dev: The device whose System MMU is about to be disabled.
 * @mm: mm struct of user requested to map
 * @vaddr: start vaddr in valid vma
 * @iova: start io vaddr to be unmapped
 * @size: size to map
 *
 * This function unmaps all user pages mapped in sysmmu page table.
 */
int exynos_sysmmu_unmap_user_pages(struct device *dev,
					struct mm_struct *mm,
					unsigned long vaddr,
					exynos_iova_t iova,
					size_t size);
/**
 * exynos_iommu_sync_for_device()
 *			- maintain cache lines on the given area before DMA
 * @dev:	The device that is about to see the area
 * @iova:	The start DMA address of @dev to maintain
 * @len:	The length of the area
 * @dir:	Indicate whether @dev read from or write to the area
 */
void exynos_iommu_sync_for_device(struct device *dev, dma_addr_t iova,
				  size_t len, enum dma_data_direction dir);

/**
 * exynos_iommu_sync_for_cpu()
 *			- maintain cache lines on the given area after DMA
 * @dev:	The device that is about to see the area
 * @iova:	The start DMA address of @dev to maintain
 * @len:	The length of the area
 * @dir:	Indicate whether @dev read from or write to the area
 */
void exynos_iommu_sync_for_cpu(struct device *dev, dma_addr_t iova, size_t len,
				enum dma_data_direction dir);

/**
 * TODO: description
 */
dma_addr_t exynos_iovmm_map_userptr(struct device *dev, unsigned long vaddr,
				    size_t size, int prot);
/**
 * TODO: description
 */
void exynos_iovmm_unmap_userptr(struct device *dev, dma_addr_t iova);

int iovmm_map_oto(struct device *dev, phys_addr_t phys, size_t size);
void iovmm_unmap_oto(struct device *dev, phys_addr_t phys);
/*
 * The handle_pte_fault() is called by exynos_sysmmu_map_user_pages().
 * Driver cannot include include/linux/huge_mm.h because
 * CONFIG_TRANSPARENT_HUGEPAGE is disabled.
 */
extern int handle_pte_fault(struct mm_struct *mm,
			    struct vm_area_struct *vma, unsigned long address,
			    pte_t *pte, pmd_t *pmd, unsigned int flags);

/*
 * sysmmu_set_prefetch_buffer_by_region() - set prefetch buffer configuration
 *
 * @dev: device descriptor of master device
 * @pb_reg: array of regions where prefetch buffer contains.
 *
 * If @dev is NULL or @pb_reg is 0, prefetch buffers is disabled.
 *
 */
void sysmmu_set_prefetch_buffer_by_region(struct device *dev,
			struct sysmmu_prefbuf pb_reg[], unsigned int num_reg);

int sysmmu_set_prefetch_buffer_property(struct device *dev,
			unsigned int inplanes, unsigned int onplanes,
			unsigned int ipoption[], unsigned int opoption[]);
void exynos_sysmmu_show_status(struct device *dev);
void exynos_sysmmu_dump_pgtable(struct device *dev);
void exynos_sysmmu_control(struct device *master, bool enable);

/*
 * exynos_sysmmu_set/clear/show_ppc_event() -
 *		set/clear/show system mmu ppc event
 *
 * @dev: device descriptor of master device.
 * @event: system mmu ppc event id.
 * Returns 0 if setting is successful. -EINVAL if the argument is invalid.
 *
 */
int exynos_sysmmu_set_ppc_event(struct device *dev, int event);
void exynos_sysmmu_clear_ppc_event(struct device *dev);
void exynos_sysmmu_show_ppc_event(struct device *dev);

/*
 * iovmm_set_fault_handler - register fault handler of dev to iommu controller
 * @dev: the device that wants to register fault handler
 * @handler: fault handler
 * @token: any data the device driver needs to get when fault occurred
 */
void iovmm_set_fault_handler(struct device *dev,
			     iommu_fault_handler_t handler, void *token);

#else
#define sysmmu_set_prefetch_buffer_property(dev, inplanes, onplnes, ipoption, opoption) \
					(0)
#define sysmmu_set_prefetch_buffer_by_region(dev, pb_reg, num_reg) \
					do { } while (0)
#define exynos_sysmmu_map_user_pages(dev, mm, vaddr, iova, size, write, sharable) \
					(-ENOSYS)
#define exynos_sysmmu_unmap_user_pages(dev, mm, vaddr, iova, size) \
					do { } while (0)
#define exynos_sysmmu_show_status(dev) do { } while (0)
#define exynos_sysmmu_dump_pgtable(dev) do { } while (0)

#define exynos_sysmmu_clear_ppc_event(dev) do { } while (0)
#define exynos_sysmmu_show_ppc_event(dev) do { } while (0)
#define exynos_sysmmu_set_ppc_event(dev, event) do { } while (0)
#define iovmm_set_fault_handler(dev, handler, token) do { } while(0)

#define exynos_iommu_sync_for_device(dev, iova, len, dir) do { } while (0)
#define exynos_iommu_sync_for_cpu(dev, iova, len, dir) do { } while (0)

#endif
#endif /*__ASM_PLAT_IOVMM_H*/
