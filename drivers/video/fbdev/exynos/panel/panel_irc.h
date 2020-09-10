/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Jihoon Kim <jihoonn.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_IRC_H__
#define __PANEL_IRC_H__

#include <linux/types.h>
#include <linux/kernel.h>

struct brightness_table;

#define SCALEUP_4					4
#define SCALEUP_5					5
#define SCALEUP_9					9

enum {
	IRC_INTERPOLATION_V1,			// before ha9(crown, b0,...)
	IRC_INTERPOLATION_V2,			// after ha9(b1, b2)
	IRC_INTERPOLATION_MAX
};

struct panel_irc_info {
	u32 irc_version;
	u32 fixed_offset;
	u32 fixed_len;
	u32 dynamic_offset;
	u32 dynamic_len;
	u32 total_len;
	u32 hbm_coef;
	u8 *buffer;
	u8 *ref_tbl;
};

int generate_irc(struct brightness_table *brt_tbl, struct panel_irc_info *info, int brightness);


#endif /* __PANEL_IRC_H__ */
