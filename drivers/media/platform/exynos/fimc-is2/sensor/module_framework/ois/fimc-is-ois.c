/*
 * Samsung Exynos5 SoC series OIS driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/firmware.h>

#include "fimc-is-helper-ois-i2c.h"
#include "fimc-is-device-sensor-peri.h"
#include "fimc-is-ois.h"

#ifdef CONFIG_OIS_DIRECT_FW_CONTROL
int fimc_is_ois_fw_open(struct fimc_is_ois *ois, char *name)
{
	const struct firmware *fw_blob = NULL;
	int size = 0;
	int ret = 0;
	int retry_count = 0;
	long fsize = 0;
	long nread = 0;
	u8 *buf = NULL;

	char fw_name[100];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	int fw_requested = 1;

	FIMC_BUG(!ois);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(fw_name, sizeof(fw_name), "%s%s", FIMC_IS_OIS_SDCARD_PATH, name);
	fp = filp_open(fw_name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		pr_info("failed to open SDCARD fw!\n");
		goto request_fw;
	}
	fw_requested = 0;

	fsize = fp->f_path.dentry->d_inode->i_size;
	pr_info("start, file_path: %s, size: %ld Bytes\n", fw_name, fsize);

	buf = vmalloc(fsize);
	if (!buf) {
		err("failed to allocate memory");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = vfs_read(fp, (char __user *)buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		err("failed to read ois firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto p_err;
	}

	/* Firmware version save */
	ret = fimc_is_ois_fw_ver_copy(ois, buf, fsize);
	if (ret < 0) {
		err("OIS fw version copy fail\n");
		goto p_err;
	}

request_fw:
	if (fw_requested) {
		snprintf(fw_name, sizeof(fw_name), "%s", name);
		set_fs(old_fs);
		retry_count = 3;
		ret = request_firmware(&fw_blob, fw_name, ois->subdev->dev);

		while (--retry_count && ret == -EAGAIN) {
			err("request_firmware retry(count:%d)", retry_count);
			ret = request_firmware(&fw_blob, fw_name, ois->subdev->dev);
		}

		if (ret < 0) {
			err("request_firmware is fail(%d)", ret);
			ret = -EINVAL;
			goto p_err;
		}

		if (!fw_blob) {
			err("fw_blob is NULL");
			ret = -EINVAL;
			goto p_err;
		}

		if (!fw_blob->data) {
			err("fw_blob->data is NULL");
			ret = -EINVAL;
			goto p_err;
		}

		size = fw_blob->size;
		buf = vmalloc(size);
		if (!buf) {
			err("failed to allocate memory");
			ret = -ENOMEM;
			goto p_err;
		}

		memcpy((void *)buf, fw_blob->data, size);

		/* Firmware version save */
		ret = fimc_is_ois_fw_ver_copy(ois, buf, fsize);
		if (ret < 0) {
			err("OIS fw version copy fail\n");
			goto p_err;
		}
	}

p_err:

	if (buf)
		vfree(buf);

	if (!fw_requested) {
		if (!IS_ERR_OR_NULL(fp)) {
			filp_close(fp, current->files);
		}
		set_fs(old_fs);
	} else {
		if (!IS_ERR_OR_NULL(fw_blob)) {
			release_firmware(fw_blob);
		}
	}

	if (ret < 0)
		err("OIS firmware loading is fail\n");
	else
		pr_info("OIS: the FW were applied successfuly\n");

	return ret;
}
#endif

