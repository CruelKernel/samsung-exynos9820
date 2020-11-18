/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/sched/clock.h>

#include "npu-device.h"
#include "npu-debug.h"
#include "npu-log.h"
#include "npu-interface.h"

/* Non-printable character to mark the last dump postion is overwritten or not */
const char NPU_LOG_DUMP_MARK = (char)0x01;

/* Log level to filter message written to kernel log */
struct npu_log npu_log = {
	.pr_level = NPU_LOG_INFO,	/* To kmsg */
	.st_level = NPU_LOG_DBG,	/* To memory buffer */
	.st_buf = NULL,
	.st_size = 0,
	.wr_pos = 0,
	.line_cnt = 0,
	.last_dump_line_cnt = 0,
	.last_dump_mark_pos = 0,
	.fs_ref = ATOMIC_INIT(0),
};

/*
*	In fw_report, some var(line_cnt) will be used with another purpose.
*	this var will be used finish line when write pointer circled to start line.
*	So, buffer in file object does not know about ...
*/
struct npu_log fw_report = {
	.st_buf = NULL,
	.st_size = 0,
	.wr_pos = 0,
	.line_cnt = 0,
	.last_dump_line_cnt = 0,
};
/* Spinlock for memory logger */
static DEFINE_SPINLOCK(npu_log_lock);
static DEFINE_SPINLOCK(fw_report_lock);

/* Temporal buffer */
#define NPU_LOG_MSG_MAX		1024

const char *LOG_LEVEL_NAME[NPU_LOG_INVALID] = {
	"Trace", "Debug", "Info", "Warning", "Error", "None"
};
const char LOG_LEVEL_MARK[NPU_LOG_INVALID] = {
	'T', 'D', 'I', 'W', 'E', '_'
};

/*
 * ISR-aware spin lock for log implementation
 *
 * returns 0 if locking is successful.
 *  - Case 1 : Current context is not interrupt. and spin_lock() was successful.
 *  - Case 2 : Current context is interrupt. and spin_trylock() was successful.
 * returns -EINTR if
 *  - Current context is interrupt, and spin_trylock() returns zero
 *    (i.e. spin_lock is currently held)
 *    Caller should not access shared resource in this case
 */
static inline int spin_lock_safe_isr(spinlock_t *lock)
{
	int	ret;

	if (unlikely(in_interrupt())) {
		ret = spin_trylock(lock);
		if (ret == 0)
			return -EINTR;
	} else {
		spin_lock(lock);
	}
	return 0;
}

/*
 * 0 : log success
 * != 0 : Failure
 */
int npu_store_log(npu_log_level_e loglevel, const char *fmt, ...)
{
	int		ret;
	size_t		pr_size;
	size_t		wr_len = 0;
	size_t		remain;
	va_list		arg_ptr;
	char		*buf;

	/*
	 * Do not use store log on interrupt context, because it requires
	 * use of spin_lock_irqsave() which lowers system's responsiveness
	 * (The message will be forwarded to pr_XXXX macro instead)
	 */
	ret = spin_lock_safe_isr(&npu_log_lock);
	if (ret)
		goto imm_exit;

	remain = npu_log.st_size - npu_log.wr_pos;
	buf = npu_log.st_buf + npu_log.wr_pos;

	if (unlikely(!npu_log.st_buf)) {
		ret = -ENOENT;
		goto unlock_exit;
	}

	if ((npu_log.line_cnt & NPU_STORE_LOG_SYNC_MARK_INTERVAL_MASK) == 0)
		pr_info(NPU_STORE_LOG_SYNC_MARK_MSG, npu_log.line_cnt);

	/* Execute from start */
	goto start;

retry:
	/* Executed when hit the buffer end during the writing messages. */
	if (unlikely(buf == npu_log.st_buf)) {
		ret = -ENOMEM;
		goto err_exit;		/* Buffer is too short */
	}

	/* Fill the remain buffer as separator line and move to start pos to retry */
	memset(buf, '#', npu_log.st_size - npu_log.wr_pos);
	npu_log.st_buf[npu_log.st_size - 1] = '\n';
	buf = npu_log.st_buf;
	remain = npu_log.st_size;
	npu_log.wr_pos = 0;
	wr_len = 0;

start:
	if ((npu_log.line_cnt & NPU_STORE_LOG_SYNC_MARK_INTERVAL_MASK) == 0) {
		pr_size = scnprintf(buf + wr_len, remain, NPU_STORE_LOG_SYNC_MARK_MSG, npu_log.line_cnt);
		if (unlikely(pr_size < 0)) {
			ret = -EFAULT;
			goto err_exit;
		}
		remain -= pr_size;
		wr_len += pr_size;
		if ((remain <= 1) || (pr_size == 0)) {		/* Underflow on 'remain -= pr_size' */
			goto retry;
		}
	}

	pr_size = scnprintf(buf + wr_len, remain, "%016llu;%c;"
		, sched_clock(), LOG_LEVEL_MARK[loglevel]);
	if (unlikely(pr_size < 0)) {
		ret = -EFAULT;
		goto err_exit;
	}
	remain -= pr_size;
	wr_len += pr_size;
	if ((remain <= 1) || (pr_size == 0)) {		/* Underflow on 'remain -= pr_size' */
		goto retry;
	}

	va_start(arg_ptr, fmt);
	pr_size = vscnprintf(buf + wr_len, remain, fmt, arg_ptr);
	va_end(arg_ptr);
	if (unlikely(pr_size < 0)) {
		ret = -EFAULT;
		goto err_exit;
	}
	remain -= pr_size;
	wr_len += pr_size;
	if ((remain <= 1) || (pr_size == 0)) {		/* Underflow on 'remain -= pr_size' */
		goto retry;
	}

	/* Update write position */
	npu_log.wr_pos = (npu_log.wr_pos + wr_len) % npu_log.st_size;
	npu_log.line_cnt++;

	ret = 0;
	goto unlock_exit;

err_exit:
	pr_err("Log store error : remain: %zu wr_len: %zu pr_size : %zu ret :%d\n",
		remain, wr_len, pr_size, ret);

unlock_exit:
	spin_unlock(&npu_log_lock);

imm_exit:
	return ret;
}

