/*
 *sec_debug_test.c
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
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include <linux/slab.h>
//#include <linux/exynos-ss.h>
#include <asm-generic/io.h>
#include <linux/ctype.h>
#include <linux/pm_qos.h>
#include <linux/sec_debug.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/preempt.h>
#include <linux/rwsem.h>
#include <linux/moduleparam.h>

//#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-debug.h>

/* Override the default prefix for the compatibility with other models */
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "sec_debug."

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
static void simulate_SAFEFAULT(char *arg);
static void simulate_PABRT(char *arg);
static void simulate_UNDEF(char *arg);
static void simulate_DFREE(char *arg);
static void simulate_DREF(char *arg);
static void simulate_MCRPT(char *arg);
static void simulate_LOMEM(char *arg);
static void simulate_SOFT_LOCKUP(char *arg);
static void simulate_SOFTIRQ_LOCKUP(char *arg);
static void simulate_SOFTIRQ_STORM(char *arg);
static void simulate_TASK_HARD_LOCKUP(char *arg);
static void simulate_IRQ_HARD_LOCKUP(char *arg);
static void simulate_BAD_SCHED(char *arg);
static void simulate_SPIN_LOCKUP(char *arg);
static void simulate_SPINLOCK_ALLCORE(char *arg);
static void simulate_SPINLOCK_SOFTLOCKUP(char *arg);
static void simulate_SPINLOCK_HARDLOCKUP(char *arg);
static void simulate_RW_LOCKUP(char *arg);
static void simulate_ALLRW_LOCKUP(char *arg);
static void simulate_PC_ABORT(char *arg);
static void simulate_SP_ABORT(char *arg);
static void simulate_JUMP_ZERO(char *arg);
static void simulate_BUSMON_ERROR(char *arg);
static void simulate_UNALIGNED(char *arg);
static void simulate_WRITE_RO(char *arg);
static void simulate_OVERFLOW(char *arg);
static void simulate_CORRUPT_MAGIC(char *arg);
static void simulate_IRQ_STORM(char *arg);
static void simulate_SYNC_IRQ_LOCKUP(char *arg);
static void simulate_DISK_SLEEP(char *arg);
static void simulate_CORRUPT_DELAYED_WORK(char *arg);
static void simulate_MUTEX_AA(char *arg);
static void simulate_MUTEX_ABBA(char *arg);
static void simulate_WQ_LOCKUP(char *arg);
static void simulate_RWSEM_R(char *arg);
static void simulate_RWSEM_W(char *arg);
static void simulate_PRINTK_FAULT(char *arg);

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
	FORCE_SAFEFAULT_ABORT,		/* SAFE FAULT */
	FORCE_PREFETCH_ABORT,		/* PABRT */
	FORCE_UNDEFINED_INSTRUCTION,	/* UNDEF */
	FORCE_DOUBLE_FREE,		/* DFREE */
	FORCE_DANGLING_REFERENCE,	/* DREF */
	FORCE_MEMORY_CORRUPTION,	/* MCRPT */
	FORCE_LOW_MEMEMORY,		/* LOMEM */
	FORCE_SOFT_LOCKUP,		/* SOFT LOCKUP */
	FORCE_SOFTIRQ_LOCKUP,		/* SOFTIRQ LOCKUP */
	FORCE_SOFTIRQ_STORM,		/* SOFTIRQ_STORM */
	FORCE_TASK_HARD_LOCKUP,		/* TASK HARD LOCKUP */
	FORCE_IRQ_HARD_LOCKUP,		/* IRQ HARD LOCKUP */
	FORCE_SPIN_LOCKUP,		/* SPIN LOCKUP */
	FORCE_SPIN_ALLCORE,		/* SPINLOCK ALL CORE */
	FORCE_SPIN_SOFTLOCKUP,		/* SPINLOCK SOFT LOCKUP */
	FORCE_SPIN_HARDLOCKUP,		/* SPINLOCK HARD LOCKUP */
	FORCE_RW_LOCKUP,		/* RW LOCKUP */
	FORCE_ALLRW_LOCKUP,		/* ALL RW LOCKUP */
	FORCE_PC_ABORT,			/* PC ABORT */
	FORCE_SP_ABORT,			/* SP ABORT */
	FORCE_JUMP_ZERO,		/* JUMP TO ZERO */
	FORCE_BUSMON_ERROR,		/* BUSMON ERROR */
	FORCE_UNALIGNED,		/* UNALIGNED WRITE */
	FORCE_WRITE_RO,			/* WRITE RODATA */
	FORCE_OVERFLOW,			/* STACK OVERFLOW */
	FORCE_BAD_SCHEDULING,		/* BAD SCHED */
	FORCE_CORRUPT_MAGIC,		/* CM */
	FORCE_IRQ_STORM,		/* IRQ STORM */
	FORCE_SYNC_IRQ_LOCKUP,		/* SYNCIRQ LOCKUP */
	FORCE_DISK_SLEEP,		/* DISK SLEEP */
	FORCE_CORRUPT_DELAYED_WORK,	/* CORRUPT DELAYED WORK */
	FORCE_MUTEX_AA,			/* MUTEX AA */
	FORCE_MUTEX_ABBA,		/* MUTEX ABBA */
	FORCE_WQ_LOCKUP,		/* WORKQUEUE LOCKUP */
	FORCE_RWSEM_R,			/* RWSEM READER */
	FORCE_RWSEM_W,			/* RWSEM WRITER */
	FORCE_PRINTK_FAULT,		/* PRINTK FAULT */
	NR_FORCE_ERROR,
};

