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

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/regulator/consumer.h>

#include "fimc-is-binary.h"
#include "fimc-is-sec-define.h"

#ifdef CONFIG_COMPANION_DCDC_USE
#include "fimc-is-fan53555.h"
#include "fimc-is-ncp6335b.h"
#endif

#include "fimc-is-config.h"
#include "fimc-is-i2c.h"
#include "fimc-is-core.h"
#include "fimc-is-companion.h"
#include "fimc-is-companion_address_c2.h"
#include "fimc-is-device-ischain.h"
#include "fimc-is-regs.h"
#include "fimc-is-vender-specific.h"

#ifdef USE_ION_ALLOC
#include <linux/dma-buf.h>
#include <linux/exynos_ion.h>
#include <linux/ion.h>
#endif

#define COMP_FW_EVT0			"companion_fw_evt0.bin"
#define COMP_FW_SDCARD			"companion_fw.bin"
#define COMP_SETFILE_MASTER_SDCARD		"companion_master_setfile.bin"
#define COMP_SETFILE_MODE_SDCARD		"companion_mode_setfile.bin"
#define COMP_CAL_DATA			"cal"
#define COMP_LSC			"lsc"
#define COMP_PDAF			"pdaf"
#define COMP_PDAF_SHAD			"pdaf_shad"
#define COMP_COEF_CAL			"coef"
#define COMP_DEFAULT_LSC		"companion_default_lsc.bin"
#define COMP_DEFAULT_COEF		"companion_default_coef.bin"

#define FIMC_IS_COMPANION_FW_START_ADDR			(0x00080000)
#define FIMC_IS_COMPANION_CAL_START_ADDR		(0x0003F5E0)
#define FIMC_IS_COMPANION_CAL_SIZE			(0x8A20)
#define FIMC_IS_COMPANION_API_SFLASH_CMD_EVT0		(0x0010)
#define FIMC_IS_COMPANION_API_SFLASH_SRAM_ADD_EVT0	(0x0018)
#define FIMC_IS_COMPANION_API_SFLASH_SIZE_EVT0		(0x001C)
#define FIMC_IS_COMPANION_API_SFLASH_CRC32_EVT0		(0x0028)
#define FIMC_IS_COMPANION_API_SFLASH_CMD_EVT1		(0x0014)
#define FIMC_IS_COMPANION_API_SFLASH_SRAM_ADD_EVT1	(0x001C)
#define FIMC_IS_COMPANION_API_SFLASH_SIZE_EVT1		(0x0020)
#define FIMC_IS_COMPANION_API_SFLASH_CRC32_EVT1		(0x002C)

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) || defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
#define FIMC_IS_COMPANION_FIRMWARE_CRC32_ADDR		(0x00)
#define FIMC_IS_COMPANION_PDAF_BPC_CRC32_ADDR		(0x04)
#define FIMC_IS_COMPANION_PDAF_SHAD_CRC32_ADDR		(0x08)
#define FIMC_IS_COMPANION_XTALK_CRC32_ADDR		(0x0C)
#define FIMC_IS_COMPANION_LSC_MAIN_GRID_CRC32_ADDR	(0X10)
#define FIMC_IS_COMPANION_LSC_FRONT_GRID_CRC32_ADDR	(0x14)
#define FIMC_IS_COMPANION_SIGNATURE_ADDR		(0x0002)
#define FIMC_IS_COMPANION_SIGNATURE_1ST_VALUE		(0x00B0)
#define FIMC_IS_COMPANION_SIGNATURE_2ND_VALUE		(0x10B0)
#define FIMC_IS_COMPANION_VALID_CRC			(0x0001)
#endif

#define FIMC_IS_COMPANION_HOST_INTRP_FLASH_CMD		(0x6806)
#define FIMC_IS_COMPANION_LSC_ADDR			(0x000C6810)
#define FIMC_IS_COMPANION_LSC_SIZE			5880
#define FIMC_IS_COMPANION_LSC_PARAMETER_ADDR		(0x000C7F0C)
#define FIMC_IS_COMPANION_LSC_PARAMETER_SIZE		228
#define FIMC_IS_COMPANION_LSC_ADDR_FRONT		(0x000C5020)
#define FIMC_IS_COMPANION_LSC_SIZE_FRONT		5880
#define FIMC_IS_COMPANION_LSC_PARAMETER_ADDR_FRONT	(0x000C671C)
#define FIMC_IS_COMPANION_LSC_PARAMETER_SIZE_FRONT	228
#define FIMC_IS_COMPANION_PDAF_ADDR			(0x000BF5E0)
#define FIMC_IS_COMPANION_PDAF_SIZE			512
#define FIMC_IS_COMPANION_PDAF_SHAD_ADDR		(0x000BF7E4)
#define FIMC_IS_COMPANION_PDAF_SHAD_SIZE		1056
#define FIMC_IS_COMPANION_XTALK_ADDR			(0x000BFC10)
#define FIMC_IS_COMPANION_XTALK_SIZE			21504
#define FIMC_IS_COMPANION_COEF_ADDR			(0x20010000)
#define FIMC_IS_COMPANION_COEF2_ADDR			(0x20011000)
#define FIMC_IS_COMPANION_COEF3_ADDR			(0x20012000)
#define FIMC_IS_COMPANION_COEF4_ADDR			(0x20013000)
#define FIMC_IS_COMPANION_COEF5_ADDR			(0x20014000)
#define FIMC_IS_COMPANION_COEF6_ADDR			(0x20015000)
#define FIMC_IS_COMPANION_COEF_TOTAL_SIZE		(21504 + 6)
#define FIMC_IS_COMPANION_FW_EVT1
#define FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR   (0x0044)

#define COMP_SETFILE_VIRSION_SIZE			16
#define COMP_MAGIC_NUMBER				(0x73c1)

#define FIMC_IS_COMPANION_LSC_I0_SIZE			8
#define FIMC_IS_COMPANION_LSC_J0_SIZE			8
#define FIMC_IS_COMPANION_LSC_A_SIZE			16
#define FIMC_IS_COMPANION_LSC_K4_SIZE			16
#define FIMC_IS_COMPANION_LSC_SCALE_SIZE		8
#define FIMC_IS_COMPANION_GRASTUNING_AWBASHCORD_SIZE        14
#define FIMC_IS_COMPANION_GRASTUNING_AWBASHCORDINDEXES_SIZE 14
#define FIMC_IS_COMPANION_GRASTUNING_GASALPHA_SIZE          56
#define FIMC_IS_COMPANION_GRASTUNING_GASBETA_SIZE           56
#define FIMC_IS_COMPANION_GRASTUNING_GASOUTDOORALPHA_SIZE   8
#define FIMC_IS_COMPANION_GRASTUNING_GASOUTDOORBETA_SIZE    8
#define FIMC_IS_COMPANION_GRASTUNING_GASINDOORALPHA_SIZE    8
#define FIMC_IS_COMPANION_GRASTUNING_GASINDOORBETA_SIZE     8
#define FIMC_IS_COMPANION_COEF_OFFSET_R_SIZE     2
#define FIMC_IS_COMPANION_COEF_OFFSET_G_SIZE     2
#define FIMC_IS_COMPANION_COEF_OFFSET_B_SIZE     2
#define FIMC_IS_COMPANION_WCOEF1_SIZE	10
#define FIMC_IS_COMPANION_AF_INF_SIZE	2
#define FIMC_IS_COMPANION_AF_MACRO_SIZE	2
#define LITTLE_ENDIAN 0
#define BIG_ENDIAN 1

extern bool companion_lsc_isvalid;
extern bool companion_front_lsc_isvalid;
extern bool companion_coef_isvalid;
extern bool crc32_c1_check;
extern bool crc32_c1_check_front;
extern bool is_dumped_c1_fw_loading_needed;
static u16 companion_ver;
bool pdaf_valid;
bool pdaf_shad_valid;

