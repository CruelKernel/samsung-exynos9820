/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Exynos - Support SoC For Debug SnapShot
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/smc.h>
#include <linux/debug-snapshot-helper.h>
#include <soc/samsung/exynos-debug.h>
#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-bcm_dbg.h>
#include <soc/samsung/exynos-sdm.h>
#include <soc/samsung/exynos-sci.h>
#include <soc/samsung/exynos-flexpmu-dbg.h>

#include <asm/cacheflush.h>
#include <asm/cputype.h>
#include <asm/smp_plat.h>
#include <asm/core_regs.h>

#if defined(CONFIG_SEC_SIPC_MODEM_IF)
#include <soc/samsung/exynos-modem-ctrl.h>
#endif

#if defined(CONFIG_ACPM_DVFS)
#include <soc/samsung/acpm_ipc_ctrl.h>
#endif

#if defined(CONFIG_SEC_DEBUG)
#include <linux/sec_debug.h>
#endif

extern void (*arm_pm_restart)(char str, const char *cmd);

struct arm64_cpu_err_varient {
	u8 fld_end;
	u8 fld_offset;
	const char *fld_name;
};

#define CPU_ERR_VAR(name, end, offset)	\
	{					\
		.fld_end	= end,	\
		.fld_offset	= offset,	\
		.fld_name	= name,		\
	}

static struct arm64_cpu_err_varient arm64_err_type_1[] = {
	CPU_ERR_VAR("AV", 31, 31),
	CPU_ERR_VAR("Valid", 30, 30),
	CPU_ERR_VAR("UC", 29, 29),
	CPU_ERR_VAR("ER", 28, 28),
	CPU_ERR_VAR("OF", 27, 27),
	CPU_ERR_VAR("MV", 26, 26),
	CPU_ERR_VAR("CE", 25, 24),
	CPU_ERR_VAR("DE", 23, 23),
	CPU_ERR_VAR("PN", 22, 22),
	CPU_ERR_VAR("UET", 21, 20),
	CPU_ERR_VAR("RES0", 19, 16),
	CPU_ERR_VAR("IERR", 15, 8),
	CPU_ERR_VAR("SERR", 7, 0),
	CPU_ERR_VAR("END", 64, 64),
};

static struct arm64_cpu_err_varient arm64_err_type_2[] = {
	CPU_ERR_VAR("Count1", 63, 56),
	CPU_ERR_VAR("Count0", 55, 48),
	CPU_ERR_VAR("RES1", 47, 41),
	CPU_ERR_VAR("Syndrom", 40, 32),
	CPU_ERR_VAR("Stype2", 31, 28),
	CPU_ERR_VAR("SType1", 27, 24),
	CPU_ERR_VAR("SType0", 23, 16),
	CPU_ERR_VAR("Type", 15, 12),
	CPU_ERR_VAR("RES0", 11, 6),
	CPU_ERR_VAR("RE", 5, 5),
	CPU_ERR_VAR("SynValid", 4, 4),
	CPU_ERR_VAR("AddrValid", 3, 3),
	CPU_ERR_VAR("UnContained", 2, 2),
	CPU_ERR_VAR("UC", 1, 1),
	CPU_ERR_VAR("Vaild", 0, 0),
	CPU_ERR_VAR("END", 64, 64),
};

static struct arm64_cpu_err_varient arm64_err_type_3[] = {
	CPU_ERR_VAR("RES0", 63, 32),
	CPU_ERR_VAR("A", 31, 31),
	CPU_ERR_VAR("RES0", 30, 25),
	CPU_ERR_VAR("RES2", 24, 16),
	CPU_ERR_VAR("AEType", 15, 14),
	CPU_ERR_VAR("RES1", 13, 13),
	CPU_ERR_VAR("EXT", 12, 12),
	CPU_ERR_VAR("AET", 11, 10),
	CPU_ERR_VAR("EA", 9, 9),
	CPU_ERR_VAR("EA", 8, 6),
	CPU_ERR_VAR("DFSC", 5, 0),
	CPU_ERR_VAR("END", 64, 64),
};

