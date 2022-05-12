/*
 * linux/mm/rbincache.c
 *
 * A cleancache backend for fast ion allocation.
 * Cache management methods based on rbincache.
 * Copyright (C) 2019 Samsung Electronics
 *
 * With rbincache, active file pages can be backed up in memory during page
 * reclaiming. When their data is needed again the I/O reading operation is
 * avoided. Up to here it might seem as a waste of time and resource just to
 * copy file pages into cleancache area.
 *
 * However, since the cleancache API ensures the cache pages to be clean,
 * we can make use of clean file caches stored in one contiguous section.
 * By having two modes(1.cache_mode, 2.alloc_mode), rbincache normally
 * acts as a cleancache backbone, while providing a rapidly reclaimed
 * contiguous space when needed.
 *
 */

#include <linux/atomic.h>
#include <linux/cleancache.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rbinregion.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/swap.h>

/*
 * rbincache: a cleancache API implementation
 *
 * When a file page is passed from cleancache to rbincache, rbincache maintains
 * a mapping of the <filesystem_type, inode_number, page_index> to the
 * rr_handle that represents the backed up file page.
 * This mapping is achieved with a red-black tree per filesystem,
 * plus a radix tree per a red-black node.
 *
 * A rbincache pool is assigned its pool_id when a filesystem mounted
 * Each rbincache pool has a red-black tree, where the inode number(rb_index)
 * is the search key. In other words, each rb_node represents an inode.
 * Each rb_node has a radix tree which use page->index(ra_index) as the index.
 * Each radix tree slot points to the rr_handle.
 *
 * The implementation borrows pretty much concepts from Zcache.
 */

static bool rbincache_enabled __read_mostly;
module_param_named(enabled, rbincache_enabled, bool, 0444);

/* statistics */
extern unsigned long totalrbin_pages;
extern atomic_t rbin_allocated_pages;
extern atomic_t rbin_cached_pages;
extern atomic_t rbin_evicted_pages;
extern atomic_t rbin_pool_pages;
static atomic_t rbin_zero_pages = ATOMIC_INIT(0);

static atomic_t rc_num_rbnode = ATOMIC_INIT(0);
static atomic_t rc_num_ra_entry = ATOMIC_INIT(0);
static atomic_t rc_num_dup_handle = ATOMIC_INIT(0);

static atomic_t rc_num_init_fs = ATOMIC_INIT(0);
static atomic_t rc_num_init_shared_fs = ATOMIC_INIT(0);
static atomic_t rc_num_gets = ATOMIC_INIT(0);
static atomic_t rc_num_puts = ATOMIC_INIT(0);
static atomic_t rc_num_flush_page = ATOMIC_INIT(0);
static atomic_t rc_num_flush_inode = ATOMIC_INIT(0);
static atomic_t rc_num_flush_fs = ATOMIC_INIT(0);

static atomic_t rc_num_succ_init_fs = ATOMIC_INIT(0);
static atomic_t rc_num_succ_gets = ATOMIC_INIT(0);
static atomic_t rc_num_succ_puts = ATOMIC_INIT(0);
static atomic_t rc_num_succ_flush_page = ATOMIC_INIT(0);
static atomic_t rc_num_succ_flush_inode = ATOMIC_INIT(0);
static atomic_t rc_num_succ_flush_fs = ATOMIC_INIT(0);
/* statistics end */

/* rbincache data structures */
#define MAX_RC_POOLS 32

/* One rbincache pool per filesystem mount instance */
struct rc_pool {
	struct rb_root rbtree;
	rwlock_t rb_lock;			/* Protects rbtree */
};

/* Manage all rbincache pools */
struct rbincache {
	struct rc_pool *pools[MAX_RC_POOLS];
	u32 num_pools;			/* current number of rbincache pools */
	spinlock_t pool_lock;	/* Protects pools[] and num_pools */
};
struct rbincache rbincache;

/*
 * Redblack tree node, each node has a page index radix-tree.
 * Indexed by inode nubmer.
 */
struct rc_rbnode {
	struct rb_node rb_node;
	int rb_index;
	struct radix_tree_root ratree;	/* Page radix tree per inode rbtree */
	spinlock_t ra_lock;				/* Protects radix tree */
	struct kref refcount;
};
/* rbincache data structures end */

