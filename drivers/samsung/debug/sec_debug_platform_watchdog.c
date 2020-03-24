/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * Samsung debug platform watchdog
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>
#include <linux/tick.h>
#include <linux/file.h>
#include <linux/sec_class.h>
#include <linux/sec_debug.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/sec_batt.h>
#include <linux/freezer.h>
#include <linux/moduleparam.h>
#include <linux/sched/task.h>

/* Override the default prefix for the compatibility with other models */ 
#undef MODULE_PARAM_PREFIX 
#define MODULE_PARAM_PREFIX "sec_debug." 

#define SEC_DEBUG_MAX_PWDT_RESTART_CNT 20	//200 seconds
#define SEC_DEBUG_MAX_PWDT_SYNC_CNT 40	//400 seconds
#define SEC_DEBUG_MAX_PWDT_INIT_CNT 200	//2000 seconds

static unsigned long pwdt_start_ms = 0;
module_param_named(pwdt_start_ms, pwdt_start_ms, ulong, 0644);

static unsigned long pwdt_end_ms = 0;
module_param_named(pwdt_end_ms, pwdt_end_ms, ulong, 0644);

static unsigned int pwdt_pid = 0;
module_param_named(pwdt_pid, pwdt_pid, uint, 0644);

static unsigned long pwdt_sync_cnt = 0;
module_param_named(pwdt_sync_cnt, pwdt_sync_cnt, ulong, 0644);

static void sec_platform_watchdog_timer_fn(unsigned long data);

static unsigned long sample_period = 10;
static struct timer_list sec_platform_watchdog_timer =
	TIMER_DEFERRED_INITIALIZER(sec_platform_watchdog_timer_fn, 0, 0);

extern int bootmode;
static unsigned int __is_verifiedboot_state;

static int verifiedboot_state_param(char *str)
{
	static const char unlocked[] = "orange";

	if (!str)
		return -EINVAL;

	if (strncmp(str, unlocked, sizeof(unlocked)) == 0)
		__is_verifiedboot_state = 1;
	else
		__is_verifiedboot_state = 0;

	return __is_verifiedboot_state;
}
early_param("androidboot.verifiedbootstate", verifiedboot_state_param);

static void sec_debug_check_pwdt(void)
{
	struct task_struct *wdt_tsk = NULL;

	static unsigned long pwdt_sync_delay = 0;
	static unsigned long pwdt_restart_delay = 0;
	static unsigned long pwdt_init_delay = 0;
	static unsigned long last_sync_cnt = 0;

	if (bootmode == 2 || lpcharge || __is_verifiedboot_state) {
		pr_info("%s skip to check pwdt\n" , __func__);
		return;
	}

	pr_info("pid[%d], start_ms[%ld], sync_cnt[%ld], restart_delay[%ld], init_dealy[%ld], sync_delay[%ld]\n",
		pwdt_pid, pwdt_start_ms, pwdt_sync_cnt, pwdt_restart_delay, pwdt_init_delay, pwdt_sync_delay);

	/* when pwdt is not initialized */
	if (pwdt_pid == 0 && pwdt_start_ms == 0)
	{
		/* more than 2000secs */
		if (pwdt_init_delay++ >= SEC_DEBUG_MAX_PWDT_INIT_CNT) {
			panic("Platform Watchdog couldnot be initialized");
		}
	}
	/* when pwdt is killed */
	else if (pwdt_pid != 0 && pwdt_start_ms == 0)
	{
		if (pwdt_restart_delay == 0)
			pr_info("pwdt has been killed!!, start_ms[%ld], end_ms[%ld]\n", pwdt_start_ms, pwdt_end_ms);

		/* if pwdt cannot be restarted
		 * after 200 seconds since pwdt has been killed,
		 * kernel watchdog will trigger Panic
		 */
		if (pwdt_restart_delay++ >= SEC_DEBUG_MAX_PWDT_RESTART_CNT) {
			panic("Platform Watchdog couldnot be restarted");
		}
	}
	/* when pwdt is alive */
	else {
		pwdt_init_delay = 0;
		pwdt_restart_delay = 0;
		rcu_read_lock();

		wdt_tsk = find_task_by_vpid(pwdt_pid);

		/* if cannot find platform watchdog thread, 
		 * it might be killed by system_crash or zygote, We ignored this case.
		 */
		if (wdt_tsk == NULL) {
			rcu_read_unlock();
			last_sync_cnt = pwdt_sync_cnt;
			pwdt_sync_delay = 0;
			pr_info("cannot find watchdog thread!!\n");
			return;
		}

		get_task_struct(wdt_tsk);
		rcu_read_unlock();

		if (unlikely(frozen(wdt_tsk) || freezing(wdt_tsk))) {
			/* clear delay if watchdog thread is frozen or freezing */
			pr_info("wdt_task is frozen : [%d],[%d]\n", frozen(wdt_tsk), freezing(wdt_tsk));
			last_sync_cnt = pwdt_sync_cnt;
			pwdt_sync_delay = 0;
		}
		else {
			if (last_sync_cnt == pwdt_sync_cnt) {
				/* pwdt_sync_cnt is updated in every 30s,
				 * but sec_debug_check_pwdt is invoked in every 10s
				 * kernel watchdog will trigger Panic 
				 * if platform watchdog couldnot update sync_cnt for 400secs
				 */
				if (pwdt_sync_delay++ >= SEC_DEBUG_MAX_PWDT_SYNC_CNT) {
					put_task_struct(wdt_tsk);
					panic("Platform Watchdog can't update sync_cnt");
					return;
				}
			}
			else {
				last_sync_cnt = pwdt_sync_cnt;
				pwdt_sync_delay = 0;
			}
		}
		put_task_struct(wdt_tsk);
	}

	return;
}

static void sec_platform_watchdog_timer_fn(unsigned long data)
{
	sec_debug_check_pwdt();
	mod_timer(&sec_platform_watchdog_timer, jiffies + sample_period * HZ);	
}

static void sec_platform_watchdog_start_timer(void)
{
	del_timer_sync(&sec_platform_watchdog_timer);
	mod_timer(&sec_platform_watchdog_timer, jiffies + sample_period * HZ);
}

static int sec_platform_watchdog_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		pr_info("%s suspend prepare\n", __func__);
		del_timer_sync(&sec_platform_watchdog_timer);

		break;
	case PM_POST_SUSPEND:
		pr_info("%s post suspend\n", __func__);
		mod_timer(&sec_platform_watchdog_timer, jiffies + sample_period * HZ);
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block sec_platform_watchdog_notif_block = {
	.notifier_call = sec_platform_watchdog_notifier,
};

static int __init sec_platform_watchdog_init(void)
{
	sec_platform_watchdog_start_timer();
	register_pm_notifier(&sec_platform_watchdog_notif_block);
	return 0;
}

late_initcall(sec_platform_watchdog_init);
