/* state.c
 *
 * State data for Intelligent System
 *
 * Copyright (C) 2018 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/pid_namespace.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/seqlock.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/init_task.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/sched/loadavg.h>
#include "sched/sched.h"

#define LOAD_INT(x) ((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1-1)) * 100)
#define for_each_leaf_cfs_rq(rq, cfs_rq) \
	list_for_each_entry_rcu(cfs_rq, &rq->leaf_cfs_rq_list, leaf_cfs_rq_list)


static DEFINE_MUTEX(pid_array_mutex);
static pid_t *pid_array;
static int pid_array_len;

/*
 * (1) nr_running
 * (2) cpu load  1 tick
 * (3) cpu_load  2 tick
 * (4) cpu_load  4 tick
 * (5) cpu_load  8 tick
 * (6) cpu_load 16 tick
 * (7) loadavg  1 minute
 * (8) loadavg  5 minute
 * (9) loadavg 15 minute
 */
static int state_proc_show(struct seq_file *m, void *v)
{
	unsigned long avnrun[3];
	unsigned long cpuload[CPU_LOAD_IDX_MAX] = {0,};
	unsigned long removed_load_avg_cpu[NR_CPUS] = {0,};
	//unsigned long removed_load_avg = 0;
	unsigned long removed_util_avg_cpu[NR_CPUS] = {0,};
	//unsigned long removed_util_avg = 0;
	unsigned long load_avg_cpu[NR_CPUS] = {0,};
	//unsigned long load_avg = 0;
	unsigned long runnable_load_avg_cpu[NR_CPUS] = {0,};
	//unsigned long runnable_load_avg = 0;
	unsigned long util_avg_cpu[NR_CPUS] = {0,};
	//unsigned long util_avg = 0;
	struct cfs_rq *cfs_rq;
	struct rq *rq;
	int cpu;
	int i;

	/* cpu load */
	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		rq = cpu_rq(cpu);
		for (i = 0; i < CPU_LOAD_IDX_MAX; i++) {
			cpuload[i] += rq->cpu_load[i];
		}
	}	

	/* load avg */
	get_avenrun(avnrun, FIXED_1/200, 0);

	/* cfs stats */
	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		rcu_read_lock();
		for_each_leaf_cfs_rq(cpu_rq(cpu), cfs_rq) {
			//print_cfs_rq(m, cpu, cfs_rq);
			//struct rq *rq = cpu_rq(cpu);
			load_avg_cpu[cpu] += cfs_rq->avg.load_avg;
			util_avg_cpu[cpu] += cfs_rq->avg.util_avg;
			runnable_load_avg_cpu[cpu] += cfs_rq->runnable_load_avg;
			removed_load_avg_cpu[cpu] += atomic_long_read(&cfs_rq->removed_load_avg);
			removed_util_avg_cpu[cpu] += atomic_long_read(&cfs_rq->removed_util_avg);
		}
		rcu_read_unlock();
	}

	seq_printf(m, "%ld %ld %ld %ld %ld %ld %lu.%02lu %lu.%02lu %lu.%02lu\n",
		nr_running(),
		cpuload[0], cpuload[1], cpuload[2], cpuload[3], cpuload[4],
		LOAD_INT(avnrun[0]), LOAD_FRAC(avnrun[0]),
		LOAD_INT(avnrun[1]), LOAD_FRAC(avnrun[1]),
		LOAD_INT(avnrun[2]), LOAD_FRAC(avnrun[2])
		);

	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		seq_printf(m, "cpu%d: %lu %lu %lu %lu %lu\n", cpu,
			load_avg_cpu[cpu],
			util_avg_cpu[cpu],
			runnable_load_avg_cpu[cpu],
			removed_load_avg_cpu[cpu],
			removed_util_avg_cpu[cpu]);
	}
	return 0;
}

static int task_state_proc_show(struct seq_file *m, void *v)
{
	int i;
	struct task_struct *tsk;
	unsigned long load_sum = 0;
	unsigned long util_sum = 0;
	unsigned long load_avg = 0;
	unsigned long util_avg = 0;
	int cnt = 0;
	// iterate pid array
	// 	get sched infos
	mutex_lock(&pid_array_mutex);
	for (i = 0; i < pid_array_len; i++) {
		tsk = find_task_by_vpid(pid_array[i]);
		if (!tsk) continue;
		load_sum += tsk->se.avg.load_sum;
		util_sum += tsk->se.avg.util_sum;
		load_avg += tsk->se.avg.load_avg;
		util_avg += tsk->se.avg.util_avg;
		cnt++;
		seq_printf(m, "%d ", pid_array[i]);
	}
	mutex_unlock(&pid_array_mutex);
	seq_printf(m, "\n");
	load_avg /= cnt;
	util_avg /= cnt;

	seq_printf(m, "%lu %lu %lu %lu\n", load_sum, util_sum, load_avg, util_avg);
		
	return 0;
}
static ssize_t task_state_proc_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *ppos)
{
	void *kbuf;
	char **argv;
	int argc;
	int i;
	int ret;

	if (count > 8 * 2048)
		return -EINVAL;

	kbuf = kmalloc(count + 1, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;

	if (copy_from_user(kbuf, buffer, count)) {
		kfree(kbuf);
		return -EFAULT;
	}
	argv = argv_split(GFP_KERNEL, kbuf, &argc);

	kfree(kbuf);

	if (!argv)
		return -EFAULT;

	mutex_lock(&pid_array_mutex);
	pid_array_len = argc;
	if (pid_array)
		kfree(pid_array);
	pid_array = (pid_t *)kmalloc(pid_array_len * sizeof(pid_t), GFP_KERNEL);

	// iterate argv
	// 	store in pid array
	for (i = 0; i < argc; i++) { 
		ret = kstrtoint(argv[i], 0, (int *)&pid_array[i]);
		if (ret) {
			pid_array_len--;
			pr_err("failed to convert %s : %d\n", argv[i], ret);
		}

	}
	mutex_unlock(&pid_array_mutex);

	argv_free(argv);

	return count;
}

static int state_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, state_proc_show, NULL);
}

static int task_state_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, task_state_proc_show, NULL);
}

static const struct file_operations state_proc_fops = {
	.open		= state_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static const struct file_operations task_state_proc_fops = {
	.open		= task_state_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= task_state_proc_write,
};

static struct proc_dir_entry *proc_fs_intelligence;

static int __init proc_state_init(void)
{
	proc_fs_intelligence = proc_mkdir("intelligence", NULL);
	if (!proc_fs_intelligence) {
		pr_err("%s: failed to create intelligence proc entry\n",
			__func__);
		goto err;
	}

	proc_create("state", 0, proc_fs_intelligence, &state_proc_fops);

	proc_create("task_state", 0, proc_fs_intelligence, &task_state_proc_fops);
	return 0;

err:
	remove_proc_subtree("intelligence", NULL);
	return -ENOMEM;

}

fs_initcall(proc_state_init);
