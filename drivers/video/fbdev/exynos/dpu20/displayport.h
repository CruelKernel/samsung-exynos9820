/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Header file for Samsung EXYNOS SoC DisplayPort driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _DISPLAYPORT_H_
#define _DISPLAYPORT_H_

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-dv-timings.h>
#include <uapi/linux/v4l2-dv-timings.h>
#include <linux/phy/phy.h>
#if defined(CONFIG_EXTCON)
#include <linux/extcon.h>
#endif
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
#include <linux/usb/manager/usb_typec_manager_notifier.h>
#include <linux/notifier.h>
#include <linux/ccic/ccic_notifier.h>
#endif
#include <linux/dp_logger.h>
#include <linux/displayport_bigdata.h>

#if defined(CONFIG_SOC_EXYNOS9810)
#include "./cal_9810/regs-displayport.h"
#elif defined(CONFIG_SOC_EXYNOS9820)
#include "./cal_9820/regs-displayport.h"
#endif

#include "secdp_unit_test.h"
#include "./panels/decon_lcd.h"
#include "hdr_metadata.h"

#define FEATURE_SUPPORT_SPD_INFOFRAME

#define FEATURE_SUPPORT_DISPLAYID

#undef FEATURE_DP_UNDERRUN_TEST

#define DISPLAYID_EXT 0x70
#define FEATURE_MANAGE_HMD_LIST
#define FEATURE_DEX_ADAPTER_TWEAK

extern int displayport_log_level;
extern int forced_resolution;

#define DISPLAYPORT_MODULE_NAME "exynos-displayport"

