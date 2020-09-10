/* sec_bootstat.c
 *
 * Copyright (C) 2014 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/sec_class.h>
#include <linux/sec_ext.h>
#include <clocksource/arm_arch_timer.h>

static u32 mct_start;

/* timestamps at /proc/boot_stat
 * freq[3] : cluster0 / cluster1 / cluster2
 * temp[5] : cluster0 / cluster1 / cluster2 / gpu / isp
 */
struct boot_event {
	const char *string;
	unsigned int time;
	int freq[3];
	int online;
	int temp[5];
	int order;
};

static struct boot_event boot_initcall[] = {
	{"early",},
	{"core",},
	{"postcore",},
	{"arch",},
	{"subsys",},
	{"fs",},
	{"device",},
	{"late",},
};

static struct boot_event boot_events[] = {
	{"!@Boot: start init process",},
	{"!@Boot: Begin of preload()",},
	{"!@Boot: End of preload()",},
	{"!@Boot: Entered the Android system server!",},
	{"!@Boot: Start PackageManagerService",},
	{"!@Boot: End PackageManagerService",},
	{"!@Boot: Loop forever",},
	{"!@Boot: performEnableScreen",},
	{"!@Boot: Enabling Screen!",},
	{"!@Boot: bootcomplete",},
	{"!@Boot: Voice SVC is acquired",},
	{"!@Boot: Data SVC is acquired",},
	{"!@Boot_SVC : PhoneApp OnCrate",},
	{"!@Boot_DEBUG: start networkManagement",},
	{"!@Boot_DEBUG: end networkManagement",},
	{"!@Boot_SVC : RIL_UNSOL_RIL_CONNECTED",},
	{"!@Boot_SVC : setRadioPower on",},
	{"!@Boot_SVC : setUiccSubscription",},
	{"!@Boot_SVC : SIM onAllRecordsLoaded",},
	{"!@Boot_SVC : RUIM onAllRecordsLoaded",},
	{"!@Boot_SVC : setupDataCall",},
	{"!@Boot_SVC : Response setupDataCall",},
	{"!@Boot_SVC : onDataConnectionAttached",},
	{"!@Boot_SVC : IMSI Ready",},
	{"!@Boot_SVC : completeConnection",},
	{"!@Boot_DEBUG: finishUserUnlockedCompleted",},
    {"!@Boot: setIconVisibility: ims_volte: [SHOW]",},
    {"!@Boot_DEBUG: Launcher.onCreate()",},
    {"!@Boot_DEBUG: Launcher.onResume()",},
    {"!@Boot_DEBUG: Launcher.LoaderTask.run() start",},
    {"!@Boot_DEBUG: Launcher - FinishFirstBind",},
};

#define MAX_LENGTH_OF_BOOTING_LOG 90
#define DELAY_TIME_EBS 10000
#define MAX_EVENTS_EBS 100

struct enhanced_boot_time {
	struct list_head next;
	char buf[MAX_LENGTH_OF_BOOTING_LOG];
	unsigned int time;
	int freq[3];
	int online;
};

#define MAX_LENGTH_OF_SYSTEMSERVER_LOG 90
struct systemserver_init_time_entry {
	struct list_head next;
	char buf[MAX_LENGTH_OF_SYSTEMSERVER_LOG];
};

static bool bootcompleted = false;
static bool ebs_finished = false;
unsigned long long boot_complete_time = 0;
static int events_ebs = 0;

LIST_HEAD(device_init_time_list);
LIST_HEAD(systemserver_init_time_list);
LIST_HEAD(enhanced_boot_time_list);

void sec_bootstat_mct_start(u64 rate)
{
	mct_start = (u32)(rate & 0xFFFFFFFF);
}

void sec_bootstat_add_initcall(const char *s)
{
	size_t i = 0;
	unsigned long long t = 0;

	for (i = 0; i < ARRAY_SIZE(boot_initcall); i++) {
		if (!strcmp(s, boot_initcall[i].string)) {
			t = local_clock();
			do_div(t, 1000000);
			boot_initcall[i].time = (unsigned int)t;
			break;
		}
	}
}

void sec_enhanced_boot_stat_record(const char *buf)
{
	unsigned long long t = 0;
	struct enhanced_boot_time *entry;
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return;
	strncpy(entry->buf, buf, MAX_LENGTH_OF_BOOTING_LOG);
	entry->buf[MAX_LENGTH_OF_BOOTING_LOG-1] = '\0';
	t = local_clock();
	do_div(t, 1000000);
	entry->time = (unsigned int)t;
	sec_bootstat_get_cpuinfo(entry->freq, &entry->online);
	list_add(&entry->next, &enhanced_boot_time_list);
	events_ebs++;
}

