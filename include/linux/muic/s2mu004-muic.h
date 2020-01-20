/*
 * Copyright (C) 2015 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __S2MU004_MUIC_H__
#define __S2MU004_MUIC_H__

#include <linux/wakelock.h>
#include <linux/muic/muic_core.h>
#include <linux/muic/muic_interface.h>
#include <linux/muic/s2mu004-muic-hv.h>

#define MUIC_DEV_NAME	"muic-s2mu004"

#define MASK_1BIT		(0x1)
#define MASK_2BIT		(0x3)
#define MASK_3BIT		(0x7)
#define MASK_4BIT		(0xf)
#define MASK_5BIT		(0x1f)
#define MASK_6BIT		(0x3f)
#define MASK_7BIT		(0x7f)
#define MASK_8BIT		(0xff)

#define MAX_BCD_RESCAN_CNT	5

/* s2mu004 muic register read/write related information defines. */
/* S2MU004 Control register */
#define CTRL_SWITCH_OPEN_SHIFT	4
#define CTRL_RAW_DATA_SHIFT		3
#define CTRL_MANUAL_SW_SHIFT	2
#define CTRL_WAIT_SHIFT			1
#define CTRL_INT_MASK_SHIFT		0

#define CTRL_SWITCH_OPEN_MASK	(0x1 << CTRL_SWITCH_OPEN_SHIFT)
#define CTRL_RAW_DATA_MASK		(0x1 << CTRL_RAW_DATA_SHIFT)
#define CTRL_MANUAL_SW_MASK		(0x1 << CTRL_MANUAL_SW_SHIFT)
#define CTRL_WAIT_MASK			(0x1 << CTRL_WAIT_SHIFT)
#define CTRL_INT_MASK_MASK		(0x1 << CTRL_INT_MASK_SHIFT)

#if defined(CONFIG_MUIC_S2MU004_ENABLE_AUTOSW)
#define CTRL_MASK			(CTRL_SWITCH_OPEN_MASK | \
						CTRL_MANUAL_SW_MASK | CTRL_WAIT_MASK | \
						CTRL_INT_MASK_MASK)
#else
#define CTRL_MASK			(CTRL_SWITCH_OPEN_MASK | \
						CTRL_WAIT_MASK | CTRL_INT_MASK_MASK)
#endif

/* S2MU004 MUIC Interrupt 1 register */
#define INT_RID_CHG_SHIFT		5
#define INT_LKR_SHIFT			4
#define INT_LKP_SHIFT			3
#define INT_KP_SHIFT			2
#define INT_DETACH_SHIFT		1
#define INT_ATTACH_SHIFT		0

#define INT_RID_CHG_MASK		(0x1 << INT_RID_CHG_SHIFT)
#define INT_LKR_MASK			(0x1 << INT_LKR_SHIFT)
#define INT_LKP_MASK			(0x1 << INT_LKP_SHIFT)
#define INT_KP_MASK				(0x1 << INT_KP_SHIFT)
#define INT_DETACH_MASK			(0x1 << INT_DETACH_SHIFT)
#define INT_ATTACH_MASK			(0x1 << INT_ATTACH_SHIFT)

/* S2MU004 MUIC Interrupt 2 register */
#define INT_VBUS_OFF_SHIFT		7
#define INT_AV_CHANGE_SHIFT		6
#define INT_MHDL_SHIFT			5
#define INT_STUCKRCV_SHIFT		4
#define INT_STUCK_SHIFT			3
#define INT_ADC_CHANGE_SHIFT	2
#define INT_RSRV_ATTACH_SHIFT	1
#define INT_CHG_DET_SHIFT		0

#define INT_VBUS_OFF_MASK		(0x1 << INT_VBUS_OFF_SHIFT)
#define INT_AV_CHANGE_MASK		(0x1 << INT_AV_CHANGE_SHIFT)
#define INT_MHDL_MASK			(0x1 << INT_MHDL_SHIFT)
#define INT_STUCKRCV_MASK		(0x1 << INT_STUCKRCV_SHIFT)
#define INT_STUCK_MASK			(0x1 << INT_STUCK_SHIFT)
#define INT_ADC_CHANGE_MASK		(0x1 << INT_ADC_CHANGE_SHIFT)
#define INT_RSRV_ATTACH_MASK	(0x1 << INT_RSRV_ATTACH_SHIFT)
#define INT_VBUS_ON_MASK		(0x1 << INT_CHG_DET_SHIFT)

