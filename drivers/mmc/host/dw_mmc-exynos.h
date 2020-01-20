/*
 * Exynos Specific Extensions for Synopsys DW Multimedia Card Interface driver
 *
 * Copyright (C) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DW_MMC_EXYNOS_H_
#define _DW_MMC_EXYNOS_H_

#include <crypto/smu.h>
#include <crypto/fmp.h>

#define NUM_PINS(x)			(x + 2)

#define MAX_TUNING_RETRIES      7
#define MAX_TUNING_LOOP         (MAX_TUNING_RETRIES * 8 * 2)

/* Each descriptor can transfer up to 4KB of data in chained mode */
#define DW_MCI_DESC_DATA_LENGTH	0x1000

/* Tuning Phase */
#define TUNING_PHASE_0		0
#define TUNING_PHASE_1		1
#define TUNING_PHASE_2		2
#define TUNING_PHASE_3		3
#define TUNING_PHASE_4		4
#define TUNING_PHASE_5		5
#define TUNING_PHASE_6		6
#define TUNING_PHASE_7		7
#define TUNING_PHASE_8		8
#define TUNING_PHASE_9		9
#define TUNING_PHASE_10		10
#define TUNING_PHASE_11		11
#define TUNING_PHASE_12		12
#define TUNING_PHASE_13		13
#define TUNING_PHASE_14		14
#define TUNING_PHASE_15		15

struct exynos_smu_data {
	struct exynos_smu_variant_ops *vops;
	struct platform_device *pdev;
};

struct exynos_fmp_data {
	struct exynos_fmp_variant_ops *vops;
	struct platform_device *pdev;
};

/* Exynos implementation specific driver private data */
struct dw_mci_exynos_priv_data {
	u8 ctrl_type;
	u8 ciu_div;
	u32 sdr_timing;
	u32 ddr_timing;
	u32 sdr_hs_timing;
	u32 tuned_sample;
	u32 cur_speed;
	u32 dqs_delay;
	u32 saved_dqs_en;
	u32 saved_strobe_ctrl;
	u32 hs200_timing;
	u32 hs400_timing;
	u32 hs400_ulp_timing;
	u32 hs400_tx_t_fastlimit;
	u32 hs400_tx_t_initval;
	u32 sdr104_timing;
	u32 sdr50_timing;
	u32 *ref_clk;
	u32 delay_line;
	u32 tx_delay_line;
	struct pinctrl *pinctrl;
	u32 clk_drive_number;
	u32 clk_drive_tuning;
	struct pinctrl_state *clk_drive_base;
	struct pinctrl_state *clk_drive_str[6];
	struct pinctrl_state *pins_config[2];
	int cd_gpio;
	int sec_sd_slot_type;
#define SEC_NO_DET_SD_SLOT  0 /* No detect GPIO SD slot case */
#define SEC_HOTPLUG_SD_SLOT 1 /* detect GPIO SD slot without Tray */
#define SEC_HYBRID_SD_SLOT  2 /* detect GPIO SD slot with Tray */
	u32 caps;
	u32 ctrl_flag;
	u32 ctrl_windows;
	u32 ignore_phase;
	u32 selclk_drv;
	u32 voltage_int_extra;
	struct exynos_smu_data smu;
	struct exynos_fmp_data fmp;

#define DW_MMC_EXYNOS_BYPASS_FOR_ALL_PASS	BIT(0)
#define DW_MMC_EXYNOS_ENABLE_SHIFT		BIT(1)
};

#define phase6_en      BIT(6)
#define phase7_en      BIT(7)

extern int dw_mci_exynos_request_status(void);
extern void dw_mci_reg_dump(struct dw_mci *host);

/*****************/
/* SFR addresses */
/*****************/

#define SFR_OFFSET		0x0004

/*
* Registers to support idmac 64-bit address mode
*/
#define SDMMC_DBADDRL 		 0x0088
#define SDMMC_DBADDRU		(SDMMC_DBADDRL + SFR_OFFSET)
#define SDMMC_IDSTS64		(SDMMC_DBADDRU + SFR_OFFSET)
#define SDMMC_IDINTEN64		(SDMMC_IDSTS64 + SFR_OFFSET)
#define SDMMC_DSCADDRL		(SDMMC_IDINTEN64 + SFR_OFFSET)
#define SDMMC_DSCADDRU		(SDMMC_DSCADDRL + SFR_OFFSET)
#define SDMMC_BUFADDRL		(SDMMC_DSCADDRU + SFR_OFFSET)
#define SDMMC_BUFADDRU		(SDMMC_BUFADDRL + SFR_OFFSET)

#if defined(CONFIG_MMC_DW_64BIT_DESC)
#define SDMMC_AXI_BURST_LEN	0x00b4
#define SDMMC_SECTOR_NUM_INC	0x01F8
#define SDMMC_CLKSEL		(SDMMC_BUFADDRU + SFR_OFFSET)	/* specific to Samsung Exynos */
#else
#define SDMMC_CLKSEL		(SDMMC_BUFADDR + SFR_OFFSET)	/* specific to Samsung Exynos */
#define SDMMC_AXI_BURST_LEN	0xffff	/*not used */
#define SDMMC_SECTOR_NUM_INC	0xffff	/*not used */
#endif

