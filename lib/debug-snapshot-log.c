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
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/kallsyms.h>
#include <linux/platform_device.h>
#include <linux/clk-provider.h>
#include <linux/pstore_ram.h>
#include <linux/sched/clock.h>
#include <linux/ftrace.h>

#include "debug-snapshot-local.h"
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/hardirq.h>
#include <asm/stacktrace.h>
#include <asm/arch_timer.h>
#include <linux/debug-snapshot.h>
#include <linux/kernel_stat.h>
#include <linux/irqnr.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/nmi.h>
#include <linux/sec_debug.h>

struct dbg_snapshot_lastinfo {
#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
	atomic_t freq_last_idx[DSS_FLAG_END];
#endif
	char log[DSS_NR_CPUS][SZ_1K];
	char *last_p[DSS_NR_CPUS];
};

struct dss_dumper {
	bool active;
	u32 items;
	int init_idx;
	int cur_idx;
	u32 cur_cpu;
	u32 step;
};

enum dss_kevent_flag {
	DSS_FLAG_TASK = 1,
	DSS_FLAG_WORK,
	DSS_FLAG_CPUIDLE,
	DSS_FLAG_SUSPEND,
	DSS_FLAG_IRQ,
	DSS_FLAG_IRQ_EXIT,
	DSS_FLAG_SPINLOCK,
	DSS_FLAG_IRQ_DISABLE,
	DSS_FLAG_CLK,
	DSS_FLAG_FREQ,
	DSS_FLAG_REG,
	DSS_FLAG_HRTIMER,
	DSS_FLAG_REGULATOR,
	DSS_FLAG_THERMAL,
	DSS_FLAG_MAILBOX,
	DSS_FLAG_CLOCKEVENT,
	DSS_FLAG_PRINTK,
	DSS_FLAG_PRINTKL,
	DSS_FLAG_KEVENT,
};

struct dbg_snapshot_log_idx {
	atomic_t task_log_idx[DSS_NR_CPUS];
	atomic_t work_log_idx[DSS_NR_CPUS];
	atomic_t cpuidle_log_idx[DSS_NR_CPUS];
	atomic_t suspend_log_idx;
	atomic_t irq_log_idx[DSS_NR_CPUS];
#ifdef CONFIG_DEBUG_SNAPSHOT_SPINLOCK
	atomic_t spinlock_log_idx[DSS_NR_CPUS];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_DISABLED
	atomic_t irqs_disabled_log_idx[DSS_NR_CPUS];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REG
	atomic_t reg_log_idx[DSS_NR_CPUS];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_HRTIMER
	atomic_t hrtimer_log_idx[DSS_NR_CPUS];
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_CLK
	atomic_t clk_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
	atomic_t pmu_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
	atomic_t freq_log_idx;
	atomic_t freq_misc_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_DM
	atomic_t dm_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	atomic_t regulator_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	atomic_t thermal_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_I2C
	atomic_t i2c_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_SPI
	atomic_t spi_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_BINDER
	atomic_t binder_log_idx;
#endif
#ifndef CONFIG_DEBUG_SNAPSHOT_MINIMIZED_MODE
	atomic_t printkl_log_idx;
	atomic_t printk_log_idx;
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_ACPM
	atomic_t acpm_log_idx;
#endif
};

int dbg_snapshot_log_size = sizeof(struct dbg_snapshot_log);
/*
 *  including or excluding options
 *  if you want to except some interrupt, it should be written in this array
 */
int dss_irqlog_exlist[DSS_EX_MAX_NUM] = {
/*  interrupt number ex) 152, 153, 154, */
	-1,
};

#ifdef CONFIG_DEBUG_SNAPSHOT_REG
struct dss_reg_list {
	size_t addr;
	size_t size;
};

