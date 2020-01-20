/*
 * linux/arch/arm/mach-exynos/exynos-coresight.c
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/smp.h>
#include <linux/of.h>
#include <linux/suspend.h>
#include <linux/smpboot.h>
#include <linux/delay.h>
#include <linux/debug-snapshot.h>

#include <asm/core_regs.h>
#include <asm/io.h>
#include <asm/smp_plat.h>
#include "coresight-priv.h"

#define CHANNEL		(0)
#define PORT		(1)
#define NONE		(-1)
#define ARR_SZ		(2)

#define etm_writel(base, val, off)	__raw_writel((val), base + off)
#define etm_readl(base, off)		__raw_readl(base + off)

#define SOFT_LOCK(base)				\
do { mb(); isb(); etm_writel(base, 0x0, LAR); } while (0)

#define SOFT_UNLOCK(base)			\
do { etm_writel(base, OSLOCK_MAGIC, LAR); mb(); isb(); } while (0)

struct cpu_etm_info {
	void __iomem	*base;
	u32		enabled;
	u32		f_port[ARR_SZ];
};

struct funnel_info {
	void __iomem	*base;
	u32		port_status;
	u32		f_port[ARR_SZ];
};
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
struct etf_info {
	void __iomem	*base;
	u32		f_port[ARR_SZ];
};
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
struct etr_info {
	void __iomem	*base;
	void __iomem	*sfr_base;
	u32		enabled;
	u32		buf_addr;
	u32		buf_size;
	u32		buf_pointer;
	u32		q_offset;
	bool		hwacg;
};
#endif
struct exynos_trace_info {
	struct cpu_etm_info	cpu[NR_CPUS];
	spinlock_t		trace_lock;
	u32			enabled;

	u32			procsel;
	u32			config;
	u32			sync_period;
	u32			victlr;

	u32			funnel_num;
	struct funnel_info	*funnel;
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	u32			etf_num;
	struct etf_info		*etf;
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	struct etr_info		etr;
#endif
};

static struct exynos_trace_info *g_trace_info;
#ifdef CONFIG_EXYNOS_CORESIGHT_ETB
static void __iomem *g_etb_base;
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_STM
static u32 stm_funnel_port;
#endif
static DEFINE_PER_CPU(struct task_struct *, etm_task);

static void exynos_funnel_init(void)
{
	unsigned int i, port, channel;
	struct funnel_info *funnel;

	for (i = 0; i < g_trace_info->funnel_num; i++) {
		funnel = &g_trace_info->funnel[i];
		spin_lock(&g_trace_info->trace_lock);
		SOFT_UNLOCK(funnel->base);
		funnel->port_status = etm_readl(funnel->base, FUNCTRL);
		funnel->port_status = (funnel->port_status & 0x3ff) | 0x300;
		etm_writel(funnel->base, funnel->port_status, FUNCTRL);
		etm_writel(funnel->base, 0x0, FUNPRIORCTRL);
		SOFT_LOCK(funnel->base);
		spin_unlock(&g_trace_info->trace_lock);

		if (funnel->f_port[CHANNEL] != NONE) {
			channel = funnel->f_port[CHANNEL];
			port = funnel->f_port[PORT];
			funnel = &g_trace_info->funnel[channel];

			spin_lock(&g_trace_info->trace_lock);
			SOFT_UNLOCK(funnel->base);
			funnel->port_status = etm_readl(funnel->base, FUNCTRL);
			funnel->port_status |= BIT(port);
			etm_writel(funnel->base, funnel->port_status, FUNCTRL);
			SOFT_LOCK(funnel->base);
			spin_unlock(&g_trace_info->trace_lock);
		}
	}
}

static void exynos_funnel_close(void)
{
	unsigned int i;
	struct funnel_info *funnel;

	for (i = 0; i < g_trace_info->funnel_num; i++) {
		funnel = &g_trace_info->funnel[i];
		spin_lock(&g_trace_info->trace_lock);
		SOFT_UNLOCK(funnel->base);
		etm_writel(funnel->base, 0x300, FUNCTRL);
		SOFT_LOCK(funnel->base);
		spin_unlock(&g_trace_info->trace_lock);
	}
}

#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
static void exynos_etf_enable(void)
{
	unsigned int i, port, channel;
	struct etf_info *etf;
	struct funnel_info *funnel;

	for (i = 0; i < g_trace_info->etf_num; i++) {
		etf = &g_trace_info->etf[i];
		SOFT_UNLOCK(etf->base);
		etm_writel(etf->base, 0x0, TMCCTL);
		etm_writel(etf->base, 0x800, TMCRSZ);
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
		etm_writel(etf->base, 0x2, TMCMODE);
#else
		etm_writel(etf->base, 0x0, TMCMODE);
#endif
		etm_writel(etf->base, 0x0, TMCTGR);
		etm_writel(etf->base, 0x0, TMCFFCR);
		etm_writel(etf->base, 0x1, TMCCTL);
		SOFT_LOCK(etf->base);

		if (etf->f_port[CHANNEL] != NONE) {
			channel = etf->f_port[CHANNEL];
			port = etf->f_port[PORT];
			funnel = &g_trace_info->funnel[channel];

			spin_lock(&g_trace_info->trace_lock);
			SOFT_UNLOCK(funnel->base);
			funnel->port_status = etm_readl(funnel->base, FUNCTRL);
			funnel->port_status |= BIT(port);
			etm_writel(funnel->base, funnel->port_status, FUNCTRL);
			SOFT_LOCK(funnel->base);
			spin_unlock(&g_trace_info->trace_lock);
		}
	}
}

static void exynos_etf_disable(void)
{
	unsigned int i, port, channel;
	struct etf_info *etf;
	struct funnel_info *funnel;

	for (i = 0; i < g_trace_info->etf_num; i++) {
		etf = &g_trace_info->etf[i];
		SOFT_UNLOCK(etf->base);
		etm_writel(etf->base, 0x0, TMCCTL);
		SOFT_LOCK(etf->base);

		if (etf->f_port[CHANNEL] != NONE) {
			channel = etf->f_port[CHANNEL];
			port = etf->f_port[PORT];
			funnel = &g_trace_info->funnel[channel];

			spin_lock(&g_trace_info->trace_lock);
			SOFT_UNLOCK(funnel->base);
			funnel->port_status = etm_readl(funnel->base, FUNCTRL);
			funnel->port_status &= ~BIT(port);
			etm_writel(funnel->base, funnel->port_status, FUNCTRL);
			SOFT_LOCK(funnel->base);
			spin_unlock(&g_trace_info->trace_lock);
		}
	}
}
#endif

#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
static void exynos_etr_enable(void)
{
	struct etr_info *etr = &g_trace_info->etr;

	if (etr->hwacg)
		writel_relaxed(0x1, etr->sfr_base + etr->q_offset);

	SOFT_UNLOCK(etr->base);
	etm_writel(etr->base, 0x0, TMCCTL);
	etm_writel(etr->base, etr->buf_size, TMCRSZ);
	etm_writel(etr->base, 0x4, TMCTGR);
	etm_writel(etr->base, 0x0, TMCAXICTL);
	etm_writel(etr->base, etr->buf_addr, TMCDBALO);
	etm_writel(etr->base, 0x0, TMCDBAHI);
	etm_writel(etr->base, etr->buf_pointer, TMCRWP);
	etm_writel(etr->base, 0x0, TMCMODE);
	etm_writel(etr->base, 0x2001, TMCFFCR);
	etm_writel(etr->base, 0x1, TMCCTL);
	SOFT_LOCK(etr->base);
}

static void exynos_etr_disable(void)
{
	struct etr_info *etr = &g_trace_info->etr;

	SOFT_UNLOCK(etr->base);
	etm_writel(etr->base, 0x0, TMCCTL);
	etr->buf_pointer = etm_readl(etr->base, TMCRWP);
	SOFT_LOCK(etr->base);

	if (etr->hwacg)
		writel_relaxed(0x0, etr->sfr_base + etr->q_offset);
}
#endif

#ifdef CONFIG_EXYNOS_CORESIGHT_ETB
static void exynos_etb_enable(void __iomem *etb_base, int src)
{
	int i;
	unsigned int depth = etm_readl(etb_base, TMCRSZ);

	SOFT_UNLOCK(etb_base);
	etm_writel(etb_base, 0x0, TMCCTL);
	etm_writel(etb_base, 0x0, TMCRWP);

	/* clear entire RAM buffer */
	for (i = 0; i < depth; i++)
		etm_writel(etb_base, 0x0, TMCRWD);

	/* reset write RAM pointer address */
	etm_writel(etb_base, 0x0, TMCRWP);
	/* reset read RAM pointer address */
	etm_writel(etb_base, 0x0, TMCRRP);
	etm_writel(etb_base, 0x1, TMCTGR);

	if (src) {
		etm_writel(etb_base, 0x0, TMCFFCR);
		pr_info("Data formatter disabled!\n");
	} else {
		etm_writel(etb_base, 0x2001, TMCFFCR);
		pr_info("Data formatter enabled!\n");
	}

	/* ETB trace capture enable */
	etm_writel(etb_base, 0x1, TMCCTL);
	SOFT_LOCK(etb_base);
}

