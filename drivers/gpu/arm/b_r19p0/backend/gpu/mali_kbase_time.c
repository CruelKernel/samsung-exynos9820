/*
 *
 * (C) COPYRIGHT 2014-2016,2018 ARM Limited. All rights reserved.
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

#include <mali_kbase.h>
#include <mali_kbase_hwaccess_time.h>
#include <backend/gpu/mali_kbase_device_internal.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

void kbase_backend_get_gpu_time(struct kbase_device *kbdev, u64 *cycle_counter,
				u64 *system_time, struct timespec *ts)
{
	u32 hi1, hi2;

	kbase_pm_request_gpu_cycle_counter(kbdev);

	/* Read hi, lo, hi to ensure that overflow from lo to hi is handled
	 * correctly */
	do {
		hi1 = kbase_reg_read(kbdev, GPU_CONTROL_REG(CYCLE_COUNT_HI));
		*cycle_counter = kbase_reg_read(kbdev,
					GPU_CONTROL_REG(CYCLE_COUNT_LO));
		hi2 = kbase_reg_read(kbdev, GPU_CONTROL_REG(CYCLE_COUNT_HI));
		*cycle_counter |= (((u64) hi1) << 32);
	} while (hi1 != hi2);

	/* Read hi, lo, hi to ensure that overflow from lo to hi is handled
	 * correctly */
	do {
		hi1 = kbase_reg_read(kbdev, GPU_CONTROL_REG(TIMESTAMP_HI));
		*system_time = kbase_reg_read(kbdev,
					GPU_CONTROL_REG(TIMESTAMP_LO));
		hi2 = kbase_reg_read(kbdev, GPU_CONTROL_REG(TIMESTAMP_HI));
		*system_time |= (((u64) hi1) << 32);
	} while (hi1 != hi2);

	/* Record the CPU's idea of current time */
	getrawmonotonic(ts);

	kbase_pm_release_gpu_cycle_counter(kbdev);
}

/**
 * kbase_wait_write_flush -  Wait for GPU write flush
 * @kbdev: Kbase device
 *
 * Wait 1000 GPU clock cycles. This delay is known to give the GPU time to flush
 * its write buffer.
 *
 * Only in use for BASE_HW_ISSUE_6367
 *
 * Note : If GPU resets occur then the counters are reset to zero, the delay may
 * not be as expected.
 */
#ifndef CONFIG_MALI_NO_MALI
void kbase_wait_write_flush(struct kbase_device *kbdev)
{
	u32 base_count = 0;

	/*
	 * The caller must be holding onto the kctx or the call is from
	 * userspace.
	 */
	kbase_pm_context_active(kbdev);
	kbase_pm_request_gpu_cycle_counter(kbdev);

	while (true) {
		u32 new_count;

		new_count = kbase_reg_read(kbdev,
					GPU_CONTROL_REG(CYCLE_COUNT_LO));
		/* First time around, just store the count. */
		if (base_count == 0) {
			base_count = new_count;
			continue;
		}

		/* No need to handle wrapping, unsigned maths works for this. */
		if ((new_count - base_count) > 1000)
			break;
	}

	kbase_pm_release_gpu_cycle_counter(kbdev);
	kbase_pm_context_idle(kbdev);
}
#endif				/* CONFIG_MALI_NO_MALI */