static struct arm64_cpu_err_varient arm64_err_type_4[] = {
	CPU_ERR_VAR("Fatal", 63, 63),
	CPU_ERR_VAR("RES0", 62, 48),
	CPU_ERR_VAR("Other error count", 47, 40),
	CPU_ERR_VAR("Repeat error count", 39, 32),
	CPU_ERR_VAR("Valid", 31, 31),
	CPU_ERR_VAR("RAMID", 30, 24),
	CPU_ERR_VAR("RES0", 23, 21),
	CPU_ERR_VAR("CPUID", 20, 18),
	CPU_ERR_VAR("RES0", 17, 12),
	CPU_ERR_VAR("RAM address", 11, 0),
	CPU_ERR_VAR("END", 64, 64),
};

struct arm64_cpu_err_varient_data {
	const struct arm64_cpu_err_varient *varient;
	u8 valid_bit;
	const char *reg_name;
};

#define ARM64_CPU_ERR(varients, valid, reg)	\
	{					\
		.varient	= varients,	\
		.valid_bit	= valid,	\
		.reg_name	= reg,		\
	}

static struct arm64_cpu_err_varient_data exynos_cpu_err_table[] = {
	ARM64_CPU_ERR(arm64_err_type_2, 0, "FEMERR0SR"),
	ARM64_CPU_ERR(arm64_err_type_2, 0, "LSMERR0SR"),
	ARM64_CPU_ERR(arm64_err_type_2, 0, "TBWMERR0SR"),
	ARM64_CPU_ERR(arm64_err_type_2, 0, "L2MERR0SR"),
	ARM64_CPU_ERR(arm64_err_type_2, 0, "L3MERR0SR"),
	ARM64_CPU_ERR(arm64_err_type_3, 31, "DISR_EL1"),
	ARM64_CPU_ERR(arm64_err_type_1, 30, "ERXSTATUS_EL1"),
	ARM64_CPU_ERR(arm64_err_type_4, 31, "CPUMERRSR"),
	ARM64_CPU_ERR(arm64_err_type_4, 31, "L2MERRSR"),
};

void parse_cpu_err(char *reg_name, u64 reg)
{
	u8 i = 0;
	u8 fld_offset, fld_end;
	u64 valid;
	u32 field;
	struct arm64_cpu_err_varient_data *exynos_cpu_err;
	const struct arm64_cpu_err_varient *varient;

	for (i = 0; i < ARRAY_SIZE(exynos_cpu_err_table); i++) {
		if (!strncmp(exynos_cpu_err_table[i].reg_name, reg_name, sizeof(char *))) {
			exynos_cpu_err = &exynos_cpu_err_table[i];
			break;
		}
	}

	if (i == ARRAY_SIZE(exynos_cpu_err_table)) {
		pr_err("%s: there is no parse data\n", __func__);
		return;
	}

	if (exynos_cpu_err->valid_bit == 0xFF)
		goto run_valid;

	valid = reg & BIT(exynos_cpu_err->valid_bit);
	if (!valid) {
		pr_emerg("%s valid_bit(%d) is NOT set (0x%lx)\n",
				exynos_cpu_err->reg_name, exynos_cpu_err->valid_bit, valid);
		return;
	}

run_valid:
	varient = exynos_cpu_err->varient;
	while (true) {
		fld_offset = varient[i].fld_offset;
		fld_end = varient[i].fld_end;

		if (fld_end > 63)
			break;

		field = (reg & GENMASK_ULL(fld_end, fld_offset)) >> fld_offset;
		if (field != 0)
			pr_emerg("%s (%d:%d) %s %04lx\n",
				exynos_cpu_err->reg_name,
				fld_end, fld_offset,
				varient[i].fld_name, field);
		i++;
	};
};

static void exynos_early_panic(void *val)
{
	exynos_bcm_dbg_stop(PANIC_HANDLE);
	exynos_flexpmu_dbg_log_stop();
}

static void exynos_prepare_panic_entry(void *val)
{
	/* TODO: Something */
}