static int prev;
void sec_bootstat_add(const char *c)
{
	size_t i = 0;
	unsigned long long t = 0;

	if (bootcompleted && !ebs_finished) {
		t = local_clock();
		do_div(t, 1000000);
		if ((t - boot_complete_time) >= DELAY_TIME_EBS)
			ebs_finished = true;
	}

	// Collect Boot_EBS from java side
	if (!ebs_finished && events_ebs < MAX_EVENTS_EBS){
		if (!strncmp(c, "!@Boot_EBS: ", 12)) {
			sec_enhanced_boot_stat_record(c + 12);
			return;
		}
		else if (!strncmp(c, "!@Boot_EBS_", 11)) {
			sec_enhanced_boot_stat_record(c);
			return;
		}
	}

	if(!bootcompleted && !strncmp(c, "!@Boot_SystemServer: ", 21)){
		struct systemserver_init_time_entry *entry;
		entry = kmalloc(sizeof(*entry), GFP_KERNEL);
		if (!entry)
			return;
		strncpy(entry->buf,c+ 21, MAX_LENGTH_OF_SYSTEMSERVER_LOG);
		entry->buf[MAX_LENGTH_OF_SYSTEMSERVER_LOG-1] = '\0';
		list_add(&entry->next, &systemserver_init_time_list);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(boot_events); i++) {
		if (!strcmp(c, boot_events[i].string)) {
			if (boot_events[i].time == 0) {
				boot_events[prev].order = i;
				prev = i;

				t = local_clock();
				do_div(t, 1000000);
				boot_events[i].time = (unsigned int)t;
				sec_bootstat_get_cpuinfo(boot_events[i].freq, &boot_events[i].online);
				sec_bootstat_get_thermal(boot_events[i].temp);
			}
			// careful check bootcomplete message index 9
			if(i == 9) {
				bootcompleted = true;
				boot_complete_time = local_clock();
				do_div(boot_complete_time, 1000000);
			}
			break;
		}
	}
}

void print_format(struct boot_event *data, struct seq_file *m, int index, int delta)
{
	seq_printf(m, "%-50s %6u %6u %6d %4d %4d %4d L%d%d%d%d M%d%d B%d%d %2d %2d %2d %2d %2d\n",
		data[index].string, data[index].time + mct_start,
		data[index].time, delta,
		data[index].freq[0] / 1000,
		data[index].freq[1] / 1000,
		data[index].freq[2] / 1000,
		data[index].online & 1,
		(data[index].online >> 1) & 1,
		(data[index].online >> 2) & 1,
		(data[index].online >> 3) & 1,
		(data[index].online >> 4) & 1,
		(data[index].online >> 5) & 1,
		(data[index].online >> 6) & 1,
		(data[index].online >> 7) & 1,
		data[index].temp[0],
		data[index].temp[1],
		data[index].temp[2],
		data[index].temp[3],
		data[index].temp[4]
		);
}

