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

#include <linux/delay.h>
#include <linux/list.h>
#include <linux/migrate.h>
#include <linux/mmzone.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>

#include "sysdep.h"
#include "tzlog.h"
#include "tzdev.h"
#include "tz_cred.h"
#include "tz_mem.h"
#include "tz_iwio.h"

#define TZDEV_MIGRATION_MAX_RETRIES	40

#define TZDEV_PFNS_PER_PAGE		(PAGE_SIZE / sizeof(sk_pfn_t))
#define TZDEV_IWSHMEM_IDS_PER_PAGE	(PAGE_SIZE / sizeof(uint32_t))

#define TZDEV_IWSHMEM_REG_FLAG_WRITE	(1 << 0)
#define TZDEV_IWSHMEM_REG_FLAG_KERNEL	(1 << 1)

static void *tzdev_mem_release_buf;
static DEFINE_IDR(tzdev_mem_map);
static DEFINE_MUTEX(tzdev_mem_mutex);

int isolate_lru_page(struct page *page);

static unsigned long __tzdev_get_user_pages(struct task_struct *task,
		struct mm_struct *mm, unsigned long start, unsigned long nr_pages,
		int write, int force, struct page **pages,
		struct vm_area_struct **vmas)
{
	struct page **cur_pages = pages;
	unsigned long nr_pinned = 0;
	int res;

	while (nr_pinned < nr_pages) {
		res = sysdep_get_user_pages(task, mm, start, nr_pages - nr_pinned, write,
				force, cur_pages, vmas);
		if (res < 0)
			return nr_pinned;

		start += res * PAGE_SIZE;
		nr_pinned += res;
		cur_pages += res;
	}

	return nr_pinned;
}

/* This is the same approach to pinning user memory
 * as used in Infiniband drivers.
 * Refer to drivers/inifiniband/core/umem.c */
int tzdev_get_user_pages(struct task_struct *task, struct mm_struct *mm,
		unsigned long start, unsigned long nr_pages, int write,
		int force, struct page **pages, struct vm_area_struct **vmas)
{
	unsigned long i, locked, nr_pinned;

	locked = nr_pages + mm->pinned_vm;

	nr_pinned = __tzdev_get_user_pages(task, mm, start, nr_pages, write,
						force, pages, vmas);
	if (nr_pinned != nr_pages)
		goto fail;

	mm->pinned_vm = locked;


	return 0;

fail:
	for (i = 0; i < nr_pinned; i++)
		put_page(pages[i]);

	return -EFAULT;
}

void tzdev_put_user_pages(struct page **pages, unsigned long nr_pages)
{
	unsigned long i;

	for (i = 0; i < nr_pages; i++) {
		/* NULL pointers may appear here due to unsuccessful migration */
		if (pages[i])
			put_page(pages[i]);
	}
}

void tzdev_decrease_pinned_vm(struct mm_struct *mm, unsigned long nr_pages)
{
	down_write(&mm->mmap_sem);
	mm->pinned_vm -= nr_pages;
	up_write(&mm->mmap_sem);
}

static void tzdev_mem_free(int id, struct tzdev_mem_reg *mem, unsigned int is_user)
{
	struct task_struct *task;
	struct mm_struct *mm;

	if (!mem->pid) {
		if (!is_user) {
			if (mem->free_func)
				mem->free_func(mem->free_data);
			idr_remove(&tzdev_mem_map, id);
			kfree(mem);
		}

		/* Nothing to do for kernel memory */
		return;
	}

	idr_remove(&tzdev_mem_map, id);

	tzdev_put_user_pages(mem->pages, mem->nr_pages);

	task = get_pid_task(mem->pid, PIDTYPE_PID);
	put_pid(mem->pid);
	if (!task)
		goto out;

	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm)
		goto out;

	tzdev_decrease_pinned_vm(mm, mem->nr_pages);
	mmput(mm);

out:
	kfree(mem->pages);
	kfree(mem);
}

