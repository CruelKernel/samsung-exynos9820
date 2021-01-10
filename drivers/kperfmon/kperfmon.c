/* SPDX-License-Identifier: GPL-2.0
 *
 * Samsung's performance logging
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 * Binse Park <unsang.park@samsung.com>
 */

#define KPERFMON_KERNEL
#include <linux/ologk.h>
#undef KPERFMON_KERNEL

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/sec_debug.h>
#include <linux/perflog.h>
#if !defined(KPERFMON_KMALLOC)
#include <linux/vmalloc.h>
#endif
#include <linux/sched/cputime.h>
#include <linux/sched/signal.h>
#include <asm/uaccess.h>
#include <asm/stacktrace.h>


#define FLAG_NOTHING			0
#define FLAG_READING			1
#define USE_WORKQUEUE			1


#define	PROC_NAME			"kperfmon"
#if defined(KPERFMON_KMALLOC)
#define BUFFER_SIZE			(5 * 1024)
#else
#define BUFFER_SIZE			(5 * 1024 * 1024)
#endif

#define HEADER_SIZE			PERFLOG_HEADER_SIZE
#define DEBUGGER_SIZE			32
#define STREAM_SIZE			(PERFLOG_BUFF_STR_MAX_SIZE + PERFLOG_HEADER_SIZE)
#define READ_BUFFER_SIZE		(STREAM_SIZE + 100)

#define MAX_DEPTH_OF_CALLSTACK		20
#define MAX_MUTEX_RAWDATA		20

#define SIGNAL_35			35
#define SIGNAL_OLOG			5209

#if defined(USE_MONITOR)
#define MAX_MUTEX_RAWDATA_DIGIT		2
#define DIGIT_UNIT			100000000
#endif

typedef unsigned char byte;

struct tRingBuffer {
	byte *data;
	long length;
	long start;
	long end;
	long position;

	struct mutex mutex;
	long debugger;
	bool status;
};

struct tRingBuffer buffer = {0, };
struct file_operations;

#if defined(USE_WORKQUEUE)
typedef struct {
	struct work_struct ologk_work;
	union _uPLogPacket writelogpacket;
} t_ologk_work;
#endif

struct t_command {
	char *command;
	void (*func)(char *writebuffer);
};

#if defined(RETURN_ALL_DATA_AT_ONCE)
byte *readbuffer = 0;
#endif

#if defined(USE_MONITOR)
unsigned long mutex_rawdata[MAX_MUTEX_RAWDATA + 1][MAX_MUTEX_RAWDATA_DIGIT] = {{0, },};
#endif

int ops_write_buffer(struct tRingBuffer *buffer,
			byte *data, unsigned long length);
int ops_process_command(struct tRingBuffer *buffer,
			byte *data, unsigned long length);

enum {
    SH_TYPE_PACKET,
    SH_TYPE_COMMAND,
};

enum {
    SH_TYPE,
    SH_IDX_PACKET
};

int (*write_opts[])(struct tRingBuffer *buffer,
			byte *data, unsigned long length)
			= {
				ops_write_buffer,
				ops_process_command,
			};

void set_kperfmon_debugger_function(char *writebuffer);
void process_version_function(char *writebuffer);

struct t_command commands[] = {
	{"kperfmon_debugger", set_kperfmon_debugger_function},
	{"java_version", process_version_function},
	{"nativelib_version", process_version_function},
	{"perfmond_version", process_version_function},
};

struct t_before_print {
	void *pdata;
	int (*func)(char *read_buffer);
	struct list_head list;
};

struct t_before_print *before_list_cur_pos;
static LIST_HEAD(before_print_list);


void CreateBuffer(struct tRingBuffer *buffer,
			unsigned long length);
void DestroyBuffer(struct tRingBuffer *buffer);
void WriteBuffer(struct tRingBuffer *buffer,
			byte *data, unsigned long length);
void GetNext(struct tRingBuffer *buffer);
void ReadBuffer(struct tRingBuffer *buffer,
			byte *data,
			unsigned long *length);
ssize_t kperfmon_write(struct file *filp,
				const char __user *data,
				size_t length,
				loff_t *loff_data);
ssize_t kperfmon_read(struct file *filp,
				char __user *data,
				size_t count,
				loff_t *loff_data);