static struct dss_reg_list dss_reg_exlist[] = {
/*
 *  if it wants to reduce effect enabled reg feautre to system,
 *  you must add these registers - mct, serial
 *  because they are called very often.
 *  physical address, size ex) {0x10C00000, 0x1000},
 */
	{DSS_REG_MCT_ADDR, DSS_REG_MCT_SIZE},
	{DSS_REG_UART_ADDR, DSS_REG_UART_SIZE},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
static char *dss_freq_name[] = {
	"LIT", "MID", "BIG", "INT", "MIF", "ISP", "DISP", "INTCAM", "AUD", "IVA", "SCORE", "FSYS0", "MFC", "NPU", "G3D",
};
#endif

/*  Internal interface variable */
static struct dbg_snapshot_log_idx dss_idx;
static struct dbg_snapshot_lastinfo dss_lastinfo;

void __init dbg_snapshot_init_log_idx(void)
{
	int i;

#ifndef CONFIG_DEBUG_SNAPSHOT_MINIMIZED_MODE
	atomic_set(&(dss_idx.printk_log_idx), -1);
	atomic_set(&(dss_idx.printkl_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
	atomic_set(&(dss_idx.regulator_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_THERMAL
	atomic_set(&(dss_idx.thermal_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
	atomic_set(&(dss_idx.freq_log_idx), -1);
	atomic_set(&(dss_idx.freq_misc_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_DM
	atomic_set(&(dss_idx.dm_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_CLK
	atomic_set(&(dss_idx.clk_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
	atomic_set(&(dss_idx.pmu_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_ACPM
	atomic_set(&(dss_idx.acpm_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_I2C
	atomic_set(&(dss_idx.i2c_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_SPI
	atomic_set(&(dss_idx.spi_log_idx), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_BINDER
	atomic_set(&(dss_idx.binder_log_idx), -1);
#endif
	atomic_set(&(dss_idx.suspend_log_idx), -1);

	for (i = 0; i < DSS_NR_CPUS; i++) {
		atomic_set(&(dss_idx.task_log_idx[i]), -1);
		atomic_set(&(dss_idx.work_log_idx[i]), -1);
		atomic_set(&(dss_idx.cpuidle_log_idx[i]), -1);
		atomic_set(&(dss_idx.irq_log_idx[i]), -1);
#ifdef CONFIG_DEBUG_SNAPSHOT_SPINLOCK
		atomic_set(&(dss_idx.spinlock_log_idx[i]), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_DISABLED
		atomic_set(&(dss_idx.irqs_disabled_log_idx[i]), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_REG
		atomic_set(&(dss_idx.reg_log_idx[i]), -1);
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_HRTIMER
		atomic_set(&(dss_idx.hrtimer_log_idx[i]), -1);
#endif
	}
}

unsigned long sec_debug_get_kevent_index_addr(int type)
{
	switch (type) {
	case DSS_KEVENT_TASK:
		return virt_to_phys(&(dss_idx.task_log_idx[0]));

	case DSS_KEVENT_WORK:
		return virt_to_phys(&(dss_idx.work_log_idx[0]));

	case DSS_KEVENT_IRQ:
		return virt_to_phys(&(dss_idx.irq_log_idx[0]));

	case DSS_KEVENT_FREQ:
		return virt_to_phys(&(dss_idx.freq_log_idx));

	case DSS_KEVENT_IDLE:
		return virt_to_phys(&(dss_idx.cpuidle_log_idx[0]));

	case DSS_KEVENT_THRM:
		return virt_to_phys(&(dss_idx.thermal_log_idx));

	case DSS_KEVENT_ACPM:
		return virt_to_phys(&(dss_idx.acpm_log_idx));

	case DSS_KEVENT_MFRQ:
		return virt_to_phys(&(dss_idx.freq_misc_log_idx));

	default:
		return 0;
	}
}

bool dbg_snapshot_dumper_one(void *v_dumper, char *line, size_t size, size_t *len)
{
	bool ret = false;
	int idx, array_size;
	unsigned int cpu, items;
	unsigned long rem_nsec;
	u64 ts;
	struct dss_dumper *dumper = (struct dss_dumper *)v_dumper;

	if (!line || size < SZ_128 ||
		dumper->cur_cpu >= NR_CPUS)
		goto out;

	if (dumper->active) {
		if (dumper->init_idx == dumper->cur_idx)
			goto out;
	}

	cpu = dumper->cur_cpu;
	idx = dumper->cur_idx;
	items = dumper->items;

	switch(items) {
	case DSS_FLAG_TASK:
	{
		struct task_struct *task;
		array_size = ARRAY_SIZE(dss_log->task[0]) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.task_log_idx[0]) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->task[cpu][idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);
		task = dss_log->task[cpu][idx].task;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] task_name:%16s,  "
					    "task:0x%16p,  stack:0x%16p,  exec_start:%16llu\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						task->comm, task, task->stack,
						task->se.exec_start);
		break;
	}
	case DSS_FLAG_WORK:
	{
		char work_fn[KSYM_NAME_LEN] = {0,};
		char *task_comm;
		int en;

		array_size = ARRAY_SIZE(dss_log->work[0]) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.work_log_idx[0]) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->work[cpu][idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);
		lookup_symbol_name((unsigned long)dss_log->work[cpu][idx].fn, work_fn);
		task_comm = dss_log->work[cpu][idx].task_comm;
		en = dss_log->work[cpu][idx].en;

		dumper->step = 6;
		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] task_name:%16s,  work_fn:%32s,  %3s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						task_comm, work_fn,
						en == DSS_FLAG_IN ? "IN" : "OUT");
		break;
	}
	case DSS_FLAG_CPUIDLE:
	{
		unsigned int delta;
		int state, num_cpus, en;
		char *index;

		array_size = ARRAY_SIZE(dss_log->cpuidle[0]) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.cpuidle_log_idx[0]) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->cpuidle[cpu][idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);

		index = dss_log->cpuidle[cpu][idx].modes;
		en = dss_log->cpuidle[cpu][idx].en;
		state = dss_log->cpuidle[cpu][idx].state;
		num_cpus = dss_log->cpuidle[cpu][idx].num_online_cpus;
		delta = dss_log->cpuidle[cpu][idx].delta;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] cpuidle: %s,  "
					    "state:%d,  num_online_cpus:%d,  stay_time:%8u,  %3s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						index, state, num_cpus, delta,
						en == DSS_FLAG_IN ? "IN" : "OUT");
		break;
	}
	case DSS_FLAG_SUSPEND:
	{
		char suspend_fn[KSYM_NAME_LEN];
		int en;

		array_size = ARRAY_SIZE(dss_log->suspend) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.suspend_log_idx) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->suspend[idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);

		lookup_symbol_name((unsigned long)dss_log->suspend[idx].fn, suspend_fn);
		en = dss_log->suspend[idx].en;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] suspend_fn:%s,  %3s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						suspend_fn, en == DSS_FLAG_IN ? "IN" : "OUT");
		break;
	}
	case DSS_FLAG_IRQ:
	{
		char irq_fn[KSYM_NAME_LEN];
		int en, irq;

		array_size = ARRAY_SIZE(dss_log->irq[0]) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.irq_log_idx[0]) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->irq[cpu][idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);

		lookup_symbol_name((unsigned long)dss_log->irq[cpu][idx].fn, irq_fn);
		irq = dss_log->irq[cpu][idx].irq;
		en = dss_log->irq[cpu][idx].en;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] irq:%6d,  irq_fn:%32s,  %3s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						irq, irq_fn, en == DSS_FLAG_IN ? "IN" : "OUT");
		break;
	}
#ifdef CONFIG_DEBUG_SNAPSHOT_SPINLOCK
	case DSS_FLAG_SPINLOCK:
	{
		unsigned int jiffies_local;
		char callstack[CONFIG_DEBUG_SNAPSHOT_CALLSTACK][KSYM_NAME_LEN];
		int en, i;
		u16 next, owner;

		array_size = ARRAY_SIZE(dss_log->spinlock[0]) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.spinlock_log_idx[0]) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->spinlock[cpu][idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);

		jiffies_local = dss_log->spinlock[cpu][idx].jiffies;
		en = dss_log->spinlock[cpu][idx].en;
		for (i = 0; i < CONFIG_DEBUG_SNAPSHOT_CALLSTACK; i++)
			lookup_symbol_name((unsigned long)dss_log->spinlock[cpu][idx].caller[i],
						callstack[i]);

		next = dss_log->spinlock[cpu][idx].next;
		owner = dss_log->spinlock[cpu][idx].owner;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] next:%8x,  owner:%8x  jiffies:%12u,  %3s\n"
					    "callstack: %s\n"
					    "           %s\n"
					    "           %s\n"
					    "           %s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						next, owner, jiffies_local,
						en == DSS_FLAG_IN ? "IN" : "OUT",
						callstack[0], callstack[1], callstack[2], callstack[3]);
		break;
	}
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_CLK
	case DSS_FLAG_CLK:
	{
		const char *clk_name;
		char clk_fn[KSYM_NAME_LEN];
		struct clk_hw *clk;
		int en;

		array_size = ARRAY_SIZE(dss_log->clk) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.clk_log_idx) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->clk[idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);

		clk = (struct clk_hw *)dss_log->clk[idx].clk;
		clk_name = clk_hw_get_name(clk);
		lookup_symbol_name((unsigned long)dss_log->clk[idx].f_name, clk_fn);
		en = dss_log->clk[idx].mode;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU] clk_name:%30s,  clk_fn:%30s,  "
					    ",  %s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx,
						clk_name, clk_fn, en == DSS_FLAG_IN ? "IN" : "OUT");
		break;
	}
#endif
#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
	case DSS_FLAG_FREQ:
	{
		char *freq_name;
		unsigned int on_cpu;
		unsigned long old_freq, target_freq;
		int en;

		array_size = ARRAY_SIZE(dss_log->freq) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.freq_log_idx) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->freq[idx].time;
		rem_nsec = do_div(ts, NSEC_PER_SEC);

		freq_name = dss_log->freq[idx].freq_name;
		old_freq = dss_log->freq[idx].old_freq;
		target_freq = dss_log->freq[idx].target_freq;
		on_cpu = dss_log->freq[idx].cpu;
		en = dss_log->freq[idx].en;

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] freq_name:%16s,  "
					    "old_freq:%16lu,  target_freq:%16lu,  %3s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, on_cpu,
						freq_name, old_freq, target_freq,
						en == DSS_FLAG_IN ? "IN" : "OUT");
		break;
	}
