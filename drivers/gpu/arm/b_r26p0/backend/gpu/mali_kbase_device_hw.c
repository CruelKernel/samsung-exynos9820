/*
 *
 * (C) COPYRIGHT 2014-2016, 2018-2020 ARM Limited. All rights reserved.
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


/*
 *
 */
#include <mali_kbase.h>
#include <gpu/mali_kbase_gpu_fault.h>
#include <backend/gpu/mali_kbase_instr_internal.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_device_internal.h>
#include <mali_kbase_reset_gpu.h>
#include <mmu/mali_kbase_mmu.h>

#if !defined(CONFIG_MALI_NO_MALI)

#ifdef CONFIG_DEBUG_FS

int kbase_io_history_resize(struct kbase_io_history *h, u16 new_size)
{
	struct kbase_io_access *old_buf;
	struct kbase_io_access *new_buf;
	unsigned long flags;

	if (!new_size)
		goto out_err; /* The new size must not be 0 */

	new_buf = vmalloc(new_size * sizeof(*h->buf));
	if (!new_buf)
		goto out_err;

	spin_lock_irqsave(&h->lock, flags);

	old_buf = h->buf;

	/* Note: we won't bother with copying the old data over. The dumping
	 * logic wouldn't work properly as it relies on 'count' both as a
	 * counter and as an index to the buffer which would have changed with
	 * the new array. This is a corner case that we don't need to support.
	 */
	h->count = 0;
	h->size = new_size;
	h->buf = new_buf;

	spin_unlock_irqrestore(&h->lock, flags);

	vfree(old_buf);

	return 0;

out_err:
	return -1;
}


int kbase_io_history_init(struct kbase_io_history *h, u16 n)
{
	h->enabled = false;
	spin_lock_init(&h->lock);
	h->count = 0;
	h->size = 0;
	h->buf = NULL;
	if (kbase_io_history_resize(h, n))
		return -1;

	return 0;
}


void kbase_io_history_term(struct kbase_io_history *h)
{
	vfree(h->buf);
	h->buf = NULL;
}


/* kbase_io_history_add - add new entry to the register access history
 *
 * @h: Pointer to the history data structure
 * @addr: Register address
 * @value: The value that is either read from or written to the register
 * @write: 1 if it's a register write, 0 if it's a read
 */
static void kbase_io_history_add(struct kbase_io_history *h,
		void __iomem const *addr, u32 value, u8 write)
{
	struct kbase_io_access *io;
	unsigned long flags;

	spin_lock_irqsave(&h->lock, flags);

	io = &h->buf[h->count % h->size];
	io->addr = (uintptr_t)addr | write;
	io->value = value;
	++h->count;
	/* If count overflows, move the index by the buffer size so the entire
	 * buffer will still be dumped later */
	if (unlikely(!h->count))
		h->count = h->size;

	spin_unlock_irqrestore(&h->lock, flags);
}


void kbase_io_history_dump(struct kbase_device *kbdev)
{
	struct kbase_io_history *const h = &kbdev->io_history;
	u16 i;
	size_t iters;
	unsigned long flags;

	if (!unlikely(h->enabled))
		return;

	spin_lock_irqsave(&h->lock, flags);

	dev_err(kbdev->dev, "Register IO History:");
	iters = (h->size > h->count) ? h->count : h->size;
	dev_err(kbdev->dev, "Last %zu register accesses of %zu total:\n", iters,
			h->count);
	for (i = 0; i < iters; ++i) {
		struct kbase_io_access *io =
			&h->buf[(h->count - iters + i) % h->size];
		char const access = (io->addr & 1) ? 'w' : 'r';

		dev_err(kbdev->dev, "%6i: %c: reg 0x%016lx val %08x\n", i,
			access, (unsigned long)(io->addr & ~0x1), io->value);
	}

	spin_unlock_irqrestore(&h->lock, flags);
}


#endif /* CONFIG_DEBUG_FS */


