/*
 * drivers/media/platform/exynos/mfc/mfc_cal.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <trace/events/mfc.h>

#include "mfc_hw_reg_api.h"
#include "mfc_pm.h"

/* Reset the device */
void mfc_reset_mfc(struct mfc_dev *dev)
{
	int i;

	mfc_debug_enter();

	/* Zero Initialization of MFC registers */
	MFC_WRITEL(0, MFC_REG_RISC2HOST_CMD);
	MFC_WRITEL(0, MFC_REG_HOST2RISC_CMD);
	MFC_WRITEL(0, MFC_REG_FW_VERSION);

	for (i = 0; i < MFC_REG_REG_CLEAR_COUNT; i++)
		MFC_WRITEL(0, MFC_REG_REG_CLEAR_BEGIN + (i*4));

	MFC_WRITEL(0x1FFF, MFC_REG_MFC_RESET);
	MFC_WRITEL(0, MFC_REG_MFC_RESET);

	mfc_debug_leave();
}

void mfc_set_risc_base_addr(struct mfc_dev *dev,
					enum mfc_buf_usage_type buf_type)
{
	struct mfc_special_buf *fw_buf;

	fw_buf = &dev->fw_buf;

	if (buf_type == MFCBUF_DRM)
		fw_buf = &dev->drm_fw_buf;

	MFC_WRITEL(fw_buf->daddr, MFC_REG_RISC_BASE_ADDRESS);
	mfc_debug(2, "[MEMINFO][F/W] %s Base Address : %#x\n",
			buf_type == MFCBUF_DRM ? "DRM" : "NORMAL", (u32)fw_buf->daddr);
	MFC_TRACE_DEV("%s F/W Base Address : %#x\n",
			buf_type == MFCBUF_DRM ? "DRM" : "NORMAL", (u32)fw_buf->daddr);
}

void mfc_cmd_host2risc(struct mfc_dev *dev, int cmd)
{
	mfc_debug(1, "Issue the command: %d\n", cmd);
	MFC_TRACE_DEV(">> CMD : %d, (dev:0x%lx, bits:%lx, owned:%d, wl:%d, trans:%d)\n",
			cmd, dev->hwlock.dev, dev->hwlock.bits, dev->hwlock.owned_by_irq,
			dev->hwlock.wl_count, dev->hwlock.transfer_owner);
	MFC_TRACE_LOG_DEV("C%d", cmd);

	trace_mfc_frame_start(dev->curr_ctx, cmd, 0, 0);
	/* Reset RISC2HOST command except nal q stop command */
	if (cmd != MFC_REG_H2R_CMD_STOP_QUEUE)
		MFC_WRITEL(0x0, MFC_REG_RISC2HOST_CMD);

	/* Start the timeout watchdog */
	if ((cmd != MFC_REG_H2R_CMD_NAL_QUEUE) && (cmd != MFC_REG_H2R_CMD_STOP_QUEUE))
		mfc_watchdog_start_tick(dev);

	if (dbg_enable) {
		/* For FW debugging */
		mfc_dbg_set_addr(dev);
		mfc_dbg_enable(dev);
	}

	dev->last_cmd = cmd;
	dev->last_cmd_time = ktime_to_timeval(ktime_get());

	/* Issue the command */
	MFC_WRITEL(cmd, MFC_REG_HOST2RISC_CMD);
	MFC_WRITEL(0x1, MFC_REG_HOST2RISC_INT);
}

/* Check whether HW interrupt has occurred or not */
int mfc_check_risc2host(struct mfc_dev *dev)
{
	if (mfc_pm_get_pwr_ref_cnt(dev) && mfc_pm_get_clk_ref_cnt(dev)) {
		if (MFC_READL(MFC_REG_RISC2HOST_INT))
			return MFC_READL(MFC_REG_RISC2HOST_CMD);
		else
			return 0;
	}

	return 0;
}
