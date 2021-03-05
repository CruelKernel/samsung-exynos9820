/*
 *sec_debug_extrainfo.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sec_debug.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched/clock.h>
#include <asm/stacktrace.h>
#include <asm/esr.h>

#include <soc/samsung/exynos-pmu.h>
#include <linux/sec_ext.h>

#define SDEI_DEBUG	(0)
#define EXTRA_VERSION	"RI25"

#include "sec_debug_extra_info_keys.c"
#include "sec_debug_internal.h"

static bool exin_ready;

static struct sec_debug_shared_buffer *sh_buf;
static void *slot_end_addr;

char *ftype_items[MAX_ITEM_VAL_LEN] = {
	"UNDF", "BAD", "WATCH", "KERN", "MEM",
	"SPPC", "PAGE", "AUF", "EUF", "AUOF",
	"BUG", "PANIC",
};

/*****************************************************************/
/*                        UNIT FUNCTIONS                         */
/*****************************************************************/
static int get_val_len(const char *s)
{
	if (s)
		return strnlen(s, MAX_ITEM_VAL_LEN);

	return 0;
}

static int get_key_len(const char *s)
{
	return strnlen(s, MAX_ITEM_KEY_LEN);
}

static void *get_slot_addr(int idx)
{
	return (void *)phys_to_virt(sh_buf->sec_debug_sbidx[idx].paddr);
}

static int get_max_len(void *p)
{
	int i;

	if (p < get_slot_addr(SLOT_32) || (p >= slot_end_addr)) {
		pr_crit("%s: addr(%p) is not in range\n", __func__, p);

		return 0;
	}

	for (i = SLOT_32 + 1; i < NR_SLOT; i++)
		if (p < get_slot_addr(i))
			return sh_buf->sec_debug_sbidx[i - 1].size;

	return sh_buf->sec_debug_sbidx[SLOT_END].size;
}

static void *__get_item(int slot, int idx)
{
	void *p, *base;
	unsigned int size, nr;

	size = sh_buf->sec_debug_sbidx[slot].size;
	nr = sh_buf->sec_debug_sbidx[slot].nr;

	if (nr <= idx) {
		pr_crit("%s: SLOT %d has %d items (%d)\n",
					__func__, slot, nr, idx);

		return NULL;
	}

	base = get_slot_addr(slot);
	p = (void *)(base + (idx * size));

	return p;
}

static void *search_item_by_key(const char *key, int start, int end)
{
	int s, i, keylen = get_key_len(key);
	void *p;
	char *keyname;
	unsigned int max;

	if (!sh_buf || !exin_ready) {
		pr_info("%s: (%s) extra_info is not ready\n", __func__, key);

		return NULL;
	}

	for (s = start; s < end; s++) {
		if (sh_buf->sec_debug_sbidx[s].cnt)
			max = sh_buf->sec_debug_sbidx[s].cnt;
		else
			max = sh_buf->sec_debug_sbidx[s].nr;

		for (i = 0; i < max; i++) {
			p = __get_item(s, i);
			if (!p)
				break;

			keyname = (char *)p;
			if (keylen != get_key_len(keyname))
				continue;

			if (!strncmp(keyname, key, keylen))
				return p;
		}
	}

	return NULL;
}

static void *get_item(const char *key)
{
	return search_item_by_key(key, SLOT_32, NR_MAIN_SLOT);
}

static void *get_bk_item(const char *key)
{
	return search_item_by_key(key, SLOT_BK_32, NR_SLOT);
}

static char *get_item_val(const char *key)
{
	void *p;

	p = get_item(key);
	if (!p) {
		pr_crit("%s: no key %s\n", __func__, key);

		return NULL;
	}

	return ((char *)p + MAX_ITEM_KEY_LEN);
}

char *get_bk_item_val(const char *key)
{
	void *p;

	p = get_bk_item(key);
	if (!p)
		return NULL;

	return ((char *)p + MAX_ITEM_KEY_LEN);
}

void get_bk_item_val_as_string(const char *key, char *buf)
{
	void *v;
	int len;

	v = get_bk_item_val(key);
	if (v) {
		len = get_val_len(v);
		if (len)
			memcpy(buf, v, len);
	}
}