#endif
#ifndef CONFIG_DEBUG_SNAPSHOT_MINIMIZED_MODE
	case DSS_FLAG_PRINTK:
	{
		char *log;
		char callstack[CONFIG_DEBUG_SNAPSHOT_CALLSTACK][KSYM_NAME_LEN];
		unsigned int cpu;
		int i;

		array_size = ARRAY_SIZE(dss_log->printk) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.printk_log_idx) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->printk[idx].time;
		cpu = dss_log->printk[idx].cpu;
		rem_nsec = do_div(ts, NSEC_PER_SEC);
		log = dss_log->printk[idx].log;
		for (i = 0; i < CONFIG_DEBUG_SNAPSHOT_CALLSTACK; i++)
			lookup_symbol_name((unsigned long)dss_log->printk[idx].caller[i],
						callstack[i]);

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] log:%s, callstack:%s, %s, %s, %s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						log, callstack[0], callstack[1], callstack[2], callstack[3]);
		break;
	}
	case DSS_FLAG_PRINTKL:
	{
		char callstack[CONFIG_DEBUG_SNAPSHOT_CALLSTACK][KSYM_NAME_LEN];
		size_t msg, val;
		unsigned int cpu;
		int i;

		array_size = ARRAY_SIZE(dss_log->printkl) - 1;
		if (!dumper->active) {
			idx = (atomic_read(&dss_idx.printkl_log_idx) + 1) & array_size;
			dumper->init_idx = idx;
			dumper->active = true;
		}
		ts = dss_log->printkl[idx].time;
		cpu = dss_log->printkl[idx].cpu;
		rem_nsec = do_div(ts, NSEC_PER_SEC);
		msg = dss_log->printkl[idx].msg;
		val = dss_log->printkl[idx].val;
		for (i = 0; i < CONFIG_DEBUG_SNAPSHOT_CALLSTACK; i++)
			lookup_symbol_name((unsigned long)dss_log->printkl[idx].caller[i],
						callstack[i]);

		*len = snprintf(line, size, "[%8lu.%09lu][%04d:CPU%u] msg:%zx, val:%zx, callstack: %s, %s, %s, %s\n",
						(unsigned long)ts, rem_nsec / NSEC_PER_USEC, idx, cpu,
						msg, val, callstack[0], callstack[1], callstack[2], callstack[3]);
		break;
	}
#endif
	default:
		snprintf(line, size, "unsupported inforation to dump\n");
		goto out;
	}
	if (array_size == idx)
		dumper->cur_idx = 0;
	else
		dumper->cur_idx = idx + 1;

	ret = true;
out:
	return ret;
}

#ifdef CONFIG_ARM64
static inline unsigned long pure_arch_local_irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"mrs	%0, daif		// arch_local_irq_save\n"
		"msr	daifset, #2"
		: "=r" (flags)
		:
		: "memory");

	return flags;
}

static inline void pure_arch_local_irq_restore(unsigned long flags)
{
	asm volatile(
		"msr    daif, %0                // arch_local_irq_restore"
		:
		: "r" (flags)
		: "memory");
}
#else
static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	asm volatile(
		"	mrs	%0, cpsr	@ arch_local_irq_save\n"
		"	cpsid	i"
		: "=r" (flags) : : "memory", "cc");
	return flags;
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	asm volatile(
		"	msr	cpsr_c, %0	@ local_irq_restore"
		:
		: "r" (flags)
		: "memory", "cc");
}
#endif

void dbg_snapshot_task(int cpu, void *v_task)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		unsigned long i = atomic_inc_return(&dss_idx.task_log_idx[cpu]) &
				    (ARRAY_SIZE(dss_log->task[0]) - 1);

		dss_log->task[cpu][i].time = cpu_clock(cpu);
		dss_log->task[cpu][i].sp = (unsigned long)current_stack_pointer;
		dss_log->task[cpu][i].task = (struct task_struct *)v_task;
		dss_log->task[cpu][i].pid = (int)((struct task_struct *)v_task)->pid;
		strncpy(dss_log->task[cpu][i].task_comm,
			dss_log->task[cpu][i].task->comm,
			TASK_COMM_LEN - 1);
	}
}

void dbg_snapshot_work(void *worker, void *v_task, void *fn, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;

	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.work_log_idx[cpu]) &
					(ARRAY_SIZE(dss_log->work[0]) - 1);
		struct task_struct *task = (struct task_struct *)v_task;
		dss_log->work[cpu][i].time = cpu_clock(cpu);
		dss_log->work[cpu][i].sp = (unsigned long) current_stack_pointer;
		dss_log->work[cpu][i].worker = (struct worker *)worker;
		strncpy(dss_log->work[cpu][i].task_comm, task->comm, TASK_COMM_LEN - 1);
		dss_log->work[cpu][i].fn = (work_func_t)fn;
		dss_log->work[cpu][i].en = en;
	}
}

void dbg_snapshot_cpuidle(char *modes, unsigned state, int diff, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.cpuidle_log_idx[cpu]) &
				(ARRAY_SIZE(dss_log->cpuidle[0]) - 1);

		dss_log->cpuidle[cpu][i].time = cpu_clock(cpu);
		dss_log->cpuidle[cpu][i].modes = modes;
		dss_log->cpuidle[cpu][i].state = state;
		dss_log->cpuidle[cpu][i].sp = (unsigned long) current_stack_pointer;
		dss_log->cpuidle[cpu][i].num_online_cpus = num_online_cpus();
		dss_log->cpuidle[cpu][i].delta = diff;
		dss_log->cpuidle[cpu][i].en = en;
	}
}

void dbg_snapshot_suspend(char *log, void *fn, void *dev, int state, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int len;
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.suspend_log_idx) &
				(ARRAY_SIZE(dss_log->suspend) - 1);

		dss_log->suspend[i].time = cpu_clock(cpu);
		dss_log->suspend[i].sp = (unsigned long) current_stack_pointer;

		if (log) {
			len = strlen(log);
			memcpy(dss_log->suspend[i].log, log,
					len < DSS_LOG_GEN_LEN ?
					len : DSS_LOG_GEN_LEN - 1);
		} else {
			memset(dss_log->suspend[i].log, 0, DSS_LOG_GEN_LEN - 1);
		}

		dss_log->suspend[i].fn = fn;
		dss_log->suspend[i].dev = (struct device *)dev;
		dss_log->suspend[i].core = cpu;
		dss_log->suspend[i].en = en;
	}
}

static void dbg_snapshot_print_calltrace(void)
{
	int i;

	pr_info("\n<Call trace>\n");
	for (i = 0; i < DSS_NR_CPUS; i++) {
		pr_info("CPU ID: %d -----------------------------------------------\n", i);
		pr_info("%s", dss_lastinfo.log[i]);
	}
}

