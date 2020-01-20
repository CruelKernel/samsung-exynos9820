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

#ifndef __LIB_CIRC_BUF_H__
#define __LIB_CIRC_BUF_H__

#include <linux/types.h>

/* One more byte is used for differentiating between empty and full buffer */
#define CIRC_BUF_EMPTY_FLAG_SIZE	1
#define CIRC_BUF_META_SIZE		(sizeof(struct circ_buf) + CIRC_BUF_EMPTY_FLAG_SIZE)

struct circ_buf {
	u32 write_count;
	u32 read_count;
	char buffer[];
} __packed;

struct circ_buf_desc {
	struct circ_buf *circ_buf;
	unsigned int size;

	/* These are local copies of circ_buf counters
	 * for tracking actual state (circ_buf counters are shared
	 * between reader and writer and thus should only be updated
	 * when new state is ok for the reader to see). */
	unsigned int write_count;
	unsigned int read_count;
};

enum circ_buf_user_mode {
	CIRC_BUF_MODE_KERNEL,
	CIRC_BUF_MODE_USER,
	CIRC_BUF_MODE_DROP
};

/**
 * Function creates circ_buf_desc structure with circ_buf of specified size.
 * @param[in]	size		size of circ_buf
 * @return	descriptor of allocated circ_buf, or NULL if allocation failed
 */
struct circ_buf_desc *circ_buf_create(unsigned long size);

/**
 * Function destroys circ_buf_desc structure along with its circ_buf.
 * @param[in]	circ_buf_desc	descriptor of buffer to destroy
 */
void circ_buf_destroy(struct circ_buf_desc *circ_buf_desc);

/**
 * Function allocates circ_buf_desc structure and initializes its fields.
 * @return	allocated circ_buf descriptor, or NULL if allocation failed
 */
struct circ_buf_desc *circ_buf_desc_alloc(void);

/**
 * Function sets circ_buf_desc structure pointing to previously allocated buffer
 * and initializes its fields.
 * @return	allocated circ_buf descriptor, or NULL if allocation failed
 */
struct circ_buf *circ_buf_set(void *buf);

/**
 * Function frees circ_buf descriptor.
 * @param[in]	circ_buf_desc	circ_buf descriptor to free
 */
void circ_buf_desc_free(struct circ_buf_desc *circ_buf_desc);

/**
 * Function allocates circ_buf of specified size.
 * @param[in]	size		size of circ_buf
 * @return	pointer to allocated circ_buf, or NULL of allocation failed
 */
struct circ_buf *circ_buf_alloc(unsigned long size);

/**
 * Function frees circ_buf.
 * @param[in]	circ_buf	circ_buf to free
 */
void circ_buf_free(struct circ_buf *circ_buf);

/**
 * Function initialized circ_buf to be empty.
 * @param[in]	circ_buf	circ_buf to initialize
 */
void circ_buf_init(struct circ_buf *circ_buf);

/**
 * Function connects circ_buf of specified size to the circ_buf desc.
 * @param[in]	circ_buf_desc	descriptor to connect circ_buf to
 * @param[in]	circ_buf	circ_buf to connect
 * @param[in]	size		size of circ_buf
 * @return	error code
 */
void circ_buf_connect(struct circ_buf_desc *circ_buf_desc,
		struct circ_buf *circ_buf, unsigned long size);

/**
 * Function writes specified buf to circ_buf and updates
 * circ_buf write counter.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to write buf to
 * @param[in]	buf		input buf to write to circ_buf
 * @param[in]	length		amount of bytes to write
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory
 * @return	number on bytes written on success or error code
 */
ssize_t circ_buf_write(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function writes specified buf to circ_buf without updating circ_buf write counter.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to write buf to
 * @param[in]	buf		input buf to write to circ_buf
 * @param[in]	length		amount of bytes to write
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory
 * @return	number of bytes written on success or error code
 */
ssize_t circ_buf_write_local(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function reads contents of circ_buf to output buf and updates
 * circ_buf read counter.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to read from
 * @param[in]	buf		output buf to read from circ_buf
 * @param[in]	length		amount of bytes to read
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory,
 *				or contents of circ_buf must be simply dropped.
 * @return	number of bytes read on success or error code
 */
ssize_t circ_buf_read(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function reads contents of circ_buf to output buf without updating circ_buf read counter.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to read from
 * @param[in]	buf		output buf to read from circ_buf
 * @param[in]	length		amount of bytes to read
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory,
 *				or contents of circ_buf must be simply dropped.
 * @return	number of bytes read on success or error code
 */
ssize_t circ_buf_read_local(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function reads contents of circ_buf by specified offset from read_count,
 * not updating circ_buf read counter. Not intended to be used by clients,
 * required for API extensions.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to read from
 * @param[in]	buf		output buf to read from circ_buf
 * @param[in]	length		amount of bytes to read
 * @param[in]	offset		offset from read_count to start reading from
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory,
 *				or contents of circ_buf must be simply dropped.
 * @return	number of bytes read on success or error code
 */
ssize_t __circ_buf_read_local(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, size_t offset, enum circ_buf_user_mode mode);

/**
 * Function updates circ_buf write counter, so reader can see new value.
 * @param[in]	circ_buf_desc 	descriptor of circ_buf to update counter in
 */
void circ_buf_flush_write(struct circ_buf_desc *circ_buf_desc);

/**
 * Function updates circ_buf read counter, so writer can see new value.
 * @param[in]	circ_buf_desc 	descriptor of circ_buf to update counter in
 */
void circ_buf_flush_read(struct circ_buf_desc *circ_buf_desc);

/**
 * Function restores circ_buf_desc write counter to the value its circ_buf has.
 * @param[in]	circ_buf_desc 	descriptor of circ_buf to update counter in
 */
void circ_buf_rollback_write(struct circ_buf_desc *circ_buf_desc);

/**
 * Function restores circ_buf_desc read counter to the value its circ_buf has.
 * @param[in]	circ_buf_desc 	descriptor of circ_buf to update counter in
 */
void circ_buf_rollback_read(struct circ_buf_desc *circ_buf_desc);

/**
 * Function checks if circ_buf is empty.
 * Should be used with care, because result is not guaranteed to be valid
 * after the access, unless there is external locking mechanism.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to check
 * @return	non-0 is circ_buf is empty and 0 otherwise
 */
unsigned int circ_buf_is_empty(struct circ_buf_desc *circ_buf_desc);

/**
 * Function checks if circ_buf is full.
 * Should be used with care, because result is not guaranteed to be valid
 * after the access, unless there is external locking mechanism.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to check
 * @return	non-0 is circ_bufis full and 0 otherwise
 */
unsigned int circ_buf_is_full(struct circ_buf_desc *circ_buf_desc);

/**
 * Function calculates amount of bytes in buffer currently available to write.
 * Should be used with care, because result is not guaranteed to be valid
 * after the access, unless there is external locking mechanism.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to check
 * @return	number of free bytes
 */
size_t circ_buf_bytes_free(struct circ_buf_desc *circ_buf_desc);

/**
 * Function calculates amount of bytes written in buffer and not read yet.
 * Should be used with care, because result is not guaranteed to be valid
 * after the access, unless there is external locking mechanism.
 * @param[in]	circ_buf_desc	descriptor of circ_buf to check
 * @return	number of used bytes
 */
size_t circ_buf_bytes_used(struct circ_buf_desc *circ_buf_desc);

#endif /* __LIB_CIRC_BUF_H__ */
