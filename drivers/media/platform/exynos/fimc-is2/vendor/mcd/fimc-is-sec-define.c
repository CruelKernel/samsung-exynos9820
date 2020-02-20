/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "fimc-is-sec-define.h"
#include "fimc-is-vender-specific.h"
#include "fimc-is-interface-library.h"

#include <linux/i2c.h>
#include "fimc-is-device-eeprom.h"

#define prefix "[FROM]"

#define FIMC_IS_DEFAULT_CAL_SIZE		(20 * 1024)
#define FIMC_IS_DUMP_CAL_SIZE			(172 * 1024)
#define FIMC_IS_LATEST_ROM_VERSION_M	'M'

#define FIMC_IS_READ_MAX_EEP_CAL_SIZE	(32 * 1024)

bool force_caldata_dump = false;

static int cam_id = CAMERA_SINGLE_REAR;
bool is_dumped_fw_loading_needed = false;
static struct fimc_is_rom_info sysfs_finfo[ROM_ID_MAX];
static struct fimc_is_rom_info sysfs_pinfo[ROM_ID_MAX];
static char rom_buf[ROM_ID_MAX][FIMC_IS_MAX_CAL_SIZE];
#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
static char fw_buf[FIMC_IS_MAX_FW_BUFFER_SIZE];
#endif
char loaded_fw[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };
char loaded_companion_fw[30] = {0, };

enum {
	CAL_DUMP_STEP_INIT = 0,
	CAL_DUMP_STEP_CHECK,
	CAL_DUMP_STEP_DONE,
};

int check_need_cal_dump = CAL_DUMP_STEP_INIT;

bool fimc_is_sec_get_force_caldata_dump(void)
{
	return force_caldata_dump;
}

int fimc_is_sec_set_force_caldata_dump(bool fcd)
{
	force_caldata_dump = fcd;
	if (fcd)
		info("forced caldata dump enabled!!\n");
	return 0;
}

int fimc_is_sec_get_sysfs_finfo(struct fimc_is_rom_info **finfo, int rom_id)
{
	*finfo = &sysfs_finfo[rom_id];
	return 0;
}

int fimc_is_sec_get_sysfs_pinfo(struct fimc_is_rom_info **pinfo, int rom_id)
{
	*pinfo = &sysfs_pinfo[rom_id];
	return 0;
}

int fimc_is_sec_get_cal_buf(char **buf, int rom_id)
{
	*buf = rom_buf[rom_id];
	return 0;
}

int fimc_is_sec_get_loaded_fw(char **buf)
{
	*buf = &loaded_fw[0];
	return 0;
}

int fimc_is_sec_set_loaded_fw(char *buf)
{
	strncpy(loaded_fw, buf, FIMC_IS_HEADER_VER_SIZE);
	return 0;
}

int fimc_is_sec_set_camid(int id)
{
	cam_id = id;
	return 0;
}

int fimc_is_sec_get_camid(void)
{
	return cam_id;
}

int fimc_is_sec_fw_revision(char *fw_ver)
{
	int revision = 0;
	revision = revision + ((int)fw_ver[FW_PUB_YEAR] - 58) * 10000;
	revision = revision + ((int)fw_ver[FW_PUB_MON] - 64) * 100;
	revision = revision + ((int)fw_ver[FW_PUB_NUM] - 48) * 10;
	revision = revision + (int)fw_ver[FW_PUB_NUM + 1] - 48;

	return revision;
}

bool fimc_is_sec_fw_module_compare(char *fw_ver1, char *fw_ver2)
{
	if (fw_ver1[FW_CORE_VER] != fw_ver2[FW_CORE_VER]
		|| fw_ver1[FW_PIXEL_SIZE] != fw_ver2[FW_PIXEL_SIZE]
		|| fw_ver1[FW_PIXEL_SIZE + 1] != fw_ver2[FW_PIXEL_SIZE + 1]
		|| fw_ver1[FW_ISP_COMPANY] != fw_ver2[FW_ISP_COMPANY]
		|| fw_ver1[FW_SENSOR_MAKER] != fw_ver2[FW_SENSOR_MAKER]) {
		return false;
	}

	return true;
}

bool fimc_is_sec_fw_module_compare_for_dump(char *fw_ver1, char *fw_ver2)
{
	if (fw_ver1[FW_CORE_VER] != fw_ver2[FW_CORE_VER]
		|| fw_ver1[FW_PIXEL_SIZE] != fw_ver2[FW_PIXEL_SIZE]
		|| fw_ver1[FW_PIXEL_SIZE + 1] != fw_ver2[FW_PIXEL_SIZE + 1]
		|| fw_ver1[FW_ISP_COMPANY] != fw_ver2[FW_ISP_COMPANY]) {
		return false;
	}

	return true;
}

int fimc_is_sec_compare_ver(int rom_id)
{
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	if (finfo->cal_map_ver[0] == 'V'
		&& finfo->cal_map_ver[1] == '0'
		&& finfo->cal_map_ver[2] >= '0' && finfo->cal_map_ver[2] <= '9'
		&& finfo->cal_map_ver[3] >= '0' && finfo->cal_map_ver[3] <= '9') {
		return ((finfo->cal_map_ver[2] - '0') * 10) + (finfo->cal_map_ver[3] - '0');
	} else {
		err("ROM core version is invalid. version is %c%c%c%c",
			finfo->cal_map_ver[0], finfo->cal_map_ver[1], finfo->cal_map_ver[2], finfo->cal_map_ver[3]);
		return -1;
	}
}

bool fimc_is_sec_check_rom_ver(struct fimc_is_core *core, int rom_id)
{
	struct fimc_is_rom_info *finfo = NULL;
	char compare_version;
	int rom_ver;
	int latest_rom_ver;

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	if (test_bit(FIMC_IS_ROM_STATE_SKIP_CAL_LOADING, &finfo->rom_state)) {
		err("[rom_id:%d] skip_cal_loading implemented", rom_id);
		return false;
	}

	if (test_bit(FIMC_IS_ROM_STATE_SKIP_HEADER_LOADING, &finfo->rom_state)) {
		err("[rom_id:%d] skip_header_loading implemented", rom_id);
		return true;
	}

	latest_rom_ver = finfo->cal_map_es_version;
	compare_version = finfo->camera_module_es_version;

	rom_ver = fimc_is_sec_compare_ver(rom_id);

	if ((rom_ver < latest_rom_ver) ||
		(finfo->header_ver[10] < compare_version)) {
		err("[%d]invalid rom version. rom_ver %c, header_ver[10] %c\n",
			rom_id, rom_ver, finfo->header_ver[10]);
		return false;
	} else {
		return true;
	}
}

bool fimc_is_sec_check_cal_crc32(char *buf, int rom_id)
{
	u32 *buf32 = NULL;
	u32 checksum;
	u32 check_base;
	u32 check_length;
	u32 checksum_base;
	u32 address_boundary;
	bool crc32_temp, crc32_header_temp, crc32_dual_temp;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_core *core;
	struct fimc_is_vender_specific *specific;
 	int i;

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	specific = core->vender.private_data;
	buf32 = (u32 *)buf;

	info("+++ %s\n", __func__);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	finfo->crc_error = 0; /* clear all bits */

	if (test_bit(FIMC_IS_ROM_STATE_SKIP_CRC_CHECK, &finfo->rom_state)) {
		info("%s : skip crc check. return\n", __func__);
		return true;
	}

	crc32_temp = true;
	crc32_header_temp = true;
	crc32_dual_temp = true;

	address_boundary = FIMC_IS_MAX_CAL_SIZE;

	/* header crc check */
	for (i = 0; i < finfo->header_crc_check_list_len; i += 3) {
		checksum = 0;
		check_base = finfo->header_crc_check_list[i] / 4;
		check_length = (finfo->header_crc_check_list[i+1] - finfo->header_crc_check_list[i] + 1) ;
		checksum_base = finfo->header_crc_check_list[i+2] / 4;

		if (check_base > address_boundary || checksum_base > address_boundary || check_length <= 0) {
			err("[rom%d/header cal:%d] address has error: start(0x%08X), end(0x%08X)",
				rom_id, i, finfo->header_crc_check_list[i], finfo->header_crc_check_list[i+1]);
			crc32_header_temp = false;
			goto out;
		}

		checksum = (u32)getCRC((u16 *)&buf32[check_base], check_length, NULL, NULL);
		if (checksum != buf32[checksum_base]) {
			err("[rom%d/header cal:%d] CRC32 error (0x%08X != 0x%08X), base[0x%X] len[0x%X] checksum[0x%X]",
				rom_id, i, checksum, buf32[checksum_base], check_base, check_length, checksum_base);
			crc32_header_temp = false;
			goto out;
		}
	}

	/* main crc check */
	for (i = 0; i < finfo->crc_check_list_len; i += 3) {
		checksum = 0;
		check_base = finfo->crc_check_list[i] / 4;
		check_length = (finfo->crc_check_list[i+1] - finfo->crc_check_list[i] + 1) ;
		checksum_base = finfo->crc_check_list[i+2] / 4;

		if (check_base > address_boundary || checksum_base > address_boundary || check_length <= 0) {
			err("[rom%d/main cal:%d] address has error: start(0x%08X), end(0x%08X)",
				rom_id, i, finfo->crc_check_list[i], finfo->crc_check_list[i+1]);
			crc32_temp = false;
			goto out;
		}

		checksum = (u32)getCRC((u16 *)&buf32[check_base], check_length, NULL, NULL);
		if (checksum != buf32[checksum_base]) {
			err("[rom%d/main cal:%d] CRC32 error (0x%08X != 0x%08X), base[0x%X] len[0x%X] checksum[0x%X]",
				rom_id, i, checksum, buf32[checksum_base], check_base, check_length, checksum_base);
			crc32_temp = false;
			goto out;
		}
	}

	/* dual crc check */
	for (i = 0; i < finfo->dual_crc_check_list_len; i += 3) {
		checksum = 0;
		check_base = finfo->dual_crc_check_list[i] / 4;
		check_length = (finfo->dual_crc_check_list[i+1] - finfo->dual_crc_check_list[i] + 1) ;
		checksum_base = finfo->dual_crc_check_list[i+2] / 4;

		if (check_base > address_boundary || checksum_base > address_boundary || check_length <= 0) {
			err("[rom%d/dual cal:%d] data address has error: start(0x%08X), end(0x%08X)",
				rom_id, i, finfo->dual_crc_check_list[i], finfo->dual_crc_check_list[i+1]);
			crc32_temp = false;
			crc32_dual_temp = false;
			goto out;
		}

		checksum = (u32)getCRC((u16 *)&buf32[check_base], check_length, NULL, NULL);
		if (checksum != buf32[checksum_base]) {
			err("[rom%d/main cal:%d] CRC32 error (0x%08X != 0x%08X), base[0x%X] len[0x%X] checksum[0x%X]",
				rom_id, i, checksum, buf32[checksum_base], check_base, check_length, checksum_base);
			crc32_temp = false;
			crc32_dual_temp = false;
			goto out;
		}
	}

out:
	if (!crc32_temp)
		set_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error);
	if (!crc32_header_temp)
		set_bit(FIMC_IS_CRC_ERROR_HEADER, &finfo->crc_error);
	if (!crc32_dual_temp)
		set_bit(FIMC_IS_CRC_ERROR_DUAL_CAMERA, &finfo->crc_error);

	info("[rom_id:%d] crc32_check %d crc32_header %d crc32_dual %d\n",
		rom_id, crc32_temp, crc32_header_temp, crc32_dual_temp);

	return crc32_temp && crc32_header_temp;
}

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
bool fimc_is_sec_check_fw_crc32(char *buf, u32 checksum_seed, unsigned long size)
{
	u32 *buf32 = NULL;
	u32 checksum;
	u32 checksum_base;

	struct fimc_is_rom_info *finfo = NULL;
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	buf32 = (u32 *)buf;

	info("Camera: Start checking CRC32 FW\n");

	checksum = (u32)getCRC((u16 *)&buf32[0], size, NULL, NULL);
	checksum_base = (checksum_seed & 0xffffffff) / 4;
	if (checksum != buf32[checksum_base]) {
		err("Camera: CRC32 error at the binary section (0x%08X != 0x%08X) %x, %x",
					checksum, buf32[checksum_base], checksum_base, checksum_seed);
		set_bit(FIMC_IS_CRC_ERROR_FIRMWARE, &finfo->crc_error);
	} else {
		clear_bit(FIMC_IS_CRC_ERROR_FIRMWARE, &finfo->crc_error);
	}

	info("Camera: End checking CRC32 FW\n");

	return !test_bit(FIMC_IS_CRC_ERROR_FIRMWARE, &finfo->crc_error);
}
#endif

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
bool fimc_is_sec_check_setfile_crc32(char *buf)
{
	u32 *buf32 = NULL;
	u32 checksum;
	u32 checksum_base;
	u32 checksum_seed;
	struct fimc_is_rom_info *finfo = NULL;

	buf32 = (u32 *)buf;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY)
		checksum_seed = CHECKSUM_SEED_SETF_LL;
	else
		checksum_seed = CHECKSUM_SEED_SETF_LS;

	info("Camera: Start checking CRC32 Setfile\n");

	checksum_seed -= finfo->setfile_start_addr;
	checksum = (u32)getCRC((u16 *)&buf32[0], finfo->setfile_size, NULL, NULL);
	checksum_base = (checksum_seed & 0xffffffff) / 4;
	if (checksum != buf32[checksum_base]) {
		err("Camera: CRC32 error at the binary section (0x%08X != 0x%08X) %x %x",
					checksum, buf32[checksum_base], checksum_base, checksum_seed);
		set_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error);
	} else {
		clear_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error);
	}

	info("Camera: End checking CRC32 Setfile\n");

	return !test_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error);
}

