/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#ifdef CONFIG_USE_VENDER_FEATURE
#include "fimc-is-sec-define.h"
#else
#include "fimc-is-binary.h"
#endif

#ifdef CONFIG_COMPANION_DCDC_USE
#include "fimc-is-fan53555.h"
#include "fimc-is-ncp6335b.h"
#endif

#include "fimc-is-companion.h"
#include "fimc-is-companion_address.h"
#include "fimc-is-device-ischain.h"
#ifdef USE_ION_ALLOC
#include <linux/dma-buf.h>
#include <linux/exynos_ion.h>
#include <linux/ion.h>
#endif

#define COMP_FW_EVT0			"companion_fw_evt0.bin"
#define COMP_FW_EVT1			"companion_fw_evt1.bin"
#define COMP_SETFILE_MASTER		"companion_master_setfile.bin"
#define COMP_SETFILE_MODE		"companion_mode_setfile.bin"
#define COMP_CAL_DATA		"cal"
#define COMP_LSC			"lsc"
#define COMP_PDAF			"pdaf"
#define COMP_COEF_CAL			"coef"
#define COMP_DEFAULT_LSC		"companion_default_lsc.bin"
#define COMP_DEFAULT_COEF		"companion_default_coef.bin"

#define COMP_SETFILE_VIRSION_SIZE	16
#define COMP_MAGIC_NUMBER		(0x73c1)
#define FIMC_IS_COMPANION_LSC_SIZE	6600
#define FIMC_IS_COMPANION_PDAF_SIZE	512
#define FIMC_IS_COMPANION_COEF_TOTAL_SIZE	(4096 * 6)
#define FIMC_IS_COMPANION_LSC_I0_SIZE	2
#define FIMC_IS_COMPANION_LSC_J0_SIZE	2
#define FIMC_IS_COMPANION_LSC_A_SIZE	4
#define FIMC_IS_COMPANION_LSC_K4_SIZE	4
#define FIMC_IS_COMPANION_LSC_SCALE_SIZE	2
#define FIMC_IS_COMPANION_WCOEF1_SIZE	10
#define FIMC_IS_COMPANION_COEF_CAL_SIZE	4032
#define FIMC_IS_COMPANION_AF_INF_SIZE	2
#define FIMC_IS_COMPANION_AF_MACRO_SIZE	2
#define LITTLE_ENDIAN 0
#define BIG_ENDIAN 1
extern bool companion_lsc_isvalid;
extern bool companion_coef_isvalid;
extern bool crc32_c1_check;
static u16 companion_ver;
static u32 concord_fw_size;
char companion_crc[10];

static int fimc_is_comp_spi_single_write(struct spi_device *spi, u16 addr, u16 data)
{
	int ret = 0;
	u8 tx_buf[5];

	tx_buf[0] = 0x02; /* write cmd */
	tx_buf[1] = (addr >> 8) & 0xFF; /* address */
	tx_buf[2] = (addr >> 0) & 0xFF; /* address */
	tx_buf[3] = (data >>  8) & 0xFF; /* data */
	tx_buf[4] = (data >>  0) & 0xFF; /* data */

	ret = spi_write(spi, &tx_buf[0], 5);
	if (ret)
		err("spi sync error - can't read data");

	return ret;
}

/* Burst mode: <command><address><data data data ...>
 * Burst width: Maximun value is 512.
 */
static int fimc_is_comp_spi_burst_write(struct spi_device *spi,
	u8 *buf, size_t size, size_t burst_width, int endian)
{
	int ret = 0;
	u32 i = 0, j = 0;
	u8 tx_buf[512];
	size_t burst_size;

	/* check multiples of 2 */
	burst_width = (burst_width + 2 - 1) / 2 * 2;

	burst_size = size / burst_width * burst_width;

	for (i = 0; i < burst_size; i += burst_width) {
		tx_buf[0] = 0x02; /* write cmd */
		tx_buf[1] = 0x6F; /* address */
		tx_buf[2] = 0x12; /* address */
		if (endian == LITTLE_ENDIAN) {
			for (j = 0; j < burst_width; j++)
				tx_buf[j + 3] = *(buf + i + j); /* data for big endian */
		} else if (endian == BIG_ENDIAN) {
			for (j = 0; j < burst_width; j += 2) {
				tx_buf[j + 3] = *(buf + i + j + 1); /* data for little endian */
				tx_buf[j + 4] = *(buf + i + j);
			}
		}

		ret = spi_write(spi, &tx_buf[0], j + 3);
		if (ret) {
			err("spi write error - can't write data");
			goto p_err;
		}
	}

	tx_buf[0] = 0x02; /* write cmd */
	tx_buf[1] = 0x6F; /* address */
	tx_buf[2] = 0x12; /* address */
	if (endian == LITTLE_ENDIAN) {
		for (j = 0; j < (size - burst_size); j ++) {
			tx_buf[j + 3] = *(buf + i + j); /* data for big endian */
		}
	} else if (endian == BIG_ENDIAN) {
		for (j = 0; j < (size - burst_size); j += 2) {
			tx_buf[j + 3] = *(buf + i + j + 1); /* data for little endian */
			tx_buf[j + 4] = *(buf + i + j);
		}
	}

	ret = spi_write(spi, &tx_buf[0], j + 3);
	if (ret)
		err("spi write error - can't write data");


p_err:
	return ret;
}

#if 0
static int fimc_is_comp_i2c_read(struct i2c_client *client, u16 addr, u16 *data)
{
	int err;
	u8 rxbuf[2], txbuf[2];
	struct i2c_msg msg[2];

	rxbuf[0] = (addr & 0xff00) >> 8;
	rxbuf[1] = (addr & 0xff);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = rxbuf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = txbuf;

	err = i2c_transfer(client->adapter, msg, 2);
	if (unlikely(err != 2)) {
		err("%s: register read fail", __func__);
		return -EIO;
	}

	*data = ((txbuf[0] << 8) | txbuf[1]);
	return 0;
}
#endif