static void tzdev_mem_list_release(unsigned char *buf, unsigned int cnt)
{
	uint32_t *ids;
	unsigned int i;
	struct tzdev_mem_reg *mem;

	ids = (uint32_t *)buf;
	for (i = 0; i < cnt; i++) {
		mem = idr_find(&tzdev_mem_map, ids[i]);
		BUG_ON(!mem);
		tzdev_mem_free(ids[i], mem, 0);
	}
}

static int _tzdev_mem_release(int id, unsigned int is_user)
{
	struct tzdev_mem_reg *mem;
	struct tz_iwio_aux_channel *ch;
	long cnt;
	int ret = 0;

	mutex_lock(&tzdev_mem_mutex);

	mem = idr_find(&tzdev_mem_map, id);
	if (!mem) {
		ret = -ENOENT;
		goto out;
	}

	if (is_user != !!mem->pid) {
		ret = -EPERM;
		goto out;
	}

	mem->in_release = 1;

	ch = tz_iwio_get_aux_channel();
	cnt = tzdev_smc_shmem_list_rls(id);
	if (cnt > 0) {
		BUG_ON(cnt > TZDEV_IWSHMEM_IDS_PER_PAGE);

		memcpy(tzdev_mem_release_buf, ch->buffer, cnt * sizeof(uint32_t));
		tz_iwio_put_aux_channel();

		tzdev_mem_list_release(tzdev_mem_release_buf, cnt);
	} else {
		ret = cnt;
		tz_iwio_put_aux_channel();
	}

	if (ret == -ESHUTDOWN)
		tzdev_mem_free(id, mem, 0);

out:
	mutex_unlock(&tzdev_mem_mutex);

	return ret;
}

static int _tzdev_mem_register(struct tzdev_mem_reg *mem, sk_pfn_t *pfns,
		unsigned long nr_pages, unsigned int is_writable)
{
	int ret, id;
	struct tz_iwio_aux_channel *ch;
	unsigned int pfns_used, pfns_transferred, off, pfns_current;
	ssize_t size;

	ret = tz_format_cred(&mem->cred);
	if (ret) {
		tzdev_print(0, "Failed to calculate shmem credentials, %d\n", ret);
		return ret;
	}

	mutex_lock(&tzdev_mem_mutex);
	ret = sysdep_idr_alloc(&tzdev_mem_map, mem);
	if (ret < 0)
		goto unlock;

	id = ret;
	ch = tz_iwio_get_aux_channel();

	memcpy(ch->buffer, &mem->cred, sizeof(struct tz_cred));

	pfns_used = DIV_ROUND_UP(sizeof(struct tz_cred), sizeof(sk_pfn_t));
	off = pfns_used * sizeof(sk_pfn_t);
	pfns_transferred = 0;

	while (pfns_transferred < nr_pages) {
		pfns_current = min(nr_pages - pfns_transferred,
				TZDEV_PFNS_PER_PAGE - pfns_used);
		size = pfns_current * sizeof(sk_pfn_t);

		memcpy(&ch->buffer[off], &pfns[pfns_transferred], size);
		ret = tzdev_smc_shmem_list_reg(id, nr_pages, is_writable);
		if (ret) {
			tzdev_print(0, "Failed register pfns, %d\n", ret);
			goto put_aux_channel;
		}

		pfns_transferred += pfns_current;

		/* First write to aux channel is done with additional offset,
		 * because first call requires passing credentianls.
		 * For the subsequent calls offset is 0. */
		off = 0;
		pfns_used = 0;
	}

	tz_iwio_put_aux_channel();
	mutex_unlock(&tzdev_mem_mutex);

	return id;

put_aux_channel:
	tz_iwio_put_aux_channel();
	idr_remove(&tzdev_mem_map, id);
unlock:
	mutex_unlock(&tzdev_mem_mutex);

	return ret;
}

#if defined(CONFIG_TZDEV_PAGE_MIGRATION)

static struct page *tzdev_alloc_kernel_page(struct page *page, unsigned long private, int **x)
{
	return alloc_page(GFP_KERNEL);
}

static void tzdev_free_kernel_page(struct page *page, unsigned long private)
{
	__free_page(page);
}

