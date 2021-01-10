/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC MIPI-DSIM driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/pm_runtime.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/module.h>
#include <video/mipi_display.h>
#if defined(CONFIG_CAL_IF)
#include <soc/samsung/cal-if.h>
#endif
#if defined(CONFIG_SOC_EXYNOS9810)
#include <dt-bindings/clock/exynos9810.h>
#elif defined(CONFIG_SOC_EXYNOS9820)
#include <dt-bindings/clock/exynos9820.h>
#endif
#if defined(CONFIG_CPU_IDLE)
#include <soc/samsung/exynos-cpupm.h>
#endif
#include <soc/samsung/exynos-pmu.h>
#if defined(CONFIG_SUPPORT_LEGACY_ION)
#include <linux/exynos_iovmm.h>
#endif

#include "decon.h"
#include "dsim.h"

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
#include <linux/dev_ril_bridge.h>
#include "adap_freq_tbl.h"
#endif

int dsim_log_level = 6;

struct dsim_device *dsim_drvdata[MAX_DSIM_CNT];
EXPORT_SYMBOL(dsim_drvdata);

static char *dsim_state_names[] = {
	"INIT",
	"ON",
	"DOZE",
	"ULPS",
	"DOZE_SUSPEND",
	"OFF",
};

static int dsim_runtime_suspend(struct device *dev);
static int dsim_runtime_resume(struct device *dev);

static void dsim_dump(struct dsim_device *dsim, int panel_dump)
{
	struct dsim_regs regs;

	dsim_info("=== DSIM SFR DUMP ===\n");

	dsim_to_regs_param(dsim, &regs);
	__dsim_dump(dsim->id, &regs);

	/* Show panel status */
	if (panel_dump)
		call_panel_ops(dsim, dump, dsim);
}

static void dsim_long_data_wr(struct dsim_device *dsim, unsigned long d0, u32 d1)
{
	unsigned int data_cnt = 0, payload = 0;

	/* in case that data count is more then 4 */
	for (data_cnt = 0; data_cnt < d1; data_cnt += 4) {
		/*
		 * after sending 4bytes per one time,
		 * send remainder data less then 4.
		 */
		if ((d1 - data_cnt) < 4) {
			if ((d1 - data_cnt) == 3) {
				payload = *(u8 *)(d0 + data_cnt) |
				    (*(u8 *)(d0 + (data_cnt + 1))) << 8 |
					(*(u8 *)(d0 + (data_cnt + 2))) << 16;
			dsim_dbg("count = 3 payload = %x, %x %x %x\n",
				payload, *(u8 *)(d0 + data_cnt),
				*(u8 *)(d0 + (data_cnt + 1)),
				*(u8 *)(d0 + (data_cnt + 2)));
			} else if ((d1 - data_cnt) == 2) {
				payload = *(u8 *)(d0 + data_cnt) |
					(*(u8 *)(d0 + (data_cnt + 1))) << 8;
			dsim_dbg("count = 2 payload = %x, %x %x\n", payload,
				*(u8 *)(d0 + data_cnt),
				*(u8 *)(d0 + (data_cnt + 1)));
			} else if ((d1 - data_cnt) == 1) {
				payload = *(u8 *)(d0 + data_cnt);
			}

			dsim_reg_wr_tx_payload(dsim->id, payload);
		/* send 4bytes per one time. */
		} else {
			payload = *(u8 *)(d0 + data_cnt) |
				(*(u8 *)(d0 + (data_cnt + 1))) << 8 |
				(*(u8 *)(d0 + (data_cnt + 2))) << 16 |
				(*(u8 *)(d0 + (data_cnt + 3))) << 24;

			dsim_dbg("count = 4 payload = %x, %x %x %x %x\n",
				payload, *(u8 *)(d0 + data_cnt),
				*(u8 *)(d0 + (data_cnt + 1)),
				*(u8 *)(d0 + (data_cnt + 2)),
				*(u8 *)(d0 + (data_cnt + 3)));

			dsim_reg_wr_tx_payload(dsim->id, payload);
		}
	}
}

static int dsim_wait_for_cmd_fifo_empty(struct dsim_device *dsim, bool must_wait)
{
	int ret = 0;
	struct dsim_regs regs;

	if (!must_wait) {
		/* timer is running, but already command is transferred */
		if (dsim_reg_header_fifo_is_empty(dsim->id))
			del_timer(&dsim->cmd_timer);

		dsim_dbg("%s Doesn't need to wait fifo_completion\n", __func__);
		return ret;
	} else {
		del_timer(&dsim->cmd_timer);
		dsim_dbg("%s Waiting for fifo_completion...\n", __func__);
	}

	if (!wait_for_completion_timeout(&dsim->ph_wr_comp, MIPI_WR_TIMEOUT)) {
		if (dsim_reg_header_fifo_is_empty(dsim->id)) {
			reinit_completion(&dsim->ph_wr_comp);
			dsim_reg_clear_int(dsim->id, DSIM_INTSRC_SFR_PH_FIFO_EMPTY);
			return 0;
		}
		ret = -ETIMEDOUT;
	}

	if (IS_DSIM_ON_STATE(dsim) && (ret == -ETIMEDOUT)) {
		dsim_err("%s timeout\n", __func__);
		dsim_to_regs_param(dsim, &regs);
		__dsim_dump(dsim->id, &regs);
	}
	return ret;
}

/* wait for until SFR fifo is empty */
int dsim_wait_for_cmd_done(struct dsim_device *dsim)
{
	int ret = 0;
	/* FIXME: hiber only support for DECON0 */
	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block_exit(decon);

	mutex_lock(&dsim->cmd_lock);
	ret = dsim_wait_for_cmd_fifo_empty(dsim, true);
	mutex_unlock(&dsim->cmd_lock);

	decon_hiber_unblock(decon);

	return ret;
}

static bool dsim_fifo_empty_needed(struct dsim_device *dsim, unsigned int data_id,
	unsigned long data0)
{
	/* read case or partial update command */
	if (data_id == MIPI_DSI_DCS_READ
			|| data0 == MIPI_DCS_SET_COLUMN_ADDRESS
			|| data0 == MIPI_DCS_SET_PAGE_ADDRESS) {
		dsim_dbg("%s: id:%d, data=%ld\n", __func__, data_id, data0);
		return true;
	}

	/* Check a FIFO level whether writable or not */
	if (!dsim_reg_is_writable_fifo_state(dsim->id))
		return true;

	return false;
}

/*  wakeup : true  : wakeup from hibernation..
wakeup : false : return fail when dsim is inactive (hibernation) */

int dsim_write_data(struct dsim_device *dsim, u32 id, unsigned long d0, u32 d1, bool must_wait, bool wakeup)
{
	bool flag_wakeup;
	int ret = 0;
	struct decon_device *decon = get_decon_drvdata(0);

	flag_wakeup = wakeup;

	if (flag_wakeup)
		decon_hiber_block_exit(decon);

	mutex_lock(&dsim->cmd_lock);
	if (!IS_DSIM_ON_STATE(dsim)) {
		dsim_err("%s dsim%d not ready (%s)\n",
				__func__, dsim->id, dsim_state_names[dsim->state]);
		ret = -EINVAL;
		goto err_exit;
	}
	DPU_EVENT_LOG_CMD(&dsim->sd, id, d0);

	reinit_completion(&dsim->ph_wr_comp);
	dsim_reg_clear_int(dsim->id, DSIM_INTSRC_SFR_PH_FIFO_EMPTY);

	/* Run write-fail dectector */
	mod_timer(&dsim->cmd_timer, jiffies + MIPI_WR_TIMEOUT);

	switch (id) {
	/* short packet types of packet types for command. */
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE:
	case MIPI_DSI_DSC_PRA:
	case MIPI_DSI_COLOR_MODE_OFF:
	case MIPI_DSI_COLOR_MODE_ON:
	case MIPI_DSI_SHUTDOWN_PERIPHERAL:
	case MIPI_DSI_TURN_ON_PERIPHERAL:
		dsim_reg_wr_tx_header(dsim->id, id, d0, d1, false);
		if (!must_wait)
			must_wait = dsim_fifo_empty_needed(dsim, id, d0);
		break;

	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
	case MIPI_DSI_DCS_READ:
		dsim_reg_wr_tx_header(dsim->id, id, d0, d1, true);
		if (!must_wait)
			must_wait = dsim_fifo_empty_needed(dsim, id, d0);
		break;

	/* long packet types of packet types for command. */
	case MIPI_DSI_GENERIC_LONG_WRITE:
	case MIPI_DSI_DCS_LONG_WRITE:
	case MIPI_DSI_DSC_PPS:
		dsim_long_data_wr(dsim, d0, d1);
		dsim_reg_wr_tx_header(dsim->id, id, d1 & 0xff,
				(d1 & 0xff00) >> 8, false);
		if (!must_wait)
			must_wait = dsim_fifo_empty_needed(dsim, id, *(u8 *)d0);
		break;

	default:
		dsim_info("DSIM wr unsupported id:0x%x\n", id);
		ret = -EINVAL;
	}

	ret = dsim_wait_for_cmd_fifo_empty(dsim, must_wait);
	if (ret < 0)
		dsim_err("DSIM wr timeout(id:0x%x d0:0x%lx)\n", id, d0);

err_exit:
	mutex_unlock(&dsim->cmd_lock);

	if (flag_wakeup)
		decon_hiber_unblock(decon);

	return ret;
}

int dsim_read_data(struct dsim_device *dsim, u32 id, u32 addr, u32 cnt, u8 *buf)
{
	u32 rx_fifo, rx_size = 0;
	int i, j, ret = 0;
	u32 rx_fifo_depth = DSIM_RX_FIFO_MAX_DEPTH;
	struct decon_device *decon = get_decon_drvdata(0);
	struct dsim_regs regs;

	decon_hiber_block_exit(decon);
	if (IS_DSIM_OFF_STATE(dsim)) {
		dsim_err("%s dsim%d not ready (%s)\n",
				__func__, dsim->id, dsim_state_names[dsim->state]);
		decon_hiber_unblock(decon);
		return -EINVAL;
	}

	reinit_completion(&dsim->rd_comp);

	/* Init RX FIFO before read and clear DSIM_INTSRC */
	dsim_reg_clear_int(dsim->id, DSIM_INTSRC_RX_DATA_DONE);

	/* Set the maximum packet size returned */
	dsim_write_data(dsim,
		MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE, cnt, 0, false, true);

	/* Read request */
	dsim_write_data(dsim, id, addr, 0, true, true);
	if (!wait_for_completion_timeout(&dsim->rd_comp, MIPI_RD_TIMEOUT)) {
		dsim_err("MIPI DSIM read Timeout!\n");
		decon_hiber_unblock(decon);
		return -ETIMEDOUT;
	}

	mutex_lock(&dsim->cmd_lock);
	if (IS_DSIM_OFF_STATE(dsim)) {
		dsim_err("%s dsim%d is off (%s)\n",
				__func__, dsim->id, dsim_state_names[dsim->state]);
		goto exit;
	}

	DPU_EVENT_LOG_CMD(&dsim->sd, id, (char)addr);

	do {
		rx_fifo = dsim_reg_get_rx_fifo(dsim->id);

		/* Parse the RX packet data types */
		switch (rx_fifo & 0xff) {
		case MIPI_DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT:
			ret = dsim_reg_rx_err_handler(dsim->id, rx_fifo);
			if (ret < 0) {
				dsim_to_regs_param(dsim, &regs);
				__dsim_dump(dsim->id, &regs);
				goto exit;
			}
			break;
		case MIPI_DSI_RX_END_OF_TRANSMISSION:
			dsim_dbg("EoTp was received from LCD module.\n");
			break;
		case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE:
		case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE:
			dsim_dbg("1byte Short Packet was received from LCD\n");
			buf[0] = (rx_fifo >> 8) & 0xff;
			rx_size = 1;
			break;
		case MIPI_DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE:
		case MIPI_DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE:
			dsim_dbg("2bytes Short Packet was received from LCD\n");
			for (i = 0; i < 2; i++)
				buf[i] = (rx_fifo >> (8 + i * 8)) & 0xff;
			rx_size = 2;
			break;
		case MIPI_DSI_RX_DCS_LONG_READ_RESPONSE:
		case MIPI_DSI_RX_GENERIC_LONG_READ_RESPONSE:
			dsim_dbg("Long Packet was received from LCD module.\n");
			rx_size = (rx_fifo & 0x00ffff00) >> 8;
			dsim_dbg("rx fifo : %8x, response : %x, rx_size : %d\n",
					rx_fifo, rx_fifo & 0xff, rx_size);
			/* Read data from RX packet payload */
			for (i = 0; i < rx_size >> 2; i++) {
				rx_fifo = dsim_reg_get_rx_fifo(dsim->id);
				for (j = 0; j < 4; j++)
					buf[(i*4)+j] = (u8)(rx_fifo >> (j * 8)) & 0xff;
			}
			if (rx_size % 4) {
				rx_fifo = dsim_reg_get_rx_fifo(dsim->id);
				for (j = 0; j < rx_size % 4; j++)
					buf[4 * i + j] =
						(u8)(rx_fifo >> (j * 8)) & 0xff;
			}
			break;
		default:
			dsim_err("Packet format is invaild.\n");
			dsim_to_regs_param(dsim, &regs);
			__dsim_dump(dsim->id, &regs);
			ret = -EBUSY;
			goto exit;
		}
	} while (!dsim_reg_rx_fifo_is_empty(dsim->id) && --rx_fifo_depth);

	ret = rx_size;
	if (!rx_fifo_depth) {
		dsim_err("Check DPHY values about HS clk.\n");
		dsim_to_regs_param(dsim, &regs);
		__dsim_dump(dsim->id, &regs);
		ret = -EBUSY;
	}
exit:
	mutex_unlock(&dsim->cmd_lock);
	decon_hiber_unblock(decon);

	return ret;
}

