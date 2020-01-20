/*
 * Extended support for CS35L41 Amp
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:      David Rhodes    <david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/calibration.h>
#include <linux/mfd/cs35l41/big_data.h>
#include <linux/mfd/cs35l41/power.h>

#define CIRRUS_AMP_CLASS_NAME "cirrus"

struct class *cirrus_amp_class;

int __init cirrus_amp_init(void)
{
	cirrus_amp_class = class_create(THIS_MODULE,
						CIRRUS_AMP_CLASS_NAME);
        if (IS_ERR(cirrus_amp_class)) {
                pr_err("Failed to register cirrus amp class\n");
                return -EINVAL;
        }

	cirrus_cal_init(cirrus_amp_class);
	cirrus_bd_init(cirrus_amp_class);
	cirrus_pwr_init(cirrus_amp_class);

	return 0;
}

void __exit cirrus_amp_exit(void)
{
	cirrus_cal_exit();
	cirrus_bd_exit();
	cirrus_pwr_exit();
}

module_init(cirrus_amp_init);
module_exit(cirrus_amp_exit);

MODULE_AUTHOR("David Rhodes <David.Rhodes@cirrus.com>");
MODULE_DESCRIPTION("Cirrus Amp driver");
MODULE_LICENSE("GPL");

