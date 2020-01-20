/* sound/soc/samsung/abox/abox_dump.h
 *
 * ALSA SoC Audio Layer - Samsung Abox Internal Buffer Dumping driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_DUMP_H
#define __SND_SOC_ABOX_DUMP_H

#include <linux/device.h>
#include <sound/samsung/abox.h>

/**
 * Report dump data written
 * @param[in]	id		unique buffer id
 * @param[in]	pointer		byte index of the written data
 */
extern void abox_dump_period_elapsed(int id, size_t pointer);

/**
 * Transfer dump data
 * @param[in]	id		unique buffer id
 * @param[in]	buf		start of the trasferring buffer
 * @param[in]	bytes		number of bytes
 */
extern void abox_dump_transfer(int id, const char *buf, size_t bytes);

/**
 * Register abox dump buffer
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		unique buffer id
 * @param[in]	name		unique buffer name
 * @param[in]	area		virtual address of the buffer
 * @param[in]	addr		pysical address of the buffer
 * @param[in]	bytes		buffer size in bytes
 * @return	error code if any
 */
extern int abox_dump_register_buffer(struct device *dev, int id,
		const char *name, void *area, phys_addr_t addr, size_t bytes);

/**
 * Initialize abox dump module
 * @param[in]	dev		pointer to abox device
 */
extern void abox_dump_init(struct device *dev_abox);
#endif /* __SND_SOC_ABOX_DUMP_H */
