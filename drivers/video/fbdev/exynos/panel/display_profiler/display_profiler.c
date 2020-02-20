/*
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>

#include "../panel_drv.h"
#include "display_profiler.h"

static int profiler_do_seqtbl_by_index_nolock(struct profiler_device *profiler, int index)
{
	int ret;
	struct seqinfo *tbl;
	struct panel_info *panel_data;
	struct panel_device *panel = to_panel_device(profiler);
	struct timespec cur_ts, last_ts, delta_ts;
	s64 elapsed_usec;

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	if (!IS_PANEL_ACTIVE(panel))
		return 0;

	panel_data = &panel->panel_data;
	tbl = panel->profiler.seqtbl;
	ktime_get_ts(&cur_ts);

#if 0
	ktime_get_ts(&last_ts);
#endif

	if (unlikely(index < 0 || index >= MAX_PROFILER_SEQ)) {
		panel_err("%s, invalid paramter (panel %p, index %d)\n",
				__func__, panel, index);
		ret = -EINVAL;
		goto do_exit;
	}

#if 0
	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	if (elapsed_usec > 34000)
		pr_debug("seq:%s warn:elapsed %lld.%03lld msec to acquire lock\n",
				tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);
#endif

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s before seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ret = panel_do_seqtbl(panel, &tbl[index]);
	if (unlikely(ret < 0)) {
		pr_err("%s, failed to excute seqtbl %s\n",
				__func__, tbl[index].name);
		ret = -EIO;
		goto do_exit;
	}

do_exit:

	if (panel_data->props.key[CMD_LEVEL_1] != 0 ||
			panel_data->props.key[CMD_LEVEL_2] != 0 ||
			panel_data->props.key[CMD_LEVEL_3] != 0) {
		panel_warn("%s after seq:%s unbalanced key [1]%s(%d) [2]%s(%d) [3]%s(%d)\n",
				__func__, tbl[index].name,
				(panel_data->props.key[CMD_LEVEL_1] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_1],
				(panel_data->props.key[CMD_LEVEL_2] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_2],
				(panel_data->props.key[CMD_LEVEL_3] > 0 ? "ENABLED" : "DISABLED"),
				panel_data->props.key[CMD_LEVEL_3]);
		panel_data->props.key[CMD_LEVEL_1] = 0;
		panel_data->props.key[CMD_LEVEL_2] = 0;
		panel_data->props.key[CMD_LEVEL_3] = 0;
	}

	ktime_get_ts(&last_ts);
	delta_ts = timespec_sub(last_ts, cur_ts);
	elapsed_usec = timespec_to_ns(&delta_ts) / 1000;
	pr_debug("seq:%s done (elapsed %2lld.%03lld msec)\n",
			tbl[index].name, elapsed_usec / 1000, elapsed_usec % 1000);

	return 0;
}

int profiler_do_seqtbl_by_index(struct profiler_device *profiler, int index)
{
	int ret = 0;
	struct panel_device *panel = to_panel_device(profiler);

	if (panel == NULL) {
		panel_err("ERR:PANEL:%s:panel is null\n", __func__);
		return -EINVAL;
	}

	panel_info("%s : %d\n", __func__, index);

	mutex_lock(&panel->op_lock);
	ret = profiler_do_seqtbl_by_index_nolock(profiler, index);
	mutex_unlock(&panel->op_lock);

	return ret;
}


#define PROFILER_SYSTRACE_BUF_SIZE	40

extern int decon_systrace_enable;

void profiler_systrace_write(int pid, char id, char *str, int value)
{
	char buf[PROFILER_SYSTRACE_BUF_SIZE] = {0, };

	if (!decon_systrace_enable)
		return;

	 switch(id) {
		case 'B':
			snprintf(buf, PROFILER_SYSTRACE_BUF_SIZE, "B|%d|%s", pid, str);
			break;
		case 'E':
			strcpy(buf, "E");
			break;
		case 'C':
			snprintf(buf, PROFILER_SYSTRACE_BUF_SIZE,
				"C|%d|%s|%d", pid, str, 1);
			break;
		default:
			panel_err("PANEL:ERR:%s:wrong argument : %c\n", __func__, id);
			return;
	}
	panel_info("%s\n", buf);
	trace_printk(buf);

}


static int update_profile_win(struct profiler_device *profiler, void *arg)
{
	int ret;
	int width, height;
	struct panel_device *panel;
	int cmd_idx = PROFILE_DISABLE_WIN_SEQ;
	struct decon_rect *up_region = (struct decon_rect *)arg;

	panel = container_of(profiler, struct panel_device, profiler);

	memcpy((struct decon_rect *)&profiler->win_rect, up_region, sizeof(struct decon_rect));

	width = profiler->win_rect.right - profiler->win_rect.left;
	height = profiler->win_rect.bottom - profiler->win_rect.top;

	panel_info("PROFILE_WIN_UPDATE region : %d:%d, %d:%d\n",
			profiler->win_rect.left, profiler->win_rect.top,
			profiler->win_rect.right, profiler->win_rect.bottom);

	if ((width == panel->lcd_info.xres - 1) &&
		(height == panel->lcd_info.yres - 1)) {
		cmd_idx = PROFILE_DISABLE_WIN_SEQ;
		panel_info("PROFILE_WIN_UPDATE full region : disable\n");
	} else {
		cmd_idx = PROFILE_WIN_UPDATE_SEQ;
	}

	ret = profiler_do_seqtbl_by_index(profiler, cmd_idx);

	return ret;
}


static int update_profile_fps(struct profiler_device *profiler)
{
	int ret = 0;

	if (profiler->flag_font == 0) {
		ret = profiler_do_seqtbl_by_index(profiler, SEND_FROFILE_FONT_SEQ);
		ret = profiler_do_seqtbl_by_index(profiler, INIT_PROFILE_FPS_SEQ);
		profiler->flag_font = 1;
	} 

	ret = profiler_do_seqtbl_by_index(profiler, DISPLAY_PROFILE_FPS_SEQ);
	if (profiler->fps.instant_fps < 59) {
		profiler->fps.color = FPS_COLOR_GREEN;
		ret = profiler_do_seqtbl_by_index(profiler, PROFILE_SET_COLOR_SEQ);
	}
	return ret;
}


static int update_profile_win_config(struct profiler_device *profiler)
{
	int ret = 0;
	s64 diff;
	ktime_t timestamp;

	timestamp = ktime_get();

	if (profiler->fps.frame_cnt < FPS_MAX)
		profiler->fps.frame_cnt++;
	else
		profiler->fps.frame_cnt = 0;

	if (ktime_to_us(profiler->fps.win_config_time) != 0) {
		diff = ktime_to_us(ktime_sub(timestamp, profiler->fps.win_config_time));
		profiler->fps.instant_fps = (int)(1000000 / diff);

		if (profiler->fps.instant_fps >= 59)
			profiler->fps.color = FPS_COLOR_RED;
		else
			profiler->fps.color = FPS_COLOR_GREEN;

		decon_systrace(get_decon_drvdata(0), 'C', "fps_inst", (int)profiler->fps.instant_fps);

		ret = profiler_do_seqtbl_by_index(profiler, PROFILE_SET_COLOR_SEQ);
	}
	profiler->fps.win_config_time = timestamp;

	return ret;
}


struct win_config_backup {
	int state;
	struct decon_frame src;
	struct decon_frame dst;
};



static int update_profiler_circle(struct profiler_device *profiler, unsigned int color)
{
	int ret = 0;

	if (profiler->circle_color != color) {
		profiler->circle_color = color;
		ret = profiler_do_seqtbl_by_index(profiler, PROFILER_SET_CIRCLR_SEQ);
	}

	return ret;
}

static long profiler_v4l2_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	unsigned int color;
	struct panel_device *panel;
	struct profiler_device *profiler = container_of(sd, struct profiler_device, sd);
	
	panel = container_of(profiler, struct panel_device, profiler);

	switch(cmd) {
		case PROFILE_REG_DECON:
			panel_info("PROFILE_REG_DECON was called\n");
			break;

		case PROFILE_WIN_UPDATE:
			if (profiler->win_rect.disp_en)
				ret = update_profile_win(profiler, arg);
			break;

		case PROFILE_WIN_CONFIG:
			if (profiler->fps.disp_en)
				ret = update_profile_win_config(profiler);
			break;
			
		case PROFILER_SET_PID:
			profiler->systrace.pid = *(int *)arg;
			break;

		case PROFILER_COLOR_CIRCLE:
			color = *(int *)arg;
			ret = update_profiler_circle(profiler, color);
			break;

	}

	return (long)ret;
}

static const struct v4l2_subdev_core_ops profiler_v4l2_core_ops = {
	.ioctl = profiler_v4l2_ioctl,
};

static const struct v4l2_subdev_ops profiler_subdev_ops = {
	.core = &profiler_v4l2_core_ops,
};

static void profiler_init_v4l2_subdev(struct panel_device *panel)
{
	struct v4l2_subdev *sd;
	struct profiler_device *profiler = &panel->profiler;

	sd = &profiler->sd;

	v4l2_subdev_init(sd, &profiler_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = 0;

	snprintf(sd->name, sizeof(sd->name), "%s.%d", "panel-profiler", 0);

	v4l2_set_subdevdata(sd, profiler);
}


void profile_fps(struct profiler_device *profiler)
{
	s64 time_diff;
	unsigned int gap, c_frame_cnt;
	ktime_t c_time, p_time;
	struct profiler_fps *fps;
 
	fps = &profiler->fps;

	c_frame_cnt = fps->frame_cnt;
	c_time = ktime_get();

	if (c_frame_cnt >= fps->prev_frame_cnt)
		gap = c_frame_cnt - fps->prev_frame_cnt;
	else
		gap = (FPS_MAX - fps->prev_frame_cnt) + c_frame_cnt;

	p_time = fps->slot[fps->slot_cnt].timestamp;
	fps->slot[fps->slot_cnt].timestamp = c_time;

	fps->total_frame -= fps->slot[fps->slot_cnt].frame_cnt;
	fps->total_frame += gap;
	fps->slot[fps->slot_cnt].frame_cnt = gap;

	time_diff = ktime_to_us(ktime_sub(c_time, p_time));
	//panel_info("%s : diff : %llu : slot_cnt %d\n", __func__, time_diff, fps->slot_cnt);
 
	/*To Do.. after lcd off->on, must clear fps->slot data to zero and comp_fps, instan_fps set to 60Hz (default)*/
	if (ktime_to_us(p_time) != 0) {
		time_diff = ktime_to_us(ktime_sub(c_time, p_time));

		fps->average_fps = fps->total_frame;
		fps->comp_fps = (unsigned int)(((1000000000 / time_diff) * fps->total_frame) + 500) / 1000;

		panel_info("[DISP_PROFILER] : aver fps : %d\n", fps->comp_fps);

		time_diff = ktime_to_us(ktime_sub(c_time, profiler->fps.win_config_time));
		if (time_diff >= 100000) {
			fps->instant_fps = fps->average_fps;
			decon_systrace(get_decon_drvdata(0), 'C', "fps_inst", (int)fps->instant_fps);
		}
		decon_systrace(get_decon_drvdata(0), 'C', "fps_aver", (int)fps->comp_fps);
	}

	fps->prev_frame_cnt = c_frame_cnt;
	fps->slot_cnt = (fps->slot_cnt + 1) % MAX_SLOT_CNT;
}


