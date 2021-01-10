/*
 * linux/mm/rbinregion.c
 *
 * A physical memory allocator for rbincache.
 *
 * Manages physical pages based on lru
 * Although struct page->lru is used for managing lists,
 * struct page is not explicitly provided outside the region api.
 * Instead, struct rr_handle is provided to the clients.
 */

#include <linux/rbinregion.h>
#include <linux/vmalloc.h>

unsigned long totalrbin_pages __read_mostly;
atomic_t rbin_allocated_pages = ATOMIC_INIT(0);
atomic_t rbin_cached_pages = ATOMIC_INIT(0);
atomic_t rbin_evicted_pages = ATOMIC_INIT(0);
atomic_t rbin_pool_pages = ATOMIC_INIT(0);

static struct rbin_region region;

/* region management */
#define RC_EVICT_BATCH 1
static void region_mem_evict(void)
{
	struct rr_handle *handle, *next;
	int count = 0;
	unsigned long flags;

	if (!region.ops->evict)
		return;

	spin_lock_irqsave(&region.lru_lock, flags);
	list_for_each_entry_safe(handle, next, &region.usedlist, lru) {
		if (++count > RC_EVICT_BATCH)
			break;
		/* move to list tail and skip handle being used by ion. */
		if (handle->usage == ION_INUSE) {
			list_move_tail(&handle->lru, &region.usedlist);
			continue;
		}
		if (handle->usage == RC_INUSE) {
			atomic_dec(&rbin_cached_pages);
			mod_zone_page_state(region.zone, NR_FREE_RBIN_PAGES, 1);
		}
		/* mark freed, load/flush ops will be denied */
		handle->usage = RC_FREED;
		list_del(&handle->lru);
		spin_unlock_irqrestore(&region.lru_lock, flags);

		region.ops->evict((unsigned long)handle);
		atomic_inc(&rbin_evicted_pages);
		spin_lock_irqsave(&region.lru_lock, flags);
		list_add(&handle->lru, &region.freelist);
	}
	spin_unlock_irqrestore(&region.lru_lock, flags);
}

/* Add handle to [free|used]list */
static void add_to_list(struct rr_handle *handle, struct list_head *head)
{
	unsigned long flags;

	spin_lock_irqsave(&region.lru_lock, flags);
	list_add_tail(&handle->lru, head);
	spin_unlock_irqrestore(&region.lru_lock, flags);
}

/*
 * Find a free slot from region, detach the memory chunk from the list,
 * then returns the corresponding handle.
 * If there are no free slot, evict a slot from usedlist.
 */
static struct rr_handle *region_get_freemem(void)
{
	struct rr_handle *handle = NULL;
	unsigned long flags;

	spin_lock_irqsave(&region.lru_lock, flags);
	if (list_empty(&region.freelist)) {
		spin_unlock_irqrestore(&region.lru_lock, flags);
		region_mem_evict();
		spin_lock_irqsave(&region.lru_lock, flags);
	}
	handle = list_first_entry_or_null(&region.freelist, struct rr_handle, lru);
	if (!handle) {
		spin_unlock_irqrestore(&region.lru_lock, flags);
		goto out;
	}
	list_del(&handle->lru);
	spin_unlock_irqrestore(&region.lru_lock, flags);

	/* Skip if handle is(was) used by ion. Wait for eviction in usedlist */
	if (handle->usage == ION_INUSE) {
		add_to_list(handle, &region.usedlist);
		return NULL;
	}

	if (handle->usage == ION_FREED)
		region.ops->evict((unsigned long)handle);
	memset(handle, 0, sizeof(struct rr_handle));
out:
	return handle;
}

