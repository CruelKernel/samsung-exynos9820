/*
 * UFS Host Controller driver for Exynos specific extensions
 *
 * Copyright (C) 2013-2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/smc.h>
#include <soc/samsung/exynos-pm.h>
#include <soc/samsung/exynos-cpupm.h>
#include "ufshcd.h"
#include "unipro.h"
#include "mphy.h"
#include "ufshcd-pltfrm.h"
#include "ufs-exynos.h"
#include "ufs-exynos-fmp.h"
#include <soc/samsung/exynos-pmu.h>

/*
 * Unipro attribute value
 */
#define TXTRAILINGCLOCKS	0x10
#define TACTIVATE_10_USEC	400	/* unit: 10us */

/* Device ID */
#define DEV_ID	0x00
#define PEER_DEV_ID	0x01
#define PEER_CPORT_ID	0x00
#define TRAFFIC_CLASS	0x00

#define IATOVAL_NSEC		20000	/* unit: ns */

/* UFS CAL interface */

/*
 * Debugging information, SFR/attributes/misc
 */
static struct exynos_ufs *ufs_host_backup[1];
static int ufs_host_index = 0;

static struct exynos_ufs_sfr_log ufs_log_std_sfr[] = {
	{"CAPABILITIES"			,	REG_CONTROLLER_CAPABILITIES,	0},
	{"UFS VERSION"			,	REG_UFS_VERSION,		0},
	{"PRODUCT ID"			,	REG_CONTROLLER_DEV_ID,		0},
	{"MANUFACTURE ID"		,	REG_CONTROLLER_PROD_ID,		0},
	{"INTERRUPT STATUS"		,	REG_INTERRUPT_STATUS,		0},
	{"INTERRUPT ENABLE"		,	REG_INTERRUPT_ENABLE,		0},
	{"CONTROLLER STATUS"		,	REG_CONTROLLER_STATUS,		0},
	{"CONTROLLER ENABLE"		,	REG_CONTROLLER_ENABLE,		0},
	{"UTP TRANSF REQ INT AGG CNTRL"	,	REG_UTP_TRANSFER_REQ_INT_AGG_CONTROL,		0},
	{"UTP TRANSF REQ LIST BASE L"	,	REG_UTP_TRANSFER_REQ_LIST_BASE_L,		0},
	{"UTP TRANSF REQ LIST BASE H"	,	REG_UTP_TRANSFER_REQ_LIST_BASE_H,		0},
	{"UTP TRANSF REQ DOOR BELL"	,	REG_UTP_TRANSFER_REQ_DOOR_BELL,		0},
	{"UTP TRANSF REQ LIST CLEAR"	,	REG_UTP_TRANSFER_REQ_LIST_CLEAR,		0},
	{"UTP TRANSF REQ LIST RUN STOP"	,	REG_UTP_TRANSFER_REQ_LIST_RUN_STOP,		0},
	{"UTP TASK REQ LIST BASE L"	,	REG_UTP_TASK_REQ_LIST_BASE_L,		0},
	{"UTP TASK REQ LIST BASE H"	,	REG_UTP_TASK_REQ_LIST_BASE_H,		0},
	{"UTP TASK REQ DOOR BELL"	,	REG_UTP_TASK_REQ_DOOR_BELL,		0},
	{"UTP TASK REQ LIST CLEAR"	,	REG_UTP_TASK_REQ_LIST_CLEAR,		0},
	{"UTP TASK REQ LIST RUN STOP"	,	REG_UTP_TASK_REQ_LIST_RUN_STOP,		0},
	{"UIC COMMAND"			,	REG_UIC_COMMAND,		0},
	{"UIC COMMAND ARG1"		,	REG_UIC_COMMAND_ARG_1,		0},
	{"UIC COMMAND ARG2"		,	REG_UIC_COMMAND_ARG_2,		0},
	{"UIC COMMAND ARG3"		,	REG_UIC_COMMAND_ARG_3,		0},

	{},
};

/* Helper for UFS CAL interface */
static inline int ufs_init_cal(struct exynos_ufs *ufs, int idx,
					struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct ufs_cal_param *p = NULL;

	p = devm_kzalloc(dev, sizeof(*p), GFP_KERNEL);
	if (!p) {
		dev_err(ufs->dev, "cannot allocate mem for cal param\n");
		return -ENOMEM;
	}
	ufs->cal_param = p;

	p->host = ufs;
	p->board = 0;	/* ken: need a dt node for board */
	if ((ret = ufs_cal_init(p, idx)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_init_cal = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

static void exynos_ufs_update_active_lanes(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct ufs_cal_param *p = ufs->cal_param;
	u32 active_tx_lane = 0;
	u32 active_rx_lane = 0;

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_ACTIVETXDATALANES), &(active_tx_lane));
	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_ACTIVERXDATALANES), &(active_rx_lane));

	p->active_tx_lane = (u8) active_tx_lane;
	p->active_rx_lane = (u8) active_rx_lane;

	dev_info(ufs->dev, "active_tx_lane(%d), active_rx_lane(%d)\n", p->active_tx_lane, p->active_rx_lane);
}

static void exynos_ufs_update_max_gear(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct uic_pwr_mode *pmd = &ufs->req_pmd_parm;
	struct ufs_cal_param *p = ufs->cal_param;
	u32 max_rx_hs_gear = 0;

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_MAXRXHSGEAR), &(max_rx_hs_gear));

	p->max_gear = min_t(u8, max_rx_hs_gear, pmd->gear);

	dev_info(ufs->dev, "max_gear(%d)\n", p->max_gear);
}

