/*
 * lib/circ_buf_packet.c
 *
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
 *
 * Circular buffer packet operations.
 */

#include <linux/err.h>
#include "circ_buf.h"
#include "circ_buf_packet.h"

struct circ_buf_packet_header {
	uint32_t packet_size;
} __packed;

size_t circ_buf_size_for_packet(size_t size)
{
	return size + sizeof(struct circ_buf_packet_header);
}

ssize_t circ_buf_write_packet(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	ssize_t ret;

	ret = circ_buf_write_packet_local(circ_buf_desc, buf, length, mode);
	if (!IS_ERR_VALUE(ret))
		circ_buf_flush_write(circ_buf_desc);

	return ret;
}

ssize_t circ_buf_write_packet_local(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	struct circ_buf_packet_header header;
	ssize_t ret;
	unsigned int write_count_orig = circ_buf_desc->write_count;

	header.packet_size = (unsigned int)length;

	ret = circ_buf_write_local(circ_buf_desc, (void *) &header,
			sizeof(struct circ_buf_packet_header), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	ret = circ_buf_write_local(circ_buf_desc, buf, length, mode);
	if (IS_ERR_VALUE(ret))
		circ_buf_desc->write_count = write_count_orig;

	return ret;
}

ssize_t circ_buf_read_packet(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	ssize_t ret;

	ret = circ_buf_read_packet_local(circ_buf_desc, buf, length, mode);
	if (!IS_ERR_VALUE(ret))
		circ_buf_flush_read(circ_buf_desc);

	return ret;
}

ssize_t circ_buf_read_packet_local(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode)
{
	struct circ_buf_packet_header header;
	ssize_t ret;
	size_t packet_size;
	unsigned int read_count_orig = circ_buf_desc->read_count;

	ret = circ_buf_read_local(circ_buf_desc, (void *) &header,
			sizeof(struct circ_buf_packet_header), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	packet_size = header.packet_size;
	if (packet_size > length) {
		circ_buf_desc->read_count = read_count_orig;
		return -EMSGSIZE;
	}

	ret = circ_buf_read_local(circ_buf_desc, buf, packet_size, mode);
	if (IS_ERR_VALUE(ret))
		circ_buf_desc->read_count = read_count_orig;

	return ret;
}

ssize_t circ_buf_drop_packet(struct circ_buf_desc *circ_buf_desc)
{
	ssize_t ret;

	ret = circ_buf_drop_packet_local(circ_buf_desc);
	if (!IS_ERR_VALUE(ret))
		circ_buf_flush_read(circ_buf_desc);

	return ret;
}

ssize_t circ_buf_drop_packet_local(struct circ_buf_desc *circ_buf_desc)
{
	struct circ_buf_packet_header header;
	ssize_t ret;
	unsigned int read_count_orig = circ_buf_desc->read_count;

	ret = circ_buf_read_local(circ_buf_desc, (void *) &header,
			sizeof(struct circ_buf_packet_header), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	ret = circ_buf_read_local(circ_buf_desc, NULL, header.packet_size,
			CIRC_BUF_MODE_DROP);
	if (IS_ERR_VALUE(ret))
		circ_buf_desc->read_count = read_count_orig;

	return ret;
}

ssize_t circ_buf_peek_packet(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, size_t offset, enum circ_buf_user_mode mode)
{
	struct circ_buf_packet_header header;
	ssize_t ret;
	unsigned int read_count_orig = circ_buf_desc->read_count;

	ret = circ_buf_read_local(circ_buf_desc, (void *) &header,
			sizeof(struct circ_buf_packet_header), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	if (offset + length > header.packet_size) {
		ret = -EMSGSIZE;
		goto out;
	}

	ret = __circ_buf_read_local(circ_buf_desc, buf, length, offset, mode);

out:
	circ_buf_desc->read_count = read_count_orig;

	return ret;
}
