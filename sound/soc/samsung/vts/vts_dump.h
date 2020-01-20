/* sound/soc/samsung/vts/vts_dump.h
 *
 * ALSA SoC - Samsung vts dump driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_VTS_DUMP_H
#define __SND_SOC_VTS_DUMP_H

#include <linux/device.h>
#include "vts.h"

#define VTS_ADUIODUMP_AFTER_MINS	2 // VTS will dump 4.4 sec data after every 2 minutes
#define VTS_LOGDUMP_AFTER_MINS		1 // VTS will dump available log after every 1 minute

enum vts_dump_mode {
	VTS_AUDIO_DUMP = 0,
	VTS_LOG_DUMP = 1,
};

/**
 * Schedule pcm dump from sram memory to file
 * @param[in]	dev		pointer to vts device
 * @param[in]	addroffset	SRAM offset for PCM dta
 * @param[in]	size		size of pcm data
 */
extern void vts_audiodump_schedule_flush(struct device *dev);
extern void vts_logdump_schedule_flush(struct device *dev);
extern void vts_dump_addr_register(
	struct device *dev,
	u32 addroffset,
	u32 dumpsz,
	u32 dump_mode);

#endif /* __SND_SOC_VTS_DUMP_H */
