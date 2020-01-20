/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/nmi.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/debug-snapshot.h>
#include <linux/debug-snapshot-helper.h>
#include <asm-generic/io.h>

#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-debug.h>

#define EXYNOS_DEBUG_TEST_END	(0xCAFEDB9)
#define ALL_FORCE_ERRORS	31

typedef void (*force_error_func)(char *arg);

static void simulate_KP(char *arg);
static void simulate_DP(char *arg);
static void simulate_QDP(char *arg);
static void simulate_SVC(char *arg);
static void simulate_SFR(char *arg);
static void simulate_WP(char *arg);
static void simulate_TP(char *arg);
static void simulate_PANIC(char *arg);
static void simulate_BUG(char *arg);
static void simulate_WARN(char *arg);
static void simulate_DABRT(char *arg);
static void simulate_PABRT(char *arg);
static void simulate_UNDEF(char *arg);
static void simulate_DFREE(char *arg);
static void simulate_DREF(char *arg);
static void simulate_MCRPT(char *arg);
static void simulate_LOMEM(char *arg);
static void simulate_SOFT_LOCKUP(char *arg);
static void simulate_HARD_LOCKUP(char *arg);
static void simulate_BAD_SCHED(char *arg);
static void simulate_SPIN_LOCKUP(char *arg);
static void simulate_PC_ABORT(char *arg);
static void simulate_SP_ABORT(char *arg);
static void simulate_JUMP_ZERO(char *arg);
static void simulate_BUSMON_ERROR(char *arg);
static void simulate_UNALIGNED(char *arg);
static void simulate_WRITE_RO(char *arg);
static void simulate_OVERFLOW(char *arg);
static void simulate_test_start(char *arg);

static int exynos_debug_test_desc_init(struct device_node *np);

enum {
	FORCE_KERNEL_PANIC = 0,		/* KP */
	FORCE_WATCHDOG,			/* DP */
	FORCE_QUICKWATCHDOG,		/* QDP */
	FORCE_SVC,			/* SVC */
	FORCE_SFR,			/* SFR */
	FORCE_WARM_RESET,		/* WP */
	FORCE_HW_TRIPPING,		/* TP */
	FORCE_PANIC,			/* PANIC */
	FORCE_BUG,			/* BUG */
	FORCE_WARN,			/* WARN */
	FORCE_DATA_ABORT,		/* DABRT */
	FORCE_PREFETCH_ABORT,		/* PABRT */
	FORCE_UNDEFINED_INSTRUCTION,	/* UNDEF */
	FORCE_DOUBLE_FREE,		/* DFREE */
	FORCE_DANGLING_REFERENCE,	/* DREF */
	FORCE_MEMORY_CORRUPTION,	/* MCRPT */
	FORCE_LOW_MEMEMORY,		/* LOMEM */
	FORCE_SOFT_LOCKUP,		/* SOFT LOCKUP */
	FORCE_HARD_LOCKUP,		/* HARD LOCKUP */
	FORCE_SPIN_LOCKUP,		/* SPIN LOCKUP */
	FORCE_PC_ABORT,			/* PC ABORT */
	FORCE_SP_ABORT,			/* SP ABORT */
	FORCE_JUMP_ZERO,		/* JUMP TO ZERO */
	FORCE_BUSMON_ERROR,		/* BUSMON ERROR */
	FORCE_UNALIGNED,		/* UNALIGNED WRITE */
	FORCE_WRITE_RO,			/* WRITE RODATA */
	FORCE_OVERFLOW,			/* STACK OVERFLOW */
	FORCE_BAD_SCHEDULING,		/* BAD SCHED */
	FORCE_TEST_START,		/* START TEST */
	NR_FORCE_ERROR,
};

enum {
	REBOOT_REASON_WTSR = 0x1,
	REBOOT_REASON_SMPL,
	REBOOT_REASON_WDT,
	REBOOT_REASON_PANIC,
	REBOOT_REASON_NA,
};

struct debug_test_desc {
	int enabled;
	int nr_cpu;
	int nr_little_cpu;
	int nr_mid_cpu;
	int nr_big_cpu;
	int little_cpu_start;
	int mid_cpu_start;
	int big_cpu_start;
	unsigned int ps_hold_control_offset;
	unsigned int *null_pointer_ui;
	int *null_pointer_si;
	void (*null_function)(void);
	struct dentry *exynos_debug_test_debugfs_root;
	struct device_node *np;
	spinlock_t debug_test_lock;
	struct delayed_work test_work;
};

struct force_error_item {
	char errname[SZ_32];
	force_error_func errfunc;
};