#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
bool fimc_is_sec_check_front_setfile_crc32(char *buf)
{
	u32 *buf32 = NULL;
	u32 checksum;
	u32 checksum_base;
	u32 checksum_seed;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_front = NULL;

	buf32 = (u32 *)buf;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo_front, ROM_ID_FRONT);

	if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI)
		checksum_seed = CHECKSUM_SEED_FRONT_SETF_LL;
	else
		checksum_seed = CHECKSUM_SEED_FRONT_SETF_LS;

	info("Camera: Start checking CRC32 Front setfile\n");

	checksum_seed -= finfo->front_setfile_start_addr;
	checksum = (u32)getCRC((u16 *)&buf32[0], finfo->front_setfile_size, NULL, NULL);
	checksum_base = (checksum_seed & 0xffffffff) / 4;
	if (checksum != buf32[checksum_base]) {
		err("Camera: CRC32 error at the binary section (0x%08X != 0x%08X) %d %d",
					checksum, buf32[checksum_base], checksum_seed, checksum_base);
		set_bit(FIMC_IS_CRC_ERROR_SETFILE_2, &finfo->crc_error);
	} else {
		clear_bit(FIMC_IS_CRC_ERROR_SETFILE_2, &finfo->crc_error);
	}

	info("Camera: End checking CRC32 front setfile\n");

	return !test_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error);
}
#endif
#ifdef CAMERA_MODULE_HIFI_TUNNINGF_DUMP
bool fimc_is_sec_check_hifi_tunningfile_crc32(char *buf)
{
	u32 *buf32 = NULL;
	u32 checksum;
	u32 checksum_base;
	u32 checksum_seed;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	buf32 = (u32 *)buf;

	checksum_seed = CHECKSUM_SEED_HIFI_TUNNING_LS;

	info("Camera: Start checking CRC32 hifi tunning file checksum_seed %x\n", checksum_seed);

	checksum_seed -= finfo->hifi_tunning_start_addr;

	checksum = (u32)getCRC((u16 *)&buf32[0], finfo->hifi_tunning_size, NULL, NULL);
	checksum_base = (checksum_seed & 0xffffffff) / 4;
	if (checksum != buf32[checksum_base]) {
		err("Camera: CRC32 error at the binary section (0x%08X != 0x%08X)",
					checksum, buf32[checksum_base]);
		set_bit(FIMC_IS_CRC_ERROR_HIFI_TUNNING, &finfo->crc_error);
	} else {
		clear_bit(FIMC_IS_CRC_ERROR_HIFI_TUNNING, &finfo->crc_error);
	}

	info("Camera: End checking CRC32 hifi tunning file\n");

	return !test_bit(FIMC_IS_CRC_ERROR_HIFI_TUNNING, &finfo->crc_error);
}
#endif
#endif

void remove_dump_fw_file(void)
{
	mm_segment_t old_fs;
	int old_mask;
	long ret;
	char fw_path[100];
	struct fimc_is_rom_info *finfo = NULL;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	old_mask = sys_umask(0);

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	/* RTA binary */
	snprintf(fw_path, sizeof(fw_path), "%s%s", FIMC_IS_FW_DUMP_PATH, finfo->load_rta_fw_name);

	ret = sys_unlink(fw_path);
	info("sys_unlink (%s) %ld", fw_path, ret);

	/* DDK binary */
	snprintf(fw_path, sizeof(fw_path), "%s%s", FIMC_IS_FW_DUMP_PATH, finfo->load_fw_name);

	ret = sys_unlink(fw_path);
	info("sys_unlink (%s) %ld", fw_path, ret);

	sys_umask(old_mask);
	set_fs(old_fs);

	is_dumped_fw_loading_needed = false;
}

ssize_t write_data_to_file(char *name, char *buf, size_t count, loff_t *pos)
{
	struct file *fp;
	mm_segment_t old_fs;
	ssize_t tx = -ENOENT;
	int fd, old_mask;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	old_mask = sys_umask(0);

	if (force_caldata_dump) {
		sys_rmdir(name);
		fd = sys_open(name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0666);
	} else {
		fd = sys_open(name, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0664);
	}
	if (fd < 0) {
		err("open file error: %s", name);
		sys_umask(old_mask);
		set_fs(old_fs);
		return -EINVAL;
	}

	fp = fget(fd);
	if (fp) {
		tx = vfs_write(fp, buf, count, pos);
		if (tx != count) {
			err("fail to write %s. ret %zd", name, tx);
			tx = -ENOENT;
		}

		vfs_fsync(fp, 0);
		fput(fp);
	} else {
		err("fail to get file *: %s", name);
	}

	sys_close(fd);
	sys_umask(old_mask);
	set_fs(old_fs);

	return tx;
}

ssize_t read_data_rom_file(char *name, char *buf, size_t count, loff_t *pos)
{
	struct file *fp;
	mm_segment_t old_fs;
	ssize_t tx;
	int fd;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open(name, O_RDONLY, 0664);
	if (fd < 0) {
		if (-ENOENT == fd)
			info("%s: file(%s) not exist!\n", __func__, name);
		else
			info("%s: error %d, failed to open %s\n", __func__, fd, name);

		set_fs(old_fs);
		return -EINVAL;
	}
	fp = fget(fd);
	if (fp) {
		tx = vfs_read(fp, buf, count, pos);
		fput(fp);
	}
	sys_close(fd);
	set_fs(old_fs);

	return count;
}

bool fimc_is_sec_file_exist(char *name)
{
	mm_segment_t old_fs;
	bool exist = true;
	int ret;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	ret = sys_access(name, 0);
	if (ret) {
		exist = false;
		if (-ENOENT == ret)
			info("%s: file(%s) not exist!\n", __func__, name);
		else
			info("%s: error %d, failed to access %s\n", __func__, ret, name);
	}

	set_fs(old_fs);
	return exist;
}

void fimc_is_sec_make_crc32_table(u32 *table, u32 id)
{
	u32 i, j, k;

	for (i = 0; i < 256; ++i) {
		k = i;
		for (j = 0; j < 8; ++j) {
			if (k & 1)
				k = (k >> 1) ^ id;
			else
				k >>= 1;
		}
		table[i] = k;
	}
}

/**
 * fimc_is_sec_ldo_enabled: check whether the ldo has already been enabled.
 *
 * @ return: true, false or error value
 */
int fimc_is_sec_ldo_enabled(struct device *dev, char *name) {
	struct regulator *regulator = NULL;
	int enabled = 0;

	regulator = regulator_get(dev, name);
	if (IS_ERR_OR_NULL(regulator)) {
		err("%s : regulator_get(%s) fail", __func__, name);
		return -EINVAL;
	}

	enabled = regulator_is_enabled(regulator);

	regulator_put(regulator);

	return enabled;
}

int fimc_is_sec_ldo_enable(struct device *dev, char *name, bool on)
{
	struct regulator *regulator = NULL;
	int ret = 0;

	regulator = regulator_get(dev, name);
	if (IS_ERR_OR_NULL(regulator)) {
		err("%s : regulator_get(%s) fail", __func__, name);
		return -EINVAL;
	}

	if (on) {
		if (regulator_is_enabled(regulator)) {
			pr_warning("%s: regulator is already enabled\n", name);
			goto exit;
		}

		ret = regulator_enable(regulator);
		if (ret) {
			err("%s : regulator_enable(%s) fail", __func__, name);
			goto exit;
		}
	} else {
		if (!regulator_is_enabled(regulator)) {
			pr_warning("%s: regulator is already disabled\n", name);
			goto exit;
		}

		ret = regulator_disable(regulator);
		if (ret) {
			err("%s : regulator_disable(%s) fail", __func__, name);
			goto exit;
		}
	}

exit:
	regulator_put(regulator);

	return ret;
}