enum {
        COMPANION_FW = 0,
        COMPANION_MASTER,
        COMPANION_MODE,
        COMPANION_CAL,
};

static int fimc_is_comp_spi_single_read(struct fimc_is_core *core, u16 addr, u16 *data)
{
	int ret = 0;
	u8 tx_buf[4];
	struct spi_transfer t_c;
	struct spi_transfer t_r;
	struct spi_message m;
	struct spi_device *spi;

	BUG_ON(!core->spi1.device);

	spi = core->spi1.device;

	memset(&t_c, 0x00, sizeof(t_c));
	memset(&t_r, 0x00, sizeof(t_r));

	tx_buf[0] = 0x03; /* read cmd */
	tx_buf[1] = (addr >> 8) & 0xFF; /* address */
	tx_buf[2] = (addr >> 0) & 0xFF; /* address */
	tx_buf[3] = 0xFF; /* 8-bit dummy */

	t_c.tx_buf = tx_buf;
	t_c.len = 4;
	t_c.cs_change = 1;
	t_c.bits_per_word = 32;
	t_c.speed_hz = 12500000;

	t_r.rx_buf = data;
	t_r.len = 2;
	t_r.cs_change = 0;
	t_r.bits_per_word = 16;
	t_r.speed_hz = 12500000;

	spi_message_init(&m);
	spi_message_add_tail(&t_c, &m);
	spi_message_add_tail(&t_r, &m);

	ret = spi_sync(spi, &m);
	if (ret)
		err("spi sync error - can't read data");

	*data = *data << 8 | *data >> 8;

	return ret;
}

static int fimc_is_comp_spi_single_write(struct spi_device *spi, u16 addr, u16 data)
{
	int ret = 0;
	u8 tx_buf[5];

	tx_buf[0] = 0x02; /* write cmd */
	tx_buf[1] = (addr >> 8) & 0xFF; /* address */
	tx_buf[2] = (addr >> 0) & 0xFF; /* address */
	tx_buf[3] = (data >>  8) & 0xFF; /* data */
	tx_buf[4] = (data >>  0) & 0xFF; /* data */

	spi->bits_per_word = 8;
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
	ulong i = 0, j = 0;
	u8 tx_buf[512];
	size_t burst_size;

	/* check multiples of 2 */
	burst_width = ((burst_width + 2 - 1) / 2 * 2) - 3;

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

		if ((j + 3) % 32 == 0)
			spi->bits_per_word = 32;
		else
			spi->bits_per_word = 8;

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

	if ((j + 3) % 32 == 0)
		spi->bits_per_word = 32;
	else
		spi->bits_per_word = 8;

	ret = spi_write(spi, &tx_buf[0], j + 3);
	if (ret)
		err("spi write error - can't write data");


p_err:
	return ret;
}

int fimc_is_comp_i2c_read(struct i2c_client *client, u16 addr, u16 *data)
{
	int err;
	u8 rxbuf[2], txbuf[2] = {0, 0};
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

int fimc_is_comp_i2c_write(struct i2c_client *client ,u16 addr, u16 data)
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

static int fimc_is_comp_single_read(struct fimc_is_core *core , u16 addr, u16 *data)
{
	int ret = 0;
//#ifdef USE_SPI
#if 1
	ret = fimc_is_comp_spi_single_read(core, addr, data);
	if (ret) {
		err("spi_single_read() fail");
	}
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

int fimc_is_comp_get_crc(struct fimc_is_core *core, u32 addr, int size, char* crc)
{
	int ret = 0;
	int fail_count = 0;
	int retry_count = 1000;
	u16 data[2];
	u16 sflash_cmd = 0;
	u16 sflash_sram = 0;
	u16 sflash_size = 0;
	u16 sflash_crc32 = 0;

	data[0] = 0xffff;
	data[1] = 0xffff;

	if (companion_ver == FIMC_IS_COMPANION_VERSION_EVT0) {
		sflash_cmd = FIMC_IS_COMPANION_API_SFLASH_CMD_EVT0;
		sflash_sram = FIMC_IS_COMPANION_API_SFLASH_SRAM_ADD_EVT0;
		sflash_size = FIMC_IS_COMPANION_API_SFLASH_SIZE_EVT0;
		sflash_crc32 = FIMC_IS_COMPANION_API_SFLASH_CRC32_EVT0;
	} else {
		sflash_cmd = FIMC_IS_COMPANION_API_SFLASH_CMD_EVT1;
		sflash_sram = FIMC_IS_COMPANION_API_SFLASH_SRAM_ADD_EVT1;
		sflash_size = FIMC_IS_COMPANION_API_SFLASH_SIZE_EVT1;
		sflash_crc32 = FIMC_IS_COMPANION_API_SFLASH_CRC32_EVT1;
	}

	/* set CRC32 seed */
	ret = fimc_is_comp_single_write(core, sflash_crc32, 0x0000);
	if (ret) {
		fail_count++;
		err("API_SFLASH_CRC32 write fail");
	}
	ret = fimc_is_comp_single_write(core, sflash_crc32 + 2, 0x0000);
	if (ret) {
		fail_count++;
		err("API_SFLASH_CRC32 + 2 write fail");
	}

	/* set Start Address of memory */
	ret = fimc_is_comp_single_write(core, sflash_sram, (u16) ((addr & 0xFFFF0000) >> 16));
	if (ret) {
		fail_count++;
		err("API_SFLASH_SRAM_ADD write fail");
	}
	ret = fimc_is_comp_single_write(core, sflash_sram + 2, (u16) (addr & 0x0000FFFF));
	if (ret) {
		fail_count++;
		err("API_SFLASH_SRAM_ADD + 2 write fail");
	}

	/* set Size in bytes */
	ret = fimc_is_comp_single_write(core, sflash_size, (u16) ((size & 0xFFFF0000) >> 16));
	if (ret) {
		fail_count++;
		err("API_SFLASH_SFLASH_SIZE write fail");
	}
	ret = fimc_is_comp_single_write(core, sflash_size + 2, (u16) (size & 0x0000FFFF));
	if (ret) {
		fail_count++;
		err("API_SFLASH_SFLASH_SIZE + 2 write fail");
	}

	/* set CRC32 command */
	ret = fimc_is_comp_single_write(core, sflash_cmd, 0x000C);
	if (ret) {
		fail_count++;
		err("API_SFLASH_SFLASH_CMD write fail");
	}

	/* Host command interrupt */
	ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_HOST_INTRP_FLASH_CMD, 0x0001);
	if (ret) {
		fail_count++;
		err("HOST_INTRP_FLASH_CMD write fail");
	}

	/* wait for it to be cleared to Zero */
	while (retry_count--) {
		ret = fimc_is_comp_single_read(core, sflash_cmd, &data[0]);
		if (data[0] == 0x0000) {
			info("%s: API_SFLASH_CMD is cleared(%d)!\n", __func__, retry_count);
			break;
		} else {
			if (ret) {
				err("API_SFLASH_CMD read fail %d", retry_count);
			}
		}
		usleep_range(1000, 1000);
	}

	/* read CRC calculation result */
	if (retry_count <= 0) {
		fail_count++;
		err("API_SFLASH_CMD is not cleared!");
	} else {
		ret = fimc_is_comp_single_read(core, sflash_crc32, &data[0]);
		if (ret) {
			fail_count++;
			err("API_SFLASH_CRC32 read fail");
		}
		ret = fimc_is_comp_single_read(core, sflash_crc32 + 2, &data[1]);
		if (ret) {
			fail_count++;
			err("API_SFLASH_CRC32 + 2 read fail");
		}
		memcpy(crc, &data[1], 2);
		memcpy(crc + 2, &data[0], 2);
	}

	if (fail_count)
		ret = -1;
	else
		ret = 0;

	return ret;
}

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) || defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
static int fimc_is_comp_compare_crc_value(char* v1, char* v2)
{
	int ret = 0;

	if (memcmp((const void *)v1, (const void *)v2, FIMC_IS_COMPANION_CRC_SIZE)) {
		info("value is not same!\n");
		info("Value 1 = 0x%02x%02x%02x%02x\n",
				v1[0], v1[1], v1[2], v1[3]);
		info("Value 2 = 0x%02x%02x%02x%02x\n",
				v2[0], v2[1], v2[2], v2[3]);
		ret = -EINVAL;
		goto p_err;
	}

p_err:
	return ret;
}