/* rbincache slab allocation */
static struct kmem_cache *rc_rbnode_cache;
static int rc_rbnode_cache_create(void)
{
	rc_rbnode_cache = KMEM_CACHE(rc_rbnode, 0);
	return rc_rbnode_cache == NULL;
}
static void rc_rbnode_cache_destroy(void)
{
	kmem_cache_destroy(rc_rbnode_cache);
}
/* rbincache slab allocation end */

/* rbincache rb_tree & ra_tree helper functions */

/*
 * The caller should hold rb_lock
 */
static struct rc_rbnode *rc_find_rbnode(struct rb_root *rbtree,
	int index, struct rb_node **rb_parent, struct rb_node ***rb_link)
{
	struct rc_rbnode *entry;
	struct rb_node **__rb_link, *__rb_parent, *rb_prev;

	__rb_link = &rbtree->rb_node;
	rb_prev = __rb_parent = NULL;

	while (*__rb_link) {
		__rb_parent = *__rb_link;
		entry = rb_entry(__rb_parent, struct rc_rbnode, rb_node);
		if (entry->rb_index > index)
			__rb_link = &__rb_parent->rb_left;
		else if (entry->rb_index < index) {
			rb_prev = __rb_parent;
			__rb_link = &__rb_parent->rb_right;
		} else
			return entry;
	}

	if (rb_parent)
		*rb_parent = __rb_parent;
	if (rb_link)
		*rb_link = __rb_link;
	return NULL;
}

static struct rc_rbnode *rc_find_get_rbnode(struct rc_pool *rcpool,
		int rb_index)
{
	unsigned long flags;
	struct rc_rbnode *rbnode;

	read_lock_irqsave(&rcpool->rb_lock, flags);
	rbnode = rc_find_rbnode(&rcpool->rbtree, rb_index, 0, 0);
	if (rbnode)
		kref_get(&rbnode->refcount);
	read_unlock_irqrestore(&rcpool->rb_lock, flags);
	return rbnode;
}

/*
 * kref_put callback for rc_rbnode.
 * The rbnode must have been isolated from rbtree already.
 * Called when rbnode has 0 references.
 */
static void rc_rbnode_release(struct kref *kref)
{
	struct rc_rbnode *rbnode;

	rbnode = container_of(kref, struct rc_rbnode, refcount);
	BUG_ON(rbnode->ratree.rnode);
	kmem_cache_free(rc_rbnode_cache, rbnode);
	atomic_dec(&rc_num_rbnode);
}

/*
 * Check whether the radix-tree of this rbnode is empty.
 * If that's true, then we can delete this rc_rbnode from
 * rc_pool->rbtree
 *
 * Caller must hold rc_rbnode->ra_lock
 */
static inline int rc_rbnode_empty(struct rc_rbnode *rbnode)
{
	return rbnode->ratree.rnode == NULL;
}

/*
 * Remove rc_rbnode from rcpool->rbtree
 */
static void rc_rbnode_isolate(struct rc_pool *rcpool, struct rc_rbnode *rbnode)
{
	/*
	 * Someone can get reference on this rbnode before we could
	 * acquire write lock above.
	 * We want to remove it from rcpool->rbtree when only the caller and
	 * corresponding ratree holds a reference to this rbnode.
	 * Below check ensures that a racing rc put will not end up adding
	 * a page to an isolated node and thereby losing that memory.
	 */
	if (atomic_read(&rbnode->refcount.refcount.refs) == 2) {
		rb_erase(&rbnode->rb_node, &rcpool->rbtree);
		RB_CLEAR_NODE(&rbnode->rb_node);
		kref_put(&rbnode->refcount, rc_rbnode_release);
	} else {
		trace_printk("rbincache: unabled to erase rbnode : refcount=%d\n",
				atomic_read(&rbnode->refcount.refcount.refs));
	}
}