#define displayport_err(fmt, ...)						\
	do {									\
		if (displayport_log_level >= 3) {				\
			pr_err("Displayport: " pr_fmt(fmt), ##__VA_ARGS__);			\
			dp_logger_print(fmt, ##__VA_ARGS__);                    \
		}								\
	} while (0)

#define displayport_warn(fmt, ...)						\
	do {									\
		if (displayport_log_level >= 4) {				\
			pr_warn("Displayport: " pr_fmt(fmt), ##__VA_ARGS__);			\
			dp_logger_print(fmt, ##__VA_ARGS__);                    \
		}								\
	} while (0)

#define displayport_info(fmt, ...)						\
	do {									\
		if (displayport_log_level >= 6)					\
			pr_info("Displayport: " pr_fmt(fmt), ##__VA_ARGS__);			\
			dp_logger_print(fmt, ##__VA_ARGS__);                    \
	} while (0)

#define displayport_dbg(fmt, ...)						\
	do {									\
		if (displayport_log_level >= 7)					\
			pr_info("Displayport: " pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)

extern struct displayport_device *displayport_drvdata;

enum displayport_state {
	DISPLAYPORT_STATE_INIT,
	DISPLAYPORT_STATE_ON,
	DISPLAYPORT_STATE_OFF
};

enum displayport_dynamic_range_type {
	VESA_RANGE = 0,   /* (0 ~ 255) */
	CEA_RANGE = 1,    /* (16 ~ 235) */
};

struct displayport_resources {
	int aux_ch_mux_gpio;
	int irq;
	void __iomem *link_regs;
	void __iomem *phy_regs;
	void __iomem *usbdp_regs;
	struct clk *aclk;
};

enum displayport_aux_ch_command_type {
	I2C_WRITE = 0x4,
	I2C_READ = 0x5,
	DPCD_WRITE = 0x8,
	DPCD_READ = 0x9,
};

typedef enum {
	NORAMAL_DATA = 0,
	TRAINING_PATTERN_1 = 1,
	TRAINING_PATTERN_2 = 2,
	TRAINING_PATTERN_3 = 3,
} displayport_training_pattern;

typedef enum {
	DISABLE_PATTEN = 0,
	D10_2_PATTERN = 1,
	SERP_PATTERN = 2,
	PRBS7 = 3,
	CUSTOM_80BIT = 4,
	HBR2_COMPLIANCE = 5,
} displayport_qual_pattern;

typedef enum {
	ENABLE_SCRAM = 0,
	DISABLE_SCRAM = 1,
}displayport_scrambling;

enum displayport_interrupt_mask {
	PLL_LOCK_CHG_INT_MASK,
	HOTPLUG_CHG_INT_MASK,
	HPD_LOST_INT_MASK,
	PLUG_INT_MASK,
	HPD_IRQ_INT_MASK,
	RPLY_RECEIV_INT_MASK,
	AUX_ERR_INT_MASK,
	HDCP_LINK_CHECK_INT_MASK,
	HDCP_LINK_FAIL_INT_MASK,
	HDCP_R0_READY_INT_MASK,
	VIDEO_FIFO_UNDER_FLOW_MASK,
	VSYNC_DET_INT_MASK,
	AUDIO_FIFO_UNDER_RUN_INT_MASK,
	AUDIO_FIFO_OVER_RUN_INT_MASK,

	ALL_INT_MASK
};

#define MAX_LANE_CNT 4
#define MAX_LINK_RATE_NUM 3
#define RBR_PIXEL_CLOCK_PER_LANE 54000000 /* hz */
#define HBR_PIXEL_CLOCK_PER_LANE 90000000 /* hz */
#define HBR2_PIXEL_CLOCK_PER_LANE 180000000 /* hz */
#define DPCD_BUF_SIZE 12

#define FB_AUDIO_LPCM	1

#define FB_AUDIO_192KHZ	(1 << 6)
#define FB_AUDIO_176KHZ	(1 << 5)
#define FB_AUDIO_96KHZ	(1 << 4)
#define FB_AUDIO_88KHZ	(1 << 3)
#define FB_AUDIO_48KHZ	(1 << 2)
#define FB_AUDIO_44KHZ	(1 << 1)
#define FB_AUDIO_32KHZ	(1 << 0)

#define FB_AUDIO_24BIT	(1 << 2)
#define FB_AUDIO_20BIT	(1 << 1)
#define FB_AUDIO_16BIT	(1 << 0)

#define FB_AUDIO_1CH (1)
#define FB_AUDIO_2CH (1 << 1)
#define FB_AUDIO_6CH (1 << 5)
#define FB_AUDIO_8CH (1 << 7)
#define FB_AUDIO_1N2CH (FB_AUDIO_1CH | FB_AUDIO_2CH)

struct fb_audio {
	u8 format;
	u8 channel_count;
	u8 sample_rates;
	u8 bit_rates;
	u8 speaker;
};

struct fb_vendor {
	u8 vic_len;
	u8 vic_data[16];
};

#define MAX_RETRY_CNT 16
#define MAX_REACHED_CNT 3
#define MAX_SWING_REACHED_BIT_POS 2
#define MAX_PRE_EMPHASIS_REACHED_BIT_POS 5
#define DPCP_LINK_SINK_STATUS_FIELD_LENGTH 8

#define DPCD_ADD_REVISION_NUMBER 0x00000
#define DPCD_ADD_MAX_LINK_RATE 0x00001
#define LINK_RATE_1_62Gbps 0x06
#define LINK_RATE_2_7Gbps 0x0A
#define LINK_RATE_5_4Gbps 0x14

#define UHD_60HZ_PIXEL_CLOCK 500000000

#define DPCD_ADD_MAX_LANE_COUNT 0x00002
#define MAX_LANE_COUNT (0x1F << 0)
#define TPS3_SUPPORTED (1 << 6)
#define ENHANCED_FRAME_CAP (1 << 7)

#define DPCD_ADD_MAX_DOWNSPREAD 0x00003
#define NO_AUX_HANDSHAKE_LINK_TRANING (1 << 6)

#define DPCD_ADD_DOWN_STREAM_PORT_PRESENT 0x00005
#define BIT_DFP_TYPE 0x6
#define DFP_TYPE_DP 0x00
#define DFP_TYPE_VGA 0x01 /* analog video */
#define DFP_TYPE_HDMI 0x2
#define DFP_TYPE_OTHERS 0x3 /* not have EDID like composite and Svideo port */

#define DPCD_ADD_I2C_SPEED_CONTROL_CAPABILITES 0x0000C
#define I2C_1Kbps 0x01
#define I2C_5Kbps 0x02
#define I2C_10Kbps 0x04
#define I2C_100Kbps 0x08
#define I2C_400Kbps 0x10
#define I2C_1Mbps 0x20

#define DPCD_ADD_TRAINING_AUX_RD_INTERVAL 0x0000E
#define TRANING_AUX_RD_INTERVAL_400us 0x00
#define TRANING_AUX_RD_INTERVAL_4ms 0x01
#define TRANING_AUX_RD_INTERVAL_8ms 0x02
#define TRANING_AUX_RD_INTERVAL_12ms 0x03
#define TRANING_AUX_RD_INTERVAL_16ms 0x04

#define DPCD_ADD_LINK_BW_SET 0x00100

#define DPCD_ADD_LANE_COUNT_SET 0x00101

#define DPCD_ADD_TRANING_PATTERN_SET 0x00102
#define TRAINING_PTTERN_SELECT (3 << 0)
#define RECOVERED_CLOCK_OUT_EN (1 << 4)
#define DPCD_SCRAMBLING_DISABLE (1 << 5)
#define SYMBOL_ERROR_COUNT_SEL (3 << 6)

#define DPCD_ADD_TRANING_LANE0_SET 0x00103
#define DPCD_ADD_TRANING_LANE1_SET 0x00104
#define DPCD_ADD_TRANING_LANE2_SET 0x00105
#define DPCD_ADD_TRANING_LANE3_SET 0x00106
#define VOLTAGE_SWING_SET (3 << 0)
#define MAX_SWING_REACHED (1 << 2)
#define PRE_EMPHASIS_SWING_SET (3 << 3)
#define MAX_PRE_EMPHASIS_REACHED (1 << 5)

#define DPCD_ADD_I2C_SPEED_CONTROL_STATUS 0x00109

#define DPCD_ADD_LINK_QUAL_LANE0_SET 0x0010B
#define DPCD_ADD_LINK_QUAL_LANE1_SET 0x0010C
#define DPCD_ADD_LINK_QUAL_LANE2_SET 0x0010D
#define DPCD_ADD_LINK_QUAL_LANE3_SET 0x0010E
#define DPCD_LINK_QUAL_PATTERN_SET (7 << 0)

#define DPCD_ADD_SINK_COUNT 0x00200
#define SINK_COUNT2 (1 << 7)
#define CP_READY (1 << 6)
#define SINK_COUNT1 (0x3F << 0)

#define DPCD_ADD_DEVICE_SERVICE_IRQ_VECTOR 0x00201
#define AUTOMATED_TEST_REQUEST (1 << 1)
#define CP_IRQ (1 << 2)
#define MCCS_IRQ (1 << 3)
#define DOWN_REP_MSG_RDY (1 << 4)
#define UP_REQ_MSG_RDY (1 << 5)
#define SINK_SPECIFIC_IRQ (1 << 6)

#define DPCD_ADD_LANE0_1_STATUS 0x00202
#define LANE0_CR_DONE (1 << 0)
#define LANE0_CHANNEL_EQ_DONE (1 << 1)
#define LANE0_SYMBOL_LOCKED (1 << 2)
#define LANE1_CR_DONE (1 << 4)
#define LANE1_CHANNEL_EQ_DONE (1 << 5)
#define LANE1_SYMBOL_LOCKED (1 << 6)

#define DPCD_ADD_LANE2_3_STATUS 0x00203
#define LANE2_CR_DONE (1 << 0)
#define LANE2_CHANNEL_EQ_DONE (1 << 1)
#define LANE2_SYMBOL_LOCKED (1 << 2)
#define LANE3_CR_DONE (1 << 4)
#define LANE3_CHANNEL_EQ_DONE (1 << 5)
#define LANE3_SYMBOL_LOCKED (1 << 6)

#define DPCD_ADD_LANE_ALIGN_STATUS_UPDATE 0x00204
#define INTERLANE_ALIGN_DONE (1 << 0)
#define DOWNSTREAM_PORT_STATUS_CHANGED (1 << 6)
#define LINK_STATUS_UPDATE (1 << 7)

#define DPCD_ADD_SINK_STATUS 0x00205
#define RECEIVE_PORT_0_STATUS (1 << 0)
#define RECEIVE_PORT_1_STATUS (1 << 1)

#define DPCD_ADD_ADJUST_REQUEST_LANE0_1 0x00206
#define VOLTAGE_SWING_LANE0 (3 << 0)
#define PRE_EMPHASIS_LANE0 (3 << 2)
#define VOLTAGE_SWING_LANE1 (3 << 4)
#define PRE_EMPHASIS_LANE1 (3 << 6)

#define DPCD_ADD_ADJUST_REQUEST_LANE2_3 0x00207
#define VOLTAGE_SWING_LANE2 (3 << 0)
#define PRE_EMPHASIS_LANE2 (3 << 2)
#define VOLTAGE_SWING_LANE3 (3 << 4)
#define PRE_EMPHASIS_LANE3 (3 << 6)

#define DPCD_TEST_REQUEST 0x00218
#define TEST_LINK_TRAINING (1 << 0)
#define TEST_VIDEO_PATTERN (1 << 1)
#define TEST_EDID_READ (1 << 2)
#define TEST_PHY_TEST_PATTERN (1 << 3)
#define TEST_FAUX_TEST_PATTERN (1<<4)
#define TEST_AUDIO_PATTERN (1<<5)
#define TEST_AUDIO_DISABLED_VIDEO (1<<6)

#define DPCD_TEST_LINK_RATE 0x00219
#define TEST_LINK_RATE (0xFF << 0)

#define DPCD_TEST_LANE_COUNT 0x00220
#define TEST_LANE_COUNT (0x1F << 0)

#define DPCD_TEST_PATTERN 0x00221
#define DPCD_TEST_PATTERN_COLOR_RAMPS 0x01
#define DPCD_TEST_PATTERN_BW_VERTICAL_LINES 0x02
#define DPCD_TEST_PATTERN_COLOR_SQUARE 0x03


#define DPCD_TEST_H_TOTAL_1 0x00222	//[15:8]
#define DPCD_TEST_H_TOTAL_2 0x00223	//[7:0]
#define DPCD_TEST_V_TOTAL_1 0x00224	//[15:8]
#define DPCD_TEST_V_TOTAL_2 0x00225	//[7:0]

#define DPCD_TEST_H_START_1 0x00226	//[15:8]
#define DPCD_TEST_H_START_2 0x00227	//[7:0]
#define DPCD_TEST_V_START_1 0x00228	//[15:8]
#define DPCD_TEST_V_START_2 0x00229	//[7:0]

#define DPCD_TEST_H_SYNC_1 0x0022A	//[15:8]
#define DPCD_TEST_H_SYNC_2 0x0022B	//[7:0]
#define DPCD_TEST_V_SYNC_1 0x0022C	//[15:8]
#define DPCD_TEST_V_SYNC_2 0x0022D	//[7:0]

#define DPCD_TEST_H_WIDTH_1 0x0022E	//[15:8]
#define DPCD_TEST_H_WIDTH_2 0x0022F	//[7:0]
#define DPCD_TEST_V_HEIGHT_1 0x00230	//[15:8]
#define DPCD_TEST_V_HEIGHT_2 0x00231	//[7:0]

#define DPCD_TEST_MISC_1 0x00232
#define TEST_SYNCHRONOUS_CLOCK (1 << 0)
#define TEST_COLOR_FORMAT (3 << 1)
#define TEST_DYNAMIC_RANGE (1 << 3)
#define TEST_YCBCR_COEFFICIENTS (1 << 4)
#define TEST_BIT_DEPTH (7 << 5)

#define DPCD_TEST_MISC_2 0x00233
#define TEST_REFRESH_DENOMINATOR (1 << 0)
#define TEST_INTERLACED (1 << 1)

#define DPCD_TEST_REFRESH_RATE_NUMERATOR 0x00234

#define DCDP_ADD_PHY_TEST_PATTERN 0x00248
#define PHY_TEST_PATTERN_SEL (3 << 0)

#define DPCD_TEST_RESPONSE 0x00260
#define TEST_ACK (1 << 0)
#define TEST_NAK (1 << 1)
#define TEST_EDID_CHECKSUM_WRITE (1 << 2)

#define DPCD_TEST_EDID_CHECKSUM 0x00261

#define DPCD_TEST_AUDIO_MODE 0x00271
#define TEST_AUDIO_SAMPLING_RATE (0x0F << 0)
#define TEST_AUDIO_CHANNEL_COUNT (0xF0 << 0)

#define DPCD_TEST_AUDIO_PATTERN_TYPE 0x00272

#define DPCD_BRANCH_HW_REVISION	0x509
#define DPCD_BRANCH_SW_REVISION_MAJOR	0x50A
#define DPCD_BRANCH_SW_REVISION_MINOR	0x50B

#define DPCD_ADD_SET_POWER 0x00600
#define SET_POWER_STATE (3 << 0)
#define SET_POWER_DOWN 0x02
#define SET_POWER_NORMAL 0x01

#define DPCD_HDCP22_RX_CAPS 0x6921D
#define VERSION (0xFF << 16)
#define HDCP_CAPABLE (1 << 1)

#define SMC_CHECK_STREAM_TYPE_ID		((unsigned int)0x82004022)
#define DPCD_HDCP22_RX_INFO 0x69330

#define DPCD_HDCP22_RX_CAPS_LENGTH 3
#define DPCD_HDCP_VERSION_BIT_POSITION 16

#define DPCD_HDCP22_RXSTATUS_READY			(0x1 << 0)
#define DPCD_HDCP22_RXSTATUS_HPRIME_AVAILABLE		(0x1 << 1)
#define DPCD_HDCP22_RXSTATUS_PAIRING_AVAILABLE		(0x1 << 2)
#define DPCD_HDCP22_RXSTATUS_REAUTH_REQ			(0x1 << 3)
#define DPCD_HDCP22_RXSTATUS_LINK_INTEGRITY_FAIL	(0x1 << 4)

#define HDCP_VERSION_1_3 0x13
#define HDCP_VERSION_2_2 0x02

enum drm_state {
	DRM_OFF = 0x0,
	DRM_ON = 0x1,
	DRM_SAME_STREAM_TYPE = 0x2      /* If the previous contents and stream_type id are the same flag */
};

enum auth_state {
	HDCP_AUTH_PROCESS_IDLE  = 0x1,
	HDCP_AUTH_PROCESS_STOP  = 0x2,
	HDCP_AUTH_PROCESS_DONE  = 0x3
};

enum auth_signal {
        HDCP_DRM_OFF    = 0x100,
        HDCP_DRM_ON     = 0x200,
        HDCP_RP_READY   = 0x300,
};

#define SYNC_POSITIVE 0
#define SYNC_NEGATIVE 1

#define AUDIO_BUF_FULL_SIZE 40
#define AUDIO_DISABLE 0
#define AUDIO_ENABLE 1
#define AUDIO_WAIT_BUF_FULL 2
#define AUDIO_DMA_REQ_HIGH 3

enum phy_tune_info {
	AMP = 0,
	POST_EMP = 1,
	PRE_EMP = 2,
	IDRV = 3,
	ACCDRV = 4,
};

typedef enum {
	V640X480P60,
	V720X480P60,
	V720X576P50,
	V1280X800P60RB,
	V1280X720P50,
	V1280X720P60EXT,
	V1280X720P60,
	V1366X768P60,
	V1280X1024P60,
	V1920X1080P24,
	V1920X1080P25,
	V1920X1080P30,
	V1600X900P59,
	V1600X900P60RB,
	V1920X1080P50,
	V1920X1080P60EXT,
	V1920X1080P59,
	V1920X1080P60,
	V1920X1200P60RB,
	V1920X1200P60,
	V2560X1080P60,
	V2048X1536P60,
	V1920X1440P60,
	V2400X1200P90RELU,
	V2560X1440P60EXT,
	V2560X1440P59,
	V1440x2560P60,
	V1440x2560P75,
	V2560X1440P60,
	V2560X1600P60,
	V3200X1600P70,
	V3200X1600P72,
	V3440X1440P50,
	V3840X1080P60,
	V3840X1200P60,
	V3440X1440P60,
/*	V3440X1440P100,*/
	V3840X2160P24,
	V3840X2160P25,
	V3840X2160P30,
	V4096X2160P24,
	V4096X2160P25,
	V4096X2160P30,
	V3840X2160P60EXT,
	V3840X2160P59RB,
	V3840X2160P50,
	V3840X2160P60,
	V2160X3840P72,
	V4096X2160P50,
	V4096X2160P60,
	V640X10P60SACRC,
	VDUMMYTIMING,
} videoformat;

typedef enum{
	ASYNC_MODE = 0,
	SYNC_MODE,
} audio_sync_mode;

enum audio_sampling_frequency {
	FS_32KHZ	= 0,
	FS_44KHZ	= 1,
	FS_48KHZ	= 2,
	FS_88KHZ	= 3,
	FS_96KHZ	= 4,
	FS_176KHZ	= 5,
	FS_192KHZ	= 6,
};

enum audio_bit_per_channel {
	AUDIO_16_BIT = 0,
	AUDIO_20_BIT,
	AUDIO_24_BIT,
};

enum audio_16bit_dma_mode {
	NORMAL_MODE = 0,
	PACKED_MODE = 1,
	PACKED_MODE2 = 2,
};

enum audio_dma_word_length {
	WORD_LENGTH_1 = 0,
	WORD_LENGTH_2,
	WORD_LENGTH_3,
	WORD_LENGTH_4,
	WORD_LENGTH_5,
	WORD_LENGTH_6,
	WORD_LENGTH_7,
	WORD_LENGTH_8,
};

enum audio_clock_accuracy {
	Level2 = 0,
	Level1 = 1,
	Level3 = 2,
	NOT_MATCH = 3,
};

enum bit_depth{
	BPC_6 = 0,
	BPC_8,
	BPC_10,
	BPC_12,
	BPC_16,
};

enum test_pattern{
	COLOR_BAR = 0,
	WGB_BAR,
	MW_BAR,
	CTS_COLOR_RAMP,
	CTS_BLACK_WHITE,
	CTS_COLOR_SQUARE_VESA,
	CTS_COLOR_SQUARE_CEA,
};

enum hotplug_state{
	HPD_UNPLUG = 0,
	HPD_PLUG,
	HPD_IRQ,
};

#if defined(CONFIG_EXTCON)
static const unsigned int extcon_id[] = {
	EXTCON_DISP_DP,

	EXTCON_NONE,
};
#endif

struct edid_data {
	int max_support_clk;
	bool support_10bpc;
	bool hdr_support;
	u8 eotf;
	u8 max_lumi_data;
	u8 max_average_lumi_data;
	u8 min_lumi_data;
};
enum dp_state {
	DP_DISCONNECT,
	DP_CONNECT,
	DP_HDCP_READY,
};
enum dex_state {
	DEX_OFF,
	DEX_ON,
	DEX_RECONNECTING,
};
enum dex_support_type {
	DEX_NOT_SUPPORT = 0,
	DEX_FHD_SUPPORT,
	DEX_WQHD_SUPPORT,
	DEX_UHD_SUPPORT
};

#define MON_NAME_LEN	14	/* monitor name */

#ifdef FEATURE_MANAGE_HMD_LIST
#define MAX_NUM_HMD	32
#define DEX_TAG_HMD	"HMD"

enum dex_hmd_type {
	DEX_HMD_MON = 0,	/* monitor name field */
	DEX_HMD_VID,		/* vid field */
	DEX_HMD_PID,		/* pid field */
	DEX_HMD_FIELD_MAX,
};

struct secdp_sink_dev {
	u32 ven_id;		/* vendor id from PDIC */
	u32 prod_id;		/* product id from PDIC */
	char monitor_name[MON_NAME_LEN];	/* max 14 bytes, from EDID */
};
#endif

struct displayport_device {
	enum displayport_state state;
	struct device *dev;
	struct displayport_resources res;

	unsigned int data_lane;
	u32 data_lane_cnt;
	struct phy *phy;
	spinlock_t slock;

	struct dsim_lcd_driver *panel_ops;
	struct decon_lcd lcd_info;

	struct v4l2_subdev sd;
	struct v4l2_dv_timings cur_timings;

	struct workqueue_struct *dp_wq;
	struct workqueue_struct *hdcp2_wq;
	struct delayed_work hpd_plug_work;
	struct delayed_work hpd_unplug_work;
	struct delayed_work hpd_irq_work;
#if defined(CONFIG_EXTCON)
	struct extcon_dev *extcon_displayport;
	//struct extcon_dev audio_switch;
#endif
	struct delayed_work hdcp13_work;
	struct delayed_work hdcp22_work;
	struct delayed_work hdcp13_integrity_check_work;
	int hdcp_ver;

	struct mutex cmd_lock;
	struct mutex hpd_lock;
	struct mutex aux_lock;
	struct mutex training_lock;
	struct mutex hdcp2_lock;
	spinlock_t spinlock_sfr;

	wait_queue_head_t dp_wait;
	int audio_state;
	int audio_buf_empty_check;
	wait_queue_head_t audio_wait;
#if defined(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
	struct delayed_work notifier_register_work;
	struct notifier_block dp_typec_nb;
	ccic_notifier_dp_pinconf_t ccic_notify_dp_conf;
	int notifier_registered;
	bool dp_not_support;
	bool ccic_link_conf;
	bool ccic_hpd;
#endif
	int hpd_current_state;
	enum hotplug_state hpd_state;
	int dp_sw_sel;
	int gpio_sw_oe;
	int gpio_sw_sel;
	int gpio_usb_dir;
	int dfp_type;
	const char *aux_vdd;
	int phy_tune_set;

	int auto_test_mode;
	int forced_bist;
	int hdr_test;
	enum bit_depth bpc;
	u8 bist_used;
	enum test_pattern bist_type;
	enum displayport_dynamic_range_type dyn_range;
	videoformat cur_video;
	uint64_t ven_id;
	uint64_t prod_id;
	char mon_name[MON_NAME_LEN];

#ifdef FEATURE_MANAGE_HMD_LIST
	struct secdp_sink_dev hmd_list[MAX_NUM_HMD];  /*list of supported HMD device*/
	struct mutex hmd_lock;
	bool is_hmd_dev;
#endif

	enum drm_state drm_start_state;
	enum drm_state drm_smc_state;

	struct edid_data rx_edid_data;

	int idle_ip_index;
	int do_unit_test;

	int dex_setting;
	enum dex_state dex_state;
	u8 dex_ver[2];
	enum dex_support_type dex_adapter_type;
	videoformat dex_video_pick;
#ifdef FEATURE_DEX_ADAPTER_TWEAK
	bool dex_skip_adapter_check;
#endif

	u8 edid_manufacturer[4];
	u32 edid_product;
	u32 edid_serial;
	u8 *edid_test_buf;

	videoformat best_video;
};

struct displayport_debug_param {
	u8 param_used;
	u8 link_rate;
	u8 lane_cnt;
};

/* EDID functions */
/* default preset configured on probe */
#define EDID_DEFAULT_TIMINGS_IDX (0) /* 640x480@60Hz */

#define EDID_ADDRESS 0x50
#define AUX_DATA_BUF_COUNT 16
#define EDID_BUF_COUNT 256
#define AUX_RETRY_COUNT 3
#define AUX_TIMEOUT_1800us 0x03

#define EDID_BLOCK_SIZE 128
#define DATA_BLOCK_TAG_CODE_MASK 0xE0
#define DATA_BLOCK_LENGTH_MASK 0x1F
#define DATA_BLOCK_TAG_CODE_BIT_POSITION 5

#define VSDB_TAG_CODE 3
#define HDMI14_IEEE_OUI_0 0x03
#define HDMI14_IEEE_OUI_1 0x0C
#define HDMI14_IEEE_OUI_2 0x00
#define IEEE_OUI_0_BYTE_NUM 1
#define IEEE_OUI_1_BYTE_NUM 2
#define IEEE_OUI_2_BYTE_NUM 3
#define VSDB_LATENCY_FILEDS_PRESETNT_MASK 0x80
#define VSDB_I_LATENCY_FILEDS_PRESETNT_MASK 0x40
#define VSDB_HDMI_VIDEO_PRESETNT_MASK 0x20
#define VSDB_VIC_FIELD_OFFSET 14
#define VSDB_VIC_LENGTH_MASK 0xE0
#define VSDB_VIC_LENGTH_BIT_POSITION 5

#define HDMI20_IEEE_OUI_0 0xD8
#define HDMI20_IEEE_OUI_1 0x5D
#define HDMI20_IEEE_OUI_2 0xC4
#define MAX_TMDS_RATE_BYTE_NUM 5
#define DC_SUPPORT_BYTE_NUM 7
#define DC_30BIT (0x01 << 0)

#define USE_EXTENDED_TAG_CODE 7
#define EXTENDED_HDR_TAG_CODE 6
#define EXTENDED_TAG_CODE_BYTE_NUM 1
#define SUPPORTED_EOTF_BYTE_NUM 2
#define SDR_LUMI (0x01 << 0)
#define HDR_LUMI (0x01 << 1)
#define SMPTE_ST_2084 (0x01 << 2)
#define MAX_LUMI_BYTE_NUM 4
#define MAX_AVERAGE_LUMI_BYTE_NUM 5
#define MIN_LUMI_BYTE_NUM 6

#define DETAILED_TIMING_DESCRIPTION_SIZE 18
#define AUDIO_DATA_BLOCK 1
#define VIDEO_DATA_BLOCK 2
#define SPEAKER_DATA_BLOCK 4
#define SVD_VIC_MASK 0x7F

enum video_ratio_t {
	RATIO_16_9 = 0,
	RATIO_16_10,
	RATIO_21_9,
	RATIO_4_3,
	RATIO_ETC = 99,
};

struct displayport_supported_preset {
	videoformat video_format;
	struct v4l2_dv_timings dv_timings;
	u32 fps;
	u32 v_sync_pol;
	u32 h_sync_pol;
	u8 vic;
	enum video_ratio_t ratio;
	char *name;
	enum dex_support_type dex_support;
	bool pro_audio_support;
	u8 displayid_timing;
	bool edid_support_match;
};

#define V4L2_DV_BT_CVT_3840X2160P59_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3840, 2160, 0, V4L2_DV_HSYNC_POS_POL, \
		533250000, 48, 32, 80, 3, 5, 54, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, \
		V4L2_DV_FL_REDUCED_BLANKING) \
}

#define V4L2_DV_BT_CVT_2560X1440P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2560, 1440, 0, V4L2_DV_HSYNC_POS_POL, \
		312250000, 192, 272, 464, 3, 5, 45, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_2560X1440P59_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2560, 1440, 0, V4L2_DV_HSYNC_POS_POL, \
		241500000, 48, 32, 80, 3, 5, 33, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_2560X1600P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2560, 1600, 0, \
		V4L2_DV_HSYNC_POS_POL | V4L2_DV_VSYNC_POS_POL, \
		268500000, 48, 32, 80, 3, 9, 37, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_2048X1536P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2048, 1536, 0, V4L2_DV_HSYNC_POS_POL, \
		209250000, 48, 32, 80, 3, 4, 37, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_1440X2560P75_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(1440, 2560, 0, V4L2_DV_HSYNC_POS_POL, \
		307000000, 10, 8, 120, 6, 2, 26, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_1440X2560P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(1440, 2560, 0, V4L2_DV_HSYNC_POS_POL, \
		246510000, 32, 10, 108, 6, 2, 16, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_1600X900P59_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(1600, 900, 0, \
		V4L2_DV_HSYNC_POS_POL | V4L2_DV_VSYNC_POS_POL, \
		97750000, 48, 32, 80, 3, 5, 18, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT, V4L2_DV_FL_REDUCED_BLANKING) \
}

#define V4L2_DV_BT_CVT_1920X1080P59_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(1920, 1080, 0, V4L2_DV_HSYNC_POS_POL, \
		138500000, 48, 44, 68, 3, 5, 23, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_3440X1440P50_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3440, 1440, 0, V4L2_DV_HSYNC_POS_POL, \
		265250000, 48, 32, 80, 3, 10, 21, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_3440X1440P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3440, 1440, 0, V4L2_DV_HSYNC_POS_POL, \
		319750000, 48, 32, 80, 3, 10, 28, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}
/*
#define V4L2_DV_BT_CVT_3440X1440P100_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3440, 1440, 0, V4L2_DV_HSYNC_POS_POL, \
		543500000, 48, 32, 80, 3, 10, 57, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}
*/
#define V4L2_DV_BT_CEA_3840X1080P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3840, 1080, 0, V4L2_DV_HSYNC_POS_POL, \
		266500000, 48, 32, 80, 3, 10, 18, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CEA_3840X1200P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3840, 1200, 0, V4L2_DV_HSYNC_POS_POL, \
		296250000, 48, 32, 80, 3, 10, 22, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_2560x1080P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2560, 1080, 0, \
		V4L2_DV_HSYNC_POS_POL | V4L2_DV_VSYNC_POS_POL, \
		198000000, 248, 44, 148, 4, 5, 11, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define V4L2_DV_BT_CVT_640x10P60_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(640, 10, 0, V4L2_DV_HSYNC_POS_POL, \
		27000000, 16, 96, 48, 2, 2, 12, 0, 0, 0, \
		V4L2_DV_BT_STD_DMT | V4L2_DV_BT_STD_CVT, 0) \
}

#define DISPLAYID_720P_EXT { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(1280, 720, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

#define DISPLAYID_1080P_EXT { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(1920, 1080, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

#define DISPLAYID_1440P_EXT { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2560, 1440, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

#define DISPLAYID_2160P_EXT { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3840, 2160, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

#define DISPLAYID_2400X1200P90_RELUMINO { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2400, 1200, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

/* Harman VR */
#define DISPLAYID_3200X1600P70_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3200, 1600, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

/* Pico X1 VR */
#define DISPLAYID_3200X1600P72_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(3200, 1600, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

/* Pico VR */
#define DISPLAYID_2160X3840P72_ADDED { \
	.type = V4L2_DV_BT_656_1120, \
	V4L2_INIT_BT_TIMINGS(2160, 3840, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) \
}

extern const int supported_videos_pre_cnt;
extern struct displayport_supported_preset supported_videos[];

struct exynos_displayport_data {
	enum {
		EXYNOS_DISPLAYPORT_STATE_PRESET = 0,
		EXYNOS_DISPLAYPORT_STATE_ENUM_PRESET,
		EXYNOS_DISPLAYPORT_STATE_RECONNECTION,
		EXYNOS_DISPLAYPORT_STATE_HDR_INFO,
		EXYNOS_DISPLAYPORT_STATE_AUDIO,
	} state;
	struct	v4l2_dv_timings timings;
	struct	v4l2_enum_dv_timings etimings;
	__u32	audio_info;
	int hdr_support;
};

struct displayport_audio_config_data {
	u32 audio_enable;
	u32 audio_channel_cnt;
	enum audio_sampling_frequency audio_fs;
	enum audio_bit_per_channel audio_bit;
	enum audio_16bit_dma_mode audio_packed_mode;
	enum audio_dma_word_length audio_word_length;
};

/* InfoFrame */
#define	INFOFRAME_PACKET_TYPE_AVI 0x82		/** Auxiliary Video information InfoFrame */
#define INFOFRAME_PACKET_TYPE_AUDIO 0x84	/** Audio information InfoFrame */
#define INFOFRAME_PACKET_TYPE_HDR 0x87		/** HDR Metadata InfoFrame */
#define MAX_INFOFRAME_LENGTH 27
#define INFOFRAME_REGISTER_SIZE 32
#define INFOFRAME_DATA_SIZE 8
#define DATA_NUM_PER_REG (INFOFRAME_REGISTER_SIZE / INFOFRAME_DATA_SIZE)

#define AVI_INFOFRAME_VERSION 0x02
#define AVI_INFOFRAME_LENGTH 0x0D
#define ACTIVE_FORMAT_INFOMATION_PRESENT (1 << 4)	/* No Active Format Infomation */
#define ACITVE_PORTION_ASPECT_RATIO (0x8 << 0)		/* Same as Picture Aspect Ratio */

#define AUDIO_INFOFRAME_VERSION 0x01
#define AUDIO_INFOFRAME_LENGTH 0x0A
#define AUDIO_INFOFRAME_PCM (1 << 4)
#define AUDIO_INFOFRAME_SF_BIT_POSITION 2

#define HDR_INFOFRAME_VERSION 0x01
#define HDR_INFOFRAME_LENGTH 26
#define HDR_INFOFRAME_EOTF_BYTE_NUM 0
#define HDR_INFOFRAME_SMPTE_ST_2084 2
#define STATIC_MATADATA_TYPE_1 0
#define HDR_INFOFRAME_METADATA_ID_BYTE_NUM 1
#define HDR_INFOFRAME_DISP_PRI_X_0_LSB 2
#define HDR_INFOFRAME_DISP_PRI_X_0_MSB 3
#define HDR_INFOFRAME_DISP_PRI_Y_0_LSB 4
#define HDR_INFOFRAME_DISP_PRI_Y_0_MSB 5
#define HDR_INFOFRAME_DISP_PRI_X_1_LSB 6
#define HDR_INFOFRAME_DISP_PRI_X_1_MSB 7
#define HDR_INFOFRAME_DISP_PRI_Y_1_LSB 8
#define HDR_INFOFRAME_DISP_PRI_Y_1_MSB 9
#define HDR_INFOFRAME_DISP_PRI_X_2_LSB 10
#define HDR_INFOFRAME_DISP_PRI_X_2_MSB 11
#define HDR_INFOFRAME_DISP_PRI_Y_2_LSB 12
#define HDR_INFOFRAME_DISP_PRI_Y_2_MSB 13
#define HDR_INFOFRAME_WHITE_POINT_X_LSB 14
#define HDR_INFOFRAME_WHITE_POINT_X_MSB 15
#define HDR_INFOFRAME_WHITE_POINT_Y_LSB 16
#define HDR_INFOFRAME_WHITE_POINT_Y_MSB 17
#define HDR_INFOFRAME_MAX_LUMI_LSB 18
#define HDR_INFOFRAME_MAX_LUMI_MSB 19
#define HDR_INFOFRAME_MIN_LUMI_LSB 20
#define HDR_INFOFRAME_MIN_LUMI_MSB 21
#define HDR_INFOFRAME_MAX_LIGHT_LEVEL_LSB 22
#define HDR_INFOFRAME_MAX_LIGHT_LEVEL_MSB 23
#define HDR_INFOFRAME_MAX_AVERAGE_LEVEL_LSB 24
#define HDR_INFOFRAME_MAX_AVERAGE_LEVEL_MSB 25
#define LSB_MASK 0x00FF
#define MSB_MASK 0xFF00
#define SHIFT_8BIT 8

struct infoframe {
	u8 type_code;
	u8 version_number;
	u8 length;
	u8 data[MAX_INFOFRAME_LENGTH];
};

typedef struct{
	u8 HDCP13_BKSV[5];
	u8 HDCP13_R0[2];
	u8 HDCP13_AKSV[5];
	u8 HDCP13_AN[8];
	u8 HDCP13_V_H[20];
	u8 HDCP13_BCAP[1];
	u8 HDCP13_BSTATUS[1];
	u8 HDCP13_BINFO[2];
	u8 HDCP13_KSV_FIFO[15];
	u8 HDCP13_AINFO[1];
} HDCP13;

enum HDCP13_STATE {
	HDCP13_STATE_NOT_AUTHENTICATED,
	HDCP13_STATE_AUTH_PROCESS,
	HDCP13_STATE_SECOND_AUTH_DONE,
	HDCP13_STATE_AUTHENTICATED,
	HDCP13_STATE_FAIL
};

struct hdcp13_info {
	u8 cp_irq_flag;
	u8 is_repeater;
	u8 device_cnt;
	u8 revocation_check;
	u8 r0_read_flag;
	int link_check;
	enum HDCP13_STATE auth_state;
};

/* HDCP 1.3 */
extern HDCP13 HDCP13_DPCD;
extern struct hdcp13_info hdcp13_info;

#define ADDR_HDCP13_BKSV 0x68000
#define ADDR_HDCP13_R0 0x68005
#define ADDR_HDCP13_AKSV 0x68007
#define ADDR_HDCP13_AN 0x6800C
#define ADDR_HDCP13_V_H0 0x68014
#define ADDR_HDCP13_V_H1 0x68018
#define ADDR_HDCP13_V_H2 0x6801C
#define ADDR_HDCP13_V_H3 0x68020
#define ADDR_HDCP13_V_H4 0x68024
#define ADDR_HDCP13_BCAP 0x68028
#define ADDR_HDCP13_BSTATUS 0x68029
#define ADDR_HDCP13_BINFO 0x6802A
#define ADDR_HDCP13_KSV_FIFO 0x6802C
#define ADDR_HDCP13_AINFO 0x6803B
#define ADDR_HDCP13_RSVD 0x6803C
#define ADDR_HDCP13_DBG 0x680C0

#define BCAPS_RESERVED_BIT_MASK 0xFC
#define BCAPS_REPEATER (1 << 1)
#define BCAPS_HDCP_CAPABLE (1 << 0)

#define BSTATUS_READY (1<<0)
#define BSTATUS_R0_AVAILABLE (1<<1)
#define BSTATUS_LINK_INTEGRITY_FAIL (1<<2)
#define BSTATUS_REAUTH_REQ (1<<3)

#define BINFO_DEVICE_COUNT (0x7F<<0)
#define BINFO_MAX_DEVS_EXCEEDED (1<<7)
#define BINFO_DEPTH (7<<8)
#define BINFO_MAX_CASCADE_EXCEEDED (1<<11)

#define RI_READ_RETRY_CNT 3
#define RI_AVAILABLE_WAITING 2
#define RI_DELAY 100
#define RI_WAIT_COUNT (RI_DELAY / RI_AVAILABLE_WAITING)
#define REPEATER_READY_MAX_WAIT_DELAY 5000
#define REPEATER_READY_WAIT_COUNT (REPEATER_READY_MAX_WAIT_DELAY / RI_AVAILABLE_WAITING)
#define HDCP_RETRY_COUNT 1
#define KSV_SIZE 5
#define KSV_FIFO_SIZE 15
#define MAX_KSV_LIST_COUNT 127
#define M0_SIZE 8
#define SHA1_SIZE 20
#define BINFO_SIZE 2
#define V_READ_RETRY_CNT 3

#define USBDP_PHY_CONTROL 0x15860704

enum{
	LINK_CHECK_PASS = 0,
	LINK_CHECK_NEED = 1,
	LINK_CHECK_FAIL = 2,
};

enum{
	FIRST_AUTH  = 0,
	SECOND_AUTH = 1,
};

static inline struct displayport_device *get_displayport_drvdata(void)
{
	return displayport_drvdata;
}

/* register access subroutines */
static inline u32 displayport_read(u32 reg_id)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	return readl(displayport->res.link_regs + reg_id);
}

static inline u32 displayport_read_mask(u32 reg_id, u32 mask)
{
	u32 val = displayport_read(reg_id);

	val &= (mask);
	return val;
}

static inline void displayport_write(u32 reg_id, u32 val)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	writel(val, displayport->res.link_regs + reg_id);
}

static inline void displayport_write_mask(u32 reg_id, u32 val, u32 mask)
{
	struct displayport_device *displayport = get_displayport_drvdata();
	u32 old;
	u32 bit_shift;
	unsigned long flags;

	spin_lock_irqsave(&displayport->spinlock_sfr, flags);
	old = displayport_read(reg_id);
	for (bit_shift = 0; bit_shift < 32; bit_shift++) {
		if ((mask >> bit_shift) & 0x00000001)
			break;
	}

	val = ((val<<bit_shift) & mask) | (old & ~mask);
	writel(val, displayport->res.link_regs + reg_id);
	spin_unlock_irqrestore(&displayport->spinlock_sfr, flags);
}

static inline int displayport_phy_enabled(void)
{
	/* USBDP_PHY_CONTROL register */
	struct displayport_device *displayport = get_displayport_drvdata();
	int en = 0;

	if (displayport->res.usbdp_regs) {
		en = readl(displayport->res.usbdp_regs) & 0x1;

		if (!en)
			displayport_info("combo phy disabled\n");
	}

	return en;
}

static inline u32 displayport_phy_read(u32 reg_id)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	if (!displayport_phy_enabled())
		return 0;

	return readl(displayport->res.phy_regs + reg_id);
}

