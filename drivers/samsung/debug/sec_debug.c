/*
 * Copyright (c) 2014-2017 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kmsg_dump.h>
#include <linux/kallsyms.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>
#include <linux/tick.h>
#include <linux/file.h>
#include <linux/sec_class.h>
#include <linux/sec_ext.h>
#include <linux/sec_debug.h>
#include <linux/sec_hard_reset_hook.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/mount.h>
#include <linux/of_reserved_mem.h>
#include <linux/memblock.h>
#include <linux/sched/task.h>
#include <linux/moduleparam.h>

#include <asm/cacheflush.h>
#include <asm/stacktrace.h>

#include <soc/samsung/exynos-pmu.h>
#include <soc/samsung/exynos-powermode.h>
#include <linux/soc/samsung/exynos-soc.h>

#ifdef CONFIG_SEC_DEBUG

/* enable/disable sec_debug feature
 * level = 0 when enable = 0 && enable_user = 0
 * level = 1 when enable = 1 && enable_user = 0
 * level = 0x10001 when enable = 1 && enable_user = 1
 * The other cases are not considered
 */
union sec_debug_level_t {
	struct {
		u16 kernel_fault;
		u16 user_fault;
	} en;
	u32 uint_val;
} sec_debug_level = { .en.kernel_fault = 1, };

module_param_named(enable, sec_debug_level.en.kernel_fault, ushort, 0644);
module_param_named(enable_user, sec_debug_level.en.user_fault, ushort, 0644);
module_param_named(level, sec_debug_level.uint_val, uint, 0644);

static int sec_debug_reserve_ok;

static int __debug_sj_lock;

int sec_debug_check_sj(void)
{
	if (__debug_sj_lock == 1)
		/* Locked */
		return 1;
	else if (__debug_sj_lock == 0)
		/* Unlocked */
		return 0;

	return -1;
}

static int __init sec_debug_get_sj_status(char *str)
{
	unsigned long val = memparse(str, &str);

	pr_err("%s: start %lx\n", __func__, val);

	if (!val) {
		pr_err("%s: UNLOCKED (%lx)\n", __func__, val);
		__debug_sj_lock = 0;
		/* Unlocked or Disabled */
		return 1;
	} else {
		pr_err("%s: LOCKED (%lx)\n", __func__, val);
		__debug_sj_lock = 1;
		/* Locked */
		return 1;
	}
}
__setup("sec_debug.sjl=", sec_debug_get_sj_status);


int sec_debug_get_debug_level(void)
{
	return sec_debug_level.uint_val;
}

static long __force_upload;

static int sec_debug_get_force_upload(void)
{
	/* enabled */
	if (__force_upload == 1)
		return 1;
	/* disabled */
	else if (__force_upload == 0)
		return 0;

	return -1;
}

static int __init sec_debug_force_upload(char *str)
{
	unsigned long val = memparse(str, &str);

	if (!val) {
		pr_err("%s: disabled (%lx)\n", __func__, val);
		__force_upload = 0;
		/* Unlocked or Disabled */
		return 1;
	} else {
		pr_err("%s: enabled (%lx)\n", __func__, val);
		__force_upload = 1;
		/* Locked */
		return 1;
	}
}
__setup("androidboot.force_upload=", sec_debug_force_upload);

int sec_debug_enter_upload(void)
{
	return sec_debug_get_force_upload();
}

static void sec_debug_user_fault_dump(void)
{
	if (sec_debug_level.en.kernel_fault == 1 &&
	    sec_debug_level.en.user_fault == 1)
		panic("User Fault");
}

static ssize_t sec_debug_user_fault_write(struct file *file, const char __user *buffer, size_t count, loff_t *offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	if (strncmp(buf, "dump_user_fault", 15) == 0)
		sec_debug_user_fault_dump();

	return count;
}

static const struct file_operations sec_debug_user_fault_proc_fops = {
	.write = sec_debug_user_fault_write,
};

static int __init sec_debug_user_fault_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("user_fault", S_IWUSR | S_IWGRP, NULL,
			    &sec_debug_user_fault_proc_fops);
	if (!entry)
		return -ENOMEM;
	return 0;
}
device_initcall(sec_debug_user_fault_init);

/* layout of SDRAM : First 4KB of DRAM
*         0x0: magic            (4B)
*   0x4~0x3FF: panic string     (1020B)
* 0x400~0x7FF: panic Extra Info (1KB)
* 0x800~0xFFB: panic dumper log (2KB - 4B)
*       0xFFC: copy of magic    (4B)
*/

enum sec_debug_upload_magic_t {
	UPLOAD_MAGIC_INIT		= 0x0,
	UPLOAD_MAGIC_PANIC		= 0x66262564,
};

