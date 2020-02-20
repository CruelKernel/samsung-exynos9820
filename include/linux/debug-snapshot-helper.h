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

#ifndef DEBUG_SNAPSHOT_HELPER_H
#define DEBUG_SNAPSHOT_HELPER_H

struct dbg_snapshot_helper_ops {
	void (*soc_early_panic)(void *);

	void (*soc_prepare_panic_entry)(void *);
	void (*soc_prepare_panic_exit)(void *);

	void (*soc_post_panic_entry)(void *);
	void (*soc_post_panic_exit)(void *);

	void (*soc_post_reboot_entry)(void *);
	void (*soc_post_reboot_exit)(void *);

	void (*soc_save_context_entry)(void *);
	void (*soc_save_context_exit)(void *);

	void (*soc_save_core)(void *);
	void (*soc_save_context)(void *);
	void (*soc_save_system)(void *);
	void (*soc_dump_info)(void *);

	void (*soc_start_watchdog)(void *);
	void (*soc_expire_watchdog)(void *);
	void (*soc_stop_watchdog)(void *);
	void (*soc_kick_watchdog)(void *);

	int (*soc_is_power_cpu)(unsigned int);
	int (*soc_smc_call)(unsigned long, unsigned long, unsigned long, unsigned long);
};

extern void dbg_snapshot_register_soc_ops(struct dbg_snapshot_helper_ops *ops);
extern bool dbg_snapshot_is_scratch(void);

extern void dbg_snapshot_set_debug_test_reg(unsigned int val);
extern bool dbg_snapshot_debug_test_enabled(void);
extern void dbg_snapshot_set_debug_test_case(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_case(void);
extern void dbg_snapshot_set_debug_test_next(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_next(void);
extern void dbg_snapshot_set_debug_test_panic(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_panic(void);
extern void dbg_snapshot_set_debug_test_wdt(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_wdt(void);
extern void dbg_snapshot_set_debug_test_wtsr(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_wtsr(void);
extern void dbg_snapshot_set_debug_test_smpl(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_smpl(void);
extern void dbg_snapshot_set_debug_test_curr(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_curr(void);
extern void dbg_snapshot_set_debug_test_total(unsigned int val);
extern unsigned int dbg_snapshot_get_debug_test_total(void);
extern void dbg_snapshot_set_debug_test_run(unsigned int test_id, unsigned int var);
extern unsigned int dbg_snapshot_get_debug_test_run(unsigned int test_id);
extern void dbg_snapshot_clear_debug_test_runflag(void);
extern unsigned int dbg_snapshot_get_debug_test_runflag(void);

#ifdef CONFIG_ARM64
struct dbg_snapshot_mmu_reg {
	long SCTLR_EL1;
	long TTBR0_EL1;
	long TTBR1_EL1;
	long TCR_EL1;
	long ESR_EL1;
	long FAR_EL1;
	long CONTEXTIDR_EL1;
	long TPIDR_EL0;
	long TPIDRRO_EL0;
	long TPIDR_EL1;
	long MAIR_EL1;
	long ELR_EL1;
	long SP_EL0;
};

#else
struct dbg_snapshot_mmu_reg {
	int SCTLR;
	int TTBR0;
	int TTBR1;
	int TTBCR;
	int DACR;
	int DFSR;
	int DFAR;
	int IFSR;
	int IFAR;
	int DAFSR;
	int IAFSR;
	int PMRRR;
	int NMRRR;
	int FCSEPID;
	int CONTEXT;
	int URWTPID;
	int UROTPID;
	int POTPIDR;
};
#endif


struct err_variant {
	u8 fld_end;
	u8 fld_offset;
	const char *fld_name;
};

struct err_variant_data {
	const struct err_variant *variant;
	u8 valid_bit;
	const char *reg_name;
};

#define ERR_REG(variants, valid, reg)	\
	{					\
		.variant	= variants,	\
		.valid_bit	= valid,	\
		.reg_name	= reg,		\
	}

#define ERR_VAR(name, end, offset)	\
	{					\
		.fld_end	= end,	\
		.fld_offset	= offset,	\
		.fld_name	= name,		\
	}


#endif

extern void exynos_err_parse(u32 reg_idx, u64 reg, struct err_variant_data *exynos_cpu_err);
