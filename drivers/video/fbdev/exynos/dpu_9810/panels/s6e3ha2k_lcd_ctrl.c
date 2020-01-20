/*
 * drivers/video/decon/panels/s6e3ha2k_lcd_ctrl.c
 *
 * Samsung SoC MIPI LCD CONTROL functions
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 * Jiun Yu, <jiun.yu@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "s6e3ha2k_param.h"
#include "lcd_ctrl.h"

/* use FW_TEST definition when you test CAL on firmware */
/* #define FW_TEST */
#ifdef FW_TEST
#include "../dsim_fw.h"
#include "mipi_display.h"
#else
#include "../dsim.h"
#include <video/mipi_display.h>
#endif

/* Porch values. It depends on command or video mode */
#define S6E3HA2K_CMD_VBP	15
#define S6E3HA2K_CMD_VFP	1
#define S6E3HA2K_CMD_VSA	1
#define S6E3HA2K_CMD_HBP	1
#define S6E3HA2K_CMD_HFP	1
#define S6E3HA2K_CMD_HSA	1

/* These need to define */
#define S6E3HA2K_VIDEO_VBP	15
#define S6E3HA2K_VIDEO_VFP	1
#define S6E3HA2K_VIDEO_VSA	1
#define S6E3HA2K_VIDEO_HBP	20
#define S6E3HA2K_VIDEO_HFP	20
#define S6E3HA2K_VIDEO_HSA	20

#define S6E3HA2K_HORIZONTAL	1440
#define S6E3HA2K_VERTICAL	2560

#ifdef FW_TEST /* This information is moved to DT */
#define CONFIG_FB_I80_COMMAND_MODE

struct decon_lcd s6e3ha2k_lcd_info = {
#ifdef CONFIG_FB_I80_COMMAND_MODE
	.mode = DECON_MIPI_COMMAND_MODE,
	.vfp = S6E3HA2K_CMD_VFP,
	.vbp = S6E3HA2K_CMD_VBP,
	.hfp = S6E3HA2K_CMD_HFP,
	.hbp = S6E3HA2K_CMD_HBP,
	.vsa = S6E3HA2K_CMD_VSA,
	.hsa = S6E3HA2K_CMD_HSA,
#else
	.mode = DECON_VIDEO_MODE,
	.vfp = S6E3HA2K_VIDEO_VFP,
	.vbp = S6E3HA2K_VIDEO_VBP,
	.hfp = S6E3HA2K_VIDEO_HFP,
	.hbp = S6E3HA2K_VIDEO_HBP,
	.vsa = S6E3HA2K_VIDEO_VSA,
	.hsa = S6E3HA2K_VIDEO_HSA,
#endif
	.xres = S6E3HA2K_HORIZONTAL,
	.yres = S6E3HA2K_VERTICAL,

	/* Maybe, width and height will be removed */
	.width = 70,
	.height = 121,

	/* Mhz */
	.hs_clk = 1100,
	.esc_clk = 20,

	.fps = 60,
	.mic_enabled = 1,
	.mic_ver = MIC_VER_1_2,
};
#endif

/*
 * 3HA2K lcd init sequence
 *
 * Parameters
 *	- mic_enabled : if mic is enabled, MIC_ENABLE command must be sent
 *	- mode : LCD init sequence depends on command or video mode
 */