static unsigned long tzdev_get_migratetype(struct page *page)
{
	struct zone *zone;
	unsigned long flags;
	unsigned long migrate_type;

	/* Zone lock must be held to avoid race with
	 * set_pageblock_migratetype() */
	zone = page_zone(page);
	spin_lock_irqsave(&zone->lock, flags);
	migrate_type = get_pageblock_migratetype(page);
	spin_unlock_irqrestore(&zone->lock, flags);

	return migrate_type;
}

static void tzdev_verify_migration_page(struct page *page)
{
	unsigned long migrate_type;

	migrate_type = tzdev_get_migratetype(page);
	if (migrate_type == MIGRATE_CMA || migrate_type == MIGRATE_ISOLATE)
		tzdev_print(0, "%s: migrate_type == %lu\n", __func__, migrate_type);
}

static void tzdev_verify_migration(struct page **pages, unsigned long nr_pages)
{
	unsigned long i;

	for (i = 0; i < nr_pages; i++)
		tzdev_verify_migration_page(pages[i]);
}

static int __tzdev_migrate_pages(struct task_struct *task, struct mm_struct *mm,
		unsigned long start, unsigned long nr_pages, int write,
		int force, struct page **pages, unsigned long *verified_bitmap)
{
	unsigned long i = 0, migrate_nr = 0, nr_pin = 0;
	unsigned long cur_pages_index, cur_start, pinned, migrate_type;
	int res;
	struct page **cur_pages;
	LIST_HEAD(pages_list);
	int ret = 0;

	/* Add migrating pages to the list */
	while ((i = find_next_zero_bit(verified_bitmap, nr_pages, i)) < nr_pages) {
		migrate_type = tzdev_get_migratetype(pages[i]);
		/* Skip pages that is currently isolated by somebody.
		 * Isolated page may originally have MIGRATE_CMA type,
		 * so caller should repeat migration for such pages */
		if (migrate_type == MIGRATE_ISOLATE) {
			tzdev_print(0, "%s: migrate_type is MIGRATE_ISOLATE\n", __func__);
			ret = -EAGAIN;
			i++;
			continue;
		}

		/* Mark non-CMA pages as verified and skip them */
		if (migrate_type != MIGRATE_CMA) {
			bitmap_set(verified_bitmap, i, 1);
			i++;
			continue;
		}

		/* Call migrate_prep() once if migration necessary */
		if (migrate_nr == 0)
			migrate_prep();

		/* Pages should be isolated from an LRU list before migration.
		 * If isolation failed skip this page and inform caller to
		 * repeat migrate operation */
		res = isolate_lru_page(pages[i]);
		if (res < 0) {
			tzdev_print(0, "%s: isolate_lru_page() failed, res=%d\n", __func__, res);
			ret = -EAGAIN;
			i++;
			continue;
		}

		list_add_tail(&pages[i]->lru, &pages_list);
		put_page(pages[i]);
		/* pages array will be refilled with migrated pages later */
		pages[i] = NULL;
		migrate_nr++;
		i++;
	}

	if (!migrate_nr)
		return ret;

	/* make migration */
	res = sysdep_migrate_pages(&pages_list, tzdev_alloc_kernel_page, tzdev_free_kernel_page);
	if (res) {
		sysdep_putback_isolated_pages(&pages_list);
		return -EFAULT;
	}

	/* pin migrated pages */
	i = 0;
	do {
		nr_pin = 0;

		/* find index of the next migrated page */
		while (i < nr_pages && pages[i])
			i++;

		cur_pages = &pages[i];
		cur_pages_index = i;
		cur_start = start + i * PAGE_SIZE;

		/* find continuous migrated pages range */
		while (i < nr_pages && !pages[i]) {
			nr_pin++;
			i++;
		}

		/* and pin it */
		pinned = __tzdev_get_user_pages(task, mm, cur_start, nr_pin,
						write, force, cur_pages, NULL);
		if (pinned != nr_pin)
			return -EFAULT;

		/* Check that migrated pages are not MIGRATE_CMA or MIGRATE_ISOLATE */
		tzdev_verify_migration(cur_pages, nr_pin);
		bitmap_set(verified_bitmap, cur_pages_index, nr_pin);

		migrate_nr -= nr_pin;
	} while (migrate_nr);

	return ret;
}