static inline u32 displayport_phy_read_mask(u32 reg_id, u32 mask)
{
	u32 val;

	if (!displayport_phy_enabled())
		return 0;

	val = displayport_phy_read(reg_id);

	val &= (mask);

	return val;
}

static inline void displayport_phy_write(u32 reg_id, u32 val)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	if (!displayport_phy_enabled())
		return;

	writel(val, displayport->res.phy_regs + reg_id);
}

static inline void displayport_phy_write_mask(u32 reg_id, u32 val, u32 mask)
{
	struct displayport_device *displayport = get_displayport_drvdata();
	u32 bit_shift;
	u32 old;

	if (!displayport_phy_enabled())
		return;

	old = displayport_phy_read(reg_id);

	for (bit_shift = 0; bit_shift < 32; bit_shift++) {
		if ((mask >> bit_shift) & 0x00000001)
			break;
	}

	val = ((val<<bit_shift) & mask) | (old & ~mask);
	writel(val, displayport->res.phy_regs + reg_id);
}

static inline bool IS_DISPLAYPORT_HPD_PLUG_STATE(void)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	return (bool)displayport->hpd_current_state;
}

static inline bool IS_DISPLAYPORT_SWITCH_STATE(void)
{
	struct displayport_device *displayport = get_displayport_drvdata();

	if (extcon_get_state(displayport->extcon_displayport, EXTCON_DISP_DP) == true)
		return true;
	else
		return false;
}