static void exynos_etb_disable(void __iomem *etb_base, int src)
{
	uint32_t ffcr;

	SOFT_UNLOCK(etb_base);
	if (src) {
		etm_writel(etb_base, 0x2001, TMCFFCR);
		pr_info("Data formatter enabled!\n");
	} else {
		etm_writel(etb_base, 0x0, TMCFFCR);
		pr_info("Data formatter disabled!\n");
	}

	ffcr = etm_readl(etb_base, TMCFFCR);
	ffcr |= BIT(6);
	etm_writel(etb_base, ffcr, TMCFFCR);

	udelay(1500);
	etm_writel(etb_base, 0x0, TMCCTL);

	udelay(1500);
	SOFT_LOCK(etb_base);
}

extern void exynos_etb_etm(void)
{
	struct funnel_info *funnel;
	unsigned int channel, port;

	exynos_etb_disable(g_etb_base, 0);

	if (stm_funnel_port[CHANNEL] != NONE) {
		channel = stm_funnel_port[CHANNEL];
		port = stm_funnel_port[PORT];
		funnel = &g_trace_info->funnel[channel];

		spin_lock(&g_trace_info->trace_lock);
		SOFT_UNLOCK(funnel->base);
		funnel->port_status = etm_readl(funnel->base, FUNCTRL);
		funnel->port_status &= ~BIT(port);
		etm_writel(funnel->base, funnel->port_status, FUNCTRL);
		SOFT_LOCK(funnel->base);
		spin_unlock(&g_trace_info->trace_lock);
	}
	exynos_etb_enable(g_etb_base, 0);
}

