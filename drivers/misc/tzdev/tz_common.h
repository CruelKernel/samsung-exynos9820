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

#ifndef __TZ_COMMON_H__
#define __TZ_COMMON_H__

#ifndef __USED_BY_TZSL__
#include <teec/iw_messages.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/limits.h>
#else
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

#define UINT_PTR(a)			((void *)(unsigned long)(a))

#define TZ_IOC_MAGIC			'c'

#define TZIO_MEM_REGISTER		_IOW(TZ_IOC_MAGIC, 120, struct tzio_mem_register)
#define TZIO_MEM_RELEASE		_IOW(TZ_IOC_MAGIC, 121, int)
#define TZIO_CRYPTO_CLOCK_CONTROL	_IOW(TZ_IOC_MAGIC, 123, int)
#define TZIO_GET_SYSCONF		_IOW(TZ_IOC_MAGIC, 124, struct tzio_sysconf)
#define TZIO_BOOST_CONTROL		_IOW(TZ_IOC_MAGIC, 125, int)

#define TZIO_UIWSOCK_CONNECT		_IOW(TZ_IOC_MAGIC, 130, int)
#define TZIO_UIWSOCK_WAIT_CONNECTION	_IOW(TZ_IOC_MAGIC, 131, int)
#define TZIO_UIWSOCK_SEND		_IOW(TZ_IOC_MAGIC, 132, int)
#define TZIO_UIWSOCK_RECV		_IOWR(TZ_IOC_MAGIC, 133, int)
#define TZIO_UIWSOCK_LISTEN		_IOWR(TZ_IOC_MAGIC, 134, int)
#define TZIO_UIWSOCK_ACCEPT		_IOWR(TZ_IOC_MAGIC, 135, int)
#define TZIO_UIWSOCK_GETSOCKOPT		_IOWR(TZ_IOC_MAGIC, 136, int)
#define TZIO_UIWSOCK_SETSOCKOPT		_IOWR(TZ_IOC_MAGIC, 137, int)

/* NB: Sysconf related definitions should match with those in SWd */
#define SYSCONF_SWD_VERSION_LEN			256
#define SYSCONF_SWD_EARLY_SWD_INIT		(1 << 0)

#define SYSCONF_NWD_CRYPTO_CLOCK_MANAGEMENT	(1 << 0)
#define SYSCONF_NWD_CPU_HOTPLUG			(1 << 1)
#define SYSCONF_NWD_TZDEV_DEPLOY_TZAR		(1 << 2)

struct tzio_swd_sysconf {
	uint32_t os_version;			/* SWd OS version */
	uint32_t cpu_num;			/* SWd number of cpu supported */
	uint32_t big_cpus_mask;			/* SWd big cluster cpus mask */
	uint32_t flags;				/* SWd flags */
	char version[SYSCONF_SWD_VERSION_LEN];	/* SWd OS version string */
} __attribute__((__packed__));

struct tzio_nwd_sysconf {
	uint32_t flags;				/* NWd flags */
} __attribute__((__packed__));

struct tzio_sysconf {
	struct tzio_nwd_sysconf nwd_sysconf;
	struct tzio_swd_sysconf swd_sysconf;
} __attribute__((__packed__));

struct tzio_mem_register {
	const uint64_t ptr;		/* Memory region start (in) */
	uint64_t size;			/* Memory region size (in) */
	uint32_t write;			/* 1 - rw, 0 - ro */
} __attribute__((__packed__));

/* Used for switching crypto clocks ON/OFF */
enum {
	TZIO_CRYPTO_CLOCK_OFF,
	TZIO_CRYPTO_CLOCK_ON,
};

/* Used for switching boosting ON/OFF */
enum {
	TZIO_BOOST_OFF,
	TZIO_BOOST_ON,
};

#define TZ_UIWSOCK_MAX_NAME_LENGTH	256

struct tz_uiwsock_connection {
	char name[TZ_UIWSOCK_MAX_NAME_LENGTH];
};

struct tz_uiwsock_data {
	uint64_t buffer;
	uint64_t size;
	uint32_t flags;
} __attribute__((__packed__));

struct tz_uiwsock_sockopt {
	int32_t level;
	int32_t optname;
	uint64_t optval;
	uint32_t optlen;
} __attribute__((__packed__));

#endif /*__TZ_COMMON_H__*/