void npu_store_log_init(char *buf_addr, const size_t size)
{
	BUG_ON(!buf_addr);
	BUG_ON(size < PAGE_SIZE);

	spin_lock(&npu_log_lock);

	npu_log.st_buf = buf_addr;
	buf_addr[0] = '\0';
	buf_addr[size - 1] = '\0';
	npu_log.st_size = size;
	npu_log.wr_pos = 0;
	spin_unlock(&npu_log_lock);

	npu_info("Store log memory initialized : %pK[Len = %zu]\n", buf_addr, size);
}

void npu_store_log_deinit(void)
{
	/* Wake-up readers and preserve some time to flush */
	wake_up_all(&npu_log.wq);

	/* Wait a few ms until all the readers dump their log */
	if (atomic_read(&npu_log.fs_ref) > 0) {
		npu_info("Waiting for all the logs are dumped via debufgs.\n");
		msleep(NPU_STORE_LOG_FLUSH_INTERVAL_MS);
	}

	npu_info("Store log memory deinitializing : %pK -> NULL\n", npu_log.st_buf);
	spin_lock(&npu_log_lock);
	npu_log.st_buf = NULL;
	npu_log.st_size = 0;
	npu_log.wr_pos = 0;
	/*
	 * Reset ref count as Zero. There may be other client left,
	 * but they will not reduce counter if current value is zero.
	 */
	atomic_set(&npu_log.fs_ref, 0);
	spin_unlock(&npu_log_lock);
}

void npu_fw_report_init(char *buf_addr, const size_t size)
{
	unsigned long	intr_flags;

	BUG_ON(!buf_addr);
	BUG_ON(size < PAGE_SIZE);

	spin_lock_irqsave(&fw_report_lock, intr_flags);
	fw_report.st_buf = buf_addr;
	buf_addr[0] = '\0';
	buf_addr[size - 1] = '\0';
	fw_report.st_size = size;
	fw_report.wr_pos = 0;
	fw_report.line_cnt = 0;

	spin_unlock_irqrestore(&fw_report_lock, intr_flags);

}

void npu_fw_report_deinit(void)
{
	unsigned long	intr_flags;

	/* Wake-up readers and preserve some time to flush */
	wake_up_all(&fw_report.wq);
	msleep(NPU_STORE_LOG_FLUSH_INTERVAL_MS);

	npu_info("fw_report memory deinitializing : %pK -> NULL\n", fw_report.st_buf);
	spin_lock_irqsave(&fw_report_lock, intr_flags);
	fw_report.st_buf = NULL;
	fw_report.st_size = 0;
	fw_report.wr_pos = 0;
	fw_report.line_cnt = 0;

	spin_unlock_irqrestore(&fw_report_lock, intr_flags);

}

const u32 READ_OBJ_MAGIC = 0x1090D358;
struct npu_store_log_read_obj {
	u32			magic;
	size_t			read_pos;
};