struct rr_handle *region_store_cache(struct page *src, int pool_id,
				     int rb_index, int ra_index)
{
	struct rr_handle *handle;
	unsigned long flags;

	if (!try_get_rbincache())
		return NULL;

	handle = region_get_freemem();
	if (!handle)
		goto out;

	BUG_ON(!handle_is_valid(handle));

	copy_page(page_address(handle_to_page(handle)), page_address(src));

	spin_lock_irqsave(&region.lru_lock, flags);
	handle->pool_id = pool_id;
	handle->rb_index = rb_index;
	handle->ra_index = ra_index;
	handle->usage = RC_INUSE;
	spin_unlock_irqrestore(&region.lru_lock, flags);
	add_to_list(handle, &region.usedlist);
	mod_zone_page_state(region.zone, NR_FREE_RBIN_PAGES, -1);
	atomic_inc(&rbin_cached_pages);
out:
	put_rbincache();

	return handle;
}

int region_load_cache(struct rr_handle *handle, struct page *dst,
		      int pool_id, int rb_index, int ra_index)
{
	struct page *page;
	unsigned long flags;
	int ret = -EINVAL;

	if (!try_get_rbincache())
		return ret;

	BUG_ON(!handle_is_valid(handle));

	if (handle->usage != RC_INUSE)
		goto out;

	spin_lock_irqsave(&region.lru_lock, flags);
	/* skip if handle is invalid (freed or overwritten) */
	if ((handle->usage != RC_INUSE) ||
			(dst && (handle->pool_id != pool_id ||
			handle->rb_index != rb_index ||
			handle->ra_index != ra_index))) {
		spin_unlock_irqrestore(&region.lru_lock, flags);
		goto out;
	}
	handle->usage = RC_FREED;
	list_del(&handle->lru);
	spin_unlock_irqrestore(&region.lru_lock, flags);

	if (dst) {
		page = handle_to_page(handle);
		copy_page(page_address(dst), page_address(page));
	}
	add_to_list(handle, &region.freelist);
	atomic_dec(&rbin_cached_pages);
	mod_zone_page_state(region.zone, NR_FREE_RBIN_PAGES, 1);
	ret = 0;
out:
	put_rbincache();

	return ret;
}

int region_flush_cache(struct rr_handle *handle)
{
	return region_load_cache(handle, NULL, 0, 0, 0);
}

void init_region(unsigned long pfn, unsigned long nr_pages,
		const struct region_ops *ops)
{
	struct rr_handle *handle;
	unsigned long i;

	region.zone = page_zone(pfn_to_page(pfn));
	region.start_pfn = pfn;
	region.end_pfn = pfn + nr_pages;
	region.ops = ops;
	spin_lock_init(&region.lru_lock);
	spin_lock_init(&region.region_lock);
	region.handles = (struct rr_handle *)vzalloc(nr_pages *
			sizeof(struct rr_handle));
	INIT_LIST_HEAD(&region.freelist);
	INIT_LIST_HEAD(&region.usedlist);
	for (i = 0; i < nr_pages; i++) {
		handle = &region.handles[i];
		INIT_LIST_HEAD(&handle->lru);
		list_add(&handle->lru, &region.freelist);
	}
	mod_zone_page_state(region.zone, NR_FREE_RBIN_PAGES, totalrbin_pages);
}
/* region management end */

/* helper function definitions */
struct rr_handle *pfn_to_handle(unsigned long pfn)
{
	unsigned long idx = pfn - region.start_pfn;

	return &region.handles[idx];
}
struct rr_handle *page_to_handle(struct page *page)
{
	unsigned long idx = page_to_pfn(page) - region.start_pfn;

	return &region.handles[idx];
}
struct page *handle_to_page(struct rr_handle *handle)
{
	unsigned long idx = (unsigned long)(handle - region.handles);

	return pfn_to_page(region.start_pfn + idx);
}
bool handle_is_valid(struct rr_handle *handle)
{
	unsigned long nr_pages = region.end_pfn - region.start_pfn;

	return (handle >= region.handles)
		&& (handle < region.handles + nr_pages);
}
/* helper function definitions end */