struct force_error_item {
	char errname[SZ_32];
	force_error_func errfunc;
};

struct force_error {
	struct force_error_item item[NR_FORCE_ERROR];
};

struct force_error force_error_vector = {
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
		{"safefault",	&simulate_SAFEFAULT},
		{"pabrt",	&simulate_PABRT},
		{"undef",	&simulate_UNDEF},
		{"dfree",	&simulate_DFREE},
		{"danglingref",	&simulate_DREF},
		{"memcorrupt",	&simulate_MCRPT},
		{"lowmem",	&simulate_LOMEM},
		{"softlockup",	&simulate_SOFT_LOCKUP},
		{"softirqlockup",	&simulate_SOFTIRQ_LOCKUP},
		{"softirqstorm",	&simulate_SOFTIRQ_STORM},
		{"taskhardlockup",	&simulate_TASK_HARD_LOCKUP},
		{"irqhardlockup",	&simulate_IRQ_HARD_LOCKUP},
		{"spinlockup",	&simulate_SPIN_LOCKUP},
		{"spinlock-allcore", 	&simulate_SPINLOCK_ALLCORE},
		{"spinlock-softlockup",	&simulate_SPINLOCK_SOFTLOCKUP},
		{"spinlock-hardlockup",	&simulate_SPINLOCK_HARDLOCKUP},
		{"rwlockup",	&simulate_RW_LOCKUP},
		{"allrwlockup", &simulate_ALLRW_LOCKUP},
		{"pcabort",	&simulate_PC_ABORT},
		{"spabort",	&simulate_SP_ABORT},
		{"jumpzero",	&simulate_JUMP_ZERO},
		{"busmon",	&simulate_BUSMON_ERROR},
		{"unaligned",	&simulate_UNALIGNED},
		{"writero",	&simulate_WRITE_RO},
		{"overflow",	&simulate_OVERFLOW},
		{"badsched",	&simulate_BAD_SCHED},
		{"CM",		&simulate_CORRUPT_MAGIC},
		{"irqstorm",	&simulate_IRQ_STORM},
		{"syncirqlockup",	&simulate_SYNC_IRQ_LOCKUP},
		{"disksleep",	&simulate_DISK_SLEEP},
		{"CDW",		&simulate_CORRUPT_DELAYED_WORK},
		{"mutexaa",	&simulate_MUTEX_AA},
		{"mutexabba",	&simulate_MUTEX_ABBA},
		{"wqlockup",	&simulate_WQ_LOCKUP},
		{"rwsem-r",	&simulate_RWSEM_R},
		{"rwsem-w",	&simulate_RWSEM_W},
		{"printkfault",	&simulate_PRINTK_FAULT},
	}
};

struct debug_delayed_work_info {
	int start;
	u32 work_magic;
	struct delayed_work read_info_work;
};

struct work_struct lockup_work;

static DEFINE_SPINLOCK(sec_debug_test_lock);
static DEFINE_RWLOCK(sec_debug_test_rw_lock);