/*
 * Set readpos to specified offset from current wr_pos.
 * 0 means set offset as big as possible.
 */
static void npu_store_log_set_offset(struct npu_store_log_read_obj *robj, ssize_t offset)
{
	int	ret;
	ssize_t	new_rpos;
	ssize_t	wr_pos, st_size, last_dump_mark_pos;

	ret = spin_lock_safe_isr(&npu_log_lock);
	if (ret)
		goto imm_exit;

	st_size = npu_log.st_size;
	wr_pos = npu_log.wr_pos;
	last_dump_mark_pos = npu_log.last_dump_mark_pos;

	if (st_size < offset + PAGE_SIZE) {
		new_rpos = 0;
		goto unlock_exit;
	}

	/* Biggest offset -> buffer size - PAGE_SIZE */
	if (offset == 0)
		offset = st_size - PAGE_SIZE;

	new_rpos = 0;
	if (npu_log.st_buf) {
		/* Limit offset if last dump postion is available */
		if (npu_log.st_buf[last_dump_mark_pos] == NPU_LOG_DUMP_MARK) {
			/* offset = min(offset, distance between wr_pos and last_dump_mark_pos) */
			offset = min(offset,
				(wr_pos + st_size - (last_dump_mark_pos + 1)) % st_size);
		}

		if (npu_log.st_buf[st_size - 1] != '\0') {
			/* Buffer is rolled */
			new_rpos = (wr_pos + (st_size - offset)) % st_size;
		} else {
			/* Buffer is not rolled */
			if (offset > wr_pos)
				new_rpos = 0;
			else
				new_rpos = wr_pos - offset;
		}
	}

unlock_exit:
	BUG_ON(new_rpos < 0);
	robj->read_pos = (size_t)new_rpos;
	spin_unlock(&npu_log_lock);

imm_exit:
	return;
}

static int npu_store_log_fops_open(struct inode *inode, struct file *file)
{
	int				ret;
	struct npu_store_log_read_obj	*robj;

	robj = kzalloc(sizeof(*robj), GFP_ATOMIC);
	if (!robj) {
		ret = -ENOMEM;
		goto err_exit;
	}

	robj->magic = READ_OBJ_MAGIC;
	/* TODO: It would be more useful if read_pos can start from circular queue tail */

	file->private_data = robj;

	/* Increase reference counter */
	atomic_inc(&npu_log.fs_ref);

	npu_store_log_set_offset(robj, 0);

	npu_info("fd open robj @ %pK\n", robj);
	return 0;

err_exit:
	return ret;
}

static int npu_fw_report_fops_open(struct inode *inode, struct file *file)
{
	int				ret;
	struct npu_store_log_read_obj	*robj;

	robj = kzalloc(sizeof(*robj), GFP_ATOMIC);
	if (!robj) {
		ret = -ENOMEM;
		goto err_exit;
	}
	robj->read_pos = 0;
	robj->magic = READ_OBJ_MAGIC;
	/* TODO: It would be more useful if read_pos can start from circular queue tail */
	file->private_data = robj;

	npu_info("fd open robj @ %pK\n", robj);
	return 0;

err_exit:
	return ret;
}

static int npu_fw_report_fops_close(struct inode *inode, struct file *file)
{
	struct npu_store_log_read_obj	*robj;

	BUG_ON(!file);
	robj = file->private_data;
	BUG_ON(!robj);
	BUG_ON(robj->magic != READ_OBJ_MAGIC);

	npu_info("fd close robj @ %pK\n", robj);
	kfree(robj);

	return 0;

}

static int npu_store_log_fops_close(struct inode *inode, struct file *file)
{
	struct npu_store_log_read_obj	*robj;

	BUG_ON(!file);
	robj = file->private_data;
	BUG_ON(!robj);
	BUG_ON(robj->magic != READ_OBJ_MAGIC);

	/* Decrease reference counter */
	atomic_dec_if_positive(&npu_log.fs_ref);

	npu_info("fd close robj @ %pK\n", robj);
	kfree(robj);

	return 0;
}

