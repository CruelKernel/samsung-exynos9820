/*
 * drivers/media/platform/exynos/mfc/mfc_hw_reg_api.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __MFC_HW_REG_API_H
#define __MFC_HW_REG_API_H __FILE__

#include "mfc_reg_api.h"

#include "mfc_common.h"

#include "mfc_utils.h"

#define mfc_get_int_reason()	(MFC_READL(MFC_REG_RISC2HOST_CMD)		\
						& MFC_REG_RISC2HOST_CMD_MASK)
#define mfc_clear_int()				\
		do {							\
			MFC_WRITEL(0, MFC_REG_RISC2HOST_CMD);	\
			MFC_WRITEL(0, MFC_REG_RISC2HOST_INT);	\
		} while (0)

static inline int mfc_wait_pending(struct mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	/* Check F/W wait status */
	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while waiting MFC F/W done\n");
			return -EIO;
		}
		status = MFC_READL(MFC_REG_FIRMWARE_STATUS_INFO);
	} while ((status & 0x1) == 0);

	/* Check H/W pending status */
	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while pendng clear\n");
			mfc_err_dev("MFC access pending R: %#x, BUS: %#x\n",
					MFC_READL(MFC_REG_MFC_RPEND),
					MFC_READL(MFC_REG_MFC_BUS_STATUS));
			return -EIO;
		}
		status = MFC_READL(MFC_REG_MFC_RPEND);
	} while (status != 0);

	MFC_TRACE_DEV("** pending wait done\n");

	return 0;
}

static inline int mfc_stop_bus(struct mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	/* Reset */
	MFC_WRITEL(0x1, MFC_REG_MFC_BUS_RESET_CTRL);

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check bus status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while resetting MFC.\n");
			return -EIO;
		}
		status = MFC_READL(MFC_REG_MFC_BUS_RESET_CTRL);
	} while ((status & 0x2) == 0);

	return 0;
}

static inline void mfc_start_bus(struct mfc_dev *dev)
{
	int val;

	val = MFC_READL(MFC_REG_MFC_BUS_RESET_CTRL);
	val &= ~(0x1);
	MFC_WRITEL(val, MFC_REG_MFC_BUS_RESET_CTRL);
}

static inline void mfc_risc_on(struct mfc_dev *dev)
{
	mfc_clean_dev_int_flags(dev);

	MFC_WRITEL(0x1, MFC_REG_RISC_ON);
	MFC_WRITEL(0x0, MFC_REG_MFC_OFF);
	mfc_debug(1, "RISC_ON\n");
	MFC_TRACE_DEV(">> RISC ON\n");
}

static inline void mfc_risc_off(struct mfc_dev *dev)
{
	unsigned int status;
	unsigned long timeout;

	timeout = jiffies + msecs_to_jiffies(MFC_BW_TIMEOUT);
	/* Check pending status */
	do {
		if (time_after(jiffies, timeout)) {
			mfc_err_dev("Timeout while pendng clear\n");
			mfc_err_dev("MFC access pending state: %#x\n", status);
			mfc_err_dev("MFC access pending R: %#x, W: %#x\n",
					MFC_READL(MFC_REG_MFC_RPEND),
					MFC_READL(MFC_REG_MFC_WPEND));
			break;
		}
		status = MFC_READL(MFC_REG_MFC_BUS_STATUS);
	} while (status != 0);

	MFC_WRITEL(0x0, MFC_REG_RISC_ON);
}

static inline void mfc_mfc_off(struct mfc_dev *dev)
{
	mfc_info_dev("MFC h/w state: %d\n",
			MFC_READL(MFC_REG_MFC_STATE) & 0x7);
	MFC_WRITEL(0x1, MFC_REG_MFC_OFF);
}

static inline void mfc_enable_all_clocks(struct mfc_dev *dev)
{
	/* Enable all FW clock gating */
	MFC_WRITEL(0xFFFFFFFF, MFC_REG_MFC_FW_CLOCK);
}

void mfc_reset_mfc(struct mfc_dev *dev);
void mfc_set_risc_base_addr(struct mfc_dev *dev,
				enum mfc_buf_usage_type buf_type);
void mfc_cmd_host2risc(struct mfc_dev *dev, int cmd);
int mfc_check_risc2host(struct mfc_dev *dev);

#endif /* __MFC_HW_REG_API_H */
