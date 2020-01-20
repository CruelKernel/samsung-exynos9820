/*
 * drivers/soc/samsung/exynos-topology.c
 *
 * Copyright (C) 2018 Samsung Electronics.
 *
 * Based on the arm64 version written by Mark Brown in turn based on
 * arch/arm64/kernel/topology.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/arch_topology.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/sched/topology.h>
#include <linux/sched/energy.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <asm/cpu.h>
#include <asm/cputype.h>
#include <asm/topology.h>

struct cpu_energy_level {
	int core;
	int coregroup;
	int cluster;
} cpu_energy_level[NR_CPUS];

static int __init get_cpu_for_node(struct device_node *node)
{
	struct device_node *cpu_node;
	int cpu;

	cpu_node = of_parse_phandle(node, "cpu", 0);
	if (!cpu_node)
		return -1;

	for_each_possible_cpu(cpu) {
		if (of_get_cpu_node(cpu, NULL) == cpu_node) {
			topology_parse_cpu_capacity(cpu_node, cpu);
			of_node_put(cpu_node);
			return cpu;
		}
	}

	pr_crit("Unable to find CPU node for %pOF\n", cpu_node);

	of_node_put(cpu_node);
	return -1;
}

static int __init parse_core(struct device_node *core, int cluster_id, int cluster_elvl,
					int coregroup_id, int coregroup_elvl, int core_id)
{
	int cpu;
	int core_elvl = 0;

	cpu = get_cpu_for_node(core);
	if (cpu < 0) {
		pr_err("%pOF: Can't get CPU for leaf core\n", core);
		return 0;
	}

	cpu_topology[cpu].cluster_id = cluster_id;
	cpu_topology[cpu].coregroup_id = coregroup_id;
	cpu_topology[cpu].core_id = core_id;

	if (coregroup_elvl != -1)
		coregroup_elvl++;

	if (cluster_elvl != -1)
		cluster_elvl++;

	cpu_energy_level[cpu].core = core_elvl;
	cpu_energy_level[cpu].coregroup = coregroup_elvl;
	cpu_energy_level[cpu].cluster = cluster_elvl;

	return 0;
}

static int __init parse_coregroup(struct device_node *coregroup, int cluster_id, int cluster_elvl,
						int coregroup_id)
{
	char name[10];
	bool has_cores = false;
	struct device_node *c;
	int core_id;
	int coregroup_elvl = 0;
	int ret;

	if (cluster_elvl != -1)
		cluster_elvl++;

	core_id = 0;
	do {
		snprintf(name, sizeof(name), "core%d", core_id);
		c = of_get_child_by_name(coregroup, name);
		if (c) {
			has_cores = true;
			ret = parse_core(c, cluster_id, cluster_elvl,
					coregroup_id, coregroup_elvl, core_id++);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
	} while (c);

	if (!has_cores)
		pr_warn("%pOF: empty coregroup\n", coregroup);

	return 0;
}

static int __init parse_cluster(struct device_node *cluster, int cluster_id)
{
	char name[20];
	bool leaf = true;
	bool has_cores = false;
	struct device_node *c;
	int coregroup_id, core_id;
	int cluster_elvl = 0;
	int ret;

	/* First check for child coregroup */
	coregroup_id = 0;
	do {
		snprintf(name, sizeof(name), "coregroup%d", coregroup_id);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			leaf = false;
			ret = parse_coregroup(c, cluster_id, cluster_elvl,
					coregroup_id++);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
	} while (c);

	/* Cluster shouldn't have child coregroup and core simultaneously */
	if (!leaf)
		return 0;

	/* If cluster doesn't have coregroup, check for cores */
	core_id = 0;
	do {
		snprintf(name, sizeof(name), "core%d", core_id);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			has_cores = true;
			ret = parse_core(c, cluster_id, cluster_elvl,
					coregroup_id, -1, core_id++);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
	} while (c);

	if (!has_cores)
		pr_warn("%pOF: empty cluster\n", cluster);

	return 0;
}

