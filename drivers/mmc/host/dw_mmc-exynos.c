/*
 * Exynos Specific Extensions for Synopsys DW Multimedia Card Interface driver
 *
 * Copyright (C) 2012, Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/smc.h>
#include <linux/sec_class.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"
#include "dw_mmc-exynos.h"

extern int cal_pll_mmc_set_ssc(unsigned int mfr, unsigned int mrr, unsigned int ssc_on);
extern int cal_pll_mmc_check(void);

static void dw_mci_exynos_register_dump(struct dw_mci *host)
{
	dev_err(host->dev, ": EMMCP_BASE:	0x%08x\n",
		host->sfr_dump->fmp_emmcp_base = mci_readl(host, EMMCP_BASE));
	dev_err(host->dev, ": MPSECURITY:	0x%08x\n",
		host->sfr_dump->mpsecurity = mci_readl(host, MPSECURITY));
	dev_err(host->dev, ": MPSTAT:		0x%08x\n",
		host->sfr_dump->mpstat = mci_readl(host, MPSTAT));
	dev_err(host->dev, ": MPSBEGIN:		0x%08x\n",
		host->sfr_dump->mpsbegin = mci_readl(host, MPSBEGIN0));
	dev_err(host->dev, ": MPSEND:		0x%08x\n",
		host->sfr_dump->mpsend = mci_readl(host, MPSEND0));
	dev_err(host->dev, ": MPSCTRL:		0x%08x\n",
		host->sfr_dump->mpsctrl = mci_readl(host, MPSCTRL0));
	dev_err(host->dev, ": HS400_DQS_EN:  	0x%08x\n",
		host->sfr_dump->hs400_rdqs_en = mci_readl(host, HS400_DQS_EN));
	dev_err(host->dev, ": HS400_ASYNC_FIFO_CTRL:   0x%08x\n",
		host->sfr_dump->hs400_acync_fifo_ctrl = mci_readl(host, HS400_ASYNC_FIFO_CTRL));
	dev_err(host->dev, ": HS400_DLINE_CTRL:        0x%08x\n",
		host->sfr_dump->hs400_dline_ctrl = mci_readl(host, HS400_DLINE_CTRL));
}

void dw_mci_reg_dump(struct dw_mci *host)
{
	u32 reg;

	dev_err(host->dev, ": ============== REGISTER DUMP ==============\n");
	dev_err(host->dev, ": CTRL:	 0x%08x\n", host->sfr_dump->contrl = mci_readl(host, CTRL));
	dev_err(host->dev, ": PWREN:	 0x%08x\n", host->sfr_dump->pwren = mci_readl(host, PWREN));
	dev_err(host->dev, ": CLKDIV:	 0x%08x\n",
		host->sfr_dump->clkdiv = mci_readl(host, CLKDIV));
	dev_err(host->dev, ": CLKSRC:	 0x%08x\n",
		host->sfr_dump->clksrc = mci_readl(host, CLKSRC));
	dev_err(host->dev, ": CLKENA:	 0x%08x\n",
		host->sfr_dump->clkena = mci_readl(host, CLKENA));
	dev_err(host->dev, ": TMOUT:	 0x%08x\n", host->sfr_dump->tmout = mci_readl(host, TMOUT));
	dev_err(host->dev, ": CTYPE:	 0x%08x\n", host->sfr_dump->ctype = mci_readl(host, CTYPE));
	dev_err(host->dev, ": BLKSIZ:	 0x%08x\n",
		host->sfr_dump->blksiz = mci_readl(host, BLKSIZ));
	dev_err(host->dev, ": BYTCNT:	 0x%08x\n",
		host->sfr_dump->bytcnt = mci_readl(host, BYTCNT));
	dev_err(host->dev, ": INTMSK:	 0x%08x\n",
		host->sfr_dump->intmask = mci_readl(host, INTMASK));
	dev_err(host->dev, ": CMDARG:	 0x%08x\n",
		host->sfr_dump->cmdarg = mci_readl(host, CMDARG));
	dev_err(host->dev, ": CMD:	 0x%08x\n", host->sfr_dump->cmd = mci_readl(host, CMD));
	dev_err(host->dev, ": RESP0:	 0x%08x\n", mci_readl(host, RESP0));
	dev_err(host->dev, ": RESP1:	 0x%08x\n", mci_readl(host, RESP1));
	dev_err(host->dev, ": RESP2:	 0x%08x\n", mci_readl(host, RESP2));
	dev_err(host->dev, ": RESP3:	 0x%08x\n", mci_readl(host, RESP3));
	dev_err(host->dev, ": MINTSTS:	 0x%08x\n",
		host->sfr_dump->mintsts = mci_readl(host, MINTSTS));
	dev_err(host->dev, ": RINTSTS:	 0x%08x\n",
		host->sfr_dump->rintsts = mci_readl(host, RINTSTS));
	dev_err(host->dev, ": STATUS:	 0x%08x\n",
		host->sfr_dump->status = mci_readl(host, STATUS));
	dev_err(host->dev, ": FIFOTH:	 0x%08x\n",
		host->sfr_dump->fifoth = mci_readl(host, FIFOTH));
	dev_err(host->dev, ": CDETECT:	 0x%08x\n", mci_readl(host, CDETECT));
	dev_err(host->dev, ": WRTPRT:	 0x%08x\n", mci_readl(host, WRTPRT));
	dev_err(host->dev, ": GPIO:	 0x%08x\n", mci_readl(host, GPIO));
	dev_err(host->dev, ": TCBCNT:	 0x%08x\n",
		host->sfr_dump->tcbcnt = mci_readl(host, TCBCNT));
	dev_err(host->dev, ": TBBCNT:	 0x%08x\n",
		host->sfr_dump->tbbcnt = mci_readl(host, TBBCNT));
	dev_err(host->dev, ": DEBNCE:	 0x%08x\n", mci_readl(host, DEBNCE));
	dev_err(host->dev, ": USRID:	 0x%08x\n", mci_readl(host, USRID));
	dev_err(host->dev, ": VERID:	 0x%08x\n", mci_readl(host, VERID));
	dev_err(host->dev, ": HCON:	 0x%08x\n", mci_readl(host, HCON));
	dev_err(host->dev, ": UHS_REG:	 0x%08x\n",
		host->sfr_dump->uhs_reg = mci_readl(host, UHS_REG));
	dev_err(host->dev, ": BMOD:	 0x%08x\n", host->sfr_dump->bmod = mci_readl(host, BMOD));
	dev_err(host->dev, ": PLDMND:	 0x%08x\n", mci_readl(host, PLDMND));
	dev_err(host->dev, ": DBADDRL:	 0x%08x\n",
		host->sfr_dump->dbaddrl = mci_readl(host, DBADDRL));
	dev_err(host->dev, ": DBADDRU:	 0x%08x\n",
		host->sfr_dump->dbaddru = mci_readl(host, DBADDRU));
	dev_err(host->dev, ": DSCADDRL:	 0x%08x\n",
		host->sfr_dump->dscaddrl = mci_readl(host, DSCADDRL));
	dev_err(host->dev, ": DSCADDRU:	 0x%08x\n",
		host->sfr_dump->dscaddru = mci_readl(host, DSCADDRU));
	dev_err(host->dev, ": BUFADDR:	 0x%08x\n",
		host->sfr_dump->bufaddr = mci_readl(host, BUFADDR));
	dev_err(host->dev, ": BUFADDRU:	 0x%08x\n",
		host->sfr_dump->bufaddru = mci_readl(host, BUFADDRU));
	dev_err(host->dev, ": DBADDR:	 0x%08x\n",
		host->sfr_dump->dbaddr = mci_readl(host, DBADDR));
	dev_err(host->dev, ": DSCADDR:	 0x%08x\n",
		host->sfr_dump->dscaddr = mci_readl(host, DSCADDR));
	dev_err(host->dev, ": BUFADDR:	 0x%08x\n",
		host->sfr_dump->bufaddr = mci_readl(host, BUFADDR));
	dev_err(host->dev, ": CLKSEL:    0x%08x\n",
		host->sfr_dump->clksel = mci_readl(host, CLKSEL));
	dev_err(host->dev, ": IDSTS:	 0x%08x\n", mci_readl(host, IDSTS));
	dev_err(host->dev, ": IDSTS64:	 0x%08x\n",
		host->sfr_dump->idsts64 = mci_readl(host, IDSTS64));
	dev_err(host->dev, ": IDINTEN:	 0x%08x\n", mci_readl(host, IDINTEN));
	dev_err(host->dev, ": IDINTEN64: 0x%08x\n",
		host->sfr_dump->idinten64 = mci_readl(host, IDINTEN64));
	dev_err(host->dev, ": RESP_TAT: 0x%08x\n", mci_readl(host, RESP_TAT));
	dev_err(host->dev, ": FORCE_CLK_STOP: 0x%08x\n",
		host->sfr_dump->force_clk_stop = mci_readl(host, FORCE_CLK_STOP));
	dev_err(host->dev, ": CDTHRCTL: 0x%08x\n", mci_readl(host, CDTHRCTL));
	if (host->ch_id == 0)
		dw_mci_exynos_register_dump(host);
	dev_err(host->dev, ": ============== STATUS DUMP ================\n");
	dev_err(host->dev, ": cmd_status:      0x%08x\n",
		host->sfr_dump->cmd_status = host->cmd_status);
	dev_err(host->dev, ": data_status:     0x%08x\n",
		host->sfr_dump->force_clk_stop = host->data_status);
	dev_err(host->dev, ": pending_events:  0x%08lx\n",
		host->sfr_dump->pending_events = host->pending_events);
	dev_err(host->dev, ": completed_events:0x%08lx\n",
		host->sfr_dump->completed_events = host->completed_events);
	dev_err(host->dev, ": state:           %d\n", host->sfr_dump->host_state = host->state);
	dev_err(host->dev, ": gate-clk:            %s\n",
		atomic_read(&host->ciu_clk_cnt) ? "enable" : "disable");
	dev_err(host->dev, ": ciu_en_win:           %d\n", atomic_read(&host->ciu_en_win));
	reg = mci_readl(host, CMD);
	dev_err(host->dev, ": ================= CMD REG =================\n");
	if ((reg >> 9) & 0x1) {
		dev_err(host->dev, ": read/write        : %s\n",
			(reg & (0x1 << 10)) ? "write" : "read");
		dev_err(host->dev, ": data expected     : %d\n", (reg >> 9) & 0x1);
	}
	dev_err(host->dev, ": cmd index         : %d\n",
		host->sfr_dump->cmd_index = ((reg >> 0) & 0x3f));
	reg = mci_readl(host, STATUS);
	dev_err(host->dev, ": ================ STATUS REG ===============\n");
	dev_err(host->dev, ": fifocount         : %d\n",
		host->sfr_dump->fifo_count = ((reg >> 17) & 0x1fff));
	dev_err(host->dev, ": response index    : %d\n", (reg >> 11) & 0x3f);
	dev_err(host->dev, ": data state mc busy: %d\n", (reg >> 10) & 0x1);
	dev_err(host->dev, ": data busy         : %d\n",
		host->sfr_dump->data_busy = ((reg >> 9) & 0x1));
	dev_err(host->dev, ": data 3 state      : %d\n",
		host->sfr_dump->data_3_state = ((reg >> 8) & 0x1));
	dev_err(host->dev, ": command fsm state : %d\n", (reg >> 4) & 0xf);
	dev_err(host->dev, ": fifo full         : %d\n", (reg >> 3) & 0x1);
	dev_err(host->dev, ": fifo empty        : %d\n", (reg >> 2) & 0x1);
	dev_err(host->dev, ": fifo tx watermark : %d\n",
		host->sfr_dump->fifo_tx_watermark = ((reg >> 1) & 0x1));
	dev_err(host->dev, ": fifo rx watermark : %d\n",
		host->sfr_dump->fifo_rx_watermark = ((reg >> 0) & 0x1));
	dev_err(host->dev, ": ===========================================\n");
}

/* Variations in Exynos specific dw-mshc controller */
enum dw_mci_exynos_type {
	DW_MCI_TYPE_EXYNOS,
};

