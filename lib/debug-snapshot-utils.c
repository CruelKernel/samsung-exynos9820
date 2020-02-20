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
#include <linux/io.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/input.h>
#include <linux/smc.h>
#include <linux/bitops.h>
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/nmi.h>
#include <linux/init_task.h>
#include <linux/ftrace.h>

#include <asm/cputype.h>
#include <asm/smp_plat.h>
#include <asm/core_regs.h>
#include <asm/cacheflush.h>
#include <linux/irqflags.h>

#include "debug-snapshot-local.h"

DEFINE_PER_CPU(struct pt_regs *, dss_core_reg);
DEFINE_PER_CPU(struct dbg_snapshot_mmu_reg *, dss_mmu_reg);

struct dbg_snapshot_allcorelockup_param {
	unsigned long last_pc_addr;
	unsigned long spin_pc_addr;
} dss_allcorelockup_param;

void dbg_snapshot_hook_hardlockup_entry(void *v_regs)
{
	int cpu = raw_smp_processor_id();
	unsigned int val;

	if (!dss_base.enabled)
		return;

	if (!dss_desc.hardlockup_core_mask) {
		if (dss_desc.multistage_wdt_irq &&
			!dss_desc.allcorelockup_detected) {
			/* 1st FIQ trigger */
			val = readl(dbg_snapshot_get_base_vaddr() +
				DSS_OFFSET_CORE_LAST_PC + (DSS_NR_CPUS * sizeof(unsigned long)));
			if (val == DSS_SIGN_LOCKUP || val == (DSS_SIGN_LOCKUP + 1)) {
				dss_desc.allcorelockup_detected = true;
				dss_desc.hardlockup_core_mask = GENMASK(DSS_NR_CPUS - 1, 0);
			} else {
				return;
			}
		}
	}

	/* re-check the cpu number which is lockup */
	if (dss_desc.hardlockup_core_mask & BIT(cpu)) {
		int ret;
		unsigned long last_pc;
		struct pt_regs *regs;
		unsigned long timeout = USEC_PER_SEC * 2;

		do {
			/*
			 * If one cpu is occurred to lockup,
			 * others are going to output its own information
			 * without side-effect.
			 */
			ret = do_raw_spin_trylock(&dss_desc.nmi_lock);
			if (!ret)
				udelay(1);
		} while (!ret && timeout--);

		last_pc = dbg_snapshot_get_last_pc(cpu);

		regs = (struct pt_regs *)v_regs;

		/* Replace real pc value even if it is invalid */
		regs->pc = last_pc;

		/* Then, we expect bug() function works well */
		pr_emerg("\n--------------------------------------------------------------------------\n"
			"%s - Debugging Information for Hardlockup core - CPU(%d), Mask:(0x%lx)"
			"\n--------------------------------------------------------------------------\n\n",
			(dss_desc.allcorelockup_detected) ? "All Core" : "Core",
			cpu, dss_desc.hardlockup_core_mask);

#if defined(CONFIG_HARDLOCKUP_DETECTOR_OTHER_CPU)			\
	&& defined(CONFIG_SEC_DEBUG)
		update_hardlockup_type(cpu);
#endif

#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
		sec_debug_set_extra_info_backtrace_cpu(v_regs, cpu);
#endif
	}
}

void dbg_snapshot_hook_hardlockup_exit(void)
{
	int cpu = raw_smp_processor_id();

	if (!dss_base.enabled ||
		!dss_desc.hardlockup_core_mask) {
		return;
	}

	/* re-check the cpu number which is lockup */
	if (dss_desc.hardlockup_core_mask & BIT(cpu)) {
		/* clear bit to complete replace */
		dss_desc.hardlockup_core_mask &= ~(BIT(cpu));
		/*
		 * If this unlock function does not make a side-effect
		 * even it's not lock
		 */
		do_raw_spin_unlock(&dss_desc.nmi_lock);
	}
}

