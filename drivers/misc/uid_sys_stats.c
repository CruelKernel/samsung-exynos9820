/* drivers/misc/uid_sys_stats.c
 *
 * Copyright (C) 2014 - 2015 Google, Inc.
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

#include <linux/atomic.h>
#include <linux/cpufreq_times.h>
#include <linux/err.h>
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/profile.h>
#include <linux/rtmutex.h>
#include <linux/sched/cputime.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "uid_sys_stats_bgio_whitelist.h"

#define UID_HASH_BITS	10
DECLARE_HASHTABLE(hash_table, UID_HASH_BITS);

static DEFINE_RT_MUTEX(uid_lock);
static struct proc_dir_entry *cpu_parent;
static struct proc_dir_entry *io_parent;
static struct proc_dir_entry *proc_parent;

struct io_stats {
	u64 read_bytes;
	u64 write_bytes;
	u64 rchar;
	u64 wchar;
	u64 fsync;
#ifdef CONFIG_UID_STAT_JBD_SUBMIT_BH
	u64 submit_bh_write_bytes;
#endif
};

#define UID_STATE_FOREGROUND	0
#define UID_STATE_BACKGROUND	1
#define UID_STATE_BUCKET_SIZE	2

#define UID_STATE_TOTAL_CURR	2
#define UID_STATE_TOTAL_LAST	3
#define UID_STATE_DEAD_TASKS	4
#define UID_STATE_SIZE		5

#define MAX_TASK_COMM_LEN 256

struct task_entry {
	char comm[MAX_TASK_COMM_LEN];
	pid_t pid;
	struct io_stats io[UID_STATE_SIZE];
	struct hlist_node hash;
};

struct uid_entry {
	uid_t uid;
	u64 utime;
	u64 stime;
	u64 active_utime;
	u64 active_stime;
	int state;
	struct io_stats io[UID_STATE_SIZE];
	struct hlist_node hash;
#ifdef CONFIG_UID_SYS_STATS_DEBUG
	DECLARE_HASHTABLE(task_entries, UID_HASH_BITS);
#endif

	u64 last_fg_write_bytes;
	u64 last_bg_write_bytes;
	u64 daily_writes;

	u64 last_fg_fsync;
	u64 last_bg_fsync;
	u64 daily_fsync;

	struct list_head top_n_list;
	int is_whitelist;
};

/*********** Background IOSTAT ***************/
#define NR_TOP_BG_ENTRIES	10
#define TOP_BG_ENTRY_NAMELEN	32
#define BG_IO_THRESHOLD		300 // Unit : MiB

struct bg_iostat_attr {
	struct kobj_attribute attr;
	int value;
};

#define BG_IOSTAT_RO_ATTR(name, func) \
static struct bg_iostat_attr bg_iostat_attr_##name = { \
	.attr = __ATTR(name, 0440, func, NULL), \
}

#define BG_IOSTAT_WO_ATTR(name, func) \
static struct bg_iostat_attr bg_iostat_attr_##name = { \
	.attr = __ATTR(name, 0640, NULL, func), \
}

#define BG_IOSTAT_GENERAL_RW_ATTR(name, val) \
static struct bg_iostat_attr bg_iostat_attr_##name = { \
	.attr = __ATTR(name, 0640, bg_iostat_generic_show, bg_iostat_generic_store), \
	.value = val, \
}

#define ATTR_VALUE(name) bg_iostat_attr_##name.value

#define UID_ENTRY_DAILY_FG_WRITE(entry)	\
	(entry->io[UID_STATE_FOREGROUND].write_bytes - entry->last_fg_write_bytes)
#define UID_ENTRY_DAILY_BG_WRITE(entry)	\
	(entry->io[UID_STATE_BACKGROUND].write_bytes - entry->last_bg_write_bytes)

#define UID_ENTRY_DAILY_FG_FSYNC(entry)	\
	(entry->io[UID_STATE_FOREGROUND].fsync - entry->last_fg_fsync)
#define UID_ENTRY_DAILY_BG_FSYNC(entry)	\
	(entry->io[UID_STATE_BACKGROUND].fsync - entry->last_bg_fsync)

#define BtoM(val)	((val) >> 20)

#define NR_ENTRIES_IN_ARR(v)	(sizeof(v) / sizeof(*v))

typedef uid_t appid_t;

appid_t get_appid(const char *key);
void uid_to_packagename(u32 uid, char* output, int buf_size);