void lcd_init(int id, struct decon_lcd *lcd)
{
	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TEST_KEY_ON_F0,
				ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("fail to write KEY_ON init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_REG_F2,
				ARRAY_SIZE(SEQ_REG_F2)) < 0)
		dsim_err("fail to write F2 init command.\n");

	if (lcd->mic_enabled)
		if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_REG_F9,
					ARRAY_SIZE(SEQ_REG_F9)) < 0)
			dsim_err("fail to write F9 init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, (unsigned long)SEQ_SLEEP_OUT[0], 0) < 0)
		dsim_err("fail to write SLEEP_OUT init command.\n");

	dsim_wait_for_cmd_completion(id);
	msleep(10);

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TEST_KEY_ON_F0,
				ARRAY_SIZE(SEQ_TEST_KEY_ON_F0)) < 0)
		dsim_err("fail to write KEY_ON init command.\n");

	/* TE rising time change : 10 line earlier */
	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TE_START_SETTING,
				ARRAY_SIZE(SEQ_TE_START_SETTING)) < 0)
		dsim_err("fail to write TE_START_SETTING command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_REG_F2,
				ARRAY_SIZE(SEQ_REG_F2)) < 0)
		dsim_err("fail to write F2 init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, SEQ_TE_ON[0], 0) < 0)
		dsim_err("fail to write TE_on init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TOUCH_HSYNC,
				ARRAY_SIZE(SEQ_TOUCH_HSYNC)) < 0)
		dsim_err("fail to write TOUCH_HSYNC init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_PENTILE_CONTROL,
				ARRAY_SIZE(SEQ_PENTILE_CONTROL)) < 0)
		dsim_err("fail to write PENTILE_CONTROL init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_COLUMN_ADDRESS,
				ARRAY_SIZE(SEQ_COLUMN_ADDRESS)) < 0)
		dsim_err("fail to write COLUMN_ADDRESS init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_GAMMA_CONDITION_SET,
				ARRAY_SIZE(SEQ_GAMMA_CONDITION_SET)) < 0)
		dsim_err("fail to write GAMMA_CONDITION_SET init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_AID_SET,
				ARRAY_SIZE(SEQ_AID_SET)) < 0)
		dsim_err("fail to write AID_SET init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_ELVSS_SET,
				ARRAY_SIZE(SEQ_ELVSS_SET)) < 0)
		dsim_err("fail to write ELVSS_SET init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_GAMMA_UPDATE,
				ARRAY_SIZE(SEQ_GAMMA_UPDATE)) < 0)
		dsim_err("fail to write GAMMA_UPDATE init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_ACL_OFF,
				ARRAY_SIZE(SEQ_ACL_OFF)) < 0)
		dsim_err("fail to write ACL_OFF init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_ACL_OPR,
				ARRAY_SIZE(SEQ_ACL_OPR)) < 0)
		dsim_err("fail to write ACL_OPR init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_HBM_OFF,
				ARRAY_SIZE(SEQ_HBM_OFF)) < 0)
		dsim_err("fail to write HBM_OFF init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TSET_GLOBAL,
				ARRAY_SIZE(SEQ_TSET_GLOBAL)) < 0)
		dsim_err("fail to write TSET_GLOBAL init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TSET,
				ARRAY_SIZE(SEQ_TSET)) < 0)
		dsim_err("fail to write TSET init command.\n");

	if (dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)SEQ_TEST_KEY_OFF_F0,
				ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0)) < 0)
		dsim_err("fail to write KEY_OFF init command.\n");

	/* Added 120ms delay before SEQ_DISPLAY_ON */
	dsim_wait_for_cmd_completion(id);
	msleep(120);
}

void lcd_enable(int id)
{
	if (dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, (unsigned long)SEQ_DISPLAY_ON[0], 0) < 0)
		dsim_err("fail to write DISPLAY_ON command.\n");
}

void lcd_disable(int id)
{
	/* This function needs to implement */
}

/*
 * Set gamma values
 *
 * Parameter
 *	- backlightlevel : It is from 0 to 26.
 */
int lcd_gamma_ctrl(int id, u32 backlightlevel)
{
/* This will be implemented
	int ret;
	ret = dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (u32)gamma22_table[backlightlevel],
			GAMMA_PARAM_SIZE);
	if (ret) {
		dsim_err("fail to write gamma value.\n");
		return ret;
	}
*/
	return 0;
}

int lcd_gamma_update(int id)
{
/* This will be implemented
	int ret;
	ret = dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (u32)SEQ_GAMMA_UPDATE,
			ARRAY_SIZE(SEQ_GAMMA_UPDATE));
	if (ret) {
		dsim_err("fail to update gamma value.\n");
		return ret;
	}
*/
	return 0;
}

int dsim_write_by_panel(int id, const u8 *cmd, u32 cmdSize)
{
	int ret;

	if (cmdSize == 1)
		ret = dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE, cmd[0], 0);
	else if (cmdSize == 2)
		ret = dsim_wr_data(id, MIPI_DSI_DCS_SHORT_WRITE_PARAM, cmd[0], cmd[1]);
	else
		ret = dsim_wr_data(id, MIPI_DSI_DCS_LONG_WRITE, (unsigned long)cmd, cmdSize);

	return ret;
}