int fimc_is_sec_rom_power_on(struct fimc_is_core *core, int position)
{
	int ret = 0;
	struct exynos_platform_fimc_is_module *module_pdata;
	struct fimc_is_module_enum *module = NULL;
	int i = 0;

	info("%s: Sensor position = %d.", __func__, position);

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		fimc_is_search_sensor_module_with_position(&core->sensor[i], position, &module);
		if (module)
			break;
	}

	if (!module) {
		err("%s: Could not find sensor id.", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	module_pdata = module->pdata;

	if (!module_pdata->gpio_cfg) {
		err("gpio_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = module_pdata->gpio_cfg(module, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_ON);
	if (ret) {
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

int fimc_is_sec_rom_power_off(struct fimc_is_core *core, int position)
{
	int ret = 0;
	struct exynos_platform_fimc_is_module *module_pdata;
	struct fimc_is_module_enum *module = NULL;
	int i = 0;

	info("%s: Sensor position = %d.", __func__, position);

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		fimc_is_search_sensor_module_with_position(&core->sensor[i], position, &module);
		if (module)
			break;
	}

	if (!module) {
		err("%s: Could not find sensor id.", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	module_pdata = module->pdata;

	if (!module_pdata->gpio_cfg) {
		err("gpio_cfg is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	ret = module_pdata->gpio_cfg(module, SENSOR_SCENARIO_READ_ROM, GPIO_SCENARIO_OFF);
	if (ret) {
		err("gpio_cfg is fail(%d)", ret);
		goto p_err;
	}

p_err:
	return ret;
}

void fimc_is_sec_check_module_state(struct fimc_is_rom_info *finfo)
{
	struct fimc_is_core *core = dev_get_drvdata(fimc_is_dev);
	struct fimc_is_vender_specific *specific = core->vender.private_data;

	if (test_bit(FIMC_IS_ROM_STATE_SKIP_HEADER_LOADING, &finfo->rom_state)) {
		clear_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state);
		info("%s : skip header loading. return ", __func__);
		return;
	}

	if (finfo->header_ver[3] == 'L' || finfo->header_ver[3] == 'X') {
		clear_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state);
	} else {
		set_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state);
	}

	if (specific->use_module_check) {
		if (finfo->header_ver[10] >= finfo->camera_module_es_version) {
			set_bit(FIMC_IS_ROM_STATE_LATEST_MODULE, &finfo->rom_state);
		} else {
			clear_bit(FIMC_IS_ROM_STATE_LATEST_MODULE, &finfo->rom_state);
		}

		if (finfo->header_ver[10] == FIMC_IS_LATEST_ROM_VERSION_M) {
			set_bit(FIMC_IS_ROM_STATE_FINAL_MODULE, &finfo->rom_state);
		} else {
			clear_bit(FIMC_IS_ROM_STATE_FINAL_MODULE, &finfo->rom_state);
		}
	} else {
		set_bit(FIMC_IS_ROM_STATE_LATEST_MODULE, &finfo->rom_state);
		set_bit(FIMC_IS_ROM_STATE_FINAL_MODULE, &finfo->rom_state);
	}
}

int fimc_is_i2c_read(struct i2c_client *client, void *buf, u32 addr, size_t size)
{
	const u32 addr_size = 2, max_retry = 2;
	u8 addr_buf[addr_size];
	int retries = max_retry;
	int ret = 0;

	if (!client) {
		info("%s: client is null\n", __func__);
		return -ENODEV;
	}

	/* Send addr */
	addr_buf[0] = ((u16)addr) >> 8;
	addr_buf[1] = (u8)addr;

	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_send(client, addr_buf, addr_size);
		if (likely(addr_size == ret))
			break;

		info("%s: i2c_master_send failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(1000, 1000);
	}

	if (unlikely(ret <= 0)) {
		err("%s: error %d, fail to write 0x%04X", __func__, ret, addr);
		return ret ? ret : -ETIMEDOUT;
	}

	/* Receive data */
	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_recv(client, buf, size);
		if (likely(ret == size))
			break;

		info("%s: i2c_master_recv failed(%d), try %d\n", __func__,  ret, retries);
		usleep_range(1000, 1000);
	}

	if (unlikely(ret <= 0)) {
		err("%s: error %d, fail to read 0x%04X", __func__, ret, addr);
		return ret ? ret : -ETIMEDOUT;
	}

	return 0;
}

int fimc_is_i2c_write(struct i2c_client *client, u16 addr, u8 data)
{
	const u32 write_buf_size = 3, max_retry = 2;
	u8 write_buf[write_buf_size];
	int retries = max_retry;
	int ret = 0;

	if (!client) {
		pr_info("%s: client is null\n", __func__);
		return -ENODEV;
	}

	/* Send addr+data */
	write_buf[0] = ((u16)addr) >> 8;
	write_buf[1] = (u8)addr;
	write_buf[2] = data;


	for (retries = max_retry; retries > 0; retries--) {
		ret = i2c_master_send(client, write_buf, write_buf_size);
		if (likely(write_buf_size == ret))
			break;

		pr_info("%s: i2c_master_send failed(%d), try %d\n", __func__, ret, retries);
		usleep_range(1000, 1000);
	}

	if (unlikely(ret <= 0)) {
		pr_err("%s: error %d, fail to write 0x%04X\n", __func__, ret, addr);
		return ret ? ret : -ETIMEDOUT;
	}

	return 0;
}

int fimc_is_sec_check_reload(struct fimc_is_core *core)
{
	struct file *reload_key_fp = NULL;
	struct file *supend_resume_key_fp = NULL;
	mm_segment_t old_fs;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	char file_path[100];

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	memset(file_path, 0x00, sizeof(file_path));
	snprintf(file_path, sizeof(file_path), "%sreload/r1e2l3o4a5d.key", FIMC_IS_FW_DUMP_PATH);
	reload_key_fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(reload_key_fp)) {
		reload_key_fp = NULL;
	} else {
		info("Reload KEY exist, reload cal data.\n");
		force_caldata_dump = true;
		specific->suspend_resume_disable = true;
	}

	if (reload_key_fp)
		filp_close(reload_key_fp, current->files);

	memset(file_path, 0x00, sizeof(file_path));
	snprintf(file_path, sizeof(file_path), "%si1s2p3s4r.key", FIMC_IS_FW_DUMP_PATH);
	supend_resume_key_fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(supend_resume_key_fp)) {
		supend_resume_key_fp = NULL;
	} else {
		info("Supend_resume KEY exist, disable runtime supend/resume. \n");
		specific->suspend_resume_disable = true;
	}

	if (supend_resume_key_fp)
		filp_close(supend_resume_key_fp, current->files);

	set_fs(old_fs);

	return 0;
}

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
void fimc_is_sec_cal_dump(struct fimc_is_core *core)
{
	struct file *key_fp = NULL;
	struct file *dump_fp = NULL;
	mm_segment_t old_fs;
	char file_path[100];
	char *cal_buf;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	int i;

	loff_t pos = 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	memset(file_path, 0x00, sizeof(file_path));
	snprintf(file_path, sizeof(file_path), "%s1q2w3e4r.key", FIMC_IS_FW_DUMP_PATH);
	key_fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(key_fp)) {
		info("KEY does not exist.\n");
		key_fp = NULL;
		goto key_err;
	} else {
		memset(file_path, 0x00, sizeof(file_path));
		snprintf(file_path, sizeof(file_path), "%sdump", FIMC_IS_FW_DUMP_PATH);
		dump_fp = filp_open(file_path, O_RDONLY, 0);
		if (IS_ERR(dump_fp)) {
			info("dump folder does not exist.\n");
			dump_fp = NULL;
			goto key_err;
		} else {
			info("dump folder exist.\n");
			for (i = 0; i < ROM_ID_MAX; i++) {
				if (specific->rom_valid[i] == true) {
					fimc_is_sec_get_cal_buf(&cal_buf, i);
					fimc_is_sec_get_sysfs_finfo(&finfo, i);
					pos = 0;
					info("Dump ROM_ID(%d) cal data.\n", i);
					memset(file_path, 0x00, sizeof(file_path));
					snprintf(file_path, sizeof(file_path), "%sdump/rom_%d_cal.bin", FIMC_IS_FW_DUMP_PATH, i);
					if (write_data_to_file(file_path, cal_buf, finfo->rom_size, &pos) < 0) {
						info("Failed to dump rom_id(%d) cal data.\n", i);
						goto dump_err;
					}
				}
			}
		}
	}

dump_err:
	if (dump_fp)
		filp_close(dump_fp, current->files);
key_err:
	if (key_fp)
		filp_close(key_fp, current->files);
	set_fs(old_fs);
}
#endif

int fimc_is_sec_parse_rom_info(struct fimc_is_rom_info *finfo, char *buf, int rom_id)
{
#if defined(CONFIG_CAMERA_USE_MCU)
	struct fimc_is_ois_info *ois_pinfo = NULL;
#endif

	if (finfo->rom_header_version_start_addr != -1) {
		memcpy(finfo->header_ver, &buf[finfo->rom_header_version_start_addr], FIMC_IS_HEADER_VER_SIZE);
		finfo->header_ver[FIMC_IS_HEADER_VER_SIZE] = '\0';
	}

	if (finfo->rom_header_sensor2_version_start_addr != -1) {
		memcpy(finfo->header2_ver, &buf[finfo->rom_header_sensor2_version_start_addr], FIMC_IS_HEADER_VER_SIZE);
		finfo->header2_ver[FIMC_IS_HEADER_VER_SIZE] = '\0';
	}

	if (finfo->rom_header_project_name_start_addr != -1) {
		memcpy(finfo->project_name, &buf[finfo->rom_header_project_name_start_addr], FIMC_IS_PROJECT_NAME_SIZE);
		finfo->project_name[FIMC_IS_PROJECT_NAME_SIZE] = '\0';
	}

	if (finfo->rom_header_module_id_addr != -1) {
		memcpy(finfo->rom_module_id, &buf[finfo->rom_header_module_id_addr], FIMC_IS_MODULE_ID_SIZE);
		finfo->rom_module_id[FIMC_IS_MODULE_ID_SIZE] = '\0';
	}

	if (finfo->rom_header_sensor_id_addr != -1) {
		memcpy(finfo->rom_sensor_id, &buf[finfo->rom_header_sensor_id_addr], FIMC_IS_SENSOR_ID_SIZE);
		finfo->rom_sensor_id[FIMC_IS_SENSOR_ID_SIZE] = '\0';
	}

	if (finfo->rom_header_sensor2_id_addr != -1) {
		memcpy(finfo->rom_sensor2_id, &buf[finfo->rom_header_sensor2_id_addr], FIMC_IS_SENSOR_ID_SIZE);
		finfo->rom_sensor2_id[FIMC_IS_SENSOR_ID_SIZE] = '\0';
	}

#if defined(CONFIG_CAMERA_USE_MCU)
	if (rom_id == ROM_ID_REAR && finfo->rom_ois_list_len == FIMC_IS_ROM_OIS_MAX_LIST) {
		fimc_is_sec_get_ois_pinfo(&ois_pinfo);
		memcpy(ois_pinfo->wide_xgg, &buf[finfo->rom_ois_list[0]], FIMC_IS_OIS_GYRO_DATA_SIZE);
		memcpy(ois_pinfo->wide_ygg, &buf[finfo->rom_ois_list[1]], FIMC_IS_OIS_GYRO_DATA_SIZE);
		memcpy(ois_pinfo->wide_xcoef, &buf[finfo->rom_ois_list[2]], FIMC_IS_OIS_COEF_DATA_SIZE);
		memcpy(ois_pinfo->wide_ycoef, &buf[finfo->rom_ois_list[3]], FIMC_IS_OIS_COEF_DATA_SIZE);
		memcpy(ois_pinfo->wide_supperssion_xratio, &buf[finfo->rom_ois_list[4]], FIMC_IS_OIS_SUPPERSSION_RATIO_DATA_SIZE);
		memcpy(ois_pinfo->wide_supperssion_yratio, &buf[finfo->rom_ois_list[5]], FIMC_IS_OIS_SUPPERSSION_RATIO_DATA_SIZE);
		memcpy(ois_pinfo->wide_cal_mark, &buf[finfo->rom_ois_list[6]], FIMC_IS_OIS_CAL_MARK_DATA_SIZE);
		memcpy(ois_pinfo->tele_xgg, &buf[finfo->rom_ois_list[7]], FIMC_IS_OIS_GYRO_DATA_SIZE);
		memcpy(ois_pinfo->tele_ygg, &buf[finfo->rom_ois_list[8]], FIMC_IS_OIS_GYRO_DATA_SIZE);
		memcpy(ois_pinfo->tele_xcoef, &buf[finfo->rom_ois_list[9]], FIMC_IS_OIS_COEF_DATA_SIZE);
		memcpy(ois_pinfo->tele_ycoef, &buf[finfo->rom_ois_list[10]], FIMC_IS_OIS_COEF_DATA_SIZE);
		memcpy(ois_pinfo->tele_supperssion_xratio, &buf[finfo->rom_ois_list[11]], FIMC_IS_OIS_SUPPERSSION_RATIO_DATA_SIZE);
		memcpy(ois_pinfo->tele_supperssion_yratio, &buf[finfo->rom_ois_list[12]], FIMC_IS_OIS_SUPPERSSION_RATIO_DATA_SIZE);
		memcpy(ois_pinfo->tele_cal_mark, &buf[finfo->rom_ois_list[13]], FIMC_IS_OIS_CAL_MARK_DATA_SIZE);
	}
#endif

#if 1
	/* debug info dump */
	info("++++ ROM data info - rom_id:%d\n", rom_id);
	info("1. Header info\n");
	info("Module info : %s\n", finfo->header_ver);
	info(" ID : %c\n", finfo->header_ver[FW_CORE_VER]);
	info(" Pixel num : %c%c\n", finfo->header_ver[FW_PIXEL_SIZE],
		finfo->header_ver[FW_PIXEL_SIZE + 1]);
	info(" ISP ID : %c\n", finfo->header_ver[FW_ISP_COMPANY]);
	info(" Sensor Maker : %c\n", finfo->header_ver[FW_SENSOR_MAKER]);
	info(" Year : %c\n", finfo->header_ver[FW_PUB_YEAR]);
	info(" Month : %c\n", finfo->header_ver[FW_PUB_MON]);
	info(" Release num : %c%c\n", finfo->header_ver[FW_PUB_NUM],
		finfo->header_ver[FW_PUB_NUM + 1]);
	info(" Manufacturer ID : %c\n", finfo->header_ver[FW_MODULE_COMPANY]);
	info(" Module ver : %c\n", finfo->header_ver[FW_VERSION_INFO]);
	info(" Project name : %s\n", finfo->project_name);
	info(" Cal data map ver : %s\n", finfo->cal_map_ver);
	info(" MODULE ID : %c%c%c%c%c%02X%02X%02X%02X%02X\n",
		finfo->rom_module_id[0], finfo->rom_module_id[1], finfo->rom_module_id[2],
		finfo->rom_module_id[3], finfo->rom_module_id[4], finfo->rom_module_id[5],
		finfo->rom_module_id[6], finfo->rom_module_id[7], finfo->rom_module_id[8],
		finfo->rom_module_id[9]);
	info("---- ROM data info\n");
#endif
	return 0;
}

int fimc_is_sec_read_eeprom_header(struct device *dev, int rom_id)
{
	int ret = 0;
	struct fimc_is_core *core = dev_get_drvdata(fimc_is_dev);
	struct fimc_is_vender_specific *specific;
	u8 header_version[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };
	u8 header2_version[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };
	struct i2c_client *client;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_device_eeprom *eeprom;

	specific = core->vender.private_data;
	client = specific->eeprom_client[rom_id];

	eeprom = i2c_get_clientdata(client);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	if (finfo->rom_header_version_start_addr != -1) {
		ret = fimc_is_i2c_read(client, header_version,
					finfo->rom_header_version_start_addr,
					FIMC_IS_HEADER_VER_SIZE);

		if (unlikely(ret)) {
			err("failed to fimc_is_i2c_read for header version (%d)\n", ret);
			ret = -EINVAL;
		}

		memcpy(finfo->header_ver, header_version, FIMC_IS_HEADER_VER_SIZE);
		finfo->header_ver[FIMC_IS_HEADER_VER_SIZE] = '\0';
	}

	if (finfo->rom_header_sensor2_version_start_addr != -1) {
		ret = fimc_is_i2c_read(client, header2_version,
					finfo->rom_header_sensor2_version_start_addr,
					FIMC_IS_HEADER_VER_SIZE);

		if (unlikely(ret)) {
			err("failed to fimc_is_i2c_read for header version (%d)\n", ret);
			ret = -EINVAL;
		}

		memcpy(finfo->header2_ver, header2_version, FIMC_IS_HEADER_VER_SIZE);
		finfo->header2_ver[FIMC_IS_HEADER_VER_SIZE] = '\0';
	}

	return ret;
}

int fimc_is_sec_readcal_eeprom(struct device *dev, int rom_id)
{
	int ret = 0;
	int count = 0;
	char *buf = NULL;
	int retry = FIMC_IS_CAL_RETRY_CNT;
	struct fimc_is_core *core = dev_get_drvdata(fimc_is_dev);
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	int cal_size = 0;
	u32 read_addr = 0x0;
	struct i2c_client *client = NULL;
#ifdef DEBUG_FORCE_DUMP_ENABLE
	loff_t pos = 0;
#endif
	struct fimc_is_device_eeprom *eeprom;

	info("Camera: read cal data from EEPROM (rom_id:%d)\n", rom_id);

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&buf, rom_id);
	client = specific->eeprom_client[rom_id];

	eeprom = i2c_get_clientdata(client);

	cal_size = finfo->rom_size;
	info("%s: rom_id : %d, cal_size :%d\n", __func__, rom_id, cal_size);

	if (finfo->rom_header_cal_map_ver_start_addr != -1) {
		ret = fimc_is_i2c_read(client, finfo->cal_map_ver,
					finfo->rom_header_cal_map_ver_start_addr,
					FIMC_IS_CAL_MAP_VER_SIZE);

		if (unlikely(ret)) {
			err("failed to fimc_is_i2c_read (%d)\n", ret);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (finfo->rom_header_version_start_addr != -1) {
		ret = fimc_is_i2c_read(client, finfo->header_ver,
					finfo->rom_header_version_start_addr,
					FIMC_IS_HEADER_VER_SIZE);

		if (unlikely(ret)) {
			err("failed to fimc_is_i2c_read (%d)\n", ret);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (unlikely(ret)) {
		err("failed to fimc_is_i2c_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	info("Camera : EEPROM Cal map_version = %s(%x%x%x%x)\n", finfo->cal_map_ver, finfo->cal_map_ver[0],
			finfo->cal_map_ver[1], finfo->cal_map_ver[2], finfo->cal_map_ver[3]);
	info("EEPROM header version = %s(%x%x%x%x)\n", finfo->header_ver,
		finfo->header_ver[0], finfo->header_ver[1], finfo->header_ver[2], finfo->header_ver[3]);

	if (!fimc_is_sec_check_rom_ver(core, rom_id)) {
		info("Camera: Do not read eeprom cal data. EEPROM version is low.\n");
		return 0;
	}

crc_retry:

	/* read cal data */
	info("Camera: I2C read cal data\n");
	read_addr = 0x0;
	if (cal_size > FIMC_IS_READ_MAX_EEP_CAL_SIZE) {
		for (count = 0; count < cal_size/FIMC_IS_READ_MAX_EEP_CAL_SIZE; count++) {
			ret = fimc_is_i2c_read(client, &buf[read_addr], read_addr, FIMC_IS_READ_MAX_EEP_CAL_SIZE);
			if (ret) {
				err("failed to fimc_is_i2c_read (%d)\n", ret);
				ret = -EINVAL;
				goto exit;
			}
			read_addr += FIMC_IS_READ_MAX_EEP_CAL_SIZE;
		}

		if (read_addr < cal_size) {
			ret = fimc_is_i2c_read(client, &buf[read_addr], read_addr, cal_size - read_addr);
		}
	} else {
		ret = fimc_is_i2c_read(client, buf, read_addr, cal_size);
		if (ret) {
			err("failed to fimc_is_i2c_read (%d)\n", ret);
			ret = -EINVAL;
			goto exit;
		}
	}

	fimc_is_sec_parse_rom_info(finfo, buf, rom_id);

#ifdef CAMERA_REAR_TOF
	if (rom_id == REAR_TOF_ROM_ID) {
#ifdef REAR_TOF_CHECK_SENSOR_ID
		fimc_is_sec_sensorid_find_rear_tof(core);
		if (specific->rear_tof_sensor_id == SENSOR_NAME_IMX316) {
			finfo->rom_tof_cal_uid_addr_len = 1;
			finfo->rom_tof_cal_size_addr_len = 1;
			if (finfo->cal_map_ver[3] == '1') {
				finfo->crc_check_list[1] = REAR_TOF_IMX316_CRC_ADDR1_MAP001;
			} else {
				finfo->crc_check_list[1] = REAR_TOF_IMX316_CRC_ADDR1_MAP002;
			}
		}
#endif
		fimc_is_sec_sensor_find_rear_tof_uid(core, buf);
	}
#endif

#ifdef CAMERA_FRONT_TOF
	if (rom_id == FRONT_TOF_ROM_ID) {
		fimc_is_sec_sensor_find_front_tof_uid(core, buf);
	}
#endif

#ifdef DEBUG_FORCE_DUMP_ENABLE
	{
		char file_path[100];

		loff_t pos = 0;

		memset(file_path, 0x00, sizeof(file_path));
		snprintf(file_path, sizeof(file_path), "%srom%d_dump.bin", FIMC_IS_FW_DUMP_PATH, rom_id);

		if (write_data_to_file(file_path, buf, cal_size, &pos) < 0) {
			info("Failed to dump cal data. rom_id:%d\n", rom_id);
		}
	}
#endif

	/* CRC check */
	if (!fimc_is_sec_check_cal_crc32(buf, rom_id) && (retry > 0)) {
		retry--;
		goto crc_retry;
	}

	fimc_is_sec_check_module_state(finfo);

exit:
	return ret;
}

#if defined(CONFIG_CAMERA_OTPROM_SUPPORT_REAR) || defined(CONFIG_CAMERA_OTPROM_SUPPORT_FRONT)
int fimc_is_sec_readcal_otprom(struct device *dev, int rom_id)
{
	int ret = 0;
	char *buf = NULL;
	int retry = FIMC_IS_CAL_RETRY_CNT;
	struct fimc_is_core *core = dev_get_drvdata(dev);
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	int cal_size = 0;
	struct i2c_client *client = NULL;
	struct file *key_fp = NULL;
	struct file *dump_fp = NULL;
	mm_segment_t old_fs;
	char file_path[100];
	char selected_page[2] = {0,};
	loff_t pos = 0;

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);
	fimc_is_sec_get_cal_buf(&buf, rom_id);
	client = specific->eeprom_client[rom_id];

	cal_size = finfo->rom_size;

	msleep(10);

	ret = fimc_is_i2c_write(client, 0xA00, 0x04);
	if (unlikely(ret)) {
		err("failed to fimc_is_i2c_write (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	fimc_is_i2c_write(client, 0xA02, 0x02);
	fimc_is_i2c_write(client, 0xA00, 0x01);

	ret = fimc_is_i2c_read(client, selected_page, 0xA12, 0x1);
	if (unlikely(ret)) {
		err("failed to fimc_is_i2c_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	printk(KERN_INFO "Camera: otp_bank = %d\n", selected_page[0]);
	if (selected_page[0] == 0x3) {
		printk(KERN_INFO "Camera: OTP 3 page selected\n");
		fimc_is_i2c_write(client, 0xA00, 0x04);
		fimc_is_i2c_write(client, 0xA00, 0x00);

		msleep(1);

		fimc_is_i2c_write(client, 0xA00, 0x04);
		fimc_is_i2c_write(client, 0xA02, 0x03);
		fimc_is_i2c_write(client, 0xA00, 0x01);
	}
	fimc_is_i2c_read(client, cal_map_version, 0xA22, 0x4);

	fimc_is_i2c_write(client, 0xA00, 0x04);
	fimc_is_i2c_write(client, 0xA00, 0x00);

	if(finfo->cal_map_ver[0] != 'V') {
		printk(KERN_INFO "Camera: Cal Map version read fail or there's no available data.\n");
		set_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error);
		goto exit;
	}

	printk(KERN_INFO "Camera: OTPROM Cal map_version = %c%c%c%c\n", finfo->cal_map_ver[0],
			finfo->cal_map_ver[1], finfo->cal_map_ver[2], finfo->cal_map_ver[3]);

crc_retry:
	cal_size = 50;
	fimc_is_i2c_write(client, 0xA00, 0x04);
	if(selected_page[0] == 1)
		fimc_is_i2c_write(client, 0xA02, 0x02);
	else
		fimc_is_i2c_write(client, 0xA02, 0x03);
	fimc_is_i2c_write(client, 0xA00, 0x01);

	/* read cal data */
	pr_info("Camera: I2C read cal data\n\n");
	fimc_is_i2c_read(client, buf, 0xA15, cal_size);

	fimc_is_i2c_write(client, 0xA00, 0x04);
	fimc_is_i2c_write(client, 0xA00, 0x00);

	fimc_is_sec_parse_rom_info(finfo, buf, rom_id);

	/* CRC check */
	ret = fimc_is_sec_check_cal_crc32(buf, rom_id);

	if (!ret && (retry > 0)) {
		retry--;
		goto crc_retry;
	}

	fimc_is_sec_check_module_state(finfo);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	memset(file_path, 0x00, sizeof(file_path));
	snprintf(file_path, sizeof(file_path), "%s1q2w3e4r.key", FIMC_IS_FW_DUMP_PATH);
	key_fp = filp_open(file_path, O_RDONLY, 0);
	if (IS_ERR(key_fp)) {
		pr_info("KEY does not exist.\n");
		key_fp = NULL;
		goto key_err;
	} else {
		memset(file_path, 0x00, sizeof(file_path));
		snprintf(file_path, sizeof(file_path), "%sdump", FIMC_IS_FW_DUMP_PATH);
		dump_fp = filp_open(file_path, O_RDONLY, 0);
		if (IS_ERR(dump_fp)) {
			pr_info("dump folder does not exist.\n");
			dump_fp = NULL;
			goto key_err;
		} else {
			pr_info("dump folder exist, Dump OTPROM cal data.\n");
			if (rom_id == ROM_ID_FRONT) {
				if (write_data_to_file(FIMC_IS_CAL_DUMP_FRONT, buf, FIMC_IS_DUMP_CAL_SIZE, &pos) < 0) {
					pr_info("Failed to dump cal data.\n");
					goto dump_err;
				}
			} else {
				if (write_data_to_file(FIMC_IS_CAL_DUMP, buf, FIMC_IS_DUMP_CAL_SIZE, &pos) < 0) {
					pr_info("Failed to dump cal data.\n");
					goto dump_err;
				}
			}
		}
	}

dump_err:
	if (dump_fp)
		filp_close(dump_fp, current->files);
key_err:
	if (key_fp)
		filp_close(key_fp, current->files);
	set_fs(old_fs);
exit:
	return ret;
}
#endif

#if defined(CONFIG_CAMERA_FROM)
int fimc_is_sec_read_from_header(struct device *dev)
{
	int ret = 0;
	struct fimc_is_core *core = dev_get_drvdata(fimc_is_dev);
	u8 header_version[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	ret = fimc_is_spi_read(&core->spi0, header_version,
		finfo->rom_header_version_start_addr, FIMC_IS_HEADER_VER_SIZE);
	if (ret < 0) {
		printk(KERN_ERR "failed to fimc_is_spi_read for header version (%d)\n", ret);
		ret = -EINVAL;
	}

	memcpy(finfo->header_ver, header_version, FIMC_IS_HEADER_VER_SIZE);
	finfo->header_ver[FIMC_IS_HEADER_VER_SIZE] = '\0';

	return ret;
}

int fimc_is_sec_check_status(struct fimc_is_core *core)
{
	int retry_read = 50;
	u8 temp[5] = {0x0, };
	int ret = 0;

	do {
		memset(temp, 0x0, sizeof(temp));
		fimc_is_spi_read_status_bit(&core->spi0, &temp[0]);
		if (retry_read < 0) {
			ret = -EINVAL;
			err("check status failed.");
			break;
		}
		retry_read--;
		msleep(3);
	} while (temp[0]);

	return ret;
}

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
int fimc_is_sec_read_fw_from_sdcard(char *name, unsigned long *size)
{
	struct file *fw_fp = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	char data_path[100];
	int ret = 0;
	unsigned long fsize;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(data_path, sizeof(data_path), "%s", name);
	memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	fw_fp = filp_open(data_path, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fw_fp)) {
		info("%s does not exist.\n", data_path);
		fw_fp = NULL;
		ret = -EIO;
		goto fw_err;
	} else {
		info("%s exist, Dump from sdcard.\n", name);
		fsize = fw_fp->f_path.dentry->d_inode->i_size;
		if (FIMC_IS_MAX_FW_BUFFER_SIZE >= fsize) {
			read_data_rom_file(name, fw_buf, fsize, &pos);
			*size = fsize;
		} else {
			err("FW size is larger than FW buffer.\n");
			BUG();
		}
	}

fw_err:
	if (fw_fp)
		filp_close(fw_fp, current->files);
	set_fs(old_fs);

	return ret;
}

u32 fimc_is_sec_get_fw_crc32(char *buf, size_t size)
{
	u32 *buf32 = NULL;
	u32 checksum;

	buf32 = (u32 *)buf;
	checksum = (u32)getCRC((u16 *)&buf32[0], size, NULL, NULL);

	return checksum;
}

int fimc_is_sec_change_from_header(struct fimc_is_core *core)
{
	int ret = 0;
	u8 crc_value[4];
	u32 crc_result = 0;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	/* read header data */
	info("Camera: Start SPI read header data\n");
	memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	ret = fimc_is_spi_read(&core->spi0, fw_buf, 0x0, HEADER_CRC32_LEN);
	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	fw_buf[0x7] = (finfo->bin_end_addr & 0xFF000000) >> 24;
	fw_buf[0x6] = (finfo->bin_end_addr & 0xFF0000) >> 16;
	fw_buf[0x5] = (finfo->bin_end_addr & 0xFF00) >> 8;
	fw_buf[0x4] = (finfo->bin_end_addr & 0xFF);
	fw_buf[0x27] = (finfo->setfile_end_addr & 0xFF000000) >> 24;
	fw_buf[0x26] = (finfo->setfile_end_addr & 0xFF0000) >> 16;
	fw_buf[0x25] = (finfo->setfile_end_addr & 0xFF00) >> 8;
	fw_buf[0x24] = (finfo->setfile_end_addr & 0xFF);
	fw_buf[0x37] = (finfo->concord_bin_end_addr & 0xFF000000) >> 24;
	fw_buf[0x36] = (finfo->concord_bin_end_addr & 0xFF0000) >> 16;
	fw_buf[0x35] = (finfo->concord_bin_end_addr & 0xFF00) >> 8;
	fw_buf[0x34] = (finfo->concord_bin_end_addr & 0xFF);

	strncpy(&fw_buf[0x40], finfo->header_ver, 9);
	strncpy(&fw_buf[0x50], finfo->concord_header_ver, FIMC_IS_HEADER_VER_SIZE);
	strncpy(&fw_buf[0x64], finfo->setfile_ver, FIMC_IS_ISP_SETFILE_VER_SIZE);

	fimc_is_spi_write_enable(&core->spi0);
	ret = fimc_is_spi_erase_sector(&core->spi0, 0x0);
	if (ret) {
		err("failed to fimc_is_spi_erase_sector (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	ret = fimc_is_sec_check_status(core);
	if (ret) {
		err("failed to fimc_is_sec_check_status (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	ret = fimc_is_spi_write_enable(&core->spi0);
	ret = fimc_is_spi_write(&core->spi0, 0x0, fw_buf, HEADER_CRC32_LEN);
	if (ret) {
		err("failed to fimc_is_spi_write (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	ret = fimc_is_sec_check_status(core);
	if (ret) {
		err("failed to fimc_is_sec_check_status (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	crc_result = fimc_is_sec_get_fw_crc32(fw_buf, HEADER_CRC32_LEN);
	crc_value[3] = (crc_result & 0xFF000000) >> 24;
	crc_value[2] = (crc_result & 0xFF0000) >> 16;
	crc_value[1] = (crc_result & 0xFF00) >> 8;
	crc_value[0] = (crc_result & 0xFF);

	ret = fimc_is_spi_write_enable(&core->spi0);
	ret = fimc_is_spi_write(&core->spi0, 0x0FFC, crc_value, 0x4);
	if (ret) {
		err("failed to fimc_is_spi_write (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	ret = fimc_is_sec_check_status(core);
	if (ret) {
		err("failed to fimc_is_sec_check_status (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	info("Camera: End SPI read header data\n");

exit:
	return ret;
}

int fimc_is_sec_write_fw_to_from(struct fimc_is_core *core, char *name, bool first_section)
{
	int ret = 0;
	unsigned long i = 0;
	unsigned long size = 0;
	u32 start_addr = 0, erase_addr = 0, end_addr = 0;
	u32 checksum_addr = 0, crc_result = 0, erase_end_addr = 0;
	u8 crc_value[4];
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	if (!strcmp(name, FIMC_IS_FW_FROM_SDCARD)) {
		ret = fimc_is_sec_read_fw_from_sdcard(FIMC_IS_FW_FROM_SDCARD, &size);
		start_addr = finfo->bin_start_addr;
		end_addr = (u32)size + start_addr - 1;
		finfo->bin_end_addr = end_addr;
		checksum_addr = 0x3FFFFF;
		finfo->fw_size = size;
		strncpy(finfo->header_ver, &fw_buf[size - FIMC_IS_HEADER_VER_OFFSET], 9);
	} else if (!strcmp(name, FIMC_IS_SETFILE_FROM_SDCARD)) {
		ret = fimc_is_sec_read_fw_from_sdcard(FIMC_IS_SETFILE_FROM_SDCARD, &size);
		start_addr = finfo->setfile_start_addr;
		end_addr = (u32)size + start_addr - 1;
		finfo->setfile_end_addr = end_addr;
		if (fimc_is_sec_fw_module_compare(finfo->header_ver, FW_2P2_B))
			checksum_addr = ROM_WRITE_CHECKSUM_SETF_LL;
		else
			checksum_addr = ROM_WRITE_CHECKSUM_SETF_LS;
		finfo->setfile_size = size;
		strncpy(finfo->setfile_ver, &fw_buf[size - 64], 6);
	} else if (!strcmp(name, FIMC_IS_COMPANION_FROM_SDCARD)) {
		ret = fimc_is_sec_read_fw_from_sdcard(FIMC_IS_COMPANION_FROM_SDCARD, &size);
		start_addr = finfo->concord_bin_start_addr;
		end_addr = (u32)size + start_addr - 1;
		finfo->concord_bin_end_addr = end_addr;
		if (fimc_is_sec_fw_module_compare(finfo->header_ver, FW_2P2_B))
			checksum_addr = ROM_WRITE_CHECKSUM_COMP_LL;
		else
			checksum_addr = ROM_WRITE_CHECKSUM_COMP_LS;
		erase_end_addr = 0x3FFFFF;
		finfo->comp_fw_size = size;
		strncpy(finfo->concord_header_ver, &fw_buf[size - 16], FIMC_IS_HEADER_VER_SIZE);
	} else {
		err("Not supported binary type.");
		return -EIO;
	}

	if (ret < 0) {
		err("FW is not exist in sdcard.");
		return -EIO;
	}

	info("Start %s write to FROM.\n", name);

	if (first_section) {
		for (erase_addr = start_addr; erase_addr < erase_end_addr; erase_addr += FIMC_IS_FROM_ERASE_SIZE) {
			ret = fimc_is_spi_write_enable(&core->spi0);
			ret |= fimc_is_spi_erase_block(&core->spi0, erase_addr);
			if (ret) {
				err("failed to fimc_is_spi_erase_block (%d)\n", ret);
				ret = -EINVAL;
				goto exit;
			}
			ret = fimc_is_sec_check_status(core);
			if (ret) {
				err("failed to fimc_is_sec_check_status (%d)\n", ret);
				ret = -EINVAL;
				goto exit;
			}
		}
	}

	for (i = 0; i < size; i += 256) {
		ret = fimc_is_spi_write_enable(&core->spi0);
		if (size - i >= 256) {
			ret = fimc_is_spi_write(&core->spi0, start_addr + i, fw_buf + i, 256);
			if (ret) {
				err("failed to fimc_is_spi_write (%d)\n", ret);
				ret = -EINVAL;
				goto exit;
			}
		} else {
			ret = fimc_is_spi_write(&core->spi0, start_addr + i, fw_buf + i, size - i);
			if (ret) {
				err("failed to fimc_is_spi_write (%d)\n", ret);
				ret = -EINVAL;
				goto exit;
			}
		}
		ret = fimc_is_sec_check_status(core);
		if (ret) {
			err("failed to fimc_is_sec_check_status (%d)\n", ret);
			ret = -EINVAL;
			goto exit;
		}
	}

	crc_result = fimc_is_sec_get_fw_crc32(fw_buf, size);
	crc_value[3] = (crc_result & 0xFF000000) >> 24;
	crc_value[2] = (crc_result & 0xFF0000) >> 16;
	crc_value[1] = (crc_result & 0xFF00) >> 8;
	crc_value[0] = (crc_result & 0xFF);

	ret = fimc_is_spi_write_enable(&core->spi0);
	if (ret) {
		err("failed to fimc_is_spi_write_enable (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	ret = fimc_is_spi_write(&core->spi0, checksum_addr -4 + 1, crc_value, 0x4);
	if (ret) {
		err("failed to fimc_is_spi_write (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	ret = fimc_is_sec_check_status(core);
	if (ret) {
		err("failed to fimc_is_sec_check_status (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	info("End %s write to FROM.\n", name);

exit:
	return ret;
}

int fimc_is_sec_write_fw(struct fimc_is_core *core, struct device *dev)
{
	int ret = 0;
	struct file *key_fp = NULL;
	struct file *comp_fw_fp = NULL;
	struct file *setfile_fp = NULL;
	struct file *isp_fw_fp = NULL;
	mm_segment_t old_fs;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	bool camera_running;

	camera_running = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	key_fp = filp_open(FIMC_IS_KEY_FROM_SDCARD, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(key_fp)) {
		info("KEY does not exist.\n");
		key_fp = NULL;
		ret = -EIO;
		goto key_err;
	} else {
		comp_fw_fp = filp_open(FIMC_IS_COMPANION_FROM_SDCARD, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(comp_fw_fp)) {
			info("Companion FW does not exist.\n");
			comp_fw_fp = NULL;
			ret = -EIO;
			goto comp_fw_err;
		}

		setfile_fp = filp_open(FIMC_IS_SETFILE_FROM_SDCARD, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(setfile_fp)) {
			info("setfile does not exist.\n");
			setfile_fp = NULL;
			ret = -EIO;
			goto setfile_err;
		}

		isp_fw_fp = filp_open(FIMC_IS_FW_FROM_SDCARD, O_RDONLY, 0);
		if (IS_ERR_OR_NULL(isp_fw_fp)) {
			info("ISP FW does not exist.\n");
			isp_fw_fp = NULL;
			ret = -EIO;
			goto isp_fw_err;
		}
	}

	info("FW file exist, Write Firmware to FROM .\n");

	if (!fimc_is_sec_ldo_enabled(dev, "VDDIO_1.8V_CAM"))
		fimc_is_sec_rom_power_on(core, SENSOR_POSITION_REAR);

	ret = fimc_is_sec_write_fw_to_from(core, FIMC_IS_COMPANION_FROM_SDCARD, true);
	if (ret) {
		err("fimc_is_sec_write_fw_to_from failed.");
		ret = -EIO;
		goto err;
	}

	ret = fimc_is_sec_write_fw_to_from(core, FIMC_IS_SETFILE_FROM_SDCARD, false);
	if (ret) {
		err("fimc_is_sec_write_fw_to_from failed.");
		ret = -EIO;
		goto err;
	}

	ret = fimc_is_sec_write_fw_to_from(core, FIMC_IS_FW_FROM_SDCARD, false);
	if (ret) {
		err("fimc_is_sec_write_fw_to_from failed.");
		ret = -EIO;
		goto err;
	}

	/* Off to reset FROM operation. Without this routine, spi read does not work. */
	if (!camera_running)
		fimc_is_sec_rom_power_off(core, SENSOR_POSITION_REAR);

	if (!fimc_is_sec_ldo_enabled(dev, "VDDIO_1.8V_CAM"))
		fimc_is_sec_rom_power_on(core, SENSOR_POSITION_REAR);

	ret = fimc_is_sec_change_from_header(core);
	if (ret) {
		err("fimc_is_sec_change_from_header failed.");
		ret = -EIO;
		goto err;
	}

err:
	if (!camera_running)
		fimc_is_sec_rom_power_off(core, SENSOR_POSITION_REAR);

isp_fw_err:
	if (isp_fw_fp)
		filp_close(isp_fw_fp, current->files);

setfile_err:
	if (setfile_fp)
		filp_close(setfile_fp, current->files);

comp_fw_err:
	if (comp_fw_fp)
		filp_close(comp_fw_fp, current->files);

key_err:
	if (key_fp)
		filp_close(key_fp, current->files);
	set_fs(old_fs);

	return ret;
}
#endif

int fimc_is_sec_readcal(struct fimc_is_core *core)
{
	int ret = 0;
	int retry = FIMC_IS_CAL_RETRY_CNT;
	int module_retry = FIMC_IS_CAL_RETRY_CNT;
	u16 id = 0;
	int cal_map_ver = 0;

	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;
	char *cal_buf;

	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	/* reset spi */
	if (!core->spi0.device) {
		err("spi0 device is not available");
		goto exit;
	}

	ret = fimc_is_spi_reset(&core->spi0);
	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

module_id_retry:
	ret = fimc_is_spi_read_module_id(&core->spi0, &id,
		ROM_HEADER_MODULE_ID_START_ADDR, ROM_HEADER_MODULE_ID_SIZE);
	if (ret) {
		printk(KERN_ERR "fimc_is_spi_read_module_id (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	info("Camera: FROM Module ID = 0x%04x\n", id);
	if (id == 0) {
		err("FROM rear module id NULL %d\n", module_retry);
		if (module_retry > 0) {
			usleep_range(5000, 6000);
			module_retry--;
			goto module_id_retry;
		}
	}

	ret = fimc_is_spi_read(&core->spi0, finfo->cal_map_ver,
		finfo->rom_header_cal_map_ver_start_addr, FIMC_IS_CAL_MAP_VER_SIZE);
	if (ret) {
		printk(KERN_ERR "failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	info("Camera: FROM Cal map_version = %c%c%c%c\n", finfo->cal_map_ver[0],
		finfo->cal_map_ver[1], finfo->cal_map_ver[2], finfo->cal_map_ver[3]);

crc_retry:
	/* read cal data */
	info("Camera: SPI read cal data\n");
	ret = fimc_is_spi_read(&core->spi0, cal_buf, 0x0, finfo->rom_size);
	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}

	finfo->bin_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_BINARY_START_ADDR]);
	finfo->bin_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_BINARY_END_ADDR]);
	info("Binary start = 0x%08x, end = 0x%08x\n",
		(finfo->bin_start_addr), (finfo->bin_end_addr));

	memcpy(finfo->setfile_ver, &cal_buf[ROM_HEADER_ISP_SETFILE_VER_START_ADDR], FIMC_IS_ISP_SETFILE_VER_SIZE);
	finfo->setfile_ver[FIMC_IS_ISP_SETFILE_VER_SIZE] = '\0';

	fimc_is_sec_parse_rom_info(finfo, cal_buf, ROM_ID_REAR);

	finfo->fw_size = (unsigned long)(finfo->bin_end_addr - finfo->bin_start_addr + 1);
	info("fw_size = %ld\n", finfo->fw_size);
	finfo->rta_fw_size = (unsigned long)(finfo->rta_bin_end_addr - finfo->rta_bin_start_addr + 1);
	info("rta_fw_size = %ld\n", finfo->rta_fw_size);

#ifdef DEBUG_FORCE_DUMP_ENABLE
	{
		char file_path[100];

		loff_t pos = 0;

		memset(file_path, 0x00, sizeof(file_path));
		snprintf(file_path, sizeof(file_path), "%sfrom_cal.bin", FIMC_IS_FW_DUMP_PATH);
		if (write_data_to_file(file_path, cal_buf, FIMC_IS_DUMP_CAL_SIZE, &pos) < 0)
			info("Failed to dump from cal data.\n");
	}
#endif

	/* CRC check */
	if (fimc_is_sec_check_rom_ver(core, ROM_ID_REAR)) {
		if (!fimc_is_sec_check_cal_crc32(cal_buf, ROM_ID_REAR) && (retry > 0)) {
			retry--;
			goto crc_retry;
		}
	} else {
		set_bit(FIMC_IS_ROM_STATE_INVALID_ROM_VERSION, &finfo->rom_state);
		set_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error);
		set_bit(FIMC_IS_CRC_ERROR_DUAL_CAMERA, &finfo->crc_error);
	}

	fimc_is_sec_check_module_state(finfo);

exit:
	return ret;
}

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
void fimc_is_sec_get_rta_bin_addr(void)
{
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_front = NULL;
	char *cal_buf;

	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo_front, ROM_ID_FRONT);

	if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI) {
		finfo->rta_bin_start_addr = *((u32 *)&cal_buf[ROM_HEADER_RTA_BINARY_LY_START_ADDR]);
		finfo->rta_bin_end_addr = *((u32 *)&cal_buf[ROM_HEADER_RTA_BINARY_LY_END_ADDR]);
	} else if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SONY) {
		finfo->rta_bin_start_addr = *((u32 *)&cal_buf[ROM_HEADER_RTA_BINARY_LS_START_ADDR]);
		finfo->rta_bin_end_addr = *((u32 *)&cal_buf[ROM_HEADER_RTA_BINARY_LS_END_ADDR]);
	} else {
		finfo->rta_bin_start_addr = *((u32 *)&cal_buf[ROM_HEADER_RTA_BINARY_LY_START_ADDR]);
		finfo->rta_bin_end_addr = *((u32 *)&cal_buf[ROM_HEADER_RTA_BINARY_LY_END_ADDR]);
	}

	info("rta bin start = 0x%08x, end = 0x%08x\n",
		finfo->rta_bin_start_addr, finfo->rta_bin_end_addr);
	finfo->rta_fw_size = (unsigned long)(finfo->rta_bin_end_addr - finfo->rta_bin_start_addr + 1);
	info("rta fw size = %ld\n", finfo->rta_fw_size);
}

void fimc_is_sec_get_rear_setfile_addr(void)
{
	struct fimc_is_rom_info *finfo = NULL;
	char *cal_buf;

	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY) {
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
		finfo->setfile_start_addr = 0;
		finfo->setfile_end_addr = 0;
#else
		finfo->setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LL_START_ADDR]);
		finfo->setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LL_END_ADDR]);
#endif
	} else if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SONY ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SONY_LSI) {
		finfo->setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LS_START_ADDR]);
		finfo->setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LS_END_ADDR]);
	} else {
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
		finfo->setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LS_START_ADDR]);
		finfo->setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LS_END_ADDR]);
#else
		finfo->setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LL_START_ADDR]);
		finfo->setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE_LL_END_ADDR]);
#endif
	}
	if (finfo->setfile_end_addr < ROM_ISP_BINARY_SETFILE_START_ADDR
		|| finfo->setfile_end_addr > ROM_ISP_BINARY_SETFILE_END_ADDR) {
		err("setfile end_addr has error!!  0x%08x\n", finfo->setfile_end_addr);
		finfo->setfile_end_addr = 0x1fffff;
	}

	info("Setfile start = 0x%08x, end = 0x%08x\n",
		(finfo->setfile_start_addr), (finfo->setfile_end_addr));
	finfo->setfile_size = (unsigned long)(finfo->setfile_end_addr - finfo->setfile_start_addr + 1);
	info("setfile_size = %ld\n", finfo->setfile_size);
}

#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
void fimc_is_sec_get_rear2_setfile_addr(void)
{
	struct fimc_is_rom_info *finfo = NULL;
	char *cal_buf;

	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY) {
		finfo->setfile2_start_addr = 0;
		finfo->setfile2_end_addr = 0;
	} else {
		finfo->setfile2_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE2_LL_START_ADDR]);
		finfo->setfile2_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_SETFILE2_LL_END_ADDR]);
	}

	info("Setfile2 start = 0x%08x, end = 0x%08x\n",
		(finfo->setfile2_start_addr), (finfo->setfile2_end_addr));
	finfo->setfile2_size =
		(unsigned long)(finfo->setfile2_end_addr - finfo->setfile2_start_addr + 1);
	info("setfile2_size = %ld\n", finfo->setfile2_size);
}
#endif

