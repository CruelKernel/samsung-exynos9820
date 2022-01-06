/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/slab.h>

#include <gpex_dvfs.h>
#include <gpex_utils.h>
#include <gpex_clock.h>
#include <gpex_pm.h>

#include <gpexbe_pm.h>
#include <gpexbe_devicetree.h>
#include <gpexbe_utilization.h>
#include <gpex_qos.h>

#include "gpu_dvfs_governor.h"
#include "gpex_dvfs_internal.h"

struct dvfs_info dvfs;

static int gpu_dvfs_handler_init(void);
static int gpu_dvfs_handler_deinit(void);

static void gpex_dvfs_context_init(struct device **dev)
{
	int i;
	const char *of_string;
	gpu_dt *dt = gpexbe_devicetree_get_gpu_dt();
	dvfs.kbdev = container_of(dev, struct kbase_device, dev);
	dvfs.table_size = dt->gpu_dvfs_table_size.row;
	dvfs.table = kcalloc(dvfs.table_size, sizeof(*dvfs.table), GFP_KERNEL);

	for (i = 0; i < dvfs.table_size; i++) {
		dvfs.table[i].clock = dt->gpu_dvfs_table[i].clock;
		dvfs.table[i].voltage = 0; // get it from gpex_clock
		dvfs.table[i].min_threshold = dt->gpu_dvfs_table[i].min_threshold;
		dvfs.table[i].max_threshold = dt->gpu_dvfs_table[i].max_threshold;
		dvfs.table[i].down_staycount = dt->gpu_dvfs_table[i].down_staycount;
	}

	of_string = gpexbe_devicetree_get_str(governor);

	if (!strncmp("interactive", of_string, strlen("interactive"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_INTERACTIVE;
		dvfs.interactive.highspeed_clock =
			gpexbe_devicetree_get_int(interactive_info.highspeed_clock);
		dvfs.interactive.highspeed_load =
			gpexbe_devicetree_get_int(interactive_info.highspeed_load);
		dvfs.interactive.highspeed_delay =
			gpexbe_devicetree_get_int(interactive_info.highspeed_delay);
	} else if (!strncmp("joint", of_string, strlen("joint"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_JOINT;
	} else if (!strncmp("static", of_string, strlen("static"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_STATIC;
	} else if (!strncmp("booster", of_string, strlen("booster"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_BOOSTER;
	} else if (!strncmp("dynamic", of_string, strlen("dynamic"))) {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_DYNAMIC;
	} else {
		dvfs.governor_type = G3D_DVFS_GOVERNOR_DEFAULT;
	}

	for (i = 0; i < G3D_MAX_GOVERNOR_NUM; i++) {
		gpu_dvfs_update_start_clk(i, gpex_clock_get_boot_clock());
	}

	dvfs.gpu_dvfs_config_clock = gpexbe_devicetree_get_int(gpu_dvfs_bl_config_clock);
	dvfs.polling_speed = gpexbe_devicetree_get_int(gpu_dvfs_polling_time);
}

static int gpu_dvfs_calculate_env_data()
{
	unsigned long flags;
	static int polling_period;

	spin_lock_irqsave(&dvfs.spinlock, flags);
	dvfs.env_data.utilization = gpexbe_utilization_calc_utilization();
	gpexbe_utilization_calculate_compute_ratio();
	spin_unlock_irqrestore(&dvfs.spinlock, flags);

	polling_period -= dvfs.polling_speed;
	if (polling_period > 0)
		return 0;

	return 0;
}

static int kbase_platform_dvfs_event(u32 utilisation)
{
	/* TODO: find out why this is needed and add back if necessary */
#if 0
	char *env[2] = {"FEATURE=GPUI", NULL};
	if(platform->fault_count >= 5 && platform->bigdata_uevent_is_sent == false)
	{
		platform->bigdata_uevent_is_sent = true;
		kobject_uevent_env(&kbdev->dev->kobj, KOBJ_CHANGE, env);
	}
#endif

	mutex_lock(&dvfs.handler_lock);
	if (gpex_pm_get_status(true)) {
		int clk = 0;
		gpu_dvfs_calculate_env_data();
		clk = gpu_dvfs_decide_next_freq(dvfs.env_data.utilization);
		gpex_clock_set(clk);
	}
	mutex_unlock(&dvfs.handler_lock);

	GPU_LOG(MALI_EXYNOS_DEBUG, "dvfs hanlder is called\n");

	return 0;
}

static void dvfs_callback(struct work_struct *data)
{
	unsigned long flags;

	//KBASE_DEBUG_ASSERT(data != NULL);

	kbase_platform_dvfs_event(0);

	spin_lock_irqsave(&dvfs.spinlock, flags);

	if (dvfs.timer_active)
		queue_delayed_work_on(0, dvfs.dvfs_wq, &dvfs.dvfs_work,
				      msecs_to_jiffies(dvfs.polling_speed));

	spin_unlock_irqrestore(&dvfs.spinlock, flags);
}

static void gpu_dvfs_timer_control(bool timer_state)
{
	unsigned long flags;

	if (!dvfs.status) {
		GPU_LOG(MALI_EXYNOS_ERROR, "%s: DVFS is disabled\n", __func__);
		return;
	}
	if (dvfs.timer_active && !timer_state) {
		cancel_delayed_work(&dvfs.dvfs_work);
		flush_workqueue(dvfs.dvfs_wq);
	} else if (!dvfs.timer_active && timer_state) {
		gpex_qos_set_from_clock(gpex_clock_get_cur_clock());

		queue_delayed_work_on(0, dvfs.dvfs_wq, &dvfs.dvfs_work,
				      msecs_to_jiffies(dvfs.polling_speed));
		spin_lock_irqsave(&dvfs.spinlock, flags);
		dvfs.down_requirement = dvfs.table[dvfs.step].down_staycount;
		dvfs.interactive.delay_count = 0;
		spin_unlock_irqrestore(&dvfs.spinlock, flags);
	}

	spin_lock_irqsave(&dvfs.spinlock, flags);
	dvfs.timer_active = timer_state;
	spin_unlock_irqrestore(&dvfs.spinlock, flags);
}

void gpex_dvfs_start()
{
	gpu_dvfs_timer_control(true);
}

void gpex_dvfs_stop()
{
	gpu_dvfs_timer_control(false);
}
/* TODO */
/* MIN_POLLING_SPEED is dependant to Operating System(kernel timer, HZ) */
/* it is rarely changed from 4 to other values, but it would be better to make it follow kernel env */
int gpex_dvfs_set_polling_speed(int polling_speed)
{
	if ((polling_speed < GPEX_DVFS_MIN_POLLING_SPEED) ||
	    (polling_speed > GPEX_DVFS_MAX_POLLING_SPEED)) {
		GPU_LOG(MALI_EXYNOS_WARNING, "%s: out of range [4~1000] (%d)\n", __func__,
			polling_speed);
		return -ENOENT;
	}

	dvfs.polling_speed = polling_speed;

	return 0;
}

static int gpu_dvfs_on_off(bool enable)
{
	/* TODO get proper return values from gpu_dvfs_handler_init */
	if (enable && !dvfs.status) {
		mutex_lock(&dvfs.handler_lock);
		gpex_clock_set(gpex_clock_get_cur_clock());
		gpu_dvfs_handler_init();
		mutex_unlock(&dvfs.handler_lock);

		gpex_dvfs_start();
	} else if (!enable && dvfs.status) {
		gpex_dvfs_stop();

		mutex_lock(&dvfs.handler_lock);
		gpu_dvfs_handler_deinit();
		gpex_clock_set(dvfs.gpu_dvfs_config_clock);
		mutex_unlock(&dvfs.handler_lock);
	}

	return 0;
}

int gpex_dvfs_enable()
{
	return gpu_dvfs_on_off(true);
}

int gpex_dvfs_disable()
{
	return gpu_dvfs_on_off(false);
}

static int gpu_dvfs_handler_init()
{
	if (!dvfs.status)
		dvfs.status = true;

	gpex_clock_set(dvfs.table[dvfs.step].clock);

	dvfs.timer_active = true;

	GPU_LOG(MALI_EXYNOS_INFO, "dvfs handler initialized\n");
	return 0;
}

static int gpu_dvfs_handler_deinit()
{
	if (dvfs.status)
		dvfs.status = false;

	dvfs.timer_active = false;

	GPU_LOG(MALI_EXYNOS_INFO, "dvfs handler de-initialized\n");
	return 0;
}

static int gpu_pm_metrics_init()
{
	INIT_DELAYED_WORK(&dvfs.dvfs_work, dvfs_callback);
	dvfs.dvfs_wq = create_workqueue("g3d_dvfs");

	queue_delayed_work_on(0, dvfs.dvfs_wq, &dvfs.dvfs_work,
			      msecs_to_jiffies(dvfs.polling_speed));

	return 0;
}

static void gpu_pm_metrics_term()
{
	cancel_delayed_work(&dvfs.dvfs_work);
	flush_workqueue(dvfs.dvfs_wq);
	destroy_workqueue(dvfs.dvfs_wq);
}

int gpex_dvfs_init(struct device **dev)
{
	spin_lock_init(&dvfs.spinlock);
	mutex_init(&dvfs.handler_lock);

	gpex_dvfs_context_init(dev);

	gpu_dvfs_governor_init(&dvfs);

	gpu_dvfs_handler_init();

	/* dvfs wq start */
	gpu_pm_metrics_init();

	gpex_dvfs_sysfs_init(&dvfs);

	gpex_dvfs_external_init(&dvfs);

	return 0;
}

void gpex_dvfs_term()
{
	/* DVFS stuff */
	gpu_pm_metrics_term();
	gpu_dvfs_handler_deinit();
	dvfs.kbdev = NULL;
}

int gpex_dvfs_get_status()
{
	return dvfs.status;
}

int gpex_dvfs_get_governor_type(void)
{
	return dvfs.governor_type;
}

int gpex_dvfs_get_utilization(void)
{
	return dvfs.env_data.utilization;
}

int gpex_dvfs_get_polling_speed(void)
{
	return dvfs.polling_speed;
}

void gpex_dvfs_spin_lock(unsigned long *flags)
{
	spin_lock_irqsave(&dvfs.spinlock, *flags);
}

void gpex_dvfs_spin_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&dvfs.spinlock, *flags);
}
