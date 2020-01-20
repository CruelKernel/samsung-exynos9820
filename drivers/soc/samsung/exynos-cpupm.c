/*
 * Copyright (c) 2018 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Exynos CPU Power Management driver implementation
 */

#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/tick.h>
#include <linux/psci.h>
#include <linux/cpuhotplug.h>
#include <linux/cpuidle_profiler.h>
#include <trace/events/power.h>

#include <soc/samsung/exynos-cpupm.h>
#include <soc/samsung/exynos-powermode.h>
#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-pmu.h>

#if defined(CONFIG_SEC_PM)
#if defined(CONFIG_MUIC_NOTIFIER) && defined(CONFIG_CCIC_NOTIFIER)
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#include <linux/ccic/ccic_notifier.h>
#endif /* CONFIG_MUIC_NOTIFIER && CONFIG_CCIC_NOTIFIER */
#endif /* CONFIG_SEC_PM */

#ifdef CONFIG_CPU_IDLE
/*
 * State of each cpu is managed by a structure declared by percpu, so there
 * is no need for protection for synchronization. However, when entering
 * the power mode, it is necessary to set the critical section to check the
 * state of cpus in the power domain, cpupm_lock is used for it.
 */
static spinlock_t cpupm_lock;

/******************************************************************************
 *                                  IDLE_IP                                   *
 ******************************************************************************/
#define PMU_IDLE_IP_BASE		0x03E0
#define PMU_IDLE_IP_MASK_BASE		0x03F0
#define PMU_IDLE_IP(x)			(PMU_IDLE_IP_BASE + (x * 0x4))
#define PMU_IDLE_IP_MASK(x)		(PMU_IDLE_IP_MASK_BASE + (x * 0x4))

#define IDLE_IP_REG_SIZE		32
#define NUM_IDLE_IP_REG			4

static int idle_ip_mask[NUM_IDLE_IP_REG];

#define IDLE_IP_MAX_INDEX		127
static int idle_ip_max_index = IDLE_IP_MAX_INDEX;

char *idle_ip_names[NUM_IDLE_IP_REG][IDLE_IP_REG_SIZE];

static int check_idle_ip(int reg_index)
{
	unsigned int val, mask;
	int ret;

	exynos_pmu_read(PMU_IDLE_IP(reg_index), &val);
	mask = idle_ip_mask[reg_index];

	ret = (val & ~mask) == ~mask ? 0 : -EBUSY;
	if (ret) {
		/*
		 * Profile non-idle IP using idle_ip.
		 * A bit of idle-ip equals 0, it means non-idle. But then, if
		 * same bit of idle-ip-mask is 1, PMU does not see this bit.
		 * To know what IP blocks to enter system power mode, suppose
		 * below example: (express only 8 bits)
		 *
		 * idle-ip  : 1 0 1 1 0 0 1 0
		 * mask     : 1 1 0 0 1 0 0 1
		 *
		 * First, clear masked idle-ip bit.
		 *
		 * idle-ip  : 1 0 1 1 0 0 1 0
		 * ~mask    : 0 0 1 1 0 1 1 0
		 * -------------------------- (AND)
		 * idle-ip' : 0 0 1 1 0 0 1 0
		 *
		 * In upper case, only idle-ip[2] is not in idle. Calculates
		 * as follows, then we can get the non-idle IP easily.
		 *
		 * idle-ip' : 0 0 1 1 0 0 1 0
		 * ~mask    : 0 0 1 1 0 1 1 0
		 *--------------------------- (XOR)
		 *            0 0 0 0 0 1 0 0
		 */
		cpuidle_profile_idle_ip(reg_index, ((val & ~mask) ^ ~mask));
	}

	return ret;
}

static void set_idle_ip_mask(void)
{
	int i;

	for (i = 0; i < NUM_IDLE_IP_REG; i++)
		exynos_pmu_write(PMU_IDLE_IP_MASK(i), idle_ip_mask[i]);
}

static void clear_idle_ip_mask(void)
{
	int i;

	for (i = 0; i < NUM_IDLE_IP_REG; i++)
		exynos_pmu_write(PMU_IDLE_IP_MASK(i), 0);
}

