/* linux/drivers/video/fbdev/exynos/dpu/regs-displayport.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Register definition file for Samsung vpp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DISPLAYPORT_REGS_H_
#define DISPLAYPORT_REGS_H_

#define SYSTEM_DEVICE_VERSION			(0x0000)
#define DEVICE_VERSION				(0xFFFFFFFF << 0)

#define SYSTEM_SW_RESET_CONTROL			(0x0004)
#define SW_RESET				(0x01 << 0)

#define SYSTEM_CLK_CONTROL			(0x0008)
#define GFMUX_STATUS_TXCLK			(0x03 << 8)
#define TXCLK_SEL				(0x01 << 5)
#define TXCLK_SEL_MODE				(0x01 << 4)
#define OSC_CLK_SEL				(0x01 << 0)

#define SYSTEM_MAIN_LINK_BANDWIDTH		(0x000C)
#define LINK_BW_SET			        (0x1F << 0)

#define SYSTEM_MAIN_LINK_LANE_COUNT		(0x0010)
#define LANE_COUNT_SET			        (0x07 << 0)

#define SYSTEM_SW_FUNCTION_ENABLE		(0x0014)
#define SW_FUNC_EN			        (0x01 << 0)

#define SYSTEM_COMMON_FUNCTION_ENABLE		(0x0018)
#define HDCP22_FUNC_EN				(0x01 << 4)
#define HDCP13_FUNC_EN				(0x01 << 3)
#define GTC_FUNC_EN				(0x01 << 2)
#define PCS_FUNC_EN				(0x01 << 1)
#define AUX_FUNC_EN				(0x01 << 0)

#define SYSTEM_SST1_FUNCTION_ENABLE		(0x001C)
#define SST1_LH_PWR_ON_STATUS			(0x01 << 5)
#define SST1_LH_PWR_ON				(0x01 << 4)
#define SST1_AUDIO_FIFO_FUNC_EN			(0x01 << 2)
#define SST1_AUDIO_FUNC_EN			(0x01 << 1)
#define SST1_VIDEO_FUNC_EN			(0x01 << 0)

#define SYSTEM_SST2_FUNCTION_ENABLE		(0x0020)
#define SST2_LH_PWR_ON_STATUS			(0x01 << 5)
#define SST2_LH_PWR_ON				(0x01 << 4)
#define SST2_AUDIO_FIFO_FUNC_EN			(0x01 << 2)
#define SST2_AUDIO_FUNC_EN			(0x01 << 1)
#define SST2_VIDEO_FUNC_EN			(0x01 << 0)

#define SYSTEM_MISC_CONTROL			(0x0024)
#define MISC_CTRL_EN				(0x01 << 1)
#define HDCP_HPD_RST				(0x01 << 0)

#define SYSTEM_HPD_CONTROL			(0x0028)
#define HPD_HDCP				(0x01 << 30)
#define HPD_DEGLITCH_COUNT			(0x3FFF << 16)
#define HPD_STATUS				(0x01 << 8)
#define HPD_EVENT_UNPLUG			(0x01 << 7)
#define HPD_EVENT_PLUG				(0x01 << 6)
#define HPD_EVENT_IRQ				(0x01 << 5)
#define HPD_EVENT_CTRL_EN			(0x01 << 4)
#define HPD_FORCE				(0x01 << 1)
#define HPD_FORCE_EN				(0x01 << 0)

#define SYSTEM_PLL_LOCK_CONTROL			(0x002C)
#define PLL_LOCK_STATUS				(0x01 << 4)
#define PLL_LOCK_FORCE				(0x01 << 3)
#define PLL_LOCK_FORCE_EN			(0x01 << 2)

#define SYSTEM_INTERRUPT_CONTROL		(0x0100)
#define SW_INTR_CTRL				(0x01 << 1)
#define INTR_POL				(0x01 << 0)

#define SYSTEM_INTERRUPT_REQUEST		(0x0104)
#define IRQ_MST					(0x01 << 3)
#define IRQ_STR2				(0x01 << 2)
#define IRQ_STR1				(0x01 << 1)
#define IRQ_COMMON				(0x01 << 0)

#define SYSTEM_IRQ_COMMON_STATUS		(0x0108)
#define HDCP_ENC_EN_CHG				(0x01 << 22)
#define HDCP_LINK_CHK_FAIL			(0x01 << 21)
#define HDCP_R0_CHECK_FLAG			(0x01 << 20)
#define HDCP_BKSV_RDY				(0x01 << 19)
#define HDCP_SHA_DONE				(0x01 << 18)
#define HDCP_AUTH_STATE_CHG			(0x01 << 17)
#define HDCP_AUTH_DONE				(0x01 << 16)
#define HPD_IRQ_FLAG				(0x01 << 11)
#define HPD_CHG					(0x01 << 10)
#define HPD_LOST				(0x01 << 9)
#define HPD_PLUG_INT				(0x01 << 8)
#define AUX_REPLY_RECEIVED			(0x01 << 5)
#define AUX_ERR					(0x01 << 4)
#define PLL_LOCK_CHG				(0x01 << 1)
#define SW_INTR					(0x01 << 0)

#define SYSTEM_IRQ_COMMON_STATUS_MASK		(0x010C)
#define MST_BUF_OVERFLOW_MASK			(0x01 << 23)
#define HDCP_ENC_EN_CHG_MASK			(0x01 << 22)
#define HDCP_LINK_CHK_FAIL_MASK			(0x01 << 21)
#define HDCP_R0_CHECK_FLAG_MASK			(0x01 << 20)
#define HDCP_BKSV_RDY_MASK			(0x01 << 19)
#define HDCP_SHA_DONE_MASK			(0x01 << 18)
#define HDCP_AUTH_STATE_CHG_MASK		(0x01 << 17)
#define HDCP_AUTH_DONE_MASK			(0x01 << 16)
#define HPD_IRQ_MASK				(0x01 << 11)
#define HPD_CHG_MASK				(0x01 << 10)
#define HPD_LOST_MASK				(0x01 << 9)
#define HPD_PLUG_MASK				(0x01 << 8)
#define AUX_REPLY_RECEIVED_MASK			(0x01 << 5)
#define AUX_ERR_MASK				(0x01 << 4)
#define PLL_LOCK_CHG_MASK			(0x01 << 1)
#define SW_INTR_MASK				(0x01 << 0)

#define SYSTEM_DEBUG				(0x0200)
#define AUX_HPD_CONTROL				(0x01 << 2)
#define CLKGATE_DISABLE				(0x01 << 1)

#define SYSTEM_DEBUG_LH_PCH			(0x0204)
#define SST2_LH_PSTATE				(0x01 << 12)
#define SST2_LH_PCH_FSM_STATE			(0x0F << 8)
#define SST1_LH_PSTATE				(0x01 << 4)
#define SST1_LH_PCH_FSM_STATE			(0x0F << 0)

#define AUX_CONTROL				(0x1000)
#define AUX_POWER_DOWN				(0x01 << 16)
#define AUX_REPLY_TIMER_MODE			(0x03 << 12)
#define AUX_RETRY_TIMER				(0x07 << 8)
#define AUX_PN_INV				(0x01 << 1)
#define REG_MODE_SEL				(0x01 << 0)

#define AUX_TRANSACTION_START			(0x1004)
#define AUX_TRAN_START				(0x01 << 0)

#define AUX_BUFFER_CLEAR			(0x1008)
#define AUX_BUF_CLR				(0x01 << 0)

#define AUX_ADDR_ONLY_COMMAND			(0x100C)
#define ADDR_ONLY_CMD				(0x01 << 0)

#define AUX_REQUEST_CONTROL			(0x1010)
#define REQ_COMM				(0x0F << 28)
#define REQ_ADDR				(0xFFFFF << 8)
#define REQ_LENGTH				(0x3F << 0)

#define AUX_COMMAND_CONTROL			(0x1014)
#define DEFER_CTRL_EN				(0x01 << 8)
#define DEFER_COUNT				(0x7F << 0)

#define AUX_MONITOR_1				(0x1018)
#define AUX_BUF_DATA_COUNT			(0x7F << 24)
#define AUX_DETECTED_PERIOD_MON			(0x1FF << 12)
#define AUX_CMD_STATUS				(0x0F << 8)
#define AUX_RX_COMM				(0x0F << 4)
#define AUX_LAST_MODE				(0x01 << 3)
#define AUX_BUSY				(0x01 << 2)
#define AUX_REQ_WAIT_GRANT			(0x01 << 1)
#define AUX_REQ_SIGNAL				(0x01 << 0)

#define AUX_MONITOR_2				(0x101C)
#define AUX_ERROR_NUMBER			(0xFF << 0)

#define AUX_TX_DATA_SET0			(0x1030)
#define TX_DATA_3				(0xFF << 24)
#define TX_DATA_2				(0xFF << 16)
#define TX_DATA_1				(0xFF << 8)
#define TX_DATA_0				(0xFF << 0)

#define AUX_TX_DATA_SET1			(0x1034)
#define TX_DATA_7				(0xFF << 24)
#define TX_DATA_6				(0xFF << 16)
#define TX_DATA_5				(0xFF << 8)
#define TX_DATA_4				(0xFF << 0)

#define AUX_TX_DATA_SET2			(0x1038)
#define TX_DATA_11				(0xFF << 24)
#define TX_DATA_10				(0xFF << 16)
#define TX_DATA_9				(0xFF << 8)
#define TX_DATA_8				(0xFF << 0)

#define AUX_TX_DATA_SET3			(0x103C)
#define TX_DATA_15				(0xFF << 24)
#define TX_DATA_14				(0xFF << 16)
#define TX_DATA_13				(0xFF << 8)
#define TX_DATA_12				(0xFF << 0)

#define AUX_RX_DATA_SET0			(0x1040)
#define RX_DATA_3				(0xFF << 24)
#define RX_DATA_2				(0xFF << 16)
#define RX_DATA_1				(0xFF << 8)
#define RX_DATA_0				(0xFF << 0)

#define AUX_RX_DATA_SET1			(0x1044)
#define RX_DATA_7				(0xFF << 24)
#define RX_DATA_6				(0xFF << 16)
#define RX_DATA_5				(0xFF << 8)
#define RX_DATA_4				(0xFF << 0)

#define AUX_RX_DATA_SET2			(0x1048)
#define RX_DATA_11				(0xFF << 24)
#define RX_DATA_10				(0xFF << 16)
#define RX_DATA_9				(0xFF << 8)
#define RX_DATA_8				(0xFF << 0)

#define AUX_RX_DATA_SET3			(0x104C)
#define RX_DATA_15				(0xFF << 24)
#define RX_DATA_14				(0xFF << 16)
#define RX_DATA_13				(0xFF << 8)
#define RX_DATA_12				(0xFF << 0)

#define GTC_CONTROL				(0x1100)
#define GTC_DELTA_ADJ_EN			(0x01 << 2)
#define IMPL_OPTION				(0x01 << 1)
#define GTC_TX_MASTER				(0x01 << 0)

#define GTC_WORK_ENABLE				(0x1104)
#define GTC_WORK_EN				(0x01 << 0)

#define GTC_TIME_CONTROL			(0x1108)
#define TIME_PERIOD_SEL				(0x03 << 28)
#define TIME_PERIOD_FRACTIONAL			(0xFFFFF << 8)
#define TIME_PERIOD_INT1			(0x0F << 4)
#define TIME_PERIOD_INT2			(0x0F << 0)

#define GTC_ATTEMPT_CONTROL			(0x110C)
#define GTC_STATE_CHANGE_CTRL			(0x01 << 8)
#define NUM_GTC_ATTEMPT				(0xFF << 0)

#define GTC_TX_VALUE_MONITOR			(0x1110)
#define GTC_TX_VAL				(0xFFFFFFFF << 0)

#define GTC_RX_VALUE_MONITOR			(0x1114)
#define GTC_RX_VAL				(0xFFFFFFFF << 0)

#define GTC_STATUS_MONITOR			(0x1118)
#define FREQ_ADJ_10_3				(0xFF << 8)
#define FREQ_ADJ_2_0				(0x07 << 5)
#define TXGTC_LOCK_DONE_FLAG			(0x01 << 1)
#define RXGTC_LOCK_DONE_FLAG			(0x01 << 0)

#define AUX_GTC_DEBUG				(0x1200)
#define DEBUG_STATE_SEL				(0xFF << 8)
#define DEBUG_STATE				(0xFF << 0)

#define MST_ENABLE				(0x2000)
#define MST_EN					(0x01 << 0)

#define MST_VC_PAYLOAD_UPDATE_FLAG		(0x2004)
#define VC_PAYLOAD_UPDATE_FLAG			(0x01 << 0)

#define MST_STREAM_1_ENABLE			(0x2010)
#define STRM_1_EN				(0x01 << 0)

#define MST_STREAM_1_HDCP_CTRL			(0x2014)
#define STRM_1_HDCP22_TYPE			(0x01 << 1)
#define STRM_1_HDCP_EN				(0x01 << 0)

#define MST_STREAM_1_X_VALUE			(0x2018)
#define STRM_1_X_VALUE				(0xFF << 0)

#define MST_STREAM_1_Y_VALUE			(0x201C)
#define STRM_1_Y_VALUE				(0xFF << 0)

#define MST_STREAM_2_ENABLE			(0x2020)
#define STRM_2_EN				(0x01 << 0)

#define MST_STREAM_2_HDCP_CTRL			(0x2024)
#define STRM_2_HDCP22_TYPE			(0x01 << 1)
#define STRM_2_HDCP_EN				(0x01 << 0)

#define MST_STREAM_2_X_VALUE			(0x2028)
#define STRM_2_X_VALUE				(0xFF << 0)

#define MST_STREAM_2_Y_VALUE			(0x202C)
#define STRM_2_Y_VALUE				(0xFF << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_01_08	(0x2040)
#define VC_PAYLOAD_ID_TIMESLOT_01		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_02		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_03		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_04		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_05		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_06		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_07		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_08		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_09_16	(0x2044)
#define VC_PAYLOAD_ID_TIMESLOT_09		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_10		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_11		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_12		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_13		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_14		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_15		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_16		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_17_24	(0x2048)
#define VC_PAYLOAD_ID_TIMESLOT_17		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_18		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_19		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_20		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_21		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_22		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_23		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_24		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_25_32	(0x204C)
#define VC_PAYLOAD_ID_TIMESLOT_25		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_26		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_27		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_28		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_29		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_30		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_31		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_32		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_33_40	(0x2050)
#define VC_PAYLOAD_ID_TIMESLOT_33		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_34		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_35		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_36		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_37		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_38		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_39		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_40		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_41_48	(0x2054)
#define VC_PAYLOAD_ID_TIMESLOT_41		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_42		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_43		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_44		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_45		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_46		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_47		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_48		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_49_56	(0x2058)
#define VC_PAYLOAD_ID_TIMESLOT_49		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_50		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_51		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_52		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_53		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_54		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_55		(0x03 << 4)
#define VC_PAYLOAD_ID_TIMESLOT_56		(0x03 << 0)

#define MST_VC_PAYLOAD_ID_TIMESLOT_57_63	(0x205C)
#define VC_PAYLOAD_ID_TIMESLOT_57		(0x03 << 28)
#define VC_PAYLOAD_ID_TIMESLOT_58		(0x03 << 24)
#define VC_PAYLOAD_ID_TIMESLOT_59		(0x03 << 20)
#define VC_PAYLOAD_ID_TIMESLOT_60		(0x03 << 16)
#define VC_PAYLOAD_ID_TIMESLOT_61		(0x03 << 12)
#define VC_PAYLOAD_ID_TIMESLOT_62		(0x03 << 8)
#define VC_PAYLOAD_ID_TIMESLOT_63		(0x03 << 4)

#define MST_ETC_OPTION				(0x2060)
#define ALLOCATE_TIMESLOT_CHECK_TO_ACT		(0x01 << 1)
#define HDCP22_LVP_TYPE				(0x01 << 0)

#define MST_BUF_STATUS				(0x2064)
#define STRM_2_BUF_OVERFLOW			(0x01 << 5)
#define STRM_1_BUF_OVERFLOW			(0x01 << 4)
#define STRM_2_BUF_OVERFLOW_CLEAR		(0x01 << 1)
#define STRM_1_BUF_OVERFLOW_CLEAR		(0x01 << 0)

#define PCS_CONTROL				(0x3000)
#define FEC_FUNC_EN				(0x01 << 8)
#define LINK_TRAINING_PATTERN_SET		(0x03 << 4)
#define BYTE_SWAP				(0x01 << 3)
#define BIT_SWAP				(0x01 << 2)
#define SCRAMBLE_RESET_VALUE			(0x01 << 1)
#define SCRAMBLE_BYPASS				(0x01 << 0)

#define PCS_LANE_CONTROL			(0x3004)
#define LANE_DATA_INV_EN			(0x01 << 20)
#define LANE3_DATA_INV				(0x01 << 19)
#define LANE2_DATA_INV				(0x01 << 18)
#define LANE1_DATA_INV				(0x01 << 17)
#define LANE0_DATA_INV				(0x01 << 16)
#define LANE3_MAP				(0x03 << 12)
#define LANE2_MAP				(0x03 << 8)
#define LANE1_MAP				(0x03 << 4)
#define LANE0_MAP				(0x03 << 0)

#define PCS_TEST_PATTERN_CONTROL		(0x3008)
#define TEST_PATTERN				(0x3F << 8)
#define LINK_QUALITY_PATTERN_SET		(0x07 << 0)

#define PCS_TEST_PATTERN_SET0			(0x300C)
#define TEST_80BIT_PATTERN_SET0			(0xFFFFFFFF << 0)

#define PCS_TEST_PATTERN_SET1			(0x3010)
#define TEST_80BIT_PATTERN_SET1			(0xFFFFFFFF << 0)

#define PCS_TEST_PATTERN_SET2			(0x3014)
#define TEST_80BIT_PATTERN_SET2			(0xFFFF << 0)

#define PCS_DEBUG_CONTROL			(0x3018)
#define FEC_FLIP_CDADJ_CODES_CASE4		(0x01 << 6)
#define FEC_FLIP_CDADJ_CODES_CASE2		(0x01 << 5)
#define FEC_DECODE_EN_4TH_SEL			(0x01 << 4)
#define FEC_DECODE_DIS_4TH_SEL			(0x01 << 3)
#define PRBS7_OPTION				(0x01 << 2)
#define DISABLE_AUTO_RESET_ENCODE		(0x01 << 1)
#define PRBS31_EN				(0x01 << 0)

#define PCS_HBR2_EYE_SR_CONTROL			(0x3020)
#define HBR2_EYE_SR_CTRL			(0x03 << 16)
#define HBR2_EYE_SR_COUNT			(0xFFFF << 0)

#define PCS_SA_CRC_CONTROL_1			(0x3100)
#define SA_CRC_CLEAR				(0x01 << 13)
#define SA_CRC_SW_COMPARE			(0x01 << 12)
#define SA_CRC_LN3_PASS				(0x01 << 11)
#define SA_CRC_LN2_PASS				(0x01 << 10)
#define SA_CRC_LN1_PASS				(0x01 << 9)
#define SA_CRC_LN0_PASS				(0x01 << 8)
#define SA_CRC_LN3_FAIL				(0x01 << 7)
#define SA_CRC_LN2_FAIL				(0x01 << 6)
#define SA_CRC_LN1_FAIL				(0x01 << 5)
#define SA_CRC_LN0_FAIL				(0x01 << 4)
#define SA_CRC_LANE_3_ENABLE			(0x01 << 3)
#define SA_CRC_LANE_2_ENABLE			(0x01 << 2)
#define SA_CRC_LANE_1_ENABLE			(0x01 << 1)
#define SA_CRC_LANE_0_ENABLE			(0x01 << 0)

#define PCS_SA_CRC_CONTROL_2			(0x3104)
#define SA_CRC_LN0_REF				(0xFFFF << 16)
#define SA_CRC_LN0_RESULT			(0xFFFF << 0)

#define PCS_SA_CRC_CONTROL_3			(0x3108)
#define SA_CRC_LN1_REF				(0xFFFF << 16)
#define SA_CRC_LN1_RESULT			(0xFFFF << 0)

#define PCS_SA_CRC_CONTROL_4			(0x310C)
#define SA_CRC_LN2_REF				(0xFFFF << 16)
#define SA_CRC_LN2_RESULT			(0xFFFF << 0)

#define PCS_SA_CRC_CONTROL_5			(0x3110)
#define SA_CRC_LN3_REF				(0xFFFF << 16)
#define SA_CRC_LN3_RESULT			(0xFFFF << 0)

#define HDCP13_STATUS				(0x4000)
#define REAUTH_REQUEST				(0x01 << 7)
#define AUTH_FAIL				(0x01 << 6)
#define HW_1ST_AUTHEN_PASS			(0x01 << 5)
#define BKSV_VALID				(0x01 << 3)
#define ENCRYPT					(0x01 << 2)
#define HW_AUTHEN_PASS				(0x01 << 1)
#define AKSV_VALID				(0x01 << 0)

#define HDCP13_CONTROL_0			(0x4004)
#define SW_STORE_AN				(0x01 << 7)
#define SW_RX_REPEATER				(0x01 << 6)
#define HW_RE_AUTHEN				(0x01 << 5)
#define SW_AUTH_OK				(0x01 << 4)
#define HW_AUTH_EN				(0x01 << 3)
#define HDCP13_ENC_EN				(0x01 << 2)
#define HW_1ST_PART_ATHENTICATION_EN		(0x01 << 1)
#define HW_2ND_PART_ATHENTICATION_EN		(0x01 << 0)

#define HDCP13_CONTROL_1			(0x4008)
#define DPCD_REV_1_2				(0x01 << 3)
#define HW_AUTH_POLLING_MODE			(0x01 << 1)
#define HDCP_INT				(0x01 << 0)

#define HDCP13_AKSV_0				(0x4010)
#define AKSV0					(0xFFFFFFFF << 0)

#define HDCP13_AKSV_1				(0x4014)
#define AKSV1					(0xFF << 0)

#define HDCP13_AN_0				(0x4018)
#define AN0					(0xFFFFFFFF << 0)

#define HDCP13_AN_1				(0x401C)
#define AN1					(0xFFFFFFFF << 0)

#define HDCP13_BKSV_0				(0x4020)
#define BKSV0					(0xFFFFFFFF << 0)

#define HDCP13_BKSV_1				(0x4024)
#define BKSV1					(0xFF << 0)

#define HDCP13_R0_REG				(0x4028)
#define R0					(0xFFFF << 0)

#define HDCP13_BCAPS				(0x4030)
#define BCAPS					(0xFF << 0)

#define HDCP13_BINFO_REG			(0x4034)
#define BINFO					(0xFF << 0)

#define HDCP13_DEBUG_CONTROL			(0x4038)
#define CHECK_KSV				(0x01 << 2)
#define REVOCATION_CHK_DONE			(0x01 << 1)
#define HW_SKIP_RPT_ZERO_DEV			(0x01 << 0)

#define HDCP13_AUTH_DBG				(0x4040)
#define DDC_STATE				(0x07 << 5)
#define AUTH_STATE				(0x1F << 0)

#define HDCP13_ENC_DBG				(0x4044)
#define ENC_STATE				(0x07 << 3)

#define HDCP13_AM0_0				(0x4048)
#define AM0_0					(0xFFFFFFFF << 0)

#define HDCP13_AM0_1				(0x404C)
#define AM0_1					(0xFFFFFFFF << 0)

#define HDCP13_WAIT_R0_TIME			(0x4054)
#define HW_WRITE_AKSV_WAIT			(0xFF << 0)

#define HDCP13_LINK_CHK_TIME			(0x4058)
#define LINK_CHK_TIMER				(0xFF << 0)

#define HDCP13_REPEATER_READY_WAIT_TIME		(0x405C)
#define HW_RPTR_RDY_TIMER			(0xFF << 0)

#define HDCP13_READY_POLL_TIME			(0x4060)
#define POLLING_TIMER_TH			(0xFF << 0)

#define HDCP13_STREAM_ID_ENCRYPTION_CONTROL	(0x4068)
#define STRM_ID_ENC_UPDATE			(0x01 << 7)
#define STRM_ID_ENC				(0x7F << 0)

#define HDCP22_SYS_EN				(0x4400)
#define SYSTEM_ENABLE				(0x01 << 0)

#define HDCP22_CONTROL				(0x4404)
#define HDCP22_BYPASS_MODE			(0x01 << 1)
#define HDCP22_ENC_EN				(0x01 << 0)

#define HDCP22_CONTROL				(0x4404)
#define HDCP22_BYPASS_MODE			(0x01 << 1)
#define HDCP22_ENC_EN				(0x01 << 0)

#define HDCP22_STREAM_TYPE			(0x4454)
#define STREAM_TYPE				(0x01 << 0)

#define HDCP22_LVP				(0x4460)
#define LINK_VERIFICATION_PATTERN		(0xFFFF << 0)

#define HDCP22_LVP_GEN				(0x4464)
#define LVP_GEN					(0x01 << 0)

#define HDCP22_LVP_CNT_KEEP			(0x4468)
#define LVP_COUNT_KEEP_ENABLE			(0x01 << 0)

#define HDCP22_LANE_DECODE_CTRL			(0x4470)
#define ENHANCED_FRAMING_MODE			(0x01 << 3)
#define LVP_EN_DECODE_ENABLE			(0x01 << 2)
#define ENCRYPTION_SIGNAL_DECODE_ENABLE		(0x01 << 1)
#define LANE_DECODE_ENABLE			(0x01 << 0)

#define HDCP22_SR_VALUE				(0x4480)
#define SR_VALUE				(0xFF << 0)

#define HDCP22_CP_VALUE				(0x4484)
#define CP_VALUE				(0xFF << 0)

#define HDCP22_BF_VALUE				(0x4488)
#define BF_VALUE				(0xFF << 0)

#define HDCP22_BS_VALUE				(0x448C)
#define BS_VALUE				(0xFF << 0)

#define HDCP22_RIV_XOR				(0x4490)
#define RIV_XOR_LOCATION			(0x01 << 0)

#define HDCP22_RIV_0				(0x4500)
#define RIV_KEY_0				(0xFFFFFFFF << 0)

#define HDCP22_RIV_1				(0x4504)
#define RIV_KEY_1				(0xFFFFFFFF << 0)

#define SST1_MAIN_CONTROL			(0x5000)
#define MVID_MODE				(0x01 << 11)
#define MAUD_MODE				(0x01 << 10)
#define MVID_UPDATE_RATE			(0x03 << 8)
#define VIDEO_MODE				(0x01 << 6)
#define ENHANCED_MODE				(0x01 << 5)
#define ODD_TU_CONTROL				(0x01 << 4)

#define SST1_MAIN_FIFO_CONTROL			(0x5004)
#define CLEAR_AUDIO_FIFO			(0x01 << 3)
#define CLEAR_PIXEL_MAPPING_FIFO		(0x01 << 2)
#define CLEAR_MAPI_FIFO				(0x01 << 1)
#define CLEAR_GL_DATA_FIFO			(0x01 << 0)

#define SST1_GNS_CONTROL			(0x5008)
#define RS_CTRL					(0x01 << 0)

#define SST1_SR_CONTROL				(0x500C)
#define SR_COUNT_RESET_VALUE			(0x1FF << 16)
#define SR_REPLACE_BS_COUNT			(0x1F << 4)
#define SR_START_CTRL				(0x01 << 1)
#define SR_REPLACE_BS_EN			(0x01 << 0)

#define SST1_INTERRUPT_MONITOR			(0x5020)
#define INT_STATE				(0x01 << 0)

#define SST1_INTERRUPT_STATUS_SET0		(0x5024)
#define VSC_SDP_TX_INCOMPLETE			(0x01 << 9)
#define MAPI_FIFO_UNDER_FLOW			(0x01 << 8)
#define VSYNC_DET				(0x01 << 7)

#define SST1_INTERRUPT_STATUS_SET1		(0x5028)
#define AFIFO_UNDER				(0x01 << 7)
#define AFIFO_OVER				(0x01 << 6)

#define SST1_INTERRUPT_MASK_SET0		(0x502C)
#define VSC_SDP_TX_INCOMPLETE_MASK		(0x01 << 9)
#define MAPI_FIFO_UNDER_FLOW_MASK		(0x01 << 8)
#define VSYNC_DET_MASK				(0x01 << 7)

#define SST1_INTERRUPT_MASK_SET1		(0x5030)
#define AFIFO_UNDER_MASK			(0x01 << 7)
#define AFIFO_OVER_MASK				(0x01 << 6)

#define SST1_MVID_CALCULATION_CONTROL		(0x5040)
#define MVID_GEN_FILTER_TH			(0xFF << 8)
#define MVID_GEN_FILTER_EN			(0x01 << 0)

#define SST1_MVID_MASTER_MODE			(0x5044)
#define MVID_MASTER				(0xFFFFFFFF << 0)

#define SST1_NVID_MASTER_MODE			(0x5048)
#define NVID_MASTER				(0xFFFFFFFF << 0)

#define SST1_MVID_SFR_CONFIGURE			(0x504C)
#define MVID_SFR_CONFIG				(0xFFFFFF << 0)

#define SST1_NVID_SFR_CONFIGURE			(0x5050)
#define NVID_SFR_CONFIG				(0xFFFFFF << 0)

#define SST1_MVID_MONITOR			(0x5054)
#define MVID_MON				(0xFFFFFF << 0)

#define SST1_MAUD_CALCULATION_CONTROL		(0x5058)
#define M_AUD_GEN_FILTER_TH			(0xFF << 8)
#define M_AUD_GEN_FILTER_EN			(0x01 << 0)

#define SST1_MAUD_MASTER_MODE			(0x505C)
#define MAUD_MASTER				(0xFFFFFFFF << 0)

#define SST1_NAUD_MASTER_MODE			(0x5060)
#define NAUD_MASTER				(0xFFFFFF << 0)

#define SST1_MAUD_SFR_CONFIGURE			(0x5064)
#define MAUD_SFR_CONFIG				(0xFFFFFF << 0)

#define SST1_NAUD_SFR_CONFIGURE			(0x5068)
#define NAUD_SFR_CONFIG				(0xFFFFFF << 0)

#define SST1_NARROW_BLANK_CONTROL		(0x506C)
#define NARROW_BLANK_EN				(0x01 << 1)
#define VIDEO_FIFO_FLUSH_DISABLE		(0x01 << 0)

#define SST1_LOW_TU_CONTROL			(0x5070)
#define NULL_TU_CONTROL				(0x01 << 1)
#define HALF_FREQUENCY_CONTROL			(0x01 << 0)

#define SST1_ACTIVE_SYMBOL_INTEGER_FEC_OFF	(0x5080)
#define ACTIVE_SYMBOL_INTEGER_FEC_OFF		(0x3F << 0)

#define SST1_ACTIVE_SYMBOL_FRACTION_FEC_OFF	(0x5084)
#define ACTIVE_SYMBOL_FRACTION_FEC_OFF		(0x3FFFFFFF << 0)

#define SST1_ACTIVE_SYMBOL_THRESHOLD_FEC_OFF	(0x5088)
#define ACTIVE_SYMBOL_THRESHOLD_FEC_OFF		(0x0F << 0)

#define SST1_ACTIVE_SYMBOL_THRESHOLD_SEL_FEC_OFF	(0x508C)
#define ACTIVE_SYMBOL_THRESHOLD_SEL_FEC_OFF	(0x01 << 0)

#define SST1_ACTIVE_SYMBOL_INTEGER_FEC_ON	(0x5090)
#define ACTIVE_SYMBOL_INTEGER_FEC_ON		(0x3F << 0)

#define SST1_ACTIVE_SYMBOL_FRACTION_FEC_ON	(0x5094)
#define ACTIVE_SYMBOL_FRACTION_FEC_ON		(0x3FFFFFFF << 0)

#define SST1_ACTIVE_SYMBOL_THRESHOLD_FEC_ON	(0x5098)
#define ACTIVE_SYMBOL_THRESHOLD_FEC_ON		(0x0F << 0)

#define SST1_ACTIVE_SYMBOL_THRESHOLD_SEL_FEC_ON	(0x509C)
#define ACTIVE_SYMBOL_THRESHOLD_SEL_FEC_ON	(0x01 << 0)

#define SST1_FEC_DISABLE_SEND_CONTROL		(0x5100)
#define FEC_DISABLE_SEND_CONTROL		(0x01 << 0)

#define SST1_ACTIVE_SYMBOL_MODE_CONTROL		(0x5104)
#define ACTIVE_SYMBOL_MODE_CONTROL		(0x01 << 0)

#define SST1_VIDEO_CONTROL			(0x5400)
#define STRM_VALID_MON				(0x01 << 10)
#define STRM_VALID_FORCE			(0x01 << 9)
#define STRM_VALID_CTRL				(0x01 << 8)
#define DYNAMIC_RANGE_MODE			(0x01 << 7)
#define BPC					(0x07 << 4)
#define COLOR_FORMAT				(0x03 << 2)
#define VSYNC_POLARITY				(0x01 << 1)
#define HSYNC_POLARITY				(0x01 << 0)

#define SST1_VIDEO_ENABLE			(0x5404)
#define VIDEO_EN				(0x01 << 0)

#define SST1_VIDEO_MASTER_TIMING_GEN		(0x5408)
#define VIDEO_MASTER_TIME_GEN			(0x01 << 0)

#define SST1_VIDEO_MUTE				(0x540C)
#define VIDEO_MUTE				(0x01 << 0)

#define SST1_VIDEO_FIFO_THRESHOLD_CONTROL	(0x5410)
#define GL_FIFO_TH_CTRL				(0x01 << 5)
#define GL_FIFO_TH_VALUE			(0x1F << 0)

#define SST1_VIDEO_HORIZONTAL_TOTAL_PIXELS	(0x5414)
#define H_TOTAL_MASTER				(0xFFFFFFFF << 0)

#define SST1_VIDEO_VERTICAL_TOTAL_PIXELS	(0x5418)
#define V_TOTAL_MASTER				(0xFFFFFFFF << 0)

#define SST1_VIDEO_HORIZONTAL_FRONT_PORCH	(0x541C)
#define H_F_PORCH_MASTER			(0xFFFFFFFF << 0)

#define SST1_VIDEO_HORIZONTAL_BACK_PORCH	(0x5420)
#define H_B_PORCH_MASTER			(0xFFFFFFFF << 0)

#define SST1_VIDEO_HORIZONTAL_ACTIVE		(0x5424)
#define H_ACTIVE_MASTER				(0xFFFFFFFF << 0)

#define SST1_VIDEO_VERTICAL_FRONT_PORCH		(0x5428)
#define V_F_PORCH_MASTER			(0xFFFFFFFF << 0)

#define SST1_VIDEO_VERTICAL_BACK_PORCH		(0x542C)
#define V_B_PORCH_MASTER			(0xFFFFFFFF << 0)

#define SST1_VIDEO_VERTICAL_ACTIVE		(0x5430)
#define V_ACTIVE_MASTER				(0xFFFFFFFF << 0)

#define SST1_VIDEO_DSC_STREAM_CONTROL_0		(0x5434)
#define DSC_ENABLE				(0x01 << 4)
#define SLICE_COUNT_PER_LINE			(0x07 << 0)

#define SST1_VIDEO_DSC_STREAM_CONTROL_1		(0x5438)
#define CHUNK_SIZE_1				(0xFFFF << 16)
#define CHUNK_SIZE_0				(0xFFFF << 0)

#define SST1_VIDEO_DSC_STREAM_CONTROL_2		(0x543C)
#define CHUNK_SIZE_3				(0xFFFF << 16)
#define CHUNK_SIZE_2				(0xFFFF << 0)

#define SST1_VIDEO_BIST_CONTROL			(0x5450)
#define CTS_BIST_EN				(0x01 << 17)
#define CTS_BIST_TYPE				(0x03 << 15)
#define BIST_PRBS7_SEED				(0x7F << 8)
#define BIST_USER_DATA_EN			(0x01 << 4)
#define BIST_EN					(0x01 << 3)
#define BIST_WIDTH				(0x01 << 2)
#define BIST_TYPE				(0x03 << 0)

#define SST1_VIDEO_BIST_USER_DATA_R		(0x5454)
#define BIST_USER_DATA_R			(0x3FF << 0)

#define SST1_VIDEO_BIST_USER_DATA_G		(0x5458)
#define BIST_USER_DATA_G			(0x3FF << 0)

#define SST1_VIDEO_BIST_USER_DATA_B		(0x545C)
#define BIST_USER_DATA_B			(0x3FF << 0)

#define SST1_VIDEO_DEBUG_FSM_STATE		(0x5460)
#define DATA_PACK_FSM_STATE			(0x3F << 16)
#define LINE_FSM_STATE				(0x07 << 8)
#define PIXEL_FSM_STATE				(0x07 << 0)

#define SST1_VIDEO_DEBUG_MAPI			(0x5464)
#define MAPI_UNDERFLOW_STATUS			(0x01 << 0)

#define SST1_VIDEO_DEBUG_ACTV_SYM_STEP_CNTL	(0x5468)
#define ACTV_SYM_STEP_CNT_VAL			(0x3FF << 1)
#define ACTV_SYM_STEP_CNT_EN			(0x01 << 0)

#define SST1_VIDEO_DEBUG_HOR_BLANK_AUD_BW_ADJ	(0x546C)
#define HOR_BLANK_AUD_BW_ADJ			(0x01 << 0)

#define SST1_AUDIO_CONTROL			(0x5800)
#define DMA_REQ_GEN_EN				(0x01 << 30)
#define SW_AUD_CODING_TYPE			(0x07 << 27)
#define AUD_DMA_IF_LTNCY_TRG_MODE		(0x01 << 26)
#define AUD_DMA_IF_MODE_CONFIG			(0x01 << 25)
#define AUD_ODD_CHANNEL_DUMMY			(0x01 << 24)
#define AUD_M_VALUE_CMP_SPD_MASTER		(0x07 << 21)
#define DMA_BURST_SEL				(0x07 << 18)
#define AUDIO_BIT_MAPPING_TYPE			(0x03 << 16)
#define PCM_SIZE				(0x03 << 13)
#define AUDIO_CH_STATUS_SAME			(0x01 << 5)
#define AUD_GTC_CHST_EN				(0x01 << 1)

#define SST1_AUDIO_ENABLE			(0x5804)
#define AUDIO_EN				(0x01 << 0)

#define SST1_AUDIO_MASTER_TIMING_GEN		(0x5808)
#define AUDIO_MASTER_TIME_GEN			(0x01 << 0)

#define SST1_AUDIO_DMA_REQUEST_LATENCY_CONFIG	(0x580C)
#define AUD_DMA_ACK_STATUS			(0x01 << 21)
#define AUD_DMA_FORCE_ACK			(0x01 << 20)
#define AUD_DMA_FORCE_ACK_SEL			(0x01 << 19)
#define AUD_DMA_REQ_STATUS			(0x01 << 18)
#define AUD_DMA_FORCE_REQ_VAL			(0x01 << 17)
#define AUD_DMA_FORCE_REQ_SEL			(0x01 << 16)
#define MASTER_DMA_REQ_LTNCY_CONFIG		(0xFF << 0)

#define SST1_AUDIO_MUTE_CONTROL			(0x5810)
#define AUD_MUTE_UNDRUN_EN			(0x01 << 5)
#define AUD_MUTE_OVFLOW_EN			(0x01 << 4)
#define AUD_MUTE_CLKCHG_EN			(0x01 << 1)

#define SST1_AUDIO_MARGIN_CONTROL		(0x5814)
#define FORCE_AUDIO_MARGIN			(0x01 << 16)
#define AUDIO_MARGIN				(0x1FFF << 0)

#define SST1_AUDIO_DATA_WRITE_FIFO		(0x5818)
#define AUDIO_DATA_FIFO				(0xFFFFFFFF << 0)

#define SST1_AUDIO_GTC_CONTROL			(0x5824)
#define AUD_GTC_DELTA				(0xFFFFFFFF << 0)

#define SST1_AUDIO_GTC_VALID_BIT_CONTROL	(0x5828)
#define AUDIO_GTC_VALID_CONTROL			(0x01 << 1)
#define AUDIO_GTC_VALID				(0x01 << 0)

#define SST1_AUDIO_3DLPCM_PACKET_WAIT_TIMER	(0x582C)
#define AUDIO_3D_PKT_WAIT_TIMER			(0x3F << 0)

#define SST1_AUDIO_BIST_CONTROL			(0x5830)
#define SIN_AMPL				(0x0F << 4)
#define AUD_BIST_EN				(0x01 << 0)

#define SST1_AUDIO_BIST_CHANNEL_STATUS_SET0	(0x5834)
#define CHNL_BIT1				(0x03 << 30)
#define CLK_ACCUR				(0x03 << 28)
#define FS_FREQ					(0x0F << 24)
#define CH_NUM					(0x0F << 20)
#define SOURCE_NUM				(0x0F << 16)
#define CAT_CODE				(0xFF << 8)
#define MODE					(0x03 << 6)
#define PCM_MODE				(0x07 << 3)
#define SW_CPRGT				(0x01 << 2)
#define NON_PCM					(0x01 << 1)
#define PROF_APP				(0x01 << 0)

#define SST1_AUDIO_BIST_CHANNEL_STATUS_SET1	(0x5838)
#define CHNL_BIT2				(0x0F << 4)
#define WORD_LENGTH				(0x07 << 1)
#define WORD_MAX				(0x01 << 0)

#define SST1_AUDIO_BUFFER_CONTROL		(0x583C)
#define MASTER_AUDIO_INIT_BUFFER_THRD		(0x7F << 24)
#define MASTER_AUDIO_BUFFER_THRD		(0x3F << 18)
#define MASTER_AUDIO_BUFFER_EMPTY_INT_MASK	(0x01 << 17)
#define MASTER_AUDIO_CHANNEL_COUNT		(0x1F << 12)
#define MASTER_AUDIO_BUFFER_LEVEL		(0x7F << 5)
#define MASTER_AUDIO_BUFFER_LEVEL_BIT_POS	(5)
#define AUD_DMA_NOISE_INT_MASK			(0x01 << 4)
#define AUD_DMA_NOISE_INT			(0x01 << 3)
#define AUD_DMA_NOISE_INT_EN			(0x01 << 2)
#define MASTER_AUDIO_BUFFER_EMPTY_INT		(0x01 << 1)
#define MASTER_AUDIO_BUFFER_EMPTY_INT_EN	(0x01 << 0)

#define SST1_AUDIO_CHANNEL_1_4_REMAP		(0x5840)
#define AUD_CH_04_REMAP				(0x3F << 24)
#define AUD_CH_03_REMAP				(0x3F << 16)
#define AUD_CH_02_REMAP				(0x3F << 8)
#define AUD_CH_01_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_5_8_REMAP		(0x5844)
#define AUD_CH_08_REMAP				(0x3F << 24)
#define AUD_CH_07_REMAP				(0x3F << 16)
#define AUD_CH_06_REMAP				(0x3F << 8)
#define AUD_CH_05_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_9_12_REMAP		(0x5848)
#define AUD_CH_12_REMAP				(0x3F << 24)
#define AUD_CH_11_REMAP				(0x3F << 16)
#define AUD_CH_10_REMAP				(0x3F << 8)
#define AUD_CH_09_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_13_16_REMAP		(0x584C)
#define AUD_CH_16_REMAP				(0x3F << 24)
#define AUD_CH_15_REMAP				(0x3F << 16)
#define AUD_CH_14_REMAP				(0x3F << 8)
#define AUD_CH_13_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_17_20_REMAP		(0x5850)
#define AUD_CH_20_REMAP				(0x3F << 24)
#define AUD_CH_19_REMAP				(0x3F << 16)
#define AUD_CH_18_REMAP				(0x3F << 8)
#define AUD_CH_17_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_21_24_REMAP		(0x5854)
#define AUD_CH_24_REMAP				(0x3F << 24)
#define AUD_CH_23_REMAP				(0x3F << 16)
#define AUD_CH_22_REMAP				(0x3F << 8)
#define AUD_CH_21_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_25_28_REMAP		(0x5858)
#define AUD_CH_28_REMAP				(0x3F << 24)
#define AUD_CH_27_REMAP				(0x3F << 16)
#define AUD_CH_26_REMAP				(0x3F << 8)
#define AUD_CH_25_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_29_32_REMAP		(0x585C)
#define AUD_CH_32_REMAP				(0x3F << 24)
#define AUD_CH_31_REMAP				(0x3F << 16)
#define AUD_CH_30_REMAP				(0x3F << 8)
#define AUD_CH_29_REMAP				(0x3F << 0)

#define SST1_AUDIO_CHANNEL_1_2_STATUS_CTRL_0	(0x5860)
#define MASTER_AUD_GP0_STA_3			(0xFF << 24)
#define MASTER_AUD_GP0_STA_2			(0xFF << 16)
#define MASTER_AUD_GP0_STA_1			(0xFF << 8)
#define MASTER_AUD_GP0_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_1_2_STATUS_CTRL_1	(0x5864)
#define MASTER_AUD_GP0_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_3_4_STATUS_CTRL_0	(0x5868)
#define MASTER_AUD_GP1_STA_3			(0xFF << 24)
#define MASTER_AUD_GP1_STA_2			(0xFF << 16)
#define MASTER_AUD_GP1_STA_1			(0xFF << 8)
#define MASTER_AUD_GP1_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_3_4_STATUS_CTRL_1	(0x586C)
#define MASTER_AUD_GP1_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_5_6_STATUS_CTRL_0	(0x5870)
#define MASTER_AUD_GP2_STA_3			(0xFF << 24)
#define MASTER_AUD_GP2_STA_2			(0xFF << 16)
#define MASTER_AUD_GP2_STA_1			(0xFF << 8)
#define MASTER_AUD_GP2_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_5_6_STATUS_CTRL_1	(0x5874)
#define MASTER_AUD_GP2_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_7_8_STATUS_CTRL_0	(0x5878)
#define MASTER_AUD_GP3_STA_3			(0xFF << 24)
#define MASTER_AUD_GP3_STA_2			(0xFF << 16)
#define MASTER_AUD_GP3_STA_1			(0xFF << 8)
#define MASTER_AUD_GP3_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_7_8_STATUS_CTRL_1	(0x587C)
#define MASTER_AUD_GP3_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_9_10_STATUS_CTRL_0	(0x5880)
#define MASTER_AUD_GP4_STA_3			(0xFF << 24)
#define MASTER_AUD_GP4_STA_2			(0xFF << 16)
#define MASTER_AUD_GP4_STA_1			(0xFF << 8)
#define MASTER_AUD_GP4_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_9_10_STATUS_CTRL_1	(0x5884)
#define MASTER_AUD_GP4_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_11_12_STATUS_CTRL_0	(0x5888)
#define MASTER_AUD_GP5_STA_3			(0xFF << 24)
#define MASTER_AUD_GP5_STA_2			(0xFF << 16)
#define MASTER_AUD_GP5_STA_1			(0xFF << 8)
#define MASTER_AUD_GP5_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_11_12_STATUS_CTRL_1	(0x588C)
#define MASTER_AUD_GP5_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_13_14_STATUS_CTRL_0	(0x5890)
#define MASTER_AUD_GP6_STA_3			(0xFF << 24)
#define MASTER_AUD_GP6_STA_2			(0xFF << 16)
#define MASTER_AUD_GP6_STA_1			(0xFF << 8)
#define MASTER_AUD_GP6_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_13_14_STATUS_CTRL_1	(0x5894)
#define MASTER_AUD_GP6_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_15_16_STATUS_CTRL_0	(0x5898)
#define MASTER_AUD_GP7_STA_3			(0xFF << 24)
#define MASTER_AUD_GP7_STA_2			(0xFF << 16)
#define MASTER_AUD_GP7_STA_1			(0xFF << 8)
#define MASTER_AUD_GP7_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_15_16_STATUS_CTRL_1	(0x589C)
#define MASTER_AUD_GP7_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_17_18_STATUS_CTRL_0	(0x58A0)
#define MASTER_AUD_GP8_STA_3			(0xFF << 24)
#define MASTER_AUD_GP8_STA_2			(0xFF << 16)
#define MASTER_AUD_GP8_STA_1			(0xFF << 8)
#define MASTER_AUD_GP8_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_17_18_STATUS_CTRL_1	(0x58A4)
#define MASTER_AUD_GP8_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_19_20_STATUS_CTRL_0	(0x58A8)
#define MASTER_AUD_GP9_STA_3			(0xFF << 24)
#define MASTER_AUD_GP9_STA_2			(0xFF << 16)
#define MASTER_AUD_GP9_STA_1			(0xFF << 8)
#define MASTER_AUD_GP9_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_19_20_STATUS_CTRL_1	(0x58AC)
#define MASTER_AUD_GP9_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_21_22_STATUS_CTRL_0	(0x58B0)
#define MASTER_AUD_GP10_STA_3			(0xFF << 24)
#define MASTER_AUD_GP10_STA_2			(0xFF << 16)
#define MASTER_AUD_GP10_STA_1			(0xFF << 8)
#define MASTER_AUD_GP10_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_21_22_STATUS_CTRL_1	(0x58B4)
#define MASTER_AUD_GP10_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_23_24_STATUS_CTRL_0	(0x58B8)
#define MASTER_AUD_GP11_STA_3			(0xFF << 24)
#define MASTER_AUD_GP11_STA_2			(0xFF << 16)
#define MASTER_AUD_GP11_STA_1			(0xFF << 8)
#define MASTER_AUD_GP11_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_23_24_STATUS_CTRL_1	(0x58BC)
#define MASTER_AUD_GP11_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_25_26_STATUS_CTRL_0	(0x58C0)
#define MASTER_AUD_GP12_STA_3			(0xFF << 24)
#define MASTER_AUD_GP12_STA_2			(0xFF << 16)
#define MASTER_AUD_GP12_STA_1			(0xFF << 8)
#define MASTER_AUD_GP12_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_25_26_STATUS_CTRL_1	(0x58C4)
#define MASTER_AUD_GP12_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_27_28_STATUS_CTRL_0	(0x58C8)
#define MASTER_AUD_GP13_STA_3			(0xFF << 24)
#define MASTER_AUD_GP13_STA_2			(0xFF << 16)
#define MASTER_AUD_GP13_STA_1			(0xFF << 8)
#define MASTER_AUD_GP13_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_27_28_STATUS_CTRL_1	(0x58CC)
#define MASTER_AUD_GP13_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_29_30_STATUS_CTRL_0	(0x58D0)
#define MASTER_AUD_GP14_STA_3			(0xFF << 24)
#define MASTER_AUD_GP14_STA_2			(0xFF << 16)
#define MASTER_AUD_GP14_STA_1			(0xFF << 8)
#define MASTER_AUD_GP14_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_29_30_STATUS_CTRL_1	(0x58D4)
#define MASTER_AUD_GP14_STA_4			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_31_32_STATUS_CTRL_0	(0x58D8)
#define MASTER_AUD_GP15_STA_3			(0xFF << 24)
#define MASTER_AUD_GP15_STA_2			(0xFF << 16)
#define MASTER_AUD_GP15_STA_1			(0xFF << 8)
#define MASTER_AUD_GP15_STA_0			(0xFF << 0)

#define SST1_AUDIO_CHANNEL_31_32_STATUS_CTRL_1	(0x58DC)
#define MASTER_AUD_GP15_STA_4			(0xFF << 0)

#define SST1_STREAM_IF_CRC_CONTROL_1		(0x58E0)
#define IF_CRC_CLEAR				(0x01 << 13)
#define IF_CRC_PASS				(0x01 << 12)
#define IF_CRC_FAIL				(0x01 << 8)
#define IF_CRC_SW_COMPARE			(0x01 << 4)
#define IF_CRC_EN				(0x01 << 0)

#define SST1_STREAM_IF_CRC_CONTROL_2		(0x58E4)
#define IF_CRC_R_REF				(0xFF << 16)
#define IF_CRC_R_RESULT				(0xFF << 0)

#define SST1_STREAM_IF_CRC_CONTROL_3		(0x58E8)
#define IF_CRC_G_REF				(0xFF << 16)
#define IF_CRC_G_RESULT				(0xFF << 0)

#define SST1_STREAM_IF_CRC_CONTROL_4		(0x58EC)
#define IF_CRC_B_REF				(0xFF << 16)
#define IF_CRC_B_RESULT				(0xFF << 0)

#define SST1_AUDIO_DEBUG_MARGIN_CONTROL		(0x5900)
#define AUDIO_DEBUG_MARGIN_EN			(0x01 << 6)
#define AUDIO_DEBUG_MARGIN_VAL			(0x3F << 0)

#define SST1_SDP_SPLITTING_CONTROL		(0x5C00)
#define SDP_SPLITTING_EN			(0x01 << 0)

#define SST1_INFOFRAME_UPDATE_CONTROL		(0x5C04)
#define HDR_INFO_UPDATE				(0x01 << 4)
#define AUDIO_INFO_UPDATE			(0x01 << 3)
#define AVI_INFO_UPDATE				(0x01 << 2)
#define MPEG_INFO_UPDATE			(0x01 << 1)
#define SPD_INFO_UPDATE				(0x01 << 0)

#define SST1_INFOFRAME_SEND_CONTROL		(0x5C08)
#define HDR_INFO_SEND				(0x01 << 4)
#define AUDIO_INFO_SEND				(0x01 << 3)
#define AVI_INFO_SEND				(0x01 << 2)
#define MPEG_INFO_SEND				(0x01 << 1)
#define SPD_INFO_SEND				(0x01 << 0)

#define SST1_INFOFRAME_SDP_VERSION_CONTROL	(0x5C0C)
#define INFOFRAME_SDP_HB3_SEL			(0x01 << 1)
#define INFOFRAME_SDP_VERSION_SEL		(0x01 << 0)

#define SST1_INFOFRAME_SPD_PACKET_TYPE		(0x5C10)
#define SPD_TYPE				(0xFF << 0)

#define SST1_INFOFRAME_SPD_REUSE_PACKET_CONTROL	(0x5C14)
#define SPD_REUSE_EN				(0x01 << 0)

#define SST1_PPS_SDP_CONTROL			(0x5C20)
#define PPS_SDP_CHANGE_STATUS			(0x01 << 2)
#define PPS_SDP_FRAME_SEND_ENABLE		(0x01 << 1)
#define PPS_SDP_UPDATE				(0x01 << 0)

#define SST1_VSC_SDP_CONTROL_1			(0x5C24)
#define VSC_TOTAL_BYTES_IN_SDP			(0xFFF << 8)
#define VSC_CHANGE_STATUS			(0x01 << 5)
#define VSC_FORCE_PACKET_MARGIN			(0x01 << 4)
#define VSC_FIX_PACKET_SEQUENCE			(0x01 << 3)
#define VSC_EXT_VESA_CEA			(0x01 << 2)
#define VSC_SDP_FRAME_SEND_ENABLE		(0x01 << 1)
#define VSC_SDP_UPDATE				(0x01 << 0)

#define SST1_VSC_SDP_CONTROL_2			(0x5C28)
#define VSC_SETUP_TIME				(0xFFF << 20)
#define VSC_PACKET_MARGIN			(0x1FFF << 0)

#define SST1_MST_WAIT_TIMER_CONTROL_1		(0x5C2C)
#define AUDIO_WAIT_TIMER			(0x1FFF << 16)
#define INFOFRAME_WAIT_TIMER			(0x1FFF << 0)

#define SST1_MST_WAIT_TIMER_CONTROL_2		(0x5C30)
#define PPS_WAIT_TIMER				(0x1FFF << 16)
#define VSC_PACKET_WAIT_TIMER			(0x1FFF << 0)

#define SST1_INFOFRAME_AVI_PACKET_DATA_SET0	(0x5C40)
#define AVI_DB4					(0xFF << 24)
#define AVI_DB3					(0xFF << 16)
#define AVI_DB2					(0xFF << 8)
#define AVI_DB1					(0xFF << 0)

#define SST1_INFOFRAME_AVI_PACKET_DATA_SET1	(0x5C44)
#define AVI_DB8					(0xFF << 24)
#define AVI_DB7					(0xFF << 16)
#define AVI_DB6					(0xFF << 8)
#define AVI_DB5					(0xFF << 0)

#define SST1_INFOFRAME_AVI_PACKET_DATA_SET2	(0x5C48)
#define AVI_DB12				(0xFF << 24)
#define AVI_DB11				(0xFF << 16)
#define AVI_DB10				(0xFF << 8)
#define AVI_DB9					(0xFF << 0)

#define SST1_INFOFRAME_AVI_PACKET_DATA_SET3	(0x5C4C)
#define AVI_DB13				(0xFF << 0)

#define SST1_INFOFRAME_AUDIO_PACKET_DATA_SET0	(0x5C50)
#define AUDIO_DB4				(0xFF << 24)
#define AUDIO_DB3				(0xFF << 16)
#define AUDIO_DB2				(0xFF << 8)
#define AUDIO_DB1				(0xFF << 0)

#define SST1_INFOFRAME_AUDIO_PACKET_DATA_SET1	(0x5C54)
#define AUDIO_DB8				(0xFF << 24)
#define AUDIO_DB7				(0xFF << 16)
#define AUDIO_DB6				(0xFF << 8)
#define AUDIO_DB5				(0xFF << 0)

#define SST1_INFOFRAME_AUDIO_PACKET_DATA_SET2	(0x5C58)
#define AVI_DB10				(0xFF << 8)
#define AVI_DB9					(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET0	(0x5C60)
#define SPD_DB4					(0xFF << 24)
#define SPD_DB3					(0xFF << 16)
#define SPD_DB2					(0xFF << 8)
#define SPD_DB1					(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET1	(0x5C64)
#define SPD_DB8					(0xFF << 24)
#define SPD_DB7					(0xFF << 16)
#define SPD_DB6					(0xFF << 8)
#define SPD_DB5					(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET2	(0x5C68)
#define SPD_DB12				(0xFF << 24)
#define SPD_DB11				(0xFF << 16)
#define SPD_DB10				(0xFF << 8)
#define SPD_DB9					(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET3	(0x5C6C)
#define SPD_DB16				(0xFF << 24)
#define SPD_DB15				(0xFF << 16)
#define SPD_DB14				(0xFF << 8)
#define SPD_DB13				(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET4	(0x5C70)
#define SPD_DB20				(0xFF << 24)
#define SPD_DB19				(0xFF << 16)
#define SPD_DB18				(0xFF << 8)
#define SPD_DB17				(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET5	(0x5C74)
#define SPD_DB24				(0xFF << 24)
#define SPD_DB23				(0xFF << 16)
#define SPD_DB22				(0xFF << 8)
#define SPD_DB21				(0xFF << 0)

#define SST1_INFOFRAME_SPD_PACKET_DATA_SET6	(0x5C78)
#define SPD_DB25				(0xFF << 0)

#define SST1_INFOFRAME_MPEG_PACKET_DATA_SET0	(0x5C80)
#define MPEG_DB4				(0xFF << 24)
#define MPEG_DB3				(0xFF << 16)
#define MPEG_DB2				(0xFF << 8)
#define MPEG_DB1				(0xFF << 0)

#define SST1_INFOFRAME_MPEG_PACKET_DATA_SET1	(0x5C84)
#define MPEG_DB8				(0xFF << 24)
#define MPEG_DB7				(0xFF << 16)
#define MPEG_DB6				(0xFF << 8)
#define MPEG_DB5				(0xFF << 0)

#define SST1_INFOFRAME_MPEG_PACKET_DATA_SET2	(0x5C88)
#define MPEG_DB10				(0xFF << 8)
#define MPEG_DB9				(0xFF << 0)

#define SST1_INFOFRAME_SPD_REUSE_PACKET_HEADER_SET	(0x5C90)
#define SPD_REUSE_HB3				(0xFF << 24)
#define SPD_REUSE_HB2				(0xFF << 16)
#define SPD_REUSE_HB1				(0xFF << 8)
#define SPD_REUSE_HB0				(0xFF << 0)

#define SST1_INFOFRAME_SPD_REUSE_PACKET_PARITY_SET	(0x5C94)
#define SPD_REUSE_PB3				(0xFF << 24)
#define SPD_REUSE_PB2				(0xFF << 16)
#define SPD_REUSE_PB1				(0xFF << 8)
#define SPD_REUSE_PB0				(0xFF << 0)

#define SST1_HDR_PACKET_DATA_SET_0		(0x5CA0)
#define HDR_INFOFRAME_DATA_0			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_1		(0x5CA4)
#define HDR_INFOFRAME_DATA_1			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_2		(0x5CA8)
#define HDR_INFOFRAME_DATA_2			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_3		(0x5CAC)
#define HDR_INFOFRAME_DATA_3			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_4		(0x5CB0)
#define HDR_INFOFRAME_DATA_4			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_5		(0x5CB4)
#define HDR_INFOFRAME_DATA_5			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_6		(0x5CB8)
#define HDR_INFOFRAME_DATA_6			(0xFFFFFFFF << 0)

#define SST1_HDR_PACKET_DATA_SET_7		(0x5CBC)
#define HDR_INFOFRAME_DATA_7			(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_0			(0x5D00)
#define PPS_SDP_DATA_0				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_1			(0x5D04)
#define PPS_SDP_DATA_1				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_2			(0x5D08)
#define PPS_SDP_DATA_2				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_3			(0x5D0C)
#define PPS_SDP_DATA_3				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_4			(0x5D10)
#define PPS_SDP_DATA_4				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_5			(0x5D14)
#define PPS_SDP_DATA_5				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_6			(0x5D18)
#define PPS_SDP_DATA_6				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_7			(0x5D1C)
#define PPS_SDP_DATA_7				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_8			(0x5D20)
#define PPS_SDP_DATA_8				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_9			(0x5D24)
#define PPS_SDP_DATA_9				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_10			(0x5D28)
#define PPS_SDP_DATA_10				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_11			(0x5D2C)
#define PPS_SDP_DATA_11				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_12			(0x5D30)
#define PPS_SDP_DATA_12				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_13			(0x5D34)
#define PPS_SDP_DATA_13				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_14			(0x5D38)
#define PPS_SDP_DATA_14				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_15			(0x5D3C)
#define PPS_SDP_DATA_15				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_16			(0x5D40)
#define PPS_SDP_DATA_16				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_17			(0x5D44)
#define PPS_SDP_DATA_17				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_18			(0x5D48)
#define PPS_SDP_DATA_18				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_19			(0x5D4C)
#define PPS_SDP_DATA_19				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_20			(0x5D50)
#define PPS_SDP_DATA_20				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_21			(0x5D54)
#define PPS_SDP_DATA_21				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_22			(0x5D58)
#define PPS_SDP_DATA_22				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_23			(0x5D5C)
#define PPS_SDP_DATA_23				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_24			(0x5D60)
#define PPS_SDP_DATA_24				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_25			(0x5D64)
#define PPS_SDP_DATA_25				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_26			(0x5D68)
#define PPS_SDP_DATA_26				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_27			(0x5D6C)
#define PPS_SDP_DATA_27				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_28			(0x5D70)
#define PPS_SDP_DATA_28				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_29			(0x5D74)
#define PPS_SDP_DATA_29				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_30			(0x5D78)
#define PPS_SDP_DATA_30				(0xFFFFFFFF << 0)

#define SST1_PPS_SDP_PAYLOAD_31			(0x5D7C)
#define PPS_SDP_DATA_31				(0xFFFFFFFF << 0)

#define SST1_VSC_SDP_DATA_PAYLOAD_FIFO		(0x5D80)
#define VSC_SDP_DATA_PAYLOAD_FIFO		(0xFFFFFFFF << 0)

#define SST2_MAIN_CONTROL			(0x6000)
#define SST2_MAIN_FIFO_CONTROL			(0x6004)
#define SST2_GNS_CONTROL			(0x6008)
#define SST2_SR_CONTROL				(0x600C)
#define SST2_INTERRUPT_MONITOR			(0x6020)
#define SST2_INTERRUPT_STATUS_SET0		(0x6024)
#define SST2_INTERRUPT_STATUS_SET1		(0x6028)
#define SST2_INTERRUPT_MASK_SET0		(0x602C)
#define SST2_INTERRUPT_MASK_SET1		(0x6030)
#define SST2_MVID_CALCULATION_CONTROL		(0x6040)
#define SST2_MVID_MASTER_MODE			(0x6044)
#define SST2_NVID_MASTER_MODE			(0x6048)
#define SST2_MVID_SFR_CONFIGURE			(0x604C)
#define SST2_NVID_SFR_CONFIGURE			(0x6050)
#define SST2_MVID_MONITOR			(0x6054)
#define SST2_MAUD_CALCULATION_CONTROL		(0x6058)
#define SST2_MAUD_MASTER_MODE			(0x605C)
#define SST2_NAUD_MASTER_MODE			(0x6060)
#define SST2_MAUD_SFR_CONFIGURE			(0x6064)
#define SST2_NAUD_SFR_CONFIGURE			(0x6068)
#define SST2_NARROW_BLANK_CONTROL		(0x606C)
#define SST2_LOW_TU_CONTROL			(0x6070)
#define SST2_ACTIVE_SYMBOL_INTEGER_FEC_OFF	(0x6080)
#define SST2_ACTIVE_SYMBOL_FRACTION_FEC_OFF	(0x6084)
#define SST2_ACTIVE_SYMBOL_THRESHOLD_FEC_OFF	(0x6088)
#define SST2_ACTIVE_SYMBOL_THRESHOLD_SEL_FEC_OFF	(0x608C)
#define SST2_ACTIVE_SYMBOL_INTEGER_FEC_ON	(0x6090)
#define SST2_ACTIVE_SYMBOL_FRACTION_FEC_ON	(0x6094)
#define SST2_ACTIVE_SYMBOL_THRESHOLD_FEC_ON	(0x6098)
#define SST2_ACTIVE_SYMBOL_THRESHOLD_SEL_FEC_ON	(0x609C)
#define SST2_FEC_DISABLE_SEND_CONTROL		(0x6100)
#define SST2_ACTIVE_SYMBOL_MODE_CONTROL		(0x6104)
#define SST2_VIDEO_CONTROL			(0x6400)
#define SST2_VIDEO_ENABLE			(0x6404)
#define SST2_VIDEO_MASTER_TIMING_GEN		(0x6408)
#define SST2_VIDEO_MUTE				(0x640C)
#define SST2_VIDEO_FIFO_THRESHOLD_CONTROL	(0x6410)
#define SST2_VIDEO_HORIZONTAL_TOTAL_PIXELS	(0x6414)
#define SST2_VIDEO_VERTICAL_TOTAL_PIXELS	(0x6418)
#define SST2_VIDEO_HORIZONTAL_FRONT_PORCH	(0x641C)
#define SST2_VIDEO_HORIZONTAL_BACK_PORCH	(0x6420)
#define SST2_VIDEO_HORIZONTAL_ACTIVE		(0x6424)
#define SST2_VIDEO_VERTICAL_FRONT_PORCH		(0x6428)
#define SST2_VIDEO_VERTICAL_BACK_PORCH		(0x642C)
#define SST2_VIDEO_VERTICAL_ACTIVE		(0x6430)
#define SST2_VIDEO_DSC_STREAM_CONTROL_0		(0x6434)
#define SST2_VIDEO_DSC_STREAM_CONTROL_1		(0x6438)
#define SST2_VIDEO_DSC_STREAM_CONTROL_2		(0x643C)
#define SST2_VIDEO_BIST_CONTROL			(0x6450)
#define SST2_VIDEO_BIST_USER_DATA_R		(0x6454)
#define SST2_VIDEO_BIST_USER_DATA_G		(0x6458)
#define SST2_VIDEO_BIST_USER_DATA_B		(0x645C)
#define SST2_VIDEO_DEBUG_FSM_STATE		(0x6460)
#define SST2_VIDEO_DEBUG_MAPI			(0x6464)
#define SST2_VIDEO_DEBUG_ACTV_SYM_STEP_CNTL	(0x6468)
#define SST2_VIDEO_DEBUG_HOR_BLANK_AUD_BW_ADJ	(0x646C)
#define SST2_AUDIO_CONTROL			(0x6800)
#define SST2_AUDIO_ENABLE			(0x6804)
#define SST2_AUDIO_MASTER_TIMING_GEN		(0x6808)
#define SST2_AUDIO_DMA_REQUEST_LATENCY_CONFIG	(0x680C)
#define SST2_AUDIO_MUTE_CONTROL			(0x6810)
#define SST2_AUDIO_MARGIN_CONTROL		(0x6814)
#define SST2_AUDIO_DATA_WRITE_FIFO		(0x6818)
#define SST2_AUDIO_GTC_CONTROL			(0x6824)
#define SST2_AUDIO_GTC_VALID_BIT_CONTROL	(0x6828)
#define SST2_AUDIO_3DLPCM_PACKET_WAIT_TIMER	(0x682C)
#define SST2_AUDIO_BIST_CONTROL			(0x6830)
#define SST2_AUDIO_BIST_CHANNEL_STATUS_SET0	(0x6834)
#define SST2_AUDIO_BIST_CHANNEL_STATUS_SET1	(0x6838)
#define SST2_AUDIO_BUFFER_CONTROL		(0x683C)
#define SST2_AUDIO_CHANNEL_1_4_REMAP		(0x6840)
#define SST2_AUDIO_CHANNEL_5_8_REMAP		(0x6844)
#define SST2_AUDIO_CHANNEL_9_12_REMAP		(0x6848)
#define SST2_AUDIO_CHANNEL_13_16_REMAP		(0x684C)
#define SST2_AUDIO_CHANNEL_17_20_REMAP		(0x6850)
#define SST2_AUDIO_CHANNEL_21_24_REMAP		(0x6854)
#define SST2_AUDIO_CHANNEL_25_28_REMAP		(0x6858)
#define SST2_AUDIO_CHANNEL_29_32_REMAP		(0x685C)
#define SST2_AUDIO_CHANNEL_1_2_STATUS_CTRL_0	(0x6860)
#define SST2_AUDIO_CHANNEL_1_2_STATUS_CTRL_1	(0x6864)
#define SST2_AUDIO_CHANNEL_3_4_STATUS_CTRL_0	(0x6868)
#define SST2_AUDIO_CHANNEL_3_4_STATUS_CTRL_1	(0x686C)
#define SST2_AUDIO_CHANNEL_5_6_STATUS_CTRL_0	(0x6870)
#define SST2_AUDIO_CHANNEL_5_6_STATUS_CTRL_1	(0x6874)
#define SST2_AUDIO_CHANNEL_7_8_STATUS_CTRL_0	(0x6878)
#define SST2_AUDIO_CHANNEL_7_8_STATUS_CTRL_1	(0x687C)
#define SST2_AUDIO_CHANNEL_9_10_STATUS_CTRL_0	(0x6880)
#define SST2_AUDIO_CHANNEL_9_10_STATUS_CTRL_1	(0x6884)
#define SST2_AUDIO_CHANNEL_11_12_STATUS_CTRL_0	(0x6888)
#define SST2_AUDIO_CHANNEL_11_12_STATUS_CTRL_1	(0x688C)
#define SST2_AUDIO_CHANNEL_13_14_STATUS_CTRL_0	(0x6890)
#define SST2_AUDIO_CHANNEL_13_14_STATUS_CTRL_1	(0x6894)
#define SST2_AUDIO_CHANNEL_15_16_STATUS_CTRL_0	(0x6898)
#define SST2_AUDIO_CHANNEL_15_16_STATUS_CTRL_1	(0x689C)
#define SST2_AUDIO_CHANNEL_17_18_STATUS_CTRL_0	(0x68A0)
#define SST2_AUDIO_CHANNEL_17_18_STATUS_CTRL_1	(0x68A4)
#define SST2_AUDIO_CHANNEL_19_20_STATUS_CTRL_0	(0x68A8)
#define SST2_AUDIO_CHANNEL_19_20_STATUS_CTRL_1	(0x68AC)
#define SST2_AUDIO_CHANNEL_21_22_STATUS_CTRL_0	(0x68B0)
#define SST2_AUDIO_CHANNEL_21_22_STATUS_CTRL_1	(0x68B4)
#define SST2_AUDIO_CHANNEL_23_24_STATUS_CTRL_0	(0x68B8)
#define SST2_AUDIO_CHANNEL_23_24_STATUS_CTRL_1	(0x68BC)
#define SST2_AUDIO_CHANNEL_25_26_STATUS_CTRL_0	(0x68C0)
#define SST2_AUDIO_CHANNEL_25_26_STATUS_CTRL_1	(0x68C4)
#define SST2_AUDIO_CHANNEL_27_28_STATUS_CTRL_0	(0x68C8)
#define SST2_AUDIO_CHANNEL_27_28_STATUS_CTRL_1	(0x68CC)
#define SST2_AUDIO_CHANNEL_29_30_STATUS_CTRL_0	(0x68D0)
#define SST2_AUDIO_CHANNEL_29_30_STATUS_CTRL_1	(0x68D4)
#define SST2_AUDIO_CHANNEL_31_32_STATUS_CTRL_0	(0x68D8)
#define SST2_AUDIO_CHANNEL_31_32_STATUS_CTRL_1	(0x68DC)
#define SST2_STREAM_IF_CRC_CONTROL_1		(0x68E0)
#define SST2_STREAM_IF_CRC_CONTROL_2		(0x68E4)
#define SST2_STREAM_IF_CRC_CONTROL_3		(0x68E8)
#define SST2_STREAM_IF_CRC_CONTROL_4		(0x68EC)
#define SST2_AUDIO_DEBUG_MARGIN_CONTROL		(0x6900)
#define SST2_SDP_SPLITTING_CONTROL		(0x6C00)
#define SST2_INFOFRAME_UPDATE_CONTROL		(0x6C04)
#define SST2_INFOFRAME_SEND_CONTROL		(0x6C08)
#define SST2_INFOFRAME_SDP_VERSION_CONTROL	(0x6C0C)
#define SST2_INFOFRAME_SPD_PACKET_TYPE		(0x6C10)
#define SST2_INFOFRAME_SPD_REUSE_PACKET_CONTROL	(0x6C14)
#define SST2_PPS_SDP_CONTROL			(0x6C20)
#define SST2_VSC_SDP_CONTROL_1			(0x6C24)
#define SST2_VSC_SDP_CONTROL_2			(0x6C28)
#define SST2_MST_WAIT_TIMER_CONTROL_1		(0x6C2C)
#define SST2_MST_WAIT_TIMER_CONTROL_2		(0x6C30)
#define SST2_INFOFRAME_AVI_PACKET_DATA_SET0	(0x6C40)
#define SST2_INFOFRAME_AVI_PACKET_DATA_SET1	(0x6C44)
#define SST2_INFOFRAME_AVI_PACKET_DATA_SET2	(0x6C48)
#define SST2_INFOFRAME_AVI_PACKET_DATA_SET3	(0x6C4C)
#define SST2_INFOFRAME_AUDIO_PACKET_DATA_SET0	(0x6C50)
#define SST2_INFOFRAME_AUDIO_PACKET_DATA_SET1	(0x6C54)
#define SST2_INFOFRAME_AUDIO_PACKET_DATA_SET2	(0x6C58)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET0	(0x6C60)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET1	(0x6C64)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET2	(0x6C68)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET3	(0x6C6C)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET4	(0x6C70)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET5	(0x6C74)
#define SST2_INFOFRAME_SPD_PACKET_DATA_SET6	(0x6C78)
#define SST2_INFOFRAME_MPEG_PACKET_DATA_SET0	(0x6C80)
#define SST2_INFOFRAME_MPEG_PACKET_DATA_SET1	(0x6C84)
#define SST2_INFOFRAME_MPEG_PACKET_DATA_SET2	(0x6C88)
#define SST2_INFOFRAME_SPD_REUSE_PACKET_HEADER_SET	(0x6C90)
#define SST2_INFOFRAME_SPD_REUSE_PACKET_PARITY_SET	(0x6C94)
#define SST2_HDR_PACKET_DATA_SET_0		(0x6CA0)
#define SST2_HDR_PACKET_DATA_SET_1		(0x6CA4)
#define SST2_HDR_PACKET_DATA_SET_2		(0x6CA8)
#define SST2_HDR_PACKET_DATA_SET_3		(0x6CAC)
#define SST2_HDR_PACKET_DATA_SET_4		(0x6CB0)
#define SST2_HDR_PACKET_DATA_SET_5		(0x6CB4)
#define SST2_HDR_PACKET_DATA_SET_6		(0x6CB8)
#define SST2_HDR_PACKET_DATA_SET_7		(0x6CBC)
#define SST2_PPS_SDP_PAYLOAD_0			(0x6D00)
#define SST2_PPS_SDP_PAYLOAD_1			(0x6D04)
#define SST2_PPS_SDP_PAYLOAD_2			(0x6D08)
#define SST2_PPS_SDP_PAYLOAD_3			(0x6D0C)
#define SST2_PPS_SDP_PAYLOAD_4			(0x6D10)
#define SST2_PPS_SDP_PAYLOAD_5			(0x6D14)
#define SST2_PPS_SDP_PAYLOAD_6			(0x6D18)
#define SST2_PPS_SDP_PAYLOAD_7			(0x6D1C)
#define SST2_PPS_SDP_PAYLOAD_8			(0x6D20)
#define SST2_PPS_SDP_PAYLOAD_9			(0x6D24)
#define SST2_PPS_SDP_PAYLOAD_10			(0x6D28)
#define SST2_PPS_SDP_PAYLOAD_11			(0x6D2C)
#define SST2_PPS_SDP_PAYLOAD_12			(0x6D30)
#define SST2_PPS_SDP_PAYLOAD_13			(0x6D34)
#define SST2_PPS_SDP_PAYLOAD_14			(0x6D38)
#define SST2_PPS_SDP_PAYLOAD_15			(0x6D3C)
#define SST2_PPS_SDP_PAYLOAD_16			(0x6D40)
#define SST2_PPS_SDP_PAYLOAD_17			(0x6D44)
#define SST2_PPS_SDP_PAYLOAD_18			(0x6D48)
#define SST2_PPS_SDP_PAYLOAD_19			(0x6D4C)
#define SST2_PPS_SDP_PAYLOAD_20			(0x6D50)
#define SST2_PPS_SDP_PAYLOAD_21			(0x6D54)
#define SST2_PPS_SDP_PAYLOAD_22			(0x6D58)
#define SST2_PPS_SDP_PAYLOAD_23			(0x6D5C)
#define SST2_PPS_SDP_PAYLOAD_24			(0x6D60)
#define SST2_PPS_SDP_PAYLOAD_25			(0x6D64)
#define SST2_PPS_SDP_PAYLOAD_26			(0x6D68)
#define SST2_PPS_SDP_PAYLOAD_27			(0x6D6C)
#define SST2_PPS_SDP_PAYLOAD_28			(0x6D70)
#define SST2_PPS_SDP_PAYLOAD_29			(0x6D74)
#define SST2_PPS_SDP_PAYLOAD_30			(0x6D78)
#define SST2_PPS_SDP_PAYLOAD_31			(0x6D7C)
#define SST2_VSC_SDP_DATA_PAYLOAD_FIFO		(0x6D80)

/* PHY register */
#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
#define DEFAULT_SFR_CNT				91
#else /* EVT1 */
#define DEFAULT_SFR_CNT				32
#endif