struct force_error_test_item {
	char arg[SZ_128];
	int enabled;
	int reason;
};

struct force_error {
	struct force_error_item item[NR_FORCE_ERROR];
};

static struct debug_test_desc exynos_debug_desc = { 0, };

static struct force_error force_error_vector = {
	.item = {
		{"KP",		&simulate_KP},
		{"DP",		&simulate_DP},
		{"QDP",		&simulate_QDP},
		{"SVC",		&simulate_SVC},
		{"SFR",		&simulate_SFR},
		{"WP",		&simulate_WP},
		{"TP",		&simulate_TP},
		{"panic",	&simulate_PANIC},
		{"bug",		&simulate_BUG},
		{"warn",	&simulate_WARN},
		{"dabrt",	&simulate_DABRT},
		{"pabrt",	&simulate_PABRT},
		{"undef",	&simulate_UNDEF},
		{"dfree",	&simulate_DFREE},
		{"danglingref",	&simulate_DREF},
		{"memcorrupt",	&simulate_MCRPT},
		{"lowmem",	&simulate_LOMEM},
		{"softlockup",	&simulate_SOFT_LOCKUP},
		{"hardlockup",	&simulate_HARD_LOCKUP},
		{"spinlockup",	&simulate_SPIN_LOCKUP},
		{"pcabort",	&simulate_PC_ABORT},
		{"spabort",	&simulate_SP_ABORT},
		{"jumpzero",	&simulate_JUMP_ZERO},
		{"busmon",	&simulate_BUSMON_ERROR},
		{"unaligned",	&simulate_UNALIGNED},
		{"writero",	&simulate_WRITE_RO},
		{"overflow",	&simulate_OVERFLOW},
		{"badsched",	&simulate_BAD_SCHED},
		{"all",		&simulate_test_start},
	}
};

static unsigned int test_state;

static struct force_error_test_item test_vector[] = {
	{"KP",				1, REBOOT_REASON_PANIC},
	{"SVC",				1, REBOOT_REASON_PANIC},
	{"SFR 0x1ffffff0",		1, REBOOT_REASON_PANIC},
	{"SFR 0x1ffffff0 0x12345678",	1, REBOOT_REASON_PANIC},
	{"WP",				1, REBOOT_REASON_WTSR},
	{"panic",			1, REBOOT_REASON_PANIC},
	{"bug",				1, REBOOT_REASON_PANIC},
	{"dabrt",			1, REBOOT_REASON_PANIC},
	{"pabrt",			1, REBOOT_REASON_PANIC},
	{"undef",			1, REBOOT_REASON_PANIC},
	{"memcorrupt",			1, REBOOT_REASON_PANIC},
	{"softlockup",			1, REBOOT_REASON_PANIC},
	{"hardlockup 0",		1, REBOOT_REASON_WDT},
	{"hardlockup LITTLE",		1, REBOOT_REASON_WDT},
	{"hardlockup MID",		1, REBOOT_REASON_WDT},
	{"hardlockup BIG",		1, REBOOT_REASON_WDT},
	{"spinlockup",			1, REBOOT_REASON_PANIC},
	{"pcabort",			1, REBOOT_REASON_PANIC},
	{"jumpzero",			1, REBOOT_REASON_PANIC},
	{"writero",			1, REBOOT_REASON_PANIC},
	{"danglingref",			0, REBOOT_REASON_PANIC},
	{"dfree",			0, REBOOT_REASON_PANIC},
	{"badsched",			0, REBOOT_REASON_PANIC},
	{"QDP",				0, REBOOT_REASON_WDT},
	{"spabort",			0, REBOOT_REASON_NA},
	{"overflow",			0, REBOOT_REASON_NA},
};

static int str_to_num(char *s)
{
	int ret = -1;

	if (s) {
		switch (s[0]) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			ret = s[0] - '0';
			break;
		default:
			ret = -1;
			break;
		}
	}
	return ret;
}

static void clear_dbg_snapshot_test_regs(void)
{
	dbg_snapshot_set_debug_test_case(0);
	dbg_snapshot_set_debug_test_next(0);
	dbg_snapshot_set_debug_test_panic(0);
	dbg_snapshot_set_debug_test_wdt(0);
	dbg_snapshot_set_debug_test_wtsr(0);
	dbg_snapshot_set_debug_test_smpl(0);
	dbg_snapshot_set_debug_test_curr(0);
	dbg_snapshot_set_debug_test_total(0);
	dbg_snapshot_clear_debug_test_runflag();
	test_state = 0;
}