extern void exynos_etb_stm(void)
{
	struct funnel_info *funnel;
	unsigned int channel, port;

	exynos_etb_disable(g_etb_base, 1);

	if (stm_funnel_port[CHANNEL] != NONE) {
		channel = stm_funnel_port[CHANNEL];
		port = stm_funnel_port[PORT];
		funnel = &g_trace_info->funnel[channel];

		spin_lock(&g_trace_info->trace_lock);
		SOFT_UNLOCK(funnel->base);
		funnel->port_status = etm_readl(funnel->base, FUNCTRL);
		funnel->port_status |= BIT(port);
		etm_writel(funnel->base, funnel->port_status, FUNCTRL);
		SOFT_LOCK(funnel->base);
		spin_unlock(&g_trace_info->trace_lock);
	}
	exynos_etb_enable(g_etb_base, 1);
}
#endif

static int etm_info_init(void)
{
	/* Main control and Configuration */
	spin_lock_init(&g_trace_info->trace_lock);

	g_trace_info->funnel_num = 0;
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	g_trace_info->etf_num = 0;
#endif
	g_trace_info->procsel = 0;
	g_trace_info->config = 0;
	g_trace_info->sync_period = 0x8;
	g_trace_info->victlr = 0x0;
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	g_trace_info->etr.buf_addr = dbg_snapshot_get_item_paddr("log_etm");
	if (!g_trace_info->etr.buf_addr)
		return -ENOMEM;
	g_trace_info->etr.buf_size = dbg_snapshot_get_item_size("log_etm") / 4;
	if (!g_trace_info->etr.buf_size)
		return -ENOMEM;
	g_trace_info->etr.buf_pointer = 0;

#endif
	return 0;
}