static int fimc_is_comp_check_signature(struct fimc_is_core *core, u16 addr, char* value)
{
	int ret = 0;
	u16 data;

	ret = fimc_is_comp_single_read(core, addr, &data);
	if (ret) {
		err("Check Signature read fail");
		return -EINVAL;
	}

	if (memcmp((const void *)value, (const void *)&data, 2)) {
		err("0x%04x signature is not same!\n", addr);
		info("0x%04x signature = 0x%02x%02x\n",
				addr, value[0], value[1]);
		info("companion signature = 0x%04x\n", data);
		ret = -EINVAL;
		goto p_err;
	} else {
		info("0x%04x signature is same!\n", addr);
	}

p_err:
	return ret;
}
#endif

static int fimc_is_comp_check_crc32(struct fimc_is_core *core, u32 addr, int size, char* crc32)
{
	int ret = 0;
	char calc_crc[FIMC_IS_COMPANION_CRC_SIZE];

	ret = fimc_is_comp_get_crc(core, addr, size, calc_crc);
	if (ret) {
		err("fimc_is_comp_get_crc(0x%08x) fail", addr);
		ret = -EINVAL;
		goto p_err;
	}

	if (memcmp((const void *)crc32, (const void *)calc_crc, FIMC_IS_COMPANION_CRC_SIZE)) {
		err("0x%08x crc32 is not same!\n", addr);
		info("0x%08x crc32 = 0x%02x%02x%02x%02x\n",
				addr, crc32[0], crc32[1], crc32[2], crc32[3]);
		info("companion crc32 = 0x%02x%02x%02x%02x\n",
				calc_crc[0],
				calc_crc[1],
				calc_crc[2],
				calc_crc[3]);
		ret = -EINVAL;
		goto p_err;
	} else {
		info("0x%08x crc32 is same!\n", addr);
	}

p_err:
	return ret;
}

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) || defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
int fimc_is_comp_check_crc_with_signature(struct fimc_is_core *core)
{
	int ret = 0;
	struct fimc_is_rom_info *sysfs_finfo;
	struct fimc_is_companion_retention *ret_data;
	struct fimc_is_vender_specific *specific;
	char *cal_buf;
	char *pcalc_crc;
	char calc_crc[FIMC_IS_COMPANION_CRC_SIZE * FIMC_IS_COMPANION_CRC_OBJECT];
	char firmware_crc[FIMC_IS_COMPANION_CRC_SIZE];
	char pdaf_crc[FIMC_IS_COMPANION_CRC_SIZE];
	char pdaf_shad_crc[FIMC_IS_COMPANION_CRC_SIZE];
	char xtalk_crc[FIMC_IS_COMPANION_CRC_SIZE];
	char lsc_main_grid_crc[FIMC_IS_COMPANION_CRC_SIZE];
	char lsc_front_grid_crc[FIMC_IS_COMPANION_CRC_SIZE];
	u16 data[2];
	int i;

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	char isValid[2];
#endif

	pcalc_crc = calc_crc;

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN)
	fimc_is_comp_single_write(core, 0x642C, 0x2000);
	fimc_is_comp_single_write(core, 0x642E, 0xB72C);
#elif defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	fimc_is_comp_single_write(core, 0x642C, 0x000B);
	fimc_is_comp_single_write(core, 0x642E, 0xF500);
	fimc_is_comp_single_read(core, 0x6F12, &data[0]);
	fimc_is_comp_single_read(core, 0x6F12, &data[1]);
	memcpy(pcalc_crc, &data[0], 2);
	isValid[0] = (FIMC_IS_COMPANION_VALID_CRC & 0x00ff);
	isValid[1] = (FIMC_IS_COMPANION_VALID_CRC & 0xff00) >> 8;
	if (memcmp((const void *)pcalc_crc, (const void *)isValid, 2)) {
		info("crc is not valid! do not check crc.\n");
		info("Companion Valid = 0x%02x\n", pcalc_crc[0]);
		return 0;
	}
#endif
	for (i = 0; i < FIMC_IS_COMPANION_CRC_OBJECT; i++) {
		fimc_is_comp_single_read(core, 0x6F12, &data[0]);
		fimc_is_comp_single_read(core, 0x6F12, &data[1]);
		memcpy(pcalc_crc, &data[1], 2);
		memcpy(pcalc_crc + 2, &data[0], 2);
		pcalc_crc += 4;
	}

	memcpy(firmware_crc,
		calc_crc + FIMC_IS_COMPANION_FIRMWARE_CRC32_ADDR,
		FIMC_IS_COMPANION_CRC_SIZE);
	memcpy(pdaf_crc,
		calc_crc + FIMC_IS_COMPANION_PDAF_BPC_CRC32_ADDR,
		FIMC_IS_COMPANION_CRC_SIZE);
	memcpy(pdaf_shad_crc,
		calc_crc + FIMC_IS_COMPANION_PDAF_SHAD_CRC32_ADDR,
		FIMC_IS_COMPANION_CRC_SIZE);
	memcpy(xtalk_crc,
		calc_crc + FIMC_IS_COMPANION_XTALK_CRC32_ADDR,
		FIMC_IS_COMPANION_CRC_SIZE);
	memcpy(lsc_main_grid_crc,
		calc_crc + FIMC_IS_COMPANION_LSC_MAIN_GRID_CRC32_ADDR,
		FIMC_IS_COMPANION_CRC_SIZE);
	memcpy(lsc_front_grid_crc,
		calc_crc + FIMC_IS_COMPANION_LSC_FRONT_GRID_CRC32_ADDR,
		FIMC_IS_COMPANION_CRC_SIZE);

	specific = core->vender.private_data;
	ret_data = &specific->retention_data;

	ret = fimc_is_comp_compare_crc_value(firmware_crc, ret_data->firmware_crc32);
	if (ret < 0) {
		info("firmware crc is not same!\n");
		return ret;
	}

	if (core->current_position == SENSOR_POSITION_REAR) {
		fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
		fimc_is_sec_get_cal_buf(&cal_buf);
	} else {
		fimc_is_sec_get_sysfs_finfo_front(&sysfs_finfo);
		fimc_is_sec_get_front_cal_buf(&cal_buf);
	}

	if (fimc_is_sec_check_rom_ver(core, core->current_position)) {
		if(core->current_position == SENSOR_POSITION_REAR) {
			if (companion_lsc_isvalid) {
				ret = fimc_is_comp_compare_crc_value(lsc_main_grid_crc,
					&cal_buf[sysfs_finfo->lsc_gain_crc_addr]);
				if (ret < 0) {
					info("LSC MAIN GRID crc is not same!\n");
					return ret;
				}
			}
			if (pdaf_valid) {
				ret = fimc_is_comp_compare_crc_value(pdaf_crc,
					&cal_buf[sysfs_finfo->pdaf_crc_addr]);
				if (ret < 0) {
					info("PDAF crc is not same!\n");
					return ret;
				}
			}
			if (pdaf_shad_valid) {
				ret = fimc_is_comp_compare_crc_value(pdaf_shad_crc,
					&cal_buf[sysfs_finfo->pdaf_shad_crc_addr]);
				if (ret < 0) {
					info("PDAF SHAD crc is not same!\n");
					return ret;
				}
			}
			if (companion_coef_isvalid) {
				ret = fimc_is_comp_compare_crc_value(xtalk_crc,
					&cal_buf[sysfs_finfo->xtalk_coef_crc_addr]);
				if (ret < 0) {
					info("XTALK crc is not same!\n");
					return ret;
				}
			}
		} else {
			if (companion_front_lsc_isvalid ) {
				ret = fimc_is_comp_compare_crc_value(lsc_front_grid_crc,
					&cal_buf[sysfs_finfo->lsc_gain_crc_addr]);
			}
			if (ret < 0) {
				info("LSC FRONT GRID crc is not same!\n");
				return ret;
			}
		}
	}

	return ret;
}
#endif

