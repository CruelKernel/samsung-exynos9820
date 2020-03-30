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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/memblock.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/sched/clock.h>

#include "debug-snapshot-local.h"

/* To Support Samsung SoC */
#include <dt-bindings/soc/samsung/debug-snapshot-table.h>
#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
#include <soc/samsung/cal-if.h>
#endif
#ifdef CONFIG_SEC_DEBUG
#include <linux/sec_debug.h>
#endif
#ifdef CONFIG_SEC_PM_DEBUG
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#endif

#include <linux/sec_debug.h>

extern void register_hook_logbuf(void (*)(const char *, size_t));
extern void register_hook_logger(void (*)(const char *, const char *, size_t));

struct dbg_snapshot_interface {
	struct dbg_snapshot_log *info_event;
	struct dbg_snapshot_item info_log[DSS_ITEM_MAX_NUM];
};

#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
struct dbg_snapshot_ops {
        int (*pd_status)(unsigned int id);
};

struct dbg_snapshot_ops dss_ops = {
	.pd_status = cal_pd_status,
};
#endif

const char *debug_level_val[] = {
	"low",
	"mid",
};

struct dbg_snapshot_bl *dss_bl;
struct dbg_snapshot_item dss_items[] = {
	{"header",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_kernel",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_platform",	{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_sfr",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_s2d",		{0, 0, 0, false, false}, NULL, NULL, 0, },
	{"log_cachedump",	{0, 0, 0, false, false}, NULL, NULL, 0, },
	{"log_arraydump",	{0, 0, 0, false, false}, NULL, NULL, 0, },
	{"log_etm",		{0, 0, 0, true, false}, NULL ,NULL, 0, },
	{"log_bcm",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_llc",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_dbgc",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
	{"log_pstore",		{0, 0, 0, true, false}, NULL ,NULL, 0, },
	{"log_kevents",		{0, 0, 0, false, false}, NULL ,NULL, 0, },
};

/*  External interface variable for trace debugging */
static struct dbg_snapshot_interface dss_info __attribute__ ((used));
static struct dbg_snapshot_interface *ess_info __attribute__ ((used));

struct dbg_snapshot_base dss_base;
struct dbg_snapshot_base ess_base;
struct dbg_snapshot_log *dss_log = NULL;
struct dbg_snapshot_desc dss_desc;

void sec_debug_get_kevent_info(struct ess_info_offset *p, int type)
{
	unsigned long kevent_base_va = (unsigned long)(dss_log->task);
	unsigned long kevent_base_pa = dss_items[dss_desc.kevents_num].entry.paddr;

	switch (type) {
	case DSS_KEVENT_TASK:
		p->base = kevent_base_pa + (unsigned long)(dss_log->task) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __task_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_WORK:
		p->base = kevent_base_pa + (unsigned long)(dss_log->work) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __work_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_IRQ:
		p->base = kevent_base_pa + (unsigned long)(dss_log->irq) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM * 2;
		p->size = sizeof(struct __irq_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_FREQ:
		p->base = kevent_base_pa + (unsigned long)(dss_log->freq) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __freq_log);
		p->per_core = 0;
		break;

	case DSS_KEVENT_IDLE:
		p->base = kevent_base_pa + (unsigned long)(dss_log->cpuidle) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __cpuidle_log);
		p->per_core = 1;
		break;

	case DSS_KEVENT_THRM:
		p->base = kevent_base_pa + (unsigned long)(dss_log->thermal) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __thermal_log);
		p->per_core = 0;
		break;

	case DSS_KEVENT_ACPM:
		p->base = kevent_base_pa + (unsigned long)(dss_log->acpm) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __acpm_log);
		p->per_core = 0;
		break;

	case DSS_KEVENT_MFRQ:
		p->base = kevent_base_pa + (unsigned long)(dss_log->freq_misc) - kevent_base_va;
		p->nr = DSS_LOG_MAX_NUM;
		p->size = sizeof(struct __freq_misc_log);
		p->per_core = 0;
		break;

	default:
		p->base = 0;
		p->nr = 0;
		p->size = 0;
		p->per_core = 0;
		break;
	}

	p->last = sec_debug_get_kevent_index_addr(type);
}

