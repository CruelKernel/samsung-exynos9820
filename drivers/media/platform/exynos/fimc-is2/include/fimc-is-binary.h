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

#ifndef FIMC_IS_BINARY_H
#define FIMC_IS_BINARY_H

#include "fimc-is-config.h"

#ifdef VENDER_PATH
#define FIMC_IS_FW_PATH 			"/system/vendor/firmware/"
#define FIMC_IS_FW_DUMP_PATH			"/data/vendor/camera/"
#define FIMC_IS_SETFILE_SDCARD_PATH		"/data/vendor/camera/"
#define FIMC_IS_FW_SDCARD			"/data/vendor/camera/fimc_is_fw.bin"
#define FIMC_IS_FW				"fimc_is_fw.bin"
#define FIMC_IS_LIB_PATH			"/system/vendor/firmware/"
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
#define FIMC_IS_ISP_LIB_SDCARD_PATH		NULL
#else
#define FIMC_IS_ISP_LIB_SDCARD_PATH		"/data/vendor/camera/"
#endif
#define FIMC_IS_REAR_CAL_SDCARD_PATH		"/data/vendor/camera/"
#define FIMC_IS_FRONT_CAL_SDCARD_PATH		"/data/media/0/"
#else
#define FIMC_IS_FW_PATH 			"/system/vendor/firmware/"
#define FIMC_IS_FW_DUMP_PATH			"/data/"
#define FIMC_IS_SETFILE_SDCARD_PATH		"/data/"
#define FIMC_IS_FW_SDCARD			"/data/fimc_is_fw2.bin"
#define FIMC_IS_FW				"fimc_is_fw2.bin"
#define FIMC_IS_ISP_LIB_SDCARD_PATH		"/data/"
#define FIMC_IS_REAR_CAL_SDCARD_PATH		"/data/"
#define FIMC_IS_FRONT_CAL_SDCARD_PATH		"/data/"
#endif

#ifdef USE_ONE_BINARY
#define FIMC_IS_ISP_LIB				"fimc_is_lib.bin"
#else
#define FIMC_IS_ISP_LIB				"fimc_is_lib_isp.bin"
#define FIMC_IS_VRA_LIB				"fimc_is_lib_vra.bin"
#endif

#define FIMC_IS_RTA_LIB				"fimc_is_rta.bin"

#define FD_SW_BIN_NAME				"fimc_is_fd.bin"
#define FD_SW_SDCARD				"/data/fimc_is_fd.bin"

#ifdef ENABLE_IS_CORE
#define FW_MEM_SIZE			0x02000000
#define FW_BACKUP_SIZE			0x02000000
#define PARAM_REGION_SIZE		0x00005000
#define SHARED_OFFSET			0x01FC0000
#define SHARED_SIZE			0x00010000
#define TTB_OFFSET			0x01BF8000
#define TTB_SIZE			0x00008000
#define DEBUG_REGION_OFFSET		0x01F40000
#define DEBUG_REGION_SIZE		0x0007D000 /* 500KB */
#define CAL_OFFSET0			0x01FD0000
#define CAL_OFFSET1			0x01FE0000

#define DEBUGCTL_OFFSET			(DEBUG_REGION_OFFSET + DEBUG_REGION_SIZE)
#else /* #ifdef ENABLE_IS_CORE */
/* static reserved memory for libraries */
#define CDH_SIZE		SZ_128K		/* CDH : Camera Debug Helper */

#define LIB_OFFSET		(VMALLOC_START + 0xF6000000 - 0x8000000)
#define __LIB_START		(LIB_OFFSET + 0x04000000 - CDH_SIZE)
#define LIB_START		(__LIB_START)

#define VRA_LIB_ADDR		(LIB_START + CDH_SIZE)
#define VRA_LIB_SIZE		(SZ_512K + SZ_256K)

#define DDK_LIB_ADDR		(LIB_START + VRA_LIB_SIZE + CDH_SIZE)
#define DDK_LIB_SIZE		((SZ_2M + SZ_1M + SZ_256K) + SZ_1M)

#define RTA_LIB_ADDR		(LIB_START + VRA_LIB_SIZE + DDK_LIB_SIZE + CDH_SIZE)
#define RTA_LIB_SIZE		(SZ_2M + SZ_2M)

#ifdef USE_RTA_BINARY
#define LIB_SIZE		(VRA_LIB_SIZE + DDK_LIB_SIZE +  RTA_LIB_SIZE + CDH_SIZE)
#else
#define LIB_SIZE		(VRA_LIB_SIZE + DDK_LIB_SIZE + CDH_SIZE)
#endif

#define HEAP_START		(LIB_START + SZ_16M)
#define HEAP_SIZE		(FIMC_IS_HEAP_SIZE)