int fimc_is_comp_check_cal_crc(struct fimc_is_core *core)
{
	int ret = 0;
	struct fimc_is_rom_info *sysfs_finfo;
	char *cal_buf;

	if (core->current_position == SENSOR_POSITION_REAR) {
		fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
		fimc_is_sec_get_cal_buf(&cal_buf);
	} else {
		fimc_is_sec_get_sysfs_finfo_front(&sysfs_finfo);
		fimc_is_sec_get_front_cal_buf(&cal_buf);
	}

	if (fimc_is_sec_check_rom_ver(core, core->current_position)) {
		if(core->current_position == SENSOR_POSITION_REAR) {
			if (companion_lsc_isvalid) {
				ret = fimc_is_comp_check_crc32(core,
					FIMC_IS_COMPANION_LSC_ADDR,
					FIMC_IS_COMPANION_LSC_SIZE,
					&cal_buf[sysfs_finfo->lsc_gain_crc_addr]);
			}
			if (pdaf_valid) {
				ret |= fimc_is_comp_check_crc32(core,
					FIMC_IS_COMPANION_PDAF_ADDR,
					FIMC_IS_COMPANION_PDAF_SIZE,
					&cal_buf[sysfs_finfo->pdaf_crc_addr]);
			}
			if (pdaf_shad_valid) {
				ret |= fimc_is_comp_check_crc32(core,
					FIMC_IS_COMPANION_PDAF_SHAD_ADDR,
					FIMC_IS_COMPANION_PDAF_SHAD_SIZE,
					&cal_buf[sysfs_finfo->pdaf_shad_crc_addr]);
			}
			if (companion_coef_isvalid) {
				ret |= fimc_is_comp_check_crc32(core,
					FIMC_IS_COMPANION_XTALK_ADDR,
					FIMC_IS_COMPANION_COEF_TOTAL_SIZE,
					&cal_buf[sysfs_finfo->xtalk_coef_crc_addr]);
			}
		} else {
			if (companion_front_lsc_isvalid ) {
				ret = fimc_is_comp_check_crc32(core,
					FIMC_IS_COMPANION_LSC_ADDR_FRONT,
					FIMC_IS_COMPANION_LSC_SIZE_FRONT,
					&cal_buf[sysfs_finfo->lsc_gain_crc_addr]);
			}
		}
	}

	return ret;
}