void CreateBuffer(struct tRingBuffer *buffer,
			unsigned long length)
{
	if (buffer->data != 0) {
		return;
	}

#if defined(KPERFMON_KMALLOC)
	buffer->data = kmalloc(length + 1, GFP_KERNEL);
#else
	buffer->data = vmalloc(length + 1);
#endif
	if (buffer->data == 0) {
		pr_info("kperfmon error [%s] buffer->data is null!!!\n",
			__func__);
		return;
	}

	buffer->length = length;
	buffer->start = -1;
	buffer->end = 0;
	buffer->status = FLAG_NOTHING;
	buffer->debugger = 0;
#if defined(RETURN_ALL_DATA_AT_ONCE)
	readbuffer = 0;
#endif
	memset(buffer->data, 0, length + 1);

	mutex_init(&buffer->mutex);
}

void DestroyBuffer(struct tRingBuffer *buffer)
{
	if (buffer->data != 0) {
#if defined(KPERFMON_KMALLOC)
		kfree(buffer->data);
#else
		vfree(buffer->data);
#endif
		buffer->data = 0;
	}
#if defined(RETURN_ALL_DATA_AT_ONCE)
	if (readbuffer != 0) {
		kfree(readbuffer);
		readbuffer = 0;
	}
#endif
}

void WriteBuffer(struct tRingBuffer *buffer,
			byte *data,
			unsigned long length)
{
	long RemainSize = 0;

	if (length < 0) {
		return;
	}

	if (buffer->length < buffer->end + length) {
		long FirstSize = buffer->length - buffer->end;

		WriteBuffer(buffer, data, FirstSize);
		WriteBuffer(buffer, data + FirstSize, length - FirstSize);
		return;
	}

	RemainSize = (buffer->start < buffer->end) ?
			(buffer->length - buffer->end) :
			(buffer->start - buffer->end);

	while (RemainSize < length) {
		int bstart = (buffer->start + HEADER_SIZE - 1);
		int cur_length = *(buffer->data + bstart % buffer->length);

		buffer->start += HEADER_SIZE + cur_length;
		buffer->start %= buffer->length;

		RemainSize = (buffer->start < buffer->end) ?
			(buffer->length - buffer->end) :
			(buffer->start - buffer->end);
	}

	memcpy(buffer->data + buffer->end, data, length);
	//copy_from_user(buffer->data + buffer->end, data, length);

	buffer->end += length;

	if (buffer->start < 0) {
		buffer->start = 0;
	}

	if (buffer->end >= buffer->length) {
		buffer->end = 0;
	}

	if (buffer->status != FLAG_READING) {
		buffer->position = buffer->start;
	}
}

void ReadBuffer(struct tRingBuffer *buffer,
			byte *data,
			unsigned long *length)
{
	if (buffer->start < buffer->end) {
		*length = buffer->end - buffer->start;
		memcpy(data, buffer->data + buffer->start, *length);
		//copy_to_user(data, (buffer->data + buffer->start), *length);
	} else {
		*length = buffer->length - buffer->start;
		memcpy(data, buffer->data + buffer->start, *length);
		memcpy(data + *length, buffer->data, buffer->end);
		//copy_to_user(data, (buffer->data + buffer->start), *length);
		//copy_to_user(data + *length, (buffer->data), buffer->end);
	}
}

void ReadBufferByPosition(struct tRingBuffer *buffer,
					byte *data,
					unsigned long *length,
					unsigned long start,
					unsigned long end)
{
	if (start < end) {
		*length = end - start;
		memcpy(data, buffer->data + start, *length);
	} else {
		*length = buffer->length - start;
		memcpy(data, buffer->data + start, *length);
		memcpy(data + *length, buffer->data, end);
	}
}

void GetNext(struct tRingBuffer *buffer)
{
	int bstart = (buffer->position + HEADER_SIZE - 1);
	int cur_length = *(buffer->data + bstart % buffer->length);

	buffer->position += HEADER_SIZE + cur_length;
	buffer->position %= buffer->length;
}

static const struct file_operations kperfmon_fops = {
	.read = kperfmon_read,
	.write = kperfmon_write,
};

void set_kperfmon_debugger_function(char *writebuffer)
{
	buffer.debugger = !buffer.debugger;

	pr_info("%s() - buffer.debugger : %d\n",
		__func__,
		(int)buffer.debugger);
}