static int profiler_thread(void *data)
{
	int ret;
	struct profiler_device *profiler = data;
	//struct profiler_fps *fps = &profiler->fps;
 
	while(!kthread_should_stop()) {
		schedule_timeout_interruptible(msecs_to_jiffies(100));
		if (profiler->fps.disp_en) {
			profile_fps(profiler);
			ret = update_profile_fps(profiler);
		}

	}
	return 0;
}


static ssize_t prop_fps_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct profiler_device *profiler;
	struct panel_device *panel = dev_get_drvdata(dev);

	profiler = &panel->profiler;
	if (profiler == NULL) {
		panel_err("PROFILER:ERR:%s:profiler is null\n", __func__);
		return -EINVAL;
	}

	snprintf(buf, PAGE_SIZE, "%u\n", profiler->fps.disp_en);

	return strlen(buf);
}


static ssize_t prop_fps_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct profiler_device *profiler;
	struct panel_device *panel = dev_get_drvdata(dev);

	profiler = &panel->profiler;
	if (profiler == NULL) {
		panel_err("PROFILER:ERR:%s:profiler is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	panel_info("PROFILER:INFO:%s:FPS Disp En : %d\n", __func__, value);

	profiler->fps.disp_en = (value) ? true : false;

	return size;
}


static ssize_t prop_partial_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct profiler_device *profiler;
	struct panel_device *panel = dev_get_drvdata(dev);

	profiler = &panel->profiler;
	if (profiler == NULL) {
		panel_err("PROFILER:ERR:%s:profiler is null\n", __func__);
		return -EINVAL;
	}

	snprintf(buf, PAGE_SIZE, "%u\n", profiler->win_rect.disp_en);

	return strlen(buf);
}