static void dsim_write_timeout_fn(struct work_struct *work)
{
	struct dsim_device *dsim =
		container_of(work, struct dsim_device, wr_timeout_work);
	struct dsim_regs regs;
	struct decon_device *decon = get_decon_drvdata(0);

	dsim_dbg("%s +\n", __func__);
	decon_hiber_block(decon);
	mutex_lock(&dsim->cmd_lock);
	if (IS_DSIM_OFF_STATE(dsim)) {
		dsim_err("%s dsim%d not ready (%s)\n",
				__func__, dsim->id, dsim_state_names[dsim->state]);
		goto exit;
	}

	/* If already FIFO empty even though the timer is no pending */
	if (dsim_reg_header_fifo_is_empty(dsim->id)) {
		reinit_completion(&dsim->ph_wr_comp);
		dsim_reg_clear_int(dsim->id, DSIM_INTSRC_SFR_PH_FIFO_EMPTY);
		goto exit;
	}

	dsim_to_regs_param(dsim, &regs);
	__dsim_dump(dsim->id, &regs);

exit:
	mutex_unlock(&dsim->cmd_lock);
	decon_hiber_unblock(decon);
	dsim_dbg("%s -\n", __func__);

	return;
}

static void dsim_cmd_fail_detector(unsigned long arg)
{
	struct dsim_device *dsim = (struct dsim_device *)arg;

	if (timer_pending(&dsim->cmd_timer)) {
		dsim_info("%s timer is pending\n", __func__);
		return;
	}

	queue_work(dsim->wq, &dsim->wr_timeout_work);
}

#if defined(CONFIG_EXYNOS_BTS)
static void dsim_bts_print_info(struct bts_decon_info *info)
{
	int i;

	for (i = 0; i < BTS_DPP_MAX; ++i) {
		if (!info->dpp[i].used)
			continue;

		dsim_info("\t\tDPP[%d] b(%d) s(%d %d) d(%d %d %d %d) r(%d)\n",
				i, info->dpp[i].bpp,
				info->dpp[i].src_w, info->dpp[i].src_h,
				info->dpp[i].dst.x1, info->dpp[i].dst.x2,
				info->dpp[i].dst.y1, info->dpp[i].dst.y2,
				info->dpp[i].rotation);
	}
}
#endif

static void dsim_underrun_info(struct dsim_device *dsim)
{
#if defined(CONFIG_EXYNOS_BTS)
	struct decon_device *decon;
	int i, decon_cnt;

	dsim_info("\tMIF(%lu), INT(%lu), DISP(%lu)\n",
			cal_dfs_get_rate(ACPM_DVFS_MIF),
			cal_dfs_get_rate(ACPM_DVFS_INT),
			cal_dfs_get_rate(ACPM_DVFS_DISP));

	decon_cnt = get_decon_drvdata(0)->dt.decon_cnt;

	for (i = 0; i < decon_cnt; ++i) {
		decon = get_decon_drvdata(i);

		if (decon) {
			dsim_info("\tDECON%d: bw(%u %u), disp(%u %u), p(%u)\n",
					decon->id,
					decon->bts.prev_total_bw,
					decon->bts.total_bw,
					decon->bts.prev_max_disp_freq,
					decon->bts.max_disp_freq,
					decon->bts.peak);
			dsim_bts_print_info(&decon->bts.bts_info);
		}
	}
#endif
}

static irqreturn_t dsim_irq_handler(int irq, void *dev_id)
{
	unsigned int int_src;
	struct dsim_device *dsim = dev_id;
	struct decon_device *decon = get_decon_drvdata(0);
#ifdef CONFIG_EXYNOS_PD
	int active;
#endif

	spin_lock(&dsim->slock);

#ifdef CONFIG_EXYNOS_PD
	active = pm_runtime_active(dsim->dev);
	if (!active) {
		dsim_info("dsim power(%d), state(%d)\n", active, dsim->state);
		spin_unlock(&dsim->slock);
		return IRQ_HANDLED;
	}
#endif

	int_src = dsim_reg_get_int_and_clear(dsim->id);
	if (int_src & DSIM_INTSRC_SFR_PH_FIFO_EMPTY) {
		del_timer(&dsim->cmd_timer);
		complete(&dsim->ph_wr_comp);
		dsim_dbg("dsim%d PH_FIFO_EMPTY irq occurs\n", dsim->id);
	}
	if (int_src & DSIM_INTSRC_RX_DATA_DONE)
		complete(&dsim->rd_comp);
	if (int_src & DSIM_INTSRC_FRAME_DONE)
		dsim_dbg("dsim%d framedone irq occurs\n", dsim->id);
	if (int_src & DSIM_INTSRC_ERR_RX_ECC)
		dsim_err("RX ECC Multibit error was detected!\n");

	if (int_src & DSIM_INTSRC_UNDER_RUN) {
		dsim->total_underrun_cnt++;
		dsim_info("dsim%d underrun irq occurs(%d)\n", dsim->id,
				dsim->total_underrun_cnt);
		dsim_underrun_info(dsim);
	}
	if (int_src & DSIM_INTSRC_VT_STATUS) {
		dsim_dbg("dsim%d vt_status(vsync) irq occurs\n", dsim->id);
		if (decon) {
			decon->vsync.timestamp = ktime_get();
			wake_up_interruptible_all(&decon->vsync.wait);
		}
	}

	spin_unlock(&dsim->slock);

	return IRQ_HANDLED;
}

static void dsim_clocks_info(struct dsim_device *dsim)
{
}

static int dsim_get_clocks(struct dsim_device *dsim)
{
	dsim->res.aclk = devm_clk_get(dsim->dev, "aclk");
	if (IS_ERR_OR_NULL(dsim->res.aclk)) {
		dsim_err("failed to get aclk\n");
		return PTR_ERR(dsim->res.aclk);
	}

	return 0;
}

int dsim_reset_panel(struct dsim_device *dsim)
{
	struct dsim_resources *res = &dsim->res;
	int ret;

	dsim_dbg("%s +\n", __func__);

	ret = gpio_request_one(res->lcd_reset, GPIOF_OUT_INIT_HIGH, "lcd_reset");
	if (ret < 0) {
		dsim_err("failed to get LCD reset GPIO\n");
		return -EINVAL;
	}

	usleep_range(5000, 6000);
	gpio_set_value(res->lcd_reset, 0);
	usleep_range(5000, 6000);
	gpio_set_value(res->lcd_reset, 1);

	gpio_free(res->lcd_reset);

	usleep_range(10000, 11000);

	dsim_dbg("%s -\n", __func__);
	return 0;
}

#if !defined(CONFIG_EXYNOS_COMMON_PANEL)
static int dsim_get_gpios(struct dsim_device *dsim)
{
	struct device *dev = dsim->dev;
	struct dsim_resources *res = &dsim->res;

	dsim_info("%s +\n", __func__);

	if (of_get_property(dev->of_node, "gpios", NULL) != NULL)  {
		/* panel reset */
		res->lcd_reset = of_get_gpio(dev->of_node, 0);
		if (res->lcd_reset < 0) {
			dsim_err("failed to get lcd reset GPIO");
			return -ENODEV;
		}
		res->lcd_power[0] = of_get_gpio(dev->of_node, 1);
		if (res->lcd_power[0] < 0) {
			res->lcd_power[0] = -1;
			dsim_info("This board doesn't support LCD power GPIO");
		}
		res->lcd_power[1] = of_get_gpio(dev->of_node, 2);
		if (res->lcd_power[1] < 0) {
			res->lcd_power[1] = -1;
			dsim_info("This board doesn't support 2nd LCD power GPIO");
		}
	}

	dsim_info("%s -\n", __func__);
	return 0;
}

static int dsim_get_regulator(struct dsim_device *dsim)
{
	struct device *dev = dsim->dev;
	struct dsim_resources *res = &dsim->res;

	char *str_regulator_1p8v = NULL;
	char *str_regulator_3p3v = NULL;

	res->regulator_1p8v = NULL;
	res->regulator_3p3v = NULL;

	if(!of_property_read_string(dev->of_node, "regulator_1p8v",
				(const char **)&str_regulator_1p8v)) {
		res->regulator_1p8v = regulator_get(dev, str_regulator_1p8v);
		if (IS_ERR(res->regulator_1p8v)) {
			dsim_err("%s : dsim regulator 1.8V get failed\n", __func__);
			res->regulator_1p8v = NULL;
		}
	}

	if(!of_property_read_string(dev->of_node, "regulator_3p3v",
				(const char **)&str_regulator_3p3v)) {
		res->regulator_3p3v = regulator_get(dev, str_regulator_3p3v);
		if (IS_ERR(res->regulator_3p3v)) {
			dsim_err("%s : dsim regulator 3.3V get failed\n", __func__);
			res->regulator_3p3v = NULL;
		}
	}

	return 0;
}

int dsim_set_panel_power(struct dsim_device *dsim, bool on)
{
	struct dsim_resources *res = &dsim->res;
	int ret;

	dsim_dbg("%s(%d) +\n", __func__, on);


	if (on) {
		if (res->lcd_power[0] > 0) {
			ret = gpio_request_one(res->lcd_power[0],
					GPIOF_OUT_INIT_HIGH, "lcd_power0");
			if (ret < 0) {
				dsim_err("failed LCD power on\n");
				return -EINVAL;
			}
			gpio_free(res->lcd_power[0]);
			usleep_range(10000, 11000);
		}

		if (res->lcd_power[1] > 0) {
			ret = gpio_request_one(res->lcd_power[1],
					GPIOF_OUT_INIT_HIGH, "lcd_power1");
			if (ret < 0) {
				dsim_err("failed 2nd LCD power on\n");
				return -EINVAL;
			}
			gpio_free(res->lcd_power[1]);
			usleep_range(10000, 11000);
		}
		if (res->regulator_1p8v > 0) {
			ret = regulator_enable(res->regulator_1p8v);
			if (ret) {
				dsim_err("%s : dsim regulator 1.8V enable failed\n", __func__);
				return ret;
			}
			usleep_range(5000, 6000);
		}

		if (res->regulator_3p3v > 0) {
			ret = regulator_enable(res->regulator_3p3v);
			if (ret) {
				dsim_err("%s : dsim regulator 3.3V enable failed\n", __func__);
				return ret;
			}
		}
	} else {
		ret = gpio_request_one(res->lcd_reset, GPIOF_OUT_INIT_LOW,
				"lcd_reset");
		if (ret < 0) {
			dsim_err("failed LCD reset off\n");
			return -EINVAL;
		}
		gpio_free(res->lcd_reset);

		if (res->lcd_power[0] > 0) {
			ret = gpio_request_one(res->lcd_power[0],
					GPIOF_OUT_INIT_LOW, "lcd_power0");
			if (ret < 0) {
				dsim_err("failed LCD power off\n");
				return -EINVAL;
			}
			gpio_free(res->lcd_power[0]);
			usleep_range(5000, 6000);
		}

		if (res->lcd_power[1] > 0) {
			ret = gpio_request_one(res->lcd_power[1],
					GPIOF_OUT_INIT_LOW, "lcd_power1");
			if (ret < 0) {
				dsim_err("failed 2nd LCD power off\n");
				return -EINVAL;
			}
			gpio_free(res->lcd_power[1]);
			usleep_range(5000, 6000);
		}
		if (res->regulator_1p8v > 0) {
			ret = regulator_disable(res->regulator_1p8v);
			if (ret) {
				dsim_err("%s : dsim regulator 1.8V disable failed\n", __func__);
				return ret;
			}
		}

		if (res->regulator_3p3v > 0) {
			ret = regulator_disable(res->regulator_3p3v);
			if (ret) {
				dsim_err("%s : dsim regulator 3.3V disable failed\n", __func__);
				return ret;
			}
		}
	}

	dsim_dbg("%s(%d) -\n", __func__, on);

	return 0;
}

