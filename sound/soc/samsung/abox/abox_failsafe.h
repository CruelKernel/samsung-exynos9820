/* sound/soc/samsung/abox/abox_failsafe.h
 *
 * ALSA SoC - Samsung Abox Fail-safe driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_FAILSAFE_H
#define __SND_SOC_ABOX_FAILSAFE_H

#include <linux/device.h>
#include <sound/samsung/abox.h>

/**
 * Report failure to user space
 * @param[in]	dev		pointer to abox device
 */
extern void abox_failsafe_report(struct device *dev);

/**
 * Report reset
 * @param[in]	dev		pointer to abox device
 */
extern void abox_failsafe_report_reset(struct device *dev);

/**
 * Register abox fail-safe
 * @param[in]	dev		pointer to abox device
 */
extern void abox_failsafe_init(struct device *dev);

#endif /* __SND_SOC_ABOX_FAILSAFE_H */