/**
 * There are 4 IDLE_IP registers in PMU, IDLE_IP therefore supports 128 index,
 * 0 from 127. To access the IDLE_IP register, convert_idle_ip_index() converts
 * idle_ip index to register index and bit in regster. For example, idle_ip index
 * 33 converts to IDLE_IP1[1]. convert_idle_ip_index() returns register index
 * and ships bit in register to *ip_index.
 */
static int convert_idle_ip_index(int *ip_index)
{
	int reg_index;

	reg_index = *ip_index / IDLE_IP_REG_SIZE;
	*ip_index = *ip_index % IDLE_IP_REG_SIZE;

	return reg_index;
}

static int idle_ip_index_available(int ip_index)
{
	struct device_node *dn = of_find_node_by_path("/cpupm/idle-ip");
	int proplen;
	int ref_idle_ip[IDLE_IP_MAX_INDEX];
	int i;

	proplen = of_property_count_u32_elems(dn, "idle-ip-mask");

	if (proplen <= 0)
		return false;

	if (!of_property_read_u32_array(dn, "idle-ip-mask",
					ref_idle_ip, proplen)) {
		for (i = 0; i < proplen; i++)
			if (ip_index == ref_idle_ip[i])
				return true;
	}

	return false;
}

static DEFINE_SPINLOCK(idle_ip_mask_lock);
static void unmask_idle_ip(int ip_index)
{
	int reg_index;
	unsigned long flags;

	if (!idle_ip_index_available(ip_index))
		return;

	reg_index = convert_idle_ip_index(&ip_index);

	spin_lock_irqsave(&idle_ip_mask_lock, flags);
	idle_ip_mask[reg_index] &= ~(0x1 << ip_index);
	spin_unlock_irqrestore(&idle_ip_mask_lock, flags);
}

int exynos_get_idle_ip_index(const char *ip_name)
{
	struct device_node *dn = of_find_node_by_path("/cpupm/idle-ip");
	int ip_index;

	ip_index = of_property_match_string(dn, "idle-ip-list", ip_name);
	if (ip_index < 0) {
		pr_err("%s: Fail to find %s in idle-ip list with err %d\n",
					__func__, ip_name, ip_index);
		return ip_index;
	}

	if (ip_index > idle_ip_max_index) {
		pr_err("%s: %s index %d is out of range\n",
					__func__, ip_name, ip_index);
		return -EINVAL;
	}

	/**
	 * If it successes to find IP in idle_ip list, we set
	 * corresponding bit in idle_ip mask.
	 */
	unmask_idle_ip(ip_index);

	return ip_index;
}

static DEFINE_SPINLOCK(ip_idle_lock);
void exynos_update_ip_idle_status(int ip_index, int idle)
{
	unsigned long flags;
	int reg_index;

	if (ip_index < 0 || ip_index > idle_ip_max_index)
		return;

	reg_index = convert_idle_ip_index(&ip_index);

	spin_lock_irqsave(&ip_idle_lock, flags);
	exynos_pmu_update(PMU_IDLE_IP(reg_index),
				1 << ip_index, idle << ip_index);
	spin_unlock_irqrestore(&ip_idle_lock, flags);

	return;
}

static void __init init_idle_ip_names(struct device_node *dn)
{
	int size;
	const char *list[IDLE_IP_MAX_INDEX];
	int i, bit, reg_index;

	size = of_property_count_strings(dn, "idle-ip-list");
	if (size < 0)
		return;

	of_property_read_string_array(dn, "idle-ip-list", list, size);
	for (i = 0, bit = 0; i < size; i++, bit = i) {
		reg_index = convert_idle_ip_index(&bit);
		idle_ip_names[reg_index][bit] = (char *)list[i];
	}

	size = of_property_count_strings(dn, "fix-idle-ip");
	if (size < 0)
		return;

	of_property_read_string_array(dn, "fix-idle-ip", list, size);
	for (i = 0; i < size; i++) {
		if (!of_property_read_u32_index(dn, "fix-idle-ip-index", i, &bit)) {
			reg_index = convert_idle_ip_index(&bit);
			idle_ip_names[reg_index][bit] = (char *)list[i];
		}
	}
}

