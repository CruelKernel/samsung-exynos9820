/*
 * 2019.7 Expanded to CPUIDLE per cpufreq
 * by Jungwook Kim <jwook1.kim@samsung.com>
 * Based on CPUIDLE profiler for Exynos
 * Copyright (c) 2018 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/cpuidle.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>

/* whether profiling has started */
static bool profile_started;

/*
 * Represents statistic of idle state.
 * All idle states are mapped 1:1 with cpuidle_stats.
 */
struct cpuidle_stats {
	/* time to enter idle state */
	ktime_t			idle_entry_time;

	/* number of times an idle state is entered */
	unsigned int		entry_count;

	/* number of times the entry into idle state is canceled */
	unsigned int		cancel_count;

	/* time in idle state */
	unsigned long long	time;
};

/* description length of idle state */
#define DESC_LEN	32

/*
 * Manages idle state where cpu enters individually. One cpu_idle_state
 * structure manages a idle state for each cpu to enter, and the number
 * of structure is determined by cpuidle driver.
 */
#define MAX_FREQ	30
#define MAX_CLUSTER 3

struct cpu_idle_state {
	/* description of idle state */
	char 			desc[DESC_LEN];

	/* idle state statstics for each cpu */
	struct cpuidle_stats	stats[NR_CPUS][MAX_FREQ];
	unsigned int		enter_freqs[NR_CPUS];
};

/* cpu idle state list and length of cpu idle state list */
static struct cpu_idle_state *cpu_idle_state;
static int cpu_idle_state_count;
static const char *prefix = "exynos_perf";
static unsigned int cpufreq_list[MAX_CLUSTER][MAX_FREQ];
static int cluster_last_cpu[MAX_CLUSTER];
static int cluster_first_cpu[MAX_CLUSTER];

/************************************************************************
 *                              Profiling                               *
 ************************************************************************/
static void idle_enter(struct cpuidle_stats *stats)
{
	stats->idle_entry_time = ktime_get();
	stats->entry_count++;
}

static void idle_exit(struct cpuidle_stats *stats, int cancel)
{
	s64 diff;

	/*
	 * If profiler is started with cpu already in idle state,
	 * idle_entry_time is 0 because entry event is not recorded.
	 * From the start of the profile to cpu wakeup is the idle time,
	 * but ignore this because it is complex to handle it and the
	 * time is not large.
	 */
	if (!stats->idle_entry_time)
		return;

	if (cancel) {
		stats->cancel_count++;
		return;
	}

	diff = ktime_to_us(ktime_sub(ktime_get(), stats->idle_entry_time));
	stats->time += diff;

	stats->idle_entry_time = 0;
}

static int get_cluster_index(int cpu)
{
	int index = 0;
	int i;
	for (i = 0; i < MAX_CLUSTER; i++) {
		if (cpu <= cluster_last_cpu[i]) {
			index = i;
			break;
		}
	}
	return index;
}

/*
 * cpuidle_profile_cpu_idle_enter/cpuidle_profile_cpu_idle_exit
 * : profilie for cpu idle state
 */
void exynos_perf_cpu_idle_enter(int cpu, int index)
{
	int freq_index = 0;
	int cluster_index;
	uint cpufreq;
	int i;

	if (!profile_started)
		return;

	cpufreq = (uint)cpufreq_quick_get(cpu);
	cluster_index = get_cluster_index(cpu);
	for (i = 0; i < MAX_FREQ; i++) {
		if (cpufreq == cpufreq_list[cluster_index][i]) {
			freq_index = i;
			break;
		}
	}

	cpu_idle_state[index].enter_freqs[cpu] = cpufreq;

	idle_enter(&cpu_idle_state[index].stats[cpu][freq_index]);
}

void exynos_perf_cpu_idle_exit(int cpu, int index, int cancel)
{
	int freq_index = 0;
	int cluster_index;
	uint cpufreq;
	int i;

	if (!profile_started)
		return;

	cpufreq = cpu_idle_state[index].enter_freqs[cpu];
	cluster_index = get_cluster_index(cpu);
	for (i = 0; i < MAX_FREQ; i++) {
		if (cpufreq == cpufreq_list[cluster_index][i]) {
			freq_index = i;
			break;
		}
	}

	idle_exit(&cpu_idle_state[index].stats[cpu][freq_index], cancel);
}

struct cpufreq_stats {
	unsigned int total_trans;
	unsigned long long last_time;
	unsigned int max_state;
	unsigned int state_num;
	unsigned int last_index;
	u64 *time_in_state;
	unsigned int *freq_table;
	unsigned int *trans_table;
};

