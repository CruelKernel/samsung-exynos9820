/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Exynos-SnapShot debugging framework for Exynos SoC
 *
 * Author: Hosung Kim <Hosung0.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/kallsyms.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/pstore_ram.h>
#include <linux/sched/clock.h>
#include <linux/ftrace.h>
#ifdef CONFIG_SEC_EXT
#include <linux/sec_ext.h>
#endif

#include "debug-snapshot-local.h"
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/hardirq.h>
#include <asm/stacktrace.h>
#include <linux/debug-snapshot.h>
#include <linux/kernel_stat.h>
#include <linux/irqnr.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>

/* This defines are for PSTORE */
#define DSS_LOGGER_LEVEL_HEADER		(1)
#define DSS_LOGGER_LEVEL_PREFIX		(2)
#define DSS_LOGGER_LEVEL_TEXT		(3)
#define DSS_LOGGER_LEVEL_MAX		(4)
#define DSS_LOGGER_SKIP_COUNT		(4)
#define DSS_LOGGER_STRING_PAD		(1)
#define DSS_LOGGER_HEADER_SIZE		(68)

#define DSS_LOG_ID_MAIN			(0)
#define DSS_LOG_ID_RADIO		(1)
#define DSS_LOG_ID_EVENTS		(2)
#define DSS_LOG_ID_SYSTEM		(3)
#define DSS_LOG_ID_CRASH		(4)
#define DSS_LOG_ID_KERNEL		(5)

typedef struct __attribute__((__packed__)) {
	uint8_t magic;
	uint16_t len;
	uint16_t uid;
	uint16_t pid;
} dss_pmsg_log_header_t;

typedef struct __attribute__((__packed__)) {
	unsigned char id;
	uint16_t tid;
	int32_t tv_sec;
	int32_t tv_nsec;
} dss_android_log_header_t;

typedef struct dss_logger {
	uint16_t	len;
	uint16_t	id;
	uint16_t	pid;
	uint16_t	tid;
	uint16_t	uid;
	uint16_t	level;
	int32_t		tv_sec;
	int32_t		tv_nsec;
	char		msg;
	char		*buffer;
	void		(*func_hook_logger)(const char*, const char*, size_t);
} __attribute__((__packed__)) dss_logger;

static dss_logger logger;

void register_hook_logger(void (*func)(const char *name, const char *buf, size_t size))
{
	logger.func_hook_logger = func;
	logger.buffer = vmalloc(PAGE_SIZE * 3);

	if (logger.buffer)
		pr_info("debug-snapshot: logger buffer alloc address: 0x%p\n", logger.buffer);
}
EXPORT_SYMBOL(register_hook_logger);

static int dbg_snapshot_combine_pmsg(char *buffer, size_t count, unsigned int level)
{
	char *logbuf = logger.buffer;

	if (!logbuf)
		return -ENOMEM;

	switch (level) {
	case DSS_LOGGER_LEVEL_HEADER:
	{
			struct tm tmBuf;
			u64 tv_kernel;
			unsigned int logbuf_len;
			unsigned long rem_nsec;

			if (logger.id == DSS_LOG_ID_EVENTS)
				break;

			tv_kernel = local_clock();
			rem_nsec = do_div(tv_kernel, 1000000000);
			time_to_tm(logger.tv_sec, 0, &tmBuf);

			logbuf_len = snprintf(logbuf, DSS_LOGGER_HEADER_SIZE,
					"\n[%5lu.%06lu][%d:%16s] %02d-%02d %02d:%02d:%02d.%03d %5d %5d  ",
					(unsigned long)tv_kernel, rem_nsec / 1000,
					raw_smp_processor_id(), current->comm,
					tmBuf.tm_mon + 1, tmBuf.tm_mday,
					tmBuf.tm_hour, tmBuf.tm_min, tmBuf.tm_sec,
					logger.tv_nsec / 1000000, logger.pid, logger.tid);

			logger.func_hook_logger("log_platform", logbuf, logbuf_len - 1);
		}
		break;
	case DSS_LOGGER_LEVEL_PREFIX:
		{
			static const char *kPrioChars = "!.VDIWEFS";
			unsigned char prio = logger.msg;

			if (logger.id == DSS_LOG_ID_EVENTS)
				break;

			logbuf[0] = prio < strlen(kPrioChars) ? kPrioChars[prio] : '?';
			logbuf[1] = ' ';

			logger.func_hook_logger("log_platform", logbuf, DSS_LOGGER_LEVEL_PREFIX);
		}
		break;
	case DSS_LOGGER_LEVEL_TEXT:
		{
			char *eatnl = buffer + count - DSS_LOGGER_STRING_PAD;

			if (logger.id == DSS_LOG_ID_EVENTS)
				break;
			if (count == DSS_LOGGER_SKIP_COUNT && *eatnl != '\0')
				break;

			logger.func_hook_logger("log_platform", buffer, count - 1);
#ifdef CONFIG_SEC_EXT
			if (count > 1 && strncmp(buffer, "!@", 2) == 0) {
				/* To prevent potential buffer overrun
				 * put a null at the end of the buffer if required
				 */
				buffer[count - 1] = '\0';
				pr_info("%s\n", buffer);
#ifdef CONFIG_SEC_BOOTSTAT
				if (count > 5 && strncmp(buffer, "!@Boot", 6) == 0)
					sec_bootstat_add(buffer);
#endif /* CONFIG_SEC_BOOTSTAT */

			}
#endif /* CONFIG_SEC_EXT */
		}
		break;
	default:
		break;
	}
	return 0;
}