static int debug_force_error(const char *val)
{
	int i;
	char *temp;
	char *ptr;
	int test_vector_id = -1;

	for (i = 0; i < (int)ARRAY_SIZE(test_vector); i++) {
		if (test_vector[i].enabled &&
			!strncmp(val, test_vector[i].arg,
				      max(strlen(val), strlen(test_vector[i].arg)))) {
			test_vector_id = i;
			break;
		}
	}

	for (i = 0; i < NR_FORCE_ERROR; i++) {
		if (!strncmp(val, force_error_vector.item[i].errname,
			     strlen(force_error_vector.item[i].errname))) {
			if (test_vector_id >= 0 &&
				!dbg_snapshot_get_debug_test_run(ALL_FORCE_ERRORS)) {
				dbg_snapshot_set_debug_test_reg(1);
				dbg_snapshot_set_debug_test_run(test_vector_id, 1);
				dbg_snapshot_set_debug_test_case(test_vector_id);
				dbg_snapshot_set_debug_test_next(test_vector_id + 1);
				test_state = 1 << test_vector_id;
			}
			temp = (char *)val;
			ptr = strsep(&temp, " ");	/* ignore the first token */
			ptr = strsep(&temp, " ");	/* take the second token */
			force_error_vector.item[i].errfunc(ptr);
		}
	}
	return 0;
}

static void exynos_debug_test_print(unsigned int cnt)
{
	int i;
	int pass;

	if (!cnt)
		return;

	if (cnt >= ARRAY_SIZE(test_vector)) {
		pr_info("=============== DEBUG TEST RESULT ===============\n");
		cnt = ARRAY_SIZE(test_vector);
	} else {
		pr_info("================ DEBUG TEST LOG =================\n");
	}

	for (i = 0; i < cnt; i++) {
		if (!test_vector[i].enabled) {
			pr_info("TestCase%3d: [%30s] disabled\n", i,
							test_vector[i].arg);
			continue;
		}
		switch (test_vector[i].reason) {
		case REBOOT_REASON_WTSR:
			if ((1 << i) & dbg_snapshot_get_debug_test_wtsr())
				pass = 1;
			else
				pass = 0;
			break;
		case REBOOT_REASON_SMPL:
			if ((1 << i) & dbg_snapshot_get_debug_test_smpl())
				pass = 1;
			else
				pass = 0;
			break;
		case REBOOT_REASON_WDT:
			if ((1 << i) & dbg_snapshot_get_debug_test_wdt())
				pass = 1;
			else
				pass = 0;
			break;
		case REBOOT_REASON_PANIC:
			if ((1 << i) & dbg_snapshot_get_debug_test_panic())
				pass = 1;
			else
				pass = 0;
			break;
		default:
			pass = 0;
			break;
		}

		pr_info("TestCase%3d: [%30s] result: [%s]\n",
				i, test_vector[i].arg, pass ? "PASS" : "FAIL");
	}
	pr_info("reg info: panic[0x%x] wdt[0x%x] wtsr[0x%x] smpl[0x%x]\n",
						dbg_snapshot_get_debug_test_panic(),
						dbg_snapshot_get_debug_test_wdt(),
						dbg_snapshot_get_debug_test_wtsr(),
						dbg_snapshot_get_debug_test_smpl());
	pr_info("=================================================\n");
}

static void exynos_debug_test_run_test(struct work_struct *work)
{
	int i = dbg_snapshot_get_debug_test_next();

	/* find test and do test */
	for (; i < (int)ARRAY_SIZE(test_vector); i++) {
		if (test_vector[i].enabled) {
			char *temp;

			temp = kmalloc(SZ_128, GFP_KERNEL);
			if (!temp)
				return;
			exynos_debug_test_print(i);
			dbg_snapshot_set_debug_test_next(i + 1);
			dbg_snapshot_set_debug_test_case(i);
			dbg_snapshot_set_debug_test_run(i, 1);
			test_state = 1 << i;
			pr_info("DEBUG TEST: test case%d\t:\t%s\n", i, test_vector[i].arg);
			memcpy(temp, test_vector[i].arg, SZ_128);
			debug_force_error(temp);
			kfree(temp);
			return;
		}
	}

	/* end of test */
	exynos_debug_test_print(i);
	dbg_snapshot_set_debug_test_reg(0);
	dbg_snapshot_set_debug_test_case(EXYNOS_DEBUG_TEST_END);
}

/* timeout for dog bark/bite */
#define DELAY_TIME 30000