int dbg_snapshot_get_debug_level(void)
{
	return dss_desc.debug_level;
}

int dbg_snapshot_add_bl_item_info(const char *name, unsigned int paddr, unsigned int size)
{
	if (dss_bl->item_count >= SZ_16)
		return -1;

	memcpy(dss_bl->item[dss_bl->item_count].name, name, strlen(name) + 1);
	dss_bl->item[dss_bl->item_count].paddr = paddr;
	dss_bl->item[dss_bl->item_count].size = size;
	dss_bl->item[dss_bl->item_count].enabled = 1;
	dss_bl->item_count++;

	return 0;
}

int dbg_snapshot_set_enable(const char *name, int en)
{
	struct dbg_snapshot_item *item = NULL;
	unsigned long i;

	if (!strncmp(name, "base", strlen(name))) {
		dss_base.enabled = en;
		pr_info("debug-snapshot: %sabled\n", en ? "en" : "dis");
	} else {
		for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
			if (!strncmp(dss_items[i].name, name, strlen(name))) {
				item = &dss_items[i];
				item->entry.enabled = en;
				item->time = local_clock();
				pr_info("debug-snapshot: item - %s is %sabled\n",
						name, en ? "en" : "dis");
				break;
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(dbg_snapshot_set_enable);

int dbg_snapshot_try_enable(const char *name, unsigned long long duration)
{
	struct dbg_snapshot_item *item = NULL;
	unsigned long long time;
	unsigned long i;
	int ret = -1;

	/* If DSS was disabled, just return */
	if (unlikely(!dss_base.enabled) || !dbg_snapshot_get_enable("header"))
		return ret;

	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (!strncmp(dss_items[i].name, name, strlen(name))) {
			item = &dss_items[i];

			/* We only interest in disabled */
			if (!item->entry.enabled) {
				time = local_clock() - item->time;
				if (time > duration) {
					item->entry.enabled = true;
					ret = 1;
				} else
					ret = 0;
			}
			break;
		}
	}
	return ret;
}
EXPORT_SYMBOL(dbg_snapshot_try_enable);

int dbg_snapshot_get_enable(const char *name)
{
	struct dbg_snapshot_item *item = NULL;
	unsigned long i;
	int ret = 0;

	if (!strncmp(name, "base", strlen(name)))
		return dss_base.enabled;

	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (!strncmp(dss_items[i].name, name, strlen(name))) {
			item = &dss_items[i];
			ret = item->entry.enabled;
			break;
		}
	}
	return ret;
}
EXPORT_SYMBOL(dbg_snapshot_get_enable);

static inline int dbg_snapshot_check_eob(struct dbg_snapshot_item *item,
						size_t size)
{
	size_t max, cur;

	max = (size_t)(item->head_ptr + item->entry.size);
	cur = (size_t)(item->curr_ptr + size);

	if (unlikely(cur > max))
		return -1;
	else
		return 0;
}

static inline void dbg_snapshot_hook_logger(const char *name,
					 const char *buf, size_t size)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.log_platform_num];

	if (likely(dss_base.enabled && item->entry.enabled)) {
		if (unlikely((dbg_snapshot_check_eob(item, size))))
			item->curr_ptr = item->head_ptr;
		memcpy(item->curr_ptr, buf, size);
		item->curr_ptr += size;
	}
}

#ifdef CONFIG_SEC_PM_DEBUG
static bool sec_log_full;
#endif

size_t dbg_snapshot_get_curr_ptr_for_sysrq(void)
{
#ifdef CONFIG_SEC_DEBUG_SYSRQ_KMSG
	struct dbg_snapshot_item *item = &dss_items[dss_desc.log_kernel_num];

	return (size_t)item->curr_ptr;
#else
	return 0;
#endif
}