static void inline update_daily_writes(struct uid_entry *entry) {
	entry->last_fg_write_bytes = entry->io[UID_STATE_FOREGROUND].write_bytes;
	entry->last_bg_write_bytes = entry->io[UID_STATE_BACKGROUND].write_bytes;
	entry->last_fg_fsync = entry->io[UID_STATE_FOREGROUND].fsync;
	entry->last_bg_fsync = entry->io[UID_STATE_BACKGROUND].fsync;

}
/* END ****** Background IOSTAT ***************/

static u64 compute_write_bytes(struct task_struct *task)
{
	if (task->ioac.write_bytes <= task->ioac.cancelled_write_bytes)
		return 0;

	return task->ioac.write_bytes - task->ioac.cancelled_write_bytes;
}

static void compute_io_bucket_stats(struct io_stats *io_bucket,
					struct io_stats *io_curr,
					struct io_stats *io_last,
					struct io_stats *io_dead)
{
	/* tasks could switch to another uid group, but its io_last in the
	 * previous uid group could still be positive.
	 * therefore before each update, do an overflow check first
	 */
	int64_t delta;

	delta = io_curr->read_bytes + io_dead->read_bytes -
		io_last->read_bytes;
	io_bucket->read_bytes += delta > 0 ? delta : 0;
	delta = io_curr->write_bytes + io_dead->write_bytes -
		io_last->write_bytes;
	io_bucket->write_bytes += delta > 0 ? delta : 0;
	delta = io_curr->rchar + io_dead->rchar - io_last->rchar;
	io_bucket->rchar += delta > 0 ? delta : 0;
	delta = io_curr->wchar + io_dead->wchar - io_last->wchar;
	io_bucket->wchar += delta > 0 ? delta : 0;
	delta = io_curr->fsync + io_dead->fsync - io_last->fsync;
	io_bucket->fsync += delta > 0 ? delta : 0;
#ifdef CONFIG_UID_STAT_JBD_SUBMIT_BH
	delta = io_curr->submit_bh_write_bytes 
		+ io_dead->submit_bh_write_bytes
		- io_last->submit_bh_write_bytes;
	io_bucket->submit_bh_write_bytes += delta > 0 ? delta : 0;
#endif

	io_last->read_bytes = io_curr->read_bytes;
	io_last->write_bytes = io_curr->write_bytes;
	io_last->rchar = io_curr->rchar;
	io_last->wchar = io_curr->wchar;
	io_last->fsync = io_curr->fsync;
#ifdef CONFIG_UID_STAT_JBD_SUBMIT_BH
	io_last->submit_bh_write_bytes = io_curr->submit_bh_write_bytes;
#endif

	memset(io_dead, 0, sizeof(struct io_stats));
}


#if defined(CONFIG_UID_STAT_JBD_SUBMIT_BH) && defined(CONFIG_SUBMIT_BH_IO_ACCOUNTING)
static void compute_jbd_submit_bh_write(struct io_stats *io_slot,
		struct task_struct *task) {
	//Submit_bh bytes of JBD thread -> add to Background write_bytes
	if(task->ioac.submit_bh_write_bytes &&
		strstr(task->comm, "jbd2") == task->comm) {
		io_slot->submit_bh_write_bytes += 
			task->ioac.submit_bh_write_bytes;
	}
};
static void compute_submit_bh_io_stats(struct io_stats *io_slots) {
	u64 sum = 0;
	sum = io_slots[UID_STATE_FOREGROUND].submit_bh_write_bytes +
		io_slots[UID_STATE_BACKGROUND].submit_bh_write_bytes;

	if(sum)
		io_slots[UID_STATE_BACKGROUND].write_bytes = sum;
}
#else
static void compute_jbd_submit_bh_write(struct io_stats *io_slot,
		struct task_struct *task) {};
static void compute_submit_bh_io_stats(struct io_stats *io_slots) {};
#endif

#ifdef CONFIG_UID_SYS_STATS_DEBUG
static void get_full_task_comm(struct task_entry *task_entry,
		struct task_struct *task)
{
	int i = 0, offset = 0, len = 0;
	/* save one byte for terminating null character */
	int unused_len = MAX_TASK_COMM_LEN - TASK_COMM_LEN - 1;
	char buf[unused_len];
	struct mm_struct *mm = task->mm;

	/* fill the first TASK_COMM_LEN bytes with thread name */
	__get_task_comm(task_entry->comm, TASK_COMM_LEN, task);
	i = strlen(task_entry->comm);
	while (i < TASK_COMM_LEN)
		task_entry->comm[i++] = ' ';

