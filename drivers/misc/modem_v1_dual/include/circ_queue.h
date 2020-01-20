/**
@file		circ_queue.h
@brief		header file for general circular queue operations
@date		2014/02/18
@author		Hankook Jang (hankook.jang@samsung.com)
*/

/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_CIRCULAR_QUEUE_H__
#define __MODEM_CIRCULAR_QUEUE_H__

#include <linux/spinlock.h>
#include <linux/types.h>

#define GROUP_CIRC_QUEUE

#ifdef GROUP_CIRC_QUEUE
/**
@defgroup group_circ_queue	Circular Queue
@{
*/

/**
@brief		the structure for a circular queue in a memory-type interface
*/
struct circ_queue {
	spinlock_t lock;

	/**
	 * the flag and counter for checking busy status of a circualr queue
	 */
	atomic_t busy;

	/**
	 * the start address of the data buffer in a circualr queue
	 */
	void __iomem *buff;

	/**
	 * the size of the data buffer in a circular queue
	 */
	unsigned int size;

	/**
	 * the pointer to the "HEAD (IN)" variable that contains a byte offset
	 * from @b @@buff
	 */
	void __iomem *head;

	/**
	 * the pointer to the "TAIL (OUT)" variable that contains a byte offset
	 * from @b @@buff
	 */
	void __iomem *tail;
};

/**
@brief		get the start address of the data buffer in a circular queue

@param q	the pointer to a circular queue

@return		the start address of the data buffer in the @e @@q
*/
static inline char *get_buff(struct circ_queue *q)
{
	return q->buff;
}

/**
@brief		get the size of the data buffer in a circular queue

@param q	the pointer to a circular queue

@return		the size of the data buffer in the @e @@q
*/
static inline unsigned int get_size(struct circ_queue *q)
{
	return q->size;
}

/**
@brief		get the "HEAD (IN)" pointer value of a circular queue

@param q	the pointer to a circular queue

@return		the "HEAD (IN)" pointer value of the @e @@q
*/
static inline unsigned int get_head(struct circ_queue *q)
{
	return ioread32(q->head);
}

/**
@brief		set the "HEAD (IN)" pointer value of a circular queue with @b
		@@in

@param q	the pointer to a circular queue
@param in	the value to be stored into the "HEAD (IN)" pointer
*/
static inline void set_head(struct circ_queue *q, unsigned int in)
{
	iowrite32(in, q->head);
}

/**
@brief		get the "TAIL (OUT)" pointer value of a circular queue

@param q	the pointer to a circular queue

@return		the "TAIL (OUT)" pointer value of the @e @@q
*/
static inline unsigned int get_tail(struct circ_queue *q)
{
	return ioread32(q->tail);
}

/**
@brief		set the "TAIL (OUT)" pointer value of a circular queue with @e
		@@out

@param q	the pointer to a circular queue
@param out	the value to be stored into the "TAIL (OUT)" pointer
*/
static inline void set_tail(struct circ_queue *q, unsigned int out)
{
	iowrite32(out, q->tail);
}

/**
@brief		check whether or not both "IN" and "OUT" pointer values are
		valid

@param qsize	the size of the data buffer in a circular queue
@param in	the value of the "HEAD (IN)" pointer
@param out	the value of the "TAIL (OUT)" pointer

@retval "true"	if all pointer values are valid
@retval "false"	if either IN or OUT pointer value is NOT valid
*/
static inline bool circ_valid(unsigned int qsize,
			      unsigned int in,
			      unsigned int out)
{
	if (unlikely(in >= qsize))
		return false;

	if (unlikely(out >= qsize))
		return false;

	return true;
}

/**
@brief		check whether or not a circular queue is empty

@param in	the value of the "HEAD (IN)" pointer
@param out	the value of the "TAIL (OUT)" pointer

@retval "true"	if a circular queue is empty
@retval "false"	if a circular queue is NOT empty
*/
static inline bool circ_empty(unsigned int in, unsigned int out)
{
	return (in == out);
}

/**
@brief		get the size of free space in a circular queue

@param qsize	the size of the data buffer in a circular queue
@param in	the value of the "HEAD (IN)" pointer
@param out	the value of the "TAIL (OUT)" pointer

@return		the size of free space in a circular queue
*/
static inline unsigned int circ_get_space(unsigned int qsize,
					  unsigned int in,
					  unsigned int out)
{
	return (in < out) ? (out - in - 1) : (qsize + out - in - 1);
}

static inline bool circ_full(unsigned int qsize, unsigned int in,
			     unsigned int out)
{
	return (circ_get_space(qsize, in, out) == 0);
}

/**
@brief		get the size of data in a circular queue

@param qsize	the size of the data buffer in a circular queue
@param in the	value of the "HEAD (IN)" pointer
@param out the	value of the "TAIL (OUT)" pointer

@return		the size of data in a circular queue
*/
static inline unsigned int circ_get_usage(unsigned int qsize,
					  unsigned int in,
					  unsigned int out)
{
	return (in >= out) ? (in - out) : (qsize - out + in);
}

/**
@brief		calculate a new pointer value for a circular queue

@param qsize	the size of the data buffer in a circular queue
@param p	the old value of a queue pointer
@param len	the length to be added to the @e @@p pointer value

@return		the new value for the queue pointer
*/
static inline unsigned int circ_new_ptr(unsigned int qsize,
					unsigned int p,
					unsigned int len)
{
	unsigned int np = (p + len);
	while (np >= qsize)
		np -= qsize;
	return np;
}

/**
@brief		copy the data in a circular queue to a local buffer

@param dst	the start address of the local buffer
@param src	the start address of the data buffer in a circular queue
@param qsize	the size of the data buffer in a circular queue
@param out	the offset in the data buffer to be read
@param len	the length of data to be read

@remark		This function should be invoked after checking the data length.
*/
static inline void circ_read(u8 *dst, u8 *src, unsigned int qsize,
			     unsigned int out, unsigned int len)
{
	if ((out + len) <= qsize) {
		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */
		memcpy(dst, (src + out), len);
	} else {
		unsigned int len1;

		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */

		/* 1) data start (out) ~ buffer end */
		len1 = qsize - out;
		memcpy(dst, (src + out), len1);

		/* 2) buffer start ~ data end (in?) */
		memcpy((dst + len1), src, (len - len1));
	}
}

/**
@brief		copy the data in a local buffer to a circular queue

@param dst	the start address of the data buffer in a circular queue
@param src	the start address of the data in a local buffer
@param qsize	the size of the data buffer in a circular queue
@param in	the offset in the data buffer for the data to be stored
@param len	the length of data to be stored

@remark		This function should be invoked after checking the free space.
*/
static inline void circ_write(u8 *dst, u8 *src, unsigned int qsize,
			      unsigned int in, unsigned int len)
{
	if ((in + len) < qsize) {
		/*       (in) ----------- (out)   */
		/* 00 7e      -----------   7f 00 */
		memcpy((dst + in), src, len);
	} else {
		unsigned int space;

		/* ----- (out)         (in) ----- */
		/* -----   7f 00 00 7e      ----- */

		/* 1) space start (in) ~ buffer end */
		space = qsize - in;
		memcpy((dst + in), src, ((len > space) ? space : len));

		/* 2) buffer start ~ data end */
		if (len > space)
			memcpy(dst, (src + space), (len - space));
	}
}

/**
// End of group_circ_queue
@}
*/
#endif

#endif