#define CMN_REG0009				(0x0024)
#define ANA_AUX_TX_LVL_CTRL			(0x0F << 3)

#define CMN_REG00A2				(0x0288)
#define LANE_MUX_SEL_DP_LN3			(0x01 << 7)
#define LANE_MUX_SEL_DP_LN2			(0x01 << 6)
#define LANE_MUX_SEL_DP_LN1			(0x01 << 5)
#define LANE_MUX_SEL_DP_LN0			(0x01 << 4)
#define DP_LANE_EN_LN3				(0x01 << 3)
#define DP_LANE_EN_LN2				(0x01 << 2)
#define DP_LANE_EN_LN1				(0x01 << 1)
#define DP_LANE_EN_LN0				(0x01 << 0)

#define CMN_REG00A3				(0x028C)
#define DP_TX_LINK_BW				(0x03 << 5)
#define OVRD_RX_CDR_DATA_MODE_EXIT		(0x01 << 4)

#define CMN_REG00B4				(0x02D0)
#define ROPLL_SSC_EN				(0x01 << 1)

#define CMN_REG00E3				(0x038C)
#define DP_INIT_RSTN				(0x01 << 3)
#define DP_CMN_RSTN				(0x01 << 2)
#define CDR_WATCHDOG_EN				(0x01 << 1)