	/* next the executable file name */
	if (mm) {
		down_read(&mm->mmap_sem);
		if (mm->exe_file) {
			char *pathname = d_path(&mm->exe_file->f_path, buf,
					unused_len);

			if (!IS_ERR(pathname)) {
				len = strlcpy(task_entry->comm + i, pathname,
						unused_len);
				i += len;
				task_entry->comm[i++] = ' ';
				unused_len--;
			}
		}
		up_read(&mm->mmap_sem);
	}
	unused_len -= len;

	/* fill the rest with command line argument
	 * replace each null or new line character
	 * between args in argv with whitespace */
	len = get_cmdline(task, buf, unused_len);
	while (offset < len) {
		if (buf[offset] != '\0' && buf[offset] != '\n')
			task_entry->comm[i++] = buf[offset];
		else
			task_entry->comm[i++] = ' ';
		offset++;
	}

	/* get rid of trailing whitespaces in case when arg is memset to
	 * zero before being reset in userspace
	 */
	while (task_entry->comm[i-1] == ' ')
		i--;
	task_entry->comm[i] = '\0';
}

static struct task_entry *find_task_entry(struct uid_entry *uid_entry,
		struct task_struct *task)
{
	struct task_entry *task_entry;

	hash_for_each_possible(uid_entry->task_entries, task_entry, hash,
			task->pid) {
		if (task->pid == task_entry->pid) {
			/* if thread name changed, update the entire command */
			int len = strnchr(task_entry->comm, ' ', TASK_COMM_LEN)
				- task_entry->comm;

			if (strncmp(task_entry->comm, task->comm, len))
				get_full_task_comm(task_entry, task);
			return task_entry;
		}
	}
	return NULL;
}

static struct task_entry *find_or_register_task(struct uid_entry *uid_entry,
		struct task_struct *task)
{
	struct task_entry *task_entry;
	pid_t pid = task->pid;

	task_entry = find_task_entry(uid_entry, task);
	if (task_entry)
		return task_entry;

	task_entry = kzalloc(sizeof(struct task_entry), GFP_ATOMIC);
	if (!task_entry)
		return NULL;

	get_full_task_comm(task_entry, task);

	task_entry->pid = pid;
	hash_add(uid_entry->task_entries, &task_entry->hash, (unsigned int)pid);

	return task_entry;
}

static void remove_uid_tasks(struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;
	struct hlist_node *tmp_task;

	hash_for_each_safe(uid_entry->task_entries, bkt_task,
			tmp_task, task_entry, hash) {
		hash_del(&task_entry->hash);
		kfree(task_entry);
	}
}

static void set_io_uid_tasks_zero(struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;

	hash_for_each(uid_entry->task_entries, bkt_task, task_entry, hash) {
		memset(&task_entry->io[UID_STATE_TOTAL_CURR], 0,
			sizeof(struct io_stats));
	}
}

static void add_uid_tasks_io_stats(struct uid_entry *uid_entry,
		struct task_struct *task, int slot)
{
	struct task_entry *task_entry = find_or_register_task(uid_entry, task);
	struct io_stats *task_io_slot = &task_entry->io[slot];

	task_io_slot->read_bytes += task->ioac.read_bytes;
	task_io_slot->write_bytes += compute_write_bytes(task);
	task_io_slot->rchar += task->ioac.rchar;
	task_io_slot->wchar += task->ioac.wchar;
	task_io_slot->fsync += task->ioac.syscfs;

	compute_jbd_submit_bh_write(task_io_slot, task);
}

static void compute_io_uid_tasks(struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;

	hash_for_each(uid_entry->task_entries, bkt_task, task_entry, hash) {
		compute_io_bucket_stats(&task_entry->io[uid_entry->state],
					&task_entry->io[UID_STATE_TOTAL_CURR],
					&task_entry->io[UID_STATE_TOTAL_LAST],
					&task_entry->io[UID_STATE_DEAD_TASKS]);
		compute_submit_bh_io_stats(task_entry->io);
	}
}

