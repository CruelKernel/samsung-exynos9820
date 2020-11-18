/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/syscalls.h>
#include <linux/vmalloc.h>

#include "npu-binary.h"
#include "npu-log.h"

static noinline_for_stack long __get_file_size(struct file *file)
{
	struct kstat st;

	if (vfs_getattr(&file->f_path, &st, STATX_TYPE | STATX_MODE, AT_STATX_SYNC_AS_STAT)) {
		return -1;
	}
	if (!S_ISREG(st.mode)) {
		return -1;
	}
	if (st.size != (long)st.size) {
		return -1;
	}
	return st.size;
}

int npu_binary_init(struct npu_binary *binary,
	struct device *dev,
	char *fpath1,
	char *fpath2,
	char *fname)
{
	BUG_ON(!binary);
	BUG_ON(!dev);

	binary->dev = dev;
	binary->image_size = 0;
	snprintf(binary->fpath1, sizeof(binary->fpath1), "%s%s", fpath1, fname);
	snprintf(binary->fpath2, sizeof(binary->fpath2), "%s%s", fpath2, fname);

	return 0;
}

int npu_binary_g_size(struct npu_binary *binary, size_t *size)
{
	int ret;
	mm_segment_t old_fs;
	struct file *fp = NULL;
	size_t fsize = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(binary->fpath1, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		set_fs(old_fs);
		ret = -EINVAL;
		fp = NULL;
		goto p_err;
	}

	fsize = __get_file_size(fp);
	if (fsize <= 0) {
		npu_err("fail(%ld) in __get_file_size\n", fsize);
		ret = -EINVAL;
		goto p_err;
	}
	ret = 0;

p_err:
	if (fp)
		filp_close(fp, current->files);
	set_fs(old_fs);
	*size = fsize;
	return ret;
}

#define MAX_SIGNATURE_LEN       128
#define SIGNATURE_HEADING       "NPU Firmware signature : "
static void print_fw_signature(char *fw_addr, size_t fw_size)
{
	char buf[MAX_SIGNATURE_LEN];            /* Buffer to hold the signature */
	char *p_fw;             /* Pointer over fw_addr */
	char *p_buf;            /* Pointer over buf */

	/* Put pointer at last position */
	buf[MAX_SIGNATURE_LEN - 1] = '\0';
	p_fw = (fw_addr + fw_size) - 1;
	p_buf = &buf[MAX_SIGNATURE_LEN - 2];

	if (*p_fw-- != ']') {
		/* No signature */
		npu_info(SIGNATURE_HEADING "none\n");
		return;
	}

	/* Extract signature */
	for (; p_fw > fw_addr && p_buf > buf && *p_fw != '['; p_buf--, p_fw--)
		*p_buf = *p_fw;

	/* Print out */
	/* *p_buf == '[' if everything was OK */
	if (p_buf > buf)
		npu_info(SIGNATURE_HEADING "%s\n", p_buf + 1);
	else
		npu_info(SIGNATURE_HEADING "invalid\n");
}

static long __npu_binary_read(struct npu_binary *binary, void *target, size_t target_size)
{
	int ret = 0;
	const struct firmware *fw_blob;
	u8 *buf = NULL;
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;
	char *fpath = NULL;

	loff_t file_offset = 0;

	BUG_ON(!binary);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fpath = binary->fpath1;
	fp = filp_open(fpath, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		fpath = binary->fpath2;
		fp = filp_open(fpath, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(fp)) {
			set_fs(old_fs);
			goto request_fw;
		}
	}

	fsize = __get_file_size(fp);
	if (fsize <= 0) {
		npu_err("fail(%ld) in __get_file_size\n", fsize);
		ret = -EBADF;
		goto p_err;
	}

	buf = vmalloc(fsize);
	if (!buf) {
		npu_err("fail in vmalloc\n");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = kernel_read(fp, buf, fsize, &file_offset);
	if (nread != fsize) {
		npu_err("fail(%ld != %ld) in kernel_read\n", nread, fsize);
		ret = -EIO;
		goto p_err;
	}

	if (fsize > target_size) {
		npu_err("size over(%ld > %ld) for image\n", fsize, target_size);
		ret = -EIO;
		goto p_err;
	}

	binary->image_size = fsize;
	memcpy(target, (void *)buf, fsize);

	npu_info("success of fw(%s, %ld) apply.\n", fpath, fsize);
	ret = fsize;

p_err:
	if (buf)
		vfree(buf);
	if (fp)
		filp_close(fp, current->files);
	set_fs(old_fs);

	return ret;

request_fw:
	ret = request_firmware(&fw_blob, NPU_FW_PATH3 NPU_FW_NAME, binary->dev);
	if (ret) {
		npu_err("fail(%d) in request_firmware(%s)\n", ret, NPU_FW_PATH3 NPU_FW_NAME);
		ret = -EINVAL;
		goto request_err;
	}

	if (!fw_blob) {
		npu_err("null in fw_blob\n");
		ret = -EINVAL;
		goto request_err;
	}

	if (!fw_blob->data) {
		npu_err("null in fw_blob->data\n");
		ret = -EINVAL;
		goto request_err;
	}

	if (fw_blob->size > target_size) {
		npu_err("image size over(%ld > %ld)\n", binary->image_size, target_size);
		ret = -EIO;
		goto p_err;
	}

	binary->image_size = fw_blob->size;
	memcpy(target, fw_blob->data, fw_blob->size);

	npu_info("success of binay(%s, %ld) apply.\n", NPU_FW_PATH3 NPU_FW_NAME, fw_blob->size);
	ret = fw_blob->size;

request_err:
	release_firmware(fw_blob);
	return ret;
}


int npu_binary_read(struct npu_binary *binary, void *target, size_t target_size)
{
	long ret = __npu_binary_read(binary, target, target_size);

	if (ret > 0)
		return 0;

	/* Error case */
	return ret;
}

int npu_firmware_file_read(struct npu_binary *binary, void *target, size_t target_size)
{
	long ret = __npu_binary_read(binary, target, target_size);

	if (ret > 0) {
		/* Print firmware signature on success */
		print_fw_signature(target, ret);
		return 0;
	}

	/* Error case */
	return ret;

}

int npu_binary_write(struct npu_binary *binary,
	void *target,
	size_t target_size)
{
	int ret = 0;
	struct file *fp;
	mm_segment_t old_fs;
	long nwrite;

	BUG_ON(!binary);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(binary->fpath1, O_RDWR | O_CREAT, 0);
	if (IS_ERR_OR_NULL(fp)) {
		set_fs(old_fs);
		npu_err("fail(%pK) in filp_open\n", fp);
		ret = -EBADF;
		goto p_err;
	}

	nwrite = kernel_write(fp, target, target_size, 0);
	if (nwrite != target_size) {
		filp_close(fp, current->files);
		set_fs(old_fs);
		npu_err("fail(%ld != %ld) in kernel_write\n", nwrite, target_size);
		ret = -EIO;
		goto p_err;
	}

	binary->image_size = target_size;

	npu_info("success of binary(%s, %ld) apply.\n", binary->fpath1, target_size);

	filp_close(fp, current->files);
	set_fs(old_fs);

p_err:
	return ret;
}
