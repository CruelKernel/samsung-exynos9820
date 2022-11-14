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

#ifndef __TZDEV_H__
#define __TZDEV_H__

#include <linux/compiler.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/version.h>

#include "tz_cdev.h"
#include "tz_common.h"
#include "tz_iwlog.h"
#include "tz_iwlog_polling.h"
#include "tz_hotplug.h"
#include "tzdev_smc.h"
#include "tzlog.h"
#include "tzprofiler.h"

#define TZDEV_DRIVER_VERSION(a,b,c)	(((a) << 16) | ((b) << 8) | (c))
#define TZDEV_DRIVER_CODE		TZDEV_DRIVER_VERSION(3,0,0)

#define SMC_NO(x)			"" # x
#define SMC(x)				"smc " SMC_NO(x)

#if defined(CONFIG_ARM)
#define REGISTERS_NAME	"r"
#define ARCH_EXTENSION	".arch_extension sec\n"
#elif defined(CONFIG_ARM64)
#define REGISTERS_NAME	"x"
#define ARCH_EXTENSION	""
#endif /* CONFIG_ARM */

#if defined(CONFIG_TZDEV_USE_ARM_CALLING_CONVENTION)
#define TZDEV_SMC_MAGIC			0
#define TZDEV_SMC_COMMAND(fid)		CREATE_SMC_CMD(SMC_TYPE_FAST, SMC_AARCH_32, SMC_TOS0_SERVICE_MASK, (fid))
#else /* CONFIG_TZDEV_USE_ARM_CALLING_CONVENTION */
#define TZDEV_SMC_MAGIC			(0xE << 16)
#define TZDEV_SMC_COMMAND(fid)		(fid)
#endif /* CONFIG_TZDEV_USE_ARM_CALLING_CONVENTION */

#define TZDEV_SMC_CONNECT_AUX		TZDEV_SMC_COMMAND(0)
#define TZDEV_SMC_CONNECT		TZDEV_SMC_COMMAND(1)
#define TZDEV_SMC_TELEMETRY_CONTROL	TZDEV_SMC_COMMAND(2)
#define TZDEV_SMC_SCHEDULE		TZDEV_SMC_COMMAND(3)
#define TZDEV_SMC_SHMEM_LIST_REG	TZDEV_SMC_COMMAND(6)
#define TZDEV_SMC_SHMEM_LIST_RLS	TZDEV_SMC_COMMAND(7)
#define TZDEV_SMC_SYSCONF		TZDEV_SMC_COMMAND(9)
#define TZDEV_SMC_TZ_PANIC_DUMP_INIT	TZDEV_SMC_COMMAND(12)
#define TZDEV_SMC_CHECK_VERSION		TZDEV_SMC_COMMAND(13)
#define TZDEV_SMC_MEM_REG		TZDEV_SMC_COMMAND(14)
#define TZDEV_SMC_BOOT_LOG_READ		TZDEV_SMC_COMMAND(16)
#define TZDEV_SMC_PROFILER_CONTROL	TZDEV_SMC_COMMAND(17)

struct tzdev_fd_data {
	unsigned int crypto_clk_state;
	unsigned int boost_state;
	struct mutex mutex;
};

/* Define type for exchange with Secure kernel */
#if defined(CONFIG_TZDEV_SK_PFNS_64BIT)
typedef	uint64_t	sk_pfn_t;
#else /* CONFIG_TZDEV_SK_PFNS_64BIT */
typedef	uint32_t	sk_pfn_t;
#endif /* CONFIG_TZDEV_SK_PFNS_64BIT */

#if defined(CONFIG_TZDEV_SK_MULTICORE)
#define NR_SW_CPU_IDS	nr_cpu_ids
#else /* CONFIG_TZDEV_SK_MULTICORE */
#define NR_SW_CPU_IDS	1
#endif /* CONFIG_TZDEV_SK_MULTICORE */

#define NR_SMC_ARGS	7

struct tzdev_smc_data {
	uint32_t args[NR_SMC_ARGS];
};

int __tzdev_smc_cmd(struct tzdev_smc_data *data);

#define tzdev_smc_cmd(p0, p1, p2, p3, p4, p5, p6)				\
({										\
	int ret;								\
	struct tzdev_smc_data data = { .args = {p0, p1, p2, p3, p4, p5, p6} };	\
										\
	ret = __tzdev_smc_cmd(&data);						\
										\
	if (!ret)								\
		ret = data.args[0];						\
										\
	ret;									\
})

#define tzdev_smc_cmd_extended(p0, p1, p2, p3, p4, p5, p6)			\
({										\
	int ret;								\
	struct tzdev_smc_data data = { .args = {p0, p1, p2, p3, p4, p5, p6} };	\
										\
	ret = __tzdev_smc_cmd(&data);						\
										\
	if (ret)								\
		data.args[0] = ret;						\
										\
	data;									\
})

#define tzdev_smc_schedule()					tzdev_smc_cmd(TZDEV_SMC_SCHEDULE, 0, 0, 0, 0, 0, 0)
#define tzdev_smc_connect_aux(pfn)				tzdev_smc_cmd(TZDEV_SMC_CONNECT_AUX, (pfn), 0, 0, 0, 0, 0)
#define tzdev_smc_connect(mode, nr_pages)			tzdev_smc_cmd(TZDEV_SMC_CONNECT, (mode), (nr_pages), 0, 0, 0, 0)
#define tzdev_smc_telemetry_control(mode, type, arg)		tzdev_smc_cmd(TZDEV_SMC_TELEMETRY_CONTROL, (mode), (type), (arg), 0, 0, 0)
#define tzdev_smc_shmem_list_reg(id, total_pfns, flags)		tzdev_smc_cmd(TZDEV_SMC_SHMEM_LIST_REG, (id), (total_pfns), (flags), 0, 0, 0)
#define tzdev_smc_shmem_list_rls(id)				tzdev_smc_cmd(TZDEV_SMC_SHMEM_LIST_RLS, (id), 0, 0, 0, 0, 0)
#define tzdev_smc_sysconf()					tzdev_smc_cmd(TZDEV_SMC_SYSCONF, 0, 0, 0, 0, 0, 0)
#define tzdev_smc_tz_panic_dump_init()				tzdev_smc_cmd(TZDEV_SMC_TZ_PANIC_DUMP_INIT, 0, 0, 0, 0, 0, 0)
#define tzdev_smc_check_version()				tzdev_smc_cmd(TZDEV_SMC_CHECK_VERSION, LINUX_VERSION_CODE, TZDEV_DRIVER_CODE, 0, 0, 0, 0)
#define tzdev_smc_mem_reg(pfn, order)				tzdev_smc_cmd(TZDEV_SMC_MEM_REG, (pfn), (order), 0, 0, 0, 0)
#define tzdev_smc_boot_log_read(pfn, nr_pages)			tzdev_smc_cmd(TZDEV_SMC_BOOT_LOG_READ, (pfn), (nr_pages), 0, 0, 0, 0)
#define tzdev_smc_profiler_control(cmd, arg)			tzdev_smc_cmd(TZDEV_SMC_PROFILER_CONTROL, (cmd), (arg), 0, 0, 0, 0)

int tzdev_run_init_sequence(void);
struct tz_cdev *tzdev_get_cdev(void);

#endif /* __TZDEV_H__ */
