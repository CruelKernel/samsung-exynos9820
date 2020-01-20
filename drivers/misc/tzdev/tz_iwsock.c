/*
 * Copyright (C) 2012-2017, Samsung Electronics Co., Ltd.
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

#include <linux/crypto.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/socket.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>

#include "lib/circ_buf.h"
#include "lib/circ_buf_packet.h"

#include <tzdev/tzdev.h>

#include "sysdep.h"
#include "tzdev.h"
#include "tz_cred.h"
#include "tz_iwio.h"
#include "tz_iwsock.h"
#include "tz_kthread_pool.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
#include <linux/sched.h>
#else
#include <linux/sched/signal.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
#define tz_iwsock_wait_event(wq, condition)					\
	wait_event(wq, condition)

#define tz_iwsock_wait_event_interruptible(wq, condition)			\
	wait_event_interruptible(wq, condition)
#else
#define tz_iwsock_wait_event(wq, condition)					\
({										\
	DEFINE_WAIT_FUNC(wait, woken_wake_function);				\
	add_wait_queue(&wq, &wait);						\
	while (!(condition))							\
		wait_woken(&wait, TASK_UNINTERRUPTIBLE,				\
			MAX_SCHEDULE_TIMEOUT);					\
	remove_wait_queue(&wq, &wait);						\
})

#define tz_iwsock_wait_event_interruptible(wq, condition)			\
({										\
	DEFINE_WAIT_FUNC(wait, woken_wake_function);				\
	long __ret = 0;								\
	add_wait_queue(&wq, &wait);						\
	while (!(condition)) {							\
		if (signal_pending(current)) {					\
			__ret = -ERESTARTSYS;					\
			break;							\
		}								\
		wait_woken(&wait, TASK_INTERRUPTIBLE,				\
			MAX_SCHEDULE_TIMEOUT);					\
	}									\
	remove_wait_queue(&wq, &wait);						\
	__ret;									\
})
#endif

#define IWD_SOCKET_EVENTS_BUF_NUM_PAGES	1

#define DEFAULT_MAX_MSG_SIZE		0
#define DEFAULT_OOB_BUFFER_SIZE		128

/* Socket's transmission buffer layout.
 *
 * [.....write.buffer.....][.....read.buffer.....][..oob.write.buffer..]
 * |<-----------------------------page-size--------------------------->|
 *
 * DEFAULT_TRANSMIT_BUFFER_SIZE defines default size of write and read buffers
 * available for transmitting data calculated to fit one page.
 */
#define DEFAULT_TRANSMIT_BUFFER_SIZE						\
	round_down(								\
		(PAGE_SIZE - 							\
			3 * round_up(CIRC_BUF_META_SIZE, sizeof(int32_t)) -	\
			2 * sizeof(int) - DEFAULT_OOB_BUFFER_SIZE) / 2,		\
		sizeof(int32_t))

#define IWD_EVENTS_BUFFER_SIZE							\
	round_down(								\
		IWD_SOCKET_EVENTS_BUF_NUM_PAGES * PAGE_SIZE / 2 -		\
			CIRC_BUF_META_SIZE,					\
		sizeof(int32_t))

#define IWD_EVENTS_ALLOCATION_BUFFER_SIZE					\
	round_up(IWD_EVENTS_BUFFER_SIZE + 					\
		CIRC_BUF_EMPTY_FLAG_SIZE, sizeof(int32_t))

#define INTERNAL_EVENTS_BUF_SIZE	512

/* Macros used for connection of circ buffers inside of shared socket's buffer */
#define GET_SOCKET_SEND_BUF(iwd_buf)						\
	(struct circ_buf *)((char *)iwd_buf + sizeof(struct iwd_sock_buf_head))

#define GET_SOCKET_RECEIVE_BUF(iwd_buf, rcv_buf_size)				\
	(struct circ_buf *)((char *)iwd_buf + 					\
		sizeof(struct iwd_sock_buf_head) + 				\
		circ_buf_total_size(rcv_buf_size))

#define GET_SOCKET_OOB_BUF(iwd_buf, rcv_buf_size, snd_buf_size)			\
	(struct circ_buf *)((char *)iwd_buf + 					\
		sizeof(struct iwd_sock_buf_head) +				\
		circ_buf_total_size(rcv_buf_size) +				\
		circ_buf_total_size(snd_buf_size))

enum operation {
	CONNECT_FROM_NWD,
	NEW_NWD_LISTEN,
	ACCEPT_FROM_NWD
};

enum state {
	NOT_INITIALIZED,
	READY,
	RELEASED
};

struct iwd_sock_events_buf {
	struct circ_buf nwd_events_head;
	char nwd_events_buf[IWD_EVENTS_ALLOCATION_BUFFER_SIZE];

	struct circ_buf swd_events_head;
	char swd_events_buf[IWD_EVENTS_ALLOCATION_BUFFER_SIZE];
} __packed;

struct iwd_sock_connect_request {
	int32_t swd_id;
	uint32_t snd_buf_size;
	uint32_t rcv_buf_size;
	uint32_t oob_buf_size;
	uint32_t max_msg_size;
} __packed;

struct connect_ext_data {
	enum operation op;
	struct sock_desc *sd;
	const char *name;
	int swd_id;
};

static DEFINE_IDR(tzdev_sock_map);
static DEFINE_MUTEX(tzdev_sock_map_lock);

