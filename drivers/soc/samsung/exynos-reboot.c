/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * Exynos - Support SoC specific Reboot
 * Author: Hosung Kim <hosung0.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <linux/debug-snapshot.h>

#ifdef CONFIG_EXYNOS_ACPM
#include <soc/samsung/acpm_ipc_ctrl.h>
#endif
#include <soc/samsung/exynos-pmu.h>

extern void (*arm_pm_restart)(enum reboot_mode reboot_mode, const char *cmd);
static void __iomem *exynos_pmu_base = NULL;

static const char * const little_cores[] = {
	"arm,ananke",
	NULL,
};

static const char * const big_cores[] = {
	"arm,mongoose",
	"arm,meerkat",
	"arm,cortex-a73",
	NULL,
};

static bool is_what_cpu(struct device_node *cn, const char * const *cores)
{
	const char * const *lc;
	for (lc = (const char * const *)cores; *lc; lc++)
		if (of_device_is_compatible(cn, *lc))
			return true;
	return false;
}

int soc_has_little(void)
{
	struct device_node *cn = NULL;
	u32 little_cpu_cnt = 0;

	while ((cn = of_find_node_by_type(cn, "cpu"))) {
		if (is_what_cpu(cn, little_cores))
			little_cpu_cnt++;
	}
	return little_cpu_cnt;

}

int soc_has_big(void)
{
	struct device_node *cn = NULL;
	u32 big_cpu_cnt = 0;

	/* find arm,mongoose compatable in device tree */
	while ((cn = of_find_node_by_type(cn, "cpu"))) {
		if (is_what_cpu(cn, big_cores))
			big_cpu_cnt++;
	}
	return big_cpu_cnt;
}

#define CPU_RESET_DISABLE_FROM_SOFTRESET	(0x041C)
#define CPU_RESET_DISABLE_FROM_WDTRESET		(0x0414)
#define BIG_CPU0_RESET				(0x3C0C)
#define BIG_NONCPU_ETC_RESET			(0x3C8C)
#define LITTLE_CPU0_ETC_RESET			(0x3C4C)
#ifdef CONFIG_SOC_EXYNOS9810
#define SWRESET					(0x0400)
#define RST_STAT				(0x0404)
#define SWRESET_TRIGGER				(1 << 0)
#define PS_HOLD_CONTROL				(0x330C)
#else
#define SWRESET					(0x3A00)
#define RST_STAT				(0x0404)
#define SWRESET_TRIGGER				(1 << 1)
#define PS_HOLD_CONTROL				(0x030C)
#endif
#define RESET_SEQUENCER_CONFIGURATION		(0x0500)
#define PS_HOLD_EN				(1 << 8)
#define EXYNOS_PMU_SYSIP_DAT0			(0x0810)

/* defines for BIG reset */
#define PEND_BIG				(1 << 1)
#define PEND_LITTLE				(1 << 0)
#define DEFAULT_VAL_CPU_RESET_DISABLE		(0xFFFFFFFC)
#define RESET_DISABLE_GPR_CPUPORESET		(1 << 15)
#define RESET_DISABLE_WDT_CORERESET		(1 << 13)
#define RESET_DISABLE_WDT_CPUPORESET		(1 << 12)
#define RESET_DISABLE_CORERESET			(1 << 9)
#define RESET_DISABLE_CPUPORESET		(1 << 8)
#define RESET_DISABLE_WDT_PRESET_DBG		(1 << 26)
#define RESET_DISABLE_PRESET_DBG		(1 << 18)
#define DFD_EDPCSR_DUMP_EN			(1 << 0)
#define RESET_DISABLE_L2RESET			(1 << 16)
#define RESET_DISABLE_WDT_L2RESET		(1 << 31)

#define DFD_EDPCSR_DUMP_EN			(1 << 0)
#define DFD_L2RSTDISABLE_BIG_EN			(1 << 11)
#define DFD_DBGL1RSTDISABLE_BIG_EN		(1 << 10)
#define DFD_L2RSTDISABLE_LITTLE_EN		(1 << 9)
#define DFD_DBGL1RSTDISABLE_LITTLE_EN		(0xF << 24)
#define DFD_CLEAR_L2RSTDISABLE_BIG		(1 << 7)
#define DFD_CLEAR_DBGL1RSTDISABLE_BIG		(1 << 6)
#define DFD_CLEAR_L2RSTDISABLE_LITTLE		(1 << 5)
#define DFD_CLEAR_DBGL1RSTDISABLE_LITTLE	(0xF << 20)