static void __init idle_ip_init(void)
{
	struct device_node *dn = of_find_node_by_path("/cpupm/idle-ip");
	int index, count, i;

	for (i = 0; i < NUM_IDLE_IP_REG; i++)
		idle_ip_mask[i] = 0xFFFFFFFF;

	init_idle_ip_names(dn);

	/*
	 * To unmask fixed idle-ip, fix-idle-ip and fix-idle-ip-index,
	 * both properties must be existed and size must be same.
	 */
	if (!of_find_property(dn, "fix-idle-ip", NULL)
			|| !of_find_property(dn, "fix-idle-ip-index", NULL))
		return;

	count = of_property_count_strings(dn, "fix-idle-ip");
	if (count != of_property_count_u32_elems(dn, "fix-idle-ip-index")) {
		pr_err("Mismatch between fih-idle-ip and fix-idle-ip-index\n");
		return;
	}

	for (i = 0; i < count; i++) {
		of_property_read_u32_index(dn, "fix-idle-ip-index", i, &index);
		unmask_idle_ip(index);
	}

	idle_ip_max_index -= count;
}
#endif

/******************************************************************************
 *                                CAL interfaces                              *
 ******************************************************************************/
#ifdef CONFIG_EXYNOS_REBOOT
extern void big_reset_control(int en);
#else
static inline void big_reset_control(int en) { }
#endif

static void cpu_enable(unsigned int cpu)
{
	cal_cpu_enable(cpu);
}

static void cpu_disable(unsigned int cpu)
{
	cal_cpu_disable(cpu);
}

static DEFINE_PER_CPU(int, vcluster_id);
DEFINE_PER_CPU(int, clhead_cpu);
static void cluster_enable(unsigned int cpu_id)
{
	unsigned int cluster_id = per_cpu(vcluster_id, cpu_id);;
	cal_cluster_enable(cluster_id);
	big_reset_control(1);
}

static void cluster_disable(unsigned int cpu_id)
{
	unsigned int cluster_id = per_cpu(vcluster_id, cpu_id);
	big_reset_control(0);
	cal_cluster_disable(cluster_id);
}

/******************************************************************************
 *                               CPU HOTPLUG                                  *
 ******************************************************************************/
/* cpumask for indecating last cpu of a cluster */
struct cpumask cpuhp_last_mask;

bool exynos_cpuhp_last_cpu(unsigned int cpu)
{
	return cpumask_test_cpu(cpu, &cpuhp_last_mask);
}

static int cpuhp_cpupm_online(unsigned int cpu)
{
	struct cpumask mask;

	cpumask_and(&mask, cpu_cluster_mask(cpu), cpu_online_mask);
	if (cpumask_weight(&mask) == 0) {
		cluster_enable(cpu);
		/* clear cpus of this cluster from cpuhp_last_mask */
		cpumask_andnot(&cpuhp_last_mask,
			&cpuhp_last_mask, cpu_cluster_mask(cpu));
	}

	cpu_enable(cpu);

	return 0;
}

static int cpuhp_cpupm_offline(unsigned int cpu)
{
	struct cpumask online_mask, last_mask;

	cpu_disable(cpu);

#ifdef CONFIG_CPU_IDLE
	spin_lock(&cpupm_lock);
#endif
	cpumask_and(&online_mask, cpu_cluster_mask(cpu), cpu_online_mask);
	cpumask_and(&last_mask, cpu_cluster_mask(cpu), &cpuhp_last_mask);
	if ((cpumask_weight(&online_mask) == 0) && cpumask_empty(&last_mask)) {
		/* set cpu cpuhp_last_mask */
		cpumask_set_cpu(cpu, &cpuhp_last_mask);
		cluster_disable(cpu);
	}
#ifdef CONFIG_CPU_IDLE
	spin_unlock(&cpupm_lock);
#endif
	return 0;
}

#ifdef CONFIG_CPU_IDLE

/******************************************************************************
 *                            CPU idle management                             *
 ******************************************************************************/
#define IS_NULL(object)		(object == NULL)