static int rc_store_handle(int pool_id, int rb_index, int ra_index, void *handle)
{
	unsigned long flags;
	struct rc_pool *rcpool;
	struct rc_rbnode *rbnode, *dup_rbnode;
	struct rb_node **link = NULL, *parent = NULL;
	int ret = 0;
	void *dup_handle;

	rcpool = rbincache.pools[pool_id];
	rbnode = rc_find_get_rbnode(rcpool, rb_index);
	if (!rbnode) {
		/* create, init, and get(inc refcount) a new rbnode */
		rbnode = kmem_cache_alloc(rc_rbnode_cache, 0);
		if (!rbnode)
			return -ENOMEM;
		atomic_inc(&rc_num_rbnode);

		INIT_RADIX_TREE(&rbnode->ratree, GFP_ATOMIC|__GFP_NOWARN);
		spin_lock_init(&rbnode->ra_lock);
		rbnode->rb_index = rb_index;
		kref_init(&rbnode->refcount);
		RB_CLEAR_NODE(&rbnode->rb_node);

		/* add that rbnode to rbtree */
		write_lock_irqsave(&rcpool->rb_lock, flags);
		dup_rbnode = rc_find_rbnode(&rcpool->rbtree, rb_index,
				&parent, &link);
		if (dup_rbnode) {
			/* somebody else allocated new rbnode */
			kmem_cache_free(rc_rbnode_cache, rbnode);
			atomic_dec(&rc_num_rbnode);
			rbnode = dup_rbnode;
		} else {
			rb_link_node(&rbnode->rb_node, parent, link);
			rb_insert_color(&rbnode->rb_node, &rcpool->rbtree);
		}
		/* Inc the reference of this rc_rbnode */
		kref_get(&rbnode->refcount);
		write_unlock_irqrestore(&rcpool->rb_lock, flags);
	}

	/* Succfully got a rc_rbnode when arriving here */
	spin_lock_irqsave(&rbnode->ra_lock, flags);
	dup_handle = radix_tree_delete(&rbnode->ratree, ra_index);
	if (unlikely(dup_handle)) {
		atomic_inc(&rc_num_dup_handle);
		if (dup_handle == ZERO_HANDLE)
			atomic_dec(&rbin_zero_pages);
		else
			region_flush_cache(dup_handle);
	}

	ret = radix_tree_insert(&rbnode->ratree, ra_index, (void *)handle);
	spin_unlock_irqrestore(&rbnode->ra_lock, flags);

	if (unlikely(ret)) {
		// insert failed
		write_lock_irqsave(&rcpool->rb_lock, flags);
		spin_lock(&rbnode->ra_lock);

		if (rc_rbnode_empty(rbnode))
			rc_rbnode_isolate(rcpool, rbnode);

		spin_unlock(&rbnode->ra_lock);
		write_unlock_irqrestore(&rcpool->rb_lock, flags);
		trace_printk("rbincache: handle insertion failed\n");
	} else {
		atomic_inc(&rc_num_ra_entry);
	}

	kref_put(&rbnode->refcount, rc_rbnode_release);
	return ret;
}

/*
 * Load handle and delete it from radix tree.
 * If the radix tree of the corresponding rbnode is empty, delete the rbnode
 * from rcpool->rbtree also.
 */
static struct rr_handle *rc_load_del_handle(int pool_id,
		int rb_index, int ra_index)
{
	struct rc_pool *rcpool;
	struct rc_rbnode *rbnode;
	struct rr_handle *handle = NULL;
	unsigned long flags;

	rcpool = rbincache.pools[pool_id];
	rbnode = rc_find_get_rbnode(rcpool, rb_index);
	if (!rbnode)
		goto out;

	BUG_ON(rbnode->rb_index != rb_index);

	spin_lock_irqsave(&rbnode->ra_lock, flags);
	handle = radix_tree_delete(&rbnode->ratree, ra_index);
	spin_unlock_irqrestore(&rbnode->ra_lock, flags);
	if (!handle)
		goto no_handle;
	else if (handle == ZERO_HANDLE)
		atomic_dec(&rbin_zero_pages);

	atomic_dec(&rc_num_ra_entry);
	/* rb_lock and ra_lock must be taken again in the given sequence */
	write_lock_irqsave(&rcpool->rb_lock, flags);
	spin_lock(&rbnode->ra_lock);
	if (rc_rbnode_empty(rbnode))
		rc_rbnode_isolate(rcpool, rbnode);
	spin_unlock(&rbnode->ra_lock);
	write_unlock_irqrestore(&rcpool->rb_lock, flags);

no_handle:
	kref_put(&rbnode->refcount, rc_rbnode_release);
out:
	return handle;
}

/* rbincache rb_tree & ra_tree helper functions */