#ifndef USE_SPI
static int fimc_is_comp_i2c_write(struct i2c_client *client ,u16 addr, u16 data)
{
        int retries = I2C_RETRY_COUNT;
        int ret = 0, err = 0;
        u8 buf[4] = {0,};
        struct i2c_msg msg = {
                .addr   = client->addr,
                .flags  = 0,
                .len    = 4,
                .buf    = buf,
        };

        buf[0] = addr >> 8;
        buf[1] = addr;
        buf[2] = data >> 8;
        buf[3] = data & 0xff;

        do {
                ret = i2c_transfer(client->adapter, &msg, 1);
                if (likely(ret == 1))
                        break;

                usleep_range(10000,11000);
                err = ret;
        } while (--retries > 0);

        /* Retry occured */
        if (unlikely(retries < I2C_RETRY_COUNT)) {
                err("i2c_write: error %d, write (%04X, %04X), retry %d",
                        err, addr, data, I2C_RETRY_COUNT - retries);
        }

        if (unlikely(ret != 1)) {
                err("I2C does not work\n");
                return -EIO;
        }

        return 0;
}
#endif

static int fimc_is_comp_single_write(struct fimc_is_core *core , u16 addr, u16 data)
{
	int ret = 0;

#ifdef USE_SPI
	struct spi_device *device = core->spi1.device;
	ret = fimc_is_comp_spi_single_write(device, addr, data);
	if (ret) {
		err("spi_single_write() fail");
	}
#else
	struct i2c_client *client = core->client0;
	fimc_is_s_int_comb_isp(core, true, INTMR2_INTMCIS22); /* interrupt on */
	ret = fimc_is_comp_i2c_write(client, addr, data);
	if (ret) {
		err("i2c write fail");
	}
	fimc_is_s_int_comb_isp(core, false, INTMR2_INTMCIS22); /* interrupt off */
#endif

	return ret;
}

static int fimc_is_comp_single_read(struct fimc_is_core *core , u16 addr, u16 *data, size_t size)
{
	int ret = 0;
//#ifdef USE_SPI
#if 1
	ret = fimc_is_spi_read(&core->spi1, data, addr << 8, size);
	if (ret) {
		err("spi_single_read() fail");
	}
	*data = *data << 8 | *data >> 8;
#else
	struct i2c_client *client = core->client0;
	fimc_is_s_int_comb_isp(core, true, INTMR2_INTMCIS22); /* I2C0 interrupt on */
	ret = fimc_is_comp_i2c_read(client, addr, data);
	if (ret) {
		err("i2c read fail");
	}
	fimc_is_s_int_comb_isp(core, false, INTMR2_INTMCIS22); /* I2C0 interrupt off */
#endif

	return ret;
}

static int fimc_is_comp_check_crc32(struct fimc_is_core *core, char *name)
{
	int ret = 0;
	int  retries = CRC_RETRY_COUNT;
	u32 checksum, from_checksum = 0;
	u16 addr1, addr2, read_addr, data_size1, data_size2;
	u16 result_data, cmd_data, temp_data;
	struct fimc_is_rom_info *sysfs_finfo;
	char *cal_buf;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	fimc_is_sec_get_cal_buf(&cal_buf);

	if (!strcmp(name, COMP_LSC)) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_LSC_SIZE;
		addr1 = 0x2001;
		addr2 = 0xA000;
		if (companion_lsc_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->lsc_gain_crc_addr]);
		else
			from_checksum = 0x043DC8F8;
	} else if (!strcmp(name, COMP_PDAF)) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_PDAF_SIZE;
		addr1 = 0x2000;
		addr2 = 0xB900;
		from_checksum = *((u32 *)&cal_buf[sysfs_finfo->pdaf_crc_addr]);
	} else if (!strcmp(name, COMP_FW_EVT1) || !strcmp(name, COMP_FW_EVT0)) {
		concord_fw_size = concord_fw_size - 4;
		data_size1 = (concord_fw_size >> 16) & 0xFFFF;
		data_size2 = concord_fw_size & 0xFFFF;
		addr1 = 0x0000;
		addr2 = 0x0000;
		from_checksum = *((u32 *)&companion_crc[0]);
	} else if (!strcmp(name, COMP_COEF_CAL)) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_COEF_CAL_SIZE;
		addr1 = 0x2001;
		addr2 = 0x0000;
		if (companion_coef_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->coef1_crc_addr]);
		else
			from_checksum = 0x98E85DAC;
	} else if (!strcmp(name, "coef2")) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_COEF_CAL_SIZE;
		addr1 = 0x2001;
		addr2 = 0x1000;
		if (companion_coef_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->coef2_crc_addr]);
		else
			from_checksum = 0x88358F49;
	} else if (!strcmp(name, "coef3")) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_COEF_CAL_SIZE;
		addr1 = 0x2001;
		addr2 = 0x2000;
		if (companion_coef_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->coef3_crc_addr]);
		else
			from_checksum = 0x138AF2AB;
	} else if (!strcmp(name, "coef4")) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_COEF_CAL_SIZE;
		addr1 = 0x2001;
		addr2 = 0x3000;
		if (companion_coef_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->coef4_crc_addr]);
		else
			from_checksum = 0x4447017A;
	} else if (!strcmp(name, "coef5")) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_COEF_CAL_SIZE;
		addr1 = 0x2001;
		addr2 = 0x4000;
		if (companion_coef_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->coef5_crc_addr]);
		else
			from_checksum = 0xDBB05433;
	} else if (!strcmp(name, "coef6")) {
		data_size1 = 0;
		data_size2 = FIMC_IS_COMPANION_COEF_CAL_SIZE;
		addr1 = 0x2001;
		addr2 = 0x5000;
		if (companion_coef_isvalid)
			from_checksum = *((u32 *)&cal_buf[sysfs_finfo->coef6_crc_addr]);
		else
			from_checksum = 0x66064DFA;
	} else {
		err("wrong companion cal data name\n");
		return -EINVAL;
	}

	ret = fimc_is_comp_single_write(core, 0x0024, 0x0000);
	ret |= fimc_is_comp_single_write(core, 0x0026, 0x0000);//clear result register
	ret |= fimc_is_comp_single_write(core, 0x0014, addr1);
	ret |= fimc_is_comp_single_write(core, 0x0016, addr2);// source address
	ret |= fimc_is_comp_single_write(core, 0x0018, data_size1);
	ret |= fimc_is_comp_single_write(core, 0x001A, data_size2);//source size
	ret |= fimc_is_comp_single_write(core, 0x000C, 0x000C);
	ret |= fimc_is_comp_single_write(core, 0x6806, 0x0001);//interrupt on
	if (ret) {
		err("fimc_is_comp_check_crc32() i2c write fail");
		return -EIO;
	}

	read_addr = 0x000C;
	do {
		fimc_is_comp_single_read(core, read_addr, &cmd_data, 2);
		usleep_range(500, 500);
		if (--retries < 0) {
			err("Read register failed!!!!, data = 0x%04x\n", cmd_data);
			break;
		}
	} while (cmd_data);
	read_addr = 0x0024;
	fimc_is_comp_single_read(core, read_addr, &result_data, 2);
	read_addr = 0x0026;
	fimc_is_comp_single_read(core, read_addr, &temp_data, 2);

	checksum = result_data << 16 | temp_data;
	if (checksum != from_checksum) {
		err("[%s] CRC check is failed. Checksum = 0x%08x, FROM checksum = 0x%08x\n",
			name, checksum, from_checksum);
		return -EIO;
	}

	return ret;
}

