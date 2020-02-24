/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * block.c
 */

/*
 * This file implements the low-level routines to read and decompress
 * datablocks and metadata blocks.
 */

#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs.h"
#include "decompressor.h"
#include "page_actor.h"

/*
 * Read the metadata block length, this is stored in the first two
 * bytes of the metadata block.
 */
static struct buffer_head *get_block_length(struct super_block *sb,
			u64 *cur_index, int *offset, int *length)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	struct buffer_head *bh;

	bh = sb_bread(sb, *cur_index);
	if (bh == NULL)
		return NULL;

	if (msblk->devblksize - *offset == 1) {
		*length = (unsigned char) bh->b_data[*offset];
		put_bh(bh);
		bh = sb_bread(sb, ++(*cur_index));
		if (bh == NULL)
			return NULL;
		*length |= (unsigned char) bh->b_data[0] << 8;
		*offset = 1;
	} else {
		*length = (unsigned char) bh->b_data[*offset] |
			(unsigned char) bh->b_data[*offset + 1] << 8;
		*offset += 2;

		if (*offset == msblk->devblksize) {
			put_bh(bh);
			bh = sb_bread(sb, ++(*cur_index));
			if (bh == NULL)
				return NULL;
			*offset = 0;
		}
	}

	return bh;
}


/*
 * Read and decompress a metadata block or datablock.  Length is non-zero
 * if a datablock is being read (the size is stored elsewhere in the
 * filesystem), otherwise the length is obtained from the first two bytes of
 * the metadata block.  A bit in the length field indicates if the block
 * is stored uncompressed in the filesystem (usually because compression
 * generated a larger block - this does occasionally happen with compression
 * algorithms).
 */
int squashfs_read_data(struct super_block *sb, u64 index, int length,
		u64 *next_index, struct squashfs_page_actor *output)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	struct buffer_head **bh;
	int offset = index & ((1 << msblk->devblksize_log2) - 1);
	u64 cur_index = index >> msblk->devblksize_log2;
	int bytes, compressed, b = 0, k = 0, avail, i;

	bh = kcalloc(((output->length + msblk->devblksize - 1)
		>> msblk->devblksize_log2) + 1, sizeof(*bh), GFP_KERNEL);
	if (bh == NULL)
		return -ENOMEM;

	if (length) {
		/*
		 * Datablock.
		 */
		bytes = -offset;
		compressed = SQUASHFS_COMPRESSED_BLOCK(length);
		length = SQUASHFS_COMPRESSED_SIZE_BLOCK(length);
		if (next_index)
			*next_index = index + length;

		TRACE("Block @ 0x%llx, %scompressed size %d, src size %d\n",
			index, compressed ? "" : "un", length, output->length);

		if (length < 0 || length > output->length ||
				(index + length) > msblk->bytes_used)
			goto read_failure;

		for (b = 0; bytes < length; b++, cur_index++) {
			bh[b] = sb_getblk(sb, cur_index);
			if (bh[b] == NULL)
				goto block_release;
			bytes += msblk->devblksize;
		}
		ll_rw_block(REQ_OP_READ, 0, b, bh);
	} else {
		/*
		 * Metadata block.
		 */
		if ((index + 2) > msblk->bytes_used)
			goto read_failure;

<<<<<<< HEAD
		/* Otherwise, submit the current BIO and create a new one */
		if (bio)
			submit_bio(bio);
		bio_req = kcalloc(1, sizeof(struct squashfs_bio_request),
				  GFP_NOIO);
		if (!bio_req)
			goto req_alloc_failed;
		bio_req->bh = &req->bh[b];
		bio = bio_alloc(GFP_NOIO, bio_max_pages);
		if (!bio)
			goto bio_alloc_failed;
		bio_set_dev(bio, req->sb->s_bdev);
		bio->bi_iter.bi_sector = (block + b)
				       << (msblk->devblksize_log2 - 9);
		bio_set_op_attrs(bio, REQ_OP_READ, 0);
		bio->bi_private = bio_req;
		bio->bi_end_io = squashfs_bio_end_io;

		bio_add_page(bio, bh->b_page, blksz, offset);
		bio_req->nr_buffers += 1;
		prev_block = b;
	}
	if (bio)
		submit_bio(bio);

	if (req->synchronous)
		squashfs_process_blocks(req);
	else {
		INIT_WORK(&req->offload, read_wq_handler);
		schedule_work(&req->offload);
	}
	return 0;

