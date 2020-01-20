/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Exynos9110 DSIM CAL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_DSIM_CAL_H__
#define __SAMSUNG_DSIM_CAL_H__

#include "../panels/decon_lcd.h"

#define MAX_DSIM_CNT		1

struct dsim_clks {
	u32 hs_clk;
	u32 esc_clk;
	u32 byte_clk;
	u32 word_clk;
};

/*************** DSIM CAL APIs exposed to DSIM driver ***************/
/* DPHY system register control */
void dpu_sysreg_select_dphy_rst_control(void __iomem *sysreg, u32 dsim_id, u32 sel);

/* DSIM control */
void dsim_reg_init(u32 id, struct decon_lcd *lcd_info, struct dsim_clks *clks,
		bool panel_ctrl);
void dsim_reg_start(u32 id);
int dsim_reg_stop(u32 id, u32 lanes);

/* ULPS control */
int dsim_reg_exit_ulps_and_start(u32 id, u32 ddi_type, u32 lanes);
int dsim_reg_stop_and_enter_ulps(u32 id, u32 ddi_type, u32 lanes);

/* DSIM interrupt control */
int dsim_reg_get_int_and_clear(u32 id);
void dsim_reg_clear_int(u32 id, u32 int_src);

/* DSIM read/write command control */
void dsim_reg_wr_tx_header(u32 id, u32 d_id, unsigned long d0, u32 d1, u32 bta);
void dsim_reg_wr_tx_payload(u32 id, u32 payload);
u32 dsim_reg_header_fifo_is_empty(u32 id);
u32 dsim_reg_is_writable_fifo_state(u32 id);
u32 dsim_reg_get_rx_fifo(u32 id);
u32 dsim_reg_rx_fifo_is_empty(u32 id);
int dsim_reg_rx_err_handler(u32 id, u32 rx_fifo);

/* For reading DSIM shadow SFR */
void dsim_reg_enable_shadow_read(u32 id, u32 en);

/* For window update and multi resolution feature */
void dsim_reg_function_reset(u32 id);
void dsim_reg_set_partial_update(u32 id, struct decon_lcd *lcd_info);
void dsim_reg_set_mres(u32 id, struct decon_lcd *lcd_info);

/* DSIM BIST for test */
void dsim_reg_set_bist(u32 id, u32 en);

void dsim_reg_set_cmd_transfer_mode(u32 id, u32 lp);
#endif /* __SAMSUNG_DSIM_CAL_H__ */
