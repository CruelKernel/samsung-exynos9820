/*
 *  sec_audio_debug.c
 *
 *  Copyright (c) 2018 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include <linux/sched/clock.h>

#include <sound/soc.h>
#include <sound/samsung/sec_audio_debug.h>

#include <linux/sec_debug.h>
#include <sound/samsung/abox.h>
#include "abox/abox.h"

#define DBG_STR_BUFF_SZ 256
#define LOG_MSG_BUFF_SZ 512

#define SEC_AUDIO_DEBUG_STRING_WQ_NAME "sec_audio_dbg_str_wq"

struct sec_audio_debug_data {
	char *dbg_str_buf;
	unsigned long long mode_time;
	unsigned long mode_nanosec_t;
	struct mutex dbg_lock;
	struct workqueue_struct *debug_string_wq;
	struct work_struct debug_string_work;
	enum abox_debug_err_type debug_err_type;
	unsigned int *abox_dbg_addr;
};

static struct sec_audio_debug_data *p_debug_data;

static struct dentry *audio_debugfs;
static struct sec_audio_log_data *p_debug_log_data;
static struct sec_audio_log_data *p_debug_bootlog_data;
static struct sec_audio_log_data *p_debug_pmlog_data;
static unsigned int debug_buff_switch;

int is_abox_rdma_enabled(int id)
{
	struct abox_data *data = abox_get_abox_data();

	return (readl(data->sfr_base + ABOX_RDMA_BASE + (ABOX_RDMA_INTERVAL * id)
		+ ABOX_RDMA_CTRL0) & ABOX_RDMA_ENABLE_MASK);
}

int is_abox_wdma_enabled(int id)
{
	struct abox_data *data = abox_get_abox_data();

	return (readl(data->sfr_base + ABOX_WDMA_BASE + (ABOX_WDMA_INTERVAL * id)
		+ ABOX_WDMA_CTRL) & ABOX_WDMA_ENABLE_MASK);
}

static void abox_debug_string_update_workfunc(struct work_struct *wk)
{
	struct abox_data *data = abox_get_abox_data();
	int i;
	unsigned int len = 0;
/*
	struct sec_audio_debug_data *dbg_data = container_of(wk,
					   struct sec_audio_debug_data, debug_string_work);
*/
	if (!p_debug_data)
		return;

	mutex_lock(&p_debug_data->dbg_lock);

	p_debug_data->mode_time = data->audio_mode_time;
	p_debug_data->mode_nanosec_t = do_div(p_debug_data->mode_time, NSEC_PER_SEC);

	p_debug_data->dbg_str_buf = kzalloc(sizeof(char) * DBG_STR_BUFF_SZ, GFP_KERNEL);
	if (!p_debug_data->dbg_str_buf) {
		pr_err("%s: no memory\n", __func__);
		mutex_unlock(&p_debug_data->dbg_lock);
		return;
	}

	switch (p_debug_data->debug_err_type) {
	case TYPE_ABOX_DATAABORT:
	case TYPE_ABOX_PREFETCHABORT:
		len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len,
					"ABOXERR%1d ", p_debug_data->debug_err_type);

		if (!abox_is_on()) {
			len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len, "Abox disabled");
			goto buff_done;
		}

		if (p_debug_data->abox_dbg_addr == NULL) {
			len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len, "GPR NULL");
			goto buff_done;
		}

		for (i = 0; i <= 14; i++) {
			len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
						"%08x ", *p_debug_data->abox_dbg_addr++);
			if (len >= DBG_STR_BUFF_SZ - 1)
				goto buff_done;
		}
		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
					"PC:%08x ", *p_debug_data->abox_dbg_addr++);
		if (len >= DBG_STR_BUFF_SZ - 1)
			goto buff_done;

		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
					"m%d:%05lu", data->audio_mode, (unsigned long) p_debug_data->mode_time);

		break;
	case TYPE_ABOX_OSERROR:
	case TYPE_ABOX_UNDEFEXCEPTION:
	case TYPE_ABOX_UNKNOWNERROR:
		len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len,
					"ABOXERR%1d ", p_debug_data->debug_err_type);

		if (!abox_is_on()) {
			len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len, "Abox disabled");
			goto buff_done;
		}

		for (i = 0; i <= 14; i++) {
			len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
							"%08x ", readl(data->sfr_base + ABOX_CPU_R(i)));
			if (len >= DBG_STR_BUFF_SZ - 1)
				goto buff_done;
		}
		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
							"PC:%08x ", readl(data->sfr_base + ABOX_CPU_PC));
		if (len >= DBG_STR_BUFF_SZ - 1)
			goto buff_done;

		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
							"L2C:%08x ", readl(data->sfr_base + ABOX_CPU_L2C_STATUS));
		if (len >= DBG_STR_BUFF_SZ - 1)
			goto buff_done;

		len += snprintf(p_debug_data->dbg_str_buf + len, DBG_STR_BUFF_SZ - len,
							"m%d:%05lu", data->audio_mode, (unsigned long) p_debug_data->mode_time);

		break;
	case TYPE_ABOX_VSSERROR:
		len += snprintf(p_debug_data->dbg_str_buf, DBG_STR_BUFF_SZ - len, "VSSERROR");
		break;
	default:
		pr_err("%s: unknown type %d\n", __func__, p_debug_data->debug_err_type);
		break;
	}