static int is_key_in_blacklist(const char *key)
{
	char blkey[][MAX_ITEM_KEY_LEN] = {
		"KTIME", "BAT", "FTYPE", "ODR", "DDRID",
		"PSTIE", "ASB",
	};

	int nr_blkey, keylen, i;

	keylen = get_key_len(key);
	nr_blkey = ARRAY_SIZE(blkey);

	for (i = 0; i < nr_blkey; i++)
		if (!strncmp(key, blkey[i], keylen))
			return 1;

	return 0;
}

static DEFINE_SPINLOCK(keyorder_lock);

static void set_key_order(const char *key)
{
	void *p;
	char *v;
	char tmp[MAX_ITEM_VAL_LEN] = {0, };
	int max = MAX_ITEM_VAL_LEN;
	int len_prev, len_remain, len_this;

	if (is_key_in_blacklist(key))
		return;

	spin_lock(&keyorder_lock);

	p = get_item("ODR");
	if (!p) {
		pr_crit("%s: fail to find %s\n", __func__, key);

		goto unlock_keyorder;
	}

	max = get_max_len(p);
	if (!max) {
		pr_crit("%s: fail to get max len %s\n", __func__, key);

		goto unlock_keyorder;
	}

	v = get_item_val(p);

	/* keep previous value */
	len_prev = get_val_len(v);
	if ((!len_prev) || (MAX_ITEM_VAL_LEN <= len_prev))
		len_prev = MAX_ITEM_VAL_LEN - 1;

	snprintf(tmp, len_prev + 1, "%s", v);

	/* calculate the remained size */
	len_remain = max;

	/* get_item_val returned address without key */
	len_remain -= MAX_ITEM_KEY_LEN;

	/* need comma */
	len_this = get_key_len(key) + 1;
	len_remain -= len_this;

	/* put last key at the first of ODR */
	/* +1 to add NULL (by snprintf) */
	snprintf(v, len_this + 1, "%s,", key);

	/* -1 to remove NULL between KEYS */
	/* +1 to add NULL (by snprintf) */
	snprintf((char *)(v + len_this), len_remain + 1, tmp);

unlock_keyorder:
	spin_unlock(&keyorder_lock);
}

static void set_item_val(const char *key, const char *fmt, ...)
{
	va_list args;
	void *p;
	char *v;
	int max = MAX_ITEM_VAL_LEN;

	p = get_item(key);
	if (!p) {
		pr_crit("%s: fail to find %s\n", __func__, key);

		return;
	}

	max = get_max_len(p);
	if (!max) {
		pr_crit("%s: fail to get max len %s\n", __func__, key);

		return;
	}

	v = get_item_val(p);
	if (!get_val_len(v)) {
		va_start(args, fmt);
		vsnprintf(v, max, fmt, args);
		va_end(args);

		set_key_order(key);
	}
}

static void set_bk_item_val(const char *key, int slot, const char *fmt, ...)
{
	va_list args;
	void *p;
	char *v;
	int max = MAX_ITEM_VAL_LEN;
	unsigned int cnt;

	p = get_bk_item(key);
	if (!p) {
		pr_crit("%s: fail to find %s\n", __func__, key);

		if (slot > SLOT_MAIN_END) {
			cnt = sh_buf->sec_debug_sbidx[slot].cnt;
			sh_buf->sec_debug_sbidx[slot].cnt++;

			p = __get_item(slot, cnt);
			if (!p) {
				pr_crit("%s: slot%2d cnt: %d, fail\n", __func__, slot, cnt);

				return;
			}

			snprintf((char *)p, get_key_len(key) + 1, key);

			v = ((char *)p + MAX_ITEM_KEY_LEN);

			pr_crit("%s: add slot%2d cnt: %d (%s)\n", __func__, slot, cnt, (char *)p);

			goto set_val;
		}

		return;
	}

	max = get_max_len(p);
	if (!max) {
		pr_crit("%s: fail to get max len %s\n", __func__, key);

		if (slot > SLOT_MAIN_END) {
			max = MAX_ITEM_VAL_LEN;
		} else {
			pr_crit("%s: slot(%d) is not in bk slot\n", __func__, slot);

			return;
		}
	}

	v = get_bk_item_val(p);
	if (!v) {
		pr_crit("%s: fail to find value address for %s\n", __func__, key);

		return;
	} else {
		if (get_val_len(v)) {
			pr_crit("%s: some value is in %s\n", __func__, key);

			return;
		}
	}

set_val:
	va_start(args, fmt);
	vsnprintf(v, max, fmt, args);
	va_end(args);
}