/* Caller should acquire npu_log_lock spinlock before call this function */
static ssize_t __npu_store_log_fops_read(struct npu_store_log_read_obj *robj, char *outbuf, const size_t outlen)
{
	size_t				copy_len, buf_end;

	/* Copy data to temporary buffer*/
	if (npu_log.st_buf) {
		buf_end = (robj->read_pos > npu_log.wr_pos) ? npu_log.st_size : npu_log.wr_pos;
		copy_len = min(outlen, buf_end - robj->read_pos);
		memcpy(outbuf, npu_log.st_buf + robj->read_pos, copy_len);
	} else {
		buf_end = 0;
		copy_len = 0;
	}

	pr_debug("end: %zu  copy_len: %zu  wr_pos: %zu  read_pos: %zu\n", buf_end, copy_len, npu_log.wr_pos, robj->read_pos);

	if (copy_len > 0) {
		robj->read_pos = (robj->read_pos + copy_len) % npu_log.st_size;
		/* Set dump mark */
		npu_log.last_dump_mark_pos = (robj->read_pos - 1) % npu_log.st_size;
		npu_log.st_buf[npu_log.last_dump_mark_pos] = NPU_LOG_DUMP_MARK;
	}

	pr_debug("Buf = %pK, Read pos = %zu, Read len = %zu\n", npu_log.st_buf, robj->read_pos, copy_len);

	return copy_len;
}

static ssize_t npu_store_log_fops_read(struct file *file, char __user *outbuf, size_t outlen, loff_t *loff)
{
	struct npu_store_log_read_obj	*robj;
	ssize_t				ret, copy_len;
	size_t				tmp_buf_len;
	char				*tmp_buf = NULL;

	BUG_ON(!file);
	robj = file->private_data;
	BUG_ON(!robj);
	BUG_ON(robj->magic != READ_OBJ_MAGIC);

	/* Temporal kernel buffer, to read inside of spinlock */
	tmp_buf_len = min(outlen, PAGE_SIZE);
	tmp_buf = kzalloc(tmp_buf_len, GFP_KERNEL);
	if (!tmp_buf) {
		ret = -ENOMEM;
		goto err_exit;
	}

	/* Check data available */
	ret = 0;
	while (ret == 0) {	/* ret = 0 on timeout */
		/* TODO: Accessing npu_log.wr_pos outside of spinlock is potentially dangerous */
		ret = wait_event_interruptible_timeout(npu_log.wq, robj->read_pos != npu_log.wr_pos, 1 * HZ);
		if (ret == -ERESTARTSYS) {
			ret = 0;
			goto err_exit;
		}
	}

	ret = spin_lock_safe_isr(&npu_log_lock);
	if (ret)
		goto err_exit;

	copy_len = __npu_store_log_fops_read(robj, tmp_buf, tmp_buf_len);
	spin_unlock(&npu_log_lock);

	if (copy_len > 0) {
		ret = copy_to_user(outbuf, tmp_buf, copy_len);
		if (ret) {
			pr_err("copy_to_user failed : %zd\n", ret);
			ret = -EFAULT;
			goto err_exit;
		}
	}

	ret = copy_len;
err_exit:

	if (tmp_buf)
		kfree(tmp_buf);

	/* schedule() is insearted to prevent busy-waiting loop around npu_log_lock */
	schedule();
	return ret;
}

void npu_fw_report_store(char *strRep, int nSize)
{
	size_t	wr_len = 0;
	size_t	remain = fw_report.st_size - fw_report.wr_pos;
	char	*buf = fw_report.st_buf + fw_report.wr_pos;
	unsigned long intr_flags;

	spin_lock_irqsave(&fw_report_lock, intr_flags);

	//check overflow
	if (nSize >= (int)remain) {
		fw_report.st_buf[fw_report.wr_pos] = '\0';
		fw_report.line_cnt = fw_report.wr_pos;
		fw_report.wr_pos = 0;
		remain = fw_report.st_size;
		buf = fw_report.st_buf;
		fw_report.last_dump_line_cnt++;
	}

	memcpy(buf, strRep, nSize);
	remain -= nSize;
	wr_len += nSize;
	npu_dbg("fw_report nSize : %d,\t remain = %zu\n", nSize, remain);

	/* Update write position */
	fw_report.wr_pos = (fw_report.wr_pos + wr_len) % fw_report.st_size;
	fw_report.st_buf[fw_report.wr_pos] = '\0';

	spin_unlock_irqrestore(&fw_report_lock, intr_flags);
}

static ssize_t __npu_fw_report_fops_read(struct npu_store_log_read_obj *robj, char *outbuf, const size_t outlen)
{
	size_t	copy_len, buf_end;

	/* Copy data to temporary buffer*/
	if (fw_report.st_buf) {
		buf_end = (robj->read_pos > fw_report.wr_pos) ? fw_report.line_cnt : fw_report.wr_pos;
		copy_len = min(outlen, buf_end - robj->read_pos);
		memcpy(outbuf, fw_report.st_buf + robj->read_pos, copy_len);
	} else
		copy_len = 0;

	if (copy_len > 0) {
		robj->read_pos = (robj->read_pos + copy_len) % fw_report.st_size;
		if (fw_report.line_cnt == robj->read_pos) {
			fw_report.line_cnt = 0;
			robj->read_pos = 0;
			memset(&fw_report.st_buf[fw_report.wr_pos], '\0', (fw_report.st_size - fw_report.wr_pos));
		}
	}

	return copy_len;
}

