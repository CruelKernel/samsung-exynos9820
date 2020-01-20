/**
 * i2c-exynos5.h - Samsung Exynos5 I2C Controller Driver Header file
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __I2C_EXYNOS5_H
#define __I2C_EXYNOS5_H

struct exynos5_i2c {
	struct list_head	node;
	struct i2c_adapter	adap;
	unsigned int		need_hw_init;
	unsigned int		suspended:1;

	struct i2c_msg		*msg;
	struct completion	msg_complete;
	unsigned int		msg_ptr;
	unsigned int		msg_len;

	unsigned int		irq;

	void __iomem		*regs;
	struct clk		*clk;
	struct clk		*rate_clk;
	struct device		*dev;
	int			state;

	/*
	 * Since the TRANS_DONE bit is cleared on read, and we may read it
	 * either during an IRQ or after a transaction, keep track of its
	 * state here.
	 */
	int			trans_done;

	/* Controller operating frequency */
	unsigned int		fs_clock;
	unsigned int		hs_clock;
	unsigned int		fs_plus_clock;
	unsigned int		stand_clock;

	/* to set the source clock */
	unsigned int		default_clk;

	/*
	 * HSI2C Controller can operate in
	 * 1. High speed upto 3.4Mbps
	 * 2. Fast speed upto 1Mbps
	 */
	int			speed_mode;
	int			operation_mode;
	int			bus_id;
	int			scl_clk_stretch;
	int			stop_after_trans;
	unsigned int		transfer_delay;

	int			idle_ip_index;
	int			reset_before_trans;
	unsigned int		runtime_resumed;
	int			nack_restart;
};
#endif /*__I2C_EXYNOS5_H */