#define SDMMC_CDTHRCTL		0x100
#define SDMMC_DATA(x)		(x)

/* Extended Register's Offset */
#define SDMMC_HS400_ENABLE_SHIFT	0x110
#define SDMMC_HS400_DQS_EN		0x180
#define SDMMC_HS400_ASYNC_FIFO_CTRL	0x184
#define SDMMC_HS400_DLINE_CTRL		0x188

/* Protector Register */
#define SDMMC_EMMCP_BASE	0x1000
#define SDMMC_MPSTAT			(SDMMC_EMMCP_BASE + 0x0008)
#define SDMMC_MPSECURITY		(SDMMC_EMMCP_BASE + 0x0010)
#define SDMMC_MPENCKEY			(SDMMC_EMMCP_BASE + 0x0020)
#define SDMMC_MPSBEGIN0			(SDMMC_EMMCP_BASE + 0x0200)
#define SDMMC_MPSEND0			(SDMMC_EMMCP_BASE + 0x0204)
#define SDMMC_MPSLUN0			(SDMMC_EMMCP_BASE + 0x0208)
#define SDMMC_MPSCTRL0			(SDMMC_EMMCP_BASE + 0x020C)
#define SDMMC_MPSBEGIN1			(SDMMC_EMMCP_BASE + 0x0210)
#define SDMMC_MPSEND1			(SDMMC_EMMCP_BASE + 0x0214)
#define SDMMC_MPSCTRL1			(SDMMC_EMMCP_BASE + 0x021C)

#define SDMMC_FORCE_CLK_STOP	0x0b0

/* CLKSEL register defines */
#define SDMMC_CLKSEL_CCLK_SAMPLE(x)	(((x) & 7) << 0)
#define SDMMC_CLKSEL_CCLK_FINE_SAMPLE(x)	(((x) & 0xF) << 0)
#define SDMMC_CLKSEL_CCLK_DRIVE(x)	(((x) & 7) << 16)
#define SDMMC_CLKSEL_CCLK_FINE_DRIVE(x)	(((x) & 3) << 22)
#define SDMMC_CLKSEL_CCLK_DIVIDER(x)	(((x) & 7) << 24)
#define SDMMC_CLKSEL_GET_DRV_WD3(x)	(((x) >> 16) & 0x7)
#define SDMMC_CLKSEL_GET_DIV(x)		(((x) >> 24) & 0x7)
#define SDMMC_CLKSEL_GET_DIVRATIO(x)	((((x) >> 24) & 0x7) + 1)
#define SDMMC_CLKSEL_UP_SAMPLE(x, y)	(((x) & ~SDMMC_CLKSEL_CCLK_SAMPLE(7)) |\
					 SDMMC_CLKSEL_CCLK_SAMPLE(y))
#define SDMMC_CLKSEL_TIMING(div, f_drv, drv, sample) \
	(SDMMC_CLKSEL_CCLK_DIVIDER(div) |	\
	 SDMMC_CLKSEL_CCLK_FINE_DRIVE(f_drv) |	\
	 SDMMC_CLKSEL_CCLK_DRIVE(drv) |		\
	 SDMMC_CLKSEL_CCLK_SAMPLE(sample))
#define SDMMC_CLKSEL_TIMING_MASK	SDMMC_CLKSEL_TIMING(0x7, 0x7, 0x7, 0x7)
#define SDMMC_CLKSEL_WAKEUP_INT		BIT(11)

/* RCLK_EN register defines */
#define DATA_STROBE_EN			BIT(0)
#define AXI_NON_BLOCKING_WR	BIT(7)

/* SDMMC_DDR200_RDDQS_EN */
#define DWMCI_TXDT_CRC_TIMER_FASTLIMIT(x)	(((x) & 0xFF) << 16)
#define DWMCI_TXDT_CRC_TIMER_INITVAL(x)		(((x) & 0xFF) << 8)
#define DWMCI_TXDT_CRC_TIMER_SET(x, y)	(DWMCI_TXDT_CRC_TIMER_FASTLIMIT(x) | \
					DWMCI_TXDT_CRC_TIMER_INITVAL(y))
#define DWMCI_AXI_NON_BLOCKING_WRITE		BIT(7)
#define DWMCI_RESP_RCLK_MODE			BIT(5)
#define DWMCI_BUSY_CHK_CLK_STOP_EN		BIT(2)
#define DWMCI_RXDATA_START_BIT_SEL		BIT(1)
#define DWMCI_RDDQS_EN				BIT(0)
#define DWMCI_DDR200_RDDQS_EN_DEF	(DWMCI_TXDT_CRC_TIMER_FASTLIMIT(0x13) | \
					DWMCI_TXDT_CRC_TIMER_INITVAL(0x15))

/* SDMMC_SECTOR_NUM_INC */
#define DWMCI_BURST_LENGTH_MASK		(0xF)
#define DWMCI_BURST_LENGTH_CTRL(x)	(((x)&DWMCI_BURST_LENGTH_MASK) | \
					(((x)&DWMCI_BURST_LENGTH_MASK)<<16))

