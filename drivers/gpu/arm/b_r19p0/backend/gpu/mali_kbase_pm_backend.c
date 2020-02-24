 /*
 *
 * (C) COPYRIGHT 2010-2019 ARM Limited. All rights reserved.
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
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */


/*
 * GPU backend implementation of base kernel power management APIs
 */

#include <mali_kbase.h>
#include <mali_midg_regmap.h>
#include <mali_kbase_config_defaults.h>

#include <mali_kbase_pm.h>
#include <mali_kbase_hwaccess_jm.h>
#include <mali_kbase_hwcnt_context.h>
#include <backend/gpu/mali_kbase_js_internal.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_jm_internal.h>

static void kbase_pm_gpu_poweroff_wait_wq(struct work_struct *data);
static void kbase_pm_hwcnt_disable_worker(struct work_struct *data);

int kbase_pm_runtime_init(struct kbase_device *kbdev)
{
	struct kbase_pm_callback_conf *callbacks;

	callbacks = (struct kbase_pm_callback_conf *)POWER_MANAGEMENT_CALLBACKS;
	if (callbacks) {
		kbdev->pm.backend.callback_power_on =
					callbacks->power_on_callback;
		kbdev->pm.backend.callback_power_off =
					callbacks->power_off_callback;
		kbdev->pm.backend.callback_power_suspend =
					callbacks->power_suspend_callback;
		kbdev->pm.backend.callback_power_resume =
					callbacks->power_resume_callback;
		kbdev->pm.callback_power_runtime_init =
					callbacks->power_runtime_init_callback;
		kbdev->pm.callback_power_runtime_term =
					callbacks->power_runtime_term_callback;
		kbdev->pm.backend.callback_power_runtime_on =
					callbacks->power_runtime_on_callback;
		kbdev->pm.backend.callback_power_runtime_off =
					callbacks->power_runtime_off_callback;
		kbdev->pm.backend.callback_power_runtime_idle =
					callbacks->power_runtime_idle_callback;
		/* MALI_SEC_INTEGRATION */
		kbdev->pm.backend.callback_power_dvfs_on =
					callbacks->power_dvfs_on_callback;

		if (callbacks->power_runtime_init_callback)
			return callbacks->power_runtime_init_callback(kbdev);
		else
			return 0;
	}

	kbdev->pm.backend.callback_power_on = NULL;
	kbdev->pm.backend.callback_power_off = NULL;
	kbdev->pm.backend.callback_power_suspend = NULL;
	kbdev->pm.backend.callback_power_resume = NULL;
	kbdev->pm.callback_power_runtime_init = NULL;
	kbdev->pm.callback_power_runtime_term = NULL;
	kbdev->pm.backend.callback_power_runtime_on = NULL;
	kbdev->pm.backend.callback_power_runtime_off = NULL;
	kbdev->pm.backend.callback_power_runtime_idle = NULL;

	return 0;
}

void kbase_pm_runtime_term(struct kbase_device *kbdev)
{
	if (kbdev->pm.callback_power_runtime_term) {
		kbdev->pm.callback_power_runtime_term(kbdev);
	}
}

void kbase_pm_register_access_enable(struct kbase_device *kbdev)
{
	struct kbase_pm_callback_conf *callbacks;

	callbacks = (struct kbase_pm_callback_conf *)POWER_MANAGEMENT_CALLBACKS;

	if (callbacks)
		callbacks->power_on_callback(kbdev);

	kbdev->pm.backend.gpu_powered = true;
}

void kbase_pm_register_access_disable(struct kbase_device *kbdev)
{
	struct kbase_pm_callback_conf *callbacks;

	callbacks = (struct kbase_pm_callback_conf *)POWER_MANAGEMENT_CALLBACKS;

	if (callbacks)
		callbacks->power_off_callback(kbdev);

	kbdev->pm.backend.gpu_powered = false;
}