void dbg_snapshot_save_log(int cpu, unsigned long where)
{
	if (dss_lastinfo.last_p[cpu] == NULL)
		dss_lastinfo.last_p[cpu] = &dss_lastinfo.log[cpu][0];

	if (dss_lastinfo.last_p[cpu] > &dss_lastinfo.log[cpu][SZ_1K - SZ_128])
		return;

	*(unsigned long *)&(dss_lastinfo.last_p[cpu]) += sprintf(dss_lastinfo.last_p[cpu],
			"[<%p>] %pS\n", (void *)where, (void *)where);

}

static void dbg_snapshot_get_sec(unsigned long long ts, unsigned long *sec, unsigned long *msec)
{
	*sec = ts / NSEC_PER_SEC;
	*msec = (ts % NSEC_PER_SEC) / USEC_PER_MSEC;
}

static void dbg_snapshot_print_last_irq(int cpu)
{
	unsigned long idx, sec, msec;
	char fn_name[KSYM_NAME_LEN];

	idx = atomic_read(&dss_idx.irq_log_idx[cpu]) & (ARRAY_SIZE(dss_log->irq[0]) - 1);
	dbg_snapshot_get_sec(dss_log->irq[cpu][idx].time, &sec, &msec);
	lookup_symbol_name((unsigned long)dss_log->irq[cpu][idx].fn, fn_name);

	pr_info("%-16s: [%4lu] %10lu.%06lu sec, %10s: %24s, %8s: %8d, %10s: %2d, %s\n",
			">>> last irq", idx, sec, msec,
			"handler", fn_name,
			"irq", dss_log->irq[cpu][idx].irq,
			"en", dss_log->irq[cpu][idx].en,
			(dss_log->irq[cpu][idx].en == 1) ? "[Missmatch]" : "");
}

static void dbg_snapshot_print_last_task(int cpu)
{
	unsigned long idx, sec, msec;
	struct task_struct *task;

	idx = atomic_read(&dss_idx.task_log_idx[cpu]) & (ARRAY_SIZE(dss_log->task[0]) - 1);
	dbg_snapshot_get_sec(dss_log->task[cpu][idx].time, &sec, &msec);
	task = dss_log->task[cpu][idx].task;

	pr_info("%-16s: [%4lu] %10lu.%06lu sec, %10s: %24s, %8s: 0x%-16p, %10s: %16llu\n",
			">>> last task", idx, sec, msec,
			"task_comm", (task) ? task->comm : "NULL",
			"task", task,
			"exec_start", (task) ? task->se.exec_start : 0);
}

static void dbg_snapshot_print_last_work(int cpu)
{
	unsigned long idx, sec, msec;
	char fn_name[KSYM_NAME_LEN];

	idx = atomic_read(&dss_idx.work_log_idx[cpu]) & (ARRAY_SIZE(dss_log->work[0]) - 1);
	dbg_snapshot_get_sec(dss_log->work[cpu][idx].time, &sec, &msec);
	lookup_symbol_name((unsigned long)dss_log->work[cpu][idx].fn, fn_name);

	pr_info("%-16s: [%4lu] %10lu.%06lu sec, %10s: %24s, %8s: %20s, %3s: %3d %s\n",
			">>> last work", idx, sec, msec,
			"task_name", dss_log->work[cpu][idx].task_comm,
			"work_fn", fn_name,
			"en", dss_log->work[cpu][idx].en,
			(dss_log->work[cpu][idx].en == 1) ? "[Missmatch]" : "");
}

static void dbg_snapshot_print_last_cpuidle(int cpu)
{
	unsigned long idx, sec, msec;

	idx = atomic_read(&dss_idx.cpuidle_log_idx[cpu]) & (ARRAY_SIZE(dss_log->cpuidle[0]) - 1);
	dbg_snapshot_get_sec(dss_log->cpuidle[cpu][idx].time, &sec, &msec);

	pr_info("%-16s: [%4lu] %10lu.%06lu sec, %10s: %24d, %8s: %4s, %6s: %3d, %12s: %2d, %3s: %3d %s\n",
			">>> last cpuidle", idx, sec, msec,
			"stay time", dss_log->cpuidle[cpu][idx].delta,
			"modes", dss_log->cpuidle[cpu][idx].modes,
			"state", dss_log->cpuidle[cpu][idx].state,
			"online_cpus", dss_log->cpuidle[cpu][idx].num_online_cpus,
			"en", dss_log->cpuidle[cpu][idx].en,
			(dss_log->cpuidle[cpu][idx].en == 1) ? "[Missmatch]" : "");
}

static void dbg_snapshot_print_lastinfo(void)
{
	int cpu;

	pr_info("<last info>\n");
	for (cpu = 0; cpu < DSS_NR_CPUS; cpu++) {
		pr_info("CPU ID: %d -----------------------------------------------\n", cpu);
		dbg_snapshot_print_last_task(cpu);
		dbg_snapshot_print_last_work(cpu);
		dbg_snapshot_print_last_irq(cpu);
		dbg_snapshot_print_last_cpuidle(cpu);
	}
}

#ifdef CONFIG_DEBUG_SNAPSHOT_REGULATOR
void dbg_snapshot_regulator(unsigned long long timestamp, char* f_name, unsigned int addr, unsigned int volt, unsigned int rvolt, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.regulator_log_idx) &
				(ARRAY_SIZE(dss_log->regulator) - 1);
		int size = strlen(f_name);
		if (size >= SZ_16)
			size = SZ_16 - 1;
		dss_log->regulator[i].time = cpu_clock(cpu);
		dss_log->regulator[i].cpu = cpu;
		dss_log->regulator[i].acpm_time = timestamp;
		strncpy(dss_log->regulator[i].name, f_name, size);
		dss_log->regulator[i].reg = addr;
		dss_log->regulator[i].en = en;
		dss_log->regulator[i].voltage = volt;
		dss_log->regulator[i].raw_volt = rvolt;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_THERMAL
void dbg_snapshot_thermal(void *data, unsigned int temp, char *name, unsigned int max_cooling)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.thermal_log_idx) &
				(ARRAY_SIZE(dss_log->thermal) - 1);

		dss_log->thermal[i].time = cpu_clock(cpu);
		dss_log->thermal[i].cpu = cpu;
		dss_log->thermal[i].data = (struct exynos_tmu_platform_data *)data;
		dss_log->thermal[i].temp = temp;
		dss_log->thermal[i].cooling_device = name;
		dss_log->thermal[i].cooling_state = max_cooling;
	}
}
#endif