/* SDMMC_SECTOR_NUM_INC */
#define DWMCI_SECTOR_SIZE_MASK		(0x1FFF)
#define DWMCI_SECTOR_SIZE_CTRL(x)	((x)&DWMCI_SECTOR_SIZE_MASK)

/* SDMMC_DDR200_ENABLE_SHIFT */
#define DWMCI_ENABLE_SHIFT_MASK			(0x3)
#define DWMCI_ENABLE_SHIFT(x)			((x) & DWMCI_ENABLE_SHIFT_MASK)

/* SDMMC_DDR200_ASYNC_FIFO_CTRL */
#define DWMCI_ASYNC_FIFO_RESET		BIT(0)

/* SDMMC_DDR200_DLINE_CTRL */
#define DWMCI_WD_DQS_DELAY_CTRL(x)		(((x) & 0x3FF) << 20)
#define DWMCI_FIFO_CLK_DELAY_CTRL(x)		(((x) & 0x3) << 16)
#define DWMCI_RD_DQS_DELAY_CTRL(x)		((x) & 0x3FF)
#define DWMCI_DDR200_DLINE_CTRL_SET(x, y, z)	(DWMCI_WD_DQS_DELAY_CTRL(x) | \
						DWMCI_FIFO_CLK_DELAY_CTRL(y) | \
						DWMCI_RD_DQS_DELAY_CTRL(z))
#define DWMCI_DDR200_DLINE_CTRL_DEF	(DWMCI_FIFO_CLK_DELAY_CTRL(0x2) | \
					DWMCI_RD_DQS_DELAY_CTRL(0x40))

/* DLINE_CTRL register defines */
#define DQS_CTRL_RD_DELAY(x, y)		(((x) & ~0x3FF) | ((y) & 0x3FF))
#define DQS_CTRL_GET_RD_DELAY(x)	((x) & 0x3FF)

/* Block number in eMMC */
#define SDMMC_BLOCK_NUM			0xFFFFFFFF

/* SMU control defines */
#define DWMCI_MPSCTRL_SECURE_READ_BIT		BIT(7)
#define DWMCI_MPSCTRL_SECURE_WRITE_BIT		BIT(6)
#define DWMCI_MPSCTRL_NON_SECURE_READ_BIT	BIT(5)
#define DWMCI_MPSCTRL_NON_SECURE_WRITE_BIT	BIT(4)
#define DWMCI_MPSCTRL_USE_FUSE_KEY		BIT(3)
#define DWMCI_MPSCTRL_ECB_MODE			BIT(2)
#define DWMCI_MPSCTRL_ENCRYPTION		BIT(1)
#define DWMCI_MPSCTRL_VALID			BIT(0)
#define DWMCI_MPSCTRL_BYPASS			(DWMCI_MPSCTRL_SECURE_READ_BIT |\
						DWMCI_MPSCTRL_SECURE_WRITE_BIT |\
						DWMCI_MPSCTRL_NON_SECURE_READ_BIT |\
						DWMCI_MPSCTRL_NON_SECURE_WRITE_BIT |\
						DWMCI_MPSCTRL_VALID)

/* Maximum number of Ending sector */
#define SDMMC_ENDING_SEC_NR_MAX	0xFFFFFFFF

/* Fixed clock divider */
#define EXYNOS4210_FIXED_CIU_CLK_DIV	2
#define EXYNOS4412_FIXED_CIU_CLK_DIV	4
#define HS400_FIXED_CIU_CLK_DIV		1

/* Minimal required clock frequency for cclkin, unit: HZ */
#define EXYNOS_CCLKIN_MIN	25000000

/* FMP SECURITY bits */
#define DWMCI_MPSECURITY_PROTBYTZPC		BIT(31)
#define DWMCI_MPSECURITY_MMC_SFR_PROT_ON	BIT(29)
#define DWMCI_MPSECURITY_FMP_ENC_ON		BIT(28)
#define DWMCI_MPSECURITY_DESCTYPE(type) 	((type & 0x3) << 19)

/* HWACG Control */
#define MMC_HWACG_CONTROL			BIT(4)
#define HWACG_Q_ACTIVE_EN			1
#define HWACG_Q_ACTIVE_DIS			0

/* PINS STATE Control */
#define PINS_FUNC			1
#define PINS_PDN			0

/* Phase 7 Mux Control */
#define sample_path_sel_en(dev, reg) ({\
		u32 __ret = 0;\
		__ret = __raw_readl((dev)->regs + SDMMC_##reg);\
		__ret &= ~(0x1 << 31);\
		__raw_writel(((__ret) | (0x1 << 31)) , (dev)->regs + SDMMC_##reg);\
		})

#define sample_path_sel_dis(dev, reg) ({\
		u32 __ret = 0;\
		__ret = __raw_readl((dev)->regs + SDMMC_##reg);\
		__ret &= ~(0x1 << 31);\
		__raw_writel((__ret) , (dev)->regs + SDMMC_##reg);\
		})

#endif				/* _DW_MMC_EXYNOS_H_ */
