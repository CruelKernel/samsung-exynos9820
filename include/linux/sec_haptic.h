/* sec_haptic.h
 *
 * Copyright (C) 2017 Samsung Electronics
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SEC_HAPTIC_H
#define SEC_HAPTIC_H

#include <linux/kthread.h>

#define MAX_INTENSITY			10000
#define PACKET_MAX_SIZE		1000
#define MULTI_FREQ_DIVIDER		78125000 /* 1s/128(pdiv)=7812500 */

#define HAPTIC_ENGINE_FREQ_MIN		1200
#define HAPTIC_ENGINE_FREQ_MAX		3500

#define VIB_BUFSIZE                     30

#define BOOST_ON                        1
#define BOOST_OFF                       0

#define HOMEKEY_PRESS_FREQ		5
#define HOMEKEY_RELEASE_FREQ		6
#define HOMEKEY_DURATION			7

struct vib_packet {
	int time;
	int intensity;
	int freq;
	int overdrive;
};

enum {
	VIB_PACKET_TIME = 0,
	VIB_PACKET_INTENSITY,
	VIB_PACKET_FREQUENCY,
	VIB_PACKET_OVERDRIVE,
	VIB_PACKET_MAX,
};

struct sec_haptic_drvdata {
	struct class *to_class;
	struct device *to_dev;
	struct device *dev;
	struct hrtimer timer;
	struct kthread_worker kworker;
	struct kthread_work kwork;
	struct mutex mutex;
	struct vib_packet test_pac[PACKET_MAX_SIZE];
	void *data;
	bool f_packet_en;
	bool packet_running;
	int force_touch_intensity;
	int ratio;
	int normal_ratio;
	int overdrive_ratio;
	int multi_frequency;
	int packet_size;
	int packet_cnt;
	int freq_num;
	const char *vib_type;
	u16 max_timeout;
	u32 period;
	u32 intensity;
	u32 timeout;
	u32 max_duty;
	u32 duty;
	u32 *multi_freq_duty;
	u32 *multi_freq_period;

	int (*set_intensity)(void *data, u32 duty);
	int (*enable)(void *data, bool en);
	int (*boost)(void *data, int control);
};

extern int sec_haptic_register(struct sec_haptic_drvdata *ddata);
extern int sec_haptic_unregister(struct sec_haptic_drvdata *ddata);

#endif /* SEC_HAPTIC_H */
