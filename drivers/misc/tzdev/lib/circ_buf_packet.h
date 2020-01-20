/*
 * lib/circ_buf_packet.h
 *
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
 *
 * Circular buffer packet operations.
 */

#ifndef __LIB_CIRC_BUF_PACKET_H__
#define __LIB_CIRC_BUF_PACKET_H__

/**
 * Function returns required size of buffer to fit packet's payload of specified size.
 * @param[in]	size	packet's payload size
 * @return	size of buffer sufficient for transmitting of packet
 */
size_t circ_buf_size_for_packet(size_t size);

/**
 * Function writes packet with the specified buffer to circ_buf.
 * Function updates circ_buf write counter.
 * @param[in]	circ_buf_desc	descriptor of buffer to write buf to
 * @param[in]	buf		input buf to write to buffer
 * @param[in]	length		amount of bytes to write
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory
 * @return	number of bytes written on success or error code
 */
ssize_t circ_buf_write_packet(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function writes packet with the specified buffer to circ_buf.
 * Function does not update circ_buf write counter.
 * @param[in]	circ_buf_desc	descriptor of buffer to write buf to
 * @param[in]	buf		input buf to write to buffer
 * @param[in]	length		amount of bytes to write
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory
 * @return	number of bytes written on success or error code
 */
ssize_t circ_buf_write_packet_local(struct circ_buf_desc *circ_buf_desc, const char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function reads packet from circ_buf to output buf.
 * Function updates circ_buf read counter.
 * @param[in]	circ_buf_desc	descriptor of buffer to read from
 * @param[in]	buf		output buf to read from buffer
 * @param[in]	length		maximum size of message to read
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory,
 *				or contents of circ_buf must be simply dropped.
* @return	number of bytes read on success or error code
 */
ssize_t circ_buf_read_packet(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function reads packet from circ_buf to output buf.
 * Function does not update circ_buf read counter.
 * @param[in]	circ_buf_desc	descriptor of buffer to read from
 * @param[in]	buf		output buf to read from buffer
 * @param[in]	length		maximum size of message to read
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory,
 *				or contents of circ_buf must be simply dropped.
* @return	number of bytes read on success or error code
 */
ssize_t circ_buf_read_packet_local(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, enum circ_buf_user_mode mode);

/**
 * Function drops one packet from the circ_buf.
 * Function updates circ_buf read counter.
 * @param[in]	circ_buf_desc	descriptor of buffer to drop packet in
 * @return	number of bytes dropped on success or error code
 */
ssize_t circ_buf_drop_packet(struct circ_buf_desc *circ_buf_desc);

/**
 * Function drops one packet from the circ_buf.
 * Function does not update circ_buf read counter.
 * @param[in]	circ_buf_desc	descriptor of buffer to drop packet in
 * @return	number of bytes dropped on success or error code
 */
ssize_t circ_buf_drop_packet_local(struct circ_buf_desc *circ_buf_desc);

/**
 * Function checks if there is an available packet in the circ_buf,
 * and copies its contents by specified offset and length. Function does not
 * modify any counters and leaves circ_buf in the state it was before call.
 * @param[in]	circ_buf_desc	descriptor of buffer to read from
 * @param[in]	buf		output buf to read from buffer
 * @param[in]	length		number of bytes to read
 * @param[in]	offset		offset in packet to read from
 * @param[in]	mode		enum indicating whether passed buf
 *				belongs to userspace or kernel memory,
 *				or contents of circ_buf must be simply dropped.
 * @return	number of bytes read on success or error code
 */
ssize_t circ_buf_peek_packet(struct circ_buf_desc *circ_buf_desc, char *buf,
		size_t length, size_t offset, enum circ_buf_user_mode mode);

#endif /* __LIB_CIRC_BUF_PACKET_H__ */