static int sec_boot_stat_proc_show(struct seq_file *m, void *v)
{
	size_t i = 0;
	unsigned int last_time = 0;
	struct device_init_time_entry *entry;
	struct systemserver_init_time_entry *systemserver_entry;

	seq_puts(m, "boot event                                           time  ktime  delta f_c0 f_c1 f_c2  online_mask   B  M  L  G  I\n");
	seq_puts(m, "-------------------------------------------------------------------------------------------------------------------\n");
	seq_puts(m, "BOOTLOADER - KERNEL\n");
	seq_puts(m, "-------------------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "MCT is initialized in bl2                          %6u %6u %6u\n", 0, 0, 0);
	seq_printf(m, "start kernel timer                                 %6u %6u %6u\n", mct_start, 0, mct_start);

	for (i = 0; i < ARRAY_SIZE(boot_initcall); i++) {
		print_format(boot_initcall, m, i, boot_initcall[i].time - last_time);
		last_time = boot_initcall[i].time;
	}

	seq_puts(m, "-------------------------------------------------------------------------------------------------------------------\n");
	seq_puts(m, "FRAMEWORK\n");
	seq_puts(m, "-------------------------------------------------------------------------------------------------------------------\n");
	i = 0;
	do {
		if (boot_events[i].time != 0) {
			print_format(boot_events, m, i, boot_events[i].time - last_time);
			last_time = boot_events[i].time;
		}

		if (i != boot_events[i].order)
			i = boot_events[i].order;
		else
			break;
	} while (i > 0 && i < ARRAY_SIZE(boot_events));

	seq_puts(m, "-------------------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "device init time over %d ms\n\n",
			DEVICE_INIT_TIME_100MS / 1000);

	list_for_each_entry (entry, &device_init_time_list, next)
		seq_printf(m, "%-20s : %lld usces\n",
				entry->buf, entry->duration);

	seq_puts(m, "-------------------------------------------------------------------------------------------------------------------\n");
	seq_puts(m, "SystemServer services that took long time\n\n");
	list_for_each_entry (systemserver_entry, &systemserver_init_time_list, next)
		seq_printf(m, "%s\n",systemserver_entry->buf);
	
	return 0;
}

static int sec_enhanced_boot_stat_proc_show(struct seq_file *m, void *v)
{
	size_t i = 0;
	unsigned int last_time = 0;
	struct enhanced_boot_time *entry;

	seq_printf(m, "%-90s %6s %6s %6s %4s %4s %4s\n", "Boot Events", "time", "ktime", "delta", "f_c0", "f_c1", "f_c2");
	seq_puts(m, "-----------------------------------------------------------------------------------------------------------------------\n");
	seq_puts(m, "BOOTLOADER - KERNEL\n");
	seq_puts(m, "-----------------------------------------------------------------------------------------------------------------------\n");
	seq_printf(m, "%-90s %6u %6u %6u\n", "MCT is initialized in bl2", 0, 0, 0);
	seq_printf(m, "%-90s %6u %6u %6u\n", "start kernel timer", mct_start, 0, mct_start);

	for (i = 0; i < ARRAY_SIZE(boot_initcall); i++) {
		seq_printf(m, "%-90s %6u %6u %6u %4d %4d %4d\n", boot_initcall[i].string, boot_initcall[i].time + mct_start, boot_initcall[i].time,
				boot_initcall[i].time - last_time, boot_initcall[i].freq[0] / 1000, boot_initcall[i].freq[1] / 1000, boot_initcall[i].freq[2] / 1000);
		last_time = boot_initcall[i].time;
	}

	seq_puts(m, "-----------------------------------------------------------------------------------------------------------------------\n");
	seq_puts(m, "FRAMEWORK\n");
	seq_puts(m, "-----------------------------------------------------------------------------------------------------------------------\n");
	list_for_each_entry_reverse (entry, &enhanced_boot_time_list, next){
		if (entry->buf[0] == '!') {
			seq_printf(m, "%-90s %6u %6u %6u %4d %4d %4d\n", entry->buf, entry->time + mct_start, entry->time, entry->time - last_time,
			entry->freq[0] / 1000, entry->freq[1] / 1000, entry->freq[2] / 1000);
			last_time = entry->time;
		}
		else {
			seq_printf(m, "%-90s %6u %6u %11d %4d %4d\n", entry->buf, entry->time + mct_start, entry->time, entry->freq[0] / 1000, entry->freq[1] / 1000, entry->freq[2] / 1000);
		}
	}
	return 0;
}

static int sec_boot_stat_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_boot_stat_proc_show, NULL);
}

static int sec_enhanced_boot_stat_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_enhanced_boot_stat_proc_show, NULL);
}

static const struct file_operations sec_boot_stat_proc_fops = {
	.open    = sec_boot_stat_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static const struct file_operations sec_enhanced_boot_stat_proc_fops = {
	.open    = sec_enhanced_boot_stat_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static ssize_t store_boot_stat(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	unsigned long long t = 0;

	// Collect Boot_EBS from native side
	if (!ebs_finished && events_ebs < MAX_EVENTS_EBS) {
		if (!strncmp(buf, "!@Boot_EBS: ", 12)) {
			sec_enhanced_boot_stat_record(buf + 12);
			return count;
		}
		else if (!strncmp(buf, "!@Boot_EBS_", 11)) {
			sec_enhanced_boot_stat_record(buf);
			return count;
		}
	}

	if (!strncmp(buf, "!@Boot: start init process", 26)) {
		t = local_clock();
		do_div(t, 1000000);
		boot_events[0].time = (unsigned int)t;
		sec_bootstat_get_cpuinfo(boot_events[0].freq, &boot_events[0].online);
		sec_bootstat_get_thermal(boot_events[0].temp);
	}

	return count;
}

static DEVICE_ATTR(boot_stat, 0220, NULL, store_boot_stat);

static int __init sec_bootstat_init(void)
{
	struct proc_dir_entry *entry;
	struct proc_dir_entry *enhanced_entry;
	struct device *dev;

	u32 arch_timer_rate;

	// mct
	arch_timer_rate = arch_timer_get_rate();
	arch_timer_rate /= 1000;
	if (arch_timer_rate)
		mct_start /= arch_timer_rate;

	// proc
	entry = proc_create("boot_stat", S_IRUGO, NULL, &sec_boot_stat_proc_fops);
	if (!entry)
		return -ENOMEM;
	enhanced_entry = proc_create("enhanced_boot_stat", S_IRUGO, NULL, &sec_enhanced_boot_stat_proc_fops);
	if (!enhanced_entry)
		return -ENOMEM;

	// sysfs
	dev = sec_device_create(NULL, "bsp");
	BUG_ON(!dev);
	if (IS_ERR(dev))
		pr_err("%s:Failed to create devce\n", __func__);

	if (device_create_file(dev, &dev_attr_boot_stat) < 0)
		pr_err("%s: Failed to create device file\n", __func__);

	return 0;
}

module_init(sec_bootstat_init);