static int str_to_num(char *s)
{
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
			return (s[0] - '0');

		default:
			return -1;
		}
	}
	return -1;
}

/* timeout for dog bark/bite */
#define DELAY_TIME 30000
#define EXYNOS_PS_HOLD_CONTROL 0x330c

static void pull_down_other_cpus(void)
{
#ifdef CONFIG_HOTPLUG_CPU
	int cpu, ret;

	for (cpu = NR_CPUS - 1; cpu > 0 ; cpu--) {
		ret = cpu_down(cpu);
		if (ret)
			pr_crit("%s: CORE%d ret: %x\n", __func__, cpu, ret);
	}
#endif
}

static void simulate_KP(char *arg)
{
	pr_crit("%s()\n", __func__);
	*(volatile unsigned int *)0x0 = 0x0; /* SVACE: intended */
}

static void simulate_DP(char *arg)
{
	pr_crit("%s()\n", __func__);

	pull_down_other_cpus();

	pr_crit("%s() start to hanging\n", __func__);
	local_irq_disable();
	mdelay(DELAY_TIME);
	local_irq_enable();

	/* should not reach here */
}

static void simulate_QDP(char *arg)
{
	pr_crit("%s()\n", __func__);

	s3c2410wdt_set_emergency_reset(10, 0);

	mdelay(DELAY_TIME);

	/* should not reach here */
}