static int fimc_is_comp_load_binary(struct fimc_is_core *core, char *name)
{
	int ret = 0;
	u32 size = 0;
	const struct firmware *fw_blob = NULL;
	static char fw_name[100];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;
	int fw_requested = 1;
	u32 i;
	u8 *buf = NULL;
	u32 data, cal_addr;
	char version_str[60];
	u16 addr1, addr2;
	char companion_ver[12] = {0, };
	struct fimc_is_rom_info *sysfs_finfo;
#ifdef USE_ION_ALLOC
	struct ion_handle *handle = NULL;
#endif
	int retry_count = 0;

	BUG_ON(!core);
	BUG_ON(!core->pdev);
	BUG_ON(!core->companion.pdev);
	BUG_ON(!name);
#ifdef USE_ION_ALLOC
	BUG_ON(!core->fimc_ion_client);
#endif
	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(fw_name, sizeof(fw_name), "%s%s",FIMC_IS_SETFILE_SDCARD_PATH, name);

	fp = filp_open(fw_name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		goto request_fw;
	}

	fw_requested = 0;
	size = fp->f_path.dentry->d_inode->i_size;
	info("start read sdcard, file path %s, size %d Bytes\n", fw_name, size);

#ifdef USE_ION_ALLOC
	handle = ion_alloc(core->fimc_ion_client, (size_t)size, 0,
				EXYNOS_ION_HEAP_SYSTEM_MASK, 0);
	if (IS_ERR_OR_NULL(handle)) {
		err("fimc_is_comp_load_binary:failed to ioc_alloc\n");
		ret = -ENOMEM;
		goto p_err;
	}

	buf = (u8 *)ion_map_kernel(core->fimc_ion_client, handle);
	if (IS_ERR_OR_NULL(buf)) {
		err("fimc_is_comp_load_binary:fail to ion_map_kernle\n");
		ret = -ENOMEM;
		goto p_err;
	}
#else
	buf = vmalloc(size);
	if (!buf) {
		err("failed to allocate memory");
		ret = -ENOMEM;
		goto p_err;
	}
#endif

	nread = vfs_read(fp, (char __user *)buf, size, &fp->f_pos);
	if (nread != size) {
		err("failed to read firmware file, %ld Bytes\n", nread);
		ret = -EIO;
		goto p_err;
	}
	if ((!strcmp(name, COMP_FW_EVT0)) || (!strcmp(name, sysfs_finfo->load_c1_fw_name))) {
		strncpy(companion_crc, buf+nread-4, 4);
		strncpy(companion_ver, buf+nread - 16, 11);
	}

request_fw:
	if (fw_requested) {
		snprintf(fw_name, sizeof(fw_name), "%s", name);
		set_fs(old_fs);
		retry_count = 3;
		ret = request_firmware(&fw_blob, fw_name, &core->companion.pdev->dev);
		while (--retry_count && ret == -EAGAIN) {
			err("request_firmware retry(count:%d)", retry_count);
			ret = request_firmware(&fw_blob, fw_name, &core->companion.pdev->dev);
		}

		if (ret) {
			err("request_firmware is fail(ret:%d). fw name: %s", ret, fw_name);
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
#ifdef USE_ION_ALLOC
		handle = ion_alloc(core->fimc_ion_client, (size_t)size, 0,
				EXYNOS_ION_HEAP_SYSTEM_MASK, 0);
		if (IS_ERR_OR_NULL(handle)) {
			err("fimc_is_comp_load_binary:failed to ioc_alloc\n");
			ret = -ENOMEM;
			goto p_err;
		}

		buf = (u8 *)ion_map_kernel(core->fimc_ion_client, handle);
		if (IS_ERR_OR_NULL(buf)) {
			err("fimc_is_comp_load_binary:fail to ion_map_kernle\n");
			ret = -ENOMEM;
			goto p_err;
		}
#else
		buf = vmalloc(size);
		if (!buf) {
			err("failed to allocate memory");
			ret = -ENOMEM;
			goto p_err;
		}
#endif
		memcpy((void *)buf, fw_blob->data, size);
		if ((!strcmp(name, COMP_FW_EVT0)) || (!strcmp(name, sysfs_finfo->load_c1_fw_name))) {
			memcpy((void *)companion_crc, fw_blob->data + size - 4, 4);
			memcpy((void *)companion_ver, fw_blob->data + size - 16, 11);
		}
	}

	if (!strcmp(name, COMP_FW_EVT0)) {
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, LITTLE_ENDIAN);
		if (ret) {
			err("fimc_is_comp_spi_write() fail");
			goto p_err;
		}
		concord_fw_size = size;
		info("%s version : %s\n", name, companion_ver);
	} else if (!strcmp(name, sysfs_finfo->load_c1_fw_name)) {
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, LITTLE_ENDIAN);
		if (ret) {
			err("fimc_is_comp_spi_write() fail");
			goto p_err;
		}
		concord_fw_size = size;
		info("%s version : %s\n", name, companion_ver);
	} else if (!strcmp(name, COMP_DEFAULT_LSC)) {
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			cal_addr = MEM_GRAS_B_IMX240;
		} else {
			cal_addr = MEM_GRAS_B_2P2;
		}
		addr1 = (cal_addr >> 16) & 0xFFFF;
		addr2 = cal_addr & 0xFFFF;
		ret = fimc_is_comp_single_write(core, 0x6428, addr1);
		if (ret) {
			err("fimc_is_comp_load_cal() write fail");
		}
		ret = fimc_is_comp_single_write(core, 0x642A, addr2);
		if (ret) {
			err("fimc_is_comp_load_cal() write fail");
		}
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, BIG_ENDIAN);
		if (ret) {
			err("fimc_is_comp_spi_write() fail");
			goto p_err;
		}
	} else if (!strcmp(name, COMP_DEFAULT_COEF)) {
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			cal_addr = MEM_XTALK_10_IMX240;
		} else {
			cal_addr = MEM_XTALK_10_2P2;
		}
		addr1 = (cal_addr >> 16) & 0xFFFF;
		addr2 = cal_addr & 0xFFFF;
		ret = fimc_is_comp_single_write(core, 0x6428, addr1);
		if (ret) {
			err("fimc_is_comp_load_cal() write fail");
		}
		ret = fimc_is_comp_single_write(core, 0x642A, addr2);
		if (ret) {
			err("fimc_is_comp_load_cal() write fail");
		}
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, LITTLE_ENDIAN);
		if (ret) {
			err("fimc_is_comp_spi_write() fail");
			goto p_err;
		}
	} else {
		u32 offset = size - COMP_SETFILE_VIRSION_SIZE;
		for (i = 0; i < offset; i += 4) {
			data =	*(buf + i + 0) << 24 |
				*(buf + i + 1) << 16 |
				*(buf + i + 2) << 8 |
				*(buf + i + 3) << 0;
			if(!strcmp(name, sysfs_finfo->load_c1_mastersetf_name)) {
				ret = fimc_is_comp_spi_single_write(core->spi1.device, (data >> 16), (u16)data);
				if (ret) {
					err("fimc_is_comp_spi_setf_write() fail");
					break;
				}
			} else {
				ret = fimc_is_comp_single_write(core, (data >> 16), (u16)data);
				if (ret) {
					err("fimc_is_comp_write() fail");
					break;
				}
			}
		}

		memcpy(version_str, buf + offset, COMP_SETFILE_VIRSION_SIZE);
		version_str[PREP_SETFILE_VIRSION_SIZE] = '\0';

		info("%s version : %s\n", name, version_str);
	}