int displayport_enable(struct displayport_device *displayport);
int displayport_disable(struct displayport_device *displayport);

void displayport_reg_init(void);
void displayport_reg_deinit(void);
void displayport_reg_sw_reset(void);
void displayport_reg_set_interrupt_mask(enum displayport_interrupt_mask param, u8 set);
u32 displayport_reg_get_interrupt_and_clear(u32 interrupt_status_register);
void displayport_reg_start(void);
void displayport_reg_video_mute(u32 en);
void displayport_reg_stop(void);
void displayport_reg_set_video_configuration(videoformat video_format, u8 bpc, u8 range);
int displayport_reg_dpcd_write(u32 address, u32 length, u8 *data);
int displayport_reg_dpcd_read(u32 address, u32 length, u8 *data);
int displayport_reg_dpcd_write_burst(u32 address, u32 length, u8 *data);
int displayport_reg_dpcd_read_burst(u32 address, u32 length, u8 *data);
int displayport_reg_edid_write(u8 edid_addr_offset, u32 length, u8 *data);
int displayport_reg_edid_read(u8 edid_addr_offset, u32 length, u8 *data);
int displayport_reg_i2c_read(u32 address, u32 length, u8 *data);
int displayport_reg_i2c_write(u32 address, u32 length, u8 *data);
void displayport_reg_phy_reset(u32 en);
void displayport_reg_phy_disable(void);
void displayport_reg_phy_init_setting(void);
void displayport_reg_phy_mode_setting(void);
void displayport_reg_phy_ssc_enable(u32 en);
void displayport_reg_phy_set_link_bw(u8 link_rate);
u32 displayport_reg_phy_get_link_bw(void);
void displayport_reg_set_lane_count(u8 lane_cnt);
u32 displayport_reg_get_lane_count(void);
void displayport_reg_wait_phy_pll_lock(void);
void displayport_reg_set_training_pattern(displayport_training_pattern pattern);
void displayport_reg_set_qual_pattern(displayport_qual_pattern pattern, displayport_scrambling scramble);
void displayport_reg_set_hbr2_scrambler_reset(u32 uResetCount);
void displayport_reg_set_pattern_PLTPAT(void);
void displayport_reg_set_voltage_and_pre_emphasis(u8 *voltage, u8 *pre_emphasis);
void displayport_reg_set_bist_video_configuration(videoformat video_format, u8 bpc, u8 type, u8 range);
void displayport_reg_set_bist_video_configuration_for_blue_screen(videoformat video_format);
void displayport_reg_set_video_bist_mode(u32 en);
void displayport_reg_set_audio_bist_mode(u32 en);
void displayport_reg_lh_p_ch_power(u32 en);

