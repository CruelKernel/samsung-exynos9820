/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - CPU PMU(Power Management Unit) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/smp.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <soc/samsung/exynos-pmu.h>

/**
 * "pmureg" has the mapped base address of PMU(Power Management Unit)
 */
static struct regmap *pmureg;
#ifdef CONFIG_SOC_EXYNOS9820
static void __iomem *pmu_alive;
static spinlock_t update_lock;
#endif

#ifdef CONFIG_SOC_EXYNOS9820
/* Atomic operation for PMU_ALIVE registers. (offset 0~0x3FFF)
   When the targer register can be accessed by multiple masters,
   This functions should be used. */
static inline void exynos_pmu_set_bit_atomic(unsigned int offset, unsigned int val)
{
	__raw_writel(val, pmu_alive + (offset | 0xc000));
}

static inline void exynos_pmu_clr_bit_atomic(unsigned int offset, unsigned int val)
{
	__raw_writel(val, pmu_alive + (offset | 0x8000));
}
#endif

/**
 * No driver refers the "pmureg" directly, through the only exported API.
 */
int exynos_pmu_read(unsigned int offset, unsigned int *val)
{
	return regmap_read(pmureg, offset, val);
}

int exynos_pmu_write(unsigned int offset, unsigned int val)
{
	return regmap_write(pmureg, offset, val);
}

int exynos_pmu_update(unsigned int offset, unsigned int mask, unsigned int val)
{
#ifdef CONFIG_SOC_EXYNOS9820
	int i;
	unsigned long flags;

	if (offset > 0x3fff) {
		return regmap_update_bits(pmureg, offset, mask, val);
	} else {
		spin_lock_irqsave(&update_lock, flags);
		for (i = 0; i < 32; i++) {
			if (mask & (1 << i)) {
				if (val & (1 << i))
					exynos_pmu_set_bit_atomic(offset, i);
				else
					exynos_pmu_clr_bit_atomic(offset, i);
			}
		}
		spin_unlock_irqrestore(&update_lock, flags);
		return 0;
	}
#else
	return regmap_update_bits(pmureg, offset, mask, val);
#endif
}

struct regmap *exynos_get_pmu_regmap(void)
{
	return pmureg;
}

EXPORT_SYMBOL_GPL(exynos_get_pmu_regmap);
EXPORT_SYMBOL(exynos_pmu_read);
EXPORT_SYMBOL(exynos_pmu_write);
EXPORT_SYMBOL(exynos_pmu_update);

#define PMU_CPU_CONFIG_BASE			0x1000
#define PMU_CPU_STATUS_BASE			0x1004
#define CPU_LOCAL_PWR_CFG			0x1

static int pmu_cpu_offset(unsigned int cpu)
{
	unsigned int offset = 0;

	switch (cpu) {
	case 0:
		offset = 0x0;
		break;
	case 1:
		offset = 0x80;
		break;
	case 2:
		offset = 0x100;
		break;
	case 3:
		offset = 0x180;
		break;
	case 4:
		offset = 0x300;
		break;
	case 5:
		offset = 0x380;
		break;
	case 6:
		offset = 0x500;
		break;
	case 7:
		offset = 0x580;
		break;
	default:
		BUG();
		break;
	}
	return offset;
}

static void pmu_cpu_ctrl(unsigned int cpu, int enable)
{
	unsigned int offset;

	offset = pmu_cpu_offset(cpu);
	regmap_update_bits(pmureg, PMU_CPU_CONFIG_BASE + offset,
			CPU_LOCAL_PWR_CFG,
			enable ? CPU_LOCAL_PWR_CFG : 0);
}

static int pmu_cpu_state(unsigned int cpu)
{
	unsigned int offset, val = 0;

	offset = pmu_cpu_offset(cpu);
	regmap_read(pmureg, PMU_CPU_STATUS_BASE + offset, &val);

	return ((val & CPU_LOCAL_PWR_CFG) == CPU_LOCAL_PWR_CFG);
}

#define CLUSTER_ADDR_OFFSET			0x8
#define PMU_NONCPU_CONFIG_BASE			0x2040
#define PMU_NONCPU_STATUS_BASE			0x2044
#define PMU_MEMORY_CLUSTER1_NONCPU_STATUS	0x2380
#define MEMORY_CLUSTER_ADDR_OFFSET		0x21C
#define NONCPU_LOCAL_PWR_CFG			0xF
#define SHARED_CACHE_LOCAL_PWR_CFG		0x1

static void pmu_cluster_ctrl(unsigned int cpu, int enable)
{
	unsigned int offset;

	offset = phy_cluster(cpu) * CLUSTER_ADDR_OFFSET;
	regmap_update_bits(pmureg,
			PMU_NONCPU_CONFIG_BASE + offset,
			NONCPU_LOCAL_PWR_CFG,
			enable ? NONCPU_LOCAL_PWR_CFG : 0);
}