void dbg_snapshot_recall_hardlockup_core(void)
{
	int i;
#ifdef SMC_CMD_KERNEL_PANIC_NOTICE
	int ret;
#endif
	unsigned long cpu_mask = 0, tmp_bit = 0;
	unsigned long last_pc_addr = 0, timeout;

	if (dss_desc.allcorelockup_detected) {
		pr_emerg("debug-snapshot: skip recall hardlockup for dump of each core\n");
		goto out;
	}

	for (i = 0; i < DSS_NR_CPUS; i++) {
		if (i == raw_smp_processor_id())
			continue;
		tmp_bit = cpu_online_mask->bits[DSS_NR_CPUS/SZ_64] & (1 << i);
		if (tmp_bit)
			cpu_mask |= tmp_bit;
	}

	if (!cpu_mask)
		goto out;

	last_pc_addr = dbg_snapshot_get_last_pc_paddr();

	pr_emerg("debug-snapshot: core hardlockup mask information: 0x%lx\n", cpu_mask);
	dss_desc.hardlockup_core_mask = cpu_mask;

#ifdef SMC_CMD_KERNEL_PANIC_NOTICE
	/* Setup for generating NMI interrupt to unstopped CPUs */
	ret = dss_soc_ops->soc_smc_call(SMC_CMD_KERNEL_PANIC_NOTICE,
				 cpu_mask,
				 (unsigned long)dbg_snapshot_bug_func,
				 last_pc_addr);
	if (ret) {
		pr_emerg("debug-snapshot: failed to generate NMI, "
			 "not support to dump information of core\n");
		dss_desc.hardlockup_core_mask = 0;
		goto out;
	}
#endif
	/* Wait up to 3 seconds for NMI interrupt */
	timeout = USEC_PER_SEC * 3;
	while (dss_desc.hardlockup_core_mask != 0 && timeout--)
		udelay(1);
out:
	return;
}

void dbg_snapshot_save_system(void *unused)
{
	struct dbg_snapshot_mmu_reg *mmu_reg;

	if (!dbg_snapshot_get_enable("header"))
		return;

	mmu_reg = per_cpu(dss_mmu_reg, raw_smp_processor_id());

	dss_soc_ops->soc_save_system((void *)mmu_reg);
}

int dbg_snapshot_dump(void)
{
	dss_soc_ops->soc_dump_info(NULL);
	return 0;
}
EXPORT_SYMBOL(dbg_snapshot_dump);

int dbg_snapshot_save_core(void *v_regs)
{
	struct pt_regs *regs = (struct pt_regs *)v_regs;
	struct pt_regs *core_reg =
			per_cpu(dss_core_reg, smp_processor_id());

	if(!dbg_snapshot_get_enable("header"))
		return 0;

	if (!regs)
		dss_soc_ops->soc_save_core((void *)core_reg);
	else
		memcpy(core_reg, regs, sizeof(struct user_pt_regs));

	pr_emerg("debug-snapshot: core register saved(CPU:%d)\n",
						smp_processor_id());
	return 0;
}
EXPORT_SYMBOL(dbg_snapshot_save_core);

int dbg_snapshot_save_context(void *v_regs)
{
	int cpu;
	unsigned long flags;
	struct pt_regs *regs = (struct pt_regs *)v_regs;

	if (unlikely(!dss_base.enabled))
		return 0;

	dss_soc_ops->soc_save_context_entry(NULL);

	cpu = smp_processor_id();
	raw_spin_lock_irqsave(&dss_desc.ctrl_lock, flags);

	/* If it was already saved the context information, it should be skipped */
	if (dbg_snapshot_get_core_panic_stat(cpu) !=  DSS_SIGN_PANIC) {
		dbg_snapshot_save_system(NULL);
		dbg_snapshot_save_core(regs);
		dbg_snapshot_dump();
		dbg_snapshot_set_core_panic_stat(DSS_SIGN_PANIC, cpu);
		pr_emerg("debug-snapshot: context saved(CPU:%d)\n", cpu);
	} else
		pr_emerg("debug-snapshot: skip context saved(CPU:%d)\n", cpu);

	raw_spin_unlock_irqrestore(&dss_desc.ctrl_lock, flags);

	dss_soc_ops->soc_save_context_exit(NULL);
	return 0;
}
EXPORT_SYMBOL(dbg_snapshot_save_context);