#define DFD_L2RSTDISABLE		(DFD_L2RSTDISABLE_BIG_EN	\
					| DFD_DBGL1RSTDISABLE_BIG_EN	\
					| DFD_L2RSTDISABLE_LITTLE_EN	\
					| DFD_DBGL1RSTDISABLE_LITTLE_EN)
#define DFD_CLEAR_L2RSTDISABLE		(DFD_CLEAR_L2RSTDISABLE_BIG 	\
					| DFD_CLEAR_DBGL1RSTDISABLE_BIG	\
					| DFD_CLEAR_L2RSTDISABLE_LITTLE	\
					| DFD_CLEAR_DBGL1RSTDISABLE_LITTLE)
#define DFD_RESET_PEND			(PEND_BIG | PEND_LITTLE)
#define DFD_DISABLE_RESET		(RESET_DISABLE_WDT_CORERESET	\
					| RESET_DISABLE_WDT_CPUPORESET	\
					| RESET_DISABLE_CORERESET	\
					| RESET_DISABLE_CPUPORESET)
#define	DFD_BIG_NONCPU_ETC_RESET	(RESET_DISABLE_WDT_PRESET_DBG	\
					| RESET_DISABLE_PRESET_DBG	\
					| RESET_DISABLE_WDT_L2RESET)
#define	PMU_CPU_OFFSET			(0x10)

void dfd_set_dump_gpr(int en)
{
	u32 reg_val;

	if (en) {
		reg_val = DFD_EDPCSR_DUMP_EN | DFD_L2RSTDISABLE;
		exynos_pmu_write(RESET_SEQUENCER_CONFIGURATION, reg_val);
	} else {
		exynos_pmu_read(RESET_SEQUENCER_CONFIGURATION, &reg_val);
		if (reg_val) {
			reg_val = DFD_EDPCSR_DUMP_EN | DFD_CLEAR_L2RSTDISABLE;
			exynos_pmu_write(RESET_SEQUENCER_CONFIGURATION, reg_val);
		}
	}
#ifdef REBOOT_DEBUG
	exynos_pmu_read(RESET_SEQUENCER_CONFIGURATION, &reg_val);
	pr_info("RESET_SEQUENCER_CONFIGURATION :%x\n", reg_val);
#endif
}

void little_reset_control(int en)
{
	u32 val;
	u32 little_cpu_cnt = soc_has_little();
	u32 check_dumpGPR;
	u32 soc_id = exynos_soc_info.product_id;
	u32 revision = exynos_soc_info.revision;

	if (little_cpu_cnt == 0 || !exynos_pmu_base)
		return;

	if ((soc_id == EXYNOS9810_SOC_ID && revision < EXYNOS_MAIN_REV_1) ||
		soc_id != EXYNOS9810_SOC_ID)
		return;

	exynos_pmu_read(RESET_SEQUENCER_CONFIGURATION, &check_dumpGPR);
	if (!(check_dumpGPR & DFD_EDPCSR_DUMP_EN))
		return;

	if (en) {
		/* reset disable for Little Cores */
		for (val = 0; val < little_cpu_cnt; val++)
			exynos_pmu_update(LITTLE_CPU0_ETC_RESET + (val * PMU_CPU_OFFSET),
					RESET_DISABLE_CPUPORESET | RESET_DISABLE_WDT_CPUPORESET,
					RESET_DISABLE_CPUPORESET | RESET_DISABLE_WDT_CPUPORESET);
	} else {
		/* reset enable for Little Cores */
		for (val = 0; val < little_cpu_cnt; val++)
			exynos_pmu_update(LITTLE_CPU0_ETC_RESET + (val * PMU_CPU_OFFSET),
					RESET_DISABLE_CPUPORESET | RESET_DISABLE_WDT_CPUPORESET, 0);
	}
}

