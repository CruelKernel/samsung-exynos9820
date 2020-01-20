/*
 * linux/platform_data/cs40l2x.h -- Platform data for
 * CS40L20/CS40L25/CS40L25A/CS40L25B
 *
 * Copyright 2018 Cirrus Logic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CS40L2X_H
#define __CS40L2X_H

struct cs40l2x_platform_data {
	unsigned int boost_ind;
	unsigned int boost_cap;
	unsigned int boost_ipk;
	bool refclk_gpio2;
	unsigned int f0_default;
	unsigned int f0_min;
	unsigned int f0_max;
	unsigned int redc_default;
	unsigned int redc_min;
	unsigned int redc_max;
	unsigned int gpio1_rise_index;
	unsigned int gpio1_fall_index;
	unsigned int gpio1_fall_timeout;
	unsigned int gpio1_mode;
	bool hiber_enable;
};

#endif /* __CS40L2X_H */