enum sec_debug_upload_cause_t {
	UPLOAD_CAUSE_INIT		= 0xCAFEBABE,
	UPLOAD_CAUSE_KERNEL_PANIC	= 0x000000C8,
	UPLOAD_CAUSE_FORCED_UPLOAD	= 0x00000022,
	UPLOAD_CAUSE_USER_FORCED_UPLOAD	= 0x00000074,
	UPLOAD_CAUSE_CP_ERROR_FATAL	= 0x000000CC,
	UPLOAD_CAUSE_USER_FAULT		= 0x0000002F,
	UPLOAD_CAUSE_HSIC_DISCONNECTED	= 0x000000DD,
	UPLOAD_CAUSE_POWERKEY_LONG_PRESS = 0x00000085,
	UPLOAD_CAUSE_HARD_RESET	= 0x00000066,
};

static unsigned long sec_debug_rmem_virt;
static unsigned long sec_debug_rmem_phys;
static unsigned long sec_debug_rmem_size;

static void sec_debug_set_upload_magic(unsigned magic, char *str)
{
	*(unsigned int *)sec_debug_rmem_virt = magic;

	if (str) {
#ifdef CONFIG_SEC_DEBUG_EXTRA_INFO
		sec_debug_set_extra_info_panic(str);
		sec_debug_finish_extra_info();
#endif
	}

	pr_emerg("sec_debug: set magic code (0x%x)\n", magic);
}

static void sec_debug_set_upload_cause(enum sec_debug_upload_cause_t type)
{
	exynos_pmu_write(SEC_DEBUG_PANIC_INFORM, type);

	pr_emerg("sec_debug: set upload cause (0x%x)\n", type);
}

static int __init sec_debug_magic_setup(struct reserved_mem *rmem)
{
	pr_info("%s: Reserved Mem(0x%llx, 0x%llx) - Success\n",
		__func__, rmem->base, rmem->size);

	sec_debug_rmem_phys = rmem->base;
	sec_debug_rmem_size = rmem->size;

	sec_debug_reserve_ok = 1;

	return 0;
}
RESERVEDMEM_OF_DECLARE(sec_debug_magic, "exynos,sec_debug_magic", sec_debug_magic_setup);

#define MAX_RECOVERY_CAUSE_SIZE 256
char recovery_cause[MAX_RECOVERY_CAUSE_SIZE];
unsigned long recovery_cause_offset;

static ssize_t show_recovery_cause(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!recovery_cause_offset)
		return 0;

	sec_get_param_str(recovery_cause_offset, buf);
	pr_info("%s: %s\n", __func__, buf);

	return strlen(buf);
}

static ssize_t store_recovery_cause(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if (!recovery_cause_offset)
		return 0;

	if (strlen(buf) > sizeof(recovery_cause))
		pr_err("%s: input buffer length is out of range.\n", __func__);

	snprintf(recovery_cause, sizeof(recovery_cause), "%s:%d ", current->comm, task_pid_nr(current));
	if (strlen(recovery_cause) + strlen(buf) >= sizeof(recovery_cause)) {
		pr_err("%s: input buffer length is out of range.\n", __func__);
		return count;
	}
	strncat(recovery_cause, buf, strlen(buf));

	sec_set_param_str(recovery_cause_offset, recovery_cause, sizeof(recovery_cause));
	pr_info("%s: %s, count:%d\n", __func__, recovery_cause, (int)count);

	return count;
}

static DEVICE_ATTR(recovery_cause, 0660, show_recovery_cause, store_recovery_cause);

void sec_debug_recovery_reboot(void)
{
	char *buf;

	if (recovery_cause_offset) {
		if (!recovery_cause[0] || !strlen(recovery_cause)) {
			buf = "empty caller";
			store_recovery_cause(NULL, NULL, buf, strlen(buf));
		}
	}
}

static int __init sec_debug_recovery_cause_setup(char *str)
{
	recovery_cause_offset = memparse(str, &str);

	/* If we encounter any problem parsing str ... */
	if (!recovery_cause_offset) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

	pr_info("%s, recovery_cause_offset :%lx\n", __func__, recovery_cause_offset);
out:
	return 0;
}
__setup("androidboot.recovery_offset=", sec_debug_recovery_cause_setup);

static unsigned long fmm_lock_offset;

static int __init sec_debug_fmm_lock_offset(char *arg)
{
	fmm_lock_offset = simple_strtoul(arg, NULL, 10);
	return 0;
}

early_param("sec_debug.fmm_lock_offset", sec_debug_fmm_lock_offset);

static ssize_t store_FMM_lock(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
	char lock;

	sscanf(buf, "%c", &lock);
	pr_info("%s: store %c in FMM_lock\n", __func__, lock);
	sec_set_param(fmm_lock_offset, lock);

        return count;
}

static DEVICE_ATTR(FMM_lock, 0220, NULL, store_FMM_lock);