void big_reset_control(int en)
{
	u32 reg_val, val;
	u32 big_cpu_cnt = soc_has_big();
	u32 check_dumpGPR;

	if (big_cpu_cnt == 0 || !exynos_pmu_base)
		return;

	exynos_pmu_read(RESET_SEQUENCER_CONFIGURATION, &check_dumpGPR);
	if (!(check_dumpGPR & DFD_EDPCSR_DUMP_EN))
		return;

	if (en) {
		/* reset disable for BIG */
		exynos_pmu_read(CPU_RESET_DISABLE_FROM_SOFTRESET, &reg_val);
		if (reg_val != DEFAULT_VAL_CPU_RESET_DISABLE)
			exynos_pmu_update(CPU_RESET_DISABLE_FROM_SOFTRESET,
					DFD_RESET_PEND, 0);

		exynos_pmu_read(CPU_RESET_DISABLE_FROM_WDTRESET, &reg_val);
		if (reg_val != DEFAULT_VAL_CPU_RESET_DISABLE)
			exynos_pmu_update(CPU_RESET_DISABLE_FROM_WDTRESET,
					DFD_RESET_PEND, 0);

		for (val = 0; val < big_cpu_cnt; val++)
			exynos_pmu_update(BIG_CPU0_RESET + (val * PMU_CPU_OFFSET),
					DFD_DISABLE_RESET, DFD_DISABLE_RESET);

		exynos_pmu_update(BIG_NONCPU_ETC_RESET, DFD_BIG_NONCPU_ETC_RESET,
					DFD_BIG_NONCPU_ETC_RESET);
	} else {
		/* reset enable for BIG */
		for (val = 0; val < big_cpu_cnt; val++)
			exynos_pmu_update(BIG_CPU0_RESET + (val * PMU_CPU_OFFSET),
					DFD_DISABLE_RESET, 0);

		exynos_pmu_update(BIG_NONCPU_ETC_RESET, DFD_BIG_NONCPU_ETC_RESET, 0);
	}

#ifdef REBOOT_DEBUG
	exynos_pmu_read(CPU_RESET_DISABLE_FROM_SOFTRESET, &reg_val);
	pr_info("CPU_RESET_DISABLE_FROM_SOFTRESET :%x\n", reg_val);
	exynos_pmu_read(CPU_RESET_DISABLE_FROM_WDTRESET, &reg_val);
	pr_info("CPU_RESET_DISABLE_FROM_WDTRESET :%x\n", reg_val);
	exynos_pmu_read(BIG_NONCPU_ETC_RESET, &reg_val);
	pr_info("BIG_NONCPU_ETC_RESET :%x\n", reg_val);

	for (val = 0; val < big_cpu_cnt; val++) {
		exynos_pmu_read(BIG_CPU0_RESET + (val * PMU_CPU_OFFSET), &reg_val);
		pr_info("BIG_CPU%d_RESET :%x\n", val, reg_val);
	}
#endif
}

#define INFORM_NONE		0x0
#define INFORM_RAMDUMP		0xd
#define INFORM_RECOVERY		0xf

#define REBOOT_MODE_NORMAL		0x00
#define REBOOT_MODE_CHARGE		0x0A
/* Reboot into fastboot mode */
#define REBOOT_MODE_FASTBOOT		0xFC
/* Reboot into recovery */
#define REBOOT_MODE_RECOVERY		0xFF

#if !defined(CONFIG_SEC_REBOOT)
#ifdef CONFIG_OF
static void exynos_power_off(void)
{
	int poweroff_try = 0;
	int power_gpio = -1;
	unsigned int keycode = 0;
	struct device_node *np, *pp;

	np = of_find_node_by_path("/gpio_keys");
	if (!np)
		return;

	for_each_child_of_node(np, pp) {
		if (!of_find_property(pp, "gpios", NULL))
			continue;
		of_property_read_u32(pp, "linux,code", &keycode);
		if (keycode == KEY_POWER) {
			pr_info("%s: <%u>\n", __func__,  keycode);
			power_gpio = of_get_gpio(pp, 0);
			break;
		}
	}

	of_node_put(np);

	if (!gpio_is_valid(power_gpio)) {
		pr_err("Couldn't find power key node\n");
		return;
	}

	while (1) {
		/* wait for power button release */
		if (gpio_get_value(power_gpio)) {
#ifdef CONFIG_EXYNOS_ACPM
			exynos_acpm_reboot();
#endif
			pr_emerg("%s: Set PS_HOLD Low.\n", __func__);
			exynos_pmu_update(PS_HOLD_CONTROL, PS_HOLD_EN, 0);
			++poweroff_try;
			pr_emerg("%s: Should not reach here! (poweroff_try:%d)\n",
				 __func__, poweroff_try);
		} else {
			/* if power button is not released, wait and check TA again */
			pr_info("%s: PowerButton is not released.\n", __func__);
		}
		mdelay(1000);
	}
}
#else
static void exynos_power_off(void)
{
	pr_info("Exynos power off does not support.\n");
}
#endif
#endif

