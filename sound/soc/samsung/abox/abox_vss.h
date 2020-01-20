/* sound/soc/samsung/abox/abox_vss.h
 *
 * ALSA SoC - Samsung Abox VSS
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_VSS_H
#define __SND_SOC_ABOX_VSS_H

/**
 * Notify Call start or stop
 * @param[in]	dev		pointer to calling device
 * @param[in]	data		abox_data
 * @param[in]	en		if enable 1, if not 0
 * @return	0 or error code
 */
extern int abox_vss_notify_call(struct device *dev, struct abox_data *data,
		int en);

#endif /* __SND_SOC_ABOX_VSS_H */