void dbg_snapshot_irq(int irq, void *fn, void *val, unsigned long long start_time, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];
	unsigned long flags;

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;

	flags = pure_arch_local_irq_save();
	{
		int cpu = raw_smp_processor_id();
		unsigned long long time, latency;
		unsigned long i;

		time = cpu_clock(cpu);

		if (start_time == 0)
			start_time = time;

		latency = time - start_time;
		i = atomic_inc_return(&dss_idx.irq_log_idx[cpu]) &
				(ARRAY_SIZE(dss_log->irq[0]) - 1);

		dss_log->irq[cpu][i].time = time;
		dss_log->irq[cpu][i].sp = (unsigned long) current_stack_pointer;
		dss_log->irq[cpu][i].irq = irq;
		dss_log->irq[cpu][i].fn = (void *)fn;
		dss_log->irq[cpu][i].desc = (struct irq_desc *)val;
		dss_log->irq[cpu][i].latency = latency;
		dss_log->irq[cpu][i].en = en;
	}
	pure_arch_local_irq_restore(flags);
}

#ifdef CONFIG_DEBUG_SNAPSHOT_SPINLOCK
void dbg_snapshot_spinlock(void *v_lock, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned index = atomic_inc_return(&dss_idx.spinlock_log_idx[cpu]);
		unsigned long j, i = index & (ARRAY_SIZE(dss_log->spinlock[0]) - 1);
		raw_spinlock_t *lock = (raw_spinlock_t *)v_lock;
#ifdef CONFIG_ARM_ARCH_TIMER
		dss_log->spinlock[cpu][i].time = cpu_clock(cpu);
#else
		dss_log->spinlock[cpu][i].time = index;
#endif
		dss_log->spinlock[cpu][i].sp = (unsigned long) current_stack_pointer;
		dss_log->spinlock[cpu][i].jiffies = jiffies_64;
#ifdef CONFIG_DEBUG_SPINLOCK
		dss_log->spinlock[cpu][i].lock = lock;
		dss_log->spinlock[cpu][i].next = lock->raw_lock.next;
		dss_log->spinlock[cpu][i].owner = lock->raw_lock.owner;
#endif
		dss_log->spinlock[cpu][i].en = en;

		for (j = 0; j < dss_desc.callstack; j++) {
			dss_log->spinlock[cpu][i].caller[j] =
				(void *)((size_t)return_address(j + 1));
		}
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_IRQ_DISABLED
void dbg_snapshot_irqs_disabled(unsigned long flags)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];
	int cpu = raw_smp_processor_id();

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;

	if (unlikely(flags)) {
		unsigned j, local_flags = pure_arch_local_irq_save();

		/* If flags has one, it shows interrupt enable status */
		atomic_set(&dss_idx.irqs_disabled_log_idx[cpu], -1);
		dss_log->irqs_disabled[cpu][0].time = 0;
		dss_log->irqs_disabled[cpu][0].index = 0;
		dss_log->irqs_disabled[cpu][0].task = NULL;
		dss_log->irqs_disabled[cpu][0].task_comm = NULL;

		for (j = 0; j < dss_desc.callstack; j++) {
			dss_log->irqs_disabled[cpu][0].caller[j] = NULL;
		}

		pure_arch_local_irq_restore(local_flags);
	} else {
		unsigned index = atomic_inc_return(&dss_idx.irqs_disabled_log_idx[cpu]);
		unsigned long j, i = index % ARRAY_SIZE(dss_log->irqs_disabled[0]);

		dss_log->irqs_disabled[cpu][0].time = jiffies_64;
		dss_log->irqs_disabled[cpu][i].index = index;
		dss_log->irqs_disabled[cpu][i].task = get_current();
		dss_log->irqs_disabled[cpu][i].task_comm = get_current()->comm;

		for (j = 0; j < dss_desc.callstack; j++) {
			dss_log->irqs_disabled[cpu][i].caller[j] =
				(void *)((size_t)return_address(j + 1));
		}
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_CLK
void dbg_snapshot_clk(void *clock, const char *func_name, unsigned long arg, int mode)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.clk_log_idx) &
				(ARRAY_SIZE(dss_log->clk) - 1);

		dss_log->clk[i].time = cpu_clock(cpu);
		dss_log->clk[i].mode = mode;
		dss_log->clk[i].arg = arg;
		dss_log->clk[i].clk = (struct clk_hw *)clock;
		dss_log->clk[i].f_name = func_name;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_PMU
void dbg_snapshot_pmu(int id, const char *func_name, int mode)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.pmu_log_idx) &
				(ARRAY_SIZE(dss_log->pmu) - 1);

		dss_log->pmu[i].time = cpu_clock(cpu);
		dss_log->pmu[i].mode = mode;
		dss_log->pmu[i].id = id;
		dss_log->pmu[i].f_name = func_name;
	}
}
#endif

static struct notifier_block **dss_should_check_nl[] = {
	(struct notifier_block **)(&panic_notifier_list.head),
	(struct notifier_block **)(&reboot_notifier_list.head),
	(struct notifier_block **)(&restart_handler_list.head),
#ifdef CONFIG_PM_SLEEP
	(struct notifier_block **)(&pm_chain_head.head),
#endif
#ifdef CONFIG_EXYNOS_ITMON
	(struct notifier_block **)(&itmon_notifier_list.head),
#endif
};

void dbg_snapshot_print_notifier_call(void **nl, unsigned long func, int en)
{
	struct notifier_block **nl_org = (struct notifier_block **)nl;
	char notifier_name[KSYM_NAME_LEN];
	char notifier_func_name[KSYM_NAME_LEN];
	int i;

	for (i = 0; i < ARRAY_SIZE(dss_should_check_nl); i++) {
		if (nl_org == dss_should_check_nl[i]) {
			lookup_symbol_name((unsigned long)nl_org, notifier_name);
			lookup_symbol_name((unsigned long)func, notifier_func_name);

			pr_info("debug-snapshot: %s -> %s call %s\n",
				notifier_name,
				notifier_func_name,
				en == DSS_FLAG_IN ? "+" : "-");
			break;
		}
	}
}

#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
static void dbg_snapshot_print_freqinfo(void)
{
	unsigned long idx, sec, msec;
	char *freq_name;
	unsigned int i;
	unsigned long old_freq, target_freq;

	pr_info("\n<freq info>\n");

	for (i = 0; i < DSS_FLAG_END; i++) {
		idx = atomic_read(&dss_lastinfo.freq_last_idx[i]) & (ARRAY_SIZE(dss_log->freq) - 1);
		freq_name = dss_log->freq[idx].freq_name;
		if ((!freq_name) || strncmp(freq_name, dss_freq_name[i], strlen(dss_freq_name[i]))) {
			pr_info("%10s: no infomation\n", dss_freq_name[i]);
			continue;
		}

		dbg_snapshot_get_sec(dss_log->freq[idx].time, &sec, &msec);
		old_freq = dss_log->freq[idx].old_freq;
		target_freq = dss_log->freq[idx].target_freq;
		pr_info("%10s: [%4lu] %10lu.%06lu sec, %12s: %6luMhz, %12s: %6luMhz, %3s: %3d %s\n",
					freq_name, idx, sec, msec,
					"old_freq", old_freq/1000,
					"target_freq", target_freq/1000,
					"en", dss_log->freq[idx].en,
					(dss_log->freq[idx].en == 1) ? "[Missmatch]" : "");
	}
}