void kbase_reg_write(struct kbase_device *kbdev, u32 offset, u32 value)
{
	KBASE_DEBUG_ASSERT(kbdev->pm.backend.gpu_powered);
	KBASE_DEBUG_ASSERT(kbdev->dev != NULL);

	writel(value, kbdev->reg + offset);

#ifdef CONFIG_DEBUG_FS
	if (unlikely(kbdev->io_history.enabled))
		kbase_io_history_add(&kbdev->io_history, kbdev->reg + offset,
				value, 1);
#endif /* CONFIG_DEBUG_FS */
	dev_dbg(kbdev->dev, "w: reg %08x val %08x", offset, value);
}

KBASE_EXPORT_TEST_API(kbase_reg_write);

u32 kbase_reg_read(struct kbase_device *kbdev, u32 offset)
{
	u32 val;
	KBASE_DEBUG_ASSERT(kbdev->pm.backend.gpu_powered);
	KBASE_DEBUG_ASSERT(kbdev->dev != NULL);

	val = readl(kbdev->reg + offset);

#ifdef CONFIG_DEBUG_FS
	if (unlikely(kbdev->io_history.enabled))
		kbase_io_history_add(&kbdev->io_history, kbdev->reg + offset,
				val, 0);
#endif /* CONFIG_DEBUG_FS */
	dev_dbg(kbdev->dev, "r: reg %08x val %08x", offset, val);

	return val;
}

KBASE_EXPORT_TEST_API(kbase_reg_read);

bool kbase_is_gpu_lost(struct kbase_device *kbdev)
{
	u32 val;

	val = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_ID));

	return val == 0;
}
#endif /* !defined(CONFIG_MALI_NO_MALI) */

/**
 * kbase_report_gpu_fault - Report a GPU fault.
 * @kbdev:    Kbase device pointer
 * @multiple: Zero if only GPU_FAULT was raised, non-zero if MULTIPLE_GPU_FAULTS
 *            was also set
 *
 * This function is called from the interrupt handler when a GPU fault occurs.
 * It reports the details of the fault using dev_warn().
 */
static void kbase_report_gpu_fault(struct kbase_device *kbdev, int multiple)
{
	u32 status = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_FAULTSTATUS));
	u64 address = (u64) kbase_reg_read(kbdev,
			GPU_CONTROL_REG(GPU_FAULTADDRESS_HI)) << 32;

	address |= kbase_reg_read(kbdev,
			GPU_CONTROL_REG(GPU_FAULTADDRESS_LO));

	/* MALI_SEC_INTEGRATION */
	if (kbdev->vendor_callbacks->update_status)
		kbdev->vendor_callbacks->update_status(kbdev, "completion_code", status);

	dev_warn(kbdev->dev, "GPU Fault 0x%08x (%s) at 0x%016llx",
		status,
		kbase_gpu_exception_name(status & 0xFF),
		address);
	if (multiple)
		dev_warn(kbdev->dev, "There were multiple GPU faults - some have not been reported\n");
}

static bool kbase_gpu_fault_interrupt(struct kbase_device *kbdev, int multiple)
{
	kbase_report_gpu_fault(kbdev, multiple);
	return false;
}

void kbase_gpu_start_cache_clean_nolock(struct kbase_device *kbdev)
{
	u32 irq_mask;

	lockdep_assert_held(&kbdev->hwaccess_lock);

	if (kbdev->cache_clean_in_progress) {
		/* If this is called while another clean is in progress, we
		 * can't rely on the current one to flush any new changes in
		 * the cache. Instead, trigger another cache clean immediately
		 * after this one finishes.
		 */
		kbdev->cache_clean_queued = true;
		return;
	}

	/* Enable interrupt */
	irq_mask = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK));
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK),
				irq_mask | CLEAN_CACHES_COMPLETED);

	KBASE_KTRACE_ADD(kbdev, CORE_GPU_CLEAN_INV_CACHES, NULL, 0);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
					GPU_COMMAND_CLEAN_INV_CACHES);

	kbdev->cache_clean_in_progress = true;
}