static void clear_item_val(const char *key)
{
	void *p;
	int max_len;

	p = get_item(key);
	if (!p) {
		pr_crit("%s: fail to find %s\n", __func__, key);

		return;
	}

	max_len = get_max_len(p);
	if (!max_len) {
		pr_crit("%s: fail to get max len %s\n", __func__, key);

		return;
	}

	memset(get_item_val(p), 0, max_len - MAX_ITEM_KEY_LEN);
}

static void dump_slot(int type, void *ptr)
{
	struct seq_file *m;
	unsigned int cnt, i;
	char *p, *v;

	if (ptr)
		m = (struct seq_file *)ptr;
	else
		m = NULL;

	if (!exin_ready) {
		pr_crit("%s: EXIN is not ready\n", __func__);
		return;
	}

	/* temporally for backup slot */
	cnt = sh_buf->sec_debug_sbidx[type].cnt;

	for (i = 0; i < cnt; i++) {
		p = __get_item(type, i);
		if (!p)
			break;

		v = p + MAX_ITEM_KEY_LEN;

		if (m)
			seq_printf(m, "%s: %s - %s\n", __func__, p, v);
		else
			pr_crit("%s: %s - %s\n", __func__, p, v);
	}
}

static void dump_slots(int start, int end, void *ptr)
{
	int i;

	for (i = start; i < end; i++)
		dump_slot(i, ptr);
}


static void dump_all_keys(void)
{
	void *p;
	int s, i;
	unsigned int cnt;

	if (!exin_ready) {
		pr_crit("%s: EXIN is not ready\n", __func__);

		return;
	}

	for (s = 0; s < NR_SLOT; s++) {
		cnt = sh_buf->sec_debug_sbidx[s].cnt;

		for (i = 0; i < cnt; i++) {
			p = __get_item(s, i);
			if (!p) {
				pr_crit("%s: %d/%d: no item %p\n", __func__, s, i, p);
				break;
			}

			if (!get_key_len(p))
				break;

			pr_crit("%s: [%d][%02d] key %s\n",
							__func__, s, i, (char *)p);
		}
	}
}

/*****************************************************************/
/*                        INIT FUNCTIONS                         */
/*****************************************************************/
static void init_shared_buffer(int type, int nr_keys, void *ptr)
{
	char (* keys)[MAX_ITEM_KEY_LEN];
	unsigned int base, size, nr;
	void *addr;
	int i;

	keys = (char (*)[MAX_ITEM_KEY_LEN])ptr;

	base = sh_buf->sec_debug_sbidx[type].paddr;
	size = sh_buf->sec_debug_sbidx[type].size;
	nr = sh_buf->sec_debug_sbidx[type].nr;

	addr = phys_to_virt(base);
	memset(addr, 0, size * nr);

	pr_crit("%s: SLOT%d: nr keys: %d\n", __func__, type, nr_keys);

	for (i = 0; i < nr_keys; i++) {
		/* NULL is considered as +1 */
		snprintf((char *)addr, get_key_len(keys[i]) + 1, keys[i]);

		base += size;
		addr = phys_to_virt(base);
	}

	sh_buf->sec_debug_sbidx[type].cnt = i;
}

static void sec_debug_extra_info_key_init(void)
{
	int nr_keys;

	nr_keys = sizeof(key32) / sizeof(key32[0]);
	init_shared_buffer(SLOT_32, nr_keys, (void *)key32);

	nr_keys = sizeof(key64) / sizeof(key64[0]);
	init_shared_buffer(SLOT_64, nr_keys, (void *)key64);

	nr_keys = sizeof(key256) / sizeof(key256[0]);
	init_shared_buffer(SLOT_256, nr_keys, (void *)key256);

	nr_keys = sizeof(key1024) / sizeof(key1024[0]);
	init_shared_buffer(SLOT_1024, nr_keys, (void *)key1024);
}