void process_version_function(char *writebuffer)
{
	struct t_before_print *pprinter = NULL;
	int length = 0;

	if (writebuffer == NULL) {
		return;
	}

	pprinter = kmalloc(sizeof(struct t_before_print), GFP_ATOMIC);

	if (pprinter == NULL) {
		return;
	}

	length = strlen(writebuffer) + 1;
	pprinter->pdata = kmalloc(length, GFP_ATOMIC);

	if (pprinter->pdata == NULL) {
		kfree(pprinter);
		return;
	}

	strlcpy(pprinter->pdata, writebuffer, length);

	list_add_tail(&pprinter->list, &before_print_list);
}

int ops_write_buffer(struct tRingBuffer *buffer,
			byte *writebuffer, unsigned long length)
{
	unsigned long DataLength;

	if (buffer == NULL) {
		return length;
	}

	if (writebuffer[HEADER_SIZE - 1] > PERFLOG_BUFF_STR_MAX_SIZE) {
		writebuffer[HEADER_SIZE - 1] = PERFLOG_BUFF_STR_MAX_SIZE;
	}
	DataLength = writebuffer[HEADER_SIZE - 1] + HEADER_SIZE;

	mutex_lock(&buffer->mutex);
	WriteBuffer(buffer, writebuffer, DataLength);
	mutex_unlock(&buffer->mutex);
#if defined(KPERFMON_DEBUG)
	{
		int i;

		for (i = 0 ; i < 110 ; i++) {
			pr_info("%s(buffer.data[%d] : %c\n",
				__func__,
				i,
				buffer.data[i]);
		}
	}
#endif
	return length;
}

int ops_process_command(struct tRingBuffer *buffer,
			byte *writebuffer, unsigned long length)
{
	int idx;
	int max_commands = (int)(sizeof(commands) / sizeof(struct t_command)); /*commands[].command should be const string array.*/

	for (idx = 0 ; idx < max_commands ; idx++) {
		int cmd_length = strlen(commands[idx].command);

		if (cmd_length >= HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE) {
			continue;
		}

		if (!strncmp(writebuffer, commands[idx].command, cmd_length)
		    && commands[idx].func != NULL
		    && strlen(writebuffer) > cmd_length) {
			commands[idx].func(writebuffer);
			return length;
		}
	}

	return length;
}

ssize_t kperfmon_write(struct file *filp,
				const char __user *data,
				size_t length,
				loff_t *loff_data)
{
	byte writebuffer[HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE + SH_IDX_PACKET + 1] = {0, };
	unsigned long DataLength = length;
	int max_write_ops = (int)(sizeof(write_opts) / sizeof(void *));
	int type_of_data;

	if (!buffer.data) {
		pr_info("%s() - Error buffer allocation is failed!!!\n",
			__func__);
		return length;
	}

	if (length <= 0) {
		pr_info("%s() - Error length : %d", __func__, length);
		return length;
	}

	if (DataLength > (HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE + SH_IDX_PACKET)) {
		DataLength = HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE + SH_IDX_PACKET;
	}

	if (copy_from_user(writebuffer, data, DataLength)) {
		return length;
	}

        // [[[ This will be replaced with below code
	type_of_data = writebuffer[SH_TYPE];

	if (type_of_data < max_write_ops && type_of_data >= 0) {
		return write_opts[type_of_data](&buffer,
			writebuffer + SH_IDX_PACKET,
			DataLength - SH_IDX_PACKET);
	}
        // This will be replaced with below code ]]]

	type_of_data -= (int)'0';

	if (type_of_data < max_write_ops && type_of_data >= 0) {
		return write_opts[type_of_data](&buffer,
			writebuffer + SH_IDX_PACKET,
			DataLength - SH_IDX_PACKET);
	}

	return length;
}