int tzdev_migrate_pages(struct task_struct *task, struct mm_struct *mm,
		unsigned long start, unsigned long nr_pages, int write,
		int force, struct page **pages)
{
	int ret;
	unsigned int retries = 0;
	unsigned long *verified_bitmap;
	size_t bitmap_size = DIV_ROUND_UP(nr_pages, BITS_PER_LONG);

	verified_bitmap = kcalloc(bitmap_size, sizeof(unsigned long), GFP_KERNEL);
	if (!verified_bitmap)
		return -ENOMEM;

	do {
		ret = __tzdev_migrate_pages(task, mm, start, nr_pages, write,
				force, pages, verified_bitmap);

		if (ret != -EAGAIN || (retries++ >= TZDEV_MIGRATION_MAX_RETRIES))
			break;
		msleep(1);
	} while (1);

	kfree(verified_bitmap);

	return ret;
}
#endif /* CONFIG_TZDEV_PAGE_MIGRATION */

int tzdev_mem_init(void)
{
	struct page *page;

	page = alloc_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	tzdev_mem_release_buf = page_address(page);

	tzdev_print(0, "AUX channels mem release buffer allocated\n");

	return 0;
}

void tzdev_mem_fini(void)
{
	struct tzdev_mem_reg *mem;
	unsigned int id;

	mutex_lock(&tzdev_mem_mutex);
	idr_for_each_entry(&tzdev_mem_map, mem, id)
		tzdev_mem_free(id, mem, 0);
	mutex_unlock(&tzdev_mem_mutex);

	__free_page(virt_to_page(tzdev_mem_release_buf));
}

