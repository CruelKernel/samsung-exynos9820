/* sound/soc/samsung/abox/abox_dbg.h
 *
 * ALSA SoC - Samsung Abox Debug driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_DEBUG_H
#define __SND_SOC_ABOX_DEBUG_H

#include "abox.h"

enum abox_dbg_dump_src {
	ABOX_DBG_DUMP_KERNEL,
	ABOX_DBG_DUMP_FIRMWARE,
	ABOX_DBG_DUMP_COUNT,
};

/**
 * Initialize abox debug driver
 * @return	dentry of abox debugfs root directory
 */
extern struct dentry *abox_dbg_get_root_dir(void);

/**
 * print gpr into the kmsg from memory
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	addr		pointer to gpr dump
 */
extern void abox_dbg_print_gpr_from_addr(struct device *dev,
		struct abox_data *data, unsigned int *addr);

/**
 * print gpr into the kmsg
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 */
extern void abox_dbg_print_gpr(struct device *dev, struct abox_data *data);

/**
 * dump gpr from memory
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	addr		pointer to gpr dump
 * @param[in]	src		source of the dump request
 * @param[in]	reason		reason description
 */
extern void abox_dbg_dump_gpr_from_addr(struct device *dev, unsigned int *addr,
		enum abox_dbg_dump_src src, const char *reason);

/**
 * dump gpr
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	src		source of the dump request
 * @param[in]	reason		reason description
 */
extern void abox_dbg_dump_gpr(struct device *dev, struct abox_data *data,
		enum abox_dbg_dump_src src, const char *reason);

/**
 * dump memory
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	src		source of the dump request
 * @param[in]	reason		reason description
 */
extern void abox_dbg_dump_mem(struct device *dev, struct abox_data *data,
		enum abox_dbg_dump_src src, const char *reason);

/**
 * dump gpr and memory
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	src		source of the dump request
 * @param[in]	reason		reason description
 */
extern void abox_dbg_dump_gpr_mem(struct device *dev, struct abox_data *data,
		enum abox_dbg_dump_src src, const char *reason);

/**
 * dump gpr and memory except DRAM
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	reason		reason description
 */
extern void abox_dbg_dump_simple(struct device *dev, struct abox_data *data,
		const char *reason);

/**
 * dump gpr and memory except DRAM just before abox suspend
 * @param[in]	dev		pointer to device which invokes this API
 * @param[in]	data		pointer to abox_data structure
 */
extern void abox_dbg_dump_suspend(struct device *dev, struct abox_data *data);

/**
 * Push status of the abox
 * @param[in]	dev		pointer to abox device
 * @param[in]	ok		true for okay, false on otherwise
 */
extern void abox_dbg_report_status(struct device *dev, bool ok);

#endif /* __SND_SOC_ABOX_DEBUG_H */