/* freqvariant idle coefficient */
DECLARE_PER_CPU(struct freqvariant_idlefactor, fv_ifactor);

/*
 * State of CPUPM objects
 * All CPUPM objects have 2 states, RUN and POWERDOWN.
 *
 * @RUN
 * a state in which the power domain referred to by the object is turned on.
 *
 * @POWERDOWN
 * a state in which the power domain referred to by the object is turned off.
 * However, the power domain is not necessarily turned off even if the object
 * is in POEWRDOWN state because the cpu may be booting or executing power off
 * sequence.
 */
enum {
	CPUPM_STATE_RUN = 0,
	CPUPM_STATE_POWERDOWN,
};

/* Macros for CPUPM state */
#define set_state_run(object)		(object)->state = CPUPM_STATE_RUN
#define set_state_powerdown(object)	(object)->state = CPUPM_STATE_POWERDOWN
#define check_state_run(object)		((object)->state == CPUPM_STATE_RUN)
#define check_state_powerdown(object)	((object)->state == CPUPM_STATE_POWERDOWN)

/* Length of power mode name */
#define NAME_LEN	32

/*
 * Power modes
 * In CPUPM, power mode controls the power domain consisting of cpu and enters
 * the power mode by cpuidle. Basically, to enter power mode, all cpus in power
 * domain must be in POWERDOWN state, and sleep length of cpus must be smaller
 * than target_residency.
 */
struct power_mode {
	/* id of this power mode, it is used by cpuidle profiler now */
	int		id;

	/* name of power mode, it is declared in device tree */
	char		name[NAME_LEN];

	/* power mode state, RUN or POWERDOWN */
	int		state;

	/* sleep length criterion of cpus to enter power mode */
	int		target_residency;

	/* PSCI index for entering power mode */
	int		psci_index;

	/* type according to h/w configuration of power domain */
	int		type;

	/* cpus belonging to the power domain */
	struct cpumask	siblings;

	/*
	 * Among siblings, the cpus that can enter the power mode.
	 * Due to H/W constraint, only certain cpus need to enter power mode.
	 */
	struct cpumask	entry_allowed;

	/* disable count */
	atomic_t	disable;

	/*
	 * Some power modes can determine whether to enter power mode
	 * depending on system idle state
	 */
	bool		system_idle;

	/*
	 * kobject attribute for sysfs,
	 * it supports for enabling or disabling this power mode
	 */
	struct kobj_attribute	attr;

	/* user's request for enabling/disabling power mode */
	bool		user_request;
};

/* Maximum number of power modes manageable per cpu */
#define MAX_MODE	5

/*
 * Main struct of CPUPM
 * Each cpu has its own data structure and main purpose of this struct is to
 * manage the state of the cpu and the power modes containing the cpu.
 */
struct exynos_cpupm {
	/* cpu state, RUN or POWERDOWN */
	int			state;

	/* array to manage the power mode that contains the cpu */
	struct power_mode *	modes[MAX_MODE];
};

static DEFINE_PER_CPU(struct exynos_cpupm, cpupm);

/* Nop function to awake cpus */
static void do_nothing(void *unused)
{
}

static void awake_cpus(const struct cpumask *cpus)
{
	int cpu;

	for_each_cpu_and(cpu, cpus, cpu_online_mask)
		smp_call_function_single(cpu, do_nothing, NULL, 1);
}

/*
 * disable_power_mode/enable_power_mode
 * It provides "disable" function to enable/disable power mode as required by
 * user or device driver. To handle multiple disable requests, it use the
 * atomic disable count, and disable the mode that contains the given cpu.
 */
void disable_power_mode(int cpu, int type)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;
	unsigned int i;

	spin_lock(&cpupm_lock);
	pm = &per_cpu(cpupm, cpu);

	for (i = 0; i < MAX_MODE; i++) {
		mode = pm->modes[i];

		if (IS_NULL(mode))
			break;

		/*
		 * There are no entry allowed cpus, it means that mode is
		 * disabled, skip awaking cpus.
		 */
		if (cpumask_empty(&mode->entry_allowed))
			continue;

		if (mode->type == type) {
			/*
			 * The first mode disable request wakes the cpus to
			 * exit power mode
			 */
			if (atomic_inc_return(&mode->disable) > 0) {
				spin_unlock(&cpupm_lock);
				awake_cpus(&mode->siblings);
				return;
			}
		}
	}
	spin_unlock(&cpupm_lock);
}