/* reserved memory for FIMC-IS */
#define SETFILE_SIZE		(0x0044B000)
#define CALDATA_SIZE		(0x00010000)
#define DEBUG_REGION_SIZE	(0x0007D000)
#define EVENT_REGION_SIZE	(0x0007D000)
#define FSHARED_REGION_SIZE	(0x00010000)
#define DATA_REGION_SIZE	(0x00010000)
#define PARAM_REGION_SIZE	(0x00005000)	/* 20KB * instance(4) */

#define HEAP_RTA_START		(HEAP_START + SZ_64M)		/* HEAP_SIZE(for DDK) should be smaller than 64MB */
#define HEAP_RTA_SIZE		(FIMC_IS_RESERVE_LIB_SIZE)	/* 6MB ~ */
#define TAAISP_DMA_SIZE		(FIMC_IS_TAAISP_SIZE)	/* 512KB */
#define MEDRC_DMA_SIZE		(TAAISP_MEDRC_SIZE)
#define LHFD_MAP_SIZE		(0x009F0000)	/* 9.9375MB */
#define VRA_DMA_SIZE		(FIMC_IS_VRA_SIZE)	/* 8MB */
#define MCSC_DNR_DMA_SIZE	(FIMC_IS_MCSC_DNR_SIZE)

/* for compatibility */
#define DEBUGCTL_OFFSET		(0)
#define DEBUG_REGION_OFFSET	(0)
#define DEBUG_REGION_SIZE	(0x0007D000) /* 500KB */
#define TTB_OFFSET		(0)
#define TTB_SIZE		(0)
#define CAL_OFFSET0		(0x01FD0000)
#define CAL_OFFSET1		(0x01FE0000)

/* EEPROM offset */
#define EEPROM_HEADER_BASE	(0)
#define EEPROM_OEM_BASE		(0x100)
#define EEPROM_AWB_BASE		(0x200)
#define EEPROM_SHADING_BASE	(0x300)
#define EEPROM_PDAF_BASE	(0x0)

/* FROM offset */
#define FROM_HEADER_BASE	(0)
#define FROM_OEM_BASE		(0x1000)
#define FROM_AWB_BASE		(0x2000)
#define FROM_SHADING_BASE	(0x3000)
#define FROM_PDAF_BASE		(0x5000)
#endif

#ifdef USE_BINARY_PADDING_DATA_ADDED
#define IS_SIGNATURE_LEN	528
#else
#define IS_SIGNATURE_LEN	0
#endif

#define LIBRARY_VER_LEN		44
#define LIBRARY_VER_OFS		(LIBRARY_VER_LEN + IS_SIGNATURE_LEN)
#define SETFILE_VER_LEN		52
#define SETFILE_VER_OFS		64

#define FIMC_IS_REAR_CAL		"rear_cal_data.bin"
#define FIMC_IS_FRONT_CAL		"front_cal_data.bin"
#define FIMC_IS_CAL_SDCARD		"/data/cal_data.bin"
#define FIMC_IS_CAL_RETRY_CNT		(2)
#define FIMC_IS_FW_RETRY_CNT		(2)
#define FIMC_IS_CAL_VER_SIZE		(12)

/*
 * binary types for IS
 *
 * library based on type(DDK, RTA)
 * setfile based on sensor positon
 */
enum is_bin_type {
	IS_BIN_LIBRARY,
	IS_BIN_SETFILE,
};

struct is_bin_ver_info {
	enum is_bin_type type;
	unsigned int maxhint;
	unsigned int offset;
	unsigned int length;
	void *s;

	char *(*get_buf)(const struct is_bin_ver_info *info, unsigned int hint);
	unsigned int (*get_name_idx)(unsigned int hint);
};

#define IS_BIN_LIB_HINT_DDK	0
#define IS_BIN_LIB_HINT_RTA	1

struct fimc_is_binary {
	void *data;
	size_t size;

	const struct firmware *fw;

	unsigned long customized;

	/* request_firmware retry */
	unsigned int retry_cnt;
	int	retry_err;

	/* custom memory allocation */
	void *(*alloc)(unsigned long size);
	void (*free)(const void *buf);
};

ssize_t write_data_to_file(char *name, char *buf, size_t count, loff_t *pos);
int get_filesystem_binary(const char *filename, struct fimc_is_binary *bin);
int put_filesystem_binary(const char *filename, struct fimc_is_binary *bin, u32 flags);
void setup_binary_loader(struct fimc_is_binary *bin,
				unsigned int retry_cnt, int retry_err,
				void *(*alloc)(unsigned long size),
				void (*free)(const void *buf));
int request_binary(struct fimc_is_binary *bin, const char *path,
				const char *name, struct device *device);
void release_binary(struct fimc_is_binary *bin);
int was_loaded_by(struct fimc_is_binary *bin);

/* FIXME: void carve_binary_version(enum is_bin_type type, int hint, struct fimc_is_binary *bin); */
int carve_binary_version(enum is_bin_type type, unsigned int hint, void *data, size_t size);
char *get_binary_version(enum is_bin_type type, unsigned int hint);

#endif