/* S2MU004 MUIC Interrupt 1 MASK register */
#define INTm_RID_CHG_SHIFT		5
#define INTm_LKR_SHIFT			4
#define INTm_LKP_SHIFT			3
#define INTm_KP_SHIFT			2
#define INTm_DETACH_SHIFT		1
#define INTm_ATTACH_SHIFT		0

#define INTm_RID_CHG_MASK		(0x1 << INTm_RID_CHG_SHIFT)
#define INTm_LKR_MASK			(0x1 << INTm_LKR_SHIFT)
#define INTm_LKP_MASK			(0x1 << INTm_LKP_SHIFT)
#define INTm_KP_MASK			(0x1 << INTm_KP_SHIFT)
#define INTm_DETACH_MASK		(0x1 << INTm_DETACH_SHIFT)
#define INTm_ATTACH_MASK		(0x1 << INTm_ATTACH_SHIFT)

/* S2MU004 MUIC Interrupt 2 MASK register */
#define INTm_VBUS_OFF_SHIFT		7
#define INTm_AV_CHANGE_SHIFT	6
#define INTm_MHDL_SHIFT			5
#define INTm_STUCKRCV_SHIFT		4
#define INTm_STUCK_SHIFT		3
#define INTm_ADC_CHANGE_SHIFT	2
#define INTm_RSRV_ATTACH_SHIFT	1
#define INTm_CHG_DET_SHIFT		0

#define INTm_VBUS_OFF_MASK		(0x1 << INTm_VBUS_OFF_SHIFT)
#define INTm_AV_CHANGE_MASK		(0x1 << INTm_AV_CHANGE_SHIFT)
#define INTm_MHDL_MASK			(0x1 << INTm_MHDL_SHIFT)
#define INTm_STUCKRCV_MASK		(0x1 << INTm_STUCKRCV_SHIFT)
#define INTm_STUCK_MASK			(0x1 << INTm_STUCK_SHIFT)
#define INTm_ADC_CHANGE_MASK	(0x1 << INTm_ADC_CHANGE_SHIFT)
#define INTm_RSRV_ATTACH_MASK	(0x1 << INTm_RSRV_ATTACH_SHIFT)
#define INTm_VBUS_ON_MASK		(0x1 << INTm_CHG_DET_SHIFT)

/* S2MU004 MUIC Interrupt Maksing for pdic */
#define INT_PDIC_MASK1			(0xFC)
#define INT_PDIC_MASK2			(0x7A)

/* S2MU004 ADC register */
#define ADC_MASK				(0x1f)
#define ADC_CONVERSION_ERR_MASK		(0x1 << 7)

/* S2MU004 Timing Set 1 & 2 register Timing table */
#define KEY_PRESS_TIME_100MS		(0x00)
#define KEY_PRESS_TIME_200MS		(0x10)
#define KEY_PRESS_TIME_300MS		(0x20)
#define KEY_PRESS_TIME_700MS		(0x60)

#define LONGKEY_PRESS_TIME_300MS	(0x00)
#define LONGKEY_PRESS_TIME_500MS	(0x02)
#define LONGKEY_PRESS_TIME_1000MS	(0x07)
#define LONGKEY_PRESS_TIME_1500MS	(0x0C)

#define SWITCHING_WAIT_TIME_10MS	(0x00)
#define SWITCHING_WAIT_TIME_210MS	(0xa0)