bio_alloc_failed:
	kfree(bio_req);
req_alloc_failed:
	unlock_buffer(bh);
	while (--nr_buffers >= b)
		if (req->bh[nr_buffers])
			put_bh(req->bh[nr_buffers]);
	while (--b >= 0)
		if (req->bh[b])
			wait_on_buffer(req->bh[b]);
getblk_failed:
	free_read_request(req, -ENOMEM);
	return -ENOMEM;
}

static int read_metadata_block(struct squashfs_read_request *req,
			       u64 *next_index)
{
	int ret, error, bytes_read = 0, bytes_uncompressed = 0;
	struct squashfs_sb_info *msblk = req->sb->s_fs_info;

	if (req->index + 2 > msblk->bytes_used) {
		free_read_request(req, -EINVAL);
		return -EINVAL;
	}
	req->length = 2;

	/* Do not read beyond the end of the device */
	if (req->index + req->length > msblk->bytes_used)
		req->length = msblk->bytes_used - req->index;
	req->data_processing = SQUASHFS_METADATA;

	/*
	 * Reading metadata is always synchronous because we don't know the
	 * length in advance and the function is expected to update
	 * 'next_index' and return the length.
	 */
	req->synchronous = true;
	req->res = &error;
	req->bytes_read = &bytes_read;
	req->bytes_uncompressed = &bytes_uncompressed;

	TRACE("Metadata block @ 0x%llx, %scompressed size %d, src size %d\n",
	      req->index, req->compressed ? "" : "un", bytes_read,
	      req->output->length);
=======
		bh[0] = get_block_length(sb, &cur_index, &offset, &length);
		if (bh[0] == NULL)
			goto read_failure;
		b = 1;
>>>>>>> refs/rewritten/Merge-4.14.113-into-android-4.14-q-2

		bytes = msblk->devblksize - offset;
		compressed = SQUASHFS_COMPRESSED(length);
		length = SQUASHFS_COMPRESSED_SIZE(length);
		if (next_index)
			*next_index = index + length + 2;

		TRACE("Block @ 0x%llx, %scompressed size %d\n", index,
				compressed ? "" : "un", length);

		if (length < 0 || length > output->length ||
					(index + length) > msblk->bytes_used)
			goto block_release;

		for (; bytes < length; b++) {
			bh[b] = sb_getblk(sb, ++cur_index);
			if (bh[b] == NULL)
				goto block_release;
			bytes += msblk->devblksize;
		}
		ll_rw_block(REQ_OP_READ, 0, b - 1, bh + 1);
	}

	for (i = 0; i < b; i++) {
		wait_on_buffer(bh[i]);
		if (!buffer_uptodate(bh[i]))
			goto block_release;
	}

<<<<<<< HEAD
	req->sb = sb;
	req->index = index;
	req->output = output;

	if (next_index)
		*next_index = index;

	if (length)
		length = read_data_block(req, length, next_index, sync);
	else
		length = read_metadata_block(req, next_index);

	if (length < 0) {
		ERROR("squashfs_read_data failed to read block 0x%llx\n",
		      (unsigned long long)index);
		return -EIO;
=======
	if (compressed) {
		if (!msblk->stream)
			goto read_failure;
		length = squashfs_decompress(msblk, bh, b, offset, length,
			output);
		if (length < 0)
			goto read_failure;
	} else {
		/*
		 * Block is uncompressed.
		 */
		int in, pg_offset = 0;
		void *data = squashfs_first_page(output);

		for (bytes = length; k < b; k++) {
			in = min(bytes, msblk->devblksize - offset);
			bytes -= in;
			while (in) {
				if (pg_offset == PAGE_SIZE) {
					data = squashfs_next_page(output);
					pg_offset = 0;
				}
				avail = min_t(int, in, PAGE_SIZE -
						pg_offset);
				memcpy(data + pg_offset, bh[k]->b_data + offset,
						avail);
				in -= avail;
				pg_offset += avail;
				offset += avail;
			}
			offset = 0;
			put_bh(bh[k]);
		}
		squashfs_finish_page(output);
>>>>>>> refs/rewritten/Merge-4.14.113-into-android-4.14-q-2
	}

	kfree(bh);
	return length;

block_release:
	for (; k < b; k++)
		put_bh(bh[k]);

read_failure:
	ERROR("squashfs_read_data failed to read block 0x%llx\n",
					(unsigned long long) index);
	kfree(bh);
	return -EIO;
}
