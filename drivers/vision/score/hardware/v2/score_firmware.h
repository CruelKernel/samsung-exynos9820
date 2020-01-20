/*
 * Samsung Exynos SoC series SCore driver
 *
 * Firmware manager module for firmware binary that SCore device executes
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_FIRMWARE_H__
#define __SCORE_FIRMWARE_H__

#include <linux/device.h>
#include <linux/types.h>

struct score_system;

#define SCORE_FW_PATH_LEN		(64)

enum score_firmware_id {
	SCORE_FIRMWARE_DEVELOP1,
	SCORE_FIRMWARE_DEVELOP2,
	SCORE_FIRMWARE_DEVELOP3,
	SCORE_FIRMWARE_CAMERA1,
	SCORE_FIRMWARE_CAMERA2,
	SCORE_FIRMWARE_CAMERA3,
	SCORE_FIRMWARE_CAMERA4,
	SCORE_FIRMWARE_CAMERA5,
	SCORE_FIRMWARE_CAMERA6,
	SCORE_FIRMWARE_CAMERA7,
	SCORE_FIRMWARE_CAMERA8,
	SCORE_FIRMWARE_CAMERA9,
	SCORE_FIRMWARE_END
};

/**
 * enum score_core_id - Unique id of three core included in SCore device
 * @SCORE_MASTER: master core id - 0
 * @SCORE_KNIGHT1: knight1 core id - 1
 * @SCORE_KNIGHT2: knight2 core id - 2
 * @SCORE_CORE_NUM: the number of core included in SCore device
 */
enum score_core_id {
	SCORE_MASTER = 0,
	SCORE_KNIGHT1,
	SCORE_KNIGHT2,
	SCORE_CORE_NUM
};

struct score_firmware_info {
	const char			*text_path;
	size_t				text_mem_size;

	const char			*data_path;
	size_t				data_mem_size;
	size_t				data_stack;
};

struct score_firmware_format {
	const char			*name;
	unsigned int			id;

	unsigned int			default_qos;
	bool				fw_load;
	struct score_firmware_info	mc;
	struct score_firmware_info	kc1;
	struct score_firmware_info	kc2;
	bool				duplication;
};


/**
 * struct score_firmware_bin -
 * @path: name of pm binary
 * @mem_size: size of memory allocated for pm binary
 * @fw_size: size of pm binary
 * @kvaddr: kernel virtual address of memory allocated for pm binary
 * @dvaddr: device virtual address of memory allocated for pm binary
 * @stack:
 */
struct score_firmware_bin {
	char				path[SCORE_FW_PATH_LEN];
	size_t				mem_size;
	size_t				fw_size;
	void				*kvaddr;
	dma_addr_t			dvaddr;
	size_t				stack;
};

/**
 * struct score_firmware - Object including firmware information
 * @dev: device structure registered in platform
 * @idx: core id
 * @fw_loaded: state that firmware is loaded(true) or not(false)
 * @text: information of pm firmware
 * @data: information of dm firmware
 */
struct score_firmware {
	struct device			*dev;
	int				idx;
	bool				fw_loaded;

	struct score_firmware_bin	text;
	struct score_firmware_bin	data;
};

/**
 * struct score_firmware_manager -
 *	Manager controlling information of each firmware
 * @master: score_firmware structure for master core
 * @knight1: score_firmware structure for knight1 core
 * @knight2: score_firmware structure for knight2 core
 */
struct score_firmware_manager {
	struct score_firmware		master;
	struct score_firmware		knight1;
	struct score_firmware		knight2;
};

int score_firmware_manager_get_dvaddr(struct score_firmware_manager *fwmgr,
		int core_id, dma_addr_t *text_addr, dma_addr_t *data_addr);
int score_firmware_load(struct score_firmware *fw);
int score_firmware_copy(struct score_firmware *dst, struct score_firmware *src);

int score_firmware_manager_open(struct score_firmware_manager *fwmgr,
		unsigned int firmware_id, void *kvaddr, dma_addr_t dvaddr);
void score_firmware_manager_close(struct score_firmware_manager *fwmgr);

int score_firmware_manager_probe(struct score_system *system);
void score_firmware_manager_remove(struct score_firmware_manager *fwmgr);

#endif
