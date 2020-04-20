/*
 *  Copyright (c) 2016 Realtek Semiconductor Corp. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *  This product is covered by one or more of the following patents:
 *  US6,570,884, US6,115,776, and US6,327,625.
 */

#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/usb.h>
#include <linux/crc32.h>
#include <linux/if_vlan.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ip6_checksum.h>
#include <linux/usb/cdc.h>
#include <linux/suspend.h>

#include "compatibility.h"

/* Version Information */
#define DRIVER_VERSION "v2.08.0 (2016/12/09)"
#define DRIVER_AUTHOR "Realtek nic sw <nic_swsd@realtek.com>"
#define DRIVER_DESC "Realtek RTL8152/RTL8153 Based USB Ethernet Adapters"
#define MODULENAME "r8152"

#define PATENTS		"This product is covered by one or more of the " \
			"following patents:\n" \
			"\t\tUS6,570,884, US6,115,776, and US6,327,625.\n"

#define R8152_PHY_ID		32

#define PLA_IDR			0xc000
#define PLA_RCR			0xc010
#define PLA_RMS			0xc016
#define PLA_RXFIFO_CTRL0	0xc0a0
#define PLA_RXFIFO_CTRL1	0xc0a4
#define PLA_RXFIFO_CTRL2	0xc0a8
#define PLA_DMY_REG0		0xc0b0
#define PLA_FMC			0xc0b4
#define PLA_CFG_WOL		0xc0b6
#define PLA_TEREDO_CFG		0xc0bc
#define PLA_TEREDO_WAKE_BASE	0xc0c4
#define PLA_MAR			0xcd00
#define PLA_BACKUP		0xd000
#define PAL_BDC_CR		0xd1a0
#define PLA_TEREDO_TIMER	0xd2cc
#define PLA_REALWOW_TIMER	0xd2e8
#define PLA_EFUSE_DATA		0xdd00
#define PLA_EFUSE_CMD		0xdd02
#define PLA_LEDSEL		0xdd90
#define PLA_LED_FEATURE		0xdd92
#define PLA_PHYAR		0xde00
#define PLA_BOOT_CTRL		0xe004
#define PLA_GPHY_INTR_IMR	0xe022
#define PLA_EEE_CR		0xe040
#define PLA_EEEP_CR		0xe080
#define PLA_MAC_PWR_CTRL	0xe0c0
#define PLA_MAC_PWR_CTRL2	0xe0ca
#define PLA_MAC_PWR_CTRL3	0xe0cc
#define PLA_MAC_PWR_CTRL4	0xe0ce
#define PLA_WDT6_CTRL		0xe428
#define PLA_TCR0		0xe610
#define PLA_TCR1		0xe612
#define PLA_MTPS		0xe615
#define PLA_TXFIFO_CTRL		0xe618
#define PLA_RSTTALLY		0xe800
#define PLA_CR			0xe813
#define PLA_CRWECR		0xe81c
#define PLA_CONFIG12		0xe81e	/* CONFIG1, CONFIG2 */
#define PLA_CONFIG34		0xe820	/* CONFIG3, CONFIG4 */
#define PLA_CONFIG5		0xe822
#define PLA_PHY_PWR		0xe84c
#define PLA_OOB_CTRL		0xe84f
#define PLA_CPCR		0xe854
#define PLA_MISC_0		0xe858
#define PLA_MISC_1		0xe85a
#define PLA_OCP_GPHY_BASE	0xe86c
#define PLA_TALLYCNT		0xe890
#define PLA_SFF_STS_7		0xe8de
#define PLA_PHYSTATUS		0xe908
#define PLA_BP_BA		0xfc26
#define PLA_BP_0		0xfc28
#define PLA_BP_1		0xfc2a
#define PLA_BP_2		0xfc2c
#define PLA_BP_3		0xfc2e
#define PLA_BP_4		0xfc30
#define PLA_BP_5		0xfc32
#define PLA_BP_6		0xfc34
#define PLA_BP_7		0xfc36
#define PLA_BP_EN		0xfc38

#define USB_USB2PHY		0xb41e
#define USB_SSPHYLINK2		0xb428
#define USB_U2P3_CTRL		0xb460
#define USB_CSR_DUMMY1		0xb464
#define USB_CSR_DUMMY2		0xb466
#define USB_DEV_STAT		0xb808
#define USB_CONNECT_TIMER	0xcbf8
#define USB_MSC_TIMER		0xcbfc
#define USB_BURST_SIZE		0xcfc0
#define USB_LPM_CONFIG		0xcfd8
#define USB_USB_CTRL		0xd406
#define USB_PHY_CTRL		0xd408
#define USB_TX_AGG		0xd40a
#define USB_RX_BUF_TH		0xd40c
#define USB_USB_TIMER		0xd428
#define USB_RX_EARLY_TIMEOUT	0xd42c
#define USB_RX_EARLY_SIZE	0xd42e
#define USB_PM_CTRL_STATUS	0xd432	/* RTL8153A */
#define USB_RX_EXTRA_AGGR_TMR	0xd432	/* RTL8153B */
#define USB_TX_DMA		0xd434
#define USB_UPT_RXDMA_OWN	0xd437
#define USB_TOLERANCE		0xd490
#define USB_LPM_CTRL		0xd41a
#define USB_BMU_RESET		0xd4b0
#define USB_U1U2_TIMER		0xd4da
#define USB_UPS_CTRL		0xd800
#define USB_POWER_CUT		0xd80a
#define USB_MISC_0		0xd81a
#define USB_AFE_CTRL2		0xd824
#define USB_UPS_CFG		0xd842
#define USB_UPS_FLAGS		0xd848
#define USB_WDT11_CTRL		0xe43c
#define USB_BP_BA		0xfc26
#define USB_BP_0		0xfc28
#define USB_BP_1		0xfc2a
#define USB_BP_2		0xfc2c
#define USB_BP_3		0xfc2e
#define USB_BP_4		0xfc30
#define USB_BP_5		0xfc32
#define USB_BP_6		0xfc34
#define USB_BP_7		0xfc36
#define USB_BP_EN		0xfc38
#define USB_BP_8		0xfc38
#define USB_BP_9		0xfc3a
#define USB_BP_10		0xfc3c
#define USB_BP_11		0xfc3e
#define USB_BP_12		0xfc40
#define USB_BP_13		0xfc42
#define USB_BP_14		0xfc44
#define USB_BP_15		0xfc46
#define USB_BP2_EN		0xfc48

/* OCP Registers */
#define OCP_ALDPS_CONFIG	0x2010
#define OCP_EEE_CONFIG1		0x2080
#define OCP_EEE_CONFIG2		0x2092
#define OCP_EEE_CONFIG3		0x2094
#define OCP_BASE_MII		0xa400
#define OCP_EEE_AR		0xa41a
#define OCP_EEE_DATA		0xa41c
#define OCP_PHY_STATUS		0xa420
#define OCP_NCTL_CFG		0xa42c
#define OCP_POWER_CFG		0xa430
#define OCP_EEE_CFG		0xa432
#define OCP_SRAM_ADDR		0xa436
#define OCP_SRAM_DATA		0xa438
#define OCP_DOWN_SPEED		0xa442
#define OCP_EEE_ABLE		0xa5c4
#define OCP_EEE_ADV		0xa5d0
#define OCP_EEE_LPABLE		0xa5d2
#define OCP_PHY_STATE		0xa708		/* nway state for 8153 */
#define OCP_PHY_PATCH_STAT	0xb800
#define OCP_PHY_PATCH_CMD	0xb820
#define OCP_ADC_IOFFSET		0xbcfc
#define OCP_ADC_CFG		0xbc06
#define OCP_SYSCLK_CFG		0xc416

/* SRAM Register */
#define SRAM_GREEN_CFG		0x8011
#define SRAM_LPF_CFG		0x8012
#define SRAM_10M_AMP1		0x8080
#define SRAM_10M_AMP2		0x8082
#define SRAM_IMPEDANCE		0x8084

/* PLA_RCR */
#define RCR_AAP			0x00000001
#define RCR_APM			0x00000002
#define RCR_AM			0x00000004
#define RCR_AB			0x00000008
#define RCR_ACPT_ALL		(RCR_AAP | RCR_APM | RCR_AM | RCR_AB)

/* PLA_RXFIFO_CTRL0 */
#define RXFIFO_THR1_NORMAL	0x00080002
#define RXFIFO_THR1_OOB		0x01800003

/* PLA_RXFIFO_CTRL1 */
#define RXFIFO_THR2_FULL	0x00000060
#define RXFIFO_THR2_HIGH	0x00000038
#define RXFIFO_THR2_OOB		0x0000004a
#define RXFIFO_THR2_NORMAL	0x00a0

/* PLA_RXFIFO_CTRL2 */
#define RXFIFO_THR3_FULL	0x00000078
#define RXFIFO_THR3_HIGH	0x00000048
#define RXFIFO_THR3_OOB		0x0000005a
#define RXFIFO_THR3_NORMAL	0x0110

/* PLA_TXFIFO_CTRL */
#define TXFIFO_THR_NORMAL	0x00400008
#define TXFIFO_THR_NORMAL2	0x01000008

/* PLA_DMY_REG0 */
#define ECM_ALDPS		0x0002

/* PLA_FMC */
#define FMC_FCR_MCU_EN		0x0001

/* PLA_EEEP_CR */
#define EEEP_CR_EEEP_TX		0x0002

/* PLA_WDT6_CTRL */
#define WDT6_SET_MODE		0x0010

/* PLA_TCR0 */
#define TCR0_TX_EMPTY		0x0800
#define TCR0_AUTO_FIFO		0x0080

/* PLA_TCR1 */
#define VERSION_MASK		0x7cf0

/* PLA_MTPS */
#define MTPS_JUMBO		(12 * 1024 / 64)
#define MTPS_DEFAULT		(6 * 1024 / 64)

/* PLA_RSTTALLY */
#define TALLY_RESET		0x0001

/* PLA_CR */
#define CR_RST			0x10
#define CR_RE			0x08
#define CR_TE			0x04

/* PLA_CRWECR */
#define CRWECR_NORAML		0x00
#define CRWECR_CONFIG		0xc0

/* PLA_OOB_CTRL */
#define NOW_IS_OOB		0x80
#define TXFIFO_EMPTY		0x20
#define RXFIFO_EMPTY		0x10
#define LINK_LIST_READY		0x02
#define DIS_MCU_CLROOB		0x01
#define FIFO_EMPTY		(TXFIFO_EMPTY | RXFIFO_EMPTY)

/* PLA_MISC_1 */
#define RXDY_GATED_EN		0x0008

/* PLA_SFF_STS_7 */
#define RE_INIT_LL		0x8000
#define MCU_BORW_EN		0x4000

/* PLA_CPCR */
#define CPCR_RX_VLAN		0x0040

/* PLA_CFG_WOL */
#define MAGIC_EN		0x0001

/* PLA_TEREDO_CFG */
#define TEREDO_SEL		0x8000
#define TEREDO_WAKE_MASK	0x7f00
#define TEREDO_RS_EVENT_MASK	0x00fe
#define OOB_TEREDO_EN		0x0001

/* PAL_BDC_CR */
#define ALDPS_PROXY_MODE	0x0001

/* PLA_EFUSE_CMD */
#define EFUSE_READ_CMD		BIT(15)
#define EFUSE_DATA_BIT16	BIT(7)

/* PLA_CONFIG34 */
#define LINK_ON_WAKE_EN		0x0010
#define LINK_OFF_WAKE_EN	0x0008

/* PLA_CONFIG5 */
#define BWF_EN			0x0040
#define MWF_EN			0x0020
#define UWF_EN			0x0010
#define LAN_WAKE_EN		0x0002

/* PLA_LED_FEATURE */
#define LED_MODE_MASK		0x0700

/* PLA_PHY_PWR */
#define TX_10M_IDLE_EN		0x0080
#define PFM_PWM_SWITCH		0x0040

/* PLA_MAC_PWR_CTRL */
#define D3_CLK_GATED_EN		0x00004000
#define MCU_CLK_RATIO		0x07010f07
#define MCU_CLK_RATIO_MASK	0x0f0f0f0f
#define ALDPS_SPDWN_RATIO	0x0f87

/* PLA_MAC_PWR_CTRL2 */
#define EEE_SPDWN_RATIO		0x8007
#define MAC_CLK_SPDWN_EN	BIT(15)

/* PLA_MAC_PWR_CTRL3 */
#define PKT_AVAIL_SPDWN_EN	0x0100
#define SUSPEND_SPDWN_EN	0x0004
#define U1U2_SPDWN_EN		0x0002
#define L1_SPDWN_EN		0x0001

/* PLA_MAC_PWR_CTRL4 */
#define PWRSAVE_SPDWN_EN	0x1000
#define RXDV_SPDWN_EN		0x0800
#define TX10MIDLE_EN		0x0100
#define TP100_SPDWN_EN		0x0020
#define TP500_SPDWN_EN		0x0010
#define TP1000_SPDWN_EN		0x0008
#define EEE_SPDWN_EN		0x0001

/* PLA_GPHY_INTR_IMR */
#define GPHY_STS_MSK		0x0001
#define SPEED_DOWN_MSK		0x0002
#define SPDWN_RXDV_MSK		0x0004
#define SPDWN_LINKCHG_MSK	0x0008

/* PLA_PHYAR */
#define PHYAR_FLAG		0x80000000

/* PLA_EEE_CR */
#define EEE_RX_EN		0x0001
#define EEE_TX_EN		0x0002

/* PLA_BOOT_CTRL */
#define AUTOLOAD_DONE		0x0002

/* USB_USB2PHY */
#define USB2PHY_SUSPEND		0x0001
#define USB2PHY_L1		0x0002

/* USB_SSPHYLINK2 */
#define pwd_dn_scale_mask	0x3ffe
#define pwd_dn_scale(x)		((x) << 1)

/* USB_CSR_DUMMY1 */
#define DYNAMIC_BURST		0x0001

/* USB_CSR_DUMMY2 */
#define EP4_FULL_FC		0x0001

/* USB_DEV_STAT */
#define STAT_SPEED_MASK		0x0006
#define STAT_SPEED_HIGH		0x0000
#define STAT_SPEED_FULL		0x0002

/* USB_LPM_CONFIG */
#define LPM_U1U2_EN		BIT(0)

/* USB_TX_AGG */
#define TX_AGG_MAX_THRESHOLD	0x03

/* USB_RX_BUF_TH */
#define RX_THR_SUPPER		0x0c350180
#define RX_THR_HIGH		0x7a120180
#define RX_THR_SLOW		0xffff0180
#define RX_THR_B		0x00010001

/* USB_TX_DMA */
#define TEST_MODE_DISABLE	0x00000001
#define TX_SIZE_ADJUST1		0x00000100

/* USB_BMU_RESET */
#define BMU_RESET_EP_IN		0x01
#define BMU_RESET_EP_OUT	0x02

/* USB_UPT_RXDMA_OWN */
#define OWN_UPDATE		BIT(0)
#define OWN_CLEAR		BIT(1)

/* USB_UPS_CTRL */
#define POWER_CUT		0x0100

/* USB_PM_CTRL_STATUS */
#define RESUME_INDICATE		0x0001

/* USB_USB_CTRL */
#define RX_AGG_DISABLE		0x0010
#define RX_ZERO_EN		0x0080

/* USB_U2P3_CTRL */
#define U2P3_ENABLE		0x0001

/* USB_POWER_CUT */
#define PWR_EN			0x0001
#define PHASE2_EN		0x0008
#define UPS_EN			BIT(4)
#define USP_PREWAKE		BIT(5)

/* USB_MISC_0 */
#define PCUT_STATUS		0x0001

/* USB_RX_EARLY_TIMEOUT */
#define COALESCE_SUPER		 85000U
#define COALESCE_HIGH		250000U
#define COALESCE_SLOW		524280U

/* USB_WDT11_CTRL */
#define TIMER11_EN		0x0001

/* USB_LPM_CTRL */
/* bit 4 ~ 5: fifo empty boundary */
#define FIFO_EMPTY_1FB		0x30	/* 0x1fb * 64 = 32448 bytes */
/* bit 2 ~ 3: LMP timer */
#define LPM_TIMER_MASK		0x0c
#define LPM_TIMER_500MS		0x04	/* 500 ms */
#define LPM_TIMER_500US		0x0c	/* 500 us */
#define ROK_EXIT_LPM		0x02

/* USB_AFE_CTRL2 */
#define SEN_VAL_MASK		0xf800
#define SEN_VAL_NORMAL		0xa000
#define SEL_RXIDLE		0x0100

/* USB_UPS_CFG */
#define SAW_CNT_1MS_MASK	0x0fff

/* USB_UPS_FLAGS */
#define UPS_FLAGS_R_TUNE		BIT(0)
#define UPS_FLAGS_EN_10M_CKDIV		BIT(1)
#define UPS_FLAGS_250M_CKDIV		BIT(2)
#define UPS_FLAGS_EN_ALDPS		BIT(3)
#define UPS_FLAGS_CTAP_SHORT_DIS	BIT(4)
#define UPS_FLAGS_SPEED_MASK		(0xf << 16)
#define ups_flags_speed(x)		((x) << 16)
#define UPS_FLAGS_EN_EEE		BIT(20)
#define UPS_FLAGS_EN_500M_EEE		BIT(21)
#define UPS_FLAGS_EN_EEE_CKDIV		BIT(22)
#define UPS_FLAGS_EEE_PLLOFF_GIGA	BIT(24)
#define UPS_FLAGS_EEE_CMOD_LV_EN	BIT(25)
#define UPS_FLAGS_EN_GREEN		BIT(26)
#define UPS_FLAGS_EN_FLOW_CTR		BIT(27)

enum spd_duplex {
	NWAY_10M_HALF = 1,
	NWAY_10M_FULL,
	NWAY_100M_HALF,
	NWAY_100M_FULL,
	NWAY_1000M_FULL,
	FORCE_10M_HALF,
	FORCE_10M_FULL,
	FORCE_100M_HALF,
	FORCE_100M_FULL,
};

/* OCP_ALDPS_CONFIG */
#define ENPWRSAVE		0x8000
#define ENPDNPS			0x0200
#define LINKENA			0x0100
#define DIS_SDSAVE		0x0010

/* OCP_PHY_STATUS */
#define PHY_STAT_MASK		0x0007
#define PHY_STAT_LAN_ON		3
#define PHY_STAT_PWRDN		5

/* OCP_NCTL_CFG */
#define PGA_RETURN_EN		BIT(1)

/* OCP_POWER_CFG */
#define EEE_CLKDIV_EN		0x8000
#define EN_ALDPS		0x0004
#define EN_10M_PLLOFF		0x0001

/* OCP_EEE_CONFIG1 */
#define RG_TXLPI_MSK_HFDUP	0x8000
#define RG_MATCLR_EN		0x4000
#define EEE_10_CAP		0x2000
#define EEE_NWAY_EN		0x1000
#define TX_QUIET_EN		0x0200
#define RX_QUIET_EN		0x0100
#define sd_rise_time_mask	0x0070
#define sd_rise_time(x)		(min(x, 7) << 4)	/* bit 4 ~ 6 */
#define RG_RXLPI_MSK_HFDUP	0x0008
#define SDFALLTIME		0x0007	/* bit 0 ~ 2 */

/* OCP_EEE_CONFIG2 */
#define RG_LPIHYS_NUM		0x7000	/* bit 12 ~ 15 */
#define RG_DACQUIET_EN		0x0400
#define RG_LDVQUIET_EN		0x0200
#define RG_CKRSEL		0x0020
#define RG_EEEPRG_EN		0x0010

/* OCP_EEE_CONFIG3 */
#define fast_snr_mask		0xff80
#define fast_snr(x)		(min(x, 0x1ff) << 7)	/* bit 7 ~ 15 */
#define RG_LFS_SEL		0x0060	/* bit 6 ~ 5 */
#define MSK_PH			0x0006	/* bit 0 ~ 3 */

/* OCP_EEE_AR */
/* bit[15:14] function */
#define FUN_ADDR		0x0000
#define FUN_DATA		0x4000
/* bit[4:0] device addr */

/* OCP_EEE_CFG */
#define CTAP_SHORT_EN		0x0040
#define EEE10_EN		0x0010

/* OCP_DOWN_SPEED */
#define EN_EEE_CMODE		BIT(14)
#define EN_EEE_1000		BIT(13)
#define EN_EEE_100		BIT(12)
#define EN_10M_CLKDIV		BIT(11)
#define EN_10M_BGOFF		0x0080

/* OCP_PHY_STATE */
#define TXDIS_STATE		0x01
#define ABD_STATE		0x02

/* OCP_PHY_PATCH_STAT */
#define PATCH_READY		BIT(6)

/* OCP_PHY_PATCH_CMD */
#define PATCH_REQUEST		BIT(4)

/* OCP_ADC_CFG */
#define CKADSEL_L		0x0100
#define ADC_EN			0x0080
#define EN_EMI_L		0x0040

/* OCP_SYSCLK_CFG */
#define clk_div_expo(x)		(min(x, 5) << 8)

/* SRAM_GREEN_CFG */
#define GREEN_ETH_EN		BIT(15)
#define R_TUNE_EN		BIT(11)

/* SRAM_LPF_CFG */
#define LPF_AUTO_TUNE		0x8000

/* SRAM_10M_AMP1 */
#define GDAC_IB_UPALL		0x0008

/* SRAM_10M_AMP2 */
#define AMP_DN			0x0200

/* SRAM_IMPEDANCE */
#define RX_DRIVING_MASK		0x6000

enum rtl_register_content {
	_1000bps	= 0x10,
	_100bps		= 0x08,
	_10bps		= 0x04,
	LINK_STATUS	= 0x02,
	FULL_DUP	= 0x01,
};

#define RTL8152_MAX_TX		4
#define RTL8152_MAX_RX		10
#define INTBUFSIZE		2
#define CRC_SIZE		4
#define TX_ALIGN		4
#define RX_ALIGN		8

#define INTR_LINK		0x0004

#define RTL8152_REQT_READ	0xc0
#define RTL8152_REQT_WRITE	0x40
#define RTL8152_REQ_GET_REGS	0x05
#define RTL8152_REQ_SET_REGS	0x05

#define BYTE_EN_DWORD		0xff
#define BYTE_EN_WORD		0x33
#define BYTE_EN_BYTE		0x11
#define BYTE_EN_SIX_BYTES	0x3f
#define BYTE_EN_START_MASK	0x0f
#define BYTE_EN_END_MASK	0xf0

#define RTL8153_MAX_PACKET	9216 /* 9K */
#define RTL8153_MAX_MTU		(RTL8153_MAX_PACKET - VLAN_ETH_HLEN - CRC_SIZE)
#define RTL8152_RMS		(VLAN_ETH_FRAME_LEN + CRC_SIZE)
#define RTL8153_RMS		RTL8153_MAX_PACKET
#define RTL8152_TX_TIMEOUT	(5 * HZ)
#define RTL8152_NAPI_WEIGHT	64
#define rx_reserved_size(x)	((x) + VLAN_ETH_HLEN + CRC_SIZE + \
				 sizeof(struct rx_desc) + RX_ALIGN)

/* rtl8152 flags */
enum rtl8152_flags {
	RTL8152_UNPLUG = 0,
	RTL8152_SET_RX_MODE,
	WORK_ENABLE,
	RTL8152_LINK_CHG,
	SELECTIVE_SUSPEND,
	PHY_RESET,
	SCHEDULE_NAPI,
};

/* Define these values to match your device */
#define VENDOR_ID_REALTEK		0x0bda
#define VENDOR_ID_SAMSUNG		0x04e8
#define VENDOR_ID_LENOVO		0x17ef
#define VENDOR_ID_TPLINK		0x2357
#define VENDOR_ID_NVIDIA		0x0955

#define MCU_TYPE_PLA			0x0100
#define MCU_TYPE_USB			0x0000

struct tally_counter {
	__le64	tx_packets;
	__le64	rx_packets;
	__le64	tx_errors;
	__le32	rx_errors;
	__le16	rx_missed;
	__le16	align_errors;
	__le32	tx_one_collision;
	__le32	tx_multi_collision;
	__le64	rx_unicast;
	__le64	rx_broadcast;
	__le32	rx_multicast;
	__le16	tx_aborted;
	__le16	tx_underrun;
};

struct rx_desc {
	__le32 opts1;
#define RD_CRC				BIT(15)
#define RX_LEN_MASK			0x7fff

	__le32 opts2;
#define RD_UDP_CS			BIT(23)
#define RD_TCP_CS			BIT(22)
#define RD_IPV6_CS			BIT(20)
#define RD_IPV4_CS			BIT(19)

	__le32 opts3;
#define IPF				BIT(23) /* IP checksum fail */
#define UDPF				BIT(22) /* UDP checksum fail */
#define TCPF				BIT(21) /* TCP checksum fail */
#define RX_VLAN_TAG			BIT(16)

	__le32 opts4;
	__le32 opts5;
	__le32 opts6;
};

struct tx_desc {
	__le32 opts1;
#define TX_FS			BIT(31) /* First segment of a packet */
#define TX_LS			BIT(30) /* Final segment of a packet */
#define LGSEND			BIT(29)
#define GTSENDV4		BIT(28)
#define GTSENDV6		BIT(27)
#define GTTCPHO_SHIFT		18
#define GTTCPHO_MAX		0x7fU
#define TX_LEN_MAX		0x3ffffU

	__le32 opts2;
#define UDP_CS			BIT(31) /* Calculate UDP/IP checksum */
#define TCP_CS			BIT(30) /* Calculate TCP/IP checksum */
#define IPV4_CS			BIT(29) /* Calculate IPv4 checksum */
#define IPV6_CS			BIT(28) /* Calculate IPv6 checksum */
#define MSS_SHIFT		17
#define MSS_MAX			0x7ffU
#define TCPHO_SHIFT		17
#define TCPHO_MAX		0x7ffU
#define TX_VLAN_TAG		BIT(16)
};

struct r8152;

struct rx_agg {
	struct list_head list;
	struct urb *urb;
	struct r8152 *context;
	void *buffer;
	void *head;
};

struct tx_agg {
	struct list_head list;
	struct urb *urb;
	struct r8152 *context;
	void *buffer;
	void *head;
	u32 skb_num;
	u32 skb_len;
};

struct r8152 {
	unsigned long flags;
	struct usb_device *udev;
	struct napi_struct napi;
	struct usb_interface *intf;
	struct net_device *netdev;
	struct urb *intr_urb;
	struct tx_agg tx_info[RTL8152_MAX_TX];
	struct rx_agg rx_info[RTL8152_MAX_RX];
	struct list_head rx_done, tx_free;
	struct sk_buff_head tx_queue, rx_queue;
	spinlock_t rx_lock, tx_lock;
	struct delayed_work schedule, hw_phy_work;
	struct mii_if_info mii;
	struct mutex control;	/* use for hw setting */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	struct vlan_group *vlgrp;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
	struct net_device_stats stats;
#endif

	struct rtl_ops {
		void (*init)(struct r8152 *);
		int (*enable)(struct r8152 *);
		void (*disable)(struct r8152 *);
		void (*up)(struct r8152 *);
		void (*down)(struct r8152 *);
		void (*unload)(struct r8152 *);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
		int (*eee_get)(struct r8152 *, struct ethtool_eee *);
		int (*eee_set)(struct r8152 *, struct ethtool_eee *);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */
		bool (*in_nway)(struct r8152 *);
		void (*hw_phy_cfg)(struct r8152 *);
		void (*aldps_enable)(struct r8152 *tp, bool enable);
		void (*u1u2_enable)(struct r8152 *tp, bool enable);
		void (*u2p3_enable)(struct r8152 *tp, bool enable);
		void (*power_cut_en)(struct r8152 *tp, bool enable);
		void (*autosuspend_en)(struct r8152 *tp, bool enable);
	} rtl_ops;

	int intr_interval;
	u32 saved_wolopts;
	u32 msg_enable;
	u32 tx_qlen;
	u32 coalesce;
	u16 ocp_base;
	u16 speed;
	u8 *intr_buff;
	u8 version;
	u8 rtk_enable_diag;
	u8 duplex;
	u8 autoneg;
	u32 bottom_half_event;
	u32 bottom_half_wait_time;
	wait_queue_head_t bottom_half_wait_q;
};

enum rtl_version {
	RTL_VER_UNKNOWN = 0,
	RTL_VER_01,
	RTL_VER_02,
	RTL_VER_03,
	RTL_VER_04,
	RTL_VER_05,
	RTL_VER_06,
	RTL_VER_07,
	RTL_VER_08,
	RTL_VER_09,
	RTL_VER_MAX
};

enum tx_csum_stat {
	TX_CSUM_SUCCESS = 0,
	TX_CSUM_TSO,
	TX_CSUM_NONE
};

/* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
 * The RTL chips use a 64 element hash table based on the Ethernet CRC.
 */
static const int multicast_filter_limit = 32;
static unsigned int agg_buf_sz = 16384;

#define RTL_LIMITED_TSO_SIZE	(agg_buf_sz - sizeof(struct tx_desc) - \
				 VLAN_ETH_HLEN - CRC_SIZE)