/* S2MU004 MUIC Device Type 1 register */
#define DEV_TYPE1_USB_OTG			(0x1 << 7)
#define DEV_TYPE1_DEDICATED_CHG		(0x1 << 6)
#define DEV_TYPE1_CDP				(0x1 << 5)
#define DEV_TYPE1_T1_T2_CHG			(0x1 << 4)
#define DEV_TYPE1_UART				(0x1 << 3)
#define DEV_TYPE1_USB				(0x1 << 2)
#define DEV_TYPE1_AUDIO_2			(0x1 << 1)
#define DEV_TYPE1_AUDIO_1			(0x1 << 0)
#define DEV_TYPE1_USB_TYPES			(DEV_TYPE1_USB_OTG | DEV_TYPE1_CDP | DEV_TYPE1_USB)
#define DEV_TYPE1_CHG_TYPES			(DEV_TYPE1_DEDICATED_CHG | DEV_TYPE1_CDP)

/* S2MU004 MUIC Device Type 2 register */
#define DEV_TYPE2_SDP_1P8S			(0x1 << 7)
#define DEV_TYPE2_AV				(0x1 << 6)
#define DEV_TYPE2_TTY				(0x1 << 5)
#define DEV_TYPE2_PPD				(0x1 << 4)
#define DEV_TYPE2_JIG_UART_OFF		(0x1 << 3)
#define DEV_TYPE2_JIG_UART_ON		(0x1 << 2)
#define DEV_TYPE2_JIG_USB_OFF		(0x1 << 1)
#define DEV_TYPE2_JIG_USB_ON		(0x1 << 0)
#define DEV_TYPE2_JIG_USB_TYPES		(DEV_TYPE2_JIG_USB_OFF | DEV_TYPE2_JIG_USB_ON)
#define DEV_TYPE2_JIG_UART_TYPES	(DEV_TYPE2_JIG_UART_OFF)
#define DEV_TYPE2_JIG_TYPES			(DEV_TYPE2_JIG_UART_TYPES | DEV_TYPE2_JIG_USB_TYPES)

/* S2MU004 MUIC Device Type 3 register */
#define DEV_TYPE3_U200_CHG			(0x1 << 7)
#define DEV_TYPE3_AV_WITH_VBUS		(0x1 << 4)
#define DEV_TYPE3_VBUS_R255			(0x1 << 1)
#define DEV_TYPE3_MHL				(0x1 << 0)
#define DEV_TYPE3_CHG_TYPE			(DEV_TYPE3_U200_CHG | DEV_TYPE3_VBUS_R255)

/* S2MU004 MUIC APPLE Device Type register */
#define DEV_TYPE_APPLE_APPLE0P5A_CHG	(0x1 << 7)
#define DEV_TYPE_APPLE_APPLE1A_CHG		(0x1 << 6)
#define DEV_TYPE_APPLE_APPLE2A_CHG		(0x1 << 5)
#define DEV_TYPE_APPLE_APPLE2P4A_CHG	(0x1 << 4)
#define DEV_TYPE_APPLE_SDP_DCD_OUT		(0x1 << 3)
#define DEV_TYPE_APPLE_RID_WAKEUP		(0x1 << 2)
#define DEV_TYPE_APPLE_VBUS_WAKEUP		(0x1 << 1)
#define DEV_TYPE_APPLE_BCV1P2_OR_OPEN	(0x1 << 0)

/* S2MU004 MUIC CHG Type register */
#define CHG_TYPE_VBUS_R255		(0x1 << 7)
#define DEV_TYPE_U200			(0x1 << 4)
#define DEV_TYPE_SDP_1P8S		(0x1 << 3)
#define DEV_TYPE_USB			(0x1 << 2)
#define DEV_TYPE_CDPCHG			(0x1 << 1)
#define DEV_TYPE_DCPCHG			(0x1 << 0)
#define DEV_TYPE_CHG_TYPE		(CHG_TYPE_VBUS_R255 | DEV_TYPE_U200 | DEV_TYPE_SDP_1P8S)

/* S2MU004 MUIC AFC STATUS register (0x48) */
#define MUIC_AFC_STATUS_VDNMON_SHIFT	4
#define MUIC_AFC_STATUS_VDNMON_MASK	(MASK_1BIT << MUIC_AFC_STATUS_VDNMON_SHIFT)