/* Cleancache API implementation start */
static bool is_zero_page(struct page *page)
{
	unsigned long *ptr = kmap_atomic(page);
	int i;
	bool ret = false;

	for (i = 0; i < PAGE_SIZE / sizeof(*ptr); i++) {
		if (ptr[i])
			goto out;
	}
	ret = true;
out:
	kunmap_atomic(ptr);
	return ret;
}

/*
 * function for cleancache_ops->put_page
 * Though it might fail, it does not matter since Cleancache does not
 * guarantee the stored pages to be preserved.
 */
static void rc_store_page(int pool_id, struct cleancache_filekey key,
				     pgoff_t index, struct page *src)
{
	struct rr_handle *handle;
	int ret;
	bool zero;

	if (!current_is_kswapd())
		return;

	atomic_inc(&rc_num_puts);
	zero = is_zero_page(src);
	if (zero) {
		handle = (struct rr_handle *)ZERO_HANDLE;
		goto out_zero;
	}
	handle = region_store_cache(src, pool_id, key.u.ino, index);
	if (!handle)
		return;
out_zero:
	ret = rc_store_handle(pool_id, key.u.ino, index, handle);
	if (ret) { // failed
		if (!zero)
			region_flush_cache(handle);
		return;
	}

	/* update stats */
	atomic_inc(&rc_num_succ_puts);
	if (zero)
		atomic_inc(&rbin_zero_pages);
}

/*
 * function for cleancache_ops->get_page
 */
static int rc_load_page(int pool_id, struct cleancache_filekey key,
				    pgoff_t index, struct page *dst)
{
	struct rr_handle *handle;
	int ret = -EINVAL;
	void *addr;

	atomic_inc(&rc_num_gets);
	handle = rc_load_del_handle(pool_id, key.u.ino, index);
	if (!handle)
		goto out;

	if (handle == ZERO_HANDLE) {
		addr = kmap_atomic(dst);
		memset(addr, 0, PAGE_SIZE);
		kunmap_atomic(addr);
		flush_dcache_page(dst);
		ret = 0;
	} else {
		ret = region_load_cache(handle, dst, pool_id, key.u.ino, index);
	}
	if (ret)
		goto out;

	/* update stats */
	atomic_inc(&rc_num_succ_gets);
out:
	return ret;
}

static void rc_flush_page(int pool_id, struct cleancache_filekey key,
				       pgoff_t index)
{
	struct rr_handle *handle;

	atomic_inc(&rc_num_flush_page);
	handle = rc_load_del_handle(pool_id, key.u.ino, index);
	if (!handle)
		return;

	if (handle != ZERO_HANDLE)
		region_flush_cache(handle);

	atomic_inc(&rc_num_succ_flush_page);
}

#define FREE_BATCH 16
/*
 * Callers must hold the lock
 */
static int rc_flush_ratree(struct rc_pool *rcpool,
		struct rc_rbnode *rbnode)
{
	unsigned long index = 0;
	int count, i, total_count = 0;
	struct rr_handle *handle;
	void *results[FREE_BATCH];
	unsigned long indices[FREE_BATCH];

	do {
		count = radix_tree_gang_lookup_index(&rbnode->ratree,
				(void **)results, indices, index, FREE_BATCH);

		for (i = 0; i < count; i++) {
			if (results[i] == ZERO_HANDLE) {
				handle = radix_tree_delete(&rbnode->ratree,
								indices[i]);
				if (handle)
					atomic_dec(&rbin_zero_pages);
				continue;
			}
			handle = (struct rr_handle *)results[i];
			index = handle->ra_index;
			handle = radix_tree_delete(&rbnode->ratree, index);
			if (!handle)
				continue;
			atomic_dec(&rc_num_ra_entry);
			total_count++;
			region_flush_cache(handle);
		}
		index++;
	} while (count == FREE_BATCH);
	return total_count;
}