void kbase_gpu_start_cache_clean(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_gpu_start_cache_clean_nolock(kbdev);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void kbase_gpu_cache_clean_wait_complete(struct kbase_device *kbdev)
{
	lockdep_assert_held(&kbdev->hwaccess_lock);

	kbdev->cache_clean_queued = false;
	kbdev->cache_clean_in_progress = false;
	wake_up(&kbdev->cache_clean_wait);
}

static void kbase_clean_caches_done(struct kbase_device *kbdev)
{
	u32 irq_mask;
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);

	if (kbdev->cache_clean_queued) {
		kbdev->cache_clean_queued = false;

		KBASE_KTRACE_ADD(kbdev, CORE_GPU_CLEAN_INV_CACHES, NULL, 0);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
				GPU_COMMAND_CLEAN_INV_CACHES);
	} else {
		/* Disable interrupt */
		irq_mask = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK));
		kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK),
				irq_mask & ~CLEAN_CACHES_COMPLETED);

		kbase_gpu_cache_clean_wait_complete(kbdev);
	}

	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

static inline bool get_cache_clean_flag(struct kbase_device *kbdev)
{
	bool cache_clean_in_progress;
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	cache_clean_in_progress = kbdev->cache_clean_in_progress;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return cache_clean_in_progress;
}

void kbase_gpu_wait_cache_clean(struct kbase_device *kbdev)
{
	while (get_cache_clean_flag(kbdev)) {
		wait_event_interruptible(kbdev->cache_clean_wait,
				!kbdev->cache_clean_in_progress);
	}
}

int kbase_gpu_wait_cache_clean_timeout(struct kbase_device *kbdev,
				unsigned int wait_timeout_ms)
{
	long remaining = msecs_to_jiffies(wait_timeout_ms);

	while (remaining && get_cache_clean_flag(kbdev)) {
		remaining = wait_event_timeout(kbdev->cache_clean_wait,
					!kbdev->cache_clean_in_progress,
					remaining);
	}

	return (remaining ? 0 : -ETIMEDOUT);
}

void kbase_gpu_interrupt(struct kbase_device *kbdev, u32 val)
{
	bool clear_gpu_fault = false;

	KBASE_KTRACE_ADD(kbdev, CORE_GPU_IRQ, NULL, val);
	if (val & GPU_FAULT)
		clear_gpu_fault = kbase_gpu_fault_interrupt(kbdev,
					val & MULTIPLE_GPU_FAULTS);

	if (val & RESET_COMPLETED)
		kbase_pm_reset_done(kbdev);

	if (val & PRFCNT_SAMPLE_COMPLETED)
		kbase_instr_hwcnt_sample_done(kbdev);

	KBASE_KTRACE_ADD(kbdev, CORE_GPU_IRQ_CLEAR, NULL, val);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), val);

	/* kbase_pm_check_transitions (called by kbase_pm_power_changed) must
	 * be called after the IRQ has been cleared. This is because it might
	 * trigger further power transitions and we don't want to miss the
	 * interrupt raised to notify us that these further transitions have
	 * finished. The same applies to kbase_clean_caches_done() - if another
	 * clean was queued, it might trigger another clean, which might
	 * generate another interrupt which shouldn't be missed.
	 */

	if (val & CLEAN_CACHES_COMPLETED)
		kbase_clean_caches_done(kbdev);

	if (val & POWER_CHANGED_ALL) {
		kbase_pm_power_changed(kbdev);
	} else if (val & CLEAN_CACHES_COMPLETED) {
		/* If cache line evict messages can be lost when shader cores
		 * power down then we need to flush the L2 cache before powering
		 * down cores. When the flush completes, the shaders' state
		 * machine needs to be re-invoked to proceed with powering down
		 * cores.
		 */
		if (kbdev->pm.backend.l2_always_on ||
				kbase_hw_has_issue(kbdev, BASE_HW_ISSUE_TTRX_921))
			kbase_pm_power_changed(kbdev);
	}


	KBASE_KTRACE_ADD(kbdev, CORE_GPU_IRQ_DONE, NULL, val);
}

