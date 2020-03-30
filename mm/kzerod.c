// SPDX-License-Identifier: GPL-2.0
/*
 * linux/mm/kzerod.c
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 */
#include <uapi/linux/sched/types.h>
#include <linux/suspend.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/magic.h>
#include <linux/mount.h>
#include <linux/migrate.h>

static bool kzerod_enabled = true;

#define K(x) ((x) << (PAGE_SHIFT-10))
#define GB_TO_PAGES(x) ((x) << (30 - PAGE_SHIFT))
#define MB_TO_PAGES(x) ((x) << (20 - PAGE_SHIFT))

static struct task_struct *task_kzerod;
DECLARE_WAIT_QUEUE_HEAD(kzerod_wait);

enum kzerod_enum_state {
	KZEROD_RUNNING = 0,
	KZEROD_SLEEP_DONE,
	KZEROD_SLEEP_NOMEM,
	KZEROD_SLEEP_DISABLED,
};
static enum kzerod_enum_state kzerod_state = KZEROD_SLEEP_DONE;

static LIST_HEAD(prezeroed_list);
static spinlock_t prezeroed_lock;
static unsigned long nr_prezeroed;
static unsigned long kzerod_wmark_high;
static unsigned long kzerod_wmark_low;

static int kzerod_app_launch_notifier(struct notifier_block *nb,
					 unsigned long action, void *data)
{
	static bool app_launch = false;
	bool prev_launch;
	static unsigned long prev_total = 0;
	static unsigned long prev_prezero = 0;
	static unsigned long prev_jiffies = 0;
	unsigned long cur_total, cur_prezero;

	prev_launch = app_launch;
	app_launch = action ? true : false;

	if (!prev_launch && app_launch) {
		prev_total = global_node_page_state(ZERO_PAGE_ALLOC_TOTAL);
		prev_prezero = global_node_page_state(ZERO_PAGE_ALLOC_PREZERO);
		prev_jiffies = jiffies;
		trace_printk("kzerod: %s %d\n", current->comm, current->pid);
	} else if (prev_launch && !app_launch) {
		cur_total = global_node_page_state(ZERO_PAGE_ALLOC_TOTAL);
		cur_prezero = global_node_page_state(ZERO_PAGE_ALLOC_PREZERO);
		trace_printk("kzerod: launch finished used zero %luKB/%luKB %ums\n",
			     K(cur_prezero - prev_prezero),
			     K(cur_total - prev_total),
			     jiffies_to_msecs(jiffies - prev_jiffies));
		pr_info("kzerod: launch finished used zero %luKB/%luKB %ums\n",
			     K(cur_prezero - prev_prezero),
			     K(cur_total - prev_total),
			     jiffies_to_msecs(jiffies - prev_jiffies));
	}

	return 0;
}

static struct notifier_block kzerod_app_launch_nb = {
	.notifier_call = kzerod_app_launch_notifier,
};

unsigned long kzerod_get_zeroed_size(void)
{
	return nr_prezeroed;
}

static unsigned long kzerod_totalram[] = {
	GB_TO_PAGES(4),
};
static unsigned long kzerod_wmark[] = {
	MB_TO_PAGES(50),
};
#define KZEROD_MAX_WMARK	MB_TO_PAGES(50)

static inline void kzerod_update_wmark(void)
{
	static unsigned long kzerod_totalram_pages = 0;
	int i, array_size;

	if (!kzerod_totalram_pages || kzerod_totalram_pages != totalram_pages) {
		kzerod_totalram_pages = totalram_pages;

		array_size = ARRAY_SIZE(kzerod_totalram);
		for (i = 0; i < array_size; i++) {
			if (totalram_pages <= kzerod_totalram[i]) {
				kzerod_wmark_high = kzerod_wmark[i];
				break;
			}
		}
		if (i == array_size)
			kzerod_wmark_high = KZEROD_MAX_WMARK;
		kzerod_wmark_low = kzerod_wmark_high >> 1;
	}
}

static inline bool kzerod_wmark_high_ok(void)
{
	return nr_prezeroed >= kzerod_wmark_high;
}

static inline bool kzerod_wmark_low_ok(void)
{
	return nr_prezeroed >= kzerod_wmark_low;
}

static struct vfsmount *kzerod_mnt;
static struct inode *kzerod_inode;

/* should be called with page lock */
static inline void set_kzerod_page(struct page *page)
{
	__SetPageMovable(page, kzerod_inode->i_mapping);
}

/* should be called with page lock */
static inline void unset_kzerod_page(struct page *page)
{
	__ClearPageMovable(page);
}

struct page *alloc_zeroed_page(void)
{
	struct page *page = NULL;

	if (!kzerod_enabled)
		return NULL;