ssize_t kperfmon_read(struct file *filp,
				char __user *data,
				size_t count,
				loff_t *loff_data)
{
	unsigned long length;

#if defined(RETURN_ALL_DATA_AT_ONCE) // Return Total Data
	buffer.status = FLAG_READING;

	printk(KERN_INFO "kperfmon_read(count : %d)\n", (int)count);
	printk(KERN_INFO "kperfmon_read(buffer.start : %d)\n", (int)buffer.start);
	printk(KERN_INFO "kperfmon_read(buffer.end : %d)\n", (int)buffer.end);

	if (readbuffer == 0) {
		readbuffer = kmalloc(BUFFER_SIZE + 1, GFP_KERNEL);
	}
	printk(KERN_INFO "kperfmon_read(readbuffer ----)\n");
	mutex_lock(&buffer.mutex);
	ReadBuffer(&buffer, readbuffer, &length);
	mutex_unlock(&buffer.mutex);
	printk(KERN_INFO "kperfmon_read(length : %d)\n", (int)length);

#if defined(KPERFMON_KMALLOC)
	if (copy_to_user(data, readbuffer, length)) {
		printk(KERN_INFO "kperfmon_read(copy_to_user returned 0)\n");
		buffer.status = FLAG_NOTHING;
		return 0;
	}
	//printk(KERN_INFO "kperfmon_read(copy_to_user returned 0)\n");
#else
	memcpy(data, readbuffer, DataLength);
#endif

	if (readbuffer != 0) {
		kfree(readbuffer);
		readbuffer = 0;
	}

	buffer.status = FLAG_NOTHING;
	return 0;
#else // Return Line by Line
	byte readbuffer[READ_BUFFER_SIZE] = {0, };
	union _uPLogPacket readlogpacket;
	char timestamp[32] = {0, };

	unsigned long start = 0;
	unsigned long end = 0;

	if (!buffer.data) {
		pr_info("%s() Error buffer allocation is failed!\n", __func__);
		return 0;
	}
#if defined(USE_MONITOR)
	if (buffer.position == buffer.start) {
		char mutex_log[PERFLOG_BUFF_STR_MAX_SIZE + 1] = {0, };
		int i, idx_mutex_log = 0;

		idx_mutex_log += snprintf((mutex_log + idx_mutex_log),
					PERFLOG_BUFF_STR_MAX_SIZE - idx_mutex_log,
					"mutex test ");

		for (i = 0;
		    i <= MAX_MUTEX_RAWDATA &&
		    idx_mutex_log < (PERFLOG_BUFF_STR_MAX_SIZE - 20);
		    i++) {

			int digit, flag = 0;

			mutex_log[idx_mutex_log++] = '[';
			idx_mutex_log += snprintf((mutex_log + idx_mutex_log),
						PERFLOG_BUFF_STR_MAX_SIZE - idx_mutex_log,
						"%d",
						i);
			mutex_log[idx_mutex_log++] = ']';
			mutex_log[idx_mutex_log++] = ':';
			//idx_mutex_log += snprintf((mutex_log + idx_mutex_log),
			//			PERFLOG_BUFF_STR_MAX_SIZE - idx_mutex_log,
			//			"%d",
			//			mutex_rawdata[i]);
			//mutex_rawdata[i][1] = 99999999;
			for (digit = (MAX_MUTEX_RAWDATA_DIGIT-1) ; digit >= 0 ; digit--) {
				if (flag) {
					idx_mutex_log += snprintf((mutex_log + idx_mutex_log),
								PERFLOG_BUFF_STR_MAX_SIZE - idx_mutex_log,
								"%08u",
								mutex_rawdata[i][digit]);
				} else {
					if (mutex_rawdata[i][digit] > 0) {
						idx_mutex_log += snprintf((mutex_log + idx_mutex_log),
									PERFLOG_BUFF_STR_MAX_SIZE - idx_mutex_log,
									"%u",
									mutex_rawdata[i][digit]);
						flag = 1;
					}
				}
			}

			if (!flag) {
				mutex_log[idx_mutex_log++] = '0';
			}

			mutex_log[idx_mutex_log++] = ' ';
		}

		_perflog(PERFLOG_EVT, PERFLOG_MUTEX, mutex_log);
	}
#endif
	buffer.status = FLAG_READING;

	mutex_lock(&buffer.mutex);

	if (buffer.position == buffer.start) {

		if (before_list_cur_pos !=
		   list_last_entry(&before_print_list, typeof(*before_list_cur_pos), list)) {
			before_list_cur_pos = list_next_entry(before_list_cur_pos, list);

			if (before_list_cur_pos != 0 && before_list_cur_pos->pdata != 0) {
				int length = snprintf(readbuffer,
							READ_BUFFER_SIZE,
							"%s\n",
							before_list_cur_pos->pdata);

				if (copy_to_user(data, readbuffer, length)) {
					pr_info("%s(copy_to_user(4) returned > 0)\n", __func__);
					mutex_unlock(&buffer.mutex);
					buffer.status = FLAG_NOTHING;
					return 0;
				}

				mutex_unlock(&buffer.mutex);
				return length;
			}
		}
	}

	if (buffer.position == buffer.end || buffer.start < 0) {
		buffer.position = buffer.start;
		mutex_unlock(&buffer.mutex);
		buffer.status = FLAG_NOTHING;
		before_list_cur_pos
			= list_first_entry(&before_print_list, typeof(*before_list_cur_pos), list);
		return 0;
	}

	start = buffer.position;
	GetNext(&buffer);
	end = buffer.position;

	//printk("kperfmon_read(start : %d, end : %d)\n", (int)start, (int)end);

	if (start == end) {
		buffer.position = buffer.start;
		mutex_unlock(&buffer.mutex);
		buffer.status = FLAG_NOTHING;
		return 0;
	}

	//ReadPacket.raw = &rawpacket;
	ReadBufferByPosition(&buffer, readlogpacket.stream, &length, start, end);
	mutex_unlock(&buffer.mutex);
	//printk(KERN_INFO "kperfmon_read(length : %d)\n", (int)length);
	//readlogpacket.stream[length++] = '\n';
	readlogpacket.stream[length] = 0;

#if 0
	change2localtime(timestamp, readlogpacket.itemes.timestemp_sec);
#else
	snprintf(timestamp, 32, "%02d-%02d %02d:%02d:%02d.%03d",
			readlogpacket.itemes.timestamp.month,
			readlogpacket.itemes.timestamp.day,
			readlogpacket.itemes.timestamp.hour,
			readlogpacket.itemes.timestamp.minute,
			readlogpacket.itemes.timestamp.second,
			readlogpacket.itemes.timestamp.msecond);
#endif

	if (readlogpacket.itemes.type >= OlogTestEnum_Type_maxnum
	    || readlogpacket.itemes.type < 0) {
		readlogpacket.itemes.type = PERFLOG_LOG;
	}

	if (readlogpacket.itemes.id >= OlogTestEnum_ID_maxnum
	    || readlogpacket.itemes.id < 0) {
		readlogpacket.itemes.id = PERFLOG_UNKNOWN;
	}

	length = snprintf(readbuffer, READ_BUFFER_SIZE,
				"[%s %d %5d %5d (%3d)][%s][%s] %s\n",
				timestamp,
				readlogpacket.itemes.type,
				readlogpacket.itemes.pid,
				readlogpacket.itemes.tid,
				readlogpacket.itemes.context_length,
				OlogTestEnum_Type_strings[readlogpacket.itemes.type],
				OlogTestEnum_ID_strings[readlogpacket.itemes.id],
				readlogpacket.itemes.context_buffer);


	if (length > count) {
		length = count;
	}

	if (buffer.debugger && count > DEBUGGER_SIZE) {
		char debugger[DEBUGGER_SIZE] = "______________________________";

		snprintf(debugger, DEBUGGER_SIZE, "S:%010lu_E:%010lu_____", start, end);

		if (length + DEBUGGER_SIZE > count) {
			length = count - DEBUGGER_SIZE;
		}

		if (copy_to_user(data, debugger, strnlen(debugger, DEBUGGER_SIZE))) {
			pr_info("%s(copy_to_user(1) returned > 0)\n", __func__);
			return 0;
		}

		if (copy_to_user(data + DEBUGGER_SIZE, readbuffer, length)) {
			pr_info("%s(copy_to_user(2) returned > 0)\n", __func__);
			return 0;
		}

		length += DEBUGGER_SIZE;
	} else {
		if (copy_to_user(data, readbuffer, length)) {
			pr_info("%s(copy_to_user(3) returned > 0)\n", __func__);
			return 0;
		}
	}

	//printk(KERN_INFO "kperfmon_read(count : %d)\n", count);


	return length;
#endif
}