static int busy_wait_on_irq(struct kbase_device *kbdev, u32 irq_bit)
{
	char *irq_flag_name;
	/* Previously MMU-AS command was used for L2 cache flush on page-table update.
	 * And we're using the same max-loops count for GPU command, because amount of
	 * L2 cache flush overhead are same between them.
	 */
	unsigned int max_loops = KBASE_AS_INACTIVE_MAX_LOOPS;

	/* Wait for the GPU cache clean operation to complete */
	while (--max_loops &&
	       !(kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_RAWSTAT)) & irq_bit)) {
		;
	}

	/* reset gpu if time-out occurred */
	if (max_loops == 0) {
		switch (irq_bit) {
		case CLEAN_CACHES_COMPLETED:
			irq_flag_name = "CLEAN_CACHES_COMPLETED";
			break;
		case FLUSH_PA_RANGE_COMPLETED:
			irq_flag_name = "FLUSH_PA_RANGE_COMPLETED";
			break;
		default:
			irq_flag_name = "UNKNOWN";
			break;
		}

		dev_err(kbdev->dev,
			"Stuck waiting on %s bit, might be caused by slow/unstable GPU clock or possible faulty FPGA connector\n",
			irq_flag_name);

		if (kbase_prepare_to_reset_gpu_locked(kbdev))
			kbase_reset_gpu_locked(kbdev);
		return -EBUSY;
	}

	/* Clear the interrupt bit. */
	KBASE_KTRACE_ADD(kbdev, CORE_GPU_IRQ_CLEAR, NULL, irq_bit);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR), irq_bit);

	return 0;
}

int kbase_gpu_cache_flush_and_busy_wait(struct kbase_device *kbdev,
					u32 flush_op)
{
	int need_to_wake_up = 0;
	int ret = 0;

	/* hwaccess_lock must be held to avoid any sync issue with
	 * kbase_gpu_start_cache_clean() / kbase_clean_caches_done()
	 */
	lockdep_assert_held(&kbdev->hwaccess_lock);

	/* 1. Check if kbdev->cache_clean_in_progress is set.
	 *    If it is set, it means there are threads waiting for
	 *    CLEAN_CACHES_COMPLETED irq to be raised and that the
	 *    corresponding irq mask bit is set.
	 *    We'll clear the irq mask bit and busy-wait for the cache
	 *    clean operation to complete before submitting the cache
	 *    clean command required after the GPU page table update.
	 *    Pended flush commands will be merged to requested command.
	 */
	if (kbdev->cache_clean_in_progress) {
		/* disable irq first */
		u32 irq_mask = kbase_reg_read(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK));
		kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_MASK),
				irq_mask & ~CLEAN_CACHES_COMPLETED);

		/* busy wait irq status to be enabled */
		ret = busy_wait_on_irq(kbdev, (u32)CLEAN_CACHES_COMPLETED);
		if (ret)
			return ret;

		/* merge pended command if there's any */
		flush_op = GPU_COMMAND_FLUSH_CACHE_MERGE(
			kbdev->cache_clean_queued, flush_op);

		/* enable wake up notify flag */
		need_to_wake_up = 1;
	} else {
		/* Clear the interrupt CLEAN_CACHES_COMPLETED bit. */
		kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_IRQ_CLEAR),
				CLEAN_CACHES_COMPLETED);
	}

	/* 2. Issue GPU_CONTROL.COMMAND.FLUSH_CACHE operation. */
	KBASE_KTRACE_ADD(kbdev, CORE_GPU_CLEAN_INV_CACHES, NULL, flush_op);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), flush_op);

	/* 3. Busy-wait irq status to be enabled. */
	ret = busy_wait_on_irq(kbdev, (u32)CLEAN_CACHES_COMPLETED);
	if (ret)
		return ret;

	/* 4. Wake-up blocked threads when there is any. */
	if (need_to_wake_up)
		kbase_gpu_cache_clean_wait_complete(kbdev);

	return ret;
}