static int __init sec_debug_recovery_cause_init(void)
{
	struct device *dev;

	memset(recovery_cause, 0, MAX_RECOVERY_CAUSE_SIZE);

	dev = sec_device_create(NULL, "sec_debug");
	WARN_ON(!dev);
	if (IS_ERR(dev))
		pr_err("%s:Failed to create devce\n", __func__);

	if (device_create_file(dev, &dev_attr_recovery_cause) < 0)
		pr_err("%s: Failed to create device file\n", __func__);

	if (device_create_file(dev, &dev_attr_FMM_lock) < 0)
		pr_err("%s: Failed to create device file\n", __func__);

	return 0;
}
late_initcall(sec_debug_recovery_cause_init);

#ifndef arch_irq_stat_cpu
#define arch_irq_stat_cpu(cpu) 0
#endif
#ifndef arch_irq_stat
#define arch_irq_stat() 0
#endif
#ifdef arch_idle_time
static cputime64_t get_idle_time(int cpu)
{
	cputime64_t idle;

	idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	if (cpu_online(cpu) && !nr_iowait_cpu(cpu))
		idle += arch_idle_time(cpu);
	return idle;
}

static cputime64_t get_iowait_time(int cpu)
{
	cputime64_t iowait;

	iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	if (cpu_online(cpu) && nr_iowait_cpu(cpu))
		iowait += arch_idle_time(cpu);
	return iowait;
}
#else
static u64 get_idle_time(int cpu)
{
	u64 idle, idle_time = -1ULL;

	if (cpu_online(cpu))
		idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.idle */
		idle = kcpustat_cpu(cpu).cpustat[CPUTIME_IDLE];
	else
		idle = idle_time * NSEC_PER_USEC;

	return idle;
}

static u64 get_iowait_time(int cpu)
{
	u64 iowait, iowait_time = -1ULL;

	if (cpu_online(cpu))
		iowait_time = get_cpu_iowait_time_us(cpu, NULL);

	if (iowait_time == -1ULL)
		/* !NO_HZ or cpu offline so we can rely on cpustat.iowait */
		iowait = kcpustat_cpu(cpu).cpustat[CPUTIME_IOWAIT];
	else
		iowait = iowait_time * NSEC_PER_USEC;

	return iowait;
}
#endif

static void sec_debug_dump_cpu_stat(void)
{
	int i, j;
	u64 user = 0;
	u64 nice = 0;
	u64 system = 0;
	u64 idle = 0;
	u64 iowait = 0;
	u64 irq = 0;
	u64 softirq = 0;
	u64 steal = 0;
	u64 guest = 0;
	u64 guest_nice = 0;
	u64 sum = 0;
	u64 sum_softirq = 0;
	unsigned int per_softirq_sums[NR_SOFTIRQS] = {0};

	char *softirq_to_name[NR_SOFTIRQS] = { "HI", "TIMER", "NET_TX", "NET_RX", "BLOCK", "BLOCK_IOPOLL", "TASKLET", "SCHED", "HRTIMER", "RCU" };

	for_each_possible_cpu(i) {
		user	+= kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice	+= kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system	+= kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle	+= get_idle_time(i);
		iowait	+= get_iowait_time(i);
		irq	+= kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq	+= kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal	+= kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest	+= kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice += kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];
		sum	+= kstat_cpu_irqs_sum(i);
		sum	+= arch_irq_stat_cpu(i);

		for (j = 0; j < NR_SOFTIRQS; j++) {
			unsigned int softirq_stat = kstat_softirqs_cpu(j, i);

			per_softirq_sums[j] += softirq_stat;
			sum_softirq += softirq_stat;
		}
	}
	sum += arch_irq_stat();

	pr_info("\n");
	pr_info("cpu   user:%llu \tnice:%llu \tsystem:%llu \tidle:%llu \tiowait:%llu \tirq:%llu \tsoftirq:%llu \t %llu %llu %llu\n",
		(unsigned long long)nsec_to_clock_t(user),
		(unsigned long long)nsec_to_clock_t(nice),
		(unsigned long long)nsec_to_clock_t(system),
		(unsigned long long)nsec_to_clock_t(idle),
		(unsigned long long)nsec_to_clock_t(iowait),
		(unsigned long long)nsec_to_clock_t(irq),
		(unsigned long long)nsec_to_clock_t(softirq),
		(unsigned long long)nsec_to_clock_t(steal),
		(unsigned long long)nsec_to_clock_t(guest),
		(unsigned long long)nsec_to_clock_t(guest_nice));
	pr_info("-------------------------------------------------------------------------------------------------------------\n");

	for_each_possible_cpu(i) {
		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user	= kcpustat_cpu(i).cpustat[CPUTIME_USER];
		nice	= kcpustat_cpu(i).cpustat[CPUTIME_NICE];
		system	= kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM];
		idle	= get_idle_time(i);
		iowait	= get_iowait_time(i);
		irq	= kcpustat_cpu(i).cpustat[CPUTIME_IRQ];
		softirq	= kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ];
		steal	= kcpustat_cpu(i).cpustat[CPUTIME_STEAL];
		guest	= kcpustat_cpu(i).cpustat[CPUTIME_GUEST];
		guest_nice = kcpustat_cpu(i).cpustat[CPUTIME_GUEST_NICE];

		pr_info("cpu%d  user:%llu \tnice:%llu \tsystem:%llu \tidle:%llu \tiowait:%llu \tirq:%llu \tsoftirq:%llu \t %llu %llu %llu\n",
			i,
			(unsigned long long)nsec_to_clock_t(user),
			(unsigned long long)nsec_to_clock_t(nice),
			(unsigned long long)nsec_to_clock_t(system),
			(unsigned long long)nsec_to_clock_t(idle),
			(unsigned long long)nsec_to_clock_t(iowait),
			(unsigned long long)nsec_to_clock_t(irq),
			(unsigned long long)nsec_to_clock_t(softirq),
			(unsigned long long)nsec_to_clock_t(steal),
			(unsigned long long)nsec_to_clock_t(guest),
			(unsigned long long)nsec_to_clock_t(guest_nice));
	}
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("irq : %llu", (unsigned long long)sum);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	/* sum again ? it could be updated? */
	for_each_irq_nr(j) {
		unsigned int irq_stat = kstat_irqs(j);

		if (irq_stat) {
			pr_info("irq-%-4d : %8u %s\n", j, irq_stat,
				irq_to_desc(j)->action ? irq_to_desc(j)->action->name ? : "???" : "???");
		}
	}
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("softirq : %llu", (unsigned long long)sum_softirq);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
	for (i = 0; i < NR_SOFTIRQS; i++)
		if (per_softirq_sums[i])
			pr_info("softirq-%d : %8u %s\n", i, per_softirq_sums[i], softirq_to_name[i]);
	pr_info("-------------------------------------------------------------------------------------------------------------\n");
}

