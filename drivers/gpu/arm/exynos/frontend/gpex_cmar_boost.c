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

/* Implements */
#include <gpex_cmar_boost.h>

/* Uses */
#include <linux/sched.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <uapi/linux/sched/types.h>
#endif

#include <mali_exynos_ioctl.h>
#include <gpex_utils.h>

int gpex_cmar_boost_set_flag(struct platform_context *pctx, int request)
{
	if (!pctx)
		return -EINVAL;

	switch (request) {
	case CMAR_BOOST_SET_RT:
		pctx->cmar_boost |= CMAR_BOOST_SET_RT;
		GPU_LOG(MALI_EXYNOS_DEBUG, "cmar boost SET requested for pid(%d)", pctx->pid);
		break;

	case CMAR_BOOST_SET_DEFAULT:
		pctx->cmar_boost |= CMAR_BOOST_SET_DEFAULT;
		GPU_LOG(MALI_EXYNOS_DEBUG, "cmar boost UNSET requested for pid(%d)", pctx->pid);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static bool current_priority_is_rt(void)
{
	return current->policy > 0;
}

void gpex_cmar_boost_set_thread_priority(struct platform_context *pctx)
{
	if (!pctx)
		return;

	if (pctx->cmar_boost == (CMAR_BOOST_SET_RT | CMAR_BOOST_DEFAULT) &&
	    !current_priority_is_rt()) {
		struct sched_param param = { .sched_priority = 1 };
		int ret = 0;

		ret = sched_setscheduler_nocheck(current, SCHED_FIFO, &param);
		if (ret) {
			pctx->cmar_boost = CMAR_BOOST_DEFAULT;

			GPU_LOG(MALI_EXYNOS_WARNING, "failed to set RT SCHED_CLASS cmar-backend\n");
		} else {
			pctx->cmar_boost = CMAR_BOOST_USER_RT;

			GPU_LOG(MALI_EXYNOS_DEBUG,
				"rt priority set for tid(%d) policy(%d) prio(%d) static_prio(%d) normal_prio(%d) rt_priority(%d)",
				current->pid, current->policy, current->prio, current->static_prio,
				current->normal_prio, current->rt_priority);
		}
	} else if (pctx->cmar_boost == (CMAR_BOOST_SET_DEFAULT | CMAR_BOOST_USER_RT) &&
		   current_priority_is_rt()) {
		struct sched_param param = { .sched_priority = 0 };
		int ret = 0;

		ret = sched_setscheduler_nocheck(current, SCHED_NORMAL, &param);
		if (ret) {
			GPU_LOG(MALI_EXYNOS_WARNING,
				"failed to set default SCHED_CLASS cmar-backend\n");
		} else {
			pctx->cmar_boost = CMAR_BOOST_DEFAULT;

			GPU_LOG(MALI_EXYNOS_DEBUG,
				"default priority set for tid(%d) policy(%d) prio(%d) static_prio(%d) normal_prio(%d) rt_priority(%d)",
				current->pid, current->policy, current->prio, current->static_prio,
				current->normal_prio, current->rt_priority);
		}
	}
}
