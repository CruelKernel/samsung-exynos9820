/*
 * Pinctrl for Cirrus Logic CS47L92
 *
 * Copyright 2016-2017 Cirrus Logic
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/module.h>

#include <linux/mfd/madera/core.h>

#include "pinctrl-madera.h"

/*
 * The alt func groups are the most commonly used functions we place these at
 * the lower function indexes for convenience, and the less commonly used gpio
 * functions at higher indexes.
 *
 * To stay consistent with the datasheet the function names are the same as
 * the group names for that function's pins
 *
 * Note - all 1 less than in datasheet because these are zero-indexed
 */
static const unsigned int cs47l92_spk1_pins[] = { 2, 3 };
static const unsigned int cs47l92_aif1_pins[] = { 4, 5, 6, 7 };
static const unsigned int cs47l92_aif2_pins[] = { 8, 9, 10, 11 };
static const unsigned int cs47l92_aif3_pins[] = { 12, 13, 14, 15 };

static const struct madera_pin_groups cs47l92_pin_groups[] = {
	{ "pdmspk1", cs47l92_spk1_pins, ARRAY_SIZE(cs47l92_spk1_pins) },
	{ "aif1", cs47l92_aif1_pins, ARRAY_SIZE(cs47l92_aif1_pins) },
	{ "aif2", cs47l92_aif2_pins, ARRAY_SIZE(cs47l92_aif2_pins) },
	{ "aif3", cs47l92_aif3_pins, ARRAY_SIZE(cs47l92_aif3_pins) },
};

const struct madera_pin_chip cs47l92_pin_chip = {
	.n_pins = CS47L92_NUM_GPIOS,
	.pin_groups = cs47l92_pin_groups,
	.n_pin_groups = ARRAY_SIZE(cs47l92_pin_groups),
};