static ssize_t npu_fw_report_fops_read(struct file *file, char __user *outbuf, size_t outlen, loff_t *loff)
{
	struct npu_store_log_read_obj	*robj;
	ssize_t				ret, copy_len;
	size_t				tmp_buf_len;
	char				*tmp_buf = NULL;
	unsigned long			intr_flags;

	BUG_ON(!file);
	robj = file->private_data;
	BUG_ON(!robj);
	BUG_ON(robj->magic != READ_OBJ_MAGIC);

	/* Temporal kernel buffer, to read inside of spinlock */
	tmp_buf_len = min(outlen, PAGE_SIZE);
	tmp_buf = kzalloc(tmp_buf_len, GFP_KERNEL);
	if (!tmp_buf) {
		ret = -ENOMEM;
		goto err_exit;
	}

	/* Check data available */
	ret = 0;
	while (ret == 0) {	/* ret = 0 on timeout */
		/* TODO: Accessing npu_log.wr_pos outside of spinlock is potentially dangerous */
		ret = wait_event_interruptible_timeout(fw_report.wq, robj->read_pos != fw_report.wr_pos, 1 * HZ);
		if (ret == -ERESTARTSYS) {
			ret = 0;
			goto err_exit;
		}
	}

	spin_lock_irqsave(&fw_report_lock, intr_flags);
	copy_len = __npu_fw_report_fops_read(robj, tmp_buf, tmp_buf_len);
	spin_unlock_irqrestore(&fw_report_lock, intr_flags);

	if (copy_len > 0) {
		ret = copy_to_user(outbuf, tmp_buf, copy_len);
		if (ret) {
			pr_err("copy_to_user failed : %zd\n", ret);
			ret = -EFAULT;
			goto err_exit;
		}
	}

	ret = copy_len;
err_exit:
	if (tmp_buf)
		kfree(tmp_buf);
	return ret;
}

const struct file_operations npu_store_log_fops = {
	.owner = THIS_MODULE,
	.open = npu_store_log_fops_open,
	.release = npu_store_log_fops_close,
	.read = npu_store_log_fops_read,
};

const struct file_operations npu_fw_report_fops = {
	.owner = THIS_MODULE,
	.open = npu_fw_report_fops_open,
	.release = npu_fw_report_fops_close,
	.read = npu_fw_report_fops_read,
};

static int npu_store_log_dump(const size_t dump_size)
{
	char				*dump_buf = NULL;
	struct npu_store_log_read_obj	dump_robj;
	size_t				rd, total, pos;
	char				*st;
	int				ret;

	/* waitq initialization is not necessary */
	memset(&dump_robj, 0, sizeof(dump_robj));
	dump_robj.magic = READ_OBJ_MAGIC;

	npu_store_log_set_offset(&dump_robj, dump_size);

	dump_buf = kzalloc(dump_size, GFP_ATOMIC);
	if (!dump_buf)
		return -ENOMEM;

	/* Read log */
	total = 0;
	ret = spin_lock_safe_isr(&npu_log_lock);
	if (ret) {
		pr_err("NPU log dump is not available - in interrupt context\n", total);
		goto err_exit;
	}

	if (npu_log.line_cnt == npu_log.last_dump_line_cnt + 1) {
		/* No need to print out because already dumped */
		total = 0;
	} else {
		while (total < dump_size) {
			rd = __npu_store_log_fops_read(&dump_robj, (dump_buf + total), (dump_size - total));
			if (rd == 0)	/* No data anymore */
				break;
			total += rd;
		}
	}
	npu_log.last_dump_line_cnt = npu_log.line_cnt;
	spin_unlock(&npu_log_lock);

	if (total > 0) {
		/* Print stack back trace - Printed if there is a log to print to avoid duplication */
		//dump_stack();// removed for kernel pointer leakage issue

		pos = 0;
		st = dump_buf;
		pr_err("---------- Start NPU log dump (len = %zu) ---------------\n", total);
		while (pos < total) {
			if (dump_buf[pos] == '\n') {
				/* Print on newline character */
				dump_buf[pos] = '\0';
				pr_cont("%s\n", st);
				st = dump_buf + pos + 1;
			}
			pos++;
		}
		if (st != dump_buf + pos) {
			/* Printout remaining - Not common case but for fail safe */
			dump_buf[pos - 1] = '\0';
			pr_cont("%s <<No Line Ending>>\n", st);
		}
		pr_cont("---------- End of NPU log dump ---------------\n");
	}

err_exit:

	kfree(dump_buf);
	return ret;
}