void displayport_audio_enable(struct displayport_audio_config_data *audio_config_data);
void displayport_audio_disable(void);
void displayport_audio_wait_buf_full(void);
void displayport_audio_dma_force_req_release(void);
void displayport_audio_bist_enable(struct displayport_audio_config_data audio_config_data);
void displayport_audio_init_config(void);
void displayport_audio_bist_config(struct displayport_audio_config_data audio_config_data);
void displayport_reg_print_audio_state(void);

void displayport_reg_set_hdcp22_system_enable(u32 en);
void displayport_reg_set_hdcp22_mode(u32 en);
void displayport_reg_set_hdcp22_encryption_enable(u32 en);
u32 displayport_reg_get_hdcp22_encryption_enable(void);
void displayport_reg_set_aux_pn_inv(u32 val);
void displayport_hpd_changed(int state);
int displayport_get_hpd_state(void);
bool is_displayport_not_running(void);

int displayport_reg_stand_alone_crc_sorting(void);

int edid_read(struct displayport_device *hdev, u8 **data);
int edid_update(struct displayport_device *hdev);
struct v4l2_dv_timings edid_preferred_preset(void);
void edid_set_preferred_preset(int mode);
int edid_find_resolution(u16 xres, u16 yres, u16 refresh);
u8 edid_read_checksum(void);
u32 edid_audio_informs(void);
bool edid_support_pro_audio(void);
struct fb_audio *edid_get_test_audio_info(void);

