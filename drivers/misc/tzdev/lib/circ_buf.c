/*
 * Copyright (C) 2012-2017 Samsung Electronics, Inc.
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

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "circ_buf.h"

struct circ_buf_desc *circ_buf_create(unsigned long size)
{
	struct circ_buf *circ_buf;
	struct circ_buf_desc *circ_buf_desc;

	circ_buf_desc = circ_buf_desc_alloc();
	if (!circ_buf_desc)
		return NULL;

	circ_buf = circ_buf_alloc(size);
	if (!circ_buf) {
		circ_buf_desc_free(circ_buf_desc);
		return NULL;
	}

	circ_buf_connect(circ_buf_desc, circ_buf, size);

	return circ_buf_desc;
}

void circ_buf_destroy(struct circ_buf_desc *circ_buf_desc)
{
	circ_buf_free(circ_buf_desc->circ_buf);
	circ_buf_desc_free(circ_buf_desc);
}

struct circ_buf_desc *circ_buf_desc_alloc(void)
{
	struct circ_buf_desc *circ_buf_desc;

	circ_buf_desc = kmalloc(sizeof(struct circ_buf_desc), GFP_KERNEL);
	if (!circ_buf_desc)
		return NULL;

	circ_buf_desc->circ_buf = NULL;
	circ_buf_desc->write_count = 0;
	circ_buf_desc->read_count = 0;
	circ_buf_desc->size = 0;

	return circ_buf_desc;
}

void circ_buf_desc_free(struct circ_buf_desc *circ_buf_desc)
{
	kfree(circ_buf_desc);
}

struct circ_buf *circ_buf_alloc(unsigned long size)
{
	struct circ_buf *circ_buf;

	circ_buf = kmalloc(size + CIRC_BUF_META_SIZE, GFP_KERNEL);
	if (!circ_buf)
		return NULL;

	circ_buf_init(circ_buf);

	return circ_buf;
}

struct circ_buf *circ_buf_set(void *buf)
{
	struct circ_buf *circ_buf = buf;

	circ_buf_init(circ_buf);

	return circ_buf;
}

void circ_buf_free(struct circ_buf *circ_buf)
{
	kfree(circ_buf);
}

void circ_buf_init(struct circ_buf *circ_buf)
{
	circ_buf->write_count = 0;
	circ_buf->read_count = 0;
}

void circ_buf_connect(struct circ_buf_desc *circ_buf_desc,
		struct circ_buf *circ_buf, unsigned long size)
{
	circ_buf_desc->circ_buf = circ_buf;
	circ_buf_desc->read_count = circ_buf->read_count;
	circ_buf_desc->write_count = circ_buf->write_count;
	circ_buf_desc->size = (unsigned int)size;
}

ssize_t circ_buf_write(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	ssize_t ret;

	ret = circ_buf_write_local(circ_buf_desc, buf, length, mode);
	if (!IS_ERR_VALUE(ret))
		circ_buf_flush_write(circ_buf_desc);

	return ret;
}

static int circ_buf_write_contig(struct circ_buf_desc *circ_buf_desc,
		const char *buf, size_t length, enum circ_buf_user_mode mode)
{
	int ret = 0;
	struct circ_buf *circ_buf = circ_buf_desc->circ_buf;

	switch (mode) {
	case CIRC_BUF_MODE_KERNEL:
		memcpy(circ_buf->buffer + circ_buf_desc->write_count, buf, length);
		break;
	case CIRC_BUF_MODE_USER:
		if (copy_from_user(circ_buf->buffer + circ_buf_desc->write_count,
				buf, length))
			ret = -EFAULT;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static unsigned int circ_buf_is_count_valid(struct circ_buf_desc *circ_buf_desc,
		unsigned int count)
{
	return count <= circ_buf_desc->size;
}

ssize_t circ_buf_write_local(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	size_t avail, cur = 0;
	unsigned int size = circ_buf_desc->size;
	unsigned int end = circ_buf_desc->size + 1;
	unsigned int new_count;
	ssize_t ret;
	size_t cur_length = length;
	size_t bytes_used;
	unsigned int write_count_orig = circ_buf_desc->write_count;
	unsigned int write_count_shared = circ_buf_desc->circ_buf->write_count;
	unsigned int read_count_shared = circ_buf_desc->circ_buf->read_count;

	if (mode == CIRC_BUF_MODE_DROP)
		return -EINVAL;

	if (!circ_buf_is_count_valid(circ_buf_desc, write_count_shared)
			|| !circ_buf_is_count_valid(circ_buf_desc, read_count_shared))
		return -EIO;

	circ_buf_desc->read_count = read_count_shared;

	if (circ_buf_desc->write_count >= circ_buf_desc->read_count) {
		avail = size - (circ_buf_desc->write_count - circ_buf_desc->read_count);
		bytes_used = write_count_shared - circ_buf_desc->read_count;
	} else {
		avail = circ_buf_desc->read_count - circ_buf_desc->write_count - 1;
		bytes_used = end - circ_buf_desc->read_count + write_count_shared;
	}

	if (avail + bytes_used < cur_length) {
		ret = -EMSGSIZE;
		goto fail;
	}

	if (avail < cur_length) {
		ret = -EAGAIN;
		goto fail;
	}

	if (circ_buf_desc->write_count >= circ_buf_desc->read_count) {
		avail = end - circ_buf_desc->write_count - (circ_buf_desc->read_count ? 0 : 1);

		cur = min(cur_length, avail);

		ret = circ_buf_write_contig(circ_buf_desc, buf, cur, mode);
		if (ret)
			goto fail;

		new_count = (unsigned int)(circ_buf_desc->write_count + cur);
		circ_buf_desc->write_count = (new_count == end ? 0 : new_count);
		buf += cur;
		cur_length -= cur;
	}

	ret = circ_buf_write_contig(circ_buf_desc, buf, cur_length, mode);
	if (ret)
		goto fail;

	circ_buf_desc->write_count += (unsigned int)cur_length;

	return length;

fail:
	circ_buf_desc->write_count = write_count_orig;

	return ret;
}

ssize_t circ_buf_read(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	ssize_t ret;

	ret = circ_buf_read_local(circ_buf_desc, buf, length, mode);
	if (!IS_ERR_VALUE(ret))
		circ_buf_flush_read(circ_buf_desc);

	return ret;
}

ssize_t circ_buf_read_local(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	return __circ_buf_read_local(circ_buf_desc, buf, length, 0, mode);
}

static int circ_buf_read_contig(struct circ_buf_desc *circ_buf_desc,
		char *buf, size_t length, enum circ_buf_user_mode mode)
{
	int ret = 0;
	struct circ_buf *circ_buf = circ_buf_desc->circ_buf;

	switch(mode) {
	case CIRC_BUF_MODE_KERNEL:
		memcpy(buf, circ_buf->buffer + circ_buf_desc->read_count, length);
		break;
	case CIRC_BUF_MODE_USER:
		if (copy_to_user(buf, circ_buf->buffer + circ_buf_desc->read_count, length))
			ret = -EFAULT;
		break;
	case CIRC_BUF_MODE_DROP:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

ssize_t __circ_buf_read_local(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, size_t offset, enum circ_buf_user_mode mode)
{
	size_t avail, cur = 0;
	unsigned int end = circ_buf_desc->size + 1;
	unsigned int new_count;
	ssize_t ret;
	size_t cur_length = length;
	size_t bytes_free;
	unsigned int read_count_orig = circ_buf_desc->read_count;
	unsigned int write_count_shared = circ_buf_desc->circ_buf->write_count;
	unsigned int read_count_shared = circ_buf_desc->circ_buf->read_count;

	if (!circ_buf_is_count_valid(circ_buf_desc, write_count_shared)
			|| !circ_buf_is_count_valid(circ_buf_desc, read_count_shared))
		return -EIO;

	circ_buf_desc->write_count = write_count_shared;
	circ_buf_desc->read_count += (unsigned int)offset;

	if (circ_buf_desc->write_count >= circ_buf_desc->read_count) {
		avail = circ_buf_desc->write_count - circ_buf_desc->read_count;
		bytes_free = circ_buf_desc->size - circ_buf_desc->write_count
				+ read_count_shared;
	} else {
		avail = end - (circ_buf_desc->read_count - circ_buf_desc->write_count);
		bytes_free = read_count_shared - circ_buf_desc->write_count - 1;
	}

	if (avail + bytes_free < cur_length) {
		ret = -EMSGSIZE;
		goto fail;
	}

	if (avail < cur_length) {
		ret = -EAGAIN;
		goto fail;
	}

	smp_rmb();

	if (circ_buf_desc->write_count < circ_buf_desc->read_count) {
		avail = end - circ_buf_desc->read_count;

		cur = min(cur_length, avail);

		ret = circ_buf_read_contig(circ_buf_desc, buf, cur, mode);
		if (ret)
			goto fail;

		new_count = (unsigned int)(circ_buf_desc->read_count + cur);
		circ_buf_desc->read_count = (new_count == end ? 0 : new_count);
		buf += cur;
		cur_length -= cur;
	}

	ret = circ_buf_read_contig(circ_buf_desc, buf, cur_length, mode);
	if (ret)
		goto fail;

	circ_buf_desc->read_count += (unsigned int)cur_length;

	return length;

fail:
	circ_buf_desc->read_count = read_count_orig;

	return ret;
}

void circ_buf_flush_write(struct circ_buf_desc *circ_buf_desc)
{
	smp_wmb();

	circ_buf_desc->circ_buf->write_count = circ_buf_desc->write_count;
}

void circ_buf_flush_read(struct circ_buf_desc *circ_buf_desc)
{
	circ_buf_desc->circ_buf->read_count = circ_buf_desc->read_count;
}

void circ_buf_rollback_write(struct circ_buf_desc *circ_buf_desc)
{
	circ_buf_desc->write_count = circ_buf_desc->circ_buf->write_count;
}

void circ_buf_rollback_read(struct circ_buf_desc *circ_buf_desc)
{
	circ_buf_desc->read_count = circ_buf_desc->circ_buf->read_count;
}

unsigned int circ_buf_is_empty(struct circ_buf_desc *circ_buf_desc)
{
	return circ_buf_bytes_used(circ_buf_desc) == 0;
}

unsigned int circ_buf_is_full(struct circ_buf_desc *circ_buf_desc)
{
	return circ_buf_bytes_used(circ_buf_desc) == circ_buf_desc->size;
}

size_t circ_buf_bytes_free(struct circ_buf_desc *circ_buf_desc)
{
	return circ_buf_desc->size - circ_buf_bytes_used(circ_buf_desc);
}

size_t circ_buf_bytes_used(struct circ_buf_desc *circ_buf_desc)
{
	unsigned int write_count = circ_buf_desc->circ_buf->write_count;
	unsigned int read_count = circ_buf_desc->circ_buf->read_count;

	if (write_count >= read_count)
		return write_count - read_count;

	return circ_buf_desc->size + 1 - read_count + write_count;
}