int kbase_hwaccess_pm_early_init(struct kbase_device *kbdev)
{
	int ret = 0;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	mutex_init(&kbdev->pm.lock);

	kbdev->pm.backend.gpu_poweroff_wait_wq = alloc_workqueue("kbase_pm_poweroff_wait",
			WQ_HIGHPRI | WQ_UNBOUND, 1);
	if (!kbdev->pm.backend.gpu_poweroff_wait_wq)
		return -ENOMEM;

	INIT_WORK(&kbdev->pm.backend.gpu_poweroff_wait_work,
			kbase_pm_gpu_poweroff_wait_wq);

	kbdev->pm.backend.ca_cores_enabled = ~0ull;
	kbdev->pm.backend.gpu_powered = false;
	kbdev->pm.suspending = false;
	/* MALI_SEC_INTEGRATION */
	init_waitqueue_head(&kbdev->pm.suspending_wait);

#ifdef CONFIG_MALI_DEBUG
	kbdev->pm.backend.driver_ready_for_irqs = false;
#endif /* CONFIG_MALI_DEBUG */
	init_waitqueue_head(&kbdev->pm.backend.gpu_in_desired_state_wait);

	/* Initialise the metrics subsystem */
	ret = kbasep_pm_metrics_init(kbdev);
	if (ret)
		return ret;

	init_waitqueue_head(&kbdev->pm.backend.reset_done_wait);
	kbdev->pm.backend.reset_done = false;

	init_waitqueue_head(&kbdev->pm.zero_active_count_wait);
	kbdev->pm.active_count = 0;

	spin_lock_init(&kbdev->pm.backend.gpu_cycle_counter_requests_lock);

	init_waitqueue_head(&kbdev->pm.backend.poweroff_wait);

	if (kbase_pm_ca_init(kbdev) != 0)
		goto workq_fail;

	if (kbase_pm_policy_init(kbdev) != 0)
		goto pm_policy_fail;

	if (kbase_pm_state_machine_init(kbdev) != 0)
		goto pm_state_machine_fail;

	return 0;

pm_state_machine_fail:
	kbase_pm_policy_term(kbdev);
pm_policy_fail:
	kbase_pm_ca_term(kbdev);
workq_fail:
	kbasep_pm_metrics_term(kbdev);
	return -EINVAL;
}

int kbase_hwaccess_pm_late_init(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	kbdev->pm.backend.hwcnt_desired = false;
	kbdev->pm.backend.hwcnt_disabled = true;
	INIT_WORK(&kbdev->pm.backend.hwcnt_disable_work,
		kbase_pm_hwcnt_disable_worker);
	kbase_hwcnt_context_disable(kbdev->hwcnt_gpu_ctx);

	return 0;
}

void kbase_pm_do_poweron(struct kbase_device *kbdev, bool is_resume)
{
	lockdep_assert_held(&kbdev->pm.lock);

	/* Turn clocks and interrupts on - no-op if we haven't done a previous
	 * kbase_pm_clock_off() */
	kbase_pm_clock_on(kbdev, is_resume);

	if (!is_resume) {
		unsigned long flags;

		/* Force update of L2 state - if we have abandoned a power off
		 * then this may be required to power the L2 back on.
		 */
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		kbase_pm_update_state(kbdev);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	}

	/* Update core status as required by the policy */
	kbase_pm_update_cores_state(kbdev);

	/* NOTE: We don't wait to reach the desired state, since running atoms
	 * will wait for that state to be reached anyway */
}

static void kbase_pm_gpu_poweroff_wait_wq(struct work_struct *data)
{
	struct kbase_device *kbdev = container_of(data, struct kbase_device,
			pm.backend.gpu_poweroff_wait_work);
	struct kbase_pm_device_data *pm = &kbdev->pm;
	struct kbase_pm_backend_data *backend = &pm->backend;
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;
	unsigned long flags;

	if (!platform_power_down_only)
		/* Wait for power transitions to complete. We do this with no locks held
		 * so that we don't deadlock with any pending workqueues.
		 */
		kbase_pm_wait_for_desired_state(kbdev);

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);

	/* MALI_SEC_INTEGRATION */
	KBASE_TRACE_ADD(kbdev, KBASE_DEVICE_PM_WAIT_WQ_RUN, NULL, NULL, \
		backend->poweron_required, backend->poweroff_is_suspend);

	if (!backend->poweron_required) {
		if (!platform_power_down_only) {
			unsigned long flags;

			spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
			WARN_ON(backend->shaders_state != KBASE_SHADERS_OFF_CORESTACK_OFF ||
					backend->l2_state != KBASE_L2_OFF);
			spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
		}

		/* Disable interrupts and turn the clock off */
		if (!kbase_pm_clock_off(kbdev, backend->poweroff_is_suspend)) {
			/*
			 * Page/bus faults are pending, must drop locks to
			 * process.  Interrupts are disabled so no more faults
			 * should be generated at this point.
			 */
			mutex_unlock(&kbdev->pm.lock);
			mutex_unlock(&js_devdata->runpool_mutex);
			kbase_flush_mmu_wqs(kbdev);
			mutex_lock(&js_devdata->runpool_mutex);
			mutex_lock(&kbdev->pm.lock);

			/* Turn off clock now that fault have been handled. We
			 * dropped locks so poweron_required may have changed -
			 * power back on if this is the case (effectively only
			 * re-enabling of the interrupts would be done in this
			 * case, as the clocks to GPU were not withdrawn yet).
			 */
			if (backend->poweron_required)
				kbase_pm_clock_on(kbdev, false);
			else
				WARN_ON(!kbase_pm_clock_off(kbdev,
						backend->poweroff_is_suspend));
		}
	}

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	backend->poweroff_wait_in_progress = false;
	if (backend->poweron_required) {
		backend->poweron_required = false;
		kbdev->pm.backend.l2_desired = true;
		kbase_pm_update_state(kbdev);
		kbase_pm_update_cores_state_nolock(kbdev);
		kbase_backend_slot_update(kbdev);
	}
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);

	wake_up(&kbdev->pm.backend.poweroff_wait);
}