static void show_io_uid_tasks(struct seq_file *m, struct uid_entry *uid_entry)
{
	struct task_entry *task_entry;
	unsigned long bkt_task;

	hash_for_each(uid_entry->task_entries, bkt_task, task_entry, hash) {
		/* Separated by comma because space exists in task comm */
		seq_printf(m, "task,%s,%lu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
				task_entry->comm,
				(unsigned long)task_entry->pid,
				task_entry->io[UID_STATE_FOREGROUND].rchar,
				task_entry->io[UID_STATE_FOREGROUND].wchar,
				task_entry->io[UID_STATE_FOREGROUND].read_bytes,
				task_entry->io[UID_STATE_FOREGROUND].write_bytes,
				task_entry->io[UID_STATE_BACKGROUND].rchar,
				task_entry->io[UID_STATE_BACKGROUND].wchar,
				task_entry->io[UID_STATE_BACKGROUND].read_bytes,
				task_entry->io[UID_STATE_BACKGROUND].write_bytes,
				task_entry->io[UID_STATE_FOREGROUND].fsync,
				task_entry->io[UID_STATE_BACKGROUND].fsync);
	}
}
#else
static void remove_uid_tasks(struct uid_entry *uid_entry) {};
static void set_io_uid_tasks_zero(struct uid_entry *uid_entry) {};
static void add_uid_tasks_io_stats(struct uid_entry *uid_entry,
		struct task_struct *task, int slot) {};
static void compute_io_uid_tasks(struct uid_entry *uid_entry) {};
static void show_io_uid_tasks(struct seq_file *m,
		struct uid_entry *uid_entry) {}
#endif

static struct uid_entry *find_uid_entry(uid_t uid)
{
	struct uid_entry *uid_entry;
	hash_for_each_possible(hash_table, uid_entry, hash, uid) {
		if (uid_entry->uid == uid)
			return uid_entry;
	}
	return NULL;
}

static struct uid_entry *find_or_register_uid(uid_t uid)
{
	struct uid_entry *uid_entry;

	uid_entry = find_uid_entry(uid);
	if (uid_entry)
		return uid_entry;

	uid_entry = kzalloc(sizeof(struct uid_entry), GFP_ATOMIC);
	if (!uid_entry)
		return NULL;

	uid_entry->uid = uid;
#ifdef CONFIG_UID_SYS_STATS_DEBUG
	hash_init(uid_entry->task_entries);
#endif
	hash_add(hash_table, &uid_entry->hash, uid);

	return uid_entry;
}

static int uid_cputime_show(struct seq_file *m, void *v)
{
	struct uid_entry *uid_entry = NULL;
	struct task_struct *task, *temp;
	struct user_namespace *user_ns = current_user_ns();
	u64 utime;
	u64 stime;
	unsigned long bkt;
	uid_t uid;

	rt_mutex_lock(&uid_lock);

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		uid_entry->active_stime = 0;
		uid_entry->active_utime = 0;
	}

	rcu_read_lock();
	do_each_thread(temp, task) {
		uid = from_kuid_munged(user_ns, task_uid(task));
		if (!uid_entry || uid_entry->uid != uid)
			uid_entry = find_or_register_uid(uid);
		if (!uid_entry) {
			rcu_read_unlock();
			rt_mutex_unlock(&uid_lock);
			pr_err("%s: failed to find the uid_entry for uid %d\n",
				__func__, uid);
			return -ENOMEM;
		}
		task_cputime_adjusted(task, &utime, &stime);
		uid_entry->active_utime += utime;
		uid_entry->active_stime += stime;
	} while_each_thread(temp, task);
	rcu_read_unlock();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		u64 total_utime = uid_entry->utime +
							uid_entry->active_utime;
		u64 total_stime = uid_entry->stime +
							uid_entry->active_stime;
		seq_printf(m, "%d: %llu %llu\n", uid_entry->uid,
			ktime_to_ms(total_utime) * USEC_PER_MSEC,
			ktime_to_ms(total_stime) * USEC_PER_MSEC);
	}

	rt_mutex_unlock(&uid_lock);
	return 0;
}

static int uid_cputime_open(struct inode *inode, struct file *file)
{
	return single_open(file, uid_cputime_show, PDE_DATA(inode));
}

static const struct file_operations uid_cputime_fops = {
	.open		= uid_cputime_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int uid_remove_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, NULL);
}

static ssize_t uid_remove_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	struct uid_entry *uid_entry;
	struct hlist_node *tmp;
	char uids[128];
	char *start_uid, *end_uid = NULL;
	long int uid_start = 0, uid_end = 0;

	if (count >= sizeof(uids))
		count = sizeof(uids) - 1;

	if (copy_from_user(uids, buffer, count))
		return -EFAULT;

	uids[count] = '\0';
	end_uid = uids;
	start_uid = strsep(&end_uid, "-");

	if (!start_uid || !end_uid)
		return -EINVAL;

	if (kstrtol(start_uid, 10, &uid_start) != 0 ||
		kstrtol(end_uid, 10, &uid_end) != 0) {
		return -EINVAL;
	}

	/* Also remove uids from /proc/uid_time_in_state */
	cpufreq_task_times_remove_uids(uid_start, uid_end);

	rt_mutex_lock(&uid_lock);

	for (; uid_start <= uid_end; uid_start++) {
		hash_for_each_possible_safe(hash_table, uid_entry, tmp,
							hash, (uid_t)uid_start) {
			if (uid_start == uid_entry->uid) {
				remove_uid_tasks(uid_entry);
				hash_del(&uid_entry->hash);
				kfree(uid_entry);
			}
		}
	}

	rt_mutex_unlock(&uid_lock);
	return count;
}