/*
 * Called when an error message is printed
 */
void npu_log_on_error(void)
{
	int ret;

	/* Print last log */
	ret = npu_store_log_dump(NPU_STORE_LOG_DUMP_SIZE_ON_ERROR);
	if (ret)
		pr_err("%s(): npu_store_log_dump() failed: ret = %d\n", __func__, ret);
}

s32 atoi(const char *psz_buf)
{
	const char *pch = psz_buf;
	s32 base = 0;

	while (isspace(*pch))
		pch++;

	if (*pch == '-' || *pch == '+') {
		base = 10;
		pch++;
	} else if (*pch && tolower(pch[strlen(pch) - 1]) == 'h') {
		base = 16;
	}

	return simple_strtoul(pch, NULL, base);
}

int bitmap_scnprintf(char *buf, unsigned int buflen,
	const unsigned long *maskp, int nmaskbits)
{
	int i, word, bit, len = 0;
	unsigned long val;
	const char *sep = "";
	int chunksz;
	u32 chunkmask;

	chunksz = nmaskbits & (CHUNKSZ - 1);
	if (chunksz == 0)
		chunksz = CHUNKSZ;

	i = ALIGN(nmaskbits, CHUNKSZ) - CHUNKSZ;
	for (; i >= 0; i -= CHUNKSZ) {
		chunkmask = ((1ULL << chunksz) - 1);
		word = i / BITS_PER_LONG;
		bit = i % BITS_PER_LONG;
		val = (maskp[word] >> bit) & chunkmask;
		len += scnprintf(buf+len, buflen-len, "%s%0*lx", sep,
			(chunksz+3)/4, val);
		chunksz = CHUNKSZ;
		sep = ",";
	}
	return len;
}