buff_done:
	pr_info("%s: %s\n", __func__, p_debug_data->dbg_str_buf);
	sec_debug_set_extra_info_aud(p_debug_data->dbg_str_buf);

	kfree(p_debug_data->dbg_str_buf);
	p_debug_data->dbg_str_buf = NULL;
	mutex_unlock(&p_debug_data->dbg_lock);
}

void abox_debug_string_update(enum abox_debug_err_type type, void *addr)
{
	p_debug_data->debug_err_type = type;
	p_debug_data->abox_dbg_addr = (unsigned int*) addr;

	queue_work(p_debug_data->debug_string_wq, &p_debug_data->debug_string_work);
}
EXPORT_SYMBOL_GPL(abox_debug_string_update);

void adev_err(struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];

	va_start(args, fmt);
	vsnprintf(temp_buf, sizeof(temp_buf), fmt, args);
	va_end(args);

	dev_printk(KERN_ERR, dev, "%s", temp_buf);
	sec_audio_log(3, dev, "%s", temp_buf);
}

void adev_warn(struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];

	va_start(args, fmt);
	vsnprintf(temp_buf, sizeof(temp_buf), fmt, args);
	va_end(args);

	dev_printk(KERN_WARNING, dev, "%s", temp_buf);
	sec_audio_log(4, dev, "%s", temp_buf);
}

void adev_info(struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];

	va_start(args, fmt);
	vsnprintf(temp_buf, sizeof(temp_buf), fmt, args);
	va_end(args);

	dev_printk(KERN_INFO, dev, "%s", temp_buf);
	sec_audio_log(6, dev, "%s", temp_buf);
}

void adev_dbg(struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];

	va_start(args, fmt);
	vsnprintf(temp_buf, sizeof(temp_buf), fmt, args);
	va_end(args);

	dev_printk(KERN_DEBUG, dev, "%s", temp_buf);
	sec_audio_log(7, dev, "%s", temp_buf);
}

static int get_debug_buffer_switch(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = debug_buff_switch;

	return 0;
}

static int set_debug_buffer_switch(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	unsigned int val;
	int ret = 0;

	val = (unsigned int)ucontrol->value.integer.value[0];

	if (val) {
		alloc_sec_audio_log(p_debug_log_data, SZ_1M);
		debug_buff_switch = SZ_1M;
	} else {
		alloc_sec_audio_log(p_debug_log_data, 0);
		debug_buff_switch = 0;
	}

	return ret;
}

static const struct snd_kcontrol_new debug_controls[] = {
	SOC_SINGLE_BOOL_EXT("Debug Buffer Switch", 0,
			get_debug_buffer_switch,
			set_debug_buffer_switch),
};

int register_debug_mixer(struct snd_soc_card *card)
{
	return snd_soc_add_card_controls(card, debug_controls,
				ARRAY_SIZE(debug_controls));
}
EXPORT_SYMBOL_GPL(register_debug_mixer);

static int audio_log_open_file(struct inode *inode, struct  file *file)
{
	struct sec_audio_log_data *p_dbg_log_data;

	if (inode->i_private)
		file->private_data = inode->i_private;

	p_dbg_log_data = file->private_data;

	pr_info("%s: %s\n", __func__, p_dbg_log_data->name);
	p_dbg_log_data->read_idx = 0;

	return 0;
}