	if (unlikely(!spin_trylock(&prezeroed_lock)))
		return NULL;
	if (!list_empty(&prezeroed_list)) {
		page = list_first_entry(&prezeroed_list, struct page, lru);
		if (trylock_page(page)) {
			list_del(&page->lru);
			unset_kzerod_page(page);
			/* The page will be served soon. Let's clean it up */
			page->mapping = NULL;
			unlock_page(page);
			nr_prezeroed--;
		} else {
			page = NULL;
		}
	}
	spin_unlock(&prezeroed_lock);

	if (!kzerod_wmark_low_ok() && (kzerod_state != KZEROD_RUNNING)) {
		trace_printk("kzerod: %d to %d\n", kzerod_state,
			     KZEROD_RUNNING);
		kzerod_state = KZEROD_RUNNING,
		wake_up(&kzerod_wait);
	}

	/*
	 * putback to prezereoed list and return NULL
	 * if page is being touched by migration context
	 */
	if (page && page_count(page) != 1) {
		lock_page(page);
		spin_lock(&prezeroed_lock);
		set_kzerod_page(page);
		list_add_tail(&page->lru, &prezeroed_list);
		nr_prezeroed++;
		spin_unlock(&prezeroed_lock);
		unlock_page(page);
		page = NULL;
	}

	return page;
}

static void drain_zeroed_page(void)
{
	struct page *page, *next;
	unsigned long prev_zero;

	prev_zero = nr_prezeroed;
restart:
	spin_lock(&prezeroed_lock);
	list_for_each_entry_safe(page, next, &prezeroed_list, lru) {
		if (trylock_page(page)) {
			list_del(&page->lru);
			unset_kzerod_page(page);
			unlock_page(page);
			__free_pages(page, 0);
			nr_prezeroed--;
		} else {
			spin_unlock(&prezeroed_lock);
			goto restart;
		}
	}
	spin_unlock(&prezeroed_lock);

	trace_printk("kzerod: drained %luKB\n", K(prev_zero));
}

/* page is already locked before this function is called */
bool kzerod_page_isolate(struct page *page, isolate_mode_t mode)
{
	bool ret = true;
	BUG_ON(!PageMovable(page));
	BUG_ON(PageIsolated(page));

	spin_lock(&prezeroed_lock);
	/* kzerod page must be in the prezeroed_list at this point */
	list_del(&page->lru);
	nr_prezeroed--;
	spin_unlock(&prezeroed_lock);

	return ret;
}

/* page and newpage are already locked before this function is called */
int kzerod_page_migrate(struct address_space *mapping, struct page *newpage,
		struct page *page, enum migrate_mode mode)
{
	int ret = MIGRATEPAGE_SUCCESS;
	void *s_addr, *d_addr;

	BUG_ON(!PageMovable(page));
	BUG_ON(!PageIsolated(page));

	/* set the newpage attributes and copy content from page to newpage */
	get_page(newpage);
	set_kzerod_page(newpage);
	s_addr = kmap_atomic(page);
	d_addr = kmap_atomic(newpage);
	memcpy(d_addr, s_addr, PAGE_SIZE);
	kunmap_atomic(d_addr);
	kunmap_atomic(s_addr);

	/* clear the original page attributes */
	unset_kzerod_page(page);
	put_page(page);

	/* put the newpage into the list again */
	spin_lock(&prezeroed_lock);
	list_add(&newpage->lru, &prezeroed_list);
	nr_prezeroed++;
	spin_unlock(&prezeroed_lock);

	return ret;
}

/* page is already locked before this function is called */
void kzerod_page_putback(struct page *page)
{
	BUG_ON(!PageMovable(page));
	BUG_ON(!PageIsolated(page));

	/* put the page into the list again */
	spin_lock(&prezeroed_lock);
	list_add(&page->lru, &prezeroed_list);
	nr_prezeroed++;
	spin_unlock(&prezeroed_lock);
}

const struct address_space_operations kzerod_aops = {
	.isolate_page = kzerod_page_isolate,
	.migratepage = kzerod_page_migrate,
	.putback_page = kzerod_page_putback,
};

static int kzerod_register_migration(void)
{
	kzerod_inode = alloc_anon_inode(kzerod_mnt->mnt_sb);
	if (IS_ERR(kzerod_inode)) {
		kzerod_inode = NULL;
		return 1;
	}

	kzerod_inode->i_mapping->a_ops = &kzerod_aops;
	return 0;
}

static void kzerod_unregister_migration(void)
{
	iput(kzerod_inode);
}

static struct dentry *kzerod_pseudo_mount(struct file_system_type *fs_type,
				int flags, const char *dev_name, void *data)
{
	static const struct dentry_operations ops = {
		.d_dname = simple_dname,
	};

	return mount_pseudo(fs_type, "kzerod:", NULL, &ops, KZEROD_MAGIC);
}