static bool pmu_noncpu_state(unsigned int cpu)
{
	unsigned int noncpu_stat = 0;
	unsigned int offset;

	offset = phy_cluster(cpu) * CLUSTER_ADDR_OFFSET;
	regmap_read(pmureg,
		PMU_NONCPU_STATUS_BASE + offset, &noncpu_stat);

	return !!(noncpu_stat & NONCPU_LOCAL_PWR_CFG);
}

static int pmu_shared_cache_state(unsigned int cpu)
{
	unsigned int shared_stat = 0;
	unsigned int offset;

	offset = phy_cluster(cpu) * MEMORY_CLUSTER_ADDR_OFFSET;

	regmap_read(pmureg,
		PMU_MEMORY_CLUSTER1_NONCPU_STATUS + offset, &shared_stat);

	return (shared_stat & SHARED_CACHE_LOCAL_PWR_CFG);
}

static void exynos_cpu_up(unsigned int cpu)
{
	pmu_cpu_ctrl(cpu, 1);
}

static void exynos_cpu_down(unsigned int cpu)
{
	pmu_cpu_ctrl(cpu, 0);
}

static int exynos_cpu_state(unsigned int cpu)
{
	return pmu_cpu_state(cpu);
}

static void exynos_cluster_up(unsigned int cpu)
{
	pmu_cluster_ctrl(cpu, false);
}

static void exynos_cluster_down(unsigned int cpu)
{
	pmu_cluster_ctrl(cpu, true);
}

static int exynos_cluster_state(unsigned int cpu)
{
	return pmu_shared_cache_state(cpu) &&
			pmu_noncpu_state(cpu);
}

struct exynos_cpu_power_ops exynos_cpu = {
	.power_up = exynos_cpu_up,
	.power_down = exynos_cpu_down,
	.power_state = exynos_cpu_state,
	.cluster_up = exynos_cluster_up,
	.cluster_down = exynos_cluster_down,
	.cluster_state = exynos_cluster_state,
};

#define PMU_CP_STAT		0x0038
int exynos_check_cp_status(void)
{
	unsigned int val;

	exynos_pmu_read(PMU_CP_STAT, &val);

	return val;
}

static struct bus_type exynos_info_subsys = {
	.name = "exynos_info",
	.dev_name = "exynos_info",
};

#define NR_CPUS_PER_CLUSTER		4
static ssize_t core_status_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	ssize_t n = 0;
	int cpu;

	for_each_possible_cpu(cpu) {
		/*
		 * Each cluster has four cores.
		 * "cpu % NR_CPUS_PER_CLUSTER == 0" means that
		 * the cpu is a first one of each cluster.
		 */
		if (!(cpu % NR_CPUS_PER_CLUSTER)) {
			n += scnprintf(buf + n, 26, "%s shared_cache : %d\n",
				(!cpu) ? "boot" : "nonboot",
				pmu_shared_cache_state(cpu));

			n += scnprintf(buf + n, 24, "%s Noncpu : %d\n",
				(!cpu) ? "boot" : "nonboot",
				pmu_noncpu_state(cpu));
		}
		n += scnprintf(buf + n, 24, "CPU%d : %d\n",
				cpu, pmu_cpu_state(cpu));
	}

	return n;
}

static struct kobj_attribute cs_attr =
	__ATTR(core_status, 0644, core_status_show, NULL);

static struct attribute *cs_sysfs_attrs[] = {
	&cs_attr.attr,
	NULL,
};

static struct attribute_group cs_sysfs_group = {
	.attrs = cs_sysfs_attrs,
};

static const struct attribute_group *cs_sysfs_groups[] = {
	&cs_sysfs_group,
	NULL,
};

static int exynos_pmu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
#ifdef CONFIG_SOC_EXYNOS9820
	struct resource *res;
#endif

	pmureg = syscon_regmap_lookup_by_phandle(dev->of_node,
						"samsung,syscon-phandle");
	if (IS_ERR(pmureg)) {
		pr_err("Fail to get regmap of PMU\n");
		return PTR_ERR(pmureg);
	}

#ifdef CONFIG_SOC_EXYNOS9820
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pmu_alive");
	pmu_alive = devm_ioremap_resource(dev, res);
	if (IS_ERR(pmu_alive)) {
		pr_err("Failed to get address of PMU_ALIVE\n");
		return PTR_ERR(pmu_alive);
	}
	spin_lock_init(&update_lock);
#endif

	if (subsys_system_register(&exynos_info_subsys,
					cs_sysfs_groups))
		pr_err("Fail to register exynos_info subsys\n");

	return 0;
}

static const struct of_device_id of_exynos_pmu_match[] = {
	{ .compatible = "samsung,exynos-pmu", },
	{ },
};

static const struct platform_device_id exynos_pmu_ids[] = {
	{ "exynos-pmu", },
	{ }
};

static struct platform_driver exynos_pmu_driver = {
	.driver = {
		.name = "exynos-pmu",
		.owner = THIS_MODULE,
		.of_match_table = of_exynos_pmu_match,
	},
	.probe		= exynos_pmu_probe,
	.id_table	= exynos_pmu_ids,
};

int __init exynos_pmu_init(void)
{
	return platform_driver_register(&exynos_pmu_driver);
}
subsys_initcall(exynos_pmu_init);