void fimc_is_sec_get_front_setfile_addr(void)
{
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_front = NULL;
	char *cal_buf;

	fimc_is_sec_get_cal_buf(&cal_buf, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo_front, ROM_ID_FRONT);

	if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI) {
		finfo->front_setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_FRONT_SETFILE_LL_START_ADDR]);
		finfo->front_setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_FRONT_SETFILE_LL_END_ADDR]);
	} else if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SONY) {
		finfo->front_setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_FRONT_SETFILE_LS_START_ADDR]);
		finfo->front_setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_FRONT_SETFILE_LS_END_ADDR]);
	} else {
		finfo->front_setfile_start_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_FRONT_SETFILE_LL_START_ADDR]);
		finfo->front_setfile_end_addr = *((u32 *)&cal_buf[ROM_HEADER_ISP_FRONT_SETFILE_LL_END_ADDR]);
	}
	info("Front setfile start = 0x%08x, end = 0x%08x\n",
		(finfo->front_setfile_start_addr), (finfo->front_setfile_end_addr));
	finfo->front_setfile_size = (unsigned long)(finfo->front_setfile_end_addr - finfo->front_setfile_start_addr + 1);
	info("front setfile_size = %ld\n", finfo->front_setfile_size);
}

void fimc_is_sec_get_hifi_tunningfile_addr(void)
{
	struct fimc_is_rom_info *finfo = NULL;
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY) {
		finfo->hifi_tunning_start_addr = 0;
		finfo->hifi_tunning_end_addr = 0;
	} else {
	    finfo->hifi_tunning_start_addr = *((u32 *)&cal_buf[ROM_HEADER_HIFI_TUNNINGFILE_LS_START_ADDR]);
		finfo->hifi_tunning_end_addr = *((u32 *)&cal_buf[ROM_HEADER_HIFI_TUNNINGFILE_LS_END_ADDR]);
	}
	info("Hifi tunning file start = 0x%08x, end = 0x%08x\n",
		(finfo->hifi_tunning_start_addr), (finfo->hifi_tunning_end_addr));
	finfo->hifi_tunning_size = (unsigned long)(finfo->hifi_tunning_end_addr - finfo->hifi_tunning_start_addr + 1);
	info("Hifi tunning file_size = %ld\n", finfo->hifi_tunning_size);
}

