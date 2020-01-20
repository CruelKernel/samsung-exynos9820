/* sound/soc/samsung/abox/abox_log.h
 *
 * ALSA SoC - Samsung Abox Log driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_LOG_H
#define __SND_SOC_ABOX_LOG_H

#include <linux/device.h>
#include <sound/samsung/abox.h>

/**
 * Flush log from all shared memories to kernel memory
 * @param[in]	dev		pointer to abox device
 */
extern void abox_log_flush_all(struct device *dev);

/**
 * Schedule log flush from all shared memories to kernel memory
 * @param[in]	dev		pointer to abox device
 */
extern void abox_log_schedule_flush_all(struct device *dev);

/**
 * drain log and stop scheduling log flush
 * @param[in]	dev		pointer to abox device
 */
extern void abox_log_drain_all(struct device *dev);

/**
 * Flush log from specific shared memory to kernel memory
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		unique buffer id
 */
extern void abox_log_flush_by_id(struct device *dev, int id);

/**
 * Register abox log buffer
 * @param[in]	dev		pointer to abox device
 * @param[in]	id		unique buffer id
 * @param[in]	buffer		pointer to shared buffer
 * @return	error code if any
 */
extern int abox_log_register_buffer(struct device *dev, int id,
		struct ABOX_LOG_BUFFER *buffer);

#endif /* __SND_SOC_ABOX_LOG_H */