#else
int dsim_function_reset(struct dsim_device *dsim)
{
	int ret = 0;
#if 0
	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block_exit(decon);

	mutex_lock(&dsim->cmd_lock);
	if (!IS_DSIM_ON_STATE(dsim)) {
		dsim_err("DSIM is off. state(%d)\n", dsim->state);
		ret = -EINVAL;
		goto err_exit;
	}
	dsim_reg_function_reset(dsim->id);
	dsim_info("dsim-%d sw function reset\n", dsim->id);

err_exit:
	mutex_unlock(&dsim->cmd_lock);
	decon_hiber_unblock(decon);
#endif

	return ret;
}

int dsim_set_panel_power(struct dsim_device *dsim, bool on)
{
	int ret;

	dsim_dbg("%s(%d) +\n", __func__, on);

	if (on)
		ret = call_panel_ops(dsim, poweron, dsim);
	else
		ret = call_panel_ops(dsim, poweroff, dsim);
	if (ret < 0) {
		dsim_err("%s failed to set power\n", __func__);
		return ret;
	}
	dsim_dbg("%s(%d) -\n", __func__, on);

	return 0;
}
#endif
static int _dsim_enable(struct dsim_device *dsim, enum dsim_state state)
{
	bool panel_ctrl;
	int ret = 0;

	if (IS_DSIM_ON_STATE(dsim)) {
		dsim_warn("%s dsim already on(%s)\n",
				__func__, dsim_state_names[dsim->state]);
		dsim->state = state;
		return 0;
	}

	dsim_dbg("%s %s +\n", __func__, dsim_state_names[dsim->state]);

#if defined(CONFIG_CPU_IDLE)
	exynos_update_ip_idle_status(dsim->idle_ip_index, 0);
#endif

	pm_runtime_get_sync(dsim->dev);

	/* DPHY reset control from SYSREG(0) */
	dpu_sysreg_select_dphy_rst_control(dsim->res.ss_regs, dsim->id, 0);

	/* DPHY power on : iso release */
	phy_power_on(dsim->phy);
	if (dsim->phy_ex)
		phy_power_on(dsim->phy_ex);

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	dsim_update_adaptive_freq(dsim, false);
#endif
	panel_ctrl = (state == DSIM_STATE_ON || state == DSIM_STATE_DOZE) ? true : false;
	ret = dsim_reg_init(dsim->id, &dsim->lcd_info, &dsim->clks, panel_ctrl);

#ifdef CONFIG_DYNAMIC_FREQ
	dsim->df_mode = DSIM_MODE_POWER_OFF;
	call_panel_ops(dsim, set_df_default, dsim);
#endif

	dsim_reg_start(dsim->id);

	dsim->state = state;
	enable_irq(dsim->res.irq);

	return ret;
}

static int dsim_enable(struct dsim_device *dsim)
{
	int ret;
	enum dsim_state prev_state = dsim->state;
	enum dsim_state next_state = DSIM_STATE_ON;

	if (prev_state == next_state) {
		dsim_warn("dsim-%d %s already %s state\n", dsim->id,
				__func__, dsim_state_names[dsim->state]);
		return 0;
	}

	dsim_info("dsim-%d %s +\n", dsim->id, __func__);
	ret = _dsim_enable(dsim, next_state);
	if (ret < 0) {
		dsim_err("dsim-%d failed to set %s (ret %d)\n",
				dsim->id, dsim_state_names[next_state], ret);
		goto out;
	}

	if (prev_state != DSIM_STATE_INIT) {
#if defined(CONFIG_EXYNOS_COMMON_PANEL)
		ret = call_panel_ops(dsim, sleepout, dsim);
#else
		ret = call_panel_ops(dsim, displayon, dsim);
#endif
		if (ret < 0) {
			dsim_err("dsim-%d failed to set %s (ret %d)\n",
					dsim->id, dsim_state_names[next_state], ret);
			goto out;
		}
	}

	dsim_info("dsim-%d %s - (state:%s -> %s)\n", dsim->id, __func__,
			dsim_state_names[prev_state],
			dsim_state_names[dsim->state]);

out:
	return ret;
}

static int dsim_doze(struct dsim_device *dsim)
{
	int ret;
	enum dsim_state prev_state = dsim->state;
	enum dsim_state next_state = DSIM_STATE_DOZE;

	if (prev_state == next_state) {
		dsim_warn("dsim-%d %s already %s state\n", dsim->id,
				__func__, dsim_state_names[dsim->state]);
		return 0;
	}

	dsim_info("dsim-%d %s +\n", dsim->id, __func__);
	ret = _dsim_enable(dsim, next_state);
	if (ret < 0) {
		dsim_err("dsim-%d failed to set %s (ret %d)\n",
				dsim->id, dsim_state_names[next_state], ret);
		goto out;
	}
	if (prev_state != DSIM_STATE_INIT) {
		ret = call_panel_ops(dsim, doze, dsim);
		if (ret < 0) {
			dsim_err("dsim-%d failed to set %s (ret %d)\n",
					dsim->id, dsim_state_names[next_state], ret);
			goto out;
		}
	}
	dsim_info("dsim-%d %s - (state:%s -> %s)\n", dsim->id, __func__,
			dsim_state_names[prev_state],
			dsim_state_names[dsim->state]);

out:
	return ret;
}

static int _dsim_disable(struct dsim_device *dsim, enum dsim_state state)
{
	struct dsim_regs regs;

	if (IS_DSIM_OFF_STATE(dsim)) {
		dsim_warn("%s dsim already off(%s)\n",
				__func__, dsim_state_names[dsim->state]);
		if (state == DSIM_STATE_OFF)
			dsim_set_panel_power(dsim, 0);
		dsim->state = state;
		return 0;
	}

	dsim_dbg("%s %s +\n", __func__, dsim_state_names[dsim->state]);

	/* Wait for current read & write CMDs. */
	mutex_lock(&dsim->cmd_lock);
	del_timer(&dsim->cmd_timer);
	dsim->state = state;
	mutex_unlock(&dsim->cmd_lock);

	if (dsim_reg_stop(dsim->id, dsim->data_lane) < 0) {
		dsim_to_regs_param(dsim, &regs);
		__dsim_dump(dsim->id, &regs);
	}
	disable_irq(dsim->res.irq);

	/* HACK */
	phy_power_off(dsim->phy);
	if (dsim->phy_ex)
		phy_power_off(dsim->phy_ex);

	if (state == DSIM_STATE_OFF)
		dsim_set_panel_power(dsim, 0);

	pm_runtime_put_sync(dsim->dev);

	dsim_dbg("%s %s -\n", __func__, dsim_state_names[dsim->state]);
#if defined(CONFIG_CPU_IDLE)
	exynos_update_ip_idle_status(dsim->idle_ip_index, 1);
#endif

	return 0;
}

static int dsim_disable(struct dsim_device *dsim)
{
	int ret;
	enum dsim_state prev_state = dsim->state;
	enum dsim_state next_state = DSIM_STATE_OFF;

	if (prev_state == next_state) {
		dsim_warn("dsim-%d %s already %s state\n", dsim->id,
				__func__, dsim_state_names[dsim->state]);
		return 0;
	}

	dsim_info("dsim-%d %s +\n", dsim->id, __func__);
	call_panel_ops(dsim, suspend, dsim);
	ret = _dsim_disable(dsim, next_state);
	if (ret < 0) {
		dsim_err("dsim-%d failed to set %s (ret %d)\n",
				dsim->id, dsim_state_names[next_state], ret);
		goto out;
	}
	dsim_info("dsim-%d %s - (state:%s -> %s)\n", dsim->id, __func__,
			dsim_state_names[prev_state],
			dsim_state_names[dsim->state]);

out:
	return ret;
}

static int dsim_doze_suspend(struct dsim_device *dsim)
{
	int ret;
	enum dsim_state prev_state = dsim->state;
	enum dsim_state next_state = DSIM_STATE_DOZE_SUSPEND;

	if (prev_state == next_state) {
		dsim_warn("dsim-%d %s already %s state\n", dsim->id,
				__func__, dsim_state_names[dsim->state]);
		return 0;
	}

	dsim_info("dsim-%d %s +\n", dsim->id, __func__);
	call_panel_ops(dsim, doze_suspend, dsim);
	ret = _dsim_disable(dsim, next_state);
	if (ret < 0) {
		dsim_err("dsim-%d failed to set %s (ret %d)\n",
				dsim->id, dsim_state_names[next_state], ret);
		goto out;
	}
	dsim_info("dsim-%d %s - (state:%s -> %s)\n", dsim->id, __func__,
			dsim_state_names[prev_state],
			dsim_state_names[dsim->state]);

out:
	return ret;
}

static int dsim_enter_ulps(struct dsim_device *dsim)
{
	int ret = 0;

	DPU_EVENT_START();
	dsim_dbg("%s +\n", __func__);

	if (!IS_DSIM_ON_STATE(dsim)) {
		ret = -EBUSY;
		goto err;
	}

	/* Wait for current read & write CMDs. */
	mutex_lock(&dsim->cmd_lock);
	dsim->state = DSIM_STATE_ULPS;
	mutex_unlock(&dsim->cmd_lock);

	disable_irq(dsim->res.irq);
	ret = dsim_reg_stop_and_enter_ulps(dsim->id, dsim->lcd_info.ddi_type,
			dsim->data_lane);
	if (ret < 0)
		dsim_dump(dsim, 0);

	phy_power_off(dsim->phy);
	if (dsim->phy_ex)
		phy_power_off(dsim->phy_ex);

	pm_runtime_put_sync(dsim->dev);
#if defined(CONFIG_CPU_IDLE)
	exynos_update_ip_idle_status(dsim->idle_ip_index, 1);
#endif

	DPU_EVENT_LOG(DPU_EVT_ENTER_ULPS, &dsim->sd, start);
err:
	dsim_dbg("%s -\n", __func__);
	return ret;
}

static int dsim_exit_ulps(struct dsim_device *dsim)
{
	int ret = 0;

	DPU_EVENT_START();
	dsim_dbg("%s +\n", __func__);

	if (dsim->state != DSIM_STATE_ULPS) {
		ret = -EBUSY;
		goto err;
	}
#if defined(CONFIG_CPU_IDLE)
	exynos_update_ip_idle_status(dsim->idle_ip_index, 0);
#endif

	pm_runtime_get_sync(dsim->dev);

	/* DPHY reset control from SYSREG(0) */
	dpu_sysreg_select_dphy_rst_control(dsim->res.ss_regs, dsim->id, 0);
	/* DPHY power on : iso release */
	phy_power_on(dsim->phy);
	if (dsim->phy_ex)
		phy_power_on(dsim->phy_ex);

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	dsim_update_adaptive_freq(dsim, false);
#endif
	dsim_reg_init(dsim->id, &dsim->lcd_info, &dsim->clks, false);

#ifdef CONFIG_DYNAMIC_FREQ
	dsim->df_mode = DSIM_MODE_HIBERNATION;
	call_panel_ops(dsim, set_df_default, dsim);
#endif
	ret = dsim_reg_exit_ulps_and_start(dsim->id, dsim->lcd_info.ddi_type,
			dsim->data_lane);
	if (ret < 0)
		dsim_dump(dsim, 0);

	enable_irq(dsim->res.irq);

	dsim->state = DSIM_STATE_ON;
	DPU_EVENT_LOG(DPU_EVT_EXIT_ULPS, &dsim->sd, start);
err:
	dsim_dbg("%s -\n", __func__);

	return 0;
}

static int dsim_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);

	if (enable)
		return dsim_enable(dsim);
	else
		return dsim_disable(dsim);
}


#ifdef CONFIG_DYNAMIC_FREQ


static int dsim_set_pre_freq_hop(struct dsim_device *dsim, struct df_param *param)
{
	int ret = 0;
	
#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	return ret;
#endif

	if (param->context)
		dsim_dbg("[DYN_FREQ]:INFO:%s:p,m,k:%d,%d,%d\n", 
			__func__, param->pms.p, param->pms.m, param->pms.k);

	dsim_reg_set_dphy_freq_hopping(dsim->id,
		param->pms.p, param->pms.m, param->pms.k, 1);

	//memcpy(status->c_lp_ref, status->r_lp_ref, sizeof(unsigned int) * mres_cnt);

	return ret;
}


static int dsim_set_post_freq_hop(struct dsim_device *dsim, struct df_param *param)
{
	int ret = 0;

#if defined(CONFIG_SOC_EXYNOS9820_EVT0)
	return ret;
#endif

	if (param->context)
		dsim_dbg("[DYN_FREQ]:INFO:%s:p,m,k:%d,%d,%d\n", 
			__func__, param->pms.p, param->pms.m, param->pms.k);

	dsim_reg_set_dphy_freq_hopping(dsim->id,
		param->pms.p, param->pms.m, param->pms.k, 0);

	return ret;
}
#endif