static struct dw_mci_exynos_compatible {
	char *compatible;
	enum dw_mci_exynos_type ctrl_type;
} exynos_compat[] = {
	{
.compatible = "samsung,exynos-dw-mshc",.ctrl_type = DW_MCI_TYPE_EXYNOS,},};

static inline u8 dw_mci_exynos_get_ciu_div(struct dw_mci *host)
{
	return SDMMC_CLKSEL_GET_DIV(mci_readl(host, CLKSEL)) + 1;
}

static int dw_mci_exynos_priv_init(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	priv->saved_strobe_ctrl = mci_readl(host, HS400_DLINE_CTRL);
	priv->saved_dqs_en = mci_readl(host, HS400_DQS_EN);
	priv->saved_dqs_en |= AXI_NON_BLOCKING_WR;
	mci_writel(host, HS400_DQS_EN, priv->saved_dqs_en);
	if (!priv->dqs_delay)
		priv->dqs_delay = DQS_CTRL_GET_RD_DELAY(priv->saved_strobe_ctrl);
#if defined(CONFIG_MMC_DW_64BIT_DESC)
	if (priv->voltage_int_extra != 0) {
		u32 reg = 0;
		reg = mci_readl(host, AXI_BURST_LEN);
		reg &= ~(0x7 << 24);
		reg |= (priv->voltage_int_extra << 24);
		mci_writel(host, AXI_BURST_LEN, reg);
	}
#endif
	return 0;
}

static void dw_mci_exynos_ssclk_control(struct dw_mci *host, int enable)
{
	u32 err;

	if (!(host->pdata->quirks & DW_MCI_QUIRK_USE_SSC))
		return;

	if (enable) {
		if (cal_pll_mmc_check() == true)
			goto out;

		if (host->pdata->ssc_rate > 8) {
			dev_info(host->dev, "unvalid SSC rate value\n");
			goto out;
		}

		err = cal_pll_mmc_set_ssc(12, host->pdata->ssc_rate, 1);
		if (err)
			dev_info(host->dev, "SSC set fail.\n");
		else
			dev_info(host->dev, "SSC set enable.\n");
	} else {
		if (cal_pll_mmc_check() == false)
			goto out;

		err = cal_pll_mmc_set_ssc(0, 0, 0);
		if (err)
			dev_info(host->dev, "SSC set fail.\n");
		else
			dev_info(host->dev, "SSC set disable.\n");
	}
out:
	return;
}

static void dw_mci_exynos_set_clksel_timing(struct dw_mci *host, u32 timing)
{
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);
	clksel = (clksel & ~SDMMC_CLKSEL_TIMING_MASK) | timing;

	if (!((host->pdata->io_mode == MMC_TIMING_MMC_HS400) ||
	      (host->pdata->io_mode == MMC_TIMING_MMC_HS400_ES)))
		clksel &= ~(BIT(30) | BIT(19));

	mci_writel(host, CLKSEL, clksel);
}

#ifdef CONFIG_PM
static int dw_mci_exynos_runtime_resume(struct device *dev)
{
	return dw_mci_runtime_resume(dev);
}

/**
 * dw_mci_exynos_resume_noirq - Exynos-specific resume code
 *
 * On exynos5420 there is a silicon errata that will sometimes leave the
 * WAKEUP_INT bit in the CLKSEL register asserted.  This bit is 1 to indicate
 * that it fired and we can clear it by writing a 1 back.  Clear it to prevent
 * interrupts from going off constantly.
 *
 * We run this code on all exynos variants because it doesn't hurt.
 */

static int dw_mci_exynos_resume_noirq(struct device *dev)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);

	if (clksel & SDMMC_CLKSEL_WAKEUP_INT)
		mci_writel(host, CLKSEL, clksel);

	return 0;
}
#else
#define dw_mci_exynos_resume_noirq	NULL
#endif				/* CONFIG_PM */

static void dw_mci_card_int_hwacg_ctrl(struct dw_mci *host, u32 flag)
{
	u32 reg;

	reg = mci_readl(host, FORCE_CLK_STOP);
	if (flag == HWACG_Q_ACTIVE_EN) {
		reg |= MMC_HWACG_CONTROL;
		host->qactive_check = HWACG_Q_ACTIVE_EN;
	} else {
		reg &= ~(MMC_HWACG_CONTROL);
		host->qactive_check = HWACG_Q_ACTIVE_DIS;
	}
	mci_writel(host, FORCE_CLK_STOP, reg);
}

