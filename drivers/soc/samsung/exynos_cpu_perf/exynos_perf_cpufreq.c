/*
 * cpufreq logging driver
 * Jungwook Kim <jwook1.kim@samsung.com>
 */

#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/cpufreq.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/thermal.h>
#include <asm/topology.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/gpu_cooling.h>
#include <linux/of.h>

#include <soc/samsung/cal-if.h>

#define MAX_CLUSTER 3

extern unsigned int get_cpufreq_max_limit(void);
extern unsigned int get_ocp_clipped_freq(void);

static const char *prefix = "exynos_perf";

static uint cluster_last_cpu[MAX_CLUSTER];
static uint cluster_first_cpu[MAX_CLUSTER];
static int cluster_count = 0;
static uint cal_id_mif = 0;
static uint cal_id_g3d = 0;

static int is_running = 0;
static void *buf = NULL;
static uint polling_ms = 100;
static uint bind_cpu = 0;
static uint pid = 0;

static int init_cluster_info(void)
{
	struct cpufreq_policy *policy;
	struct cpumask *mask;
	unsigned int next_cpu = 0;
	unsigned int cpu, last_cpu;

	cluster_count = 0;

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
		cluster_first_cpu[cluster_count] = next_cpu;
		cluster_last_cpu[cluster_count] = last_cpu;
		cluster_count++;

		next_cpu = last_cpu + 1;
	}

	return 0;
}

//---------------------------------------
// thread main
static int cpufreq_log_thread(void *data)
{
	int i;
	int ret = 0;
	gfp_t gfp;
	uint buf_size;
	struct thermal_zone_device *tz;
	int temp[3], temp_g3d;
#ifdef CONFIG_SOC_EXYNOS3830
	const char *tz_names[] = { "LITTLE0", "LITTLE1" };
#else
	const char *tz_names[] = { "BIG", "LITTLE", "MID" };
#endif
	uint siop, ocp;
	uint cpu;
	uint cpu_cur[MAX_CLUSTER];
	uint cpu_max[MAX_CLUSTER];
	uint mif, gpu;
	int gpu_util;
	struct task_struct *tsk;
	int grp_start, grp_num;

	if (is_running) {
		pr_info("[%s] already running!!\n", prefix);
		return 0;
	}

	// reset cluster count
	init_cluster_info();

	// alloc buf
	gfp = GFP_KERNEL;
	buf_size = KMALLOC_MAX_SIZE;
	if (buf)
		kfree(buf);
	buf = kmalloc(buf_size, gfp);
	if (!buf) {
		pr_info("[%s] kmalloc failed: buf\n", prefix);
		return 0;
	} else {
		memset(buf, 0, buf_size);
		pr_info("[%s] kmalloc ok. buf=%p, buf_size=%d\n", prefix, buf, buf_size);
	}

	// start
	is_running = 1;
	pr_info("[%s] cpufreq log start\n", prefix);

	//---------------------
	// header
	//---------------------
	// temperature
#ifdef CONFIG_SOC_EXYNOS3830
	ret += snprintf(buf + ret, buf_size - ret, "01-temp_LITTLE0 ");
	ret += snprintf(buf + ret, buf_size - ret, "01-temp_LITTLE1 ");
#else
	ret += snprintf(buf + ret, buf_size - ret, "01-temp_BIG ");
	ret += snprintf(buf + ret, buf_size - ret, "01-temp_LITTLE ");
#endif
	if (cluster_count > 2)
		ret += snprintf(buf + ret, buf_size - ret, "01-temp_MID ");
	ret += snprintf(buf + ret, buf_size - ret, "01-temp_G3D ");
	// cpu
	grp_start = 2;
	for (i = cluster_count-1; i >= 0; i--) {
		grp_num = grp_start + (cluster_count-1 - i);
		cpu = cluster_first_cpu[i];
		ret += snprintf(buf + ret, buf_size - ret, "0%d-cpu%d_max ", grp_num, cpu);
		ret += snprintf(buf + ret, buf_size - ret, "0%d-cpu%d_cur ", grp_num, cpu);
		if (i == (cluster_count-1)) { // if big cluster
			ret += snprintf(buf + ret, buf_size - ret, "0%d-cpu%d_siop ", grp_num, cpu);
			ret += snprintf(buf + ret, buf_size - ret, "0%d-cpu%d_ocp ", grp_num, cpu);
		}
	}
	// mif, gpu, task
	ret += snprintf(buf + ret, buf_size - ret, "05-mif_cur 06-gpu_util 06-gpu_cur 07-task_cpu\n", grp_num, cpu);

	//---------------------
	// body
	//---------------------
	while (is_running) {
		// cpu temperature
		for (i = 0; i < cluster_count; i++) {
			tz = thermal_zone_get_zone_by_name(tz_names[i]);
			thermal_zone_get_temp(tz, &temp[i]);
			temp[i] = (temp[i] < 0)? 0 : temp[i] / 1000;
			ret += snprintf(buf + ret, buf_size - ret, "%d ", temp[i]);
		}
		// gpu temperature
		tz = thermal_zone_get_zone_by_name("G3D");
		thermal_zone_get_temp(tz, &temp_g3d);
		temp_g3d = (temp_g3d < 0)? 0 : temp_g3d / 1000;
		ret += snprintf(buf + ret, buf_size - ret, "%d ", temp_g3d);

		// siop, ocp
#ifdef CONFIG_ARM_EXYNOS_UFC
		siop = get_cpufreq_max_limit() / 1000;
#else
		siop = 0;
#endif
#ifdef CONFIG_EXYNOS_OCP
		ocp = get_ocp_clipped_freq() / 1000;
#else
		ocp = 0;
#endif
		// cpufreq
		for (i = cluster_count - 1; i >= 0; i--) {
			cpu = cluster_first_cpu[i];
			cpu_max[i] = cpufreq_quick_get_max(cpu) / 1000;
			cpu_cur[i] = cpufreq_quick_get(cpu) / 1000;
			ret += snprintf(buf + ret, buf_size - ret, "%d %d ", cpu_max[i], cpu_cur[i]);
			if (i == (cluster_count-1)) {
				ret += snprintf(buf + ret, buf_size - ret, "%d %d ", siop, ocp);
			}
		}
		// mif
#if defined(CONFIG_SOC_EXYNOS9830) || defined(CONFIG_SOC_EXYNOS991)
		mif = (uint)exynos_devfreq_get_domain_freq(devfreq_mif) / 1000;
#else
		mif = (uint)cal_dfs_cached_get_rate(cal_id_mif) / 1000;
#endif
		// gpu
		gpu_util = gpu_dvfs_get_utilization();
		gpu = (uint)cal_dfs_cached_get_rate(cal_id_g3d) / 1000;
		ret += snprintf(buf + ret, buf_size - ret, "%d %d %d ", mif, gpu_util, gpu);
		// task
		tsk = find_task_by_vpid(pid);
		cpu = (!tsk)? 0 : task_cpu(tsk);
		ret += snprintf(buf + ret, buf_size - ret, "%d\n", cpu);

		// check buf size
		if ( ret + 256 > buf_size ) {
			pr_info("[%s] buf_size: %d + 256 > %d!!", prefix, ret, buf_size);
			break;
		}
		msleep(polling_ms);
	}

	return 0;
}