static int dsim_set_freq_hop(struct dsim_device *dsim, struct decon_freq_hop *freq)
{
#if !defined(CONFIG_SOC_EXYNOS9820_EVT0)
	struct stdphy_pms *pms;

	if (!IS_DSIM_ON_STATE(dsim)) {
		dsim_err("%s: dsim%d is off state\n", __func__, dsim->id);
		return -EINVAL;
	}

	pms = &dsim->lcd_info.dphy_pms;
	/* If target M value is 0, frequency hopping will be disabled */
	dsim_reg_set_dphy_freq_hopping(dsim->id, pms->p, freq->target_m,
			freq->target_k, (freq->target_m > 0) ? 1 : 0);
#endif

	return 0;
}

#if defined(CONFIG_EXYNOS_ADAPTIVE_FREQ)
static int dsim_set_adaptive_freq(struct dsim_device *dsim, int req_freq_idx)
{
	struct adaptive_freq_info *freq_info;
	struct adaptive_info *adap_info;
	struct adaptive_idx *adap_idx;
	int i;

	adap_info = &dsim->lcd_info.adaptive_info;
	adap_idx = dsim->lcd_info.adaptive_info.adap_idx;
	if (adap_idx == NULL)
		return -EINVAL;

	if (req_freq_idx >= adap_info->freq_cnt)
		return -EINVAL;

	freq_info = &adap_info->freq_info[req_freq_idx];

	dsim_info("[ADAP_FREQ] DSIM:CUR_FREQ_IDX: %d -> %d\n",
			adap_idx->cur_freq_idx, req_freq_idx);

	dsim_info("[ADAP_FREQ] DSIM:FREQ:%d(%d, %d, %d, %d) -> %d(%d, %d, %d, %d)\n",
			dsim->lcd_info.hs_clk,
			dsim->lcd_info.dphy_pms.p, dsim->lcd_info.dphy_pms.m,
			dsim->lcd_info.dphy_pms.s, dsim->lcd_info.dphy_pms.k,
			freq_info->hs_clk,
			freq_info->dphy_pms.p, freq_info->dphy_pms.m,
			freq_info->dphy_pms.s, freq_info->dphy_pms.k);

	dsim_info("[ADAP_FREQ] DSIM:ESC_CLK: %d -> %d\n",
			dsim->lcd_info.esc_clk, freq_info->esc_clk);

	for (i = 0; i < dsim->lcd_info.dt_lcd_mres.mres_number; i++)
		dsim_info("[ADAP_FREQ] DSIM:CMD_UNDERRUN_LP[%d]: %d -> %d\n",
				i, dsim->lcd_info.cmd_underrun_lp_ref[i],
				freq_info->cmd_underrun_lp_ref[i]);

	dsim->lcd_info.hs_clk = freq_info->hs_clk;
	dsim->clks.hs_clk = dsim->lcd_info.hs_clk;

	dsim->lcd_info.esc_clk = freq_info->esc_clk;
	dsim->clks.esc_clk = dsim->lcd_info.esc_clk;

	memcpy(&dsim->lcd_info.dphy_pms, &freq_info->dphy_pms, MAX_PMSK_CNT);
	memcpy(dsim->lcd_info.cmd_underrun_lp_ref,
			freq_info->cmd_underrun_lp_ref,
			(u32)ARRAY_SIZE(dsim->lcd_info.cmd_underrun_lp_ref));

	adap_idx->cur_freq_idx = req_freq_idx;

	return 0;
}

int dsim_update_adaptive_freq(struct dsim_device *dsim, bool force)
{
	int ret = 0;
	struct decon_lcd *lcd_info;
	struct adaptive_info *adap_info;
	struct adaptive_idx *adap_idx;
	int req_freq_idx;

	lcd_info = &dsim->lcd_info;
	adap_info = &dsim->lcd_info.adaptive_info;
	adap_idx = dsim->lcd_info.adaptive_info.adap_idx;
	if (adap_idx == NULL)
		return -EINVAL;

	mutex_lock(&dsim->adap_freq.lock);
	req_freq_idx = adap_idx->req_freq_idx;
	if (!force && adap_idx->cur_freq_idx == req_freq_idx) {
		mutex_unlock(&dsim->adap_freq.lock);
		return 0;
	}

	if (req_freq_idx >= adap_info->freq_cnt) {
		dsim_info("[ADAP_FREQ] %s:req_idx:%d exceed freq cnt:%d->Set to 0\n",
				__func__, adap_idx->req_freq_idx, adap_info->freq_cnt);
		adap_idx->req_freq_idx = 0;
		req_freq_idx = 0;
	}

	ret = dsim_set_adaptive_freq(dsim, req_freq_idx);
	mutex_unlock(&dsim->adap_freq.lock);

	/* run adap_freq_thread */
	dsim->adap_freq.timestamp = ktime_get();
	wake_up_interruptible_all(&dsim->adap_freq.wait);

	return ret;
}

static int dsim_mipi_freq_change(struct dsim_device *dsim)
{
	int ret = 0;
	struct adaptive_idx *adap_idx;
	struct decon_device *decon = get_decon_drvdata(0);

	decon_hiber_block_exit(decon);
	adap_idx = dsim->lcd_info.adaptive_info.adap_idx;
	mutex_lock(&dsim->adap_freq.lock);
	dsim_info("[ADAP_FREQ] DSIM:FFC Changed %d\n",
			adap_idx->cur_freq_idx);
	ret = call_panel_ops(dsim, mipi_freq_change, dsim);
	mutex_unlock(&dsim->adap_freq.lock);
	decon_hiber_unblock(decon);

	return ret;
}

static int search_adap_idx(int band_idx, int freq)
{
	int ret = 0;
	int array_idx;
	int i;
	int min, max;
	struct ril_band_info *info_tbl;
	struct band_info *band_info;

	if (band_idx >= RIL_BAND_MAX) {
		dsim_err("[ADAP_FREQ] ERR:%s: exceed max band idx : %d\n", __func__, band_idx);
		ret = -1;
		goto search_exit;
	}

	info_tbl = &total_band_info[band_idx];

	if (info_tbl == NULL) {
		dsim_err("[ADAP_FREQ] ERR:%s:failed to find band_idx : %d\n", __func__, band_idx);
		ret = -1;
		goto search_exit;
	}
	array_idx = info_tbl->size;

	for (i = 0; i < array_idx; i++) {
		band_info = &info_tbl->array[i];
		min = (int)freq - band_info->min;
		max = (int)freq - band_info->max;

		if ((min >= 0) && (max <= 0)) {
			dsim_info("[ADAP_FREQ] Found adap_freq idx : %d\n", band_info->freq_idx);
			ret = band_info->freq_idx;
			break;
		}
	}

	if (i >= array_idx) {
		dsim_err("[ADAP_FREQ] ERR:%s:Can't found freq idx\n", __func__);
		ret = -1;
		goto search_exit;
	}

search_exit:
	return ret;
}

static int dsim_ril_notifier(struct notifier_block *self, unsigned long size, void *buf)
{
	int req_idx = 0;
	struct dsim_device *dsim;
	struct dev_ril_bridge_msg *msg;
	struct ril_noti_info *noti_info;
	struct adaptive_info *adap_info;
	struct adaptive_idx *adap_idx;

	dsim = container_of(self, struct dsim_device, ril_notif);
	if (dsim == NULL) {
		dsim_err("[ADPA_FREQ] ERR:%s:dsim is null\n", __func__);
		goto noti_exit;
	}

	adap_info = &dsim->lcd_info.adaptive_info;
	adap_idx = dsim->lcd_info.adaptive_info.adap_idx;
	if (adap_idx == NULL) {
		dsim_err("[ADPA_FREQ] ERR:%s:adap_idx is null\n", __func__);
		goto noti_exit;
	}

	msg = (struct dev_ril_bridge_msg *)buf;
	if (msg == NULL) {
		dsim_err("[ADPA_FREQ] ERR:%s:msg is null\n", __func__);
		goto noti_exit;
	}

	if (size == sizeof(struct dev_ril_bridge_msg)
			&& msg->dev_id == IPC_SYSTEM_CP_CHANNEL_INFO
			&& msg->data_len == sizeof(struct ril_noti_info)) {
		noti_info = (struct ril_noti_info *)msg->data;
		if (noti_info == NULL) {
			dsim_err("[ADPA_FREQ] ERR:%s:noti_info is null\n", __func__);
			goto noti_exit;
		}

		dsim_info("[ADAP_FREQ] %s: (b:%d, c:%d)\n",
			__func__, noti_info->band, noti_info->channel);

		req_idx = search_adap_idx(noti_info->band, noti_info->channel);
		if (req_idx < 0) {
			dsim_err("[ADPA_FREQ] ERR:%s failed to found adap freq idx\n",
				__func__);
			goto noti_exit;
		}

		if (req_idx >= adap_info->freq_cnt) {
			dsim_err("[ADAP_FREQ]:ERR:%s:Wrong freq idx : %d\n",
				__func__, req_idx);
			goto noti_exit;
		}

		mutex_lock(&dsim->adap_freq.lock);
		dsim_info("[ADAP_FREQ] %s: req_freq_idx %d -> %d\n",
				__func__, adap_idx->req_freq_idx, req_idx);
		adap_idx->req_freq_idx = req_idx;
		mutex_unlock(&dsim->adap_freq.lock);
	}

noti_exit:
	return NOTIFY_OK;
}

static int dsim_adap_freq_thread(void *data)
{
	struct dsim_device *dsim = data;
	ktime_t timestamp;
	int ret;

	while (!kthread_should_stop()) {
		timestamp = dsim->adap_freq.timestamp;
#if defined(CONFIG_SUPPORT_KERNEL_4_9)
		ret = wait_event_interruptible(dsim->adap_freq.wait,
			!ktime_equal(timestamp, dsim->adap_freq.timestamp) &&
			dsim->adap_freq.active);
#else
		ret = wait_event_interruptible(dsim->adap_freq.wait,
			(timestamp != dsim->adap_freq.timestamp) &&
			dsim->adap_freq.active);
#endif

		if (!ret) {
			ret = dsim_mipi_freq_change(dsim);
			if (ret)
				dsim_err("DSIM:ERR:%s:failed to set FREQ_CHANGED\n", __func__);
		}
	}

	return 0;
}

int dsim_create_adap_freq_thread(struct dsim_device *dsim)
{
	char name[32];

	sprintf(name, "dsim%d-adap_freq", dsim->id);
	dsim->adap_freq.thread = kthread_run(dsim_adap_freq_thread, dsim, name);
	if (IS_ERR_OR_NULL(dsim->adap_freq.thread)) {
		dsim_err("failed to run adaptive freq thread\n");
		dsim->adap_freq.thread = NULL;
		return PTR_ERR(dsim->adap_freq.thread);
	}

	return 0;
}

void dsim_destroy_adap_freq_thread(struct dsim_device *dsim)
{
	if (dsim->adap_freq.thread)
		kthread_stop(dsim->adap_freq.thread);
}
#endif