static int __init kperfmon_init(void)
{
	struct proc_dir_entry *entry;

	CreateBuffer(&buffer, BUFFER_SIZE);

	if (!buffer.data) {
		printk("kperfmon_init() - Error buffer allocation is failed!!!\n");
		return -ENOMEM;
	}

	entry = proc_create(PROC_NAME, 0664, NULL, &kperfmon_fops);

	if (!entry) {
		printk("kperfmon_init() - Error creating entry in proc failed!!!\n");
		DestroyBuffer(&buffer);
		return -EBUSY;
	}

	/*dbg_level_is_low = (sec_debug_level() == ANDROID_DEBUG_LEVEL_LOW);*/

	INIT_LIST_HEAD(&before_print_list);
	before_list_cur_pos =
		list_first_entry(&before_print_list, typeof(*before_list_cur_pos), list);
	process_version_function("  ");
	process_version_function("kperfmon_version [1.0]");

	printk(KERN_INFO "kperfmon_init()\n");

	return 0;
}

static void __exit kperfmon_exit(void)
{
	DestroyBuffer(&buffer);
	printk(KERN_INFO "kperfmon_exit()\n");
}

#if defined(USE_WORKQUEUE)
static void ologk_workqueue_func(struct work_struct *work)
{
	t_ologk_work *workqueue = (t_ologk_work *)work;

	if (work) {
		mutex_lock(&buffer.mutex);
		WriteBuffer(&buffer,
			workqueue->writelogpacket.stream,
			PERFLOG_HEADER_SIZE + workqueue->writelogpacket.itemes.context_length);
		mutex_unlock(&buffer.mutex);

		kfree((void *)work);
	}
}
#endif