static void kbase_pm_hwcnt_disable_worker(struct work_struct *data)
{
	struct kbase_device *kbdev = container_of(data, struct kbase_device,
			pm.backend.hwcnt_disable_work);
	struct kbase_pm_device_data *pm = &kbdev->pm;
	struct kbase_pm_backend_data *backend = &pm->backend;
	unsigned long flags;

	bool do_disable;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	do_disable = !backend->hwcnt_desired && !backend->hwcnt_disabled;
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	if (!do_disable)
		return;

	kbase_hwcnt_context_disable(kbdev->hwcnt_gpu_ctx);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	do_disable = !backend->hwcnt_desired && !backend->hwcnt_disabled;

	if (do_disable) {
		/* PM state did not change while we were doing the disable,
		 * so commit the work we just performed and continue the state
		 * machine.
		 */
		backend->hwcnt_disabled = true;
		kbase_pm_update_state(kbdev);
		kbase_backend_slot_update(kbdev);
	} else {
		/* PM state was updated while we were doing the disable,
		 * so we need to undo the disable we just performed.
		 */
		kbase_hwcnt_context_enable(kbdev->hwcnt_gpu_ctx);
	}

	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void kbase_pm_do_poweroff(struct kbase_device *kbdev, bool is_suspend)
{
	unsigned long flags;

	lockdep_assert_held(&kbdev->pm.lock);

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);

	if (!kbdev->pm.backend.gpu_powered)
		goto unlock_hwaccess;

	if (kbdev->pm.backend.poweroff_wait_in_progress)
		goto unlock_hwaccess;

	/* Force all cores off */
	kbdev->pm.backend.shaders_desired = false;
	kbdev->pm.backend.l2_desired = false;

	kbdev->pm.backend.poweroff_wait_in_progress = true;
	kbdev->pm.backend.poweroff_is_suspend = is_suspend;
	kbdev->pm.backend.invoke_poweroff_wait_wq_when_l2_off = true;

	/* l2_desired being false should cause the state machine to
	 * start powering off the L2. When it actually is powered off,
	 * the interrupt handler will call kbase_pm_l2_update_state()
	 * again, which will trigger the kbase_pm_gpu_poweroff_wait_wq.
	 * Callers of this function will need to wait on poweroff_wait.
	 */
	kbase_pm_update_state(kbdev);

unlock_hwaccess:
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

static bool is_poweroff_in_progress(struct kbase_device *kbdev)
{
	bool ret;
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	ret = (kbdev->pm.backend.poweroff_wait_in_progress == false);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	return ret;
}

void kbase_pm_wait_for_poweroff_complete(struct kbase_device *kbdev)
{
	wait_event_killable(kbdev->pm.backend.poweroff_wait,
			is_poweroff_in_progress(kbdev));
}

int kbase_hwaccess_pm_powerup(struct kbase_device *kbdev,
		unsigned int flags)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;
	unsigned long irq_flags;
	int ret;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);

	/* A suspend won't happen during startup/insmod */
	KBASE_DEBUG_ASSERT(!kbase_pm_is_suspending(kbdev));

	/* Power up the GPU, don't enable IRQs as we are not ready to receive
	 * them. */
	ret = kbase_pm_init_hw(kbdev, flags);
	if (ret) {
		mutex_unlock(&kbdev->pm.lock);
		mutex_unlock(&js_devdata->runpool_mutex);
		return ret;
	}

	kbdev->pm.debug_core_mask_all = kbdev->pm.debug_core_mask[0] =
			kbdev->pm.debug_core_mask[1] =
			kbdev->pm.debug_core_mask[2] =
			kbdev->gpu_props.props.raw_props.shader_present;

	/* Pretend the GPU is active to prevent a power policy turning the GPU
	 * cores off */
	kbdev->pm.active_count = 1;

	spin_lock_irqsave(&kbdev->pm.backend.gpu_cycle_counter_requests_lock,
								irq_flags);
	/* Ensure cycle counter is off */
	kbdev->pm.backend.gpu_cycle_counter_requests = 0;
	spin_unlock_irqrestore(
			&kbdev->pm.backend.gpu_cycle_counter_requests_lock,
								irq_flags);

	/* We are ready to receive IRQ's now as power policy is set up, so
	 * enable them now. */