static ssize_t prop_partial_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct profiler_device *profiler;
	struct panel_device *panel = dev_get_drvdata(dev);

	profiler = &panel->profiler;
	if (profiler == NULL) {
		panel_err("PROFILER:ERR:%s:profiler is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	panel_info("PROFILER:INFO:%s:Partial Disp En : %d\n", __func__, value);
	
	profiler->win_rect.disp_en = (value) ? true : false;

	return size;
}


static ssize_t prop_hiber_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct profiler_device *profiler;
	struct panel_device *panel = dev_get_drvdata(dev);

	profiler = &panel->profiler;
	if (profiler == NULL) {
		panel_err("PROFILER:ERR:%s:profiler is null\n", __func__);
		return -EINVAL;
	}

	//snprintf(buf, PAGE_SIZE, "%u\n", profiler->fps.disp_en);

	return strlen(buf);
}


static ssize_t prop_hiber_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int value, rc;
	struct profiler_device *profiler;
	struct panel_device *panel = dev_get_drvdata(dev);

	profiler = &panel->profiler;
	if (profiler == NULL) {
		panel_err("PROFILER:ERR:%s:profiler is null\n", __func__);
		return -EINVAL;
	}

	rc = kstrtouint(buf, 0, &value);
	if (rc < 0)
		return rc;

	panel_info("PROFILER:INFO:%s:FPS Disp En : %d\n", value);
	//profiler->fps.disp_en = value;

	return size;
}