#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
int fimc_is_sec_get_front_setf_name(struct fimc_is_core *core)
{
	int ret = 0;
	struct fimc_is_module_enum *module = NULL;
	int i = 0;
	struct fimc_is_rom_info *finfo = NULL;
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	for (i = 0; i < FIMC_IS_SENSOR_COUNT; i++) {
		fimc_is_search_sensor_module_with_position(&core->sensor[i], SENSOR_POSITION_FRONT, &module);
		if (module)
			break;
	}

	if (!module) {
		err("%s: Could not find sensor id.", __func__);
		ret = -EINVAL;
		goto p_err;
	}

	memcpy(finfo->load_front_setfile_name, module->setfile_name,
		sizeof(finfo->load_front_setfile_name));

p_err:
	return ret;
}
#endif

#ifdef USE_RTA_BINARY
int fimc_is_sec_readfw_rta(struct fimc_is_core *core)
{
	int ret = 0;
	loff_t pos = 0;
	char fw_path[100];
	int retry = FIMC_IS_FW_RETRY_CNT;
	u32 checksum_seed;
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	u8 *buf = NULL;
#endif
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_front = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo_front, ROM_ID_FRONT);

	info("RTA FW need to be dumped\n");

crc_retry:
	/* read fw data */
	if (FIMC_IS_MAX_FW_BUFFER_SIZE >= FIMC_IS_MAX_FW_SIZE) {
		info("Start SPI read rta fw data\n");
		memset(fw_buf, 0x0, FIMC_IS_MAX_FW_SIZE);
		ret = fimc_is_spi_read(&core->spi0, fw_buf, finfo->rta_bin_start_addr, FIMC_IS_MAX_RTA_FW_SIZE);
		if (ret) {
			err("failed to fimc_is_spi_read (%d)\n", ret);
			ret = -EINVAL;
			goto exit;
		}
		info("End SPI read rta fw data %x %ld\n", finfo->rta_bin_start_addr, finfo->rta_fw_size);

		if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI)
			checksum_seed = CHECKSUM_SEED_ISP_FW_RTA_LY;
		else
			checksum_seed = CHECKSUM_SEED_ISP_FW_RTA_LS;

		checksum_seed -= finfo->rta_bin_start_addr;

		/* CRC check */
		if (!fimc_is_sec_check_fw_crc32(fw_buf, checksum_seed, finfo->rta_fw_size) && (retry > 0)) {
			retry--;
			goto crc_retry;
		} else if (!retry) {
			ret = -EINVAL;
			err("RTA FW Data has dumped fail.. CRC ERROR ! \n");
			goto exit;
		}

		snprintf(fw_path, sizeof(fw_path), "%s%s", FIMC_IS_FW_DUMP_PATH, finfo->load_rta_fw_name);
		pos = 0;

