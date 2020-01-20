/*
 *
 * drivers/media/tdmb/tdmb_ebi.c
 *
 * tdmb driver
 *
 * Copyright (C) (2011, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-srom.h>
#include <mach/gpio.h>

#include "tdmb.h"

static struct tdmb_ebi_dt_data *ebi_dt_pdata;
static void __iomem *v_addr_ebi_cs_base;


/* SROMC bank attributes in BW (Bus width and Wait control) register */
enum sromc_bank_attr {
	SROMC_DATA_16   = 0x1,	/* 16-bit data bus	*/
	SROMC_BYTE_ADDR = 0x2,	/* Byte base address	*/
	SROMC_WAIT_EN   = 0x4,	/* Wait enabled		*/
	SROMC_BYTE_EN   = 0x8,	/* Byte access enabled	*/
	SROMC_ATTR_MASK = 0xF
};

/* SROMC bank configuration */
struct sromc_bank_cfg {
	unsigned int csn;		/* CSn #			*/
	unsigned int attr;		/* SROMC bank attributes	*/
	unsigned int size;		/* Size of a memory		*/
	unsigned int addr;		/* Start address (physical)	*/
};

/* SROMC bank access timing configuration */
struct sromc_timing_cfg {
	u32 tacs;		/* Address set-up before CSn		*/
	u32 tcos;		/* Chip selection set-up before OEn	*/
	u32 tacc;		/* Access cycle				*/
	u32 tcoh;		/* Chip selection hold on OEn		*/
	u32 tcah;		/* Address holding time after CSn	*/
	u32 tacp;		/* Page mode access cycle at Page mode	*/
	u32 pmc;		/* Page Mode config			*/
};

/**
 * sromc_enable
 *
 * Enables SROM controller (SROMC) block
 *
 */
static int sromc_enable(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct clk *clk;

	clk = devm_clk_get(dev, "sromc_clk");
	if (IS_ERR(clk)) {
		DPRINTK("Failed to get tdmb_sromc clock\n");
		return -1;
	}

	if (clk_prepare_enable(clk)) {
		DPRINTK("%s: clk_prepare_enable failed\n", __func__);
		return -1;
	}
	return 0;
}

/**
 * sromc_config_demux_gpio
 *
 * Configures GPIO pins for REn, WEn, LBn, UBn, address bus, and data bus
 * as demux mode
 *
 * Returns 0 if there is no error
 *
 */
static int sromc_config_demux_gpio(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pinctrl *pinctrl;

	pinctrl = devm_pinctrl_get_select_default(dev);
	if (IS_ERR(pinctrl))
		DPRINTK("%s: Failed to configure pinctrl\n", __func__);

	return 0;
}

/**
 * sromc_config_access_attr
 * @csn: CSn number
 * @attr: SROMC attribute for this CSn
 *
 * Configures SROMC attribute for a CSn
 *
 */
static void sromc_config_access_attr(unsigned int csn, unsigned int attr)
{
	unsigned int bw = 0;	/* Bus width and Wait control */

	DPRINTK("%s: for CSn%d\n", __func__, csn);

	bw = __raw_readl(S5P_SROM_BW);
	DPRINTK("%s: old BW setting = 0x%08X\n", __func__, bw);

	/* Configure BW control field for the CSn */
	bw &= ~(SROMC_ATTR_MASK << (csn << 2));
	bw |= (attr << (csn << 2));
	writel(bw, S5P_SROM_BW);

	/* Verify SROMC settings */
	bw = __raw_readl(S5P_SROM_BW);
	DPRINTK("%s: new BW setting = 0x%08X\n", __func__, bw);
}

/**
 * sromc_config_access_timing
 * @csn: CSn number
 * @tm_cfg: pointer to an sromc_timing_cfg
 *
 * Configures SROMC access timing register
 *
 */
static void sromc_config_access_timing(unsigned int csn,
				struct sromc_timing_cfg *tm_cfg)
{
	void __iomem *bank_sfr = S5P_SROM_BC0 + (4 * csn);
	unsigned int bc = 0;	/* Bank Control */

	bc = __raw_readl(bank_sfr);
	DPRINTK("%s: old BC%d setting = 0x%08X\n", __func__, csn, bc);

	/* Configure memory access timing for the CSn */
	bc = tm_cfg->tacs | tm_cfg->tcos | tm_cfg->tacc |
	     tm_cfg->tcoh | tm_cfg->tcah | tm_cfg->tacp | tm_cfg->pmc;
	writel(bc, bank_sfr);