void sec_debug_clear_magic_rambase(void)
{
	/* Clear magic code in normal reboot */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_INIT, NULL);
}

void sec_debug_reboot_handler(void *p)
{
	pr_emerg("sec_debug: %s\n", __func__);

	/* Clear magic code in normal reboot */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_INIT, NULL);

	if (p != NULL)
		if (!strcmp(p, "recovery"))
			sec_debug_recovery_reboot();
}

void sec_debug_panic_handler(void *buf, bool dump)
{
	pr_emerg("sec_debug: %s\n", __func__);

	/* Set upload cause */
	sec_debug_set_upload_magic(UPLOAD_MAGIC_PANIC, buf);
	if (!strncmp(buf, "User Fault", 10))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FAULT);
	else if (is_hard_reset_occurred())
		sec_debug_set_upload_cause(UPLOAD_CAUSE_HARD_RESET);
	else if (!strncmp(buf, "Crash Key", 9))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_FORCED_UPLOAD);
	else if (!strncmp(buf, "User Crash Key", 14))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_USER_FORCED_UPLOAD);
	else if (!strncmp(buf, "CP Crash", 8))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_CP_ERROR_FATAL);
	else if (!strncmp(buf, "HSIC Disconnected", 17))
		sec_debug_set_upload_cause(UPLOAD_CAUSE_HSIC_DISCONNECTED);
	else
		sec_debug_set_upload_cause(UPLOAD_CAUSE_KERNEL_PANIC);

	/* dump debugging info */
	if (dump) {
		sec_debug_dump_cpu_stat();
		debug_show_all_locks();
	}
}

void sec_debug_post_panic_handler(void)
{
	hard_reset_delay();

	/* reset */
	pr_emerg("sec_debug: %s\n", linux_banner);
	pr_emerg("sec_debug: rebooting...\n");

	flush_cache_all();
}

#ifdef CONFIG_SEC_DEBUG_FILE_LEAK
void sec_debug_print_file_list(void)
{
	int i = 0;
	unsigned int count = 0;
	struct file *file = NULL;
	struct files_struct *files = current->files;
	const char *p_rootname = NULL;
	const char *p_filename = NULL;

	count = files->fdt->max_fds;

	pr_err("[Opened file list of process %s(PID:%d, TGID:%d) :: %d]\n",
	       current->group_leader->comm, current->pid, current->tgid, count);

	for (i = 0; i < count; i++) {
		rcu_read_lock();
		file = fcheck_files(files, i);

		p_rootname = NULL;
		p_filename = NULL;

		if (file) {
			if (file->f_path.mnt && file->f_path.mnt->mnt_root &&
			    file->f_path.mnt->mnt_root->d_name.name)
				p_rootname = file->f_path.mnt->mnt_root->d_name.name;

			if (file->f_path.dentry && file->f_path.dentry->d_name.name)
				p_filename = file->f_path.dentry->d_name.name;

			pr_err("[%04d]%s%s\n", i, p_rootname ? p_rootname : "null",
			       p_filename ? p_filename : "null");
		}
		rcu_read_unlock();
	}
}