static const struct file_operations uid_remove_fops = {
	.open		= uid_remove_open,
	.release	= single_release,
	.write		= uid_remove_write,
};


static void add_uid_io_stats(struct uid_entry *uid_entry,
			struct task_struct *task, int slot)
{
	struct io_stats *io_slot = &uid_entry->io[slot];

	io_slot->read_bytes += task->ioac.read_bytes;
	io_slot->write_bytes += compute_write_bytes(task);
	io_slot->rchar += task->ioac.rchar;
	io_slot->wchar += task->ioac.wchar;
	io_slot->fsync += task->ioac.syscfs;

	compute_jbd_submit_bh_write(io_slot, task);

	add_uid_tasks_io_stats(uid_entry, task, slot);
}

static void update_io_stats_all_locked(void)
{
	struct uid_entry *uid_entry = NULL;
	struct task_struct *task, *temp;
	struct user_namespace *user_ns = current_user_ns();
	unsigned long bkt;
	uid_t uid;

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		memset(&uid_entry->io[UID_STATE_TOTAL_CURR], 0,
			sizeof(struct io_stats));
		set_io_uid_tasks_zero(uid_entry);
	}

	rcu_read_lock();
	do_each_thread(temp, task) {
		uid = from_kuid_munged(user_ns, task_uid(task));
		if (!uid_entry || uid_entry->uid != uid)
			uid_entry = find_or_register_uid(uid);
		if (!uid_entry)
			continue;
		add_uid_io_stats(uid_entry, task, UID_STATE_TOTAL_CURR);
	} while_each_thread(temp, task);
	rcu_read_unlock();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		compute_io_bucket_stats(&uid_entry->io[uid_entry->state],
					&uid_entry->io[UID_STATE_TOTAL_CURR],
					&uid_entry->io[UID_STATE_TOTAL_LAST],
					&uid_entry->io[UID_STATE_DEAD_TASKS]);
		compute_submit_bh_io_stats(uid_entry->io);
		compute_io_uid_tasks(uid_entry);
	}
}

static void update_io_stats_uid_locked(struct uid_entry *uid_entry)
{
	struct task_struct *task, *temp;
	struct user_namespace *user_ns = current_user_ns();

	memset(&uid_entry->io[UID_STATE_TOTAL_CURR], 0,
		sizeof(struct io_stats));
	set_io_uid_tasks_zero(uid_entry);

	rcu_read_lock();
	do_each_thread(temp, task) {
		if (from_kuid_munged(user_ns, task_uid(task)) != uid_entry->uid)
			continue;
		add_uid_io_stats(uid_entry, task, UID_STATE_TOTAL_CURR);
	} while_each_thread(temp, task);
	rcu_read_unlock();

	compute_io_bucket_stats(&uid_entry->io[uid_entry->state],
				&uid_entry->io[UID_STATE_TOTAL_CURR],
				&uid_entry->io[UID_STATE_TOTAL_LAST],
				&uid_entry->io[UID_STATE_DEAD_TASKS]);
	compute_submit_bh_io_stats(uid_entry->io);
	compute_io_uid_tasks(uid_entry);
}


static int uid_io_show(struct seq_file *m, void *v)
{
	struct uid_entry *uid_entry;
	unsigned long bkt;

	rt_mutex_lock(&uid_lock);

	update_io_stats_all_locked();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		seq_printf(m, "%d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
				uid_entry->uid,
				uid_entry->io[UID_STATE_FOREGROUND].rchar,
				uid_entry->io[UID_STATE_FOREGROUND].wchar,
				uid_entry->io[UID_STATE_FOREGROUND].read_bytes,
				uid_entry->io[UID_STATE_FOREGROUND].write_bytes,
				uid_entry->io[UID_STATE_BACKGROUND].rchar,
				uid_entry->io[UID_STATE_BACKGROUND].wchar,
				uid_entry->io[UID_STATE_BACKGROUND].read_bytes,
				uid_entry->io[UID_STATE_BACKGROUND].write_bytes,
				uid_entry->io[UID_STATE_FOREGROUND].fsync,
				uid_entry->io[UID_STATE_BACKGROUND].fsync);
		show_io_uid_tasks(m, uid_entry);
	}

	rt_mutex_unlock(&uid_lock);
	return 0;
}