static int __init parse_dt_topology(void)
{
	char name[10];
	bool has_cluster = false;
	struct device_node *map, *cluster;
	int cluster_id;
	int cpu;
	int ret;

	/*
	 * When topology is provided cpu-map is essentially a root
	 * cluster with restricted subnodes.
	 */
	map = of_find_node_by_path("/cpus/cpu-map");
	if (!map) {
		pr_err("No CPU information found in DT\n");
		return 0;
	}

	init_sched_energy_costs();

	cluster_id = 0;
	do {
		snprintf(name, sizeof(name), "cluster%d", cluster_id);
		cluster = of_get_child_by_name(map, name);
		if (cluster) {
			has_cluster = true;
			ret = parse_cluster(cluster, cluster_id++);
			of_node_put(cluster);
			if (ret != 0)
				goto out_map;
		}
	} while (cluster);

	if (!has_cluster) {
		pr_err("%pOF: cpu-map children should be clusters\n", map);
		ret = -EINVAL;
		goto out_map;
	}

	topology_normalize_cpu_scale();

	/*
	 * Check that all cores are in the topology; the SMP code will
	 * only mark cores described in the DT as possible.
	 */
	for_each_possible_cpu(cpu)
		if (cpu_topology[cpu].cluster_id == -1)
			ret = -EINVAL;

out_map:
	of_node_put(map);
	return ret;
}

/*
 * cpu topology table
 */
struct cpu_topology cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return &cpu_topology[cpu].core_sibling;
}

const struct cpumask *cpu_cluster_mask(int cpu)
{
	return &cpu_topology[cpu].cluster_sibling;
}

static void update_siblings_masks(unsigned int cpuid)
{
	struct cpu_topology *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];
	int cpu;

	/* update core and thread sibling masks */
	for_each_possible_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->cluster_id != cpu_topo->cluster_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->cluster_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->cluster_sibling);

		if (cpuid_topo->coregroup_id != cpu_topo->coregroup_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->core_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->core_sibling);

		if (cpuid_topo->core_id != cpu_topo->core_id)
			continue;

		cpumask_set_cpu(cpuid, &cpu_topo->thread_sibling);
		if (cpu != cpuid)
			cpumask_set_cpu(cpu, &cpuid_topo->thread_sibling);
	}
}

void store_cpu_topology(unsigned int cpuid)
{
	struct cpu_topology *cpuid_topo = &cpu_topology[cpuid];

	if (cpuid_topo->cluster_id == -1) {
		pr_err("CPU topology isn't composed properly\n");
		BUG_ON(cpuid_topo->cluster_id);
	}

	pr_debug("CPU%u: cluster %d coregroup %d core %d\n", cpuid,
			cpuid_topo->cluster_id, cpuid_topo->coregroup_id, cpuid_topo->core_id);

	update_siblings_masks(cpuid);
	topology_detect_flags();
}

static int core_flags(void)
{
	return cpu_core_flags() | topology_core_flags();
}

static int cluster_flags(void)
{
	return cpu_core_flags() | topology_cluster_flags();
}

static int cpu_flags(void)
{
	return topology_cpu_flags();
}

#ifdef CONFIG_SIMPLIFIED_ENERGY_MODEL
#define use_simplified	1
#else
#define use_simplified	0
#endif

static inline
const struct sched_group_energy * const cpu_core_energy(int cpu)
{
	struct sched_group_energy *sge;
	unsigned long capacity;
	int max_cap_idx;
	int level = cpu_energy_level[cpu].core;

	if (use_simplified)
		return NULL;

	if (level < 0)
		return NULL;

	sge = sge_array[cpu][level];
	if (!sge) {
		pr_warn("Invalid sched_group_energy for CPU%d\n", cpu);
		return NULL;
	}

	max_cap_idx = sge->nr_cap_states - 1;
	capacity = sge->cap_states[max_cap_idx].cap;

	printk_deferred("cpu=%d set cpu scale %lu from energy model\n",
			cpu, capacity);

	topology_set_cpu_scale(cpu, capacity);

	return sge;
}

