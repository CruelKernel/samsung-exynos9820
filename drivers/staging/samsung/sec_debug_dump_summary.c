/*
 * Copyright (c) 2014-2018 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung System Debugging Code - Dump Summary
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sec_debug.h>
#include <linux/of_reserved_mem.h>
#include <linux/cpufreq.h>
#include <linux/sched/clock.h>
#include <linux/io.h>

struct sec_debug_summary *summary_info;
static char *sec_summary_log_buf;
static unsigned long sec_summary_log_size;
static unsigned long reserved_out_buf;
static unsigned long reserved_out_size;
static char *last_summary_buffer;
static size_t last_summary_size;

#define ARM_PT_REG_PC pc
#define ARM_PT_REG_LR regs[30]

void __sec_debug_task_sched_log(int cpu, struct task_struct *task, char *msg)
{
	unsigned long i;

	if (!summary_info)
		return;

	if (!task && !msg)
		return;

	i = atomic_inc_return(&summary_info->sched_log.idx_sched[cpu]) & (SCHED_LOG_MAX - 1);
	summary_info->sched_log.sched[cpu][i].time = cpu_clock(cpu);
	if (task) {
		strlcpy(summary_info->sched_log.sched[cpu][i].comm, task->comm,
			sizeof(summary_info->sched_log.sched[cpu][i].comm));
		summary_info->sched_log.sched[cpu][i].pid = task->pid;
		summary_info->sched_log.sched[cpu][i].pTask = task;
	} else {
		strlcpy(summary_info->sched_log.sched[cpu][i].comm, msg,
			sizeof(summary_info->sched_log.sched[cpu][i].comm));
		summary_info->sched_log.sched[cpu][i].pid = -1;
		summary_info->sched_log.sched[cpu][i].pTask = NULL;
	}
}

void sec_debug_task_sched_log_short_msg(char *msg)
{
	__sec_debug_task_sched_log(raw_smp_processor_id(), NULL, msg);
}

void sec_debug_task_sched_log(int cpu, struct task_struct *task)
{
	__sec_debug_task_sched_log(cpu, task, NULL);
}

void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en)
{
	int cpu = smp_processor_id();
	unsigned long i;

	if (!summary_info)
		return;

	i = atomic_inc_return(&summary_info->sched_log.idx_irq[cpu]) & (SCHED_LOG_MAX - 1);
	summary_info->sched_log.irq[cpu][i].time = cpu_clock(cpu);
	summary_info->sched_log.irq[cpu][i].irq = irq;
	summary_info->sched_log.irq[cpu][i].fn = (void *)fn;
	summary_info->sched_log.irq[cpu][i].en = en;
	summary_info->sched_log.irq[cpu][i].preempt_count = preempt_count();
	summary_info->sched_log.irq[cpu][i].context = &cpu;
}

void sec_debug_irq_enterexit_log(unsigned int irq, unsigned long long start_time)
{
	int cpu = smp_processor_id();
	unsigned long i;

	if (!summary_info)
		return;

	i = atomic_inc_return(&summary_info->sched_log.idx_irq_exit[cpu]) & (SCHED_LOG_MAX - 1);
	summary_info->sched_log.irq_exit[cpu][i].time = start_time;
	summary_info->sched_log.irq_exit[cpu][i].end_time = cpu_clock(cpu);
	summary_info->sched_log.irq_exit[cpu][i].irq = irq;
	summary_info->sched_log.irq_exit[cpu][i].elapsed_time =
		summary_info->sched_log.irq_exit[cpu][i].end_time - start_time;
}

void sec_debug_summary_set_reserved_out_buf(unsigned long buf, unsigned long size)
{
	reserved_out_buf = buf;
	reserved_out_size = size;
}

int sec_debug_save_cpu_info(void)
{
	struct cpufreq_policy *policy;
	int i;

	for_each_possible_cpu(i) {
		policy = cpufreq_cpu_get_raw(i);
		if (policy) {
			strcpy(summary_info->kernel.cpu_info[i].policy_name, policy->governor->name);
			summary_info->kernel.cpu_info[i].freq_min = policy->min;
			summary_info->kernel.cpu_info[i].freq_max = policy->max;
			summary_info->kernel.cpu_info[i].freq_cur = policy->cur;
		} else {
			summary_info->kernel.cpu_info[i].freq_min = -1;
			summary_info->kernel.cpu_info[i].freq_max = -1;
			summary_info->kernel.cpu_info[i].freq_cur = -1;
		}
	}

	return 0;
}

int sec_debug_save_die_info(const char *str, struct pt_regs *regs)
{
	if (!sec_summary_log_buf || !summary_info || !regs)
		return 0;
	snprintf(summary_info->kernel.excp.pc_sym, sizeof(summary_info->kernel.excp.pc_sym),
			"%pS", (void *)regs->ARM_PT_REG_PC);
	snprintf(summary_info->kernel.excp.lr_sym, sizeof(summary_info->kernel.excp.lr_sym),
			"%pS", (void *)regs->ARM_PT_REG_LR);

	return 0;
}

int sec_debug_save_panic_info(const char *str, unsigned long caller)
{
	if (!sec_summary_log_buf || !summary_info)
		return 0;
	snprintf(summary_info->kernel.excp.panic_caller, sizeof(summary_info->kernel.excp.panic_caller),
			"%pS", (void *)caller);
	snprintf(summary_info->kernel.excp.panic_msg, sizeof(summary_info->kernel.excp.panic_msg),
			"%s", str);
	snprintf(summary_info->kernel.excp.thread, sizeof(summary_info->kernel.excp.thread),
			"%s:%d", current->comm, task_pid_nr(current));

	return 0;
}

static int __init sec_debug_dump_summary_setup(char *str)
{
	unsigned long size = memparse(str, &str);
	unsigned long base = 0;

	/* If we encounter any problem parsing str ... */
	if (!size || *str != '@' || kstrtoul(str + 1, 0, &base)) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

	last_summary_size = size;