static void bg_iostat_set_whitelist(u32 uid, int set) {
	struct uid_entry *uid_entry;

	uid_entry = find_uid_entry(uid);
	if (uid_entry != NULL) 
		uid_entry->is_whitelist = set;

	return;
}

static void bg_iostat_predefined_whitelist(void) {
	int i, nr_entries;
	u32 uid = 0;

	nr_entries = NR_ENTRIES_IN_ARR(bgio_whitelist_by_name);
	for (i=0; i<nr_entries; i++) {
		uid = get_appid(bgio_whitelist_by_name[i]);
		if (uid != 0) {
			bg_iostat_set_whitelist(uid, 1);
		}
	}

	nr_entries = NR_ENTRIES_IN_ARR(bgio_whitelist_by_uid);
	for (i=0; i<nr_entries; i++) {
		uid = bgio_whitelist_by_uid[i];
		bg_iostat_set_whitelist(uid, 1);
	}
}

static ssize_t bg_iostat_generic_show(struct kobject *kobj, 
		struct kobj_attribute *attr, char *buf)
{
	struct bg_iostat_attr *entry = \
				container_of(attr, struct bg_iostat_attr, attr);

	return scnprintf(buf, PAGE_SIZE, "%d\n", entry->value);
}

static ssize_t bg_iostat_generic_store(struct kobject *kobj, 
		struct kobj_attribute *attr, const char *buf, size_t len)
{
	struct bg_iostat_attr *entry = \
				container_of(attr, struct bg_iostat_attr, attr);
	sscanf(buf, "%d", &entry->value);

	return len;
}

BG_IOSTAT_GENERAL_RW_ATTR(write_threshold, BG_IO_THRESHOLD);
BG_IOSTAT_GENERAL_RW_ATTR(auto_reset_daily_stat, 1);

static ssize_t bg_iostat_show(struct kobject *kobj, struct kobj_attribute *attr, 
		char *buf)
{
	LIST_HEAD(top_n_head); //HEAD : Small ----> Large : TAIL
	struct uid_entry *uid_entry;
	unsigned long bkt;
	int nr_entries = 0;
	u64 smallest_bg_io = 0;
	struct uid_entry *cur_entry;
	char package_name_buf[TOP_BG_ENTRY_NAMELEN];
	int idx;
	int len = 0;
	u64 total_fg_bytes = 0;
	u64 total_bg_bytes = 0;
	u64 daily_bg_writes, daily_fg_writes;
	u64 daily_writes = 0;
	u64 daily_fsync = 0;
	unsigned int nr_apps = 0, nr_apps_bg = 0, nr_apps_thr = 0;

	rt_mutex_lock(&uid_lock);

	update_io_stats_all_locked();
	
	bg_iostat_predefined_whitelist();

	hash_for_each(hash_table, bkt, uid_entry, hash) {
		nr_apps++;

		daily_fg_writes = UID_ENTRY_DAILY_FG_WRITE(uid_entry);
		daily_bg_writes = UID_ENTRY_DAILY_BG_WRITE(uid_entry);
		daily_fsync = UID_ENTRY_DAILY_FG_FSYNC(uid_entry) + \
			      UID_ENTRY_DAILY_BG_FSYNC(uid_entry);

		if (ATTR_VALUE(auto_reset_daily_stat))
			update_daily_writes(uid_entry);

		total_fg_bytes += daily_fg_writes;
		total_bg_bytes += daily_bg_writes;
		daily_writes = daily_fg_writes + daily_bg_writes;

		if (uid_entry->is_whitelist || daily_writes == 0)
			continue;

		if (daily_bg_writes)
			nr_apps_bg++;

		if (BtoM(daily_writes) > ATTR_VALUE(write_threshold))
			nr_apps_thr++;

		if (daily_writes > smallest_bg_io) {
			list_for_each_entry(cur_entry, &top_n_head, top_n_list) {
				if (cur_entry->daily_writes > daily_writes)
					break;
			}

			__list_add(&uid_entry->top_n_list,
					cur_entry->top_n_list.prev,
					&cur_entry->top_n_list);

			uid_entry->daily_writes = daily_writes;
			uid_entry->daily_fsync = daily_fsync;

			if (nr_entries >= NR_TOP_BG_ENTRIES) {
				cur_entry = list_first_entry(&top_n_head, struct uid_entry, top_n_list);
				list_del(&cur_entry->top_n_list);

				cur_entry = list_first_entry(&top_n_head, struct uid_entry, top_n_list);
				smallest_bg_io = cur_entry->daily_writes;
			} else {
				nr_entries++;
			}
		}
	}

	len += snprintf(buf, PAGE_SIZE,
		"\"bg_stat_ver\":\"%d\",\"total_io_MB\":\"%llu\",\"bg_io_MB\":\"%llu\","
		"\"nr_apps_tot\":\"%u\",\"nr_apps_bg\":\"%u\",\"nr_apps_thr\":\"%u\","
		"\"bg_threshold\":\"%d\"",
		BG_STAT_VER, BtoM(total_fg_bytes + total_bg_bytes), BtoM(total_bg_bytes),
		nr_apps, nr_apps_bg, nr_apps_thr,
		ATTR_VALUE(write_threshold)
		);

	idx = 1;
	list_for_each_entry_reverse(cur_entry, &top_n_head, top_n_list) {
		memset(package_name_buf, 0x0, TOP_BG_ENTRY_NAMELEN);
		if (unlikely((cur_entry->uid%10000) == 1000))
			snprintf(package_name_buf, TOP_BG_ENTRY_NAMELEN, "<uid - 1000>");
		else
			uid_to_packagename(cur_entry->uid, package_name_buf, TOP_BG_ENTRY_NAMELEN);
		len += snprintf(buf+len, PAGE_SIZE,
			",\"title_%d\":\"%s\",\"fg_val_%d\":\"%llu\",\"bg_val_%d\":\"%llu\"",
			idx, package_name_buf,
			idx, BtoM(cur_entry->daily_writes),
			idx, cur_entry->daily_fsync);
		idx++;
	}

	for (; idx <= NR_TOP_BG_ENTRIES; idx++) {
		len += snprintf(buf+len, PAGE_SIZE,
			",\"title_%d\":\"<nodata>\",\"fg_val_%d\":\"0\",\"bg_val_%d\":\"0\"",
			idx, idx, idx);
	}

	len += snprintf(buf+len, PAGE_SIZE, "\n");

	rt_mutex_unlock(&uid_lock);
	return len;
}
BG_IOSTAT_RO_ATTR(sec_stat, bg_iostat_show);