static inline int ufs_pre_link(struct exynos_ufs *ufs)
{
	int ret = 0;
	struct ufs_cal_param *p = ufs->cal_param;

	p->mclk_rate = ufs->mclk_rate;
	p->available_lane = ufs->num_rx_lanes;

	if ((ret = ufs_cal_pre_link(p)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_pre_link = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

static inline int ufs_post_link(struct exynos_ufs *ufs)
{
	int ret = 0;

	/* update active lanes after link*/
	exynos_ufs_update_active_lanes(ufs->hba);

	/* update max gear after link*/
	exynos_ufs_update_max_gear(ufs->hba);

	if ((ret = ufs_cal_post_link(ufs->cal_param)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_post_link = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

static inline int ufs_pre_gear_change(struct exynos_ufs *ufs,
				struct uic_pwr_mode *pmd)
{
	struct ufs_cal_param *p = ufs->cal_param;
	int ret = 0;

	p->pmd = pmd;
	if ((ret = ufs_cal_pre_pmc(p)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_pre_gear_change = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

static inline int ufs_post_gear_change(struct exynos_ufs *ufs)
{
	int ret = 0;

	if ((ret = ufs_cal_post_pmc(ufs->cal_param)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_post_gear_change = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

static inline int ufs_post_h8_enter(struct exynos_ufs *ufs)
{
	int ret = 0;

	if ((ret = ufs_cal_post_h8_enter(ufs->cal_param)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_post_h8_enter = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

static inline int ufs_pre_h8_exit(struct exynos_ufs *ufs)
{
	int ret = 0;

	if ((ret = ufs_cal_pre_h8_exit(ufs->cal_param)) != UFS_CAL_NO_ERROR) {
		dev_err(ufs->dev, "ufs_pre_h8_exit = %d!!!\n", ret);
		return -EPERM;
	}

	return 0;
}

/* Adaptor for UFS CAL */
void ufs_lld_dme_set(void *h, u32 addr, u32 val)
{
	ufshcd_dme_set(((struct exynos_ufs *)h)->hba, addr, val);
}

void ufs_lld_dme_get(void *h, u32 addr, u32 *val)
{
	ufshcd_dme_get(((struct exynos_ufs *)h)->hba, addr, val);
}

void ufs_lld_dme_peer_set(void *h, u32 addr, u32 val)
{
	ufshcd_dme_peer_set(((struct exynos_ufs *)h)->hba, addr, val);
}

void ufs_lld_pma_write(void *h, u32 val, u32 addr)
{
	phy_pma_writel((struct exynos_ufs *)h, val, addr);
}

u32 ufs_lld_pma_read(void *h, u32 addr)
{
	return phy_pma_readl((struct exynos_ufs *)h, addr);
}

void ufs_lld_unipro_write(void *h, u32 val, u32 addr)
{
	unipro_writel((struct exynos_ufs *)h, val, addr);
}

void ufs_lld_udelay(u32 val)
{
	udelay(val);
}

void ufs_lld_usleep_delay(u32 min, u32 max)
{
	usleep_range(min, max);
}

unsigned long ufs_lld_get_time_count(unsigned long offset)
{
	return jiffies;
}

unsigned long ufs_lld_calc_timeout(const unsigned int ms)
{
	return msecs_to_jiffies(ms);
}

static inline void exynos_ufs_ctrl_phy_pwr(struct exynos_ufs *ufs, bool en)
{
	exynos_pmu_update(EXYNOS_PMU_UFS_PHY_OFFSET, 0x1 << 0, (en ? 1 : 0) << 0);
	dev_info(ufs->dev, "exynos_ufs_ctrl_phy_pwr = %d\n", (int)en);
}

#ifndef __EXYNOS_UFS_VS_DEBUG__
static void exynos_ufs_dump_std_sfr(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct exynos_ufs_sfr_log* cfg = ufs->debug.std_sfr;

	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": \t\tREGISTER DUMP\n");
	dev_err(hba->dev, ": --------------------------------------------------- \n");

	while(cfg) {
		if (!cfg->name)
			break;
		cfg->val = ufshcd_readl(hba, cfg->offset);

		/* Dump */
		dev_err(hba->dev, ": %s(0x%04x):\t\t\t\t0x%08x\n",
				cfg->name, cfg->offset, cfg->val);

		/* Next SFR */
		cfg++;
	}
}
#endif

/*
 * Exynos debugging main function
 */
static void exynos_ufs_dump_debug_info(struct ufs_hba *hba)
{
#ifdef __EXYNOS_UFS_VS_DEBUG__
	exynos_ufs_get_uic_info(hba);
#else
	exynos_ufs_dump_std_sfr(hba);
#endif
}

static void exynos_ufs_select_refclk(struct exynos_ufs *ufs, bool en)
{
	u32 reg;
	if (ufs->hw_rev != UFS_VER_0004)
		return;

	/*
	 * true : alternative clock path, false : active clock path
	 */
	reg = hci_readl(ufs, HCI_MPHY_REFCLK_SEL);
	if (en)
		hci_writel(ufs, reg | MPHY_REFCLK_SEL, HCI_MPHY_REFCLK_SEL);
	else
		hci_writel(ufs, reg & ~MPHY_REFCLK_SEL, HCI_MPHY_REFCLK_SEL);
}

inline void exynos_ufs_set_hwacg_control(struct exynos_ufs *ufs, bool en)
{
	u32 reg;
	if ((ufs->hw_rev != UFS_VER_0004) && (ufs->hw_rev != UFS_VER_0005) && (ufs->hw_rev != UFS_VER_0006))
		return;

	/*
	 * default value 1->0 at KC. so,
	 * need to set "1(disable HWACG)" during UFS init
	 */
	reg = hci_readl(ufs, HCI_UFS_ACG_DISABLE);
	if (en)
		hci_writel(ufs, reg & (~HCI_UFS_ACG_DISABLE_EN), HCI_UFS_ACG_DISABLE);
	else
		hci_writel(ufs, reg | HCI_UFS_ACG_DISABLE_EN, HCI_UFS_ACG_DISABLE);

}

inline void exynos_ufs_ctrl_auto_hci_clk(struct exynos_ufs *ufs, bool en)
{
	u32 reg = hci_readl(ufs, HCI_FORCE_HCS);

	if (en)
		hci_writel(ufs, reg | HCI_CORECLK_STOP_EN, HCI_FORCE_HCS);
	else
		hci_writel(ufs, reg & ~HCI_CORECLK_STOP_EN, HCI_FORCE_HCS);
}

static inline void exynos_ufs_ctrl_clk(struct exynos_ufs *ufs, bool en)
{
	u32 reg = hci_readl(ufs, HCI_FORCE_HCS);

	if (en)
		hci_writel(ufs, reg | CLK_STOP_CTRL_EN_ALL, HCI_FORCE_HCS);
	else
		hci_writel(ufs, reg & ~CLK_STOP_CTRL_EN_ALL, HCI_FORCE_HCS);
}

static inline void exynos_ufs_gate_clk(struct exynos_ufs *ufs, bool en)
{

	u32 reg = hci_readl(ufs, HCI_CLKSTOP_CTRL);

	if (en)
		hci_writel(ufs, reg | CLK_STOP_ALL, HCI_CLKSTOP_CTRL);
	else
		hci_writel(ufs, reg & ~CLK_STOP_ALL, HCI_CLKSTOP_CTRL);
}

static void exynos_ufs_set_unipro_mclk(struct exynos_ufs *ufs)
{
	ufs->mclk_rate = (u32)clk_get_rate(ufs->clk_unipro);
}

static void exynos_ufs_fit_aggr_timeout(struct exynos_ufs *ufs)
{
	u32 cnt_val;
	unsigned long nVal;

	/* IA_TICK_SEL : 1(1us_TO_CNT_VAL) */
	nVal = hci_readl(ufs, HCI_UFSHCI_V2P1_CTRL);
	nVal |= IA_TICK_SEL;
	hci_writel(ufs, nVal, HCI_UFSHCI_V2P1_CTRL);

	cnt_val = ufs->mclk_rate / 1000000 ;
	hci_writel(ufs, cnt_val & CNT_VAL_1US_MASK, HCI_1US_TO_CNT_VAL);
}

static void exynos_ufs_init_pmc_req(struct ufs_hba *hba,
		struct ufs_pa_layer_attr *pwr_max,
		struct ufs_pa_layer_attr *pwr_req)
{

	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct uic_pwr_mode *req_pmd = &ufs->req_pmd_parm;
	struct uic_pwr_mode *act_pmd = &ufs->act_pmd_parm;
	struct ufs_cal_param *p = ufs->cal_param;

	/* update lane variable after link */
	ufs->num_rx_lanes = pwr_max->lane_rx;
	ufs->num_tx_lanes = pwr_max->lane_tx;

	p->connected_rx_lane = pwr_max->lane_rx;
	p->connected_tx_lane = pwr_max->lane_tx;

	pwr_req->gear_rx
		= act_pmd->gear= min_t(u8, pwr_max->gear_rx, req_pmd->gear);
	pwr_req->gear_tx
		= act_pmd->gear = min_t(u8, pwr_max->gear_tx, req_pmd->gear);
	pwr_req->lane_rx
		= act_pmd->lane = min_t(u8, pwr_max->lane_rx, req_pmd->lane);
	pwr_req->lane_tx
		= act_pmd->lane = min_t(u8, pwr_max->lane_tx, req_pmd->lane);
	pwr_req->pwr_rx = act_pmd->mode = req_pmd->mode;
	pwr_req->pwr_tx = act_pmd->mode = req_pmd->mode;
	pwr_req->hs_rate = act_pmd->hs_series = req_pmd->hs_series;
}

static void exynos_ufs_config_intr(struct exynos_ufs *ufs, u32 errs, u8 index)
{
	switch(index) {
	case UNIP_PA_LYR:
		hci_writel(ufs, DFES_ERR_EN | errs, HCI_ERROR_EN_PA_LAYER);
		break;
	case UNIP_DL_LYR:
		hci_writel(ufs, DFES_ERR_EN | errs, HCI_ERROR_EN_DL_LAYER);
		break;
	case UNIP_N_LYR:
		hci_writel(ufs, DFES_ERR_EN | errs, HCI_ERROR_EN_N_LAYER);
		break;
	case UNIP_T_LYR:
		hci_writel(ufs, DFES_ERR_EN | errs, HCI_ERROR_EN_T_LAYER);
		break;
	case UNIP_DME_LYR:
		hci_writel(ufs, DFES_ERR_EN | errs, HCI_ERROR_EN_DME_LAYER);
		break;
	}
}

static void exynos_ufs_dev_hw_reset(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	/* bit[1] for resetn */
	hci_writel(ufs, 0 << 0, HCI_GPIO_OUT);
	udelay(5);
	hci_writel(ufs, 1 << 0, HCI_GPIO_OUT);
}

static void exynos_ufs_init_host(struct exynos_ufs *ufs)
{
	u32 reg;

	/* internal clock control */
	exynos_ufs_ctrl_auto_hci_clk(ufs, false);
	exynos_ufs_set_unipro_mclk(ufs);

	/* period for interrupt aggregation */
	exynos_ufs_fit_aggr_timeout(ufs);

	/* misc HCI configurations */
	hci_writel(ufs, 0xA, HCI_DATA_REORDER);
	hci_writel(ufs, PRDT_PREFECT_EN | PRDT_SET_SIZE(12),
			HCI_TXPRDT_ENTRY_SIZE);
	hci_writel(ufs, PRDT_SET_SIZE(12), HCI_RXPRDT_ENTRY_SIZE);
	hci_writel(ufs, 0xFFFFFFFF, HCI_UTRL_NEXUS_TYPE);
	hci_writel(ufs, 0xFFFFFFFF, HCI_UTMRL_NEXUS_TYPE);

	reg = hci_readl(ufs, HCI_AXIDMA_RWDATA_BURST_LEN) &
					~BURST_LEN(0);
	hci_writel(ufs, WLU_EN | BURST_LEN(3),
					HCI_AXIDMA_RWDATA_BURST_LEN);

	/*
	 * Enable HWAGC control by IOP
	 *
	 * default value 1->0 at KC.
	 * always "0"(controlled by UFS_ACG_DISABLE)
	 */
	reg = hci_readl(ufs, HCI_IOP_ACG_DISABLE);
	hci_writel(ufs, reg & (~HCI_IOP_ACG_DISABLE_EN), HCI_IOP_ACG_DISABLE);
}

static void exynos_ufs_pre_hibern8(struct ufs_hba *hba, u8 enter)
{
}

static void exynos_ufs_post_hibern8(struct ufs_hba *hba, u8 enter)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	if (!enter) {
		struct uic_pwr_mode *act_pmd = &ufs->act_pmd_parm;
		u32 mode = 0;

		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_PWRMODE), &mode);
		if (mode != (act_pmd->mode << 4 | act_pmd->mode)) {
			dev_warn(hba->dev, "%s: power mode not matched, mode : 0x%x, act_mode : 0x%x\n",
					__func__, mode, act_pmd->mode);
			hba->pwr_info.pwr_rx = (mode >> 4) & 0xf;
			hba->pwr_info.pwr_tx = mode & 0xf;
			ufshcd_config_pwr_mode(hba, &hba->max_pwr_info.info);
		}
	}
}

static void exynos_ufs_modify_sysreg(struct exynos_ufs *ufs, int index)
{
	struct exynos_ufs_sys *sys = &ufs->sys;
	void __iomem *reg_sys = sys->reg_sys[index];
	const char *const name[NUM_OF_SYSREG] = {
		"ufs-io-coherency",
	};
	u32 reg;

	if (!of_get_child_by_name(ufs->dev->of_node, name[index]))
		return;

	reg = readl(reg_sys);
	writel((reg & ~(sys->mask[index])) | sys->bits[index], reg_sys);
}

static int exynos_ufs_init_system(struct exynos_ufs *ufs)
{
	struct device *dev = ufs->dev;
	int ret = 0;

	/* PHY isolation bypass */
	exynos_ufs_ctrl_phy_pwr(ufs, true);

	/* IO cohernecy */
	if (!of_get_child_by_name(dev->of_node, "ufs-io-coherency")) {
		dev_err(dev, "Not configured to use IO coherency\n");
	} else {
		if (!of_find_property(dev->of_node, "dma-coherent", NULL))
			BUG();

		exynos_ufs_modify_sysreg(ufs, 0);
	}

	return ret;
}

static int exynos_ufs_get_clks(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct list_head *head = &hba->clk_list_head;
	struct ufs_clk_info *clki;

	ufs_host_backup[ufs_host_index++] = ufs;
	ufs->debug.std_sfr = ufs_log_std_sfr;

	if (!head || list_empty(head))
		goto out;

	list_for_each_entry(clki, head, list) {
		if (!IS_ERR_OR_NULL(clki->clk)) {
			if (!strcmp(clki->name, "GATE_UFS_EMBD"))
				ufs->clk_hci = clki->clk;
			if (!strcmp(clki->name, "UFS_EMBD")) {
				ufs->clk_unipro = clki->clk;
			}
		}
	}
out:
	if (!ufs->clk_hci || !ufs->clk_unipro)
		return -EINVAL;

	return 0;
}

static void exynos_ufs_set_features(struct ufs_hba *hba, u32 hw_rev)
{
	/* caps */
	hba->caps = UFSHCD_CAP_CLK_GATING |
			UFSHCD_CAP_HIBERN8_WITH_CLK_GATING |
			UFSHCD_CAP_INTR_AGGR;

	/* quirks of common driver */
	hba->quirks = UFSHCD_QUIRK_PRDT_BYTE_GRAN |
			UFSHCI_QUIRK_SKIP_INTR_AGGR |
			UFSHCD_QUIRK_UNRESET_INTR_AGGR |
			UFSHCD_QUIRK_BROKEN_REQ_LIST_CLR;

	/* quirks of exynos-specific driver */
}

/*
 * Exynos-specific callback functions
 *
 * init			| Pure SW init & system-related init
 * host_reset		| Host SW reset & init
 * pre_setup_clocks	| specific power down
 * setup_clocks		| specific power up
 * ...
 *
 * Initializations for software, host controller and system
 * should be contained only in ->host_reset() as possible.
 */

static int exynos_ufs_init(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int ret;
	int id;

	/* set features, such as caps or quirks */
	exynos_ufs_set_features(hba, ufs->hw_rev);

	/* get some clock sources and debug infomation structures */
	ret = exynos_ufs_get_clks(hba);
	if (ret)
		return ret;

	/* system init */
	ret = exynos_ufs_init_system(ufs);
	if (ret)
		return ret;

	/* get fmp & smu id */
	ret = of_property_read_u32(ufs->dev->of_node, "fmp-id", &id);
	if (ret)
		ufs->fmp = SMU_ID_MAX;
	else
		ufs->fmp = id;

	ret = of_property_read_u32(ufs->dev->of_node, "smu-id", &id);
	if (ret)
		ufs->smu = SMU_ID_MAX;
	else
		ufs->smu = id;

	/* FMPSECURITY & SMU */
	exynos_ufs_fmp_sec_cfg(ufs);
	exynos_ufs_smu_init(ufs);

	/* Enable log */
	ret =  exynos_ufs_init_dbg(hba);

	if (ret)
		return ret;

	ufs->misc_flags = EXYNOS_UFS_MISC_TOGGLE_LOG;

	return 0;
}

static void exynos_ufs_host_reset(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	unsigned long timeout = jiffies + msecs_to_jiffies(1);

	exynos_ufs_ctrl_auto_hci_clk(ufs, false);

	hci_writel(ufs, UFS_SW_RST_MASK, HCI_SW_RST);

	do {
		if (!(hci_readl(ufs, HCI_SW_RST) & UFS_SW_RST_MASK))
			goto success;
	} while (time_before(jiffies, timeout));

	dev_err(ufs->dev, "timeout host sw-reset\n");

	exynos_ufs_dump_uic_info(hba);

	goto out;

success:
	/* host init */
	exynos_ufs_init_host(ufs);

	/* device reset */
	exynos_ufs_dev_hw_reset(hba);

	exynos_ufs_ctrl_cport_log(ufs, true, 0);
out:
	return;
}

static inline void exynos_ufs_dev_reset_ctrl(struct exynos_ufs *ufs, bool en)
{

	if (en)
		hci_writel(ufs, 1 << 0, HCI_GPIO_OUT);
	else
		hci_writel(ufs, 0 << 0, HCI_GPIO_OUT);
}

static int exynos_ufs_pre_setup_clocks(struct ufs_hba *hba, bool on)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int ret = 0;

	if (on) {
#ifdef CONFIG_CPU_IDLE
		exynos_update_ip_idle_status(ufs->idle_ip_index, 0);
#endif
		/*
		 * Now all used blocks would not be turned off in a host.
		 */
		exynos_ufs_ctrl_auto_hci_clk(ufs, false);
		exynos_ufs_gate_clk(ufs, false);

		/* HWAGC disable */
		exynos_ufs_set_hwacg_control(ufs, false);
	} else {
		pm_qos_update_request(&ufs->pm_qos_int, 0);
	}

	return ret;
}

static int exynos_ufs_setup_clocks(struct ufs_hba *hba, bool on)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int ret = 0;

	if (on) {
		pm_qos_update_request(&ufs->pm_qos_int, ufs->pm_qos_int_value);
	} else {
		/*
		 * Now all used blocks would be turned off in a host.
		 */
		exynos_ufs_gate_clk(ufs, true);
		exynos_ufs_ctrl_auto_hci_clk(ufs, true);

		/* HWAGC enable */
		exynos_ufs_set_hwacg_control(ufs, true);

#ifdef CONFIG_CPU_IDLE
		exynos_update_ip_idle_status(ufs->idle_ip_index, 1);
#endif
	}

	return ret;
}

static int exynos_ufs_link_startup_notify(struct ufs_hba *hba,
					enum ufs_notify_change_status status)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int ret = 0;

	switch (status) {
	case PRE_CHANGE:
		/* refer to hba */
		ufs->hba = hba;

		/* hci */
		exynos_ufs_config_intr(ufs, DFES_DEF_DL_ERRS, UNIP_DL_LYR);
		exynos_ufs_config_intr(ufs, DFES_DEF_N_ERRS, UNIP_N_LYR);
		exynos_ufs_config_intr(ufs, DFES_DEF_T_ERRS, UNIP_T_LYR);

		exynos_ufs_ctrl_clk(ufs, true);
		exynos_ufs_select_refclk(ufs, true);
		exynos_ufs_gate_clk(ufs, false);
		exynos_ufs_set_hwacg_control(ufs, false);

		if (ufs->num_rx_lanes == 0 || ufs->num_tx_lanes == 0) {
			ufshcd_dme_get(hba, UIC_ARG_MIB(PA_AVAILRXDATALANES),
					&ufs->num_rx_lanes);
			ufshcd_dme_get(hba, UIC_ARG_MIB(PA_AVAILTXDATALANES),
					&ufs->num_tx_lanes);
			WARN(ufs->num_rx_lanes != ufs->num_tx_lanes,
					"available data lane is not equal(rx:%d, tx:%d)\n",
					ufs->num_rx_lanes, ufs->num_tx_lanes);
		}

		ufs->mclk_rate = clk_get_rate(ufs->clk_unipro);

		ret = ufs_pre_link(ufs);
		break;
	case POST_CHANGE:
		/* UIC configuration table after link startup */
		ret = ufs_post_link(ufs);
		break;
	default:
		break;
	}

	return ret;
}

static int exynos_ufs_pwr_change_notify(struct ufs_hba *hba,
					enum ufs_notify_change_status status,
					struct ufs_pa_layer_attr *pwr_max,
					struct ufs_pa_layer_attr *pwr_req)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct uic_pwr_mode *act_pmd = &ufs->act_pmd_parm;
	int ret = 0;

	switch (status) {
	case PRE_CHANGE:

		/* Set PMC parameters to be requested */
		exynos_ufs_init_pmc_req(hba, pwr_max, pwr_req);

		/* UIC configuration table before power mode change */
		ret = ufs_pre_gear_change(ufs, act_pmd);

		break;
	case POST_CHANGE:

		/* update active lanes after pmc */
		exynos_ufs_update_active_lanes(hba);

		/* UIC configuration table after power mode change */
		ret = ufs_post_gear_change(ufs);

		dev_info(ufs->dev,
				"Power mode change(%d): M(%d)G(%d)L(%d)HS-series(%d)\n",
				ret, act_pmd->mode, act_pmd->gear,
				act_pmd->lane, act_pmd->hs_series);
		break;
	default:
		break;
	}

	return ret;
}

static void exynos_ufs_set_nexus_t_xfer_req(struct ufs_hba *hba,
				int tag, struct scsi_cmnd *cmd)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	u32 type;

	type =  hci_readl(ufs, HCI_UTRL_NEXUS_TYPE);

	if (cmd)
		type |= (1 << tag);
	else
		type &= ~(1 << tag);

	hci_writel(ufs, type, HCI_UTRL_NEXUS_TYPE);
}

static void exynos_ufs_set_nexus_t_task_mgmt(struct ufs_hba *hba, int tag, u8 tm_func)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	u32 type;

	type =  hci_readl(ufs, HCI_UTMRL_NEXUS_TYPE);

	switch (tm_func) {
	case UFS_ABORT_TASK:
	case UFS_QUERY_TASK:
		type |= (1 << tag);
		break;
	case UFS_ABORT_TASK_SET:
	case UFS_CLEAR_TASK_SET:
	case UFS_LOGICAL_RESET:
	case UFS_QUERY_TASK_SET:
		type &= ~(1 << tag);
		break;
	}

	hci_writel(ufs, type, HCI_UTMRL_NEXUS_TYPE);
}

static void exynos_ufs_hibern8_notify(struct ufs_hba *hba,
				u8 enter, bool notify)
{
	int noti = (int) notify;

	switch (noti) {
	case PRE_CHANGE:
		exynos_ufs_pre_hibern8(hba, enter);
		break;
	case POST_CHANGE:
		exynos_ufs_post_hibern8(hba, enter);
		break;
	default:
		break;
	}
}

static int exynos_ufs_hibern8_prepare(struct ufs_hba *hba,
				u8 enter, bool notify)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int ret = 0;
	int noti = (int) notify;

	switch (noti) {
	case PRE_CHANGE:
		if (!enter)
			ret = ufs_pre_h8_exit(ufs);
		break;
	case POST_CHANGE:
		if (enter)
			ret = ufs_post_h8_enter(ufs);
		break;
	default:
		break;
	}

	return ret;
}

static int __exynos_ufs_reset_ctrl(struct ufs_hba *hba, enum ufs_dev_reset dev_reset)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	exynos_ufs_dev_reset_ctrl(ufs, (bool)dev_reset);

	return 0;
}

static int __exynos_ufs_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	pm_qos_update_request(&ufs->pm_qos_int, 0);

	exynos_ufs_dev_reset_ctrl(ufs, false);

	exynos_ufs_ctrl_phy_pwr(ufs, false);

	return 0;
}

static int __exynos_ufs_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int ret = 0;

	exynos_ufs_ctrl_phy_pwr(ufs, true);

	/* system init */
	ret = exynos_ufs_init_system(ufs);
	if (ret)
		return ret;

	if (ufshcd_is_clkgating_allowed(hba))
		clk_prepare_enable(ufs->clk_hci);
	exynos_ufs_ctrl_auto_hci_clk(ufs, false);

	/* FMPSECURITY & SMU resume */
	exynos_ufs_fmp_sec_cfg(ufs);
	exynos_ufs_smu_resume(ufs);

	exynos_ufs_ctrl_cport_log(ufs, true, 0);

	if (ufshcd_is_clkgating_allowed(hba))
		clk_disable_unprepare(ufs->clk_hci);

	return 0;
}

static u8 exynos_ufs_get_unipro_direct(struct ufs_hba *hba, u32 num)
{
	u32 offset[] = {
		UNIP_DME_LINKSTARTUP_CNF_RESULT,
		UNIP_DME_HIBERN8_ENTER_CNF_RESULT,
		UNIP_DME_HIBERN8_EXIT_CNF_RESULT,
		UNIP_DME_PWR_IND_RESULT,
		UNIP_DME_HIBERN8_ENTER_IND_RESULT,
		UNIP_DME_HIBERN8_EXIT_IND_RESULT
	};

	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	return unipro_readl(ufs, offset[num]);
}

static int exynos_ufs_crypto_engine_cfg(struct ufs_hba *hba,
				struct ufshcd_lrb *lrbp,
				struct scatterlist *sg, int index,
				int sector_offset, int page_index)
{
	return exynos_ufs_fmp_cfg(hba, lrbp, sg, index, sector_offset, page_index);
}

static int exynos_ufs_crypto_engine_clear(struct ufs_hba *hba,
				struct ufshcd_lrb *lrbp)
{
	return exynos_ufs_fmp_clear(hba, lrbp);
}

static int exynos_ufs_access_control_abort(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	return exynos_ufs_smu_abort(ufs);
}

static struct ufs_hba_variant_ops exynos_ufs_ops = {
	.init = exynos_ufs_init,
	.host_reset = exynos_ufs_host_reset,
	.pre_setup_clocks = exynos_ufs_pre_setup_clocks,
	.setup_clocks = exynos_ufs_setup_clocks,
	.link_startup_notify = exynos_ufs_link_startup_notify,
	.pwr_change_notify = exynos_ufs_pwr_change_notify,
	.set_nexus_t_xfer_req = exynos_ufs_set_nexus_t_xfer_req,
	.set_nexus_t_task_mgmt = exynos_ufs_set_nexus_t_task_mgmt,
	.hibern8_notify = exynos_ufs_hibern8_notify,
	.hibern8_prepare = exynos_ufs_hibern8_prepare,
	.dbg_register_dump = exynos_ufs_dump_debug_info,
	.reset_ctrl = __exynos_ufs_reset_ctrl,
	.suspend = __exynos_ufs_suspend,
	.resume = __exynos_ufs_resume,
	.get_unipro_result = exynos_ufs_get_unipro_direct,
	.crypto_engine_cfg = exynos_ufs_crypto_engine_cfg,
	.crypto_engine_clear = exynos_ufs_crypto_engine_clear,
	.access_control_abort = exynos_ufs_access_control_abort,
};

static int exynos_ufs_populate_dt_sys_per_feature(struct device *dev,
				struct exynos_ufs *ufs,	int index)
{
	struct device_node *np;
	struct exynos_ufs_sys *sys = &ufs->sys;
	struct resource io_res;
	int ret;
	const char *const name[NUM_OF_SYSREG] = {
		"ufs-io-coherency",
	};

	np = of_get_child_by_name(dev->of_node, name[index]);
	if (!np) {
		dev_err(dev, "failed to get ufs-sys node\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(np, 0, &io_res);
	if (ret) {
		dev_err(dev, "failed to get i/o address %s\n", name[index]);
		if (ret == -EINVAL)
			ret = 0;
	} else {
		sys->reg_sys[index] = devm_ioremap_resource(dev, &io_res);
		if (IS_ERR(sys->reg_sys[index])) {
			dev_err(dev, "failed to ioremap sysreg\n");
			ret = -ENOMEM;
		} else {
			ret = of_property_read_u32(np, "mask",
						&sys->mask[index]);
			ret = of_property_read_u32(np, "bits",
						&sys->bits[index]);
			if (ret)
				ret = -EINVAL;
		}
	}

	of_node_put(np);

	return ret;
}
static int exynos_ufs_populate_dt_sys(struct device *dev, struct exynos_ufs *ufs)
{
	int i = 0;
	int ret;

	for (i = 0 ; i < NUM_OF_SYSREG ; i++) {
		ret = exynos_ufs_populate_dt_sys_per_feature(dev, ufs, i);
		if (ret && ret != -ENODEV)
			break;
	}

	return ret;
}

static int exynos_ufs_populate_dt_phy(struct device *dev, struct exynos_ufs *ufs)
{
	struct device_node *ufs_phy/* , *phy_sys*/;
	struct exynos_ufs_phy *phy = &ufs->phy;
	struct resource io_res;
	int ret;

	ufs_phy = of_get_child_by_name(dev->of_node, "ufs-phy");
	if (!ufs_phy) {
		dev_err(dev, "failed to get ufs-phy node\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(ufs_phy, 0, &io_res);
	if (ret) {
		dev_err(dev, "failed to get i/o address phy pma\n");
		goto err_0;
	}


	phy->reg_pma = devm_ioremap_resource(dev, &io_res);
	if (!phy->reg_pma) {
		dev_err(dev, "failed to ioremap for phy pma\n");
		ret = -ENOMEM;
		goto err_0;
	}

err_0:
	of_node_put(ufs_phy);

	return ret;
}

static int exynos_ufs_get_pwr_mode(struct device_node *np,
				struct exynos_ufs *ufs)
{
	struct uic_pwr_mode *pmd = &ufs->req_pmd_parm;

	pmd->mode = FAST_MODE;

	if (of_property_read_u8(np, "ufs,pmd-attr-lane", &pmd->lane))
		pmd->lane = 1;

	if (of_property_read_u8(np, "ufs,pmd-attr-gear", &pmd->gear))
		pmd->gear = 1;

	pmd->hs_series = PA_HS_MODE_B;

	return 0;
}

static int exynos_ufs_populate_dt(struct device *dev, struct exynos_ufs *ufs)
{
	struct device_node *np = dev->of_node;
	int ret;

	/* Get exynos-specific version for featuring */
	if (of_property_read_u32(np, "hw-rev", &ufs->hw_rev))
		ufs->hw_rev = UFS_VER_0004;

	/* Get exynos-evt version for featuring */
	if (of_property_read_u8(np, "evt-ver", &ufs->cal_param->evt_ver))
		ufs->cal_param->evt_ver = 0;

	ret = exynos_ufs_populate_dt_phy(dev, ufs);
	if (ret) {
		dev_err(dev, "failed to populate dt-phy\n");
		goto out;
	}

	ret = exynos_ufs_populate_dt_sys(dev, ufs);
	if (ret)
		dev_err(dev, "failed to populate ufs-sys\n");

	exynos_ufs_get_pwr_mode(np, ufs);

	if (of_property_read_u8(np, "brd-for-cal", &ufs->cal_param->board))
		ufs->cal_param->board = 0;

	if (of_property_read_u32(np, "ufs-pm-qos-int", &ufs->pm_qos_int_value))
		ufs->pm_qos_int_value = 0;

	if (of_property_read_u32(np, "ufs-pm-qos-fsys0", &ufs->pm_qos_fsys0_value))
		ufs->pm_qos_fsys0_value = 0;

	if (of_find_property(np, "enable_tw", NULL)) {
		ufs->enable_tw = true;
		dev_info(dev, "host supports ufs turbo write\n");
	}

out:
	return ret;
}

static u64 exynos_ufs_dma_mask = DMA_BIT_MASK(32);

static int exynos_ufs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct exynos_ufs *ufs;
	struct resource *res;
	int ret;

	ufs = devm_kzalloc(dev, sizeof(*ufs), GFP_KERNEL);
	if (!ufs) {
		dev_err(dev, "cannot allocate mem for exynos-ufs\n");
		return -ENOMEM;
	}

	/* exynos-specific hci */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ufs->reg_hci = devm_ioremap_resource(dev, res);
	if (!ufs->reg_hci) {
		dev_err(dev, "cannot ioremap for hci vendor register\n");
		return -ENOMEM;
	}

	/* unipro */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	ufs->reg_unipro = devm_ioremap_resource(dev, res);
	if (!ufs->reg_unipro) {
		dev_err(dev, "cannot ioremap for unipro register\n");
		return -ENOMEM;
	}

	/* ufs protector */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	ufs->reg_ufsp = devm_ioremap_resource(dev, res);
	if (!ufs->reg_ufsp) {
		dev_err(dev, "cannot ioremap for ufs protector register\n");
		return -ENOMEM;
	}

	/* This must be before calling exynos_ufs_populate_dt */
	ret = ufs_init_cal(ufs, ufs_host_index, pdev);
	if (ret)
		return ret;

	ret = exynos_ufs_populate_dt(dev, ufs);
	if (ret) {
		dev_err(dev, "failed to get dt info.\n");
		return ret;
	}

#ifdef CONFIG_CPU_IDLE
	ufs->idle_ip_index = exynos_get_idle_ip_index(dev_name(&pdev->dev));
	exynos_update_ip_idle_status(ufs->idle_ip_index, 0);
#endif

	ufs->dev = dev;
	dev->platform_data = ufs;
	dev->dma_mask = &exynos_ufs_dma_mask;

	pm_qos_add_request(&ufs->pm_qos_int, PM_QOS_DEVICE_THROUGHPUT, 0);

	ret = ufshcd_pltfrm_init(pdev, &exynos_ufs_ops);

	return ret;
}

static int exynos_ufs_remove(struct platform_device *pdev)
{
	struct exynos_ufs *ufs = dev_get_platdata(&pdev->dev);

	ufshcd_pltfrm_exit(pdev);

	pm_qos_remove_request(&ufs->pm_qos_int);

	ufs->misc_flags = EXYNOS_UFS_MISC_TOGGLE_LOG;

	exynos_ufs_ctrl_phy_pwr(ufs, false);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int exynos_ufs_suspend(struct device *dev)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	return ufshcd_system_suspend(hba);
}

static int exynos_ufs_resume(struct device *dev)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	return ufshcd_system_resume(hba);
}
#else
#define exynos_ufs_suspend	NULL
#define exynos_ufs_resume	NULL
#endif /* CONFIG_PM_SLEEP */

#ifdef CONFIG_PM_RUNTIME
static int exynos_ufs_runtime_suspend(struct device *dev)
{
	return ufshcd_system_suspend(dev_get_drvdata(dev));
}

static int exynos_ufs_runtime_resume(struct device *dev)
{
	return ufshcd_system_resume(dev_get_drvdata(dev));
}

static int exynos_ufs_runtime_idle(struct device *dev)
{
	return ufshcd_runtime_idle(dev_get_drvdata(dev));
}

#else
#define exynos_ufs_runtime_suspend	NULL
#define exynos_ufs_runtime_resume	NULL
#define exynos_ufs_runtime_idle		NULL
#endif /* CONFIG_PM_RUNTIME */

static void exynos_ufs_shutdown(struct platform_device *pdev)
{
	ufshcd_shutdown((struct ufs_hba *)platform_get_drvdata(pdev));
}

static const struct dev_pm_ops exynos_ufs_dev_pm_ops = {
	.suspend		= exynos_ufs_suspend,
	.resume			= exynos_ufs_resume,
	.runtime_suspend	= exynos_ufs_runtime_suspend,
	.runtime_resume		= exynos_ufs_runtime_resume,
	.runtime_idle		= exynos_ufs_runtime_idle,
};

static const struct ufs_hba_variant exynos_ufs_drv_data = {
	.ops		= &exynos_ufs_ops,
};

static const struct of_device_id exynos_ufs_match[] = {
	{ .compatible = "samsung,exynos-ufs", },
	{},
};
MODULE_DEVICE_TABLE(of, exynos_ufs_match);

static struct platform_driver exynos_ufs_driver = {
	.driver = {
		.name = "exynos-ufs",
		.owner = THIS_MODULE,
		.pm = &exynos_ufs_dev_pm_ops,
		.of_match_table = exynos_ufs_match,
		.suppress_bind_attrs = true,
	},
	.probe = exynos_ufs_probe,
	.remove = exynos_ufs_remove,
	.shutdown = exynos_ufs_shutdown,
};

module_platform_driver(exynos_ufs_driver);
MODULE_DESCRIPTION("Exynos Specific UFSHCI driver");
MODULE_AUTHOR("Seungwon Jeon <tgih.jun@samsung.com>");
MODULE_AUTHOR("Kiwoong Kim <kwmad.kim@samsung.com>");
MODULE_LICENSE("GPL");