static void exynos_prepare_panic_exit(void *val)
{
#if defined(CONFIG_SEC_SIPC_MODEM_IF)
	modem_send_panic_noti_ext();
#endif
#if defined(CONFIG_ACPM_DVFS)
	acpm_stop_log();
#endif
}

static void exynos_post_panic_entry(void *val)
{
	flush_cache_all();

#ifdef CONFIG_EXYNOS_SDM
	if (dbg_snapshot_is_scratch() && sec_debug_enter_upload())
		exynos_sdm_dump_secure_region();
#endif
}

static void exynos_post_panic_exit(void *val)
{
	sci_error_dump();
	llc_dump();
#ifdef CONFIG_DEBUG_SNAPSHOT_PANIC_REBOOT
	arm_pm_restart(0, "panic");
#endif
}

static void exynos_post_reboot_entry(void *val)
{
	/* TODO: Something */

}

static void exynos_post_reboot_exit(void *val)
{
	flush_cache_all();
}

static void exynos_save_core(void *val)
{
	struct pt_regs *core_reg = (struct pt_regs *)val;

	asm volatile ("str x0, [%0, #0]\n\t"
		"mov x0, %0\n\t"
		"str x1, [x0, #8]\n\t"
		"str x2, [x0, #16]\n\t"
		"str x3, [x0, #24]\n\t"
		"str x4, [x0, #32]\n\t"
		"str x5, [x0, #40]\n\t"
		"str x6, [x0, #48]\n\t"
		"str x7, [x0, #56]\n\t"
		"str x8, [x0, #64]\n\t"
		"str x9, [x0, #72]\n\t"
		"str x10, [x0, #80]\n\t"
		"str x11, [x0, #88]\n\t"
		"str x12, [x0, #96]\n\t"
		"str x13, [x0, #104]\n\t"
		"str x14, [x0, #112]\n\t"
		"str x15, [x0, #120]\n\t"
		"str x16, [x0, #128]\n\t"
		"str x17, [x0, #136]\n\t"
		"str x18, [x0, #144]\n\t"
		"str x19, [x0, #152]\n\t"
		"str x20, [x0, #160]\n\t"
		"str x21, [x0, #168]\n\t"
		"str x22, [x0, #176]\n\t"
		"str x23, [x0, #184]\n\t"
		"str x24, [x0, #192]\n\t"
		"str x25, [x0, #200]\n\t"
		"str x26, [x0, #208]\n\t"
		"str x27, [x0, #216]\n\t"
		"str x28, [x0, #224]\n\t"
		"str x29, [x0, #232]\n\t"
		"str x30, [x0, #240]\n\t" :
		: "r"(core_reg));
	core_reg->sp = core_reg->regs[29];
	core_reg->pc =
		(unsigned long)(core_reg->regs[30] - sizeof(unsigned int));
}