static struct iwd_sock_events_buf *sock_events;
static struct circ_buf_desc nwd_sock_events;
static struct circ_buf_desc swd_sock_events;

static DEFINE_SPINLOCK(internal_events_in_lock);
static DECLARE_WAIT_QUEUE_HEAD(internal_events_wq);

static struct circ_buf_desc internal_events_in;
static struct circ_buf_desc internal_events_out;

static DEFINE_MUTEX(nwd_events_list_lock);

static DECLARE_WAIT_QUEUE_HEAD(tz_iwsock_wq);
static DECLARE_WAIT_QUEUE_HEAD(tz_iwsock_full_event_buf_wq);

static struct task_struct *notification_kthread;

static unsigned int tz_iwsock_state = NOT_INITIALIZED;

static void tz_iwsock_free_resources(struct sock_desc *sd)
{
	tz_iwio_free_iw_channel(sd->iwd_buf);

	kfree(sd);
}

static void tz_iwsock_get_sd(struct sock_desc *sd)
{
	atomic_inc(&sd->ref_count);
}

static struct sock_desc *tz_iwsock_get_sd_by_sid(unsigned int sid)
{
	struct sock_desc *sd;

	mutex_lock(&tzdev_sock_map_lock);
	sd = idr_find(&tzdev_sock_map, sid);
	if (sd)
		tz_iwsock_get_sd(sd);
	mutex_unlock(&tzdev_sock_map_lock);

	return sd;
}

void tz_iwsock_put_sd(struct sock_desc *sd)
{
	if (atomic_sub_return(1, &sd->ref_count) == 0)
		tz_iwsock_free_resources(sd);
}

static int tz_iwsock_publish_sd(struct sock_desc *sd)
{
	int sid;

	mutex_lock(&tzdev_sock_map_lock);

	sid = sysdep_idr_alloc(&tzdev_sock_map, sd);
	if (sid > 0) {
		sd->id = sid;
		tz_iwsock_get_sd(sd);
	}

	mutex_unlock(&tzdev_sock_map_lock);

	return sid;
}

static void tz_iwsock_unpublish_sd(struct sock_desc *sd)
{
	mutex_lock(&tzdev_sock_map_lock);
	idr_remove(&tzdev_sock_map, sd->id);
	mutex_unlock(&tzdev_sock_map_lock);

	tz_iwsock_put_sd(sd);
}

unsigned long circ_buf_total_size(unsigned long size)
{
	return round_up(size + CIRC_BUF_META_SIZE, sizeof(u32));
}

static unsigned long connected_iwd_buf_total_size(struct sock_desc *sd)
{
	return round_up(sizeof(struct iwd_sock_buf_head) +
			circ_buf_total_size(sd->snd_buf_size) +
			circ_buf_total_size(sd->rcv_buf_size) +
			circ_buf_total_size(sd->oob_buf_size), PAGE_SIZE);
}