void enable_power_mode(int cpu, int type)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;
	int i;

	spin_lock(&cpupm_lock);
	pm = &per_cpu(cpupm, cpu);

	for (i = 0; i < MAX_MODE; i++) {
		mode = pm->modes[i];
		if (IS_NULL(mode))
			break;

		if (mode->type == type) {
			atomic_dec(&mode->disable);
			spin_unlock(&cpupm_lock);
			awake_cpus(&mode->siblings);
			return;
		}
	}
	spin_unlock(&cpupm_lock);
}

/* get sleep length of given cpu from tickless framework */
static s64 get_sleep_length(int cpu)
{
	return ktime_to_us(ktime_sub(*(get_next_event_cpu(cpu)), ktime_get()));
}

static int cpus_busy(int target_residency, const struct cpumask *cpus)
{
	unsigned int cpu;
	int leader_cpu = per_cpu(clhead_cpu, cpumask_any(cpus));
	struct freqvariant_idlefactor *pfv_factor = &per_cpu(fv_ifactor, leader_cpu);
	unsigned int tmp_cur_freqvar_if;
	unsigned long flags;

	/* target residency for system-wide c-state (CPD/SICD) is
	 * re-evaluated in accordance with the current frequency variant idle factor (cur_freqvar_if)
	 */
	spin_lock_irqsave(&pfv_factor->freqvar_if_lock, flags);
	tmp_cur_freqvar_if = pfv_factor->cur_freqvar_if;
	spin_unlock_irqrestore(&pfv_factor->freqvar_if_lock, flags);

	target_residency = (target_residency * tmp_cur_freqvar_if) / 100;

	/*
	 * If there is even one cpu which is not in POWERDOWN state or has
	 * the smaller sleep length than target_residency, CPUPM regards
	 * it as BUSY.
	 */
	for_each_cpu_and(cpu, cpu_online_mask, cpus) {
		struct exynos_cpupm *pm = &per_cpu(cpupm, cpu);

		if (check_state_run(pm))
			return -EBUSY;

		if (get_sleep_length(cpu) < target_residency)
			return -EBUSY;
	}

	return 0;
}

static int system_busy(void)
{
	int i;

	for (i = 0; i < NUM_IDLE_IP_REG; i++)
		if (check_idle_ip(i))
			return 1;

	return 0;
}

/*
 * In order to enter the power mode, the following conditions must be met:
 * 1. power mode should not be disabled
 * 2. the cpu attempting to enter must be a cpu that is allowed to enter the
 *    power mode.
 * 3. all cpus in the power domain must be in POWERDOWN state and the sleep
 *    length of the cpus must be less than target_residency.
 */
static int can_enter_power_mode(int cpu, struct power_mode *mode)
{
	if (atomic_read(&mode->disable))
		return 0;

	if (!cpumask_test_cpu(cpu, &mode->entry_allowed))
		return 0;

	if (cpus_busy(mode->target_residency, &mode->siblings))
		return 0;

	if (is_IPI_pending(&mode->siblings))
		return 0;

	if (mode->system_idle && system_busy())
		return 0;

	return 1;
}

extern struct cpu_topology cpu_topology[NR_CPUS];

static int try_to_enter_power_mode(int cpu, struct power_mode *mode)
{
	int i;

	/* Check whether mode can be entered */
	if (!can_enter_power_mode(cpu, mode)) {
		/* fail to enter power mode */
		return 0;
	}

	/*
	 * From this point on, it has succeeded in entering the power mode.
	 * It prepares to enter power mode, and makes the corresponding
	 * setting according to type of power mode.
	 */
	switch (mode->type) {
	case POWERMODE_TYPE_CLUSTER:
		for_each_cpu(i, &mode->siblings)
			trace_cpupm_rcuidle(mode->psci_index, i);
		cluster_disable(cpu);
		break;
	case POWERMODE_TYPE_SYSTEM:
		if (unlikely(exynos_system_idle_enter()))
			return 0;

		set_idle_ip_mask();
		break;
	}

	dbg_snapshot_cpuidle(mode->name, 0, 0, DSS_FLAG_IN);
	set_state_powerdown(mode);

	cpuidle_profile_group_idle_enter(mode->id);

	return 1;
}