static void dw_mci_exynos_config_hs400(struct dw_mci *host, u32 timing)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 dqs, strobe;

	/*
	 * Not supported to configure register
	 * related to HS400
	 */

	dqs = priv->saved_dqs_en;
	strobe = priv->saved_strobe_ctrl;

	if (timing == MMC_TIMING_MMC_HS400 || timing == MMC_TIMING_MMC_HS400_ES) {
		dqs &= ~(DWMCI_TXDT_CRC_TIMER_SET(0xFF, 0xFF));
		dqs |= (DWMCI_TXDT_CRC_TIMER_SET(priv->hs400_tx_t_fastlimit,
						 priv->hs400_tx_t_initval) | DWMCI_RDDQS_EN |
			DWMCI_AXI_NON_BLOCKING_WRITE);
		if (host->pdata->quirks & DW_MCI_QUIRK_ENABLE_ULP) {
			if (priv->delay_line || priv->tx_delay_line)
				strobe = DWMCI_WD_DQS_DELAY_CTRL(priv->tx_delay_line) |
				    DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(priv->delay_line);
			else
				strobe = DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(90);
		} else {
			if (priv->delay_line)
				strobe = DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(priv->delay_line);
			else
				strobe = DWMCI_FIFO_CLK_DELAY_CTRL(0x2) |
				    DWMCI_RD_DQS_DELAY_CTRL(90);
		}
		dqs |= (DATA_STROBE_EN | DWMCI_AXI_NON_BLOCKING_WRITE);
		if (timing == MMC_TIMING_MMC_HS400_ES)
			dqs |= DWMCI_RESP_RCLK_MODE;
	} else {
		dqs &= ~DATA_STROBE_EN;
	}

	mci_writel(host, HS400_DQS_EN, dqs);
	mci_writel(host, HS400_DLINE_CTRL, strobe);
}

static void dw_mci_exynos_adjust_clock(struct dw_mci *host, unsigned int wanted)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	unsigned long actual;
	u8 div;
	int ret;
	u32 clock;

	/*
	 * Don't care if wanted clock is zero or
	 * ciu clock is unavailable
	 */
	if (!wanted || IS_ERR(host->ciu_clk))
		return;

	/* Guaranteed minimum frequency for cclkin */
	if (wanted < EXYNOS_CCLKIN_MIN)
		wanted = EXYNOS_CCLKIN_MIN;

	div = dw_mci_exynos_get_ciu_div(host);

	if (wanted == priv->cur_speed) {
		clock = clk_get_rate(host->ciu_clk);
		if (clock == priv->cur_speed * div)
			return;
	}

	ret = clk_set_rate(host->ciu_clk, wanted * div);
	if (ret)
		dev_warn(host->dev, "failed to set clk-rate %u error: %d\n", wanted * div, ret);
	actual = clk_get_rate(host->ciu_clk);
	host->bus_hz = actual / div;
	priv->cur_speed = wanted;
	host->current_speed = 0;
}

#ifndef MHZ
#define MHZ (1000 * 1000)
#endif

#define KHZ (1000)

static void dw_mci_exynos_set_ios(struct dw_mci *host, struct mmc_ios *ios)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	unsigned int wanted = ios->clock;
	u32 *clk_tbl = priv->ref_clk;
	u32 timing = ios->timing, clksel;
	u32 cclkin;

	cclkin = clk_tbl[timing];
	host->pdata->io_mode = timing;
	if (host->bus_hz != cclkin)
		wanted = cclkin;

	switch (timing) {
	case MMC_TIMING_MMC_HS400:
	case MMC_TIMING_MMC_HS400_ES:
		if (host->pdata->quirks & DW_MCI_QUIRK_ENABLE_ULP) {
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->hs400_ulp_timing, priv->tuned_sample);
			clksel |= (BIT(30) | BIT(19));	/* ultra low powermode on */
		} else {
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->hs400_timing, priv->tuned_sample);
			clksel &= ~(BIT(30) | BIT(19));	/* ultra low powermode on */
			wanted <<= 1;
		}
		if (host->pdata->is_fine_tuned)
			clksel |= BIT(6);
		break;
	case MMC_TIMING_MMC_DDR52:
	case MMC_TIMING_UHS_DDR50:
		clksel = priv->ddr_timing;
		/* Should be double rate for DDR mode */
		if (ios->bus_width == MMC_BUS_WIDTH_8)
			wanted <<= 1;
		break;
	case MMC_TIMING_MMC_HS200:
		clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->hs200_timing, priv->tuned_sample);
		break;
	case MMC_TIMING_UHS_SDR104:
		if (priv->sdr104_timing)
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr104_timing, priv->tuned_sample);
		else {
			dev_info(host->dev, "Setting of SDR104 timing in not been!!\n");
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr_timing, priv->tuned_sample);
		}
		break;
	case MMC_TIMING_UHS_SDR50:
		if (priv->sdr50_timing)
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr50_timing, priv->tuned_sample);
		else {
			dev_info(host->dev, "Setting of SDR50 timing is not been!!\n");
			clksel = SDMMC_CLKSEL_UP_SAMPLE(priv->sdr_timing, priv->tuned_sample);
		}
		break;
	default:
		clksel = priv->sdr_timing;

	}

	if (host->pdata->quirks & DW_MCI_QUIRK_USE_SSC) {
		if ((ios->clock > 0) && (ios->clock < 100 * MHZ))
			dw_mci_exynos_ssclk_control(host, 0);
		else if (ios->clock)
			dw_mci_exynos_ssclk_control(host, 1);
	}

	if ((ios->clock > 0) && (ios->clock <= 400 * KHZ))
		sample_path_sel_dis(host, AXI_BURST_LEN);

	host->cclk_in = wanted;

	/* Set clock timing for the requested speed mode */
	dw_mci_exynos_set_clksel_timing(host, clksel);

	/* Configure setting for HS400 */
	dw_mci_exynos_config_hs400(host, timing);

	/* Configure clock rate */
	dw_mci_exynos_adjust_clock(host, wanted);
}

