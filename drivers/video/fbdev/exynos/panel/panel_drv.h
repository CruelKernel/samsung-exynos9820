/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Author: Minwoo Kim <minwoo7945.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_DRV_H__
#define __PANEL_DRV_H__

#include <linux/device.h>
#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <media/v4l2-subdev.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>

#if defined(CONFIG_EXYNOS_DPU20)
#include "../dpu20/disp_err.h"
#include "../dpu20/dsim.h"
#include "../dpu20/decon.h"
#include "../dpu20/panels/decon_lcd.h"
#else
#include "../dpu_9810/disp_err.h"
#include "../dpu_9810/dsim.h"
#include "../dpu_9810/decon.h"
#include "../dpu_9810/panels/decon_lcd.h"
#endif

#include "panel.h"
#include "mdnie.h"
#include "copr.h"
#ifdef CONFIG_SUPPORT_DDI_FLASH
#include "panel_poc.h"
#endif
#ifdef CONFIG_ACTIVE_CLOCK
#include "active_clock.h"
#endif
#ifdef CONFIG_EXTEND_LIVE_CLOCK
#include "./aod/aod_drv.h"
#endif

#ifdef CONFIG_SUPPORT_DISPLAY_PROFILER
#include "./display_profiler/display_profiler.h"
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
#include "panel_spi.h"
#endif
#if defined(CONFIG_TDMB_NOTIFIER)
#include <linux/tdmb_notifier.h>
#endif

extern int panel_log_level;

#define CONFIG_DISP_PMIC_SSD

void clear_pending_bit(int irq);