static int fimc_is_comp_load_binary(struct fimc_is_core *core, char *name, int bin_type)
{
	int ret = 0;
	ulong size = 0;
	const struct firmware *fw_blob = NULL;
	static char fw_name[100];
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;
	int fw_requested = 1;
	ulong i;
	u8 *buf = NULL;
	u32 data, cal_addr;
	char version_str[60];
	u16 addr1, addr2;
	char companion_rev[12] = {0, };
	struct fimc_is_rom_info *sysfs_finfo;
	struct fimc_is_rom_info *sysfs_pinfo;
	struct fimc_is_companion_retention *ret_data;
	struct fimc_is_vender_specific *specific;
#ifdef USE_ION_ALLOC
	struct ion_handle *handle = NULL;
#endif
	int retry_count = 0;

	BUG_ON(!core);
	BUG_ON(!core->pdev);
	BUG_ON(!core->preproc.pdev);
	BUG_ON(!name);
#ifdef USE_ION_ALLOC
	BUG_ON(!core->fimc_ion_client);
#endif
	specific = core->vender.private_data;
	ret_data = &specific->retention_data;
	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	fimc_is_sec_get_sysfs_pinfo(&sysfs_pinfo);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	if (bin_type == COMPANION_FW) {
		snprintf(fw_name, sizeof(fw_name), "%s%s",FIMC_IS_SETFILE_SDCARD_PATH, COMP_FW_SDCARD);
	} else if (bin_type == COMPANION_MASTER) {
		snprintf(fw_name, sizeof(fw_name), "%s%s",FIMC_IS_SETFILE_SDCARD_PATH, COMP_SETFILE_MASTER_SDCARD);
	} else if (bin_type == COMPANION_MODE) {
		snprintf(fw_name, sizeof(fw_name), "%s%s",FIMC_IS_SETFILE_SDCARD_PATH, COMP_SETFILE_MODE_SDCARD);
	} else {
		snprintf(fw_name, sizeof(fw_name), "%s%s",FIMC_IS_SETFILE_SDCARD_PATH, name);
	}

	fp = filp_open(fw_name, O_RDONLY, 0);
	if (IS_ERR_OR_NULL(fp)) {
		if (is_dumped_c1_fw_loading_needed && bin_type == COMPANION_FW) {
			snprintf(fw_name, sizeof(fw_name), "%s%s",
				FIMC_IS_FW_DUMP_PATH, sysfs_finfo->load_c1_fw_name);
			fp = filp_open(fw_name, O_RDONLY, 0);
			if (IS_ERR_OR_NULL(fp)) {
				warn("%s open failed", fw_name);
				fp = NULL;
				ret = -EIO;
				set_fs(old_fs);
				goto p_err;
			}
		} else {
			goto request_fw;
		}
	}

	fw_requested = 0;
	size = fp->f_path.dentry->d_inode->i_size;
	info("start read sdcard, file path %s, size %lu Bytes\n", fw_name, size);

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
	if (bin_type == COMPANION_FW) {
		strncpy(sysfs_pinfo->concord_header_ver, buf + nread - 16, FIMC_IS_HEADER_VER_SIZE);
		fimc_is_sec_set_loaded_c1_fw(sysfs_pinfo->concord_header_ver);
		strncpy(companion_rev, buf + nread - 16, 11);
		ret_data->firmware_size = (int)size;
		memset(ret_data->firmware_crc32, 0, sizeof(ret_data->firmware_crc32));
		memcpy(ret_data->firmware_crc32, (const void *) (buf + nread - FIMC_IS_COMPANION_CRC_SIZE), FIMC_IS_COMPANION_CRC_SIZE);
		info("Companion firmware size = %d firmware crc32 = 0x%02x%02x%02x%02x\n",
				ret_data->firmware_size,
				ret_data->firmware_crc32[0],
				ret_data->firmware_crc32[1],
				ret_data->firmware_crc32[2],
				ret_data->firmware_crc32[3]);
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
		if (bin_type == COMPANION_FW) {
			memcpy((void *)companion_rev, fw_blob->data + size - 16, 11);
			ret_data->firmware_size = (int)size;
			memcpy(ret_data->firmware_crc32, (const void *) (fw_blob->data + size - FIMC_IS_COMPANION_CRC_SIZE), FIMC_IS_COMPANION_CRC_SIZE);
			info("Companion firmware size = %d firmware crc32 = 0x%02x%02x%02x%02x\n",
					ret_data->firmware_size,
					ret_data->firmware_crc32[0],
					ret_data->firmware_crc32[1],
					ret_data->firmware_crc32[2],
					ret_data->firmware_crc32[3]);
		}
	}

	if (!strcmp(name, COMP_FW_EVT0)) {
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, LITTLE_ENDIAN);
		if (ret) {
			err("fimc_is_comp_spi_write() fail");
			goto p_err;
		}
		info("%s version : %s\n", name, companion_rev);
	} else if (!strcmp(name, sysfs_finfo->load_c1_fw_name)) {
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, LITTLE_ENDIAN);
		if (ret) {
			err("fimc_is_comp_spi_write() fail");
			goto p_err;
		}
		info("%s version : %s\n", name, companion_rev);
	} else if (!strcmp(name, COMP_DEFAULT_LSC)) {
		cal_addr = MEM_GRAS_B;
		addr1 = (cal_addr >> 16) & 0xFFFF;
		addr2 = cal_addr & 0xFFFF;
		ret = fimc_is_comp_single_write(core, 0x6428, addr1);
		if (ret) {
			err("%s:fimc_is_comp_single_write(0x6428) write fail", name);
		}
		ret = fimc_is_comp_single_write(core, 0x642A, addr2);
		if (ret) {
			err("%s:fimc_is_comp_single_write(0x642A) write fail", name);
		}
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, BIG_ENDIAN);
		if (ret) {
			err("%s:fimc_is_comp_spi_burst_writes() fail", name);
			goto p_err;
		}
	} else if (!strcmp(name, COMP_DEFAULT_COEF)) {
		cal_addr = MEM_XTALK_10;
		addr1 = (cal_addr >> 16) & 0xFFFF;
		addr2 = cal_addr & 0xFFFF;
		ret = fimc_is_comp_single_write(core, 0x6428, addr1);
		if (ret) {
			err("%s:fimc_is_comp_single_write(0x6428) write fail", name);
		}
		ret = fimc_is_comp_single_write(core, 0x642A, addr2);
		if (ret) {
			err("%s:fimc_is_comp_single_write(0x642A) write fail", name);
		}
		ret = fimc_is_comp_spi_burst_write(core->spi1.device, buf, size, 256, LITTLE_ENDIAN);
		if (ret) {
			err("%s:fimc_is_comp_spi_burst_write() fail", name);
			goto p_err;
		}
	} else {
		ulong offset = size - COMP_SETFILE_VIRSION_SIZE;
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
		version_str[COMP_SETFILE_VIRSION_SIZE] = '\0';

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

static int fimc_is_comp_load_cal(struct fimc_is_core *core, char *name, int position)
{

	int ret = 0, endian;
	u32 data_size = 0, offset;
	struct fimc_is_rom_info *sysfs_finfo;
	char *cal_buf;
	u16 data1, data2;
	u32 comp_addr = 0;

	if (position == SENSOR_POSITION_REAR) {
		fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
		fimc_is_sec_get_cal_buf(&cal_buf);
	} else {
		fimc_is_sec_get_sysfs_finfo_front(&sysfs_finfo);
		fimc_is_sec_get_front_cal_buf(&cal_buf);
	}
	info("Camera: SPI write cal data, name = %s\n", name);

	if (!strcmp(name, COMP_LSC)) {
		offset = sysfs_finfo->lsc_gain_start_addr;
		if (position == SENSOR_POSITION_REAR) {
			data_size = FIMC_IS_COMPANION_LSC_SIZE;
			comp_addr = MEM_GRAS_B;
			if (fimc_is_sec_check_rom_ver(core, position))
				endian = LITTLE_ENDIAN;
			else
				endian = BIG_ENDIAN;
		} else {
			data_size = FIMC_IS_COMPANION_LSC_SIZE_FRONT;
			comp_addr = MEM_GRAS_B_front;
			if (fimc_is_sec_check_rom_ver(core, position))
				endian = LITTLE_ENDIAN;
			else
				endian = BIG_ENDIAN;
		}
	} else if (!strcmp(name, COMP_PDAF)) {
		data_size = FIMC_IS_COMPANION_PDAF_SIZE;
		offset = sysfs_finfo->pdaf_start_addr;
		comp_addr = memoriesOffsets_PAF_DEF0;
		endian = LITTLE_ENDIAN;
	} else if (!strcmp(name, COMP_PDAF_SHAD)) {
		data_size = FIMC_IS_COMPANION_PDAF_SHAD_SIZE;
		offset = sysfs_finfo->pdaf_shad_start_addr;
		comp_addr = memoriesOffsets_PAF_SHAD1;
		endian = LITTLE_ENDIAN;
	} else if (!strcmp(name, COMP_COEF_CAL)) {
		data_size = FIMC_IS_COMPANION_XTALK_SIZE;
		offset = sysfs_finfo->xtalk_coef_start;
		comp_addr = memoriesOffsets_XT_COEF_R3;
		endian = LITTLE_ENDIAN;
	} else {
		err("wrong companion cal data name\n");
		return -EINVAL;
	}
	data1 = comp_addr >> 16;
	data2 = (u16)comp_addr;
	ret = fimc_is_comp_single_write(core, 0x6428, data1);
	if (ret) {
		err("%s:fimc_is_comp_single_write(0x6428) i2c write fail",__FUNCTION__);
	}
	ret = fimc_is_comp_single_write(core, 0x642A, data2);
	if (ret) {
		err("%s:fimc_is_comp_single_write(0x642A) i2c write fail", __FUNCTION__);
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

static int fimc_is_comp_load_i2c_cal(struct fimc_is_core *core, u32 addr, int position)
{
	int ret = 0;
	u8 *buf = NULL;
	u32 i, data_size = 0;
	u16 data1, data2, data3 ,data4 = 0;
	u32 comp_addr = 0;
	struct fimc_is_rom_info *sysfs_finfo;
	char *cal_buf;

	if (position == SENSOR_POSITION_REAR) {
		fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
		fimc_is_sec_get_cal_buf(&cal_buf);
	} else {
		fimc_is_sec_get_sysfs_finfo_front(&sysfs_finfo);
		fimc_is_sec_get_front_cal_buf(&cal_buf);
	}

	if (addr == sysfs_finfo->lsc_i0_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_I0_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_uParabolicCenterX;
		else
			comp_addr = grasTuning_uParabolicCenterX_front;
	} else if (addr == sysfs_finfo->lsc_j0_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_J0_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_uParabolicCenterY;
		else
			comp_addr = grasTuning_uParabolicCenterY_front;
	} else if (addr == sysfs_finfo->lsc_a_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_A_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_uBiQuadFactorA;
		else
			comp_addr = grasTuning_uBiQuadFactorA_front;
	} else if (addr == sysfs_finfo->lsc_k4_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_K4_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_uBiQuadFactorB;
		else
			comp_addr = grasTuning_uBiQuadFactorB_front;
	} else if (addr == sysfs_finfo->lsc_scale_gain_addr) {
		data_size = FIMC_IS_COMPANION_LSC_SCALE_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_uBiQuadScaleShiftAdder;
		else
			comp_addr = grasTuning_uBiQuadScaleShiftAdder_front;
	} else if (addr == sysfs_finfo->grasTuning_AwbAshCord_N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_AWBASHCORD_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_AwbAshCord;
		else
			comp_addr = grasTuning_AwbAshCord_front;
	} else if (addr == sysfs_finfo->grasTuning_awbAshCordIndexes_N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_AWBASHCORDINDEXES_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_awbAshCordIndexes;
		else
			comp_addr = grasTuning_awbAshCordIndexes_front;
	} else if (addr == sysfs_finfo->grasTuning_GASAlpha_M__N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_GASALPHA_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_GASAlpha;
		else
			comp_addr = grasTuning_GASAlpha_front;
	} else if (addr == sysfs_finfo->grasTuning_GASBeta_M__N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_GASBETA_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_GASBeta;
		else
			comp_addr = grasTuning_GASBeta_front;
	} else if (addr == sysfs_finfo->grasTuning_GASOutdoorAlpha_N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_GASOUTDOORALPHA_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_GASOutdoorAlpha;
		else
			comp_addr = grasTuning_GASOutdoorAlpha_front;
	} else if (addr == sysfs_finfo->grasTuning_GASOutdoorBeta_N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_GASOUTDOORBETA_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_GASOutdoorBeta;
		else
			comp_addr = grasTuning_GASOutdoorBeta_front;
	} else if (addr == sysfs_finfo->grasTuning_GASIndoorAlpha_N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_GASINDOORALPHA_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_GASIndoorAlpha;
		else
			comp_addr = grasTuning_GASIndoorAlpha_front;
	} else if (addr == sysfs_finfo->grasTuning_GASIndoorBeta_N_addr) {
		data_size = FIMC_IS_COMPANION_GRASTUNING_GASINDOORBETA_SIZE;
		if (position == SENSOR_POSITION_REAR)
			comp_addr = grasTuning_GASIndoorBeta;
		else
			comp_addr = grasTuning_GASIndoorBeta_front;
	} else if (addr == sysfs_finfo->coef_offset_R) {
		    data_size = FIMC_IS_COMPANION_COEF_OFFSET_R_SIZE;
		    comp_addr = dmcTuningParams_xtalk_coeff_offset_r;
	} else if (addr == sysfs_finfo->coef_offset_G) {
		    data_size = FIMC_IS_COMPANION_COEF_OFFSET_G_SIZE;
		    comp_addr = dmcTuningParams_xtalk_coeff_offset_g;
	} else if (addr == sysfs_finfo->coef_offset_B) {
		    data_size = FIMC_IS_COMPANION_COEF_OFFSET_B_SIZE;
		    comp_addr = dmcTuningParams_xtalk_coeff_offset_b;
	} else {
		err("wrong companion cal data name\n");
		return -EINVAL;
	}
	data1 = comp_addr >> 16;
	data2 = (u16)comp_addr;

	ret = fimc_is_comp_i2c_write(core->client0, 0x6028, data1);
	if (ret) {
		err("fimc_is_comp_load_i2c_cal() i2c write fail");
	}
	ret = fimc_is_comp_i2c_write(core->client0, 0x602A, data2);
	if (ret) {
		err("fimc_is_comp_load_i2c_cal() i2c write fail");
	}

	buf = cal_buf + addr;
	if (addr == sysfs_finfo->coef_offset_R
		|| addr == sysfs_finfo->coef_offset_G
		|| addr == sysfs_finfo->coef_offset_B) {
		if (companion_coef_isvalid) {
			for (i = 0; i < data_size; i += 2) {
				data3 =	(*(buf + i + 1)) | (*(buf + i) << 8);
				ret = fimc_is_comp_i2c_write(core->client0, 0x6F12, data3);
				if (ret) {
					err("fimc_is_comp_i2c_setf_write() fail");
					break;
				}
				data2 += 2;
			}
		} else {
			ret = fimc_is_comp_i2c_write(core->client0, 0x6F12, 0x0203);
			ret |= fimc_is_comp_i2c_write(core->client0, 0x6F12, 0xB902);
			ret |= fimc_is_comp_i2c_write(core->client0, 0x6F12, 0xA903);
			ret |= fimc_is_comp_i2c_write(core->client0, 0x6F12, 0x2900);
			ret |= fimc_is_comp_i2c_write(core->client0, 0x6F12, 0x4900);
			if (ret) {
				err("fimc_is_comp_i2c_setf_write() fail");
			}
		}
	} else {
		if ((companion_lsc_isvalid && position == SENSOR_POSITION_REAR)
			|| (companion_front_lsc_isvalid && position == SENSOR_POSITION_FRONT)) {
			if ((addr == sysfs_finfo->lsc_a_gain_addr) || (addr == sysfs_finfo->lsc_k4_gain_addr)) {
				for (i = 0; i < data_size; i += 4) {
					data3 =	(*(buf + i + 1) << 8) | (*(buf + i));
					data4 =	(*(buf + i + 3) << 8) | (*(buf + i + 2));
					ret = fimc_is_comp_i2c_write(core->client0, 0x6F12, data4);
					ret = fimc_is_comp_i2c_write(core->client0, 0x6F12, data3);
					if (ret) {
						err("fimc_is_comp_i2c_setf_write() fail");
						break;
					}
					data2 += 4;
				}
			} else {
				for (i = 0; i < data_size; i += 2) {
					data3 =	(*(buf + i + 1) << 8) | (*(buf + i));
					ret = fimc_is_comp_i2c_write(core->client0, 0x6F12, data3);
					if (ret) {
						err("fimc_is_comp_i2c_setf_write() fail");
						break;
					}
					data2 += 2;
				}
			}
		}
	}
	return ret;
}

#ifdef CONFIG_COMPANION_DCDC_USE
int fimc_is_power_binning(void *core_data)
{
	struct fimc_is_core *core = core_data;
	struct fimc_is_vender_specific *specific = core->vender.private_data;
	struct dcdc_power *dcdc = &specific->companion_dcdc;
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

	ret = fimc_is_comp_spi_single_write(core->spi1.device, 0x642E, 0x5002);
	if (ret) {
		err_check = 1;
		err("spi_single_write() fail");
	}

	ret = fimc_is_comp_single_read(core, 0x6F12, &read_value);
	if (ret) {
		err_check = 1;
		err("fimc_is_comp_single_read() fail");
	}

	info("[%s::%d][BIN_INFO::0x%04x]\n", __FUNCTION__, __LINE__, read_value);

	if (read_value & 0x3F) {
		if (read_value & (1 << CC_BIN7)) {
			buf[0]='7';
		} else if (read_value & (1 << CC_BIN6)) {
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

	if (dcdc->type != DCDC_VENDOR_PMIC) {
		vout = dcdc->get_vout_val(vout_sel);
		ret = dcdc->set_vout(dcdc->client, vout);
		if (ret) {
			err("dcdc set_vout(%d), sel %d", ret, vout_sel);
		} else {
			info("%s: DCDC %s, sel %d %sV\n", __func__,
				dcdc->name, vout_sel, dcdc->get_vout_str(vout_sel));
		}
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

	/* check validation(Read data must be 0x73C2) */
	fimc_is_comp_i2c_read(core->client0, 0x0, &companion_id);
	info("Companion validation: 0x%04x\n", companion_id);

exit:
	return ret;
}

int fimc_is_comp_read_ver(void *core_data)
{
	struct fimc_is_core *core = core_data;
	int ret = 0;

	if (!core->client0) {
		info("i2c device is not available\n");
		goto p_err;
	}

	companion_ver = 0;
	/* check companion version(EVT0, EVT1) */
	fimc_is_comp_i2c_read(core->client0, 0x02, &companion_ver);
	info("%s_c2: Companion version: 0x%04x\n", __func__, companion_ver);

p_err:
	return ret;
}

u16 fimc_is_comp_get_ver(void)
{
	return companion_ver;
}

int fimc_is_comp_loadcal(void *core_data, int position)
{
	struct fimc_is_core *core = core_data;
	struct fimc_is_rom_info *sysfs_finfo;
	int ret = 0;
	struct exynos_platform_fimc_is *core_pdata = NULL;

	core_pdata = dev_get_platdata(fimc_is_dev);
	if (!core_pdata) {
		err("core->pdata is null");
		goto p_err;
	}

	if (!core->spi1.device) {
		pr_debug("spi1 device is not available");
		goto p_err;
	}

	if (!fimc_is_sec_check_rom_ver(core, position)) {
		err("%s: error, not implemented! skip..", __func__);
		return 0;
	}

	if ((!crc32_c1_check && (position == SENSOR_POSITION_REAR))
	|| (!crc32_c1_check_front && (position == SENSOR_POSITION_FRONT))) {
		pr_debug("CRC check fail.Do not apply cal data.");
		return 0;
	}

	if(position == SENSOR_POSITION_REAR) {
		pdaf_valid = false;
		pdaf_shad_valid = false;
		fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	} else {
		fimc_is_sec_get_sysfs_finfo_front(&sysfs_finfo);
	}

	if (fimc_is_sec_check_rom_ver(core, position)) {
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_i0_gain_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_i0_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_j0_gain_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail",sysfs_finfo->lsc_j0_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_a_gain_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_a_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_k4_gain_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_k4_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->lsc_scale_gain_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->lsc_scale_gain_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_AwbAshCord_N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_AwbAshCord_N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_awbAshCordIndexes_N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_awbAshCordIndexes_N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_GASAlpha_M__N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_GASAlpha_M__N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_GASBeta_M__N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_GASBeta_M__N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_GASOutdoorAlpha_N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_GASOutdoorAlpha_N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_GASOutdoorBeta_N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_GASOutdoorBeta_N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_GASIndoorAlpha_N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_GASIndoorAlpha_N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);
		ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->grasTuning_GASIndoorBeta_N_addr, position);
		if (ret) {
			err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->grasTuning_GASIndoorBeta_N_addr);
			goto p_err;
		}
		usleep_range(1000, 1000);

		if(position == SENSOR_POSITION_REAR)
		{
			ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->coef_offset_R, position);
			if (ret) {
				err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->coef_offset_R);
				goto p_err;
			}
			usleep_range(1000, 1000);
			ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->coef_offset_G, position);
			if (ret) {
				err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->coef_offset_G);
				goto p_err;
			}
			usleep_range(1000, 1000);
			ret = fimc_is_comp_load_i2c_cal(core, sysfs_finfo->coef_offset_B, position);
			if (ret) {
				err("fimc_is_comp_load_binary(0x%04x) fail", sysfs_finfo->coef_offset_B);
				goto p_err;
			}
			usleep_range(1000, 1000);
		}
	}

	if (fimc_is_sec_check_rom_ver(core, position)) {
		if ((companion_lsc_isvalid && position == SENSOR_POSITION_REAR)
		||(companion_front_lsc_isvalid && position == SENSOR_POSITION_FRONT)) {
			ret = fimc_is_comp_load_cal(core, COMP_LSC, position);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_LSC);
				goto p_err;
			}
			info("LSC from FROM loaded\n");
		}