static
int get_registers(struct r8152 *tp, u16 value, u16 index, u16 size, void *data)
{
	int ret;
	void *tmp;

	tmp = kmalloc(size, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = usb_control_msg(tp->udev, usb_rcvctrlpipe(tp->udev, 0),
			      RTL8152_REQ_GET_REGS, RTL8152_REQT_READ,
			      value, index, tmp, size, 500);
	if (ret < 0)
		memset(data, 0xff, size);
	else
		memcpy(data, tmp, size);

	kfree(tmp);

	return ret;
}

static
int set_registers(struct r8152 *tp, u16 value, u16 index, u16 size, void *data)
{
	int ret;
	void *tmp;

	tmp = kmemdup(data, size, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = usb_control_msg(tp->udev, usb_sndctrlpipe(tp->udev, 0),
			      RTL8152_REQ_SET_REGS, RTL8152_REQT_WRITE,
			      value, index, tmp, size, 500);

	kfree(tmp);

	return ret;
}

static int generic_ocp_read(struct r8152 *tp, u16 index, u16 size,
			    void *data, u16 type)
{
	u16 limit = 64;
	int ret = 0;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return -ENODEV;

	/* both size and indix must be 4 bytes align */
	if ((size & 3) || !size || (index & 3) || !data)
		return -EPERM;

	if ((u32)index + (u32)size > 0xffff)
		return -EPERM;

	while (size) {
		if (size > limit) {
			ret = get_registers(tp, index, type, limit, data);
			if (ret < 0)
				break;

			index += limit;
			data += limit;
			size -= limit;
		} else {
			ret = get_registers(tp, index, type, size, data);
			if (ret < 0)
				break;

			index += size;
			data += size;
			size = 0;
			break;
		}
	}

	if (ret == -ENODEV)
		set_bit(RTL8152_UNPLUG, &tp->flags);

	return ret;
}

static int generic_ocp_write(struct r8152 *tp, u16 index, u16 byteen,
			     u16 size, void *data, u16 type)
{
	int ret;
	u16 byteen_start, byteen_end, byen;
	u16 limit = 512;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return -ENODEV;

	/* both size and indix must be 4 bytes align */
	if ((size & 3) || !size || (index & 3) || !data)
		return -EPERM;

	if ((u32)index + (u32)size > 0xffff)
		return -EPERM;

	byteen_start = byteen & BYTE_EN_START_MASK;
	byteen_end = byteen & BYTE_EN_END_MASK;

	byen = byteen_start | (byteen_start << 4);
	ret = set_registers(tp, index, type | byen, 4, data);
	if (ret < 0)
		goto error1;

	index += 4;
	data += 4;
	size -= 4;

	if (size) {
		size -= 4;

		while (size) {
			if (size > limit) {
				ret = set_registers(tp, index,
						    type | BYTE_EN_DWORD,
						    limit, data);
				if (ret < 0)
					goto error1;

				index += limit;
				data += limit;
				size -= limit;
			} else {
				ret = set_registers(tp, index,
						    type | BYTE_EN_DWORD,
						    size, data);
				if (ret < 0)
					goto error1;

				index += size;
				data += size;
				size = 0;
				break;
			}
		}

		byen = byteen_end | (byteen_end >> 4);
		ret = set_registers(tp, index, type | byen, 4, data);
		if (ret < 0)
			goto error1;
	}

error1:
	if (ret == -ENODEV)
		set_bit(RTL8152_UNPLUG, &tp->flags);

	return ret;
}

static inline
int pla_ocp_read(struct r8152 *tp, u16 index, u16 size, void *data)
{
	return generic_ocp_read(tp, index, size, data, MCU_TYPE_PLA);
}

static inline
int pla_ocp_write(struct r8152 *tp, u16 index, u16 byteen, u16 size, void *data)
{
	return generic_ocp_write(tp, index, byteen, size, data, MCU_TYPE_PLA);
}

static inline
int usb_ocp_read(struct r8152 *tp, u16 index, u16 size, void *data)
{
	return generic_ocp_read(tp, index, size, data, MCU_TYPE_USB);
}

static inline
int usb_ocp_write(struct r8152 *tp, u16 index, u16 byteen, u16 size, void *data)
{
	return generic_ocp_write(tp, index, byteen, size, data, MCU_TYPE_USB);
}

static u32 ocp_read_dword(struct r8152 *tp, u16 type, u16 index)
{
	__le32 data;

	generic_ocp_read(tp, index, sizeof(data), &data, type);

	return __le32_to_cpu(data);
}

static void ocp_write_dword(struct r8152 *tp, u16 type, u16 index, u32 data)
{
	__le32 tmp = __cpu_to_le32(data);

	generic_ocp_write(tp, index, BYTE_EN_DWORD, sizeof(tmp), &tmp, type);
}

static u16 ocp_read_word(struct r8152 *tp, u16 type, u16 index)
{
	u32 data;
	__le32 tmp;
	u16 byen = BYTE_EN_WORD;
	u8 shift = index & 2;

	index &= ~3;
	byen <<= shift;

	generic_ocp_read(tp, index, sizeof(tmp), &tmp, type | byen);

	data = __le32_to_cpu(tmp);
	data >>= (shift * 8);
	data &= 0xffff;

	return (u16)data;
}

static void ocp_write_word(struct r8152 *tp, u16 type, u16 index, u32 data)
{
	u32 mask = 0xffff;
	__le32 tmp;
	u16 byen = BYTE_EN_WORD;
	u8 shift = index & 2;

	data &= mask;

	if (index & 2) {
		byen <<= shift;
		mask <<= (shift * 8);
		data <<= (shift * 8);
		index &= ~3;
	}

	tmp = __cpu_to_le32(data);

	generic_ocp_write(tp, index, byen, sizeof(tmp), &tmp, type);
}

static u8 ocp_read_byte(struct r8152 *tp, u16 type, u16 index)
{
	u32 data;
	__le32 tmp;
	u8 shift = index & 3;

	index &= ~3;

	generic_ocp_read(tp, index, sizeof(tmp), &tmp, type);

	data = __le32_to_cpu(tmp);
	data >>= (shift * 8);
	data &= 0xff;

	return (u8)data;
}

static void ocp_write_byte(struct r8152 *tp, u16 type, u16 index, u32 data)
{
	u32 mask = 0xff;
	__le32 tmp;
	u16 byen = BYTE_EN_BYTE;
	u8 shift = index & 3;

	data &= mask;

	if (index & 3) {
		byen <<= shift;
		mask <<= (shift * 8);
		data <<= (shift * 8);
		index &= ~3;
	}

	tmp = __cpu_to_le32(data);

	generic_ocp_write(tp, index, byen, sizeof(tmp), &tmp, type);
}

static u16 ocp_reg_read(struct r8152 *tp, u16 addr)
{
	u16 ocp_base, ocp_index;

	ocp_base = addr & 0xf000;
	if (ocp_base != tp->ocp_base) {
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE, ocp_base);
		tp->ocp_base = ocp_base;
	}

	ocp_index = (addr & 0x0fff) | 0xb000;
	return ocp_read_word(tp, MCU_TYPE_PLA, ocp_index);
}

static void ocp_reg_write(struct r8152 *tp, u16 addr, u16 data)
{
	u16 ocp_base, ocp_index;

	ocp_base = addr & 0xf000;
	if (ocp_base != tp->ocp_base) {
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE, ocp_base);
		tp->ocp_base = ocp_base;
	}

	ocp_index = (addr & 0x0fff) | 0xb000;
	ocp_write_word(tp, MCU_TYPE_PLA, ocp_index, data);
}

static inline void r8152_mdio_write(struct r8152 *tp, u32 reg_addr, u32 value)
{
	ocp_reg_write(tp, OCP_BASE_MII + reg_addr * 2, value);
}

static inline int r8152_mdio_read(struct r8152 *tp, u32 reg_addr)
{
	return ocp_reg_read(tp, OCP_BASE_MII + reg_addr * 2);
}

static void sram_write(struct r8152 *tp, u16 addr, u16 data)
{
	ocp_reg_write(tp, OCP_SRAM_ADDR, addr);
	ocp_reg_write(tp, OCP_SRAM_DATA, data);
}

static u16 sram_read(struct r8152 *tp, u16 addr)
{
	ocp_reg_write(tp, OCP_SRAM_ADDR, addr);
	return ocp_reg_read(tp, OCP_SRAM_DATA);
}

static int read_mii_word(struct net_device *netdev, int phy_id, int reg)
{
	struct r8152 *tp = netdev_priv(netdev);
	int ret;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return -ENODEV;

	if (phy_id != R8152_PHY_ID)
		return -EINVAL;

	ret = r8152_mdio_read(tp, reg);

	return ret;
}

static
void write_mii_word(struct net_device *netdev, int phy_id, int reg, int val)
{
	struct r8152 *tp = netdev_priv(netdev);

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	if (phy_id != R8152_PHY_ID)
		return;

	r8152_mdio_write(tp, reg, val);
}

static int
r8152_submit_rx(struct r8152 *tp, struct rx_agg *agg, gfp_t mem_flags);

static int rtl8152_set_mac_address(struct net_device *netdev, void *p)
{
	struct r8152 *tp = netdev_priv(netdev);
	struct sockaddr *addr = p;
	int ret = -EADDRNOTAVAIL;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	if (!is_valid_ether_addr(addr->sa_data))
		goto out1;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out1;

	mutex_lock(&tp->control);

	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);

	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_CONFIG);
	pla_ocp_write(tp, PLA_IDR, BYTE_EN_SIX_BYTES, 8, addr->sa_data);
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);
out1:
	return ret;
}

static int set_ethernet_addr(struct r8152 *tp)
{
	struct net_device *dev = tp->netdev;
	struct sockaddr sa;
	int ret;

	if (tp->version == RTL_VER_01)
		ret = pla_ocp_read(tp, PLA_IDR, 8, sa.sa_data);
	else
		ret = pla_ocp_read(tp, PLA_BACKUP, 8, sa.sa_data);

	if (ret < 0) {
		netif_err(tp, probe, dev, "Get ether addr fail\n");
	} else if (!is_valid_ether_addr(sa.sa_data)) {
		netif_err(tp, probe, dev, "Invalid ether addr %pM\n",
			  sa.sa_data);
		eth_hw_addr_random(dev);
		ether_addr_copy(sa.sa_data, dev->dev_addr);
		ret = rtl8152_set_mac_address(dev, &sa);
		netif_info(tp, probe, dev, "Random ether addr %pM\n",
			   sa.sa_data);
	} else {
		if (tp->version == RTL_VER_01)
			ether_addr_copy(dev->dev_addr, sa.sa_data);
		else
			ret = rtl8152_set_mac_address(dev, &sa);
	}

	return ret;
}

static inline struct net_device_stats *rtl8152_get_stats(struct net_device *dev)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
	struct rtl8152 *tp = netdev_priv(dev);

	return (struct net_device_stats *)&tp->stats;
#else
	return &dev->stats;
#endif
}

static void read_bulk_callback(struct urb *urb)
{
	struct net_device *netdev;
	int status = urb->status;
	struct rx_agg *agg;
	struct r8152 *tp;

	agg = urb->context;
	if (!agg)
		return;

	tp = agg->context;
	if (!tp)
		return;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	if (!test_bit(WORK_ENABLE, &tp->flags))
		return;

	netdev = tp->netdev;

	/* When link down, the driver would cancel all bulks. */
	/* This avoid the re-submitting bulk */
	if (!netif_carrier_ok(netdev))
		return;

	usb_mark_last_busy(tp->udev);

	switch (status) {
	case 0:
		if (urb->actual_length < ETH_ZLEN)
			break;

		spin_lock(&tp->rx_lock);
		list_add_tail(&agg->list, &tp->rx_done);
		spin_unlock(&tp->rx_lock);
		napi_schedule(&tp->napi);
		return;
	case -ESHUTDOWN:
		set_bit(RTL8152_UNPLUG, &tp->flags);
		netif_device_detach(tp->netdev);
		return;
	case -ENOENT:
		return;	/* the urb is in unlink state */
	case -ETIME:
		if (net_ratelimit())
			netif_warn(tp, rx_err, netdev,
				   "maybe reset is needed?\n");
		break;
	default:
		if (net_ratelimit())
			netif_warn(tp, rx_err, netdev,
				   "Rx status %d\n", status);
		break;
	}

	r8152_submit_rx(tp, agg, GFP_ATOMIC);
}

static void write_bulk_callback(struct urb *urb)
{
	struct net_device_stats *stats;
	struct net_device *netdev;
	struct tx_agg *agg;
	struct r8152 *tp;
	int status = urb->status;

	agg = urb->context;
	if (!agg)
		return;

	tp = agg->context;
	if (!tp)
		return;

	netdev = tp->netdev;
	stats = rtl8152_get_stats(netdev);
	if (status) {
		if (net_ratelimit())
			netif_warn(tp, tx_err, netdev,
				   "Tx status %d\n", status);
		stats->tx_errors += agg->skb_num;
	} else {
		stats->tx_packets += agg->skb_num;
		stats->tx_bytes += agg->skb_len;
	}

	spin_lock(&tp->tx_lock);
	list_add_tail(&agg->list, &tp->tx_free);
	spin_unlock(&tp->tx_lock);

	usb_autopm_put_interface_async(tp->intf);

	if (!netif_carrier_ok(netdev))
		return;

	if (!test_bit(WORK_ENABLE, &tp->flags))
		return;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	if (!skb_queue_empty(&tp->tx_queue))
		napi_schedule(&tp->napi);
}

static void intr_callback(struct urb *urb)
{
	struct r8152 *tp;
	__le16 *d;
	int status = urb->status;
	int res;

	tp = urb->context;
	if (!tp)
		return;

	if (!test_bit(WORK_ENABLE, &tp->flags))
		return;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	switch (status) {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ESHUTDOWN:
		netif_device_detach(tp->netdev);
	case -ENOENT:
	case -EPROTO:
		netif_info(tp, intr, tp->netdev,
			   "Stop submitting intr, status %d\n", status);
		return;
	case -EOVERFLOW:
		netif_info(tp, intr, tp->netdev, "intr status -EOVERFLOW\n");
		goto resubmit;
	/* -EPIPE:  should clear the halt */
	default:
		netif_info(tp, intr, tp->netdev, "intr status %d\n", status);
		goto resubmit;
	}

	d = urb->transfer_buffer;
	if (INTR_LINK & __le16_to_cpu(d[0])) {
		if (!netif_carrier_ok(tp->netdev)) {
			set_bit(RTL8152_LINK_CHG, &tp->flags);
			schedule_delayed_work(&tp->schedule, 0);
		}
	} else {
		if (netif_carrier_ok(tp->netdev)) {
			set_bit(RTL8152_LINK_CHG, &tp->flags);
			schedule_delayed_work(&tp->schedule, 0);
		}
	}

resubmit:
	res = usb_submit_urb(urb, GFP_ATOMIC);
	if (res == -ENODEV) {
		set_bit(RTL8152_UNPLUG, &tp->flags);
		netif_device_detach(tp->netdev);
	} else if (res) {
		netif_err(tp, intr, tp->netdev,
			  "can't resubmit intr, status %d\n", res);
	}
}

static inline void *rx_agg_align(void *data)
{
	return (void *)ALIGN((uintptr_t)data, RX_ALIGN);
}

static inline void *tx_agg_align(void *data)
{
	return (void *)ALIGN((uintptr_t)data, TX_ALIGN);
}

static void free_all_mem(struct r8152 *tp)
{
	int i;

	for (i = 0; i < RTL8152_MAX_RX; i++) {
		usb_free_urb(tp->rx_info[i].urb);
		tp->rx_info[i].urb = NULL;

		kfree(tp->rx_info[i].buffer);
		tp->rx_info[i].buffer = NULL;
		tp->rx_info[i].head = NULL;
	}

	for (i = 0; i < RTL8152_MAX_TX; i++) {
		usb_free_urb(tp->tx_info[i].urb);
		tp->tx_info[i].urb = NULL;

		kfree(tp->tx_info[i].buffer);
		tp->tx_info[i].buffer = NULL;
		tp->tx_info[i].head = NULL;
	}

	usb_free_urb(tp->intr_urb);
	tp->intr_urb = NULL;

	kfree(tp->intr_buff);
	tp->intr_buff = NULL;
}

static int alloc_all_mem(struct r8152 *tp)
{
	struct net_device *netdev = tp->netdev;
	struct usb_interface *intf = tp->intf;
	struct usb_host_interface *alt = intf->cur_altsetting;
	struct usb_host_endpoint *ep_intr = alt->endpoint + 2;
	struct urb *urb;
	int node, i;
	u8 *buf;

	node = netdev->dev.parent ? dev_to_node(netdev->dev.parent) : -1;

	spin_lock_init(&tp->rx_lock);
	spin_lock_init(&tp->tx_lock);
	INIT_LIST_HEAD(&tp->tx_free);
	skb_queue_head_init(&tp->tx_queue);
	skb_queue_head_init(&tp->rx_queue);

	for (i = 0; i < RTL8152_MAX_RX; i++) {
		buf = kmalloc_node(agg_buf_sz, GFP_KERNEL, node);
		if (!buf)
			goto err1;

		if (buf != rx_agg_align(buf)) {
			kfree(buf);
			buf = kmalloc_node(agg_buf_sz + RX_ALIGN, GFP_KERNEL,
					   node);
			if (!buf)
				goto err1;
		}

		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			kfree(buf);
			goto err1;
		}

		INIT_LIST_HEAD(&tp->rx_info[i].list);
		tp->rx_info[i].context = tp;
		tp->rx_info[i].urb = urb;
		tp->rx_info[i].buffer = buf;
		tp->rx_info[i].head = rx_agg_align(buf);
	}

	for (i = 0; i < RTL8152_MAX_TX; i++) {
		buf = kmalloc_node(agg_buf_sz, GFP_KERNEL, node);
		if (!buf)
			goto err1;

		if (buf != tx_agg_align(buf)) {
			kfree(buf);
			buf = kmalloc_node(agg_buf_sz + TX_ALIGN, GFP_KERNEL,
					   node);
			if (!buf)
				goto err1;
		}

		urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!urb) {
			kfree(buf);
			goto err1;
		}

		INIT_LIST_HEAD(&tp->tx_info[i].list);
		tp->tx_info[i].context = tp;
		tp->tx_info[i].urb = urb;
		tp->tx_info[i].buffer = buf;
		tp->tx_info[i].head = tx_agg_align(buf);

		list_add_tail(&tp->tx_info[i].list, &tp->tx_free);
	}

	tp->intr_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!tp->intr_urb)
		goto err1;

	tp->intr_buff = kmalloc(INTBUFSIZE, GFP_KERNEL);
	if (!tp->intr_buff)
		goto err1;

	tp->intr_interval = (int)ep_intr->desc.bInterval;
	usb_fill_int_urb(tp->intr_urb, tp->udev, usb_rcvintpipe(tp->udev, 3),
			 tp->intr_buff, INTBUFSIZE, intr_callback,
			 tp, tp->intr_interval);

	return 0;

err1:
	free_all_mem(tp);
	return -ENOMEM;
}

static struct tx_agg *r8152_get_tx_agg(struct r8152 *tp)
{
	struct tx_agg *agg = NULL;
	unsigned long flags;

	if (list_empty(&tp->tx_free))
		return NULL;

	spin_lock_irqsave(&tp->tx_lock, flags);
	if (!list_empty(&tp->tx_free)) {
		struct list_head *cursor;

		cursor = tp->tx_free.next;
		list_del_init(cursor);
		agg = list_entry(cursor, struct tx_agg, list);
	}
	spin_unlock_irqrestore(&tp->tx_lock, flags);

	return agg;
}

/* r8152_csum_workaround()
 * The hw limites the value the transport offset. When the offset is out of the
 * range, calculate the checksum by sw.
 */
static void r8152_csum_workaround(struct r8152 *tp, struct sk_buff *skb,
				  struct sk_buff_head *list)
{
	if (skb_shinfo(skb)->gso_size) {
		netdev_features_t features = tp->netdev->features;
		struct sk_buff_head seg_list;
		struct sk_buff *segs, *nskb;

		features &= ~(NETIF_F_SG | NETIF_F_IPV6_CSUM | NETIF_F_TSO6);
		segs = skb_gso_segment(skb, features);
		if (IS_ERR(segs) || !segs)
			goto drop;

		__skb_queue_head_init(&seg_list);

		do {
			nskb = segs;
			segs = segs->next;
			nskb->next = NULL;
			__skb_queue_tail(&seg_list, nskb);
		} while (segs);

		skb_queue_splice(&seg_list, list);
		dev_kfree_skb(skb);
	} else if (skb->ip_summed == CHECKSUM_PARTIAL) {
		if (skb_checksum_help(skb) < 0)
			goto drop;

		__skb_queue_head(list, skb);
	} else {
		struct net_device_stats *stats;

drop:
		stats = rtl8152_get_stats(tp->netdev);
		stats->tx_dropped++;
		dev_kfree_skb(skb);
	}
}

/* msdn_giant_send_check()
 * According to the document of microsoft, the TCP Pseudo Header excludes the
 * packet length for IPv6 TCP large packets.
 */
static int msdn_giant_send_check(struct sk_buff *skb)
{
	const struct ipv6hdr *ipv6h;
	struct tcphdr *th;
	int ret;

	ret = skb_cow_head(skb, 0);
	if (ret)
		return ret;

	ipv6h = ipv6_hdr(skb);
	th = tcp_hdr(skb);

	th->check = 0;
	th->check = ~tcp_v6_check(0, &ipv6h->saddr, &ipv6h->daddr, 0);

	return ret;
}

static inline void rtl_tx_vlan_tag(struct tx_desc *desc, struct sk_buff *skb)
{
	if (skb_vlan_tag_present(skb)) {
		u32 opts2;

		opts2 = TX_VLAN_TAG | swab16(skb_vlan_tag_get(skb));
		desc->opts2 |= cpu_to_le32(opts2);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

static inline bool
rtl_rx_vlan_tag(struct r8152 *tp, struct rx_desc *desc, struct sk_buff *skb)
{
	u32 opts2 = le32_to_cpu(desc->opts2);

	if (tp->vlgrp && (opts2 & RX_VLAN_TAG)) {
		vlan_gro_receive(&tp->napi, tp->vlgrp, swab16(opts2 & 0xffff),
				 skb);
		return true;
	}

	return false;
}

static inline void
rtl_vlan_put_tag(struct r8152 *tp, struct rx_desc *desc, struct sk_buff *skb)
{
	u32 opts2 = le32_to_cpu(desc->opts2);

	if (tp->vlgrp && (opts2 & RX_VLAN_TAG))
		__vlan_hwaccel_put_tag(skb, swab16(opts2 & 0xffff));
}

static inline __u16
rtl_vlan_get_tag(struct sk_buff *skb)
{
	__u16 tag;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
	__vlan_hwaccel_get_tag(skb, &tag);
#else
	tag = skb->vlan_tci;
#endif

	return tag;
}

#else

static inline void rtl_rx_vlan_tag(struct rx_desc *desc, struct sk_buff *skb)
{
	u32 opts2 = le32_to_cpu(desc->opts2);

	if (opts2 & RX_VLAN_TAG)
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
		__vlan_hwaccel_put_tag(skb, swab16(opts2 & 0xffff));
#else
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q),
				       swab16(opts2 & 0xffff));
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0) */
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0) */

static int r8152_tx_csum(struct r8152 *tp, struct tx_desc *desc,
			 struct sk_buff *skb, u32 len, u32 transport_offset)
{
	u32 mss = skb_shinfo(skb)->gso_size;
	u32 opts1, opts2 = 0;
	int ret = TX_CSUM_SUCCESS;

	WARN_ON_ONCE(len > TX_LEN_MAX);

	opts1 = len | TX_FS | TX_LS;

	if (mss) {
		if (transport_offset > GTTCPHO_MAX) {
			netif_warn(tp, tx_err, tp->netdev,
				   "Invalid transport offset 0x%x for TSO\n",
				   transport_offset);
			ret = TX_CSUM_TSO;
			goto unavailable;
		}

		switch (vlan_get_protocol(skb)) {
		case htons(ETH_P_IP):
			opts1 |= GTSENDV4;
			break;

		case htons(ETH_P_IPV6):
			if (msdn_giant_send_check(skb)) {
				ret = TX_CSUM_TSO;
				goto unavailable;
			}
			opts1 |= GTSENDV6;
			break;

		default:
			WARN_ON_ONCE(1);
			break;
		}

		opts1 |= transport_offset << GTTCPHO_SHIFT;
		opts2 |= min(mss, MSS_MAX) << MSS_SHIFT;
	} else if (skb->ip_summed == CHECKSUM_PARTIAL) {
		u8 ip_protocol;

		if (transport_offset > TCPHO_MAX) {
			netif_warn(tp, tx_err, tp->netdev,
				   "Invalid transport offset 0x%x\n",
				   transport_offset);
			ret = TX_CSUM_NONE;
			goto unavailable;
		}

		switch (vlan_get_protocol(skb)) {
		case htons(ETH_P_IP):
			opts2 |= IPV4_CS;
			ip_protocol = ip_hdr(skb)->protocol;
			break;

		case htons(ETH_P_IPV6):
			opts2 |= IPV6_CS;
			ip_protocol = ipv6_hdr(skb)->nexthdr;
			break;

		default:
			ip_protocol = IPPROTO_RAW;
			break;
		}

		if (ip_protocol == IPPROTO_TCP)
			opts2 |= TCP_CS;
		else if (ip_protocol == IPPROTO_UDP)
			opts2 |= UDP_CS;
		else
			WARN_ON_ONCE(1);

		opts2 |= transport_offset << TCPHO_SHIFT;
	}

	desc->opts2 = cpu_to_le32(opts2);
	desc->opts1 = cpu_to_le32(opts1);

unavailable:
	return ret;
}

static int r8152_tx_agg_fill(struct r8152 *tp, struct tx_agg *agg)
{
	struct sk_buff_head skb_head, *tx_queue = &tp->tx_queue;
	int remain, ret;
	u8 *tx_data;

	__skb_queue_head_init(&skb_head);
	spin_lock(&tx_queue->lock);
	skb_queue_splice_init(tx_queue, &skb_head);
	spin_unlock(&tx_queue->lock);

	tx_data = agg->head;
	agg->skb_num = 0;
	agg->skb_len = 0;
	remain = agg_buf_sz;

	while (remain >= ETH_ZLEN + sizeof(struct tx_desc)) {
		struct tx_desc *tx_desc;
		struct sk_buff *skb;
		unsigned int len;
		u32 offset;

		skb = __skb_dequeue(&skb_head);
		if (!skb)
			break;

		len = skb->len + sizeof(*tx_desc);

		if (len > remain) {
			__skb_queue_head(&skb_head, skb);
			break;
		}

		tx_data = tx_agg_align(tx_data);
		tx_desc = (struct tx_desc *)tx_data;

		offset = (u32)skb_transport_offset(skb);

		if (r8152_tx_csum(tp, tx_desc, skb, skb->len, offset)) {
			r8152_csum_workaround(tp, skb, &skb_head);
			continue;
		}

		rtl_tx_vlan_tag(tx_desc, skb);

		tx_data += sizeof(*tx_desc);

		len = skb->len;

		if ((u8 *)agg->head + agg_buf_sz < (tx_data + len)
				|| (u8 *)agg->head > tx_data) {
			__skb_queue_head(&skb_head, skb);
			break;
		}

		if (skb_copy_bits(skb, 0, tx_data, len) < 0) {
			struct net_device_stats *stats = &tp->netdev->stats;

			stats->tx_dropped++;
			dev_kfree_skb_any(skb);
			tx_data -= sizeof(*tx_desc);
			continue;
		}

		tx_data += len;
		agg->skb_len += len;
		agg->skb_num++;

		dev_kfree_skb_any(skb);

		remain = agg_buf_sz - (int)(tx_agg_align(tx_data) - agg->head);
	}

	if (!skb_queue_empty(&skb_head)) {
		spin_lock(&tx_queue->lock);
		skb_queue_splice(&skb_head, tx_queue);
		spin_unlock(&tx_queue->lock);
	}

	netif_tx_lock(tp->netdev);

	if (netif_queue_stopped(tp->netdev) &&
	    skb_queue_len(&tp->tx_queue) < tp->tx_qlen)
		netif_wake_queue(tp->netdev);

	netif_tx_unlock(tp->netdev);

	ret = usb_autopm_get_interface_async(tp->intf);
	if (ret < 0)
		goto out_tx_fill;

	usb_fill_bulk_urb(agg->urb, tp->udev, usb_sndbulkpipe(tp->udev, 2),
			  agg->head, (int)(tx_data - (u8 *)agg->head),
			  (usb_complete_t)write_bulk_callback, agg);

	ret = usb_submit_urb(agg->urb, GFP_ATOMIC);
	if (ret < 0)
		usb_autopm_put_interface_async(tp->intf);

out_tx_fill:
	return ret;
}

static u8 r8152_rx_csum(struct r8152 *tp, struct rx_desc *rx_desc)
{
	u8 checksum = CHECKSUM_NONE;
	u32 opts2, opts3;

	if (tp->version == RTL_VER_01)
		goto return_result;

	opts2 = le32_to_cpu(rx_desc->opts2);
	opts3 = le32_to_cpu(rx_desc->opts3);

	if (opts2 & RD_IPV4_CS) {
		if (opts3 & IPF)
			checksum = CHECKSUM_NONE;
		else if ((opts2 & RD_UDP_CS) && (opts3 & UDPF))
			checksum = CHECKSUM_NONE;
		else if ((opts2 & RD_TCP_CS) && (opts3 & TCPF))
			checksum = CHECKSUM_NONE;
		else
			checksum = CHECKSUM_UNNECESSARY;
	} else if (RD_IPV6_CS) {
		if ((opts2 & RD_UDP_CS) && !(opts3 & UDPF))
			checksum = CHECKSUM_UNNECESSARY;
		else if ((opts2 & RD_TCP_CS) && !(opts3 & TCPF))
			checksum = CHECKSUM_UNNECESSARY;
	}

return_result:
	return checksum;
}