/* S2MU004 MUIC AFC CTRL1 register (0x49) */
#define MUIC_AFC_CTRL1_AFC_EN_SHIFT			7
#define MUIC_AFC_CTRL1_CTRL_IDM_ON_SHIFT	6
#define MUIC_AFC_CTRL1_VB_ADC_EN_SHIFT		5
#define MUIC_AFC_CTRL1_DPVD_SEL_SHIFT		3
#define MUIC_AFC_CTRL1_DNVD_SEL_SHIFT		1
#define MUIC_AFC_CTRL1_DPDNVD_EN_SHIFT		0

#define MUIC_AFC_CTRL1_AFC_EN_MASK			(MASK_1BIT << MUIC_AFC_CTRL1_AFC_EN_SHIFT)
#define MUIC_AFC_CTRL1_CTRL_IDM_ON_MASK		(MASK_1BIT << MUIC_AFC_CTRL1_CTRL_IDM_ON_SHIFT)
#define MUIC_AFC_CTRL1_VB_ADC_EN_MASK		(MASK_1BIT << MUIC_AFC_CTRL1_VB_ADC_EN_SHIFT)
#define MUIC_AFC_CTRL1_DPVD_SEL_MASK		(MASK_2BIT << MUIC_AFC_CTRL1_DPVD_SEL_SHIFT)
#define MUIC_AFC_CTRL1_DNVD_SEL_MASK		(MASK_2BIT << MUIC_AFC_CTRL1_DNVD_SEL_SHIFT)
#define MUIC_AFC_CTRL1_DPDNVD_EN_MASK		(MASK_1BIT << MUIC_AFC_CTRL1_DPDNVD_EN_SHIFT)

/* S2MU004 MUIC AFC LOGIC CTRL2 register (0x5F) */
#define MUIC_AFC_LOGIC_CTRL2_AFC_RST_SHIFT					7
#define MUIC_AFC_LOGIC_CTRL2_AFC_REQUEST_OPT_REDUMP_SHIFT	6
#define MUIC_AFC_LOGIC_CTRL2_UI_MASTER_SLAVE_SEL_SHIFT		5
#define MUIC_AFC_LOGIC_CTRL2_PROTOCOL_SW_ON_SHFIT			2
#define MUIC_AFC_LOGIC_CTRL2_RX_COMPHY_SHIFT				0

#define MUIC_AFC_LOGIC_CTRL2_AFC_RST_MASK	(MASK_1BIT << MUIC_AFC_LOGIC_CTRL2_AFC_RST_SHIFT)
#define MUIC_AFC_LOGIC_CTRL2_AFC_REQUEST_OPT_REDUMP_MASK (MASK_1BIT << MUIC_AFC_LOGIC_CTRL2_AFC_REQUEST_OPT_REDUMP_SHIFT)
#define MUIC_AFC_LOGIC_CTRL2_UI_MASTER_SLAVE_SEL_MASK (MASK_1BIT << MUIC_AFC_LOGIC_CTRL2_UI_MASTER_SLAVE_SEL_SHIFT)
#define MUIC_AFC_LOGIC_CTRL2_PROTOCOL_SW_ON_MASK (MASK_1BIT << MUIC_AFC_LOGIC_CTRL2_PROTOCOL_SW_ON_SHFIT)
#define MUIC_AFC_LOGIC_CTRL2_RX_COMPHY_MASK	(MASK_2BIT << MUIC_AFC_LOGIC_CTRL2_RX_COMPHY_SHIFT)

/* S2MU004 MUIC BCD RESCAN (0x6A) */
#define MUIC_BCD_RESCAN_SHIFT		0

#define MUIC_BCD_RESCAN_MASK		(MASK_1BIT << MUIC_BCD_RESCAN_SHIFT)

/* S2MU004 MUIC TIMER SET3 register (0xCB) */
#define TIMER_SET3_JIG_WAIT_TIME_SHIFT		5
#define TIMER_SET3_ADC_PERIOD_SHIFT			3
#define TIMER_SET3_DCDTMRSET_SHIFT			0