static void etm_enable(unsigned int cpu)
{
	struct cpu_etm_info *cinfo = &g_trace_info->cpu[cpu];
	struct funnel_info *funnel;
	unsigned int channel, port;

	SOFT_UNLOCK(cinfo->base);
	etm_writel(cinfo->base, 0x1, ETMOSLAR);
	etm_writel(cinfo->base, 0x0, ETMCTLR);

	/* Main control and Configuration */
	etm_writel(cinfo->base, g_trace_info->procsel, ETMPROCSELR);
	etm_writel(cinfo->base, g_trace_info->config, ETMCONFIG);
	etm_writel(cinfo->base, g_trace_info->sync_period, ETMSYNCPR);

	etm_writel(cinfo->base, cpu+1, ETMTRACEIDR);

	/* additional register setting */
	etm_writel(cinfo->base, 0x1000, ETMEVENTCTL0R);
	etm_writel(cinfo->base, 0x0, ETMEVENTCTL1R);
	etm_writel(cinfo->base, 0xc, ETMSTALLCTLR);
	etm_writel(cinfo->base, 0x801, ETMCONFIG);
	etm_writel(cinfo->base, 0x0, ETMTSCTLR);
	etm_writel(cinfo->base, 0x4, ETMCCCCTLR);

	etm_writel(cinfo->base, 0x201, ETMVICTLR);
	etm_writel(cinfo->base, 0x0, ETMVIIECTLR);
	etm_writel(cinfo->base, 0x0, ETMVISSCTLR);
	etm_writel(cinfo->base, 0x2, ETMAUXCTLR);

	etm_writel(cinfo->base, 0x1, ETMCTLR);
	etm_writel(cinfo->base, 0x0, ETMOSLAR);

	channel = cinfo->f_port[CHANNEL];
	port = cinfo->f_port[PORT];
	funnel = &g_trace_info->funnel[channel];

	spin_lock(&g_trace_info->trace_lock);
	cinfo->enabled = 1;
	SOFT_UNLOCK(funnel->base);
	funnel->port_status = etm_readl(funnel->base, FUNCTRL);
	funnel->port_status |= BIT(port);
	etm_writel(funnel->base, funnel->port_status, FUNCTRL);
	SOFT_LOCK(funnel->base);
	spin_unlock(&g_trace_info->trace_lock);
}

static void etm_disable(unsigned int cpu)
{
	struct cpu_etm_info *cinfo = &g_trace_info->cpu[cpu];
	struct funnel_info *funnel;
	unsigned int channel, port;

	channel = cinfo->f_port[CHANNEL];
	port = cinfo->f_port[PORT];
	funnel = &g_trace_info->funnel[channel];

	spin_lock(&g_trace_info->trace_lock);
	cinfo->enabled = 0;
	SOFT_UNLOCK(funnel->base);
	funnel->port_status = etm_readl(funnel->base, FUNCTRL);
	funnel->port_status &= ~BIT(port);
	etm_writel(funnel->base, funnel->port_status, FUNCTRL);
	SOFT_LOCK(funnel->base);
	spin_unlock(&g_trace_info->trace_lock);

	SOFT_UNLOCK(cinfo->base);
	etm_writel(cinfo->base, 0x0, ETMCTLR);
	etm_writel(cinfo->base, 0x1, ETMOSLAR);
}