static int rx_bottom(struct r8152 *tp, int budget)
{
	unsigned long flags;
	struct list_head *cursor, *next, rx_queue;
	int ret = 0, work_done = 0;

	if (!skb_queue_empty(&tp->rx_queue)) {
		while (work_done < budget) {
			struct sk_buff *skb = __skb_dequeue(&tp->rx_queue);
			struct net_device *netdev = tp->netdev;
			struct net_device_stats *stats;
			unsigned int pkt_len;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
			u16 vlan_tci;
#endif

			if (!skb)
				break;

			pkt_len = skb->len;
			stats = rtl8152_get_stats(netdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
			vlan_tci = rtl_vlan_get_tag(skb);

			if (vlan_tci)
				vlan_gro_receive(&tp->napi, tp->vlgrp, vlan_tci,
						 skb);
			else
				napi_gro_receive(&tp->napi, skb);
#else
			napi_gro_receive(&tp->napi, skb);
#endif

			work_done++;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
			netdev->last_rx = jiffies;
#endif
			stats->rx_packets++;
			stats->rx_bytes += pkt_len;
		}
	}

	if (list_empty(&tp->rx_done))
		goto out1;

	INIT_LIST_HEAD(&rx_queue);
	spin_lock_irqsave(&tp->rx_lock, flags);
	list_splice_init(&tp->rx_done, &rx_queue);
	spin_unlock_irqrestore(&tp->rx_lock, flags);

	list_for_each_safe(cursor, next, &rx_queue) {
		struct rx_desc *rx_desc;
		struct rx_agg *agg;
		int len_used = 0;
		struct urb *urb;
		u8 *rx_data;

		list_del_init(cursor);

		agg = list_entry(cursor, struct rx_agg, list);
		urb = agg->urb;
		if (urb->actual_length < ETH_ZLEN)
			goto submit;

		rx_desc = agg->head;
		rx_data = agg->head;
		len_used += sizeof(struct rx_desc);

		while (urb->actual_length > len_used) {
			struct net_device *netdev = tp->netdev;
			struct net_device_stats *stats;
			unsigned int pkt_len;
			struct sk_buff *skb;

			pkt_len = le32_to_cpu(rx_desc->opts1) & RX_LEN_MASK;
			if (pkt_len < ETH_ZLEN)
				break;

			len_used += pkt_len;
			if (urb->actual_length < len_used)
				break;

			stats = rtl8152_get_stats(netdev);

			pkt_len -= CRC_SIZE;
			rx_data += sizeof(struct rx_desc);

			skb = napi_alloc_skb(&tp->napi, pkt_len);
			if (!skb) {
				stats->rx_dropped++;
				goto find_next_rx;
			}

			skb->ip_summed = r8152_rx_csum(tp, rx_desc);
			memcpy(skb->data, rx_data, pkt_len);
			skb_put(skb, pkt_len);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
			skb->dev = netdev;
#endif
			skb->protocol = eth_type_trans(skb, netdev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
			if (work_done < budget) {
				if (!rtl_rx_vlan_tag(tp, rx_desc, skb))
					napi_gro_receive(&tp->napi, skb);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
				netdev->last_rx = jiffies;
#endif
				work_done++;
				stats->rx_packets++;
				stats->rx_bytes += pkt_len;
			} else {
				rtl_vlan_put_tag(tp, rx_desc, skb);
				__skb_queue_tail(&tp->rx_queue, skb);
			}
#else
			rtl_rx_vlan_tag(rx_desc, skb);
			if (work_done < budget) {
				napi_gro_receive(&tp->napi, skb);
				work_done++;
				stats->rx_packets++;
				stats->rx_bytes += pkt_len;
			} else {
				__skb_queue_tail(&tp->rx_queue, skb);
			}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0) */

find_next_rx:
			rx_data = rx_agg_align(rx_data + pkt_len + CRC_SIZE);
			rx_desc = (struct rx_desc *)rx_data;
			len_used = (int)(rx_data - (u8 *)agg->head);
			len_used += sizeof(struct rx_desc);
		}

submit:
		if (!ret) {
			ret = r8152_submit_rx(tp, agg, GFP_ATOMIC);
		} else {
			urb->actual_length = 0;
			list_add_tail(&agg->list, next);
		}
	}

	if (!list_empty(&rx_queue)) {
		spin_lock_irqsave(&tp->rx_lock, flags);
		list_splice_tail(&rx_queue, &tp->rx_done);
		spin_unlock_irqrestore(&tp->rx_lock, flags);
	}

out1:
	return work_done;
}

static void tx_bottom(struct r8152 *tp)
{
	int res;

	do {
		struct tx_agg *agg;

		if (skb_queue_empty(&tp->tx_queue))
			break;

		agg = r8152_get_tx_agg(tp);
		if (!agg)
			break;

		res = r8152_tx_agg_fill(tp, agg);
		if (res) {
			struct net_device *netdev = tp->netdev;

			if (res == -ENODEV) {
				set_bit(RTL8152_UNPLUG, &tp->flags);
				netif_device_detach(netdev);
			} else {
				struct net_device_stats *stats;
				unsigned long flags;

				stats = rtl8152_get_stats(netdev);
				netif_warn(tp, tx_err, netdev,
					   "failed tx_urb %d\n", res);
				stats->tx_dropped += agg->skb_num;

				spin_lock_irqsave(&tp->tx_lock, flags);
				list_add_tail(&agg->list, &tp->tx_free);
				spin_unlock_irqrestore(&tp->tx_lock, flags);
			}
		}
	} while (res == 0);
}

static void set_bottom_half_event(struct r8152 *tp, int mode)
{
	if (mode) {
		tp->bottom_half_event = 1;
		wake_up_interruptible(&tp->bottom_half_wait_q);
	} else
		tp->bottom_half_event = 0;
}

static void bottom_half(struct r8152 *tp)
{
	set_bottom_half_event(tp, 0);

	if (test_bit(RTL8152_UNPLUG, &tp->flags)) {
		pr_info("%s : RTL8152_UNPLUG\n", __func__);
		set_bottom_half_event(tp, 1);
		return;
	}

	if (!test_bit(WORK_ENABLE, &tp->flags)) {
		pr_info("%s : WORK_ENABLE\n", __func__);
		set_bottom_half_event(tp, 1);
		return;
	}

	/* When link down, the driver would cancel all bulks. */
	/* This avoid the re-submitting bulk */
	if (!netif_carrier_ok(tp->netdev)) {
		pr_info("%s : netif_carrier_ok is false\n", __func__);
		set_bottom_half_event(tp, 1);
		return;
	}

	clear_bit(SCHEDULE_NAPI, &tp->flags);

	tx_bottom(tp);

	set_bottom_half_event(tp, 1);
}

static inline int __r8152_poll(struct r8152 *tp, int budget)
{
	struct napi_struct *napi = &tp->napi;
	int work_done;

	work_done = rx_bottom(tp, budget);
	bottom_half(tp);

	if (work_done < budget) {
		napi_complete(napi);
		if (!list_empty(&tp->rx_done))
			napi_schedule(napi);
	}

	return work_done;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
static int r8152_poll(struct net_device *dev, int *budget)
{
	struct r8152 *tp = netdev_priv(dev);
	int quota = min(dev->quota, *budget);
	int work_done;

	work_done = __r8152_poll(tp, quota);

	*budget -= work_done;
	dev->quota -= work_done;

	return (work_done >= quota);
}
#else
static int r8152_poll(struct napi_struct *napi, int budget)
{
	struct r8152 *tp = container_of(napi, struct r8152, napi);

	return __r8152_poll(tp, budget);
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24) */

static
int r8152_submit_rx(struct r8152 *tp, struct rx_agg *agg, gfp_t mem_flags)
{
	int ret;

	/* The rx would be stopped, so skip submitting */
	if (test_bit(RTL8152_UNPLUG, &tp->flags) ||
	    !test_bit(WORK_ENABLE, &tp->flags) || !netif_carrier_ok(tp->netdev))
		return 0;

	usb_fill_bulk_urb(agg->urb, tp->udev, usb_rcvbulkpipe(tp->udev, 1),
			  agg->head, agg_buf_sz,
			  (usb_complete_t)read_bulk_callback, agg);

	ret = usb_submit_urb(agg->urb, mem_flags);
	if (ret == -ENODEV) {
		set_bit(RTL8152_UNPLUG, &tp->flags);
		netif_device_detach(tp->netdev);
	} else if (ret) {
		struct urb *urb = agg->urb;
		unsigned long flags;

		urb->actual_length = 0;
		spin_lock_irqsave(&tp->rx_lock, flags);
		list_add_tail(&agg->list, &tp->rx_done);
		spin_unlock_irqrestore(&tp->rx_lock, flags);

		netif_err(tp, rx_err, tp->netdev,
			  "Couldn't submit rx[%p], ret = %d\n", agg, ret);

		napi_schedule(&tp->napi);
	}

	return ret;
}

static void rtl_drop_queued_tx(struct r8152 *tp)
{
	struct net_device_stats *stats = rtl8152_get_stats(tp->netdev);
	struct sk_buff_head skb_head, *tx_queue = &tp->tx_queue;
	struct sk_buff *skb;

	if (skb_queue_empty(tx_queue))
		return;

	__skb_queue_head_init(&skb_head);
	spin_lock_bh(&tx_queue->lock);
	skb_queue_splice_init(tx_queue, &skb_head);
	spin_unlock_bh(&tx_queue->lock);

	while ((skb = __skb_dequeue(&skb_head))) {
		dev_kfree_skb(skb);
		stats->tx_dropped++;
	}
}

static void rtl8152_tx_timeout(struct net_device *netdev)
{
	struct r8152 *tp = netdev_priv(netdev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
	int i;
#endif

	netif_warn(tp, tx_err, netdev, "Tx timeout\n");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
	for (i = 0; i < RTL8152_MAX_TX; i++)
		usb_unlink_urb(tp->tx_info[i].urb);
#else
	usb_queue_reset_device(tp->intf);
#endif
}

static void rtl8152_set_rx_mode(struct net_device *netdev)
{
	struct r8152 *tp = netdev_priv(netdev);

	if (netif_carrier_ok(netdev)) {
		set_bit(RTL8152_SET_RX_MODE, &tp->flags);
		schedule_delayed_work(&tp->schedule, 0);
	}
}

static void _rtl8152_set_rx_mode(struct net_device *netdev)
{
	struct r8152 *tp = netdev_priv(netdev);
	u32 mc_filter[2];	/* Multicast hash filter */
	__le32 tmp[2];
	u32 ocp_data;

	clear_bit(RTL8152_SET_RX_MODE, &tp->flags);

	if (!netif_carrier_ok(netdev))
		return;

	netif_stop_queue(netdev);
	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	ocp_data |= RCR_AB | RCR_APM;

	if (netdev->flags & IFF_PROMISC) {
		/* Unconditionally log net taps. */
		netif_notice(tp, link, netdev, "Promiscuous mode enabled\n");
		ocp_data |= RCR_AM | RCR_AAP;
		mc_filter[1] = 0xffffffff;
		mc_filter[0] = 0xffffffff;
	} else if ((netdev_mc_count(netdev) > multicast_filter_limit) ||
		   (netdev->flags & IFF_ALLMULTI)) {
		/* Too many to filter perfectly -- accept all multicasts. */
		ocp_data |= RCR_AM;
		mc_filter[1] = 0xffffffff;
		mc_filter[0] = 0xffffffff;
	} else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
		struct dev_mc_list *mclist;
		unsigned int i;

		mc_filter[1] = mc_filter[0] = 0;
		for (i = 0, mclist = netdev->mc_list;
		     mclist && i < netdev->mc_count;
		     i++, mclist = mclist->next) {
			int bit_nr;

			bit_nr = ether_crc(ETH_ALEN, mclist->dmi_addr) >> 26;
			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
			ocp_data |= RCR_AM;
		}
#else
		struct netdev_hw_addr *ha;

		mc_filter[1] = 0;
		mc_filter[0] = 0;
		netdev_for_each_mc_addr(ha, netdev) {
			int bit_nr = ether_crc(ETH_ALEN, ha->addr) >> 26;

			mc_filter[bit_nr >> 5] |= 1 << (bit_nr & 31);
			ocp_data |= RCR_AM;
		}
#endif
	}

	tmp[0] = __cpu_to_le32(swab32(mc_filter[1]));
	tmp[1] = __cpu_to_le32(swab32(mc_filter[0]));

	pla_ocp_write(tp, PLA_MAR, BYTE_EN_DWORD, sizeof(tmp), tmp);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);
	netif_wake_queue(netdev);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 4)
static netdev_features_t
rtl8152_features_check(struct sk_buff *skb, struct net_device *dev,
		       netdev_features_t features)
{
	u32 mss = skb_shinfo(skb)->gso_size;
	int max_offset = mss ? GTTCPHO_MAX : TCPHO_MAX;
	int offset = skb_transport_offset(skb);

	if ((mss || skb->ip_summed == CHECKSUM_PARTIAL) && offset > max_offset)
		features &= ~(NETIF_F_CSUM_MASK | NETIF_F_GSO_MASK);
	else if ((skb->len + sizeof(struct tx_desc)) > agg_buf_sz)
		features &= ~NETIF_F_GSO_MASK;

	return features;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,4) */

static netdev_tx_t rtl8152_start_xmit(struct sk_buff *skb,
				      struct net_device *netdev)
{
	struct r8152 *tp = netdev_priv(netdev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 18, 4)
	if (unlikely((skb->len + sizeof(struct tx_desc)) > agg_buf_sz)) {
		netdev_features_t features = netdev->features;
		struct sk_buff *segs, *nskb;

		features &= ~(NETIF_F_TSO | NETIF_F_TSO6);
		segs = skb_gso_segment(skb, features);
		if (IS_ERR(segs) || !segs)
			goto free_skb;

		do {
			nskb = segs;
			segs = segs->next;
			nskb->next = NULL;
			rtl8152_start_xmit(nskb, netdev);
		} while (segs);

free_skb:
		dev_kfree_skb_any(skb);

		return NETDEV_TX_OK;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 31)
	netdev->trans_start = jiffies
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31) */
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3,18,4) */

	skb_tx_timestamp(skb);

	skb_queue_tail(&tp->tx_queue, skb);

	if (!list_empty(&tp->tx_free)) {
		if (test_bit(SELECTIVE_SUSPEND, &tp->flags)) {
			set_bit(SCHEDULE_NAPI, &tp->flags);
			schedule_delayed_work(&tp->schedule, 0);
		} else {
			usb_mark_last_busy(tp->udev);
			napi_schedule(&tp->napi);
		}
	} else if (skb_queue_len(&tp->tx_queue) > tp->tx_qlen) {
		netif_stop_queue(netdev);
	}

	return NETDEV_TX_OK;
}

static void r8152b_reset_packet_filter(struct r8152 *tp)
{
	u32	ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_FMC);
	ocp_data &= ~FMC_FCR_MCU_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_FMC, ocp_data);
	ocp_data |= FMC_FCR_MCU_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_FMC, ocp_data);
}

static void rtl8152_nic_reset(struct r8152 *tp)
{
	int	i;

	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CR, CR_RST);

	for (i = 0; i < 1000; i++) {
		if (!(ocp_read_byte(tp, MCU_TYPE_PLA, PLA_CR) & CR_RST))
			break;
		usleep_range(100, 400);
	}
}

static void set_tx_qlen(struct r8152 *tp)
{
	struct net_device *netdev = tp->netdev;

	tp->tx_qlen = agg_buf_sz / (netdev->mtu + VLAN_ETH_HLEN + CRC_SIZE +
				    sizeof(struct tx_desc));
}

static inline u8 rtl8152_get_speed(struct r8152 *tp)
{
	return ocp_read_byte(tp, MCU_TYPE_PLA, PLA_PHYSTATUS);
}

static void rtl_set_eee_plus(struct r8152 *tp)
{
	u32 ocp_data;
	u8 speed;

	speed = rtl8152_get_speed(tp);
	if (speed & _10bps) {
		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEEP_CR);
		ocp_data |= EEEP_CR_EEEP_TX;
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_EEEP_CR, ocp_data);
	} else {
		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEEP_CR);
		ocp_data &= ~EEEP_CR_EEEP_TX;
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_EEEP_CR, ocp_data);
	}
}

static void rxdy_gated_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_MISC_1);
	if (enable)
		ocp_data |= RXDY_GATED_EN;
	else
		ocp_data &= ~RXDY_GATED_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_MISC_1, ocp_data);
}

#if defined(RTL8152_S5_WOL) && defined(CONFIG_PM)
static int rtl_s5_wol(struct r8152 *tp)
{
	struct usb_device *udev = tp->udev;

	if (!tp->saved_wolopts)
		return 0;

	/* usb_enable_remote_wakeup */
	if (udev->speed < USB_SPEED_SUPER)
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_DEVICE,
				USB_DEVICE_REMOTE_WAKEUP, 0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	else
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_INTERFACE,
				USB_INTRF_FUNC_SUSPEND,
				USB_INTRF_FUNC_SUSPEND_RW |
					USB_INTRF_FUNC_SUSPEND_LP,
				NULL, 0, USB_CTRL_SET_TIMEOUT);
}
#endif

static int rtl_start_rx(struct r8152 *tp)
{
	int i, ret = 0;

	INIT_LIST_HEAD(&tp->rx_done);
	for (i = 0; i < RTL8152_MAX_RX; i++) {
		INIT_LIST_HEAD(&tp->rx_info[i].list);
		ret = r8152_submit_rx(tp, &tp->rx_info[i], GFP_KERNEL);
		if (ret)
			break;
	}

	if (ret && ++i < RTL8152_MAX_RX) {
		struct list_head rx_queue;
		unsigned long flags;

		INIT_LIST_HEAD(&rx_queue);

		do {
			struct rx_agg *agg = &tp->rx_info[i++];
			struct urb *urb = agg->urb;

			urb->actual_length = 0;
			list_add_tail(&agg->list, &rx_queue);
		} while (i < RTL8152_MAX_RX);

		spin_lock_irqsave(&tp->rx_lock, flags);
		list_splice_tail(&rx_queue, &tp->rx_done);
		spin_unlock_irqrestore(&tp->rx_lock, flags);
	}

	return ret;
}

static int rtl_stop_rx(struct r8152 *tp)
{
	int i;

	for (i = 0; i < RTL8152_MAX_RX; i++)
		usb_kill_urb(tp->rx_info[i].urb);

	while (!skb_queue_empty(&tp->rx_queue))
		dev_kfree_skb(__skb_dequeue(&tp->rx_queue));

	return 0;
}

static int rtl_enable(struct r8152 *tp)
{
	u32 ocp_data;

	r8152b_reset_packet_filter(tp);

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_CR);
	ocp_data |= CR_RE | CR_TE;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CR, ocp_data);

	rxdy_gated_en(tp, false);

	return 0;
}

static int rtl8152_enable(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return -ENODEV;

	set_tx_qlen(tp);
	rtl_set_eee_plus(tp);

	return rtl_enable(tp);
}

static inline void r8153b_rx_agg_chg_indicate(struct r8152 *tp)
{
	ocp_write_byte(tp, MCU_TYPE_USB, USB_UPT_RXDMA_OWN,
		       OWN_UPDATE | OWN_CLEAR);
}

static void r8153_set_rx_early_timeout(struct r8152 *tp)
{
	u32 ocp_data = tp->coalesce / 8;

	switch (tp->version) {
	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
		ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_TIMEOUT,
			       ocp_data);
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		/* The RTL8153B uses USB_RX_EXTRA_AGGR_TMR for rx timeout
		 * primarily. For USB_RX_EARLY_TIMEOUT, we fix it to 128ns.
		 */
		ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_TIMEOUT,
			       128 / 8);
		ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EXTRA_AGGR_TMR,
			       ocp_data);
		r8153b_rx_agg_chg_indicate(tp);
		break;

	default:
		break;
	}
}

static void r8153_set_rx_early_size(struct r8152 *tp)
{
	u32 mtu = tp->netdev->mtu;
	u32 ocp_data = (agg_buf_sz - rx_reserved_size(mtu)) / 8;

	ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_SIZE, ocp_data);

	switch (tp->version) {
	case RTL_VER_08:
	case RTL_VER_09:
		r8153b_rx_agg_chg_indicate(tp);
		break;
	default:
		break;
	}
}

static int rtl8153_enable(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return -ENODEV;

	usb_disable_lpm(tp->udev);
	set_tx_qlen(tp);
	rtl_set_eee_plus(tp);
	r8153_set_rx_early_timeout(tp);
	r8153_set_rx_early_size(tp);

	return rtl_enable(tp);
}

static void rtl_disable(struct r8152 *tp)
{
	u32 ocp_data;
	int i;

	if (test_bit(RTL8152_UNPLUG, &tp->flags)) {
		rtl_drop_queued_tx(tp);
		return;
	}

	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);

	rtl_drop_queued_tx(tp);

	for (i = 0; i < RTL8152_MAX_TX; i++)
		usb_kill_urb(tp->tx_info[i].urb);

	rxdy_gated_en(tp, true);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if ((ocp_data & FIFO_EMPTY) == FIFO_EMPTY)
			break;
		usleep_range(1000, 2000);
	}

	for (i = 0; i < 1000; i++) {
		if (ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR0) & TCR0_TX_EMPTY)
			break;
		usleep_range(1000, 2000);
	}

	rtl_stop_rx(tp);

	rtl8152_nic_reset(tp);
}

static void r8152_power_cut_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_UPS_CTRL);
	if (enable)
		ocp_data |= POWER_CUT;
	else
		ocp_data &= ~POWER_CUT;
	ocp_write_word(tp, MCU_TYPE_USB, USB_UPS_CTRL, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_PM_CTRL_STATUS);
	ocp_data &= ~RESUME_INDICATE;
	ocp_write_word(tp, MCU_TYPE_USB, USB_PM_CTRL_STATUS, ocp_data);
}

static void rtl_rx_vlan_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CPCR);
	if (enable)
		ocp_data |= CPCR_RX_VLAN;
	else
		ocp_data &= ~CPCR_RX_VLAN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_CPCR, ocp_data);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)

static void
rtl8152_vlan_rx_register(struct net_device *dev, struct vlan_group *grp)
{
	struct r8152 *tp = netdev_priv(dev);

	if (unlikely(tp->rtk_enable_diag))
		return;

	if (usb_autopm_get_interface(tp->intf) < 0)
		return;

	mutex_lock(&tp->control);

	tp->vlgrp = grp;
	if (tp->vlgrp)
		rtl_rx_vlan_en(tp, true);
	else
		rtl_rx_vlan_en(tp, false);

	mutex_unlock(&tp->control);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)

static void rtl8152_vlan_rx_kill_vid(struct net_device *dev, unsigned short vid)
{
	struct r8152 *tp = netdev_priv(dev);

	vlan_group_set_device(tp->vlgrp, vid, NULL);
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22) */

#else

static int rtl8152_set_features(struct net_device *dev,
				netdev_features_t features)
{
	netdev_features_t changed = features ^ dev->features;
	struct r8152 *tp = netdev_priv(dev);
	int ret;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	mutex_lock(&tp->control);

	if (changed & NETIF_F_HW_VLAN_CTAG_RX) {
		if (features & NETIF_F_HW_VLAN_CTAG_RX)
			rtl_rx_vlan_en(tp, true);
		else
			rtl_rx_vlan_en(tp, false);
	}

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0) */

#define WAKE_ANY (WAKE_PHY | WAKE_MAGIC | WAKE_UCAST | WAKE_BCAST | WAKE_MCAST)

static u32 __rtl_get_wol(struct r8152 *tp)
{
	u32 ocp_data;
	u32 wolopts = 0;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CONFIG34);
	if (ocp_data & LINK_ON_WAKE_EN)
		wolopts |= WAKE_PHY;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CONFIG5);
	if (ocp_data & UWF_EN)
		wolopts |= WAKE_UCAST;
	if (ocp_data & BWF_EN)
		wolopts |= WAKE_BCAST;
	if (ocp_data & MWF_EN)
		wolopts |= WAKE_MCAST;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CFG_WOL);
	if (ocp_data & MAGIC_EN)
		wolopts |= WAKE_MAGIC;

	return wolopts;
}

static void __rtl_set_wol(struct r8152 *tp, u32 wolopts)
{
	u32 ocp_data;

	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_CONFIG);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CONFIG34);
	ocp_data &= ~LINK_ON_WAKE_EN;
	if (wolopts & WAKE_PHY)
		ocp_data |= LINK_ON_WAKE_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_CONFIG34, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CONFIG5);
	ocp_data &= ~(UWF_EN | BWF_EN | MWF_EN);
	if (wolopts & WAKE_UCAST)
		ocp_data |= UWF_EN;
	if (wolopts & WAKE_BCAST)
		ocp_data |= BWF_EN;
	if (wolopts & WAKE_MCAST)
		ocp_data |= MWF_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_CONFIG5, ocp_data);

	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CFG_WOL);
	ocp_data &= ~MAGIC_EN;
	if (wolopts & WAKE_MAGIC)
		ocp_data |= MAGIC_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_CFG_WOL, ocp_data);

	if (wolopts & WAKE_ANY)
		device_set_wakeup_enable(&tp->udev->dev, true);
	else
		device_set_wakeup_enable(&tp->udev->dev, false);
}

static void r8153_u1u2en(struct r8152 *tp, bool enable)
{
	u8 u1u2[8];

	if (enable)
		memset(u1u2, 0xff, sizeof(u1u2));
	else
		memset(u1u2, 0x00, sizeof(u1u2));

	usb_ocp_write(tp, USB_TOLERANCE, BYTE_EN_SIX_BYTES, sizeof(u1u2), u1u2);
}

static void r8153b_u1u2en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_LPM_CONFIG);
	if (enable)
		ocp_data |= LPM_U1U2_EN;
	else
		ocp_data &= ~LPM_U1U2_EN;

	ocp_write_word(tp, MCU_TYPE_USB, USB_LPM_CONFIG, ocp_data);
}

static void r8153_u2p3en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_U2P3_CTRL);
	if (enable && tp->version != RTL_VER_03 && tp->version != RTL_VER_04)
		ocp_data |= U2P3_ENABLE;
	else
		ocp_data &= ~U2P3_ENABLE;
	ocp_write_word(tp, MCU_TYPE_USB, USB_U2P3_CTRL, ocp_data);
}

static void r8153b_ups_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, USB_POWER_CUT);
	if (enable) {
		ocp_data |= UPS_EN | PHASE2_EN;
		ocp_write_byte(tp, MCU_TYPE_USB, USB_POWER_CUT, ocp_data);

		ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, 0xcfff);
		ocp_data |= BIT(0);
		ocp_write_byte(tp, MCU_TYPE_USB, 0xcfff, ocp_data);
	} else {
		ocp_data &= ~UPS_EN;
		ocp_write_byte(tp, MCU_TYPE_USB, USB_POWER_CUT, ocp_data);

		ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, 0xcfff);
		ocp_data &= ~BIT(0);
		ocp_write_byte(tp, MCU_TYPE_USB, 0xcfff, ocp_data);

		ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_MISC_0);
		ocp_data &= ~PCUT_STATUS;
		ocp_write_word(tp, MCU_TYPE_USB, USB_MISC_0, ocp_data);
	}
}

static void r8153_power_cut_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_POWER_CUT);
	if (enable)
		ocp_data |= PWR_EN | PHASE2_EN;
	else
		ocp_data &= ~(PWR_EN | PHASE2_EN);
	ocp_write_word(tp, MCU_TYPE_USB, USB_POWER_CUT, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_MISC_0);
	ocp_data &= ~PCUT_STATUS;
	ocp_write_word(tp, MCU_TYPE_USB, USB_MISC_0, ocp_data);
}

static void r8153B_power_cut_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_POWER_CUT);
	if (enable)
		ocp_data |= PWR_EN | PHASE2_EN;
	else
		ocp_data &= ~PWR_EN;
	ocp_write_word(tp, MCU_TYPE_USB, USB_POWER_CUT, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_MISC_0);
	ocp_data &= ~PCUT_STATUS;
	ocp_write_word(tp, MCU_TYPE_USB, USB_MISC_0, ocp_data);
}

static void r8153b_queue_wake(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, 0xd38a);
	if (enable)
		ocp_data |= BIT(0);
	else
		ocp_data &= ~BIT(0);
	ocp_write_byte(tp, MCU_TYPE_PLA, 0xd38a, ocp_data);

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, 0xd38c);
	ocp_data &= ~BIT(0);
	ocp_write_byte(tp, MCU_TYPE_PLA, 0xd38c, ocp_data);
}

static bool rtl_can_wakeup(struct r8152 *tp)
{
	struct usb_device *udev = tp->udev;

	return (udev->actconfig->desc.bmAttributes & USB_CONFIG_ATT_WAKEUP);
}

static void rtl_runtime_suspend_enable(struct r8152 *tp, bool enable)
{
	if (enable) {
		u32 ocp_data;

		__rtl_set_wol(tp, WAKE_ANY);

		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_CONFIG);

		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CONFIG34);
		ocp_data |= LINK_OFF_WAKE_EN;
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_CONFIG34, ocp_data);

		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);
	} else {
		u32 ocp_data;

		__rtl_set_wol(tp, tp->saved_wolopts);

		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_CONFIG);

		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_CONFIG34);
		ocp_data &= ~LINK_OFF_WAKE_EN;
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_CONFIG34, ocp_data);

		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);
	}
}

static void rtl8153_runtime_enable(struct r8152 *tp, bool enable)
{
	rtl_runtime_suspend_enable(tp, enable);
	tp->rtl_ops.u1u2_enable(tp, !enable);
	tp->rtl_ops.u2p3_enable(tp, !enable);
}

static void rtl8153b_runtime_enable(struct r8152 *tp, bool enable)
{
	r8153b_queue_wake(tp, enable);
	rtl_runtime_suspend_enable(tp, enable);
	tp->rtl_ops.u1u2_enable(tp, !enable);
	r8153b_ups_en(tp, enable);
}

#if !defined(CONFIG_MII) && !defined(CONFIG_MII_MODULE)
int mii_nway_restart (struct mii_if_info *mii)
{
	struct r8152 *tp = netdev_priv(mii->dev);
	int r = -EINVAL;
	int bmcr;

	/* if autoneg is off, it's an error */
	bmcr = r8152_mdio_read(tp, MII_BMCR);

	if (bmcr & BMCR_ANENABLE) {
		bmcr |= BMCR_ANRESTART;
		r8152_mdio_write(tp, MII_BMCR, bmcr);
		r = 0;
	}

	return r;
}
#endif

static void r8153_teredo_off(struct r8152 *tp)
{
	u32 ocp_data;

	switch (tp->version) {
	case RTL_VER_01:
	case RTL_VER_02:
	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
	case RTL_VER_07:
		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_TEREDO_CFG);
		ocp_data &= ~(TEREDO_SEL | TEREDO_RS_EVENT_MASK |
			      OOB_TEREDO_EN);
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_TEREDO_CFG, ocp_data);
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		/* The bit 0 ~ 7 are relative with teredo settings. They are
		 * W1C (write 1 to clear), so set all 1 to disable it.
		 */
		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_TEREDO_CFG, 0xff);
		break;

	default:
		break;
	}

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_WDT6_CTRL, WDT6_SET_MODE);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_REALWOW_TIMER, 0);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_TEREDO_TIMER, 0);
}

static void rtl_reset_bmu(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, USB_BMU_RESET);
	ocp_data &= ~(BMU_RESET_EP_IN | BMU_RESET_EP_OUT);
	ocp_write_byte(tp, MCU_TYPE_USB, USB_BMU_RESET, ocp_data);
	ocp_data |= BMU_RESET_EP_IN | BMU_RESET_EP_OUT;
	ocp_write_byte(tp, MCU_TYPE_USB, USB_BMU_RESET, ocp_data);
}

static void rtl_clear_bp(struct r8152 *tp)
{
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_BP_0, 0);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_BP_2, 0);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_BP_4, 0);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_BP_6, 0);
	ocp_write_dword(tp, MCU_TYPE_USB, USB_BP_0, 0);
	ocp_write_dword(tp, MCU_TYPE_USB, USB_BP_2, 0);
	ocp_write_dword(tp, MCU_TYPE_USB, USB_BP_4, 0);
	ocp_write_dword(tp, MCU_TYPE_USB, USB_BP_6, 0);
	usleep_range(3000, 6000);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_BP_BA, 0);
	ocp_write_word(tp, MCU_TYPE_USB, USB_BP_BA, 0);
}

static void r8153_clear_bp(struct r8152 *tp)
{
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_BP_EN, 0);
	ocp_write_byte(tp, MCU_TYPE_USB, USB_BP_EN, 0);
	rtl_clear_bp(tp);
}

/*
static void r8153b_clear_bp(struct r8152 *tp, u16 type)
{
	if (type == MCU_TYPE_PLA)
		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_BP_EN, 0);
	else
		ocp_write_byte(tp, MCU_TYPE_USB, USB_BP2_EN, 0);

	ocp_write_word(tp, type, PLA_BP_0, 0);
	ocp_write_word(tp, type, PLA_BP_1, 0);
	ocp_write_word(tp, type, PLA_BP_2, 0);
	ocp_write_word(tp, type, PLA_BP_3, 0);
	ocp_write_word(tp, type, PLA_BP_4, 0);
	ocp_write_word(tp, type, PLA_BP_5, 0);
	ocp_write_word(tp, type, PLA_BP_6, 0);
	ocp_write_word(tp, type, PLA_BP_7, 0);

	if (type == MCU_TYPE_USB) {
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_8, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_9, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_10, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_11, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_12, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_13, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_14, 0);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_15, 0);
	}
	usleep_range(3000, 6000);
	ocp_write_word(tp, type, PLA_BP_BA, 0);
}
*/

static void patch4(struct r8152 *tp)
{
	u8 data;

	data = ocp_read_byte(tp, MCU_TYPE_USB, 0xd429);
	data |= 0x80;
	ocp_write_byte(tp, MCU_TYPE_USB, 0xd429, data);
	ocp_write_word(tp, MCU_TYPE_USB, 0xc0ce, 0x0210);
	data = ocp_read_byte(tp, MCU_TYPE_USB, 0xd429);
	data &= ~0x80;
	ocp_write_byte(tp, MCU_TYPE_USB, 0xd429, data);
}