#ifdef CONFIG_MALI_DEBUG
	kbdev->pm.backend.driver_ready_for_irqs = true;
#endif
	kbase_pm_enable_interrupts(kbdev);

	/* Turn on the GPU and any cores needed by the policy */
	kbase_pm_do_poweron(kbdev, false);
	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);

	return 0;
}

void kbase_hwaccess_pm_halt(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	mutex_lock(&kbdev->pm.lock);
	kbase_pm_do_poweroff(kbdev, false);
	mutex_unlock(&kbdev->pm.lock);
}

KBASE_EXPORT_TEST_API(kbase_hwaccess_pm_halt);

void kbase_hwaccess_pm_early_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(kbdev->pm.active_count == 0);
	KBASE_DEBUG_ASSERT(kbdev->pm.backend.gpu_cycle_counter_requests == 0);

	/* Free any resources the policy allocated */
	kbase_pm_state_machine_term(kbdev);
	kbase_pm_policy_term(kbdev);
	kbase_pm_ca_term(kbdev);

	/* Shut down the metrics subsystem */
	kbasep_pm_metrics_term(kbdev);

	destroy_workqueue(kbdev->pm.backend.gpu_poweroff_wait_wq);
}

void kbase_hwaccess_pm_late_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);

	cancel_work_sync(&kbdev->pm.backend.hwcnt_disable_work);

	if (kbdev->pm.backend.hwcnt_disabled) {
		unsigned long flags;

		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		kbase_hwcnt_context_enable(kbdev->hwcnt_gpu_ctx);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	}
}

void kbase_pm_power_changed(struct kbase_device *kbdev)
{
	unsigned long flags;

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	kbase_pm_update_state(kbdev);

	kbase_backend_slot_update(kbdev);

	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
}

void kbase_pm_set_debug_core_mask(struct kbase_device *kbdev,
		u64 new_core_mask_js0, u64 new_core_mask_js1,
		u64 new_core_mask_js2)
{
	kbdev->pm.debug_core_mask[0] = new_core_mask_js0;
	kbdev->pm.debug_core_mask[1] = new_core_mask_js1;
	kbdev->pm.debug_core_mask[2] = new_core_mask_js2;
	kbdev->pm.debug_core_mask_all = new_core_mask_js0 | new_core_mask_js1 |
			new_core_mask_js2;

	kbase_pm_update_cores_state_nolock(kbdev);
}

void kbase_hwaccess_pm_gpu_active(struct kbase_device *kbdev)
{
	kbase_pm_update_active(kbdev);
}

void kbase_hwaccess_pm_gpu_idle(struct kbase_device *kbdev)
{
	kbase_pm_update_active(kbdev);
}

void kbase_hwaccess_pm_suspend(struct kbase_device *kbdev)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;

	/* Force power off the GPU and all cores (regardless of policy), only
	 * after the PM active count reaches zero (otherwise, we risk turning it
	 * off prematurely) */
	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);

	kbase_pm_do_poweroff(kbdev, true);

	kbase_backend_timer_suspend(kbdev);

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);

	kbase_pm_wait_for_poweroff_complete(kbdev);

	/* MALI_SEC_INTEGRATION */
	KBASE_TRACE_ADD(kbdev, KBASE_DEVICE_PM_SUSPEND, NULL, NULL, 0u, 0u);
}

void kbase_hwaccess_pm_resume(struct kbase_device *kbdev)
{
	struct kbasep_js_device_data *js_devdata = &kbdev->js_data;

	mutex_lock(&js_devdata->runpool_mutex);
	mutex_lock(&kbdev->pm.lock);

	kbdev->pm.suspending = false;
	kbase_pm_do_poweron(kbdev, true);
	/* MALI_SEC_INTEGRATION */
	wake_up(&kbdev->pm.suspending_wait);

	kbase_backend_timer_resume(kbdev);

	mutex_unlock(&kbdev->pm.lock);
	mutex_unlock(&js_devdata->runpool_mutex);

	/* MALI_SEC_INTEGRATION */
	KBASE_TRACE_ADD(kbdev, KBASE_DEVICE_PM_RESUME, NULL, NULL, 0u, 0u);
}