static int refill_cpufreq_list(void)
{
	struct cpufreq_policy *policy;
	struct cpumask *mask;
	struct cpufreq_stats *stats;
	int cluster_count = 0;
	int next_cpu = 0;
	int i, cpu, last_cpu;

	memset(cpufreq_list, 0, sizeof(unsigned int)*MAX_CLUSTER*MAX_FREQ);

	while ( next_cpu < num_possible_cpus() ) {
		policy = cpufreq_cpu_get(next_cpu);
		if (!policy) {
			next_cpu++;
			continue;
		}
		mask = policy->related_cpus;
		for_each_cpu(cpu, mask) {
			last_cpu = cpu;
		}
		stats = policy->stats;
		for (i = 0; i < stats->state_num; i++)
			cpufreq_list[cluster_count][i] = stats->freq_table[i];

		cluster_first_cpu[cluster_count] = next_cpu;
		cluster_last_cpu[cluster_count] = last_cpu;
		cluster_count++;

		next_cpu = last_cpu + 1;
	}

	return 0;
}


/************************************************************************
 *                          Profile start/stop                          *
 ************************************************************************/
/* totoal profiling time */
static s64 profile_time;

/* start time of profile */
static ktime_t profile_start_time;

static void clear_stats(struct cpuidle_stats *stats)
{
	if (!stats)
		return;

	stats->idle_entry_time = 0;

	stats->entry_count = 0;
	stats->cancel_count = 0;
	stats->time = 0;
}

static void reset_profile(void)
{
	int cpu, i, f;

	profile_start_time = 0;

	for (i = 0; i < cpu_idle_state_count; i++)
		for_each_possible_cpu(cpu)
			for (f = 0; f < MAX_FREQ; f++)
				clear_stats(&cpu_idle_state[i].stats[cpu][f]);

	/* refill cpufreq list */
	refill_cpufreq_list();
}

void exynos_perf_cpuidle_start(void)
{
	if (profile_started) {
		pr_err("[%s] cpuidle profile is ongoing\n", prefix);
		return;
	}

	reset_profile();
	profile_start_time = ktime_get();

	profile_started = 1;

	pr_info("cpuidle profile start\n");
}

void exynos_perf_cpuidle_stop(void)
{
	if (!profile_started) {
		pr_err("[%s] CPUIDLE profile does not start yet\n", prefix);
		return;
	}

	pr_info("[%s] cpuidle profile stop\n", prefix);

	profile_started = 0;

	profile_time = ktime_to_us(ktime_sub(ktime_get(), profile_start_time));
}

/************************************************************************
 *                              Show result                             *
 ************************************************************************/
static int calculate_percent(s64 residency)
{
	if (!residency)
		return 0;

	residency *= 100;
	do_div(residency, profile_time);

	return residency;
}

static unsigned long long cpu_idle_time(int cpu)
{
	unsigned long long idle_time = 0;
	int i, f;
	int cluster_index;

	cluster_index = get_cluster_index(cpu);
	for (i = 0; i < cpu_idle_state_count; i++)
		for (f = 0; f < MAX_FREQ; f++) {
			if (cpufreq_list[cluster_index][f] == 0)
				break;
			idle_time += cpu_idle_state[i].stats[cpu][f].time;
		}

	return idle_time;
}

static int cpu_idle_ratio(int cpu)
{
	return calculate_percent(cpu_idle_time(cpu));
}

static ssize_t show_result(char *buf)
{
	int ret = 0;
	int cpu, i;
	int cluster_index = 0;
	struct cpuidle_stats *stats;
	int freq, freq_value;

	if (profile_started) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
				"CPUIDLE profile is ongoing\n");
		return ret;
	}

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
		"Profiling Time : %lluus\n", profile_time);

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
		"\n");

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
		"[total idle ratio]\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
		"#cpu      #time    #ratio\n");
	for_each_possible_cpu(cpu)
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"cpu%d %10lluus   %3u%%\n",
			cpu, cpu_idle_time(cpu), cpu_idle_ratio(cpu));

	ret += snprintf(buf + ret, PAGE_SIZE - ret,
		"\n");

	/*
	 * Example of cpu idle state profile result.
	 * Below is an example from the quad core architecture. The number of
	 * rows depends on the number of cpu.
	 *
	 * [state : {desc}]
	 * #cpu   #time
	 * cpu0  8808916us
	 */
	for (i = 0; i < cpu_idle_state_count; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"[state : %s]\n", cpu_idle_state[i].desc);

		for_each_possible_cpu(cpu) {

			cluster_index = get_cluster_index(cpu);

			if (cpu == cluster_first_cpu[cluster_index]) {
				/* header: cpufreq */
				ret += snprintf(buf + ret, PAGE_SIZE - ret, "#freq ", cpu);
				for (freq = 0; freq < MAX_FREQ; freq++) {
					freq_value = cpufreq_list[cluster_index][freq];
					if (freq_value == 0) {
						ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
						break;
					}
					ret += snprintf(buf + ret, PAGE_SIZE - ret, "%u ", freq_value);
				}
			}

			ret += snprintf(buf + ret, PAGE_SIZE - ret, "cpu%d ", cpu);
			for (freq = 0; freq < MAX_FREQ; freq++) {
				freq_value = cpufreq_list[cluster_index][freq];
				if (freq_value == 0) {
					ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
					break;
				}
				stats = &cpu_idle_state[i].stats[cpu][freq];
				ret += snprintf(buf + ret, PAGE_SIZE - ret, "%llu ", stats->time);
			}
		}

		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}

	return ret;
}