static inline void dbg_snapshot_hook_logbuf(const char *buf, size_t size)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.log_kernel_num];

	if (likely(dss_base.enabled && item->entry.enabled)) {
		size_t last_buf;

		if (dbg_snapshot_check_eob(item, size)) {
			item->curr_ptr = item->head_ptr;
#ifdef CONFIG_SEC_DEBUG_LAST_KMSG
			*((unsigned long long *)(item->head_ptr + item->entry.size - (size_t)0x08)) = SEC_LKMSG_MAGICKEY;
#endif
#ifdef CONFIG_SEC_PM_DEBUG
			if (unlikely(!sec_log_full))
				sec_log_full = true;
#endif
		}

		memcpy(item->curr_ptr, buf, size);
		item->curr_ptr += size;
		/*  save the address of last_buf to physical address */
		last_buf = (size_t)item->curr_ptr;

		__raw_writel(item->entry.paddr + (last_buf - item->entry.vaddr),
				dbg_snapshot_get_base_vaddr() + DSS_OFFSET_LAST_LOGBUF);
	}
}

#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
static bool dbg_snapshot_check_pmu(struct dbg_snapshot_sfrdump *sfrdump,
						const struct device_node *np)
{
	int ret = 0, count, i;
	unsigned int val;

	if (!sfrdump->pwr_mode)
		return true;

	count = of_property_count_u32_elems(np, "cal-pd-id");
	for (i = 0; i < count; i++) {
		ret = of_property_read_u32_index(np, "cal-pd-id", i, &val);
		if (ret < 0) {
			pr_err("failed to get pd-id - %s\n", sfrdump->name);
			return false;
		}
		ret = dss_ops.pd_status(val);
		if (ret < 0) {
			pr_err("not powered - %s (pd-id: %d)\n", sfrdump->name, i);
			return false;
		}
	}
	return true;
}

void dbg_snapshot_dump_sfr(void)
{
	struct dbg_snapshot_sfrdump *sfrdump;
	struct dbg_snapshot_item *item = &dss_items[dss_desc.log_sfr_num];
	struct list_head *entry;
	struct device_node *np;
	unsigned int reg, offset, val, size;
	int i, ret;
	static char buf[SZ_64];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;

	if (list_empty(&dss_desc.sfrdump_list)) {
		pr_emerg("debug-snapshot: %s: No information\n", __func__);
		return;
	}

	list_for_each(entry, &dss_desc.sfrdump_list) {
		sfrdump = list_entry(entry, struct dbg_snapshot_sfrdump, list);
		np = of_node_get(sfrdump->node);
		ret = dbg_snapshot_check_pmu(sfrdump, np);
		if (!ret)
			/* may off */
			continue;

		for (i = 0; i < sfrdump->num; i++) {
			ret = of_property_read_u32_index(np, "addr", i, &reg);
			if (ret < 0) {
				pr_err("debug-snapshot: failed to get address information - %s\n",
					sfrdump->name);
				break;
			}
			if (reg == 0xFFFFFFFF || reg == 0)
				break;
			offset = reg - sfrdump->phy_reg;
			if (reg < offset) {
				pr_err("debug-snapshot: invalid address information - %s: 0x%08x\n",
				sfrdump->name, reg);
				break;
			}
			val = __raw_readl(sfrdump->reg + offset);
			snprintf(buf, SZ_64, "0x%X = 0x%0X\n",reg, val);
			size = (unsigned int)strlen(buf);
			if (unlikely((dbg_snapshot_check_eob(item, size))))
				item->curr_ptr = item->head_ptr;
			memcpy(item->curr_ptr, buf, strlen(buf));
			item->curr_ptr += strlen(buf);
		}
		of_node_put(np);
		pr_info("debug-snapshot: complete to dump %s\n", sfrdump->name);
	}

}