void sec_debug_EMFILE_error_proc(unsigned long files_addr)
{
	if (files_addr != (unsigned long)(current->files)) {
		pr_err("Too many open files Error at %pS\n"
		       "%s(%d) thread of %s process tried fd allocation by proxy.\n"
		       "files_addr = 0x%lx, current->files=0x%p\n",
		       __builtin_return_address(0),
		       current->comm, current->tgid, current->group_leader->comm,
		       files_addr, current->files);
		return;
	}

	pr_err("Too many open files(%d:%s) at %pS\n",
	       current->tgid, current->group_leader->comm, __builtin_return_address(0));

	if (!sec_debug_level.en.kernel_fault)
		return;

	/* We check EMFILE error in only "system_server","mediaserver" and "surfaceflinger" process.*/
	if (!strcmp(current->group_leader->comm, "system_server") ||
	    !strcmp(current->group_leader->comm, "mediaserver") ||
	    !strcmp(current->group_leader->comm, "surfaceflinger")) {
		sec_debug_print_file_list();
		panic("Too many open files");
	}
}
#endif /* CONFIG_SEC_DEBUG_FILE_LEAK */

static struct sec_debug_next *sdn;
static unsigned long sec_debug_next_phys;
static unsigned long sec_debug_next_size;

void sec_debug_set_task_in_pm_suspend(uint64_t task)
{
	if (sdn)
		sdn->kernd.task_in_pm_suspend = task;
}

void sec_debug_set_task_in_sys_reboot(uint64_t task)
{
	if (sdn)
		sdn->kernd.task_in_sys_reboot = task;
}

struct watchdogd_info *sec_debug_get_wdd_info(void)
{
	if (sdn) {
		pr_crit("%s: return right value\n", __func__);

		return &(sdn->kernd.wddinfo);
	}

	pr_crit("%s: return NULL\n", __func__);

	return NULL;
}

struct bad_stack_info *sec_debug_get_bs_info(void)
{
	if (sdn)
		return &sdn->kernd.bsi;

	return NULL;
}

void *sec_debug_get_debug_base(int type)
{
	if (sdn) {
		if (type == SDN_MAP_AUTO_COMMENT)
			return &(sdn->auto_comment);
		else if (type == SDN_MAP_EXTRA_INFO)
			return &(sdn->extra_info);
	}

	pr_crit("%s: return NULL\n", __func__);

	return NULL;
}

unsigned long sec_debug_get_buf_base(int type)
{
	if (sdn) {
		return sdn->map.buf[type].base;
	}

	pr_crit("%s: return 0\n", __func__);

	return 0;
}

unsigned long sec_debug_get_buf_size(int type)
{
	if (sdn) {
		return sdn->map.buf[type].size;
	}

	pr_crit("%s: return 0\n", __func__);

	return 0;
}

void secdbg_write_buf(struct outbuf *obuf, int len, const char *fmt, ...)
{
	va_list list;
	char *base;
	int rem, ret;

	base = obuf->buf;
	base += obuf->index;

	rem = sizeof(obuf->buf);
	rem -= obuf->index;

	if (rem <= 0)
		return;

	if ((len > 0) && (len < rem))
		rem = len;

	va_start(list, fmt);
	ret = vsnprintf(base, rem, fmt, list);
	if (ret)
		obuf->index += ret;

	va_end(list);
}

void sec_debug_set_task_in_sys_shutdown(uint64_t task)
{
	if (sdn)
		sdn->kernd.task_in_sys_shutdown = task;
}

void sec_debug_set_task_in_dev_shutdown(uint64_t task)
{
	if (sdn)
		sdn->kernd.task_in_dev_shutdown = task;
}

void sec_debug_set_sysrq_crash(struct task_struct *task)
{
	if (sdn) {
		sdn->kernd.task_in_sysrq_crash = (uint64_t)task;

#ifdef CONFIG_SEC_DEBUG_SYSRQ_KMSG
		if (task) {
			if (strcmp(task->comm, "init") == 0)
				sdn->kernd.sysrq_ptr = sec_debug_get_curr_init_ptr();
			else
				sdn->kernd.sysrq_ptr = dbg_snapshot_get_curr_ptr_for_sysrq();
#endif
		}
	}
}

void sec_debug_set_task_in_soft_lockup(uint64_t task)
{
	if (sdn)
		sdn->kernd.task_in_soft_lockup = task;
}

void sec_debug_set_cpu_in_soft_lockup(uint64_t cpu)
{
	if (sdn)
		sdn->kernd.cpu_in_soft_lockup = cpu;
}

