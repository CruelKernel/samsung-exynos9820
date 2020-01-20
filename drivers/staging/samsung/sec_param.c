/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung TN debugging code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/sec_ext.h>

#define PARAM_RD		0
#define PARAM_WR		1

#define SEC_PARAM_NAME	  "/dev/block/param"
#define STR_LENGTH	  1024

struct sec_param_data_s {
	struct work_struct sec_param_work;
	unsigned long offset;
	char val;
};

struct sec_param_data_s_str {
	struct work_struct sec_param_work_str;
	struct completion work;
	unsigned long offset;
	char str[STR_LENGTH];
	int direction;
};

static struct sec_param_data_s sec_param_data;
static struct sec_param_data_s_str sec_param_data_str;
static DEFINE_MUTEX(sec_param_mutex);
static DEFINE_MUTEX(sec_param_mutex_str);

static void sec_param_update(struct work_struct *work)
{
	int ret = -1;
	struct file *fp;
	struct sec_param_data_s *param_data =
		container_of(work, struct sec_param_data_s, sec_param_work);

	fp = filp_open(SEC_PARAM_NAME, O_WRONLY | O_SYNC, 0);
	if (IS_ERR(fp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(fp));
		return;
	}
	pr_info("%s: set param %c at %lu\n", __func__,
		param_data->val, param_data->offset);
	ret = fp->f_op->llseek(fp, param_data->offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}

	ret = vfs_write(fp, &param_data->val, 1, &fp->f_pos);
	if (ret < 0)
		pr_err("%s: write error! %d\n", __func__, ret);

close_fp_out:
	filp_close(fp, NULL);
	pr_info("%s: exit %d\n", __func__, ret);
}

static void sec_param_update_str(struct work_struct *work)
{
	int ret = -1;
	struct file *fp;
	mm_segment_t fs;
	struct sec_param_data_s_str *param_data_str =
		container_of(work, struct sec_param_data_s_str, sec_param_work_str);
	int flag = (param_data_str->direction == PARAM_WR)
			? (O_RDWR | O_SYNC) : O_RDONLY;

	fp = filp_open(SEC_PARAM_NAME, flag, 0);

	if (IS_ERR(fp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(fp));
		complete(&param_data_str->work);
		return;
	}

	fs = get_fs();
	set_fs(get_ds());

	ret = vfs_llseek(fp, param_data_str->offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}

	if (param_data_str->direction == PARAM_WR)
		ret = vfs_write(fp, (const char __user *)param_data_str->str, sizeof(param_data_str->str), &fp->f_pos);
	else if (param_data_str->direction == PARAM_RD)
		ret = vfs_read(fp, (char __user *)param_data_str->str, sizeof(param_data_str->str), &fp->f_pos);

	if (ret < 0) {
		pr_err("%s: %s error! %d\n", __func__, param_data_str->direction ? "write" : "read", ret);
		goto close_fp_out;
	}

close_fp_out:
	set_fs(fs);
	filp_close(fp, NULL);
	complete(&param_data_str->work);
	pr_info("%s: exit %d\n", __func__, ret);
}

int sec_set_param(unsigned long offset, char val)
{
	int ret = -1;

	mutex_lock(&sec_param_mutex);

	if ((offset < CM_OFFSET) || (offset > CM_OFFSET + CM_OFFSET_LIMIT))
		goto unlock_out;

	switch (val) {
	case PARAM_OFF:
	case PARAM_ON:
		goto set_param;
	default:
		goto unlock_out;
	}

set_param:
	sec_param_data.offset = offset;
	sec_param_data.val = val;

	schedule_work(&sec_param_data.sec_param_work);

	/* how to determine to return success or fail ? */
	ret = 0;
unlock_out:
	mutex_unlock(&sec_param_mutex);
	return ret;
}

int sec_set_param_str(unsigned long offset, const char *str, int size)
{
	int ret = 0;

	mutex_lock(&sec_param_mutex_str);

	if (!strcmp(str, "")) {
		mutex_unlock(&sec_param_mutex_str);
		return -1;
	}

	memset(sec_param_data_str.str, 0, sizeof(sec_param_data_str.str));
	sec_param_data_str.offset = offset;
	sec_param_data_str.direction = PARAM_WR;
	strncpy(sec_param_data_str.str, str, size);

	schedule_work(&sec_param_data_str.sec_param_work_str);
	wait_for_completion_timeout(&sec_param_data_str.work, msecs_to_jiffies(HZ));

	mutex_unlock(&sec_param_mutex_str);
	return ret;
}

int sec_get_param_str(unsigned long offset, char *str)
{
	mutex_lock(&sec_param_mutex_str);

	memset(sec_param_data_str.str, 0, sizeof(sec_param_data_str.str));
	sec_param_data_str.offset = offset;
	sec_param_data_str.direction = PARAM_RD;

	schedule_work(&sec_param_data_str.sec_param_work_str);
	wait_for_completion_timeout(&sec_param_data_str.work, msecs_to_jiffies(HZ));

	snprintf(str, sizeof(sec_param_data_str.str), "%s", sec_param_data_str.str);

	mutex_unlock(&sec_param_mutex_str);
	return 0;
}

static int __init sec_param_work_init(void)
{
	pr_info("%s: start\n", __func__);

	sec_param_data.offset = 0;
	sec_param_data.val = '0';

	sec_param_data_str.offset = 0;
	memset(sec_param_data_str.str, 0, sizeof(sec_param_data_str.str));
	sec_param_data_str.direction = 0;

	INIT_WORK(&sec_param_data.sec_param_work, sec_param_update);
	init_completion(&sec_param_data_str.work);
	INIT_WORK(&sec_param_data_str.sec_param_work_str, sec_param_update_str);

	return 0;
}

static void __exit sec_param_work_exit(void)
{
	cancel_work_sync(&sec_param_data.sec_param_work);
	pr_info("%s: exit\n", __func__);
}

module_init(sec_param_work_init);
module_exit(sec_param_work_exit);