static void r8152b_firmware(struct r8152 *tp)
{
	if (tp->version == RTL_VER_01) {
		int i;
		static u8 pla_patch_a[] = {
			0x08, 0xe0, 0x40, 0xe0,
			0x78, 0xe0, 0x85, 0xe0,
			0x5d, 0xe1, 0xa1, 0xe1,
			0xa3, 0xe1, 0xab, 0xe1,
			0x31, 0xc3, 0x60, 0x72,
			0xa0, 0x49, 0x10, 0xf0,
			0xa4, 0x49, 0x0e, 0xf0,
			0x2c, 0xc3, 0x62, 0x72,
			0x26, 0x70, 0x80, 0x49,
			0x05, 0xf0, 0x2f, 0x48,
			0x62, 0x9a, 0x24, 0x70,
			0x60, 0x98, 0x24, 0xc3,
			0x60, 0x99, 0x23, 0xc3,
			0x00, 0xbb, 0x2c, 0x75,
			0xdc, 0x21, 0xbc, 0x25,
			0x04, 0x13, 0x0a, 0xf0,
			0x03, 0x13, 0x08, 0xf0,
			0x02, 0x13, 0x06, 0xf0,
			0x01, 0x13, 0x04, 0xf0,
			0x08, 0x13, 0x02, 0xf0,
			0x03, 0xe0, 0xd4, 0x49,
			0x04, 0xf1, 0x14, 0xc2,
			0x12, 0xc3, 0x00, 0xbb,
			0x12, 0xc3, 0x60, 0x75,
			0xd0, 0x49, 0x05, 0xf1,
			0x50, 0x48, 0x60, 0x9d,
			0x09, 0xc6, 0x00, 0xbe,
			0xd0, 0x48, 0x60, 0x9d,
			0xf3, 0xe7, 0xc2, 0xc0,
			0x38, 0xd2, 0xc6, 0xd2,
			0x84, 0x17, 0xa2, 0x13,
			0x0c, 0x17, 0xbc, 0xc0,
			0xa2, 0xd1, 0x33, 0xc5,
			0xa0, 0x74, 0xc0, 0x49,
			0x1f, 0xf0, 0x30, 0xc5,
			0xa0, 0x73, 0x00, 0x13,
			0x04, 0xf1, 0xa2, 0x73,
			0x00, 0x13, 0x14, 0xf0,
			0x28, 0xc5, 0xa0, 0x74,
			0xc8, 0x49, 0x1b, 0xf1,
			0x26, 0xc5, 0xa0, 0x76,
			0xa2, 0x74, 0x01, 0x06,
			0x20, 0x37, 0xa0, 0x9e,
			0xa2, 0x9c, 0x1e, 0xc5,
			0xa2, 0x73, 0x23, 0x40,
			0x10, 0xf8, 0x04, 0xf3,
			0xa0, 0x73, 0x33, 0x40,
			0x0c, 0xf8, 0x15, 0xc5,
			0xa0, 0x74, 0x41, 0x48,
			0xa0, 0x9c, 0x14, 0xc5,
			0xa0, 0x76, 0x62, 0x48,
			0xe0, 0x48, 0xa0, 0x9e,
			0x10, 0xc6, 0x00, 0xbe,
			0x0a, 0xc5, 0xa0, 0x74,
			0x48, 0x48, 0xa0, 0x9c,
			0x0b, 0xc5, 0x20, 0x1e,
			0xa0, 0x9e, 0xe5, 0x48,
			0xa0, 0x9e, 0xf0, 0xe7,
			0xbc, 0xc0, 0xc8, 0xd2,
			0xcc, 0xd2, 0x28, 0xe4,
			0x22, 0x02, 0xf0, 0xc0,
			0x0b, 0xc0, 0x00, 0x71,
			0x0a, 0xc0, 0x00, 0x72,
			0xa0, 0x49, 0x04, 0xf0,
			0xa4, 0x49, 0x02, 0xf0,
			0x93, 0x48, 0x04, 0xc0,
			0x00, 0xb8, 0x00, 0xe4,
			0xc2, 0xc0, 0x8c, 0x09,
			0x14, 0xc2, 0x40, 0x73,
			0xba, 0x48, 0x40, 0x9b,
			0x11, 0xc2, 0x40, 0x73,
			0xb0, 0x49, 0x17, 0xf0,
			0xbf, 0x49, 0x03, 0xf1,
			0x09, 0xc5, 0x00, 0xbd,
			0xb1, 0x49, 0x11, 0xf0,
			0xb1, 0x48, 0x40, 0x9b,
			0x02, 0xc2, 0x00, 0xba,
			0x82, 0x18, 0x00, 0xa0,
			0x1e, 0xfc, 0xbc, 0xc0,
			0xf0, 0xc0, 0xde, 0xe8,
			0x00, 0x80, 0x00, 0x60,
			0x2c, 0x75, 0xd4, 0x49,
			0x12, 0xf1, 0x29, 0xe0,
			0xf8, 0xc2, 0x46, 0x71,
			0xf7, 0xc2, 0x40, 0x73,
			0xbe, 0x49, 0x03, 0xf1,
			0xf5, 0xc7, 0x02, 0xe0,
			0xf2, 0xc7, 0x4f, 0x30,
			0x26, 0x62, 0xa1, 0x49,
			0xf0, 0xf1, 0x22, 0x72,
			0xa0, 0x49, 0xed, 0xf1,
			0x25, 0x25, 0x18, 0x1f,
			0x97, 0x30, 0x91, 0x30,
			0x36, 0x9a, 0x2c, 0x75,
			0x32, 0xc3, 0x60, 0x73,
			0xb1, 0x49, 0x0d, 0xf1,
			0xdc, 0x21, 0xbc, 0x25,
			0x27, 0xc6, 0xc0, 0x77,
			0x04, 0x13, 0x18, 0xf0,
			0x03, 0x13, 0x19, 0xf0,
			0x02, 0x13, 0x1a, 0xf0,
			0x01, 0x13, 0x1b, 0xf0,
			0xd4, 0x49, 0x03, 0xf1,
			0x1c, 0xc5, 0x00, 0xbd,
			0xcd, 0xc6, 0xc6, 0x67,
			0x2e, 0x75, 0xd7, 0x22,
			0xdd, 0x26, 0x05, 0x15,
			0x1a, 0xf0, 0x14, 0xc6,
			0x00, 0xbe, 0x13, 0xc5,
			0x00, 0xbd, 0x12, 0xc5,
			0x00, 0xbd, 0xf1, 0x49,
			0xfb, 0xf1, 0xef, 0xe7,
			0xf4, 0x49, 0xfa, 0xf1,
			0xec, 0xe7, 0xf3, 0x49,
			0xf7, 0xf1, 0xe9, 0xe7,
			0xf2, 0x49, 0xf4, 0xf1,
			0xe6, 0xe7, 0xb6, 0xc0,
			0x6a, 0x14, 0xac, 0x13,
			0xd6, 0x13, 0xfa, 0x14,
			0xa0, 0xd1, 0x00, 0x00,
			0xc0, 0x75, 0xd0, 0x49,
			0x46, 0xf0, 0x26, 0x72,
			0xa7, 0x49, 0x43, 0xf0,
			0x22, 0x72, 0x25, 0x25,
			0x20, 0x1f, 0x97, 0x30,
			0x91, 0x30, 0x40, 0x73,
			0xf3, 0xc4, 0x1c, 0x40,
			0x04, 0xf0, 0xd7, 0x49,
			0x05, 0xf1, 0x37, 0xe0,
			0x53, 0x48, 0xc0, 0x9d,
			0x08, 0x02, 0x40, 0x66,
			0x64, 0x27, 0x06, 0x16,
			0x30, 0xf1, 0x46, 0x63,
			0x3b, 0x13, 0x2d, 0xf1,
			0x34, 0x9b, 0x18, 0x1b,
			0x93, 0x30, 0x2b, 0xc3,
			0x10, 0x1c, 0x2b, 0xe8,
			0x01, 0x14, 0x25, 0xf1,
			0x00, 0x1d, 0x26, 0x1a,
			0x8a, 0x30, 0x22, 0x73,
			0xb5, 0x25, 0x0e, 0x0b,
			0x00, 0x1c, 0x2c, 0xe8,
			0x1f, 0xc7, 0x27, 0x40,
			0x1a, 0xf1, 0x38, 0xe8,
			0x32, 0x1f, 0x8f, 0x30,
			0x08, 0x1b, 0x24, 0xe8,
			0x36, 0x72, 0x46, 0x77,
			0x00, 0x17, 0x0d, 0xf0,
			0x13, 0xc3, 0x1f, 0x40,
			0x03, 0xf1, 0x00, 0x1f,
			0x46, 0x9f, 0x44, 0x77,
			0x9f, 0x44, 0x5f, 0x44,
			0x17, 0xe8, 0x0a, 0xc7,
			0x27, 0x40, 0x05, 0xf1,
			0x02, 0xc3, 0x00, 0xbb,
			0x50, 0x1a, 0x06, 0x1a,
			0xff, 0xc7, 0x00, 0xbf,
			0xb8, 0xcd, 0xff, 0xff,
			0x02, 0x0c, 0x54, 0xa5,
			0xdc, 0xa5, 0x2f, 0x40,
			0x05, 0xf1, 0x00, 0x14,
			0xfa, 0xf1, 0x01, 0x1c,
			0x02, 0xe0, 0x00, 0x1c,
			0x80, 0xff, 0xb0, 0x49,
			0x04, 0xf0, 0x01, 0x0b,
			0xd3, 0xa1, 0x03, 0xe0,
			0x02, 0x0b, 0xd3, 0xa5,
			0x27, 0x31, 0x20, 0x37,
			0x02, 0x0b, 0xd3, 0xa5,
			0x27, 0x31, 0x20, 0x37,
			0x00, 0x13, 0xfb, 0xf1,
			0x80, 0xff, 0x22, 0x73,
			0xb5, 0x25, 0x18, 0x1e,
			0xde, 0x30, 0xd9, 0x30,
			0x64, 0x72, 0x11, 0x1e,
			0x68, 0x23, 0x16, 0x31,
			0x80, 0xff, 0xd4, 0x49,
			0x28, 0xf0, 0x02, 0xb4,
			0x2a, 0xc4, 0x00, 0x1d,
			0x2e, 0xe8, 0xe0, 0x73,
			0xb9, 0x21, 0xbd, 0x25,
			0x04, 0x13, 0x02, 0xf0,
			0x1a, 0xe0, 0x22, 0xc4,
			0x23, 0xc3, 0x2f, 0xe8,
			0x23, 0xc3, 0x2d, 0xe8,
			0x00, 0x1d, 0x21, 0xe8,
			0xe2, 0x73, 0xbb, 0x49,
			0xfc, 0xf0, 0xe0, 0x73,
			0xb7, 0x48, 0x03, 0xb4,
			0x81, 0x1d, 0x19, 0xe8,
			0x40, 0x1a, 0x84, 0x1d,
			0x16, 0xe8, 0x12, 0xc3,
			0x1e, 0xe8, 0x03, 0xb0,
			0x81, 0x1d, 0x11, 0xe8,
			0x0e, 0xc3, 0x19, 0xe8,
			0x02, 0xb0, 0x06, 0xc7,
			0x04, 0x1e, 0xe0, 0x9e,
			0x02, 0xc6, 0x00, 0xbe,
			0x22, 0x02, 0x20, 0xe4,
			0x04, 0xb8, 0x34, 0xb0,
			0x00, 0x02, 0x00, 0x03,
			0x00, 0x0e, 0x00, 0x0c,
			0x09, 0xc7, 0xe0, 0x9b,
			0xe2, 0x9a, 0xe4, 0x9c,
			0xe6, 0x8d, 0xe6, 0x76,
			0xef, 0x49, 0xfe, 0xf1,
			0x80, 0xff, 0x08, 0xea,
			0x82, 0x1d, 0xf5, 0xef,
			0x00, 0x1a, 0x88, 0x1d,
			0xf2, 0xef, 0xed, 0xc2,
			0xf0, 0xef, 0x80, 0xff,
			0x02, 0xc6, 0x00, 0xbe,
			0x46, 0x06, 0x08, 0xc2,
			0x40, 0x73, 0x3a, 0x48,
			0x40, 0x9b, 0x06, 0xff,
			0x02, 0xc6, 0x00, 0xbe,
			0x86, 0x17, 0x1e, 0xfc,
			0x36, 0xf0, 0x08, 0x1c,
			0xea, 0x8c, 0xe3, 0x64,
			0xc7, 0x49, 0x25, 0xf1,
			0xe0, 0x75, 0xff, 0x1b,
			0xeb, 0x47, 0xff, 0x1b,
			0x6b, 0x47, 0xe0, 0x9d,
			0x15, 0xc3, 0x60, 0x75,
			0xd8, 0x49, 0x04, 0xf0,
			0x81, 0x1d, 0xe2, 0x8d,
			0x05, 0xe0, 0xe2, 0x63,
			0x81, 0x1d, 0xdd, 0x47,
			0xe2, 0x8b, 0x0b, 0xc3,
			0x00, 0x1d, 0x61, 0x8d,
			0x3c, 0x03, 0x60, 0x75,
			0xd8, 0x49, 0x06, 0xf1,
			0xdf, 0x48, 0x61, 0x95,
			0x16, 0xe0, 0x4e, 0xe8,
			0x12, 0xe8, 0x21, 0xc5,
			0xa0, 0x73, 0xb0, 0x49,
			0x03, 0xf0, 0x31, 0x48,
			0xa0, 0x9b, 0x0d, 0xe0,
			0xc0, 0x49, 0x0b, 0xf1,
			0xe2, 0x63, 0x7e, 0x1d,
			0xdd, 0x46, 0xe2, 0x8b,
			0xe0, 0x75, 0x83, 0x1b,
			0xeb, 0x46, 0xfe, 0x1b,
			0x6b, 0x46, 0xe0, 0x9d,
			0xe4, 0x49, 0x11, 0xf0,
			0x10, 0x1d, 0xea, 0x8d,
			0xe3, 0x64, 0xc6, 0x49,
			0x09, 0xf1, 0x07, 0xc5,
			0xa0, 0x73, 0xb1, 0x48,
			0xa0, 0x9b, 0x02, 0xc5,
			0x00, 0xbd, 0xe6, 0x04,
			0xa0, 0xd1, 0x02, 0xc5,
			0x00, 0xbd, 0xfe, 0x04,
			0x02, 0xc5, 0x00, 0xbd,
			0x30, 0x05, 0x00, 0x00 };
		static u16 ram_code1[] = {
			0x9700, 0x7fe0, 0x4c00, 0x4007,
			0x4400, 0x4800, 0x7c1f, 0x4c00,
			0x5310, 0x6000, 0x7c07, 0x6800,
			0x673e, 0x0000, 0x0000, 0x571f,
			0x5ffb, 0xaa05, 0x5b58, 0x7d80,
			0x6100, 0x3019, 0x5b64, 0x7d80,
			0x6080, 0xa6f8, 0xdcdb, 0x0015,
			0xb915, 0xb511, 0xd16b, 0x000f,
			0xb40f, 0xd06b, 0x000d, 0xb206,
			0x7c01, 0x5800, 0x7c04, 0x5c00,
			0x3011, 0x7c01, 0x5801, 0x7c04,
			0x5c04, 0x3019, 0x30a5, 0x3127,
			0x31d5, 0x7fe0, 0x4c60, 0x7c07,
			0x6803, 0x7d00, 0x6900, 0x65a0,
			0x0000, 0x0000, 0xaf03, 0x6015,
			0x303e, 0x6017, 0x57e0, 0x580c,
			0x588c, 0x7fdd, 0x5fa2, 0x4827,
			0x7c1f, 0x4c00, 0x7c1f, 0x4c10,
			0x8400, 0x7c30, 0x6020, 0x48bf,
			0x7c1f, 0x4c00, 0x7c1f, 0x4c01,
			0x7c07, 0x6803, 0xb806, 0x7c08,
			0x6800, 0x0000, 0x0000, 0x305c,
			0x7c08, 0x6808, 0x0000, 0x0000,
			0xae06, 0x7c02, 0x5c02, 0x0000,
			0x0000, 0x3067, 0x8e05, 0x7c02,
			0x5c00, 0x0000, 0x0000, 0xad06,
			0x7c20, 0x5c20, 0x0000, 0x0000,
			0x3072, 0x8d05, 0x7c20, 0x5c00,
			0x0000, 0x0000, 0xa008, 0x7c07,
			0x6800, 0xb8db, 0x7c07, 0x6803,
			0xd9b3, 0x00d7, 0x7fe0, 0x4c80,
			0x7c08, 0x6800, 0x0000, 0x0000,
			0x7c23, 0x5c23, 0x481d, 0x7c1f,
			0x4c00, 0x7c1f, 0x4c02, 0x5310,
			0x81ff, 0x30f5, 0x7fe0, 0x4d00,
			0x4832, 0x7c1f, 0x4c00, 0x7c1f,
			0x4c10, 0x7c08, 0x6000, 0xa49e,
			0x7c07, 0x6800, 0xb89b, 0x7c07,
			0x6803, 0xd9b3, 0x00f9, 0x7fe0,
			0x4d20, 0x7e00, 0x6200, 0x3001,
			0x7fe0, 0x4dc0, 0xd09d, 0x0002,
			0xb4fe, 0x7fe0, 0x4d80, 0x7c04,
			0x6004, 0x7c07, 0x6802, 0x6728,
			0x0000, 0x0000, 0x7c08, 0x6000,
			0x486c, 0x7c1f, 0x4c00, 0x7c1f,
			0x4c01, 0x9503, 0x7e00, 0x6200,
			0x571f, 0x5fbb, 0xaa05, 0x5b58,
			0x7d80, 0x6100, 0x30c2, 0x5b64,
			0x7d80, 0x6080, 0xcdab, 0x0063,
			0xcd8d, 0x0061, 0xd96b, 0x005f,
			0xd0a0, 0x00d7, 0xcba0, 0x0003,
			0x80ec, 0x30cf, 0x30dc, 0x7fe0,
			0x4ce0, 0x4832, 0x7c1f, 0x4c00,
			0x7c1f, 0x4c08, 0x7c08, 0x6008,
			0x8300, 0xb902, 0x30a5, 0x308a,
			0x7fe0, 0x4da0, 0x65a8, 0x0000,
			0x0000, 0x56a0, 0x590c, 0x7ffd,
			0x5fa2, 0xae06, 0x7c02, 0x5c02,
			0x0000, 0x0000, 0x30f0, 0x8e05,
			0x7c02, 0x5c00, 0x0000, 0x0000,
			0xcba4, 0x0004, 0xcd8d, 0x0002,
			0x80f1, 0x7fe0, 0x4ca0, 0x7c08,
			0x6408, 0x0000, 0x0000, 0x7d00,
			0x6800, 0xb603, 0x7c10, 0x6010,
			0x7d1f, 0x551f, 0x5fb3, 0xaa07,
			0x7c80, 0x5800, 0x5b58, 0x7d80,
			0x6100, 0x310f, 0x7c80, 0x5800,
			0x5b64, 0x7d80, 0x6080, 0x4827,
			0x7c1f, 0x4c00, 0x7c1f, 0x4c10,
			0x8400, 0x7c10, 0x6000, 0x7fe0,
			0x4cc0, 0x5fbb, 0x4824, 0x7c1f,
			0x4c00, 0x7c1f, 0x4c04, 0x8200,
			0x7ce0, 0x5400, 0x6728, 0x0000,
			0x0000, 0x30cf, 0x3001, 0x7fe0,
			0x4e00, 0x4007, 0x4400, 0x5310,
			0x7c07, 0x6800, 0x673e, 0x0000,
			0x0000, 0x570f, 0x5fff, 0xaa05,
			0x585b, 0x7d80, 0x6100, 0x313b,
			0x5867, 0x7d80, 0x6080, 0x9403,
			0x7e00, 0x6200, 0xcda3, 0x00e7,
			0xcd85, 0x00e5, 0xd96b, 0x00e3,
			0x96e3, 0x7c07, 0x6800, 0x673e,
			0x0000, 0x0000, 0x7fe0, 0x4e20,
			0x96db, 0x8b04, 0x7c08, 0x5008,
			0xab03, 0x7c08, 0x5000, 0x7c07,
			0x6801, 0x677e, 0x0000, 0x0000,
			0xdb7c, 0x00ec, 0x0000, 0x7fe1,
			0x4f40, 0x4837, 0x4418, 0x41c7,
			0x7fe0, 0x4e40, 0x7c40, 0x5400,
			0x7c1f, 0x4c01, 0x7c1f, 0x4c01,
			0x8fbf, 0xd2a0, 0x004b, 0x9204,
			0xa042, 0x3168, 0x3127, 0x7fe1,
			0x4f60, 0x489c, 0x4628, 0x7fe0,
			0x4e60, 0x7e28, 0x4628, 0x7c40,
			0x5400, 0x7c01, 0x5800, 0x7c04,
			0x5c00, 0x41e8, 0x7c1f, 0x4c01,
			0x7c1f, 0x4c01, 0x8fa5, 0xb241,
			0xa02a, 0x3182, 0x7fe0, 0x4ea0,
			0x7c02, 0x4402, 0x4448, 0x4894,
			0x7c1f, 0x4c01, 0x7c1f, 0x4c03,
			0x4824, 0x7c1f, 0x4c07, 0x41ef,
			0x41ff, 0x4891, 0x7c1f, 0x4c07,
			0x7c1f, 0x4c17, 0x8400, 0x8ef8,
			0x41c7, 0x8f8a, 0x92d5, 0xa10f,
			0xd480, 0x0008, 0xd580, 0x00b8,
			0xa202, 0x319d, 0x7c04, 0x4404,
			0x319d, 0xd484, 0x00f3, 0xd484,
			0x00f1, 0x3127, 0x7fe0, 0x4ee0,
			0x7c40, 0x5400, 0x4488, 0x41cf,
			0x3127, 0x7fe0, 0x4ec0, 0x48f3,
			0x7c1f, 0x4c01, 0x7c1f, 0x4c09,
			0x4508, 0x41c7, 0x8fb0, 0xd218,
			0x00ae, 0xd2a4, 0x009e, 0x31be,
			0x7fe0, 0x4e80, 0x4832, 0x7c1f,
			0x4c01, 0x7c1f, 0x4c11, 0x4428,
			0x7c40, 0x5440, 0x7c01, 0x5801,
			0x7c04, 0x5c04, 0x41e8, 0xa4b3,
			0x31d3, 0x7fe0, 0x4f20, 0x7c07,
			0x6800, 0x673e, 0x0000, 0x0000,
			0x570f, 0x5fff, 0xaa04, 0x585b,
			0x6100, 0x31e4, 0x5867, 0x6080,
			0xbcf1, 0x3001 };

		patch4(tp);
		rtl_clear_bp(tp);

		generic_ocp_write(tp, 0xf800, 0x3f, sizeof(pla_patch_a),
				  pla_patch_a, MCU_TYPE_PLA);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc26, 0x8000);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc28, 0x170b);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2a, 0x01e1);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2c, 0x0989);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2e, 0x1349);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc30, 0x01b7);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc32, 0x061d);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xe422, 0x0020);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xe420, 0x0018);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc34, 0x1785);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc36, 0x047b);

		ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE, 0x2000);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb092, 0x7070);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb098, 0x0600);
		for (i = 0; i < ARRAY_SIZE(ram_code1); i++)
			ocp_write_word(tp, MCU_TYPE_PLA, 0xb09a, ram_code1[i]);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb098, 0x0200);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb092, 0x7030);
	} else if (tp->version == RTL_VER_02) {
		static u8 pla_patch_a2[] = {
			0x08, 0xe0, 0x1a, 0xe0,
			0xf2, 0xe0, 0xfa, 0xe0,
			0x32, 0xe1, 0x34, 0xe1,
			0x36, 0xe1, 0x38, 0xe1,
			0x2c, 0x75, 0xdc, 0x21,
			0xbc, 0x25, 0x04, 0x13,
			0x0b, 0xf0, 0x03, 0x13,
			0x09, 0xf0, 0x02, 0x13,
			0x07, 0xf0, 0x01, 0x13,
			0x05, 0xf0, 0x08, 0x13,
			0x03, 0xf0, 0x04, 0xc3,
			0x00, 0xbb, 0x03, 0xc3,
			0x00, 0xbb, 0xd2, 0x17,
			0xbc, 0x17, 0x14, 0xc2,
			0x40, 0x73, 0xba, 0x48,
			0x40, 0x9b, 0x11, 0xc2,
			0x40, 0x73, 0xb0, 0x49,
			0x17, 0xf0, 0xbf, 0x49,
			0x03, 0xf1, 0x09, 0xc5,
			0x00, 0xbd, 0xb1, 0x49,
			0x11, 0xf0, 0xb1, 0x48,
			0x40, 0x9b, 0x02, 0xc2,
			0x00, 0xba, 0x4e, 0x19,
			0x00, 0xa0, 0x1e, 0xfc,
			0xbc, 0xc0, 0xf0, 0xc0,
			0xde, 0xe8, 0x00, 0x80,
			0x00, 0x60, 0x2c, 0x75,
			0xd4, 0x49, 0x12, 0xf1,
			0x29, 0xe0, 0xf8, 0xc2,
			0x46, 0x71, 0xf7, 0xc2,
			0x40, 0x73, 0xbe, 0x49,
			0x03, 0xf1, 0xf5, 0xc7,
			0x02, 0xe0, 0xf2, 0xc7,
			0x4f, 0x30, 0x26, 0x62,
			0xa1, 0x49, 0xf0, 0xf1,
			0x22, 0x72, 0xa0, 0x49,
			0xed, 0xf1, 0x25, 0x25,
			0x18, 0x1f, 0x97, 0x30,
			0x91, 0x30, 0x36, 0x9a,
			0x2c, 0x75, 0x32, 0xc3,
			0x60, 0x73, 0xb1, 0x49,
			0x0d, 0xf1, 0xdc, 0x21,
			0xbc, 0x25, 0x27, 0xc6,
			0xc0, 0x77, 0x04, 0x13,
			0x18, 0xf0, 0x03, 0x13,
			0x19, 0xf0, 0x02, 0x13,
			0x1a, 0xf0, 0x01, 0x13,
			0x1b, 0xf0, 0xd4, 0x49,
			0x03, 0xf1, 0x1c, 0xc5,
			0x00, 0xbd, 0xcd, 0xc6,
			0xc6, 0x67, 0x2e, 0x75,
			0xd7, 0x22, 0xdd, 0x26,
			0x05, 0x15, 0x1a, 0xf0,
			0x14, 0xc6, 0x00, 0xbe,
			0x13, 0xc5, 0x00, 0xbd,
			0x12, 0xc5, 0x00, 0xbd,
			0xf1, 0x49, 0xfb, 0xf1,
			0xef, 0xe7, 0xf4, 0x49,
			0xfa, 0xf1, 0xec, 0xe7,
			0xf3, 0x49, 0xf7, 0xf1,
			0xe9, 0xe7, 0xf2, 0x49,
			0xf4, 0xf1, 0xe6, 0xe7,
			0xb6, 0xc0, 0xf6, 0x14,
			0x36, 0x14, 0x62, 0x14,
			0x86, 0x15, 0xa0, 0xd1,
			0x00, 0x00, 0xc0, 0x75,
			0xd0, 0x49, 0x46, 0xf0,
			0x26, 0x72, 0xa7, 0x49,
			0x43, 0xf0, 0x22, 0x72,
			0x25, 0x25, 0x20, 0x1f,
			0x97, 0x30, 0x91, 0x30,
			0x40, 0x73, 0xf3, 0xc4,
			0x1c, 0x40, 0x04, 0xf0,
			0xd7, 0x49, 0x05, 0xf1,
			0x37, 0xe0, 0x53, 0x48,
			0xc0, 0x9d, 0x08, 0x02,
			0x40, 0x66, 0x64, 0x27,
			0x06, 0x16, 0x30, 0xf1,
			0x46, 0x63, 0x3b, 0x13,
			0x2d, 0xf1, 0x34, 0x9b,
			0x18, 0x1b, 0x93, 0x30,
			0x2b, 0xc3, 0x10, 0x1c,
			0x2b, 0xe8, 0x01, 0x14,
			0x25, 0xf1, 0x00, 0x1d,
			0x26, 0x1a, 0x8a, 0x30,
			0x22, 0x73, 0xb5, 0x25,
			0x0e, 0x0b, 0x00, 0x1c,
			0x2c, 0xe8, 0x1f, 0xc7,
			0x27, 0x40, 0x1a, 0xf1,
			0x38, 0xe8, 0x32, 0x1f,
			0x8f, 0x30, 0x08, 0x1b,
			0x24, 0xe8, 0x36, 0x72,
			0x46, 0x77, 0x00, 0x17,
			0x0d, 0xf0, 0x13, 0xc3,
			0x1f, 0x40, 0x03, 0xf1,
			0x00, 0x1f, 0x46, 0x9f,
			0x44, 0x77, 0x9f, 0x44,
			0x5f, 0x44, 0x17, 0xe8,
			0x0a, 0xc7, 0x27, 0x40,
			0x05, 0xf1, 0x02, 0xc3,
			0x00, 0xbb, 0x1c, 0x1b,
			0xd2, 0x1a, 0xff, 0xc7,
			0x00, 0xbf, 0xb8, 0xcd,
			0xff, 0xff, 0x02, 0x0c,
			0x54, 0xa5, 0xdc, 0xa5,
			0x2f, 0x40, 0x05, 0xf1,
			0x00, 0x14, 0xfa, 0xf1,
			0x01, 0x1c, 0x02, 0xe0,
			0x00, 0x1c, 0x80, 0xff,
			0xb0, 0x49, 0x04, 0xf0,
			0x01, 0x0b, 0xd3, 0xa1,
			0x03, 0xe0, 0x02, 0x0b,
			0xd3, 0xa5, 0x27, 0x31,
			0x20, 0x37, 0x02, 0x0b,
			0xd3, 0xa5, 0x27, 0x31,
			0x20, 0x37, 0x00, 0x13,
			0xfb, 0xf1, 0x80, 0xff,
			0x22, 0x73, 0xb5, 0x25,
			0x18, 0x1e, 0xde, 0x30,
			0xd9, 0x30, 0x64, 0x72,
			0x11, 0x1e, 0x68, 0x23,
			0x16, 0x31, 0x80, 0xff,
			0x08, 0xc2, 0x40, 0x73,
			0x3a, 0x48, 0x40, 0x9b,
			0x06, 0xff, 0x02, 0xc6,
			0x00, 0xbe, 0x4e, 0x18,
			0x1e, 0xfc, 0x33, 0xc5,
			0xa0, 0x74, 0xc0, 0x49,
			0x1f, 0xf0, 0x30, 0xc5,
			0xa0, 0x73, 0x00, 0x13,
			0x04, 0xf1, 0xa2, 0x73,
			0x00, 0x13, 0x14, 0xf0,
			0x28, 0xc5, 0xa0, 0x74,
			0xc8, 0x49, 0x1b, 0xf1,
			0x26, 0xc5, 0xa0, 0x76,
			0xa2, 0x74, 0x01, 0x06,
			0x20, 0x37, 0xa0, 0x9e,
			0xa2, 0x9c, 0x1e, 0xc5,
			0xa2, 0x73, 0x23, 0x40,
			0x10, 0xf8, 0x04, 0xf3,
			0xa0, 0x73, 0x33, 0x40,
			0x0c, 0xf8, 0x15, 0xc5,
			0xa0, 0x74, 0x41, 0x48,
			0xa0, 0x9c, 0x14, 0xc5,
			0xa0, 0x76, 0x62, 0x48,
			0xe0, 0x48, 0xa0, 0x9e,
			0x10, 0xc6, 0x00, 0xbe,
			0x0a, 0xc5, 0xa0, 0x74,
			0x48, 0x48, 0xa0, 0x9c,
			0x0b, 0xc5, 0x20, 0x1e,
			0xa0, 0x9e, 0xe5, 0x48,
			0xa0, 0x9e, 0xf0, 0xe7,
			0xbc, 0xc0, 0xc8, 0xd2,
			0xcc, 0xd2, 0x28, 0xe4,
			0x22, 0x02, 0xf0, 0xc0,
			0x02, 0xc6, 0x00, 0xbe,
			0x00, 0x00, 0x02, 0xc6,
			0x00, 0xbe, 0x00, 0x00,
			0x02, 0xc6, 0x00, 0xbe,
			0x00, 0x00, 0x02, 0xc6,
			0x00, 0xbe, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00 };

		rtl_clear_bp(tp);

		generic_ocp_write(tp, 0xf800, 0xff, sizeof(pla_patch_a2),
				  pla_patch_a2, MCU_TYPE_PLA);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc26, 0x8000);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc28, 0x17a5);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2a, 0x13ad);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2c, 0x184d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2e, 0x01e1);
	}
}

static void r8152_aldps_en(struct r8152 *tp, bool enable)
{
	if (enable) {
		ocp_reg_write(tp, OCP_ALDPS_CONFIG, ENPWRSAVE | ENPDNPS |
						    LINKENA | DIS_SDSAVE);
	} else {
		ocp_reg_write(tp, OCP_ALDPS_CONFIG, ENPDNPS | LINKENA |
						    DIS_SDSAVE);
		msleep(20);
	}
}

static inline void r8152_mmd_indirect(struct r8152 *tp, u16 dev, u16 reg)
{
	ocp_reg_write(tp, OCP_EEE_AR, FUN_ADDR | dev);
	ocp_reg_write(tp, OCP_EEE_DATA, reg);
	ocp_reg_write(tp, OCP_EEE_AR, FUN_DATA | dev);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
static u16 r8152_mmd_read(struct r8152 *tp, u16 dev, u16 reg)
{
	u16 data;

	r8152_mmd_indirect(tp, dev, reg);
	data = ocp_reg_read(tp, OCP_EEE_DATA);
	ocp_reg_write(tp, OCP_EEE_AR, 0x0000);

	return data;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */

static void r8152_mmd_write(struct r8152 *tp, u16 dev, u16 reg, u16 data)
{
	r8152_mmd_indirect(tp, dev, reg);
	ocp_reg_write(tp, OCP_EEE_DATA, data);
	ocp_reg_write(tp, OCP_EEE_AR, 0x0000);
}

static void r8152_eee_en(struct r8152 *tp, bool enable)
{
	u16 config1, config2, config3;
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEE_CR);
	config1 = ocp_reg_read(tp, OCP_EEE_CONFIG1) & ~sd_rise_time_mask;
	config2 = ocp_reg_read(tp, OCP_EEE_CONFIG2);
	config3 = ocp_reg_read(tp, OCP_EEE_CONFIG3) & ~fast_snr_mask;

	if (enable) {
		ocp_data |= EEE_RX_EN | EEE_TX_EN;
		config1 |= EEE_10_CAP | EEE_NWAY_EN | TX_QUIET_EN | RX_QUIET_EN;
		config1 |= sd_rise_time(1);
		config2 |= RG_DACQUIET_EN | RG_LDVQUIET_EN;
		config3 |= fast_snr(42);
	} else {
		ocp_data &= ~(EEE_RX_EN | EEE_TX_EN);
		config1 &= ~(EEE_10_CAP | EEE_NWAY_EN | TX_QUIET_EN |
			     RX_QUIET_EN);
		config1 |= sd_rise_time(7);
		config2 &= ~(RG_DACQUIET_EN | RG_LDVQUIET_EN);
		config3 |= fast_snr(511);
	}

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_EEE_CR, ocp_data);
	ocp_reg_write(tp, OCP_EEE_CONFIG1, config1);
	ocp_reg_write(tp, OCP_EEE_CONFIG2, config2);
	ocp_reg_write(tp, OCP_EEE_CONFIG3, config3);
}