static unsigned long listen_iwd_buf_total_size(struct sock_desc *sd)
{
	return round_up(sizeof(struct iwd_sock_buf_head) +
			circ_buf_total_size(sd->snd_buf_size) +
			circ_buf_total_size(sd->rcv_buf_size), PAGE_SIZE);
}
static int tz_iwsock_pre_connect_callback(void *buf,
		unsigned long num_pages, void *ext_data)
{
	struct connect_ext_data *data = ext_data;
	struct sock_desc *sd = data->sd;
	struct iwd_sock_buf_head *iwd_buf = buf;
	struct circ_buf *rcv_buf, *snd_buf, *oob_buf;
	int32_t nwd_id;
	unsigned long ret;

	sd->iwd_buf = buf;

	snd_buf = GET_SOCKET_SEND_BUF(iwd_buf);
	rcv_buf = GET_SOCKET_RECEIVE_BUF(iwd_buf, sd->snd_buf_size);

	/* Write buffer */
	circ_buf_connect(&sd->write_buf, circ_buf_set(snd_buf), sd->snd_buf_size);

	/* Receive buffer */
	circ_buf_connect(&sd->read_buf, circ_buf_set(rcv_buf), sd->rcv_buf_size);

	iwd_buf->nwd_state = BUF_SK_CONNECTED;
	iwd_buf->swd_state = BUF_SK_NEW;

	nwd_id = sd->id;

	/* Place operation's ID into shared buffer */
	ret = circ_buf_write_packet(&sd->write_buf, (void *)&data->op,
			sizeof(data->op), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	/* Place socket's ID into shared buffer */
	ret = circ_buf_write_packet(&sd->write_buf, (void *)&nwd_id,
			sizeof(nwd_id), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	switch (data->op) {
	case CONNECT_FROM_NWD:
	case NEW_NWD_LISTEN:
		/* Place name of the socket to connect to or listening socket name
		 * in NWd into shared buffer. */
		ret = circ_buf_write_packet(&sd->write_buf, (char *)data->name,
				strlen(data->name) + 1, CIRC_BUF_MODE_KERNEL);
		if (IS_ERR_VALUE(ret))
			return ret;
		break;

	case ACCEPT_FROM_NWD:
		/* Place SWd ID into shared buffer to identify socket connecting
		 * from SWd */
		ret = circ_buf_write_packet(&sd->write_buf,
				(char *)&data->swd_id, sizeof(data->swd_id),
				CIRC_BUF_MODE_KERNEL);
		if (IS_ERR_VALUE(ret))
			return ret;
		break;
	}

	/* Write buffers size */
	ret = circ_buf_write_packet(&sd->write_buf, (void *)&sd->snd_buf_size,
			sizeof(sd->snd_buf_size), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	ret = circ_buf_write_packet(&sd->write_buf, (void *)&sd->rcv_buf_size,
			sizeof(sd->rcv_buf_size), CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		return ret;

	switch (data->op) {
	case CONNECT_FROM_NWD:
	case ACCEPT_FROM_NWD:
		oob_buf = GET_SOCKET_OOB_BUF(iwd_buf,
				sd->snd_buf_size, sd->rcv_buf_size);

		/* Connect OOB buffer */
		circ_buf_connect(&sd->oob_buf,
				circ_buf_set(oob_buf), sd->oob_buf_size);

		ret = circ_buf_write_packet(&sd->write_buf,
				(void *)&sd->oob_buf_size,
				sizeof(sd->oob_buf_size), CIRC_BUF_MODE_KERNEL);
		if (IS_ERR_VALUE(ret))
			return ret;
		break;
	default:
		break;
	}

	return 0;
}

static int tz_iwsock_check_ready(void)
{
	smp_rmb();

	return (tz_iwsock_state == READY);
}

static int tz_iwsock_check_init_done(void)
{
	smp_rmb();

	if (tz_iwsock_state == NOT_INITIALIZED)
		return -EAGAIN;
	else if (tz_iwsock_state == READY)
		return 0;
	else
		return -ENODEV;
}

static int tz_iwsock_try_write(struct circ_buf_desc *cd,
		void *buf, unsigned int size)
{
	unsigned long ret;

	if (!tz_iwsock_check_ready())
		return -ECONNRESET;

	ret = circ_buf_write(cd, (char *)buf, size, CIRC_BUF_MODE_KERNEL);

	if (ret == -EAGAIN)
		tz_kthread_pool_enter_swd();
	else if (IS_ERR_VALUE(ret))
		BUG();

	return ret;
}

static int tz_iwsock_try_write_swd_event(int32_t id)
{
	return tz_iwsock_try_write(&nwd_sock_events, &id, sizeof(id));
}

static void tz_iwsock_notify_swd(int32_t sid)
{
	int ret;

	might_sleep();

	smp_wmb();

	mutex_lock(&nwd_events_list_lock);
	wait_event(tz_iwsock_full_event_buf_wq,
			(ret = tz_iwsock_try_write_swd_event(sid)) != -EAGAIN);
	mutex_unlock(&nwd_events_list_lock);

	if (ret > 0)
		tz_kthread_pool_enter_swd();
}

static int tz_iwsock_try_write_internal_event(int32_t id)
{
	unsigned long ret;

	if (!tz_iwsock_check_ready())
		return -ECONNRESET;

	spin_lock(&internal_events_in_lock);
	ret = circ_buf_write(&internal_events_in,
			(char *)&id, sizeof(id), CIRC_BUF_MODE_KERNEL);
	spin_unlock(&internal_events_in_lock);

	if (ret == -EAGAIN)
		wake_up(&tz_iwsock_wq);
	else if (IS_ERR_VALUE(ret))
		BUG();

	return ret;
}

static void tz_iwsock_notify_internally(int32_t sid)
{
	wait_event(internal_events_wq,
			tz_iwsock_try_write_internal_event(sid) != -EAGAIN);
	wake_up(&tz_iwsock_wq);
}

struct sock_desc *tz_iwsock_socket(unsigned int is_kern)
{
	struct sock_desc *sd;
	int ret;

	/* Allocate sock_desc structure */
	sd = kmalloc(sizeof(struct sock_desc), GFP_KERNEL);
	if (!sd)
		return ERR_PTR(-ENOMEM);

	memset(sd, 0, sizeof(struct sock_desc));

	atomic_set(&sd->ref_count, 1);
	init_waitqueue_head(&sd->wq);
	mutex_init(&sd->lock);

	sd->mm = ERR_PTR(-EINVAL);

	sd->cred.cmsghdr.cmsg_len = TZ_CMSG_LEN(sizeof(struct tz_cred));
	sd->cred.cmsghdr.cmsg_level = SOL_UNSPEC;
	sd->cred.cmsghdr.cmsg_type = TZ_SCM_CREDENTIALS;

	sd->snd_buf_size = DEFAULT_TRANSMIT_BUFFER_SIZE;
	sd->rcv_buf_size = DEFAULT_TRANSMIT_BUFFER_SIZE;
	sd->oob_buf_size = DEFAULT_OOB_BUFFER_SIZE;
	sd->max_msg_size = DEFAULT_MAX_MSG_SIZE;

	sd->is_kern = is_kern;

	sd->state = TZ_SK_NEW;

	if ((ret = tz_iwsock_publish_sd(sd)) <= 0) {
		tz_iwsock_put_sd(sd);
		sd = ERR_PTR(ret);
	}

	return sd;
}

static int tz_iwsock_copy_from(void *dst, void *src, int size, unsigned int is_kern)
{
	if (is_kern) {
		memcpy(dst, src, size);
		return 0;
	} else {
		return copy_from_user(dst, src, size);
	}
}

static int tz_iwsock_copy_to(void *dst, void *src, int size, unsigned int is_kern)
{
	if (is_kern) {
		memcpy(dst, src, size);
		return 0;
	} else {
		return copy_to_user(dst, src, size);
	}
}

static int tz_iwsock_getsockopt_socket(struct sock_desc *sd,
		int optname, void *optval, socklen_t *optlen)
{
	unsigned int size = sizeof(uint32_t);

	switch(optname) {
	case SO_SNDBUF:
		if (tz_iwsock_copy_to(optlen, &size, sizeof(size), sd->is_kern))
			return -EFAULT;

		if (tz_iwsock_copy_to(optval, &sd->snd_buf_size,
				sizeof(sd->snd_buf_size), sd->is_kern))
			return -EFAULT;

		return 0;

	case SO_RCVBUF:
		if (tz_iwsock_copy_to(optlen, &size, sizeof(size), sd->is_kern))
			return -EFAULT;

		if (tz_iwsock_copy_to(optval, &sd->rcv_buf_size,
				sizeof(sd->rcv_buf_size), sd->is_kern))
			return -EFAULT;

		return 0;

	default:
		return -ENOPROTOOPT;
	}
}

static int tz_iwsock_getsockopt_iwd(struct sock_desc *sd,
		int optname, void *optval, socklen_t *optlen)
{
	unsigned int size = sizeof(uint32_t);

	switch (optname) {
	case SO_IWD_MAX_MSG_SIZE:
		if (tz_iwsock_copy_to(optlen, &size, sizeof(size), sd->is_kern))
			return -EFAULT;

		if (tz_iwsock_copy_to(optval, &sd->max_msg_size,
				sizeof(sd->max_msg_size), sd->is_kern))
			return -EFAULT;

		return 0;
	default:
		return -ENOPROTOOPT;
	}
}

int tz_iwsock_getsockopt(struct sock_desc *sd, int level,
		int optname, void *optval, socklen_t *optlen)
{
	switch (level) {
	case SOL_SOCKET:
		return tz_iwsock_getsockopt_socket(sd, optname, optval, optlen);
	case SOL_IWD:
		return tz_iwsock_getsockopt_iwd(sd, optname, optval, optlen);
	default:
		return -EINVAL;
	}
}

static int tz_iwsock_setsockopt_socket(struct sock_desc *sd,
		int optname, void *optval, socklen_t optlen)
{
	unsigned int size;

	switch(optname) {
	case SO_SNDBUF:
		if (optlen != sizeof(unsigned int))
			return -EINVAL;
		if (tz_iwsock_copy_from(&size, optval, optlen, sd->is_kern))
			return -EFAULT;
		if (!size)
			return -EINVAL;

		sd->snd_buf_size = (unsigned int)(circ_buf_size_for_packet(size) +
				circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));

		return 0;

	case SO_RCVBUF:
		if (optlen != sizeof(unsigned int))
			return -EINVAL;
		if (tz_iwsock_copy_from(&size, optval, optlen, sd->is_kern))
			return -EFAULT;
		if (!size)
			return -EINVAL;

		sd->rcv_buf_size = (unsigned int)(circ_buf_size_for_packet(size) +
				circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));

		return 0;

	default:
		return -ENOPROTOOPT;
	}
}

static int tz_iwsock_setsockopt_iwd(struct sock_desc *sd,
		int optname, void *optval, socklen_t optlen)
{
	unsigned int size;

	switch (optname) {
	case SO_IWD_MAX_MSG_SIZE:
		if (optlen != sizeof(unsigned int))
			return -EINVAL;
		if (tz_iwsock_copy_from(&size, optval, optlen, sd->is_kern))
			return -EFAULT;

		if (size)
			sd->max_msg_size = (unsigned int)(circ_buf_size_for_packet(size) +
					circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));
		else
			sd->max_msg_size = 0;

		return 0;
	default:
		return -ENOPROTOOPT;
	}
}

int tz_iwsock_setsockopt(struct sock_desc *sd, int level,
		int optname, void *optval, socklen_t optlen)
{
	int ret;

	mutex_lock(&sd->lock);

	if (sd->state != TZ_SK_NEW) {
		ret = -EBUSY;
		goto unlock;
	}

	switch (level) {
	case SOL_SOCKET:
		ret = tz_iwsock_setsockopt_socket(sd, optname, optval, optlen);
		break;
	case SOL_IWD:
		ret = tz_iwsock_setsockopt_iwd(sd, optname, optval, optlen);
		break;
	default:
		ret = -EINVAL;
		break;
	}

unlock:
	mutex_unlock(&sd->lock);

	return ret;
}

int tz_iwsock_connect(struct sock_desc *sd, const char *name, int flags)
{
	struct connect_ext_data ext_data = {.op = CONNECT_FROM_NWD, .sd = sd, .name = name};
	void *buf;
	int ret = 0;

	if (unlikely(tz_iwsock_state != READY))
		return -ENODEV;

	smp_rmb();

	if (strnlen(name, TZ_FILE_NAME_LEN) == TZ_FILE_NAME_LEN)
		return -ENAMETOOLONG;

	mutex_lock(&sd->lock);

	switch (sd->state) {
	case TZ_SK_CONNECTED:
		ret = -EISCONN;
		goto wrong_state;
	case TZ_SK_CONNECT_IN_PROGRESS:
		ret = -EALREADY;
		goto wrong_state;
	case TZ_SK_NEW:
		sd->state = TZ_SK_CONNECT_IN_PROGRESS;
		break;
	case TZ_SK_LISTENING:
		ret = -EINVAL;
		goto wrong_state;
	case TZ_SK_RELEASED:
		BUG();
	}

	buf = tz_iwio_alloc_iw_channel(TZ_IWIO_CONNECT_SOCKET,
			connected_iwd_buf_total_size(sd) / PAGE_SIZE,
			tz_iwsock_pre_connect_callback,
			NULL, &ext_data);

	if (IS_ERR(buf)) {
		sd->state = TZ_SK_NEW;
		sd->iwd_buf = NULL;
		ret = PTR_ERR(buf);
		goto sock_connect_failed;
	}

	if (flags & MSG_DONTWAIT) {
		smp_rmb();
		/* Non-blocking mode */
		if (sd->iwd_buf->swd_state != BUF_SK_CONNECTED)
			ret = -EINPROGRESS;
		else
			sd->state = TZ_SK_CONNECTED;
	} else {
		mutex_unlock(&sd->lock);

		/* Blocking mode */
		return tz_iwsock_wait_connection(sd);
	}

wrong_state:
sock_connect_failed:
	mutex_unlock(&sd->lock);

	return ret;
}

static int tz_iwsock_is_connection_done(struct sock_desc *sd)
{
	smp_rmb();

	return sd->state != TZ_SK_CONNECT_IN_PROGRESS ||
			sd->iwd_buf->swd_state != BUF_SK_NEW;
}

int tz_iwsock_wait_connection(struct sock_desc *sd)
{
	int ret = 0;

	if (unlikely(tz_iwsock_state != READY))
		return -ENODEV;

	smp_rmb();

	mutex_lock(&sd->lock);

	switch (sd->state) {
	case TZ_SK_NEW:
	case TZ_SK_LISTENING:
		ret = -EINVAL;
		goto unlock_sd;

	case TZ_SK_CONNECT_IN_PROGRESS:
		break;

	case TZ_SK_CONNECTED:
		goto unlock_sd;

	case TZ_SK_RELEASED:
		BUG();
	}

	if (sd->is_kern)
		wait_event(sd->wq,
				!tz_iwsock_check_ready() ||
				tz_iwsock_is_connection_done(sd));
	else
		ret = wait_event_interruptible(sd->wq,
				!tz_iwsock_check_ready() ||
				tz_iwsock_is_connection_done(sd));

	if (ret)
		goto unlock_sd;

	if (!tz_iwsock_check_ready()) {
		ret = -ECONNREFUSED;
		goto unlock_sd;
	}

	switch (sd->iwd_buf->swd_state) {
	case BUF_SK_CONNECTED:
		sd->state = TZ_SK_CONNECTED;
		ret = 0;
		break;
	case BUF_SK_CLOSED:
		ret = -ECONNREFUSED;
		break;
	default:
		break;
	}

unlock_sd:
	mutex_unlock(&sd->lock);

	return ret;
}

int tz_iwsock_listen(struct sock_desc *sd, const char *name)
{
	struct connect_ext_data ext_data = {.op = NEW_NWD_LISTEN, .sd = sd, .name = name};
	void *buf;
	unsigned int snd_buf_size = sd->snd_buf_size, rcv_buf_size = sd->rcv_buf_size;
	int ret = 0;

	if (sd->is_kern)
		wait_event(tz_iwsock_wq, tz_iwsock_check_init_done() != -EAGAIN);
	else if ((ret = wait_event_interruptible(
				tz_iwsock_wq, tz_iwsock_check_init_done() != -EAGAIN)))
		return ret;

	if (!tz_iwsock_check_ready())
		return -ENODEV;

	if (strnlen(name, TZ_FILE_NAME_LEN) == TZ_FILE_NAME_LEN)
		return -ENAMETOOLONG;

	mutex_lock(&sd->lock);

	if (sd->state != TZ_SK_NEW) {
		ret = -EBADF;
		goto unlock;
	}

	strcpy(sd->listen_name, name);

	sd->snd_buf_size = DEFAULT_TRANSMIT_BUFFER_SIZE;
	sd->rcv_buf_size = DEFAULT_TRANSMIT_BUFFER_SIZE;

	buf = tz_iwio_alloc_iw_channel(TZ_IWIO_CONNECT_SOCKET,
			listen_iwd_buf_total_size(sd) / PAGE_SIZE,
			tz_iwsock_pre_connect_callback, NULL, &ext_data);

	if (IS_ERR(buf)) {
		sd->iwd_buf = NULL;

		sd->snd_buf_size = snd_buf_size;
		sd->rcv_buf_size = rcv_buf_size;

		ret = PTR_ERR(buf);
	} else {
		sd->state = TZ_SK_LISTENING;
	}

unlock:
	mutex_unlock(&sd->lock);

	return ret;
}

struct sock_desc *tz_iwsock_accept(struct sock_desc *sd)
{
	struct connect_ext_data ext_data = {.op = ACCEPT_FROM_NWD};
	struct iwd_sock_connect_request conn_req;
	struct sock_desc *new_sd, *ret;
	void *iwd_buf;
	int res;

	if (unlikely(tz_iwsock_state != READY))
		return ERR_PTR(-ENODEV);

	smp_rmb();

	mutex_lock(&sd->lock);
	if (sd->is_kern)
		wait_event(sd->wq, !tz_iwsock_check_ready() ||
			sd->state != TZ_SK_LISTENING ||
			!circ_buf_is_empty(&sd->read_buf));
	else if ((res = wait_event_interruptible(sd->wq,
			!tz_iwsock_check_ready() ||
			sd->state != TZ_SK_LISTENING ||
			!circ_buf_is_empty(&sd->read_buf)))) {
		mutex_unlock(&sd->lock);
		return ERR_PTR(res);
	}

	if (!tz_iwsock_check_ready() || sd->state != TZ_SK_LISTENING) {
		mutex_unlock(&sd->lock);
		return ERR_PTR(-EINVAL);
	}

	res = circ_buf_read(&sd->read_buf, (char *)&conn_req,
			sizeof(struct iwd_sock_connect_request), CIRC_BUF_MODE_KERNEL);
	mutex_unlock(&sd->lock);

	if (res != sizeof(struct iwd_sock_connect_request))
		return ERR_PTR(res);

	new_sd = tz_iwsock_socket(sd->is_kern);
	if (IS_ERR(new_sd)) {
		ret = new_sd;
		goto socket_allocation_failed;
	}

	new_sd->snd_buf_size = (unsigned int)(circ_buf_size_for_packet(conn_req.snd_buf_size) +
			circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));
	new_sd->rcv_buf_size = (unsigned int)(circ_buf_size_for_packet(conn_req.rcv_buf_size) +
			circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));
	new_sd->oob_buf_size = (unsigned int)(circ_buf_size_for_packet(conn_req.oob_buf_size) +
			circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));

	if (conn_req.max_msg_size)
		new_sd->max_msg_size = (unsigned int)(circ_buf_size_for_packet(conn_req.max_msg_size) +
				circ_buf_size_for_packet(sizeof(struct tz_cmsghdr_cred)));

	ext_data.swd_id = conn_req.swd_id;

	ext_data.sd = new_sd;

	iwd_buf = tz_iwio_alloc_iw_channel(TZ_IWIO_CONNECT_SOCKET,
			connected_iwd_buf_total_size(new_sd) / PAGE_SIZE,
			tz_iwsock_pre_connect_callback,
			NULL, &ext_data);
	if (IS_ERR(iwd_buf)) {
		new_sd->iwd_buf = NULL;
		ret = iwd_buf;
		tzdev_print(0, "accept from NWd failed, err=%ld\n", PTR_ERR(iwd_buf));
		goto iwio_connect_failed;
	}

	/* Mark new socket as connected */
	new_sd->state = TZ_SK_CONNECTED;

	ret = new_sd;

	return ret;

