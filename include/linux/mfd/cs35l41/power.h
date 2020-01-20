/*
 * power.h - Power-management defines for Cirrus Logic CS35L41 amplifier
 *
 * Copyright 2018 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define CIRRUS_PWR_CSPL_PASSPORT_ENABLE		0x2800448
#define CIRRUS_PWR_CSPL_OUTPUT_POWER_SQ		0x280044c

void cirrus_pwr_start(bool right_amp);
void cirrus_pwr_stop(bool right_amp);
int cirrus_pwr_amp_add(struct regmap *regmap_new, bool right_channel_amp);
int cirrus_pwr_set_params(bool global_enable, bool right_channel_amp,
			unsigned int target_temp, unsigned int exit_temp);
int cirrus_pwr_init(struct class *cirrus_amp_class);
void cirrus_pwr_exit(void);