void sec_debug_set_task_in_hard_lockup(uint64_t task)
{
	if (sdn)
		sdn->kernd.task_in_hard_lockup = task;
}

void sec_debug_set_cpu_in_hard_lockup(uint64_t cpu)
{
	if (sdn)
		sdn->kernd.cpu_in_hard_lockup = cpu;
}

void sec_debug_set_unfrozen_task(uint64_t task)
{
	if (sdn)
		sdn->kernd.unfrozen_task = task;
}

void sec_debug_set_unfrozen_task_count(uint64_t count)
{
	if (sdn)
		sdn->kernd.unfrozen_task_count = count;
}

void sec_debug_set_task_in_sync_irq(uint64_t task, unsigned int irq, const char *name, struct irq_desc *desc)
{
	if (sdn) {
		sdn->kernd.sync_irq_task = task;
		sdn->kernd.sync_irq_num = irq;
		sdn->kernd.sync_irq_name = (uint64_t)name;
		sdn->kernd.sync_irq_desc = (uint64_t)desc;

		if (desc) {
			sdn->kernd.sync_irq_threads_active = desc->threads_active.counter;

			if (desc->action && (desc->action->irq == irq) && desc->action->thread)
				sdn->kernd.sync_irq_thread = (uint64_t)(desc->action->thread);
			else
				sdn->kernd.sync_irq_thread = 0;
		}		
	}
}

void sec_debug_set_device_shutdown_timeinfo(uint64_t start, uint64_t end, uint64_t duration, uint64_t func)
{
	if (sdn && func) {
		if (duration > sdn->kernd.dev_shutdown_duration) {
			sdn->kernd.dev_shutdown_start = start;
			sdn->kernd.dev_shutdown_end = end;
			sdn->kernd.dev_shutdown_duration = duration;
			sdn->kernd.dev_shutdown_func = func;
		}
	}
}

void sec_debug_clr_device_shutdown_timeinfo(void)
{
	if (sdn) {
		sdn->kernd.dev_shutdown_start = 0;
		sdn->kernd.dev_shutdown_end = 0;
		sdn->kernd.dev_shutdown_duration = 0;
		sdn->kernd.dev_shutdown_func = 0;
	}
}

void sec_debug_set_shutdown_device(const char *fname, const char *dname)
{
	if (sdn) {
		sdn->kernd.sdi.shutdown_func = (uint64_t)fname;
		sdn->kernd.sdi.shutdown_device = (uint64_t)dname;
	}
}

void sec_debug_set_suspend_device(const char *fname, const char *dname)
{
	if (sdn) {
		sdn->kernd.sdi.suspend_func = (uint64_t)fname;
		sdn->kernd.sdi.suspend_device = (uint64_t)dname;
	}
}

static void init_ess_info(unsigned int index, char *key)
{
	struct ess_info_offset *p;

	p = &(sdn->ss_info.item[index]);

	sec_debug_get_kevent_info(p, index);

	memset(p->key, 0, SD_ESSINFO_KEY_SIZE);
	snprintf(p->key, SD_ESSINFO_KEY_SIZE, key);
}

static void sec_debug_set_essinfo(void)
{
	unsigned int index = 0;

	memset(&(sdn->ss_info), 0, sizeof(struct sec_debug_ess_info));

	init_ess_info(index++, "kevnt-task");
	init_ess_info(index++, "kevnt-work");
	init_ess_info(index++, "kevnt-irq");
	init_ess_info(index++, "kevnt-freq");
	init_ess_info(index++, "kevnt-idle");
	init_ess_info(index++, "kevnt-thrm");
	init_ess_info(index++, "kevnt-acpm");
	init_ess_info(index++, "kevnt-mfrq");

	for (; index < SD_NR_ESSINFO_ITEMS;)
		init_ess_info(index++, "empty");

	for (index = 0; index < SD_NR_ESSINFO_ITEMS; index++)
		printk("%s: key: %s offset: %llx nr: %x\n", __func__,
				sdn->ss_info.item[index].key,
				sdn->ss_info.item[index].base,
				sdn->ss_info.item[index].nr);
}

