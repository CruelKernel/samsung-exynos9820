/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is video functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/time.h>

#include "fimc-is-time.h"
#include "fimc-is-groupmgr.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"

static struct timeval itime[10];

#define JITTER_CNT 50
static u64 jitter_array[JITTER_CNT];
static u64 jitter_prio;
static u64 jitter_cnt = 0;

void TIME_STR(unsigned int index)
{
	do_gettimeofday(&itime[index]);
}

void TIME_END(unsigned int index, const char *name)
{
	long time;
	struct timeval temp;

	do_gettimeofday(&temp);
	time = (temp.tv_sec - itime[index].tv_sec)*1000000 +
		(temp.tv_usec - itime[index].tv_usec);

	info("TIME(%s) : %ld us\n", name, time);
}

void fimc_is_jitter(u64 timestamp)
{
	if (jitter_cnt == 0) {
		jitter_prio = timestamp;
		jitter_cnt++;
		return;
	}

	jitter_array[jitter_cnt-1] = timestamp - jitter_prio;
	jitter_prio = timestamp;

	if (jitter_cnt >= JITTER_CNT) {
		u64 i, variance, tot = 0, square_tot = 0, avg = 0, square_avg = 0;;

		for (i = 0; i < JITTER_CNT; ++i) {
			tot += jitter_array[i];
			square_tot += (jitter_array[i] * jitter_array[i]);
		}

		avg = tot / JITTER_CNT;
		square_avg = square_tot / JITTER_CNT;
		variance = square_avg - (avg * avg);

		info("[TIM] variance : %lld, average : %lld\n", variance, avg);
		jitter_cnt = 0;
	} else {
		jitter_cnt++;
	}
}

u64 fimc_is_get_timestamp(void)
{
	struct timespec curtime;

	ktime_get_ts(&curtime);

	return (u64)curtime.tv_sec*1000000000 + curtime.tv_nsec;
}

u64 fimc_is_get_timestamp_boot(void)
{
	struct timespec curtime;

	curtime = ktime_to_timespec(ktime_get_boottime());

	return (u64)curtime.tv_sec*1000000000 + curtime.tv_nsec;
}

static inline u32 fimc_is_get_time(struct timeval *str, struct timeval *end)
{
	return (end->tv_sec - str->tv_sec)*1000000 + (end->tv_usec - str->tv_usec);
}

#ifdef MEASURE_TIME
#ifdef MONITOR_TIME
void monitor_init(struct fimc_is_time *time)
{
	time->time_count = 0;
	memset(&time->t_dq[0], 0, sizeof(unsigned long long) * 10);
	time->t_dqq_max = 0;
	time->t_dqq_tot = 0;
	time->time1_max = 0;
	time->time1_tot = 0;
	time->time2_max = 0;
	time->time2_tot = 0;
	time->time3_max = 0;
	time->time3_tot = 0;
	time->time4_cur = 0;
	time->time4_old = 0;
	time->time4_tot = 0;
}

static void monitor_report(void *group_data,
	void *frame_data)
{
	unsigned int avg_cnt;
	u32 index, shotindex;
	unsigned long long fduration, ctime, dtime;
	unsigned long long temp_s = 0, temp_sd = 0, temp_d = 0, temp_dqq = 0, shot_to_shot = 0;
	struct fimc_is_monitor *mp;
	struct fimc_is_device_ischain *device;
	struct fimc_is_group *group;
	struct fimc_is_frame *frame;
	struct fimc_is_time *time;
	bool valid = true;
	u32 f_dqq;

	FIMC_BUG_VOID(!group_data);
	FIMC_BUG_VOID(!frame_data);

	group = group_data;
	device = group->device;
	time = &group->time;
	frame = frame_data;
	mp = frame->mpoint;
	fduration = (1000000 / fimc_is_sensor_g_framerate(device->sensor)) + MONITOR_TIMEOUT;
	ctime = 5000;
	dtime = 5000;
	f_dqq = frame->fcount % 10;

	/* Shot kthread */
	if (!frame->result && mp[TMS_Q].check && mp[TMS_SHOT1].check) {
		temp_s = (mp[TMS_SHOT1].time - mp[TMS_Q].time) / 1000;
		if (!test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state) && temp_s > ctime)
			mgrinfo("[TIM] late S(%llu us > %llu us)\n", device, group, frame, temp_s, ctime);
		if (time->t_dq[f_dqq])
			temp_dqq = (mp[TMS_Q].time - time->t_dq[f_dqq]) / 1000;
	} else {
		valid = false;
	}

	/* Shot - Done */
	if (test_bit(FIMC_IS_GROUP_OTF_INPUT, &group->state))
		shotindex = TMS_SHOT2;
	else
		shotindex = TMS_SHOT1;

	if (!frame->result && mp[shotindex].check && mp[TMS_SDONE].check) {
		temp_sd = (mp[TMS_SDONE].time - mp[shotindex].time) / 1000;
		if (temp_sd > fduration)
			mgrinfo("[TIM] late S-D(%llu us > %llu us)\n", device, group, frame, temp_sd, fduration);
	} else {
		valid = false;
	}

	/* Done - Deque */
	if (!frame->result && mp[TMS_SDONE].check && mp[TMS_DQ].check) {
		temp_d = (mp[TMS_DQ].time - mp[TMS_SDONE].time) / 1000;
		if (temp_d > dtime)
			mgrinfo("[TIM] late D(%llu us > %llu us)\n", device, group, frame, temp_d, dtime);
		if (group->gnext)
			group->gnext->time.t_dq[f_dqq] = mp[TMS_DQ].time;
	} else {
		valid = false;
	}

	for (index = 0; index < TMS_END; ++index)
		mp[index].check = false;

	if (!valid)
		return;

	if (!time->time_count) {
		time->t_dqq_max = temp_dqq;
		time->time1_max = temp_s;
		time->time2_max = temp_sd;
		time->time3_max = temp_d;
	} else {
		if (time->t_dqq_max < temp_dqq)
			time->t_dqq_max = temp_dqq;

		if (time->time1_max < temp_s)
			time->time1_max = temp_s;

		if (time->time2_max < temp_sd)
			time->time2_max = temp_sd;

		if (time->time3_max < temp_d)
			time->time3_max = temp_d;
	}

	time->t_dqq_tot += temp_dqq;
	time->time1_tot += temp_s;
	time->time2_tot += temp_sd;
	time->time3_tot += temp_d;

	time->time4_cur = mp[shotindex].time;
	time->time4_tot += (time->time4_cur - time->time4_old) / 1000;
	time->time4_old = time->time4_cur;

	time->time_count++;

	if (!test_bit(FIMC_IS_ISCHAIN_REPROCESSING, &device->state)) {
		avg_cnt = debug_time_shot;
		if (time->time_count % avg_cnt)
			return;
	} else {
		avg_cnt = 1;
	}

	shot_to_shot = time->time4_tot / avg_cnt;

	mginfo("[TIM] DQ-Q(avg: %05llu max: %05llu), S(avg: %05llu max: %05llu), S-D(avg: %05llu max: %05llu), D(avg: %05llu max: %05llu): %llu(%llufps)\n",
		device, group,
		time->t_dqq_tot / avg_cnt, time->t_dqq_max,
		time->time1_tot / avg_cnt, time->time1_max,
		time->time2_tot / avg_cnt, time->time2_max,
		time->time3_tot / avg_cnt, time->time3_max,
		shot_to_shot, 1000000 / shot_to_shot);

	time->time_count = 0;
	time->t_dqq_tot = 0;
	time->time1_tot = 0;
	time->time2_tot = 0;
	time->time3_tot = 0;
	time->time4_tot = 0;
}
#endif