p_err:
#ifdef USE_ION_ALLOC
	if (!IS_ERR_OR_NULL(buf)) {
		ion_unmap_kernel(core->fimc_ion_client, handle);
	}

	if (!IS_ERR_OR_NULL(handle)) {
		ion_free(core->fimc_ion_client, handle);
	}
#else
	if (buf) {
		vfree(buf);
	}
#endif
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

static int fimc_is_comp_load_cal(struct fimc_is_core *core, char *name)
{

	int ret = 0, endian;
	u32 data_size = 0, offset;
	struct fimc_is_rom_info *sysfs_finfo;
	char *cal_buf;
	u16 data1, data2;
	u32 comp_addr = 0;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	fimc_is_sec_get_cal_buf(&cal_buf);
	info("Camera: SPI write cal data, name = %s\n", name);

	if (!strcmp(name, COMP_LSC)) {
		data_size = FIMC_IS_COMPANION_LSC_SIZE;
		offset = sysfs_finfo->lsc_gain_start_addr;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = MEM_GRAS_B_IMX240;
		} else {
			comp_addr = MEM_GRAS_B_2P2;
		}
		endian = BIG_ENDIAN;
	} else if (!strcmp(name, COMP_PDAF)) {
		data_size = FIMC_IS_COMPANION_PDAF_SIZE;
		offset = sysfs_finfo->pdaf_start_addr;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = MEM_AF_10_1_IMX240;
		} else {
			comp_addr = MEM_AF_10_1_2P2;
		}
		endian = LITTLE_ENDIAN;
	} else if (!strcmp(name, COMP_COEF_CAL)) {
		data_size = FIMC_IS_COMPANION_COEF_TOTAL_SIZE;
		offset = sysfs_finfo->coef1_start;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = MEM_XTALK_10_IMX240;
		} else {
			comp_addr = MEM_XTALK_10_2P2;
		}
		endian = LITTLE_ENDIAN;
	} else {
		err("wrong companion cal data name\n");
		return -EINVAL;
	}
	data1 = comp_addr >> 16;
	data2 = (u16)comp_addr;
	ret = fimc_is_comp_single_write(core, 0x6428, data1);
	if (ret) {
		err("fimc_is_comp_load_cal() i2c write fail");
	}
	ret = fimc_is_comp_single_write(core, 0x642A, data2);
	if (ret) {
		err("fimc_is_comp_load_cal() i2c write fail");
	}

	ret = fimc_is_comp_spi_burst_write(core->spi1.device, cal_buf + offset, data_size, 256, endian);
	if (ret) {
		err("fimc_is_comp_spi_write_cal() fail\n");
	}
	return ret;
}