#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
		buf = (u8 *)fw_buf;
		fimc_is_sec_inflate_fw(&buf, &finfo->rta_fw_size);
#endif
		if (write_data_to_file(fw_path, fw_buf, finfo->rta_fw_size, &pos) < 0) {
			ret = -EIO;
			goto exit;
		}

		info("RTA FW Data has dumped successfully\n");
	} else {
		err("FW size is larger than FW buffer.\n");
		BUG();
	}

exit:
	return ret;
}
#endif

int fimc_is_sec_readfw(struct fimc_is_core *core)
{
	int ret = 0;
	loff_t pos = 0;
	char fw_path[100];
	int retry = FIMC_IS_FW_RETRY_CNT;
	u32 checksum_seed;
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	u8 *buf = NULL;
#endif
	struct fimc_is_rom_info *finfo = NULL;
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	info("FW need to be dumped\n");

crc_retry:
	/* read fw data */
	if (FIMC_IS_MAX_FW_BUFFER_SIZE >= FIMC_IS_MAX_FW_SIZE) {
		info("Start SPI read fw data\n");
		memset(fw_buf, 0x0, FIMC_IS_MAX_FW_SIZE);
		ret = fimc_is_spi_read(&core->spi0, fw_buf, finfo->bin_start_addr, FIMC_IS_MAX_FW_SIZE);
		if (ret) {
			err("failed to fimc_is_spi_read (%d)\n", ret);
			ret = -EINVAL;
			goto exit;
		}
		info("End SPI read fw data \n");

		checksum_seed = CHECKSUM_SEED_ISP_FW - finfo->bin_start_addr;

		/* CRC check */
		if (!fimc_is_sec_check_fw_crc32(fw_buf, checksum_seed, finfo->fw_size) && (retry > 0)) {
			retry--;
			goto crc_retry;
		} else if (!retry) {
			ret = -EINVAL;
			err("FW Data has dumped fail.. CRC ERROR ! \n");
			goto exit;
		}

		snprintf(fw_path, sizeof(fw_path), "%s%s", FIMC_IS_FW_DUMP_PATH, finfo->load_fw_name);
		pos = 0;

#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
		buf = (u8 *)fw_buf;
		fimc_is_sec_inflate_fw(&buf, &finfo->fw_size);
#endif
		if (write_data_to_file(fw_path, fw_buf, finfo->fw_size, &pos) < 0) {
			ret = -EIO;
			goto exit;
		}

		info("FW Data has dumped successfully\n");
	} else {
		err("FW size is larger than FW buffer.\n");
		BUG();
	}

#ifdef USE_RTA_BINARY
	ret = fimc_is_sec_readfw_rta(core);
#endif

exit:
	return ret;
}

int fimc_is_sec_read_setfile(struct fimc_is_core *core)
{
	int ret = 0;
	loff_t pos = 0;
	char setfile_path[100];
	int retry = FIMC_IS_FW_RETRY_CNT;
	u32 read_size = 0;
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	u8 *buf = NULL;
#endif
	struct fimc_is_rom_info *finfo = NULL;
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	info("Setfile need to be dumped\n");

setfile_crc_retry:
	/* read setfile data */
	info("Start SPI read setfile data\n");
	memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY)
		read_size = FIMC_IS_MAX_SETFILE_SIZE_LL;
	else
		read_size = FIMC_IS_MAX_SETFILE_SIZE_LS;

	ret = fimc_is_spi_read(&core->spi0, fw_buf, finfo->setfile_start_addr,
			read_size);
	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	info("End SPI read setfile data %x %ld\n", finfo->setfile_start_addr, finfo->setfile_size);

	/* CRC check */
	if (!fimc_is_sec_check_setfile_crc32(fw_buf) && (retry > 0)) {
		retry--;
		goto setfile_crc_retry;
	} else if (!retry) {
		err("Setfile has dumped fail.. CRC ERROR ! \n");
		ret = -EINVAL;
		goto exit;
	}

	snprintf(setfile_path, sizeof(setfile_path), "%s%s", FIMC_IS_FW_DUMP_PATH, finfo->load_setfile_name);
	pos = 0;

#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	buf = (u8 *)fw_buf;
	fimc_is_sec_inflate_fw(&buf, &finfo->setfile_size);
#endif
	if (write_data_to_file(setfile_path, fw_buf, finfo->setfile_size, &pos) < 0) {
		ret = -EIO;
		goto exit;
	}

	info("Setfile has dumped successfully\n");

exit:
	return ret;
}
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
int fimc_is_sec_read_setfile2(struct fimc_is_core *core)
{
	int ret = 0;
	loff_t pos = 0;
	char setfile_path[100];
	int retry = FIMC_IS_FW_RETRY_CNT;
	u32 read_size = 0;
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	u8 *buf = NULL;
#endif
	struct fimc_is_rom_info *finfo = NULL;
	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	info("Setfile2 need to be dumped\n");

setfile_crc_retry:
	/* read setfile data */
	info("Start SPI read setfile2 data\n");
	memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	if (finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
		finfo->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY)
		read_size = FIMC_IS_MAX_SETFILE_SIZE_LL;
	else
		read_size = FIMC_IS_MAX_SETFILE_SIZE_LS;

	ret = fimc_is_spi_read(&core->spi0, fw_buf, finfo->setfile2_start_addr,
			read_size);
	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	info("End SPI read setfile2 data %x %ld\n", finfo->setfile2_start_addr, finfo->setfile2_size);

	/* CRC check */
	if (!fimc_is_sec_check_setfile2_crc32(fw_buf) && (retry > 0)) {
		retry--;
		goto setfile_crc_retry;
	} else if (!retry) {
		err("Setfile2 has dumped fail.. CRC ERROR !\n");
		ret = -EINVAL;
		goto exit;
	}

	snprintf(setfile_path, sizeof(setfile_path), "%s%s", FIMC_IS_FW_DUMP_PATH, finfo->load_setfile2_name);
	pos = 0;

#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	buf = (u8 *)fw_buf;
	fimc_is_sec_inflate_fw(&buf, &finfo->setfile2_size);
#endif
	if (write_data_to_file(setfile_path, fw_buf, finfo->setfile2_size, &pos) < 0) {
		ret = -EIO;
		goto exit;
	}

	info("Setfile2 has dumped successfully\n");

exit:
	return ret;
}
#endif

#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
int fimc_is_sec_read_front_setfile(struct fimc_is_core *core)
{
	int ret = 0;
	loff_t pos = 0;
	char setfile_path[100];
	int retry = FIMC_IS_FW_RETRY_CNT;
	u32 read_size = 0;
#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	u8 *buf = NULL;
#endif
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_front = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo_front, ROM_ID_FRONT);

	info("Front setfile need to be dumped\n");

setfile_crc_retry:
	/* read setfile data */
	info("Start SPI read front setfile data\n");
	memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	if (finfo_front->header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI)
		read_size = FIMC_IS_MAX_SETFILE_SIZE_FRONT_LL;
	else
		read_size = FIMC_IS_MAX_SETFILE_SIZE_FRONT_LS;

	ret = fimc_is_spi_read(&core->spi0, fw_buf, finfo->front_setfile_start_addr,
			read_size);

	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	info("End SPI read front setfile data\n");

	/* CRC check */
	if (!fimc_is_sec_check_front_setfile_crc32(fw_buf) && (retry > 0)) {
		retry--;
		goto setfile_crc_retry;
	} else if (!retry) {
		ret = -EINVAL;
		err("Front setfile has dumped fail.. CRC ERROR ! \n");
		goto exit;
	}

	fimc_is_sec_get_front_setf_name(core);
	snprintf(setfile_path, sizeof(setfile_path), "%s%s", FIMC_IS_FW_DUMP_PATH,
		finfo->load_front_setfile_name);
	pos = 0;

#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
	buf = (u8 *)fw_buf;
	fimc_is_sec_inflate_fw(&buf, &finfo->front_setfile_size);
#endif
	if (write_data_to_file(setfile_path, fw_buf, finfo->front_setfile_size, &pos) < 0) {
		ret = -EIO;
		goto exit;
	}

	info("Front setfile has dumped successfully\n");

exit:
	return ret;
}
#endif

#ifdef CAMERA_MODULE_HIFI_TUNNINGF_DUMP
int fimc_is_sec_read_hifi_tunning_file(struct fimc_is_core *core)
{
	int ret = 0;
	loff_t pos = 0;
	char path[100];
	int retry = FIMC_IS_FW_RETRY_CNT;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	if (finfo->hifi_tunning_start_addr == 0) {
		info("hifi tunning don't need to be dumped\n");
		return 0;
	}

	info("hifi tunning need to be dumped\n");

crc_retry:
	/* read setfile data */
	info("Start SPI read hifi tunning data\n");
	memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	ret = fimc_is_spi_read(&core->spi0, fw_buf, finfo->hifi_tunning_start_addr,
		FIMC_IS_MAX_HIFI_TUNNING_SIZE_LS);
	if (ret) {
		err("failed to fimc_is_spi_read (%d)\n", ret);
		ret = -EINVAL;
		goto exit;
	}
	info("End SPI read hifi tunning data %x %ld\n", finfo->hifi_tunning_start_addr, finfo->hifi_tunning_size);

	/* CRC check */
	if (!fimc_is_sec_check_hifi_tunningfile_crc32(fw_buf) && (retry > 0)) {
		retry--;
		goto crc_retry;
	} else if (!retry) {
		ret = -EINVAL;
		err("hifi tunning has dumped fail.. CRC ERROR ! \n");
		goto exit;
	}

	snprintf(path, sizeof(path), "%s%s", FIMC_IS_FW_DUMP_PATH,
		finfo->load_tunning_hifills_name);
	pos = 0;

	if (write_data_to_file(path, fw_buf, finfo->hifi_tunning_size, &pos) < 0) {
		ret = -EIO;
		goto exit;
	}

	info("hifi tunning has dumped successfully\n");

exit:
	return ret;
}
#endif

#ifdef CAMERA_MODULE_COMPRESSED_FW_DUMP
int fimc_is_sec_inflate_fw(u8 **buf, unsigned long *size)
{
	z_stream zs_inflate;
	int ret = 0;
	char *unzip_buf;

	unzip_buf = vmalloc(FIMC_IS_MAX_FW_BUFFER_SIZE);
	if (!unzip_buf) {
		err("failed to allocate memory\n");
		ret = -ENOMEM;
		goto exit;
	}
	memset(unzip_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);

	zs_inflate.workspace = vmalloc(zlib_inflate_workspacesize());
	ret = zlib_inflateInit2(&zs_inflate, -MAX_WBITS);
	if (ret != Z_OK) {
		err("Camera : inflateInit error\n");
	}

	zs_inflate.next_in = *buf;
	zs_inflate.next_out = unzip_buf;
	zs_inflate.avail_in = *size;
	zs_inflate.avail_out = FIMC_IS_MAX_FW_BUFFER_SIZE;

	ret = zlib_inflate(&zs_inflate, Z_NO_FLUSH);
	if (ret != Z_STREAM_END) {
		err("Camera : zlib_inflate error %d \n", ret);
	}

	zlib_inflateEnd(&zs_inflate);
	vfree(zs_inflate.workspace);

	*size = FIMC_IS_MAX_FW_BUFFER_SIZE - zs_inflate.avail_out;
	memset(*buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);
	memcpy(*buf, unzip_buf, *size);
	vfree(unzip_buf);

exit:
	return ret;
}
#endif
#endif
#endif

int fimc_is_sec_get_pixel_size(char *header_ver)
{
	int pixelsize = 0;

	pixelsize += (int) (header_ver[FW_PIXEL_SIZE] - 0x30) * 10;
	pixelsize += (int) (header_ver[FW_PIXEL_SIZE + 1] - 0x30);

	return pixelsize;
}

int fimc_is_sec_core_voltage_select(struct device *dev, char *header_ver)
{
	struct regulator *regulator = NULL;
	int ret = 0;
	int minV, maxV;
	int pixelSize = 0;

	regulator = regulator_get(dev, "cam_sensor_core_1.2v");
	if (IS_ERR_OR_NULL(regulator)) {
		err("%s : regulator_get fail",
			__func__);
		return -EINVAL;
	}
	pixelSize = fimc_is_sec_get_pixel_size(header_ver);

	if (header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SONY ||
		header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SONY_LSI) {
		if (pixelSize == 13) {
			minV = 1050000;
			maxV = 1050000;
		} else if (pixelSize == 8) {
			minV = 1100000;
			maxV = 1100000;
		} else {
			minV = 1050000;
			maxV = 1050000;
		}
	} else if (header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI ||
				header_ver[FW_SENSOR_MAKER] == FW_SENSOR_MAKER_SLSI_SONY) {
		minV = 1200000;
		maxV = 1200000;
	} else {
		minV = 1050000;
		maxV = 1050000;
	}

	ret = regulator_set_voltage(regulator, minV, maxV);

	if (ret >= 0)
		info("%s : set_core_voltage %d, %d successfully\n",
				__func__, minV, maxV);
	regulator_put(regulator);

	return ret;
}