static void exynos_save_system(void *val)
{
	struct dbg_snapshot_mmu_reg *mmu_reg =
			(struct dbg_snapshot_mmu_reg *)val;

#ifdef CONFIG_ARM64
	asm volatile ("mrs x1, SCTLR_EL1\n\t"		/* SCTLR_EL1 */
		"str x1, [%0]\n\t"
		"mrs x1, TTBR0_EL1\n\t"		/* TTBR0_EL1 */
		"str x1, [%0,#8]\n\t"
		"mrs x1, TTBR1_EL1\n\t"		/* TTBR1_EL1 */
		"str x1, [%0,#16]\n\t"
		"mrs x1, TCR_EL1\n\t"		/* TCR_EL1 */
		"str x1, [%0,#24]\n\t"
		"mrs x1, ESR_EL1\n\t"		/* ESR_EL1 */
		"str x1, [%0,#32]\n\t"
		"mrs x1, FAR_EL1\n\t"		/* FAR_EL1 */
		"str x1, [%0,#40]\n\t"
		/* Don't populate AFSR0_EL1 and AFSR1_EL1 */
		"mrs x1, CONTEXTIDR_EL1\n\t"	/* CONTEXTIDR_EL1 */
		"str x1, [%0,#48]\n\t"
		"mrs x1, TPIDR_EL0\n\t"		/* TPIDR_EL0 */
		"str x1, [%0,#56]\n\t"
		"mrs x1, TPIDRRO_EL0\n\t"		/* TPIDRRO_EL0 */
		"str x1, [%0,#64]\n\t"
		"mrs x1, TPIDR_EL1\n\t"		/* TPIDR_EL1 */
		"str x1, [%0,#72]\n\t"
		"mrs x1, MAIR_EL1\n\t"		/* MAIR_EL1 */
		"str x1, [%0,#80]\n\t"
		"mrs x1, ELR_EL1\n\t"		/* ELR_EL1 */
		"str x1, [%0, #88]\n\t"
		"mrs x1, SP_EL0\n\t"		/* SP_EL0 */
		"str x1, [%0, #96]\n\t" :	/* output */
		: "r"(mmu_reg)			/* input */
		: "%x1", "memory"			/* clobbered register */
	);
#else
	asm volatile ("mrc    p15, 0, r1, c1, c0, 0\n\t"	/* SCTLR */
	    "str r1, [%0]\n\t"
	    "mrc    p15, 0, r1, c2, c0, 0\n\t"	/* TTBR0 */
	    "str r1, [%0,#4]\n\t"
	    "mrc    p15, 0, r1, c2, c0,1\n\t"	/* TTBR1 */
	    "str r1, [%0,#8]\n\t"
	    "mrc    p15, 0, r1, c2, c0,2\n\t"	/* TTBCR */
	    "str r1, [%0,#12]\n\t"
	    "mrc    p15, 0, r1, c3, c0,0\n\t"	/* DACR */
	    "str r1, [%0,#16]\n\t"
	    "mrc    p15, 0, r1, c5, c0,0\n\t"	/* DFSR */
	    "str r1, [%0,#20]\n\t"
	    "mrc    p15, 0, r1, c6, c0,0\n\t"	/* DFAR */
	    "str r1, [%0,#24]\n\t"
	    "mrc    p15, 0, r1, c5, c0,1\n\t"	/* IFSR */
	    "str r1, [%0,#28]\n\t"
	    "mrc    p15, 0, r1, c6, c0,2\n\t"	/* IFAR */
	    "str r1, [%0,#32]\n\t"
	    /* Don't populate DAFSR and RAFSR */
	    "mrc    p15, 0, r1, c10, c2,0\n\t"	/* PMRRR */
	    "str r1, [%0,#44]\n\t"
	    "mrc    p15, 0, r1, c10, c2,1\n\t"	/* NMRRR */
	    "str r1, [%0,#48]\n\t"
	    "mrc    p15, 0, r1, c13, c0,0\n\t"	/* FCSEPID */
	    "str r1, [%0,#52]\n\t"
	    "mrc    p15, 0, r1, c13, c0,1\n\t"	/* CONTEXT */
	    "str r1, [%0,#56]\n\t"
	    "mrc    p15, 0, r1, c13, c0,2\n\t"	/* URWTPID */
	    "str r1, [%0,#60]\n\t"
	    "mrc    p15, 0, r1, c13, c0,3\n\t"	/* UROTPID */
	    "str r1, [%0,#64]\n\t"
	    "mrc    p15, 0, r1, c13, c0,4\n\t"	/* POTPIDR */
	    "str r1, [%0,#68]\n\t" :		/* output */
	    : "r"(mmu_reg)			/* input */
	    : "%r1", "memory"			/* clobbered register */
	);
#endif
}