static void r8152b_enable_eee(struct r8152 *tp)
{
	r8152_eee_en(tp, true);
	r8152_mmd_write(tp, MDIO_MMD_AN, MDIO_AN_EEE_ADV, MDIO_EEE_100TX);
}

static void r8152b_enable_fc(struct r8152 *tp)
{
	u16 anar;

	anar = r8152_mdio_read(tp, MII_ADVERTISE);
	anar |= ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
	r8152_mdio_write(tp, MII_ADVERTISE, anar);
}

static void rtl8152_disable(struct r8152 *tp)
{
	tp->rtl_ops.aldps_enable(tp, false);
	rtl_disable(tp);
	tp->rtl_ops.aldps_enable(tp, true);
}

static void r8152b_hw_phy_cfg(struct r8152 *tp)
{
	r8152b_firmware(tp);

	r8152b_enable_eee(tp);
	r8152_aldps_en(tp, true);
	r8152b_enable_fc(tp);

	set_bit(PHY_RESET, &tp->flags);
}

static void r8152b_exit_oob(struct r8152 *tp)
{
	u32 ocp_data;
	int i;

	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);

	rxdy_gated_en(tp, true);
	r8153_teredo_off(tp);
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CR, 0x00);

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data &= ~NOW_IS_OOB;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data &= ~MCU_BORW_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data |= RE_INIT_LL;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	rtl8152_nic_reset(tp);

	/* rx share fifo credit full threshold */
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL0, RXFIFO_THR1_NORMAL);

	if (tp->udev->speed == USB_SPEED_FULL ||
	    tp->udev->speed == USB_SPEED_LOW) {
		/* rx share fifo credit near full threshold */
		ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1,
				RXFIFO_THR2_FULL);
		ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2,
				RXFIFO_THR3_FULL);
	} else {
		/* rx share fifo credit near full threshold */
		ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1,
				RXFIFO_THR2_HIGH);
		ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2,
				RXFIFO_THR3_HIGH);
	}

	/* TX share fifo free credit full threshold */
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_TXFIFO_CTRL, TXFIFO_THR_NORMAL);

	ocp_write_byte(tp, MCU_TYPE_USB, USB_TX_AGG, TX_AGG_MAX_THRESHOLD);
	ocp_write_dword(tp, MCU_TYPE_USB, USB_RX_BUF_TH, RX_THR_HIGH);
	ocp_write_dword(tp, MCU_TYPE_USB, USB_TX_DMA,
			TEST_MODE_DISABLE | TX_SIZE_ADJUST1);

	rtl_rx_vlan_en(tp, tp->netdev->features & NETIF_F_HW_VLAN_CTAG_RX);

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, RTL8152_RMS);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR0);
	ocp_data |= TCR0_AUTO_FIFO;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_TCR0, ocp_data);
}

static void r8152b_enter_oob(struct r8152 *tp)
{
	u32 ocp_data;
	int i;

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data &= ~NOW_IS_OOB;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL0, RXFIFO_THR1_OOB);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1, RXFIFO_THR2_OOB);
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2, RXFIFO_THR3_OOB);

	rtl_disable(tp);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data |= RE_INIT_LL;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, RTL8152_RMS);

	rtl_rx_vlan_en(tp, true);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PAL_BDC_CR);
	ocp_data |= ALDPS_PROXY_MODE;
	ocp_write_word(tp, MCU_TYPE_PLA, PAL_BDC_CR, ocp_data);

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data |= NOW_IS_OOB | DIS_MCU_CLROOB;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	rxdy_gated_en(tp, false);

	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data |= RCR_APM | RCR_AM | RCR_AB;
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);
}

static int r8153_patch_request(struct r8152 *tp, bool request)
{
	u16 data;
	int i;

	data = ocp_reg_read(tp, OCP_PHY_PATCH_CMD);
	if (request)
		data |= PATCH_REQUEST;
	else
		data &= ~PATCH_REQUEST;
	ocp_reg_write(tp, OCP_PHY_PATCH_CMD, data);

	for (i = 0; request && i < 5000; i++) {
		usleep_range(1000, 2000);
		if (ocp_reg_read(tp, OCP_PHY_PATCH_STAT) & PATCH_READY)
			break;
	}

	if (request && !(ocp_reg_read(tp, OCP_PHY_PATCH_STAT) & PATCH_READY)) {
		netif_err(tp, drv, tp->netdev, "patch request fail\n");
		r8153_patch_request(tp, false);
		return -ETIME;
	} else {
		return 0;
	}
}

static int r8153_pre_ram_code(struct r8152 *tp, u16 key_addr, u16 patch_key)
{
	if (r8153_patch_request(tp, true))
		return -ETIME;

	sram_write(tp, key_addr, patch_key);
	sram_write(tp, 0xb82e, 0x0001);

	return 0;
}

static int r8153_post_ram_code(struct r8152 *tp, u16 key_addr)
{
	u16 data;

	sram_write(tp, 0x0000, 0x0000);

	data = ocp_reg_read(tp, 0xb82e);
	data &= ~0x0001;
	ocp_reg_write(tp, 0xb82e, data);

	sram_write(tp, key_addr, 0x0000);

	r8153_patch_request(tp, false);

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE, tp->ocp_base);

	return 0;
}

static void r8153_wdt1_end(struct r8152 *tp)
{
	int i;

	for (i = 0; i < 104; i++) {
		if (!(ocp_read_byte(tp, MCU_TYPE_USB, 0xe404) & 1))
			break;
		usleep_range(1000, 2000);
	}
}

static void r8153_firmware(struct r8152 *tp)
{
	if (tp->version == RTL_VER_03) {
		r8153_clear_bp(tp);

		r8153_pre_ram_code(tp, 0x8146, 0x7000);
		sram_write(tp, 0xb820, 0x0290);
		sram_write(tp, 0xa012, 0x0000);
		sram_write(tp, 0xa014, 0x2c04);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c18);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c45);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c45);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd502);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8301);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8306);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8208);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd501);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xe018);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x0308);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x60f2);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8404);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x607d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc117);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c16);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc116);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c16);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x607d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc117);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa404);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x0800);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd501);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x62d2);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x615d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc115);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa404);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc307);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd502);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8301);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8306);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8208);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c42);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc114);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8404);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc317);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd701);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x435d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa208);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd502);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa306);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa301);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c42);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8404);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x613d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc115);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc307);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd502);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8301);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8306);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8208);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c42);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc114);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xc317);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd701);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x40dd);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa208);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd502);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa306);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa301);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd500);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd702);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x0800);
		sram_write(tp, 0xa01a, 0x0000);
		sram_write(tp, 0xa006, 0x0fff);
		sram_write(tp, 0xa004, 0x0fff);
		sram_write(tp, 0xa002, 0x05a3);
		sram_write(tp, 0xa000, 0x3591);
		sram_write(tp, 0xb820, 0x0210);
		r8153_post_ram_code(tp, 0x8146);
	} else if (tp->version == RTL_VER_04) {
		static u8 usb_patch_b[] = {
			0x08, 0xe0, 0x0f, 0xe0,
			0x18, 0xe0, 0x24, 0xe0,
			0x26, 0xe0, 0x3a, 0xe0,
			0x84, 0xe0, 0x9c, 0xe0,
			0xc2, 0x49, 0x04, 0xf0,
			0x02, 0xc0, 0x00, 0xb8,
			0x14, 0x18, 0x02, 0xc0,
			0x00, 0xb8, 0x2e, 0x18,
			0x06, 0x89, 0x08, 0xc0,
			0x0c, 0x61, 0x92, 0x48,
			0x93, 0x48, 0x0c, 0x89,
			0x02, 0xc0, 0x00, 0xb8,
			0x08, 0x05, 0x40, 0xb4,
			0x16, 0x89, 0x6d, 0xc0,
			0x00, 0x61, 0x95, 0x49,
			0x06, 0xf0, 0xfa, 0xc0,
			0x0c, 0x61, 0x92, 0x48,
			0x93, 0x48, 0x0c, 0x89,
			0x02, 0xc0, 0x00, 0xb8,
			0xe2, 0x04, 0x02, 0xc2,
			0x00, 0xba, 0xec, 0x11,
			0x60, 0x60, 0x85, 0x49,
			0x0d, 0xf1, 0x11, 0xc6,
			0xd2, 0x61, 0x91, 0x49,
			0xfd, 0xf0, 0x74, 0x60,
			0x04, 0x48, 0x74, 0x88,
			0x08, 0xc6, 0x08, 0xc0,
			0xc4, 0x98, 0x01, 0x18,
			0xc0, 0x88, 0x02, 0xc0,
			0x00, 0xb8, 0x6e, 0x12,
			0x04, 0xe4, 0x0d, 0x00,
			0x00, 0xd4, 0xd1, 0x49,
			0x3c, 0xf1, 0xd2, 0x49,
			0x16, 0xf1, 0xd3, 0x49,
			0x18, 0xf1, 0xd4, 0x49,
			0x19, 0xf1, 0xd5, 0x49,
			0x1a, 0xf1, 0xd6, 0x49,
			0x1b, 0xf1, 0xd7, 0x49,
			0x1c, 0xf1, 0xd8, 0x49,
			0x1d, 0xf1, 0xd9, 0x49,
			0x20, 0xf1, 0xda, 0x49,
			0x23, 0xf1, 0xdb, 0x49,
			0x24, 0xf1, 0x02, 0xc4,
			0x00, 0xbc, 0x20, 0x04,
			0xe5, 0x8e, 0x02, 0xc4,
			0x00, 0xbc, 0x14, 0x02,
			0x02, 0xc4, 0x00, 0xbc,
			0x16, 0x02, 0x02, 0xc4,
			0x00, 0xbc, 0x18, 0x02,
			0x02, 0xc4, 0x00, 0xbc,
			0x1a, 0x02, 0x02, 0xc4,
			0x00, 0xbc, 0x1c, 0x02,
			0x02, 0xc4, 0x00, 0xbc,
			0x94, 0x02, 0x10, 0xc7,
			0xe0, 0x8e, 0x02, 0xc4,
			0x00, 0xbc, 0x8a, 0x02,
			0x0b, 0xc7, 0xe4, 0x8e,
			0x02, 0xc4, 0x00, 0xbc,
			0x88, 0x02, 0x02, 0xc4,
			0x00, 0xbc, 0x6e, 0x02,
			0x02, 0xc4, 0x00, 0xbc,
			0x5a, 0x02, 0x30, 0xe4,
			0x0c, 0xc3, 0x60, 0x64,
			0xc5, 0x49, 0x04, 0xf1,
			0x74, 0x64, 0xc4, 0x48,
			0x74, 0x8c, 0x06, 0xc3,
			0x64, 0x8e, 0x02, 0xc4,
			0x00, 0xbc, 0x20, 0x04,
			0x00, 0xd8, 0x00, 0xe4,
			0xb2, 0xc0, 0x00, 0x61,
			0x90, 0x49, 0x09, 0xf1,
			0x8b, 0xc6, 0xca, 0x61,
			0x94, 0x49, 0x0e, 0xf1,
			0xf6, 0xc6, 0xda, 0x60,
			0x81, 0x49, 0x0a, 0xf0,
			0x65, 0x60, 0x03, 0x48,
			0x65, 0x88, 0xef, 0xc6,
			0xdc, 0x60, 0x80, 0x48,
			0xdc, 0x88, 0x05, 0xc6,
			0x00, 0xbe, 0x02, 0xc6,
			0x00, 0xbe, 0x36, 0x13,
			0x4c, 0x17, 0x99, 0xc4,
			0x80, 0x65, 0xd0, 0x49,
			0x04, 0xf1, 0xfa, 0x75,
			0x04, 0xc4, 0x00, 0xbc,
			0x03, 0xc4, 0x00, 0xbc,
			0x9a, 0x00, 0xee, 0x01 };
		static u8 pla_patch_b[] = {
			0x08, 0xe0, 0xea, 0xe0,
			0xf2, 0xe0, 0x04, 0xe1,
			0x09, 0xe1, 0x0e, 0xe1,
			0x46, 0xe1, 0xf7, 0xe1,
			0x14, 0xc2, 0x40, 0x73,
			0xba, 0x48, 0x40, 0x9b,
			0x11, 0xc2, 0x40, 0x73,
			0xb0, 0x49, 0x17, 0xf0,
			0xbf, 0x49, 0x03, 0xf1,
			0x09, 0xc5, 0x00, 0xbd,
			0xb1, 0x49, 0x11, 0xf0,
			0xb1, 0x48, 0x40, 0x9b,
			0x02, 0xc2, 0x00, 0xba,
			0x1a, 0x17, 0x00, 0xe0,
			0x1e, 0xfc, 0xbc, 0xc0,
			0xf0, 0xc0, 0xde, 0xe8,
			0x00, 0x80, 0x00, 0x20,
			0x2c, 0x75, 0xd4, 0x49,
			0x12, 0xf1, 0x32, 0xe0,
			0xf8, 0xc2, 0x46, 0x71,
			0xf7, 0xc2, 0x40, 0x73,
			0xbe, 0x49, 0x03, 0xf1,
			0xf5, 0xc7, 0x02, 0xe0,
			0xf2, 0xc7, 0x4f, 0x30,
			0x26, 0x62, 0xa1, 0x49,
			0xf0, 0xf1, 0x22, 0x72,
			0xa0, 0x49, 0xed, 0xf1,
			0x25, 0x25, 0x18, 0x1f,
			0x97, 0x30, 0x91, 0x30,
			0x36, 0x9a, 0x2c, 0x75,
			0x3c, 0xc3, 0x60, 0x73,
			0xb1, 0x49, 0x0d, 0xf1,
			0xdc, 0x21, 0xbc, 0x25,
			0x30, 0xc6, 0xc0, 0x77,
			0x04, 0x13, 0x21, 0xf0,
			0x03, 0x13, 0x22, 0xf0,
			0x02, 0x13, 0x23, 0xf0,
			0x01, 0x13, 0x24, 0xf0,
			0x08, 0x13, 0x08, 0xf1,
			0x2e, 0x73, 0xba, 0x21,
			0xbd, 0x25, 0x05, 0x13,
			0x03, 0xf1, 0x24, 0xc5,
			0x00, 0xbd, 0xd4, 0x49,
			0x03, 0xf1, 0x1c, 0xc5,
			0x00, 0xbd, 0xc4, 0xc6,
			0xc6, 0x67, 0x2e, 0x75,
			0xd7, 0x22, 0xdd, 0x26,
			0x05, 0x15, 0x1b, 0xf0,
			0x14, 0xc6, 0x00, 0xbe,
			0x13, 0xc5, 0x00, 0xbd,
			0x12, 0xc5, 0x00, 0xbd,
			0xf1, 0x49, 0xfb, 0xf1,
			0xef, 0xe7, 0xf4, 0x49,
			0xfa, 0xf1, 0xec, 0xe7,
			0xf3, 0x49, 0xf7, 0xf1,
			0xe9, 0xe7, 0xf2, 0x49,
			0xf4, 0xf1, 0xe6, 0xe7,
			0xb6, 0xc0, 0x9e, 0x12,
			0xde, 0x11, 0x0a, 0x12,
			0x3c, 0x13, 0x00, 0xa0,
			0xa0, 0xd1, 0x00, 0x00,
			0xc0, 0x75, 0xd0, 0x49,
			0x46, 0xf0, 0x26, 0x72,
			0xa7, 0x49, 0x43, 0xf0,
			0x22, 0x72, 0x25, 0x25,
			0x20, 0x1f, 0x97, 0x30,
			0x91, 0x30, 0x40, 0x73,
			0xf3, 0xc4, 0x1c, 0x40,
			0x04, 0xf0, 0xd7, 0x49,
			0x05, 0xf1, 0x37, 0xe0,
			0x53, 0x48, 0xc0, 0x9d,
			0x08, 0x02, 0x40, 0x66,
			0x64, 0x27, 0x06, 0x16,
			0x30, 0xf1, 0x46, 0x63,
			0x3b, 0x13, 0x2d, 0xf1,
			0x34, 0x9b, 0x18, 0x1b,
			0x93, 0x30, 0x2b, 0xc3,
			0x10, 0x1c, 0x2b, 0xe8,
			0x01, 0x14, 0x25, 0xf1,
			0x00, 0x1d, 0x26, 0x1a,
			0x8a, 0x30, 0x22, 0x73,
			0xb5, 0x25, 0x0e, 0x0b,
			0x00, 0x1c, 0x2c, 0xe8,
			0x1f, 0xc7, 0x27, 0x40,
			0x1a, 0xf1, 0x38, 0xe8,
			0x32, 0x1f, 0x8f, 0x30,
			0x08, 0x1b, 0x24, 0xe8,
			0x36, 0x72, 0x46, 0x77,
			0x00, 0x17, 0x0d, 0xf0,
			0x13, 0xc3, 0x1f, 0x40,
			0x03, 0xf1, 0x00, 0x1f,
			0x46, 0x9f, 0x44, 0x77,
			0x9f, 0x44, 0x5f, 0x44,
			0x17, 0xe8, 0x0a, 0xc7,
			0x27, 0x40, 0x05, 0xf1,
			0x02, 0xc3, 0x00, 0xbb,
			0xfa, 0x18, 0xb0, 0x18,
			0xff, 0xc7, 0x00, 0xbf,
			0xb8, 0xcd, 0xff, 0xff,
			0x02, 0x0c, 0x54, 0xa5,
			0xdc, 0xa5, 0x2f, 0x40,
			0x05, 0xf1, 0x00, 0x14,
			0xfa, 0xf1, 0x01, 0x1c,
			0x02, 0xe0, 0x00, 0x1c,
			0x80, 0xff, 0xb0, 0x49,
			0x04, 0xf0, 0x01, 0x0b,
			0xd3, 0xa1, 0x03, 0xe0,
			0x02, 0x0b, 0xd3, 0xa5,
			0x27, 0x31, 0x20, 0x37,
			0x02, 0x0b, 0xd3, 0xa5,
			0x27, 0x31, 0x20, 0x37,
			0x00, 0x13, 0xfb, 0xf1,
			0x80, 0xff, 0x22, 0x73,
			0xb5, 0x25, 0x18, 0x1e,
			0xde, 0x30, 0xd9, 0x30,
			0x64, 0x72, 0x11, 0x1e,
			0x68, 0x23, 0x16, 0x31,
			0x80, 0xff, 0x08, 0xc2,
			0x40, 0x73, 0x3a, 0x48,
			0x40, 0x9b, 0x06, 0xff,
			0x02, 0xc6, 0x00, 0xbe,
			0x08, 0x16, 0x1e, 0xfc,
			0x2c, 0x75, 0xdc, 0x21,
			0xbc, 0x25, 0x04, 0x13,
			0x0b, 0xf0, 0x03, 0x13,
			0x09, 0xf0, 0x02, 0x13,
			0x07, 0xf0, 0x01, 0x13,
			0x05, 0xf0, 0x08, 0x13,
			0x03, 0xf0, 0x04, 0xc3,
			0x00, 0xbb, 0x03, 0xc3,
			0x00, 0xbb, 0x8c, 0x15,
			0x76, 0x15, 0xa0, 0x64,
			0x40, 0x48, 0xa0, 0x8c,
			0x02, 0xc4, 0x00, 0xbc,
			0x82, 0x00, 0xa0, 0x62,
			0x21, 0x48, 0xa0, 0x8a,
			0x02, 0xc2, 0x00, 0xba,
			0x40, 0x03, 0x33, 0xc5,
			0xa0, 0x74, 0xc0, 0x49,
			0x1f, 0xf0, 0x30, 0xc5,
			0xa0, 0x73, 0x00, 0x13,
			0x04, 0xf1, 0xa2, 0x73,
			0x00, 0x13, 0x14, 0xf0,
			0x28, 0xc5, 0xa0, 0x74,
			0xc8, 0x49, 0x1b, 0xf1,
			0x26, 0xc5, 0xa0, 0x76,
			0xa2, 0x74, 0x01, 0x06,
			0x20, 0x37, 0xa0, 0x9e,
			0xa2, 0x9c, 0x1e, 0xc5,
			0xa2, 0x73, 0x23, 0x40,
			0x10, 0xf8, 0x04, 0xf3,
			0xa0, 0x73, 0x33, 0x40,
			0x0c, 0xf8, 0x15, 0xc5,
			0xa0, 0x74, 0x41, 0x48,
			0xa0, 0x9c, 0x14, 0xc5,
			0xa0, 0x76, 0x62, 0x48,
			0xe0, 0x48, 0xa0, 0x9e,
			0x10, 0xc6, 0x00, 0xbe,
			0x0a, 0xc5, 0xa0, 0x74,
			0x48, 0x48, 0xa0, 0x9c,
			0x0b, 0xc5, 0x20, 0x1e,
			0xa0, 0x9e, 0xe5, 0x48,
			0xa0, 0x9e, 0xf0, 0xe7,
			0xbc, 0xc0, 0xc8, 0xd2,
			0xcc, 0xd2, 0x28, 0xe4,
			0xe6, 0x01, 0xf0, 0xc0,
			0x18, 0x89, 0x00, 0x1d,
			0x3c, 0xc3, 0x64, 0x71,
			0x3c, 0xc0, 0x02, 0x99,
			0x00, 0x61, 0x67, 0x11,
			0x3c, 0xf1, 0x69, 0x33,
			0x35, 0xc0, 0x28, 0x40,
			0xf6, 0xf1, 0x34, 0xc0,
			0x00, 0x19, 0x81, 0x1b,
			0x91, 0xe8, 0x31, 0xc0,
			0x04, 0x1a, 0x84, 0x1b,
			0x8d, 0xe8, 0x82, 0xe8,
			0xa3, 0x49, 0xfe, 0xf0,
			0x2b, 0xc0, 0x7e, 0xe8,
			0xa1, 0x48, 0x28, 0xc0,
			0x84, 0x1b, 0x84, 0xe8,
			0x00, 0x1d, 0x69, 0x33,
			0x00, 0x1e, 0x01, 0x06,
			0xff, 0x18, 0x30, 0x40,
			0xfd, 0xf1, 0x19, 0xc0,
			0x00, 0x76, 0x2e, 0x40,
			0xf7, 0xf1, 0x21, 0x48,
			0x19, 0xc0, 0x84, 0x1b,
			0x75, 0xe8, 0x10, 0xc0,
			0x69, 0xe8, 0xa1, 0x49,
			0xfd, 0xf0, 0x11, 0xc0,
			0x00, 0x1a, 0x84, 0x1b,
			0x6d, 0xe8, 0x62, 0xe8,
			0xa5, 0x49, 0xfe, 0xf0,
			0x09, 0xc0, 0x01, 0x19,
			0x81, 0x1b, 0x66, 0xe8,
			0x54, 0xe0, 0x10, 0xd4,
			0x88, 0xd3, 0xb8, 0x0b,
			0x50, 0xe8, 0x20, 0xb4,
			0x10, 0xd8, 0x84, 0xd4,
			0xfd, 0xc0, 0x52, 0xe8,
			0x48, 0x33, 0xf9, 0xc0,
			0x00, 0x61, 0x9c, 0x20,
			0x9c, 0x24, 0xd0, 0x49,
			0x04, 0xf0, 0x04, 0x11,
			0x02, 0xf1, 0x03, 0xe0,
			0x00, 0x11, 0x06, 0xf1,
			0x5c, 0xc0, 0x00, 0x61,
			0x92, 0x48, 0x00, 0x89,
			0x3a, 0xe0, 0x06, 0x11,
			0x06, 0xf1, 0x55, 0xc0,
			0x00, 0x61, 0x11, 0x48,
			0x00, 0x89, 0x33, 0xe0,
			0x05, 0x11, 0x08, 0xf1,
			0x4e, 0xc0, 0x00, 0x61,
			0x91, 0x49, 0x04, 0xf0,
			0x91, 0x48, 0x00, 0x89,
			0x11, 0xe0, 0xd9, 0xc0,
			0x00, 0x61, 0x98, 0x20,
			0x98, 0x24, 0x25, 0x11,
			0x24, 0xf1, 0x44, 0xc0,
			0x29, 0xe8, 0x95, 0x49,
			0x20, 0xf0, 0xcf, 0xc0,
			0x00, 0x61, 0x98, 0x20,
			0x98, 0x24, 0x25, 0x11,
			0x1a, 0xf1, 0x37, 0xc0,
			0x00, 0x61, 0x92, 0x49,
			0x16, 0xf1, 0x12, 0x48,
			0x00, 0x89, 0x2f, 0xc0,
			0x00, 0x19, 0x00, 0x89,
			0x2d, 0xc0, 0x01, 0x89,
			0x2d, 0xc0, 0x04, 0x19,
			0x81, 0x1b, 0x1c, 0xe8,
			0x2a, 0xc0, 0x14, 0x19,
			0x81, 0x1b, 0x18, 0xe8,
			0x21, 0xc0, 0x0c, 0xe8,
			0x1f, 0xc0, 0x12, 0x48,
			0x81, 0x1b, 0x12, 0xe8,
			0xae, 0xc3, 0x66, 0x71,
			0xae, 0xc0, 0x02, 0x99,
			0x02, 0xc0, 0x00, 0xb8,
			0x96, 0x07, 0x13, 0xc4,
			0x84, 0x98, 0x00, 0x1b,
			0x86, 0x8b, 0x86, 0x73,
			0xbf, 0x49, 0xfe, 0xf1,
			0x80, 0x71, 0x82, 0x72,
			0x80, 0xff, 0x09, 0xc4,
			0x84, 0x98, 0x80, 0x99,
			0x82, 0x9a, 0x86, 0x8b,
			0x86, 0x73, 0xbf, 0x49,
			0xfe, 0xf1, 0x80, 0xff,
			0x08, 0xea, 0x30, 0xd4,
			0x10, 0xc0, 0x12, 0xe8,
			0x8a, 0xd3, 0x28, 0xe4,
			0x2c, 0xe4, 0x00, 0xd8,
			0x00, 0x00, 0x00, 0x00 };

		r8153_pre_ram_code(tp, 0x8146, 0x7001);
		sram_write(tp, 0xb820, 0x0290);
		sram_write(tp, 0xa012, 0x0000);
		sram_write(tp, 0xa014, 0x2c04);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c07);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c0a);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c0d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa240);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa104);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x292d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8620);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa480);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2a2c);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8480);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa101);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2a36);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd056);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2223);
		sram_write(tp, 0xa01a, 0x0000);
		sram_write(tp, 0xa006, 0x0222);
		sram_write(tp, 0xa004, 0x0a35);
		sram_write(tp, 0xa002, 0x0a2b);
		sram_write(tp, 0xa000, 0xf92c);
		sram_write(tp, 0xb820, 0x0210);
		r8153_post_ram_code(tp, 0x8146);

		r8153_wdt1_end(tp);
		r8153_clear_bp(tp);

		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x0000);
		generic_ocp_write(tp, 0xf800, 0xff, sizeof(usb_patch_b),
				  usb_patch_b, MCU_TYPE_USB);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc26, 0xa000);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc28, 0x180c);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2a, 0x0506);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2c, 0x04E0);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2e, 0x11E4);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc30, 0x125C);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc32, 0x0232);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc34, 0x131E);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc36, 0x0098);
		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x00FF);

		if (!(ocp_read_word(tp, MCU_TYPE_PLA, 0xd38e) & BIT(0))) {
			ocp_write_word(tp, MCU_TYPE_PLA, 0xd38c, 0x0082);
			ocp_write_word(tp, MCU_TYPE_PLA, 0xd38e, 0x0082);
		}

		ocp_write_word(tp, MCU_TYPE_PLA, PLA_BP_EN, 0x0000);
		generic_ocp_write(tp, 0xf800, 0xff, sizeof(pla_patch_b),
				  pla_patch_b, MCU_TYPE_PLA);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc26, 0x8000);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc28, 0x1154);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2a, 0x1606);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2c, 0x155a);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2e, 0x0080);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc30, 0x033c);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc32, 0x01a0);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc34, 0x0794);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc36, 0x0000);
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_BP_EN, 0x007f);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xd388, 0x08ca);
	} else if (tp->version == RTL_VER_05) {
		u32 ocp_data;
		static u8 usb_patch_c[] = {
			0x08, 0xe0, 0x0a, 0xe0,
			0x14, 0xe0, 0x2e, 0xe0,
			0x3a, 0xe0, 0x41, 0xe0,
			0x70, 0xe0, 0x7b, 0xe0,
			0x02, 0xc5, 0x00, 0xbd,
			0x38, 0x3b, 0xdb, 0x49,
			0x04, 0xf1, 0x06, 0xc3,
			0x00, 0xbb, 0x5a, 0x02,
			0x05, 0xc4, 0x03, 0xc3,
			0x00, 0xbb, 0xa4, 0x04,
			0x7e, 0x02, 0x30, 0xd4,
			0x30, 0x18, 0x18, 0xc1,
			0x0c, 0xe8, 0x17, 0xc6,
			0xc7, 0x65, 0xd0, 0x49,
			0x05, 0xf0, 0x32, 0x48,
			0x02, 0xc2, 0x00, 0xba,
			0x3e, 0x16, 0x02, 0xc2,
			0x00, 0xba, 0x48, 0x16,
			0x02, 0xb4, 0x09, 0xc2,
			0x40, 0x99, 0x0e, 0x48,
			0x42, 0x98, 0x42, 0x70,
			0x8e, 0x49, 0xfe, 0xf1,
			0x02, 0xb0, 0x80, 0xff,
			0xc0, 0xd4, 0xe4, 0x40,
			0x20, 0xd4, 0x0c, 0xc0,
			0x00, 0x63, 0xb5, 0x49,
			0x0c, 0xc0, 0x30, 0x18,
			0x06, 0xc1, 0xed, 0xef,
			0xf8, 0xc7, 0x02, 0xc0,
			0x00, 0xb8, 0xd0, 0x10,
			0xe4, 0x4b, 0x00, 0xd8,
			0x07, 0xc3, 0x70, 0x61,
			0x12, 0x48, 0x70, 0x89,
			0x02, 0xc3, 0x00, 0xbb,
			0x9c, 0x15, 0x20, 0xd4,
			0x2b, 0xc5, 0xa0, 0x77,
			0x00, 0x1c, 0xa0, 0x9c,
			0x28, 0xc5, 0xa0, 0x64,
			0xc0, 0x48, 0xc1, 0x48,
			0xc2, 0x48, 0xa0, 0x8c,
			0xb1, 0x64, 0xc0, 0x48,
			0xb1, 0x8c, 0x20, 0xc5,
			0xa0, 0x64, 0x40, 0x48,
			0x41, 0x48, 0xc2, 0x48,
			0xa0, 0x8c, 0x19, 0xc5,
			0xa4, 0x64, 0x44, 0x48,
			0xa4, 0x8c, 0xb1, 0x64,
			0x40, 0x48, 0xb1, 0x8c,
			0x14, 0xc4, 0x80, 0x73,
			0x13, 0xc4, 0x82, 0x9b,
			0x11, 0x1b, 0x80, 0x9b,
			0x0c, 0xc5, 0xa0, 0x64,
			0x40, 0x48, 0x41, 0x48,
			0x42, 0x48, 0xa0, 0x8c,
			0x05, 0xc5, 0xa0, 0x9f,
			0x02, 0xc5, 0x00, 0xbd,
			0x6c, 0x3a, 0x1e, 0xfc,
			0x10, 0xd8, 0x86, 0xd4,
			0xf8, 0xcb, 0x20, 0xe4,
			0x0a, 0xc0, 0x16, 0x61,
			0x91, 0x48, 0x16, 0x89,
			0x07, 0xc0, 0x11, 0x19,
			0x0c, 0x89, 0x02, 0xc1,
			0x00, 0xb9, 0x02, 0x06,
			0x00, 0xd4, 0x40, 0xb4,
			0xfe, 0xc0, 0x16, 0x61,
			0x91, 0x48, 0x16, 0x89,
			0xfb, 0xc0, 0x11, 0x19,
			0x0c, 0x89, 0x02, 0xc1,
			0x00, 0xb9, 0xd2, 0x05 };
		static u8 pla_patch_c[] = {
			0x08, 0xe0, 0xea, 0xe0,
			0xf2, 0xe0, 0x04, 0xe1,
			0x06, 0xe1, 0x08, 0xe1,
			0x40, 0xe1, 0xf4, 0xe1,
			0x14, 0xc2, 0x40, 0x73,
			0xba, 0x48, 0x40, 0x9b,
			0x11, 0xc2, 0x40, 0x73,
			0xb0, 0x49, 0x17, 0xf0,
			0xbf, 0x49, 0x03, 0xf1,
			0x09, 0xc5, 0x00, 0xbd,
			0xb1, 0x49, 0x11, 0xf0,
			0xb1, 0x48, 0x40, 0x9b,
			0x02, 0xc2, 0x00, 0xba,
			0xde, 0x18, 0x00, 0xe0,
			0x1e, 0xfc, 0xbc, 0xc0,
			0xf0, 0xc0, 0xde, 0xe8,
			0x00, 0x80, 0x00, 0x20,
			0x2c, 0x75, 0xd4, 0x49,
			0x12, 0xf1, 0x32, 0xe0,
			0xf8, 0xc2, 0x46, 0x71,
			0xf7, 0xc2, 0x40, 0x73,
			0xbe, 0x49, 0x03, 0xf1,
			0xf5, 0xc7, 0x02, 0xe0,
			0xf2, 0xc7, 0x4f, 0x30,
			0x26, 0x62, 0xa1, 0x49,
			0xf0, 0xf1, 0x22, 0x72,
			0xa0, 0x49, 0xed, 0xf1,
			0x25, 0x25, 0x18, 0x1f,
			0x97, 0x30, 0x91, 0x30,
			0x36, 0x9a, 0x2c, 0x75,
			0x3c, 0xc3, 0x60, 0x73,
			0xb1, 0x49, 0x0d, 0xf1,
			0xdc, 0x21, 0xbc, 0x25,
			0x30, 0xc6, 0xc0, 0x77,
			0x04, 0x13, 0x21, 0xf0,
			0x03, 0x13, 0x22, 0xf0,
			0x02, 0x13, 0x23, 0xf0,
			0x01, 0x13, 0x24, 0xf0,
			0x08, 0x13, 0x08, 0xf1,
			0x2e, 0x73, 0xba, 0x21,
			0xbd, 0x25, 0x05, 0x13,
			0x03, 0xf1, 0x24, 0xc5,
			0x00, 0xbd, 0xd4, 0x49,
			0x03, 0xf1, 0x1c, 0xc5,
			0x00, 0xbd, 0xc4, 0xc6,
			0xc6, 0x67, 0x2e, 0x75,
			0xd7, 0x22, 0xdd, 0x26,
			0x05, 0x15, 0x1b, 0xf0,
			0x14, 0xc6, 0x00, 0xbe,
			0x13, 0xc5, 0x00, 0xbd,
			0x12, 0xc5, 0x00, 0xbd,
			0xf1, 0x49, 0xfb, 0xf1,
			0xef, 0xe7, 0xf4, 0x49,
			0xfa, 0xf1, 0xec, 0xe7,
			0xf3, 0x49, 0xf7, 0xf1,
			0xe9, 0xe7, 0xf2, 0x49,
			0xf4, 0xf1, 0xe6, 0xe7,
			0xb6, 0xc0, 0x50, 0x14,
			0x90, 0x13, 0xbc, 0x13,
			0xf2, 0x14, 0x00, 0xa0,
			0xa0, 0xd1, 0x00, 0x00,
			0xc0, 0x75, 0xd0, 0x49,
			0x46, 0xf0, 0x26, 0x72,
			0xa7, 0x49, 0x43, 0xf0,
			0x22, 0x72, 0x25, 0x25,
			0x20, 0x1f, 0x97, 0x30,
			0x91, 0x30, 0x40, 0x73,
			0xf3, 0xc4, 0x1c, 0x40,
			0x04, 0xf0, 0xd7, 0x49,
			0x05, 0xf1, 0x37, 0xe0,
			0x53, 0x48, 0xc0, 0x9d,
			0x08, 0x02, 0x40, 0x66,
			0x64, 0x27, 0x06, 0x16,
			0x30, 0xf1, 0x46, 0x63,
			0x3b, 0x13, 0x2d, 0xf1,
			0x34, 0x9b, 0x18, 0x1b,
			0x93, 0x30, 0x2b, 0xc3,
			0x10, 0x1c, 0x2b, 0xe8,
			0x01, 0x14, 0x25, 0xf1,
			0x00, 0x1d, 0x26, 0x1a,
			0x8a, 0x30, 0x22, 0x73,
			0xb5, 0x25, 0x0e, 0x0b,
			0x00, 0x1c, 0x2c, 0xe8,
			0x1f, 0xc7, 0x27, 0x40,
			0x1a, 0xf1, 0x38, 0xe8,
			0x32, 0x1f, 0x8f, 0x30,
			0x08, 0x1b, 0x24, 0xe8,
			0x36, 0x72, 0x46, 0x77,
			0x00, 0x17, 0x0d, 0xf0,
			0x13, 0xc3, 0x1f, 0x40,
			0x03, 0xf1, 0x00, 0x1f,
			0x46, 0x9f, 0x44, 0x77,
			0x9f, 0x44, 0x5f, 0x44,
			0x17, 0xe8, 0x0a, 0xc7,
			0x27, 0x40, 0x05, 0xf1,
			0x02, 0xc3, 0x00, 0xbb,
			0xbe, 0x1a, 0x74, 0x14,
			0xff, 0xc7, 0x00, 0xbf,
			0xb8, 0xcd, 0xff, 0xff,
			0x02, 0x0c, 0x54, 0xa5,
			0xdc, 0xa5, 0x2f, 0x40,
			0x05, 0xf1, 0x00, 0x14,
			0xfa, 0xf1, 0x01, 0x1c,
			0x02, 0xe0, 0x00, 0x1c,
			0x80, 0xff, 0xb0, 0x49,
			0x04, 0xf0, 0x01, 0x0b,
			0xd3, 0xa1, 0x03, 0xe0,
			0x02, 0x0b, 0xd3, 0xa5,
			0x27, 0x31, 0x20, 0x37,
			0x02, 0x0b, 0xd3, 0xa5,
			0x27, 0x31, 0x20, 0x37,
			0x00, 0x13, 0xfb, 0xf1,
			0x80, 0xff, 0x22, 0x73,
			0xb5, 0x25, 0x18, 0x1e,
			0xde, 0x30, 0xd9, 0x30,
			0x64, 0x72, 0x11, 0x1e,
			0x68, 0x23, 0x16, 0x31,
			0x80, 0xff, 0x08, 0xc2,
			0x40, 0x73, 0x3a, 0x48,
			0x40, 0x9b, 0x06, 0xff,
			0x02, 0xc6, 0x00, 0xbe,
			0xcc, 0x17, 0x1e, 0xfc,
			0x2c, 0x75, 0xdc, 0x21,
			0xbc, 0x25, 0x04, 0x13,
			0x0b, 0xf0, 0x03, 0x13,
			0x09, 0xf0, 0x02, 0x13,
			0x07, 0xf0, 0x01, 0x13,
			0x05, 0xf0, 0x08, 0x13,
			0x03, 0xf0, 0x04, 0xc3,
			0x00, 0xbb, 0x03, 0xc3,
			0x00, 0xbb, 0x50, 0x17,
			0x3a, 0x17, 0x02, 0xc6,
			0x00, 0xbe, 0x00, 0x00,
			0x02, 0xc6, 0x00, 0xbe,
			0x00, 0x00, 0x33, 0xc5,
			0xa0, 0x74, 0xc0, 0x49,
			0x1f, 0xf0, 0x30, 0xc5,
			0xa0, 0x73, 0x00, 0x13,
			0x04, 0xf1, 0xa2, 0x73,
			0x00, 0x13, 0x14, 0xf0,
			0x28, 0xc5, 0xa0, 0x74,
			0xc8, 0x49, 0x1b, 0xf1,
			0x26, 0xc5, 0xa0, 0x76,
			0xa2, 0x74, 0x01, 0x06,
			0x20, 0x37, 0xa0, 0x9e,
			0xa2, 0x9c, 0x1e, 0xc5,
			0xa2, 0x73, 0x23, 0x40,
			0x10, 0xf8, 0x04, 0xf3,
			0xa0, 0x73, 0x33, 0x40,
			0x0c, 0xf8, 0x15, 0xc5,
			0xa0, 0x74, 0x41, 0x48,
			0xa0, 0x9c, 0x14, 0xc5,
			0xa0, 0x76, 0x62, 0x48,
			0xe0, 0x48, 0xa0, 0x9e,
			0x10, 0xc6, 0x00, 0xbe,
			0x0a, 0xc5, 0xa0, 0x74,
			0x48, 0x48, 0xa0, 0x9c,
			0x0b, 0xc5, 0x20, 0x1e,
			0xa0, 0x9e, 0xe5, 0x48,
			0xa0, 0x9e, 0xf0, 0xe7,
			0xbc, 0xc0, 0xc8, 0xd2,
			0xcc, 0xd2, 0x28, 0xe4,
			0xfa, 0x01, 0xf0, 0xc0,
			0x18, 0x89, 0x00, 0x1d,
			0x4a, 0xc3, 0x66, 0x62,
			0xa0, 0x49, 0x06, 0xf0,
			0x40, 0xc0, 0x02, 0x71,
			0x60, 0x99, 0x3e, 0xc1,
			0x03, 0xe0, 0x3b, 0xc0,
			0x3c, 0xc1, 0x02, 0x99,
			0x00, 0x61, 0x67, 0x11,
			0x3d, 0xf1, 0x69, 0x33,
			0x33, 0xc0, 0x28, 0x40,
			0xf7, 0xf1, 0x34, 0xc0,
			0x00, 0x19, 0x81, 0x1b,
			0x8d, 0xe8, 0x31, 0xc0,
			0x04, 0x1a, 0x84, 0x1b,
			0x89, 0xe8, 0x7e, 0xe8,
			0xa3, 0x49, 0xfe, 0xf0,
			0x2b, 0xc0, 0x7a, 0xe8,
			0xa1, 0x48, 0x28, 0xc0,
			0x84, 0x1b, 0x80, 0xe8,
			0x00, 0x1d, 0x69, 0x33,
			0x00, 0x1e, 0x01, 0x06,
			0xff, 0x18, 0x30, 0x40,
			0xfd, 0xf1, 0x1f, 0xc0,
			0x00, 0x76, 0x2e, 0x40,
			0xf7, 0xf1, 0x21, 0x48,
			0x19, 0xc0, 0x84, 0x1b,
			0x71, 0xe8, 0x7a, 0xc0,
			0x65, 0xe8, 0xa1, 0x49,
			0xfd, 0xf0, 0x11, 0xc0,
			0x00, 0x1a, 0x84, 0x1b,
			0x69, 0xe8, 0x5e, 0xe8,
			0xa5, 0x49, 0xfe, 0xf0,
			0x09, 0xc0, 0x01, 0x19,
			0x81, 0x1b, 0x62, 0xe8,
			0x4d, 0xe0, 0xb8, 0x0b,
			0x50, 0xe8, 0x83, 0x00,
			0x82, 0x00, 0x20, 0xb4,
			0x10, 0xd8, 0x84, 0xd4,
			0x88, 0xd3, 0xfc, 0xc0,
			0x4d, 0xe8, 0x48, 0x33,
			0xf6, 0xc0, 0x00, 0x61,
			0x9c, 0x20, 0x9c, 0x24,
			0xd0, 0x49, 0x04, 0xf0,
			0x04, 0x11, 0x02, 0xf1,
			0x03, 0xe0, 0x00, 0x11,
			0x06, 0xf1, 0x58, 0xc0,
			0x00, 0x61, 0x92, 0x48,
			0x00, 0x89, 0x32, 0xe0,
			0x06, 0x11, 0x06, 0xf1,
			0x51, 0xc0, 0x00, 0x61,
			0x11, 0x48, 0x00, 0x89,
			0x2b, 0xe0, 0x05, 0x11,
			0x08, 0xf1, 0x4a, 0xc0,
			0x00, 0x61, 0x91, 0x49,
			0x04, 0xf0, 0x91, 0x48,
			0x00, 0x89, 0x11, 0xe0,
			0xd6, 0xc0, 0x00, 0x61,
			0x98, 0x20, 0x98, 0x24,
			0x25, 0x11, 0x1c, 0xf1,
			0x3e, 0xc0, 0x24, 0xe8,
			0x95, 0x49, 0x18, 0xf0,
			0xcc, 0xc0, 0x00, 0x61,
			0x98, 0x20, 0x98, 0x24,
			0x25, 0x11, 0x12, 0xf1,
			0x33, 0xc0, 0x00, 0x61,
			0x92, 0x49, 0x0e, 0xf1,
			0x12, 0x48, 0x00, 0x89,
			0x2b, 0xc0, 0x00, 0x19,
			0x00, 0x89, 0x29, 0xc0,
			0x01, 0x89, 0x25, 0xc0,
			0x0f, 0xe8, 0x23, 0xc0,
			0x12, 0x48, 0x81, 0x1b,
			0x15, 0xe8, 0xbb, 0xc3,
			0x66, 0x62, 0xa0, 0x49,
			0x04, 0xf0, 0x64, 0x71,
			0xb0, 0xc0, 0x02, 0x99,
			0x02, 0xc0, 0x00, 0xb8,
			0xd6, 0x07, 0x13, 0xc4,
			0x84, 0x98, 0x00, 0x1b,
			0x86, 0x8b, 0x86, 0x73,
			0xbf, 0x49, 0xfe, 0xf1,
			0x80, 0x71, 0x82, 0x72,
			0x80, 0xff, 0x09, 0xc4,
			0x84, 0x98, 0x80, 0x99,
			0x82, 0x9a, 0x86, 0x8b,
			0x86, 0x73, 0xbf, 0x49,
			0xfe, 0xf1, 0x80, 0xff,
			0x08, 0xea, 0x10, 0xd4,
			0x30, 0xd4, 0x10, 0xc0,
			0x12, 0xe8, 0x8a, 0xd3,
			0x00, 0xd8, 0x02, 0xc0,
			0x00, 0xb8, 0xe0, 0x08,
			0x00, 0x00, 0x00, 0x00 };

		ocp_data = ocp_read_word(tp, MCU_TYPE_USB, 0xcfca);
		ocp_data &= ~0x4000;
		ocp_write_word(tp, MCU_TYPE_USB, 0xcfca, ocp_data);

		r8153_pre_ram_code(tp, 0x8146, 0x7001);
		sram_write(tp, 0xb820, 0x0290);
		sram_write(tp, 0xa012, 0x0000);
		sram_write(tp, 0xa014, 0x2c04);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c07);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c0a);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c0d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa240);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa104);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x292d);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8620);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa480);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2a2c);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x8480);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa101);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2a36);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xd056);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2223);
		sram_write(tp, 0xa01a, 0x0000);
		sram_write(tp, 0xa006, 0x0222);
		sram_write(tp, 0xa004, 0x0a35);
		sram_write(tp, 0xa002, 0x0a2b);
		sram_write(tp, 0xa000, 0xf92c);
		sram_write(tp, 0xb820, 0x0210);
		r8153_post_ram_code(tp, 0x8146);

		r8153_wdt1_end(tp);
		r8153_clear_bp(tp);

		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x0000);
		generic_ocp_write(tp, 0xf800, 0xff, sizeof(usb_patch_c),
				  usb_patch_c, MCU_TYPE_USB);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc26, 0xa000);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc28, 0x3b34);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2a, 0x027c);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2c, 0x162c);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2e, 0x10ce);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc30, 0x1578);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc32, 0x3a28);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc34, 0x05f8);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc36, 0x05c8);
		if (ocp_read_byte(tp, MCU_TYPE_USB, 0xcfef) & 1)
			ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x00ff);
		else
			ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x00ef);

		ocp_write_word(tp, MCU_TYPE_PLA, PLA_BP_EN, 0x0000);
		generic_ocp_write(tp, 0xf800, 0xff, sizeof(pla_patch_c),
				  pla_patch_c, MCU_TYPE_PLA);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc26, 0x8000);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc28, 0x1306);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2a, 0x17ca);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2c, 0x171e);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc2e, 0x0000);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc30, 0x0000);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc32, 0x01b4);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc34, 0x07d4);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xfc36, 0x0894);
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_BP_EN, 0x00e7);

		ocp_write_word(tp, MCU_TYPE_PLA, 0xd388, 0x08ca);

		ocp_data = ocp_read_word(tp, MCU_TYPE_USB, 0xcfca);
		ocp_data |= 0x4000;
		ocp_write_word(tp, MCU_TYPE_USB, 0xcfca, ocp_data);
	} else if (tp->version == RTL_VER_06) {
		static u8 usb_patch_d[] = {
			0x08, 0xe0, 0x0e, 0xe0,
			0x11, 0xe0, 0x24, 0xe0,
			0x30, 0xe0, 0x38, 0xe0,
			0x3a, 0xe0, 0x3c, 0xe0,
			0x1e, 0xc3, 0x70, 0x61,
			0x12, 0x48, 0x70, 0x89,
			0x02, 0xc3, 0x00, 0xbb,
			0x02, 0x17, 0x31, 0x19,
			0x02, 0xc3, 0x00, 0xbb,
			0x44, 0x14, 0x30, 0x18,
			0x11, 0xc1, 0x05, 0xe8,
			0x10, 0xc6, 0x02, 0xc2,
			0x00, 0xba, 0x94, 0x17,
			0x02, 0xb4, 0x09, 0xc2,
			0x40, 0x99, 0x0e, 0x48,
			0x42, 0x98, 0x42, 0x70,
			0x8e, 0x49, 0xfe, 0xf1,
			0x02, 0xb0, 0x80, 0xff,
			0xc0, 0xd4, 0xe4, 0x40,
			0x20, 0xd4, 0x0c, 0xc0,
			0x00, 0x63, 0xb5, 0x49,
			0x0c, 0xc0, 0x30, 0x18,
			0x06, 0xc1, 0xed, 0xef,
			0xf8, 0xc7, 0x02, 0xc0,
			0x00, 0xb8, 0x38, 0x12,
			0xe4, 0x4b, 0x00, 0xd8,
			0x0c, 0x61, 0x95, 0x48,
			0x96, 0x48, 0x92, 0x48,
			0x93, 0x48, 0x0c, 0x89,
			0x02, 0xc0, 0x00, 0xb8,
			0x0e, 0x06, 0x02, 0xc5,
			0x00, 0xbd, 0x00, 0x00,
			0x02, 0xc1, 0x00, 0xb9,
			0x00, 0x00, 0x02, 0xc1,
			0x00, 0xb9, 0x00, 0x00 };

		r8153_pre_ram_code(tp, 0x8146, 0x7002);
		sram_write(tp, 0xb820, 0x0290);
		sram_write(tp, 0xa012, 0x0000);
		sram_write(tp, 0xa014, 0x2c04);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c07);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c07);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2c07);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa240);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0xa104);
		ocp_write_word(tp, MCU_TYPE_PLA, 0xb438, 0x2944);
		sram_write(tp, 0xa01a, 0x0000);
		sram_write(tp, 0xa006, 0x0fff);
		sram_write(tp, 0xa004, 0x0fff);
		sram_write(tp, 0xa002, 0x0fff);
		sram_write(tp, 0xa000, 0x1943);
		sram_write(tp, 0xb820, 0x0210);
		r8153_post_ram_code(tp, 0x8146);

		r8153_clear_bp(tp);

		ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x0000);
		generic_ocp_write(tp, 0xf800, 0xff, sizeof(usb_patch_d),
				  usb_patch_d, MCU_TYPE_USB);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc26, 0xa000);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc28, 0x16de);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2a, 0x1442);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2c, 0x1792);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc2e, 0x1236);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc30, 0x0606);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc32, 0x0000);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc34, 0x0000);
		ocp_write_word(tp, MCU_TYPE_USB, 0xfc36, 0x0000);
		if (ocp_read_byte(tp, MCU_TYPE_USB, 0xcfef) & 1)
			ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x001b);
		else
			ocp_write_word(tp, MCU_TYPE_USB, USB_BP_EN, 0x001a);
	}
}