static ssize_t audio_log_read_file(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	size_t num_msg;
	ssize_t ret;
	loff_t pos = *ppos;
	size_t copy_len;
	struct sec_audio_log_data *p_dbg_log_data = file->private_data;

	if (*ppos < 0 || !count)
		return -EINVAL;

	if (p_dbg_log_data->full)
		num_msg = p_dbg_log_data->sz_log_buff - p_dbg_log_data->read_idx;
	else
		num_msg = (size_t) p_dbg_log_data->buff_idx - p_dbg_log_data->read_idx;

	if (num_msg < 0) {
		pr_err("%s: buff idx invalid for %s\n", __func__, p_dbg_log_data->name);
		return -EINVAL;
	}

	if (pos > num_msg) {
		pr_err("%s: invalid offset for %s\n", __func__, p_dbg_log_data->name);
		return -EINVAL;
	}

	copy_len = min(count, num_msg);

	ret = copy_to_user(user_buf, p_dbg_log_data->audio_log_buffer + p_dbg_log_data->read_idx, copy_len);
	if (ret) {
		pr_err("%s: %s copy fail %d\n", __func__, p_dbg_log_data->name, (int) ret);
		return -EFAULT;
	}
	p_dbg_log_data->read_idx += copy_len;

	return copy_len;
}

static const struct file_operations audio_log_fops = {
	.open = audio_log_open_file,
	.read = audio_log_read_file,
	.llseek = default_llseek,
};

int alloc_sec_audio_log(struct sec_audio_log_data *p_dbg_log_data, size_t buffer_len)
{
	if (p_dbg_log_data->sz_log_buff) {
		p_dbg_log_data->sz_log_buff = 0;
		free_sec_audio_log(p_dbg_log_data);
	}

	p_dbg_log_data->buff_idx = 0;
	p_dbg_log_data->full = 0;

	if (buffer_len <= 0) {
		pr_err("%s: Invalid buffer_len for %s %d\n", __func__, p_dbg_log_data->name, (int) buffer_len);
		p_dbg_log_data->sz_log_buff = 0;
		return 0;
	}

	if (p_dbg_log_data->virtual) {
		p_dbg_log_data->audio_log_buffer = vzalloc(buffer_len);
	} else {
		p_dbg_log_data->audio_log_buffer = kzalloc(buffer_len, GFP_KERNEL);
	}
	if (p_dbg_log_data->audio_log_buffer == NULL) {
		pr_err("%s: Failed to alloc audio_log_buffer for %s\n", __func__, p_dbg_log_data->name);
		p_dbg_log_data->sz_log_buff = 0;
		return -ENOMEM;
	}

	p_dbg_log_data->sz_log_buff = buffer_len;

	return p_dbg_log_data->sz_log_buff;
}
EXPORT_SYMBOL_GPL(alloc_sec_audio_log);

void free_sec_audio_log(struct sec_audio_log_data *p_dbg_log_data)
{
	p_dbg_log_data->sz_log_buff = 0;
	if (p_dbg_log_data->virtual)
		vfree(p_dbg_log_data->audio_log_buffer);
	else
		kfree(p_dbg_log_data->audio_log_buffer);
	p_dbg_log_data->audio_log_buffer = NULL;
}

