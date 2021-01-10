/*
 *
 * (C) COPYRIGHT 2015, 2017-2020 ARM Limited. All rights reserved.
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
#include <mali_kbase_defs.h>
#include <linux/clk.h>
#include "mali_kbase_config_platform.h"
#include "gpu_control.h"

/*
 * Exynos is not using standarized clock framework to handle GPU clock,
 * so a custom implementation to handle GPU clock change notifications
 * are needed.
 * 
 * Exynos platform only uses one clock, and there is no suitable
 * clock struct to be used as "gpu_clk_handle".
 * We therefor use the platform_context as the one and only clock handle
 * 
 * A short-cut is taken with the registration of callbacks, as we only
 * allow one callback to be installed at any time. This should be enough
 * for now at least.
 */
static void *enumerate_gpu_clk(struct kbase_device *kbdev,
		unsigned int index)
{
	void *ret = NULL;
	
	if (index == 0)
		ret = kbdev->platform_context;
	
	return ret;
}

static unsigned long get_gpu_clk_rate(struct kbase_device *kbdev,
		void *gpu_clk_handle)
{
	struct exynos_context *platform = (struct exynos_context *)gpu_clk_handle;
	return gpu_get_cur_clock(platform);
}

static int gpu_clk_notifier_register(struct kbase_device *kbdev,
		void *gpu_clk_handle, struct notifier_block *nb)
{
	struct exynos_context *platform = (struct exynos_context *)gpu_clk_handle;

	compiletime_assert(offsetof(struct clk_notifier_data, clk) ==
		offsetof(struct kbase_gpu_clk_notifier_data, gpu_clk_handle),
		"mismatch in the offset of clk member");

	compiletime_assert(sizeof(((struct clk_notifier_data *)0)->clk) ==
	     sizeof(((struct kbase_gpu_clk_notifier_data *)0)->gpu_clk_handle),
	     "mismatch in the size of clk member");

	if (platform->nb_clock_change != NULL)
		return -EEXIST; /* callback already installed, do currently not support multiple */

	platform->nb_clock_change = nb;
	return 0;
}

static void gpu_clk_notifier_unregister(struct kbase_device *kbdev,
		void *gpu_clk_handle, struct notifier_block *nb)
{
	struct exynos_context *platform = (struct exynos_context *)gpu_clk_handle;
	platform->nb_clock_change = NULL;
}

struct kbase_clk_rate_trace_op_conf clk_rate_trace_ops = {
	.get_gpu_clk_rate = get_gpu_clk_rate,
	.enumerate_gpu_clk = enumerate_gpu_clk,
	.gpu_clk_notifier_register = gpu_clk_notifier_register,
	.gpu_clk_notifier_unregister = gpu_clk_notifier_unregister,
};