static void r8153b_firmware(struct r8152 *tp)
{
/*
	r8153b_clear_bp(tp, MCU_TYPE_PLA);
	r8153b_clear_bp(tp, MCU_TYPE_USB);
*/
}

static void r8153b_ups_flags_w0w1(struct r8152 *tp, u32 set, u32 clear)
{
	u32 ocp_data;

	ocp_data = ocp_read_dword(tp, MCU_TYPE_USB, USB_UPS_FLAGS);
	ocp_data &= ~clear;
	ocp_data |= set;
	ocp_write_dword(tp, MCU_TYPE_USB, USB_UPS_FLAGS, ocp_data);
}

static void r8153_aldps_en(struct r8152 *tp, bool enable)
{
	u16 data;

	data = ocp_reg_read(tp, OCP_POWER_CFG);
	if (enable) {
		data |= EN_ALDPS;
		ocp_reg_write(tp, OCP_POWER_CFG, data);
	} else {
		int i;

		data &= ~EN_ALDPS;
		ocp_reg_write(tp, OCP_POWER_CFG, data);
		for (i = 0; i < 20; i++) {
			usleep_range(1000, 2000);
			if (ocp_read_word(tp, MCU_TYPE_PLA, 0xe000) & 0x0100)
				break;
		}
	}
}

static void r8153b_aldps_en(struct r8152 *tp, bool enable)
{
	r8153_aldps_en(tp, enable);

	if (enable)
		r8153b_ups_flags_w0w1(tp, UPS_FLAGS_EN_ALDPS, 0);
	else
		r8153b_ups_flags_w0w1(tp, 0, UPS_FLAGS_EN_ALDPS);
}

static void r8153_eee_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;
	u16 config;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEE_CR);
	config = ocp_reg_read(tp, OCP_EEE_CFG);

	if (enable) {
		ocp_data |= EEE_RX_EN | EEE_TX_EN;
		config |= EEE10_EN;
	} else {
		ocp_data &= ~(EEE_RX_EN | EEE_TX_EN);
		config &= ~EEE10_EN;
	}

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_EEE_CR, ocp_data);
	ocp_reg_write(tp, OCP_EEE_CFG, config);
}

static void r8153b_eee_en(struct r8152 *tp, bool enable)
{
	r8153_eee_en(tp, enable);

	if (enable)
		r8153b_ups_flags_w0w1(tp, UPS_FLAGS_EN_EEE, 0);
	else
		r8153b_ups_flags_w0w1(tp, 0, UPS_FLAGS_EN_EEE);
}

static void r8153b_enable_fc(struct r8152 *tp)
{
	r8152b_enable_fc(tp);
	r8153b_ups_flags_w0w1(tp, UPS_FLAGS_EN_FLOW_CTR, 0);
}

static void r8153_hw_phy_cfg(struct r8152 *tp)
{
	u32 ocp_data;
	u16 data;

	/* disable ALDPS before updating the PHY parameters */
	r8153_aldps_en(tp, false);

	/* disable EEE before updating the PHY parameters */
	r8153_eee_en(tp, false);
	ocp_reg_write(tp, OCP_EEE_ADV, 0);

	r8153_firmware(tp);

	if (tp->version == RTL_VER_03) {
		data = ocp_reg_read(tp, OCP_EEE_CFG);
		data &= ~CTAP_SHORT_EN;
		ocp_reg_write(tp, OCP_EEE_CFG, data);
	}

	data = ocp_reg_read(tp, OCP_POWER_CFG);
	data |= EEE_CLKDIV_EN;
	ocp_reg_write(tp, OCP_POWER_CFG, data);

	data = ocp_reg_read(tp, OCP_DOWN_SPEED);
	data |= EN_10M_BGOFF;
	ocp_reg_write(tp, OCP_DOWN_SPEED, data);
	data = ocp_reg_read(tp, OCP_POWER_CFG);
	data |= EN_10M_PLLOFF;
	ocp_reg_write(tp, OCP_POWER_CFG, data);
	sram_write(tp, SRAM_IMPEDANCE, 0x0b13);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR);
	ocp_data |= PFM_PWM_SWITCH;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR, ocp_data);

	/* Enable LPF corner auto tune */
	sram_write(tp, SRAM_LPF_CFG, 0xf70f);

	/* Adjust 10M Amplitude */
	sram_write(tp, SRAM_10M_AMP1, 0x00af);
	sram_write(tp, SRAM_10M_AMP2, 0x0208);

	r8153_eee_en(tp, true);
	ocp_reg_write(tp, OCP_EEE_ADV, MDIO_EEE_1000T | MDIO_EEE_100TX);

	r8153_aldps_en(tp, true);
	r8152b_enable_fc(tp);

	set_bit(PHY_RESET, &tp->flags);
}

static u32 r8152_efuse_read(struct r8152 *tp, u8 addr)
{
	u32 ocp_data;

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_EFUSE_CMD, EFUSE_READ_CMD | addr);
	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EFUSE_CMD);
	ocp_data = (ocp_data & EFUSE_DATA_BIT16) << 9;	/* data of bit16 */
	ocp_data |= ocp_read_word(tp, MCU_TYPE_PLA, PLA_EFUSE_DATA);

	return ocp_data;
}

static void r8153b_hw_phy_cfg(struct r8152 *tp)
{
	u32 ocp_data, ups_flags = 0;
	u16 data;

	/* disable ALDPS before updating the PHY parameters */
	r8153b_aldps_en(tp, false);

	/* disable EEE before updating the PHY parameters */
	r8153b_eee_en(tp, false);
	ocp_reg_write(tp, OCP_EEE_ADV, 0);

	r8153b_firmware(tp);

	data = sram_read(tp, SRAM_GREEN_CFG);
	data |= GREEN_ETH_EN | R_TUNE_EN;
	sram_write(tp, SRAM_GREEN_CFG, data);
	data = ocp_reg_read(tp, OCP_NCTL_CFG);
	data |= PGA_RETURN_EN;
	ocp_reg_write(tp, OCP_NCTL_CFG, data);
	ups_flags |= UPS_FLAGS_EN_GREEN;

	/* ADC Bias Calibration:
	 * read efuse offset 0x7d to get a 17-bit data. Remove the dummy/fake
	 * bit (bit3) to rebuild the real 16-bit data. Write the data to the
	 * ADC ioffset.
	 */
	ocp_data = r8152_efuse_read(tp, 0x7d);
	data = (u16)(((ocp_data & 0x1fff0) >> 1) | (ocp_data & 0x7));
	if (data != 0xffff)
		ocp_reg_write(tp, OCP_ADC_IOFFSET, data);

	/* ups mode tx-link-pulse timing adjustment:
	 * rg_saw_cnt = OCP reg 0xC426 Bit[13:0]
	 * swr_cnt_1ms_ini = 16000000 / rg_saw_cnt
	 */
	ocp_data = ocp_reg_read(tp, 0xc426);
	ocp_data &= 0x3fff;
	if (ocp_data) {
		u32 swr_cnt_1ms_ini;

		swr_cnt_1ms_ini = (16000000 / ocp_data) & SAW_CNT_1MS_MASK;
		ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_UPS_CFG);
		ocp_data = (ocp_data & ~SAW_CNT_1MS_MASK) | swr_cnt_1ms_ini;
		ocp_write_word(tp, MCU_TYPE_USB, USB_UPS_CFG, ocp_data);
	}

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR);
	ocp_data |= PFM_PWM_SWITCH;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR, ocp_data);

	/* Advnace EEE */
	if (!r8153_patch_request(tp, true)) {
		data = ocp_reg_read(tp, OCP_POWER_CFG);
		data |= EEE_CLKDIV_EN;
		ocp_reg_write(tp, OCP_POWER_CFG, data);

		data = ocp_reg_read(tp, OCP_DOWN_SPEED);
		data |= EN_EEE_CMODE | EN_EEE_1000 | EN_EEE_100 | EN_10M_CLKDIV;
		ocp_reg_write(tp, OCP_DOWN_SPEED, data);

		ocp_reg_write(tp, OCP_SYSCLK_CFG, 0);
		ocp_reg_write(tp, OCP_SYSCLK_CFG, clk_div_expo(5));

		ups_flags |= UPS_FLAGS_EN_10M_CKDIV | UPS_FLAGS_250M_CKDIV |
			     UPS_FLAGS_EN_EEE_CKDIV | UPS_FLAGS_EEE_CMOD_LV_EN |
			     UPS_FLAGS_EEE_PLLOFF_GIGA;

		r8153_patch_request(tp, false);
	}

	r8153b_ups_flags_w0w1(tp, ups_flags, 0);

	r8153b_eee_en(tp, true);
	ocp_reg_write(tp, OCP_EEE_ADV, MDIO_EEE_1000T | MDIO_EEE_100TX);

	r8153b_aldps_en(tp, true);
	r8153b_enable_fc(tp);

	set_bit(PHY_RESET, &tp->flags);
}

static void r8153_first_init(struct r8152 *tp)
{
	u32 ocp_data;
	int i;

	rxdy_gated_en(tp, true);
	r8153_teredo_off(tp);

	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);

	rtl8152_nic_reset(tp);
	rtl_reset_bmu(tp);

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data &= ~NOW_IS_OOB;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data &= ~MCU_BORW_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data |= RE_INIT_LL;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	rtl_rx_vlan_en(tp, tp->netdev->features & NETIF_F_HW_VLAN_CTAG_RX);

	ocp_data = tp->netdev->mtu + VLAN_ETH_HLEN + CRC_SIZE;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, ocp_data);
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_MTPS, MTPS_JUMBO);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR0);
	ocp_data |= TCR0_AUTO_FIFO;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_TCR0, ocp_data);

	rtl8152_nic_reset(tp);

	/* rx share fifo credit full threshold */
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL0, RXFIFO_THR1_NORMAL);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1, RXFIFO_THR2_NORMAL);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2, RXFIFO_THR3_NORMAL);
	/* TX share fifo free credit full threshold */
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_TXFIFO_CTRL, TXFIFO_THR_NORMAL2);

	switch (tp->version) {
	case RTL_VER_08:
	case RTL_VER_09:
		ocp_write_dword(tp, MCU_TYPE_USB, USB_RX_BUF_TH, RX_THR_B);
		break;

	default:
		break;
	}

	/* rx aggregation */
	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_USB_CTRL);
	ocp_data &= ~(RX_AGG_DISABLE | RX_ZERO_EN);
	ocp_write_word(tp, MCU_TYPE_USB, USB_USB_CTRL, ocp_data);
}

static void r8153_enter_oob(struct r8152 *tp)
{
	u32 ocp_data;
	int i;

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data &= ~NOW_IS_OOB;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	rtl_disable(tp);
	rtl_reset_bmu(tp);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data |= RE_INIT_LL;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	for (i = 0; i < 1000; i++) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
		if (ocp_data & LINK_LIST_READY)
			break;
		usleep_range(1000, 2000);
	}

	ocp_data = tp->netdev->mtu + VLAN_ETH_HLEN + CRC_SIZE;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, ocp_data);

	switch (tp->version) {
	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_TEREDO_CFG);
		ocp_data &= ~TEREDO_WAKE_MASK;
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_TEREDO_CFG, ocp_data);
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		/* Clear teredo wake event. bit[15:8] is the teredo wakeup
		 * type. Set it to zero. bits[7:0] are the W1C bits about
		 * the events. Set them to all 1 to clear them.
		 */
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_TEREDO_WAKE_BASE, 0x00ff);
		break;

	default:
		break;
	}

	rtl_rx_vlan_en(tp, true);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PAL_BDC_CR);
	ocp_data |= ALDPS_PROXY_MODE;
	ocp_write_word(tp, MCU_TYPE_PLA, PAL_BDC_CR, ocp_data);

	ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data |= NOW_IS_OOB | DIS_MCU_CLROOB;
	ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	rxdy_gated_en(tp, false);

	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data |= RCR_APM | RCR_AM | RCR_AB;
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);
}

