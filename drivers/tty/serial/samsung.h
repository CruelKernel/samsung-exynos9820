#ifndef __SAMSUNG_H
#define __SAMSUNG_H

/*
 * Driver for Samsung SoC onboard UARTs.
 *
 * Ben Dooks, Copyright (c) 2003-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/pm_qos.h>

#define S3C24XX_UART_PORT_RESUME		0x0
#define S3C24XX_UART_PORT_SUSPEND		0x3
#define S3C24XX_UART_PORT_LPM			0x5

#define S3C24XX_SERIAL_CTRL_NUM			0x4
#define S3C24XX_SERIAL_BUAD_NUM			0x2

struct s3c24xx_uart_info {
	char			*name;
	unsigned int		type;
	unsigned int		fifosize;
	unsigned long		rx_fifomask;
	unsigned long		rx_fifoshift;
	unsigned long		rx_fifofull;
	unsigned long		tx_fifomask;
	unsigned long		tx_fifoshift;
	unsigned long		tx_fifofull;
	unsigned int		rts_trig_shift;
	unsigned int		def_clk_sel;
	unsigned long		num_clks;
	unsigned long		clksel_mask;
	unsigned long		clksel_shift;

	/* uart port features */

	unsigned int		has_divslot:1;

	/* uart controls */
	int (*reset_port)(struct uart_port *, struct s3c2410_uartcfg *);
};

struct s3c24xx_serial_drv_data {
	struct s3c24xx_uart_info	*info;
	struct s3c2410_uartcfg		*def_cfg;
	unsigned int			fifosize[CONFIG_SERIAL_SAMSUNG_UARTS];
};

struct uart_local_buf {
	unsigned char *buffer;
	unsigned int size;
	unsigned int index;
};

struct s3c24xx_uart_port {
	struct list_head		node;
	unsigned char			rx_claimed;
	unsigned char			tx_claimed;
	unsigned long			baudclk_rate;

	unsigned int			rx_irq;
	unsigned int			tx_irq;

	int				check_separated_clk;
	unsigned int			src_clk_rate;
	struct s3c24xx_uart_info	*info;
	struct clk			*clk;
	struct clk			*separated_clk;
	struct clk			*baudclk;
	struct uart_port		port;
	struct s3c24xx_serial_drv_data	*drv_data;

	u32				uart_irq_affinity;
	s32				mif_qos_val;
	s32				cpu_qos_val;
	u32				use_default_irq;
	unsigned long			qos_timeout;
	unsigned int			usi_v2;
	unsigned int			uart_panic_log;
	struct pinctrl_state 	*uart_pinctrl_tx_dat;
	struct pinctrl_state 	*uart_pinctrl_rts;
	struct pinctrl_state 	*uart_pinctrl_default;
	struct pinctrl *default_uart_pinctrl;
	unsigned int		rts_control;
	unsigned int		rts_trig_level;

	/* reference to platform data */
	struct s3c2410_uartcfg		*cfg;

	struct platform_device		*pdev;

	struct pm_qos_request		s3c24xx_uart_mif_qos;
	struct pm_qos_request		s3c24xx_uart_cpu_qos;
	struct delayed_work		qos_work;

	unsigned int			in_band_wakeup;
	unsigned int dbg_mode;
	unsigned int			uart_logging;
	struct uart_local_buf		uart_local_buf;
};

/* conversion functions */

#define s3c24xx_dev_to_port(__dev) dev_get_drvdata(__dev)

/* register access controls */

#define portaddr(port, reg) ((port)->membase + (reg))
#define portaddrl(port, reg) \
	((unsigned long *)(unsigned long)((port)->membase + (reg)))

#define rd_regb(port, reg) (readb_relaxed(portaddr(port, reg)))
#define rd_regl(port, reg) (readl_relaxed(portaddr(port, reg)))

#define wr_regb(port, reg, val) writeb_relaxed(val, portaddr(port, reg))
#define wr_regl(port, reg, val) writel_relaxed(val, portaddr(port, reg))
static void uart_copy_to_local_buf(int dir, struct uart_local_buf *local_buf, unsigned char *trace_buf, int len);
#define SS_UART_LOG(dir, local_buf, trace_buf) uart_copy_to_local_buf(dir, local_buf, trace_buf, sizeof(trace_buf))

#endif