	/* Verify SROMC settings */
	bc = __raw_readl(bank_sfr);
	DPRINTK("%s: new BC%d setting = 0x%08X\n", __func__, csn, bc);
}

static struct sromc_bank_cfg tdmb_sromc_bank_cfg = {
	.csn = 0,
	.attr = 0,
};

static struct sromc_timing_cfg tdmb_sromc_timing_cfg = {
	.tacs = 0x0F << 28,
	.tcos = 0x02 << 24,
	.tacc = 0x1F << 16,
	.tcoh = 0x02 << 12,
	.tcah = 0x0F << 8,
	.tacp = 0x00 << 4,
	.pmc  = 0x00 << 0,
};

unsigned long tdmb_get_if_handle(void)
{
	return (unsigned long)v_addr_ebi_cs_base;
}
EXPORT_SYMBOL_GPL(tdmb_get_if_handle);

static int tdmb_ebi_init(struct platform_device *pdev)
{
	struct sromc_bank_cfg *bnk_cfg;
	struct sromc_timing_cfg *tm_cfg;

	if (sromc_enable(pdev) < 0) {
		DPRINTK("tdmb_dev_init sromc_enable fail\n");
		return -1;
	}

	if (sromc_config_demux_gpio(pdev) < 0) {
		DPRINTK("tdmb_dev_init sromc_config_demux_gpio fail\n");
		return -1;
	}

	bnk_cfg = &tdmb_sromc_bank_cfg;
	sromc_config_access_attr(bnk_cfg->csn, bnk_cfg->attr);

	tm_cfg = &tdmb_sromc_timing_cfg;
	sromc_config_access_timing(bnk_cfg->csn, tm_cfg);
	return 0;
}
static struct tdmb_ebi_dt_data *get_ebi_dt_pdata(struct device *dev)
{
	struct tdmb_ebi_dt_data *pdata;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		DPRINTK("%s : could not allocate memory for platform data\n", __func__);
		return NULL;
	}

	if (!of_property_read_u32(dev->of_node, "srom_cs_base", &pdata->cs_base))
		DPRINTK("%s - srom_cs_base : 0x%x\n", __func__, pdata->cs_base);
	else {
		DPRINTK("%s - cs_base missing\n", __func__);
		goto fail;
	}

	if (!of_property_read_u32(dev->of_node, "srom_mem_size", &pdata->mem_size))
		DPRINTK("%s - srom_mem_size : 0x%x\n", __func__, pdata->mem_size);
	else {
		DPRINTK("%s - mem_size missing\n", __func__);
		goto fail;
	}

	return pdata;

fail:
	devm_kfree(dev, pdata);
	return NULL;

}

static int tdmb_sromc_remove(struct platform_device *pdev)
{
	DPRINTK("%s!\n", __func__);
	return 0;
}

static int tdmb_sromc_probe(struct platform_device *pdev)
{
	if (pdev->dev.of_node) {
		ebi_dt_pdata = get_ebi_dt_pdata(&pdev->dev);
		if (!ebi_dt_pdata) {
			DPRINTK("%s : ebi_dt_pdata is NULL\n", __func__);
			return -1;
		}
	}

	if (tdmb_ebi_init(pdev) < 0) {
		DPRINTK("%s : tdmb_ebi_init error\n", __func__);
		return -1;
	}

	v_addr_ebi_cs_base = ioremap(ebi_dt_pdata->cs_base, ebi_dt_pdata->mem_size);
	DPRINTK("%s : v_addr_ebi_cs_base 0x%p\n", __func__, v_addr_ebi_cs_base);
	return 0;
}

static const struct of_device_id sromc_match_table[] = {
	{.compatible = "samsung,sromc"},
	{}
};

static struct platform_driver tdmb_ebi_driver = {
	.driver = {
		.name	= "sromc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sromc_match_table),
	},
	.remove = tdmb_sromc_remove,
};

int __init tdmb_sromc_init(void)
{
	return platform_driver_probe(&tdmb_ebi_driver, tdmb_sromc_probe);
}
module_init(tdmb_sromc_init);

static void __exit tdmb_sromc_exit(void)
{
	platform_driver_unregister(&tdmb_ebi_driver);
}
module_exit(tdmb_sromc_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("SROMC Driver");
MODULE_LICENSE("GPL v2");
