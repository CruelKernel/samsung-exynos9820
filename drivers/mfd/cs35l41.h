/*
 * cs35l41.h  --  MFD internals for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#ifndef CS35L41_MFD_H
#define CS35L41_MFD_H

#include <linux/pm.h>
#include <linux/of.h>

#include <linux/mfd/cs35l41/core.h>

int cs35l41_dev_init(struct cs35l41_data *cs35l41);
int cs35l41_dev_exit(struct cs35l41_data *cs35l41);

#endif