static int dw_mci_exynos_parse_dt(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv;
	struct device_node *np = host->dev->of_node;
	u32 timing[4];
	u32 div = 0, voltage_int_extra = 0;
	int idx;
	u32 ref_clk_size;
	u32 *ref_clk;
	u32 *ciu_clkin_values = NULL;
	int idx_ref;
	int ret = 0;
	int id = 0, i;

	priv = devm_kzalloc(host->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(host->dev, "mem alloc failed for private data\n");
		return -ENOMEM;
	}

	for (idx = 0; idx < ARRAY_SIZE(exynos_compat); idx++) {
		if (of_device_is_compatible(np, exynos_compat[idx].compatible))
			priv->ctrl_type = exynos_compat[idx].ctrl_type;
	}

	if (of_property_read_u32(np, "num-ref-clks", &ref_clk_size)) {
		dev_err(host->dev, "Getting a number of referece clock failed\n");
		ret = -ENODEV;
		goto err_ref_clk;
	}

	ref_clk = devm_kzalloc(host->dev, ref_clk_size * sizeof(*ref_clk), GFP_KERNEL);
	if (!ref_clk) {
		dev_err(host->dev, "Mem alloc failed for reference clock table\n");
		ret = -ENOMEM;
		goto err_ref_clk;
	}

	ciu_clkin_values = devm_kzalloc(host->dev,
					ref_clk_size * sizeof(*ciu_clkin_values), GFP_KERNEL);

	if (!ciu_clkin_values) {
		dev_err(host->dev, "Mem alloc failed for temporary clock values\n");
		ret = -ENOMEM;
		goto err_ref_clk;
	}
	if (of_property_read_u32_array(np, "ciu_clkin", ciu_clkin_values, ref_clk_size)) {
		dev_err(host->dev, "Getting ciu_clkin values faild\n");
		ret = -ENOMEM;
		goto err_ref_clk;
	}

	for (idx_ref = 0; idx_ref < ref_clk_size; idx_ref++, ref_clk++, ciu_clkin_values++) {
		if (*ciu_clkin_values > MHZ)
			*(ref_clk) = (*ciu_clkin_values);
		else
			*(ref_clk) = (*ciu_clkin_values) * MHZ;
	}

	ref_clk -= ref_clk_size;
	ciu_clkin_values -= ref_clk_size;
	priv->ref_clk = ref_clk;

	if (of_get_property(np, "card-detect", NULL))
		priv->cd_gpio = of_get_named_gpio(np, "card-detect", 0);
	else
		priv->cd_gpio = -1;

	if (of_get_property(np, "sec-sd-slot-type", NULL))
		of_property_read_u32(np,
				"sec-sd-slot-type", &priv->sec_sd_slot_type);
	else {
		if (priv->cd_gpio != -1) /* treat default SD slot if cd_gpio is defined */
			priv->sec_sd_slot_type = SEC_HOTPLUG_SD_SLOT;
		else
			priv->sec_sd_slot_type = -1;
	}

	/* Swapping clock drive strength */
	of_property_read_u32(np, "clk-drive-number", &priv->clk_drive_number);

	priv->pinctrl = devm_pinctrl_get(host->dev);

	if (IS_ERR(priv->pinctrl)) {
		priv->pinctrl = NULL;
	} else {
		priv->clk_drive_base = pinctrl_lookup_state(priv->pinctrl, "default");
		priv->clk_drive_str[0] = pinctrl_lookup_state(priv->pinctrl, "fast-slew-rate-1x");
		priv->clk_drive_str[1] = pinctrl_lookup_state(priv->pinctrl, "fast-slew-rate-2x");
		priv->clk_drive_str[2] = pinctrl_lookup_state(priv->pinctrl, "fast-slew-rate-3x");
		priv->clk_drive_str[3] = pinctrl_lookup_state(priv->pinctrl, "fast-slew-rate-4x");
		priv->clk_drive_str[4] = pinctrl_lookup_state(priv->pinctrl, "fast-slew-rate-5x");
		priv->clk_drive_str[5] = pinctrl_lookup_state(priv->pinctrl, "fast-slew-rate-6x");

		for (i = 0; i < 6; i++) {
			if (IS_ERR(priv->clk_drive_str[i]))
				priv->clk_drive_str[i] = NULL;
		}
		priv->pins_config[0] = pinctrl_lookup_state(priv->pinctrl, "pins-as-pdn");
		priv->pins_config[1] = pinctrl_lookup_state(priv->pinctrl, "pins-as-func");
		
		for (i = 0; i < 2; i++) {
			if (IS_ERR(priv->pins_config[i]))
				priv->pins_config[i] = NULL;
		}
	}

	of_property_read_u32(np, "samsung,dw-mshc-ciu-div", &div);
	priv->ciu_div = div;

	if (of_property_read_u32(np, "samsung,voltage-int-extra", &voltage_int_extra))
		priv->voltage_int_extra = voltage_int_extra;

	ret = of_property_read_u32_array(np, "samsung,dw-mshc-sdr-timing", timing, 4);
	if (ret)
		return ret;

	priv->sdr_timing = SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

	ret = of_property_read_u32_array(np, "samsung,dw-mshc-ddr-timing", timing, 4);
	if (ret)
		return ret;

	priv->ddr_timing = SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

	of_property_read_u32(np, "ignore-phase", &priv->ignore_phase);
	if (of_find_property(np, "bypass-for-allpass", NULL))
		priv->ctrl_flag |= DW_MMC_EXYNOS_BYPASS_FOR_ALL_PASS;
	if (of_find_property(np, "use-enable-shift", NULL))
		priv->ctrl_flag |= DW_MMC_EXYNOS_ENABLE_SHIFT;

	id = of_alias_get_id(host->dev->of_node, "mshc");
	switch (id) {
		/* dwmmc0 : eMMC    */
	case 0:
		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs200-timing", timing, 4);
		if (ret)
			goto err_ref_clk;
		priv->hs200_timing =
		    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs400-timing", timing, 4);
		if (ret)
			goto err_ref_clk;

		priv->hs400_timing =
		    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-hs400-ulp-timing", timing, 4);
		if (!ret)
			priv->hs400_ulp_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else
			ret = 0;

		/* Rx Delay Line */
		of_property_read_u32(np, "samsung,dw-mshc-hs400-delay-line", &priv->delay_line);

		/* Tx Delay Line */
		of_property_read_u32(np,
				     "samsung,dw-mshc-hs400-tx-delay-line", &priv->tx_delay_line);

		/* The fast RXCRC packet arrival time */
		of_property_read_u32(np,
				     "samsung,dw-mshc-txdt-crc-timer-fastlimit",
				     &priv->hs400_tx_t_fastlimit);

		/* Initial value of the timeout down counter for RXCRC packet */
		of_property_read_u32(np,
				     "samsung,dw-mshc-txdt-crc-timer-initval",
				     &priv->hs400_tx_t_initval);
		break;
		/* dwmmc1 : SDIO    */
	case 1:
		/* dwmmc2 : SD Card */
	case 2:
		ret = of_property_read_u32_array(np, "samsung,dw-mshc-sdr50-timing", timing, 4);	/* SDR50 100Mhz */
		if (!ret)
			priv->sdr50_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else {
			priv->sdr50_timing = priv->sdr_timing;
			ret = 0;
		}

		ret = of_property_read_u32_array(np, "samsung,dw-mshc-sdr104-timing", timing, 4);	/* SDR104 200mhz */
		if (!ret)
			priv->sdr104_timing =
			    SDMMC_CLKSEL_TIMING(timing[0], timing[1], timing[2], timing[3]);
		else {
			priv->sdr104_timing = priv->sdr_timing;
			ret = 0;
		}
		break;
	default:
		ret = -ENODEV;
	}
	host->priv = priv;
 err_ref_clk:
	return ret;
}

static inline u8 dw_mci_exynos_get_clksmpl(struct dw_mci *host)
{
	return SDMMC_CLKSEL_CCLK_SAMPLE(mci_readl(host, CLKSEL));
}

static inline void dw_mci_exynos_set_clksmpl(struct dw_mci *host, u8 sample)
{
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);
	clksel = SDMMC_CLKSEL_UP_SAMPLE(clksel, sample);
	mci_writel(host, CLKSEL, clksel);
}

static inline u8 dw_mci_exynos_move_next_clksmpl(struct dw_mci *host)
{
	u32 clksel;
	u8 sample;

	clksel = mci_readl(host, CLKSEL);
	sample = (clksel + 1) & 0x7;
	clksel = (clksel & ~0x7) | sample;
	mci_writel(host, CLKSEL, clksel);
	return sample;
}

static void dw_mci_set_quirk_endbit(struct dw_mci *host, s8 mid)
{
	u32 clksel, phase;
	u32 shift;

	clksel = mci_readl(host, CLKSEL);
	phase = (((clksel >> 24) & 0x7) + 1) << 1;
	shift = 360 / phase;

	if (host->verid < DW_MMC_260A && (shift * mid) % 360 >= 225)
		host->quirks |= DW_MCI_QUIRK_NO_DETECT_EBIT;
	else
		host->quirks &= ~DW_MCI_QUIRK_NO_DETECT_EBIT;
}

static void dw_mci_exynos_set_enable_shift(struct dw_mci *host, u32 sample, bool fine_tune)
{
	u32 i, j, en_shift, en_shift_phase[3][4] = { {0, 0, 1, 0},
	{1, 2, 3, 3},
	{2, 4, 5, 5}
	};

	en_shift = mci_readl(host, HS400_ENABLE_SHIFT)
	    & ~(DWMCI_ENABLE_SHIFT_MASK);

	for (i = 0; i < 3; i++) {
		for (j = 1; j < 4; j++) {
			if (sample == en_shift_phase[i][j]) {
				en_shift |= DWMCI_ENABLE_SHIFT(en_shift_phase[i][0]);
				break;
			}
		}
	}
	if ((en_shift < 2) && fine_tune)
		en_shift += 1;
	mci_writel(host, HS400_ENABLE_SHIFT, en_shift);
}

static u8 dw_mci_tuning_sampling(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 clksel, i;
	u8 sample;

	clksel = mci_readl(host, CLKSEL);
	sample = (clksel + 1) & 0x7;
	clksel = SDMMC_CLKSEL_UP_SAMPLE(clksel, sample);

	if (priv->ignore_phase) {
		for (i = 0; i < 8; i++) {
			if (priv->ignore_phase & (0x1 << sample))
				sample = (sample + 1) & 0x7;
			else
				break;
		}
	}
	clksel = (clksel & 0xfffffff8) | sample;
	mci_writel(host, CLKSEL, clksel);

	if (phase6_en & (0x1 << sample) || phase7_en & (0x1 << sample))
		sample_path_sel_en(host, AXI_BURST_LEN);
	else
		sample_path_sel_dis(host, AXI_BURST_LEN);

	if (priv->ctrl_flag & DW_MMC_EXYNOS_ENABLE_SHIFT)
		dw_mci_exynos_set_enable_shift(host, sample, false);

	return sample;
}

/* initialize the clock sample to given value */
static void dw_mci_exynos_set_sample(struct dw_mci *host, u32 sample, bool tuning)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 clksel;

	clksel = mci_readl(host, CLKSEL);
	clksel = (clksel & ~0x7) | SDMMC_CLKSEL_CCLK_SAMPLE(sample);
	mci_writel(host, CLKSEL, clksel);
	if (sample == 6 || sample == 7)
		sample_path_sel_en(host, AXI_BURST_LEN);
	else
		sample_path_sel_dis(host, AXI_BURST_LEN);

	if (priv->ctrl_flag & DW_MMC_EXYNOS_ENABLE_SHIFT)
		dw_mci_exynos_set_enable_shift(host, sample, false);
	if (!tuning)
		dw_mci_set_quirk_endbit(host, clksel);
}