void dbg_snapshot_freq(int type, unsigned long old_freq, unsigned long target_freq, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.freq_log_idx) &
				(ARRAY_SIZE(dss_log->freq) - 1);

		if (atomic_read(&dss_idx.freq_log_idx) > atomic_read(&dss_lastinfo.freq_last_idx[type]))
			atomic_set(&dss_lastinfo.freq_last_idx[type], atomic_read(&dss_idx.freq_log_idx));

		dss_log->freq[i].time = cpu_clock(cpu);
		dss_log->freq[i].cpu = cpu;
		dss_log->freq[i].freq_name = dss_freq_name[type];
		dss_log->freq[i].freq_type = type;
		dss_log->freq[i].old_freq = old_freq;
		dss_log->freq[i].target_freq = target_freq;
		dss_log->freq[i].en = en;
	}
}

void dbg_snapshot_freq_misc(int type, unsigned long old_freq, unsigned long target_freq, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.freq_misc_log_idx) &
				(ARRAY_SIZE(dss_log->freq_misc) - 1);

		dss_log->freq_misc[i].time = cpu_clock(cpu);
		dss_log->freq_misc[i].cpu = cpu;
		dss_log->freq_misc[i].freq_name = dss_freq_name[type];
		dss_log->freq_misc[i].freq_type = type;
		dss_log->freq_misc[i].old_freq = old_freq;
		dss_log->freq_misc[i].target_freq = target_freq;
		dss_log->freq_misc[i].en = en;
	}
}

#endif

#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif

static void dbg_snapshot_print_irq(void)
{
	int i, j;
	u64 sum = 0;

	for_each_possible_cpu(i) {
		sum += kstat_cpu_irqs_sum(i);
		sum += arch_irq_stat_cpu(i);
	}
	sum += arch_irq_stat();

	pr_info("\n<irq info>\n");
	pr_info("------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("sum irq : %llu", (unsigned long long)sum);
	pr_info("------------------------------------------------------------------\n");

	for_each_irq_nr(j) {
		unsigned int irq_stat = kstat_irqs(j);

		if (irq_stat) {
			struct irq_desc *desc = irq_to_desc(j);
			const char *name;

			name = desc->action ? (desc->action->name ? desc->action->name : "???") : "???";
			pr_info("irq-%-4d(hwirq-%-4d) : %8u %s\n",
				j, (int)desc->irq_data.hwirq, irq_stat, name);
		}
	}
}

void dbg_snapshot_print_panic_report(void)
{
	pr_info("============================================================\n");
	pr_info("Panic Report\n");
	pr_info("============================================================\n");
	dbg_snapshot_print_lastinfo();
#ifdef CONFIG_DEBUG_SNAPSHOT_FREQ
	dbg_snapshot_print_freqinfo();
#endif
	dbg_snapshot_print_calltrace();
	dbg_snapshot_print_irq();
	pr_info("============================================================\n");
}

#ifdef CONFIG_DEBUG_SNAPSHOT_DM
void dbg_snapshot_dm(int type, unsigned long min, unsigned long max, s32 wait_t, s32 t)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.dm_log_idx) &
				(ARRAY_SIZE(dss_log->dm) - 1);

		dss_log->dm[i].time = cpu_clock(cpu);
		dss_log->dm[i].cpu = cpu;
		dss_log->dm[i].dm_num = type;
		dss_log->dm[i].min_freq = min;
		dss_log->dm[i].max_freq = max;
		dss_log->dm[i].wait_dmt = wait_t;
		dss_log->dm[i].do_dmt = t;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_HRTIMER
void dbg_snapshot_hrtimer(void *timer, s64 *now, void *fn, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.hrtimer_log_idx[cpu]) &
				(ARRAY_SIZE(dss_log->hrtimers[0]) - 1);

		dss_log->hrtimers[cpu][i].time = cpu_clock(cpu);
		dss_log->hrtimers[cpu][i].now = *now;
		dss_log->hrtimers[cpu][i].timer = (struct hrtimer *)timer;
		dss_log->hrtimers[cpu][i].fn = fn;
		dss_log->hrtimers[cpu][i].en = en;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_I2C
void dbg_snapshot_i2c(struct i2c_adapter *adap, struct i2c_msg *msgs, int num, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.i2c_log_idx) &
				(ARRAY_SIZE(dss_log->i2c) - 1);

		dss_log->i2c[i].time = cpu_clock(cpu);
		dss_log->i2c[i].cpu = cpu;
		dss_log->i2c[i].adap = adap;
		dss_log->i2c[i].msgs = msgs;
		dss_log->i2c[i].num = num;
		dss_log->i2c[i].en = en;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_SPI
void dbg_snapshot_spi(struct spi_controller *ctlr, struct spi_message *cur_msg, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.spi_log_idx) &
				(ARRAY_SIZE(dss_log->spi) - 1);

		dss_log->spi[i].time = cpu_clock(cpu);
		dss_log->spi[i].cpu = cpu;
		dss_log->spi[i].ctlr = ctlr;
		dss_log->spi[i].cur_msg = cur_msg;
		dss_log->spi[i].en = en;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_BINDER
void dbg_snapshot_binder(struct trace_binder_transaction_base *base,
			 struct trace_binder_transaction *transaction,
			 struct trace_binder_transaction_error *error)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];
	int cpu;
	unsigned long i;

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	if (base == NULL)
		return;

	cpu = raw_smp_processor_id();
	i = atomic_inc_return(&dss_idx.binder_log_idx) &
				(ARRAY_SIZE(dss_log->binder) - 1);

	dss_log->binder[i].time = cpu_clock(cpu);
	dss_log->binder[i].cpu = cpu;
	dss_log->binder[i].base = *base;

	if (transaction) {
		dss_log->binder[i].transaction = *transaction;
	} else {
		dss_log->binder[i].transaction.to_node_id = 0;
		dss_log->binder[i].transaction.reply = 0;
		dss_log->binder[i].transaction.flags = 0;
		dss_log->binder[i].transaction.code = 0;
	}
	if (error) {
		dss_log->binder[i].error = *error;
	} else {
		dss_log->binder[i].error.return_error = 0;
		dss_log->binder[i].error.return_error_param = 0;
		dss_log->binder[i].error.return_error_line = 0;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_ACPM
void dbg_snapshot_acpm(unsigned long long timestamp, const char *log, unsigned int data)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long i = atomic_inc_return(&dss_idx.acpm_log_idx) &
				(ARRAY_SIZE(dss_log->acpm) - 1);
		int len = strlen(log);

		if (len >= 8)
			len = 8;

		dss_log->acpm[i].time = cpu_clock(cpu);
		dss_log->acpm[i].acpm_time = timestamp;
		strncpy(dss_log->acpm[i].log, log, len);
		dss_log->acpm[i].log[len] = '\0';
		dss_log->acpm[i].data = data;
	}
}
#endif

