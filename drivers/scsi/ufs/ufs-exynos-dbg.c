/*
 * UFS debugging functions for Exynos specific extensions
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * Authors:
 *	Kiwoong <kwmad.kim@samsung.com>
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/io.h>
#include <linux/smc.h>

#include "ufshcd.h"
#include "unipro.h"
#include "mphy.h"
#include "ufs-exynos.h"
#include <soc/samsung/exynos-pmu.h>

/*
 * This is a list for latest SoC.
 */
static struct exynos_ufs_sfr_log ufs_log_sfr[] = {
	{"STD HCI SFR"			,	LOG_STD_HCI_SFR,		0},

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
	{"UTP TRANSF REQ LIST CNR"	,	REG_UTP_TRANSFER_REQ_LIST_CNR,		0},
	{"UTP TASK REQ LIST BASE L"	,	REG_UTP_TASK_REQ_LIST_BASE_L,		0},
	{"UTP TASK REQ LIST BASE H"	,	REG_UTP_TASK_REQ_LIST_BASE_H,		0},
	{"UTP TASK REQ DOOR BELL"	,	REG_UTP_TASK_REQ_DOOR_BELL,		0},
	{"UTP TASK REQ LIST CLEAR"	,	REG_UTP_TASK_REQ_LIST_CLEAR,		0},
	{"UTP TASK REQ LIST RUN STOP"	,	REG_UTP_TASK_REQ_LIST_RUN_STOP,		0},
	{"UIC COMMAND"			,	REG_UIC_COMMAND,		0},
	{"UIC COMMAND ARG1"		,	REG_UIC_COMMAND_ARG_1,		0},
	{"UIC COMMAND ARG2"		,	REG_UIC_COMMAND_ARG_2,		0},
	{"UIC COMMAND ARG3"		,	REG_UIC_COMMAND_ARG_3,		0},
	{"CCAP"				,	REG_CRYPTO_CAPABILITY,		0},


	{"VS HCI SFR"			,	LOG_VS_HCI_SFR,			0},

	{"TXPRDT ENTRY SIZE"		,	HCI_TXPRDT_ENTRY_SIZE,		0},
	{"RXPRDT ENTRY SIZE"		,	HCI_RXPRDT_ENTRY_SIZE,		0},
	{"TO CNT DIV VAL"		,	HCI_TO_CNT_DIV_VAL,		0},
	{"1US TO CNT VAL"		,	HCI_1US_TO_CNT_VAL,		0},
	{"INVALID UPIU CTRL"		,	HCI_INVALID_UPIU_CTRL,		0},
	{"INVALID UPIU BADDR"		,	HCI_INVALID_UPIU_BADDR,		0},
	{"INVALID UPIU UBADDR"		,	HCI_INVALID_UPIU_UBADDR,		0},
	{"INVALID UTMR OFFSET ADDR"	,	HCI_INVALID_UTMR_OFFSET_ADDR,		0},
	{"INVALID UTR OFFSET ADDR"	,	HCI_INVALID_UTR_OFFSET_ADDR,		0},
	{"INVALID DIN OFFSET ADDR"	,	HCI_INVALID_DIN_OFFSET_ADDR,		0},
	{"VENDOR SPECIFIC IS"		,	HCI_VENDOR_SPECIFIC_IS,		0},
	{"VENDOR SPECIFIC IE"		,	HCI_VENDOR_SPECIFIC_IE,		0},
	{"UTRL NEXUS TYPE"		,	HCI_UTRL_NEXUS_TYPE,		0},
	{"UTMRL NEXUS TYPE"		,	HCI_UTMRL_NEXUS_TYPE,		0},
	{"SW RST"			,	HCI_SW_RST,		0},
	{"RX UPIU MATCH ERROR CODE"	,	HCI_RX_UPIU_MATCH_ERROR_CODE,		0},
	{"DATA REORDER"			,	HCI_DATA_REORDER,		0},
	{"AXIDMA RWDATA BURST LEN"	,	HCI_AXIDMA_RWDATA_BURST_LEN,		0},
	{"WRITE DMA CTRL"		,	HCI_WRITE_DMA_CTRL,		0},
	{"V2P1 CTRL"			,	HCI_UFSHCI_V2P1_CTRL,	 	0},
	{"CLKSTOP CTRL"			,	HCI_CLKSTOP_CTRL,		0},
	{"FORCE HCS"			,	HCI_FORCE_HCS,		0},
	{"FSM MONITOR"			,	HCI_FSM_MONITOR,		0},
	{"DMA0 MONITOR STATE"		,	HCI_DMA0_MONITOR_STATE,		0},
	{"DMA0 MONITOR CNT"		,	HCI_DMA0_MONITOR_CNT,		0},
	{"DMA1 MONITOR STATE"		,	HCI_DMA1_MONITOR_STATE,		0},
	{"DMA1 MONITOR CNT"		,	HCI_DMA1_MONITOR_CNT,		0},
	{"DMA0 DOORBELL DEBUG"		,	HCI_DMA0_DOORBELL_DEBUG,		0},
	{"DMA1 DOORBELL DEBUG"		,	HCI_DMA1_DOORBELL_DEBUG,		0},

	{"AXI DMA IF CTRL"		,	HCI_UFS_AXI_DMA_IF_CTRL,	0},
	{"UFS ACG DISABLE"	 	,	HCI_UFS_ACG_DISABLE,		0},
	{"MPHY REFCLK SEL"		,	HCI_MPHY_REFCLK_SEL,		0},