#define panel_err(fmt, ...)							\
	do {									\
		if (panel_log_level >= 3) {					\
			pr_err(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)

#define panel_warn(fmt, ...)							\
	do {									\
		if (panel_log_level >= 4) {					\
			pr_warn(pr_fmt(fmt), ##__VA_ARGS__);			\
		}								\
	} while (0)
#define panel_info(fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)
#define panel_dbg(fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(fmt), ##__VA_ARGS__);			\
	} while (0)

enum {
	PANEL_REGULATOR_TYPE_PWR = 0,
	PANEL_REGULATOR_TYPE_SSD = 1,
	PANEL_REGULATOR_TYPE_MAX,
};

enum {
	PANEL_REGULATOR_DDI_3P0 = 0,
	PANEL_REGULATOR_DDI_1P8,
	PANEL_REGULATOR_DDR_1P6,
	PANEL_REGULATOR_SSD,
	PANEL_REGULATOR_MAX
};

enum panel_gpio_lists {
	PANEL_GPIO_RESET = 0,
	PANEL_GPIO_DISP_DET,
	PANEL_GPIO_PCD,
	PANEL_GPIO_ERR_FG,
	PANEL_GPIO_CONN_DET,
	PANEL_GPIO_MAX,
};

#define PANEL_GPIO_NAME_RESET ("disp-reset")
#define PANEL_GPIO_NAME_DISP_DET ("disp-det")
#define PANEL_GPIO_NAME_PCD ("pcd")
#define PANEL_GPIO_NAME_ERR_FG ("err-fg")
#define PANEL_GPIO_NAME_CONN_DET ("conn-det")

#define PANEL_REGULATOR_NAME_DDI_3P0 ("ddi-3p0")
#define PANEL_REGULATOR_NAME_DDI_1P8 ("ddi-1p8")
#define PANEL_REGULATOR_NAME_DDR_1P6 ("ddr-1p6")
#define PANEL_REGULATOR_NAME_SSD ("short-detect")

struct panel_gpio {
	const char *name;
	int num;
	bool active_low;
	int dir;		/* 0:out, 1:in */
	int irq;
	unsigned int irq_type;
	void __iomem *irq_pend_reg;
	int irq_pend_bit;
};

struct panel_regulator {
	const char *name;
	struct regulator *reg;
	int type;
	int def_voltage;
	int lpm_voltage;
	int def_current;
	int lpm_current;
};

#define DSIM_OPTION_WAIT_TX_DONE	(1U << 0)
#define DSIM_OPTION_POINT_GPARA		(1U << 1)

struct mipi_drv_ops {
	int (*read)(u32 id, u8 addr, u8 ofs, u8 *buf, int size, u32 option);
	int (*write)(u32 id, u8 cmd_id, const u8 *cmd, u8 ofs, int size, u32 option, bool wakeup);
	enum dsim_state(*get_state)(u32 id);
	void (*parse_dt)(struct device_node *node, struct decon_lcd *lcd_info);
};

#define PANEL_INIT_KERNEL		0
#define PANEL_INIT_BOOT			1

#define PANEL_DISP_DET_HIGH		1
#define PANEL_DISP_DET_LOW		0

enum {
	PANEL_STATE_NOK = 0,
	PANEL_STATE_OK = 1,
};

enum {
	PANEL_EL_OFF = 0,
	PANEL_EL_ON = 1,
};

enum {
	PANEL_UB_CONNECTED = 0,
	PANEL_UB_DISCONNECTED = 1,
};

enum {
	PANEL_DISCONNECT = 0,
	PANEL_CONNECT = 1,
};

#define ALPM_MODE	0
#define HLPM_MODE	1

enum panel_active_state {
	PANEL_STATE_OFF = 0,
	PANEL_STATE_ON,
	PANEL_STATE_NORMAL,
	PANEL_STATE_ALPM,
};

enum panel_power {
	PANEL_POWER_OFF = 0,
	PANEL_POWER_ON
};

enum {
	PANEL_DISPLAY_OFF = 0,
	PANEL_DISPLAY_ON,
};

enum {
	PANEL_HMD_OFF = 0,
	PANEL_HMD_ON,
};

enum {
	PANEL_WORK_DISP_DET = 0,
	PANEL_WORK_PCD,
	PANEL_WORK_ERR_FG,
	PANEL_WORK_CONN_DET,
	PANEL_WORK_DIM_FLASH,
	PANEL_WORK_CHECK_CONDITION,
	PANEL_WORK_MAX,
};

struct panel_state {
	int init_at;
	int connect_panel;
	int connected;
	int cur_state;
	int power;
	int disp_on;
#ifdef CONFIG_SUPPORT_HMD
	int hmd_on;
#endif
	int lpm_brightness;
};

struct copr_spi_gpios {
	int gpio_sck;
	int gpio_miso;
	int gpio_mosi;
	int gpio_cs;
};

struct host_cb {
	int (*cb)(void *data);
	void *data;
};

enum {
	NO_CHECK_STATE = 0,
	PRINT_NORMAL_PANEL_INFO,
	CHECK_NORMAL_PANEL_INFO,
	PRINT_DOZE_PANEL_INFO,
	STATE_MAX
};

#define STR_NO_CHECK			("no state")
#define STR_NOMARL_ON			("after normal disp on")
#define STR_NOMARL_100FRAME		("check normal in 100frames")
#define STR_AOD_ON				("after aod disp on")

struct panel_condition_check {
	bool is_panel_check;
	u32 frame_cnt;
	u8 check_state;
	char str_state[STATE_MAX][30];
};

enum GAMMA_FLASH_RESULT {
	GAMMA_FLASH_ERROR_MTP_OFFSET = -4,
	GAMMA_FLASH_ERROR_CHECKSUM_MISMATCH = -3,
	GAMMA_FLASH_ERROR_NOT_EXIST = -2,
	GAMMA_FLASH_ERROR_READ_FAIL = -1,
	GAMMA_FLASH_PROGRESS = 0,
	GAMMA_FLASH_SUCCESS = 1,
};

struct dim_flash_result {
	u8 mtp_reg[34];
	u8 mtp_flash[34];
	bool exist;

	u32 dim_chksum_ok;
	u32 dim_chksum_by_calc;
	u32 dim_chksum_by_read;

	u32 mtp_chksum_ok;
	u32 mtp_chksum_by_calc;
	u32 mtp_chksum_by_read;
};

typedef void (*panel_wq_handler)(struct work_struct *);

struct panel_work {
	struct mutex lock;
	struct workqueue_struct *wq;
	struct delayed_work dwork;
	atomic_t running;
	void *data;
	int ret;
};

#ifdef CONFIG_DYNAMIC_FREQ

#define MAX_DYNAMIC_FREQ	5
#define DF_CONTEXT_RIL		1

#endif


struct panel_device {
	int id;
	int dsi_id;

	struct device_node *ddi_node;

	struct spi_device *spi;
	struct copr_info copr;
	struct copr_spi_gpios spi_gpio;

	/* mutex lock for panel operations */
	struct mutex op_lock;
	/* mutex lock for panel's data */
	struct mutex data_lock;
	/* mutex lock for panel's ioctl */
	struct mutex io_lock;

	struct device *dev;
	struct mdnie_info mdnie;

	struct lcd_device *lcd;
	struct panel_bl_device panel_bl;

	unsigned char panel_id[3];

	struct v4l2_subdev sd;

	struct panel_gpio gpio[PANEL_GPIO_MAX];
	struct panel_regulator regulator[PANEL_REGULATOR_MAX];

	struct decon_lcd lcd_info;

	struct mipi_drv_ops mipi_drv;

	struct panel_state state;

	struct panel_work work[PANEL_WORK_MAX];

	struct notifier_block fb_notif;
#ifdef CONFIG_DISPLAY_USE_INFO
	struct notifier_block panel_dpui_notif;
#endif
	struct panel_info panel_data;

#ifdef CONFIG_ACTIVE_CLOCK
	struct act_clock_dev act_clk_dev;
#endif

#ifdef CONFIG_EXTEND_LIVE_CLOCK
	struct aod_dev_info aod;
#endif
#ifdef CONFIG_SUPPORT_DDI_FLASH
	struct panel_poc_device poc_dev;
#endif
#ifdef CONFIG_SUPPORT_POC_SPI
	struct panel_spi_dev panel_spi_dev;
#endif
#ifdef CONFIG_SUPPORT_TDMB_TUNE
	struct notifier_block tdmb_notif;
#endif
	struct disp_error_cb_info error_cb_info;

	ktime_t ktime_panel_on;
	ktime_t ktime_panel_off;

#ifdef CONFIG_SUPPORT_DIM_FLASH
	struct dim_flash_result dim_flash_result;
	struct panel_irc_info *irc_info;
#endif
	struct panel_condition_check condition_check;

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	struct adaptive_idx adap_idx;
#endif

#ifdef CONFIG_DYNAMIC_FREQ
	struct notifier_block df_noti;
	struct df_status_info df_status;
	struct df_freq_tbl_info *df_freq_tbl;
#endif
#ifdef CONFIG_SUPPORT_DISPLAY_PROFILER
	struct profiler_device profiler;
#endif
};

#ifdef CONFIG_SUPPORT_DIM_FLASH
int panel_update_dim_type(struct panel_device *panel, u32 dim_type);
#endif

static inline bool IS_PANEL_PWR_ON_STATE(struct panel_device *panel)
{
	return (panel->state.cur_state == PANEL_STATE_ON ||
			panel->state.cur_state == PANEL_STATE_NORMAL ||
			panel->state.cur_state == PANEL_STATE_ALPM);
}

static inline bool IS_PANEL_PWR_OFF_STATE(struct panel_device *panel)
{
	return panel->state.cur_state == PANEL_STATE_OFF;
}

int panel_display_on(struct panel_device *panel);
int __set_panel_power(struct panel_device *panel, int power);

#ifdef CONFIG_EXTEND_LIVE_CLOCK
static inline int panel_aod_init_panel(struct panel_device *panel)
{
	return (panel->aod.ops && panel->aod.ops->init_panel) ?
		panel->aod.ops->init_panel(&panel->aod) : 0;
}

static inline int panel_aod_enter_to_lpm(struct panel_device *panel)
{
	return (panel->aod.ops && panel->aod.ops->enter_to_lpm) ?
		panel->aod.ops->enter_to_lpm(&panel->aod) : 0;
}

static inline int panel_aod_exit_from_lpm(struct panel_device *panel)
{
	return (panel->aod.ops && panel->aod.ops->exit_from_lpm) ?
		panel->aod.ops->exit_from_lpm(&panel->aod) : 0;
}

static inline int panel_aod_doze_suspend(struct panel_device *panel)
{
	return (panel->aod.ops && panel->aod.ops->doze_suspend) ?
		panel->aod.ops->doze_suspend(&panel->aod) : 0;
}

static inline int panel_aod_power_off(struct panel_device *panel)
{
	return (panel->aod.ops && panel->aod.ops->power_off) ?
		panel->aod.ops->power_off(&panel->aod) : 0;
}

#ifdef SUPPORT_NORMAL_SELF_MOVE
static inline int panel_self_move_pattern_update(struct panel_device *panel)
{
	return (panel->aod.ops && panel->aod.ops->self_move_pattern_update) ?
		panel->aod.ops->self_move_pattern_update(&panel->aod) : 0;
}
#endif
#endif

bool ub_con_disconnected(struct panel_device *panel);
int panel_wake_lock(struct panel_device *panel);
void panel_wake_unlock(struct panel_device *panel);
bool panel_gpio_valid(struct panel_gpio *gpio);
void panel_send_ubconn_uevent(struct panel_device *panel);

#define PANEL_DRV_NAME "panel-drv"

#define PANEL_IOC_BASE	'P'

#define PANEL_IOC_DSIM_PROBE			_IOW(PANEL_IOC_BASE, 1, struct decon_lcd *)
#define PANEL_IOC_GET_PANEL_STATE		_IOW(PANEL_IOC_BASE, 3, struct panel_state *)
#define PANEL_IOC_DSIM_PUT_MIPI_OPS		_IOR(PANEL_IOC_BASE, 4, struct mipi_ops *)
#define PANEL_IOC_PANEL_PROBE			_IO(PANEL_IOC_BASE, 5)

#define PANEL_IOC_SET_POWER				_IO(PANEL_IOC_BASE, 6)

#define PANEL_IOC_SLEEP_IN				_IO(PANEL_IOC_BASE, 7)
#define PANEL_IOC_SLEEP_OUT				_IO(PANEL_IOC_BASE, 8)

#define PANEL_IOC_DISP_ON				_IO(PANEL_IOC_BASE, 9)

#define PANEL_IOC_PANEL_DUMP			_IO(PANEL_IOC_BASE, 10)

#define PANEL_IOC_EVT_FRAME_DONE		_IOW(PANEL_IOC_BASE, 11, struct timespec *)
#define PANEL_IOC_EVT_VSYNC				_IOW(PANEL_IOC_BASE, 12, struct timespec *)

#ifdef CONFIG_SUPPORT_DOZE
#define PANEL_IOC_DOZE					_IO(PANEL_IOC_BASE, 31)
#define PANEL_IOC_DOZE_SUSPEND			_IO(PANEL_IOC_BASE, 32)
#endif

#ifdef CONFIG_SUPPORT_DSU
#ifdef CONFIG_EXYNOS_MULTIRESOLUTION
#define PANEL_IOC_SET_DSU				_IOW(PANEL_IOC_BASE, 41, int *)
#else
#define PANEL_IOC_SET_DSU				_IOW(PANEL_IOC_BASE, 41, struct dsu_info *)
#endif
#endif
#define PANEL_IOC_REG_RESET_CB			_IOR(PANEL_IOC_BASE, 51, struct host_cb *)
#ifdef CONFIG_SUPPORT_INDISPLAY
#define PANEL_IOC_SET_FINGER_SET		_IO(PANEL_IOC_BASE, 61)
#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
#define PANEL_IOC_MIPI_FREQ_CHANGED		_IOR(PANEL_IOC_BASE, 71, int *)
#endif

#ifdef CONFIG_DYNAMIC_FREQ
#define MAGIC_DF_UPDATED				(0x0A55AA55)

#define PANEL_IOC_GET_DF_STATUS			_IOR(PANEL_IOC_BASE, 85, int *)
#define PANEL_IOC_DYN_FREQ_FFC			_IOR(PANEL_IOC_BASE, 82, int *)
#endif
#endif //__PANEL_DRV_H__
