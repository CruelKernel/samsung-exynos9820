/*
 * drivers/video/dpu/panels/emul_disp_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "emul_disp_param.h"

#include <video/mipi_display.h>
#include "../dsim.h"

static int dsim_write_hl_data(u32 id, const u8 *cmd, u32 cmdSize)
{
	int ret;
	int retry;

	retry = 5;

try_write:
	if (cmdSize == 1)
		ret = dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, cmd[0], 0);
	else if (cmdSize == 2)
		ret = dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE_PARAM, cmd[0], cmd[1]);
	else
		ret = dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)cmd,
cmdSize);

	if (ret != 0) {
		if (--retry)
			goto try_write;
		else
			dsim_err("dsim write failed,  cmd : %x\n", cmd[0]);
	}
	return ret;
}

void lcd_enable(int id)
{
	int ret = 0;

	dsim_info("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(id, SEQ_DISPLAY_ON, ARRAY_SIZE(SEQ_DISPLAY_ON));
	if (ret < 0)
		dsim_err("%s : fail to write CMD : DISPLAY_ON\n", __func__);
}

void lcd_disable(int id)
{
	/* This function needs to implement */
}

void lcd_init(int id, struct decon_lcd *lcd)
{
	int ret = 0;

	dsim_dbg("MDD : %s was called\n", __func__);

	ret = dsim_write_hl_data(id, SEQ_SLEEP_OUT, ARRAY_SIZE(SEQ_SLEEP_OUT));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_SLEEP_OUT\n", __func__);
		goto init_exit;
	}

	msleep(25);

#if 0 /* If you want to configure LCD as command mode, below code is needed */
	ret = dsim_write_hl_data(id, SEQ_TE_OUT, ARRAY_SIZE(SEQ_TE_OUT));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : SEQ_TE_OUT\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(id, CA_SET_600, ARRAY_SIZE(CA_SET_600));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : CA_SET_600\n", __func__);
		goto init_exit;
	}

	ret = dsim_write_hl_data(id, PA_SET_1280, ARRAY_SIZE(PA_SET_1280));
	if (ret < 0) {
		dsim_err(":%s fail to write CMD : PA_SET_1280\n", __func__);
		goto init_exit;
	}
#endif
init_exit:

	dsim_dbg("%s -\n", __func__);

	return;
}
