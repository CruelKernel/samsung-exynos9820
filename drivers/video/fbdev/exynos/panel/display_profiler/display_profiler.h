/*
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DISPLAY_PROFILER_H__
#define __DISPLAY_PROFILER_H__

#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>

#include <media/v4l2-subdev.h>

enum PROFILER_SEQ {
	PROFILE_WIN_UPDATE_SEQ = 0,
	PROFILE_DISABLE_WIN_SEQ,
	SEND_FROFILE_FONT_SEQ,
	INIT_PROFILE_FPS_SEQ,
	DISPLAY_PROFILE_FPS_SEQ,
	PROFILE_SET_COLOR_SEQ,
	PROFILER_SET_CIRCLR_SEQ,
	MAX_PROFILER_SEQ,
};


enum PROFILER_MAPTBL {
	PROFILE_WIN_UPDATE_MAP = 0,
	DISPLAY_PROFILE_FPS_MAP,
	PROFILE_SET_COLOR_MAP,
	PROFILE_SET_CIRCLE,
	MAX_PROFILER_MAPTBL,
};


#define FPS_COLOR_RED	0
#define FPS_COLOR_BLUE	1
#define FPS_COLOR_GREEN	2


struct profiler_tune {
	char *name;
	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;
};


struct prifiler_rect {
	u32 left;
	u32 top;
	u32 right;
	u32 bottom;
	
	bool disp_en;
};


struct fps_slot {
	ktime_t timestamp;
	unsigned int frame_cnt;
};


struct profiler_systrace {
	int pid;
};

#define FPS_MAX 100000
#define MAX_SLOT_CNT 10

struct profiler_fps {
	unsigned int frame_cnt;
	unsigned int prev_frame_cnt;

	unsigned int instant_fps;
	unsigned int average_fps;
	unsigned int comp_fps;

	ktime_t win_config_time;

	struct fps_slot slot[MAX_SLOT_CNT];
	unsigned int slot_cnt;
	unsigned int total_frame;
	unsigned int color;

	bool disp_en;
};

struct profiler_device {
	struct v4l2_subdev sd;

	struct seqinfo *seqtbl;
	u32 nr_seqtbl;
	struct maptbl *maptbl;
	u32 nr_maptbl;

	struct prifiler_rect win_rect;
	//struct profile_data *data;

	struct profiler_fps fps;
	
	int prev_win_cnt;
	int flag_font;
	unsigned int circle_color;

	struct profiler_systrace systrace;

	struct task_struct *thread;


};

#define PROFILER_IOC_BASE	'P'

#define PROFILE_REG_DECON 	_IOW(PANEL_IOC_BASE, 1, struct profile_data *)
#define PROFILE_WIN_UPDATE	_IOW(PANEL_IOC_BASE, 2, struct decon_rect *)
#define PROFILE_WIN_CONFIG	_IOW(PANEL_IOC_BASE, 3, struct decon_win_config_data *)
#define PROFILER_SET_PID	_IOW(PANEL_IOC_BASE, 4, int *)
#define PROFILER_COLOR_CIRCLE	_IOW(PANEL_IOC_BASE, 5, int *)

int profiler_probe(struct panel_device *panel, struct profiler_tune *profile_tune);

#endif //__DISPLAY_PROFILER_H__