static void exit_mode(int cpu, struct power_mode *mode, int cancel)
{
	int i;

	cpuidle_profile_group_idle_exit(mode->id, cancel);

	/*
	 * Configure settings to exit power mode. This is executed by the
	 * first cpu exiting from power mode.
	 */
	set_state_run(mode);
	dbg_snapshot_cpuidle(mode->name, 0, 0, DSS_FLAG_OUT);

	switch (mode->type) {
	case POWERMODE_TYPE_CLUSTER:
		for_each_cpu(i, &mode->siblings)
			trace_cpupm_rcuidle(777, i);
		cluster_enable(cpu);
		break;
	case POWERMODE_TYPE_SYSTEM:
		exynos_system_idle_exit(cancel);
		clear_idle_ip_mask();
		break;
	}
}

/*
 * Exynos cpuidle driver call exynos_cpu_pm_enter() and exynos_cpu_pm_exit()
 * to handle platform specific configuration to control cpu power domain.
 */
int exynos_cpu_pm_enter(int cpu, int index)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;
	int i;

	spin_lock(&cpupm_lock);
	pm = &per_cpu(cpupm, cpu);

	/* Configure PMUCAL to power down core */
	cpu_disable(cpu);

	/* Set cpu state to POWERDOWN */
	set_state_powerdown(pm);

	/* Try to enter power mode */
	for (i = 0; i < MAX_MODE; i++) {
		mode = pm->modes[i];
		if (IS_NULL(mode))
			break;

		if (try_to_enter_power_mode(cpu, mode))
			index |= mode->psci_index;
	}

	spin_unlock(&cpupm_lock);

	return index;
}

void exynos_cpu_pm_exit(int cpu, int cancel)
{
	struct exynos_cpupm *pm;
	struct power_mode *mode;
	int i;

	spin_lock(&cpupm_lock);
	pm = &per_cpu(cpupm, cpu);

	/* Make settings to exit from mode */
	for (i = 0; i < MAX_MODE; i++) {
		mode = pm->modes[i];
		if (IS_NULL(mode))
			break;

		if (check_state_powerdown(mode))
			exit_mode(cpu, mode, cancel);
	}

	/* Set cpu state to RUN */
	set_state_run(pm);

	/* Configure PMUCAL to power up core */
	cpu_enable(cpu);

	spin_unlock(&cpupm_lock);
}

/******************************************************************************
 *                               sysfs interface                              *
 ******************************************************************************/
static ssize_t show_power_mode(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	struct power_mode *mode = container_of(attr, struct power_mode, attr);

	return sprintf(buf, "%s\n",
		atomic_read(&mode->disable) > 0 ? "disabled" : "enabled");
}

static ssize_t store_power_mode(struct kobject *kobj,
			struct kobj_attribute *attr, const char *buf,
			size_t count)
{
	unsigned int val;
	int cpu, type;
	struct power_mode *mode = container_of(attr, struct power_mode, attr);

	if (!sscanf(buf, "%u", &val))
		return -EINVAL;

	cpu = cpumask_any(&mode->siblings);
	type = mode->type;

	val = !!val;
	if (mode->user_request == val)
		return count;

	mode->user_request = val;
	if (val)
		enable_power_mode(cpu, type);
	else
		disable_power_mode(cpu, type);

	return count;
}

/*
 * attr_pool is used to create sysfs node at initialization time. Saving the
 * initiailized attr to attr_pool, and it creates nodes of each attr at the
 * time of sysfs creation. 10 is the appropriate value for the size of
 * attr_pool.
 */
static struct attribute *attr_pool[10];

static struct kobject *cpupm_kobj;
static struct attribute_group attr_group;