void _perflog(int type, int logid, const char *fmt, ...)
{
#if !defined(USE_WORKQUEUE)
	union _uPLogPacket writelogpacket;
#endif
	struct rtc_time tm;
	struct timeval time;
	unsigned long local_time;
#if defined(USE_WORKQUEUE)
	t_ologk_work *workqueue = 0;
#endif
	va_list args;

	va_start(args, fmt);

	if (buffer.data == 0) {
		va_end(args);
		return;
	}


#if defined(USE_WORKQUEUE)
	workqueue = (t_ologk_work *)kmalloc(sizeof(t_ologk_work), GFP_ATOMIC);

	if (workqueue) {
		struct _PLogPacket *pitemes = &workqueue->writelogpacket.itemes;

		INIT_WORK((struct work_struct *)workqueue, ologk_workqueue_func);

		do_gettimeofday(&time);
		local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
		rtc_time_to_tm(local_time, &tm);

		//printk(" @ (%04d-%02d-%02d %02d:%02d:%02d)\n",
		//	tm.tm_year + 1900,
		//	tm.tm_mon + 1,
		//	tm.tm_mday,
		//	tm.tm_hour,
		//	tm.tm_min,
		//	tm.tm_sec);

		pitemes->timestamp.month = tm.tm_mon + 1;
		pitemes->timestamp.day = tm.tm_mday;
		pitemes->timestamp.hour = tm.tm_hour;
		pitemes->timestamp.minute = tm.tm_min;
		pitemes->timestamp.second = tm.tm_sec;
		pitemes->timestamp.msecond = time.tv_usec / 1000;
		pitemes->type = PERFLOG_LOG;
		pitemes->id = logid;
		pitemes->pid = current->pid;//getpid();
		pitemes->tid = 0;//gettid();
		pitemes->context_length = vscnprintf(
						pitemes->context_buffer,
						PERFLOG_BUFF_STR_MAX_SIZE,
						fmt,
						args);

		if (pitemes->context_length > PERFLOG_BUFF_STR_MAX_SIZE) {
			pitemes->context_length = PERFLOG_BUFF_STR_MAX_SIZE;
		}

		schedule_work((struct work_struct *)workqueue);

		/*{
			struct timeval end_time;
			do_gettimeofday(&end_time);
			printk("ologk() execution time with workqueue : %ld us ( %ld - %ld )\n",
					end_time.tv_usec - time.tv_usec,
					end_time.tv_usec,
					time.tv_usec);
		}*/
	} else {
		printk(KERN_INFO "_perflog : workqueue is not working\n");
	}

#else
	do_gettimeofday(&time);
	local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
	rtc_time_to_tm(local_time, &tm);

	//printk(" @ (%04d-%02d-%02d %02d:%02d:%02d)\n",
	//	tm.tm_year + 1900,
	//	tm.tm_mon + 1,
	//	tm.tm_mday,
	//	tm.tm_hour,
	//	tm.tm_min,
	//	tm.tm_sec);

	writelogpacket.itemes.timestamp.month = tm.tm_mon + 1;
	writelogpacket.itemes.timestamp.day = tm.tm_mday;
	writelogpacket.itemes.timestamp.hour = tm.tm_hour;
	writelogpacket.itemes.timestamp.minute = tm.tm_min;
	writelogpacket.itemes.timestamp.second = tm.tm_sec;
	writelogpacket.itemes.timestamp.msecond = time.tv_usec / 1000;
	writelogpacket.itemes.type = type;
	writelogpacket.itemes.pid = current->pid;//getpid();
	writelogpacket.itemes.tid = 0;//gettid();
	writelogpacket.itemes.context_length
		= vscnprintf(writelogpacket.itemes.context_buffer,
							PERFLOG_BUFF_STR_MAX_SIZE,
							fmt,
							args);

	if (writelogpacket.itemes.context_length > PERFLOG_BUFF_STR_MAX_SIZE) {
		writelogpacket.itemes.context_length = PERFLOG_BUFF_STR_MAX_SIZE;
	}

	mutex_lock(&buffer.mutex);
	WriteBuffer(&buffer,
			writelogpacket.stream,
			PERFLOG_HEADER_SIZE + writelogpacket.itemes.context_length);
	mutex_unlock(&buffer.mutex);

	/*{
		struct timeval end_time;
		do_gettimeofday(&end_time);
		printk(KERN_INFO "ologk() execution time : %ld us ( %ld - %ld )\n",
				end_time.tv_usec - time.tv_usec,
				end_time.tv_usec, time.tv_usec);
	}*/
#endif

	va_end(args);
}