/*********************************************************************
 *                          Sysfs interface                          *
 *********************************************************************/
static int run_seq_show(struct seq_file *file, void *iter)
{
	int ret = 0;
	static char buf[PAGE_SIZE];

	if (profile_started)
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "Not finished!");
	else
		ret += show_result(buf);
	buf[ret] = '\0';
	seq_printf(file, "%s", buf);

	return 0;
}

static ssize_t run_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	int run;
	char buf[10];
	count = (count > 10)? 10 : count;
	if (copy_from_user(buf, buffer, count) != 0)
		return -EFAULT;

	if (!sscanf(buf, "%1d", &run))
		return -EINVAL;
	if (run)
		exynos_perf_cpuidle_start();
	else
		exynos_perf_cpuidle_stop();

	return count;
}

static int run_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, run_seq_show, inode->i_private);
}

static struct file_operations run_debugfs_fops;

/* freq */
static int freq_seq_show(struct seq_file *file, void *iter)
{
	int i, j;
	int ret = 0;
	static char buf[PAGE_SIZE];

	refill_cpufreq_list();

	for (i = 0; i < MAX_CLUSTER; i++) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "cluster[%d] = ", i);
		for (j = 0; j < MAX_FREQ; j++)
			ret += snprintf(buf + ret, PAGE_SIZE - ret, "%u ", cpufreq_list[i][j]);
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "\n");
	}
	buf[ret] = '\0';
	seq_printf(file, "%s", buf);

	return 0;
}

static ssize_t freq_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	return count;
}

static int freq_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, freq_seq_show, inode->i_private);
}

static struct file_operations freq_debugfs_fops;


/*********************************************************************
 *                   Initialize cpuidle profiler                     *
 *********************************************************************/
void __init
exynos_perf_cpu_idle_register(struct cpuidle_driver *drv)
{
	struct cpu_idle_state *state;
	int state_count = drv->state_count;
	int i;

	state = kzalloc(sizeof(struct cpu_idle_state) * state_count,
							GFP_KERNEL);
	if (!state) {
		pr_err("%s: Failed to allocate memory\n", __func__);
		return;
	}

	for (i = 0; i < state_count; i++)
		strncpy(state[i].desc, drv->states[i].desc, DESC_LEN - 1);

	cpu_idle_state = state;
	cpu_idle_state_count = state_count;
}

static int __init exynos_perf_cpuidle_profile_init(void)
{
	struct dentry *root, *d;
	struct file_operations fops;

	root = debugfs_create_dir("exynos_perf_cpuidle", NULL);
	if (!root) {
		printk("%s: create debugfs\n", __FILE__);
		return -ENOMEM;
	}

	fops.owner		= THIS_MODULE;
	fops.open		= run_debugfs_open;
	fops.write		= run_seq_write;
	fops.read		= seq_read;
	fops.read_iter		= NULL;
	fops.llseek		= seq_lseek;
	fops.release		= single_release;

	run_debugfs_fops	= fops;

	d = debugfs_create_file("run", S_IRUSR, root,
					(unsigned int *)0,
					&run_debugfs_fops);
	if (!d)
		return -ENOMEM;

	fops.open		= freq_debugfs_open;
	fops.write		= freq_seq_write;
	freq_debugfs_fops	= fops;

	d = debugfs_create_file("freq", S_IRUSR, root,
					(unsigned int *)0,
					&freq_debugfs_fops);
	if (!d)
		return -ENOMEM;

	return 0;
}
late_initcall(exynos_perf_cpuidle_profile_init);
