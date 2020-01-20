#include <linux/ologk.h>
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
#if !defined(KPERFMON_KMALLOC)
#include <linux/vmalloc.h>
#endif
#include <asm/uaccess.h>

#if defined(CONFIG_ARCH_EXYNOS)
#include <../../../../../../system/core/liblog/include/log/perflog.h>
#elif defined(CONFIG_ARCH_QCOM)
#include <../../../../../system/core/liblog/include/log/perflog.h>
#endif

typedef unsigned char byte;

#define	PROC_NAME	"kperfmon"
#if defined(KPERFMON_KMALLOC)
#define BUFFER_SIZE	5 * 1024
#else
#define BUFFER_SIZE	5 * 1024 * 1024
#endif

#define HEADER_SIZE	PERFLOG_HEADER_SIZE
#define DEBUGGER_SIZE 32
#define FLAG_NOTHING  0
#define FLAG_READING  1
#define USE_WORKQUEUE 1

struct tRingBuffer
{
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
//static struct workqueue_struct *ologk_wq = 0;

typedef struct {
	struct work_struct ologk_work;
	union _uPLogPacket writelogpacket;
} t_ologk_work;
#endif

#if 0 // Return Total Data
byte *readbuffer = 0;
#endif 

void CreateBuffer(struct tRingBuffer *buffer, unsigned long length);
void DestroyBuffer(struct tRingBuffer *buffer);
void WriteBuffer(struct tRingBuffer *buffer, byte *data, unsigned long length);
void GetNext(struct tRingBuffer *buffer);
void ReadBuffer(struct tRingBuffer *buffer, byte *data, unsigned long *length);
ssize_t kperfmon_write(struct file *filp, const char __user *data, size_t length, loff_t *loff_data);
ssize_t kperfmon_read(struct file *filp, char __user *data, size_t count, loff_t *loff_data);

void CreateBuffer(struct tRingBuffer *buffer, unsigned long length)
{
	if(buffer->data != 0) {
		return;
	}
	
#if defined(KPERFMON_KMALLOC)
	buffer->data = kmalloc(length + 1, GFP_KERNEL);//(byte *)malloc(sizeof(byte) * (length + 1));
#else
	buffer->data = vmalloc(length + 1);
#endif
	if (!buffer->data) {
		printk(KERN_INFO "kperfmon memory allocation is failed!!!\n");
		return;
	}
	
	buffer->length = length;
	buffer->start = -1;
	buffer->end = 0;
	buffer->status = FLAG_NOTHING;
	buffer->debugger = 0;
#if 0 // Return Total Data
	readbuffer = 0;
#endif
	memset(buffer->data, 0, length + 1);
	
	mutex_init(&buffer->mutex);
}

void DestroyBuffer(struct tRingBuffer *buffer)
{
	if(buffer->data != 0) {
#if defined(KPERFMON_KMALLOC)
		kfree(buffer->data);
#else
		vfree(buffer->data);
#endif
		buffer->data = 0;
	}
#if 0 // Return Total Data
	if(readbuffer != 0) {
		kfree(readbuffer);
		readbuffer = 0;
	}
#endif
}

void WriteBuffer(struct tRingBuffer *buffer, byte *data, unsigned long length)
{
	long RemainSize = 0;

	if (length < 0)
	{
		return;
	}
	if (buffer->length < buffer->end + length)
	{
		long FirstSize = buffer->length - buffer->end;

		WriteBuffer(buffer, data, FirstSize);
		WriteBuffer(buffer, data + FirstSize, length - FirstSize);
		return;
	}

//	RemainSize = (buffer->start == buffer->end) ? buffer->length : ( (buffer->start < buffer->end) ? (buffer->length - buffer->end) : (buffer->start - buffer->end) );
	RemainSize = (buffer->start < buffer->end) ? (buffer->length - buffer->end) : (buffer->start - buffer->end);

	while(RemainSize < length)
	{
		buffer->start += HEADER_SIZE + *(buffer->data + (buffer->start + HEADER_SIZE - 1) % buffer->length);
		buffer->start %= buffer->length;

		RemainSize = (buffer->start < buffer->end) ? (buffer->length - buffer->end) : (buffer->start - buffer->end);
	}

	memcpy(buffer->data + buffer->end, data, length);
	//copy_from_user(buffer->data + buffer->end, data, length);

	buffer->end += length;

	if (buffer->start < 0)
	{
		buffer->start = 0;
	}
	if (buffer->end >= buffer->length)
	{
		buffer->end = 0;
	}
	
	if(buffer->status != FLAG_READING) {
		buffer->position = buffer->start;
	}
}

void ReadBuffer(struct tRingBuffer *buffer, byte *data, unsigned long *length)
{
	if (buffer->start < buffer->end)
	{
		*length = buffer->end - buffer->start;
		memcpy(data, buffer->data + buffer->start, *length);
		//copy_to_user(data, (buffer->data + buffer->start), *length);
	}
	else
	{
		*length = buffer->length - buffer->start;
		memcpy(data, buffer->data + buffer->start, *length);
		memcpy(data + *length, buffer->data, buffer->end);
		//copy_to_user(data, (buffer->data + buffer->start), *length);
		//copy_to_user(data + *length, (buffer->data), buffer->end);
	}
}

void ReadBufferByPosition(struct tRingBuffer *buffer, byte *data, unsigned long *length, unsigned long start, unsigned long end)
{
	if (start < end)
	{
		*length = end - start;
		memcpy(data, buffer->data + start, *length);
	}
	else
	{
		*length = buffer->length - start;
		memcpy(data, buffer->data + start, *length);
		memcpy(data + *length, buffer->data, end);
	}
}

void GetNext(struct tRingBuffer *buffer)
{
	buffer->position += HEADER_SIZE + *(buffer->data + (buffer->position + HEADER_SIZE - 1) % buffer->length);
	buffer->position %= buffer->length;
}

static const struct file_operations kperfmon_fops = { \
	.read = kperfmon_read,                        \
	.write = kperfmon_write,                      
};

ssize_t kperfmon_write(struct file *filp, const char __user *data, size_t length, loff_t *loff_data)
{
	byte writebuffer[HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE] = {0, };
	unsigned long DataLength = length;

	if(!buffer.data) {
		printk(KERN_INFO "kperfmon_write() - Error buffer allocation is failed!!!\n");
		return length;
	}
	//printk(KERN_INFO "kperfmon_write(DataLength : %d)\n", (int)DataLength);

	if(DataLength > (HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE)) {
		DataLength = HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE;
	}

	if (copy_from_user(writebuffer, data, DataLength)) {
		return length;
	}

	if(!strncmp(writebuffer, "kperfmon_debugger", 17)) {
		if(buffer.debugger == 1) {
			buffer.debugger = 0;
		} else {
			buffer.debugger = 1;
		}
		
		printk(KERN_INFO "kperfmon_write(buffer.debugger : %d)\n", (int)buffer.debugger);
		return length;
	}


	//printk(KERN_INFO "kperfmon_write(DataLength : %d, %c, %s)\n", (int)DataLength, writebuffer[HEADER_SIZE - 1], writebuffer);
	//printk(KERN_INFO "kperfmon_write(DataLength : %d, %d)\n", writebuffer[0], writebuffer[1]);

	if(PERFLOG_BUFF_STR_MAX_SIZE < writebuffer[HEADER_SIZE - 1]) {
		writebuffer[HEADER_SIZE - 1] = PERFLOG_BUFF_STR_MAX_SIZE;
	}
	DataLength = writebuffer[HEADER_SIZE - 1] + HEADER_SIZE;
	//printk(KERN_INFO "kperfmon_write(DataLength : %d)\n", (int)DataLength);
	
	mutex_lock(&buffer.mutex);
	WriteBuffer(&buffer, writebuffer, DataLength);
	mutex_unlock(&buffer.mutex);
#if 0
	{
		int i;
		for(i = 0 ; i < 110 ; i++) {
			printk(KERN_INFO "kperfmon_write(buffer.data[%d] : %c\n", i, buffer.data[i]);
		}
	}
#endif
	return length;
}

ssize_t kperfmon_read(struct file *filp, char __user *data, size_t count, loff_t *loff_data)
{
	unsigned long length;
	
#if 0 // Return Total Data
	buffer.status = FLAG_READING;
	
	printk(KERN_INFO "kperfmon_read(count : %d)\n", (int)count);
	printk(KERN_INFO "kperfmon_read(buffer.start : %d)\n", (int)buffer.start);
	printk(KERN_INFO "kperfmon_read(buffer.end : %d)\n", (int)buffer.end);
	
	if(readbuffer == 0) {
		readbuffer = kmalloc(BUFFER_SIZE + 1, GFP_KERNEL);
	}
	printk(KERN_INFO "kperfmon_read(readbuffer ----)\n");
	mutex_lock(&buffer.mutex);
	ReadBuffer(&buffer, readbuffer, &length);
	mutex_unlock(&buffer.mutex);
	printk(KERN_INFO "kperfmon_read(length : %d)\n", (int)length);

#if defined(KPERFMON_KMALLOC)
	if (copy_to_user(data, readbuffer, length)) {
		printk(KERN_INFO "kperfmon_read(copy_to_user retuned > 0)\n");
		buffer.status = FLAG_NOTHING;
		return 0;
	}
	printk(KERN_INFO "kperfmon_read(copy_to_user retuned 0)\n");
#else
	memcpy(data, readbuffer, DataLength);
#endif

	if(readbuffer != 0) {
		kfree(readbuffer);
		readbuffer = 0;
	}
	
	buffer.status = FLAG_NOTHING;
	return 0;
#else // Return Line by Line
	byte readbuffer[PERFLOG_BUFF_STR_MAX_SIZE + PERFLOG_HEADER_SIZE + 100] = {0, };
	union _uPLogPacket readlogpacket;
	char timestamp[32] = {0, };
	
	unsigned long start = 0;
	unsigned long end = 0;

	if(!buffer.data) {
		printk(KERN_INFO "kperfmon_read() - Error buffer allocation is failed!!!\n");
		return 0;
	}

	buffer.status = FLAG_READING;
		
	mutex_lock(&buffer.mutex);
	if(buffer.position == buffer.end || buffer.start < 0) {
		buffer.position = buffer.start;
		mutex_unlock(&buffer.mutex);
		buffer.status = FLAG_NOTHING;
		return 0;
	}

	start = buffer.position;
	GetNext(&buffer);
	end = buffer.position;
	
	//printk(KERN_INFO "kperfmon_read(start : %d, end : %d)\n", (int)start, (int)end);
		
	if(start == end) {
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
	snprintf(timestamp, 32, "%02d-%02d %02d:%02d:%02d.%03d", readlogpacket.itemes.timestamp.month, 
														readlogpacket.itemes.timestamp.day,
														readlogpacket.itemes.timestamp.hour,
														readlogpacket.itemes.timestamp.minute,
														readlogpacket.itemes.timestamp.second,
														readlogpacket.itemes.timestamp.msecond);
#endif

	length = snprintf(readbuffer, PERFLOG_BUFF_STR_MAX_SIZE + PERFLOG_HEADER_SIZE + 100, "[%s %d %d %d (%d)] %s\n", timestamp, 
															readlogpacket.itemes.type, 
															readlogpacket.itemes.pid, 
															readlogpacket.itemes.tid,
															readlogpacket.itemes.context_length,
															readlogpacket.itemes.context_buffer);

	if(buffer.debugger) {
		char debugger[DEBUGGER_SIZE] = "______________________________";
		
		snprintf(debugger, DEBUGGER_SIZE, "S:%010lu_E:%010lu_____", start, end);
		//memcpy(data, debugger, length);
		if (copy_to_user(data, debugger, strnlen(debugger,DEBUGGER_SIZE))) {
			printk(KERN_INFO "kperfmon_read(copy_to_user(data, readbuffer, length) retuned > 0)\n");
			return 0;
		}
		//memcpy(data + DEBUGGER_SIZE, readbuffer, length);
		if (copy_to_user(data + DEBUGGER_SIZE, readbuffer, length)) {
			printk(KERN_INFO "kperfmon_read(copy_to_user(data + DEBUGGER_SIZE, readbuffer, length) retuned > 0)\n");
			return 0;
		}

		length += DEBUGGER_SIZE;
	} else {
		//memcpy(data, readbuffer, length);
		if (copy_to_user(data, readbuffer, length)) {
			printk(KERN_INFO "kperfmon_read(copy_to_user(data, readbuffer, length) retuned > 0)\n");
			return 0;
		}			
	}

	//printk(KERN_INFO "kperfmon_read(long : %d)\n", (int)(sizeof(long)));
	
	return length;
#endif
}

static int __init kperfmon_init(void)
{
	struct proc_dir_entry* entry;

	CreateBuffer(&buffer, BUFFER_SIZE);

	if(!buffer.data) {
		printk(KERN_INFO "kperfmon_init() - Error buffer allocation is failed!!!\n");
		return -ENOMEM;
	}

	entry = proc_create(PROC_NAME, 0664, NULL, &kperfmon_fops);

	if ( !entry ) {
		printk( KERN_ERR "kperfmon_init() - Error creating entry in proc failed!!!\n" );
		DestroyBuffer(&buffer);
		return -EBUSY;
	}

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

	mutex_lock(&buffer.mutex);
	WriteBuffer(&buffer, workqueue->writelogpacket.stream, PERFLOG_HEADER_SIZE + workqueue->writelogpacket.itemes.context_length);
	mutex_unlock(&buffer.mutex);

	if(work) {
		kfree((void *)work);
	}
}
#endif

void ologk(const char *fmt, ...) 
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
	
	if(buffer.data == 0) {
		return;
	}


#if defined(USE_WORKQUEUE)
	workqueue = (t_ologk_work *)kmalloc(sizeof(t_ologk_work), GFP_KERNEL);

	if(workqueue) {
		INIT_WORK((struct work_struct *)workqueue, ologk_workqueue_func);

		do_gettimeofday(&time);
		local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
		rtc_time_to_tm(local_time, &tm);

		//printk(" @ (%04d-%02d-%02d %02d:%02d:%02d)\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
		workqueue->writelogpacket.itemes.timestamp.month = tm.tm_mon + 1;
		workqueue->writelogpacket.itemes.timestamp.day = tm.tm_mday;
		workqueue->writelogpacket.itemes.timestamp.hour = tm.tm_hour;
		workqueue->writelogpacket.itemes.timestamp.minute = tm.tm_min;
		workqueue->writelogpacket.itemes.timestamp.second = tm.tm_sec;
		workqueue->writelogpacket.itemes.timestamp.msecond = time.tv_usec / 1000;
		workqueue->writelogpacket.itemes.type = 0;
		workqueue->writelogpacket.itemes.pid = current->pid;//getpid();
		workqueue->writelogpacket.itemes.tid = 0;//gettid();
		workqueue->writelogpacket.itemes.context_length = vscnprintf(workqueue->writelogpacket.itemes.context_buffer, PERFLOG_BUFF_STR_MAX_SIZE, fmt, args);
	
		if(workqueue->writelogpacket.itemes.context_length > PERFLOG_BUFF_STR_MAX_SIZE) {
			workqueue->writelogpacket.itemes.context_length = PERFLOG_BUFF_STR_MAX_SIZE;
		}
	
		schedule_work((struct work_struct *)workqueue);
	
		/*{
			struct timeval end_time;
			do_gettimeofday(&end_time);
			printk(KERN_INFO "ologk() execution time with workqueue : %ld us ( %ld - %ld )\n", end_time.tv_usec - time.tv_usec, end_time.tv_usec, time.tv_usec);
		}*/
	}
#else
	do_gettimeofday(&time);
	local_time = (u32)(time.tv_sec - (sys_tz.tz_minuteswest * 60));
	rtc_time_to_tm(local_time, &tm);

	//printk(" @ (%04d-%02d-%02d %02d:%02d:%02d)\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	writelogpacket.itemes.timestamp.month = tm.tm_mon + 1;
	writelogpacket.itemes.timestamp.day = tm.tm_mday;
	writelogpacket.itemes.timestamp.hour = tm.tm_hour;
	writelogpacket.itemes.timestamp.minute = tm.tm_min;
	writelogpacket.itemes.timestamp.second = tm.tm_sec;
	writelogpacket.itemes.timestamp.msecond = time.tv_usec / 1000;
	writelogpacket.itemes.type = 0;
	writelogpacket.itemes.pid = current->pid;//getpid();
	writelogpacket.itemes.tid = 0;//gettid();
	writelogpacket.itemes.context_length = vscnprintf(writelogpacket.itemes.context_buffer, PERFLOG_BUFF_STR_MAX_SIZE, fmt, args);

	if(writelogpacket.itemes.context_length > PERFLOG_BUFF_STR_MAX_SIZE) {
		writelogpacket.itemes.context_length = PERFLOG_BUFF_STR_MAX_SIZE;
	}

	mutex_lock(&buffer.mutex);
	WriteBuffer(&buffer, writelogpacket.stream, PERFLOG_HEADER_SIZE + writelogpacket.itemes.context_length);
	mutex_unlock(&buffer.mutex);
	
	/*{
		struct timeval end_time;
		do_gettimeofday(&end_time);
		printk(KERN_INFO "ologk() execution time : %ld us ( %ld - %ld )\n", end_time.tv_usec - time.tv_usec, end_time.tv_usec, time.tv_usec);
	}*/
#endif

	va_end(args);
}
EXPORT_SYMBOL(ologk);

module_init(kperfmon_init);
module_exit(kperfmon_exit);