#ifdef INTERFACE_TIME
void measure_init(struct fimc_is_interface_time *time, u32 cmd)
{
	time->cmd = cmd;
	time->time_max = 0;
	time->time_min = 0;
	time->time_tot = 0;
	time->time_cnt = 0;
}

void measure_time(struct fimc_is_interface_time *time,
	u32 instance,
	u32 group,
	struct timeval *start,
	struct timeval *end)
{
	u32 temp;

	temp = (end->tv_sec - start->tv_sec)*1000000 + (end->tv_usec - start->tv_usec);

	if (time->time_cnt) {
		time->time_max = temp;
		time->time_min = temp;
	} else {
		if (time->time_min > temp)
			time->time_min = temp;

		if (time->time_max < temp)
			time->time_max = temp;
	}

	time->time_tot += temp;
	time->time_cnt++;

	pr_info("cmd[%d][%d](%d) : curr(%d), max(%d), avg(%d)\n",
		instance, group, time->cmd, temp, time->time_max, time->time_tot / time->time_cnt);
}
#endif
#endif

void monitor_point(void *group_data,
	u32 mpoint)
{
	struct fimc_is_group *group;

	FIMC_BUG_VOID(!group_data);

	group = group_data;
	group->pcount = mpoint;
}

void monitor_time_shot(void *group_data,
	void *frame_data,
	u32 mpoint)
{
#if defined(MEASURE_TIME) && defined(MONITOR_TIME)
	struct fimc_is_frame *frame;
	struct fimc_is_monitor *point;

	FIMC_BUG_VOID(!frame_data);

	frame = frame_data;
	point = &frame->mpoint[mpoint];
	point->time = sched_clock();
	point->check = true;

	if (mpoint == TMS_DQ)
		monitor_report(group_data, frame_data);
#endif
}

void monitor_time_queue(void *vctx_data,
	u32 mpoint)
{
#if defined(MEASURE_TIME) && defined(MONITOR_TIME)
	unsigned int avg_cnt = debug_time_queue;
	struct fimc_is_video_ctx *vctx;
	struct fimc_is_device_ischain *ischain;
	struct fimc_is_video *video;
	unsigned long long temp;

	vctx = vctx_data;
	if (!vctx)
		return;

	ischain = GET_DEVICE_ISCHAIN(vctx);
	if (!ischain)
		return;

	video = vctx->video;
	if (!video)
		return;

	temp = sched_clock();

	if (mpoint == TMQ_DQ) {
		if (vctx->time[mpoint] == 0) {		/* Normal: DQ --> Q --> DQ --> Q */
			vctx->time[mpoint] = temp;
			vctx->time_cnt++;
		} else {				/* Abnormal: DQ --> DQ */
			minfo("[TIM] late Q(DQ --> DQ(%llu))\n", vctx,
				(temp - vctx->time[mpoint]) / 1000);
		}
	} else {
		if (vctx->time[mpoint - 1])  {		/* check pre-point */
			vctx->time[mpoint] = temp;
			vctx->time_total[mpoint] += vctx->time[mpoint] - vctx->time[mpoint - 1];

			if (mpoint == TMQ_QE) {		/* reset at final step */
				if (vctx->time_cnt % avg_cnt == 0) {
					minfo("[TIM][%s] DQ-Q(%05llu us), Q(%05llu us)\n",
						vctx, vctx->queue.name,
						vctx->time_total[TMQ_QS] / avg_cnt / 1000,
						vctx->time_total[TMQ_QE] / avg_cnt / 1000);
					vctx->time_total[TMQ_QS] = 0;
					vctx->time_total[TMQ_QE] = 0;
					vctx->time_cnt = 0;
				}

				vctx->time[TMQ_DQ] = 0;
			}
		}
	}
#endif
}