static void dw_mci_set_fine_tuning_bit(struct dw_mci *host, bool is_fine_tuning)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 clksel, sample;

	clksel = mci_readl(host, CLKSEL);
	clksel = (clksel & ~BIT(6));
	sample = (clksel & 0x7);

	if (is_fine_tuning) {
		host->pdata->is_fine_tuned = true;
		clksel |= BIT(6);
	} else
		host->pdata->is_fine_tuned = false;
	mci_writel(host, CLKSEL, clksel);
	if (priv->ctrl_flag & DW_MMC_EXYNOS_ENABLE_SHIFT) {
		if (((sample % 2) == 1) && is_fine_tuning && sample != 0x7)
			dw_mci_exynos_set_enable_shift(host, sample, true);
		else
			dw_mci_exynos_set_enable_shift(host, sample, false);
	}
}

/* read current clock sample offset */
static u32 dw_mci_exynos_get_sample(struct dw_mci *host)
{
	u32 clksel = mci_readl(host, CLKSEL);
	return SDMMC_CLKSEL_CCLK_SAMPLE(clksel);
}

static int __find_median_of_16bits(u32 orig_bits, u16 mask, u8 startbit)
{
	u32 i, testbits;

	testbits = orig_bits;
	for (i = startbit; i < (16 + startbit); i++, testbits >>= 1)
		if ((testbits & mask) == mask)
			return SDMMC_CLKSEL_CCLK_FINE_SAMPLE(i);
	return -1;
}

#define NUM_OF_MASK	7
static int find_median_of_16bits(struct dw_mci *host, unsigned int map, bool force)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	u32 orig_bits;
	u8 i, divratio;
	int sel = -1;
	u16 mask[NUM_OF_MASK] = { 0x1fff, 0x7ff, 0x1ff, 0x7f, 0x1f, 0xf, 0x7 };
	/* Tuning during the center value is set to 3/2 */
	int optimum[NUM_OF_MASK] = { 9, 7, 6, 5, 3, 2, 1 };

	/* replicate the map so "arithimetic shift right" shifts in
	 * the same bits "again". e.g. portable "Rotate Right" bit operation.
	 */
	if (map == 0xFFFF && force == false)
		return sel;

	divratio = (mci_readl(host, CLKSEL) >> 24) & 0x7;
	dev_info(host->dev, "divratio: %d map: 0x %08x\n", divratio, map);

	orig_bits = map | (map << 16);

	if (divratio == 1) {
		if (!(priv->ctrl_flag & DW_MMC_EXYNOS_ENABLE_SHIFT))
			orig_bits = orig_bits & (orig_bits >> 8);
	}

	for (i = 0; i < NUM_OF_MASK; i++) {
		sel = __find_median_of_16bits(orig_bits, mask[i], optimum[i]);
		if (-1 != sel)
			break;
	}

	return sel;
}

static void exynos_dwmci_tuning_drv_st(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	dev_info(host->dev, "Clock GPIO Drive Strength Value: x%d\n", (priv->clk_drive_tuning));

	if (priv->pinctrl && priv->clk_drive_str[priv->clk_drive_tuning - 1])
		pinctrl_select_state(priv->pinctrl,
				     priv->clk_drive_str[priv->clk_drive_tuning - 1]);
}

static void dw_mci_set_pins_state(struct dw_mci *host, int config)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (priv->pinctrl && priv->pins_config[config])
		pinctrl_select_state(priv->pinctrl, priv->pins_config[config]);
}

/*
 * Test all 8 possible "Clock in" Sample timings.
 * Create a bitmap of which CLock sample values work and find the "median"
 * value. Apply it and remember that we found the best value.
 */
static int dw_mci_exynos_execute_tuning(struct dw_mci_slot *slot, u32 opcode,
					struct dw_mci_tuning_data *tuning_data)
{
	struct dw_mci *host = slot->host;
	struct dw_mci_exynos_priv_data *priv = host->priv;
	struct mmc_host *mmc = slot->mmc;
	unsigned int tuning_loop = MAX_TUNING_LOOP;
	unsigned int drv_str_retries;
	bool tuned = 0;
	int ret = 0;
	u8 *tuning_blk;		/* data read from device */

	unsigned int sample_good = 0;	/* bit map of clock sample (0-7) */
	u32 test_sample = -1;
	u32 orig_sample;
	int best_sample = 0, best_sample_ori = 0;
	u8 pass_index;
	bool is_fine_tuning = false;
	unsigned int abnormal_result = 0xFFFF;
	unsigned int temp_ignore_phase = priv->ignore_phase;
	int ffs_ignore_phase = 0;
	u8 all_pass_count = 0;
	bool bypass = false;

	while (temp_ignore_phase) {
		ffs_ignore_phase = ffs(temp_ignore_phase) - 1;
		abnormal_result &= ~(0x3 << (2 * ffs_ignore_phase));
		temp_ignore_phase &= ~(0x1 << ffs_ignore_phase);
	}

	/* Short circuit: don't tune again if we already did. */
	if (host->pdata->tuned) {
		host->drv_data->misc_control(host, CTRL_RESTORE_CLKSEL, NULL);
		mci_writel(host, CDTHRCTL, host->cd_rd_thr << 16 | 1);
		dev_info(host->dev, "EN_SHIFT 0x %08x CLKSEL 0x %08x\n",
			 mci_readl(host, HS400_ENABLE_SHIFT), mci_readl(host, CLKSEL));
		return 0;
	}

	tuning_blk = kmalloc(2 * tuning_data->blksz, GFP_KERNEL);
	if (!tuning_blk)
		return -ENOMEM;

	test_sample = orig_sample = dw_mci_exynos_get_sample(host);
	host->cd_rd_thr = 512;
	mci_writel(host, CDTHRCTL, host->cd_rd_thr << 16 | 1);

	/*
	 * eMMC 4.5 spec section 6.6.7.1 says the device is guaranteed to
	 * complete 40 iteration of CMD21 in 150ms. So this shouldn't take
	 * longer than about 30ms or so....at least assuming most values
	 * work and don't time out.
	 */

	if (host->pdata->io_mode == MMC_TIMING_MMC_HS400)
		host->quirks |= DW_MCI_QUIRK_NO_DETECT_EBIT;

	dev_info(host->dev, "Tuning Abnormal_result 0x%08x.\n", abnormal_result);

	priv->clk_drive_tuning = priv->clk_drive_number;
	drv_str_retries = priv->clk_drive_number;