#ifdef USE_DEFAULT_CAL
		else {
			ret = fimc_is_comp_load_binary(core, COMP_DEFAULT_LSC, COMPANION_CAL);
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
	if(position == SENSOR_POSITION_REAR) {
		if (fimc_is_sec_check_rom_ver(core, position)) {
			ret = fimc_is_comp_load_cal(core, COMP_PDAF, position);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_PDAF);
				goto p_err;
			}
			pdaf_valid = true;
			usleep_range(1000, 1000);

			ret = fimc_is_comp_load_cal(core, COMP_PDAF_SHAD, position);
			if (ret) {
				err("fimc_is_comp_load_binary(%s) fail", COMP_PDAF_SHAD);
				goto p_err;
			}
			pdaf_shad_valid = true;
			usleep_range(1000, 1000);

			if (companion_coef_isvalid) {
				ret = fimc_is_comp_load_cal(core, COMP_COEF_CAL, position);
				if (ret) {
					err("fimc_is_comp_load_binary(%s) fail", COMP_COEF_CAL);
					goto p_err;
				}
				info("COEF from FROM loaded\n");
			}
#ifdef USE_DEFAULT_CAL
			else {
				ret = fimc_is_comp_load_binary(core, COMP_DEFAULT_COEF, COMPANION_CAL);
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
	}

	usleep_range(5000, 5000);

p_err:
	return 0;
}