struct device_attribute profiler_attrs[] = {
	__PANEL_ATTR_RW(prop_fps, 0664),
	__PANEL_ATTR_RW(prop_partial, 0664),
	__PANEL_ATTR_RW(prop_hiber, 0664),
};


int profiler_probe(struct panel_device *panel, struct profiler_tune *tune)
{
	int i;
	int ret = 0;
	struct lcd_device *lcd;
	struct profiler_device *profiler;

	if (!panel) {
		panel_err("PANEL:ERR:%s:panel is not exist\n", __func__);
		return -EINVAL;
	}

	if (!tune) {
		panel_err("PANEL:ERR:%s:tune is null\n");
		return -EINVAL;
	}

	lcd = panel->lcd;
		if (unlikely(!lcd)) {
			pr_err("%s, lcd device not exist\n", __func__);
			return -ENODEV;
		}

	profiler = &panel->profiler;
	profiler_init_v4l2_subdev(panel);

	profiler->seqtbl = tune->seqtbl;
	profiler->nr_seqtbl = tune->nr_seqtbl;
	profiler->maptbl = tune->maptbl;
	profiler->nr_maptbl = tune->nr_maptbl;

	for (i = 0; i < profiler->nr_maptbl; i++) {
		profiler->maptbl[i].pdata = profiler;
		maptbl_init(&profiler->maptbl[i]);
	}

	for (i = 0; i < ARRAY_SIZE(profiler_attrs); i++) {
		ret = device_create_file(&lcd->dev, &profiler_attrs[i]);
		if (ret < 0) {
			dev_err(&lcd->dev, "%s, failed to add %s sysfs entries, %d\n",
					__func__, profiler_attrs[i].attr.name, ret);
			return -ENODEV;
		}
	}

	profiler->thread = kthread_run(profiler_thread, profiler, "profiler");
	if (IS_ERR_OR_NULL(profiler->thread)) {
		panel_err("PROFILER:ERR:%s:failed to run thread\n", __func__);
		ret = PTR_ERR(profiler->thread);
		goto err;
	}

	profiler->fps.disp_en = false;
	profiler->win_rect.disp_en = false;

err:
	return ret;
}

