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

#include <linux/notifier.h>
#include <soc/samsung/tmu.h>

#include <gpexbe_notifier.h>
#include <gpex_thermal.h>
#include <gpex_utils.h>
#include <gpex_clock.h>

#include "gpexbe_notifier_internal.h"

/**
 * tmu_notifier() - Set gpu clock depending on TMU event.
 *
 * @notifier: unused.
 * @event: GPU temperature status.
 * @v: pointer to int value containing GPU target frequency.
 *
 * Callback function meant to be stored in notifier_block structure and registered
 * to exynos's TMU notifier. Sets the GPU clock depending on the GPU temperature
 * status and the target frequency it received as parameters. Some versions of
 * Exynos TMU calls this function directly, so this function should not be renamed.
 *
 * Return: NOTIFY_OK on success or if GPU TMU is disabled.
 */
static int tmu_notifier(struct notifier_block *notifier, unsigned long event, void *freqp)
{
	int frequency;

	frequency = *(int *)freqp;

	switch (event) {
	case GPU_NORMAL:
	case GPU_COLD:
		gpex_thermal_gpu_normal();
		break;
	case GPU_THROTTLING:
	case GPU_TRIPPING:
	default:
		gpex_thermal_gpu_throttle(frequency);
	}

	return NOTIFY_OK;
}

int gpu_tmu_notifier(struct notifier_block *notifier, unsigned long event, void *freqp)
{
	return tmu_notifier(notifier, event, freqp);
}
EXPORT_SYMBOL_GPL(gpu_tmu_notifier);

static int gpu_pmqos_min_notifier(struct notifier_block *nb, unsigned long val, void *v)
{
	GPU_LOG(MALI_EXYNOS_DEBUG, "%s: GPU pmqos min lock %ld kHz\n", __func__, val);

	if (val > 0)
		gpex_clock_lock_clock(GPU_CLOCK_MIN_LOCK, PMQOS_LOCK, val);
	else
		gpex_clock_lock_clock(GPU_CLOCK_MIN_UNLOCK, PMQOS_LOCK, 0);

	return NOTIFY_OK;
}

static int gpu_pmqos_max_notifier(struct notifier_block *nb, unsigned long val, void *v)
{
	GPU_LOG(MALI_EXYNOS_DEBUG, "%s: GPU pmqos max lock %ld kHz\n", __func__, val);

	if (val > 0)
		gpex_clock_lock_clock(GPU_CLOCK_MAX_LOCK, PMQOS_LOCK, val);
	else
		gpex_clock_lock_clock(GPU_CLOCK_MAX_UNLOCK, PMQOS_LOCK, 0);

	return NOTIFY_OK;
}

static struct notifier_block gpu_min_qos_notifier = {
	.notifier_call = gpu_pmqos_min_notifier,
	.priority = INT_MAX,
};

static struct notifier_block gpu_max_qos_notifier = {
	.notifier_call = gpu_pmqos_max_notifier,
	.priority = INT_MAX,
};

static struct notifier_block gpu_tmu_nb = {
	.notifier_call = tmu_notifier,
};

int gpexbe_notifier_init()
{
	int ret = 0;

	ret = gpexbe_notifier_internal_add(GPU_NOTIFIER_THERMAL, &gpu_tmu_nb);
	if (ret) {
		/* TODO: error message. could not register gpu thermal notifier cb */
		;
	} else
		gpex_thermal_set_status(true);

	gpexbe_notifier_internal_add(GPU_NOTIFIER_MIN_LOCK, &gpu_min_qos_notifier);
	gpexbe_notifier_internal_add(GPU_NOTIFIER_MAX_LOCK, &gpu_max_qos_notifier);

	return ret;
}

void gpexbe_notifier_term()
{
	gpexbe_notifier_internal_remove(GPU_NOTIFIER_MIN_LOCK, &gpu_min_qos_notifier);
	gpexbe_notifier_internal_remove(GPU_NOTIFIER_MAX_LOCK, &gpu_max_qos_notifier);
}