int dsim_read_from_panel(int id, u8 addr, u32 size, u8 *buf)
{
	int ret;

	ret = dsim_rd_data(id, MIPI_DSI_DCS_READ, (u32)addr, size, buf);

	return ret;
}

static int s6e3ha2_wqhd_dump(int dsim)
{
	int ret = 0;
	unsigned char id[S6E3HA2_RD_LEN];
	unsigned char rddpm[S6E3HA2_RD_LEN + 1];
	unsigned char rddsm[S6E3HA2_RD_LEN + 1];
	unsigned char err_buf[S6E3HA2_RD_LEN + 1];

	dsim_info(" + %s\n", __func__);
	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_ON_F0, ARRAY_SIZE(SEQ_TEST_KEY_ON_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_F0\n", __func__);
	}

	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_ON_FC, ARRAY_SIZE(SEQ_TEST_KEY_ON_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_ON_FC\n", __func__);
	}

	ret = dsim_read_from_panel(dsim, 0xEA, S6E3HA2_RD_LEN, err_buf);
	if (ret != S6E3HA2_RD_LEN) {
		dsim_err("%s : can't read Panel's EA Reg\n",__func__);
		goto dump_exit;
	}

	dsim_dbg("=== Panel's 0xEA Reg Value ===\n");
	dsim_dbg("* 0xEA : buf[0] = %x\n", err_buf[0]);
	dsim_dbg("* 0xEA : buf[1] = %x\n", err_buf[1]);

	ret = dsim_read_from_panel(dsim, S6E3HA2_RDDPM_ADDR, S6E3HA2_RD_LEN, rddpm);
	if (ret != S6E3HA2_RD_LEN) {
		dsim_err("%s : can't read RDDPM Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's RDDPM Reg Value : %x ===\n", rddpm[0]);

	if (rddpm[0] & 0x80)
		dsim_info("* Booster Voltage Status : ON\n");
	else
		dsim_info("* Booster Voltage Status : OFF\n");

	if (rddpm[0] & 0x40)
		dsim_info("* Idle Mode : On\n");
	else
		dsim_info("* Idle Mode : OFF\n");

	if (rddpm[0] & 0x20)
		dsim_info("* Partial Mode : On\n");
	else
		dsim_info("* Partial Mode : OFF\n");

	if (rddpm[0] & 0x10)
		dsim_info("* Sleep OUT and Working Ok\n");
	else
		dsim_info("* Sleep IN\n");

	if (rddpm[0] & 0x08)
		dsim_info("* Normal Mode On and Working Ok\n");
	else
		dsim_info("* Sleep IN\n");

	if (rddpm[0] & 0x04)
		dsim_info("* Display On and Working Ok\n");
	else
		dsim_info("* Display Off\n");

	ret = dsim_read_from_panel(dsim, S6E3HA2_RDDSM_ADDR, S6E3HA2_RD_LEN, rddsm);
	if (ret != S6E3HA2_RD_LEN) {
		dsim_err("%s : can't read RDDSM Reg\n",__func__);
		goto dump_exit;
	}

	dsim_info("=== Panel's RDDSM Reg Value : %x ===\n", rddsm[0]);

	if (rddsm[0] & 0x80)
		dsim_info("* TE On\n");
	else
		dsim_info("* TE OFF\n");

	if (rddsm[0] & 0x02)
		dsim_info("* S_DSI_ERR : Found\n");

	if (rddsm[0] & 0x01)
		dsim_info("* DSI_ERR : Found\n");

	ret = dsim_read_from_panel(dsim, S6E3HA2_ID_REG, S6E3HA2_RD_LEN, id);
	if (ret != S6E3HA2_RD_LEN) {
		dsim_err("%s : can't read panel id\n",__func__);
		goto dump_exit;
	}

	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_OFF_FC, ARRAY_SIZE(SEQ_TEST_KEY_OFF_FC));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_FC\n", __func__);
	}

	ret = dsim_write_by_panel(dsim, SEQ_TEST_KEY_OFF_F0, ARRAY_SIZE(SEQ_TEST_KEY_OFF_F0));
	if (ret < 0) {
		dsim_err("%s : fail to write CMD : SEQ_TEST_KEY_OFF_F0\n", __func__);
	}
dump_exit:
	dsim_info(" - %s\n", __func__);
	return ret;

}

int lcd_dump(int id)
{
	s6e3ha2_wqhd_dump(id);
	return 0;
}