#define TIMER_SET3_JIG_WAIT_TIME_MASK	(MASK_3BIT << TIMER_SET3_JIG_WAIT_TIME_SHIFT)
#define TIMER_SET3_ADC_PERIOD_MASK	(MASK_2BIT << TIMER_SET3_ADC_PERIOD_SHIFT)
#define TIMER_SET3_DCDTMRSET_MASK	(MASK_3BIT << TIMER_SET3_DCDTMRSET_SHIFT)

/* S2MU004 MUIC CTRL2 register (0xCC) */
#define MUIC_CTRL2_ADCEN_CNTR_SHIFT		4
#define MUIC_CTRL2_DCDTMRSET_SHIFT		3
#define MUIC_CTRL2_USB_2ND_EN_SHIFT		2
#define MUIC_CTRL2_ADC_OFF_SHIFT		1
#define MUIC_CTRL2_CPEN_SHIFT			0

#define MUIC_CTRL2_ADCEN_CNTR_MASK		(MASK_3BIT << MUIC_CTRL2_ADCEN_CNTR_SHIFT)
#define MUIC_CTRL2_DCDTMRSET_MASK		(MASK_1BIT << MUIC_CTRL2_DCDTMRSET_SHIFT)
#define MUIC_CTRL2_USB_2ND_EN_MASK		(MASK_1BIT << MUIC_CTRL2_USB_2ND_EN_SHIFT)
#define MUIC_CTRL2_ADC_OFF_MASK			(MASK_1BIT << MUIC_CTRL2_ADC_OFF_SHIFT)
#define MUIC_CTRL2_CPEN_MASK			(MASK_1BIT << MUIC_CTRL2_CPEN_SHIFT)

/* S2MU004 CHARGER DET OTP register (0xCE) */
#define CHARGER_DET_OTP_VDAT_REF_SHIFT 0

#define CHARGER_DET_OTP_VDAT_REF_MASK (MASK_4BIT << CHARGER_DET_OTP_VDAT_REF_SHIFT)

/* S2MU004 AFC_OTP6(RGB) register (0xDA) */
#define MUIC_AFC_OTP6_JIG_QBAT_OFF_SHIFT		7
#define MUIC_AFC_OTP6_FAST_LC_MODE_SHIFT		6
#define MUIC_AFC_OTP6_NO_FAST_3L_LC_MODE_SHIFT	5
#define MUIC_AFC_OTP6_EN_VBUS_DET_MUIC_SHIFT	4
#define MUIC_AFC_OTP6_LED1_ITRIM_SHFIT			3
#define MUIC_AFC_OTP6_LED2_ITRIM_SHFIT			2
#define MUIC_AFC_OTP6_LED3_ITRIM_SHFIT			1
#define MUIC_AFC_OTP6_LED4_ITRIM_SHFIT			0

#define MUIC_AFC_OTP6_JIG_QBAT_OFF_MASK			(MASK_1BIT << MUIC_AFC_OTP6_JIG_QBAT_OFF_SHIFT)
#define MUIC_AFC_OTP6_FAST_LC_MODE_MASK			(MASK_1BIT << MUIC_AFC_OTP6_FAST_LC_MODE_SHIFT)
#define MUIC_AFC_OTP6_NO_FAST_3L_LC_MODE_MASK	(MASK_1BIT << MUIC_AFC_OTP6_NO_FAST_3L_LC_MODE_SHIFT)
#define MUIC_AFC_OTP6_EN_VBUS_DET_MUIC_MASK		(MASK_1BIT << MUIC_AFC_OTP6_EN_VBUS_DET_MUIC_SHIFT)
#define MUIC_AFC_OTP6_LED1_ITRIM_MASK			(MASK_1BIT << MUIC_AFC_OTP6_LED1_ITRIM_SHFIT)
#define MUIC_AFC_OTP6_LED2_ITRIM_MASK			(MASK_1BIT << MUIC_AFC_OTP6_LED2_ITRIM_SHFIT)
#define MUIC_AFC_OTP6_LED3_ITRIM_MASK			(MASK_1BIT << MUIC_AFC_OTP6_LED3_ITRIM_SHFIT)
#define MUIC_AFC_OTP6_LED4_ITRIM_MASK			(MASK_1BIT << MUIC_AFC_OTP6_LED4_ITRIM_SHFIT)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2] / CHARGER[1] / OTGEN[0]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 * 00: Vbus to Open / 01: Vbus to Charger / 10: Vbus to MIC / 11: Vbus to VBout
 */