static ssize_t bg_iostat_whitelist_mngt(struct kobject *kobj, 
		struct kobj_attribute *attr, const char *buf, size_t len)
{
	struct bg_iostat_attr *entry = \
				container_of(attr, struct bg_iostat_attr, attr);
	struct uid_entry *uid_entry;
	u32 ret, uid = -1;
	int set;

	if (!strcmp(entry->attr.attr.name, "add_to_whitelist_by_id")) {
		set = 1;
		sscanf(buf, "%u", &uid);
	}
	else if (!strcmp(entry->attr.attr.name, "add_to_whitelist_by_name")) {
		set = 1;
		ret = get_appid(buf);
		if(ret != 0) uid = ret;
	}
	else if (!strcmp(entry->attr.attr.name, "remove_to_whitelist_by_id")) {
		set = 0;
		sscanf(buf, "%u", &uid);
	}
	else if (!strcmp(entry->attr.attr.name, "remove_to_whitelist_by_name")) {
		set = 0;
		ret = get_appid(buf);
		if(ret != 0) uid = ret;
	}

	if (uid != -1) {
		uid_entry = find_uid_entry(uid);
		if(uid_entry == NULL) return 0;
		uid_entry->is_whitelist = set;
	}

	return len;
}

BG_IOSTAT_WO_ATTR(add_to_whitelist_by_id, bg_iostat_whitelist_mngt);
BG_IOSTAT_WO_ATTR(add_to_whitelist_by_name, bg_iostat_whitelist_mngt);
BG_IOSTAT_WO_ATTR(remove_to_whitelist_by_id, bg_iostat_whitelist_mngt);
BG_IOSTAT_WO_ATTR(remove_to_whitelist_by_name, bg_iostat_whitelist_mngt);

#define ATTR_LIST(name) (&bg_iostat_attr_##name.attr.attr)
static struct attribute *bg_iostat_attrs[] = {
	ATTR_LIST(sec_stat),
	ATTR_LIST(write_threshold),
	ATTR_LIST(auto_reset_daily_stat),
	ATTR_LIST(add_to_whitelist_by_id),
	ATTR_LIST(add_to_whitelist_by_name),
	ATTR_LIST(remove_to_whitelist_by_id),
	ATTR_LIST(remove_to_whitelist_by_name),
	NULL,
};