extern void exynos_trace_start(void)
{
	g_trace_info->enabled = 1;

	exynos_funnel_init();
#ifdef CONFIG_EXYNOS_CORESIGHT_ETB
	exynos_etb_enable(g_etb_base, 0);
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	exynos_etf_enable();
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	exynos_etr_enable();
#endif
	pr_info("coresight: %s.\n", __func__);
}

extern void exynos_trace_stop(void)
{
	if (!g_trace_info->enabled)
		return;

	exynos_funnel_close();
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	exynos_etf_disable();
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	exynos_etr_disable();
#endif
	etm_disable(raw_smp_processor_id());

	g_trace_info->enabled = 0;
	pr_info("coresight: %s.\n", __func__);
}

static int exynos_etm_dying_cpu(unsigned int cpu)
{
	etm_disable(cpu);

	return 0;
}

static int exynos_c2_etm_pm_notifier(struct notifier_block *self,
						unsigned long action, void *v)
{
	int cpu = raw_smp_processor_id();

	switch (action) {
	case CPU_PM_ENTER:
		etm_disable(cpu);
		break;
        case CPU_PM_ENTER_FAILED:
        case CPU_PM_EXIT:
		etm_enable(cpu);
		break;
	case CPU_CLUSTER_PM_ENTER:
		break;
	case CPU_CLUSTER_PM_ENTER_FAILED:
	case CPU_CLUSTER_PM_EXIT:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block exynos_c2_etm_pm_nb = {
	.notifier_call = exynos_c2_etm_pm_notifier,
};

static int exynos_etm_pm_notifier(struct notifier_block *notifier,
						unsigned long pm_event, void *v)
{
	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		exynos_trace_stop();
		break;
	case PM_POST_SUSPEND:
		exynos_trace_start();
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block exynos_etm_pm_nb = {
	.notifier_call = exynos_etm_pm_notifier,
};

#ifdef CONFIG_OF
static const struct of_device_id etm_dt_match[] = {
	{ .compatible = "exynos,coresight",},
};
#endif

static int exynos_cs_etm_init_dt(void)
{
	struct device_node *cs_np, *np = NULL;
	unsigned int offset, cs_reg_base;
	int i = 0;
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	int ret;
	u32 reg[2];
#endif


	g_trace_info = kzalloc(sizeof(struct exynos_trace_info), GFP_KERNEL);
	if (!g_trace_info)
		return -ENOMEM;

	if (etm_info_init())
		return -ENOMEM;

	cs_np = of_find_matching_node(NULL, etm_dt_match);

	if (of_property_read_u32(cs_np, "funnel-num", &g_trace_info->funnel_num))
		return -EINVAL;
	g_trace_info->funnel = kzalloc(sizeof(struct funnel_info) *
				g_trace_info->funnel_num, GFP_KERNEL);

#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	if (of_property_read_u32(cs_np, "etf-num", &g_trace_info->etf_num))
		return -EINVAL;
	g_trace_info->etf = kzalloc(sizeof(struct etf_info) *
				g_trace_info->etf_num, GFP_KERNEL);
#endif

	if (of_property_read_u32(cs_np, "base", &cs_reg_base))
		return -EINVAL;
	while ((np = of_find_node_by_type(np, "cs"))) {
		if (of_property_read_u32(np, "etm-offset", &offset))
			return -EINVAL;

		g_trace_info->cpu[i].base = ioremap(cs_reg_base + offset, SZ_4K);
		if (!g_trace_info->cpu[i].base)
			return -ENOMEM;
		if (of_property_read_u32_array(np, "funnel-port",
				g_trace_info->cpu[i].f_port, 2))
			g_trace_info->cpu[i].f_port[CHANNEL] = NONE;
		i++;
	}
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	i = 0;
	while ((np = of_find_node_by_type(np, "etf"))) {
		if (of_property_read_u32(np, "offset", &offset))
			return -EINVAL;

		g_trace_info->etf[i].base = ioremap(cs_reg_base + offset, SZ_4K);
		if (!g_trace_info->etf[i].base)
			return -ENOMEM;
		if (of_property_read_u32_array(np, "funnel-port",
				g_trace_info->etf[i].f_port, 2))
			g_trace_info->etf[i].f_port[CHANNEL] = NONE;
		i++;
	}
#endif
	i = 0;
	while ((np = of_find_node_by_type(np, "funnel"))) {
		if (of_property_read_u32(np, "offset", &offset))
			return -EINVAL;
		g_trace_info->funnel[i].base = ioremap(cs_reg_base + offset, SZ_4K);

		if (!g_trace_info->funnel[i].base)
			return -ENOMEM;
		if (of_property_read_u32_array(np, "funnel-port",
				g_trace_info->funnel[i].f_port, 2))
			g_trace_info->funnel[i].f_port[CHANNEL] = NONE;
		i++;
	}
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	if (!(np = of_find_node_by_type(np, "etr")))
		return -EINVAL;
	if (of_property_read_u32(np, "offset", &offset))
		return -EINVAL;
	g_trace_info->etr.base = ioremap(cs_reg_base + offset, SZ_4K);
	if (!g_trace_info->etr.base)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "samsung,cs-sfr", reg, 2);
	if (!ret) {
		g_trace_info->etr.sfr_base = ioremap(reg[0], reg[1]);
		if (!g_trace_info->etr.sfr_base)
			return -EINVAL;
		of_property_read_u32(np, "samsung,q-offset", &offset);
		g_trace_info->etr.q_offset = offset;
		g_trace_info->etr.hwacg = true;
	}
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETB
	if (!(np = of_find_node_by_type(np, "etb")))
		return -EINVAL;
	if (of_property_read_u32(np, "offset", &offset))
			return -EINVAL;
	g_etb_base = ioremap(cs_reg_base + cs_offset, SZ_4K);
	if (!g_etb_base)
		return -ENOMEM;
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_STM
	if (!(np = of_find_node_by_type(np, "stm")))
		return -EINVAL;
	if (of_property_read_u32_array(np, "funnel-port",
			&stm_funnel_port, 2))
		stm_funnel_port[CHANNEL] = NONE;
#endif
	return 0;
}

static void etm_hotplug_out(unsigned int cpu)
{
	etm_disable(cpu);
}

static void etm_hotplug_in(unsigned int cpu)
{
	etm_enable(cpu);
}

static int etm_should_run(unsigned int cpu) { return 0; }

static void etm_thread_fn(unsigned int cpu) { }

static struct smp_hotplug_thread etm_threads = {
	.store				= &etm_task,
	.thread_should_run	= etm_should_run,
	.thread_fn			= etm_thread_fn,
	.thread_comm		= "etm/%u",
	.setup				= etm_hotplug_in,
	.park				= etm_hotplug_out,
	.unpark				= etm_hotplug_in,
};

static int __init exynos_etm_init(void)
{
	int ret;

	ret = exynos_cs_etm_init_dt();
	if (ret < 0)
		goto err;

	ret = smpboot_register_percpu_thread(&etm_threads);
	if (ret < 0)
		goto err;

	register_pm_notifier(&exynos_etm_pm_nb);
	cpuhp_setup_state(CPUHP_AP_ETM_DYING, "CPUHP_AP_ETM_DYING",
				NULL, exynos_etm_dying_cpu);
	cpu_pm_register_notifier(&exynos_c2_etm_pm_nb);

	g_trace_info->enabled = 1;
	pr_info("coresight: ETM enable.\n");
	return 0;
err:
	g_trace_info->enabled = 0;
	pr_err("coresight: ETM enable FAILED!!! : ret = %d\n", ret);
	return ret;
}
early_initcall(exynos_etm_init);

static int __init exynos_tmc_init(void)
{
	if (!g_trace_info->enabled) {
		pr_err("coresight TMC init FAILED!!!\n");
		return -ENODEV;
	}
	exynos_trace_start();
	return 0;
}
postcore_initcall(exynos_tmc_init);

#ifdef CONFIG_EXYNOS_CORESIGHT_ETM_SYSFS
static struct bus_type etm_subsys = {
	.name = "exynos-etm",
	.dev_name = "exynos-etm",
};

static ssize_t etm_show_all_status(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct cpu_etm_info *cinfo;
	struct funnel_info *funnel;
	struct etf_info *etf;
	unsigned long tmp, port_status, read_p;
	int i, channel, port, size = 0;

	size += scnprintf(buf + size, 15,"ETM Status\n");
	size += scnprintf(buf + size, 80,
			"-------------------------------------------------------------\n");
	size += scnprintf(buf + size, 80, " %-8s | %-10s | %-14s | %-4s | %-11s\n",
			"Core Num", "ETM status", "Funnel_channel", "Port", "Port Status");
	for (i = 0; i < NR_CPUS; i++) {
		cinfo = &g_trace_info->cpu[i];
		channel = cinfo->f_port[CHANNEL];
		port = cinfo->f_port[PORT];
		funnel = &g_trace_info->funnel[channel];
		spin_lock(&g_trace_info->trace_lock);
		port_status = (funnel->port_status >> port) & 0x1;
		spin_unlock(&g_trace_info->trace_lock);
		size += scnprintf(buf + size, 80,
				" %-8d | %10s | %-14u | %-4d | %-11s\n",
				i, cinfo->enabled ? "enabled" : "disabled", channel, port,
				port_status ? "open" : "close");
	}
	size += scnprintf(buf + size, 80,
			"-------------------------------------------------------------\n");
	for (i = 0; i < g_trace_info->funnel_num; i++) {
		funnel = &g_trace_info->funnel[i];
		SOFT_UNLOCK(funnel->base);
		tmp = etm_readl(funnel->base, FUNCTRL);
		SOFT_LOCK(funnel->base);
		size += scnprintf(buf + size, 30, "FUNNEL%d Status : 0x%lx\n", i, tmp);
	}
#ifdef CONFIG_EXYNOS_CORESIGHT_ETF
	for (i = 0; i < g_trace_info->etf_num; i++) {
		etf = &g_trace_info->etf[i];
		SOFT_UNLOCK(etf->base);
		tmp = etm_readl(etf->base, TMCCTL);
		read_p = etm_readl(etf->base, TMCRWP);
		SOFT_LOCK(etf->base);
		size += scnprintf(buf + size, 30, "ETF%d Status : %3sabled\n",
				i, tmp & 0x1 ? "en" : "dis");
		size += scnprintf(buf + size, 30, "ETF%d RWP Reg : 0x%lx\n", i, read_p);
	}
#endif
#ifdef CONFIG_EXYNOS_CORESIGHT_ETR
	SOFT_UNLOCK(g_trace_info->etr.base);
	tmp = etm_readl(g_trace_info->etr.base, TMCCTL);
	read_p = etm_readl(g_trace_info->etr.base, TMCRWP);
	SOFT_LOCK(g_trace_info->etr.base);
	size += scnprintf(buf + size, 30, "ETR Status : %3sabled\n",
			tmp & 0x1 ? "en" : "dis");
	size += scnprintf(buf + size, 30, "ETR RWP Reg : 0x%lx\n", read_p);
	size += scnprintf(buf + size, 30, "ETR save RWP : 0x%x\n\n",
			g_trace_info->etr.buf_pointer);
#endif
	return size;
}

static struct kobj_attribute etm_enable_attr =
	__ATTR(etm_status, 0644, etm_show_all_status, NULL);

static struct attribute *etm_sysfs_attrs[] = {
	&etm_enable_attr.attr,
	NULL,
};

static struct attribute_group etm_sysfs_group = {
	.attrs = etm_sysfs_attrs,
};

static const struct attribute_group *etm_sysfs_groups[] = {
	&etm_sysfs_group,
	NULL,
};

static int __init exynos_etm_sysfs_init(void)
{
	int ret = 0;

	ret = subsys_system_register(&etm_subsys, etm_sysfs_groups);
	if (ret)
		pr_err("fail to register exynos-etm subsys\n");

	return ret;
}
late_initcall(exynos_etm_sysfs_init);
#endif