static void dbg_snapshot_dump_one_task_info(struct task_struct *tsk, bool is_main)
{
	char state_array[] = {'R', 'S', 'D', 'T', 't', 'X', 'Z', 'P', 'x', 'K', 'W', 'I', 'N'};
	unsigned char idx = 0;
	unsigned long state;
	unsigned long wchan;
	unsigned long pc = 0;
	char symname[KSYM_NAME_LEN];

	if ((tsk == NULL) || !try_get_task_stack(tsk))
		return;
	state = tsk->state | tsk->exit_state;

	pc = KSTK_EIP(tsk);
	wchan = get_wchan(tsk);
	if (lookup_symbol_name(wchan, symname) < 0)
		snprintf(symname, KSYM_NAME_LEN, "%lu", wchan);

	while (state) {
		idx++;
		state >>= 1;
	}

	/*
	 * kick watchdog to prevent unexpected reset during panic sequence
	 * and it prevents the hang during panic sequence by watchedog
	 */
	touch_softlockup_watchdog();
	dss_soc_ops->soc_kick_watchdog(NULL);

	pr_info("%8d %8d %8d %16lld %c(%d) %3d  %16zx %16zx  %16zx %c %16s [%s]\n",
			tsk->pid, (int)(tsk->utime), (int)(tsk->stime),
			tsk->se.exec_start, state_array[idx], (int)(tsk->state),
			task_cpu(tsk), wchan, pc, (unsigned long)tsk,
			is_main ? '*' : ' ', tsk->comm, symname);

	if (tsk->state == TASK_RUNNING || tsk->state == TASK_UNINTERRUPTIBLE || tsk->state == TASK_KILLABLE) {		
		sec_debug_wtsk_print_info(tsk, true);
		show_stack(tsk, NULL);
		pr_info("\n");
	}
}

static inline struct task_struct *get_next_thread(struct task_struct *tsk)
{
	return container_of(tsk->thread_group.next,
				struct task_struct,
				thread_group);
}

void dbg_snapshot_dump_task_info(void)
{
	struct task_struct *frst_tsk;
	struct task_struct *curr_tsk;
	struct task_struct *frst_thr;
	struct task_struct *curr_thr;

	pr_info("\n");
	pr_info(" current proc : %d %s\n", current->pid, current->comm);
	pr_info(" ----------------------------------------------------------------------------------------------------------------------------\n");
	pr_info("     pid      uTime    sTime      exec(ns)  stat  cpu       wchan           user_pc        task_struct       comm   sym_wchan\n");
	pr_info(" ----------------------------------------------------------------------------------------------------------------------------\n");

	/* processes */
	frst_tsk = &init_task;
	curr_tsk = frst_tsk;
	while (curr_tsk != NULL) {
		dbg_snapshot_dump_one_task_info(curr_tsk,  true);
		/* threads */
		if (curr_tsk->thread_group.next != NULL) {
			frst_thr = get_next_thread(curr_tsk);
			curr_thr = frst_thr;
			if (frst_thr != curr_tsk) {
				while (curr_thr != NULL) {
					dbg_snapshot_dump_one_task_info(curr_thr, false);
					curr_thr = get_next_thread(curr_thr);
					if (curr_thr == curr_tsk)
						break;
				}
			}
		}
		curr_tsk = container_of(curr_tsk->tasks.next,
					struct task_struct, tasks);
		if (curr_tsk == frst_tsk)
			break;
	}
	pr_info(" ----------------------------------------------------------------------------------------------------------------------------\n");
}