#if defined(CONFIG_CAMERA_FROM)
int fimc_is_sec_check_bin_files(struct fimc_is_core *core)
{
	int ret = 0;
	char fw_path[100];
	char dump_fw_path[100];
	char dump_fw_version[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };
	char phone_fw_version[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };
#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
	int from_fw_revision = 0;
	int dump_fw_revision = 0;
	int phone_fw_revision = 0;
	bool dump_flag = false;
	struct file *setfile_fp = NULL;
	char setfile_path[100];
#else
	static char fw_buf[FIMC_IS_MAX_FW_BUFFER_SIZE];
#endif
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize, nread;
	u8 *temp_buf = NULL;
	bool is_dump_existed = false;
	bool is_dump_needed = false;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_spi *spi = &core->spi0;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *pinfo = NULL;
	bool camera_running;

	camera_running = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR);

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_pinfo(&pinfo, ROM_ID_REAR);

	/* Use mutex for spi read */
	mutex_lock(&specific->rom_lock);

	if (test_bit(FIMC_IS_ROM_STATE_CHECK_BIN_DONE, &finfo->rom_state) && !force_caldata_dump) {
		mutex_unlock(&specific->rom_lock);
		return 0;
	}

	fimc_is_set_fw_names(finfo->load_fw_name, finfo->load_rta_fw_name);

	memset(fw_buf, 0x0, FIMC_IS_HEADER_VER_SIZE+10);
	temp_buf = fw_buf;

	fimc_is_sec_rom_power_on(core, SENSOR_POSITION_REAR);
	usleep_range(1000, 1000);

	/* Set SPI function */
	fimc_is_spi_s_pin(spi, SPI_PIN_STATE_ISP_FW); //spi-chip-select-mode required

	ret = fimc_is_spi_reset(&core->spi0);

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
	fimc_is_sec_get_rta_bin_addr();
	fimc_is_sec_get_rear_setfile_addr();
	fimc_is_sec_get_front_setfile_addr();
	fimc_is_sec_get_hifi_tunningfile_addr();
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
	fimc_is_sec_get_rear2_setfile_addr();
#endif
#endif

	snprintf(fw_path, sizeof(fw_path), "%s%s", FIMC_IS_FW_PATH, finfo->load_rta_fw_name);

	snprintf(dump_fw_path, sizeof(dump_fw_path), "%s%s",
		FIMC_IS_FW_DUMP_PATH, finfo->load_rta_fw_name);
	info("Camera: f-rom fw version: %s\n", finfo->header_ver);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(dump_fw_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		info("Camera: There is no dumped firmware(%s)\n", dump_fw_path);
		is_dump_existed = false;
		goto read_phone_fw;
	} else {
		is_dump_existed = true;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	info("start, file path %s, size %ld Bytes\n",
		dump_fw_path, fsize);

	fp->f_pos = fsize - FIMC_IS_HEADER_VER_OFFSET;
	fsize = FIMC_IS_HEADER_VER_SIZE;
	nread = vfs_read(fp, (char __user *)temp_buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		err("failed to read firmware file, %ld Bytes", nread);
		ret = -EIO;
		goto read_phone_fw;
	}

	strncpy(dump_fw_version, temp_buf, FIMC_IS_HEADER_VER_SIZE);
	info("Camera: dumped fw version: %s\n", dump_fw_version);

read_phone_fw:
	if (fp && is_dump_existed) {
		filp_close(fp, current->files);
		fp = NULL;
	}

	set_fs(old_fs);

	if (ret < 0)
		goto exit;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(fw_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		err("Camera: Failed open phone firmware(%s)", fw_path);
		fp = NULL;
		goto read_phone_fw_exit;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	info("start, file path %s, size %ld Bytes\n", fw_path, fsize);

	fp->f_pos = fsize - FIMC_IS_HEADER_VER_OFFSET;
	fsize = FIMC_IS_HEADER_VER_SIZE;
	nread = vfs_read(fp, (char __user *)temp_buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		err("failed to read firmware file, %ld Bytes", nread);
		ret = -EIO;
		goto read_phone_fw_exit;
	}

	strncpy(phone_fw_version, temp_buf, FIMC_IS_HEADER_VER_SIZE);
	strncpy(pinfo->header_ver, temp_buf, FIMC_IS_HEADER_VER_SIZE);
	info("Camera: phone fw version: %s\n", phone_fw_version);

read_phone_fw_exit:
	if (fp) {
		filp_close(fp, current->files);
		fp = NULL;
	}

	set_fs(old_fs);

	if (ret < 0)
		goto exit;

#if defined(CAMERA_MODULE_DUALIZE) && defined(CAMERA_MODULE_AVAILABLE_DUMP_VERSION)
	if (!strncmp(CAMERA_MODULE_AVAILABLE_DUMP_VERSION, finfo->header_ver, 3)) {
		from_fw_revision = fimc_is_sec_fw_revision(finfo->header_ver);
		phone_fw_revision = fimc_is_sec_fw_revision(phone_fw_version);
		if (is_dump_existed) {
			dump_fw_revision = fimc_is_sec_fw_revision(dump_fw_version);
		}

		info("from_fw_revision = %d, phone_fw_revision = %d, dump_fw_revision = %d\n",
			from_fw_revision, phone_fw_revision, dump_fw_revision);

		if (fimc_is_sec_compare_ver(ROM_ID_REAR) >= 0 /* Check if a module is connected or not */
			&& (!fimc_is_sec_fw_module_compare_for_dump(finfo->header_ver, phone_fw_version) ||
				(from_fw_revision > phone_fw_revision))) {
			is_dumped_fw_loading_needed = true;
			if (is_dump_existed) {
				if (!fimc_is_sec_fw_module_compare_for_dump(finfo->header_ver,
							dump_fw_version)) {
					is_dump_needed = true;
				} else if (from_fw_revision > dump_fw_revision) {
					is_dump_needed = true;
				} else {
					is_dump_needed = false;
				}
			} else {
				is_dump_needed = true;
			}
		} else {
			is_dump_needed = false;
			if (is_dump_existed) {
				if (!fimc_is_sec_fw_module_compare_for_dump(phone_fw_version,
					dump_fw_version)) {
					is_dumped_fw_loading_needed = false;
				} else if (phone_fw_revision > dump_fw_revision) {
					is_dumped_fw_loading_needed = false;
				} else {
					is_dumped_fw_loading_needed = true;
				}
			} else {
				is_dumped_fw_loading_needed = false;
			}
		}

		if (force_caldata_dump) {
			if ((!fimc_is_sec_fw_module_compare_for_dump(finfo->header_ver, phone_fw_version))
				|| (from_fw_revision > phone_fw_revision))
				dump_flag = true;
		} else {
			if (is_dump_needed) {
				dump_flag = true;
				set_bit(FIMC_IS_CRC_ERROR_FIRMWARE, &finfo->crc_error);
				set_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error);
#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
				set_bit(FIMC_IS_CRC_ERROR_SETFILE_2, &finfo->crc_error);
#endif
#ifdef CAMERA_MODULE_HIFI_TUNNINGF_DUMP
				set_bit(FIMC_IS_CRC_ERROR_HIFI_TUNNING, &finfo->crc_error);
#endif
			}
		}

		if (dump_flag) {
			info("Dump ISP Firmware.\n");

			ret = fimc_is_sec_readfw(core);
			msleep(20);
			ret |= fimc_is_sec_read_setfile(core);
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
			ret |= fimc_is_sec_read_setfile2(core);
#endif
#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
			ret |= fimc_is_sec_read_front_setfile(core);
#endif
#ifdef CAMERA_MODULE_HIFI_TUNNINGF_DUMP
			ret |= fimc_is_sec_read_hifi_tunning_file(core);
#endif
			if (ret < 0) {
				if (test_bit(FIMC_IS_CRC_ERROR_FIRMWARE, &finfo->crc_error)
					|| test_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error)
#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
					|| test_bit(FIMC_IS_CRC_ERROR_SETFILE_2, &finfo->crc_error)
#endif
#ifdef CAMERA_MODULE_HIFI_TUNNINGF_DUMP
					|| test_bit(FIMC_IS_CRC_ERROR_HIFI_TUNNING, &finfo->crc_error)
#endif
					) {
					is_dumped_fw_loading_needed = false;
					err("Firmware CRC is not valid. Does not use dumped firmware.\n");
				}
			}
		}

		if (phone_fw_version[0] == 0) {
			strcpy(pinfo->header_ver, "NULL");
		}

		if (is_dumped_fw_loading_needed) {
			old_fs = get_fs();
			set_fs(KERNEL_DS);
			snprintf(setfile_path, sizeof(setfile_path), "%s%s",
				FIMC_IS_FW_DUMP_PATH, finfo->load_setfile_name);
			setfile_fp = filp_open(setfile_path, O_RDONLY, 0);
			if (IS_ERR_OR_NULL(setfile_fp)) {
				set_bit(FIMC_IS_CRC_ERROR_SETFILE_1, &finfo->crc_error);
				info("setfile does not exist. Retry setfile dump.\n");

				fimc_is_sec_read_setfile(core);
				setfile_fp = NULL;
			} else {
				if (setfile_fp) {
					filp_close(setfile_fp, current->files);
				}
				setfile_fp = NULL;
			}

#ifdef CAMERA_MODULE_FRONT_SETF_DUMP
			memset(setfile_path, 0x0, sizeof(setfile_path));
			fimc_is_sec_get_front_setf_name(core);
			snprintf(setfile_path, sizeof(setfile_path), "%s%s",
				FIMC_IS_FW_DUMP_PATH, finfo->load_front_setfile_name);
			setfile_fp = filp_open(setfile_path, O_RDONLY, 0);
			if (IS_ERR_OR_NULL(setfile_fp)) {
				set_bit(FIMC_IS_CRC_ERROR_SETFILE_2, &finfo->crc_error);
				info("setfile does not exist. Retry front setfile dump.\n");

				fimc_is_sec_read_front_setfile(core);
				setfile_fp = NULL;
			} else {
				if (setfile_fp) {
					filp_close(setfile_fp, current->files);
				}
				setfile_fp = NULL;
			}
#endif
			set_fs(old_fs);
		}
	}
#endif

	if (is_dump_needed && is_dumped_fw_loading_needed) {
		strncpy(loaded_fw, finfo->header_ver, FIMC_IS_HEADER_VER_SIZE);
	} else if (!is_dump_needed && is_dumped_fw_loading_needed) {
		strncpy(loaded_fw, dump_fw_version, FIMC_IS_HEADER_VER_SIZE);
	} else {
		strncpy(loaded_fw, phone_fw_version, FIMC_IS_HEADER_VER_SIZE);
	}

exit:
	/* Set spi pin to out */
	fimc_is_spi_s_pin(spi, SPI_PIN_STATE_IDLE);

	set_bit(FIMC_IS_ROM_STATE_CHECK_BIN_DONE, &finfo->rom_state);

	if ((specific->f_rom_power == FROM_POWER_SOURCE_REAR_SECOND && !camera_running) ||
		(specific->f_rom_power == FROM_POWER_SOURCE_REAR && !camera_running)) {
		fimc_is_sec_rom_power_off(core, SENSOR_POSITION_REAR);
	}

	mutex_unlock(&specific->rom_lock);
	return 0;
}
#endif

int fimc_is_sec_sensorid_find(struct fimc_is_core *core)
{
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	snprintf(finfo->load_fw_name, sizeof(FIMC_IS_DDK), "%s", FIMC_IS_DDK);

	if (fimc_is_sec_fw_module_compare(finfo->header_ver, FW_2L4_L)) {
		specific->rear_sensor_id = SENSOR_NAME_SAK2L4;
	}

	info("%s sensor id %d\n", __func__, specific->rear_sensor_id);

	return 0;
}

int fimc_is_sec_sensorid_find_front(struct fimc_is_core *core)
{
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_FRONT);

	if (fimc_is_sec_fw_module_compare(finfo->header_ver, FW_3J1_X)) {
		specific->front_sensor_id = SENSOR_NAME_S5K3J1;
	}

	info("%s sensor id %d\n", __func__, specific->front_sensor_id);

	return 0;
}

int fimc_is_sec_sensorid_find_rear_tof(struct fimc_is_core *core)
{
#if defined(CAMERA_REAR_TOF) && defined(REAR_TOF_CHECK_SENSOR_ID)
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, REAR_TOF_ROM_ID);

	if(finfo->header_ver[FW_PIXEL_SIZE+1] == REAR_TOF_CHECK_SENSOR_ID) {
		specific->rear_tof_sensor_id = SENSOR_NAME_IMX316;
		info("current sensor is imx316");
	}
#endif
	return 0;
}

int fimc_is_sec_sensor_find_rear_tof_uid(struct fimc_is_core *core, char *buf)
{
#ifdef CAMERA_REAR_TOF
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, REAR_TOF_ROM_ID);

	if (finfo->cal_map_ver[3] >= REAR_TOF_CHECK_MAP_VERSION) {
		char uid_list[256] = {0, };
		char uid_temp[10] = {0, };
		for (int i = 0; i < finfo->rom_tof_cal_uid_addr_len; i++) {
			specific->rear_tof_uid[i] = *((int32_t*)&buf[finfo->rom_tof_cal_uid_addr[i]]);
			sprintf(uid_temp, "0x%x ", specific->rear_tof_uid[i]);
			strcat(uid_list, uid_temp);
		}
		info("rear_tof_uid: %s\n", uid_list);
	} else {
		for (int i = 0; i < finfo->rom_tof_cal_uid_addr_len; i++) {
			specific->rear_tof_uid[i] = REAR_TOF_DEFAULT_UID;
		}
		info("rear_tof_uid: 0x%x, use default 0x%x", *((int32_t*)&buf[finfo->rom_tof_cal_uid_addr[0]]),
													specific->rear_tof_uid[0]);
	}
#endif
	return 0;
}

int fimc_is_sec_sensor_find_front_tof_uid(struct fimc_is_core *core, char *buf)
{
#ifdef CAMERA_FRONT_TOF
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, FRONT_TOF_ROM_ID);

	if (finfo->cal_map_ver[3] >= FRONT_TOF_CHECK_MAP_VERSION) {
		char uid_list[256] = {0, };
		char uid_temp[10] = {0, };
		for (int i = 0; i < finfo->rom_tof_cal_uid_addr_len; i++) {
			specific->front_tof_uid[i] = *((int32_t*)&buf[finfo->rom_tof_cal_uid_addr[i]]);
			sprintf(uid_temp, "0x%x ", specific->front_tof_uid[i]);
			strcat(uid_list, uid_temp);
		}
		info("front_tof_uid: %s\n", uid_list);
	} else {
		for (int i = 0; i < finfo->rom_tof_cal_uid_addr_len; i++) {
			specific->front_tof_uid[i] = FRONT_TOF_DEFAULT_UID;
		}
		info("front_tof_uid: 0x%x, use default 0x%x", *((int32_t*)&buf[finfo->rom_tof_cal_uid_addr[0]]),
													specific->front_tof_uid[0]);
	}