#ifdef CONFIG_PREPROCESSOR_STANDBY_USE
int fimc_is_comp_retention(void *core_data)
{
	int ret = 0;
	struct fimc_is_core *core = core_data;
	struct exynos_platform_fimc_is *core_pdata = NULL;
	struct fimc_is_companion_retention *ret_data;
	static char fw_name[100];
	struct file *fp = NULL;
#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) || defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	char signature[2];
#endif
	mm_segment_t old_fs;
	struct fimc_is_vender_specific *specific = core->vender.private_data;

	info("%s: start\n", __func__);

	core_pdata = dev_get_platdata(fimc_is_dev);
	if (!core_pdata) {
		err("core->pdata is null");
	}

	if (!core->spi1.device) {
		err("spi1 device is not available");
		goto p_err;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(fw_name, sizeof(fw_name), "%s%s", FIMC_IS_SETFILE_SDCARD_PATH, COMP_FW_SDCARD);

	fp = filp_open(fw_name, O_RDONLY, 0);
	if (!IS_ERR_OR_NULL(fp)) {
		filp_close(fp, current->files);
		set_fs(old_fs);
		err("firmware file is on SDCARD! retention mode does not using(%s)", fw_name);
		return -EINVAL;
	}
	set_fs(old_fs);

	ret_data = &specific->retention_data;
#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) || defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	signature[0] = (FIMC_IS_COMPANION_SIGNATURE_1ST_VALUE & 0x00ff);
	signature[1] = (FIMC_IS_COMPANION_SIGNATURE_1ST_VALUE & 0xff00) >> 8;
	ret = fimc_is_comp_check_signature(core,
			FIMC_IS_COMPANION_SIGNATURE_ADDR,
			signature);
	if (ret) {
		err("check signature fail. (%d)", FIMC_IS_COMPANION_SIGNATURE_1ST_VALUE);
	}
#else
	fimc_is_comp_is_valid(core);
	usleep_range(1000, 1200);
#endif

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	ret = fimc_is_comp_check_crc_with_signature(core);
	if (ret) {
		err("fimc_is_comp_check_crc_with_signature fail");
		ret = -EINVAL;
		goto p_err;
	}
	info("signature crc check success!\n");
#endif

#ifdef CONFIG_COMPANION_STANDBY_CRC
	ret = fimc_is_comp_single_write(core, 0x0008, 0x0001);
	if (ret) {
		err("initialization for CRC Checking write fail");
	}
	usleep_range(1000, 1000);

#if !defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) && !defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	ret = fimc_is_comp_check_crc32(core,
			FIMC_IS_COMPANION_FW_START_ADDR,
			ret_data->firmware_size - FIMC_IS_COMPANION_CRC_SIZE,
			ret_data->firmware_crc32);

	if (ret) {
		err("fimc_is_comp_check_crc32(firmware) fail");
		ret = -EINVAL;
		goto p_err;
	}

	if (fimc_is_sec_check_rom_ver(core, core->current_position)) {
		ret = fimc_is_comp_check_cal_crc(core);
		if (ret) {
			err("fimc_is_comp_check_crc32(cal) fail");
			ret = -EINVAL;
			goto p_err;
		}
	}
#endif
#endif

	/* ARM Reset & Memory Remap */
	ret = fimc_is_comp_single_write(core, 0x6048, 0x0001);
	if (ret) {
		err("ARM Reset & Memory Remap write fail");
	}

#if !defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) && !defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	/* Clear Signature */
	ret = fimc_is_comp_single_write(core, 0x0000, 0xBEEF);
	if (ret) {
		err("Clear Signature write fail");
	}
#endif

	/* ARM Go */
	ret = fimc_is_comp_single_write(core, 0x6014, 0x0001);
	if (ret) {
		err("ARM Go write fail");
	}

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN) || defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN)
	usleep_range(10000, 10000);
#elif defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_CLOSE)
	usleep_range(1000, 1000);