#define MANUAL_SW_JIG_EN		(0x1 << 0)

#define MANUAL_SW_DM_SHIFT		5
#define MANUAL_SW_DP_SHIFT		2
#define MANUAL_SW_CHG_SHIFT		1
#define MANUAL_SW_DM_DP_MASK	0xFC

#define MANUAL_SW_OPEN			(0x0)
#define MANUAL_SW_USB			(0x1 << MANUAL_SW_DM_SHIFT | 0x1 << MANUAL_SW_DP_SHIFT)
#define MANUAL_SW_UART			(0x2 << MANUAL_SW_DM_SHIFT | 0x2 << MANUAL_SW_DP_SHIFT)
#define MANUAL_SW_UART2			(0x3 << MANUAL_SW_DM_SHIFT | 0x3 << MANUAL_SW_DP_SHIFT)
#define MANUAL_SW_AUDIO			(0x0 << MANUAL_SW_DM_SHIFT | 0x0 << MANUAL_SW_DP_SHIFT)	/* Not Used */

#define MANUAL_SW_OTGEN			(0x1)

/* S2MU004_REG_MUIC_RID_CTRL */
#define RID_CTRL_ADC_OFF_SHIFT	1
#define RID_CTRL_ADC_OFF_MASK	(0x1 << RID_CTRL_ADC_OFF_SHIFT)

#define WATER_DET_RETRY_CNT				10
#define WATER_CCIC_WAIT_DURATION_MS		4000
#define WATER_DRY_RETRY_INTERVAL_MS		30000
#define WATER_DRY_INTERVAL_MS			10000
#define WATER_DET_STABLE_DURATION_MS	2000
#define DRY_DET_RETRY_CNT_MAX			3
#define RID_REFRESH_DURATION_MS			100
#define WATER_TOGGLE_WA_DURATION_US		20000

/* s2mu004-muic macros */
#define ENUM_STR(x, r) { case x: r = #x; break; }

#define REQUEST_IRQ(_irq, _dev_id, _name, _func)				\
do {									\
	ret = request_threaded_irq(_irq, NULL, _func,	\
				0, _name, _dev_id);	\
	if (ret < 0) {							\
		pr_err("%s:%s Failed to request IRQ #%d: %d\n",		\
				MUIC_DEV_NAME, __func__, _irq, ret);	\
		_irq = 0;						\
	}								\
} while (0)

#define FREE_IRQ(_irq, _dev_id, _name)					\
do {									\
	if (_irq) {							\
		free_irq(_irq, _dev_id);				\
		pr_info("%s:%s IRQ(%d):%s free done\n", MUIC_DEV_NAME,	\
				__func__, _irq, _name);			\
	}								\
} while (0)

#define IS_WATER_ADC(adc)\
		(((adc) > (ADC_GND)) && ((adc) < (ADC_OPEN)) \
		? 1 : 0)
#define IS_AUDIO_ADC(adc)\
		(((adc) >= (ADC_SEND_END)) && ((adc) <= (ADC_REMOTE_S12)) \
		? 1 : 0)
#define IS_ACC_ADC(adc)\
		(((adc) >= (ADC_RESERVED_VZW)) \
		&& ((adc) <= (ADC_AUDIOMODE_W_REMOTE)) \
		? 1 : 0)
/* end of macros */

