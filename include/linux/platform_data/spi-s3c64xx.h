/*
 * Copyright (C) 2009 Samsung Electronics Ltd.
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SPI_S3C64XX_H
#define __SPI_S3C64XX_H

#include <linux/dmaengine.h>

/*
 * The configuration of spi dma mode
 */
enum {
	DMA_MODE = 0,
	CPU_MODE = 1,
};

/*
 * The configuration of spi swap mode
 */
enum {
	NO_SWAP_MODE = 0,
	SWAP_MODE = 1,
};

/*
 * The configuration of spi secure mode
 */
enum {
	NONSECURE_MODE = 0,
	SECURE_MODE = 1,
};

enum {
	MANUAL_CS_MODE = 0,
	AUTO_CS_MODE = 1,
};

/*
 * Located domain
 */
enum spi_domain {
	DOMAIN_TOP = 0,
	DOMAIN_ISP,
	DOMAIN_CAM1,
};

struct platform_device;

/**
 * struct s3c64xx_spi_csinfo - ChipSelect description
 * @fb_delay: Slave specific feedback delay.
 *            Refer to FB_CLK_SEL register definition in SPI chapter.
 * @line: Custom 'identity' of the CS line.
 *
 * This is per SPI-Slave Chipselect information.
 * Allocate and initialize one in machine init code and make the
 * spi_board_info.controller_data point to it.
 */
struct s3c64xx_spi_csinfo {
	u8 fb_delay;
	u8 cs_mode;
	unsigned line;
};

/**
 * struct s3c64xx_spi_info - SPI Controller defining structure
 * @src_clk_nr: Clock source index for the CLK_CFG[SPI_CLKSEL] field.
 * @num_cs: Number of CS this controller emulates.
 * @cfg_gpio: Configure pins for this SPI controller.
 */
struct s3c64xx_spi_info {
	struct list_head node;
	unsigned int need_hw_init;
	int src_clk_nr;
	int num_cs;
	int dma_mode;
	int swap_mode;
	int secure_mode;
	int (*cfg_gpio)(void);
	dma_filter_fn filter;
	enum spi_domain domain;
	unsigned int dbg_mode;
};

struct s3c64xx_spi_dma_data {
	struct dma_chan *ch;
	enum dma_transfer_direction direction;
	unsigned long dmach;
};

/**
 * struct s3c64xx_spi_driver_data - Runtime info holder for SPI driver.
 * @clk: Pointer to the spi clock.
 * @src_clk: Pointer to the clock used to generate SPI signals.
 * @master: Pointer to the SPI Protocol master.
 * @cntrlr_info: Platform specific data for the controller this driver manages.
 * @tgl_spi: Pointer to the last CS left untoggled by the cs_change hint.
 * @queue: To log SPI xfer requests.
 * @lock: Controller specific lock.
 * @state: Set of FLAGS to indicate status.
 * @rx_dmach: Controller's DMA channel for Rx.
 * @tx_dmach: Controller's DMA channel for Tx.
 * @sfr_start: BUS address of SPI controller regs.
 * @regs: Pointer to ioremap'ed controller registers.
 * @irq: interrupt
 * @xfer_completion: To indicate completion of xfer task.
 * @cur_mode: Stores the active configuration of the controller.
 * @cur_bpw: Stores the active bits per word settings.
 * @cur_speed: Stores the active xfer clock speed.
 */
struct s3c64xx_spi_driver_data {
	void __iomem                    *regs;
	struct clk                      *clk;
	struct clk                      *src_clk;
	struct platform_device          *pdev;
	struct spi_master               *master;
	struct s3c64xx_spi_info  *cntrlr_info;
	struct spi_device               *tgl_spi;
	struct list_head                queue;
	spinlock_t                      lock;
	unsigned long                   sfr_start;
	struct completion               xfer_completion;
	unsigned                        state;
	unsigned                        cur_mode, cur_bpw;
	unsigned                        cur_speed;
	struct s3c64xx_spi_dma_data	rx_dma;
	struct s3c64xx_spi_dma_data	tx_dma;
	struct samsung_dma_ops		*ops;

	struct s3c64xx_spi_port_config	*port_conf;
	unsigned int			port_id;
	unsigned long			gpios[4];

	struct pinctrl			*pinctrl;
	struct pinctrl_state		*pin_def;
	struct pinctrl_state		*pin_idle;

	int is_probed;
	int spi_clkoff_time;
	int idle_ip_index;
};

/**
 * s3c64xx_spi_set_platdata - SPI Controller configure callback by the board
 *				initialization code.
 * @cfg_gpio: Pointer to gpio setup function.
 * @src_clk_nr: Clock the SPI controller is to use to generate SPI clocks.
 * @num_cs: Number of elements in the 'cs' array.
 *
 * Call this from machine init code for each SPI Controller that
 * has some chips attached to it.
 */
extern void s3c64xx_spi0_set_platdata(struct s3c64xx_spi_info *pd,
				      int src_clk_nr, int num_cs);
extern void s3c64xx_spi1_set_platdata(struct s3c64xx_spi_info *pd,
				      int src_clk_nr, int num_cs);
extern void s3c64xx_spi2_set_platdata(struct s3c64xx_spi_info *pd,
				      int src_clk_nr, int num_cs);

/* defined by architecture to configure gpio */
extern int s3c64xx_spi0_cfg_gpio(void);
extern int s3c64xx_spi1_cfg_gpio(void);
extern int s3c64xx_spi2_cfg_gpio(void);

extern struct s3c64xx_spi_info s3c64xx_spi0_pdata;
extern struct s3c64xx_spi_info s3c64xx_spi1_pdata;
extern struct s3c64xx_spi_info s3c64xx_spi2_pdata;
#endif /*__SPI_S3C64XX_H */