int npu_debug_memdump8(u8 *start, u8 *end)
{
	int ret = 0;
	u8 *cur;
	u32 items;
	size_t offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump8(%pK ~ %pK)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
#ifdef DEBUG_LOG_MEMORY
			printk(KERN_DEBUG "%s\n", sentence);
#else
			printk(KERN_INFO "%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %pK:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "%02X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		printk(KERN_DEBUG "%s\n", sentence);
#else
		printk(KERN_INFO "%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}


int npu_debug_memdump16(u16 *start, u16 *end)
{
	int ret = 0;
	u16 *cur;
	u32 items;
	size_t offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump16(%pK ~ %pK)", start, end);

	while (cur < end) {
		if ((items % 16) == 0) {
#ifdef DEBUG_LOG_MEMORY
			printk(KERN_DEBUG "%s\n", sentence);
#else
			printk(KERN_INFO "%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %pK:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%04X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		printk(KERN_DEBUG "%s\n", sentence);
#else
		printk(KERN_INFO "%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}

int npu_debug_memdump32(u32 *start, u32 *end)
{
	int ret = 0;
	u32 *cur;
	u32 items;
	size_t offset;
	char term[50], sentence[250];

	cur = start;
	items = 0;
	offset = 0;

	memset(sentence, 0, sizeof(sentence));
	snprintf(sentence, sizeof(sentence), "[V] Memory Dump32(%pK ~ %pK)", start, end);

	while (cur < end) {
		if ((items % 8) == 0) {
#ifdef DEBUG_LOG_MEMORY
			printk(KERN_DEBUG "%s\n", sentence);
#else
			printk(KERN_INFO "%s\n", sentence);
#endif
			offset = 0;
			snprintf(term, sizeof(term), "[V] %pK:      ", cur);
			snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
			offset += strlen(term);
			items = 0;
		}

		snprintf(term, sizeof(term), "0x%08X ", *cur);
		snprintf(&sentence[offset], sizeof(sentence) - offset, "%s", term);
		offset += strlen(term);
		cur++;
		items++;
	}

	if (items) {
#ifdef DEBUG_LOG_MEMORY
		printk(KERN_DEBUG "%s\n", sentence);
#else
		printk(KERN_INFO "%s\n", sentence);
#endif
	}

	ret = cur - end;

	return ret;
}

/*
       This function could be used when direct access to SRAM is not approved.
*/
int npu_debug_memdump32_by_memcpy(u32 *start, u32 *end)
{
	int j, k, l;
	int ret = 0;
	u32 items;
	u32 *cur;
	char term[4], strHexa[128], strString[128], sentence[256];

	cur = start;
	items = 0;
	j = k = l = 0;

	memset(sentence, 0, sizeof(sentence));
	memset(strString, 0, sizeof(strString));
	memset(strHexa, 0, sizeof(strHexa));
	j = sprintf(sentence, "[V] Memory Dump32(%pK ~ %pK)", start, end);
	while (cur < end) {
		if ((items % 4) == 0) {
			j += sprintf(sentence + j, "%s   %s", strHexa, strString);
#ifdef DEBUG_LOG_MEMORY
			pr_debug("%s\n", sentence);
#else
			pr_info("%s\n", sentence);
#endif
			j = 0; items = 0; k = 0; l = 0;
			j = sprintf(sentence, "[V] %pK:      ", cur);
			items = 0;
		}
		memcpy_fromio(term, cur, sizeof(term));
		k += sprintf(strHexa + k, "%02X%02X%02X%02X ",
			term[0], term[1], term[2], term[3]);
		l += sprintf(strString + l, "%c%c%c%c", ISPRINTABLE(term[0]),
			ISPRINTABLE(term[1]), ISPRINTABLE(term[2]), ISPRINTABLE(term[3]));
		cur++;
		items++;
	}
	if (items) {
		j += sprintf(sentence + j, "%s   %s", strHexa, strString);
#ifdef DEBUG_LOG_MEMORY
		pr_debug("%s\n", sentence);
#else
		pr_info("%s\n", sentence);
#endif
	}
	ret = cur - end;
	return ret;
}


static ssize_t npu_chg_log_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/*
	 * sysfs read buffer size is PAGE_SIZE
	 * Ref: http://www.kernel.org/pub/linux/kernel/people/mochel/doc/papers/ols-2005/mochel.pdf
	 */
	ssize_t remain = PAGE_SIZE - 1;
	ssize_t len = 0;
	ssize_t ret = 0;
	int	lv;

	npu_info("start.");

	npu_dbg("current log level : pr = %d, st = %d", npu_log.pr_level, npu_log.st_level);
	ret = scnprintf(buf, remain,
		" Usage  : echo <printk>.<Memory> > log_level\n"
		" Example: # echo 3.4 > log_level\n\n"
		"   <printk>      <Memory>\n");
	if (ret > 0)
		len += ret, remain -= ret, buf += ret;

	for (lv = 0; lv < NPU_LOG_INVALID; ++lv) {
		if (npu_log.pr_level == lv)
			ret = scnprintf(buf, remain,
				"[%d] %-10s", lv, LOG_LEVEL_NAME[lv]);
		else
			ret = scnprintf(buf, remain,
					" %d  %-10s", lv, LOG_LEVEL_NAME[lv]);
		if (ret > 0)
			len += ret, remain -= ret, buf += ret;

		if (npu_log.st_level == lv)
			ret = scnprintf(buf, remain,
					"[%d] %-10s\n", lv, LOG_LEVEL_NAME[lv]);
		else
			ret = scnprintf(buf, remain,
					" %d  %-10s\n", lv, LOG_LEVEL_NAME[lv]);
		if (ret > 0)
			len += ret, remain -= ret, buf += ret;
	}

	return len;
}

static ssize_t npu_chg_log_level_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int		pr, st;
	int		ret;

	/* Parsing */
	pr = st = -1;
	ret = sscanf(buf, "%1d.%1d", &pr, &st);
	if (ret != 2) {
		npu_err("Command line parsing error.\n");
		return -EINVAL;
	}
	if (pr < NPU_LOG_TRACE || NPU_LOG_INVALID < pr) {
		npu_err("Invalid pr_level [%d] specified.\n", pr);
		return -EINVAL;
	}
	if (st < NPU_LOG_TRACE || NPU_LOG_INVALID < st) {
		npu_err("Invalid st_level [%d] specified.\n", st);
		return -EINVAL;
	}

	/* Change log level */
	npu_info("log level for pr_level [%d -> %d] / st_level [%d -> %d]\n",
		npu_log.pr_level, pr, npu_log.st_level, st);
	npu_log.pr_level = pr;
	npu_log.st_level = st;
	barrier();

	return count;
}

static DEVICE_ATTR(log_level, 0644, npu_chg_log_level_show, npu_chg_log_level_store);

int fw_will_note(size_t len)
{
	unsigned long intr_flags;
	char *buf;
	size_t i, pos;
	bool bReqLegacy = FALSE;

	//Gather one more time before make will note.
	fw_rprt_manager();
	spin_lock_irqsave(&fw_report_lock, intr_flags);

	//Consideration for early phase
	if (fw_report.wr_pos < len) {
		len = fw_report.wr_pos;
		bReqLegacy = TRUE;
	}
	buf = kzalloc((len+1), GFP_ATOMIC);
	if (!buf) {
		npu_err("%s() fail to alloc mem for fw_report\n", __func__);
		spin_unlock_irqrestore(&fw_report_lock, intr_flags);
		return -ENOMEM;
	}
	pos = 0;
	pr_err("----------- Start will_note for npu_fw (/sys/kernel/debug/npu/fw-report )-------------\n");
	if ((fw_report.last_dump_line_cnt != 0) && (bReqLegacy == TRUE)) {
		for (i = fw_report.st_size - (len - fw_report.wr_pos); i < fw_report.st_size; i++) {
			buf[pos] = fw_report.st_buf[i];
			pos++;
			if (fw_report.st_buf[i] == '\n') {
				buf[pos] = '\0';
				pr_cont("%s", buf);
				pos = 0;
				buf[0] = '\0';
			}
		}
		//Change length to current position.
		len = fw_report.wr_pos;
	}
	for (i = fw_report.wr_pos - len ; i < fw_report.wr_pos; i++) {
		buf[pos] = fw_report.st_buf[i];
		pos++;
		if (fw_report.st_buf[i] == '\n') {
			buf[pos] = '\0';
			pr_cont("%s", buf);
			pos = 0;
			buf[0] = '\0';
		}
	}

	pr_cont("----------- End of will_note for npu_fw -------------\n");
	spin_unlock_irqrestore(&fw_report_lock, intr_flags);
	kfree(buf);
	pr_err("----------- Check unposted_mbox ---------------------\n");
	npu_check_unposted_mbox(ECTRL_LOW);
	npu_check_unposted_mbox(ECTRL_HIGH);
	npu_check_unposted_mbox(ECTRL_ACK);
	npu_check_unposted_mbox(ECTRL_REPORT);
	pr_err("----------- Done unposted_mbox ----------------------\n");
	return 0;

}

/* Exported functions */
int npu_log_probe(struct npu_device *npu_device)
{
	int ret;

	probe_info("start in npu_log_probe\n");

	/* Basic initialization of log store */
	npu_log.st_buf = NULL;
	npu_log.st_size = 0;
	npu_log.wr_pos = 0;
	init_waitqueue_head(&npu_log.wq);
	init_waitqueue_head(&fw_report.wq);

	/* Log level change function on sysfs */
	ret = device_create_file(npu_device->dev, &dev_attr_log_level);
	if (ret) {
		probe_err("device_create_file() failed: ret = %d\n", ret);
		return ret;
	}

	/* Register memory store logger */
	ret = npu_debug_register("dev-log", &npu_store_log_fops);
	if (ret) {
		npu_err("npu_debug_register error : ret = %d\n", ret);
		return ret;
	}

	/* Register FW log keeper */
	ret = npu_debug_register("fw-report", &npu_fw_report_fops);
	if (ret) {
		npu_err("npu_debug_register error : ret = %d\n", ret);
		return ret;
	}

	probe_info("complete in npu_log_probe\n");
	return 0;
}

int npu_log_release(struct npu_device *npu_device)
{
	probe_info("start in npu_log_release\n");
	device_remove_file(npu_device->dev, &dev_attr_log_level);
	probe_info("complete in npu_log_release\n");
	return 0;
}

int npu_log_open(struct npu_device *npu_device)
{
	npu_info("start in npu_log_open\n");
	npu_info("complete in npu_log_open\n");
	return 0;
}

int npu_log_close(struct npu_device *npu_device)
{
	npu_info("start in npu_log_close\n");
	npu_info("complete in npu_log_close\n");
	return 0;
}

int npu_log_start(struct npu_device *npu_device)
{
	npu_info("start in npu_log_start\n");
	npu_info("complete in npu_log_start\n");
	return 0;
}

int npu_log_stop(struct npu_device *npu_device)
{
	npu_info("start in npu_log_stop\n");
	npu_info("complete in npu_log_stop\n");
	return 0;
}

/* Unit test */
#ifdef CONFIG_VISION_UNITTEST
#define IDIOT_TESTCASE_IMPL "npu-log.idiot"
#include "idiot-def.h"
#endif
