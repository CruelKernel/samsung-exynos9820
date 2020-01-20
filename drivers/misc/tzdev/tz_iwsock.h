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

#ifndef __TZ_IWSOCK_H__
#define __TZ_IWSOCK_H__

#include "lib/circ_buf.h"
#include "tz_cred.h"

#define TZ_SCM_CREDENTIALS	2

#define SOL_UNSPEC		0xFFFE
#define SOL_IWD			0xFFFD

#define SO_IWD_MAX_MSG_SIZE	1

#define TZ_CMSG_ALIGN(len)	(((len) + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1))
#define TZ_CMSG_LEN(len)	(TZ_CMSG_ALIGN(sizeof(struct tz_cmsghdr)) + (len))

#define TZ_FILE_NAME_LEN	80

typedef uint32_t		socklen_t;

struct tz_cmsghdr {
	uint32_t cmsg_len;	/* data byte count, including header */
	uint32_t cmsg_level;	/* originating protocol */
	uint32_t cmsg_type;	/* protocol-specific type */
	/* followed by the actual control message data */
};

struct tz_cmsghdr_cred {
	struct tz_cmsghdr cmsghdr;
	struct tz_cred cred;
};

/* Enum values should fit SWd's version */
enum buf_endpoint_state {
	BUF_SK_CLOSED,
	BUF_SK_CONNECTED,
	BUF_SK_NEW
};

enum tz_endpoint_state {
	TZ_SK_NEW,
	TZ_SK_LISTENING,
	TZ_SK_CONNECT_IN_PROGRESS,
	TZ_SK_CONNECTED,
	TZ_SK_RELEASED,
};

struct iwd_sock_buf;

struct iwd_sock_buf_head {
	enum buf_endpoint_state nwd_state;
	enum buf_endpoint_state swd_state;
} __packed;

struct sock_desc {
	struct iwd_sock_buf_head *iwd_buf;
	struct circ_buf_desc write_buf;
	struct circ_buf_desc read_buf;
	struct circ_buf_desc oob_buf;
	wait_queue_head_t wq;
	int id;
	enum tz_endpoint_state state;
	atomic_t ref_count;
	struct mutex lock;
	char listen_name[TZ_FILE_NAME_LEN];
	unsigned int is_kern;
	struct mm_struct *mm;
	struct tz_cmsghdr_cred cred;
	unsigned int snd_buf_size;
	unsigned int rcv_buf_size;
	unsigned int oob_buf_size;
	unsigned int max_msg_size;
};

int tz_iwsock_init(void);
void tz_iwsock_fini(void);
void tz_iwsock_check_notifications(void);
int tz_iwsock_need_swd_entry(void);
void tz_iwsock_kernel_panic_handler(void);

struct sock_desc *tz_iwsock_socket(unsigned int is_kern);
int tz_iwsock_getsockopt(struct sock_desc *sd, int level,
		int optname, void *optval, socklen_t *optlen);
int tz_iwsock_setsockopt(struct sock_desc *sd, int level,
		int optname, void *optval, socklen_t optlen);
int tz_iwsock_connect(struct sock_desc *sd, const char *name, int flags);
int tz_iwsock_wait_connection(struct sock_desc *sd);
int tz_iwsock_listen(struct sock_desc *sd, const char *name);
struct sock_desc *tz_iwsock_accept(struct sock_desc *sd);
void tz_iwsock_release(struct sock_desc *sd);
ssize_t tz_iwsock_read(struct sock_desc *sd, void *buf, size_t count, int flags);
ssize_t tz_iwsock_write(struct sock_desc *sd, void *buf, size_t count, int flags);

void tz_iwsock_put_sd(struct sock_desc *sd);

#endif /* __TZ_IWSOCK_H__ */