	do {
		struct mmc_request mrq;
		struct mmc_command cmd;
		struct mmc_command stop;
		struct mmc_data data;
		struct scatterlist sg;

		if (!tuning_loop)
			break;

		memset(&cmd, 0, sizeof(cmd));
		cmd.opcode = opcode;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
		cmd.error = 0;
		cmd.busy_timeout = 10;	/* 2x * (150ms/40 + setup overhead) */

		memset(&stop, 0, sizeof(stop));
		stop.opcode = MMC_STOP_TRANSMISSION;
		stop.arg = 0;
		stop.flags = MMC_RSP_R1B | MMC_CMD_AC;
		stop.error = 0;

		memset(&data, 0, sizeof(data));
		data.blksz = tuning_data->blksz;
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;
		data.error = 0;

		memset(tuning_blk, ~0U, tuning_data->blksz);
		sg_init_one(&sg, tuning_blk, tuning_data->blksz);

		memset(&mrq, 0, sizeof(mrq));
		mrq.cmd = &cmd;
		mrq.stop = &stop;
		mrq.data = &data;
		host->mrq = &mrq;

		/*
		 * DDR200 tuning Sequence with fine tuning setup
		 *
		 * 0. phase 0 (0 degree) + no fine tuning setup
		 * - pass_index = 0
		 * 1. phase 0 + fine tuning setup
		 * - pass_index = 1
		 * 2. phase 1 (90 degree) + no fine tuning setup
		 * - pass_index = 2
		 * ..
		 * 15. phase 7 + fine tuning setup
		 * - pass_index = 15
		 *
		 */
		dw_mci_set_fine_tuning_bit(host, is_fine_tuning);

		dw_mci_set_timeout(host, dw_mci_calc_timeout(host));
		mmc_wait_for_req(mmc, &mrq);

		pass_index = (u8) test_sample *2;

		if (is_fine_tuning)
			pass_index++;

		if (!cmd.error && !data.error) {
			/*
			 * Verify the "tuning block" arrived (to host) intact.
			 * If yes, remember this sample value works.
			 */
			if (host->use_dma == 1) {
				sample_good |= (1 << pass_index);
			} else {
				if (!memcmp
				    (tuning_data->blk_pattern, tuning_blk, tuning_data->blksz))
					sample_good |= (1 << pass_index);
			}
		} else {
			dev_info(&mmc->class_dev,
				 "Tuning error: cmd.error:%d, data.error:%d CLKSEL = 0x%08x, EN_SHIFT = 0x%08x\n",
				 cmd.error, data.error,
				 mci_readl(host, CLKSEL), mci_readl(host, HS400_ENABLE_SHIFT));
		}

		if (is_fine_tuning)
			test_sample = dw_mci_tuning_sampling(host);

		is_fine_tuning = !is_fine_tuning;

		if (orig_sample == test_sample && !is_fine_tuning) {

			/*
			 * Get at middle clock sample values.
			 */
			if (sample_good == abnormal_result)
				all_pass_count++;

			if (priv->ctrl_flag & DW_MMC_EXYNOS_BYPASS_FOR_ALL_PASS)
				bypass = (all_pass_count > priv->clk_drive_number) ? true : false;

			if (bypass) {
				dev_info(host->dev, "Bypassed for all pass at %d times\n",
					 priv->clk_drive_number);
				sample_good = abnormal_result & 0xFFFF;
				tuned = true;
			}

			best_sample = find_median_of_16bits(host, sample_good, bypass);

			if (best_sample >= 0) {
				dev_info(host->dev, "sample_good: 0x%02x best_sample: 0x%02x\n",
					 sample_good, best_sample);

				if (sample_good != abnormal_result || bypass) {
					tuned = true;
					break;
				}
			} else
				dev_info(host->dev,
					 "Failed to find median value in sample good (0x%02x)\n",
					 sample_good);

			if (drv_str_retries) {
				drv_str_retries--;
				if (priv->clk_drive_str[0]) {
					exynos_dwmci_tuning_drv_st(host);
					if (priv->clk_drive_tuning > 0)
						priv->clk_drive_tuning--;
				}
				sample_good = 0;
			} else
				break;
		}
		tuning_loop--;
	} while (!tuned);

	/*
	 * To set sample value with mid, the value should be divided by 2,
	 * because mid represents index in pass map extended.(8 -> 16 bits)
	 * And that mid is odd number, means the selected case includes
	 * using fine tuning.
	 */

	best_sample_ori = best_sample;
	best_sample /= 2;

	if (host->pdata->io_mode == MMC_TIMING_MMC_HS400)
		host->quirks &= ~DW_MCI_QUIRK_NO_DETECT_EBIT;

	if (tuned) {
		host->pdata->clk_smpl = priv->tuned_sample = best_sample;
		if (host->pdata->only_once_tune)
			host->pdata->tuned = true;

		if (best_sample_ori % 2)
			best_sample += 1;

		dw_mci_exynos_set_sample(host, best_sample, false);
		dw_mci_set_fine_tuning_bit(host, false);
	} else {
		/* Failed. Just restore and return error */
		dev_err(host->dev, "tuning err\n");
		mci_writel(host, CDTHRCTL, 0 << 16 | 0);
		dw_mci_exynos_set_sample(host, orig_sample, false);
		ret = -EIO;
	}

	/* Rollback Clock drive strength */
	if (priv->pinctrl && priv->clk_drive_base)
		pinctrl_select_state(priv->pinctrl, priv->clk_drive_base);

	dev_info(host->dev, "CLKSEL = 0x%08x, EN_SHIFT = 0x%08x\n",
		 mci_readl(host, CLKSEL), mci_readl(host, HS400_ENABLE_SHIFT));

	kfree(tuning_blk);
	return ret;
}

static struct device *sd_detection_cmd_dev;

static ssize_t sd_detection_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (priv->sec_sd_slot_type > 0 && !gpio_is_valid(priv->cd_gpio))
		goto gpio_error;

	if (gpio_get_value(priv->cd_gpio) ^ (host->pdata->use_gpio_invert)
			&& (priv->sec_sd_slot_type == SEC_HYBRID_SD_SLOT)) {
		dev_info(host->dev, "SD slot tray Removed.\n");
		return sprintf(buf, "Notray\n");
	}

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		dev_info(host->dev, "SD card inserted.\n");
		return sprintf(buf, "Insert\n");
	}
	dev_info(host->dev, "SD card removed.\n");
	return sprintf(buf, "Remove\n");

gpio_error:
	dev_info(host->dev, "%s : External SD detect pin Error\n", __func__);
	return  sprintf(buf, "Error\n");
}

static ssize_t sd_detection_cnt_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	dev_info(host->dev, "%s : CD count is = %u\n", __func__, host->card_detect_cnt);
	return  sprintf(buf, "%u", host->card_detect_cnt);
}

static ssize_t sd_detection_maxmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	const char *uhs_bus_speed_mode = "";
	struct device_node *np = host->dev->of_node;

	if (of_find_property(np, "sd-uhs-sdr104", NULL))
		uhs_bus_speed_mode = "SDR104";
	else if (of_find_property(np, "sd-uhs-ddr50", NULL))
		uhs_bus_speed_mode = "DDR50";
	else if (of_find_property(np, "sd-uhs-sdr50", NULL))
		uhs_bus_speed_mode = "SDR50";
	else if (of_find_property(np, "sd-uhs-sdr25", NULL))
		uhs_bus_speed_mode = "SDR25";
	else if (of_find_property(np, "sd-uhs-sdr12", NULL))
		uhs_bus_speed_mode = "SDR12";
	else
		uhs_bus_speed_mode = "HS";

	dev_info(host->dev, "%s : Max supported Host Speed Mode = %s\n", __func__, uhs_bus_speed_mode);
	return  sprintf(buf, "%s\n", uhs_bus_speed_mode);
}
static ssize_t sd_detection_curmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);

	const char *uhs_bus_speed_mode = "";
	static const char *const uhs_speeds[] = {
		[UHS_SDR12_BUS_SPEED] = "SDR12",
		[UHS_SDR25_BUS_SPEED] = "SDR25",
		[UHS_SDR50_BUS_SPEED] = "SDR50",
		[UHS_SDR104_BUS_SPEED] = "SDR104",
		[UHS_DDR50_BUS_SPEED] = "DDR50",
	};

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		if (mmc_card_uhs(host->slot->mmc->card))
			uhs_bus_speed_mode = uhs_speeds[host->slot->mmc->card->sd_bus_speed];
		else
			uhs_bus_speed_mode = "HS";
	} else
		uhs_bus_speed_mode = "No Card";

	dev_info(host->dev, "%s : Current SD Card Speed = %s\n", __func__, uhs_bus_speed_mode);
	return  sprintf(buf, "%s\n", uhs_bus_speed_mode);
}

static ssize_t sdcard_summary_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *card;
	const char *uhs_bus_speed_mode = "";
	static const char *const uhs_speeds[] = {
		[UHS_SDR12_BUS_SPEED] = "SDR12",
		[UHS_SDR25_BUS_SPEED] = "SDR25",
		[UHS_SDR50_BUS_SPEED] = "SDR50",
		[UHS_SDR104_BUS_SPEED] = "SDR104",
		[UHS_DDR50_BUS_SPEED] = "DDR50",
	};
	static const char *const unit[] = {"KB", "MB", "GB", "TB"};
	unsigned int size, serial;
	int	digit = 1;
	char ret_size[6];

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		card = host->slot->mmc->card;

		/* MANID */
		/* SERIAL */
		serial = card->cid.serial & (0x0000FFFF);

		/*SIZE*/
		if (card->csd.read_blkbits == 9)			/* 1 Sector = 512 Bytes */
			size = (card->csd.capacity) >> 1;
		else if (card->csd.read_blkbits == 11)	/* 1 Sector = 2048 Bytes */
			size = (card->csd.capacity) << 1;
		else								/* 1 Sector = 1024 Bytes */
			size = card->csd.capacity;

		if (size >= 380000000 && size <= 410000000) {	/* QUIRK 400GB SD Card */ 
			sprintf(ret_size, "400GB"); 
		} else if(size >= 190000000 && size <= 210000000) {	/* QUIRK 200GB SD Card */
			sprintf(ret_size, "200GB");
		} else {
			while ((size >> 1) > 0) {
				size = size >> 1;
				digit++;
			}
			sprintf(ret_size, "%d%s", 1 << (digit%10), unit[digit/10]);
		}

		/* SPEEDMODE */
		if (mmc_card_uhs(card))
			uhs_bus_speed_mode = uhs_speeds[card->sd_bus_speed];
		else if (mmc_card_hs(card))
			uhs_bus_speed_mode = "HS";
		else
			uhs_bus_speed_mode = "DS";

		/* SUMMARY */
		dev_info(host->dev, "MANID : 0x%02X, SERIAL : %04X, SIZE : %s, SPEEDMODE : %s\n",
				card->cid.manfid, serial, ret_size, uhs_bus_speed_mode);
		return sprintf(buf, "\"MANID\":\"0x%02X\",\"SERIAL\":\"%04X\""\
				",\"SIZE\":\"%s\",\"SPEEDMODE\":\"%s\",\"NOTI\":\"%d\"\n",
				card->cid.manfid, serial, ret_size, uhs_bus_speed_mode,
				card->err_log[0].noti_cnt);
	} else {
		/* SUMMARY : No SD Card Case */
		dev_info(host->dev, "%s : No SD Card\n", __func__);
		return sprintf(buf, "\"MANID\":\"NoCard\",\"SERIAL\":\"NoCard\""\
			",\"SIZE\":\"NoCard\",\"SPEEDMODE\":\"NoCard\",\"NOTI\":\"NoCard\"\n");
	}
}

