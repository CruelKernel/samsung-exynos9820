/*
 * Copyright (c) 2018 Park Bumgyu, Samsung Electronics Co., Ltd <bumgyu.park@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef CPUIDLE_PROFILE_H
#define CPUIDLE_PROFILE_H __FILE__

#include <linux/ktime.h>
#include <linux/cpuidle.h>

#include <asm/cputype.h>

extern void cpuidle_profile_cpu_idle_enter(int cpu, int index);
extern void cpuidle_profile_cpu_idle_exit(int cpu, int index, int cancel);
extern void cpuidle_profile_cpu_idle_register(struct cpuidle_driver *drv);
extern void cpuidle_profile_group_idle_enter(int id);
extern void cpuidle_profile_group_idle_exit(int id, int cancel);
extern void cpuidle_profile_group_idle_register(int id, const char *name);
extern void cpuidle_profile_idle_ip(int index, unsigned int idle_ip);

#endif /* CPUIDLE_PROFILE_H */