static void cpupm_sysfs_node_init(int attr_count)
{
	attr_group.attrs = kcalloc(attr_count + 1,
			sizeof(struct attribute *), GFP_KERNEL);
	if (!attr_group.attrs)
		return;

	memcpy(attr_group.attrs, attr_pool,
		sizeof(struct attribute *) * attr_count);

	cpupm_kobj = kobject_create_and_add("cpupm", power_kobj);
	if (!cpupm_kobj)
		goto out;

	if (sysfs_create_group(cpupm_kobj, &attr_group))
		goto out;

	return;

out:
	kfree(attr_group.attrs);
}

#define cpupm_attr_init(_attr, _name, _index)				\
	sysfs_attr_init(&_attr.attr);				\
	_attr.attr.name	= _name;				\
	_attr.attr.mode	= VERIFY_OCTAL_PERMISSIONS(0644);	\
	_attr.show	= show_power_mode;			\
	_attr.store	= store_power_mode;			\
	attr_pool[_index] = &_attr.attr;

/******************************************************************************
 *                                Initialization                              *
 ******************************************************************************/
static void __init
add_mode(struct power_mode **modes, struct power_mode *new)
{
	struct power_mode *mode;
	int i;

	for (i = 0; i < MAX_MODE; i++) {
		mode = modes[i];
		if (IS_NULL(mode)) {
			modes[i] = new;
			return;
		}
	}

	pr_warn("The number of modes exceeded\n");
}

static int __init cpu_power_mode_init(void)
{
	struct device_node *dn = NULL;
	struct power_mode *mode;
	const char *buf;
	int id = 0, attr_count = 0;

	while ((dn = of_find_node_by_type(dn, "cpupm"))) {
		int cpu;

		/*
		 * Power mode is dynamically generated according to what is defined
		 * in device tree.
		 */
		mode = kzalloc(sizeof(struct power_mode), GFP_KERNEL);
		if (!mode) {
			pr_warn("%s: No memory space!\n", __func__);
			continue;
		}

		mode->id = id++;
		strncpy(mode->name, dn->name, NAME_LEN - 1);

		of_property_read_u32(dn, "target-residency", &mode->target_residency);
		of_property_read_u32(dn, "psci-index", &mode->psci_index);
		of_property_read_u32(dn, "type", &mode->type);

		if (of_property_read_bool(dn, "system-idle"))
			mode->system_idle = true;

		if (!of_property_read_string(dn, "siblings", &buf)) {
			cpulist_parse(buf, &mode->siblings);
			cpumask_and(&mode->siblings, &mode->siblings, cpu_possible_mask);
		}

		if (!of_property_read_string(dn, "entry-allowed", &buf)) {
			cpulist_parse(buf, &mode->entry_allowed);
			cpumask_and(&mode->entry_allowed, &mode->entry_allowed, cpu_possible_mask);
		}

		atomic_set(&mode->disable, 0);

		/*
		 * The users' request is set to enable since initialization state of
		 * power mode is enabled.
		 */
		mode->user_request = true;

		/*
		 * Initialize attribute for sysfs.
		 * The absence of entry allowed cpu is equivalent to this power
		 * mode being disabled. In this case, no attribute is created.
		 */
		if (!cpumask_empty(&mode->entry_allowed)) {
			cpupm_attr_init(mode->attr, mode->name, attr_count);
			attr_count++;
		}

		/* Connect power mode to the cpus in the power domain */
		for_each_cpu(cpu, &mode->siblings)
			add_mode(per_cpu(cpupm, cpu).modes, mode);

		cpuidle_profile_group_idle_register(mode->id, mode->name);
	}

	if (attr_count)
		cpupm_sysfs_node_init(attr_count);

	dn = of_find_node_by_path("/cpupm/vcpu_topology");
	if (dn){
		int i, cluster_cnt = 0;
		of_property_read_u32(dn, "vcluster_cnt", &cluster_cnt);

		pr_info("%s:Virtual Cluster Info\n", __func__);
		for (i = 0 ; i < cluster_cnt ; i++) {
			int cpu;
			char name[20];
			struct cpumask sibling;
			snprintf(name, sizeof(name), "vcluster%d_sibling", i);
			if (!of_property_read_string(dn, name, &buf)) {
				cpulist_parse(buf, &sibling);
				cpumask_and(&sibling, &sibling, cpu_possible_mask);

				pr_info("Cluster%d : ", i);
				for_each_cpu(cpu, &sibling) {
					per_cpu(vcluster_id, cpu) = i;
					per_cpu(clhead_cpu, cpu) = cpumask_first(&sibling);
					pr_info("%d ", cpu);
				}
				pr_info("\n");
			}
		}
	}else {
		pr_info("No Virtual Cluster Info\n");
	}

	return 0;
}

