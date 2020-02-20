/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Debug-SnapShot: Debug Framework for Ramdump based debugging method
 * The original code is Exynos-Snapshot for Exynos SoC
 *
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 * Author: Changki Kim <changki.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DEBUG_SNAPSHOT_LOCAL_H
#define DEBUG_SNAPSHOT_LOCAL_H
#include <linux/debug-snapshot.h>
#include <linux/debug-snapshot-helper.h>
#include "debug-snapshot-log.h"

#ifdef CONFIG_DEBUG_SNAPSHOT_LINUX_BUILD
#include <linux/clk-provider.h>
#endif

extern void (*arm_pm_restart)(char str, const char *cmd);

extern void dbg_snapshot_init_log_idx(void);
extern void dbg_snapshot_init_utils(void);
extern void dbg_snapshot_init_helper(void);
extern void __iomem *dbg_snapshot_get_base_vaddr(void);
extern void __iomem *dbg_snapshot_get_base_paddr(void);
extern void dbg_snapshot_scratch_reg(unsigned int val);
extern void dbg_snapshot_print_panic_report(void);
extern void dbg_snapshot_dump_task_info(void);

extern unsigned int dbg_snapshot_get_core_panic_stat(unsigned cpu);
extern void dbg_snapshot_set_core_panic_stat(unsigned int val, unsigned cpu);
extern void dbg_snapshot_recall_hardlockup_core(void);

extern struct dbg_snapshot_helper_ops *dss_soc_ops;

/* SoC Specific define, This will be removed */
#define DSS_REG_MCT_ADDR	(0)
#define DSS_REG_MCT_SIZE	(0)
#define DSS_REG_UART_ADDR	(0)
#define DSS_REG_UART_SIZE	(0)

typedef int (*dss_initcall_t)(const struct device_node *);

struct dbg_snapshot_base {
	size_t size;
	size_t vaddr;
	size_t paddr;
	unsigned int persist;
	unsigned int enabled;
};

struct dbg_snapshot_item {
	char *name;
	struct dbg_snapshot_base entry;
	unsigned char *head_ptr;
	unsigned char *curr_ptr;
	unsigned long long time;
};

struct dbg_snapshot_bl {
	unsigned int magic1;
	unsigned int magic2;
	unsigned int item_count;
	unsigned int reserved;
	struct {
		char name[SZ_16];
		unsigned int paddr;
		unsigned int size;
		unsigned int enabled;
	} item[SZ_16];
};

struct dbg_snapshot_sfrdump {
	char *name;
	void __iomem *reg;
	unsigned int phy_reg;
	unsigned int num;
	struct device_node *node;
	struct list_head list;
	bool pwr_mode;
};

struct dbg_snapshot_desc {
	struct list_head sfrdump_list;
	raw_spinlock_t ctrl_lock;
	raw_spinlock_t nmi_lock;
	unsigned int header_num;
	unsigned int kevents_num;
	unsigned int log_kernel_num;
	unsigned int log_platform_num;
	unsigned int log_sfr_num;
	unsigned int log_pstore_num;
	unsigned int log_etm_num;
	unsigned int log_cnt;

	unsigned int callstack;
	unsigned long hardlockup_core_mask;
	unsigned long hardlockup_core_pc[DSS_NR_CPUS];
	int multistage_wdt_irq;
	int hardlockup_detected;
	int allcorelockup_detected;
	int no_wdt_dev;
	int debug_level;
	int sjtag_status;
};


extern struct dbg_snapshot_base dss_base;
extern struct dbg_snapshot_log *dss_log;
extern struct dbg_snapshot_desc dss_desc;
extern struct dbg_snapshot_item dss_items[];
extern int dbg_snapshot_log_size;

#ifdef CONFIG_SEC_PM_DEBUG
extern ssize_t dss_log_work_print(char *buf);
#endif /* CONFIG_SEC_PM_DEBUG */
#endif