int dbg_snapshot_hook_pmsg(char *buffer, size_t count)
{
	dss_android_log_header_t header;
	dss_pmsg_log_header_t pmsg_header;

	if (!logger.buffer)
		return -ENOMEM;

	switch (count) {
	case sizeof(pmsg_header):
		memcpy((void *)&pmsg_header, buffer, count);
		if (pmsg_header.magic != 'l') {
			dbg_snapshot_combine_pmsg(buffer, count, DSS_LOGGER_LEVEL_TEXT);
		} else {
			/* save logger data */
			logger.pid = pmsg_header.pid;
			logger.uid = pmsg_header.uid;
			logger.len = pmsg_header.len;
		}
		break;
	case sizeof(header):
		/* save logger data */
		memcpy((void *)&header, buffer, count);
		logger.id = header.id;
		logger.tid = header.tid;
		logger.tv_sec = header.tv_sec;
		logger.tv_nsec  = header.tv_nsec;
		if (logger.id > 7) {
			/* write string */
			dbg_snapshot_combine_pmsg(buffer, count, DSS_LOGGER_LEVEL_TEXT);
		} else {
			/* write header */
			dbg_snapshot_combine_pmsg(buffer, count, DSS_LOGGER_LEVEL_HEADER);
		}
		break;
	case sizeof(unsigned char):
		logger.msg = buffer[0];
		/* write char for prefix */
		dbg_snapshot_combine_pmsg(buffer, count, DSS_LOGGER_LEVEL_PREFIX);
		break;
	default:
		/* write string */
		dbg_snapshot_combine_pmsg(buffer, count, DSS_LOGGER_LEVEL_TEXT);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(dbg_snapshot_hook_pmsg);

/*
 *  To support pstore/pmsg/pstore_ram, following is implementation for debug-snapshot
 *  dss_ramoops platform_device is used by pstore fs.
 */

static struct ramoops_platform_data dss_ramoops_data = {
	.record_size	= SZ_4K,
	.pmsg_size	= SZ_4K,
	.dump_oops	= 1,
};

static struct platform_device dss_ramoops = {
	.name = "ramoops",
	.dev = {
		.platform_data = &dss_ramoops_data,
	},
};

static int __init dss_pstore_init(void)
{
	if (dbg_snapshot_get_enable("log_pstore")) {
		dss_ramoops_data.mem_size = dbg_snapshot_get_item_size("log_pstore");
		dss_ramoops_data.mem_address = dbg_snapshot_get_item_paddr("log_pstore");
		dss_ramoops_data.pmsg_size = dss_ramoops_data.mem_size / 2;
		dss_ramoops_data.record_size = dss_ramoops_data.mem_size / 2;
	}
	return platform_device_register(&dss_ramoops);
}

static void __exit dss_pstore_exit(void)
{
	platform_device_unregister(&dss_ramoops);
}
module_init(dss_pstore_init);
module_exit(dss_pstore_exit);

MODULE_DESCRIPTION("Exynos Snapshot pstore module");
MODULE_LICENSE("GPL");