#ifdef CONFIG_DEBUG_SNAPSHOT_REG
static phys_addr_t virt_to_phys_high(size_t vaddr)
{
	phys_addr_t paddr = 0;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	if (virt_addr_valid((void *) vaddr)) {
		paddr = virt_to_phys((void *) vaddr);
		goto out;
	}

	pgd = pgd_offset_k(vaddr);
	if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
		goto out;

	if (pgd_val(*pgd) & 2) {
		paddr = pgd_val(*pgd) & SECTION_MASK;
		goto out;
	}

	pmd = pmd_offset((pud_t *)pgd, vaddr);
	if (pmd_none_or_clear_bad(pmd))
		goto out;

	pte = pte_offset_kernel(pmd, vaddr);
	if (pte_none(*pte))
		goto out;

	paddr = pte_val(*pte) & PAGE_MASK;

out:
	return paddr | (vaddr & UL(SZ_4K - 1));
}

void dbg_snapshot_reg(unsigned int read, size_t val, size_t reg, int en)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];
	int cpu = raw_smp_processor_id();
	unsigned long i, j;
	size_t phys_reg, start_addr, end_addr;

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;

	if (dss_reg_exlist[0].addr == 0)
		return;

	phys_reg = virt_to_phys_high(reg);
	if (unlikely(!phys_reg))
		return;

	for (j = 0; j < ARRAY_SIZE(dss_reg_exlist); j++) {
		if (dss_reg_exlist[j].addr == 0)
			break;
		start_addr = dss_reg_exlist[j].addr;
		end_addr = start_addr + dss_reg_exlist[j].size;
		if (start_addr <= phys_reg && phys_reg <= end_addr)
			return;
	}

	i = atomic_inc_return(&dss_idx.reg_log_idx[cpu]) &
		(ARRAY_SIZE(dss_log->reg[0]) - 1);

	dss_log->reg[cpu][i].time = cpu_clock(cpu);
	dss_log->reg[cpu][i].read = read;
	dss_log->reg[cpu][i].val = val;
	dss_log->reg[cpu][i].reg = phys_reg;
	dss_log->reg[cpu][i].en = en;

	for (j = 0; j < dss_desc.callstack; j++) {
		dss_log->reg[cpu][i].caller[j] =
			(void *)((size_t)return_address(j + 1));
	}
}
#endif

#ifndef CONFIG_DEBUG_SNAPSHOT_MINIMIZED_MODE
void dbg_snapshot_printk(const char *fmt, ...)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		va_list args;
		int ret;
		unsigned long j, i = atomic_inc_return(&dss_idx.printk_log_idx) &
				(ARRAY_SIZE(dss_log->printk) - 1);

		va_start(args, fmt);
		ret = vsnprintf(dss_log->printk[i].log,
				sizeof(dss_log->printk[i].log), fmt, args);
		va_end(args);

		dss_log->printk[i].time = cpu_clock(cpu);
		dss_log->printk[i].cpu = cpu;

		for (j = 0; j < dss_desc.callstack; j++) {
			dss_log->printk[i].caller[j] =
				(void *)((size_t)return_address(j));
		}
	}
}

void dbg_snapshot_printkl(size_t msg, size_t val)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return;
	{
		int cpu = raw_smp_processor_id();
		unsigned long j, i = atomic_inc_return(&dss_idx.printkl_log_idx) &
				(ARRAY_SIZE(dss_log->printkl) - 1);

		dss_log->printkl[i].time = cpu_clock(cpu);
		dss_log->printkl[i].cpu = cpu;
		dss_log->printkl[i].msg = msg;
		dss_log->printkl[i].val = val;

		for (j = 0; j < dss_desc.callstack; j++) {
			dss_log->printkl[i].caller[j] =
				(void *)((size_t)return_address(j));
		}
	}
}
#endif

#ifdef CONFIG_SEC_PM_DEBUG
static ssize_t dss_log_work_lookup(char *buf, ssize_t n, int cpu, int idx)
{
	char work_fn[KSYM_NAME_LEN];
	unsigned long sec, msec;
	u64 ts;
	int en;

	if (!(dss_log->work[cpu][idx].fn))
		return n;

	lookup_symbol_name((unsigned long)dss_log->work[cpu][idx].fn, work_fn);

	ts = dss_log->work[cpu][idx].time;
	sec = ts / NSEC_PER_SEC;
	msec = (ts % NSEC_PER_SEC) / USEC_PER_MSEC;

	en = dss_log->work[cpu][idx].en;

	n += scnprintf(buf + n, 100,
			"%d: %10lu.%06lu task:%16s, fn:%32s, %1s\n",
			cpu, sec, msec, dss_log->work[cpu][idx].task_comm,
			work_fn, en == DSS_FLAG_IN ? "I" : "O");

	return n;
}

ssize_t dss_log_work_print(char *buf)
{
	int cpu, array_size;
	ssize_t n = 0;

	if (!dss_log)
		return 0;

	array_size = ARRAY_SIZE(dss_log->work[0]) - 1;

	for_each_possible_cpu(cpu) {
		int i, idx;

		idx = atomic_read(&dss_idx.work_log_idx[cpu]);

		for (i = 0; i < 5 && i < array_size; i++, idx--) {
			idx &= array_size;
			n = dss_log_work_lookup(buf, n, cpu, idx);
		}
	}

	return n;
}
#endif /* CONFIG_SEC_PM_DEBUG */

#if defined(CONFIG_DEBUG_SNAPSHOT_THERMAL) && defined(CONFIG_SEC_PM_DEBUG)
#include <linux/debugfs.h>

static int exynos_ss_thermal_show(struct seq_file *m, void *unused)
{
	struct dbg_snapshot_item *item = &dss_items[dss_desc.kevents_num];
	unsigned long idx, size;
	unsigned long rem_nsec;
	u64 ts;
	int i;

	if (unlikely(!dss_base.enabled || !item->entry.enabled))
		return 0;

	seq_puts(m, "time\t\t\ttemperature\tcooling_device\t\tmax_frequency\n");

	size = ARRAY_SIZE(dss_log->thermal);
	idx = atomic_read(&dss_idx.thermal_log_idx);

	for (i = 0; i < size; i++, idx--) {
		idx &= size - 1;

		ts = dss_log->thermal[idx].time;
		if (!ts)
			break;

		rem_nsec = do_div(ts, NSEC_PER_SEC);

		seq_printf(m, "[%8lu.%06lu]\t%u\t\t%-16s\t%u\n",
				(unsigned long)ts, rem_nsec / NSEC_PER_USEC,
				dss_log->thermal[idx].temp,
				dss_log->thermal[idx].cooling_device,
				dss_log->thermal[idx].cooling_state);
	}

	return 0;
}

static int exynos_ss_thermal_open(struct inode *inode, struct file *file)
{
	return single_open(file, exynos_ss_thermal_show, NULL);
}