#if 0
static int fimc_is_comp_read_i2c_cal(struct fimc_is_core *core, u32 addr)
{
	int ret = 0;
	u32 i,data_size = 0;
	u16 data1, data2;
	u8 read_data[2];

	struct fimc_is_rom_info *sysfs_finfo;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);

	fimc_is_s_int_comb_isp(core, true, INTMR2_INTMCIS22); /* interrupt on */
	if (addr == sysfs_finfo->lsc_i0_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_I0_SIZE;
		data1 = 0x0EF8;
	} else if (addr == sysfs_finfo->lsc_j0_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_J0_SIZE;
		data1 = 0x0EFA;
	} else if (addr == sysfs_finfo->lsc_a_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_A_SIZE;
		data1 = 0x0F00;
	} else if (addr == sysfs_finfo->lsc_k4_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_K4_SIZE;
		data1 = 0x0F04;
	} else if (addr == sysfs_finfo->lsc_scale_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_SCALE_SIZE;
		data1 = 0x0F08;
	} else if (addr == sysfs_finfo->wcoefficient1_addr) {
		data_size = FIMC_IS_COMPANION_WCOEF1_SIZE;
		data1 = 0x1420;
	} else {
		err("wrong companion cal data addr\n");
		return -EINVAL;
	}
	pr_info("===Camera: I2C read cal data, addr [0x%04x], size(%d)\n", addr,data_size);
	ret = fimc_is_comp_i2c_write(core->client0, 0x642C, 0x4000);
	if (ret) {
		err("fimc_is_comp_load_i2c_cal() i2c write fail");
	}
	ret = fimc_is_comp_i2c_write(core->client0, 0x642E, data1);
	if (ret) {
		err("fimc_is_comp_load_i2c_cal() i2c write fail");
	}

	for (i = 0; i < data_size; i += 2) {
		fimc_is_comp_spi_read(core->spi1, (void *)read_data, data1, 2);
		data2 = read_data[0] << 8 | read_data[1] << 0;
//		pr_info("===Camera: I2C read addr[0x%04x],data[0x%04x]\n", data1,data2);
		if (ret) {
				err("fimc_is_comp_i2c_setf_write() fail");
				break;
		}
		data1 += 2;
	}

	fimc_is_s_int_comb_isp(core, false, INTMR2_INTMCIS22); /* interrupt off */

	return ret;
}
#endif

