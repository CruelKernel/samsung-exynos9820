/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SYSDEP_H__
#define __SYSDEP_H__

#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/cpumask.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/migrate.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/version.h>

#include <asm/barrier.h>
#include <asm/cacheflush.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
#include <uapi/linux/sched/types.h>
#include <linux/wait.h>

#ifndef CPU_UP_CANCELED
#define CPU_UP_CANCELED		0x0004
#endif

typedef struct wait_queue_entry wait_queue_t;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/sched/task.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
#include <crypto/hash.h>
#endif

/* For MAX_RT_PRIO definitions */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 15, 0)
#include <linux/sched/prio.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
#include <linux/sched/rt.h>
#else
#include <linux/sched.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
#include <linux/uidgid.h>
#else
typedef uid_t kuid_t;
typedef gid_t kgid_t;

static inline uid_t __kuid_val(kuid_t uid)
{
	return uid;
}

static inline gid_t __kgid_val(kgid_t gid)
{
	return gid;
}
#endif

#if defined(CONFIG_ARM64)
#define outer_inv_range(s, e)
#else
#define __flush_dcache_area(s, e)	__cpuc_flush_dcache_area(s, e)
#endif

#if defined(CONFIG_TZDEV_PAGE_MIGRATION)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
#define sysdep_migrate_pages(list, alloc, free)		migrate_pages((list), (alloc), (free), 0, MIGRATE_SYNC, MR_MEMORY_FAILURE)
#define sysdep_putback_isolated_pages(list)		putback_movable_pages(list)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
#define sysdep_migrate_pages(list, alloc, free)		({(void)free; migrate_pages((list), (alloc), 0, MIGRATE_SYNC, MR_MEMORY_FAILURE);})
#define sysdep_putback_isolated_pages(list)		putback_lru_pages(list)
#else
#define sysdep_migrate_pages(list, alloc, free)		({(void)free; migrate_pages((list), (alloc), 0, false, MIGRATE_SYNC);})
#define sysdep_putback_isolated_pages(list)		putback_lru_pages(list)
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
#define sysdep_kfifo_put(fifo, val) kfifo_put(fifo, val)
#else
#define sysdep_kfifo_put(fifo, val) kfifo_put(fifo, &val)
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 13, 0)
#define U8_MAX		((u8)~0U)
#define S8_MAX		((s8)(U8_MAX>>1))
#define S8_MIN		((s8)(-S8_MAX - 1))
#define U16_MAX		((u16)~0U)
#define S16_MAX		((s16)(U16_MAX>>1))
#define S16_MIN		((s16)(-S16_MAX - 1))
#define U32_MAX		((u32)~0U)
#define S32_MAX		((s32)(U32_MAX>>1))
#define S32_MIN		((s32)(-S32_MAX - 1))
#define U64_MAX		((u64)~0ULL)
#define S64_MAX		((s64)(U64_MAX>>1))
#define S64_MIN		((s64)(-S64_MAX - 1))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
#define idr_for_each_entry(idp, entry, id) \
		for (id = 0; ((entry) = idr_get_next(idp, &(id))) != NULL; ++id)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define for_each_cpu_mask(cpu, mask)	for_each_cpu((cpu), &(mask))
#define cpu_isset(cpu, mask)		cpumask_test_cpu((cpu), &(mask))
#else
#define cpumask_pr_args(maskp)		nr_cpu_ids, cpumask_bits(maskp)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0)
#if !defined(CONFIG_OF)
static inline unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
{
	(void)dev;
	(void)index;

	return 0;
}
#endif /*!defined(CONFIG_OF) */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
/*
 * atomic_dec_if_positive - decrement by 1 if old value positive
 * @v: pointer of type atomic_t
 *
 * The function returns the old value of *v minus 1, even if
 * the atomic variable, v, was not decremented.
 */
static inline int atomic_dec_if_positive(atomic_t *v)
{
	int c, old, dec;
	c = atomic_read(v);
	for (;;) {
		dec = c - 1;
		if (unlikely(dec < 0))
			break;
		old = atomic_cmpxchg((v), c, dec);
		if (likely(old == c))
			break;
		c = old;
	}
	return dec;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
static inline void reinit_completion(struct completion *x)
{
	x->done = 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
#define set_mb(var, value)		\
	do {				\
		var = value;		\
		smp_mb();		\
	} while (0)
#endif

int sysdep_idr_alloc(struct idr *idr, void *mem);

long sysdep_get_user_pages(struct task_struct *task,
		struct mm_struct *mm, unsigned long start, unsigned long nr_pages,
		int write, int force, struct page **pages,
		struct vm_area_struct **vmas);
void sysdep_register_cpu_notifier(struct notifier_block* notifier,
		int (*startup)(unsigned int cpu),
		int (*teardown)(unsigned int cpu));
void sysdep_unregister_cpu_notifier(struct notifier_block* notifier);
int sysdep_crypto_sha1(uint8_t* hash, struct scatterlist* sg, char *p, int len);
int sysdep_vfs_getattr(struct file *filp, struct kstat *stat);

#endif /* __SYSDEP_H__ */