static const struct file_operations thermal_fops = {
	.owner = THIS_MODULE,
	.open = exynos_ss_thermal_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct dentry *debugfs_ess_root;

static int __init exynos_ss_debugfs_init(void)
{
	debugfs_ess_root = debugfs_create_dir("exynos-ss", NULL);
	if (!debugfs_ess_root) {
		pr_err("Failed to create exynos-ss debugfs\n");
		return 0;
	}

	debugfs_create_file("thermal", 0444, debugfs_ess_root, NULL,
			&thermal_fops);

	return 0;
}

late_initcall(exynos_ss_debugfs_init);
#endif /* CONFIG_DEBUG_SNAPSHOT_THERMAL && CONFIG_SEC_PM_DEBUG */

#if defined(CONFIG_HARDLOCKUP_DETECTOR_OTHER_CPU)			\
	&& defined(CONFIG_SEC_DEBUG)
#define for_each_generated_irq_in_snapshot(idx, i, max, base, cpu)							\
	for (i = 0, idx = base; i < max; ++i, idx = (base - i) & (ARRAY_SIZE(dss_log->irq[0]) - 1))		\
		if (dss_log->irq[cpu][idx].en == DSS_FLAG_IN)

static inline void dbg_snapshot_get_busiest_irq(struct hardlockup_info *hl_info, unsigned long start_idx, int cpu)
{
	#define MAX_BUF 5
	int i, j, idx, max_count = 20;
	int buf_count = 0;
	int max_irq_idx = 0;

	struct irq_info_buf {
		unsigned int occurrences;
		int irq;
		void *fn;
		unsigned long long total_duration;
		unsigned long long last_time;
	};

	struct irq_info_buf i_buf[MAX_BUF] = {{0,},};

	for_each_generated_irq_in_snapshot(idx, i, max_count, start_idx, cpu) {
		for (j = 0; j < buf_count; j++) {
			if (i_buf[j].irq == dss_log->irq[cpu][idx].irq) {
				i_buf[j].total_duration += (i_buf[j].last_time - dss_log->irq[cpu][idx].time);
				i_buf[j].last_time = dss_log->irq[cpu][idx].time;
				i_buf[j].occurrences++;
				break;
			}
		}

		if (j == buf_count && buf_count < MAX_BUF) {
			i_buf[buf_count].irq = dss_log->irq[cpu][idx].irq;
			i_buf[buf_count].fn = dss_log->irq[cpu][idx].fn;
			i_buf[buf_count].occurrences = 0;
			i_buf[buf_count].total_duration = 0;
			i_buf[buf_count].last_time = dss_log->irq[cpu][idx].time;
			buf_count++;
		} else if (buf_count == MAX_BUF) {
			pr_info("Buffer overflow. Various irqs were generated!!\n");
		}
	}

	for (i = 1; i < buf_count; i++) {
		if (i_buf[max_irq_idx].occurrences < i_buf[i].occurrences)
			max_irq_idx = i;
	}

	hl_info->irq_info.irq = i_buf[max_irq_idx].irq;
	hl_info->irq_info.fn = i_buf[max_irq_idx].fn;
	hl_info->irq_info.avg_period = i_buf[max_irq_idx].total_duration / i_buf[max_irq_idx].occurrences;
}

void dbg_snapshot_get_hardlockup_info(unsigned int cpu,  void *info)
{
	struct hardlockup_info *hl_info = info;
	unsigned long cpuidle_idx, irq_idx, task_idx;
	unsigned long long cpuidle_delay_time, irq_delay_time, task_delay_time;
	unsigned long long curr, thresh;

	thresh = get_hardlockup_thresh();
	curr = local_clock();

	cpuidle_idx = atomic_read(&dss_idx.cpuidle_log_idx[cpu]) & (ARRAY_SIZE(dss_log->cpuidle[0]) - 1);
	cpuidle_delay_time = curr - dss_log->cpuidle[cpu][cpuidle_idx].time;

	if (dss_log->cpuidle[cpu][cpuidle_idx].en == DSS_FLAG_IN
		&& cpuidle_delay_time > thresh) {
		hl_info->delay_time = cpuidle_delay_time;
		hl_info->cpuidle_info.mode = dss_log->cpuidle[cpu][cpuidle_idx].modes;
		hl_info->hl_type = HL_IDLE_STUCK;
		return;
	}

	irq_idx = atomic_read(&dss_idx.irq_log_idx[cpu]) & (ARRAY_SIZE(dss_log->irq[0]) - 1);
	irq_delay_time = curr - dss_log->irq[cpu][irq_idx].time;

	if (dss_log->irq[cpu][irq_idx].en == DSS_FLAG_IN
		&& irq_delay_time > thresh) {

		hl_info->delay_time = irq_delay_time;

		if (dss_log->irq[cpu][irq_idx].irq < 0) {				// smc calls have negative irq number
			hl_info->smc_info.cmd = dss_log->irq[cpu][irq_idx].irq;
			hl_info->hl_type = HL_SMC_CALL_STUCK;
			return;
		} else {
			hl_info->irq_info.irq = dss_log->irq[cpu][irq_idx].irq;
			hl_info->irq_info.fn = dss_log->irq[cpu][irq_idx].fn;
			hl_info->hl_type = HL_IRQ_STUCK;
			return;
		}
	}

	task_idx = atomic_read(&dss_idx.task_log_idx[cpu]) & (ARRAY_SIZE(dss_log->task[0]) - 1);
	task_delay_time = curr - dss_log->task[cpu][task_idx].time;

	if (task_delay_time > thresh) {
		hl_info->delay_time = task_delay_time;
		if (irq_delay_time > thresh) {
			strncpy(hl_info->task_info.task_comm,
				dss_log->task[cpu][task_idx].task_comm,
				TASK_COMM_LEN - 1);
			hl_info->task_info.task_comm[TASK_COMM_LEN - 1] = '\0';
			hl_info->hl_type = HL_TASK_STUCK;
			return;
		} else {
			dbg_snapshot_get_busiest_irq(hl_info, irq_idx, cpu);
			hl_info->hl_type = HL_IRQ_STORM;
			return;
		}
	}

	hl_info->hl_type = HL_UNKNOWN_STUCK;
}

void dbg_snapshot_get_softlockup_info(unsigned int cpu, void *info)
{
	struct softlockup_info *sl_info = info;
	unsigned long task_idx;
	unsigned long long task_delay_time;
	unsigned long long curr, thresh;

	thresh = get_dss_softlockup_thresh();
	curr = local_clock();
	task_idx = atomic_read(&dss_idx.task_log_idx[cpu]) & (ARRAY_SIZE(dss_log->task[0]) - 1);
	task_delay_time = curr - dss_log->task[cpu][task_idx].time;
	sl_info->delay_time = task_delay_time;

	strncpy(sl_info->task_info.task_comm,
		dss_log->task[cpu][task_idx].task_comm,
		TASK_COMM_LEN - 1);
	sl_info->task_info.task_comm[TASK_COMM_LEN - 1] = '\0';

	if (task_delay_time > thresh)
		sl_info->sl_type = SL_TASK_STUCK;
	else
		sl_info->sl_type = SL_UNKNOWN_STUCK;
}
#endif