static struct device *sd_info_cmd_dev;
static ssize_t sd_count_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *cur_card = NULL;
	struct mmc_card_error_log *err_log;
	u64 total_cnt = 0;
	int len = 0;
	int i = 0;

	if (host->slot && host->slot->mmc && host->slot->mmc->card)
		cur_card = host->slot->mmc->card;
	else {
		len = snprintf(buf, PAGE_SIZE, "no card\n");
		goto out;
	}

	err_log = cur_card->err_log;

	for (i = 0; i < 6; i++) {
		if (total_cnt < MAX_CNT_U64)
			total_cnt += err_log[i].count;
	}
	len = snprintf(buf, PAGE_SIZE, "%lld\n", total_cnt);

out:
	return len;
}

static struct device *sd_data_cmd_dev;
static ssize_t sd_data_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *cur_card = NULL;
	struct mmc_card_error_log *err_log;
	u64 total_c_cnt = 0;
	u64 total_t_cnt = 0;
	int len = 0;
	int i = 0;

	if (host->slot && host->slot->mmc && host->slot->mmc->card)
		cur_card = host->slot->mmc->card;
	else {
		len = snprintf(buf, PAGE_SIZE,
			"\"GE\":\"0\",\"CC\":\"0\",\"ECC\":\"0\",\"WP\":\"0\"\
,\"OOR\":\"0\",\"CRC\":\"0\",\"TMO\":\"0\"\n");
		goto out;
	}

	err_log = cur_card->err_log;

	for (i = 0; i < 6; i++) {
		if (err_log[i].err_type == -EILSEQ && total_c_cnt < MAX_CNT_U64)
			total_c_cnt += err_log[i].count;
		if (err_log[i].err_type == -ETIMEDOUT && total_t_cnt < MAX_CNT_U64)
			total_t_cnt += err_log[i].count;
	}

	len = snprintf(buf, PAGE_SIZE,
		"\"GE\":\"%d\",\"CC\":\"%d\",\"ECC\":\"%d\",\"WP\":\"%d\"\
,\"OOR\":\"%d\",\"CRC\":\"%lld\",\"TMO\":\"%lld\"\n",
		err_log[0].ge_cnt, err_log[0].cc_cnt, err_log[0].ecc_cnt, err_log[0].wp_cnt,
		err_log[0].oor_cnt, total_c_cnt, total_t_cnt);
out:
	return len;
}

static ssize_t sd_cid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *cur_card = NULL;
	int len = 0;

	if (host->slot && host->slot->mmc && host->slot->mmc->card)
		cur_card = host->slot->mmc->card;
	else {
		len = snprintf(buf, PAGE_SIZE, "No Card\n");
		goto out;
	}

	len = snprintf(buf, PAGE_SIZE,
			"%08x%08x%08x%08x\n",
			cur_card->raw_cid[0], cur_card->raw_cid[1],
			cur_card->raw_cid[2], cur_card->raw_cid[3]);
out:
	return len;
}

static ssize_t sd_health_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *cur_card = NULL;
	struct mmc_card_error_log *err_log;
	u64 total_c_cnt = 0;
	u64 total_t_cnt = 0;
	int len = 0;
	int i = 0;

	if (host->slot && host->slot->mmc && host->slot->mmc->card)
		cur_card = host->slot->mmc->card;

	if (!cur_card) {
		//There should be no spaces in 'No Card'(Vold Team).
		len = snprintf(buf, PAGE_SIZE, "NOCARD\n");
		goto out;
	}

	err_log = cur_card->err_log;

	for (i = 0; i < 6; i++) {
		if (err_log[i].err_type == -EILSEQ && total_c_cnt < MAX_CNT_U64)
			total_c_cnt += err_log[i].count;
		if (err_log[i].err_type == -ETIMEDOUT && total_t_cnt < MAX_CNT_U64)
			total_t_cnt += err_log[i].count;
	}

	if(err_log[0].ge_cnt > 100 || err_log[0].ecc_cnt > 0 || err_log[0].wp_cnt > 0 ||
	   err_log[0].oor_cnt > 10 || total_t_cnt > 100 || total_c_cnt > 100)
		len = snprintf(buf, PAGE_SIZE, "BAD\n");
	else
		len = snprintf(buf, PAGE_SIZE, "GOOD\n");

out:
	return len;
}

static DEVICE_ATTR(status, 0444, sd_detection_cmd_show, NULL);
static DEVICE_ATTR(cd_cnt, 0444, sd_detection_cnt_show, NULL);
static DEVICE_ATTR(max_mode, 0444, sd_detection_maxmode_show, NULL);
static DEVICE_ATTR(current_mode, 0444, sd_detection_curmode_show, NULL);
static DEVICE_ATTR(sdcard_summary, 0444, sdcard_summary_show, NULL);
static DEVICE_ATTR(sd_count, 0444, sd_count_show, NULL);
static DEVICE_ATTR(sd_data, 0444, sd_data_show, NULL);
static DEVICE_ATTR(data, 0444, sd_cid_show, NULL);
static DEVICE_ATTR(fc, 0444, sd_health_show, NULL);

/* Callback function for SD Card IO Error */
static int sdcard_uevent(struct mmc_card *card)
{
	pr_info("%s: Send Notification about SD Card IO Error\n", mmc_hostname(card->host));
	return kobject_uevent(&sd_detection_cmd_dev->kobj, KOBJ_CHANGE);
}

static int dw_mci_sdcard_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct dw_mci *host = dev_get_drvdata(dev);
	struct mmc_card *card;
	int retval;
	bool card_exist;

	add_uevent_var(env, "DEVNAME=%s", dev->kobj.name);

	if (host->slot && host->slot->mmc && host->slot->mmc->card) {
		card_exist = true;
		card = host->slot->mmc->card;
	} else
		card_exist = false;

	retval = add_uevent_var(env, "IOERROR=%s", card_exist ? (
				((card->err_log[0].ge_cnt && !(card->err_log[0].ge_cnt % 1000)) ||
				 (card->err_log[0].ecc_cnt && !(card->err_log[0].ecc_cnt % 1000)) ||
				 (card->err_log[0].wp_cnt && !(card->err_log[0].wp_cnt % 100)) ||
				 (card->err_log[0].oor_cnt && !(card->err_log[0].oor_cnt % 100)))
				? "YES" : "NO")	: "NoCard");

	return retval;
}

static struct device_type sdcard_type = {
	.uevent = dw_mci_sdcard_uevent,
};

static int dw_mci_exynos_request_ext_irq(struct dw_mci *host, irq_handler_t func)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;
	int ext_cd_irq = 0;

	if (gpio_is_valid(priv->cd_gpio) && !gpio_request(priv->cd_gpio, "DWMCI_EXT_CD")) {
		ext_cd_irq = gpio_to_irq(priv->cd_gpio);
		if (ext_cd_irq &&
		    devm_request_irq(host->dev, ext_cd_irq, func,
				     IRQF_TRIGGER_RISING |
				     IRQF_TRIGGER_FALLING |
				     IRQF_ONESHOT, "tflash_det", host) == 0) {
			dev_info(host->dev, "success to request irq for card detect.\n");
			enable_irq_wake(ext_cd_irq);
		} else
			dev_info(host->dev, "cannot request irq for card detect.\n");
	}

	return 0;
}

