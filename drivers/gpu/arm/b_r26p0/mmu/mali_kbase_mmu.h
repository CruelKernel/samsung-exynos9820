/*
 *
 * (C) COPYRIGHT 2019 ARM Limited. All rights reserved.
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

#ifndef _KBASE_MMU_H_
#define _KBASE_MMU_H_


#define KBASE_MMU_PAGE_ENTRIES 512

struct kbase_context;
struct kbase_mmu_table;

/**
 * enum kbase_caller_mmu_sync_info - MMU-synchronous caller info.
 * A pointer to this type is passed down from the outer-most callers in the kbase
 * module - where the information resides as to the synchronous / asynchronous
 * nature of the call flow, with respect to MMU operations. ie - does the call flow relate to
 * existing GPU work does it come from requests (like ioctl) from user-space, power management,
 * etc.
 *
 * @CALLER_MMU_UNSET_SYNCHRONICITY: default value must be invalid to avoid accidental choice
 *                                  of a 'valid' value
 * @CALLER_MMU_SYNC: Arbitrary value for 'synchronous that isn't easy to choose by accident
 * @CALLER_MMU_ASYNC: Also hard to choose by accident
 */
enum kbase_caller_mmu_sync_info {
	CALLER_MMU_UNSET_SYNCHRONICITY,
	CALLER_MMU_SYNC = 0x02,
	CALLER_MMU_ASYNC
};

/**
 * kbase_mmu_init - Initialise an object representing GPU page tables
 *
 * The structure should be terminated using kbase_mmu_term()
 *
 * @kbdev:    Instance of GPU platform device, allocated from the probe method.
 * @mmut:     GPU page tables to be initialized.
 * @kctx:     Optional kbase context, may be NULL if this set of MMU tables
 *            is not associated with a context.
 * @group_id: The physical group ID from which to allocate GPU page tables.
 *            Valid range is 0..(MEMORY_GROUP_MANAGER_NR_GROUPS-1).
 *
 * Return:    0 if successful, otherwise a negative error code.
 */
int kbase_mmu_init(struct kbase_device *kbdev, struct kbase_mmu_table *mmut,
		struct kbase_context *kctx, int group_id);

/**
 * kbase_mmu_interrupt - Process an MMU interrupt.
 *
 * Process the MMU interrupt that was reported by the &kbase_device.
 *
 * @kbdev:       Pointer to the kbase device for which the interrupt happened.
 * @irq_stat:    Value of the MMU_IRQ_STATUS register.
 */
void kbase_mmu_interrupt(struct kbase_device *kbdev, u32 irq_stat);

/**
 * kbase_mmu_term - Terminate an object representing GPU page tables
 *
 * This will free any page tables that have been allocated
 *
 * @kbdev: Instance of GPU platform device, allocated from the probe method.
 * @mmut:  GPU page tables to be destroyed.
 */
void kbase_mmu_term(struct kbase_device *kbdev, struct kbase_mmu_table *mmut);

/**
 * kbase_mmu_create_ate - Create an address translation entry
 *
 * @kbdev:    Instance of GPU platform device, allocated from the probe method.
 * @phy:      Physical address of the page to be mapped for GPU access.
 * @flags:    Bitmask of attributes of the GPU memory region being mapped.
 * @level:    Page table level for which to build an address translation entry.
 * @group_id: The physical memory group in which the page was allocated.
 *            Valid range is 0..(MEMORY_GROUP_MANAGER_NR_GROUPS-1).
 *
 * This function creates an address translation entry to encode the physical
 * address of a page to be mapped for access by the GPU, along with any extra
 * attributes required for the GPU memory region.
 *
 * Return: An address translation entry, either in LPAE or AArch64 format
 *         (depending on the driver's configuration).
 */
u64 kbase_mmu_create_ate(struct kbase_device *kbdev,
	struct tagged_addr phy, unsigned long flags, int level, int group_id);

int kbase_mmu_insert_pages_no_flush(struct kbase_device *kbdev, struct kbase_mmu_table *mmut,
				    const u64 start_vpfn, struct tagged_addr *phys, size_t nr,
				    unsigned long flags, int group_id, u64 *dirty_pgds);
int kbase_mmu_insert_pages(struct kbase_device *kbdev,
			   struct kbase_mmu_table *mmut, u64 vpfn,
			   struct tagged_addr *phys, size_t nr,
			   unsigned long flags, int as_nr, int group_id,
			   enum kbase_caller_mmu_sync_info mmu_sync_info);
int kbase_mmu_insert_single_page(struct kbase_context *kctx, u64 vpfn,
				 struct tagged_addr phys, size_t nr,
				 unsigned long flags, int group_id,
				 enum kbase_caller_mmu_sync_info mmu_sync_info);

int kbase_mmu_teardown_pages(struct kbase_device *kbdev, struct kbase_mmu_table *mmut, u64 vpfn,
			     struct tagged_addr *phys, size_t nr, int as_nr);
int kbase_mmu_update_pages(struct kbase_context *kctx, u64 vpfn,
			   struct tagged_addr *phys, size_t nr,
			   unsigned long flags, int const group_id);

/**
 * kbase_mmu_bus_fault_interrupt - Process a bus fault interrupt.
 *
 * Process the bus fault interrupt that was reported for a particular GPU
 * address space.
 *
 * @kbdev:       Pointer to the kbase device for which bus fault was reported.
 * @status:      Value of the GPU_FAULTSTATUS register.
 * @as_nr:       GPU address space for which the bus fault occurred.
 *
 * Return: zero if the operation was successful, non-zero otherwise.
 */
int kbase_mmu_bus_fault_interrupt(struct kbase_device *kbdev, u32 status,
		u32 as_nr);

#endif /* _KBASE_MMU_H_ */