static void simulate_SVC(char *arg)
{
	pr_crit("%s()\n", __func__);

	asm("svc #0x0");

	/* should not reach here */
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

static void simulate_SFR(char *arg)
{
	int ret, index = 0;
	unsigned long reg, val;
	char tmp[10], *tmparg;
	void __iomem *addr;

	pr_crit("%s() start\n", __func__);

	index = find_blank(arg);
	memcpy(tmp, arg, index);
	tmp[index] = '\0';

	ret = kstrtoul(tmp, 16, &reg);
	addr = ioremap(reg, 0x10);
	if (!addr) {
		pr_crit("%s() failed to remap 0x%lx, quit\n", __func__, reg);
		return;
	}

	pr_crit("%s() 1st parameter: 0x%lx\n", __func__, reg);

	tmparg = &arg[index + 1];

	index = find_blank(tmparg);
	if (index == 0) {
		pr_crit("%s() there is no 2nd parameter\n", __func__);
		pr_crit("%s() try to read 0x%lx\n", __func__, reg);

		ret = __raw_readl(addr);

		pr_crit("%s() result : 0x%x\n", __func__, ret);

	} else {
		memcpy(tmp, tmparg, index);
		tmp[index] = '\0';

		ret = kstrtoul(tmp, 16, &val);
		pr_crit("%s() 2nd parameter: 0x%lx\n", __func__, val);
		pr_crit("%s() try to write 0x%lx to 0x%lx\n", __func__, val, reg);

		__raw_writel(val, addr);
	}


	/* should not reach here */
}

static void simulate_WP(char *arg)
{
#if 0
	unsigned int ps_hold_control;

	pr_crit("%s()\n", __func__);
	exynos_pmu_read(EXYNOS_PS_HOLD_CONTROL, &ps_hold_control);
	exynos_pmu_write(EXYNOS_PS_HOLD_CONTROL, ps_hold_control & 0xFFFFFEFF);
#endif
}

static void simulate_TP(char *arg)
{
	pr_crit("%s()\n", __func__);
}

static void simulate_PANIC(char *arg)
{
	pr_crit("%s()\n", __func__);
	panic("simulate_panic");
}

static void simulate_BUG(char *arg)
{
	pr_crit("%s()\n", __func__);
	BUG();
}

static void simulate_WARN(char *arg)
{
	pr_crit("%s()\n", __func__);
	WARN_ON(1);
}

static void simulate_DABRT(char *arg)
{
#if 0
	pr_crit("%s()\n", __func__);
	*((int *)0) = 0; /* SVACE: intended */
#endif
}

static void simulate_PABRT(char *arg)
{
	pr_crit("%s()\n", __func__);
	((void (*)(void))0x0)(); /* SVACE: intended */
}

static void simulate_UNDEF(char *arg)
{
	pr_crit("%s()\n", __func__);
	asm volatile(".word 0xe7f001f2\n\t");
	unreachable();
}

static void simulate_DFREE(char *arg)
{
	void *p;

	pr_crit("%s()\n", __func__);
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

	pr_crit("%s()\n", __func__);
	p = kmalloc(sizeof(int), GFP_KERNEL);
	if (p) {
		kfree(p);
		*p = 0x1234; /* SVACE: intended */
	}
}

static void simulate_MCRPT(char *arg)
{
	int *ptr;

	pr_crit("%s()\n", __func__);
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

	pr_crit("%s()\n", __func__);
	pr_crit("Allocating memory until failure!\n");
	while (kmalloc(128 * 1024, GFP_KERNEL)) /* SVACE: intended */
		i++;
	pr_crit("Allocated %d KB!\n", i * 128);
}

static void simulate_SOFT_LOCKUP(char *arg)
{
#if 0
	pr_crit("%s()\n", __func__);
#ifdef CONFIG_LOCKUP_DETECTOR
	softlockup_panic = 1;
#endif
	preempt_disable();
	asm("b .");
	preempt_enable();
#endif
}

static struct tasklet_struct sec_debug_tasklet;
static struct hrtimer softirq_storm_hrtimer;
static unsigned long sample_period;

static void softirq_lockup_tasklet(unsigned long data)
{
	asm("b .");
}

static void simulate_SOFTIRQ_handler(void *info)
{
	tasklet_schedule(&sec_debug_tasklet);
}

static void simulate_SOFTIRQ_LOCKUP(char *arg)
{
	int cpu;

	tasklet_init(&sec_debug_tasklet, softirq_lockup_tasklet, (unsigned long)0);
	pr_crit("%s()\n", __func__);

	if (arg) {
		cpu = str_to_num(arg);
		smp_call_function_single(cpu,
					simulate_SOFTIRQ_handler, 0, 0);
	} else {
		for_each_online_cpu(cpu) {
			if (cpu == smp_processor_id())
				continue;
			smp_call_function_single(cpu,
						 simulate_SOFTIRQ_handler,
						 0, 0);
		}
	}
}

static void softirq_storm_tasklet(unsigned long data)
{
	preempt_disable();
	mdelay(500);
	pr_crit("%s\n", __func__);
	preempt_enable();
}

static enum hrtimer_restart softirq_storm_timer_fn(struct hrtimer *hrtimer)
{
	hrtimer_forward_now(hrtimer, ns_to_ktime(sample_period));
	tasklet_schedule(&sec_debug_tasklet);
	return HRTIMER_RESTART;
}

static void simulate_SOFTIRQ_STORM(char *arg)
{
	if ((arg && kstrtol(arg, 10, &sample_period)) || !arg) {
		sample_period = 1000000;
	}
	pr_crit("%s : set period (%d)\n", __func__, (unsigned int)sample_period);

	tasklet_init(&sec_debug_tasklet, softirq_storm_tasklet, 0);

	hrtimer_init(&softirq_storm_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	softirq_storm_hrtimer.function = softirq_storm_timer_fn;
	hrtimer_start(&softirq_storm_hrtimer, ns_to_ktime(sample_period),
	      HRTIMER_MODE_REL_PINNED);
}

int task_hard_lockup(void *info)
{
	while (!kthread_should_stop()) {
		local_irq_disable();
		asm("b .");
	}
	return 0;
}

static void simulate_TASK_HARD_LOCKUP(char *arg)
{
	int cpu;
	struct task_struct *tsk;

	pr_crit("%s()\n", __func__);

	tsk = kthread_create(task_hard_lockup, 0, "hl_test");
	if (IS_ERR(tsk)) {
		pr_warn("Failed to create thread hl_test\n");
		return;
	}

	if (arg) {
		cpu = str_to_num(arg);
		set_cpus_allowed_ptr(tsk, cpumask_of(cpu));
	} else {
		set_cpus_allowed_ptr(tsk, cpumask_of(smp_processor_id()));
	}
	wake_up_process(tsk);
}

static void simulate_IRQ_HARD_LOCKUP_handler(void *info)
{
	asm("b .");
}

static void simulate_IRQ_HARD_LOCKUP(char *arg)
{
	int cpu;

	pr_crit("%s()\n", __func__);

	if (arg) {
		cpu = str_to_num(arg);
		smp_call_function_single(cpu,
					 simulate_IRQ_HARD_LOCKUP_handler, 0, 0);
	} else {
		for_each_online_cpu(cpu) {
			if (cpu == smp_processor_id())
				continue;
			smp_call_function_single(cpu,
						 simulate_IRQ_HARD_LOCKUP_handler,
						 0, 0);
		}
	}
}

static struct pm_qos_request sec_min_pm_qos;

static void simulate_ALLSPIN_LOCKUP_handler(void *info)
{
	unsigned long flags = 0;

	int cpu = smp_processor_id();

	pr_crit("%s()/cpu:%d\n", __func__, cpu);
	spin_lock_irqsave(&sec_debug_test_lock, flags);
	spin_lock_irqsave(&sec_debug_test_lock, flags);
}

static void make_all_cpu_online(void)
{
	pr_crit("%s()\n", __func__);

	pm_qos_add_request(&sec_min_pm_qos, PM_QOS_CPU_ONLINE_MIN,
			   PM_QOS_CPU_ONLINE_MIN_DEFAULT_VALUE);
	pm_qos_update_request(&sec_min_pm_qos,
			      PM_QOS_CPU_ONLINE_MAX_DEFAULT_VALUE);
	while (true) {
		if (num_online_cpus() == PM_QOS_CPU_ONLINE_MAX_DEFAULT_VALUE)
			break;
	}
}

static void simulate_SPINLOCK_ALLCORE(char *arg)
{
	unsigned long flags;

	pr_crit("%s()\n", __func__);

	make_all_cpu_online();
	preempt_disable();
	smp_call_function(simulate_ALLSPIN_LOCKUP_handler, NULL, 0);
	spin_lock_irqsave(&sec_debug_test_lock, flags);
	spin_lock_irqsave(&sec_debug_test_lock, flags);
}

static void simulate_SPINLOCK_SOFTLOCKUP(char *arg)
{
	pr_crit("%s()\n", __func__);

	make_all_cpu_online();
	preempt_disable();
	spin_lock(&sec_debug_test_lock);
	spin_lock(&sec_debug_test_lock);
}

static void simulate_SPINLOCK_HARDLOCKUP(char *arg)
{
	pr_crit("%s()\n", __func__);

	make_all_cpu_online();
	preempt_disable();
	smp_call_function_single(1, simulate_ALLSPIN_LOCKUP_handler, NULL, 0);
	smp_call_function_single(2, simulate_ALLSPIN_LOCKUP_handler, NULL, 0);
	smp_call_function_single(3, simulate_ALLSPIN_LOCKUP_handler, NULL, 0);
	smp_call_function_single(4, simulate_ALLSPIN_LOCKUP_handler, NULL, 0);
}

static void simulate_SAFEFAULT(char *arg)
{
	pr_crit("%s()\n", __func__);

	make_all_cpu_online();
	preempt_disable();

	smp_call_function(simulate_ALLSPIN_LOCKUP_handler, NULL, 0);

	pr_info("%s %p %s %d %p %p %llx\n",
		__func__, current, current->comm, current->pid,
		current_thread_info(), current->stack, current_stack_pointer);

	write_sysreg(0xfafa, sp_el0);
	mb();

	*((volatile unsigned int *)0) = 0;
}

static void simulate_SPIN_LOCKUP(char *arg)
{
	pr_crit("%s()\n", __func__);

	spin_lock(&sec_debug_test_lock);
	spin_lock(&sec_debug_test_lock);
}

static void simulate_RW_LOCKUP(char *arg)
{
	pr_crit("%s()\n", __func__);

	write_lock(&sec_debug_test_rw_lock);
	read_lock(&sec_debug_test_rw_lock);
}

static void simulate_ALLRW_LOCKUP_handler(void *info)
{
	unsigned long flags = 0;

	int cpu = raw_smp_processor_id();

	pr_crit("%s()/cpu:%d\n", __func__, cpu);
	if (cpu % 2)
		read_lock_irqsave(&sec_debug_test_rw_lock, flags);
	else
		write_lock_irqsave(&sec_debug_test_rw_lock, flags);
}

static void simulate_ALLRW_LOCKUP(char *arg)
{
	unsigned long flags;

	pr_crit("%s()\n", __func__);

	make_all_cpu_online();

	write_lock_irqsave(&sec_debug_test_rw_lock, flags);

	smp_call_function(simulate_ALLRW_LOCKUP_handler, NULL, 0);

	read_lock_irqsave(&sec_debug_test_rw_lock, flags);
}

static void simulate_PC_ABORT(char *arg)
{
	pr_crit("%s()\n", __func__);
	asm("add x30, x30, #0x1\n\t"
	    "ret");
}

static void simulate_SP_ABORT(char *arg)
{
	pr_crit("%s()\n", __func__);
	asm("mov x29, #0xff00\n\t"
	    "mov sp, #0xff00\n\t"
	    "ret");
}

static void simulate_JUMP_ZERO(char *arg)
{
	pr_crit("%s()\n", __func__);
	asm("mov x0, #0x0\n\t"
	    "br x0");
}

static void simulate_BUSMON_ERROR(char *arg)
{
	pr_crit("%s()\n", __func__);
}

static void simulate_UNALIGNED(char *arg)
{
	static u8 data[5] __aligned(4) = {1, 2, 3, 4, 5};
	u32 *p;
	u32 val = 0x12345678;

	pr_crit("%s()\n", __func__);

	p = (u32 *)(data + 1);
	if (*p == 0)
		val = 0x87654321;
	*p = val;
}

static void simulate_WRITE_RO(char *arg)
{
	unsigned long *ptr;

	pr_crit("%s()\n", __func__);
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

	/*sub sp, sp, #(S_FRAME_SIZE+PRESERVE_STACK_SIZE) = 320+256 = 576 @kernel_ventry*/
	if((unsigned long)(current->stack)+575 > current_stack_pointer)
		*((volatile unsigned int *)0) = 0;

	/* Make sure compiler does not optimize this away. */
	memset(buf, (remaining & 0xff) | 0x1, BUFFER_SIZE);
	if (!remaining)
		return 0;
	else
		return recursive_loop(remaining - 1);
}

static void simulate_OVERFLOW(char *arg)
{
	pr_crit("%s()\n", __func__);

	recursive_loop(1000);
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

	pr_crit("%s()\n", __func__);

	while (true) {
		tries++;
		pr_crit("%dth try.\n", tries);
		for_each_online_cpu(cpu) {
			if (idle_cpu(cpu))
				smp_call_function_single(cpu, simulate_BAD_SCHED_handler, &ret, 1);
			if (ret)
				return;	/* success */
		}
		mdelay(100);
	}
}

static void simulate_CORRUPT_MAGIC(char *arg)
{
	int magic;

	pr_crit("%s()\n", __func__);

	if (arg) {
		magic = str_to_num(arg);
		simulate_extra_info_force_error(magic);
	} else {
		simulate_extra_info_force_error(0);
	}
}

static void simulate_IRQ_STORM(char *arg)
{
	int i;
	long irq;

	pr_crit("%s()\n", __func__);

	if (arg) {
		if (!kstrtol(arg, 10, &irq))
			irq_set_irq_type((unsigned int)irq, IRQF_TRIGGER_HIGH | IRQF_SHARED);
		else
			pr_crit("%s : wrong irq number (%d)\n", __func__, (unsigned int)irq);
	} else {
		for_each_irq_nr(i) {			
			struct irq_desc *desc = irq_to_desc(i);

			if (desc && desc->action && desc->action->name)
				if (!strcmp(desc->action->name, "gpio-keys: KEY_WINK")) {
					irq_set_irq_type(i, IRQF_TRIGGER_HIGH | IRQF_SHARED);
					break;
				}
		}
		if (i == nr_irqs)
			pr_crit("%s : irq (gpio-keys: KEY_WINK) not found\n", __func__);

	}
}

static void dummy_wait_for_completion(void)
{
	DECLARE_COMPLETION_ONSTACK(done);

	pr_crit("%s()\n", __func__);

	wait_for_completion(&done);
}

static irqreturn_t dummy_wait_for_completion_irq_handler(int irq, void *data)
{
	dummy_wait_for_completion();
	return IRQ_HANDLED;
}

static void simulate_SYNC_IRQ_LOCKUP(char *arg)
{
	int i;
	long irq;

	pr_crit("%s()\n", __func__);

	if (arg) {
		if (!kstrtol(arg, 10, &irq)) {
			struct irq_desc *desc = irq_to_desc(i);

			if (desc && desc->action && desc->action->thread_fn)
				desc->action->thread_fn = dummy_wait_for_completion_irq_handler;	
		}
		else {
			pr_crit("%s : wrong irq number (%d)\n", __func__, (unsigned int)irq);
		}
	} else {
		for_each_irq_nr(i) {			
			struct irq_desc *desc = irq_to_desc(i);

			if (desc && desc->action && desc->action->name && desc->action->thread_fn)
				if (!strcmp(desc->action->name, "sec_ts")) {
					desc->action->thread_fn = dummy_wait_for_completion_irq_handler;	
					break;
				}
		}
		if (i == nr_irqs)
			pr_crit("%s : irq (sec_ts) not found\n", __func__);

	}
}

static void simulate_DISK_SLEEP(char *arg)
{
	dummy_wait_for_completion();
}

static void sec_debug_work(struct work_struct *work)
{
	struct debug_delayed_work_info *info = container_of(work, struct debug_delayed_work_info,
							    read_info_work.work);

	pr_crit("%s info->work_magic : %d\n", __func__, info->work_magic);
}

static void sec_debug_wq_lockup(struct work_struct *work)
{
	pr_crit("%s\n", __func__);
	asm("b .");
}

static void simulate_CORRUPT_DELAYED_WORK(char *arg)
{
	struct debug_delayed_work_info *info;

	info = kzalloc(sizeof(struct debug_delayed_work_info), GFP_KERNEL);

	pr_info("%s(): address of info is 0x%p", __func__, info);

	if (!info)
		return;

	info->start = true;
	info->work_magic = 0xE055E055;

	INIT_DELAYED_WORK(&info->read_info_work, sec_debug_work);
	schedule_delayed_work(&info->read_info_work, msecs_to_jiffies(5000));
	kfree(info);
}

DEFINE_MUTEX(sec_debug_test_mutex_0);
DEFINE_MUTEX(sec_debug_test_mutex_1);

static void test_mutex_aa(struct mutex *lock)
{
	pr_crit("%s()\n", __func__);

	mutex_lock(lock);

	mutex_lock(lock);
}

static void simulate_MUTEX_AA(char *arg)
{
	int num;

	pr_crit("%s()\n", __func__);

	if (arg)
		num = str_to_num(arg);
	else
		num = 0;

	switch (num % 2) {
	case 1:
		test_mutex_aa(&sec_debug_test_mutex_1);
		break;
	case 0:
	default:
		test_mutex_aa(&sec_debug_test_mutex_0);
		break;
	}

}

struct test_abba {
	struct work_struct work;
	struct mutex a_mutex;
	struct mutex b_mutex;
	struct completion a_ready;
	struct completion b_ready;
};

static void test_abba_work(struct work_struct *work)
{
	struct test_abba *abba = container_of(work, typeof(*abba), work);

	mutex_lock(&abba->b_mutex);

	complete(&abba->b_ready);
	wait_for_completion(&abba->a_ready);

	mutex_lock(&abba->a_mutex);

	pr_err("%s: got 2 mutex\n", __func__);

	mutex_unlock(&abba->a_mutex);
	mutex_unlock(&abba->b_mutex);
}

static void test_mutex_abba(void)
{
	struct test_abba abba;

	mutex_init(&abba.a_mutex);
	mutex_init(&abba.b_mutex);
	INIT_WORK_ONSTACK(&abba.work, test_abba_work);
	init_completion(&abba.a_ready);
	init_completion(&abba.b_ready);

	schedule_work(&abba.work);

	mutex_lock(&abba.a_mutex);

	complete(&abba.a_ready);
	wait_for_completion(&abba.b_ready);

	mutex_lock(&abba.b_mutex);

	pr_err("%s: got 2 mutex\n", __func__);

	mutex_unlock(&abba.b_mutex);
	mutex_unlock(&abba.a_mutex);

	flush_work(&abba.work);
	destroy_work_on_stack(&abba.work);
}

static void simulate_MUTEX_ABBA(char *arg)
{
	pr_crit("%s()\n", __func__);

	test_mutex_abba();
}

static void simulate_WQ_LOCKUP(char *arg)
{
	int cpu;

	pr_crit("%s()\n", __func__);
	INIT_WORK(&lockup_work, sec_debug_wq_lockup);

	if (arg) {
		cpu = str_to_num(arg);

		if (cpu >= 0 && cpu <= num_possible_cpus() - 1) {
			pr_crit("Put works into cpu%d\n", cpu);
			schedule_work_on(cpu, &lockup_work);
		}
	}
}

struct test_resem {
	struct work_struct work;
	struct completion a_ready;
	struct completion b_ready;
};

static DECLARE_RWSEM(secdbg_test_rwsem);
static DEFINE_MUTEX(secdbg_test_mutex_for_rwsem);

static void test_rwsem_read_work(struct work_struct *work)
{
	struct test_resem *t = container_of(work, typeof(*t), work);

	pr_crit("%s: trying read\n", __func__);
	down_read(&secdbg_test_rwsem);

	complete(&t->b_ready);
	wait_for_completion(&t->a_ready);

	mutex_lock(&secdbg_test_mutex_for_rwsem);

	pr_crit("%s: error\n", __func__);

	mutex_unlock(&secdbg_test_mutex_for_rwsem);
	up_read(&secdbg_test_rwsem);
}

static void simulate_RWSEM_R(char *arg)
{
	struct test_resem twork;

	pr_crit("%s()\n", __func__);

	INIT_WORK_ONSTACK(&twork.work, test_rwsem_read_work);
	init_completion(&twork.a_ready);
	init_completion(&twork.b_ready);

	schedule_work(&twork.work);

	mutex_lock(&secdbg_test_mutex_for_rwsem);

	complete(&twork.a_ready);
	wait_for_completion(&twork.b_ready);

	pr_crit("%s: trying write\n", __func__);
	down_write(&secdbg_test_rwsem);

	pr_crit("%s: error\n", __func__);

	up_write(&secdbg_test_rwsem);
	mutex_unlock(&secdbg_test_mutex_for_rwsem);
}

static void test_rwsem_write_work(struct work_struct *work)
{
	struct test_resem *t = container_of(work, typeof(*t), work);

	pr_crit("%s: trying write\n", __func__);
	down_write(&secdbg_test_rwsem);

	complete(&t->b_ready);
	wait_for_completion(&t->a_ready);

	mutex_lock(&secdbg_test_mutex_for_rwsem);

	pr_crit("%s: error\n", __func__);

	mutex_unlock(&secdbg_test_mutex_for_rwsem);
	up_write(&secdbg_test_rwsem);
}

static void simulate_RWSEM_W(char *arg)
{
	struct test_resem twork;

	pr_crit("%s()\n", __func__);

	INIT_WORK_ONSTACK(&twork.work, test_rwsem_write_work);
	init_completion(&twork.a_ready);
	init_completion(&twork.b_ready);

	schedule_work(&twork.work);

	mutex_lock(&secdbg_test_mutex_for_rwsem);

	complete(&twork.a_ready);
	wait_for_completion(&twork.b_ready);

	pr_crit("%s: trying read\n", __func__);
	down_read(&secdbg_test_rwsem);

	pr_crit("%s: error\n", __func__);

	up_read(&secdbg_test_rwsem);
	mutex_unlock(&secdbg_test_mutex_for_rwsem);
}

static void simulate_PRINTK_FAULT(char *arg)
{
	pr_crit("%s()\n", __func__);
	pr_err("%s: trying fault: %s\n", __func__, (char *)0x80000000);
}

static int sec_debug_get_force_error(char *buffer, const struct kernel_param *kp)
{
	int i;
	int size = 0;

	for (i = 0; i < NR_FORCE_ERROR; i++)
		size += scnprintf(buffer + size, PAGE_SIZE - size, "%s\n",
				  force_error_vector.item[i].errname);

	return size;
}

static int sec_debug_set_force_error(const char *val, const struct kernel_param *kp)
{
	int i;
	char *temp;
	char *ptr;

	for (i = 0; i < NR_FORCE_ERROR; i++)
		if (!strncmp(val, force_error_vector.item[i].errname,
			     strlen(force_error_vector.item[i].errname))) {
			temp = (char *)val;
			ptr = strsep(&temp, " ");	/* ignore the first token */
			ptr = strsep(&temp, " ");	/* take the second token */
			force_error_vector.item[i].errfunc(ptr);
	}
	return 0;
}

static const struct kernel_param_ops sec_debug_force_error_ops = {
	.set	= sec_debug_set_force_error,
	.get	= sec_debug_get_force_error,
};

module_param_cb(force_error, &sec_debug_force_error_ops, NULL, 0600);