/* S2MU004_REG_LDOADC_VSETH register */
#define LDOADC_VSETH_MASK	0x1F
#define LDOADC_VSETL_MASK	0x1F
#define LDOADC_VSET_3V		0x1F
#define LDOADC_VSET_2_7V	0x1C
#define LDOADC_VSET_2_6V	0x0E
#define LDOADC_VSET_2_4V	0x0C
#define LDOADC_VSET_2_2V	0x0A
#define LDOADC_VSET_2_0V	0x08
#define LDOADC_VSET_1_8V	0x06
#define LDOADC_VSET_1_7V	0x05
#define LDOADC_VSET_1_6V	0x04
#define LDOADC_VSET_1_5V	0x03
#define LDOADC_VSET_1_4V	0x02
#define LDOADC_VSET_1_2V	0x00
#define LDOADC_VSETH_WAKE_HYS_SHIFT	6
#define LDOADC_VSETH_WAKE_HYS_MASK	(0x1 << LDOADC_VSETH_WAKE_HYS_SHIFT)

#define MANSW_OPEN				(MANUAL_SW_OPEN)
#define MANSW_USB					(MANUAL_SW_USB)
#define MANSW_AUDIO				(MANUAL_SW_AUDIO)
#define MANSW_OTG					(MANUAL_SW_USB | MANUAL_SW_OTGEN)
#define MANSW_UART_AP				(MANUAL_SW_UART)
#if IS_ENABLED(CONFIG_PMU_UART_SWITCH)
#define MANSW_UART_CP				(MANUAL_SW_UART)
#else
#define MANSW_UART_CP				(MANUAL_SW_UART2)
#endif
#define MANSW_HICCUP				(MANUAL_SW_UART2)
#define MANSW_OPEN_RUSTPROOF	(MANUAL_SW_OPEN)

enum s2mu004_muic_mode {
	S2MU004_NONE_CABLE,
	S2MU004_MUIC_ATTACH,
	S2MU004_MUIC_DETACH,
	S2MU004_MUIC_OTG,
	S2MU004_MUIC_JIG,
};

enum s2mu004_muic_adc_mode {
	S2MU004_ADC_ONESHOT,
	S2MU004_ADC_PERIODIC,
};

typedef enum {
	S2MU004_WATER_MUIC_IDLE,
	S2MU004_WATER_MUIC_DET,
	S2MU004_WATER_MUIC_CCIC_DET,
	S2MU004_WATER_MUIC_CCIC_STABLE,
	S2MU004_WATER_MUIC_CCIC_INVALID,
} t_water_status;

typedef enum {
	S2MU004_WATER_DRY_MUIC_IDLE,
	S2MU004_WATER_DRY_MUIC_DET,
	S2MU004_WATER_DRY_MUIC_CCIC_DET,
	S2MU004_WATER_DRY_MUIC_CCIC_INVALID,
} t_water_dry_status;

enum s2mu004_muic_enable {
	S2MU004_DISABLE,
	S2MU004_ENABLE,
};

typedef enum {
	S2MU004_DETECT_NONE,
	S2MU004_DETECT_DONE,
} t_detect_status;

typedef enum {
	S2MU004_IRQ_CHECK_DONE,
	S2MU004_IRQ_SKIP,
} t_irq_status;

enum s2mu004_muic_detect_dev_read_val {
	DEVICE_TYPE1 = 0,
	DEVICE_TYPE2,
	DEVICE_TYPE3,
	REV_ID,
	ADC,
	DEVICE_APPLE,
	CHG_TYPE,
	SC_STATUS2,
	MAX_NUM,
};

/* muic chip specific internal data structure
 * that setted at muic-xxxx.c file
 */
struct s2mu004_muic_data {
	struct device *dev;
	struct device *switch_device;
	struct i2c_client *i2c;	/* i2c addr: 0x7A; MUIC */
	struct s2mu004_dev *s2mu004_dev;

	struct mutex muic_mutex;
	struct mutex afc_mutex;
	struct mutex switch_mutex;

	/* struct wakeup_source wakeup_src; */
	struct wake_lock wake_lock;

	/* model dependant mfd platform data */
	struct s2mu004_platform_data *mfd_pdata;

	/* model dependant muic platform data */
	struct muic_platform_data *pdata;

	void *if_data;

	int irq_attach;
	int irq_detach;
	int irq_rid_chg;
	int irq_vbus_on;
	int irq_rsvd_attach;
	int irq_adc_change;
	int irq_av_charge;
	int irq_vbus_off;