static ssize_t log_enable_read_file(struct file *file, char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[16];
	int len;
	struct sec_audio_log_data *p_dbg_log_data = file->private_data;

	len = snprintf(buf, 16, "%d\n", (int) p_dbg_log_data->sz_log_buff);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t log_enable_write_file(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	char buf[16];
	size_t size;
	u64 value;
	struct sec_audio_log_data *p_dbg_log_data = file->private_data;

	size = min(count, (sizeof(buf) - 1));
	if (copy_from_user(buf, user_buf, size)) {
		pr_err("%s: copy_from_user err\n", __func__);
		return -EFAULT;
	}
	buf[size] = 0;

	if (kstrtou64(buf, 10, &value)) {
		pr_err("%s: Invalid value\n", __func__);
		return -EINVAL;
	}

	/* do not alloc over 2MB */
	if (value > SZ_2M)
		value = SZ_2M;

	alloc_sec_audio_log(p_dbg_log_data, (size_t) value);

	return size;
}

static const struct file_operations log_enable_fops = {
	.open = simple_open,
	.read = log_enable_read_file,
	.write = log_enable_write_file,
	.llseek = default_llseek,
};

ssize_t make_prefix_msg(char *buff, int level, struct device *dev)
{
	unsigned long long time = local_clock();
	unsigned long nanosec_t = do_div(time, 1000000000);
	ssize_t msg_size = 0;

	msg_size = scnprintf(buff, LOG_MSG_BUFF_SZ, "<%d> [%5lu.%06lu] %s %s: ",
						level, (unsigned long) time, nanosec_t / 1000,
						(dev)? dev_driver_string(dev): "NULL", (dev)? dev_name(dev): "NULL");
	return msg_size;
}

void copy_msgs(char *buff, struct sec_audio_log_data *p_dbg_log_data)
{
	if (p_dbg_log_data->buff_idx + strlen(buff) > p_dbg_log_data->sz_log_buff - 1) {
		p_dbg_log_data->full = 1;
		p_dbg_log_data->buff_idx = 0;
	}
	p_dbg_log_data->buff_idx +=
		scnprintf(p_dbg_log_data->audio_log_buffer + p_dbg_log_data->buff_idx,
				(strlen(buff) + 1), "%s", buff);
}

void sec_audio_log(int level, struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];
	ssize_t temp_buff_idx = 0;
	struct sec_audio_log_data *p_dbg_log_data = p_debug_log_data;

	if (!p_dbg_log_data->sz_log_buff) {
		return;
	}

	temp_buff_idx = make_prefix_msg(temp_buf, level, dev);

	va_start(args, fmt);
	temp_buff_idx +=
		vsnprintf(temp_buf + temp_buff_idx,
				LOG_MSG_BUFF_SZ - temp_buff_idx, fmt, args);
	va_end(args);

	copy_msgs(temp_buf, p_dbg_log_data);
}
EXPORT_SYMBOL_GPL(sec_audio_log);

void sec_audio_bootlog(int level, struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];
	ssize_t temp_buff_idx = 0;
	struct sec_audio_log_data *p_dbg_log_data = p_debug_bootlog_data;

	if (!p_dbg_log_data->sz_log_buff) {
		return;
	}

	temp_buff_idx = make_prefix_msg(temp_buf, level, dev);

	va_start(args, fmt);
	temp_buff_idx +=
		vsnprintf(temp_buf + temp_buff_idx,
				LOG_MSG_BUFF_SZ - (temp_buff_idx + 1), fmt, args);
	va_end(args);

	copy_msgs(temp_buf, p_dbg_log_data);
}
EXPORT_SYMBOL_GPL(sec_audio_bootlog);

void sec_audio_pmlog(int level, struct device *dev, const char *fmt, ...)
{
	va_list args;
	char temp_buf[LOG_MSG_BUFF_SZ];
	ssize_t temp_buff_idx = 0;
	struct sec_audio_log_data *p_dbg_log_data = p_debug_pmlog_data;

	if (!p_dbg_log_data->sz_log_buff) {
		return;
	}

	temp_buff_idx = make_prefix_msg(temp_buf, level, dev);

	va_start(args, fmt);
	temp_buff_idx +=
		vsnprintf(temp_buf + temp_buff_idx,
				LOG_MSG_BUFF_SZ - (temp_buff_idx + 1), fmt, args);
	va_end(args);

	copy_msgs(temp_buf, p_dbg_log_data);
}
EXPORT_SYMBOL_GPL(sec_audio_pmlog);

static const struct file_operations abox_qos_fops = {
	.open = simple_open,
	.read = abox_qos_read_file,
	.llseek = default_llseek,
};

static int __init sec_audio_debug_init(void)
{
	struct sec_audio_debug_data *data;
	struct sec_audio_log_data *log_data;
	struct sec_audio_log_data *bootlog_data;
	struct sec_audio_log_data *pmlog_data;
	int ret = 0;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("%s: failed to alloc data\n", __func__);
		return -ENOMEM;
	}

	p_debug_data = data;
	mutex_init(&p_debug_data->dbg_lock);

	audio_debugfs = debugfs_create_dir("audio", NULL);
	if (!audio_debugfs) {
		pr_err("Failed to create audio debugfs\n");
		ret = -EPERM;
		goto err_data;
	}

	log_data = kzalloc(sizeof(*log_data), GFP_KERNEL);
	if (!log_data) {
		pr_err("%s: failed to alloc log_data\n", __func__);
		ret = -ENOMEM;
		goto err_debugfs;
	}
	p_debug_log_data = log_data;