#ifdef CONFIG_NO_BOOTMEM
	if (memblock_is_region_reserved(base, size) || memblock_reserve(base, size)) {
#else
	if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
#endif
		pr_err("%s: failed to reserve size:0x%lx at base 0x%lx\n", __func__, size, base);
		goto out;
	}

	sec_summary_log_buf = phys_to_virt(base);
	sec_summary_log_size = round_up(sizeof(struct sec_debug_summary), PAGE_SIZE);
	last_summary_buffer = phys_to_virt(base + sec_summary_log_size);
	sec_debug_summary_set_reserved_out_buf(base + sec_summary_log_size,
						(size - sec_summary_log_size));

	pr_info("%s: base:0x%lx size:0x%lx\n", __func__, base, size);
	pr_info("%s: rsvbuf:0x%lx size:0x%lx\n", __func__, reserved_out_buf, reserved_out_size);

out:
	return 0;
}
__setup("sec_summary_log=", sec_debug_dump_summary_setup);

int sec_debug_dump_summary_init(void)
{
	int offset = 0;
	int i;

	pr_info("%s: start\n", __func__, offset);

	if (!sec_summary_log_buf) {
		pr_info("%s: no summary buffer\n", __func__);

		return 0;
	}

	summary_info = (struct sec_debug_summary *)sec_summary_log_buf;
	memset(summary_info, 0, DUMP_SUMMARY_MAX_SIZE);

	sec_debug_save_cpu_info();

	summary_info->kernel.nr_cpus = CONFIG_NR_CPUS;

	strcpy(summary_info->summary_cmdline, saved_command_line);
	strcpy(summary_info->summary_linuxbanner, linux_banner);

	summary_info->reserved_out_buf = reserved_out_buf;
	summary_info->reserved_out_size = reserved_out_size;

	memset_io(summary_info->sched_log.sched, 0x0, sizeof(summary_info->sched_log.sched));
	memset_io(summary_info->sched_log.irq, 0x0, sizeof(summary_info->sched_log.irq));
	memset_io(summary_info->sched_log.irq_exit, 0x0, sizeof(summary_info->sched_log.irq_exit));

	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		atomic_set(&summary_info->sched_log.idx_sched[i], -1);
		atomic_set(&summary_info->sched_log.idx_irq[i], -1);
		atomic_set(&summary_info->sched_log.idx_irq_exit[i], -1);
	}

	summary_info->magic[0] = SEC_DEBUG_SUMMARY_MAGIC0;
	summary_info->magic[1] = SEC_DEBUG_SUMMARY_MAGIC1;
	summary_info->magic[2] = SEC_DEBUG_SUMMARY_MAGIC2;
	summary_info->magic[3] = SEC_DEBUG_SUMMARY_MAGIC3;

	sec_debug_set_kallsyms_info(&(summary_info->ksyms), SEC_DEBUG_SUMMARY_MAGIC1);

	pr_info("%s: done\n", __func__);

	return 0;
}
late_initcall(sec_debug_dump_summary_init);