static int fimc_is_comp_load_i2c_cal(struct fimc_is_core *core, u32 addr)
{
	int ret = 0;
	u8 *buf = NULL;
	u32 i, j, data_size = 0;
	u16 data1, data2, data3 = 0, data4 = 0xFFFF;
	u32 comp_addr = 0;

	struct fimc_is_rom_info *sysfs_finfo;
	char *cal_buf;

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	fimc_is_sec_get_cal_buf(&cal_buf);

	if (addr == sysfs_finfo->lsc_i0_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_I0_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_uParabolicCenterX_IMX240;
		} else {
			comp_addr = grasTuning_uParabolicCenterX_2P2;
		}
		if (!companion_lsc_isvalid)
			data3 = 0x780A;
	} else if (addr == sysfs_finfo->lsc_j0_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_J0_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_uParabolicCenterY_IMX240;
		} else {
			comp_addr = grasTuning_uParabolicCenterY_2P2;
		}
		if (!companion_lsc_isvalid)
			data3 = 0xEC05;
	} else if (addr == sysfs_finfo->lsc_a_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_A_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_uBiQuadFactorA_IMX240;
		} else {
			comp_addr = grasTuning_uBiQuadFactorA_2P2;
		}
		if (!companion_lsc_isvalid) {
			data3 = 0x6A2A;
			data4 = 0x0100;
		}
	} else if (addr == sysfs_finfo->lsc_k4_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_K4_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_uBiQuadFactorB_IMX240;
		} else {
			comp_addr = grasTuning_uBiQuadFactorB_2P2;
		}
		if (!companion_lsc_isvalid) {
			data3 = 0x0040;
			data4 = 0x0000;
		}
	} else if (addr == sysfs_finfo->lsc_scale_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_SCALE_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_uBiQuadScaleShiftAdder_IMX240;
		} else {
			comp_addr = grasTuning_uBiQuadScaleShiftAdder_2P2;
		}
		if (!companion_lsc_isvalid)
			data3 = 0x0600;
	} else if (addr == sysfs_finfo->wcoefficient1_addr) {
		data_size = FIMC_IS_COMPANION_WCOEF1_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = xtalkTuningParams_wcr_IMX240;
		} else {
			comp_addr = xtalkTuningParams_wcr_2P2;
		}
	} else if (addr == sysfs_finfo->af_inf_addr) {
		data_size = FIMC_IS_COMPANION_AF_INF_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_actuatorPositionToShadingPowerLut_0_IMX240;
		} else {
			comp_addr = grasTuning_actuatorPositionToShadingPowerLut_0_2P2;
		}
	} else if (addr == sysfs_finfo->af_macro_addr) {
		data_size = FIMC_IS_COMPANION_AF_MACRO_SIZE;
		if (sysfs_finfo->sensor_id == COMPANION_SENSOR_IMX240) {
			comp_addr = grasTuning_actuatorPositionToShadingPowerLut_9_IMX240;
		} else {
			comp_addr = grasTuning_actuatorPositionToShadingPowerLut_9_2P2;
		}
	} else {
		err("wrong companion cal data name\n");
		return -EINVAL;
	}
	data1 = comp_addr >> 16;
	data2 = (u16)comp_addr;

	ret = fimc_is_comp_single_write(core, 0x6028, data1);
	if (ret) {
		err("fimc_is_comp_load_i2c_cal() i2c write fail");
	}
	ret = fimc_is_comp_single_write(core, 0x602A, data2);
	if (ret) {
		err("fimc_is_comp_load_i2c_cal() i2c write fail");
	}

	buf = cal_buf + addr;
	if (addr == sysfs_finfo->af_inf_addr || addr == sysfs_finfo->af_macro_addr) {
		data3 = *((u16 *)buf) / 2;
		ret = fimc_is_comp_single_write(core, data2, data3);
	} else {
		if (addr == sysfs_finfo->wcoefficient1_addr) {
			if (companion_coef_isvalid) {
				for (i = 0; i < data_size; i += 2) {
					data3 =	(*(buf + i + 1) << 8) | (*(buf + i));
					ret = fimc_is_comp_single_write(core, data2, data3);
					if (ret) {
						err("fimc_is_comp_i2c_setf_write() fail");
						break;
					}
					data2 += 2;
				}
			} else {
				ret = fimc_is_comp_single_write(core, data2, 0x0203);
				ret |= fimc_is_comp_single_write(core, data2 + 2, 0xB902);
				ret |= fimc_is_comp_single_write(core, data2 + 4, 0xA903);
				ret |= fimc_is_comp_single_write(core, data2 + 6, 0x2900);
				ret |= fimc_is_comp_single_write(core, data2 + 8, 0x4900);
				if (ret) {
					err("fimc_is_comp_i2c_setf_write() fail");
				}
			}
		} else {
			if (companion_lsc_isvalid) {
				j = data_size - 1;
				for (i = 0; i < data_size; i += 2) {
					data3 =	(*(buf + j) << 8) | (*(buf + j - 1));
					ret = fimc_is_comp_single_write(core, data2, data3);
					if (ret) {
						err("fimc_is_comp_i2c_setf_write() fail");
						break;
					}
					data2 += 2;
					j -= 2;
				}
			} else {
				ret = fimc_is_comp_single_write(core, data2, data3);
				if (ret) {
					err("fimc_is_comp_i2c_setf_write() fail");
				}
				if (data4 != 0xFFFF) {
					data2 += 2;
					ret = fimc_is_comp_single_write(core, data2, data4);
					if (ret) {
						err("fimc_is_comp_i2c_setf_write() fail");
					}
				}
			}
		}
	}
//	ret = fimc_is_comp_read_i2c_cal(core, addr);
	return ret;
}

#ifdef CONFIG_COMPANION_DCDC_USE
int fimc_is_power_binning(void *core_data)
{
	struct fimc_is_core *core = core_data;
	struct dcdc_power *dcdc = &core->companion_dcdc;
	int ret = 0;
	int err_check = 0, vout_sel = 0;
	u16 read_value = 0x80;
	int vout = 0;
	char buf[2]={0,};
	loff_t pos = 0;

	if ((DCDC_VENDOR_NONE == dcdc->type) || !dcdc->name) {
		err("DCDC power not exist");
		return -ENODEV;
	}

	ret = read_data_rom_file(FIMC_IS_ISP_CV, buf, 1, &pos);
	if(ret > 0) {
		if (kstrtoint(buf, 10, &vout_sel))
			err("convert fail");

		vout = dcdc->get_vout_val(vout_sel);
		ret = dcdc->set_vout(dcdc->client, vout);
		if (ret < 0 )
			err("dcdc set_vout(%d). sel %d", ret, vout_sel);

		info("[%s::%d][BIN_INFO::%s, sel %d] read path(%s), DCDC %s(%sV)\n",
				__FUNCTION__, __LINE__, buf, vout_sel, FIMC_IS_ISP_CV,
				dcdc->name, dcdc->get_vout_str(vout_sel));
		return vout_sel;
	}

	ret = fimc_is_comp_spi_single_write(core->spi1.device, 0x642C, 0x5000);
	if (ret) {
		err_check = 1;
		err("spi_single_write() fail");
	}

	ret = fimc_is_comp_spi_single_write(core->spi1.device, 0x642E, 0x500C);
	if (ret) {
		err_check = 1;
		err("spi_single_write() fail");
	}

	ret = fimc_is_comp_single_read(core, 0x6F12, &read_value, 2);
	if (ret) {
		err_check = 1;
		err("fimc_is_comp_single_read() fail");
	}

	info("[%s::%d][BIN_INFO::0x%04x]\n", __FUNCTION__, __LINE__, read_value);

	if (read_value & 0x3F) {
		if (read_value & (1 << CC_BIN6)) {
			buf[0]='6';
		} else if (read_value & (1 << CC_BIN5)) {
			buf[0]='5';
		} else if (read_value & (1 << CC_BIN4)) {
			buf[0]='4';
		} else if (read_value & (1 << CC_BIN3)) {
			buf[0]='3';
		} else if (read_value & (1 << CC_BIN2)) {
			buf[0]='2';
		} else if (read_value & (1 << CC_BIN1)) {
			buf[0]='1';
		} else {
			buf[0]='0'; /* Default voltage */
		}
	} else {
		buf[0]='0'; /* Default voltage */
		err_check = 1;
	}
	if (kstrtoint(buf, 10, &vout_sel))
		err("convert fail");

	vout = dcdc->get_vout_val(vout_sel);
	ret = dcdc->set_vout(dcdc->client, vout);
	if (ret) {
		err("dcdc set_vout(%d), sel %d", ret, vout_sel);
	} else {
		info("%s: DCDC %s, sel %d %sV\n", __func__,
			dcdc->name, vout_sel, dcdc->get_vout_str(vout_sel));
	}

	if (err_check == 0) {
		ret = write_data_to_file(FIMC_IS_ISP_CV, buf, 1, &pos);
		if (ret < 0)
			err("write_data_to_file() fail(%s)", buf);
	}

	info("[%s::%d][BIN_INFO::0x%04x] buf(%s), write %s. sel %d\n",
		__FUNCTION__, __LINE__, read_value, buf, err_check ? "skipped" : FIMC_IS_ISP_CV, vout_sel);

	return vout_sel;
}
#endif

