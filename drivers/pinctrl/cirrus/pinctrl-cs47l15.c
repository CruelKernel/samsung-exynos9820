/*
 * Pinctrl for Cirrus Logic CS47L15
 *
 * Copyright 2018 Cirrus Logic
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
static const unsigned int cs47l15_aif1_pins[] = { 0, 1, 2, 3 };
static const unsigned int cs47l15_aif2_pins[] = { 4, 5, 6, 7 };
static const unsigned int cs47l15_aif3_pins[] = { 8, 9, 10, 11 };
static const unsigned int cs47l15_spk1_pins[] = { 12, 13, 14 };

static const struct madera_pin_groups cs47l15_pin_groups[] = {
	{ "aif1", cs47l15_aif1_pins, ARRAY_SIZE(cs47l15_aif1_pins) },
	{ "aif2", cs47l15_aif2_pins, ARRAY_SIZE(cs47l15_aif2_pins) },
	{ "aif3", cs47l15_aif3_pins, ARRAY_SIZE(cs47l15_aif3_pins) },
	{ "pdmspk1", cs47l15_spk1_pins, ARRAY_SIZE(cs47l15_spk1_pins) },
};

const struct madera_pin_chip cs47l15_pin_chip = {
	.n_pins = CS47L15_NUM_GPIOS,
	.pin_groups = cs47l15_pin_groups,
	.n_pin_groups = ARRAY_SIZE(cs47l15_pin_groups),
};