/*
	p_debug_log_data->audio_log_buffer = NULL;
	p_debug_log_data->buff_idx = 0;
	p_debug_log_data->full = 0;
	p_debug_log_data->read_idx = 0;
	p_debug_log_data->sz_log_buff = 0;
*/
	p_debug_log_data->virtual = 1;
	p_debug_log_data->name = kasprintf(GFP_KERNEL, "runtime");

	bootlog_data = kzalloc(sizeof(*bootlog_data), GFP_KERNEL);
	if (!bootlog_data) {
		pr_err("%s: failed to alloc bootlog_data\n", __func__);
		ret = -ENOMEM;
		goto err_log_data;
	}
	p_debug_bootlog_data = bootlog_data;
	p_debug_bootlog_data->name = kasprintf(GFP_KERNEL, "boot");

	pmlog_data = kzalloc(sizeof(*pmlog_data), GFP_KERNEL);
	if (!pmlog_data) {
		pr_err("%s: failed to alloc pmlog_data\n", __func__);
		ret = -ENOMEM;
		goto err_bootlog_data;
	}

	p_debug_pmlog_data = pmlog_data;
	p_debug_pmlog_data->name = kasprintf(GFP_KERNEL, "pm");

	alloc_sec_audio_log(p_debug_bootlog_data, SZ_1K);
	alloc_sec_audio_log(p_debug_pmlog_data, SZ_4K);

	p_debug_data->debug_string_wq = create_singlethread_workqueue(
						SEC_AUDIO_DEBUG_STRING_WQ_NAME);
	if (p_debug_data->debug_string_wq == NULL) {
		pr_err("%s: failed to creat debug_string_wq\n", __func__);
		ret = -ENOENT;
		goto err_pmlog_data;
	}

	INIT_WORK(&p_debug_data->debug_string_work, abox_debug_string_update_workfunc);

	debugfs_create_file("log_enable",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, p_debug_log_data, &log_enable_fops);

	debugfs_create_file("log",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, p_debug_log_data, &audio_log_fops);

	debugfs_create_file("bootlog_enable",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, p_debug_bootlog_data, &log_enable_fops);

	debugfs_create_file("bootlog",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, p_debug_bootlog_data, &audio_log_fops);

	debugfs_create_file("pmlog_enable",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, p_debug_pmlog_data, &log_enable_fops);

	debugfs_create_file("pmlog",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, p_debug_pmlog_data, &audio_log_fops);

	debugfs_create_file("abox_qos",
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
			audio_debugfs, NULL, &abox_qos_fops);

	return 0;

err_pmlog_data:
	kfree_const(p_debug_pmlog_data->name);
	kfree(p_debug_pmlog_data);
	p_debug_pmlog_data = NULL;

err_bootlog_data:
	kfree_const(p_debug_bootlog_data->name);
	kfree(p_debug_bootlog_data);
	p_debug_bootlog_data = NULL;

err_log_data:
	kfree_const(p_debug_log_data->name);
	kfree(p_debug_log_data);
	p_debug_log_data = NULL;

err_debugfs:
	debugfs_remove_recursive(audio_debugfs);

err_data:
	kfree(p_debug_data);
	p_debug_data = NULL;

	return ret;
}
early_initcall(sec_audio_debug_init);

static void __exit sec_audio_debug_exit(void)
{
	if (p_debug_pmlog_data->sz_log_buff)
		free_sec_audio_log(p_debug_pmlog_data);
	kfree_const(p_debug_pmlog_data->name);
	if (p_debug_pmlog_data)
		kfree(p_debug_pmlog_data);
	p_debug_pmlog_data = NULL;

	if (p_debug_bootlog_data->sz_log_buff)
		free_sec_audio_log(p_debug_bootlog_data);
	kfree_const(p_debug_bootlog_data->name);
	if (p_debug_bootlog_data)
		kfree(p_debug_bootlog_data);
	p_debug_bootlog_data = NULL;

	if (p_debug_log_data->sz_log_buff)
		free_sec_audio_log(p_debug_log_data);
	kfree_const(p_debug_log_data->name);
	if (p_debug_log_data)
		kfree(p_debug_log_data);
	p_debug_log_data = NULL;

	destroy_workqueue(p_debug_data->debug_string_wq);
	p_debug_data->debug_string_wq = NULL;

	debugfs_remove_recursive(audio_debugfs);

	kfree(p_debug_data);
	p_debug_data = NULL;
}
module_exit(sec_audio_debug_exit);
MODULE_DESCRIPTION("Samsung Electronics Audio Debug driver");
MODULE_LICENSE("GPL");