static struct task_struct *task;
static void cpufreq_log_start(void)
{
	if (is_running) {
		pr_err("[%s] already running!!\n", prefix);
		return;
	}
	task = kthread_create(cpufreq_log_thread, NULL, "thread%u", 0);
	kthread_bind_mask(task, cpu_coregroup_mask(bind_cpu));
	wake_up_process(task);
	return;
}

static void cpufreq_log_stop(void)
{
	is_running = 0;
	pr_info("[%s] cpufreq profile done\n", prefix);
}

/*********************************************************************
 *                          Sysfs interface                          *
 *********************************************************************/
// macro for normal nodes
#define DEF_NODE(name) \
	static int name##_seq_show(struct seq_file *file, void *iter) {	\
		seq_printf(file, "%d\n", name);	\
		return 0;	\
	}	\
	static ssize_t name##_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off) {	\
		char buf[20];	\
		count = (count > 20)? 20 : count;	\
		if (copy_from_user(buf, buffer, count) != 0)	\
			return -EFAULT;	\
		sscanf(buf, "%d", &name);	\
		return count;	\
	}	\
	static int name##_debugfs_open(struct inode *inode, struct file *file) { \
		return single_open(file, name##_seq_show, inode->i_private);	\
	}	\
	static const struct file_operations name##_debugfs_fops = {	\
		.owner		= THIS_MODULE,	\
		.open		= name##_debugfs_open,	\
		.read		= seq_read,	\
		.write		= name##_seq_write,	\
		.llseek		= seq_lseek,	\
		.release	= single_release,	\
	};

//------------------------- normal nodes
DEF_NODE(polling_ms)
DEF_NODE(bind_cpu)
DEF_NODE(pid)

static int run_seq_show(struct seq_file *file, void *iter)
{
	if (is_running) {
		seq_printf(file, "NO RESULT\n");
	} else {
		seq_printf(file, "%s", buf);	// PRINT RESULT
	}
	return 0;
}

static ssize_t run_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	int run;
	char bbuf[10];
	count = (count > 10)? 10 : count;
	if (copy_from_user(bbuf, buffer, count) != 0)
		return -EFAULT;

	if (!sscanf(bbuf, "%1d", &run))
		return -EINVAL;
	if (run)
		cpufreq_log_start();
	else
		cpufreq_log_stop();

	return count;
}

static int run_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, run_seq_show, inode->i_private);
}

static struct file_operations run_debugfs_fops;


/*--------------------------------------*/
// MAIN

static int __init exynos_perf_cpufreq_profile_init(void)
{
	struct device_node *dn = NULL;
	struct dentry *root, *d;
	struct file_operations fops;

	dn = of_find_node_by_name(dn, "exynos_perf_ncmemcpy");
	of_property_read_u32(dn, "cal-id-mif", &cal_id_mif);
	of_property_read_u32(dn, "cal-id-g3d", &cal_id_g3d);

	root = debugfs_create_dir("exynos_perf_cpufreq", NULL);
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

	d = debugfs_create_file("polling_ms", S_IRUSR, root,
					(unsigned int *)0,
					&polling_ms_debugfs_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("bind_cpu", S_IRUSR, root,
					(unsigned int *)0,
					&bind_cpu_debugfs_fops);
	if (!d)
		return -ENOMEM;

	d = debugfs_create_file("pid", S_IRUSR, root,
					(unsigned int *)0,
					&pid_debugfs_fops);
	if (!d)
		return -ENOMEM;

	return 0;
}
late_initcall(exynos_perf_cpufreq_profile_init);