static long dsim_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct dsim_device *dsim = container_of(sd, struct dsim_device, sd);
	int ret = 0;

	switch (cmd) {
	case DSIM_IOC_GET_LCD_INFO:
		v4l2_set_subdev_hostdata(sd, &dsim->lcd_info);
		break;

	case DSIM_IOC_ENTER_ULPS:
		if ((unsigned long)arg)
			ret = dsim_enter_ulps(dsim);
		else
			ret = dsim_exit_ulps(dsim);
		break;

	case DSIM_IOC_DUMP:
		dsim_info("DSIM_IOC_DUMP : %d\n", *((u32 *)arg));
		dsim_dump(dsim, *((u32 *)arg));
		break;

	case DSIM_IOC_GET_WCLK:
		v4l2_set_subdev_hostdata(sd, &dsim->clks.word_clk);
		break;

	case EXYNOS_DPU_GET_ACLK:
		return clk_get_rate(dsim->res.aclk);

	case DSIM_IOC_DOZE:
		ret = dsim_doze(dsim);
		break;

	case DSIM_IOC_DOZE_SUSPEND:
		ret = dsim_doze_suspend(dsim);
		break;

	case DSIM_IOC_SET_FREQ_HOP:
		ret = dsim_set_freq_hop(dsim, (struct decon_freq_hop *)arg);
		break;

#ifdef CONFIG_DYNAMIC_FREQ
	case DSIM_IOC_SET_PRE_FREQ_HOP:
		ret = dsim_set_pre_freq_hop(dsim, (struct df_param *) arg);
		break;
	
	case DSIM_IOC_SET_POST_FREQ_HOP:
		ret = dsim_set_post_freq_hop(dsim, (struct df_param *)arg);
		break;
#endif

#if defined(CONFIG_EXYNOS_COMMON_PANEL)
	case DSIM_IOC_NOTIFY:
		call_panel_ops(dsim, notify, dsim, arg);
		break;

	case DSIM_IOC_SET_ERROR_CB:
		if (arg == NULL) {
			dsim_err("%s invalid arg\n", __func__);
			ret = -EINVAL;
			break;
		}
		dsim->error_cb_info.error_cb = ((struct disp_error_cb_info *)arg)->error_cb;
		dsim->error_cb_info.data = ((struct disp_error_cb_info *)arg)->data;
		call_panel_ops(dsim, set_error_cb, dsim);
		break;
#endif

#if defined(CONFIG_EXYNOS_ADAPTIVE_FREQ)
	case DSIM_IOC_FREQ_CHANGE:
		ret = dsim_mipi_freq_change(dsim);
		break;
#endif

	default:
		dsim_err("unsupported ioctl");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops dsim_sd_core_ops = {
	.ioctl = dsim_ioctl,
};

static const struct v4l2_subdev_video_ops dsim_sd_video_ops = {
	.s_stream = dsim_s_stream,
};

static const struct v4l2_subdev_ops dsim_subdev_ops = {
	.core = &dsim_sd_core_ops,
	.video = &dsim_sd_video_ops,
};

static void dsim_init_subdev(struct dsim_device *dsim)
{
	struct v4l2_subdev *sd = &dsim->sd;

	v4l2_subdev_init(sd, &dsim_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = dsim->id;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "dsim-sd", dsim->id);
	v4l2_set_subdevdata(sd, dsim);
}

static int dsim_cmd_sysfs_write(struct dsim_device *dsim, bool on)
{
	int ret = 0;

	if (on)
		ret = dsim_write_data(dsim, MIPI_DSI_DCS_SHORT_WRITE,
			MIPI_DCS_SET_DISPLAY_ON, 0, false, true);
	else
		ret = dsim_write_data(dsim, MIPI_DSI_DCS_SHORT_WRITE,
			MIPI_DCS_SET_DISPLAY_OFF, 0, false, true);
	if (ret < 0)
		dsim_err("Failed to write test data!\n");
	else
		dsim_dbg("Succeeded to write test data!\n");

	return ret;
}

static int dsim_cmd_sysfs_read(struct dsim_device *dsim)
{
	int ret = 0;
	unsigned int id;
	u8 buf[4];

	/* dsim sends the request for the lcd id and gets it buffer */
	ret = dsim_read_data(dsim, MIPI_DSI_DCS_READ,
		MIPI_DCS_GET_DISPLAY_ID, DSIM_DDI_ID_LEN, buf);
	id = *(unsigned int *)buf;
	if (ret < 0)
		dsim_err("Failed to read panel id!\n");
	else
		dsim_info("Suceeded to read panel id : 0x%08x\n", id);

	return ret;
}

static ssize_t dsim_cmd_sysfs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t dsim_cmd_sysfs_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	unsigned long cmd;
	struct dsim_device *dsim = dev_get_drvdata(dev);

	ret = kstrtoul(buf, 0, &cmd);
	if (ret)
		return ret;

	switch (cmd) {
	case 1:
		ret = dsim_cmd_sysfs_read(dsim);
		call_panel_ops(dsim, dump, dsim);
		if (ret)
			return ret;
		break;
	case 2:
		ret = dsim_cmd_sysfs_write(dsim, true);
		dsim_info("Dsim write command, display on!!\n");
		if (ret)
			return ret;
		break;
	case 3:
		ret = dsim_cmd_sysfs_write(dsim, false);
		dsim_info("Dsim write command, display off!!\n");
		if (ret)
			return ret;
		break;
	default :
		dsim_info("unsupportable command\n");
		break;
	}

	return count;
}
static DEVICE_ATTR(cmd_rw, 0644, dsim_cmd_sysfs_show, dsim_cmd_sysfs_store);

int dsim_create_cmd_rw_sysfs(struct dsim_device *dsim)
{
	int ret = 0;

	ret = device_create_file(dsim->dev, &dev_attr_cmd_rw);
	if (ret)
		dsim_err("failed to create command read & write sysfs\n");

	return ret;
}

static int dsim_calc_slice_width(u32 dsc_cnt, u32 slice_num, u32 xres)
{
	u32 slice_width;
	u32 width_eff;
	u32 slice_width_byte_unit, comp_slice_width_byte_unit;
	u32 comp_slice_width_pixel_unit;
	u32 compressed_slice_w = 0;
	u32 i, j;

	if (dsc_cnt == 2)
		width_eff = xres >> 1;
	else
		width_eff = xres;

	if (slice_num / dsc_cnt == 2)
		slice_width = width_eff >> 1;
	else
		slice_width = width_eff;

	/* 3bytes per pixel */
	slice_width_byte_unit = slice_width * 3;
	/* integer value, /3 for 1/3 compression */
	comp_slice_width_byte_unit = slice_width_byte_unit / 3;
	/* integer value, /3 for pixel unit */
	comp_slice_width_pixel_unit = comp_slice_width_byte_unit / 3;

	i = comp_slice_width_byte_unit % 3;
	j = comp_slice_width_pixel_unit % 2;

	if (i == 0 && j == 0) {
		compressed_slice_w = comp_slice_width_pixel_unit;
	} else if (i == 0 && j != 0) {
		compressed_slice_w = comp_slice_width_pixel_unit + 1;
	} else if (i != 0) {
		while (1) {
			comp_slice_width_pixel_unit++;
			j = comp_slice_width_pixel_unit % 2;
			if (j == 0)
				break;
		}
		compressed_slice_w = comp_slice_width_pixel_unit;
	}

	return compressed_slice_w;
}