static void sec_debug_extra_info_copy_shared_buffer(bool mflag)
{
	int i;
	unsigned int total_size = 0, slot_base;
	char *backup_base;

	for (i = 0; i < NR_MAIN_SLOT; i++)
		total_size += sh_buf->sec_debug_sbidx[i].nr * sh_buf->sec_debug_sbidx[i].size;

	slot_base = sh_buf->sec_debug_sbidx[SLOT_32].paddr;

	backup_base = phys_to_virt(slot_base + total_size);

	pr_crit("%s: dst: %p src: %p (%x)\n",
				__func__, backup_base, phys_to_virt(slot_base), total_size);

	memcpy(backup_base, phys_to_virt(slot_base), total_size);

	/* backup shared buffer header info */
	memcpy(&(sh_buf->sec_debug_sbidx[SLOT_BK_32]),
				&(sh_buf->sec_debug_sbidx[SLOT_32]),
				sizeof(struct sec_debug_sb_index) * (NR_SLOT - NR_MAIN_SLOT));

	for (i = SLOT_BK_32; i < NR_SLOT; i++) {
		sh_buf->sec_debug_sbidx[i].paddr += total_size;
		if (SDEI_DEBUG) {
			pr_crit("%s: SLOT %2d: paddr: 0x%x\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].paddr);
			pr_crit("%s: SLOT %2d: size: %d\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].size);
			pr_crit("%s: SLOT %2d: nr: %d\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].nr);
			pr_crit("%s: SLOT %2d: cnt: %d\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].cnt);
		}
	}
}

static void sec_debug_extra_info_dump_sb_index(void)
{
	int i;

	for (i = 0; i < NR_SLOT; i++) {
		pr_info("%s: SLOT%02d: paddr: %x\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].paddr);
		pr_info("%s: SLOT%02d: cnt: %d\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].cnt);
		pr_info("%s: SLOT%02d: blmark: %lx\n",
					__func__, i, sh_buf->sec_debug_sbidx[i].blmark);
		pr_info("\n");
	}
}

static void sec_debug_init_extra_info_sbidx(int type, struct sec_debug_sb_index idx, bool mflag)
{
	sh_buf->sec_debug_sbidx[type].paddr = idx.paddr;
	sh_buf->sec_debug_sbidx[type].size = idx.size;
	sh_buf->sec_debug_sbidx[type].nr = idx.nr;
	sh_buf->sec_debug_sbidx[type].cnt = idx.cnt;
	sh_buf->sec_debug_sbidx[type].blmark = 0;

	pr_crit("%s: slot: %d / paddr: 0x%x / size: %d / nr: %d\n",
					__func__, type,
					sh_buf->sec_debug_sbidx[type].paddr,
					sh_buf->sec_debug_sbidx[type].size,
					sh_buf->sec_debug_sbidx[type].nr);
}

static bool sec_debug_extra_info_check_magic(void)
{
	if (sh_buf->magic[0] != SEC_DEBUG_SHARED_MAGIC0)
		return false;

	if (sh_buf->magic[1] != SEC_DEBUG_SHARED_MAGIC1)
		return false;

	if (sh_buf->magic[2] != SEC_DEBUG_SHARED_MAGIC2)
		return false;

	return true;
}

static void sec_debug_extra_info_buffer_init(void)
{
	unsigned long tmp_addr;
	struct sec_debug_sb_index tmp_idx;
	bool flag_valid = false;

	flag_valid = sec_debug_extra_info_check_magic();

	if (SDEI_DEBUG)
		sec_debug_extra_info_dump_sb_index();

	tmp_idx.cnt = 0;
	tmp_idx.blmark = 0;

	/* SLOT_32, 32B, 64 items */
	tmp_addr = sec_debug_get_buf_base(SDN_MAP_EXTRA_INFO);
	tmp_idx.paddr = (unsigned int)tmp_addr;
	tmp_idx.size = 32;
	tmp_idx.nr = 64;
	sec_debug_init_extra_info_sbidx(SLOT_32, tmp_idx, flag_valid);

	/* SLOT_64, 64B, 64 items */
	tmp_addr += tmp_idx.size * tmp_idx.nr;
	tmp_idx.paddr = (unsigned int)tmp_addr;
	tmp_idx.size = 64;
	tmp_idx.nr = 64;
	sec_debug_init_extra_info_sbidx(SLOT_64, tmp_idx, flag_valid);

	/* SLOT_256, 256B, 16 items */
	tmp_addr += tmp_idx.size * tmp_idx.nr;
	tmp_idx.paddr = (unsigned int)tmp_addr;
	tmp_idx.size = 256;
	tmp_idx.nr = 16;
	sec_debug_init_extra_info_sbidx(SLOT_256, tmp_idx, flag_valid);

	/* SLOT_1024, 1024B, 16 items */
	tmp_addr += tmp_idx.size * tmp_idx.nr;
	tmp_idx.paddr = (unsigned int)tmp_addr;
	tmp_idx.size = 1024;
	tmp_idx.nr = 16;
	sec_debug_init_extra_info_sbidx(SLOT_1024, tmp_idx, flag_valid);

	/* backup shared buffer contents */
	sec_debug_extra_info_copy_shared_buffer(flag_valid);

	sec_debug_extra_info_key_init();

	if (SDEI_DEBUG)
		dump_all_keys();

	slot_end_addr = (void *)phys_to_virt(sh_buf->sec_debug_sbidx[SLOT_END].paddr +
				((phys_addr_t)(sh_buf->sec_debug_sbidx[SLOT_END].size) *
				 (phys_addr_t)(sh_buf->sec_debug_sbidx[SLOT_END].nr)));
}

#define MAX_EXTRA_INFO_LEN	(MAX_ITEM_KEY_LEN + MAX_ITEM_VAL_LEN)

static void sec_debug_store_extra_info(char (* keys)[MAX_ITEM_KEY_LEN], int nr_keys, char *ptr)
{
	int i;
	unsigned long len, max_len;
	void *p;
	char *v, *start_addr = ptr;

	memset(ptr, 0, SZ_1K);

	for (i = 0; i < nr_keys; i++) {
		p = get_bk_item(keys[i]);
		if (!p) {
			pr_crit("%s: no key: %s\n", __func__, keys[i]);

			continue;
		}

		v = p + MAX_ITEM_KEY_LEN;

		/* get_key_len returns length of the key + 1 */
		len = (unsigned long)ptr + strlen(p) + get_val_len(v)
				+ MAX_EXTRA_INFO_HDR_LEN;

		max_len = (unsigned long)start_addr + SZ_1K;

		if (len > max_len)
			break;

		ptr += snprintf(ptr, MAX_EXTRA_INFO_LEN, "\"%s\":\"%s\"", (char *)p, v);

		if ((i + 1) != nr_keys)
			ptr += sprintf(ptr, ",");
	}

	printk("%s: %s\n", __func__, ptr);
}

/******************************************************************************
 * sec_debug_extra_info details
 *
 *	etr_a : basic reset information
 *	etr_b : basic reset information
 *	etr_c : hard-lockup information (callstack)
 *	etr_m : mfc error information
 *
 ******************************************************************************/

void sec_debug_store_extra_info_A(char *ptr)
{
	int nr_keys;

	nr_keys = sizeof(akeys) / sizeof(akeys[0]);

	sec_debug_store_extra_info(akeys, nr_keys, ptr);
}

void sec_debug_store_extra_info_B(char *ptr)
{
	int nr_keys;

	nr_keys = sizeof(bkeys) / sizeof(bkeys[0]);

	sec_debug_store_extra_info(bkeys, nr_keys, ptr);
}

void sec_debug_store_extra_info_C(char *ptr)
{
	int nr_keys;

	nr_keys = sizeof(ckeys) / sizeof(ckeys[0]);

	sec_debug_store_extra_info(ckeys, nr_keys, ptr);
}

void sec_debug_store_extra_info_M(char *ptr)
{
	int nr_keys;

	nr_keys = sizeof(mkeys) / sizeof(mkeys[0]);

	sec_debug_store_extra_info(mkeys, nr_keys, ptr);
}

void sec_debug_store_extra_info_F(char *ptr)
{
	int nr_keys;

	nr_keys = sizeof(fkeys) / sizeof(fkeys[0]);

	sec_debug_store_extra_info(fkeys, nr_keys, ptr);
}

/* get dram info from bootloader by cmdline */
#define MAX_DRAMINFO	15
static char dram_info[MAX_DRAMINFO + 1];

static int __init sec_hw_param_get_dram_info(char *arg)
{
	if (strlen(arg) > MAX_DRAMINFO)
		return 0;

	memcpy(dram_info, arg, (int)strlen(arg));

	return 0;
}
early_param("androidboot.dram_info", sec_hw_param_get_dram_info);

static void sec_debug_set_extra_info_id(void)
{
	struct timespec ts;

	getnstimeofday(&ts);

	set_bk_item_val("ID", SLOT_BK_32, "%09lu%s", ts.tv_nsec, EXTRA_VERSION);

	set_item_val("ASB", "%d", id_get_asb_ver());
	set_item_val("PSITE", "%d", id_get_product_line());
	set_item_val("DDRID", "%s", dram_info);
}

static void sec_debug_set_extra_info_ktime(void)
{
	u64 ts_nsec;

	ts_nsec = local_clock();
	do_div(ts_nsec, 1000000000);

	set_item_val("KTIME", "%lu", (unsigned long)ts_nsec);
}

void sec_debug_set_extra_info_fault(enum sec_debug_extra_fault_type type,
				    unsigned long addr, struct pt_regs *regs)
{

	phys_addr_t paddr = 0;

	if (regs) {
		pr_crit("%s = %s / 0x%lx\n", __func__, ftype_items[type], addr);

		set_item_val("FTYPE", "%s", ftype_items[type]);
		set_item_val("FAULT", "0x%lx", addr);
		set_item_val("PC", "%pS", regs->pc);
		set_item_val("LR", "%pS",
					 compat_user_mode(regs) ?
					 regs->compat_lr : regs->regs[30]);

		if (type == UNDEF_FAULT && addr >= kimage_voffset) {
			paddr = virt_to_phys((void *)addr);

			pr_crit("%s: 0x%x / 0x%x\n", __func__,
				upper_32_bits(paddr), lower_32_bits(paddr));
//			exynos_pmu_write(EXYNOS_PMU_INFORM8, lower_32_bits(paddr));
//			exynos_pmu_write(EXYNOS_PMU_INFORM9, upper_32_bits(paddr));
		}
	}
}

void sec_debug_set_extra_info_bug(const char *file, unsigned int line)
{
	set_item_val("BUG", "%s:%u", file, line);
}

void sec_debug_set_extra_info_panic(char *str)
{
	if (strstr(str, "\nPC is at"))
		strcpy(strstr(str, "\nPC is at"), "");

	set_item_val("PANIC", "%s", str);
}

void sec_debug_set_extra_info_backtrace(struct pt_regs *regs)
{
	char fbuf[MAX_ITEM_VAL_LEN];
	char buf[64];
	struct stackframe frame;
	int offset = 0;
	int sym_name_len;
	char *v;

	v = get_item_val("STACK");
	if (!v) {
		pr_crit("%s: no STACK in items\n", __func__);

		return;
	}

	if (get_val_len(v)) {
		pr_crit("%s: already %s in STACK\n", __func__, v);

		return;
	}

	memset(fbuf, 0, MAX_ITEM_VAL_LEN);

	pr_crit("sec_debug_store_backtrace\n");

	if (regs) {
		frame.fp = regs->regs[29];
		//frame.sp = regs->sp;
		frame.pc = regs->pc;
	} else {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		//frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)sec_debug_set_extra_info_backtrace;
	}

	while (1) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(NULL, &frame);
		if (ret < 0)
			break;

		snprintf(buf, sizeof(buf), "%pf", (void *)where);
		sym_name_len = strlen(buf);

		if (offset + sym_name_len > MAX_ITEM_VAL_LEN)
			break;

		if (offset)
			offset += sprintf(fbuf + offset, ":");

		snprintf(fbuf + offset, MAX_ITEM_VAL_LEN - offset, "%s", buf);
		offset += sym_name_len;
	}

	set_item_val("STACK", fbuf);
}

void sec_debug_set_extra_info_backtrace_cpu(struct pt_regs *regs, int cpu)
{
	char fbuf[MAX_ITEM_VAL_LEN];
	char key[MAX_ITEM_KEY_LEN];
	char buf[64];
	struct stackframe frame;
	int offset = 0;
	int sym_name_len;
	char *v;

	snprintf(key, 5, "CPU%d", cpu);

	v = get_item_val(key);
	if (!v) {
		pr_crit("%s: no %s in items\n", __func__, key);

		return;
	}

	if (get_val_len(v)) {
		pr_crit("%s: already %s in %s\n", __func__, v, key);

		return;
	}

	memset(fbuf, 0, MAX_ITEM_VAL_LEN);

	pr_crit("sec_debug_store_backtrace_cpu(%d)\n", cpu);

	if (regs) {
		frame.fp = regs->regs[29];
	//	frame.sp = regs->sp;
		frame.pc = regs->pc;
	} else {
		frame.fp = (unsigned long)__builtin_frame_address(0);
	//	frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)sec_debug_set_extra_info_backtrace_cpu;
	}

	while (1) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(NULL, &frame);
		if (ret < 0)
			break;

		snprintf(buf, sizeof(buf), "%pf", (void *)where);
		sym_name_len = strlen(buf);

		if (offset + sym_name_len > MAX_ITEM_VAL_LEN)
			break;

		if (offset)
			offset += sprintf(fbuf + offset, ":");

		snprintf(fbuf + offset, MAX_ITEM_VAL_LEN - offset, "%s", buf);

		offset += sym_name_len;
	}

	set_item_val(key, fbuf);
}

void sec_debug_set_extra_info_backtrace_task(struct task_struct *tsk)
{
	char fbuf[MAX_ITEM_VAL_LEN];
	char buf[64];
	struct stackframe frame;
	int offset = 0;
	int sym_name_len;
	char *v;
	int cnt = 0;

	if (!tsk) {
		pr_crit("%s: no TASK, quit\n", __func__);

		return;
	}

	if (!try_get_task_stack(tsk)) {
		pr_crit("%s: fail to get task stack, quit\n", __func__);

		return;
	}

	v = get_item_val("STACK");
	if (!v) {
		pr_crit("%s: no STACK in items\n", __func__);

		goto out;
	}

	if (get_val_len(v)) {
		pr_crit("%s: already %s in STACK\n", __func__, v);

		goto out;
	}

	memset(fbuf, 0, MAX_ITEM_VAL_LEN);

	pr_crit("sec_debug_store_backtrace_task\n");

	frame.fp = thread_saved_fp(tsk);
	frame.pc = thread_saved_pc(tsk);

	while (1) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(tsk, &frame);
		if (ret < 0)
			break;

		snprintf(buf, sizeof(buf), "%pf", (void *)where);
		sym_name_len = strlen(buf);

		if (offset + sym_name_len > MAX_ITEM_VAL_LEN)
			break;

		if (offset)
			offset += sprintf(fbuf + offset, ":");

		snprintf(fbuf + offset, MAX_ITEM_VAL_LEN - offset, "%s", buf);
		offset += sym_name_len;

		cnt++;
	}

	set_item_val("STACK", fbuf);

out:
	put_task_stack(tsk);
}


void sec_debug_set_extra_info_sysmmu(char *str)
{
	set_item_val("SMU", "%s", str);
}

void sec_debug_set_extra_info_busmon(char *str)
{
	set_item_val("BUS", "%s", str);
}

void sec_debug_set_extra_info_dpm_timeout(char *devname)
{
	set_item_val("DPM", "%s", devname);
}

void sec_debug_set_extra_info_smpl(unsigned long count)
{
	clear_item_val("SMP");
	set_item_val("SMP", "%lu", count);
}

void sec_debug_set_extra_info_ufs_error(char *str)
{
	set_item_val("ETC", "%s", str);
}

void sec_debug_set_extra_info_zswap(char *str)
{
	set_item_val("ETC", "%s", str);
}

void sec_debug_set_extra_info_aud(char *str)
{
	set_item_val("AUD", "%s", str);
}

void sec_debug_set_extra_info_epd(char *str)
{
	set_item_val("EPD", "%s", str);
}

void sec_debug_set_extra_info_mfc_error(char *str)
{
	clear_item_val("STACK");
	set_item_val("STACK", "MFC ERROR");
	set_item_val("MFC", "%s", str);
}

void sec_debug_set_extra_info_esr(unsigned int esr)
{
	set_item_val("ESR", "%s (0x%08x)",
						 esr_get_class_string(esr), esr);
}

void sec_debug_set_extra_info_merr(char *merr)
{
	set_item_val("MER", "%s", merr);
}

void sec_debug_set_extra_info_hint(u64 hint)
{
	if (hint)
		set_item_val("HINT", "%llx", hint);
}

void sec_debug_set_extra_info_decon(unsigned int err)
{
	set_item_val("DCN", "%08x", err);
}

void sec_debug_set_extra_info_batt(int cap, int volt, int temp, int curr)
{
	clear_item_val("BAT");
	set_item_val("BAT", "%03d/%04d/%04d/%06d", cap, volt, temp, curr);
}

void sec_debug_finish_extra_info(void)
{
	sec_debug_set_extra_info_ktime();
}

#define MAX_UNFZ_VAL_LEN (240)

void sec_debug_set_extra_info_unfz(char *comm)
{
	void *p;
	char *v;
	char tmp[MAX_UNFZ_VAL_LEN] = {0, };
	int max = MAX_UNFZ_VAL_LEN;
	int len_prev, len_remain, len_this;

	p = get_item("UNFZ");
	if (!p) {
		pr_crit("%s: fail to find %s\n", __func__, comm);

		return;
	}

	max = get_max_len(p);
	if (!max) {
		pr_crit("%s: fail to get max len %s\n", __func__, comm);

		return;
	}

	v = get_item_val(p);

	/* keep previous value */
	len_prev = get_val_len(v);
	if ((!len_prev) || (MAX_UNFZ_VAL_LEN <= len_prev))
		len_prev = MAX_UNFZ_VAL_LEN - 1;

	snprintf(tmp, len_prev + 1, "%s", v);

	/* calculate the remained size */
	len_remain = max;

	/* get_item_val returned address without key */
	len_remain -= MAX_ITEM_KEY_LEN;

	/* need comma */
	len_this = strlen(comm) + 1;
	len_remain -= len_this;

	/* put last key at the first of ODR */
	/* +1 to add NULL (by snprintf) */
	snprintf(v, len_this + 1, "%s,", comm);

	/* -1 to remove NULL between KEYS */
	/* +1 to add NULL (by snprintf) */
	snprintf((char *)(v + len_this), len_remain + 1, tmp);
}

/*********** TEST V3 **************************************/
static void test_v3(void *seqm)
{
	struct seq_file *m = (struct seq_file *)seqm;

	seq_printf(m, " -- SEC DEBUG SHARED INFO V3 (SHARED BUFFER) -- \n");
	dump_slots(SLOT_32, NR_MAIN_SLOT, m);

	seq_printf(m, " -- SEC DEBUG SHARED INFO V3 (SHARED BUFFER BK) -- \n");
	dump_slots(SLOT_BK_32, NR_SLOT, m);
}
/*********** TEST V3 **************************************/
static int set_debug_reset_rwc_proc_show(struct seq_file *m, void *v)
{
	char *rstcnt;

	rstcnt = get_bk_item_val("RSTCNT");
	if (!rstcnt)
		seq_printf(m, "%d", secdbg_rere_get_rstcnt_from_cmdline());
	else
		seq_printf(m, "%s", rstcnt);

	seq_printf(m, "%s", rstcnt);

	return 0;
}

static int sec_debug_reset_rwc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_debug_reset_rwc_proc_show, NULL);
}

static const struct file_operations sec_debug_reset_rwc_proc_fops = {
	.open = sec_debug_reset_rwc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int set_debug_reset_extra_info_proc_show(struct seq_file *m, void *v)
{
	char buf[SZ_1K];

	if (0)
		test_v3(m);

	sec_debug_store_extra_info_A(buf);
	seq_printf(m, buf);

	return 0;
}

static int sec_debug_reset_extra_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, set_debug_reset_extra_info_proc_show, NULL);
}

