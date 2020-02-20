/* drivers/video/exynos/decon/panels/common_mipi_lcd.c
 *
 * Samsung SoC MIPI LCD driver.
 *
 * Copyright (c) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <video/mipi_display.h>
#include <linux/platform_device.h>

#include "../disp_err.h"
#include "../decon.h"
#include "../dsim.h"
#include "../../panel/panel.h"
#include "../../panel/panel_drv.h"
#include "lcd_ctrl.h"
#include "decon_lcd.h"

static struct v4l2_subdev *panel_sd;
static DEFINE_MUTEX(cmd_lock);
struct panel_state *panel_state;

static int dsim_ioctl_panel(struct dsim_device *dsim, u32 cmd, void *arg)
{
	int ret;

	if (unlikely(!panel_sd))
		return -EINVAL;

	ret = v4l2_subdev_call(panel_sd, core, ioctl, cmd, arg);

	return ret;
}

static int __match_panel_v4l2_subdev(struct device *dev, void *data)
{
	struct panel_device *panel;

	dsim_info("DSIM:INFO:matched panel drv name : %s\n", dev->driver->name);

	panel = (struct panel_device *)dev_get_drvdata(dev);
	if (panel == NULL) {
		dsim_err("DSIM:ERR:%s:failed to get panel's v4l2 sub dev\n", __func__);
	}
	panel_sd = &panel->sd;

	return 0;
}

static int dsim_notify_panel(struct v4l2_subdev *sd,
		unsigned int notification, void *arg)
{
	struct v4l2_event *ev = (struct v4l2_event *)arg;
	int ret;

	if (notification != V4L2_DEVICE_NOTIFY_EVENT) {
		decon_dbg("%s unknown event\n", __func__);
		return -EINVAL;
	}

	switch (ev->type) {
	case V4L2_EVENT_DECON_FRAME_DONE:
		ret = v4l2_subdev_call(sd, core, ioctl,
				PANEL_IOC_EVT_FRAME_DONE, &ev->timestamp);
		if (ret) {
			pr_err("%s failed to notify FRAME_DONE\n", __func__);
			return ret;
		}
		break;
	case V4L2_EVENT_DECON_VSYNC:
		ret = v4l2_subdev_call(sd, core, ioctl,
				PANEL_IOC_EVT_VSYNC, &ev->timestamp);
		if (ret) {
			pr_err("%s failed to notify VSYNC\n", __func__);
			return ret;
		}
		break;
	default:
		pr_warn("%s unknown event type %d\n", __func__, ev->type);
		break;
	}

	pr_debug("%s event type %d timestamp %ld %ld nsec\n",
			__func__, ev->type, ev->timestamp.tv_sec,
			ev->timestamp.tv_nsec);

	return 0;
}

static inline int dsim_check_cb(struct dsim_device *dsim)
{
	struct disp_check_cb_info *info = &dsim->check_cb_info;

	return info->check_cb(info->data);
}

int dsim_panel_error_cb(struct dsim_device *dsim,
		struct disp_check_cb_info *info)
{
	struct disp_error_cb_info *error_cb_info = &dsim->error_cb_info;
	struct disp_check_cb_info dsim_check_info = {
		.check_cb = (disp_check_cb *)dsim_check_cb,
		.data = dsim,
	};
	int ret = 0;

	memcpy(&dsim->check_cb_info, info,
			sizeof(struct disp_check_cb_info));
	if (error_cb_info->error_cb) {
		ret = error_cb_info->error_cb(error_cb_info->data,
				&dsim_check_info);
		if (ret)
			panel_err("PANEL:ERR:%s:failed to recovery dsim\n", __func__);
	}

	return ret;
}

static int common_lcd_set_error_cb(struct dsim_device *dsim)
{
	struct disp_error_cb_info dsim_panel_error_info = {
		.error_cb = (disp_error_cb *)dsim_panel_error_cb,
		.data = dsim,
	};
	int ret;

	v4l2_set_subdev_hostdata(panel_sd, &dsim_panel_error_info);
	ret = dsim_ioctl_panel(dsim, PANEL_IOC_REG_RESET_CB, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to set panel error callback\n", __func__);
		return ret;
	}

	return 0;
}

static int dsim_panel_probe(struct dsim_device *dsim)
{
	int ret;
	struct decon_lcd *lcd_info;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DSIM_PROBE, (void *)&dsim->id);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel dsim probe\n", __func__);
		return ret;
	}

	lcd_info = (struct decon_lcd *)v4l2_get_subdev_hostdata(panel_sd);
	if (IS_ERR_OR_NULL(lcd_info)) {
		dsim_err("DSIM:ERR:%s:failed to get lcd information\n", __func__);
		return -EINVAL;
	}
	memcpy(&dsim->lcd_info, lcd_info, sizeof(struct decon_lcd));

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	dsim_update_adaptive_freq(dsim, true);
#else
	dsim->clks.hs_clk = dsim->lcd_info.hs_clk;
	dsim->clks.esc_clk = dsim->lcd_info.esc_clk;
#endif
	dsim->data_lane_cnt = dsim->lcd_info.data_lane;

	return ret;
}


#ifdef CONFIG_DYNAMIC_FREQ
static int common_update_lcd_info(struct dsim_device *dsim)
{
	int ret;
	struct decon_lcd *lcd_info;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DSIM_PROBE, (void *)&dsim->id);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel dsim probe\n", __func__);
		return ret;
	}

	lcd_info = (struct decon_lcd *)v4l2_get_subdev_hostdata(panel_sd);
	if (IS_ERR_OR_NULL(lcd_info)) {
		dsim_err("DSIM:ERR:%s:failed to get lcd information\n", __func__);
		return -EINVAL;
	}
	memcpy(&dsim->lcd_info, lcd_info, sizeof(struct decon_lcd));

	return ret;
}
#endif


static int dsim_panel_get_state(struct dsim_device *dsim)
{
	int ret = 0;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_GET_PANEL_STATE, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get panel state", __func__);
		return ret;
	}

	panel_state = (struct panel_state *)v4l2_get_subdev_hostdata(panel_sd);
	if (IS_ERR_OR_NULL(panel_state)) {
		dsim_err("DSIM:ERR:%s:failed to get lcd information\n", __func__);
		return -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_DYNAMIC_FREQ
static int dsim_panel_get_df_status(struct dsim_device *dsim)
{
	int ret = 0;
	struct df_status_info *df_status;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_GET_DF_STATUS, NULL);
	if (ret < 0) {
		dsim_err("DSIM:ERR:%s:failed to get df status\n", __func__);
		goto err_get_df;
	}
	df_status = (struct df_status_info*)v4l2_get_subdev_hostdata(panel_sd);
	if (df_status != NULL)
		dsim->df_status = df_status;

	dsim_info("[DYN_FREQ]:INFO:%s:req,tar,cur:%d,%d:%d\n", __func__,
		dsim->df_status->request_df, dsim->df_status->target_df,
		dsim->df_status->current_df);
err_get_df:
	return ret;
}
#endif

int mipi_write(u32 id, u8 cmd_id, const u8 *cmd, u8 offset, int size, u32 option, bool wakeup)
{
	int ret, retry = 3;
	unsigned long d0;
	u32 type, d1;
	bool block = (option & DSIM_OPTION_WAIT_TX_DONE);
	struct dsim_device *dsim = get_dsim_drvdata(id);

	if (!cmd) {
		pr_err("%s cmd is null\n", __func__);
		return -EINVAL;
	}

	if (cmd_id == MIPI_DSI_WR_DSC_CMD) {
		type = MIPI_DSI_DSC_PRA;
		d0 = (unsigned long)cmd[0];
		d1 = 0;
	} else if (cmd_id == MIPI_DSI_WR_PPS_CMD) {
		type = MIPI_DSI_DSC_PPS;
		d0 = (unsigned long)cmd;
		d1 = size;
	} else if ((cmd_id == MIPI_DSI_WR_GEN_CMD) ||
		(cmd_id == MIPI_DSI_WR_CMD_NO_WAKE)) {
		if (size == 1) {
			type = MIPI_DSI_DCS_SHORT_WRITE;
			d0 = (unsigned long)cmd[0];
			d1 = 0;
		} else {
			type = MIPI_DSI_DCS_LONG_WRITE;
			d0 = (unsigned long)cmd;
			d1 = size;
		}
	} else {
		pr_info("%s invalid cmd_id %d\n", __func__, cmd_id);
		return -EINVAL;
	}

	mutex_lock(&cmd_lock);
	while (--retry >= 0) {
		if (offset > 0) {
			if (option & DSIM_OPTION_POINT_GPARA) {
				u8 gpara[3] = { 0xB0, offset, cmd[0] };
				if (dsim_write_data(dsim, MIPI_DSI_DCS_LONG_WRITE,
							(unsigned long)gpara, ARRAY_SIZE(gpara), false, wakeup)) {
					pr_err("%s failed to write gpara %d (retry %d)\n",
							__func__, offset, retry);
					dsim_function_reset(dsim);
					continue;
				}
			} else {
				if (dsim_write_data(dsim,
							MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xB0, offset, false, wakeup)) {
					pr_err("%s failed to write gpara %d (retry %d)\n",
							__func__, offset, retry);
					dsim_function_reset(dsim);
					continue;
				}
			}
		}

		if (dsim_write_data(dsim, type, d0, d1, block, wakeup)) {
			pr_err("%s failed to write cmd %02X size %d(retry %d)\n",
					__func__, cmd[0], size, retry);
			dsim_function_reset(dsim);
			continue;
		}
		break;
	}

	if (retry < 0) {
		pr_err("%s failed: exceed retry count (cmd %02X)\n",
				__func__, cmd[0]);
		ret = -EIO;
		goto error;
	}

	pr_debug("%s cmd_id %d, addr %02X offset %d size %d\n",
			__func__, cmd_id, cmd[0], offset, size);
	ret = size;

error:
	mutex_unlock(&cmd_lock);
	return ret;
}

int mipi_read(u32 id, u8 addr, u8 offset, u8 *buf, int size, u32 option)
{
	int ret, retry = 3;
	struct dsim_device *dsim = get_dsim_drvdata(id);

	if (!buf) {
		pr_err("%s buf is null\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&cmd_lock);
	while (--retry >= 0) {
		if (offset > 0) {
			if (option & DSIM_OPTION_POINT_GPARA) {
				u8 gpara[3] = { 0xB0, offset, addr };
				if (dsim_write_data(dsim, MIPI_DSI_DCS_LONG_WRITE,
							(unsigned long)gpara, ARRAY_SIZE(gpara), false, true)) {
					pr_err("%s failed to write gpara %d (retry %d)\n",
							__func__, offset, retry);
					dsim_function_reset(dsim);
					continue;
				}
			} else {
				if (dsim_write_data(dsim,
							MIPI_DSI_DCS_SHORT_WRITE_PARAM, 0xB0, offset, false, true)) {
					pr_err("%s failed to write gpara %d (retry %d)\n",
							__func__, offset, retry);
					dsim_function_reset(dsim);
					continue;
				}
			}
		}

		ret = dsim_read_data(dsim, MIPI_DSI_DCS_READ,
				(u32)addr, size, buf);
		if (ret != size) {
			pr_err("%s, failed to read addr %02X ofs %d size %d (ret %d, retry %d)\n",
					__func__, addr, offset, size, ret, retry);
			dsim_function_reset(dsim);
			continue;
		}
		break;
	}

	if (retry < 0) {
		pr_err("%s failed: exceed retry count (addr %02X)\n",
				__func__, addr);
		ret = -EIO;
		goto error;
	}

	pr_debug("%s addr %02X ofs %d size %d, buf %02X done\n",
			__func__, addr, offset, size, buf[0]);

	ret = size;

error:
	mutex_unlock(&cmd_lock);
	return ret;
}

enum dsim_state get_dsim_state(u32 id)
{
	struct dsim_device *dsim = NULL;

	dsim = dsim_drvdata[id];
	if (dsim == NULL) {
		dsim_err("ERR:DSIM:%s:dsim is NULL\n", __func__);
		return -ENODEV;
	}
	return dsim->state;
}

static int dsim_panel_put_ops(struct dsim_device *dsim)
{
	int ret = 0;
	struct mipi_drv_ops mipi_ops;

	mipi_ops.read = mipi_read;
	mipi_ops.write = mipi_write;
	mipi_ops.get_state = get_dsim_state;

	v4l2_set_subdev_hostdata(panel_sd, &mipi_ops);

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DSIM_PUT_MIPI_OPS, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to set mipi ops\n", __func__);
		return ret;
	}

	return ret;
}

static int dsim_probe_panel(struct dsim_device *dsim)
{
	int ret;
	struct device *dev;
	struct device_driver *drv;

	drv = driver_find(PANEL_DRV_NAME, &platform_bus_type);
	if (IS_ERR_OR_NULL(drv)) {
		dsim_err("DSIM:ERR:%sfailed to find driver\n", __func__);
		return -ENODEV;
	}

	dev = driver_find_device(drv, NULL, dsim, __match_panel_v4l2_subdev);
	if (panel_sd == NULL) {
		dsim_err("DSIM:ERR:%s:failed to fined panel's v4l2 subdev\n", __func__);
		return -ENODEV;
	}

	ret = dsim_panel_probe(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to probe panel", __func__);
		goto do_exit;
	}

	ret = dsim_panel_put_ops(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to put ops\n", __func__);
		goto do_exit;
	}

	ret = dsim_panel_get_state(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get panel state\n", __func__);
		goto do_exit;
	}
#ifdef CONFIG_DYNAMIC_FREQ
	ret = dsim_panel_get_df_status(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get df status\n", __func__);
		goto do_exit;
	}
#endif


	return ret;

do_exit:
	return ret;
}

static int common_lcd_connected(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_panel_get_state(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to get panel state\n", __func__);
		return ret;
	}

	ret = !(panel_state->connect_panel == PANEL_DISCONNECT);
	pr_info("%s panel %s\n", __func__,
			ret ? "connected" : "disconnected");

	return ret;
}

static int common_lcd_init(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_probe_panel(dsim);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to init common panel\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_probe(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_PANEL_PROBE, &dsim->id);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to probe panel\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_displayon(struct dsim_device *dsim)
{
	int ret;
	int disp_on = 1;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DISP_ON, (void *)&disp_on);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to display on\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_suspend(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_IN, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to sleep in\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_resume(struct dsim_device *dsim)
{
	return 0;
}

static int common_lcd_dump(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_PANEL_DUMP, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to dump panel\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_setarea(struct dsim_device *dsim, u32 l, u32 r, u32 t, u32 b)
{
	int ret = 0;
	char column[5];
	char page[5];
	int retry;

	column[0] = MIPI_DCS_SET_COLUMN_ADDRESS;
	column[1] = (l >> 8) & 0xff;
	column[2] = l & 0xff;
	column[3] = (r >> 8) & 0xff;
	column[4] = r & 0xff;

	page[0] = MIPI_DCS_SET_PAGE_ADDRESS;
	page[1] = (t >> 8) & 0xff;
	page[2] = t & 0xff;
	page[3] = (b >> 8) & 0xff;
	page[4] = b & 0xff;

	mutex_lock(&cmd_lock);
	retry = 2;
	while (dsim_write_data(dsim, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned long)column, ARRAY_SIZE(column), false, true) != 0) {
		dsim_err("failed to write COLUMN_ADDRESS\n");
		dsim_function_reset(dsim);
		if (--retry <= 0) {
			dsim_err("COLUMN_ADDRESS is failed: exceed retry count\n");
			ret = -EINVAL;
			goto error;
		}
	}

	retry = 2;
	while (dsim_write_data(dsim, MIPI_DSI_DCS_LONG_WRITE,
				(unsigned long)page, ARRAY_SIZE(page), true, true) != 0) {
		dsim_err("failed to write PAGE_ADDRESS\n");
		dsim_function_reset(dsim);
		if (--retry <= 0) {
			dsim_err("PAGE_ADDRESS is failed: exceed retry count\n");
			ret = -EINVAL;
			goto error;
		}
	}
	pr_debug("RECT [l:%d r:%d t:%d b:%d w:%d h:%d]\n",
			l, r, t, b, r - l + 1, b - t + 1);

error:
	mutex_unlock(&cmd_lock);
	return ret;
}

static int common_lcd_power(struct dsim_device *dsim, int on)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_POWER, (void *)&on);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to panel %s\n",
				__func__, on ? "on" : "off");
		return ret;
	}
	pr_debug("%s power %s\n", __func__, on ? "on" : "off");

	return 0;
}

static int common_lcd_poweron(struct dsim_device *dsim)
{
	return common_lcd_power(dsim, 1);
}

static int common_lcd_poweroff(struct dsim_device *dsim)
{
	return common_lcd_power(dsim, 0);
}

static int common_lcd_sleepin(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_IN, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to sleep in\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_sleepout(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SLEEP_OUT, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to sleep out\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_notify(struct dsim_device *dsim, void *data)
{
	int ret;

	ret = dsim_notify_panel(panel_sd, V4L2_DEVICE_NOTIFY_EVENT, data);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to sleep out\n", __func__);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_SUPPORT_DOZE
static int common_lcd_doze(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DOZE, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to sleep out\n", __func__);
		return ret;
	}

	return 0;
}

static int common_lcd_doze_suspend(struct dsim_device *dsim)
{
	int ret;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_DOZE_SUSPEND, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to sleep out\n", __func__);
		return ret;
	}

	return 0;
}
#endif

#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
static int common_mipi_freq_change(struct dsim_device *dsim)
{
	int ret = 0;
	
	ret = dsim_ioctl_panel(dsim,
			PANEL_IOC_MIPI_FREQ_CHANGED, NULL);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to change freq\n",
				__func__);
		return ret;
	}
	
	return ret;
}
#endif


#ifdef CONFIG_DYNAMIC_FREQ
static int common_set_df_default(struct dsim_device *dsim)
{
	int ret = 0;
	struct df_status_info *status = dsim->df_status;
	struct df_dt_info *df_info = &dsim->lcd_info.df_set_info;

	status->target_df = df_info->dft_index;
	status->current_df = df_info->dft_index;

	if (dsim->df_mode == DSIM_MODE_POWER_OFF) {
		status->target_df = MAX_DYNAMIC_FREQ;
		status->current_df = MAX_DYNAMIC_FREQ;
		status->ffc_df = MAX_DYNAMIC_FREQ;
	}

	return ret;
}
#endif


#ifdef CONFIG_SUPPORT_DSU
static int common_lcd_dsu(struct dsim_device *dsim, int mres_idx)
{
	int ret = 0;

	ret = dsim_ioctl_panel(dsim, PANEL_IOC_SET_DSU, &mres_idx);
	if (ret) {
		dsim_err("DSIM:ERR:%s:failed to set multi-resolution\n", __func__);
		goto dsu_exit;
	}

	/* update lcd resolution @ lcd_info */
	dsim_panel_probe(dsim);