void get_callstack(char *buffer, int max_size, int max_count)
{
	struct stackframe frame;
	struct task_struct *tsk = current;
	//int len;

	if (!try_get_task_stack(tsk))
		return;

	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.pc = (unsigned long)get_callstack;

#if defined(CONFIG_FUNCTION_GRAPH_TRACER)
	frame.graph = tsk->curr_ret_stack;
#endif
	if (max_size > 0) {
		int count = 0;

		max_count += 3;

		do {
			if (count > 2) {
				int len = snprintf(buffer, max_size, " %pS", (void *)frame.pc);
				max_size -= len;
				buffer += len;
			}
			count++;
		} while (!unwind_frame(tsk, &frame) &&
				max_size > 0 &&
				max_count > count);

		put_task_stack(tsk);
	}
}

void send_signal(void)
{
	siginfo_t info;

	info.si_signo = SIGNAL_35;
	info.si_errno = SIGNAL_OLOG;
	info.si_code = SIGNAL_OLOG;
	send_sig_info(SIGNAL_35, &info, current);
}

void perflog_evt(int logid, int arg1)
{
#if defined(USE_MONITOR)
	struct timeval start_time;
	struct timeval end_time;

	int digit = 0;

	do_gettimeofday(&start_time);
#endif
	if (arg1 < 0 || buffer.status != FLAG_NOTHING) {
		return;
	}

	if (arg1 > MAX_MUTEX_RAWDATA) {
		char log_buffer[PERFLOG_BUFF_STR_MAX_SIZE];
		int len;
		u64 utime, stime;

		task_cputime(current, &utime, &stime);

		if (utime > 0) {
			len = snprintf(log_buffer,
					PERFLOG_BUFF_STR_MAX_SIZE,
					"%d jiffies",
					arg1);
			/*send_signal();*/ // Make some stuck problems to be needed to check how many the mutex logging are occured. Refer to P200523-00343, P200523-01815.

			get_callstack(log_buffer + len,
					PERFLOG_BUFF_STR_MAX_SIZE - len,
					/*(dbg_level_is_low ? 1 : 3)*/MAX_DEPTH_OF_CALLSTACK);
			_perflog(PERFLOG_EVT, PERFLOG_MUTEX, log_buffer);
			arg1 = MAX_MUTEX_RAWDATA;

			//do_gettimeofday(&end_time);
			//_perflog(PERFLOG_EVT,
			//		PERFLOG_MUTEX,
			//		"[MUTEX] processing time : %d",
			//		end_time.tv_usec - start_time.tv_usec);
		}
	}
#if defined(USE_MONITOR)
	for (digit = 0 ; digit < MAX_MUTEX_RAWDATA_DIGIT ; digit++) {
		mutex_rawdata[arg1][digit]++;
		if (mutex_rawdata[arg1][digit] >= DIGIT_UNIT) {
			mutex_rawdata[arg1][digit] = 0;
		} else {
			break;
		}
	}
#endif
}

//EXPORT_SYMBOL(ologk);
EXPORT_SYMBOL(_perflog);
EXPORT_SYMBOL(perflog_evt);

module_init(kperfmon_init);
module_exit(kperfmon_exit);