static void pull_down_other_cpus(void)
{
#ifdef CONFIG_HOTPLUG_CPU
	int cpu, ret;

	for (cpu = exynos_debug_desc.nr_cpu - 1; cpu > 0 ; cpu--) {
		ret = cpu_down(cpu);
		if (ret)
			pr_crit("DEBUG TEST: %s() CORE%d ret: %x\n",
							__func__, cpu, ret);
	}
#endif
}

static int find_blank(char *arg)
{
	int i;

	/* if parameter is not one, a space between parameters is 0
	 * End of parameter is lf(10)
	 */
	for (i = 0; !isspace(arg[i]) && arg[i]; i++)
		continue;

	return i;
}

static void simulate_test_start(char *arg)
{
	int i;
	int test_cnt;

	test_cnt = ARRAY_SIZE(test_vector);
	for (i = 0; i < (int)ARRAY_SIZE(test_vector); i++) {
		if (!test_vector[i].enabled)
			test_cnt--;
	}

	pr_info("DEBUG TEST: TEST START!(total test case = %d)\n", test_cnt);
	clear_dbg_snapshot_test_regs();
	dbg_snapshot_set_debug_test_reg(1);
	dbg_snapshot_set_debug_test_total(test_cnt);
	dbg_snapshot_set_debug_test_run(ALL_FORCE_ERRORS, 1);
	exynos_debug_test_run_test(&exynos_debug_desc.test_work.work);
}

static void simulate_KP(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	*exynos_debug_desc.null_pointer_ui = 0x0; /* SVACE: intended */
}

static void simulate_DP(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	pull_down_other_cpus();
	pr_crit("DEBUG TEST: %s() start to hanging\n", __func__);
	local_irq_disable();
	mdelay(DELAY_TIME);
	local_irq_enable();
	/* should not reach here */
}

static void simulate_QDP(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	s3c2410wdt_set_emergency_reset(10, 0);
	mdelay(DELAY_TIME);
	/* should not reach here */
}

static void simulate_SVC(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	asm("svc #0x0");
	/* should not reach here */
}

static void simulate_SFR(char *arg)
{
	int ret, index = 0;
	unsigned long reg, val;
	char *tmp, *tmparg;
	void __iomem *addr;

	pr_crit("DEBUG TEST: %s() start\n", __func__);

	index = find_blank(arg);
	if (index > PAGE_SIZE)
		return;

	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	memcpy(tmp, arg, index);
	tmp[index] = '\0';

	ret = kstrtoul(tmp, 16, &reg);
	addr = ioremap(reg, 0x10);
	if (!addr) {
		pr_crit("DEBUG TEST: %s() failed to remap 0x%lx, quit\n", __func__, reg);
		kfree(tmp);
		return;
	}
	pr_crit("DEBUG TEST: %s() 1st parameter: 0x%lx\n", __func__, reg);

	tmparg = &arg[index + 1];
	index = find_blank(tmparg);
	if (index == 0) {
		pr_crit("DEBUG TEST: %s() there is no 2nd parameter\n", __func__);
		pr_crit("DEBUG TEST: %s() try to read 0x%lx\n", __func__, reg);

		ret = __raw_readl(addr);
		pr_crit("%s() result : 0x%x\n", __func__, ret);

	} else {
		if (index > PAGE_SIZE) {
			kfree(tmp);
			return;
		}
		memcpy(tmp, tmparg, index);
		tmp[index] = '\0';
		ret = kstrtoul(tmp, 16, &val);
		pr_crit("DEBUG TEST: %s() 2nd parameter: 0x%lx\n", __func__, val);
		pr_crit("DEBUG TEST: %s() try to write 0x%lx to 0x%lx\n", __func__, val, reg);

		__raw_writel(val, addr);
	}
	kfree(tmp);
	/* should not reach here */
}

static void simulate_WP(char *arg)
{
	unsigned int ps_hold_control;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	exynos_pmu_read(exynos_debug_desc.ps_hold_control_offset, &ps_hold_control);
	exynos_pmu_write(exynos_debug_desc.ps_hold_control_offset, ps_hold_control & 0xFFFFFEFF);
}

static void simulate_TP(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);
}

static void simulate_PANIC(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	panic("simulate_panic");
}

static void simulate_BUG(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	BUG();
}

static void simulate_WARN(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	WARN_ON(1);
}

static void simulate_DABRT(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	*exynos_debug_desc.null_pointer_si = 0; /* SVACE: intended */
}

static void simulate_PABRT(char *arg)
{
	pr_crit("DEBUG TEST: %s() call address=[%llx]\n", __func__,
			(unsigned long long)exynos_debug_desc.null_function);

	exynos_debug_desc.null_function(); /* SVACE: intended */
}

