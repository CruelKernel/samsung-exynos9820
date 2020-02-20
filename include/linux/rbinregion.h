#ifndef	_LINUX_RBIN_REGION_H
#define	_LINUX_RBIN_REGION_H

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/genalloc.h>
#include <linux/highmem.h>

#define E_NOREGION 55 // returned when region is disabled

struct rr_handle {
	int pool_id;	/* Pool index         : corresponds to filesystem */
	int rb_index;	/* Redblack tree index: value equals inode number */
	int ra_index;	/* Radix tree index   : value equals page->index  */
	int usage;	/* indicates current usage of corresponding page  */
	struct list_head lru;		/* use lru of struct page instead */
};

enum usage {
	RC_FREED,
	RC_INUSE,
	ION_FREED,
	ION_INUSE,
};

struct region_ops {
	void (*evict)(unsigned long handle);
};

struct rbin_region {
	struct zone *zone;
	unsigned long start_pfn;
	unsigned long end_pfn;
	struct rr_handle *handles;
	struct list_head freelist;	/* protected by lru_lock. handle->lru */
	struct list_head usedlist;	/* protected by lru_lock. handle->lru */
	const struct region_ops *ops;

	spinlock_t lru_lock;
	spinlock_t region_lock;
	struct gen_pool *pool;
	int ion_inflight;
	int rc_inflight;
	bool rc_disabled;
	unsigned long timeout;		/* timeout for rbincache enable */
};

/* rbin region api's */
#define ZERO_HANDLE     ((void *)~(~0UL >> 1))
struct rr_handle *region_store_cache(struct page *page, int pool_id,
				     int rb_index, int ra_index);
int region_load_cache(struct rr_handle *handle, struct page *page,
		      int pool_id, int rb_index, int ra_index);
int region_flush_cache(struct rr_handle *handle);
bool try_get_rbincache(void);
void put_rbincache(void);
void init_region(unsigned long pfn, unsigned long nr_pages,
		const struct region_ops *ops);
/* rbin region api's end */

/* helper function declaration */
struct rr_handle *pfn_to_handle(unsigned long pfn);
struct rr_handle *page_to_handle(struct page *page);
struct page *handle_to_page(struct rr_handle *handle);
bool handle_is_valid(struct rr_handle *handle);
/* helper function declaration end */

/* ion_rbin_heap apis */
#define ION_RBIN_ALLOCATE_FAIL -1
phys_addr_t ion_rbin_allocate(unsigned long size);
void ion_rbin_free(phys_addr_t addr, unsigned long size);
int init_rbinregion(unsigned long base, unsigned long size);
/* ion_rbin_heap apis end */

#endif	/* _LINUX_RBIN_REGION_H*/