	bool afc_check;
	bool otg_state;
	bool is_dcd_recheck;
	bool is_otg_vboost;
	bool is_otg_reboost;

	muic_attached_dev_t new_dev;

	int rev_id;
	int adc;
	int vbvolt;
	int vmid;
	int reg[MAX_NUM];
	int re_detect;
	int bcd_rescan_cnt;

#if defined(CONFIG_MUIC_S2MU004_HV)
	int retry_cnt;
	int retry_qc_cnt;
	int qc_prepare;
	int afc_irq;
	int irq_dnres;
	int irq_mrxrdy;
	int irq_mpnack;
	int irq_vbadc;
	int irq_vdnmon;
#endif

	/* muic common callback driver internal data */
	struct sec_switch_data *switch_data;

	/* muic support vps list */
	bool muic_support_list[ATTACHED_DEV_NUM];

	/* muic Device ID */
	u8 muic_vendor;		/* Vendor ID */
	u8 muic_version;	/* Version ID */
	u8 ic_rev_id;		/* Rev ID */

	/* W/A waiting for the charger ic */
	struct delayed_work dcd_recheck;
	struct delayed_work incomplete_check;
#if defined(CONFIG_MUIC_S2MU004_HV)
	struct delayed_work afc_check_vbadc;
	struct delayed_work afc_cable_type_work;
	struct delayed_work afc_irq_detect;
	struct delayed_work afc_send_mpnack;
	struct delayed_work afc_control_ping_retry;
	struct delayed_work afc_qc_retry;
	struct delayed_work afc_after_prepare;
	struct delayed_work afc_check_interrupt;
	struct delayed_work afc_mrxrdy;

	muic_afc_data_t afc_data;

	bool is_afc_muic_ready;
	bool is_afc_handshaking;
	bool is_afc_muic_prepare;
	bool is_charger_ready;
	bool is_mrxrdy;

	int afc_count;

	u8 tx_data;
	u8 qc_hv;

	/* muic status value */
	u8 status1;
	u8 status2;
	u8 status3;
	u8 status4;

	/* muic hvcontrol value */
	u8 hvcontrol1;
	u8 hvcontrol2;
#endif
};

extern struct muic_platform_data muic_pdata;

int s2mu004_muic_com_to_open_with_vbus(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_com_to_open(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_com_to_usb(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_com_to_otg(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_com_to_uart(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_com_to_audio(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_recheck_adc(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_get_vbus_state(struct s2mu004_muic_data *muic_data);
void s2mu004_muic_control_vbus_det(struct s2mu004_muic_data *muic_data,
				   bool enable);
int s2mu004_i2c_read_byte(struct i2c_client *client, u8 command);
int s2mu004_i2c_write_byte(struct i2c_client *client, u8 command, u8 value);
extern void s2mu004_muic_dcd_rescan(struct s2mu004_muic_data *muic_data);
extern int s2mu004_muic_control_rid_adc(struct s2mu004_muic_data *muic_data,
					bool enable);
extern int s2mu004_muic_bcd_rescan(struct s2mu004_muic_data *muic_data);
int s2mu004_muic_jig_on(struct s2mu004_muic_data *muic_data);
void s2mu004_muic_control_vbus_det(struct s2mu004_muic_data *muic_data,
				   bool enable);

int s2mu004_set_gpio_uart_sel(struct s2mu004_muic_data *muic_data,
			      int uart_sel);
int s2mu004_init_interface(struct s2mu004_muic_data *muic_data,
			   struct muic_interface_t *muic_if);
#if IS_ENABLED(CONFIG_CCIC_S2MU004)
int s2mu004_muic_refresh_adc(struct s2mu004_muic_data *muic_data);
#endif
#if IS_ENABLED(CONFIG_SEC_FACTORY)
int s2mu004_muic_set_otg_reg(struct s2mu004_muic_data *muic_data, bool on);
#else
int s2mu004_muic_get_otg_state(void);
#endif
#endif /* __S2MU004_MUIC_H__ */
