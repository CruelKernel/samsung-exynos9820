/* sound/soc/samsung/vts/vts_log.h
 *
 * ALSA SoC - Samsung vts Log driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_VTS_LOG_H
#define __SND_SOC_VTS_LOG_H

#include <linux/device.h>
#include "vts.h"

/**
 * Schedule log flush sram memory to kernel memory
 * @param[in]	dev		pointer to vts device
 */
extern void vts_log_schedule_flush(struct device *dev, u32 index);

/**
 * Register abox log buffer
 * @param[in]	dev		pointer to abox device
 * @param[in]	addroffset	Sram log buffer offset
 * @param[in]	logsz		log buffer size
 * @return	error code if any
 */
extern int vts_register_log_buffer(
	struct device *dev,
	u32 addroffset,
	u32 logsz);

#endif /* __SND_SOC_VTS_LOG_H */