#if !defined(CONFIG_EXYNOS_COMMON_PANEL)
static void dsim_parse_lcd_info(struct dsim_device *dsim)
{
	u32 res[14];
	struct device_node *node;
	unsigned int mres_num = 1;
	u32 mres_w[3] = {0, };
	u32 mres_h[3] = {0, };
	u32 mres_dsc_w[3] = {0, };
	u32 mres_dsc_h[3] = {0, };
	u32 mres_dsc_en[3] = {0, };
	u32 mres_partial_w[3] = {0, };
	u32 mres_partial_h[3] = {0, };
	u32 hdr_num = 0;
	u32 hdr_type[HDR_CAPA_NUM] = {0, };
	u32 hdr_mxl = 0;
	u32 hdr_mal = 0;
	u32 hdr_mnl = 0;
	int k;

	node = of_parse_phandle(dsim->dev->of_node, "lcd_info", 0);

	of_property_read_u32(node, "mode", &dsim->lcd_info.mode);
	dsim_info("%s mode\n", dsim->lcd_info.mode ? "command" : "video");

	of_property_read_u32_array(node, "resolution", res, 2);
	dsim->lcd_info.xres = res[0];
	dsim->lcd_info.yres = res[1];
	dsim_info("LCD(%s) resolution: xres(%d), yres(%d)\n",
			of_node_full_name(node), res[0], res[1]);

	of_property_read_u32_array(node, "size", res, 2);
	dsim->lcd_info.width = res[0];
	dsim->lcd_info.height = res[1];
	dsim_dbg("LCD size: width(%d), height(%d)\n", res[0], res[1]);

	of_property_read_u32(node, "timing,refresh", &dsim->lcd_info.fps);
	dsim_dbg("LCD refresh rate(%d)\n", dsim->lcd_info.fps);

	of_property_read_u32_array(node, "timing,h-porch", res, 3);
	dsim->lcd_info.hbp = res[0];
	dsim->lcd_info.hfp = res[1];
	dsim->lcd_info.hsa = res[2];
	dsim_dbg("hbp(%d), hfp(%d), hsa(%d)\n", res[0], res[1], res[2]);

	of_property_read_u32_array(node, "timing,v-porch", res, 3);
	dsim->lcd_info.vbp = res[0];
	dsim->lcd_info.vfp = res[1];
	dsim->lcd_info.vsa = res[2];
	dsim_dbg("vbp(%d), vfp(%d), vsa(%d)\n", res[0], res[1], res[2]);

	of_property_read_u32(node, "timing,dsi-hs-clk", &dsim->lcd_info.hs_clk);
	dsim->clks.hs_clk = dsim->lcd_info.hs_clk;
	dsim_dbg("requested hs clock(%d)\n", dsim->lcd_info.hs_clk);

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	of_property_read_u32_array(node, "timing,pmsk", res, 14);
#else
	of_property_read_u32_array(node, "timing,pmsk", res, 4);
#endif
	dsim->lcd_info.dphy_pms.p = res[0];
	dsim->lcd_info.dphy_pms.m = res[1];
	dsim->lcd_info.dphy_pms.s = res[2];
	dsim->lcd_info.dphy_pms.k = res[3];
	dsim_dbg("p(%d), m(%d), s(%d), k(%d)\n", res[0], res[1], res[2], res[3]);
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	dsim->lcd_info.dphy_pms.mfr = res[4];
	dsim->lcd_info.dphy_pms.mrr = res[5];
	dsim->lcd_info.dphy_pms.sel_pf = res[6];
	dsim->lcd_info.dphy_pms.icp = res[7];
	dsim->lcd_info.dphy_pms.afc_enb = res[8];
	dsim->lcd_info.dphy_pms.extafc = res[9];
	dsim->lcd_info.dphy_pms.feed_en = res[10];
	dsim->lcd_info.dphy_pms.fsel = res[11];
	dsim->lcd_info.dphy_pms.fout_mask = res[12];
	dsim->lcd_info.dphy_pms.rsel = res[13];
	dsim_info(" mfr(%d), mrr(0x%x), sel_pf(%d), icp(%d)\n",
				res[4], res[5], res[6], res[7]);
	dsim_info(" afc_enb(%d), extafc(%d), feed_en(%d), fsel(%d)\n",
				res[8], res[9], res[10], res[11]);
	dsim_info(" fout_mask(%d), rsel(%d)\n", res[12], res[13]);
#endif

	of_property_read_u32(node, "timing,dsi-escape-clk",
			&dsim->lcd_info.esc_clk);
	dsim->clks.esc_clk = dsim->lcd_info.esc_clk;
	dsim_dbg("requested escape clock(%d)\n", dsim->lcd_info.esc_clk);

	of_property_read_u32(node, "mic_en", &dsim->lcd_info.mic_enabled);
	dsim_info("mic enabled (%d)\n", dsim->lcd_info.mic_enabled);

	of_property_read_u32(node, "type_of_ddi", &dsim->lcd_info.ddi_type);
	dsim_dbg("ddi type(%d)\n", dsim->lcd_info.ddi_type);

	of_property_read_u32(node, "dsc_en", &dsim->lcd_info.dsc_enabled);
	dsim_info("dsc is %s\n", dsim->lcd_info.dsc_enabled ? "enabled" : "disabled");

	if (dsim->lcd_info.dsc_enabled) {
		of_property_read_u32(node, "dsc_cnt", &dsim->lcd_info.dsc_cnt);
		dsim_info("dsc count(%d)\n", dsim->lcd_info.dsc_cnt);
		of_property_read_u32(node, "dsc_slice_num",
				&dsim->lcd_info.dsc_slice_num);
		dsim_info("dsc slice count(%d)\n", dsim->lcd_info.dsc_slice_num);
		of_property_read_u32(node, "dsc_slice_h",
				&dsim->lcd_info.dsc_slice_h);
		dsim_info("dsc slice height(%d)\n", dsim->lcd_info.dsc_slice_h);

		dsim->lcd_info.dsc_enc_sw = dsim_calc_slice_width(dsim->lcd_info.dsc_cnt,
					dsim->lcd_info.dsc_slice_num, dsim->lcd_info.xres);
		dsim->lcd_info.dsc_dec_sw = dsim->lcd_info.xres / dsim->lcd_info.dsc_slice_num;
		dsim_info("dsc enc_sw(%d), dec_sw(%d)\n",
				dsim->lcd_info.dsc_enc_sw, dsim->lcd_info.dsc_dec_sw);
	}

	of_property_read_u32(node, "partial_width", &dsim->lcd_info.partial_width[0]);
	of_property_read_u32(node, "partial_height", &dsim->lcd_info.partial_height[0]);

	dsim_info("partial w * h = %d * %d\n", dsim->lcd_info.partial_width[0], dsim->lcd_info.partial_height[0]);

	of_property_read_u32(node, "data_lane", &dsim->data_lane_cnt);
	dsim_info("using data lane count(%d)\n", dsim->data_lane_cnt);

	dsim->lcd_info.data_lane = dsim->data_lane_cnt;

	of_property_read_u32(node, "mres_en", &dsim->lcd_info.dt_lcd_mres.mres_en);
	dsim_info("mres_en(%d)\n", dsim->lcd_info.dt_lcd_mres.mres_en);
	dsim->lcd_info.mres_mode = 0; /* 0=WQHD, 1=FHD, 2=HD */
	dsim->lcd_info.dt_lcd_mres.mres_number = mres_num; /* default = 1 */

	if (dsim->lcd_info.dt_lcd_mres.mres_en) {
		of_property_read_u32(node, "mres_number", &mres_num);
		dsim->lcd_info.dt_lcd_mres.mres_number = mres_num;
		dsim_info("mres_number(%d)\n", mres_num);

		of_property_read_u32_array(node, "mres_width", mres_w, mres_num);
		of_property_read_u32_array(node, "mres_height", mres_h, mres_num);
		of_property_read_u32_array(node, "mres_dsc_width", mres_dsc_w, mres_num);
		of_property_read_u32_array(node, "mres_dsc_height", mres_dsc_h, mres_num);
		of_property_read_u32_array(node, "mres_dsc_en", mres_dsc_en, mres_num);
		of_property_read_u32_array(node, "mres_partial_width", mres_partial_w, mres_num);
		of_property_read_u32_array(node, "mres_partial_height", mres_partial_h, mres_num);

		switch (mres_num) {
		case 3:
			dsim->lcd_info.dt_lcd_mres.res_info[2].width = mres_w[2];
			dsim->lcd_info.dt_lcd_mres.res_info[2].height = mres_h[2];
			dsim->lcd_info.dt_lcd_mres.res_info[2].dsc_en = mres_dsc_en[2];
			dsim->lcd_info.dt_lcd_mres.res_info[2].dsc_width = mres_dsc_w[2];
			dsim->lcd_info.dt_lcd_mres.res_info[2].dsc_height = mres_dsc_h[2];
			dsim->lcd_info.dt_dsc_slice.dsc_enc_sw[2] =
				dsim_calc_slice_width(dsim->lcd_info.dsc_cnt,
						dsim->lcd_info.dsc_slice_num, mres_w[2]);
			dsim->lcd_info.dt_dsc_slice.dsc_dec_sw[2] =
				 mres_w[2] / dsim->lcd_info.dsc_slice_num;
			dsim_info("mres[2]: dsc enc_sw(%d), dec_sw(%d)\n",
					dsim->lcd_info.dt_dsc_slice.dsc_enc_sw[2],
					dsim->lcd_info.dt_dsc_slice.dsc_dec_sw[2]);
			dsim->lcd_info.partial_width[2] = mres_partial_w[2];
			dsim->lcd_info.partial_height[2] = mres_partial_h[2];
		case 2:
			dsim->lcd_info.dt_lcd_mres.res_info[1].width = mres_w[1];
			dsim->lcd_info.dt_lcd_mres.res_info[1].height = mres_h[1];
			dsim->lcd_info.dt_lcd_mres.res_info[1].dsc_en = mres_dsc_en[1];
			dsim->lcd_info.dt_lcd_mres.res_info[1].dsc_width = mres_dsc_w[1];
			dsim->lcd_info.dt_lcd_mres.res_info[1].dsc_height = mres_dsc_h[1];
			dsim->lcd_info.dt_dsc_slice.dsc_enc_sw[1] =
				dsim_calc_slice_width(dsim->lcd_info.dsc_cnt,
						dsim->lcd_info.dsc_slice_num, mres_w[1]);
			dsim->lcd_info.dt_dsc_slice.dsc_dec_sw[1] =
				 mres_w[1] / dsim->lcd_info.dsc_slice_num;
			dsim_info("mres[1]: dsc enc_sw(%d), dec_sw(%d)\n",
					dsim->lcd_info.dt_dsc_slice.dsc_enc_sw[1],
					dsim->lcd_info.dt_dsc_slice.dsc_dec_sw[1]);
			dsim->lcd_info.partial_width[1] = mres_partial_w[1];
			dsim->lcd_info.partial_height[1] = mres_partial_h[1];
		case 1:
			dsim->lcd_info.dt_lcd_mres.res_info[0].width = mres_w[0];
			dsim->lcd_info.dt_lcd_mres.res_info[0].height = mres_h[0];
			dsim->lcd_info.dt_lcd_mres.res_info[0].dsc_en = mres_dsc_en[0];
			dsim->lcd_info.dt_lcd_mres.res_info[0].dsc_width = mres_dsc_w[0];
			dsim->lcd_info.dt_lcd_mres.res_info[0].dsc_height = mres_dsc_h[0];
			dsim->lcd_info.dt_dsc_slice.dsc_enc_sw[0] =
				dsim_calc_slice_width(dsim->lcd_info.dsc_cnt,
						dsim->lcd_info.dsc_slice_num, mres_w[0]);
			dsim->lcd_info.dt_dsc_slice.dsc_dec_sw[0] =
				 mres_w[0] / dsim->lcd_info.dsc_slice_num;
			dsim_info("mres[0]: dsc enc_sw(%d), dec_sw(%d)\n",
					dsim->lcd_info.dt_dsc_slice.dsc_enc_sw[0],
					dsim->lcd_info.dt_dsc_slice.dsc_dec_sw[0]);
			dsim->lcd_info.partial_width[0] = mres_partial_w[0];
			dsim->lcd_info.partial_height[0] = mres_partial_h[0];
			break;
		default:
			dsim->lcd_info.dt_lcd_mres.res_info[0].width = dsim->lcd_info.xres;
			dsim->lcd_info.dt_lcd_mres.res_info[0].height = dsim->lcd_info.yres;
			dsim_warn("check multi-resolution configurations at DT\n");
			break;
		}
		dsim_info("[LCD multi(%d)-resolution info] 1st(%dx%d), 2nd(%dx%d), 3rd(%dx%d)\n",
				mres_num, mres_w[0], mres_h[0],
				mres_w[1], mres_h[1], mres_w[2], mres_h[2]);
	} else {
		dsim->lcd_info.dt_lcd_mres.res_info[0].width = dsim->lcd_info.width;
		dsim->lcd_info.dt_lcd_mres.res_info[0].height = dsim->lcd_info.height;
		dsim->lcd_info.dt_lcd_mres.res_info[0].dsc_en = dsim->lcd_info.dsc_enabled;
		if (dsim->lcd_info.dsc_enabled && dsim->lcd_info.dsc_slice_num != 0) {
			dsim->lcd_info.dt_lcd_mres.res_info[0].dsc_width =
				dsim->lcd_info.width / dsim->lcd_info.dsc_slice_num;
			dsim->lcd_info.dt_lcd_mres.res_info[0].dsc_height =
				dsim->lcd_info.dsc_slice_h;
		}
	}

	if (dsim->lcd_info.mode == DECON_MIPI_COMMAND_MODE) {
		of_property_read_u32_array(node, "cmd_underrun_lp_ref",
				dsim->lcd_info.cmd_underrun_lp_ref,
				dsim->lcd_info.dt_lcd_mres.mres_number);
		for (k = 0; k < dsim->lcd_info.dt_lcd_mres.mres_number; k++)
			dsim_info("mres[%d] cmd_underrun_lp_ref(%d)\n", k,
					dsim->lcd_info.cmd_underrun_lp_ref[k]);
	} else {
		of_property_read_u32(node, "vt_compensation",
				&dsim->lcd_info.vt_compensation);
		dsim_info("vt_compensation(%d)\n", dsim->lcd_info.vt_compensation);
	}

	/* HDR info */
	of_property_read_u32(node, "hdr_num", &hdr_num);
	dsim->lcd_info.dt_lcd_hdr.hdr_num = hdr_num;
	dsim_info("hdr_num(%d)\n", hdr_num);

	if (hdr_num != 0) {
		of_property_read_u32_array(node, "hdr_type", hdr_type, hdr_num);
		for (k = 0; k < hdr_num; k++) {
			dsim->lcd_info.dt_lcd_hdr.hdr_type[k] = hdr_type[k];
			dsim_info("hdr_type[%d] = %d\n", k, hdr_type[k]);
		}

		of_property_read_u32(node, "hdr_max_luma", &hdr_mxl);
		of_property_read_u32(node, "hdr_max_avg_luma", &hdr_mal);
		of_property_read_u32(node, "hdr_min_luma", &hdr_mnl);
		dsim->lcd_info.dt_lcd_hdr.hdr_max_luma = hdr_mxl;
		dsim->lcd_info.dt_lcd_hdr.hdr_max_avg_luma = hdr_mal;
		dsim->lcd_info.dt_lcd_hdr.hdr_min_luma = hdr_mnl;
		dsim_info("hdr_max_luma(%d), hdr_max_avg_luma(%d), hdr_min_luma(%d)\n",
				hdr_mxl, hdr_mal, hdr_mnl);
	}
}
#else
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
static int parse_adaptive_freq(struct device_node *node, struct decon_lcd *lcd_info)
{
	int ret = 0;
	int freq_cnt, i, k, cur_freq_idx;
	u32 res[MAX_PMSK_CNT];
	struct device_node *freq_node;
	struct adaptive_freq_info *freq_info;
	struct adaptive_info *adap_info = &lcd_info->adaptive_info;

	freq_cnt = of_property_count_u32_elems(node, "adaptive_freq_info");
	if (freq_cnt <= 0) {
		panel_warn("PANEL:WARN:%s:Can't found adaptive freq info\n", __func__);
		return -EINVAL;
	}

	dsim_info("[ADAP_FREQ] supporting freq count : %d\n", freq_cnt);
	if (freq_cnt > MAX_ADAPTABLE_FREQ) {
		dsim_info("[ADAP_FREQ] ERR:%s:freq cnt exceed max freq num (%d:%d)\n",
			__func__,freq_cnt, MAX_ADAPTABLE_FREQ);
		freq_cnt = MAX_ADAPTABLE_FREQ;
	}
	adap_info->freq_cnt = freq_cnt;

	for (i = 0; i < freq_cnt; i++) {
		freq_info = &adap_info->freq_info[i];
		freq_node = of_parse_phandle(node, "adaptive_freq_info", i);

		of_property_read_u32(freq_node, "hs-clk", &freq_info->hs_clk);
		of_property_read_u32(freq_node, "escape-clk", &freq_info->esc_clk);
		of_property_read_u32_array(freq_node, "cmd_underrun_lp_ref",
				freq_info->cmd_underrun_lp_ref,
				lcd_info->dt_lcd_mres.mres_number);
		of_property_read_u32_array(freq_node, "pmsk", res, MAX_PMSK_CNT);

		freq_info->dphy_pms.p = res[0];
		freq_info->dphy_pms.m = res[1];
		freq_info->dphy_pms.s = res[2];
		freq_info->dphy_pms.k = res[3];
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
		freq_info->dphy_pms.mfr = res[4];
		freq_info->dphy_pms.mrr = res[5];
		freq_info->dphy_pms.sel_pf = res[6];
		freq_info->dphy_pms.icp = res[7];
		freq_info->dphy_pms.afc_enb = res[8];
		freq_info->dphy_pms.extafc = res[9];
		freq_info->dphy_pms.feed_en = res[10];
		freq_info->dphy_pms.fsel = res[11];
		freq_info->dphy_pms.fout_mask = res[12];
		freq_info->dphy_pms.rsel = res[13];
#endif

		dsim_info("[ADAP_FREQ] Freq: %d\n", freq_info->hs_clk);
		dsim_info("[ADAP_FREQ] Escape Clk: %d\n", freq_info->esc_clk);
		for (k = 0; k < lcd_info->dt_lcd_mres.mres_number; k++)
			dsim_info("[ADAP_FREQ] mres[%d] cmd_underrun_lp_ref: %d\n",
					k, freq_info->cmd_underrun_lp_ref[k]);
		dsim_info("[ADAP_FREQ] PMS[p] : %d\n", freq_info->dphy_pms.p);
		dsim_info("[ADAP_FREQ] PMS[m] : %d\n", freq_info->dphy_pms.m);
		dsim_info("[ADAP_FREQ] PMS[s] : %d\n", freq_info->dphy_pms.s);
		dsim_info("[ADAP_FREQ] PMS[k] : %d\n", freq_info->dphy_pms.k);
	}

	if (!lcd_info->adaptive_info.adap_idx)
		dsim_err("[ADAP_FREQ] ERR:%s:adap_idx is null\n", __func__);
	else
		cur_freq_idx = lcd_info->adaptive_info.adap_idx->cur_freq_idx;

	if (cur_freq_idx < freq_cnt) {
		freq_info = &adap_info->freq_info[cur_freq_idx];
		lcd_info->hs_clk = freq_info->hs_clk;
		lcd_info->esc_clk = freq_info->esc_clk;
		memcpy(&lcd_info->dphy_pms,
				&freq_info->dphy_pms, MAX_PMSK_CNT);
		memcpy(lcd_info->cmd_underrun_lp_ref,
				freq_info->cmd_underrun_lp_ref,
				(u32)ARRAY_SIZE(lcd_info->cmd_underrun_lp_ref));
	}

	return ret;
}
#endif
void parse_lcd_info(struct device_node *node, struct decon_lcd *lcd_info)
{
	u32 res[14];
	unsigned int mres_num = 1;
	u32 mres_w[3] = {0, };
	u32 mres_h[3] = {0, };
	u32 mres_dsc_w[3] = {0, };
	u32 mres_dsc_h[3] = {0, };
	u32 mres_dsc_en[3] = {0, };
	u32 mres_partial_w[3] = {0, };
	u32 mres_partial_h[3] = {0, };
	u32 hdr_num = 0;
	u32 hdr_type[HDR_CAPA_NUM] = {0, };
	u32 hdr_mxl = 0;
	u32 hdr_mal = 0;
	u32 hdr_mnl = 0;
	int k;

	of_property_read_u32(node, "mode", &lcd_info->mode);
	dsim_info("%s mode\n", lcd_info->mode ? "command" : "video");

	of_property_read_u32_array(node, "resolution", res, 2);
	lcd_info->xres = res[0];
	lcd_info->yres = res[1];
	dsim_info("LCD(%s) resolution: xres(%d), yres(%d)\n",
			of_node_full_name(node), res[0], res[1]);

	of_property_read_u32_array(node, "size", res, 2);
	lcd_info->width = res[0];
	lcd_info->height = res[1];
	dsim_info("LCD size: width(%d), height(%d)\n", res[0], res[1]);

	of_property_read_u32(node, "timing,refresh", &lcd_info->fps);
	dsim_dbg("LCD refresh rate(%d)\n", lcd_info->fps);

	of_property_read_u32_array(node, "timing,h-porch", res, 3);
	lcd_info->hbp = res[0];
	lcd_info->hfp = res[1];
	lcd_info->hsa = res[2];
	dsim_dbg("hbp(%d), hfp(%d), hsa(%d)\n", res[0], res[1], res[2]);

	of_property_read_u32_array(node, "timing,v-porch", res, 3);
	lcd_info->vbp = res[0];
	lcd_info->vfp = res[1];
	lcd_info->vsa = res[2];
	dsim_dbg("vbp(%d), vfp(%d), vsa(%d)\n", res[0], res[1], res[2]);

	of_property_read_u32(node, "timing,dsi-hs-clk", &lcd_info->hs_clk);
	dsim_dbg("requested hs clock(%d)\n", lcd_info->hs_clk);

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	of_property_read_u32_array(node, "timing,pmsk", res, 14);
#else
	of_property_read_u32_array(node, "timing,pmsk", res, 4);
#endif
	lcd_info->dphy_pms.p = res[0];
	lcd_info->dphy_pms.m = res[1];
	lcd_info->dphy_pms.s = res[2];
	lcd_info->dphy_pms.k = res[3];
	dsim_dbg("p(%d), m(%d), s(%d), k(%d)\n", res[0], res[1], res[2], res[3]);
#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	lcd_info->dphy_pms.mfr = res[4];
	lcd_info->dphy_pms.mrr = res[5];
	lcd_info->dphy_pms.sel_pf = res[6];
	lcd_info->dphy_pms.icp = res[7];
	lcd_info->dphy_pms.afc_enb = res[8];
	lcd_info->dphy_pms.extafc = res[9];
	lcd_info->dphy_pms.feed_en = res[10];
	lcd_info->dphy_pms.fsel = res[11];
	lcd_info->dphy_pms.fout_mask = res[12];
	lcd_info->dphy_pms.rsel = res[13];
	dsim_info(" mfr(%d), mrr(0x%x), sel_pf(%d), icp(%d)\n",
			res[4], res[5], res[6], res[7]);
	dsim_info(" afc_enb(%d), extafc(%d), feed_en(%d), fsel(%d)\n",
			res[8], res[9], res[10], res[11]);
	dsim_info(" fout_mask(%d), rsel(%d)\n", res[12], res[13]);
#endif

	of_property_read_u32(node, "timing,dsi-escape-clk",
			&lcd_info->esc_clk);
	dsim_dbg("requested escape clock(%d)\n", lcd_info->esc_clk);

	of_property_read_u32(node, "mic_en", &lcd_info->mic_enabled);
	dsim_info("mic enabled (%d)\n", lcd_info->mic_enabled);

	of_property_read_u32(node, "type_of_ddi", &lcd_info->ddi_type);
	dsim_dbg("ddi type(%d)\n", lcd_info->ddi_type);

	of_property_read_u32(node, "dsc_en", &lcd_info->dsc_enabled);
	dsim_info("dsc is %s\n", lcd_info->dsc_enabled ? "enabled" : "disabled");

	if (lcd_info->dsc_enabled) {
		of_property_read_u32(node, "dsc_cnt", &lcd_info->dsc_cnt);
		dsim_info("dsc count(%d)\n", lcd_info->dsc_cnt);
		of_property_read_u32(node, "dsc_slice_num",
				&lcd_info->dsc_slice_num);
		dsim_info("dsc slice count(%d)\n", lcd_info->dsc_slice_num);
		of_property_read_u32(node, "dsc_slice_h",
				&lcd_info->dsc_slice_h);
		dsim_info("dsc slice height(%d)\n", lcd_info->dsc_slice_h);

		lcd_info->dsc_enc_sw = dsim_calc_slice_width(lcd_info->dsc_cnt,
					lcd_info->dsc_slice_num, lcd_info->xres);
		lcd_info->dsc_dec_sw = lcd_info->xres / lcd_info->dsc_slice_num;
		dsim_info("dsc enc_sw(%d), dec_sw(%d)\n",
				lcd_info->dsc_enc_sw, lcd_info->dsc_dec_sw);
	}

	of_property_read_u32(node, "partial_width", &lcd_info->partial_width[0]);
	of_property_read_u32(node, "partial_height", &lcd_info->partial_height[0]);

	dsim_info("partial w * h = %d * %d\n", lcd_info->partial_width[0], lcd_info->partial_height[0]);

	of_property_read_u32(node, "data_lane", &lcd_info->data_lane);
	dsim_info("using data lane count(%d)\n", lcd_info->data_lane);

	of_property_read_u32(node, "mres_en", &lcd_info->dt_lcd_mres.mres_en);
	dsim_info("mres_en(%d)\n", lcd_info->dt_lcd_mres.mres_en);
	lcd_info->mres_mode = DSU_MODE_1; /* 0=WQHD, 1=FHD, 2=HD */
	lcd_info->dt_lcd_mres.mres_number = mres_num; /* default = 1 */

	if (lcd_info->dt_lcd_mres.mres_en) {
		of_property_read_u32(node, "mres_number", &mres_num);
		lcd_info->dt_lcd_mres.mres_number = mres_num;
		dsim_info("mres_number(%d)\n", mres_num);

		of_property_read_u32_array(node, "mres_width", mres_w, mres_num);
		of_property_read_u32_array(node, "mres_height", mres_h, mres_num);
		of_property_read_u32_array(node, "mres_dsc_width", mres_dsc_w, mres_num);
		of_property_read_u32_array(node, "mres_dsc_height", mres_dsc_h, mres_num);
		of_property_read_u32_array(node, "mres_dsc_en", mres_dsc_en, mres_num);
		of_property_read_u32_array(node, "mres_partial_width", mres_partial_w, mres_num);
		of_property_read_u32_array(node, "mres_partial_height", mres_partial_h, mres_num);

		switch (mres_num) {
		case 3:
			lcd_info->dt_lcd_mres.res_info[2].width = mres_w[2];
			lcd_info->dt_lcd_mres.res_info[2].height = mres_h[2];
			lcd_info->dt_lcd_mres.res_info[2].dsc_en = mres_dsc_en[2];
			lcd_info->dt_lcd_mres.res_info[2].dsc_width = mres_dsc_w[2];
			lcd_info->dt_lcd_mres.res_info[2].dsc_height = mres_dsc_h[2];
			lcd_info->dt_dsc_slice.dsc_enc_sw[2] =
				dsim_calc_slice_width(lcd_info->dsc_cnt,
						lcd_info->dsc_slice_num, mres_w[2]);
			lcd_info->dt_dsc_slice.dsc_dec_sw[2] =
				 mres_w[2] / lcd_info->dsc_slice_num;
			lcd_info->partial_width[2] = mres_partial_w[2];
			lcd_info->partial_height[2] = mres_partial_h[2];
		case 2:
			lcd_info->dt_lcd_mres.res_info[1].width = mres_w[1];
			lcd_info->dt_lcd_mres.res_info[1].height = mres_h[1];
			lcd_info->dt_lcd_mres.res_info[1].dsc_en = mres_dsc_en[1];
			lcd_info->dt_lcd_mres.res_info[1].dsc_width = mres_dsc_w[1];
			lcd_info->dt_lcd_mres.res_info[1].dsc_height = mres_dsc_h[1];
			lcd_info->dt_dsc_slice.dsc_enc_sw[1] =
				dsim_calc_slice_width(lcd_info->dsc_cnt,
						lcd_info->dsc_slice_num, mres_w[1]);
			lcd_info->dt_dsc_slice.dsc_dec_sw[1] =
				 mres_w[1] / lcd_info->dsc_slice_num;
			lcd_info->partial_width[1] = mres_partial_w[1];
			lcd_info->partial_height[1] = mres_partial_h[1];
		case 1:
			lcd_info->dt_lcd_mres.res_info[0].width = mres_w[0];
			lcd_info->dt_lcd_mres.res_info[0].height = mres_h[0];
			lcd_info->dt_lcd_mres.res_info[0].dsc_en = mres_dsc_en[0];
			lcd_info->dt_lcd_mres.res_info[0].dsc_width = mres_dsc_w[0];
			lcd_info->dt_lcd_mres.res_info[0].dsc_height = mres_dsc_h[0];
			lcd_info->dt_dsc_slice.dsc_enc_sw[0] =
				dsim_calc_slice_width(lcd_info->dsc_cnt,
						lcd_info->dsc_slice_num, mres_w[0]);
			lcd_info->dt_dsc_slice.dsc_dec_sw[0] =
				 mres_w[0] / lcd_info->dsc_slice_num;
			lcd_info->partial_width[0] = mres_partial_w[0];
			lcd_info->partial_height[0] = mres_partial_h[0];
			break;
		default:
			lcd_info->dt_lcd_mres.res_info[0].width = lcd_info->width;
			lcd_info->dt_lcd_mres.res_info[0].height = lcd_info->height;
			dsim_warn("check multi-resolution configurations at DT\n");
			break;
		}
		dsim_info("[LCD multi(%d)-resolution info] 1st(%dx%d), 2nd(%dx%d), 3rd(%dx%d)\n",
				mres_num, mres_w[0], mres_h[0],
				mres_w[1], mres_h[1], mres_w[2], mres_h[2]);
	} else {
		lcd_info->dt_lcd_mres.res_info[0].width = lcd_info->xres;
		lcd_info->dt_lcd_mres.res_info[0].height = lcd_info->yres;
		lcd_info->dt_lcd_mres.res_info[0].dsc_en = lcd_info->dsc_enabled;
		if (lcd_info->dsc_enabled && lcd_info->dsc_slice_num != 0) {
			lcd_info->dt_lcd_mres.res_info[0].dsc_width = lcd_info->xres / lcd_info->dsc_slice_num;
			lcd_info->dt_lcd_mres.res_info[0].dsc_height = lcd_info->dsc_slice_h;
		}
	}

	if (lcd_info->mode == DECON_MIPI_COMMAND_MODE) {
		of_property_read_u32_array(node, "cmd_underrun_lp_ref",
				lcd_info->cmd_underrun_lp_ref,
				lcd_info->dt_lcd_mres.mres_number);
		for (k = 0; k < lcd_info->dt_lcd_mres.mres_number; k++)
			dsim_info("mres[%d] cmd_underrun_lp_ref(%d)\n", k,
					lcd_info->cmd_underrun_lp_ref[k]);
	} else {
		of_property_read_u32(node, "vt_compensation",
				&lcd_info->vt_compensation);
		dsim_info("vt_compensation(%d)\n", lcd_info->vt_compensation);
	}

	/* HDR info */
	of_property_read_u32(node, "hdr_num", &hdr_num);
	lcd_info->dt_lcd_hdr.hdr_num = hdr_num;
	dsim_info("hdr_num(%d)\n", hdr_num);

	if (hdr_num != 0) {
		of_property_read_u32_array(node, "hdr_type", hdr_type, hdr_num);
		for (k = 0; k < hdr_num; k++) {
			lcd_info->dt_lcd_hdr.hdr_type[k] = hdr_type[k];
			dsim_info("hdr_type[%d] = %d\n", k, hdr_type[k]);
		}

		of_property_read_u32(node, "hdr_max_luma", &hdr_mxl);
		of_property_read_u32(node, "hdr_max_avg_luma", &hdr_mal);
		of_property_read_u32(node, "hdr_min_luma", &hdr_mnl);
		lcd_info->dt_lcd_hdr.hdr_max_luma = hdr_mxl;
		lcd_info->dt_lcd_hdr.hdr_max_avg_luma = hdr_mal;
		lcd_info->dt_lcd_hdr.hdr_min_luma = hdr_mnl;
		dsim_info("hdr_max_luma(%d), hdr_max_avg_luma(%d), hdr_min_luma(%d)\n",
				hdr_mxl, hdr_mal, hdr_mnl);
	}

	of_property_read_u32(node, "color_mode_num", &lcd_info->color_mode_cnt);
	dsim_info("supporting color mode : %d\n", lcd_info->color_mode_cnt);
	if (lcd_info->color_mode_cnt != 0) {
		of_property_read_u32_array(node, "color_mode",
			lcd_info->color_mode, lcd_info->color_mode_cnt);
	}

	for(k = 0; k < lcd_info->color_mode_cnt; k++)
		dsim_info("color mode[%d] : %d\n", k, lcd_info->color_mode[k]);

#if defined(CONFIG_EXYNOS_DSIM_DITHER)
	dsim_info("DSIM MIPI SSCG Enabled\n");
#else
	dsim_info("DSIM MIPI SSCG Disabled\n");
#endif

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	parse_adaptive_freq(node, lcd_info);
#endif
}
#endif
static int dsim_parse_dt(struct dsim_device *dsim, struct device *dev)
{
	if (IS_ERR_OR_NULL(dev->of_node)) {
		dsim_err("no device tree information\n");
		return -EINVAL;
	}

	dsim->id = of_alias_get_id(dev->of_node, "dsim");
	dsim_info("dsim(%d) probe start..\n", dsim->id);

	if(of_property_read_u32(dev->of_node, "board_info",
			&dsim->board_info))
		dsim->board_info = 0;
	dsim_info("board info... (%d)\n", dsim->board_info);

	dsim->phy = devm_phy_get(dev, "dsim_dphy");
	if (IS_ERR_OR_NULL(dsim->phy)) {
		dsim_err("failed to get phy\n");
		return PTR_ERR(dsim->phy);
	}

	dsim->phy_ex = devm_phy_get(dev, "dsim_dphy_extra");
	if (IS_ERR_OR_NULL(dsim->phy_ex)) {
		dsim_err("failed to get extra phy. It's not mandatary.\n");
		dsim->phy_ex = NULL;
	}

	dsim->dev = dev;

#if !defined(CONFIG_EXYNOS_COMMON_PANEL)
	dsim_get_gpios(dsim);
	dsim_get_regulator(dsim);
	dsim_parse_lcd_info(dsim);
#endif

	return 0;
}