	{"SMU RD ABORT MATCH INFO"		,	HCI_SMU_RD_ABORT_MATCH_INFO,	0},
	{"SMU WR ABORT MATCH INFO"		,	HCI_SMU_WR_ABORT_MATCH_INFO,	0},

	{"DBR DUPLICATION INFO"		,	HCI_DBR_DUPLICATION_INFO,	0},
	{"INVALID PRDT CTRL"		,	HCI_INVALID_PRDT_CTRL,		0},
	{"DBR TIMER CONFIG"		,	HCI_DBR_TIMER_CONFIG,		0},
	{"UTRL DBR TIMER ENABLE"		,	HCI_UTRL_DBR_TIMER_ENABLE,		0},
	{"UTRL DBR TIMER STATUS"		,	HCI_UTRL_DBR_TIMER_STATUS,		0},

	{"UTMRL DBR TIMER ENABLE"		,	HCI_UTMRL_DBR_TIMER_ENABLE,		0},
	{"UTMRL DBR TIMER STATUS"		,	HCI_UTMRL_DBR_TIMER_STATUS,		0},

	{"UTRL DBR 3 0 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_3_0_TIMER_EXPIRED_VALUE,		0},
	{"UTRL DBR 7 4 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_7_4_TIMER_EXPIRED_VALUE,	0},
	{"UTRL DBR 11 8 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_11_8_TIMER_EXPIRED_VALUE,	0},
	{"UTRL DBR 15 12 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_15_12_TIMER_EXPIRED_VALUE,		0},
	{"UTRL DBR 19 16 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_19_16_TIMER_EXPIRED_VALUE,		0},
	{"UTRL DBR 23 20 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_23_20_TIMER_EXPIRED_VALUE,	0},
	{"UTRL DBR 27 24 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_27_24_TIMER_EXPIRED_VALUE,	0},
	{"UTRL DBR 31 28 TIMER EXPIRED VALUE"		,	HCI_UTRL_DBR_31_28_TIMER_EXPIRED_VALUE,		0},
	{"UTMRL DBR 3 0 TIMER EXPIRED VALUE"		,	HCI_UTMRL_DBR_3_0_TIMER_EXPIRED_VALUE,		0},

	{"FMP SFR"			,	LOG_FMP_SFR,			0},

	{"UFSPRCTRL"			,	UFSPRCTRL,			0},
	{"UFSPRSTAT"			,	UFSPRSTAT,			0},
	{"UFSPRSECURITY"		,	UFSPRSECURITY,			0},
	{"UFSPWCTRL"			,	UFSPWCTRL,			0},
	{"UFSPWSTAT"			,	UFSPWSTAT,			0},
	{"UFSPSBEGIN0"			,	UFSPSBEGIN0,			0},
	{"UFSPSEND0"			,	UFSPSEND0,			0},
	{"UFSPSLUN0"			,	UFSPSLUN0,			0},
	{"UFSPSCTRL0"			,	UFSPSCTRL0,			0},
	{"UFSPSBEGIN1"			,	UFSPSBEGIN1,			0},
	{"UFSPSEND1"			,	UFSPSEND1,			0},
	{"UFSPSLUN1"			,	UFSPSLUN1,			0},
	{"UFSPSCTRL1"			,	UFSPSCTRL1,			0},
	{"UFSPSBEGIN2"			,	UFSPSBEGIN2,			0},
	{"UFSPSEND2"			,	UFSPSEND2,			0},
	{"UFSPSLUN2"			,	UFSPSLUN2,			0},
	{"UFSPSCTRL2"			,	UFSPSCTRL2,			0},
	{"UFSPSBEGIN3"			,	UFSPSBEGIN3,			0},
	{"UFSPSEND3"			,	UFSPSEND3,			0},
	{"UFSPSLUN3"			,	UFSPSLUN3,			0},
	{"UFSPSCTRL3"			,	UFSPSCTRL3,			0},
	{"UFSPSBEGIN4"			,	UFSPSBEGIN4,			0},
	{"UFSPSLUN4"			,	UFSPSLUN4,			0},
	{"UFSPSCTRL4"			,	UFSPSCTRL4,			0},
	{"UFSPSBEGIN5"			,	UFSPSBEGIN5,			0},
	{"UFSPSEND5"			,	UFSPSEND5,			0},
	{"UFSPSLUN5"			,	UFSPSLUN5,			0},
	{"UFSPSCTRL5"			,	UFSPSCTRL5,			0},
	{"UFSPSBEGIN6"			,	UFSPSBEGIN6,			0},
	{"UFSPSEND6"			,	UFSPSEND6,			0},
	{"UFSPSLUN6"			,	UFSPSLUN6,			0},
	{"UFSPSCTRL6"			,	UFSPSCTRL6,			0},
	{"UFSPSBEGIN7"			,	UFSPSBEGIN7,			0},
	{"UFSPSEND7"			,	UFSPSEND7,			0},
	{"UFSPSLUN7"			,	UFSPSLUN7,			0},
	{"UFSPSCTRL7"			,	UFSPSCTRL7,			0},

	{"UNIPRO SFR"			,	LOG_UNIPRO_SFR,			0},

	{"DME_LINKSTARTUP_CNF_RESULT"	,	UNIP_DME_LINKSTARTUP_CNF_RESULT	,	0},
	{"DME_HIBERN8_ENTER_CNF_RESULT"	,	UNIP_DME_HIBERN8_ENTER_CNF_RESULT,	0},
	{"DME_HIBERN8_ENTER_IND_RESULT"	,	UNIP_DME_HIBERN8_ENTER_IND_RESULT,	0},
	{"DME_HIBERN8_EXIT_CNF_RESULT"	,	UNIP_DME_HIBERN8_EXIT_CNF_RESULT,	0},
	{"DME_HIBERN8_EXIT_IND_RESULT"	,	UNIP_DME_HIBERN8_EXIT_IND_RESULT,	0},
	{"DME_PWR_IND_RESULT"		,	UNIP_DME_PWR_IND_RESULT	,	0},
	{"DME_INTR_STATUS_LSB"		,	UNIP_DME_INTR_STATUS_LSB,	0},
	{"DME_INTR_STATUS_MSB"		,	UNIP_DME_INTR_STATUS_MSB,	0},
	{"DME_INTR_ERROR_CODE"		,	UNIP_DME_INTR_ERROR_CODE,	0},
	{"DME_DISCARD_PORT_ID"		,	UNIP_DME_DISCARD_PORT_ID,	0},
	{"DME_DBG_OPTION_SUITE"		,	UNIP_DME_DBG_OPTION_SUITE,	0},
	{"DME_DBG_CTRL_FSM"		,	UNIP_DME_DBG_CTRL_FSM,	0},
	{"DME_DBG_FLAG_STATUS"		,	UNIP_DME_DBG_FLAG_STATUS,	0},
	{"DME_DBG_LINKCFG_FSM"		,	UNIP_DME_DBG_LINKCFG_FSM,	0},

	{"PMA SFR"			,	LOG_PMA_SFR,			0},

	{"COMN 0x45"			,	(0x0114),			0},
	{"COMN 0x46"			,	(0x0118),			0},
	{"COMN 0x47"			,	(0x011C),			0},
	{"TRSV_L0 0x1C5"		,	(0x0714),			0},
	{"TRSV_L0 0x1EC"		,	(0x07B0),			0},
	{"TRSV_L0 0x1ED"		,	(0x07B4),			0},
	{"TRSV_L0 0x1EE"		,	(0x07B8),			0},
	{"TRSV_L0 0x1EF"		,	(0x07BC),			0},

	{"TRSV_L1 0x1C5"		,	(0x0B14),			0},
	{"TRSV_L1 0x1EC"		,	(0x0BB0),			0},
	{"TRSV_L1 0x1ED"		,	(0x0BB4),			0},
	{"TRSV_L1 0x1EE"		,	(0x0BB8),			0},
	{"TRSV_L1 0x1EF"		,	(0x0BBC),			0},

	{"TRSV_L0 0x152"		,	(0x0548),			0},
	{"TRSV_L0 0x153"		,	(0x054C),			0},
	{"TRSV_L0 0x154"		,	(0x0550),			0},
	{"TRSV_L0 0x159"		,	(0x0564),			0},
	{"TRSV_L0 0x1F2"		,	(0x07C8),			0},

	{"TRSV_L1 0x252"		,	(0x0948),			0},
	{"TRSV_L1 0x253"		,	(0x094C),			0},
	{"TRSV_L1 0x254"		,	(0x0950),			0},
	{"TRSV_L1 0x259"		,	(0x0964),			0},
	{"TRSV_L1 0x2F2"		,	(0x0BC8),			0},
	{},
};

static struct exynos_ufs_attr_log ufs_log_attr[] = {
	/* PA Standard */
	{UIC_ARG_MIB(0x1520),	0, 0},
	{UIC_ARG_MIB(0x1540),	0, 0},
	{UIC_ARG_MIB(0x1543),	0, 0},
	{UIC_ARG_MIB(0x155C),	0, 0},
	{UIC_ARG_MIB(0x155D),	0, 0},
	{UIC_ARG_MIB(0x155F),	0, 0},
	{UIC_ARG_MIB(0x1560),	0, 0},
	{UIC_ARG_MIB(0x1561),	0, 0},
	{UIC_ARG_MIB(0x1564),	0, 0},
	{UIC_ARG_MIB(0x1567),	0, 0},
	{UIC_ARG_MIB(0x1568),	0, 0},
	{UIC_ARG_MIB(0x1569),	0, 0},
	{UIC_ARG_MIB(0x156A),	0, 0},
	{UIC_ARG_MIB(0x1571),	0, 0},
	{UIC_ARG_MIB(0x1580),	0, 0},
	{UIC_ARG_MIB(0x1581),	0, 0},
	{UIC_ARG_MIB(0x1582),	0, 0},
	{UIC_ARG_MIB(0x1583),	0, 0},
	{UIC_ARG_MIB(0x1584),	0, 0},
	{UIC_ARG_MIB(0x1585),	0, 0},
	{UIC_ARG_MIB(0x1590),	0, 0},
	{UIC_ARG_MIB(0x1591),	0, 0},
	{UIC_ARG_MIB(0x15A1),	0, 0},
	{UIC_ARG_MIB(0x15A2),	0, 0},
	{UIC_ARG_MIB(0x15A3),	0, 0},
	{UIC_ARG_MIB(0x15A4),	0, 0},
	{UIC_ARG_MIB(0x15A7),	0, 0},
	{UIC_ARG_MIB(0x15A8),	0, 0},
	{UIC_ARG_MIB(0x15A9),	0, 0},
	{UIC_ARG_MIB(0x15C0),	0, 0},
	{UIC_ARG_MIB(0x15C1),	0, 0},
	{UIC_ARG_MIB(0x15D2),	0, 0},
	{UIC_ARG_MIB(0x15D3),	0, 0},
	{UIC_ARG_MIB(0x15D4),	0, 0},
	{UIC_ARG_MIB(0x15D5),	0, 0},
	/* PA Debug */
	{UIC_ARG_MIB(0x9500),	0, 0},
	{UIC_ARG_MIB(0x9501),	0, 0},
	{UIC_ARG_MIB(0x9502),	0, 0},
	{UIC_ARG_MIB(0x9504),	0, 0},
	{UIC_ARG_MIB(0x9564),	0, 0},
	{UIC_ARG_MIB(0x956A),	0, 0},
	{UIC_ARG_MIB(0x956D),	0, 0},
	{UIC_ARG_MIB(0x9570),	0, 0},
	{UIC_ARG_MIB(0x9595),	0, 0},
	{UIC_ARG_MIB(0x9596),	0, 0},
	{UIC_ARG_MIB(0x9597),	0, 0},
	/* DL Standard */
	{UIC_ARG_MIB(0x2047),	0, 0},
	{UIC_ARG_MIB(0x2067),	0, 0},
	/* DL Debug */
	{UIC_ARG_MIB(0xA000),	0, 0},
	{UIC_ARG_MIB(0xA005),	0, 0},
	{UIC_ARG_MIB(0xA007),	0, 0},
	{UIC_ARG_MIB(0xA010),	0, 0},
	{UIC_ARG_MIB(0xA011),	0, 0},
	{UIC_ARG_MIB(0xA020),	0, 0},
	{UIC_ARG_MIB(0xA021),	0, 0},
	{UIC_ARG_MIB(0xA022),	0, 0},
	{UIC_ARG_MIB(0xA023),	0, 0},
	{UIC_ARG_MIB(0xA024),	0, 0},
	{UIC_ARG_MIB(0xA025),	0, 0},
	{UIC_ARG_MIB(0xA026),	0, 0},
	{UIC_ARG_MIB(0xA027),	0, 0},
	{UIC_ARG_MIB(0xA028),	0, 0},
	{UIC_ARG_MIB(0xA029),	0, 0},
	{UIC_ARG_MIB(0xA02A),	0, 0},
	{UIC_ARG_MIB(0xA02B),	0, 0},
	{UIC_ARG_MIB(0xA100),	0, 0},
	{UIC_ARG_MIB(0xA101),	0, 0},
	{UIC_ARG_MIB(0xA102),	0, 0},
	{UIC_ARG_MIB(0xA103),	0, 0},
	{UIC_ARG_MIB(0xA114),	0, 0},
	{UIC_ARG_MIB(0xA115),	0, 0},
	{UIC_ARG_MIB(0xA116),	0, 0},
	{UIC_ARG_MIB(0xA120),	0, 0},
	{UIC_ARG_MIB(0xA121),	0, 0},
	{UIC_ARG_MIB(0xA122),	0, 0},
	/* NL Standard */
	/* NL Debug */
	{UIC_ARG_MIB(0xB011),	0, 0},
	/* TL Standard */
	{UIC_ARG_MIB(0x4020),	0, 0},
	/* TL Debug */
	{UIC_ARG_MIB(0xC001),	0, 0},
	{UIC_ARG_MIB(0xC024),	0, 0},
	{UIC_ARG_MIB(0xC026),	0, 0},
	/* MPHY PCS Lane 0*/
	{UIC_ARG_MIB_SEL(0x0021, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0022, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0023, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0024, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0028, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0029, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x002A, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x002B, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x002C, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x002D, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0033, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0035, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0036, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0041, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A1, RX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A2, RX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A3, RX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A4, RX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A7, RX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00C1, RX_LANE_0+0),	0, 0},
	/* MPHY PCS Lane 1*/
	{UIC_ARG_MIB_SEL(0x0021, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0022, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0023, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0024, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0028, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0029, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x002A, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x002B, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x002C, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x002D, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0033, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0035, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0036, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0041, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A1, RX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A2, RX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A3, RX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A4, RX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A7, RX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00C1, RX_LANE_0+1),	0, 0},
	{},
};

static struct exynos_ufs_sfr_log ufs_show_sfr[] = {
	{"STD HCI SFR"			,	LOG_STD_HCI_SFR,		0},

	{"INTERRUPT STATUS"		,	REG_INTERRUPT_STATUS,		0},
	{"CONTROLLER STATUS"		,	REG_CONTROLLER_STATUS,		0},
	{"UTP TRANSF REQ DOOR BELL"	,	REG_UTP_TRANSFER_REQ_DOOR_BELL,		0},
	{"UTP TASK REQ DOOR BELL"	,	REG_UTP_TASK_REQ_DOOR_BELL,		0},

	{"VS HCI SFR"			,	LOG_VS_HCI_SFR,			0},

	{"VENDOR SPECIFIC IS"		,	HCI_VENDOR_SPECIFIC_IS,		0},
	{"RX UPIU MATCH ERROR CODE"	,	HCI_RX_UPIU_MATCH_ERROR_CODE,		0},
	{"CLKSTOP CTRL",			HCI_CLKSTOP_CTRL,		0},
	{"FORCE HCS",				HCI_FORCE_HCS,		0},
	{"DMA0 MONITOR STATE"		,	HCI_DMA0_MONITOR_STATE,		0},
	{"DMA1 MONITOR STATE"		,	HCI_DMA1_MONITOR_STATE,		0},
	{"DMA0 DOORBELL DEBUG"		,	HCI_DMA0_DOORBELL_DEBUG,		0},
	{"DMA1 DOORBELL DEBUG"		,	HCI_DMA1_DOORBELL_DEBUG,		0},
	{"SMU RD ABORT MATCH INFO"		,	HCI_SMU_RD_ABORT_MATCH_INFO,	0},
	{"SMU WR ABORT MATCH INFO"		,	HCI_SMU_WR_ABORT_MATCH_INFO,	0},

	{"FMP SFR"			,	LOG_FMP_SFR,			0},

	{"UFSPRSECURITY"		,	UFSPRSECURITY,			0},


	{"UNIPRO SFR"			,	LOG_UNIPRO_SFR,			0},

	{"DME_HIBERN8_ENTER_IND_RESULT"	,	UNIP_DME_HIBERN8_ENTER_IND_RESULT	,	0},
	{"DME_HIBERN8_EXIT_IND_RESULT"	,	UNIP_DME_HIBERN8_EXIT_IND_RESULT	,	0},
	{"DME_PWR_IND_RESULT"		,	UNIP_DME_PWR_IND_RESULT			,	0},
	{"DME_DBG_CTRL_FSM"		,	UNIP_DME_DBG_CTRL_FSM			,	0},

	{"PMA SFR"			,	LOG_PMA_SFR,			0},

	{"COMN 0x45"			,	(0x0114),			0},
	{"COMN 0x46"			,	(0x0118),			0},
	{"COMN 0x47"			,	(0x011C),			0},
	{"TRSV_L0 0x1C5"		,	(0x0714),			0},
	{"TRSV_L0 0x1EC"		,	(0x07B0),			0},
	{"TRSV_L0 0x1ED"		,	(0x07B4),			0},
	{"TRSV_L0 0x1EE"		,	(0x07B8),			0},
	{"TRSV_L0 0x1EF"		,	(0x07BC),			0},

	{"TRSV_L1 0x1C5"		,	(0x0B14),			0},
	{"TRSV_L1 0x1EC"		,	(0x0BB0),			0},
	{"TRSV_L1 0x1ED"		,	(0x0BB4),			0},
	{"TRSV_L1 0x1EE"		,	(0x0BB8),			0},
	{"TRSV_L1 0x1EF"		,	(0x0BBC),			0},

	{"TRSV_L0 0x152"		,	(0x0548),			0},
	{"TRSV_L0 0x153"		,	(0x054C),			0},
	{"TRSV_L0 0x154"		,	(0x0550),			0},
	{"TRSV_L0 0x159"		,	(0x0564),			0},
	{"TRSV_L0 0x1F2"		,	(0x07C8),			0},

	{"TRSV_L1 0x252"		,	(0x0948),			0},
	{"TRSV_L1 0x253"		,	(0x094C),			0},
	{"TRSV_L1 0x254"		,	(0x0950),			0},
	{"TRSV_L1 0x259"		,	(0x0964),			0},
	{"TRSV_L1 0x2F2"		,	(0x0BC8),			0},
	{},
};

static struct exynos_ufs_attr_log ufs_show_attr[] = {
	/* PA Standard */
	{UIC_ARG_MIB(0x1560),	0, 0},
	{UIC_ARG_MIB(0x1571),	0, 0},
	{UIC_ARG_MIB(0x1580),	0, 0},
	/* PA Debug */
	{UIC_ARG_MIB(0x9595),	0, 0},
	{UIC_ARG_MIB(0x9596),	0, 0},
	{UIC_ARG_MIB(0x9597),	0, 0},
	/* DL Debug */
	{UIC_ARG_MIB(0xA000),	0, 0},
	{UIC_ARG_MIB(0xA005),	0, 0},
	{UIC_ARG_MIB(0xA010),	0, 0},
	{UIC_ARG_MIB(0xA114),	0, 0},
	{UIC_ARG_MIB(0xA116),	0, 0},
	/* MPHY PCS Lane 0*/
	{UIC_ARG_MIB_SEL(0x002B, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x0041, TX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A7, RX_LANE_0+0),	0, 0},
	{UIC_ARG_MIB_SEL(0x00C1, RX_LANE_0+0),	0, 0},
	/* MPHY PCS Lane 1*/
	{UIC_ARG_MIB_SEL(0x002B, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x0041, TX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00A7, RX_LANE_0+1),	0, 0},
	{UIC_ARG_MIB_SEL(0x00C1, RX_LANE_0+1),	0, 0},
	{},
};
static void exynos_ufs_get_misc(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct exynos_ufs_clk_info *clki;
	struct list_head *head = &ufs->debug.misc.clk_list_head;
	u32 reg = 0;

	list_for_each_entry(clki, head, list) {
		if (!IS_ERR_OR_NULL(clki->clk))
			clki->freq = clk_get_rate(clki->clk);
	}
	exynos_pmu_read(EXYNOS_PMU_UFS_PHY_OFFSET, &reg);
	ufs->debug.misc.isolation = (bool)(reg & BIT(0));
}

static void exynos_ufs_get_sfr(struct ufs_hba *hba,
					struct exynos_ufs_sfr_log* cfg)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	int sel_api = 0;

	while(cfg) {
		if (!cfg->name)
			break;

		if (cfg->offset >= LOG_STD_HCI_SFR) {
			/* Select an API to get SFRs */
			sel_api = cfg->offset;
		} else {
			/* Fetch value */
			if (sel_api == LOG_STD_HCI_SFR)
				cfg->val = ufshcd_readl(hba, cfg->offset);
			else if (sel_api == LOG_VS_HCI_SFR)
				cfg->val = hci_readl(ufs, cfg->offset);
#ifdef CONFIG_SCSI_UFS_FMP_DUMP
			else if (sel_api == LOG_FMP_SFR)
				cfg->val = exynos_smc(SMC_CMD_FMP_SMU_DUMP, 0, 0, cfg->offset);
#endif
			else if (sel_api == LOG_UNIPRO_SFR)
				cfg->val = unipro_readl(ufs, cfg->offset);
			else if (sel_api == LOG_PMA_SFR)
				cfg->val = phy_pma_readl(ufs, cfg->offset);
			else
				cfg->val = 0xFFFFFFFF;
		}

		/* Next SFR */
		cfg++;
	}
}

static void exynos_ufs_get_attr(struct ufs_hba *hba,
					struct exynos_ufs_attr_log* cfg)
{
	u32 i;
	u32 intr_enable;

	/* Disable and backup interrupts */
	intr_enable = ufshcd_readl(hba, REG_INTERRUPT_ENABLE);
	ufshcd_writel(hba, 0, REG_INTERRUPT_ENABLE);

	while(cfg) {
		if (cfg->offset == 0)
			break;

		/* Send DME_GET */
		ufshcd_writel(hba, cfg->offset, REG_UIC_COMMAND_ARG_1);
		ufshcd_writel(hba, UIC_CMD_DME_GET, REG_UIC_COMMAND);

		i = 0;
		while(!(ufshcd_readl(hba, REG_INTERRUPT_STATUS) &
					UIC_COMMAND_COMPL)) {
			if (i++ > 20000) {
				dev_err(hba->dev,
					"Failed to fetch a value of %x",
					cfg->offset);
				goto out;
			}
		}

		/* Clear UIC command completion */
		ufshcd_writel(hba, UIC_COMMAND_COMPL, REG_INTERRUPT_STATUS);

		/* Fetch result and value */
		cfg->res = ufshcd_readl(hba, REG_UIC_COMMAND_ARG_2 &
				MASK_UIC_COMMAND_RESULT);
		cfg->val = ufshcd_readl(hba, REG_UIC_COMMAND_ARG_3);

		/* Next attribute */
		cfg++;
	}

out:
	/* Restore and enable interrupts */
	ufshcd_writel(hba, intr_enable, REG_INTERRUPT_ENABLE);
}

static void exynos_ufs_dump_misc(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct exynos_ufs_misc_log* cfg = &ufs->debug.misc;
	struct exynos_ufs_clk_info *clki;
	struct list_head *head = &cfg->clk_list_head;

	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": \t\tMISC DUMP\n");
	dev_err(hba->dev, ": --------------------------------------------------- \n");

	list_for_each_entry(clki, head, list) {
		if (!IS_ERR_OR_NULL(clki->clk)) {
			dev_err(hba->dev, "%s: %lu\n",
					clki->name, clki->freq);
		}
	}
	dev_err(hba->dev, "iso: %d\n", ufs->debug.misc.isolation);
}

static void exynos_ufs_dump_sfr(struct ufs_hba *hba,
					struct exynos_ufs_sfr_log* cfg)
{
	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": \t\tREGISTER DUMP\n");
	dev_err(hba->dev, ": --------------------------------------------------- \n");

	while(cfg) {
		if (!cfg->name)
			break;

		/* Dump */
		dev_err(hba->dev, ": %s(0x%04x):\t\t\t\t0x%08x\n",
				cfg->name, cfg->offset, cfg->val);

		/* Next SFR */
		cfg++;
	}
}

static void exynos_ufs_dump_attr(struct ufs_hba *hba,
					struct exynos_ufs_attr_log* cfg)
{
	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": \t\tATTRIBUTE DUMP\n");
	dev_err(hba->dev, ": --------------------------------------------------- \n");

	while(cfg) {
		if (!cfg->offset)
			break;

		/* Dump */
		dev_err(hba->dev, ": 0x%04x:\t\t0x%08x\t\t0x%08x\n",
				cfg->offset, cfg->val, cfg->res);

		/* Next SFR */
		cfg++;
	}
}

#define cport_writel_raw(ufs, val, reg)	\
		writel_relaxed((val), (ufs)->debug.reg_cport + (reg))
#define cport_readl_raw(hba, reg)	\
		readl_relaxed((ufs)->debug.reg_cport + (reg))

/*
 * Functions to be provied externally
 *
 * There are two classes that are to initialize data structures for debug
 * and to define actual behavior.
 */
void exynos_ufs_get_uic_info(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	if (!(ufs->misc_flags & EXYNOS_UFS_MISC_TOGGLE_LOG))
		return;

	exynos_ufs_get_sfr(hba, ufs->debug.sfr);
	exynos_ufs_get_attr(hba, ufs->debug.attr);
	exynos_ufs_get_misc(hba);

	ufs->misc_flags &= ~(EXYNOS_UFS_MISC_TOGGLE_LOG);
}

void exynos_ufs_dump_uic_info(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);

	exynos_ufs_dump_cport_log(hba);

	exynos_ufs_get_sfr(hba, ufs->debug.sfr);
	exynos_ufs_get_attr(hba, ufs->debug.attr);
	exynos_ufs_get_misc(hba);

	exynos_ufs_dump_sfr(hba, ufs->debug.sfr);
	exynos_ufs_dump_attr(hba, ufs->debug.attr);
	exynos_ufs_dump_misc(hba);
}

void exynos_ufs_show_uic_info(struct ufs_hba *hba)
{
	exynos_ufs_get_sfr(hba, ufs_show_sfr);
	exynos_ufs_get_attr(hba, ufs_show_attr);

	exynos_ufs_dump_sfr(hba, ufs_show_sfr);
	exynos_ufs_dump_attr(hba, ufs_show_attr);
}

#define UFS_CPORT_PRINT		0	/*CPORT printing enable:1 , disable:0*/

#define CPORT_TX_BUF_SIZE	0x100
#define CPORT_RX_BUF_SIZE	0x100
#define CPORT_LOG_PTR_SIZE	0x100
#define CPORT_UTRL_SIZE		0x400
#define CPORT_UCD_SIZE		0x400
#define CPORT_UTMRL_SIZE	0xc0
#define CPORT_BUF_SIZE		(CPORT_TX_BUF_SIZE + CPORT_RX_BUF_SIZE + \
				CPORT_LOG_PTR_SIZE +  CPORT_UTRL_SIZE + \
				CPORT_UCD_SIZE +  CPORT_UTMRL_SIZE)

#define CPORT_TX_BUF_PTR	0x0
#define CPORT_RX_BUF_PTR	(CPORT_TX_BUF_PTR + CPORT_TX_BUF_SIZE)
#define CPORT_LOG_PTR_OFFSET	(CPORT_RX_BUF_PTR + CPORT_RX_BUF_SIZE)
#define CPORT_UTRL_PTR		(CPORT_LOG_PTR_OFFSET + CPORT_LOG_PTR_SIZE)
#define CPORT_UCD_PTR		(CPORT_UTRL_PTR + CPORT_UTRL_SIZE)
#define CPORT_UTMRL_PTR		(CPORT_UCD_PTR + CPORT_UCD_SIZE)

#define UFS_CPORT_PRINT	0

struct exynos_ufs_log_cport {
	int poison_for_once;
	u32 ptr;
	u8 buf[CPORT_BUF_SIZE];
} ufs_log_cport;

static int exynos_ufs_populate_dt_cport(struct exynos_ufs *ufs)
{
	struct device *dev = ufs->dev;
	struct device_node *np;
	struct resource io_res;
	int ret = 0;

	np = of_get_child_by_name(dev->of_node, "ufs-cport");
	if (!np) {
		dev_err(dev, "failed to get ufs-cport node\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(np, 0, &io_res);
	if (ret) {
		dev_err(dev, "failed to get i/o address cport\n");
		goto err_0;
	}

	ufs->debug.reg_cport = devm_ioremap_resource(dev, &io_res);
	if (!ufs->debug.reg_cport) {
		dev_err(dev, "failed to ioremap for cport\n");
		ret = -ENOMEM;
		goto err_0;
	}

err_0:
	return ret;
}

int exynos_ufs_init_dbg(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	struct list_head *head = &hba->clk_list_head;
	struct ufs_clk_info *clki;
	struct exynos_ufs_clk_info *exynos_clki;
	int ret = 0;

	ufs->debug.sfr = ufs_log_sfr;
	ufs->debug.attr = ufs_log_attr;
	INIT_LIST_HEAD(&ufs->debug.misc.clk_list_head);

	if (!head || list_empty(head))
		return 0;

	list_for_each_entry(clki, head, list) {
		exynos_clki = devm_kzalloc(hba->dev, sizeof(*exynos_clki), GFP_KERNEL);
		if (!exynos_clki) {
			return -ENOMEM;
		}
		exynos_clki->clk = clki->clk;
		exynos_clki->name = clki->name;
		exynos_clki->freq = 0;
		list_add_tail(&exynos_clki->list, &ufs->debug.misc.clk_list_head);
	}

	/* CPort log map and enable */
	ret = exynos_ufs_populate_dt_cport(ufs);
	if (ret && ret != -ENODEV) {
		ufs_log_cport.poison_for_once = 0xDEADBEAF;
		return ret;
	}

	return 0;
}

void exynos_ufs_ctrl_cport_log(struct exynos_ufs *ufs,
						bool en, int log_type)
{
	int type;

	if (ufs_log_cport.poison_for_once == 0xDEADBEAF)
		return;

	type = (log_type << 4) | log_type;

	if (en) {
		hci_writel(ufs, type, 0x114);
		hci_writel(ufs, 1, 0x110);
	} else {
		hci_writel(ufs, 0, 0x110);

		ufs_log_cport.poison_for_once = 0xDEADBEAF;
	}
}

void exynos_ufs_dump_cport_log(struct ufs_hba *hba)
{
	struct exynos_ufs *ufs = to_exynos_ufs(hba);
	u32 *buf_ptr;
	u8 *buf_ptr_out;
	u32 offset;
	u32 size = 0;
	u32 cur_ptr = 0;

	if (ufs_log_cport.poison_for_once == 0xDEADBEAF)
		return;

	/* CPort log disable */
	exynos_ufs_ctrl_cport_log(ufs, false, 0);

	/*
	 * Dump data
	 *
	 * [ log type 0 ]
	 * First 4 double words
	 *
	 * [ log type 1 ]
	 * 6 double words, for Rx
	 * 8 double words, for Tx
	 *
	 * [ log type 2 ]
	 * 4 double words, for Command UPIU, DATA OUT/IN UPIU and RTT.
	 * 2 double words, otherwise.
	 *
	 */
	ufs_log_cport.ptr = cport_readl_raw(ufs, CPORT_LOG_PTR_OFFSET);
	hba->cport_addr = buf_ptr = (u32 *)&ufs_log_cport.buf[0];
	size = 0;
	offset = 0;

	while (size < CPORT_BUF_SIZE) {
		*buf_ptr = cport_readl_raw(ufs, offset);
		size += 4;
		buf_ptr += 1;
		offset += 4;
	}
	mb(); /* memory barrier for ufs cport dump */

	dev_err(hba->dev, "cport logging finished\n");

	/* Print data */
	buf_ptr_out = &ufs_log_cport.buf[0];
	cur_ptr = 0;

#ifdef UFS_CPORT_PRINT
	while (cur_ptr < CPORT_BUF_SIZE) {
		switch (cur_ptr) {
		case CPORT_TX_BUF_PTR:
			dev_err(hba->dev, ":---------------------------------------------------\n");
			dev_err(hba->dev, ": \t\tTX BUF (%d)\n", ((ufs_log_cport.ptr >> 0) & 0x3F)/2);
			dev_err(hba->dev, ":---------------------------------------------------\n");
			break;
		case  CPORT_RX_BUF_PTR:
			dev_err(hba->dev, ":---------------------------------------------------\n");
			dev_err(hba->dev, ": \t\tRX BUF (%d)\n", ((ufs_log_cport.ptr >> 8) & 0x3F)/2);
			dev_err(hba->dev, ":---------------------------------------------------\n");
			break;
		case CPORT_LOG_PTR_OFFSET:
			dev_err(hba->dev, ":---------------------------------------------------\n");
			dev_err(hba->dev, ": \t\tCPORT LOG PTR\n");
			dev_err(hba->dev, ":---------------------------------------------------\n");
			break;
		case CPORT_UTRL_PTR:
			dev_err(hba->dev, ":---------------------------------------------------\n");
			dev_err(hba->dev, ": \t\tUTRL\n");
			dev_err(hba->dev, ":---------------------------------------------------\n");
			break;
		case CPORT_UCD_PTR:
			dev_err(hba->dev, ":---------------------------------------------------\n");
			dev_err(hba->dev, ": \t\tUCD\n");
			dev_err(hba->dev, ":---------------------------------------------------\n");
			break;
		case CPORT_UTMRL_PTR:
			dev_err(hba->dev, ":---------------------------------------------------\n");
			dev_err(hba->dev, ": \t\tUTMRL\n");
			dev_err(hba->dev, ":---------------------------------------------------\n");
			break;
		default:
			break;
		}

		if (cur_ptr == CPORT_LOG_PTR_OFFSET) {
			dev_err(hba->dev, "%02x%02x%02x%02x ",
				*(buf_ptr_out+0x3), *(buf_ptr_out+0x2), *(buf_ptr_out+0x1), *(buf_ptr_out+0x0));
			buf_ptr_out += 0x100;
			cur_ptr += 0x100;
		} else {
			dev_err(hba->dev, "%02x%02x%02x%02x %02x%02x%02x%02x "
				"%02x%02x%02x%02x %02x%02x%02x%02x\n",
				*(buf_ptr_out+0x3), *(buf_ptr_out+0x2), *(buf_ptr_out+0x1), *(buf_ptr_out+0x0),
				*(buf_ptr_out+0x7), *(buf_ptr_out+0x6), *(buf_ptr_out+0x5), *(buf_ptr_out+0x4),
				*(buf_ptr_out+0xb), *(buf_ptr_out+0xa), *(buf_ptr_out+0x9), *(buf_ptr_out+0x8),
				*(buf_ptr_out+0xf), *(buf_ptr_out+0xe), *(buf_ptr_out+0xd), *(buf_ptr_out+0xc));
			buf_ptr_out += 0x10;
			cur_ptr += 0x10;
		}
	}
#endif
}

static	struct ufs_cmd_info   ufs_cmd_queue;
static	struct ufs_cmd_logging_category ufs_cmd_log;

static void exynos_ufs_putItem_start(struct ufs_cmd_info *cmdQueue, struct ufs_cmd_logging_category *cmdData)
{
	cmdQueue->addr_per_tag[cmdData->tag] = &cmdQueue->data[cmdQueue->last];
	cmdQueue->data[cmdQueue->last].cmd_opcode = cmdData->cmd_opcode;
	cmdQueue->data[cmdQueue->last].tag = cmdData->tag;
	cmdQueue->data[cmdQueue->last].lba = cmdData->lba;
	cmdQueue->data[cmdQueue->last].sct = cmdData->sct;
	cmdQueue->data[cmdQueue->last].retries = cmdData->retries;
	cmdQueue->data[cmdQueue->last].start_time = cmdData->start_time;
	cmdQueue->data[cmdQueue->last].outstanding_reqs = cmdData->outstanding_reqs;
	cmdQueue->last = (cmdQueue->last + 1) % MAX_CMD_LOGS;
}



void exynos_ufs_cmd_log_start(struct ufs_hba *hba, struct scsi_cmnd *cmd)
{
	int cpu = raw_smp_processor_id();

	unsigned long lba = (cmd->cmnd[2] << 24) |
					(cmd->cmnd[3] << 16) |
					(cmd->cmnd[4] << 8) |
					(cmd->cmnd[5] << 0);
	unsigned int sct = (cmd->cmnd[7] << 8) |
					(cmd->cmnd[8] << 0);

	ufs_cmd_log.start_time = cpu_clock(cpu);
	ufs_cmd_log.cmd_opcode = cmd->cmnd[0];
	ufs_cmd_log.tag = cmd->request->tag;
	ufs_cmd_log.outstanding_reqs = hba->outstanding_reqs;

	if(cmd->cmnd[0] != UNMAP)
		ufs_cmd_log.lba = lba;

	ufs_cmd_log.sct = sct;
	ufs_cmd_log.retries = cmd->retries;

	exynos_ufs_putItem_start(&ufs_cmd_queue, &ufs_cmd_log);
}


void exynos_ufs_cmd_log_end(struct ufs_hba *hba, int tag)
{
	int cpu = raw_smp_processor_id();

	ufs_cmd_queue.addr_per_tag[tag]->end_time = cpu_clock(cpu);
}
