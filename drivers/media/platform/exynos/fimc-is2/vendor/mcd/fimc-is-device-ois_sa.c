/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#ifdef CONFIG_OIS_FW_UPDATE_THREAD_USE
#include <linux/kthread.h>
#endif

#include <exynos-fimc-is-sensor.h>
#include "fimc-is-core.h"
#include "fimc-is-interface.h"
#include "fimc-is-sec-define.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-dt.h"
#include "fimc-is-device-ois.h"
#include "fimc-is-vender-specific.h"
#ifdef CONFIG_AF_HOST_CONTROL
#include "fimc-is-device-af.h"
#endif
#include <linux/pinctrl/pinctrl.h>
#include "fimc-is-device-ois_sa.h"

static u8 bootCode[OIS_BOOT_FW_SIZE] = {0,};
static u8 progCode[OIS_PROG_FW_SIZE] = {0,};
static bool fw_sdcard = false;
extern struct fimc_is_ois_info ois_minfo;
extern struct fimc_is_ois_info ois_pinfo;
extern struct fimc_is_ois_info ois_uinfo;
extern struct fimc_is_ois_exif ois_exif_data;

int fimc_is_ois_self_test_impl(struct fimc_is_core *core)
{
	int ret = 0;
	u8 val = 0;
	int retries = 20;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	info("%s : E\n", __FUNCTION__);

	ret = fimc_is_ois_i2c_write(client, 0x0014, 0x08);
	if (ret) {
		err("i2c write fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0014, &val);
		if (ret != 0) {
			val = -EIO;
			break;
		}
		msleep(10);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	ret = fimc_is_ois_i2c_read(client, 0x0004, &val);
	if (ret != 0) {
		val = -EIO;
	}

	info("%s(%d) : X\n", __FUNCTION__, val);
	return (int)val;
}

bool fimc_is_ois_diff_test_impl(struct fimc_is_core *core, int *x_diff, int *y_diff)
{
	int ret = 0;
	u8 val = 0, x = 0, y = 0;
	u16 x_min = 0, y_min = 0, x_max = 0, y_max = 0;
	int retries = 20, default_diff = 1100;
	u8 read_x[2], read_y[2];
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

#ifdef CONFIG_AF_HOST_CONTROL
	fimc_is_af_move_lens(core);
	msleep(30);
#endif

	info("(%s) : E\n", __FUNCTION__);

	ret = fimc_is_ois_i2c_read_multi(client, 0x021A, read_x, 2);
	ret |= fimc_is_ois_i2c_read_multi(client, 0x021C, read_y, 2);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_write_multi(client, 0x0022, read_x, 4);
	ret |= fimc_is_ois_i2c_write_multi(client, 0x0024, read_y, 4);
	ret |= fimc_is_ois_i2c_write(client, 0x0002, 0x02);
	ret |= fimc_is_ois_i2c_write(client, 0x0000, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}
	msleep(400);
	info("(%s) : OIS Position = Center\n", __FUNCTION__);

	ret = fimc_is_ois_i2c_write(client, 0x0000, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	do {
		ret = fimc_is_ois_i2c_read(client, 0x0001, &val);
		if (ret != 0) {
			break;
		}
		msleep(10);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val != 1);

	ret = fimc_is_ois_i2c_write(client, 0x0034, 0x64);
	ret |= fimc_is_ois_i2c_write(client, 0x0230, 0x64);
	ret |= fimc_is_ois_i2c_write(client, 0x0231, 0x00);
	ret |= fimc_is_ois_i2c_write(client, 0x0232, 0x64);
	ret |= fimc_is_ois_i2c_write(client, 0x0233, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_read(client, 0x0034, &val);
	info("OIS[read_val_0x0034::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0230, &val);
	info("OIS[read_val_0x0230::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0231, &val);
	info("OIS[read_val_0x0231::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0232, &val);
	info("OIS[read_val_0x0232::0x%04x]\n", val);
	ret |= fimc_is_ois_i2c_read(client, 0x0233, &val);
	info("OIS[read_val_0x0233::0x%04x]\n", val);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_write(client, 0x020E, 0x00);
	ret |= fimc_is_ois_i2c_write(client, 0x020F, 0x00);
	ret |= fimc_is_ois_i2c_write(client, 0x0210, 0x50);
	ret |= fimc_is_ois_i2c_write(client, 0x0211, 0x50);
	if (ret) {
		err("i2c write fail\n");
	}

	ret = fimc_is_ois_i2c_write(client, 0x0013, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

#if 0
	ret = fimc_is_ois_i2c_write(client, 0x0012, 0x01);
	if (ret) {
		err("i2c write fail\n");
	}

	retries = 30;
	do { //polarity check
		ret = fimc_is_ois_i2c_read(client, 0x0012, &val);
		if (ret != 0) {
			break;
		}
		msleep(100);
		if (--retries < 0) {
			err("Polarity check is not done or not [read_val_0x0012::0x%04x]\n", val);
			break;
		}
	} while (val);
	fimc_is_ois_i2c_read(client, 0x0200, &val);
	err("OIS[read_val_0x0200::0x%04x]\n", val);
#endif

	retries = 120;
	do {
		ret = fimc_is_ois_i2c_read(client, 0x0013, &val);
		if (ret != 0) {
			break;
		}
		msleep(100);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", val);
			break;
		}
	} while (val);

	fimc_is_ois_i2c_read(client, 0x0004, &val);
	info("OIS[read_val_0x0004::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0005, &val);
	info("OIS[read_val_0x0005::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0006, &val);
	info("OIS[read_val_0x0006::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0022, &val);
	info("OIS[read_val_0x0022::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0023, &val);
	info("OIS[read_val_0x0023::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0024, &val);
	info("OIS[read_val_0x0024::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0025, &val);
	info("OIS[read_val_0x0025::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0200, &val);
	info("OIS[read_val_0x0200::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x020E, &val);
	info("OIS[read_val_0x020E::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x020F, &val);
	info("OIS[read_val_0x020F::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0210, &val);
	info("OIS[read_val_0x0210::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0211, &val);
	info("OIS[read_val_0x0211::0x%04x]\n", val);

	fimc_is_ois_i2c_read(client, 0x0212, &val);
	x = val;
	info("OIS[read_val_0x0212::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0213, &val);
	x_max = (val << 8) | x;
	info("OIS[read_val_0x0213::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0214, &val);
	x = val;
	info("OIS[read_val_0x0214::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0215, &val);
	x_min = (val << 8) | x;
	info("OIS[read_val_0x0215::0x%04x]\n", val);

	fimc_is_ois_i2c_read(client, 0x0216, &val);
	y = val;
	info("OIS[read_val_0x0216::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0217, &val);
	y_max = (val << 8) | y;
	info("OIS[read_val_0x0217::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0218, &val);
	y = val;
	info("OIS[read_val_0x0218::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x0219, &val);
	y_min = (val << 8) | y;
	info("OIS[read_val_0x0219::0x%04x]\n", val);

	fimc_is_ois_i2c_read(client, 0x021A, &val);
	info("OIS[read_val_0x021A::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x021B, &val);
	info("OIS[read_val_0x021B::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x021C, &val);
	info("OIS[read_val_0x021C::0x%04x]\n", val);
	fimc_is_ois_i2c_read(client, 0x021D, &val);
	info("OIS[read_val_0x021D::0x%04x]\n", val);

	*x_diff = abs(x_max - x_min);
	*y_diff = abs(y_max - y_min);

	info("(%s) : X (default_diff:%d)(%d,%d)\n", __FUNCTION__,
			default_diff, *x_diff, *y_diff);

	ret = fimc_is_ois_i2c_read_multi(client, 0x021A, read_x, 2);
	ret |= fimc_is_ois_i2c_read_multi(client, 0x021C, read_y, 2);
	if (ret) {
		err("i2c read fail\n");
	}

	ret = fimc_is_ois_i2c_write_multi(client, 0x0022, read_x, 4);
	ret |= fimc_is_ois_i2c_write_multi(client, 0x0024, read_y, 4);
	ret |= fimc_is_ois_i2c_write(client, 0x0002, 0x02);
	ret |= fimc_is_ois_i2c_write(client, 0x0000, 0x01);
	msleep(400);
	ret |= fimc_is_ois_i2c_write(client, 0x0000, 0x00);
	if (ret) {
		err("i2c write fail\n");
	}

	if (*x_diff > default_diff  && *y_diff > default_diff) {
		return true;
	} else {
		return false;
	}
}

bool fimc_is_ois_auto_test_impl(struct fimc_is_core *core,
		            int threshold, bool *x_result, bool *y_result, int *sin_x, int *sin_y);

{
	bool value = false;

	return value;
}

bool fimc_is_ois_fw_version(struct fimc_is_core *core)
{
	int ret = 0;
	char version[7] = {0, };
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ret = fimc_is_ois_i2c_read(client, 0x00F9, &version[0]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x00F8, &version[1]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007C, &version[2]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007D, &version[3]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007E, &version[4]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	ret = fimc_is_ois_i2c_read(client, 0x007F, &version[5]);
	if (ret) {
		err("i2c read fail\n");
		goto exit;
	}

	memcpy(ois_minfo.header_ver, version, 6);
	specific->ois_ver_read = true;

	fimc_is_ois_fw_status(core);

	return true;

exit:
	return false;
}

int fimc_is_ois_fw_revision(char *fw_ver)
{
	int revision = 0;
	revision = revision + ((int)fw_ver[FW_RELEASE_MONTH] - 64) * 100;
	revision = revision + ((int)fw_ver[FW_RELEASE_COUNT] - 48) * 10;
	revision = revision + (int)fw_ver[FW_RELEASE_COUNT + 1] - 48;

	return revision;
}

bool fimc_is_ois_read_userdata(struct fimc_is_core *core)
{
	u8 SendData[2], RcvData[256];
	u8 ois_shift_info = 0;

	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);
	struct fimc_is_vender_specific *specific = core->vender.private_data;

	/* Read OIS Shift CAL */
	fimc_is_ois_i2c_write(client ,0x000F, 0x0E);
	SendData[0] = 0x00;
	SendData[1] = 0x04;
	fimc_is_ois_i2c_write_multi(client, 0x0010, SendData, 4);
	fimc_is_ois_i2c_write(client ,0x000E, 0x04);

	fimc_is_ois_i2c_read(client, 0x0100, &ois_shift_info);
	info("OIS Shift Info : 0x%x\n", ois_shift_info);

	if (ois_shift_info == 0x11) {
		u16 ois_shift_checksum = 0;
		u16 ois_shift_x_diff = 0;
		u16 ois_shift_y_diff = 0;
		s16 ois_shift_x_cal = 0;
		s16 ois_shift_y_cal = 0;
		int i = 0;
		u8 calData[2];

		/* OIS Shift CheckSum */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0102, calData, 2);
		ois_shift_checksum = (calData[1] << 8)|(calData[0]);
		info("OIS Shift CheckSum = 0x%x\n", ois_shift_checksum);

		/* OIS Shift X Diff */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0104, calData, 2);
		ois_shift_x_diff = (calData[1] << 8)|(calData[0]);
		info("OIS Shift X Diff = 0x%x\n", ois_shift_x_diff);

		/* OIS Shift Y Diff */
		calData[0] = 0;
		calData[1] = 0;
		fimc_is_ois_i2c_read_multi(client, 0x0106, calData, 2);
		ois_shift_y_diff = (calData[1] << 8)|(calData[0]);
		info("OIS Shift Y Diff = 0x%x\n", ois_shift_y_diff);

		/* OIS Shift CAL DATA */
		for (i = 0; i< 9; i++) {
			calData[0] = 0;
			calData[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0108 + i*2, calData, 2);
			ois_shift_x_cal = (calData[1] << 8)|(calData[0]);

			calData[0] = 0;
			calData[1] = 0;
			fimc_is_ois_i2c_read_multi(client, 0x0120 + i*2, calData, 2);
			ois_shift_y_cal = (calData[1] << 8)|(calData[0]);
			info("OIS CAL[%d]:X[%d], Y[%d]\n", i, ois_shift_x_cal, ois_shift_y_cal);
		}
	}

	/* Read OIS FW */
	fimc_is_ois_i2c_write(client ,0x000F, 0x40);
	SendData[0] = 0x00;
	SendData[1] = 0x00;
	fimc_is_ois_i2c_write_multi(client, 0x0010, SendData, 4);
	fimc_is_ois_i2c_write(client ,0x000E, 0x04);

	fimc_is_ois_i2c_read(client, 0x000E, &RcvData[0]);
	if (RcvData[0] == 0x14) {
		fimc_is_ois_i2c_read_multi(client, 0x0100, RcvData, FW_TRANS_SIZE);
		memcpy(ois_uinfo.header_ver, RcvData, 6);
		return true;
	} else {
		return false;
	}
}

int fimc_is_ois_open_fw(struct fimc_is_core *core, char *name, u8 **buf)
{
	int ret = 0;
	ulong size = 0;
	const struct firmware *fw_blob = NULL;
	static char fw_name[100];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;
	int fw_requested = 1;
	int retry_count = 0;
	struct fimc_is_device_ois *ois_device = NULL;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);

	ois_device = i2c_get_clientdata(client);

	fw_sdcard = false;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(fw_name, sizeof(fw_name), "%s%s", FIMC_IS_OIS_SDCARD_PATH, name);
	fp = filp_open(fw_name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		info("failed to open SDCARD fw!!!\n");
		goto request_fw;
	}

	fw_requested = 0;
	size = fp->f_path.dentry->d_inode->i_size;
	info("start read sdcard, file path %s, size %lu Bytes\n", fw_name, size);

	*buf = vmalloc(size);
	if (!(*buf)) {
		err("failed to allocate memory");
		ret = -ENOMEM;
		goto p_err;
	}

	nread = vfs_read(fp, (char __user *)(*buf), size, &fp->f_pos);
	if (nread != size) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto p_err;
	}

	memcpy(ois_pinfo.header_ver, *buf + OIS_BIN_HEADER, OIS_FW_HEADER_SIZE);
	memcpy(bootCode, *buf, OIS_BOOT_FW_SIZE);
	memcpy(progCode, *buf + OIS_BIN_LEN - OIS_PROG_FW_SIZE, OIS_PROG_FW_SIZE);

	fw_sdcard = true;
	if (OIS_BIN_LEN >= nread) {
		ois_device->not_crc_bin = true;
		err("ois fw binary size = %ld.\n", nread);
	}

request_fw:
	if (fw_requested) {
		snprintf(fw_name, sizeof(fw_name), "%s", name);
		set_fs(old_fs);
		retry_count = 3;
		ret = request_firmware(&fw_blob, fw_name, &core->preproc.pdev->dev);
		while (--retry_count && ret == -EAGAIN) {
			err("request_firmware retry(count:%d)", retry_count);
			ret = request_firmware(&fw_blob, fw_name, &core->preproc.pdev->dev);
		}

		if (ret) {
			err("request_firmware is fail(ret:%d)", ret);
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

		*buf = vmalloc(size);
		if (!(*buf)) {
			err("failed to allocate memory");
			ret = -ENOMEM;
			goto p_err;
		}
		memcpy((void *)(*buf), fw_blob->data, size);
		memcpy(ois_pinfo.header_ver, *buf + OIS_BIN_HEADER, OIS_FW_HEADER_SIZE);
		memcpy(bootCode, *buf, OIS_BOOT_FW_SIZE);
		memcpy(progCode, *buf + OIS_BIN_LEN - OIS_PROG_FW_SIZE, OIS_PROG_FW_SIZE);

		if (OIS_BIN_LEN >= size) {
			ois_device->not_crc_bin = true;
			err("ois fw binary size = %lu.\n", size);
		}
		info("OIS firmware is loaded from Phone binary.\n");
	}

p_err:
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

	return ret;
}

bool fimc_is_ois_check_fw_impl(struct fimc_is_core *core)
{
	u8 *buf = NULL;
	int ret = 0;


	ret = fimc_is_ois_fw_version(core);
	if (!ret) {
		err("Failed to read ois fw version.");
		return false;
	}

	fimc_is_ois_read_userdata(core);

	if (ois_minfo.header_ver[FW_GYRO_SENSOR] == '6') {
		if (ois_minfo.header_ver[FW_CORE_VERSION] == 'C' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'E'
			|| ois_minfo.header_ver[FW_CORE_VERSION] == 'G' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'I') {
			ret = fimc_is_ois_open_fw(core, FIMC_OIS_FW_NAME_DOM, &buf);
		} else if (ois_minfo.header_ver[FW_CORE_VERSION] == 'D' ||
		ois_minfo.header_ver[FW_CORE_VERSION] == 'F'
			|| ois_minfo.header_ver[FW_CORE_VERSION] == 'H' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'J') {
			ret = fimc_is_ois_open_fw(core, FIMC_OIS_FW_NAME_SEC, &buf);
		}
	} else if (ois_minfo.header_ver[FW_GYRO_SENSOR] == '8') {
		if (ois_minfo.header_ver[FW_CORE_VERSION] == 'H' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'J') {
			ret = fimc_is_ois_open_fw(core, FIMC_OIS_FW_NAME_SEC_2, &buf);
		}
	} else {
		info("Module FW version does not matched with phone FW version.\n");
		strcpy(ois_pinfo.header_ver, "NULL");
		return;
	}

	if (ret) {
		err("fimc_is_ois_open_fw failed(%d)\n", ret);
	}

	if (buf) {
		vfree(buf);
	}

	return true;
}

void fimc_is_ois_fw_update_impl(struct fimc_is_core *core)
{
	u8 SendData[256], RcvData;
	u16 RcvDataShort = 0;
	u8 data[2] = {0, };
	int block, i, position = 0;
	u16 checkSum;
	u8 *buf = NULL;
	int ret = 0, sum_size = 0, retry_count = 3;
	int module_ver = 0, binary_ver = 0;
	bool forced_update = false, crc_result = false;
	bool need_reset = false;
	u8 ois_status = 0;
	struct i2c_client *client = fimc_is_ois_i2c_get_client(core);
	struct fimc_is_vender_specific *specific = core->vender.private_data;

	/* OIS Status Check */
	ois_status = fimc_is_ois_read_status(core);
	if (ois_status == 0x01) {
		forced_update = true;
	}

	ret = fimc_is_ois_fw_version(core);
	if (!ret) {
		err("Failed to read ois fw version.");
		return;
	}
	fimc_is_ois_read_userdata(core);

	if (ois_minfo.header_ver[FW_GYRO_SENSOR] == '6') {
		if (ois_minfo.header_ver[FW_CORE_VERSION] == 'C' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'E'
			|| ois_minfo.header_ver[FW_CORE_VERSION] == 'G' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'I') {
			ret = fimc_is_ois_open_fw(core, FIMC_OIS_FW_NAME_DOM, &buf);
		} else if (ois_minfo.header_ver[FW_CORE_VERSION] == 'D' ||
		ois_minfo.header_ver[FW_CORE_VERSION] == 'F'
			|| ois_minfo.header_ver[FW_CORE_VERSION] == 'H' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'J') {
			ret = fimc_is_ois_open_fw(core, FIMC_OIS_FW_NAME_SEC, &buf);
		}
	} else if (ois_minfo.header_ver[FW_GYRO_SENSOR] == '8') {
		if (ois_minfo.header_ver[FW_CORE_VERSION] == 'H' ||
			ois_minfo.header_ver[FW_CORE_VERSION] == 'J') {
			ret = fimc_is_ois_open_fw(core, FIMC_OIS_FW_NAME_SEC_2, &buf);
		}
	} else {
		info("Module FW version does not matched with phone FW version.\n");
		strcpy(ois_pinfo.header_ver, "NULL");
		return;
	}
	if (ret) {
		err("fimc_is_ois_open_fw failed(%d)", ret);
	}

	if (ois_minfo.header_ver[FW_CORE_VERSION] != CAMERA_OIS_DOM_UPDATE_VERSION
		&& ois_minfo.header_ver[FW_CORE_VERSION] != CAMERA_OIS_SEC_UPDATE_VERSION) {
		info("Do not update ois Firmware. FW version is low.\n");
		goto p_err;
	}

	if (buf == NULL) {
		err("buf is NULL. OIS FW Update failed.");
		return;
	}

	if (!forced_update) {
		if (ois_uinfo.header_ver[0] == 0xFF && ois_uinfo.header_ver[1] == 0xFF &&
			ois_uinfo.header_ver[2] == 0xFF) {
			err("OIS userdata is not valid.");
			goto p_err;
		}
	}

	/*Update OIS FW when Gyro sensor/Driver IC/ Core version is same*/
	if (!forced_update) {
		if (!fimc_is_ois_version_compare(ois_minfo.header_ver, ois_pinfo.header_ver,
			ois_uinfo.header_ver)) {
			err("Does not update ois fw. OIS module ver = %s, binary ver = %s, userdata ver = %s",
				ois_minfo.header_ver, ois_pinfo.header_ver, ois_uinfo.header_ver);
			goto p_err;
		}
	} else {
		if (!fimc_is_ois_version_compare_default(ois_minfo.header_ver, ois_pinfo.header_ver)) {
			err("Does not update ois fw. OIS module ver = %s, binary ver = %s",
				ois_minfo.header_ver, ois_pinfo.header_ver);
			goto p_err;
		}
	}

	crc_result = fimc_is_ois_crc_check(core, buf);
	if (crc_result == false) {
		err("OIS CRC32 error.\n");
		goto p_err;
	}

	if (!fw_sdcard && !forced_update) {
		module_ver = fimc_is_ois_fw_revision(ois_minfo.header_ver);
		binary_ver = fimc_is_ois_fw_revision(ois_pinfo.header_ver);

		if (binary_ver <= module_ver) {
			err("OIS module ver = %d, binary ver = %d", module_ver, binary_ver);
			goto p_err;
		}
	}
	info("OIS fw update started. module ver = %s, binary ver = %s, userdata ver = %s\n",
		ois_minfo.header_ver, ois_pinfo.header_ver, ois_uinfo.header_ver);

retry:
	if (need_reset) {
		fimc_is_ois_gpio_off(core);
		msleep(30);
		fimc_is_ois_gpio_on(core);
		msleep(150);
	}

	fimc_is_ois_i2c_read(client, 0x01, &RcvData); /* OISSTS Read */

	if (RcvData != 0x01) {/* OISSTS != IDLE */
		err("RCV data return!!");
		goto p_err;
	}

	/* Update a Boot Program */
	/* SET FWUP_CTRL REG */
	fimc_is_ois_i2c_write(client ,0x000C, 0x0B);
	msleep(20);

	position = 0;
	/* Write Boot Data */
	for (block = 4; block < 8; block++) {       /* 0x1000 - 0x1FFF */
		for (i = 0; i < 4; i++) {
			memcpy(SendData, bootCode + position, FW_TRANS_SIZE);
			fimc_is_ois_i2c_write_multi(client, 0x0100, SendData, FW_TRANS_SIZE + 2);
			position += FW_TRANS_SIZE;
			if (i == 0 ) {
				msleep(20);
			} else if ((i == 1) || (i == 2)) {	 /* Wait Erase and Write process */
				 msleep(10);		/* Wait Write process */
			} else {
				msleep(15); /* Wait Write and Verify process */
			}
		}
	}

	/* CHECKSUM (Boot) */
	sum_size = OIS_BOOT_FW_SIZE;
	checkSum = fimc_is_ois_calc_checksum(&bootCode[0], sum_size);
	SendData[0] = (checkSum & 0x00FF);
	SendData[1] = (checkSum & 0xFF00) >> 8;
	SendData[2] = 0x00;
	SendData[3] = 0x00;
	fimc_is_ois_i2c_write_multi(client, 0x08, SendData, 6);
	msleep(10);			 /* (RUMBA Self Reset) */

	fimc_is_ois_i2c_read_multi(client, 0x06, data, 2); /* Error Status read */
	info("%s: OISSTS Read = 0x%02x%02x\n", __func__, data[1], data[0]);

	if (data[1] == 0x00 && data[0] == 0x01) {					/* Boot F/W Update Error check */
		/* Update a User Program */
		/* SET FWUP_CTRL REG */
		position = 0;
		SendData[0] = 0x09;		/* FWUP_DSIZE=256Byte, FWUP_MODE=PROG, FWUPEN=Start */
		fimc_is_ois_i2c_write(client, 0x0C, SendData[0]);		/* FWUPCTRL REG(0x000C) 1Byte Send */

		/* Write UserProgram Data */
		for (block = 4; block <= 27; block++) {		/* 0x1000 -0x 6FFF (RUMBA-SA)*/
			for (i = 0; i < 4; i++) {
				memcpy(SendData, &progCode[position], (size_t)FW_TRANS_SIZE);
				fimc_is_ois_i2c_write_multi(client, 0x100, SendData, FW_TRANS_SIZE + 2); /* FLS_DATA REG(0x0100) 256Byte Send */
				position += FW_TRANS_SIZE;
				if (i ==0 ) {
					msleep(20);		/* Wait Erase and Write process */
				} else if ((i==1) || (i==2)) {
					msleep(10);		/* Wait Write process */
				} else {
					msleep(15);		/* Wait Write and Verify process */
				}
			}
		}

		/* CHECKSUM (Program) */
		sum_size = OIS_PROG_FW_SIZE;
		checkSum = fimc_is_ois_calc_checksum(&progCode[0], sum_size);
		SendData[0] = (checkSum & 0x00FF);
		SendData[1] = (checkSum & 0xFF00) >> 8;
		SendData[2] = 0;		/* Don't Care */
		SendData[3] = 0x80;		/* Self Reset Request */

		fimc_is_ois_i2c_write_multi(client, 0x08, SendData, 6);  /* FWUP_CHKSUM REG(0x0008) 4Byte Send */
		msleep(60);		/* (Wait RUMBA Self Reset) */
		fimc_is_ois_i2c_read(client, 0x6, (u8*)&RcvDataShort); /* Error Status read */

		if (RcvDataShort == 0x0000) {
			/* F/W Update Success Process */
			info("%s: OISLOG OIS program update success\n", __func__);
		} else {
			/* Retry Process */
			if (retry_count > 0) {
				err("OISLOG OIS program update fail, retry update. count = %d", retry_count);
				retry_count--;
				need_reset = true;
				goto retry;
			}

			/* F/W Update Error Process */
			err("OISLOG OIS program update fail");
			goto p_err;
		}
	} else {
		/* Retry Process */
		if (retry_count > 0) {
			err("OISLOG OIS boot update fail, retry update. count = %d", retry_count);
			retry_count--;
			need_reset = true;
			goto retry;
		}

		/* Error Process */
		err("CAN'T process OIS program update");
		goto p_err;
	}

	/* S/W Reset */
	fimc_is_ois_i2c_write(client ,0x000D, 0x01);
	fimc_is_ois_i2c_write(client ,0x000E, 0x06);

	msleep(30);
	ret = fimc_is_ois_fw_version(core);
	if (!ret) {
		err("Failed to read ois fw version.");
		goto p_err;
	}

	if (!fimc_is_ois_version_compare_default(ois_minfo.header_ver, ois_pinfo.header_ver)) {
		err("After update ois fw is not correct. OIS module ver = %s, binary ver = %s",
			ois_minfo.header_ver, ois_pinfo.header_ver);
		goto p_err;
	}

	/* Param init - Flash to Rumba */
	fimc_is_ois_i2c_write(client ,0x0036, 0x03);
	msleep(30);

p_err:
	info("OIS module ver = %s, binary ver = %s, userdata ver = %s\n",
		ois_minfo.header_ver, ois_pinfo.header_ver, ois_uinfo.header_ver);

	if (buf) {
		vfree(buf);
	}
	return;
}

MODULE_DESCRIPTION("OIS driver for Rumba SA");
MODULE_AUTHOR("kyoungho yun <kyoungho.yun@samsung.com>");
MODULE_LICENSE("GPL v2");