static void rc_flush_inode(int pool_id, struct cleancache_filekey key)
{
	struct rc_rbnode *rbnode;
	unsigned long flags1, flags2;
	struct rc_pool *rcpool = rbincache.pools[pool_id];
	int pages_flushed;

	atomic_inc(&rc_num_flush_inode);
	/*
	 * Refuse new pages added in to the same rbnode, so get rb_lock at
	 * first.
	 */
	write_lock_irqsave(&rcpool->rb_lock, flags1);
	rbnode = rc_find_rbnode(&rcpool->rbtree, key.u.ino, 0, 0);
	if (!rbnode) {
		write_unlock_irqrestore(&rcpool->rb_lock, flags1);
		return;
	}

	kref_get(&rbnode->refcount);
	spin_lock_irqsave(&rbnode->ra_lock, flags2);

	pages_flushed = rc_flush_ratree(rcpool, rbnode);
	if (rc_rbnode_empty(rbnode))
		/* When arrvied here, we already hold rb_lock */
		rc_rbnode_isolate(rcpool, rbnode);
	else {
		pr_err("ra_tree flushed but not emptied\n");
		BUG();
	}

	spin_unlock_irqrestore(&rbnode->ra_lock, flags2);
	kref_put(&rbnode->refcount, rc_rbnode_release);
	write_unlock_irqrestore(&rcpool->rb_lock, flags1);

	atomic_inc(&rc_num_succ_flush_inode);
	trace_printk("rbincache: %d pages flushed\n", pages_flushed);
}

static void rc_flush_fs(int pool_id)
{
	struct rc_rbnode *rbnode = NULL;
	struct rb_node *node;
	unsigned long flags1, flags2;
	struct rc_pool *rcpool;
	int pages_flushed = 0;

	atomic_inc(&rc_num_flush_fs);

	if (pool_id < 0)
		return;
	rcpool = rbincache.pools[pool_id];
	if (!rcpool)
		return;

	/*
	 * Refuse new pages added in, so get rb_lock at first.
	 */
	write_lock_irqsave(&rcpool->rb_lock, flags1);

	node = rb_first(&rcpool->rbtree);
	while (node) {
		rbnode = rb_entry(node, struct rc_rbnode, rb_node);
		node = rb_next(node);
		if (rbnode) {
			kref_get(&rbnode->refcount);
			spin_lock_irqsave(&rbnode->ra_lock, flags2);
			pages_flushed += rc_flush_ratree(rcpool, rbnode);
			if (rc_rbnode_empty(rbnode))
				rc_rbnode_isolate(rcpool, rbnode);
			spin_unlock_irqrestore(&rbnode->ra_lock, flags2);
			kref_put(&rbnode->refcount, rc_rbnode_release);
		}
	}
	write_unlock_irqrestore(&rcpool->rb_lock, flags1);

	atomic_inc(&rc_num_succ_flush_fs);
	trace_printk("rbincache: %d pages flushed\n", pages_flushed);
}

static int rc_init_fs(size_t pagesize)
{
	struct rc_pool *rcpool;
	int ret = -1;

	// init does not check rbincache_disabled. Even disabled, continue init.

	atomic_inc(&rc_num_init_fs);

	if (pagesize != PAGE_SIZE) {
		pr_warn("Unsupported page size: %zu", pagesize);
		ret = -EINVAL;
		goto out;
	}

	rcpool = kzalloc(sizeof(*rcpool), GFP_KERNEL);
	if (!rcpool) {
		ret = -ENOMEM;
		goto out;
	}

	spin_lock(&rbincache.pool_lock);
	if (rbincache.num_pools == MAX_RC_POOLS) {
		pr_err("Cannot create new pool (limit:%u)\n", MAX_RC_POOLS);
		ret = -EPERM;
		goto out_unlock;
	}

	rwlock_init(&rcpool->rb_lock);
	rcpool->rbtree = RB_ROOT;

	/* Add to pool list */
	for (ret = 0; ret < MAX_RC_POOLS; ret++)
		if (!rbincache.pools[ret])
			break;
	rbincache.pools[ret] = rcpool;
	rbincache.num_pools++;
	pr_info("New pool created id:%d\n", ret);

	atomic_inc(&rc_num_succ_init_fs);
	trace_printk("rbincache\n");

out_unlock:
	spin_unlock(&rbincache.pool_lock);
out:
	return ret;
}

static int rc_init_shared_fs(uuid_t *uuid, size_t pagesize)
{
	atomic_inc(&rc_num_init_shared_fs);
	return rc_init_fs(pagesize);
}