iwio_connect_failed:
	tz_iwsock_release(new_sd);

socket_allocation_failed:
	/* Write swd_id to unblock waiting threads in SWd */
	mutex_lock(&sd->lock);
	wait_event(sd->wq, tz_iwsock_try_write(&sd->write_buf,
			&conn_req.swd_id, sizeof(conn_req.swd_id)) != -EAGAIN);
	mutex_unlock(&sd->lock);

	tz_iwsock_notify_swd(sd->id);

	return ret;
}

void tz_iwsock_release(struct sock_desc *sd)
{
	unsigned int notify_swd = 0, notify_nwd = 0;
	struct iwd_sock_connect_request conn_req;

	mutex_lock(&sd->lock);

	switch (sd->state) {
	case TZ_SK_NEW:
		tz_iwsock_unpublish_sd(sd);
		break;

	case TZ_SK_LISTENING:
		/* Mark NWd state as closed to avoid new connect requests */
		sd->iwd_buf->nwd_state = BUF_SK_CLOSED;

		smp_mb();

		/* Read all requests and write IDs back to wake waiting threads */
		while (sd->iwd_buf->swd_state != BUF_SK_CLOSED &&
				circ_buf_read(&sd->read_buf, (char *)&conn_req,
				sizeof(conn_req), CIRC_BUF_MODE_KERNEL) > 0) {
			wait_event(sd->wq, tz_iwsock_try_write(&sd->write_buf,
					(char *)&conn_req.swd_id, sizeof(conn_req.swd_id)) != -EAGAIN);
		}

		/* Listen socket has connected IWd buffer, so proceed to check
		 * it's state. */
	case TZ_SK_CONNECT_IN_PROGRESS:
	case TZ_SK_CONNECTED:
		/* Mark NWd state as closed */
		sd->iwd_buf->nwd_state = BUF_SK_CLOSED;

		smp_mb();

		if (sd->iwd_buf->swd_state != BUF_SK_CLOSED)
			notify_swd = 1;
		else
			notify_nwd = 1;

		break;

	case TZ_SK_RELEASED:
		BUG();
	}

	sd->state = TZ_SK_RELEASED;

	mutex_unlock(&sd->lock);

	if (notify_swd)
		tz_iwsock_notify_swd(sd->id);
	else if (notify_nwd)
		tz_iwsock_notify_internally(sd->id);

	tz_iwsock_put_sd(sd);

	return;
}