int fimc_is_comp_is_valid(void *core_data)
{
	struct fimc_is_core *core = core_data;
	int ret = 0;
	u16 companion_id = 0;

	if (!core->spi1.device) {
		info("spi1 device is not available\n");
		goto exit;
	}

	/* check validation(Read data must be 0x73C1) */
	fimc_is_comp_single_read(core, 0x0, &companion_id, 2); /* reset addr : 0 */
	info("Companion vaildation: 0x%04x\n", companion_id);

exit:
	return ret;
}

int fimc_is_comp_read_ver(void *core_data)
{
	struct fimc_is_core *core = core_data;
	int ret = 0;

	if (!core->spi1.device) {
		info("spi1 device is not available\n");
		goto p_err;
	}

	companion_ver =  0;
	/* check validation(Read data must be 0x73C1) */
	fimc_is_comp_single_read(core, 0x02, &companion_ver, 2);
	info("%s: Companion version: 0x%04x\n", __func__, companion_ver);

	if (companion_ver == 0x00A0) {
		ret = fimc_is_comp_single_write(core, 0x0256, 0x0001);
	} else if (companion_ver == 0x00B0) {
		ret = fimc_is_comp_single_write(core, 0x0122, 0x0001);
	} else {
		err("companion version is invalid");
	}

	if (ret)
		err("fimc_is_comp_read_ver() fail");

p_err:
	return ret;
}

u8 fimc_is_comp_is_compare_ver(void *core_data)
{
	struct fimc_is_core *core = core_data;
	char *cal_buf;
	u32 rom_ver = 0, def_ver = 0;
	u8 ret = 0;
	char ver[3] = {'V', '0', '0'};

	fimc_is_sec_get_cal_buf(&cal_buf);

	if (!core->spi1.device) {
		info("spi1 device is not available\n");
		goto exit;
	}

	def_ver = ver[0] << 16 | ver[1] << 8 | ver[2];
	rom_ver = cal_buf[96] << 16 | cal_buf[97] << 8 | cal_buf[98];
	if (rom_ver == def_ver) {
		return cal_buf[99];
	} else {
		err("FROM core version is invalid. version is %c%c%c%c", cal_buf[96], cal_buf[97], cal_buf[98], cal_buf[99]);
		return 0;
	}
exit:
	return ret;
}

int fimc_is_comp_loadcal(void *core_data)
{
	struct fimc_is_core *core = core_data;
	struct fimc_is_rom_info *sysfs_finfo;
	int ret = 0;
	int retry_count = 3;
	bool pdaf_valid = false;
	struct exynos_platform_fimc_is *core_pdata = NULL;

	core_pdata = dev_get_platdata(fimc_is_dev);
	if (!core_pdata) {
		err("core->pdata is null");
	}

	if (!core->spi1.device) {
		pr_debug("spi1 device is not available");
		goto p_err;
	}

	if (!fimc_is_sec_check_rom_ver(core)) {
		err("%s: error, not implemented! skip..", __func__);
		return 0;
	}

	if (!crc32_c1_check) {
		pr_debug("CRC check fail.Do not apply cal data.");
		return 0;
	}

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
retry:
	if (fimc_is_comp_is_compare_ver(core) >= FROM_VERSION_V004) {
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_i0_gain_addr);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_i0_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_j0_gain_addr);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail",sysfs_finfo->lsc_j0_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_a_gain_addr);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_a_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_k4_gain_addr);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_k4_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_scale_gain_addr);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_scale_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->wcoefficient1_addr);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->wcoefficient1_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
	}

	if (fimc_is_comp_is_compare_ver(core) >= FROM_VERSION_V004) {
		if (companion_lsc_isvalid) {
			ret = fimc_is_comp_load_cal(core, COMP_LSC);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_LSC);
				goto p_err;
			}
			info("LSC from FROM loaded\n");
		}
#ifdef USE_DEFAULT_CAL
		else {
			ret = fimc_is_comp_load_binary(core, COMP_DEFAULT_LSC);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_DEFAULT_LSC);
				goto p_err;
			}
			info("Default LSC loaded\n");
		}