static void simulate_UNDEF(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	asm volatile(".word 0xe7f001f2\n\t");
	unreachable();
}

static void simulate_DFREE(char *arg)
{
	void *p;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	p = kmalloc(sizeof(unsigned int), GFP_KERNEL);
	if (p) {
		*(unsigned int *)p = 0x0;
		kfree(p);
		msleep(1000);
		kfree(p); /* SVACE: intended */
	}
}

static void simulate_DREF(char *arg)
{
	unsigned int *p;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	p = kmalloc(sizeof(int), GFP_KERNEL);
	if (p) {
		kfree(p);
		*p = 0x1234; /* SVACE: intended */
	}
}

static void simulate_MCRPT(char *arg)
{
	int *ptr;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	ptr = kmalloc(sizeof(int), GFP_KERNEL);
	if (ptr) {
		*ptr++ = 4;
		*ptr = 2;
		panic("MEMORY CORRUPTION");
	}
}

static void simulate_LOMEM(char *arg)
{
	int i = 0;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	pr_crit("Allocating memory until failure!\n");
	while (kmalloc(128 * 1024, GFP_KERNEL)) /* SVACE: intended */
		i++;
	pr_crit("Allocated %d KB!\n", i * 128);
}

static void simulate_SOFT_LOCKUP(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

#ifdef CONFIG_SOFTLOCKUP_DETECTOR
	softlockup_panic = 1;
#endif
	local_irq_disable();
	preempt_disable();
	local_irq_enable();
	asm("b .");
	preempt_enable();
}

static void simulate_HARD_LOCKUP_handler(void *info)
{
	asm("b .");
}

static void simulate_HARD_LOCKUP(char *arg)
{
	int cpu;
	int start = -1;
	int end;
	int curr_cpu;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	if (arg) {
		if (!strncmp(arg, "LITTLE", strlen("LITTLE"))) {
			if (exynos_debug_desc.little_cpu_start < 0 ||
					exynos_debug_desc.nr_little_cpu < 0) {
				pr_info("DEBUG TEST: %s() no little cpu info\n", __func__);
				return;
			}
			start = exynos_debug_desc.little_cpu_start;
			end = start + exynos_debug_desc.nr_little_cpu - 1;
		} else if (!strncmp(arg, "MID", strlen("MID"))) {
			if (exynos_debug_desc.mid_cpu_start < 0 ||
						exynos_debug_desc.nr_mid_cpu < 0) {
				pr_info("DEBUG TEST: %s() no mid cpu info\n", __func__);
				return;
			}
			start = exynos_debug_desc.mid_cpu_start;
			end = start + exynos_debug_desc.nr_mid_cpu - 1;
		} else if (!strncmp(arg, "BIG", strlen("BIG"))) {
			if (exynos_debug_desc.big_cpu_start < 0 ||
						exynos_debug_desc.nr_big_cpu < 0) {
				pr_info("DEBUG TEST: %s() no big cpu info\n", __func__);
				return;
			}
			start = exynos_debug_desc.big_cpu_start;
			end = start + exynos_debug_desc.nr_big_cpu - 1;
		}

		if (start >= 0) {
			preempt_disable();
			curr_cpu = smp_processor_id();
			for (cpu = start; cpu <= end; cpu++) {
				if (cpu == curr_cpu)
					continue;
				smp_call_function_single(cpu,
						simulate_HARD_LOCKUP_handler, 0, 0);
			}
			if (curr_cpu >= start && curr_cpu <= end) {
				local_irq_disable();
				asm("b .");
			}
			preempt_enable();
			return;
		}

		cpu = str_to_num(arg);
		if (cpu == -1) {
			pr_info("DEBUG TEST: %s() input is invalid\n", __func__);
			return;
		}
		smp_call_function_single(cpu, simulate_HARD_LOCKUP_handler, 0, 0);
	} else {
		start = 0;
		end = exynos_debug_desc.nr_cpu;

		local_irq_disable();
		curr_cpu = smp_processor_id();

		for (cpu = start; cpu < end; cpu++) {
			if (cpu == curr_cpu)
				continue;
			smp_call_function_single(cpu, simulate_HARD_LOCKUP_handler, 0, 0);
		}
		asm("b .");
	}
}

static void simulate_SPIN_LOCKUP(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	spin_lock(&exynos_debug_desc.debug_test_lock);
	spin_lock(&exynos_debug_desc.debug_test_lock);
}

static void simulate_PC_ABORT(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	asm("add x30, x30, #0x1\n\t"
	    "ret");
}