static void exynos_reboot(enum reboot_mode mode, const char *cmd)
{
	u32 reg_val, soc_id, revision;
	void __iomem *addr;




	if (!exynos_pmu_base)
		return;
#ifdef CONFIG_EXYNOS_ACPM
	exynos_acpm_reboot();
#endif
	printk("[%s] reboot cmd: %s\n", __func__, cmd);

	addr = exynos_pmu_base + EXYNOS_PMU_SYSIP_DAT0;
	if (cmd) {
		if (!strcmp((char *)cmd, "charge")) {
			__raw_writel(REBOOT_MODE_CHARGE, addr);
		} else if (!strcmp(cmd, "bootloader") || !strcmp(cmd, "bl") ||
				!strcmp((char *)cmd, "fastboot") || !strcmp(cmd, "fb")) {
			__raw_writel(REBOOT_MODE_FASTBOOT, addr);
		} else if (!strcmp(cmd, "recovery")) {
			__raw_writel(REBOOT_MODE_RECOVERY, addr);
		}
	}

	/* Check by each SoC */
	soc_id = exynos_soc_info.product_id;
	revision = exynos_soc_info.revision;
	pr_info("SOC ID %X. Revision: %x\n", soc_id, revision);
	switch(soc_id) {
	case EXYNOS9810_SOC_ID:
		/* Check reset_sequencer_configuration register */
		exynos_pmu_read(RESET_SEQUENCER_CONFIGURATION, &reg_val);
		if (reg_val & DFD_EDPCSR_DUMP_EN) {
			little_reset_control(0);
			big_reset_control(0);
			dfd_set_dump_gpr(0);
		}
		if (revision < EXYNOS_MAIN_REV_1) {
			pr_emerg("%s: Exynos SoC reset right now with fake watchdog\n", __func__);
#ifdef CONFIG_S3C2410_WATCHDOG
//			s3c2410wdt_set_emergency_reset(1000, 1);
//			s3c2410wdt_reset_confirm(100, 1);
#endif
			while (1)
				wfi();
		}
		break;
	default:
		break;
	}

	/* Clear RST_STAT */
	exynos_pmu_write(RST_STAT, 0);
	/* Do S/W Reset */
	pr_emerg("%s: Exynos SoC reset right now\n", __func__);
	exynos_pmu_write(SWRESET, SWRESET_TRIGGER);
	while (1)
		wfi();
}

static int __init exynos_reboot_setup(struct device_node *np)
{
	int err = 0;
	u32 id;

	if (!of_property_read_u32(np, "pmu_base", &id)) {
		exynos_pmu_base = ioremap(id, SZ_16K);
		if (!exynos_pmu_base) {
			pr_err("%s: failed to map to exynos-pmu-base address 0x%x\n",
				__func__, id);
			err = -ENOMEM;
		}
	}

	of_node_put(np);

	pr_info("[Exynos Reboot]: Success to register arm_pm_restart\n");
	return err;
}

static const struct of_device_id reboot_of_match[] __initconst = {
	{ .compatible = "exynos,reboot", .data = exynos_reboot_setup},
	{},
};

typedef int (*reboot_initcall_t)(const struct device_node *);
static int __init exynos_reboot_init(void)
{
	struct device_node *np;
	const struct of_device_id *matched_np;
	reboot_initcall_t init_fn;

	np = of_find_matching_node_and_match(NULL, reboot_of_match, &matched_np);
	if (!np)
		return -ENODEV;

	arm_pm_restart = exynos_reboot;
#if !defined(CONFIG_SEC_REBOOT)
	pm_power_off = exynos_power_off;
#endif
	init_fn = (reboot_initcall_t)matched_np->data;

	return init_fn(np);
}
subsys_initcall(exynos_reboot_init);