#endif
	return 0;
}
int fimc_is_get_dual_cal_buf(int slave_position, char **buf, int *size)
{
	char *cal_buf;
	u32 rom_dual_cal_start_addr;
	u32 rom_dual_cal_size;
	u32 rom_dual_flag_dummy_addr = 0;
	int rom_type;
	int rom_dualcal_id;
	int rom_dualcal_index;
	struct fimc_is_core *core = dev_get_drvdata(fimc_is_dev);
	struct fimc_is_rom_info *finfo = NULL;

	fimc_is_vendor_get_rom_dualcal_info_from_position(slave_position, &rom_type, &rom_dualcal_id, &rom_dualcal_index);
	if (rom_type == ROM_TYPE_NONE) {
		err("[rom_dualcal_id:%d pos:%d] not support, no rom for camera", rom_dualcal_id, slave_position);
		return -EINVAL;
	} else if (rom_dualcal_id == ROM_ID_NOTHING) {
		err("[rom_dualcal_id:%d pos:%d] invalid ROM ID", rom_dualcal_id, slave_position);
		return -EINVAL;
	}

	fimc_is_sec_get_cal_buf(&cal_buf, rom_dualcal_id);
	if (fimc_is_sec_check_rom_ver(core, rom_dualcal_id) == false) {
		err("[rom_dualcal_id:%d pos:%d] ROM version is low. Cannot load dual cal.",
			rom_dualcal_id, slave_position);
		return -EINVAL;
	}

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_dualcal_id);
	if (test_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error) ||
		test_bit(FIMC_IS_CRC_ERROR_DUAL_CAMERA, &finfo->crc_error)) {
		err("[rom_dualcal_id:%d pos:%d] ROM Cal CRC is wrong. Cannot load dual cal.",
			rom_dualcal_id, slave_position);
		return -EINVAL;
	}

	if (rom_dualcal_index == ROM_DUALCAL_SLAVE0) {
		rom_dual_cal_start_addr = finfo->rom_dualcal_slave0_start_addr;
		rom_dual_cal_size = finfo->rom_dualcal_slave0_size;
	} else if (rom_dualcal_index == ROM_DUALCAL_SLAVE1) {
		rom_dual_cal_start_addr = finfo->rom_dualcal_slave1_start_addr;
		rom_dual_cal_size = finfo->rom_dualcal_slave1_size;
		rom_dual_flag_dummy_addr = ROM_REAR3_FLAG_DUMMY_ADDR;
	} else {
		err("[index:%d] not supported index.", rom_dualcal_index);
		return -EINVAL;
	}

	if (rom_dual_flag_dummy_addr != 0 && cal_buf[rom_dual_flag_dummy_addr] != 7) {
		err("[rom_dualcal_id:%d pos:%d] invalid dummy_flag [%d]. Cannot load dual cal.",
			rom_dualcal_id, slave_position, cal_buf[rom_dual_flag_dummy_addr]);
		return -EINVAL;
	}

	*buf = &cal_buf[rom_dual_cal_start_addr];
	*size = rom_dual_cal_size;

	return 0;
}

int fimc_is_sec_fw_find(struct fimc_is_core *core)
{
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	int front_id = specific->front_sensor_id;
	int rear_id = specific->rear_sensor_id;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_front = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);
	fimc_is_sec_get_sysfs_finfo(&finfo_front, ROM_ID_FRONT);

	if (test_bit(FIMC_IS_ROM_STATE_FW_FIND_DONE, &finfo->rom_state) && !force_caldata_dump)
		return 0;

	switch (rear_id) {
	case SENSOR_NAME_SAK2L4:
		snprintf(finfo->load_setfile_name, sizeof(FIMC_IS_2L4_SETF), "%s", FIMC_IS_2L4_SETF);
#ifdef CAMERA_MODULE_REAR2_SETF_DUMP
		snprintf(finfo->load_setfile2_name, sizeof(FIMC_IS_3M5_SETF), "%s", FIMC_IS_3M5_SETF);
#endif
		snprintf(finfo->load_rta_fw_name, sizeof(FIMC_IS_RTA), "%s", FIMC_IS_RTA);
		break;
	default:
		snprintf(finfo->load_setfile_name, sizeof(FIMC_IS_2L4_SETF), "%s", FIMC_IS_2L4_SETF);
		snprintf(finfo->load_rta_fw_name, sizeof(FIMC_IS_RTA), "%s", FIMC_IS_RTA);
		break;
	}

	switch (front_id) {
	case SENSOR_NAME_S5K3J1:
		snprintf(finfo_front->load_front_setfile_name, sizeof(FIMC_IS_3J1_SETF), "%s", FIMC_IS_3J1_SETF);
		break;
	case SENSOR_NAME_S5K4HA:
		snprintf(finfo_front->load_front_setfile_name, sizeof(FIMC_IS_4HA_SETF), "%s", FIMC_IS_4HA_SETF);
		break;
	default:
		snprintf(finfo_front->load_front_setfile_name, sizeof(FIMC_IS_3J1_SETF), "%s", FIMC_IS_3J1_SETF);
		break;
	}

#if defined(FIMC_IS_TUNNING_HIFILLS)
	snprintf(finfo->load_tunning_hifills_name, sizeof(FIMC_IS_TUNNING_HIFILLS),
		"%s", FIMC_IS_TUNNING_HIFILLS);
#endif

	info("%s rta name is %s\n", __func__, finfo->load_rta_fw_name);
	set_bit(FIMC_IS_ROM_STATE_FW_FIND_DONE, &finfo->rom_state);

	return 0;
}

int fimc_is_sec_run_fw_sel(struct device *dev, int rom_id)
{
	struct fimc_is_core *core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	int ret = 0;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *finfo_rear = NULL;

	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	/* FIMC_IS_FW_DUMP_PATH folder cannot access until User unlock handset */
	if (!test_bit(FIMC_IS_ROM_STATE_CAL_RELOAD, &finfo->rom_state)) {
		if (fimc_is_sec_file_exist(FIMC_IS_FW_DUMP_PATH)) {
			/* Check reload cal data enabled */
			fimc_is_sec_check_reload(core);
			set_bit(FIMC_IS_ROM_STATE_CAL_RELOAD, &finfo->rom_state);
			check_need_cal_dump = CAL_DUMP_STEP_CHECK;
			info("CAL_DUMP_STEP_CHECK");
		}
	}

	/* Check need to dump cal data */
	if (check_need_cal_dump == CAL_DUMP_STEP_CHECK) {
		if (test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state)) {
#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
			fimc_is_sec_cal_dump(core);
#endif
			check_need_cal_dump = CAL_DUMP_STEP_DONE;
			info("CAL_DUMP_STEP_DONE");
		}
	}

	info("%s rom id[%d] %d\n", __func__, rom_id,
		test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state));

	if (rom_id != ROM_ID_REAR) {
		fimc_is_sec_get_sysfs_finfo(&finfo_rear, ROM_ID_REAR);
		if (!test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo_rear->rom_state) || force_caldata_dump) {
			ret = fimc_is_sec_fw_sel_eeprom(dev, ROM_ID_REAR, true);
		}
	}

	if (!test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state) || force_caldata_dump) {
#if defined(CONFIG_CAMERA_FROM)
		ret = fimc_is_sec_fw_sel(core, dev, false);
#else
		ret = fimc_is_sec_fw_sel_eeprom(dev, rom_id, false);
#endif
	}

	if (specific->check_sensor_vendor) {
		if (fimc_is_sec_check_rom_ver(core, rom_id)) {
			fimc_is_sec_check_module_state(finfo);

			if (test_bit(FIMC_IS_ROM_STATE_OTHER_VENDOR, &finfo->rom_state)) {
				err("Not supported module. Rom ID = %d, Module ver = %s", rom_id, finfo->header_ver);
				return  -EIO;
			}
		}
	}

	return ret;
}

int fimc_is_sec_fw_sel_eeprom(struct device *dev, int rom_id, bool headerOnly)
{
	int ret = 0;
	char fw_path[100];
	char phone_fw_version[FIMC_IS_HEADER_VER_SIZE + 1] = {0, };

	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize, nread;
	u8 *read_buf = NULL;
	u8 *temp_buf = NULL;
	bool is_ldo_enabled;
	struct fimc_is_core *core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_rom_info *finfo = NULL;
	struct fimc_is_rom_info *pinfo = NULL;
	bool camera_running;

	fimc_is_sec_get_sysfs_pinfo(&pinfo, rom_id);
	fimc_is_sec_get_sysfs_finfo(&finfo, rom_id);

	is_ldo_enabled = false;

	/* Use mutex for i2c read */
	mutex_lock(&specific->rom_lock);

	if (!test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state) || force_caldata_dump) {
		if (rom_id == ROM_ID_REAR)
			is_dumped_fw_loading_needed = false;

		if (force_caldata_dump)
			info("forced caldata dump!!\n");

		fimc_is_sec_rom_power_on(core, finfo->rom_power_position);
		is_ldo_enabled = true;

		info("Camera: read cal data from EEPROM (ROM ID:%d)\n", rom_id);
		if (rom_id == ROM_ID_REAR && headerOnly) {
			fimc_is_sec_read_eeprom_header(dev, rom_id);
		} else {
			if (!fimc_is_sec_readcal_eeprom(dev, rom_id)) {
				set_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state);
			}
		}

		if (rom_id != ROM_ID_REAR) {
			goto exit;
		}
	}

	fimc_is_sec_sensorid_find(core);
	if (headerOnly) {
		goto exit;
	}

	snprintf(fw_path, sizeof(fw_path), "%s%s", FIMC_IS_FW_PATH, finfo->load_fw_name);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(fw_path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		err("Camera: Failed open phone firmware");
		ret = -EIO;
		fp = NULL;
		goto read_phone_fw_exit;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	info("start, file path %s, size %ld Bytes\n",
		fw_path, fsize);

#if defined(CONFIG_CAMERA_FROM) && defined(CAMERA_MODULE_DUALIZE)
	if (FIMC_IS_MAX_FW_BUFFER_SIZE >= fsize) {
		memset(fw_buf, 0x0, FIMC_IS_MAX_FW_BUFFER_SIZE);
		temp_buf = fw_buf;
	} else
#endif
	{
		info("Phone FW size is larger than FW buffer. Use vmalloc.\n");
		read_buf = vmalloc(fsize);
		if (!read_buf) {
			err("failed to allocate memory");
			ret = -ENOMEM;
			goto read_phone_fw_exit;
		}
		temp_buf = read_buf;
	}
	nread = vfs_read(fp, (char __user *)temp_buf, fsize, &fp->f_pos);
	if (nread != fsize) {
		err("failed to read firmware file, %ld Bytes", nread);
		ret = -EIO;
		goto read_phone_fw_exit;
	}

	strncpy(phone_fw_version, temp_buf + nread - FIMC_IS_HEADER_VER_OFFSET, FIMC_IS_HEADER_VER_SIZE);
	strncpy(pinfo->header_ver, temp_buf + nread - FIMC_IS_HEADER_VER_OFFSET, FIMC_IS_HEADER_VER_SIZE);
	info("Camera: phone fw version: %s\n", phone_fw_version);

read_phone_fw_exit:
	if (read_buf) {
		vfree(read_buf);
		read_buf = NULL;
		temp_buf = NULL;
	}

	if (fp) {
		filp_close(fp, current->files);
		fp = NULL;
	}

	set_fs(old_fs);

exit:
	camera_running = fimc_is_vendor_check_camera_running(finfo->rom_power_position);
	if (is_ldo_enabled && !camera_running)
		fimc_is_sec_rom_power_off(core, finfo->rom_power_position);

	mutex_unlock(&specific->rom_lock);

	return ret;
}

#if defined(CONFIG_CAMERA_FROM)
int fimc_is_sec_fw_sel(struct fimc_is_core *core, struct device *dev, bool headerOnly)
{
	int ret = 0;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct fimc_is_spi *spi = &core->spi0;
	bool is_FromPower_enabled = false;
	struct exynos_platform_fimc_is *core_pdata = NULL;
	struct fimc_is_rom_info *finfo = NULL;
	bool camera_running;
	bool camera_running_rear2;

	camera_running = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR);
	camera_running_rear2 = fimc_is_vendor_check_camera_running(SENSOR_POSITION_REAR2);

	fimc_is_sec_get_sysfs_finfo(&finfo, ROM_ID_REAR);

	core_pdata = dev_get_platdata(fimc_is_dev);
	if (!core_pdata) {
		err("core->pdata is null\n");
		return -EINVAL;
	}

	/* Use mutex for spi read */
	mutex_lock(&specific->rom_lock);
	if (!test_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state) || force_caldata_dump) {
		is_dumped_fw_loading_needed = false;
		if (force_caldata_dump)
			info("forced caldata dump!!\n");

		fimc_is_sec_rom_power_on(core, SENSOR_POSITION_REAR);
		usleep_range(1000, 1000);
		is_FromPower_enabled = true;

		info("read cal data from FROM\n");
		/* Set SPI function */
		fimc_is_spi_s_pin(spi, SPI_PIN_STATE_ISP_FW); //spi-chip-select-mode required

		if (headerOnly) {
			fimc_is_sec_read_from_header(dev);
		} else {
			if (!fimc_is_sec_readcal(core)) {
				set_bit(FIMC_IS_ROM_STATE_CAL_READ_DONE, &finfo->rom_state);
			}
		}

		/*select AF actuator*/
		if (test_bit(FIMC_IS_CRC_ERROR_ALL_SECTION, &finfo->crc_error)) {
			info("Camera : CRC32 error for all section.\n");
		}

		fimc_is_sec_sensorid_find(core);
		if (headerOnly) {
			goto exit;
		}

	} else {
		info("already loaded the firmware, Phone version=%s, F-ROM version=%s\n",
			sysfs_pinfo.header_ver, finfo->header_ver);
	}

exit:
	/* Set spi pin to out */
	fimc_is_spi_s_pin(spi, SPI_PIN_STATE_IDLE);
	if (is_FromPower_enabled &&
		((specific->f_rom_power == FROM_POWER_SOURCE_REAR_SECOND && !camera_running_rear2) ||
		(specific->f_rom_power == FROM_POWER_SOURCE_REAR && !camera_running))) {
		fimc_is_sec_rom_power_off(core, SENSOR_POSITION_REAR);
	}

	mutex_unlock(&specific->rom_lock);

	return ret;
}
#endif