static void simulate_SP_ABORT(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	asm("mov x29, #0xff00\n\t"
	    "mov sp, #0xff00\n\t"
	    "ret");
}

static void simulate_JUMP_ZERO(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	asm("mov x0, #0x0\n\t"
	    "br x0");
}

static void simulate_BUSMON_ERROR(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);
}

static void simulate_UNALIGNED(char *arg)
{
	static u8 data[5] __aligned(4) = {1, 2, 3, 4, 5};
	u32 *p;
	u32 val = 0x12345678;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	p = (u32 *)(data + 1);
	pr_crit("DEBUG TEST: data=[0x%llx] p=[0x%llx]\n",
				(unsigned long long)data, (unsigned long long)p);
	if (*p == 0)
		val = 0x87654321;
	*p = val;
	pr_crit("DEBUG TEST: val = [0x%x] *p = [0x%x]\n", val, *p);
}

static void simulate_WRITE_RO(char *arg)
{
	unsigned long *ptr;

	pr_crit("DEBUG TEST: %s()\n", __func__);

// Write to function addr will triger a warning by JOPP compiler
#ifdef CONFIG_RKP_CFP_JOPP
	ptr = (unsigned long *)__start_rodata;
#else
	ptr = (unsigned long *)simulate_WRITE_RO;
#endif
	*ptr ^= 0x12345678;
}

#define BUFFER_SIZE SZ_1K

static int recursive_loop(int remaining)
{
	char buf[BUFFER_SIZE];

	pr_crit("DEBUG TEST: %s() remainig = [%d]\n", __func__, remaining);

	/* Make sure compiler does not optimize this away. */
	memset(buf, (remaining & 0xff) | 0x1, BUFFER_SIZE);
	if (!remaining)
		return 0;
	else
		return recursive_loop(remaining - 1);
}

static void simulate_OVERFLOW(char *arg)
{
	pr_crit("DEBUG TEST: %s()\n", __func__);

	recursive_loop(600);
}

static void simulate_BAD_SCHED_handler(void *info)
{
	if (idle_cpu(smp_processor_id())) {
		*(int *)info = 1;
		msleep(1000);
	}
}

static void simulate_BAD_SCHED(char *arg)
{
	int cpu;
	int ret = 0;
	int tries = 0;

	pr_crit("DEBUG TEST: %s()\n", __func__);

	while (true) {
		tries++;
		pr_crit("%dth try.\n", tries);
		for_each_online_cpu(cpu) {
			if (idle_cpu(cpu))
				smp_call_function_single(cpu,
					simulate_BAD_SCHED_handler, &ret, 1);
			if (ret)
				return;	/* success */
		}
		mdelay(100);
	}
}

static ssize_t exynos_debug_test_write(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char *buf;
	size_t buf_size;
	int i;

	buf_size = ((count + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
	if (buf_size <= 0)
		return 0;

	buf = kmalloc(buf_size, GFP_KERNEL);
	if (!buf)
		return 0;

	if (copy_from_user(buf, user_buf, count)) {
		kfree(buf);
		return -EFAULT;
	}
	buf[count] = '\0';

	for (i = 0; buf[i] != '\0'; i++)
		if (buf[i] == '\n') {
			buf[i] = '\0';
			break;
		}

	if (!strncmp("clear", buf, 5)) {
		clear_dbg_snapshot_test_regs();
	} else {
		pr_info("DEBUG TEST: %s() user_buf=[%s]\n", __func__, buf);
		debug_force_error(buf);
	}

	kfree(buf);
	return count;
}

static ssize_t exynos_debug_test_read(struct file *file,
				char __user *user_buf, size_t count,
				loff_t *ppos)
{
	char *buf;
	size_t copy_cnt;
	int ret = 0;
	int i, pass;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return ret;

	if (!dbg_snapshot_get_debug_test_runflag())
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "TEST NOT PERFORMED\n");
	else if (dbg_snapshot_get_debug_test_run(ALL_FORCE_ERRORS))
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "ALL TEST PERFORMED\n");
	else
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "INDIVIDUAL TESTS PERFORMED\n");

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"=============== DEBUG TEST RESULT ===============\n");

	for (i = 0; i < (int)ARRAY_SIZE(test_vector); i++) {
		if (!test_vector[i].enabled) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"TestCase%3d: [%30s] disabled.\n", i,
							test_vector[i].arg);
			continue;
		}
		if (!dbg_snapshot_get_debug_test_run(i)) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"TestCase%3d: [%30s] not performed.\n", i,
							test_vector[i].arg);
			continue;
		}
		if (test_state & (1 << i)) {
			ret += snprintf(buf + ret, PAGE_SIZE - ret,
					"TestCase%3d: [%30s] testing...\n", i,
							test_vector[i].arg);
			continue;
		}
		switch (test_vector[i].reason) {
		case REBOOT_REASON_WTSR:
			if ((1 << i) & dbg_snapshot_get_debug_test_wtsr())
				pass = 1;
			else
				pass = 0;
			break;
		case REBOOT_REASON_SMPL:
			if ((1 << i) & dbg_snapshot_get_debug_test_smpl())
				pass = 1;
			else
				pass = 0;
			break;
		case REBOOT_REASON_WDT:
			if ((1 << i) & dbg_snapshot_get_debug_test_wdt())
				pass = 1;
			else
				pass = 0;
			break;
		case REBOOT_REASON_PANIC:
			if ((1 << i) & dbg_snapshot_get_debug_test_panic())
				pass = 1;
			else
				pass = 0;
			break;
		default:
			pass = 0;
			break;
		}

		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"TestCase%3d: [%30s] result: [%s]\n",
				i, test_vector[i].arg, pass ? "PASS" : "FAIL");
	}

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"reg info: panic[0x%x] wdt[0x%x] wtsr[0x%x] smpl[0x%x] run[0x%x]\n",
						dbg_snapshot_get_debug_test_panic(),
						dbg_snapshot_get_debug_test_wdt(),
						dbg_snapshot_get_debug_test_wtsr(),
						dbg_snapshot_get_debug_test_smpl(),
						dbg_snapshot_get_debug_test_runflag());
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"=================================================\n");
	copy_cnt = ret;
	ret = simple_read_from_buffer(user_buf, count, ppos, buf, copy_cnt);

	kfree(buf);
	return ret;
}