static void dsim_register_panel(struct dsim_device *dsim)
{
#if IS_ENABLED(CONFIG_EXYNOS_COMMON_PANEL)
	dsim->panel_ops = &common_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_S6E3HA2K)
	dsim->panel_ops = &s6e3ha2k_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_S6E3HF4)
	dsim->panel_ops = &s6e3hf4_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_S6E3HA6)
	dsim->panel_ops = &s6e3ha6_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_S6E3HA8)
	dsim->panel_ops = &s6e3ha8_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_S6E3HA9)
	dsim->panel_ops = &s6e3ha9_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_S6E3AA2)
	dsim->panel_ops = &s6e3aa2_mipi_lcd_driver;
#elif IS_ENABLED(CONFIG_EXYNOS_DECON_LCD_EMUL_DISP)
	dsim->panel_ops = &emul_disp_mipi_lcd_driver;
#else
	dsim->panel_ops = &s6e3ha2k_mipi_lcd_driver;
#endif
}

static int dsim_get_data_lanes(struct dsim_device *dsim)
{
	int i;

	if (dsim->data_lane_cnt > MAX_DSIM_DATALANE_CNT) {
		dsim_err("%d data lane couldn't be supported\n",
				dsim->data_lane_cnt);
		return -EINVAL;
	}

	dsim->data_lane = DSIM_LANE_CLOCK;
	for (i = 1; i < dsim->data_lane_cnt + 1; ++i)
		dsim->data_lane |= 1 << i;

	dsim_info("%s: lanes(0x%x)\n", __func__, dsim->data_lane);

	return 0;
}

