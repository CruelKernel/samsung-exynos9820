/*
 *  linux/include/linux/gpu_cooling.h
 *
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *  Copyright (C) 2012  Amit Daniel <amit.kachhap@linaro.org>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef __GPU_COOLING_H__
#define __GPU_COOLING_H__

#include <linux/of.h>
#include <linux/thermal.h>
#include <linux/cpumask.h>
#include <linux/platform_device.h>

#define GPU_TABLE_END     ~1

#ifdef CONFIG_GPU_THERMAL
/**
 * gpufreq_cooling_register - function to create gpufreq cooling device.
 * @clip_gpus: cpumask of gpus where the frequency constraints will happen
 */
struct thermal_cooling_device *
gpufreq_cooling_register(const struct cpumask *clip_gpus);

struct thermal_cooling_device *
gpufreq_power_cooling_register(const struct cpumask *clip_gpus,
			       u32 capacitance);

/**
 * of_gpufreq_cooling_register - create gpufreq cooling device based on DT.
 * @np: a valid struct device_node to the cooling device device tree node.
 * @clip_gpus: cpumask of gpus where the frequency constraints will happen
 */
#ifdef CONFIG_THERMAL_OF
struct thermal_cooling_device *
of_gpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_gpus);

struct thermal_cooling_device *
of_gpufreq_power_cooling_register(struct device_node *np,
				  const struct cpumask *clip_gpus,
				  u32 capacitance);
#else
static inline struct thermal_cooling_device *
of_gpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_gpus)
{
	return NULL;
}

static inline struct thermal_cooling_device *
of_gpufreq_power_cooling_register(struct device_node *np,
				  const struct cpumask *clip_gpus,
				  u32 capacitance);
{
	return NULL;
}
#endif

/**
 * gpufreq_cooling_unregister - function to remove gpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 */
void gpufreq_cooling_unregister(struct thermal_cooling_device *cdev);

unsigned long gpufreq_cooling_get_level(unsigned int gpu, unsigned int freq);
#else /* !CONFIG_GPU_THERMAL */
static inline struct thermal_cooling_device *
gpufreq_cooling_register(const struct cpumask *clip_gpus)
{
	return NULL;
}

static inline struct thermal_cooling_device *
gpufreq_power_cooling_register(const struct cpumask *clip_gpus,
			       u32 capacitance)
{
	return NULL;
}

static inline struct thermal_cooling_device *
of_gpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_gpus)
{
	return NULL;
}

static inline struct thermal_cooling_device *
of_gpufreq_power_cooling_register(struct device_node *np,
				  const struct cpumask *clip_gpus,
				  u32 capacitance)
{
	return NULL;
}

static inline
void gpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	return;
}
static inline
unsigned long gpufreq_cooling_get_level(unsigned int gpu, unsigned int freq)
{
	return THERMAL_CSTATE_INVALID;
}
#endif	/* CONFIG_GPU_THERMAL */
#ifdef CONFIG_MALI_DVFS
extern int gpu_dvfs_get_clock(int level);
extern int gpu_dvfs_get_voltage(int clock);
extern int gpu_dvfs_get_step(void);
extern int gpu_dvfs_get_cur_clock(void);
extern int gpu_dvfs_get_utilization(void);
extern int gpu_dvfs_get_max_freq(void);
#else
static inline int gpu_dvfs_get_clock(int level) { return 0; }
static inline int gpu_dvfs_get_voltage(int clock) { return 0; }
static inline int gpu_dvfs_get_step(void) { return 0; }
static inline int gpu_dvfs_get_cur_clock(void) { return 0; }
static inline int gpu_dvfs_get_utilization(void) { return 0; }
static inline int gpu_dvfs_get_max_freq(void) { return 0; }
#endif
#endif /* __GPU_COOLING_H__ */