static void sec_debug_set_taskinfo(void)
{
	sdn->task.stack_size = THREAD_SIZE;
	sdn->task.start_sp = THREAD_START_SP;
	sdn->task.irq_stack.pcpu_stack = (uint64_t)&irq_stack_ptr;
	sdn->task.irq_stack.size = IRQ_STACK_SIZE;
	sdn->task.irq_stack.start_sp = IRQ_STACK_START_SP;

	sdn->task.ti.struct_size = sizeof(struct thread_info);
	SET_MEMBER_TYPE_INFO(&sdn->task.ti.flags, struct thread_info, flags);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.cpu, struct task_struct, cpu);

	sdn->task.ts.struct_size = sizeof(struct task_struct);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.state, struct task_struct, state);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.exit_state, struct task_struct,
					exit_state);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.stack, struct task_struct, stack);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.flags, struct task_struct, flags);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.on_cpu, struct task_struct, on_cpu);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.pid, struct task_struct, pid);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.on_rq, struct task_struct, on_rq);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.comm, struct task_struct, comm);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.tasks_next, struct task_struct,
					tasks.next);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.thread_group_next,
					struct task_struct, thread_group.next);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.fp, struct task_struct,
					thread.cpu_context.fp);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.sp, struct task_struct,
					thread.cpu_context.sp);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.pc, struct task_struct,
					thread.cpu_context.pc);

	SET_MEMBER_TYPE_INFO(&sdn->task.ts.sched_info__pcount,
					struct task_struct, sched_info.pcount);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.sched_info__run_delay,
					struct task_struct,
					sched_info.run_delay);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.sched_info__last_arrival,
					struct task_struct,
					sched_info.last_arrival);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.sched_info__last_queued,
					struct task_struct,
					sched_info.last_queued);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.ssdbg_wait__type,
					struct task_struct,
					ssdbg_wait.type);
	SET_MEMBER_TYPE_INFO(&sdn->task.ts.ssdbg_wait__data,
					struct task_struct,
					ssdbg_wait.data);

	sdn->task.init_task = (uint64_t)&init_task;
}

static void sec_debug_set_spinlockinfo(void)
{
#ifdef CONFIG_DEBUG_SPINLOCK
	SET_MEMBER_TYPE_INFO(&sdn->rlock.owner_cpu, struct raw_spinlock, owner_cpu);
	SET_MEMBER_TYPE_INFO(&sdn->rlock.owner, struct raw_spinlock, owner);
	sdn->rlock.debug_enabled = 1;
#else
	sdn->rlock.debug_enabled = 0;
#endif
}

static unsigned long kconfig_base;
static unsigned long kconfig_size;

void sec_debug_set_kconfig(unsigned long base, unsigned long size)
{
	if (!sdn) {
		pr_info("%s: call before sdn init\n", __func__);
	}

	kconfig_base = base;
	kconfig_size = size;
}

static void sec_debug_set_kconstants(void)
{
	sdn->kcnst.nr_cpus = NR_CPUS;
	sdn->kcnst.per_cpu_offset.pa = virt_to_phys(__per_cpu_offset);
	sdn->kcnst.per_cpu_offset.size = sizeof(__per_cpu_offset[0]);
	sdn->kcnst.per_cpu_offset.count = sizeof(__per_cpu_offset) / sizeof(__per_cpu_offset[0]);

	sdn->kcnst.phys_offset = PHYS_OFFSET;
	sdn->kcnst.phys_mask = PHYS_MASK;
	sdn->kcnst.page_offset = PAGE_OFFSET;
	sdn->kcnst.page_mask = PAGE_MASK;
	sdn->kcnst.page_shift = PAGE_SHIFT;

	sdn->kcnst.va_bits = VA_BITS;
	sdn->kcnst.kimage_vaddr = kimage_vaddr;
	sdn->kcnst.kimage_voffset = kimage_voffset;

	sdn->kcnst.pa_swapper = (uint64_t)virt_to_phys(init_mm.pgd);
	sdn->kcnst.pgdir_shift = PGDIR_SHIFT;
	sdn->kcnst.pud_shift = PUD_SHIFT;
	sdn->kcnst.pmd_shift = PMD_SHIFT;
	sdn->kcnst.ptrs_per_pgd = PTRS_PER_PGD;
	sdn->kcnst.ptrs_per_pud = PTRS_PER_PUD;
	sdn->kcnst.ptrs_per_pmd = PTRS_PER_PMD;
	sdn->kcnst.ptrs_per_pte = PTRS_PER_PTE;

	sdn->kcnst.kconfig_base = kconfig_base;
	sdn->kcnst.kconfig_size = kconfig_size;

	sdn->kcnst.pa_text = virt_to_phys(_text);
	sdn->kcnst.pa_start_rodata = virt_to_phys(__start_rodata);

}

static void __init sec_debug_init_sdn(struct sec_debug_next *d)
{
#define clear_sdn_field(__p, __m)	memset(&(__p)->__m, 0x0, sizeof((__p)->__m));

	clear_sdn_field(d, memtab);
	clear_sdn_field(d, ksyms);
	clear_sdn_field(d, kcnst);
	clear_sdn_field(d, task);
	clear_sdn_field(d, ss_info);
	clear_sdn_field(d, rlock);
	clear_sdn_field(d, kernd);
}