static int __init exynos_cpupm_init(void)
{
	cpu_power_mode_init();

	spin_lock_init(&cpupm_lock);

	idle_ip_init();

	return 0;
}
arch_initcall(exynos_cpupm_init);
#endif

static int cpuhp_cpupm_enable_idle(unsigned int cpu)
{
	struct cpuidle_device *dev = per_cpu(cpuidle_devices, cpu);
	cpuidle_enable_device(dev);

	return 0;
}
static int cpuhp_cpupm_disable_idle(unsigned int cpu)
{
	struct cpuidle_device *dev = per_cpu(cpuidle_devices, cpu);
	cpuidle_disable_device(dev);

	return 0;
}

static int __init exynos_cpupm_early_init(void)
{
	cpuhp_setup_state(CPUHP_AP_EXYNOS_IDLE_CTRL,
				"AP_EXYNOS_IDLE_CTROL",
				cpuhp_cpupm_enable_idle,
				cpuhp_cpupm_disable_idle);

	cpuhp_setup_state(CPUHP_AP_EXYNOS_CPU_UP_POWER_CONTROL,
				"AP_EXYNOS_CPU_UP_POWER_CONTROL",
				cpuhp_cpupm_online, NULL);
	cpuhp_setup_state(CPUHP_AP_EXYNOS_CPU_DOWN_POWER_CONTROL,
				"AP_EXYNOS_CPU_DOWN_POWER_CONTROL",
				NULL, cpuhp_cpupm_offline);

	return 0;
}
early_initcall(exynos_cpupm_early_init);

#if defined(CONFIG_SEC_PM)
#if defined(CONFIG_MUIC_NOTIFIER) && defined(CONFIG_CCIC_NOTIFIER)
struct notifier_block cpuidle_muic_nb;

static int exynos_cpupm_muic_notifier(struct notifier_block *nb,
				unsigned long action, void *data)
{
	CC_NOTI_ATTACH_TYPEDEF *pnoti = (CC_NOTI_ATTACH_TYPEDEF *)data;
	muic_attached_dev_t attached_dev = pnoti->cable_type;
	bool jig_is_attached = false;

	switch (attached_dev) {
	case ATTACHED_DEV_JIG_UART_OFF_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_OTG_MUIC:
	case ATTACHED_DEV_JIG_UART_OFF_VB_FG_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_MUIC:
	case ATTACHED_DEV_JIG_UART_ON_VB_MUIC:
		if (action == MUIC_NOTIFY_CMD_DETACH) {
			jig_is_attached = false;
			enable_power_mode(0, POWERMODE_TYPE_SYSTEM);
		} else if (action == MUIC_NOTIFY_CMD_ATTACH) {
			jig_is_attached = true;
			disable_power_mode(0, POWERMODE_TYPE_SYSTEM);
		} else {
			pr_err("%s: ACTION Error!(%lu)\n", __func__, action);
		}

		pr_info("%s: JIG(%d) is %s\n", __func__, attached_dev,
				jig_is_attached ? "attached" : "detached");
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static int __init exynos_cpupm_muic_notifier_init(void)
{
	return muic_notifier_register(&cpuidle_muic_nb,
			exynos_cpupm_muic_notifier, MUIC_NOTIFY_DEV_CPUIDLE);
}
late_initcall(exynos_cpupm_muic_notifier_init);
#endif /* CONFIG_MUIC_NOTIFIER && CONFIG_CCIC_NOTIFIER */
#endif /* CONFIG_SEC_PM */