int tzdev_mem_register_user(void *ptr, unsigned long size, unsigned int write)
{
	struct task_struct *task;
	struct mm_struct *mm;
	struct page **pages;
	struct tzdev_mem_reg *mem;
	sk_pfn_t *pfns;
	unsigned long start, end;
	unsigned long nr_pages = 0;
	int ret, res, i, id;
	unsigned int flags = 0;

	if (!size)
		return -EINVAL;

	if (!access_ok(write ? VERIFY_WRITE : VERIFY_READ, ptr, size))
		return -EFAULT;

	start = (unsigned long)ptr >> PAGE_SHIFT;
	end = ((unsigned long)ptr + size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	nr_pages = end - start;

	if (write)
		flags |= TZDEV_IWSHMEM_REG_FLAG_WRITE;

	task = current;
	mm = get_task_mm(task);
	if (!mm)
		return -ESRCH;

	pages = kcalloc(nr_pages, sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		ret = -ENOMEM;
		goto out_mm;
	}

	pfns = kmalloc(nr_pages * sizeof(sk_pfn_t), GFP_KERNEL);
	if (!pfns) {
		ret = -ENOMEM;
		goto out_pages;
	}

	mem = kmalloc(sizeof(struct tzdev_mem_reg), GFP_KERNEL);
	if (!mem) {
		ret = -ENOMEM;
		goto out_pfns;
	}

	mem->pid = get_task_pid(task, PIDTYPE_PID);
	mem->nr_pages = nr_pages;
	mem->pages = pages;
	mem->free_func = NULL;
	mem->free_data = NULL;
	mem->in_release = 0;

	/*
	 * Holding 'mm->mmap_sem' is required to synchronize users who tries to register same pages simultaneously.
	 * Without synchronization both users would hold page refcount and so preventing migration.
	 */
	down_write(&mm->mmap_sem);
	res = tzdev_get_user_pages(task, mm, (unsigned long)ptr,
			nr_pages, 1, !write, pages, NULL);
	if (res) {
		up_write(&mm->mmap_sem);
		tzdev_print(0, "Failed to pin user pages (%d)\n", res);
		ret = res;
		goto out_mem;
	}

#if defined(CONFIG_TZDEV_PAGE_MIGRATION)
	/*
	 * In case of enabled migration it is possible that userspace pages
	 * will be migrated from current physical page to some other
	 * To avoid fails of CMA migrations we have to move pages to other
	 * region which can not be inside any CMA region. This is done by
	 * allocations with GFP_KERNEL flag to point UNMOVABLE memblock
	 * to be used for such allocations.
	 */
	res = tzdev_migrate_pages(task, mm, (unsigned long)ptr, nr_pages,
			1, !write, pages);
	if (res < 0) {
		up_write(&mm->mmap_sem);
		tzdev_print(0, "Failed to migrate CMA pages (%d)\n", res);
		ret = res;
		goto out_pin;
	}
#endif /* CONFIG_TZDEV_PAGE_MIGRATION */
	up_write(&mm->mmap_sem);
	for (i = 0; i < nr_pages; i++)
		pfns[i] = page_to_pfn(pages[i]);

	id = _tzdev_mem_register(mem, pfns, nr_pages, flags);
	if (id < 0) {
		ret = id;
		goto out_pin;
	}

	kfree(pfns);

	mmput(mm);

	return id;

out_pin:
	tzdev_put_user_pages(pages, nr_pages);
	tzdev_decrease_pinned_vm(mm, nr_pages);
out_mem:
	kfree(mem);
out_pfns:
	kfree(pfns);
out_pages:
	kfree(pages);
out_mm:
	mmput(mm);

	return ret;
}

int tzdev_mem_register(void *ptr, unsigned long size, unsigned int write,
		tzdev_mem_free_func_t free_func, void *free_data)
{
	struct tzdev_mem_reg *mem;
	struct page *page;
	sk_pfn_t *pfns;
	unsigned long addr, start, end;
	unsigned long i, nr_pages = 0;
	int ret, id;
	unsigned int flags = TZDEV_IWSHMEM_REG_FLAG_KERNEL;

	if (!size)
		return -EINVAL;

	addr = (unsigned long)ptr;

	BUG_ON(addr + size <= addr || !IS_ALIGNED(addr | size, PAGE_SIZE));

	start = addr >> PAGE_SHIFT;
	end = (addr + size) >> PAGE_SHIFT;
	nr_pages = end - start;

	if (write)
		flags |= TZDEV_IWSHMEM_REG_FLAG_WRITE;

	pfns = kmalloc(nr_pages * sizeof(sk_pfn_t), GFP_KERNEL);
	if (!pfns)
		return -ENOMEM;

	mem = kmalloc(sizeof(struct tzdev_mem_reg), GFP_KERNEL);
	if (!mem) {
		ret = -ENOMEM;
		goto out_pfns;
	}

	mem->pid = NULL;
	mem->free_func = free_func;
	mem->free_data = free_data;
	mem->in_release = 0;

	for (i = 0; i < nr_pages; i++) {
		page = is_vmalloc_addr(ptr + PAGE_SIZE * i)
				? vmalloc_to_page(ptr + PAGE_SIZE * i)
				: virt_to_page(addr + PAGE_SIZE * i);

		pfns[i] = page_to_pfn(page);
	}

	id = _tzdev_mem_register(mem, pfns, nr_pages, flags);
	if (id < 0) {
		ret = id;
		goto out_mem;
	}

	kfree(pfns);

	return id;

out_mem:
	kfree(mem);
out_pfns:
	kfree(pfns);

	return ret;
}

int tzdev_mem_release_user(unsigned int id)
{
	return _tzdev_mem_release(id, 1);
}

int tzdev_mem_release(unsigned int id)
{
	return _tzdev_mem_release(id, 0);
}

void tzdev_mem_release_panic_handler(void)
{
	struct tzdev_mem_reg *mem;
	unsigned int id;

	mutex_lock(&tzdev_mem_mutex);
	idr_for_each_entry(&tzdev_mem_map, mem, id)
		if (mem->in_release)
			tzdev_mem_free(id, mem, 0);
	mutex_unlock(&tzdev_mem_mutex);
}