static struct attribute_group bg_iostat_group = {
	.attrs = bg_iostat_attrs,
};

static void bgio_stat_init(void) {
	extern struct kobject *fs_iostat_kobj;
	struct kobject *bg_iostat_kobj;
	int ret;

	bg_iostat_kobj = kobject_create_and_add("bgiostat", fs_iostat_kobj);
	if(!bg_iostat_kobj) {
		printk(KERN_WARNING "%s: BG iostat kobj create error\n",
					__func__);
		return;
	}
	
	ret = sysfs_create_group(bg_iostat_kobj, &bg_iostat_group);
	if (ret) {
		printk("%s: sysfs_create_group() failed. ret=%d\n", 
				__func__, ret);
		return;
	}
}

static int uid_io_open(struct inode *inode, struct file *file)
{
	return single_open(file, uid_io_show, PDE_DATA(inode));
}

static const struct file_operations uid_io_fops = {
	.open		= uid_io_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int uid_procstat_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, NULL);
}

static ssize_t uid_procstat_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	struct uid_entry *uid_entry;
	uid_t uid;
	int argc, state;
	char input[128];

	if (count >= sizeof(input))
		return -EINVAL;

	if (copy_from_user(input, buffer, count))
		return -EFAULT;

	input[count] = '\0';

	argc = sscanf(input, "%u %d", &uid, &state);
	if (argc != 2)
		return -EINVAL;

	if (state != UID_STATE_BACKGROUND && state != UID_STATE_FOREGROUND)
		return -EINVAL;

	rt_mutex_lock(&uid_lock);

	uid_entry = find_or_register_uid(uid);
	if (!uid_entry) {
		rt_mutex_unlock(&uid_lock);
		return -EINVAL;
	}

	if (uid_entry->state == state) {
		rt_mutex_unlock(&uid_lock);
		return count;
	}

	update_io_stats_uid_locked(uid_entry);

	uid_entry->state = state;

	rt_mutex_unlock(&uid_lock);

	return count;
}

static const struct file_operations uid_procstat_fops = {
	.open		= uid_procstat_open,
	.release	= single_release,
	.write		= uid_procstat_write,
};

static int process_notifier(struct notifier_block *self,
			unsigned long cmd, void *v)
{
	struct task_struct *task = v;
	struct uid_entry *uid_entry;
	u64 utime, stime;
	uid_t uid;

	if (!task)
		return NOTIFY_OK;

	rt_mutex_lock(&uid_lock);
	uid = from_kuid_munged(current_user_ns(), task_uid(task));
	uid_entry = find_or_register_uid(uid);
	if (!uid_entry) {
		pr_err("%s: failed to find uid %d\n", __func__, uid);
		goto exit;
	}

	task_cputime_adjusted(task, &utime, &stime);
	uid_entry->utime += utime;
	uid_entry->stime += stime;

	add_uid_io_stats(uid_entry, task, UID_STATE_DEAD_TASKS);

exit:
	rt_mutex_unlock(&uid_lock);
	return NOTIFY_OK;
}

static struct notifier_block process_notifier_block = {
	.notifier_call	= process_notifier,
};

static int __init proc_uid_sys_stats_init(void)
{
	hash_init(hash_table);

	cpu_parent = proc_mkdir("uid_cputime", NULL);
	if (!cpu_parent) {
		pr_err("%s: failed to create uid_cputime proc entry\n",
			__func__);
		goto err;
	}

	proc_create_data("remove_uid_range", 0222, cpu_parent,
		&uid_remove_fops, NULL);
	proc_create_data("show_uid_stat", 0444, cpu_parent,
		&uid_cputime_fops, NULL);

	io_parent = proc_mkdir("uid_io", NULL);
	if (!io_parent) {
		pr_err("%s: failed to create uid_io proc entry\n",
			__func__);
		goto err;
	}

	proc_create_data("stats", 0444, io_parent,
		&uid_io_fops, NULL);

	proc_parent = proc_mkdir("uid_procstat", NULL);
	if (!proc_parent) {
		pr_err("%s: failed to create uid_procstat proc entry\n",
			__func__);
		goto err;
	}

	proc_create_data("set", 0222, proc_parent,
		&uid_procstat_fops, NULL);

	bgio_stat_init();

	profile_event_register(PROFILE_TASK_EXIT, &process_notifier_block);

	return 0;

err:
	remove_proc_subtree("uid_cputime", NULL);
	remove_proc_subtree("uid_io", NULL);
	remove_proc_subtree("uid_procstat", NULL);
	return -ENOMEM;
}

early_initcall(proc_uid_sys_stats_init);
