/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_DIMMING_H__
#define __PANEL_DIMMING_H__
#include "dimming.h"
#include "panel.h"

struct panel_dimming_info {
	char *name;
	struct dimming_init_info dim_init_info;
	struct dimming_info *dim_info;
	s32 extend_hbm_target_luminance;
	s32 nr_extend_hbm_luminance;
	s32 hbm_target_luminance;
	s32 nr_hbm_luminance;
	s32 target_luminance;
	s32 nr_luminance;
	struct brightness_table *brt_tbl;
	struct maptbl *dimming_maptbl;
	bool dim_flash_on;
	s32 *dim_flash_gamma_offset;
	struct panel_irc_info *irc_info;
};

enum {
	DIMMING_GAMMA_MAPTBL,
	DIMMING_AOR_MAPTBL,
	DIMMING_VINT_MAPTBL,
	DIMMING_ELVSS_MAPTBL,
	DIMMING_IRC_MAPTBL,
	MAX_DIMMING_MAPTBL
};

extern int register_panel_dimming(struct panel_dimming_info *info);
#endif /* __PANEL_DIMMING_H__ */