static int dbg_snapshot_sfr_dump_init(struct device_node *np)
{
	struct device_node *dump_np;
	struct dbg_snapshot_sfrdump *sfrdump;
	char *dump_str;
	int count, ret, i;
	u32 phy_regs[2];

	ret = of_property_count_strings(np, "sfr-dump-list");
	if (ret < 0) {
		pr_err("failed to get sfr-dump-list\n");
		return ret;
	}
	count = ret;

	INIT_LIST_HEAD(&dss_desc.sfrdump_list);
	for (i = 0; i < count; i++) {
		ret = of_property_read_string_index(np, "sfr-dump-list", i,
						(const char **)&dump_str);
		if (ret < 0) {
			pr_err("failed to get sfr-dump-list\n");
			continue;
		}

		dump_np = of_get_child_by_name(np, dump_str);
		if (!dump_np) {
			pr_err("failed to get %s node, count:%d\n", dump_str, count);
			continue;
		}

		sfrdump = kzalloc(sizeof(struct dbg_snapshot_sfrdump), GFP_KERNEL);
		if (!sfrdump) {
			pr_err("failed to get memory region of dbg_snapshot_sfrdump\n");
			of_node_put(dump_np);
			continue;
		}

		ret = of_property_read_u32_array(dump_np, "reg", phy_regs, 2);
		if (ret < 0) {
			pr_err("failed to get register information\n");
			of_node_put(dump_np);
			kfree(sfrdump);
			continue;
		}

		sfrdump->reg = ioremap(phy_regs[0], phy_regs[1]);
		if (!sfrdump->reg) {
			pr_err("failed to get i/o address %s node\n", dump_str);
			of_node_put(dump_np);
			kfree(sfrdump);
			continue;
		}
		sfrdump->name = dump_str;

		ret = of_property_count_u32_elems(dump_np, "addr");
		if (ret < 0) {
			pr_err("failed to get addr count\n");
			of_node_put(dump_np);
			kfree(sfrdump);
			continue;
		}
		sfrdump->phy_reg = phy_regs[0];
		sfrdump->num = ret;

		ret = of_property_count_u32_elems(dump_np, "cal-pd-id");
		if (ret < 0)
			sfrdump->pwr_mode = false;
		else
			sfrdump->pwr_mode = true;

		sfrdump->node = dump_np;
		list_add(&sfrdump->list, &dss_desc.sfrdump_list);

		pr_info("success to regsiter %s\n", sfrdump->name);
		of_node_put(dump_np);
		ret = 0;
	}
	return ret;
}
#endif

static int __init dbg_snapshot_remap(void)
{
	unsigned long i, j;
	unsigned long flags = VM_NO_GUARD | VM_MAP;
	unsigned int enabled_count = 0;
	pgprot_t prot = __pgprot(PROT_NORMAL_NC);
	int page_size;
	struct page *page;
	struct page **pages;
	void *vaddr;

	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (dss_items[i].entry.enabled) {
			enabled_count++;
			page_size = dss_items[i].entry.size / PAGE_SIZE;
			pages = kzalloc(sizeof(struct page *) * page_size, GFP_KERNEL);
			page = phys_to_page(dss_items[i].entry.paddr);
			pr_info("%s: %2d: paddr: 0x%x\n", __func__, i, dss_items[i].entry.paddr);

			for (j = 0; j < page_size; j++)
				pages[j] = page++;

			vaddr = vmap(pages, page_size, flags, prot);
			kfree(pages);
			if (!vaddr) {
				pr_err("debug-snapshot: failed to mapping between virt and phys");
				return -ENOMEM;
			}

			dss_items[i].entry.vaddr = (size_t)vaddr;
			dss_items[i].head_ptr = (unsigned char *)dss_items[i].entry.vaddr;
			dss_items[i].curr_ptr = (unsigned char *)dss_items[i].entry.vaddr;

			if (strnstr(dss_items[i].name, "header", strlen("header")))
				dss_base.vaddr = dss_items[i].entry.vaddr;
		}
	}
	dss_desc.log_cnt = ARRAY_SIZE(dss_items);
	return enabled_count;
}

static int __init dbg_snapshot_init_desc(void)
{
	unsigned int i;
	unsigned long len;

	/* initialize dss_desc */
	memset((struct dbg_snapshot_desc *)&dss_desc, 0, sizeof(struct dbg_snapshot_desc));
	dss_desc.callstack = CONFIG_DEBUG_SNAPSHOT_CALLSTACK;
	raw_spin_lock_init(&dss_desc.ctrl_lock);
	raw_spin_lock_init(&dss_desc.nmi_lock);

	for (i = 0; i < (unsigned int)ARRAY_SIZE(dss_items); i++) {
		len = strlen(dss_items[i].name);
		if (!strncmp(dss_items[i].name, "header", len))
			dss_desc.header_num = i;
		else if (!strncmp(dss_items[i].name, "log_kevents", len))
			dss_desc.kevents_num = i;
		else if (!strncmp(dss_items[i].name, "log_kernel", len))
			dss_desc.log_kernel_num = i;
		else if (!strncmp(dss_items[i].name, "log_platform", len))
			dss_desc.log_platform_num = i;
		else if (!strncmp(dss_items[i].name, "log_sfr", len))
			dss_desc.log_sfr_num = i;
		else if (!strncmp(dss_items[i].name, "log_pstore", len))
			dss_desc.log_pstore_num = i;
	}

#ifdef CONFIG_S3C2410_WATCHDOG
	dss_desc.no_wdt_dev = false;
#else
	dss_desc.no_wdt_dev = true;
#endif
	return 0;
}