void displayport_reg_set_avi_infoframe(struct infoframe avi_infofrmae);
#ifdef FEATURE_SUPPORT_SPD_INFOFRAME
void displayport_reg_set_spd_infoframe(struct infoframe spd_infofrmae);
#endif
void displayport_reg_set_audio_infoframe(struct infoframe audio_infofrmae, u32 en);
void displayport_reg_set_hdr_infoframe(struct infoframe hdr_infofrmae, u32 en);

void hdcp13_run(void);
void hdcp13_dpcd_buffer(void);
u8 hdcp13_read_bcap(void);
void hdcp13_link_integrity_check(void);

extern int hdcp_calc_sha1(u8 *digest, const u8 *buf, unsigned int buflen);

#define DISPLAYPORT_IOC_DUMP			_IOW('V', 0, u32)
#define DISPLAYPORT_IOC_GET_ENUM_DV_TIMINGS	_IOW('V', 1, u8)
#define DISPLAYPORT_IOC_SET_RECONNECTION	_IOW('V', 2, u8)
#define DISPLAYPORT_IOC_DP_SA_SORTING		_IOW('V', 3, int)
#define DISPLAYPORT_IOC_SET_HDR_METADATA	_IOW('V', 4, struct exynos_hdr_static_info *)
#define DISPLAYPORT_IOC_GET_HDR_INFO	_IOW('V', 5, int)
#endif