static const struct file_operations sec_debug_reset_extra_info_proc_fops = {
	.open = sec_debug_reset_extra_info_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void simulate_extra_info_force_error(unsigned int magic)
{
	if (!exin_ready) {
		pr_crit("%s: EXIN is not ready\n", __func__);
		return;
	}

	sh_buf->magic[0] = magic;
}

static int __init sec_debug_extra_info_init(void)
{
	struct proc_dir_entry *entry;

	sh_buf = sec_debug_get_debug_base(SDN_MAP_EXTRA_INFO);
	if (sh_buf) {
		sec_debug_extra_info_buffer_init();

		sh_buf->magic[0] = SEC_DEBUG_SHARED_MAGIC0;
		sh_buf->magic[1] = SEC_DEBUG_SHARED_MAGIC1;
		sh_buf->magic[2] = SEC_DEBUG_SHARED_MAGIC2;
		sh_buf->magic[3] = SEC_DEBUG_SHARED_MAGIC3;
	}

	exin_ready = true;

	entry = proc_create("reset_reason_extra_info",
				0644, NULL, &sec_debug_reset_extra_info_proc_fops);
	if (!entry)
		return -ENOMEM;

	proc_set_size(entry, SZ_1K);

	entry = proc_create("reset_rwc", S_IWUGO, NULL,
				&sec_debug_reset_rwc_proc_fops);

	if (!entry)
		return -ENOMEM;

	sec_debug_set_extra_info_id();

	return 0;
}
late_initcall(sec_debug_extra_info_init);