dsu_exit:
	return ret;
}
#endif

struct dsim_lcd_driver common_mipi_lcd_driver = {
	.init		= common_lcd_init,
	.probe		= common_lcd_probe,
	.suspend	= common_lcd_suspend,
	.resume		= common_lcd_resume,
	.dump		= common_lcd_dump,
	.connected	= common_lcd_connected,
	.setarea	= common_lcd_setarea,
	.poweron	= common_lcd_poweron,
	.poweroff	= common_lcd_poweroff,
	.sleepin	= common_lcd_sleepin,
	.sleepout	= common_lcd_sleepout,
	.displayon	= common_lcd_displayon,
	.notify		= common_lcd_notify,
	.set_error_cb	= common_lcd_set_error_cb,
#ifdef CONFIG_SUPPORT_DOZE
	.doze		= common_lcd_doze,
	.doze_suspend	= common_lcd_doze_suspend,
#endif
#ifdef CONFIG_EXYNOS_ADAPTIVE_FREQ
	.mipi_freq_change = common_mipi_freq_change,
#endif
#ifdef CONFIG_DYNAMIC_FREQ
	.set_df_default = common_set_df_default,
	.update_lcd_info = common_update_lcd_info,
#endif

#ifdef CONFIG_SUPPORT_DSU
	.mres = common_lcd_dsu,
#endif
};