static void exynos_dump_info(void *val)
{
	/*
	 *  Output CPU Memory Error syndrome Register
	 *  CPUMERRSR, L2MERRSR
	 */
#ifdef CONFIG_ARM64
	unsigned long reg1, reg2, reg3;

	if (read_cpuid_implementor() == ARM_CPU_IMP_SEC) {
		switch (read_cpuid_part_number()) {
		case ARM_CPU_PART_MONGOOSE:
		case ARM_CPU_PART_MEERKAT:
		case ARM_CPU_PART_CHEETAH:
			asm volatile ("mrs %0, S3_1_c15_c2_0\n\t"
					"mrs %1, S3_1_c15_c2_4\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("FEMERR0SR: %016lx, FEMERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("FEMERR0SR", reg1);
			asm volatile ("mrs %0, S3_1_c15_c2_1\n\t"
					"mrs %1, S3_1_c15_c2_5\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("LSMERR0SR: %016lx, LSMERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("LSMERR0SR", reg1);
			asm volatile ("mrs %0, S3_1_c15_c2_2\n\t"
					"mrs %1, S3_1_c15_c2_6\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("TBWMERR0SR: %016lx, TBWMERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("TBWMERR0SR", reg1);
			asm volatile ("mrs %0, S3_1_c15_c2_3\n\t"
					"mrs %1, S3_1_c15_c2_7\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("L2MERR0SR: %016lx, L2MERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("L2MERR0SR", reg1);

			/* L3 MERR */
			asm volatile ("msr S3_1_c15_c7_1, %0\n\t"
					"isb\n"
					:: "r" (0));
			asm volatile ("mrs %0, S3_1_c15_c3_0\n\t"
					"mrs %1, S3_1_c15_c3_4\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("BANK0 L3MERR0SR: %016lx, L3MERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("L3MERR0SR", reg1);
			asm volatile ("msr S3_1_c15_c7_1, %0\n\t"
					"isb\n"
					:: "r" (1));
			asm volatile ("mrs %0, S3_1_c15_c3_0\n\t"
					"mrs %1, S3_1_c15_c3_4\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("BANK1 L3MERR0SR: %016lx, L3MERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("L3MERR0SR", reg1);
			asm volatile ("msr S3_1_c15_c7_1, %0\n\t"
					"isb\n"
					:: "r" (2));
			asm volatile ("mrs %0, S3_1_c15_c3_0\n\t"
					"mrs %1, S3_1_c15_c3_4\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("BANK2 L3MERR0SR: %016lx, L3MERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("L3MERR0SR", reg1);
			asm volatile ("msr S3_1_c15_c7_1, %0\n\t"
					"isb\n"
					:: "r" (3));
			asm volatile ("mrs %0, S3_1_c15_c3_0\n\t"
					"mrs %1, S3_1_c15_c3_4\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("BANK3 L3MERR0SR: %016lx, L3MERR1SR: %016lx\n", reg1, reg2);
			parse_cpu_err("L3MERR0SR", reg1);

			break;
		default:
			break;
		}
	} else {
		switch (read_cpuid_part_number()) {
		case ARM_CPU_PART_CORTEX_A57:
		case ARM_CPU_PART_CORTEX_A53:
			asm volatile ("mrs %0, S3_1_c15_c2_2\n\t"
					"mrs %1, S3_1_c15_c2_3\n"
					: "=r" (reg1), "=r" (reg2));
			pr_emerg("CPUMERRSR: %016lx, L2MERRSR: %016lx\n", reg1, reg2);
			break;
		case ARM_CPU_PART_ANANKE:
		case ARM_CPU_PART_CORTEX_A75:
			asm volatile ("HINT #16");
			asm volatile ("mrs %0, S3_0_c12_c1_1\n" : "=r" (reg1)); /* read DISR_EL1 */
			pr_emerg("DISR_EL1: %016lx\n", reg1);
			parse_cpu_err("DISR_EL1", reg1);

			asm volatile ("msr S3_0_c5_c3_1, %0\n"
					"isb\n"
					:: "r" (0)); /* set CPU ERRSELR_EL1 */

			asm volatile ("mrs %0, S3_0_c5_c4_2\n"
					"mrs %1, S3_0_c5_c4_3\n"
					"mrs %2, S3_0_c5_c5_0\n"
					: "=r" (reg1), "=r" (reg2), "=r" (reg3));
			pr_emerg("CPU : ERXSTATUS_EL1: %016lx, ERXADDR_EL1: %016lx, "
					"ERXMISC0_EL1: %016lx\n", reg1, reg2, reg3);
			parse_cpu_err("ERXSTATUS_EL1", reg1);

			asm volatile ("msr S3_0_c5_c3_1, %0\n"
					"isb\n"
					:: "r" (1)); /* set DSU ERRSELR_EL1 */

			asm volatile ("mrs %0, S3_0_c5_c4_2\n"
					"mrs %1, S3_0_c5_c4_3\n"
					"mrs %2, S3_0_c5_c5_0\n"
					: "=r" (reg1), "=r" (reg2), "=r" (reg3));
			pr_emerg("DSU : ERXSTATUS_EL1: %016lx, ERXADDR_EL1: %016lx, "
					"ERXMISC0_EL1: %016lx\n", reg1, reg2, reg3);
			parse_cpu_err("ERXSTATUS_EL1", reg1);

			break;
		default:
			break;
		}
	}
#else
	unsigned long reg0;
	asm volatile ("mrc p15, 0, %0, c0, c0, 0\n": "=r" (reg0));
	if (((reg0 >> 4) & 0xFFF) == 0xC0F) {
		/*  Only Cortex-A15 */
		unsigned long reg1, reg2, reg3;
		asm volatile ("mrrc p15, 0, %0, %1, c15\n\t"
			"mrrc p15, 1, %2, %3, c15\n"
			: "=r" (reg0), "=r" (reg1),
			"=r" (reg2), "=r" (reg3));
		pr_emerg("CPUMERRSR: %08lx_%08lx, L2MERRSR: %08lx_%08lx\n",
				reg1, reg0, reg3, reg2);
	}
#endif
}

static void exynos_save_context_entry(void *val)
{
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	exynos_trace_stop();
#endif
}

static void exynos_save_context_exit(void *val)
{
#if defined(CONFIG_ACPM_DVFS)
	acpm_stop_log();
#endif
	flush_cache_all();
	pr_emerg("============ CPU Error Log Info after Cache flush ================\n");
	exynos_dump_info(val);
}

static void exynos_start_watchdog(void *val)
{
#ifdef CONFIG_S3C2410_WATCHDOG
	s3c2410wdt_keepalive_emergency(true, 0);
#endif
}

static void exynos_expire_watchdog(void *val)
{
#ifdef CONFIG_S3C2410_WATCHDOG
	s3c2410wdt_set_emergency_reset(100, 0);
#endif
}

static void exynos_stop_watchdog(void *val)
{

}

static void exynos_kick_watchdog(void *val)
{
#ifdef CONFIG_S3C2410_WATCHDOG
	s3c2410wdt_keepalive_emergency(false, 0);
#endif
}

static int exynos_is_power_cpu(unsigned int cpu)
{
#ifdef CONFIG_EXYNOS_PMU
	return exynos_cpu.power_state(cpu);
#else
	return 0;
#endif
}

struct dbg_snapshot_helper_ops exynos_debug_ops = {
	.soc_early_panic 	= exynos_early_panic,
	.soc_prepare_panic_entry = exynos_prepare_panic_entry,
	.soc_prepare_panic_exit	= exynos_prepare_panic_exit,
	.soc_post_panic_entry	= exynos_post_panic_entry,
	.soc_post_panic_exit	= exynos_post_panic_exit,
	.soc_post_reboot_entry	= exynos_post_reboot_entry,
	.soc_post_reboot_exit	= exynos_post_reboot_exit,
	.soc_save_context_entry	= exynos_save_context_entry,
	.soc_save_context_exit	= exynos_save_context_exit,
	.soc_save_core		= exynos_save_core,
	.soc_save_system	= exynos_save_system,
	.soc_dump_info		= exynos_dump_info,
	.soc_start_watchdog	= exynos_start_watchdog,
	.soc_expire_watchdog	= exynos_expire_watchdog,
	.soc_stop_watchdog	= exynos_stop_watchdog,
	.soc_kick_watchdog	= exynos_kick_watchdog,
	.soc_is_power_cpu	= exynos_is_power_cpu,
	.soc_smc_call		= exynos_smc,
};

void __init dbg_snapshot_soc_helper_init(void)
{
	dbg_snapshot_register_soc_ops(&exynos_debug_ops);
}