static int dw_mci_exynos_check_cd(struct dw_mci *host)
{
	int ret = -1;
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if (gpio_is_valid(priv->cd_gpio)) {
		if (host->pdata->use_gpio_invert)
			ret = gpio_get_value(priv->cd_gpio) ? 1 : 0;
		else
			ret = gpio_get_value(priv->cd_gpio) ? 0 : 1;
	}
	return ret;
}

static void dw_mci_exynos_add_sysfs(struct dw_mci *host)
{
	struct dw_mci_exynos_priv_data *priv = host->priv;

	if ((priv->sec_sd_slot_type) >= 0) {
		if (!sd_detection_cmd_dev) {
			sd_detection_cmd_dev = sec_device_create(host, "sdcard");
			if (IS_ERR(sd_detection_cmd_dev))
				pr_err("Fail to create sysfs dev\n");

			sd_detection_cmd_dev->type = &sdcard_type;
			host->slot->mmc->sdcard_uevent = sdcard_uevent;
			if (device_create_file(sd_detection_cmd_dev,
						&dev_attr_status) < 0)
				pr_err("Fail to create status sysfs file\n");

			if (device_create_file(sd_detection_cmd_dev,
						&dev_attr_cd_cnt) < 0)
				pr_err("Fail to create cd_cnt sysfs file\n");

			if (device_create_file(sd_detection_cmd_dev,
						&dev_attr_max_mode) < 0)
				pr_err("Fail to create max_mode sysfs file\n");

			if (device_create_file(sd_detection_cmd_dev,
						&dev_attr_current_mode) < 0)
				pr_err("Fail to create current_mode sysfs file\n");

			if (device_create_file(sd_detection_cmd_dev,
						&dev_attr_sdcard_summary) < 0)
				pr_err("Fail to create sdcard_summary sysfs file\n");
		}

		if (!sd_info_cmd_dev) {
			sd_info_cmd_dev = sec_device_create(host, "sdinfo");
			if (IS_ERR(sd_info_cmd_dev))
				pr_err("Fail to create sysfs dev\n");
			if (device_create_file(sd_info_cmd_dev,
						&dev_attr_sd_count) < 0)
				pr_err("Fail to create status sysfs file\n");

			if (device_create_file(sd_info_cmd_dev,
						&dev_attr_data) < 0)
				pr_err("Fail to create status sysfs file\n");

			if (device_create_file(sd_info_cmd_dev,
						&dev_attr_fc) < 0)
				pr_err("Fail to create status sysfs file\n");
		}

		if (!sd_data_cmd_dev) {
			sd_data_cmd_dev = sec_device_create(host, "sddata");
			if (IS_ERR(sd_data_cmd_dev))
				pr_err("Fail to create sysfs dev\n");
			if (device_create_file(sd_data_cmd_dev,
						&dev_attr_sd_data) < 0)
				pr_err("Fail to create status sysfs file\n");
		}
	}
}

/* Common capabilities of Exynos4/Exynos5 SoC */
static unsigned long exynos_dwmmc_caps[4] = {
	MMC_CAP_1_8V_DDR | MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
	MMC_CAP_CMD23,
};

static int dw_mci_exynos_misc_control(struct dw_mci *host,
				      enum dw_mci_misc_control control, void *priv)
{
	int ret = 0;

	switch (control) {
	case CTRL_RESTORE_CLKSEL:
		dw_mci_exynos_set_sample(host, host->pdata->clk_smpl, false);
		dw_mci_set_fine_tuning_bit(host, host->pdata->is_fine_tuned);
		break;
	case CTRL_REQUEST_EXT_IRQ:
		ret = dw_mci_exynos_request_ext_irq(host, (irq_handler_t) priv);
		break;
	case CTRL_CHECK_CD:
		ret = dw_mci_exynos_check_cd(host);
		break;
		case CTRL_ADD_SYSFS:
			dw_mci_exynos_add_sysfs(host);
			break;
	default:
		dev_err(host->dev, "dw_mmc exynos: wrong case\n");
		ret = -ENODEV;
	}
	return ret;
}

#ifdef CONFIG_MMC_DW_EXYNOS_FMP
static int dw_mci_exynos_crypto_engine_cfg(struct dw_mci *host,
					   void *desc,
					   struct mmc_data *data,
					   struct page *page, int sector_offset, bool cmdq_enabled)
{
	return exynos_mmc_fmp_cfg(host, desc, data, page, sector_offset, cmdq_enabled);
}

static int dw_mci_exynos_crypto_engine_clear(struct dw_mci *host, void *desc, bool cmdq_enabled)
{
	return exynos_mmc_fmp_clear(host, desc, cmdq_enabled);
}

static int dw_mci_exynos_access_control_get_dev(struct dw_mci *host)
{
	return exynos_mmc_smu_get_dev(host);
}

static int dw_mci_exynos_access_control_sec_cfg(struct dw_mci *host)
{
	return exynos_mmc_smu_sec_cfg(host);
}

static int dw_mci_exynos_access_control_init(struct dw_mci *host)
{
	return exynos_mmc_smu_init(host);
}

static int dw_mci_exynos_access_control_abort(struct dw_mci *host)
{
	return exynos_mmc_smu_abort(host);
}

static int dw_mci_exynos_access_control_resume(struct dw_mci *host)
{
	return exynos_mmc_smu_resume(host);
}
#endif

static const struct dw_mci_drv_data exynos_drv_data = {
	.caps = exynos_dwmmc_caps,
	.num_caps		= ARRAY_SIZE(exynos_dwmmc_caps),
	.init = dw_mci_exynos_priv_init,
	.set_ios = dw_mci_exynos_set_ios,
	.parse_dt = dw_mci_exynos_parse_dt,
	.execute_tuning = dw_mci_exynos_execute_tuning,
	.hwacg_control = dw_mci_card_int_hwacg_ctrl,
	.pins_control = dw_mci_set_pins_state,
	.misc_control = dw_mci_exynos_misc_control,
#ifdef CONFIG_MMC_DW_EXYNOS_FMP
	.crypto_engine_cfg = dw_mci_exynos_crypto_engine_cfg,
	.crypto_engine_clear = dw_mci_exynos_crypto_engine_clear,
	.access_control_get_dev = dw_mci_exynos_access_control_get_dev,
	.access_control_sec_cfg = dw_mci_exynos_access_control_sec_cfg,
	.access_control_init = dw_mci_exynos_access_control_init,
	.access_control_abort = dw_mci_exynos_access_control_abort,
	.access_control_resume = dw_mci_exynos_access_control_resume,
#endif
	.ssclk_control = dw_mci_exynos_ssclk_control,
};

static const struct of_device_id dw_mci_exynos_match[] = {
	{.compatible = "samsung,exynos-dw-mshc",
	 .data = &exynos_drv_data,},
	{},
};

MODULE_DEVICE_TABLE(of, dw_mci_exynos_match);

static int dw_mci_exynos_probe(struct platform_device *pdev)
{
	const struct dw_mci_drv_data *drv_data;
	const struct of_device_id *match;
	int ret;

	match = of_match_node(dw_mci_exynos_match, pdev->dev.of_node);
	drv_data = match->data;

	pm_runtime_get_noresume(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = dw_mci_pltfm_register(pdev, drv_data);
	if (ret) {
		pm_runtime_disable(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
		pm_runtime_put_noidle(&pdev->dev);

		return ret;
	}

	return 0;
}

static int dw_mci_exynos_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
	pm_runtime_put_noidle(&pdev->dev);

	return dw_mci_pltfm_remove(pdev);
}

static const struct dev_pm_ops dw_mci_exynos_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
	    SET_RUNTIME_PM_OPS(dw_mci_runtime_suspend,
			       dw_mci_exynos_runtime_resume,
			       NULL)
	    .resume_noirq = dw_mci_exynos_resume_noirq,
	.thaw_noirq = dw_mci_exynos_resume_noirq,
	.restore_noirq = dw_mci_exynos_resume_noirq,
};

static struct platform_driver dw_mci_exynos_pltfm_driver = {
	.probe = dw_mci_exynos_probe,
	.remove = dw_mci_exynos_remove,
	.driver = {
		   .name = "dwmmc_exynos",
		   .of_match_table = dw_mci_exynos_match,
		   .pm = &dw_mci_exynos_pmops,
		   .suppress_bind_attrs = true,
		   },
};

module_platform_driver(dw_mci_exynos_pltfm_driver);

MODULE_DESCRIPTION("Samsung Specific DW-MSHC Driver Extension");
MODULE_AUTHOR("Thomas Abraham <thomas.ab@samsung.com");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:dwmmc_exynos");