#define TRSV_REG0204				(0x0810)
#define LN0_TX_DRV_LVL_CTRL			(0x1F << 0)

#define TRSV_REG0215				(0x0854)
#define LN0_ANA_TX_SER_TXCLK_INV	(0x01 << 1)

#define TRSV_REG0400				(0x1000)
#define OVRD_LN1_TX_DRV_BECON_LFPS_OUT_EN	(0x01 << 5)

#define TRSV_REG0404				(0x1010)
#define LN1_TX_DRV_LVL_CTRL			(0x1F << 0)

#define TRSV_REG0415				(0x1054)
#define LN1_ANA_TX_SER_TXCLK_INV	(0x01 << 1)

#define TRSV_REG0604				(0x1810)
#define LN2_TX_DRV_LVL_CTRL			(0x1F << 0)

#define TRSV_REG0615				(0x1854)
#define LN2_ANA_TX_SER_TXCLK_INV	(0x01 << 1)

#define TRSV_REG0800				(0x2000)
#define OVRD_LN3_TX_DRV_BECON_LFPS_OUT_EN	(0x01 << 5)

#define TRSV_REG0804				(0x2010)
#define LN3_TX_DRV_LVL_CTRL			(0x1F << 0)

#define TRSV_REG0815				(0x2054)
#define LN3_ANA_TX_SER_TXCLK_INV	(0x01 << 1)
#endif