/* region sharing apis */
#define RC_AUTO_ENABLE_TIMEOUT (5*HZ) /* 5 sec */
bool try_get_rbincache(void)
{
	bool ret = true;
	unsigned long flags;

	spin_lock_irqsave(&region.region_lock, flags);
	if (region.timeout < jiffies) {
		if (region.rc_disabled == true)
			wake_ion_rbin_heap_shrink();
		region.rc_disabled = false;
	}

	if (region.rc_disabled || region.ion_inflight)
		ret = false;
	else
		region.rc_inflight++;
	spin_unlock_irqrestore(&region.region_lock, flags);

	return ret;
}

void put_rbincache(void)
{
	unsigned long flags;

	spin_lock_irqsave(&region.region_lock, flags);
	region.rc_inflight--;
	spin_unlock_irqrestore(&region.region_lock, flags);
}

static bool try_get_ion_rbin(void)
{
	bool ret = true;
	unsigned long flags;

	spin_lock_irqsave(&region.region_lock, flags);
	/* disable rbincache ops for a while */
	region.rc_disabled = true;
	region.timeout = jiffies + RC_AUTO_ENABLE_TIMEOUT;
	if (region.rc_inflight)
		ret = false;
	else
		region.ion_inflight++;
	spin_unlock_irqrestore(&region.region_lock, flags);

	return ret;
}

static void put_ion_rbin(void)
{
	unsigned long flags;

	spin_lock_irqsave(&region.region_lock, flags);
	region.ion_inflight--;
	spin_unlock_irqrestore(&region.region_lock, flags);
}

static void isolate_region(unsigned long start_pfn, unsigned long nr_pages)
{
	struct rr_handle *handle;
	unsigned long pfn;
	int nr_cached = 0;

	for (pfn = start_pfn; pfn < start_pfn + nr_pages; pfn++) {
		/*
		 * Mark pages used by ion. Rbincache ops are blocked for now,
		 * so it's okay to do this without any lock. Later, accessing
		 * these pages by rbincache will be denied.
		 */
		handle = pfn_to_handle(pfn);
		if (handle->usage == RC_INUSE)
			nr_cached++;
		handle->usage = ION_INUSE;
	}
	atomic_sub(nr_cached, &rbin_cached_pages);
	mod_zone_page_state(region.zone, NR_FREE_RBIN_PAGES, -(nr_pages - nr_cached));
}

static void putback_region(unsigned start_pfn, unsigned long nr_pages)
{
	struct rr_handle *handle;
	unsigned long pfn;

	for (pfn = start_pfn; pfn < start_pfn + nr_pages; pfn++) {
		handle = pfn_to_handle(pfn);
		handle->usage = ION_FREED;
	}
	mod_zone_page_state(region.zone, NR_FREE_RBIN_PAGES, nr_pages);
}
/* region sharing apis */

/* ion_rbin_heap apis */
phys_addr_t ion_rbin_allocate(unsigned long size)
{
	unsigned long offset;

	if (!try_get_ion_rbin())
		return ION_RBIN_ALLOCATE_FAIL;

	offset = gen_pool_alloc(region.pool, size);
	if (!offset) {
		offset = ION_RBIN_ALLOCATE_FAIL;
		goto out;
	}
	isolate_region(PFN_DOWN(offset), size >> PAGE_SHIFT);
out:
	put_ion_rbin();

	return offset;
}

void ion_rbin_free(phys_addr_t addr, unsigned long size)
{
	if (addr == ION_RBIN_ALLOCATE_FAIL)
		return;

	putback_region(PFN_DOWN(addr), size >> PAGE_SHIFT);
	gen_pool_free(region.pool, addr, size);
}

int init_rbinregion(unsigned long base, unsigned long size)
{
	region.pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!region.pool) {
		pr_err("%s failed get_pool_create\n", __func__);
		return -1;
	}
	gen_pool_add(region.pool, base, size, -1);
	if (init_rbincache(PFN_DOWN(base), size >> PAGE_SHIFT)) {
		gen_pool_destroy(region.pool);
		return -1;
	}
	return 0;
}
/* ion_rbin_heap apis end */