#endif
		usleep_range(1000, 1000);
	} else {
		info("Did not load LSC cal data\n");
	}

	/*Workaround : If FROM has ver.V003, Skip PDAF Cal loading to companion.*/
	if (fimc_is_comp_is_compare_ver(core) >= FROM_VERSION_V004) {
		ret = fimc_is_comp_load_cal(core, COMP_PDAF);
		if (ret) {
			err("fimc_is_comp_load_binary(%s) fail", COMP_PDAF);
			goto p_err;
		}
		pdaf_valid = true;
		usleep_range(1000, 1000);
	}
	if (fimc_is_comp_is_compare_ver(core) >= FROM_VERSION_V004) {
		if (companion_coef_isvalid) {
			ret = fimc_is_comp_load_cal(core, COMP_COEF_CAL);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_COEF_CAL);
				goto p_err;
			}
			info("COEF from FROM loaded\n");
		}
#ifdef USE_DEFAULT_CAL
		else {
			ret = fimc_is_comp_load_binary(core, COMP_DEFAULT_COEF);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_DEFAULT_COEF);
				goto p_err;
			}
			info("Default COEF loaded\n");
		}
#endif
		usleep_range(1000, 1000);
	} else {
		info("Did not load COEF cal data\n");
	}

	if (fimc_is_comp_is_compare_ver(core) >= FROM_VERSION_V004) {
		ret = fimc_is_comp_check_crc32(core, COMP_LSC);
		if (pdaf_valid)
			ret |= fimc_is_comp_check_crc32(core, COMP_PDAF);
		ret |= fimc_is_comp_check_crc32(core, COMP_COEF_CAL);
		ret |= fimc_is_comp_check_crc32(core, "coef2");
		ret |= fimc_is_comp_check_crc32(core, "coef3");
		ret |= fimc_is_comp_check_crc32(core, "coef4");
		ret |= fimc_is_comp_check_crc32(core, "coef5");
		ret |= fimc_is_comp_check_crc32(core, "coef6");
		if ((ret < 0) && (retry_count > 0)) {
			retry_count--;
			goto retry;
		}
	}
	usleep_range(5000, 5000);

p_err:
	return 0;
}

int fimc_is_comp_loadfirm(void *core_data)
{
	int ret = 0;
	int retry_count = 3;
	struct fimc_is_core *core = core_data;
	struct exynos_platform_fimc_is *core_pdata = NULL;
	struct fimc_is_rom_info *sysfs_finfo;

	core_pdata = dev_get_platdata(fimc_is_dev);
	if (!core_pdata) {
		err("core->pdata is null");
	}

	if (!core->spi1.device) {
		err("spi1 device is not available");
		goto p_err;
	}
	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);

	ret = fimc_is_comp_read_ver(core);
	if (ret) {
		err("fimc_is_comp_read_ver fail");
		goto p_err;
	}
	msleep(1);

retry:
	ret = fimc_is_comp_single_write(core, 0x6042, 0x0001);
	if (ret) {
		err("fimc_is_comp_i2c_setf_write() fail");
	}

	ret = fimc_is_comp_single_write(core, 0x6428, 0x0000);
	if (ret) {
		err("fimc_is_comp_i2c_setf_write() fail");
	}

	ret = fimc_is_comp_single_write(core, 0x642A, 0x0000);
	if (ret) {
		err("fimc_is_comp_i2c_setf_write() fail");
	}

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	if (companion_ver == 0x00A0)
		ret = fimc_is_comp_load_binary(core, COMP_FW_EVT0);
	else if (companion_ver == 0x00B0)
		ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_fw_name);
	if (ret) {
		err("fimc_is_comp_load_firmware fail");
		goto p_err;
	}

	ret = fimc_is_comp_single_write(core, 0x6014, 0x0001);
	if (ret) {
		err("fimc_is_comp_i2c_setf_write() fail");
	}

	if (fimc_is_sec_check_rom_ver(core)) {
		if (companion_ver == 0x00A0)
			ret = fimc_is_comp_check_crc32(core, COMP_FW_EVT0);
		else if (companion_ver == 0x00B0)
			ret = fimc_is_comp_check_crc32(core, COMP_FW_EVT1);
		if ((ret < 0) && (retry_count > 0)) {
			retry_count--;
			goto retry;
		}
	}
	usleep_range(5000, 5000);

	ret = fimc_is_comp_loadcal(core);
	if (ret) {
		err("fimc_is_comp_loadcal() fail");
	}
p_err:
	return ret;
}

int fimc_is_comp_loadsetf(void *core_data)
{
	int ret = 0;
	struct fimc_is_core *core = core_data;
	struct fimc_is_rom_info *sysfs_finfo;

	if (!core->spi1.device) {
		pr_debug("spi1 device is not available");
		goto p_err;
	}

	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);

	ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_mastersetf_name);
	if (ret) {
		err("fimc_is_comp_load_binary(%s) fail", sysfs_finfo->load_c1_mastersetf_name);
		goto p_err;
	}

		ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_modesetf_name);
		if (ret) {
			err("fimc_is_comp_load_binary(%s) fail", sysfs_finfo->load_c1_modesetf_name);
			goto p_err;
		}

	usleep_range(5000, 5000);
	/*Stream on concord*/
	ret = fimc_is_comp_single_write(core, 0x6800, 0x0001);
	if (ret) {
		err("fimc_is_comp_i2c_setf_write() fail");
	}
p_err:
	return ret;
}

void fimc_is_s_int_comb_isp(void *core_data, bool on, u32 ch)
{
	struct fimc_is_core *core = core_data;
	u32 cfg = ~0x0;

	if(on) {
		cfg &= ~ch;
		writel(cfg, core->regs + INTMR2);
	} else {
		cfg = 0xFFFFFFFF;
		writel(cfg, core->regs + INTMR2);
	}
}