static int dsim_init_resources(struct dsim_device *dsim, struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dsim_err("failed to get mem resource\n");
		return -ENOENT;
	}
	dsim_info("res: start(0x%x), end(0x%x)\n", (u32)res->start, (u32)res->end);

	dsim->res.regs = devm_ioremap_resource(dsim->dev, res);
	if (!dsim->res.regs) {
		dsim_err("failed to remap DSIM SFR region\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dsim_info("no 2nd mem resource\n");
		dsim->res.phy_regs = NULL;
	} else {
		dsim_info("dphy res: start(0x%x), end(0x%x)\n",
				(u32)res->start, (u32)res->end);

		dsim->res.phy_regs = devm_ioremap_resource(dsim->dev, res);
		if (!dsim->res.phy_regs) {
			dsim_err("failed to remap DSIM DPHY SFR region\n");
			return -EINVAL;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res) {
		dsim_info("no extra dphy resource\n");
		dsim->res.phy_regs_ex = NULL;
	} else {
		dsim_info("dphy_extra res: start(0x%x), end(0x%x)\n",
				(u32)res->start, (u32)res->end);

		dsim->res.phy_regs_ex = devm_ioremap_resource(dsim->dev, res);
		if (!dsim->res.phy_regs_ex) {
			dsim_err("failed to remap DSIM DPHY(EXTRA) SFR region\n");
			return -EINVAL;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dsim_err("failed to get irq resource\n");
		return -ENOENT;
	}

	dsim->res.irq = res->start;
	ret = devm_request_irq(dsim->dev, res->start,
			dsim_irq_handler, 0, pdev->name, dsim);
	if (ret) {
		dsim_err("failed to install DSIM irq\n");
		return -EINVAL;
	}
	disable_irq(dsim->res.irq);

	dsim->res.ss_regs = dpu_get_sysreg_addr();
	if (IS_ERR_OR_NULL(dsim->res.ss_regs)) {
		dsim_err("failed to get sysreg addr\n");
		return -EINVAL;
	}

	return 0;
}

static int dsim_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct dsim_device *dsim = NULL;
	char name[32];

	dsim = devm_kzalloc(dev, sizeof(struct dsim_device), GFP_KERNEL);
	if (!dsim) {
		dsim_err("failed to allocate dsim device.\n");
		ret = -ENOMEM;
		goto err;
	}
#if !defined(CONFIG_SUPPORT_LEGACY_ION)
	dma_set_mask(dev, DMA_BIT_MASK(36));
#endif
	ret = dsim_parse_dt(dsim, dev);
	if (ret)
		goto err_dt;

	dsim_drvdata[dsim->id] = dsim;
	ret = dsim_get_clocks(dsim);
	if (ret)
		goto err_dt;

	spin_lock_init(&dsim->slock);
	mutex_init(&dsim->cmd_lock);
#if defined(CONFIG_EXYNOS_ADAPTIVE_FREQ)
	mutex_init(&dsim->adap_freq.lock);
	init_waitqueue_head(&dsim->adap_freq.wait);
	dsim->adap_freq.active = true;
#endif

	init_completion(&dsim->ph_wr_comp);
	init_completion(&dsim->rd_comp);

	ret = dsim_init_resources(dsim, pdev);
	if (ret)
		goto err_dt;

	dsim_init_subdev(dsim);
	platform_set_drvdata(pdev, dsim);
	dsim_register_panel(dsim);
#if defined(CONFIG_EXYNOS_COMMON_PANEL)
	call_panel_ops(dsim, init, dsim);
#endif

	snprintf(name, 32, "dsim%d-wq", dsim->id);
	INIT_WORK(&dsim->wr_timeout_work, dsim_write_timeout_fn);
	dsim->wq = create_workqueue(name);

	setup_timer(&dsim->cmd_timer, dsim_cmd_fail_detector,
			(unsigned long)dsim);

#if defined(CONFIG_CPU_IDLE)
	dsim->idle_ip_index = exynos_get_idle_ip_index(dev_name(&pdev->dev));
	dsim_info("dsim idle_ip_index[%d]\n", dsim->idle_ip_index);
	if (dsim->idle_ip_index < 0)
		dsim_warn("idle ip index is not provided for dsim\n");
	exynos_update_ip_idle_status(dsim->idle_ip_index, 1);
#endif

	pm_runtime_enable(dev);

	ret = iovmm_activate(dev);
	if (ret) {
		dsim_err("failed to activate iovmm\n");
		goto err_dt;
	}
	iovmm_set_fault_handler(dev, dpu_sysmmu_fault_handler, NULL);

	ret = dsim_get_data_lanes(dsim);
	if (ret)
		goto err_dt;

	phy_init(dsim->phy);
	if (dsim->phy_ex)
		phy_init(dsim->phy_ex);

	dsim->state = DSIM_STATE_INIT;
	dsim_enable(dsim);

	/* TODO: If you want to enable DSIM BIST mode. you must turn on LCD here */

#if !defined(BRINGUP_DSIM_BIST)
	call_panel_ops(dsim, probe, dsim);
#if defined(CONFIG_EXYNOS_COMMON_PANEL)
	call_panel_ops(dsim, sleepout, dsim);
#endif
#else
	/* TODO: This is for dsim BIST mode in zebu emulator. only for test*/
	call_panel_ops(dsim, displayon, dsim);
	dsim_reg_set_bist(dsim->id, true);
#endif

#ifdef CONFIG_DYNAMIC_FREQ
	call_panel_ops(dsim, update_lcd_info, dsim);
#endif
	/* for debug */
	/* dsim_dump(dsim); */

	dsim_clocks_info(dsim);
	dsim_create_cmd_rw_sysfs(dsim);

#ifdef DPHY_LOOP
	dsim_reg_set_dphy_loop_back_test(dsim->id);
#endif

#if defined(CONFIG_EXYNOS_ADAPTIVE_FREQ)
	ret = dsim_create_adap_freq_thread(dsim);
	if (ret)
		goto err_dt;

	dsim->ril_notif.notifier_call = dsim_ril_notifier;
	register_dev_ril_bridge_event_notifier(&dsim->ril_notif);
#endif

	dsim_info("dsim%d driver(%s mode) has been probed.\n", dsim->id,
		dsim->lcd_info.mode == DECON_MIPI_COMMAND_MODE ? "cmd" : "video");
	return 0;

err_dt:
	kfree(dsim);
err:
	return ret;
}

static int dsim_remove(struct platform_device *pdev)
{
	struct dsim_device *dsim = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	mutex_destroy(&dsim->cmd_lock);
	dsim_info("dsim%d driver removed\n", dsim->id);

	return 0;
}

static void dsim_shutdown(struct platform_device *pdev)
{
#if 0
	struct dsim_device *dsim = platform_get_drvdata(pdev);

	DPU_EVENT_LOG(DPU_EVT_DSIM_SHUTDOWN, &dsim->sd, ktime_set(0, 0));
	dsim_info("%s + state:%d\n", __func__, dsim->state);

	dsim_disable(dsim);

	dsim_info("%s -\n", __func__);
#else
	dsim_info("%s +-\n", __func__);
#endif
}

static int dsim_runtime_suspend(struct device *dev)
{
	struct dsim_device *dsim = dev_get_drvdata(dev);

	DPU_EVENT_LOG(DPU_EVT_DSIM_SUSPEND, &dsim->sd, ktime_set(0, 0));
	dsim_dbg("%s +\n", __func__);
	clk_disable_unprepare(dsim->res.aclk);
	dsim_dbg("%s -\n", __func__);
	return 0;
}

static int dsim_runtime_resume(struct device *dev)
{
	struct dsim_device *dsim = dev_get_drvdata(dev);

	DPU_EVENT_LOG(DPU_EVT_DSIM_RESUME, &dsim->sd, ktime_set(0, 0));
	dsim_dbg("%s: +\n", __func__);
	clk_prepare_enable(dsim->res.aclk);
	dsim_dbg("%s -\n", __func__);
	return 0;
}

static const struct of_device_id dsim_of_match[] = {
	{ .compatible = "samsung,exynos9-dsim" },
	{},
};
MODULE_DEVICE_TABLE(of, dsim_of_match);

static const struct dev_pm_ops dsim_pm_ops = {
	.runtime_suspend	= dsim_runtime_suspend,
	.runtime_resume		= dsim_runtime_resume,
};

static struct platform_driver dsim_driver __refdata = {
	.probe			= dsim_probe,
	.remove			= dsim_remove,
	.shutdown		= dsim_shutdown,
	.driver = {
		.name		= DSIM_MODULE_NAME,
		.owner		= THIS_MODULE,
		.pm		= &dsim_pm_ops,
		.of_match_table	= of_match_ptr(dsim_of_match),
		.suppress_bind_attrs = true,
	}
};

static int __init dsim_init(void)
{
	int ret = platform_driver_register(&dsim_driver);
	if (ret)
		pr_err("dsim driver register failed\n");

	return ret;
}
late_initcall(dsim_init);

static void __exit dsim_exit(void)
{
	platform_driver_unregister(&dsim_driver);
}

module_exit(dsim_exit);
MODULE_AUTHOR("Yeongran Shin <yr613.shin@samsung.com>");
MODULE_DESCRIPTION("Samusung EXYNOS DSIM driver");
MODULE_LICENSE("GPL");