#ifdef CONFIG_DEBUG_SNAPSHOT_CRASH_KEY
void dbg_snapshot_check_crash_key(unsigned int code, int value)
{
	static bool volup_p;
	static bool voldown_p;
	static int loopcount;

	static const unsigned int VOLUME_UP = KEY_VOLUMEUP;
#if defined(CONFIG_DEBUG_SNAPSHOT_ONE_CRASH_KEY)
	static const unsigned int VOLUME_DOWN = KEY_RESET;
#else
	static const unsigned int VOLUME_DOWN = KEY_VOLUMEDOWN;
#endif
	if (code == KEY_POWER)
		pr_crit("debug-snapshot: POWER-KEY %s\n", value ? "pressed" : "released");

	/* Enter Forced Upload
	 *  Hold volume down key first
	 *  and then press power key twice
	 *  and volume up key should not be pressed
	 */
	if (value) {
		if (code == VOLUME_UP)
			volup_p = true;
		if (code == VOLUME_DOWN)
			voldown_p = true;
		if (!volup_p && voldown_p) {
			if (code == KEY_POWER) {
				pr_info
				    ("debug-snapshot: count for entering forced upload [%d]\n",
				     ++loopcount);
				if (loopcount == 2) {
					panic("Crash Key");
				}
			}
		}
	} else {
		if (code == VOLUME_UP)
			volup_p = false;
		if (code == VOLUME_DOWN) {
			loopcount = 0;
			voldown_p = false;
		}
	}
}
EXPORT_SYMBOL(dbg_snapshot_check_crash_key);
#endif

void __init dbg_snapshot_allcorelockup_detector_init(void)
{
	int ret;

	if (!dss_desc.multistage_wdt_irq)
		return;

	dss_allcorelockup_param.last_pc_addr = dbg_snapshot_get_last_pc_paddr();
	dss_allcorelockup_param.spin_pc_addr = __pa_symbol(dbg_snapshot_spin_func);

	__flush_dcache_area((void *)&dss_allcorelockup_param,
			sizeof(struct dbg_snapshot_allcorelockup_param));

#ifdef SMC_CMD_LOCKUP_NOTICE
	/* Setup for generating NMI interrupt to unstopped CPUs */
	ret = dss_soc_ops->soc_smc_call(SMC_CMD_LOCKUP_NOTICE,
				 (unsigned long)dbg_snapshot_bug_func,
				 dss_desc.multistage_wdt_irq,
				 (unsigned long)(virt_to_phys)(&dss_allcorelockup_param));
#endif

	pr_emerg("debug-snapshot: %s to register all-core lockup detector - ret: %d\n",
			ret == 0 ? "success" : "failed", ret);
}

void __init dbg_snapshot_init_utils(void)
{
	size_t vaddr;
	int i;

	vaddr = dss_items[dss_desc.header_num].entry.vaddr;

	for (i = 0; i < DSS_NR_CPUS; i++) {
		per_cpu(dss_mmu_reg, i) = (struct dbg_snapshot_mmu_reg *)
					  (vaddr + DSS_HEADER_SZ +
					   i * DSS_MMU_REG_OFFSET);
		per_cpu(dss_core_reg, i) = (struct pt_regs *)
					   (vaddr + DSS_HEADER_SZ + DSS_MMU_REG_SZ +
					    i * DSS_CORE_REG_OFFSET);
	}

	/* hardlockup_detector function should be called before secondary booting */
	dbg_snapshot_allcorelockup_detector_init();
}

static int __init dbg_snapshot_utils_save_systems_all(void)
{
	smp_call_function(dbg_snapshot_save_system, NULL, 1);
	dbg_snapshot_save_system(NULL);

	return 0;
}
postcore_initcall(dbg_snapshot_utils_save_systems_all);