static const struct file_operations exynos_debug_test_file_fops = {
	.open	= simple_open,
	.read	= exynos_debug_test_read,
	.write	= exynos_debug_test_write,
	.llseek	= default_llseek,
};

static ssize_t exynos_debug_test_enable_write(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char *buf;
	ssize_t ret_val = 0;
	size_t copy_cnt;
	int ret;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return ret_val;

	if (count > PAGE_SIZE)
		copy_cnt = PAGE_SIZE;
	else
		copy_cnt = count;

	ret = copy_from_user(buf, user_buf, copy_cnt);
	ret_val = copy_cnt - ret;

	if (ret_val && (buf[0] == '1')) {
		ret = 0;
		if (exynos_debug_desc.np)
			ret = exynos_debug_test_desc_init(exynos_debug_desc.np);
		else
			pr_info("DEBUG TEST: %s() no dvice tree entry\n", __func__);

		if (ret)
			pr_info("DEBUG TEST: %s() fail to enable debug test[0x%x]\n",
									__func__, ret);
	} else {
		pr_info("DEBUG TEST: %s() copy count[%lu], input value[%c]\n",
						__func__, ret_val, ret_val ? buf[0] : '0');
	}

	kfree(buf);
	return ret_val;
}

static ssize_t exynos_debug_test_enable_read(struct file *file,
				char __user *user_buf, size_t count,
				loff_t *ppos)
{
	char *buf;
	size_t copy_cnt;
	int ret = 0;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return ret;

	snprintf(buf, PAGE_SIZE, "%sABLED\n", exynos_debug_desc.enabled ? "EN" : "DIS");
	copy_cnt = strlen(buf);

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, copy_cnt);

	kfree(buf);
	return ret;
}

static const struct file_operations exynos_debug_test_enable_fops = {
	.open	= simple_open,
	.read	= exynos_debug_test_enable_read,
	.write	= exynos_debug_test_enable_write,
	.llseek	= default_llseek,
};