static void rtl8153_disable(struct r8152 *tp)
{
	tp->rtl_ops.aldps_enable(tp, false);
	rtl_disable(tp);
	rtl_reset_bmu(tp);
	tp->rtl_ops.aldps_enable(tp, true);
	usb_enable_lpm(tp->udev);
}

static int rtl8152_set_speed(struct r8152 *tp, u8 autoneg, u16 speed, u8 duplex)
{
	u16 bmcr, anar, gbcr;
	enum spd_duplex speed_duplex;
	int ret = 0;

	anar = r8152_mdio_read(tp, MII_ADVERTISE);
	anar &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL |
		  ADVERTISE_100HALF | ADVERTISE_100FULL);
	if (tp->mii.supports_gmii) {
		gbcr = r8152_mdio_read(tp, MII_CTRL1000);
		gbcr &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
	} else {
		gbcr = 0;
	}

	if (autoneg == AUTONEG_DISABLE) {
		if (speed == SPEED_10) {
			bmcr = 0;
			anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
			speed_duplex = FORCE_10M_HALF;
		} else if (speed == SPEED_100) {
			bmcr = BMCR_SPEED100;
			anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
			speed_duplex = FORCE_100M_HALF;
		} else if (speed == SPEED_1000 && tp->mii.supports_gmii) {
			bmcr = BMCR_SPEED1000;
			gbcr |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;
			speed_duplex = NWAY_1000M_FULL;
		} else {
			ret = -EINVAL;
			goto out;
		}

		if (duplex == DUPLEX_FULL) {
			bmcr |= BMCR_FULLDPLX;
			if (speed != SPEED_1000)
				speed_duplex++;
		}
	} else {
		if (speed == SPEED_10) {
			if (duplex == DUPLEX_FULL) {
				anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
				speed_duplex = NWAY_10M_FULL;
			} else {
				anar |= ADVERTISE_10HALF;
				speed_duplex = NWAY_10M_HALF;
			}
		} else if (speed == SPEED_100) {
			if (duplex == DUPLEX_FULL) {
				anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
				anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
				speed_duplex = NWAY_100M_FULL;
			} else {
				anar |= ADVERTISE_10HALF;
				anar |= ADVERTISE_100HALF;
				speed_duplex = NWAY_100M_HALF;
			}
		} else if (speed == SPEED_1000 && tp->mii.supports_gmii) {
			if (duplex == DUPLEX_FULL) {
				anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
				anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
				gbcr |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;
			} else {
				anar |= ADVERTISE_10HALF;
				anar |= ADVERTISE_100HALF;
				gbcr |= ADVERTISE_1000HALF;
			}
			speed_duplex = NWAY_1000M_FULL;
		} else {
			ret = -EINVAL;
			goto out;
		}

		bmcr = BMCR_ANENABLE | BMCR_ANRESTART;
	}

	if (test_and_clear_bit(PHY_RESET, &tp->flags))
		bmcr |= BMCR_RESET;

	if (tp->mii.supports_gmii)
		r8152_mdio_write(tp, MII_CTRL1000, gbcr);

	r8152_mdio_write(tp, MII_ADVERTISE, anar);
	r8152_mdio_write(tp, MII_BMCR, bmcr);

	switch (tp->version) {
	case RTL_VER_08:
	case RTL_VER_09:
		r8153b_ups_flags_w0w1(tp, ups_flags_speed(speed_duplex),
				      UPS_FLAGS_SPEED_MASK);
		break;

	default:
		break;
	}

	if (bmcr & BMCR_RESET) {
		int i;

		for (i = 0; i < 50; i++) {
			msleep(20);
			if ((r8152_mdio_read(tp, MII_BMCR) & BMCR_RESET) == 0)
				break;
		}
	}

out:
	return ret;
}

static void rtl8152_up(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	r8152_aldps_en(tp, false);
	r8152b_exit_oob(tp);
	r8152_aldps_en(tp, true);
}

static void rtl8152_down(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags)) {
		rtl_drop_queued_tx(tp);
		return;
	}

	r8152_power_cut_en(tp, false);
	r8152_aldps_en(tp, false);
	r8152b_enter_oob(tp);
	r8152_aldps_en(tp, true);
	if (tp->version == RTL_VER_01)
		rtl8152_set_speed(tp, AUTONEG_ENABLE, SPEED_10, DUPLEX_FULL);
}

static void rtl8153_up(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	tp->rtl_ops.u1u2_enable(tp, false);
	tp->rtl_ops.aldps_enable(tp, false);
	r8153_first_init(tp);
	tp->rtl_ops.aldps_enable(tp, true);
	tp->rtl_ops.u2p3_enable(tp, true);
	tp->rtl_ops.u1u2_enable(tp, true);
	usb_enable_lpm(tp->udev);
}

static void rtl8153_down(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags)) {
		rtl_drop_queued_tx(tp);
		return;
	}

	tp->rtl_ops.u1u2_enable(tp, false);
	tp->rtl_ops.u2p3_enable(tp, false);
	tp->rtl_ops.power_cut_en(tp, false);
	tp->rtl_ops.aldps_enable(tp, false);
	r8153_enter_oob(tp);
	tp->rtl_ops.aldps_enable(tp, true);
}

static bool rtl8152_in_nway(struct r8152 *tp)
{
	u16 nway_state;

	ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE, 0x2000);
	tp->ocp_base = 0x2000;
	ocp_write_byte(tp, MCU_TYPE_PLA, 0xb014, 0x4c);		/* phy state */
	nway_state = ocp_read_word(tp, MCU_TYPE_PLA, 0xb01a);

	/* bit 15: TXDIS_STATE, bit 14: ABD_STATE */
	if (nway_state & 0xc000)
		return false;
	else
		return true;
}

static bool rtl8153_in_nway(struct r8152 *tp)
{
	u16 phy_state = ocp_reg_read(tp, OCP_PHY_STATE) & 0xff;

	if (phy_state == TXDIS_STATE || phy_state == ABD_STATE)
		return false;
	else
		return true;
}

static void set_carrier(struct r8152 *tp)
{
	struct net_device *netdev = tp->netdev;
	u8 speed;

	speed = rtl8152_get_speed(tp);

	if (speed & LINK_STATUS) {
		if (!netif_carrier_ok(netdev)) {
			tp->rtl_ops.enable(tp);
			set_bit(RTL8152_SET_RX_MODE, &tp->flags);
			napi_disable(&tp->napi);
			netif_carrier_on(netdev);
			rtl_start_rx(tp);
			napi_enable(&tp->napi);
			pr_info("r8152 carrier on\n");
		}
	} else {
		if (netif_carrier_ok(netdev)) {
			netif_carrier_off(netdev);
			napi_disable(&tp->napi);
			tp->rtl_ops.disable(tp);
			napi_enable(&tp->napi);
			pr_info("r8152 carrier off\n");
		}
	}
}

static inline void __rtl_work_func(struct r8152 *tp)
{
	/* If the device is unplugged or !netif_running(), the workqueue
	 * doesn't need to wake the device, and could return directly.
	 */
	if (test_bit(RTL8152_UNPLUG, &tp->flags) || !netif_running(tp->netdev))
		return;

	if (usb_autopm_get_interface(tp->intf) < 0)
		return;

	if (!test_bit(WORK_ENABLE, &tp->flags))
		goto out1;

	if (!mutex_trylock(&tp->control)) {
		schedule_delayed_work(&tp->schedule, 0);
		goto out1;
	}

	if (test_and_clear_bit(RTL8152_LINK_CHG, &tp->flags))
		set_carrier(tp);

	if (test_bit(RTL8152_SET_RX_MODE, &tp->flags))
		_rtl8152_set_rx_mode(tp->netdev);

	/* don't schedule napi before linking */
	if (test_and_clear_bit(SCHEDULE_NAPI, &tp->flags) &&
	    netif_carrier_ok(tp->netdev))
		napi_schedule(&tp->napi);

	mutex_unlock(&tp->control);

out1:
	usb_autopm_put_interface(tp->intf);
}

static inline void __rtl_hw_phy_work_func(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	if (usb_autopm_get_interface(tp->intf) < 0)
		return;

	mutex_lock(&tp->control);

	tp->rtl_ops.hw_phy_cfg(tp);

	rtl8152_set_speed(tp, tp->autoneg, tp->speed, tp->duplex);

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)

static void rtl_work_func_t(void *data)
{
	struct r8152 *tp = (struct r8152 *)data;

	__rtl_work_func(tp);
}

static void rtl_hw_phy_work_func_t(void *data)
{
	struct r8152 *tp = (struct r8152 *)data;

	__rtl_hw_phy_work_func(tp);
}

#else

static void rtl_work_func_t(struct work_struct *work)
{
	struct r8152 *tp = container_of(work, struct r8152, schedule.work);

	__rtl_work_func(tp);
}

static void rtl_hw_phy_work_func_t(struct work_struct *work)
{
	struct r8152 *tp = container_of(work, struct r8152, hw_phy_work.work);

	__rtl_hw_phy_work_func(tp);
}

#endif

static int rtk_disable_diag(struct r8152 *tp)
{
	tp->rtk_enable_diag--;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE, tp->ocp_base);
	netif_info(tp, drv, tp->netdev, "disable rtk diag %d\n",
		   tp->rtk_enable_diag);
	mutex_unlock(&tp->control);
	usb_autopm_put_interface(tp->intf);

	return 0;
}

static int rtl8152_open(struct net_device *netdev)
{
	struct r8152 *tp = netdev_priv(netdev);
	int res = 0;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	res = alloc_all_mem(tp);
	if (res)
		goto out;
	pr_info("%s : memory allocation success!\n", __func__);

	res = usb_autopm_get_interface(tp->intf);
	if (res < 0) {
		free_all_mem(tp);
		goto out;
	}

	mutex_lock(&tp->control);

	tp->rtl_ops.up(tp);

	netif_carrier_off(netdev);
	netif_start_queue(netdev);
	set_bit(WORK_ENABLE, &tp->flags);

	res = usb_submit_urb(tp->intr_urb, GFP_KERNEL);
	if (res) {
		if (res == -ENODEV)
			netif_device_detach(tp->netdev);
		netif_warn(tp, ifup, netdev, "intr_urb submit failed: %d\n",
			   res);
		free_all_mem(tp);
	} else {
		napi_enable(&tp->napi);
	}

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	pr_info("%s : end of function!\n", __func__);
	return res;
}

static int rtl8152_close(struct net_device *netdev)
{
	struct r8152 *tp = netdev_priv(netdev);
	int res = 0;
	int timeleft = -1;

	clear_bit(WORK_ENABLE, &tp->flags);
	usb_kill_urb(tp->intr_urb);
	cancel_delayed_work_sync(&tp->schedule);
	netif_stop_queue(netdev);

	if (unlikely(tp->rtk_enable_diag)) {
		netif_err(tp, drv, tp->netdev, "rtk diag isn't disabled\n");
		rtk_disable_diag(tp);
	}
	napi_disable(&tp->napi);
	pr_info("%s : napi is disabled\n", __func__);

	res = usb_autopm_get_interface(tp->intf);
	if (res < 0 || test_bit(RTL8152_UNPLUG, &tp->flags)) {
		rtl_drop_queued_tx(tp);
		rtl_stop_rx(tp);
	} else {
		mutex_lock(&tp->control);

		tp->rtl_ops.down(tp);
#if defined(RTL8152_S5_WOL) && defined(CONFIG_PM)
		res = rtl_s5_wol(tp);
#endif
		mutex_unlock(&tp->control);

		usb_autopm_put_interface(tp->intf);
	}
	timeleft = wait_event_interruptible_timeout(tp->bottom_half_wait_q,
					tp->bottom_half_event, (tp->bottom_half_wait_time)*HZ);
	pr_info("%s : wait for end of rx_bottom , timeleft = %d\n", __func__, timeleft);

	free_all_mem(tp);

	return res;
}

static void rtl_tally_reset(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_RSTTALLY);
	ocp_data |= TALLY_RESET;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_RSTTALLY, ocp_data);
}

static void r8152b_init(struct r8152 *tp)
{
	u32 ocp_data;
	u16 data;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

#if 0
	/* Clear EP3 Fifo before using interrupt transfer */
	if (ocp_read_byte(tp, MCU_TYPE_USB, 0xb963) & 0x80) {
		ocp_write_byte(tp, MCU_TYPE_USB, 0xb963, 0x08);
		ocp_write_byte(tp, MCU_TYPE_USB, 0xb963, 0x40);
		ocp_write_byte(tp, MCU_TYPE_USB, 0xb963, 0x00);
		ocp_write_byte(tp, MCU_TYPE_USB, 0xb968, 0x00);
		ocp_write_word(tp, MCU_TYPE_USB, 0xb010, 0x00e0);
		ocp_write_byte(tp, MCU_TYPE_USB, 0xb963, 0x04);
	}
#endif

	data = r8152_mdio_read(tp, MII_BMCR);
	if (data & BMCR_PDOWN) {
		data &= ~BMCR_PDOWN;
		r8152_mdio_write(tp, MII_BMCR, data);
	}

	r8152_aldps_en(tp, false);

	if (tp->version == RTL_VER_01) {
		ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE);
		ocp_data &= ~LED_MODE_MASK;
		ocp_write_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE, ocp_data);
	}

	r8152_power_cut_en(tp, false);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR);
	ocp_data |= TX_10M_IDLE_EN | PFM_PWM_SWITCH;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR, ocp_data);
	ocp_data = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL);
	ocp_data &= ~MCU_CLK_RATIO_MASK;
	ocp_data |= MCU_CLK_RATIO | D3_CLK_GATED_EN;
	ocp_write_dword(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL, ocp_data);
	ocp_data = GPHY_STS_MSK | SPEED_DOWN_MSK |
		   SPDWN_RXDV_MSK | SPDWN_LINKCHG_MSK;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_GPHY_INTR_IMR, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_USB_TIMER);
	ocp_data |= BIT(15);
	ocp_write_word(tp, MCU_TYPE_USB, USB_USB_TIMER, ocp_data);
	ocp_write_word(tp, MCU_TYPE_USB, 0xcbfc, 0x03e8);
	ocp_data &= ~BIT(15);
	ocp_write_word(tp, MCU_TYPE_USB, USB_USB_TIMER, ocp_data);

	rtl_tally_reset(tp);

	/* enable rx aggregation */
	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_USB_CTRL);
	ocp_data &= ~(RX_AGG_DISABLE | RX_ZERO_EN);
	ocp_write_word(tp, MCU_TYPE_USB, USB_USB_CTRL, ocp_data);
}

static void r8153_init(struct r8152 *tp)
{
	u32 ocp_data;
	u16 data;
	int i;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	r8153_u1u2en(tp, false);

	for (i = 0; i < 500; i++) {
		if (ocp_read_word(tp, MCU_TYPE_PLA, PLA_BOOT_CTRL) &
		    AUTOLOAD_DONE)
			break;
		msleep(20);
	}

	for (i = 0; i < 500; i++) {
		ocp_data = ocp_reg_read(tp, OCP_PHY_STATUS) & PHY_STAT_MASK;
		if (ocp_data == PHY_STAT_LAN_ON || ocp_data == PHY_STAT_PWRDN)
			break;
		msleep(20);
	}

	if (tp->version == RTL_VER_03 || tp->version == RTL_VER_04 ||
	    tp->version == RTL_VER_05)
		ocp_reg_write(tp, OCP_ADC_CFG, CKADSEL_L | ADC_EN | EN_EMI_L);

	data = r8152_mdio_read(tp, MII_BMCR);
	if (data & BMCR_PDOWN) {
		data &= ~BMCR_PDOWN;
		r8152_mdio_write(tp, MII_BMCR, data);
	}

	for (i = 0; i < 500; i++) {
		ocp_data = ocp_reg_read(tp, OCP_PHY_STATUS) & PHY_STAT_MASK;
		if (ocp_data == PHY_STAT_LAN_ON)
			break;
		msleep(20);
	}

	usb_disable_lpm(tp->udev);
	r8153_u2p3en(tp, false);

	if (tp->version == RTL_VER_04) {
		ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_SSPHYLINK2);
		ocp_data &= ~pwd_dn_scale_mask;
		ocp_data |= pwd_dn_scale(96);
		ocp_write_word(tp, MCU_TYPE_USB, USB_SSPHYLINK2, ocp_data);

		ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, USB_USB2PHY);
		ocp_data |= USB2PHY_L1 | USB2PHY_SUSPEND;
		ocp_write_byte(tp, MCU_TYPE_USB, USB_USB2PHY, ocp_data);
	} else if (tp->version == RTL_VER_05) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA, PLA_DMY_REG0);
		ocp_data &= ~ECM_ALDPS;
		ocp_write_byte(tp, MCU_TYPE_PLA, PLA_DMY_REG0, ocp_data);

		ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY1);
		if (ocp_read_word(tp, MCU_TYPE_USB, USB_BURST_SIZE) == 0)
			ocp_data &= ~DYNAMIC_BURST;
		else
			ocp_data |= DYNAMIC_BURST;
		ocp_write_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY1, ocp_data);
	} else if (tp->version == RTL_VER_06) {
		ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY1);
		if (ocp_read_word(tp, MCU_TYPE_USB, USB_BURST_SIZE) == 0)
			ocp_data &= ~DYNAMIC_BURST;
		else
			ocp_data |= DYNAMIC_BURST;
		ocp_write_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY1, ocp_data);
	}

	ocp_data = ocp_read_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY2);
	ocp_data |= EP4_FULL_FC;
	ocp_write_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY2, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_WDT11_CTRL);
	ocp_data &= ~TIMER11_EN;
	ocp_write_word(tp, MCU_TYPE_USB, USB_WDT11_CTRL, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE);
	ocp_data &= ~LED_MODE_MASK;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE, ocp_data);

	ocp_data = FIFO_EMPTY_1FB | ROK_EXIT_LPM;
	if (tp->version == RTL_VER_04 && tp->udev->speed < USB_SPEED_SUPER)
		ocp_data |= LPM_TIMER_500MS;
	else
		ocp_data |= LPM_TIMER_500US;
	ocp_write_byte(tp, MCU_TYPE_USB, USB_LPM_CTRL, ocp_data);

	ocp_data = ocp_read_word(tp, MCU_TYPE_USB, USB_AFE_CTRL2);
	ocp_data &= ~SEN_VAL_MASK;
	ocp_data |= SEN_VAL_NORMAL | SEL_RXIDLE;
	ocp_write_word(tp, MCU_TYPE_USB, USB_AFE_CTRL2, ocp_data);

	ocp_write_word(tp, MCU_TYPE_USB, USB_CONNECT_TIMER, 0x0001);

	r8153_power_cut_en(tp, false);
	r8153_u1u2en(tp, true);

	/* MAC clock speed down */
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL, 0);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL2, 0);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL3, 0);
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL4, 0);

	rtl_tally_reset(tp);
	r8153_u2p3en(tp, true);

	switch (tp->udev->speed) {
	case USB_SPEED_SUPER:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
	case USB_SPEED_SUPER_PLUS:
#endif
		tp->coalesce = COALESCE_SUPER;
		break;
	case USB_SPEED_HIGH:
		tp->coalesce = COALESCE_HIGH;
		break;
	default:
		tp->coalesce = COALESCE_SLOW;
		break;
	}
}

static void r8153b_init(struct r8152 *tp)
{
	u32 ocp_data;
	u16 data;
	int i;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	r8153b_u1u2en(tp, false);

	for (i = 0; i < 500; i++) {
		if (ocp_read_word(tp, MCU_TYPE_PLA, PLA_BOOT_CTRL) &
		    AUTOLOAD_DONE)
			break;
		msleep(20);
	}

	for (i = 0; i < 500; i++) {
		ocp_data = ocp_reg_read(tp, OCP_PHY_STATUS) & PHY_STAT_MASK;
		if (ocp_data == PHY_STAT_LAN_ON || ocp_data == PHY_STAT_PWRDN)
			break;
		msleep(20);
	}

	data = r8152_mdio_read(tp, MII_BMCR);
	if (data & BMCR_PDOWN) {
		data &= ~BMCR_PDOWN;
		r8152_mdio_write(tp, MII_BMCR, data);
	}

	for (i = 0; i < 500; i++) {
		ocp_data = ocp_reg_read(tp, OCP_PHY_STATUS) & PHY_STAT_MASK;
		if (ocp_data == PHY_STAT_LAN_ON)
			break;
		msleep(20);
	}

	r8153_u2p3en(tp, false);

	/* MSC timer = 0xfff * 8ms = 32760 ms */
	ocp_write_word(tp, MCU_TYPE_USB, USB_MSC_TIMER, 0x0fff);

	/* U1/U2/L1 idle timer. 500 us */
	ocp_write_word(tp, MCU_TYPE_USB, USB_U1U2_TIMER, 500);

	r8153B_power_cut_en(tp, false);
	r8153b_ups_en(tp, false);
	r8153b_queue_wake(tp, false);
	r8153b_u1u2en(tp, true);

	/* MAC clock speed down */
	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL2);
	ocp_data |= MAC_CLK_SPDWN_EN;
	ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL2, ocp_data);

	rtl_tally_reset(tp);

	tp->coalesce = 15000;	/* 15 us */
}

static bool rtl_vendor_mode(struct usb_interface *intf)
{
	struct usb_host_interface *alt = intf->cur_altsetting;

	return alt->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC;
}

static int rtl8152_pre_reset(struct usb_interface *intf)
{
	struct r8152 *tp = usb_get_intfdata(intf);
	struct net_device *netdev;

	if (!tp)
		return 0;

	netdev = tp->netdev;
	if (!netif_running(netdev))
		return 0;

	napi_disable(&tp->napi);
	clear_bit(WORK_ENABLE, &tp->flags);
	usb_kill_urb(tp->intr_urb);
	cancel_delayed_work_sync(&tp->schedule);
	if (netif_carrier_ok(netdev)) {
		netif_stop_queue(netdev);
		mutex_lock(&tp->control);
		tp->rtl_ops.disable(tp);
		mutex_unlock(&tp->control);
	}

	return 0;
}

static int rtl8152_post_reset(struct usb_interface *intf)
{
	struct r8152 *tp = usb_get_intfdata(intf);
	struct net_device *netdev;

	if (!tp)
		return 0;

	netdev = tp->netdev;
	if (!netif_running(netdev))
		return 0;

	set_bit(WORK_ENABLE, &tp->flags);
	if (netif_carrier_ok(netdev)) {
		mutex_lock(&tp->control);
		tp->rtl_ops.enable(tp);
		rtl8152_set_rx_mode(netdev);
		mutex_unlock(&tp->control);
		netif_wake_queue(netdev);
	}

	napi_enable(&tp->napi);

	return 0;
}

static bool delay_autosuspend(struct r8152 *tp)
{
	bool sw_linking = !!netif_carrier_ok(tp->netdev);
	bool hw_linking = !!(rtl8152_get_speed(tp) & LINK_STATUS);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
#define work_busy(x)	0
#endif
	/* This means a linking change occurs and the driver doesn't detect it,
	 * yet. If the driver has disabled tx/rx and hw is linking on, the
	 * device wouldn't wake up by receiving any packet.
	 */
	if (work_busy(&tp->schedule.work) || sw_linking != hw_linking)
		return true;

	/* If the linking down is occurred by nway, the device may miss the
	 * linking change event. And it wouldn't wake when linking on.
	 */
	if (!sw_linking && tp->rtl_ops.in_nway(tp))
		return true;
	else
		return false;
}

static int rtl8152_rumtime_suspend(struct r8152 *tp)
{
	struct net_device *netdev = tp->netdev;
	int ret = 0;

	if (netif_running(netdev) && test_bit(WORK_ENABLE, &tp->flags)) {
		u32 rcr = 0;

		if (delay_autosuspend(tp)) {
			ret = -EBUSY;
			goto out1;
		}

		if (netif_carrier_ok(netdev)) {
			u32 ocp_data;

			rcr = ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
			ocp_data = rcr & ~RCR_ACPT_ALL;
			ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);
			rxdy_gated_en(tp, true);
			ocp_data = ocp_read_byte(tp, MCU_TYPE_PLA,
						 PLA_OOB_CTRL);
			if (!(ocp_data & RXFIFO_EMPTY)) {
				rxdy_gated_en(tp, false);
				ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, rcr);
				ret = -EBUSY;
				goto out1;
			}
		}

		clear_bit(WORK_ENABLE, &tp->flags);
		usb_kill_urb(tp->intr_urb);

		tp->rtl_ops.autosuspend_en(tp, true);

		if (netif_carrier_ok(netdev)) {
			napi_disable(&tp->napi);
			rtl_stop_rx(tp);
			rxdy_gated_en(tp, false);
			ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, rcr);
			napi_enable(&tp->napi);
		}
	}

	set_bit(SELECTIVE_SUSPEND, &tp->flags);

out1:
	return ret;
}

static int rtl8152_system_suspend(struct r8152 *tp)
{
	struct net_device *netdev = tp->netdev;
	int ret = 0;

	netif_device_detach(netdev);

	if (netif_running(netdev) && test_bit(WORK_ENABLE, &tp->flags)) {
		clear_bit(WORK_ENABLE, &tp->flags);
		usb_kill_urb(tp->intr_urb);
		napi_disable(&tp->napi);
		cancel_delayed_work_sync(&tp->schedule);
		tp->rtl_ops.down(tp);
		napi_enable(&tp->napi);
	}

	return ret;
}

static int rtl8152_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct r8152 *tp = usb_get_intfdata(intf);
	int ret;

	mutex_lock(&tp->control);

	if (PMSG_IS_AUTO(message))
		ret = rtl8152_rumtime_suspend(tp);
	else
		ret = rtl8152_system_suspend(tp);

	mutex_unlock(&tp->control);

	return ret;
}

static int rtl8152_resume(struct usb_interface *intf)
{
	struct r8152 *tp = usb_get_intfdata(intf);

	mutex_lock(&tp->control);

	if (!test_bit(SELECTIVE_SUSPEND, &tp->flags))
		netif_device_attach(tp->netdev);

	if (netif_running(tp->netdev) && tp->netdev->flags & IFF_UP) {
		if (test_bit(SELECTIVE_SUSPEND, &tp->flags)) {
			tp->rtl_ops.autosuspend_en(tp, false);
			clear_bit(SELECTIVE_SUSPEND, &tp->flags);
			napi_disable(&tp->napi);
			set_bit(WORK_ENABLE, &tp->flags);
			if (netif_carrier_ok(tp->netdev))
				rtl_start_rx(tp);
			napi_enable(&tp->napi);
		} else {
			tp->rtl_ops.up(tp);
			netif_carrier_off(tp->netdev);
			set_bit(WORK_ENABLE, &tp->flags);
		}
		usb_submit_urb(tp->intr_urb, GFP_KERNEL);
	} else if (test_bit(SELECTIVE_SUSPEND, &tp->flags)) {
		if (tp->netdev->flags & IFF_UP)
			tp->rtl_ops.autosuspend_en(tp, false);
		clear_bit(SELECTIVE_SUSPEND, &tp->flags);
	}

	mutex_unlock(&tp->control);

	return 0;
}

static int rtl8152_reset_resume(struct usb_interface *intf)
{
	struct r8152 *tp = usb_get_intfdata(intf);

	clear_bit(SELECTIVE_SUSPEND, &tp->flags);
	tp->rtl_ops.init(tp);
	queue_delayed_work(system_long_wq, &tp->hw_phy_work, 0);
	set_ethernet_addr(tp);
	return rtl8152_resume(intf);
}

static void rtl8152_get_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	struct r8152 *tp = netdev_priv(dev);

	if (unlikely(tp->rtk_enable_diag))
		return;

	if (usb_autopm_get_interface(tp->intf) < 0)
		return;

	if (!rtl_can_wakeup(tp)) {
		wol->supported = 0;
		wol->wolopts = 0;
	} else {
		mutex_lock(&tp->control);
		wol->supported = WAKE_ANY;
		wol->wolopts = __rtl_get_wol(tp);
		mutex_unlock(&tp->control);
	}

	usb_autopm_put_interface(tp->intf);
}

static int rtl8152_set_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	struct r8152 *tp = netdev_priv(dev);
	int ret;

	if (!rtl_can_wakeup(tp))
		return -EOPNOTSUPP;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out_set_wol;

	mutex_lock(&tp->control);

	__rtl_set_wol(tp, wol->wolopts);
	tp->saved_wolopts = wol->wolopts & WAKE_ANY;

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out_set_wol:
	return ret;
}

static u32 rtl8152_get_msglevel(struct net_device *dev)
{
	struct r8152 *tp = netdev_priv(dev);

	return tp->msg_enable;
}

static void rtl8152_set_msglevel(struct net_device *dev, u32 value)
{
	struct r8152 *tp = netdev_priv(dev);

	tp->msg_enable = value;
}

static void rtl8152_get_drvinfo(struct net_device *netdev,
				struct ethtool_drvinfo *info)
{
	struct r8152 *tp = netdev_priv(netdev);

	strlcpy(info->driver, MODULENAME, sizeof(info->driver));
	strlcpy(info->version, DRIVER_VERSION, sizeof(info->version));
	usb_make_path(tp->udev, info->bus_info, sizeof(info->bus_info));
}

static
int rtl8152_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct r8152 *tp = netdev_priv(netdev);
	u16 bmcr, bmsr, ctrl1000 = 0, stat1000 = 0;
	int ret, advert;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	cmd->supported =
	    (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
	     SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
	     SUPPORTED_Autoneg | SUPPORTED_MII);
	if (tp->mii.supports_gmii)
		cmd->supported |= SUPPORTED_1000baseT_Full;

	/* only supports twisted-pair */
	cmd->port = PORT_MII;

	/* only supports internal transceiver */
	cmd->transceiver = XCVR_INTERNAL;
	cmd->phy_address = 32;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 30)
	cmd->mdio_support = ETH_MDIO_SUPPORTS_C22;