static struct file_system_type kzerod_fs = {
	.name		= "kzerod",
	.mount		= kzerod_pseudo_mount,
	.kill_sb	= kill_anon_super,
};

static int kzerod_mount(void)
{
	int ret = 0;

	kzerod_mnt = kern_mount(&kzerod_fs);
	if (IS_ERR(kzerod_mnt)) {
		ret = PTR_ERR(kzerod_mnt);
		kzerod_mnt = NULL;
	}

	return ret;
}

static void kzerod_unmount(void)
{
	kern_unmount(kzerod_mnt);
}


static int kzerod_zeroing(unsigned long *prezeroed)
{
	struct page *page;
	int ret;
	gfp_t gfp_mask;
	*prezeroed = 0;

	kzerod_update_wmark();
	gfp_mask = (GFP_HIGHUSER_MOVABLE & ~__GFP_DIRECT_RECLAIM) | __GFP_ZERO
			| __GFP_NOWARN;
	while (true) {
		if (!kzerod_enabled) {
			ret = -ENODEV;
			break;
		}
		if (kzerod_wmark_high_ok()) {
			ret = 0;
			break;
		}
		page = alloc_pages(gfp_mask, 0);
		if (!page) {
			ret = -ENOMEM;
			break;
		}
		lock_page(page);
		spin_lock(&prezeroed_lock);
		set_kzerod_page(page);
		list_add(&page->lru, &prezeroed_list);
		nr_prezeroed++;
		spin_unlock(&prezeroed_lock);
		unlock_page(page);
		(*prezeroed)++;
	}
	return ret;
}

static int kzerod(void *p)
{
	int ret;
	unsigned long prev_zero, cur_zero, prezeroed;
	static unsigned long prev_jiffies;

	kzerod_update_wmark();
	while (true) {
		wait_event_freezable(kzerod_wait,
				     kzerod_state == KZEROD_RUNNING);
		prev_zero = nr_prezeroed;
		trace_printk("woken up %lu < %lu < %lu KB\n",
			     K(kzerod_wmark_low), K(nr_prezeroed),
			     K(kzerod_wmark_high));
		prev_jiffies = jiffies;
		ret = kzerod_zeroing(&prezeroed);
		cur_zero = nr_prezeroed;
		trace_printk("ret:%s(%d) zeroed:%lu->%lu(%luKB) %ums\n",
			     ret ? "paused" : "finished", ret,
			     K(prev_zero), K(cur_zero), K(prezeroed),
			     jiffies_to_msecs(jiffies - prev_jiffies));
		switch (ret) {
		case 0:
			kzerod_state = KZEROD_SLEEP_DONE;
			break;
		case -ENOMEM:
			kzerod_state = KZEROD_SLEEP_NOMEM;
			break;
		case -ENODEV:
			kzerod_state = KZEROD_SLEEP_DISABLED;
			break;
		}
	}

	return 0;
}

static int __init kzerod_init(void)
{
	int ret;
	struct sched_param param = { .sched_priority = 0 };

	spin_lock_init(&prezeroed_lock);
	am_app_launch_notifier_register(&kzerod_app_launch_nb);
	task_kzerod = kthread_run(kzerod, NULL, "kzerod");
	if (IS_ERR(task_kzerod)) {
		task_kzerod = NULL;
		pr_err("Failed to start kzerod\n");
		return 0;
	}
	sched_setscheduler(task_kzerod, SCHED_IDLE, &param);

	ret = kzerod_mount();
	if (ret)
		goto out;
	if (kzerod_register_migration())
		goto out;

	return 0;
out:
	BUG();
	return -EINVAL;
}

static void __exit kzerod_exit(void)
{
	if (kzerod_inode)
		kzerod_unregister_migration();
	if (kzerod_mnt)
		kzerod_unmount();
}

static int kzerod_enabled_param_set(const char *val,
				   const struct kernel_param *kp)
{
	int error;
	bool prev;

	if (!task_kzerod) {
		pr_err("can't enable, task_kzerod is not ready\n");
		return -ENODEV;
	}

	prev = kzerod_enabled;
	error = param_set_bool(val, kp);
	if (error)
		return error;

	if (!prev && kzerod_enabled) {
		kzerod_state = KZEROD_RUNNING,
		wake_up(&kzerod_wait);
		trace_printk("kzerod: enabled\n");
	} else if (prev && !kzerod_enabled) {
		drain_zeroed_page();
		trace_printk("kzerod: disabled\n");
	}
	return error;
}

static struct kernel_param_ops kzerod_enabled_param_ops = {
	.set =	kzerod_enabled_param_set,
	.get =	param_get_bool,
};
module_param_cb(enabled, &kzerod_enabled_param_ops, &kzerod_enabled, 0644);

#undef K
module_init(kzerod_init)
module_exit(kzerod_exit);