#endif
	signature[0] = (FIMC_IS_COMPANION_SIGNATURE_2ND_VALUE & 0x00ff);
	signature[1] = (FIMC_IS_COMPANION_SIGNATURE_2ND_VALUE & 0xff00) >> 8;
	ret = fimc_is_comp_check_signature(core,
			FIMC_IS_COMPANION_SIGNATURE_ADDR,
			signature);
	if (ret) {
		err("check signature fail. (%d)", FIMC_IS_COMPANION_SIGNATURE_2ND_VALUE);
	}

#if defined(CONFIG_COMPANION_AUTOMATIC_CRC_ON_OPEN)
	ret = fimc_is_comp_check_crc_with_signature(core);
	if (ret) {
		err("fimc_is_comp_check_crc_with_signature fail");
		ret = -EINVAL;
		goto p_err;
	}
	info("signature crc check success!\n");
#endif
#else
	usleep_range(1000, 1000);

	ret = fimc_is_comp_read_ver(core);
	if (ret) {
		err("fimc_is_comp_read_ver fail");
	}
#endif

	/* Use M2M Cal Data */
	if(fimc_is_sec_check_rom_ver(core, core->current_position)) {
		if ((companion_lsc_isvalid && companion_coef_isvalid && core->current_position == SENSOR_POSITION_REAR)
			||(companion_front_lsc_isvalid && core->current_position == SENSOR_POSITION_FRONT)) {
			ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR, 0x0001);
		} else {
			err("Companion cal data is not valid. Does not use M2M cal Data.)");
			ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR, 0x0000);
		}
	} else {
		err("Camera module is not valid. Does not use M2M cal Data.)");
		ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR, 0x0000);
	}
	if (ret) {
		err("Use M2M Cal Data write fail");
	}

p_err:
	return ret;
}
#endif

int fimc_is_comp_loadfirm(void *core_data)
{
	int ret = 0;
	int retry_count = 3;
	struct fimc_is_core *core = core_data;
	struct exynos_platform_fimc_is *core_pdata = NULL;
	struct fimc_is_rom_info *sysfs_finfo;
	struct fimc_is_companion_retention *ret_data;
	struct fimc_is_vender_specific *specific;

	core_pdata = dev_get_platdata(fimc_is_dev);
	if (!core_pdata) {
		err("core->pdata is null");
	}

	if (!core->spi1.device) {
		err("spi1 device is not available");
		goto p_err;
	}

	specific = core->vender.private_data;
	ret_data = &specific->retention_data;
	fimc_is_sec_get_sysfs_finfo(&sysfs_finfo);
	fimc_is_comp_is_valid(core);

retry:
	ret = fimc_is_comp_single_write(core, 0x0008, 0x0001);
	if (ret) {
		err("Initialization for download write fail");
	}
	usleep_range(1000, 1000);

	fimc_is_comp_read_ver(core);

	ret = fimc_is_comp_single_write(core, 0x6428, 0x0008);
	if (ret) {
		err("Upload firmware into RAM write(0x6428) fail");
	}

	ret = fimc_is_comp_single_write(core, 0x642A, 0x0000);
	if (ret) {
		err("Upload firmware into RAM write(0x642A) fail");
	}

	if (companion_ver == FIMC_IS_COMPANION_VERSION_EVT0) {
		if (fimc_is_sec_fw_module_compare(sysfs_finfo->header_ver, FW_2T2)) {
			snprintf(sysfs_finfo->load_fw_name, sizeof(FIMC_IS_FW_2T2_EVT0), "%s", FIMC_IS_FW_2T2_EVT0);
		}
		ret = fimc_is_comp_load_binary(core, COMP_FW_EVT0, COMPANION_FW);
	} else if (companion_ver == FIMC_IS_COMPANION_VERSION_EVT1) {
		ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_fw_name, COMPANION_FW);
	} else {
		err("Invalid companion version. Loading EVT1 Firmware.");
		ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_fw_name, COMPANION_FW);
	}
	if (ret) {
		err("fimc_is_comp_load_firmware fail");
		goto p_err;
	}

	ret = fimc_is_comp_loadcal(core, SENSOR_POSITION_REAR);
#ifdef CONFIG_CAMERA_EEPROM_SUPPORT_FRONT
	ret |= fimc_is_comp_loadcal(core, SENSOR_POSITION_FRONT);
#endif
	if (ret) {
		err("fimc_is_comp_loadcal() fail");
		goto p_err;
	}

	ret = fimc_is_comp_check_crc32(core,
			FIMC_IS_COMPANION_FW_START_ADDR,
			ret_data->firmware_size - FIMC_IS_COMPANION_CRC_SIZE,
			ret_data->firmware_crc32);

	if (fimc_is_sec_check_rom_ver(core, core->current_position)) {
		ret |= fimc_is_comp_check_cal_crc(core);
	}

	if ((ret < 0) && (retry_count > 0)) {
		retry_count--;
		goto retry;
	}

	/* ARM Reset & Memory Remap */
	ret = fimc_is_comp_single_write(core, 0x6048, 0x0001);
	if (ret) {
		err("ARM Reset & Memory Remap write fail");
	}

	/* Clear Signature */
	ret = fimc_is_comp_single_write(core, 0x0000, 0xBEEF);
	if (ret) {
		err("Clear Signature write fail");
	}

	/* ARM Go */
	ret = fimc_is_comp_single_write(core, 0x6014, 0x0001);
	if (ret) {
		err("ARM Go write fail");
	}

	usleep_range(1000, 1000);

	/* Use M2M Cal Data */
	if(fimc_is_sec_check_rom_ver(core, core->current_position)) {
		if ((companion_lsc_isvalid && companion_coef_isvalid && core->current_position == SENSOR_POSITION_REAR)
			||(companion_front_lsc_isvalid && core->current_position == SENSOR_POSITION_FRONT)) {
			ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR, 0x0001);
		} else {
			err("Companion cal data is not valid. Does not use M2M cal Data.)");
			ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR, 0x0000);
		}
	} else {
		err("Camera module is not valid. Does not use M2M cal Data.)");
		ret = fimc_is_comp_single_write(core, FIMC_IS_COMPANION_USING_M2M_CAL_DATA_ADDR, 0x0000);
	}
	if (ret) {
		err("Use M2M Cal Data write fail");
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

	ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_mastersetf_name, COMPANION_MASTER);
	if (ret) {
		err("fimc_is_comp_load_binary(%s) fail", sysfs_finfo->load_c1_mastersetf_name);
		goto p_err;
	}

	usleep_range(1000, 1000);

	ret = fimc_is_comp_load_binary(core, sysfs_finfo->load_c1_modesetf_name, COMPANION_MODE);
	if (ret) {
		err("fimc_is_comp_load_binary(%s) fail", sysfs_finfo->load_c1_modesetf_name);
		goto p_err;
	}

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
#ifdef ENABLE_IS_CORE
	struct fimc_is_core *core = core_data;
	u32 cfg = ~0x0;

	if(on) {
		cfg &= ~ch;
		writel(cfg, core->regs + INTMR2);
	} else {
		cfg = 0xFFFFFFFF;
		writel(cfg, core->regs + INTMR2);
	}
#endif
}

#ifndef CONFIG_COMPANION_DCDC_USE
/* Temporary Fixes. Set voltage to 0.85V for EVT0, 0.8V for EVT1 */
int fimc_is_comp_set_voltage(char *volt_name, int uV)
{
	struct regulator *regulator = NULL;
	static u32 cnt_fail = 0;
	int ret;

	regulator = regulator_get(NULL, volt_name);
	if (IS_ERR_OR_NULL(regulator)) {
		err("regulator_get fail\n");
		regulator_put(regulator);
		return -EINVAL;
	}

	ret = regulator_set_voltage(regulator, uV, uV);
	if(ret) {
		err("regulator_set_voltage(%d) fail. Fail count %u\n", ret, ++cnt_fail);
	}

	regulator_put(regulator);
	return ret;
}
#endif
