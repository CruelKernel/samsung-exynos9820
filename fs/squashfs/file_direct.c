/*
 * Copyright (c) 2013
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * This work is licensed under the terms of the GNU GPL, version 2. See
 * the COPYING file in the top-level directory.
 */

#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/mutex.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs_fs_i.h"
#include "squashfs.h"
#include "page_actor.h"

<<<<<<< HEAD
static void release_actor_pages(struct page **page, int pages, int error)
{
	int i;

	for (i = 0; i < pages; i++) {
		if (!page[i])
			continue;
		flush_dcache_page(page[i]);
		if (!error)
			SetPageUptodate(page[i]);
		else {
			SetPageError(page[i]);
			zero_user_segment(page[i], 0, PAGE_SIZE);
		}
		unlock_page(page[i]);
		put_page(page[i]);
	}
	kfree(page);
}
=======
static int squashfs_read_cache(struct page *target_page, u64 block, int bsize,
	int pages, struct page **page, int bytes);

/* Read separately compressed datablock directly into page cache */
int squashfs_readpage_block(struct page *target_page, u64 block, int bsize,
	int expected)
>>>>>>> refs/rewritten/Merge-4.14.113-into-android-4.14-q-2

{
	struct inode *inode = target_page->mapping->host;
	struct squashfs_sb_info *msblk = inode->i_sb->s_fs_info;

	int file_end = (i_size_read(inode) - 1) >> PAGE_SHIFT;
	int mask = (1 << (msblk->block_log - PAGE_SHIFT)) - 1;
	int start_index = target_page->index & ~mask;
	int end_index = start_index | mask;
	int i, n, pages, missing_pages, bytes, res = -ENOMEM;
	struct page **page;
	struct squashfs_page_actor *actor;
	void *pageaddr;

	if (end_index > file_end)
		end_index = file_end;

	pages = end_index - start_index + 1;

	page = kmalloc_array(pages, sizeof(void *), GFP_KERNEL);
	if (page == NULL)
		return res;

	/*
	 * Create a "page actor" which will kmap and kunmap the
	 * page cache pages appropriately within the decompressor
	 */
	actor = squashfs_page_actor_init_special(page, pages, 0);
	if (actor == NULL)
		goto out;

	/* Try to grab all the pages covered by the Squashfs block */
	for (missing_pages = 0, i = 0, n = start_index; i < pages; i++, n++) {
		page[i] = (n == target_page->index) ? target_page :
			grab_cache_page_nowait(target_page->mapping, n);

		if (page[i] == NULL) {
			missing_pages++;
			continue;
		}

		if (PageUptodate(page[i])) {
			unlock_page(page[i]);
			put_page(page[i]);
			page[i] = NULL;
			missing_pages++;
		}
	}

<<<<<<< HEAD
	actor = squashfs_page_actor_init(page, actor_pages, 0,
			release_actor_pages);
	if (!actor) {
		release_actor_pages(page, actor_pages, -ENOMEM);
		kfree(page);
		return NULL;
	}
	return actor;
=======
	if (missing_pages) {
		/*
		 * Couldn't get one or more pages, this page has either
		 * been VM reclaimed, but others are still in the page cache
		 * and uptodate, or we're racing with another thread in
		 * squashfs_readpage also trying to grab them.  Fall back to
		 * using an intermediate buffer.
		 */
		res = squashfs_read_cache(target_page, block, bsize, pages,
							page, expected);
		if (res < 0)
			goto mark_errored;

		goto out;
	}

	/* Decompress directly into the page cache buffers */
	res = squashfs_read_data(inode->i_sb, block, bsize, NULL, actor);
	if (res < 0)
		goto mark_errored;

	if (res != expected) {
		res = -EIO;
		goto mark_errored;
	}

	/* Last page may have trailing bytes not filled */
	bytes = res % PAGE_SIZE;
	if (bytes) {
		pageaddr = kmap_atomic(page[pages - 1]);
		memset(pageaddr + bytes, 0, PAGE_SIZE - bytes);
		kunmap_atomic(pageaddr);
	}

	/* Mark pages as uptodate, unlock and release */
	for (i = 0; i < pages; i++) {
		flush_dcache_page(page[i]);
		SetPageUptodate(page[i]);
		unlock_page(page[i]);
		if (page[i] != target_page)
			put_page(page[i]);
	}

	kfree(actor);
	kfree(page);

	return 0;

mark_errored:
	/* Decompression failed, mark pages as errored.  Target_page is
	 * dealt with by the caller
	 */
	for (i = 0; i < pages; i++) {
		if (page[i] == NULL || page[i] == target_page)
			continue;
		flush_dcache_page(page[i]);
		SetPageError(page[i]);
		unlock_page(page[i]);
		put_page(page[i]);
	}

out:
	kfree(actor);
	kfree(page);
	return res;
>>>>>>> refs/rewritten/Merge-4.14.113-into-android-4.14-q-2
}


<<<<<<< HEAD
{
	struct squashfs_page_actor *actor;
	struct inode *inode = mapping->host;
	struct squashfs_sb_info *msblk = inode->i_sb->s_fs_info;
	int start_index, end_index, file_end, actor_pages, res;
	int mask = (1 << (msblk->block_log - PAGE_SHIFT)) - 1;

	/*
	 * If readpage() is called on an uncompressed datablock, we can just
	 * read the pages instead of fetching the whole block.
	 * This greatly improves the performance when a process keep doing
	 * random reads because we only fetch the necessary data.
	 * The readahead algorithm will take care of doing speculative reads
	 * if necessary.
	 * We can't read more than 1 block even if readahead provides use more
	 * pages because we don't know yet if the next block is compressed or
	 * not.
	 */
	if (bsize && !SQUASHFS_COMPRESSED_BLOCK(bsize)) {
		u64 block_end = block + msblk->block_size;

		block += (page_index & mask) * PAGE_SIZE;
		actor_pages = (block_end - block) / PAGE_SIZE;
		if (*nr_pages < actor_pages)
			actor_pages = *nr_pages;
		start_index = page_index;
		bsize = min_t(int, bsize, (PAGE_SIZE * actor_pages)
					  | SQUASHFS_COMPRESSED_BIT_BLOCK);
	} else {
		file_end = (i_size_read(inode) - 1) >> PAGE_SHIFT;
		start_index = page_index & ~mask;
		end_index = start_index | mask;
		if (end_index > file_end)
			end_index = file_end;
		actor_pages = end_index - start_index + 1;
=======
static int squashfs_read_cache(struct page *target_page, u64 block, int bsize,
	int pages, struct page **page, int bytes)
{
	struct inode *i = target_page->mapping->host;
	struct squashfs_cache_entry *buffer = squashfs_get_datablock(i->i_sb,
						 block, bsize);
	int res = buffer->error, n, offset = 0;

	if (res) {
		ERROR("Unable to read page, block %llx, size %x\n", block,
			bsize);
		goto out;
	}

	for (n = 0; n < pages && bytes > 0; n++,
			bytes -= PAGE_SIZE, offset += PAGE_SIZE) {
		int avail = min_t(int, bytes, PAGE_SIZE);

		if (page[n] == NULL)
			continue;

		squashfs_fill_page(page[n], buffer, offset, avail);
		unlock_page(page[n]);
		if (page[n] != target_page)
			put_page(page[n]);
>>>>>>> refs/rewritten/Merge-4.14.113-into-android-4.14-q-2
	}

out:
	squashfs_cache_put(buffer);
	return res;
}