static inline
const struct sched_group_energy * const cpu_coregroup_energy(int cpu)
{
	struct sched_group_energy *sge;
	int level = cpu_energy_level[cpu].coregroup;

	if (use_simplified)
		return NULL;

	if (level < 0)
		return NULL;

	sge = sge_array[cpu][level];
	if (!sge) {
		pr_warn("Invalid sched_group_energy for Coregroup%d\n", cpu);
		return NULL;
	}

	return sge;
}

static inline
const struct sched_group_energy * const cpu_cluster_energy(int cpu)
{
	struct sched_group_energy *sge;
	int level = cpu_energy_level[cpu].cluster;

	if (use_simplified)
		return NULL;

	if (level < 0)
		return NULL;

	sge = sge_array[cpu][level];
	if (!sge) {
		pr_warn("Invalid sched_group_energy for Cluster%d\n", cpu);
		return NULL;
	}

	return sge;
}

static struct sched_domain_topology_level exynos_topology[NR_SD_LEVELS];

#ifdef CONFIG_SCHED_DEBUG
#define sd_init_name(topology, type)	topology.name = #type
#else
#define sd_init_name(topology, type)
#endif

static void __init build_sched_topology(void)
{
	struct cpu_topology *cpuid_topo, *cpu_topo = &cpu_topology[0];
	bool cluster_level = false;
	bool coregroup_level = false;
	bool core_level = false;
	int cpu;
	int level = 0;

	for_each_possible_cpu(cpu) {
		cpuid_topo = &cpu_topology[cpu];

		if (cpuid_topo->cluster_id != cpu_topo->cluster_id)
			cluster_level = true;
		if (cpuid_topo->coregroup_id != cpu_topo->coregroup_id)
			coregroup_level = true;
		if (cpuid_topo->core_id != cpu_topo->core_id)
			core_level = true;
	}

	if (core_level) {
		exynos_topology[level].mask = cpu_coregroup_mask;
		exynos_topology[level].sd_flags = core_flags;
		exynos_topology[level].energy = cpu_core_energy;
		sd_init_name(exynos_topology[level], MC);

		level++;
	}
	if (coregroup_level) {
		exynos_topology[level].mask = cpu_cluster_mask;
		exynos_topology[level].sd_flags = cluster_flags;
		exynos_topology[level].energy = cpu_coregroup_energy;
		sd_init_name(exynos_topology[level], DSU);

		level++;
	}
	if (cluster_level) {
		exynos_topology[level].mask = cpu_cpu_mask;
		exynos_topology[level].sd_flags = cpu_flags;
		exynos_topology[level].energy = cpu_cluster_energy;
		sd_init_name(exynos_topology[level], DIE);

		level++;
	}
	exynos_topology[level].mask = NULL;
}

static void __init reset_cpu_topology(void)
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];
		struct cpu_energy_level *cpu_elvl = &cpu_energy_level[cpu];

		cpu_topo->core_id = 0;
		cpu_topo->coregroup_id = -1;
		cpu_topo->cluster_id = -1;

		cpumask_clear(&cpu_topo->cluster_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->cluster_sibling);
		cpumask_clear(&cpu_topo->core_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->core_sibling);
		cpumask_clear(&cpu_topo->thread_sibling);
		cpumask_set_cpu(cpu, &cpu_topo->thread_sibling);

		cpu_elvl->core = -1;
		cpu_elvl->coregroup = -1;
		cpu_elvl->cluster = -1;
	}
}

void __init init_cpu_topology(void)
{
	reset_cpu_topology();

	/*
	 * Discard anything that was parsed if we hit an error so we
	 * don't use partial information.
	 */
	if (of_have_populated_dt() && parse_dt_topology())
		reset_cpu_topology();
	else {
		build_sched_topology();
		set_sched_topology(exynos_topology);
	}
}