static const struct cleancache_ops rbincache_ops = {
	.init_fs = rc_init_fs,
	.init_shared_fs = rc_init_shared_fs,
	.get_page = rc_load_page,
	.put_page = rc_store_page,
	.invalidate_page = rc_flush_page,
	.invalidate_inode = rc_flush_inode,
	.invalidate_fs = rc_flush_fs,
};

/* Cleancache API implementation end */
static void rc_evict_cb(unsigned long raw_handle)
{
	struct rr_handle *h = (struct rr_handle *)raw_handle;

	/* try find and remove entry if exists */
	rc_load_del_handle(h->pool_id, h->rb_index, h->ra_index);
}

static struct region_ops rc_region_ops = {
	.evict = rc_evict_cb
};

static int __init rc_sysfs_init(void);

int init_rbincache(unsigned long pfn, unsigned long nr_pages)
{
	int err = 0;

	totalrbin_pages = nr_pages;
	init_region(pfn, nr_pages, &rc_region_ops);

	err = rc_rbnode_cache_create();
	if (err) {
		pr_err("entry cache creation failed\n");
		goto error;
	}

	spin_lock_init(&rbincache.pool_lock);

	err = cleancache_register_ops(&rbincache_ops);
	if (err) {
		pr_err("failed to register cleancache_ops: %d\n", err);
		goto register_fail;
	}

	if (rc_sysfs_init())
		pr_warn("sysfs initialization failed\n");

	pr_info("cleancache enabled for rbin cleancache\n");
	return 0;

register_fail:
	rc_rbnode_cache_destroy();
error:
	return err;
}

/*
 * Statistics
 */
#ifdef CONFIG_SYSFS
static ssize_t stats_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
			"total_pages      : %8lu\n"
			"cached_pages     : %8d\n"
			"evicted_pages    : %8d\n"
			"pool_pages       : %8d\n"
			"zero_pages       : %8d\n"
			"num_dup_handle   : %8d\n"
			"init_fs          : %8d\n"
			"init_shared_fs   : %8d\n"
			"puts             : %8d\n"
			"gets             : %8d\n"
			"flush_page       : %8d\n"
			"flush_inode      : %8d\n"
			"flush_fs         : %8d\n"
			"succ_init_fs     : %8d\n"
			"succ_puts        : %8d\n"
			"succ_gets        : %8d\n"
			"succ_flush_page  : %8d\n"
			"succ_flush_inode : %8d\n"
			"succ_flush_fs    : %8d\n",
			totalrbin_pages,
			atomic_read(&rbin_cached_pages),
			atomic_read(&rbin_evicted_pages),
			atomic_read(&rbin_pool_pages),
			atomic_read(&rbin_zero_pages),
			atomic_read(&rc_num_dup_handle),
			atomic_read(&rc_num_init_fs),
			atomic_read(&rc_num_init_shared_fs),
			atomic_read(&rc_num_puts),
			atomic_read(&rc_num_gets),
			atomic_read(&rc_num_flush_page),
			atomic_read(&rc_num_flush_inode),
			atomic_read(&rc_num_flush_fs),
			atomic_read(&rc_num_succ_init_fs),
			atomic_read(&rc_num_succ_puts),
			atomic_read(&rc_num_succ_gets),
			atomic_read(&rc_num_succ_flush_page),
			atomic_read(&rc_num_succ_flush_inode),
			atomic_read(&rc_num_succ_flush_fs));
}

static ssize_t stats_store(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	return count;
}

#define RC_ATTR(_name) \
	static struct kobj_attribute _name##_attr = \
		__ATTR(_name, 0644, _name##_show, _name##_store)
RC_ATTR(stats);

static struct attribute *rc_attrs[] = {
	&stats_attr.attr,
	NULL,
};

static struct attribute_group rc_attr_group = {
	.attrs = rc_attrs,
	.name = "rbincache",
};

static int __init rc_sysfs_init(void)
{
	int err;
	
	err = sysfs_create_group(mm_kobj, &rc_attr_group);
	if (err) {
		pr_err("sysfs create failed(%d)\n", err);
		return err;
	}
	return 0;
}

static void __exit rc_sysfs_exit(void)
{
	sysfs_remove_group(mm_kobj, &rc_attr_group);
}
#else
static int __init rc_sysfs_init(void)
{
	return 0;
}
static void __exit rc_sysfs_exit(void)
{
}
#endif

/* Debugfs functions end */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RBIN Cleancache");