static int __init sec_debug_next_init(void)
{
	if (!sdn) {
		pr_info("%s: sdn is not allocated, quit\n", __func__);

		return -1;
	}

	/* set magic */
	sdn->magic[0] = SEC_DEBUG_MAGIC0;
	sdn->magic[1] = SEC_DEBUG_MAGIC1;

	sdn->version[1] = SEC_DEBUG_KERNEL_UPPER_VERSION << 16;
	sdn->version[1] += SEC_DEBUG_KERNEL_LOWER_VERSION;

	/* set member table */
	secdbg_base_set_memtab_info(&sdn->memtab);

	/* set kernel symbols */
	sec_debug_set_kallsyms_info(&(sdn->ksyms), SEC_DEBUG_MAGIC1);

	/* set kernel constants */
	sec_debug_set_kconstants();
	sec_debug_set_taskinfo();
	sec_debug_set_essinfo();
	sec_debug_set_spinlockinfo();

	sec_debug_set_task_in_pm_suspend((uint64_t)NULL);
	sec_debug_set_task_in_sys_reboot((uint64_t)NULL);
	sec_debug_set_task_in_sys_shutdown((uint64_t)NULL);
	sec_debug_set_task_in_dev_shutdown((uint64_t)NULL);
	sec_debug_set_sysrq_crash(NULL);
	sec_debug_set_task_in_soft_lockup((uint64_t)NULL);
	sec_debug_set_cpu_in_soft_lockup((uint64_t)0);
	sec_debug_set_task_in_hard_lockup((uint64_t)NULL);
	sec_debug_set_cpu_in_hard_lockup((uint64_t)0);
	sec_debug_set_unfrozen_task((uint64_t)NULL);
	sec_debug_set_unfrozen_task_count((uint64_t)0);
	sec_debug_set_task_in_sync_irq((uint64_t)NULL, 0, NULL, NULL);
	sec_debug_clr_device_shutdown_timeinfo();

	pr_crit("%s: TEST: %d\n", __func__, sec_debug_get_debug_level());

	return 0;
}
late_initcall(sec_debug_next_init);

static int __init sec_debug_nocache_remap(void)
{
	pgprot_t prot = __pgprot(PROT_NORMAL_NC);
	int page_size, i;
	struct page *page;
	struct page **pages;
	void *addr;

	if (!sec_debug_rmem_size || !sec_debug_rmem_phys) {
		pr_err("%s: failed to set nocache pages\n", __func__);

		return -1;
	}

	page_size = (sec_debug_rmem_size + PAGE_SIZE - 1) / PAGE_SIZE; 
	pages = kzalloc(sizeof(struct page *) * page_size, GFP_KERNEL);
	if (!pages) {
		pr_err("%s: failed to allocate pages\n", __func__);

		return -1;
	}
	page = phys_to_page(sec_debug_rmem_phys);
	for (i = 0; i < page_size; i++)
		pages[i] = page++;

	addr = vm_map_ram(pages, page_size, -1, prot);
	if (!addr) {
		pr_err("%s: failed to mapping between virt and phys\n", __func__);
		kfree(pages);

		return -1;
	}

	pr_info("%s: virt: 0x%p\n", __func__, addr);
	sec_debug_rmem_virt = (unsigned long)addr;

	kfree(pages);

	memset((void *)sec_debug_rmem_virt, 0, sec_debug_rmem_size);
	sec_debug_set_upload_magic(UPLOAD_MAGIC_PANIC, NULL);

	return 0;
}
early_initcall(sec_debug_nocache_remap);

static int __init sec_debug_next_setup(char *str)
{
	unsigned long size = memparse(str, &str);
	unsigned long base = 0;

	/* If we encounter any problem parsing str ... */
	if (!size || *str != '@' || kstrtoul(str + 1, 0, &base)) {
		pr_err("%s: failed to parse address.\n", __func__);
		goto out;
	}

#ifdef CONFIG_NO_BOOTMEM
	if (memblock_is_region_reserved(base, size) || memblock_reserve(base, size)) {
#else
	if reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE) {
#endif
		/* size is not match with -size and size + sizeof(...) */
		pr_err("%s: failed to reserve size:0x%lx at base 0x%lx\n",
		       __func__, size, base);
		goto out;
	}

	sdn = (struct sec_debug_next *)phys_to_virt(base);
	if (!sdn) {
		pr_info("%s: fail to init sec debug next buffer\n", __func__);

		goto out;
	}
	sec_debug_init_sdn(sdn);

	sec_debug_next_phys = base;
	sec_debug_next_size = size;
	pr_info("%s: base(virt):0x%lx size:0x%lx\n", __func__, (unsigned long)sdn, size);
	pr_info("%s: ds size: 0x%lx\n", __func__, round_up(sizeof(struct sec_debug_next), PAGE_SIZE));

out:
	return 0;
}
__setup("sec_debug_next=", sec_debug_next_setup);

#endif /* CONFIG_SEC_DEBUG */