#ifdef CONFIG_OF_RESERVED_MEM
static int __init dbg_snapshot_item_reserved_mem_setup(struct reserved_mem *remem)
{
	unsigned int i;

	for (i = 0; i < (unsigned int)ARRAY_SIZE(dss_items); i++) {
		if (strnstr(remem->name, dss_items[i].name, strlen(remem->name)))
			break;
	}

	if (i == ARRAY_SIZE(dss_items))
		return -ENODEV;

	dss_items[i].entry.paddr = remem->base;
	dss_items[i].entry.size = remem->size;
	dss_items[i].entry.enabled = true;

	if (strnstr(remem->name, "header", strlen(remem->name))) {
		dss_base.paddr = remem->base;
		ess_base = dss_base;
		dss_base.enabled = false;
	}
	dss_base.size += remem->size;
	return 0;
}

#define DECLARE_DBG_SNAPSHOT_RESERVED_REGION(compat, name) \
RESERVEDMEM_OF_DECLARE(name, compat#name, dbg_snapshot_item_reserved_mem_setup)

DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", header);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_kernel);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_platform);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_sfr);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_s2d);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_cachedump);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_arraydump);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_etm);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_bcm);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_llc);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_dbgc);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_pstore);
DECLARE_DBG_SNAPSHOT_RESERVED_REGION("debug-snapshot,", log_kevents);
#endif

/*
 *  ---------------------------------------------------------------------
 *  - dummy data:phy_addr, virtual_addr, buffer_size, magic_key(4K)	-
 *  ---------------------------------------------------------------------
 *  -		Cores MMU register(4K)					-
 *  ---------------------------------------------------------------------
 *  -		Cores CPU register(4K)					-
 *  ---------------------------------------------------------------------
 */
static int __init dbg_snapshot_output(void)
{
	unsigned long i, size = 0;

	pr_info("debug-snapshot physical / virtual memory layout:\n");
	for (i = 0; i < ARRAY_SIZE(dss_items); i++) {
		if (dss_items[i].entry.enabled)
			pr_info("%-12s: phys:0x%zx / virt:0x%zx / size:0x%zx\n",
				dss_items[i].name,
				dss_items[i].entry.paddr,
				dss_items[i].entry.vaddr,
				dss_items[i].entry.size);
		size += dss_items[i].entry.size;
	}

	pr_info("total_item_size: %ldKB, dbg_snapshot_log struct size: %dKB\n",
			size / SZ_1K, dbg_snapshot_log_size / SZ_1K);

	return 0;
}

/*	Header dummy data(4K)
 *	-------------------------------------------------------------------------
 *		0		4		8		C
 *	-------------------------------------------------------------------------
 *	0	vaddr	phy_addr	size		magic_code
 *	4	Scratch_val	logbuf_addr	0		0
 *	-------------------------------------------------------------------------
*/

static void __init dbg_snapshot_fixmap_header(void)
{
	/*  fill 0 to next to header */
	size_t vaddr, paddr, size;
	size_t *addr;

	vaddr = dss_items[dss_desc.header_num].entry.vaddr;
	paddr = dss_items[dss_desc.header_num].entry.paddr;
	size = dss_items[dss_desc.header_num].entry.size;

	/*  set to confirm debug-snapshot */
	addr = (size_t *)vaddr;
	memcpy(addr, &dss_base, sizeof(struct dbg_snapshot_base));

	if (!dbg_snapshot_get_enable("header"))
		return;

	/*  initialize kernel event to 0 except only header */
	memset((size_t *)(vaddr + DSS_KEEP_HEADER_SZ), 0, size - DSS_KEEP_HEADER_SZ);

	dss_bl = dbg_snapshot_get_base_vaddr() + DSS_OFFSET_ITEM_INFO;
	memset(dss_bl, 0, sizeof(struct dbg_snapshot_bl));
	dss_bl->magic1 = 0x01234567;
	dss_bl->magic2 = 0x89ABCDEF;
	dss_bl->item_count = ARRAY_SIZE(dss_items);
	memcpy(dss_bl->item[dss_desc.header_num].name,
			dss_items[dss_desc.header_num].name,
			strlen(dss_items[dss_desc.header_num].name) + 1);
	dss_bl->item[dss_desc.header_num].paddr = paddr;
	dss_bl->item[dss_desc.header_num].size = size;
	dss_bl->item[dss_desc.header_num].enabled =
		dss_items[dss_desc.header_num].entry.enabled;
}

static void __init dbg_snapshot_fixmap(void)
{
	size_t last_buf;
	size_t vaddr, paddr, size;
	unsigned long i;

	/*  fixmap to header first */
	dbg_snapshot_fixmap_header();

	for (i = 1; i < ARRAY_SIZE(dss_items); i++) {
		memcpy(dss_bl->item[i].name, dss_items[i].name,	strlen(dss_items[i].name) + 1);
		dss_bl->item[i].enabled = dss_items[i].entry.enabled;

		if (!dss_items[i].entry.enabled)
			continue;

		/*  assign dss_item information */
		paddr = dss_items[i].entry.paddr;
		vaddr = dss_items[i].entry.vaddr;
		size = dss_items[i].entry.size;

		if (i == dss_desc.log_kernel_num) {
			/*  load last_buf address value(phy) by virt address */
			last_buf = (size_t)__raw_readl(dbg_snapshot_get_base_vaddr() +
							DSS_OFFSET_LAST_LOGBUF);
			/*  check physical address offset of kernel logbuf */
			if (last_buf >= dss_items[i].entry.paddr &&
				(last_buf) <= (dss_items[i].entry.paddr + dss_items[i].entry.size)) {
				/*  assumed valid address, conversion to virt */
				dss_items[i].curr_ptr = (unsigned char *)(dss_items[i].entry.vaddr +
							(last_buf - dss_items[i].entry.paddr));
			} else {
				/*  invalid address, set to first line */
				dss_items[i].curr_ptr = (unsigned char *)vaddr;
				/*  initialize logbuf to 0 */
				memset((size_t *)vaddr, 0, size);
			}
		} else {
			/*  initialized log to 0 if persist == false */
			if (!dss_items[i].entry.persist)
				memset((size_t *)vaddr, 0, size);
		}
		dss_info.info_log[i - 1].name = kstrdup(dss_items[i].name, GFP_KERNEL);
		dss_info.info_log[i - 1].head_ptr = (unsigned char *)dss_items[i].entry.vaddr;
		dss_info.info_log[i - 1].curr_ptr = NULL;
		dss_info.info_log[i - 1].entry.size = size;

		memcpy(dss_bl->item[i].name,  dss_items[i].name, strlen(dss_items[i].name) + 1);
		dss_bl->item[i].paddr = paddr;
		dss_bl->item[i].size = size;
	}

	dss_log = (struct dbg_snapshot_log *)(dss_items[dss_desc.kevents_num].entry.vaddr);

	/*  set fake translation to virtual address to debug trace */
	dss_info.info_event = dss_log;
	ess_info = &dss_info;

	/* output the information of debug-snapshot */
	dbg_snapshot_output();
	
#ifdef CONFIG_SEC_DEBUG
	sec_debug_save_last_kmsg(dss_items[dss_desc.log_kernel_num].head_ptr,
			dss_items[dss_desc.log_kernel_num].curr_ptr,
			dss_items[dss_desc.log_kernel_num].entry.size);
#endif
}

static int dbg_snapshot_init_dt_parse(struct device_node *np)
{
	int ret = 0;
	struct device_node *sfr_dump_np;

	if (of_property_read_u32(np, "use_multistage_wdt_irq",
				&dss_desc.multistage_wdt_irq)) {
		dss_desc.multistage_wdt_irq = 0;
		pr_err("debug-snapshot: no support multistage_wdt\n");
	}

	sfr_dump_np = of_get_child_by_name(np, "dump-info");
	if (!sfr_dump_np) {
		pr_err("debug-snapshot: failed to get dump-info node\n");
		ret = -ENODEV;
	} else {
#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
		ret = dbg_snapshot_sfr_dump_init(sfr_dump_np);
		if (ret < 0) {
			pr_err("debug-snapshot: failed to register sfr dump node\n");
			ret = -ENODEV;
			of_node_put(sfr_dump_np);
		}
#endif
	}
	if (ret < 0)
		dbg_snapshot_set_enable("log_sfr", false);

	of_node_put(np);
	return ret;
}

static const struct of_device_id dss_of_match[] __initconst = {
	{ .compatible 	= "debug-snapshot-soc",
	  .data		= dbg_snapshot_init_dt_parse},
	{},
};

static int __init dbg_snapshot_init_dt(void)
{
	struct device_node *np;
	const struct of_device_id *matched_np;
	dss_initcall_t init_fn;

	np = of_find_matching_node_and_match(NULL, dss_of_match, &matched_np);

	if (!np) {
		pr_info("debug-snapshot: couldn't find device tree file of debug-snapshot\n");
		dbg_snapshot_set_enable("log_sfr", false);
		return -ENODEV;
	}

	init_fn = (dss_initcall_t)matched_np->data;
	return init_fn(np);
}

static int __init dbg_snapshot_init_value(void)
{
	dss_desc.debug_level = dbg_snapshot_get_debug_level_reg();

	pr_info("debug-snapshot: debug_level [%s]\n",
		debug_level_val[dss_desc.debug_level]);

	if (dss_desc.debug_level != DSS_DEBUG_LEVEL_LOW)
		dbg_snapshot_scratch_reg(DSS_SIGN_SCRATCH);

	dbg_snapshot_set_sjtag_status();

	/* copy linux_banner, physical address of
	 * kernel log / platform log / kevents to DSS header */
	strncpy(dbg_snapshot_get_base_vaddr() + DSS_OFFSET_LINUX_BANNER,
		linux_banner, strlen(linux_banner));

	return 0;
}

static int __init dbg_snapshot_init(void)
{
	dbg_snapshot_init_desc();
	if (dbg_snapshot_remap() > 0) {
	/*
	 *  for debugging when we don't know the virtual address of pointer,
	 *  In just privous the debug buffer, It is added 16byte dummy data.
	 *  start address(dummy 16bytes)
	 *  --> @virtual_addr | @phy_addr | @buffer_size | @magic_key(0xDBDBDBDB)
	 *  And then, the debug buffer is shown.
	 */
		dbg_snapshot_init_log_idx();
		dbg_snapshot_fixmap();
		dbg_snapshot_init_dt();
		dbg_snapshot_init_helper();
		dbg_snapshot_init_utils();
		dbg_snapshot_init_value();

		dbg_snapshot_set_enable("base", true);

		register_hook_logbuf(dbg_snapshot_hook_logbuf);
		register_hook_logger(dbg_snapshot_hook_logger);
	} else
		pr_err("debug-snapshot: %s failed\n", __func__);

	return 0;
}
early_initcall(dbg_snapshot_init);

#ifdef CONFIG_SEC_PM_DEBUG
static ssize_t sec_log_read_all(struct file *file, char __user *buf,
				size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;
	size_t size;
	struct dbg_snapshot_item *item = &dss_items[dss_desc.log_kernel_num];

	if (sec_log_full)
		size = item->entry.size;
	else
		size = (size_t)(item->curr_ptr - item->head_ptr);

	if (pos >= size)
		return 0;

	count = min(len, size);

	if ((pos + count) > size)
		count = size - pos;

	if (copy_to_user(buf, item->head_ptr + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations sec_log_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_log_read_all,
};

static int __init sec_log_late_init(void)
{
	struct proc_dir_entry *entry;
	struct dbg_snapshot_item *item = &dss_items[dss_desc.log_kernel_num];

	if (!item->head_ptr)
		return 0;

	entry = proc_create("sec_log", 0440, NULL, &sec_log_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	proc_set_size(entry, item->entry.size);

	return 0;
}

late_initcall(sec_log_late_init);
#endif /* CONFIG_SEC_PM_DEBUG */