static int exynos_debug_test_desc_init(struct device_node *np)
{
	int ret = 0;

	/* get data from device tree */
	ret = of_property_read_u32(np, "ps_hold_control_offset",
					&exynos_debug_desc.ps_hold_control_offset);
	if (ret) {
		pr_err("DEBUG TEST: %s() no data(ps_hold_control offset)\n", __func__);
		goto edt_desc_init_out;
	}

	ret = of_property_read_u32(np, "nr_cpu",
					&exynos_debug_desc.nr_cpu);
	if (ret) {
		pr_err("DEBUG TEST: %s() no data(nr_cpu)\n", __func__);
		goto edt_desc_init_out;
	}

	ret = of_property_read_u32(np, "little_cpu_start",
					&exynos_debug_desc.little_cpu_start);
	if (ret) {
		pr_info("DEBUG TEST: %s() no data(little_cpu_start)\n", __func__);
		exynos_debug_desc.little_cpu_start = -1;
	}

	ret = of_property_read_u32(np, "nr_little_cpu",
					&exynos_debug_desc.nr_little_cpu);
	if (ret) {
		pr_info("DEBUG TEST: %s() no data(nr_little_cpu)\n", __func__);
		exynos_debug_desc.nr_little_cpu = -1;
	}

	ret = of_property_read_u32(np, "mid_cpu_start",
					&exynos_debug_desc.mid_cpu_start);
	if (ret) {
		pr_info("DEBUG TEST: %s() no data(mid_cpu_start)\n", __func__);
		exynos_debug_desc.mid_cpu_start = -1;
	}

	ret = of_property_read_u32(np, "nr_mid_cpu",
					&exynos_debug_desc.nr_mid_cpu);
	if (ret) {
		pr_info("DEBUG TEST: %s() no data(nr_mid_cpu)\n", __func__);
		exynos_debug_desc.nr_mid_cpu = -1;
	}

	ret = of_property_read_u32(np, "big_cpu_start",
					&exynos_debug_desc.big_cpu_start);
	if (ret) {
		pr_info("DEBUG TEST: %s() no data(big_cpu_start)\n", __func__);
		exynos_debug_desc.big_cpu_start = -1;
	}

	ret = of_property_read_u32(np, "nr_big_cpu",
					&exynos_debug_desc.nr_big_cpu);
	if (ret) {
		pr_info("DEBUG TEST: %s() no data(nr_big_cpu)\n", __func__);
		exynos_debug_desc.nr_big_cpu = -1;
	}

	exynos_debug_desc.null_function = (void (*)(void))0x1234;
	spin_lock_init(&exynos_debug_desc.debug_test_lock);

	/* create debugfs test file */
	debugfs_create_file("test", 0644,
			exynos_debug_desc.exynos_debug_test_debugfs_root,
			NULL, &exynos_debug_test_file_fops);
	ret = 0;
	exynos_debug_desc.enabled = 1;

edt_desc_init_out:
	return ret;
}

static const struct of_device_id of_exynos_debug_test_matches[] __initconst = {
	{.compatible = "samsung,exynos-debug-test"},
	{},
};

static int __init exynos_debug_test_init(void)
{
	struct device_node *np = NULL;
	const char *enable_str;
	int ret = 0;

	pr_info("DEBUG TEST: %s() called\n", __func__);

	/* find device tree */
	np = of_find_matching_node(NULL, of_exynos_debug_test_matches);
	exynos_debug_desc.np = np;
	if (!np) {
		pr_err("DEBUG TEST: %s() no device tree\n", __func__);
		ret = -ENODEV;
		goto edt_out;
	}

	/* create debugfs dir */
	exynos_debug_desc.exynos_debug_test_debugfs_root =
				debugfs_create_dir("exynos-debug-test", NULL);
	if (!exynos_debug_desc.exynos_debug_test_debugfs_root) {
		pr_err("DEBUG TEST: %s() cannot create debugfs dir\n", __func__);
		ret = -ENOMEM;
		goto edt_out;
	}

	/* create debugfs enable file */
	debugfs_create_file("enable", 0644,
			exynos_debug_desc.exynos_debug_test_debugfs_root,
			NULL, &exynos_debug_test_enable_fops);

	/* checking debug test is enabled */
	ret = of_property_read_string(np, "enabled", &enable_str);
	if (ret) {
		pr_err("DEBUG TEST: %s() no data(enabled)\n", __func__);
		goto edt_out;
	}

	if (strncmp(enable_str, "dbg_test", strlen("dbg_test")) &&
					!dbg_snapshot_debug_test_enabled()) {
		pr_info("DEBUG TEST: %s() debug test is not enabled\n", __func__);
		goto edt_out;
	}

	/* initializing desc data */
	ret = exynos_debug_test_desc_init(np);
	if (ret)
		goto edt_out;

	/* checking debug test is on going */
	if (dbg_snapshot_debug_test_enabled()) {
		if (dbg_snapshot_get_debug_test_run(ALL_FORCE_ERRORS)) {
			INIT_DELAYED_WORK(&exynos_debug_desc.test_work,
					exynos_debug_test_run_test);
			schedule_delayed_work(&exynos_debug_desc.test_work, HZ * 20);
		} else {
			dbg_snapshot_set_debug_test_reg(0);
		}
	}

edt_out:
	pr_info("DEBUG TEST: %s() ret=[0x%x]\n", __func__, ret);
	return ret;
}
late_initcall(exynos_debug_test_init);