#endif
	cmd->advertising = ADVERTISED_MII;

	mutex_lock(&tp->control);

	bmcr = r8152_mdio_read(tp, MII_BMCR);
	bmsr = r8152_mdio_read(tp, MII_BMSR);
	if (tp->mii.supports_gmii) {
		ctrl1000 = r8152_mdio_read(tp, MII_CTRL1000);
		stat1000 = r8152_mdio_read(tp, MII_STAT1000);
	}

	advert = r8152_mdio_read(tp, MII_ADVERTISE);
	if (advert & ADVERTISE_10HALF)
		cmd->advertising |= ADVERTISED_10baseT_Half;
	if (advert & ADVERTISE_10FULL)
		cmd->advertising |= ADVERTISED_10baseT_Full;
	if (advert & ADVERTISE_100HALF)
		cmd->advertising |= ADVERTISED_100baseT_Half;
	if (advert & ADVERTISE_100FULL)
		cmd->advertising |= ADVERTISED_100baseT_Full;
	if (advert & ADVERTISE_PAUSE_CAP)
		cmd->advertising |= ADVERTISED_Pause;
	if (advert & ADVERTISE_PAUSE_ASYM)
		cmd->advertising |= ADVERTISED_Asym_Pause;
	if (tp->mii.supports_gmii) {
		if (ctrl1000 & ADVERTISE_1000HALF)
			cmd->advertising |= ADVERTISED_1000baseT_Half;
		if (ctrl1000 & ADVERTISE_1000FULL)
			cmd->advertising |= ADVERTISED_1000baseT_Full;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	if (bmsr & BMSR_ANEGCOMPLETE) {
		advert = r8152_mdio_read(tp, MII_LPA);
		if (advert & LPA_LPACK)
			cmd->lp_advertising |= ADVERTISED_Autoneg;
		if (advert & ADVERTISE_10HALF)
			cmd->lp_advertising |=
				ADVERTISED_10baseT_Half;
		if (advert & ADVERTISE_10FULL)
			cmd->lp_advertising |=
				ADVERTISED_10baseT_Full;
		if (advert & ADVERTISE_100HALF)
			cmd->lp_advertising |=
				ADVERTISED_100baseT_Half;
		if (advert & ADVERTISE_100FULL)
			cmd->lp_advertising |=
				ADVERTISED_100baseT_Full;

		if (tp->mii.supports_gmii) {
			if (stat1000 & LPA_1000HALF)
				cmd->lp_advertising |=
					ADVERTISED_1000baseT_Half;
			if (stat1000 & LPA_1000FULL)
				cmd->lp_advertising |=
					ADVERTISED_1000baseT_Full;
		}
	} else {
		cmd->lp_advertising = 0;
	}
#endif

	if (bmcr & BMCR_ANENABLE) {
		cmd->advertising |= ADVERTISED_Autoneg;
		cmd->autoneg = AUTONEG_ENABLE;
	} else {
		cmd->autoneg = AUTONEG_DISABLE;
	}

	if (netif_carrier_ok(tp->netdev)) {
		u8 speed = rtl8152_get_speed(tp);

		if (speed & _100bps)
			cmd->speed = SPEED_100;
		else if (speed & _10bps)
			cmd->speed = SPEED_10;
		else if (tp->mii.supports_gmii && (speed & _1000bps))
			cmd->speed = SPEED_1000;

		cmd->duplex = (speed & FULL_DUP) ? DUPLEX_FULL : DUPLEX_HALF;
	} else {
		cmd->speed = 0;
		cmd->duplex = 255;
	}

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}

static int rtl8152_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct r8152 *tp = netdev_priv(dev);
	int ret;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	mutex_lock(&tp->control);

	ret = rtl8152_set_speed(tp, cmd->autoneg, cmd->speed, cmd->duplex);
	if (!ret) {
		tp->autoneg = cmd->autoneg;
		tp->speed = cmd->speed;
		tp->duplex = cmd->duplex;
	}

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}

static const char rtl8152_gstrings[][ETH_GSTRING_LEN] = {
	"tx_packets",
	"rx_packets",
	"tx_errors",
	"rx_errors",
	"rx_missed",
	"align_errors",
	"tx_single_collisions",
	"tx_multi_collisions",
	"rx_unicast",
	"rx_broadcast",
	"rx_multicast",
	"tx_aborted",
	"tx_underrun",
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
static int rtl8152_get_sset_count(struct net_device *dev)
{
	return ARRAY_SIZE(rtl8152_gstrings);
}
#else
static int rtl8152_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(rtl8152_gstrings);
	default:
		return -EOPNOTSUPP;
	}
}
#endif

static void rtl8152_get_ethtool_stats(struct net_device *dev,
				      struct ethtool_stats *stats, u64 *data)
{
	struct r8152 *tp = netdev_priv(dev);
	struct tally_counter tally;

	if (unlikely(tp->rtk_enable_diag))
		return;

	if (usb_autopm_get_interface(tp->intf) < 0)
		return;

	generic_ocp_read(tp, PLA_TALLYCNT, sizeof(tally), &tally, MCU_TYPE_PLA);

	usb_autopm_put_interface(tp->intf);

	data[0] = le64_to_cpu(tally.tx_packets);
	data[1] = le64_to_cpu(tally.rx_packets);
	data[2] = le64_to_cpu(tally.tx_errors);
	data[3] = le32_to_cpu(tally.rx_errors);
	data[4] = le16_to_cpu(tally.rx_missed);
	data[5] = le16_to_cpu(tally.align_errors);
	data[6] = le32_to_cpu(tally.tx_one_collision);
	data[7] = le32_to_cpu(tally.tx_multi_collision);
	data[8] = le64_to_cpu(tally.rx_unicast);
	data[9] = le64_to_cpu(tally.rx_broadcast);
	data[10] = le32_to_cpu(tally.rx_multicast);
	data[11] = le16_to_cpu(tally.tx_aborted);
	data[12] = le16_to_cpu(tally.tx_underrun);
}

static void rtl8152_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	switch (stringset) {
	case ETH_SS_STATS:
		memcpy(data, *rtl8152_gstrings, sizeof(rtl8152_gstrings));
		break;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
static int r8152_get_eee(struct r8152 *tp, struct ethtool_eee *eee)
{
	u32 ocp_data, lp, adv, supported = 0;
	u16 val;

	val = r8152_mmd_read(tp, MDIO_MMD_PCS, MDIO_PCS_EEE_ABLE);
	supported = mmd_eee_cap_to_ethtool_sup_t(val);

	val = r8152_mmd_read(tp, MDIO_MMD_AN, MDIO_AN_EEE_ADV);
	adv = mmd_eee_adv_to_ethtool_adv_t(val);

	val = r8152_mmd_read(tp, MDIO_MMD_AN, MDIO_AN_EEE_LPABLE);
	lp = mmd_eee_adv_to_ethtool_adv_t(val);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEE_CR);
	ocp_data &= EEE_RX_EN | EEE_TX_EN;

	eee->eee_enabled = !!ocp_data;
	eee->eee_active = !!(supported & adv & lp);
	eee->supported = supported;
	eee->advertised = adv;
	eee->lp_advertised = lp;

	return 0;
}

static int r8152_set_eee(struct r8152 *tp, struct ethtool_eee *eee)
{
	u16 val = ethtool_adv_to_mmd_eee_adv_t(eee->advertised);

	r8152_eee_en(tp, eee->eee_enabled);

	if (!eee->eee_enabled)
		val = 0;

	r8152_mmd_write(tp, MDIO_MMD_AN, MDIO_AN_EEE_ADV, val);

	return 0;
}

static int r8153_get_eee(struct r8152 *tp, struct ethtool_eee *eee)
{
	u32 ocp_data, lp, adv, supported = 0;
	u16 val;

	val = ocp_reg_read(tp, OCP_EEE_ABLE);
	supported = mmd_eee_cap_to_ethtool_sup_t(val);

	val = ocp_reg_read(tp, OCP_EEE_ADV);
	adv = mmd_eee_adv_to_ethtool_adv_t(val);

	val = ocp_reg_read(tp, OCP_EEE_LPABLE);
	lp = mmd_eee_adv_to_ethtool_adv_t(val);

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEE_CR);
	ocp_data &= EEE_RX_EN | EEE_TX_EN;

	eee->eee_enabled = !!ocp_data;
	eee->eee_active = !!(supported & adv & lp);
	eee->supported = supported;
	eee->advertised = adv;
	eee->lp_advertised = lp;

	return 0;
}

static int r8153_set_eee(struct r8152 *tp, struct ethtool_eee *eee)
{
	u16 val = ethtool_adv_to_mmd_eee_adv_t(eee->advertised);

	r8153_eee_en(tp, eee->eee_enabled);

	if (!eee->eee_enabled)
		val = 0;

	ocp_reg_write(tp, OCP_EEE_ADV, val);

	return 0;
}

static int r8153b_set_eee(struct r8152 *tp, struct ethtool_eee *eee)
{
	u16 val = ethtool_adv_to_mmd_eee_adv_t(eee->advertised);

	r8153b_eee_en(tp, eee->eee_enabled);

	if (!eee->eee_enabled)
		val = 0;

	ocp_reg_write(tp, OCP_EEE_ADV, val);

	return 0;
}

static int
rtl_ethtool_get_eee(struct net_device *net, struct ethtool_eee *edata)
{
	struct r8152 *tp = netdev_priv(net);
	int ret;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	mutex_lock(&tp->control);

	ret = tp->rtl_ops.eee_get(tp, edata);

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}

static int
rtl_ethtool_set_eee(struct net_device *net, struct ethtool_eee *edata)
{
	struct r8152 *tp = netdev_priv(net);
	int ret;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	mutex_lock(&tp->control);

	ret = tp->rtl_ops.eee_set(tp, edata);
	if (!ret)
		ret = mii_nway_restart(&tp->mii);

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */

static int rtl8152_nway_reset(struct net_device *dev)
{
	struct r8152 *tp = netdev_priv(dev);
	int ret;

	if (unlikely(tp->rtk_enable_diag))
		return -EBUSY;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	mutex_lock(&tp->control);

	ret = mii_nway_restart(&tp->mii);

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}

static int rtl8152_get_coalesce(struct net_device *netdev,
				struct ethtool_coalesce *coalesce)
{
	struct r8152 *tp = netdev_priv(netdev);

	switch (tp->version) {
	case RTL_VER_01:
	case RTL_VER_02:
	case RTL_VER_07:
		return -EOPNOTSUPP;
	default:
		break;
	}

	coalesce->rx_coalesce_usecs = tp->coalesce;

	return 0;
}

static int rtl8152_set_coalesce(struct net_device *netdev,
				struct ethtool_coalesce *coalesce)
{
	struct r8152 *tp = netdev_priv(netdev);
	int ret;

	switch (tp->version) {
	case RTL_VER_01:
	case RTL_VER_02:
	case RTL_VER_07:
		return -EOPNOTSUPP;
	default:
		break;
	}

	if (coalesce->rx_coalesce_usecs > COALESCE_SLOW)
		return -EINVAL;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&tp->control);

	if (tp->coalesce != coalesce->rx_coalesce_usecs) {
		tp->coalesce = coalesce->rx_coalesce_usecs;

		if (netif_running(tp->netdev) && netif_carrier_ok(netdev))
			r8153_set_rx_early_timeout(tp);
	}

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

	return ret;
}

static struct ethtool_ops ops = {
	.get_drvinfo = rtl8152_get_drvinfo,
	.get_settings = rtl8152_get_settings,
	.set_settings = rtl8152_set_settings,
	.get_link = ethtool_op_get_link,
	.nway_reset = rtl8152_nway_reset,
	.get_msglevel = rtl8152_get_msglevel,
	.set_msglevel = rtl8152_set_msglevel,
	.get_wol = rtl8152_get_wol,
	.set_wol = rtl8152_set_wol,
	.get_strings = rtl8152_get_strings,
	.get_sset_count = rtl8152_get_sset_count,
	.get_ethtool_stats = rtl8152_get_ethtool_stats,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)
	.get_tx_csum = ethtool_op_get_tx_csum,
	.set_tx_csum = ethtool_op_set_tx_csum,
	.get_sg = ethtool_op_get_sg,
	.set_sg = ethtool_op_set_sg,
#ifdef NETIF_F_TSO
	.get_tso = ethtool_op_get_tso,
	.set_tso = ethtool_op_set_tso,
#endif
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0) */
	.get_coalesce = rtl8152_get_coalesce,
	.set_coalesce = rtl8152_set_coalesce,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
	.get_eee = rtl_ethtool_get_eee,
	.set_eee = rtl_ethtool_set_eee,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0) */
};

static int rtltool_ioctl(struct r8152 *tp, struct ifreq *ifr)
{
	struct net_device *netdev = tp->netdev;
	struct rtltool_cmd my_cmd, *myptr;
	struct usb_device_info *uinfo;
	struct usb_device *udev;
	__le32	ocp_data;
	void	*buffer;
	int	ret;

	myptr = (struct rtltool_cmd *)ifr->ifr_data;
	if (copy_from_user(&my_cmd, myptr, sizeof(my_cmd)))
		return -EFAULT;

	ret = 0;

	switch (my_cmd.cmd) {
	case RTLTOOL_PLA_OCP_READ_DWORD:
		pla_ocp_read(tp, (u16)my_cmd.offset, sizeof(ocp_data),
			     &ocp_data);
		my_cmd.data = __le32_to_cpu(ocp_data);

		if (copy_to_user(myptr, &my_cmd, sizeof(my_cmd))) {
			ret = -EFAULT;
			break;
		}
		break;

	case RTLTOOL_PLA_OCP_WRITE_DWORD:
		if (!tp->rtk_enable_diag && net_ratelimit())
			netif_warn(tp, drv, netdev,
				   "rtk diag isn't enable\n");

		ocp_data = __cpu_to_le32(my_cmd.data);
		pla_ocp_write(tp, (u16)my_cmd.offset, (u16)my_cmd.byteen,
			      sizeof(ocp_data), &ocp_data);
		break;

	case RTLTOOL_USB_OCP_READ_DWORD:
		usb_ocp_read(tp, (u16)my_cmd.offset, sizeof(ocp_data),
			     &ocp_data);
		my_cmd.data = __le32_to_cpu(ocp_data);

		if (copy_to_user(myptr, &my_cmd, sizeof(my_cmd))) {
			ret = -EFAULT;
			break;
		}
		break;


	case RTLTOOL_USB_OCP_WRITE_DWORD:
		if (!tp->rtk_enable_diag && net_ratelimit())
			netif_warn(tp, drv, netdev,
				   "rtk diag isn't enable\n");

		ocp_data = __cpu_to_le32(my_cmd.data);
		usb_ocp_write(tp, (u16)my_cmd.offset, (u16)my_cmd.byteen,
			      sizeof(ocp_data), &ocp_data);
		break;

	case RTLTOOL_PLA_OCP_READ:
		buffer = kmalloc(my_cmd.data, GFP_KERNEL);
		if (!buffer) {
			ret = -ENOMEM;
			break;
		}

		pla_ocp_read(tp, (u16)my_cmd.offset, my_cmd.data, buffer);

		if (copy_to_user(my_cmd.buf, buffer, my_cmd.data))
			ret = -EFAULT;

		kfree(buffer);
		break;

	case RTLTOOL_PLA_OCP_WRITE:
		if (!tp->rtk_enable_diag && net_ratelimit())
			netif_warn(tp, drv, netdev,
				   "rtk diag isn't enable\n");

		buffer = kmalloc(my_cmd.data, GFP_KERNEL);
		if (!buffer) {
			ret = -ENOMEM;
			break;
		}

		if (copy_from_user(buffer, my_cmd.buf, my_cmd.data)) {
			ret = -EFAULT;
			kfree(buffer);
			break;
		}

		pla_ocp_write(tp, (u16)my_cmd.offset, (u16)my_cmd.byteen,
			      my_cmd.data, buffer);
		kfree(buffer);
		break;

	case RTLTOOL_USB_OCP_READ:
		buffer = kmalloc(my_cmd.data, GFP_KERNEL);
		if (!buffer) {
			ret = -ENOMEM;
			break;
		}

		usb_ocp_read(tp, (u16)my_cmd.offset, my_cmd.data, buffer);

		if (copy_to_user(my_cmd.buf, buffer, my_cmd.data))
			ret = -EFAULT;

		kfree(buffer);
		break;

	case RTLTOOL_USB_OCP_WRITE:
		if (!tp->rtk_enable_diag && net_ratelimit())
			netif_warn(tp, drv, netdev,
				   "rtk diag isn't enable\n");

		buffer = kmalloc(my_cmd.data, GFP_KERNEL);
		if (!buffer) {
			ret = -ENOMEM;
			break;
		}

		if (copy_from_user(buffer, my_cmd.buf, my_cmd.data)) {
			ret = -EFAULT;
			kfree(buffer);
			break;
		}

		usb_ocp_write(tp, (u16)my_cmd.offset, (u16)my_cmd.byteen,
			      my_cmd.data, buffer);
		kfree(buffer);
		break;

	case RTLTOOL_USB_INFO:
		uinfo = (struct usb_device_info *)&my_cmd.nic_info;
		udev = tp->udev;
		uinfo->idVendor = __le16_to_cpu(udev->descriptor.idVendor);
		uinfo->idProduct = __le16_to_cpu(udev->descriptor.idProduct);
		uinfo->bcdDevice = __le16_to_cpu(udev->descriptor.bcdDevice);
		strlcpy(uinfo->devpath, udev->devpath, sizeof(udev->devpath));
		pla_ocp_read(tp, PLA_IDR, sizeof(uinfo->dev_addr),
			     uinfo->dev_addr);

		if (copy_to_user(myptr, &my_cmd, sizeof(my_cmd)))
			ret = -EFAULT;

		break;

	case RTL_ENABLE_USB_DIAG:
		ret = usb_autopm_get_interface(tp->intf);
		if (ret < 0)
			break;

		mutex_lock(&tp->control);
		tp->rtk_enable_diag++;
		netif_info(tp, drv, netdev, "enable rtk diag %d\n",
			   tp->rtk_enable_diag);
		break;

	case RTL_DISABLE_USB_DIAG:
		if (!tp->rtk_enable_diag) {
			netif_err(tp, drv, netdev,
				  "Invalid using rtk diag\n");
			ret = -EPERM;
			break;
		}

		rtk_disable_diag(tp);
		break;

	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static int rtl8152_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	struct r8152 *tp = netdev_priv(netdev);
	struct mii_ioctl_data *data = if_mii(rq);
	int ret;

	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return -ENODEV;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		goto out;

	switch (cmd) {
	case SIOCGMIIPHY:
		data->phy_id = R8152_PHY_ID; /* Internal PHY */
		break;

	case SIOCGMIIREG:
		if (unlikely(tp->rtk_enable_diag)) {
			ret = -EBUSY;
			break;
		}

		mutex_lock(&tp->control);
		data->val_out = r8152_mdio_read(tp, data->reg_num);
		mutex_unlock(&tp->control);
		break;

	case SIOCSMIIREG:
		if (!capable(CAP_NET_ADMIN)) {
			ret = -EPERM;
			break;
		}

		if (unlikely(tp->rtk_enable_diag)) {
			ret = -EBUSY;
			break;
		}

		mutex_lock(&tp->control);
		r8152_mdio_write(tp, data->reg_num, data->val_in);
		mutex_unlock(&tp->control);
		break;

	case SIOCDEVPRIVATE:
		if (!capable(CAP_NET_ADMIN)) {
			ret = -EPERM;
			break;
		}
		ret = rtltool_ioctl(tp, rq);
		break;

	default:
		ret = -EOPNOTSUPP;
	}

	usb_autopm_put_interface(tp->intf);

out:
	return ret;
}

static int rtl8152_change_mtu(struct net_device *dev, int new_mtu)
{
	struct r8152 *tp = netdev_priv(dev);
	int ret;

	switch (tp->version) {
	case RTL_VER_01:
	case RTL_VER_02:
	case RTL_VER_07:
		return eth_change_mtu(dev, new_mtu);
	default:
		break;
	}

	if (new_mtu < 68 || new_mtu > RTL8153_MAX_MTU)
		return -EINVAL;

	ret = usb_autopm_get_interface(tp->intf);
	if (ret < 0)
		return ret;

	mutex_lock(&tp->control);

	dev->mtu = new_mtu;

	if (netif_running(dev)) {
		u32 rms = new_mtu + VLAN_ETH_HLEN + CRC_SIZE;

		ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, rms);

		if (netif_carrier_ok(dev))
			r8153_set_rx_early_size(tp);
	}

	mutex_unlock(&tp->control);

	usb_autopm_put_interface(tp->intf);

	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
static const struct net_device_ops rtl8152_netdev_ops = {
	.ndo_open		= rtl8152_open,
	.ndo_stop		= rtl8152_close,
	.ndo_do_ioctl		= rtl8152_ioctl,
	.ndo_start_xmit		= rtl8152_start_xmit,
	.ndo_tx_timeout		= rtl8152_tx_timeout,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
	.ndo_vlan_rx_register	= rtl8152_vlan_rx_register,
#else
	.ndo_set_features	= rtl8152_set_features,
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0) */
	.ndo_set_rx_mode	= rtl8152_set_rx_mode,
	.ndo_set_mac_address	= rtl8152_set_mac_address,
	.ndo_change_mtu		= rtl8152_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 4)
	.ndo_features_check	= rtl8152_features_check,
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 4) */
};
#endif

static void r8152b_get_version(struct r8152 *tp)
{
	u32	ocp_data;
	u16	version;

	ocp_data = ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR1);
	version = (u16)(ocp_data & VERSION_MASK);

	switch (version) {
	case 0x4c00:
		tp->version = RTL_VER_01;
		break;
	case 0x4c10:
		tp->version = RTL_VER_02;
		break;
	case 0x5c00:
		tp->version = RTL_VER_03;
		tp->mii.supports_gmii = 1;
		break;
	case 0x5c10:
		tp->version = RTL_VER_04;
		tp->mii.supports_gmii = 1;
		break;
	case 0x5c20:
		tp->version = RTL_VER_05;
		tp->mii.supports_gmii = 1;
		break;
	case 0x5c30:
		tp->version = RTL_VER_06;
		tp->mii.supports_gmii = 1;
		break;
	case 0x4800:
		tp->version = RTL_VER_07;
		break;
	case 0x6000:
		tp->version = RTL_VER_08;
		tp->mii.supports_gmii = 1;
		break;
	case 0x6010:
		tp->version = RTL_VER_09;
		tp->mii.supports_gmii = 1;
		break;
	default:
		netif_info(tp, probe, tp->netdev,
			   "Unknown version 0x%04x\n", version);
		break;
	}
}

static void rtl8152_unload(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	if (tp->version != RTL_VER_01)
		r8152_power_cut_en(tp, true);
}

static void rtl8153_unload(struct r8152 *tp)
{
	if (test_bit(RTL8152_UNPLUG, &tp->flags))
		return;

	tp->rtl_ops.power_cut_en(tp, false);
}

static void rtl_fake_enable(struct r8152 *tp, bool enable)
{
}

static int rtl_ops_init(struct r8152 *tp)
{
	struct rtl_ops *ops = &tp->rtl_ops;
	int ret = 0;

	switch (tp->version) {
	case RTL_VER_01:
	case RTL_VER_02:
	case RTL_VER_07:
		ops->init		= r8152b_init;
		ops->enable		= rtl8152_enable;
		ops->disable		= rtl8152_disable;
		ops->up			= rtl8152_up;
		ops->down		= rtl8152_down;
		ops->unload		= rtl8152_unload;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
		ops->eee_get		= r8152_get_eee;
		ops->eee_set		= r8152_set_eee;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */
		ops->in_nway		= rtl8152_in_nway;
		ops->hw_phy_cfg		= r8152b_hw_phy_cfg;
		ops->aldps_enable	= r8152_aldps_en;
		ops->u1u2_enable	= rtl_fake_enable;
		ops->u2p3_enable	= rtl_fake_enable;
		ops->power_cut_en	= r8152_power_cut_en;
		ops->autosuspend_en	= rtl_runtime_suspend_enable;
		break;

	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
		ops->init		= r8153_init;
		ops->enable		= rtl8153_enable;
		ops->disable		= rtl8153_disable;
		ops->up			= rtl8153_up;
		ops->down		= rtl8153_down;
		ops->unload		= rtl8153_unload;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
		ops->eee_get		= r8153_get_eee;
		ops->eee_set		= r8153_set_eee;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */
		ops->in_nway		= rtl8153_in_nway;
		ops->hw_phy_cfg		= r8153_hw_phy_cfg;
		ops->aldps_enable	= r8153_aldps_en;
		ops->u1u2_enable	= r8153_u1u2en;
		ops->u2p3_enable	= r8153_u2p3en;
		ops->power_cut_en	= r8153_power_cut_en;
		ops->autosuspend_en	= rtl8153_runtime_enable;
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		ops->init		= r8153b_init;
		ops->enable		= rtl8153_enable;
		ops->disable		= rtl8153_disable;
		ops->up			= rtl8153_up;
		ops->down		= rtl8153_down;
		ops->unload		= rtl8153_unload;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
		ops->eee_get		= r8153_get_eee;
		ops->eee_set		= r8153b_set_eee;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) */
		ops->in_nway		= rtl8153_in_nway;
		ops->hw_phy_cfg		= r8153b_hw_phy_cfg;
		ops->aldps_enable	= r8153b_aldps_en;
		ops->u1u2_enable	= r8153b_u1u2en;
		ops->u2p3_enable	= rtl_fake_enable;
		ops->power_cut_en	= r8153B_power_cut_en;
		ops->autosuspend_en	= rtl8153b_runtime_enable;
		break;

	default:
		ret = -ENODEV;
		netif_err(tp, probe, tp->netdev, "Unknown Device\n");
		break;
	}

	return ret;
}

static int rtl8152_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct r8152 *tp;
	struct net_device *netdev;
	int ret;

	if (!rtl_vendor_mode(intf)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
		dev_err(&intf->dev, "The kernel too old to set configuration\n");
#else
		usb_driver_set_configuration(udev, 1);
#endif
		return -ENODEV;
	}

	usb_reset_device(udev);
	netdev = alloc_etherdev(sizeof(struct r8152));
	if (!netdev) {
		dev_err(&intf->dev, "Out of memory\n");
		return -ENOMEM;
	}

	SET_NETDEV_DEV(netdev, &intf->dev);
	tp = netdev_priv(netdev);
	tp->msg_enable = 0x7FFF;

	tp->udev = udev;
	tp->netdev = netdev;
	tp->intf = intf;

	r8152b_get_version(tp);
	ret = rtl_ops_init(tp);
	if (ret)
		goto out;

	mutex_init(&tp->control);
	INIT_DELAYED_WORK(&tp->schedule, rtl_work_func_t);
	INIT_DELAYED_WORK(&tp->hw_phy_work, rtl_hw_phy_work_func_t);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
	netdev->open = rtl8152_open;
	netdev->stop = rtl8152_close;
	netdev->get_stats = rtl8152_get_stats;
	netdev->hard_start_xmit = rtl8152_start_xmit;
	netdev->tx_timeout = rtl8152_tx_timeout;
	netdev->change_mtu = rtl8152_change_mtu;
	netdev->set_mac_address = rtl8152_set_mac_address;
	netdev->do_ioctl = rtl8152_ioctl;
	netdev->set_multicast_list = rtl8152_set_rx_mode;
	netdev->vlan_rx_register = rtl8152_vlan_rx_register;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
	netdev->vlan_rx_kill_vid = rtl8152_vlan_rx_kill_vid;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22) */
#else
	netdev->netdev_ops = &rtl8152_netdev_ops;
#endif /* HAVE_NET_DEVICE_OPS */

	netdev->watchdog_timeo = RTL8152_TX_TIMEOUT;

	netdev->features |= NETIF_F_RXCSUM | NETIF_F_IP_CSUM | NETIF_F_SG |
			    NETIF_F_TSO | NETIF_F_FRAGLIST | NETIF_F_IPV6_CSUM |
			    NETIF_F_TSO6 | NETIF_F_HW_VLAN_CTAG_RX |
			    NETIF_F_HW_VLAN_CTAG_TX;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 38)
	netdev->hw_features = NETIF_F_RXCSUM | NETIF_F_IP_CSUM | NETIF_F_SG |
			      NETIF_F_TSO | NETIF_F_FRAGLIST |
			      NETIF_F_IPV6_CSUM | NETIF_F_TSO6 |
			      NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_TX;
	netdev->vlan_features = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_TSO |
				NETIF_F_HIGHDMA | NETIF_F_FRAGLIST |
				NETIF_F_IPV6_CSUM | NETIF_F_TSO6;
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2,6,38) */

	netdev->ethtool_ops = &ops;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 26)
	netif_set_gso_max_size(netdev, RTL_LIMITED_TSO_SIZE);
#else
	netdev->features &= ~(NETIF_F_TSO | NETIF_F_TSO6);
#endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2,6,25) */

	tp->mii.dev = netdev;
	tp->mii.mdio_read = read_mii_word;
	tp->mii.mdio_write = write_mii_word;
	tp->mii.phy_id_mask = 0x3f;
	tp->mii.reg_num_mask = 0x1f;
	tp->mii.phy_id = R8152_PHY_ID;

	tp->autoneg = AUTONEG_ENABLE;
	tp->speed = tp->mii.supports_gmii ? SPEED_1000 : SPEED_100;
	tp->duplex = DUPLEX_FULL;

	intf->needs_remote_wakeup = 1;

	tp->rtl_ops.init(tp);
	queue_delayed_work(system_long_wq, &tp->hw_phy_work, 0);
	set_ethernet_addr(tp);

	usb_set_intfdata(intf, tp);
	netif_napi_add(netdev, &tp->napi, r8152_poll, RTL8152_NAPI_WEIGHT);

	ret = register_netdev(netdev);
	if (ret != 0) {
		netif_err(tp, probe, netdev, "couldn't register the device\n");
		goto out1;
	}

	if (!rtl_can_wakeup(tp))
		__rtl_set_wol(tp, 0);

	tp->saved_wolopts = __rtl_get_wol(tp);
	if (tp->saved_wolopts)
		device_set_wakeup_enable(&udev->dev, true);
	else
		device_set_wakeup_enable(&udev->dev, false);

	/* usb_enable_autosuspend(udev); */

	netif_info(tp, probe, netdev, "%s\n", DRIVER_VERSION);
	netif_info(tp, probe, netdev, "%s\n", PATENTS);
	tp->bottom_half_event = 1;
	tp->bottom_half_wait_time = 3;
	init_waitqueue_head(&tp->bottom_half_wait_q);


	return 0;

out1:
	netif_napi_del(&tp->napi);
	usb_set_intfdata(intf, NULL);
out:
	free_netdev(netdev);
	return ret;
}

static void rtl8152_disconnect(struct usb_interface *intf)
{
	struct r8152 *tp = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	if (tp) {
		struct usb_device *udev = tp->udev;

		if (udev->state == USB_STATE_NOTATTACHED)
			set_bit(RTL8152_UNPLUG, &tp->flags);

		netif_device_detach(tp->netdev);
		netif_napi_del(&tp->napi);
		unregister_netdev(tp->netdev);
		cancel_delayed_work_sync(&tp->hw_phy_work);
		tp->rtl_ops.unload(tp);
		free_netdev(tp->netdev);
	}
}

#define REALTEK_USB_DEVICE_INTERFACE_CLASS(vend, prod)	\
	USB_DEVICE_INTERFACE_CLASS(vend, prod, USB_CLASS_VENDOR_SPEC)


#define REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(vend, prod)	\
	USB_DEVICE_AND_INTERFACE_INFO(vend, prod, USB_CLASS_COMM, \
				      USB_CDC_SUBCLASS_ETHERNET, \
				      USB_CDC_PROTO_NONE) \

/* table of devices that work with this driver */
static struct usb_device_id rtl8152_table[] = {
	/* Realtek */
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_REALTEK, 0x8050)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_REALTEK, 0x8050)},

	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_REALTEK, 0x8152)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_REALTEK, 0x8152)},

	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_REALTEK, 0x8153)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_REALTEK, 0x8153)},
	/* Samsung */
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_SAMSUNG, 0xa101)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_SAMSUNG, 0xa101)},

	/* Lenovo */
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x304f)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x304f)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x3052)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x3052)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x3054)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x3054)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x3057)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x3057)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x3062)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x3062)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x3069)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x3069)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x7205)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x7205)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x720a)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x720a)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x720b)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x720b)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x720c)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x720c)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_LENOVO, 0x7214)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_LENOVO, 0x7214)},

	/* TP-LINK */
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_TPLINK, 0x0601)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_TPLINK, 0x0601)},

	/* Nvidia */
	{REALTEK_USB_DEVICE_INTERFACE_CLASS(VENDOR_ID_NVIDIA,  0x09ff)},
	{REALTEK_USB_DEVICE_INTERFACE_CLASS_AND_INTERFACE_INFO(VENDOR_ID_NVIDIA,  0x09ff)},
	{}
};

MODULE_DEVICE_TABLE(usb, rtl8152_table);

static struct usb_driver rtl8152_driver = {
	.name =		MODULENAME,
	.id_table =	rtl8152_table,
	.probe =	rtl8152_probe,
	.disconnect =	rtl8152_disconnect,
	.suspend =	rtl8152_suspend,
	.resume =	rtl8152_resume,
	.reset_resume =	rtl8152_reset_resume,
	.pre_reset =	rtl8152_pre_reset,
	.post_reset =	rtl8152_post_reset,
	.supports_autosuspend = 1,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	.disable_hub_initiated_lpm = 1,
#endif
};

module_usb_driver(rtl8152_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