static int __tz_iwsock_read(struct sock_desc *sd, struct circ_buf_desc *read_buf,
			void *buf1, size_t len1, void *buf2, size_t len2, int flags,
			unsigned int *need_notify)
{
	int mode = (sd->is_kern) ? CIRC_BUF_MODE_KERNEL : CIRC_BUF_MODE_USER;
	int swd_state;
	unsigned long ret;
	size_t nbytes;

	if (unlikely(tz_iwsock_state != READY))
		return -ENODEV;

	mutex_lock(&sd->lock);

	switch (sd->state) {
	case TZ_SK_NEW:
	case TZ_SK_LISTENING:
	case TZ_SK_CONNECT_IN_PROGRESS:
		ret = -ENOTCONN;
		goto unlock;
	case TZ_SK_CONNECTED:
		break;
	case TZ_SK_RELEASED:
		BUG();
	}

	swd_state = sd->iwd_buf->swd_state;

	smp_rmb();

	ret = circ_buf_read_packet_local(read_buf,
			(char *)buf1, len1, CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		goto recheck;

	ret = circ_buf_read_packet(read_buf,
			buf2, len2, mode);
	if (IS_ERR_VALUE(ret))
		circ_buf_rollback_read(&sd->read_buf);

	if (sd->max_msg_size) {
		nbytes = circ_buf_size_for_packet(sd->max_msg_size) * 2;

		if (circ_buf_bytes_free(read_buf) < nbytes)
			*need_notify = 1;
	} else {
		*need_notify = 1;
	}

recheck:
	if (ret == -EAGAIN && swd_state == BUF_SK_CLOSED)
		ret = 0;

unlock:
	mutex_unlock(&sd->lock);

	return ret;
}

ssize_t tz_iwsock_read(struct sock_desc *sd, void *buf, size_t count, int flags)
{
	struct tz_cmsghdr_cred cred;
	int res = 0, ret = 0;
	unsigned int need_notify = 0;

	if (sd->is_kern)
		tz_iwsock_wait_event(sd->wq,
				(ret = __tz_iwsock_read(sd, &sd->read_buf, &cred,
					sizeof(cred), buf, count, flags,
					&need_notify)) != -EAGAIN);
	else
		res = tz_iwsock_wait_event_interruptible(sd->wq,
				(ret = __tz_iwsock_read(sd, &sd->read_buf, &cred,
				sizeof(cred), buf, count, flags,
				&need_notify)) != -EAGAIN);

	if (res)
		ret = res;

	if (ret > 0 && need_notify)
		tz_iwsock_notify_swd(sd->id);

	return ret;
}

static int tz_iwsock_format_cred_scm(struct sock_desc *sd)
{
	int ret;

	if (sd->mm == current->mm)
		return 0;

	ret = tz_format_cred(&sd->cred.cred);
	if (ret) {
		tzdev_print(0, "Failed to format socket credentials %d\n", ret);
		return ret;
	}

	sd->mm = current->mm;

	return ret;
}

static int __tz_iwsock_write(struct sock_desc *sd, struct circ_buf_desc *write_buf,
			void *scm_buf, size_t scm_len, void *data_buf, size_t data_len, int flags)
{
	unsigned long ret;
	int mode = (sd->is_kern) ? CIRC_BUF_MODE_KERNEL : CIRC_BUF_MODE_USER;

	if (unlikely(tz_iwsock_state != READY))
		return -ENODEV;

	mutex_lock(&sd->lock);

	switch (sd->state) {
	case TZ_SK_NEW:
	case TZ_SK_LISTENING:
	case TZ_SK_CONNECT_IN_PROGRESS:
		ret = -ENOTCONN;
		goto unlock;
	case TZ_SK_CONNECTED:
		break;
	case TZ_SK_RELEASED:
		BUG();
	}

	smp_rmb();

	if (sd->iwd_buf->swd_state == BUF_SK_CLOSED) {
		ret = -ECONNRESET;
		goto unlock;
	}

	if (sd->max_msg_size && data_len > sd->max_msg_size) {
		ret = -EMSGSIZE;
		goto unlock;
	}

	ret = circ_buf_write_packet_local(write_buf,
			(char *)scm_buf, scm_len, CIRC_BUF_MODE_KERNEL);
	if (IS_ERR_VALUE(ret))
		goto unlock;

	ret = circ_buf_write_packet(write_buf,
			(char *)data_buf, data_len, mode);
	if (IS_ERR_VALUE(ret))
		circ_buf_rollback_write(write_buf);

unlock:
	mutex_unlock(&sd->lock);

	return ret;
}

ssize_t tz_iwsock_write(struct sock_desc *sd, void *buf, size_t count, int flags)
{
	struct circ_buf_desc *write_buf;
	int ret, res = 0;

	write_buf = (flags & MSG_OOB) ? &sd->oob_buf : &sd->write_buf;

	if ((ret = tz_iwsock_format_cred_scm(sd)))
		return ret;

	if (sd->is_kern)
		tz_iwsock_wait_event(sd->wq,
				(ret = __tz_iwsock_write(sd, write_buf, &sd->cred,
					sizeof(sd->cred), buf, count, flags)) != -EAGAIN);
	else
		res = tz_iwsock_wait_event_interruptible(sd->wq,
			(ret = __tz_iwsock_write(sd, write_buf, &sd->cred,
				sizeof(sd->cred), buf, count, flags)) != -EAGAIN);

	if (res)
		ret = res;

	if (ret > 0)
		tz_iwsock_notify_swd(sd->id);

	return ret;
}

void tz_iwsock_wake_up_all(void)
{
	struct sock_desc *sd;
	int id;

	mutex_lock(&tzdev_sock_map_lock);
	idr_for_each_entry(&tzdev_sock_map, sd, id)
		wake_up(&sd->wq);
	mutex_unlock(&tzdev_sock_map_lock);
}

void tz_iwsock_kernel_panic_handler(void)
{
	if (likely(tz_iwsock_state == READY)) {
		smp_rmb();

		tz_iwsock_state = RELEASED;

		smp_wmb();

		tz_iwsock_wake_up_all();
	}
}

void tz_iwsock_check_notifications(void)
{
	if (unlikely(tz_iwsock_state != READY))
		return;

	smp_rmb();

	wake_up_all(&tz_iwsock_full_event_buf_wq);

	if (!circ_buf_is_empty(&swd_sock_events))
		wake_up(&tz_iwsock_wq);
}

static void tz_iwsock_process_notifications(void)
{
	struct sock_desc *sd;
	int swd_event_buffer_is_full;
	int32_t id;
	ssize_t res;

	while (1) {
		/* Events are read from this thread only, so no need of lock. */
		swd_event_buffer_is_full =
				circ_buf_bytes_free(&swd_sock_events) < sizeof(int32_t);

		res = circ_buf_read(&swd_sock_events, (char *)&id,
				sizeof(id), CIRC_BUF_MODE_KERNEL);

		if (res != sizeof(id)) {
			res = circ_buf_read(&internal_events_out, (char *)&id,
					sizeof(id), CIRC_BUF_MODE_KERNEL);
			if (res == sizeof(id))
				wake_up(&internal_events_wq);
			else if (res != -EAGAIN)
				BUG();
		}

		if (res != sizeof(id))
			break;

		if (swd_event_buffer_is_full)
			/* Request re-entry in to SWd to wake sleeping socket's notifiers.
			 * We do that because cpu mask can be zero on return to NWd. */
			tz_kthread_pool_enter_swd();

		sd = tz_iwsock_get_sd_by_sid(id);
		if (sd) {
			smp_rmb();

			if (sd->state == TZ_SK_RELEASED &&
					sd->iwd_buf->swd_state == BUF_SK_CLOSED)
				tz_iwsock_unpublish_sd(sd);
			else
				wake_up(&sd->wq);

			tz_iwsock_put_sd(sd);
		}
	}
}

static int tz_iwsock_kthread(void *data)
{
	(void)data;

	while (!kthread_should_stop()) {
		wait_event(tz_iwsock_wq,
				!circ_buf_is_empty(&swd_sock_events) ||
				!circ_buf_is_empty(&internal_events_out) ||
				kthread_should_stop());

		if (kthread_should_stop())
			return 0;

		tz_iwsock_process_notifications();
	}

	return 0;
}

static int tz_iwsock_events_pre_connect_callback(void *buf, unsigned long num_pages, void *ext_data)
{
	struct iwd_sock_events_buf *sock_events = (struct iwd_sock_events_buf *)buf;

	/* Set counters in events shared buffer */
	circ_buf_init(&sock_events->nwd_events_head);
	circ_buf_init(&sock_events->swd_events_head);

	return 0;
}

int tz_iwsock_init(void)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	struct circ_buf *circ_buf;
	int ret;

	circ_buf = circ_buf_alloc(INTERNAL_EVENTS_BUF_SIZE);
	if (!circ_buf)
		return -ENOMEM;

	circ_buf_connect(&internal_events_in, circ_buf, INTERNAL_EVENTS_BUF_SIZE);
	circ_buf_connect(&internal_events_out, circ_buf, INTERNAL_EVENTS_BUF_SIZE);

	sock_events = tz_iwio_alloc_iw_channel(TZ_IWIO_CONNECT_SOCKET_EVENTS,
			IWD_SOCKET_EVENTS_BUF_NUM_PAGES,
			tz_iwsock_events_pre_connect_callback, NULL, NULL);
	if (IS_ERR(sock_events)) {
		ret = PTR_ERR(sock_events);
		sock_events = NULL;
		goto sock_events_connect_failed;
	}

	circ_buf_connect(&nwd_sock_events,
			&sock_events->nwd_events_head, IWD_EVENTS_BUFFER_SIZE);

	circ_buf_connect(&swd_sock_events,
			&sock_events->swd_events_head, IWD_EVENTS_BUFFER_SIZE);

	smp_wmb();

	notification_kthread = kthread_run(tz_iwsock_kthread, NULL, "tz_iwsock");
	if (IS_ERR(notification_kthread)) {
		ret = PTR_ERR(notification_kthread);
		goto kthread_start_failed;
	}

	sched_setscheduler(notification_kthread, SCHED_FIFO, &param);

	tz_iwsock_state = READY;

	smp_wmb();

	wake_up(&tz_iwsock_wq);

	return 0;

kthread_start_failed:
	tz_iwio_free_iw_channel(sock_events);
	notification_kthread = NULL;

sock_events_connect_failed:
	circ_buf_free(internal_events_in.circ_buf);

	return ret;
}

void tz_iwsock_fini(void)
{
	if (likely(tz_iwsock_state == READY)) {
		smp_rmb();

		tz_iwsock_state = RELEASED;

		smp_wmb();

		tz_iwsock_wake_up_all();

		wake_up(&tz_iwsock_wq);
		wake_up(&internal_events_wq);
		wake_up(&tz_iwsock_full_event_buf_wq);

		kthread_stop(notification_kthread);

		notification_kthread = NULL;
	}
}
